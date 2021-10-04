#include <linux/kprobes.h>
#include <linux/refcount.h>

#include "ptracker.h"
#include "system_hook.h"

static bool shook_initialized = false;
static bool shook_enabled = false;
static bool system_wide_mode = false;

/* Basic implementation of safe-unmount */
static refcount_t usage = REFCOUNT_INIT(1);
static refcount_t unmounting = REFCOUNT_INIT(1);

static void dummy_ctx_func(struct task_struct *prev, bool prev_on, bool curr_on)
{
	// Empty call
}

static unsigned long *ctx_hook = (unsigned long *)dummy_ctx_func;

void switch_hook_set_state_enable(bool state)
{
	shook_enabled = state;
}
EXPORT_SYMBOL(switch_hook_set_state_enable);

void switch_hook_set_system_wide_mode(bool mode)
{
	system_wide_mode = mode;
}
EXPORT_SYMBOL(switch_hook_set_system_wide_mode);

static int context_switch_entry_handler(struct kretprobe_instance *ri,
					struct pt_regs *regs)
{
	unsigned long *prev_address = (unsigned long *)ri->data;

	/* Take the reference to prev task */
	*prev_address = regs->di;
	return 0;
}

static int context_switch_handler(struct kretprobe_instance *ri,
				  struct pt_regs *regs)
{
	struct task_struct *prev =
		(struct task_struct *)*((unsigned long *)ri->data);

	if (!shook_enabled)
		goto end;

	/* Skip if unmounting */
	if (refcount_read(&unmounting) > 1)
		goto end;

	preempt_disable();
	refcount_inc(&usage);

	if (system_wide_mode) {
		((ctx_func *)ctx_hook)(prev, true, true);
	} else {
		((ctx_func *)ctx_hook)(prev, query_tracker(prev),
				       query_tracker(current));
	}

	refcount_dec(&usage);
	preempt_enable();
end:
	return 0;
}

static struct kretprobe krp_post = { .handler = context_switch_handler,
				     .entry_handler =
					     context_switch_entry_handler,
				     .data_size = sizeof(struct task_struct *),
				     .maxactive = 8 + 1 };

/**
 * TODO - Redefine 
 * This is an unsafe way to set the callback
 */
void set_hook_callback(ctx_func *hook)
{
	if (!hook) {
		pr_warn("Hook is NULL. Setting default callback\n");
		ctx_hook = (unsigned long *)dummy_ctx_func;
	} else {
		ctx_hook = (unsigned long *)hook;
	}
}
EXPORT_SYMBOL(set_hook_callback);

static __init int switch_hook_module_init(void)
{
	int err = 0;

	/* Hook post function */
	krp_post.kp.symbol_name = SWITCH_POST_FUNC;

	err = register_kretprobe(&krp_post);
	if (err) {
		pr_warn("Cannot hook post function - ERR_CODE: %d\n", err);
		goto post_err;
	};

	err = tracker_init();
	if (err) {
		pr_warn("Cannot init tracker - ERR_CODE: %d\n", err);
		goto tracker_err;
	};

	shook_initialized = 1;

	pr_info("Module loaded\n");
	return 0;

tracker_err:
	unregister_kretprobe(&krp_post);

post_err:
	return err;
}

static void __exit switch_hook_module_exit(void)
{
	if (!shook_initialized) {
		pr_err("Whoops. The system is not hooked\n");
		return;
	}

	if (!refcount_inc_not_zero(&unmounting)) {
		pr_err("Whoops. Module exit already called\n");
		return;
	}

	shook_enabled = 0;

	/*
	 * Grace period. This may waste several cpu cycles,
	 * but it will complete eventually.
	 */
	while (refcount_read(&usage) > 1)
		;

	tracker_fini();

	unregister_kretprobe(&krp_post);

	/* nmissed > 0 suggests that maxactive was set too low. */
	if (krp_post.nmissed)
		pr_info("Missed %u invocations\n", krp_post.nmissed);

	shook_initialized = 0;

	pr_info("Module unloaded\n");
}

module_init(switch_hook_module_init);
module_exit(switch_hook_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stefano Carna'");
#include <linux/kprobes.h>
#include <linux/refcount.h>

#include "ptracker.h"
#include "system_hook.h"

static bool system_hooked = 0;
static bool hook_enabled = 0;
static bool system_wide_mode = 0;

/* Basic implementation of safe-unmount */
static refcount_t usage = REFCOUNT_INIT(1);
static refcount_t unmounting = REFCOUNT_INIT(1);

static unsigned long *ctx_hook = NULL;

void switch_hook_pause(void)
{
	hook_enabled = 0;
}
EXPORT_SYMBOL(switch_hook_pause);

void switch_hook_resume(void)
{
	hook_enabled = 1;
}
EXPORT_SYMBOL(switch_hook_resume);

void switch_hook_set_mode(unsigned mode)
{
	system_wide_mode = !!mode;
}
EXPORT_SYMBOL(switch_hook_set_mode);

int hook_register(ctx_func hook)
{
	int err = 0;

	if (!hook) {
		err = -1;
		goto end;
	}

	ctx_hook = (unsigned long *)hook;

end:
	return err;
}
EXPORT_SYMBOL(hook_register);

void hook_unregister(void)
{
	/* Already unregistrering */
	if (!refcount_inc_not_zero(&unmounting))
		return;

	/*
	 * Grace period. This may waste several cpu cycles,
	 * but it will complete eventually.
	 */
	while (refcount_read(&usage) > 1)
		;
}
EXPORT_SYMBOL(hook_unregister);

static int finish_task_switch_handler(struct kretprobe_instance *ri,
				      struct pt_regs *regs);
static int finish_task_switch_entry_handler(struct kretprobe_instance *ri,
					    struct pt_regs *regs);

static struct kretprobe krp_post = { .handler = finish_task_switch_handler,
				     .entry_handler =
					     finish_task_switch_entry_handler,
				     .data_size = sizeof(struct task_struct *),
				     .maxactive = 8 + 1 };

static int finish_task_switch_entry_handler(struct kretprobe_instance *ri,
					    struct pt_regs *regs)
{
	unsigned long *prev_address = (unsigned long *)ri->data;

	/* Take the reference to prev task */
	*prev_address = regs->di;
	return 0;
}

static int finish_task_switch_handler(struct kretprobe_instance *ri,
				      struct pt_regs *regs)
{
	struct task_struct *prev =
		(struct task_struct *)*((unsigned long *)ri->data);
	preempt_disable();

	/* Set the usage reference */
	refcount_inc(&usage);

	/* Skip if unmounting */
	if (refcount_read(&unmounting) > 1)
		goto end;

	if (!hook_enabled)
		goto end;

	if (system_wide_mode) {
		if (ctx_hook)
			((ctx_func *)ctx_hook)(prev, true, true);
	} else {
		if (ctx_hook)
			((ctx_func *)ctx_hook)(prev, is_pid_tracked(prev->tgid),
					       is_pid_tracked(current->tgid));
	}

end:
	refcount_dec(&usage);
	preempt_enable();
	return 0;
}

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

	system_hooked = 1;

	tracker_init();

	pr_info("module loaded\n");

post_err:
	return err;
}

static void __exit switch_hook_module_exit(void)
{
	if (!system_hooked) {
		pr_info("The system is not hooked\n");
		return;
	}

	hook_unregister();

	tracker_fini();

	unregister_kretprobe(&krp_post);

	/* nmissed > 0 suggests that maxactive was set too low. */
	if (krp_post.nmissed)
		pr_info("Missed %u invocations\n", krp_post.nmissed);

	system_hooked = 0;

	pr_info("module shutdown\n");
}

module_init(switch_hook_module_init);
module_exit(switch_hook_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stefano Carna'");
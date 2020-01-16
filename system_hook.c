#include <linux/kprobes.h>

#include "ptracker.h"
#include "system_hook.h"

static int system_hooked = 0;
static int hook_enabled = 0;

static DEFINE_PER_CPU(pid_t, pcpu_last_scheduled) = 0;

static struct hook_pair registred_hooks = {0, 0};

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

int hook_register(struct hook_pair *hooks)
{
	int err = 0;

	if (!hooks) {
		err = -1;
		goto end;
	}

	registred_hooks.func_pos = hooks->func_pos;
	registred_hooks.func_neg = hooks->func_neg;

end:
	return err;
}
EXPORT_SYMBOL(hook_register);


static int switch_post_handler(struct kretprobe_instance *ri, struct pt_regs *regs);


static struct kretprobe krp_post = {
	.handler = switch_post_handler,
};


static int switch_post_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	pid_t last_scheduled;
	unsigned cpu = get_cpu();

	last_scheduled = this_cpu_read(pcpu_last_scheduled);

	if (unlikely(last_scheduled <= 0)) {
		goto switch_in;
	}

	// TODO compute here the last process' data

	
switch_in:
	
	if (hook_enabled)
		pr_info("[CPU %u] LAST %u - CURRENT %u\n", cpu, last_scheduled, current->pid);

	last_scheduled = current->pid;

	if (!hook_enabled)
		goto end;

	// TODO WARNING the function can be executed while it's being unregistered
	if (is_pid_present(current->pid)) {
		if (registred_hooks.func_pos) ((h_func*) registred_hooks.func_pos)();
	} else {
		if (registred_hooks.func_neg) ((h_func*) registred_hooks.func_neg)();
	}

end:
	put_cpu();
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

	pr_info("SHOOK module loaded\n");

post_err:
	return err;
}

static void __exit switch_hook_module_exit(void)
{
	if (!system_hooked) {
		pr_info("The system is not hooked\n");
		return;
	}

	tracker_fini();

	unregister_kretprobe(&krp_post);

	/* nmissed > 0 suggests that maxactive was set too low. */
	if (krp_post.nmissed) pr_info("Missed %u invocations\n", krp_post.nmissed);

	system_hooked = 0;

	pr_info("SHOOK module shutdown\n");
}// hook_exit

module_init(switch_hook_module_init);
module_exit(switch_hook_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stefano Carna'");
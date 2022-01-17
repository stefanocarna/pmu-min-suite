#include <linux/kprobes.h>
#include <linux/refcount.h>

#include "system_hook.h"

#define CTXS_FUNC_NAME "finish_task_switch"
#define EXIT_FUNC_NAME "do_exit"
#define FORK_FUNC_NAME "_do_fork"

static struct kretprobe krp_ctxs;
static struct kretprobe krp_exit;
static struct kretprobe krp_fork;

static int ctxs_entry_handler(struct kretprobe_instance *ri,
			      struct pt_regs *regs)
{
	unsigned long *prev_address = (unsigned long *)ri->data;
	/* Take the reference to prev task */
	*prev_address = regs->di;
	return 0;
}

static int ctxs_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	struct task_struct *prev =
		(struct task_struct *)*((unsigned long *)ri->data);

	if (start_handler()) {
		hook_sched_in(NULL, false, prev, current);
		stop_handler();
	}

	return 0;
}

static int exit_entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	if (start_handler()) {
		hook_proc_exit(NULL, current);
		stop_handler();
	}
	return 0;
}

static int fork_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	unsigned long retval;
	struct task_struct *ntask;

	if (start_handler()) {
		retval = regs_return_value(regs);
		if (is_syscall_success(regs)) {
			ntask = get_pid_task(find_get_pid(retval), PIDTYPE_PID);
			hook_proc_fork(NULL, current, ntask);
		}
		stop_handler();
	}

	return 0;
}

static int init_kprobes(void)
{
	int i, err;
	struct kretprobe *krps[] = { &krp_ctxs, &krp_exit, &krp_fork };

	krp_ctxs.kp.symbol_name = CTXS_FUNC_NAME;
	krp_ctxs.handler = ctxs_handler;
	krp_ctxs.entry_handler = ctxs_entry_handler;
	krp_ctxs.data_size = sizeof(struct task_struct *),
	krp_ctxs.maxactive = num_possible_cpus() + 1;

	krp_exit.kp.symbol_name = EXIT_FUNC_NAME;
	krp_exit.entry_handler = exit_entry_handler;
	krp_exit.maxactive = num_possible_cpus() + 1;

	krp_fork.kp.symbol_name = FORK_FUNC_NAME;
	krp_fork.handler = fork_handler;
	krp_fork.maxactive = num_possible_cpus() + 1;

	for (i = 0; i < ARRAY_SIZE(krps); ++i) {
		err = register_kretprobe(krps[i]);
		if (err)
			goto err;
	}

	return 0;

err:
	pr_err("Error while registering kprobe %u\n", i);
	for (i = i - 1; i >= 0; --i)
		unregister_kretprobe(krps[i]);

	return err;
}

static void fini_kprobes(void)
{
	int i;
	struct kretprobe *krps[] = { &krp_fork, &krp_exit, &krp_ctxs};

	for (i = 0; i < ARRAY_SIZE(krps); ++i) {
		unregister_kretprobe(krps[i]);
		if (krps[i]->nmissed)
			pr_info("[%u] NMissed %u\n", i, krps[i]->nmissed);
	}
}

static __init int switch_hook_module_init(void)
{
	int err;

	err = init_kprobes();
	if (err)
		goto krpobes_err;

	err = tracker_init();
	if (err)
		goto tracker_err;

	shook_initialized = 1;

	pr_info("KPROBE MODE - Module loaded\n");

	return 0;

tracker_err:
	fini_kprobes();
krpobes_err:
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

	fini_kprobes();

	shook_initialized = 0;

	pr_info("Module unloaded\n");
}

module_init(switch_hook_module_init);
module_exit(switch_hook_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stefano Carna'");

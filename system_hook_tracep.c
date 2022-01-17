#include <linux/kprobes.h>
#include <linux/refcount.h>

#include "system_hook.h"
#include "hooks.h"

static void whook_sched_in(void *data, bool preempt, struct task_struct *prev,
			   struct task_struct *next)
{
	if (start_handler()) {
		hook_sched_in(data, preempt, prev, next);
		stop_handler();
	}
}

static void whook_proc_fork(void *data, struct task_struct *parent,
			    struct task_struct *child)
{
	if (start_handler()) {
		hook_proc_fork(data, parent, child);
		stop_handler();
	}
}

static void whook_proc_exit(void *data, struct task_struct *p)
{
	if (start_handler()) {
		hook_proc_exit(data, p);
		stop_handler();
	}
}

struct hook_func {
	void *func;
	enum hook_type type;
};

static struct hook_func hook_funcs[] = { { whook_sched_in, SCHED_IN },
					 { whook_proc_fork, PROC_FORK },
					 { whook_proc_exit, PROC_EXIT } };

int system_hooks_init(void)
{
	int i, err;

	for (i = 0; i < ARRAY_SIZE(hook_funcs); ++i) {
		err = register_hook(hook_funcs[i].type, hook_funcs[i].func);
		if (err)
			goto no_hooks;
	}

	return 0;

no_hooks:
	for (i = i - 1; i >= 0; --i)
		unregister_hook(hook_funcs[i].type, hook_funcs[i].func);

	return err;
}

void system_hooks_fini(void)
{
	uint i;

	for (i = 0; i < ARRAY_SIZE(hook_funcs); ++i)
		unregister_hook(hook_funcs[i].type, hook_funcs[i].func);
}

static __init int switch_hook_module_init(void)
{
	int err = 0;

	err = system_hooks_init();

	if (err) {
		pr_warn("Cannot register tracepoint - ERR_CODE: %d\n", err);
		goto post_err;
	};

	err = tracker_init();
	if (err) {
		pr_warn("Cannot init tracker - ERR_CODE: %d\n", err);
		goto tracker_err;
	};

	shook_initialized = 1;

	switch_hook_set_system_wide_mode(true);
	switch_hook_set_state_enable(true);

	pr_info("TRACEP MODE - Module loaded\n");
	return 0;

tracker_err:
	system_hooks_fini();

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

	system_hooks_fini();

	shook_initialized = 0;

	pr_info("Module unloaded\n");
}

module_init(switch_hook_module_init);
module_exit(switch_hook_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stefano Carna'");

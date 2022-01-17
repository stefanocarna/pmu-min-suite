#include <linux/cpumask.h>
#include <linux/kprobes.h>
#include <linux/list.h>
#include <linux/hashtable.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/wait.h>

#include "system_hook.h"
#include "ptracker.h"

static void track_ctxs_wrapper(ARGS_SCHED_IN)
{
	query_tracker(prev);
	query_tracker(next);
}

static void track_fork_wrapper(ARGS_PROC_FORK)
{
	if (query_tracker(parent)) {
		/* Enable Preemption to allow normal memory allocation */
		preempt_enable_notrace();
		local_irq_enable();
		tracker_add(child, hook_on_attach(NULL));
		local_irq_disable();
		preempt_disable_notrace();
	}
}

static void track_exit_wrapper(ARGS_PROC_EXIT)
{
	/* Delete if it exists */
	data = tracker_del(p);
	hook_on_detach(data);
}

__weak int tracker_add(struct task_struct *task, void *data)
{
	return 0;
}

__weak void *tracker_del(struct task_struct *task)
{
	return 0;
}

__weak bool query_tracker(struct task_struct *task)
{
	return false;
}

int register_hooks(void)
{
	set_shook_callback((unsigned long *)track_ctxs_wrapper, SCHED_IN);
	set_shook_callback((unsigned long *)track_fork_wrapper, PROC_FORK);
	set_shook_callback((unsigned long *)track_exit_wrapper, PROC_EXIT);

	return 0;
}

void unregister_hooks(void)
{
	set_shook_callback(NULL, SCHED_IN);
	set_shook_callback(NULL, PROC_FORK);
	set_shook_callback(NULL, PROC_EXIT);
}

__weak int tracker_init(void)
{
	return 0;
}

__weak void tracker_fini(void)
{
	/* Do nothing */
}

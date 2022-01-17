// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
#include <linux/slab.h>
#include <linux/module.h>

#include "pmu_low.h"
#include "device/proc.h"
#include "client_module.h"

#include "buffering/buffering.h"

bool __read_mostly thread_filter;

int register_process(struct task_struct *task)
{
	struct proc_data *proc_data = kmalloc(sizeof(*proc_data), GFP_KERNEL);

	if (!proc_data)
		return -ENOMEM;

	return tracker_add(task, proc_data);
}

static void *client_hook_on_attach(void *data)
{
	struct proc_data *proc_data =
		kmalloc(sizeof(struct proc_data), GFP_KERNEL);

	if (!proc_data)
		return NULL;

	proc_data->clist =
		buffering_dynamic_init(NULL, sizeof(struct proc_data));

	return proc_data;
}

static void *client_hook_on_detach(void *data)
{
	struct proc_data *proc_data = data;

	if (proc_data) {
		buffering_dynamic_fini(proc_data->clist);
		kfree(proc_data);
	}

	return NULL;
}

static void client_hook_sched_in(ARGS_SCHED_IN)
{
	bool prev_en, next_en;

	if (!thread_filter)
		return;

	prev_en = query_tracker(prev);
	next_en = query_tracker(next);

	if (prev_en) {
		/* TODO Enable - This is conflicting with PMI preemption */
		// snapshot_pmc(prev);
		if (!next_en)
			disable_pmcs_local();
	} else if (next_en) {
		enable_pmcs_local();
	}
}

static int init_shook(void)
{
	set_shook_callback((ulong *)client_hook_on_attach, ON_ATTACH);
	set_shook_callback((ulong *)client_hook_on_detach, ON_DETACH);
	set_shook_callback((ulong *)client_hook_sched_in, SCHED_IN);
	switch_hook_set_system_wide_mode(true);
	switch_hook_set_state_enable(true);

	return 0;
}

static void fini_shook(void)
{
	switch_hook_set_state_enable(false);
	switch_hook_set_system_wide_mode(false);
	set_shook_callback(NULL, SCHED_IN);
	set_shook_callback(NULL, ON_ATTACH);
	set_shook_callback(NULL, ON_DETACH);
}

static __init int client_module_init(void)
{
	int err;

	err = buffering_static_init(NULL, sizeof(struct pmcs_data));
	if (err)
		goto no_buffering;

	err = init_shook();
	if (err)
		goto no_shook;

	err = init_pmu();
	if (err)
		goto no_pmu;

	err = init_proc();
	if (err)
		goto no_proc;

	pr_info("Module loaded\n");
	return 0;

no_proc:
	fini_pmu();
no_pmu:
	fini_shook();
no_shook:
	buffering_static_fini(NULL);
no_buffering:
	return err;
}

static void __exit client_module_exit(void)
{
	fini_proc();
	fini_pmu();
	fini_shook();

	buffering_static_fini(NULL);

	pr_info("Module unloaded\n");
}

module_init(client_module_init);
module_exit(client_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stefano Carna'");

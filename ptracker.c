#include <linux/cpumask.h>
#include <linux/kprobes.h>
#include <linux/list.h>
#include <linux/hashtable.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/wait.h>

#include "system_hook.h"
#include "ptracker.h"

/* PIDs are stored into a Hashmap */
DECLARE_HASHTABLE(tracked_map, 8);

/* List of tracked processes */
static LIST_HEAD(tracked_list);
static unsigned tracked_count = 0;

bool use_pid_register_id = true;

/* Lock the list access */
static spinlock_t lock;

static void dummy_smp_func(void *dummy)
{
	// Empty call
}

static smp_call_func_t exit_callback = dummy_smp_func;

void set_exit_callback(smp_call_func_t callback)
{
	unsigned long flags;

	spin_lock_irqsave(&lock, flags);
	if (!callback) {
		pr_warn("'Exit Callback' is NULL. Setting default callback\n");
		exit_callback = dummy_smp_func;
	} else {
		exit_callback = callback;
	}
	spin_unlock_irqrestore(&lock, flags);
}
EXPORT_SYMBOL(set_exit_callback);

static int exit_pre_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	/* Directly check and remove to save double checks */
	pid_t id = (use_pid_register_id) ? current->pid : current->tgid;
	return tracker_del(id);
}

/* TODO - Fix TGID check */
static int fork_ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	unsigned long retval = 0;

	pid_t id = (use_pid_register_id) ? current->pid : current->tgid;

	if (query_tracker(id)) {
		retval = regs_return_value(regs);
		if (is_syscall_success(regs)) {
			/* ADD new pid (retval) to tracked map */
			tracker_add(retval);
		}
	}

	return 0;
}

static struct kretprobe krp_fork = {
	.handler = fork_ret_handler,
};

static struct kretprobe krp_exit = {
	.entry_handler = exit_pre_handler,
};

/**
 * Register a process/trhead to be tracked.
 * 
 * @id: is the pid or tgid according to init setup.
 */
int tracker_add(pid_t id)
{
	unsigned long flags;
	struct tp_node *tp;
	tp = kmalloc(sizeof(struct tp_node), GFP_KERNEL);
	if (!tp) {
		pr_warn("Cannot allocate memory to track ID %u\n", id);
		return - ENOMEM;
	}

	tp->id = id;

	spin_lock_irqsave(&lock, flags);
	hash_add(tracked_map, &tp->node, id);
	tracked_count++;
	spin_unlock_irqrestore(&lock, flags);

	pr_info("Registered ID %u (Current PID:%u - TGID:%u)\n", id,
		current->pid, current->tgid);

	return 0;
}
EXPORT_SYMBOL(tracker_add);

/**
 * Unregister a process/trhead from tracked ones.
 * 
 * @id: is the pid or tgid according to init setup.
 */
int tracker_del(pid_t id)
{
	unsigned long flags;
	bool last_tracked = false;
	struct tp_node *cur;
	struct hlist_node *next;

	spin_lock_irqsave(&lock, flags);
	hash_for_each_possible_safe(tracked_map, cur, next, node, id) {
		if (cur->id == id) {
			hash_del(&cur->node);
			pr_info("Unregistered ID %u\n", id);
			kfree(cur);

			tracked_count--;
			last_tracked = !tracked_count;
			break;
		}
        }
	spin_unlock_irqrestore(&lock, flags);

	if (last_tracked)
		exit_callback(NULL);

	return 0;
}
EXPORT_SYMBOL(tracker_del);

bool query_tracker(pid_t id)
{
	unsigned long flags;
	struct tp_node *cur;

	spin_lock_irqsave(&lock, flags);
	hash_for_each_possible(tracked_map, cur, node, id) {
		if (cur->id == id) {
			spin_unlock_irqrestore(&lock, flags);
			return true;
		}
	}
	spin_unlock_irqrestore(&lock, flags);
	return false;
}
EXPORT_SYMBOL(query_tracker);

int tracker_init(void)
{
	int err = 0;
	
	spin_lock_init(&lock);

	// Initialize the hashtable.
    	hash_init(tracked_map);

	krp_fork.kp.symbol_name = FORK_FUNC_NAME;
	krp_fork.maxactive = num_online_cpus() + 1;

	krp_exit.kp.symbol_name = EXIT_FUNC_NAME;
	krp_exit.maxactive = num_online_cpus() + 1;

	err = register_kretprobe(&krp_fork);
	if (err) {
		pr_err("Cannot register probe on 'fork'\n");
		goto no_fork;
	}

	err = register_kretprobe(&krp_exit);
	if (err) {
		pr_err("Cannot register probe on 'fork'\n");
		goto no_exit;
	}

	pr_info("Tracker initialized\n");
	return err;

no_exit:
	unregister_kretprobe(&krp_fork);
no_fork:
	return err;
}

void tracker_fini(void)
{
	unregister_kretprobe(&krp_fork);
	unregister_kretprobe(&krp_exit);

	pr_info("Tracker finalized\n");
}

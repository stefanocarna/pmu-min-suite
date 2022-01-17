#include <linux/cpumask.h>
#include <linux/kprobes.h>
#include <linux/list.h>
#include <linux/hashtable.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/wait.h>

#include "system_hook.h"
#include "ptracker.h"

struct tp_node {
	pid_t id;
	void *data;
	struct hlist_node node;
};

/* PIDs are stored into a Hashmap */
DECLARE_HASHTABLE(tracked_map, 8);
static int tracked_count;

/* Lock the list access */
static spinlock_t lock;

/**
 * Register a process/trhead to be tracked.
 *
 * @id: is the pid or tgid according to init setup.
 */
int tracker_add(struct task_struct *task, void *data)
{
	unsigned long flags;
	struct tp_node *tp;
	pid_t id = task->pid;

	tp = kmalloc(sizeof(struct tp_node), GFP_KERNEL);
	if (!tp)
		return -ENOMEM;

	tp->id = id;
	tp->data = data;

	spin_lock_irqsave(&lock, flags);

	hash_add(tracked_map, &tp->node, id);
	tracked_count++;

	spin_unlock_irqrestore(&lock, flags);

	// pr_info("$$$ [%u] ADD: %llx\n", task->pid, (u64)tp->data);

	return 0;
}
EXPORT_SYMBOL(tracker_add);

/**
 * Unregister a process/trhead from tracked ones.
 *
 * @id: is the pid or tgid according to init setup.
 */
void *tracker_del(struct task_struct *task)
{
	unsigned long flags;
	bool exist = false;
	struct tp_node *cur;
	struct hlist_node *next;
	pid_t id = task->pid;
	void *data = NULL;

	spin_lock_irqsave(&lock, flags);

	hash_for_each_possible_safe (tracked_map, cur, next, node, id) {
		if (cur->id == id) {
			tracked_count--;
			exist = true;
			hash_del(&cur->node);
			break;
		}
	}
	spin_unlock_irqrestore(&lock, flags);

	if (exist) {
		// pr_info("$$$ [%u] DEL: %llx\n", task->pid, (u64)cur->data);
		data = cur->data;
		kfree(cur);
	}

	/* TODO REMOVE */
	// return NULL;
	return data;
}
EXPORT_SYMBOL(tracker_del);

bool query_tracker(struct task_struct *task)
{
	bool exist = false;
	unsigned long flags;
	struct tp_node *cur;
	pid_t id = task->pid;

	/**
	 * This code may be called inside NMI. If such a case, if the preempted
	 * routine was holding the lock, there is a deadlock. There are many
	 * ways to deal with this problem, however, for the nature of the
	 * experiment, we can sync all agents (ROUTINES, IRQ and NMI)
	 */
	if (!spin_trylock_irqsave(&lock, flags))
		return NULL;

	hash_for_each_possible (tracked_map, cur, node, id)
		if (cur->id == id) {
			exist = true;
			break;
		}

	spin_unlock_irqrestore(&lock, flags);

	return exist;
}
EXPORT_SYMBOL(query_tracker);

void *query_data_tracker(struct task_struct *task)
{
	bool exist = false;
	unsigned long flags;
	struct tp_node *cur;
	pid_t id = task->pid;

	/**
	 * This code may be called inside NMI. If such a case, if the preempted
	 * routine was holding the lock, there is a deadlock. There are many
	 * ways to deal with this problem, however, for the nature of the
	 * experiment, we can sync all agents (ROUTINES, IRQ and NMI)
	 */
	if (!spin_trylock_irqsave(&lock, flags))
		return NULL;

	hash_for_each_possible (tracked_map, cur, node, id)
		if (cur->id == id) {
			exist = true;
			break;
		}

	spin_unlock_irqrestore(&lock, flags);

	/* TODO REMOVE */
	// return NULL;
	return (exist) ? cur->data : NULL;
}
EXPORT_SYMBOL(query_data_tracker);

int tracker_init(void)
{
	hash_init(tracked_map);
	spin_lock_init(&lock);

	register_hooks();

	pr_info("HASH - Tracker initialized\n");

	return 0;
}

void tracker_fini(void)
{
	unregister_hooks();

	pr_info("HASH - Tracker finalized\n");
}

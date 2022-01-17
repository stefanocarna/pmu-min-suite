#include <linux/sched/task_stack.h>

#include "system_hook.h"
#include "ptracker.h"

/**
 * [0:47]       buffer pointer
 * [48:61]	crc
 * [62]		p bit
 * [63]		e bit
 */
#define PTR_BITS 48
#define PTR_THR_BIT (1ULL << (PTR_BITS - 1)) /* threshold bit */
#define PTR_USED_MASK ((1ULL << PTR_BITS) - 1) // 0x0000F...F
#define PTR_FREE_MASK (~PTR_USED_MASK) // 0xFFFF0...0
#define PTR_CRC_MASK (((1ULL << (64 - PTR_BITS - 2)) - 1) << 48) // 0x3FFF0...0
#define PTR_CCB_MASK ((1ULL << (64 - PTR_BITS - 2)) - 1) // 0x0...03FFF
#define PROCESS_BIT 62
#define ENABLED_BIT 63
#define PROCESS_MASK (1ULL << PROCESS_BIT)
#define ENABLED_MASK (1ULL << ENABLED_BIT)

#define CRC_MAGIC 0xc
#define set_crc(ctl) (ctl.crc = ctl.ccb ^ CRC_MAGIC)
#define check_crc(ctl)                                                         \
	(((ctl & PTR_CCB_MASK) ^ CRC_MAGIC) == ((ctl & PTR_CRC_MASK) >> 48))
#define build_ptr(ptr)                                                         \
	((ptr & PTR_THR_BIT) ? PTR_FREE_MASK | ptr : PTR_USED_MASK & ptr)

#define last_entry_size(task, size)                                            \
	({                                                                     \
		ulong stack = (ulong)task_stack_page(task);                    \
		!!stack ? (ulong *)(stack + size) : NULL;                      \
	})

static int patch_thread_stack(struct task_struct *task, void *data)
{
	int err = 0;
	unsigned long flags;
	unsigned long magic_entry;
	unsigned long *last_entry;

	if (!task)
		return -EINVAL;

	local_irq_save(flags);

	last_entry = last_entry_size(task, sizeof(ulong));

	if (!last_entry) {
		err = -EINVAL;
		goto no_entry;
	}

	/* the last magic_entry position contains the pt buffer addr */
	magic_entry = ((unsigned long)data) & PTR_USED_MASK;
	/* set the crc */
	magic_entry |= ((magic_entry & PTR_CCB_MASK) ^ CRC_MAGIC) << 48;
	/* clear processing bit */
	magic_entry &= ~PROCESS_MASK;
	/* set enable bit */
	magic_entry |= ENABLED_MASK;

	*last_entry = (unsigned long)magic_entry;

no_entry:
	local_irq_restore(flags);

	/**
	 * Remember that the stack grows down, so the last entry is pointed by the task->stack.
	 * If CONFIG_STACK_GROWSUP is defined, the stack grows up, so the last entry is
	 * pointed by (task->stack + THREAD_SIZE) - (sizeof(unsigned long)).
	 * If CONFIG_THREAD_INFO_IN_TASK is defined, end_of_stack macro points to the entry
	 * containing the magic number for corruption detection.
	 */

	return err;
}

static int restore_thread_stack(struct task_struct *task)
{
	unsigned long flags;
	unsigned long *last_entry;

	if (!task)
		return -EINVAL;

	local_irq_save(flags);

	last_entry = last_entry_size(task, sizeof(ulong));
	if (last_entry)
		*last_entry = 0UL;

	local_irq_restore(flags);

	return 0;
}

static unsigned long *get_magic_entry(struct task_struct *task)
{
	unsigned long *last_entry;

	last_entry = last_entry_size(task, sizeof(ulong));

	if (!last_entry)
		return NULL;

	if (!check_crc((*last_entry)) || !(*last_entry & ENABLED_MASK))
		return NULL;

	return last_entry;
}

static void *get_patched_data(struct task_struct *task)
{
	unsigned long *magic_entry = get_magic_entry(task);

	if (!magic_entry)
		return NULL;

	/* build canonical form pointer */
	return (void *)build_ptr(*magic_entry);
}

/**
 * Register a process/trhead to be tracked.
 *
 * @id: is the pid or tgid according to init setup.
 */
int tracker_add(struct task_struct *task, void *data)
{
	patch_thread_stack(task, data);
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
	void *data = NULL;
	unsigned long *magic_entry = get_magic_entry(task);

	if (magic_entry) {
		data = get_patched_data(task);
		restore_thread_stack(task);
	}

	return data;
}
EXPORT_SYMBOL(tracker_del);

bool query_tracker(struct task_struct *task)
{
	return !!get_magic_entry(task);
}
EXPORT_SYMBOL(query_tracker);

void *query_data_tracker(struct task_struct *task)
{
	return get_patched_data(task);
}
EXPORT_SYMBOL(query_data_tracker);

int tracker_init(void)
{
	register_hooks();

	pr_info("STACK - Tracker initialized\n");

	return 0;
}

void tracker_fini(void)
{
	unregister_hooks();

	pr_info("STACK - Tracker finalized\n");
}

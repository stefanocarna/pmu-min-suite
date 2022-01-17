#include <linux/perf_event.h>

#include "system_hook.h"
#include "ptracker.h"

DEFINE_PER_CPU(bool, pcpu_removing_event) = false;

static inline struct perf_event *__get_perf_event(struct task_struct *task)
{
	/* Should we use the mutex here? */
	return list_first_entry_or_null(&(task->perf_event_list),
					struct perf_event, owner_entry);
}

static struct perf_event *__get_task_event(struct task_struct *task)
{
	/* We preempted a removing operation */
	if (this_cpu_read(pcpu_removing_event))
		return NULL;

	return __get_perf_event(task);
}

static void *get_task_data(struct task_struct *task)
{
	struct perf_event *event = __get_task_event(task);

	if (!event)
		return NULL;

	return (void *)event->pmu_private;
}

static bool create_task_data(struct task_struct *task, void *data)
{
	struct perf_event *event;
	struct perf_event_attr attr = {
		.type = PERF_TYPE_SOFTWARE,
		.config = PERF_COUNT_SW_DUMMY,
		.size = sizeof(attr),
		.disabled = true,
	};

	if (!task)
		return false;

	event = perf_event_create_kernel_counter(&attr, -1, task, NULL, NULL);

	if (IS_ERR(event)) {
		pr_info("CANNOT MOCKUP thread data. perf_event_create failed: %ld\n",
			PTR_ERR(event));
		return false;
	}

	/* TODO Place here data */
	event->pmu_private = data;

	mutex_lock(&task->perf_event_mutex);
	list_add_tail(&event->owner_entry, &task->perf_event_list);
	mutex_unlock(&task->perf_event_mutex);

	return true;
}

static void destroy_task_data(struct task_struct *task)
{
	struct perf_event *event;

	preempt_disable_notrace();
	this_cpu_write(pcpu_removing_event, true);

	event = __get_perf_event(task);

	if (!event)
		goto end;

	if (event->pmu_private) {
		/* TODO Free here data */
		event->pmu_private = NULL;
	}

	perf_event_release_kernel(event);

	mutex_lock(&task->perf_event_mutex);
	list_del(&event->owner_entry);
	mutex_unlock(&task->perf_event_mutex);

end:
	this_cpu_write(pcpu_removing_event, false);
	preempt_enable_notrace();
}

/**
 * Register a process/trhead to be tracked.
 *
 * @id: is the pid or tgid according to init setup.
 */
int tracker_add(struct task_struct *task, void *data)
{
	if (!__get_task_event(task))
		create_task_data(task, data);

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
	struct perf_event *event = __get_task_event(task);

	if (event) {
		data = event->pmu_private;
		destroy_task_data(task);
	}

	return data;
}
EXPORT_SYMBOL(tracker_del);

bool query_tracker(struct task_struct *task)
{
	return !!__get_task_event(task);
}
EXPORT_SYMBOL(query_tracker);

void *query_data_tracker(struct task_struct *task)
{
	return get_task_data(task);
}
EXPORT_SYMBOL(query_data_tracker);

int tracker_init(void)
{
	register_hooks();

	pr_info("PERF - Tracker initialized\n");

	return 0;
}

void tracker_fini(void)
{
	unregister_hooks();

	pr_info("PERF - Tracker finalized\n");
}

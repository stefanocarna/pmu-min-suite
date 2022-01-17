#include <linux/kprobes.h>
#include <linux/refcount.h>

#include "system_hook.h"

bool shook_enabled;
bool system_wide_mode;
bool shook_initialized;

/* Basic implementation of safe-unmount */
refcount_t usage = REFCOUNT_INIT(1);
refcount_t unmounting = REFCOUNT_INIT(1);


void dummy_sched_in(ARGS_SCHED_IN)
{
}
void dummy_proc_fork(ARGS_PROC_FORK)
{
}
void dummy_proc_exit(ARGS_PROC_EXIT)
{
}
void *dummy_on_attach(void *data)
{
	return NULL;
}
void *dummy_on_detach(void *data)
{
	return NULL;
}

bool start_handler(void)
{
	if (!shook_enabled)
		return false;

	/* Skip if unmounting */
	if (refcount_read(&unmounting) > 1)
		return false;

	preempt_disable_notrace();
	refcount_inc(&usage);
	return true;
}

void stop_handler(void)
{
	refcount_dec(&usage);
	preempt_enable_notrace();
}

void (*hook_sched_in)(ARGS_SCHED_IN) = dummy_sched_in;
void (*hook_proc_fork)(ARGS_PROC_FORK) = dummy_proc_fork;
void (*hook_proc_exit)(ARGS_PROC_EXIT) = dummy_proc_exit;
/* Used when a new process is automatically attached */
void *(*hook_on_attach)(void *data) = dummy_on_attach;
void *(*hook_on_detach)(void *data) = dummy_on_detach;

#define assign_hook(name, hook)                                                \
	do {                                                                   \
		if (hook)                                                      \
			WRITE_ONCE(hook_##name, (typeof(hook_##name))hook);                         \
		else                                                           \
			WRITE_ONCE(hook_##name, dummy_##name);                 \
	} while (0)

__weak void set_shook_callback(unsigned long *hook, enum hook_type type)
{
	switch (type) {
	case SCHED_IN:
		assign_hook(sched_in, hook);
		break;
	case PROC_FORK:
		assign_hook(proc_fork, hook);
		break;
	case PROC_EXIT:
		assign_hook(proc_exit, hook);
		break;
	case ON_ATTACH:
		assign_hook(on_attach, hook);
		break;
	case ON_DETACH:
		assign_hook(on_detach, hook);
		break;
	default:
		pr_err("Unhandled hook_type. Skip\n");
	}
}
EXPORT_SYMBOL(set_shook_callback);

__weak void switch_hook_set_state_enable(bool state)
{
	shook_enabled = state;
}
EXPORT_SYMBOL(switch_hook_set_state_enable);

__weak void switch_hook_set_system_wide_mode(bool mode)
{
	system_wide_mode = mode;
}
EXPORT_SYMBOL(switch_hook_set_system_wide_mode);

__weak int register_fork_hook(void)
{
	return 0;
}
__weak int register_exit_hook(void)
{
	return 0;
}

__weak void unregister_fork_hook(void)
{
	/* Do nothing */
}
__weak void unregister_exit_hook(void)
{
	/* Do nothing */
}

__weak int register_hook(enum hook_type type, void *func)
{
	return 0;
}

__weak void unregister_hook(enum hook_type type, void *func)
{
	/* Do nothing */
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stefano Carna'");

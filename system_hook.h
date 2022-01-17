/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _SWITCH_HOOK_H
#define _SWITCH_HOOK_H

#include <linux/version.h>
#include "hooks.h"
#include "ptracker.h"

#define MODNAME "SHook"

#undef pr_fmt
#define pr_fmt(fmt) MODNAME ": " fmt

extern bool start_handler(void);
extern void stop_handler(void);

// extern unsigned long *hook_sched_in;
// extern unsigned long *hook_proc_fork;
// extern unsigned long *hook_proc_exit;

extern void (*hook_sched_in)(ARGS_SCHED_IN);
extern void (*hook_proc_fork)(ARGS_PROC_FORK);
extern void (*hook_proc_exit)(ARGS_PROC_EXIT);
extern void *(*hook_on_attach)(void *data);
extern void *(*hook_on_detach)(void *data);

extern void set_shook_callback(unsigned long *hook, enum hook_type type);
extern void switch_hook_set_state_enable(bool state);
extern void switch_hook_set_system_wide_mode(bool mode);

extern int register_fork_hook(void);
extern int register_exit_hook(void);

extern void unregister_fork_hook(void);
extern void unregister_exit_hook(void);

extern bool shook_enabled;
extern bool system_wide_mode;
extern bool shook_initialized;
extern refcount_t usage;
extern refcount_t unmounting;
extern unsigned long *ctx_hook;

#endif /* _SWITCH_HOOK_H */

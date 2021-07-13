#ifndef _SWITCH_HOOK_H
#define _SWITCH_HOOK_H

#define MODNAME	"SHook"

#undef pr_fmt
#define pr_fmt(fmt) MODNAME ": " fmt


// #if LINUX_VERSION_CODE >= KERNEL_VERSION(5,11,24)
#define SWITCH_POST_FUNC "schedule_tail"
// #else
// #define SWITCH_POST_FUNC "finish_task_switch"
// #endif

typedef void ctx_func(struct task_struct *prev, bool prev_on, bool curr_on);

extern struct hook_pair registred_hooks;

extern void switch_hook_set_state_enable(bool state);

extern void switch_hook_set_system_wide_mode(bool mode);

extern void set_hook_callback(ctx_func *hook);

#endif /* _SWITCH_HOOK_H */
#ifndef _SWITCH_HOOK_H
#define _SWITCH_HOOK_H

#define SWITCH_POST_FUNC "finish_task_switch"

typedef void ctx_func(struct task_struct *prev, bool prev_on, bool curr_on);

extern struct hook_pair registred_hooks;

extern void switch_hook_pause(void);

extern void switch_hook_resume(void);

extern void switch_hook_set_mode(unsigned mode);

extern int hook_register(ctx_func *hook);

// TODO inc the refcount
extern void hook_unregister(void);

#endif /* _SWITCH_HOOK_H */
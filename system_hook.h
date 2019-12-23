#ifndef _SWITCH_HOOK_H
#define _SWITCH_HOOK_H

#define SWITCH_PRE_FUNC "perf_event_task_sched_out"
#define SWITCH_POST_FUNC "finish_task_switch"

typedef void h_func(void);

struct hook_pair {
	h_func *func_pos;
	h_func *func_neg;
};

extern void switch_hook_pause(void);

extern void switch_hook_resume(void);

extern int hook_register(struct hook_pair *hooks);

#endif /* _SWITCH_HOOK_H */
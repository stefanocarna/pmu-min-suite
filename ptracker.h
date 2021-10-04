#ifndef _PTRACKER_H
#define _PTRACKER_H

#include <linux/version.h>

#define EXIT_FUNC_NAME	"do_exit"
#define EXEC_FUNC_NAME 	"do_execve"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
#define FORK_FUNC_NAME 	"kernel_clone"
#else
#define FORK_FUNC_NAME 	"_do_fork"
#endif

extern bool use_pid_register_id;

#define TRACKER_GET_ID(tsk) ((use_pid_register_id) ? tsk->pid : tsk->tgid)

extern int tracker_add(struct task_struct *tsk);
extern int tracker_del(struct task_struct *tsk);
extern bool query_tracker(struct task_struct *tsk);

extern int tracker_init(void);
extern void tracker_fini(void);

extern void set_exit_callback(smp_call_func_t callback);

struct tp_node {
	pid_t id;
	unsigned counter;
	struct hlist_node node;
};

#endif /* _PTRACKER_H */
#ifndef _PTRACKER_H
#define _PTRACKER_H

#define EXIT_FUNC_NAME	"do_exit"
#define FORK_FUNC_NAME 	"_do_fork"
#define EXEC_FUNC_NAME 	"do_execve"

extern int pid_register(pid_t pid);

extern int pid_unregister(pid_t pid);

extern bool is_pid_tracked(pid_t pid);

extern unsigned get_tracked_pids(void);

extern int tracker_init(void);
extern void tracker_fini(void);

extern void wait_track(void);

extern void set_exit_callback(smp_call_func_t callback);

struct tracked_process {
	pid_t pid;
	struct list_head list;
};

#endif /* _PTRACKER_H */
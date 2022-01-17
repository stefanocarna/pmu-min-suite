
#ifndef _PROC_H
#define _PROC_H

#include <linux/proc_fs.h>
#include <linux/sched/task.h>
#include <linux/version.h>

#include "../client_module.h"

/* Configure your paths */
#define PROC_TOP "client_module"

/* Uility */
#define _STRINGIFY(s) s
#define STRINGIFY(s) _STRINGIFY(s)
#define PATH_SEP "/"
#define GET_PATH(s) STRINGIFY(PROC_TOP) STRINGIFY(PATH_SEP) s
#define GET_SUB_PATH(p, s) STRINGIFY(p)##s

extern struct proc_dir_entry *root_pd_dir;

/* Proc and subProc functions  */
extern int init_proc(void);
extern void fini_proc(void);

extern int register_proc_processes(void);

#endif /* _PROC_H */
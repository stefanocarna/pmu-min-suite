#ifndef _PTRACKER_H
#define _PTRACKER_H

#include <linux/version.h>

extern int tracker_add(struct task_struct *task, void *data);
extern void *tracker_del(struct task_struct *task);
extern bool query_tracker(struct task_struct *task);
extern void *query_data_tracker(struct task_struct *task);

extern int register_hooks(void);
extern void unregister_hooks(void);

extern int tracker_init(void);
extern void tracker_fini(void);

#endif /* _PTRACKER_H */
#include "proc.h"

#define SUCCESS_OR_EXIT(f)                                                     \
	do {                                                                   \
		err = f();                                                     \
		if (err)                                                       \
			goto no_proc;                                          \
	} while (0)

struct proc_dir_entry *root_pd_dir;

int init_proc(void)
{
	int err = 0;

	root_pd_dir = proc_mkdir(PROC_TOP, NULL);

	if (!root_pd_dir)
		goto no_dir;

	SUCCESS_OR_EXIT(register_proc_processes);

	return err;

no_proc:
	proc_remove(root_pd_dir);
no_dir:
	return err;
}

void fini_proc(void)
{
	proc_remove(root_pd_dir);
}

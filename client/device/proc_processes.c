/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#include <linux/vmalloc.h>

#include "proc.h"

/*
 * Proc and Fops related to Tracked processes
 */

static ssize_t processes_write(struct file *file, const char __user *buffer,
			       size_t count, loff_t *ppos)
{
	int err;
	pid_t pidp;
	struct task_struct *task;

	err = kstrtouint_from_user(buffer, count, 10, &pidp);
	if (err)
		goto err_read;

	/* Retrieve pid task_struct */
	task = get_pid_task(find_get_pid(pidp), PIDTYPE_PID);
	if (!task) {
		pr_info("Cannot find task_struct for pid %u\n", pidp);
		err = -EINVAL;
		goto err_pid;
	}

	err = register_process(task);
	if (err) {
		pr_err("Cannot register pid: %u. (%d)\n", pidp, err);
		goto err_reg;
	}

	put_task_struct(task);

	pr_info("Registrered pid: %u\n", pidp);

	return count;

err_reg:
	put_task_struct(task);
err_pid:
err_read:
	return err;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
struct file_operations processes_proc_fops = {
	.write = processes_write,
#else
struct proc_ops processes_proc_fops = {
	.proc_write = processes_write,
#endif
};

int register_proc_processes(void)
{
	struct proc_dir_entry *dir;

	dir = proc_create(GET_PATH("processes"), 0222, NULL,
			  &processes_proc_fops);

	return !dir;
}
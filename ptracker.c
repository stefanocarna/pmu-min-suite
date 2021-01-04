#include <linux/cpumask.h>
#include <linux/kprobes.h>
#include <linux/list.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/wait.h>

#include "system_hook.h"
#include "ptracker.h"

/* List of tracked processes */
static LIST_HEAD(tracked_list);
static unsigned tracked_count = 0;

/* Lock the list access */
static spinlock_t lock;

static void smp_dummy(void *dummy)
{
}

smp_call_func_t exit_callback = smp_dummy;

void set_exit_callback(smp_call_func_t callback)
{
	spin_lock(&lock);
	exit_callback = callback ? callback : smp_dummy;
	spin_unlock(&lock);
}
EXPORT_SYMBOL(set_exit_callback);

static int exit_pre_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	/* Directly check and remove to save double checks */
	pid_unregister(current->pid);
	return 0;
}

static int fork_ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	unsigned long retval = 0;

	if (is_pid_tracked(current->tgid)) {
		retval = regs_return_value(regs);
		if (is_syscall_success(regs)) {
			/* ADD new pid (retval) to tracked list */
			pid_register(retval);
		}
	}

	return 0;
}

static struct kretprobe krp_fork = {
	.handler = fork_ret_handler,
};

static struct kretprobe krp_exit = {
	.entry_handler = exit_pre_handler,
};

int pid_register(pid_t pid)
{
	struct tracked_process *tp;
	tp = kmalloc(sizeof(struct tracked_process), GFP_KERNEL);
	tp->pid = pid;

	spin_lock(&lock);
	list_add(&tp->list, &tracked_list);
	tracked_count++;
	spin_unlock(&lock);

	pr_info("Registered pid %u, current: %u (TGID %u)\n", pid, current->pid,
		current->tgid);

	return 0;
}
EXPORT_SYMBOL(pid_register);

int pid_unregister(pid_t pid)
{
	struct list_head *p;
	struct tracked_process *tp;
	bool last_tracked = false;

	spin_lock(&lock);
	list_for_each (p, &tracked_list) {
		tp = list_entry(p, struct tracked_process, list);
		if (tp->pid == pid) {
			list_del(p);
			pr_info("Unregistered pid %u\n", pid);
			kfree(tp);
			tracked_count--;
			last_tracked = !tracked_count;
			goto end;
		}
	}
end:
	spin_unlock(&lock);

	if (last_tracked)
		exit_callback(NULL);
	return 0;
}
EXPORT_SYMBOL(pid_unregister);

bool is_pid_tracked(pid_t pid)
{
	struct list_head *p;
	struct tracked_process *tp;

	spin_lock(&lock);
	list_for_each (p, &tracked_list) {
		tp = list_entry(p, struct tracked_process, list);
		if (tp->pid == pid) {
			spin_unlock(&lock);
			return true;
		}
	}
	spin_unlock(&lock);
	return false;
}
EXPORT_SYMBOL(is_pid_tracked);

unsigned get_tracked_pids(void)
{
	return tracked_count;
}
EXPORT_SYMBOL(get_tracked_pids);

int tracker_init(void)
{
	int err = 0;
	tracked_count = 0;

	spin_lock_init(&lock);

	krp_fork.kp.symbol_name = FORK_FUNC_NAME;
	krp_fork.maxactive = num_online_cpus() + 1;

	krp_exit.kp.symbol_name = EXIT_FUNC_NAME;
	krp_exit.maxactive = num_online_cpus() + 1;

	err = register_kretprobe(&krp_fork);
	if (err) {
		pr_err("Cannot register probe on 'fork'\n");
		goto end;
	}

	err = register_kretprobe(&krp_exit);
	if (err) {
		pr_err("Cannot register probe on 'fork'\n");
		goto end;
	}

	pr_info("Tracker correctly initialized\n");

end:
	return err;
}

void tracker_fini(void)
{
	unregister_kretprobe(&krp_exit);
	unregister_kretprobe(&krp_fork);

	pr_info("Tracker correctly uninstalled\n");
}

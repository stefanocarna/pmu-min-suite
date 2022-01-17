#include <linux/seq_file.h>

#include "../client/client_module.h"
#include "../client/pmu_low.h"
#include "breader_module.h"

#define FILE_NAME "breader_seq"
static struct proc_dir_entry *proc_entry;

static void *__breader_seq_next(loff_t *pos)
{
	if (*pos >= DATA_BUFFER_LEN)
		return NULL;

	return &data_buffer[*pos];
}

static void *breader_seq_start(struct seq_file *m, loff_t *pos)
{
	return __breader_seq_next(pos);
}

static void *breader_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
	(*pos)++;
	return __breader_seq_next(pos);
}

static int breader_seq_show(struct seq_file *m, void *v)
{
	uint pmc;

	u64 *pmcs_data = v;

	for_each_pmc(pmc, 8)
		seq_printf(m, ",%llx", pmcs_data[pmc]);

	return 0;
}

static void breader_seq_stop(struct seq_file *m, void *v)
{
	seq_puts(m, "\n");
	/* Nothing to free */
}

static struct seq_operations breader_seq_ops = {
	.start = breader_seq_start,
	.next = breader_seq_next,
	.stop = breader_seq_stop,
	.show = breader_seq_show
};

static int breader_open(struct inode *inode, struct file *filp)
{
	return seq_open(filp, &breader_seq_ops);
}

static const struct file_operations breader_fops = {
	.open		= breader_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release
};

int init_seq_file(void)
{
	proc_entry = proc_create(FILE_NAME, 0444, NULL, &breader_fops);

	if (!proc_entry)
		return -ENODEV;

	pr_info("Created %s\n", FILE_NAME);

	return 0;
}

void fini_seq_file(void)
{
	proc_remove(proc_entry);
}

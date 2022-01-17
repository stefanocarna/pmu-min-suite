#include <linux/uaccess.h>

#include "../client/client_module.h"
#include "../client/pmu_low.h"
#include "breader_module.h"

#define FILE_NAME "breader_raw"
static struct proc_dir_entry *proc_entry;

ssize_t breader_read(struct file *file, char __user *buf, size_t count,
		     loff_t *ppos)
{
	ssize_t failed;
	int start, to_copy;

	/* Read only aligned bytes */
	if (*ppos & 0x7)
		return -EINVAL;

	start = *ppos >> 3;

	if (start == DATA_BUFFER_LEN)
		return 0;

	if (start > DATA_BUFFER_LEN || count < sizeof(u64))
		return -EINVAL;

	to_copy = (DATA_BUFFER_LEN - start) * sizeof(u64);

	if (count < to_copy)
		to_copy = count;

	to_copy = count >> 3;

	failed = copy_to_user(buf, data_buffer, to_copy);
	to_copy -= failed;

	*ppos += to_copy;

	return to_copy;
}

static const struct file_operations breader_fops = {
	.llseek = generic_file_llseek,
	.read = breader_read,
	.open = simple_open,
};

int init_raw_file(void)
{
	proc_entry = proc_create(FILE_NAME, 0444, NULL, &breader_fops);

	if (!proc_entry)
		return -ENODEV;

	pr_info("Created %s\n", FILE_NAME);

	return 0;
}

void fini_raw_file(void)
{
	proc_remove(proc_entry);
}

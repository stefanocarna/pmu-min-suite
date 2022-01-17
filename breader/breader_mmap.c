#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h> /* min */
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h> /* copy_from_user, copy_to_user */
#include <linux/slab.h>

#include "../client/client_module.h"
#include "../client/pmu_low.h"
#include "breader_module.h"

#define FILE_NAME "breader_mmap"
static struct proc_dir_entry *proc_entry;

static void vm_close(struct vm_area_struct *vma)
{
	/* Do nothing */
}

/* First page access. */
static vm_fault_t vm_fault(struct vm_fault *vmf)
{
	struct page *page;

	if (data_buffer) {
		page = virt_to_page(data_buffer);
		get_page(page);
		vmf->page = page;
	}
	return 0;
}

static void vm_open(struct vm_area_struct *vma)
{
	/* Do nothing */
}

static struct vm_operations_struct vm_ops = {
	.close = vm_close,
	.fault = vm_fault,
	.open = vm_open,
};

static int mmap(struct file *filp, struct vm_area_struct *vma)
{
	vma->vm_ops = &vm_ops;
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
	vma->vm_private_data = filp->private_data;
	vm_open(vma);
	return 0;
}

static const struct file_operations breader_fops = {
	.mmap = mmap,
	.llseek = generic_file_llseek,
	.open = simple_open,
};

int init_mmap_file(void)
{
	proc_entry = proc_create(FILE_NAME, 0666, NULL, &breader_fops);

	if (!proc_entry)
		return -ENODEV;

	pr_info("Created %s\n", FILE_NAME);

	return 0;
}

void fini_mmap_file(void)
{
	proc_remove(proc_entry);
}

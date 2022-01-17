// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
#include <linux/slab.h>
#include <linux/module.h>
#include <asm/string_64.h>

#include "breader_module.h"

u64 *data_buffer;
struct proc_dir_entry *proc_entry;

// #define CODE	(0xabcd00dead00abcdULL)
#define CODE	(0x1ULL)

int init_data(void)
{
	data_buffer = (u64 *)__get_free_pages(GFP_KERNEL,
							   DATA_BUFFER_ORDER);

	if (!data_buffer)
		return -ENOMEM;

	memset64((u64 *)data_buffer, CODE, DATA_BUFFER_SIZE_U64);

	pr_info("SEtting %lu\n", DATA_BUFFER_LEN);

	memset(&data_buffer[DATA_BUFFER_LEN - 1], 0, sizeof(u64));

	return 0;
}

void fini_data(void)
{
	free_pages((ulong)data_buffer, DATA_BUFFER_ORDER);
}

static __init int breader_module_init(void)
{
	int err;

	err = init_data();
	if (err)
		goto no_data;

	err = init_seq_file();
	if (err)
		goto no_seq_proc;

	err = init_raw_file();
	if (err)
		goto no_raw_proc;

	err = init_mmap_file();
	if (err)
		goto no_mmap_proc;

	pr_info("Module Loaded");

	return 0;

no_mmap_proc:
	fini_raw_file();
no_raw_proc:
	fini_seq_file();
no_seq_proc:
	fini_data();
no_data:
	return err;
}

static void __exit breader_module_exit(void)
{
	fini_mmap_file();
	fini_raw_file();
	fini_seq_file();

	fini_data();

	pr_info("Module unloaded\n");
}

module_init(breader_module_init);
module_exit(breader_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stefano Carna'");

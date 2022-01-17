// SPDX-License-Identifier: GPL-2.0
#include "clist.h"
#include <linux/printk.h>

static u64 dummy[1];

__weak void *init_clist(size_t entry_count, size_t entry_size)
{
	return dummy;
}

__weak void fini_clist(void *clist)
{
	/* Do nothing */
}

__weak int read_clist(void *clist, void *bucket)
{
	return 0;
}

__weak void write_clist(void *clist, void *bucket)
{
	/* Do nothing */
}

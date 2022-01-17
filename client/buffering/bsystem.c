// SPDX-License-Identifier: GPL-2.0
#include <linux/refcount.h>

#include "buffering.h"
#include "../clist/clist.h"

refcount_t usage = REFCOUNT_INIT(1);
static bool removing;
static void *system_clist;

int buffering_static_init(void *info, size_t entry_size)
{
	size_t buff_entries = GLOBAL_BUFFER_SIZE / entry_size;

	system_clist = init_clist(buff_entries, entry_size);

	return !system_clist;
}

void buffering_static_fini(void *info)
{
	removing = true;

	while (refcount_read(&usage) > 1)
		;

	fini_clist(system_clist);
}

int write_sample(void *info, u8 *sample)
{
	if (removing)
		return 0;

	refcount_inc(&usage);
	write_clist(system_clist, sample);
	refcount_dec(&usage);
	return 0;
}

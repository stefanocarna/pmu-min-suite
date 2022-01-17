// SPDX-License-Identifier: GPL-2.0
#include <uapi/asm-generic/errno-base.h>

#include "buffering.h"
#include "../clist/clist.h"

void *buffering_dynamic_init(void *info, size_t entry_size)
{
	struct clist *process_clist;
	size_t buff_entries = LOCAL_BUFFER_SIZE / entry_size;

	process_clist = init_clist(buff_entries, entry_size);

	return process_clist;
}

void buffering_dynamic_fini(void *info)
{
	struct clist *process_clist = info;

	fini_clist(process_clist);
}

int write_sample(void *info, u8 *sample)
{
	struct clist *process_clist = info;

	if (process_clist) {
		write_clist(process_clist, sample);
		return 0;
	}

	return -EINVAL;
}

// SPDX-License-Identifier: GPL-2.0
#include "buffering.h"
#include "../clist/clist.h"

__weak int buffering_static_init(void *info, size_t entry_size)
{
	return 0;
}

__weak void *buffering_dynamic_init(void *info, size_t entry_size)
{
	return NULL;
}

__weak void buffering_dynamic_fini(void *info)
{
	/* Do nothing */
}

__weak void buffering_static_fini(void *info)
{
	/* Do nothing */
}

__weak int write_sample(void *info, u8 *sample)
{
	return 0;
}

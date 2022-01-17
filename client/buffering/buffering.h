/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _BUFFERING_H
#define _BUFFERING_H

#include <linux/types.h>
#include <asm/page_types.h>

#define LOCAL_BUFFER_SIZE (PAGE_SIZE * 16)

#define GLOBAL_BUFFER_SIZE (PAGE_SIZE * 256) // 1Mb

int buffering_static_init(void *info, size_t entry_size);

void *buffering_dynamic_init(void *info, size_t entry_size);

void buffering_dynamic_fini(void *info);

void buffering_static_fini(void *info);

int write_sample(void *info, u8 *sample);

#endif /* _BUFFERING_H */

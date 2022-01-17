/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _CLIST_H
#define _CLIST_H

#include <linux/types.h>

void *init_clist(size_t entry_count, size_t entry_size);

void fini_clist(void *clist);

int read_clist(void *clist, void *bucket);

void write_clist(void *clist, void *bucket);

#endif /* _CLIST_H */
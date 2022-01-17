// SPDX-License-Identifier: GPL-2.0
#include <linux/spinlock.h>
#include <linux/mm.h>
#include "clist.h"

#define TAIL_MASK ((1ULL << 32) - 1)
#define HEAD_MASK (~TAIL_MASK)
#define TAIL(t) (t & TAIL_MASK)
#define HEAD(h) ((h & HEAD_MASK) >> 32)

struct carray {
	size_t len;
	size_t stride;
	int idx;
	spinlock_t lock;
	u8 buf[];
};

void *init_clist(size_t entry_count, size_t entry_size)
{
	size_t size = sizeof(struct carray) + (entry_count * entry_size);
	struct carray *clist = kvmalloc(size, GFP_KERNEL);

	if (!clist)
		return NULL;

	spin_lock_init(&clist->lock);

	clist->len = entry_count;
	clist->stride = entry_size;
	clist->idx = 0;

	return clist;
}

void fini_clist(void *clist)
{
	kvfree(clist);
}

int read_clist(void *clist, void *bucket)
{
	/* Never called outside interrupt context, thus, never called */
	return 0;
}

/**
 * NOTE
 * This implementation is for experimental evaluation. We know that this
 * code is called only inside the PMI handler routine, thus only IRQs/NMIs would
 * content the lock. If we call this code from less priviledged level, the PMI
 * may preempt the current task while holding the lock... DEADLOCK.
 *
 * Moreover, inner copy is carried out on kernel memory ro avoid sleeps or
 * other interfereces.
 */

void write_clist(void *clist, void *bucket)
{
	u8 *entry;
	struct carray *carray = clist;

	spin_lock(&carray->lock);

	entry = &(carray->buf[carray->idx]);
	memcpy(entry, bucket, carray->stride);

	/* Check overwrites */
	if (carray->idx < carray->len - 1)
		carray->idx += carray->stride;
	else
		carray->idx = 0;

	// pr_info("IDX: %u\n", carray->idx);

	spin_unlock(&carray->lock);
}

// SPDX-License-Identifier: GPL-2.0
#include <linux/mm.h>

#include "clist.h"

#define TAIL_MASK ((1ULL << 32) - 1)
#define HEAD_MASK (~TAIL_MASK)
#define TAIL(t) (t & TAIL_MASK)
#define HEAD(h) ((h & HEAD_MASK) >> 32)

struct nbblist {
	size_t stride;
	size_t len;
	/* <HEAD, TAIL> */
	u64 idx;
	u8 buf[];
};

void *init_clist(size_t entry_count, size_t entry_size)
{
	size_t size = sizeof(struct nbblist) + (entry_count * entry_size);
	struct nbblist *clist = kvmalloc(size, GFP_KERNEL);

	if (!clist)
		return NULL;

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
	u64 midx;
	u8 *entry;

	struct nbblist *nbblist = clist;

	while (1) {
		do {
			midx = nbblist->idx;
		} while (HEAD(midx) + nbblist->len < TAIL(midx));

		if (HEAD(midx) == TAIL(midx))
			return 0; /* empty */

		entry = &(nbblist->buf[nbblist->stride *
				       (HEAD(midx) % nbblist->len)]);
		memcpy(&bucket, entry, nbblist->stride);

		if (cmpxchg(&(nbblist->idx), midx,
			    midx + ((nbblist->stride) << 32)) == midx)
			return nbblist->stride; /* success */
	}
}

void write_clist(void *clist, void *bucket)
{
	u64 midx;
	u8 *entry;

	struct nbblist *nbblist = clist;

	/* this is a wait-free write */
	midx = xadd(&(nbblist->idx), nbblist->stride);
	entry = &(nbblist->buf[nbblist->stride * (TAIL(midx) % nbblist->len)]);

	memcpy(entry, bucket, nbblist->stride);

	if (HEAD(midx) + nbblist->len == TAIL(midx))
		nbblist->idx += ((nbblist->stride) << 32);

	// pr_info("IDX: %llu\n", TAIL(midx));
}
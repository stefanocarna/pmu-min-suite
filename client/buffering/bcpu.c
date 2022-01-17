// SPDX-License-Identifier: GPL-2.0
#include <linux/smp.h>

#include "buffering.h"
#include "../clist/clist.h"

DEFINE_PER_CPU(void *, pcpu_clist);

int buffering_static_init(void *info, size_t entry_size)
{
	int cpu, rcpu;
	size_t buff_entries = GLOBAL_BUFFER_SIZE / entry_size;

	for_each_possible_cpu(cpu) {
		per_cpu(pcpu_clist, cpu) = init_clist(buff_entries, entry_size);
		if (!per_cpu(pcpu_clist, cpu))
			goto alloc_fail;
	}

	return 0;

alloc_fail:
	for_each_possible_cpu(rcpu) {
		if (rcpu == cpu)
			break;
		fini_clist(per_cpu(pcpu_clist, cpu));
	}

	return -ENOMEM;
}

void buffering_static_fini(void *info)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		fini_clist(per_cpu(pcpu_clist, cpu));
	}
}

int write_sample(void *info, u8 *sample)
{
	write_clist(this_cpu_read(pcpu_clist), sample);
	return 0;
}

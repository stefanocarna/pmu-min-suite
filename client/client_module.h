/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _CLIENT_MODULE_H
#define _CLIENT_MODULE_H

#include <linux/sched.h>
#include "../system_hook.h"

#ifdef MODNAME
#undef MODNAME
#endif

#if __has_include(<asm/fast_irq.h>)
#include <asm/fast_irq.h>
#define FAST_IRQ_ENABLED 1
#endif

#define MODNAME "Client-Module"

#undef pr_fmt
#define pr_fmt(fmt) MODNAME ": " fmt

struct pmcs_data {
	u64 fixed[3];
	u64 general[8];
};

struct proc_data {
	void *clist;
	struct pmcs_data pmcs;
};

extern int init_pmu(void);
extern void fini_pmu(void);

extern int pmi_init(void);
extern void pmi_fini(void);

extern int register_process(struct task_struct *task);

extern void snapshot_pmc(struct task_struct *task);

#define PERF_COND_CHGD_IGNORE_MASK (BIT_ULL(63) - 1)

#define NMI_NAME "JBASE_PMI"
#define LVT_NMI		(0x4 << 8)
#define NMI_LINE	2

#define RECODE_PMI 239
#define PMI_DELAY 0x100

enum pmi_vector {
	NMI = 0,
#ifdef FAST_IRQ_ENABLED
	IRQ = 1,
	MAX_VECTOR = 2
#else
	MAX_VECTOR = 1
#endif
};

extern enum pmi_vector pmi_vector;

extern u64 __read_mostly gbl_fixed_ctrl;
extern u64 __read_mostly reset_period;
extern int __read_mostly gbl_fixed_pmc_pmi;

extern int __read_mostly gbl_nr_pmc_fixed;
extern int __read_mostly gbl_nr_pmc_general;
extern int __read_mostly readable_pmcs;

extern bool __read_mostly thread_filter;

#endif /* _CLIENT_MODULE_H */

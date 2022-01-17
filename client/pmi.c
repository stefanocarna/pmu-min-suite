// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
#include <linux/percpu-defs.h>
#include <linux/smp.h>
#include <asm/apic.h>
#include <asm/nmi.h>

#include "pmu_low.h"
#include "client_module.h"
#include "buffering/buffering.h"

/* TODO - place inside metadata */
static DEFINE_PER_CPU(u32, pcpu_lvt_bkp);
static DEFINE_PER_CPU(int, pcpu_pmi_counter);
static struct nmiaction handler_na;

void snapshot_pmc(struct task_struct *task)
{
	int pmc;
	struct proc_data *proc_data = query_data_tracker(task);

	if (!proc_data)
		return;

	for_each_fixed_pmc(pmc)
		proc_data->pmcs.fixed[pmc] = READ_FIXED_PMC(pmc);

	for_each_pmc(pmc, readable_pmcs)
		/* TODO 0001 Restore */
		// proc_data->pmcs.general[pmc] = READ_GENERAL_PMC(pmc);
		proc_data->pmcs.general[pmc] = READ_GENERAL_PMC((pmc & 1));

	/* TODO RESTORE */
	write_sample(proc_data->clist, (u8 *)&proc_data->pmcs);

	// pr_info("[%u] Read %u general pmcs\n", smp_processor_id(),
	// 	readable_pmcs);
}

/*
 * Performance Monitor Interrupt handler
 */
static int pmi_handler(unsigned int cmd, struct pt_regs *regs)
{
	u64 global;
	uint handled = 0;
	uint cpu = smp_processor_id();

	/* Read the PMCs state */
	rdmsrl(MSR_CORE_PERF_GLOBAL_STATUS, global);

	/* Nothing to do here */
	if (!global) {
		pr_info("[%u] Got PMI on PMC %u - FIXED: %llx\n", cpu,
			gbl_fixed_pmc_pmi,
			(u64)READ_FIXED_PMC(gbl_fixed_pmc_pmi));

		goto end;
	}

	/* This IRQ is not originated from PMC overflow */
	if (!(global & (PERF_GLOBAL_CTRL_FIXED0_MASK << gbl_fixed_pmc_pmi)) &&
	    !(global && PERF_COND_CHGD_IGNORE_MASK)) {
		pr_info("Something triggered PMI - GLOBAL: %llx\n", global);
		goto no_pmi;
	}

	// if (this_cpu_read(pcpu_pmi_counter) == 128) {
	// 	pr_info("[%u] NMI - %llx\n", smp_processor_id(), reset_period);
	// 	this_cpu_write(pcpu_pmi_counter, 0);
	// } else {
	this_cpu_inc(pcpu_pmi_counter);
	// }

	snapshot_pmc(current);

	handled++;

	WRITE_FIXED_PMC(gbl_fixed_pmc_pmi, PMC_TRIM(~reset_period));

no_pmi:
	if (pmi_vector == NMI) {
		apic_write(APIC_LVTPC, LVT_NMI);
	} else {
		apic_write(APIC_LVTPC, RECODE_PMI);
		apic_eoi();
	}

	wrmsrl(MSR_CORE_PERF_GLOBAL_OVF_CTRL, global);
end:

	return handled;
}

static void pmi_lvt_setup_local(void *dummy)
{
	/* Backup old LVT entry */
	*this_cpu_ptr(&pcpu_lvt_bkp) = apic_read(APIC_LVTPC);

	if (pmi_vector == NMI)
		apic_write(APIC_LVTPC, LVT_NMI);
	else
		apic_write(APIC_LVTPC, RECODE_PMI);
}

static void pmi_lvt_cleanup_local(void *dummy)
{
	/* Restore old LVT entry */
	apic_write(APIC_LVTPC, *this_cpu_ptr(&pcpu_lvt_bkp));
}

/* Setup the PMI's NMI handler */
int pmi_nmi_setup(void)
{
	int err = 0;

	handler_na.handler = pmi_handler;
	handler_na.name = NMI_NAME;
	handler_na.flags = NMI_FLAG_FIRST;

	pr_info("Setting NMI as PMI vector\n");

	err = __register_nmi_handler(NMI_LOCAL, &handler_na);
	if (err)
		goto out;

	on_each_cpu(pmi_lvt_setup_local, NULL, 1);
out:
	return err;
}

void pmi_nmi_cleanup(void)
{
	pr_info("Free NMI vector\n");
	on_each_cpu(pmi_lvt_cleanup_local, NULL, 1);
	unregister_nmi_handler(NMI_LOCAL, NMI_NAME);
}

#ifdef FAST_IRQ_ENABLED
static int fast_irq_pmi_handler(void)
{
	return pmi_handler(0, NULL);
}
#endif

int pmi_irq_setup(void)
{
#ifdef CONFIG_RUNNING_ON_VM
	return 0;
#else
#ifndef FAST_IRQ_ENABLED
	pr_info("PMI on IRQ not available on this kernel. Proceed with NMI\n");
	return pmi_nmi_setup();
#else
	int irq = 0;

	pr_info("Mapping PMI on IRQ #%u\n", RECODE_PMI);

	/* Setup fast IRQ */
	irq = request_fast_irq(RECODE_PMI, fast_irq_pmi_handler);

	if (irq != RECODE_PMI)
		return -1;

	on_each_cpu(pmi_lvt_setup_local, NULL, 1);

	return 0;
#endif
#endif
}

void pmi_irq_cleanup(void)
{
#ifndef FAST_IRQ_ENABLED
	pmi_nmi_cleanup();
}
#else
	int unused = 0;

	pr_info("Free IRQ vector\n");
	unused = free_fast_irq(RECODE_PMI);
}
#endif

int pmi_init(void)
{
	int err;

	if (pmi_vector == NMI)
		err = pmi_nmi_setup();
	else
		err = pmi_irq_setup();

	enable_pmcs_global();

	return err;
}

void pmi_fini(void)
{
	int cpu;

	disable_pmcs_global();

	if (pmi_vector == NMI)
		pmi_nmi_cleanup();
	else
		pmi_irq_cleanup();

	for_each_online_cpu(cpu)
		pr_info("[%u] PMIs: %u\n", cpu, per_cpu(pcpu_pmi_counter, cpu));
}
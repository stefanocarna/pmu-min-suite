// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
#include <asm/apic.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <asm/perf_event.h>

#include "pmu_low.h"
#include "client_module.h"

bool machine_info_read;

u64 __read_mostly gbl_fixed_ctrl;
int __read_mostly gbl_fixed_pmc_pmi = 1;

int __read_mostly gbl_nr_pmc_fixed;
int __read_mostly gbl_nr_pmc_general;

int __read_mostly readable_pmcs;
u64 __read_mostly reset_period = (BIT_ULL(28) - 1);

enum pmi_vector pmi_vector = IRQ;

void get_machine_configuration(void)
{
	union cpuid10_edx edx;
	union cpuid10_eax eax;
	union cpuid10_ebx ebx;
	uint unused;
	uint version;

	if (machine_info_read)
		return;

	cpuid(10, &eax.full, &ebx.full, &unused, &edx.full);

	if (eax.split.mask_length < 7)
		return;

	version = eax.split.version_id;
	gbl_nr_pmc_general = eax.split.num_counters;
	gbl_nr_pmc_fixed = edx.split.num_counters_fixed;

	pr_info("PMU Version: %u\n", version);
	pr_info(" - NR Counters: %u\n", eax.split.num_counters);
	pr_info(" - Counter's Bits: %u\n", eax.split.bit_width);
	pr_info(" - Counter's Mask: %llx\n", (1ULL << eax.split.bit_width) - 1);
	pr_info(" - NR PEBS' events: %x\n",
		min_t(uint, 8, eax.split.num_counters));

	machine_info_read = true;
}

static void __enable_pmcs_local(void *dummy)
{
#ifndef CONFIG_RUNNING_ON_VM
	wrmsrl(MSR_CORE_PERF_GLOBAL_CTRL, FIXED_PMCS_TO_BITS_MASK);
#endif
}

static void __disable_pmcs_local(void *dummy)
{
#ifndef CONFIG_RUNNING_ON_VM
	wrmsrl(MSR_CORE_PERF_GLOBAL_CTRL, 0ULL);
#endif
}

void enable_pmcs_local(void)
{
	__enable_pmcs_local(NULL);
}

void disable_pmcs_local(void)
{
	__disable_pmcs_local(NULL);
}

void enable_pmcs_global(void)
{
	on_each_cpu(__enable_pmcs_local, NULL, 1);
}

void disable_pmcs_global(void)
{
	on_each_cpu(__disable_pmcs_local, NULL, 1);
}

static void __init_pmu_local(void *dummy)
{
#ifndef CONFIG_RUNNING_ON_VM
	u64 msr;
	uint pmc;

	/* Refresh APIC entry */
	if (pmi_vector == NMI)
		apic_write(APIC_LVTPC, LVT_NMI);
	else
		apic_write(APIC_LVTPC, RECODE_PMI);

	/* Clear a possible stale state */
	rdmsrl(MSR_CORE_PERF_GLOBAL_STATUS, msr);
	wrmsrl(MSR_CORE_PERF_GLOBAL_OVF_CTRL, msr);

	/* Enable FREEZE_ON_PMI */
	wrmsrl(MSR_IA32_DEBUGCTLMSR, BIT(12));

	for_each_fixed_pmc(pmc)
	{
		if (pmc == gbl_fixed_pmc_pmi) {
			/* TODO Check */
			WRITE_FIXED_PMC(pmc, PMC_TRIM(~reset_period));
		} else {
			WRITE_FIXED_PMC(pmc, 0ULL);
		}
	}

	/* Setup FIXED PMCs */
	wrmsrl(MSR_CORE_PERF_FIXED_CTR_CTRL, gbl_fixed_ctrl);

#else
	pr_warn("CONFIG_RUNNING_ON_VM is enabled. PMCs are ignored\n");
#endif
}

int pmu_global_init(void)
{
	uint pmc;

	for_each_fixed_pmc(pmc)
	{
		/**
		 * bits: 3   2   1   0
		 *      PMI, 0, USR, OS
		 */
		if (pmc == gbl_fixed_pmc_pmi) {
			/* Set PMI */
			gbl_fixed_ctrl |= (BIT(3) << (pmc * 4));
		}
		// if (params_cpl_usr)
		gbl_fixed_ctrl |= (BIT(1) << (pmc * 4));
		// if (params_cpl_os)
		gbl_fixed_ctrl |= (BIT(0) << (pmc * 4));
	}

	/* Metadata doesn't require initialization at the moment */
	on_each_cpu(__init_pmu_local, NULL, 1);

	pr_info("PMI set on fixed pmc %u\n", gbl_fixed_pmc_pmi);
	pr_warn("PMUs initialized on all cores\n");

	return 0;
}

void pmc_global_fini(void)
{
	/* TODO - To be implemented */
	on_each_cpu(__disable_pmcs_local, NULL, 1);
}

int init_pmu(void)
{
	int err;

	get_machine_configuration();

	err = pmu_global_init();
	if (err) {
		pr_err("Cannot init pmu. %u\n", err);
		goto no_pmu;
	}

	err = pmi_init();
	if (err) {
		pr_err("Cannot init pmi. %u\n", err);
		goto no_pmi;
	}

	return 0;

no_pmi:
	pmc_global_fini();
no_pmu:
	return err;
}

void fini_pmu(void)
{
	pmi_fini();
	pmc_global_fini();

	pr_info("reset_period %llx\n", reset_period);
}
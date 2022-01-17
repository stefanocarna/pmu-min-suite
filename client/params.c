// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
#include <linux/moduleparam.h>

#include "pmu_low.h"
#include "client_module.h"

static int readable_pmcs_set(const char *str, const struct kernel_param *kp)
{
	int val, ret;

	get_machine_configuration();

	ret = kstrtoint(str, 0, &val);
	if (ret < 0)
		return ret;

	/* Should check gbl_nr_pmc_general, but this is for experiment */
	/* TODO 0001 Restore */
	// if (val < 0 || val > gbl_nr_pmc_general)
	// 	return -EINVAL;

	readable_pmcs = val;
	pr_info("Set readable_pmcs %u\n", readable_pmcs);

	return ret;
}

static const struct kernel_param_ops readable_pmcs_ops = {
	.set = readable_pmcs_set,
};

module_param_cb(readable_pmcs, &readable_pmcs_ops, NULL, 0660);
MODULE_PARM_DESC(readable_pmcs, "Number of general pmcs to be read [0-8]");

static int reset_period_set(const char *str, const struct kernel_param *kp)
{
	int val, ret;

	get_machine_configuration();

	ret = kstrtoint(str, 0, &val);
	if (ret < 0)
		return ret;

	/* Should check gbl_nr_pmc_general, but this is for experiment */
	if (val < 12 || val > 48)
		return -EINVAL;

	reset_period = (BIT_ULL(val) - 1);
	pr_info("Set reset_period %llx\n", reset_period);

	return ret;
}

static const struct kernel_param_ops reset_period_ops = {
	.set = reset_period_set,
};

module_param_cb(reset_period, &reset_period_ops, NULL, 0660);
MODULE_PARM_DESC(reset_period, "PMI reset period - (2^x) - 1 [12-48]");

static int thread_filter_set(const char *str, const struct kernel_param *kp)
{
	int ret;
	bool val;

	ret = kstrtobool(str, &val);
	if (ret < 0)
		return ret;

	thread_filter = val;
	pr_info("Set thread_filter %s\n", thread_filter ? "true" : "false");

	return ret;
}

static const struct kernel_param_ops thread_filter_ops = {
	.set = thread_filter_set,
};

module_param_cb(thread_filter, &thread_filter_ops, NULL, 0660);
MODULE_PARM_DESC(thread_filter, "PMI reset period - (2^x) - 1 [12-48]");

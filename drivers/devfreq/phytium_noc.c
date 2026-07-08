// SPDX-License-Identifier: GPL-1.0
/*
 * Phytium Processor NOC Frequency Driver (v1/v2 unified)
 *
 * Copyright (C) 2024, Phytium Technology Co.,Ltd.
 */
#include <linux/module.h>
#include <linux/device.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/devfreq.h>
#include <linux/pm_opp.h>
#include <linux/io.h>
#include <linux/acpi.h>
#include <linux/suspend.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#define NOCFREQ_DRIVER_VERSION "1.1.0"

enum phytium_noc_type {
	PHYTIUM_NOC_V1,
	PHYTIUM_NOC_V2,
};

struct phytium_nocfreq_info {
	enum phytium_noc_type type;
	const char *name;
};

#define DEVICE_TYPE_V1 7
#define DEVICE_TYPE_V2 7

/* v1 Register definition */
#define V1_MINI_SIZE 0x400
#define V1_CNT_ENABLE 0x000
#define V1_WORK_STATE 0X004
#define V1_CLR_EN 0X010
#define V1_SNAPSHOT_EN 0X014
#define V1_INT_CTRL_CLR 0x024
#define V1_WR_NOLAST_HANDSHARK_NUM 0x44

/* v2 Register definition */
#define V2_REG_NOC_STATUS   0x0
#define V2_BUSY_CODE_MASK   0x1f
#define V2_BUSY_CODE_MAX    16

/* Fast ramp-up thresholds on each level (busy code out of 16) */
#define V2_UP_225_TO_450_BUSY    4
#define V2_UP_450_TO_900_BUSY    8
#define V2_UP_900_TO_1800_BUSY   12

/* Hysteresis hold/down thresholds */
#define V2_DOWN_450_TO_225_BUSY  2
#define V2_DOWN_900_TO_450_BUSY  6
#define V2_DOWN_1800_TO_900_BUSY 10

struct phytium_nocfreq {
	struct device *dev;
	const struct phytium_nocfreq_info *info;

	struct devfreq *devfreq;
	struct devfreq_dev_profile profile;
	struct devfreq_simple_ondemand_data ondemand_data;

	/* v1 only */
	void __iomem *reg_noc_v1;

	/* v2 only */
	void __iomem *reg_noc_v2;
	unsigned int uid;
	unsigned int v2_busy_code;

	struct mutex lock;

	unsigned long rate, target_rate, suspend_freq;
	unsigned int freq_count;
	unsigned long freq_table[];
};

/* v1 Bandwidth acquisition */
static u32 phytium_nocfreq_get_peak_bw_v1(struct phytium_nocfreq *priv)
{
	unsigned long peak_bw, bw_0, bw_1, bw_2, bw_3;

	bw_0 = readl_relaxed(priv->reg_noc_v1 + V1_WR_NOLAST_HANDSHARK_NUM);
	bw_1 = readl_relaxed(priv->reg_noc_v1 + V1_MINI_SIZE*1 +
			V1_WR_NOLAST_HANDSHARK_NUM);
	bw_2 = readl_relaxed(priv->reg_noc_v1 + V1_MINI_SIZE*2 +
			V1_WR_NOLAST_HANDSHARK_NUM);
	bw_3 = readl_relaxed(priv->reg_noc_v1 + V1_MINI_SIZE*3 +
			V1_WR_NOLAST_HANDSHARK_NUM);

	peak_bw = bw_0;
	if (bw_1 > peak_bw)
		peak_bw = bw_1;
	if (bw_2 > peak_bw)
		peak_bw = bw_2;
	if (bw_3 > peak_bw)
		peak_bw = bw_3;
	return peak_bw;
}

static void phytium_nocfreq_restart_handshark_counters_v1(struct phytium_nocfreq *priv)
{
	writel_relaxed(0x80000000, priv->reg_noc_v1 + V1_INT_CTRL_CLR);
	writel_relaxed(0x80000000, priv->reg_noc_v1 + V1_MINI_SIZE*1 + V1_INT_CTRL_CLR);
	writel_relaxed(0x80000000, priv->reg_noc_v1 + V1_MINI_SIZE*2 + V1_INT_CTRL_CLR);
	writel_relaxed(0x80000000, priv->reg_noc_v1 + V1_MINI_SIZE*3 + V1_INT_CTRL_CLR);

	writel_relaxed(0x1, priv->reg_noc_v1 + V1_CLR_EN);
	writel_relaxed(0x1, priv->reg_noc_v1 + V1_MINI_SIZE*1 + V1_CLR_EN);
	writel_relaxed(0x1, priv->reg_noc_v1 + V1_MINI_SIZE*2 + V1_CLR_EN);
	writel_relaxed(0x1, priv->reg_noc_v1 + V1_MINI_SIZE*3 + V1_CLR_EN);
}

/* v2 Bandwidth acquisition */
static u32 phytium_nocfreq_get_peak_bw_v2(struct phytium_nocfreq *priv)
{
	return readl_relaxed(priv->reg_noc_v2 + V2_REG_NOC_STATUS);
}

static unsigned long phytium_nocfreq_min_rate(struct phytium_nocfreq *priv)
{
	unsigned long min_rate = priv->freq_table[0];
	int i;

	for (i = 1; i < priv->freq_count; i++) {
		if (priv->freq_table[i] < min_rate)
			min_rate = priv->freq_table[i];
	}

	return min_rate;
}

static unsigned long phytium_nocfreq_max_rate(struct phytium_nocfreq *priv)
{
	unsigned long max_rate = priv->freq_table[0];
	int i;

	for (i = 1; i < priv->freq_count; i++) {
		if (priv->freq_table[i] > max_rate)
			max_rate = priv->freq_table[i];
	}

	return max_rate;
}

static unsigned long phytium_nocfreq_next_higher_rate(struct phytium_nocfreq *priv,
						       unsigned long rate)
{
	unsigned long next_rate = ~0UL;
	int i;

	for (i = 0; i < priv->freq_count; i++) {
		if (priv->freq_table[i] > rate && priv->freq_table[i] < next_rate)
			next_rate = priv->freq_table[i];
	}

	if (next_rate == ~0UL)
		return rate;

	return next_rate;
}

/* v1/v2 General frequency setting */
static int phytium_noc_set_freq(struct device *dev, unsigned long freq)
{
	struct phytium_nocfreq *priv = dev_get_drvdata(dev);
	acpi_handle handle = ACPI_HANDLE(dev);
	union acpi_object args[4];
	struct acpi_object_list arg_list = {
		.pointer = args,
		.count = ARRAY_SIZE(args),
	};
	acpi_status status;
	unsigned long long ret;

	args[0].type = ACPI_TYPE_INTEGER;
	args[0].integer.value = DEVICE_TYPE_V1;
	args[1].type = ACPI_TYPE_INTEGER;
	args[1].integer.value = freq;

	if (priv->info->type == PHYTIUM_NOC_V2) {
		args[2].type = ACPI_TYPE_INTEGER;
		args[2].integer.value = priv->uid;
	} else {
		args[2].type = ACPI_TYPE_INTEGER;
		args[2].integer.value = 0;
	}
	args[3].type = ACPI_TYPE_INTEGER;
	args[3].integer.value = 0;

	mutex_lock(&priv->lock);
	status = acpi_evaluate_integer(handle, "PSCF", &arg_list, &ret);
	mutex_unlock(&priv->lock);

	if (ACPI_FAILURE(status)) {
		dev_err(dev, "No PSCF method\n");
		return -EIO;
	}
	if (ret) {
		dev_err(dev, "Failed to set the freq to %lu\n", freq);
		return -EIO;
	}
	dev_dbg(dev, "set target_freq = %lu khz\n", freq);
	return 0;
}

static int phytium_noc_target(struct device *dev, unsigned long *freq, u32 flags)
{
	struct phytium_nocfreq *priv = dev_get_drvdata(dev);
	unsigned long old_freq = priv->rate;
	unsigned long target_rate;
	struct dev_pm_opp *opp;
	int ret;

	opp = devfreq_recommended_opp(dev, freq, flags);
	if (IS_ERR(opp))
		return PTR_ERR(opp);

	target_rate = dev_pm_opp_get_freq(opp);
	dev_pm_opp_put(opp);

	if (priv->info->type == PHYTIUM_NOC_V2) {
		unsigned long low = phytium_nocfreq_min_rate(priv);
		unsigned long mid1 = phytium_nocfreq_next_higher_rate(priv, low);
		unsigned long mid2 = phytium_nocfreq_next_higher_rate(priv, mid1);
		unsigned long high = phytium_nocfreq_max_rate(priv);

		if (old_freq == low) {
			if (priv->v2_busy_code >= V2_UP_225_TO_450_BUSY && mid1 > low &&
			    target_rate < mid1)
				target_rate = mid1;
		} else if (old_freq == mid1) {
			if (priv->v2_busy_code >= V2_UP_450_TO_900_BUSY && mid2 > mid1 &&
			    target_rate < mid2)
				target_rate = mid2;
			else if (priv->v2_busy_code > V2_DOWN_450_TO_225_BUSY &&
				 priv->v2_busy_code < V2_UP_450_TO_900_BUSY)
				target_rate = mid1;
		} else if (old_freq == mid2) {
			if (priv->v2_busy_code >= V2_UP_900_TO_1800_BUSY && high > mid2 &&
			    target_rate < high)
				target_rate = high;
			else if (priv->v2_busy_code > V2_DOWN_900_TO_450_BUSY &&
				 priv->v2_busy_code < V2_UP_900_TO_1800_BUSY)
				target_rate = mid2;
		} else if (old_freq == high) {
			if (priv->v2_busy_code > V2_DOWN_1800_TO_900_BUSY)
				target_rate = high;
		}
	}

	if (target_rate == old_freq)
		return 0;

	ret = phytium_noc_set_freq(dev, target_rate);
	if (ret) {
		dev_warn(dev, "failed to set noc frequency: %d\n", ret);
		*freq = old_freq;
	}
	priv->rate = target_rate;
	return ret;
}

static int phytium_noc_get_cur_freq(struct device *dev, unsigned long *freq)
{
	struct phytium_nocfreq *priv = dev_get_drvdata(dev);
	acpi_handle handle = ACPI_HANDLE(dev);
	union acpi_object args[3];
	struct acpi_object_list arg_list = {
		.pointer = args,
		.count = ARRAY_SIZE(args),
	};
	acpi_status status;
	unsigned long long ret;

	args[0].type = ACPI_TYPE_INTEGER;
	args[0].integer.value = DEVICE_TYPE_V1;
	args[1].type = ACPI_TYPE_INTEGER;
	args[1].integer.value = 0;
	args[2].type = ACPI_TYPE_INTEGER;
	args[2].integer.value = 0;

	mutex_lock(&priv->lock);
	status = acpi_evaluate_integer(handle, "PGCF", &arg_list, &ret);
	mutex_unlock(&priv->lock);
	if (ACPI_FAILURE(status)) {
		dev_err(dev, "No PGCF method\n");
		return -EIO;
	}
	if (ret < 0) {
		dev_err(dev, "Failed to get the freq\n");
		return -EIO;
	}
	*freq = ret;
	return 0;
}

static int phytium_noc_get_freq_info(struct device *dev, u32 flags)
{
	struct phytium_nocfreq *priv = dev_get_drvdata(dev);
	struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };
	union acpi_object args[3], *package, *element;
	struct acpi_object_list arg_list = {
		.pointer = args,
		.count = ARRAY_SIZE(args),
	};
	acpi_handle handle = ACPI_HANDLE(dev);
	acpi_status status;
	int i;

	args[0].type = ACPI_TYPE_INTEGER;
	args[0].integer.value = flags;
	args[1].type = ACPI_TYPE_INTEGER;
	args[1].integer.value = 0;
	args[2].type = ACPI_TYPE_INTEGER;
	args[2].integer.value = 0;

	status = acpi_evaluate_object(handle, "PGCL", &arg_list, &buffer);
	if (ACPI_FAILURE(status)) {
		dev_err(dev, "No PGCL method\n");
		return -EIO;
	}
	if (!buffer.length) {
		dev_err(dev, "buffer is NULL\n");
		return -EINVAL;
	}

	package = buffer.pointer;
	element = &package->package.elements[1];
	priv->freq_count = element->integer.value;

	for (i = 0; i < priv->freq_count; i++) {
		element = &package->package.elements[i+2];
		priv->freq_table[i] = element->integer.value;
		dev_dbg(dev, "freq_table[%d] = %llu\n", i, element->integer.value);
	}
	kfree(buffer.pointer);
	return 0;
}

static int get_freq_count(struct device *dev)
{
	int freq_count = -1;
	struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };
	union acpi_object args[3], *package, *element;
	struct acpi_object_list arg_list = {
		.pointer = args,
		.count = ARRAY_SIZE(args),
	};
	acpi_handle handle = ACPI_HANDLE(dev);
	acpi_status status;

	args[0].type = ACPI_TYPE_INTEGER;
	args[0].integer.value = DEVICE_TYPE_V1;
	args[1].type = ACPI_TYPE_INTEGER;
	args[1].integer.value = 0;
	args[2].type = ACPI_TYPE_INTEGER;
	args[2].integer.value = 0;

	status = acpi_evaluate_object(handle, "PGCL", &arg_list, &buffer);
	if (ACPI_FAILURE(status)) {
		dev_err(dev, "No PGCL method\n");
		return -EIO;
	}
	if (!buffer.length) {
		dev_err(dev, "buffer is NULL\n");
		return -EINVAL;
	}

	package = buffer.pointer;
	element = &package->package.elements[1];
	freq_count = element->integer.value;
	dev_dbg(dev, "freq_count = %d\n", freq_count);
	kfree(buffer.pointer);
	return freq_count;
}

static int phytium_noc_get_dev_status(struct device *dev, struct devfreq_dev_status *stat)
{
	struct phytium_nocfreq *priv = dev_get_drvdata(dev);

	if (priv->info->type == PHYTIUM_NOC_V1) {
		writel_relaxed(0x1, priv->reg_noc_v1 + V1_SNAPSHOT_EN);
		writel_relaxed(0x1, priv->reg_noc_v1 + V1_MINI_SIZE*1 + V1_SNAPSHOT_EN);
		writel_relaxed(0x1, priv->reg_noc_v1 + V1_MINI_SIZE*2 + V1_SNAPSHOT_EN);
		writel_relaxed(0x1, priv->reg_noc_v1 + V1_MINI_SIZE*3 + V1_SNAPSHOT_EN);

		stat->busy_time = phytium_nocfreq_get_peak_bw_v1(priv);
		stat->total_time = 320000 * DIV_ROUND_CLOSEST(priv->rate * 100,
				priv->profile.initial_freq);
		stat->current_frequency = priv->rate;

		phytium_nocfreq_restart_handshark_counters_v1(priv);
	} else {
		u32 raw_busy = phytium_nocfreq_get_peak_bw_v2(priv);
		u32 busy_code = raw_busy & V2_BUSY_CODE_MASK;

		if (busy_code > V2_BUSY_CODE_MAX)
			busy_code = V2_BUSY_CODE_MAX;

		priv->v2_busy_code = busy_code;
		stat->busy_time = busy_code;
		stat->total_time = V2_BUSY_CODE_MAX;
		stat->current_frequency = priv->rate;
	}
	return 0;
}

static __maybe_unused int phytium_nocfreq_suspend(struct device *dev)
{
	struct phytium_nocfreq *priv = dev_get_drvdata(dev);
	int ret = 0;

	ret = phytium_noc_get_cur_freq(dev, &priv->suspend_freq);
	if (ret)
		dev_warn(dev, "failed to get suspend freq\n");

	ret = devfreq_suspend_device(priv->devfreq);
	if (ret < 0)
		dev_err(dev, "failed to suspend the devfreq devices\n");
	priv->devfreq->stop_polling = true;

	if (priv->info->type == PHYTIUM_NOC_V1) {
		writel_relaxed(0x0, priv->reg_noc_v1 + V1_CNT_ENABLE);
		writel_relaxed(0x0, priv->reg_noc_v1 + V1_MINI_SIZE*1+V1_CNT_ENABLE);
		writel_relaxed(0x0, priv->reg_noc_v1 + V1_MINI_SIZE*2+V1_CNT_ENABLE);
		writel_relaxed(0x0, priv->reg_noc_v1 + V1_MINI_SIZE*3+V1_CNT_ENABLE);
	}
	return ret;
}

static __maybe_unused int phytium_nocfreq_resume(struct device *dev)
{
	struct phytium_nocfreq *priv = dev_get_drvdata(dev);
	int ret = 0;

	ret = devfreq_resume_device(priv->devfreq);
	if (ret < 0)
		dev_err(dev, "failed to resume the devfreq devices\n");
	if (!delayed_work_pending(&priv->devfreq->work) && priv->devfreq->profile->polling_ms)
		priv->devfreq->stop_polling = true;

	if (priv->info->type == PHYTIUM_NOC_V1) {
		writel_relaxed(0x02, priv->reg_noc_v1 + V1_WORK_STATE);
		writel_relaxed(0x02, priv->reg_noc_v1 + V1_MINI_SIZE*1 + V1_WORK_STATE);
		writel_relaxed(0x02, priv->reg_noc_v1 + V1_MINI_SIZE*2 + V1_WORK_STATE);
		writel_relaxed(0x02, priv->reg_noc_v1 + V1_MINI_SIZE*3 + V1_WORK_STATE);

		writel_relaxed(0x3f, priv->reg_noc_v1 + V1_CNT_ENABLE);
		writel_relaxed(0x3f, priv->reg_noc_v1 + V1_MINI_SIZE*1+V1_CNT_ENABLE);
		writel_relaxed(0x3f, priv->reg_noc_v1 + V1_MINI_SIZE*2+V1_CNT_ENABLE);
		writel_relaxed(0x3f, priv->reg_noc_v1 + V1_MINI_SIZE*3+V1_CNT_ENABLE);
	}
	if (priv->suspend_freq) {
		ret = phytium_noc_set_freq(dev, priv->suspend_freq);
		if (ret < 0)
			dev_warn(dev, "failed to restore suspend freq %lu\n", priv->suspend_freq);
		else
			priv->rate = priv->suspend_freq;
	}
	return ret;
}

static SIMPLE_DEV_PM_OPS(phytium_nocfreq_pm, phytium_nocfreq_suspend, phytium_nocfreq_resume);

static int phytium_nocfreq_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct phytium_nocfreq *priv;
	const struct phytium_nocfreq_info *info;
	const char *gov = DEVFREQ_GOV_SIMPLE_ONDEMAND;
	int i, ret;
	unsigned int max_state;
	struct resource *mem;

	info = device_get_match_data(dev);
	if (!info) {
		dev_err(dev, "No match data for this device\n");
		return -ENODEV;
	}

	max_state = get_freq_count(dev);
	if (max_state <= 0)
		return -EINVAL;
	priv = devm_kzalloc(dev, struct_size(priv, freq_table, max_state), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	priv->info = info;
	mutex_init(&priv->lock);

	platform_set_drvdata(pdev, priv);

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(dev, "no mem resource");
		return -EINVAL;
	}

	if (info->type == PHYTIUM_NOC_V1) {
		priv->reg_noc_v1 = devm_ioremap_resource(dev, mem);
		if (!priv->reg_noc_v1)
			return PTR_ERR(priv->reg_noc_v1);
		dev->init_name = "nocfreq";
	} else {
		acpi_handle handle = ACPI_HANDLE(dev);
		unsigned long long uid;

		ret = acpi_evaluate_integer(handle, "_UID", NULL, &uid);
		priv->uid = uid;
		priv->reg_noc_v2 = devm_ioremap_resource(dev, mem);
		if (IS_ERR(priv->reg_noc_v2))
			return PTR_ERR(priv->reg_noc_v2);
		dev_set_name(dev, "noc-%u", priv->uid);
	}

	ret = phytium_noc_get_freq_info(dev, DEVICE_TYPE_V1);
	if (ret)
		return ret;
	priv->profile.initial_freq		= priv->freq_table[0];
	priv->profile.polling_ms		= 100;
	priv->profile.target			= phytium_noc_target;
	priv->profile.get_cur_freq		= phytium_noc_get_cur_freq;
	priv->profile.get_dev_status		= phytium_noc_get_dev_status;
	priv->profile.freq_table		= priv->freq_table;
	priv->profile.max_state			= priv->freq_count;
	priv->rate				= priv->freq_table[0];

	if (info->type == PHYTIUM_NOC_V1) {
		priv->ondemand_data.upthreshold		= 80;
		priv->ondemand_data.downdifferential	= 10;
	} else {
		priv->ondemand_data.upthreshold		= 80;
		priv->ondemand_data.downdifferential	= 10;
	}

	for (i = 0; i < max_state; ++i) {
		ret = dev_pm_opp_add(dev, priv->freq_table[i], 0);
		if (ret < 0)
			goto err;
	}
	priv->devfreq = devm_devfreq_add_device(dev, &priv->profile, gov, &priv->ondemand_data);
	if (IS_ERR(priv->devfreq)) {
		ret = PTR_ERR(priv->devfreq);
		goto err;
	}
	ret = phytium_noc_set_freq(dev, priv->profile.initial_freq);
	if (ret)
		dev_warn(dev, "failed to init noc frequency: %d\n", ret);

	if (info->type == PHYTIUM_NOC_V1) {
		writel_relaxed(0x02, priv->reg_noc_v1 + V1_WORK_STATE);
		writel_relaxed(0x02, priv->reg_noc_v1 + V1_MINI_SIZE*1 + V1_WORK_STATE);
		writel_relaxed(0x02, priv->reg_noc_v1 + V1_MINI_SIZE*2 + V1_WORK_STATE);
		writel_relaxed(0x02, priv->reg_noc_v1 + V1_MINI_SIZE*3 + V1_WORK_STATE);

		writel_relaxed(0x3f, priv->reg_noc_v1 + V1_CNT_ENABLE);
		writel_relaxed(0x3f, priv->reg_noc_v1 + V1_MINI_SIZE*1 + V1_CNT_ENABLE);
		writel_relaxed(0x3f, priv->reg_noc_v1 + V1_MINI_SIZE*2 + V1_CNT_ENABLE);
		writel_relaxed(0x3f, priv->reg_noc_v1 + V1_MINI_SIZE*3 + V1_CNT_ENABLE);
	}
	return 0;
err:
	dev_pm_opp_of_remove_table(dev);
	return ret;
}

static int phytium_nocfreq_remove(struct platform_device *pdev)
{
	struct phytium_nocfreq *priv = platform_get_drvdata(pdev);
	unsigned long initial_freq = priv->profile.initial_freq;
	struct device *dev = &pdev->dev;
	int ret;

	if (priv->info->type == PHYTIUM_NOC_V1) {
		writel_relaxed(0x0, priv->reg_noc_v1 + V1_CNT_ENABLE);
		writel_relaxed(0x0, priv->reg_noc_v1 + V1_MINI_SIZE*1 + V1_CNT_ENABLE);
		writel_relaxed(0x0, priv->reg_noc_v1 + V1_MINI_SIZE*2 + V1_CNT_ENABLE);
		writel_relaxed(0x0, priv->reg_noc_v1 + V1_MINI_SIZE*3 + V1_CNT_ENABLE);

		writel_relaxed(0x1, priv->reg_noc_v1 + V1_CLR_EN);
		writel_relaxed(0x1, priv->reg_noc_v1 + V1_MINI_SIZE*1 + V1_CLR_EN);
		writel_relaxed(0x1, priv->reg_noc_v1 + V1_MINI_SIZE*2 + V1_CLR_EN);
		writel_relaxed(0x1, priv->reg_noc_v1 + V1_MINI_SIZE*3 + V1_CLR_EN);
	}

	ret = phytium_noc_set_freq(dev, initial_freq);
	if (ret)
		dev_warn(dev, "failed to restore NOC frequency: %d\n", ret);

	if (!priv->devfreq)
		return 0;
	dev_pm_opp_remove_all_dynamic(dev);
	return 0;
}

/* Matching Type Table */
static const struct phytium_nocfreq_info phytium_noc_v1_info = {
	.type = PHYTIUM_NOC_V1,
	.name = "phytium_noc_v1",
};
static const struct phytium_nocfreq_info phytium_noc_v2_info = {
	.type = PHYTIUM_NOC_V2,
	.name = "phytium_noc_v2",
};

#ifdef CONFIG_ACPI
static const struct acpi_device_id phytium_noc_acpi_ids[] = {
	{ "PHYT0047", (kernel_ulong_t)&phytium_noc_v1_info },
	{ "PHYT3010", (kernel_ulong_t)&phytium_noc_v2_info },
	{},
};
MODULE_DEVICE_TABLE(acpi, phytium_noc_acpi_ids);
#endif

static struct platform_driver phytium_nocfreq_driver = {
	.probe		= phytium_nocfreq_probe,
	.remove		= phytium_nocfreq_remove,
	.driver = {
		.name			= "phytium_nocfreq",
		.pm			= &phytium_nocfreq_pm,
#ifdef CONFIG_ACPI
		.acpi_match_table	= phytium_noc_acpi_ids,
#endif
		.suppress_bind_attrs	= true,
	},
};
module_platform_driver(phytium_nocfreq_driver);

MODULE_DESCRIPTION("Phytium NOC Controller frequency driver (v1/v2 unified)");
MODULE_AUTHOR("Li Mingzhe <limingzhe@phytium.com.cn>");
MODULE_LICENSE("GPL");
MODULE_VERSION(NOCFREQ_DRIVER_VERSION);

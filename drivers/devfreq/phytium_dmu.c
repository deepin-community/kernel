// SPDX-License-Identifier: GPL-1.0
/*
 * Phytium Processor DMU Frequency Driver (v1/v2 unified)
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
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/slab.h>
#include <linux/suspend.h>

#define DEVICE_TYPE 9

#define DMUFREQ_DRIVER_VERSION "1.1.0"

/* v1 Register definition */
#define DMU_V1_PMU_STRIDE      0x80000
#define DMU_V1_AXI_MONITOR2_L  0x084
#define DMU_V1_AXI_MONITOR3_L  0x08c
#define DMU_V1_AXI_MONITOR_EN  0x01c
#define DMU_V1_TIMER_START     0x000
#define DMU_V1_TIMER_STOP      0x004
#define DMU_V1_CLEAR_EVENT     0x008

#define DDR_PMU_NOTICE_START    0x0
#define DDR_PMU_NOTICE_STOP     0x1

/* v2 Register definition */
#define DMU_V2_PDM_START       0xC0000
#define DMU_V2_PDM_STRIDE      0x1000
#define DMU_V2_MONITOR_START   0x304
#define DMU_V2_MONITOR_SNAPSHOT 0x30c
#define DMU_V2_EVENT_CLEAR     0x308
#define DMU_V2_EVENT_L_CNT     0x310
#define DMU_V2_EVENT_H_CNT     0x314

enum phytium_dmu_type {
	PHYTIUM_DMU_V1,
	PHYTIUM_DMU_V2,
};

struct phytium_dmufreq_info {
	enum phytium_dmu_type type;
	const char *name;
};

struct phytium_dmufreq {
	struct device *dev;
	const struct phytium_dmufreq_info *info;

	struct devfreq *devfreq;
	struct devfreq_dev_profile profile;
	struct devfreq_simple_ondemand_data ondemand_data;

	unsigned long	rate;
	unsigned long	bandwidth;
	unsigned long	single_threshold_value;
	int max_count;
	int cnt;

	/* v1: multichannel，v2: Single-channel multiple instances */
	union {
		void __iomem **basev1;
		void __iomem *basev2;
	};

	/* v1 only */
	unsigned long *read_bw;
	unsigned long *write_bw;
	struct notifier_block nb;
	bool pmu_active;
	unsigned long last_bust_time;

	/* v2 only */
	unsigned int uid;
	struct mutex lock;

	unsigned int freq_count;
	unsigned long freq_table[];
};

struct acpi_result {
	int status;
	unsigned long long value;
};

/* v1/v2 Distinguishing-type register access */
static inline void dmu_v1_write32(struct phytium_dmufreq *priv,
		int dmu, unsigned long offset, unsigned long value)
{
	writel_relaxed(value, priv->basev1[dmu] + offset);
}

static inline unsigned long dmu_v1_read32(struct phytium_dmufreq *priv,
		int dmu, unsigned long offset)
{
	return readl_relaxed(priv->basev1[dmu] + offset);
}

static inline void dmu_v2_write32(struct phytium_dmufreq *priv,
		unsigned long offset, unsigned long value)
{
	writel_relaxed(value, priv->basev2 + offset);
}

static inline unsigned long dmu_v2_read32(struct phytium_dmufreq *priv,
		unsigned long offset)
{
	return readl_relaxed(priv->basev2 + offset);
}

/* ACPI/BIOS Related general methods */
static struct acpi_result phytium_acpi_eval_integer(struct device *dev, const char *method)
{
	acpi_handle handle = ACPI_HANDLE(dev);
	acpi_status status;
	unsigned long long val;
	struct acpi_result result;

	status = acpi_evaluate_integer(handle, (char *)method, NULL, &val);
	if (ACPI_FAILURE(status)) {
		dev_err(dev, "Failed to evaluate %s: ACPI status 0x%x\n", method, status);
		result.status = -EIO;
		result.value = 0;
		return result;
	}
	result.status = 0;
	result.value = val;
	return result;
}

/* V1 PMU notification chain, only V1 is registered */
BLOCKING_NOTIFIER_HEAD(dmu_pmu_notifier_chain);
EXPORT_SYMBOL(dmu_pmu_notifier_chain);

static int dmu_pmu_notifier_call(struct notifier_block *nb, unsigned long event, void *data)
{
	struct phytium_dmufreq *priv = container_of(nb, struct phytium_dmufreq, nb);
	struct device *dev = priv->dev;

	switch (event) {
	case DDR_PMU_NOTICE_START:
		priv->pmu_active = false;
		dev_dbg(dev, "DDR PMU START: Stopping monitoring\n");
		break;
	case DDR_PMU_NOTICE_STOP:
		priv->cnt = 0;
		priv->pmu_active = true;
		dev_dbg(dev, "DDR PMU STOP: Resuming monitoring\n");
		break;
	default:
		break;
	}
	return NOTIFY_OK;
}

/* v1/v2 General ACPI Frequency Settings */
static int phytium_dmu_set_freq(struct device *dev, unsigned long freq)
{
	acpi_handle handle = ACPI_HANDLE(dev);
	struct phytium_dmufreq *priv = dev_get_drvdata(dev);
	union acpi_object args[4];
	struct acpi_object_list arg_list = {
		.pointer = args,
		.count = ARRAY_SIZE(args),
	};
	acpi_status status;
	unsigned long long ret;

	args[0].type = ACPI_TYPE_INTEGER;
	args[0].integer.value = DEVICE_TYPE;
	args[1].type = ACPI_TYPE_INTEGER;
	args[1].integer.value = freq;
	if (priv->info->type == PHYTIUM_DMU_V2)
		args[2].integer.value = priv->uid;
	else
		args[2].integer.value = 0;
	args[2].type = ACPI_TYPE_INTEGER;
	args[3].type = ACPI_TYPE_INTEGER;
	args[3].integer.value = 0;

	if (priv->info->type == PHYTIUM_DMU_V2)
		mutex_lock(&priv->lock);
	status = acpi_evaluate_integer(handle, "PSCF", &arg_list, &ret);
	if (priv->info->type == PHYTIUM_DMU_V2)
		mutex_unlock(&priv->lock);

	if (ACPI_FAILURE(status)) {
		dev_err(dev, "No PSCF method\n");
		return -EIO;
	}
	return 0;
}

static int phytium_dmu_target(struct device *dev, unsigned long *freq, u32 flags)
{
	struct phytium_dmufreq *priv = dev_get_drvdata(dev);
	unsigned long old_freq = priv->rate;
	unsigned long target_rate;
	struct dev_pm_opp *opp;
	int ret;

	opp = devfreq_recommended_opp(dev, freq, flags);
	if (IS_ERR(opp))
		return PTR_ERR(opp);

	target_rate = dev_pm_opp_get_freq(opp);
	dev_pm_opp_put(opp);

	if (target_rate == old_freq)
		return 0;

	dev_dbg(dev, "target_rate = %lu\n", target_rate);
	ret = phytium_dmu_set_freq(dev, target_rate);

	if (ret) {
		dev_warn(dev, "failed to set DRAM frequency: %lu\n", target_rate);
		return ret;
	}
	priv->rate = target_rate;
	return ret;
}

static int phytium_dmu_get_cur_freq(struct device *dev, unsigned long *freq)
{
	struct phytium_dmufreq *priv = dev_get_drvdata(dev);

	*freq = priv->rate;
	return 0;
}

/* v1/v2 General ACPI Method */
static int get_freq_count(struct device *dev)
{
	struct acpi_result result;
	struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };
	union acpi_object args[3], *package, *element;
	struct acpi_object_list arg_list = {
		.pointer = args,
		.count = ARRAY_SIZE(args),
	};
	acpi_handle handle = ACPI_HANDLE(dev);
	acpi_status status;

	args[0].type = ACPI_TYPE_INTEGER;
	args[0].integer.value = DEVICE_TYPE;
	args[1].type = ACPI_TYPE_INTEGER;
	args[1].integer.value = 0;
	args[2].type = ACPI_TYPE_INTEGER;
	args[2].integer.value = 0;

	status = acpi_evaluate_object(handle, "PGCL", &arg_list, &buffer);
	if (ACPI_FAILURE(status)) {
		dev_err(dev, "No PGCL method, status = %d\n", status);
		return -EIO;
	}
	package = buffer.pointer;
	element = &package->package.elements[1];
	result.value = element->integer.value;

	kfree(buffer.pointer);
	return result.value;
}

static int phytium_dmu_get_freq_info(struct device *dev)
{
	struct phytium_dmufreq *priv = dev_get_drvdata(dev);
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
	args[0].integer.value = DEVICE_TYPE;
	args[1].type = ACPI_TYPE_INTEGER;
	args[1].integer.value = 0;
	args[2].type = ACPI_TYPE_INTEGER;
	args[2].integer.value = 0;

	status = acpi_evaluate_object(handle, "PGCL", &arg_list, &buffer);
	if (ACPI_FAILURE(status)) {
		dev_err(dev, "No PGCL method\n");
		return -EIO;
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

/* v1/v2 General ACPI State */
static struct acpi_result phytium_dmufreq_state(struct device *dev)
{
	return phytium_acpi_eval_integer(dev, "STAT");
}
static struct acpi_result phytium_current_enabled_channels(struct device *dev)
{
	return phytium_acpi_eval_integer(dev, "CHAN");
}
static struct acpi_result phytium_read_threshold_value(struct device *dev)
{
	return phytium_acpi_eval_integer(dev, "BAND");
}

/* v1 Bandwidth acquisition with PMU notification chain */
static u64 phytium_dmufreq_get_real_bw_v1(struct phytium_dmufreq *priv)
{
	unsigned long peak_bw = 0;
	unsigned long sum_peak_bw = 0;
	int i;

	for (i = 0; i < priv->max_count; i++) {
		priv->read_bw[i] = dmu_v1_read32(priv, i, DMU_V1_AXI_MONITOR2_L);
		priv->write_bw[i] = dmu_v1_read32(priv, i, DMU_V1_AXI_MONITOR3_L);

		dmu_v1_write32(priv, i, DMU_V1_CLEAR_EVENT, 0x1);
		dmu_v1_write32(priv, i, DMU_V1_TIMER_START, 0x1);
		sum_peak_bw = priv->read_bw[i] + priv->write_bw[i];
		if (sum_peak_bw > peak_bw)
			peak_bw = sum_peak_bw;
	}
	dev_dbg(priv->dev, "peak_bw = %lu\n", peak_bw);
	return peak_bw;
}

static void polling_handle_v1(struct phytium_dmufreq *priv)
{
	int i;
	if (!priv->pmu_active) {
		priv->bandwidth = priv->last_bust_time;
		return;
	}
	if (priv->cnt > 0) {
		for (i = 0; i < priv->max_count ; i++) {
			dmu_v1_write32(priv, i, DMU_V1_AXI_MONITOR_EN, 0x101);
			dmu_v1_write32(priv, i, DMU_V1_TIMER_STOP, 0x1);
		}
		priv->bandwidth = phytium_dmufreq_get_real_bw_v1(priv);
	}
	priv->cnt = 1;
}

/* v2 Multi-instance bandwidth acquisition */
static void phytium_dmufreq_restart_clear_timer_v2(struct phytium_dmufreq *priv)
{
	dmu_v2_write32(priv, DMU_V2_PDM_START + DMU_V2_EVENT_CLEAR, 0x1);
	dmu_v2_write32(priv, DMU_V2_PDM_START +
			DMU_V2_PDM_STRIDE + DMU_V2_EVENT_CLEAR, 0x1);
	dmu_v2_write32(priv, DMU_V2_PDM_START + DMU_V2_EVENT_CLEAR, 0x0);
	dmu_v2_write32(priv, DMU_V2_PDM_START +
			DMU_V2_PDM_STRIDE + DMU_V2_EVENT_CLEAR, 0x0);
}

static u64 phytium_dmufreq_get_real_bw_v2(struct phytium_dmufreq *priv)
{
	u32 lo, hi;
	u64 cnt_0, cnt_1, peak_bw;

	lo = dmu_v2_read32(priv, DMU_V2_PDM_START + DMU_V2_EVENT_L_CNT);
	hi = dmu_v2_read32(priv, DMU_V2_PDM_START + DMU_V2_EVENT_H_CNT);
	cnt_0 = ((u64)hi << 32) | lo;
	lo = dmu_v2_read32(priv, DMU_V2_PDM_START +
			DMU_V2_PDM_STRIDE + DMU_V2_EVENT_L_CNT);
	hi = dmu_v2_read32(priv, DMU_V2_PDM_START +
			DMU_V2_PDM_STRIDE + DMU_V2_EVENT_H_CNT);
	cnt_1 = ((u64)hi << 32) | lo;
	peak_bw = (cnt_0 + cnt_1) * 64UL;

	dev_dbg(priv->dev, "peak_bw = %llu\n", peak_bw);
	return peak_bw;
}

static void polling_handle_v2(struct phytium_dmufreq *priv)
{
	dmu_v2_write32(priv, DMU_V2_PDM_START + DMU_V2_MONITOR_SNAPSHOT, 0x1);
	dmu_v2_write32(priv, DMU_V2_PDM_START +
			DMU_V2_PDM_STRIDE + DMU_V2_MONITOR_SNAPSHOT, 0x1);
	priv->bandwidth = phytium_dmufreq_get_real_bw_v2(priv);
	dmu_v2_write32(priv, DMU_V2_PDM_START + DMU_V2_MONITOR_SNAPSHOT, 0x0);
	dmu_v2_write32(priv, DMU_V2_PDM_START +
			DMU_V2_PDM_STRIDE + DMU_V2_MONITOR_SNAPSHOT, 0x0);
}

/* devfreq Framework interface */
static int phytium_dmu_get_dev_status(struct device *dev, struct devfreq_dev_status *stat)
{
	struct phytium_dmufreq *priv = dev_get_drvdata(dev);

	if (priv->info->type == PHYTIUM_DMU_V1) {
		polling_handle_v1(priv);
		priv->last_bust_time = stat->busy_time = priv->bandwidth;
	} else {
		polling_handle_v2(priv);
		stat->busy_time = priv->bandwidth;
		phytium_dmufreq_restart_clear_timer_v2(priv);
	}
	stat->total_time = (priv->single_threshold_value * priv->rate) / priv->freq_table[0];
	stat->current_frequency = priv->rate;
	return 0;
}

/* Power Management */
static __maybe_unused int phytium_dmufreq_suspend(struct device *dev)
{
	struct phytium_dmufreq *priv = dev_get_drvdata(dev);
	int ret = devfreq_suspend_device(priv->devfreq);

	if (ret < 0)
		dev_err(dev, "failed to suspend the devfreq devices\n");
	return ret;
}

static __maybe_unused int phytium_dmufreq_resume(struct device *dev)
{
	struct phytium_dmufreq *priv = dev_get_drvdata(dev);
	int ret = devfreq_resume_device(priv->devfreq);

	if (ret < 0)
		dev_err(dev, "failed to resume the devfreq devices\n");
	return ret;
}

static SIMPLE_DEV_PM_OPS(phytium_dmufreq_pm, phytium_dmufreq_suspend, phytium_dmufreq_resume);

static int phytium_dmufreq_probe(struct platform_device *pdev)
{
	struct phytium_dmufreq *priv;
	struct device *dev = &pdev->dev;
	const struct phytium_dmufreq_info *info;
	struct acpi_result result;
	struct resource *res;
	const char *gov;
	void *gov_data = NULL;
	int i, ret;
	unsigned int max_state;

	info = device_get_match_data(dev);
	if (!info) {
		dev_err(dev, "No match data for this device\n");
		return -ENODEV;
	}

	max_state = get_freq_count(dev);
	if (max_state <= 0)
		return -EINVAL;

	result = phytium_dmufreq_state(dev);
	if (result.value == 0) {
		dev_err(dev, "DMUFREQ is not enabled\n");
		return -ENODEV;
	}

	priv = devm_kzalloc(dev, struct_size(priv, freq_table, max_state), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	priv->info = info;

	if (info->type == PHYTIUM_DMU_V1) {
		gov = DEVFREQ_GOV_PERFORMANCE;
		gov_data = NULL;
	} else {
		gov = DEVFREQ_GOV_SIMPLE_ONDEMAND;
		gov_data = &priv->ondemand_data;
	}

	result = phytium_current_enabled_channels(dev);
	if (result.status) {
		dev_err(dev, "Failed to get enabled channels\n");
		return -EINVAL;
	}

	priv->max_count = result.value;
	result = phytium_read_threshold_value(dev);
	if (result.status) {
		dev_err(dev, "Failed to get threshold value\n");
		return -EINVAL;
	}
	priv->single_threshold_value = result.value;
	priv->single_threshold_value = (priv->single_threshold_value * 1024 * 1024) / 10;
	platform_set_drvdata(pdev, priv);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (info->type == PHYTIUM_DMU_V1) {
		/* V1: Multi-channel, allocation base/read_bw/write_bw */
		priv->basev1 = devm_kcalloc(dev, priv->max_count,
				sizeof(void __iomem *), GFP_KERNEL);
		priv->read_bw = devm_kcalloc(dev, priv->max_count,
				sizeof(unsigned long), GFP_KERNEL);
		priv->write_bw = devm_kcalloc(dev, priv->max_count,
				sizeof(unsigned long), GFP_KERNEL);
		if (!priv->basev1 || !priv->read_bw || !priv->write_bw)
			return -ENOMEM;
		for (i = 0; i < priv->max_count; i++) {
			resource_size_t offset = res->start + i * DMU_V1_PMU_STRIDE;

			priv->basev1[i] = devm_ioremap(dev, offset, resource_size(res));
			if (IS_ERR(priv->basev1[i]))
				return PTR_ERR(priv->basev1[i]);
		}
		/* Register PMU notification chain */
		priv->nb.notifier_call = dmu_pmu_notifier_call;
		ret = blocking_notifier_chain_register(&dmu_pmu_notifier_chain, &priv->nb);
		if (ret) {
			dev_err(dev, "Failed to register notifier\n");
			return ret;
		}
		priv->pmu_active = true;
		priv->cnt = 1;
		dev->init_name = "dmufreq";
	} else {
		/* V2: Single-channel multi-instance, allocate base, get uid */
		unsigned long long uid;

		mutex_init(&priv->lock);
		acpi_evaluate_integer(ACPI_HANDLE(dev), "_UID", NULL, &uid);
		priv->uid = uid;
		priv->basev2 = devm_ioremap(dev, res->start, resource_size(res));
		if (IS_ERR(priv->basev2))
			return PTR_ERR(priv->basev2);
		dev_set_name(dev, "dmu%u", priv->uid);
	}

	ret = phytium_dmu_get_freq_info(dev);
	if (ret)
		return ret;

	priv->profile.initial_freq = priv->freq_table[0];
	priv->profile.polling_ms = 100;
	priv->profile.timer = DEVFREQ_TIMER_DELAYED;
	priv->profile.target = phytium_dmu_target;
	priv->profile.get_cur_freq = phytium_dmu_get_cur_freq;
	priv->profile.get_dev_status = phytium_dmu_get_dev_status;
	priv->profile.freq_table = priv->freq_table;
	priv->rate = priv->profile.initial_freq;
	priv->profile.max_state = priv->freq_count;
	priv->ondemand_data.upthreshold = 80;
	priv->ondemand_data.downdifferential = 10;

	for (i = 0; i < max_state; ++i) {
		ret = dev_pm_opp_add(dev, priv->freq_table[i], 0);
		if (ret < 0)
			goto err;
	}

	priv->devfreq = devm_devfreq_add_device(dev, &priv->profile, gov, gov_data);
	if (IS_ERR(priv->devfreq)) {
		ret = PTR_ERR(priv->devfreq);
		goto err;
	}

	/* v1: Enable PMU */
	if (info->type == PHYTIUM_DMU_V1 && priv->pmu_active) {
		for (i = 0; i < priv->max_count; i++) {
			dmu_v1_write32(priv, i, DMU_V1_AXI_MONITOR_EN, 0x101);
			dmu_v1_write32(priv, i, DMU_V1_CLEAR_EVENT, 0x1);
			dmu_v1_write32(priv, i, DMU_V1_TIMER_START, 0x1);
		}
	}

	/* v2: Enable Monitoring */
	if (info->type == PHYTIUM_DMU_V2) {
		dmu_v2_write32(priv, DMU_V2_PDM_START +
				DMU_V2_MONITOR_START, 0x1);
		dmu_v2_write32(priv, DMU_V2_PDM_START +
				DMU_V2_PDM_STRIDE + DMU_V2_MONITOR_START, 0x1);
	}

	priv->dev = dev;
	return 0;
err:
	dev_pm_opp_of_remove_table(dev);
	return ret;
}

static int phytium_dmufreq_remove(struct platform_device *pdev)
{
	struct phytium_dmufreq *priv = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	int i;

	if (priv->info->type == PHYTIUM_DMU_V1) {
		for (i = 0; i < priv->max_count; i++) {
			dmu_v1_write32(priv, i, DMU_V1_TIMER_STOP, 0x1);
			dmu_v1_write32(priv, i, DMU_V1_AXI_MONITOR_EN, 0x0);
		}
		blocking_notifier_chain_unregister(&dmu_pmu_notifier_chain, &priv->nb);
	}
	if (!priv->devfreq)
		return 0;
	dev_pm_opp_remove_all_dynamic(dev);
	return 0;
}

/* Matching Type Table */
static const struct phytium_dmufreq_info phytium_dmu_v1_info = {
	.type = PHYTIUM_DMU_V1,
	.name = "phytium_dmu_v1",
};

static const struct phytium_dmufreq_info phytium_dmu_v2_info = {
	.type = PHYTIUM_DMU_V2,
	.name = "phytium_dmu_v2",
};

#ifdef CONFIG_ACPI
static const struct acpi_device_id phytium_dmufreq_acpi_ids[] = {
	{ "PHYT0063", (kernel_ulong_t)&phytium_dmu_v1_info },
	{ "PHYT3011", (kernel_ulong_t)&phytium_dmu_v2_info },
	{},
};
MODULE_DEVICE_TABLE(acpi, phytium_dmufreq_acpi_ids);
#endif

static struct platform_driver phytium_dmufreq_driver = {
	.probe		= phytium_dmufreq_probe,
	.remove		= phytium_dmufreq_remove,
	.driver = {
		.name	= "phytium_dmufreq",
		.pm	= &phytium_dmufreq_pm,

#ifdef CONFIG_ACPI
		.acpi_match_table = phytium_dmufreq_acpi_ids,
#endif
		.suppress_bind_attrs = true,
	}
};
module_platform_driver(phytium_dmufreq_driver);

MODULE_DESCRIPTION("Phytium DDR Memory Unit frequency driver (v1/v2 unified)");
MODULE_AUTHOR("Li Mingzhe <limingzhe@phytium.com.cn>");
MODULE_LICENSE("GPL");
MODULE_VERSION(DMUFREQ_DRIVER_VERSION);

// SPDX-License-Identifier: GPL-1.0
/*
 *phytium_dmu.c - Phytium Processor dmu Frequency Driver
 *
 *Copyright (C) 2024,Phytium Technology Co.,Ltd.
 */
#include <linux/module.h>
#include <linux/device.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/devfreq.h>
#include <linux/pm_opp.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/arm-smccc.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/property.h>
#include <linux/acpi.h>
#include <linux/kthread.h>
#include <linux/notifier.h>
#include <linux/suspend.h>

#define DEBUG

#define DEVICE_TYPE 9	//DMU ID

#define	DMU_PMU_STRIDE		0x80000

#define	AXI_MONITOR2_L		0x084
#define	AXI_MONITOR3_L		0x08c
#define AXI_MONITOR_EN		0X01c
#define TIMER_START		0X000
#define TIMER_STOP		0X004
#define CLEAR_EVENT		0X008

#define MCU_STRIDE			0x00080000
/* PMU notifier event */
#define DDR_PMU_NOTICE_START  0x0
#define DDR_PMU_NOTICE_STOP   0x1

#define DMUFREQ_DRIVER_VERSION "1.0.2"

struct phytium_dmufreq {
	struct device *dev;

	struct devfreq *devfreq;
	struct devfreq_dev_profile profile;
	struct devfreq_simple_ondemand_data ondemand_data;

	unsigned long	rate, target_rate;
	unsigned long	bandwidth;
	unsigned long	single_threshold_value;
	int max_count;
	int cnt;

	void __iomem	**base;

	unsigned long	*read_bw;
	unsigned long	*write_bw;

	struct notifier_block nb;

	/*dmu to pmu operation status identification 0: not operable, 1: operable*/
	bool pmu_active;

	unsigned long last_bust_time;

	unsigned int	freq_count;
	unsigned long	freq_table[];
};

struct acpi_result {
	int status;
	unsigned long long value;
};

static inline void dmu_write32(struct phytium_dmufreq *priv, int dmu,
							unsigned long offest, unsigned long value)
{
	writel_relaxed(value, priv->base[dmu] + offest);
}

static inline unsigned long dmu_read32(struct phytium_dmufreq *priv, int dmu,
									unsigned long offest)
{
	return readl_relaxed(priv->base[dmu] + offest);
}

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


static int phytium_dmu_set_freq(struct device *dev, unsigned long freq)
{
	acpi_handle handle = ACPI_HANDLE(dev);
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
	args[2].type = ACPI_TYPE_INTEGER;
	args[2].integer.value = 0;
	args[3].type = ACPI_TYPE_INTEGER;
	args[3].integer.value = 0;

	status = acpi_evaluate_integer(handle, "PSCF", &arg_list, &ret);
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
	/*
	 * Read back the clk rate to verify switch was correct and so that
	 * we can report it on all error paths.
	 */
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

struct acpi_result phytium_current_enabled_channels(struct device *dev)
{
	acpi_handle handle = ACPI_HANDLE(dev);
	acpi_status status;
	unsigned long long enabled_channels;
	struct acpi_result result;

	status = acpi_evaluate_integer(handle, "CHAN", NULL, &enabled_channels);
	if (ACPI_FAILURE(status)) {
		dev_err(dev, "Failed to evaluate CHAN method: ACPI status 0x%x\n", status);
		result.status = -EIO;
		result.value = 0;
		return result;
	}
	dev_dbg(dev, "enabled_channels = %lld\n", enabled_channels);
	result.status = 0;
	result.value = enabled_channels;
	return result;
}

struct acpi_result phytium_controller_bit_width(struct device *dev)
{
	acpi_handle handle = ACPI_HANDLE(dev);
	acpi_status status;
	unsigned long long single_bit_width;
	struct acpi_result result;

	status = acpi_evaluate_integer(handle, "BITW", NULL, &single_bit_width);
	if (ACPI_FAILURE(status)) {
		dev_err(dev, "Failed to evaluate BITW method: ACPI status 0x%x\n", status);
		result.status = -EIO;
		result.value = 0;
		return result;
	}
	dev_dbg(dev, "single_bit_width = %lld(MB/s)\n", single_bit_width);
	result.status = 0;
	result.value = single_bit_width;
	return result;
}

struct acpi_result phytium_dmufreq_state(struct device *dev)
{
	struct acpi_result result;
	acpi_handle handle = ACPI_HANDLE(dev);
	acpi_status status;
	unsigned long long dmufreq_state;

	status = acpi_evaluate_integer(handle, "STAT", NULL, &dmufreq_state);
	if (ACPI_FAILURE(status)) {
		dev_err(dev, "Failed to evaluate STAT method: ACPI status 0x%x\n", status);
		result.status = -EIO;
		result.value = 0;
		return result;
	}
	dev_dbg(dev, "dmufreq_state = %lld\n", dmufreq_state);
	result.status = 0;
	result.value = dmufreq_state;
	return result;
}

struct acpi_result phytium_read_threshold_value(struct device *dev)
{
	acpi_handle handle = ACPI_HANDLE(dev);
	acpi_status status;
	unsigned long long single_threshold_value;
	struct acpi_result result;

	status = acpi_evaluate_integer(handle, "BAND", NULL, &single_threshold_value);
	if (ACPI_FAILURE(status)) {
		WARN_ONCE(1, "Failed to evaluate BAND method: ACPI status 0x%x\n", status);
		result.status = -EIO;
		result.value = 0;
		return result;
	}
	dev_dbg(dev, "single_threshold_value = %llu\n", single_threshold_value);
	result.status = 0;
	result.value = single_threshold_value;
	return result;
}

static u64 phytium_dmufreq_get_real_bw(struct phytium_dmufreq *priv)
{
	unsigned long peak_bw = 0;
	unsigned long sum_peak_bw = 0;
	int i;

	for (i = 0; i < priv->max_count; i++) {
		priv->read_bw[i] = dmu_read32(priv, i, AXI_MONITOR2_L);
		priv->write_bw[i] = dmu_read32(priv, i, AXI_MONITOR3_L);

		/*clear the counter(only pmu_reg active)*/
		dmu_write32(priv, i, CLEAR_EVENT, 0x1);
		dmu_write32(priv, i, TIMER_START, 0x1);
		sum_peak_bw = priv->read_bw[i] + priv->write_bw[i];
		if (sum_peak_bw > peak_bw)
			peak_bw = sum_peak_bw;
	}
	dev_dbg(priv->dev, "peak_bw = %lu\n", peak_bw);
	return peak_bw;
}

static void polling_handle(struct phytium_dmufreq *priv)
{
	int i;

	/*if the pmu_reg is not active, return the last busy time(pmu_reg not work)*/
	if (!priv->pmu_active) {
		priv->bandwidth = priv->last_bust_time;
		return;
	}
	if (priv->cnt > 0) {
		for (i = 0; i < priv->max_count ; i++) {
			dmu_write32(priv, i, AXI_MONITOR_EN, 0x101);
			dmu_write32(priv, i, TIMER_STOP, 0x1);
		}
		priv->bandwidth = phytium_dmufreq_get_real_bw(priv);
	}
	priv->cnt = 1;
}

static int phytium_dmu_get_dev_status(struct device *dev,
					  struct devfreq_dev_status *stat)
{
	struct phytium_dmufreq *priv = dev_get_drvdata(dev);

	polling_handle(priv);
	priv->last_bust_time = stat->busy_time = priv->bandwidth;
	stat->total_time = (priv->single_threshold_value * priv->rate) / priv->freq_table[0];

	dev_dbg(dev, "busy_time = %lu, total_time = %lu,single_threshold_value = %llu\n",
		stat->busy_time, stat->total_time, priv->single_threshold_value);

	stat->current_frequency	= priv->rate;
	return 0;
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
	freq_count = element->integer.value;
	dev_dbg(dev, "freq_count = %d\n", freq_count);

	return freq_count;
}

static __maybe_unused int phytium_dmufreq_suspend(struct device *dev)
{
	struct phytium_dmufreq *priv = dev_get_drvdata(dev);
	int ret = 0;

	dev_dbg(dev, "DMU is being suspended\n");

	ret = devfreq_suspend_device(priv->devfreq);
	if (ret < 0) {
		dev_err(dev, "failed to suspend the devfreq devices\n");
		return ret;
	}

	return 0;
}

static __maybe_unused int phytium_dmufreq_resume(struct device *dev)
{
	struct phytium_dmufreq *priv = dev_get_drvdata(dev);
	int ret = 0;

	dev_dbg(dev, "DMU is being resumed\n");

	ret = devfreq_resume_device(priv->devfreq);
	if (ret < 0) {
		dev_err(dev, "failed to resume the devfreq devices\n");
		return ret;
	}

	return 0;
}

static SIMPLE_DEV_PM_OPS(phytium_dmufreq_pm, phytium_dmufreq_suspend,
				phytium_dmufreq_resume);

static int phytium_dmufreq_probe(struct platform_device *pdev)
{
	struct phytium_dmufreq *priv;
	struct device *dev = &pdev->dev;
	const char *gov = DEVFREQ_GOV_PERFORMANCE;
	int i, ret;
	unsigned int max_state = get_freq_count(dev);
	struct acpi_result result;
	struct resource *res;

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

	dev->init_name = "dmufreq";

	priv->base = devm_kcalloc(dev, priv->max_count, sizeof(void __iomem *), GFP_KERNEL);
	priv->read_bw = devm_kcalloc(dev, priv->max_count, sizeof(unsigned long), GFP_KERNEL);
	priv->write_bw = devm_kcalloc(dev, priv->max_count, sizeof(unsigned long), GFP_KERNEL);
	if (!priv->base || !priv->read_bw || !priv->write_bw) {
		dev_err(dev, "failed to allocate memory\n");
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, priv);

	/* Register the notifier */
	priv->nb.notifier_call = dmu_pmu_notifier_call;
	ret = blocking_notifier_chain_register(&dmu_pmu_notifier_chain, &priv->nb);
	if (ret) {
		dev_err(dev, "Failed to register notifier\n");
		return ret;
	}

	/* Get the base address of the DMU PMU */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	for (i = 0; i < priv->max_count; i++) {
		resource_size_t offset = res->start + i * DMU_PMU_STRIDE;

		priv->base[i] = devm_ioremap(&pdev->dev, offset, resource_size(res));
			if (IS_ERR(priv->base[i])) {
				dev_err(dev, "Ioremap failed for dmu base resource\n");
				return PTR_ERR(priv->base);
			}
	}

	ret = phytium_dmu_get_freq_info(dev);
	if (ret) {
		dev_err(dev, "failed to get ddr frequency info\n");
		return -EIO;
	}

	priv->pmu_active			= true;
	priv->cnt				= 1;
	priv->profile.initial_freq		= priv->freq_table[0];
	priv->profile.polling_ms		= 100;
	priv->profile.timer			= DEVFREQ_TIMER_DELAYED;
	priv->profile.target			= phytium_dmu_target;
	priv->profile.get_cur_freq		= phytium_dmu_get_cur_freq;
	priv->profile.get_dev_status		= phytium_dmu_get_dev_status;
	priv->profile.freq_table		= priv->freq_table;
	priv->rate				= priv->profile.initial_freq;
	priv->profile.max_state			= priv->freq_count;
	priv->ondemand_data.upthreshold		= 80;
	priv->ondemand_data.downdifferential	= 10;

	for (i = 0; i < max_state; ++i) {
		ret = dev_pm_opp_add(dev, priv->freq_table[i], 0);
		if (ret < 0) {
			dev_err(dev, "failed to get OPP table\n");
			goto err;
		}
	}

	priv->devfreq = devm_devfreq_add_device(dev, &priv->profile,
						gov, NULL);
	if (IS_ERR(priv->devfreq)) {
		ret = PTR_ERR(priv->devfreq);
		dev_err(dev, "failed to add devfreq device: %d\n", ret);
		goto err;
	}

	/*Enable PMU*/
	if (priv->pmu_active) {
		for (i = 0; i < priv->max_count; i++) {
			dmu_write32(priv, i, AXI_MONITOR_EN, 0x101);
			dmu_write32(priv, i, CLEAR_EVENT, 0x1);
			dmu_write32(priv, i, TIMER_START, 0x1);
		}
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

	for (i = 0; i < priv->max_count; i++) {
		dmu_write32(priv, i, TIMER_STOP, 0x1);
		dmu_write32(priv, i, AXI_MONITOR_EN, 0x0);
	}

	/*Unregister the notifier*/
	blocking_notifier_chain_unregister(&dmu_pmu_notifier_chain, &priv->nb);

	if (!priv->devfreq)
		return 0;
	dev_pm_opp_remove_all_dynamic(dev);

	return 0;
}

#ifdef CONFIG_ACPI
static const struct acpi_device_id phytium_dmufreq_acpi_ids[] = {
	{"PHYT0063"},
	{},
};

MODULE_DEVICE_TABLE(acpi, phytium_dmufreq_acpi_ids);
#else
#define phytium_dmu_acpi_ids NULL
#endif

struct notifier_block nb = {
	.notifier_call = dmu_pmu_notifier_call,
};

static struct platform_driver phytium_dmufreq_driver = {
	.probe		= phytium_dmufreq_probe,
	.remove		= phytium_dmufreq_remove,
	.driver = {
		.name	= "phytium_dmufreq",
		.pm	= &phytium_dmufreq_pm,
		.acpi_match_table = ACPI_PTR(phytium_dmufreq_acpi_ids),
		.suppress_bind_attrs = true,
	},
};
module_platform_driver(phytium_dmufreq_driver);

MODULE_DESCRIPTION("Phytium DDR Memory Unit frequency driver");
MODULE_AUTHOR("Li Jiayi <lijiayi@phytium.com.cn>");
MODULE_AUTHOR("Li Mingzhe <limingzhe@phytium.com.cn>");
MODULE_LICENSE("GPL");
MODULE_VERSION(DMUFREQ_DRIVER_VERSION);

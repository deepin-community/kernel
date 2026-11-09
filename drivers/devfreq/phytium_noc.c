// SPDX-License-Identifier: GPL-1.0
/*
 *phytium_noc.c - Phytium Processor noc Frequency Driver
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
#include <linux/acpi.h>

#define MINI_SIZE 0x400
#define CNT_ENABLE 0x000
#define WORK_STATE 0X004
#define CLR_EN 0X010
#define SNAPSHOT_EN 0X014
#define INT_CTRL_CLR 0x024
#define WR_NOLAST_HANDSHARK_NUM 0x44

#define DEBUG
#define DEVICE_TYPE 7

#define NOCFREQ_DRIVER_VERSION "1.0.0"

struct phytium_nocfreq {
	struct device *dev;

	struct devfreq *devfreq;
	struct devfreq_dev_profile profile;
	struct devfreq_simple_ondemand_data ondemand_data;

	void __iomem *reg_noc;
	struct mutex lock;

	unsigned long rate, target_rate;
	unsigned int freq_count;
	unsigned long freq_table[];
};

static u32 phytium_nocfreq_get_peak_bw(struct phytium_nocfreq *priv)
{
	/*Returns the peak number of dmu read/write commands on the axi bus.*/
	unsigned long peak_bw, bw_0, bw_1, bw_2, bw_3;

	bw_0 = readl_relaxed(priv->reg_noc + WR_NOLAST_HANDSHARK_NUM);
	bw_1 = readl_relaxed(priv->reg_noc + MINI_SIZE*1 + WR_NOLAST_HANDSHARK_NUM);
	bw_2 = readl_relaxed(priv->reg_noc + MINI_SIZE*2 + WR_NOLAST_HANDSHARK_NUM);
	bw_3 = readl_relaxed(priv->reg_noc + MINI_SIZE*3 + WR_NOLAST_HANDSHARK_NUM);

	peak_bw = bw_0;
	if (bw_1 > peak_bw)
		peak_bw = bw_1;
	if (bw_2 > peak_bw)
		peak_bw = bw_2;
	if (bw_3 > peak_bw)
		peak_bw = bw_3;

	return peak_bw;
}

static void phytium_nocfreq_restart_handshark_counters(struct phytium_nocfreq *priv)
{

	/*clear interrupt*/

	writel_relaxed(0x80000000, priv->reg_noc + INT_CTRL_CLR);
	writel_relaxed(0x80000000, priv->reg_noc + MINI_SIZE*1 + INT_CTRL_CLR);
	writel_relaxed(0x80000000, priv->reg_noc + MINI_SIZE*2 + INT_CTRL_CLR);
	writel_relaxed(0x80000000, priv->reg_noc + MINI_SIZE*3 + INT_CTRL_CLR);

	/*clear counters*/
	writel_relaxed(0x1, priv->reg_noc + CLR_EN);
	writel_relaxed(0x1, priv->reg_noc + MINI_SIZE*1 + CLR_EN);
	writel_relaxed(0x1, priv->reg_noc + MINI_SIZE*2 + CLR_EN);
	writel_relaxed(0x1, priv->reg_noc + MINI_SIZE*3 + CLR_EN);
}


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
	args[0].integer.value = DEVICE_TYPE;
	args[1].type = ACPI_TYPE_INTEGER;
	args[1].integer.value = freq;
	args[2].type = ACPI_TYPE_INTEGER;
	args[2].integer.value = 0;
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

	if (target_rate == old_freq)
		return 0;
	/*
	 * Read back the clk rate to verify switch was correct and so that
	 * we can report it on all error paths.
	 */
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
	args[0].integer.value = DEVICE_TYPE;
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

	return freq_count;
}

static int phytium_noc_get_dev_status(struct device *dev,
					  struct devfreq_dev_status *stat)
{
	struct phytium_nocfreq *priv = dev_get_drvdata(dev);
	unsigned int val;

	writel_relaxed(0x1, priv->reg_noc + SNAPSHOT_EN);
	writel_relaxed(0x1, priv->reg_noc + MINI_SIZE*1 + SNAPSHOT_EN);
	writel_relaxed(0x1, priv->reg_noc + MINI_SIZE*2 + SNAPSHOT_EN);
	writel_relaxed(0x1, priv->reg_noc + MINI_SIZE*3 + SNAPSHOT_EN);

	val = DIV_ROUND_CLOSEST(priv->rate * 100, priv->profile.initial_freq);
	stat->busy_time		= phytium_nocfreq_get_peak_bw(priv);
	stat->total_time	= 320000 * val;
	stat->current_frequency	= priv->rate;

	phytium_nocfreq_restart_handshark_counters(priv);
	dev_dbg(dev, "Using %lu/%lu (%lu%%) at %lu KHz\n",
		stat->busy_time, stat->total_time,
		DIV_ROUND_CLOSEST(stat->busy_time * 100, stat->total_time),
		stat->current_frequency);

	return 0;
}


static int phytium_nocfreq_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct phytium_nocfreq *priv;
	const char *gov = DEVFREQ_GOV_SIMPLE_ONDEMAND;
	int i, ret;
	unsigned int max_state;
	struct resource *mem;

	max_state = get_freq_count(dev);

	dev->init_name = "nocfreq";

	priv = devm_kzalloc(dev, struct_size(priv, freq_table, max_state), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	mutex_init(&priv->lock);
	platform_set_drvdata(pdev, priv);

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource");
		return -EINVAL;
	}

	priv->reg_noc = devm_ioremap_resource(&pdev->dev, mem);
	if (!priv->reg_noc) {
		dev_err(dev, "NOC region map failed\n");
		return PTR_ERR(priv->reg_noc);
	}

	ret = phytium_noc_get_freq_info(dev, DEVICE_TYPE);
	if (ret) {
		dev_err(dev, "failed to get noc frequency info\n");
		return -EIO;
	}

	priv->profile.initial_freq		= priv->freq_table[0];
	priv->profile.polling_ms		= 100;
	priv->profile.target			= phytium_noc_target;
	priv->profile.get_cur_freq		= phytium_noc_get_cur_freq;
	priv->profile.get_dev_status		= phytium_noc_get_dev_status;
	priv->profile.freq_table		= priv->freq_table;
	priv->profile.max_state			= priv->freq_count;
	priv->rate				= priv->freq_table[0];
	priv->ondemand_data.upthreshold		= 80;
	priv->ondemand_data.downdifferential	= 10;
	priv->profile.max_state			= priv->freq_count;

	for (i = 0; i < max_state; ++i) {
		ret = dev_pm_opp_add(dev, priv->freq_table[i], 0);
		if (ret < 0) {
			dev_err(dev, "failed to get OPP table\n");
			goto err;
		}
	}
	priv->devfreq = devm_devfreq_add_device(dev, &priv->profile,
					gov, &priv->ondemand_data);
	if (IS_ERR(priv->devfreq)) {
		ret = PTR_ERR(priv->devfreq);
		dev_err(dev, "failed to add devfreq device: %d\n", ret);
		goto err;
	}

	ret = phytium_noc_set_freq(dev, priv->profile.initial_freq);
	if (ret)
		dev_warn(dev, "failed to init noc frequency: %d\n", ret);

	writel_relaxed(0x02, priv->reg_noc + WORK_STATE);
	writel_relaxed(0x02, priv->reg_noc + MINI_SIZE*1 + WORK_STATE);
	writel_relaxed(0x02, priv->reg_noc + MINI_SIZE*2 + WORK_STATE);
	writel_relaxed(0x02, priv->reg_noc + MINI_SIZE*3 + WORK_STATE);

	writel_relaxed(0x3f, priv->reg_noc + CNT_ENABLE);
	writel_relaxed(0x3f, priv->reg_noc + MINI_SIZE*1 + CNT_ENABLE);
	writel_relaxed(0x3f, priv->reg_noc + MINI_SIZE*2 + CNT_ENABLE);
	writel_relaxed(0x3f, priv->reg_noc + MINI_SIZE*3 + CNT_ENABLE);
	return 0;

err:
	dev_pm_opp_of_remove_table(dev);
	kfree(priv);
	return ret;
}

static int phytium_nocfreq_remove(struct platform_device *pdev)
{
	struct phytium_nocfreq *priv = platform_get_drvdata(pdev);
	unsigned long initial_freq = priv->profile.initial_freq;
	struct device *dev = &pdev->dev;
	int ret;

	writel_relaxed(0x0, priv->reg_noc + CNT_ENABLE);
	writel_relaxed(0x0, priv->reg_noc + MINI_SIZE*1 + CNT_ENABLE);
	writel_relaxed(0x0, priv->reg_noc + MINI_SIZE*2 + CNT_ENABLE);
	writel_relaxed(0x0, priv->reg_noc + MINI_SIZE*3 + CNT_ENABLE);

	writel_relaxed(0x1, priv->reg_noc + CLR_EN);
	writel_relaxed(0x1, priv->reg_noc + MINI_SIZE*1 + CLR_EN);
	writel_relaxed(0x1, priv->reg_noc + MINI_SIZE*2 + CLR_EN);
	writel_relaxed(0x1, priv->reg_noc + MINI_SIZE*3 + CLR_EN);

	ret = phytium_noc_set_freq(dev, initial_freq);
	if (ret)
		dev_warn(dev, "failed to restore NOC frequency: %d\n", ret);

	iounmap(priv->reg_noc);

	if (!priv->devfreq)
		return 0;

	dev_pm_opp_remove_all_dynamic(dev);

	kfree(priv);
	return 0;
}

static const struct acpi_device_id phytium_noc_acpi_ids[] = {
	{"PHYT0047"},
	{},
};

MODULE_DEVICE_TABLE(acpi, phytium_noc_acpi_ids);

static struct platform_driver phytium_nocfreq_driver = {
	.probe		= phytium_nocfreq_probe,
	.remove		= phytium_nocfreq_remove,
	.driver = {
		.name			= "phytium_nocfreq",
		.acpi_match_table	= ACPI_PTR(phytium_noc_acpi_ids),
		.suppress_bind_attrs	= true,
	},
};
module_platform_driver(phytium_nocfreq_driver);

MODULE_DESCRIPTION("Phytium NOC Controller frequency driver");
MODULE_AUTHOR("Li Jiayi <lijiayi@phytium.com.cn>");
MODULE_LICENSE("GPL");
MODULE_VERSION(NOCFREQ_DRIVER_VERSION);

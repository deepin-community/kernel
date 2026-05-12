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

#define DEBUG

#define DEVICE_TYPE 9	//DMU ID

#define UPDATE_INTERVAL_MS 10

#define DMUFREQ_DRIVER_VERSION "1.0.0"

struct phytium_dmufreq {
	struct device *dev;

	struct devfreq *devfreq;
	struct devfreq_dev_profile profile;
	struct devfreq_simple_ondemand_data ondemand_data;

	unsigned long	rate, target_rate;
	unsigned long	bandwidth;

	struct timer_list sampling;
	struct work_struct work;

	unsigned int	freq_count;
	unsigned long	freq_table[];
};

static ktime_t stop;

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

static int phytium_read_perf_counter(struct device *dev)
{
	struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };
	union acpi_object *package, *elements;
	acpi_handle handle = ACPI_HANDLE(dev);
	acpi_status status;

	status = acpi_evaluate_object(handle, "PDMU", NULL, &buffer);
	if (ACPI_FAILURE(status)) {
		dev_err(dev, "No PGCL method\n");
		return -EIO;
	}

	package = buffer.pointer;

	elements = package->package.elements;

	return elements[0].integer.value + elements[1].integer.value;
}

static void sampling_timer_callback(struct timer_list *t)
{
	struct phytium_dmufreq *priv = from_timer(priv, t, sampling);

	schedule_work(&priv->work);
}

static void sampling_work_handle(struct work_struct *work)
{
	struct phytium_dmufreq *priv = container_of(work, struct phytium_dmufreq, work);
	struct device *dev = priv->dev;
	static unsigned long load_counter;
	static int count;
	unsigned long current_load;

	current_load = phytium_read_perf_counter(dev);

	load_counter += current_load;
	count += 1;

	if (ktime_after(ktime_get(), stop)) {
		priv->bandwidth = load_counter / count;
		load_counter = 0;
		count = 0;
		stop = ktime_add_ms(ktime_get(), priv->profile.polling_ms);
		mod_timer(&priv->sampling, jiffies + msecs_to_jiffies(UPDATE_INTERVAL_MS));
	} else
		mod_timer(&priv->sampling, jiffies + msecs_to_jiffies(UPDATE_INTERVAL_MS));
}

static int phytium_dmu_get_dev_status(struct device *dev,
					  struct devfreq_dev_status *stat)
{
	struct phytium_dmufreq *priv = dev_get_drvdata(dev);

	stat->busy_time = priv->bandwidth;
	stat->total_time = (500000 * priv->rate) / priv->freq_table[0];
	dev_dbg(dev, "busy_time = %lu, total_time = %lu\n",
			stat->busy_time, stat->total_time);

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

static int phytium_dmufreq_probe(struct platform_device *pdev)
{
	struct phytium_dmufreq *priv;
	struct device *dev = &pdev->dev;
	const char *gov = DEVFREQ_GOV_SIMPLE_ONDEMAND;
	int i, ret;
	unsigned int max_state = get_freq_count(dev);

	if (max_state <= 0)
		return max_state;

	dev->init_name = "dmufreq";

	priv = kzalloc(sizeof(struct phytium_dmufreq) +
		       max_state * sizeof(unsigned long), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	platform_set_drvdata(pdev, priv);

	ret = phytium_dmu_get_freq_info(dev);
	if (ret) {
		dev_err(dev, "failed to get ddr frequency info\n");
		return -EIO;
	}

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
						gov, &priv->ondemand_data);
	if (IS_ERR(priv->devfreq)) {
		ret = PTR_ERR(priv->devfreq);
		dev_err(dev, "failed to add devfreq device: %d\n", ret);
		goto err;
	}

	INIT_WORK(&priv->work, sampling_work_handle);
	timer_setup(&priv->sampling, sampling_timer_callback, 0);
	stop = ktime_add_ms(ktime_get(), priv->profile.polling_ms);
	mod_timer(&priv->sampling, jiffies + msecs_to_jiffies(UPDATE_INTERVAL_MS));

	priv->dev = dev;

	return 0;

err:
	dev_pm_opp_of_remove_table(dev);
	kfree(priv);
	return ret;
}

static int phytium_dmufreq_remove(struct platform_device *pdev)
{
	struct phytium_dmufreq *priv = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;


	if (!priv->devfreq)
		return 0;
	flush_work(&priv->work);
	del_timer_sync(&priv->sampling);

	dev_pm_opp_remove_all_dynamic(dev);

	kfree(priv);

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

static struct platform_driver phytium_dmufreq_driver = {
	.probe		= phytium_dmufreq_probe,
	.remove		= phytium_dmufreq_remove,
	.driver = {
		.name	= "phytium_dmufreq",
		.acpi_match_table = ACPI_PTR(phytium_dmufreq_acpi_ids),
		.suppress_bind_attrs = true,
	},
};
module_platform_driver(phytium_dmufreq_driver);

MODULE_DESCRIPTION("Phytium DDR Memory Unit frequency driver");
MODULE_AUTHOR("Li Jiayi <lijiayi@phytium.com.cn>");
MODULE_LICENSE("GPL");
MODULE_VERSION(DMUFREQ_DRIVER_VERSION);

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Phytium I2C adapter driver.
 *
 * Copyright (C) 2023-2024, Phytium Technology Co., Ltd.
 */
#include <linux/acpi.h>
#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dmi.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/property.h>
#include <linux/reset.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/suspend.h>

#include "i2c-phyt-core.h"

#define DRV_NAME "i2c-phytium-v2-platform"

static u32 i2c_phyt_get_clk_rate_khz(struct i2c_phyt_dev *dev)
{
	return clk_get_rate(dev->clk)/1000;
}

static ssize_t debug_show(struct device *dev,
			struct device_attribute *da,
			char *buf)
{
	struct i2c_phyt_dev *adapter_dev = dev_get_drvdata(dev);
	ssize_t ret;
	u32 reg;

	reg = i2c_phyt_read_reg(adapter_dev, FT_I2C_REGFILE_DEBUG);
	ret = sprintf(buf, "%x\n", reg);

	return ret;
}

static ssize_t debug_store(struct device *dev,
					struct device_attribute *da,
					const char *buf, size_t size)
{
	u8 loc, dis_en, status = 0;
	char *p;
	char *token;
	long value;
	u32 reg;
	struct i2c_phyt_dev *adapter_dev = dev_get_drvdata(dev);

	dev_info(dev, "echo alive(1)/debug(0) enable(1)/disable(0) > debug\n");
	dev_info(dev, "Example:echo 0 1 > debug; Enable Debug Function\n");

	p = kmalloc(size, GFP_KERNEL);
	strscpy(p, buf, size);

	token = strsep(&p, " ");
	if (!token)
		return -EINVAL;

	status = kstrtol(token, 0, &value);
	if (status)
		return status;
	loc = (u8)value;

	token = strsep(&p, " ");
	if (!token)
		return -EINVAL;

	status = kstrtol(token, 0, &value);
	if (status)
		return status;
	dis_en = value;

	reg = i2c_phyt_read_reg(adapter_dev, FT_I2C_REGFILE_DEBUG);

	if (loc == 1) {
		if (dis_en == 1) {
			adapter_dev->alive_enabled = true;
			reg |= BIT(loc);
		}	else if (dis_en == 0) {
			adapter_dev->alive_enabled = false;
			reg &= ~BIT(loc);
		}
	} else if (loc == 0) {
		if (dis_en == 1) {
			adapter_dev->debug_enabled = true;
			reg |= BIT(loc);
		} else if (dis_en == 0) {
			adapter_dev->debug_enabled = false;
			reg &= ~BIT(loc);
		}
	}

	i2c_phyt_write_reg(adapter_dev, FT_I2C_REGFILE_DEBUG, reg);

	dev_info(dev, "reg write reg =0x%x, loc = %d, val=%d\n", reg, loc, dis_en);

	kfree(p);
	return size;
}
static DEVICE_ATTR_RW(debug);

static struct attribute *i2c_ft_device_attrs[] = {
	&dev_attr_debug.attr,
	NULL,
};

static const struct attribute_group i2c_ft_device_group = {
	.attrs = i2c_ft_device_attrs,
};

#ifdef CONFIG_ACPI
static void i2c_phyt_acpi_params(struct platform_device *pdev, char method[],
				u16 *hcnt, u16 *lcnt, u32 *sda_hold)
{
	struct acpi_buffer buf = { ACPI_ALLOCATE_BUFFER };
	acpi_handle handle = ACPI_HANDLE(&pdev->dev);
	union acpi_object *obj;

	if (ACPI_FAILURE(acpi_evaluate_object(handle, method, NULL, &buf)))
		return;

	obj = (union acpi_object *)buf.pointer;
	if (obj->type == ACPI_TYPE_PACKAGE && obj->package.count == 3) {
		const union acpi_object *objs = obj->package.elements;

		*hcnt = (u16)objs[0].integer.value;
		*lcnt = (u16)objs[1].integer.value;
		*sda_hold = (u32)objs[2].integer.value;
	}

	kfree(buf.pointer);
}

static int i2c_phyt_acpi_configure(struct platform_device *pdev)
{
	struct i2c_phyt_dev *dev = platform_get_drvdata(pdev);
	struct i2c_timings *t = &dev->timings;
	u32 ss_ht = 0, fp_ht = 0, hs_ht = 0, fs_ht = 0;
	const struct acpi_device_id *id;

	dev->adapter.nr = -1;

	/*
	 * Try to get SDA hold time and *CNT values from an ACPI method for
	 * selected speed modes.
	 */
	i2c_phyt_acpi_params(pdev, "SSCN", &dev->ss_hcnt, &dev->ss_lcnt, &ss_ht);
	i2c_phyt_acpi_params(pdev, "FPCN", &dev->fp_hcnt, &dev->fp_lcnt, &fp_ht);
	i2c_phyt_acpi_params(pdev, "HSCN", &dev->hs_hcnt, &dev->hs_lcnt, &hs_ht);
	i2c_phyt_acpi_params(pdev, "FMCN", &dev->fs_hcnt, &dev->fs_lcnt, &fs_ht);

	switch (t->bus_freq_hz) {
	case FT_I2C_SPEED_100K:
		dev->sda_hold_time = ss_ht;
		break;
	case FT_I2C_SPEED_1000K:
		dev->sda_hold_time = fp_ht;
		break;
	case FT_I2C_SPEED_3400K:
		dev->sda_hold_time = hs_ht;
		break;
	case FT_I2C_SPEED_400K:
	default:
		dev->sda_hold_time = fs_ht;
		break;
	}

	id = acpi_match_device(pdev->dev.driver->acpi_match_table, &pdev->dev);
	if (id && id->driver_data)
		dev->flags |= (u32)id->driver_data;

	return 0;
}

static const struct acpi_device_id i2c_phyt_acpi_match[] = {
	{ "PHYT0059", 0 },
	{ }
};
MODULE_DEVICE_TABLE(acpi, i2c_phyt_acpi_match);
#else
static inline int i2c_phyt_acpi_configure(struct platform_device *pdev)
{
	return -ENODEV;
}
#endif

static void i2c_phyt_configure_master(struct i2c_phyt_dev *dev)
{
	struct i2c_timings *t = &dev->timings;

	dev->functionality = I2C_FUNC_10BIT_ADDR | FT_IC_DEFAULT_FUNCTIONALITY;

	dev->master_cfg = FT_IC_CON_MASTER | FT_IC_CON_SLAVE_DISABLE |
			  FT_IC_CON_RESTART_EN;

	dev->mode = phyt_IC_MASTER;

	switch (t->bus_freq_hz) {
	case FT_I2C_SPEED_100K:
		dev->master_cfg |= FT_IC_CON_SPEED_STD;
		break;
	case FT_I2C_SPEED_3400K:
		dev->master_cfg |= FT_IC_CON_SPEED_HIGH;
		break;
	default:
		dev->master_cfg |= FT_IC_CON_SPEED_FAST;
	}
}

static void i2c_phyt_configure_slave(struct i2c_phyt_dev *dev)
{
	dev->functionality = I2C_FUNC_SLAVE | FT_IC_DEFAULT_FUNCTIONALITY;

	dev->slave_cfg = FT_IC_CON_RX_FIFO_FULL_HLD_CTRL |
			 FT_IC_CON_RESTART_EN | FT_IC_CON_STOP_DET_IFADDRESSED;

	dev->mode = phyt_IC_SLAVE;
}


static void i2c_phyt_timer_handle(struct timer_list *t)
{
	struct i2c_phyt_dev *dev = from_timer(dev, t, timer);

	if (dev->alive_enabled && dev->watchdog)
		dev->watchdog(dev);

	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(2000));
}

static int i2c_phyt_plat_probe(struct platform_device *pdev)
{
	struct i2c_adapter *adap;
	struct i2c_phyt_dev *dev;
	struct i2c_timings *t;
	u32 acpi_speed;
	struct resource *reg_mem, *share_mem;
	int  irq, ret, i;
	static const int supported_speeds[] = {
		0, FT_I2C_SPEED_100K, FT_I2C_SPEED_400K, FT_I2C_SPEED_1000K, FT_I2C_SPEED_3400K
	};


	irq = platform_get_irq(pdev, 0);

	if (irq < 0) {
		dev_err(&pdev->dev, "Err:irq :%d,exit\n", irq);
		return irq;
	}

	dev = devm_kzalloc(&pdev->dev, sizeof(struct i2c_phyt_dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->irq = irq;
	/*find regfile info*/
	reg_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!reg_mem) {
		dev_err(&pdev->dev, "Err:can't find valid regfile mem,exit\n");
		return -ENOMEM;
	}
	/*get regfile base address*/
	dev->base = devm_ioremap_resource(&pdev->dev, reg_mem);
	if (IS_ERR(dev->base)) {
		dev_err(&pdev->dev, "dev->base is err exit\n");
		return PTR_ERR(dev->base);
	}
	/*find share mem info*/
	share_mem = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!share_mem) {
		dev_err(&pdev->dev, "Err:can't find valid shmem,exit\n");
		return -ENOMEM;
	}

	/*set tx share mem start addr*/
	dev->tx_shmem_addr = devm_ioremap_wc(&pdev->dev, share_mem->start,
					     resource_size(share_mem));
	if (IS_ERR(dev->tx_shmem_addr))	{
		dev_err(&pdev->dev, "tx_shmem_addr is err\n");
		return PTR_ERR(dev->tx_shmem_addr);
	}

	/*set rx share mem start addr*/
	dev->mng.tx_ring_cnt = (i2c_phyt_read_reg(dev, FT_I2C_REGFILE_RING) &
			    FT_I2C_REGFILE_TX_RING_MASK) >> FT_I2C_REGFILE_TX_RING_OFFSET;
	if (!dev->mng.tx_ring_cnt || (dev->mng.tx_ring_cnt > 8)) {
		dev_err(&pdev->dev, "failed set tx ring cnt:%d\n", dev->mng.tx_ring_cnt);
		return -EINVAL;
	}
	dev->rx_shmem_addr = dev->tx_shmem_addr +
			    dev->mng.tx_ring_cnt * sizeof(struct phyt_msg_info);

	dev->dev = &pdev->dev;
	platform_set_drvdata(pdev, dev);

	ret = i2c_phyt_malloc_log_mem(dev);
	if (ret)
		return -ENOMEM;

	dev->timer.expires = jiffies + msecs_to_jiffies(3000);
	timer_setup(&dev->timer, i2c_phyt_timer_handle, 0);
	add_timer(&dev->timer);

	dev->watchdog = i2c_phyt_common_watchdog;

	i2c_phyt_disable_debug(dev);
	dev->debug_enabled = false;

	dev->rst = devm_reset_control_get_optional_exclusive(&pdev->dev, NULL);
	if (IS_ERR(dev->rst)) {
		if (PTR_ERR(dev->rst) == -EPROBE_DEFER)	{
			dev_err(&pdev->dev, "dev rst not null\n");
			return -EPROBE_DEFER;
		}
	} else {
		reset_control_deassert(dev->rst);
	}

	t = &dev->timings;
	i2c_parse_fw_timings(&pdev->dev, t, false);

	acpi_speed = i2c_acpi_find_bus_speed(&pdev->dev);

	/*
	 * Some DSTDs use a non standard speed, round down to the lowest
	 * standard speed.
	 */
	for (i = 1; i < ARRAY_SIZE(supported_speeds); i++) {
		if (acpi_speed < supported_speeds[i])
			break;
	}
	acpi_speed = supported_speeds[i - 1];

	/*
	 * Find bus speed from the "clock-frequency" device property, ACPI
	 * or by using fast mode if neither is set.
	 */
	if (acpi_speed && t->bus_freq_hz)
		t->bus_freq_hz = min(t->bus_freq_hz, acpi_speed);
	else if (acpi_speed || t->bus_freq_hz)
		t->bus_freq_hz = max(t->bus_freq_hz, acpi_speed);
	else
		t->bus_freq_hz = FT_I2C_SPEED_400K;

	if (has_acpi_companion(&pdev->dev))
		i2c_phyt_acpi_configure(pdev);

	/*
	 * Only standard mode at 100kHz, fast mode at 400kHz,
	 * fast mode plus at 1MHz and high speed mode at 3.4MHz are supported.
	 */
	if (t->bus_freq_hz != FT_I2C_SPEED_100K && t->bus_freq_hz != FT_I2C_SPEED_400K &&
	    t->bus_freq_hz != FT_I2C_SPEED_1000K && t->bus_freq_hz != FT_I2C_SPEED_3400K) {
		dev_err(&pdev->dev,
			"%d Hz is unsupported, only 100kHz, 400kHz, 1MHz and 3.4MHz are supported\n",
			t->bus_freq_hz);
		ret = -EINVAL;
		goto exit_reset;
	}

	if (i2c_detect_slave_mode(&pdev->dev))
		i2c_phyt_configure_slave(dev);
	else
		i2c_phyt_configure_master(dev);

	dev->clk = devm_clk_get(&pdev->dev, NULL);

	if (!i2c_phyt_prepare_clk(dev, true)) {
		u64 clk_khz;

		dev->get_clk_rate_khz = i2c_phyt_get_clk_rate_khz;
		clk_khz = dev->get_clk_rate_khz(dev);

		if (!dev->sda_hold_time && t->sda_hold_ns)
			dev->sda_hold_time =
				div_u64(clk_khz * t->sda_hold_ns + 500000, 1000000);
	}

	dev->adapter.nr = pdev->id;

	adap = &dev->adapter;
	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_DEPRECATED;
	ACPI_COMPANION_SET(&adap->dev, ACPI_COMPANION(&pdev->dev));
	adap->dev.of_node = pdev->dev.of_node;
	adap->timeout = HZ;
	adap->dev.fwnode = pdev->dev.fwnode;
	dev_pm_set_driver_flags(&pdev->dev,
				DPM_FLAG_SMART_PREPARE |
				DPM_FLAG_SMART_SUSPEND |
				DPM_FLAG_MAY_SKIP_RESUME);

	/* The code below assumes runtime PM to be disabled. */
	WARN_ON(pm_runtime_enabled(&pdev->dev));

	pm_runtime_set_autosuspend_delay(&pdev->dev, 1000);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_set_active(&pdev->dev);

	pm_runtime_enable(&pdev->dev);

	if (dev->mode == phyt_IC_SLAVE)
		ret = i2c_phyt_slave_probe(dev);
	else
		ret = i2c_phyt_master_probe(dev);

	if (ret)
		goto exit_probe;

	if (sysfs_create_group(&dev->dev->kobj, &i2c_ft_device_group))
		dev_warn(&pdev->dev, "failed create sysfs\n");

	i2c_phyt_enable_alive(dev);
	dev->alive_enabled = true;

	return ret;

exit_probe:
	pm_runtime_disable(dev->dev);
exit_reset:
	del_timer(&dev->timer);
	if (!IS_ERR_OR_NULL(dev->rst))
		reset_control_assert(dev->rst);
	return ret;
}

static int i2c_phyt_plat_remove(struct platform_device *pdev)
{
	struct i2c_phyt_dev *dev = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "i2c_2.0 remove\n");
	pm_runtime_get_sync(&pdev->dev);

	i2c_del_adapter(&dev->adapter);
	sysfs_remove_group(&dev->dev->kobj, &i2c_ft_device_group);
	dev->disable(dev);
	/*disable alive function*/
	i2c_phyt_disable_alive(dev);
	del_timer(&dev->timer);

	pm_runtime_dont_use_autosuspend(&pdev->dev);
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(dev->dev);

	if (!IS_ERR_OR_NULL(dev->rst))
		reset_control_assert(dev->rst);

	i2c_phyt_common_regfile_disable_int(dev);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id i2c_phyt_of_match[] = {
	{ .compatible = "phytium,i2c-2.0", },
	{},
};
MODULE_DEVICE_TABLE(of, i2c_phyt_of_match);
#endif

static int __maybe_unused i2c_phyt_plat_suspend(struct device *dev)
{
	struct i2c_phyt_dev *idev = dev_get_drvdata(dev);

	idev->disable(idev);

	i2c_phyt_disable_alive(idev);
	idev->alive_enabled = false;

	i2c_phyt_prepare_clk(idev, false);

	return 0;
}

static int __maybe_unused i2c_phyt_plat_resume(struct device *dev)
{
	struct i2c_phyt_dev *idev = dev_get_drvdata(dev);

	i2c_phyt_prepare_clk(idev, true);
	idev->init(idev);

	i2c_phyt_enable_alive(idev);
	idev->alive_enabled = true;

	return 0;
}

static const struct dev_pm_ops i2c_phyt_dev_pm_ops = {
	SET_LATE_SYSTEM_SLEEP_PM_OPS(i2c_phyt_plat_suspend,
				     i2c_phyt_plat_resume)
	SET_RUNTIME_PM_OPS(i2c_phyt_plat_suspend,
			   i2c_phyt_plat_resume, NULL)
};

static struct platform_driver i2c_phyt_driver = {
	.probe = i2c_phyt_plat_probe,
	.remove = i2c_phyt_plat_remove,
	.driver = {
		.name = DRV_NAME,
		.of_match_table = of_match_ptr(i2c_phyt_of_match),
		.acpi_match_table = ACPI_PTR(i2c_phyt_acpi_match),
		.pm = &i2c_phyt_dev_pm_ops,
	},
};
module_platform_driver(i2c_phyt_driver);

MODULE_ALIAS("platform:i2c-2.0");
MODULE_AUTHOR("Wu Jinyong <wujinyong1788@phytium.com.cn>");
MODULE_DESCRIPTION("Phytium I2C bus adapter");
MODULE_LICENSE("GPL");
MODULE_VERSION(I2C_PHYTIUM_V2_DRV_VERSION);

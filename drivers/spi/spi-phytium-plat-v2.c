// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium SPI core controller platform driver.
 *
 * Copyright (c) 2023-2024, Phytium Technology Co., Ltd.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/highmem.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/scatterlist.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/property.h>
#include <linux/acpi.h>

#include "spi-phytium.h"

#define DRIVER_NAME_PHYT "phytium_spi_2.0"
#define DRIVER_VERSION	"1.0.6"

#define PHYTIUM_CPU_PART_FTC872		0x872

#define MIDR_PHYTIUM_FTC872 MIDR_CPU_MODEL(ARM_CPU_IMP_PHYTIUM, PHYTIUM_CPU_PART_FTC872)

static ssize_t debug_show(struct device *dev,
		struct device_attribute *da,
		char *buf)
{
	struct phytium_spi *fts = dev_get_drvdata(dev);
	ssize_t ret;
	u32 reg;

	reg = phytium_read_regfile(fts, SPI_REGFILE_DEBUG);
	ret = sprintf(buf, "%x\n", reg);

	return ret;
}

static ssize_t debug_store(struct device *dev,
		struct device_attribute *da, const char *buf,
		size_t size)
{
	u8 loc, dis_en, status = 0;
	char *p;
	char *token;
	long value;
	u32 reg;
	struct phytium_spi *fts = dev_get_drvdata(dev);

	dev_info(dev, "echo alive(1)/debug(0) enable(1)/disable(0) > debug\n");
	dev_info(dev, "Example:echo 0 1 > debug; Enable Debug Function\n");

	p = kmalloc(size, GFP_KERNEL);
	if (!p)
		return -ENOMEM;
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

	reg = phytium_read_regfile(fts, SPI_REGFILE_DEBUG);

	if (loc == 1) {
		if (dis_en == 1) {
			fts->alive_enabled = true;
			reg |= BIT(loc);
		} else if (dis_en == 0) {
			fts->alive_enabled = false;
			reg &= ~BIT(loc);
		}
		fts->runtimes = 0;
	} else if (loc == 0) {
		if (dis_en == 1) {
			fts->debug_enabled = true;
			reg |= BIT(loc);
		} else if (dis_en == 0) {
			fts->debug_enabled = false;
			reg &= ~BIT(loc);
		}
	}

	phytium_write_regfile(fts, SPI_REGFILE_DEBUG, reg);

	kfree(p);

	return size;
}

static DEVICE_ATTR_RW(debug);

static struct attribute *spi_phyt_device_attrs[] = {
	&dev_attr_debug.attr,
	NULL,
};

static const struct attribute_group spi_phyt_device_group = {
	.attrs = spi_phyt_device_attrs,
};

static int spi_phyt_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct phytium_spi *fts;
	struct resource *regfile_mem, *share_mem;
	int ret;
	int num_cs;
	int cs_gpio;
	int global_cs = 0;
	int i;
	u32 clk_rate = SPI_DEFAULT_CLK;

	fts = devm_kzalloc(&pdev->dev, sizeof(struct phytium_spi),
			GFP_KERNEL);
	if (!fts)
		return -ENOMEM;

	regfile_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regfile_mem) {
		dev_err(&pdev->dev, "no regfile_mem resource?\n");
		return -EINVAL;
	}

	fts->regfile = devm_ioremap_resource(&pdev->dev, regfile_mem);
	if (IS_ERR(fts->regfile)) {
		dev_err(&pdev->dev, "fts->regfile map failed\n");
		return PTR_ERR(fts->regfile);
	}

	share_mem = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!share_mem) {
		dev_err(&pdev->dev, "no share_mem resource?\n");
		return -EINVAL;
	}

	fts->mem_tx_physic = (u32)share_mem->start + sizeof(struct msg);
	fts->mem_rx_physic = (u32)share_mem->start + sizeof(struct msg);

	fts->tx_shmem_addr = devm_ioremap_resource(&pdev->dev, share_mem);
	if (IS_ERR(fts->tx_shmem_addr)) {
		dev_err(&pdev->dev, "fts->tx_shmem_addr map failed\n");
		return PTR_ERR(fts->tx_shmem_addr);
	}

	fts->msg = (struct msg *)fts->tx_shmem_addr;

	fts->mem_tx = (u64)fts->msg + sizeof(struct msg);
	fts->mem_rx = (u64)fts->msg + sizeof(struct msg);

	fts->irq = platform_get_irq(pdev, 0);
	if (fts->irq < 0) {
		dev_err(&pdev->dev, "no irq resource?\n");
		return fts->irq; /* -ENXIO */
	}

	if (pdev->dev.of_node) {
		fts->clk = devm_clk_get(&pdev->dev, NULL);

		if (IS_ERR(fts->clk))
			return PTR_ERR(fts->clk);
		ret = clk_prepare_enable(fts->clk);
		if (ret)
			return ret;

		fts->max_freq = clk_get_rate(fts->clk);
	} else if (has_acpi_companion(&pdev->dev)) {
		fwnode_property_read_u32(dev->fwnode, "spi-clock", &clk_rate);
		fts->max_freq = clk_rate;
	}

	fts->bus_num = pdev->id;
	device_property_read_u32(&pdev->dev, "reg-io-width", &fts->reg_io_width);

	num_cs = 4;

	device_property_read_u32(&pdev->dev, "num-cs", &num_cs);

	fts->num_cs = num_cs;

	if (pdev->dev.of_node) {
		int i;

		for (i = 0; i < fts->num_cs; i++) {
			cs_gpio = of_get_named_gpio(pdev->dev.of_node,
					"cs-gpios", i);

			if (cs_gpio == -EPROBE_DEFER) {
				ret = cs_gpio;
				goto out;
			}

			if (gpio_is_valid(cs_gpio)) {
				ret = devm_gpio_request(&pdev->dev, cs_gpio,
						dev_name(&pdev->dev));
				if (ret)
					goto out;
			}
		}
	} else if (has_acpi_companion(&pdev->dev)) {
		int n;
		int *cs;
		struct gpio_desc *gpiod;

		n =  gpiod_count(&pdev->dev, "cs");

		cs = devm_kcalloc(&pdev->dev, n, sizeof(int), GFP_KERNEL);
		fts->cs = cs;

		for (i = 0; i < n; i++) {
			gpiod = devm_gpiod_get_index_optional(&pdev->dev, "cs", i,
							      GPIOD_OUT_LOW);

			if (IS_ERR(gpiod)) {
				ret = PTR_ERR(gpiod);
				goto out;
			}

			cs_gpio = desc_to_gpio(gpiod);
			cs[i] = cs_gpio;
		}
	}

	device_property_read_u32(&pdev->dev, "global-cs", &global_cs);
	fts->global_cs = global_cs;

	fts->dma_get_ddrdata = false;
	if ((read_cpuid_id() & MIDR_CPU_MODEL_MASK) == MIDR_PHYTIUM_FTC872)
		fts->dma_get_ddrdata = true;

	ret = spi_phyt_add_host(&pdev->dev, fts);
	if (ret)
		goto out;

	platform_set_drvdata(pdev, fts);

	if (sysfs_create_group(&pdev->dev.kobj, &spi_phyt_device_group))
		dev_warn(&pdev->dev, "failed create sysfs\n");

	return 0;

out:
	clk_disable_unprepare(fts->clk);
	return ret;
}

static int spi_phyt_remove(struct platform_device *pdev)
{
	struct phytium_spi *fts = platform_get_drvdata(pdev);

	spi_phyt_remove_host(fts);
	sysfs_remove_group(&pdev->dev.kobj, &spi_phyt_device_group);
	clk_disable_unprepare(fts->clk);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int spi_phyt_suspend(struct device *dev)
{
	struct phytium_spi *fts = dev_get_drvdata(dev);

	return spi_phyt_suspend_host(fts);
}

static int spi_phyt_resume(struct device *dev)
{
	struct phytium_spi *fts = dev_get_drvdata(dev);

	return spi_phyt_resume_host(fts);
}
#endif

static SIMPLE_DEV_PM_OPS(spi_phyt_pm_ops, spi_phyt_suspend, spi_phyt_resume);

static const struct of_device_id spi_phyt_of_match[] = {
	{ .compatible = "phytium,spi-2.0", .data = (void *)0 },
	{ /* end of table */}
};
MODULE_DEVICE_TABLE(of, spi_phyt_of_match);

static const struct acpi_device_id spi_phyt_acpi_match[] = {
	{"PHYT0060", 0},
	{}
};
MODULE_DEVICE_TABLE(acpi, spi_phyt_acpi_match);

static struct platform_driver spi_phyt_driver = {
	.probe		= spi_phyt_probe,
	.remove		= spi_phyt_remove,
	.driver		= {
		.name	= DRIVER_NAME_PHYT,
		.of_match_table = of_match_ptr(spi_phyt_of_match),
		.acpi_match_table = ACPI_PTR(spi_phyt_acpi_match),
		.pm = &spi_phyt_pm_ops,
	},
};
module_platform_driver(spi_phyt_driver);

MODULE_AUTHOR("Peng Min <pengmin1540@phytium.com.cn>");
MODULE_DESCRIPTION("Platform Driver for Phytium SPI controller core");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);

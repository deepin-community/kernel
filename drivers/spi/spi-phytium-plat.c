// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium SPI core controller platform driver.
 *
 * Copyright (c) 2019-2023, Phytium Technology Co., Ltd.
 *
 * Derived from drivers/spi/spi-dw-mmio.c
 *   Copyright (c) 2010, Octasic semiconductor.
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

#define DRIVER_NAME "phytium_spi"

struct phytium_spi_clk {
	struct phytium_spi  fts;
	struct clk     *clk;
};

static int phytium_spi_probe(struct platform_device *pdev)
{
	struct phytium_spi_clk *ftsc;
	struct phytium_spi *fts;
	struct resource *mem;
	int ret;
	int num_cs;
	int global_cs;

	ftsc = devm_kzalloc(&pdev->dev, sizeof(struct phytium_spi_clk),
			GFP_KERNEL);
	if (!ftsc)
		return -ENOMEM;

	fts = &ftsc->fts;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource?\n");
		return -EINVAL;
	}

	fts->regs = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(fts->regs)) {
		dev_err(&pdev->dev, "SPI region map failed\n");
		return PTR_ERR(fts->regs);
	}

	fts->irq = platform_get_irq(pdev, 0);
	if (fts->irq < 0) {
		dev_err(&pdev->dev, "no irq resource?\n");
		return fts->irq; /* -ENXIO */
	}

	if (pdev->dev.of_node) {
		ftsc->clk = devm_clk_get(&pdev->dev, NULL);

		if (IS_ERR(ftsc->clk))
			return PTR_ERR(ftsc->clk);
		ret = clk_prepare_enable(ftsc->clk);
		if (ret)
			return ret;

		fts->max_freq = clk_get_rate(ftsc->clk);
	} else if (has_acpi_companion(&pdev->dev)) {
		fts->max_freq = 48000000;
	}

	fts->bus_num = pdev->id;
	device_property_read_u32(&pdev->dev, "reg-io-width", &fts->reg_io_width);

	num_cs = 4;

	device_property_read_u32(&pdev->dev, "num-cs", &num_cs);

	fts->num_cs = num_cs;

	device_property_read_u32(&pdev->dev, "global-cs", &global_cs);
	fts->global_cs = global_cs;

	ret = phytium_spi_add_host(&pdev->dev, fts);
	if (ret)
		goto out;

	platform_set_drvdata(pdev, ftsc);
	return 0;

out:
	clk_disable_unprepare(ftsc->clk);
	return ret;
}

static int phytium_spi_remove(struct platform_device *pdev)
{
	struct phytium_spi_clk *ftsc = platform_get_drvdata(pdev);

	phytium_spi_remove_host(&ftsc->fts);
	clk_disable_unprepare(ftsc->clk);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int spi_suspend(struct device *dev)
{
	struct phytium_spi_clk *ftsc = dev_get_drvdata(dev);

	return phytium_spi_suspend_host(&ftsc->fts);
}

static int spi_resume(struct device *dev)
{
	struct phytium_spi_clk *ftsc = dev_get_drvdata(dev);

	return phytium_spi_resume_host(&ftsc->fts);
}
#endif

static SIMPLE_DEV_PM_OPS(phytium_spi_pm_ops, spi_suspend, spi_resume);

static const struct of_device_id phytium_spi_of_match[] = {
	{ .compatible = "phytium,spi", .data = (void *)0 },
	{ /* end of table */}
};
MODULE_DEVICE_TABLE(of, phytium_spi_of_match);

static const struct acpi_device_id phytium_spi_acpi_match[] = {
	{"PHYT000E", 0},
	{}
};
MODULE_DEVICE_TABLE(acpi, phytium_spi_acpi_match);

static struct platform_driver phytium_spi_driver = {
	.probe		= phytium_spi_probe,
	.remove		= phytium_spi_remove,
	.driver		= {
		.name	= DRIVER_NAME,
		.of_match_table = of_match_ptr(phytium_spi_of_match),
		.acpi_match_table = ACPI_PTR(phytium_spi_acpi_match),
		.pm = &phytium_spi_pm_ops,
	},
};
module_platform_driver(phytium_spi_driver);

MODULE_AUTHOR("Yiqun Zhang <zhangyiqun@phytium.com.cn>");
MODULE_DESCRIPTION("Platform Driver for Phytium SPI controller core");
MODULE_LICENSE("GPL");

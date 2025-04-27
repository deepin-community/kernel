// SPDX-License-Identifier: GPL-2.0+
/*
 * Phytium Multimedia Card Interface PCI driver
 *
 * Copyright (C) 2024 Phytium Technology Co.,Ltd.
 */

#include <linux/module.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/acpi.h>
#include <linux/dma-mapping.h>
#include "phytium-mci-v2.h"

static u32 mci_caps = MMC_CAP_CMD23;

#if defined CONFIG_PM && defined CONFIG_PM_SLEEP

static const struct dev_pm_ops phytium_mci_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(phyt_mci_suspend,
				phyt_mci_resume)
	SET_RUNTIME_PM_OPS(phyt_mci_runtime_suspend,
			   phyt_mci_runtime_resume, NULL)
};
#else
#define phytium_mci_dev_pm_ops NULL
#endif

static ssize_t debug_enable_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct mmc_host *mmc = container_of(dev, struct mmc_host, class_dev);
	struct phytium_mci_host *host = mmc_priv(mmc);

	return sprintf(buf, "%d\n", host->debug_enable);
}

static ssize_t debug_enable_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct mmc_host *mmc = container_of(dev, struct mmc_host, class_dev);
	struct phytium_mci_host *host = mmc_priv(mmc);
	int enable;
	int ret;

	ret = sscanf(buf, "%d\n", &enable);
	if (ret == 0) {
		ret = -EINVAL;
		return ret;
	}

	host->debug_enable = enable;

	phytium_mci_set_debug_enable(host, host->debug_enable);

	return count;
}

static ssize_t alive_enable_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct mmc_host *mmc = container_of(dev, struct mmc_host, class_dev);
	struct phytium_mci_host *host = mmc_priv(mmc);

	return sprintf(buf, "%d\n", host->alive_enable);
}

static ssize_t alive_enable_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct mmc_host *mmc = container_of(dev, struct mmc_host, class_dev);
	struct phytium_mci_host *host = mmc_priv(mmc);
	int enable;
	int ret;

	ret = sscanf(buf, "%d\n", &enable);
	if (ret == 0) {
		ret = -EINVAL;
		return ret;
	}

	host->alive_enable = enable;

	phytium_mci_set_alive_enable(host, host->alive_enable);

	return count;
}

static DEVICE_ATTR_RW(debug_enable);
static DEVICE_ATTR_RW(alive_enable);

static int phytium_mci_probe(struct platform_device *pdev)
{
	struct mmc_host *mmc;
	struct phytium_mci_host *host;
	struct resource *res;
	const struct acpi_device_id *match;
	struct device *dev = &pdev->dev;
	int ret;

	mmc = mmc_alloc_host(sizeof(struct phytium_mci_host), &pdev->dev);
	if (!mmc)
		return -ENOMEM;
	host = mmc_priv(mmc);
	ret = mmc_of_parse(mmc);
	if (ret)
		goto host_free;

	if (dev->of_node) {
		host->src_clk = devm_clk_get(&pdev->dev, "phytium_mci_clk");
		if (IS_ERR(host->src_clk)) {
			ret = PTR_ERR(host->src_clk);
			goto host_free;
		}

		host->clk_rate = clk_get_rate(host->src_clk);
	} else if (has_acpi_companion(dev)) {
		match = acpi_match_device(dev->driver->acpi_match_table, dev);
		if (!match) {
			dev_err(dev, "Error ACPI match data is missing\n");
			return -ENODEV;
		}
		host->clk_rate = 1200000000;
	}

	host->is_use_dma = 1;
	host->is_device_x100 = 0;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	host->regf_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(host->regf_base)) {
		ret = PTR_ERR(host->regf_base);
		goto host_free;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	host->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(host->base)) {
		ret = PTR_ERR(host->base);
		goto host_free;
	}

	host->irq = platform_get_irq(pdev, 0);

	if (host->irq < 0) {
		ret = -EINVAL;
		goto host_free;
	}
	host->irq_flags = IRQF_SHARED;
	dev_dbg(&pdev->dev, "%s %d:irq:%d\n", __func__, __LINE__, host->irq);
	host->dev = &pdev->dev;
	host->caps = mci_caps;
	host->mmc = mmc;
	ret = phyt_mci_common_probe(host);
	if (ret == MCI_REALEASE_MEM) {
		ret = -ENOMEM;
		goto release_mem;
	} else if (ret) {
		goto release;
	}

	ret = device_create_file(&mmc->class_dev,
				&dev_attr_debug_enable);
	if (ret < 0)
		goto debug_enable_free;

	ret = device_create_file(&mmc->class_dev,
				&dev_attr_alive_enable);
	if (ret < 0)
		goto alive_enable_free;

	platform_set_drvdata(pdev, mmc);
	dev_info(&pdev->dev, "%s %d: probe phytium mci successful.\n", __func__, __LINE__);
	return 0;

alive_enable_free:
	device_remove_file(&mmc->class_dev, &dev_attr_alive_enable);
debug_enable_free:
	device_remove_file(&mmc->class_dev, &dev_attr_debug_enable);
release:
	phyt_mci_deinit_hw(host);
release_mem:
	if (host->dma.adma_table) {
		dma_free_coherent(&pdev->dev,
				  MAX_BD_NUM * sizeof(struct phytium_adma2_64_desc),
				  host->dma.adma_table, host->dma.adma_addr);
	}
host_free:
	mmc_free_host(mmc);
	return ret;
}

static int phytium_mci_remove(struct platform_device *pdev)
{
	struct mmc_host *mmc;
	struct phytium_mci_host *host;

	mmc = platform_get_drvdata(pdev);
	if (!mmc) {
		dev_info(&pdev->dev, "%s %d: mmc is null.\n", __func__, __LINE__);
		return -1;
	}
	host = mmc_priv(mmc);
	if (!host) {
		dev_info(&pdev->dev, "%s %d: host is null.\n", __func__, __LINE__);
		mmc_remove_host(mmc);
		mmc_free_host(mmc);
		return -1;
	}
	del_timer(&host->alive_timer);
	del_timer(&host->hotplug_timer);
	mmc_remove_host(host->mmc);

	if (host->dma.adma_table) {
		dma_free_coherent(&pdev->dev,
				  MAX_BD_NUM * sizeof(struct phytium_adma2_64_desc),
				  host->dma.adma_table, host->dma.adma_addr);
	}
	phyt_mci_deinit_hw(host);
	mmc_free_host(mmc);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static const struct of_device_id phytium_mci_of_ids[] = {
	{   .compatible =  "phytium,mci_2.0", },
	{}
};

MODULE_DEVICE_TABLE(of, phytium_mci_of_ids);

#ifdef CONFIG_ACPI
static const struct acpi_device_id phytium_mci_acpi_ids[] = {
	{ .id = "PHYT0061" },
	{ }
};

MODULE_DEVICE_TABLE(acpi, phytium_mci_acpi_ids);
#else
#define phytium_mci_acpi_ids NULL
#endif

static struct platform_driver phytium_mci_driver = {
	.probe = phytium_mci_probe,
	.remove = phytium_mci_remove,
	.driver = {
		.name = "phytium-mci-platform-2.0",
		.of_match_table = phytium_mci_of_ids,
		.acpi_match_table = phytium_mci_acpi_ids,
		.pm = &phytium_mci_dev_pm_ops,
	},
};

module_platform_driver(phytium_mci_driver);

MODULE_DESCRIPTION("Phytium Multimedia Card Interface driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lai Xueyu <laixueyu1280@phytium.com.cn>");

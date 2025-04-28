// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium USB DRD Driver.
 *
 * Copyright (C) 2023 - 2024 Phytium.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/irq.h>
#include <linux/acpi.h>

#include "core.h"

static int phytium_usbplat_probe(struct platform_device *pdev)
{
	struct phytium_usb *phytium_usb;
	struct device *dev = &pdev->dev;
	int irq;
	struct resource *res;
	int ret;

	phytium_usb = devm_kzalloc(dev, sizeof(*phytium_usb), GFP_KERNEL);
	if (!phytium_usb)
		return -ENOMEM;

	phytium_usb->dev = dev;
	phytium_usb->platdata = dev_get_platdata(dev);

	irq = platform_get_irq(pdev, 1);
	if (irq < 0)
		return -EINVAL;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_warn(dev, "get otg resource failed\n");
		return -ENXIO;
	}
	phytium_usb->otg_res = *res;
	phytium_usb->otg_irq = irq;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return -EINVAL;

	phytium_usb->host_res[0].start = irq;
	phytium_usb->host_res[0].end = irq;
	phytium_usb->host_res[0].flags = IORESOURCE_IRQ | irq_get_trigger_type(irq);
	phytium_usb->host_res[0].name = "host";

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (!res) {
		dev_warn(dev, "get host resource failed\n");
		return -ENXIO;
	}

	phytium_usb->host_res[1] = *res;
	phytium_usb->device_irq = irq;
	phytium_usb->device_regs = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(phytium_usb->device_regs))
		return PTR_ERR(phytium_usb->device_regs);

	ret = phytium_usb_init(phytium_usb);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, phytium_usb);

	return 0;
}

static int phytium_usbplat_remove(struct platform_device *pdev)
{
	struct phytium_usb *phytium_usb = platform_get_drvdata(pdev);

	if (phytium_usb)
		phytium_usb_uninit(phytium_usb);

	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int phytium_usbplat_suspend(struct device *dev)
{
	struct phytium_usb *phytium_usb = dev_get_drvdata(dev);

	return  phytium_usb_suspend(phytium_usb);
}

static int phytium_usbplat_resume(struct device *dev)
{
	struct phytium_usb *phytium_usb = dev_get_drvdata(dev);
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&phytium_usb->lock, flags);
	ret = phytium_usb_resume(phytium_usb, 1);
	spin_unlock_irqrestore(&phytium_usb->lock, flags);

	return ret;
}
#endif

static const struct dev_pm_ops phytium_usbpm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(phytium_usbplat_suspend, phytium_usbplat_resume)
};

#ifdef CONFIG_OF
static const struct of_device_id of_phytium_usbmatch[] = {
	{ .compatible = "phytium,usb2-2.0" },
	{ }
};
MODULE_DEVICE_TABLE(of, of_phytium_usbmatch);
#endif

static const struct acpi_device_id acpi_phytium_usbmatch[] = {
	{ "PHYT0064", },
	{ }
};
MODULE_DEVICE_TABLE(acpi, acpi_phytium_usbmatch);

static struct platform_driver phytium_usbdriver = {
	.probe	= phytium_usbplat_probe,
	.remove	= phytium_usbplat_remove,
	.driver	= {
		.name	= "phytium-usb2-2.0",
		.of_match_table	= of_match_ptr(of_phytium_usbmatch),
		.acpi_match_table = ACPI_PTR(acpi_phytium_usbmatch),
		.pm	= &phytium_usbpm_ops,
	},
};

module_platform_driver(phytium_usbdriver);

MODULE_AUTHOR("Zhenhua Chen <chenzhenhua@phytium.com.cn>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Phytium USB2 DRD Controller Driver");
MODULE_VERSION(PHYTIUM_USB_DRIVER_V2_VERSION);

// SPDX-License-Identifier: GPL-2.0
/*
 * base_ctrl driver for Phytium.
 *
 * Copyright (C) 2021-2024, Phytium Technology Co., Ltd.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/spinlock.h>
#include <linux/acpi.h>

#include "phytium_base_ctrl.h"

#define BASE_CTRL_DRIVER_VERSION "1.1.0"

static struct phytium_base_ctrl *boot_base_ctrl;

int phytium_base_ctrl_irq(void)
{
	if (!boot_base_ctrl)
		return -ENODEV;

	return boot_base_ctrl->irq;
}
EXPORT_SYMBOL(phytium_base_ctrl_irq);

int base_ctrl_read_int_status(void)
{
	unsigned long flags;
	u16 value;

	if (!boot_base_ctrl)
		return 0;

	spin_lock_irqsave(&boot_base_ctrl->lock, flags);
	value = readw(boot_base_ctrl->base + boot_base_ctrl->int_status_reg);
	spin_unlock_irqrestore(&boot_base_ctrl->lock, flags);

	return value;
}
EXPORT_SYMBOL(base_ctrl_read_int_status);

void base_ctrl_write_int_clear(int val)
{
	unsigned long flags;

	if (!boot_base_ctrl)
		return;

	spin_lock_irqsave(&boot_base_ctrl->lock, flags);
	writew(val, boot_base_ctrl->base + boot_base_ctrl->int_clear_reg);
	spin_unlock_irqrestore(&boot_base_ctrl->lock, flags);
}
EXPORT_SYMBOL(base_ctrl_write_int_clear);

bool phytium_check_cpu(void)
{
#ifdef CONFIG_ARCH_PHYTIUM
	if (read_cpuid_implementor() == ARM_CPU_IMP_PHYTIUM)
		return true;
#endif
	return false;
}
EXPORT_SYMBOL(phytium_check_cpu);

u8 base_ctrl_readb(unsigned long offset)
{
	unsigned long flags;
	u8 value;

	if (!boot_base_ctrl)
		return 1;

	spin_lock_irqsave(&boot_base_ctrl->lock, flags);
	value = readb(boot_base_ctrl->base + offset);
	spin_unlock_irqrestore(&boot_base_ctrl->lock, flags);
	return value;
}
EXPORT_SYMBOL(base_ctrl_readb);

u32 base_ctrl_readl(unsigned long offset)
{
	unsigned long flags;
	u32 value;

	if (!boot_base_ctrl)
		return 1;

	spin_lock_irqsave(&boot_base_ctrl->lock, flags);
	value = readl(boot_base_ctrl->base + offset);
	spin_unlock_irqrestore(&boot_base_ctrl->lock, flags);

	return value;
}
EXPORT_SYMBOL(base_ctrl_readl);

int base_ctrl_writeb(unsigned long offset, u8 value)
{
	unsigned long flags;

	if (!boot_base_ctrl)
		return 0;

	spin_lock_irqsave(&boot_base_ctrl->lock, flags);
	writeb(value, boot_base_ctrl->base + offset);
	spin_unlock_irqrestore(&boot_base_ctrl->lock, flags);

	return 0;
}
EXPORT_SYMBOL(base_ctrl_writeb);

int base_ctrl_writel(unsigned long offset, u32 value)
{
	unsigned long flags;

	if (!boot_base_ctrl)
		return 0;

	spin_lock_irqsave(&boot_base_ctrl->lock, flags);
	writel(value, boot_base_ctrl->base + offset);
	spin_unlock_irqrestore(&boot_base_ctrl->lock, flags);

	return 0;
}
EXPORT_SYMBOL(base_ctrl_writel);

static int phytium_base_ctrl_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct phytium_base_ctrl *base_ctrl;
	int error = -1;

	base_ctrl = devm_kzalloc(dev, sizeof(*base_ctrl), GFP_KERNEL);
	if (!base_ctrl)
		return -ENOMEM;

	base_ctrl->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENOENT;

	base_ctrl->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base_ctrl->base)) {
		dev_err(&pdev->dev, "region map failed\n");
		return PTR_ERR(base_ctrl->base);
	}

	base_ctrl->irq = platform_get_irq(pdev, 0);
	if (base_ctrl->irq < 0) {
		dev_err(&pdev->dev, "no irq resource?\n");
		return base_ctrl->irq;
	}

	error = device_property_read_u32(&pdev->dev, "int_state",
			&base_ctrl->int_status_reg);
	if (error)
		base_ctrl->int_status_reg = base_ctrl_INT_STATE;

	error = device_property_read_u32(&pdev->dev, "clr_int",
				&base_ctrl->int_clear_reg);
	if (error)
		base_ctrl->int_clear_reg = base_ctrl_CLR_INT;

	spin_lock_init(&base_ctrl->lock);
	boot_base_ctrl = base_ctrl;
	platform_set_drvdata(pdev, base_ctrl);

	return 0;
}

static const struct acpi_device_id base_ctrl_acpi_match[] = {
	{ "PHYT0007", 0 },
	{ }
};
MODULE_DEVICE_TABLE(acpi, base_ctrl_acpi_match);

static struct platform_driver phytium_base_ctrl_driver = {
	.probe		= phytium_base_ctrl_probe,
	.driver	= {
		.name	= "phytium_base_ctrl",
		.acpi_match_table = ACPI_PTR(base_ctrl_acpi_match),
	},
};

module_platform_driver(phytium_base_ctrl_driver);

static int __init phytium_base_ctrl_init(void)
{
	platform_driver_register(&phytium_base_ctrl_driver);
	return 0;
}

static void __exit phytium_base_ctrl_exit(void)
{
	platform_driver_unregister(&phytium_base_ctrl_driver);
}

early_initcall(phytium_base_ctrl_init);
MODULE_AUTHOR("Li Yuze <liyuze@phytium.com.cn>");
MODULE_DESCRIPTION("Phytium base_ctrl driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(BASE_CTRL_DRIVER_VERSION);

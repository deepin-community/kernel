// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Phytium PIO driver
 *
 * Copyright (C) 2024 Phytium Technology Co., Ltd. All Rights Reserved.
 */

#include <linux/acpi.h>
#include <linux/cputypes.h>
#include <linux/io.h>
#include <linux/logic_pio.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include "phytium_pio.h"

#define PHYT_PIO_DRIVER_NAME	"phytium-pio"
#define PHYT_PIO_DRV_VER	"1.1.0"

static struct phytium_pio *ppio;
int phytium_pio_int_state;
int phytium_pio_clr_int;

bool check_cpu_type(void)
{
	/* Only support Phytium CPU except ft2004 */
	if (cpu_is_ft2004()) {
		pr_notice_once("PHYT_PIO %s cannot support ft2004\n", PHYT_PIO_DRV_VER);
		return false;
	}
	if (read_cpuid_implementor() == ARM_CPU_IMP_PHYTIUM)
		return true;
	return false;
}
EXPORT_SYMBOL(check_cpu_type);

void phytium_pio_clear_interrupt(u32 val)
{
	unsigned long flags;

	if (!ppio)
		return;
	spin_lock_irqsave(&ppio->lock, flags);
	writel(val, ppio->membase + phytium_pio_clr_int);
	spin_unlock_irqrestore(&ppio->lock, flags);
}
EXPORT_SYMBOL(phytium_pio_clear_interrupt);

u32 phytium_pio_get_int_status(void)
{
	unsigned long flags;
	u32 ret = 0;

	if (!ppio)
		return ret;
	spin_lock_irqsave(&ppio->lock, flags);
	ret = readl(ppio->membase + phytium_pio_int_state);
	spin_unlock_irqrestore(&ppio->lock, flags);

	return ret;
}
EXPORT_SYMBOL(phytium_pio_get_int_status);

int phytium_pio_get_irq(void)
{
	if (!ppio)
		return -EINVAL;
	return ppio->irq;
}
EXPORT_SYMBOL(phytium_pio_get_irq);

/*
 * phytium_pio_in - input the data from the target I/O port
 * @hostdata: pointer to the device information relevant to PIO controller
 * @addr: the target I/O port address
 * @dwidth: the data width required reading from the target I/O port
 *
 * Return the data read back on success, -ERRNO otherwise.
 */
static u32 phytium_pio_in(void *hostdata, unsigned long addr, size_t dwidth)
{
	struct phytium_pio *pio = hostdata;
	struct logic_pio_hwaddr *range = pio->range;
	__le32 rd_data = 0;
	unsigned long flags;

	spin_lock_irqsave(&pio->lock, flags);
	switch (dwidth) {
	case 1:
		rd_data = readb(pio->membase + addr - range->io_start);
		break;
	case 2:
		rd_data = readw(pio->membase + addr - range->io_start);
		break;
	case 4:
		rd_data = readl(pio->membase + addr - range->io_start);
		break;
	default:
		break;
	}
	spin_unlock_irqrestore(&pio->lock, flags);
	return le32_to_cpu(rd_data);
}

/*
 * phytium_pio_out - output the data to the target I/O port
 * @hostdata: pointer to the device information relevant to PIO controller
 * @addr: the target I/O port address
 * @val: the data to be output
 * @dwidth: the data width required writing to the target I/O port
 */
static void phytium_pio_out(void *hostdata, unsigned long addr, u32 val,
		size_t dwidth)
{
	struct phytium_pio *pio = hostdata;
	struct logic_pio_hwaddr *range = pio->range;
	unsigned long flags;

	spin_lock_irqsave(&pio->lock, flags);
	switch (dwidth) {
	case 1:
		writeb(val, pio->membase + addr - range->io_start);
		break;
	case 2:
		writew(val, pio->membase + addr - range->io_start);
		break;
	case 4:
		writel(val, pio->membase + addr - range->io_start);
		break;
	default:
		break;
	}
	spin_unlock_irqrestore(&pio->lock, flags);
}

/*
 * phytium_pio_ins - input the data in the buffer in multiple operations
 * @hostdata: pointer to the device information relevant to PIO controller
 * @pio: the target I/O port address
 * @buffer: the buffer to store the data
 * @dwidth: the data width required reading from the target I/O port
 * @count: the number of data to be read
 *
 * When success, the data read back is stored in buffer pointed by buffer.
 * Return 0 on success, -ERRNO otherwise.
 */
static u32 phytium_pio_ins(void *hostdata, unsigned long addr, void *buffer,
		size_t dwidth, unsigned int count)
{
	struct phytium_pio *pio = hostdata;
	unsigned char *buf = buffer;

	if (!pio || !buf || !count || !dwidth || dwidth > 4)
		return -EINVAL;

	do {
		*(u32 *)buf = phytium_pio_in(pio, addr, dwidth);
		buf += dwidth;
		addr += dwidth;
	} while (--count);

	return 0;
}

/*
 * phytium_pio_outs - output the data in the buffer in multiple operations
 * @hostdata: pointer to the device information relevant to PIO controller
 * @addr: the target I/O port address
 * @buffer: the buffer to store the data
 * @dwidth: the data width required writing to the target I/O port
 * @count: the number of data to be written
 */
static void phytium_pio_outs(void *hostdata, unsigned long addr,
		const void *buffer, size_t dwidth, unsigned int count)
{
	struct phytium_pio *pio = hostdata;
	const unsigned char *buf = buffer;

	if (!pio || !buf || !count || !dwidth || dwidth > 4)
		return;

	do {
		phytium_pio_out(pio, addr, *(u32 *)buf, dwidth);
		buf += dwidth;
		addr += dwidth;
	} while (--count);
}

static const struct logic_pio_host_ops phytium_pio_ops = {
	.in = phytium_pio_in,
	.ins = phytium_pio_ins,
	.out = phytium_pio_out,
	.outs = phytium_pio_outs,
};

static int phytium_pio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct logic_pio_hwaddr *range;
	struct phytium_pio *pio;
	int ret;

	pio = devm_kzalloc(dev, sizeof(*pio), GFP_KERNEL);
	if (!pio)
		return -ENOMEM;
	pio->membase = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR_OR_NULL(pio->membase))
		return PTR_ERR(pio->membase);

	pio->irq = platform_get_irq(pdev, 0);
	if (pio->irq < 0) {
		dev_err(dev, "no irq resource?\n");
		return pio->irq;
	}

	{
		struct resource *res;
		resource_size_t size;
		u64 offset_from_end;

		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		if (res) {
			size = resource_size(res);

			offset_from_end = PHYTIUM_PIO_LENGTH_LONG - phytium_pio_int_state;
			if (offset_from_end <= size && offset_from_end > 0)
				phytium_pio_int_state = size - offset_from_end;

			offset_from_end = PHYTIUM_PIO_LENGTH_LONG - phytium_pio_clr_int;
			if (offset_from_end <= size && offset_from_end > 0)
				phytium_pio_clr_int = size - offset_from_end;
		}
	}

	ret = device_property_read_u32(&pdev->dev, "int_state",
			&pio->int_status_reg);
	if (ret)
		pio->int_status_reg = phytium_pio_int_state;

	ret = device_property_read_u32(&pdev->dev, "clr_int",
			&pio->int_clear_reg);
	if (ret)
		pio->int_clear_reg = phytium_pio_clr_int;

	dev_dbg(dev, "phytium_pio: int_status_reg=0x%x, int_clear_reg=0x%x\n",
		pio->int_status_reg, pio->int_clear_reg);
	range = devm_kzalloc(dev, sizeof(*range), GFP_KERNEL);
	if (!range)
		return -ENOMEM;
	range->fwnode = dev_fwnode(dev);
	range->flags = LOGIC_PIO_INDIRECT;
	range->size = PIO_INDIRECT_SIZE;
	range->hostdata = pio;
	range->ops = &phytium_pio_ops;
	pio->range = range;
	ret = logic_pio_register_range(range);
	if (ret) {
		dev_err(dev, "Failed to register logic pio range (%d)!\n", ret);
		goto out;
	}
	ppio = pio;
	spin_lock_init(&pio->lock);
	dev_set_drvdata(dev, pio);
out:
	return ret;
}

static int phytium_pio_remove(struct platform_device *pdev)
{
	struct logic_pio_hwaddr *range;

	range = find_io_range_by_fwnode(pdev->dev.fwnode);
	if (!range)
		return -EINVAL;

	logic_pio_unregister_range(range);

	return 0;
}

static const struct acpi_device_id phytium_pio_acpi_match[] = {
	{ "PHYT0007", },
	{ "LPC0001", },
	{},
};
MODULE_DEVICE_TABLE(acpi, phytium_pio_acpi_match);

static struct platform_driver phytium_pio_driver = {
	.driver = {
			.name = PHYT_PIO_DRIVER_NAME,
			.acpi_match_table = ACPI_PTR(phytium_pio_acpi_match),
		},
	.probe = phytium_pio_probe,
	.remove = phytium_pio_remove,
};


static int __init phytium_pio_init(void)
{
	if (unlikely(!check_cpu_type())) {
		pr_info("Not running on Phytium CPU\n");
		return -ENODEV;
	}
	if (cpu_is_ft_d2000()) {
		phytium_pio_int_state = FT2000_PIO_INT_STATE;
		phytium_pio_clr_int = FT2000_PIO_CLR_INT;
	} else if (cpu_is_ft_d3000() || cpu_is_ft_d3000m()) {
		phytium_pio_int_state = FT3000_PIO_INT_STATE;
		phytium_pio_clr_int = FT3000_PIO_CLR_INT;
	} else {
		phytium_pio_int_state = FT3000_PIO_INT_STATE;
		phytium_pio_clr_int = FT3000_PIO_CLR_INT;
		pr_notice("Unknown Phytium CPU, use D3000 regs (C0,C4).\n");
	}
	return platform_driver_register(&phytium_pio_driver);
}
arch_initcall(phytium_pio_init);

MODULE_DESCRIPTION("Phytium pio driver");
MODULE_VERSION(PHYT_PIO_DRV_VER);
MODULE_LICENSE("GPL");


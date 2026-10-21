// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Phytium PIO driver
 *
 * Copyright (C) 2024 Phytium Technology Co., Ltd. All Rights Reserved.
 */

#include <linux/acpi.h>
#include <linux/bitops.h>
#include <linux/cputypes.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/logic_pio.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include "phytium_pio.h"
#ifdef CONFIG_ARCH_PHYTIUM
#include <asm/phytium_cputype.h>
#endif

#define PHYT_PIO_DRIVER_NAME	"phytium-pio"
#define PHYT_PIO_DRV_VER	"1.1.0"

static struct phytium_pio *ppio;
int phytium_pio_int_state;
int phytium_pio_clr_int;

static irqreturn_t phytium_pio_irq_handler(int irq, void *dev_id);

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
	writel(val, ppio->regbase + ppio->int_clear_reg);
	readl(ppio->regbase + ppio->int_status_reg);
	spin_unlock_irqrestore(&ppio->lock, flags);
}
EXPORT_SYMBOL(phytium_pio_clear_interrupt);

void phytium_pio_handle_interrupt_bit(unsigned int bit)
{
	unsigned long flags;

	if (!ppio || bit >= 32)
		return;

	spin_lock_irqsave(&ppio->lock, flags);
	ppio->handled_bits |= BIT(bit);
	spin_unlock_irqrestore(&ppio->lock, flags);
}
EXPORT_SYMBOL(phytium_pio_handle_interrupt_bit);

void phytium_pio_unhandle_interrupt_bit(unsigned int bit)
{
	unsigned long flags;

	if (!ppio || bit >= 32)
		return;

	spin_lock_irqsave(&ppio->lock, flags);
	ppio->handled_bits &= ~BIT(bit);
	spin_unlock_irqrestore(&ppio->lock, flags);
}
EXPORT_SYMBOL(phytium_pio_unhandle_interrupt_bit);

bool phytium_pio_clear_interrupt_bit(unsigned int bit)
{
	unsigned long flags;
	u32 pending;

	if (!ppio || bit >= 32)
		return false;

	spin_lock_irqsave(&ppio->lock, flags);
	pending = readl(ppio->regbase + ppio->int_status_reg);
	if (pending & BIT(bit)) {
		writel(pending & ~BIT(bit), ppio->regbase + ppio->int_clear_reg);
		readl(ppio->regbase + ppio->int_status_reg);
	}
	spin_unlock_irqrestore(&ppio->lock, flags);

	return pending & BIT(bit);
}
EXPORT_SYMBOL(phytium_pio_clear_interrupt_bit);

u32 phytium_pio_get_int_status(void)
{
	unsigned long flags;
	u32 ret = 0;

	if (!ppio)
		return ret;
	spin_lock_irqsave(&ppio->lock, flags);
	ret = readl(ppio->regbase + ppio->int_status_reg);
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

static irqreturn_t phytium_pio_irq_handler(int irq, void *dev_id)
{
	struct phytium_pio *pio = dev_id;
	unsigned long flags;
	u32 clear_bits, pending;

	spin_lock_irqsave(&pio->lock, flags);
	pending = readl(pio->regbase + pio->int_status_reg);
	clear_bits = pending & ~pio->handled_bits;

	if (clear_bits) {
		writel(pending & ~clear_bits, pio->regbase + pio->int_clear_reg);
		readl(pio->regbase + pio->int_status_reg);
	}
	spin_unlock_irqrestore(&pio->lock, flags);

	return IRQ_RETVAL(pending);
}

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
	resource_size_t reg_size = PHYTIUM_LPC_IOMEM_SIZE;
	phys_addr_t reg_start;
	struct resource *res;
	int ret;

	pio = devm_kzalloc(dev, sizeof(*pio), GFP_KERNEL);
	if (!pio)
		return -ENOMEM;
	spin_lock_init(&pio->lock);

	pio->membase = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR_OR_NULL(pio->membase))
		return PTR_ERR(pio->membase);

	pio->irq = platform_get_irq(pdev, 0);
	if (pio->irq < 0) {
		dev_err(dev, "no irq resource?\n");
		return pio->irq;
	}

	/*
	 * The ACPI _CRS window only covers the short PIO aperture, while the
	 * interrupt latch registers live near the end of the long LPC window
	 * (base + 0x7ffffc0/0x7ffffc4).  Map the long window starting at the
	 * firmware supplied LPC base (0x10000000 on D3000M, 0x20000000 on
	 * D2000/D3000): reading the latch through the short window can return
	 * unrelated device state such as UART_IIR.
	 */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;
	reg_start = res->start;

	pio->regbase = devm_ioremap(dev, reg_start, reg_size);
	if (!pio->regbase)
		return -ENOMEM;

	pio->int_status_reg = phytium_pio_int_state;
	pio->int_clear_reg = phytium_pio_clr_int;
	writel(0, pio->regbase + pio->int_clear_reg);
	readl(pio->regbase + pio->int_status_reg);

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
	ppio = pio;
	ret = devm_request_irq(dev, pio->irq, phytium_pio_irq_handler,
			       IRQF_SHARED, dev_name(dev), pio);
	if (ret) {
		dev_err(dev, "Failed to request irq (%d)!\n", ret);
		goto out;
	}

	ret = logic_pio_register_range(range);
	if (ret) {
		dev_err(dev, "Failed to register logic pio range (%d)!\n", ret);
		goto out;
	}
	dev_set_drvdata(dev, pio);
out:
	if (ret)
		ppio = NULL;
	return ret;
}

static int phytium_pio_remove(struct platform_device *pdev)
{
	struct logic_pio_hwaddr *range;
	struct phytium_pio *pio = dev_get_drvdata(&pdev->dev);

	range = find_io_range_by_fwnode(pdev->dev.fwnode);
	if (!range)
		return -EINVAL;

	if (ppio == pio)
		ppio = NULL;
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
	} else if (is_pd2408()) {
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


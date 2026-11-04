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
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>
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
static int phytium_pio_int_state;
static int phytium_pio_clr_int;

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

static void phytium_pio_irq_noop(struct irq_data *d)
{
}

static struct irq_chip phytium_pio_irq_chip = {
	.name		= PHYT_PIO_DRIVER_NAME,
	.irq_mask	= phytium_pio_irq_noop,
	.irq_unmask	= phytium_pio_irq_noop,
	.irq_ack	= phytium_pio_irq_noop,
};

/* One Linux irq per latch bit, acknowledged once before the dispatch. */
static void phytium_pio_irq_dispatch(struct irq_desc *desc)
{
	struct phytium_pio *pio = irq_desc_get_handler_data(desc);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	unsigned long bits;
	u32 pending, again;
	unsigned int bit;

	chained_irq_enter(chip, desc);

	pending = readl(pio->regbase + pio->int_status_reg);
	if (!pending)
		goto out;

	/* The latch is write-zero-to-clear. */
	writel(~pending, pio->regbase + pio->int_clear_reg);
	again = readl(pio->regbase + pio->int_status_reg);

	bits = pending;
	for_each_set_bit(bit, &bits, PHYTIUM_PIO_IRQ_NR)
		generic_handle_domain_irq(pio->domain, bit);

	/*
	 * A source that is still asserted after its latch was cleared cannot
	 * be acknowledged; shut the line down instead of looping forever.
	 */
	if (again & pending && ++pio->stuck_acks > PHYTIUM_PIO_STUCK_LIMIT) {
		pr_err("phytium-pio: latch bit 0x%x stuck, disabling irq %d\n",
		       again & pending, pio->irq);
		disable_irq_nosync(pio->irq);
	} else if (!(again & pending)) {
		pio->stuck_acks = 0;
	}
out:
	chained_irq_exit(chip, desc);
}

static int phytium_pio_domain_map(struct irq_domain *d, unsigned int virq,
				  irq_hw_number_t hwirq)
{
	irq_set_chip_and_handler(virq, &phytium_pio_irq_chip, handle_simple_irq);
	irq_set_noprobe(virq);

	return 0;
}

static const struct irq_domain_ops phytium_pio_domain_ops = {
	.map = phytium_pio_domain_map,
};

/**
 * phytium_pio_irq_get - Linux irq of a latch bit
 * @bit: latch bit, see PHYTIUM_PIO_*_INT
 *
 * The mapping is created on the first call.
 *
 * Return: the Linux irq, or a negative errno.
 */
int phytium_pio_irq_get(unsigned int bit)
{
	unsigned int virq;

	if (!ppio || !ppio->domain || bit >= PHYTIUM_PIO_IRQ_NR)
		return -ENODEV;

	virq = irq_create_mapping(ppio->domain, bit);

	return virq ? virq : -ENOMEM;
}
EXPORT_SYMBOL_GPL(phytium_pio_irq_get);

/* Children live in the firmware window, not in the latch window. */
bool phytium_pio_is_lpc_mem(phys_addr_t addr)
{
	return ppio && addr >= ppio->win_base &&
	       addr - ppio->win_base < PHYTIUM_PIO_LENGTH_LONG;
}
EXPORT_SYMBOL(phytium_pio_is_lpc_mem);

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
	phys_addr_t reg_start;
	struct resource *res;
	int ret;

	pio = devm_kzalloc(dev, sizeof(*pio), GFP_KERNEL);
	if (!pio)
		return -ENOMEM;
	spin_lock_init(&pio->lock);

	pio->membase = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(pio->membase))
		return PTR_ERR(pio->membase);

	pio->irq = platform_get_irq(pdev, 0);
	if (pio->irq < 0) {
		dev_err(dev, "no irq resource?\n");
		return pio->irq;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;
	pio->win_base = res->start;

	/* The latch window is a SoC property, not what resource 0 describes. */
	if (cpu_is_ft_d2000()) {
		reg_start = PHYTIUM_PIO_D2000_BASE;
	} else if (is_pd2408()) {
		reg_start = PHYTIUM_PIO_D3000M_BASE;
	} else {
		reg_start = res->start;
		if (resource_size(res) < PHYTIUM_PIO_LENGTH_LONG)
			dev_warn(dev, "short window on an unknown SoC, the latch address may be wrong\n");
	}

	pio->regbase = devm_ioremap(dev, reg_start, PHYTIUM_PIO_LENGTH_LONG);
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

	pio->domain = irq_domain_create_linear(dev_fwnode(dev),
					       PHYTIUM_PIO_IRQ_NR,
					       &phytium_pio_domain_ops, pio);
	if (!pio->domain) {
		dev_err(dev, "Failed to create the latch irq domain\n");
		return -ENOMEM;
	}

	ret = logic_pio_register_range(range);
	if (ret) {
		dev_err(dev, "Failed to register logic pio range (%d)!\n", ret);
		irq_domain_remove(pio->domain);
		pio->domain = NULL;
		return ret;
	}

	irq_set_chained_handler_and_data(pio->irq, phytium_pio_irq_dispatch, pio);

	/* Children look the controller up through ppio. */
	ppio = pio;
	dev_set_drvdata(dev, pio);

	return 0;
}

static int phytium_pio_remove(struct platform_device *pdev)
{
	struct logic_pio_hwaddr *range;
	struct phytium_pio *pio = dev_get_drvdata(&pdev->dev);

	range = find_io_range_by_fwnode(pdev->dev.fwnode);
	if (!range)
		return -EINVAL;

	if (pio && pio->domain) {
		int bit;

		irq_set_chained_handler_and_data(pio->irq, NULL, NULL);
		for (bit = 0; bit < PHYTIUM_PIO_IRQ_NR; bit++) {
			unsigned int virq = irq_find_mapping(pio->domain, bit);

			if (virq)
				irq_dispose_mapping(virq);
		}
		irq_domain_remove(pio->domain);
		pio->domain = NULL;
	}

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
			.suppress_bind_attrs = true,
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


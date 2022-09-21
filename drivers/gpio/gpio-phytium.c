/*
 * Support functions for Phytium GPIO
 *
 * Copyright (c) 2019, Phytium Corporation.
 * Written by Chen Baozi <chenbaozi@phytium.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/acpi.h>
#include <linux/err.h>
#include <linux/gpio/driver.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/bitops.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/property.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>

#include "gpiolib.h"

#define GPIO_SWPORTA_DR		0x00 /* WR Port A Output Data Register */
#define GPIO_SWPORTA_DDR	0x04 /* WR Port A Data Direction Register */
#define GPIO_EXT_PORTA		0x08 /* RO Port A Input Data Register */
#define GPIO_SWPORTB_DR		0x0c /* WR Port B Output Data Register */
#define GPIO_SWPORTB_DDR	0x10 /* WR Port B Data Direction Register */
#define GPIO_EXT_PORTB		0x14 /* RO Port B Input Data Register */

#define GPIO_INTEN		0x18 /* WR Port A Interrput Enable Register */
#define GPIO_INTMASK		0x1c /* WR Port A Interrupt Mask Register */
#define GPIO_INTTYPE_LEVEL	0x20 /* WR Port A Interrupt Level Register */
#define GPIO_INT_POLARITY	0x24 /* WR Port A Interrupt Polarity Register */
#define GPIO_INTSTATUS		0x28 /* RO Port A Interrupt Status Register */
#define GPIO_RAW_INTSTATUS	0x2c /* RO Port A Raw Interrupt Status Register */
#define GPIO_LS_SYNC		0x30 /* WR Level-sensitive Synchronization Enable Register */
#define GPIO_DEBOUNCE		0x34 /* WR Debounce Enable Register */
#define GPIO_PORTA_EOI		0x38 /* WO Port A Clear Interrupt Register */

#define MAX_NPORTS		2
#define NGPIO_DEFAULT		8
#define NGPIO_MAX		32
#define GPIO_PORT_STRIDE	(GPIO_EXT_PORTB - GPIO_EXT_PORTA)

struct pin_loc {
	unsigned port;
	unsigned offset;
};

#ifdef CONFIG_PM_SLEEP
struct phytium_gpio_ctx {
	u32 swporta_dr;
	u32 swporta_ddr;
	u32 ext_porta;
	u32 swportb_dr;
	u32 swportb_ddr;
	u32 ext_portb;
	u32 inten;
	u32 intmask;
	u32 inttype_level;
	u32 int_polarity;
	u32 intstatus;
	u32 raw_intstatus;
	u32 ls_sync;
	u32 debounce;
};
#endif

struct phytium_gpio {
	raw_spinlock_t		lock;
	void __iomem		*regs;
	struct gpio_chip	gc;
	unsigned int		ngpio[2];
	int			irq;
#ifdef CONFIG_PM_SLEEP
	struct phytium_gpio_ctx	ctx;
#endif
};

static int get_pin_location(struct phytium_gpio *gpio, unsigned int offset,
			    struct pin_loc *pl)
{
	int ret;

	if (offset < gpio->ngpio[0]) {
		pl->port = 0;
		pl->offset = offset;
		ret = 0;
	} else if (offset < (gpio->ngpio[0] + gpio->ngpio[1])) {
		pl->port = 1;
		pl->offset = offset - gpio->ngpio[0];
		ret = 0;
	} else {
		ret = -EINVAL;
	}

	return ret;
}

static void phytium_gpio_toggle_trigger(struct phytium_gpio *gpio,
					unsigned int offset)
{
	struct gpio_chip *gc;
	u32 pol;
	int val;

	/* Only port A can provide interrupt source */
	if (offset >= gpio->ngpio[0])
		return;

	gc = &gpio->gc;

	pol = readl(gpio->regs + GPIO_INT_POLARITY);
	/* Just read the current value right out of the data register */
	val = gc->get(gc, offset);
	if (val)
		pol &= ~BIT(offset);
	else
		pol |= ~BIT(offset);

	writel(pol, gpio->regs + GPIO_INT_POLARITY);
}

static void phytium_gpio_irq_ack(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct phytium_gpio *gpio = gpiochip_get_data(gc);
	u32 val = BIT(irqd_to_hwirq(d));

	raw_spin_lock(&gpio->lock);

	writel(val , gpio->regs + GPIO_PORTA_EOI);

	raw_spin_unlock(&gpio->lock);
}

static void phytium_gpio_irq_mask(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct phytium_gpio *gpio = gpiochip_get_data(gc);
	u32 val;

	/* Only port A can provide interrupt source */
	if (irqd_to_hwirq(d) >= gpio->ngpio[0])
		return;

	raw_spin_lock(&gpio->lock);

	val = readl(gpio->regs + GPIO_INTMASK);
	val |= BIT(irqd_to_hwirq(d));
	writel(val, gpio->regs + GPIO_INTMASK);

	raw_spin_unlock(&gpio->lock);
}

static void phytium_gpio_irq_unmask(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct phytium_gpio *gpio = gpiochip_get_data(gc);
	u32 val;

	/* Only port A can provide interrupt source */
	if (irqd_to_hwirq(d) >= gpio->ngpio[0])
		return;

	raw_spin_lock(&gpio->lock);

	val = readl(gpio->regs + GPIO_INTMASK);
	val &= ~BIT(irqd_to_hwirq(d));
	writel(val, gpio->regs + GPIO_INTMASK);

	raw_spin_unlock(&gpio->lock);
}

static int phytium_gpio_irq_set_type(struct irq_data *d, unsigned int flow_type)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct phytium_gpio *gpio = gpiochip_get_data(gc);
	int hwirq = irqd_to_hwirq(d);
	unsigned long flags, lvl, pol;

	if (hwirq < 0 || hwirq >= gpio->ngpio[0])
		return -EINVAL;

	if ((flow_type & (IRQ_TYPE_LEVEL_HIGH | IRQ_TYPE_LEVEL_LOW)) &&
	    (flow_type & (IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING))) {
		dev_err(gc->parent,
			"trying to configure line %d for both level and edge "
			"detection, choose one!\n",
			hwirq);
		return -EINVAL;
	}

	raw_spin_lock_irqsave(&gpio->lock, flags);

	lvl = readl(gpio->regs + GPIO_INTTYPE_LEVEL);
	pol = readl(gpio->regs + GPIO_INT_POLARITY);

	switch (flow_type) {
	case IRQ_TYPE_EDGE_BOTH:
		lvl |= BIT(hwirq);
		phytium_gpio_toggle_trigger(gpio, hwirq);
		irq_set_handler_locked(d, handle_edge_irq);
		dev_dbg(gc->parent, "line %d: IRQ on both edges\n", hwirq);
		break;
	case IRQ_TYPE_EDGE_RISING:
		lvl |= BIT(hwirq);
		pol |= BIT(hwirq);
		irq_set_handler_locked(d, handle_edge_irq);
		dev_dbg(gc->parent, "line %d: IRQ on RISING edge\n", hwirq);
		break;
	case IRQ_TYPE_EDGE_FALLING:
		lvl |= BIT(hwirq);
		pol &= ~BIT(hwirq);
		irq_set_handler_locked(d, handle_edge_irq);
		dev_dbg(gc->parent, "line %d: IRQ on FALLING edge\n", hwirq);
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		lvl &= ~BIT(hwirq);
		pol |= BIT(hwirq);
		irq_set_handler_locked(d, handle_level_irq);
		dev_dbg(gc->parent, "line %d: IRQ on HIGH level\n", hwirq);
		break;
	case IRQ_TYPE_LEVEL_LOW:
		lvl &= ~BIT(hwirq);
		pol &= ~BIT(hwirq);
		irq_set_handler_locked(d, handle_level_irq);
		dev_dbg(gc->parent, "line %d: IRQ on LOW level\n", hwirq);
		break;
	}

	writel(lvl, gpio->regs + GPIO_INTTYPE_LEVEL);
	if (flow_type != IRQ_TYPE_EDGE_BOTH)
		writel(pol, gpio->regs + GPIO_INT_POLARITY);

	raw_spin_unlock_irqrestore(&gpio->lock, flags);

	return 0;
}

static void phytium_gpio_irq_enable(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct phytium_gpio *gpio = gpiochip_get_data(gc);
	unsigned long flags;
	u32 val;

	/* Only port A can provide interrupt source */
	if (irqd_to_hwirq(d) >= gpio->ngpio[0])
		return;

	raw_spin_lock_irqsave(&gpio->lock, flags);

	val = readl(gpio->regs + GPIO_INTEN);
	val |= BIT(irqd_to_hwirq(d));
	writel(val, gpio->regs + GPIO_INTEN);

	raw_spin_unlock_irqrestore(&gpio->lock, flags);
}

static void phytium_gpio_irq_disable(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct phytium_gpio *gpio = gpiochip_get_data(gc);
	unsigned long flags;
	u32 val;

	/* Only port A can provide interrupt source */
	if (irqd_to_hwirq(d) >= gpio->ngpio[0])
		return;

	raw_spin_lock_irqsave(&gpio->lock, flags);

	val = readl(gpio->regs + GPIO_INTEN);
	val &= ~BIT(irqd_to_hwirq(d));
	writel(val, gpio->regs + GPIO_INTEN);

	raw_spin_unlock_irqrestore(&gpio->lock, flags);
}

static void phytium_gpio_irq_handler(struct irq_desc *desc)
{
	struct gpio_chip *gc = irq_desc_get_handler_data(desc);
	struct phytium_gpio *gpio = gpiochip_get_data(gc);
	struct irq_chip *irqchip = irq_desc_get_chip(desc);
	unsigned long pending;
	int offset;

	chained_irq_enter(irqchip, desc);

	pending = readl(gpio->regs + GPIO_INTSTATUS);
	if (pending) {
		for_each_set_bit(offset, &pending, gpio->ngpio[0]) {
			int gpio_irq = irq_find_mapping(gc->irq.domain,
							offset);
			generic_handle_irq(gpio_irq);

			if ((irq_get_trigger_type(gpio_irq) &
			    IRQ_TYPE_SENSE_MASK) == IRQ_TYPE_EDGE_BOTH)
				phytium_gpio_toggle_trigger(gpio, offset);
		}
	}

	chained_irq_exit(irqchip, desc);
}

static struct irq_chip phytium_gpio_irqchip = {
	.name 			= "phytium_gpio",
	.irq_ack		= phytium_gpio_irq_ack,
	.irq_mask		= phytium_gpio_irq_mask,
	.irq_unmask		= phytium_gpio_irq_unmask,
	.irq_set_type		= phytium_gpio_irq_set_type,
	.irq_enable		= phytium_gpio_irq_enable,
	.irq_disable		= phytium_gpio_irq_disable,
};

static const struct of_device_id phytium_gpio_of_match[] = {
	{ .compatible = "phytium,gpio", },
	{ }
};
MODULE_DEVICE_TABLE(of, phytium_gpio_of_match);

static const struct acpi_device_id phytium_gpio_acpi_match[] = {
	{ "PHYT0001", 0 },
	{ "FTGP0001", 0 },
	{ },
};
MODULE_DEVICE_TABLE(acpi, phytium_gpio_acpi_match);

static int phytium_gpio_get_direction(struct gpio_chip *gc, unsigned offset)
{
	struct phytium_gpio *gpio = gpiochip_get_data(gc);
	struct pin_loc loc;
	void __iomem *ddr;

	if (get_pin_location(gpio, offset, &loc))
		return -EINVAL;
	ddr = gpio->regs + GPIO_SWPORTA_DDR + (loc.port * GPIO_PORT_STRIDE);

	return !(readl(ddr) & BIT(loc.offset));
}

static int phytium_gpio_direction_input(struct gpio_chip *gc, unsigned offset)
{
	struct phytium_gpio *gpio = gpiochip_get_data(gc);
	struct pin_loc loc;
	unsigned long flags;
	void __iomem *ddr;

	if (get_pin_location(gpio, offset, &loc))
		return -EINVAL;
	ddr = gpio->regs + GPIO_SWPORTA_DDR + (loc.port * GPIO_PORT_STRIDE);

	raw_spin_lock_irqsave(&gpio->lock, flags);

	writel(readl(ddr) & ~(BIT(loc.offset)), ddr);

	raw_spin_unlock_irqrestore(&gpio->lock, flags);

	return 0;
}

static void phytium_gpio_set(struct gpio_chip *gc, unsigned offset, int value)
{
	struct phytium_gpio *gpio = gpiochip_get_data(gc);
	struct pin_loc loc;
	void __iomem *dr;
	unsigned long flags;
	u32 mask;

	if (get_pin_location(gpio, offset, &loc))
		return;
	dr = gpio->regs + GPIO_SWPORTA_DR + (loc.port * GPIO_PORT_STRIDE);

	raw_spin_lock_irqsave(&gpio->lock, flags);

	if (value)
		mask = readl(dr) | BIT(loc.offset);
	else
		mask = readl(dr) & ~BIT(loc.offset);

	writel(mask, dr);

	raw_spin_unlock_irqrestore(&gpio->lock, flags);

	return;
}

static int phytium_gpio_direction_output(struct gpio_chip *gc, unsigned offset,
					 int value)
{
	struct phytium_gpio *gpio = gpiochip_get_data(gc);
	struct pin_loc loc;
	unsigned long flags;
	void __iomem *ddr;

	if (get_pin_location(gpio, offset, &loc))
		return -EINVAL;
	ddr = gpio->regs + GPIO_SWPORTA_DDR + (loc.port * GPIO_PORT_STRIDE);

	raw_spin_lock_irqsave(&gpio->lock, flags);

	writel(readl(ddr) | BIT(loc.offset), ddr);

	raw_spin_unlock_irqrestore(&gpio->lock, flags);

	phytium_gpio_set(gc, offset, value);

	return 0;
}

static int phytium_gpio_get(struct gpio_chip *gc, unsigned offset)
{
	struct phytium_gpio *gpio = gpiochip_get_data(gc);
	struct pin_loc loc;
	void __iomem *dat;

	if (get_pin_location(gpio, offset, &loc))
		return -EINVAL;

	dat = gpio->regs + GPIO_EXT_PORTA + (loc.port * GPIO_PORT_STRIDE);

	return !!(readl(dat) & BIT(loc.offset));
}

static int phytium_gpio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct phytium_gpio *gpio;
	struct fwnode_handle *fwnode;
	struct gpio_irq_chip *girq;
	int err;

	gpio = devm_kzalloc(&pdev->dev, sizeof(*gpio), GFP_KERNEL);
	if (!gpio)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	gpio->regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(gpio->regs))
		return PTR_ERR(gpio->regs);

	gpio->irq = -ENXIO;
	gpio->irq = platform_get_irq(pdev, 0);
	if (gpio->irq < 0)
		dev_warn(dev, "no irq is found.\n");

	if (!device_get_child_node_count(dev))
		return -ENODEV;

	device_for_each_child_node(dev, fwnode) {
		int idx;

		if (fwnode_property_read_u32(fwnode, "reg", &idx) ||
		    idx >= MAX_NPORTS) {
			dev_err(dev, "missing/invalid port index\n");
			fwnode_handle_put(fwnode);
			return -EINVAL;
		}

		if (fwnode_property_read_u32(fwnode, "nr-gpios",
					     &gpio->ngpio[idx])) {
			dev_info(dev,
				 "failed to get number of gpios for Port%c\n",
				 idx ? 'B' : 'A');
			gpio->ngpio[idx] = NGPIO_DEFAULT;
		}
	}

	/* irq_chip support */
	raw_spin_lock_init(&gpio->lock);

	gpio->gc.base = -1;
	gpio->gc.get_direction = phytium_gpio_get_direction;
	gpio->gc.direction_input = phytium_gpio_direction_input;
	gpio->gc.direction_output = phytium_gpio_direction_output;
	gpio->gc.get = phytium_gpio_get;
	gpio->gc.set = phytium_gpio_set;
	gpio->gc.ngpio = gpio->ngpio[0] + gpio->ngpio[1];
	gpio->gc.label = dev_name(dev);
	gpio->gc.parent = dev;
	gpio->gc.owner = THIS_MODULE;

	girq = &gpio->gc.irq;
	girq->chip = &phytium_gpio_irqchip;
	girq->parent_handler = phytium_gpio_irq_handler;
	girq->num_parents = 1;
	girq->parents = devm_kcalloc(dev, 1, sizeof(*girq->parents), GFP_KERNEL);
	if (!girq->parents)
		return -ENOMEM;
	girq->parents[0] = platform_get_irq(pdev, 0);
	girq->default_type = IRQ_TYPE_NONE;
	girq->handler = handle_bad_irq;

	err = devm_gpiochip_add_data(dev, &gpio->gc, gpio);
	if (err)
	{
		dev_err(dev, "cannot add phytium gpio chip, %d\n", err);
		return err;
	}
	platform_set_drvdata(pdev, gpio);

	return 0;
}

static int phytium_gpio_remove(struct platform_device *pdev)
{
	struct phytium_gpio *gpio = platform_get_drvdata(pdev);

	gpiochip_remove(&gpio->gc);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int phytium_gpio_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct phytium_gpio *gpio = platform_get_drvdata(pdev);
	unsigned long flags;

	raw_spin_lock_irqsave(&gpio->lock, flags);

	gpio->ctx.swporta_dr = readl(gpio->regs + GPIO_SWPORTA_DR);
	gpio->ctx.swporta_ddr = readl(gpio->regs + GPIO_SWPORTA_DDR);
	gpio->ctx.ext_porta = readl(gpio->regs + GPIO_EXT_PORTA);
	gpio->ctx.swportb_dr = readl(gpio->regs + GPIO_SWPORTB_DR);
	gpio->ctx.swportb_ddr = readl(gpio->regs + GPIO_SWPORTB_DDR);
	gpio->ctx.ext_portb = readl(gpio->regs + GPIO_EXT_PORTB);

	gpio->ctx.inten = readl(gpio->regs + GPIO_INTEN);
	gpio->ctx.intmask = readl(gpio->regs + GPIO_INTMASK);
	gpio->ctx.inttype_level = readl(gpio->regs + GPIO_INTTYPE_LEVEL);
	gpio->ctx.int_polarity = readl(gpio->regs + GPIO_INT_POLARITY);
	gpio->ctx.debounce = readl(gpio->regs + GPIO_DEBOUNCE);

	raw_spin_unlock_irqrestore(&gpio->lock, flags);

	return 0;
}

static int phytium_gpio_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct phytium_gpio *gpio = platform_get_drvdata(pdev);
	unsigned long flags;

	raw_spin_lock_irqsave(&gpio->lock, flags);

	writel(gpio->ctx.swporta_dr, gpio->regs + GPIO_SWPORTA_DR);
	writel(gpio->ctx.swporta_ddr, gpio->regs + GPIO_SWPORTA_DDR);
	writel(gpio->ctx.ext_porta, gpio->regs + GPIO_EXT_PORTA);
	writel(gpio->ctx.swportb_dr, gpio->regs + GPIO_SWPORTB_DR);
	writel(gpio->ctx.swportb_ddr, gpio->regs + GPIO_SWPORTB_DDR);
	writel(gpio->ctx.ext_portb, gpio->regs + GPIO_EXT_PORTB);

	writel(gpio->ctx.inten, gpio->regs + GPIO_INTEN);
	writel(gpio->ctx.intmask, gpio->regs + GPIO_INTMASK);
	writel(gpio->ctx.inttype_level, gpio->regs + GPIO_INTTYPE_LEVEL);
	writel(gpio->ctx.int_polarity, gpio->regs + GPIO_INT_POLARITY);
	writel(gpio->ctx.debounce, gpio->regs + GPIO_DEBOUNCE);

	writel(0xffffffff, gpio->regs + GPIO_PORTA_EOI);

	raw_spin_unlock_irqrestore(&gpio->lock, flags);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(phytium_gpio_pm_ops, phytium_gpio_suspend,
			 phytium_gpio_resume);

static struct platform_driver phytium_gpio_driver = {
	.driver		= {
		.name	= "gpio-phytium",
		.pm	= &phytium_gpio_pm_ops,
		.of_match_table = of_match_ptr(phytium_gpio_of_match),
		.acpi_match_table = ACPI_PTR(phytium_gpio_acpi_match),
	},
	.probe		= phytium_gpio_probe,
	.remove		= phytium_gpio_remove,
};

module_platform_driver(phytium_gpio_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chen Baozi <chenbaozi@phytium.com.cn>");
MODULE_DESCRIPTION("Phytium GPIO driver");

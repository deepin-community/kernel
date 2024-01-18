// SPDX-License-Identifier: GPL-2.0
#include <linux/bitops.h>
#include <linux/irq.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/irqchip.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/interrupt.h>

#define LPC_NR_IRQS 16
#define	LPC_IRQ  0x4
#define	LPC_IRQ_MASK  0x8

struct lpc_intc_data {
	struct irq_domain *domain;
	struct irq_chip_generic *gc;
};

static void lpc_irq_mask_ack(struct irq_data *data)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(data);
	struct irq_chip_type *ct = irq_data_get_chip_type(data);
	unsigned int mask = data->mask;

	irq_gc_lock(gc);
	*ct->mask_cache |= mask;
	irq_reg_writel(gc, *ct->mask_cache, ct->regs.mask);
	irq_reg_writel(gc, mask, ct->regs.ack);
	irq_gc_unlock(gc);
}

static void lpc_irq_handler(struct irq_desc *desc)
{
	struct lpc_intc_data *b = irq_desc_get_handler_data(desc);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	unsigned int irq;
	u32 status;

	chained_irq_enter(chip, desc);

	status = irq_reg_readl(b->gc, LPC_IRQ);

	if (status == 0) {
		raw_spin_lock(&desc->lock);
		handle_bad_irq(desc);
		raw_spin_unlock(&desc->lock);
		goto out;
	}

	while (status) {
		irq = __ffs(status);
		status &= ~BIT(irq);
		generic_handle_irq(irq_find_mapping(b->domain, irq));
	}

out:
	chained_irq_exit(chip, desc);
}

static int __init lpc_intc_of_init(struct device_node *np,
				  struct device_node *parent)
{
	unsigned int set = IRQ_NOPROBE | IRQ_LEVEL;
	struct lpc_intc_data *data;
	struct irq_chip_type *ct;
	int parent_irq, ret;
	void __iomem *base;
	int hwirq = 0;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	base = of_iomap(np, 0);
	if (!base) {
		pr_err("failed to remap lpc intc registers\n");
		ret = -ENOMEM;
		goto out_free;
	}

	parent_irq = irq_of_parse_and_map(np, 0);
	if (!parent_irq) {
		pr_err("failed to find parent interrupt\n");
		ret = -EINVAL;
		goto out_unmap;
	}

	data->domain = irq_domain_add_linear(np, LPC_NR_IRQS,
				&irq_generic_chip_ops, NULL);
	if (!data->domain) {
		ret = -ENOMEM;
		goto out_unmap;
	}

	/* Allocate a single Generic IRQ chip for this node */
	ret = irq_alloc_domain_generic_chips(data->domain, 16, 1, np->name,
					     handle_level_irq, 0, set,
					     IRQ_GC_INIT_MASK_CACHE);
	if (ret) {
		pr_err("failed to allocate generic irq chip\n");
		goto out_free_domain;
	}

	/* Set the IRQ chaining logic */
	irq_set_chained_handler_and_data(parent_irq,
					 lpc_irq_handler, data);

	data->gc = irq_get_domain_generic_chip(data->domain, 0);
	data->gc->reg_base = base;
	data->gc->private = data;

	ct = data->gc->chip_types;

	ct->regs.ack = LPC_IRQ;
	ct->regs.mask = LPC_IRQ_MASK;
	ct->chip.irq_mask = irq_gc_mask_set_bit;
	ct->chip.irq_unmask = irq_gc_mask_clr_bit;
	ct->chip.irq_ack = irq_gc_ack_set_bit;
	ct->chip.irq_mask_ack = lpc_irq_mask_ack;

	for (hwirq = 0 ; hwirq < 16 ; hwirq++)
		irq_create_mapping(data->domain, hwirq);

	/* Enable LPC interrupts */
	writel(0xffffebdd, base + LPC_IRQ_MASK);

	return 0;

out_free_domain:
	irq_domain_remove(data->domain);
out_unmap:
	iounmap(base);
out_free:
	kfree(data);
	return ret;
}
IRQCHIP_DECLARE(sw_lpc_intc, "sw64,lpc_intc", lpc_intc_of_init);

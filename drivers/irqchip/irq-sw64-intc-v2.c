// SPDX-License-Identifier: GPL-2.0
#include <linux/irqdomain.h>
#include <linux/irqchip.h>
#include <linux/acpi.h>
#include <linux/acpi_iort.h>
#include <linux/of_irq.h>
#include <asm/sw64io.h>

static void fake_irq_mask(struct irq_data *data)
{
}

static void fake_irq_unmask(struct irq_data *data)
{
}

static struct irq_chip onchip_intc = {
	.name           = "SW fake Intc",
	.irq_mask       = fake_irq_mask,
	.irq_unmask     = fake_irq_unmask,
};

static int sw64_intc_domain_map(struct irq_domain *d, unsigned int irq,
			      irq_hw_number_t hw)
{

	irq_set_chip_and_handler(irq, &onchip_intc, handle_level_irq);
	irq_set_status_flags(irq, IRQ_LEVEL);
	return 0;
}

static const struct irq_domain_ops sw64_intc_domain_ops = {
	.xlate = irq_domain_xlate_onecell,
	.map = sw64_intc_domain_map,
};

#ifdef CONFIG_OF
static struct irq_domain *root_domain;

static int __init
init_onchip_IRQ(struct device_node *intc, struct device_node *parent)
{

	int node = 0;
	int hwirq = 0, nirq = 8;

	if (parent)
		panic("DeviceTree incore intc not a root irq controller\n");

	root_domain = irq_domain_add_linear(intc, 8,
						&sw64_intc_domain_ops, NULL);

	if (!root_domain)
		panic("root irq domain not avail\n");

	/* with this we don't need to export root_domain */
	irq_set_default_host(root_domain);

	for (hwirq = 0 ; hwirq < nirq ; hwirq++)
		irq_create_mapping(root_domain, hwirq);

	/*enable MCU_DVC_INT_EN*/
	sw64_io_write(node, MCU_DVC_INT_EN, 0xff);

	return 0;
}

IRQCHIP_DECLARE(sw64_intc, "sw64,sw6_irq_controller", init_onchip_IRQ);

static int __init
init_onchip_vt_IRQ(struct device_node *intc, struct device_node *parent)
{
	if (parent)
		panic("DeviceTree incore intc not a root irq controller\n");

	root_domain = irq_domain_add_legacy(intc, 16, 0, 0,
						&sw64_intc_domain_ops, NULL);

	if (!root_domain)
		panic("root irq domain not avail\n");

	/* with this we don't need to export root_domain */
	irq_set_default_host(root_domain);

	return 0;
}

IRQCHIP_DECLARE(sw64_vt_intc, "sw64,sw6_irq_vt_controller", init_onchip_vt_IRQ);
#endif

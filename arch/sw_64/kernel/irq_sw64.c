// SPDX-License-Identifier: GPL-2.0
/*
 * SW64 specific irq code.
 */

#include <linux/irq.h>
#include <linux/irqchip.h>

#include <asm/dma.h>
#include <asm/irq_impl.h>

void __init
init_IRQ(void)
{
	/*
	 * Just in case the platform init_irq() causes interrupts/mchecks
	 * (as is the case with RAWHIDE, at least).
	 */
	if (is_in_host()) {
		write_csr(0xffffffffffffffffUL, CSR_PCIE_MSI0_INTEN);
		write_csr(0xffffffffffffffffUL, CSR_PCIE_MSI1_INTEN);
		write_csr(0xffffffffffffffffUL, CSR_PCIE_MSI2_INTEN);
		write_csr(0xffffffffffffffffUL, CSR_PCIE_MSI3_INTEN);
	}

	wrent(entInt, 0);

	sw64_init_irq();
	irqchip_init();
}

DEFINE_SPINLOCK(irq_lock);

static void
__enable_irq(struct irq_data *d)
{
}

static void
__disable_irq(struct irq_data *d)
{
}

static unsigned int
__startup_irq(struct irq_data *d)
{
	__enable_irq(d);
	return 0;
}

static void
__mask_and_ack_irq(struct irq_data *d)
{
	spin_lock(&irq_lock);
	__disable_irq(d);
	spin_unlock(&irq_lock);
}

struct irq_chip sw64_irq_chip = {
	.name = "SW64_NODE",
	.irq_startup = __startup_irq,
	.irq_unmask = __enable_irq,
	.irq_mask = __disable_irq,
	.irq_mask_ack = __mask_and_ack_irq,
};

void __weak arch_init_msi_domain(struct irq_domain *parent) {}

int __init arch_early_irq_init(void)
{
	int i;

	for (i = 0; i < NR_IRQS; ++i) {
		irq_set_chip_and_handler(i, &sw64_irq_chip, handle_level_irq);
		irq_set_status_flags(i, IRQ_LEVEL);
	}
	arch_init_msi_domain(NULL);
	return 0;
}

int __init arch_probe_nr_irqs(void)
{
	return NR_IRQS_LEGACY;
}

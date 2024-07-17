/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_IRQ_H
#define _ASM_SW64_IRQ_H

/*
 *	arch/sw/include/asm/irq.h
 *
 *	(C) 2012 OSKernel JN
 */

#include <linux/linkage.h>

#define NR_VECTORS_PERCPU	256
#define NR_IRQS_LEGACY		16
#define NR_IRQS			((NR_VECTORS_PERCPU + NR_IRQS_LEGACY) * NR_CPUS)

static inline int irq_canonicalize(int irq)
{
	/*
	 * XXX is this true for all Sw?  The old serial driver
	 * did it this way for years without any complaints, so....
	 */
	return ((irq == 2) ? 9 : irq);
}

struct pt_regs;
extern void (*perf_irq)(unsigned long vector, struct pt_regs *regs);
extern void fixup_irqs(void);
extern void sw64_timer_interrupt(void);

struct irq_domain;
struct fwnode_handle;

struct acpi_madt_sw_pintc;
struct acpi_madt_sw_msic;
struct acpi_madt_sw_lpc_intc;

extern int __init sw64_add_gsi_domain_map(u32 gsi_base, u32 gsi_count,
		struct fwnode_handle *handle);

#ifdef CONFIG_SW64_PINTC
extern int __init pintc_acpi_init(struct irq_domain *parent,
		struct acpi_madt_sw_pintc *pintc);
#else
static inline int __init pintc_acpi_init(struct irq_domain *parent,
		struct acpi_madt_sw_pintc *pintc)
{
	return 0;
}
#endif

#ifdef CONFIG_SW64_LPC_INTC
extern int __init lpc_intc_acpi_init(struct irq_domain *parent,
		struct acpi_madt_sw_lpc_intc *lpc_intc);
#else
static inline int __init lpc_intc_acpi_init(struct irq_domain *parent,
		struct acpi_madt_sw_lpc_intc *lpc_intc)
{
	return 0;
}
#endif

#endif /* _ASM_SW64_IRQ_H */

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_HW_IRQ_H
#define _ASM_SW64_HW_IRQ_H

#include <asm/msi.h>

extern volatile unsigned long irq_err_count;
DECLARE_PER_CPU(unsigned long, irq_pmi_count);

#define ACTUAL_NR_IRQS nr_irqs

extern struct irq_domain *mcu_irq_domain;

#ifdef CONFIG_PCI_MSI
typedef unsigned int vector_irq_t[PERCPU_MSI_IRQS];
DECLARE_PER_CPU(vector_irq_t, vector_irq);
#endif
#endif /* _ASM_SW64_HW_IRQ_H */

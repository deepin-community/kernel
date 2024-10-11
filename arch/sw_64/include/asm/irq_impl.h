/* SPDX-License-Identifier: GPL-2.0 */
/*
 * This file contains declarations and inline functions for interfacing
 * with the IRQ handling routines in irq.c.
 */

#ifndef _ASM_SW64_IRQ_IMPL_H
#define _ASM_SW64_IRQ_IMPL_H

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/profile.h>

#include <asm/sw64io.h>

#define SW64_PCIE0_INT_BASE 17
#define SW64_PCIE0_MSI_BASE 21

#define SW64_PCIE1_INT_BASE 277
#define SW64_PCIE1_MSI_BASE 281

#define RTC_IRQ		8
#define SWI2C_IRQ	14

enum sw64_irq_type {
	INT_IPI		= 1,
	INT_PC		= 2,	/* C4 PMI */
	INT_PC0		= 2,	/* C3 PMI 0 */
	INT_PC1		= 3,	/* C3 PMI 1 */
	INT_INTx	= 5,
	INT_MSI		= 6,
	INT_MT		= 7,
	INT_RTC		= 9,
	INT_FAULT	= 10,
	INT_VT_SERIAL	= 12,
	INT_VT_HOTPLUG	= 13,
	INT_VT_GPIOA_PIN0 = 15,
	INT_DEV		= 17,
	INT_NMI		= 18,
	INT_LEGACY	= 31,
};

extern struct irqaction timer_irqaction;
extern void init_rtc_irq(irq_handler_t handler);
extern void handle_irq(int irq);
extern void handle_ipi(struct pt_regs *regs);
extern void __init sunway_init_pci_intx(void);
extern irqreturn_t timer_interrupt(int irq, void *dev);

#endif /* _ASM_SW64_IRQ_IMPL_H */

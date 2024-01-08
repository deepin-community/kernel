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

#endif /* _ASM_SW64_IRQ_H */

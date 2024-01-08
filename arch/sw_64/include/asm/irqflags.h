/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_IRQFLAGS_H
#define _ASM_SW64_IRQFLAGS_H

#include <asm/hmcall.h>

#define IPL_MIN		0
#define IPL_MAX		7

#define getipl()		(rdps() & 7)
#define setipl(ipl)		((void) swpipl(ipl))

static inline unsigned long arch_local_save_flags(void)
{
	return rdps();
}

static inline void arch_local_irq_disable(void)
{
	setipl(IPL_MAX);
	barrier();
}

static inline unsigned long arch_local_irq_save(void)
{
	unsigned long flags = swpipl(IPL_MAX);

	barrier();
	return flags;
}

static inline void arch_local_irq_enable(void)
{
	barrier();
	setipl(IPL_MIN);
}

static inline void arch_local_irq_restore(unsigned long flags)
{
	barrier();
	setipl(flags);
	barrier();
}

static inline bool arch_irqs_disabled_flags(unsigned long flags)
{
	return flags > IPL_MIN;
}

static inline bool arch_irqs_disabled(void)
{
	return arch_irqs_disabled_flags(getipl());
}

#endif /* _ASM_SW64_IRQFLAGS_H */

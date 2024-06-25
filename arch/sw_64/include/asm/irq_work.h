/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_SW64_IRQ_WORK_H
#define __ASM_SW64_IRQ_WORK_H

extern void arch_irq_work_raise(void);

static inline bool arch_irq_work_has_interrupt(void)
{
	return IS_ENABLED(CONFIG_SMP);
}

#endif /* __ASM_SW64_IRQ_WORK_H */

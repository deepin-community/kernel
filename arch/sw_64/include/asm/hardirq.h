/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_HARDIRQ_H
#define _ASM_SW64_HARDIRQ_H

void ack_bad_irq(unsigned int irq);
#define ack_bad_irq ack_bad_irq

#include <linux/irq.h>

#define __ARCH_IRQ_STAT
typedef struct {
	u16		__softirq_pending;
	unsigned int	timer_irqs_event;
} ____cacheline_aligned irq_cpustat_t;

DECLARE_PER_CPU_SHARED_ALIGNED(irq_cpustat_t, irq_stat);

#define inc_irq_stat(member)	this_cpu_inc(irq_stat.member)
#define arch_irq_stat_cpu	arch_irq_stat_cpu
#define arch_irq_stat		arch_irq_stat
extern u64 arch_irq_stat_cpu(unsigned int cpu);
extern u64 arch_irq_stat(void);

#endif /* _ASM_SW64_HARDIRQ_H */

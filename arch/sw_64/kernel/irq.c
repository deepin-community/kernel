// SPDX-License-Identifier: GPL-2.0
/*
 *	linux/arch/sw_64/kernel/irq.c
 *
 *	Copyright (C) 1995 Linus Torvalds
 *
 * This file contains the code used by various IRQ handling routines:
 * asking for different IRQ's should be done through these routines
 * instead of just grabbing them. Thus setups with different IRQ numbers
 * shouldn't result in any weird surprises, and installing new handlers
 * should be easier.
 */

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/seq_file.h>

volatile unsigned long irq_err_count;
DEFINE_PER_CPU(unsigned long, irq_pmi_count);
DEFINE_PER_CPU_SHARED_ALIGNED(irq_cpustat_t, irq_stat);
EXPORT_PER_CPU_SYMBOL(irq_stat);

void ack_bad_irq(unsigned int irq)
{
	irq_err_count++;
	pr_crit("Unexpected IRQ trap at vector %u\n", irq);
}

u64 arch_irq_stat_cpu(unsigned int cpu)
{
	u64 sum = per_cpu(irq_stat, cpu).timer_irqs_event;

	return sum;
}

u64 arch_irq_stat(void)
{
	return 0;
}

int arch_show_interrupts(struct seq_file *p, int prec)
{
	int j;

	seq_printf(p, "%*s: ", prec, "TIMER");
	for_each_online_cpu(j)
		seq_printf(p, "%10u", per_cpu(irq_stat, j).timer_irqs_event);
	seq_puts(p, "\n");

#ifdef CONFIG_SMP
	seq_printf(p, "%*s: ", prec, "IPI");
	for_each_online_cpu(j)
		seq_printf(p, "%10lu ", cpu_data[j].ipi_count);
	seq_puts(p, "\n");
#endif
	seq_printf(p, "%*s: ", prec, "PMI");
	for_each_online_cpu(j)
		seq_printf(p, "%10lu ", per_cpu(irq_pmi_count, j));
	seq_puts(p, "\n");

	seq_printf(p, "ERR: %10lu\n", irq_err_count);
	return 0;
}

/*
 * handle_irq handles all normal device IRQ's (the special
 * SMP cross-CPU interrupts have their own specific
 * handlers).
 */

#define MAX_ILLEGAL_IRQS 16

void
handle_irq(int irq)
{
	/*
	 * We ack quickly, we don't want the irq controller
	 * thinking we're snobs just because some other CPU has
	 * disabled global interrupts (we have already done the
	 * INT_ACK cycles, it's too late to try to pretend to the
	 * controller that we aren't taking the interrupt).
	 *
	 * 0 return value means that this irq is already being
	 * handled by some other CPU. (or is disabled)
	 */
	static unsigned int illegal_count;
	struct irq_desc *desc = irq_to_desc(irq);

	if (!desc || ((unsigned int) irq > ACTUAL_NR_IRQS &&
	    illegal_count < MAX_ILLEGAL_IRQS)) {
		irq_err_count++;
		illegal_count++;
		pr_crit("device_interrupt: invalid interrupt %d\n", irq);
		return;
	}

	irq_enter();
	generic_handle_irq_desc(desc);
	irq_exit();
}

#ifdef CONFIG_HOTPLUG_CPU
void fixup_irqs(void)
{
	irq_migrate_all_off_this_cpu();
}
#endif

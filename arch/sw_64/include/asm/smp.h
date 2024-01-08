/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_SMP_H
#define _ASM_SW64_SMP_H

#include <asm/core.h>
#include <asm/current.h>
#include <asm/hcall.h>
#include <asm/hmcall.h>
#include <asm/hw_init.h>

#include <linux/bitops.h>
#include <linux/cpumask.h>
#include <linux/threads.h>

/* HACK: Cabrio WHAMI return value is bogus if more than 8 bits used.. :-( */

extern cpumask_t core_start;

static inline unsigned long
read_vpcr(void)
{
	register unsigned long __r0 __asm__("$0");
	__asm__ __volatile__(
		"sys_call %1 #rvpcr"
		: "=r"(__r0)
		: "i" (0x39)
		: "$1", "$22", "$23", "$24", "$25");
	return __r0;
}

#ifdef CONFIG_SMP
/* SMP initialization hook for setup_arch */
void __init setup_smp(void);

#include <asm/irq.h>

/* smp reset control block */
struct smp_rcb_struct {
	void (*restart_entry)(unsigned long args);
	unsigned long restart_args;
	unsigned long ready;
	unsigned long init_done;
};

#define INIT_SMP_RCB ((struct smp_rcb_struct *) __va(0x820000UL))


#ifdef GENERATING_ASM_OFFSETS
#define raw_smp_processor_id() (0)
#else
#include <asm/asm-offsets.h>
#define raw_smp_processor_id() (*((unsigned int *)((void *)current + TASK_CPU)))
#endif
#define hard_smp_processor_id()	cpu_to_rcid(raw_smp_processor_id())

/* The map from sequential logical cpu number to hard cid.  */
extern int __cpu_to_rcid[NR_CPUS];
#define cpu_to_rcid(cpu)  __cpu_to_rcid[cpu]
#define cpu_physical_id(cpu)    __cpu_to_rcid[cpu]

extern unsigned long tidle_pcb[NR_CPUS];
extern void arch_send_call_function_single_ipi(int cpu);
extern void arch_send_call_function_ipi_mask(const struct cpumask *mask);

#ifdef CONFIG_HOTPLUG_CPU
int __cpu_disable(void);
void __cpu_die(unsigned int cpu);
#endif /* CONFIG_HOTPLUG_CPU */

#else /* CONFIG_SMP */
#define hard_smp_processor_id()		0
#define smp_call_function_on_cpu(func, info, wait, cpu)    ({ 0; })
/* The map from sequential logical cpu number to hard cid.  */
extern int __cpu_to_rcid[NR_CPUS];
#define cpu_to_rcid(cpu)  __cpu_to_rcid[0]
#define cpu_physical_id(cpu)    __cpu_to_rcid[0]
#endif /* CONFIG_SMP */

#define NO_PROC_ID	(-1)

static inline void send_ipi(int cpu, unsigned long type)
{
	int rcid;

	rcid = cpu_to_rcid(cpu);

	if (is_in_guest())
		hcall(HCALL_IVI, rcid, type, 0);
	else
		sendii(rcid, type, 0);
}

#define reset_cpu(cpu)  send_ipi((cpu), II_RESET)

#endif /* _ASM_SW64_SMP_H */

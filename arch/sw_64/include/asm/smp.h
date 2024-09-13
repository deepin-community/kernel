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

/* smp reset control block */
struct smp_rcb_struct {
	void (*restart_entry)(unsigned long args);
	unsigned long restart_args;
	unsigned long ready;
	unsigned long init_done;
};

extern bool __init is_rcid_duplicate(int rcid);

#ifdef CONFIG_SMP
/* SMP initialization hook for setup_arch */
void __init setup_smp(void);

#include <asm/irq.h>

#define INIT_SMP_RCB ((struct smp_rcb_struct *) __va(0x820000UL))
extern void __init smp_rcb_init(struct smp_rcb_struct *smp_rcb_base_addr);

#ifdef GENERATING_ASM_OFFSETS
#define raw_smp_processor_id() (0)
#else
#define raw_smp_processor_id() (current_thread_info()->cpu)
#endif
#define hard_smp_processor_id()	cpu_to_rcid(raw_smp_processor_id())

/* The map from sequential logical cpu number to hard cid.  */
extern int __cpu_to_rcid[NR_CPUS];
#define cpu_to_rcid(cpu)  __cpu_to_rcid[cpu]
#define cpu_physical_id(cpu)    __cpu_to_rcid[cpu]

static inline void set_rcid_map(unsigned int logical, int rcid)
{
	__cpu_to_rcid[logical] = rcid;
}

extern unsigned long tidle_pcb[NR_CPUS];
extern void arch_send_call_function_single_ipi(int cpu);
extern void arch_send_call_function_ipi_mask(const struct cpumask *mask);

#ifdef CONFIG_HOTPLUG_CPU
int __cpu_disable(void);
void __cpu_die(unsigned int cpu);
#endif /* CONFIG_HOTPLUG_CPU */

struct rcid_information {
	unsigned long thread_bits  : 8;    /* which thread */
	unsigned long thread_shift : 8;
	unsigned long core_bits    : 8;    /* which core */
	unsigned long core_shift   : 8;
	unsigned long domain_bits  : 8;    /* which node */
	unsigned long domain_shift : 8;
	unsigned long initialized  : 1;
};

extern struct rcid_information rcid_info;
extern void rcid_information_init(int core_version);

extern int get_core_id_from_rcid(int rcid);
extern int get_thread_id_from_rcid(int rcid);
extern int get_domain_id_from_rcid(int rcid);

#else /* CONFIG_SMP */

static inline void __init setup_smp(void) { store_cpu_data(0); }

#define hard_smp_processor_id()		0
#define smp_call_function_on_cpu(func, info, wait, cpu)    ({ 0; })
/* The map from sequential logical cpu number to hard cid.  */
extern int __cpu_to_rcid[NR_CPUS];
#define cpu_to_rcid(cpu)  __cpu_to_rcid[0]
#define cpu_physical_id(cpu)    __cpu_to_rcid[0]

static inline void smp_rcb_init(struct smp_rcb_struct *smp_rcb_base_addr) { }

static inline void set_rcid_map(unsigned int logical, int rcid)
{
	__cpu_to_rcid[0] = 0;
}

static inline void rcid_information_init(int core_version) { }
static inline int get_core_id_from_rcid(int rcid) { return 0; }
static inline int get_thread_id_from_rcid(int rcid) { return 0; }
static inline int get_domain_id_from_rcid(int rcid) { return 0; }

#endif /* CONFIG_SMP */

#define NO_PROC_ID	(-1)

static inline void send_ipi(int cpu, unsigned long type)
{
	int rcid;

	if (is_in_guest())
		hcall(HCALL_IVI, cpu, type, 0);
	else {
		rcid = cpu_to_rcid(cpu);
		sendii(rcid, type, 0);
	}
}

#define reset_cpu(cpu)  send_ipi((cpu), II_RESET)

#endif /* _ASM_SW64_SMP_H */

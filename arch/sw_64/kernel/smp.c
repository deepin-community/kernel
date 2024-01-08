// SPDX-License-Identifier: GPL-2.0
/*
 *	linux/arch/sw_64/kernel/smp.c
 */

#include <linux/errno.h>
#include <linux/sched/mm.h>
#include <linux/sched/hotplug.h>
#include <linux/smp.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/cpu.h>

#include <asm/mmu_context.h>
#include <asm/tlbflush.h>
#include <asm/sw64_init.h>
#include <asm/topology.h>
#include <asm/timer.h>
#include <asm/core.h>

#include "proto.h"

struct smp_rcb_struct *smp_rcb;

extern struct cpuinfo_sw64 cpu_data[NR_CPUS];

int smp_booted;

void *idle_task_pointer[NR_CPUS];

/* State of each CPU */
DEFINE_PER_CPU(int, cpu_state) = { 0 };

/* A collection of single bit ipi messages.  */
static struct {
	unsigned long bits ____cacheline_aligned;
} ipi_data[NR_CPUS] __cacheline_aligned;

enum ipi_message_type {
	IPI_RESCHEDULE,
	IPI_CALL_FUNC,
	IPI_CPU_STOP,
};

int smp_num_cpus = 1;		/* Number that came online.  */
EXPORT_SYMBOL(smp_num_cpus);

#define send_sleep_interrupt(cpu)	send_ipi((cpu), II_SLEEP)
#define send_wakeup_interrupt(cpu)	send_ipi((cpu), II_WAKE)

/*
 * Where secondaries begin a life of C.
 */
void smp_callin(void)
{
	int cpuid = smp_processor_id();

	local_irq_disable();

	if (cpu_online(cpuid)) {
		pr_err("??, cpu 0x%x already present??\n", cpuid);
		BUG();
	}
	set_cpu_online(cpuid, true);

	/* clear ksp, usp  */
	wrksp(0);
	wrusp(0);

	/* Set trap vectors.  */
	trap_init();

	/* Set interrupt vector.  */
	if (is_in_host()) {
		write_csr(0xffffffffffffffffUL, CSR_PCIE_MSI0_INTEN);
		write_csr(0xffffffffffffffffUL, CSR_PCIE_MSI1_INTEN);
		write_csr(0xffffffffffffffffUL, CSR_PCIE_MSI2_INTEN);
		write_csr(0xffffffffffffffffUL, CSR_PCIE_MSI3_INTEN);
	}
	wrent(entInt, 0);

	/* Get our local ticker going. */
	sw64_setup_timer();

	/* All kernel threads share the same mm context.  */
	mmgrab(&init_mm);
	current->active_mm = &init_mm;
	/* update csr:ptbr */
	update_ptbr_sys(virt_to_phys(init_mm.pgd));

	/* inform the notifiers about the new cpu */
	notify_cpu_starting(cpuid);

	per_cpu(cpu_state, cpuid) = CPU_ONLINE;
	per_cpu(hard_node_id, cpuid) = rcid_to_domain_id(cpu_to_rcid(cpuid));

	/* Must have completely accurate bogos.  */
	local_irq_enable();

	/* Cpu0 init preempt_count at start_kernel, other smp cpus do here. */
	preempt_disable();

	cpu_startup_entry(CPUHP_AP_ONLINE_IDLE);
}


/*
 * Set ready for secondary cpu.
 */
static inline void set_secondary_ready(int cpuid)
{
	smp_rcb->ready = cpuid;
}

/*
 * Convince the hmcode to have a secondary cpu begin execution.
 */
static int secondary_cpu_start(int cpuid, struct task_struct *idle)
{
	unsigned long timeout;
	/*
	 * Precalculate the target ksp.
	 */
	idle_task_pointer[cpuid] = idle;

	set_cpu_online(cpuid, false);
	wmb();

	set_secondary_ready(cpuid);

	/* Wait 10 seconds for secondary cpu.  */
	timeout = jiffies + 10*HZ;
	while (time_before(jiffies, timeout)) {
		if (cpu_online(cpuid))
			goto started;
		udelay(10);
		barrier();
	}
	pr_err("SMP: Processor %d failed to start.\n", cpuid);
	return -1;

started:
	store_cpu_topology(cpuid);
	numa_add_cpu(cpuid);
	return 0;
}

/*
 * Bring one cpu online.
 */
static int smp_boot_one_cpu(int cpuid, struct task_struct *idle)
{
	per_cpu(cpu_state, cpuid) = CPU_UP_PREPARE;

	return secondary_cpu_start(cpuid, idle);
}

static void __init process_nr_cpu_ids(void)
{
	int i;

	for (i = nr_cpu_ids; i < NR_CPUS; i++) {
		set_cpu_possible(i, false);
		set_cpu_present(i, false);
	}

	nr_cpu_ids = num_possible_cpus();
}

void __init smp_rcb_init(void)
{
	smp_rcb = INIT_SMP_RCB;
	memset(smp_rcb, 0, sizeof(struct smp_rcb_struct));
	/* Setup SMP_RCB fields that uses to activate secondary CPU */
	smp_rcb->restart_entry = __smp_callin;
	smp_rcb->init_done = 0xDEADBEEFUL;
	mb();
}

/*
 * Called from setup_arch.  Detect an SMP system and which processors
 * are present.
 */
void __init setup_smp(void)
{
	int i = 0, num = 0;

	init_cpu_possible(cpu_none_mask);

	/* For unified kernel, NR_CPUS is the maximum possible value */
	for (; i < NR_CPUS; i++) {
		if (cpu_to_rcid(i) != -1) {
			set_cpu_possible(num, true);
			store_cpu_data(num);
			if (!cpumask_test_cpu(i, &cpu_offline))
				set_cpu_present(num, true);
			num++;
		}
	}

	process_nr_cpu_ids();

	pr_info("Detected %u possible CPU(s), %u CPU(s) are present\n",
			nr_cpu_ids, num_present_cpus());

	smp_rcb_init();
}
/*
 * Called by smp_init prepare the secondaries
 */
void __init smp_prepare_cpus(unsigned int max_cpus)
{
	unsigned int cpu;
	/* Take care of some initial bookkeeping.  */
	memset(ipi_data, 0, sizeof(ipi_data));

	init_cpu_topology();
	store_cpu_topology(smp_processor_id());
	numa_add_cpu(smp_processor_id());

	for_each_possible_cpu(cpu) {
		numa_store_cpu_info(cpu);
	}

	/* Nothing to do on a UP box, or when told not to.  */
	if (nr_cpu_ids == 1 || max_cpus == 0) {
		init_cpu_possible(cpumask_of(0));
		init_cpu_present(cpumask_of(0));
		pr_info("SMP mode deactivated.\n");
		return;
	}

	pr_info("SMP starting up secondaries.\n");
}

void smp_prepare_boot_cpu(void)
{
	int me = smp_processor_id();

	per_cpu(cpu_state, me) = CPU_ONLINE;
}

int vt_cpu_up(unsigned int cpu, struct task_struct *tidle)
{
	pr_info("%s: cpu = %d\n", __func__, cpu);

	wmb();
	smp_rcb->ready = 0;
	if (smp_booted) {
		/* irq must be disabled before reset vCPU */
		reset_cpu(cpu);
	}
	smp_boot_one_cpu(cpu, tidle);

	return cpu_online(cpu) ? 0 : -EIO;
}

#ifdef CONFIG_SUBARCH_C3B
DECLARE_STATIC_KEY_FALSE(use_tc_as_sched_clock);
#endif

int __cpu_up(unsigned int cpu, struct task_struct *tidle)
{
	if (is_in_guest())
		return vt_cpu_up(cpu, tidle);

	wmb();
	smp_rcb->ready = 0;

	/* send wake up signal */
	send_wakeup_interrupt(cpu);
	/* send reset signal */
	if (smp_booted) {
		if (is_in_host()) {
			reset_cpu(cpu);
		} else {
			while (1)
				cpu_relax();
		}
	}
	smp_boot_one_cpu(cpu, tidle);

#ifdef CONFIG_SUBARCH_C3B
	if (static_branch_likely(&use_tc_as_sched_clock)) {
		if (smp_booted) {
			tc_sync_clear();
			smp_call_function_single(cpu, tc_sync_ready, NULL, 0);
			tc_sync_set();
		}
	}
#endif

	return cpu_online(cpu) ? 0 : -EIO;
}

void __init smp_cpus_done(unsigned int max_cpus)
{
	smp_booted = 1;
	pr_info("SMP: Total of %d processors activated.\n", num_online_cpus());
}

int setup_profiling_timer(unsigned int multiplier)
{
	return -EINVAL;
}


static void send_ipi_message(const struct cpumask *to_whom, enum ipi_message_type operation)
{
	int i;

	mb();
	for_each_cpu(i, to_whom)
		set_bit(operation, &ipi_data[i].bits);

	mb();
	for_each_cpu(i, to_whom)
		send_ipi(i, II_II0);
}

static void ipi_cpu_stop(int cpu)
{
	local_irq_disable();
	set_cpu_online(cpu, false);
	while (1)
		wait_for_interrupt();
}

void handle_ipi(struct pt_regs *regs)
{
	int cpu = smp_processor_id();
	unsigned long *pending_ipis = &ipi_data[cpu].bits;
	unsigned long ops;

	mb();	/* Order interrupt and bit testing. */
	while ((ops = xchg(pending_ipis, 0)) != 0) {
		mb();	/* Order bit clearing and data access. */
		do {
			unsigned long which;

			which = ops & -ops;
			ops &= ~which;
			which = __ffs(which);

			switch (which) {
			case IPI_RESCHEDULE:
				scheduler_ipi();
				break;

			case IPI_CALL_FUNC:
				irq_enter();
				generic_smp_call_function_interrupt();
				irq_exit();
				break;

			case IPI_CPU_STOP:
				ipi_cpu_stop(cpu);
				break;

			default:
				pr_crit("Unknown IPI on CPU %d: %lu\n", cpu, which);
				break;
			}
		} while (ops);

		mb();	/* Order data access and bit testing. */
	}

	cpu_data[cpu].ipi_count++;
}

void arch_smp_send_reschedule(int cpu)
{
	send_ipi_message(cpumask_of(cpu), IPI_RESCHEDULE);
}
EXPORT_SYMBOL(arch_smp_send_reschedule);

void smp_send_stop(void)
{
	unsigned long timeout;

	if (num_online_cpus() > 1) {
		cpumask_t mask;

		cpumask_copy(&mask, cpu_online_mask);
		cpumask_clear_cpu(smp_processor_id(), &mask);

		if (system_state <= SYSTEM_RUNNING)
			pr_crit("SMP: stopping secondary CPUs\n");
		send_ipi_message(&mask, IPI_CPU_STOP);
	}

	/* Wait up to one second for other CPUs to stop */
	timeout = USEC_PER_SEC;
	while (num_online_cpus() > 1 && timeout--)
		udelay(1);

	if (num_online_cpus() > 1)
		pr_warn("SMP: failed to stop secondary CPUs %*pbl\n",
				cpumask_pr_args(cpu_online_mask));
}

void arch_send_call_function_ipi_mask(const struct cpumask *mask)
{
	send_ipi_message(mask, IPI_CALL_FUNC);
}

void arch_send_call_function_single_ipi(int cpu)
{
	send_ipi_message(cpumask_of(cpu), IPI_CALL_FUNC);
}

static void ipi_flush_tlb_all(void *ignored)
{
	local_flush_tlb_all();
}

void flush_tlb_all(void)
{
	/* Although we don't have any data to pass, we do want to
	 * synchronize with the other processors.
	 */
	on_each_cpu(ipi_flush_tlb_all, NULL, 1);
}

static void ipi_flush_tlb_mm(void *x)
{
	local_flush_tlb_mm((struct mm_struct *)x);
}

void flush_tlb_mm(struct mm_struct *mm)
{

	/* happens as a result of exit_mmap()
	 * Shall we clear mm->context.asid[] here?
	 */
	if (atomic_read(&mm->mm_users) == 0)
		return;

	preempt_disable();

	if (atomic_read(&mm->mm_users) != 1 || mm != current->mm) {
		on_each_cpu_mask(mm_cpumask(mm), ipi_flush_tlb_mm, mm, 1);
	} else {
		int cpu, this_cpu = smp_processor_id();

		for_each_online_cpu(cpu) {
			if (cpu != this_cpu && mm->context.asid[cpu])
				mm->context.asid[cpu] = 0;
		}
		local_flush_tlb_mm(mm);
	}

	preempt_enable();
}
EXPORT_SYMBOL(flush_tlb_mm);

struct flush_tlb_info {
	struct vm_area_struct *vma;
	unsigned long addr;
#define start addr
	unsigned long end;
};

static void ipi_flush_tlb_page(void *x)
{
	struct flush_tlb_info *info = x;

	local_flush_tlb_page(info->vma, info->addr);
}

void flush_tlb_page(struct vm_area_struct *vma, unsigned long addr)
{
	struct mm_struct *mm = vma->vm_mm;

	preempt_disable();

	if (atomic_read(&mm->mm_users) != 1 || mm != current->mm) {
		struct flush_tlb_info info = {
			.vma = vma,
			.addr = addr,
		};
		on_each_cpu_mask(mm_cpumask(mm), ipi_flush_tlb_page, &info, 1);
	} else {
		int cpu, this_cpu = smp_processor_id();

		for_each_online_cpu(cpu) {
			if (cpu != this_cpu && mm->context.asid[cpu])
				mm->context.asid[cpu] = 0;
		}
		local_flush_tlb_page(vma, addr);
	}

	preempt_enable();
}
EXPORT_SYMBOL(flush_tlb_page);

/* It always flush the whole user tlb by now. To be optimized. */
void flush_tlb_range(struct vm_area_struct *vma, unsigned long start, unsigned long end)
{
	flush_tlb_mm(vma->vm_mm);
}
EXPORT_SYMBOL(flush_tlb_range);

static void ipi_flush_tlb_kernel_range(void *x)
{
	struct flush_tlb_info *info = x;

	local_flush_tlb_kernel_range(info->start, info->end);
}

void flush_tlb_kernel_range(unsigned long start, unsigned long end)
{
	struct flush_tlb_info info = {
		.start = start,
		.end = end,
	};

	on_each_cpu(ipi_flush_tlb_kernel_range, &info, 1);
}
EXPORT_SYMBOL(flush_tlb_kernel_range);

#ifdef CONFIG_HOTPLUG_CPU
int __cpu_disable(void)
{
	int cpu = smp_processor_id();

	set_cpu_online(cpu, false);
	remove_cpu_topology(cpu);
	numa_remove_cpu(cpu);
	clear_tasks_mm_cpumask(cpu);
	return 0;
}

void __cpu_die(unsigned int cpu)
{
	/* We don't do anything here: idle task is faking death itself. */
	unsigned int i;

	for (i = 0; i < 10; i++) {
		/* They ack this in play_dead by setting CPU_DEAD */
		if (per_cpu(cpu_state, cpu) == CPU_DEAD) {
			if (system_state == SYSTEM_RUNNING)
				pr_info("CPU %u is now offline\n", cpu);
			smp_rcb->ready = 0;
			return;
		}
		msleep(100);
	}
	pr_err("CPU %u didn't die...\n", cpu);
}

void arch_cpu_idle_dead(void)
{
	idle_task_exit();
	mb();
	__this_cpu_write(cpu_state, CPU_DEAD);
	fixup_irqs();
	local_irq_disable();

	if (is_in_guest()) {
		hcall(HCALL_SET_CLOCKEVENT, 0, 0, 0);
		hcall(HCALL_STOP, 0, 0, 0);
	} else {
		wrtimer(0);
	}

#ifdef CONFIG_SUSPEND
	sleepen();
	send_sleep_interrupt(smp_processor_id());
	while (1)
		asm("nop");
#else
	asm volatile("memb");
	asm volatile("halt");
#endif
}
#endif

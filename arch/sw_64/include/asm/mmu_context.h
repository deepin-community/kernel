/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_MMU_CONTEXT_H
#define _ASM_SW64_MMU_CONTEXT_H

#include <linux/mm_types.h>

#include <asm/compiler.h>
#include <asm/io.h>

/*
 * The maximum ASID's the processor supports.
 */

#if defined(CONFIG_SUBARCH_C3B) || defined(CONFIG_SUBARCH_C4)
#define ASID_BITS	10
#endif

#include <asm/cpu.h>
#define last_asid(cpu)		(cpu_data[cpu].last_asid)

#define ASID_FIRST_VERSION	(1UL << ASID_BITS)
#define ASID_MASK		((1UL << ASID_BITS) - 1)

#define cpu_asid(cpu, mm)	((mm)->context.asid[cpu] & ASID_MASK)

static inline bool asid_valid(struct mm_struct *mm, unsigned int cpu)
{
	return !((mm->context.asid[cpu] ^ last_asid(cpu)) & ~ASID_MASK);
}

/*
 * NOTE! The way this is set up, the high bits of the "last_asid" (and
 * the "mm->context.asid[cpu]") are the ASID _version_ code. A version
 * of 0 is always considered invalid, so to invalidate another process
 * you only need to do "p->mm->context.asid[cpu] = 0".
 *
 * If we need more ASID's than the processor has, we invalidate the old
 * user TLB's (tbivp()) and start a new ASID version. That will force a
 * new asid for any other processes the next time they want to run.
 */

static inline void __get_new_mm_context(struct mm_struct *mm, long cpu)
{
	unsigned long ptbr;
	unsigned long asid = last_asid(cpu);

	if (!(++asid & ASID_MASK)) {
		ptbr = virt_to_pfn(mm->pgd);
		wrap_asid(asid, ptbr);
	}
	mm->context.asid[cpu] = last_asid(cpu) = asid;

}

static inline void
switch_mm_irqs_off(struct mm_struct *prev_mm, struct mm_struct *next_mm,
		   struct task_struct *next)
{
	/* Check if our ASID is of an older version, and thus invalid. */
	unsigned long asid, ptbr;
	long cpu = smp_processor_id();

	if (!asid_valid(next_mm, cpu))
		__get_new_mm_context(next_mm, cpu);

	/* Update CSR:UPN and CSR:PTBR. Another thread may have allocated
	 * a new mm->context[asid] (via flush_tlb_mm) without the ASID serial
	 * number wrapping.  We have no way to detect when this is needed.
	 */
	asid = cpu_asid(cpu, next_mm);
	ptbr = virt_to_pfn(next_mm->pgd);
	load_mm(asid, ptbr);
	cpumask_set_cpu(cpu, mm_cpumask(next_mm));
}

#define switch_mm_irqs_off switch_mm_irqs_off

static inline void
switch_mm(struct mm_struct *prev_mm, struct mm_struct *next_mm,
	  struct task_struct *tsk)
{
	unsigned long flags;

	local_irq_save(flags);
	switch_mm_irqs_off(prev_mm, next_mm, tsk);
	local_irq_restore(flags);
}

#define activate_mm(prev, next) switch_mm(prev, next, current)
#define deactivate_mm(tsk, mm)	do { } while (0)

static inline int init_new_context(struct task_struct *tsk,
				   struct mm_struct *mm)
{
	int i;

	for_each_possible_cpu(i)
		mm->context.asid[i] = 0;
	return 0;
}

static inline void destroy_context(struct mm_struct *mm)
{
	/* Nothing to do.  */
}

static inline void enter_lazy_tlb(struct mm_struct *mm,
				  struct task_struct *tsk)
{
}

static inline int arch_dup_mmap(struct mm_struct *oldmm,
				struct mm_struct *mm)
{
	return 0;
}

static inline void arch_exit_mmap(struct mm_struct *mm)
{
}

static inline void arch_unmap(struct mm_struct *mm, unsigned long start,
				unsigned long end)
{
}

static inline void arch_bprm_mm_init(struct mm_struct *mm,
				     struct vm_area_struct *vma)
{
}

static inline bool arch_vma_access_permitted(struct vm_area_struct *vma,
					     bool write, bool execute,
					     bool foreign)
{
	/* by default, allow everything */
	return true;
}
#endif /* _ASM_SW64_MMU_CONTEXT_H */

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_TLBFLUSH_H
#define _ASM_SW64_TLBFLUSH_H

#include <linux/mm.h>
#include <linux/sched.h>
#include <asm/compiler.h>
#include <asm/pgalloc.h>
#include <asm/hw_init.h>
#include <asm/hmcall.h>
#include <asm/mmu_context.h>

static inline void local_flush_tlb_all(void)
{
	tbiv();
}

static inline void local_flush_tlb_mm(struct mm_struct *mm)
{
	int cpu;
	unsigned long flags;

	local_irq_save(flags);

	cpu = smp_processor_id();
	if (!asid_valid(mm, cpu)) {
		cpumask_clear_cpu(cpu, mm_cpumask(mm));
		goto out;
	}

	if (current->mm == mm) {
		__get_new_mm_context(mm, cpu);
		wrasid(cpu_asid(cpu, mm));
	} else {
		mm->context.asid[cpu] = 0;
		cpumask_clear_cpu(cpu, mm_cpumask(mm));
	}
out:
	local_irq_restore(flags);
}

static inline void
local_flush_tlb_page(struct vm_area_struct *vma, unsigned long addr)
{
	int cpu;
	struct mm_struct *mm;

	cpu = smp_processor_id();
	mm = vma->vm_mm;

	if (asid_valid(mm, cpu))
		tbisasid(cpu_asid(cpu, mm), addr);
	else
		cpumask_clear_cpu(cpu, mm_cpumask(mm));
}

/*
 * It flushes the whole user tlb now.
 */
static inline void
local_flush_tlb_range(struct vm_area_struct *vma,
		unsigned long start, unsigned long end)
{
	local_flush_tlb_mm(vma->vm_mm);
}

/*
 * There is no way to invalidate kernel pages only, so it has to
 * inlvalidate all mapping.
 */
static inline void
local_flush_tlb_kernel_range(unsigned long start, unsigned long end)
{
	local_flush_tlb_all();
}


#ifdef CONFIG_SMP
extern void flush_tlb_all(void);
extern void flush_tlb_mm(struct mm_struct *mm);
extern void flush_tlb_page(struct vm_area_struct *vma, unsigned long addr);
extern void flush_tlb_range(struct vm_area_struct *vma, unsigned long start,
		unsigned long end);
extern void flush_tlb_kernel_range(unsigned long start, unsigned long end);
#else
#define flush_tlb_all()				local_flush_tlb_all()
#define flush_tlb_mm(mm)			local_flush_tlb_mm(mm)
#define flush_tlb_page(vma, addr)		local_flush_tlb_page(vma, addr)
#define flush_tlb_range(vma, start, end)	local_flush_tlb_range(vma, start, end)
#define flush_tlb_kernel_range(start, end)	local_flush_tlb_kernel_range(start, end)

#endif /* CONFIG_SMP */

#endif /* _ASM_SW64_TLBFLUSH_H */

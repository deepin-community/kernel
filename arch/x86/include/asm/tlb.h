/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_TLB_H
#define _ASM_X86_TLB_H

#define tlb_flush tlb_flush
static inline void tlb_flush(struct mmu_gather *tlb);

#include <asm-generic/tlb.h>

static inline void tlb_flush(struct mmu_gather *tlb)
{
	unsigned long start = 0UL, end = TLB_FLUSH_ALL;
	unsigned int stride_shift = tlb_get_unmap_shift(tlb);

	if (!tlb->fullmm && !tlb->need_flush_all) {
		start = tlb->start;
		end = tlb->end;
	}

	flush_tlb_mm_range(tlb->mm, start, end, stride_shift, tlb->freed_tables);
}

/*
 * While x86 architecture in general requires an IPI to perform TLB
 * shootdown, enablement code for several hypervisors overrides
 * .flush_tlb_others hook in pv_mmu_ops and implements it by issuing
 * a hypercall. To keep software pagetable walkers safe in this case we
 * switch to RCU based table free (MMU_GATHER_RCU_TABLE_FREE). See the comment
 * below 'ifdef CONFIG_MMU_GATHER_RCU_TABLE_FREE' in include/asm-generic/tlb.h
 * for more details.
 */
static inline void __tlb_remove_table(void *table)
{
	free_page_and_swap_cache(table);
}

static inline void invlpg(unsigned long addr)
{
	asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}

#ifdef CONFIG_PTP
#include <linux/ptp-cache.h>
static inline void __ptp_tlb_remove_table(void *table)
{
	iee_cache_free(&pg_cache, page_to_virt((struct page *)table));
}
#endif

#endif /* _ASM_X86_TLB_H */

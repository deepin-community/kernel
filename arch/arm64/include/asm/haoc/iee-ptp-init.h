/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IEE_PTP_INIT_H
#define _LINUX_IEE_PTP_INIT_H

#include <asm/kernel-pgtable.h>

#define NR_BM_PTE_TABLES \
	SPAN_NR_ENTRIES(FIXADDR_TOT_START, FIXADDR_TOP, PMD_SHIFT)
#define NR_BM_PMD_TABLES \
	SPAN_NR_ENTRIES(FIXADDR_TOT_START, FIXADDR_TOP, PUD_SHIFT)

static_assert(NR_BM_PMD_TABLES == 1);

#define __BM_TABLE_IDX(addr, shift) \
	(((addr) >> (shift)) - (FIXADDR_TOT_START >> (shift)))

#define BM_PTE_TABLE_IDX(addr)	__BM_TABLE_IDX(addr, PMD_SHIFT)

extern pte_t bm_pte[NR_BM_PTE_TABLES][PTRS_PER_PTE] __section(".iee.ptp") __aligned(PAGE_SIZE);
extern pmd_t bm_pmd[PTRS_PER_PMD] __section(".iee.ptp") __aligned(PAGE_SIZE) __maybe_unused;
extern pud_t bm_pud[PTRS_PER_PUD] __section(".iee.ptp") __aligned(PAGE_SIZE) __maybe_unused;

extern void __iee_pgd_populate_pre_init(pgd_t *pgdp, phys_addr_t p4dp, pgdval_t prot);
extern void __iee_p4d_populate_pre_init(p4d_t *p4dp, phys_addr_t pudp, p4dval_t prot);
extern void __iee_pud_populate_pre_init(pud_t *pudp, phys_addr_t pmdp, pudval_t prot);
extern void __iee_pmd_populate_pre_init(pmd_t *pmdp, phys_addr_t ptep,
				  pmdval_t prot);
extern int iee_pud_set_huge_pre_init(pud_t *pudp, phys_addr_t phys, pgprot_t prot);
extern int iee_pmd_set_huge_pre_init(pmd_t *pmdp, phys_addr_t phys, pgprot_t prot);

extern void __iee_set_fixmap_pre_init(enum fixed_addresses idx,
			       phys_addr_t phys, pgprot_t flags);

extern void iee_set_pgtable_pre_init(unsigned long *addr, unsigned long content);
extern void set_iee_address_pre_init(unsigned long addr, bool valid);
extern void iee_alloc_init_pud_pre_init(pgd_t *pgdp, unsigned long addr, unsigned long end,
			   phys_addr_t phys, pgprot_t prot,
			   phys_addr_t (*pgtable_alloc)(int),
			   int flags);
extern void __create_pgd_mapping_pre_init(pgd_t *pgdir, phys_addr_t phys,
				 unsigned long virt, phys_addr_t size,
				 pgprot_t prot,
				 phys_addr_t (*pgtable_alloc)(int),
				 int flags);

extern void __init efi_memmap_unmap_after_init(void);

extern int early_ioremap_debug __initdata;
extern int after_paging_init __initdata;
extern void __iomem *prev_map[FIX_BTMAPS_SLOTS] __initdata;
extern unsigned long prev_size[FIX_BTMAPS_SLOTS] __initdata;
extern unsigned long slot_virt[FIX_BTMAPS_SLOTS] __initdata;

extern bool pgattr_change_is_safe(u64 old, u64 new);

#endif

// SPDX-License-Identifier: GPL-2.0
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Lyu Jinglin <lvjl2022@zgclab.edu.cn>
 *          Zhang Shiyang <zhangsy2023@zgclab.edu.cn>
 */

#include <asm/haoc/haoc-def.h>
#include <asm/haoc/haoc-bitmap.h>
#include <asm/haoc/iee-token.h>
#include <asm/haoc/iee.h>
#include <asm/tlbflush.h>
#include <asm/pgalloc.h>
#include <asm/ptrace.h>
#include <asm/system_misc.h>
#include <asm/daifflags.h>
#include "slab.h"

static inline bool iee_support_pmd_block(unsigned long addr, unsigned int order)
{
	u64 end = addr + (PAGE_SIZE << order);

	return ((addr | end) & ~PMD_MASK) == 0;
}

static inline bool iee_support_cont_pte(unsigned long addr, unsigned int order)
{
	u64 end = addr + (PAGE_SIZE << order);

	return ((addr | end) & ~CONT_PTE_MASK) == 0;
}

/* Would clear continuous bits or split pmd block descriptors if needed. */
static void iee_may_split_pmd(pud_t *pudp, unsigned long addr, unsigned int order)
{
	pmd_t *pmdp = pmd_offset(pudp, addr);

	if (!pmd_leaf(*pmdp))
		return;

	/* Handling cont mapping for pmd blocks. */
	if ((pmd_val(*pmdp) & PTE_CONT)) {
		/* Get the beginning of cont mapping. */
		pmd_t *cont_pmdp = pmd_offset(pudp, addr & CONT_PMD_MASK);

		for (int i = 0; i < CONT_PMDS; i++) {
			set_pmd(cont_pmdp, __pmd(pmd_val(*cont_pmdp) & ~PTE_CONT));
			cont_pmdp++;
		}
	}

	/* May split Block Descriptor. */
	if (!iee_support_pmd_block(addr, order)) {
		struct page *page = pmd_page(*pmdp);
		pte_t *pgtable = pte_alloc_one_kernel(&init_mm);

		if (!pgtable)
			panic("Alloc pgtable error.\n");
		else {
			/* Iterate on the new page table. */
			pte_t *ptep = pgtable;

			/* Try to support continuous mappings on pte. */
			for (int i = 0; i < PTRS_PER_PMD; i++, ptep++) {
				pte_t entry;
				pgprot_t pgprot = PAGE_KERNEL;

				pgprot = __pgprot(pgprot_val(pgprot) | PTE_CONT);

				entry = mk_pte(page + i, pgprot);
				__set_pte(ptep, entry);
			}
		}

		/* Ensure that this pmd hasn't be splited by other threads. */
		spinlock_t *ptl = pmd_lock(&init_mm, pmdp);

		if (pmd_leaf(READ_ONCE(*pmdp))) {
			/* for sync. */
			smp_wmb();
			pmd_populate_kernel(&init_mm, pmdp, pgtable);
			pgtable = NULL;
		}
		spin_unlock(ptl);

		if (pgtable)
			pte_free_kernel(&init_mm, pgtable);
	}
}

/*
 * Used to enforce or remove RO protection linear addresses of IEE objects.
 * Not handling block descriptors except pmd blocks when change page tables,
 * so DO NOT use larger block on kernel linear mappings.
 */
void iee_set_logical_mem(unsigned long addr, unsigned int order, bool prot)
{
	pgd_t *pgdir = swapper_pg_dir;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, addr);
	p4d_t *p4dp = p4d_offset(pgdp, addr);
	pud_t *pudp = pud_offset(p4dp, addr);
	pmd_t *pmdp;

	/* Split pmd block if needed. */
	iee_may_split_pmd(pudp, addr, order);

	pmdp = pmd_offset(pudp, addr);

	if (pmd_leaf(READ_ONCE(*pmdp))) {
		/* Only permits writing single pmd block right now. */
		if (order != (PMD_SHIFT - PAGE_SHIFT))
			panic("%s: error on linear mappings.", __func__);

		pmd_t pmd = READ_ONCE(*pmdp);

		if (prot)
			pmd = __pmd((pmd_val(pmd) | PMD_SECT_RDONLY) & ~PTE_DBM);
		else
			pmd = __pmd(pmd_val(pmd) & ~PMD_SECT_RDONLY);
		set_pmd(pmdp, pmd);
	} else {
		pte_t *ptep = pte_offset_kernel(pmdp, addr);
		pte_t pte;

		/* Protect addresses one by one on this pte table.*/
		for (int i = 0; i < (1UL << order); i++) {
			/* Clear continuous bits first. */
			if (pte_val(__ptep_get(ptep)) & PTE_CONT &&
					!iee_support_cont_pte(addr, order)) {
				pte_t *cont_ptep = pte_offset_kernel(pmdp, addr & CONT_PTE_MASK);

				for (int j = 0; j < CONT_PTES; j++) {
					pte_t cont_pte = __ptep_get(cont_ptep);

					__set_pte(cont_ptep,
						  __pte(pte_val(cont_pte) & ~PTE_CONT));
					cont_ptep++;
				}
			}

			pte = __ptep_get(ptep);
			if (prot)
				pte = __pte((pte_val(pte) | PTE_RDONLY) & ~PTE_DBM);
			else
				pte = __pte(pte_val(pte) & ~PTE_RDONLY);
			__set_pte(ptep, pte);

			ptep++;
		}
	}
}

void iee_set_logical_mem_ro(unsigned long addr)
{
	iee_set_logical_mem(addr, 0, true);
	__flush_tlb_kernel_pgtable(addr);
	isb();
}

void iee_set_logical_mem_rw(unsigned long addr)
{
	iee_set_logical_mem(addr, 0, false);
	__flush_tlb_kernel_pgtable(addr);
	isb();
}

/*
 * IEE addresses are mapped in page granule so we only need to valid or invalid
 * the pte entries of these mappings to mark whether a physical page is inside IEE.
 */
void set_iee_address(unsigned long addr, unsigned int order, bool valid)
{
	pgd_t *pgdir = swapper_pg_dir;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, addr);
	p4d_t *p4dp = p4d_offset(pgdp, addr);
	pud_t *pudp = pud_offset(p4dp, addr);
	pmd_t *pmdp = pmd_offset(pudp, addr);
	pte_t *ptep = pte_offset_kernel(pmdp, addr);
	unsigned long end_addr = addr + (PAGE_SIZE << order);

	if ((addr < (PAGE_OFFSET + IEE_OFFSET)) |
			(addr > (PAGE_OFFSET + BIT(vabits_actual - 1)))) {
		pr_err("IEE: Invalid address to valid in IEE.");
		return;
	}

	if (addr != ALIGN(addr, PAGE_SIZE)
			|| end_addr > ALIGN(addr + 1, PMD_SIZE))
		panic("%s: invalid input address range 0x%lx-0x%lx.", __func__,
				addr, end_addr);

	for (int i = 0; i < (1UL << order); i++) {
		pte_t pte = __ptep_get(ptep);

		if (valid)
			pte = __pte(pte_val(pte) | PTE_VALID);
		else
			pte = __pte(pte_val(pte) & ~PTE_VALID);

		__set_pte(ptep, pte);
		ptep++;
	}
}

// TODO: Delete when allocate page table from pool
void set_iee_address_valid(unsigned long lm_addr, unsigned int order)
{
	set_iee_address(__virt_to_iee(lm_addr), order, true);
}

void set_iee_address_invalid(unsigned long lm_addr, unsigned int order)
{
	set_iee_address(__virt_to_iee(lm_addr), order, false);
}
// TODO END

/* Modify linear and IEE mappings of each address at the same time to avoid
 * synchronization problems.
 */
static void iee_set_sensitive_pte(pte_t *lm_ptep, pte_t *iee_ptep, int order,
				int use_block_pmd)
{
	int i;

	if (use_block_pmd) {
		pmd_t pmd = __pmd(pte_val(__ptep_get(lm_ptep)));

		pmd = __pmd((pmd_val(pmd) | PMD_SECT_RDONLY) & ~PTE_DBM);
		WRITE_ONCE(*lm_ptep, __pte(pmd_val(pmd)));
		for (i = 0; i < (1 << order); i++) {
			pte_t pte = __ptep_get(iee_ptep);

			pte = __pte(pte_val(pte) | PTE_VALID);
			WRITE_ONCE(*iee_ptep, pte);
			iee_ptep++;
		}
	} else {
		for (i = 0; i < (1 << order); i++) {
			pte_t pte = __ptep_get(lm_ptep);

			pte = __pte((pte_val(pte) | PTE_RDONLY) & ~PTE_DBM);
			WRITE_ONCE(*lm_ptep, pte);
			pte = __ptep_get(iee_ptep);
			pte = __pte(pte_val(pte) | PTE_VALID);
			WRITE_ONCE(*iee_ptep, pte);
			lm_ptep++;
			iee_ptep++;
		}
	}
	dsb(ishst);
	isb();
}

static void iee_unset_sensitive_pte(pte_t *lm_ptep, pte_t *iee_ptep, int order, int use_block_pmd)
{
	int i;

	if (use_block_pmd) {
		pmd_t pmd = __pmd(pte_val(__ptep_get(lm_ptep)));

		pmd = __pmd(pmd_val(pmd) | PTE_DBM);
		WRITE_ONCE(*lm_ptep, __pte(pmd_val(pmd)));
		for (i = 0; i < (1 << order); i++) {
			pte_t pte = __ptep_get(iee_ptep);

			pte = __pte(pte_val(pte) & ~PTE_VALID);
			WRITE_ONCE(*iee_ptep, pte);
			iee_ptep++;
		}
	} else {
		for (i = 0; i < (1 << order); i++) {
			pte_t pte = __ptep_get(lm_ptep);

			pte = __pte(pte_val(pte) | PTE_DBM);
			WRITE_ONCE(*lm_ptep, pte);
			pte = __ptep_get(iee_ptep);
			pte = __pte(pte_val(pte) & ~PTE_VALID);
			WRITE_ONCE(*iee_ptep, pte);
			lm_ptep++;
			iee_ptep++;
		}
	}
	dsb(ishst);
	isb();
}

/* Only support address range smaller then one PMD block. */
void put_pages_into_iee_block(unsigned long addr, int order)
{
	pgd_t *pgdir = swapper_pg_dir;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, addr);
	p4d_t *p4dp = p4d_offset(pgdp, addr);
	pud_t *pudp = pud_offset(p4dp, addr);
	pmd_t *pmdp;
	pte_t *lm_ptep;
	pte_t *iee_ptep;
	unsigned long iee_addr;
	int use_block_pmd = 0;

	/* Split pmd block if needed. */
	iee_may_split_pmd(pudp, addr, order);

	pmdp = pmd_offset(pudp, addr);

	use_block_pmd = pmd_leaf(READ_ONCE(*pmdp));

	if (use_block_pmd)
		lm_ptep = (pte_t *)pmdp;
	else
		lm_ptep = pte_offset_kernel(pmdp, addr);

	// Handling cont mapping.
	if (((1 << order) < CONT_PTES) &&
			(pte_val(__ptep_get(lm_ptep)) & PTE_CONT)) {
		// The beginning of cont mapping.
		int i;
		pte_t *ptep = pte_offset_kernel(pmdp, addr & CONT_PTE_MASK);

		if (order < CONFIG_ARM64_CONT_PTE_SHIFT) {
			for (i = 0; i < CONT_PTES; i++) {
				pte_t pte = __ptep_get(ptep);

				__set_pte(ptep, __pte(pte_val(pte) & ~PTE_CONT));
				ptep++;
			}
		}
	}

	iee_addr = ((unsigned long)addr | IEE_OFFSET);
	pgdp = pgd_offset_pgd(pgdir, iee_addr);
	p4dp = p4d_offset(pgdp, iee_addr);
	pudp = pud_offset(p4dp, iee_addr);
	pmdp = pmd_offset(pudp, iee_addr);
	iee_ptep = pte_offset_kernel(pmdp, iee_addr);
	/* Valid the IEE mappings of these pages to enable IEE access. */
	iee_set_sensitive_pte(lm_ptep, iee_ptep, order, use_block_pmd);
	flush_tlb_kernel_range(addr, addr+PAGE_SIZE*(1 << order));
	isb();
}

/*
 * Put the given pages into IEE by enforcing RO protection of their linear
 * mappings and setting the IEE addresses valid.
 *
 * @addr: The start linear address of pages to be protected.
 * @order: The effected address size would be (1 << order) pages.
 */
void put_pages_into_iee(unsigned long addr, int order)
{
	unsigned long end = addr + (PAGE_SIZE << order);

	if (addr & IEE_OFFSET)
		return;

	/* Split the address range if needed. */
	if (order < IEE_DATA_ORDER)
		put_pages_into_iee_block(addr, order);
	else {
		if (addr != ALIGN(addr, PMD_SIZE))
			panic("IEE: Invalid input addr 0x%lx order %d for %s",
					addr, order, __func__);
		while (addr < end) {
			put_pages_into_iee_block(addr, IEE_DATA_ORDER);
			addr += PMD_SIZE;
		}
	}
}

/* The reverse operation of put_pages_into_iee().
 * Call this function when you are returning pages back to kernel.
 */
static void remove_pages_from_iee(unsigned long addr, int order)
{
	pgd_t *pgdir = swapper_pg_dir;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, addr);
	p4d_t *p4dp = p4d_offset(pgdp, addr);
	pud_t *pudp = pud_offset(p4dp, addr);
	pmd_t *pmdp = pmd_offset(pudp, addr);
	pte_t *lm_ptep;
	pte_t *iee_ptep;
	unsigned long iee_addr;
	int use_block_pmd = 0;

	// Use Block Descriptor.
	if (pmd_leaf(*pmdp)) {
		use_block_pmd = 1;
		lm_ptep = (pte_t *)pmdp;
	} else
		lm_ptep = pte_offset_kernel(pmdp, addr);

	iee_addr = ((unsigned long)addr | IEE_OFFSET);
	pgdp = pgd_offset_pgd(pgdir, iee_addr);
	p4dp = p4d_offset(pgdp, iee_addr);
	pudp = pud_offset(p4dp, iee_addr);
	pmdp = pmd_offset(pudp, iee_addr);
	iee_ptep = pte_offset_kernel(pmdp, iee_addr);
	iee_unset_sensitive_pte(lm_ptep, iee_ptep, order, use_block_pmd);
	flush_tlb_kernel_range(addr, addr+PAGE_SIZE*(1 << order));
	flush_tlb_kernel_range(iee_addr, iee_addr+PAGE_SIZE*(1 << order));
	isb();
}

/* See put_pages_into_iee(). */
void set_iee_page_type(unsigned long addr, int order, enum HAOC_BITMAP_TYPE type)
{
	put_pages_into_iee(addr, order);
	iee_set_bitmap_type(addr, 1UL << order, type);
}

/* See put_pages_into_iee(). */
void set_iee_page(unsigned long addr, int order)
{
	put_pages_into_iee(addr, order);
}

/* See remove_pages_from_iee(). */
void unset_iee_page(unsigned long addr, int order)
{
	remove_pages_from_iee(addr, order);
}

unsigned int iee_calculate_order(struct kmem_cache *s, unsigned int order)
{
	if (!haoc_enabled) {
		return order;
	}
#ifdef CONFIG_IEE_PTRP
	if (strcmp(s->name, "task_struct") == 0)
		return IEE_DATA_ORDER;
#endif
#ifdef CONFIG_CREDP
	if (strcmp(s->name, "cred_jar") == 0)
		return IEE_DATA_ORDER;
#endif
	return order;
}

bool iee_free_slab_data(struct kmem_cache *s, struct slab *slab,
			unsigned int order)
{
#ifdef CONFIG_IEE_PTRP
	if (s == task_struct_cachep)
		iee_free_task_token_slab(s, slab, order);
#endif
	return false;
}

void iee_set_min_partial(struct kmem_cache *s)
{
	if (!haoc_enabled) {
		return ;
	}
#ifdef CONFIG_IEE_PTRP
	if (strcmp(s->name, "task_struct") == 0)
		s->min_partial *= 16;
#endif
}

static char *handler[] = {
	"ELR_EL1",
	"TCR_EL1",
};

/* Print out the reason of IEE panics. Called when IEE check failed. */
asmlinkage void notrace iee_bad_mode(struct pt_regs *regs, int reason,
		unsigned int esr, unsigned long info)
{
	arm64_enter_nmi(regs);

	console_verbose();

	pr_crit("IEE : Bad mode in %s check detected on CPU%d, code 0x%08x -- %s\n",
		handler[reason], smp_processor_id(), esr,
		esr_get_class_string(esr));

	__show_regs(regs);
	local_daif_mask();

	switch (reason) {
	case 0:
		pr_crit("IEE: Bad ELR_EL1 0x%llx\n", (u64)info);
		break;
	case 1:
		pr_crit("IEE: Bad TCR_EL1 0x%llx\n", (u64)info);
		break;
	}

	panic("bad mode");
}

#ifdef CONFIG_IEE_PTRP
struct task_token *iee_get_task_token(struct task_struct *task)
{
	unsigned long slab_addr;
	unsigned long task_addr;
	unsigned int index;

	if (!__is_lm_address((u64)task))
		return (struct task_token *)__kimg_to_iee(task);

	slab_addr = (unsigned long)page_to_virt(virt_to_head_page(task));
	task_addr = (unsigned long)page_to_virt(virt_to_page(task));
	index = (task_addr - slab_addr) / PAGE_SIZE;

	return (struct task_token *)(__virt_to_iee(slab_addr) +
				     index * IEE_TOKEN_BLOCK_SIZE);
}
#endif

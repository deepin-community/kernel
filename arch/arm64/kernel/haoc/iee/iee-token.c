// SPDX-License-Identifier: GPL-2.0
#include <asm/haoc/iee.h>
#include <asm/haoc/iee-mmu.h>
#include <asm/haoc/iee-token.h>
#include "slab.h"

void __init iee_prepare_init_task_token(void)
{
	struct task_token *init_token = (struct task_token *)__kimg_to_iee(&init_task);
	u64 init_token_addr = (u64)init_token;
	phys_addr_t init_token_page;
	int order = 0;

	/*
	 * if the init token covers the page boundary, the token page shall
	 * be allocated more.
	 */
	if (ALIGN((init_token_addr + sizeof(struct task_token)), PAGE_SIZE)
			!= ALIGN(init_token_addr + 1, PAGE_SIZE))
		order = 1;

	/* Allocate from IEE data pool. */
	init_token_page = early_iee_data_alloc(order);

	/* Map the IEE token address of init_task. */
	for (int i = 0; i < 1UL << order; i++) {
		pgd_t *pgdir = swapper_pg_dir;
		pgd_t *pgdp = pgd_offset_pgd(pgdir, init_token_addr);
		p4d_t *p4dp = p4d_offset(pgdp, init_token_addr);
		pud_t *pudp = pud_offset(p4dp, init_token_addr);
		pmd_t *pmdp = pmd_offset(pudp, init_token_addr);
		pte_t *ptep = pte_offset_kernel(pmdp, init_token_addr);
		pte_t pte = __ptep_get(ptep);

		pte = __pte(((pte_val(pte) | PTE_VALID) & ~PTE_ADDR_MASK)
					| __phys_to_pte_val(init_token_page));
		/* Manaully go through IEE gates to bypass PTP checks. */
		#ifdef CONFIG_PTP
		write_sysreg(read_sysreg(TCR_EL1) | TCR_HPD1 | TCR_A1, tcr_el1);
		isb();
		WRITE_ONCE(*__ptr_to_iee(ptep), pte);
		write_sysreg(read_sysreg(TCR_EL1) & ~(TCR_HPD1 | TCR_A1), tcr_el1);
		isb();
		#else
		__set_pte(ptep, pte);
		#endif

		init_token_addr += PAGE_SIZE;
	}

	/* Operate on the init token since it's already valid. */
	iee_validate_token(&init_task);
	pr_info("IEE: CONFIG_PTRP enabled.");
}

/* To map or unmap token pages when allocate or free task_struct. */
static inline void iee_set_token(unsigned long token_addr, unsigned long token_pages,
						unsigned int order, bool prot)
{
	phys_addr_t token_phys = __pa(token_pages);
	unsigned long end_addr = token_addr + (PAGE_SIZE << order);
	u64 curr_addr = token_addr;
	/*
	 * IEE mappings must be 4-level so we just need to find out the pte of start
	 * address.
	 */
	pgd_t *pgdir = swapper_pg_dir;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, token_addr);
	p4d_t *p4dp = p4d_offset(pgdp, token_addr);
	pud_t *pudp = pud_offset(p4dp, token_addr);
	pmd_t *pmdp = pmd_offset(pudp, token_addr);
	pte_t *ptep = pte_offset_kernel(pmdp, token_addr);

	/*
	 * We assume that input address range would never cross the boundary
	 * of a pmd block, so this function could simply use ptep++ to iterate
	 * inside a pte table.
	 */
	if (token_addr != ALIGN(token_addr, PAGE_SIZE)
			|| end_addr > ALIGN(token_addr + 1, PMD_SIZE))
		panic("%s: invalid input address range 0x%lx-0x%lx.", __func__,
				token_addr, end_addr);

	/* map new pages to IEE addresses one by one or clear them. */
	for (int i = 0; i < (1UL << order); i++) {
		pte_t pte = __ptep_get(ptep);

		if (prot) {
			/* rewrite physical address on pte. */
			pte = __pte(((pte_val(pte) | PTE_VALID) & ~PTE_ADDR_MASK)
					| __phys_to_pte_val(token_phys));
			token_phys += PAGE_SIZE;
		} else {
			/* Restore the 1:1 mapping with physical addresses. */
			pte = __pte(((pte_val(pte) & ~PTE_VALID) & ~PTE_ADDR_MASK)
						| __phys_to_pte_val(__iee_to_phys(curr_addr)));
			curr_addr += PAGE_SIZE;
		}

		__set_pte(ptep, pte);
		ptep++;
	}

	/* Apply or remove RO protection on linear mappings. */
	iee_set_logical_mem(token_pages, order, prot);

	flush_tlb_kernel_range(token_addr, end_addr);
	flush_tlb_kernel_range(token_pages, (token_pages + (PAGE_SIZE << order)));
}

static unsigned long iee_token_page(unsigned long token_addr)
{
	pgd_t *pgdir = swapper_pg_dir;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, token_addr);
	p4d_t *p4dp = p4d_offset(pgdp, token_addr);
	pud_t *pudp = pud_offset(p4dp, token_addr);
	pmd_t *pmdp = pmd_offset(pudp, token_addr);
	pte_t *ptep = pte_offset_kernel(pmdp, token_addr);

	return (unsigned long)page_address(pte_page(READ_ONCE(*ptep)));
}

/*
 * iee_set_token_page_valid() - After allocated task token pages, map them to the
 * corresponding IEE addresses, and enforce RO protection on their linear mappings.
 *
 * @token_addr: the start IEE address of task tokens.
 * @token_pages: virtual address of allocated token pages.
 * @order: size for address range.
 */
void iee_set_token_page_valid(unsigned long token_addr, unsigned long token_pages,
			unsigned int order)
{
	#ifdef DEBUG
	pr_info("IEE: Set token addr 0x%lx order %d for page 0x%lx", token_addr,
			order, token_pages);
	#endif
	iee_set_token(token_addr, token_pages, order, true);
}

void iee_set_token_page_invalid(unsigned long token_addr, unsigned long token_pages,
			unsigned int order)
{
	#ifdef DEBUG
	pr_info("IEE: Unset token addr 0x%lx order %d for page 0x%lx", token_addr,
			order, token_pages);
	#endif
	iee_set_token(token_addr, token_pages, order, false);
}

struct slab *iee_alloc_task_token_slab(struct kmem_cache *s,
				struct slab *slab, unsigned int order)
{
	if (!slab || s != task_struct_cachep)
		return slab;

	struct folio *folio = slab_folio(slab);
	unsigned int task_order = order;
	unsigned int token_order = IEE_TOKEN_ORDER(task_order);
	unsigned long token_addr = __slab_to_iee(slab);
	unsigned long alloc_token =
		__get_free_pages(GFP_KERNEL | __GFP_ZERO, token_order);

	/* Allocation of task_struct and token pages must be done at the same time. */
	if (!alloc_token) {
		/* Failed on allocation of token page. Free the allocated ones,
		 * return and try smaller order.
		 */
		__slab_clear_pfmemalloc(slab);
		folio->mapping = NULL;
		/* Make the mapping reset visible before clearing the flag */
		smp_wmb();
		__folio_clear_slab(folio);
		__free_pages((struct page *)folio, task_order);
		return NULL;
	}

	/* Map allocated token pages to token addresses. */
	iee_set_token_page_valid(token_addr, alloc_token, token_order);
	return slab;
}

void iee_free_task_token_slab(struct kmem_cache *s, struct slab *slab,
			      unsigned int order)
{
	unsigned int token_order;
	unsigned long token_addr;
	unsigned long token_page;

	if (!slab || s != task_struct_cachep)
		return;

	token_order = IEE_TOKEN_ORDER(order);
	token_addr = __slab_to_iee(slab);
	token_page = iee_token_page(token_addr);

	iee_set_token_page_invalid(token_addr, token_page, token_order);
	free_pages(token_page, token_order);
}

void __iee_code _iee_init_token(unsigned long __unused, struct task_struct *tsk)
{
	/* Do nothing for now. Wait for later update. */
}

void __iee_code _iee_set_token_pgd(unsigned long __unused, struct task_struct *tsk,
					pgd_t *pgd)
{
	struct task_token *token = (struct task_token *)iee_get_task_token(tsk);

	token->pgd = pgd;
}

void __iee_code _iee_validate_token(unsigned long __unused, struct task_struct *tsk)
{
	struct task_token *token = (struct task_token *)iee_get_task_token(tsk);

	if (token->valid)
		pr_err("IEE: validate token for multiple times.");
#ifdef CONFIG_CREDP
	token->new_cred = NULL;
	token->curr_cred = tsk->cred;
#endif
	token->valid = true;
}

void __iee_code _iee_invalidate_token(unsigned long __unused, struct task_struct *tsk)
{
	struct task_token *token = (struct task_token *)iee_get_task_token(tsk);

	token->pgd = NULL;
#ifdef CONFIG_CREDP
	token->new_cred = NULL;
	token->curr_cred = NULL;
#endif
	token->valid = false;
}

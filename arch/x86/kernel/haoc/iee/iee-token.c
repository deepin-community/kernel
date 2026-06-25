// SPDX-License-Identifier: GPL-2.0
#include <asm/set_memory.h>
#include <asm/haoc/iee-token.h>
#include "slab.h"

void iee_set_token_page_valid(unsigned long token, unsigned long token_page,
			      unsigned int order)
{
	set_memory_4k(token, 1 << order);
	set_memory_4k(token_page, 1 << order);

	pgd_t *pgdir = swapper_pg_dir;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, token);
	p4d_t *p4dp = p4d_offset(pgdp, token);
	pud_t *pudp = pud_offset(p4dp, token);
	pmd_t *token_pmdp = pmd_offset(pudp, token);
	pte_t *token_ptep = pte_offset_kernel(token_pmdp, token);

	if (!token_page)
		panic("Token of task_struct was unset.\n");

	pgdp = pgd_offset_pgd(pgdir, token_page);
	p4dp = p4d_offset(pgdp, token_page);
	pudp = pud_offset(p4dp, token_page);
	pmd_t *token_page_pmdp = pmd_offset(pudp, token_page);
	pte_t *token_page_ptep = pte_offset_kernel(token_page_pmdp, token_page);

	for (int i = 0; i < (0x1 << order); i++) {
		pte_t pte = READ_ONCE(*token_ptep);

		pte = __pte((pte_val(pte) & ~PTE_PFN_MASK) |
			    (__phys_to_pfn(__pa(token_page + i * PAGE_SIZE))
			     << PAGE_SHIFT));
		WRITE_ONCE(*token_ptep, pte);
		pte = READ_ONCE(*token_page_ptep);
		pte = __pte((pte_val(pte) & ~__RW) & ~___D);
		WRITE_ONCE(*token_page_ptep, pte);
		token_ptep++;
		token_page_ptep++;
	}

	flush_tlb_kernel_range(token, token + (PAGE_SIZE * (1 << order)));
	flush_tlb_kernel_range(token_page,
			       token_page + (PAGE_SIZE * (1 << order)));
}

void iee_set_token_page_invalid(unsigned long token, unsigned long __unused,
				unsigned int order)
{
	pgd_t *pgdir = swapper_pg_dir;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, token);
	p4d_t *p4dp = p4d_offset(pgdp, token);
	pud_t *pudp = pud_offset(p4dp, token);
	pmd_t *token_pmdp = pmd_offset(pudp, token);
	pte_t *token_ptep = pte_offset_kernel(token_pmdp, token);
	unsigned long token_page =
		(unsigned long)page_address(pte_page(*token_ptep));

	if (!token_page)
		panic("Token of task_struct was unset.\n");

	pgdp = pgd_offset_pgd(pgdir, token_page);
	p4dp = p4d_offset(pgdp, token_page);
	pudp = pud_offset(p4dp, token_page);
	pmd_t *token_page_pmdp = pmd_offset(pudp, token_page);
	pte_t *token_page_ptep = pte_offset_kernel(token_page_pmdp, token_page);

	for (int i = 0; i < (0x1 << order); i++) {
		pte_t pte = READ_ONCE(*token_ptep);

		pte = __pte((pte_val(pte) & ~PTE_PFN_MASK) |
			    (__phys_to_pfn(__iee_pa(token + i * PAGE_SIZE))
			     << PAGE_SHIFT));
		WRITE_ONCE(*token_ptep, pte);
		pte = READ_ONCE(*token_page_ptep);
		pte = __pte(pte_val(pte) | ___D | __RW);
		WRITE_ONCE(*token_page_ptep, pte);
		token_ptep++;
		token_page_ptep++;
	}
	free_pages(token_page, order);
	flush_tlb_kernel_range(token, token + (PAGE_SIZE * (1 << order)));
	flush_tlb_kernel_range(token_page,
			       token_page + (PAGE_SIZE * (1 << order)));
}

struct slab *iee_alloc_task_token_slab(struct kmem_cache *s, struct slab *slab,
				       unsigned int order)
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

void _iee_set_token_pgd(unsigned long __unused, struct task_struct *tsk,
			pgd_t *pgd)
{
	struct task_token *token = (struct task_token *)__addr_to_token(tsk);

	token->pgd = pgd;
}

void _iee_invalidate_token(unsigned long __unused, struct task_struct *tsk)
{
	struct task_token *token = (struct task_token *)__addr_to_token(tsk);

	token->pgd = NULL;
#ifdef CONFIG_CREDP
	token->new_cred = NULL;
	token->curr_cred = NULL;
#endif
	token->valid = false;
}

void _iee_validate_token(unsigned long __unused, struct task_struct *tsk)
{
	struct task_token *token = (struct task_token *)__addr_to_token(tsk);

	if (token->valid)
		pr_err("IEE: validate token for multiple times.");
#ifdef CONFIG_CREDP
	token->new_cred = NULL;
	token->curr_cred = tsk->cred;
#endif
	token->valid = true;
}

// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <asm/set_memory.h>
#include <asm/haoc/iee-token.h>
#include <linux/mm.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/sched/signal.h>
#include <asm/haoc/iee-func.h>
#include "slab.h"

/*
 * Slab-allocated task_struct objects use dedicated token pages, but init_task
 * is a kernel image symbol whose IEE alias initially maps its own page. Give
 * init_task a dedicated token page before its first credential operation.
 */
void __init iee_prepare_init_task_token(void)
{
	unsigned long token = (unsigned long)__kimg_to_iee(&init_task);
	unsigned long token_page;
	unsigned int order = 0;

	if (ALIGN(token + sizeof(struct task_token), PAGE_SIZE) !=
	    ALIGN(token + 1, PAGE_SIZE))
		order = 1;

	token_page = __get_free_pages(GFP_KERNEL | __GFP_ZERO, order);
	if (!token_page)
		panic("IEE: failed to allocate token page for init_task\n");

	iee_set_token_page_valid(ALIGN_DOWN(token, PAGE_SIZE), token_page,
				 order);
	iee_validate_token(&init_task);
}

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

#ifdef CONFIG_PTP
	iee_rw_gate(IEE_OP_SET_TOKEN, token_ptep, token_page_ptep, token_page, order);
#else
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
#endif

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

#ifdef CONFIG_PTP
	iee_rw_gate(IEE_OP_UNSET_TOKEN, token_ptep, token_page_ptep, token, order);
#else
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
#endif
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
	seqcount_init(&token->seq);
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
	seqcount_init(&token->seq);
	token->valid = true;
}

#ifdef CONFIG_PTP
void _iee_unset_token(unsigned long __unused, pte_t *token_ptep,
	pte_t *token_page_ptep, unsigned long token, unsigned int order)
{
	token_ptep = (pte_t *)__addr_to_iee(token_ptep);
	token_page_ptep = (pte_t *)__addr_to_iee(token_page_ptep);

	for (int i = 0; i < (0x1 << order); i++) {
		pte_t pte = READ_ONCE(*token_ptep);

		pte = __pte((pte_val(pte) & ~PTE_PFN_MASK) |
			(__phys_to_pfn(__iee_pa(token + i * PAGE_SIZE)) << PAGE_SHIFT));
		WRITE_ONCE(*token_ptep, pte);
		pte = READ_ONCE(*token_page_ptep);
		pte = __pte(pte_val(pte) | ___D | __RW);
		WRITE_ONCE(*token_page_ptep, pte);
		token_ptep++;
		token_page_ptep++;
	}
}

void _iee_set_token(unsigned long __unused, pte_t *token_ptep,
	pte_t *token_page_ptep, unsigned long token_page, unsigned int order)
{
	token_ptep = (pte_t *)__addr_to_iee(token_ptep);
	token_page_ptep = (pte_t *)__addr_to_iee(token_page_ptep);

	for (int i = 0; i < (0x1 << order); i++) {
		pte_t pte = READ_ONCE(*token_ptep);

		pte = __pte(((pte_val(pte) & ~PTE_PFN_MASK)) |
			(__phys_to_pfn(__pa(token_page + i * PAGE_SIZE)) << PAGE_SHIFT));
		WRITE_ONCE(*token_ptep, pte);
		pte = READ_ONCE(*token_page_ptep);
		pte = __pte((pte_val(pte) & ~__RW) & ~___D);
		WRITE_ONCE(*token_page_ptep, pte);
		token_ptep++;
		token_page_ptep++;
	}
}
#endif

#if defined(CONFIG_IEE_PTRP) && !defined(CONFIG_IEE_PTRP_W)
void iee_verify_token(struct task_struct *tsk)
{
	struct task_token *token;
#ifdef CONFIG_CREDP
	const struct cred *old_cred;
	const struct cred *token_cred;
#endif
	bool valid;
	unsigned int seq;

	if (unlikely(!haoc_init_done))
		return;

	if (!haoc_enabled)
		return;

	if (tsk == &init_task)
		return;

	token = (struct task_token *)__addr_to_token(tsk);
	do {
		seq = read_seqcount_begin(&token->seq);
		valid = token->valid;
#ifdef CONFIG_CREDP
		old_cred = tsk->cred;
		token_cred = token->curr_cred;
#endif
	} while (read_seqcount_retry(&token->seq, seq));

	if (!valid)
		panic("IEE: (%s) Invalid Token.", __func__);
#ifdef CONFIG_CREDP
	if (token_cred != old_cred)
		panic("IEE: (%s) Task cred corruptted! token cred 0x%llx, curr 0x%llx",
		      __func__, (u64)token_cred, (u64)old_cred);
#endif
}

static void check_all_threads(void)
{
	struct task_struct *task;

	rcu_read_lock();
	for_each_process(task)
		iee_verify_token(task);
	rcu_read_unlock();
}

static int checker_thread(void *data)
{
	int check_interval_ms = 500;

	while (!kthread_should_stop()) {
		check_all_threads();
		msleep_interruptible(check_interval_ms);
	}
	pr_info("[IEE] Kernel thread exiting\n");
	return 0;
}

static int __init thread_checker_init(void)
{
	struct task_struct *checker_task;

	if (!haoc_enabled)
		return 0;

	pr_info("IEE: Initializing thread checker\n");
	checker_task = kthread_run(checker_thread, NULL, "thread_credp_cycle");
	if (IS_ERR(checker_task)) {
		pr_err("IEE: Failed to create thread checker task: %ld\n",
		       PTR_ERR(checker_task));
		return PTR_ERR(checker_task);
	}

	pr_info("IEE: Thread checker started successfully\n");
	return 0;
}

late_initcall(thread_checker_init);

void iee_verify_pgd(struct task_struct *next)
{
	if (haoc_enabled && next != &init_task) {
		struct task_token *token;

		token = (struct task_token *)__addr_to_token(next);
		if (token->pgd != next->mm->pgd)
			panic("IEE Pgd Error: next_pgd: 0x%lx, token_pgd: 0x%lx",
				(unsigned long)next->mm->pgd, (unsigned long)token->pgd);
	}
}
#endif /* CONFIG_IEE_PTRP && !CONFIG_IEE_PTRP_W */

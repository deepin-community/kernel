/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IEE_TOKEN_H
#define _LINUX_IEE_TOKEN_H

#include <asm/haoc/haoc-def.h>
#include <linux/log2.h>

#ifdef CONFIG_IEE_PTRP
extern struct kmem_cache *task_struct_cachep;

extern void __init iee_prepare_init_task_token(void);
extern void iee_set_token_page_valid(unsigned long token, unsigned long new,
				unsigned int order);
extern void iee_set_token_page_invalid(unsigned long token_addr,
				unsigned long token_page, unsigned int order);
extern struct slab *iee_alloc_task_token_slab(struct kmem_cache *s,
					struct slab *slab, unsigned int order);
extern void iee_free_task_token_slab(struct kmem_cache *s, struct slab *slab,
				     unsigned int order);

#ifndef IEE_TOKEN_BLOCK_SIZE
#define IEE_TOKEN_BLOCK_SIZE	64
#endif

#ifndef IEE_TOKEN_ORDER
#define IEE_TOKEN_ORDER(task_order) \
	order_base_2(((1U << (task_order)) * IEE_TOKEN_BLOCK_SIZE) / PAGE_SIZE)
#endif

extern struct task_token *iee_get_task_token(struct task_struct *task);
#endif /* CONFIG_IEE_PTRP */

#ifdef CONFIG_IEE
struct task_token {
	pgd_t *pgd; /* Logical VA */
	void *iee_stack; /* VA */
	void *tmp_page;
	bool valid;
	void *kernel_stack; /* VA */
#ifdef CONFIG_CREDP
	const struct cred *new_cred;	/* The valid target for commit_creds. */
	const struct cred *curr_cred;	/* The current subjective credentials. */
#endif
};
#endif /* CONFIG_IEE */

#ifdef CONFIG_IEE_PTRP
#ifndef CONFIG_IEE_SIP
#include <asm/haoc/iee.h>
static inline void iee_verify_pgd(struct task_struct *tsk)
{
	struct task_token *token;

	if (tsk == &init_task)
		return;

	token = (struct task_token *)iee_get_task_token(tsk);
	if (token->pgd != tsk->mm->pgd)
		panic("IEE Pgd Error: tsk_pgd: 0x%lx, token_pgd: 0x%lx",
			(unsigned long)tsk->mm->pgd, (unsigned long)token->pgd);
}
#else
static inline void iee_verify_pgd(struct task_struct *tsk)
{

}
#endif

static inline void iee_set_token_pgd(struct task_struct *tsk, pgd_t *pgd)
{
	iee_rw_gate(IEE_OP_SET_TOKEN_PGD, tsk, pgd);
}

static inline void iee_init_token(struct task_struct *tsk)
{
	iee_rw_gate(IEE_OP_INIT_TOKEN, tsk);
}

static inline void iee_invalidate_token(struct task_struct *tsk)
{
	iee_rw_gate(IEE_OP_INVALIDATE_TOKEN, tsk);
}

static inline void iee_validate_token(struct task_struct *tsk)
{
	iee_rw_gate(IEE_OP_VALIDATE_TOKEN, tsk);
}
#endif /* CONFIG_IEE_PTRP */

#endif

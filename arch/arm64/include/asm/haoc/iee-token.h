/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IEE_TOKEN_H
#define _LINUX_IEE_TOKEN_H

#include <asm/haoc/haoc-def.h>

extern struct kmem_cache *task_struct_cachep;

extern void __init iee_prepare_init_task_token(void);
extern void iee_set_token_page_valid(unsigned long token, unsigned long new,
				unsigned int order);
extern void iee_set_token_page_invalid(unsigned long token_addr,
				unsigned long token_page, unsigned int order);
extern struct slab *iee_alloc_task_token_slab(struct kmem_cache *s,
					struct slab *slab, unsigned int order);

struct task_token {
	pgd_t *pgd; /* Logical VA */
	bool valid;
};

#ifndef CONFIG_IEE_SIP
#include <asm/haoc/iee.h>
static inline void iee_verify_token_pgd(struct task_struct *tsk)
{
	struct task_token *token;

	if (tsk == &init_task)
		return;

	token = (struct task_token *)__addr_to_iee(tsk);
	if (token->pgd != tsk->mm->pgd)
		panic("IEE Pgd Error: tsk_pgd: 0x%lx, token_pgd: 0x%lx",
			(unsigned long)tsk->mm->pgd, (unsigned long)token->pgd);
}
#else
static inline void iee_verify_token_pgd(struct task_struct *tsk)
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

#endif

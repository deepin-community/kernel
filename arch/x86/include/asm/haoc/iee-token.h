/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IEE_TOKEN_H
#define _LINUX_IEE_TOKEN_H
#include <asm/haoc/iee.h>
#include <asm/haoc/haoc-def.h>
#include <linux/sched/task.h>
#include <linux/sched.h>

extern unsigned long long iee_rw_gate(int flag, ...);

extern struct kmem_cache *task_struct_cachep;

extern void iee_prepare_init_task_token(void);
extern void iee_set_token_page_valid(unsigned long token, unsigned long new,
				     unsigned int order);
extern void iee_set_token_page_invalid(unsigned long token_addr,
				       unsigned long __unused,
				       unsigned int order);
extern struct slab *iee_alloc_task_token_slab(struct kmem_cache *s,
					      struct slab *slab,
					      unsigned int order);

#ifdef CONFIG_IEE
struct task_token {
	pgd_t *pgd; /* Logical VA */
	void *iee_stack; /* VA */
	void *tmp_page;
	bool valid;
	void *kernel_stack; /* VA */
#ifdef CONFIG_CREDP
	struct cred *new_cred;	/* The valid target for commit_creds. */
#endif
};
#endif /* CONFIG_IEE */

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

static inline void iee_set_token_pgd(struct task_struct *tsk, pgd_t *pgd)
{
	iee_rw_gate(IEE_OP_SET_TOKEN_PGD, tsk, pgd);
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
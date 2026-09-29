/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IEE_TOKEN_H
#define _LINUX_IEE_TOKEN_H
#include <asm/haoc/iee.h>
#include <asm/haoc/haoc-def.h>
#include <linux/init.h>
#include <linux/log2.h>
#include <linux/sched/task.h>
#include <linux/sched.h>
#include <linux/seqlock.h>

extern unsigned long long iee_rw_gate(int flag, ...);

#ifdef CONFIG_IEE_PTRP
extern struct kmem_cache *task_struct_cachep;

extern void __init iee_prepare_init_task_token(void);
extern void iee_set_token_page_valid(unsigned long token, unsigned long new,
					     unsigned int order);
extern void iee_set_token_page_invalid(unsigned long token_addr,
				       unsigned long __unused,
				       unsigned int order);
extern struct slab *iee_alloc_task_token_slab(struct kmem_cache *s,
					      struct slab *slab,
					      unsigned int order);

#ifndef IEE_TOKEN_BLOCK_SIZE
#define IEE_TOKEN_BLOCK_SIZE	64
#endif

#ifndef IEE_TOKEN_ORDER
#define IEE_TOKEN_ORDER(task_order) \
	order_base_2(((1U << (task_order)) * IEE_TOKEN_BLOCK_SIZE) / PAGE_SIZE)
#endif
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
#ifdef CONFIG_IEE_PTRP
	seqcount_t seq;
#endif
};
#endif /* CONFIG_IEE */

#ifdef CONFIG_IEE_PTRP
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

#if !defined(CONFIG_IEE_PTRP_W)
void iee_verify_pgd(struct task_struct *next);
void iee_verify_token(struct task_struct *tsk);
#endif

#endif /* CONFIG_IEE_PTRP */

#endif /* _LINUX_IEE_TOKEN_H */

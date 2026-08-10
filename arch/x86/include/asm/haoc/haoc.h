/* SPDX-License-Identifier: GPL-2.0 */
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Shu Hang <shuh2023@zgclab.edu.cn>
 *          Hu Bing <hubing2023@zgclab.edu.cn>
 */

#ifndef _LINUX_HAOC_H
#define _LINUX_HAOC_H

#include <linux/types.h>
#include <linux/sched.h>

void _iee_memcpy(unsigned long __unused,void *dst,void *src, size_t n);
void _iee_memset(unsigned long __unused,void *ptr, int data, size_t n);
void _iee_set_freeptr(unsigned long __unused,void **pptr,void *ptr);
unsigned long _iee_test_and_clear_bit(unsigned long __unused,
					long nr, unsigned long *addr);
#ifdef CONFIG_IEE_PTRP
void _iee_set_token_pgd(unsigned long __unused, struct task_struct *tsk,
		pgd_t *pgd);
void _iee_invalidate_token(unsigned long __unused, struct task_struct *tsk);
void _iee_validate_token(unsigned long __unused, struct task_struct *tsk);
#endif

#ifdef CONFIG_CREDP
#include <linux/cred.h>

void _iee_copy_cred(unsigned long __unused, struct cred *new);
void _iee_copy_kernel_cred(unsigned long iee_offset, const struct cred *old, struct cred *new);
void _iee_abort_cred(unsigned long iee_offset, const struct cred *cred);
void _iee_init_copied_cred(unsigned long iee_offset, struct task_struct *new_task,
		struct cred *new);
void _iee_commit_creds(unsigned long iee_offset, const struct cred *new);
void _iee_set_cred_uid(unsigned long __unused, struct cred *cred, kuid_t uid);
void _iee_set_cred_gid(unsigned long __unused, struct cred *cred, kgid_t gid);
void _iee_set_cred_suid(unsigned long __unused, struct cred *cred, kuid_t suid);
void _iee_set_cred_sgid(unsigned long __unused, struct cred *cred, kgid_t sgid);
void _iee_set_cred_euid(unsigned long __unused, struct cred *cred, kuid_t euid);
void _iee_set_cred_egid(unsigned long __unused, struct cred *cred, kgid_t egid);
void _iee_set_cred_fsuid(unsigned long __unused, struct cred *cred, kuid_t fsuid);
void _iee_set_cred_fsgid(unsigned long __unused, struct cred *cred, kgid_t fsgid);
void _iee_set_cred_user(unsigned long __unused, struct cred *cred, struct user_struct *user);
void _iee_set_cred_user_ns(unsigned long __unused,
			struct cred *cred, struct user_namespace *user_ns);
void _iee_set_cred_group_info(unsigned long __unused,
			struct cred *cred, struct group_info *group_info);
void _iee_set_cred_securebits(unsigned long __unused,
			struct cred *cred, unsigned int securebits);
void _iee_set_cred_cap_inheritable(unsigned long __unused,
			struct cred *cred, kernel_cap_t cap_inheritable);
void _iee_set_cred_cap_permitted(unsigned long __unused,
			struct cred *cred, kernel_cap_t cap_permitted);
void _iee_set_cred_cap_effective(unsigned long __unused,
			struct cred *cred, kernel_cap_t cap_effective);
void _iee_set_cred_cap_bset(unsigned long __unused, struct cred *cred, kernel_cap_t cap_bset);
void _iee_set_cred_cap_ambient(unsigned long __unused, struct cred *cred, kernel_cap_t cap_ambient);
void _iee_set_cred_jit_keyring(unsigned long __unused,
			struct cred *cred, unsigned char jit_keyring);
void _iee_set_cred_session_keyring(unsigned long __unused,
			struct cred *cred, struct key *session_keyring);
void _iee_set_cred_process_keyring(unsigned long __unused,
			struct cred *cred, struct key *process_keyring);
void _iee_set_cred_thread_keyring(unsigned long __unused,
			struct cred *cred, struct key *thread_keyring);
void _iee_set_cred_request_key_auth(unsigned long __unused,
			struct cred *cred, struct key *request_key_auth);
void _iee_set_cred_non_rcu(unsigned long __unused, struct cred *cred, int non_rcu);
void _iee_set_cred_atomic_set_usage(unsigned long __unused, struct cred *cred, int i);
unsigned long _iee_set_cred_atomic_op_usage(unsigned long __unused,
			struct cred *cred, int flag, int nr);
void _iee_set_cred_security(unsigned long __unused, struct cred *cred,void *security);
void _iee_set_cred_rcu(unsigned long __unused, struct cred *cred, struct rcu_head *rcu);
void _iee_set_cred_ucounts(unsigned long __unused, struct cred *cred, struct ucounts *ucounts);
#endif

/* Called from iee_rw_gate (asm) to dispatch IEE ops to the _iee_*() functions. */
unsigned long iee_dispatch(int flag, unsigned long arg1,
			   unsigned long arg2, unsigned long arg3);
#endif

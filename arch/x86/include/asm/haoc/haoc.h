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

#include <asm/haoc/haoc-bitmap.h>
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
void _iee_set_bitmap_type(unsigned long __unused, uint8_t *bitmap_ptr,
		enum HAOC_BITMAP_TYPE type, int num_pages);

#ifdef CONFIG_IEE_SELINUX_P
#include <asm/haoc/iee-selinux.h>
void _iee_set_selinux_status_pg(unsigned long iee_offset, struct page *new_page);
void _iee_set_selinux_enforcing(unsigned long iee_offset, bool value);
void _iee_mark_selinux_initialized(unsigned long iee_offset);
void _iee_set_sel_policy_cap(unsigned long iee_offset, unsigned int idx, int cap);
void _iee_sel_rcu_assign_policy(unsigned long iee_offset,
			struct selinux_policy *new_policy, struct selinux_policy *iee_new_policy);
#endif

#ifdef CONFIG_PTP
#ifdef CONFIG_IEE_PTRP
void _iee_unset_token(unsigned long __unused, pte_t *token_ptep,
	pte_t *token_page_ptep, unsigned long token, unsigned int order);
void _iee_set_token(unsigned long __unused, pte_t *token_ptep,
	pte_t *token_page_ptep, unsigned long token_page, unsigned int order);
#endif
void _iee_set_pte(unsigned long __unused, pte_t *ptep, pte_t pte);
void _iee_set_pmd(unsigned long __unused, pmd_t *pmdp, pmd_t pmd);
void _iee_set_pud(unsigned long __unused, pud_t *pudp, pud_t pud);
void _iee_set_p4d(unsigned long __unused, p4d_t *p4dp, p4d_t p4d);
void _iee_set_pgd(unsigned long __unused, pgd_t *pgdp, pgd_t pgd);
void _iee_set_pte_text_poke(unsigned long __unused, pte_t *ptep, pte_t pte);
void _iee_set_pmd_ident(unsigned long __unused, pmd_t *pmdp, pmd_t pmd);
void _iee_set_pud_ident(unsigned long __unused, pud_t *pudp, pud_t pud);
void _iee_set_sx_pte(unsigned long __unused, pte_t *ptep, pte_t pte);
#endif

#ifdef CONFIG_CREDP
#include <linux/cred.h>

void _iee_copy_cred(unsigned long __unused, struct cred *new);
void _iee_copy_kernel_cred(unsigned long iee_offset, const struct cred *old, struct cred *new);
void _iee_abort_cred(unsigned long iee_offset, const struct cred *cred);
void _iee_fill_cred_for_session_keyring(unsigned long __unused, struct cred *new,
			const struct cred *old);
void _iee_init_copied_cred(unsigned long iee_offset, struct task_struct *new_task,
		struct cred *new);
void _iee_commit_creds(unsigned long iee_offset, const struct cred *new);
void _iee_override_creds(unsigned long iee_offset, const struct cred *new);
void _iee_revert_creds(unsigned long iee_offset, const struct cred *old);
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

#ifdef CONFIG_VARP
void _iee_set_varp_modprobe_path(unsigned long __unused, char *data, int maxlen, size_t len,
		char *buffer, size_t *lenp);
#endif

#ifdef CONFIG_VARP
void _iee_set_varp_modprobe_path(unsigned long __unused, char *data, int maxlen, size_t len,
		char *buffer, size_t *lenp);
#endif

#ifdef CONFIG_KEYP
#include <linux/key.h>
struct watch_list;

void _iee_set_key_union(unsigned long __unused, struct key *key, struct key_union *key_union);
void _iee_set_key_struct(unsigned long __unused, struct key *key, struct key_struct *key_struct);
void _iee_set_key_payload(unsigned long __unused, struct key *key, union key_payload *key_payload);
unsigned long _iee_set_key_usage(unsigned long __unused, struct key *key, int n, int flag);
void _iee_set_key_serial(unsigned long __unused, struct key *key, key_serial_t serial);
void _iee_set_key_watchers(unsigned long __unused, struct key *key, struct watch_list *watchers);
void _iee_set_key_user(unsigned long __unused, struct key *key, struct key_user *user);
void _iee_set_key_security(unsigned long __unused, struct key *key, void *security);
void _iee_set_key_expiry(unsigned long __unused, struct key *key, time64_t expiry);
void _iee_set_key_revoked_at(unsigned long __unused, struct key *key, time64_t revoked_at);
void _iee_set_key_last_used_at(unsigned long __unused, struct key *key, time64_t last_used_at);
void _iee_set_key_uid(unsigned long __unused, struct key *key, kuid_t uid);
void _iee_set_key_gid(unsigned long __unused, struct key *key, kgid_t gid);
void _iee_set_key_perm(unsigned long __unused, struct key *key, key_perm_t perm);
void _iee_set_key_quotalen(unsigned long __unused, struct key *key, unsigned short quotalen);
void _iee_set_key_datalen(unsigned long __unused, struct key *key, unsigned short datalen);
void _iee_set_key_state(unsigned long __unused, struct key *key, short state);
void _iee_set_key_magic(unsigned long __unused, struct key *key, unsigned int magic);
void _iee_set_key_flags(unsigned long __unused, struct key *key, unsigned long flags);
void _iee_set_key_index_key(unsigned long __unused, struct key *key,
					struct keyring_index_key *index_key);
void _iee_set_key_hash(unsigned long __unused, struct key *key, unsigned long hash);
void _iee_set_key_len_desc(unsigned long __unused, struct key *key, unsigned long len_desc);
void _iee_set_key_type(unsigned long __unused, struct key *key, struct key_type *type);
void _iee_set_key_domain_tag(unsigned long __unused, struct key *key, struct key_tag *domain_tag);
void _iee_set_key_description(unsigned long __unused, struct key *key, char *description);
void _iee_set_key_restrict_link(unsigned long __unused, struct key *key,
					struct key_restriction *restrict_link);
unsigned long _iee_set_key_flag_bit(unsigned long __unused, struct key *key, long nr, int flag);
#endif

unsigned long iee_dispatch(unsigned long flag, unsigned long arg1,
			   unsigned long arg2, unsigned long arg3,
			   unsigned long arg4, unsigned long arg5);
#endif

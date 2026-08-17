// SPDX-License-Identifier: GPL-2.0
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Shu Hang <shuh2023@zgclab.edu.cn>
 *          Hu Bing <hubing2023@zgclab.edu.cn>
 */

#include <linux/ftrace.h>
#include <linux/panic.h>
#include <asm/haoc/haoc.h>
#include <asm/haoc/haoc-def.h>
#include <asm/pgtable.h>

notrace unsigned long iee_dispatch(unsigned long flag, unsigned long arg1,
				   unsigned long arg2, unsigned long arg3,
				   unsigned long arg4, unsigned long arg5)
{
	switch (flag) {
	case IEE_OP_MEMCPY:
		_iee_memcpy(flag, (void *)arg1, (void *)arg2, (size_t)arg3);
		break;
	case IEE_OP_MEMSET:
		_iee_memset(flag, (void *)arg1, (int)arg2, (size_t)arg3);
		break;
	case IEE_OP_SET_FREEPTR:
		_iee_set_freeptr(flag, (void **)arg1, (void *)arg2);
		break;
	case IEE_OP_TEST_CLEAR_BIT:
		return _iee_test_and_clear_bit(flag, (long)arg1,
					       (unsigned long *)arg2);
#ifdef CONFIG_IEE_PTRP
	case IEE_OP_SET_TOKEN_PGD:
		_iee_set_token_pgd(flag, (struct task_struct *)arg1,
				   (pgd_t *)arg2);
		break;
	case IEE_OP_INVALIDATE_TOKEN:
		_iee_invalidate_token(flag, (struct task_struct *)arg1);
		break;
	case IEE_OP_VALIDATE_TOKEN:
		_iee_validate_token(flag, (struct task_struct *)arg1);
		break;
#endif
	case IEE_OP_SET_BITMAP_TYPE:
		_iee_set_bitmap_type(flag, (uint8_t *)arg1,
				     (enum HAOC_BITMAP_TYPE)arg2, (int)arg3);
		break;
#ifdef CONFIG_CREDP
	case IEE_OP_COPY_CRED:
		_iee_copy_cred(flag, (struct cred *)arg1);
		break;
	case IEE_OP_COPY_KERNEL_CRED:
		_iee_copy_kernel_cred(flag, (const struct cred *)arg1,
				      (struct cred *)arg2);
		break;
	case IEE_OP_INIT_COPIED_CRED:
		_iee_init_copied_cred(flag, (struct task_struct *)arg1,
				      (struct cred *)arg2);
		break;
	case IEE_OP_COMMIT_CRED:
		_iee_commit_creds(flag, (const struct cred *)arg1);
		break;
	case IEE_OP_ABORT_CRED:
		_iee_abort_cred(flag, (const struct cred *)arg1);
		break;
	case IEE_OP_FILL_SESSION_KEYRING_CRED:
		_iee_fill_cred_for_session_keyring(flag, (struct cred *)arg1,
						 (const struct cred *)arg2);
		break;
	case IEE_OP_OVERRIDE_CRED:
		_iee_override_creds(flag, (const struct cred *)arg1);
		break;
	case IEE_OP_REVERT_CRED:
		_iee_revert_creds(flag, (const struct cred *)arg1);
		break;
	case IEE_OP_SET_CRED_UID:
		_iee_set_cred_uid(flag, (struct cred *)arg1,
				  KUIDT_INIT((uid_t)arg2));
		break;
	case IEE_OP_SET_CRED_GID:
		_iee_set_cred_gid(flag, (struct cred *)arg1,
				  KGIDT_INIT((gid_t)arg2));
		break;
	case IEE_OP_SET_CRED_SUID:
		_iee_set_cred_suid(flag, (struct cred *)arg1,
				   KUIDT_INIT((uid_t)arg2));
		break;
	case IEE_OP_SET_CRED_SGID:
		_iee_set_cred_sgid(flag, (struct cred *)arg1,
				   KGIDT_INIT((gid_t)arg2));
		break;
	case IEE_OP_SET_CRED_EUID:
		_iee_set_cred_euid(flag, (struct cred *)arg1,
				   KUIDT_INIT((uid_t)arg2));
		break;
	case IEE_OP_SET_CRED_EGID:
		_iee_set_cred_egid(flag, (struct cred *)arg1,
				   KGIDT_INIT((gid_t)arg2));
		break;
	case IEE_OP_SET_CRED_FSUID:
		_iee_set_cred_fsuid(flag, (struct cred *)arg1,
				    KUIDT_INIT((uid_t)arg2));
		break;
	case IEE_OP_SET_CRED_FSGID:
		_iee_set_cred_fsgid(flag, (struct cred *)arg1,
				    KGIDT_INIT((gid_t)arg2));
		break;
	case IEE_OP_SET_CRED_USER:
		_iee_set_cred_user(flag, (struct cred *)arg1,
				   (struct user_struct *)arg2);
		break;
	case IEE_OP_SET_CRED_USER_NS:
		_iee_set_cred_user_ns(flag, (struct cred *)arg1,
				      (struct user_namespace *)arg2);
		break;
	case IEE_OP_SET_CRED_GROUP_INFO:
		_iee_set_cred_group_info(flag, (struct cred *)arg1,
					 (struct group_info *)arg2);
		break;
	case IEE_OP_SET_CRED_SECUREBITS:
		_iee_set_cred_securebits(flag, (struct cred *)arg1,
					 (unsigned int)arg2);
		break;
	case IEE_OP_SET_CRED_CAP_INHER:
		_iee_set_cred_cap_inheritable(flag, (struct cred *)arg1,
					      (kernel_cap_t){ .val = arg2 });
		break;
	case IEE_OP_SET_CRED_CAP_PERM:
		_iee_set_cred_cap_permitted(flag, (struct cred *)arg1,
					    (kernel_cap_t){ .val = arg2 });
		break;
	case IEE_OP_SET_CRED_CAP_EFFECT:
		_iee_set_cred_cap_effective(flag, (struct cred *)arg1,
					    (kernel_cap_t){ .val = arg2 });
		break;
	case IEE_OP_SET_CRED_CAP_BSET:
		_iee_set_cred_cap_bset(flag, (struct cred *)arg1,
				       (kernel_cap_t){ .val = arg2 });
		break;
	case IEE_OP_SET_CRED_CAP_AMBIENT:
		_iee_set_cred_cap_ambient(flag, (struct cred *)arg1,
					  (kernel_cap_t){ .val = arg2 });
		break;
	case IEE_OP_SET_CRED_JIT_KEYRING:
		_iee_set_cred_jit_keyring(flag, (struct cred *)arg1,
					  (unsigned char)arg2);
		break;
	case IEE_OP_SET_CRED_SESS_KEYRING:
		_iee_set_cred_session_keyring(flag, (struct cred *)arg1,
					      (struct key *)arg2);
		break;
	case IEE_OP_SET_CRED_PROC_KEYRING:
		_iee_set_cred_process_keyring(flag, (struct cred *)arg1,
					      (struct key *)arg2);
		break;
	case IEE_OP_SET_CRED_THREAD_KEYRING:
		_iee_set_cred_thread_keyring(flag, (struct cred *)arg1,
					     (struct key *)arg2);
		break;
	case IEE_OP_SET_CRED_REQ_KEYRING:
		_iee_set_cred_request_key_auth(flag, (struct cred *)arg1,
					       (struct key *)arg2);
		break;
	case IEE_OP_SET_CRED_NON_RCU:
		_iee_set_cred_non_rcu(flag, (struct cred *)arg1, (int)arg2);
		break;
	case IEE_OP_SET_CRED_ATSET_USAGE:
		_iee_set_cred_atomic_set_usage(flag, (struct cred *)arg1,
					       (int)arg2);
		break;
	case IEE_OP_SET_CRED_ATOP_USAGE:
		return _iee_set_cred_atomic_op_usage(flag, (struct cred *)arg1,
						     (int)arg2, (int)arg3);
	case IEE_OP_SET_CRED_SECURITY:
		_iee_set_cred_security(flag, (struct cred *)arg1, (void *)arg2);
		break;
	case IEE_OP_SET_CRED_RCU:
		_iee_set_cred_rcu(flag, (struct cred *)arg1,
				  (struct rcu_head *)arg2);
		break;
	case IEE_OP_SET_CRED_UCOUNTS:
		_iee_set_cred_ucounts(flag, (struct cred *)arg1,
				      (struct ucounts *)arg2);
		break;
#endif
#ifdef CONFIG_KEYP
	case IEE_OP_SET_KEY_UNION:
		_iee_set_key_union(flag, (struct key *)arg1,
				   (struct key_union *)arg2);
		break;
	case IEE_OP_SET_KEY_STRUCT:
		_iee_set_key_struct(flag, (struct key *)arg1,
				    (struct key_struct *)arg2);
		break;
	case IEE_OP_SET_KEY_PAYLOAD:
		_iee_set_key_payload(flag, (struct key *)arg1,
				     (union key_payload *)arg2);
		break;
	case IEE_OP_SET_KEY_USAGE:
		return _iee_set_key_usage(flag, (struct key *)arg1, (int)arg2,
					  (int)arg3);
	case IEE_OP_SET_KEY_SERIAL:
		_iee_set_key_serial(flag, (struct key *)arg1,
				    (key_serial_t)arg2);
		break;
	case IEE_OP_SET_KEY_WATCHERS:
		_iee_set_key_watchers(flag, (struct key *)arg1,
				      (struct watch_list *)arg2);
		break;
	case IEE_OP_SET_KEY_USERS:
		_iee_set_key_user(flag, (struct key *)arg1,
				  (struct key_user *)arg2);
		break;
	case IEE_OP_SET_KEY_SECURITY:
		_iee_set_key_security(flag, (struct key *)arg1, (void *)arg2);
		break;
	case IEE_OP_SET_KEY_EXPIRY:
		_iee_set_key_expiry(flag, (struct key *)arg1, (time64_t)arg2);
		break;
	case IEE_OP_SET_KEY_REVOKED_AT:
		_iee_set_key_revoked_at(flag, (struct key *)arg1,
					 (time64_t)arg2);
		break;
	case IEE_OP_SET_KEY_LAST_USED_AT:
		_iee_set_key_last_used_at(flag, (struct key *)arg1,
					   (time64_t)arg2);
		break;
	case IEE_OP_SET_KEY_UID:
		_iee_set_key_uid(flag, (struct key *)arg1,
				 KUIDT_INIT((uid_t)arg2));
		break;
	case IEE_OP_SET_KEY_GID:
		_iee_set_key_gid(flag, (struct key *)arg1,
				 KGIDT_INIT((gid_t)arg2));
		break;
	case IEE_OP_SET_KEY_PERM:
		_iee_set_key_perm(flag, (struct key *)arg1, (key_perm_t)arg2);
		break;
	case IEE_OP_SET_KEY_QUOTALEN:
		_iee_set_key_quotalen(flag, (struct key *)arg1,
				      (unsigned short)arg2);
		break;
	case IEE_OP_SET_KEY_DATALEN:
		_iee_set_key_datalen(flag, (struct key *)arg1,
				     (unsigned short)arg2);
		break;
	case IEE_OP_SET_KEY_STATE:
		_iee_set_key_state(flag, (struct key *)arg1, (short)arg2);
		break;
	case IEE_OP_SET_KEY_MAGIC:
		_iee_set_key_magic(flag, (struct key *)arg1,
				   (unsigned int)arg2);
		break;
	case IEE_OP_SET_KEY_FLAGS:
		_iee_set_key_flags(flag, (struct key *)arg1, arg2);
		break;
	case IEE_OP_SET_KEY_INDEX_KEY:
		_iee_set_key_index_key(flag, (struct key *)arg1,
				       (struct keyring_index_key *)arg2);
		break;
	case IEE_OP_SET_KEY_HASH:
		_iee_set_key_hash(flag, (struct key *)arg1, arg2);
		break;
	case IEE_OP_SET_KEY_LEN_DESC:
		_iee_set_key_len_desc(flag, (struct key *)arg1, arg2);
		break;
	case IEE_OP_SET_KEY_TYPE:
		_iee_set_key_type(flag, (struct key *)arg1,
				  (struct key_type *)arg2);
		break;
	case IEE_OP_SET_KEY_TAG:
		_iee_set_key_domain_tag(flag, (struct key *)arg1,
					(struct key_tag *)arg2);
		break;
	case IEE_OP_SET_KEY_DESCRIPTION:
		_iee_set_key_description(flag, (struct key *)arg1, (char *)arg2);
		break;
	case IEE_OP_SET_KEY_RESTRICT_LINK:
		_iee_set_key_restrict_link(flag, (struct key *)arg1,
					   (struct key_restriction *)arg2);
		break;
	case IEE_OP_SET_KEY_FLAG_BIT:
		return _iee_set_key_flag_bit(flag, (struct key *)arg1, (long)arg2,
					     (int)arg3);
#endif
#ifdef CONFIG_IEE_SELINUX_P
	case IEE_SEL_SET_STATUS_PG:
		_iee_set_selinux_status_pg(flag, (struct page *)arg1);
		break;
	case IEE_SEL_SET_ENFORCING:
		_iee_set_selinux_enforcing(flag, (bool)arg1);
		break;
	case IEE_SEL_SET_INITIALIZED:
		_iee_mark_selinux_initialized(flag);
		break;
	case IEE_SEL_SET_POLICY_CAP:
		_iee_set_sel_policy_cap(flag, (unsigned int)arg1, (int)arg2);
		break;
	case IEE_SEL_RCU_ASSIGN_POLICY:
		_iee_sel_rcu_assign_policy(flag, (struct selinux_policy *)arg1,
					   (struct selinux_policy *)arg2);
		break;
#endif
#ifdef CONFIG_PTP
#ifdef CONFIG_IEE_PTRP
	case IEE_OP_UNSET_TOKEN:
		_iee_unset_token(flag, (pte_t *)arg1, (pte_t *)arg2, arg3,
				 (unsigned int)arg4);
		break;
	case IEE_OP_SET_TOKEN:
		_iee_set_token(flag, (pte_t *)arg1, (pte_t *)arg2, arg3,
			       (unsigned int)arg4);
		break;
#endif
	case IEE_OP_SET_PTE:
		_iee_set_pte(flag, (pte_t *)arg1, __pte(arg2));
		break;
	case IEE_OP_SET_PMD:
		_iee_set_pmd(flag, (pmd_t *)arg1, __pmd(arg2));
		break;
	case IEE_OP_SET_PUD:
		_iee_set_pud(flag, (pud_t *)arg1, __pud(arg2));
		break;
	case IEE_OP_SET_P4D:
		_iee_set_p4d(flag, (p4d_t *)arg1, __p4d(arg2));
		break;
	case IEE_OP_SET_PGD:
		_iee_set_pgd(flag, (pgd_t *)arg1, __pgd(arg2));
		break;
	case IEE_OP_SET_PTE_TEXT_POKE:
		_iee_set_pte_text_poke(flag, (pte_t *)arg1, __pte(arg2));
		break;
	case IEE_OP_SET_PMD_IDENT:
		_iee_set_pmd_ident(flag, (pmd_t *)arg1, __pmd(arg2));
		break;
	case IEE_OP_SET_PUD_IDENT:
		_iee_set_pud_ident(flag, (pud_t *)arg1, __pud(arg2));
		break;
	case IEE_OP_SET_SX_PTE:
		_iee_set_sx_pte(flag, (pte_t *)arg1, __pte(arg2));
		break;
#endif
#ifdef CONFIG_VARP
	case IEE_OP_SET_VARP_MODPROBE_PATH:
		_iee_set_varp_modprobe_path(flag, (char *)arg1, (int)arg2,
					   (size_t)arg3, (char *)arg4,
					   (size_t *)arg5);
		break;
#endif
	default:
		panic("iee_dispatch: invalid flag %lu\n", flag);
	}

	return 0;
}

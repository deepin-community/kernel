// SPDX-License-Identifier: GPL-2.0
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Shu Hang <shuh2023@zgclab.edu.cn>
 *          Hu Bing <hubing2023@zgclab.edu.cn>
 */

#include <asm/haoc/haoc.h>

typedef void (*iee_func)(void);
iee_func iee_funcs[] = {
	(iee_func)_iee_memcpy,
	(iee_func)_iee_memset,
	(iee_func)_iee_set_freeptr,
	(iee_func)_iee_test_and_clear_bit,
#ifdef CONFIG_IEE_PTRP
	(iee_func)_iee_set_token_pgd,
	(iee_func)_iee_invalidate_token,
	(iee_func)_iee_validate_token,
#endif
	(iee_func)_iee_set_bitmap_type,
#ifdef CONFIG_CREDP
	(iee_func)_iee_copy_cred,
	(iee_func)_iee_copy_kernel_cred,
	(iee_func)_iee_init_copied_cred,
	(iee_func)_iee_commit_creds,
	(iee_func)_iee_abort_cred,
	(iee_func)_iee_fill_cred_for_session_keyring,
	(iee_func)_iee_override_creds,
	(iee_func)_iee_revert_creds,
	(iee_func)_iee_set_cred_uid,
	(iee_func)_iee_set_cred_gid,
	(iee_func)_iee_set_cred_suid,
	(iee_func)_iee_set_cred_sgid,
	(iee_func)_iee_set_cred_euid,
	(iee_func)_iee_set_cred_egid,
	(iee_func)_iee_set_cred_fsuid,
	(iee_func)_iee_set_cred_fsgid,
	(iee_func)_iee_set_cred_user,
	(iee_func)_iee_set_cred_user_ns,
	(iee_func)_iee_set_cred_group_info,
	(iee_func)_iee_set_cred_securebits,
	(iee_func)_iee_set_cred_cap_inheritable,
	(iee_func)_iee_set_cred_cap_permitted,
	(iee_func)_iee_set_cred_cap_effective,
	(iee_func)_iee_set_cred_cap_bset,
	(iee_func)_iee_set_cred_cap_ambient,
	(iee_func)_iee_set_cred_jit_keyring,
	(iee_func)_iee_set_cred_session_keyring,
	(iee_func)_iee_set_cred_process_keyring,
	(iee_func)_iee_set_cred_thread_keyring,
	(iee_func)_iee_set_cred_request_key_auth,
	(iee_func)_iee_set_cred_non_rcu,
	(iee_func)_iee_set_cred_atomic_set_usage,
	(iee_func)_iee_set_cred_atomic_op_usage,
	(iee_func)_iee_set_cred_security,
	(iee_func)_iee_set_cred_rcu,
	(iee_func)_iee_set_cred_ucounts,
#endif
#ifdef CONFIG_KEYP
	(iee_func)_iee_set_key_union,
	(iee_func)_iee_set_key_struct,
	(iee_func)_iee_set_key_payload,
	(iee_func)_iee_set_key_usage,
	(iee_func)_iee_set_key_serial,
	(iee_func)_iee_set_key_watchers,
	(iee_func)_iee_set_key_user,
	(iee_func)_iee_set_key_security,
	(iee_func)_iee_set_key_expiry,
	(iee_func)_iee_set_key_revoked_at,
	(iee_func)_iee_set_key_last_used_at,
	(iee_func)_iee_set_key_uid,
	(iee_func)_iee_set_key_gid,
	(iee_func)_iee_set_key_perm,
	(iee_func)_iee_set_key_quotalen,
	(iee_func)_iee_set_key_datalen,
	(iee_func)_iee_set_key_state,
	(iee_func)_iee_set_key_magic,
	(iee_func)_iee_set_key_flags,
	(iee_func)_iee_set_key_index_key,
	(iee_func)_iee_set_key_hash,
	(iee_func)_iee_set_key_len_desc,
	(iee_func)_iee_set_key_type,
	(iee_func)_iee_set_key_domain_tag,
	(iee_func)_iee_set_key_description,
	(iee_func)_iee_set_key_restrict_link,
	(iee_func)_iee_set_key_flag_bit,
#endif
#ifdef CONFIG_IEE_SELINUX_P
	(iee_func)_iee_set_selinux_status_pg,
	(iee_func)_iee_set_selinux_enforcing,
	(iee_func)_iee_mark_selinux_initialized,
	(iee_func)_iee_set_sel_policy_cap,
	(iee_func)_iee_sel_rcu_assign_policy,
#endif
#ifdef CONFIG_PTP
#ifdef CONFIG_IEE_PTRP
	(iee_func)_iee_unset_token,
	(iee_func)_iee_set_token,
#endif
	(iee_func)_iee_set_pte,
	(iee_func)_iee_set_pmd,
	(iee_func)_iee_set_pud,
	(iee_func)_iee_set_p4d,
	(iee_func)_iee_set_pgd,
	(iee_func)_iee_set_pte_text_poke,
	(iee_func)_iee_set_pmd_ident,
	(iee_func)_iee_set_pud_ident,
	(iee_func)_iee_set_sx_pte,
#endif
	NULL
};

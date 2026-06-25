// SPDX-License-Identifier: GPL-2.0
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Lyu Jinglin <lvjl2022@zgclab.edu.cn>
 *          Zhang Shiyang <zhangsy2023@zgclab.edu.cn>
 */

#include <asm/haoc/haoc.h>

typedef void (*iee_func)(void);

/*
 * Register IEE handler functions here.
 * IEE gate would find out the specific handler function inside this array
 * using the index that iee_rw_gate() gives, so the arrangement of these
 * IEE functions should correspond one-to-one with the enum entries in haoc-def.h,
 * such as IEE_OP_MEMSET to call _iee_memset().
 */
iee_func iee_funcs[] = {
	(iee_func)_iee_memcpy,
	(iee_func)_iee_memset,
	(iee_func)_iee_set_freeptr,
#ifdef CONFIG_IEE_PTRP
	(iee_func)_iee_set_token_pgd,
	(iee_func)_iee_init_token,
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
	NULL
};

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

/*
 * IEE function dispatcher.
 *
 * Called from iee_rw_gate (asm) on the per-cpu IEE stack with interrupts
 * disabled and CR0.WP cleared. The first argument of every _iee_*()
 * function absorbs @flag, keeping the register convention of the old
 * indirect function table.
 *
 * Direct calls only: no indirect branch (retpoline/objtool clean) and no
 * dispatch table in writable memory.
 */
notrace unsigned long iee_dispatch(int flag, unsigned long arg1,
				   unsigned long arg2, unsigned long arg3)
{
	switch (flag) {
	case IEE_OP_MEMCPY:
		_iee_memcpy(flag, (void *)arg1, (void *)arg2, arg3);
		break;
	case IEE_OP_MEMSET:
		_iee_memset(flag, (void *)arg1, (int)arg2, arg3);
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
	default:
		/*
		 * Unreachable in practice: all flags are compile-time
		 * constants in the iee_*() wrappers. An unknown flag means
		 * memory corruption or ROP into the gate; halt the machine
		 * instead of continuing with CR0.WP cleared on this CPU.
		 */
		panic("iee_dispatch: unknown flag %d\n", flag);
	}
	return 0;
}

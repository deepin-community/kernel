/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _LINUX_IEE_CRED_H
#define _LINUX_IEE_CRED_H

#include <asm/haoc/haoc-def.h>
#include <linux/cred.h>

extern unsigned long long iee_rw_gate(int flag, ...);

static void __maybe_unused iee_copy_cred(struct cred *new)
{
	iee_rw_gate(IEE_OP_COPY_CRED, new);
}

static void __maybe_unused iee_copy_kernel_cred(const struct cred *old,
								struct cred *new)
{
	iee_rw_gate(IEE_OP_COPY_KERNEL_CRED, old, new);
}

static void __maybe_unused iee_init_copied_cred(struct task_struct *new_task,
								const struct cred *new)
{
	iee_rw_gate(IEE_OP_INIT_COPIED_CRED, new_task, new);
}

static void __maybe_unused iee_abort_creds(struct cred *cred)
{
	iee_rw_gate(IEE_OP_ABORT_CRED, cred);
}

static void __maybe_unused iee_fill_cred_for_session_keyring(struct cred *new,
							     const struct cred *old)
{
	iee_rw_gate(IEE_OP_FILL_SESSION_KEYRING_CRED, new, old);
}

static void __maybe_unused iee_commit_creds(const struct cred *new)
{
	iee_rw_gate(IEE_OP_COMMIT_CRED, new);
}

static void __maybe_unused iee_set_cred_uid(struct cred *cred, kuid_t uid)
{
    if(!haoc_enabled)
    {
        cred->uid = uid;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_UID, cred, uid);
}

static void __maybe_unused iee_set_cred_gid(struct cred *cred, kgid_t gid)
{
    if(!haoc_enabled)
    {
        cred->gid = gid;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_GID, cred, gid);
}

static void __maybe_unused iee_set_cred_suid(struct cred *cred, kuid_t suid)
{
    if(!haoc_enabled)
    {
        cred->suid = suid;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_SUID, cred, suid);
}

static void __maybe_unused iee_set_cred_sgid(struct cred *cred, kgid_t sgid)
{
    if(!haoc_enabled)
    {
        cred->sgid = sgid;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_SGID, cred, sgid);
}

static void __maybe_unused iee_set_cred_euid(struct cred *cred, kuid_t euid)
{
    if(!haoc_enabled)
    {
        cred->euid = euid;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_EUID, cred, euid);
}

static void __maybe_unused iee_set_cred_egid(struct cred *cred, kgid_t egid)
{
    if(!haoc_enabled)
    {
        cred->egid = egid;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_EGID, cred, egid);
}

static void __maybe_unused iee_set_cred_fsuid(struct cred *cred, kuid_t fsuid)
{
    if(!haoc_enabled)
    {
        cred->fsuid = fsuid;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_FSUID, cred, fsuid);
}

static void __maybe_unused iee_set_cred_fsgid(struct cred *cred, kgid_t fsgid)
{
    if(!haoc_enabled)
    {
        cred->fsgid = fsgid;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_FSGID, cred, fsgid);
}

static void __maybe_unused iee_set_cred_user(struct cred *cred, struct user_struct *user)
{
    if(!haoc_enabled)
    {
        cred->user = user;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_USER, cred, user);
}

static void __maybe_unused iee_set_cred_user_ns(struct cred *cred, struct user_namespace *user_ns)
{
    if(!haoc_enabled)
    {
        cred->user_ns = user_ns;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_USER_NS, cred, user_ns);
}

static void __maybe_unused iee_set_cred_ucounts(struct cred *cred, struct ucounts *ucounts)
{
    if(!haoc_enabled)
    {
        cred->ucounts = ucounts;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_UCOUNTS, cred, ucounts);
}

static void __maybe_unused iee_set_cred_group_info(struct cred *cred, struct group_info *group_info)
{
    if(!haoc_enabled)
    {
        cred->group_info = group_info;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_GROUP_INFO, cred, group_info);
}

static void __maybe_unused iee_set_cred_securebits(struct cred *cred,
			unsigned int securebits)
{
    if(!haoc_enabled)
    {
        cred->securebits = securebits;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_SECUREBITS, cred, securebits);
}

static void __maybe_unused iee_set_cred_cap_inheritable(struct cred *cred,
			kernel_cap_t cap_inheritable)
{
    if(!haoc_enabled)
    {
        cred->cap_inheritable = cap_inheritable;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_CAP_INHER, cred, cap_inheritable);
}

static void __maybe_unused iee_set_cred_cap_permitted(struct cred *cred, kernel_cap_t cap_permitted)
{
    if(!haoc_enabled)
    {
        cred->cap_permitted = cap_permitted;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_CAP_PERM, cred, cap_permitted);
}

static void __maybe_unused iee_set_cred_cap_effective(struct cred *cred, kernel_cap_t cap_effective)
{
    if(!haoc_enabled)
    {
        cred->cap_effective = cap_effective;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_CAP_EFFECT, cred, cap_effective);
}

static void __maybe_unused iee_set_cred_cap_bset(struct cred *cred, kernel_cap_t cap_bset)
{
    if(!haoc_enabled)
    {
        cred->cap_bset = cap_bset;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_CAP_BSET, cred, cap_bset);
}

static void __maybe_unused iee_set_cred_cap_ambient(struct cred *cred, kernel_cap_t cap_ambient)
{
    if(!haoc_enabled)
    {
        cred->cap_ambient = cap_ambient;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_CAP_AMBIENT, cred, cap_ambient);
}

static void __maybe_unused iee_set_cred_atomic_set_usage(struct cred *cred, int i)
{
    if(!haoc_enabled)
    {
        atomic_long_set(&cred->usage, i);
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_ATSET_USAGE, cred, i);
}

static void __maybe_unused iee_set_cred_rcu(struct cred *cred, struct rcu_head *rcu)
{
	iee_rw_gate(IEE_OP_SET_CRED_RCU, cred, rcu);
}

#ifdef CONFIG_KEYS
static void __maybe_unused iee_set_cred_jit_keyring(struct cred *cred, unsigned char jit_keyring)
{
    if(!haoc_enabled)
    {
        cred->jit_keyring = jit_keyring;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_JIT_KEYRING, cred, jit_keyring);
}

static void __maybe_unused iee_set_cred_session_keyring(struct cred *cred,
			struct key *session_keyring)
{
    if(!haoc_enabled)
    {
        cred->session_keyring = session_keyring;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_SESS_KEYRING, cred, session_keyring);
}

static void __maybe_unused iee_set_cred_process_keyring(struct cred *cred,
			struct key *process_keyring)
{
    if(!haoc_enabled)
    {
        cred->process_keyring = process_keyring;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_PROC_KEYRING, cred, process_keyring);
}

static void __maybe_unused iee_set_cred_thread_keyring(struct cred *cred,
			struct key *thread_keyring)
{
    if(!haoc_enabled)
    {
        cred->thread_keyring = thread_keyring;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_THREAD_KEYRING, cred, thread_keyring);
}

static void __maybe_unused iee_set_cred_request_key_auth(struct cred *cred,
			struct key *request_key_auth)
{
    if(!haoc_enabled)
    {
        cred->request_key_auth = request_key_auth;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_REQ_KEYRING, cred, request_key_auth);
}
#endif

#ifdef CONFIG_SECURITY
static void __maybe_unused iee_set_cred_security(struct cred *cred, void *security)
{
    if(!haoc_enabled)
    {
        cred->security = security;
        return;
    }
	iee_rw_gate(IEE_OP_SET_CRED_SECURITY, cred, security);
}
#endif

#endif
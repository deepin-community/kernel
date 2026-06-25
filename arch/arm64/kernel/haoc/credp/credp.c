// SPDX-License-Identifier: GPL-2.0
#include <asm/haoc/haoc-def.h>
#include <asm/haoc/haoc.h>
#include <asm/haoc/iee.h>
#include <asm/haoc/iee-cred.h>
#include <linux/key.h>
#include <linux/user_namespace.h>
#include <linux/user_namespace.h>
extern struct cred init_cred;

static inline void iee_verify_cred_type(const struct cred *cred)
{
	if (!iee_init_done || !haoc_bitmap_ready)
		return;

	iee_verify_type((unsigned long)cred, IEE_CRED, "cred");
}

static inline void iee_verify_current_cred_type(struct cred *cred)
{
	iee_verify_cred_type(cred);
	iee_verify_cred();
}

static inline void iee_verify_new_cred_type(struct cred *cred)
{
	iee_verify_cred_type(cred);
	iee_verify_update_cred(cred);
}

void __iee_code _iee_set_cred_rcu(unsigned long __unused, struct cred *cred,
						struct rcu_head *rcu)
{
	iee_verify_current_cred_type(cred);
	cred = __ptr_to_iee(cred);
	*((struct rcu_head **)(&(cred->rcu.func))) = rcu;
}

void __iee_code _iee_set_cred_security(unsigned long __unused, struct cred *cred,
						void *security)
{
	iee_verify_current_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->security = security;
}

bool __iee_code _iee_set_cred_atomic_op_usage(unsigned long __unused,
						struct cred *cred, int flag, int nr)
{
	iee_verify_current_cred_type(cred);
	cred = __ptr_to_iee(cred);
	switch (flag) {
	case AT_ADD: {
		atomic_long_add(nr, &cred->usage);
		return 0;
	}
	case AT_INC_NOT_ZERO: {
		return atomic_long_inc_not_zero(&cred->usage);
	}
	case AT_SUB_AND_TEST: {
		return atomic_long_sub_and_test(nr, &cred->usage);
	}
	}
	return 0;
}

void __iee_code _iee_set_cred_atomic_set_usage(unsigned long __unused,
						struct cred *cred, int i)
{
	iee_verify_current_cred_type(cred);
	cred = __ptr_to_iee(cred);
	atomic_long_set(&cred->usage, i);
}

void __iee_code _iee_set_cred_non_rcu(unsigned long __unused, struct cred *cred,
						int non_rcu)
{
	iee_verify_current_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->non_rcu = non_rcu;
}

void __iee_code _iee_set_cred_session_keyring(unsigned long __unused,
						struct cred *cred, struct key *session_keyring)
{
	iee_verify_cred_type(cred);
	iee_verify_cred();
	cred = __ptr_to_iee(cred);
	cred->session_keyring = session_keyring;
}

void __iee_code _iee_set_cred_process_keyring(unsigned long __unused,
						struct cred *cred, struct key *process_keyring)
{
	iee_verify_new_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->process_keyring = process_keyring;
}

void __iee_code _iee_set_cred_thread_keyring(unsigned long __unused,
						struct cred *cred, struct key *thread_keyring)
{
	iee_verify_current_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->thread_keyring = thread_keyring;
}

void __iee_code _iee_set_cred_request_key_auth(unsigned long __unused,
						struct cred *cred, struct key *request_key_auth)
{
	iee_verify_new_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->request_key_auth = request_key_auth;
}

void __iee_code _iee_set_cred_jit_keyring(unsigned long __unused,
						struct cred *cred, unsigned char jit_keyring)
{
	iee_verify_new_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->jit_keyring = jit_keyring;
}

void __iee_code _iee_set_cred_cap_inheritable(unsigned long __unused,
						struct cred *cred, kernel_cap_t cap_inheritable)
{
	iee_verify_new_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->cap_inheritable = cap_inheritable;
}

void __iee_code _iee_set_cred_cap_permitted(unsigned long __unused,
						struct cred *cred, kernel_cap_t cap_permitted)
{
	iee_verify_new_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->cap_permitted = cap_permitted;
}

void __iee_code _iee_set_cred_cap_effective(unsigned long __unused,
						struct cred *cred, kernel_cap_t cap_effective)
{
	iee_verify_new_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->cap_effective = cap_effective;
}

void __iee_code _iee_set_cred_cap_bset(unsigned long __unused,
						struct cred *cred, kernel_cap_t cap_bset)
{
	iee_verify_new_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->cap_bset = cap_bset;
}

void __iee_code _iee_set_cred_cap_ambient(unsigned long __unused,
						struct cred *cred, kernel_cap_t cap_ambient)
{
	iee_verify_new_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->cap_ambient = cap_ambient;
}

void __iee_code _iee_set_cred_securebits(unsigned long __unused,
						struct cred *cred, unsigned int securebits)
{
	iee_verify_new_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->securebits = securebits;
}

void __iee_code _iee_set_cred_group_info(unsigned long __unused,
						struct cred *cred, struct group_info *group_info)
{
	iee_verify_current_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->group_info = group_info;
}

void __iee_code _iee_set_cred_ucounts(unsigned long __unused, struct cred *cred,
						struct ucounts *ucounts)
{
	iee_verify_new_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->ucounts = ucounts;
}

void __iee_code _iee_set_cred_user_ns(unsigned long __unused, struct cred *cred,
						struct user_namespace *user_ns)
{
	iee_verify_new_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->user_ns = user_ns;
}

void __iee_code _iee_set_cred_user(unsigned long __unused, struct cred *cred,
						struct user_struct *user)
{
	iee_verify_new_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->user = user;
}

void __iee_code _iee_set_cred_fsgid(unsigned long __unused, struct cred *cred,
						kgid_t fsgid)
{
	iee_verify_new_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->fsgid = fsgid;
}

void __iee_code _iee_set_cred_fsuid(unsigned long __unused, struct cred *cred,
						kuid_t fsuid)
{
	iee_verify_new_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->fsuid = fsuid;
}

void __iee_code _iee_set_cred_egid(unsigned long __unused, struct cred *cred,
						kgid_t egid)
{
	iee_verify_new_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->egid = egid;
}

void __iee_code _iee_set_cred_euid(unsigned long __unused, struct cred *cred,
						kuid_t euid)
{
	iee_verify_new_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->euid = euid;
}

void __iee_code _iee_set_cred_sgid(unsigned long __unused, struct cred *cred,
						kgid_t sgid)
{
	iee_verify_new_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->sgid = sgid;
}

void __iee_code _iee_set_cred_suid(unsigned long __unused, struct cred *cred,
						kuid_t suid)
{
	iee_verify_new_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->suid = suid;
}

extern void _iee_memcpy(unsigned long __unused, void *dst, void *src, size_t n);

/* Only used to copy privilege creds like init_cred. */
void __iee_code _iee_copy_kernel_cred(unsigned long __unused, const struct cred *old,
	struct cred *new)
{
	struct rcu_head *rcu = (struct rcu_head *)(new->rcu.func);
	struct cred *_new = __ptr_to_iee(new);

	iee_verify_cred();
	iee_verify_cred_type(old);
	iee_verify_cred_type(new);

	if (!uid_eq(current_uid(), init_cred.uid))
		panic("IEE: calling prepare_kernel_cred by unprivileged process.");

#ifdef CONFIG_IEE_PTRP
	if (haoc_enabled) {
		struct task_token *token = (struct task_token *)iee_get_task_token(current);

		/* Would verify this field in commit_creds. */
		token->new_cred = new;
	}
#endif

	_iee_memcpy(0, new, (struct cred *)old, sizeof(struct cred));
	_new->non_rcu = 0;
	atomic_long_set(&_new->usage, 1);
	*(struct rcu_head **)(&(_new->rcu.func)) = rcu;
	*(struct rcu_head *)(_new->rcu.func) = *(struct rcu_head *)(old->rcu.func);
}

/* Used only inside copy_creds. */
void _iee_init_copied_cred(unsigned long __unused,
		struct task_struct *new_task, struct cred *new)
{
	iee_verify_cred();
	iee_verify_cred_type(new);

#ifdef CONFIG_IEE_PTRP
	if (haoc_enabled) {
		struct task_token *old_task_token =
			(struct task_token *)iee_get_task_token(current);
		struct task_token *new_task_token =
			(struct task_token *)iee_get_task_token(new_task);

		if (!old_task_token->valid || !new_task_token->valid)
			panic("IEE: (%s) Invalid Token.", __func__);
		if (old_task_token->new_cred != new)
			panic("IEE: (%s) token error. token new cred 0x%llx, new 0%llx",
			      __func__, (u64)old_task_token->new_cred, (u64)new);
		/* Update token info of new task by current task token. */
		old_task_token->new_cred = NULL;
		new_task_token->curr_cred = new;
	}
#endif

	new_task->cred = new_task->real_cred = new;
}

void _iee_commit_creds(unsigned long __unused, const struct cred *new)
{
	struct task_struct *task = current;

	iee_verify_cred();
	iee_verify_cred_type(new);

#ifdef CONFIG_IEE_PTRP
	if (haoc_enabled) {
		struct task_token *token = (struct task_token *)iee_get_task_token(task);

		if (token->new_cred != new)
			panic("IEE: (%s) Invalid cred 0x%llx. token->new_cred 0x%llx",
			      __func__, (u64)new, (u64)token->new_cred);
		/* task->cred shall be updated once. */
		token->new_cred = NULL;
		token->curr_cred = new;
	}
#endif

	rcu_assign_pointer(task->real_cred, new);
	rcu_assign_pointer(task->cred, new);
}

void _iee_abort_cred(unsigned long __unused, const struct cred *cred)
{
	iee_verify_cred();
	iee_verify_cred_type(cred);

#ifdef CONFIG_IEE_PTRP
	if (haoc_enabled) {
		struct task_token *token = (struct task_token *)iee_get_task_token(current);

		token->new_cred = NULL;
	}
#endif
}

void _iee_fill_cred_for_session_keyring(unsigned long __unused, struct cred *new,
						const struct cred *old)
{
	iee_verify_cred_type(new);
	iee_verify_cred_type(old);
	iee_verify_cred();

#ifdef CONFIG_IEE_PTRP
	if (haoc_enabled) {
		struct task_token *token = (struct task_token *)iee_get_task_token(current);

		token->new_cred = new;
	}
#endif

	new = __ptr_to_iee(new);
	new->uid = old->uid;
	new->euid = old->euid;
	new->suid = old->suid;
	new->fsuid = old->fsuid;
	new->gid = old->gid;
	new->egid = old->egid;
	new->sgid = old->sgid;
	new->fsgid = old->fsgid;
	new->user = get_uid(old->user);
	new->ucounts = old->ucounts;
	new->user_ns = get_user_ns(old->user_ns);
	new->group_info = get_group_info(old->group_info);

	new->securebits = old->securebits;
	new->cap_inheritable = old->cap_inheritable;
	new->cap_permitted = old->cap_permitted;
	new->cap_effective = old->cap_effective;
	new->cap_ambient = old->cap_ambient;
	new->cap_bset = old->cap_bset;

	new->jit_keyring = old->jit_keyring;
	new->thread_keyring = key_get(old->thread_keyring);
	new->process_keyring = key_get(old->process_keyring);
}

void _iee_copy_cred(unsigned long __unused, struct cred *new)
{
	struct rcu_head *rcu = (struct rcu_head *)(new->rcu.func);
	struct cred *_new = __ptr_to_iee(new);
	/* Get old cred inside IEE is safer. */
	const struct cred *old = current_cred();

	iee_verify_cred();
	iee_verify_cred_type(new);

#ifdef CONFIG_IEE_PTRP
	if (haoc_enabled) {
		struct task_token *token = (struct task_token *)iee_get_task_token(current);

		/* Would verify this field in commit_creds. */
		token->new_cred = new;
	}
#endif

	_iee_memcpy(0, new, (struct cred *)old, sizeof(struct cred));
	_new->non_rcu = 0;
	atomic_long_set(&_new->usage, 1);
	*(struct rcu_head **)(&(_new->rcu.func)) = rcu;
	*(struct rcu_head *)(_new->rcu.func) = *(struct rcu_head *)(old->rcu.func);
}

void __iee_code _iee_set_cred_gid(unsigned long __unused, struct cred *cred,
						kgid_t gid)
{
	iee_verify_new_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->gid = gid;
}

void __iee_code _iee_set_cred_uid(unsigned long __unused, struct cred *cred,
						kuid_t uid)
{
	iee_verify_new_cred_type(cred);
	cred = __ptr_to_iee(cred);
	cred->uid = uid;
}

void __iee_code _iee_override_creds(unsigned long __unused, const struct cred *new)
{
	iee_verify_cred();
	iee_verify_cred_type(new);

#ifdef CONFIG_IEE_PTRP
	if (haoc_enabled) {
		struct task_token *token = (struct task_token *)iee_get_task_token(current);

		token->curr_cred = new;
	}
#endif

	rcu_assign_pointer(current->cred, new);
}

void __iee_code _iee_revert_creds(unsigned long __unused, const struct cred *old)
{
	iee_verify_cred();
	iee_verify_cred_type(old);

#ifdef CONFIG_IEE_PTRP
	if (haoc_enabled) {
		struct task_token *token = (struct task_token *)iee_get_task_token(current);

		token->curr_cred = old;
	}
#endif

	rcu_assign_pointer(current->cred, old);
}

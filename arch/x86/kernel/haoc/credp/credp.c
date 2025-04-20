// SPDX-License-Identifier: GPL-2.0
#include <asm/haoc/haoc-def.h>
#include <asm/haoc/iee.h>
#include <linux/cred.h>

#include <linux/mm.h>
#include <asm/haoc/iee-func.h>
#include <linux/slab.h>
#include "slab.h"

extern struct cred init_cred;

void _iee_set_cred_rcu(unsigned long __unused, struct cred *cred, struct rcu_head *rcu)
{
	*((struct rcu_head **)(&(cred->rcu.func))) = rcu;
}

void _iee_set_cred_security(unsigned long __unused, struct cred *cred, void *security)
{
	cred->security = security;
}

unsigned long _iee_set_cred_atomic_op_usage(unsigned long __unused,
			struct cred *cred, int flag, int nr)
{
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

void _iee_set_cred_atomic_set_usage(unsigned long __unused, struct cred *cred, int i)
{
	atomic_long_set(&cred->usage, i);
}

void _iee_set_cred_non_rcu(unsigned long __unused, struct cred *cred, int non_rcu)
{
	cred->non_rcu = non_rcu;
}

void _iee_set_cred_session_keyring(unsigned long __unused, struct cred *cred,
			struct key *session_keyring)
{
	cred->session_keyring = session_keyring;
}

void _iee_set_cred_process_keyring(unsigned long __unused, struct cred *cred,
			struct key *process_keyring)
{
	cred->process_keyring = process_keyring;
}

void _iee_set_cred_thread_keyring(unsigned long __unused, struct cred *cred,
			struct key *thread_keyring)
{
	cred->thread_keyring = thread_keyring;
}

void _iee_set_cred_request_key_auth(unsigned long __unused, struct cred *cred,
			struct key *request_key_auth)
{
	cred->request_key_auth = request_key_auth;
}

void _iee_set_cred_jit_keyring(unsigned long __unused, struct cred *cred, unsigned char jit_keyring)
{
	cred->jit_keyring = jit_keyring;
}

void _iee_set_cred_cap_inheritable(unsigned long __unused, struct cred *cred,
			kernel_cap_t cap_inheritable)
{
	cred->cap_inheritable = cap_inheritable;
}

void _iee_set_cred_cap_permitted(unsigned long __unused, struct cred *cred,
			kernel_cap_t cap_permitted)
{
	cred->cap_permitted = cap_permitted;
}

void _iee_set_cred_cap_effective(unsigned long __unused, struct cred *cred,
			kernel_cap_t cap_effective)
{
	cred->cap_effective = cap_effective;
}

void _iee_set_cred_cap_bset(unsigned long __unused, struct cred *cred, kernel_cap_t cap_bset)
{
	cred->cap_bset = cap_bset;
}

void _iee_set_cred_cap_ambient(unsigned long __unused, struct cred *cred, kernel_cap_t cap_ambient)
{
	cred->cap_ambient = cap_ambient;
}

void _iee_set_cred_securebits(unsigned long __unused, struct cred *cred,
			unsigned int securebits)
{
	cred->securebits = securebits;
}

void _iee_set_cred_group_info(unsigned long __unused, struct cred *cred,
			struct group_info *group_info)
{
	cred->group_info = group_info;
}

void _iee_set_cred_ucounts(unsigned long __unused, struct cred *cred, struct ucounts *ucounts)
{
	cred->ucounts = ucounts;
}

void _iee_set_cred_user_ns(unsigned long __unused, struct cred *cred,
			struct user_namespace *user_ns)
{
	cred->user_ns = user_ns;
}

void _iee_set_cred_user(unsigned long __unused, struct cred *cred, struct user_struct *user)
{
	cred->user = user;
}

void _iee_set_cred_fsgid(unsigned long __unused, struct cred *cred, kgid_t fsgid)
{
	cred->fsgid = fsgid;
}

void _iee_set_cred_fsuid(unsigned long __unused, struct cred *cred, kuid_t fsuid)
{
	cred->fsuid = fsuid;
}

void _iee_set_cred_egid(unsigned long __unused, struct cred *cred, kgid_t egid)
{
	cred->egid = egid;
}

void _iee_set_cred_euid(unsigned long __unused, struct cred *cred, kuid_t euid)
{
	cred->euid = euid;
}

void _iee_set_cred_sgid(unsigned long __unused, struct cred *cred, kgid_t sgid)
{
	cred->sgid = sgid;
}

void _iee_set_cred_suid(unsigned long __unused, struct cred *cred, kuid_t suid)
{
	cred->suid = suid;
}

void _iee_copy_cred(unsigned long __unused, struct cred *old, struct cred *new)
{
	if (new == &init_cred)
		panic("copy_cred for init_cred: %lx\n", (unsigned long)new);
	struct rcu_head *rcu = (struct rcu_head *)(new->rcu.func);

	memcpy(new, old, sizeof(struct cred));
	*(struct rcu_head **)(&(new->rcu.func)) = rcu;
	*(struct rcu_head *)(new->rcu.func) = *(struct rcu_head *)(old->rcu.func);
}

void _iee_set_cred_gid(unsigned long __unused, struct cred *cred, kgid_t gid)
{
	cred->gid = gid;
}

void _iee_set_cred_uid(unsigned long __unused, struct cred *cred, kuid_t uid)
{
	cred->uid = uid;
}

struct iee_free_slab_work {
	struct work_struct work;
	struct kmem_cache *s;
	struct slab *slab;
};

void iee_free_cred_slab(struct work_struct *work)
{
	struct iee_free_slab_work *iee_free_slab_work =
				container_of(work, struct iee_free_slab_work, work);
	struct slab *slab = iee_free_slab_work->slab;
	struct folio *folio = slab_folio(slab);
	int order = folio_order(folio);

	unset_iee_page((unsigned long)page_address(folio_page(slab_folio(slab), 0)), order);
	__free_pages(&folio->page, order);
	kfree(iee_free_slab_work);
}

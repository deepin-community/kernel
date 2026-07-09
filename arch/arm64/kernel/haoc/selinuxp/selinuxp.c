// SPDX-License-Identifier: GPL-2.0
#include <linux/mutex.h>
#include <asm/haoc/iee.h>
#include "security.h"
#include "ss/services.h"
#include <asm/haoc/haoc-bitmap.h>

static inline void iee_verify_selinux_type(unsigned long va)
{
	iee_verify_type(va, IEE_SELINUX, "selinux");
}

void __iee_code _iee_set_selinux_status_pg(unsigned long iee_offset,
					struct page *new_page)
{
	*__ptr_to_iee(&selinux_state.status_page) = new_page;
}

void __iee_code _iee_set_selinux_enforcing(unsigned long iee_offset, bool value)
{
	*__ptr_to_iee(&selinux_state.enforcing) = value;
}

void __iee_code _iee_mark_selinux_initialized(unsigned long iee_offset)
{
	/* do a synchronized write to avoid race conditions */
	smp_store_release(__ptr_to_iee(&(selinux_state.initialized)), true);
	if (haoc_enabled)
		pr_info("HAOC: Mark selinux initialized.");
}

void __iee_code _iee_set_sel_policy_cap(unsigned long iee_offset,
					unsigned int idx, int cap)
{
	*__ptr_to_iee(&(selinux_state.policycap[idx])) = cap;
}

/*
 * Please make sure param iee_new_policy is from policy_jar memcache.
 * Need to free new_policy after calling this func as it's only used to
 * trans data from kernel.
 */
void __iee_code _iee_sel_rcu_assign_policy(unsigned long iee_offset,
					struct selinux_policy *new_policy,
					struct selinux_policy *iee_new_policy)
{
	/* TODO: Verify information from incoming policy. */
	iee_verify_selinux_type((unsigned long)iee_new_policy);

	/* Copy data from kernel to new allocated policy struct inside iee. */
	memcpy(__ptr_to_iee(iee_new_policy), new_policy, sizeof(struct selinux_policy));
	rcu_assign_pointer(*__ptr_to_iee(&selinux_state.policy), iee_new_policy);
}

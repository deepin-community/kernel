// SPDX-License-Identifier: GPL-2.0
#include <linux/mutex.h>
#include "security.h"
#include "ss/services.h"
#include <asm/haoc/haoc-bitmap.h>
#include <asm/haoc/iee-func.h>
#include <asm/haoc/iee-selinux.h>

void _iee_set_selinux_status_pg(unsigned long iee_offset, struct page *new_page)
{
	selinux_state.status_page = new_page;
}

void _iee_set_selinux_enforcing(unsigned long iee_offset, bool value)
{
	selinux_state.enforcing = value;
}

void _iee_mark_selinux_initialized(unsigned long iee_offset)
{
	/* do a synchronized write to avoid race conditions */
	smp_store_release(((bool *)&(selinux_state.initialized)), true);
	pr_info("IEE: Mark selinux initialized.");
}

void _iee_set_sel_policy_cap(unsigned long iee_offset, unsigned int idx, int cap)
{
	selinux_state.policycap[idx] = cap;
}

/*
 * Please make sure param iee_new_policy is from policy_jar memcache.
 * Need to free new_policy after calling this func as it's only used to
 * trans data from kernel.
 */
void _iee_sel_rcu_assign_policy(unsigned long iee_offset, struct selinux_policy *new_policy,
					struct selinux_policy *iee_new_policy)
{
	iee_verify_type(__pa(iee_new_policy), IEE_POLICY, "policy");
	/* TODO: Verify information from incoming policy. */

	/* Copy data from kernel to new allocated policy struct inside iee. */
	memcpy(iee_new_policy, new_policy, sizeof(struct selinux_policy));
	rcu_assign_pointer(selinux_state.policy, iee_new_policy);
	pr_info("IEE: assigned rcu pointer selinux_state.policy.");
}

void __init iee_selinuxp_init(void)
{
	unsigned long start, end;
	int num_pages;

	start = (unsigned long)__iee_selinux_data_start;
	end = (unsigned long)__iee_selinux_data_end;
	num_pages = (end - start) / PAGE_SIZE;
	set_iee_pages(start, num_pages, IEE_SIP_DATA);
}

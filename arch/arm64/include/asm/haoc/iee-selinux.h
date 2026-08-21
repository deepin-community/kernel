/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IEE_SELINUX_P_H
#define _LINUX_IEE_SELINUX_P_H

#include <asm/haoc/haoc-def.h>
#include <linux/mutex.h>
#include "security.h"
#include "ss/services.h"

extern struct kmem_cache *policy_jar;
extern unsigned long long iee_rw_gate(int flag, ...);

static inline struct mutex *iee_get_selinux_policy_lock(void)
{
	return (struct mutex *)(selinux_state.policy_mutex.owner.counter);
}

static inline struct mutex *iee_get_selinux_status_lock(void)
{
	return (struct mutex *)(selinux_state.status_lock.owner.counter);
}

static inline void iee_set_selinux_status_pg(struct page *new_page)
{
	iee_rw_gate(IEE_SEL_SET_STATUS_PG, new_page);
}

static inline void enforcing_set(bool value)
{
	iee_rw_gate(IEE_SEL_SET_ENFORCING, value);
}

static inline void selinux_mark_initialized(void)
{
	iee_rw_gate(IEE_SEL_SET_INITIALIZED);
}

static inline void iee_set_sel_policy_cap(unsigned int idx, int cap)
{
	iee_rw_gate(IEE_SEL_SET_POLICY_CAP, idx, cap);
}

static inline void iee_sel_rcu_assign_policy(struct selinux_policy *new_policy,
				      struct selinux_policy *iee_new_policy)
{
	iee_rw_gate(IEE_SEL_RCU_ASSIGN_POLICY, new_policy, iee_new_policy);
}

#endif

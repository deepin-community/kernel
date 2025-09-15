/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __NETNS_CORE_H__
#define __NETNS_CORE_H__

#include <linux/types.h>
#include <linux/deepin_kabi.h>

struct ctl_table_header;
struct prot_inuse;
struct cpumask;

struct netns_core {
	/* core sysctls */
	struct ctl_table_header	*sysctl_hdr;

	int	sysctl_somaxconn;
	u8	sysctl_txrehash;

#ifdef CONFIG_PROC_FS
	struct prot_inuse __percpu *prot_inuse;
#endif

#if IS_ENABLED(CONFIG_RPS) && IS_ENABLED(CONFIG_SYSCTL)
	struct cpumask *rps_default_mask;
#endif

	DEEPIN_KABI_RESERVE(1)
	DEEPIN_KABI_RESERVE(2)
	DEEPIN_KABI_RESERVE(3)
	DEEPIN_KABI_RESERVE(4)
};

#endif

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __NETNS_SMC_H__
#define __NETNS_SMC_H__
#include <linux/mutex.h>
#include <linux/percpu.h>

struct smc_stats_rsn;
struct smc_stats;
struct netns_smc {
	/* per cpu counters for SMC */
	struct smc_stats __percpu	*smc_stats;
	/* protect fback_rsn */
	struct mutex			mutex_fback_rsn;
	struct smc_stats_rsn		*fback_rsn;

	bool				limit_smc_hs;	/* constraint on handshake */
#ifdef CONFIG_SYSCTL
	struct ctl_table_header		*smc_hdr;
#endif
	unsigned int			sysctl_autocorking_size;
	unsigned int			sysctl_smcr_buf_type;
	int				sysctl_smcr_testlink_time;
	int				sysctl_wmem;
	int				sysctl_rmem;

	DEEPIN_KABI_RESERVE(1)
	DEEPIN_KABI_RESERVE(2)
	DEEPIN_KABI_RESERVE(3)
	DEEPIN_KABI_RESERVE(4)
	DEEPIN_KABI_RESERVE(5)
	DEEPIN_KABI_RESERVE(6)
	DEEPIN_KABI_RESERVE(7)
	DEEPIN_KABI_RESERVE(8)
	DEEPIN_KABI_RESERVE(9)
	DEEPIN_KABI_RESERVE(10)
	DEEPIN_KABI_RESERVE(11)
	DEEPIN_KABI_RESERVE(12)
	DEEPIN_KABI_RESERVE(13)
	DEEPIN_KABI_RESERVE(14)
	DEEPIN_KABI_RESERVE(15)
	DEEPIN_KABI_RESERVE(16)
	DEEPIN_KABI_RESERVE(17)
	DEEPIN_KABI_RESERVE(18)
	DEEPIN_KABI_RESERVE(19)
	DEEPIN_KABI_RESERVE(20)
	DEEPIN_KABI_RESERVE(21)
	DEEPIN_KABI_RESERVE(22)
	DEEPIN_KABI_RESERVE(23)
	DEEPIN_KABI_RESERVE(24)
	DEEPIN_KABI_RESERVE(25)
	DEEPIN_KABI_RESERVE(26)
	DEEPIN_KABI_RESERVE(27)
	DEEPIN_KABI_RESERVE(28)
	DEEPIN_KABI_RESERVE(29)
	DEEPIN_KABI_RESERVE(30)
	DEEPIN_KABI_RESERVE(31)
	DEEPIN_KABI_RESERVE(32)
};
#endif

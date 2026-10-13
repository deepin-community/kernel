/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _NETNS_NFTABLES_H_
#define _NETNS_NFTABLES_H_

#include <linux/deepin_kabi.h>

struct netns_nftables {
	u8			gencursor;
	DEEPIN_KABI_USE(1, unsigned int base_seq)
	DEEPIN_KABI_RESERVE(2)
};

#endif

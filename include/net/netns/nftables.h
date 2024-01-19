/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _NETNS_NFTABLES_H_
#define _NETNS_NFTABLES_H_
#include <linux/kabi.h>
struct netns_nftables {
	u8			gencursor;
	KABI_RESERVE(1)
};

#endif

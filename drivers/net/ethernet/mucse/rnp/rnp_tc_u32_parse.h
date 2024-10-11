/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#ifndef __RNP_TC_U32_PARSE_H__
#define __RNP_TC_U32_PARSE_H__
#include "rnp.h"

struct rnp_match_parser {
	int off; /* the skb offset begin form the 12 bytes mac_type */
	/* parse the value/mask to realy value*/
	int (*val)(struct rnp_fdir_filter *f, __be32 val, __be32 mask);
};
inline void ip_print(u32 ip, bool src_true)
{
	printk(KERN_DEBUG "%s_ip is %d.%d.%d.%d \n", src_true ? "src" : "dst",
	       ip & 0xff, ip >> 8 & 0xff, ip >> 16 & 0xff, ip >> 24 & 0xff);
}
/* Ipv4 Rule Parse */
static inline int rnp_fill_ipv4_src_ip(struct rnp_fdir_filter *f, __be32 val,
				       __be32 mask)
{
	memcpy(&f->filter.formatted.src_ip[0], &val, sizeof(u32));
	memcpy(&f->filter.formatted.src_ip_mask[0], &mask, sizeof(u32));

	f->filter.formatted.flow_type = RNP_ATR_FLOW_TYPE_IPV4;
	f->filter.layer2_formate.proto = htons(ETH_P_IP);

	ip_print(f->filter.formatted.src_ip[0], true);
	printk(KERN_DEBUG "ip mask is 0x%.2x\n",
	       f->filter.formatted.src_ip_mask[0]);
	return 0;
}

static inline int rnp_fill_ipv4_dst_ip(struct rnp_fdir_filter *f, __be32 val,
				       __be32 mask)
{
	memcpy(&f->filter.formatted.dst_ip[0], &val, sizeof(u32));
	memcpy(&f->filter.formatted.dst_ip_mask[0], &mask, sizeof(u32));

	f->filter.formatted.flow_type = RNP_ATR_FLOW_TYPE_IPV4;
	f->filter.layer2_formate.proto = htons(ETH_P_IP);

	ip_print(f->filter.formatted.dst_ip[0], false);
	printk(KERN_DEBUG "ip mask is 0x%.2x\n",
	       f->filter.formatted.dst_ip_mask[0]);

	return 0;
}

static const struct rnp_match_parser rnp_ipv4_parser[] = {
	{ .off = 12, .val = rnp_fill_ipv4_src_ip },
	{ .off = 16, .val = rnp_fill_ipv4_dst_ip },
	{ .val = NULL }
};

#endif

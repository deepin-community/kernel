/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2022 - 2025 Mucse Corporation. */

#ifndef __RNPM_TC_U32_PARSE_H__
#define __RNPM_TC_U32_PARSE_H__
#include "rnpm.h"

struct rnpm_match_parser {
	int off;
	int (*val)(struct rnpm_fdir_filter *f, __be32 val, __be32 mask);
};

/* Ipv4 Rule Parse */
static inline int rnpm_fill_ipv4_src_ip(struct rnpm_fdir_filter *f,
					__be32 val, __be32 mask)
{
	memcpy(&f->filter.formatted.src_ip[0], &val, sizeof(u32));
	memcpy(&f->filter.formatted.src_ip_mask[0], &mask, sizeof(u32));

	f->filter.formatted.flow_type = RNPM_ATR_FLOW_TYPE_IPV4;
	f->filter.layer2_format.proto = htons(ETH_P_IP);
	return 0;
}

static inline int rnpm_fill_ipv4_dst_ip(struct rnpm_fdir_filter *f,
					__be32 val, __be32 mask)
{
	memcpy(&f->filter.formatted.dst_ip[0], &val, sizeof(u32));
	memcpy(&f->filter.formatted.dst_ip_mask[0], &mask, sizeof(u32));

	f->filter.formatted.flow_type = RNPM_ATR_FLOW_TYPE_IPV4;
	f->filter.layer2_format.proto = htons(ETH_P_IP);
	return 0;
}

static const struct rnpm_match_parser rnpm_ipv4_parser[] = {
	{ .off = 12, .val = rnpm_fill_ipv4_src_ip },
	{ .off = 16, .val = rnpm_fill_ipv4_dst_ip },
	{ .val = NULL }
};

#endif

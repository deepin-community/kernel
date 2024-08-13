// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [TOOL]" fmt

#include "sss_kernel.h"
#include "sss_tool_comm.h"
#include "sss_tool_nic.h"
#include "sss_nic_netdev_ops_api.h"
#include "sss_nic_ethtool_stats_api.h"

enum sss_tool_show_set {
	SSS_TOOL_SHOW_SSET_IO_STATS = 1,
};

static void sss_tool_reset_nicdev_stats(struct sss_nic_dev *nic_dev)
{
	u64_stats_update_begin(&nic_dev->tx_stats.stats_sync);
	nic_dev->tx_stats.rsvd1 = 0;
	nic_dev->tx_stats.rsvd2 = 0;
	nic_dev->tx_stats.tx_drop = 0;
	nic_dev->tx_stats.tx_timeout = 0;
	nic_dev->tx_stats.tx_invalid_qid = 0;
	u64_stats_update_end(&nic_dev->tx_stats.stats_sync);
}

static void sss_tool_reset_rq_stats(struct sss_nic_rq_stats *rq_stats)
{
	u64_stats_update_begin(&rq_stats->stats_sync);
	rq_stats->reset_drop_sge = 0;
	rq_stats->rx_packets = 0;
	rq_stats->alloc_rx_dma_err = 0;
	rq_stats->rx_bytes = 0;

	rq_stats->csum_errors = 0;
	rq_stats->rx_dropped = 0;
	rq_stats->errors = 0;
	rq_stats->large_xdp_pkts = 0;
	rq_stats->rx_buf_errors = 0;
	rq_stats->alloc_skb_err = 0;
	rq_stats->xdp_dropped = 0;
	rq_stats->other_errors = 0;
	rq_stats->rsvd2 = 0;
	u64_stats_update_end(&rq_stats->stats_sync);
}

static void sss_tool_reset_sq_stats(struct sss_nic_sq_stats *sq_stats)
{
	u64_stats_update_begin(&sq_stats->stats_sync);
	sq_stats->unknown_tunnel_proto = 0;
	sq_stats->tx_packets = 0;
	sq_stats->tx_dropped = 0;
	sq_stats->frag_len_overflow = 0;
	sq_stats->tx_busy = 0;
	sq_stats->wake = 0;
	sq_stats->skb_pad_err = 0;
	sq_stats->dma_map_err = 0;
	sq_stats->frag_size_zero = 0;
	sq_stats->tx_bytes = 0;
	sq_stats->offload_err = 0;
	sq_stats->rsvd1 = 0;
	sq_stats->rsvd2 = 0;
	u64_stats_update_end(&sq_stats->stats_sync);
}

int sss_tool_clear_func_stats(struct sss_nic_dev *nic_dev, const void *in_buf,
			      u32 in_len, void *out_buf, u32 *out_len)
{
	int i;

	if (!out_len) {
		tool_err("Invalid out len is null\n");
		return -EINVAL;
	}

#ifndef HAVE_NETDEV_STATS_IN_NETDEV
	memset(&nic_dev->net_stats, 0, sizeof(nic_dev->net_stats));
#endif
	sss_tool_reset_nicdev_stats(nic_dev);
	for (i = 0; i < nic_dev->max_qp_num; i++) {
		sss_tool_reset_rq_stats(&nic_dev->rq_desc_group[i].stats);
		sss_tool_reset_sq_stats(&nic_dev->sq_desc_group[i].stats);
	}

	*out_len = 0;

	return 0;
}

int sss_tool_get_sset_count(struct sss_nic_dev *nic_dev, const void *in_buf,
			    u32 in_len, void *out_buf, u32 *out_len)
{
	u32 count = 0;

	if (!in_buf || in_len != sizeof(count) || !out_len ||
	    *out_len != sizeof(count) || !out_buf) {
		tool_err("Invalid in_len: %u\n", in_len);
		return -EINVAL;
	}

	if (*((u32 *)in_buf) == SSS_TOOL_SHOW_SSET_IO_STATS)
		count = sss_nic_get_io_stats_size(nic_dev);

	*((u32 *)out_buf) = count;

	return 0;
}

int sss_tool_get_sset_stats(struct sss_nic_dev *nic_dev, const void *in_buf,
			    u32 in_len, void *out_buf, u32 *out_len)
{
	struct sss_tool_show_item *items = out_buf;
	u32 count;

	if (!in_buf || in_len != sizeof(count) || !out_len || !out_buf) {
		tool_err("Invalid in_len: %u\n", in_len);
		return -EINVAL;
	}

	if (*((u32 *)in_buf) != SSS_TOOL_SHOW_SSET_IO_STATS) {
		tool_err("Invalid input para %u stats\n", *((u32 *)in_buf));
		return -EINVAL;
	}

	count = sss_nic_get_io_stats_size(nic_dev);

	if (count * sizeof(*items) != *out_len) {
		tool_err("Invalid out len: %u is not equal to %lu\n",
			 *out_len, count * sizeof(*items));
		return -EINVAL;
	}

	sss_nic_get_io_stats(nic_dev, items);

	return 0;
}

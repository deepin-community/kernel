// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/if_vlan.h>
#include <linux/ethtool.h>

#include "ossl_knl.h"
#include "hinic3_hw.h"
#include "hinic3_crm.h"
#include "hinic3_mt.h"
#include "hinic3_nic_cfg.h"
#include "hinic3_nic_dev.h"
#include "hinic3_tx.h"
#include "hinic3_rx.h"

#define HINIC_SET_LINK_STR_LEN          128
#define HINIC_ETHTOOL_FEC_INFO_LEN      6
#define HINIC_SUPPORTED_FEC_CMD         0
#define HINIC_ADVERTISED_FEC_CMD        1

struct hinic3_ethtool_fec {
	u8 hinic_fec_offset;
	u8 ethtool_bit_offset;
};

static struct hinic3_ethtool_fec
	hinic3_ethtool_fec_info[HINIC_ETHTOOL_FEC_INFO_LEN] = {
	{.hinic_fec_offset = PORT_FEC_NOT_SET,  .ethtool_bit_offset = 0xFF},  
	{.hinic_fec_offset = PORT_FEC_RSFEC,    .ethtool_bit_offset = 0x32},
	{.hinic_fec_offset = PORT_FEC_BASEFEC,  .ethtool_bit_offset = 0x33},
	{.hinic_fec_offset = PORT_FEC_NOFEC,    .ethtool_bit_offset = 0x31},
	{.hinic_fec_offset = PORT_FEC_LLRSFEC,  .ethtool_bit_offset = 0x4A},
	{.hinic_fec_offset = PORT_FEC_AUTO,     .ethtool_bit_offset = 0XFF}
};

struct hinic3_stats {
	char name[ETH_GSTRING_LEN];
	u32 size;
	int offset;
};

struct hinic3_netdev_link_count_str {
	u64 link_down_events_phy;
};

#define HINIC3_NETDEV_LINK_COUNT(_stat_item) { \
	.name = #_stat_item, \
	.size = FIELD_SIZEOF(struct hinic3_netdev_link_count_str, _stat_item), \
	.offset = offsetof(struct hinic3_netdev_link_count_str, _stat_item) \
}

static struct hinic3_stats hinic3_netdev_link_count[] = {
	HINIC3_NETDEV_LINK_COUNT(link_down_events_phy),
};

#define HINIC3_NETDEV_STAT(_stat_item) { \
	.name = #_stat_item, \
	.size = FIELD_SIZEOF(struct rtnl_link_stats64, _stat_item), \
	.offset = offsetof(struct rtnl_link_stats64, _stat_item) \
}

static struct hinic3_stats hinic3_netdev_stats[] = {
	HINIC3_NETDEV_STAT(rx_packets),
	HINIC3_NETDEV_STAT(tx_packets),
	HINIC3_NETDEV_STAT(rx_bytes),
	HINIC3_NETDEV_STAT(tx_bytes),
	HINIC3_NETDEV_STAT(rx_errors),
	HINIC3_NETDEV_STAT(tx_errors),
	HINIC3_NETDEV_STAT(rx_dropped),
	HINIC3_NETDEV_STAT(tx_dropped),
	HINIC3_NETDEV_STAT(multicast),
	HINIC3_NETDEV_STAT(collisions),
	HINIC3_NETDEV_STAT(rx_length_errors),
	HINIC3_NETDEV_STAT(rx_over_errors),
	HINIC3_NETDEV_STAT(rx_crc_errors),
	HINIC3_NETDEV_STAT(rx_frame_errors),
	HINIC3_NETDEV_STAT(rx_fifo_errors),
	HINIC3_NETDEV_STAT(rx_missed_errors),
	HINIC3_NETDEV_STAT(tx_aborted_errors),
	HINIC3_NETDEV_STAT(tx_carrier_errors),
	HINIC3_NETDEV_STAT(tx_fifo_errors),
	HINIC3_NETDEV_STAT(tx_heartbeat_errors),
};

#define HINIC3_NIC_STAT(_stat_item) { \
	.name = #_stat_item, \
	.size = FIELD_SIZEOF(struct hinic3_nic_stats, _stat_item), \
	.offset = offsetof(struct hinic3_nic_stats, _stat_item) \
}

static struct hinic3_stats hinic3_nic_dev_stats[] = {
	HINIC3_NIC_STAT(netdev_tx_timeout),
};

static struct hinic3_stats hinic3_nic_dev_stats_extern[] = {
	HINIC3_NIC_STAT(tx_carrier_off_drop),
	HINIC3_NIC_STAT(tx_invalid_qid),
	HINIC3_NIC_STAT(rsvd1),
	HINIC3_NIC_STAT(rsvd2),
};

#define HINIC3_RXQ_STAT(_stat_item) { \
	.name = "rxq%d_"#_stat_item, \
	.size = FIELD_SIZEOF(struct hinic3_rxq_stats, _stat_item), \
	.offset = offsetof(struct hinic3_rxq_stats, _stat_item) \
}

#define HINIC3_TXQ_STAT(_stat_item) { \
	.name = "txq%d_"#_stat_item, \
	.size = FIELD_SIZEOF(struct hinic3_txq_stats, _stat_item), \
	.offset = offsetof(struct hinic3_txq_stats, _stat_item) \
}

static struct hinic3_stats hinic3_rx_queue_stats[] = {
	HINIC3_RXQ_STAT(packets),
	HINIC3_RXQ_STAT(bytes),
	HINIC3_RXQ_STAT(errors),
	HINIC3_RXQ_STAT(csum_errors),
	HINIC3_RXQ_STAT(other_errors),
	HINIC3_RXQ_STAT(dropped),
#ifdef HAVE_XDP_SUPPORT
	HINIC3_RXQ_STAT(xdp_dropped),
#endif
	HINIC3_RXQ_STAT(rx_buf_empty),
};

static struct hinic3_stats hinic3_rx_queue_stats_extern[] = {
	HINIC3_RXQ_STAT(alloc_skb_err),
	HINIC3_RXQ_STAT(alloc_rx_buf_err),
	HINIC3_RXQ_STAT(xdp_large_pkt),
	HINIC3_RXQ_STAT(restore_drop_sge),
	HINIC3_RXQ_STAT(rsvd2),
};

static struct hinic3_stats hinic3_tx_queue_stats[] = {
	HINIC3_TXQ_STAT(packets),
	HINIC3_TXQ_STAT(bytes),
	HINIC3_TXQ_STAT(busy),
	HINIC3_TXQ_STAT(wake),
	HINIC3_TXQ_STAT(dropped),
};

static struct hinic3_stats hinic3_tx_queue_stats_extern[] = {
	HINIC3_TXQ_STAT(skb_pad_err),
	HINIC3_TXQ_STAT(frag_len_overflow),
	HINIC3_TXQ_STAT(offload_cow_skb_err),
	HINIC3_TXQ_STAT(map_frag_err),
	HINIC3_TXQ_STAT(unknown_tunnel_pkt),
	HINIC3_TXQ_STAT(frag_size_err),
	HINIC3_TXQ_STAT(rsvd1),
	HINIC3_TXQ_STAT(rsvd2),
};

#define HINIC3_FUNC_STAT(_stat_item) {	\
	.name = #_stat_item, \
	.size = FIELD_SIZEOF(struct hinic3_vport_stats, _stat_item), \
	.offset = offsetof(struct hinic3_vport_stats, _stat_item) \
}

static struct hinic3_stats hinic3_function_stats[] = {
	HINIC3_FUNC_STAT(tx_unicast_pkts_vport),
	HINIC3_FUNC_STAT(tx_unicast_bytes_vport),
	HINIC3_FUNC_STAT(tx_multicast_pkts_vport),
	HINIC3_FUNC_STAT(tx_multicast_bytes_vport),
	HINIC3_FUNC_STAT(tx_broadcast_pkts_vport),
	HINIC3_FUNC_STAT(tx_broadcast_bytes_vport),

	HINIC3_FUNC_STAT(rx_unicast_pkts_vport),
	HINIC3_FUNC_STAT(rx_unicast_bytes_vport),
	HINIC3_FUNC_STAT(rx_multicast_pkts_vport),
	HINIC3_FUNC_STAT(rx_multicast_bytes_vport),
	HINIC3_FUNC_STAT(rx_broadcast_pkts_vport),
	HINIC3_FUNC_STAT(rx_broadcast_bytes_vport),

	HINIC3_FUNC_STAT(tx_discard_vport),
	HINIC3_FUNC_STAT(rx_discard_vport),
	HINIC3_FUNC_STAT(tx_err_vport),
	HINIC3_FUNC_STAT(rx_err_vport),
};

#define HINIC3_PORT_STAT(_stat_item) { \
	.name = #_stat_item, \
	.size = FIELD_SIZEOF(struct mag_cmd_port_stats, _stat_item), \
	.offset = offsetof(struct mag_cmd_port_stats, _stat_item) \
}

static struct hinic3_stats hinic3_port_stats[] = {
	HINIC3_PORT_STAT(mac_tx_fragment_pkt_num),
	HINIC3_PORT_STAT(mac_tx_undersize_pkt_num),
	HINIC3_PORT_STAT(mac_tx_undermin_pkt_num),
	HINIC3_PORT_STAT(mac_tx_64_oct_pkt_num),
	HINIC3_PORT_STAT(mac_tx_65_127_oct_pkt_num),
	HINIC3_PORT_STAT(mac_tx_128_255_oct_pkt_num),
	HINIC3_PORT_STAT(mac_tx_256_511_oct_pkt_num),
	HINIC3_PORT_STAT(mac_tx_512_1023_oct_pkt_num),
	HINIC3_PORT_STAT(mac_tx_1024_1518_oct_pkt_num),
	HINIC3_PORT_STAT(mac_tx_1519_2047_oct_pkt_num),
	HINIC3_PORT_STAT(mac_tx_2048_4095_oct_pkt_num),
	HINIC3_PORT_STAT(mac_tx_4096_8191_oct_pkt_num),
	HINIC3_PORT_STAT(mac_tx_8192_9216_oct_pkt_num),
	HINIC3_PORT_STAT(mac_tx_9217_12287_oct_pkt_num),
	HINIC3_PORT_STAT(mac_tx_12288_16383_oct_pkt_num),
	HINIC3_PORT_STAT(mac_tx_1519_max_bad_pkt_num),
	HINIC3_PORT_STAT(mac_tx_1519_max_good_pkt_num),
	HINIC3_PORT_STAT(mac_tx_oversize_pkt_num),
	HINIC3_PORT_STAT(mac_tx_jabber_pkt_num),
	HINIC3_PORT_STAT(mac_tx_bad_pkt_num),
	HINIC3_PORT_STAT(mac_tx_bad_oct_num),
	HINIC3_PORT_STAT(mac_tx_good_pkt_num),
	HINIC3_PORT_STAT(mac_tx_good_oct_num),
	HINIC3_PORT_STAT(mac_tx_total_pkt_num),
	HINIC3_PORT_STAT(mac_tx_total_oct_num),
	HINIC3_PORT_STAT(mac_tx_uni_pkt_num),
	HINIC3_PORT_STAT(mac_tx_multi_pkt_num),
	HINIC3_PORT_STAT(mac_tx_broad_pkt_num),
	HINIC3_PORT_STAT(mac_tx_pause_num),
	HINIC3_PORT_STAT(mac_tx_pfc_pkt_num),
	HINIC3_PORT_STAT(mac_tx_pfc_pri0_pkt_num),
	HINIC3_PORT_STAT(mac_tx_pfc_pri1_pkt_num),
	HINIC3_PORT_STAT(mac_tx_pfc_pri2_pkt_num),
	HINIC3_PORT_STAT(mac_tx_pfc_pri3_pkt_num),
	HINIC3_PORT_STAT(mac_tx_pfc_pri4_pkt_num),
	HINIC3_PORT_STAT(mac_tx_pfc_pri5_pkt_num),
	HINIC3_PORT_STAT(mac_tx_pfc_pri6_pkt_num),
	HINIC3_PORT_STAT(mac_tx_pfc_pri7_pkt_num),
	HINIC3_PORT_STAT(mac_tx_control_pkt_num),
	HINIC3_PORT_STAT(mac_tx_err_all_pkt_num),
	HINIC3_PORT_STAT(mac_tx_from_app_good_pkt_num),
	HINIC3_PORT_STAT(mac_tx_from_app_bad_pkt_num),

	HINIC3_PORT_STAT(mac_rx_fragment_pkt_num),
	HINIC3_PORT_STAT(mac_rx_undersize_pkt_num),
	HINIC3_PORT_STAT(mac_rx_undermin_pkt_num),
	HINIC3_PORT_STAT(mac_rx_64_oct_pkt_num),
	HINIC3_PORT_STAT(mac_rx_65_127_oct_pkt_num),
	HINIC3_PORT_STAT(mac_rx_128_255_oct_pkt_num),
	HINIC3_PORT_STAT(mac_rx_256_511_oct_pkt_num),
	HINIC3_PORT_STAT(mac_rx_512_1023_oct_pkt_num),
	HINIC3_PORT_STAT(mac_rx_1024_1518_oct_pkt_num),
	HINIC3_PORT_STAT(mac_rx_1519_2047_oct_pkt_num),
	HINIC3_PORT_STAT(mac_rx_2048_4095_oct_pkt_num),
	HINIC3_PORT_STAT(mac_rx_4096_8191_oct_pkt_num),
	HINIC3_PORT_STAT(mac_rx_8192_9216_oct_pkt_num),
	HINIC3_PORT_STAT(mac_rx_9217_12287_oct_pkt_num),
	HINIC3_PORT_STAT(mac_rx_12288_16383_oct_pkt_num),
	HINIC3_PORT_STAT(mac_rx_1519_max_bad_pkt_num),
	HINIC3_PORT_STAT(mac_rx_1519_max_good_pkt_num),
	HINIC3_PORT_STAT(mac_rx_oversize_pkt_num),
	HINIC3_PORT_STAT(mac_rx_jabber_pkt_num),
	HINIC3_PORT_STAT(mac_rx_bad_pkt_num),
	HINIC3_PORT_STAT(mac_rx_bad_oct_num),
	HINIC3_PORT_STAT(mac_rx_good_pkt_num),
	HINIC3_PORT_STAT(mac_rx_good_oct_num),
	HINIC3_PORT_STAT(mac_rx_total_pkt_num),
	HINIC3_PORT_STAT(mac_rx_total_oct_num),
	HINIC3_PORT_STAT(mac_rx_uni_pkt_num),
	HINIC3_PORT_STAT(mac_rx_multi_pkt_num),
	HINIC3_PORT_STAT(mac_rx_broad_pkt_num),
	HINIC3_PORT_STAT(mac_rx_pause_num),
	HINIC3_PORT_STAT(mac_rx_pfc_pkt_num),
	HINIC3_PORT_STAT(mac_rx_pfc_pri0_pkt_num),
	HINIC3_PORT_STAT(mac_rx_pfc_pri1_pkt_num),
	HINIC3_PORT_STAT(mac_rx_pfc_pri2_pkt_num),
	HINIC3_PORT_STAT(mac_rx_pfc_pri3_pkt_num),
	HINIC3_PORT_STAT(mac_rx_pfc_pri4_pkt_num),
	HINIC3_PORT_STAT(mac_rx_pfc_pri5_pkt_num),
	HINIC3_PORT_STAT(mac_rx_pfc_pri6_pkt_num),
	HINIC3_PORT_STAT(mac_rx_pfc_pri7_pkt_num),
	HINIC3_PORT_STAT(mac_rx_control_pkt_num),
	HINIC3_PORT_STAT(mac_rx_sym_err_pkt_num),
	HINIC3_PORT_STAT(mac_rx_fcs_err_pkt_num),
	HINIC3_PORT_STAT(mac_rx_send_app_good_pkt_num),
	HINIC3_PORT_STAT(mac_rx_send_app_bad_pkt_num),
	HINIC3_PORT_STAT(mac_rx_unfilter_pkt_num),
};

#define HINIC3_RSFEC_STAT(_stat_item) { \
	.name = #_stat_item, \
	.size = FIELD_SIZEOF(struct mag_cmd_rsfec_stats, _stat_item), \
	.offset = offsetof(struct mag_cmd_rsfec_stats, _stat_item) \
}

static struct hinic3_stats g_hinic3_rsfec_stats[] = {
	HINIC3_RSFEC_STAT(rx_err_lane_phy),
};

#define HINIC3_FGPA_PORT_STAT(_stat_item) { \
	.name = #_stat_item, \
	.size = FIELD_SIZEOF(struct hinic3_phy_fpga_port_stats, _stat_item), \
	.offset = offsetof(struct hinic3_phy_fpga_port_stats, _stat_item) \
}

static char g_hinic_priv_flags_strings[][ETH_GSTRING_LEN] = {
	"Symmetric-RSS",
	"Force-Link-up",
	"Rxq_Recovery",
};

u32 hinic3_get_io_stats_size(const struct hinic3_nic_dev *nic_dev)
{
	u32 count;

	count = ARRAY_LEN(hinic3_nic_dev_stats) +
		ARRAY_LEN(hinic3_nic_dev_stats_extern) +
		(ARRAY_LEN(hinic3_tx_queue_stats) +
		 ARRAY_LEN(hinic3_tx_queue_stats_extern) +
		 ARRAY_LEN(hinic3_rx_queue_stats) +
		 ARRAY_LEN(hinic3_rx_queue_stats_extern)) * nic_dev->max_qps;

	return count;
}

static u64 get_value_of_ptr(u32 size, const void *ptr)
{
	u64 ret = (size) == sizeof(u64) ? *(u64 *)(ptr) :
		  (size) == sizeof(u32) ? *(u32 *)(ptr) :
		  (size) == sizeof(u16) ? *(u16 *)(ptr) :
					  *(u8 *)(ptr);
	return ret;
}

static int dev_stats_pack(struct hinic3_show_item *items, int len,
			  struct hinic3_stats *array, const void *stats_ptr)
{
	int j;
	int item_idx = 0;

	for (j = 0; j < len; j++) {
		memcpy(items[item_idx].name, array[j].name,
		       HINIC3_SHOW_ITEM_LEN);
		items[item_idx].hexadecimal = 0;
		items[item_idx].value =
				get_value_of_ptr(array[j].size,
						 stats_ptr + array[j].offset);
		item_idx++;
	}

	return item_idx;
}

int hinic3_rx_queue_stat_pack(struct hinic3_show_item *item,
			      struct hinic3_stats *stat,
			      struct hinic3_rxq_stats *rxq_stats, u16 qid)
{
	snprintf(item->name, HINIC3_SHOW_ITEM_LEN, stat->name, qid);

	item->hexadecimal = 0;
	item->value = get_value_of_ptr(stat->size, (char *)(rxq_stats) +
				       stat->offset);

	return 0;
}

int hinic3_tx_queue_stat_pack(struct hinic3_show_item *item,
			      struct hinic3_stats *stat,
			      struct hinic3_txq_stats *txq_stats, u16 qid)
{
	snprintf(item->name, HINIC3_SHOW_ITEM_LEN, stat->name, qid);

	item->hexadecimal = 0;
	item->value = get_value_of_ptr(stat->size, (char *)(txq_stats) +
				       stat->offset);

	return 0;
}

int hinic3_get_io_stats(const struct hinic3_nic_dev *nic_dev, void *stats)
{
	struct hinic3_show_item *items = stats;
	int item_idx = 0;
	u16 qid;
	int idx;
	int ret;

	dev_stats_pack(items, item_idx, hinic3_nic_dev_stats, &nic_dev->stats);
	dev_stats_pack(items, item_idx, hinic3_nic_dev_stats_extern, &nic_dev->stats);

	for (qid = 0; qid < nic_dev->max_qps; qid++) {
		for (idx = 0; idx < ARRAY_LEN(hinic3_tx_queue_stats); idx++) {
			ret = hinic3_tx_queue_stat_pack(&items[item_idx++],
							&hinic3_tx_queue_stats[idx],
							&nic_dev->txqs[qid].txq_stats, qid);
			if (ret != 0)
				return -EINVAL;
		}
	}

	for (idx = 0; idx < ARRAY_LEN(hinic3_tx_queue_stats_extern); idx++) {
		ret = hinic3_tx_queue_stat_pack(&items[item_idx++],
						&hinic3_tx_queue_stats_extern[idx],
						&nic_dev->txqs[qid].txq_stats, qid);
		if (ret != 0)
			return -EINVAL;
	}

	for (qid = 0; qid < nic_dev->max_qps; qid++) {
		for (idx = 0; idx < ARRAY_LEN(hinic3_rx_queue_stats); idx++) {
			ret = hinic3_rx_queue_stat_pack(&items[item_idx++],
							&hinic3_rx_queue_stats[idx],
							&nic_dev->rxqs[qid].rxq_stats, qid);
			if (ret != 0)
				return -EINVAL;
		}

		for (idx = 0; idx < ARRAY_LEN(hinic3_rx_queue_stats_extern); idx++) {
			ret = hinic3_rx_queue_stat_pack(&items[item_idx++],
							&hinic3_rx_queue_stats_extern[idx],
							&nic_dev->rxqs[qid].rxq_stats, qid);
			if (ret != 0)
				return -EINVAL;
		}
	}

	return 0;
}

static char g_hinic3_test_strings[][ETH_GSTRING_LEN] = {
	"Internal lb test  (on/offline)",
	"External lb test (external_lb)",
};

int hinic3_get_sset_count(struct net_device *netdev, int sset)
{
	int count = 0, q_num = 0;
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);

	switch (sset) {
	case ETH_SS_TEST:
		return ARRAY_LEN(g_hinic3_test_strings);
	case ETH_SS_STATS:
		q_num = nic_dev->q_params.num_qps;
		count = ARRAY_LEN(hinic3_netdev_stats) +
			ARRAY_LEN(hinic3_nic_dev_stats) +
			ARRAY_LEN(hinic3_netdev_link_count) +
			ARRAY_LEN(hinic3_function_stats) +
			(ARRAY_LEN(hinic3_tx_queue_stats) +
			 ARRAY_LEN(hinic3_rx_queue_stats)) * q_num;

		if (!HINIC3_FUNC_IS_VF(nic_dev->hwdev)) {
			count += ARRAY_LEN(hinic3_port_stats);
			count += ARRAY_LEN(g_hinic3_rsfec_stats);
		}

		return count;
	case ETH_SS_PRIV_FLAGS:
		return ARRAY_LEN(g_hinic_priv_flags_strings);
	default:
		return -EOPNOTSUPP;
	}
}

static void get_drv_queue_stats(struct hinic3_nic_dev *nic_dev, u64 *data)
{
	struct hinic3_txq_stats txq_stats;
	struct hinic3_rxq_stats rxq_stats;
	u16 i = 0, j = 0, qid = 0;
	char *p = NULL;

	for (qid = 0; qid < nic_dev->q_params.num_qps; qid++) {
		if (!nic_dev->txqs)
			break;

		hinic3_txq_get_stats(&nic_dev->txqs[qid], &txq_stats);
		for (j = 0; j < ARRAY_LEN(hinic3_tx_queue_stats); j++, i++) {
			p = (char *)(&txq_stats) +
			    hinic3_tx_queue_stats[j].offset;
			data[i] =
				(hinic3_tx_queue_stats[j].size == sizeof(u64)) ?
					*(u64 *)p :
					*(u32 *)p;
		}
	}

	for (qid = 0; qid < nic_dev->q_params.num_qps; qid++) {
		if (!nic_dev->rxqs)
			break;

		hinic3_rxq_get_stats(&nic_dev->rxqs[qid], &rxq_stats);
		for (j = 0; j < ARRAY_LEN(hinic3_rx_queue_stats); j++, i++) {
			p = (char *)(&rxq_stats) +
			    hinic3_rx_queue_stats[j].offset;
			data[i] =
				(hinic3_rx_queue_stats[j].size == sizeof(u64)) ?
					*(u64 *)p :
					*(u32 *)p;
		}
	}
}

static u16 get_ethtool_port_stats(struct hinic3_nic_dev *nic_dev, u64 *data)
{
	struct mag_cmd_port_stats *port_stats = NULL;
	char *p = NULL;
	u16 i = 0, j = 0;
	int err;

	port_stats = kzalloc(sizeof(*port_stats), GFP_KERNEL);
	if (!port_stats) {
		memset(&data[i], 0,
		       ARRAY_LEN(hinic3_port_stats) * sizeof(*data));
		i += ARRAY_LEN(hinic3_port_stats);
		return i;
	}

	err = hinic3_get_phy_port_stats(nic_dev->hwdev, port_stats);
	if (err)
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Failed to get port stats from fw\n");

	for (j = 0; j < ARRAY_LEN(hinic3_port_stats); j++, i++) {
		p = (char *)(port_stats) + hinic3_port_stats[j].offset;
		data[i] = (hinic3_port_stats[j].size == sizeof(u64)) ?
				  *(u64 *)p :
				  *(u32 *)p;
	}

	kfree(port_stats);

	return i;
}

static u16 get_ethtool_rsfec_stats(struct hinic3_nic_dev *nic_dev, u64 *data)
{
	struct mag_cmd_rsfec_stats *port_stats = NULL;
	char *p = NULL;
	u16 i = 0, j = 0;
	int err;

	port_stats = kzalloc(sizeof(*port_stats), GFP_KERNEL);
	if (!port_stats) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Failed to malloc port stats\n");
	memset(&data[i], 0,
	       ARRAY_LEN(g_hinic3_rsfec_stats) * sizeof(*data));
	i += ARRAY_LEN(g_hinic3_rsfec_stats);
	return i;
	}

	err = hinic3_get_phy_rsfec_stats(nic_dev->hwdev, port_stats);
	if (err)
		nicif_err(nic_dev, drv, nic_dev->netdev,
		"Failed to get rsfec stats from fw\n");

	for (j = 0; j < ARRAY_LEN(g_hinic3_rsfec_stats); j++, i++) {
		p = (char *)(port_stats) + g_hinic3_rsfec_stats[j].offset;
		data[i] = (g_hinic3_rsfec_stats[j].size ==
		sizeof(u64)) ? *(u64 *)p : *(u32 *)p;
	}

	kfree(port_stats);

	return i;
}

void hinic3_get_ethtool_stats(struct net_device *netdev,
			      struct ethtool_stats *stats, u64 *data)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
#ifdef HAVE_NDO_GET_STATS64
	struct rtnl_link_stats64 temp;
	const struct rtnl_link_stats64 *net_stats = NULL;
#else
	const struct net_device_stats *net_stats = NULL;
#endif
	struct hinic3_nic_stats *nic_stats = NULL;

	struct hinic3_vport_stats vport_stats = { 0 };
	u16 i = 0, j = 0;
	char *p = NULL;
	int err;
	int link_down_events_phy_tmp = 0;
	struct hinic3_netdev_link_count_str link_count = {0};

#ifdef HAVE_NDO_GET_STATS64
	net_stats = dev_get_stats(netdev, &temp);
#else
	net_stats = dev_get_stats(netdev);
#endif
	for (j = 0; j < ARRAY_LEN(hinic3_netdev_stats); j++, i++) {
		p = (char *)(net_stats) + hinic3_netdev_stats[j].offset;
		data[i] = get_value_of_ptr(hinic3_netdev_stats[j].size, p);
	}

	nic_stats = &nic_dev->stats;
	for (j = 0; j < ARRAY_LEN(hinic3_nic_dev_stats); j++, i++) {
		p = (char *)(nic_stats) + hinic3_nic_dev_stats[j].offset;
		data[i] = get_value_of_ptr(hinic3_nic_dev_stats[j].size, p);
	}

	err = hinic3_get_link_event_stats(nic_dev->hwdev,
					  &link_down_events_phy_tmp);

	link_count.link_down_events_phy = (u64)link_down_events_phy_tmp;
	for (j = 0; j < ARRAY_LEN(hinic3_netdev_link_count); j++, i++) {
		p = (char *)(&link_count) + hinic3_netdev_link_count[j].offset;
		data[i] = get_value_of_ptr(hinic3_netdev_link_count[j].size, p);
	}

	err = hinic3_get_vport_stats(nic_dev->hwdev,
				     hinic3_global_func_id(nic_dev->hwdev),
				     &vport_stats);
	if (err)
		nicif_err(nic_dev, drv, netdev,
			  "Failed to get function stats from fw\n");

	for (j = 0; j < ARRAY_LEN(hinic3_function_stats); j++, i++) {
		p = (char *)(&vport_stats) + hinic3_function_stats[j].offset;
		data[i] = get_value_of_ptr(hinic3_function_stats[j].size, p);
	}

	if (!HINIC3_FUNC_IS_VF(nic_dev->hwdev)) {
		i += get_ethtool_port_stats(nic_dev, data + i);
		i += get_ethtool_rsfec_stats(nic_dev, data + i);
	}

	get_drv_queue_stats(nic_dev, data + i);
}

static u16 get_drv_dev_strings(struct hinic3_nic_dev *nic_dev, char *p)
{
	u16 i, cnt = 0;

	for (i = 0; i < ARRAY_LEN(hinic3_netdev_stats); i++) {
		memcpy(p, hinic3_netdev_stats[i].name, ETH_GSTRING_LEN);
		p += ETH_GSTRING_LEN;
		cnt++;
	}

	for (i = 0; i < ARRAY_LEN(hinic3_nic_dev_stats); i++) {
		memcpy(p, hinic3_nic_dev_stats[i].name, ETH_GSTRING_LEN);
		p += ETH_GSTRING_LEN;
		cnt++;
	}

	for (i = 0; i < ARRAY_LEN(hinic3_netdev_link_count); i++) {
		memcpy(p, hinic3_netdev_link_count[i].name, ETH_GSTRING_LEN);
		p += ETH_GSTRING_LEN;
		cnt++;
	}

	return cnt;
}

static u16 get_hw_stats_strings(struct hinic3_nic_dev *nic_dev, char *p)
{
	u16 i, cnt = 0;

	for (i = 0; i < ARRAY_LEN(hinic3_function_stats); i++) {
		memcpy(p, hinic3_function_stats[i].name, ETH_GSTRING_LEN);
		p += ETH_GSTRING_LEN;
		cnt++;
	}

	if (!HINIC3_FUNC_IS_VF(nic_dev->hwdev)) {
		for (i = 0; i < ARRAY_LEN(hinic3_port_stats); i++) {
			memcpy(p, hinic3_port_stats[i].name, ETH_GSTRING_LEN);
			p += ETH_GSTRING_LEN;
			cnt++;
		}
		for (i = 0; i < ARRAY_LEN(g_hinic3_rsfec_stats); i++) {
			memcpy(p, g_hinic3_rsfec_stats[i].name,
			       ETH_GSTRING_LEN);
			p += ETH_GSTRING_LEN;
			cnt++;
		}
	}

	return cnt;
}

static u16 get_qp_stats_strings(const struct hinic3_nic_dev *nic_dev, char *p)
{
	u16 i = 0, j = 0, cnt = 0;
	int err;

	for (i = 0; i < nic_dev->q_params.num_qps; i++) {
		for (j = 0; j < ARRAY_LEN(hinic3_tx_queue_stats); j++) {
			err = sprintf(p, hinic3_tx_queue_stats[j].name, i);
			if (err < 0)
				nicif_err(nic_dev, drv, nic_dev->netdev,
					  "Failed to sprintf tx queue stats name, idx_qps: %u, idx_stats: %u\n",
					  i, j);
			p += ETH_GSTRING_LEN;
			cnt++;
		}
	}

	for (i = 0; i < nic_dev->q_params.num_qps; i++) {
		for (j = 0; j < ARRAY_LEN(hinic3_rx_queue_stats); j++) {
			err = sprintf(p, hinic3_rx_queue_stats[j].name, i);
			if (err < 0)
				nicif_err(nic_dev, drv, nic_dev->netdev,
					  "Failed to sprintf rx queue stats name, idx_qps: %u, idx_stats: %u\n",
					  i, j);
			p += ETH_GSTRING_LEN;
			cnt++;
		}
	}

	return cnt;
}

void hinic3_get_strings(struct net_device *netdev, u32 stringset, u8 *data)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	char *p = (char *)data;
	u16 offset = 0;

	switch (stringset) {
	case ETH_SS_TEST:
		memcpy(data, *g_hinic3_test_strings,
		       sizeof(g_hinic3_test_strings));
		return;
	case ETH_SS_STATS:
		offset = get_drv_dev_strings(nic_dev, p);
		offset += get_hw_stats_strings(nic_dev,
					       p + offset * ETH_GSTRING_LEN);
		get_qp_stats_strings(nic_dev, p + offset * ETH_GSTRING_LEN);

		return;
	case ETH_SS_PRIV_FLAGS:
		memcpy(data, g_hinic_priv_flags_strings,
		       sizeof(g_hinic_priv_flags_strings));
		return;
	default:
		nicif_err(nic_dev, drv, netdev, "Invalid string set %u.",
			  stringset);
		return;
	}
}

static const u32 hinic3_mag_link_mode_ge[] = {
	ETHTOOL_LINK_MODE_1000baseT_Full_BIT,
	ETHTOOL_LINK_MODE_1000baseKX_Full_BIT,
	ETHTOOL_LINK_MODE_1000baseX_Full_BIT,
};

static const u32 hinic3_mag_link_mode_10ge_base_r[] = {
	ETHTOOL_LINK_MODE_10000baseKR_Full_BIT,
	ETHTOOL_LINK_MODE_10000baseR_FEC_BIT,
	ETHTOOL_LINK_MODE_10000baseCR_Full_BIT,
	ETHTOOL_LINK_MODE_10000baseSR_Full_BIT,
	ETHTOOL_LINK_MODE_10000baseLR_Full_BIT,
	ETHTOOL_LINK_MODE_10000baseLRM_Full_BIT,
};

static const u32 hinic3_mag_link_mode_25ge_base_r[] = {
	ETHTOOL_LINK_MODE_25000baseCR_Full_BIT,
	ETHTOOL_LINK_MODE_25000baseKR_Full_BIT,
	ETHTOOL_LINK_MODE_25000baseSR_Full_BIT,
};

static const u32 hinic3_mag_link_mode_40ge_base_r4[] = {
	ETHTOOL_LINK_MODE_40000baseKR4_Full_BIT,
	ETHTOOL_LINK_MODE_40000baseCR4_Full_BIT,
	ETHTOOL_LINK_MODE_40000baseSR4_Full_BIT,
	ETHTOOL_LINK_MODE_40000baseLR4_Full_BIT,
};

static const u32 hinic3_mag_link_mode_50ge_base_r[] = {
	ETHTOOL_LINK_MODE_50000baseKR_Full_BIT,
	ETHTOOL_LINK_MODE_50000baseSR_Full_BIT,
	ETHTOOL_LINK_MODE_50000baseCR_Full_BIT,
};

static const u32 hinic3_mag_link_mode_50ge_base_r2[] = {
	ETHTOOL_LINK_MODE_50000baseCR2_Full_BIT,
	ETHTOOL_LINK_MODE_50000baseKR2_Full_BIT,
	ETHTOOL_LINK_MODE_50000baseSR2_Full_BIT,
};

static const u32 hinic3_mag_link_mode_100ge_base_r[] = {
	ETHTOOL_LINK_MODE_100000baseKR_Full_BIT,
	ETHTOOL_LINK_MODE_100000baseSR_Full_BIT,
	ETHTOOL_LINK_MODE_100000baseCR_Full_BIT,
};

static const u32 hinic3_mag_link_mode_100ge_base_r2[] = {
	ETHTOOL_LINK_MODE_100000baseKR2_Full_BIT,
	ETHTOOL_LINK_MODE_100000baseSR2_Full_BIT,
	ETHTOOL_LINK_MODE_100000baseCR2_Full_BIT,
};

static const u32 hinic3_mag_link_mode_100ge_base_r4[] = {
	ETHTOOL_LINK_MODE_100000baseKR4_Full_BIT,
	ETHTOOL_LINK_MODE_100000baseSR4_Full_BIT,
	ETHTOOL_LINK_MODE_100000baseCR4_Full_BIT,
	ETHTOOL_LINK_MODE_100000baseLR4_ER4_Full_BIT,
};

static const u32 hinic3_mag_link_mode_200ge_base_r2[] = {
	ETHTOOL_LINK_MODE_200000baseKR2_Full_BIT,
	ETHTOOL_LINK_MODE_200000baseSR2_Full_BIT,
	ETHTOOL_LINK_MODE_200000baseCR2_Full_BIT,
};

static const u32 hinic3_mag_link_mode_200ge_base_r4[] = {
	ETHTOOL_LINK_MODE_200000baseKR4_Full_BIT,
	ETHTOOL_LINK_MODE_200000baseSR4_Full_BIT,
	ETHTOOL_LINK_MODE_200000baseCR4_Full_BIT,
};

struct hw2ethtool_link_mode {
	const u32 *link_mode_bit_arr;
	u32 arr_size;
	u32 speed;
};

static const struct hw2ethtool_link_mode
	hw2ethtool_link_mode_table[LINK_MODE_MAX_NUMBERS] = {
	[LINK_MODE_GE] = {
		.link_mode_bit_arr = hinic3_mag_link_mode_ge,
		.arr_size = ARRAY_LEN(hinic3_mag_link_mode_ge),
		.speed = SPEED_1000,
	},
	[LINK_MODE_10GE_BASE_R] = {
		.link_mode_bit_arr = hinic3_mag_link_mode_10ge_base_r,
		.arr_size = ARRAY_LEN(hinic3_mag_link_mode_10ge_base_r),
		.speed = SPEED_10000,
	},
	[LINK_MODE_25GE_BASE_R] = {
		.link_mode_bit_arr = hinic3_mag_link_mode_25ge_base_r,
		.arr_size = ARRAY_LEN(hinic3_mag_link_mode_25ge_base_r),
		.speed = SPEED_25000,
	},
	[LINK_MODE_40GE_BASE_R4] = {
		.link_mode_bit_arr = hinic3_mag_link_mode_40ge_base_r4,
		.arr_size = ARRAY_LEN(hinic3_mag_link_mode_40ge_base_r4),
		.speed = SPEED_40000,
	},
	[LINK_MODE_50GE_BASE_R] = {
		.link_mode_bit_arr = hinic3_mag_link_mode_50ge_base_r,
		.arr_size = ARRAY_LEN(hinic3_mag_link_mode_50ge_base_r),
		.speed = SPEED_50000,
	},
	[LINK_MODE_50GE_BASE_R2] = {
		.link_mode_bit_arr = hinic3_mag_link_mode_50ge_base_r2,
		.arr_size = ARRAY_LEN(hinic3_mag_link_mode_50ge_base_r2),
		.speed = SPEED_50000,
	},
	[LINK_MODE_100GE_BASE_R] = {
		.link_mode_bit_arr = hinic3_mag_link_mode_100ge_base_r,
		.arr_size = ARRAY_LEN(hinic3_mag_link_mode_100ge_base_r),
		.speed = SPEED_100000,
	},
	[LINK_MODE_100GE_BASE_R2] = {
		.link_mode_bit_arr = hinic3_mag_link_mode_100ge_base_r2,
		.arr_size = ARRAY_LEN(hinic3_mag_link_mode_100ge_base_r2),
		.speed = SPEED_100000,
	},
	[LINK_MODE_100GE_BASE_R4] = {
		.link_mode_bit_arr = hinic3_mag_link_mode_100ge_base_r4,
		.arr_size = ARRAY_LEN(hinic3_mag_link_mode_100ge_base_r4),
		.speed = SPEED_100000,
	},
	[LINK_MODE_200GE_BASE_R2] = {
		.link_mode_bit_arr = hinic3_mag_link_mode_200ge_base_r2,
		.arr_size = ARRAY_LEN(hinic3_mag_link_mode_200ge_base_r2),
		.speed = SPEED_200000,
	},
	[LINK_MODE_200GE_BASE_R4] = {
		.link_mode_bit_arr = hinic3_mag_link_mode_200ge_base_r4,
		.arr_size = ARRAY_LEN(hinic3_mag_link_mode_200ge_base_r4),
		.speed = SPEED_200000,
	},
};

#define GET_SUPPORTED_MODE 0
#define GET_ADVERTISED_MODE 1

struct cmd_link_settings {
	__ETHTOOL_DECLARE_LINK_MODE_MASK(supported);
	__ETHTOOL_DECLARE_LINK_MODE_MASK(advertising);

	u32 speed;
	u8 duplex;
	u8 port;
	u8 autoneg;
};

#define ETHTOOL_ADD_SUPPORTED_LINK_MODE(ecmd, mode) \
	set_bit(ETHTOOL_LINK_MODE_##mode##_BIT, (ecmd)->supported)
#define ETHTOOL_ADD_ADVERTISED_LINK_MODE(ecmd, mode) \
	set_bit(ETHTOOL_LINK_MODE_##mode##_BIT, (ecmd)->advertising)

static void ethtool_add_supported_speed_link_mode(struct cmd_link_settings *link_settings,
						  u32 mode)
{
	u32 i;

	for (i = 0; i < hw2ethtool_link_mode_table[mode].arr_size; i++) {
		if (hw2ethtool_link_mode_table[mode].link_mode_bit_arr[i] >=
			__ETHTOOL_LINK_MODE_MASK_NBITS)
			continue;
		set_bit(hw2ethtool_link_mode_table[mode].link_mode_bit_arr[i],
			link_settings->supported);
	}
}

static void ethtool_add_advertised_speed_link_mode(struct cmd_link_settings *link_settings,
						   u32 mode)
{
	u32 i;

	for (i = 0; i < hw2ethtool_link_mode_table[mode].arr_size; i++) {
		if (hw2ethtool_link_mode_table[mode].link_mode_bit_arr[i] >=
			__ETHTOOL_LINK_MODE_MASK_NBITS)
			continue;
		set_bit(hw2ethtool_link_mode_table[mode].link_mode_bit_arr[i],
			link_settings->advertising);
	}
}

/* Related to enum mag_cmd_port_speed */
static u32 hw_to_ethtool_speed[] = {
	(u32)SPEED_UNKNOWN, SPEED_10,	 SPEED_100,   SPEED_1000,   SPEED_10000,
	SPEED_25000,	    SPEED_40000, SPEED_50000, SPEED_100000, SPEED_200000
};

static int hinic3_ethtool_to_hw_speed_level(u32 speed)
{
	int i;

	for (i = 0; i < ARRAY_LEN(hw_to_ethtool_speed); i++) {
		if (hw_to_ethtool_speed[i] == speed)
			break;
	}

	return i;
}

static void
hinic3_add_ethtool_link_mode(struct cmd_link_settings *link_settings,
			     u32 hw_link_mode, u32 name)
{
	u32 link_mode;

	for (link_mode = 0; link_mode < LINK_MODE_MAX_NUMBERS; link_mode++) {
		if (hw_link_mode & BIT(link_mode)) {
			if (name == GET_SUPPORTED_MODE)
				ethtool_add_supported_speed_link_mode(link_settings,
								      link_mode);
			else
				ethtool_add_advertised_speed_link_mode(
						link_settings, link_mode);
		}
	}
}

static int hinic3_link_speed_set(struct hinic3_nic_dev *nic_dev,
				 struct cmd_link_settings *link_settings,
				 struct nic_port_info *port_info)
{
	u8 link_state = 0;
	int err;

	if (port_info->supported_mode != LINK_MODE_UNKNOWN)
		hinic3_add_ethtool_link_mode(link_settings,
					     port_info->supported_mode,
					     GET_SUPPORTED_MODE);
	if (port_info->advertised_mode != LINK_MODE_UNKNOWN)
		hinic3_add_ethtool_link_mode(link_settings,
					     port_info->advertised_mode,
					     GET_ADVERTISED_MODE);

	err = hinic3_get_link_state(nic_dev->hwdev, &link_state);
	if (!err && link_state) {
		if (hinic3_get_bond_create_mode(nic_dev->hwdev)) {
			link_settings->speed = port_info->bond_speed;
		} else {
			link_settings->speed =
				port_info->speed <
				ARRAY_LEN(hw_to_ethtool_speed) ?
				hw_to_ethtool_speed[port_info->speed] :
				(u32)SPEED_UNKNOWN;
		}

		link_settings->duplex = port_info->duplex;
	} else {
		link_settings->speed = (u32)SPEED_UNKNOWN;
		link_settings->duplex = DUPLEX_UNKNOWN;
	}

	return 0;
}

static void hinic3_link_port_type(struct cmd_link_settings *link_settings,
				  u8 port_type)
{
	switch (port_type) {
	case MAG_CMD_WIRE_TYPE_ELECTRIC:
		ETHTOOL_ADD_SUPPORTED_LINK_MODE(link_settings, TP);
		ETHTOOL_ADD_ADVERTISED_LINK_MODE(link_settings, TP);
		link_settings->port = PORT_TP;
		break;

	case MAG_CMD_WIRE_TYPE_AOC:
	case MAG_CMD_WIRE_TYPE_MM:
	case MAG_CMD_WIRE_TYPE_SM:
		ETHTOOL_ADD_SUPPORTED_LINK_MODE(link_settings, FIBRE);
		ETHTOOL_ADD_ADVERTISED_LINK_MODE(link_settings, FIBRE);
		link_settings->port = PORT_FIBRE;
		break;

	case MAG_CMD_WIRE_TYPE_COPPER:
		ETHTOOL_ADD_SUPPORTED_LINK_MODE(link_settings, FIBRE);
		ETHTOOL_ADD_ADVERTISED_LINK_MODE(link_settings, FIBRE);
		link_settings->port = PORT_DA;
		break;

	case MAG_CMD_WIRE_TYPE_BACKPLANE:
		ETHTOOL_ADD_SUPPORTED_LINK_MODE(link_settings, Backplane);
		ETHTOOL_ADD_ADVERTISED_LINK_MODE(link_settings, Backplane);
		link_settings->port = PORT_NONE;
		break;

	default:
		link_settings->port = PORT_OTHER;
		break;
	}
}

static int get_link_pause_settings(struct hinic3_nic_dev *nic_dev,
				   struct cmd_link_settings *link_settings)
{
	struct nic_pause_config nic_pause = { 0 };
	int err;

	err = hinic3_get_pause_info(nic_dev->hwdev, &nic_pause);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Failed to get pauseparam from hw\n");
		return err;
	}

	ETHTOOL_ADD_SUPPORTED_LINK_MODE(link_settings, Pause);
	if (nic_pause.rx_pause && nic_pause.tx_pause) {
		ETHTOOL_ADD_ADVERTISED_LINK_MODE(link_settings, Pause);
	} else if (nic_pause.tx_pause) {
		ETHTOOL_ADD_ADVERTISED_LINK_MODE(link_settings, Asym_Pause);
	} else if (nic_pause.rx_pause) {
		ETHTOOL_ADD_ADVERTISED_LINK_MODE(link_settings, Pause);
		ETHTOOL_ADD_ADVERTISED_LINK_MODE(link_settings, Asym_Pause);
	}

	return 0;
}

static bool is_bit_offset_defined(u8 bit_offset)
{
	if (bit_offset < __ETHTOOL_LINK_MODE_MASK_NBITS)
		return true;
	return false;
}

static void ethtool_add_supported_advertised_fec(struct cmd_link_settings *link_settings,
						 u32 fec, u8 cmd)
{
	u8 i;

	for (i = 0; i < HINIC_ETHTOOL_FEC_INFO_LEN; i++) {
		if ((fec & BIT(hinic3_ethtool_fec_info[i].hinic_fec_offset)) == 0)
			continue;
		if (is_bit_offset_defined(hinic3_ethtool_fec_info[i].ethtool_bit_offset) == true &&
		    cmd == HINIC_ADVERTISED_FEC_CMD) {
			set_bit(hinic3_ethtool_fec_info[i].ethtool_bit_offset,
				link_settings->advertising);
			return; /* There can be only one advertised fec mode. */
		}
		if (is_bit_offset_defined(hinic3_ethtool_fec_info[i].ethtool_bit_offset) == true &&
		    cmd == HINIC_SUPPORTED_FEC_CMD)
			set_bit(hinic3_ethtool_fec_info[i].ethtool_bit_offset,
				link_settings->supported);
	}
}

static void hinic3_link_fec_type(struct cmd_link_settings *link_settings,
				 u32 fec, u32 supported_fec)
{
	ethtool_add_supported_advertised_fec(link_settings, supported_fec,
					     HINIC_SUPPORTED_FEC_CMD);
	ethtool_add_supported_advertised_fec(link_settings, fec,
					     HINIC_ADVERTISED_FEC_CMD);
}

static int get_link_settings(struct net_device *netdev,
			     struct cmd_link_settings *link_settings)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	struct nic_port_info port_info = { 0 };
	int err;

	err = hinic3_get_port_info(nic_dev->hwdev, &port_info,
				   HINIC3_CHANNEL_NIC);
	if (err) {
		nicif_err(nic_dev, drv, netdev, "Failed to get port info\n");
		return err;
	}

	err = hinic3_link_speed_set(nic_dev, link_settings, &port_info);
	if (err)
		return err;

	hinic3_link_port_type(link_settings, port_info.port_type);

	/* port_info.fec is bit offset, value is BIT(port_info.fec);
	 * but port_info.supported_fec_mode is bit value
	 */
	hinic3_link_fec_type(link_settings, BIT(port_info.fec),
			     port_info.supported_fec_mode);

	link_settings->autoneg = port_info.autoneg_state == PORT_CFG_AN_ON ?
					 AUTONEG_ENABLE :
					 AUTONEG_DISABLE;
	if (port_info.autoneg_cap)
		ETHTOOL_ADD_SUPPORTED_LINK_MODE(link_settings, Autoneg);
	if (port_info.autoneg_state == PORT_CFG_AN_ON)
		ETHTOOL_ADD_ADVERTISED_LINK_MODE(link_settings, Autoneg);

	if (!HINIC3_FUNC_IS_VF(nic_dev->hwdev))
		err = get_link_pause_settings(nic_dev, link_settings);

	return err;
}

#ifdef ETHTOOL_GLINKSETTINGS
#ifndef XENSERVER_HAVE_NEW_ETHTOOL_OPS
int hinic3_get_link_ksettings(struct net_device *netdev,
			      struct ethtool_link_ksettings *link_settings)
{
	struct cmd_link_settings settings = { { 0 } };
	struct ethtool_link_settings *base = &link_settings->base;
	int err;

	ethtool_link_ksettings_zero_link_mode(link_settings, supported);
	ethtool_link_ksettings_zero_link_mode(link_settings, advertising);

	err = get_link_settings(netdev, &settings);
	if (err)
		return err;

	bitmap_copy(link_settings->link_modes.supported, settings.supported,
		    __ETHTOOL_LINK_MODE_MASK_NBITS);
	bitmap_copy(link_settings->link_modes.advertising, settings.advertising,
		    __ETHTOOL_LINK_MODE_MASK_NBITS);

	base->autoneg = settings.autoneg;
	base->speed = settings.speed;
	base->duplex = settings.duplex;
	base->port = settings.port;

	return 0;
}
#endif
#endif

static bool hinic3_is_support_speed(u32 supported_link, u32 speed)
{
	u32 link_mode;

	for (link_mode = 0; link_mode < LINK_MODE_MAX_NUMBERS; link_mode++) {
		if (!(supported_link & BIT(link_mode)))
			continue;

		if (hw2ethtool_link_mode_table[link_mode].speed == speed)
			return true;
	}

	return false;
}

static int hinic3_is_speed_legal(struct hinic3_nic_dev *nic_dev,
				 struct nic_port_info *port_info, u32 speed)
{
	struct net_device *netdev = nic_dev->netdev;
	int speed_level = 0;

	if (port_info->supported_mode == LINK_MODE_UNKNOWN ||
	    port_info->advertised_mode == LINK_MODE_UNKNOWN) {
		nicif_err(nic_dev, drv, netdev,
			  "Unknown supported link modes\n");
		return -EAGAIN;
	}

	speed_level = hinic3_ethtool_to_hw_speed_level(speed);
	if (speed_level >= PORT_SPEED_UNKNOWN ||
	    !hinic3_is_support_speed(port_info->supported_mode, speed)) {
		nicif_err(nic_dev, drv, netdev, "Not supported speed: %u\n",
			  speed);
		return -EINVAL;
	}

	return 0;
}

static int get_link_settings_type(struct hinic3_nic_dev *nic_dev, u8 autoneg,
				  u32 speed, u32 *set_settings)
{
	struct nic_port_info port_info = { 0 };
	int err;

	err = hinic3_get_port_info(nic_dev->hwdev, &port_info,
				   HINIC3_CHANNEL_NIC);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Failed to get current settings\n");
		return -EAGAIN;
	}

	/* Alwayse set autonegation */
	if (port_info.autoneg_cap)
		*set_settings |= HILINK_LINK_SET_AUTONEG;

	if (autoneg == AUTONEG_ENABLE) {
		if (!port_info.autoneg_cap) {
			nicif_err(nic_dev, drv, nic_dev->netdev,
				  "Not support autoneg\n");
			return -EOPNOTSUPP;
		}
	} else if (speed != (u32)SPEED_UNKNOWN) {
		/* Set speed only when autoneg is disable */
		err = hinic3_is_speed_legal(nic_dev, &port_info, speed);
		if (err)
			return err;

		*set_settings |= HILINK_LINK_SET_SPEED;
	} else {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Need to set speed when autoneg is off\n");
		return -EOPNOTSUPP;
	}

	return 0;
}

static int hinic3_set_settings_to_hw(struct hinic3_nic_dev *nic_dev,
				     u32 set_settings, u8 autoneg, u32 speed)
{
	struct net_device *netdev = nic_dev->netdev;
	struct hinic3_link_ksettings settings = { 0 };
	int speed_level = 0;
	char set_link_str[HINIC_SET_LINK_STR_LEN] = {0};
	char link_info[HINIC_SET_LINK_STR_LEN] = {0};
	int err = 0;

	snprintf(link_info, sizeof(link_info), "%s",
		 (bool)(set_settings & HILINK_LINK_SET_AUTONEG) ?
		 ((bool)autoneg ? "autong enable " : "autong disable ") : "");

	if (set_settings & HILINK_LINK_SET_SPEED) {
		speed_level = hinic3_ethtool_to_hw_speed_level(speed);
		snprintf(set_link_str, sizeof(set_link_str),
			 "%sspeed %u ", link_info, speed);
	}

	settings.valid_bitmap = set_settings;
	settings.autoneg = (bool)autoneg ? PORT_CFG_AN_ON : PORT_CFG_AN_OFF;
	settings.speed = (u8)speed_level;

	err = hinic3_set_link_settings(nic_dev->hwdev, &settings);
	if (err)
		nicif_err(nic_dev, drv, netdev, "Set %sfailed\n", set_link_str);
	else
		nicif_info(nic_dev, drv, netdev, "Set %ssuccess\n",
			   set_link_str);

	return err;
}

static int set_link_settings(struct net_device *netdev, u8 autoneg, u32 speed)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	u32 set_settings = 0;
	int err = 0;

	err = get_link_settings_type(nic_dev, autoneg, speed, &set_settings);
	if (err)
		return err;

	if (set_settings)
		err = hinic3_set_settings_to_hw(nic_dev, set_settings, autoneg,
						speed);
	else
		nicif_info(nic_dev, drv, netdev, "Nothing changed, exiting without setting anything\n");

	return err;
}

#ifdef ETHTOOL_GLINKSETTINGS
#ifndef XENSERVER_HAVE_NEW_ETHTOOL_OPS
int hinic3_set_link_ksettings(struct net_device *netdev,
			      const
			      struct ethtool_link_ksettings *link_settings)
{
	/* Only support to set autoneg and speed */
	return set_link_settings(netdev, link_settings->base.autoneg,
				 link_settings->base.speed);
}
#endif
#endif

#ifndef HAVE_NEW_ETHTOOL_LINK_SETTINGS_ONLY
int hinic3_get_settings(struct net_device *netdev, struct ethtool_cmd *ep)
{
	struct cmd_link_settings settings = { { 0 } };
	int err;

	err = get_link_settings(netdev, &settings);
	if (err)
		return err;

	ep->supported = settings.supported[0] & ((u32)~0);
	ep->advertising = settings.advertising[0] & ((u32)~0);

	ep->autoneg = settings.autoneg;
	ethtool_cmd_speed_set(ep, settings.speed);
	ep->duplex = settings.duplex;
	ep->port = settings.port;
	ep->transceiver = XCVR_INTERNAL;

	return 0;
}

int hinic3_set_settings(struct net_device *netdev,
			struct ethtool_cmd *link_settings)
{
	/* Only support to set autoneg and speed */
	return set_link_settings(netdev, link_settings->autoneg,
				 ethtool_cmd_speed(link_settings));
}
#endif

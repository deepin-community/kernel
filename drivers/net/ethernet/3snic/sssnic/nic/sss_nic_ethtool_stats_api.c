// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

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

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_nic_cfg.h"
#include "sss_nic_vf_cfg.h"
#include "sss_nic_mag_cfg.h"
#include "sss_nic_rss_cfg.h"
#include "sss_nic_dev_define.h"
#include "sss_nic_tx.h"
#include "sss_nic_rx.h"
#include "sss_nic_ethtool_stats_api.h"
#include "sss_tool_comm.h"
#include "sss_tool_nic.h"

#define SSSNIC_SET_SUPPORTED_MODE  0
#define SSSNIC_SET_ADVERTISED_MODE 1

#define SSSNIC_ETHTOOL_ADD_SUPPORTED_LINK_MODE(ecmd, mode)	\
		set_bit(ETHTOOL_LINK_MODE_##mode##_BIT, (ecmd)->supported)
#define SSSNIC_ETHTOOL_ADD_ADVERTISED_LINK_MODE(ecmd, mode)	\
		set_bit(ETHTOOL_LINK_MODE_##mode##_BIT, (ecmd)->advertising)

#define SSSNIC_ETHTOOL_ADD_SPPED_LINK_MODE(ecmd, mode, op) \
do { \
	u32 _link_mode; \
	unsigned long *val = ((op) == SSSNIC_SET_SUPPORTED_MODE) ? \
			(ecmd)->supported : (ecmd)->advertising; \
	for (_link_mode = 0; _link_mode < g_link_mode_table[mode].array_len; _link_mode++) { \
		if (g_link_mode_table[mode].array[_link_mode] >= \
			__ETHTOOL_LINK_MODE_MASK_NBITS) \
			continue; \
		set_bit(g_link_mode_table[mode].array[_link_mode], val); \
	} \
} while (0)

#define SSSNIC_NETDEV_STATS(_item) { \
	.name = #_item, \
	.len = FIELD_SIZEOF(struct rtnl_link_stats64, _item), \
	.offset = offsetof(struct rtnl_link_stats64, _item) \
}

#define SSSNIC_TX_STATS(_item) { \
	.name = #_item, \
	.len = FIELD_SIZEOF(struct sss_nic_tx_stats, _item), \
	.offset = offsetof(struct sss_nic_tx_stats, _item) \
}

#define SSSNIC_RQ_STATS(_item) { \
	.name = "rxq%d_"#_item, \
	.len = FIELD_SIZEOF(struct sss_nic_rq_stats, _item), \
	.offset = offsetof(struct sss_nic_rq_stats, _item) \
}

#define SSSNIC_SQ_STATS(_item) { \
	.name = "txq%d_"#_item, \
	.len = FIELD_SIZEOF(struct sss_nic_sq_stats, _item), \
	.offset = offsetof(struct sss_nic_sq_stats, _item) \
}

#define SSSNIC_FUNCTION_STATS(_item) {	\
	.name = #_item, \
	.len = FIELD_SIZEOF(struct sss_nic_port_stats, _item), \
	.offset = offsetof(struct sss_nic_port_stats, _item) \
}

#define SSSNIC_PORT_STATS(_item) { \
	.name = #_item, \
	.len = FIELD_SIZEOF(struct sss_nic_mag_port_stats, _item), \
	.offset = offsetof(struct sss_nic_mag_port_stats, _item) \
}

#define SSSNIC_GET_VALUE_OF_PTR(len, ptr) (		\
	(len) == sizeof(u64) ? *(u64 *)(ptr) :			\
	(len) == sizeof(u32) ? *(u32 *)(ptr) :			\
	(len) == sizeof(u16) ? *(u16 *)(ptr) : *(u8 *)(ptr)	\
)

#define SSSNIC_DEV_STATS_PACK(items, item_idx, array, stats_ptr) \
do { \
	int j; \
	for (j = 0; j < ARRAY_LEN(array); j++) { \
		memcpy((items)[item_idx].name, (array)[j].name, SSS_TOOL_SHOW_ITEM_LEN); \
		(items)[item_idx].hexadecimal = 0; \
		(items)[item_idx].value = \
			SSSNIC_GET_VALUE_OF_PTR((array)[j].len, \
						(char *)(stats_ptr) + (array)[j].offset); \
		(item_idx)++; \
	} \
} while (0)

#define SSSNIC_QUEUE_STATS_PACK(items, item_idx, array, stats_ptr, qid) \
do { \
	int j; \
	for (j = 0; j < ARRAY_LEN(array); j++) { \
		memcpy((items)[item_idx].name, (array)[j].name, \
		       SSS_TOOL_SHOW_ITEM_LEN); \
		snprintf((items)[item_idx].name, SSS_TOOL_SHOW_ITEM_LEN, \
			 (array)[j].name, (qid)); \
		(items)[item_idx].hexadecimal = 0; \
		(items)[item_idx].value = \
			SSSNIC_GET_VALUE_OF_PTR((array)[j].len, \
						(char *)(stats_ptr) + (array)[j].offset); \
		(item_idx)++; \
	} \
} while (0)

#define SSSNIC_CONVERT_DATA_TYPE(len, p) (((len) == sizeof(u64)) ? *(u64 *)(p) : *(u32 *)(p))
#define SSSNIC_AUTONEG_STRING(autoneg) ((autoneg) ? ("autong enable") : ("autong disable"))
#define SSSNIC_AUTONEG_ENABLE(autoneg) ((autoneg) ? SSSNIC_PORT_CFG_AN_ON : SSSNIC_PORT_CFG_AN_OFF)

#define SSSNIC_NEGATE_ZERO_U32 ((u32)~0)

struct sss_nic_hw2ethtool_link_mode {
	const u32 *array;
	u32 array_len;
	u32 speed;
};

typedef void (*sss_nic_port_type_handler_t)(struct sss_nic_cmd_link_settings *cmd);

static void sss_nic_set_fibre_port(struct sss_nic_cmd_link_settings *cmd);
static void sss_nic_set_da_port(struct sss_nic_cmd_link_settings *cmd);
static void sss_nic_set_tp_port(struct sss_nic_cmd_link_settings *cmd);
static void sss_nic_set_none_port(struct sss_nic_cmd_link_settings *cmd);

static char g_test_strings[][ETH_GSTRING_LEN] = {
	"Internal lb test  (on/offline)",
	"External lb test (external_lb)",
};

static char g_priv_flags_strings[][ETH_GSTRING_LEN] = {
	"Symmetric-RSS",
	"Force-Link-up",
	"Rxq_Recovery",
};

static struct sss_nic_stats g_nic_sq_stats[] = {
	SSSNIC_SQ_STATS(tx_packets),
	SSSNIC_SQ_STATS(tx_bytes),
	SSSNIC_SQ_STATS(tx_busy),
	SSSNIC_SQ_STATS(wake),
	SSSNIC_SQ_STATS(tx_dropped),
};

static struct sss_nic_stats g_nic_sq_stats_extern[] = {
	SSSNIC_SQ_STATS(skb_pad_err),
	SSSNIC_SQ_STATS(offload_err),
	SSSNIC_SQ_STATS(dma_map_err),
	SSSNIC_SQ_STATS(unknown_tunnel_proto),
	SSSNIC_SQ_STATS(frag_size_zero),
	SSSNIC_SQ_STATS(frag_len_overflow),
	SSSNIC_SQ_STATS(rsvd1),
	SSSNIC_SQ_STATS(rsvd2),
};

static struct sss_nic_stats g_nic_rq_stats[] = {
	SSSNIC_RQ_STATS(rx_packets),
	SSSNIC_RQ_STATS(rx_bytes),
	SSSNIC_RQ_STATS(errors),
	SSSNIC_RQ_STATS(csum_errors),
	SSSNIC_RQ_STATS(other_errors),
	SSSNIC_RQ_STATS(rx_dropped),
#ifdef HAVE_XDP_SUPPORT
	SSSNIC_RQ_STATS(xdp_dropped),
#endif
	SSSNIC_RQ_STATS(rx_buf_errors),
};

static struct sss_nic_stats g_nic_rq_stats_extern[] = {
	SSSNIC_RQ_STATS(alloc_rx_dma_err),
	SSSNIC_RQ_STATS(alloc_skb_err),
	SSSNIC_RQ_STATS(reset_drop_sge),
	SSSNIC_RQ_STATS(large_xdp_pkts),
	SSSNIC_RQ_STATS(rsvd2),
};

static struct sss_nic_stats g_netdev_stats[] = {
	SSSNIC_NETDEV_STATS(rx_packets),
	SSSNIC_NETDEV_STATS(tx_packets),
	SSSNIC_NETDEV_STATS(rx_bytes),
	SSSNIC_NETDEV_STATS(tx_bytes),
	SSSNIC_NETDEV_STATS(rx_errors),
	SSSNIC_NETDEV_STATS(tx_errors),
	SSSNIC_NETDEV_STATS(rx_dropped),
	SSSNIC_NETDEV_STATS(tx_dropped),
	SSSNIC_NETDEV_STATS(multicast),
	SSSNIC_NETDEV_STATS(collisions),
	SSSNIC_NETDEV_STATS(rx_length_errors),
	SSSNIC_NETDEV_STATS(rx_over_errors),
	SSSNIC_NETDEV_STATS(rx_crc_errors),
	SSSNIC_NETDEV_STATS(rx_frame_errors),
	SSSNIC_NETDEV_STATS(rx_fifo_errors),
	SSSNIC_NETDEV_STATS(rx_missed_errors),
	SSSNIC_NETDEV_STATS(tx_aborted_errors),
	SSSNIC_NETDEV_STATS(tx_carrier_errors),
	SSSNIC_NETDEV_STATS(tx_fifo_errors),
	SSSNIC_NETDEV_STATS(tx_heartbeat_errors),
};

static struct sss_nic_stats g_dev_stats[] = {
	SSSNIC_TX_STATS(tx_timeout),
};

static struct sss_nic_stats g_dev_stats_extern[] = {
	SSSNIC_TX_STATS(tx_drop),
	SSSNIC_TX_STATS(tx_invalid_qid),
	SSSNIC_TX_STATS(rsvd1),
	SSSNIC_TX_STATS(rsvd2),
};

static struct sss_nic_stats g_function_stats[] = {
	SSSNIC_FUNCTION_STATS(tx_unicast_pkts),
	SSSNIC_FUNCTION_STATS(tx_unicast_bytes),
	SSSNIC_FUNCTION_STATS(tx_multicast_pkts),
	SSSNIC_FUNCTION_STATS(tx_multicast_bytes),
	SSSNIC_FUNCTION_STATS(tx_broadcast_pkts),
	SSSNIC_FUNCTION_STATS(tx_broadcast_bytes),

	SSSNIC_FUNCTION_STATS(rx_unicast_pkts),
	SSSNIC_FUNCTION_STATS(rx_unicast_bytes),
	SSSNIC_FUNCTION_STATS(rx_multicast_pkts),
	SSSNIC_FUNCTION_STATS(rx_multicast_bytes),
	SSSNIC_FUNCTION_STATS(rx_broadcast_pkts),
	SSSNIC_FUNCTION_STATS(rx_broadcast_bytes),

	SSSNIC_FUNCTION_STATS(tx_discard),
	SSSNIC_FUNCTION_STATS(rx_discard),
	SSSNIC_FUNCTION_STATS(tx_err),
	SSSNIC_FUNCTION_STATS(rx_err),
};

static struct sss_nic_stats g_port_stats[] = {
	SSSNIC_PORT_STATS(tx_fragment_pkts),
	SSSNIC_PORT_STATS(tx_undersize_pkts),
	SSSNIC_PORT_STATS(tx_undermin_pkts),
	SSSNIC_PORT_STATS(tx_64_oct_pkts),
	SSSNIC_PORT_STATS(tx_65_127_oct_pkts),
	SSSNIC_PORT_STATS(tx_128_255_oct_pkts),
	SSSNIC_PORT_STATS(tx_256_511_oct_pkts),
	SSSNIC_PORT_STATS(tx_512_1023_oct_pkts),
	SSSNIC_PORT_STATS(tx_1024_1518_oct_pkts),
	SSSNIC_PORT_STATS(tx_1519_2047_oct_pkts),
	SSSNIC_PORT_STATS(tx_2048_4095_oct_pkts),
	SSSNIC_PORT_STATS(tx_4096_8191_oct_pkts),
	SSSNIC_PORT_STATS(tx_8192_9216_oct_pkts),
	SSSNIC_PORT_STATS(tx_9217_12287_oct_pkts),
	SSSNIC_PORT_STATS(tx_12288_16383_oct_pkts),
	SSSNIC_PORT_STATS(tx_1519_max_bad_pkts),
	SSSNIC_PORT_STATS(tx_1519_max_good_pkts),
	SSSNIC_PORT_STATS(tx_oversize_pkts),
	SSSNIC_PORT_STATS(tx_jabber_pkts),
	SSSNIC_PORT_STATS(tx_bad_pkts),
	SSSNIC_PORT_STATS(tx_bad_octs),
	SSSNIC_PORT_STATS(tx_good_pkts),
	SSSNIC_PORT_STATS(tx_good_octs),
	SSSNIC_PORT_STATS(tx_total_pkts),
	SSSNIC_PORT_STATS(tx_total_octs),
	SSSNIC_PORT_STATS(tx_uni_pkts),
	SSSNIC_PORT_STATS(tx_multi_pkts),
	SSSNIC_PORT_STATS(tx_broad_pkts),
	SSSNIC_PORT_STATS(tx_pauses),
	SSSNIC_PORT_STATS(tx_pfc_pkts),
	SSSNIC_PORT_STATS(tx_pfc_pri0_pkts),
	SSSNIC_PORT_STATS(tx_pfc_pri1_pkts),
	SSSNIC_PORT_STATS(tx_pfc_pri2_pkts),
	SSSNIC_PORT_STATS(tx_pfc_pri3_pkts),
	SSSNIC_PORT_STATS(tx_pfc_pri4_pkts),
	SSSNIC_PORT_STATS(tx_pfc_pri5_pkts),
	SSSNIC_PORT_STATS(tx_pfc_pri6_pkts),
	SSSNIC_PORT_STATS(tx_pfc_pri7_pkts),
	SSSNIC_PORT_STATS(tx_control_pkts),
	SSSNIC_PORT_STATS(tx_err_all_pkts),
	SSSNIC_PORT_STATS(tx_from_app_good_pkts),
	SSSNIC_PORT_STATS(tx_from_app_bad_pkts),

	SSSNIC_PORT_STATS(rx_fragment_pkts),
	SSSNIC_PORT_STATS(rx_undersize_pkts),
	SSSNIC_PORT_STATS(rx_undermin_pkts),
	SSSNIC_PORT_STATS(rx_64_oct_pkts),
	SSSNIC_PORT_STATS(rx_65_127_oct_pkts),
	SSSNIC_PORT_STATS(rx_128_255_oct_pkts),
	SSSNIC_PORT_STATS(rx_256_511_oct_pkts),
	SSSNIC_PORT_STATS(rx_512_1023_oct_pkts),
	SSSNIC_PORT_STATS(rx_1024_1518_oct_pkts),
	SSSNIC_PORT_STATS(rx_1519_2047_oct_pkts),
	SSSNIC_PORT_STATS(rx_2048_4095_oct_pkts),
	SSSNIC_PORT_STATS(rx_4096_8191_oct_pkts),
	SSSNIC_PORT_STATS(rx_8192_9216_oct_pkts),
	SSSNIC_PORT_STATS(rx_9217_12287_oct_pkts),
	SSSNIC_PORT_STATS(rx_12288_16383_oct_pkts),
	SSSNIC_PORT_STATS(rx_1519_max_bad_pkts),
	SSSNIC_PORT_STATS(rx_1519_max_good_pkts),
	SSSNIC_PORT_STATS(rx_oversize_pkts),
	SSSNIC_PORT_STATS(rx_jabber_pkts),
	SSSNIC_PORT_STATS(rx_bad_pkts),
	SSSNIC_PORT_STATS(rx_bad_octs),
	SSSNIC_PORT_STATS(rx_good_pkts),
	SSSNIC_PORT_STATS(rx_good_octs),
	SSSNIC_PORT_STATS(rx_total_pkts),
	SSSNIC_PORT_STATS(rx_total_octs),
	SSSNIC_PORT_STATS(rx_uni_pkts),
	SSSNIC_PORT_STATS(rx_multi_pkts),
	SSSNIC_PORT_STATS(rx_broad_pkts),
	SSSNIC_PORT_STATS(rx_pauses),
	SSSNIC_PORT_STATS(rx_pfc_pkts),
	SSSNIC_PORT_STATS(rx_pfc_pri0_pkts),
	SSSNIC_PORT_STATS(rx_pfc_pri1_pkts),
	SSSNIC_PORT_STATS(rx_pfc_pri2_pkts),
	SSSNIC_PORT_STATS(rx_pfc_pri3_pkts),
	SSSNIC_PORT_STATS(rx_pfc_pri4_pkts),
	SSSNIC_PORT_STATS(rx_pfc_pri5_pkts),
	SSSNIC_PORT_STATS(rx_pfc_pri6_pkts),
	SSSNIC_PORT_STATS(rx_pfc_pri7_pkts),
	SSSNIC_PORT_STATS(rx_control_pkts),
	SSSNIC_PORT_STATS(rx_sym_err_pkts),
	SSSNIC_PORT_STATS(rx_fcs_err_pkts),
	SSSNIC_PORT_STATS(rx_send_app_good_pkts),
	SSSNIC_PORT_STATS(rx_send_app_bad_pkts),
	SSSNIC_PORT_STATS(rx_unfilter_pkts),
};

static const u32 g_mag_link_mode_ge[] = {
	ETHTOOL_LINK_MODE_1000baseT_Full_BIT,
	ETHTOOL_LINK_MODE_1000baseKX_Full_BIT,
	ETHTOOL_LINK_MODE_1000baseX_Full_BIT,
};

static const u32 g_mag_link_mode_10ge_base_r[] = {
	ETHTOOL_LINK_MODE_10000baseKR_Full_BIT,
	ETHTOOL_LINK_MODE_10000baseR_FEC_BIT,
	ETHTOOL_LINK_MODE_10000baseCR_Full_BIT,
	ETHTOOL_LINK_MODE_10000baseSR_Full_BIT,
	ETHTOOL_LINK_MODE_10000baseLR_Full_BIT,
	ETHTOOL_LINK_MODE_10000baseLRM_Full_BIT,
};

static const u32 g_mag_link_mode_25ge_base_r[] = {
	ETHTOOL_LINK_MODE_25000baseCR_Full_BIT,
	ETHTOOL_LINK_MODE_25000baseKR_Full_BIT,
	ETHTOOL_LINK_MODE_25000baseSR_Full_BIT,
};

static const u32 g_mag_link_mode_40ge_base_r4[] = {
	ETHTOOL_LINK_MODE_40000baseKR4_Full_BIT,
	ETHTOOL_LINK_MODE_40000baseCR4_Full_BIT,
	ETHTOOL_LINK_MODE_40000baseSR4_Full_BIT,
	ETHTOOL_LINK_MODE_40000baseLR4_Full_BIT,
};

static const u32 g_mag_link_mode_50ge_base_r[] = {
	ETHTOOL_LINK_MODE_50000baseKR_Full_BIT,
	ETHTOOL_LINK_MODE_50000baseSR_Full_BIT,
	ETHTOOL_LINK_MODE_50000baseCR_Full_BIT,
};

static const u32 g_mag_link_mode_50ge_base_r2[] = {
	ETHTOOL_LINK_MODE_50000baseCR2_Full_BIT,
	ETHTOOL_LINK_MODE_50000baseKR2_Full_BIT,
	ETHTOOL_LINK_MODE_50000baseSR2_Full_BIT,
};

static const u32 g_mag_link_mode_100ge_base_r[] = {
	ETHTOOL_LINK_MODE_100000baseKR_Full_BIT,
	ETHTOOL_LINK_MODE_100000baseSR_Full_BIT,
	ETHTOOL_LINK_MODE_100000baseCR_Full_BIT,
};

static const u32 g_mag_link_mode_100ge_base_r2[] = {
	ETHTOOL_LINK_MODE_100000baseKR2_Full_BIT,
	ETHTOOL_LINK_MODE_100000baseSR2_Full_BIT,
	ETHTOOL_LINK_MODE_100000baseCR2_Full_BIT,
};

static const u32 g_mag_link_mode_100ge_base_r4[] = {
	ETHTOOL_LINK_MODE_100000baseKR4_Full_BIT,
	ETHTOOL_LINK_MODE_100000baseSR4_Full_BIT,
	ETHTOOL_LINK_MODE_100000baseCR4_Full_BIT,
	ETHTOOL_LINK_MODE_100000baseLR4_ER4_Full_BIT,
};

static const u32 g_mag_link_mode_200ge_base_r2[] = {
	ETHTOOL_LINK_MODE_200000baseKR2_Full_BIT,
	ETHTOOL_LINK_MODE_200000baseSR2_Full_BIT,
	ETHTOOL_LINK_MODE_200000baseCR2_Full_BIT,
};

static const u32 g_mag_link_mode_200ge_base_r4[] = {
	ETHTOOL_LINK_MODE_200000baseKR4_Full_BIT,
	ETHTOOL_LINK_MODE_200000baseSR4_Full_BIT,
	ETHTOOL_LINK_MODE_200000baseCR4_Full_BIT,
};

static const struct sss_nic_hw2ethtool_link_mode
	g_link_mode_table[SSSNIC_LINK_MODE_MAX_NUMBERS] = {
	[SSSNIC_LINK_MODE_GE] = {
		.array = g_mag_link_mode_ge,
		.array_len = ARRAY_LEN(g_mag_link_mode_ge),
		.speed = SPEED_1000,
	},
	[SSSNIC_LINK_MODE_10GE_BASE_R] = {
		.array = g_mag_link_mode_10ge_base_r,
		.array_len = ARRAY_LEN(g_mag_link_mode_10ge_base_r),
		.speed = SPEED_10000,
	},
	[SSSNIC_LINK_MODE_25GE_BASE_R] = {
		.array = g_mag_link_mode_25ge_base_r,
		.array_len = ARRAY_LEN(g_mag_link_mode_25ge_base_r),
		.speed = SPEED_25000,
	},
	[SSSNIC_LINK_MODE_40GE_BASE_R4] = {
		.array = g_mag_link_mode_40ge_base_r4,
		.array_len = ARRAY_LEN(g_mag_link_mode_40ge_base_r4),
		.speed = SPEED_40000,
	},
	[SSSNIC_LINK_MODE_50GE_BASE_R] = {
		.array = g_mag_link_mode_50ge_base_r,
		.array_len = ARRAY_LEN(g_mag_link_mode_50ge_base_r),
		.speed = SPEED_50000,
	},
	[SSSNIC_LINK_MODE_50GE_BASE_R2] = {
		.array = g_mag_link_mode_50ge_base_r2,
		.array_len = ARRAY_LEN(g_mag_link_mode_50ge_base_r2),
		.speed = SPEED_50000,
	},
	[SSSNIC_LINK_MODE_100GE_BASE_R] = {
		.array = g_mag_link_mode_100ge_base_r,
		.array_len = ARRAY_LEN(g_mag_link_mode_100ge_base_r),
		.speed = SPEED_100000,
	},
	[SSSNIC_LINK_MODE_100GE_BASE_R2] = {
		.array = g_mag_link_mode_100ge_base_r2,
		.array_len = ARRAY_LEN(g_mag_link_mode_100ge_base_r2),
		.speed = SPEED_100000,
	},
	[SSSNIC_LINK_MODE_100GE_BASE_R4] = {
		.array = g_mag_link_mode_100ge_base_r4,
		.array_len = ARRAY_LEN(g_mag_link_mode_100ge_base_r4),
		.speed = SPEED_100000,
	},
	[SSSNIC_LINK_MODE_200GE_BASE_R2] = {
		.array = g_mag_link_mode_200ge_base_r2,
		.array_len = ARRAY_LEN(g_mag_link_mode_200ge_base_r2),
		.speed = SPEED_200000,
	},
	[SSSNIC_LINK_MODE_200GE_BASE_R4] = {
		.array = g_mag_link_mode_200ge_base_r4,
		.array_len = ARRAY_LEN(g_mag_link_mode_200ge_base_r4),
		.speed = SPEED_200000,
	},
};

/* Related to enum sss_nic_mag_opcode_port_speed */
static u32 g_hw_to_ethtool_speed[] = {
	(u32)SPEED_UNKNOWN, SPEED_10, SPEED_100, SPEED_1000, SPEED_10000,
	SPEED_25000, SPEED_40000, SPEED_50000, SPEED_100000, SPEED_200000
};

static sss_nic_port_type_handler_t g_link_port_set_handler[] = {
	NULL,
	sss_nic_set_fibre_port,
	sss_nic_set_fibre_port,
	sss_nic_set_da_port,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	sss_nic_set_fibre_port,
	sss_nic_set_tp_port,
	sss_nic_set_none_port
};

u32 sss_nic_get_io_stats_size(const struct sss_nic_dev *nic_dev)
{
	u32 count;

	count = ARRAY_LEN(g_dev_stats) +
		ARRAY_LEN(g_dev_stats_extern) +
		(ARRAY_LEN(g_nic_sq_stats) +
		ARRAY_LEN(g_nic_sq_stats_extern) +
		ARRAY_LEN(g_nic_rq_stats) +
		ARRAY_LEN(g_nic_rq_stats_extern)) * nic_dev->max_qp_num;

	return count;
}

int sss_nic_eth_ss_test(struct sss_nic_dev *nic_dev)
{
	return ARRAY_LEN(g_test_strings);
}

int sss_nic_eth_ss_stats(struct sss_nic_dev *nic_dev)
{
	int count;
	int q_num;

	q_num = nic_dev->qp_res.qp_num;
	count = ARRAY_LEN(g_netdev_stats) + ARRAY_LEN(g_dev_stats) +
		ARRAY_LEN(g_function_stats) + (ARRAY_LEN(g_nic_sq_stats) +
					       ARRAY_LEN(g_nic_rq_stats)) * q_num;

	if (!SSSNIC_FUNC_IS_VF(nic_dev->hwdev))
		count += ARRAY_LEN(g_port_stats);

	return count;
}

int sss_nic_eth_ss_priv_flags(struct sss_nic_dev *nic_dev)
{
	return ARRAY_LEN(g_priv_flags_strings);
}

static void sss_nic_get_ethtool_stats_data(char *ethtool_stats,
					   struct sss_nic_stats *stats, u16 stats_len, u64 *data)
{
	u16 i = 0;
	u16 j = 0;
	char *ptr = NULL;

	for (j = 0; j < stats_len; j++) {
		ptr = ethtool_stats + stats[j].offset;
		data[i] = SSSNIC_CONVERT_DATA_TYPE(stats[j].len, ptr);
		i++;
	}
}

u16 sss_nic_get_ethtool_dev_stats(struct sss_nic_dev *nic_dev,
				  u64 *data)
{
	u16 cnt = 0;
#ifdef HAVE_NDO_GET_STATS64
	struct rtnl_link_stats64 temp;
	const struct rtnl_link_stats64 *net_stats = NULL;

	net_stats = dev_get_stats(nic_dev->netdev, &temp);
#else
	const struct net_device_stats *net_stats = NULL;

	net_stats = dev_get_stats(nic_dev->netdev);
#endif

	sss_nic_get_ethtool_stats_data((char *)net_stats, g_netdev_stats,
				       ARRAY_LEN(g_netdev_stats), data);
	cnt += ARRAY_LEN(g_netdev_stats);

	sss_nic_get_ethtool_stats_data((char *)&nic_dev->tx_stats, g_dev_stats,
				       ARRAY_LEN(g_dev_stats), data + cnt);
	cnt += ARRAY_LEN(g_dev_stats);

	return cnt;
}

void sss_nic_get_drv_queue_stats(struct sss_nic_dev *nic_dev, u64 *data)
{
	u16 qid;
	struct sss_nic_rq_stats rq_stats = {0};
	struct sss_nic_sq_stats sq_stats = {0};

	for (qid = 0; qid < nic_dev->qp_res.qp_num; qid++) {
		if (!nic_dev->sq_desc_group)
			break;

		sss_nic_get_sq_stats(&nic_dev->sq_desc_group[qid], &sq_stats);
		sss_nic_get_ethtool_stats_data((char *)&sq_stats, g_nic_sq_stats,
					       ARRAY_LEN(g_nic_sq_stats),
					       data + qid * ARRAY_LEN(g_nic_sq_stats));
	}

	data += ARRAY_LEN(g_nic_sq_stats) * nic_dev->qp_res.qp_num;

	for (qid = 0; qid < nic_dev->qp_res.qp_num; qid++) {
		if (!nic_dev->rq_desc_group)
			break;

		sss_nic_get_rq_stats(&nic_dev->rq_desc_group[qid], &rq_stats);
		sss_nic_get_ethtool_stats_data((char *)&rq_stats, g_nic_rq_stats,
					       ARRAY_LEN(g_nic_rq_stats),
					       data + qid * ARRAY_LEN(g_nic_rq_stats));
	}
}

int sss_nic_get_ethtool_vport_stats(struct sss_nic_dev *nic_dev,
				    u64 *data)
{
	int ret;
	struct sss_nic_port_stats vport_stats = {0};

	ret = sss_nic_get_vport_stats(nic_dev, sss_get_global_func_id(nic_dev->hwdev),
				      &vport_stats);
	if (ret != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Fail to get function stats from fw, ret:%d\n", ret);
		return ARRAY_LEN(g_function_stats);
	}
	sss_nic_get_ethtool_stats_data((char *)&vport_stats, g_function_stats,
				       ARRAY_LEN(g_function_stats), data);

	return ARRAY_LEN(g_function_stats);
}

u16 sss_nic_get_ethtool_port_stats(struct sss_nic_dev *nic_dev,
				   u64 *data)
{
	int ret;
	u16 i = 0;
	struct sss_nic_mag_port_stats *stats = NULL;

	stats = kzalloc(sizeof(*stats), GFP_KERNEL);
	if (!stats) {
		memset(&data[i], 0, ARRAY_LEN(g_port_stats) * sizeof(*data));
		nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to Malloc port stats\n");
		return ARRAY_LEN(g_port_stats);
	}

	ret = sss_nic_get_phy_port_stats(nic_dev, stats);
	if (ret != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to get port stats from fw\n");
		goto out;
	}

	sss_nic_get_ethtool_stats_data((char *)stats, g_port_stats,
				       ARRAY_LEN(g_port_stats), data);

out:
	kfree(stats);

	return ARRAY_LEN(g_port_stats);
}

u16 sss_nic_get_stats_strings(struct sss_nic_stats *stats,
			      u16 stats_len, char *buffer)
{
	u16 i;

	for (i = 0; i < stats_len; i++) {
		memcpy(buffer, stats[i].name, ETH_GSTRING_LEN);
		buffer += ETH_GSTRING_LEN;
	}

	return i;
}

u16 sss_nic_get_drv_dev_strings(struct sss_nic_dev *nic_dev,
				char *buffer)
{
	u16 cnt =
		sss_nic_get_stats_strings(g_netdev_stats, ARRAY_LEN(g_netdev_stats), buffer);
	cnt += sss_nic_get_stats_strings(g_dev_stats, ARRAY_LEN(g_dev_stats),
					 buffer + cnt * ETH_GSTRING_LEN);

	return cnt;
}

u16 sss_nic_get_hw_stats_strings(struct sss_nic_dev *nic_dev,
				 char *buffer)
{
	u16 cnt = sss_nic_get_stats_strings(g_function_stats,
					    ARRAY_LEN(g_function_stats), buffer);

	if (SSSNIC_FUNC_IS_VF(nic_dev->hwdev))
		return cnt;

	cnt += sss_nic_get_stats_strings(g_port_stats,
					 ARRAY_LEN(g_port_stats), buffer + cnt * ETH_GSTRING_LEN);

	return cnt;
}

int sss_nic_get_queue_stats_cnt(const struct sss_nic_dev *nic_dev,
				struct sss_nic_stats *stats, u16 stats_len, u16 qid, char *buffer)
{
	int ret;
	u16 i;

	for (i = 0; i < stats_len; i++) {
		ret = sprintf(buffer, stats[i].name, qid);
		if (ret < 0)
			nicif_err(nic_dev, drv, nic_dev->netdev,
				  "Fail to sprintf stats name:%s, qid: %u, stats id: %u\n",
				  stats[i].name, qid, i);
		buffer += ETH_GSTRING_LEN;
	}

	return i;
}

u16 sss_nic_get_qp_stats_strings(const struct sss_nic_dev *nic_dev,
				 char *buffer)
{
	u16 qid = 0;
	u16 cnt = 0;

	for (qid = 0; qid < nic_dev->qp_res.qp_num; qid++)
		cnt += sss_nic_get_queue_stats_cnt(nic_dev, g_nic_sq_stats,
						   ARRAY_LEN(g_nic_sq_stats), qid,
						   buffer + cnt * ETH_GSTRING_LEN);

	for (qid = 0; qid < nic_dev->qp_res.qp_num; qid++)
		cnt += sss_nic_get_queue_stats_cnt(nic_dev, g_nic_rq_stats,
						   ARRAY_LEN(g_nic_rq_stats), qid,
						   buffer + cnt * ETH_GSTRING_LEN);

	return cnt;
}

void sss_nic_get_test_strings(struct sss_nic_dev *nic_dev, u8 *buffer)
{
	memcpy(buffer, *g_test_strings, sizeof(g_test_strings));
}

void sss_nic_get_drv_stats_strings(struct sss_nic_dev *nic_dev,
				   u8 *buffer)
{
	u16 offset = 0;

	offset = sss_nic_get_drv_dev_strings(nic_dev, buffer);
	offset += sss_nic_get_hw_stats_strings(nic_dev, buffer + offset * ETH_GSTRING_LEN);
	sss_nic_get_qp_stats_strings(nic_dev, buffer + offset * ETH_GSTRING_LEN);
}

void sss_nic_get_priv_flags_strings(struct sss_nic_dev *nic_dev,
				    u8 *buffer)
{
	memcpy(buffer, g_priv_flags_strings, sizeof(g_priv_flags_strings));
}

int sss_nic_get_speed_level(u32 speed)
{
	int level;

	for (level = 0; level < ARRAY_LEN(g_hw_to_ethtool_speed); level++) {
		if (g_hw_to_ethtool_speed[level] == speed)
			break;
	}

	return level;
}

void sss_nic_add_ethtool_link_mode(struct sss_nic_cmd_link_settings *cmd,
				   u32 hw_mode, u32 op)
{
	u32 i;

	for (i = 0; i < SSSNIC_LINK_MODE_MAX_NUMBERS; i++) {
		if (test_bit(i, (unsigned long *)&hw_mode))
			SSSNIC_ETHTOOL_ADD_SPPED_LINK_MODE(cmd, i, op);
	}
}

void sss_nic_set_link_speed(struct sss_nic_dev *nic_dev,
			    struct sss_nic_cmd_link_settings *cmd,
			    struct sss_nic_port_info *port_info)
{
	int ret;
	u8 state = 0;

	if (port_info->supported_mode != SSSNIC_LINK_MODE_UNKNOWN)
		sss_nic_add_ethtool_link_mode(cmd,
					      port_info->supported_mode,
					      SSSNIC_SET_SUPPORTED_MODE);
	if (port_info->advertised_mode != SSSNIC_LINK_MODE_UNKNOWN)
		sss_nic_add_ethtool_link_mode(cmd,
					      port_info->advertised_mode,
					      SSSNIC_SET_ADVERTISED_MODE);

	ret = sss_nic_get_hw_link_state(nic_dev, &state);
	if (ret != 0 || state == 0) {
		cmd->duplex = DUPLEX_UNKNOWN;
		cmd->speed = (u32)SPEED_UNKNOWN;
		return;
	}

	cmd->duplex = port_info->duplex;
	cmd->speed = port_info->speed < ARRAY_LEN(g_hw_to_ethtool_speed) ?
		     g_hw_to_ethtool_speed[port_info->speed] : (u32)SPEED_UNKNOWN;
}

static void sss_nic_set_fibre_port(struct sss_nic_cmd_link_settings *cmd)
{
	SSSNIC_ETHTOOL_ADD_SUPPORTED_LINK_MODE(cmd, FIBRE);
	SSSNIC_ETHTOOL_ADD_ADVERTISED_LINK_MODE(cmd, FIBRE);
	cmd->port = PORT_FIBRE;
}

static void sss_nic_set_da_port(struct sss_nic_cmd_link_settings *cmd)
{
	SSSNIC_ETHTOOL_ADD_SUPPORTED_LINK_MODE(cmd, FIBRE);
	SSSNIC_ETHTOOL_ADD_ADVERTISED_LINK_MODE(cmd, FIBRE);
	cmd->port = PORT_DA;
}

static void sss_nic_set_tp_port(struct sss_nic_cmd_link_settings *cmd)
{
	SSSNIC_ETHTOOL_ADD_SUPPORTED_LINK_MODE(cmd, TP);
	SSSNIC_ETHTOOL_ADD_ADVERTISED_LINK_MODE(cmd, TP);
	cmd->port = PORT_TP;
}

static void sss_nic_set_none_port(struct sss_nic_cmd_link_settings *cmd)
{
	SSSNIC_ETHTOOL_ADD_SUPPORTED_LINK_MODE(cmd, Backplane);
	SSSNIC_ETHTOOL_ADD_ADVERTISED_LINK_MODE(cmd, Backplane);
	cmd->port = PORT_NONE;
}

void sss_nic_link_port_type(struct sss_nic_cmd_link_settings *cmd,
			    u8 port_type)
{
	if (port_type >= ARRAY_LEN(g_link_port_set_handler)) {
		cmd->port = PORT_OTHER;
		return;
	}

	if (!g_link_port_set_handler[port_type]) {
		cmd->port = PORT_OTHER;
		return;
	}

	g_link_port_set_handler[port_type](cmd);
}

int sss_nic_get_link_pause_setting(struct sss_nic_dev *nic_dev,
				   struct sss_nic_cmd_link_settings *cmd)
{
	int ret;
	struct sss_nic_pause_cfg pause_config = {0};

	ret = sss_nic_get_hw_pause_info(nic_dev, &pause_config);
	if (ret != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Fail to get pauseparam from hw\n");
		return ret;
	}

	SSSNIC_ETHTOOL_ADD_SUPPORTED_LINK_MODE(cmd, Pause);
	if (pause_config.rx_pause != 0 && pause_config.tx_pause != 0) {
		SSSNIC_ETHTOOL_ADD_ADVERTISED_LINK_MODE(cmd, Pause);
		return 0;
	}

	SSSNIC_ETHTOOL_ADD_ADVERTISED_LINK_MODE(cmd, Asym_Pause);
	if (pause_config.rx_pause != 0)
		SSSNIC_ETHTOOL_ADD_ADVERTISED_LINK_MODE(cmd, Pause);

	return 0;
}

int sss_nic_get_link_setting(struct net_device *net_dev,
			     struct sss_nic_cmd_link_settings *cmd)
{
	int ret;
	struct sss_nic_dev *nic_dev = netdev_priv(net_dev);
	struct sss_nic_port_info info = {0};

	ret = sss_nic_get_hw_port_info(nic_dev, &info, SSS_CHANNEL_NIC);
	if (ret != 0) {
		nicif_err(nic_dev, drv, net_dev, "Fail to get port info\n");
		return ret;
	}

	sss_nic_set_link_speed(nic_dev, cmd, &info);
	sss_nic_link_port_type(cmd, info.port_type);

	cmd->autoneg = info.autoneg_state == SSSNIC_PORT_CFG_AN_ON ?
		       AUTONEG_ENABLE : AUTONEG_DISABLE;
	if (info.autoneg_cap != 0)
		SSSNIC_ETHTOOL_ADD_SUPPORTED_LINK_MODE(cmd, Autoneg);
	if (info.autoneg_state == SSSNIC_PORT_CFG_AN_ON)
		SSSNIC_ETHTOOL_ADD_ADVERTISED_LINK_MODE(cmd, Autoneg);

	if (!SSSNIC_FUNC_IS_VF(nic_dev->hwdev))
		ret = sss_nic_get_link_pause_setting(nic_dev, cmd);

	return ret;
}

#ifdef ETHTOOL_GLINKSETTINGS
#ifndef XENSERVER_HAVE_NEW_ETHTOOL_OPS
void sss_nic_copy_ksetting(struct ethtool_link_ksettings *ksetting,
			   struct sss_nic_cmd_link_settings *cmd)
{
	struct ethtool_link_settings *setting = &ksetting->base;

	bitmap_copy(ksetting->link_modes.advertising, cmd->advertising,
		    __ETHTOOL_LINK_MODE_MASK_NBITS);
	bitmap_copy(ksetting->link_modes.supported, cmd->supported,
		    __ETHTOOL_LINK_MODE_MASK_NBITS);

	setting->speed = cmd->speed;
	setting->duplex = cmd->duplex;
	setting->port = cmd->port;
	setting->autoneg = cmd->autoneg;
}
#endif
#endif

bool sss_nic_is_support_speed(u32 support_mode, u32 speed)
{
	u32 link_mode;

	for (link_mode = 0; link_mode < SSSNIC_LINK_MODE_MAX_NUMBERS; link_mode++) {
		if ((support_mode & BIT(link_mode)) == 0)
			continue;

		if (g_link_mode_table[link_mode].speed == speed)
			return true;
	}

	return false;
}

int sss_nic_get_link_settings_param(struct sss_nic_dev *nic_dev,
				    u8 autoneg, u32 speed, u32 *settings)
{
	struct sss_nic_port_info info = {0};
	int ret;
	int level;

	ret = sss_nic_get_hw_port_info(nic_dev, &info, SSS_CHANNEL_NIC);
	if (ret != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to get port info\n");
		return -EAGAIN;
	}

	if (info.autoneg_cap != 0)
		*settings |= SSSNIC_LINK_SET_AUTONEG;

	if (autoneg == AUTONEG_ENABLE) {
		if (info.autoneg_cap == 0) {
			nicif_err(nic_dev, drv, nic_dev->netdev, "Unsupport autoneg\n");
			return -EOPNOTSUPP;
		}

		return 0;
	}

	if (speed != (u32)SPEED_UNKNOWN) {
		if (info.supported_mode == SSSNIC_LINK_MODE_UNKNOWN ||
		    info.advertised_mode == SSSNIC_LINK_MODE_UNKNOWN) {
			nicif_err(nic_dev, drv, nic_dev->netdev, "Unsupport link mode\n");
			return -EAGAIN;
		}

		/* Set speed only when autoneg is disable */
		level = sss_nic_get_speed_level(speed);
		if (level >= SSSNIC_PORT_SPEED_UNKNOWN ||
		    !sss_nic_is_support_speed(info.supported_mode, speed)) {
			nicif_err(nic_dev, drv, nic_dev->netdev, "Unsupport speed: %u\n", speed);
			return -EINVAL;
		}

		*settings |= SSSNIC_LINK_SET_SPEED;
		return 0;
	}

	nicif_err(nic_dev, drv, nic_dev->netdev, "Set speed when autoneg is off\n");
	return -EOPNOTSUPP;
}

int sss_nic_set_settings_to_hw(struct sss_nic_dev *nic_dev,
			       u8 autoneg, u32 speed, u32 settings)
{
	int ret;
	int level = 0;
	char cmd_str[128] = {0};
	struct sss_nic_link_ksettings cmd = {0};
	struct net_device *netdev = nic_dev->netdev;
	char *str = (bool)((settings & SSSNIC_LINK_SET_AUTONEG) != 0) ?
		    SSSNIC_AUTONEG_STRING((bool)autoneg) : "";

	ret = snprintf(cmd_str, sizeof(cmd_str) - 1, "%s", str);
	if (ret < 0)
		return -EINVAL;

	if ((settings & SSSNIC_LINK_SET_SPEED) != 0) {
		level = sss_nic_get_speed_level(speed);
		ret = sprintf(cmd_str + strlen(cmd_str), "speed %u ", speed);
		if (ret < 0)
			return -EINVAL;
	}

	cmd.valid_bitmap = settings;
	cmd.autoneg = SSSNIC_AUTONEG_ENABLE((bool)autoneg);
	cmd.speed = (u8)level;

	ret = sss_nic_set_link_settings(nic_dev, &cmd);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to set %s\n", cmd_str);
		return ret;
	}

	nicif_info(nic_dev, drv, netdev, "Success to set %s, ret: %d\n", cmd_str, ret);
	return 0;
}

int sssnic_set_link_settings(struct net_device *netdev,
			     u8 autoneg, u32 speed)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	u32 settings = 0;
	int ret;

	ret = sss_nic_get_link_settings_param(nic_dev, autoneg, speed, &settings);
	if (ret != 0)
		return ret;

	if (settings != 0)
		return sss_nic_set_settings_to_hw(nic_dev, autoneg, speed, settings);

	nicif_info(nic_dev, drv, netdev, "Nothing change, exit.\n");

	return 0;
}

void sss_nic_get_io_stats(const struct sss_nic_dev *nic_dev, void *stats)
{
	struct sss_tool_show_item *items = stats;
	int item_idx = 0;
	u16 qid;

	SSSNIC_DEV_STATS_PACK(items, item_idx, g_dev_stats, &nic_dev->tx_stats);
	SSSNIC_DEV_STATS_PACK(items, item_idx, g_dev_stats_extern,
			      &nic_dev->tx_stats);

	for (qid = 0; qid < nic_dev->max_qp_num; qid++) {
		SSSNIC_QUEUE_STATS_PACK(items, item_idx, g_nic_sq_stats,
					&nic_dev->sq_desc_group[qid].stats, qid);
		SSSNIC_QUEUE_STATS_PACK(items, item_idx, g_nic_sq_stats_extern,
					&nic_dev->sq_desc_group[qid].stats, qid);
	}

	for (qid = 0; qid < nic_dev->max_qp_num; qid++) {
		SSSNIC_QUEUE_STATS_PACK(items, item_idx, g_nic_rq_stats,
					&nic_dev->rq_desc_group[qid].stats, qid);
		SSSNIC_QUEUE_STATS_PACK(items, item_idx, g_nic_rq_stats_extern,
					&nic_dev->rq_desc_group[qid].stats, qid);
	}
}

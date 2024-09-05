#ifndef __SXE_ETHTOOL_H__
#define __SXE_ETHTOOL_H__

#include <linux/ethtool.h>
#include "sxe.h"

#define SXE_FNAV_RULES_TABLE_SIZE_UNIT  (1024)
#define UDP_RSS_FLAGS (SXE_RSS_FIELD_IPV4_UDP | \
		       SXE_RSS_FIELD_IPV6_UDP)

#define SXE_SFP_INIT_WAIT_ITR_MIN	(1000)
#define SXE_SFP_INIT_WAIT_ITR_MAX	(2000)

#define SXE_TEST_GSTRING_ARRAY_SIZE sxe_self_test_suite_num_get()

#define SXE_RX_RING_NUM netdev->num_tx_queues
#define SXE_STATS_ARRAY_SIZE sxe_stats_num_get()
#define SXE_RING_STATS_LEN ( \
		(netdev->num_tx_queues + SXE_RX_RING_NUM) * \
		(sizeof(struct sxe_ring_stats) / sizeof(u64)))
#define SXE_DBU_PKT_BUF_STATS_LEN ( \
		(sizeof(((struct sxe_adapter *)0)->stats.hw.dburxtcin) + \
		sizeof(((struct sxe_adapter *)0)->stats.hw.dburxtcout) + \
		sizeof(((struct sxe_adapter *)0)->stats.hw.dburxgdreecnt) + \
		sizeof(((struct sxe_adapter *)0)->stats.hw.dburxdrofpcnt) + \
		sizeof(((struct sxe_adapter *)0)->stats.hw.dbutxtcin) + \
		sizeof(((struct sxe_adapter *)0)->stats.hw.dbutxtcout)) \
		/ sizeof(u64))

#define SXE_DMA_QUEUE_STATS_NUM 5
#define SXE_DMA_QUEUE_STATS_LEN ( SXE_DCB_8_TC * SXE_DMA_QUEUE_STATS_NUM )

#define SXE_DMA_STATS_LEN (SXE_DMA_QUEUE_STATS_LEN + sxe_dma_stats_num_get())

#define SXE_FC_STATS_LEN ( \
		(sizeof(((struct sxe_adapter *)0)->stats.hw.prcpf) + \
		sizeof(((struct sxe_adapter *)0)->stats.hw.pfct)) \
		/ sizeof(u64) )

#define SXE_STATS_LEN (SXE_STATS_ARRAY_SIZE + \
		SXE_RING_STATS_LEN + \
		SXE_DBU_PKT_BUF_STATS_LEN + \
		SXE_DMA_STATS_LEN + \
		SXE_FC_STATS_LEN)

#define SXE_PRIV_FLAGS_LEGACY_RX	BIT(0)
#ifdef SXE_IPSEC_CONFIGURE
#define SXE_PRIV_FLAGS_VF_IPSEC_EN	BIT(1)
#endif
#define SXE_PRIV_FLAGS_STR_LEN		sxe_priv_flags_num_get()

#define SXE_ETHTOOL_DUMP_REGS_NUM 	(SXE_MAC_REGS_NUM + \
						SXE_MAC_STATS_REGS_NUM)
#define SXE_ETHTOOL_DUMP_REGS_LEN 	(SXE_MAC_REGS_VAL_LEN + \
						SXE_MAC_STATS_REGS_VAL_LEN)
#define SXE_MAC_REGS_NUM		sxe_mac_reg_num_get()
#define SXE_MAC_REGS_VAL_LEN		(SXE_MAC_REGS_NUM * sizeof(u32))
#define SXE_MAC_STATS_REGS_NUM		sxe_mac_stats_regs_num_get()
#define SXE_MAC_STATS_REGS_VAL_LEN	(sizeof(u64) * SXE_MAC_STATS_REGS_NUM)

#define SXE_LOOPBACK_TEST_DESC_COUNT	64
#define SXE_LOOPBACK_TEST_LOOP			2
#define SXE_LOOPBACK_TEST_FRAME_SIZE	1024

#define SXE_LPBK_TX_DISB_WAIT_MIN	(10000)
#define SXE_LPBK_TX_DISB_WAIT_MAX	(20000)

#define SXE_NIC_RESET_WAIT_MIN		(1000)
#define SXE_NIC_RESET_WAIT_MAX		(2000)

enum {
	NETDEV_STATS, 
	SXE_STATS
};

struct sxe_ethtool_stats {
	s8 stat_string[ETH_GSTRING_LEN];
	u32 type;
	u32 sizeof_stat;
	u32 stat_offset;
};

struct sxe_mac_stats_info {
	s8 stat_string[ETH_GSTRING_LEN];
	u32 stat_offset;
};

u32 sxe_dma_stats_num_get(void);

u32 sxe_mac_stats_regs_num_get(void);

u32 sxe_self_test_suite_num_get(void);

u32 sxe_stats_num_get(void);

u32 sxe_priv_flags_num_get(void);

void sxe_ethtool_ops_set(struct net_device *netdev);

s32 sxe_fnav_dest_queue_parse(struct sxe_adapter *adapter,
				u64 ring_cookie,
				u8 *queue);

s32 sxe_fw_wol_set(struct sxe_adapter *adapter, u32 enable);

#endif 

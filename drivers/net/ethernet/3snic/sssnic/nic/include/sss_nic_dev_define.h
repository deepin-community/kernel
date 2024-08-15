/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_DEV_DEFINE_H
#define SSS_NIC_DEV_DEFINE_H

#include <linux/types.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/semaphore.h>
#include <linux/list.h>
#include <linux/bitops.h>

#include "sss_kernel.h"
#include "sss_hw_uld_driver.h"
#include "sss_hw_svc_cap.h"
#include "sss_hw_irq.h"
#include "sss_nic_common.h"
#include "sss_nic_cfg_define.h"
#include "sss_nic_dcb_define.h"
#include "sss_nic_tx_define.h"
#include "sss_nic_rx_define.h"
#include "sss_nic_irq_define.h"
#include "sss_nic_tcam_define.h"

enum sss_nic_flags {
	SSSNIC_INTF_UP,
	SSSNIC_MAC_FILTER_CHANGED,
	SSSNIC_LP_TEST,
	SSSNIC_RSS_ENABLE,
	SSSNIC_DCB_ENABLE,
	SSSNIC_SAME_RXTX,
	SSSNIC_INTR_ADAPT,
	SSSNIC_UPDATE_MAC_FILTER,
	SSSNIC_CHANGE_RES_INVALID,
	SSSNIC_RSS_DEFAULT_INDIR,
	SSSNIC_FORCE_LINK_UP,
	SSSNIC_BONDING_MASTER,
	SSSNIC_AUTONEG_RESET,
	SSSNIC_RXQ_RECOVERY,
};

enum sss_nic_event_flags {
	SSSNIC_EVENT_TX_TIMEOUT,
};

struct sss_nic_tx_stats {
	u64	tx_timeout;

	/* Subdivision statistics show in private tool */
	u64	tx_drop;
	u64	tx_invalid_qid;
	u64	rsvd1;
	u64	rsvd2;

#ifdef HAVE_NDO_GET_STATS64
	struct u64_stats_sync	stats_sync;
#else
	struct u64_stats_sync_empty stats_sync;
#endif
};

struct sss_nic_qp_resource {
	u16	qp_num;
	u8	cos_num;
	u8	rsvd1;
	u32	sq_depth;
	u32	rq_depth;

	struct sss_nic_sq_resource	*sq_res_group;
	struct sss_nic_rq_resource	*rq_res_group;
	struct sss_nic_irq_cfg		*irq_cfg;
};

struct sss_nic_rx_rule {
	struct list_head rule_list;
	int rule_cnt;
};

struct sss_nic_dev {
	struct pci_dev		*pdev;
	struct net_device	*netdev;
	struct sss_hal_dev	*uld_dev;
	void				*hwdev;
	void				*dev_hdl;
	struct sss_nic_io	*nic_io;

	int					poll_budget;

	u32					msg_enable;

	unsigned long		flags;
	unsigned long		event_flag;
	unsigned long		dcb_flags;
	unsigned long		rx_mode;

	u32			rx_poll_wqe;

	u32			rx_dma_buff_size;
	u16			rx_buff_len;

	u16			max_qp_num;

	u32			page_order;

	/* Rss related varibles */
	u8			rss_hash_engine;
	u8			rsvd1[3];
	u8			*rss_key;
	u32			*rss_key_big; /* hkey in big endian */
	u32			*rss_indir_tbl;
	struct sss_nic_rss_type	rss_type;

	u8			max_cos_num;
	u8			dft_func_cos_bitmap;
	u16			dft_port_cos_bitmap;

	int			disable_port_cnt;

	unsigned long		last_jiffies;

	u32					use_adaptive_rx_coalesce;
	u32					rsvd2;

	struct sss_nic_intr_coal_info	*coal_info;
	struct workqueue_struct		*workq;

	int		netdev_uc_cnt;
	int		netdev_mc_cnt;

	int		loop_test_rx_cnt;
	int		loop_pkt_len;
	u8		*loop_test_rx_buf;

	struct sss_irq_desc		*irq_desc_group;
	u16		irq_desc_num;

	u8		link_status;

	u8		rsvd3;

	u32		get_rq_fail_cnt;

	struct sss_nic_tx_stats	tx_stats;

	struct sss_nic_sq_desc	*sq_desc_group;
	struct sss_nic_rq_desc	*rq_desc_group;

	struct sss_nic_qp_resource qp_res;

	struct delayed_work	routine_work;
	struct delayed_work	rq_watchdog_work;

	struct list_head	uc_filter_list;
	struct list_head	mc_filter_list;

	unsigned long		*vlan_bitmap;
#ifdef HAVE_XDP_SUPPORT
	struct bpf_prog		*xdp_prog;
#endif

	/* lock for qp_res,qp_info access */
	struct mutex		qp_mutex;
	struct semaphore	port_sem;

	struct work_struct	rx_mode_work;

	struct delayed_work	moderation_task;

	struct sss_nic_dcb_config hw_dcb_cfg;
	struct sss_nic_dcb_config backup_dcb_cfg;

#ifndef HAVE_NETDEV_STATS_IN_NETDEV
	struct net_device_stats net_stats;
#endif

	struct sss_nic_tcam_info	tcam_info;
	struct sss_nic_rx_rule		rx_rule;

	struct sss_nic_service_cap	nic_svc_cap;

};

#define SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, flag)				\
			test_bit(flag, &(nic_dev)->flags)
#define SSSNIC_SET_NIC_DEV_FLAG(nic_dev, flag)				\
			set_bit(flag, &(nic_dev)->flags)
#define SSSNIC_CLEAR_NIC_DEV_FLAG(nic_dev, flag)			\
			clear_bit(flag, &(nic_dev)->flags)
#define SSSNIC_TEST_CLEAR_NIC_DEV_FLAG(nic_dev, flag)		\
			test_and_clear_bit(flag, &(nic_dev)->flags)
#define SSSNIC_TEST_SET_NIC_DEV_FLAG(nic_dev, flag)			\
			test_and_set_bit(flag, &(nic_dev)->flags)

#ifdef HAVE_XDP_SUPPORT
#define SSSNIC_IS_XDP_ENABLE(nic_dev) (!!(nic_dev)->xdp_prog)
#endif

#define SSS_CHANNEL_RES_VALID(nic_dev)	\
		(test_bit(SSSNIC_INTF_UP, &(nic_dev)->flags) && \
		 !test_bit(SSSNIC_CHANGE_RES_INVALID, &(nic_dev)->flags))

#define SSSNIC_VLAN_BITMAP_BYTE_SIZE(nic_dev)	(sizeof(*(nic_dev)->vlan_bitmap))
#define SSSNIC_VLAN_BITMAP_BIT_SIZE(nic_dev)	(SSSNIC_VLAN_BITMAP_BYTE_SIZE(nic_dev) * 8)
#define SSSNIC_VLAN_NUM_BITMAP(nic_dev)	(VLAN_N_VID / \
					SSSNIC_VLAN_BITMAP_BIT_SIZE(nic_dev))
#define SSSNIC_VLAN_BITMAP_SIZE(nic_dev)	(VLAN_N_VID / \
					SSSNIC_VLAN_BITMAP_BYTE_SIZE(nic_dev))
#define SSSNIC_VID_LINE(nic_dev, vid)	((vid) / SSSNIC_VLAN_BITMAP_BIT_SIZE(nic_dev))
#define SSSNIC_VID_COL(nic_dev, vid)	((vid) & (SSSNIC_VLAN_BITMAP_BIT_SIZE(nic_dev) - 1))
#define SSSNIC_TEST_VLAN_BIT(nic_dev, vid) \
		((nic_dev)->vlan_bitmap[SSSNIC_VID_LINE(nic_dev, vid)] & \
		(1UL << SSSNIC_VID_COL(nic_dev, vid)))

#define SSSNIC_SET_VLAN_BITMAP(nic_dev, vid)                                   \
	set_bit(SSSNIC_VID_COL(nic_dev, vid),                                         \
		&(nic_dev)->vlan_bitmap[SSSNIC_VID_LINE(nic_dev, vid)])
#define SSSNIC_CLEAR_VLAN_BITMAP(nic_dev, vid)                                   \
	clear_bit(SSSNIC_VID_COL(nic_dev, vid),                                         \
		&(nic_dev)->vlan_bitmap[SSSNIC_VID_LINE(nic_dev, vid)])

#define SSSNIC_SET_NIC_EVENT_FLAG(nic_dev, flag)	\
	set_bit(flag, &(nic_dev)->event_flag)

#define SSSNIC_TEST_CLEAR_NIC_EVENT_FLAG(nic_dev, flag)                               \
	test_and_clear_bit(flag, &(nic_dev)->event_flag)

#define SSSNIC_STATS_TX_TIMEOUT_INC(nic_dev)			\
do {								\
	typeof(nic_dev) (_nic_dev) = (nic_dev);			\
	u64_stats_update_begin(&(_nic_dev)->tx_stats.stats_sync);	\
	(_nic_dev)->tx_stats.tx_timeout++;			\
	u64_stats_update_end(&(_nic_dev)->tx_stats.stats_sync);	\
} while (0)

#define SSSNIC_STATS_TX_DROP_INC(nic_dev)			\
do {								\
	typeof(nic_dev) (_nic_dev) = (nic_dev);			\
	u64_stats_update_begin(&(_nic_dev)->tx_stats.stats_sync);	\
	(_nic_dev)->tx_stats.tx_drop++;				\
	u64_stats_update_end(&(_nic_dev)->tx_stats.stats_sync);	\
} while (0)

#define SSSNIC_STATS_TX_INVALID_QID_INC(nic_dev)		\
do {								\
	typeof(nic_dev) (_nic_dev) = (nic_dev);			\
	u64_stats_update_begin(&(_nic_dev)->tx_stats.stats_sync);	\
	(_nic_dev)->tx_stats.tx_invalid_qid++;			\
	u64_stats_update_end(&(_nic_dev)->tx_stats.stats_sync);	\
} while (0)

#define sssnic_msg(level, nic_dev, msglvl, format, arg...)	\
do {								\
	if ((nic_dev)->netdev && (nic_dev)->netdev->reg_state	\
	    == NETREG_REGISTERED)				\
		nicif_##level((nic_dev), msglvl, (nic_dev)->netdev,	\
			      format, ## arg);			\
	else							\
		nic_##level(&(nic_dev)->pdev->dev,		\
			    format, ## arg);			\
} while (0)

#define sss_nic_info(nic_dev, msglvl, format, arg...)	\
	sssnic_msg(info, nic_dev, msglvl, format, ## arg)

#define sss_nic_warn(nic_dev, msglvl, format, arg...)	\
	sssnic_msg(warn, nic_dev, msglvl, format, ## arg)

#define sss_nic_err(nic_dev, msglvl, format, arg...)	\
	sssnic_msg(err, nic_dev, msglvl, format, ## arg)

#endif

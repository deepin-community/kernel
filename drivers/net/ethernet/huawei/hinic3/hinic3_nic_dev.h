/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_NIC_DEV_H
#define	HINIC3_NIC_DEV_H

#include <linux/netdevice.h>
#include <linux/semaphore.h>
#include <linux/types.h>
#include <linux/bitops.h>

#include "ossl_knl.h"
#include "hinic3_nic_io.h"
#include "hinic3_nic_cfg.h"
#include "hinic3_tx.h"
#include "hinic3_rx.h"
#include "hinic3_dcb.h"
#include "vram_common.h"

#define HINIC3_NIC_DRV_NAME	"hinic3"
#define HINIC3_NIC_DRV_VERSION	"17.7.8.1"

#define HINIC3_FUNC_IS_VF(hwdev)	(hinic3_func_type(hwdev) == TYPE_VF)

#define HINIC3_AVG_PKT_SMALL      256U
#define HINIC3_MODERATONE_DELAY   HZ

#define LP_PKT_CNT 64
#define LP_PKT_LEN 60

#define NAPI_IS_REGIN 1
#define NAPI_NOT_REGIN 0

enum hinic3_flags {
	HINIC3_INTF_UP,
	HINIC3_MAC_FILTER_CHANGED,
	HINIC3_LP_TEST,
	HINIC3_RSS_ENABLE,
	HINIC3_DCB_ENABLE,
	HINIC3_SAME_RXTX,
	HINIC3_INTR_ADAPT,
	HINIC3_UPDATE_MAC_FILTER,
	HINIC3_CHANGE_RES_INVALID,
	HINIC3_RSS_DEFAULT_INDIR,
	HINIC3_FORCE_LINK_UP,
	HINIC3_BONDING_MASTER,
	HINIC3_AUTONEG_RESET,
	HINIC3_RXQ_RECOVERY,
};

#define HINIC3_CHANNEL_RES_VALID(nic_dev)	\
		(test_bit(HINIC3_INTF_UP, &(nic_dev)->flags) && \
		 !test_bit(HINIC3_CHANGE_RES_INVALID, &(nic_dev)->flags))

#define RX_BUFF_NUM_PER_PAGE	2

#define VLAN_BITMAP_BYTE_SIZE(nic_dev)	(sizeof(*(nic_dev)->vlan_bitmap))
#define VLAN_BITMAP_BITS_SIZE(nic_dev)	(VLAN_BITMAP_BYTE_SIZE(nic_dev) * 8)
#define VLAN_NUM_BITMAPS(nic_dev)	(VLAN_N_VID / \
					VLAN_BITMAP_BITS_SIZE(nic_dev))
#define VLAN_BITMAP_SIZE(nic_dev)	(VLAN_N_VID / \
					VLAN_BITMAP_BYTE_SIZE(nic_dev))
#define VID_LINE(nic_dev, vid)	((vid) / VLAN_BITMAP_BITS_SIZE(nic_dev))
#define VID_COL(nic_dev, vid)	((vid) & (VLAN_BITMAP_BITS_SIZE(nic_dev) - 1))

#define NIC_DRV_DEFAULT_FEATURE		NIC_F_ALL_MASK

enum hinic3_event_work_flags {
	EVENT_WORK_TX_TIMEOUT,
};

enum hinic3_rx_mode_state {
	HINIC3_HW_PROMISC_ON,
	HINIC3_HW_ALLMULTI_ON,
	HINIC3_PROMISC_FORCE_ON,
	HINIC3_ALLMULTI_FORCE_ON,
};

enum mac_filter_state {
	HINIC3_MAC_WAIT_HW_SYNC,
	HINIC3_MAC_HW_SYNCED,
	HINIC3_MAC_WAIT_HW_UNSYNC,
	HINIC3_MAC_HW_UNSYNCED,
};

struct hinic3_mac_filter {
	struct list_head list;
	u8 addr[ETH_ALEN];
	unsigned long state;
};

struct hinic3_irq {
	struct net_device	*netdev;
	/* IRQ corresponding index number */
	u16			msix_entry_idx;
	u16			rsvd1;
	u32			irq_id;         /* The IRQ number from OS */

	u32			napi_reign;

	char			irq_name[IFNAMSIZ + 16];
	struct napi_struct	napi;
	cpumask_t		affinity_mask;
	struct hinic3_txq	*txq;
	struct hinic3_rxq	*rxq;
};

struct hinic3_intr_coal_info {
	u8	pending_limt;
	u8	coalesce_timer_cfg;
	u8	resend_timer_cfg;

	u64	pkt_rate_low;
	u8	rx_usecs_low;
	u8	rx_pending_limt_low;
	u64	pkt_rate_high;
	u8	rx_usecs_high;
	u8	rx_pending_limt_high;

	u8	user_set_intr_coal_flag;
};

struct hinic3_dyna_txrxq_params {
	u16	num_qps;
	u8	num_cos;
	u8	rsvd1;
	u32	sq_depth;
	u32	rq_depth;

	struct hinic3_dyna_txq_res	*txqs_res;
	struct hinic3_dyna_rxq_res	*rxqs_res;
	struct hinic3_irq		*irq_cfg;
	char irq_cfg_vram_name[VRAM_NAME_MAX_LEN];
};

struct hinic3_flush_rq {
	union {
		struct {
#if defined(BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)
			u32 lb_proc : 1;
			u32 rsvd : 10;
			u32 rq_id : 8;
			u32 func_id : 13;
#else
			u32 func_id : 13;
			u32 rq_id : 8;
			u32 rsvd : 10;
			u32 lb_proc : 1;
#endif
		} bs;
		u32 value;
	} dw;

	union {
		struct {
#if defined(BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)
			u32 rsvd2 : 2;
			u32 src_chnl : 12;
			u32 pkt_len : 18;
#else
			u32 pkt_len : 18;
			u32 src_chnl : 12;
			u32 rsvd2 : 2;
#endif
		} bs;
		u32 value;
	} lb_info0; /* loop back information, used by uCode */
};

#define HINIC3_NIC_STATS_INC(nic_dev, field)			\
do {								\
	u64_stats_update_begin(&(nic_dev)->stats.syncp);	\
	(nic_dev)->stats.field++;				\
	u64_stats_update_end(&(nic_dev)->stats.syncp);		\
} while (0)

struct hinic3_nic_stats {
	u64	netdev_tx_timeout;

	/* Subdivision statistics show in private tool */
	u64	tx_carrier_off_drop;
	u64	tx_invalid_qid;
	u64	rsvd1;
	u64	rsvd2;
#ifdef HAVE_NDO_GET_STATS64
	struct u64_stats_sync	syncp;
#else
	struct u64_stats_sync_empty syncp;
#endif
};

struct hinic3_nic_vport_stats {
	u64 rx_discard_vport;
};

#define HINIC3_TCAM_DYNAMIC_BLOCK_SIZE 16
#define HINIC3_MAX_TCAM_FILTERS	512

#define HINIC3_PKT_TCAM_DYNAMIC_INDEX_START(block_index)  \
		(HINIC3_TCAM_DYNAMIC_BLOCK_SIZE * (block_index))

struct hinic3_rx_flow_rule {
	struct list_head rules;
	int tot_num_rules;
};

struct hinic3_tcam_dynamic_block {
	struct list_head block_list;
	u16 dynamic_block_id;
	u16 dynamic_index_cnt;
	u8 dynamic_index_used[HINIC3_TCAM_DYNAMIC_BLOCK_SIZE];
};

struct hinic3_tcam_dynamic_block_info {
	struct list_head tcam_dynamic_list;
	u16 dynamic_block_cnt;
};

struct hinic3_tcam_filter {
	struct list_head tcam_filter_list;
	u16 dynamic_block_id;
	u16 index;
	struct tag_tcam_key tcam_key;
	u16 queue;
};

/* function level struct info */
struct hinic3_tcam_info {
	u16 tcam_rule_nums;
	struct list_head tcam_list;
	struct hinic3_tcam_dynamic_block_info tcam_dynamic_info;
};

struct hinic3_dcb {
	u8			 cos_config_num_max;
	u8			 func_dft_cos_bitmap;
	/* used to tool validity check */
	u16			 port_dft_cos_bitmap;

	struct hinic3_dcb_config hw_dcb_cfg;
	struct hinic3_dcb_config wanted_dcb_cfg;
	unsigned long		 dcb_flags;
};

struct hinic3_vram {
	u32 vram_mtu;
	u16 vram_num_qps;
	unsigned long flags;
};

struct hinic3_outband_cfg {
	u16 outband_default_vid;
	u16 rsvd;
};

struct hinic3_nic_dev {
	struct pci_dev		*pdev;
	struct net_device	*netdev;
	struct hinic3_lld_dev	*lld_dev;
	void			*hwdev;

	int			poll_weight;
	u32			rsvd1;
	unsigned long		*vlan_bitmap;

	u16			max_qps;

	u32			msg_enable;
	unsigned long		flags;

	u32			lro_replenish_thld;
	u32			dma_rx_buff_size;
	u16			rx_buff_len;
	u32			page_order;
	bool			page_pool_enabled;

	/* Rss related varibles */
	u8			rss_hash_engine;
	struct nic_rss_type	rss_type;
	u8			*rss_hkey;
	/* hkey in big endian */
	u32			*rss_hkey_be;
	u32			*rss_indir;

	struct hinic3_dcb	*dcb;
	char			dcb_name[VRAM_NAME_MAX_LEN];

	struct hinic3_vram	*nic_vram;
	char			nic_vram_name[VRAM_NAME_MAX_LEN];

	int			disable_port_cnt;

	struct hinic3_intr_coal_info *intr_coalesce;
	unsigned long		last_moder_jiffies;
	u32			adaptive_rx_coal;
	u8			intr_coal_set_flag;

#ifndef HAVE_NETDEV_STATS_IN_NETDEV
	struct net_device_stats net_stats;
#endif

	struct hinic3_nic_stats	stats;
	struct hinic3_nic_vport_stats	vport_stats;

	/* lock for nic resource */
	struct mutex		nic_mutex;
	u8			link_status;

	struct nic_service_cap	nic_cap;

	struct hinic3_txq	*txqs;
	struct hinic3_rxq	*rxqs;
	struct hinic3_dyna_txrxq_params q_params;
	u8 cqe_mode; /* rx_cqe */

	u16			num_qp_irq;
	struct irq_info		*qps_irq_info;

	struct workqueue_struct *workq;

	struct work_struct	rx_mode_work;
	struct delayed_work	moderation_task;

	struct list_head	uc_filter_list;
	struct list_head	mc_filter_list;
	unsigned long		rx_mod_state;
	int			netdev_uc_cnt;
	int			netdev_mc_cnt;

	int			lb_test_rx_idx;
	int			lb_pkt_len;
	u8			*lb_test_rx_buf;

	struct hinic3_tcam_info tcam;
	struct hinic3_rx_flow_rule rx_flow_rule;

#ifdef HAVE_XDP_SUPPORT
	struct bpf_prog		*xdp_prog;
#endif

	struct delayed_work	periodic_work;
	/* reference to enum hinic3_event_work_flags */
	unsigned long		event_flag;

	struct hinic3_nic_prof_attr *prof_attr;
	struct hinic3_prof_adapter *prof_adap;
	u64			rsvd8[7];
	struct hinic3_outband_cfg	outband_cfg;
	u32			rxq_get_err_times;
	struct delayed_work	rxq_check_work;
	struct delayed_work		vport_stats_work;

	struct hinic3_tx_rx_ops	tx_rx_ops;
};

#define hinic_msg(level, nic_dev, msglvl, format, arg...)	\
do {								\
	if ((nic_dev)->netdev && (nic_dev)->netdev->reg_state	\
	    == NETREG_REGISTERED)				\
		nicif_##level((nic_dev), msglvl, (nic_dev)->netdev,	\
			      format, ## arg);			\
	else							\
		nic_##level(&(nic_dev)->pdev->dev,		\
			    format, ## arg);			\
} while (0)

#define hinic3_info(nic_dev, msglvl, format, arg...)	\
	hinic_msg(info, nic_dev, msglvl, format, ## arg)

#define hinic3_warn(nic_dev, msglvl, format, arg...)	\
	hinic_msg(warn, nic_dev, msglvl, format, ## arg)

#define hinic3_err(nic_dev, msglvl, format, arg...)	\
	hinic_msg(err, nic_dev, msglvl, format, ## arg)

struct hinic3_uld_info *hinic3_get_nic_uld_info(void);

u32 hinic3_get_io_stats_size(const struct hinic3_nic_dev *nic_dev);

int hinic3_get_io_stats(const struct hinic3_nic_dev *nic_dev, void *stats);

int hinic3_open(struct net_device *netdev);

int hinic3_close(struct net_device *netdev);

void hinic3_set_ethtool_ops(struct net_device *netdev);

void hinic3vf_set_ethtool_ops(struct net_device *netdev);

int hinic3_ioctl(void *uld_dev, u32 cmd, const void *buf_in,
	      u32 in_size, void *buf_out, u32 *out_size);

void hinic3_update_num_qps(struct net_device *netdev);

int hinic3_qps_irq_init(struct hinic3_nic_dev *nic_dev);

void hinic3_qps_irq_deinit(struct hinic3_nic_dev *nic_dev);

void qp_del_napi(struct hinic3_irq *irq_cfg);

void hinic3_set_netdev_ops(struct hinic3_nic_dev *nic_dev);

bool hinic3_is_netdev_ops_match(const struct net_device *netdev);

int hinic3_set_hw_features(struct hinic3_nic_dev *nic_dev);

void hinic3_set_rx_mode_work(struct work_struct *work);

void hinic3_clean_mac_list_filter(struct hinic3_nic_dev *nic_dev);

void hinic3_get_strings(struct net_device *netdev, u32 stringset, u8 *data);

void hinic3_get_ethtool_stats(struct net_device *netdev,
			      struct ethtool_stats *stats, u64 *data);

int hinic3_get_sset_count(struct net_device *netdev, int sset);

int hinic3_maybe_set_port_state(struct hinic3_nic_dev *nic_dev, bool enable);

#ifdef ETHTOOL_GLINKSETTINGS
#ifndef XENSERVER_HAVE_NEW_ETHTOOL_OPS
int hinic3_get_link_ksettings(struct net_device *netdev,
			      struct ethtool_link_ksettings *link_settings);
int hinic3_set_link_ksettings(struct net_device *netdev,
			      const struct ethtool_link_ksettings
			      *link_settings);
#endif
#endif

#ifndef HAVE_NEW_ETHTOOL_LINK_SETTINGS_ONLY
int hinic3_get_settings(struct net_device *netdev, struct ethtool_cmd *ep);
int hinic3_set_settings(struct net_device *netdev,
			struct ethtool_cmd *link_settings);
#endif

void hinic3_auto_moderation_work(struct work_struct *work);

typedef void (*hinic3_reopen_handler)(struct hinic3_nic_dev *nic_dev,
				      const void *priv_data);
int hinic3_change_channel_settings(struct hinic3_nic_dev *nic_dev,
				   struct hinic3_dyna_txrxq_params *trxq_params,
				   hinic3_reopen_handler reopen_handler,
				   const void *priv_data);

void hinic3_link_status_change(struct hinic3_nic_dev *nic_dev, bool status);

#ifdef HAVE_XDP_SUPPORT
bool hinic3_is_xdp_enable(struct hinic3_nic_dev *nic_dev);
int hinic3_xdp_max_mtu(struct hinic3_nic_dev *nic_dev);
#endif

#ifdef HAVE_UDP_TUNNEL_NIC_INFO
int hinic3_udp_tunnel_set_port(struct net_device *netdev, unsigned int table,
			       unsigned int entry, struct udp_tunnel_info *ti);
int hinic3_udp_tunnel_unset_port(struct net_device *netdev, unsigned int table,
				 unsigned int entry,
				 struct udp_tunnel_info *ti);
#endif /* HAVE_UDP_TUNNEL_NIC_INFO */

#if defined(ETHTOOL_GFECPARAM) && defined(ETHTOOL_SFECPARAM)
int set_fecparam(void *hwdev, u8 fecparam);
int get_fecparam(void *hwdev, u8 *advertised_fec, u8 *supported_fec);
#endif

#endif


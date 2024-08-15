/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#ifndef _NE6X_ARFS_H
#define _NE6X_ARFS_H

/* protocol enumeration for filters */
enum ne6x_fltr_ptype {
	/* NONE - used for undef/error */
	NE6X_FLTR_PTYPE_NONF_NONE = 0,
	NE6X_FLTR_PTYPE_NONF_IPV4_UDP,
	NE6X_FLTR_PTYPE_NONF_IPV4_TCP,
	NE6X_FLTR_PTYPE_NONF_IPV6_UDP,
	NE6X_FLTR_PTYPE_NONF_IPV6_TCP,
	NE6X_FLTR_PTYPE_MAX,
};

struct ne6x_fster_v4 {
	__be32 rsv0[3];
	__be32 dst_ip;
	__be32 rsv1[3];
	__be32 src_ip;
	__be16 dst_port;
	__be16 src_port;
	__be16 rsv2;
	u8 pi;
	u8 proto;
	u8 rsv3[24];
};

#define NE6X_IPV6_ADDR_LEN_AS_U32	4

struct ne6x_fster_v6 {
	__be32 dst_ip[NE6X_IPV6_ADDR_LEN_AS_U32];
	__be32 src_ip[NE6X_IPV6_ADDR_LEN_AS_U32];
	__be16 dst_port;
	__be16 src_port;
	__be16 rsv0;
	u8 pi;
	u8 proto;
	u8 rsv1[24];
};

struct ne6x_fster_data {
	u8 tab_id;
	u8 port;
	__be16 cos;
	__be32 hash;
	u8 rsv0[24];
};

struct ne6x_fster_table {
	union {
		struct ne6x_fster_v4 v4;
		struct ne6x_fster_v6 v6;
	} ip;
	struct ne6x_fster_data data;
};

struct ne6x_fster_search_result {
	u32 key_index;
	struct ne6x_fster_data data;
};

struct ne6x_fster_fltr {
	struct list_head fltr_node;
	enum ne6x_fltr_ptype flow_type;

	union {
		struct ne6x_fster_v4 v4;
		struct ne6x_fster_v6 v6;
	} ip;
	struct ne6x_fster_data data;

	/* filter control */
	u16 q_index;
	u16 dest_adpt;
	u8 cnt_ena;
	u16 cnt_index;
	u32 fltr_id;
};

enum ne6x_arfs_fltr_state {
	NE6X_ARFS_INACTIVE,
	NE6X_ARFS_ACTIVE,
	NE6X_ARFS_TODEL,
};

struct ne6x_arfs_entry {
	struct ne6x_fster_fltr fltr_info;
	struct ne6x_arfs_active_fltr_cntrs *arfs_fltr_cntrs;
	struct hlist_node list_entry;
	u64 time_activated;	/* only valid for UDP flows */
	u32 flow_id;
	/* fltr_state = 0 - NE6X_ARFS_INACTIVE:
	 *	filter needs to be updated or programmed in HW.
	 * fltr_state = 1 - NE6X_ARFS_ACTIVE:
	 *	filter is active and programmed in HW.
	 * fltr_state = 2 - NE6X_ARFS_TODEL:
	 *	filter has been deleted from HW and needs to be removed from
	 *	the aRFS hash table.
	 */
	u8 fltr_state;
};

struct ne6x_arfs_entry_ptr {
	struct ne6x_arfs_entry *arfs_entry;
	struct hlist_node list_entry;
};

struct ne6x_arfs_active_fltr_cntrs {
	atomic_t active_tcpv4_cnt;
	atomic_t active_tcpv6_cnt;
	atomic_t active_udpv4_cnt;
	atomic_t active_udpv6_cnt;
};

#ifdef CONFIG_RFS_ACCEL
int
ne6x_rx_flow_steer(struct net_device *netdev, const struct sk_buff *skb,
		   u16 rxq_idx, u32 flow_id);
void ne6x_clear_arfs(struct ne6x_adapter *adpt);
void ne6x_free_cpu_rx_rmap(struct ne6x_adapter *adpt);
void ne6x_init_arfs(struct ne6x_adapter *adpt);
void ne6x_sync_arfs_fltrs(struct ne6x_pf *pf);
int ne6x_set_cpu_rx_rmap(struct ne6x_adapter *adpt);
void ne6x_remove_arfs(struct ne6x_adapter *adpt);
#else
static inline void ne6x_clear_arfs(struct ne6x_adapter *adpt) { }
static inline void ne6x_free_cpu_rx_rmap(struct ne6x_adapter *adpt) { }
static inline void ne6x_init_arfs(struct ne6x_adapter *adpt) { }
static inline void ne6x_sync_arfs_fltrs(struct ne6x_pf *pf) { }
static inline void ne6x_remove_arfs(struct ne6x_adapter *adpt) { }

static inline int ne6x_set_cpu_rx_rmap(struct ne6x_adapter __always_unused *adpt)
{
	return 0;
}

static inline int
ne6x_rx_flow_steer(struct net_device __always_unused *netdev,
		   const struct sk_buff __always_unused *skb,
		   u16 __always_unused rxq_idx, u32 __always_unused flow_id)
{
	return -EOPNOTSUPP;
}
#endif /* CONFIG_RFS_ACCEL */

#endif

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#ifndef _NE6X_VIRTCHNL_PF_H
#define _NE6X_VIRTCHNL_PF_H

#include "mailbox.h"

#define NE6X_NO_ADPT 0xffff

enum virtchnl_event_codes {
	VIRTCHNL_EVENT_UNKNOWN = 0,
	VIRTCHNL_EVENT_LINK_CHANGE,
	VIRTCHNL_EVENT_RESET_IMPENDING,
	VIRTCHNL_EVENT_PF_DRIVER_CLOSE,
	VIRTCHNL_EVENT_DCF_ADPT_MAP_UPDATE,
};

struct virtchnl_pf_event {
	u8 event;
	u32 link_speed;
	u8 link_status;
};

union u_ne6x_mbx_snap_buffer_data {
	struct ne6x_mbx_snap_buffer_data snap;
	u64 val;
};

/* Specific VF states */
enum ne6x_vf_states {
	NE6X_VF_STATE_INIT = 0, /* PF is initializing VF */
	NE6X_VF_STATE_ACTIVE,   /* VF resources are allocated for use */
	NE6X_VF_STATE_QS_ENA,   /* VF queue(s) enabled */
	NE6X_VF_STATE_DIS,
	NE6X_VF_STATE_MC_PROMISC,
	NE6X_VF_STATE_UC_PROMISC,
	NE6X_VF_STATES_NBITS
};

struct virtchnl_ether_addr {
	u8 addr[ETH_ALEN];
};

struct virtchnl_promisc_info {
	u16 adpt_id;
	u16 flags;
};

#define FLAG_VF_UNICAST_PROMISC   0x00000001
#define FLAG_VF_MULTICAST_PROMISC 0x00000002

enum ne6x_promisc_flags {
	NE6X_PROMISC_UCAST_RX = 0x1,
	NE6X_PROMISC_UCAST_TX = 0x2,
	NE6X_PROMISC_MCAST_RX = 0x4,
	NE6X_PROMISC_MCAST_TX = 0x8,
	NE6X_PROMISC_BCAST_RX = 0x10,
	NE6X_PROMISC_BCAST_TX = 0x20,
	NE6X_PROMISC_VLAN_RX = 0x40,
	NE6X_PROMISC_VLAN_TX = 0x80,
};

#define NE6X_UCAST_PROMISC_BITS (NE6X_PROMISC_UCAST_TX | NE6X_PROMISC_UCAST_RX)
#define NE6X_MCAST_PROMISC_BITS (NE6X_PROMISC_MCAST_TX | NE6X_PROMISC_MCAST_RX)

enum ne6x_vf_config_flag {
	NE6X_VF_CONFIG_FLAG_TRUSTED = 0,
	NE6X_VF_CONFIG_FLAG_LINK_FORCED,
	NE6X_VF_CONFIG_FLAG_NBITS /* must be last */
};

struct ne6x_key {
	u8 rsv0;
	u8 pi;
	u8 mac_addr[6];
	u8 rsv1[56];
};

/* VF information structure */
struct ne6x_vf {
	struct ne6x_pf *pf;
	struct ne6x_adapter *adpt;

	u16 vf_id;	 /* VF ID in the PF space */
	u16 lan_adpt_idx; /* index into PF struct */
			 /* first vector index of this VF in the PF space */
	u16 vfp_vid;
	u16 vfp_tpid;
	int tx_rate;
	u8 rx_tx_state;
	bool ready;
	bool ready_to_link_notify;

	u16 base_queue;
	u16 num_vf_qs;
	u16 num_req_qs;

	struct ne6x_vlan port_vlan_info; /* Port VLAN ID, QoS, and TPID */

	u8 trusted     : 1;
	u8 link_forced : 1;
	u8 link_up     : 1; /* only valid if VF link is forced */

	struct virtchnl_ether_addr dev_lan_addr;
	DECLARE_BITMAP(vf_states, NE6X_VF_STATES_NBITS); /* VF runtime states */
	DECLARE_BITMAP(opcodes_allowlist, VIRTCHNL_OP_MAX);
	DECLARE_BITMAP(vf_config_flag, NE6X_VF_CONFIG_FLAG_NBITS);
};

#define ne6x_for_each_vf(pf, i) for ((i) = 0; (i) < (pf)->num_alloc_vfs; (i)++)
#define ne6x_for_each_pf(pf, i) for ((i) = 0; (i) < (pf)->num_alloc_adpt; (i)++)

#ifdef CONFIG_PCI_IOV
int ne6x_set_vf_trust(struct net_device *netdev, int vf_id, bool trusted);
int ne6x_set_vf_link_state(struct net_device *netdev, int vf_id, int link_state);

int ne6x_sriov_configure(struct pci_dev *pdev, int num_vfs);
void ne6x_vc_process_vf_msg(struct ne6x_pf *pf);
void ne6x_vc_notify_link_state(struct ne6x_vf *vf);
int ne6x_set_vf_mac(struct net_device *netdev, int vf_id, u8 *mac);
void ne6x_clr_vf_bw_for_max_vpnum(struct ne6x_pf *pf);

struct ne6x_adapter *ne6x_get_vf_adpt(struct ne6x_vf *vf);
int ne6x_ndo_set_vf_bw(struct net_device *netdev, int vf_id, int min_tx_rate, int max_tx_rate);
int ne6x_get_vf_config(struct net_device *netdev, int vf_id, struct ifla_vf_info *ivi);

#else /* CONFIG_PCI_IOV */
static inline int ne6x_sriov_configure(struct pci_dev __always_unused *pdev,
				       int __always_unused num_vfs)
{
	return -EOPNOTSUPP;
}

static inline int ne6x_set_vf_trust(struct net_device *netdev, int vf_id, bool trusted)
{
	return -EOPNOTSUPP;
}

static inline int ne6x_set_vf_link_state(struct net_device *netdev, int vf_id, int link_state)
{
	return -EOPNOTSUPP;
}

static inline int ne6x_set_vf_mac(struct net_device *netdev, int vf_id, u8 *mac)
{
	return -EOPNOTSUPP;
}

static inline int ne6x_ndo_set_vf_bw(struct net_device *netdev, int vf_id,
				     int min_tx_rate, int max_tx_rate)
{
	return -EOPNOTSUPP;
}

static inline int ne6x_get_vf_config(struct net_device *netdev, int vf_id, struct ifla_vf_info *ivi)
{
	return -EOPNOTSUPP;
}

#endif /* CONFIG_PCI_IOV */

#endif

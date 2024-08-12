/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#ifndef _NE6XVF_VIRTCHNL_H
#define _NE6XVF_VIRTCHNL_H

#define NE6XVF_SDK_LARGE_BUF                6

struct ne6xvf_spinlock {
	/* mutext lock */
	struct mutex spinlock;
};

struct virtchnl_vsi_resource {
	u16 vsi_id;
	u16 num_queue_pairs;

	/* see enum virtchnl_vsi_type */
	s32 vsi_type;
	u16 qset_handle;
	u8 default_mac_addr[ETH_ALEN];
};

struct virtchnl_ether_addr {
	u8 addr[ETH_ALEN];
	u8 type;
	u8 pad;
};

struct virtchnl_vf_resource {
	u16 num_vsis;
	u16 num_queue_pairs;
	u16 max_vectors;
	u16 max_mtu;

	u32 vf_cap_flags;
	u32 rss_key_size;
	u32 rss_lut_size;

	struct virtchnl_vsi_resource vsi_res[];
};

enum nacf_virtchnl_vsi_type {
	NE6XVF_VIRTCHNL_VSI_TYPE_INVALID = 0,
	NE6XVF_VIRTCHNL_VSI_SRIOV = 6,
};

struct virtchnl_ether_addr_list {
	u16 vsi_id;
	u16 num_elements;
	struct virtchnl_ether_addr list[];
};

struct ne6xvf_arq_event_info {
	struct ne6x_mbx_snap_buffer_data snap;
	u16 msg_len;
	u16 buf_len;
	u8 *msg_buf;
};

/* VF resource request */
struct ne6xvf_virtchnl_vf_res_request {
	u16 num_queue_pairs;
	u8 need_reset;
	u8 rsv;
};

#define FLAG_VF_UNICAST_PROMISC   0x00000001
#define FLAG_VF_MULTICAST_PROMISC 0x00000002

/* VIRTCHNL_OP_CONFIG_PROMISCUOUS_MODE
 * VF sends VSI id and flags.
 * PF returns status code in retval.
 * Note: we assume that broadcast accept mode is always enabled.
 */
struct ne6xvf_virtchnl_promisc_info {
	u16 vsi_id;
	u16 flags;
};

union u_ne6x_mbx_snap_buffer_data {
	struct ne6x_mbx_snap_buffer_data snap;
	u64 val;
};

struct ne6xvf_sdk_mbx_info {
	struct ne6xvf_spinlock mbx_spinlock;
	struct ne6x_mbx_snap_buffer_data sq_data;
	struct ne6x_mbx_snap_buffer_data cq_data;
	int init_flag;
};

#define NE6XVF_VIRTCHNL_VERSION_MAJOR 1
#define NE6XVF_VIRTCHNL_VERSION_MINOR 1

struct ne6xvf_virtchnl_version_info {
	u8 major;
	u8 minor;
};

/* VIRTCHNL_OP_EVENT
 * PF sends this message to inform the VF driver of events that may affect it.
 * No direct response is expected from the VF, though it may generate other
 * messages in response to this one.
 */
enum ne6xvf_virtchnl_event_codes {
	NE6XVF_VIRTCHNL_EVENT_UNKNOWN = 0,
	NE6XVF_VIRTCHNL_EVENT_LINK_CHANGE,
	NE6XVF_VIRTCHNL_EVENT_RESET_IMPENDING,
	NE6XVF_VIRTCHNL_EVENT_PF_DRIVER_CLOSE,
	NE6XVF_VIRTCHNL_EVENT_DCF_VSI_MAP_UPDATE,
};

struct virtchnl_pf_event {
	u8 event;
	u8 link_speed_0;
	u8 link_speed_1;
	u8 link_speed_2;
	u8 link_speed_3;
	u8 link_status;
};

#endif

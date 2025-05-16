/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_NIC_H
#define HINIC3_NIC_H

#include <linux/types.h>
#include <linux/semaphore.h>

#include "hinic3_common.h"
#include "hinic3_nic_io.h"
#include "hinic3_nic_cfg.h"
#include "mag_mpu_cmd.h"
#include "mag_mpu_cmd_defs.h"

/* ************************ array index define ********************* */
#define ARRAY_INDEX_0 0
#define ARRAY_INDEX_1 1
#define ARRAY_INDEX_2 2
#define ARRAY_INDEX_3 3
#define ARRAY_INDEX_4 4
#define ARRAY_INDEX_5 5
#define ARRAY_INDEX_6 6
#define ARRAY_INDEX_7 7

#define XSFP_TLV_PRE_INFO_LEN 4

enum hinic3_link_port_type {
	LINK_PORT_UNKNOWN,
	LINK_PORT_OPTICAL_MM,
	LINK_PORT_OPTICAL_SM,
	LINK_PORT_PAS_COPPER,
	LINK_PORT_ACC,
	LINK_PORT_BASET,
	LINK_PORT_AOC = 0x40,
	LINK_PORT_ELECTRIC,
	LINK_PORT_BACKBOARD_INTERFACE,
};

enum hilink_fibre_subtype {
	FIBRE_SUBTYPE_SR = 1,
	FIBRE_SUBTYPE_LR,
	FIBRE_SUBTYPE_MAX,
};

enum hilink_fec_type {
	HILINK_FEC_NOT_SET,
	HILINK_FEC_RSFEC,
	HILINK_FEC_BASEFEC,
	HILINK_FEC_NOFEC,
	HILINK_FEC_LLRSFE,
	HILINK_FEC_MAX_TYPE,
};

#define	SQ_CI_ADDR_SHIFT 2
#define	RQ_CI_ADDR_SHIFT 4

struct hinic3_sq_attr {
	u8 dma_attr_off;
	u8 pending_limit;
	u8 coalescing_time;
	u8 intr_en;
	u16 intr_idx;
	u32 l2nic_sqn;
	u64 ci_dma_base;
};

struct hinic3_rq_attr {
	u8 cqe_type;
	u8 pending_limit;
	u8 coalescing_time;
	u8 rsv;
	u16 intr_idx;
	u32 l2nic_rqn;
	u64 ci_dma_base;
};

struct vf_data_storage {
	u8 drv_mac_addr[ETH_ALEN];
	u8 user_mac_addr[ETH_ALEN];
	bool registered;
	bool use_specified_mac;
	u16 pf_vlan;
	u8 pf_qos;
	u8 rsvd2;
	u32 max_rate;
	u32 min_rate;

	bool link_forced;
	bool link_up; /* only valid if VF link is forced */
	bool spoofchk;
	bool trust;
	u16 num_qps;
	u32 support_extra_feature;
};

struct hinic3_port_routine_cmd {
	bool mpu_send_sfp_info;
	bool mpu_send_sfp_abs;

	struct mag_cmd_get_xsfp_info std_sfp_info;
	struct mag_cmd_get_xsfp_present abs;
};

struct hinic3_port_routine_cmd_extern {
	bool					mpu_send_xsfp_tlv_info;

	struct drv_tag_mag_cmd_get_xsfp_tlv_rsp	std_xsfp_tlv_info;
};

struct hinic3_nic_cfg {
	struct semaphore	cfg_lock;

	/* Valid when pfc is disable */
	bool			pause_set;
	struct nic_pause_config	nic_pause;

	u8			pfc_en;
	u8			pfc_bitmap;

	struct nic_port_info	port_info;

	/* percentage of pf link bandwidth */
	u32			pf_bw_tx_limit;
	u32			pf_bw_rx_limit;

	struct hinic3_port_routine_cmd		rt_cmd;
	struct hinic3_port_routine_cmd_extern	rt_cmd_ext;
	/* mutex used for copy sfp info */
	struct mutex				sfp_mutex;
};

struct hinic3_nic_cmdq_ops;
struct hinic3_nic_io {
	void				*hwdev;
	void				*pcidev_hdl;
	void				*dev_hdl;

	u8				link_status;
	u8				direct;
	u32				rsvd2;

	struct hinic3_io_queue		*sq;
	struct hinic3_io_queue		*rq;

	u16				num_qps;
	u16				max_qps;

	void				*sq_ci_vaddr_base;
	dma_addr_t			sq_ci_dma_base;

	void				*rq_ci_vaddr_base;
	dma_addr_t			rq_ci_dma_base;

	u8 __iomem			*sqs_db_addr;
	u8 __iomem			*rqs_db_addr;

	u16				max_vfs;
	u16				rsvd3;
	u32				rsvd4;

	struct vf_data_storage		*vf_infos;
	struct hinic3_dcb_state		dcb_state;
	struct hinic3_nic_cfg		nic_cfg;

	u16				rx_buff_len;
	u16				rsvd5;
	u32				rsvd6;
	u64				feature_cap;
	u64				rsvd7;
	struct hinic3_nic_cmdq_ops	*cmdq_ops;
};

struct vf_msg_handler {
	u16 cmd;
	int (*handler)(struct hinic3_nic_io *nic_io, u16 vf,
		       void *buf_in, u16 in_size,
		       void *buf_out, u16 *out_size);
};

struct nic_event_handler {
	u16 cmd;
	void (*handler)(void *hwdev, void *buf_in, u16 in_size,
			void *buf_out, u16 *out_size);
};

int hinic3_set_sq_ci_ctx(struct hinic3_nic_io *nic_io, struct hinic3_sq_attr *attr);

int hinic3_set_rq_ci_ctx(struct hinic3_nic_io *nic_io, struct hinic3_rq_attr *attr);

int l2nic_msg_to_mgmt_sync(void *hwdev, u16 cmd, void *buf_in, u16 in_size,
			   void *buf_out, u16 *out_size);

int l2nic_msg_to_mgmt_sync_ch(void *hwdev, u16 cmd, void *buf_in, u16 in_size,
			      void *buf_out, u16 *out_size, u16 channel);

int hinic3_cfg_vf_vlan(struct hinic3_nic_io *nic_io, u8 opcode, u16 vid,
		       u8 qos, int vf_id);

int hinic3_vf_event_handler(void *hwdev,
			    u16 cmd, void *buf_in, u16 in_size,
			    void *buf_out, u16 *out_size);

void hinic3_pf_event_handler(void *hwdev, u16 cmd,
			     void *buf_in, u16 in_size,
			     void *buf_out, u16 *out_size);

int hinic3_pf_mbox_handler(void *hwdev,
			   u16 vf_id, u16 cmd, void *buf_in, u16 in_size,
			   void *buf_out, u16 *out_size);

u8 hinic3_nic_sw_aeqe_handler(void *hwdev, u8 event, u8 *data);

int hinic3_vf_func_init(struct hinic3_nic_io *nic_io);

void hinic3_vf_func_free(struct hinic3_nic_io *nic_io);

void hinic3_notify_dcb_state_event(struct hinic3_nic_io *nic_io,
				   struct hinic3_dcb_state *dcb_state);

int hinic3_save_dcb_state(struct hinic3_nic_io *nic_io,
			  struct hinic3_dcb_state *dcb_state);

void hinic3_notify_vf_link_status(struct hinic3_nic_io *nic_io,
				  u16 vf_id, u8 link_status);

int hinic3_vf_mag_event_handler(void *hwdev, u16 cmd,
				void *buf_in, u16 in_size, void *buf_out,
				u16 *out_size);

void hinic3_pf_mag_event_handler(void *pri_handle, u16 cmd,
				 void *buf_in, u16 in_size, void *buf_out,
				 u16 *out_size);

int hinic3_pf_mag_mbox_handler(void *hwdev, u16 vf_id,
			       u16 cmd, void *buf_in, u16 in_size,
			       void *buf_out, u16 *out_size);

void hinic3_unregister_vf(struct hinic3_nic_io *nic_io, u16 vf_id);

#endif

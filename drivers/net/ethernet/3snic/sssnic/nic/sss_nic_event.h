/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_EVENT_H
#define SSS_NIC_EVENT_H

#include <linux/types.h>
#include <linux/semaphore.h>

#include "sss_hw_common.h"
#include "sss_nic_io.h"
#include "sss_nic_cfg.h"
#include "sss_nic_cfg_mag_define.h"

enum sss_nic_event_type {
	SSSNIC_EVENT_LINK_DOWN,
	SSSNIC_EVENT_LINK_UP,
	SSSNIC_EVENT_PORT_MODULE_EVENT,
	SSSNIC_EVENT_DCB_STATE_CHANGE,
	SSSNIC_EVENT_MAX
};

struct sss_nic_vf_msg_handler {
	u16 opcode;
	int (*msg_handler)(struct sss_nic_io *nic_io,
			   u16 vf, void *buf_in, u16 in_size, void *buf_out, u16 *out_size);
};

struct nic_event_handler {
	u16 opcode;
	void (*event_handler)(struct sss_nic_io *nic_io, void *buf_in, u16 in_size,
			      void *buf_out, u16 *out_size);
};

int sss_nic_dettach_vf(struct sss_nic_io *nic_io, u16 vf_id);

int sss_nic_l2nic_msg_to_mgmt_sync(void *hwdev, u16 cmd, void *buf_in, u16 in_size,
				   void *buf_out, u16 *out_size);

int sss_nic_l2nic_msg_to_mgmt_sync_ch(void *hwdev, u16 cmd, void *buf_in, u16 in_size,
				      void *buf_out, u16 *out_size, u16 channel);

int sss_nic_pf_mbx_handler(void *hwdev,
			   u16 vf_id, u16 cmd, void *buf_in, u16 in_size,
			   void *buf_out, u16 *out_size);

void sss_nic_notify_dcb_state_event(void *hwdev,
				    struct sss_nic_dcb_info *dcb_info);

int sss_nic_vf_event_handler(void *hwdev,
			     u16 cmd, void *buf_in, u16 in_size,
			     void *buf_out, u16 *out_size);

void sss_nic_pf_event_handler(void *hwdev, u16 cmd,
			      void *buf_in, u16 in_size,
			      void *buf_out, u16 *out_size);

#endif

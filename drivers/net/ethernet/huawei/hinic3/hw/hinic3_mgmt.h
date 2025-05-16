/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_MGMT_H
#define HINIC3_MGMT_H

#include <linux/types.h>
#include <linux/completion.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>

#include "mpu_cmd_base_defs.h"
#include "hinic3_hw.h"
#include "hinic3_api_cmd.h"
#include "hinic3_hwdev.h"

#define HINIC3_MGMT_WQ_NAME			"hinic3_mgmt"

#define HINIC3_CLP_REG_GAP			0x20
#define HINIC3_CLP_INPUT_BUF_LEN_HOST		4096UL
#define HINIC3_CLP_DATA_UNIT_HOST		4UL

enum clp_data_type {
	HINIC3_CLP_REQ_HOST = 0,
	HINIC3_CLP_RSP_HOST = 1
};

enum clp_reg_type {
	HINIC3_CLP_BA_HOST = 0,
	HINIC3_CLP_SIZE_HOST = 1,
	HINIC3_CLP_LEN_HOST = 2,
	HINIC3_CLP_START_REQ_HOST = 3,
	HINIC3_CLP_READY_RSP_HOST = 4
};

#define HINIC3_CLP_REQ_SIZE_OFFSET		0
#define HINIC3_CLP_RSP_SIZE_OFFSET		16
#define HINIC3_CLP_BASE_OFFSET			0
#define HINIC3_CLP_LEN_OFFSET			0
#define HINIC3_CLP_START_OFFSET			31
#define HINIC3_CLP_READY_OFFSET			31
#define HINIC3_CLP_OFFSET(member)		(HINIC3_CLP_##member##_OFFSET)

#define HINIC3_CLP_SIZE_MASK			0x7ffUL
#define HINIC3_CLP_BASE_MASK			0x7ffffffUL
#define HINIC3_CLP_LEN_MASK			0x7ffUL
#define HINIC3_CLP_START_MASK			0x1UL
#define HINIC3_CLP_READY_MASK			0x1UL
#define HINIC3_CLP_MASK(member)			(HINIC3_CLP_##member##_MASK)

#define HINIC3_CLP_DELAY_CNT_MAX		200UL
#define HINIC3_CLP_SRAM_SIZE_REG_MAX		0x3ff
#define HINIC3_CLP_SRAM_BASE_REG_MAX		0x7ffffff
#define HINIC3_CLP_LEN_REG_MAX			0x3ff
#define HINIC3_CLP_START_OR_READY_REG_MAX	0x1

struct hinic3_recv_msg {
	void			*msg;

	u16			msg_len;
	u16			rsvd1;
	enum hinic3_mod_type	mod;

	u16			cmd;
	u8			seq_id;
	u8			rsvd2;
	u16			msg_id;
	u16			rsvd3;

	int			async_mgmt_to_pf;
	u32			rsvd4;

	struct completion	recv_done;
};

struct hinic3_msg_head {
	u8	status;
	u8	version;
	u8	resp_aeq_num;
	u8	rsvd0[5];
};

enum comm_pf_to_mgmt_event_state {
	SEND_EVENT_UNINIT = 0,
	SEND_EVENT_START,
	SEND_EVENT_SUCCESS,
	SEND_EVENT_FAIL,
	SEND_EVENT_TIMEOUT,
	SEND_EVENT_END,
};

enum hinic3_mgmt_msg_cb_state {
	HINIC3_MGMT_MSG_CB_REG = 0,
	HINIC3_MGMT_MSG_CB_RUNNING,
};

struct hinic3_clp_pf_to_mgmt {
	struct semaphore	clp_msg_lock;
	void			*clp_msg_buf;
};

struct hinic3_msg_pf_to_mgmt {
	struct hinic3_hwdev		*hwdev;

	/* Async cmd can not be scheduling */
	spinlock_t			async_msg_lock;
	struct semaphore		sync_msg_lock;

	struct workqueue_struct		*workq;

	void				*async_msg_buf;
	void				*sync_msg_buf;
	void				*mgmt_ack_buf;

	struct hinic3_recv_msg		recv_msg_from_mgmt;
	struct hinic3_recv_msg		recv_resp_msg_from_mgmt;

	u16				async_msg_id;
	u16				sync_msg_id;
	u32				rsvd1;
	struct hinic3_api_cmd_chain	*cmd_chain[HINIC3_API_CMD_MAX];

	hinic3_mgmt_msg_cb		recv_mgmt_msg_cb[HINIC3_MOD_HW_MAX];
	void				*recv_mgmt_msg_data[HINIC3_MOD_HW_MAX];
	unsigned long			mgmt_msg_cb_state[HINIC3_MOD_HW_MAX];

	void				*async_msg_cb_data[HINIC3_MOD_HW_MAX];

	/* lock when sending msg */
	spinlock_t			sync_event_lock;
	enum comm_pf_to_mgmt_event_state event_flag;
	u64				rsvd2;
};

struct hinic3_mgmt_msg_handle_work {
	struct work_struct	work;
	struct hinic3_msg_pf_to_mgmt *pf_to_mgmt;

	void			*msg;
	u16			msg_len;
	u16			rsvd1;

	enum hinic3_mod_type	mod;
	u16			cmd;
	u16			msg_id;

	int			async_mgmt_to_pf;
};

void hinic3_mgmt_msg_aeqe_handler(void *hwdev, u8 *header, u8 size);

int hinic3_pf_to_mgmt_init(struct hinic3_hwdev *hwdev);

void hinic3_pf_to_mgmt_free(struct hinic3_hwdev *hwdev);

int hinic3_pf_to_mgmt_sync(void *hwdev, u8 mod, u16 cmd, void *buf_in,
			   u16 in_size, void *buf_out, u16 *out_size,
			   u32 timeout);
int hinic3_pf_to_mgmt_async(void *hwdev, u8 mod, u16 cmd, const void *buf_in,
			    u16 in_size);

int hinic3_pf_msg_to_mgmt_sync(void *hwdev, u8 mod, u16 cmd, void *buf_in,
			       u16 in_size, void *buf_out, u16 *out_size,
			       u32 timeout);

int hinic3_pf_to_mgmt_no_ack(void *hwdev, enum hinic3_mod_type mod, u8 cmd, void *buf_in,
			     u16 in_size);

int hinic3_api_cmd_read_ack(void *hwdev, u8 dest, const void *cmd, u16 size,
			    void *ack, u16 ack_size);

int hinic3_api_cmd_write_nack(void *hwdev, u8 dest, const void *cmd, u16 size);

int hinic3_pf_clp_to_mgmt(void *hwdev, u8 mod, u16 cmd, const void *buf_in,
			  u16 in_size, void *buf_out, u16 *out_size);

int hinic3_clp_pf_to_mgmt_init(struct hinic3_hwdev *hwdev);

void hinic3_clp_pf_to_mgmt_free(struct hinic3_hwdev *hwdev);

#endif

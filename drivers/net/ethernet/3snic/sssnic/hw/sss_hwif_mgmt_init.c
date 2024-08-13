// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/completion.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_hwdev.h"
#include "sss_hwif_mbx.h"
#include "sss_hwif_mbx_init.h"
#include "sss_hwif_mgmt_common.h"
#include "sss_hwif_ctrlq_init.h"
#include "sss_hwif_adm_init.h"

#define SSS_DEF_OUT_SIZE		1

struct sss_mgmt_msg_handle_work {
	struct work_struct	work;
	struct sss_msg_pf_to_mgmt *pf_to_mgmt;

	void				*msg;
	u16					msg_len;
	u8					no_ack;
	u8					resvd;

	enum sss_mod_type	mod;
	u16					cmd;
	u16					msg_id;
};

static void sss_send_response_mbx_to_mgmt(struct sss_hwdev *hwdev, u8 mod, u16 cmd,
					  void *buf_in, u16 in_size, u16 msg_id)
{
	struct sss_mbx_msg_info info;

	info.msg_id = (u8)msg_id;
	info.state = 0;

	sss_send_mbx_msg(hwdev->mbx, mod, cmd, buf_in, in_size,
			 SSS_MGMT_SRC_ID, SSS_RESP_MSG, SSS_MSG_NO_ACK, &info);
}

static void sss_mgmt_recv_msg_handler(struct sss_msg_pf_to_mgmt *mgmt_msg,
				      u8 mod, u16 cmd, void *in_buf,
				      u16 in_size, u16 msg_id, int resp_need)
{
	u16 size;
	u16 out_size = 0;
	void *dev_hdl = SSS_TO_HWDEV(mgmt_msg)->dev_hdl;
	void *out_buf = mgmt_msg->ack_buf;

	memset(out_buf, 0, SSS_PF_MGMT_BUF_LEN_MAX);

	if (mod >= SSS_MOD_TYPE_HW_MAX) {
		sdk_warn(dev_hdl, "Recv illegal msg from mgmt cpu, mod = %d\n", mod);
		out_size = sizeof(struct sss_mgmt_msg_head);
		((struct sss_mgmt_msg_head *)out_buf)->state = SSS_MGMT_CMD_UNSUPPORTED;
		goto out;
	}

	set_bit(SSS_CALLBACK_RUNNING, &mgmt_msg->recv_handler_state[mod]);

	if (!mgmt_msg->recv_handler[mod] ||
	    !test_bit(SSS_CALLBACK_REG, &mgmt_msg->recv_handler_state[mod])) {
		sdk_warn(dev_hdl, "Recv mgmt cb is null, mod = %d\n", mod);
		clear_bit(SSS_CALLBACK_RUNNING, &mgmt_msg->recv_handler_state[mod]);
		out_size = sizeof(struct sss_mgmt_msg_head);
		((struct sss_mgmt_msg_head *)out_buf)->state = SSS_MGMT_CMD_UNSUPPORTED;
		goto out;
	}

	mgmt_msg->recv_handler[mod](mgmt_msg->recv_data[mod],
				    cmd, in_buf, in_size, out_buf, &out_size);

	clear_bit(SSS_CALLBACK_RUNNING, &mgmt_msg->recv_handler_state[mod]);

out:
	if (resp_need != 0) {
		size = (out_size == 0) ? SSS_DEF_OUT_SIZE : out_size;
		sss_send_response_mbx_to_mgmt(SSS_TO_HWDEV(mgmt_msg), mod, cmd,
					      out_buf, size, msg_id);
	}
}

static void sss_recv_mgmt_msg_work_handler(struct work_struct *work)
{
	struct sss_mgmt_msg_handle_work *msg_work =
		container_of(work, struct sss_mgmt_msg_handle_work, work);

	sss_mgmt_recv_msg_handler(msg_work->pf_to_mgmt, msg_work->mod,
				  msg_work->cmd, msg_work->msg, msg_work->msg_len, msg_work->msg_id,
				  !msg_work->no_ack);

	destroy_work(&msg_work->work);

	kfree(msg_work->msg);
	kfree(msg_work);
}

static void sss_init_mgmt_recv_msg(struct sss_recv_msg *msg_recv, u64 msg_header)
{
	msg_recv->cmd = SSS_GET_MSG_HEADER(msg_header, CMD);
	msg_recv->mod = SSS_GET_MSG_HEADER(msg_header, MODULE);
	msg_recv->no_ack = SSS_GET_MSG_HEADER(msg_header, NO_ACK);
	msg_recv->buf_len = SSS_GET_MSG_HEADER(msg_header, MSG_LEN);
	msg_recv->msg_id = SSS_GET_MSG_HEADER(msg_header, MSG_ID);
	msg_recv->seq_id = SSS_MGMT_SEQ_ID_MAX;
}

static bool sss_check_mgmt_head_info(struct sss_recv_msg *msg_recv, u64 header)
{
	u8 seg_len = SSS_GET_MSG_HEADER(header, SEG_LEN);
	u8 seg_id  = SSS_GET_MSG_HEADER(header, SEQID);
	u16 msg_id = SSS_GET_MSG_HEADER(header, MSG_ID);

	if (seg_id > SSS_MGMT_SEQ_ID_MAX || seg_len > SSS_SEG_LEN ||
	    (seg_id == SSS_MGMT_SEQ_ID_MAX && seg_len > SSS_MGMT_LAST_SEG_LEN_MAX))
		return false;

	if (seg_id == 0) {
		msg_recv->msg_id = msg_id;
		msg_recv->seq_id = seg_id;

		return true;
	}

	if (seg_id != (msg_recv->seq_id + 1) || msg_id != msg_recv->msg_id)
		return false;

	msg_recv->seq_id = seg_id;

	return true;
}

static void sss_mgmt_resp_msg_handler(struct sss_msg_pf_to_mgmt *mgmt_msg,
				      struct sss_recv_msg *msg_recv)
{
	void *dev_hdl = SSS_TO_HWDEV(mgmt_msg)->dev_hdl;

	if ((msg_recv->msg_id & SSS_ASYNC_MSG_FLAG) != 0)
		return;

	spin_lock(&mgmt_msg->sync_event_lock);
	if (msg_recv->msg_id == mgmt_msg->sync_msg_id &&
	    mgmt_msg->event_state == SSS_ADM_EVENT_START) {
		mgmt_msg->event_state = SSS_ADM_EVENT_SUCCESS;
		complete(&msg_recv->done);
		spin_unlock(&mgmt_msg->sync_event_lock);
		return;
	}

	sdk_err(dev_hdl, "Send msg id(0x%x) recv msg id(0x%x) dismatch, event state=%d\n",
		mgmt_msg->sync_msg_id, msg_recv->msg_id, mgmt_msg->event_state);
	sdk_err(dev_hdl, "Wait timeout, send and recv msg id(0x%x)(0x%x), event state=%d\n",
		mgmt_msg->sync_msg_id, msg_recv->msg_id, mgmt_msg->event_state);
	spin_unlock(&mgmt_msg->sync_event_lock);
}

static void sss_init_mgmt_msg_work(struct sss_msg_pf_to_mgmt *mgmt_msg,
				   struct sss_recv_msg *msg_recv)
{
	struct sss_mgmt_msg_handle_work *msg_work = NULL;

	msg_work = kzalloc(sizeof(*msg_work), GFP_KERNEL);
	if (!msg_work)
		return;

	if (msg_recv->buf_len != 0) {
		msg_work->msg = kzalloc(msg_recv->buf_len, GFP_KERNEL);
		if (!msg_work->msg) {
			kfree(msg_work);
			return;
		}
	}

	msg_work->pf_to_mgmt = mgmt_msg;
	msg_work->msg_len = msg_recv->buf_len;
	memcpy(msg_work->msg, msg_recv->buf, msg_recv->buf_len);
	msg_work->msg_id = msg_recv->msg_id;
	msg_work->mod = msg_recv->mod;
	msg_work->cmd = msg_recv->cmd;
	msg_work->no_ack = msg_recv->no_ack;

	INIT_WORK(&msg_work->work, sss_recv_mgmt_msg_work_handler);
	queue_work_on(WORK_CPU_UNBOUND, mgmt_msg->workq, &msg_work->work);
}

static void sss_recv_mgmt_msg_handler(struct sss_msg_pf_to_mgmt *mgmt_msg,
				      u8 *msg_header, struct sss_recv_msg *msg_recv)
{
	u8 seq_id;
	u8 seq_len;
	u16 msg_id;
	u32	msg_offset;
	u64 dir;
	u64 header = *((u64 *)msg_header);
	void *msg_body;
	struct sss_hwdev *hwdev = SSS_TO_HWDEV(mgmt_msg);

	dir = SSS_GET_MSG_HEADER(header, DIRECTION);
	msg_id = SSS_GET_MSG_HEADER(header, MSG_ID);
	if (dir == SSS_RESP_MSG && (msg_id & SSS_ASYNC_MSG_FLAG) != 0)
		return;

	if (!sss_check_mgmt_head_info(msg_recv, header)) {
		msg_recv->seq_id = SSS_MGMT_SEQ_ID_MAX;
		sdk_err(hwdev->dev_hdl, "Fail to check Mgmt msg seq id and seq len\n");
		return;
	}

	seq_len = SSS_GET_MSG_HEADER(header, SEG_LEN);
	seq_id  = SSS_GET_MSG_HEADER(header, SEQID);
	msg_offset = seq_id * SSS_SEG_LEN;
	msg_body = msg_header + sizeof(header);
	memcpy((u8 *)msg_recv->buf + msg_offset, msg_body, seq_len);

	if (!SSS_GET_MSG_HEADER(header, LAST))
		return;

	sss_init_mgmt_recv_msg(msg_recv, header);

	if (SSS_GET_MSG_HEADER(header, DIRECTION) == SSS_RESP_MSG) {
		sss_mgmt_resp_msg_handler(mgmt_msg, msg_recv);
		return;
	}

	sss_init_mgmt_msg_work(mgmt_msg, msg_recv);
}

static void sss_set_mbx_event_timeout(struct sss_hwdev *hwdev)
{
	struct sss_mbx *mbx = hwdev->mbx;

	spin_lock(&mbx->mbx_lock);
	if (mbx->event_flag == SSS_EVENT_START)
		mbx->event_flag = SSS_EVENT_TIMEOUT;
	spin_unlock(&mbx->mbx_lock);
}

void sss_mgmt_msg_aeqe_handler(void *hwdev, u8 *msg_header, u8 size)
{
	bool msg_dir;
	struct sss_recv_msg *msg = NULL;
	struct sss_msg_pf_to_mgmt *mgmt_msg = NULL;
	struct sss_hwdev *dev = (struct sss_hwdev *)hwdev;

	if (SSS_GET_MSG_HEADER(*(u64 *)msg_header, SOURCE) == SSS_MSG_SRC_MBX) {
		sss_recv_mbx_aeq_handler(hwdev, msg_header, size);
		return;
	}

	mgmt_msg = dev->pf_to_mgmt;
	if (!mgmt_msg)
		return;

	msg_dir = SSS_GET_MSG_HEADER(*(u64 *)msg_header, DIRECTION) == SSS_DIRECT_SEND_MSG;

	msg = msg_dir ? &mgmt_msg->recv_msg : &mgmt_msg->recv_resp_msg;

	sss_recv_mgmt_msg_handler(mgmt_msg, msg_header, msg);
}

void sss_force_complete_all(void *dev)
{
	struct sss_hwdev *hwdev = dev;

	spin_lock_bh(&hwdev->channel_lock);

	if (sss_get_func_type(hwdev) != SSS_FUNC_TYPE_VF &&
	    test_bit(SSS_HW_ADM_INIT_OK, &hwdev->func_state))
		sss_complete_adm_event(hwdev);

	if (test_bit(SSS_HW_MBX_INIT_OK, &hwdev->func_state))
		sss_set_mbx_event_timeout(hwdev);

	if (test_bit(SSS_HW_CTRLQ_INIT_OK, &hwdev->func_state))
		sss_ctrlq_flush_sync_cmd(hwdev);

	spin_unlock_bh(&hwdev->channel_lock);
}

void sss_flush_mgmt_workq(void *hwdev)
{
	struct sss_hwdev *dev = (struct sss_hwdev *)hwdev;

	flush_workqueue(dev->aeq_info->workq);

	if (sss_get_func_type(dev) != SSS_FUNC_TYPE_VF)
		flush_workqueue(dev->pf_to_mgmt->workq);
}

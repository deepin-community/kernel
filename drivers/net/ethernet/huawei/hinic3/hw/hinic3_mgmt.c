// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [COMM]" fmt

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/semaphore.h>

#include "ossl_knl.h"
#include "hinic3_crm.h"
#include "hinic3_hw.h"
#include "hinic3_common.h"
#include "mpu_inband_cmd.h"
#include "hinic3_hwdev.h"
#include "hinic3_eqs.h"
#include "hinic3_mbox.h"
#include "hinic3_api_cmd.h"
#include "hinic3_prof_adap.h"
#include "hinic3_csr.h"
#include "hinic3_mgmt.h"

#define HINIC3_MSG_TO_MGMT_MAX_LEN	2016

#define HINIC3_API_CHAIN_AEQ_ID		2
#define MAX_PF_MGMT_BUF_SIZE		2048UL
#define SEGMENT_LEN			48
#define ASYNC_MSG_FLAG			0x8
#define MGMT_MSG_MAX_SEQ_ID	(ALIGN(HINIC3_MSG_TO_MGMT_MAX_LEN, \
				       SEGMENT_LEN) / SEGMENT_LEN)

#define MGMT_MSG_LAST_SEG_MAX_LEN	(MAX_PF_MGMT_BUF_SIZE - \
					 SEGMENT_LEN * MGMT_MSG_MAX_SEQ_ID)

#define BUF_OUT_DEFAULT_SIZE		1

#define MGMT_MSG_SIZE_MIN		20
#define MGMT_MSG_SIZE_STEP		16
#define	MGMT_MSG_RSVD_FOR_DEV		8

#define SYNC_MSG_ID_MASK		0x7
#define ASYNC_MSG_ID_MASK		0x7

#define SYNC_FLAG			0
#define ASYNC_FLAG			1

#define MSG_NO_RESP			0xFFFF

#define MGMT_MSG_TIMEOUT		20000 /* millisecond */

#define SYNC_MSG_ID(pf_to_mgmt)	((pf_to_mgmt)->sync_msg_id)

#define SYNC_MSG_ID_INC(pf_to_mgmt)	(SYNC_MSG_ID(pf_to_mgmt) = \
			(SYNC_MSG_ID(pf_to_mgmt) + 1) & SYNC_MSG_ID_MASK)
#define ASYNC_MSG_ID(pf_to_mgmt)	((pf_to_mgmt)->async_msg_id)

#define ASYNC_MSG_ID_INC(pf_to_mgmt)	(ASYNC_MSG_ID(pf_to_mgmt) = \
			((ASYNC_MSG_ID(pf_to_mgmt) + 1) & ASYNC_MSG_ID_MASK) \
			 | ASYNC_MSG_FLAG)

static void pf_to_mgmt_send_event_set(struct hinic3_msg_pf_to_mgmt *pf_to_mgmt,
				      int event_flag)
{
	spin_lock(&pf_to_mgmt->sync_event_lock);
	pf_to_mgmt->event_flag = event_flag;
	spin_unlock(&pf_to_mgmt->sync_event_lock);
}

/**
 * hinic3_register_mgmt_msg_cb - register sync msg handler for a module
 * @hwdev: the pointer to hw device
 * @mod: module in the chip that this handler will handle its sync messages
 * @pri_handle: specific mod's private data that will be used in callback
 * @callback: the handler for a sync message that will handle messages
 **/
int hinic3_register_mgmt_msg_cb(void *hwdev, u8 mod, void *pri_handle,
				hinic3_mgmt_msg_cb callback)
{
	struct hinic3_msg_pf_to_mgmt *pf_to_mgmt = NULL;

	if (mod >= HINIC3_MOD_HW_MAX || !hwdev)
		return -EFAULT;

	pf_to_mgmt = ((struct hinic3_hwdev *)hwdev)->pf_to_mgmt;
	if (!pf_to_mgmt)
		return -EINVAL;

	pf_to_mgmt->recv_mgmt_msg_cb[mod] = callback;
	pf_to_mgmt->recv_mgmt_msg_data[mod] = pri_handle;

	set_bit(HINIC3_MGMT_MSG_CB_REG, &pf_to_mgmt->mgmt_msg_cb_state[mod]);

	return 0;
}
EXPORT_SYMBOL(hinic3_register_mgmt_msg_cb);

/**
 * hinic3_unregister_mgmt_msg_cb - unregister sync msg handler for a module
 * @hwdev: the pointer to hw device
 * @mod: module in the chip that this handler will handle its sync messages
 **/
void hinic3_unregister_mgmt_msg_cb(void *hwdev, u8 mod)
{
	struct hinic3_msg_pf_to_mgmt *pf_to_mgmt = NULL;

	if (!hwdev || mod >= HINIC3_MOD_HW_MAX)
		return;

	pf_to_mgmt = ((struct hinic3_hwdev *)hwdev)->pf_to_mgmt;
	if (!pf_to_mgmt)
		return;

	clear_bit(HINIC3_MGMT_MSG_CB_REG, &pf_to_mgmt->mgmt_msg_cb_state[mod]);

	while (test_bit(HINIC3_MGMT_MSG_CB_RUNNING,
			&pf_to_mgmt->mgmt_msg_cb_state[mod]))
		usleep_range(900, 1000); /* sleep 900 us ~ 1000 us */

	pf_to_mgmt->recv_mgmt_msg_cb[mod] = NULL;
	pf_to_mgmt->recv_mgmt_msg_data[mod] = NULL;
}
EXPORT_SYMBOL(hinic3_unregister_mgmt_msg_cb);

/**
 * mgmt_msg_len - calculate the total message length
 * @msg_data_len: the length of the message data
 * Return: the total message length
 **/
static u16 mgmt_msg_len(u16 msg_data_len)
{
	/* u64 - the size of the header */
	u16 msg_size;

	msg_size = (u16)(MGMT_MSG_RSVD_FOR_DEV + sizeof(u64) + msg_data_len);

	if (msg_size > MGMT_MSG_SIZE_MIN)
		msg_size = MGMT_MSG_SIZE_MIN +
				ALIGN((msg_size - MGMT_MSG_SIZE_MIN),
				      MGMT_MSG_SIZE_STEP);
	else
		msg_size = MGMT_MSG_SIZE_MIN;

	return msg_size;
}

/**
 * prepare_header - prepare the header of the message
 * @pf_to_mgmt: PF to MGMT channel
 * @header: pointer of the header to prepare
 * @msg_len: the length of the message
 * @mod: module in the chip that will get the message
 * @direction: the direction of the original message
 * @msg_id: message id
 **/
static void prepare_header(struct hinic3_msg_pf_to_mgmt *pf_to_mgmt,
			   u64 *header, u16 msg_len, u8 mod,
			   enum hinic3_msg_ack_type ack_type,
			   enum hinic3_msg_direction_type direction,
			   enum hinic3_mgmt_cmd cmd, u32 msg_id)
{
	struct hinic3_hwif *hwif = pf_to_mgmt->hwdev->hwif;

	*header = HINIC3_MSG_HEADER_SET(msg_len, MSG_LEN) |
		HINIC3_MSG_HEADER_SET(mod, MODULE) |
		HINIC3_MSG_HEADER_SET(msg_len, SEG_LEN) |
		HINIC3_MSG_HEADER_SET(ack_type, NO_ACK) |
		HINIC3_MSG_HEADER_SET(HINIC3_DATA_INLINE, DATA_TYPE) |
		HINIC3_MSG_HEADER_SET(0, SEQID) |
		HINIC3_MSG_HEADER_SET(HINIC3_API_CHAIN_AEQ_ID, AEQ_ID) |
		HINIC3_MSG_HEADER_SET(LAST_SEGMENT, LAST) |
		HINIC3_MSG_HEADER_SET(direction, DIRECTION) |
		HINIC3_MSG_HEADER_SET(cmd, CMD) |
		HINIC3_MSG_HEADER_SET(HINIC3_MSG_FROM_MGMT, SOURCE) |
		HINIC3_MSG_HEADER_SET(hwif->attr.func_global_idx,
				      SRC_GLB_FUNC_IDX) |
		HINIC3_MSG_HEADER_SET(msg_id, MSG_ID);
}

static void clp_prepare_header(struct hinic3_hwdev *hwdev, u64 *header,
			       u16 msg_len, u8 mod,
			       enum hinic3_msg_ack_type ack_type,
			       enum hinic3_msg_direction_type direction,
			       enum hinic3_mgmt_cmd cmd, u32 msg_id)
{
	struct hinic3_hwif *hwif = hwdev->hwif;

	*header = HINIC3_MSG_HEADER_SET(msg_len, MSG_LEN) |
		HINIC3_MSG_HEADER_SET(mod, MODULE) |
		HINIC3_MSG_HEADER_SET(msg_len, SEG_LEN) |
		HINIC3_MSG_HEADER_SET(ack_type, NO_ACK) |
		HINIC3_MSG_HEADER_SET(HINIC3_DATA_INLINE, DATA_TYPE) |
		HINIC3_MSG_HEADER_SET(0, SEQID) |
		HINIC3_MSG_HEADER_SET(HINIC3_API_CHAIN_AEQ_ID, AEQ_ID) |
		HINIC3_MSG_HEADER_SET(LAST_SEGMENT, LAST) |
		HINIC3_MSG_HEADER_SET(direction, DIRECTION) |
		HINIC3_MSG_HEADER_SET(cmd, CMD) |
		HINIC3_MSG_HEADER_SET(hwif->attr.func_global_idx,
				      SRC_GLB_FUNC_IDX) |
		HINIC3_MSG_HEADER_SET(msg_id, MSG_ID);
}

/**
 * prepare_mgmt_cmd - prepare the mgmt command
 * @mgmt_cmd: pointer to the command to prepare
 * @header: pointer of the header to prepare
 * @msg: the data of the message
 * @msg_len: the length of the message
 **/
static int prepare_mgmt_cmd(u8 *mgmt_cmd, u64 *header, const void *msg, int msg_len)
{
	u8 *mgmt_cmd_new = mgmt_cmd;

	memset(mgmt_cmd_new, 0, MGMT_MSG_RSVD_FOR_DEV);

	mgmt_cmd_new += MGMT_MSG_RSVD_FOR_DEV;
	memcpy(mgmt_cmd_new, header, sizeof(*header));

	mgmt_cmd_new += sizeof(*header);
	memcpy(mgmt_cmd_new, msg, (size_t)(u32)msg_len);

	return 0;
}

/**
 * send_msg_to_mgmt_sync - send async message
 * @pf_to_mgmt: PF to MGMT channel
 * @mod: module in the chip that will get the message
 * @cmd: command of the message
 * @msg: the msg data
 * @msg_len: the msg data length
 * @direction: the direction of the original message
 * @resp_msg_id: msg id to response for
 * Return: 0 - success, negative - failure
 **/
static int send_msg_to_mgmt_sync(struct hinic3_msg_pf_to_mgmt *pf_to_mgmt,
				 u8 mod, u16 cmd, const void *msg, u16 msg_len,
				 enum hinic3_msg_ack_type ack_type,
				 enum hinic3_msg_direction_type direction,
				 u16 resp_msg_id)
{
	void *mgmt_cmd = pf_to_mgmt->sync_msg_buf;
	struct hinic3_api_cmd_chain *chain = NULL;
	u8 node_id = HINIC3_MGMT_CPU_NODE_ID(pf_to_mgmt->hwdev);
	u64 header;
	u16 cmd_size = mgmt_msg_len(msg_len);
	int ret;

	if (hinic3_get_chip_present_flag(pf_to_mgmt->hwdev) == 0)
		return -EFAULT;

	if (cmd_size > HINIC3_MSG_TO_MGMT_MAX_LEN)
		return -EFAULT;

	if (direction == HINIC3_MSG_RESPONSE)
		prepare_header(pf_to_mgmt, &header, msg_len, mod, ack_type,
			       direction, cmd, resp_msg_id);
	else
		prepare_header(pf_to_mgmt, &header, msg_len, mod, ack_type,
			       direction, cmd, SYNC_MSG_ID_INC(pf_to_mgmt));
	chain = pf_to_mgmt->cmd_chain[HINIC3_API_CMD_WRITE_TO_MGMT_CPU];

	if (ack_type == HINIC3_MSG_ACK)
		pf_to_mgmt_send_event_set(pf_to_mgmt, SEND_EVENT_START);

	ret = prepare_mgmt_cmd((u8 *)mgmt_cmd, &header, msg, msg_len);
	if (ret != 0)
		return ret;

	return hinic3_api_cmd_write(chain, node_id, mgmt_cmd, cmd_size);
}

/**
 * send_msg_to_mgmt_async - send async message
 * @pf_to_mgmt: PF to MGMT channel
 * @mod: module in the chip that will get the message
 * @cmd: command of the message
 * @msg: the data of the message
 * @msg_len: the length of the message
 * @direction: the direction of the original message
 * Return: 0 - success, negative - failure
 **/
static int send_msg_to_mgmt_async(struct hinic3_msg_pf_to_mgmt *pf_to_mgmt,
				  u8 mod, u16 cmd, const void *msg, u16 msg_len,
				  enum hinic3_msg_direction_type direction)
{
	void *mgmt_cmd = pf_to_mgmt->async_msg_buf;
	struct hinic3_api_cmd_chain *chain = NULL;
	u8 node_id = HINIC3_MGMT_CPU_NODE_ID(pf_to_mgmt->hwdev);
	u64 header;
	u16 cmd_size = mgmt_msg_len(msg_len);
	int ret;

	if (hinic3_get_chip_present_flag(pf_to_mgmt->hwdev) == 0)
		return -EFAULT;

	if (cmd_size > HINIC3_MSG_TO_MGMT_MAX_LEN)
		return -EFAULT;

	prepare_header(pf_to_mgmt, &header, msg_len, mod, HINIC3_MSG_NO_ACK,
		       direction, cmd, ASYNC_MSG_ID(pf_to_mgmt));

	ret = prepare_mgmt_cmd((u8 *)mgmt_cmd, &header, msg, msg_len);
	if (ret != 0)
		return ret;

	chain = pf_to_mgmt->cmd_chain[HINIC3_API_CMD_WRITE_ASYNC_TO_MGMT_CPU];

	return hinic3_api_cmd_write(chain, node_id, mgmt_cmd, cmd_size);
}

static inline int msg_to_mgmt_pre(u8 mod, void *buf_in, u16 in_size)
{
	struct hinic3_msg_head *msg_head = NULL;

	/* set aeq fix num to 3, need to ensure response aeq id < 3 */
	if (mod == HINIC3_MOD_COMM || mod == HINIC3_MOD_L2NIC) {
		if (in_size < sizeof(struct hinic3_msg_head))
			return -EINVAL;

		msg_head = buf_in;

		if (msg_head->resp_aeq_num >= HINIC3_MAX_AEQS)
			msg_head->resp_aeq_num = 0;
	}

	return 0;
}

int hinic3_pf_to_mgmt_sync(void *hwdev, u8 mod, u16 cmd, void *buf_in,
			   u16 in_size, void *buf_out, u16 *out_size,
			   u32 timeout)
{
	struct hinic3_msg_pf_to_mgmt *pf_to_mgmt = NULL;
	void *dev = ((struct hinic3_hwdev *)hwdev)->dev_hdl;
	struct hinic3_recv_msg *recv_msg = NULL;
	struct completion *recv_done = NULL;
	ulong timeo;
	int err;
	ulong ret;

	if (!COMM_SUPPORT_API_CHAIN((struct hinic3_hwdev *)hwdev))
		return -EPERM;

	if (!buf_in || in_size == 0)
		return -EINVAL;

	ret = msg_to_mgmt_pre(mod, buf_in, in_size);
	if (ret != 0)
		return -EINVAL;

	pf_to_mgmt = ((struct hinic3_hwdev *)hwdev)->pf_to_mgmt;

	/* Lock the sync_msg_buf */
	down(&pf_to_mgmt->sync_msg_lock);
	recv_msg = &pf_to_mgmt->recv_resp_msg_from_mgmt;
	recv_done = &recv_msg->recv_done;

	init_completion(recv_done);

	err = send_msg_to_mgmt_sync(pf_to_mgmt, mod, cmd, buf_in, in_size,
				    HINIC3_MSG_ACK, HINIC3_MSG_DIRECT_SEND,
				    MSG_NO_RESP);
	if (err) {
		sdk_err(dev, "Failed to send sync msg to mgmt, sync_msg_id: %u\n",
			pf_to_mgmt->sync_msg_id);
		pf_to_mgmt_send_event_set(pf_to_mgmt, SEND_EVENT_FAIL);
		goto unlock_sync_msg;
	}

	timeo = msecs_to_jiffies(timeout ? timeout : MGMT_MSG_TIMEOUT);

	ret = wait_for_completion_timeout(recv_done, timeo);
	if (!ret) {
		sdk_err(dev, "Mgmt response sync cmd timeout, sync_msg_id: %u\n",
			pf_to_mgmt->sync_msg_id);
		hinic3_dump_aeq_info((struct hinic3_hwdev *)hwdev);
		err = -ETIMEDOUT;
		pf_to_mgmt_send_event_set(pf_to_mgmt, SEND_EVENT_TIMEOUT);
		goto unlock_sync_msg;
	}

	spin_lock(&pf_to_mgmt->sync_event_lock);
	if (pf_to_mgmt->event_flag == SEND_EVENT_TIMEOUT) {
		spin_unlock(&pf_to_mgmt->sync_event_lock);
		err = -ETIMEDOUT;
		goto unlock_sync_msg;
	}
	spin_unlock(&pf_to_mgmt->sync_event_lock);

	pf_to_mgmt_send_event_set(pf_to_mgmt, SEND_EVENT_END);

	if (!(((struct hinic3_hwdev *)hwdev)->chip_present_flag)) {
		destroy_completion(recv_done);
		up(&pf_to_mgmt->sync_msg_lock);
		return -ETIMEDOUT;
	}

	if (buf_out && out_size) {
		if (*out_size < recv_msg->msg_len) {
			sdk_err(dev, "Invalid response message length: %u for mod %d cmd %u from mgmt, should less than: %u\n",
				recv_msg->msg_len, mod, cmd, *out_size);
			err = -EFAULT;
			goto unlock_sync_msg;
		}

		if (recv_msg->msg_len)
			memcpy(buf_out, recv_msg->msg, recv_msg->msg_len);
		*out_size = recv_msg->msg_len;
	}

unlock_sync_msg:
	destroy_completion(recv_done);
	up(&pf_to_mgmt->sync_msg_lock);

	return err;
}

int hinic3_pf_to_mgmt_async(void *hwdev, u8 mod, u16 cmd, const void *buf_in,
			    u16 in_size)
{
	struct hinic3_msg_pf_to_mgmt *pf_to_mgmt = NULL;
	void *dev = ((struct hinic3_hwdev *)hwdev)->dev_hdl;
	int err;

	if (!COMM_SUPPORT_API_CHAIN((struct hinic3_hwdev *)hwdev))
		return -EPERM;

	pf_to_mgmt = ((struct hinic3_hwdev *)hwdev)->pf_to_mgmt;

	/* Lock the async_msg_buf */
	spin_lock_bh(&pf_to_mgmt->async_msg_lock);
	ASYNC_MSG_ID_INC(pf_to_mgmt);

	err = send_msg_to_mgmt_async(pf_to_mgmt, mod, cmd, buf_in, in_size,
				     HINIC3_MSG_DIRECT_SEND);
	spin_unlock_bh(&pf_to_mgmt->async_msg_lock);

	if (err) {
		sdk_err(dev, "Failed to send async mgmt msg\n");
		return err;
	}

	return 0;
}

/* This function is only used by tx/rx flush */
int hinic3_pf_to_mgmt_no_ack(void *hwdev, enum hinic3_mod_type mod, u8 cmd, void *buf_in,
			     u16 in_size)
{
	struct hinic3_msg_pf_to_mgmt *pf_to_mgmt = NULL;
	void *dev = NULL;
	int err = -EINVAL;
	struct hinic3_hwdev *tmp_hwdev = NULL;

	if (!hwdev)
		return -EINVAL;

	tmp_hwdev = (struct hinic3_hwdev *)hwdev;
	dev = tmp_hwdev->dev_hdl;
	pf_to_mgmt = tmp_hwdev->pf_to_mgmt;

	if (in_size > HINIC3_MBOX_DATA_SIZE) {
		sdk_err(dev, "Mgmt msg buffer size: %u is invalid\n", in_size);
		return -EINVAL;
	}

	if (!(tmp_hwdev->chip_present_flag))
		return -EPERM;

	/* lock the sync_msg_buf */
	down(&pf_to_mgmt->sync_msg_lock);

	err = send_msg_to_mgmt_sync(pf_to_mgmt, mod, cmd, buf_in, in_size, HINIC3_MSG_NO_ACK,
				    HINIC3_MSG_DIRECT_SEND, MSG_NO_RESP);

	up(&pf_to_mgmt->sync_msg_lock);

	return err;
}

int hinic3_pf_msg_to_mgmt_sync(void *hwdev, u8 mod, u16 cmd, void *buf_in,
			       u16 in_size, void *buf_out, u16 *out_size,
			       u32 timeout)
{
	if (!hwdev)
		return -EINVAL;

	if (hinic3_get_chip_present_flag(hwdev) == 0)
		return -EPERM;

	if (in_size > HINIC3_MSG_TO_MGMT_MAX_LEN)
		return -EINVAL;

	if (!COMM_SUPPORT_API_CHAIN((struct hinic3_hwdev *)hwdev))
		return -EPERM;

	return hinic3_pf_to_mgmt_sync(hwdev, mod, cmd, buf_in, in_size,
				      buf_out, out_size, timeout);
}

int hinic3_msg_to_mgmt_sync(void *hwdev, u8 mod, u16 cmd, void *buf_in,
			    u16 in_size, void *buf_out, u16 *out_size,
			    u32 timeout, u16 channel)
{
	if (!hwdev)
		return -EINVAL;

	if (hinic3_get_chip_present_flag(hwdev) == 0)
		return -EPERM;

	return hinic3_send_mbox_to_mgmt(hwdev, mod, cmd, buf_in, in_size,
					buf_out, out_size, timeout, channel);
}
EXPORT_SYMBOL(hinic3_msg_to_mgmt_sync);

int hinic3_msg_to_mgmt_no_ack(void *hwdev, u8 mod, u16 cmd, void *buf_in,
			      u16 in_size, u16 channel)
{
	if (!hwdev)
		return -EINVAL;

	if (hinic3_get_chip_present_flag(hwdev) == 0)
		return -EPERM;

	return hinic3_send_mbox_to_mgmt_no_ack(hwdev, mod, cmd, buf_in,
					       in_size, channel);
}
EXPORT_SYMBOL(hinic3_msg_to_mgmt_no_ack);

int hinic3_msg_to_mgmt_async(void *hwdev, u8 mod, u16 cmd, void *buf_in,
			     u16 in_size, u16 channel)
{
	return hinic3_msg_to_mgmt_api_chain_async(hwdev, mod, cmd, buf_in,
						  in_size);
}
EXPORT_SYMBOL(hinic3_msg_to_mgmt_async);

int hinic3_msg_to_mgmt_api_chain_sync(void *hwdev, u8 mod, u16 cmd,
				      void *buf_in, u16 in_size, void *buf_out,
				      u16 *out_size, u32 timeout)
{
	if (!hwdev)
		return -EINVAL;

	if (hinic3_get_chip_present_flag(hwdev) == 0)
		return -EPERM;

	if (!COMM_SUPPORT_API_CHAIN((struct hinic3_hwdev *)hwdev)) {
		sdk_err(((struct hinic3_hwdev *)hwdev)->dev_hdl,
			"PF don't support api chain\n");
		return -EPERM;
	}

	return hinic3_pf_msg_to_mgmt_sync(hwdev, mod, cmd, buf_in, in_size,
					  buf_out, out_size, timeout);
}

int hinic3_msg_to_mgmt_api_chain_async(void *hwdev, u8 mod, u16 cmd,
				       const void *buf_in, u16 in_size)
{
	int err;

	if (!hwdev)
		return -EINVAL;

	if (hinic3_func_type(hwdev) == TYPE_VF) {
		err = -EFAULT;
		sdk_err(((struct hinic3_hwdev *)hwdev)->dev_hdl,
			"VF don't support async cmd\n");
	} else if (!COMM_SUPPORT_API_CHAIN((struct hinic3_hwdev *)hwdev)) {
		err = -EPERM;
		sdk_err(((struct hinic3_hwdev *)hwdev)->dev_hdl,
			"PF don't support api chain\n");
	} else {
		err = hinic3_pf_to_mgmt_async(hwdev, mod, cmd, buf_in, in_size);
	}

	return err;
}
EXPORT_SYMBOL(hinic3_msg_to_mgmt_api_chain_async);

static void send_mgmt_ack(struct hinic3_msg_pf_to_mgmt *pf_to_mgmt,
			  u8 mod, u16 cmd, void *buf_in, u16 in_size,
			  u16 msg_id)
{
	u16 buf_size;

	if (!in_size)
		buf_size = BUF_OUT_DEFAULT_SIZE;
	else
		buf_size = in_size;

	hinic3_response_mbox_to_mgmt(pf_to_mgmt->hwdev, mod, cmd, buf_in,
				     buf_size, msg_id);
}

static void mgmt_recv_msg_handler(struct hinic3_msg_pf_to_mgmt *pf_to_mgmt,
				  u8 mod, u16 cmd, void *buf_in, u16 in_size,
				  u16 msg_id, int need_resp)
{
	void *dev = pf_to_mgmt->hwdev->dev_hdl;
	void *buf_out = pf_to_mgmt->mgmt_ack_buf;
	enum hinic3_mod_type tmp_mod = mod;
	bool ack_first = false;
	u16 out_size = 0;

	memset(buf_out, 0, MAX_PF_MGMT_BUF_SIZE);

	if (mod >= HINIC3_MOD_HW_MAX) {
		sdk_warn(dev, "Receive illegal message from mgmt cpu, mod = %d\n",
			 mod);
		goto unsupported;
	}

	set_bit(HINIC3_MGMT_MSG_CB_RUNNING,
		&pf_to_mgmt->mgmt_msg_cb_state[tmp_mod]);

	if (!pf_to_mgmt->recv_mgmt_msg_cb[mod] ||
	    !test_bit(HINIC3_MGMT_MSG_CB_REG,
	    &pf_to_mgmt->mgmt_msg_cb_state[tmp_mod])) {
		sdk_warn(dev, "Receive mgmt callback is null, mod = %u, cmd=%u\n", mod, cmd);
		clear_bit(HINIC3_MGMT_MSG_CB_RUNNING,
			  &pf_to_mgmt->mgmt_msg_cb_state[tmp_mod]);
		goto unsupported;
	}

	pf_to_mgmt->recv_mgmt_msg_cb[tmp_mod](pf_to_mgmt->recv_mgmt_msg_data[tmp_mod],
					cmd, buf_in, in_size,
					buf_out, &out_size);

	clear_bit(HINIC3_MGMT_MSG_CB_RUNNING,
		  &pf_to_mgmt->mgmt_msg_cb_state[tmp_mod]);

	goto resp;

unsupported:
	out_size = sizeof(struct mgmt_msg_head);
	((struct mgmt_msg_head *)buf_out)->status = HINIC3_MGMT_CMD_UNSUPPORTED;

resp:
	if (!ack_first && need_resp)
		send_mgmt_ack(pf_to_mgmt, mod, cmd, buf_out, out_size, msg_id);
}

/**
 * mgmt_resp_msg_handler - handler for response message from mgmt cpu
 * @pf_to_mgmt: PF to MGMT channel
 * @recv_msg: received message details
 **/
static void mgmt_resp_msg_handler(struct hinic3_msg_pf_to_mgmt *pf_to_mgmt,
				  struct hinic3_recv_msg *recv_msg)
{
	void *dev = pf_to_mgmt->hwdev->dev_hdl;

	/* delete async msg */
	if (recv_msg->msg_id & ASYNC_MSG_FLAG)
		return;

	spin_lock(&pf_to_mgmt->sync_event_lock);
	if (recv_msg->msg_id == pf_to_mgmt->sync_msg_id &&
	    pf_to_mgmt->event_flag == SEND_EVENT_START) {
		pf_to_mgmt->event_flag = SEND_EVENT_SUCCESS;
		complete(&recv_msg->recv_done);
	} else if (recv_msg->msg_id != pf_to_mgmt->sync_msg_id) {
		sdk_err(dev, "Send msg id(0x%x) recv msg id(0x%x) dismatch, event state=%d\n",
			pf_to_mgmt->sync_msg_id, recv_msg->msg_id,
			pf_to_mgmt->event_flag);
	} else {
		sdk_err(dev, "Wait timeout, send msg id(0x%x) recv msg id(0x%x), event state=%d!\n",
			pf_to_mgmt->sync_msg_id, recv_msg->msg_id,
			pf_to_mgmt->event_flag);
	}
	spin_unlock(&pf_to_mgmt->sync_event_lock);
}

static void recv_mgmt_msg_work_handler(struct work_struct *work)
{
	struct hinic3_mgmt_msg_handle_work *mgmt_work =
		container_of(work, struct hinic3_mgmt_msg_handle_work, work);

	mgmt_recv_msg_handler(mgmt_work->pf_to_mgmt, mgmt_work->mod,
			      mgmt_work->cmd, mgmt_work->msg,
			      mgmt_work->msg_len, mgmt_work->msg_id,
			      !mgmt_work->async_mgmt_to_pf);

	destroy_work(&mgmt_work->work);

	kfree(mgmt_work->msg);
	kfree(mgmt_work);
}

static bool check_mgmt_head_info(struct hinic3_recv_msg *recv_msg,
				 u8 seq_id, u8 seg_len, u16 msg_id)
{
	if (seq_id > MGMT_MSG_MAX_SEQ_ID || seg_len > SEGMENT_LEN ||
	    (seq_id == MGMT_MSG_MAX_SEQ_ID && seg_len > MGMT_MSG_LAST_SEG_MAX_LEN))
		return false;

	if (seq_id == 0) {
		recv_msg->seq_id = seq_id;
		recv_msg->msg_id = msg_id;
	} else {
		if (seq_id != recv_msg->seq_id + 1 || msg_id != recv_msg->msg_id)
			return false;

		recv_msg->seq_id = seq_id;
	}

	return true;
}

static void init_mgmt_msg_work(struct hinic3_msg_pf_to_mgmt *pf_to_mgmt,
			       struct hinic3_recv_msg *recv_msg)
{
	struct hinic3_mgmt_msg_handle_work *mgmt_work = NULL;
	struct hinic3_hwdev *hwdev = pf_to_mgmt->hwdev;

	mgmt_work = kzalloc(sizeof(*mgmt_work), GFP_KERNEL);
	if (!mgmt_work)
		return;

	if (recv_msg->msg_len) {
		mgmt_work->msg = kzalloc(recv_msg->msg_len, GFP_KERNEL);
		if (!mgmt_work->msg) {
			kfree(mgmt_work);
			return;
		}
	}

	mgmt_work->pf_to_mgmt = pf_to_mgmt;
	mgmt_work->msg_len = recv_msg->msg_len;
	memcpy(mgmt_work->msg, recv_msg->msg, recv_msg->msg_len);
	mgmt_work->msg_id = recv_msg->msg_id;
	mgmt_work->mod = recv_msg->mod;
	mgmt_work->cmd = recv_msg->cmd;
	mgmt_work->async_mgmt_to_pf = recv_msg->async_mgmt_to_pf;

	INIT_WORK(&mgmt_work->work, recv_mgmt_msg_work_handler);
	queue_work_on(hisdk3_get_work_cpu_affinity(hwdev, WORK_TYPE_MGMT_MSG),
		      pf_to_mgmt->workq, &mgmt_work->work);
}

/**
 * recv_mgmt_msg_handler - handler a message from mgmt cpu
 * @pf_to_mgmt: PF to MGMT channel
 * @header: the header of the message
 * @recv_msg: received message details
 **/
static void recv_mgmt_msg_handler(struct hinic3_msg_pf_to_mgmt *pf_to_mgmt,
				  u8 *header, struct hinic3_recv_msg *recv_msg)
{
	struct hinic3_hwdev *hwdev = pf_to_mgmt->hwdev;
	u64 mbox_header = *((u64 *)header);
	void *msg_body = header + sizeof(mbox_header);
	u8 seq_id, seq_len;
	u16 msg_id;
	u32 offset;
	u64 dir;

	/* Don't need to get anything from hw when cmd is async */
	dir = HINIC3_MSG_HEADER_GET(mbox_header, DIRECTION);
	if (dir == HINIC3_MSG_RESPONSE &&
	    (HINIC3_MSG_HEADER_GET(mbox_header, MSG_ID) & ASYNC_MSG_FLAG))
		return;

	seq_len = HINIC3_MSG_HEADER_GET(mbox_header, SEG_LEN);
	seq_id  = HINIC3_MSG_HEADER_GET(mbox_header, SEQID);
	msg_id = HINIC3_MSG_HEADER_GET(mbox_header, MSG_ID);
	if (!check_mgmt_head_info(recv_msg, seq_id, seq_len, msg_id)) {
		sdk_err(hwdev->dev_hdl, "Mgmt msg sequence id and segment length check failed\n");
		sdk_err(hwdev->dev_hdl,
			"Front seq_id: 0x%x,current seq_id: 0x%x, seg len: 0x%x, front msg_id: %d, cur: %d\n",
			recv_msg->seq_id, seq_id, seq_len, recv_msg->msg_id, msg_id);
		/* set seq_id to invalid seq_id */
		recv_msg->seq_id = MGMT_MSG_MAX_SEQ_ID;
		return;
	}

	offset  = seq_id * SEGMENT_LEN;
	memcpy((u8 *)recv_msg->msg + offset, msg_body, seq_len);

	if (!HINIC3_MSG_HEADER_GET(mbox_header, LAST))
		return;

	recv_msg->cmd = HINIC3_MSG_HEADER_GET(mbox_header, CMD);
	recv_msg->mod = HINIC3_MSG_HEADER_GET(mbox_header, MODULE);
	recv_msg->async_mgmt_to_pf = HINIC3_MSG_HEADER_GET(mbox_header,
							   NO_ACK);
	recv_msg->msg_len = HINIC3_MSG_HEADER_GET(mbox_header, MSG_LEN);
	recv_msg->msg_id = msg_id;
	recv_msg->seq_id = MGMT_MSG_MAX_SEQ_ID;

	if (HINIC3_MSG_HEADER_GET(mbox_header, DIRECTION) ==
	    HINIC3_MSG_RESPONSE) {
		mgmt_resp_msg_handler(pf_to_mgmt, recv_msg);
		return;
	}

	init_mgmt_msg_work(pf_to_mgmt, recv_msg);
}

/**
 * hinic3_mgmt_msg_aeqe_handler - handler for a mgmt message event
 * @handle: PF to MGMT channel
 * @header: the header of the message
 * @size: unused
 **/
void hinic3_mgmt_msg_aeqe_handler(void *hwdev, u8 *header, u8 size)
{
	struct hinic3_hwdev *dev = (struct hinic3_hwdev *)hwdev;
	struct hinic3_msg_pf_to_mgmt *pf_to_mgmt = NULL;
	struct hinic3_recv_msg *recv_msg = NULL;
	bool is_send_dir = false;

	if ((HINIC3_MSG_HEADER_GET(*(u64 *)header, SOURCE) ==
	     HINIC3_MSG_FROM_MBOX)) {
		hinic3_mbox_func_aeqe_handler(hwdev, header, size);
		return;
	}

	pf_to_mgmt = dev->pf_to_mgmt;
	if (!pf_to_mgmt)
		return;

	is_send_dir = (HINIC3_MSG_HEADER_GET(*(u64 *)header, DIRECTION) ==
		       HINIC3_MSG_DIRECT_SEND) ? true : false;

	recv_msg = is_send_dir ? &pf_to_mgmt->recv_msg_from_mgmt :
		   &pf_to_mgmt->recv_resp_msg_from_mgmt;

	recv_mgmt_msg_handler(pf_to_mgmt, header, recv_msg);
}

/**
 * alloc_recv_msg - allocate received message memory
 * @recv_msg: pointer that will hold the allocated data
 * Return: 0 - success, negative - failure
 **/
static int alloc_recv_msg(struct hinic3_recv_msg *recv_msg)
{
	recv_msg->seq_id = MGMT_MSG_MAX_SEQ_ID;

	recv_msg->msg = kzalloc(MAX_PF_MGMT_BUF_SIZE, GFP_KERNEL);
	if (!recv_msg->msg)
		return -ENOMEM;

	return 0;
}

/**
 * free_recv_msg - free received message memory
 * @recv_msg: pointer that holds the allocated data
 **/
static void free_recv_msg(struct hinic3_recv_msg *recv_msg)
{
	kfree(recv_msg->msg);
	recv_msg->msg = NULL;
}

/**
 * alloc_msg_buf - allocate all the message buffers of PF to MGMT channel
 * @pf_to_mgmt: PF to MGMT channel
 * Return: 0 - success, negative - failure
 **/
static int alloc_msg_buf(struct hinic3_msg_pf_to_mgmt *pf_to_mgmt)
{
	int err;
	void *dev = pf_to_mgmt->hwdev->dev_hdl;

	err = alloc_recv_msg(&pf_to_mgmt->recv_msg_from_mgmt);
	if (err) {
		sdk_err(dev, "Failed to allocate recv msg\n");
		return err;
	}

	err = alloc_recv_msg(&pf_to_mgmt->recv_resp_msg_from_mgmt);
	if (err) {
		sdk_err(dev, "Failed to allocate resp recv msg\n");
		goto alloc_msg_for_resp_err;
	}

	pf_to_mgmt->async_msg_buf = kzalloc(MAX_PF_MGMT_BUF_SIZE, GFP_KERNEL);
	if (!pf_to_mgmt->async_msg_buf) {
		err = -ENOMEM;
		goto async_msg_buf_err;
	}

	pf_to_mgmt->sync_msg_buf = kzalloc(MAX_PF_MGMT_BUF_SIZE, GFP_KERNEL);
	if (!pf_to_mgmt->sync_msg_buf) {
		err = -ENOMEM;
		goto sync_msg_buf_err;
	}

	pf_to_mgmt->mgmt_ack_buf = kzalloc(MAX_PF_MGMT_BUF_SIZE, GFP_KERNEL);
	if (!pf_to_mgmt->mgmt_ack_buf) {
		err = -ENOMEM;
		goto ack_msg_buf_err;
	}

	return 0;

ack_msg_buf_err:
	kfree(pf_to_mgmt->sync_msg_buf);

sync_msg_buf_err:
	kfree(pf_to_mgmt->async_msg_buf);

async_msg_buf_err:
	free_recv_msg(&pf_to_mgmt->recv_resp_msg_from_mgmt);

alloc_msg_for_resp_err:
	free_recv_msg(&pf_to_mgmt->recv_msg_from_mgmt);
	return err;
}

/**
 * free_msg_buf - free all the message buffers of PF to MGMT channel
 * @pf_to_mgmt: PF to MGMT channel
 * Return: 0 - success, negative - failure
 **/
static void free_msg_buf(struct hinic3_msg_pf_to_mgmt *pf_to_mgmt)
{
	kfree(pf_to_mgmt->mgmt_ack_buf);
	kfree(pf_to_mgmt->sync_msg_buf);
	kfree(pf_to_mgmt->async_msg_buf);

	free_recv_msg(&pf_to_mgmt->recv_resp_msg_from_mgmt);
	free_recv_msg(&pf_to_mgmt->recv_msg_from_mgmt);
	pf_to_mgmt->mgmt_ack_buf = NULL;
	pf_to_mgmt->sync_msg_buf = NULL;
	pf_to_mgmt->async_msg_buf = NULL;
}

/**
 * hinic_pf_to_mgmt_init - initialize PF to MGMT channel
 * @hwdev: the pointer to hw device
 * Return: 0 - success, negative - failure
 **/
int hinic3_pf_to_mgmt_init(struct hinic3_hwdev *hwdev)
{
	struct hinic3_msg_pf_to_mgmt *pf_to_mgmt;
	void *dev = hwdev->dev_hdl;
	int err;

	pf_to_mgmt = kzalloc(sizeof(*pf_to_mgmt), GFP_KERNEL);
	if (!pf_to_mgmt)
		return -ENOMEM;

	hwdev->pf_to_mgmt = pf_to_mgmt;
	pf_to_mgmt->hwdev = hwdev;
	spin_lock_init(&pf_to_mgmt->async_msg_lock);
	spin_lock_init(&pf_to_mgmt->sync_event_lock);
	sema_init(&pf_to_mgmt->sync_msg_lock, 1);
	pf_to_mgmt->workq = create_singlethread_workqueue(HINIC3_MGMT_WQ_NAME);
	if (!pf_to_mgmt->workq) {
		sdk_err(dev, "Failed to initialize MGMT workqueue\n");
		err = -ENOMEM;
		goto create_mgmt_workq_err;
	}

	err = alloc_msg_buf(pf_to_mgmt);
	if (err) {
		sdk_err(dev, "Failed to allocate msg buffers\n");
		goto alloc_msg_buf_err;
	}

	err = hinic3_api_cmd_init(hwdev, pf_to_mgmt->cmd_chain);
	if (err) {
		sdk_err(dev, "Failed to init the api cmd chains\n");
		goto api_cmd_init_err;
	}

	return 0;

api_cmd_init_err:
	free_msg_buf(pf_to_mgmt);

alloc_msg_buf_err:
	destroy_workqueue(pf_to_mgmt->workq);

create_mgmt_workq_err:
	spin_lock_deinit(&pf_to_mgmt->sync_event_lock);
	spin_lock_deinit(&pf_to_mgmt->async_msg_lock);
	sema_deinit(&pf_to_mgmt->sync_msg_lock);
	kfree(pf_to_mgmt);

	return err;
}

/**
 * hinic_pf_to_mgmt_free - free PF to MGMT channel
 * @hwdev: the pointer to hw device
 **/
void hinic3_pf_to_mgmt_free(struct hinic3_hwdev *hwdev)
{
	struct hinic3_msg_pf_to_mgmt *pf_to_mgmt = hwdev->pf_to_mgmt;

	/* destroy workqueue before free related pf_to_mgmt resources in case of
	 * illegal resource access
	 */
	destroy_workqueue(pf_to_mgmt->workq);
	hinic3_api_cmd_free(hwdev, pf_to_mgmt->cmd_chain);

	free_msg_buf(pf_to_mgmt);
	spin_lock_deinit(&pf_to_mgmt->sync_event_lock);
	spin_lock_deinit(&pf_to_mgmt->async_msg_lock);
	sema_deinit(&pf_to_mgmt->sync_msg_lock);
	kfree(pf_to_mgmt);
}

void hinic3_flush_mgmt_workq(void *hwdev)
{
	struct hinic3_hwdev *dev = (struct hinic3_hwdev *)hwdev;

	flush_workqueue(dev->aeqs->workq);

	if (hinic3_func_type(dev) != TYPE_VF)
		flush_workqueue(dev->pf_to_mgmt->workq);
}

int hinic3_api_cmd_read_ack(void *hwdev, u8 dest, const void *cmd,
			    u16 size, void *ack, u16 ack_size)
{
	struct hinic3_msg_pf_to_mgmt *pf_to_mgmt = NULL;
	struct hinic3_api_cmd_chain *chain = NULL;

	if (!hwdev || !cmd || (ack_size && !ack) || size > MAX_PF_MGMT_BUF_SIZE)
		return -EINVAL;

	if (!COMM_SUPPORT_API_CHAIN((struct hinic3_hwdev *)hwdev))
		return -EPERM;

	pf_to_mgmt = ((struct hinic3_hwdev *)hwdev)->pf_to_mgmt;
	chain = pf_to_mgmt->cmd_chain[HINIC3_API_CMD_POLL_READ];

	if (!(((struct hinic3_hwdev *)hwdev)->chip_present_flag))
		return -EPERM;

	return hinic3_api_cmd_read(chain, dest, cmd, size, ack, ack_size);
}

/**
 * api cmd write or read bypass default use poll, if want to use aeq interrupt,
 * please set wb_trigger_aeqe to 1
 **/
int hinic3_api_cmd_write_nack(void *hwdev, u8 dest, const void *cmd, u16 size)
{
	struct hinic3_msg_pf_to_mgmt *pf_to_mgmt = NULL;
	struct hinic3_api_cmd_chain *chain = NULL;

	if (!hwdev || !size || !cmd || size > MAX_PF_MGMT_BUF_SIZE)
		return -EINVAL;

	if (!COMM_SUPPORT_API_CHAIN((struct hinic3_hwdev *)hwdev))
		return -EPERM;

	pf_to_mgmt = ((struct hinic3_hwdev *)hwdev)->pf_to_mgmt;
	chain = pf_to_mgmt->cmd_chain[HINIC3_API_CMD_POLL_WRITE];

	if (!(((struct hinic3_hwdev *)hwdev)->chip_present_flag))
		return -EPERM;

	return hinic3_api_cmd_write(chain, dest, cmd, size);
}

static int get_clp_reg(void *hwdev, enum clp_data_type data_type,
		       enum clp_reg_type reg_type, u32 *reg_addr)
{
	switch (reg_type) {
	case HINIC3_CLP_BA_HOST:
		*reg_addr = (data_type == HINIC3_CLP_REQ_HOST) ?
			     HINIC3_CLP_REG(REQBASE) :
			     HINIC3_CLP_REG(RSPBASE);
		break;

	case HINIC3_CLP_SIZE_HOST:
		*reg_addr = HINIC3_CLP_REG(SIZE);
		break;

	case HINIC3_CLP_LEN_HOST:
		*reg_addr = (data_type == HINIC3_CLP_REQ_HOST) ?
			     HINIC3_CLP_REG(REQ) : HINIC3_CLP_REG(RSP);
		break;

	case HINIC3_CLP_START_REQ_HOST:
		*reg_addr = HINIC3_CLP_REG(REQ);
		break;

	case HINIC3_CLP_READY_RSP_HOST:
		*reg_addr = HINIC3_CLP_REG(RSP);
		break;

	default:
		*reg_addr = 0;
		break;
	}
	if (*reg_addr == 0)
		return -EINVAL;

	return 0;
}

static inline int clp_param_valid(struct hinic3_hwdev *hwdev,
				  enum clp_data_type data_type,
				  enum clp_reg_type reg_type)
{
	if (data_type == HINIC3_CLP_REQ_HOST &&
	    reg_type == HINIC3_CLP_READY_RSP_HOST)
		return -EINVAL;

	if (data_type == HINIC3_CLP_RSP_HOST &&
	    reg_type == HINIC3_CLP_START_REQ_HOST)
		return -EINVAL;

	return 0;
}

static u32 get_clp_reg_value(struct hinic3_hwdev *hwdev,
			     enum clp_data_type data_type,
			     enum clp_reg_type reg_type, u32 reg_addr)
{
	u32 value;

	value = hinic3_hwif_read_reg(hwdev->hwif, reg_addr);

	switch (reg_type) {
	case HINIC3_CLP_BA_HOST:
		value = ((value >> HINIC3_CLP_OFFSET(BASE)) &
			HINIC3_CLP_MASK(BASE));
		break;

	case HINIC3_CLP_SIZE_HOST:
		if (data_type == HINIC3_CLP_REQ_HOST)
			value = ((value >> HINIC3_CLP_OFFSET(REQ_SIZE)) &
				HINIC3_CLP_MASK(SIZE));
		else
			value = ((value >> HINIC3_CLP_OFFSET(RSP_SIZE)) &
				HINIC3_CLP_MASK(SIZE));
		break;

	case HINIC3_CLP_LEN_HOST:
		value = ((value >> HINIC3_CLP_OFFSET(LEN)) &
			HINIC3_CLP_MASK(LEN));
		break;

	case HINIC3_CLP_START_REQ_HOST:
		value = ((value >> HINIC3_CLP_OFFSET(START)) &
			HINIC3_CLP_MASK(START));
		break;

	case HINIC3_CLP_READY_RSP_HOST:
		value = ((value >> HINIC3_CLP_OFFSET(READY)) &
			HINIC3_CLP_MASK(READY));
		break;

	default:
		break;
	}

	return value;
}

static int hinic3_read_clp_reg(struct hinic3_hwdev *hwdev,
			       enum clp_data_type data_type,
			       enum clp_reg_type reg_type, u32 *read_value)
{
	u32 reg_addr;
	int err;

	err = clp_param_valid(hwdev, data_type, reg_type);
	if (err)
		return err;

	err = get_clp_reg(hwdev, data_type, reg_type, &reg_addr);
	if (err)
		return err;

	*read_value = get_clp_reg_value(hwdev, data_type, reg_type, reg_addr);

	return 0;
}

static int check_data_type(enum clp_data_type data_type,
			   enum clp_reg_type reg_type)
{
	if (data_type == HINIC3_CLP_REQ_HOST &&
	    reg_type == HINIC3_CLP_READY_RSP_HOST)
		return -EINVAL;
	if (data_type == HINIC3_CLP_RSP_HOST &&
	    reg_type == HINIC3_CLP_START_REQ_HOST)
		return -EINVAL;

	return 0;
}

static int check_reg_value(enum clp_reg_type reg_type, u32 value)
{
	if (reg_type == HINIC3_CLP_BA_HOST &&
	    value > HINIC3_CLP_SRAM_BASE_REG_MAX)
		return -EINVAL;

	if (reg_type == HINIC3_CLP_SIZE_HOST &&
	    value > HINIC3_CLP_SRAM_SIZE_REG_MAX)
		return -EINVAL;

	if (reg_type == HINIC3_CLP_LEN_HOST &&
	    value > HINIC3_CLP_LEN_REG_MAX)
		return -EINVAL;

	if ((reg_type == HINIC3_CLP_START_REQ_HOST ||
	     reg_type == HINIC3_CLP_READY_RSP_HOST) &&
	    value > HINIC3_CLP_START_OR_READY_REG_MAX)
		return -EINVAL;

	return 0;
}

static int hinic3_check_clp_init_status(struct hinic3_hwdev *hwdev)
{
	int err;
	u32 reg_value = 0;

	err = hinic3_read_clp_reg(hwdev, HINIC3_CLP_REQ_HOST,
				  HINIC3_CLP_BA_HOST, &reg_value);
	if (err || !reg_value) {
		sdk_err(hwdev->dev_hdl, "Wrong req ba value: 0x%x\n",
			reg_value);
		return -EINVAL;
	}

	err = hinic3_read_clp_reg(hwdev, HINIC3_CLP_RSP_HOST,
				  HINIC3_CLP_BA_HOST, &reg_value);
	if (err || !reg_value) {
		sdk_err(hwdev->dev_hdl, "Wrong rsp ba value: 0x%x\n",
			reg_value);
		return -EINVAL;
	}

	err = hinic3_read_clp_reg(hwdev, HINIC3_CLP_REQ_HOST,
				  HINIC3_CLP_SIZE_HOST, &reg_value);
	if (err || !reg_value) {
		sdk_err(hwdev->dev_hdl, "Wrong req size\n");
		return -EINVAL;
	}

	err = hinic3_read_clp_reg(hwdev, HINIC3_CLP_RSP_HOST,
				  HINIC3_CLP_SIZE_HOST, &reg_value);
	if (err || !reg_value) {
		sdk_err(hwdev->dev_hdl, "Wrong rsp size\n");
		return -EINVAL;
	}

	return 0;
}

static void hinic3_write_clp_reg(struct hinic3_hwdev *hwdev,
				 enum clp_data_type data_type,
				 enum clp_reg_type reg_type, u32 value)
{
	u32 reg_addr, reg_value;

	if (check_data_type(data_type, reg_type))
		return;

	if (check_reg_value(reg_type, value))
		return;

	if (get_clp_reg(hwdev, data_type, reg_type, &reg_addr))
		return;

	reg_value = hinic3_hwif_read_reg(hwdev->hwif, reg_addr);

	switch (reg_type) {
	case HINIC3_CLP_LEN_HOST:
		reg_value = reg_value &
			    (~(HINIC3_CLP_MASK(LEN) << HINIC3_CLP_OFFSET(LEN)));
		reg_value = reg_value | (value << HINIC3_CLP_OFFSET(LEN));
		break;

	case HINIC3_CLP_START_REQ_HOST:
		reg_value = reg_value &
			    (~(HINIC3_CLP_MASK(START) <<
			      HINIC3_CLP_OFFSET(START)));
		reg_value = reg_value | (value << HINIC3_CLP_OFFSET(START));
		break;

	case HINIC3_CLP_READY_RSP_HOST:
		reg_value = reg_value &
			    (~(HINIC3_CLP_MASK(READY) <<
			    HINIC3_CLP_OFFSET(READY)));
		reg_value = reg_value | (value << HINIC3_CLP_OFFSET(READY));
		break;

	default:
		return;
	}

	hinic3_hwif_write_reg(hwdev->hwif, reg_addr, reg_value);
}

static int hinic3_read_clp_data(struct hinic3_hwdev *hwdev,
				void *buf_out, u16 *out_size)
{
	int err;
	u32 reg = HINIC3_CLP_DATA(RSP);
	u32 ready, delay_cnt;
	u32 *ptr = (u32 *)buf_out;
	u32 temp_out_size = 0;

	err = hinic3_read_clp_reg(hwdev, HINIC3_CLP_RSP_HOST,
				  HINIC3_CLP_READY_RSP_HOST, &ready);
	if (err)
		return err;

	delay_cnt = 0;
	while (ready == 0) {
		usleep_range(9000, 10000); /* sleep 9000 us ~ 10000 us */
		delay_cnt++;
		err = hinic3_read_clp_reg(hwdev, HINIC3_CLP_RSP_HOST,
					  HINIC3_CLP_READY_RSP_HOST, &ready);
		if (err || delay_cnt > HINIC3_CLP_DELAY_CNT_MAX) {
			sdk_err(hwdev->dev_hdl, "Timeout with delay_cnt: %u\n",
				delay_cnt);
			return -EINVAL;
		}
	}

	err = hinic3_read_clp_reg(hwdev, HINIC3_CLP_RSP_HOST,
				  HINIC3_CLP_LEN_HOST, &temp_out_size);
	if (err)
		return err;

	if (temp_out_size > HINIC3_CLP_SRAM_SIZE_REG_MAX || !temp_out_size) {
		sdk_err(hwdev->dev_hdl, "Invalid temp_out_size: %u\n",
			temp_out_size);
		return -EINVAL;
	}

	*out_size = (u16)temp_out_size;
	for (; temp_out_size > 0; temp_out_size--) {
		*ptr = hinic3_hwif_read_reg(hwdev->hwif, reg);
		ptr++;
		/* read 4 bytes every time */
		reg = reg + 4;
	}

	hinic3_write_clp_reg(hwdev, HINIC3_CLP_RSP_HOST,
			     HINIC3_CLP_READY_RSP_HOST, (u32)0x0);
	hinic3_write_clp_reg(hwdev, HINIC3_CLP_RSP_HOST, HINIC3_CLP_LEN_HOST,
			     (u32)0x0);

	return 0;
}

static int hinic3_write_clp_data(struct hinic3_hwdev *hwdev,
				 void *buf_in, u16 in_size)
{
	int err;
	u32 reg = HINIC3_CLP_DATA(REQ);
	u32 start = 1;
	u32 delay_cnt = 0;
	u32 *ptr = (u32 *)buf_in;
	u16 size_in = in_size;

	err = hinic3_read_clp_reg(hwdev, HINIC3_CLP_REQ_HOST,
				  HINIC3_CLP_START_REQ_HOST, &start);
	if (err != 0)
		return err;

	while (start == 1) {
		usleep_range(9000, 10000); /* sleep 9000 us ~ 10000 us */
		delay_cnt++;
		err = hinic3_read_clp_reg(hwdev, HINIC3_CLP_REQ_HOST,
					  HINIC3_CLP_START_REQ_HOST, &start);
		if (err || delay_cnt > HINIC3_CLP_DELAY_CNT_MAX)
			return -EINVAL;
	}

	hinic3_write_clp_reg(hwdev, HINIC3_CLP_REQ_HOST,
			     HINIC3_CLP_LEN_HOST, size_in);
	hinic3_write_clp_reg(hwdev, HINIC3_CLP_REQ_HOST,
			     HINIC3_CLP_START_REQ_HOST, (u32)0x1);

	for (; size_in > 0; size_in--) {
		hinic3_hwif_write_reg(hwdev->hwif, reg, *ptr);
		ptr++;
		reg = reg + sizeof(u32);
	}

	return 0;
}

static void hinic3_clear_clp_data(struct hinic3_hwdev *hwdev,
				  enum clp_data_type data_type)
{
	u32 reg = (data_type == HINIC3_CLP_REQ_HOST) ?
		   HINIC3_CLP_DATA(REQ) : HINIC3_CLP_DATA(RSP);
	u32 count = HINIC3_CLP_INPUT_BUF_LEN_HOST / HINIC3_CLP_DATA_UNIT_HOST;

	for (; count > 0; count--) {
		hinic3_hwif_write_reg(hwdev->hwif, reg, 0x0);
		reg = reg + sizeof(u32);
	}
}

int hinic3_pf_clp_to_mgmt(void *hwdev, u8 mod, u16 cmd, const void *buf_in,
			  u16 in_size, void *buf_out, u16 *out_size)
{
	struct hinic3_clp_pf_to_mgmt *clp_pf_to_mgmt = NULL;
	struct hinic3_hwdev *dev = hwdev;
	u64 header;
	u16 real_size;
	u8 *clp_msg_buf = NULL;
	int err;

	if (!COMM_SUPPORT_CLP(dev))
		return -EPERM;

	clp_pf_to_mgmt = ((struct hinic3_hwdev *)hwdev)->clp_pf_to_mgmt;
	if (!clp_pf_to_mgmt)
		return -EPERM;

	clp_msg_buf = clp_pf_to_mgmt->clp_msg_buf;

	/* 4 bytes alignment */
	if (in_size % HINIC3_CLP_DATA_UNIT_HOST)
		real_size = (in_size + (u16)sizeof(header) +
			     HINIC3_CLP_DATA_UNIT_HOST);
	else
		real_size = in_size + (u16)sizeof(header);
	real_size = real_size / HINIC3_CLP_DATA_UNIT_HOST;

	if (real_size >
	    (HINIC3_CLP_INPUT_BUF_LEN_HOST / HINIC3_CLP_DATA_UNIT_HOST)) {
		sdk_err(dev->dev_hdl, "Invalid real_size: %u\n", real_size);
		return -EINVAL;
	}
	down(&clp_pf_to_mgmt->clp_msg_lock);

	err = hinic3_check_clp_init_status(dev);
	if (err) {
		sdk_err(dev->dev_hdl, "Check clp init status failed\n");
		up(&clp_pf_to_mgmt->clp_msg_lock);
		return err;
	}

	hinic3_clear_clp_data(dev, HINIC3_CLP_RSP_HOST);
	hinic3_write_clp_reg(dev, HINIC3_CLP_RSP_HOST,
			     HINIC3_CLP_READY_RSP_HOST, 0x0);

	/* Send request */
	memset(clp_msg_buf, 0x0, HINIC3_CLP_INPUT_BUF_LEN_HOST);
	clp_prepare_header(dev, &header, in_size, mod, 0, 0, cmd, 0);

	memcpy(clp_msg_buf, &header, sizeof(header));

	clp_msg_buf += sizeof(header);
	memcpy(clp_msg_buf, buf_in, in_size);

	clp_msg_buf = clp_pf_to_mgmt->clp_msg_buf;

	hinic3_clear_clp_data(dev, HINIC3_CLP_REQ_HOST);
	err = hinic3_write_clp_data(hwdev,
				    clp_pf_to_mgmt->clp_msg_buf, real_size);
	if (err) {
		sdk_err(dev->dev_hdl, "Send clp request failed\n");
		up(&clp_pf_to_mgmt->clp_msg_lock);
		return -EINVAL;
	}

	/* Get response */
	clp_msg_buf = clp_pf_to_mgmt->clp_msg_buf;
	memset(clp_msg_buf, 0x0, HINIC3_CLP_INPUT_BUF_LEN_HOST);
	err = hinic3_read_clp_data(hwdev, clp_msg_buf, &real_size);
	hinic3_clear_clp_data(dev, HINIC3_CLP_RSP_HOST);
	if (err) {
		sdk_err(dev->dev_hdl, "Read clp response failed\n");
		up(&clp_pf_to_mgmt->clp_msg_lock);
		return -EINVAL;
	}

	real_size = (u16)((real_size * HINIC3_CLP_DATA_UNIT_HOST) & 0xffff);
	if (real_size <= sizeof(header) || real_size > HINIC3_CLP_INPUT_BUF_LEN_HOST) {
		sdk_err(dev->dev_hdl, "Invalid response size: %u", real_size);
		up(&clp_pf_to_mgmt->clp_msg_lock);
		return -EINVAL;
	}
	real_size = real_size - sizeof(header);
	if (real_size != *out_size) {
		sdk_err(dev->dev_hdl, "Invalid real_size:%u, out_size: %u\n",
			real_size, *out_size);
		up(&clp_pf_to_mgmt->clp_msg_lock);
		return -EINVAL;
	}

	memcpy(buf_out, (clp_msg_buf + sizeof(header)), real_size);
	up(&clp_pf_to_mgmt->clp_msg_lock);

	return 0;
}

int hinic3_clp_to_mgmt(void *hwdev, u8 mod, u16 cmd, const void *buf_in,
		       u16 in_size, void *buf_out, u16 *out_size)

{
	struct hinic3_hwdev *dev = hwdev;
	int err;

	if (!dev)
		return -EINVAL;

	if (!dev->chip_present_flag)
		return -EPERM;

	if (hinic3_func_type(hwdev) == TYPE_VF)
		return -EINVAL;

	if (!COMM_SUPPORT_CLP(dev))
		return -EPERM;

	err = hinic3_pf_clp_to_mgmt(dev, mod, cmd, buf_in, in_size, buf_out,
				    out_size);

	return err;
}

int hinic3_clp_pf_to_mgmt_init(struct hinic3_hwdev *hwdev)
{
	struct hinic3_clp_pf_to_mgmt *clp_pf_to_mgmt = NULL;

	if (!COMM_SUPPORT_CLP(hwdev))
		return 0;

	clp_pf_to_mgmt = kzalloc(sizeof(*clp_pf_to_mgmt), GFP_KERNEL);
	if (!clp_pf_to_mgmt)
		return -ENOMEM;

	clp_pf_to_mgmt->clp_msg_buf = kzalloc(HINIC3_CLP_INPUT_BUF_LEN_HOST,
					      GFP_KERNEL);
	if (!clp_pf_to_mgmt->clp_msg_buf) {
		kfree(clp_pf_to_mgmt);
		return -ENOMEM;
	}
	sema_init(&clp_pf_to_mgmt->clp_msg_lock, 1);

	hwdev->clp_pf_to_mgmt = clp_pf_to_mgmt;

	return 0;
}

void hinic3_clp_pf_to_mgmt_free(struct hinic3_hwdev *hwdev)
{
	struct hinic3_clp_pf_to_mgmt *clp_pf_to_mgmt = hwdev->clp_pf_to_mgmt;

	if (!COMM_SUPPORT_CLP(hwdev))
		return;

	sema_deinit(&clp_pf_to_mgmt->clp_msg_lock);
	kfree(clp_pf_to_mgmt->clp_msg_buf);
	kfree(clp_pf_to_mgmt);
}

// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>

#include "sss_kernel.h"
#include "sss_hw_common.h"
#include "sss_hwdev.h"
#include "sss_hwif_api.h"
#include "sss_hwif_eq.h"
#include "sss_hwif_mbx.h"
#include "sss_hwif_aeq.h"
#include "sss_csr.h"
#include "sss_common.h"

#define SSS_MBX_INT_DST_AEQN_SHIFT			10
#define SSS_MBX_INT_SRC_RESP_AEQN_SHIFT	12
#define SSS_MBX_INT_STAT_DMA_SHIFT			14
/* The size of data to be send (unit of 4 bytes) */
#define SSS_MBX_INT_TX_SIZE_SHIFT			20
/* SO_RO(strong order, relax order)  */
#define SSS_MBX_INT_STAT_DMA_SO_RO_SHIFT	25
#define SSS_MBX_INT_WB_EN_SHIFT			28

#define SSS_MBX_INT_DST_AEQN_MASK			0x3
#define SSS_MBX_INT_SRC_RESP_AEQN_MASK		0x3
#define SSS_MBX_INT_STAT_DMA_MASK			0x3F
#define SSS_MBX_INT_TX_SIZE_MASK			0x1F
#define SSS_MBX_INT_STAT_DMA_SO_RO_MASK	0x3
#define SSS_MBX_INT_WB_EN_MASK				0x1

#define SSS_SET_MBX_INT(val, field)	\
			(((val) & SSS_MBX_INT_##field##_MASK) << \
			SSS_MBX_INT_##field##_SHIFT)

enum sss_mbx_tx_status {
	SSS_MBX_TX_NOT_COMPLETE = 1,
};

#define SSS_MBX_CTRL_TRIGGER_AEQE_SHIFT	0

#define SSS_MBX_CTRL_TX_STATUS_SHIFT		1
#define SSS_MBX_CTRL_DST_FUNC_SHIFT		16

#define SSS_MBX_CTRL_TRIGGER_AEQE_MASK		0x1
#define SSS_MBX_CTRL_TX_STATUS_MASK		0x1
#define SSS_MBX_CTRL_DST_FUNC_MASK			0x1FFF

#define SSS_SET_MBX_CTRL(val, field)	\
			(((val) & SSS_MBX_CTRL_##field##_MASK) << \
			SSS_MBX_CTRL_##field##_SHIFT)

#define SSS_MBX_SEGLEN_MASK			\
			SSS_SET_MSG_HEADER(SSS_MSG_HEADER_SEG_LEN_MASK, SEG_LEN)

#define SSS_MBX_MSG_POLL_TIMEOUT_MS		8000
#define SSS_MBX_COMPLETE_WAIT_TIME_MS			40000U

#define SSS_SEQ_ID_START_VAL			0

/* mbx write back status is 16B, only first 4B is used */
#define SSS_MBX_WB_STATUS_ERRCODE_MASK			0xFFFF
#define SSS_MBX_WB_STATUS_MASK					0xFF
#define SSS_MBX_WB_ERRCODE_MASK				0xFF00
#define SSS_MBX_WB_STATUS_FINISHED_SUCCESS		0xFF
#define SSS_MBX_WB_STATUS_NOT_FINISHED			0x00

#define SSS_MBX_STATUS_FINISHED(wb)	\
	(((wb) & SSS_MBX_WB_STATUS_MASK) != SSS_MBX_WB_STATUS_NOT_FINISHED)
#define SSS_MBX_STATUS_SUCCESS(wb)		\
	(((wb) & SSS_MBX_WB_STATUS_MASK) == SSS_MBX_WB_STATUS_FINISHED_SUCCESS)
#define SSS_MBX_STATUS_ERRCODE(wb)		\
	((wb) & SSS_MBX_WB_ERRCODE_MASK)

#define SSS_NO_DMA_ATTR					0

#define SSS_MBX_MSG_ID_MASK			0xF
#define SSS_MBX_MSG_ID(mbx)			((mbx)->send_msg_id)
#define SSS_INCREASE_MBX_MSG_ID(mbx)	\
	((mbx)->send_msg_id = ((mbx)->send_msg_id + 1) & SSS_MBX_MSG_ID_MASK)

#define SSS_MBX_MSG_CHN_STOP(mbx)	\
	((((mbx)->lock_channel_en) && \
	test_bit((mbx)->cur_msg_channel, &(mbx)->channel_stop)) ? true : false)

#define SSS_MBX_DMA_MSG_INIT_XOR_VAL	0x5a5a5a5a
#define SSS_MBX_XOR_DATA_ALIGN			4

#define SSS_MQ_ID_MASK(mq, id)			((id) & ((mq)->depth - 1))
#define SSS_IS_MSG_QUEUE_FULL(mq)		\
			(SSS_MQ_ID_MASK(mq, (mq)->pi + 1) == SSS_MQ_ID_MASK(mq, (mq)->ci))

#define SSS_MBX_TRY_LOCK_SLEPP_US			1000

#define SSS_FILL_MSG_HEADER(hwdev, msg_info, msg_len, mod, ack_type, type, direction, cmd) \
	(SSS_SET_MSG_HEADER((msg_len), MSG_LEN) | \
	SSS_SET_MSG_HEADER((mod), MODULE) | \
	SSS_SET_MSG_HEADER(SSS_MBX_SEG_SIZE, SEG_LEN) | \
	SSS_SET_MSG_HEADER((ack_type), NO_ACK) | \
	SSS_SET_MSG_HEADER((type), DATA_TYPE) | \
	SSS_SET_MSG_HEADER(SSS_SEQ_ID_START_VAL, SEQID) | \
	SSS_SET_MSG_HEADER(SSS_NOT_LAST_SEG, LAST) | \
	SSS_SET_MSG_HEADER((direction), DIRECTION) | \
	SSS_SET_MSG_HEADER((cmd), CMD) | \
	SSS_SET_MSG_HEADER((msg_info)->msg_id, MSG_ID) | \
	SSS_SET_MSG_HEADER((((hwdev)->poll || \
		(hwdev)->hwif->attr.aeq_num >= SSS_MGMT_RSP_MSG_AEQ) ? \
		SSS_MBX_RSP_MSG_AEQ : SSS_ASYNC_MSG_AEQ), AEQ_ID) | \
	SSS_SET_MSG_HEADER(SSS_MSG_SRC_MBX, SOURCE) | \
	SSS_SET_MSG_HEADER(!!(msg_info)->state, STATUS) | \
	SSS_SET_MSG_HEADER(sss_get_global_func_id(hwdev), SRC_GLB_FUNC_ID))

#define SSS_MBX_SEG_LEN_ALIGN	4

enum sss_msg_aeq_type {
	SSS_ASYNC_MSG_AEQ	= 0,
	/* indicate dest func or mgmt cpu which aeq to response mbx message */
	SSS_MBX_RSP_MSG_AEQ	= 1,
	/* indicate mgmt cpu which aeq to response adm message */
	SSS_MGMT_RSP_MSG_AEQ	= 2,
};

enum sss_mbx_order_type {
	SSS_MBX_STRONG_ORDER,
};

enum sss_mbx_wb_type {
	SSS_MBX_WB = 1,
};

enum sss_mbx_aeq_trig_type {
	SSS_MBX_NOT_TRIG,
};

struct sss_mbx_dma_msg {
	u32		xor;
	u32		dma_addr_h;
	u32		dma_addr_l;
	u32		msg_len;
	u64		rsvd;
};

static struct sss_msg_buffer *sss_get_msg_buffer_from_mgmt(struct sss_mbx *mbx)
{
	return &mbx->mgmt_msg;
}

static struct sss_msg_buffer *sss_get_msg_buffer_from_pf(struct sss_mbx *mbx, u64 src_func_id)
{
	struct sss_hwdev *hwdev = SSS_TO_HWDEV(mbx);

	if (src_func_id != sss_get_pf_id_of_vf(hwdev) || !mbx->func_msg)
		return NULL;

	return mbx->func_msg;
}

static struct sss_msg_buffer *sss_get_msg_buffer_from_vf(struct sss_mbx *mbx, u64 src_func_id)
{
	u16 func_id;
	struct sss_hwdev *hwdev = SSS_TO_HWDEV(mbx);

	func_id = (u16)(src_func_id - 1U) - sss_get_glb_pf_vf_offset(hwdev);
	if (func_id >= mbx->num_func_msg)
		return NULL;

	return &mbx->func_msg[func_id];
}

static struct sss_msg_buffer *sss_get_msg_buffer_from_ppf(struct sss_mbx *mbx, u64 src_func_id)
{
	u16 func_id;
	struct sss_hwdev *hwdev = SSS_TO_HWDEV(mbx);

	if (!mbx->support_h2h_msg)
		return NULL;

	for (func_id = 0; func_id < SSS_MAX_HOST_NUM(hwdev); func_id++) {
		if (src_func_id == sss_chip_get_host_ppf_id(hwdev, (u8)func_id))
			break;
	}

	if (func_id == SSS_MAX_HOST_NUM(hwdev) || !mbx->host_msg)
		return NULL;

	return &mbx->host_msg[func_id];
}

struct sss_msg_desc *sss_get_mbx_msg_desc(struct sss_mbx *mbx, u64 src_func_id, u64 direction)
{
	struct sss_hwdev *hwdev = SSS_TO_HWDEV(mbx);
	struct sss_msg_buffer *msg_buffer = NULL;

	if (src_func_id == SSS_MGMT_SRC_ID)
		msg_buffer = sss_get_msg_buffer_from_mgmt(mbx);
	else if (SSS_IS_VF(hwdev))
		msg_buffer = sss_get_msg_buffer_from_pf(mbx, src_func_id);
	else if (src_func_id > sss_get_glb_pf_vf_offset(hwdev))
		msg_buffer = sss_get_msg_buffer_from_vf(mbx, src_func_id);
	else
		msg_buffer = sss_get_msg_buffer_from_ppf(mbx, src_func_id);

	return (direction == SSS_DIRECT_SEND_MSG) ?
	       &msg_buffer->recv_msg : &msg_buffer->resp_msg;
}

static u32 sss_mbx_dma_data_xor(u32 *data, u16 data_len)
{
	u16 i;
	u16 cnt = data_len / sizeof(u32);
	u32 val = SSS_MBX_DMA_MSG_INIT_XOR_VAL;

	for (i = 0; i < cnt; i++)
		val ^= data[i];

	return val;
}

static void sss_mbx_fill_dma_msg_buf(struct sss_mbx_dma_queue *queue,
				     struct sss_mbx_dma_msg *dma_msg,
				     void *data, u16 data_len)
{
	u64 pi;
	u64 dma_paddr;
	void *dma_vaddr;

	pi = queue->pi * SSS_MBX_BUF_SIZE_MAX;
	dma_vaddr = (u8 *)queue->dma_buff_vaddr + pi;
	dma_paddr = queue->dma_buff_paddr + pi;
	memcpy(dma_vaddr, data, data_len);

	dma_msg->dma_addr_h = upper_32_bits(dma_paddr);
	dma_msg->dma_addr_l = lower_32_bits(dma_paddr);
	dma_msg->msg_len = data_len;
	dma_msg->xor = sss_mbx_dma_data_xor(dma_vaddr,
					    ALIGN(data_len, SSS_MBX_XOR_DATA_ALIGN));
}

static struct sss_mbx_dma_queue *
sss_get_mbx_dma_queue(struct sss_mbx *mbx,
		      enum sss_msg_ack_type ack_type)
{
	u32 val;
	struct sss_mbx_dma_queue *queue = NULL;

	val = sss_chip_read_reg(SSS_TO_HWDEV(mbx)->hwif, SSS_MBX_MQ_CI_OFF);
	if (ack_type == SSS_MSG_ACK) {
		queue = &mbx->sync_msg_queue;
		queue->ci = SSS_GET_MBX_MQ_CI(val, SYNC);
	} else {
		queue = &mbx->async_msg_queue;
		queue->ci = SSS_GET_MBX_MQ_CI(val, ASYNC);
	}

	if (SSS_IS_MSG_QUEUE_FULL(queue)) {
		sdk_err(SSS_TO_HWDEV(mbx)->dev_hdl, "Mbx sync mq is busy, pi: %u, ci: %u\n",
			queue->pi, SSS_MQ_ID_MASK(queue, queue->ci));
		return NULL;
	}

	return queue;
}

static void sss_fill_mbx_msg_body(struct sss_mbx_dma_queue *queue,
				  struct sss_mbx_dma_msg *dma_msg, void *msg_body, u16 body_len)
{
	sss_mbx_fill_dma_msg_buf(queue, dma_msg, msg_body, body_len);
	queue->pi = SSS_MQ_ID_MASK(queue, queue->pi + 1);
}

static void sss_clear_mbx_status(struct sss_mbx_send *send_mbx)
{
	*send_mbx->wb_state = 0;

	/* clear mbx wb state */
	wmb();
}

static void sss_chip_send_mbx_msg_header(struct sss_hwdev *hwdev,
					 struct sss_mbx_send *send_mbx, u64 *msg_header)
{
	u32 i;
	u32 *header = (u32 *)msg_header;
	u32 cnt = SSS_MBX_HEADER_SIZE / sizeof(u32);

	for (i = 0; i < cnt; i++)
		__raw_writel(cpu_to_be32(*(header + i)), send_mbx->data + i * sizeof(u32));
}

static void sss_chip_send_mbx_msg_body(struct sss_hwdev *hwdev,
				       struct sss_mbx_send *send_mbx, void *body, u16 body_len)
{
	u32 *msg_data = body;
	u32 size = sizeof(u32);
	u32 i;
	u8 buf[SSS_MBX_SEG_SIZE] = {0};
	u32 cnt = ALIGN(body_len, size) / size;

	if (body_len % size != 0) {
		memcpy(buf, body, body_len);
		msg_data = (u32 *)buf;
	}

	for (i = 0; i < cnt; i++) {
		__raw_writel(cpu_to_be32(*(msg_data + i)),
			     send_mbx->data + SSS_MBX_HEADER_SIZE + i * size);
	}
}

static void sss_chip_write_mbx_msg_attr(struct sss_mbx *mbx,
					u16 dest, u16 aeq_num, u16 seg_len)
{
	u16 size;
	u16 dest_func_id;
	u32 intr;
	u32 ctrl;

	size = ALIGN(seg_len + SSS_MBX_HEADER_SIZE, SSS_MBX_SEG_LEN_ALIGN) >> 2;
	intr = SSS_SET_MBX_INT(aeq_num, DST_AEQN) |
	       SSS_SET_MBX_INT(0, SRC_RESP_AEQN) |
	       SSS_SET_MBX_INT(SSS_NO_DMA_ATTR, STAT_DMA) |
	       SSS_SET_MBX_INT(size, TX_SIZE) |
	       SSS_SET_MBX_INT(SSS_MBX_STRONG_ORDER, STAT_DMA_SO_RO) |
	       SSS_SET_MBX_INT(SSS_MBX_WB, WB_EN);

	sss_chip_write_reg(SSS_TO_HWDEV(mbx)->hwif,
			   SSS_HW_CSR_MBX_INT_OFFSET_OFF, intr);

	/* make sure write mbx intr attr reg */
	wmb();

	dest_func_id = (SSS_IS_VF(SSS_TO_HWDEV(mbx)) && dest != SSS_MGMT_SRC_ID) ? 0 : dest;
	ctrl = SSS_SET_MBX_CTRL(SSS_MBX_TX_NOT_COMPLETE, TX_STATUS) |
	       SSS_SET_MBX_CTRL(SSS_MBX_NOT_TRIG, TRIGGER_AEQE) |
	       SSS_SET_MBX_CTRL(dest_func_id, DST_FUNC);

	sss_chip_write_reg(SSS_TO_HWDEV(mbx)->hwif,
			   SSS_HW_CSR_MBX_CTRL_OFF, ctrl);

	/* make sure write mbx ctrl reg */
	wmb();
}

static void sss_dump_mbx_reg(struct sss_hwdev *hwdev)
{
	u32 val1;
	u32 val2;

	val1 = sss_chip_read_reg(hwdev->hwif, SSS_HW_CSR_MBX_CTRL_OFF);
	val2 = sss_chip_read_reg(hwdev->hwif, SSS_HW_CSR_MBX_INT_OFFSET_OFF);

	sdk_err(hwdev->dev_hdl, "Mbx ctrl reg:0x%x, intr offset:0x%x\n", val1, val2);
}

static u16 sss_get_mbx_status(const struct sss_mbx_send *send_mbx)
{
	u64 val = be64_to_cpu(*send_mbx->wb_state);

	/* read wb state before returning it */
	rmb();

	return (u16)(val & SSS_MBX_WB_STATUS_ERRCODE_MASK);
}

static enum sss_process_ret sss_check_mbx_wb_status(void *priv_data)
{
	u16 status;
	struct sss_mbx *mbx = priv_data;

	if (SSS_MBX_MSG_CHN_STOP(mbx) || !SSS_TO_HWDEV(mbx)->chip_present_flag)
		return SSS_PROCESS_ERR;

	status = sss_get_mbx_status(&mbx->mbx_send);

	return SSS_MBX_STATUS_FINISHED(status) ? SSS_PROCESS_OK : SSS_PROCESS_DOING;
}

static int sss_chip_send_mbx_fragment(struct sss_mbx *mbx, u16 dest_func_id,
				      u64 msg_header, void *msg_body, u16 body_len)
{
	u16 aeq_type;
	u16 status = 0;
	u16 err_code;
	u16 direction;
	int ret;
	struct sss_mbx_send *send_mbx = &mbx->mbx_send;
	struct sss_hwdev *hwdev = SSS_TO_HWDEV(mbx);

	direction = SSS_GET_MSG_HEADER(msg_header, DIRECTION);
	aeq_type = (SSS_GET_HWIF_AEQ_NUM(hwdev->hwif) > SSS_MBX_RSP_MSG_AEQ &&
		    direction != SSS_DIRECT_SEND_MSG) ? SSS_MBX_RSP_MSG_AEQ : SSS_ASYNC_MSG_AEQ;

	sss_clear_mbx_status(send_mbx);

	sss_chip_send_mbx_msg_header(hwdev, send_mbx, &msg_header);

	sss_chip_send_mbx_msg_body(hwdev, send_mbx, msg_body, body_len);

	sss_chip_write_mbx_msg_attr(mbx, dest_func_id, aeq_type, body_len);

	ret = sss_check_handler_timeout(mbx, sss_check_mbx_wb_status,
					SSS_MBX_MSG_POLL_TIMEOUT_MS, USEC_PER_MSEC);
	status = sss_get_mbx_status(send_mbx);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Send mbx seg timeout, wb status: 0x%x\n", status);
		sss_dump_mbx_reg(hwdev);
		return -ETIMEDOUT;
	}

	if (!SSS_MBX_STATUS_SUCCESS(status)) {
		sdk_err(hwdev->dev_hdl, "Fail to send mbx seg to func %u, wb status: 0x%x\n",
			dest_func_id, status);
		err_code = SSS_MBX_STATUS_ERRCODE(status);
		return (err_code != 0) ? err_code : -EFAULT;
	}

	return 0;
}

static int sss_send_mbx_to_chip(struct sss_mbx *mbx, u16 dest_func_id,
				u64 msg_header, u8 *msg_body, u16 body_len)
{
	int ret;
	u16 seg_len = SSS_MBX_SEG_SIZE;
	u32 seq_id = 0;
	struct sss_hwdev *hwdev = SSS_TO_HWDEV(mbx);

	while (body_len > 0) {
		if (body_len <= SSS_MBX_SEG_SIZE) {
			msg_header &= ~SSS_MBX_SEGLEN_MASK;
			msg_header |= SSS_SET_MSG_HEADER(body_len, SEG_LEN);
			msg_header |= SSS_SET_MSG_HEADER(SSS_LAST_SEG, LAST);
			seg_len = body_len;
		}

		ret = sss_chip_send_mbx_fragment(mbx, dest_func_id, msg_header, msg_body, seg_len);
		if (ret != 0) {
			sdk_err(hwdev->dev_hdl, "Fail to send mbx seg, seq_id=0x%llx\n",
				SSS_GET_MSG_HEADER(msg_header, SEQID));
			return ret;
		}

		seq_id++;
		msg_body += seg_len;
		body_len -= seg_len;
		msg_header &= ~(SSS_SET_MSG_HEADER(SSS_MSG_HEADER_SEQID_MASK, SEQID));
		msg_header |= SSS_SET_MSG_HEADER(seq_id, SEQID);
	}

	return 0;
}

int sss_send_mbx_msg(struct sss_mbx *mbx, u8 mod, u16 cmd, void *msg,
		     u16 msg_len, u16 dest_func_id, enum sss_msg_direction_type direction,
		     enum sss_msg_ack_type ack_type, struct sss_mbx_msg_info *msg_info)
{
	u8 *msg_body = NULL;
	u64 msg_header = 0;
	int ret = 0;
	struct sss_hwdev *hwdev = SSS_TO_HWDEV(mbx);
	struct sss_mbx_dma_msg msg_dma = {0};
	enum sss_data_type type = SSS_INLINE_DATA;
	struct sss_mbx_dma_queue *queue = NULL;

	mutex_lock(&mbx->msg_send_lock);

	if (SSS_IS_DMA_MBX_MSG(dest_func_id) && !SSS_SUPPORT_MBX_SEGMENT(hwdev)) {
		queue = sss_get_mbx_dma_queue(mbx, ack_type);
		if (!queue) {
			ret = -EBUSY;
			goto out;
		}

		sss_fill_mbx_msg_body(queue, &msg_dma, msg, msg_len);

		type = SSS_DMA_DATA;
		msg = &msg_dma;
		msg_len = sizeof(msg_dma);
	}

	msg_body = (u8 *)msg;
	msg_header = SSS_FILL_MSG_HEADER(hwdev, msg_info, msg_len, mod,
					 ack_type, type, direction, cmd);

	ret = sss_send_mbx_to_chip(mbx, dest_func_id, msg_header, msg_body, msg_len);

out:
	mutex_unlock(&mbx->msg_send_lock);

	return ret;
}

static void sss_set_mbx_event_flag(struct sss_mbx *mbx,
				   enum sss_mbx_event_state event_flag)
{
	spin_lock(&mbx->mbx_lock);
	mbx->event_flag = event_flag;
	spin_unlock(&mbx->mbx_lock);
}

static enum sss_process_ret check_mbx_msg_finish(void *priv_data)
{
	struct sss_mbx *mbx = priv_data;

	if (SSS_MBX_MSG_CHN_STOP(mbx) || SSS_TO_HWDEV(mbx)->chip_present_flag == 0)
		return SSS_PROCESS_ERR;

	return (mbx->event_flag == SSS_EVENT_SUCCESS) ? SSS_PROCESS_OK : SSS_PROCESS_DOING;
}

static int sss_wait_mbx_msg_completion(struct sss_mbx *mbx, u32 timeout)
{
	u32 wait_time;
	int ret;

	wait_time = (timeout != 0) ? timeout : SSS_MBX_COMPLETE_WAIT_TIME_MS;
	ret = sss_check_handler_timeout(mbx, check_mbx_msg_finish,
					wait_time, USEC_PER_MSEC);
	if (ret != 0) {
		sss_set_mbx_event_flag(mbx, SSS_EVENT_TIMEOUT);
		return -ETIMEDOUT;
	}

	sss_set_mbx_event_flag(mbx, SSS_EVENT_END);

	return 0;
}

static int sss_send_mbx_msg_lock(struct sss_mbx *mbx, u16 channel)
{
	if (!mbx->lock_channel_en) {
		mutex_lock(&mbx->mbx_send_lock);
		return 0;
	}

	while (test_bit(channel, &mbx->channel_stop) == 0) {
		if (mutex_trylock(&mbx->mbx_send_lock) != 0)
			return 0;

		usleep_range(SSS_MBX_TRY_LOCK_SLEPP_US - 1, SSS_MBX_TRY_LOCK_SLEPP_US);
	}

	return -EAGAIN;
}

static void sss_send_mbx_msg_unlock(struct sss_mbx *mbx)
{
	mutex_unlock(&mbx->mbx_send_lock);
}

int sss_send_mbx_to_func(struct sss_mbx *mbx, u8 mod, u16 cmd,
			 u16 dest_func_id, void *buf_in, u16 in_size, void *buf_out,
			 u16 *out_size, u32 timeout, u16 channel)
{
	struct sss_msg_desc *msg_desc = NULL;
	struct sss_mbx_msg_info msg_info = {0};
	int ret;

	if (SSS_TO_HWDEV(mbx)->chip_present_flag == 0)
		return -EPERM;

	msg_desc = sss_get_mbx_msg_desc(mbx, dest_func_id, SSS_RESP_MSG);
	if (!msg_desc)
		return -EFAULT;

	ret = sss_send_mbx_msg_lock(mbx, channel);
	if (ret != 0)
		return ret;

	mbx->cur_msg_channel = channel;
	SSS_INCREASE_MBX_MSG_ID(mbx);
	sss_set_mbx_event_flag(mbx, SSS_EVENT_START);

	msg_info.msg_id = SSS_MBX_MSG_ID(mbx);
	ret = sss_send_mbx_msg(mbx, mod, cmd, buf_in, in_size, dest_func_id,
			       SSS_DIRECT_SEND_MSG, SSS_MSG_ACK, &msg_info);
	if (ret != 0) {
		sdk_err(SSS_TO_HWDEV(mbx)->dev_hdl,
			"Fail to send mbx mod %u, cmd %u, msg_id: %u, err: %d\n",
			mod, cmd, msg_info.msg_id, ret);
		sss_set_mbx_event_flag(mbx, SSS_EVENT_FAIL);
		goto send_err;
	}

	if (sss_wait_mbx_msg_completion(mbx, timeout)) {
		sdk_err(SSS_TO_HWDEV(mbx)->dev_hdl,
			"Send mbx msg timeout, msg_id: %u\n", msg_info.msg_id);
		sss_dump_aeq_info(SSS_TO_HWDEV(mbx));
		ret = -ETIMEDOUT;
		goto send_err;
	}

	if (mod != msg_desc->mod || cmd != msg_desc->cmd) {
		sdk_err(SSS_TO_HWDEV(mbx)->dev_hdl,
			"Invalid response mbx message, mod: 0x%x, cmd: 0x%x, expect mod: 0x%x, cmd: 0x%x\n",
			msg_desc->mod, msg_desc->cmd, mod, cmd);
		ret = -EFAULT;
		goto send_err;
	}

	if (msg_desc->msg_info.state) {
		ret = msg_desc->msg_info.state;
		goto send_err;
	}

	if (buf_out && out_size) {
		if (*out_size < msg_desc->msg_len) {
			sdk_err(SSS_TO_HWDEV(mbx)->dev_hdl,
				"Invalid response mbx message length: %u for mod %d cmd %u, should less than: %u\n",
				msg_desc->msg_len, mod, cmd, *out_size);
			ret = -EFAULT;
			goto send_err;
		}

		if (msg_desc->msg_len)
			memcpy(buf_out, msg_desc->msg, msg_desc->msg_len);

		*out_size = msg_desc->msg_len;
	}

send_err:
	sss_send_mbx_msg_unlock(mbx);

	return ret;
}

int sss_send_mbx_to_func_no_ack(struct sss_hwdev *hwdev, u16 func_id,
				u8 mod, u16 cmd, void *buf_in, u16 in_size, u16 channel)
{
	struct sss_mbx_msg_info msg_info = {0};
	int ret;

	ret = sss_check_mbx_param(hwdev->mbx, buf_in, in_size, channel);
	if (ret != 0)
		return ret;

	ret = sss_send_mbx_msg_lock(hwdev->mbx, channel);
	if (ret != 0)
		return ret;

	ret = sss_send_mbx_msg(hwdev->mbx, mod, cmd, buf_in, in_size,
			       func_id, SSS_DIRECT_SEND_MSG, SSS_MSG_NO_ACK, &msg_info);
	if (ret != 0)
		sdk_err(hwdev->dev_hdl, "Fail to send mbx no ack\n");

	sss_send_mbx_msg_unlock(hwdev->mbx);

	return ret;
}

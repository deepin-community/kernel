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
#include "sss_csr.h"
#include "sss_common.h"
#include "sss_adapter_mgmt.h"

#define SSS_MBX_WB_STATUS_SIZE			16UL

#define SSS_MBX_DMA_MSG_QUEUE_DEPTH	32

#define SSS_MBX_WQ_NAME				"sss_mbx"

#define SSS_MBX_AREA(hwif)				\
	((hwif)->cfg_reg_base + SSS_HW_CSR_MBX_DATA_OFF)

#define SSS_GET_MBX_BODY(header)		((u8 *)(header) + SSS_MBX_HEADER_SIZE)

#define SSS_MBX_LAST_SEG_MAX_SIZE		\
			(SSS_MBX_BUF_SIZE_MAX - SSS_MAX_SEG_ID * SSS_MBX_SEG_SIZE)

#define SSS_MSG_PROCESS_CNT_MAX		10

#define SSS_SRC_IS_PF_OR_PPF(hwdev, src_func_id)	\
	((src_func_id) < SSS_MAX_PF_NUM(hwdev))

#define SSS_MBX_MSG_NO_DATA_SIZE		1

#define SSS_MBX_PF_SEND_ERR			0x1

#define SSS_MAX_SEG_ID					42

struct sss_mbx_work {
	struct work_struct		work;
	struct sss_mbx			*mbx;
	struct sss_recv_mbx	*recv_mbx;
	struct sss_msg_buffer	*msg_buffer;
};

static int sss_alloc_mbx_mq_dma_buf(struct sss_hwdev *hwdev, struct sss_mbx_dma_queue *mq)
{
	u32 size;

	size = mq->depth * SSS_MBX_BUF_SIZE_MAX;
	mq->dma_buff_vaddr = dma_zalloc_coherent(hwdev->dev_hdl, size, &mq->dma_buff_paddr,
						 GFP_KERNEL);
	if (!mq->dma_buff_vaddr) {
		sdk_err(hwdev->dev_hdl, "Fail to alloc dma_buffer\n");
		return -ENOMEM;
	}

	return 0;
}

static void sss_free_mbx_mq_dma_buf(struct sss_hwdev *hwdev, struct sss_mbx_dma_queue *mq)
{
	dma_free_coherent(hwdev->dev_hdl, mq->depth * SSS_MBX_BUF_SIZE_MAX,
			  mq->dma_buff_vaddr, mq->dma_buff_paddr);
	mq->dma_buff_vaddr = NULL;
	mq->dma_buff_paddr = 0;
}

static int sss_mbx_alloc_mq_dma_addr(struct sss_mbx *mbx)
{
	int ret;

	ret = sss_alloc_mbx_mq_dma_buf(SSS_TO_HWDEV(mbx), &mbx->sync_msg_queue);
	if (ret != 0)
		return ret;

	ret = sss_alloc_mbx_mq_dma_buf(SSS_TO_HWDEV(mbx), &mbx->async_msg_queue);
	if (ret != 0) {
		sss_free_mbx_mq_dma_buf(SSS_TO_HWDEV(mbx), &mbx->sync_msg_queue);
		return ret;
	}

	return 0;
}

static void sss_mbx_free_mq_dma_addr(struct sss_mbx *mbx)
{
	sss_free_mbx_mq_dma_buf(SSS_TO_HWDEV(mbx), &mbx->sync_msg_queue);
	sss_free_mbx_mq_dma_buf(SSS_TO_HWDEV(mbx), &mbx->async_msg_queue);
}

static int sss_mbx_alloc_mq_wb_addr(struct sss_mbx *mbx)
{
	struct sss_mbx_send *send_mbx = &mbx->mbx_send;
	struct sss_hwdev *hwdev = SSS_TO_HWDEV(mbx);

	send_mbx->wb_vaddr = dma_zalloc_coherent(hwdev->dev_hdl, SSS_MBX_WB_STATUS_SIZE,
						 &send_mbx->wb_paddr, GFP_KERNEL);
	if (!send_mbx->wb_vaddr)
		return -ENOMEM;

	send_mbx->wb_state = send_mbx->wb_vaddr;

	return 0;
}

static void sss_mbx_free_mq_wb_addr(struct sss_mbx *mbx)
{
	struct sss_mbx_send *send_mbx = &mbx->mbx_send;
	struct sss_hwdev *hwdev = SSS_TO_HWDEV(mbx);

	dma_free_coherent(hwdev->dev_hdl, SSS_MBX_WB_STATUS_SIZE,
			  send_mbx->wb_vaddr, send_mbx->wb_paddr);

	send_mbx->wb_vaddr = NULL;
}

static int sss_alloc_mbx_msg_buffer(struct sss_msg_buffer *msg_buffer)
{
	msg_buffer->resp_msg.msg = kzalloc(SSS_MBX_BUF_SIZE_MAX, GFP_KERNEL);
	if (!msg_buffer->resp_msg.msg)
		return -ENOMEM;

	msg_buffer->recv_msg.msg = kzalloc(SSS_MBX_BUF_SIZE_MAX, GFP_KERNEL);
	if (!msg_buffer->recv_msg.msg) {
		kfree(msg_buffer->resp_msg.msg);
		msg_buffer->resp_msg.msg = NULL;
		return -ENOMEM;
	}

	atomic_set(&msg_buffer->recv_msg_cnt, 0);
	msg_buffer->recv_msg.seq_id = SSS_MAX_SEG_ID;
	msg_buffer->resp_msg.seq_id = SSS_MAX_SEG_ID;

	return 0;
}

static void sss_free_mbx_msg_buffer(struct sss_msg_buffer *msg_buffer)
{
	kfree(msg_buffer->recv_msg.msg);
	msg_buffer->recv_msg.msg = NULL;
	kfree(msg_buffer->resp_msg.msg);
	msg_buffer->resp_msg.msg = NULL;
}

static int sss_mbx_alloc_dma_addr(struct sss_mbx *sss_mbx)
{
	int ret;

	ret = sss_mbx_alloc_mq_dma_addr(sss_mbx);
	if (ret != 0) {
		sdk_err(SSS_TO_HWDEV(sss_mbx)->dev_hdl, "Fail to alloc mbx dma queue\n");
		return -ENOMEM;
	}

	ret = sss_mbx_alloc_mq_wb_addr(sss_mbx);
	if (ret != 0) {
		sdk_err(SSS_TO_HWDEV(sss_mbx)->dev_hdl, "Fail to init mbx dma wb addr\n");
		goto alloc_dma_wb_addr_err;
	}

	return 0;

alloc_dma_wb_addr_err:
	sss_mbx_free_mq_dma_addr(sss_mbx);

	return -ENOMEM;
}

static void sss_mbx_free_dma_addr(struct sss_mbx *mbx)
{
	sss_mbx_free_mq_wb_addr(mbx);
	sss_mbx_free_mq_dma_addr(mbx);
}

static int sss_init_mbx_info(struct sss_mbx *mbx)
{
	int ret;

	mutex_init(&mbx->mbx_send_lock);
	mutex_init(&mbx->msg_send_lock);
	spin_lock_init(&mbx->mbx_lock);
	mbx->sync_msg_queue.depth = SSS_MBX_DMA_MSG_QUEUE_DEPTH;
	mbx->async_msg_queue.depth = SSS_MBX_DMA_MSG_QUEUE_DEPTH;

	mbx->workq = create_singlethread_workqueue(SSS_MBX_WQ_NAME);
	if (!mbx->workq) {
		sdk_err(SSS_TO_HWDEV(mbx)->dev_hdl, "Fail to create mbx workq\n");
		return -ENOMEM;
	}

	ret = sss_alloc_mbx_msg_buffer(&mbx->mgmt_msg);
	if (ret != 0) {
		sdk_err(SSS_TO_HWDEV(mbx)->dev_hdl, "Fail to alloc mgmt message buffer\n");
		goto alloc_mbx_msg_buffer_err;
	}

	ret = sss_mbx_alloc_dma_addr(mbx);
	if (ret != 0) {
		sdk_err(SSS_TO_HWDEV(mbx)->dev_hdl, "Fail to alloc dma addr\n");
		goto mbx_alloc_dma_addr_err;
	}

	return 0;

mbx_alloc_dma_addr_err:
	sss_free_mbx_msg_buffer(&mbx->mgmt_msg);
alloc_mbx_msg_buffer_err:
	destroy_workqueue(mbx->workq);

	return -ENOMEM;
}

static void sss_deinit_mbx_info(struct sss_mbx *mbx)
{
	if (mbx->workq) {
		destroy_workqueue(mbx->workq);
		mbx->workq = NULL;
	}

	sss_mbx_free_dma_addr(mbx);
	sss_free_mbx_msg_buffer(&mbx->mgmt_msg);
}

static int sss_alloc_func_mbx_msg(struct sss_mbx *mbx, u16 func_num)
{
	if (mbx->func_msg)
		return (mbx->num_func_msg == func_num) ? 0 : -EFAULT;

	mbx->func_msg = kcalloc(func_num, sizeof(*mbx->func_msg), GFP_KERNEL);
	if (!mbx->func_msg)
		return -ENOMEM;

	return 0;
}

static void sss_free_func_mbx_msg(struct sss_mbx *mbx)
{
	kfree(mbx->func_msg);
	mbx->func_msg = NULL;
}

int sss_init_func_mbx_msg(void *hwdev, u16 func_num)
{
	u16 i;
	u16 cnt;
	int ret;
	struct sss_hwdev *dev = hwdev;
	struct sss_mbx *mbx = dev->mbx;

	if (!hwdev || func_num == 0 || func_num > SSS_MAX_FUNC)
		return -EINVAL;

	ret = sss_alloc_func_mbx_msg(mbx, func_num);
	if (ret != 0) {
		sdk_err(dev->dev_hdl, "Fail to alloc func msg\n");
		return ret;
	}

	for (cnt = 0; cnt < func_num; cnt++) {
		ret = sss_alloc_mbx_msg_buffer(&mbx->func_msg[cnt]);
		if (ret != 0) {
			sdk_err(dev->dev_hdl, "Fail to alloc func %u msg buf\n", cnt);
			goto alloc_mbx_msg_buf_err;
		}
	}

	mbx->num_func_msg = func_num;

	return 0;

alloc_mbx_msg_buf_err:
	for (i = 0; i < cnt; i++)
		sss_free_mbx_msg_buffer(&mbx->func_msg[i]);

	sss_free_func_mbx_msg(mbx);

	return -ENOMEM;
}

static void sss_deinit_func_mbx_msg(struct sss_mbx *mbx)
{
	u16 i;

	if (!mbx->func_msg)
		return;

	for (i = 0; i < mbx->num_func_msg; i++)
		sss_free_mbx_msg_buffer(&mbx->func_msg[i]);

	sss_free_func_mbx_msg(mbx);
}

static void sss_chip_reset_mbx_ci(struct sss_mbx *mbx)
{
	u32 val;

	val = sss_chip_read_reg(SSS_TO_HWDEV(mbx)->hwif, SSS_MBX_MQ_CI_OFF);
	val = SSS_CLEAR_MBX_MQ_CI(val, SYNC);
	val = SSS_CLEAR_MBX_MQ_CI(val, ASYNC);

	sss_chip_write_reg(SSS_TO_HWDEV(mbx)->hwif, SSS_MBX_MQ_CI_OFF, val);
}

static void sss_chip_set_mbx_wb_attr(struct sss_mbx *mbx)
{
	u32 addr_h;
	u32 addr_l;
	struct sss_mbx_send *send_mbx = &mbx->mbx_send;
	struct sss_hwdev *hwdev = SSS_TO_HWDEV(mbx);

	addr_h = upper_32_bits(send_mbx->wb_paddr);
	addr_l = lower_32_bits(send_mbx->wb_paddr);

	sss_chip_write_reg(hwdev->hwif, SSS_HW_CSR_MBX_RES_H_OFF, addr_h);
	sss_chip_write_reg(hwdev->hwif, SSS_HW_CSR_MBX_RES_L_OFF, addr_l);
}

static void sss_chip_set_mbx_attr(struct sss_mbx *mbx)
{
	sss_chip_reset_mbx_ci(mbx);
	sss_chip_set_mbx_wb_attr(mbx);
}

static void sss_chip_reset_mbx_attr(struct sss_mbx *sss_mbx)
{
	struct sss_hwdev *hwdev = SSS_TO_HWDEV(sss_mbx);

	sss_chip_write_reg(hwdev->hwif, SSS_HW_CSR_MBX_RES_H_OFF, 0);
	sss_chip_write_reg(hwdev->hwif, SSS_HW_CSR_MBX_RES_L_OFF, 0);
}

static void sss_prepare_send_mbx(struct sss_mbx *mbx)
{
	struct sss_mbx_send *send_mbx = &mbx->mbx_send;

	send_mbx->data = SSS_MBX_AREA(SSS_TO_HWDEV(mbx)->hwif);
}

static int sss_alloc_host_msg(struct sss_hwdev *hwdev)
{
	int i;
	int ret;
	int host_id;
	u8 max_host = SSS_MAX_HOST_NUM(hwdev);
	struct sss_mbx *mbx = hwdev->mbx;

	if (max_host == 0)
		return 0;

	mbx->host_msg = kcalloc(max_host, sizeof(*mbx->host_msg), GFP_KERNEL);
	if (!mbx->host_msg)
		return -ENOMEM;

	for (host_id = 0; host_id < max_host; host_id++) {
		ret = sss_alloc_mbx_msg_buffer(&mbx->host_msg[host_id]);
		if (ret != 0) {
			sdk_err(SSS_TO_HWDEV(mbx)->dev_hdl,
				"Fail to alloc host %d msg channel\n", host_id);
			goto out;
		}
	}

	mbx->support_h2h_msg = true;

	return 0;

out:
	for (i = 0; i < host_id; i++)
		sss_free_mbx_msg_buffer(&mbx->host_msg[i]);

	kfree(mbx->host_msg);
	mbx->host_msg = NULL;

	return -ENOMEM;
}

static void sss_free_host_msg(struct sss_mbx *mbx)
{
	int i;

	if (!mbx->host_msg)
		return;

	for (i = 0; i < SSS_MAX_HOST_NUM(SSS_TO_HWDEV(mbx)); i++)
		sss_free_mbx_msg_buffer(&mbx->host_msg[i]);

	kfree(mbx->host_msg);
	mbx->host_msg = NULL;
}

int sss_hwif_init_mbx(struct sss_hwdev *hwdev)
{
	int ret;
	struct sss_mbx *mbx;

	mbx = kzalloc(sizeof(*mbx), GFP_KERNEL);
	if (!mbx)
		return -ENOMEM;

	hwdev->mbx = mbx;
	mbx->hwdev = hwdev;

	ret = sss_init_mbx_info(mbx);
	if (ret != 0)
		goto init_mbx_info_err;

	if (SSS_IS_VF(hwdev)) {
		ret = sss_init_func_mbx_msg(hwdev, 1);
		if (ret != 0)
			goto init_func_mbx_msg_err;
	}

	sss_chip_set_mbx_attr(mbx);

	sss_prepare_send_mbx(mbx);

	ret = sss_alloc_host_msg(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to alloc host msg\n");
		goto alloc_host_msg_err;
	}

	return 0;

alloc_host_msg_err:
	sss_chip_reset_mbx_attr(mbx);
	sss_deinit_func_mbx_msg(mbx);

init_func_mbx_msg_err:
	sss_deinit_mbx_info(mbx);

init_mbx_info_err:
	kfree(mbx);
	hwdev->mbx = NULL;

	return ret;
}

void sss_hwif_deinit_mbx(struct sss_hwdev *hwdev)
{
	struct sss_mbx *mbx = hwdev->mbx;

	destroy_workqueue(mbx->workq);
	mbx->workq = NULL;

	sss_chip_reset_mbx_attr(mbx);

	sss_free_host_msg(mbx);

	sss_deinit_func_mbx_msg(mbx);

	sss_deinit_mbx_info(mbx);

	kfree(mbx);
	hwdev->mbx = NULL;
}

static bool sss_check_mbx_msg_header(void *dev_hdl,
				     struct sss_msg_desc *msg_desc, u64 mbx_header)
{
	u8 seq_id = SSS_GET_MSG_HEADER(mbx_header, SEQID);
	u8 seg_len = SSS_GET_MSG_HEADER(mbx_header, SEG_LEN);
	u8 msg_id = SSS_GET_MSG_HEADER(mbx_header, MSG_ID);
	u8 mod = SSS_GET_MSG_HEADER(mbx_header, MODULE);
	u16 cmd = SSS_GET_MSG_HEADER(mbx_header, CMD);

	if (seq_id > SSS_MAX_SEG_ID) {
		sdk_err(dev_hdl, "Current seg info: seq_id = 0x%x\n", seq_id);
		return false;
	}

	if (seg_len > SSS_MBX_SEG_SIZE) {
		sdk_err(dev_hdl, "Current seg info: seg_len = 0x%x\n", seg_len);
		return false;
	}

	if (seq_id == SSS_MAX_SEG_ID && seg_len > SSS_MBX_LAST_SEG_MAX_SIZE) {
		sdk_err(dev_hdl, "Current seg info: seq_id = 0x%x, seg_len = 0x%x\n",
			seq_id, seg_len);
		return false;
	}

	if (seq_id == 0)
		return true;

	if (seq_id != msg_desc->seq_id + 1) {
		sdk_err(dev_hdl, "Current seg info: seq_id = 0x%x, 0x%x\n",
			seq_id, msg_desc->seq_id);
		return false;
	}

	if (msg_id != msg_desc->msg_info.msg_id) {
		sdk_err(dev_hdl, "Current seg info: msg_id = 0x%x, 0x%x\n",
			msg_id, msg_desc->msg_info.msg_id);
		return false;
	}

	if (mod != msg_desc->mod) {
		sdk_err(dev_hdl, "Current seg info: mod = 0x%x, 0x%x\n",
			mod, msg_desc->mod);
		return false;
	}

	if (cmd != msg_desc->cmd) {
		sdk_err(dev_hdl, "Current seg info: cmd = 0x%x, 0x%x\n",
			cmd, msg_desc->cmd);
		return false;
	}

	return true;
}

static void sss_fill_msg_desc(struct sss_msg_desc *msg_desc, u64 *msg_header)
{
	u64 mbx_header = *msg_header;
	u8 seq_id = SSS_GET_MSG_HEADER(mbx_header, SEQID);
	u8 seg_len = SSS_GET_MSG_HEADER(mbx_header, SEG_LEN);
	u8 msg_id = SSS_GET_MSG_HEADER(mbx_header, MSG_ID);
	u8 mod = SSS_GET_MSG_HEADER(mbx_header, MODULE);
	u16 cmd = SSS_GET_MSG_HEADER(mbx_header, CMD);
	u32 offset = seq_id * SSS_MBX_SEG_SIZE;
	void *msg_body = SSS_GET_MBX_BODY(((void *)msg_header));

	msg_desc->seq_id = seq_id;
	if (seq_id == 0) {
		msg_desc->msg_info.msg_id = msg_id;
		msg_desc->mod = mod;
		msg_desc->cmd = cmd;
	}
	msg_desc->msg_len = SSS_GET_MSG_HEADER(mbx_header, MSG_LEN);
	msg_desc->msg_info.state = SSS_GET_MSG_HEADER(mbx_header, STATUS);
	memcpy((u8 *)msg_desc->msg + offset, msg_body, seg_len);
}

static struct sss_recv_mbx *sss_alloc_recv_mbx(void)
{
	struct sss_recv_mbx *recv_mbx = NULL;

	recv_mbx = kzalloc(sizeof(*recv_mbx), GFP_KERNEL);
	if (!recv_mbx)
		return NULL;

	recv_mbx->buf = kzalloc(SSS_MBX_BUF_SIZE_MAX, GFP_KERNEL);
	if (!recv_mbx->buf)
		goto alloc_recv_mbx_buf_err;

	recv_mbx->resp_buf = kzalloc(SSS_MBX_BUF_SIZE_MAX, GFP_KERNEL);
	if (!recv_mbx->resp_buf)
		goto alloc_recv_mbx_resp_buf_err;

	return recv_mbx;

alloc_recv_mbx_resp_buf_err:
	kfree(recv_mbx->buf);

alloc_recv_mbx_buf_err:
	kfree(recv_mbx);

	return NULL;
}

static void sss_free_recv_mbx(struct sss_recv_mbx *recv_mbx)
{
	kfree(recv_mbx->resp_buf);
	kfree(recv_mbx->buf);
	kfree(recv_mbx);
}

static int sss_recv_vf_mbx_handler(struct sss_mbx *mbx,
				   struct sss_recv_mbx *recv_mbx, void *resp_buf, u16 *size)
{
	int ret;
	sss_vf_mbx_handler_t callback;
	struct sss_hwdev *hwdev = SSS_TO_HWDEV(mbx);

	if (recv_mbx->mod >= SSS_MOD_TYPE_MAX) {
		sdk_warn(hwdev->dev_hdl, "Recv err mbx msg, mod = %u\n", recv_mbx->mod);
		return -EINVAL;
	}

	set_bit(SSS_VF_RECV_HANDLER_RUN, &mbx->vf_mbx_cb_state[recv_mbx->mod]);

	callback = mbx->vf_mbx_cb[recv_mbx->mod];
	if (callback &&
	    test_bit(SSS_VF_RECV_HANDLER_REG, &mbx->vf_mbx_cb_state[recv_mbx->mod])) {
		ret = callback(mbx->vf_mbx_data[recv_mbx->mod], recv_mbx->cmd, recv_mbx->buf,
			       recv_mbx->buf_len, resp_buf, size);
	} else {
		sdk_warn(hwdev->dev_hdl, "VF mbx cb is unregistered\n");
		ret = -EINVAL;
	}

	clear_bit(SSS_VF_RECV_HANDLER_RUN, &mbx->vf_mbx_cb_state[recv_mbx->mod]);

	return ret;
}

static int sss_recv_pf_from_ppf_handler(struct sss_mbx *mbx,
					struct sss_recv_mbx *recv_mbx, void *resp_buf, u16 *size)
{
	int ret;
	sss_pf_from_ppf_mbx_handler_t callback;
	enum sss_mod_type mod = recv_mbx->mod;
	struct sss_hwdev *hwdev = SSS_TO_HWDEV(mbx);

	if (mod >= SSS_MOD_TYPE_MAX) {
		sdk_warn(hwdev->dev_hdl, "Recv err mbx msg, mod = %d\n", mod);
		return -EINVAL;
	}

	set_bit(SSS_PPF_TO_PF_RECV_HANDLER_RUN, &mbx->ppf_to_pf_mbx_cb_state[mod]);

	callback = mbx->pf_recv_ppf_mbx_cb[mod];
	if (callback &&
	    test_bit(SSS_PPF_TO_PF_RECV_HANDLER_REG, &mbx->ppf_to_pf_mbx_cb_state[mod]) != 0) {
		ret = callback(mbx->pf_recv_ppf_mbx_data[mod], recv_mbx->cmd,
			       recv_mbx->buf, recv_mbx->buf_len, resp_buf, size);
	} else {
		sdk_warn(hwdev->dev_hdl, "PF recv ppf mbx cb is not registered\n");
		ret = -EINVAL;
	}

	clear_bit(SSS_PPF_TO_PF_RECV_HANDLER_RUN, &mbx->ppf_to_pf_mbx_cb_state[mod]);

	return ret;
}

static int sss_recv_ppf_mbx_handler(struct sss_mbx *mbx,
				    struct sss_recv_mbx *recv_mbx, u8 pf_id,
				    void *resp_buf, u16 *size)
{
	int ret;
	u16 vf_id = 0;
	sss_ppf_mbx_handler_t callback;
	struct sss_hwdev *hwdev = SSS_TO_HWDEV(mbx);

	if (recv_mbx->mod >= SSS_MOD_TYPE_MAX) {
		sdk_warn(hwdev->dev_hdl, "Recv err mbx msg, mod = %u\n", recv_mbx->mod);
		return -EINVAL;
	}

	set_bit(SSS_PPF_RECV_HANDLER_RUN, &mbx->ppf_mbx_cb_state[recv_mbx->mod]);

	callback = mbx->ppf_mbx_cb[recv_mbx->mod];
	if (callback &&
	    test_bit(SSS_PPF_RECV_HANDLER_REG, &mbx->ppf_mbx_cb_state[recv_mbx->mod])) {
		ret = callback(mbx->ppf_mbx_data[recv_mbx->mod], pf_id, vf_id, recv_mbx->cmd,
			       recv_mbx->buf, recv_mbx->buf_len, resp_buf, size);
	} else {
		sdk_warn(hwdev->dev_hdl, "PPF mbx cb is unregistered, mod = %u\n", recv_mbx->mod);
		ret = -EINVAL;
	}

	clear_bit(SSS_PPF_RECV_HANDLER_RUN, &mbx->ppf_mbx_cb_state[recv_mbx->mod]);

	return ret;
}

static int sss_recv_pf_from_vf_mbx_handler(struct sss_mbx *mbx,
					   struct sss_recv_mbx *recv_mbx,
					   u16 src_func_id, void *resp_buf,
					   u16 *size)
{
	int ret;
	u16 vf_id = 0;
	sss_pf_mbx_handler_t callback;
	struct sss_hwdev *hwdev = SSS_TO_HWDEV(mbx);

	if (recv_mbx->mod >= SSS_MOD_TYPE_MAX) {
		sdk_warn(hwdev->dev_hdl, "Recv err mbx msg, mod = %u\n", recv_mbx->mod);
		return -EINVAL;
	}

	set_bit(SSS_PF_RECV_HANDLER_RUN, &mbx->pf_mbx_cb_state[recv_mbx->mod]);

	callback = mbx->pf_mbx_cb[recv_mbx->mod];
	if (callback &&
	    test_bit(SSS_PF_RECV_HANDLER_REG, &mbx->pf_mbx_cb_state[recv_mbx->mod]) != 0) {
		vf_id = src_func_id - sss_get_glb_pf_vf_offset(SSS_TO_HWDEV(mbx));
		ret = callback(mbx->pf_mbx_data[recv_mbx->mod], vf_id, recv_mbx->cmd,
			       recv_mbx->buf, recv_mbx->buf_len, resp_buf, size);
	} else {
		sdk_warn(hwdev->dev_hdl, "PF mbx mod(0x%x) cb is unregistered\n", recv_mbx->mod);
		ret = -EINVAL;
	}

	clear_bit(SSS_PF_RECV_HANDLER_RUN, &mbx->pf_mbx_cb_state[recv_mbx->mod]);

	return ret;
}

static void sss_send_mbx_response(struct sss_mbx *mbx,
				  struct sss_recv_mbx *recv_mbx, int ret, u16 size, u16 src_func_id)
{
	u16 data_size;
	struct sss_mbx_msg_info msg_info = {0};
	struct sss_hwdev *hwdev = SSS_TO_HWDEV(mbx);

	msg_info.msg_id = recv_mbx->msg_id;
	if (ret != 0)
		msg_info.state = SSS_MBX_PF_SEND_ERR;

	data_size = (size == 0 || ret != 0) ? SSS_MBX_MSG_NO_DATA_SIZE : size;
	if (data_size > SSS_MBX_DATA_SIZE) {
		sdk_err(hwdev->dev_hdl, "Resp msg len(%d), out of range: %d\n",
			data_size, SSS_MBX_DATA_SIZE);
		data_size = SSS_MBX_DATA_SIZE;
	}

	sss_send_mbx_msg(mbx, recv_mbx->mod, recv_mbx->cmd, recv_mbx->resp_buf, data_size,
			 src_func_id, SSS_RESP_MSG, SSS_MSG_NO_ACK, &msg_info);
}

static void sss_recv_mbx_handler(struct sss_mbx *mbx,
				 struct sss_recv_mbx *recv_mbx)
{
	int ret = 0;
	void *resp_buf = recv_mbx->resp_buf;
	u16 size = SSS_MBX_DATA_SIZE;
	u16 src_func_id = recv_mbx->src_func_id;
	struct sss_hwdev *hwdev = SSS_TO_HWDEV(mbx);

	if (SSS_IS_VF(hwdev)) {
		ret = sss_recv_vf_mbx_handler(mbx, recv_mbx, resp_buf, &size);
		goto out;
	}

	if (SSS_SRC_IS_PF_OR_PPF(hwdev, src_func_id)) {
		if (SSS_IS_PPF(hwdev))
			ret = sss_recv_ppf_mbx_handler(mbx, recv_mbx,
						       (u8)src_func_id,
						       resp_buf, &size);
		else
			ret = sss_recv_pf_from_ppf_handler(mbx, recv_mbx, resp_buf, &size);
	} else {
		ret = sss_recv_pf_from_vf_mbx_handler(mbx,
						      recv_mbx, src_func_id,
						      resp_buf, &size);
	}

out:
	if (recv_mbx->ack_type == SSS_MSG_ACK)
		sss_send_mbx_response(mbx, recv_mbx, ret, size, src_func_id);
}

static void sss_recv_mbx_work_handler(struct work_struct *work)
{
	struct sss_mbx_work *mbx_work = container_of(work, struct sss_mbx_work, work);

	sss_recv_mbx_handler(mbx_work->mbx, mbx_work->recv_mbx);

	atomic_dec(&mbx_work->msg_buffer->recv_msg_cnt);

	destroy_work(&mbx_work->work);

	sss_free_recv_mbx(mbx_work->recv_mbx);

	kfree(mbx_work);
}

static void sss_init_recv_mbx_param(struct sss_recv_mbx *recv_mbx,
				    struct sss_msg_desc *msg_desc, u64 msg_header)
{
	recv_mbx->msg_id = msg_desc->msg_info.msg_id;
	recv_mbx->mod = SSS_GET_MSG_HEADER(msg_header, MODULE);
	recv_mbx->cmd = SSS_GET_MSG_HEADER(msg_header, CMD);
	recv_mbx->ack_type = SSS_GET_MSG_HEADER(msg_header, NO_ACK);
	recv_mbx->src_func_id = SSS_GET_MSG_HEADER(msg_header, SRC_GLB_FUNC_ID);
	recv_mbx->buf_len = msg_desc->msg_len;
	memcpy(recv_mbx->buf, msg_desc->msg, msg_desc->msg_len);
}

static int sss_init_mbx_work(struct sss_mbx *mbx, struct sss_recv_mbx *recv_mbx,
			     struct sss_msg_buffer *msg_buffer)
{
	struct sss_mbx_work *mbx_work = NULL;

	mbx_work = kzalloc(sizeof(*mbx_work), GFP_KERNEL);
	if (!mbx_work)
		return -ENOMEM;

	atomic_inc(&msg_buffer->recv_msg_cnt);

	mbx_work->msg_buffer = msg_buffer;
	mbx_work->recv_mbx = recv_mbx;
	mbx_work->mbx = mbx;

	INIT_WORK(&mbx_work->work, sss_recv_mbx_work_handler);
	queue_work_on(WORK_CPU_UNBOUND, mbx->workq, &mbx_work->work);

	return 0;
}

static void sss_recv_mbx_msg_handler(struct sss_mbx *mbx,
				     struct sss_msg_desc *msg_desc, u64 msg_header)
{
	u32 msg_cnt;
	int ret;
	struct sss_hwdev *hwdev = SSS_TO_HWDEV(mbx);
	struct sss_recv_mbx *recv_mbx = NULL;
	struct sss_msg_buffer *msg_buffer = container_of(msg_desc, struct sss_msg_buffer, recv_msg);

	msg_cnt = atomic_read(&msg_buffer->recv_msg_cnt);
	if (msg_cnt > SSS_MSG_PROCESS_CNT_MAX) {
		u64 src_func_id = SSS_GET_MSG_HEADER(msg_header, SRC_GLB_FUNC_ID);

		sdk_warn(hwdev->dev_hdl, "This func(%llu) have %u msg wait to process\n",
			 src_func_id, msg_cnt);
		return;
	}

	recv_mbx = sss_alloc_recv_mbx();
	if (!recv_mbx) {
		sdk_err(hwdev->dev_hdl, "Fail to alloc receive recv_mbx message buffer\n");
		return;
	}

	sss_init_recv_mbx_param(recv_mbx, msg_desc, msg_header);

	ret = sss_init_mbx_work(mbx, recv_mbx, msg_buffer);
	if (ret != 0)
		sss_free_recv_mbx(recv_mbx);
}

static void sss_resp_mbx_handler(struct sss_mbx *mbx,
				 const struct sss_msg_desc *msg_desc)
{
	spin_lock(&mbx->mbx_lock);
	if (msg_desc->msg_info.msg_id == mbx->send_msg_id &&
	    mbx->event_flag == SSS_EVENT_START)
		mbx->event_flag = SSS_EVENT_SUCCESS;
	else
		sdk_err(SSS_TO_HWDEV(mbx)->dev_hdl,
			"Mbx resp timeout, current send msg_id(0x%x), recv msg_id(0x%x), status(0x%x)\n",
			mbx->send_msg_id, msg_desc->msg_info.msg_id, msg_desc->msg_info.state);
	spin_unlock(&mbx->mbx_lock);
}

static void sss_recv_mbx_aeq(struct sss_mbx *mbx, u64 *msg_header,
			     struct sss_msg_desc *msg_desc)
{
	u64 header = *msg_header;

	if (!sss_check_mbx_msg_header(SSS_TO_HWDEV(mbx)->dev_hdl, msg_desc, header)) {
		msg_desc->seq_id = SSS_MAX_SEG_ID;
		return;
	}

	sss_fill_msg_desc(msg_desc, msg_header);

	if (!SSS_GET_MSG_HEADER(header, LAST))
		return;

	if (SSS_GET_MSG_HEADER(header, DIRECTION) == SSS_DIRECT_SEND_MSG) {
		sss_recv_mbx_msg_handler(mbx, msg_desc, header);
		return;
	}

	sss_resp_mbx_handler(mbx, msg_desc);
}

void sss_recv_mbx_aeq_handler(void *handle, u8 *header, u8 size)
{
	u64 msg_header = *((u64 *)header);
	u64 src_func_id = SSS_GET_MSG_HEADER(msg_header, SRC_GLB_FUNC_ID);
	u64 direction = SSS_GET_MSG_HEADER(msg_header, DIRECTION);
	struct sss_msg_desc *msg_desc = NULL;
	struct sss_hwdev *hwdev = (struct sss_hwdev *)handle;
	struct sss_mbx *mbx = hwdev->mbx;

	msg_desc = sss_get_mbx_msg_desc(mbx, src_func_id, direction);
	if (!msg_desc) {
		sdk_err(hwdev->dev_hdl, "Invalid mbx src_func_id: %u\n", (u32)src_func_id);
		return;
	}

	sss_recv_mbx_aeq(mbx, (u64 *)header, msg_desc);
}

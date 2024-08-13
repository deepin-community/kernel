// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/completion.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/semaphore.h>
#include <linux/jiffies.h>
#include <linux/delay.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_common.h"
#include "sss_hwdev.h"
#include "sss_csr.h"
#include "sss_hwif_api.h"
#include "sss_hwif_adm_common.h"
#include "sss_hwif_mgmt_common.h"

/* ADM_MSG_REQ CSR: 0x0020+adm_id*0x080 */
#define SSS_ADM_MSG_REQ_RESTART_SHIFT			1
#define SSS_ADM_MSG_REQ_WB_TRIGGER_SHIFT		2

#define SSS_ADM_MSG_REQ_RESTART_MASK			0x1U
#define SSS_ADM_MSG_REQ_WB_TRIGGER_MASK			0x1U

#define SSS_SET_ADM_MSG_REQ(val, member)		\
			(((val) & SSS_ADM_MSG_REQ_##member##_MASK) << \
			SSS_ADM_MSG_REQ_##member##_SHIFT)

#define SSS_GET_ADM_MSG_REQ(val, member)		\
			(((val) >> SSS_ADM_MSG_REQ_##member##_SHIFT) & \
			SSS_ADM_MSG_REQ_##member##_MASK)

#define SSS_CLEAR_ADM_MSG_REQ(val, member)		\
			((val) & (~(SSS_ADM_MSG_REQ_##member##_MASK	\
			<< SSS_ADM_MSG_REQ_##member##_SHIFT)))

/* ADM_MSG_CTRL CSR: 0x0014+adm_id*0x080 */
#define SSS_ADM_MSG_CTRL_RESTART_EN_SHIFT		1
#define SSS_ADM_MSG_CTRL_XOR_ERR_SHIFT			2
#define SSS_ADM_MSG_CTRL_AEQE_EN_SHIFT			4
#define SSS_ADM_MSG_CTRL_AEQ_ID_SHIFT			8
#define SSS_ADM_MSG_CTRL_XOR_CHK_EN_SHIFT		28
#define SSS_ADM_MSG_CTRL_ELEM_SIZE_SHIFT		30

#define SSS_ADM_MSG_CTRL_RESTART_EN_MASK		0x1U
#define SSS_ADM_MSG_CTRL_XOR_ERR_MASK			0x1U
#define SSS_ADM_MSG_CTRL_AEQE_EN_MASK			0x1U
#define SSS_ADM_MSG_CTRL_AEQ_ID_MASK			0x3U
#define SSS_ADM_MSG_CTRL_XOR_CHK_EN_MASK		0x3U
#define SSS_ADM_MSG_CTRL_ELEM_SIZE_MASK			0x3U

#define SSS_SET_ADM_MSG_CTRL(val, member)				\
	(((val) & SSS_ADM_MSG_CTRL_##member##_MASK) <<	\
	SSS_ADM_MSG_CTRL_##member##_SHIFT)

#define SSS_CLEAR_ADM_MSG_CTRL(val, member)			\
	((val) & (~(SSS_ADM_MSG_CTRL_##member##_MASK		\
		<< SSS_ADM_MSG_CTRL_##member##_SHIFT)))

#define SSS_ADM_MSG_BUF_SIZE				2048ULL

#define SSS_ADM_MSG_NODE_ALIGN_SIZE		512ULL
#define SSS_ADM_MSG_PAYLOAD_ALIGN_SIZE	64ULL

#define SSS_ADM_MSG_REPLY_ALIGNMENT		128ULL

#define SSS_ADM_MSG_TIMEOUT				10000

#define SSS_ADM_MSG_ELEM_SIZE_SHIFT	6U

#define SSS_ADM_MSG_ELEM_NUM				32
#define SSS_ADM_MSG_ELEM_SIZE				128
#define SSS_ADM_MSG_REPLY_DATA_SIZE			128

#define SSS_MGMT_WQ_NAME					"sssnic_mgmt"

#define SSS_GET_ADM_MSG_ELEM_PADDR(adm_msg, elem_id) \
			((adm_msg)->elem_paddr_base + (adm_msg)->elem_size_align * (elem_id))

#define SSS_GET_ADM_MSG_ELEM_VADDR(adm_msg, elem_id) \
			((adm_msg)->elem_vaddr_base + (adm_msg)->elem_size_align * (elem_id))

#define SSS_GET_ADM_MSG_BUF_PADDR(adm_msg, elem_id) \
			((adm_msg)->buf_paddr_base + (adm_msg)->buf_size_align * (elem_id))

#define SSS_GET_ADM_MSG_BUF_VADDR(adm_msg, elem_id) \
			((adm_msg)->buf_vaddr_base + (adm_msg)->buf_size_align * (elem_id))

#define SSS_GET_ADM_MSG_REPLY_PADDR(adm_msg, elem_id) \
			((adm_msg)->reply_paddr_base + (adm_msg)->reply_size_align * (elem_id))

#define SSS_GET_ADM_MSG_REPLY_VADDR(adm_msg, elem_id) \
			((adm_msg)->reply_vaddr_base + (adm_msg)->reply_size_align * (elem_id))

typedef void (*sss_alloc_elem_buf_handler_t)(struct sss_adm_msg *adm_msg, u32 elem_id);

struct sss_adm_msg_attr {
	struct sss_hwdev		*hwdev;
	enum sss_adm_msg_type	msg_type;

	u32		elem_num;
	u16		reply_size;
	u16		elem_size;
};

static enum sss_process_ret sss_adm_msg_reset_handler(void *priv_data)
{
	u32 val;
	u32 addr;
	struct sss_adm_msg *adm_msg = priv_data;

	if (!SSS_TO_HWDEV(adm_msg)->chip_present_flag)
		return SSS_PROCESS_ERR;

	addr = SSS_CSR_ADM_MSG_REQ_ADDR(adm_msg->msg_type);
	val = sss_chip_read_reg(SSS_TO_HWDEV(adm_msg)->hwif, addr);
	if (!SSS_GET_ADM_MSG_REQ(val, RESTART))
		return SSS_PROCESS_OK;

	return SSS_PROCESS_DOING;
}

static enum sss_process_ret sss_adm_msg_ready_handler(void *priv_data)
{
	u32 val;
	u32 addr;
	struct sss_adm_msg *adm_msg = priv_data;

	if (!SSS_TO_HWDEV(adm_msg)->chip_present_flag)
		return SSS_PROCESS_ERR;

	addr = SSS_CSR_ADM_MSG_STATE_0_ADDR(adm_msg->msg_type);
	val = sss_chip_read_reg(SSS_TO_HWDEV(adm_msg)->hwif, addr);
	if (SSS_GET_ADM_MSG_STATE(val, CI) == adm_msg->ci)
		return SSS_PROCESS_OK;

	return SSS_PROCESS_DOING;
}

static void sss_chip_clean_adm_msg(struct sss_adm_msg *adm_msg)
{
	u32 val;
	u32 addr = SSS_CSR_ADM_MSG_CTRL_ADDR(adm_msg->msg_type);

	val = sss_chip_read_reg(SSS_TO_HWDEV(adm_msg)->hwif, addr);
	val = SSS_CLEAR_ADM_MSG_CTRL(val, RESTART_EN) &
	      SSS_CLEAR_ADM_MSG_CTRL(val, XOR_ERR) &
	      SSS_CLEAR_ADM_MSG_CTRL(val, AEQE_EN) &
	      SSS_CLEAR_ADM_MSG_CTRL(val, XOR_CHK_EN) &
	      SSS_CLEAR_ADM_MSG_CTRL(val, ELEM_SIZE);

	sss_chip_write_reg(SSS_TO_HWDEV(adm_msg)->hwif, addr, val);
}

static void sss_chip_set_adm_msg_wb_addr(struct sss_adm_msg *adm_msg)
{
	u32 val;
	u32 addr;

	addr = SSS_CSR_ADM_MSG_STATE_HI_ADDR(adm_msg->msg_type);
	val = upper_32_bits(adm_msg->wb_state_paddr);
	sss_chip_write_reg(SSS_TO_HWDEV(adm_msg)->hwif, addr, val);

	addr = SSS_CSR_ADM_MSG_STATE_LO_ADDR(adm_msg->msg_type);
	val = lower_32_bits(adm_msg->wb_state_paddr);
	sss_chip_write_reg(SSS_TO_HWDEV(adm_msg)->hwif, addr, val);
}

static int sss_chip_reset_adm_msg(struct sss_adm_msg *adm_msg)
{
	u32 val;
	u32 addr;

	addr = SSS_CSR_ADM_MSG_REQ_ADDR(adm_msg->msg_type);
	val = sss_chip_read_reg(SSS_TO_HWDEV(adm_msg)->hwif, addr);

	val = SSS_CLEAR_ADM_MSG_REQ(val, RESTART);
	val |= SSS_SET_ADM_MSG_REQ(1, RESTART);

	sss_chip_write_reg(SSS_TO_HWDEV(adm_msg)->hwif, addr, val);

	return sss_check_handler_timeout(adm_msg, sss_adm_msg_reset_handler,
					 SSS_ADM_MSG_TIMEOUT, USEC_PER_MSEC);
}

static void sss_chip_init_elem_size(struct sss_adm_msg *adm_msg)
{
	u32 val;
	u32 addr;
	u32 size;

	addr = SSS_CSR_ADM_MSG_CTRL_ADDR(adm_msg->msg_type);
	val = sss_chip_read_reg(SSS_TO_HWDEV(adm_msg)->hwif, addr);
	val = SSS_CLEAR_ADM_MSG_CTRL(val, AEQE_EN) &
	      SSS_CLEAR_ADM_MSG_CTRL(val, ELEM_SIZE);

	size = (u32)ilog2(adm_msg->elem_size >> SSS_ADM_MSG_ELEM_SIZE_SHIFT);
	val |= SSS_SET_ADM_MSG_CTRL(0, AEQE_EN) |
	       SSS_SET_ADM_MSG_CTRL(size, ELEM_SIZE);

	sss_chip_write_reg(SSS_TO_HWDEV(adm_msg)->hwif, addr, val);
}

static void sss_chip_set_elem_num(struct sss_adm_msg *adm_msg)
{
	u32 addr;

	addr = SSS_CSR_ADM_MSG_NUM_ELEM_ADDR(adm_msg->msg_type);
	sss_chip_write_reg(SSS_TO_HWDEV(adm_msg)->hwif, addr, adm_msg->elem_num);
}

static void sss_chip_init_elem_head(struct sss_adm_msg *adm_msg)
{
	u32 val;
	u32 addr;

	addr = SSS_CSR_ADM_MSG_HEAD_HI_ADDR(adm_msg->msg_type);
	val = upper_32_bits(adm_msg->head_elem_paddr);
	sss_chip_write_reg(SSS_TO_HWDEV(adm_msg)->hwif, addr, val);

	addr = SSS_CSR_ADM_MSG_HEAD_LO_ADDR(adm_msg->msg_type);
	val = lower_32_bits(adm_msg->head_elem_paddr);
	sss_chip_write_reg(SSS_TO_HWDEV(adm_msg)->hwif, addr, val);
}

static int sss_wait_adm_msg_ready(struct sss_adm_msg *adm_msg)
{
	return sss_check_handler_timeout(adm_msg, sss_adm_msg_ready_handler,
					 SSS_ADM_MSG_TIMEOUT, USEC_PER_MSEC);
}

static int sss_chip_init_adm_msg(struct sss_adm_msg *adm_msg)
{
	sss_chip_clean_adm_msg(adm_msg);

	sss_chip_set_adm_msg_wb_addr(adm_msg);

	if (sss_chip_reset_adm_msg(adm_msg) != 0) {
		sdk_err(SSS_TO_HWDEV(adm_msg)->dev_hdl, "Fail to restart adm cmd\n");
		return -EBUSY;
	}

	sss_chip_init_elem_size(adm_msg);
	sss_chip_set_elem_num(adm_msg);
	sss_chip_init_elem_head(adm_msg);

	return sss_wait_adm_msg_ready(adm_msg);
}

static void sss_init_ctx_buf_addr(struct sss_adm_msg *adm_msg,
				  u32 elem_id)
{
	u64 paddr;
	void *vaddr;
	struct sss_adm_msg_elem_ctx *ctx = &adm_msg->elem_ctx[elem_id];
	struct sss_adm_msg_elem *elem = NULL;

	vaddr = (u8 *)SSS_GET_ADM_MSG_BUF_VADDR(adm_msg, elem_id);
	paddr = SSS_GET_ADM_MSG_BUF_PADDR(adm_msg, elem_id);

	ctx->adm_msg_vaddr = vaddr;
	elem =
		(struct sss_adm_msg_elem *)SSS_GET_ADM_MSG_ELEM_VADDR(adm_msg, elem_id);
	elem->write.hw_msg_paddr = cpu_to_be64(paddr);
}

static void sss_init_ctx_reply_addr(struct sss_adm_msg *adm_msg,
				    u32 elem_id)
{
	u64 paddr;
	void *vaddr;
	struct sss_adm_msg_elem_ctx *ctx = &adm_msg->elem_ctx[elem_id];
	struct sss_adm_msg_elem *elem = NULL;

	paddr = SSS_GET_ADM_MSG_REPLY_PADDR(adm_msg, elem_id);
	vaddr = (u8 *)SSS_GET_ADM_MSG_REPLY_VADDR(adm_msg, elem_id);

	elem =
		(struct sss_adm_msg_elem *)SSS_GET_ADM_MSG_ELEM_VADDR(adm_msg, elem_id);
	elem->read.hw_wb_reply_paddr = cpu_to_be64(paddr);
	ctx->reply_fmt = vaddr;
	ctx->adm_msg_vaddr = &elem->read.hw_msg_paddr;
}

static void sss_init_ctx_buf_reply_addr(struct sss_adm_msg *adm_msg,
					u32 elem_id)
{
	u64 buf_paddr;
	void *buf_vaddr;
	void *rsp_vaddr;
	struct sss_adm_msg_elem_ctx *ctx = &adm_msg->elem_ctx[elem_id];
	struct sss_adm_msg_elem *elem = NULL;

	rsp_vaddr = (u8 *)SSS_GET_ADM_MSG_REPLY_VADDR(adm_msg, elem_id);
	buf_paddr = SSS_GET_ADM_MSG_BUF_PADDR(adm_msg, elem_id);
	buf_vaddr = (u8 *)SSS_GET_ADM_MSG_BUF_VADDR(adm_msg, elem_id);

	elem =
		(struct sss_adm_msg_elem *)SSS_GET_ADM_MSG_ELEM_VADDR(adm_msg, elem_id);
	ctx->reply_fmt = rsp_vaddr;
	ctx->adm_msg_vaddr = buf_vaddr;
	elem->read.hw_msg_paddr = cpu_to_be64(buf_paddr);
}

static void sss_alloc_reply_buf(struct sss_adm_msg *adm_msg,
				struct sss_adm_msg_elem *elem, u32 cell_idx)
{
	struct sss_adm_msg_elem_ctx *ctx = NULL;
	void *resp_vaddr;
	u64 resp_paddr;

	resp_vaddr = (u8 *)((u64)adm_msg->reply_vaddr_base +
		adm_msg->reply_size_align * cell_idx);
	resp_paddr = adm_msg->reply_paddr_base +
		adm_msg->reply_size_align * cell_idx;

	ctx = &adm_msg->elem_ctx[cell_idx];

	ctx->reply_fmt = resp_vaddr;
	elem->read.hw_wb_reply_paddr = cpu_to_be64(resp_paddr);
}

static int sss_init_elem_ctx(struct sss_adm_msg *adm_msg, u32 elem_id)
{
	struct sss_adm_msg_elem_ctx *ctx = NULL;
	struct sss_adm_msg_elem *elem;
	sss_alloc_elem_buf_handler_t handler[] = {
		NULL,
		NULL,
		sss_init_ctx_buf_addr,
		sss_init_ctx_reply_addr,
		sss_init_ctx_buf_addr,
		sss_init_ctx_buf_reply_addr,
		sss_init_ctx_buf_addr
	};
	elem = (struct sss_adm_msg_elem *)SSS_GET_ADM_MSG_ELEM_VADDR(adm_msg, elem_id);

	if (adm_msg->msg_type == SSS_ADM_MSG_MULTI_READ ||
	    adm_msg->msg_type == SSS_ADM_MSG_POLL_READ)
		sss_alloc_reply_buf(adm_msg, elem, elem_id);

	ctx = &adm_msg->elem_ctx[elem_id];
	ctx->elem_vaddr =
		(struct sss_adm_msg_elem *)SSS_GET_ADM_MSG_ELEM_VADDR(adm_msg, elem_id);
	ctx->hwdev = adm_msg->hwdev;

	if (adm_msg->msg_type >= ARRAY_LEN(handler))
		goto out;

	if (!handler[adm_msg->msg_type])
		goto out;

	handler[adm_msg->msg_type](adm_msg, elem_id);

	return 0;

out:
	sdk_err(SSS_TO_HWDEV(adm_msg)->dev_hdl, "Unsupport adm msg type %u\n", adm_msg->msg_type);
	return -EINVAL;
}

static int sss_init_adm_msg_elem(struct sss_adm_msg *adm_msg)
{
	u32 i;
	u64 paddr;
	void *vaddr;
	struct sss_adm_msg_elem *elem = NULL;
	struct sss_adm_msg_elem *pre_elt = NULL;
	int ret;

	for (i = 0; i < adm_msg->elem_num; i++) {
		ret = sss_init_elem_ctx(adm_msg, i);
		if (ret != 0)
			return ret;

		paddr = SSS_GET_ADM_MSG_ELEM_PADDR(adm_msg, i);
		vaddr = SSS_GET_ADM_MSG_ELEM_VADDR(adm_msg, i);

		if (!pre_elt) {
			adm_msg->head_node = vaddr;
			adm_msg->head_elem_paddr = (dma_addr_t)paddr;
		} else {
			pre_elt->next_elem_paddr = cpu_to_be64(paddr);
		}

		elem = vaddr;
		elem->next_elem_paddr = 0;

		pre_elt = elem;
	}

	elem->next_elem_paddr = cpu_to_be64(adm_msg->head_elem_paddr);
	adm_msg->now_node = adm_msg->head_node;

	return 0;
}

static int sss_alloc_adm_msg_ctx(struct sss_adm_msg *adm_msg)
{
	size_t ctx_size;

	ctx_size = adm_msg->elem_num * sizeof(*adm_msg->elem_ctx);

	adm_msg->elem_ctx = kzalloc(ctx_size, GFP_KERNEL);
	if (!adm_msg->elem_ctx)
		return -ENOMEM;

	return 0;
}

static void sss_free_adm_msg_ctx(struct sss_adm_msg *adm_msg)
{
	kfree(adm_msg->elem_ctx);
	adm_msg->elem_ctx = NULL;
}

static int sss_alloc_adm_msg_wb_state(struct sss_adm_msg *adm_msg)
{
	void *dev_hdl = SSS_TO_HWDEV(adm_msg)->dev_hdl;

	adm_msg->wb_state = dma_zalloc_coherent(dev_hdl, sizeof(*adm_msg->wb_state),
						&adm_msg->wb_state_paddr, GFP_KERNEL);
	if (!adm_msg->wb_state) {
		sdk_err(dev_hdl, "Fail to alloc dma wb status\n");
		return -ENOMEM;
	}

	return 0;
}

static void sss_free_adm_msg_wb_state(struct sss_adm_msg *adm_msg)
{
	void *dev_hdl = SSS_TO_HWDEV(adm_msg)->dev_hdl;

	dma_free_coherent(dev_hdl, sizeof(*adm_msg->wb_state),
			  adm_msg->wb_state, adm_msg->wb_state_paddr);
}

static int sss_alloc_elem_buf(struct sss_adm_msg *adm_msg)
{
	int ret;
	size_t buf_size;
	void *dev_hdl = SSS_TO_HWDEV(adm_msg)->dev_hdl;

	adm_msg->buf_size_align = ALIGN(SSS_ADM_MSG_BUF_SIZE,
					SSS_ADM_MSG_PAYLOAD_ALIGN_SIZE);
	adm_msg->elem_size_align = ALIGN((u64)adm_msg->elem_size,
					 SSS_ADM_MSG_NODE_ALIGN_SIZE);
	adm_msg->reply_size_align = ALIGN((u64)adm_msg->reply_size,
					  SSS_ADM_MSG_REPLY_ALIGNMENT);
	buf_size = (adm_msg->buf_size_align + adm_msg->elem_size_align +
		    adm_msg->reply_size_align) * adm_msg->elem_num;

	ret = sss_dma_zalloc_coherent_align(dev_hdl, buf_size, SSS_ADM_MSG_NODE_ALIGN_SIZE,
					    GFP_KERNEL, &adm_msg->elem_addr);
	if (ret != 0) {
		sdk_err(dev_hdl, "Fail to alloc adm msg elem buffer\n");
		return ret;
	}

	adm_msg->elem_vaddr_base = adm_msg->elem_addr.align_vaddr;
	adm_msg->elem_paddr_base = adm_msg->elem_addr.align_paddr;

	adm_msg->reply_vaddr_base = (u8 *)((u64)adm_msg->elem_vaddr_base +
					   adm_msg->elem_size_align * adm_msg->elem_num);
	adm_msg->reply_paddr_base = adm_msg->elem_paddr_base +
				    adm_msg->elem_size_align * adm_msg->elem_num;

	adm_msg->buf_vaddr_base = (u8 *)((u64)adm_msg->reply_vaddr_base +
					 adm_msg->reply_size_align * adm_msg->elem_num);
	adm_msg->buf_paddr_base = adm_msg->reply_paddr_base +
				  adm_msg->reply_size_align * adm_msg->elem_num;

	return 0;
}

static void sss_free_elem_buf(struct sss_adm_msg *adm_msg)
{
	void *dev_hdl = SSS_TO_HWDEV(adm_msg)->dev_hdl;

	sss_dma_free_coherent_align(dev_hdl, &adm_msg->elem_addr);
}

static int sss_alloc_adm_msg_buf(struct sss_adm_msg *adm_msg)
{
	int ret;

	ret = sss_alloc_adm_msg_ctx(adm_msg);
	if (ret != 0)
		return ret;

	ret = sss_alloc_adm_msg_wb_state(adm_msg);
	if (ret != 0)
		goto alloc_wb_err;

	ret = sss_alloc_elem_buf(adm_msg);
	if (ret != 0)
		goto alloc_elem_buf_err;

	return 0;

alloc_elem_buf_err:
	sss_free_adm_msg_wb_state(adm_msg);

alloc_wb_err:
	sss_free_adm_msg_ctx(adm_msg);

	return ret;
}

static void sss_free_adm_msg_buf(struct sss_adm_msg *adm_msg)
{
	sss_free_elem_buf(adm_msg);

	sss_free_adm_msg_wb_state(adm_msg);

	sss_free_adm_msg_ctx(adm_msg);
}

static void sss_init_adm_msg_param(struct sss_adm_msg *adm_msg,
				   struct sss_hwdev *hwdev, u8 msg_type)
{
	adm_msg->hwdev = hwdev;
	adm_msg->elem_num = SSS_ADM_MSG_ELEM_NUM;
	adm_msg->reply_size = SSS_ADM_MSG_REPLY_DATA_SIZE;
	adm_msg->elem_size = SSS_ADM_MSG_ELEM_SIZE;
	adm_msg->msg_type = msg_type;
	adm_msg->pi = 0;
	adm_msg->ci = 0;
	if (adm_msg->msg_type == SSS_ADM_MSG_WRITE_ASYNC_TO_MGMT_MODULE)
		spin_lock_init(&adm_msg->async_lock);
	else
		sema_init(&adm_msg->sem, 1);
}

static int create_adm_msg(struct sss_hwdev *hwdev, struct sss_adm_msg **adm_msg, u8 msg_type)
{
	struct sss_adm_msg *msg;
	int ret;

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	sss_init_adm_msg_param(msg, hwdev, msg_type);

	ret = sss_alloc_adm_msg_buf(msg);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init adm msg buf\n");
		return ret;
	}

	ret = sss_init_adm_msg_elem(msg);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init adm msg elem\n");
		sss_free_adm_msg_buf(msg);
		return ret;
	}

	ret = sss_chip_init_adm_msg(msg);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init adm msg\n");
		sss_free_adm_msg_buf(msg);
		return ret;
	}

	*adm_msg = msg;

	return 0;
}

void sss_destroy_adm_msg(struct sss_adm_msg *adm_msg)
{
	sss_free_adm_msg_buf(adm_msg);
	kfree(adm_msg);
}

static int sss_init_adm_msg(struct sss_hwdev *hwdev,
			    struct sss_adm_msg **adm_msg)
{
	int ret;
	u8 i;
	u8 adm_msg_type;
	void *dev = ((struct sss_hwdev *)hwdev)->dev_hdl;

	if (!SSS_SUPPORT_ADM_MSG(hwdev))
		return 0;

	for (adm_msg_type = SSS_ADM_MSG_WRITE_TO_MGMT_MODULE;
	     adm_msg_type < SSS_ADM_MSG_MAX; adm_msg_type++) {
		ret = create_adm_msg(hwdev, &adm_msg[adm_msg_type], adm_msg_type);
		if (ret) {
			sdk_err(dev, "Failed to create adm msg %d\n", adm_msg_type);
			goto create_adm_msg_err;
		}
	}

	return 0;

create_adm_msg_err:
	for (i = SSS_ADM_MSG_WRITE_TO_MGMT_MODULE; i < adm_msg_type; i++)
		sss_destroy_adm_msg(hwdev->pf_to_mgmt->adm_msg[adm_msg_type]);

	return ret;
}

static void sss_deinit_adm_msg(const struct sss_hwdev *hwdev,
			       struct sss_adm_msg **adm_msg)
{
	u8 adm_msg_type;

	if (!SSS_SUPPORT_ADM_MSG(hwdev))
		return;

	for (adm_msg_type = SSS_ADM_MSG_WRITE_TO_MGMT_MODULE;
	     adm_msg_type < SSS_ADM_MSG_MAX; adm_msg_type++)
		sss_destroy_adm_msg(adm_msg[adm_msg_type]);
}

static int sss_alloc_msg_buf(struct sss_msg_pf_to_mgmt *mgmt_msg)
{
	struct sss_recv_msg *recv_msg = &mgmt_msg->recv_msg;
	struct sss_recv_msg *resp_msg = &mgmt_msg->recv_resp_msg;

	recv_msg->seq_id = SSS_MGMT_SEQ_ID_MAX;
	resp_msg->seq_id = SSS_MGMT_SEQ_ID_MAX;

	recv_msg->buf = kzalloc(SSS_PF_MGMT_BUF_LEN_MAX, GFP_KERNEL);
	if (!recv_msg->buf)
		return -ENOMEM;

	resp_msg->buf = kzalloc(SSS_PF_MGMT_BUF_LEN_MAX, GFP_KERNEL);
	if (!resp_msg->buf)
		goto alloc_resp_msg_err;

	mgmt_msg->ack_buf = kzalloc(SSS_PF_MGMT_BUF_LEN_MAX, GFP_KERNEL);
	if (!mgmt_msg->ack_buf)
		goto alloc_ack_buf_err;

	mgmt_msg->sync_buf = kzalloc(SSS_PF_MGMT_BUF_LEN_MAX, GFP_KERNEL);
	if (!mgmt_msg->sync_buf)
		goto alloc_sync_buf_err;

	mgmt_msg->async_msg_buf = kzalloc(SSS_PF_MGMT_BUF_LEN_MAX, GFP_KERNEL);
	if (!mgmt_msg->async_msg_buf)
		goto alloc_async_msg_buf_err;

	return 0;

alloc_async_msg_buf_err:
	kfree(mgmt_msg->sync_buf);
	mgmt_msg->sync_buf = NULL;
alloc_sync_buf_err:
	kfree(mgmt_msg->ack_buf);
	mgmt_msg->ack_buf = NULL;

alloc_ack_buf_err:
	kfree(resp_msg->buf);
	resp_msg->buf = NULL;

alloc_resp_msg_err:
	kfree(recv_msg->buf);
	recv_msg->buf = NULL;

	return -ENOMEM;
}

static void sss_free_msg_buf(struct sss_msg_pf_to_mgmt *mgmt_msg)
{
	struct sss_recv_msg *recv_msg = &mgmt_msg->recv_msg;
	struct sss_recv_msg *resp_msg = &mgmt_msg->recv_resp_msg;

	kfree(mgmt_msg->async_msg_buf);
	kfree(mgmt_msg->sync_buf);
	kfree(mgmt_msg->ack_buf);
	kfree(resp_msg->buf);
	kfree(recv_msg->buf);
}

int sss_hwif_init_adm(struct sss_hwdev *hwdev)
{
	int ret;
	struct sss_msg_pf_to_mgmt *mgmt_msg;

	mgmt_msg = kzalloc(sizeof(*mgmt_msg), GFP_KERNEL);
	if (!mgmt_msg)
		return -ENOMEM;

	spin_lock_init(&mgmt_msg->async_msg_lock);
	spin_lock_init(&mgmt_msg->sync_event_lock);
	sema_init(&mgmt_msg->sync_lock, 1);
	mgmt_msg->hwdev = hwdev;
	hwdev->pf_to_mgmt = mgmt_msg;

	mgmt_msg->workq = create_singlethread_workqueue(SSS_MGMT_WQ_NAME);
	if (!mgmt_msg->workq) {
		sdk_err(hwdev->dev_hdl, "Fail to init mgmt workq\n");
		ret = -ENOMEM;
		goto alloc_mgmt_wq_err;
	}

	ret = sss_alloc_msg_buf(mgmt_msg);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to alloc msg buffer\n");
		goto alloc_msg_buf_err;
	}

	ret = sss_init_adm_msg(hwdev, mgmt_msg->adm_msg);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init adm msg\n");
		goto init_all_adm_err;
	}

	return 0;

init_all_adm_err:
	sss_free_msg_buf(mgmt_msg);

alloc_msg_buf_err:
	destroy_workqueue(mgmt_msg->workq);

alloc_mgmt_wq_err:
	kfree(mgmt_msg);
	hwdev->pf_to_mgmt = NULL;

	return ret;
}

void sss_hwif_deinit_adm(struct sss_hwdev *hwdev)
{
	struct sss_msg_pf_to_mgmt *mgmt_msg = hwdev->pf_to_mgmt;

	destroy_workqueue(mgmt_msg->workq);

	sss_deinit_adm_msg(hwdev, mgmt_msg->adm_msg);

	sss_free_msg_buf(mgmt_msg);

	kfree(mgmt_msg);
	hwdev->pf_to_mgmt = NULL;
}

void sss_complete_adm_event(struct sss_hwdev *hwdev)
{
	struct sss_recv_msg *recv_msg =
			&hwdev->pf_to_mgmt->recv_resp_msg;

	spin_lock_bh(&hwdev->pf_to_mgmt->sync_event_lock);
	if (hwdev->pf_to_mgmt->event_state == SSS_ADM_EVENT_START) {
		complete(&recv_msg->done);
		hwdev->pf_to_mgmt->event_state = SSS_ADM_EVENT_TIMEOUT;
	}
	spin_unlock_bh(&hwdev->pf_to_mgmt->sync_event_lock);
}

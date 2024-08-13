// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/errno.h>
#include <linux/completion.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/module.h>

#include "sss_kernel.h"
#include "sss_pci.h"
#include "sss_hw.h"
#include "sss_hwdev.h"
#include "sss_hwif_export.h"
#include "sss_hwif_ceq.h"
#include "sss_hwif_api.h"
#include "sss_hwif_ctrlq.h"
#include "sss_common.h"

#define	SSS_CTRLQ_DEPTH							4096

#define SSS_CTRLQ_PFN_SHIFT						12
#define SSS_CTRLQ_PFN(addr) ((addr) >> SSS_CTRLQ_PFN_SHIFT)

#define	SSS_CTRLQ_CEQ_ID						0

#define SSS_CTRLQ_WQ_CLA_SIZE					512

#define SSS_CTRLQ_WQEBB_SIZE					64

#define SSS_CTRLQ_IDLE_TIMEOUT					5000

#define SSS_CTRLQ_CTX_NOW_WQE_PAGE_PFN_SHIFT	0
#define SSS_CTRLQ_CTX_CEQ_ID_SHIFT				53
#define SSS_CTRLQ_CTX_CEQ_ARM_SHIFT			61
#define SSS_CTRLQ_CTX_CEQ_EN_SHIFT				62
#define SSS_CTRLQ_CTX_HW_BUSY_BIT_SHIFT		63

#define SSS_CTRLQ_CTX_NOW_WQE_PAGE_PFN_MASK	0xFFFFFFFFFFFFF
#define SSS_CTRLQ_CTX_CEQ_ID_MASK				0xFF
#define SSS_CTRLQ_CTX_CEQ_ARM_MASK				0x1
#define SSS_CTRLQ_CTX_CEQ_EN_MASK				0x1
#define SSS_CTRLQ_CTX_HW_BUSY_BIT_MASK			0x1

#define SSS_SET_CTRLQ_CTX_INFO(val, member)                                    \
	(((u64)(val) & SSS_CTRLQ_CTX_##member##_MASK)                          \
	 << SSS_CTRLQ_CTX_##member##_SHIFT)

#define SSS_CTRLQ_CTX_WQ_BLOCK_PFN_SHIFT		0
#define SSS_CTRLQ_CTX_CI_SHIFT					52

#define SSS_CTRLQ_CTX_WQ_BLOCK_PFN_MASK		0xFFFFFFFFFFFFF
#define SSS_CTRLQ_CTX_CI_MASK					0xFFF

#define SSS_SET_CTRLQ_CTX_BLOCK_INFO(val, member)                              \
	(((u64)(val) & SSS_CTRLQ_CTX_##member##_MASK)                          \
	 << SSS_CTRLQ_CTX_##member##_SHIFT)

#define SSS_CTRLQ_CLA_WQ_PAGE_NUM (SSS_CTRLQ_WQ_CLA_SIZE / sizeof(u64))

#define SSS_GET_WQ_PAGE_SIZE(page_order)	(SSS_HW_WQ_PAGE_SIZE * (1U << (page_order)))

#define SSS_CTRLQ_DMA_POOL_NAME				"sss_ctrlq"

#define SSS_CTRLQ_WRAP_ENABLE				1

#define SSS_SET_WQE_PAGE_PFN(pfn) \
		(SSS_SET_CTRLQ_CTX_INFO(1, CEQ_ARM)	| \
		SSS_SET_CTRLQ_CTX_INFO(1, CEQ_EN)	| \
		SSS_SET_CTRLQ_CTX_INFO((pfn), NOW_WQE_PAGE_PFN) | \
		SSS_SET_CTRLQ_CTX_INFO(SSS_CTRLQ_CEQ_ID, CEQ_ID) | \
		SSS_SET_CTRLQ_CTX_INFO(1, HW_BUSY_BIT))

#define SSS_SET_WQ_BLOCK_PFN(wq, pfn) \
			(SSS_SET_CTRLQ_CTX_BLOCK_INFO((pfn), WQ_BLOCK_PFN) | \
			SSS_SET_CTRLQ_CTX_BLOCK_INFO((u16)(wq)->ci, CI))

static u32 wq_page_num = SSS_MAX_WQ_PAGE_NUM;
module_param(wq_page_num, uint, 0444);
MODULE_PARM_DESC(wq_page_num,
		 "Set wq page num, wq page size is 4K * (2 ^ wq_page_num) - default is 8");

static int sss_init_ctrq_block(struct sss_ctrlq_info *ctrlq_info)
{
	u8 i;

	if (SSS_WQ_IS_0_LEVEL_CLA(&ctrlq_info->ctrlq[SSS_CTRLQ_SYNC].wq))
		return 0;

	/* ctrlq wq's CLA table is up to 512B */
	if (ctrlq_info->ctrlq[SSS_CTRLQ_SYNC].wq.page_num > SSS_CTRLQ_CLA_WQ_PAGE_NUM) {
		sdk_err(SSS_TO_HWDEV(ctrlq_info)->dev_hdl, "Ctrlq wq page out of range: %lu\n",
			SSS_CTRLQ_CLA_WQ_PAGE_NUM);
		return -EINVAL;
	}

	ctrlq_info->wq_block_vaddr =
		dma_zalloc_coherent(SSS_TO_HWDEV(ctrlq_info)->dev_hdl, PAGE_SIZE,
				    &ctrlq_info->wq_block_paddr, GFP_KERNEL);
	if (!ctrlq_info->wq_block_vaddr) {
		sdk_err(SSS_TO_HWDEV(ctrlq_info)->dev_hdl, "Fail to alloc ctrlq wq block\n");
		return -ENOMEM;
	}

	for (i = 0; i < ctrlq_info->num; i++)
		memcpy((u8 *)ctrlq_info->wq_block_vaddr + SSS_CTRLQ_WQ_CLA_SIZE * i,
		       ctrlq_info->ctrlq[i].wq.block_vaddr,
		       ctrlq_info->ctrlq[i].wq.page_num * sizeof(u64));

	return 0;
}

static void sss_deinit_ctrq_block(struct sss_ctrlq_info *ctrlq_info)
{
	if (ctrlq_info->wq_block_vaddr) {
		dma_free_coherent(SSS_TO_HWDEV(ctrlq_info)->dev_hdl, PAGE_SIZE,
				  ctrlq_info->wq_block_vaddr, ctrlq_info->wq_block_paddr);
		ctrlq_info->wq_block_vaddr = NULL;
	}
}

static int sss_create_ctrlq_wq(struct sss_ctrlq_info *ctrlq_info)
{
	u8 i;
	int ret;
	u8 q_type;

	for (q_type = 0; q_type < ctrlq_info->num; q_type++) {
		ret = sss_create_wq(SSS_TO_HWDEV(ctrlq_info), &ctrlq_info->ctrlq[q_type].wq,
				    SSS_CTRLQ_DEPTH, SSS_CTRLQ_WQEBB_SIZE);
		if (ret != 0) {
			sdk_err(SSS_TO_HWDEV(ctrlq_info)->dev_hdl, "Fail to create ctrlq wq\n");
			goto destroy_wq;
		}
	}

	/* 1-level CLA must put all ctrlq's wq page addr in one wq block */
	ret = sss_init_ctrq_block(ctrlq_info);
	if (ret != 0)
		goto destroy_wq;

	return 0;

destroy_wq:
	for (i = 0; i < q_type; i++)
		sss_destroy_wq(&ctrlq_info->ctrlq[i].wq);
	sss_deinit_ctrq_block(ctrlq_info);

	return ret;
}

static void sss_destroy_ctrlq_wq(struct sss_ctrlq_info *ctrlq_info)
{
	u8 type;

	sss_deinit_ctrq_block(ctrlq_info);

	for (type = 0; type < ctrlq_info->num; type++)
		sss_destroy_wq(&ctrlq_info->ctrlq[type].wq);
}

static int sss_init_ctrlq_info(struct sss_ctrlq *ctrlq,
			       struct sss_ctrlq_ctxt_info *ctx,
			       dma_addr_t wq_block_paddr)
{
	struct sss_wq *wq = &ctrlq->wq;
	u64 pfn = SSS_CTRLQ_PFN(wq->page[0].align_paddr);

	ctrlq->cmd_info = kcalloc(ctrlq->wq.q_depth, sizeof(*ctrlq->cmd_info),
				  GFP_KERNEL);
	if (!ctrlq->cmd_info)
		return -ENOMEM;

	ctrlq->wrapped = SSS_CTRLQ_WRAP_ENABLE;
	spin_lock_init(&ctrlq->ctrlq_lock);

	ctx->curr_wqe_page_pfn = SSS_SET_WQE_PAGE_PFN(pfn);
	pfn = SSS_WQ_IS_0_LEVEL_CLA(wq) ? pfn : SSS_CTRLQ_PFN(wq_block_paddr);
	ctx->wq_block_pfn = SSS_SET_WQ_BLOCK_PFN(wq, pfn);

	return 0;
}

static void sss_deinit_ctrlq_info(struct sss_ctrlq *ctrlq)
{
	kfree(ctrlq->cmd_info);
}

static void sss_flush_ctrlq_sync_cmd(struct sss_ctrlq_cmd_info *info)
{
	if (info->msg_type != SSS_MSG_TYPE_DIRECT_RESP &&
	    info->msg_type != SSS_MSG_TYPE_SGE_RESP)
		return;

	info->msg_type = SSS_MSG_TYPE_FORCE_STOP;

	if (info->cmpt_code && *info->cmpt_code == SSS_CTRLQ_SEND_CMPT_CODE)
		*info->cmpt_code = SSS_CTRLQ_FORCE_STOP_CMPT_CODE;

	if (info->done) {
		complete(info->done);
		info->cmpt_code = NULL;
		info->direct_resp = NULL;
		info->err_code = NULL;
		info->done = NULL;
	}
}

static void sss_flush_ctrlq_cmd(struct sss_ctrlq *ctrlq)
{
	u16 ci = 0;

	spin_lock_bh(&ctrlq->ctrlq_lock);
	while (sss_ctrlq_read_wqe(&ctrlq->wq, &ci)) {
		sss_update_wq_ci(&ctrlq->wq, SSS_WQEBB_NUM_FOR_CTRLQ);
		sss_flush_ctrlq_sync_cmd(&ctrlq->cmd_info[ci]);
	}
	spin_unlock_bh(&ctrlq->ctrlq_lock);
}

static void sss_free_all_ctrlq_cmd_buff(struct sss_ctrlq *ctrlq)
{
	u16 i;

	for (i = 0; i < ctrlq->wq.q_depth; i++)
		sss_free_ctrlq_cmd_buf(SSS_TO_HWDEV(ctrlq), &ctrlq->cmd_info[i]);
}

static int sss_chip_set_ctrlq_ctx(struct sss_hwdev *hwdev, u8 qid,
				  struct sss_ctrlq_ctxt_info *ctxt)
{
	int ret;
	struct sss_cmd_ctrlq_ctxt cmd_ctx = {0};
	u16 out_len = sizeof(cmd_ctx);

	memcpy(&cmd_ctx.ctxt, ctxt, sizeof(*ctxt));
	cmd_ctx.ctrlq_id = qid;
	cmd_ctx.func_id = sss_get_global_func_id(hwdev);

	ret = sss_sync_send_msg(hwdev, SSS_COMM_MGMT_CMD_SET_CTRLQ_CTXT,
				&cmd_ctx, sizeof(cmd_ctx), &cmd_ctx, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_ctx)) {
		sdk_err(hwdev->dev_hdl,
			"Fail to set ctrlq ctx, ret: %d, status: 0x%x, out_len: 0x%x\n",
			ret, cmd_ctx.head.state, out_len);
		return -EFAULT;
	}

	return 0;
}

static int sss_init_ctrlq_ctx(struct sss_hwdev *hwdev)
{
	u8 q_type;
	int ret;
	struct sss_ctrlq_info *ctrlq_info = hwdev->ctrlq_info;

	for (q_type = 0; q_type < ctrlq_info->num; q_type++) {
		ret = sss_chip_set_ctrlq_ctx(hwdev, q_type, &ctrlq_info->ctrlq[q_type].ctrlq_ctxt);
		if (ret != 0)
			return ret;
	}

	ctrlq_info->disable_flag = 0;
	ctrlq_info->state |= SSS_CTRLQ_ENABLE;

	return 0;
}

int sss_reinit_ctrlq_ctx(struct sss_hwdev *hwdev)
{
	u8 ctrlq_type;
	struct sss_ctrlq_info *ctrlq_info = hwdev->ctrlq_info;

	for (ctrlq_type = 0; ctrlq_type < ctrlq_info->num; ctrlq_type++) {
		sss_flush_ctrlq_cmd(&ctrlq_info->ctrlq[ctrlq_type]);
		sss_free_all_ctrlq_cmd_buff(&ctrlq_info->ctrlq[ctrlq_type]);
		ctrlq_info->ctrlq[ctrlq_type].wrapped = 1;
		sss_wq_reset(&ctrlq_info->ctrlq[ctrlq_type].wq);
	}

	return sss_init_ctrlq_ctx(hwdev);
}

static int sss_init_ctrlq(struct sss_hwdev *hwdev)
{
	u8 i;
	u8 q_type;
	int ret = -ENOMEM;
	struct sss_ctrlq_info *ctrlq_info = NULL;

	ctrlq_info = kzalloc(sizeof(*ctrlq_info), GFP_KERNEL);
	if (!ctrlq_info)
		return -ENOMEM;

	ctrlq_info->hwdev = hwdev;
	hwdev->ctrlq_info = ctrlq_info;

	if (SSS_SUPPORT_CTRLQ_NUM(hwdev)) {
		ctrlq_info->num = hwdev->glb_attr.ctrlq_num;
		if (hwdev->glb_attr.ctrlq_num > SSS_MAX_CTRLQ_TYPE) {
			sdk_warn(hwdev->dev_hdl, "Adjust ctrlq num to %d\n", SSS_MAX_CTRLQ_TYPE);
			ctrlq_info->num = SSS_MAX_CTRLQ_TYPE;
		}
	} else {
		ctrlq_info->num = SSS_MAX_CTRLQ_TYPE;
	}

	ctrlq_info->msg_buf_pool = dma_pool_create(SSS_CTRLQ_DMA_POOL_NAME, hwdev->dev_hdl,
						   SSS_CTRLQ_BUF_LEN, SSS_CTRLQ_BUF_LEN, 0ULL);
	if (!ctrlq_info->msg_buf_pool) {
		sdk_err(hwdev->dev_hdl, "Fail to create ctrlq buffer pool\n");
		goto create_pool_err;
	}

	ret = sss_create_ctrlq_wq(ctrlq_info);
	if (ret != 0)
		goto create_wq_err;

	ret = sss_alloc_db_addr(hwdev, (void __iomem *)&ctrlq_info->db_base);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to alloc doorbell addr\n");
		goto init_db_err;
	}

	for (q_type = 0; q_type < ctrlq_info->num; q_type++) {
		ctrlq_info->ctrlq[q_type].hwdev = hwdev;
		ctrlq_info->ctrlq[q_type].ctrlq_type = q_type;
		ret = sss_init_ctrlq_info(&ctrlq_info->ctrlq[q_type],
					  &ctrlq_info->ctrlq[q_type].ctrlq_ctxt,
					  ctrlq_info->wq_block_paddr);
		if (ret != 0) {
			sdk_err(hwdev->dev_hdl, "Fail to init ctrlq i :%d\n", q_type);
			goto init_ctrlq_info_err;
		}
	}

	ret = sss_init_ctrlq_ctx(hwdev);
	if (ret != 0)
		goto init_ctrlq_info_err;

	return 0;

init_ctrlq_info_err:
	for (i = 0; i < q_type; i++)
		sss_deinit_ctrlq_info(&ctrlq_info->ctrlq[i]);

	sss_free_db_addr(hwdev, ctrlq_info->db_base);
init_db_err:
	sss_destroy_ctrlq_wq(ctrlq_info);
create_wq_err:
	dma_pool_destroy(ctrlq_info->msg_buf_pool);
create_pool_err:
	kfree(ctrlq_info);
	hwdev->ctrlq_info = NULL;

	return ret;
}

void sss_deinit_ctrlq(struct sss_hwdev *hwdev)
{
	u8 i;
	struct sss_ctrlq_info *ctrlq_info = hwdev->ctrlq_info;

	ctrlq_info->state &= ~SSS_CTRLQ_ENABLE;

	for (i = 0; i < ctrlq_info->num; i++) {
		sss_flush_ctrlq_cmd(&ctrlq_info->ctrlq[i]);
		sss_free_all_ctrlq_cmd_buff(&ctrlq_info->ctrlq[i]);
		sss_deinit_ctrlq_info(&ctrlq_info->ctrlq[i]);
	}

	sss_free_db_addr(hwdev, ctrlq_info->db_base);
	sss_destroy_ctrlq_wq(ctrlq_info);

	dma_pool_destroy(ctrlq_info->msg_buf_pool);

	kfree(ctrlq_info);
	hwdev->ctrlq_info = NULL;
}

static int sss_set_ctrlq_depth(void *hwdev)
{
	int ret;
	struct sss_cmd_root_ctxt cmd_ctx = {0};
	u16 out_len = sizeof(cmd_ctx);

	cmd_ctx.set_ctrlq_depth = 1;
	cmd_ctx.ctrlq_depth = (u8)ilog2(SSS_CTRLQ_DEPTH);
	cmd_ctx.func_id = sss_get_global_func_id(hwdev);

	ret = sss_sync_send_msg(hwdev, SSS_COMM_MGMT_CMD_SET_VAT, &cmd_ctx,
				sizeof(cmd_ctx), &cmd_ctx, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_ctx)) {
		sdk_err(SSS_TO_DEV(hwdev),
			"Fail to set ctrlq depth, ret: %d, status: 0x%x, out_len: 0x%x\n",
			ret, cmd_ctx.head.state, out_len);
		return -EFAULT;
	}

	return 0;
}

static int sss_hwif_init_ctrlq(struct sss_hwdev *hwdev)
{
	int ret;

	ret = sss_init_ctrlq(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init ctrlq\n");
		return ret;
	}

	sss_ceq_register_cb(hwdev, hwdev, SSS_NIC_CTRLQ, sss_ctrlq_ceq_handler);

	ret = sss_set_ctrlq_depth(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to set ctrlq depth\n");
		goto set_depth_err;
	}

	set_bit(SSS_HW_CTRLQ_INIT_OK, &hwdev->func_state);

	return 0;

set_depth_err:
	sss_deinit_ctrlq(hwdev);

	return ret;
}

static void sss_hwif_deinit_ctrlq(struct sss_hwdev *hwdev)
{
	spin_lock_bh(&hwdev->channel_lock);
	clear_bit(SSS_HW_CTRLQ_INIT_OK, &hwdev->func_state);
	spin_unlock_bh(&hwdev->channel_lock);

	sss_ceq_unregister_cb(hwdev, SSS_NIC_CTRLQ);
	sss_deinit_ctrlq(hwdev);
}

static bool sss_ctrlq_is_idle(struct sss_ctrlq *ctrlq)
{
	return sss_wq_is_empty(&ctrlq->wq);
}

static enum sss_process_ret sss_check_ctrlq_stop_handler(void *priv_data)
{
	struct sss_hwdev *hwdev = priv_data;
	struct sss_ctrlq_info *ctrlq_info = hwdev->ctrlq_info;
	enum sss_ctrlq_type ctrlq_type;

	/* Stop waiting when card unpresent */
	if (!hwdev->chip_present_flag)
		return SSS_PROCESS_OK;

	for (ctrlq_type = 0; ctrlq_type < ctrlq_info->num; ctrlq_type++) {
		if (!sss_ctrlq_is_idle(&ctrlq_info->ctrlq[ctrlq_type]))
			return SSS_PROCESS_DOING;
	}

	return SSS_PROCESS_OK;
}

static int sss_init_ctrlq_page_size(struct sss_hwdev *hwdev)
{
	int ret;

	if (wq_page_num > SSS_MAX_WQ_PAGE_NUM) {
		sdk_info(hwdev->dev_hdl,
			 "Invalid wq_page_num %u out of range, adjust to %d\n",
			 wq_page_num, SSS_MAX_WQ_PAGE_NUM);
		wq_page_num = SSS_MAX_WQ_PAGE_NUM;
	}

	hwdev->wq_page_size = SSS_GET_WQ_PAGE_SIZE(wq_page_num);
	ret = sss_chip_set_wq_page_size(hwdev, sss_get_global_func_id(hwdev),
					hwdev->wq_page_size);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to set wq page size\n");
		return ret;
	}

	return 0;
}

static void sss_deinit_ctrlq_page_size(struct sss_hwdev *hwdev)
{
	if (SSS_GET_FUNC_TYPE(hwdev) != SSS_FUNC_TYPE_VF)
		sss_chip_set_wq_page_size(hwdev, sss_get_global_func_id(hwdev),
					  SSS_HW_WQ_PAGE_SIZE);
}

int sss_init_ctrlq_channel(struct sss_hwdev *hwdev)
{
	int ret;

	ret = sss_hwif_init_ceq(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init hwdev ceq.\n");
		return ret;
	}

	ret = sss_init_ceq_msix_attr(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init ceq msix attr\n");
		goto init_msix_err;
	}

	ret = sss_init_ctrlq_page_size(hwdev);
	if (ret != 0)
		goto init_size_err;

	ret = sss_hwif_init_ctrlq(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init hwif ctrlq\n");
		goto init_ctrlq_err;
	}

	return 0;

init_ctrlq_err:
	sss_deinit_ctrlq_page_size(hwdev);
init_size_err:
init_msix_err:
	sss_hwif_deinit_ceq(hwdev);

	return ret;
}

void sss_deinit_ctrlq_channel(struct sss_hwdev *hwdev)
{
	sss_hwif_deinit_ctrlq(hwdev);

	sss_deinit_ctrlq_page_size(hwdev);

	sss_hwif_deinit_ceq(hwdev);
}

void sss_ctrlq_flush_sync_cmd(struct sss_hwdev *hwdev)
{
	u16 cnt;
	u16 ci;
	u16 i;
	u16 id;
	struct sss_wq *wq = NULL;
	struct sss_ctrlq *ctrlq = NULL;
	struct sss_ctrlq_cmd_info *info = NULL;

	ctrlq = &hwdev->ctrlq_info->ctrlq[SSS_CTRLQ_SYNC];

	spin_lock_bh(&ctrlq->ctrlq_lock);
	wq = &ctrlq->wq;
	id = wq->pi + wq->q_depth - wq->ci;
	cnt = (u16)SSS_WQ_MASK_ID(wq, id);
	ci = wq->ci;

	for (i = 0; i < cnt; i++) {
		info = &ctrlq->cmd_info[SSS_WQ_MASK_ID(wq, ci + i)];
		sss_flush_ctrlq_sync_cmd(info);
	}

	spin_unlock_bh(&ctrlq->ctrlq_lock);
}

int sss_wait_ctrlq_stop(struct sss_hwdev *hwdev)
{
	enum sss_ctrlq_type ctrlq_type;
	struct sss_ctrlq_info *ctrlq_info = hwdev->ctrlq_info;
	int ret;

	if (!(ctrlq_info->state & SSS_CTRLQ_ENABLE))
		return 0;

	ctrlq_info->state &= ~SSS_CTRLQ_ENABLE;

	ret = sss_check_handler_timeout(hwdev, sss_check_ctrlq_stop_handler,
					SSS_CTRLQ_IDLE_TIMEOUT, USEC_PER_MSEC);
	if (ret == 0)
		return 0;

	for (ctrlq_type = 0; ctrlq_type < ctrlq_info->num; ctrlq_type++) {
		if (!sss_ctrlq_is_idle(&ctrlq_info->ctrlq[ctrlq_type]))
			sdk_err(hwdev->dev_hdl, "Ctrlq %d is busy\n", ctrlq_type);
	}

	ctrlq_info->state |= SSS_CTRLQ_ENABLE;

	return ret;
}

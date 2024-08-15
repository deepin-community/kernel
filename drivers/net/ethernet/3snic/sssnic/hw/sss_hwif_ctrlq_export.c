// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/errno.h>
#include <linux/module.h>

#include "sss_kernel.h"
#include "sss_pci.h"
#include "sss_hw.h"
#include "sss_hwdev.h"
#include "sss_hwif_api.h"
#include "sss_hwif_ctrlq.h"
#include "sss_common.h"

#define SSS_CTRLQ_ENABLE_TIMEOUT 300

static int sss_wait_ctrlq_enable(struct sss_ctrlq_info *ctrlq_info)
{
	unsigned long end;

	end = jiffies + msecs_to_jiffies(SSS_CTRLQ_ENABLE_TIMEOUT);
	do {
		if (ctrlq_info->state & SSS_CTRLQ_ENABLE)
			return 0;
	} while (time_before(jiffies, end) &&
		 SSS_TO_HWDEV(ctrlq_info)->chip_present_flag &&
		 !ctrlq_info->disable_flag);

	ctrlq_info->disable_flag = 1;

	return -EBUSY;
}

static int sss_check_ctrlq_param(const void *hwdev, const struct sss_ctrl_msg_buf *in_buf)
{
	if (!hwdev || !in_buf) {
		pr_err("Invalid ctrlq param: hwdev: %p or in_buf: %p\n", hwdev, in_buf);
		return -EINVAL;
	}

	if (in_buf->size == 0 || in_buf->size > SSS_CTRLQ_BUF_LEN) {
		pr_err("Invalid ctrlq buf size: 0x%x\n", in_buf->size);
		return -EINVAL;
	}

	return 0;
}

struct sss_ctrl_msg_buf *sss_alloc_ctrlq_msg_buf(void *hwdev)
{
	struct sss_ctrlq_info *ctrlq_info = NULL;
	struct sss_ctrl_msg_buf *msg_buf = NULL;
	void *dev = NULL;

	if (!hwdev) {
		pr_err("Alloc ctrlq msg buf: hwdev is NULL\n");
		return NULL;
	}

	ctrlq_info = ((struct sss_hwdev *)hwdev)->ctrlq_info;
	dev = ((struct sss_hwdev *)hwdev)->dev_hdl;

	msg_buf = kzalloc(sizeof(*msg_buf), GFP_ATOMIC);
	if (!msg_buf)
		return NULL;

	msg_buf->buf = pci_pool_alloc(ctrlq_info->msg_buf_pool, GFP_ATOMIC,
				      &msg_buf->dma_addr);
	if (!msg_buf->buf) {
		sdk_err(dev, "Fail to allocate ctrlq pci pool\n");
		goto alloc_pci_buf_err;
	}

	msg_buf->size = SSS_CTRLQ_BUF_LEN;
	atomic_set(&msg_buf->ref_cnt, 1);

	return msg_buf;

alloc_pci_buf_err:
	kfree(msg_buf);
	return NULL;
}
EXPORT_SYMBOL(sss_alloc_ctrlq_msg_buf);

void sss_free_ctrlq_msg_buf(void *hwdev, struct sss_ctrl_msg_buf *msg_buf)
{
	struct sss_ctrlq_info *ctrlq_info = SSS_TO_CTRLQ_INFO(hwdev);

	if (!hwdev || !msg_buf) {
		pr_err("Invalid ctrlq param: hwdev: %p or msg_buf: %p\n", hwdev, msg_buf);
		return;
	}

	if (atomic_dec_and_test(&msg_buf->ref_cnt) == 0)
		return;

	pci_pool_free(ctrlq_info->msg_buf_pool, msg_buf->buf, msg_buf->dma_addr);
	kfree(msg_buf);
}
EXPORT_SYMBOL(sss_free_ctrlq_msg_buf);

int sss_ctrlq_direct_reply(void *hwdev, u8 mod, u8 cmd,
			   struct sss_ctrl_msg_buf *in_buf, u64 *out_param,
			   u32 timeout, u16 channel)
{
	int ret;
	struct sss_ctrlq_info *ctrlq_info = NULL;

	ret = sss_check_ctrlq_param(hwdev, in_buf);
	if (ret != 0) {
		pr_err("Invalid ctrlq parameters\n");
		return ret;
	}

	if (!sss_chip_get_present_state((struct sss_hwdev *)hwdev))
		return -EPERM;

	ctrlq_info = ((struct sss_hwdev *)hwdev)->ctrlq_info;
	ret = sss_wait_ctrlq_enable(ctrlq_info);
	if (ret != 0) {
		sdk_err(SSS_TO_HWDEV(ctrlq_info)->dev_hdl, "Ctrlq is disable\n");
		return ret;
	}

	ret = sss_ctrlq_sync_cmd_direct_reply(&ctrlq_info->ctrlq[SSS_CTRLQ_SYNC],
					      mod, cmd, in_buf, out_param, timeout, channel);

	if (!(((struct sss_hwdev *)hwdev)->chip_present_flag))
		return -ETIMEDOUT;
	else
		return ret;
}
EXPORT_SYMBOL(sss_ctrlq_direct_reply);

int sss_ctrlq_detail_reply(void *hwdev, u8 mod, u8 cmd,
			   struct sss_ctrl_msg_buf *in_buf, struct sss_ctrl_msg_buf *out_buf,
			   u64 *out_param, u32 timeout, u16 channel)
{
	int ret;
	struct sss_ctrlq_info *ctrlq_info = NULL;

	ret = sss_check_ctrlq_param(hwdev, in_buf);
	if (ret != 0)
		return ret;

	ctrlq_info = ((struct sss_hwdev *)hwdev)->ctrlq_info;

	if (!sss_chip_get_present_state((struct sss_hwdev *)hwdev))
		return -EPERM;

	ret = sss_wait_ctrlq_enable(ctrlq_info);
	if (ret != 0) {
		sdk_err(SSS_TO_HWDEV(ctrlq_info)->dev_hdl, "Ctrlq is disable\n");
		return ret;
	}

	ret = sss_ctrlq_sync_cmd_detail_reply(&ctrlq_info->ctrlq[SSS_CTRLQ_SYNC],
					      mod, cmd, in_buf, out_buf,
					      out_param, timeout, channel);
	if (!(((struct sss_hwdev *)hwdev)->chip_present_flag))
		return -ETIMEDOUT;
	else
		return ret;
}
EXPORT_SYMBOL(sss_ctrlq_detail_reply);

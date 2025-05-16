// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/vmalloc.h>

#include "ossl_knl.h"
#include "hinic3_hw.h"
#include "hinic3_mt.h"
#include "hinic3_hwdev.h"

#include "cqm_bitmap_table.h"
#include "cqm_bat_cla.h"
#include "cqm_main.h"
#include "cqm_cmd.h"

/**
 * cqm_cmd_alloc - Apply for a cmd buffer. The buffer size is fixed to 2 KB,
 *                 The buffer content is not cleared and needs to be cleared by services.
 * @ex_handle: device pointer that represents the PF
 */
struct tag_cqm_cmd_buf *cqm_cmd_alloc(void *ex_handle)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: ex_handle is null\n", __func__);
		return NULL;
	}

	atomic_inc(&handle->hw_stats.cqm_stats.cqm_cmd_alloc_cnt);

	return (struct tag_cqm_cmd_buf *)hinic3_alloc_cmd_buf(ex_handle);
}
EXPORT_SYMBOL(cqm_cmd_alloc);

/**
 * cqm_cmd_free - Release for a cmd buffer
 * @ex_handle: device pointer that represents the PF
 * @cmd_buf: command buffer
 */
void cqm_cmd_free(void *ex_handle, struct tag_cqm_cmd_buf *cmd_buf)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: ex_handle is null\n", __func__);
		return;
	}
	if (unlikely(!cmd_buf)) {
		pr_err("[CQM]%s: cmd_buf is null\n", __func__);
		return;
	}
	if (unlikely(!cmd_buf->buf)) {
		pr_err("[CQM]%s: buf is null\n", __func__);
		return;
	}

	atomic_inc(&handle->hw_stats.cqm_stats.cqm_cmd_free_cnt);

	hinic3_free_cmd_buf(ex_handle, (struct hinic3_cmd_buf *)cmd_buf);
}
EXPORT_SYMBOL(cqm_cmd_free);

/**
 * cqm_send_cmd_box - Send a cmd message in box mode,
 *                    This interface will mount a completion quantity, causing sleep.
 * @ex_handle: device pointer that represents the PF
 * @mod: command module
 * @cmd：command type
 * @buf_in: data buffer in address
 * @buf_out: data buffer out address
 * @out_param: out paramer
 * @timeout: out of time
 * @channel: mailbox channel
 */
s32 cqm_send_cmd_box(void *ex_handle, u8 mod, u8 cmd, struct tag_cqm_cmd_buf *buf_in,
		     struct tag_cqm_cmd_buf *buf_out, u64 *out_param, u32 timeout,
		     u16 channel)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: ex_handle is null\n", __func__);
		return CQM_FAIL;
	}
	if (unlikely(!buf_in)) {
		pr_err("[CQM]%s: buf_in is null\n", __func__);
		return CQM_FAIL;
	}
	if (unlikely(!buf_in->buf)) {
		pr_err("[CQM]%s: buf is null\n", __func__);
		return CQM_FAIL;
	}

	atomic_inc(&handle->hw_stats.cqm_stats.cqm_send_cmd_box_cnt);

	return hinic3_cmdq_detail_resp(ex_handle, mod, cmd,
				       (struct hinic3_cmd_buf *)buf_in,
				       (struct hinic3_cmd_buf *)buf_out,
				       out_param, timeout, channel);
}
EXPORT_SYMBOL(cqm_send_cmd_box);

/**
 * cqm_lb_send_cmd_box - Send a cmd message in box mode and open cos_id,
 *                       This interface will mount a completion quantity, causing sleep.
 * @ex_handle: device pointer that represents the PF
 * @mod: command module
 * @cmd：command type
 * @cos_id: cos id
 * @buf_in: data buffer in address
 * @buf_out: data buffer out address
 * @out_param: out paramer
 * @timeout: out of time
 * @channel: mailbox channel
 */
s32 cqm_lb_send_cmd_box(void *ex_handle, u8 mod, u8 cmd, u8 cos_id,
			struct tag_cqm_cmd_buf *buf_in, struct tag_cqm_cmd_buf *buf_out,
			u64 *out_param, u32 timeout, u16 channel)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: ex_handle is null\n", __func__);
		return CQM_FAIL;
	}
	if (unlikely(!buf_in)) {
		pr_err("[CQM]%s: buf_in is null\n", __func__);
		return CQM_FAIL;
	}
	if (unlikely(!buf_in->buf)) {
		pr_err("[CQM]%s: buf is null\n", __func__);
		return CQM_FAIL;
	}

	atomic_inc(&handle->hw_stats.cqm_stats.cqm_send_cmd_box_cnt);

	return hinic3_cos_id_detail_resp(ex_handle, mod, cmd, cos_id,
					 (struct hinic3_cmd_buf *)buf_in,
					 (struct hinic3_cmd_buf *)buf_out,
					 out_param, timeout, channel);
}
EXPORT_SYMBOL(cqm_lb_send_cmd_box);

/**
 * cqm_lb_send_cmd_box_async - Send a cmd message in box mode and open cos_id,
 *                             This interface will not wait completion
 * @ex_handle: device pointer that represents the PF
 * @mod: command module
 * @cmd：command type
 * @cos_id: cos id
 * @buf_in: data buffer in address
 * @channel: mailbox channel
 */
s32 cqm_lb_send_cmd_box_async(void *ex_handle, u8 mod, u8 cmd,
			      u8 cos_id, struct tag_cqm_cmd_buf *buf_in,
			      u16 channel)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: ex_handle is null\n", __func__);
		return CQM_FAIL;
	}
	if (unlikely(!buf_in)) {
		pr_err("[CQM]%s: buf_in is null\n", __func__);
		return CQM_FAIL;
	}
	if (unlikely(!buf_in->buf)) {
		pr_err("[CQM]%s: buf is null\n", __func__);
		return CQM_FAIL;
	}

	atomic_inc(&handle->hw_stats.cqm_stats.cqm_send_cmd_box_cnt);

	return hinic3_cmdq_async_cos(ex_handle, mod, cmd, cos_id,
				     (struct hinic3_cmd_buf *)buf_in, channel);
}
EXPORT_SYMBOL(cqm_lb_send_cmd_box_async);

// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <linux/types.h>
#include <linux/kernel.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_hwdev.h"
#include "sss_hwif_mbx.h"
#include "sss_hwif_export.h"

#define SSS_WAIT_CB_COMPLETE_MIN	900
#define SSS_WAIT_CB_COMPLETE_MAX	1000

int sss_register_pf_mbx_handler(void *hwdev, u8 mod, void *pri_handle, sss_pf_mbx_handler_t cb)
{
	struct sss_mbx *mbx = NULL;

	if (!hwdev || mod >= SSS_MOD_TYPE_MAX)
		return -EFAULT;

	mbx = ((struct sss_hwdev *)hwdev)->mbx;
	mbx->pf_mbx_cb[mod] = cb;
	mbx->pf_mbx_data[mod] = pri_handle;

	set_bit(SSS_PF_RECV_HANDLER_REG, &mbx->pf_mbx_cb_state[mod]);

	return 0;
}
EXPORT_SYMBOL(sss_register_pf_mbx_handler);

int sss_register_vf_mbx_handler(void *hwdev, u8 mod, void *pri_handle, sss_vf_mbx_handler_t cb)
{
	struct sss_mbx *mbx = NULL;

	if (!hwdev || mod >= SSS_MOD_TYPE_MAX)
		return -EFAULT;

	mbx = ((struct sss_hwdev *)hwdev)->mbx;
	mbx->vf_mbx_cb[mod] = cb;
	mbx->vf_mbx_data[mod] = pri_handle;

	set_bit(SSS_VF_RECV_HANDLER_REG, &mbx->vf_mbx_cb_state[mod]);

	return 0;
}
EXPORT_SYMBOL(sss_register_vf_mbx_handler);

void sss_unregister_pf_mbx_handler(void *hwdev, u8 mod)
{
	struct sss_mbx *mbx = NULL;

	if (!hwdev || mod >= SSS_MOD_TYPE_MAX)
		return;

	mbx = ((struct sss_hwdev *)hwdev)->mbx;

	clear_bit(SSS_PF_RECV_HANDLER_REG, &mbx->pf_mbx_cb_state[mod]);

	while (test_bit(SSS_PF_RECV_HANDLER_RUN, &mbx->pf_mbx_cb_state[mod]) != 0)
		usleep_range(SSS_WAIT_CB_COMPLETE_MIN, SSS_WAIT_CB_COMPLETE_MAX);

	mbx->pf_mbx_cb[mod] = NULL;
	mbx->pf_mbx_data[mod] = NULL;
}
EXPORT_SYMBOL(sss_unregister_pf_mbx_handler);

void sss_unregister_vf_mbx_handler(void *hwdev, u8 mod)
{
	struct sss_mbx *mbx = NULL;

	if (!hwdev || mod >= SSS_MOD_TYPE_MAX)
		return;

	mbx = ((struct sss_hwdev *)hwdev)->mbx;

	clear_bit(SSS_VF_RECV_HANDLER_REG, &mbx->vf_mbx_cb_state[mod]);

	while (test_bit(SSS_VF_RECV_HANDLER_RUN, &mbx->vf_mbx_cb_state[mod]) != 0)
		usleep_range(SSS_WAIT_CB_COMPLETE_MIN, SSS_WAIT_CB_COMPLETE_MAX);

	mbx->vf_mbx_cb[mod] = NULL;
	mbx->vf_mbx_data[mod] = NULL;
}
EXPORT_SYMBOL(sss_unregister_vf_mbx_handler);

int sss_mbx_send_to_pf(void *hwdev, u8 mod, u16 cmd, void *buf_in,
		       u16 in_size, void *buf_out, u16 *out_size, u32 timeout, u16 channel)
{
	struct sss_hwdev *dev = hwdev;
	int ret;

	if (!hwdev)
		return -EINVAL;

	if (!(dev->chip_present_flag))
		return -EPERM;

	ret = sss_check_mbx_param(dev->mbx, buf_in, in_size, channel);
	if (ret != 0)
		return ret;

	if (!SSS_IS_VF(dev)) {
		sdk_err(dev->dev_hdl, "Invalid func_type: %d\n",
			SSS_GET_FUNC_TYPE(dev));
		return -EINVAL;
	}

	return sss_send_mbx_to_func(dev->mbx, mod, cmd,
				    sss_get_pf_id_of_vf(dev), buf_in, in_size,
				    buf_out, out_size, timeout, channel);
}
EXPORT_SYMBOL(sss_mbx_send_to_pf);

int sss_mbx_send_to_vf(void *hwdev, u16 vf_id, u8 mod, u16 cmd, void *buf_in,
		       u16 in_size, void *buf_out, u16 *out_size, u32 timeout, u16 channel)
{
	struct sss_hwdev *dev = hwdev;
	int ret = 0;
	u16 dst_func_id;

	if (!hwdev)
		return -EINVAL;

	ret = sss_check_mbx_param(dev->mbx, buf_in, in_size, channel);
	if (ret != 0)
		return ret;

	if (SSS_IS_VF(dev)) {
		sdk_err(dev->dev_hdl, "Invalid func_type: %d\n",
			SSS_GET_FUNC_TYPE((struct sss_hwdev *)hwdev));
		return -EINVAL;
	}

	if (vf_id == 0) {
		sdk_err(dev->dev_hdl, "Invalid vf_id: %u\n", vf_id);
		return -EINVAL;
	}

	/* vf_offset_to_pf + vf_id is the vf's global function id of vf in
	 * this pf
	 */
	dst_func_id = sss_get_glb_pf_vf_offset(hwdev) + vf_id;

	return sss_send_mbx_to_func(dev->mbx, mod, cmd,
				    dst_func_id, buf_in, in_size,
				    buf_out, out_size, timeout, channel);
}
EXPORT_SYMBOL(sss_mbx_send_to_vf);

static int sss_send_mbx_to_mgmt(struct sss_hwdev *hwdev, u8 mod, u16 cmd,
				void *buf_in, u16 in_size, void *buf_out, u16 *out_size,
				u32 timeout, u16 channel)
{
	struct sss_mbx *func_to_func = hwdev->mbx;
	int ret;

	ret = sss_check_mbx_param(func_to_func, buf_in, in_size, channel);
	if (ret != 0)
		return ret;

	if (mod == SSS_MOD_TYPE_COMM && cmd == SSS_COMM_MGMT_CMD_SEND_API_ACK_BY_UP)
		return 0;

	return sss_send_mbx_to_func(func_to_func, mod, cmd, SSS_MGMT_SRC_ID,
				    buf_in, in_size, buf_out, out_size, timeout, channel);
}

int sss_sync_mbx_send_msg(void *hwdev, u8 mod, u16 cmd, void *buf_in,
			  u16 in_size, void *buf_out, u16 *out_size, u32 timeout, u16 channel)
{
	if (!hwdev)
		return -EINVAL;

	if (sss_get_dev_present_flag(hwdev) == 0)
		return -EPERM;

	return sss_send_mbx_to_mgmt(hwdev, mod, cmd, buf_in, in_size,
				    buf_out, out_size, timeout, channel);
}
EXPORT_SYMBOL(sss_sync_mbx_send_msg);

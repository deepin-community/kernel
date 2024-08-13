// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_hwdev.h"
#include "sss_hwif_ctrlq_init.h"
#include "sss_hwif_api.h"
#include "sss_hwif_mbx.h"
#include "sss_common.h"

#define SSS_FLR_TIMEOUT			1000
#define SSS_FLR_TIMEOUT_ONCE		10000

static enum sss_process_ret sss_check_flr_finish_handler(void *priv_data)
{
	struct sss_hwif *hwif = priv_data;
	enum sss_pf_status status;

	status = sss_chip_get_pf_status(hwif);
	if (status == SSS_PF_STATUS_FLR_FINISH_FLAG) {
		sss_chip_set_pf_status(hwif, SSS_PF_STATUS_ACTIVE_FLAG);
		return SSS_PROCESS_OK;
	}

	return SSS_PROCESS_DOING;
}

static int sss_wait_for_flr_finish(struct sss_hwif *hwif)
{
	return sss_check_handler_timeout(hwif, sss_check_flr_finish_handler,
					 SSS_FLR_TIMEOUT, SSS_FLR_TIMEOUT_ONCE);
}

static int sss_msg_to_mgmt_no_ack(void *hwdev, u8 mod, u16 cmd,
				  void *buf_in, u16 in_size, u16 channel)
{
	if (!hwdev)
		return -EINVAL;

	if (sss_get_dev_present_flag(hwdev) == 0)
		return -EPERM;

	return sss_send_mbx_to_mgmt_no_ack(hwdev, mod, cmd, buf_in,
					   in_size, channel);
}

static int sss_chip_flush_doorbell(struct sss_hwdev *hwdev, u16 channel)
{
	struct sss_hwif *hwif = hwdev->hwif;
	struct sss_cmd_clear_doorbell clear_db = {0};
	u16 out_len = sizeof(clear_db);
	int ret;

	clear_db.func_id = SSS_GET_HWIF_GLOBAL_ID(hwif);

	ret = sss_sync_send_msg_ch(hwdev, SSS_COMM_MGMT_CMD_FLUSH_DOORBELL,
				   &clear_db, sizeof(clear_db),
				   &clear_db, &out_len, channel);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &clear_db)) {
		sdk_warn(hwdev->dev_hdl,
			 "Fail to flush doorbell, ret: %d, status: 0x%x, out_size: 0x%x, channel: 0x%x\n",
			 ret, clear_db.head.state, out_len, channel);
		if (ret == 0)
			return -EFAULT;
	}

	return ret;
}

static int sss_chip_flush_resource(struct sss_hwdev *hwdev, u16 channel)
{
	struct sss_hwif *hwif = hwdev->hwif;
	struct sss_cmd_clear_resource clr_res = {0};
	int ret;

	clr_res.func_id = SSS_GET_HWIF_GLOBAL_ID(hwif);
	ret = sss_msg_to_mgmt_no_ack(hwdev, SSS_MOD_TYPE_COMM,
				     SSS_COMM_MGMT_CMD_START_FLUSH, &clr_res,
				     sizeof(clr_res), channel);
	if (ret != 0) {
		sdk_warn(hwdev->dev_hdl, "Fail to notice flush message, ret: %d, channel: 0x%x\n",
			 ret, channel);
	}

	return ret;
}

int sss_hwdev_flush_io(struct sss_hwdev *hwdev, u16 channel)
{
	struct sss_hwif *hwif = hwdev->hwif;
	int err;
	int ret = 0;

	if (hwdev->chip_present_flag == 0)
		return 0;

	if (SSS_GET_FUNC_TYPE(hwdev) != SSS_FUNC_TYPE_VF)
		msleep(100);

	err = sss_wait_ctrlq_stop(hwdev);
	if (err != 0) {
		sdk_warn(hwdev->dev_hdl, "Fail to wait ctrlq stop\n");
		ret = err;
	}

	sss_chip_disable_doorbell(hwif);

	err = sss_chip_flush_doorbell(hwdev, channel);
	if (err != 0)
		ret = err;

	if (SSS_GET_FUNC_TYPE(hwdev) != SSS_FUNC_TYPE_VF)
		sss_chip_set_pf_status(hwif, SSS_PF_STATUS_FLR_START_FLAG);
	else
		msleep(100);

	err = sss_chip_flush_resource(hwdev, channel);
	if (err != 0)
		ret = err;

	if (SSS_GET_FUNC_TYPE(hwdev) != SSS_FUNC_TYPE_VF) {
		err = sss_wait_for_flr_finish(hwif);
		if (err != 0) {
			sdk_warn(hwdev->dev_hdl, "Wait firmware FLR timeout\n");
			ret = err;
		}
	}

	sss_chip_enable_doorbell(hwif);

	err = sss_reinit_ctrlq_ctx(hwdev);
	if (err != 0) {
		sdk_warn(hwdev->dev_hdl, "Fail to reinit ctrlq ctx\n");
		ret = err;
	}

	return ret;
}

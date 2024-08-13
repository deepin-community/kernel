// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/msi.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_csr.h"
#include "sss_hwdev.h"
#include "sss_hwdev_api.h"
#include "sss_hwif_api.h"

int sss_chip_sync_time(void *hwdev, u64 mstime)
{
	int ret;
	struct sss_cmd_sync_time cmd_time = {0};
	u16 out_len = sizeof(cmd_time);

	cmd_time.mstime = mstime;
	ret = sss_sync_send_msg(hwdev, SSS_COMM_MGMT_CMD_SYNC_TIME, &cmd_time,
				sizeof(cmd_time), &cmd_time, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_time)) {
		sdk_err(SSS_TO_DEV(hwdev),
			"Fail to sync time, ret: %d, status: 0x%x, out_len: 0x%x\n",
			ret, cmd_time.head.state, out_len);
		return -EIO;
	}

	return 0;
}

void sss_chip_disable_mgmt_channel(void *hwdev)
{
	sss_chip_set_pf_status(SSS_TO_HWIF(hwdev), SSS_PF_STATUS_INIT);
}

int sss_chip_get_board_info(void *hwdev, struct sss_board_info *board_info)
{
	int ret;
	struct sss_cmd_board_info cmd_info = {0};
	u16 out_len = sizeof(cmd_info);

	ret = sss_sync_send_msg(hwdev, SSS_COMM_MGMT_CMD_GET_BOARD_INFO,
				&cmd_info, sizeof(cmd_info), &cmd_info, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_info)) {
		sdk_err(SSS_TO_DEV(hwdev),
			"Fail to get board info, ret: %d, status: 0x%x, out_len: 0x%x\n",
			ret, cmd_info.head.state, out_len);
		return -EIO;
	}

	memcpy(board_info, &cmd_info.info, sizeof(*board_info));

	return 0;
}

int sss_chip_do_nego_feature(void *hwdev, u8 opcode, u64 *feature, u16 feature_num)
{
	int ret;
	struct sss_cmd_feature_nego cmd_feature = {0};
	u16 out_len = sizeof(cmd_feature);

	cmd_feature.func_id = sss_get_global_func_id(hwdev);
	cmd_feature.opcode = opcode;
	if (opcode == SSS_MGMT_MSG_SET_CMD)
		memcpy(cmd_feature.feature, feature, (feature_num * sizeof(u64)));

	ret = sss_sync_send_msg(hwdev, SSS_COMM_MGMT_CMD_FEATURE_NEGO,
				&cmd_feature, sizeof(cmd_feature), &cmd_feature, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_feature)) {
		sdk_err(SSS_TO_DEV(hwdev),
			"Fail to nego feature, opcode: %d, ret: %d, status: 0x%x, out_len: 0x%x\n",
			opcode, ret, cmd_feature.head.state, out_len);
		return -EINVAL;
	}

	if (opcode == SSS_MGMT_MSG_GET_CMD)
		memcpy(feature, cmd_feature.feature, (feature_num * sizeof(u64)));

	return 0;
}

int sss_chip_set_pci_bdf_num(void *hwdev, u8 bus_id, u8 device_id, u8 func_id)
{
	int ret;
	struct sss_cmd_bdf_info cmd_bdf = {0};
	u16 out_len = sizeof(cmd_bdf);

	cmd_bdf.bus = bus_id;
	cmd_bdf.device = device_id;
	cmd_bdf.function = func_id;
	cmd_bdf.function_id = sss_get_global_func_id(hwdev);

	ret = sss_sync_send_msg(hwdev, SSS_COMM_MGMT_CMD_SEND_BDF_INFO,
				&cmd_bdf, sizeof(cmd_bdf), &cmd_bdf, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_bdf)) {
		sdk_err(SSS_TO_DEV(hwdev),
			"Fail to set bdf info, ret: %d, status: 0x%x, out_len: 0x%x\n",
			ret, cmd_bdf.head.state, out_len);
		return -EIO;
	}

	return 0;
}

int sss_chip_comm_channel_detect(struct sss_hwdev *hwdev)
{
	int ret;
	struct sss_cmd_channel_detect cmd_detect = {0};
	u16 out_len = sizeof(cmd_detect);

	if (!hwdev)
		return -EINVAL;

	cmd_detect.func_id = sss_get_global_func_id(hwdev);

	ret = sss_sync_send_msg(hwdev, SSS_COMM_MGMT_CMD_CHANNEL_DETECT,
				&cmd_detect, sizeof(cmd_detect), &cmd_detect, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_detect)) {
		sdk_err(hwdev->dev_hdl,
			"Fail to send channel detect, ret: %d, status: 0x%x, out_size: 0x%x\n",
			ret, cmd_detect.head.state, out_len);
		return -EINVAL;
	}

	return 0;
}

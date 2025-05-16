// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [COMM]" fmt

#include <linux/netlink.h>
#include <linux/pci.h>
#include <linux/firmware.h>

#include "hinic3_devlink.h"
#ifdef HAVE_DEVLINK_FLASH_UPDATE_PARAMS
#include "hinic3_common.h"
#include "hinic3_api_cmd.h"
#include "hinic3_mgmt.h"
#include "hinic3_hw.h"

static bool check_image_valid(struct hinic3_hwdev *hwdev, const u8 *buf,
			      u32 size, struct host_image *host_image)
{
	struct firmware_image *fw_image = NULL;
	u32 len = 0;
	u32 i;

	fw_image = (struct firmware_image *)buf;
	if (fw_image->fw_magic != FW_MAGIC_NUM) {
		sdk_err(hwdev->dev_hdl, "Wrong fw magic read from file, fw_magic: 0x%x\n",
			fw_image->fw_magic);
		return false;
	}

	if (fw_image->fw_info.section_cnt > FW_TYPE_MAX_NUM) {
		sdk_err(hwdev->dev_hdl, "Wrong fw type number read from file, fw_type_num: 0x%x\n",
			fw_image->fw_info.section_cnt);
		return false;
	}

	for (i = 0; i < fw_image->fw_info.section_cnt; i++) {
		len += fw_image->section_info[i].section_len;
		memcpy(&host_image->section_info[i], &fw_image->section_info[i],
		       sizeof(struct firmware_section));
	}

	if (len != fw_image->fw_len ||
	    (u32)(fw_image->fw_len + FW_IMAGE_HEAD_SIZE) != size) {
		sdk_err(hwdev->dev_hdl, "Wrong data size read from file\n");
		return false;
	}

	host_image->image_info.total_len = fw_image->fw_len;
	host_image->image_info.fw_version = fw_image->fw_version;
	host_image->type_num = fw_image->fw_info.section_cnt;
	host_image->device_id = fw_image->device_id;

	return true;
}

static bool check_image_integrity(struct hinic3_hwdev *hwdev, struct host_image *host_image)
{
	u64 collect_section_type = 0;
	u32 type, i;

	for (i = 0; i < host_image->type_num; i++) {
		type = host_image->section_info[i].section_type;
		if (collect_section_type & (1ULL << type)) {
			sdk_err(hwdev->dev_hdl, "Duplicate section type: %u\n", type);
			return false;
		}
		collect_section_type |= (1ULL << type);
	}

	if ((collect_section_type & IMAGE_COLD_SUB_MODULES_MUST_IN) ==
	    IMAGE_COLD_SUB_MODULES_MUST_IN &&
	    (collect_section_type & IMAGE_CFG_SUB_MODULES_MUST_IN) != 0)
		return true;

	sdk_err(hwdev->dev_hdl, "Failed to check file integrity, valid: 0x%llx, current: 0x%llx\n",
		(IMAGE_COLD_SUB_MODULES_MUST_IN | IMAGE_CFG_SUB_MODULES_MUST_IN),
		collect_section_type);

	return false;
}

static bool check_image_device_type(struct hinic3_hwdev *hwdev, u32 device_type)
{
	struct comm_cmd_board_info board_info;

	memset(&board_info, 0, sizeof(board_info));
	if (hinic3_get_board_info(hwdev, &board_info.info, HINIC3_CHANNEL_COMM)) {
		sdk_err(hwdev->dev_hdl, "Failed to get board info\n");
		return false;
	}

	if (device_type == board_info.info.board_type)
		return true;

	sdk_err(hwdev->dev_hdl, "The image device type: 0x%x doesn't match the firmware device type: 0x%x\n",
		device_type, board_info.info.board_type);

	return false;
}

static void encapsulate_update_cmd(struct hinic3_cmd_update_firmware *msg,
				   struct firmware_section *section_info,
				   const int *remain_len, u32 *send_len,
				   u32 *send_pos)
{
	memset(msg->data, 0, sizeof(msg->data));
	msg->ctl_info.sf = (*remain_len == section_info->section_len) ? true : false;
	msg->section_info.section_crc = section_info->section_crc;
	msg->section_info.section_type = section_info->section_type;
	msg->section_version = section_info->section_version;
	msg->section_len = section_info->section_len;
	msg->section_offset = *send_pos;
	msg->ctl_info.bit_signed = section_info->section_flag & 0x1;

	if (*remain_len <= FW_FRAGMENT_MAX_LEN) {
		msg->ctl_info.sl = true;
		msg->ctl_info.fragment_len = (u32)(*remain_len);
		*send_len += section_info->section_len;
	} else {
		msg->ctl_info.sl = false;
		msg->ctl_info.fragment_len = FW_FRAGMENT_MAX_LEN;
		*send_len += FW_FRAGMENT_MAX_LEN;
	}
}

static int hinic3_flash_firmware(struct hinic3_hwdev *hwdev, const u8 *data,
				 struct host_image *image)
{
	u32 send_pos, send_len, section_offset, i;
	struct hinic3_cmd_update_firmware *update_msg = NULL;
	u16 out_size = sizeof(*update_msg);
	bool total_flag = false;
	int remain_len, err;

	update_msg = kzalloc(sizeof(*update_msg), GFP_KERNEL);
	if (!update_msg)
		return -ENOMEM;

	for (i =  0; i < image->type_num; i++) {
		section_offset = image->section_info[i].section_offset;
		remain_len = (int)(image->section_info[i].section_len);
		send_len = 0;
		send_pos = 0;

		while (remain_len > 0) {
			if (!total_flag) {
				update_msg->total_len = image->image_info.total_len;
				total_flag = true;
			} else {
				update_msg->total_len = 0;
			}

			encapsulate_update_cmd(update_msg, &image->section_info[i],
					       &remain_len, &send_len, &send_pos);

			memcpy(update_msg->data,
			       ((data + FW_IMAGE_HEAD_SIZE) + section_offset) + send_pos,
			       update_msg->ctl_info.fragment_len);

			err = hinic3_pf_to_mgmt_sync(hwdev, HINIC3_MOD_COMM,
						     COMM_MGMT_CMD_UPDATE_FW,
						     update_msg, sizeof(*update_msg),
						     update_msg, &out_size,
						     FW_UPDATE_MGMT_TIMEOUT);
			if (err || !out_size || update_msg->msg_head.status) {
				sdk_err(hwdev->dev_hdl, "Failed to update firmware, err: %d, status: 0x%x, out size: 0x%x\n",
					err, update_msg->msg_head.status, out_size);
				err = update_msg->msg_head.status ?
				      update_msg->msg_head.status : -EIO;
				kfree(update_msg);
				return err;
			}

			send_pos = send_len;
			remain_len = (int)(image->section_info[i].section_len - send_len);
		}
	}

	kfree(update_msg);

	return 0;
}

static int hinic3_flash_update_notify(struct devlink *devlink, const struct firmware *fw,
				      struct host_image *image, struct netlink_ext_ack *extack)
{
	struct hinic3_devlink *devlink_dev = devlink_priv(devlink);
	struct hinic3_hwdev *hwdev = devlink_dev->hwdev;
	int err;

	devlink_flash_update_status_notify(devlink, "Flash firmware begin", NULL, 0, 0);
	sdk_info(hwdev->dev_hdl, "Flash firmware begin\n");
	err = hinic3_flash_firmware(hwdev, fw->data, image);
	if (err) {
		sdk_err(hwdev->dev_hdl, "Failed to flash firmware, err: %d\n", err);
		NL_SET_ERR_MSG_MOD(extack, "Flash firmware failed");
		devlink_flash_update_status_notify(devlink, "Flash firmware failed", NULL, 0, 0);
	} else {
		sdk_info(hwdev->dev_hdl, "Flash firmware end\n");
		devlink_flash_update_status_notify(devlink, "Flash firmware end", NULL, 0, 0);
	}

	return err;
}

#ifdef HAVE_DEVLINK_FW_FILE_NAME_PARAM
static int hinic3_devlink_flash_update(struct devlink *devlink, const char *file_name,
				       const char *component, struct netlink_ext_ack *extack)
#else
static int hinic3_devlink_flash_update(struct devlink *devlink,
				       struct devlink_flash_update_params *params,
				       struct netlink_ext_ack *extack)
#endif
{
	struct hinic3_devlink *devlink_dev = devlink_priv(devlink);
	struct hinic3_hwdev *hwdev = devlink_dev->hwdev;
	const struct firmware *fw = params->fw;
	struct host_image *image = NULL;
	int err;

	image = kzalloc(sizeof(*image), GFP_KERNEL);
	if (!image) {
		err = -ENOMEM;
		goto devlink_param_reset;
	}

	if (!check_image_valid(hwdev, fw->data, (u32)(fw->size), image) ||
	    !check_image_integrity(hwdev, image) ||
	    !check_image_device_type(hwdev, image->device_id)) {
		sdk_err(hwdev->dev_hdl, "Failed to check image\n");
		NL_SET_ERR_MSG_MOD(extack, "Check image failed");
		err = -EINVAL;
		goto devlink_update_out;
	}

	err = hinic3_flash_update_notify(devlink, fw, image, extack);

devlink_update_out:
	kfree(image);

devlink_param_reset:
	/* reset activate_fw and switch_cfg after flash update operation */
	devlink_dev->activate_fw = FW_CFG_DEFAULT_INDEX;
	devlink_dev->switch_cfg = FW_CFG_DEFAULT_INDEX;

	return err;
}

static const struct devlink_ops hinic3_devlink_ops = {
	.flash_update = hinic3_devlink_flash_update,
};

static int hinic3_devlink_get_activate_firmware_config(struct devlink *devlink, u32 id,
						       struct devlink_param_gset_ctx *ctx)
{
	struct hinic3_devlink *devlink_dev = devlink_priv(devlink);

	ctx->val.vu8 = devlink_dev->activate_fw;

	return 0;
}

static int hinic3_devlink_set_activate_firmware_config(struct devlink *devlink, u32 id,
						       struct devlink_param_gset_ctx *ctx)
{
	struct hinic3_devlink *devlink_dev = devlink_priv(devlink);
	struct hinic3_hwdev *hwdev = devlink_dev->hwdev;
	int err;

	devlink_dev->activate_fw = ctx->val.vu8;
	sdk_info(hwdev->dev_hdl, "Activate firmware begin\n");

	err = hinic3_activate_firmware(hwdev, devlink_dev->activate_fw);
	if (err) {
		sdk_err(hwdev->dev_hdl, "Failed to activate firmware, err: %d\n", err);
		return err;
	}

	sdk_info(hwdev->dev_hdl, "Activate firmware end\n");

	return 0;
}

static int hinic3_devlink_get_switch_config(struct devlink *devlink, u32 id,
					    struct devlink_param_gset_ctx *ctx)
{
	struct hinic3_devlink *devlink_dev = devlink_priv(devlink);

	ctx->val.vu8 = devlink_dev->switch_cfg;

	return 0;
}

static int hinic3_devlink_set_switch_config(struct devlink *devlink, u32 id,
					    struct devlink_param_gset_ctx *ctx)
{
	struct hinic3_devlink *devlink_dev = devlink_priv(devlink);
	struct hinic3_hwdev *hwdev = devlink_dev->hwdev;
	int err;

	devlink_dev->switch_cfg = ctx->val.vu8;
	sdk_info(hwdev->dev_hdl, "Switch cfg begin");

	err = hinic3_switch_config(hwdev, devlink_dev->switch_cfg);
	if (err) {
		sdk_err(hwdev->dev_hdl, "Failed to switch cfg, err: %d\n", err);
		return err;
	}

	sdk_info(hwdev->dev_hdl, "Switch cfg end\n");

	return 0;
}

static int hinic3_devlink_firmware_config_validate(struct devlink *devlink, u32 id,
						   union devlink_param_value val,
						   struct netlink_ext_ack *extack)
{
	struct hinic3_devlink *devlink_dev = devlink_priv(devlink);
	struct hinic3_hwdev *hwdev = devlink_dev->hwdev;
	u8 cfg_index = val.vu8;

	if (cfg_index > FW_CFG_MAX_INDEX) {
		sdk_err(hwdev->dev_hdl, "Firmware cfg index out of range [0,7]\n");
		NL_SET_ERR_MSG_MOD(extack, "Firmware cfg index out of range [0,7]");
		return -ERANGE;
	}

	return 0;
}

static const struct devlink_param hinic3_devlink_params[] = {
	DEVLINK_PARAM_DRIVER(HINIC3_DEVLINK_PARAM_ID_ACTIVATE_FW,
			     "activate_fw", DEVLINK_PARAM_TYPE_U8,
			     BIT(DEVLINK_PARAM_CMODE_PERMANENT),
			     hinic3_devlink_get_activate_firmware_config,
			     hinic3_devlink_set_activate_firmware_config,
			     hinic3_devlink_firmware_config_validate),
	DEVLINK_PARAM_DRIVER(HINIC3_DEVLINK_PARAM_ID_SWITCH_CFG,
			     "switch_cfg", DEVLINK_PARAM_TYPE_U8,
			     BIT(DEVLINK_PARAM_CMODE_PERMANENT),
			     hinic3_devlink_get_switch_config,
			     hinic3_devlink_set_switch_config,
			     hinic3_devlink_firmware_config_validate),
};

int hinic3_init_devlink(struct hinic3_hwdev *hwdev)
{
	struct devlink *devlink = NULL;
	struct pci_dev *pdev = NULL;
	int err;

	devlink = devlink_alloc(&hinic3_devlink_ops, sizeof(struct hinic3_devlink), hwdev->dev_hdl);
	if (!devlink) {
		sdk_err(hwdev->dev_hdl, "Failed to alloc devlink\n");
		return -ENOMEM;
	}

	hwdev->devlink_dev = devlink_priv(devlink);
	hwdev->devlink_dev->hwdev = hwdev;
	hwdev->devlink_dev->activate_fw = FW_CFG_DEFAULT_INDEX;
	hwdev->devlink_dev->switch_cfg = FW_CFG_DEFAULT_INDEX;

	pdev = hwdev->hwif->pdev;
	devlink_register(devlink);

	err = devlink_params_register(devlink, hinic3_devlink_params,
				      ARRAY_SIZE(hinic3_devlink_params));
	if (err) {
		sdk_err(hwdev->dev_hdl, "Failed to register devlink params\n");
		goto register_devlink_params_err;
	}

	return 0;

register_devlink_params_err:
	devlink_unregister(devlink);
	devlink_free(devlink);

	return -EFAULT;
}

void hinic3_uninit_devlink(struct hinic3_hwdev *hwdev)
{
	struct devlink *devlink = priv_to_devlink(hwdev->devlink_dev);

	devlink_params_unregister(devlink, hinic3_devlink_params,
				  ARRAY_SIZE(hinic3_devlink_params));
	devlink_unregister(devlink);
	devlink_free(devlink);
}
#endif

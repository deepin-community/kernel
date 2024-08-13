// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <linux/netlink.h>
#include <linux/pci.h>
#include <linux/firmware.h>

#include "sss_hwdev_link.h"
#ifdef HAVE_DEVLINK_FLASH_UPDATE_PARAMS
#include "sss_hw_common.h"
#include "sss_hwdev_api.h"
#include "sss_hwif_adm.h"
#include "sss_hwif_adm_common.h"

#define SSS_FW_MAGIC_NUM           0x5a5a1100
#define SSS_FW_IMAGE_HEAD_SIZE     4096
#define SSS_FW_FRAGMENT_MAX_LEN    1536
#define SSS_FW_CFG_DEFAULT_INDEX   0xFF
#define SSS_FW_UPDATE_MGMT_TIMEOUT 3000000U
#define SSS_FW_TYPE_MAX_NUM        0x40
#define SSS_FW_CFG_MAX_INDEX       8
#define SSS_FW_CFG_MIN_INDEX       1

#ifndef DEVLINK_SUPPORT_FLASH_UPDATE_COMPONENT
#define DEVLINK_SUPPORT_FLASH_UPDATE_COMPONENT          BIT(0)
#endif

enum sss_devlink_param_id {
	SSS_DEVLINK_PARAM_ID_BASE = DEVLINK_PARAM_GENERIC_ID_MAX,
	SSS_DEVLINK_PARAM_ID_ACTIVATE_FW,
	SSS_DEVLINK_PARAM_ID_SWITCH_CFG,
};

enum sss_firmware_type {
	SSS_UP_FW_UPDATE_MIN_TYPE1  = 0x0,
	SSS_UP_FW_UPDATE_UP_TEXT    = 0x0,
	SSS_UP_FW_UPDATE_UP_DATA    = 0x1,
	SSS_UP_FW_UPDATE_UP_DICT    = 0x2,
	SSS_UP_FW_UPDATE_TILE_PCPTR = 0x3,
	SSS_UP_FW_UPDATE_TILE_TEXT  = 0x4,
	SSS_UP_FW_UPDATE_TILE_DATA  = 0x5,
	SSS_UP_FW_UPDATE_TILE_DICT  = 0x6,
	SSS_UP_FW_UPDATE_PPE_STATE  = 0x7,
	SSS_UP_FW_UPDATE_PPE_BRANCH = 0x8,
	SSS_UP_FW_UPDATE_PPE_EXTACT = 0x9,
	SSS_UP_FW_UPDATE_MAX_TYPE1  = 0x9,
	SSS_UP_FW_UPDATE_CFG0       = 0xa,
	SSS_UP_FW_UPDATE_CFG1       = 0xb,
	SSS_UP_FW_UPDATE_CFG2       = 0xc,
	SSS_UP_FW_UPDATE_CFG3       = 0xd,
	SSS_UP_FW_UPDATE_MAX_TYPE1_CFG = 0xd,

	SSS_UP_FW_UPDATE_MIN_TYPE2  = 0x14,
	SSS_UP_FW_UPDATE_MAX_TYPE2  = 0x14,

	SSS_UP_FW_UPDATE_MIN_TYPE3  = 0x18,
	SSS_UP_FW_UPDATE_PHY        = 0x18,
	SSS_UP_FW_UPDATE_BIOS       = 0x19,
	SSS_UP_FW_UPDATE_HLINK_ONE  = 0x1a,
	SSS_UP_FW_UPDATE_HLINK_TWO  = 0x1b,
	SSS_UP_FW_UPDATE_HLINK_THR  = 0x1c,
	SSS_UP_FW_UPDATE_MAX_TYPE3  = 0x1c,

	SSS_UP_FW_UPDATE_MIN_TYPE4  = 0x20,
	SSS_UP_FW_UPDATE_L0FW       = 0x20,
	SSS_UP_FW_UPDATE_L1FW       = 0x21,
	SSS_UP_FW_UPDATE_BOOT       = 0x22,
	SSS_UP_FW_UPDATE_SEC_DICT   = 0x23,
	SSS_UP_FW_UPDATE_HOT_PATCH0 = 0x24,
	SSS_UP_FW_UPDATE_HOT_PATCH1 = 0x25,
	SSS_UP_FW_UPDATE_HOT_PATCH2 = 0x26,
	SSS_UP_FW_UPDATE_HOT_PATCH3 = 0x27,
	SSS_UP_FW_UPDATE_HOT_PATCH4 = 0x28,
	SSS_UP_FW_UPDATE_HOT_PATCH5 = 0x29,
	SSS_UP_FW_UPDATE_HOT_PATCH6 = 0x2a,
	SSS_UP_FW_UPDATE_HOT_PATCH7 = 0x2b,
	SSS_UP_FW_UPDATE_HOT_PATCH8 = 0x2c,
	SSS_UP_FW_UPDATE_HOT_PATCH9 = 0x2d,
	SSS_UP_FW_UPDATE_HOT_PATCH10 = 0x2e,
	SSS_UP_FW_UPDATE_HOT_PATCH11 = 0x2f,
	SSS_UP_FW_UPDATE_HOT_PATCH12 = 0x30,
	SSS_UP_FW_UPDATE_HOT_PATCH13 = 0x31,
	SSS_UP_FW_UPDATE_HOT_PATCH14 = 0x32,
	SSS_UP_FW_UPDATE_HOT_PATCH15 = 0x33,
	SSS_UP_FW_UPDATE_HOT_PATCH16 = 0x34,
	SSS_UP_FW_UPDATE_HOT_PATCH17 = 0x35,
	SSS_UP_FW_UPDATE_HOT_PATCH18 = 0x36,
	SSS_UP_FW_UPDATE_HOT_PATCH19 = 0x37,
	SSS_UP_FW_UPDATE_MAX_TYPE4   = 0x37,

	SSS_UP_FW_UPDATE_MIN_TYPE5  = 0x3a,
	SSS_UP_FW_UPDATE_OPTION_ROM = 0x3a,
	SSS_UP_FW_UPDATE_MAX_TYPE5  = 0x3a,

	SSS_UP_FW_UPDATE_MIN_TYPE6  = 0x3e,
	SSS_UP_FW_UPDATE_MAX_TYPE6  = 0x3e,

	SSS_UP_FW_UPDATE_MIN_TYPE7  = 0x40,
	SSS_UP_FW_UPDATE_MAX_TYPE7  = 0x40,
};

#define SSS_IMAGE_MPU_ALL_IN (BIT_ULL(SSS_UP_FW_UPDATE_UP_TEXT) | \
			BIT_ULL(SSS_UP_FW_UPDATE_UP_DATA) | \
			BIT_ULL(SSS_UP_FW_UPDATE_UP_DICT))

#define SSS_IMAGE_NPU_ALL_IN (BIT_ULL(SSS_UP_FW_UPDATE_TILE_PCPTR) | \
			BIT_ULL(SSS_UP_FW_UPDATE_TILE_TEXT) |  \
			BIT_ULL(SSS_UP_FW_UPDATE_TILE_DATA) |  \
			BIT_ULL(SSS_UP_FW_UPDATE_TILE_DICT) |  \
			BIT_ULL(SSS_UP_FW_UPDATE_PPE_STATE) |  \
			BIT_ULL(SSS_UP_FW_UPDATE_PPE_BRANCH) | \
			BIT_ULL(SSS_UP_FW_UPDATE_PPE_EXTACT))

#define SSS_IMAGE_COLD_ALL_IN (SSS_IMAGE_MPU_ALL_IN | SSS_IMAGE_NPU_ALL_IN)

#define SSS_IMAGE_CFG_ALL_IN (BIT_ULL(SSS_UP_FW_UPDATE_CFG0) | \
			BIT_ULL(SSS_UP_FW_UPDATE_CFG1) | \
			BIT_ULL(SSS_UP_FW_UPDATE_CFG2) | \
			BIT_ULL(SSS_UP_FW_UPDATE_CFG3))

#define SSS_CHECK_IMAGE_INTEGRATY(mask) \
			(((mask) & SSS_IMAGE_COLD_ALL_IN) == SSS_IMAGE_COLD_ALL_IN && \
			((mask) & SSS_IMAGE_CFG_ALL_IN) != 0)

#define SSS_LINK_HWDEV(link) \
		((struct sss_hwdev *)((struct sss_devlink *)devlink_priv(link))->hwdev)

struct sss_firmware_section {
	u32 section_len;
	u32 section_offset;
	u32 section_version;
	u32 section_type;
	u32 section_crc;
	u32 section_flag;
};

struct sss_firmware_image {
	u32 fw_version;
	u32 fw_len;
	u32 fw_magic;
	struct {
		u32 section_cnt : 16;
		u32 rsvd : 16;
	} fw_info;
	struct sss_firmware_section section_info[SSS_FW_TYPE_MAX_NUM];
	u32 device_id;
	u32 rsvd0[101];
	u32 rsvd1[534];
	u32 bin_data;
};

struct sss_host_image {
	struct sss_firmware_section section_info[SSS_FW_TYPE_MAX_NUM];
	struct {
		u32 total_len;
		u32 fw_version;
	} image_info;
	u32 section_cnt;
	u32 device_id;
};

struct sss_cmd_update_firmware {
	struct sss_mgmt_msg_head head;

	struct {
		u32 sl : 1;
		u32 sf : 1;
		u32 flag : 1;
		u32 bit_signed : 1;
		u32 reserved : 12;
		u32 fragment_len : 16;
	} ctl_info;

	struct {
		u32 section_crc;
		u32 section_type;
	} section_info;

	u32 total_len;
	u32 section_len;
	u32 section_version;
	u32 section_offset;
	u32 data[384];
};

struct sss_cmd_activate_firmware {
	struct sss_mgmt_msg_head head;
	u8 index; /* 0 ~ 7 */
	u8 data[7];
};

struct sss_cmd_switch_config {
	struct sss_mgmt_msg_head head;
	u8 index; /* 0 ~ 7 */
	u8 data[7];
};

static bool sss_check_image_valid(struct sss_hwdev *hwdev,
				  struct sss_firmware_image *image, u32 image_size)
{
	u32 i;
	u32 length = 0;
	u32 cnt;

	if (image->fw_magic != SSS_FW_MAGIC_NUM) {
		sdk_err(hwdev->dev_hdl, "Err fw magic: 0x%x read from file\n", image->fw_magic);
		return false;
	}

	cnt = image->fw_info.section_cnt;
	if (cnt > SSS_FW_TYPE_MAX_NUM) {
		sdk_err(hwdev->dev_hdl, "Err fw type num: 0x%x read from file\n", cnt);
		return false;
	}

	for (i = 0; i < cnt; i++)
		length += image->section_info[i].section_len;

	if (length != image->fw_len ||
	    (u32)(image->fw_len + SSS_FW_IMAGE_HEAD_SIZE) != image_size) {
		sdk_err(hwdev->dev_hdl, "Err data size: 0x%x read from file\n", length);
		return false;
	}

	return true;
}

static void sss_init_host_image(struct sss_host_image *host_image,
				struct sss_firmware_image *image)
{
	int i;

	for (i = 0; i < image->fw_info.section_cnt; i++) {
		memcpy(&host_image->section_info[i], &image->section_info[i],
		       sizeof(image->section_info[i]));
	}

	host_image->image_info.fw_version = image->fw_version;
	host_image->section_cnt = image->fw_info.section_cnt;
	host_image->device_id = image->device_id;
	host_image->image_info.total_len = image->fw_len;
}

static bool sss_check_image_integrity(struct sss_hwdev *hwdev,
				      struct sss_host_image *host_image)
{
	u32 i;
	u32 section_type;
	u64 mask = 0;

	for (i = 0; i < host_image->section_cnt; i++) {
		section_type = host_image->section_info[i].section_type;
		if (mask & (1ULL << section_type)) {
			sdk_err(hwdev->dev_hdl, "Duplicate section type: %u\n", section_type);
			return false;
		}
		mask |= (1ULL << section_type);
	}

	if (SSS_CHECK_IMAGE_INTEGRATY(mask))
		return true;

	sdk_err(hwdev->dev_hdl,
		"Fail to check file integrity, valid: 0x%llx, current: 0x%llx\n",
		(SSS_IMAGE_COLD_ALL_IN | SSS_IMAGE_CFG_ALL_IN), mask);

	return false;
}

static bool sss_check_image_device_id(struct sss_hwdev *hwdev, u32 dev_id)
{
	struct sss_cmd_board_info info = {0};

	if (sss_chip_get_board_info(hwdev, &info.info) != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to get board info\n");
		return false;
	}

	if (dev_id == info.info.board_type)
		return true;

	sdk_err(hwdev->dev_hdl,
		"The image device type: 0x%x don't match the fw dev id: 0x%x\n",
		dev_id, info.info.board_type);

	return false;
}

static void sss_init_update_cmd_param(struct sss_cmd_update_firmware *cmd_update,
				      struct sss_firmware_section *info, int remain,
				      u32 send_offset)
{
	cmd_update->ctl_info.sl = (remain <= SSS_FW_FRAGMENT_MAX_LEN) ? true : false;
	cmd_update->ctl_info.sf = (remain == info->section_len) ? true : false;
	cmd_update->ctl_info.bit_signed = info->section_flag & 0x1;
	cmd_update->ctl_info.fragment_len = min(remain, SSS_FW_FRAGMENT_MAX_LEN);

	cmd_update->section_info.section_crc = info->section_crc;
	cmd_update->section_info.section_type = info->section_type;

	cmd_update->section_version = info->section_version;
	cmd_update->section_len = info->section_len;
	cmd_update->section_offset = send_offset;
}

static int sss_chip_update_firmware(struct sss_hwdev *hwdev,
				    struct sss_cmd_update_firmware *cmd_update)
{
	int ret;
	u16 out_len = sizeof(*cmd_update);

	ret = sss_sync_send_adm_msg(hwdev, SSS_MOD_TYPE_COMM,
				    SSS_COMM_MGMT_CMD_UPDATE_FW, cmd_update, sizeof(*cmd_update),
				    cmd_update, &out_len, SSS_FW_UPDATE_MGMT_TIMEOUT);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, cmd_update)) {
		sdk_err(hwdev->dev_hdl,
			"Fail to update fw, ret: %d, status: 0x%x, out_len: 0x%x\n",
			ret, cmd_update->head.state, out_len);
		return (cmd_update->head.state != 0) ?
		       cmd_update->head.state : -EIO;
	}

	return 0;
}

static int sss_update_firmware(struct sss_hwdev *hwdev, const u8 *data,
			       struct sss_host_image *host_image)
{
	int ret;
	int remain;
	u32 i;
	u32 send_offset;
	u32 offset;
	bool flag = false;
	struct sss_cmd_update_firmware *cmd_update = NULL;

	cmd_update = kzalloc(sizeof(*cmd_update), GFP_KERNEL);
	if (!cmd_update)
		return -ENOMEM;

	for (i = 0; i < host_image->section_cnt; i++) {
		offset = host_image->section_info[i].section_offset;
		remain = (int)(host_image->section_info[i].section_len);
		send_offset = 0;

		while (remain > 0) {
			if (flag) {
				cmd_update->total_len = 0;
			} else {
				cmd_update->total_len = host_image->image_info.total_len;
				flag = true;
			}

			sss_init_update_cmd_param(cmd_update, &host_image->section_info[i],
						  remain, send_offset);

			memcpy(cmd_update->data,
			       ((data + SSS_FW_IMAGE_HEAD_SIZE) + offset) + send_offset,
			       cmd_update->ctl_info.fragment_len);

			ret = sss_chip_update_firmware(hwdev, cmd_update);
			if (ret != 0) {
				kfree(cmd_update);
				return ret;
			}

			send_offset += cmd_update->ctl_info.fragment_len;
			remain = (int)(host_image->section_info[i].section_len - send_offset);
		}
	}

	kfree(cmd_update);

	return 0;
}

static int sss_flash_update_notify(struct devlink *devlink,
				   const struct firmware *fw, struct sss_host_image *image,
				   struct netlink_ext_ack *extack)
{
	struct sss_devlink *devlink_dev = devlink_priv(devlink);
	struct sss_hwdev *hwdev = devlink_dev->hwdev;
	int ret;

#ifdef HAVE_DEVLINK_FW_FILE_NAME_MEMBER
	devlink_flash_update_begin_notify(devlink);
#endif
	devlink_flash_update_status_notify(devlink, "Flash firmware begin", NULL, 0, 0);
	sdk_info(hwdev->dev_hdl, "Flash firmware begin\n");
	ret = sss_update_firmware(hwdev, fw->data, image);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to flash firmware, ret: %d\n", ret);
		NL_SET_ERR_MSG_MOD(extack, "Fail to flash firmware");
		devlink_flash_update_status_notify(devlink, "Fail to flash firmware", NULL, 0, 0);
	} else {
		sdk_info(hwdev->dev_hdl, "Flash firmware end\n");
		devlink_flash_update_status_notify(devlink, "Flash firmware end", NULL, 0, 0);
	}
#ifdef HAVE_DEVLINK_FW_FILE_NAME_MEMBER
	devlink_flash_update_end_notify(devlink);
#endif

	return ret;
}

#ifdef HAVE_DEVLINK_FW_FILE_NAME_PARAM
static int sss_devlink_flash_update(struct devlink *link, const char *file_name,
				    const char *component, struct netlink_ext_ack *extack)
#else
static int sss_devlink_flash_update(struct devlink *link,
				    struct devlink_flash_update_params *param,
				    struct netlink_ext_ack *extack)
#endif
{
	int ret;
	struct sss_host_image *host_image = NULL;
	struct sss_devlink *link_dev = devlink_priv(link);
	struct sss_hwdev *hwdev = link_dev->hwdev;

#ifdef HAVE_DEVLINK_FW_FILE_NAME_MEMBER
	const struct firmware *fw = NULL;
#else
	const struct firmware *fw = param->fw;
#endif

	host_image = kzalloc(sizeof(*host_image), GFP_KERNEL);
	if (!host_image) {
		ret = -ENOMEM;
		goto alloc_host_image_err;
	}

#ifdef HAVE_DEVLINK_FW_FILE_NAME_PARAM
	ret = request_firmware_direct(&fw, file_name, hwdev->dev_hdl);
#else
#ifdef HAVE_DEVLINK_FW_FILE_NAME_MEMBER
	ret = request_firmware_direct(&fw, param->file_name, hwdev->dev_hdl);
#else
	ret = 0;
#endif
#endif
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to request firmware\n");
		goto request_fw_err;
	}

	if (!sss_check_image_valid(hwdev, (struct sss_firmware_image *)fw->data,
				   (u32)(fw->size))) {
		sdk_err(hwdev->dev_hdl, "Fail to check image valid\n");
		NL_SET_ERR_MSG_MOD(extack, "Fail to check image valid");
		ret = -EINVAL;
		goto check_image_err;
	}

	sss_init_host_image(host_image, (struct sss_firmware_image *)fw->data);

	if (!sss_check_image_integrity(hwdev, host_image)) {
		sdk_err(hwdev->dev_hdl, "Fail to check image integrity\n");
		NL_SET_ERR_MSG_MOD(extack, "Fail to check image integrity");
		ret = -EINVAL;
		goto check_image_err;
	}

	if (!sss_check_image_device_id(hwdev, host_image->device_id)) {
		sdk_err(hwdev->dev_hdl, "Fail to check image device id\n");
		NL_SET_ERR_MSG_MOD(extack, "Fail to check image device id");
		ret = -EINVAL;
		goto check_image_err;
	}

	ret = sss_flash_update_notify(link, fw, host_image, extack);

check_image_err:
#ifdef HAVE_DEVLINK_FW_FILE_NAME_PARAM
	release_firmware(fw);
#endif

request_fw_err:
	kfree(host_image);

alloc_host_image_err:
	link_dev->switch_cfg_id = SSS_FW_CFG_DEFAULT_INDEX;
	link_dev->active_cfg_id = SSS_FW_CFG_DEFAULT_INDEX;

	return ret;
}

static const struct devlink_ops g_devlink_ops = {
#ifdef DEVLINK_HAVE_SUPPORTED_FLASH_UPDATE_PARAMS
	.supported_flash_update_params = DEVLINK_SUPPORT_FLASH_UPDATE_COMPONENT,
#endif
	.flash_update = sss_devlink_flash_update,
};

static int sss_chip_activate_firmware(struct sss_hwdev *hwdev, u8 cfg_num)
{
	int ret;
	struct sss_cmd_activate_firmware cmd_activate = {0};
	u16 out_len = sizeof(cmd_activate);

	if (SSS_GET_FUNC_TYPE(hwdev) != SSS_FUNC_TYPE_PF &&
	    SSS_GET_FUNC_TYPE(hwdev) != SSS_FUNC_TYPE_PPF)
		return -EOPNOTSUPP;

	if (!SSS_SUPPORT_ADM_MSG(hwdev))
		return -EPERM;

	cmd_activate.index = cfg_num;

	ret = sss_sync_send_adm_msg(hwdev, SSS_MOD_TYPE_COMM, SSS_COMM_MGMT_CMD_ACTIVE_FW,
				    &cmd_activate, sizeof(cmd_activate), &cmd_activate,
				    &out_len, SSS_FW_UPDATE_MGMT_TIMEOUT);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_activate)) {
		sdk_err(hwdev->dev_hdl,
			"Fail to activate firmware, ret: %d, status: 0x%x, out_len: 0x%x\n",
			ret, cmd_activate.head.state, out_len);
		return (cmd_activate.head.state != 0) ?
		       cmd_activate.head.state : -EIO;
	}

	return 0;
}

static int sss_devlink_get_activate_fw_config(struct devlink *link, u32 id,
					      struct devlink_param_gset_ctx *param_ctx)
{
	struct sss_devlink *link_dev = devlink_priv(link);

	param_ctx->val.vu8 = link_dev->active_cfg_id;

	return 0;
}

static int sss_devlink_set_activate_fw_config(struct devlink *link, u32 id,
					      struct devlink_param_gset_ctx *param_ctx)
{
	int ret;
	struct sss_devlink *link_dev = devlink_priv(link);
	struct sss_hwdev *hwdev = link_dev->hwdev;

	link_dev->active_cfg_id = param_ctx->val.vu8;
	sdk_info(hwdev->dev_hdl, "Begin activate firmware\n");

	ret = sss_chip_activate_firmware(hwdev, link_dev->active_cfg_id - 1);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to activate firmware, ret: %d\n", ret);
		return ret;
	}

	sdk_info(hwdev->dev_hdl, "End activate firmware\n");

	return 0;
}

static int sss_chip_switch_config(struct sss_hwdev *hwdev, u8 cfg_num)
{
	int ret;
	struct sss_cmd_switch_config cmd_switch = {0};
	u16 out_len = sizeof(cmd_switch);

	if (SSS_GET_FUNC_TYPE(hwdev) != SSS_FUNC_TYPE_PF)
		return -EOPNOTSUPP;

	if (!SSS_SUPPORT_ADM_MSG(hwdev))
		return -EPERM;

	cmd_switch.index = cfg_num;

	ret = sss_sync_send_adm_msg(hwdev, SSS_MOD_TYPE_COMM, SSS_COMM_MGMT_CMD_SWITCH_CFG,
				    &cmd_switch, sizeof(cmd_switch), &cmd_switch,
				    &out_len, SSS_FW_UPDATE_MGMT_TIMEOUT);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_switch)) {
		sdk_err(hwdev->dev_hdl,
			"Fail to switch cfg, ret: %d, status: 0x%x, out_len: 0x%x\n",
			ret, cmd_switch.head.state, out_len);
		return (cmd_switch.head.state != 0) ?
		       cmd_switch.head.state : -EIO;
	}

	return 0;
}

static int sss_devlink_get_switch_config(struct devlink *link, u32 id,
					 struct devlink_param_gset_ctx *param_ctx)
{
	struct sss_devlink *link_dev = devlink_priv(link);

	param_ctx->val.vu8 = link_dev->switch_cfg_id;

	return 0;
}

static int sss_devlink_set_switch_config(struct devlink *link, u32 id,
					 struct devlink_param_gset_ctx *param_ctx)
{
	int ret;
	struct sss_devlink *link_dev = devlink_priv(link);
	struct sss_hwdev *hwdev = link_dev->hwdev;

	link_dev->switch_cfg_id = param_ctx->val.vu8;
	sdk_info(hwdev->dev_hdl, "Begin switch cfg");

	ret = sss_chip_switch_config(hwdev, link_dev->switch_cfg_id - 1);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to switch cfg, ret: %d\n", ret);
		return ret;
	}

	sdk_info(hwdev->dev_hdl, "End Switch cfg\n");

	return 0;
}

static int sss_devlink_validate_firmware_config(struct devlink *link, u32 id,
						union devlink_param_value param_val,
						struct netlink_ext_ack *ext_ack)
{
	struct sss_hwdev *hwdev = SSS_LINK_HWDEV(link);

	if (param_val.vu8 < SSS_FW_CFG_MIN_INDEX ||
	    param_val.vu8 > SSS_FW_CFG_MAX_INDEX) {
		sdk_err(hwdev->dev_hdl, "Firmware cfg id out of range [1,8]\n");
		NL_SET_ERR_MSG_MOD(ext_ack, "Firmware cfg id out of range [1,8]\n");
		return -ERANGE;
	}

	return 0;
}

static const struct devlink_param g_devlink_param[] = {
	DEVLINK_PARAM_DRIVER(SSS_DEVLINK_PARAM_ID_ACTIVATE_FW,
			     "activate_fw", DEVLINK_PARAM_TYPE_U8,
			     BIT(DEVLINK_PARAM_CMODE_PERMANENT),
			     sss_devlink_get_activate_fw_config,
			     sss_devlink_set_activate_fw_config,
			     sss_devlink_validate_firmware_config),
	DEVLINK_PARAM_DRIVER(SSS_DEVLINK_PARAM_ID_SWITCH_CFG,
			     "switch_cfg", DEVLINK_PARAM_TYPE_U8,
			     BIT(DEVLINK_PARAM_CMODE_PERMANENT),
			     sss_devlink_get_switch_config,
			     sss_devlink_set_switch_config,
			     sss_devlink_validate_firmware_config),
};

int sss_init_devlink(struct sss_hwdev *hwdev)
{
	int ret;
	struct devlink *link = NULL;
	struct pci_dev *pdev = hwdev->pcidev_hdl;

#ifdef HAS_DEVLINK_ALLOC_SETS_DEV
	link = devlink_alloc(&g_devlink_ops, sizeof(struct sss_devlink), &pdev->dev);
#else
	link = devlink_alloc(&g_devlink_ops, sizeof(struct sss_devlink));
#endif
	if (!link) {
		sdk_err(hwdev->dev_hdl, "Fail to alloc devlink\n");
		return -ENOMEM;
	}

	hwdev->devlink_dev = devlink_priv(link);
	hwdev->devlink_dev->hwdev = hwdev;
	hwdev->devlink_dev->switch_cfg_id = SSS_FW_CFG_DEFAULT_INDEX;
	hwdev->devlink_dev->active_cfg_id = SSS_FW_CFG_DEFAULT_INDEX;

#ifdef REGISTER_DEVLINK_PARAMETER_PREFERRED
	ret = devlink_params_register(devlink, g_devlink_param,
				      ARRAY_SIZE(g_devlink_param));
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to register devlink param\n");
		goto register_err;
	}
#endif

#ifdef NO_DEVLINK_REGISTER_SETS_DEV
#ifdef DEVLINK_REGISTER_RETURN_VOID
	devlink_register(link);
	ret = 0;
#else
	ret = devlink_register(link);
#endif

#else
	ret = devlink_register(link, &pdev->dev);
#endif
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to register devlink\n");
#ifdef REGISTER_DEVLINK_PARAMETER_PREFERRED
		devlink_params_unregister(devlink, g_devlink_param,
					  ARRAY_SIZE(g_devlink_param));
#endif
		goto register_err;
	}

#ifndef REGISTER_DEVLINK_PARAMETER_PREFERRED
	ret = devlink_params_register(link, g_devlink_param,
				      ARRAY_SIZE(g_devlink_param));
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to register devlink param\n");
		goto register_param_err;
	}
#endif
	devlink_params_publish(link);

	return 0;

#ifndef REGISTER_DEVLINK_PARAMETER_PREFERRED
register_param_err:
	devlink_unregister(link);
#endif

register_err:
	devlink_free(link);

	return -EFAULT;
}

void sss_deinit_devlink(struct sss_hwdev *hwdev)
{
	struct devlink *link = priv_to_devlink(hwdev->devlink_dev);

	devlink_params_unpublish(link);
	devlink_params_unregister(link, g_devlink_param,
				  ARRAY_SIZE(g_devlink_param));
	devlink_unregister(link);
	devlink_free(link);
}
#endif

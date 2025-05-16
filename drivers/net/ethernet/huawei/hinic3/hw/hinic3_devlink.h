/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_DEVLINK_H
#define HINIC3_DEVLINK_H

#include "ossl_knl.h"
#include "hinic3_hwdev.h"

#define FW_MAGIC_NUM           0x5a5a1100
#define FW_IMAGE_HEAD_SIZE     4096
#define FW_FRAGMENT_MAX_LEN    1536
#define FW_CFG_DEFAULT_INDEX   0xFF
#define FW_TYPE_MAX_NUM        0x40
#define FW_CFG_MAX_INDEX       7

#ifdef HAVE_DEVLINK_FLASH_UPDATE_PARAMS
enum hinic3_devlink_param_id {
	HINIC3_DEVLINK_PARAM_ID_BASE = DEVLINK_PARAM_GENERIC_ID_MAX,
	HINIC3_DEVLINK_PARAM_ID_ACTIVATE_FW,
	HINIC3_DEVLINK_PARAM_ID_SWITCH_CFG,
};
#endif

enum hinic3_firmware_type {
	UP_FW_UPDATE_MIN_TYPE1  = 0x0,
	UP_FW_UPDATE_UP_TEXT    = 0x0,
	UP_FW_UPDATE_UP_DATA    = 0x1,
	UP_FW_UPDATE_UP_DICT    = 0x2,
	UP_FW_UPDATE_TILE_PCPTR = 0x3,
	UP_FW_UPDATE_TILE_TEXT  = 0x4,
	UP_FW_UPDATE_TILE_DATA  = 0x5,
	UP_FW_UPDATE_TILE_DICT  = 0x6,
	UP_FW_UPDATE_PPE_STATE  = 0x7,
	UP_FW_UPDATE_PPE_BRANCH = 0x8,
	UP_FW_UPDATE_PPE_EXTACT = 0x9,
	UP_FW_UPDATE_MAX_TYPE1  = 0x9,
	UP_FW_UPDATE_CFG0       = 0xa,
	UP_FW_UPDATE_CFG1       = 0xb,
	UP_FW_UPDATE_CFG2       = 0xc,
	UP_FW_UPDATE_CFG3       = 0xd,
	UP_FW_UPDATE_MAX_TYPE1_CFG = 0xd,

	UP_FW_UPDATE_MIN_TYPE2  = 0x14,
	UP_FW_UPDATE_MAX_TYPE2  = 0x14,

	UP_FW_UPDATE_MIN_TYPE3  = 0x18,
	UP_FW_UPDATE_PHY        = 0x18,
	UP_FW_UPDATE_BIOS       = 0x19,
	UP_FW_UPDATE_HLINK_ONE  = 0x1a,
	UP_FW_UPDATE_HLINK_TWO  = 0x1b,
	UP_FW_UPDATE_HLINK_THR  = 0x1c,
	UP_FW_UPDATE_MAX_TYPE3  = 0x1c,

	UP_FW_UPDATE_MIN_TYPE4  = 0x20,
	UP_FW_UPDATE_L0FW       = 0x20,
	UP_FW_UPDATE_L1FW       = 0x21,
	UP_FW_UPDATE_BOOT       = 0x22,
	UP_FW_UPDATE_SEC_DICT   = 0x23,
	UP_FW_UPDATE_HOT_PATCH0 = 0x24,
	UP_FW_UPDATE_HOT_PATCH1 = 0x25,
	UP_FW_UPDATE_HOT_PATCH2 = 0x26,
	UP_FW_UPDATE_HOT_PATCH3 = 0x27,
	UP_FW_UPDATE_HOT_PATCH4 = 0x28,
	UP_FW_UPDATE_HOT_PATCH5 = 0x29,
	UP_FW_UPDATE_HOT_PATCH6 = 0x2a,
	UP_FW_UPDATE_HOT_PATCH7 = 0x2b,
	UP_FW_UPDATE_HOT_PATCH8 = 0x2c,
	UP_FW_UPDATE_HOT_PATCH9 = 0x2d,
	UP_FW_UPDATE_HOT_PATCH10 = 0x2e,
	UP_FW_UPDATE_HOT_PATCH11 = 0x2f,
	UP_FW_UPDATE_HOT_PATCH12 = 0x30,
	UP_FW_UPDATE_HOT_PATCH13 = 0x31,
	UP_FW_UPDATE_HOT_PATCH14 = 0x32,
	UP_FW_UPDATE_HOT_PATCH15 = 0x33,
	UP_FW_UPDATE_HOT_PATCH16 = 0x34,
	UP_FW_UPDATE_HOT_PATCH17 = 0x35,
	UP_FW_UPDATE_HOT_PATCH18 = 0x36,
	UP_FW_UPDATE_HOT_PATCH19 = 0x37,
	UP_FW_UPDATE_MAX_TYPE4   = 0x37,

	UP_FW_UPDATE_MIN_TYPE5  = 0x3a,
	UP_FW_UPDATE_OPTION_ROM = 0x3a,
	UP_FW_UPDATE_MAX_TYPE5  = 0x3a,

	UP_FW_UPDATE_MIN_TYPE6  = 0x3e,
	UP_FW_UPDATE_MAX_TYPE6  = 0x3e,

	UP_FW_UPDATE_MIN_TYPE7  = 0x40,
	UP_FW_UPDATE_MAX_TYPE7  = 0x40,
};

#define IMAGE_MPU_ALL_IN (BIT_ULL(UP_FW_UPDATE_UP_TEXT) | \
			  BIT_ULL(UP_FW_UPDATE_UP_DATA) | \
			  BIT_ULL(UP_FW_UPDATE_UP_DICT))

#define IMAGE_NPU_ALL_IN (BIT_ULL(UP_FW_UPDATE_TILE_PCPTR) | \
			  BIT_ULL(UP_FW_UPDATE_TILE_TEXT) |  \
			  BIT_ULL(UP_FW_UPDATE_TILE_DATA) |  \
			  BIT_ULL(UP_FW_UPDATE_TILE_DICT) |  \
			  BIT_ULL(UP_FW_UPDATE_PPE_STATE) |  \
			  BIT_ULL(UP_FW_UPDATE_PPE_BRANCH) | \
			  BIT_ULL(UP_FW_UPDATE_PPE_EXTACT))

#define IMAGE_COLD_SUB_MODULES_MUST_IN (IMAGE_MPU_ALL_IN | IMAGE_NPU_ALL_IN)

#define IMAGE_CFG_SUB_MODULES_MUST_IN (BIT_ULL(UP_FW_UPDATE_CFG0) | \
				       BIT_ULL(UP_FW_UPDATE_CFG1) | \
				       BIT_ULL(UP_FW_UPDATE_CFG2) | \
				       BIT_ULL(UP_FW_UPDATE_CFG3))

struct firmware_section {
	u32 section_len;
	u32 section_offset;
	u32 section_version;
	u32 section_type;
	u32 section_crc;
	u32 section_flag;
};

struct firmware_image {
	u32 fw_version;
	u32 fw_len;
	u32 fw_magic;
	struct {
		u32 section_cnt : 16;
		u32 rsvd : 16;
	} fw_info;
	struct firmware_section section_info[FW_TYPE_MAX_NUM];
	u32 device_id;   /* cfg fw board_type value */
	u32 rsvd0[101];  /* device_id and rsvd0[101] is update_head_extend_info */
	u32 rsvd1[534];  /* big bin file total size 4096B */
	u32 bin_data;    /* obtain the address for use */
};

struct host_image {
	struct firmware_section section_info[FW_TYPE_MAX_NUM];
	struct {
		u32 total_len;
		u32 fw_version;
	} image_info;
	u32 type_num;
	u32 device_id;
};

struct hinic3_cmd_update_firmware {
	struct mgmt_msg_head msg_head;

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

int hinic3_init_devlink(struct hinic3_hwdev *hwdev);
void hinic3_uninit_devlink(struct hinic3_hwdev *hwdev);

#endif

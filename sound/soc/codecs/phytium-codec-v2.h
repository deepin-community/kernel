/* SPDX-License-Identifier: GPL-2.0
 *
 * Phytium CODEC ASoC driver
 *
 * Copyright (C) 2023-2024, Phytium Technology Co., Ltd.
 *
 */

#ifndef __PHYTIUM_CODEC_V2_H
#define __PHYTIUM_CODEC_V2_H

#include <linux/io.h>
#include <sound/memalloc.h>

/****************register *****************/
#define PHYTIUM_CODEC_AP2RV_INT_MASK	0x20
#define PHYTIUM_CODEC_AP2RV_INT_STATE	0x24
	#define SEND_INTR		(1 << 4)

#define PHYTIUM_CODEC_DEBUG	0x58
	#define PHYTIUM_CODEC_DEBUG_ENABLE	(1 << 0)
	#define PHYTIUM_CODEC_ALIVE_ENABLE	(1 << 1)
	#define PHYTIUM_CODEC_HEARTBIT_VAL	(1 << 2)

#define PHYTIUM_CODEC_PLAYBACKE_VOL		0X500
#define PHYTIUM_CODEC_PLAYBACKE_OUT1_VOL	0X504
#define PHYTIUM_CODEC_PLAYBACKE_OUT2_VOL	0X508
#define PHYTIUM_CODEC_CAPTURE_VOL		0X50c
#define PHYTIUM_CODEC_CAPTURE_IN1_VOL		0X510
#define PHYTIUM_CODEC_HW_PARAM			0X514
#define PHYTIUM_CODEC_HW_PARAM_RC		0X518
#define PHYTIUM_CODEC_INMUX_ENABLE		0X51c
#define PHYTIUM_CODEC_INMUX_SEL			0X520
#define PHYTIUM_CODEC_ADC_ENABLE		0X524
#define PHYTIUM_CODEC_DAC_ENABLE		0x528
#define PHYTIUM_CODEC_INT_STATUS		0x560
#define PHYTIUM_CODEC_INT_MASK			0x564
#define PHYTIUM_CODEC_INT_ENABLE		0x590
	#define PIPE_NUM			11

#define REG_MAX	0x52c
#define REG_SH_LEN	52

/****************register end *****************/

#define	PHYTCODEC_FORMAT_S16	16
#define	PHYTCODEC_FORMAT_S18	18
#define	PHYTCODEC_FORMAT_S20	20
#define	PHYTCODEC_FORMAT_S24	24
#define	PHYTCODEC_FORMAT_S32	32

#define PHYTIUM_CODEC_LSD_ID		0x701

enum phytcodec_msg_cmd_id {
	PHYTCODEC_MSG_CMD_DEFAULT = 0,
	PHYTCODEC_MSG_CMD_SET,
	PHYTCODEC_MSG_CMD_GET,
	PHYTCODEC_MSG_CMD_DATA,
	PHYTCODEC_MSG_CMD_REPORT,
	PHYTCODEC_MSG_PROTOCOL = 0x10,
};

enum phytcodec_get_subid {
	PHYTCODEC_MSG_CMD_GET_CHANNELS = 0,
	PHYTCODEC_MSG_CMD_GET_ONE_REG,
	PHYTCODEC_MSG_CMD_GET_ALL_REGS,
};

enum phytcodec_set_subid {
	PHYTCODEC_MSG_CMD_SET_PROBE = 0,
	PHYTCODEC_MSG_CMD_SET_REMOVE,
	PHYTCODEC_MSG_CMD_SET_SUSPEND,
	PHYTCODEC_MSG_CMD_SET_RESUME,
	PHYTCODEC_MSG_CMD_SET_STARTUP,
	PHYTCODEC_MSG_CMD_SET_STARTUP_RC,
	PHYTCODEC_MSG_CMD_SET_MUTE,
	PHYTCODEC_MSG_CMD_SET_UNMUTE,
	PHYTCODEC_MSG_CMD_SET_DAI_FMT,
	PHYTCODEC_MSG_CMD_SET_BIAS_ON,
	PHYTCODEC_MSG_CMD_SET_BIAS_OFF,
	PHYTCODEC_MSG_CMD_SET_BIAS_PREPARE,
	PHYTCODEC_MSG_CMD_SET_BIAS_STANDBY,
	PHYTCODEC_MSG_CMD_SET_SHUTDOWN,
	PHYTCODEC_MSG_CMD_SET_SHUTDOWN_RC,
	PHYTCODEC_MSG_CMD_SET_ONE_REG,
};

enum phytcodec_complete {
	PHYTCODEC_COMPLETE_NOT_READY = 0,
	PHYTCODEC_COMPLETE_SUCCESS,
	PHYTCODEC_COMPLETE_GOING,
	PHYTCODEC_COMPLETE_GENERIC_ERROR = 0x10,
	PHYTCODEC_COMPLETE_TYPE_NOT_SUPPORTED,
	PHYTCODEC_COMPLETE_CMD_NOT_SUPPORTED,
	PHYTCODEC_COMPLETE_INVALID_PARAMETERS,
};

struct phytcodec_rw_data {
	uint8_t addr;
	uint8_t reg;
	uint16_t val;
};

struct phytcodec_reg {
	uint16_t total_regs_len;
	uint8_t one_reg_len;
	uint8_t cnt;
	uint8_t regs[52];
};

struct phytcodec_cmd {
	uint8_t reserved;
	uint8_t seq;
	uint8_t cmd_id;
	uint8_t cmd_subid;
	uint16_t len;
	uint8_t status;
	uint8_t complete;
	union {
		uint8_t para[56];
		struct phytcodec_reg phytcodec_reg;
		struct phytcodec_rw_data rw_data;
	} cmd_para;
};

struct phytium_codec {
	void __iomem *regfile_base;
	void __iomem *sharemem_base;
	struct device *dev;
	uint8_t *regs;
	uint8_t channels;

	struct regmap *regmap;
	struct completion comp;
	bool deemph;

	bool debug_enabled;
	bool alive_enabled;
	struct timer_list timer;
	void (*heartbeat)(struct phytium_codec *priv);
};

static inline unsigned int
phyt_readl_reg(void *base, uint32_t offset)
{
	unsigned int data;

	data = readl(base + offset);

	return data;
}

static inline void
phyt_writel_reg(void *base, uint32_t offset, uint32_t data)
{
	writel(data, base + offset);
}
#endif

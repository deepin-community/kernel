/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2021 Loongson Technology Co., Ltd.
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __NCS8805_H__
#define __NCS8805_H__

#include <drm/drm_modes.h>

#define PHY_NAME  "NCS8805"

#define NCS8805_SLAVE1_ADDR  (0x70)
#define NCS8805_SLAVE2_ADDR  (0x75)
#define NCS8805_REG_NVID_HIG_ADDR  (0x8a)
#define NCS8805_REG_NVID_MID_ADDR  (0x8b)
#define NCS8805_REG_NVID_LOW_ADDR  (0x8c)

#define NCS8805_WORKING_ENABLE   (1)
#define NCS8805_WORKING_DISABLE  (0)

#define NCS8805_RESET_ENABLE   (1)
#define NCS8805_RESET_DISABLE  (0)

#define REG_NUM_MAX        (80)
#define RESOLUTION_NUM_MAX  (8)

/* NCS8805 resource */
struct ncs_reg_cell {
	unsigned char addr;    /* 7bit i2c addr */
	unsigned char offset;  /* offset. */
	unsigned char val;     /* reg val. */
} __attribute__((packed));

struct ncs_lcd_parameter {
	unsigned char auto_mvid_hig_max;  /* reg 0x8a */
	unsigned char auto_mvid_hig_min;  /* reg 0x8a */
	unsigned char auto_mvid_mid_max;  /* reg 0x8b */
	unsigned char auto_mvid_mid_min;  /* reg 0x8b */
	unsigned char auto_mvid_low_max;  /* reg 0x8c */
	unsigned char auto_mvid_low_min;  /* reg 0x8c */
} __attribute__((packed));

struct ncs_resolution_cfg {
	unsigned int hdisplay;
	unsigned int vdisplay;
	struct ncs_lcd_parameter lcd_p;
	unsigned int reg_number;
	struct ncs_reg_cell  reg_list[REG_NUM_MAX];
} __attribute__((packed));

struct ncs_resources {
	/* major[23:16], minor[15:8], revision[7:0] */
	unsigned int version;
	unsigned char edid[EDID_LENGTH * 2];
	unsigned int resolution_number;
	struct ncs_resolution_cfg  resolution_list[RESOLUTION_NUM_MAX];
} __attribute__((packed));

/* NCS8805 struct  */
struct rx_timing {
	int hdisplay;
	int hsync_start;
	int hsync_end;
	int htotal;
	int hpolarity;

	int vdisplay;
	int vsync_start;
	int vsync_end;
	int vtotal;
	int vpolarity;
};

struct display_mode {
	struct drm_display_mode  mode;
};

#endif

/*
 * JM7200 GPU driver
 *
 * Copyright (c) 2018 ChangSha JingJiaMicro Electronics Co., Ltd.
 *
 * Author:
 *      rfshen <jjwgpu@jingjiamicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifndef __MWV206_H__
#define __MWV206_H__

#ifdef __cplusplus
extern "C" {
#endif

#define JJ_VENDOR_ID         0x0731
#define MWV206_DEVICE_ID     0x7200

#define MWV206_MASTERDEV_NO 0
#define MWV206_SLAVEDEV_NO 1
#define MWV206_MAXGPUCNT 2

#define V206CONFIG005   2
#define V206CONFIG006  4
#define V206CONFIG007  2
#define V206CONFIG008   2

#define MWV206_FB0_BASEADDR 0x00000000
#define MWV206_FB1_BASEADDR 0x80000000
#define MWV206_AXIMAP_FB0_BASEADDR 0x40000000
#define MWV206_AXIMAP_FB1_BASEADDR 0xA0000000


typedef enum {
	V206API038 = 0,
	V206API039,
	V206API040,
	V206API041,
	V206API042,
} V206API043;

typedef enum {
	V206API044 = 0,
	V206API045,
	V206API046,
} V206API047;

typedef enum {
	V206API048 = 0,
	V206API049,
	V206API050,
	V206API051,
} V206API052;

typedef enum V206IOCTL109 {
	V206IOCTL005 = 0,
	V206IOCTL006,
	V206IOCTL007,
	V206IOCTL008,
	V206IOCTL009,
	V206IOCTL010,
	V206IOCTL011,
	V206IOCTL012,
	V206IOCTL013,
	V206IOCTL014,
	V206IOCTL015
} e_mwv206_pll_t;

typedef enum {
	V206CONFIG011 = 0,
	V206CONFIG012,
	V206CONFIG013,
	V206CONFIG014,
} e_MWV206OutputType;

typedef enum output_port {
	MWV206_DP_DVO_0 = 0,
	MWV206_DP_DVO_1,
	MWV206_DP_DAC_0,
	MWV206_DP_DAC_1,
	MWV206_DP_LVDS_0,
	MWV206_DP_LVDS_1,
	MWV206_DP_HDMI_0,
	MWV206_DP_HDMI_1,
	MWV206_DP_HDMI_2,
	MWV206_DP_HDMI_3,
	MWV206_DP_COUNT,
} e_outputPort;

typedef enum pol_param {
	MWV206_POSITIVE = 0,
	MWV206_NEGATIVE = 1,
} e_pol_t;

typedef enum pixel_mode {
	V206FB002 = 0,
	V206FB003
} e_pixel_mode_t;

#ifdef __cplusplus
}
#endif

#endif
// SPDX-License-Identifier: GPL-2.0-only
/*
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from Arm Technology (China) Co., Ltd.
 *
 *            (C) COPYRIGHT 2021-2021 Arm Technology (China) Co., Ltd.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from Arm Technology (China) Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/dma-buf.h>
#include <linux/rtc.h>
#include <linux/mvx-v4l2-controls.h>
#include <linux/vmalloc.h>
#include "mvx_bitops.h"
#include "mvx_firmware.h"
#include "mvx_firmware_cache.h"
#include "mvx_session.h"
#include "mvx_seq.h"
#include "mvx_secure.h"

/******************************************************************************
 * Defines
 ******************************************************************************/

#define FRAC_TO_Q16(num, den) ((uint32_t)((((uint64_t)(num)) << 16) / (den)))

/* Limit bitstream size to 256M as VPU VA space is only 1280M for INTBUFS and BITBUFS */
#define MAX_BITSTREAM_BUFFER_SIZE (1 << 28)

#define MAX_RT_FPS_FRAMES (1 << 9)
#define FPS_SKIP_FRAMES 200

/****************************************************************************
 * Private variables
 ****************************************************************************/

static int session_watchdog_timeout = 100;
module_param(session_watchdog_timeout, int, 0660);

static int fw_watchdog_timeout;
module_param(fw_watchdog_timeout, int, 0660);

static int wait_pending_timeout = 3000;	/* 3s should be enough in worst case - 32Kx32K decode */
module_param(wait_pending_timeout, int, 0660);

static bool enable_buffer_dump;
module_param(enable_buffer_dump, bool, 0660);

/****************************************************************************
 * Private functions and variables
 ****************************************************************************/
static struct mvx_session_format_map mvx_compressed_fmts[] = {
	{.format = MVX_FORMAT_AVS,
	 .flags = V4L2_FMT_FLAG_COMPRESSED,
	 .pixelformat = V4L2_PIX_FMT_AVS,
	 .description = "AVS" },
	{.format = MVX_FORMAT_AVS2,
	 .flags = V4L2_FMT_FLAG_COMPRESSED,
	 .pixelformat = V4L2_PIX_FMT_AVS2,
	 .description = "AVS2" },
	{.format = MVX_FORMAT_H263,
	 .flags = V4L2_FMT_FLAG_COMPRESSED,
	 .pixelformat = V4L2_PIX_FMT_H263,
	 .description = "H.263" },
	{.format = MVX_FORMAT_H264,
	 .flags = V4L2_FMT_FLAG_COMPRESSED,
	 .pixelformat = V4L2_PIX_FMT_H264,
	 .description = "H.264" },
	{.format = MVX_FORMAT_HEVC,
	 .flags = V4L2_FMT_FLAG_COMPRESSED,
	 .pixelformat = V4L2_PIX_FMT_HEVC,
	 .description = "HEVC" },
	{.format = MVX_FORMAT_MPEG2,
	 .flags = V4L2_FMT_FLAG_COMPRESSED,
	 .pixelformat = V4L2_PIX_FMT_MPEG2,
	 .description = "MPEG-2 ES" },
	{.format = MVX_FORMAT_MPEG4,
	 .flags = V4L2_FMT_FLAG_COMPRESSED,
	 .pixelformat = V4L2_PIX_FMT_MPEG4,
	 .description = "MPEG-4 part 2 ES" },
	{.format = MVX_FORMAT_VC1,
	 .flags = V4L2_FMT_FLAG_COMPRESSED,
	 .pixelformat = V4L2_PIX_FMT_VC1_ANNEX_G,
	 .description = "VC-1 (SMPTE 412M Annex G)" },
	{.format = MVX_FORMAT_VC1,
	 .flags = V4L2_FMT_FLAG_COMPRESSED,
	 .pixelformat = V4L2_PIX_FMT_VC1_ANNEX_L,
	 .description = "VC-1 (SMPTE 412M Annex L)" },
	{.format = MVX_FORMAT_VP8,
	 .flags = V4L2_FMT_FLAG_COMPRESSED,
	 .pixelformat = V4L2_PIX_FMT_VP8,
	 .description = "VP8" },
	{.format = MVX_FORMAT_VP9,
	 .flags = V4L2_FMT_FLAG_COMPRESSED,
	 .pixelformat = V4L2_PIX_FMT_VP9,
	 .description = "VP9" },
	{.format = MVX_FORMAT_AV1,
	 .flags = V4L2_FMT_FLAG_COMPRESSED,
	 .pixelformat = V4L2_PIX_FMT_AV1,
	 .description = "AV1" },
	{.format = MVX_FORMAT_JPEG,
	 .flags = V4L2_FMT_FLAG_COMPRESSED,
	 .pixelformat = V4L2_PIX_FMT_JPEG,
	 .description = "JPEG" },
	{.format = MVX_FORMAT_JPEG,
	 .flags = V4L2_FMT_FLAG_COMPRESSED,
	 .pixelformat = V4L2_PIX_FMT_MJPEG,
	 .description = "MJPEG" }
};

static struct mvx_session_format_map mvx_raw_fmts[] = {
	{.format = MVX_FORMAT_YUV420_AFBC_8,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_YUV420_AFBC_8,
	 .description = "YUV420 AFBC 8 bit",
	 .bitdepth = 8,
	 .bpp = 0,
	 .to10_format = MVX_FORMAT_YUV420_AFBC_10,
	 .to10_pixelformat = V4L2_PIX_FMT_YUV420_AFBC_10 },
	{.format = MVX_FORMAT_YUV420_AFBC_10,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_YUV420_AFBC_10,
	 .description = "YUV420 AFBC 10 bit",
	 .bitdepth = 10,
	 .bpp = 0,
	 .to8_format = MVX_FORMAT_YUV420_AFBC_8,
	 .to8_pixelformat = V4L2_PIX_FMT_YUV420_AFBC_8 },
	{.format = MVX_FORMAT_YUV422_AFBC_8,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_YUV422_AFBC_8,
	 .description = "YUV422 AFBC 8 bit",
	 .bitdepth = 8,
	 .bpp = 0,
	 .to10_format = MVX_FORMAT_YUV422_AFBC_10,
	 .to10_pixelformat = V4L2_PIX_FMT_YUV422_AFBC_10 },
	{.format = MVX_FORMAT_YUV422_AFBC_10,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_YUV422_AFBC_10,
	 .description = "YUV422 AFBC 10 bit",
	 .bitdepth = 10,
	 .bpp = 0,
	 .to8_format = MVX_FORMAT_YUV422_AFBC_8,
	 .to8_pixelformat = V4L2_PIX_FMT_YUV422_AFBC_8 },
	{.format = MVX_FORMAT_Y_AFBC_8,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_Y_AFBC_8,
	 .description = "GREY AFBC 8 bit",
	 .bitdepth = 8,
	 .bpp = 0,
	 .to10_format = MVX_FORMAT_Y_AFBC_10,
	 .to10_pixelformat = V4L2_PIX_FMT_Y_AFBC_10 },
	{.format = MVX_FORMAT_Y_AFBC_10,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_Y_AFBC_10,
	 .description = "GREY AFBC 10 bit",
	 .bitdepth = 10,
	 .bpp = 0,
	 .to8_format = MVX_FORMAT_Y_AFBC_8,
	 .to8_pixelformat = V4L2_PIX_FMT_Y_AFBC_8 },
	{.format = MVX_FORMAT_YUV420_NV12,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_NV12M,
	 .description = "Y/CbCr 4:2:0 (N-C)",
	 .bitdepth = 8,
	 .bpp = 12,
	 .to10_format = MVX_FORMAT_YUV420_P010,
	 .to10_pixelformat = V4L2_PIX_FMT_P010M },
	{.format = MVX_FORMAT_YUV420_NV12,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_NV12,
	 .description = "Y/CbCr 4:2:0",
	 .bitdepth = 8,
	 .bpp = 12,
	 .to10_format = MVX_FORMAT_YUV420_P010,
	 .to10_pixelformat = V4L2_PIX_FMT_P010 },
	{.format = MVX_FORMAT_YUV420_I420,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_YUV420M,
	 .description = "Planar YUV 4:2:0 (N-C)",
	 .bitdepth = 8,
	 .bpp = 12,
	 .to10_format = MVX_FORMAT_YUV420_P010,
	 .to10_pixelformat = V4L2_PIX_FMT_P010M },
	{.format = MVX_FORMAT_YUV420_I420,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_YUV420,
	 .description = "Planar YUV 4:2:0",
	 .bitdepth = 8,
	 .bpp = 12,
	 .to10_format = MVX_FORMAT_YUV420_P010,
	 .to10_pixelformat = V4L2_PIX_FMT_P010 },
	{.format = MVX_FORMAT_YUV420_NV21,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_NV21M,
	 .description = "Y/CrCb 4:2:0 (N-C)",
	 .bitdepth = 8,
	 .bpp = 12,
	 .to10_format = MVX_FORMAT_YUV420_P010,
	 .to10_pixelformat = V4L2_PIX_FMT_P010M },
	{.format = MVX_FORMAT_YUV420_NV21,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_NV21,
	 .description = "Y/CrCb 4:2:0",
	 .bitdepth = 8,
	 .bpp = 12,
	 .to10_format = MVX_FORMAT_YUV420_P010,
	 .to10_pixelformat = V4L2_PIX_FMT_P010 },
	{.format = MVX_FORMAT_YUV420_P010,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_P010M,
	 .description = "YUV 4:2:0 P010 (Microsoft format, N-C)",
	 .bitdepth = 10,
	 .bpp = 24,
	 .to8_format = MVX_FORMAT_YUV420_NV12,
	 .to8_pixelformat = V4L2_PIX_FMT_NV12M },
	{.format = MVX_FORMAT_YUV420_P010,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_P010,
	 .description = "YUV 4:2:0 P010 (Microsoft format)",
	 .bitdepth = 10,
	 .bpp = 24,
	 .to8_format = MVX_FORMAT_YUV420_NV12,
	 .to8_pixelformat = V4L2_PIX_FMT_NV12 },
	{.format = MVX_FORMAT_YUV420_Y0L2,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_Y0L2,
	 .description = "YUV 4:2:0 Y0L2 (ARM format)",
	 .bitdepth = 10,
	 .bpp = 24,
	 .to8_format = MVX_FORMAT_YUV420_NV12,
	 .to8_pixelformat = V4L2_PIX_FMT_NV12M },
	{.format = MVX_FORMAT_YUV420_AQB1,
	 .flags = 0,
	 .pixelformat = v4l2_fourcc('Y', '0', 'A', 'B'),
	 .description = "YUV 4:2:0 AQB1 (ARM format)",
	 .bitdepth = 10,
	 .bpp = 24,
	 .to8_format = MVX_FORMAT_YUV420_NV12,
	 .to8_pixelformat = V4L2_PIX_FMT_NV12M },
	{.format = MVX_FORMAT_YUV422_YUY2,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_YUYV,
	 .description = "YYUV 4:2:2",
	 .bitdepth = 8,
	 .bpp = 16,
	 .to10_format = MVX_FORMAT_YUV422_1P_10,
	 .to10_pixelformat = V4L2_PIX_FMT_YUV422_1P_10 },
	{.format = MVX_FORMAT_YUV422_UYVY,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_UYVY,
	 .description = "UYVY 4:2:2",
	 .bitdepth = 8,
	 .bpp = 16,
	 .to10_format = MVX_FORMAT_YUV422_1P_10,
	 .to10_pixelformat = V4L2_PIX_FMT_YUV422_1P_10 },
	{.format = MVX_FORMAT_YUV422_Y210,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_Y210,
	 .description = "YUV 4:2:2 Y210 (Microsoft format)",
	 .bitdepth = 8,
	 .bpp = 16,
	 .to10_format = MVX_FORMAT_YUV422_1P_10,
	 .to10_pixelformat = V4L2_PIX_FMT_YUV422_1P_10 },

	/* ARGB */
	{.format = MVX_FORMAT_ARGB_8888,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_ARGB32,
	 .description = "32-bit ARGB 8-8-8-8",
	 .bitdepth = 8,
	 .bpp = 32,
	 .to10_format = MVX_FORMAT_YUV420_P010,
	 .to10_pixelformat = V4L2_PIX_FMT_P010M },
	{.format = MVX_FORMAT_ARGB_8888,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_RGB32,
	 .description = "32-bit ARGB 8-8-8-8",
	 .bitdepth = 8,
	 .bpp = 32,
	 .to10_format = MVX_FORMAT_YUV420_P010,
	 .to10_pixelformat = V4L2_PIX_FMT_P010M },

	/* ABGR */
	{.format = MVX_FORMAT_ABGR_8888,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_BGRA32,
	 .description = "32-bit ABGR-8-8-8-8",
	 .bitdepth = 8,
	 .bpp = 32,
	 .to10_format = MVX_FORMAT_YUV420_P010,
	 .to10_pixelformat = V4L2_PIX_FMT_P010M },

	/* RGBA */
	{.format = MVX_FORMAT_RGBA_8888,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_RGBA32,
	 .description = "32-bit RGBA 8-8-8-8",
	 .bitdepth = 8,
	 .bpp = 32,
	 .to10_format = MVX_FORMAT_YUV420_P010,
	 .to10_pixelformat = V4L2_PIX_FMT_P010M },

	/* BGRA (new and legacy format) */
	{.format = MVX_FORMAT_BGRA_8888,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_ABGR32,
	 .description = "32-bit BGRA 8-8-8-8",
	 .bitdepth = 8,
	 .bpp = 32,
	 .to10_format = MVX_FORMAT_YUV420_P010,
	 .to10_pixelformat = V4L2_PIX_FMT_P010M },
	{.format = MVX_FORMAT_BGRA_8888,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_BGR32,
	 .description = "32-bit BGRA 8-8-8-8",
	 .bitdepth = 8,
	 .bpp = 32,
	 .to10_format = MVX_FORMAT_YUV420_P010,
	 .to10_pixelformat = V4L2_PIX_FMT_P010M },

	/* RGB888 */
	{.format = MVX_FORMAT_RGB_888,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_RGB24,
	 .description = "24-bit RGB 8-8-8",
	 .bitdepth = 8,
	 .bpp = 24,
	 .to10_format = MVX_FORMAT_YUV420_P010,
	 .to10_pixelformat = V4L2_PIX_FMT_P010M },

	/*BGR888 */
	{.format = MVX_FORMAT_BGR_888,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_BGR24,
	 .description = "24-bit BGR 8-8-8",
	 .bitdepth = 8,
	 .bpp = 32,
	 .to10_format = MVX_FORMAT_YUV420_P010,
	 .to10_pixelformat = V4L2_PIX_FMT_P010M },

	/*RGB888 3 PLANNER */
	{.format = MVX_FORMAT_RGB_888_3P,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_RGB_3P,
	 .description = "24-bit RGB 8-8-8 3PLANNER",
	 .bitdepth = 8,
	 .bpp = 32,
	 .to10_format = MVX_FORMAT_YUV420_P010,
	 .to10_pixelformat = V4L2_PIX_FMT_P010M },

	/*ARGB1555 1 PLANNER */
	{.format = MVX_FORMAT_ARGB_1555,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_ARGB555,
	 .description = "16-bit ARGB 1-5-5-5 1PLANNER",
	 .bitdepth = 8,
	 .bpp = 16,
	 .to10_format = MVX_FORMAT_YUV420_P010,
	 .to10_pixelformat = V4L2_PIX_FMT_P010M },

	/*ARGB 1 PLANNER */
	{.format = MVX_FORMAT_ARGB_4444,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_ARGB444,
	 .description = "16-bit ARGB 4-4-4-4 1PLANNER",
	 .bitdepth = 8,
	 .bpp = 16,
	 .to10_format = MVX_FORMAT_YUV420_P010,
	 .to10_pixelformat = V4L2_PIX_FMT_P010M },

	/*RGB888 3 PLANNER */
	{.format = MVX_FORMAT_RGB_565,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_RGB565,
	 .description = "16-bit RGB 5-6-5 1PLANNER",
	 .bitdepth = 8,
	 .bpp = 16,
	 .to10_format = MVX_FORMAT_YUV420_P010,
	 .to10_pixelformat = V4L2_PIX_FMT_P010M },

	/*MVX_FORMAT_Y 1 PLANNER */
	{.format = MVX_FORMAT_Y,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_GREY,
	 .description = "8-bit GREY 1PLANNER",
	 .bitdepth = 8,
	 .bpp = 8,
	 .to10_format = MVX_FORMAT_Y_10,
	 .to10_pixelformat = V4L2_PIX_FMT_Y10_LE },

	/*MVX_FORMAT_Y_10 1 PLANNER */
	{.format = MVX_FORMAT_Y_10,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_Y10_LE,
	 .description = "10-bit GREY 16BIT LSB 1PLANNER",
	 .bitdepth = 10,
	 .bpp = 16,
	 .to8_format = MVX_FORMAT_Y,
	 .to8_pixelformat = V4L2_PIX_FMT_GREY },

	/*MVX_FORMAT_YUV444 3 PLANNER */
	{.format = MVX_FORMAT_YUV444,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_YUV444M,
	 .description = "8-bit YUV444 3PLANNER",
	 .bitdepth = 8,
	 .bpp = 24,
	 .to10_format = MVX_FORMAT_YUV444_10,
	 .to10_pixelformat = V4L2_PIX_FMT_YUV444_10 },

	/*MVX_FORMAT_YUV444_10 3 PLANNER */
	{.format = MVX_FORMAT_YUV444_10,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_YUV444_10,
	 .description = "10-bit YUV444 16BIT LSB 3PLANNER",
	 .bitdepth = 10,
	 .bpp = 48,
	 .to8_format = MVX_FORMAT_YUV444,
	 .to8_pixelformat = V4L2_PIX_FMT_YUV444M },

	/*MVX_FORMAT_YUV420_2P_10 2 PLANNER */
	{.format = MVX_FORMAT_YUV420_2P_10,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_YUV420_2P_10,
	 .description = "10-bit YUV420 16BIT LSB 2PLANNER",
	 .bitdepth = 10,
	 .bpp = 24,
	 .to8_format = MVX_FORMAT_YUV420_NV12,
	 .to8_pixelformat = V4L2_PIX_FMT_NV12M },

	/*MVX_FORMAT_YUV422_1P_10 1 PLANNER */
	{.format = MVX_FORMAT_YUV422_1P_10,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_YUV422_1P_10,
	 .description = "10-bit YUV422 16BIT LSB 1PLANNER",
	 .bitdepth = 10,
	 .bpp = 32,
	 .to8_format = MVX_FORMAT_YUV422_YUY2,
	 .to8_pixelformat = V4L2_PIX_FMT_YUYV },

	/*MVX_FORMAT_YUV420_I420_10 3 PLANNER */
	{.format = MVX_FORMAT_YUV420_I420_10,
	 .flags = 0,
	 .pixelformat = V4L2_PIX_FMT_YUV420_I420_10,
	 .description = "10-bit YUV420 16BIT LSB 3PLANNER",
	 .bitdepth = 10,
	 .bpp = 24,
	 .to8_format = MVX_FORMAT_YUV420_I420,
	 .to8_pixelformat = V4L2_PIX_FMT_YUV420M }
};

static void watchdog_start(struct mvx_session *session,
			   unsigned int timeout_ms, bool reset_count)
{
	int ret;

	if (session->error != 0)
		return;

	MVX_SESSION_DEBUG(session,
			  "Watchdog start. timeout_ms=%u, reset_count=%d",
			  timeout_ms, reset_count);

	ret = mod_timer(&session->watchdog_timer,
			jiffies + msecs_to_jiffies(timeout_ms));
	if (ret != 0)
		return;

	if (reset_count)
		session->watchdog_count = 0;

	kref_get(&session->isession.kref);
}

static void watchdog_stop(struct mvx_session *session)
{
	int ret;

	ret = del_timer_sync(&session->watchdog_timer);

	/* ret: 0=watchdog expired, 1=watchdog still running */
	MVX_SESSION_DEBUG(session, "Watchdog stop. ret=%d", ret);

	/* Decrement the kref if the watchdog was still running. */
	if (ret != 0)
		kref_put(&session->isession.kref, session->isession.release);
}

static void watchdog_update(struct mvx_session *session,
			    unsigned int timeout_ms)
{
	int ret;

	ret = mod_timer_pending(&session->watchdog_timer,
				jiffies + msecs_to_jiffies(timeout_ms));

	/* ret: 0=no restart, 1=restarted */
	MVX_SESSION_DEBUG(session, "Watchdog update. ret=%d, timeout_ms=%u.",
			  ret, timeout_ms);
}

static bool is_fw_loaded(struct mvx_session *session)
{
	return (IS_ERR_OR_NULL(session->fw_bin) == false);
}

static void print_debug(struct mvx_session *session)
{
	MVX_SESSION_INFO(session, "Print debug.");

	if (session->csession != NULL)
		session->client_ops->print_debug(session->csession);

	if (is_fw_loaded(session))
		session->fw.ops.print_debug(&session->fw);
}

static void dump_ivf_header(struct mvx_session *session)
{
	struct mvx_session_port *input = &session->port[MVX_DIR_INPUT];
	bool is_ivf = (input->format == MVX_FORMAT_AV1 ||
		       input->format == MVX_FORMAT_VP8 ||
		       input->format == MVX_FORMAT_VP9);

	if (!current->fs) {
		MVX_SESSION_ERR(session,
				"process exit abnormally,fs has been released!!!");
		return;
	}

	if (is_ivf) {
		uint32_t alloc_bytes = MVE_PAGE_SIZE;
		struct mvx_session_port *output =
		    &session->port[MVX_DIR_OUTPUT];
		struct mvx_ivf_header ivf_header = {
			.signature = v4l2_fourcc('D', 'K', 'I', 'F'),
			.version = 0,
			.length = sizeof(struct mvx_ivf_header),
			.fourcc = input->pixelformat,
			.width = output->width,
			.height = output->height,
			.frameRate = (30 << 16),
			.timeScale = (1 << 16),
			.frameCount = input->dump_count,
			.padding = 0
		};
		uint8_t *ivf_header_buf = (uint8_t *) (&ivf_header);
		struct file *ivf_fp = NULL;
		char ivf_file[64];
		char data_file[64];

		scnprintf(ivf_file, sizeof(ivf_file) - 1,
			  "/data/input_session_%p.ivf", session);
		scnprintf(data_file, sizeof(data_file) - 1,
			  "/data/input_session_%p.bin", session);

		ivf_fp = filp_open(ivf_file, O_RDWR | O_CREAT, 0644);
		if (IS_ERR(ivf_fp)) {
			MVX_SESSION_WARN(session, "warning: open file(%s) fail",
					 ivf_file);
			return;
		}
		// write ivf header into new file at the beginning
		kernel_write(ivf_fp, ivf_header_buf,
			     sizeof(struct mvx_ivf_header), &(ivf_fp->f_pos));

		session->data_fp = filp_open(data_file, O_RDONLY, 0644);
		if (IS_ERR(session->data_fp)) {
			MVX_SESSION_WARN(session, "warning: open file(%s) fail",
					 data_file);
			filp_close(ivf_fp, NULL);
			return;
		}
		char *ivf_data = kmalloc(alloc_bytes, GFP_KERNEL);

		if (ivf_data != NULL) {
			size_t read_bytes;
			// write ivf data from data file read
			while (read_bytes =
			       kernel_read(session->data_fp, ivf_data,
					   alloc_bytes,
					   &(session->data_fp->f_pos)),
					read_bytes > 0) {
				kernel_write(ivf_fp, ivf_data,
					     read_bytes,
						&(ivf_fp->f_pos));
				if (read_bytes < alloc_bytes)
					break;
				}
			kfree(ivf_data);
			}
		filp_close(session->data_fp, NULL);
		filp_close(ivf_fp, NULL);
	}
}

static void send_event_error(struct mvx_session *session, long error)
{
	session->error = error;
	wake_up(&session->waitq);
	session->event(session, MVX_SESSION_EVENT_ERROR,
		       (void *)session->error);
}

static void mvx_session_construct_average_fps_msg(struct mvx_session *session,
						  int fps, uint64_t frame_count,
						  time64_t start_sec,
						  time64_t end_sec)
{
	if (mvx_log_perf.avgfps) {
		struct rtc_time start, end;
		enum mvx_direction dir =
		    session->is_encoder ? MVX_DIR_OUTPUT : MVX_DIR_INPUT;
		struct mvx_session_port *p = &session->port[dir];
		struct mvx_session_format_map *map =
		    mvx_session_find_format(p->pixelformat);

		rtc_time64_to_tm(start_sec, &start);
		rtc_time64_to_tm(end_sec, &end);

		mutex_lock(&mvx_log_perf.mutex);

		snprintf(mvx_log_perf.avgfps +
			 MVX_LOG_FPS_MSG_UNIT_SIZE * mvx_log_perf.fps_msg_w,
			 MVX_LOG_FPS_MSG_UNIT_SIZE,
			 "%02d:%02d:%02d ~ %02d:%02d:%02d [%p] P%d %s %s %dx%d %lld frames, average fps %d.%02d\n",
			 start.tm_hour, start.tm_min, start.tm_sec, end.tm_hour,
			 end.tm_min, end.tm_sec, session, session->priority,
			 map->description,
			 session->is_encoder ? "encoder" : "decoder", p->width,
			 p->height, frame_count, fps / 100, fps % 100);

		mvx_log_perf.fps_msg_w = (mvx_log_perf.fps_msg_w + 1) & 31;
		mvx_log_perf.has_update = true;

		mutex_unlock(&mvx_log_perf.mutex);
	}
}

static int mvx_session_calculate_average_fps(struct mvx_session *session,
					     int ts_index, uint64_t frame_count)
{
	struct timespec64 delta;
	uint64_t delta_us;

	frame_count -= FPS_SKIP_FRAMES;
	delta = timespec64_sub(session->ts[ts_index], session->start);
	delta_us = timespec64_to_ns(&delta) / 1000;
	return (int)((frame_count * 100 * 1000 * 1000) / delta_us);
}

static void mvx_session_update_average_fps(struct mvx_session *session)
{
	int ts_index, fps;
	uint64_t frame_count;

	if (!(mvx_log_perf.enabled & MVX_LOG_PERF_FPS) || !session->ts)
		return;

	mutex_lock(&session->fps_mutex);
	ts_index = session->ts_index;
	frame_count = session->frame_count;
	mutex_unlock(&session->fps_mutex);

	if (frame_count <= FPS_SKIP_FRAMES)
		return;

	ts_index = ts_index == 0 ? MAX_RT_FPS_FRAMES - 1 : ts_index - 1;
	fps = mvx_session_calculate_average_fps(session, ts_index, frame_count);
	mvx_session_construct_average_fps_msg(session,
					      fps, frame_count,
					      session->start.tv_sec,
					      session->ts[ts_index].tv_sec);
}

static void session_unregister(struct mvx_session *session)
{
	if (!IS_ERR_OR_NULL(session->csession)) {
		if (session->frame_count > FPS_SKIP_FRAMES && session->ts)
			mvx_session_update_average_fps(session);
		session->client_ops->unregister_session(session->csession);
		session->csession = NULL;
	}
}

static void release_fw_bin(struct mvx_session *session)
{
	if (is_fw_loaded(session) != false) {
		MVX_SESSION_INFO(session, "Release firmware binary.");

		mvx_fw_destruct(&session->fw);
		mvx_fw_cache_put(session->cache, session->fw_bin);
		session->fw_bin = NULL;
		complete(&session->fw_loaded);
	}

	watchdog_stop(session);
	session_unregister(session);
}

static struct mvx_session *kref_to_session(struct kref *kref)
{
	return container_of(kref, struct mvx_session, isession.kref);
}

static void session_destructor(struct kref *kref)
{
	struct mvx_session *session = kref_to_session(kref);

	session->destructor(session);
}

static const char *state_to_string(enum mvx_fw_state state)
{
	switch (state) {
	case MVX_FW_STATE_STOPPED:
		return "Stopped";
	case MVX_FW_STATE_RUNNING:
		return "Running";
	default:
		return "Unknown";
	}
}

static enum mvx_direction get_bitstream_port(struct mvx_session *session)
{
	if (mvx_is_bitstream(session->port[MVX_DIR_INPUT].format) &&
	    mvx_is_frame(session->port[MVX_DIR_OUTPUT].format))
		return MVX_DIR_INPUT;
	else if (mvx_is_frame(session->port[MVX_DIR_INPUT].format) &&
		 mvx_is_bitstream(session->port[MVX_DIR_OUTPUT].format))
		return MVX_DIR_OUTPUT;

	return MVX_DIR_MAX;
}

static bool is_stream_on(struct mvx_session *session)
{
	if (session->is_encoder)
		return session->port[MVX_DIR_INPUT].stream_on &&
		    session->port[MVX_DIR_OUTPUT].stream_on;
	else
		return session->port[MVX_DIR_INPUT].stream_on;
}

/**
 * wait_pending() - Wait for procedure to finish.
 *
 * Wait for the number of pending firmware messages to reach 0, or for an error
 * to happen.
 *
 * Return: 0 on success, else error code.
 */
static int wait_pending(struct mvx_session *session)
{
	int ret = 0;

	while (is_fw_loaded(session) != false &&
	       session->fw.msg_pending > 0 && session->error == 0) {
		mutex_unlock(session->isession.mutex);

		ret = wait_event_timeout(session->waitq,
					 is_fw_loaded(session) == false ||
					 session->fw.msg_pending == 0 ||
					 session->error != 0,
					 msecs_to_jiffies
					 (wait_pending_timeout));

		if (ret == 0) {
			MVX_SESSION_WARN(session,
					 "Wait pending returned timeout, msg_pending=%d. try again.",
					 session->fw.msg_pending);
			session->client_ops->soft_irq(session->csession);
			ret = wait_event_timeout(session->waitq,
						 is_fw_loaded(session) == false
						 || session->fw.msg_pending == 0
						 || session->error != 0,
						 msecs_to_jiffies
						 (wait_pending_timeout));

			if (ret == 0) {
				send_event_error(session, -ETIME);
				ret = -ETIME;
				goto lock_mutex;
			}
		}

		if (ret < 0)
			goto lock_mutex;

		mutex_lock(session->isession.mutex);
	}

	return session->error;

lock_mutex:
	mutex_lock(session->isession.mutex);

	if (ret < 0)
		MVX_SESSION_WARN(session,
				 "Wait pending returned error. ret=%d, error=%d, msg_pending=%d.",
				 ret, session->error, session->fw.msg_pending);

	return ret;
}

static int send_irq(struct mvx_session *session)
{
	if (IS_ERR_OR_NULL(session->csession))
		return -EINVAL;

	return session->client_ops->send_irq(session->csession);
}

/**
 * switch_in() - Request the client device to switch in the session.
 *
 * Return: 0 on success, else error code.
 */
static int switch_in(struct mvx_session *session)
{
	int ret;

	session->idle_count = 0;

	if (session->switched_in != false)
		return 0;

	MVX_SESSION_INFO(session, "Switch in.");
	watchdog_start(session, session_watchdog_timeout, true);

	ret = session->client_ops->switch_in(session->csession);
	if (ret != 0) {
		MVX_SESSION_WARN(session, "Failed to switch in session.");
		send_event_error(session, ret);
		return ret;
	}

	session->switched_in = true;

	return 0;
}

static int switch_out_rsp(struct mvx_session *session)
{
	session->switched_in = false;
	session->client_ops->switch_out_rsp(session->csession);

	return 0;
}

/**
 * fw_send_msg() - Send firmware message and signal IRQ.
 *
 * Return: 0 on success, else error code.
 */
static int fw_send_msg(struct mvx_session *session, struct mvx_fw_msg *msg)
{
	int ret;

	if (session->error != 0)
		return session->error;

	ret = session->fw.ops.put_message(&session->fw, msg);
	if (ret != 0) {
		MVX_SESSION_WARN(session, "Failed to queue firmware message.");
		goto send_error;
	}

	ret = send_irq(session);
	if (ret != 0) {
		MVX_SESSION_WARN(session, "Failed to send irq.");
		goto send_error;
	}

	return switch_in(session);

send_error:
	send_event_error(session, ret);
	return ret;
}

static int fw_send_msg_simple(struct mvx_session *session,
			      enum mvx_fw_code code, const char *str)
{
	struct mvx_fw_msg msg = {.code = code };

	MVX_SESSION_INFO(session, "Firmware req: %s.", str);

	return fw_send_msg(session, &msg);
}

static int fw_flush(struct mvx_session *session, enum mvx_direction dir)
{
	struct mvx_fw_msg msg = {.code = MVX_FW_CODE_FLUSH, .flush.dir = dir };
	int ret = 0;

	MVX_SESSION_INFO(session, "Firmware req: Flush. dir=%d.", dir);

	ret = fw_send_msg(session, &msg);
	if (ret != 0)
		return ret;

	session->port[dir].is_flushing = true;

	return 0;
}

static int fw_state_change(struct mvx_session *session, enum mvx_fw_state state)
{
	struct mvx_fw_msg msg = {
		.code = MVX_FW_CODE_STATE_CHANGE,
		.state = state
	};
	int ret = 0;

	if (state != session->fw_state) {
		MVX_SESSION_INFO(session,
				 "Firmware req: State change. current=%d, new=%d.",
				 session->fw_state, state);
		ret = fw_send_msg(session, &msg);
	}

	return ret;
}

static int fw_job(struct mvx_session *session, unsigned int frames)
{
	struct mvx_fw_msg msg = {
		.code = MVX_FW_CODE_JOB,
		.job.cores = session->isession.ncores,
		.job.frames = frames
	};

	MVX_SESSION_INFO(session, "Firmware req: Job. frames=%u.", frames);

	return fw_send_msg(session, &msg);
}

static int fw_switch_out(struct mvx_session *session)
{
	unsigned int idle_count = session->idle_count;
	int ret;

	ret = fw_send_msg_simple(session, MVX_FW_CODE_SWITCH_OUT, "Switch out");

	/*
	 * Restore idle count. Switch out is the only message where we do not
	 * want to reset the idle counter.
	 */
	session->idle_count = idle_count;

	return ret;
}

static int fw_ping(struct mvx_session *session)
{
	return fw_send_msg_simple(session, MVX_FW_CODE_PING, "Ping");
}

static int fw_dump(struct mvx_session *session)
{
	return fw_send_msg_simple(session, MVX_FW_CODE_DUMP, "Dump");
}

static int fw_set_debug(struct mvx_session *session, uint32_t debug_level)
{
	struct mvx_fw_msg msg = {
		.code = MVX_FW_CODE_DEBUG,
		.arg = debug_level
	};

	MVX_SESSION_INFO(session, "Firmware req: Set debug. debug_level=%d.",
			 debug_level);

	return fw_send_msg(session, &msg);
}

static int fw_set_option(struct mvx_session *session,
			 struct mvx_fw_set_option *option)
{
	struct mvx_fw_msg msg = {
		.code = MVX_FW_CODE_SET_OPTION,
		.set_option = *option
	};

	MVX_SESSION_INFO(session, "Firmware req: Set option. code=%d.",
			 option->code);

	return fw_send_msg(session, &msg);
}

static int fw_eos(struct mvx_session *session)
{
	struct mvx_fw_msg msg = {
		.code = MVX_FW_CODE_EOS,
		.eos_is_frame = session->is_encoder
	};
	int ret = 0;

	MVX_SESSION_INFO(session, "Firmware req: Buffer EOS.");

	ret = fw_send_msg(session, &msg);
	if (ret != 0)
		return ret;

	session->port[MVX_DIR_INPUT].flushed = false;

	return 0;
}

static int fw_set_epr_qp(struct mvx_session *session,
			 int code, struct mvx_buffer_param_qp qp)
{
	struct mvx_fw_set_option option;
	int ret;

	if (qp.qp < 0)
		return -EINVAL;

	if (qp.qp == 0)
		return 0;

	option.code = code;
	option.epr_qp = qp;
	ret = fw_set_option(session, &option);
	if (ret != 0) {
		MVX_SESSION_WARN(session,
				 "Failed to set EPR QP. code=%d, ret=%d.",
				 code, ret);
		return ret;
	}

	return 0;
}

static int fw_set_qp(struct mvx_session *session, int code, int qp)
{
	struct mvx_fw_set_option option;
	int ret;

	if (qp < 0)
		return -EINVAL;

	if (qp == 0)
		return 0;

	option.code = code;
	option.qp = qp;
	ret = fw_set_option(session, &option);
	if (ret != 0) {
		MVX_SESSION_WARN(session,
				 "Failed to set QP. code=%d, ret=%d.",
				 code, ret);
		return ret;
	}

	return 0;
}

static int fw_set_osd_config(struct mvx_session *session,
			     int code, struct mvx_osd_config *osd)
{
	struct mvx_fw_set_option option;
	int ret;

	option.code = code;
	option.osd_config = *osd;
	ret = fw_set_option(session, &option);
	if (ret != 0) {
		MVX_SESSION_WARN(session,
				 "Failed to set OSD config. code=%d, ret=%d.",
				 code, ret);
		return ret;
	}

	return 0;
}

static int fw_set_roi_regions(struct mvx_session *session,
			      int code, struct mvx_roi_config *roi)
{
	struct mvx_fw_set_option option;
	int ret;

	if (roi->num_roi < 0)
		return -EINVAL;

	if (roi->num_roi == 0)
		return 0;

	option.code = code;
	option.roi_config = *roi;
	ret = fw_set_option(session, &option);
	if (ret != 0) {
		MVX_SESSION_WARN(session,
				 "Failed to set ROI. code=%d, ret=%d.",
				 code, ret);
		return ret;
	}

	return 0;
}

static int fw_set_chr_cfg(struct mvx_session *session,
			  int code, struct mvx_chr_cfg *chr)
{
	struct mvx_fw_set_option option;
	int ret;

	if (chr->num_chr < 0)
		return -EINVAL;

	if (chr->num_chr == 0)
		return 0;

	option.code = code;
	option.chr_cfg = *chr;
	ret = fw_set_option(session, &option);
	if (ret != 0) {
		MVX_SESSION_WARN(session,
				 "Failed to set CHR CFG. code=%d, ret=%d.",
				 code, ret);
		return ret;
	}

	return 0;
}

static int fw_set_enc_stats(struct mvx_session *session,
			    int code, struct mvx_enc_stats *stats)
{
	struct mvx_fw_set_option option;
	int ret;

	if (stats->flags == 0)
		return 0;

	option.code = code;
	option.enc_stats = *stats;
	ret = fw_set_option(session, &option);
	if (ret != 0) {
		MVX_SESSION_WARN(session,
				 "Failed to set enc stats param. code=%d, ret=%d.",
				 code, ret);
		return ret;
	}

	return 0;
}

static int fw_common_setup(struct mvx_session *session)
{
	int ret = 0;
	struct mvx_fw_set_option option;

	if (session->nalu_format != MVX_NALU_FORMAT_UNDEFINED &&
	    session->port[MVX_DIR_INPUT].format != MVX_FORMAT_AV1) {
		option.code = MVX_FW_SET_NALU_FORMAT;
		option.nalu_format = session->nalu_format;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session, "Failed to set NALU format.");
			return ret;
		}
	}

	if (session->stream_escaping != MVX_TRI_UNSET) {
		option.code = MVX_FW_SET_STREAM_ESCAPING;
		option.stream_escaping = session->stream_escaping;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set stream escaping.");
			return ret;
		}
	}

	if (mvx_log_perf.enabled & MVX_LOG_PERF_UTILIZATION) {
		option.code = MVX_FW_SET_PROFILING;
		option.profiling = 1;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to enable FW profiling.");
			return ret;
		}
	}
	return ret;
}

/* JPEG standard, Annex K */
static const uint8_t qtbl_chroma_ref[MVX_FW_QUANT_LEN] = {
	17, 18, 24, 47, 99, 99, 99, 99,
	18, 21, 26, 66, 99, 99, 99, 99,
	24, 26, 56, 99, 99, 99, 99, 99,
	47, 66, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99
};

static const uint8_t qtbl_luma_ref[MVX_FW_QUANT_LEN] = {
	16, 11, 10, 16, 24, 40, 51, 61,
	12, 12, 14, 19, 26, 58, 60, 55,
	14, 13, 16, 24, 40, 57, 69, 56,
	14, 17, 22, 29, 51, 87, 80, 62,
	18, 22, 37, 56, 68, 109, 103, 77,
	24, 35, 55, 64, 81, 104, 113, 92,
	49, 64, 78, 87, 103, 121, 120, 101,
	72, 92, 95, 98, 112, 100, 103, 99
};

void generate_quant_tbl(int quality,
			const uint8_t qtbl_ref[MVX_FW_QUANT_LEN],
			uint8_t qtbl[MVX_FW_QUANT_LEN])
{
	int i;
	int q;
	int t;

	q = (quality < 50) ? (5000 / quality) : (200 - 2 * quality);

	for (i = 0; i < MVX_FW_QUANT_LEN; ++i) {
		t = ((qtbl_ref[i] * q) + 50) / 100;
		t = min_t(int, t, 255);
		t = max_t(int, t, 1);
		qtbl[i] = t;
	}
}

static int generate_standards_yuv2rgb_coef(enum mvx_yuv_to_rgb_mode mode, struct mvx_color_conv_coef
					   *color_conv_coef)
{
	static struct mvx_color_conv_coef color_standards[] = {
		{ { { 4769, 4769, 4769}, { 0, -1605, 8263}, { 6537, -3330, 0}},
		 { 16, 128, 128} },
		{ { { 4096, 4096, 4096}, { 0, -1410, 7258}, { 5743, -2925, 0}},
		 { 0, 128, 128} },
		{ { { 4769, 4769, 4769}, { 0, -873, 8652}, { 7343, -2183, 0}},
		 { 16, 128, 128} },
		{ { { 4096, 4096, 4096}, { 0, -767, 7601}, { 6450, -1917, 0}},
		 { 0, 128, 128} },
		{ { { 4769, 4769, 4769}, { 0, -767, 8773}, { 6876, -2664, 0}},
		 { 16, 128, 128} },
		{ { { 4096, 4096, 4096}, { 0, -674, 7706}, { 6040, -2340, 0}},
		 { 0, 128, 128} },
	};

	if (mode < MVX_YUV_TO_RGB_MODE_BT601_LIMT
	    || mode >= MVX_YUV_TO_RGB_MODE_MAX) {
		mode = MVX_YUV_TO_RGB_MODE_BT601_LIMT;
		//return -EINVAL;
	}

	memcpy(color_conv_coef, &color_standards[mode],
	       sizeof(struct mvx_color_conv_coef));

	return 0;
}

static int generate_standards_rgb2yuv_coef(enum mvx_rgb_to_yuv_mode mode, struct mvx_rgb2yuv_color_conv_coef
					   *color_conv_coef)
{
	static struct mvx_rgb2yuv_color_conv_coef color_standards[] = {
		{ { 1052, 2065, 401, -607, -1192, 1799, 1799, -1506, -293},
		 { 16, 235}, { 16, 240}, { 0, 255} },
		{ { 1225, 2404, 467, -691, -1357, 2048, 2048, -1715, -333},
		 { 0, 255}, { 0, 255}, { 0, 255} },
		{ { 748, 2516, 254, -412, -1387, 1799, 1799, -1634, -165},
		 { 16, 235}, { 16, 240}, { 0, 255} },
		{ { 871, 2929, 296, -469, -1579, 2048, 2048, -1860, -188},
		 { 0, 255}, { 0, 255}, { 0, 255} },
		{ { 924, 2385, 209, -502, -1297, 1799, 1799, -1654, -145},
		 { 16, 235}, { 16, 240}, { 0, 255} },
		{ { 1076, 2777, 243, -572, -1476, 2048, 2048, -1883, -165},
		 { 0, 255}, { 0, 255}, { 0, 255} },
	};

	if (mode < MVX_RGB_TO_YUV_MODE_BT601_STUDIO
	    || mode >= MVX_RGB_TO_YUV_MODE_MAX) {
		mode = MVX_RGB_TO_YUV_MODE_BT601_STUDIO;
	}

	memcpy(color_conv_coef, &color_standards[mode],
	       sizeof(struct mvx_rgb2yuv_color_conv_coef));

	return 0;
}

static int fw_encoder_setup(struct mvx_session *session)
{
	int ret;
	enum mvx_format codec;
	struct mvx_fw_set_option option;
	enum mvx_direction dir;

	dir = get_bitstream_port(session);
	codec = session->port[dir].format;

	if (session->profile[codec] != MVX_PROFILE_NONE) {
		option.code = MVX_FW_SET_PROFILE_LEVEL;
		option.profile_level.profile = session->profile[codec];
		option.profile_level.level = session->level[codec];
		option.profile_level.tier = session->tier[codec];
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set profile/level.");
			return ret;
		}
	}

	if (session->rc_enabled && session->rc_type) {
		option.code = MVX_FW_SET_RATE_CONTROL;
		option.rate_control.target_bitrate =
		    session->rc_type ? session->target_bitrate : 0;
		option.rate_control.rate_control_mode = session->rc_type;
		if (session->rc_type == MVX_OPT_RATE_CONTROL_MODE_C_VARIABLE) {
			option.rate_control.maximum_bitrate =
			    session->maximum_bitrate;
		}
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to put target bitrate.");
			return ret;
		}
	}

	if (codec != MVX_FORMAT_JPEG) {
		option.code = MVX_FW_SET_FRAME_RATE;
		option.frame_rate = FRAC_TO_Q16(session->fps_n, session->fps_d);
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session, "Failed to put frame rate.");
			return ret;
		}

		if (session->rc_bit_i_mode != 0) {
			option.code = MVX_FW_SET_RC_BIT_I_MODE;
			option.rc_bit_i_mode = session->rc_bit_i_mode;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to put rc bit i mode.");
				return ret;
			}
		}
		if (session->rc_bit_i_ratio != 0) {
			option.code = MVX_FW_SET_RC_BIT_I_RATIO;
			option.rc_bit_i_ratio = session->rc_bit_i_ratio;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to put rc bit i ratio.");
				return ret;
			}
		}

		if (session->mulit_sps_pps != 0) {
			option.code = MVX_FW_SET_MULIT_SPS_PPS;
			option.mulit_sps_pps = session->mulit_sps_pps;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to support multi SPS PSS.");
				return ret;
			}
		}

		if (session->scd_enable != 0) {
			option.code = MVX_FW_SET_SCD_ENABLE;
			option.scd_enable = session->scd_enable;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to enable SCD.");
				return ret;
			}
		}

		if (session->scd_enable != 0 && session->scd_percent >= 0
		    && session->scd_percent <= 10) {
			option.code = MVX_FW_SET_SCD_PERCENT;
			option.scd_percent = session->scd_percent;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set SCD percent.");
				return ret;
			}
		}

		if (session->scd_enable != 0 && session->scd_threshold >= 0
		    && session->scd_threshold <= 2047) {
			option.code = MVX_FW_SET_SCD_THRESHOLD;
			option.scd_threshold = session->scd_threshold;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set SCD threshold.");
				return ret;
			}
		}

		if (session->aq_ssim_en != 0 &&
		    (codec == MVX_FORMAT_H264 || codec == MVX_FORMAT_HEVC)) {
			option.code = MVX_FW_SET_AQ_SSIM_EN;
			option.aq_ssim_en = session->aq_ssim_en;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to enable SSIM.");
				return ret;
			}
		}

		if (session->aq_ssim_en != 0 && session->aq_neg_ratio >= 0 &&
		    session->aq_neg_ratio <= 63 &&
		    (codec == MVX_FORMAT_H264 || codec == MVX_FORMAT_HEVC)) {
			option.code = MVX_FW_SET_AQ_NEG_RATIO;
			option.aq_neg_ratio = session->aq_neg_ratio;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set AQ negative ratio.");
				return ret;
			}
		}

		if (session->aq_ssim_en != 0 && session->aq_pos_ratio >= 0 &&
		    session->aq_pos_ratio <= 63 &&
		    (codec == MVX_FORMAT_H264 || codec == MVX_FORMAT_HEVC)) {
			option.code = MVX_FW_SET_AQ_POS_RATIO;
			option.aq_pos_ratio = session->aq_pos_ratio;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set AQ positive ratio.");
				return ret;
			}
		}

		if (session->aq_ssim_en != 0 && session->aq_qpdelta_lmt >= 0 &&
		    session->aq_qpdelta_lmt <= 7 &&
		    (codec == MVX_FORMAT_H264 || codec == MVX_FORMAT_HEVC)) {
			option.code = MVX_FW_SET_AQ_QPDELTA_LMT;
			option.aq_qpdelta_lmt = session->aq_qpdelta_lmt;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set AQ QPDelta LMT.");
				return ret;
			}
		}

		if (session->aq_ssim_en != 0
		    && session->aq_init_frm_avg_svar >= 0
		    && session->aq_init_frm_avg_svar <= 15
		    && (codec == MVX_FORMAT_H264 || codec == MVX_FORMAT_HEVC)) {
			option.code = MVX_FW_SET_AQ_INIT_FRM_AVG_SVAR;
			option.aq_init_frm_avg_svar =
			    session->aq_init_frm_avg_svar;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to initial frame variance.");
				return ret;
			}
		}

		if (session->enable_visual != 0) {
			option.code = MVX_FW_SET_VISUAL_ENABLE;
			option.enable_visual = session->enable_visual;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to enable visual.");
				return ret;
			}
			option.code = MVX_FW_SET_ADPTIVE_QUANTISATION;
			option.adapt_qnt = 3;	//set to 3 if enable visual
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set adaptive quantisation.");
				return ret;
			}
		}

		if (session->adaptive_intra_block != 0) {
			option.code =
			    MVX_FW_SET_VISUAL_ENABLE_ADAPTIVE_INTRA_BLOCK;
			option.adaptive_intra_block =
			    session->adaptive_intra_block;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to enable adaptive intra block.");
				return ret;
			}
		}

		if (session->rc_enabled != false) {
			if (session->qp[codec].min <= session->qp[codec].max) {
				option.code = MVX_FW_SET_QP_RANGE;
				option.qp_range.min = session->qp[codec].min;
				option.qp_range.max = session->qp[codec].max;
				ret = fw_set_option(session, &option);
				if (ret != 0) {
					MVX_SESSION_WARN(session,
							 "Failed to set qp range.");
					return ret;
				}
			}
		}
		if (session->fixedqp != 0) {
			ret = fw_set_qp(session, MVX_FW_SET_FIXED_QP,
					session->fixedqp);
			if (ret != 0)
				return ret;
		} else {
			if (session->qp[codec].i_frame != 0) {
				ret = fw_set_qp(session, MVX_FW_SET_QP_I,
						session->qp[codec].i_frame);
				if (ret != 0)
					return ret;
			}
			if (session->qp[codec].p_frame != 0) {
				ret = fw_set_qp(session, MVX_FW_SET_QP_P,
						session->qp[codec].p_frame);
				if (ret != 0)
					return ret;
			}
			if (session->qp[codec].b_frame != 0) {
				ret = fw_set_qp(session, MVX_FW_SET_QP_B,
						session->qp[codec].b_frame);
				if (ret != 0)
					return ret;
			}
		}

		if ((session->min_qp_i <= session->max_qp_i)
		    && (session->max_qp_i != 0)) {
			option.code = MVX_FW_SET_QP_RANGE_I;
			option.qp_range.min = session->min_qp_i;
			option.qp_range.max = session->max_qp_i;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set qp range.");
				return ret;
			}
		}

		if (session->b_frames != 0) {
			if (session->port[MVX_DIR_OUTPUT].format ==
			    MVX_FORMAT_VP8
			    || (session->port[MVX_DIR_OUTPUT].format ==
				MVX_FORMAT_H264
				&& session->profile[MVX_FORMAT_H264] ==
				MVX_PROFILE_H264_BASELINE)) {
				MVX_SESSION_WARN(session,
						 "The target format or profile does not support set B frames");

				// Rest B-frames and update P-frames
				session->b_frames = 0;
				session->port[MVX_DIR_INPUT].buffer_min = 1;
				session->p_frames = session->gop_size - 1;
			} else {
				option.code = MVX_FW_SET_B_FRAMES;
				option.pb_frames = session->b_frames;
				ret = fw_set_option(session, &option);
				if (ret != 0) {
					MVX_SESSION_WARN(session,
							 "Failed to set B frames.");
					return ret;
				}
			}
		}

		option.code = MVX_FW_SET_P_FRAMES;
		option.pb_frames = session->p_frames;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session, "Failed to set P frames.");
			return ret;
		}

		if (session->gop_type != MVX_GOP_TYPE_NONE) {
			option.code = MVX_FW_SET_GOP_TYPE;
			option.gop_type = session->gop_type;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set GOP type.");
				return ret;
			}
		}

		if (session->inter_med_buf_size != 0) {
			option.code = MVX_FW_SET_INTER_MED_BUF_SIZE;
			option.inter_med_buf_size = session->inter_med_buf_size;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set inter mediate buffer size.");
				return ret;
			}
		}

		if (session->svct3_level1_period != 0) {
			option.code = MVX_FW_SET_SVCT3_LEVEL1_PERIOD;
			option.svct3_level1_period =
			    session->svct3_level1_period;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set svct3_level1_period.");
				return ret;
			}
		}

		if (session->reset_ltr_period != 0) {
			option.code = MVX_FW_SET_LTR_PERIOD;
			option.reset_ltr_period = session->reset_ltr_period;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set ltr period.");
				return ret;
			}
		}

		if (session->reset_gop_pframes != 0) {
			option.code = MVX_FW_SET_GOP_PFRAMES;
			option.reset_gop_pframes = session->reset_gop_pframes;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set gop pframes.");
				return ret;
			}
		}

		if (session->cyclic_intra_refresh_mb != 0) {
			option.code = MVX_FW_SET_INTRA_MB_REFRESH;
			option.intra_mb_refresh =
			    session->cyclic_intra_refresh_mb;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set cyclic intra refresh Mb.");
				return ret;
			}
		}

		if (session->constr_ipred != MVX_TRI_UNSET &&
		    (codec == MVX_FORMAT_H264 || codec == MVX_FORMAT_HEVC)) {
			option.code = MVX_FW_SET_CONSTR_IPRED;
			option.constr_ipred = session->constr_ipred;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set constr ipred.");
				return ret;
			}
		}
	} else {
		/*
		 * //handle JPEG rc
		 * if (session->rc_type) {
		 * option.code = MVX_FW_SET_RATE_CONTROL_JPEG;
		 *
		 *  option.jpeg_rc.fps = FRAC_TO_Q16(session->fps_n, session->fps_d);
		 *  option.jpeg_rc.qscale = session->jpeg_quality;
		 *  option.jpeg_rc.qscale_chroma = session->jpeg_quality_chroma;
		 *  option.jpeg_rc.qscale_luma = session->jpeg_quality_luma;
		 *
		 *  ret = fw_set_option(session, &option);
		 *  if (ret != 0) {
		 *  MVX_SESSION_WARN(session,
		 *  "Failed to put target bitrate.");
		 *  return ret;
		 *  }
		 *  }
		 */
	}

	if (codec == MVX_FORMAT_HEVC) {
		if (session->entropy_sync != MVX_TRI_UNSET) {
			option.code = MVX_FW_SET_ENTROPY_SYNC;
			option.entropy_sync = session->entropy_sync;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set entropy sync.");
				return ret;
			}
		}

		if (session->temporal_mvp != MVX_TRI_UNSET) {
			option.code = MVX_FW_SET_TEMPORAL_MVP;
			option.temporal_mvp = session->temporal_mvp;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set temporal mvp.");
				return ret;
			}
		}
		if (session->min_luma_cb_size != 0) {
			option.code = MVX_FW_SET_MIN_LUMA_CB_SIZE;
			option.min_luma_cb_size = session->min_luma_cb_size;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set min luma cb size.");
				return ret;
			}
		}
	}

	if (codec == MVX_FORMAT_HEVC || codec == MVX_FORMAT_VP9) {
		option.code = MVX_FW_SET_TILES;
		option.tile.rows =
		    session->tile_rows > 0 ? session->tile_rows : 1;
		option.tile.cols =
		    session->tile_cols > 0 ? session->tile_cols : 1;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session, "Failed to set tile dims.");
			return ret;
		}
	}

	if (session->entropy_mode != MVX_ENTROPY_MODE_NONE &&
	    codec == MVX_FORMAT_H264) {
		option.code = MVX_FW_SET_ENTROPY_MODE;
		option.entropy_mode = session->entropy_mode;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set entropy mode.");
			return ret;
		}
	}

	if (session->intra_ipenalty_angular != MVX_INVALID_VAL) {
		option.code = MVX_FW_SET_ENC_INTRA_IPENALTY_ANGULAR;
		option.intra_ipenalty_angular = session->intra_ipenalty_angular;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set intra ipenalty angular");
			return ret;
		}
	}

	if (session->intra_ipenalty_planar != MVX_INVALID_VAL) {
		option.code = MVX_FW_SET_ENC_INTRA_IPENALTY_PLANAR;
		option.intra_ipenalty_planar = session->intra_ipenalty_planar;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set intra ipenalty planar");
			return ret;
		}
	}

	if (session->intra_ipenalty_dc != MVX_INVALID_VAL) {
		option.code = MVX_FW_SET_ENC_INTRA_IPENALTY_DC;
		option.intra_ipenalty_dc = session->intra_ipenalty_dc;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set intra ipenalty dc");
			return ret;
		}
	}

	if (session->inter_ipenalty_angular != MVX_INVALID_VAL) {
		option.code = MVX_FW_SET_ENC_INTER_IPENALTY_ANGULAR;
		option.inter_ipenalty_angular = session->inter_ipenalty_angular;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set inter ipenalty angular");
			return ret;
		}
	}

	if (session->inter_ipenalty_planar != MVX_INVALID_VAL) {
		option.code = MVX_FW_SET_ENC_INTER_IPENALTY_PLANAR;
		option.inter_ipenalty_planar = session->inter_ipenalty_planar;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set inter ipenalty planar");
			return ret;
		}
	}

	if (session->inter_ipenalty_dc != MVX_INVALID_VAL) {
		option.code = MVX_FW_SET_ENC_INTER_IPENALTY_DC;
		option.inter_ipenalty_dc = session->inter_ipenalty_dc;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set inter ipenalty dc");
			return ret;
		}
	}

	if (codec == MVX_FORMAT_H264 || codec == MVX_FORMAT_HEVC) {
		option.code = MVX_FW_SET_SLICE_SPACING_MB;
		if (session->multi_slice_mode == MVX_MULTI_SLICE_MODE_SINGLE)
			option.slice_spacing_mb = 0;
		else
			option.slice_spacing_mb = session->multi_slice_max_mb;

		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set slice spacing.");
			return ret;
		}

		option.code = MVX_FW_SET_CABAC_INIT_IDC;
		option.cabac_init_idc = session->cabac_init_idc;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set CABAC init IDC.");
			return ret;
		}
		if (session->crop_left != 0) {
			option.code = MVX_FW_SET_CROP_LEFT;
			option.crop_left = session->crop_left;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set crop left");
				return ret;
			}
		}
		if (session->crop_right != 0) {
			option.code = MVX_FW_SET_CROP_RIGHT;
			option.crop_right = session->crop_right;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set crop right");
				return ret;
			}
		}
		if (session->crop_top != 0) {
			option.code = MVX_FW_SET_CROP_TOP;
			option.crop_top = session->crop_top;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set crop top");
				return ret;
			}
		}
		if (session->crop_bottom != 0) {
			option.code = MVX_FW_SET_CROP_BOTTOM;
			option.crop_bottom = session->crop_bottom;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set crop bottom");
				return ret;
			}
		}

		if (session->color_desc.range != 0
		    || session->color_desc.matrix_coeff != 2
		    || session->color_desc.colour_primaries != 2
		    || session->color_desc.transfer_characteristics != 2
		    || session->color_desc.sar_height != 0
		    || session->color_desc.sar_width != 0
		    || session->color_desc.aspect_ratio_idc != 0
		    || session->color_desc.flags != 0) {
			struct mvx_fw_set_option option;

			option.code = MVX_FW_SET_COLOUR_DESC;
			option.colour_desc = session->color_desc;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set vui colour description");
				return ret;
			}
		}

		if (session->sei_userdata.flags) {
			option.code = MVX_FW_SET_SEI_USERDATA;
			option.userdata = session->sei_userdata;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set sei userdata");
				return ret;
			}
		}

		if (session->mvx_ltr.mode != 0 || session->mvx_ltr.period != 0) {
			option.code = MVX_FW_SET_LONG_TERM_REF;
			option.ltr.mode = session->mvx_ltr.mode;
			option.ltr.period = session->mvx_ltr.period;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set ltr mode/period");
				return ret;
			}
		}

		if (session->gdr_number > 1 && session->gdr_period > 1) {
			option.code = MVX_FW_SET_GDR_NUMBER;
			option.gdr_number = session->gdr_number;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set gdr number");
				return ret;
			}
			option.code = MVX_FW_SET_GDR_PERIOD;
			option.gdr_period = session->gdr_period;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set gdr period");
				return ret;
			}
		}

	}

	if (session->nHRDBufsize != 0) {
		option.code = MVX_FW_SET_HRD_BUF_SIZE;
		option.nHRDBufsize = session->nHRDBufsize;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set HRD Buffer Size");
			return ret;
		}
	}

	if (codec == MVX_FORMAT_VP9) {
		option.code = MVX_FW_SET_VP9_PROB_UPDATE;
		option.vp9_prob_update = session->vp9_prob_update;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set VP9 prob update mode.");
			return ret;
		}
	}

	if (session->mv_h_search_range != 0 && session->mv_v_search_range != 0) {
		option.code = MVX_FW_SET_MV_SEARCH_RANGE;
		option.mv.x = session->mv_h_search_range;
		option.mv.y = session->mv_v_search_range;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set motion vector search range.");
			return ret;
		}
	}

	if (session->bitdepth_chroma != 0 && session->bitdepth_luma != 0) {
		option.code = MVX_FW_SET_BITDEPTH;
		option.bitdepth.chroma = session->bitdepth_chroma;
		option.bitdepth.luma = session->bitdepth_luma;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session, "Failed to set bitdepth.");
			return ret;
		}
	}

	if (session->force_chroma_format != 0) {
		option.code = MVX_FW_SET_CHROMA_FORMAT;
		option.chroma_format = session->force_chroma_format;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set chroma format.");
			return ret;
		}
	}

	if (session->use_cust_rgb_to_yuv_mode == MVX_CUST_YUV2RGB_MODE_STANDARD) {
		option.code = MVX_FW_SET_RGB_TO_YUV_MODE;
		generate_standards_rgb2yuv_coef(session->rgb_to_yuv,
						&option.rgb2yuv_params);
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set rgb2yuv color conversion mode.");
			return ret;
		}
	} else if (session->use_cust_rgb_to_yuv_mode ==
		   MVX_CUST_YUV2RGB_MODE_CUSTOMIZED) {
		option.code = MVX_FW_SET_RGB_TO_YUV_MODE;
		memcpy(&option.rgb2yuv_params,
		       &session->rgb2yuv_color_conv_coef,
		       sizeof(struct mvx_rgb2yuv_color_conv_coef));
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set rgb2yuv color conversion mode.");
			return ret;
		}
	}

	if (session->band_limit != 0) {
		option.code = MVX_FW_SET_BAND_LIMIT;
		option.band_limit = session->band_limit;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set bandwidth limit.");
			return ret;
		}
	}

	if (session->init_qpi != 0) {
		option.code = MVX_FW_SET_INIT_QP_I;
		option.init_qpi = session->init_qpi;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set init qp for I frame.");
			return ret;
		}
	}
	if (session->init_qpp != 0) {
		option.code = MVX_FW_SET_INIT_QP_P;
		option.init_qpp = session->init_qpp;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set init qp for P frame.");
			return ret;
		}
	}
	if (session->sao_luma != 0) {
		option.code = MVX_FW_SET_SAO_LUMA;
		option.sao_luma = session->sao_luma;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session, "Failed to set sao luma.");
			return ret;
		}
	}
	if (session->sao_chroma != 0) {
		option.code = MVX_FW_SET_SAO_CHROMA;
		option.sao_chroma = session->sao_chroma;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session, "Failed to set sao chroma.");
			return ret;
		}
	}
	if (session->qp_delta_i_p != 0) {
		option.code = MVX_FW_SET_QP_DELTA_I_P;
		option.qp_delta_i_p = session->qp_delta_i_p;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set delta qp for I frame and P frame.");
			return ret;
		}
	}
	if (session->ref_rb_en != 0) {
		option.code = MVX_FW_SET_QP_REF_RB_EN;
		option.ref_rb_en = session->ref_rb_en;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session, "Failed to set ref_rb_en.");
			return ret;
		}
	}
	if (session->rc_qp_clip_top != 0) {
		option.code = MVX_FW_SET_RC_CLIP_TOP;
		option.rc_qp_clip_top = session->rc_qp_clip_top;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set rc_qp_clip_top.");
			return ret;
		}
	}
	if (session->rc_qp_clip_bot != 0) {
		option.code = MVX_FW_SET_RC_CLIP_BOT;
		option.rc_qp_clip_bot = session->rc_qp_clip_bot;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set rc_qp_clip_bot.");
			return ret;
		}
	}
	if (session->qpmap_qp_clip_top != 0) {
		option.code = MVX_FW_SET_QP_MAP_CLIP_TOP;
		option.qpmap_qp_clip_top = session->qpmap_qp_clip_top;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set qpmap_qp_clip_top.");
			return ret;
		}
	}
	if (session->qpmap_qp_clip_top != 0) {
		option.code = MVX_FW_SET_QP_MAP_CLIP_BOT;
		option.qpmap_qp_clip_bot = session->qpmap_qp_clip_bot;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set qpmap_qp_clip_bot.");
			return ret;
		}
	}
	if (session->lambda_scale.lambda_scale_i_q8 != 0 ||
	    session->lambda_scale.lambda_scale_sqrt_i_q8 != 0 ||
	    session->lambda_scale.lambda_scale_p_q8 != 0 ||
	    session->lambda_scale.lambda_scale_sqrt_p_q8 != 0 ||
	    session->lambda_scale.lambda_scale_b_ref_q8 != 0 ||
	    session->lambda_scale.lambda_scale_sqrt_b_ref_q8 != 0 ||
	    session->lambda_scale.lambda_scale_b_nonref_q8 != 0 ||
	    session->lambda_scale.lambda_scale_sqrt_b_nonref_q8 != 0) {
		option.code = MVX_FW_SET_ENC_LAMBDA_SCALE;

		memset(&option.lambda_scale, 0, sizeof(option.lambda_scale));
		option.lambda_scale.lambda_scale_i_q8 = (uint16_t) 0x0100;	// default is 1.0
		option.lambda_scale.lambda_scale_sqrt_i_q8 = (uint16_t) 0x0100;
		option.lambda_scale.lambda_scale_p_q8 = (uint16_t) 0x0100;	// default is 1.0
		option.lambda_scale.lambda_scale_sqrt_p_q8 = (uint16_t) 0x0100;
		option.lambda_scale.lambda_scale_b_ref_q8 = (uint16_t) 0x0100;	// default is 1.0
		option.lambda_scale.lambda_scale_sqrt_b_ref_q8 =
		    (uint16_t) 0x0100;
		option.lambda_scale.lambda_scale_b_nonref_q8 = (uint16_t) 0x0100;	// default is 1.0
		option.lambda_scale.lambda_scale_sqrt_b_nonref_q8 =
		    (uint16_t) 0x0100;

		if (session->lambda_scale.lambda_scale_i_q8 != 0) {
			option.lambda_scale.lambda_scale_i_q8 =
			    session->lambda_scale.lambda_scale_i_q8;
		}
		if (session->lambda_scale.lambda_scale_sqrt_i_q8 != 0) {
			option.lambda_scale.lambda_scale_sqrt_i_q8 =
			    session->lambda_scale.lambda_scale_sqrt_i_q8;
		}
		if (session->lambda_scale.lambda_scale_p_q8 != 0) {
			option.lambda_scale.lambda_scale_p_q8 =
			    session->lambda_scale.lambda_scale_p_q8;
		}
		if (session->lambda_scale.lambda_scale_sqrt_p_q8 != 0) {
			option.lambda_scale.lambda_scale_sqrt_p_q8 =
			    session->lambda_scale.lambda_scale_sqrt_p_q8;
		}
		if (session->lambda_scale.lambda_scale_b_ref_q8 != 0) {
			option.lambda_scale.lambda_scale_b_ref_q8 =
			    session->lambda_scale.lambda_scale_b_ref_q8;
		}
		if (session->lambda_scale.lambda_scale_sqrt_b_ref_q8 != 0) {
			option.lambda_scale.lambda_scale_sqrt_b_ref_q8 =
			    session->lambda_scale.lambda_scale_sqrt_b_ref_q8;
		}
		if (session->lambda_scale.lambda_scale_b_nonref_q8 != 0) {
			option.lambda_scale.lambda_scale_b_nonref_q8 =
			    session->lambda_scale.lambda_scale_b_nonref_q8;
		}
		if (session->lambda_scale.lambda_scale_sqrt_b_nonref_q8 != 0) {
			option.lambda_scale.lambda_scale_sqrt_b_nonref_q8 =
			    session->lambda_scale.lambda_scale_sqrt_b_nonref_q8;
		}
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set lambda scale.");
			return ret;
		}
	}

	if (codec == MVX_FORMAT_JPEG) {
		if (session->resync_interval >= 0) {
			option.code = MVX_FW_SET_RESYNC_INTERVAL;
			option.resync_interval = session->resync_interval;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set resync interval.");
				return ret;
			}
		}

		if (session->jpeg_quality != 0
		    || session->jpeg_quality_luma != 0
		    || session->jpeg_quality_chroma != 0) {
			uint8_t qtbl_chroma[MVX_FW_QUANT_LEN];
			uint8_t qtbl_luma[MVX_FW_QUANT_LEN];
			uint32_t quality_luma =
			    session->jpeg_quality_luma !=
			    0 ? session->jpeg_quality_luma : session->jpeg_quality;
			uint32_t quality_chroma =
			    session->jpeg_quality_chroma !=
			    0 ? session->jpeg_quality_chroma : session->jpeg_quality;
			option.code = MVX_FW_SET_QUANT_TABLE;
			if (quality_luma) {
				generate_quant_tbl(quality_luma,
						   qtbl_luma_ref, qtbl_luma);
				option.quant_tbl.luma = qtbl_luma;
			}
			if (quality_chroma) {
				generate_quant_tbl(quality_chroma,
						   qtbl_chroma_ref,
						   qtbl_chroma);
				option.quant_tbl.chroma = qtbl_chroma;
			}
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set quantization table.");
				return ret;
			}
		}
		if (session->huff_table.type != 0) {
			option.code = MVX_FW_SET_HUFF_TABLE;
			memcpy(&option.huff_table, &session->huff_table,
			       sizeof(struct mvx_huff_table));
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set huff table.");
				return ret;
			}
		}
	}

	if ((session->port[MVX_DIR_INPUT].format == MVX_FORMAT_YUV444
	     || session->port[MVX_DIR_INPUT].format == MVX_FORMAT_YUV444_10
	     || session->port[MVX_DIR_INPUT].format == MVX_FORMAT_YUV420_I420
	     || session->port[MVX_DIR_INPUT].format == MVX_FORMAT_YUV420_I420_10
	     || session->port[MVX_DIR_INPUT].format == MVX_FORMAT_Y
	     || session->port[MVX_DIR_INPUT].format == MVX_FORMAT_Y_10)
	    && session->forced_uv_value >= 0
	    && session->forced_uv_value < 0x400) {
		option.code = MVX_FW_SET_ENC_FORCED_UV_VAL;
		option.forced_uv_value = session->forced_uv_value;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set forced to uv value.");
			return ret;
		}
	}

	if (session->crop.width != 0
	    && session->crop.height != 0 && session->crop.crop_en != 0) {
		if (session->crop.width != session->port[MVX_DIR_INPUT].width
		    || session->crop.height !=
		    session->port[MVX_DIR_INPUT].height) {
			option.code = MVX_FW_SET_ENC_SRC_CROPPING;
			memcpy(&option.crop, &session->crop,
			       sizeof(struct mvx_crop_cfg));
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set enc src crop.");
				return ret;
			}
		}
	}

	if (session->mini_frame_cnt >= 2) {
		option.code = MVX_FW_SET_MINI_FRAME_CNT;
		option.mini_frame_cnt = session->mini_frame_cnt;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set mini frame buffer cnt.");
			return ret;
		}
	}

	ret = fw_common_setup(session);

	return ret;
}

static int fw_decoder_setup(struct mvx_session *session)
{
	int ret;
	struct mvx_fw_set_option option;

	enum mvx_format codec;
	enum mvx_direction dir;

	dir = get_bitstream_port(session);
	codec = session->port[dir].format;

	if (codec == MVX_FORMAT_VC1 &&
	    session->profile[codec] != MVX_PROFILE_NONE) {
		option.code = MVX_FW_SET_PROFILE_LEVEL;
		option.profile_level.profile = session->profile[codec];
		option.profile_level.level = session->level[codec];
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set profile/level.");
			return ret;
		}
	}

	if (codec == MVX_FORMAT_AV1 && session->fsf_mode != MVX_INVALID_VAL) {
		option.code = MVX_FW_SET_FSF_MODE;
		option.fsf_mode = session->fsf_mode;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set av1 fsf mode.");
			return ret;
		}
	}

	if (session->ignore_stream_headers != MVX_TRI_UNSET) {
		option.code = MVX_FW_SET_IGNORE_STREAM_HEADERS;
		option.ignore_stream_headers = session->ignore_stream_headers;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set ignore stream headers.");
			return ret;
		}
	}

	if (session->frame_reordering != MVX_TRI_UNSET) {
		option.code = MVX_FW_SET_FRAME_REORDERING;
		option.frame_reordering = session->frame_reordering;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set frame reordering.");
			return ret;
		}
	}

	if (session->intbuf_size != 0) {
		option.code = MVX_FW_SET_INTBUF_SIZE;
		option.intbuf_size = session->intbuf_size;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set internal buffer size.");
			return ret;
		}
	}

	if (session->dsl_frame.width != 0 && session->dsl_frame.height != 0) {
		option.code = MVX_FW_SET_DSL_FRAME;
		option.dsl_frame.width = session->dsl_frame.width;
		option.dsl_frame.height = session->dsl_frame.height;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set DSL frame width/height.");
			return ret;
		}
	}

	if (session->dsl_pos_mode >= 0 && session->dsl_pos_mode <= 2) {
		option.code = MVX_FW_SET_DSL_MODE;
		option.dsl_pos_mode = session->dsl_pos_mode;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session, "Failed to set DSL mode.");
			return ret;
		}
	}

	if (session->dsl_interp_mode >= 0 && session->dsl_interp_mode <= 1) {
		option.code = MVX_FW_SET_DSL_INTERP_MODE;
		option.dsl_interp_mode = session->dsl_interp_mode;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set DSL INTERP mode.");
			return ret;
		}
	}

	if (mvx_is_rgb24(session->port[MVX_DIR_OUTPUT].format)) {
		option.code = MVX_FW_SET_DEC_YUV2RGB_PARAMS;

		if (session->use_cust_color_conv_coef) {
			ret = 0;
			memcpy(&option.yuv2rbg_csc_coef,
			       &session->color_conv_coef,
			       sizeof(struct mvx_color_conv_coef));
		} else {
			ret =
			    generate_standards_yuv2rgb_coef
			    (session->color_conv_mode,
			     &option.yuv2rbg_csc_coef);
		}
		if (ret == 0)
			ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set yuv2rgb color conversion mode.");
			return ret;
		}
	}
	if (session->disabled_features != 0 || codec == MVX_FORMAT_AV1) {
		option.code = MVX_FW_SET_DISABLE_FEATURES;
		option.disabled_features = session->disabled_features;
		//disable MVE_OPT_DISABLE_FEATURE_AFBC_LEGACY_REF for av1
		option.disabled_features |= codec == MVX_FORMAT_AV1 ? 0x100 : 0;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set disabled features:%x.",
					 option.disabled_features);
			return ret;
		}
	}

	if (session->crop.crop_en != 0
	    && session->crop.width > 0 && session->crop.height > 0) {
		option.code = MVX_FW_SET_DEC_DST_CROPPING;
		memcpy(&option.crop, &session->crop,
		       sizeof(struct mvx_crop_cfg));
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set dec dst crop.");
			return ret;
		}
	}

	ret = fw_common_setup(session);

	return ret;
}

static int fw_initial_setup(struct mvx_session *session)
{
	int ret;
	enum mvx_direction dir;
	enum mvx_format codec;
	struct mvx_fw_set_option option;

	MVX_SESSION_INFO(session, "Firmware initial setup.");

	fw_set_debug(session, 5);

	option.code = MVX_FW_SET_WATCHDOG_TIMEOUT;
	option.watchdog_timeout = fw_watchdog_timeout;
	ret = fw_set_option(session, &option);
	if (ret != 0)
		return ret;

	dir = get_bitstream_port(session);
	codec = session->port[dir].format;

	ret = fw_job(session, session->job_frames);
	if (ret != 0)
		return ret;

	if (session->is_encoder)
		ret = fw_encoder_setup(session);
	else
		ret = fw_decoder_setup(session);

	if (ret != 0) {
		MVX_SESSION_WARN(session, "Failed to perform initial setup.\n");
		return ret;
	}

	ret = fw_state_change(session, MVX_FW_STATE_RUNNING);
	if (ret != 0) {
		MVX_SESSION_WARN(session, "Failed to queue state change.");
		return ret;
	}

	ret = fw_ping(session);
	if (ret != 0) {
		MVX_SESSION_WARN(session, "Failed to put ping message.");
		send_event_error(session, ret);
		return ret;
	}

	return ret;
}

/**
 * map_buffer() - Memory map buffer to MVE address space.
 *
 * Return 0 on success, else error code.
 */
static int map_buffer(struct mvx_session *session,
		      enum mvx_direction dir, struct mvx_buffer *buf)
{
	mvx_mmu_va begin;
	mvx_mmu_va end;
	enum mvx_fw_region region;
	int ret;
	mvx_mmu_va *next_va;

	ret = mutex_lock_interruptible(&session->fw.mem_mutex);
	if (ret != 0) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_ERROR,
			      "Cannot protect buffer VA.");
		return ret;
	}

	if (mvx_is_bitstream(session->port[dir].format)) {
		region = MVX_FW_REGION_PROTECTED;
		next_va = &session->fw.next_va_region_protected;
	} else if (mvx_is_frame(session->port[dir].format)) {
		region = MVX_FW_REGION_FRAMEBUF;
		next_va = &session->fw.next_va_region_outbuf;
	} else
		return -EINVAL;

	ret = session->fw.ops.get_region(region, &begin, &end);
	if (ret != 0) {
		mutex_unlock(&session->fw.mem_mutex);
		return ret;
	}

	ret = mvx_buffer_map(buf, begin, end, next_va, session->port[dir].size);
	if (ret != 0) {
		mutex_unlock(&session->fw.mem_mutex);
		return ret;
	}

	mutex_unlock(&session->fw.mem_mutex);
	return 0;
}

static int queue_osd_config(struct mvx_session *session,
			    struct mvx_osd_config *osd_cfg)
{
	int ret = 0;

	ret = fw_set_osd_config(session, MVX_FW_SET_OSD_CONFIG, osd_cfg);
	return ret;
}

static int queue_roi_regions(struct mvx_session *session,
			     struct mvx_roi_config *roi_cfg)
{
	int ret = 0;

	if (roi_cfg->qp_present)
		ret = fw_set_qp(session, MVX_FW_SET_QP_REGION, roi_cfg->qp);
	if (roi_cfg->roi_present)
		ret = fw_set_roi_regions(session, MVX_FW_SET_ROI_REGIONS,
					 roi_cfg);
	return ret;
}

static int queue_qp_epr(struct mvx_session *session,
			struct mvx_buffer_param_qp *qp)
{
	int ret = 0;

	ret = fw_set_epr_qp(session, MVX_FW_SET_EPR_QP, *qp);

	return ret;
}

static int queue_chr_cfg(struct mvx_session *session,
			 struct mvx_chr_cfg *chr_cfg)
{
	int ret = 0;

	ret = fw_set_chr_cfg(session, MVX_FW_SET_CHR_CFG, chr_cfg);
	return ret;
}

static int queue_enc_stats(struct mvx_session *session,
			   struct mvx_enc_stats *stats)
{
	int ret = 0;

	ret = fw_set_enc_stats(session, MVX_FW_SET_STATS_MODE, stats);
	return ret;
}

static struct mvx_session_format_map *get_format_map_by_mvx_format(enum
								   mvx_format
								   format)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mvx_raw_fmts); i++)
		if (mvx_raw_fmts[i].format == format)
			return &mvx_raw_fmts[i];

	for (i = 0; i < ARRAY_SIZE(mvx_compressed_fmts); i++)
		if (mvx_compressed_fmts[i].format == format)
			return &mvx_compressed_fmts[i];

	return NULL;
}

static void revert_frame_format(struct mvx_session *session,
				enum mvx_direction dir, struct mvx_buffer *buf)
{
	struct mvx_session_port *port = &session->port[dir];
	struct mvx_session_format_map *new_format =
	    get_format_map_by_mvx_format(port->format);
	struct mvx_session_format_map *old_format =
	    get_format_map_by_mvx_format(port->old_format);
	if (new_format != NULL && old_format != NULL
	    && old_format->bpp <= new_format->bpp) {
		unsigned int size[MVX_BUFFER_NPLANES];
		unsigned int stride[MVX_BUFFER_NPLANES];
		int ret;
		int i;

		for (i = 0; i < buf->nplanes; i++)
			stride[i] = buf->planes[i].stride;
		ret =
		    mvx_buffer_frame_dim(port->old_format, port->width,
					 port->height, &port->nplanes, stride,
					 size, session->setting_stride);
		if (ret != 0)
			return;	// just return here as nothing is changed so far

		buf->format = port->old_format;
		port->format = port->old_format;
		port->pixelformat = port->old_pixelformat;
		port->nplanes = buf->nplanes;
		memcpy(port->stride, stride,
		       sizeof(*stride) * MVX_BUFFER_NPLANES);
		memcpy(port->size, size, sizeof(*size) * MVX_BUFFER_NPLANES);
		session->event(session, MVX_SESSION_EVENT_PORT_CHANGED,
			       (void *)MVX_DIR_OUTPUT);
	}
}

/**
 * queue_buffer() - Put buffer to firmware queue.
 *
 * Return: 0 on success, else error code.
 */
static int queue_buffer(struct mvx_session *session,
			enum mvx_direction dir, struct mvx_buffer *buf)
{
	struct mvx_session_port *port = &session->port[dir];
	struct mvx_fw_msg msg;
	struct mvx_seamless_target *seamless = &session->seamless_target;
	unsigned int width;
	unsigned int height;
	unsigned int stride[MVX_BUFFER_NPLANES];
	unsigned int i;

	/*
	 * Vb2 cannot allocate buffers with bidirectional mapping, therefore
	 * proper direction should be set.
	 */
	// enum dma_data_direction dma_dir =
	//     (dir == MVX_DIR_OUTPUT) ? DMA_FROM_DEVICE : DMA_TO_DEVICE;

	int ret;

	if (dir == MVX_DIR_OUTPUT) {
		port->scaling_shift =
		    (buf->flags & MVX_BUFFER_FRAME_FLAG_SCALING_MASK) >> 14;
	}
	if (mvx_buffer_is_mapped(buf) == false) {
		/*
		 * In dual AFBC downscaling case, allocate a buffer for AFBC frame in
		 * original resolution and attach to mvx_buffer as plane 1.
		 */
		if (session->dual_afbc_downscaled && dir == MVX_DIR_OUTPUT &&
		    port->interlaced == 0 && buf->nplanes == 1
		    && port->size[1] > 0) {
			size_t npages;
			struct mvx_mmu_pages *pages;

			npages = DIV_ROUND_UP(port->size[1], MVE_PAGE_SIZE);

			if (session->fw.fw_bin->securevideo != false) {
				struct dma_buf *dmabuf;

				dmabuf =
				    mvx_secure_mem_alloc(session->fw.fw_bin->secure.secure,
							 port->size[1],
							 MVX_FW_REGION_FRAMEBUF);
				if (IS_ERR(dmabuf))
					return -ENOMEM;

				pages =
				    mvx_mmu_alloc_pages_dma_buf(session->dev,
								dmabuf, npages);
				if (IS_ERR(pages)) {
					dma_buf_put(dmabuf);
					return -ENOMEM;
				}
			} else {
				pages =
				    mvx_mmu_alloc_pages(session->dev, npages,
							npages, GFP_KERNEL);
				if (IS_ERR(pages))
					return -ENOMEM;
			}

			buf->planes[1].pages = pages;
			buf->nplanes++;
		}

		ret = map_buffer(session, dir, buf);
		if (ret != 0)
			return ret;
	}
	if (dir == MVX_DIR_OUTPUT && port->buffer_allocated < port->buffer_min) {
		buf->flags |= MVX_BUFFER_FRAME_NEED_REALLOC;
		return -EAGAIN;
	}
	/*
	 * In case of port format and buffer format mismatch, it means
	 * driver updated format after received SEQ_PARAM message from
	 * fw, but client didn't get format and reallocate buffers. Try
	 * to change the port format to buffer foramt if the format is
	 * supportdrivers/soc/cix/vpu/driver/if/mvx_session.c by VPU post-processing.
	 */
	if (mvx_is_frame(port->format) != false && !session->is_encoder &&
	    port->format != port->old_format)
		revert_frame_format(session, dir, buf);
	/*
	 * Update frame dimensions. They might have changed due to a resolution
	 * change.
	 */
	if (dir == MVX_DIR_OUTPUT
	    && session->port[MVX_DIR_INPUT].format <= MVX_FORMAT_BITSTREAM_LAST
	    && seamless->seamless_mode != 0) {
		width =
		    seamless->target_width <
		    port->width ? port->width : seamless->target_width;
		height =
		    seamless->target_height <
		    port->height ? port->height : seamless->target_height;
		for (i = 0; i < MVX_BUFFER_NPLANES; i++) {
			stride[i] =
			    seamless->target_stride[i] <
			    port->stride[i] ? port->stride[i] : seamless->target_stride[i];
		}
	} else {
		width = port->width;
		height = port->height;
		for (i = 0; i < MVX_BUFFER_NPLANES; i++)
			stride[i] = port->stride[i];
	}
	if (mvx_is_afbc(port->format) != false) {
		i = 0;
		if (session->dual_afbc_downscaled && dir == MVX_DIR_OUTPUT
		    && port->interlaced == 0 && buf->nplanes > 1) {
			ret =
			    mvx_buffer_afbc_set(buf, port->format, width,
						height,
						port->afbc_width_in_superblocks_downscaled,
						port->size[i], 0, i);
			if (ret != 0)
				return ret;
			i++;
		}
		ret = mvx_buffer_afbc_set(buf, port->format, width,
					  height, port->afbc_width,
					  port->size[i], port->interlaced, i);
		if (ret != 0)
			return ret;
	} else if (mvx_is_frame(port->format) != false) {
		ret = mvx_buffer_frame_set(buf, port->format, width,
					   height, stride,
					   port->size, port->interlaced);
		if (ret != 0)
			return ret;

	}
	// if (!session->isession.securevideo && dma_dir == DMA_TO_DEVICE) {
	//     ret = mvx_buffer_synch(buf, dma_dir);
	//     if (ret != 0)
	//         return ret;
	// }
	if (session->pending_switch_out) {
		session->pending_switch_out = false;
		fw_switch_out(session);
	}

	msg.code = MVX_FW_CODE_BUFFER;
	msg.buf = buf;

	MVX_SESSION_INFO(session,
			 "Firmware req: Buffer. dir=%u, len=[%u, %u, %u], flags=0x%08x, eos=%u, interlace=%u",
			 buf->dir,
			 buf->planes[0].filled,
			 buf->planes[1].filled,
			 buf->planes[2].filled,
			 buf->flags,
			 (buf->flags & MVX_BUFFER_EOS) != 0,
			 (buf->flags & MVX_BUFFER_INTERLACE) != 0);

	ret = session->fw.ops.put_message(&session->fw, &msg);
	if (ret != 0)
		goto send_error;

	port->buffer_count++;
	port->buffers_in_window++;
	port->flushed = false;
	ret = send_irq(session);
	if (ret != 0)
		goto send_error;

	return 0;

send_error:
	send_event_error(session, ret);
	return ret;
}

/**
 * queue_pending_buffers() - Queue pending buffers.
 *
 * Buffer that are queued when the port is still stream off will be put in the
 * pending queue. Once both input- and output ports are stream on the pending
 * buffers will be forwarded to the firmware.
 *
 * Return: 0 on success, else error code.
 */
static int queue_pending_buffers(struct mvx_session *session,
				 enum mvx_direction dir)
{
	struct mvx_buffer *buf;
	struct mvx_buffer *tmp;
	int roi_config_num = 0;
	int roi_config_index = 0;
	int qp_num = 0;
	int qp_index = 0;
	int chr_cfg_num = 0;
	int chr_cfg_index = 0;
	int enc_stats_num = 0;
	int enc_stats_index = 0;
	int osd_cfg_index = 0;
	int osd_cfg_num = 0;
	int pending_buf_idx = 0;
	int osd_buffer_idx = 0;
	struct mvx_roi_config roi_config;
	int ret = 0;

	if (dir == MVX_DIR_INPUT && session->port[dir].roi_config_num > 0)
		roi_config_num = session->port[dir].roi_config_num;
	if (dir == MVX_DIR_INPUT && session->port[dir].qp_num > 0)
		qp_num = session->port[dir].qp_num;
	if (dir == MVX_DIR_INPUT && session->port[dir].chr_cfg_num > 0)
		chr_cfg_num = session->port[dir].chr_cfg_num;
	if (dir == MVX_DIR_INPUT && session->port[dir].enc_stats_num > 0)
		enc_stats_num = session->port[dir].enc_stats_num;
	if (dir == MVX_DIR_INPUT && session->port[dir].osd_cfg_num > 0)
		osd_cfg_num = session->port[dir].osd_cfg_num;
	list_for_each_entry_safe(buf, tmp, &session->port[dir].buffer_queue,
				 head) {
		buf->flags = buf->in_flags;
		if ((buf->flags & MVX_BUFFER_FRAME_FLAG_ROI) ==
		    MVX_BUFFER_FRAME_FLAG_ROI
		    && roi_config_index < roi_config_num) {
			roi_config =
			    session->port[dir].roi_config_queue[roi_config_index];
			ret = queue_roi_regions(session, &roi_config);
			roi_config_index++;
		}
		if ((buf->flags & MVX_BUFFER_FRAME_FLAG_GENERAL) ==
		    MVX_BUFFER_FRAME_FLAG_GENERAL && qp_index < qp_num) {
			ret =
			    queue_qp_epr(session,
					 &session->port[dir].qp_queue[qp_index]);
			qp_index++;
		}
		if ((buf->flags & MVX_BUFFER_FRAME_FLAG_CHR) ==
		    MVX_BUFFER_FRAME_FLAG_CHR && chr_cfg_index < chr_cfg_num) {
			ret =
			    queue_chr_cfg(session,
					  &session->port[dir].chr_cfg_queue
					  [chr_cfg_index]);
			chr_cfg_index++;
		}
		if (enc_stats_index < enc_stats_num &&
		    session->port[dir].enc_stats_queue[enc_stats_index].pic_index ==
		    pending_buf_idx) {
			ret =
			    queue_enc_stats(session,
					    &session->port[dir].enc_stats_queue[enc_stats_index]);
			enc_stats_index++;
		}
		if (osd_cfg_index < osd_cfg_num &&
		    session->port[dir].osd_cfg_queue[osd_cfg_index].pic_index ==
		    osd_buffer_idx) {
			ret =
			    queue_osd_config(session,
					     &session->port[dir].osd_cfg_queue
					     [osd_cfg_index]);
			osd_cfg_index++;
		}
		ret = queue_buffer(session, dir, buf);
		pending_buf_idx++;
		if (!(buf->flags & MVX_BUFFER_FRAME_FLAG_OSD_MASK))
			osd_buffer_idx++;	//check for yuv bffer
		if ((buf->flags & MVX_BUFFER_FRAME_NEED_REALLOC) ==
		    MVX_BUFFER_FRAME_NEED_REALLOC) {
			session->event(session, MVX_SESSION_EVENT_BUFFER, buf);
		} else if (ret != 0) {
			break;
		}
		list_del(&buf->head);
	}

	session->port[dir].roi_config_num = 0;
	session->port[dir].qp_num = 0;
	session->port[dir].chr_cfg_num = 0;
	session->port[dir].enc_stats_num = 0;
	return ret;
}

/**
 * return_done_buffers() - Return buffers in done_queue to client.
 *
 * When resolution changed in non-keyframe, keep frames with the former resolution
 * in session->buffer_done_queue.
 *
 * In alloc_param msg handler, add the former resolution buffers and return them
 * to client and send PORT_CHANGED EVENT later.
 *
 */
static void return_done_buffers(struct mvx_session *session,
				enum mvx_direction dir)
{
	struct mvx_buffer *buf;
	struct mvx_buffer *tmp;

	list_for_each_entry_safe(buf, tmp,
				 &session->port[MVX_DIR_OUTPUT].buffer_done_queue, head) {
		list_del(&buf->head);
		session->event(session, MVX_SESSION_EVENT_BUFFER, buf);
	}
}

/**
 * fw_bin_ready() - Complete firmware configuration.
 *
 * The firmware binary load has completed and the firmware configuration can
 * begin.
 *
 * If the session is no longer 'stream on' (someone issued 'stream off' before
 * the firmware load completed) the firmware binary is put back to the cache.
 *
 * Else the client session is registered and the firmware instance is
 * constructed.
 */
static void fw_bin_ready(struct mvx_fw_bin *bin, void *arg, bool same_thread)
{
	struct mvx_session *session = arg;
	int lock_failed = 1;
	int ret;

	/*
	 * Only lock the mutex if the firmware binary was loaded by a
	 * background thread.
	 */
	if (same_thread == false) {
		lock_failed = mutex_lock_interruptible(session->isession.mutex);
		if (lock_failed != 0) {
			send_event_error(session, lock_failed);
			goto put_fw_bin;
		}
	}

	/* Return firmware binary if session is no longer 'stream on'. */
	if (!is_stream_on(session))
		goto put_fw_bin;

	/* Create client session. */
	session->isession.core_mask =
	    session->client_ops->get_core_mask(session->client_ops);
	session->isession.ncores = hweight32(session->isession.core_mask);
	session->isession.l0_pte =
	    mvx_mmu_set_pte(MVX_ATTR_PRIVATE,
			    virt_to_phys(session->mmu.page_table),
			    MVX_ACCESS_READ_WRITE);

	session->csession =
	    session->client_ops->register_session(session->client_ops,
						  &session->isession);
	if (IS_ERR(session->csession)) {
		ret = PTR_ERR(session->csession);
		send_event_error(session, ret);
		goto put_fw_bin;
	}

	/* Construct the firmware instance. */
	ret = mvx_fw_factory(&session->fw, bin, &session->mmu,
			     session, session->client_ops, session->csession,
			     session->isession.core_mask, session->dentry);
	if (ret != 0) {
		send_event_error(session, ret);
		goto unregister_csession;
	}

	session->fw_bin = bin;
	complete(&session->fw_loaded);

	mvx_fw_cache_log(bin, session->csession);

	ret = fw_initial_setup(session);
	if (ret != 0)
		goto unregister_csession;

	ret = queue_pending_buffers(session, MVX_DIR_INPUT);
	if (ret != 0)
		goto unregister_csession;

	ret = queue_pending_buffers(session, MVX_DIR_OUTPUT);
	if (ret != 0)
		goto unregister_csession;

	if (lock_failed == 0)
		mutex_unlock(session->isession.mutex);

	mvx_session_put(session);

	return;

unregister_csession:
	session->client_ops->unregister_session(session->csession);
	session->csession = NULL;

put_fw_bin:
	mvx_fw_cache_put(session->cache, bin);
	session->fw_bin = NULL;
	complete(&session->fw_loaded);

	if (lock_failed == 0)
		mutex_unlock(session->isession.mutex);

	mvx_session_put(session);
}

static int calc_afbc_size(struct mvx_session *session,
			  enum mvx_format format,
			  unsigned int width,
			  unsigned int height,
			  bool tiled_headers,
			  bool tiled_body, bool superblock, bool interlaced)
{
	static const unsigned int mb_header_size = 16;
	unsigned int payload_align = 128;
	unsigned int mb_size;
	int size;

	/* Calculate width and height in super blocks. */
    /**
     * FIXME: Add superblock calculation back.
     * So far, doesn't find a good way to transfer superblock info when set/try
     * format, in 16x16 case, the calculated size of 32x8 tiled may larger than
     * actually allocated by client(e.g gralloc). And there's no actually 32x8 encode
     * requirement. So, remove superblock calculation temporoarily.
     **/
	if (/* superblock != false */ false) {
		width = DIV_ROUND_UP(width, 32);
		height = DIV_ROUND_UP(height, 8) + 1;
	} else {
		width = DIV_ROUND_UP(width, 16);
		height = DIV_ROUND_UP(height, 16) + 1;
	}

	/* Round up size to 8x8 tiles. */
	if (tiled_headers != false || tiled_body != false) {
		width = roundup(width, 8);
		height = roundup(height, 8);
	}

	switch (format) {
	case MVX_FORMAT_YUV420_AFBC_8:
		mb_size = 384;
		break;
	case MVX_FORMAT_YUV420_AFBC_10:
		mb_size = 480;
		break;
	case MVX_FORMAT_YUV422_AFBC_8:
		mb_size = 512;
		break;
	case MVX_FORMAT_YUV422_AFBC_10:
		mb_size = 656;
		break;
	case MVX_FORMAT_Y_AFBC_8:
		mb_size = 256;
		break;
	case MVX_FORMAT_Y_AFBC_10:
		mb_size = 320;
		break;
	default:
		MVX_SESSION_WARN(session,
				 "Unsupported AFBC format. format=%u.", format);
		return -EINVAL;
	}

	/* Round up tiled body to 128 byte boundary. */
	if (tiled_body != false)
		mb_size = roundup(mb_size, payload_align);

	if (interlaced != false)
		height = DIV_ROUND_UP(height, 2);

	/* Calculate size of AFBC makroblock headers. */
	size = roundup(width * height * mb_header_size, payload_align);
	size += roundup(width * height * mb_size, payload_align);

	if (interlaced != false)
		size *= 2;

	return size;
}

static size_t divRoundUp(size_t value, size_t round)
{
	return (value + round - 1) / round;
}

static size_t roundUp(size_t value, size_t round)
{
	return divRoundUp(value, round) * round;
}

static int try_format(struct mvx_session *session,
		      enum mvx_direction dir,
		      enum mvx_format format,
		      unsigned int *width,
		      unsigned int *height,
		      uint8_t *nplanes,
		      unsigned int *stride,
		      unsigned int *size, bool *interlaced)
{
	int ret = 0;

	if ((session->is_encoder && dir == MVX_DIR_INPUT
	     && !mvx_is_frame(format)) || (!session->is_encoder
					   && dir == MVX_DIR_OUTPUT
					   && !mvx_is_frame(format)))
		return -EINVAL;

	if (dir == MVX_DIR_INPUT && format == MVX_FORMAT_JPEG) {
		/* Limit width and height to 32k for jpeg decode. */
		*width = min_t(unsigned int, *width, 32768);
		*height = min_t(unsigned int, *height, 32768);
	} else if ((dir == MVX_DIR_OUTPUT && format == MVX_FORMAT_JPEG) ||
		   (dir == MVX_DIR_INPUT && mvx_is_frame(format))) {
		/* Limit output width and height to 16k for jpeg encode. */
		*width = min_t(unsigned int, *width, 16384);
		*height = min_t(unsigned int, *height, 16384);
	} else {
		/* Limit width and height to 8k. */
		*width = min_t(unsigned int, *width, 8192);
		*height = min_t(unsigned int, *height, 8192);
	}

	/* Limit minimum width and height. */
	*width = max_t(unsigned int, *width, 2);
	*height = max_t(unsigned int, *height, 2);

	if (mvx_is_frame(format) && !mvx_is_afbc(format)
	    && dir == MVX_DIR_OUTPUT) {
		*width =
		    session->orig_width >> session->port[MVX_DIR_OUTPUT].scaling_shift;
		*height =
		    session->orig_height >> session->port[MVX_DIR_OUTPUT].scaling_shift;
		if (session->dsl_frame.width >= 16
		    && session->dsl_frame.height >= 16) {
			*width = session->dsl_frame.width;
			*height = session->dsl_frame.height;
		} else if (session->dsl_ratio.hor != 1
			   || session->dsl_ratio.ver != 1) {
			*width = session->orig_width / session->dsl_ratio.hor;
			*height = session->orig_height / session->dsl_ratio.ver;
			*width &= ~(1);
			*height &= ~(1);
		}
	} else if (dir == MVX_DIR_OUTPUT) {
		*width = session->orig_width;
		*height = session->orig_height;
	}

	if (dir == MVX_DIR_OUTPUT && !mvx_is_afbc(format)
	    && session->crop.crop_en != 0
	    && session->crop.width > 0
	    && session->crop.height > 0
	    && session->crop.width + session->crop.x <= session->orig_width
	    && session->crop.height + session->crop.y <= session->orig_height) {
		*width = session->crop.width;
		*height = session->crop.height;
	}
	/* Interlaced input is not supported by the firmware. */
	if (dir == MVX_DIR_INPUT && mvx_is_frame(format))
		*interlaced = false;

	if (mvx_is_afbc(format) != false) {
		unsigned int afbc_alloc_bytes =
		    session->port[dir].afbc_alloc_bytes;
		if (*nplanes <= 0)
			size[0] = 0;
		size[1] = 0;

		if (dir == MVX_DIR_OUTPUT
		    && session->dual_afbc_downscaled
		    && *interlaced == 0
		    && session->port[dir].afbc_alloc_bytes_downscaled != 0) {
			unsigned int afbc_alloc_bytes_downscaled =
			    session->port[dir].afbc_alloc_bytes_downscaled;

			size[0] =
			    roundup(afbc_alloc_bytes_downscaled, PAGE_SIZE);
			size[1] =
			    max_t(unsigned int, PAGE_SIZE, afbc_alloc_bytes);
			size[1] = roundup(size[1], PAGE_SIZE);

			*width = session->orig_width >> 1;
			*height = session->orig_height >> 1;
		} else {
			if (dir == MVX_DIR_INPUT) {
				/* it is basically a worst-case calcualtion based on a size rounded up to tile size */
				int s1 = calc_afbc_size(session, format, *width,
							*height, true, true, false,	//*height, false, false, false,
							*interlaced);
				int s2 = calc_afbc_size(session, format, *width,
							*height, true, true, true,	//*height, false, false, false,
							*interlaced);
				int s = max_t(unsigned int, s1, s2);

				if (s < 0)
					return s;

				size[0] = max_t(unsigned int, size[0], s);
			}

			if (*interlaced != false)
				afbc_alloc_bytes *= 2;

			/* Size should be at least one page. */
			size[0] = max_t(unsigned int, size[0], PAGE_SIZE);
			size[0] = max_t(unsigned int, size[0],
					afbc_alloc_bytes);
			size[0] = roundup(size[0], PAGE_SIZE);
		}

		*nplanes = 1;
	} else if (mvx_is_frame(format) != false) {
		uint32_t tmp_height =
		    session->mini_frame_cnt >=
		    2 ? roundUp(divRoundUp(*height, session->mini_frame_cnt),
				64) : *height;
		ret =
		    mvx_buffer_frame_dim(format, *width, tmp_height, nplanes,
					 stride, size, session->setting_stride);
	} else {
		/*
		 * For compressed formats the size should be the maximum number
		 * of bytes an image is expected to become. This is calculated
		 * as width * height * 2 B/px / 2. Size should be at least one
		 * page. For decode, limit to 256MB. For encode, some client like
		 * gstreamer may allocate 5 bitstream buffers, so limit to 192MB
		 * which is big enough for 16Kx16K random pixel JPEG encoding
		 * with quality level 90.
		 */

		stride[0] = 0;

		if (*nplanes <= 0)
			size[0] = 0;

		size[0] = max_t(unsigned int, size[0], PAGE_SIZE);

		if ((*width) * (*height) < 720 * 480)
			size[0] =
			    max_t(unsigned int, size[0],
				  (*width) * (*height) * 3);
		else
			size[0] =
			    max_t(unsigned int, size[0], (*width) * (*height));

		if (dir == MVX_DIR_OUTPUT)
			size[0] =
			    min_t(unsigned int, size[0],
				  (MAX_BITSTREAM_BUFFER_SIZE * 3) >> 2);
		else
			size[0] =
			    min_t(unsigned int, size[0],
				  MAX_BITSTREAM_BUFFER_SIZE);
		size[0] = roundup(size[0], PAGE_SIZE);

		*nplanes = 1;
	}

	MVX_SESSION_INFO(session,
			 "%s(), dir=%d fmt=%d w=%d h=%d planes=%d interlaced=%d stride=[%d %d %d] size=[%d %d %d]",
			 __func__, dir, format, *width, *height, *nplanes,
			 *interlaced, stride[0], stride[1], stride[2], size[0],
			 size[1], size[2]);
	return ret;
}

static void watchdog_work(struct work_struct *work)
{
	struct mvx_session *session =
	    container_of(work, struct mvx_session, watchdog_work);
	int log_level =
	    session->watchdog_count > 2 ? MVX_WAR_LOG_LEVEL : MVX_LOG_INFO;
	int ret;

	mutex_lock(session->isession.mutex);

	MVX_SESSION_LOG(log_level, session, "Watchdog timeout. count=%u.",
			session->watchdog_count);

	/* watchdog_count < 15 means total timeout is 12s */
	if (session->watchdog_count++ < 15) {
		if (session->switched_in) {
			/* Restart watchdog. */
			unsigned int timeout_ms =
			    session_watchdog_timeout * session->watchdog_count;
			watchdog_start(session, timeout_ms, false);
		}
	} else {
		/* Print debug information. */
		print_debug(session);
		/* Request firmware to dump its state. */
		fw_dump(session);
		session->client_ops->terminate(session->csession);
		switch_out_rsp(session);
		send_event_error(session, -ETIME);
	}

	ret = kref_put(&session->isession.kref, session->isession.release);
	if (ret != 0)
		return;

	mutex_unlock(session->isession.mutex);

	session->client_ops->soft_irq(session->csession);
}

static void watchdog_timeout(struct timer_list *timer)
{
	struct mvx_session *session =
	    container_of(timer, struct mvx_session, watchdog_timer);

	queue_work(system_unbound_wq, &session->watchdog_work);
}

static void filter_decode_output_formats(enum mvx_format compressed_format,
					 int bitdepth, int chroma_format,
					 uint64_t *formats)
{
	mvx_clear_bit(MVX_FORMAT_RGBA_8888, formats);
	mvx_clear_bit(MVX_FORMAT_BGRA_8888, formats);
	mvx_clear_bit(MVX_FORMAT_ARGB_8888, formats);
	mvx_clear_bit(MVX_FORMAT_ABGR_8888, formats);

	if (compressed_format == MVX_FORMAT_JPEG) {
		mvx_clear_bit(MVX_FORMAT_YUV420_AFBC_10, formats);
		mvx_clear_bit(MVX_FORMAT_YUV422_AFBC_10, formats);
		mvx_clear_bit(MVX_FORMAT_Y_AFBC_10, formats);
		mvx_clear_bit(MVX_FORMAT_YUV420_P010, formats);
		mvx_clear_bit(MVX_FORMAT_YUV422_Y210, formats);
		mvx_clear_bit(MVX_FORMAT_Y_10, formats);
		if (chroma_format == MVX_CHROMA_FORMAT_MONO ||
		    chroma_format == MVX_CHROMA_FORMAT_420) {
			mvx_clear_bit(MVX_FORMAT_YUV422_YUY2, formats);
			mvx_clear_bit(MVX_FORMAT_YUV422_UYVY, formats);
			mvx_clear_bit(MVX_FORMAT_YUV422_AFBC_8, formats);
			if (chroma_format == MVX_CHROMA_FORMAT_MONO)
				mvx_clear_bit(MVX_FORMAT_YUV420_AFBC_8,
					      formats);
			else
				mvx_clear_bit(MVX_FORMAT_Y_AFBC_8, formats);
		}
	} else if (compressed_format == MVX_FORMAT_AVS2 ||
		   compressed_format == MVX_FORMAT_H264 ||
		   compressed_format == MVX_FORMAT_HEVC ||
		   compressed_format == MVX_FORMAT_VP9 ||
		   compressed_format == MVX_FORMAT_AV1) {
		mvx_clear_bit(MVX_FORMAT_YUV422_AFBC_8, formats);
		mvx_clear_bit(MVX_FORMAT_YUV422_AFBC_10, formats);
		mvx_clear_bit(MVX_FORMAT_YUV422_YUY2, formats);
		mvx_clear_bit(MVX_FORMAT_YUV422_UYVY, formats);
		mvx_clear_bit(MVX_FORMAT_YUV422_Y210, formats);
		/* 8bit to 10bit post-processing is not supported */
		if (bitdepth == 8) {
			mvx_clear_bit(MVX_FORMAT_YUV420_AFBC_10, formats);
			mvx_clear_bit(MVX_FORMAT_YUV420_P010, formats);
			mvx_clear_bit(MVX_FORMAT_Y_10, formats);
			if (chroma_format == MVX_CHROMA_FORMAT_MONO)
				mvx_clear_bit(MVX_FORMAT_YUV420_AFBC_8,
					      formats);
			else
				mvx_clear_bit(MVX_FORMAT_Y_AFBC_8, formats);
		} else if (bitdepth == 10) {
			if (chroma_format == MVX_CHROMA_FORMAT_MONO)
				mvx_clear_bit(MVX_FORMAT_YUV420_AFBC_10,
					      formats);
			else
				mvx_clear_bit(MVX_FORMAT_Y_AFBC_10, formats);
		}
	} else if (compressed_format < MVX_FORMAT_BITSTREAM_LAST) {
		mvx_clear_bit(MVX_FORMAT_YUV420_AFBC_10, formats);
		mvx_clear_bit(MVX_FORMAT_YUV422_AFBC_10, formats);
		mvx_clear_bit(MVX_FORMAT_Y_AFBC_10, formats);
		mvx_clear_bit(MVX_FORMAT_YUV420_P010, formats);
		mvx_clear_bit(MVX_FORMAT_YUV422_Y210, formats);
		mvx_clear_bit(MVX_FORMAT_Y_10, formats);
		mvx_clear_bit(MVX_FORMAT_YUV422_AFBC_8, formats);
		mvx_clear_bit(MVX_FORMAT_YUV422_YUY2, formats);
		mvx_clear_bit(MVX_FORMAT_YUV422_UYVY, formats);
		if (chroma_format == MVX_CHROMA_FORMAT_MONO)
			mvx_clear_bit(MVX_FORMAT_YUV420_AFBC_8, formats);
		else
			mvx_clear_bit(MVX_FORMAT_Y_AFBC_8, formats);
	}
}

static void reset_resolution(struct mvx_session *session,
			     unsigned int *width, unsigned int *height,
			     enum mvx_direction dir)
{
	if (dir == MVX_DIR_INPUT && *width >= 0 && *height >= 0 &&
	    session->orig_width < 144 && session->orig_height < 144) {
		session->orig_width = *width;
		session->orig_height = *height;
	} else if (session->orig_width >= 144 && session->orig_height >= 144) {
		*width = session->orig_width;
		*height = session->orig_height;
	}
}

static int flush_and_qbufs(struct mvx_session *session, enum mvx_direction dir)
{
	int ret = 0;

	if (session->port[MVX_DIR_OUTPUT].received_seq_param ||
	    session->fw_state == MVX_FW_STATE_STOPPED) {
		session->port[MVX_DIR_OUTPUT].received_seq_param = false;
		ret = fw_flush(session, dir);
		if (ret != 0)
			return ret;
		ret = wait_pending(session);
		if (ret != 0)
			return ret;
	}
	ret = queue_pending_buffers(session, dir);
	if (ret != 0)
		return ret;

	return switch_in(session);
}

static void flush_and_qbufs_work(struct work_struct *work)
{
	struct mvx_session *session =
	    container_of(work, struct mvx_session, flush_and_queue_work);
	int ret;

	ret = mutex_lock_interruptible(session->isession.mutex);
	if (ret != 0)
		return;
	flush_and_qbufs(session, MVX_DIR_OUTPUT);
	mutex_unlock(session->isession.mutex);
}

/****************************************************************************
 * Exported functions
 ****************************************************************************/

int mvx_session_construct(struct mvx_session *session,
			  struct device *dev,
			  struct mvx_client_ops *client_ops,
			  struct mvx_fw_cache *cache,
			  struct mutex *mutex,
			  void (*destructor)(struct mvx_session *session),
			  void (*event)(struct mvx_session *session,
					enum mvx_session_event event,
					void *arg),
			  struct dentry *dentry, bool is_encoder)
{
	int i;
	int ret;

	if (event == NULL || destructor == NULL)
		return -EINVAL;

	memset(session, 0, sizeof(*session));
	memset(session->setting_stride, 0, sizeof(session->setting_stride));
	memset(session->port[MVX_DIR_INPUT].display_size, 0,
	       sizeof(session->port[MVX_DIR_INPUT].display_size));
	memset(session->port[MVX_DIR_OUTPUT].display_size, 0,
	       sizeof(session->port[MVX_DIR_OUTPUT].display_size));
	session->dev = dev;
	session->client_ops = client_ops;
	session->cache = cache;
	kref_init(&session->isession.kref);
	session->isession.release = session_destructor;
	session->isession.mutex = mutex;
	session->destructor = destructor;
	session->event = event;
	session->fw_event.fw_bin_ready = fw_bin_ready;
	session->fw_event.arg = session;
	session->fw_state = MVX_FW_STATE_STOPPED;
	init_waitqueue_head(&session->waitq);
	session->dentry = dentry;
	session->port[MVX_DIR_INPUT].buffer_min = 1;
	session->port[MVX_DIR_OUTPUT].buffer_min = 1;
	session->port[MVX_DIR_OUTPUT].buffer_max = VIDEO_MAX_FRAME;
	session->port[MVX_DIR_INPUT].buffer_allocated = 0;	//1;
	session->port[MVX_DIR_OUTPUT].buffer_allocated = 0;	//1;
	session->port[MVX_DIR_INPUT].scaling_shift = 0;
	session->port[MVX_DIR_OUTPUT].scaling_shift = 0;
	session->port[MVX_DIR_INPUT].afbc_alloc_bytes = 0;
	session->port[MVX_DIR_OUTPUT].afbc_alloc_bytes = 0;
	session->port[MVX_DIR_INPUT].afbc_width = 0;
	session->port[MVX_DIR_OUTPUT].afbc_width = 0;
	session->port[MVX_DIR_INPUT].afbc_alloc_bytes_downscaled = 0;
	session->port[MVX_DIR_OUTPUT].afbc_alloc_bytes_downscaled = 0;
	session->port[MVX_DIR_INPUT].afbc_width_in_superblocks_downscaled = 0;
	session->port[MVX_DIR_OUTPUT].afbc_width_in_superblocks_downscaled = 0;
	session->port[MVX_DIR_INPUT].last_interlaced_from_sps = 0;
	session->port[MVX_DIR_OUTPUT].last_interlaced_from_sps = 0;
	session->stream_escaping = MVX_TRI_UNSET;
	session->ignore_stream_headers = MVX_TRI_UNSET;
	session->frame_reordering = MVX_TRI_UNSET;
	session->constr_ipred = MVX_TRI_UNSET;
	session->entropy_sync = MVX_TRI_UNSET;
	session->temporal_mvp = MVX_TRI_UNSET;
	session->resync_interval = -1;
	session->port[MVX_DIR_OUTPUT].roi_config_num = 0;
	session->port[MVX_DIR_INPUT].roi_config_num = 0;
	session->port[MVX_DIR_OUTPUT].qp_num = 0;
	session->port[MVX_DIR_INPUT].qp_num = 0;
	session->crop_left = 0;
	session->crop_right = 0;
	session->crop_top = 0;
	session->crop_bottom = 0;
	session->dsl_ratio.hor = 1;
	session->dsl_ratio.ver = 1;
	session->dsl_pos_mode = -1;	//disable by default
	session->rc_bit_i_mode = 0;
	session->rc_bit_i_ratio = 0;
	session->mulit_sps_pps = 0;
	session->enable_visual = 0;
	session->forced_uv_value = 0x400;
	session->dsl_interp_mode = 0xffff;
	session->color_conv_mode = MVX_YUV_TO_RGB_MODE_BT601_LIMT;
	session->use_cust_color_conv_coef = false;
	session->use_cust_rgb_to_yuv_mode = MVX_CUST_YUV2RGB_MODE_UNSET;
	session->dual_afbc_downscaled = 0;
	session->job_frames = 1;
	session->fps_n = 60;	// 60fps by default
	session->fps_d = 1;
	session->coded_chroma_format = MVX_CHROMA_FORMAT_422;
	session->is_encoder = is_encoder;
	session->enable_buffer_dump = enable_buffer_dump;
	session->priority = V4L2_SESSION_PRIORITY_DEFAULT;
	session->intra_ipenalty_angular = MVX_INVALID_VAL;
	session->intra_ipenalty_planar = MVX_INVALID_VAL;
	session->intra_ipenalty_dc = MVX_INVALID_VAL;
	session->inter_ipenalty_angular = MVX_INVALID_VAL;
	session->inter_ipenalty_planar = MVX_INVALID_VAL;
	session->inter_ipenalty_dc = MVX_INVALID_VAL;

	if (session->enable_buffer_dump == true) {
		char input_file[64];

		scnprintf(input_file, sizeof(input_file) - 1,
			  "/data/input_session_%p.bin", session);
		session->data_fp =
		    filp_open(input_file, O_RDWR | O_CREAT, 0644);
		if (IS_ERR(session->data_fp)) {
			MVX_SESSION_WARN(session,
					 "warning: open dump file(%s) fail",
					 input_file);
			session->data_fp = NULL;
		}
	}

	memset(&session->color_conv_coef, 0,
	       sizeof(struct mvx_color_conv_coef));
	memset(&session->crop, 0, sizeof(struct mvx_crop_cfg));
	memset(&session->seamless_target, 0,
	       sizeof(struct mvx_seamless_target));
	memset(&session->lambda_scale, 0, sizeof(session->lambda_scale));
	init_completion(&session->fw_loaded);

	ret = mvx_mmu_construct(&session->mmu, session->dev);
	if (ret != 0)
		return ret;

	for (i = 0; i < MVX_DIR_MAX; i++) {
		INIT_LIST_HEAD(&session->port[i].buffer_queue);
		INIT_LIST_HEAD(&session->port[i].buffer_done_queue);
	}

	timer_setup(&session->watchdog_timer, watchdog_timeout, 0);
	INIT_WORK(&session->watchdog_work, watchdog_work);
	INIT_WORK(&session->flush_and_queue_work, flush_and_qbufs_work);

	if (mvx_log_perf.enabled & MVX_LOG_PERF_FPS) {
		session->ts =
		    vmalloc(sizeof(struct timespec64) * MAX_RT_FPS_FRAMES);
		mutex_init(&session->fps_mutex);
	}

	return 0;
}

void mvx_session_destruct(struct mvx_session *session)
{
	/* Destruct the session object. */

	MVX_SESSION_INFO(session, "Destroy session.");

	// dump input IVF header into a new file in the same dir with bitstream file
	if (session->data_fp != NULL) {
		filp_close(session->data_fp, NULL);
		dump_ivf_header(session);
	}
	release_fw_bin(session);
	mvx_mmu_destruct(&session->mmu);
	if (session->ts)
		vfree(session->ts);
}

void mvx_session_get(struct mvx_session *session)
{
	kref_get(&session->isession.kref);
}

int mvx_session_put(struct mvx_session *session)
{
	return kref_put(&session->isession.kref, session->isession.release);
}

unsigned int mvx_session_ref_read(struct mvx_session *session)
{
	return kref_read(&session->isession.kref);
}

struct mvx_session_format_map *mvx_session_enum_format(struct mvx_session
						       *session,
						       enum mvx_direction dir,
						       int index)
{
	struct mvx_session_format_map *mvx_fmts = NULL;
	bool is_encoder = session->is_encoder;
	uint64_t formats;
	int idx;
	int i;
	int array_size;

	mvx_session_get_formats(session, dir, &formats);
	if (!is_encoder && dir == MVX_DIR_OUTPUT)
		filter_decode_output_formats(session->port[MVX_DIR_INPUT].format,
					     max(session->bitdepth_luma,
						 session->bitdepth_chroma),
					     session->coded_chroma_format,
					     &formats);

	if ((is_encoder && dir == MVX_DIR_OUTPUT) ||
	    (!is_encoder && dir == MVX_DIR_INPUT)) {
		mvx_fmts = mvx_compressed_fmts;
		array_size = ARRAY_SIZE(mvx_compressed_fmts);
	} else {
		mvx_fmts = mvx_raw_fmts;
		array_size = ARRAY_SIZE(mvx_raw_fmts);
	}

	for (i = 0, idx = 0; i < array_size; i++) {
		if (mvx_test_bit(mvx_fmts[i].format, &formats)) {
			if (index == idx)
				return &mvx_fmts[i];

			idx++;
		}
	}

	return NULL;
}

void mvx_session_get_formats(struct mvx_session *session,
			     enum mvx_direction dir, uint64_t *formats)
{
	uint64_t fw_formats;

	session->client_ops->get_formats(session->client_ops, dir, formats);
	mvx_fw_cache_get_formats(session->cache, dir, &fw_formats);
	mvx_clear_bit(MVX_FORMAT_YUV420_Y0L2, &fw_formats);
	mvx_clear_bit(MVX_FORMAT_YUV420_AQB1, &fw_formats);
	mvx_clear_bit(MVX_FORMAT_YUV444, &fw_formats);
	mvx_clear_bit(MVX_FORMAT_YUV444_10, &fw_formats);
	mvx_clear_bit(MVX_FORMAT_YUV420_2P_10, &fw_formats);
	mvx_clear_bit(MVX_FORMAT_YUV422_1P_10, &fw_formats);
	mvx_clear_bit(MVX_FORMAT_YUV420_I420_10, &fw_formats);
	mvx_clear_bit(MVX_FORMAT_BGR_888, &fw_formats);
	mvx_clear_bit(MVX_FORMAT_ARGB_1555, &fw_formats);
	mvx_clear_bit(MVX_FORMAT_ARGB_4444, &fw_formats);
	mvx_clear_bit(MVX_FORMAT_RGB_565, &fw_formats);
	mvx_clear_bit(MVX_FORMAT_VC1, &fw_formats);
	mvx_clear_bit(MVX_FORMAT_RV, &fw_formats);
	mvx_clear_bit(MVX_FORMAT_AVS, &fw_formats);
	mvx_clear_bit(MVX_FORMAT_AVS2, &fw_formats);
	*formats &= fw_formats;
}

int mvx_session_try_format(struct mvx_session *session,
			   enum mvx_direction dir,
			   enum mvx_format format,
			   unsigned int *width,
			   unsigned int *height,
			   uint8_t *nplanes,
			   unsigned int *stride,
			   unsigned int *size, bool *interlaced)
{
	return try_format(session, dir, format, width, height, nplanes,
			  stride, size, interlaced);
}

int mvx_session_set_format(struct mvx_session *session,
			   enum mvx_direction dir,
			   enum mvx_format format,
			   unsigned int pixelformat,
			   unsigned int *width,
			   unsigned int *height,
			   uint8_t *nplanes,
			   unsigned int *stride,
			   unsigned int *size, bool *interlaced)
{
	struct mvx_session_port *port = &session->port[dir];
	int ret;

	if (session->error != 0)
		return session->error;

	if (mvx_is_afbc(format) != false &&
	    session->coded_chroma_format == MVX_CHROMA_FORMAT_MONO &&
	    (format != MVX_FORMAT_Y_AFBC_8 && format != MVX_FORMAT_Y_AFBC_10))
		return -EINVAL;

	if (session->port[dir].stream_on != false)
		return -EBUSY;

	reset_resolution(session, width, height, dir);

	ret = try_format(session, dir, format, width, height, nplanes,
			 stride, size, interlaced);
	if (ret != 0)
		return ret;

	/*
	 * If the bitstream format changes, then the firmware binary must be
	 * released.
	 */
	if (mvx_is_bitstream(port->format) != false && format != port->format) {
		if (IS_ERR(session->fw_bin) != false) {
			MVX_SESSION_WARN(session,
					 "Can't set format when firmware binary is pending. dir=%d.",
					 dir);
			return -EINVAL;
		}

		release_fw_bin(session);
	}

	/* Update port settings. */
	port->format = format;
	port->old_format = format;
	port->pixelformat = pixelformat;
	port->old_pixelformat = pixelformat;
	port->width = *width;
	port->height = *height;
	port->nplanes = *nplanes;
	port->interlaced = *interlaced;
	memcpy(port->stride, stride, sizeof(*stride) * MVX_BUFFER_NPLANES);
	memcpy(port->size, size, sizeof(*size) * MVX_BUFFER_NPLANES);

	/* TODO AFBC width will have to be provided by user space. */
	if (dir == MVX_DIR_INPUT)
		port->afbc_width = DIV_ROUND_UP(*width, 16);

	/* Input dimensions dictate output dimensions. */
	if (dir == MVX_DIR_INPUT) {
		struct mvx_session_port *p = &session->port[MVX_DIR_OUTPUT];
		(void)try_format(session, MVX_DIR_OUTPUT, p->format, &p->width,
				 &p->height, &p->nplanes, p->stride, p->size,
				 &p->interlaced);
	}

	if (mvx_is_afbc(format) != false) {
		if (dir == MVX_DIR_OUTPUT &&
		    session->port[dir].afbc_width >=
		    AFBC_MIN_WIDTH_IN_SUPERBLOCKS) {
			if (session->dual_afbc_downscaled)
				*width =
				    session->port
				    [dir].afbc_width_in_superblocks_downscaled
				    << AFBC_SUPERBLOCK_SHIFT;
			else
				*width =
				    session->port[dir].afbc_width <<
				    AFBC_SUPERBLOCK_SHIFT;
		} else
				*width +=
					session->port[dir].afbc_crop_left >> session->dual_afbc_downscaled;

		*height +=
		    session->port[dir].afbc_crop_top >> session->dual_afbc_downscaled;
	}

	if (!session->is_encoder && dir == MVX_DIR_OUTPUT &&
	    format <= MVX_FORMAT_BITSTREAM_LAST) {
		MVX_SESSION_WARN(session,
				 "Decode session, compressed format %d is not supported for output.",
				 session->port[MVX_DIR_OUTPUT].format);
		return -EINVAL;
	}

	if (pixelformat == V4L2_PIX_FMT_H264_NO_SC &&
	    session->nalu_format == MVX_NALU_FORMAT_UNDEFINED)
		mvx_session_set_nalu_format(session,
					    MVX_NALU_FORMAT_FOUR_BYTE_LENGTH_FIELD);

	return 0;
}

int mvx_session_qbuf(struct mvx_session *session,
		     enum mvx_direction dir, struct mvx_buffer *buf)
{
	int ret;

	if (session->error != 0)
		return session->error;

	buf->in_flags = buf->flags;

	if (is_fw_loaded(session) == false ||
	    session->port[dir].is_flushing != false ||
	    (session->port[dir].in_port_changing == true
	     && session->port[dir].pending_source_change_event == false)) {
		list_add_tail(&buf->head, &session->port[dir].buffer_queue);
		return 0;
	}

	ret = queue_buffer(session, dir, buf);
	if (ret != 0)
		return ret;

	ret = switch_in(session);
	if (ret != 0)
		return ret;

	return 0;
}

int mvx_session_send_eos(struct mvx_session *session)
{
	struct mvx_session_port *port = &session->port[MVX_DIR_OUTPUT];
	struct mvx_buffer *buf;

	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return fw_eos(session);

	if (list_empty(&port->buffer_queue) != false) {
		MVX_SESSION_WARN(session,
				 "Unable to signal EOS. Output buffer queue empty.");
		return 0;
	}

	buf = list_first_entry(&port->buffer_queue, struct mvx_buffer, head);
	list_del(&buf->head);

	mvx_buffer_clear(buf);
	buf->flags |= MVX_BUFFER_EOS;

	session->event(session, MVX_SESSION_EVENT_BUFFER, buf);

	return 0;
}

int mvx_session_streamon(struct mvx_session *session, enum mvx_direction dir)
{
	enum mvx_direction bdir;
	struct mvx_hw_ver hw_ver;
	enum mvx_direction i;
	int ret;

	MVX_SESSION_INFO(session, "Stream on. dir=%u.", dir);

	/* Verify that we don't enable an already activated port. */
	if (session->port[dir].stream_on != false)
		return 0;

	session->port[dir].stream_on = true;

	/* Check that both ports are stream on. */
	if (!is_stream_on(session))
		return 0;

	/* Verify that a firmware binary load is not in progress. */
	if (IS_ERR(session->fw_bin)) {
		/* Unlock during wait time as the same mutex may be locked in fw_bin_ready() */
		mutex_unlock(session->isession.mutex);
		if (!wait_for_completion_timeout
		    (&session->fw_loaded, msecs_to_jiffies(3000))) {
			mutex_lock(session->isession.mutex);
			ret = IS_ERR(session->fw_bin);
			MVX_SESSION_WARN(session,
					 "Wait for firmware loading timeout.");
			goto disable_port;
		}
		mutex_lock(session->isession.mutex);
	}

	/*
	 * If capture port is streamed on without stream-off during port changing,
	 * need to send flush message to VPU here to finish port changing.
	 * Otherwise, VPU firmware will get hang.
	 */
	if (dir == MVX_DIR_OUTPUT && session->port[dir].in_port_changing) {
		return_done_buffers(session, dir);
		flush_and_qbufs(session, dir);
		session->port[dir].in_port_changing = false;
		session->port[dir].hold_off_buffer_done = false;
	}

	/* If a firmware binary is already loaded, then we are done. */
	if (session->fw_bin != NULL) {
		ret = wait_pending(session);
		if (ret != 0)
			goto disable_port;

		ret = fw_state_change(session, MVX_FW_STATE_RUNNING);
		if (ret != 0)
			goto disable_port;

		return 0;
	}

	bdir = get_bitstream_port(session);
	if (bdir >= MVX_DIR_MAX) {
		MVX_SESSION_WARN(session,
				 "Session only support decoding and encoding, but not transcoding. input_format=%u, output_format=%u.",
				 session->port[MVX_DIR_INPUT].format,
				 session->port[MVX_DIR_OUTPUT].format);
		ret = -EINVAL;
		goto disable_port;
	}

	/* Verify that client can handle input and output formats. */
	for (i = MVX_DIR_INPUT; i < MVX_DIR_MAX; i++) {
		uint64_t formats;

		session->client_ops->get_formats(session->client_ops,
						 i, &formats);

		if (!mvx_test_bit(session->port[i].format, &formats)) {
			MVX_SESSION_WARN(session,
					 "Client cannot support requested formats. input_format=%u, output_format=%u.",
					 session->port[MVX_DIR_INPUT].format,
					 session->port[MVX_DIR_OUTPUT].format);
			ret = -ENODEV;
			goto disable_port;
		}
	}

	/* Increment session reference count and flag fw bin as pending. */
	mvx_session_get(session);
	session->fw_bin = ERR_PTR(-EINPROGRESS);
	session->client_ops->get_hw_ver(session->client_ops, &hw_ver);

	/* Requesting firmware binary to be loaded. */
	ret = mvx_fw_cache_get(session->cache, session->port[bdir].format,
			       bdir, &session->fw_event, &hw_ver,
			       session->isession.securevideo);
	if (ret != 0) {
		session->port[dir].stream_on = false;
		session->fw_bin = NULL;
		complete(&session->fw_loaded);
		mvx_session_put(session);
		return ret;
	}

	return 0;

disable_port:
	session->port[dir].stream_on = false;

	return ret;
}

int mvx_session_streamoff(struct mvx_session *session, enum mvx_direction dir)
{
	struct mvx_session_port *port = &session->port[dir];
	struct mvx_buffer *buf;
	struct mvx_buffer *tmp;
	int ret = 0;

	MVX_SESSION_INFO(session, "Stream off. dir=%u.", dir);

	port->stream_on = false;

	if (is_fw_loaded(session) != false) {
		/*
		 * Flush the ports if at least one buffer has been queued
		 * since last flush.
		 */
		if (port->flushed == false && port->is_flushing == false) {
			ret = wait_pending(session);
			if (ret != 0)
				goto dequeue_buffers;

			if (!
			    (dir == MVX_DIR_OUTPUT
			     && port->in_port_changing == true)
|| port->received_seq_param == false) {
				ret =
				    fw_state_change(session,
						    MVX_FW_STATE_STOPPED);
				if (ret != 0)
					goto dequeue_buffers;
			}

			ret = fw_flush(session, dir);
			if (ret != 0)
				goto dequeue_buffers;
			ret = wait_pending(session);
			if (ret != 0)
				goto dequeue_buffers;

			if (dir == MVX_DIR_OUTPUT) {
				port->hold_off_buffer_done = false;
				port->in_port_changing = false;
			}
		}
	}
dequeue_buffers:
	/* Return buffers in pending queue. */
	list_for_each_entry_safe(buf, tmp, &port->buffer_queue, head) {
		list_del(&buf->head);
		buf->flags |= MVX_BUFFER_CORRUPT;
		session->event(session, MVX_SESSION_EVENT_BUFFER, buf);
	}

	return 0;
}

int mvx_session_start(struct mvx_session *session)
{
	if (session->port[MVX_DIR_OUTPUT].in_port_changing == true) {
		session->port[MVX_DIR_OUTPUT].in_port_changing = false;
		session->port[MVX_DIR_OUTPUT].hold_off_buffer_done = false;
		return_done_buffers(session, MVX_DIR_OUTPUT);
		return flush_and_qbufs(session, MVX_DIR_OUTPUT);
	}

	return 0;
}

static void mvx_handle_alloc_param(struct mvx_session *session,
				   struct mvx_fw_msg *msg)
{
	struct mvx_session_port *input = &session->port[MVX_DIR_INPUT];
	struct mvx_session_port *output = &session->port[MVX_DIR_OUTPUT];

	output->in_port_changing =
	    msg->alloc_param.width != session->orig_width;
	output->in_port_changing |=
	    msg->alloc_param.height != session->orig_height;
	if (mvx_is_afbc(output->format))
		output->in_port_changing |=
		    msg->alloc_param.afbc_alloc_bytes !=
		    output->afbc_alloc_bytes;
	if (session->dual_afbc_downscaled && output->interlaced == 0)
		output->in_port_changing |=
		    msg->alloc_param.afbc_alloc_bytes_downscaled !=
		    output->afbc_alloc_bytes_downscaled;
	if (output->in_port_changing) {
		output->received_seq_param = false;
		output->pending_source_change_event = true;
	}

	session->orig_width = msg->alloc_param.width;
	session->orig_height = msg->alloc_param.height;
	/* Update input port. */
	input->width = msg->alloc_param.width;
	input->height = msg->alloc_param.height;

	try_format(session, MVX_DIR_INPUT, input->format, &input->width,
		   &input->height, &input->nplanes, input->stride,
		   input->size, &input->interlaced);

	/*
	 * Update output port. Set number of valid planes to 0 to force
	 * stride to be recalculated.
	 */

	output->nplanes = 0;
	if (input->format == MVX_FORMAT_AV1 &&
	    mvx_is_afbc(output->format) != false) {
		output->afbc_alloc_bytes =
		    max(output->afbc_alloc_bytes,
			msg->alloc_param.afbc_alloc_bytes);
		output->afbc_width =
		    max(output->afbc_width, msg->alloc_param.afbc_width);
	} else {
		output->afbc_alloc_bytes = msg->alloc_param.afbc_alloc_bytes;
		output->afbc_width = msg->alloc_param.afbc_width;
	}

	if (session->dual_afbc_downscaled && output->interlaced == 0) {
		output->afbc_alloc_bytes_downscaled =
		    msg->alloc_param.afbc_alloc_bytes_downscaled;
		output->afbc_width_in_superblocks_downscaled =
		    msg->alloc_param.afbc_width_in_superblocks_downscaled;
	} else {
		output->afbc_alloc_bytes_downscaled = 0;
		output->afbc_width_in_superblocks_downscaled = 0;
	}

	output->last_interlaced_from_sps = output->interlaced;
	output->afbc_crop_left = msg->alloc_param.cropx;
	output->afbc_crop_top = msg->alloc_param.cropy;

	try_format(session, MVX_DIR_OUTPUT, output->format,
		   &output->new_width, &output->new_height, &output->nplanes,
		   output->stride, output->size, &output->interlaced);

	session->client_ops->update_load(session->csession);

	MVX_SESSION_INFO(session,
			 "Firmware rsp: Alloc param. width=%u, height=%u, nplanes=%u, size=[%u, %u, %u], stride=[%u, %u, %u], interlaced=%d.",
			 msg->alloc_param.width,
			 msg->alloc_param.height,
			 output->nplanes,
			 output->size[0],
			 output->size[1],
			 output->size[2],
			 output->stride[0],
			 output->stride[1],
			 output->stride[2], output->interlaced);
}

static void mvx_handle_buffer_general(struct mvx_session *session,
				      struct mvx_fw_msg *msg)
{
	struct mvx_buffer *buf = msg->buf;

	session->port[buf->dir].buffer_count--;
	session->event(session, MVX_SESSION_EVENT_BUFFER, buf);
}

static void mvx_handle_buffer(struct mvx_session *session,
			      struct mvx_fw_msg *msg)
{
	uint32_t i;
	uint32_t stride[MVX_BUFFER_NPLANES];
	bool send_buffer_event = true;
	struct mvx_buffer *buf = msg->buf;
	struct mvx_session_port *output = &session->port[MVX_DIR_OUTPUT];

	/*
	 * There is no point to flush or invalidate input buffer
	 * after it was returned from the HW.
	 */
	if (buf->dir == MVX_DIR_OUTPUT && mvx_is_frame(buf->format)) {
		if (!(buf->flags & MVX_BUFFER_FRAME_PRESENT)) {
			int i = 0;

			for (i = 0; i < buf->nplanes; i++) {
				if (output->size[i] > mvx_buffer_size(buf, i) ||
				    session->port[buf->dir].buffer_allocated <
				    session->port[buf->dir].buffer_min)
					buf->flags |=
					    MVX_BUFFER_FRAME_NEED_REALLOC;
			}
		}
	}

	if (buf->dir == MVX_DIR_OUTPUT
	    && session->port[MVX_DIR_INPUT].format == MVX_FORMAT_AV1
	    && (buf->width != output->width || buf->height != output->height)
	    && mvx_is_afbc(session->port[MVX_DIR_OUTPUT].format) == false) {
		uint32_t i;
		uint32_t filled[MVX_BUFFER_NPLANES];
		uint32_t stride[MVX_BUFFER_NPLANES];

		output->nplanes = 0;
		memset(stride, 0, sizeof(stride));
		mvx_buffer_frame_dim(output->format, buf->width, buf->height,
				     &output->nplanes, stride, filled,
				     session->setting_stride);
		for (i = 0; i < buf->nplanes; i++)
			(void)mvx_buffer_filled_set(buf, i, filled[i], 0);

		MVX_SESSION_INFO(session,
				 "Firmware rsp: Buffer. dir=%u, len=[%u, %u, %u], flags=0x%08x, eos=%u",
				 buf->dir,
				 filled[0],
				 filled[1],
				 filled[2],
				 buf->flags,
				 (buf->flags & MVX_BUFFER_EOS) != 0);
	}

	session->port[buf->dir].buffer_count--;

	MVX_SESSION_INFO(session,
			 "Firmware rsp: Buffer. dir=%u, len=[%u, %u, %u], flags=0x%08x, eos=%u",
			 buf->dir,
			 buf->planes[0].filled,
			 buf->planes[1].filled,
			 buf->planes[2].filled,
			 buf->flags, (buf->flags & MVX_BUFFER_EOS) != 0);

	// if (buf->dir == MVX_DIR_OUTPUT && !session->isession.securevideo)
	//     mvx_buffer_synch(buf, DMA_FROM_DEVICE);

	/*
	 * During port changing, driver doesn't dequeue the output frame buffer
	 * to client but add it to pending queue. If client doesn't re-allocate
	 * and intends to re-use the buffer, it should issue V4L2_DEC_CMD_START.
	 * And the buffer will be enqueued to FW in V4L2_DEC_CMD_START handler.
	 *
	 * For empty buffer with flags = 0, it doesn't contain valid information
	 * to client, so just put it to buffer_queue which can be re-queued to
	 * VPU. Also, some clients, like gstreamer v4l2 plug-in, always treat
	 * empty buffer as an eos signal, which is not expected when buffer flags
	 * doesn't have EOS. So most likely it should be re-queued sliently.
	 */
	if (buf->dir == MVX_DIR_OUTPUT && mvx_is_frame(buf->format)) {
		output->frames_since_last_buffer_rejected++;
		if (output->frames_since_last_buffer_rejected >
		    MVX_DECODE_MAX_REJECTED_BUFFER_INTERVAL)
			output->buffer_rejected_flag = false;
		if (buf->planes[0].filled == 0 &&
		    (buf->flags == 0
		     || (buf->flags & MVX_BUFFER_FRAME_NEED_REALLOC) != 0)) {
			list_add_tail(&buf->head, &output->buffer_queue);
			send_buffer_event = false;
		} else if (output->in_port_changing == true) {
			output->nplanes = 0;
			for (i = 0; i < buf->nplanes; i++)
				stride[i] = buf->planes[i].stride;
			// update output buffer size for dump when resolution changed
			mvx_buffer_frame_dim(output->format, buf->width,
					     buf->height, &output->nplanes,
					     stride, output->size,
					     session->setting_stride);
			if (buf->width != output->width
			    || buf->height != output->height) {
				// When resolution changed only in ALLOC_PARAM msg
				// Hold these output buffers in port->buffer_done_queue
				// Then send MVX_SESSION_EVENT_PORT_CHANGED to client
				output->hold_off_buffer_done = true;
				session->event(session,
					       MVX_SESSION_EVENT_PORT_CHANGED,
					       (void *)MVX_DIR_OUTPUT);
			}
			if (output->hold_off_buffer_done) {
				list_add_tail(&buf->head,
					      &output->buffer_done_queue);
				send_buffer_event = false;
			}
		}

		if (buf->width > 0 && buf->height > 0
		    && (buf->flags & MVX_BUFFER_FRAME_PRESENT)) {
			for (i = 0; i < buf->nplanes; i++)
				(void)mvx_buffer_filled_set(buf, i,
							    output->size[i], 0);
		}
	}
	if (send_buffer_event)
		session->event(session, MVX_SESSION_EVENT_BUFFER, buf);

	/*
	 * If buffer is too small, fw will send ALLOC_PARAM msg first, so
	 * flag in_port_changing should be set. And driver should send PORT_CHANGED
	 * event to client to request buffer re-allocation.
	 */
	if (buf->flags & MVX_BUFFER_REJECTED
	    && output->in_port_changing == true) {
		/*
		 * some rejected buffer may be last buffer with previous resolution
		 * or ineffective.
		 * so we judge non-key frmae resolution change by both rejected flag
		 * and variable width or height.
		 */
		if (buf->width != output->width
		    || buf->height != output->height) {
			if (!output->buffer_rejected_flag) {
				output->buffer_rejected_flag = true;
				output->frames_since_last_buffer_rejected = 0;
				output->last_buffer_width = output->width;
				output->last_buffer_height = output->height;
				session->event(session,
					       MVX_SESSION_EVENT_PORT_CHANGED,
					       (void *)MVX_DIR_OUTPUT);
			} else {
				if (output->last_buffer_width == output->width
				    && output->last_buffer_height ==
				    output->height) {
					output->buffer_rejected_flag = true;
					output->frames_since_last_buffer_rejected
					    = 0;
				} else {
					send_event_error(session, -EINVAL);
					return;
				}
			}
		}
	}

	if ((mvx_log_perf.enabled & MVX_LOG_PERF_FPS) && session->ts &&
	    buf->planes[0].filled > 0 && buf->dir == MVX_DIR_OUTPUT) {
		if ((session->is_encoder && (buf->flags & MVX_BUFFER_EOF)) ||
		    !session->is_encoder) {
			struct timespec64 *ts = session->ts + session->ts_index;

			mutex_lock(&session->fps_mutex);
			ktime_get_real_ts64(ts);
			session->ts_index =
			    (session->ts_index + 1) & (MAX_RT_FPS_FRAMES - 1);
			session->frame_count++;
			if (session->frame_count == FPS_SKIP_FRAMES) {
				session->start.tv_sec = ts->tv_sec;
				session->start.tv_nsec = ts->tv_nsec;
			}
			mutex_unlock(&session->fps_mutex);
		}
	}
}

static void mvx_handle_display_size(struct mvx_session *session,
				    struct mvx_fw_msg *msg)
{
	uint32_t stride[MVX_BUFFER_NPLANES];
	uint32_t i;
	struct mvx_session_port *output = &session->port[MVX_DIR_OUTPUT];

	if (session->port[MVX_DIR_INPUT].format < MVX_FORMAT_BITSTREAM_FIRST ||
	    session->port[MVX_DIR_INPUT].format > MVX_FORMAT_BITSTREAM_LAST)
		return;

	if (mvx_is_afbc(output->format) == false) {
		output->nplanes = 0;
		memset(stride, 0, sizeof(stride));
		mvx_buffer_frame_dim(output->format,
				     msg->disp_size.display_width,
				     msg->disp_size.display_height,
				     &output->nplanes, stride,
				     output->display_size,
				     session->setting_stride);
		for (i = 0; i < MVX_BUFFER_NPLANES; i++)
			session->setting_stride[i] =
			    max_t(unsigned int, session->setting_stride[i],
				  stride[i]);

		MVX_SESSION_INFO(session,
				 "Firmware rsp: display size. len=[%u, %u, %u]",
				 output->display_size[0],
				 output->display_size[1],
				 output->display_size[2]);
	} else if (session->port[MVX_DIR_INPUT].format == MVX_FORMAT_AV1) {
		//handle for av1 afbc for now
		int s1 = calc_afbc_size(session, output->format,
					msg->disp_size.display_width,
					msg->disp_size.display_height, true,
					true,
					false,
					false);
		int s2 = calc_afbc_size(session, output->format,
					msg->disp_size.display_width,
					msg->disp_size.display_height, true,
					true,
					true,
					false);
		int s = max_t(unsigned int, s1, s2);

		output->afbc_alloc_bytes =
		    max_t(unsigned int, output->afbc_alloc_bytes, s);
	}
	output->display_size_format = output->format;
}

static void mvx_handle_color_desc(struct mvx_session *session,
				  struct mvx_fw_msg *msg)
{
	MVX_SESSION_INFO(session, "Firmware rsp: Color desc.");
	session->color_desc = msg->color_desc;
	session->event(session, MVX_SESSION_EVENT_COLOR_DESC, NULL);
}

static void mvx_handle_error(struct mvx_session *session,
			     struct mvx_fw_msg *msg)
{
	MVX_SESSION_WARN(session, "Firmware rsp: Error. code=%u, message=%s.",
			 msg->error.error_code, msg->error.message);

	/*
	 * Release the dev session. It will prevent a dead session from
	 * blocking the scheduler.
	 */
	watchdog_stop(session);
	/*
	 * Terminate this session so other session can be switched in.
	 * Client might not response to the error and terminate session properly.
	 */
	if (session->switched_in)
		session->client_ops->terminate(session->csession);
	/*
	 * Firmware could be hang or in unexpected state, and the session should exit now.
	 * Just switch out the session so suspending can move forward.
	 */
	switch_out_rsp(session);
	send_event_error(session, -EINVAL);
}

static void mvx_handle_flush(struct mvx_session *session,
			     struct mvx_fw_msg *msg)
{
	MVX_SESSION_INFO(session, "Firmware rsp: Flushed. dir=%d.",
			 msg->flush.dir);
	session->port[msg->flush.dir].is_flushing = false;
	session->port[msg->flush.dir].flushed = true;
}

static void mvx_handle_idle(struct mvx_session *session, struct mvx_fw_msg *msg)
{
	struct mvx_fw_msg msg_ack = {.code = MVX_FW_CODE_IDLE_ACK };
	int ret;

	MVX_SESSION_INFO(session, "Firmware rsp: Idle.");

	session->idle_count++;

	if (session->idle_count == 2)
		fw_switch_out(session);

	ret = session->fw.ops.put_message(&session->fw, &msg_ack);
	if (ret == 0)
		ret = send_irq(session);
	if (ret != 0)
		send_event_error(session, ret);
}

static void mvx_handle_job(struct mvx_session *session, struct mvx_fw_msg *msg)
{
	MVX_SESSION_INFO(session, "Firmware rsp: Job.");
	session->client_ops->reset_priority(session->csession);
	(void)fw_job(session, session->job_frames);
}

static void mvx_handle_pong(struct mvx_session *session, struct mvx_fw_msg *msg)
{
	MVX_SESSION_INFO(session, "Firmware rsp: Pong.");
}

static void mvx_update_bitdepth(struct mvx_session *session, uint32_t bitdepth)
{
	struct mvx_session_port *p = &session->port[MVX_DIR_OUTPUT];
	struct mvx_session_format_map *map;

	if (!mvx_is_frame(p->format))
		return;

	map = mvx_session_find_format(p->pixelformat);
	if (IS_ERR(map)) {
		MVX_SESSION_ERR(session, "Find pixelformat(0x%x) fail",
				p->pixelformat);
		return;
	}

	p->old_format = p->format;
	p->old_pixelformat = p->pixelformat;

	if (bitdepth == map->bitdepth) {
		return;
	} else if (bitdepth > map->bitdepth) {
		p->format = map->to10_format;
		p->pixelformat = map->to10_pixelformat;
	} else {
		p->format = map->to8_format;
		p->pixelformat = map->to8_pixelformat;
	}

	MVX_SESSION_INFO(session,
			 "Stream's bitdepth(%d) is different from setting(%d), update format from %x to %x",
			 bitdepth, map->bitdepth, map->format, p->format);

	try_format(session, MVX_DIR_OUTPUT, p->format,
		   &p->width, &p->height, &p->nplanes,
		   p->stride, p->size, &p->interlaced);
}

static void mvx_handle_seq_param(struct mvx_session *session,
				 struct mvx_fw_msg *msg)
{
	struct mvx_session_port *p = &session->port[MVX_DIR_OUTPUT];
	int ret;

	MVX_SESSION_INFO(session,
			 "Firmware rsp: Seq param. planar={buffers_min=%u}, afbc={buffers_min=%u}, interlaced=%d.",
			 msg->seq_param.planar.buffers_min,
			 msg->seq_param.afbc.buffers_min, p->interlaced);

	if (mvx_is_afbc(p->format) != false) {
		p->in_port_changing |=
		    msg->seq_param.afbc.buffers_min != p->buffer_min;
		p->buffer_min = msg->seq_param.afbc.buffers_min;
		//for MVX_FW_CODE_ALLOC_PARAM is send before MVX_FW_CODE_SEQ_PARAM msg
		if (p->last_interlaced_from_sps != p->interlaced) {
			//auto detected interlace streams from sps,need update try_format
			if (p->interlaced) {
				p->afbc_alloc_bytes_downscaled = 0;
				p->afbc_width_in_superblocks_downscaled = 0;
				if (session->dual_afbc_downscaled) {
					session->dual_afbc_downscaled = 0;
					MVX_SESSION_INFO(session,
							 "detect interlaced changed  %d->%d from sps for afbc out. disable dual_afbc_downscaled",
							 p->last_interlaced_from_sps,
							 p->interlaced);
				}
			}

			try_format(session, MVX_DIR_OUTPUT, p->format,
				   &p->width, &p->height, &p->nplanes,
				   p->stride, p->size, &p->interlaced);
			MVX_SESSION_INFO(session,
					 "detect interlaced changed  %d->%d from sps for afbc out. call try_format",
					 p->last_interlaced_from_sps,
					 p->interlaced);

			p->last_interlaced_from_sps = p->interlaced;
		}
	} else {
		p->in_port_changing |=
		    msg->seq_param.planar.buffers_min != p->buffer_min;
		p->buffer_min =
		    session->port[MVX_DIR_INPUT].format ==
		    MVX_FORMAT_AV1 ? MVX_DECODE_AV1_PLANNAR_BUFFER_NUM :
		    MVX_DECODE_PLANNAR_BUFFER_NUM;
		p->buffer_min =
		    p->new_width * p->new_height <=
		    8192 * 8192 ? max(msg->seq_param.planar.buffers_min,
				      p->buffer_min) : msg->seq_param.planar.buffers_min;
	}

	p->in_port_changing |=
	    msg->seq_param.bitdepth_luma != session->bitdepth_luma;
	p->in_port_changing |=
	    msg->seq_param.bitdepth_chroma != session->bitdepth_chroma;
	p->in_port_changing |=
	    msg->seq_param.chroma_format != session->coded_chroma_format;
	mvx_update_bitdepth(session, msg->seq_param.bitdepth_chroma);
	session->bitdepth_luma = msg->seq_param.bitdepth_luma;
	session->bitdepth_chroma = msg->seq_param.bitdepth_chroma;
	session->coded_chroma_format = msg->seq_param.chroma_format;

	p->received_seq_param = true;

	// update frame buffer VA as begin when received seq_param msg
	ret = mutex_lock_interruptible(&session->fw.mem_mutex);
	if (ret == 0) {
		enum mvx_fw_region region = MVX_FW_REGION_FRAMEBUF;
		mvx_mmu_va begin;
		mvx_mmu_va end;
		mvx_mmu_va available_length;

		session->fw.ops.get_region(region, &begin, &end);
		session->fw.next_va_region_outbuf = begin;
		available_length =
		    end - begin -
		    msg->seq_param.afbc.buffers_min * p->afbc_alloc_bytes;
		p->rest_frame_map_size =
		    available_length > 0 ? available_length : 0;
		mutex_unlock(&session->fw.mem_mutex);
	}

	if (mvx_is_frame(p->format) && !mvx_is_afbc(p->format)) {
		int i;
		uint32_t frame_size = 0;
		unsigned int dummy_setting_stride[MVX_BUFFER_NPLANES] = {
			0, 0, 0 };
		unsigned int dummy_stride[MVX_BUFFER_NPLANES] = { 0, 0, 0 };
		unsigned int tmp_size[MVX_BUFFER_NPLANES] = { 0, 0, 0 };

		ret =
		    mvx_buffer_frame_dim(p->format, p->width, p->height,
					 &p->nplanes, dummy_stride, tmp_size,
					 dummy_setting_stride);
		for (i = 0; i < p->nplanes; i++)
			frame_size += tmp_size[i];
		// keep buffer_min from firmware to ensure decode will not block
		// when buffer_max got from VA region is too small
		p->buffer_max =
		    clamp_t(uint32_t, p->rest_frame_map_size / frame_size,
			    msg->seq_param.planar.buffers_min, VIDEO_MAX_FRAME);
	}

	if (p->in_port_changing == true)
		session->event(session, MVX_SESSION_EVENT_PORT_CHANGED,
			       (void *)MVX_DIR_OUTPUT);
	else
		queue_work(system_unbound_wq, &session->flush_and_queue_work);

	session->client_ops->update_load(session->csession);
}

static void mvx_handle_set_option(struct mvx_session *session,
				  struct mvx_fw_msg *msg)
{
	MVX_SESSION_INFO(session, "Firmware rsp: Set option.");
}

static void mvx_handle_state_change(struct mvx_session *session,
				    struct mvx_fw_msg *msg)
{
	MVX_SESSION_INFO(session,
			 "Firmware rsp: State changed. old=%s, new=%s.",
			 state_to_string(session->fw_state),
			 state_to_string(msg->state));
	session->fw_state = msg->state;

	session->client_ops->update_load(session->csession);
}

static void mvx_handle_switch_in(struct mvx_session *session,
				 struct mvx_fw_msg *msg)
{
	watchdog_start(session, session_watchdog_timeout, true);
}

static void mvx_handle_switch_out(struct mvx_session *session,
				  struct mvx_fw_msg *msg)
{
	MVX_SESSION_INFO(session, "Firmware rsp: Switched out.");

	watchdog_stop(session);
	switch_out_rsp(session);

	if ((session->fw_state == MVX_FW_STATE_RUNNING
	     && session->idle_count < 2) || session->fw.msg_pending > 0)
		switch_in(session);
}

static void mvx_handle_dump(struct mvx_session *session, struct mvx_fw_msg *msg)
{
	MVX_SESSION_INFO(session, "Firmware rsp: dump.");
}

static void mvx_handle_debug(struct mvx_session *session,
			     struct mvx_fw_msg *msg)
{
	MVX_SESSION_INFO(session, "Firmware rsp: debug.");
}

static void mvx_handle_unknown(struct mvx_session *session,
			       struct mvx_fw_msg *msg)
{
	print_debug(session);
}

struct mvx_fw_msg_handler {
	uint32_t code;
	void (*done)(struct mvx_session *session, struct mvx_fw_msg *msg);
};

static struct mvx_fw_msg_handler handlers[] = {
	{ MVX_FW_CODE_ALLOC_PARAM, mvx_handle_alloc_param },
	{ MVX_FW_CODE_BUFFER_GENERAL, mvx_handle_buffer_general },
	{ MVX_FW_CODE_BUFFER, mvx_handle_buffer },
	{ MVX_FW_CODE_DISPLAY_SIZE, mvx_handle_display_size },
	{ MVX_FW_CODE_COLOR_DESC, mvx_handle_color_desc },
	{ MVX_FW_CODE_ERROR, mvx_handle_error },
	{ MVX_FW_CODE_FLUSH, mvx_handle_flush },
	{ MVX_FW_CODE_IDLE, mvx_handle_idle },
	{ MVX_FW_CODE_JOB, mvx_handle_job },
	{ MVX_FW_CODE_PONG, mvx_handle_pong },
	{ MVX_FW_CODE_SEQ_PARAM, mvx_handle_seq_param },
	{ MVX_FW_CODE_SET_OPTION, mvx_handle_set_option },
	{ MVX_FW_CODE_STATE_CHANGE, mvx_handle_state_change },
	{ MVX_FW_CODE_SWITCH_IN, mvx_handle_switch_in },
	{ MVX_FW_CODE_SWITCH_OUT, mvx_handle_switch_out },
	{ MVX_FW_CODE_DUMP, mvx_handle_dump },
	{ MVX_FW_CODE_DEBUG, mvx_handle_debug },
	{ MVX_FW_CODE_UNKNOWN, mvx_handle_unknown },
};

static void handle_fw_message(struct mvx_session *session,
			      struct mvx_fw_msg *msg)
{
	uint32_t i;
	struct mvx_fw_msg_handler *handler = NULL;

	for (i = 0; i < ARRAY_SIZE(handlers); i++) {
		if (handlers[i].code == msg->code) {
			handler = &handlers[i];
			break;
		}
	}

	if (handler && handler->done)
		handler->done(session, msg);
}

void mvx_session_irq(struct mvx_if_session *isession)
{
	struct mvx_session *session = mvx_if_session_to_session(isession);
	int ret;
	int retry;

	if (is_fw_loaded(session) == false)
		return;

	ret = session->fw.ops.handle_rpc(&session->fw);
	if (ret < 0) {
		send_event_error(session, ret);
		return;
	}
#define GET_MSG_MAX_RETRY 10
	retry = 0;
	do {
		struct mvx_fw_msg msg;
		unsigned int timeout_ms = session->watchdog_count > 0 ?
		    session_watchdog_timeout * session->watchdog_count :
		    session_watchdog_timeout;

		watchdog_update(session, timeout_ms);

		ret = session->fw.ops.get_message(&session->fw, &msg);
		if (ret < 0) {
			send_event_error(session, ret);
			return;
		} else if (ret == EAGAIN) {
			retry++;
			if (retry > GET_MSG_MAX_RETRY) {
				MVX_LOG_PRINT(&mvx_log_if, MVX_WAR_LOG_LEVEL,
					      "Unknown fw message code.");
				ret = -EINVAL;
			} else {
				MVX_LOG_PRINT(&mvx_log_if, MVX_WAR_LOG_LEVEL,
					      "Retry %d", retry);
				continue;
			}
		}

		retry = 0;

		if (ret > 0)
			handle_fw_message(session, &msg);
	} while (ret > 0 && session->error == 0);

	ret = session->fw.ops.handle_fw_ram_print(&session->fw);
	if (ret < 0) {
		send_event_error(session, ret);
		return;
	}

	wake_up(&session->waitq);
}

void mvx_session_port_show(struct mvx_session_port *port, struct seq_file *s)
{
	mvx_seq_printf(s, "mvx_session_port", 0, "%px\n", port);
	mvx_seq_printf(s, "format", 1, "%08x\n", port->format);
	mvx_seq_printf(s, "width", 1, "%u\n", port->width);
	mvx_seq_printf(s, "height", 1, "%u\n", port->height);
	mvx_seq_printf(s, "buffer_min", 1, "%u\n", port->buffer_min);
	mvx_seq_printf(s, "buffer_count", 1, "%u\n", port->buffer_count);
}

int mvx_session_set_securevideo(struct mvx_session *session, bool securevideo)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->isession.securevideo = securevideo;

	return 0;
}

int mvx_session_set_frame_rate(struct mvx_session *session,
			       uint32_t frame_rate_n, uint32_t frame_rate_d)
{
	int ret;

	if (session->error != 0)
		return session->error;

	if (frame_rate_n != 0 && frame_rate_d != 0) {
		session->fps_n = frame_rate_n;
		session->fps_d = frame_rate_d;
	}

	if (is_fw_loaded(session) != false && session->is_encoder) {
		struct mvx_fw_set_option option;

		option.code = MVX_FW_SET_FRAME_RATE;
		option.frame_rate = FRAC_TO_Q16(session->fps_n, session->fps_d);
		ret = fw_set_option(session, &option);
		if (ret != 0)
			return ret;
	}

	return 0;
}

int mvx_session_set_rate_control(struct mvx_session *session, bool enabled)
{
	if (session->error != 0)
		return session->error;

	session->rc_enabled = enabled;

	return 0;
}

int mvx_session_set_bitrate_mode(struct mvx_session *session, int mode)
{
	if (session->error != 0)
		return session->error;

	if (mode == V4L2_MPEG_VIDEO_BITRATE_MODE_VBR)
		session->rc_type = MVX_OPT_RATE_CONTROL_MODE_VARIABLE;
	else if (mode == V4L2_MPEG_VIDEO_BITRATE_MODE_CBR)
		session->rc_type = MVX_OPT_RATE_CONTROL_MODE_CONSTANT;
	else if (mode == V4L2_MPEG_VIDEO_BITRATE_MODE_CVBR)
		session->rc_type = MVX_OPT_RATE_CONTROL_MODE_C_VARIABLE;
	else if (mode == V4L2_MPEG_VIDEO_BITRATE_MODE_STANDARD)
		session->rc_type = MVX_OPT_RATE_CONTROL_MODE_STANDARD;
	else
		return -EINVAL;

	return 0;
}

int mvx_session_set_bitrate(struct mvx_session *session, int bitrate)
{
	int ret;

	if (session->error != 0)
		return session->error;

	session->target_bitrate = bitrate;
	if (session->rc_type == MVX_OPT_RATE_CONTROL_MODE_C_VARIABLE &&
	    session->maximum_bitrate < bitrate)
		session->maximum_bitrate = bitrate;

	if (is_fw_loaded(session) != false && session->rc_enabled != false &&
	    session->port[get_bitstream_port(session)].format !=
	    MVX_FORMAT_JPEG) {
		struct mvx_fw_set_option option;

		option.code = MVX_FW_SET_RATE_CONTROL;
		option.rate_control.target_bitrate = session->target_bitrate;
		option.rate_control.rate_control_mode = session->rc_type;

		ret = fw_set_option(session, &option);
		if (ret != 0)
			return ret;
	}

	return 0;
}

int mvx_session_set_max_bitrate(struct mvx_session *session, int bitrate)
{
	int ret;

	if (session->error != 0)
		return session->error;

	if (session->rc_type != MVX_OPT_RATE_CONTROL_MODE_C_VARIABLE)
		return 0;

	session->maximum_bitrate = bitrate;
	if (session->maximum_bitrate < session->target_bitrate)
		session->maximum_bitrate = session->target_bitrate;

	if (is_fw_loaded(session) != false && session->rc_enabled != false &&
	    session->port[get_bitstream_port(session)].format !=
	    MVX_FORMAT_JPEG) {
		struct mvx_fw_set_option option;

		option.code = MVX_FW_SET_RATE_CONTROL;
		option.rate_control.target_bitrate = session->target_bitrate;
		option.rate_control.rate_control_mode = session->rc_type;
		option.rate_control.maximum_bitrate = session->maximum_bitrate;

		ret = fw_set_option(session, &option);
		if (ret != 0)
			return ret;
	}

	return 0;
}

int mvx_session_set_crop_left(struct mvx_session *session, int32_t left)
{

	if (session->error != 0)
		return session->error;

	session->crop_left = left;

	if (is_fw_loaded(session) != false)
		return -EBUSY;
	return 0;

}

int mvx_session_set_crop_right(struct mvx_session *session, int32_t right)
{

	if (session->error != 0)
		return session->error;

	session->crop_right = right;

	if (is_fw_loaded(session) != false)
		return -EBUSY;
	return 0;

}

int mvx_session_set_crop_top(struct mvx_session *session, int32_t top)
{

	if (session->error != 0)
		return session->error;

	session->crop_top = top;

	if (is_fw_loaded(session) != false)
		return -EBUSY;
	return 0;

}

int mvx_session_set_crop_bottom(struct mvx_session *session, int32_t bottom)
{

	if (session->error != 0)
		return session->error;

	session->crop_bottom = bottom;

	if (is_fw_loaded(session) != false)
		return -EBUSY;
	return 0;

}

int mvx_session_set_rc_bit_i_mode(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	session->rc_bit_i_mode = val;

	if (is_fw_loaded(session) != false)
		return -EBUSY;
	return 0;
}

int mvx_session_set_rc_bit_i_ratio(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	session->rc_bit_i_ratio = val;

	if (is_fw_loaded(session) != false)
		return -EBUSY;
	return 0;
}

int mvx_session_set_inter_med_buf_size(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	session->inter_med_buf_size = val;

	if (is_fw_loaded(session) != false)
		return -EBUSY;
	return 0;
}

int mvx_session_set_svct3_level1_period(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	session->svct3_level1_period = val;

	if (is_fw_loaded(session) != false)
		return -EBUSY;
	return 0;
}

int mvx_session_set_nalu_format(struct mvx_session *session,
				enum mvx_nalu_format fmt)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->nalu_format = fmt;

	return 0;
}

int mvx_session_set_stream_escaping(struct mvx_session *session,
				    enum mvx_tristate status)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->stream_escaping = status;

	return 0;
}

int mvx_session_set_profile(struct mvx_session *session,
			    enum mvx_format format, enum mvx_profile profile)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->profile[format] = profile;

	return 0;
}

int mvx_session_set_level(struct mvx_session *session,
			  enum mvx_format format, enum mvx_level level)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->level[format] = level;

	return 0;
}

int mvx_session_set_tier(struct mvx_session *session,
			 enum mvx_format format, enum mvx_tier tier)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->tier[format] = tier;

	return 0;
}

int mvx_session_set_ignore_stream_headers(struct mvx_session *session,
					  enum mvx_tristate status)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->ignore_stream_headers = status;

	return 0;
}

int mvx_session_set_frame_reordering(struct mvx_session *session,
				     enum mvx_tristate status)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->frame_reordering = status;

	return 0;
}

int mvx_session_set_intbuf_size(struct mvx_session *session, int size)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->intbuf_size = size;

	return 0;
}

int mvx_session_set_b_frames(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->b_frames = val;

	return 0;
}

int mvx_session_set_gop_size(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->gop_size = val;

	return 0;
}

int mvx_session_set_gop_type(struct mvx_session *session,
			     enum mvx_gop_type gop_type)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->gop_type = gop_type;

	return 0;
}

int mvx_session_set_cyclic_intra_refresh_mb(struct mvx_session *session,
					    int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->cyclic_intra_refresh_mb = val;
	return 0;
}

int mvx_session_set_constr_ipred(struct mvx_session *session,
				 enum mvx_tristate status)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->constr_ipred = status;

	return 0;
}

int mvx_session_set_entropy_sync(struct mvx_session *session,
				 enum mvx_tristate status)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->entropy_sync = status;

	return 0;
}

int mvx_session_set_temporal_mvp(struct mvx_session *session,
				 enum mvx_tristate status)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->temporal_mvp = status;

	return 0;
}

int mvx_session_set_tile_rows(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->tile_rows = val;

	return 0;
}

int mvx_session_set_tile_cols(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->tile_cols = val;

	return 0;
}

int mvx_session_set_min_luma_cb_size(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;
	if (val == 8 || val == 16)
		session->min_luma_cb_size = val;
	else
		session->min_luma_cb_size = 0;
	return 0;
}

int mvx_session_set_mb_mask(struct mvx_session *session, int val)
{
	/*
	 * This controls is not implemented.
	 */
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->mb_mask = val;

	return 0;
}

int mvx_session_set_entropy_mode(struct mvx_session *session,
				 enum mvx_entropy_mode mode)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->entropy_mode = mode;

	return 0;
}

int mvx_session_set_multi_slice_mode(struct mvx_session *session,
				     enum mvx_multi_slice_mode mode)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->multi_slice_mode = mode;

	return 0;
}

int mvx_session_set_multi_slice_max_mb(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->multi_slice_max_mb = val;

	return 0;
}

int mvx_session_set_vp9_prob_update(struct mvx_session *session,
				    enum mvx_vp9_prob_update mode)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->vp9_prob_update = mode;

	return 0;
}

int mvx_session_set_mv_h_search_range(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->mv_h_search_range = val;

	return 0;
}

int mvx_session_set_mv_v_search_range(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->mv_v_search_range = val;

	return 0;
}

int mvx_session_set_bitdepth_chroma(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->bitdepth_chroma = val;

	return 0;
}

int mvx_session_set_bitdepth_luma(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->bitdepth_luma = val;

	return 0;
}

int mvx_session_set_force_chroma_format(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->force_chroma_format = val;

	return 0;
}

int mvx_session_set_rgb_to_yuv_mode(struct mvx_session *session,
				    enum mvx_rgb_to_yuv_mode mode)
{
	if (mode == MVX_RGB_TO_YUV_MODE_MAX)
		return 0;
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->rgb_to_yuv = mode;
	session->use_cust_rgb_to_yuv_mode = MVX_CUST_YUV2RGB_MODE_STANDARD;

	return 0;
}

int mvx_session_set_band_limit(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->band_limit = val;

	return 0;
}

int mvx_session_set_cabac_init_idc(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->cabac_init_idc = val;

	return 0;
}

static int mvx_session_get_fmt_qp_range(enum mvx_format fmt,
					int *qp_min, int *qp_max)
{
	switch (fmt) {
	case MVX_FORMAT_H263:
		*qp_min = 0;
		*qp_max = 31;
		break;
	case MVX_FORMAT_H264:
	case MVX_FORMAT_HEVC:
		*qp_min = 0;
		*qp_max = 51;
		break;
	case MVX_FORMAT_VP9:
	case MVX_FORMAT_VP8:
		*qp_min = 0;
		*qp_max = 63;
		break;
	default:
		*qp_min = 0;
		*qp_max = 63;
		break;
	}

	return 0;
}

int mvx_session_set_i_frame_qp(struct mvx_session *session,
			       enum mvx_format fmt, int qp)
{
	int ret;

	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false) {
		enum mvx_direction dir = get_bitstream_port(session);

		fmt = session->port[dir].format;
		ret = fw_set_qp(session, MVX_FW_SET_QP_I, qp);
		if (ret != 0)
			return ret;
	}

	session->qp[fmt].i_frame = qp;
	if (fmt == MVX_FORMAT_VP9)
		session->qp[MVX_FORMAT_VP8].i_frame = qp;

	return 0;
}

int mvx_session_set_p_frame_qp(struct mvx_session *session,
			       enum mvx_format fmt, int qp)
{
	int ret;

	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false) {
		enum mvx_direction dir = get_bitstream_port(session);

		fmt = session->port[dir].format;
		ret = fw_set_qp(session, MVX_FW_SET_QP_P, qp);
		if (ret != 0)
			return ret;
	}

	session->qp[fmt].p_frame = qp;
	if (fmt == MVX_FORMAT_VP9)
		session->qp[MVX_FORMAT_VP8].p_frame = qp;

	return 0;
}

int mvx_session_set_b_frame_qp(struct mvx_session *session,
			       enum mvx_format fmt, int qp)
{
	int ret;

	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false) {
		enum mvx_direction dir = get_bitstream_port(session);

		fmt = session->port[dir].format;
		ret = fw_set_qp(session, MVX_FW_SET_QP_B, qp);
		if (ret != 0)
			return ret;
	}

	session->qp[fmt].b_frame = qp;

	return 0;
}

int mvx_session_set_min_qp(struct mvx_session *session,
			   enum mvx_format fmt, int qp_min)
{
	int ret;

	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false) {
		struct mvx_fw_set_option option;
		enum mvx_direction dir = get_bitstream_port(session);
		int codec = session->port[dir].format;
		int qp_max = session->qp[codec].max;

		if (qp_min > qp_max) {
			int _qp_min, _qp_max;

			mvx_session_get_fmt_qp_range(fmt, &_qp_min, &_qp_max);
			qp_max = _qp_max;
			session->qp[fmt].max = qp_max;
			if (fmt == MVX_FORMAT_VP9)
				session->qp[MVX_FORMAT_VP8].max = qp_max;
		}

		option.code = MVX_FW_SET_QP_RANGE;
		option.qp_range.min = qp_min;
		option.qp_range.max = qp_max;
		ret = fw_set_option(session, &option);
		if (ret != 0)
			return ret;
	}

	session->qp[fmt].min = qp_min;
	if (fmt == MVX_FORMAT_VP9)
		session->qp[MVX_FORMAT_VP8].min = qp_min;

	return 0;
}

int mvx_session_set_max_qp(struct mvx_session *session,
			   enum mvx_format fmt, int qp_max)
{
	int ret;

	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false) {
		struct mvx_fw_set_option option;
		enum mvx_direction dir = get_bitstream_port(session);
		int codec = session->port[dir].format;
		int qp_min = session->qp[codec].min;

		if (qp_min > qp_max) {
			int _qp_min, _qp_max;

			mvx_session_get_fmt_qp_range(fmt, &_qp_min, &_qp_max);
			qp_min = _qp_min;
			session->qp[fmt].min = qp_min;
			if (fmt == MVX_FORMAT_VP9)
				session->qp[MVX_FORMAT_VP8].min = qp_min;
		}

		option.code = MVX_FW_SET_QP_RANGE;
		option.qp_range.min = qp_min;
		option.qp_range.max = qp_max;
		ret = fw_set_option(session, &option);
		if (ret != 0)
			return ret;
	}

	session->qp[fmt].max = qp_max;
	if (fmt == MVX_FORMAT_VP9)
		session->qp[MVX_FORMAT_VP8].max = qp_max;

	return 0;
}

int mvx_session_set_resync_interval(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->resync_interval = val;

	return 0;
}

int mvx_session_set_jpeg_quality(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->jpeg_quality = val;

	return 0;
}

int mvx_session_set_jpeg_quality_luma(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->jpeg_quality_luma = val;

	return 0;
}

int mvx_session_set_jpeg_quality_chroma(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->jpeg_quality_chroma = val;

	return 0;
}

int mvx_session_get_color_desc(struct mvx_session *session,
			       struct mvx_fw_color_desc *color_desc)
{
	*color_desc = session->color_desc;
	return 0;
}

int mvx_session_set_color_desc(struct mvx_session *session,
			       struct mvx_fw_color_desc *color_desc)
{
	int ret = 0;

	if (session->error != 0)
		return session->error;

	session->color_desc = *color_desc;
	if (is_fw_loaded(session) != false) {
		struct mvx_fw_set_option option;

		option.code = MVX_FW_SET_COLOUR_DESC;
		option.colour_desc = *color_desc;
		ret = fw_set_option(session, &option);
		if (ret != 0)
			return ret;
	}
	return 0;
}

int mvx_session_set_osd_config(struct mvx_session *session,
			       struct mvx_osd_config *osd)
{
	int ret = 0;
	int osd_cfg_num = 0;

	if (is_fw_loaded(session) == false || session->port[MVX_DIR_INPUT].is_flushing != false) {
		osd_cfg_num = session->port[MVX_DIR_INPUT].osd_cfg_num;
		if (osd_cfg_num < MVX_ROI_QP_NUMS) {
			MVX_SESSION_INFO(session,
					 "fw is not ready!!!, pending osd num:%d",
					 osd_cfg_num);
			session->port[MVX_DIR_INPUT].osd_cfg_queue[osd_cfg_num] =
			    *osd;
			session->port[MVX_DIR_INPUT].osd_cfg_num++;
		} else {
			MVX_SESSION_ERR(session,
					"fw is not ready for long time, too many osd pending:%d",
					osd_cfg_num);
		}
		return 0;
	}
	ret = queue_osd_config(session, osd);
	return ret;
}

int mvx_session_set_osd_info(struct mvx_session *session,
			     struct mvx_osd_info *osd)
{
	session->osd_info = *osd;
	return 0;
};

int mvx_session_set_roi_regions(struct mvx_session *session,
				struct mvx_roi_config *roi)
{
	int ret = 0;
	int roi_config_num = 0;

	if (is_fw_loaded(session) == false ||
	    session->port[MVX_DIR_INPUT].is_flushing != false) {
		roi_config_num = session->port[MVX_DIR_INPUT].roi_config_num;
		if (roi_config_num < MVX_ROI_QP_NUMS) {
			MVX_SESSION_INFO(session,
					 "fw is not ready!!!, pending roi num:%d",
					 roi_config_num);
			session->port[MVX_DIR_INPUT].roi_config_queue[roi_config_num]
			    = *roi;
			session->port[MVX_DIR_INPUT].roi_config_num++;
		} else {
			MVX_SESSION_ERR(session,
					"fw is not ready for long time, too many roi pending:%d",
					roi_config_num);
		}
		return 0;
	}
	ret = queue_roi_regions(session, roi);
	return 0;
}

int mvx_session_set_qp_epr(struct mvx_session *session,
			   struct mvx_buffer_param_qp *qp)
{
	int ret = 0;
	int qp_num = 0;

	if (is_fw_loaded(session) == false ||
	    session->port[MVX_DIR_INPUT].is_flushing != false) {
		qp_num = session->port[MVX_DIR_INPUT].qp_num;
		if (qp_num < MVX_ROI_QP_NUMS) {
			MVX_SESSION_WARN(session,
					 "fw is not ready!!!, pending qp num:%d",
					 qp_num);
			session->port[MVX_DIR_INPUT].qp_queue[qp_num] = *qp;
			session->port[MVX_DIR_INPUT].qp_num++;
		} else {
			MVX_SESSION_ERR(session,
					"fw is not ready for long time, too many qp pending:%d",
					qp_num);
		}
		return 0;
	}
	ret = queue_qp_epr(session, qp);
	return 0;
}

int mvx_session_set_sei_userdata(struct mvx_session *session,
				 struct mvx_sei_userdata *userdata)
{
	int ret = 0;

	if (session->error != 0)
		return session->error;

	session->sei_userdata = *userdata;
	if (is_fw_loaded(session) != false) {
		struct mvx_fw_set_option option;

		option.code = MVX_FW_SET_SEI_USERDATA;
		option.userdata = *userdata;
		ret = fw_set_option(session, &option);
		if (ret != 0)
			return ret;
	}
	return ret;
}

int mvx_session_set_hrd_buffer_size(struct mvx_session *session, int size)
{
	int ret = 0;

	if (session->error != 0)
		return session->error;

	if (session->port[MVX_DIR_OUTPUT].format == MVX_FORMAT_JPEG) {
		MVX_SESSION_WARN(session,
				 "format %d does not support set HRD Buffer Size",
				 session->port[MVX_DIR_OUTPUT].format);
		return ret;
	}

	session->nHRDBufsize = size;

	if (is_fw_loaded(session) != false) {
		struct mvx_fw_set_option option;

		option.code = MVX_FW_SET_HRD_BUF_SIZE;
		option.nHRDBufsize = size;
		ret = fw_set_option(session, &option);
		if (ret != 0)
			return ret;
	}
	return 0;
}

int mvx_session_set_dsl_frame(struct mvx_session *session,
			      struct mvx_dsl_frame *dsl)
{
	struct mvx_session_port *p = &session->port[MVX_DIR_OUTPUT];

	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->dsl_frame.width = dsl->width;
	session->dsl_frame.height = dsl->height;
	p->width = session->orig_width;
	p->height = session->orig_height;
	try_format(session, MVX_DIR_OUTPUT, p->format,
		   &p->width, &p->height, &p->nplanes,
		   p->stride, p->size, &p->interlaced);

	return 0;
}

int mvx_session_set_dsl_ratio(struct mvx_session *session,
			      struct mvx_dsl_ratio *dsl)
{
	if (session->error != 0)
		return session->error;

	session->dsl_ratio.hor = dsl->hor;
	session->dsl_ratio.ver = dsl->ver;
	return 0;
}

int mvx_session_set_long_term_ref(struct mvx_session *session,
				  struct mvx_long_term_ref *ltr)
{
	if (session->error != 0)
		return session->error;

	session->mvx_ltr.mode = ltr->mode;
	session->mvx_ltr.period = ltr->period;
	if (is_fw_loaded(session) != false)
		return -EBUSY;
	return 0;
}

int mvx_session_set_dsl_mode(struct mvx_session *session, int *mode)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->dsl_pos_mode = *mode;

	return 0;
}

int mvx_session_set_mini_frame_cnt(struct mvx_session *session, int *cnt)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->mini_frame_cnt = *cnt;
	return 0;
}

int mvx_session_set_stats_mode(struct mvx_session *session,
			       struct mvx_enc_stats *stats)
{
	int ret = 0;
	int enc_stats_num = 0;

	if (is_fw_loaded(session) == false ||
	    session->port[MVX_DIR_INPUT].is_flushing != false) {
		enc_stats_num = session->port[MVX_DIR_INPUT].enc_stats_num;
		if (enc_stats_num < MVX_ROI_QP_NUMS) {
			MVX_SESSION_INFO(session,
					 "fw is not ready!!!, pending enc stats num:%d",
					 enc_stats_num);
			session->port[MVX_DIR_INPUT].enc_stats_queue[enc_stats_num] =
			    *stats;
			session->port[MVX_DIR_INPUT].enc_stats_num++;
		} else {
			MVX_SESSION_ERR(session,
					"fw is not ready for long time, too many enc stats pending:%d",
					enc_stats_num);
		}
		return 0;
	}
	ret = queue_enc_stats(session, stats);
	return ret;
}

int mvx_session_set_chr_cfg(struct mvx_session *session,
			    struct mvx_chr_cfg *chr_cfg)
{
	int ret = 0;
	int chr_cfg_num = 0;

	if (is_fw_loaded(session) == false ||
		session->port[MVX_DIR_INPUT].is_flushing != false) {
		chr_cfg_num = session->port[MVX_DIR_INPUT].chr_cfg_num;
		if (chr_cfg_num < MVX_ROI_QP_NUMS) {
			MVX_SESSION_INFO(session,
					 "fw is not ready!!!, pending chr cfg num:%d",
					 chr_cfg_num);
			session->port[MVX_DIR_INPUT].chr_cfg_queue[chr_cfg_num] =
			    *chr_cfg;
			session->port[MVX_DIR_INPUT].chr_cfg_num++;
		} else {
			MVX_SESSION_ERR(session,
					"fw is not ready for long time, too many chr cfg pending:%d",
					chr_cfg_num);
		}
		return 0;
	}
	ret = queue_chr_cfg(session, chr_cfg);
	return ret;
}

int mvx_session_set_huff_table(struct mvx_session *session,
			       struct mvx_huff_table *table)
{
	if (is_fw_loaded(session) != false)
		return -EBUSY;
	memcpy(&session->huff_table, table, sizeof(struct mvx_huff_table));
	return 0;
}

int mvx_session_set_seamless_target(struct mvx_session *session,
				    struct mvx_seamless_target *seamless)
{
	if (is_fw_loaded(session) != false)
		return -EBUSY;
	memcpy(&session->seamless_target, seamless,
	       sizeof(struct mvx_seamless_target));
	return 0;
}

int mvx_session_set_init_qp_i(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->init_qpi = val;

	return 0;
}

int mvx_session_set_init_qp_p(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->init_qpp = val;

	return 0;
}

int mvx_session_set_sao_luma(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->sao_luma = val;

	return 0;
}

int mvx_session_set_sao_chroma(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->sao_chroma = val;

	return 0;
}

int mvx_session_set_delta_I_P(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->qp_delta_i_p = val;

	return 0;
}

int mvx_session_set_ref_rb_eb(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->ref_rb_en = val;

	return 0;
}

int mvx_session_set_rc_clip_top(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->rc_qp_clip_top = val;

	return 0;
}

int mvx_session_set_rc_clip_bot(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->rc_qp_clip_bot = val;

	return 0;
}

int mvx_session_set_qpmap_clip_top(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->qpmap_qp_clip_top = val;

	return 0;
}

int mvx_session_set_qpmap_clip_bot(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->qpmap_qp_clip_bot = val;

	return 0;
}

int mvx_session_set_max_qp_i(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false) {
		int ret;
		struct mvx_fw_set_option option;
		enum mvx_direction dir = get_bitstream_port(session);
		int codec = session->port[dir].format;
		int _qp_min, _qp_max;

		mvx_session_get_fmt_qp_range(codec, &_qp_min, &_qp_max);
		val = min(val, _qp_max);
		if (val < session->min_qp_i)
			session->min_qp_i = _qp_min;

		option.code = MVX_FW_SET_QP_RANGE_I;
		option.qp_range.min = session->min_qp_i;
		option.qp_range.max = val;
		ret = fw_set_option(session, &option);
		if (ret != 0)
			return ret;
	}
	session->max_qp_i = val;

	return 0;

}

int mvx_session_set_min_qp_i(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false) {
		int ret;
		struct mvx_fw_set_option option;
		enum mvx_direction dir = get_bitstream_port(session);
		int codec = session->port[dir].format;
		int _qp_min, _qp_max;

		mvx_session_get_fmt_qp_range(codec, &_qp_min, &_qp_max);
		val = max(val, _qp_min);
		if (val > session->max_qp_i)
			session->max_qp_i = _qp_max;

		option.code = MVX_FW_SET_QP_RANGE_I;
		option.qp_range.min = val;
		option.qp_range.max = session->max_qp_i;
		ret = fw_set_option(session, &option);
		if (ret != 0)
			return ret;
	}

	session->min_qp_i = val;

	return 0;
}

int mvx_session_set_fixedqp(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->fixedqp = val;

	return 0;

}

int mvx_session_set_visible_width(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	session->visible_width = val;

	return 0;

}

int mvx_session_set_visible_height(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	session->visible_height = val;

	return 0;

}

int mvx_session_set_gop_reset_pframes(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false) {
		int ret;
		struct mvx_fw_set_option option;

		option.code = MVX_FW_SET_GOP_PFRAMES;
		option.reset_gop_pframes = val;
		ret = fw_set_option(session, &option);
		if (ret != 0)
			return ret;
	}

	session->reset_gop_pframes = val;
	return 0;
}

int mvx_session_set_ltr_reset_period(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false) {
		int ret;
		struct mvx_fw_set_option option;

		option.code = MVX_FW_SET_LTR_PERIOD;
		option.reset_ltr_period = val;
		ret = fw_set_option(session, &option);
		if (ret != 0)
			return ret;
	}

	session->reset_ltr_period = val;
	return 0;
}

int mvx_session_set_gdr_number(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->gdr_number = val;

	return 0;
}

int mvx_session_set_gdr_period(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->gdr_period = val;

	return 0;
}

int mvx_session_set_mulit_sps_pps(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->mulit_sps_pps = val;

	return 0;
}

int mvx_session_set_enable_visual(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->enable_visual = val;

	return 0;
}

int mvx_session_set_adaptive_intra_block(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->adaptive_intra_block = val;

	return 0;

}

int mvx_session_set_scd_enable(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->scd_enable = val;

	return 0;
}

int mvx_session_set_scd_percent(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->scd_percent = val;

	return 0;
}

int mvx_session_set_scd_threshold(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->scd_threshold = val;

	return 0;
}

int mvx_session_set_aq_ssim_en(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->aq_ssim_en = val;

	return 0;
}

int mvx_session_set_aq_neg_ratio(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->aq_neg_ratio = val;

	return 0;
}

int mvx_session_set_aq_pos_ratio(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->aq_pos_ratio = val;

	return 0;
}

int mvx_session_set_aq_qpdelta_lmt(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->aq_qpdelta_lmt = val;

	return 0;
}

int mvx_session_set_aq_init_frm_avg_svar(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->aq_init_frm_avg_svar = val;

	return 0;
}

int mvx_session_set_color_conversion(struct mvx_session *session,
				     enum mvx_yuv_to_rgb_mode mode)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;
	session->color_conv_mode = mode;
	session->use_cust_color_conv_coef = false;

	return 0;
}

int mvx_session_set_color_conversion_ceof(struct mvx_session *session,
					  struct mvx_color_conv_coef *conv_coef)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	memcpy(&session->color_conv_coef, conv_coef,
	       sizeof(struct mvx_color_conv_coef));
	session->use_cust_color_conv_coef = true;

	return 0;
}

int mvx_session_set_rgb_conv_yuv_coef(struct mvx_session *session, struct mvx_rgb2yuv_color_conv_coef
				      *conv_coef)
{
	if (session->error != 0)
		return session->error;
	if (is_fw_loaded(session) != false)
		return -EBUSY;

	memcpy(&session->rgb2yuv_color_conv_coef, conv_coef,
	       sizeof(struct mvx_rgb2yuv_color_conv_coef));
	session->use_cust_color_conv_coef = true;
	session->use_cust_rgb_to_yuv_mode = MVX_CUST_YUV2RGB_MODE_CUSTOMIZED;

	return 0;
}

int mvx_session_set_forced_uv_value(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->forced_uv_value = val;

	return 0;
}

int mvx_session_set_dsl_interpolation_mode(struct mvx_session *session,
					   int mode)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->dsl_interp_mode = mode;

	return 0;
}

int mvx_session_set_disabled_features(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->disabled_features = val;

	return 0;
}

int mvx_session_set_crop(struct mvx_session *session, struct mvx_crop_cfg *crop)
{
	enum mvx_direction dir;

	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	memcpy(&session->crop, crop, sizeof(struct mvx_crop_cfg));
	for (dir = 0; dir < MVX_DIR_MAX; dir++) {
		struct mvx_session_port *p = &session->port[dir];

		if (mvx_is_frame(p->format)) {
			p->width = session->orig_width;
			p->height = session->orig_height;
			try_format(session, dir, p->format,
				   &p->width, &p->height, &p->nplanes,
				   p->stride, p->size, &p->interlaced);
		} else if (dir == MVX_DIR_OUTPUT) {
			p->width = crop->width;
			p->height = crop->height;
		}
	}

	return 0;
}

int mvx_session_set_dual_afbc_downscaled(struct mvx_session *session, int val)
{
	struct mvx_session_port *p = &session->port[MVX_DIR_OUTPUT];

	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->dual_afbc_downscaled = !!val;
	p->width = session->orig_width;
	p->height = session->orig_height;
	try_format(session, MVX_DIR_OUTPUT, p->format,
		   &p->width, &p->height, &p->nplanes,
		   p->stride, p->size, &p->interlaced);

	return 0;
}

int mvx_session_set_job_frames(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->job_frames = val;

	return 0;
}

int mvx_session_set_force_key_frame(struct mvx_session *session, uint32_t val)
{
	if (session->error != 0)
		return session->error;

	session->force_key_frame = val;

	return 0;
}

int mvx_session_update_input_buffer_min(struct mvx_session *session)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->port[MVX_DIR_INPUT].buffer_min = session->b_frames + 1;
	if (session->port[MVX_DIR_INPUT].width *
	    session->port[MVX_DIR_INPUT].height <= 8192 * 8192)
		session->port[MVX_DIR_INPUT].buffer_min +=
		    MVX_ENCODE_EXTRA_BUFFER_NUM;

	return 0;
}

int mvx_session_update_p_frames(struct mvx_session *session)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	// GOP = P-frames * (B-frames + 1) + 1
	session->p_frames = (session->gop_size - 1) / (session->b_frames + 1);

	return 0;
}

int mvx_session_set_fsf_mode(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->fsf_mode = val;

	return 0;
}

int mvx_session_set_priority(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	if (val >= 0)
		session->priority = val;

	return 0;
}

struct mvx_session_format_map *mvx_session_find_format(uint32_t pixelformat)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mvx_compressed_fmts); i++)
		if (mvx_compressed_fmts[i].pixelformat == pixelformat)
			return &mvx_compressed_fmts[i];

	for (i = 0; i < ARRAY_SIZE(mvx_raw_fmts); i++)
		if (mvx_raw_fmts[i].pixelformat == pixelformat)
			return &mvx_raw_fmts[i];

	return ERR_PTR(-EINVAL);
}

struct mvx_session_format_map *mvx_session_get_compressed_format(struct
								 mvx_session
								 *session)
{
	enum mvx_direction dir = get_bitstream_port(session);

	if (dir < MVX_DIR_MAX)
		return mvx_session_find_format(session->port[dir].pixelformat);
	else
		return ERR_PTR(-EINVAL);
}

uint32_t mvx_get_format_bpp(enum mvx_format format)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mvx_raw_fmts); i++)
		if (mvx_raw_fmts[i].format == format)
			return mvx_raw_fmts[i].bpp;

	return 0;
}

void mvx_session_enum_framesizes(struct mvx_session *session,
				 bool is_encoder, enum mvx_format format,
				 uint32_t *min_width, uint32_t *min_height,
				 uint32_t *max_width, uint32_t *max_height,
				 uint32_t *step_width, uint32_t *step_height)
{
	struct mvx_hw_ver hw_ver;

	session->client_ops->get_hw_ver(session->client_ops, &hw_ver);
	switch (format) {
	case MVX_FORMAT_AVS:
		*max_width = 1920;
		*max_height = 1080;
		break;
	case MVX_FORMAT_AVS2:
	case MVX_FORMAT_H264:
	case MVX_FORMAT_HEVC:
	case MVX_FORMAT_VP9:
	case MVX_FORMAT_AV1:
		if (hw_ver.svn_revision == MVE_SVN_4K) {
			*max_width = 4096;
			*max_height = 4096;
		} else {
			*max_width = 8192;
			*max_height = 8192;
		}
		break;
	case MVX_FORMAT_H263:
	case MVX_FORMAT_MPEG4:
	case MVX_FORMAT_VP8:
		*max_width = 2048;
		*max_height = 2048;
		break;
	case MVX_FORMAT_JPEG:
		if (is_encoder) {
			*max_width = 16384;
			*max_height = 16384;
		} else {
			*max_width = 32768;
			*max_height = 32768;
		}
		break;
	case MVX_FORMAT_MPEG2:
	case MVX_FORMAT_RV:
		*max_width = 4096;
		*max_height = 4096;
		break;
	case MVX_FORMAT_VC1:
		*max_width = 2048;
		*max_height = 4096;
		break;
	default:
		if (is_encoder) {
			*max_width = 16384;
			*max_height = 16384;
		} else {
			*max_width = 8192;
			*max_height = 8192;
		}
		break;
	}
	*min_width = 144;
	*min_height = 144;
	*step_width = 2;
	*step_height = 2;
}

void mvx_session_cancel_work(struct mvx_session *session)
{
	cancel_work_sync(&session->flush_and_queue_work);
	cancel_work_sync(&session->watchdog_work);
	watchdog_stop(session);
}

static void mvx_session_construct_realtime_fps_msg(struct mvx_session *session,
						   int avgfps, int rtfps,
						   uint64_t frame_count,
						   time64_t start_sec,
						   time64_t end_sec)
{
	if (mvx_log_perf.rtfps
	    && mvx_log_perf.rtfps_num < MVX_LOG_FPS_MSG_UNITS) {
		struct rtc_time start, end;
		enum mvx_direction dir =
		    session->is_encoder ? MVX_DIR_OUTPUT : MVX_DIR_INPUT;
		struct mvx_session_port *p = &session->port[dir];
		struct mvx_session_format_map *map =
		    mvx_session_find_format(p->pixelformat);

		rtc_time64_to_tm(start_sec, &start);
		rtc_time64_to_tm(end_sec, &end);

		mutex_lock(&mvx_log_perf.mutex);

		snprintf(mvx_log_perf.rtfps +
			 MVX_LOG_FPS_MSG_UNIT_SIZE * mvx_log_perf.rtfps_num,
			 MVX_LOG_FPS_MSG_UNIT_SIZE,
			 "%02d:%02d:%02d ~ %02d:%02d:%02d [%p] P%d %s %s %d%d %lld frames, current fps %d.%02d, average fps %d.%02d\n",
			 start.tm_hour, start.tm_min, start.tm_sec, end.tm_hour,
			 end.tm_min, end.tm_sec, session, session->priority,
			 map->description,
			 session->is_encoder ? "encoder" : "decoder", p->width,
			 p->height, frame_count, rtfps / 100, rtfps % 100,
			 avgfps / 100, avgfps % 100);

		mvx_log_perf.rtfps_num++;

		mutex_unlock(&mvx_log_perf.mutex);
	}
}

static int mvx_session_calculate_realtime_fps(struct mvx_session *session,
					      int ts_index,
					      uint64_t frame_count)
{
	int i, curr;
	int last = ts_index;
	struct timespec64 *last_ts = session->ts + last;
	struct timespec64 first_ts = { last_ts->tv_sec - 1, last_ts->tv_nsec };
	struct timespec64 delta;
	uint64_t delta_us;

	frame_count = min(frame_count, (uint64_t) MAX_RT_FPS_FRAMES);
	curr = last;
	for (i = 0; i < frame_count - 1; i++) {
		curr--;
		if (curr < 0)
			curr = MAX_RT_FPS_FRAMES - 1;
		if (timespec64_compare(&session->ts[curr], &first_ts) <= 0)
			break;
	}

	delta = timespec64_sub(*last_ts, session->ts[curr]);
	delta_us = timespec64_to_ns(&delta) / 1000;
	frame_count = i + 1;
	return (int)((frame_count * 100 * 1000 * 1000) / delta_us);
}

void mvx_session_update_realtime_fps(struct mvx_session *session)
{
	int ts_index, avgfps, rtfps;
	uint64_t frame_count;

	if (!(mvx_log_perf.enabled & MVX_LOG_PERF_FPS) || !session->ts)
		return;

	mutex_lock(&session->fps_mutex);
	ts_index = session->ts_index;
	frame_count = session->frame_count;
	mutex_unlock(&session->fps_mutex);

	if (frame_count <= FPS_SKIP_FRAMES)
		return;

	ts_index = ts_index == 0 ? MAX_RT_FPS_FRAMES - 1 : ts_index - 1;
	avgfps =
	    mvx_session_calculate_average_fps(session, ts_index, frame_count);
	rtfps =
	    mvx_session_calculate_realtime_fps(session, ts_index, frame_count);
	mvx_session_construct_realtime_fps_msg(session, avgfps, rtfps,
					       frame_count,
					       session->start.tv_sec,
					       session->ts[ts_index].tv_sec);
}

void mvx_session_update_buffer_count(struct mvx_session *session,
				     enum mvx_direction dir)
{
	struct mvx_session_port *port = &session->port[dir];
	int i;
	uint32_t frame_size = 0;

	for (i = 0; i < port->nplanes; ++i)
		frame_size += port->size[i];
	port->buffer_max =
	    clamp_t(uint32_t, port->rest_frame_map_size / frame_size, 1,
		    VIDEO_MAX_FRAME);
	port->buffer_min = min(port->buffer_min, port->buffer_max);
}

int mvx_session_switch_out(struct mvx_session *session)
{
	return fw_switch_out(session);
}

int mvx_session_set_enc_lambda_scale(struct mvx_session *session,
				     struct mvx_lambda_scale *lambda_scale)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	memcpy(&session->lambda_scale, lambda_scale,
	       sizeof(struct mvx_lambda_scale));

	return 0;
}

int mvx_session_set_enc_intra_ipenalty_angular(struct mvx_session *session,
					       int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->intra_ipenalty_angular = val;

	return 0;
}

int mvx_session_set_enc_intra_ipenalty_planar(struct mvx_session *session,
					      int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->intra_ipenalty_planar = val;

	return 0;
}

int mvx_session_set_enc_intra_ipenalty_dc(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->intra_ipenalty_dc = val;

	return 0;
}

int mvx_session_set_enc_inter_ipenalty_angular(struct mvx_session *session,
					       int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->inter_ipenalty_angular = val;

	return 0;
}

int mvx_session_set_enc_inter_ipenalty_planar(struct mvx_session *session,
					      int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->inter_ipenalty_planar = val;

	return 0;
}

int mvx_session_set_enc_inter_ipenalty_dc(struct mvx_session *session, int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->inter_ipenalty_dc = val;

	return 0;
}

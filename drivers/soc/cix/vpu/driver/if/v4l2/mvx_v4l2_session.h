/* SPDX-License-Identifier: GPL-2.0-only */
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

#ifndef _MVX_V4L2_SESSION_H_
#define _MVX_V4L2_SESSION_H_

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/videodev2.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/version.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fh.h>
#include <media/videobuf2-v4l2.h>
#include <linux/mvx-v4l2-controls.h>

#include "mvx_session.h"

/****************************************************************************
 * Types
 ****************************************************************************/

/**
 * Offset used to distinguish between input and output port.
 */
#define DST_QUEUE_OFF_BASE (1 << 30)

/**
 * Maximum of framerate that VPU can support.
 */
#define MAX_FRAME_RATE     256

/**
 * struct mvx_v4l2_port - V4L2 port type.
 *
 * Most of this structure will become redundant when buffer management
 * is transferred to Vb2 framework.
 *
 * @vsession:        Pointer to corresponding session.
 * @port:        Pointer to corresponding mvx port.
 * @dir:        Direction of a port.
 * @type:        V4L2 port type.
 * @pix_mp:        V4L2 multi planar pixel format.
 * @afbc_crop_left: AFBC frame buffer left crop of active region.
 * @afbc_crop_top:  AFBC frame buffer top crop of active region.
 * @dentry:        Debugfs directory entry for the port.
 * @q_set:        Indicates of Vb2 queue was setup.
 * @vb2_queue:        Vb2 queue.
 */
struct mvx_v4l2_port {
	struct mvx_v4l2_session *vsession;
	struct mvx_session_port *port;
	enum mvx_direction dir;
	enum v4l2_buf_type type;
	struct v4l2_pix_format_mplane pix_mp;
	unsigned int afbc_crop_left;
	unsigned int afbc_crop_top;
	struct dentry *dentry;
	bool q_set;
	struct vb2_queue vb2_queue;
};

/**
 * struct mvx_v4l2_session - V4L2 session type.
 * @ext:        Pointer to external interface object.
 * @fh:            V4L2 file handler.
 * @mutex:        Mutex protecting the session object.
 * @session:        Session object.
 * @port:        Array of v4l2 ports.
 * @dentry:        Debugfs directory entry representing a session.
 * @v4l2_ctrl:        v4l2 controls handler.
 */
struct mvx_v4l2_session {
	struct mvx_ext_if *ext;
	struct v4l2_fh fh;
	struct mutex mutex;
	struct mvx_session session;
	struct mvx_v4l2_port port[MVX_DIR_MAX];
	struct dentry *dentry;
	struct v4l2_ctrl_handler v4l2_ctrl;
	struct completion cmp;
	struct mvx_v4l2_buffer *frame_bits_buf;
	bool first_input_processed;
};

/****************************************************************************
 * Exported functions
 ****************************************************************************/

/**
 * mvx_v4l2_session_construct() - Construct v4l2 session object.
 * @vsession:    Pointer to a session object.
 * @ctx:    Pointer to an external interface object.
 *
 * Return: 0 on success, else error code.
 */
int mvx_v4l2_session_construct(struct mvx_v4l2_session *vsession,
			       struct mvx_ext_if *ctx);

/**
 * v4l2_fh_to_session() - Cast v4l2 file handler to mvx_v4l2_session.
 * @fh:        v4l2 file handler.
 *
 * Return: Pointer to a corresponding mvx_v4l2_session object.
 */
struct mvx_v4l2_session *v4l2_fh_to_session(struct v4l2_fh *fh);

/**
 * file_to_session() - Cast file object to mvx_v4l2_session.
 * @file:    Pointer to a file object.
 *
 * Return: Pointer to a corresponding mvx_v4l2_session object.
 */
struct mvx_v4l2_session *file_to_session(struct file *file);

/**
 * mvx_v4l2_session_set_sei_userdata() - Set SEI userdata.
 * @vsession:    Pointer to v4l2 session.
 * @sei_userdata:    SEI userdata.
 *
 * Return: 0 on success, else error code.
 */

int mvx_v4l2_session_set_sei_userdata(struct mvx_v4l2_session *vsession,
				      struct v4l2_sei_user_data *sei_userdata);

/**
 * mvx_v4l2_session_set_roi_regions() - Set Roi Regions.
 * @vsession:    Pointer to v4l2 session.
 * @roi:    ROI regions.
 *
 * Return: 0 on success, else error code.
 */
int mvx_v4l2_session_set_roi_regions(struct mvx_v4l2_session *vsession,
				     struct v4l2_mvx_roi_regions *roi);

/**
 * mvx_v4l2_session_set_qp_epr() - Set qp.
 * @vsession:    Pointer to v4l2 session.
 * @qp:    qp value.
 *
 * Return: 0 on success, else error code.
 */

int mvx_v4l2_session_set_qp_epr(struct mvx_v4l2_session *vsession,
				struct v4l2_buffer_param_qp *qp);

/**
 * mvx_v4l2_session_set_dsl_ratio() - Set DownScale ratio.
 * @vsession:    Pointer to v4l2 session.
 * @dsl:    DownScale ratio.
 *
 * Return: 0 on success, else error code.
 */

int mvx_v4l2_session_set_dsl_ratio(struct mvx_v4l2_session *vsession,
				   struct v4l2_mvx_dsl_ratio *dsl);

/**
 * mvx_v4l2_session_set_long_term_ref() - Set long term ref.
 * @vsession:    Pointer to v4l2 session.
 * @ltr:    long term ref.
 *
 * Return: 0 on success, else error code.
 */

int mvx_v4l2_session_set_long_term_ref(struct mvx_v4l2_session *vsession,
				       struct v4l2_mvx_long_term_ref *ltr);

/**
 * mvx_v4l2_session_set_dsl_mode() - Set DownScale mode.
 * @vsession:    Pointer to v4l2 session.
 * @mode:    DownScale mode, oly enable on high precision mode.
 *
 * Return: 0 on success, else error code.
 */

int mvx_v4l2_session_set_dsl_mode(struct mvx_v4l2_session *vsession, int *mode);

/**
 * mvx_v4l2_session_set_mini_frame_cnt() - Set DownScale mode.
 * @vsession:    Pointer to v4l2 session.
 * @cnt:    Mini Frame buffer cnt.
 *
 * Return: 0 on success, else error code.
 */

int mvx_v4l2_session_set_mini_frame_cnt(struct mvx_v4l2_session *vsession,
					int *cnt);

/**
 * mvx_v4l2_session_set_stats_mode() - Set Stats mode.
 * @vsession:    Pointer to v4l2 session.
 * @mode:    stats mode.
 *
 * Return: 0 on success, else error code.
 */

int mvx_v4l2_session_set_stats_mode(struct mvx_v4l2_session *vsession,
				    struct v4l2_buffer_param_enc_stats *stats);
int mvx_v4l2_session_set_chr_cfg(struct mvx_v4l2_session *vsession,
				 struct v4l2_mvx_chr_config *chr);
int mvx_v4l2_session_set_huff_table(struct mvx_v4l2_session *vsession,
				    struct v4l2_mvx_huff_table *table);
int mvx_v4l2_session_set_seamless_target(struct mvx_v4l2_session *vsession, struct v4l2_mvx_seamless_target
					 *seamless);
int mvx_v4l2_session_set_color_conv_coef(struct mvx_v4l2_session *vsession,
					 struct v4l2_mvx_color_conv_coef *coef);
int mvx_v4l2_session_set_rgb_conv_yuv_coef(struct mvx_v4l2_session *vsession, struct
					   v4l2_mvx_rgb2yuv_color_conv_coef
					   *coef);
int mvx_v4l2_session_set_osd_config(struct mvx_v4l2_session *vsession,
				    struct v4l2_osd_config *osd_cfg);
int mvx_v4l2_session_set_osd_info(struct mvx_v4l2_session *vsession,
				  struct v4l2_osd_info *info,
				  enum mvx_format *osd_fmt);
void mvx_v4l2_session_try_color_info(struct mvx_v4l2_session *vsession,
				     struct v4l2_pix_format_mplane *pix);
int mvx_v4l2_session_set_color_info(struct mvx_v4l2_session *vsession,
				    struct v4l2_pix_format_mplane *pix);
int mvx_v4l2_session_set_enc_lambda_scale(struct mvx_v4l2_session *vsession, struct v4l2_mvx_lambda_scale
					  *lambda_scale);
int mvx_v4l2_session_get_hdr10_cll_info(struct mvx_v4l2_session *vsession,
					struct v4l2_ctrl_hdr10_cll_info *hdr);
int mvx_v4l2_session_set_hdr10_cll_info(struct mvx_v4l2_session *vsession,
					struct v4l2_ctrl_hdr10_cll_info *hdr);
int mvx_v4l2_session_get_hdr10_mastering(struct mvx_v4l2_session *vsession, struct
					 v4l2_ctrl_hdr10_mastering_display
					 *hdr);
int mvx_v4l2_session_set_hdr10_mastering(struct mvx_v4l2_session *vsession, struct
					 v4l2_ctrl_hdr10_mastering_display
					 *hdr);

#endif /* _MVX_V4L2_SESSION_H_ */

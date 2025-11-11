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
#include <linux/sched.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-dma-sg.h>
#include <linux/mvx-v4l2-controls.h>
#include "mvx_ext_if.h"
#include "mvx_seq.h"
#include "mvx_v4l2_buffer.h"
#include "mvx_v4l2_session.h"
#include "mvx_log_group.h"

#define V4L2_MVX_COLORIMETRY_UNSUPPORTED (-1)

static const unsigned int range_map[] = {
	V4L2_QUANTIZATION_DEFAULT,
	V4L2_QUANTIZATION_LIM_RANGE,
	V4L2_QUANTIZATION_FULL_RANGE,
};

static const unsigned int primaries_map[] = {
	V4L2_MVX_COLORIMETRY_UNSUPPORTED,
	V4L2_COLORSPACE_REC709,	/*Rec. ITU-R BT.709-6 */
	V4L2_COLORSPACE_DEFAULT,
	V4L2_MVX_COLORIMETRY_UNSUPPORTED,
	V4L2_COLORSPACE_470_SYSTEM_M,	/*Rec. ITU-R BT.470-6 System M */
	V4L2_COLORSPACE_470_SYSTEM_BG,	/*Rec. ITU-R BT.470-6 System B, G */
	V4L2_COLORSPACE_SMPTE170M,	/*SMPTE170M */
	V4L2_COLORSPACE_SMPTE240M,	/*SMPTE240M */
	V4L2_COLORSPACE_GENERIC_FILM,	/*Generic film */
	V4L2_COLORSPACE_BT2020,	/*Rec. ITU-R BT.2020-2 */
	V4L2_COLORSPACE_ST428,	/*SMPTE ST 428-1 (2006) */
	V4L2_COLORSPACE_DCI_P3,	/*SMPTE RP 431-2 (2011), SMPTE ST 2113 (2019) "P3DCI" */
};

static const unsigned int xfer_map[] = {
	V4L2_MVX_COLORIMETRY_UNSUPPORTED,
	V4L2_XFER_FUNC_709,	/*Rec. ITU-R BT.709-6 */
	V4L2_XFER_FUNC_DEFAULT,
	V4L2_MVX_COLORIMETRY_UNSUPPORTED,
	V4L2_XFER_FUNC_GAMMA22,	/*Assumed display gamma 2.2. Rec. ITU-R BT.470-6 System M */
	V4L2_XFER_FUNC_GAMMA28,	/*Assumed display gamma 2.8. Rec. Rec. ITU-R BT.470-6 System B, G */
	V4L2_XFER_FUNC_709,	/*SMPTE170M */
	V4L2_XFER_FUNC_SMPTE240M,	/*SMPTE240M */
	V4L2_XFER_FUNC_NONE,	/*Linear transfer characteristics */
	V4L2_MVX_COLORIMETRY_UNSUPPORTED,
	V4L2_MVX_COLORIMETRY_UNSUPPORTED,
	V4L2_MVX_COLORIMETRY_UNSUPPORTED,	/*IEC 61966-2-4 */
	V4L2_XFER_FUNC_BT1361,	/*Rec. ITU-R BT.1361-0 extended colour gamut */
	V4L2_XFER_FUNC_SRGB,	/*IEC 61966-2-1 sRGB or sYCC */
	V4L2_XFER_FUNC_BT2020_10,	/*Rec. ITU-R BT.2020-2 (10 bit system) */
	V4L2_XFER_FUNC_BT2020_12,	/*Rec. ITU-R BT.2020-2 (12 bit system) */
	V4L2_XFER_FUNC_SMPTE2084,	/*SMPTE ST 2084 */
	V4L2_XFER_FUNC_ST428,	/*SMPTE ST 428-1 */
	V4L2_XFER_FUNC_HLG,	/*STD-B67 and Rec. ITU-R BT.2100-2 hybrid log-gamma (HLG) system */
};

static const unsigned int matrix_map[] = {
	V4L2_MVX_COLORIMETRY_UNSUPPORTED,
	V4L2_YCBCR_ENC_709,	/*Rec. ITU-R BT.709-6 */
	V4L2_YCBCR_ENC_DEFAULT,
	V4L2_MVX_COLORIMETRY_UNSUPPORTED,
	V4L2_YCBCR_ENC_BT470_6M,	/*Title 47 Code of Federal Regulations */
	V4L2_YCBCR_ENC_601,	/*Rec. ITU-R BT.601-7 625 */
	V4L2_YCBCR_ENC_601,	/*Rec. ITU-R BT.601-7 525 */
	V4L2_YCBCR_ENC_SMPTE240M,	/*SMPTE240M */
	V4L2_MVX_COLORIMETRY_UNSUPPORTED,
	V4L2_YCBCR_ENC_BT2020,	/*Rec. ITU-R BT.2020-2 */
	V4L2_YCBCR_ENC_BT2020_CONST_LUM	/*Rec. ITU-R BT.2020-2 constant */
};

/****************************************************************************
 * Exported and static functions
 ****************************************************************************/

static void set_format(struct v4l2_pix_format_mplane *pix_mp,
		       unsigned int pixelformat,
		       unsigned int width,
		       unsigned int height,
		       unsigned int num_planes,
		       unsigned int *sizeimage, unsigned int *bytesperline)
{
	int i;

	pix_mp->pixelformat = pixelformat;
	pix_mp->width = width;
	pix_mp->height = height;
	pix_mp->num_planes = num_planes;

	for (i = 0; i < num_planes; ++i) {
		pix_mp->plane_fmt[i].sizeimage = sizeimage[i];
		pix_mp->plane_fmt[i].bytesperline = bytesperline[i];
	}
}

static void set_video_signal_type(struct v4l2_pix_format_mplane *pix_mp,
				  uint8_t colour_primaries,
				  uint8_t transfer_characteristics,
				  uint8_t matrix_coeff, uint8_t range)
{
	pix_mp->colorspace = colour_primaries < ARRAY_SIZE(primaries_map) ?
	    primaries_map[colour_primaries] : V4L2_COLORSPACE_DEFAULT;
	pix_mp->xfer_func = transfer_characteristics < ARRAY_SIZE(xfer_map) ?
	    xfer_map[transfer_characteristics] : V4L2_XFER_FUNC_DEFAULT;
	pix_mp->ycbcr_enc = matrix_coeff < ARRAY_SIZE(matrix_map) ?
	    matrix_map[matrix_coeff] : V4L2_YCBCR_ENC_DEFAULT;
	pix_mp->quantization = range < ARRAY_SIZE(range_map) ?
	    range_map[range] : V4L2_QUANTIZATION_DEFAULT;

	// There is two colorspaces using BT709 primaries, use the range to differentiate.
	if (pix_mp->colorspace == V4L2_COLORSPACE_REC709 &&
	    pix_mp->quantization == V4L2_QUANTIZATION_FULL_RANGE)
		pix_mp->colorspace = V4L2_COLORSPACE_SRGB;
}

static void set_default_video_signal_type(unsigned int colorspace,
					  unsigned int *xfer_func,
					  unsigned int *ycbcr_enc,
					  unsigned int *range)
{
	switch (colorspace) {
	case V4L2_COLORSPACE_SMPTE170M:
	case V4L2_COLORSPACE_470_SYSTEM_M:
	case V4L2_COLORSPACE_470_SYSTEM_BG:
		*ycbcr_enc = V4L2_YCBCR_ENC_601;
		*xfer_func = V4L2_XFER_FUNC_709;
		break;
	case V4L2_COLORSPACE_REC709:
		*ycbcr_enc = V4L2_YCBCR_ENC_709;
		*xfer_func = V4L2_XFER_FUNC_709;
		break;
	case V4L2_COLORSPACE_SRGB:
	case V4L2_COLORSPACE_JPEG:
		*ycbcr_enc = V4L2_YCBCR_ENC_601;
		*xfer_func = V4L2_XFER_FUNC_SRGB;
		break;
	case V4L2_COLORSPACE_OPRGB:
		*ycbcr_enc = V4L2_YCBCR_ENC_601;
		*xfer_func = V4L2_XFER_FUNC_OPRGB;
		break;
	case V4L2_COLORSPACE_BT2020:
		*ycbcr_enc = V4L2_YCBCR_ENC_BT2020;
		*xfer_func = V4L2_XFER_FUNC_709;
		break;
	case V4L2_COLORSPACE_SMPTE240M:
		*ycbcr_enc = V4L2_YCBCR_ENC_SMPTE240M;
		*xfer_func = V4L2_XFER_FUNC_SMPTE240M;
		break;
	case V4L2_COLORSPACE_RAW:
	default:
		/* Explicitly unknown */
		*ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
		*xfer_func = V4L2_XFER_FUNC_DEFAULT;
	}

	*range = V4L2_QUANTIZATION_DEFAULT;
}

static int get_u32_array_index(const unsigned int *array,
			       unsigned int size, unsigned int data)
{
	int i = 0;

	for (i = 0; i < size; i++) {
		if (array[i] == data)
			return i;
	}

	return -1;
}

static void v4l2_port_show(struct mvx_v4l2_port *port, struct seq_file *s)
{
	mvx_seq_printf(s, "mvx_v4l2_port", 0, "%px\n", port);
	mvx_seq_printf(s, "pixelformat", 1, "0x%x\n", port->pix_mp.pixelformat);
	mvx_seq_printf(s, "vb2_queue", 1, "\n");
	mvx_seq_printf(s, "memory", 2, "%u\n", port->vb2_queue.memory);
	mvx_seq_printf(s, "min_buffers_needed", 2, "%u\n",
		       port->vb2_queue.min_buffers_needed);
	mvx_seq_printf(s, "num_buffers", 2, "%u\n",
		       port->vb2_queue.num_buffers);
	mvx_seq_printf(s, "queued_count", 2, "%u\n",
		       port->vb2_queue.queued_count);
	mvx_seq_printf(s, "streaming", 2, "%u\n", port->vb2_queue.streaming);
	mvx_seq_printf(s, "error", 2, "%u\n", port->vb2_queue.error);
	mvx_seq_printf(s, "last_buffer_dequeued", 2, "%u\n",
		       port->vb2_queue.last_buffer_dequeued);
}

static int port_stat_show(struct seq_file *s, void *v)
{
	struct mvx_v4l2_port *vport = s->private;
	struct mvx_session_port *sport = vport->port;

	mvx_session_port_show(sport, s);
	seq_puts(s, "\n");
	v4l2_port_show(vport, s);

	return 0;
}

static int port_stat_open(struct inode *inode, struct file *file)
{
	return single_open(file, port_stat_show, inode->i_private);
}

static const struct file_operations port_stat_fops = {
	.open = port_stat_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release
};

static int port_debugfs_init(struct device *dev,
			     unsigned int i,
			     struct mvx_v4l2_port *vport,
			     struct mvx_session_port *sport,
			     struct dentry *parent)
{
	char name[20];
	struct dentry *dentry;

	scnprintf(name, sizeof(name), "port%u", i);
	vport->dentry = debugfs_create_dir(name, parent);
	if (IS_ERR_OR_NULL(vport->dentry))
		return -ENOMEM;

	dentry = debugfs_create_file("stat", 0400, vport->dentry, vport,
				     &port_stat_fops);
	if (IS_ERR_OR_NULL(dentry))
		return -ENOMEM;

	return 0;
}

static int session_debugfs_init(struct mvx_v4l2_session *session,
				struct dentry *parent)
{
	int ret;
	char name[20];
	int i;

	scnprintf(name, sizeof(name), "%p", &session->session);
	session->dentry = debugfs_create_dir(name, parent);
	if (IS_ERR_OR_NULL(session->dentry))
		return -ENOMEM;

	for (i = 0; i < MVX_DIR_MAX; i++) {
		struct mvx_v4l2_port *vport = &session->port[i];
		struct mvx_session_port *mport = &session->session.port[i];

		ret = port_debugfs_init(session->ext->dev, i, vport, mport,
					session->dentry);
		if (ret != 0)
			goto remove_dentry;
	}

	return 0;

remove_dentry:
	debugfs_remove_recursive(session->dentry);
	return ret;
}

static struct mvx_v4l2_session *mvx_session_to_v4l2_session(struct mvx_session
							    *session)
{
	return container_of(session, struct mvx_v4l2_session, session);
}

static void free_session(struct mvx_session *session)
{
	struct mvx_v4l2_session *s = mvx_session_to_v4l2_session(session);

	MVX_SESSION_INFO(session, "v4l2: Destroy session.");

	mvx_session_destruct(session);

	if (IS_ENABLED(CONFIG_DEBUG_FS))
		debugfs_remove_recursive(s->dentry);
	if (mutex_is_locked(&s->mutex))
		mutex_unlock(&s->mutex);
	complete(&s->cmp);
}

static bool bitstream_need_merge(struct mvx_v4l2_session *vsession,
				 struct mvx_v4l2_buffer *vbuf)
{
	struct mvx_buffer *buf = &vbuf->buf;

	if (buf->dir == MVX_DIR_OUTPUT && mvx_is_bitstream(buf->format)) {
		if (!(buf->flags & MVX_BUFFER_EOF)
		    || (vsession->frame_bits_buf != NULL)) {
			if (vsession->frame_bits_buf != NULL) {
				struct vb2_buffer *vb_dst =
				    &vsession->frame_bits_buf->vb2_v4l2_buffer.vb2_buf;
				struct vb2_buffer *vb_src =
				    &vbuf->vb2_v4l2_buffer.vb2_buf;
				unsigned int size =
				    vb_src->planes[0].bytesused -
				    vb_src->planes[0].data_offset;
				unsigned int space =
				    vb_dst->planes[0].length -
				    vb_dst->planes[0].bytesused;
				if (vb2_plane_vaddr(vb_dst, 0) == NULL
				    || vb2_plane_vaddr(vb_src, 0) == NULL) {
					MVX_SESSION_WARN(&vsession->session,
							 "Unable to obtain kernel virtual address of dst/src plane\n");
					return false;
				}
				if (space < size) {
					MVX_SESSION_WARN(&vsession->session,
							 "Remaining space in bitstream buffer (%d) is not enough to store extra %d bytes\n",
							 space, size);
					return false;
				}
			}

			if (buf->planes[0].filled > 0) {
				if (buf->planes[0].filled >=
				    vbuf->vb2_v4l2_buffer.vb2_buf.planes[0].length) {
					MVX_SESSION_WARN(&vsession->session,
							 "No remaining space in bitstream buffer\n");
					return false;
				}
				return true;
			}
		}
	}

	return false;
}

static struct vb2_buffer *merge_bitstream(struct mvx_v4l2_session *vsession,
					  struct mvx_v4l2_buffer *vbuf)
{
	struct vb2_buffer *vb = NULL;

	mvx_buffer_synch(&vbuf->buf, DMA_FROM_DEVICE);
	if (vsession->frame_bits_buf == NULL) {
		vsession->frame_bits_buf = vbuf;
		MVX_SESSION_INFO(&vsession->session,
				 "Partial bitstream offset %d, used %d\n",
				 vbuf->vb2_v4l2_buffer.vb2_buf.planes[0].data_offset,
				 vbuf->vb2_v4l2_buffer.vb2_buf.planes[0].bytesused);
	} else {
		/* merge bitstream buffers */
		struct vb2_buffer *vb_dst =
		    &vsession->frame_bits_buf->vb2_v4l2_buffer.vb2_buf;
		struct vb2_buffer *vb_src = &vbuf->vb2_v4l2_buffer.vb2_buf;
		void *dst =
		    vb2_plane_vaddr(vb_dst, 0) + vb_dst->planes[0].bytesused;
		void *src =
		    vb2_plane_vaddr(vb_src, 0) + vb_src->planes[0].data_offset;
		unsigned int size =
		    vb_src->planes[0].bytesused - vb_src->planes[0].data_offset;
		if (size > 0) {
			memcpy(dst, src, size);
			vb_dst->planes[0].bytesused += size;
			vsession->frame_bits_buf->buf.planes[0].filled += size;
			MVX_SESSION_INFO(&vsession->session,
					 "Merged %d bytes of bitstream, total %d bytes\n",
					 size, vb_dst->planes[0].bytesused);
		}

		mvx_session_qbuf(&vsession->session, MVX_DIR_OUTPUT,
				 &vbuf->buf);

		if (vbuf->buf.flags & MVX_BUFFER_EOF) {
			vsession->frame_bits_buf->vb2_v4l2_buffer.flags =
			    vbuf->vb2_v4l2_buffer.flags;
			vsession->frame_bits_buf = NULL;
			vb = vb_dst;
		}
	}

	return vb;
}

static void handle_event(struct mvx_session *session,
			 enum mvx_session_event event, void *arg)
{
	struct mvx_v4l2_session *vsession =
	    mvx_session_to_v4l2_session(session);

	MVX_SESSION_INFO(&vsession->session,
			 "Event. event=%d, arg=%p.", event, arg);

	switch (event) {
	case MVX_SESSION_EVENT_BUFFER:{
			struct mvx_v4l2_buffer *vbuf =
			    mvx_buffer_to_v4l2_buffer(arg);
			struct vb2_buffer *vb = &vbuf->vb2_v4l2_buffer.vb2_buf;

			/*
			 * When streaming is stopped we don't always receive all
			 * buffers from FW back. So we just return them all to Vb2.
			 * If the FW later returns a buffer to us, we could silently
			 * skip it.
			 */
			if (vb->state != VB2_BUF_STATE_DEQUEUED) {
				enum vb2_buffer_state state =
				    mvx_v4l2_buffer_update(vbuf);

				if (vbuf->buf.dir == MVX_DIR_OUTPUT
				    && mvx_is_bitstream(vbuf->buf.format)) {
					if (bitstream_need_merge
					    (vsession, vbuf))
						vb = merge_bitstream(vsession,
								     vbuf);
					else if (vsession->frame_bits_buf !=
						 NULL) {
						vb2_buffer_done
						    (&vsession->frame_bits_buf->vb2_v4l2_buffer.vb2_buf, state);
						vsession->frame_bits_buf = NULL;
					}
				}

				if (vb != NULL)
					vb2_buffer_done(vb, state);
			}

			break;
		}
	case MVX_SESSION_EVENT_PORT_CHANGED:{
			enum mvx_direction dir = (enum mvx_direction)arg;
			struct mvx_v4l2_port *vport = &vsession->port[dir];
			struct mvx_session_port *port = &session->port[dir];
			const struct v4l2_event event = {
				.type = V4L2_EVENT_SOURCE_CHANGE,
				.u.src_change.changes =
				    V4L2_EVENT_SRC_CH_RESOLUTION
			};
			struct v4l2_pix_format_mplane *p = &vport->pix_mp;
			unsigned int field = mvx_is_afbc(port->format) ?
			    V4L2_FIELD_SEQ_TB : V4L2_FIELD_INTERLACED;
			unsigned int width = port->width;
			unsigned int height = port->height;

			if (dir == MVX_DIR_OUTPUT) {
				port->width = port->new_width;
				port->height = port->new_height;
				width = port->width;
				height = port->height;
				port->pending_source_change_event = false;
			}

			p->field = port->interlaced ? field : V4L2_FIELD_NONE;
			if (dir == MVX_DIR_OUTPUT && mvx_is_afbc(port->format)) {
				if (session->dual_afbc_downscaled)
					width =
					    session->port
					    [dir].afbc_width_in_superblocks_downscaled
					    << AFBC_SUPERBLOCK_SHIFT;
				else
					width =
					    session->port[dir].afbc_width <<
					    AFBC_SUPERBLOCK_SHIFT;
				height +=
				    session->port[dir].afbc_crop_top >>
				    session->dual_afbc_downscaled;
			}

			set_format(&vport->pix_mp, port->pixelformat, width,
				   height, port->nplanes, port->size,
				   port->stride);
			vport->afbc_crop_left = port->afbc_crop_left;
			vport->afbc_crop_top = port->afbc_crop_top;
			v4l2_event_queue_fh(&vsession->fh, &event);
			break;
		}
	case MVX_SESSION_EVENT_COLOR_DESC:{
			struct mvx_fw_color_desc *cd = &session->color_desc;

			set_video_signal_type(&vsession->port[MVX_DIR_OUTPUT].pix_mp,
					      cd->colour_primaries,
					      cd->transfer_characteristics,
					      cd->matrix_coeff, cd->range);
			vsession->port[MVX_DIR_INPUT].pix_mp.colorspace =
			    vsession->port[MVX_DIR_OUTPUT].pix_mp.colorspace;
			vsession->port[MVX_DIR_INPUT].pix_mp.xfer_func =
			    vsession->port[MVX_DIR_OUTPUT].pix_mp.xfer_func;
			vsession->port[MVX_DIR_INPUT].pix_mp.ycbcr_enc =
			    vsession->port[MVX_DIR_OUTPUT].pix_mp.ycbcr_enc;
			vsession->port[MVX_DIR_INPUT].pix_mp.quantization =
			    vsession->port[MVX_DIR_OUTPUT].pix_mp.quantization;
			break;
		}
	case MVX_SESSION_EVENT_ERROR:{
			int i;

			for (i = 0; i < MVX_DIR_MAX; ++i) {
				struct vb2_queue *q =
				    &vsession->port[i].vb2_queue;

				if (vsession->port[i].q_set)
					vb2_queue_error(q);
				else
					MVX_SESSION_WARN(&vsession->session,
							 "vb2_queue has been released, dir %d",
							 vsession->port[i].dir);
			}

			break;
		}
	default:
		MVX_SESSION_WARN(&vsession->session,
				 "Unsupported session event. event=%d", event);
	}
}

int mvx_v4l2_session_construct(struct mvx_v4l2_session *vsession,
			       struct mvx_ext_if *ctx)
{
	int i;
	int ret;

	vsession->ext = ctx;
	mutex_init(&vsession->mutex);
	init_completion(&vsession->cmp);

	for (i = 0; i < MVX_DIR_MAX; i++) {
		struct mvx_v4l2_port *vport = &vsession->port[i];

		vport->port = &vsession->session.port[i];
		vport->vsession = vsession;
		vport->dir = i;
		vport->q_set = false;
	}

	if (IS_ENABLED(CONFIG_DEBUG_FS)) {
		ret = session_debugfs_init(vsession, ctx->dsessions);
		if (ret != 0)
			return ret;
	}

	ret = mvx_session_construct(&vsession->session, ctx->dev,
				    ctx->client_ops, ctx->cache,
				    &vsession->mutex,
				    free_session, handle_event,
				    vsession->dentry, ctx->is_encoder);
	if (ret != 0)
		goto remove_dentry;

	return 0;

remove_dentry:
	if (IS_ENABLED(CONFIG_DEBUG_FS))
		debugfs_remove_recursive(vsession->dentry);

	return ret;
}

struct mvx_v4l2_session *v4l2_fh_to_session(struct v4l2_fh *fh)
{
	return container_of(fh, struct mvx_v4l2_session, fh);
}

struct mvx_v4l2_session *file_to_session(struct file *file)
{
	return v4l2_fh_to_session(file->private_data);
}

int mvx_v4l2_session_set_roi_regions(struct mvx_v4l2_session *vsession,
				     struct v4l2_mvx_roi_regions *roi)
{
	int ret;
	struct mvx_roi_config roi_regions;

	roi_regions.pic_index = roi->pic_index;
	roi_regions.num_roi = roi->num_roi;
	roi_regions.qp_present = roi->qp_present;
	roi_regions.roi_present = roi->roi_present;
	roi_regions.qp = roi->qp;

	if (roi_regions.roi_present && roi_regions.num_roi > 0) {
		int i = 0;

		for (; i < roi_regions.num_roi; i++) {
			roi_regions.roi[i].mbx_left = roi->roi[i].mbx_left;
			roi_regions.roi[i].mbx_right = roi->roi[i].mbx_right;
			roi_regions.roi[i].mby_top = roi->roi[i].mby_top;
			roi_regions.roi[i].mby_bottom = roi->roi[i].mby_bottom;
			roi_regions.roi[i].qp_delta = roi->roi[i].qp_delta;
			roi_regions.roi[i].prio = roi->roi[i].prio;
			roi_regions.roi[i].force_intra =
			    roi->roi[i].force_intra;
		}
	}
	ret = mvx_session_set_roi_regions(&vsession->session, &roi_regions);
	if (ret != 0)
		return ret;

	return 0;
}

int mvx_v4l2_session_set_qp_epr(struct mvx_v4l2_session *vsession,
				struct v4l2_buffer_param_qp *qp)
{
	int ret;
	struct mvx_buffer_param_qp epr_qp;

	epr_qp.qp = qp->qp;
	epr_qp.epr_iframe_enable = qp->epr_iframe_enable;
	ret = mvx_session_set_qp_epr(&vsession->session, &epr_qp);
	if (ret != 0)
		return ret;

	return 0;
}

int mvx_v4l2_session_set_sei_userdata(struct mvx_v4l2_session *vsession,
				      struct v4l2_sei_user_data *sei_userdata)
{
	int ret;
	struct mvx_sei_userdata userdata;

	userdata.flags = sei_userdata->flags;
	userdata.user_data_len = sei_userdata->user_data_len;
	memcpy(&userdata.user_data, &sei_userdata->user_data,
	       sizeof(userdata.user_data));
	memcpy(&userdata.uuid, &sei_userdata->uuid, sizeof(userdata.uuid));
	ret = mvx_session_set_sei_userdata(&vsession->session, &userdata);
	if (ret != 0)
		return ret;

	return 0;
}

int mvx_v4l2_session_set_dsl_ratio(struct mvx_v4l2_session *vsession,
				   struct v4l2_mvx_dsl_ratio *dsl)
{
	int ret;
	struct mvx_dsl_ratio dsl_ratio;

	dsl_ratio.hor = dsl->hor;
	dsl_ratio.ver = dsl->ver;

	ret = mvx_session_set_dsl_ratio(&vsession->session, &dsl_ratio);
	if (ret != 0)
		return ret;

	return 0;
}

int mvx_v4l2_session_set_long_term_ref(struct mvx_v4l2_session *vsession,
				       struct v4l2_mvx_long_term_ref *ltr)
{
	int ret;
	struct mvx_long_term_ref mvx_ltr;

	mvx_ltr.mode = ltr->mode;
	mvx_ltr.period = ltr->period;
	ret = mvx_session_set_long_term_ref(&vsession->session, &mvx_ltr);
	if (ret != 0)
		return ret;

	return 0;

}

int mvx_v4l2_session_set_dsl_mode(struct mvx_v4l2_session *vsession, int *mode)
{
	int ret;

	ret = mvx_session_set_dsl_mode(&vsession->session, mode);
	if (ret != 0)
		return ret;

	return 0;
}

int mvx_v4l2_session_set_mini_frame_cnt(struct mvx_v4l2_session *vsession,
					int *cnt)
{
	int ret;

	ret = mvx_session_set_mini_frame_cnt(&vsession->session, cnt);
	if (ret != 0)
		return ret;

	return 0;
}

int mvx_v4l2_session_set_stats_mode(struct mvx_v4l2_session *vsession,
				    struct v4l2_buffer_param_enc_stats *stats)
{
	int ret;
	struct mvx_enc_stats enc_stats;

	memcpy(&enc_stats, stats, sizeof(struct mvx_enc_stats));
	ret = mvx_session_set_stats_mode(&vsession->session, &enc_stats);
	if (ret != 0)
		return ret;

	return 0;
}

int mvx_v4l2_session_set_chr_cfg(struct mvx_v4l2_session *vsession,
				 struct v4l2_mvx_chr_config *chr)
{
	int ret;
	struct mvx_chr_cfg chr_cfg;

	chr_cfg.pic_index = chr->pic_index;
	chr_cfg.num_chr = chr->num_chr;

	memcpy(chr_cfg.rectangle, chr->rectangle, sizeof(chr->rectangle));
	ret = mvx_session_set_chr_cfg(&vsession->session, &chr_cfg);
	if (ret != 0)
		return ret;

	return 0;
}

int mvx_v4l2_session_set_huff_table(struct mvx_v4l2_session *vsession,
				    struct v4l2_mvx_huff_table *table)
{
	int ret;
	struct mvx_huff_table huff_table;

	memcpy(&huff_table, table, sizeof(struct mvx_huff_table));
	ret = mvx_session_set_huff_table(&vsession->session, &huff_table);
	if (ret != 0)
		return ret;

	return 0;
}

int mvx_v4l2_session_set_seamless_target(struct mvx_v4l2_session *vsession, struct v4l2_mvx_seamless_target
					 *seamless)
{
	int ret;
	struct mvx_seamless_target seamless_target;

	memcpy(&seamless_target, seamless,
	       sizeof(struct v4l2_mvx_seamless_target));
	ret =
	    mvx_session_set_seamless_target(&vsession->session,
					    &seamless_target);
	if (ret != 0)
		return ret;

	return 0;
}

int mvx_v4l2_session_set_color_conv_coef(struct mvx_v4l2_session *vsession,
					 struct v4l2_mvx_color_conv_coef *coef)
{
	int ret;
	struct mvx_color_conv_coef conv_coef;

	memcpy(&conv_coef, coef, sizeof(struct mvx_color_conv_coef));
	ret =
	    mvx_session_set_color_conversion_ceof(&vsession->session,
						  &conv_coef);
	if (ret != 0)
		return ret;

	return 0;
}

int mvx_v4l2_session_set_rgb_conv_yuv_coef(struct mvx_v4l2_session *vsession, struct
					   v4l2_mvx_rgb2yuv_color_conv_coef
					   *coef)
{
	int ret;
	struct mvx_rgb2yuv_color_conv_coef conv_coef;

	memcpy(&conv_coef, coef, sizeof(struct mvx_rgb2yuv_color_conv_coef));
	ret = mvx_session_set_rgb_conv_yuv_coef(&vsession->session, &conv_coef);
	if (ret != 0)
		return ret;

	return 0;
}

int mvx_v4l2_session_set_osd_config(struct mvx_v4l2_session *vsession,
				    struct v4l2_osd_config *osd_cfg)
{
	int ret;
	struct mvx_osd_config osd;

	memcpy(&osd, osd_cfg, sizeof(struct v4l2_osd_config));
	ret = mvx_session_set_osd_config(&vsession->session, &osd);
	if (ret != 0)
		return ret;

	return 0;
}

int mvx_v4l2_session_set_osd_info(struct mvx_v4l2_session *vsession,
				  struct v4l2_osd_info *info,
				  enum mvx_format *osd_fmt)
{
	int ret;
	int i;
	struct mvx_osd_info osd_info;

	for (i = 0; i < V4L2_MAX_FRAME_OSD_REGION; i++) {
		osd_info.width_osd[i] = info->width_osd[i];
		osd_info.height_osd[i] = info->height_osd[i];
		osd_info.inputFormat_osd[i] = osd_fmt[i];
	}
	ret = mvx_session_set_osd_info(&vsession->session, &osd_info);
	if (ret != 0)
		return ret;

	return 0;
}

void mvx_v4l2_session_try_color_info(struct mvx_v4l2_session *vsession,
				     struct v4l2_pix_format_mplane *pix)
{
	unsigned int colorspace = pix->colorspace;
	unsigned int xfer_func = V4L2_XFER_FUNC_DEFAULT;
	unsigned int ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	unsigned int quantization = V4L2_QUANTIZATION_DEFAULT;

	set_default_video_signal_type(pix->colorspace,
				      &xfer_func, &ycbcr_enc, &quantization);

	if (colorspace != V4L2_COLORSPACE_DEFAULT) {
		if (get_u32_array_index
		    (primaries_map, ARRAY_SIZE(primaries_map), colorspace) < 0
		    && pix->colorspace != V4L2_COLORSPACE_SRGB)
			pix->colorspace = V4L2_COLORSPACE_DEFAULT;
	}

	if (pix->xfer_func != V4L2_XFER_FUNC_DEFAULT) {
		if (get_u32_array_index
		    (xfer_map, ARRAY_SIZE(xfer_map), pix->xfer_func) < 0)
			pix->xfer_func = xfer_func;
	}

	if (pix->ycbcr_enc != V4L2_YCBCR_ENC_DEFAULT) {
		if (get_u32_array_index
		    (matrix_map, ARRAY_SIZE(matrix_map), pix->ycbcr_enc) < 0)
			pix->ycbcr_enc = ycbcr_enc;
	}

	if (pix->quantization != V4L2_QUANTIZATION_DEFAULT) {
		if (get_u32_array_index
		    (range_map, ARRAY_SIZE(range_map), pix->quantization) < 0)
			pix->quantization = quantization;
	}
}

int mvx_v4l2_session_set_color_info(struct mvx_v4l2_session *vsession,
				    struct v4l2_pix_format_mplane *pix)
{
	int ret;
	struct mvx_fw_color_desc cd;
	unsigned int flex_colorspace;

	ret = mvx_session_get_color_desc(&vsession->session, &cd);
	if (ret != 0)
		return ret;

	mvx_v4l2_session_try_color_info(vsession, pix);

	flex_colorspace = pix->colorspace == V4L2_COLORSPACE_SRGB ?
	    V4L2_COLORSPACE_REC709 : pix->colorspace;
	cd.colour_primaries =
	    get_u32_array_index(primaries_map, ARRAY_SIZE(primaries_map),
				flex_colorspace);
	cd.transfer_characteristics =
	    get_u32_array_index(xfer_map, ARRAY_SIZE(xfer_map), pix->xfer_func);
	cd.matrix_coeff =
	    get_u32_array_index(matrix_map, ARRAY_SIZE(matrix_map),
				pix->ycbcr_enc);
	cd.range =
	    get_u32_array_index(range_map, ARRAY_SIZE(range_map),
				pix->quantization);

	return mvx_session_set_color_desc(&vsession->session, &cd);
}

int mvx_v4l2_session_set_enc_lambda_scale(struct mvx_v4l2_session *vsession, struct v4l2_mvx_lambda_scale
					  *lambda_scale)
{
	int ret;
	struct mvx_lambda_scale mvx_lambda_scale;

	memcpy(&mvx_lambda_scale, lambda_scale,
	       sizeof(struct mvx_lambda_scale));
	ret =
	    mvx_session_set_enc_lambda_scale(&vsession->session,
					     &mvx_lambda_scale);
	if (ret != 0)
		return ret;

	return 0;
}

int mvx_v4l2_session_get_hdr10_cll_info(struct mvx_v4l2_session *vsession,
					struct v4l2_ctrl_hdr10_cll_info *hdr)
{
	int ret;
	struct mvx_fw_color_desc cd;

	ret = mvx_session_get_color_desc(&vsession->session, &cd);
	if (ret != 0)
		return ret;

	if (!(cd.flags & MVX_FW_COLOR_DESC_CONTENT_VALID)) {
		memset(hdr, 0, sizeof(*hdr));
		return 0;
	}

	hdr->max_content_light_level = cd.content.luminance_max;
	hdr->max_pic_average_light_level = cd.content.luminance_average;

	return 0;
}

int mvx_v4l2_session_set_hdr10_cll_info(struct mvx_v4l2_session *vsession,
					struct v4l2_ctrl_hdr10_cll_info *hdr)
{
	int ret;
	struct mvx_fw_color_desc cd;

	if (hdr->max_content_light_level == 0
	    && hdr->max_pic_average_light_level == 0)
		return 0;

	ret = mvx_session_get_color_desc(&vsession->session, &cd);
	if (ret != 0)
		return ret;

	cd.flags |= MVX_FW_COLOR_DESC_CONTENT_VALID;
	cd.content.luminance_max = hdr->max_content_light_level;
	cd.content.luminance_average = hdr->max_pic_average_light_level;

	return mvx_session_set_color_desc(&vsession->session, &cd);
}

int mvx_v4l2_session_get_hdr10_mastering(struct mvx_v4l2_session *vsession, struct
					 v4l2_ctrl_hdr10_mastering_display * hdr)
{
	int ret;
	struct mvx_fw_color_desc cd;

	ret = mvx_session_get_color_desc(&vsession->session, &cd);
	if (ret != 0)
		return ret;

	if (!(cd.flags & MVX_FW_COLOR_DESC_DISPLAY_VALID)) {
		memset(hdr, 0, sizeof(*hdr));
		return 0;
	}

	hdr->display_primaries_x[0] = cd.display.r.x;
	hdr->display_primaries_y[0] = cd.display.r.y;
	hdr->display_primaries_x[1] = cd.display.g.x;
	hdr->display_primaries_y[1] = cd.display.g.y;
	hdr->display_primaries_x[2] = cd.display.b.x;
	hdr->display_primaries_y[2] = cd.display.b.y;
	hdr->white_point_x = cd.display.w.x;
	hdr->white_point_y = cd.display.w.y;
	hdr->min_display_mastering_luminance = cd.display.luminance_min;
	hdr->max_display_mastering_luminance = cd.display.luminance_max;

	return 0;
}

int mvx_v4l2_session_set_hdr10_mastering(struct mvx_v4l2_session *vsession, struct
					 v4l2_ctrl_hdr10_mastering_display * hdr)
{
	int ret;
	struct mvx_fw_color_desc cd;

	if (hdr->display_primaries_x[0] == 0 || hdr->display_primaries_y[0] == 0
	    || hdr->display_primaries_x[1] == 0
	    || hdr->display_primaries_y[1] == 0
	    || hdr->display_primaries_x[2] == 0
	    || hdr->display_primaries_y[2] == 0 || hdr->white_point_x == 0
	    || hdr->white_point_y == 0
	    || hdr->min_display_mastering_luminance == 0
	    || hdr->max_display_mastering_luminance == 0)
		return 0;

	ret = mvx_session_get_color_desc(&vsession->session, &cd);
	if (ret != 0)
		return ret;

	cd.flags |= MVX_FW_COLOR_DESC_DISPLAY_VALID;
	cd.display.r.x = hdr->display_primaries_x[0];
	cd.display.r.y = hdr->display_primaries_y[0];
	cd.display.g.x = hdr->display_primaries_x[1];
	cd.display.g.y = hdr->display_primaries_y[1];
	cd.display.b.x = hdr->display_primaries_x[2];
	cd.display.b.y = hdr->display_primaries_y[2];
	cd.display.w.x = hdr->white_point_x;
	cd.display.w.y = hdr->white_point_y;
	cd.display.luminance_min = hdr->min_display_mastering_luminance;
	cd.display.luminance_max = hdr->max_display_mastering_luminance;

	return mvx_session_set_color_desc(&vsession->session, &cd);
}

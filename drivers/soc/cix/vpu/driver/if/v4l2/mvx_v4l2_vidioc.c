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

#include <linux/device.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-dma-sg.h>
#include <linux/mvx-v4l2-controls.h>
#include "mvx_bitops.h"
#include "mvx_ext_if.h"
#include "mvx_if.h"
#include "mvx_v4l2_buffer.h"
#include "mvx_v4l2_session.h"
#include "mvx_v4l2_vidioc.h"

/****************************************************************************
 * Types
 ****************************************************************************/

/****************************************************************************
 * Static functions and variables
 ****************************************************************************/
static int to_v4l2_format(struct v4l2_format *f,
			  enum v4l2_buf_type type,
			  struct v4l2_pix_format_mplane *pix,
			  unsigned int *stride,
			  unsigned int *size, bool interlaced)
{
	struct mvx_session_format_map *map;
	unsigned int field;

	map = mvx_session_find_format(pix->pixelformat);
	if (IS_ERR(map))
		return PTR_ERR(map);
	field =
	    mvx_is_afbc(map->format) ? V4L2_FIELD_SEQ_TB :
	    V4L2_FIELD_INTERLACED;

	f->type = type;

	switch (f->type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:{
			struct v4l2_pix_format *p = &f->fmt.pix;
			uint32_t i;

			p->width = pix->width;
			p->height = pix->height;
			p->pixelformat = pix->pixelformat;
			p->field = interlaced ? field : V4L2_FIELD_NONE;
			p->colorspace = pix->colorspace;
			p->flags = pix->flags;
			p->ycbcr_enc = pix->ycbcr_enc;
			p->quantization = pix->quantization;
			p->xfer_func = pix->xfer_func;

			p->sizeimage = 0;
			p->bytesperline = stride[0];
			for (i = 0; i < pix->num_planes; ++i)
				p->sizeimage += size[i];

			break;
		}
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:{
			struct v4l2_pix_format_mplane *p = &f->fmt.pix_mp;
			int i;

			memcpy(p, pix, sizeof(*p));
			memset(p->reserved, 0, sizeof(p->reserved));
			p->field = interlaced ? field : V4L2_FIELD_NONE;

			for (i = 0; i < pix->num_planes; i++) {
				p->plane_fmt[i].bytesperline = stride[i];
				p->plane_fmt[i].sizeimage = size[i];
				memset(p->plane_fmt[i].reserved, 0,
				       sizeof(p->plane_fmt[i].reserved));
			}

			break;
		}
	default:
		return -EINVAL;
	}

	return 0;
}

static int from_v4l2_format(struct mvx_v4l2_session *vsession,
			    struct v4l2_format *f,
			    struct v4l2_pix_format_mplane *pix,
			    enum mvx_format *format,
			    unsigned int *stride,
			    unsigned int *size, bool *interlaced)
{
	struct mvx_session_format_map *map;

	switch (f->type) {
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:{
			struct v4l2_pix_format *p = &f->fmt.pix;

			memset(pix, 0, sizeof(*pix));

			pix->width = p->width;
			pix->height = p->height;
			pix->pixelformat = p->pixelformat;
			pix->field = p->field;
			pix->colorspace = p->colorspace;
			pix->flags = p->flags;

			if (p->priv != V4L2_PIX_FMT_PRIV_MAGIC) {
				pix->ycbcr_enc = V4L2_COLORSPACE_DEFAULT;
				pix->quantization = V4L2_QUANTIZATION_DEFAULT;
				pix->xfer_func = V4L2_XFER_FUNC_DEFAULT;
			}

			pix->num_planes = 1;
			pix->plane_fmt[0].sizeimage = p->sizeimage;
			pix->plane_fmt[0].bytesperline = p->bytesperline;

			size[0] = p->sizeimage;
			stride[0] = p->bytesperline;

			break;
		}
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:{
			struct v4l2_pix_format_mplane *p = &f->fmt.pix_mp;
			unsigned int i;

			if (p->num_planes > MVX_BUFFER_NPLANES)
				MVX_SESSION_WARN(&vsession->session,
						 "Too many planes for format. format=0x%08x, num_planes=%u.",
						 pix->pixelformat,
						 p->num_planes);

			memcpy(pix, p, sizeof(*pix));

			for (i = 0;
			     i < min_t(unsigned int, MVX_BUFFER_NPLANES,
				       p->num_planes); i++) {
				size[i] = p->plane_fmt[i].sizeimage;
				stride[i] = p->plane_fmt[i].bytesperline;
				vsession->session.setting_stride[i] = stride[i];
			}

			break;
		}
	default:
		return -EINVAL;
	}

	/* Adjust default field and color spaces. */

	if (pix->field == V4L2_FIELD_NONE || pix->field == V4L2_FIELD_ANY)
		*interlaced = false;
	else
		*interlaced = true;

	/* Find mapping between pixel format and mvx format. */
	map = mvx_session_find_format(pix->pixelformat);
	if (IS_ERR(map)) {
		MVX_SESSION_INFO(&vsession->session,
				 "Unsupported V4L2 pixel format. format=0x%08x.",
				 pix->pixelformat);
		return PTR_ERR(map);
	}

	*format = map->format;

	return 0;
}

/**
 * print_format() - Print V4L2 format.
 * @session:    Pointer to MVX session.
 * @f:        V4L2 format.
 * @prefix:    Prefix string.
 */
static void print_format(struct mvx_session *session,
			 struct v4l2_format *f, const char *prefix)
{
	if (V4L2_TYPE_IS_MULTIPLANAR(f->type) != false) {
		struct v4l2_pix_format_mplane *p = &f->fmt.pix_mp;

		MVX_SESSION_INFO(session,
				 "v4l2: %s. type=%u, pixelformat=0x%08x, width=%u, height=%u, num_planes=%u",
				 prefix,
				 f->type, p->pixelformat,
				 p->width, p->height, p->num_planes);
		MVX_SESSION_INFO(session,
				 "v4l2: %s. colorspace=%u, ycbcr_enc=%u, xfer_func=%u, quantization=%u",
				 prefix,
				 p->colorspace, p->ycbcr_enc,
				 p->xfer_func, p->quantization);
	} else {
		struct v4l2_pix_format *p = &f->fmt.pix;

		MVX_SESSION_INFO(session,
				 "v4l2: %s. type=%u, pixelformat=0x%08x, width=%u, height=%u.",
				 prefix,
				 f->type, p->pixelformat, p->width, p->height);
		MVX_SESSION_INFO(session,
				 "v4l2: %s. colorspace=%u, ycbcr_enc=%u, xfer_func=%u, quantization=%u",
				 prefix,
				 p->colorspace, p->ycbcr_enc,
				 p->xfer_func, p->quantization);
	}
}

/**
 * dump_input_data_to_local() - dump input buffer.
 * @session:        Pointer to MVX session.
 * @vb:             Buffer to dump.
 */
static void dump_input_data_to_local(struct mvx_session *session,
				     struct vb2_buffer *vb)
{
	struct mvx_session_port *input = &session->port[MVX_DIR_INPUT];
	bool is_ivf = (input->format == MVX_FORMAT_AV1 ||
		       input->format == MVX_FORMAT_VP8 ||
		       input->format == MVX_FORMAT_VP9);
	void *buffer = (void *)vb2_plane_vaddr(vb, 0);
	unsigned int size = vb->planes[0].bytesused;

	if (is_ivf) {
		struct mvx_ivf_frame ivf_frame = {
			.size = size,
			.timestamp = input->dump_count
		};
		uint8_t *ivf_frame_head = (uint8_t *) (&ivf_frame);

		kernel_write(session->data_fp, ivf_frame_head,
			     sizeof(struct mvx_ivf_frame),
			     &(session->data_fp->f_pos));
	}
	kernel_write(session->data_fp, buffer, size,
		     &(session->data_fp->f_pos));
}

/**
 * queue_setup() - Initialize or verify queue parameters.
 * @q:        Videobuf2 queue.
 * @buf_cnt:    Requested/requered buffers count.
 * @plane_cnt:    Required number of planes.
 * @plane_size:    Required size of each plane.
 * @alloc_devs:    Device to allocate memory from.
 *
 * This callback is used to query parameters of a queue from the driver.
 * Vb2 sets buf_cnt to requested amount of buffers, but a driver is free to
 * choose another value and return it. Vb2 will then call queue_setup() again
 * to verify that the new value is accepted by a driver.
 *
 * Vb2 also uses plane_cnt parameter to signal if queue_setup() was called
 * from create_bufs() of reqbufs().
 *
 * No locking is required in this function. The reason is that will be called
 * from within vb2_reqbufs() or vb2_create_bufs() which are executed from our
 * code with session mutex already taken.
 *
 * Return: 0 on success, else error code.
 */
static int queue_setup(struct vb2_queue *q,
		       unsigned int *buf_cnt,
		       unsigned int *plane_cnt,
		       unsigned int plane_size[], struct device *alloc_devs[])
{
	struct mvx_v4l2_port *vport = vb2_get_drv_priv(q);
	struct mvx_session_port *port = vport->port;
	struct mvx_v4l2_session *vsession = vport->vsession;
	struct mvx_session *session = &vsession->session;
	unsigned int i, port_format_bpp, display_size_format_bpp;

	/*
	 * If the output frame resolution is not known, then there is no need
	 * to allocate buffers yet. But 1 buffer will be needed to carry
	 * information about 'resolution change' and 'end of stream'.
	 */
	if (vport->dir == MVX_DIR_OUTPUT &&
	    mvx_is_frame(port->format) != false &&
	    (port->width == 0 || port->height == 0))
		*buf_cnt = 1;

	memset(plane_size, 0, sizeof(plane_size[0]) * VB2_MAX_PLANES);
	*plane_cnt = port->nplanes;
	port_format_bpp = mvx_get_format_bpp(port->format);
	display_size_format_bpp = mvx_get_format_bpp(port->display_size_format);
	for (i = 0; i < port->nplanes; ++i) {
		unsigned int tmp_size;
		/*  Vb2 allocator does not handle well buffers of zero size. */
		plane_size[i] = max_t(unsigned int, port->size[i], 1);
		if (vport->dir == MVX_DIR_OUTPUT &&
		    (port_format_bpp != 0 && display_size_format_bpp != 0) &&
		    (session->dsl_frame.width < 16
		     && session->dsl_frame.height < 16)
		    && (session->crop.crop_en == 0)) {
			tmp_size =
			    (unsigned int)((uint64_t) port->display_size[i] *
					   port_format_bpp /
					   display_size_format_bpp);
			plane_size[i] =
			    max_t(unsigned int, tmp_size, plane_size[i]);
		}
		if (session->seamless_target.seamless_mode != 0 &&
		    vport->dir == MVX_DIR_OUTPUT &&
		    vsession->port[MVX_DIR_INPUT].port->format <=
		    MVX_FORMAT_BITSTREAM_LAST) {
			plane_size[i] =
			    plane_size[i] <
			    session->seamless_target.target_size[i] ? session->seamless_target.target_size[i] :
			    plane_size[i];
		}
		alloc_devs[i] = session->dev;
	}

	return 0;
}

/**
 * buf_init() - Perform initialization for Vb2 buffer.
 * @b:        Pointer to Vb2 buffer.
 *
 * Vb2 framework calls this function once for every allocated buffer.
 * A driver fetches a list of memory pages and constructs MVX V4L2 buffers.
 *
 * No locking is required in this function. The reason is that will be called
 * from within vb2_reqbufs() or vb2_create_bufs() which are executed from our
 * code with session mutex already taken.
 *
 * Return: 0 in case of success, error code otherwise.
 */
static int buf_init(struct vb2_buffer *b)
{
	struct mvx_v4l2_buffer *vbuf = vb2_to_mvx_v4l2_buffer(b);

	int ret;
	unsigned int i;
	struct sg_table *sgt[MVX_BUFFER_NPLANES] = { 0 };
	struct vb2_queue *q = b->vb2_queue;
	struct mvx_v4l2_port *vport = vb2_get_drv_priv(q);
	struct mvx_v4l2_session *vsession = vport->vsession;
	struct mvx_session *session = &vsession->session;

	MVX_SESSION_VERBOSE(session,
			    "v4l2: Initialize buffer. vb=%p, type=%u, index=%u, num_planes=%u.",
			    b, b->type, b->index, b->num_planes);

	if (b->num_planes > MVX_BUFFER_NPLANES) {
		MVX_SESSION_WARN(session,
				 "Failed to initialize buffer. Too many planes. vb=%p, num_planes=%u.",
				 b, b->num_planes);
		return -EINVAL;
	}

	for (i = 0; i < b->num_planes; ++i) {
		sgt[i] = vb2_dma_sg_plane_desc(b, i);
		if (sgt[i] == NULL) {
			MVX_SESSION_WARN(session,
					 "Cannot fetch SG descriptor. vb=%p, plane=%u.",
					 b, i);
			return -ENOMEM;
		}
	}

	ret = mvx_v4l2_buffer_construct(vbuf, vsession, vport->dir,
					b->num_planes, sgt);

	return ret;
}

/**
 * buf_cleanup() - Destroy data associated to Vb2 buffer.
 * @b:        Pointer to Vb2 buffer.
 *
 * Vb2 framework calls this function while destroying a buffer.
 */
static void buf_cleanup(struct vb2_buffer *b)
{
	struct vb2_queue *q = b->vb2_queue;
	struct mvx_v4l2_port *vport = vb2_get_drv_priv(q);
	struct mvx_v4l2_session *vsession = vport->vsession;
	struct mvx_session *session = &vsession->session;
	struct mvx_v4l2_buffer *vbuf = vb2_to_mvx_v4l2_buffer(b);

	MVX_SESSION_VERBOSE(session,
			    "v4l2: Cleanup buffer. vb=%p, index=%u, vbuf=%p, type=%u.",
			    b, b->type, b->index, vbuf);

	mvx_v4l2_buffer_destruct(vbuf);
}

/**
 * start_streaming() - Start streaming for queue.
 * @q:        Pointer to a queue.
 * @cnt:    Amount of buffers already owned by a driver.
 *
 * Vb2 calls this function when it is ready to start streaming for a queue.
 * Vb2 ensures that minimum required amount of buffers were enqueued to the
 * driver before calling this function.
 *
 * Return: 0 in case of success, error code otherwise.
 */
static int start_streaming(struct vb2_queue *q, unsigned int cnt)
{
	/*
	 * Parameter cnt is not used so far.
	 */
	struct mvx_v4l2_port *vport = vb2_get_drv_priv(q);
	struct mvx_v4l2_session *vsession = vport->vsession;
	struct mvx_session *session = &vsession->session;
	int ret;

	MVX_SESSION_VERBOSE(session,
			    "v4l2: Start streaming. queue=%p, type=%u, cnt=%u.",
			    q, q->type, cnt);

	ret = mvx_session_streamon(&vsession->session, vport->dir);

	/*
	 * If attempt was not successful, we should return all owned buffers
	 * to Vb2 with vb2_buffer_done() with state VB2_BUF_STATE_QUEUED.
	 */
	if (ret != 0 && atomic_read(&q->owned_by_drv_count) > 0) {
		int i;

		for (i = 0; i < q->num_buffers; ++i)
			if (q->bufs[i]->state == VB2_BUF_STATE_ACTIVE)
				vb2_buffer_done(q->bufs[i],
						VB2_BUF_STATE_QUEUED);

		WARN_ON(atomic_read(&q->owned_by_drv_count));
	}

	return ret;
}

/**
 * stop_streaming() - Stop streaming for a queue.
 * @q:        Pointer to a queue.
 *
 * Vb2 calls this function when streaming should be terminated.
 * The driver must ensure that no DMA transfers are ongoing and
 * return all buffers to Vb2 with vb2_buffer_done().
 */
static void stop_streaming(struct vb2_queue *q)
{
	struct mvx_v4l2_port *vport = vb2_get_drv_priv(q);
	struct mvx_v4l2_session *vsession = vport->vsession;
	struct mvx_session *session = &vsession->session;

	MVX_SESSION_VERBOSE(session,
			    "v4l2: Stop streaming. queue=%p, type=%u.",
			    q, q->type);

	mvx_session_streamoff(&vsession->session, vport->dir);

	/*
	 * We have to return all owned buffers to Vb2 before exiting from
	 * this callback.
	 *
	 * Note: there must be no access to buffers after they are returned.
	 */
	if (atomic_read(&q->owned_by_drv_count) > 0) {
		int i;

		for (i = 0; i < q->num_buffers; ++i)
			if (q->bufs[i]->state == VB2_BUF_STATE_ACTIVE)
				vb2_buffer_done(q->bufs[i],
						VB2_BUF_STATE_ERROR);

		WARN_ON(atomic_read(&q->owned_by_drv_count));
	}
}

/**
 * buf_queue() - Enqueue buffer to a driver.
 * @b:        Pointer to Vb2 buffer structure.
 *
 * Vb2 calls this function to enqueue a buffer to a driver.
 * A driver should later return a buffer to Vb2 with vb2_buffer_done().
 *
 * Return: 0 in case of success, error code otherwise.
 */
static void buf_queue(struct vb2_buffer *b)
{
	struct vb2_queue *q = b->vb2_queue;
	struct mvx_v4l2_port *vport = vb2_get_drv_priv(q);
	enum mvx_direction dir = vport->dir;
	struct mvx_v4l2_session *vsession = vport->vsession;
	struct mvx_session *session = &vsession->session;
	struct mvx_v4l2_buffer *vbuf = vb2_to_mvx_v4l2_buffer(b);

	int ret;

	MVX_SESSION_VERBOSE(session,
			    "v4l2: Queue buffer. b=%p, type=%u, index=%u.",
			    b, b->type, b->index);
	if (!vsession->first_input_processed && b->planes[0].bytesused >= 4) {
		if (dir == MVX_DIR_INPUT
		    && vport->port->format == MVX_FORMAT_JPEG) {
			uint32_t *data =
			    vb2_plane_vaddr(b, 0) + b->planes[0].data_offset;
			if (*data == v4l2_fourcc('A', 'V', 'I', 'F')) {
				/* Not a valid bitstream buffer, return it to client */
				MVX_SESSION_INFO(session,
						 "v4l2: Skip invalid bitstream buffer, offset = %d, size = %d",
						 b->planes[0].data_offset,
						 b->planes[0].bytesused);
				b->planes[0].data_offset +=
				    b->planes[0].bytesused;
				b->planes[0].bytesused = 0;
				vb2_buffer_done(b, VB2_BUF_STATE_DONE);
				return;
			}
		}
		vsession->first_input_processed = true;
	}
	vbuf->buf.format = vport->port->format;
	if (vsession->session.force_key_frame && dir == MVX_DIR_INPUT) {
		struct vb2_v4l2_buffer *vb2_v4l2 = to_vb2_v4l2_buffer(b);

		vb2_v4l2->flags |= V4L2_BUF_FLAG_KEYFRAME;
		mvx_session_set_force_key_frame(&vsession->session, 0);
	}
	ret = mvx_v4l2_buffer_set(vbuf, b);
	if (ret != 0)
		goto failed;
	if (dir == MVX_DIR_INPUT && session->data_fp != NULL) {
		dump_input_data_to_local(session, b);
		session->port[dir].dump_count++;
	}
	ret = mvx_session_qbuf(&vsession->session, dir, &vbuf->buf);
	if (ret != 0)
		goto failed;
	return;

failed:
	if (vbuf->buf.flags & MVX_BUFFER_FRAME_NEED_REALLOC) {
		vbuf->vb2_v4l2_buffer.flags |=
		    V4L2_BUF_FLAG_MVX_BUFFER_NEED_REALLOC;
		vb2_buffer_done(b, VB2_BUF_STATE_DONE);
		return;
	}
	vb2_buffer_done(b, VB2_BUF_STATE_ERROR);
}

/**
 * buf_finish() - Finish buffer before it is returned to user space.
 * @vb:        Pointer to Vb2 buffer structure.
 */
static void buf_finish(struct vb2_buffer *vb)
{
	struct mvx_v4l2_port *vport = vb2_get_drv_priv(vb->vb2_queue);
	struct mvx_v4l2_buffer *vbuf = vb2_to_mvx_v4l2_buffer(vb);

	if (vbuf->buf.planes[0].filled > 0) {
		vport->afbc_crop_left = vbuf->buf.crop_left;
		vport->afbc_crop_top = vbuf->buf.crop_top;
	}
}

/**
 * wait_prepare() - Prepare driver for waiting
 * @q:        Pointer to Vb2 queue.
 *
 * Vb2 calls this function when it is about to wait for more buffers to
 * be received. A driver should release any locks taken while calling Vb2
 * functions.
 * This is required to avoid a deadlock.
 *
 * This is unused for now and will be called from Vb2.
 */
static void wait_prepare(struct vb2_queue *q)
{
	struct mvx_v4l2_port *vport = vb2_get_drv_priv(q);
	struct mvx_v4l2_session *vsession = vport->vsession;
	struct mvx_session *session = &vsession->session;

	MVX_SESSION_VERBOSE(session, "v4l2: Wait prepare. queue=%p.", q);

	mutex_unlock(&vsession->mutex);
}

/**
 * wait_finish() - Wake up after sleep.
 * @q:        Pointer to Vb2 queue.
 *
 * Require mutexes release before.
 *
 * This is unused for now and will be called from Vb2.
 */
static void wait_finish(struct vb2_queue *q)
{
	struct mvx_v4l2_port *vport = vb2_get_drv_priv(q);
	struct mvx_v4l2_session *vsession = vport->vsession;
	struct mvx_session *session = &vsession->session;
	int ignore;

	MVX_SESSION_VERBOSE(session, "v4l2: Wait finish. queue=%p.", q);

	/*
	 * mutex_lock_interruptible is declared with attribute
	 * warn_unused_result, but we have no way to return a status
	 * from wait_finish().
	 */
	ignore = mutex_lock_interruptible(&vsession->mutex);
}

/**
 * mvx_vb2_ops - Callbacks for Vb2 framework
 * Not all possible callbacks are implemented as some of them are optional.
 */
const struct vb2_ops mvx_vb2_ops = {
	.queue_setup = queue_setup,
	.buf_init = buf_init,
	.buf_finish = buf_finish,
	.buf_cleanup = buf_cleanup,
	.start_streaming = start_streaming,
	.stop_streaming = stop_streaming,
	.buf_queue = buf_queue,
	.wait_prepare = wait_prepare,
	.wait_finish = wait_finish
};

/**
 * setup_vb2_queue() - Initialize vb2_queue before it can be used by Vb2.
 */
static int setup_vb2_queue(struct mvx_v4l2_port *vport)
{
	struct vb2_queue *q = &vport->vb2_queue;
	struct device *dev = vport->vsession->ext->dev;
	int ret;

	q->drv_priv = vport;
	q->type = vport->type;
	q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	q->dev = dev;
	q->ops = &mvx_vb2_ops;
	q->mem_ops = &vb2_dma_sg_memops;
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	q->allow_zero_bytesused = true;
	if (vport->dir == MVX_DIR_OUTPUT && mvx_is_frame(vport->port->format))
		q->min_buffers_needed = 0;
	else
		q->min_buffers_needed = 1;

	/* Let Vb2 handle mvx_v4l2_buffer allocations. */
	q->buf_struct_size = sizeof(struct mvx_v4l2_buffer);

	ret = vb2_queue_init(q);

	return ret;
}

/****************************************************************************
 * Exported functions and variables
 ****************************************************************************/

int mvx_v4l2_vidioc_querycap(struct file *file,
			     void *fh, struct v4l2_capability *cap)
{
	struct mvx_v4l2_session *session = file_to_session(file);

	MVX_SESSION_INFO(&session->session, "v4l2: Query capabilities.");

	strscpy(cap->driver, "mvx", sizeof(cap->driver));
	strscpy(cap->card, "Linlon Video device", sizeof(cap->card));
	strscpy(cap->bus_info, "platform:mvx", sizeof(cap->bus_info));

	cap->capabilities = V4L2_CAP_DEVICE_CAPS |
	    V4L2_CAP_VIDEO_M2M |
	    V4L2_CAP_VIDEO_M2M_MPLANE |
	    V4L2_CAP_EXT_PIX_FORMAT | V4L2_CAP_STREAMING;
	cap->device_caps = cap->capabilities & ~V4L2_CAP_DEVICE_CAPS;

	return 0;
}

static int mvx_v4l2_vidioc_enum_fmt_vid(struct mvx_v4l2_session *session,
					struct v4l2_fmtdesc *f,
					enum mvx_direction dir)
{
	struct mvx_session_format_map *mvx_fmt = NULL;

	mvx_fmt = mvx_session_enum_format(&session->session, dir, f->index);
	if (!mvx_fmt)
		return -EINVAL;

	f->flags = mvx_fmt->flags;
	f->pixelformat = mvx_fmt->pixelformat;
	strscpy(f->description, mvx_fmt->description, sizeof(f->description));

	return 0;
}

int mvx_v4l2_vidioc_enum_fmt_vid_cap(struct file *file,
				     void *fh, struct v4l2_fmtdesc *f)
{
	struct mvx_v4l2_session *session = file_to_session(file);
	int ret;

	ret = mvx_v4l2_vidioc_enum_fmt_vid(session, f, MVX_DIR_OUTPUT);

	return ret;
}

int mvx_v4l2_vidioc_enum_fmt_vid_out(struct file *file,
				     void *fh, struct v4l2_fmtdesc *f)
{
	struct mvx_v4l2_session *session = file_to_session(file);
	int ret;

	ret = mvx_v4l2_vidioc_enum_fmt_vid(session, f, MVX_DIR_INPUT);

	return ret;
}

int mvx_v4l2_vidioc_enum_framesizes(struct file *file,
				    void *fh, struct v4l2_frmsizeenum *fsize)
{
	struct mvx_session_format_map *format;
	struct mvx_ext_if *ctx = video_drvdata(file);
	struct mvx_v4l2_session *vsession = file_to_session(file);

	/* Verify that format is supported. */
	format = mvx_session_find_format(fsize->pixel_format);
	if (IS_ERR(format))
		return PTR_ERR(format);

	/*
	 * For uncompressed format, check the corresponding compressed format
	 * in the other port to get max/min resolution.
	 */
	if (format->format >= MVX_FORMAT_FRAME_FIRST) {
		struct mvx_session_format_map *bits_format;

		bits_format =
		    mvx_session_get_compressed_format(&vsession->session);
		if (!IS_ERR(bits_format))
			format = bits_format;
	}

	/* For stepwise/continuous frame size the index must be 0. */
	if (fsize->index != 0)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;
	mvx_session_enum_framesizes(&vsession->session, ctx->is_encoder,
				    format->format,
				    &(fsize->stepwise.min_width),
				    &(fsize->stepwise.min_height),
				    &(fsize->stepwise.max_width),
				    &(fsize->stepwise.max_height),
				    &(fsize->stepwise.step_width),
				    &(fsize->stepwise.step_height));

	return 0;
}

static void mvx_v4l2_copy_color_desc(struct v4l2_pix_format_mplane *dst,
				     struct v4l2_pix_format_mplane *src)
{
	dst->colorspace = src->colorspace;
	dst->xfer_func = src->xfer_func;
	dst->ycbcr_enc = src->ycbcr_enc;
	dst->quantization = src->quantization;
}

static int mvx_v4l2_vidioc_g_fmt_vid(struct file *file,
				     struct v4l2_format *f,
				     enum mvx_direction dir)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	struct mvx_v4l2_port *vport = &vsession->port[dir];
	struct mvx_session_port *port = &vsession->session.port[dir];
	int ret;

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	if (dir == MVX_DIR_OUTPUT && vsession->ext->is_encoder)
		mvx_v4l2_copy_color_desc(&vport->pix_mp,
					 &vsession->port[MVX_DIR_INPUT].pix_mp);

	to_v4l2_format(f, f->type, &vport->pix_mp, port->stride, port->size,
		       port->interlaced);

	mutex_unlock(&vsession->mutex);

	print_format(&vsession->session, f, "Get format");

	return 0;
}

int mvx_v4l2_vidioc_g_fmt_vid_cap(struct file *file,
				  void *fh, struct v4l2_format *f)
{
	return mvx_v4l2_vidioc_g_fmt_vid(file, f, MVX_DIR_OUTPUT);
}

int mvx_v4l2_vidioc_g_fmt_vid_out(struct file *file,
				  void *fh, struct v4l2_format *f)
{
	return mvx_v4l2_vidioc_g_fmt_vid(file, f, MVX_DIR_INPUT);
}

static int mvx_v4l2_vidioc_s_fmt_vid(struct file *file,
				     struct v4l2_format *f,
				     enum mvx_direction dir)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	struct mvx_session_port *port = &vsession->session.port[dir];
	struct mvx_v4l2_port *vport = &vsession->port[dir];
	struct v4l2_pix_format_mplane pix_mp;
	enum mvx_format format;
	unsigned int stride[MVX_BUFFER_NPLANES] = { 0 };
	unsigned int size[MVX_BUFFER_NPLANES];
	bool interlaced = false;
	int ret;

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	if (vport->q_set != false && vb2_is_busy(&vport->vb2_queue) != false) {
		MVX_SESSION_WARN(&vsession->session,
				 "Can't set format when there buffers allocated to the port.");
		ret = -EBUSY;
		goto unlock_mutex;
	}

	/* Convert V4L2 format to V4L2 multi planar pixel format. */
	ret = from_v4l2_format(vsession, f, &pix_mp, &format, stride, size,
			       &interlaced);
	if (ret != 0)
		goto unlock_mutex;

	/* Validate and adjust settings. */
	ret =
	    mvx_session_set_format(&vsession->session, dir, format,
				   pix_mp.pixelformat, &pix_mp.width,
				   &pix_mp.height, &pix_mp.num_planes, stride,
				   size, &interlaced);
	if (ret != 0)
		goto unlock_mutex;

	if (dir == MVX_DIR_INPUT && vsession->ext->is_encoder)
		mvx_v4l2_session_set_color_info(vsession, &pix_mp);

	if (dir == MVX_DIR_OUTPUT && vsession->ext->is_encoder)
		mvx_v4l2_copy_color_desc(&pix_mp,
					 &vsession->port[MVX_DIR_INPUT].pix_mp);

	/* Convert V4L2 multi planar pixel format to format. */
	ret = to_v4l2_format(f, f->type, &pix_mp, stride, size, interlaced);
	if (ret != 0)
		goto unlock_mutex;

	vport->type = f->type;
	vport->pix_mp = pix_mp;

	if (dir == MVX_DIR_OUTPUT && !vsession->ext->is_encoder &&
	    !mvx_is_afbc(port->format)) {
		mvx_session_update_buffer_count(&vsession->session, dir);
	}

	/* Update output dimensions to align with input */
	if (dir == MVX_DIR_INPUT) {
		vsession->port[MVX_DIR_OUTPUT].pix_mp.width = pix_mp.width;
		vsession->port[MVX_DIR_OUTPUT].pix_mp.height = pix_mp.height;
	}

unlock_mutex:
	mutex_unlock(&vsession->mutex);

	print_format(&vsession->session, f, "Set format");

	return ret;
}

int mvx_v4l2_vidioc_s_fmt_vid_cap(struct file *file,
				  void *fh, struct v4l2_format *f)
{
	return mvx_v4l2_vidioc_s_fmt_vid(file, f, MVX_DIR_OUTPUT);
}

int mvx_v4l2_vidioc_s_fmt_vid_out(struct file *file,
				  void *fh, struct v4l2_format *f)
{
	return mvx_v4l2_vidioc_s_fmt_vid(file, f, MVX_DIR_INPUT);
}

static int mvx_v4l2_vidioc_try_fmt_vid(struct file *file,
				       struct v4l2_format *f,
				       enum mvx_direction dir)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	struct v4l2_pix_format_mplane pix;
	enum mvx_format format;
	unsigned int stride[MVX_BUFFER_NPLANES] = { 0 };
	unsigned int size[MVX_BUFFER_NPLANES];
	bool interlaced = false;
	int ret;

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	ret = from_v4l2_format(vsession, f, &pix, &format, stride, size,
			       &interlaced);
	if (ret != 0)
		goto unlock_mutex;

	ret = mvx_session_try_format(&vsession->session, dir, format,
				     &pix.width, &pix.height, &pix.num_planes,
				     stride, size, &interlaced);
	if (ret != 0)
		goto unlock_mutex;

	if ((dir == MVX_DIR_INPUT && vsession->ext->is_encoder)
	    || !vsession->ext->is_encoder)
		mvx_v4l2_session_try_color_info(vsession, &pix);

	if (dir == MVX_DIR_OUTPUT && vsession->ext->is_encoder)
		mvx_v4l2_copy_color_desc(&pix,
					 &vsession->port[MVX_DIR_INPUT].pix_mp);

	ret = to_v4l2_format(f, f->type, &pix, stride, size, interlaced);
	if (ret != 0)
		goto unlock_mutex;

unlock_mutex:
	mutex_unlock(&vsession->mutex);

	print_format(&vsession->session, f, "Try format");

	return ret;
}

int mvx_v4l2_vidioc_try_fmt_vid_cap(struct file *file,
				    void *fh, struct v4l2_format *f)
{
	return mvx_v4l2_vidioc_try_fmt_vid(file, f, MVX_DIR_OUTPUT);
}

int mvx_v4l2_vidioc_try_fmt_vid_out(struct file *file,
				    void *fh, struct v4l2_format *f)
{
	return mvx_v4l2_vidioc_try_fmt_vid(file, f, MVX_DIR_INPUT);
}

static void mvx_get_composing(struct mvx_v4l2_session *vsession,
			      enum mvx_direction dir, struct v4l2_selection *s)
{
	struct mvx_session *session = &vsession->session;
	struct mvx_v4l2_port *vport = &vsession->port[dir];
	struct mvx_session_port *port = &session->port[dir];
	enum mvx_format format = port->format;

	if (mvx_is_frame(format)) {
		if (mvx_is_afbc(format)) {
			s->r.left =
			    vport->afbc_crop_left >> session->dual_afbc_downscaled;
			s->r.top =
			    vport->afbc_crop_top >> session->dual_afbc_downscaled;
		} else {
			s->r.left = 0;
			s->r.top = 0;
		}
		s->r.width = port->width;
		s->r.height = port->height;
	}
}

int mvx_v4l2_vidioc_g_selection(struct file *file,
				void *fh, struct v4l2_selection *s)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	struct mvx_session *session = &vsession->session;
	enum mvx_direction dir = V4L2_TYPE_IS_OUTPUT(s->type) ?
	    MVX_DIR_INPUT : MVX_DIR_OUTPUT;
	struct mvx_session_port *port = &session->port[dir];
	enum mvx_format format = port->format;
	int ret = 0;

	if (s->type != V4L2_BUF_TYPE_VIDEO_CAPTURE &&
	    s->type != V4L2_BUF_TYPE_VIDEO_OUTPUT &&
	    s->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE &&
	    s->type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		return -EINVAL;

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	switch (s->target) {
	case V4L2_SEL_TGT_CROP:
	case V4L2_SEL_TGT_CROP_DEFAULT:
	case V4L2_SEL_TGT_CROP_BOUNDS:
	case V4L2_SEL_TGT_COMPOSE:
	case V4L2_SEL_TGT_COMPOSE_DEFAULT:
	case V4L2_SEL_TGT_COMPOSE_BOUNDS:
	case V4L2_SEL_TGT_COMPOSE_PADDED:
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = session->orig_width;
		s->r.height = session->orig_height;
		break;
	default:
		mutex_unlock(&vsession->mutex);
		return -EINVAL;
	}

	if (s->target == V4L2_SEL_TGT_CROP && session->crop.crop_en) {
		s->r.left = session->crop.x;
		s->r.top = session->crop.y;
		s->r.width = session->crop.width;
		s->r.height = session->crop.height;
	}

	if (s->target == V4L2_SEL_TGT_COMPOSE) {
		if (vsession->ext->is_encoder) {
			if (format == MVX_FORMAT_H264
			    || format == MVX_FORMAT_HEVC) {
				/*
				 * Frame cropping offset parameters of H.264 or conformance
				 * cropping window offset parameters of HEVC
				 */
				s->r.left = session->crop_left;
				s->r.top = session->crop_top;
				s->r.width =
				    port->width - session->crop_left -
				    session->crop_right;
				s->r.height =
				    port->height - session->crop_top -
				    session->crop_bottom;
			}
		} else {	/* is decoder */
			mvx_get_composing(vsession, dir, s);
		}
	} else if (s->target == V4L2_SEL_TGT_COMPOSE_DEFAULT) {
		if (!vsession->ext->is_encoder)
			mvx_get_composing(vsession, dir, s);
	}

	if (s->r.width == 0 || s->r.height == 0)
		ret = -EINVAL;

	mutex_unlock(&vsession->mutex);

	if (ret == 0)
		MVX_SESSION_INFO(session,
				 "v4l2: Get selection. target = %d, dir=%u, crop={left=%u, top=%u, width=%u, height=%u.",
				 s->target, dir, s->r.left, s->r.top,
				 s->r.width, s->r.height);

	return ret;
}

static void mvx_validate_enc_crop(unsigned int width, unsigned int height,
				  struct v4l2_rect *rect)
{
	if (rect->top < 0 || rect->left < 0 || rect->width == 0
	    || rect->height == 0) {
		rect->top = 0;
		rect->left = 0;
		rect->width = width;
		rect->height = height;

		return;
	}

	rect->top = ALIGN(rect->top, 2);
	rect->left = ALIGN(rect->left, 2);
	rect->width = ALIGN(rect->width, 2);
	rect->height = ALIGN(rect->height, 2);

	rect->width = clamp_t(uint32_t, rect->width, 16, width);
	rect->left = min_t(uint32_t, rect->left, width - rect->width);
	rect->height = clamp_t(uint32_t, rect->height, 16, height);
	rect->top = min_t(uint32_t, rect->top, height - rect->height);
}

static int mvx_set_enc_crop(struct mvx_v4l2_session *vsession,
			    struct v4l2_selection *s, enum mvx_direction dir)
{
	struct mvx_session_port *port = &vsession->session.port[dir];
	struct mvx_session *session = &vsession->session;
	int ret;

	if (port->format != MVX_FORMAT_H264 && port->format != MVX_FORMAT_HEVC) {
		if (port->width > 0 && port->height > 0 &&
		    port->width != s->r.width && port->height != s->r.height) {
			MVX_SESSION_ERR(session,
					"v4l2: encode cropping is supported for H.264 and HEVC only.");
			return -EINVAL;
		} else {
			return 0;
		}
	}

	mvx_validate_enc_crop(port->width, port->height, &s->r);

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;
	do {
		ret = mvx_session_set_crop_left(session, s->r.left);
		if (ret != 0)
			break;
		ret = mvx_session_set_crop_top(session, s->r.top);
		if (ret != 0)
			break;
		ret = mvx_session_set_crop_right(session,
						 port->width - s->r.width -
						 s->r.left);
		if (ret != 0)
			break;
		ret = mvx_session_set_crop_bottom(session,
						  port->height - s->r.height -
						  s->r.top);
		if (ret != 0)
			break;
	} while (0);
	mutex_unlock(&vsession->mutex);

	MVX_SESSION_INFO(session,
			 "v4l2: Set enc crop. type=%u, crop={left=%u, top=%u, right=%u, bottom=%u.",
			 s->type, s->r.left, s->r.top,
			 port->width - s->r.width - s->r.left,
			 port->height - s->r.height - s->r.top);

	return ret;
}

static void mvx_validate_crop(unsigned int width, unsigned int height,
			      struct v4l2_selection *s, int alignment)
{
	struct v4l2_rect *rect = &s->r;

	if (rect->top < 0 || rect->left < 0 || rect->width == 0
	    || rect->height == 0) {
		rect->top = 0;
		rect->left = 0;
		rect->width = width;
		rect->height = height;

		return;
	}

	if (s->flags == V4L2_SEL_FLAG_GE) {
		rect->top = round_up(rect->top, alignment);
		rect->left = round_up(rect->left, alignment);
		rect->width = round_up(rect->width, alignment);
		rect->height = round_up(rect->height, alignment);

		rect->width = max_t(uint32_t, rect->width, 64);
		rect->height = max_t(uint32_t, rect->height, 64);
	} else {
		rect->top = round_down(rect->top, alignment);
		rect->left = round_down(rect->left, alignment);
		rect->width = round_down(rect->width, alignment);
		rect->height = round_down(rect->height, alignment);

		if (width >= 144)
			rect->width =
			    min_t(uint32_t, rect->width, width - rect->left);
		if (height >= 144)
			rect->height =
			    min_t(uint32_t, rect->height, height - rect->top);
	}
}

static int mvx_set_crop(struct mvx_v4l2_session *vsession,
			struct v4l2_selection *s, enum mvx_direction dir)
{
	struct mvx_session_port *port = &vsession->session.port[dir];
	struct mvx_session *session = &vsession->session;
	struct mvx_crop_cfg mvx_crop;
	int ret;

	if (mvx_is_afbc(port->format) || mvx_is_bitstream(port->format)) {
		MVX_SESSION_WARN(session,
				 "v4l2: cropping is not supported for AFBC and bitstream.");
		if (session->orig_width > 0 && session->orig_height > 0 &&
		    session->orig_width != s->r.width
		    && session->orig_height != s->r.height)
			return -EINVAL;
		else
			return 0;
	}

	mvx_validate_crop(session->orig_width, session->orig_height, s,
			  (vsession->ext->is_encoder) ? 2 : 4);

	if (session->orig_width >= 144 && session->orig_height >= 144) {
		if ((session->orig_width < (s->r.left + s->r.width) ||
		     session->orig_height < (s->r.top + s->r.height))) {
			MVX_SESSION_WARN(session,
					 "v4l2: crop size is larger than original size.");
			return -ERANGE;
		}

		if (s->r.width < 64 || s->r.height < 64) {
			MVX_SESSION_WARN(session,
					 "v4l2: crop size is smaller than 64.");
			return -ERANGE;
		}

		if (session->orig_width == s->r.width
		    && session->orig_height == s->r.height)
			return 0;
	}

	mvx_crop.crop_en = 1;
	mvx_crop.x = s->r.left;
	mvx_crop.y = s->r.top;
	mvx_crop.width = s->r.width;
	mvx_crop.height = s->r.height;

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;
	ret = mvx_session_set_crop(session, &mvx_crop);
	if (vsession->ext->is_encoder) {
		/* For encoder, update the output resolution to cropped one */
		vsession->port[MVX_DIR_OUTPUT].pix_mp.width = s->r.width;
		vsession->port[MVX_DIR_OUTPUT].pix_mp.height = s->r.height;
	} else {
		/*
		 * Update port resolution for decode only; keep original resolution
		 * for encoder to hold the whole frame data
		 */
		vsession->port[dir].pix_mp.width = s->r.width;
		vsession->port[dir].pix_mp.height = s->r.height;
	}
	if (dir == MVX_DIR_OUTPUT && !vsession->ext->is_encoder &&
	    !mvx_is_afbc(port->format)) {
		mvx_session_update_buffer_count(session, dir);
	}

	mutex_unlock(&vsession->mutex);

	MVX_SESSION_INFO(session,
			 "v4l2: Set crop. type=%u, crop={left=%u, top=%u, width=%u, height=%u.",
			 s->type, s->r.left, s->r.top, s->r.width, s->r.height);

	return ret;
}

static void mvx_validate_scale(unsigned int width, unsigned int height,
			       bool is_afbc, struct v4l2_rect *rect)
{
	if (rect->top < 0 || rect->left < 0 || rect->width < 16
	    || rect->height < 16) {
		rect->top = 0;
		rect->left = 0;
		rect->width = width;
		rect->height = height;
		return;
	}

	rect->top = 0;
	rect->left = 0;
	rect->width = ALIGN(rect->width, 2);
	rect->height = ALIGN(rect->height, 2);

	if (width >= 144)
		rect->width = min_t(uint32_t, rect->width, width);
	rect->width = max_t(uint32_t, rect->width, 16);
	if (height >= 144)
		rect->height = min_t(uint32_t, rect->height, height);
	rect->height = max_t(uint32_t, rect->height, 16);

	if (is_afbc) {
		/* AFBC supports dual downscaling only */
		rect->width = width >> 1;
		rect->height = height >> 1;
	}
}

static int mvx_set_scale(struct mvx_v4l2_session *vsession,
			 struct v4l2_selection *s, enum mvx_direction dir)
{
	struct mvx_session_port *port = &vsession->session.port[dir];
	struct mvx_session *session = &vsession->session;
	int ret;

	if (vsession->ext->is_encoder)
		return -EINVAL;

	if ((session->orig_width < s->r.width
	     || session->orig_height < s->r.height)
	    && (session->orig_width >= 144 && session->orig_height >= 144)) {
		MVX_SESSION_WARN(session, "v4l2: Upscaling is not supported.");
		return -EINVAL;
	}

	if (session->orig_width == s->r.width
	    && session->orig_height == s->r.height)
		return 0;
	mvx_validate_scale(session->orig_width, session->orig_height,
			   mvx_is_afbc(port->format), &s->r);
	if (session->orig_width == s->r.width
	    && session->orig_height == s->r.height)
		return 0;

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	if (mvx_is_afbc(port->format)) {
		MVX_SESSION_INFO(session, "v4l2: Force dual AFBC downscaling.");
		ret = mvx_session_set_dual_afbc_downscaled(session, 1);
		s->r.left = port->afbc_crop_left >> 1;
		s->r.top = port->afbc_crop_top >> 1;
	} else {
		struct mvx_dsl_frame dsl_frame;

		dsl_frame.width = s->r.width;
		dsl_frame.height = s->r.height;
		ret = mvx_session_set_dsl_frame(session, &dsl_frame);
	}

	vsession->port[dir].pix_mp.width = s->r.width + s->r.left;
	vsession->port[dir].pix_mp.height = s->r.height + s->r.top;

	if (dir == MVX_DIR_OUTPUT && !vsession->ext->is_encoder &&
	    !mvx_is_afbc(port->format)) {
		mvx_session_update_buffer_count(session, dir);
	}

	mutex_unlock(&vsession->mutex);

	MVX_SESSION_INFO(session,
			 "v4l2: Set compose (scaling). type=%u, dst={left=%u, top=%u, width=%u, height=%u.",
			 s->type, s->r.left, s->r.top, s->r.width, s->r.height);

	return ret;
}

int mvx_v4l2_vidioc_s_selection(struct file *file,
				void *fh, struct v4l2_selection *s)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	enum mvx_direction dir = V4L2_TYPE_IS_OUTPUT(s->type) ?
	    MVX_DIR_INPUT : MVX_DIR_OUTPUT;

	if (s->type != V4L2_BUF_TYPE_VIDEO_CAPTURE &&
	    s->type != V4L2_BUF_TYPE_VIDEO_OUTPUT &&
	    s->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE &&
	    s->type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		return -EINVAL;

	if (vsession->ext->is_encoder) {
		if (s->target == V4L2_SEL_TGT_COMPOSE && dir == MVX_DIR_OUTPUT)
			return mvx_set_enc_crop(vsession, s, dir);
		else if (s->target == V4L2_SEL_TGT_CROP && dir == MVX_DIR_INPUT)
			return mvx_set_crop(vsession, s, dir);
	} else {
		if (s->target == V4L2_SEL_TGT_COMPOSE && dir == MVX_DIR_OUTPUT)
			return mvx_set_scale(vsession, s, dir);
		else if (s->target == V4L2_SEL_TGT_CROP
			 && dir == MVX_DIR_OUTPUT)
			return mvx_set_crop(vsession, s, dir);
	}

	return -EINVAL;
}

int mvx_v4l2_vidioc_g_parm(struct file *file,
			   void *fh, struct v4l2_streamparm *a)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	struct mvx_session *session = &vsession->session;
	struct v4l2_fract *frameperiod = &a->parm.capture.timeperframe;

	if (!vsession->ext->is_encoder) {
		if (!V4L2_TYPE_IS_OUTPUT(a->type)) {
			/*
			 * Decode driver doesn't get frame rate from bitstream. So we
			 * don't set V4L2_CAP_TIMEPERFRAME capability, but just set
			 * numerator and denominator to non-zero values here in case
			 * client uses them and encounters divided-by-zero exception.
			 */
			a->parm.capture.capability = 0;
		} else {
			/* Allow client to set output port fps but driver won't send it to VPU */
			a->parm.output.capability = V4L2_CAP_TIMEPERFRAME;
		}

		if (frameperiod->denominator == 0) {
			frameperiod->numerator = session->fps_d;
			frameperiod->denominator = session->fps_n;
		}
	} else {
		frameperiod->numerator = session->fps_d;
		frameperiod->denominator = session->fps_n;
		a->parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
	}

	return 0;
}

int mvx_v4l2_vidioc_s_parm(struct file *file,
			   void *fh, struct v4l2_streamparm *a)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	struct mvx_session *session = &vsession->session;
	struct v4l2_fract *frameperiod;

	if (mutex_lock_interruptible(&vsession->mutex) != 0)
		return -EINTR;

	if (a->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
	    a->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		struct v4l2_captureparm *cparm = &a->parm.capture;
		/* Just do sanity check but not update fps as VPU doesn't support frame rate conversion */
		frameperiod = &a->parm.capture.timeperframe;
		if (frameperiod->numerator * session->fps_n !=
		    frameperiod->denominator * session->fps_d
		    || frameperiod->numerator == 0
		    || frameperiod->denominator == 0) {
			MVX_SESSION_WARN(&session,
					 "v4l2: Invalid frame period from client (%d/%d). Return %d/%d",
					 frameperiod->numerator,
					 frameperiod->denominator,
					 session->fps_n, session->fps_d);
			frameperiod->numerator = session->fps_d;
			frameperiod->denominator = session->fps_n;
		}
		if (vsession->ext->is_encoder)
			cparm->capability = V4L2_CAP_TIMEPERFRAME;
	} else if (a->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE ||
		   a->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		struct v4l2_outputparm *oparm = &a->parm.output;

		frameperiod = &a->parm.output.timeperframe;
		if (frameperiod->numerator == 0
		    || frameperiod->denominator == 0) {
			frameperiod->numerator = session->fps_n;
			frameperiod->denominator = session->fps_d;
			oparm->capability = V4L2_CAP_TIMEPERFRAME;
			mutex_unlock(&vsession->mutex);
			return 0;
		}
		if ((uint64_t) frameperiod->denominator >
		    (uint64_t) frameperiod->numerator * MAX_FRAME_RATE) {
			MVX_SESSION_WARN(&session,
					 "v4l2: Framerate is larger than maximum value of VPU");
			frameperiod->denominator =
			    MAX_FRAME_RATE * frameperiod->numerator;
		}
		/* Set frame rate if it's valid */
		mvx_session_set_frame_rate(session, frameperiod->denominator,
					   frameperiod->numerator);
		oparm->capability = V4L2_CAP_TIMEPERFRAME;
	} else {
		mutex_unlock(&vsession->mutex);
		return -EINVAL;
	}

	mutex_unlock(&vsession->mutex);
	return 0;
}

int mvx_v4l2_vidioc_streamon(struct file *file,
			     void *priv, enum v4l2_buf_type type)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	enum mvx_direction dir = V4L2_TYPE_IS_OUTPUT(type) ?
	    MVX_DIR_INPUT : MVX_DIR_OUTPUT;
	int ret;

	MVX_SESSION_INFO(&vsession->session, "v4l2: Stream on. dir=%u.", dir);

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	ret = vb2_streamon(&vsession->port[dir].vb2_queue, type);
	if (ret != 0)
		MVX_SESSION_WARN(&vsession->session,
				 "v4l2: Failed to stream on. dir=%u.", dir);

	mutex_unlock(&vsession->mutex);

	return ret;
}

int mvx_v4l2_vidioc_streamoff(struct file *file,
			      void *priv, enum v4l2_buf_type type)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	enum mvx_direction dir = V4L2_TYPE_IS_OUTPUT(type) ?
	    MVX_DIR_INPUT : MVX_DIR_OUTPUT;
	int ret;

	MVX_SESSION_INFO(&vsession->session, "v4l2: Stream off. dir=%u.", dir);

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	ret = vb2_streamoff(&vsession->port[dir].vb2_queue, type);
	if (ret != 0)
		MVX_SESSION_WARN(&vsession->session,
				 "v4l2: Failed to stream off. dir=%u.", dir);

	MVX_SESSION_INFO(&vsession->session,
			 "v4l2: Stream off exit. dir=%u, ret=%d.", dir, ret);

	mutex_unlock(&vsession->mutex);

	return ret;
}

int mvx_v4l2_vidioc_encoder_cmd(struct file *file,
				void *priv, struct v4l2_encoder_cmd *cmd)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	int ret;

	if (!vsession->ext->is_encoder)
		return -ENOTTY;

	MVX_SESSION_INFO(&vsession->session, "v4l2: encoder cmd: %u.",
			 cmd->cmd);

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	switch (cmd->cmd) {
	case V4L2_ENC_CMD_STOP:
		ret = mvx_session_send_eos(&vsession->session);
		break;
	default:
		MVX_SESSION_WARN(&vsession->session,
				 "Unsupported command. cmd: %u.", cmd->cmd);
		ret = -EINVAL;
	}

	mutex_unlock(&vsession->mutex);

	return ret;
}

int mvx_v4l2_vidioc_try_encoder_cmd(struct file *file,
				    void *priv, struct v4l2_encoder_cmd *cmd)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);

	if (!vsession->ext->is_encoder)
		return -ENOTTY;

	switch (cmd->cmd) {
	case V4L2_ENC_CMD_STOP:
		return 0;
	default:
		return -EINVAL;
	}
}

int mvx_v4l2_vidioc_decoder_cmd(struct file *file,
				void *priv, struct v4l2_decoder_cmd *cmd)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	int ret;

	if (vsession->ext->is_encoder)
		return -ENOTTY;

	MVX_SESSION_INFO(&vsession->session, "v4l2: decoder cmd: %u.",
			 cmd->cmd);

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	switch (cmd->cmd) {
	case V4L2_DEC_CMD_STOP:
		ret = mvx_session_send_eos(&vsession->session);
		break;
	case V4L2_DEC_CMD_START:
		ret = mvx_session_start(&vsession->session);
		break;
	default:
		MVX_SESSION_WARN(&vsession->session,
				 "Unsupported command. cmd: %u.", cmd->cmd);
		ret = -EINVAL;
	}

	mutex_unlock(&vsession->mutex);

	return ret;
}

int mvx_v4l2_vidioc_try_decoder_cmd(struct file *file,
				    void *priv, struct v4l2_decoder_cmd *cmd)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);

	if (vsession->ext->is_encoder)
		return -ENOTTY;

	switch (cmd->cmd) {
	case V4L2_DEC_CMD_STOP:
		cmd->stop.pts = 0;
		break;
	case V4L2_DEC_CMD_START:
		cmd->start.speed = 0;
		cmd->start.format = V4L2_DEC_START_FMT_NONE;
		break;
	default:
		return -EINVAL;
	}

	cmd->flags = 0;

	return 0;
}

int mvx_v4l2_vidioc_reqbufs(struct file *file,
			    void *fh, struct v4l2_requestbuffers *b)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	enum mvx_direction dir = V4L2_TYPE_IS_OUTPUT(b->type) ?
	    MVX_DIR_INPUT : MVX_DIR_OUTPUT;
	struct mvx_v4l2_port *vport = &vsession->port[dir];
	int ret;

	MVX_SESSION_INFO(&vsession->session,
			 "v4l2: Request buffers. dir=%d, type=%u, memory=%u, count=%u.",
			 dir, b->type, b->memory, b->count);

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	if (b->count == 0) {
		if (vport->q_set != false) {
			vb2_queue_release(&vport->vb2_queue);
			vport->q_set = false;
		}
	} else {
		if (vport->q_set == false) {
			/* Set buffer type in case of calling REQBUFS before S_FMT */
			vport->type = b->type;
			ret = setup_vb2_queue(vport);
			if (ret != 0)
				goto unlock_mutex;

			vport->q_set = true;
		}

		ret = vb2_reqbufs(&vport->vb2_queue, b);
	}
	vport->port->buffer_allocated = b->count;
unlock_mutex:
	mutex_unlock(&vsession->mutex);

	return ret;
}

int mvx_v4l2_vidioc_create_bufs(struct file *file,
				void *fh, struct v4l2_create_buffers *b)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	enum mvx_direction dir = V4L2_TYPE_IS_OUTPUT(b->format.type) ?
	    MVX_DIR_INPUT : MVX_DIR_OUTPUT;
	struct mvx_v4l2_port *vport = &vsession->port[dir];
	int ret;

	MVX_SESSION_INFO(&vsession->session,
			 "v4l2: Create buffers. dir=%d, type=%u, memory=%u, count=%u.",
			 dir, b->format.type, b->memory, b->count);

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	if (vport->q_set == false)
		ret = setup_vb2_queue(vport);

	if (ret != 0)
		goto unlock_mutex;

	vport->q_set = true;

	ret = vb2_create_bufs(&vport->vb2_queue, b);
	vport->port->buffer_allocated += b->count;
	MVX_SESSION_INFO(&vsession->session, "count =%d,buffer_allocated=%d",
			 b->count, vport->port->buffer_allocated);
unlock_mutex:
	mutex_unlock(&vsession->mutex);

	return ret;
}

int mvx_v4l2_vidioc_querybuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	enum mvx_direction dir = V4L2_TYPE_IS_OUTPUT(b->type) ?
	    MVX_DIR_INPUT : MVX_DIR_OUTPUT;
	struct mvx_v4l2_port *vport = &vsession->port[dir];
	int ret;

	MVX_SESSION_INFO(&vsession->session,
			 "v4l2: Query buffer. dir=%d, type=%u, memory=%u, index=%u.",
			 dir, b->type, b->memory, b->index);

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	ret = vb2_querybuf(&vport->vb2_queue, b);
	if (ret != 0)
		goto unlock_mutex;

	/*
	 * When user space wants to mmap() a buffer, we have to be able to
	 * determine a direction of coresponding port. To make it easier we
	 * adjust mem_offset on output port by DST_QUEUE_OFF_BASE for all
	 * buffers.
	 */
	if (dir == MVX_DIR_OUTPUT) {
		if (V4L2_TYPE_IS_MULTIPLANAR(b->type)) {
			int i;

			for (i = 0; i < b->length; ++i)
				b->m.planes[i].m.mem_offset +=
				    DST_QUEUE_OFF_BASE;
		} else {
			b->m.offset += DST_QUEUE_OFF_BASE;
		}
	}

unlock_mutex:
	mutex_unlock(&vsession->mutex);

	return ret;
}

int mvx_v4l2_vidioc_qbuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	enum mvx_direction dir = V4L2_TYPE_IS_OUTPUT(b->type) ?
	    MVX_DIR_INPUT : MVX_DIR_OUTPUT;
	struct mvx_v4l2_port *vport = &vsession->port[dir];
	struct mvx_v4l2_buffer *vbuf;
	struct mvx_buffer *buf;
	struct vb2_buffer *vb;
	struct v4l2_core_buffer_header_general *v4l2_general;
	int ret;

	MVX_SESSION_INFO(&vsession->session,
			 "v4l2: Queue buffer. dir=%d, type=%u, index=%u, flags=0x%x.",
			 dir, b->type, b->index, b->flags);

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0) {
		MVX_SESSION_WARN(&vsession->session,
				 "v4l2: Queue buffer. Get lock failed.");
		return -EAGAIN;
	}

	if ((b->flags & V4L2_BUF_FLAG_MVX_BUFFER_GENERAL) ==
	    V4L2_BUF_FLAG_MVX_BUFFER_GENERAL) {
		vb = vport->vb2_queue.bufs[b->index];
		vbuf = vb2_to_mvx_v4l2_buffer(vb);
		buf = &vbuf->buf;
		v4l2_general =
		    (struct v4l2_core_buffer_header_general *)&b->m.planes[0].reserved[0];
		buf->general.header.buffer_size = v4l2_general->buffer_size;
		buf->general.header.config_size = v4l2_general->config_size;
		buf->general.header.type = v4l2_general->type;

		if (v4l2_general->type ==
		    V4L2_BUFFER_GENERAL_TYPE_BLOCK_CONFIGS) {
			memcpy(&buf->general.config.block_configs,
			       &v4l2_general->config.config,
			       sizeof(v4l2_general->config.config));
			MVX_SESSION_INFO(&vsession->session,
					 "v4l2: Queue buffer. type:%d, config size:%d, buffer size:%d, cfg_type:0x%x, cols and rows:%d, %d",
					 v4l2_general->type,
					 v4l2_general->config_size,
					 v4l2_general->buffer_size,
					 v4l2_general->config.config.blk_cfg_type,
					 v4l2_general->config.config.blk_cfgs.rows_uncomp.n_cols_minus1,
					 v4l2_general->config.config.blk_cfgs.rows_uncomp.n_rows_minus1);
		} else {
			MVX_SESSION_ERR(&vsession->session,
					"v4l2: Queue buffer. Unknown general buffer type:%d",
					v4l2_general->type);
		}
	}
	if (dir == MVX_DIR_INPUT && V4L2_TYPE_IS_MULTIPLANAR(b->type)) {
		vb = vport->vb2_queue.bufs[b->index];
		vbuf = vb2_to_mvx_v4l2_buffer(vb);
		buf = &vbuf->buf;
		if (b->reserved2 & V4L2_BUF_FLAG_MVX_MINIFRAME) {
			//this is miniframe encode mode.
			buf->offset = b->m.planes[0].reserved[10];
		} else {
			buf->offset = 0;
		}
		if (b->reserved2 & V4L2_BUF_FLAG_MVX_OSD_MASK) {
			buf->flags |=
			    b->reserved2 & V4L2_BUF_FLAG_MVX_OSD_1 ?
			    MVX_BUFFER_FRAME_FLAG_OSD_1 : 0;
			buf->flags |=
			    b->reserved2 & V4L2_BUF_FLAG_MVX_OSD_2 ?
			    MVX_BUFFER_FRAME_FLAG_OSD_2 : 0;
		} else {
			buf->flags &= ~MVX_BUFFER_FRAME_FLAG_OSD_MASK;
		}
	}
	ret = vb2_qbuf(&vport->vb2_queue, NULL, b);
	if (ret != 0)
		MVX_SESSION_VERBOSE(&vsession->session,
				    "v4l2: Queue buffer. vb2_qbuf() failed, dir=%d, ret=%d",
				    dir, ret);
	mutex_unlock(&vsession->mutex);

	return ret;
}

int mvx_v4l2_vidioc_dqbuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	struct mvx_ext_if *ctx __maybe_unused = vsession->ext;
	enum mvx_direction dir = V4L2_TYPE_IS_OUTPUT(b->type) ?
	    MVX_DIR_INPUT : MVX_DIR_OUTPUT;
	struct mvx_v4l2_port *vport = &vsession->port[dir];
	struct vb2_buffer *vb;
	struct mvx_v4l2_buffer *vbuf;
	struct mvx_buffer *buf;
	int ret;
	uint32_t i;

	MVX_SESSION_INFO(&vsession->session,
			 "v4l2: Dequeue buffer. dir=%d, type=%u.",
			 dir, b->type);

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0) {
		MVX_SESSION_WARN(&vsession->session,
				 "v4l2: Dequeue buffer. Get lock failed.");
		return -EAGAIN;
	}

	ret = vb2_dqbuf(&vport->vb2_queue, b, file->f_flags & O_NONBLOCK);
	if (ret != 0) {
		MVX_SESSION_VERBOSE(&vsession->session,
				    "v4l2: Dequeue buffer. vb2_dqbuf() failed, dir=%d, ret=%d",
				    dir, ret);
		goto unlock_mutex;
	}

	if ((dir == MVX_DIR_OUTPUT) && (b->flags & V4L2_BUF_FLAG_LAST)) {
		const struct v4l2_event event = {
			.type = V4L2_EVENT_EOS
		};
		v4l2_event_queue_fh(&vsession->fh, &event);
	}

	/*
	 * For single planar buffers there is no data offset. Instead the
	 * offset is added to the memory pointer and subtraced from the
	 * bytesused.
	 */
	vb = vport->vb2_queue.bufs[b->index];
	if (V4L2_TYPE_IS_MULTIPLANAR(vb->type) == false) {
		b->bytesused -= vb->planes[0].data_offset;

		switch (vb->type) {
		case V4L2_MEMORY_MMAP:
			b->m.offset += vb->planes[0].data_offset;
			break;
		case V4L2_MEMORY_USERPTR:
			b->m.userptr += vb->planes[0].data_offset;
			break;
		default:
			break;
		}
	}

	if (dir == MVX_DIR_OUTPUT && !V4L2_TYPE_IS_MULTIPLANAR(b->type)) {
		vbuf = vb2_to_mvx_v4l2_buffer(vb);
		buf = &vbuf->buf;
		b->reserved2 = 0;
		b->reserved2 =
		    (buf->frame_type << 24) | (buf->src_transform << 16) |
		    (buf->bitstream_remaining_kb);
	}
	if (vsession->port[MVX_DIR_INPUT].port->format <=
	    MVX_FORMAT_BITSTREAM_LAST && dir == MVX_DIR_OUTPUT
	    && V4L2_TYPE_IS_MULTIPLANAR(b->type)) {
		vbuf = vb2_to_mvx_v4l2_buffer(vb);
		buf = &vbuf->buf;
		b->reserved2 = 0;
		b->reserved2 = (buf->width << 16) | (buf->height);
		for (i = 0; i < b->length; i++)
			b->m.planes[i].reserved[0] = buf->planes[i].stride;
	}

unlock_mutex:
	mutex_unlock(&vsession->mutex);

	MVX_SESSION_INFO(&vsession->session,
			 "v4l2: Dequeued buffer ret=%d. dir=%d, type=%u, index=%u, flags=0x%x, nevents=%u, fh=%p.",
			 ret, dir, b->type, b->index, b->flags,
			 v4l2_event_pending(&vsession->fh), fh);

	return ret;
}

int mvx_v4l2_vidioc_expbuf(struct file *file,
			   void *fh, struct v4l2_exportbuffer *b)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	enum mvx_direction dir = V4L2_TYPE_IS_OUTPUT(b->type) ?
	    MVX_DIR_INPUT : MVX_DIR_OUTPUT;
	struct mvx_v4l2_port *vport = &vsession->port[dir];
	int ret;

	MVX_SESSION_INFO(&vsession->session,
			 "v4l2: Export buffer. dir=%d, type=%u, index=%u, plane=%u.",
			 dir, b->type, b->index, b->plane);

	ret = mutex_lock_interruptible(&vsession->mutex);
	if (ret != 0)
		return ret;

	ret = vb2_expbuf(&vport->vb2_queue, b);

	mutex_unlock(&vsession->mutex);

	return ret;
}

int mvx_v4l2_vidioc_subscribe_event(struct v4l2_fh *fh,
				    const struct v4l2_event_subscription *sub)
{
	struct mvx_v4l2_session *session = v4l2_fh_to_session(fh);

	MVX_SESSION_INFO(&session->session,
			 "v4l2: Subscribe event. fh=%p, type=%u.", fh,
			 sub->type);

	switch (sub->type) {
	case V4L2_EVENT_CTRL:
		return v4l2_ctrl_subscribe_event(fh, sub);
	case V4L2_EVENT_EOS:
	case V4L2_EVENT_SOURCE_CHANGE:
		return v4l2_event_subscribe(fh, sub, 2, NULL);
	default:
		MVX_SESSION_WARN(&session->session,
				 "Can't register for unsupported event. type=%u.",
				 sub->type);
		return -EINVAL;
	}

	return 0;
}

long mvx_v4l2_vidioc_default(struct file *file,
			     void *fh,
			     bool valid_prio, unsigned int cmd, void *arg)
{
	struct mvx_v4l2_session *vsession = file_to_session(file);
	int ret;

	MVX_SESSION_INFO(&vsession->session,
			 "Custom ioctl. cmd=0x%x, arg=0x%p.", cmd, arg);

	if (mutex_lock_interruptible(&vsession->mutex) != 0)
		return -EINTR;

	switch (cmd) {
	case VIDIOC_S_MVX_ROI_REGIONS:{
			ret = mvx_v4l2_session_set_roi_regions(vsession, arg);
			break;
		}
	case VIDIOC_S_MVX_QP_EPR:{
			ret = mvx_v4l2_session_set_qp_epr(vsession, arg);
			break;
		}
	case VIDIOC_S_MVX_SEI_USERDATA:{
			ret = mvx_v4l2_session_set_sei_userdata(vsession, arg);
			break;
		}
	case VIDIOC_S_MVX_DSL_RATIO:{
			ret = mvx_v4l2_session_set_dsl_ratio(vsession, arg);
			break;
		}
	case VIDIOC_S_MVX_LONG_TERM_REF:{
			ret = mvx_v4l2_session_set_long_term_ref(vsession, arg);
			break;
		}
	case VIDIOC_S_MVX_DSL_MODE:{
			ret = mvx_v4l2_session_set_dsl_mode(vsession, arg);
			break;
		}
	case VIDIOC_S_MVX_MINI_FRAME_CNT:{
			ret =
			    mvx_v4l2_session_set_mini_frame_cnt(vsession, arg);
			break;
		}
	case VIDIOC_S_MVX_STATS_MODE:{
			ret = mvx_v4l2_session_set_stats_mode(vsession, arg);
			break;
		}
	case VIDIOC_S_MVX_CHR_CFG:{
			ret = mvx_v4l2_session_set_chr_cfg(vsession, arg);
			break;
		}
	case VIDIOC_S_MVX_HUFF_TABLE:{
			ret = mvx_v4l2_session_set_huff_table(vsession, arg);
			break;
		}
	case VIDIOC_S_MVX_SEAMLESS_TARGET:{
			ret =
			    mvx_v4l2_session_set_seamless_target(vsession, arg);
			break;
		}
	case VIDIOC_S_MVX_COLOR_CONV_COEF:{
			ret =
			    mvx_v4l2_session_set_color_conv_coef(vsession, arg);
			break;
		}
	case VIDIOC_S_MVX_RGB2YUV_COLOR_CONV_COEF:{
			ret =
			    mvx_v4l2_session_set_rgb_conv_yuv_coef(vsession,
								   arg);
			break;
		}
	case VIDIOC_S_MVX_OSD_CONFIG:{
			ret = mvx_v4l2_session_set_osd_config(vsession, arg);
			break;
		}
	case VIDIOC_S_MVX_OSD_INFO:{
			struct v4l2_osd_info *osd_info =
			    (struct v4l2_osd_info *)arg;
			enum mvx_format osd_fmt[MVX_MAX_FRAME_OSD_REGION];
			int i;
			struct mvx_session_format_map *osd_map;

			for (i = 0; i < MVX_MAX_FRAME_OSD_REGION; i++) {
				osd_map =
				    mvx_session_find_format
				    (osd_info->inputFormat_osd[i]);
				osd_fmt[i] = osd_map->format;
			}
			ret =
			    mvx_v4l2_session_set_osd_info(vsession, arg,
							  osd_fmt);
			break;
		}
	case VIDIOC_S_MVX_ENC_LAMBDA_SCALE:{
			ret =
			    mvx_v4l2_session_set_enc_lambda_scale(vsession,
								  arg);
			break;
		}
	default:
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Unsupported IOCTL. cmd=0x%x", cmd);
		ret = -ENOTTY;
	}

	mutex_unlock(&vsession->mutex);
	return ret;
}

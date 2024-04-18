/*
* SPDX-License-Identifier: GPL
*
* Copyright (c) 2020 ChangSha JingJiaMicro Electronics Co., Ltd.
* All rights reserved.
*
* Author:
*      shanjinkui <shanjinkui@jingjiamicro.com>
*
* The software and information contained herein is proprietary and
* confidential to JingJiaMicro Electronics. This software can only be
* used by JingJiaMicro Electronics Corporation. Any use, reproduction,
* or disclosure without the written permission of JingJiaMicro
* Electronics Corporation is strictly prohibited.
*/
#include <linux/dma-fence.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/freezer.h>
#include <linux/list.h>
#include <linux/irqdomain.h>
#include "mwv207_bo.h"
#include "mwv207_sched.h"

#undef MWV207_DMA_DEBUG

#define DMA_MAX_LLI_COUNT    (0x400000 / sizeof(struct mwv207_lli))

#define DMA_TS_NOT_ALIGNED(size)  ((size) - 1)
#define DMA_TS_ALIGNED(size, alignment)  (((size) >= (alignment)) ? ((size) / (alignment) - 1) : 0)
#define DMA_CTL_NOT_ALIGNED ((1UL<<(63)) \
				| (1UL<<(58)) \
				| (1UL<<(30)) \
				| (1UL<<(38)) \
				| (1UL<<(47)) \
				| ((32ul & (0xFF))<<(39)) \
				| ((32ul & (0xFF))<<(48)) \
				| ((0UL & (0x7))<<(8)) \
				| ((0UL & (0x7))<<(11)))

#define DMA_CTL_ALIGNED(a)  ((1UL<<(63)) \
				| (1UL<<(58)) \
				| (1UL<<(30)) \
				| (1UL<<(38)) \
				| (1UL<<(47)) \
				| ((32ul & (0xFF))<<(39)) \
				| ((32ul & (0xFF))<<(48)) \
				| (((((a) == 64)?6UL:5UL) & (0x7))<<(8)) \
				| (((((a) == 64)?6UL:5UL) & (0x7))<<(11)))

#define mwv207_timed_loop(tick, cond, timeout) \
	for ((tick) = jiffies; (cond) && time_before(jiffies, (tick) + msecs_to_jiffies(timeout));)

struct mwv207_lli {
	u64 sar;
	u64 dar;
	u32 ts;
	u32 resv1;
	u64 llp;
	u64 ctl;
	u32 sstat;
	u32 dstat;
	u64 llpstat;
	u32 resv2;
	u32 resv3;
};

#define to_dma_pipe(pipe) container_of(pipe, struct mwv207_pipe_dma, base)
struct mwv207_pipe_dma {
	struct mwv207_pipe base;
	int   unit;
	u32 alignment;
	struct mwv207_lli   *lli;
	struct mwv207_lli   *last_lli;
	dma_addr_t    lli_bus_addr;
	int cur_idx;

	bool coherent;
};

struct mwv207_dma_cursor {
	bool is_vram;
	u64 line_pos;
	u64 cur_pos;

	dma_addr_t *dma_address;
	u64 nr_pages;

	u64 width;
	u64 stride;
};
static inline void pipe_dma_write(struct mwv207_pipe_dma *pipe, u32 reg, u32 value)
{
	pipe_write(&pipe->base, reg, value);
}

static inline u32 pipe_dma_read(struct mwv207_pipe_dma *pipe, u32 reg)
{
	return pipe_read(&pipe->base, reg);
}

static inline void pipe_dma_chan_write(struct mwv207_pipe_dma *pipe, int chan, u32 reg, u32 val)
{
	pipe_dma_write(pipe, chan * 0x200 + reg, val);
}

static inline u32 pipe_dma_chan_read(struct mwv207_pipe_dma *pipe, int chan, u32 reg)
{
	return pipe_dma_read(pipe, chan * 0x200 + reg);
}

static inline int pipe_dma_chan_done(struct mwv207_pipe_dma *pipe, int chan)
{
	u32 status;

	if (pipe->last_lli->ctl & (1UL << (63)))
		return 0;

	status = pipe_dma_chan_read(pipe, chan, (0x188));
	return !!(status & (1 << (1)));
}

static inline void pipe_dma_chan_clear(struct mwv207_pipe_dma *pipe, int chan)
{
	pipe_dma_chan_write(pipe, chan, (0x198), 1 << (1));
}

static inline int pipe_dma_stop(struct mwv207_pipe_dma *pipe)
{
	pr_info("%s TBD", __func__);
	return 0;
}

static void mwv207_pipe_dma_reset(struct mwv207_pipe *mpipe)
{
	pr_info("%s todo", __func__);
}

static void pipe_dma_commit(struct mwv207_pipe_dma *pipe)
{
	unsigned long tick;
	int chan = 0;
	u64 llp;

	if (pipe->cur_idx == 0)
		return;

	pipe->last_lli = &pipe->lli[pipe->cur_idx - 1];
	pipe->last_lli->ctl |=  (1UL << (62));

	mb();

	pipe_dma_chan_write(pipe, chan, (0x120),
			       ((0x3)<<(2))
			       |((0x3)<<(0)));
	pipe_dma_chan_write(pipe, chan, (0x124), 0x3b8e0000);

	llp = pipe->lli_bus_addr + 0x800000000ULL;
	pipe_dma_chan_write(pipe, chan, (0x128), llp & 0xFFFFFFFF);
	pipe_dma_chan_write(pipe, chan, (0x12C),  llp >> 32);

	pipe_dma_write(pipe, (0x10),
			(1 << (1))
			|(1 << (0)));

	pipe_dma_write(pipe, (0x18),
			    ((1<<chan)<<(0))
			    |((1<<chan)<<(8)));

	mwv207_timed_loop(tick, !pipe_dma_chan_done(pipe, chan), 5000) {
		cpu_relax();
	}

	if (!pipe_dma_chan_done(pipe, chan))
		pr_err("mwv207: wait for completion of DMA timeout: 0x%x",
				pipe_dma_chan_read(pipe, chan, (0x188)));

	pipe_dma_chan_clear(pipe, chan);
	pipe->cur_idx = 0;
}

static void pipe_dma_fill_lli(struct mwv207_pipe_dma *pipe, u64 src, u64 dst, u64 len)
{
	struct mwv207_lli *lli;

	if (pipe->cur_idx >= DMA_MAX_LLI_COUNT)
		pipe_dma_commit(pipe);

	lli = &pipe->lli[pipe->cur_idx];
	lli->sar = src;
	lli->dar = dst;

	if (IS_ALIGNED(src, pipe->alignment) &&
	    IS_ALIGNED(dst, pipe->alignment) &&
	    IS_ALIGNED(len, pipe->alignment)) {
		lli->ts = DMA_TS_ALIGNED(len, pipe->alignment);
		lli->ctl = DMA_CTL_ALIGNED(pipe->alignment);
	} else {
		lli->ts = DMA_TS_NOT_ALIGNED(len);
		lli->ctl = DMA_CTL_NOT_ALIGNED;
	}

#ifdef MWV207_DMA_DEBUG
	{
		pr_info("[dma]: lli[%04d] sar  = 0x%010llx", pipe->cur_idx, lli->sar);
		pr_info("[dma]: lli[%04d] dar  = 0x%010llx", pipe->cur_idx, lli->dar);
		pr_info("[dma]: lli[%04d] ts   = 0x%08x", pipe->cur_idx, lli->ts);
		pr_info("[dma]: lli[%04d] llp  = 0x%010llx", pipe->cur_idx, lli->llp);
		pr_info("[dma]: lli[%04d] ctl  = 0x%010llx", pipe->cur_idx, lli->ctl);
	}
#endif
	pipe->cur_idx++;
}

static void mwv207_cursor_init(struct mwv207_dma_cursor *cursor, struct mwv207_dma_loc *loc,
		u64 width, u64 stride)
{
	cursor->width = width;
	cursor->stride = stride;
	cursor->is_vram = MWV207_DMA_IS_VRAM(loc->pg_nr_type);
	if (cursor->is_vram) {
		cursor->line_pos = loc->base + loc->offset;
		cursor->cur_pos = cursor->line_pos;
		cursor->dma_address = NULL;
		cursor->nr_pages = MWV207_DMA_NR_PAGES(loc->pg_nr_type);
	} else {
		cursor->line_pos = loc->offset;
		cursor->cur_pos = cursor->line_pos;
		cursor->dma_address = (dma_addr_t *)loc->base;
		cursor->nr_pages = MWV207_DMA_NR_PAGES(loc->pg_nr_type);
	}
}

static void mwv207_cursor_reset(struct mwv207_dma_cursor *cursor, struct mwv207_dma_loc *loc)
{
	if (cursor->is_vram) {
		cursor->line_pos = loc->base + loc->offset;
		cursor->cur_pos = cursor->line_pos;
	} else {
		cursor->line_pos = loc->offset;
		cursor->cur_pos = cursor->line_pos;
	}
}

static inline u64 cursor_seg_len(struct mwv207_dma_cursor *cursor)
{
	u64 len = cursor->line_pos + cursor->width - cursor->cur_pos;
	u64 offset, idx, contig_len;

	if (cursor->is_vram)
		return len;
	offset = cursor->cur_pos & (PAGE_SIZE - 1);
	idx = cursor->cur_pos >> PAGE_SHIFT;

	contig_len = PAGE_SIZE - offset;
	while (contig_len < len &&
			idx + 1 < cursor->nr_pages &&
			cursor->dma_address[idx] + PAGE_SIZE == cursor->dma_address[idx + 1]) {
		contig_len += PAGE_SIZE;
		idx++;
	}
	return min_t(u64, len, contig_len);
}

static inline u64 cursor_axi(struct mwv207_dma_cursor *cursor)
{
	u64 idx, offset;

	if (cursor->is_vram)
		return cursor->cur_pos;

	idx = cursor->cur_pos >> PAGE_SHIFT;
	offset = cursor->cur_pos & (PAGE_SIZE - 1);
	return 0x800000000ULL + cursor->dma_address[idx] + offset;
}

static inline void cursor_advance(struct mwv207_dma_cursor *cursor, u64 len)
{
	BUG_ON(cursor->cur_pos + len > cursor->line_pos + cursor->width);

	if (cursor->cur_pos + len == cursor->line_pos + cursor->width) {
		cursor->line_pos += cursor->stride;
		cursor->cur_pos = cursor->line_pos;
	} else
		cursor->cur_pos += len;
}

static void pipe_dma_flush(struct mwv207_pipe_dma *pipe,
		struct mwv207_dma_cursor *cursor, u64 size)
{
	u64 remain, len;

	if (cursor->is_vram || pipe->coherent)
		return;

	for (remain = size; remain > 0; remain -= len) {
		len = min_t(u64, cursor_seg_len(cursor), remain);
		dma_sync_single_for_device(pipe->base.jdev->dev,
				cursor_axi(cursor) - 0x800000000ULL,
				len, DMA_TO_DEVICE);
		cursor_advance(cursor, len);
	}
}

static void pipe_dma_invalidate(struct mwv207_pipe_dma *pipe,
		struct mwv207_dma_cursor *cursor, u64 size)
{
	u64 remain, len;

	if (cursor->is_vram || pipe->coherent)
		return;

	for (remain = size; remain > 0; remain -= len) {
		len = min_t(u64, cursor_seg_len(cursor), remain);
		dma_sync_single_for_cpu(pipe->base.jdev->dev,
				cursor_axi(cursor) - 0x800000000ULL,
				len, DMA_FROM_DEVICE);
		cursor_advance(cursor, len);
	}
}

static struct dma_fence *mwv207_pipe_dma_submit(struct mwv207_pipe *mpipe,
		struct mwv207_job *mjob)
{
	struct mwv207_pipe_dma *pipe = to_dma_pipe(mpipe);
	struct mwv207_dma_cmd *cmd = (struct mwv207_dma_cmd *)mjob->cmds;
	struct mwv207_dma_cursor src, dst;
	u64 remain, len;

	BUG_ON(mjob->cmd_size != sizeof(struct mwv207_dma_cmd));

	mwv207_cursor_init(&src, &cmd->src, cmd->width, cmd->src.stride);
	pipe_dma_flush(pipe, &src, cmd->width * cmd->height);

	mwv207_cursor_reset(&src, &cmd->src);
	mwv207_cursor_init(&dst, &cmd->dst, cmd->width, cmd->dst.stride);
	for (remain = cmd->width * cmd->height; remain > 0; remain -= len) {
		len = min_t(u64, cursor_seg_len(&src), 0x400000);
		len = min_t(u64, cursor_seg_len(&dst), len);
		len = min_t(u64, len, remain);

		pipe_dma_fill_lli(pipe, cursor_axi(&src), cursor_axi(&dst), len);

		cursor_advance(&src, len);
		cursor_advance(&dst, len);
	}
	pipe_dma_commit(pipe);

	mwv207_cursor_reset(&dst, &cmd->dst);
	pipe_dma_invalidate(pipe, &dst, cmd->width * cmd->height);

	return dma_fence_get_stub();
}

static void mwv207_pipe_dma_dump_state(struct mwv207_pipe *mpipe)
{
	struct mwv207_pipe_dma *pipe = to_dma_pipe(mpipe);
	pr_info("%s todo: %p", __func__, pipe);
}

static void mwv207_pipe_dma_destroy(struct mwv207_pipe *mpipe)
{
	struct mwv207_pipe_dma *pipe = to_dma_pipe(mpipe);

	pr_info("%s todo: %p", __func__, pipe);
}

struct mwv207_pipe *mwv207_pipe_dma_create(struct mwv207_device *jdev, int unit, u32 regbase,
		const char *fname)
{
	struct mwv207_pipe_dma *pipe;
	u64 llp;
	int i;

	pipe = devm_kzalloc(jdev->dev, sizeof(struct mwv207_pipe_dma), GFP_KERNEL);
	if (!pipe)
		return NULL;

	pipe->base.jdev = jdev;
	pipe->base.fname = fname;
	pipe->base.regbase = jdev->mmio + regbase;
	pipe->base.iosize = 16 * 1024;
	pipe->base.submit = mwv207_pipe_dma_submit;
	pipe->base.reset  = mwv207_pipe_dma_reset;
	pipe->base.destroy = mwv207_pipe_dma_destroy;
	pipe->base.dump_state = mwv207_pipe_dma_dump_state;

	/* current TTM impl uses ttm_dma_populate which is coherent.
	 * set to false if ttm_populate_and_map_pages is used
	 * */
	pipe->coherent = true;

	pipe->alignment = jdev->lite ? 32 : 64;
	pipe->unit = unit;

	pipe->lli = dmam_alloc_coherent(jdev->dev,
			sizeof(struct mwv207_lli) * DMA_MAX_LLI_COUNT,
			&pipe->lli_bus_addr, GFP_KERNEL);
	if (!pipe->lli)
		return NULL;

	memset(pipe->lli, 0, sizeof(struct mwv207_lli) * DMA_MAX_LLI_COUNT);
	llp = pipe->lli_bus_addr + 0x800000000ULL;
	for (i = 0; i < DMA_MAX_LLI_COUNT; i++) {
		llp += sizeof(struct mwv207_lli);
		pipe->lli[i].llp = llp;
	}
	pipe->cur_idx = 0;

	mwv207_pipe_dma_reset(&pipe->base);
	return &pipe->base;
}

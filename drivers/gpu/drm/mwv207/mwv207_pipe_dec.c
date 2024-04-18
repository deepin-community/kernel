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
#include "mwv207_pipe_codec_common.h"

#define to_dec_pipe(pipe) container_of(pipe, struct mwv207_pipe_dec, base)
struct pipe_dec_fence;
struct mwv207_pipe_dec {
	struct mwv207_pipe base;

	u64 fence_ctx;
	u64 fence_seqno;
	spinlock_t fence_lock;

	struct pipe_dec_fence *current_fence;
	struct task_struct *poll_thread;
	struct pipe_codec_sw_pp sw_pp;
};

#define  to_pipe_dec_fence(f) container_of(f, struct pipe_dec_fence, base)
struct pipe_dec_fence {
	struct dma_fence base;
	struct mwv207_pipe_dec *pipe;
	struct list_head node;
};

static inline void pipe_dec_write(struct mwv207_pipe_dec *pipe, u32 reg, u32 value)
{
	pipe_write(&pipe->base, reg, value);
}

static inline u32 pipe_dec_read(struct mwv207_pipe_dec *pipe, u32 reg)
{
	return pipe_read(&pipe->base, reg);
}

static const char *pipe_dec_fence_get_driver_name(struct dma_fence *fence)
{
	return "mwv207";
}

static const char *pipe_dec_fence_get_timeline_name(struct dma_fence *fence)
{
	return to_pipe_dec_fence(fence)->pipe->base.fname;
}

static void pipe_dec_fence_release(struct dma_fence *fence)
{
	struct pipe_dec_fence *f = to_pipe_dec_fence(fence);

	kfree_rcu(f, base.rcu);
}

static const struct dma_fence_ops pipe_dec_fence_ops = {
	.get_driver_name = pipe_dec_fence_get_driver_name,
	.get_timeline_name = pipe_dec_fence_get_timeline_name,
	.release = pipe_dec_fence_release,
};

static void pipe_dec_stat_reset(struct mwv207_pipe_dec *pipe, u32 stat)
{
	pipe_dec_write(pipe, 0x4, 0x0);
}

static int pipe_dec_polling(void *priv)
{
	struct mwv207_pipe_dec *pipe = priv;
	struct pipe_dec_fence *fence;
	u32 stat;

	set_freezable();
	while (!kthread_should_stop()) {
		usleep_range(1000, 1001);
		try_to_freeze();

		fence = smp_load_acquire(&pipe->current_fence);
		if (fence == NULL)
			continue;

		stat = pipe_dec_read(pipe, 0x4);
		if ((stat & 0x1100) != 0x1100)
			continue;

		pipe_codec_sw_pp_excute(&pipe->sw_pp, &pipe->base);
		pipe_dec_stat_reset(pipe, stat);

		smp_store_release(&pipe->current_fence, NULL);

		dma_fence_signal(&fence->base);
		dma_fence_put(&fence->base);
	}

	return 0;
}

static void mwv207_pipe_dec_reset(struct mwv207_pipe *mpipe)
{
	struct mwv207_pipe_dec *pipe = to_dec_pipe(mpipe);
	struct pipe_dec_fence *fence;
	u32 stat;
	int i;

	pipe_codec_sw_pp_reset(&pipe->sw_pp);

	stat = pipe_dec_read(pipe, 0x4);
	if (stat & 0x1)
		pipe_dec_write(pipe, 0x4, 0x20 | 0x30);

	pipe_dec_write(pipe, 0x4, 0);
	pipe_dec_write(pipe, 0x8, 0x454);

	for (i = 12; i < pipe->base.iosize; i += 0x4)
		pipe_dec_write(pipe, i, 0);

	fence = smp_load_acquire(&pipe->current_fence);
	if (fence) {
		dma_fence_put(&fence->base);

		smp_store_release(&pipe->current_fence, NULL);
	}
}

static struct dma_fence *mwv207_pipe_dec_submit(struct mwv207_pipe *mpipe,
		struct mwv207_job *mjob)
{
	struct mwv207_pipe_dec *pipe = to_dec_pipe(mpipe);
	struct pipe_dec_fence *fence;
	int i, nr, pos, cmdlen;
	bool hw_run = 0;
	u32 cmd, reg;

	fence = smp_load_acquire(&pipe->current_fence);
	if (fence)
		return ERR_PTR(-EBUSY);

	for (pos = 0; pos <= mjob->cmd_size - 4; pos += cmdlen) {
		cmd = *(u32 *)(mjob->cmds + pos);
		switch ((cmd >> 27) & 0x1f) {
		case 0x01:
			if ((cmd >> 26) & 0x1)
				return ERR_PTR(-EINVAL);
			reg = (cmd & 0xffff) * 4;
			nr  = (cmd >> 16) & 0x3ff;
			if (nr == 0)
				nr = 1024;
			for (i = pos + 4; i <= pos + nr * 4
					&& i <= mjob->cmd_size - 4
					&& reg <= pipe->base.iosize - 4;
					i += 4, reg += 4) {
				if (reg == 4) {
					pipe_dec_read(pipe, 0x4);
					hw_run = (*(u32 *)(mjob->cmds + i)) & 0x1;

					mb();
				}
				pipe_dec_write(pipe, reg, *(u32 *)(mjob->cmds + i));
			}

			if (i <= pos + nr * 4)
				dev_dbg(mpipe->jdev->dev,
						"mwv207: invalid dec register writes dropped");

			cmdlen = ALIGN(4 + nr * 4, 8);
			break;
		case 0x02:
			pipe_codec_sw_pp_submit(&pipe->sw_pp, &pipe->base, mjob, pos, &cmdlen);
			break;
		default:

			return ERR_PTR(-EINVAL);
		}
	}

	fence = kzalloc(sizeof(struct pipe_dec_fence), GFP_KERNEL);
	if (!fence)
		return ERR_PTR(-ENOMEM);
	fence->pipe = pipe;
	dma_fence_init(&fence->base, &pipe_dec_fence_ops, &pipe->fence_lock,
		       pipe->fence_ctx, ++pipe->fence_seqno);

	if (hw_run) {
		dma_fence_get(&fence->base);

		smp_store_release(&pipe->current_fence, fence);
	} else {
		pipe_codec_sw_pp_excute(&pipe->sw_pp, &pipe->base);
		dma_fence_signal(&fence->base);
	}

	return &fence->base;
}

static void mwv207_pipe_dec_dump_state(struct mwv207_pipe *mpipe)
{
	pr_info("%s irq state reg: 0x%08x", mpipe->fname,
			pipe_read(mpipe, 0x4));
}

static void mwv207_pipe_dec_destroy(struct mwv207_pipe *mpipe)
{
	struct mwv207_pipe_dec *pipe = to_dec_pipe(mpipe);

	kthread_stop(pipe->poll_thread);
}

struct mwv207_pipe *mwv207_pipe_dec_create(struct mwv207_device *jdev, int unit, u32 regbase,
		const char *fname)
{
	struct mwv207_pipe_dec *pipe;

	if (jdev->lite && unit)
		return NULL;

	pipe = devm_kzalloc(jdev->dev, sizeof(struct mwv207_pipe_dec), GFP_KERNEL);
	if (!pipe)
		return NULL;

	pipe->base.jdev       = jdev;
	pipe->base.fname      = fname;
	pipe->base.regbase    = jdev->mmio + regbase;
	pipe->base.iosize     = JPIPE_DEC_REG_NUM * 4;
	pipe->base.submit     = mwv207_pipe_dec_submit;
	pipe->base.reset      = mwv207_pipe_dec_reset;
	pipe->base.destroy    = mwv207_pipe_dec_destroy;
	pipe->base.dump_state = mwv207_pipe_dec_dump_state;

	spin_lock_init(&pipe->fence_lock);
	pipe->fence_ctx = dma_fence_context_alloc(1);
	pipe->fence_seqno = 0;

	mwv207_pipe_dec_reset(&pipe->base);

	pipe->poll_thread = kthread_run(pipe_dec_polling, pipe, "dec_polling");
	if (IS_ERR(pipe->poll_thread))
		return NULL;

	return &pipe->base;
}

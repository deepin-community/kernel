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
#include "mwv207_vbios.h"
#include "mwv207_devfreq.h"

#define DEFAULT_MAX_FREQ_3D 600

#define to_3d_pipe(pipe) container_of(pipe, struct mwv207_pipe_3d, base)
struct mwv207_pipe_3d {
	struct mwv207_pipe base;
	struct mwv207_bo *ringbuf_bo;
	u32  *ringbuf;
	u32  *head;
	u32  *tail;
	u32  *end;
	u32   ringbuf_gpu_addr;
	int   unit;
	unsigned int irq;

	u64 fence_ctx;
	u32 next_fence;
	u32 completed_fence;
	spinlock_t fence_lock;

	DECLARE_BITMAP(event_bitmap, 30);
	struct dma_fence *event_fence[30];
	spinlock_t event_lock;
};

#define  to_pipe_3d_fence(f) container_of(f, struct pipe_3d_fence, base)
struct pipe_3d_fence {
	struct dma_fence base;
	u32  *rpos;
	struct mwv207_pipe_3d *pipe;
};

static inline void pipe_3d_write(struct mwv207_pipe_3d *pipe, u32 reg, u32 value)
{
	pipe_write(&pipe->base, reg, value);
}

static inline u32 pipe_3d_read(struct mwv207_pipe_3d *pipe, u32 reg)
{
	return pipe_read(&pipe->base, reg);
}

static inline bool fence_after(u32 a, u32 b)
{
	return (s32)(a - b) > 0;
}

static const char *pipe_3d_fence_get_driver_name(struct dma_fence *fence)
{
	return "mwv207";
}

static const char *pipe_3d_fence_get_timeline_name(struct dma_fence *fence)
{
	return to_pipe_3d_fence(fence)->pipe->base.fname;
}

static bool pipe_3d_fence_signaled(struct dma_fence *fence)
{
	struct pipe_3d_fence *f = to_pipe_3d_fence(fence);

	return fence_after(f->pipe->completed_fence, f->base.seqno);
}

static void pipe_3d_fence_release(struct dma_fence *fence)
{
	struct pipe_3d_fence *f = to_pipe_3d_fence(fence);

	kfree_rcu(f, base.rcu);
}

static const struct dma_fence_ops pipe_3d_fence_ops = {
	.get_driver_name = pipe_3d_fence_get_driver_name,
	.get_timeline_name = pipe_3d_fence_get_timeline_name,
	.signaled = pipe_3d_fence_signaled,
	.release = pipe_3d_fence_release,
};

static inline struct dma_fence *pipe_3d_event_pop_irq(struct mwv207_pipe_3d *pipe, u32 event)
{
	struct dma_fence *fence;

	spin_lock(&pipe->event_lock);
	fence = pipe->event_fence[event];
	pipe->event_fence[event] = NULL;
	clear_bit(event, pipe->event_bitmap);
	spin_unlock(&pipe->event_lock);

	return fence;
}

static inline u32 pipe_3d_event_push(struct mwv207_pipe_3d *pipe, struct dma_fence *fence)
{
	u32 event;

	spin_lock_irq(&pipe->event_lock);
	event = find_first_zero_bit(pipe->event_bitmap, 30);
	if (likely(event < 30))
		pipe->event_fence[event] = dma_fence_get(fence);
	set_bit(event, pipe->event_bitmap);
	spin_unlock_irq(&pipe->event_lock);

	BUG_ON(event >= 30);

	return event;
}

static inline u32 pipe_3d_ptr_span(void *end, void *start)
{
	return (unsigned long)end - (unsigned long)start;
}

static inline u32 pipe_3d_gpu_addr(struct mwv207_pipe_3d *pipe, u32 *ptr)
{
	return pipe->ringbuf_gpu_addr + pipe_3d_ptr_span(ptr, pipe->ringbuf);
}

static inline void pipe_3d_link(struct mwv207_pipe_3d *pipe, u32 prefetch, u32 addr)
{
	pipe->tail = PTR_ALIGN(pipe->tail, 8);
	BUG_ON(pipe->tail >= pipe->end - 1
			|| (((prefetch + 7) / 8) & 0xffff0000)
			|| addr & 0x7);
	*pipe->tail++ = 0x40000000 | ((prefetch + 7) / 8);
	*pipe->tail++ = addr;
}

static inline void pipe_3d_wait(struct mwv207_pipe_3d *pipe)
{
	pipe->tail = PTR_ALIGN(pipe->tail, 8);
	BUG_ON(pipe->tail >= pipe->end);

	*pipe->tail++ = 0x38000000 | 200;
}

static inline void pipe_3d_wl(struct mwv207_pipe_3d *pipe)
{
	pipe_3d_wait(pipe);
	pipe_3d_link(pipe, 16, pipe_3d_gpu_addr(pipe, pipe->tail) - 4);
}

static inline void pipe_3d_wtol(struct mwv207_pipe_3d *pipe, u32 *w, u32 target, u32 size)
{
	BUG_ON((((size + 7) / 8) & 0xffff0000) || target & 0x7
			|| w < pipe->ringbuf || w >= pipe->end - 1);
	w[1] = target;
	pipe_3d_read(pipe, 0x4);
	mb();
	w[0] = 0x40000000 | ((size + 7) / 8);
}

static inline void pipe_3d_loadstate(struct mwv207_pipe_3d *pipe, u32 reg, u32 val)
{
	pipe->tail = PTR_ALIGN(pipe->tail, 8);
	BUG_ON(pipe->tail >= pipe->end - 1);

	*pipe->tail++ = 0x08010000 | reg;
	*pipe->tail++ = val;
}

static inline void pipe_3d_sem(struct mwv207_pipe_3d *pipe, u32 from, u32 to)
{
	from &= 0x1f;
	to   &= 0x1f;
	to  <<= 8;
	pipe_3d_loadstate(pipe, 0x0e02, from | to);
}

static inline void pipe_3d_stall(struct mwv207_pipe_3d *pipe, u32 from, u32 to)
{
	pipe->tail = PTR_ALIGN(pipe->tail, 8);
	BUG_ON(pipe->tail >= pipe->end - 1);
	from &= 0x1f;
	to   &= 0x1f;
	to  <<= 8;
	*pipe->tail++ = 0x48000000;
	*pipe->tail++ = from | to;
}

static inline void pipe_3d_start(struct mwv207_pipe_3d *pipe)
{
	u32 *start = pipe->tail;
	u32 len;

	dev_dbg(pipe->base.jdev->dev, "start wait link at 0x%x", pipe->ringbuf_gpu_addr);
	pipe_3d_loadstate(pipe, 0x0e21, 0x00020202);
	pipe_3d_loadstate(pipe, 0x0e44, 0x0000000f);
	pipe_3d_loadstate(pipe, 0x0e80, 0x00000000);
	pipe_3d_loadstate(pipe, 0x7003, 0x00000001);

	pipe_3d_wl(pipe);

	len = (pipe->tail - start) * 4;
	pipe_3d_write(pipe, 0x014, 0xffffffff);
	pipe_3d_write(pipe, 0x654, pipe->ringbuf_gpu_addr);
	pipe_3d_read(pipe, 0x4);
	mb();
	pipe_3d_write(pipe, 0x658, 0x10000 | (len / 8));
	pipe_3d_write(pipe, 0x3a4, 0x10000 | (len / 8));
}

static inline int pipe_3d_stop(struct mwv207_pipe_3d *pipe)
{
	pr_info("%s TBD", __func__);
	return 0;
}

static void pipe_3d_core_reset(struct mwv207_pipe_3d *pipe)
{
	int i;

	for (i = 0; i < 2; i++) {

		pipe_3d_write(pipe, 0x104, 0);

		pipe_3d_write(pipe, 0x10c, 0x015b0880);
		pipe_3d_write(pipe, 0x10c, 0x015b0881);
		pipe_3d_write(pipe, 0x10c, 0x015b0880);

		pipe_3d_write(pipe, 0x000, 0x00010b00);
		pipe_3d_write(pipe, 0x000, 0x00010900);

		usleep_range(1000, 1001);

		pipe_3d_write(pipe, 0x000, 0x00090900);

		pipe_3d_write(pipe, 0x3a8, 1);
		pipe_3d_write(pipe, 0x3a8, 0);

		usleep_range(1000, 1001);

		pipe_3d_write(pipe, 0x000, 0x00010900);
	}
}

static void pipe_3d_hw_init(struct mwv207_pipe_3d *pipe)
{
	pipe_3d_write(pipe, 0x000, 0x00010900);
	pipe_3d_core_reset(pipe);

	pipe_3d_write(pipe, 0x55c, 0x00ffffff);
	pipe_3d_write(pipe, 0x414, 0x3c000000);
	pipe_3d_write(pipe, 0x090, pipe_3d_read(pipe, 0x090) & 0xffffffbf);

	pipe_3d_write(pipe, 0x000, 0x00010900);
	pipe_3d_write(pipe, 0x000, 0x00070100);
	pipe_3d_write(pipe, 0x3ac, 0x00002300);
	pipe_3d_write(pipe, 0x3a8, 0x2);

	pipe_3d_write(pipe, 0x03c, 0xffffffff);
	pipe_3d_write(pipe, 0x03c, 0x00000000);

	pipe_3d_write(pipe, 0x100, 0x00140021);

	pipe_3d_write(pipe, 0x154, 0x1);
	pipe_3d_write(pipe, 0x104, 0x00430408);
	pipe_3d_write(pipe, 0x10c, 0x015b0880);
}

static void mwv207_pipe_3d_reset(struct mwv207_pipe *mpipe)
{
	struct mwv207_pipe_3d *pipe = to_3d_pipe(mpipe);
	int i;

	spin_lock_irq(&pipe->event_lock);
	bitmap_zero(pipe->event_bitmap, 30);
	for (i = 0; i < 30; i++) {
		dma_fence_put(pipe->event_fence[i]);
		pipe->event_fence[i] = NULL;
	}
	spin_unlock_irq(&pipe->event_lock);

	pipe_3d_hw_init(pipe);
	pipe->tail = pipe->ringbuf;
	pipe->head = pipe->ringbuf;

	pipe_3d_start(pipe);
}

static u32 *pipe_3d_wait_for_space(struct mwv207_pipe_3d *pipe, u32 size)
{
	u32 *head;
	int i;

	for (i = 0; i < 10000; ++i) {
		head = READ_ONCE(pipe->head);
		if (head <= pipe->tail) {
			if (pipe_3d_ptr_span(pipe->end, pipe->tail) >= size)
				return pipe->tail;
			if (pipe_3d_ptr_span(head, pipe->ringbuf) >= size)
				return pipe->ringbuf;
		} else {
			if (pipe_3d_ptr_span(head, pipe->tail) >= size)
				return pipe->tail;
		}
		usleep_range(200, 200);
	}
	return NULL;
}

static struct dma_fence *mwv207_pipe_3d_submit(struct mwv207_pipe *mpipe,
		struct mwv207_job *mjob)
{
	struct mwv207_pipe_3d *pipe = to_3d_pipe(mpipe);
	struct pipe_3d_fence *fence;
	u32 *last_tail, *start;
	u32 size, event;

	size = ALIGN(mjob->cmd_size, 8) + 256;
	last_tail  = pipe->tail;
	start = pipe_3d_wait_for_space(pipe, size);
	if (start == NULL) {
		pr_err("mwv207: engine 3d is stuck");

		return ERR_PTR(-EBUSY);
	}
	memcpy_toio(start, mjob->cmds, mjob->cmd_size);
	pipe->tail = start + mjob->cmd_size / 4;

	pipe_3d_loadstate(pipe, 0x0e03, 0xc63);
	pipe_3d_loadstate(pipe, 0x594, 0x41);

	pipe_3d_sem(pipe, 0x7, 0x1);
	pipe_3d_stall(pipe, 0x7, 0x1);
	pipe_3d_loadstate(pipe, 0x502e, 0x1);
	pipe_3d_loadstate(pipe, 0x502b, 0x3);
	pipe_3d_sem(pipe, 0x10, 0x1);
	pipe_3d_stall(pipe, 0x10, 0x1);
	pipe_3d_loadstate(pipe, 0x502e, 0x0);

	fence = kzalloc(sizeof(struct pipe_3d_fence), GFP_KERNEL);
	if (!fence) {
		pipe->tail = last_tail;
		pr_err("mwv207: failed to alloc memory for 3d pipe");

		return ERR_PTR(-ENOMEM);
	}
	dma_fence_init(&fence->base, &pipe_3d_fence_ops, &pipe->fence_lock,
		       pipe->fence_ctx, ++pipe->next_fence);
	fence->rpos = pipe->tail;
	fence->pipe = pipe;
	event = pipe_3d_event_push(pipe, &fence->base);

	pipe_3d_loadstate(pipe, 0x502e, 0x1);
	pipe_3d_loadstate(pipe, 0x50ce, 0xf);
	pipe_3d_loadstate(pipe, 0x0e01, 0x80 | event);
	pipe_3d_loadstate(pipe, 0x502e, 0x0);

	pipe_3d_wl(pipe);
	BUG_ON(pipe_3d_ptr_span(pipe->tail, start) > size);
	pipe_3d_wtol(pipe, last_tail - 4, pipe_3d_gpu_addr(pipe, start),
			pipe_3d_ptr_span(pipe->tail, start));
	return &fence->base;
}

static void mwv207_pipe_3d_dump_state(struct mwv207_pipe *mpipe)
{
	u32 state0, addr0;
	u32 state1, addr1;
	int i;

	state0 = pipe_read(mpipe, 0x660);
	addr0  = pipe_read(mpipe, 0x664);
	for (i = 0; i < 500; ++i) {
		state1 = pipe_read(mpipe, 0x660);
		addr1  = pipe_read(mpipe, 0x664);
		if (state1 != state0 || addr1 != addr0)
			break;
	}

	if (state1 != state0)
		pr_info("%s state changing, s0: 0x%08x, s1: 0x%08x", mpipe->fname, state0, state1);
	pr_info("%s current stat: 0x%08x", mpipe->fname, pipe_read(mpipe, 0x660));

	if (addr1 != addr0)
		pr_info("%s addr changing, a0: 0x%08x, a1: 0x%08x", mpipe->fname, addr0, addr1);
	pr_info("%s current addr: 0x%08x", mpipe->fname, pipe_read(mpipe, 0x664));

	pr_info("%s current cmdl: 0x%08x", mpipe->fname, pipe_read(mpipe, 0x668));
	pr_info("%s current cmdh: 0x%08x", mpipe->fname, pipe_read(mpipe, 0x66c));
}

static irqreturn_t mwv207_pipe_3d_isr(int irq_unused, void *dev_id)
{
	struct mwv207_pipe_3d *pipe = dev_id;
	struct dma_fence *fence;
	u32 event, intr;

	intr = pipe_3d_read(pipe, 0x10);
	if (unlikely(!intr)) {
		pr_warn("mwv207: 3d interrupt with no event");
		return IRQ_NONE;
	}
	if (unlikely(intr & 0x80000000)) {
		pr_warn("mwv207: 3d axi bus error");
		intr &= ~0x80000000;
	}
	if (unlikely(intr & 0x40000000)) {
		pr_warn("mwv207: 3d mmu error");
		intr &= ~0x40000000;
	}
	while ((event = ffs(intr))) {
		event -= 1;
		intr  &= ~(1 << event);

		fence = pipe_3d_event_pop_irq(pipe, event);
		if (!fence) {
			pr_warn("unexpected event: %d", event);
			continue;
		}

		if (fence_after(fence->seqno, pipe->completed_fence)) {
			struct pipe_3d_fence *pipe_fence = to_pipe_3d_fence(fence);

			pipe->completed_fence = fence->seqno;
			pipe->head = pipe_fence->rpos;
		}
		mwv207_devfreq_record_idle(&pipe->base);
		dma_fence_signal(fence);
		dma_fence_put(fence);
	}
	return IRQ_HANDLED;
}

static void mwv207_pipe_3d_destroy(struct mwv207_pipe *mpipe)
{
	struct mwv207_pipe_3d *pipe = to_3d_pipe(mpipe);
	struct mwv207_bo *jbo;
	int ret;

	free_irq(pipe->irq, pipe);
	pipe_3d_stop(pipe);

	jbo = pipe->ringbuf_bo;
	ret = mwv207_bo_reserve(jbo, true);
	if (ret) {
		pr_info("failed to reserve bo");
		return;
	}
	mwv207_bo_kunmap_reserved(jbo);
	mwv207_bo_unpin_reserved(jbo);
	mwv207_bo_unreserve(jbo);
	mwv207_bo_unref(jbo);

	mwv207_devfreq_unregister(mpipe);
}

static unsigned long pipe_3d_max_freq_get(struct mwv207_device *jdev)
{
	unsigned long freq;

	switch (jdev->pdev->subsystem_device)
	{
	case 0x9103:
		freq = 600;
		break;
	case 0x9210:
	case 0x9211:
		freq = 800;
		break;
	case 0x9101:
	case 0x9102:
	case 0x910A:
	case 0x910B:
	case 0x910C:
	case 0x920C:
	case 0x930B:
		freq = 1000;
		break;
	case 0x9200:
	case 0x920A:
	case 0x920B:
	case 0x9230:
	case 0x9231:
	case 0x930A:
	case 0x930C:
		freq = 1200;
		break;
	default:
		freq = DEFAULT_MAX_FREQ_3D;
		break;
	}

	return freq;
}

struct mwv207_pipe *mwv207_pipe_3d_create(struct mwv207_device *jdev, int unit, u32 regbase,
		const char *fname)
{
	struct mwv207_pipe_3d *pipe;
	struct mwv207_bo *jbo;
	void *logical;
	int ret;

	if (jdev->lite && unit)
		return NULL;

	pipe = devm_kzalloc(jdev->dev, sizeof(struct mwv207_pipe_3d), GFP_KERNEL);
	if (!pipe)
		return NULL;

	pipe->base.pll_id = unit ? MWV207_PLL_GU3D1 : MWV207_PLL_GU3D0;
	pipe->base.jdev = jdev;
	pipe->base.fname = fname;
	pipe->base.regbase = jdev->mmio + regbase;
	pipe->base.iosize = 16 * 1024;
	pipe->base.submit = mwv207_pipe_3d_submit;
	pipe->base.reset  = mwv207_pipe_3d_reset;
	pipe->base.destroy = mwv207_pipe_3d_destroy;
	pipe->base.dump_state = mwv207_pipe_3d_dump_state;

	pipe->unit = unit;
	pipe->fence_ctx = dma_fence_context_alloc(1);
	spin_lock_init(&pipe->fence_lock);
	spin_lock_init(&pipe->event_lock);

	ret = mwv207_devfreq_register(&pipe->base, pipe_3d_max_freq_get(jdev));
	if (ret)
		pipe->base.devfreq = NULL;

	ret = mwv207_bo_create(jdev, 0x40000, 0x1000,
			ttm_bo_type_kernel, 0x2, (1<<0),
			&jbo);
	if (ret)
		goto unregister_devfreq;

	ret = mwv207_bo_reserve(jbo, true);
	if (ret)
		goto free_bo;

	ret = mwv207_bo_pin_reserved(jbo, 0x2);
	if (ret)
		goto unreserve_bo;

	ret = mwv207_bo_kmap_reserved(jbo, &logical);
	if (ret)
		goto unpin_bo;

	pipe->ringbuf_bo = jbo;

	pipe->ringbuf = (u32 *)logical;
	pipe->ringbuf_gpu_addr = mwv207_bo_gpu_phys(jbo);
	pipe->end  = pipe->ringbuf + (0x40000 / 4);

	mwv207_pipe_3d_reset(&pipe->base);

	pipe->irq = irq_find_mapping(jdev->irq_domain, 19 + unit);
	if (pipe->irq == 0) {
		pr_err("mwv207: failed to create 3D irq mapping");
		goto unmap_bo;
	}

	ret = request_irq(pipe->irq, mwv207_pipe_3d_isr, 0, fname, pipe);
	if (ret) {
		pr_err("mwv207: failed to request 3d irq");
		goto unmap_bo;
	}

	mwv207_bo_unreserve(jbo);
	return &pipe->base;
unmap_bo:
	mwv207_bo_kunmap_reserved(jbo);
unpin_bo:
	mwv207_bo_unpin_reserved(jbo);
unreserve_bo:
	mwv207_bo_unreserve(jbo);
free_bo:
	mwv207_bo_unref(pipe->ringbuf_bo);
unregister_devfreq:
	mwv207_devfreq_unregister(&pipe->base);
	return NULL;
}

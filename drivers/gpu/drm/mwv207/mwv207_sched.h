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
#ifndef MWV207_SCHED_H_TGQQ2LXS
#define MWV207_SCHED_H_TGQQ2LXS
#include <drm/gpu_scheduler.h>
#include <drm/ttm/ttm_execbuf_util.h>
#include <linux/list.h>
#include "mwv207.h"
#include "mwv207_bo.h"

struct mwv207_submit_cmd_dat {
	int nr_relocs;
	int dat_size;
	struct drm_mwv207_submit_reloc *relocs;
	char *dat;
	struct mwv207_bo *jbo;
};

struct mwv207_tvb {
	struct ttm_validate_buffer base;
	struct dma_fence *excl;
	unsigned int nr_shared;
	struct dma_fence **shared;
};

#define to_mwv207_job(job) container_of(job, struct mwv207_job, base)
struct mwv207_job {
	struct drm_sched_job base;

	struct list_head tvblist;
	struct mwv207_tvb *mtvb;
	int nr_bos;

	struct drm_mwv207_submit_reloc *relocs;
	int nr_relocs;

	struct mwv207_submit_cmd_dat *cmd_dats;
	int nr_cmd_dat;

	char *cmds;
	int   cmd_size;

	struct dma_fence *fence_in;

	struct mwv207_ctx *ctx;
	struct drm_sched_entity *engine_entity;

	struct kref refcount;

	bool is_dma;

	char comm[TASK_COMM_LEN];
};
#define mwv207_for_each_mtvb(mtvb, mjob) list_for_each_entry(mtvb, &(mjob)->tvblist, base.head)

struct mwv207_job *mwv207_job_alloc(void);
struct mwv207_job *mwv207_job_get(struct mwv207_job *mjob);
void mwv207_job_put(struct mwv207_job *mjob);

static inline struct mwv207_bo *mwv207_job_bo(struct mwv207_job *mjob, int idx)
{
	BUG_ON(idx >= mjob->nr_bos + mjob->nr_cmd_dat);
	return to_jbo(mjob->mtvb[idx].base.bo);
}

struct mwv207_dma_loc {
	u64 base;
	u64 pg_nr_type;
	u64 offset;
	u64 stride;
};
#define MWV207_DMA_NR_PAGES_VRAM(nr) ((nr) | (0ULL << 63))
#define MWV207_DMA_NR_PAGES_RAM(nr)  ((nr) | (1ULL << 63))
#define MWV207_DMA_NR_PAGES(nr)      ((nr) & ~(1ULL << 63))
#define MWV207_DMA_IS_VRAM(nr) (((nr) >> 63) == 0)

struct mwv207_dma_cmd {
	struct mwv207_dma_loc src;
	struct mwv207_dma_loc dst;
	u64 width;
	u64 height;
};

struct devfreq;
#define to_mwv207_sched(sched) container_of(sched, struct mwv207_sched, base)
struct mwv207_pipe {
	struct dma_fence*  (*submit)(struct mwv207_pipe *pipe, struct mwv207_job *job);
	void               (*reset)(struct mwv207_pipe *pipe);
	void               (*destroy)(struct mwv207_pipe *pipe);
	void               (*dump_state)(struct mwv207_pipe *pipe);
	struct mwv207_device   *jdev;
	const char             *fname;
	int                    iosize;
	void __iomem           *regbase;

	struct device  *dev;
	struct devfreq *devfreq;
	spinlock_t devfreq_lock;
	int busy_count;
	int pll_id;
	ktime_t busy_time;
	ktime_t idle_time;
	ktime_t time_last_update;
};

struct mwv207_sched {
	struct drm_gpu_scheduler base;
	struct mwv207_pipe *pipe;
};

static inline void pipe_write(struct mwv207_pipe *pipe, u32 reg, u32 value)
{
	writel_relaxed(value, pipe->regbase + reg);
}

static inline u32 pipe_read(struct mwv207_pipe *pipe, u32 reg)
{
	return	readl(pipe->regbase + reg);
}

int  mwv207_sched_init(struct mwv207_device *jdev);
void mwv207_sched_fini(struct mwv207_device *jdev);
int  mwv207_sched_suspend(struct mwv207_device *jdev);
int  mwv207_sched_resume(struct mwv207_device *jdev);

struct mwv207_pipe *mwv207_pipe_3d_create(struct mwv207_device *jdev, int unit,
		u32 regbase, const char *fname);
struct mwv207_pipe *mwv207_pipe_2d_create(struct mwv207_device *jdev, int unit,
		u32 regbase, const char *fname);
struct mwv207_pipe *mwv207_pipe_dec_create(struct mwv207_device *jdev, int unit,
		u32 regbase, const char *fname);
struct mwv207_pipe *mwv207_pipe_enc_create(struct mwv207_device *jdev,
		u32 regbase, const char *fname);
struct mwv207_pipe *mwv207_pipe_dma_create(struct mwv207_device *jdev, int unit,
		u32 regbase, const char *fname);
#endif

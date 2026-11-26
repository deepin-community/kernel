// SPDX-License-Identifier: GPL-2.0+
/*
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
#include <linux/moduleparam.h>
#include <drm/gpu_scheduler.h>
#include "mwv207.h"
#include "mwv207_sched.h"
#include "mwv207_bo.h"
#include "mwv207_ctx.h"
#include "mwv207_db.h"
#include "mwv207_devfreq.h"

static int dump_hang_job = 1;
module_param(dump_hang_job, int, 0644);
MODULE_PARM_DESC(dump_hang_job, "dump time out job");

struct mwv207_job *mwv207_job_alloc(void)
{
	struct mwv207_job *mjob;

	mjob = kzalloc(sizeof(*mjob), GFP_KERNEL);
	if (!mjob)
		return NULL;

	kref_init(&mjob->refcount);
	INIT_LIST_HEAD(&mjob->tvblist);
	return mjob;
}

static void mwv207_job_fini(struct kref *kref)
{
	struct mwv207_job *mjob = container_of(kref, struct mwv207_job, refcount);
	struct mwv207_tvb *mtvb;
	struct mwv207_submit_cmd_dat *dat;
	int i;

	dma_fence_put(mjob->fence_in);
	if (mjob->base.s_fence)
		drm_sched_job_cleanup(&mjob->base);

	mwv207_for_each_mtvb(mtvb, mjob) {
		for (i = 0; i < mtvb->nr_shared; ++i)
			dma_fence_put(mtvb->shared[i]);
		dma_fence_put(mtvb->excl);
		kfree(mtvb->shared);
		mwv207_bo_unref(to_jbo(mtvb->base.bo));
	}

	if (mjob->nr_cmd_dat) {
		for (i = 0; i < mjob->nr_cmd_dat; ++i) {
			dat = &mjob->cmd_dats[i];
			kvfree(dat->dat);
			kvfree(dat->relocs);
			mwv207_bo_unref(dat->jbo);
		}
	}

	if (mjob->ctx)
		mwv207_ctx_put(mjob->ctx);

	kvfree(mjob->cmd_dats);
	kvfree(mjob->mtvb);
	kvfree(mjob->relocs);
	kvfree(mjob->cmds);

	kfree(mjob);
}

struct mwv207_job *mwv207_job_get(struct mwv207_job *mjob)
{
	kref_get(&mjob->refcount);
	return mjob;
}

void mwv207_job_put(struct mwv207_job *mjob)
{
	kref_put(&mjob->refcount, mwv207_job_fini);
}

static void mwv207_sched_dump_job(struct mwv207_job *mjob)
{
	int lines, i;
	u32 *cmd;

	cmd = (u32 *)mjob->cmds;
	lines = (mjob->cmd_size / 4) & ~0x3;
	for (i = 0; i < lines; i += 4) {
		pr_info("0x%08x : %08x %08x %08x %08x", i,
				cmd[i], cmd[i+1], cmd[i+2], cmd[i+3]);
	}

	switch (mjob->cmd_size / 4 - lines) {
	case 0:
		return;
	case 1:
		pr_info("0x%08x : %08x", i, cmd[i]);
		break;
	case 2:
		pr_info("0x%08x : %08x %08x", i, cmd[i], cmd[i+1]);
		break;
	case 3:
		pr_info("0x%08x : %08x %08x %08x", i,
				cmd[i], cmd[i+1], cmd[i+2]);
		break;
	default:
		pr_err("should never happen");
		break;

	}
}

static struct dma_fence *mwv207_sched_dependency(struct drm_sched_job *job,
					       struct drm_sched_entity *entity)
{
	struct mwv207_job *mjob = to_mwv207_job(job);
	struct mwv207_tvb *mtvb;
	struct dma_fence *fence;
	int i;

	if (unlikely(mjob->fence_in)) {
		fence = mjob->fence_in;
		mjob->fence_in = NULL;
		if (!dma_fence_is_signaled(fence))
			return fence;
		dma_fence_put(fence);
	}

	mwv207_for_each_mtvb(mtvb, mjob) {
		if (mtvb->excl) {
			fence = mtvb->excl;
			mtvb->excl = NULL;
			if (!dma_fence_is_signaled(fence))
				return fence;
			dma_fence_put(fence);
		}

		for (i = 0; i < mtvb->nr_shared; i++) {
			if (!mtvb->shared[i])
				continue;
			fence = mtvb->shared[i];
			mtvb->shared[i] = NULL;
			if (!dma_fence_is_signaled(fence))
				return fence;
			dma_fence_put(fence);
		}
		kfree(mtvb->shared);
		mtvb->nr_shared = 0;
		mtvb->shared = NULL;
	}

	return NULL;
}

static struct dma_fence *mwv207_sched_run_job(struct drm_sched_job *job)
{
	struct mwv207_sched *sched = to_mwv207_sched(job->sched);
	struct dma_fence *fence;

	if (unlikely(job->s_fence->finished.error))
		return ERR_PTR(-ECANCELED);

	mwv207_devfreq_record_busy(sched->pipe);

	fence = sched->pipe->submit(sched->pipe, to_mwv207_job(job));
	if (IS_ERR_OR_NULL(fence))
		DRM_ERROR("%s: submit failed, errcode=%ld",
				sched->pipe->fname, PTR_ERR(fence));
	return fence;
}

static enum drm_gpu_sched_stat mwv207_sched_timedout_job(struct drm_sched_job *job)
{
	struct mwv207_sched *sched = to_mwv207_sched(job->sched);
	struct mwv207_job *mjob = to_mwv207_job(job);
	struct drm_sched_job *timedout_job;

	pr_info("mwv207: job from '%s' to '%s', timed out!",
			mjob->comm, sched->pipe->fname);

	drm_sched_stop(&sched->base, job);

	if (dump_hang_job) {
		sched->pipe->dump_state(sched->pipe);
		mwv207_sched_dump_job(mjob);
	}

	drm_sched_increase_karma(job);

	sched->pipe->reset(sched->pipe);

	list_for_each_entry(timedout_job, &sched->base.pending_list, list)
		mwv207_devfreq_record_idle(sched->pipe);

	drm_sched_resubmit_jobs(&sched->base);

	drm_sched_start(&sched->base, true);

	return DRM_GPU_SCHED_STAT_NOMINAL;
}

static void mwv207_sched_free_job(struct drm_sched_job *job)
{
	struct mwv207_job *mjob = to_mwv207_job(job);

	mwv207_job_put(mjob);
}

static const struct drm_sched_backend_ops mwv207_sched_ops = {
	.prepare_job = mwv207_sched_dependency,
	.run_job = mwv207_sched_run_job,
	.timedout_job = mwv207_sched_timedout_job,
	.free_job = mwv207_sched_free_job,
};

static struct drm_gpu_scheduler *mwv207_sched_create(struct mwv207_device *jdev,
		unsigned int hw_submission, unsigned int hang_limit, long timeout,
		struct mwv207_pipe *pipe)
{
	struct mwv207_sched *sched;
	int ret;

	if (pipe == NULL)
		return NULL;

	sched = devm_kzalloc(jdev->dev, sizeof(struct mwv207_sched), GFP_KERNEL);
	if (!sched)
		goto err;

	sched->pipe = pipe;

	ret = drm_sched_init(&sched->base, &mwv207_sched_ops, hw_submission, hang_limit,
			msecs_to_jiffies(timeout),
			NULL, NULL,
			pipe->fname, jdev->dev);
	if (ret)
		goto err;

	return &sched->base;
err:
	sched->pipe->destroy(sched->pipe);
	return NULL;
}

static void mwv207_sched_destroy(struct drm_gpu_scheduler *sched)
{
	struct mwv207_sched *msched = to_mwv207_sched(sched);

	if (sched == NULL)
		return;

	drm_sched_fini(sched);
	msched->pipe->destroy(msched->pipe);
}

int mwv207_sched_suspend(struct mwv207_device *jdev)
{
	struct mwv207_sched *sched;
	int i, ret;

	for (i = 0; i < 6; i++) {
		if (!jdev->sched[i])
			continue;
		sched = to_mwv207_sched(jdev->sched[i]);

		if (atomic_read(&jdev->sched[i]->hw_rq_count))
			return -EBUSY;

		ret = mwv207_devfreq_suspend(sched->pipe);
		if (ret)
			return ret;
	}

	return 0;
}

int mwv207_sched_resume(struct mwv207_device *jdev)
{
	struct mwv207_sched *sched;
	int i, ret;

	for (i = 0; i < 6; i++) {
		if (!jdev->sched[i])
			continue;
		sched = to_mwv207_sched(jdev->sched[i]);

		ret = mwv207_devfreq_resume(sched->pipe);
		if (ret)
			dev_err(sched->pipe->dev, "devfreq resume failed: %d", ret);

		sched->pipe->reset(sched->pipe);
	}

	return 0;
}

int mwv207_sched_init(struct mwv207_device *jdev)
{
	struct mwv207_pipe *pipe;

	jdev->sched_3d  = &jdev->sched[0];
	jdev->sched_dec = &jdev->sched_3d[2];
	jdev->sched_enc = &jdev->sched_dec[1];
	jdev->sched_2d  = &jdev->sched_enc[1];
	jdev->sched_dma = &jdev->sched_2d[1];

	pipe = mwv207_pipe_3d_create(jdev, 0, 0x900000, "mwv207_3d0");
	jdev->sched_3d[0] = mwv207_sched_create(jdev, 30, 5, 2000, pipe);
	if (jdev->sched_3d[0] == NULL)
		return -ENODEV;

	pipe = mwv207_pipe_3d_create(jdev, 1, 0x910000, "mwv207_3d1");
	jdev->sched_3d[1] = mwv207_sched_create(jdev, 30, 5, 2000, pipe);

	pipe = mwv207_pipe_dec_create(jdev, 0, 0x930000, "mwv207_dec0");
	jdev->sched_dec[0] = mwv207_sched_create(jdev, 1, 5, 2000, pipe);
	if (jdev->sched_dec[0] == NULL)
		goto err;

	pipe = mwv207_pipe_dec_create(jdev, 1, 0x940000, "mwv207_dec1");
	jdev->sched_dec[1] = mwv207_sched_create(jdev, 1, 5, 2000, pipe);

	pipe = mwv207_pipe_enc_create(jdev, 0x920000, "mwv207_enc0");
	jdev->sched_enc[0] = mwv207_sched_create(jdev, 1, 5, 2000, pipe);

	pipe = mwv207_pipe_2d_create(jdev, 0, 0x968000, "mwv207_2d0");
	jdev->sched_2d[0] = mwv207_sched_create(jdev, 30, 5, 2000, pipe);
	if (jdev->sched_2d[0] == NULL)
		goto err;

	pipe = mwv207_pipe_dma_create(jdev, 0, 0x9d0000, "mwv207_dma0");
	jdev->sched_dma[0] = mwv207_sched_create(jdev, 1, 5, 2000, pipe);
	if (jdev->sched_dma[0] == NULL)
		goto err;

	jdev->nr_3d  = jdev->sched_3d[1] ? 2 : 1;
	jdev->nr_enc = jdev->sched_enc[0] ? 1 : 0;
	jdev->nr_dec = jdev->sched_dec[1] ? 2 : 1;
	jdev->nr_2d  = 1;
	jdev->nr_dma = 1;

	mwv207_db_add(jdev, DRM_MWV207_ACTIVE_3D_NR, jdev->nr_3d);
	mwv207_db_add(jdev, DRM_MWV207_ACTIVE_ENC_NR, jdev->nr_enc);
	mwv207_db_add(jdev, DRM_MWV207_ACTIVE_DEC_NR, jdev->nr_dec);
	mwv207_db_add(jdev, DRM_MWV207_ACTIVE_2D_NR, jdev->nr_2d);
	mwv207_db_add(jdev, DRM_MWV207_ACTIVE_DMA_NR, jdev->nr_dma);

	return 0;
err:
	mwv207_sched_fini(jdev);
	return -ENODEV;
}

void mwv207_sched_fini(struct mwv207_device *jdev)
{
	int i;

	for (i = 0; i < 6; ++i) {
		if (!jdev->sched[i])
			continue;
		mwv207_sched_destroy(jdev->sched[i]);
	}
}

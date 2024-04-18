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
#include <drm/drm_file.h>
#include <drm/drm_ioctl.h>
#include <drm/gpu_scheduler.h>
#include "mwv207.h"
#include "mwv207_drm.h"
#include "mwv207_ctx.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
struct mwv207_ctx *mwv207_ctx_lookup(struct drm_device *dev,
		struct drm_file *filp, u32 handle)
{
	struct mwv207_fpriv *fpriv = to_fpriv(filp);
	struct mwv207_ctx_mgr *mgr = &fpriv->ctx_mgr;
	struct mwv207_ctx *ctx;

	mutex_lock(&mgr->lock);
	ctx = idr_find(&mgr->handle_table, handle);
	if (ctx)
		kref_get(&ctx->refcnt);
	mutex_unlock(&mgr->lock);

	return ctx;
}

static void mwv207_ctx_entity_fini(struct mwv207_ctx *ctx)
{
	int i;

	for (i = 0; i < 11; i++) {
		if (ctx->entities[i]) {
			drm_sched_entity_destroy(ctx->entities[i]);
			kfree(ctx->entities[i]);
			ctx->entities[i] = NULL;
		}
	}
}

static int mwv207_ctx_entity_init_single(struct mwv207_ctx *ctx,
		struct drm_gpu_scheduler **sched,
		struct drm_sched_entity **entities, int nr)
{
	struct drm_gpu_scheduler *scheds[6];
	struct drm_sched_entity *entity;
	int i, ret;

	for (i = 0; i < nr; i++) {
		scheds[i] = sched[i];

		entity = kmalloc(sizeof(struct drm_sched_entity), GFP_KERNEL);
		if (!entity)
			return -ENOMEM;

		ret = drm_sched_entity_init(entity, DRM_SCHED_PRIORITY_NORMAL, &scheds[i], 1, &ctx->guilty);

		if (ret) {
			kfree(entity);
			return ret;
		}
		entities[i] = entity;
	}
	if (i) {

		entity = kmalloc(sizeof(struct drm_sched_entity), GFP_KERNEL);
		if (!entity)
			return -ENOMEM;

		ret = drm_sched_entity_init(entity, DRM_SCHED_PRIORITY_NORMAL, &scheds[0], i, &ctx->guilty);

		if (ret) {
			kfree(entity);
			return ret;
		}
		entities[i] = entity;
	}

	return 0;
}

static int mwv207_ctx_entity_init(struct drm_device *dev, struct mwv207_ctx *ctx)
{
	struct mwv207_device *jdev = ddev_to_jdev(dev);
	int ret;

	ctx->entity_3d  = &ctx->entities[0];
	ctx->entity_dec = &ctx->entity_3d[2 + 1];
	ctx->entity_enc = &ctx->entity_dec[1 + 1];
	ctx->entity_2d  = &ctx->entity_enc[1 + 1];
	ctx->entity_dma = &ctx->entity_2d[1 + 1];
	BUG_ON(ctx->entity_dma[1] != ctx->entities[11 - 1]);

	ret = mwv207_ctx_entity_init_single(ctx, jdev->sched_3d, ctx->entity_3d, jdev->nr_3d);
	if (ret)
		goto fini;
	ret = mwv207_ctx_entity_init_single(ctx, jdev->sched_dec, ctx->entity_dec, jdev->nr_dec);
	if (ret)
		goto fini;
	ret = mwv207_ctx_entity_init_single(ctx, jdev->sched_enc, ctx->entity_enc, jdev->nr_enc);
	if (ret)
		goto fini;
	ret = mwv207_ctx_entity_init_single(ctx, jdev->sched_2d, ctx->entity_2d, jdev->nr_2d);
	if (ret)
		goto fini;
	ret = mwv207_ctx_entity_init_single(ctx, jdev->sched_dma, ctx->entity_dma, jdev->nr_dma);
	if (ret)
		goto fini;
	return 0;
fini:
	mwv207_ctx_entity_fini(ctx);
	return ret;
}

static int mwv207_ctx_create(struct drm_device *dev, struct mwv207_ctx_mgr *mgr,
		u32 flags, u32 *handle)
{
	struct mwv207_ctx *ctx;
	int ret;

	ctx = kzalloc(sizeof(struct mwv207_ctx), GFP_KERNEL);
	if (ctx == NULL)
		return -ENOMEM;

	ctx->jdev = ddev_to_jdev(dev);
	kref_init(&ctx->refcnt);

	ret = mwv207_ctx_entity_init(dev, ctx);
	if (ret)
		goto out_free;

	mutex_lock(&mgr->lock);
	ret = idr_alloc(&mgr->handle_table, ctx, 1, 0, GFP_KERNEL);
	mutex_unlock(&mgr->lock);

	if (ret < 0)
		goto out_fini_entites;

	*handle = ret;
	return 0;
out_fini_entites:
	mwv207_ctx_entity_fini(ctx);
out_free:
	kfree(ctx);
	return ret;
}

static void mwv207_ctx_release(struct kref *kref)
{
	struct mwv207_ctx *ctx = container_of(kref, struct mwv207_ctx, refcnt);

	mwv207_ctx_entity_fini(ctx);
	kfree(ctx);
}

int mwv207_ctx_put(struct mwv207_ctx *ctx)
{
	return	kref_put(&ctx->refcnt, mwv207_ctx_release);
}

static int mwv207_ctx_destroy(struct drm_device *dev, struct mwv207_ctx_mgr *mgr, u32 handle)
{
	struct mwv207_ctx *ctx;

	mutex_lock(&mgr->lock);
	ctx = idr_remove(&mgr->handle_table, handle);
	mutex_unlock(&mgr->lock);

	if (ctx == NULL)
		return -EINVAL;

	mwv207_ctx_put(ctx);
	return 0;
}

int  mwv207_ctx_ioctl(struct drm_device *dev, void *data,
			    struct drm_file *filp)
{
	struct drm_mwv207_ctx *args = (struct drm_mwv207_ctx *)data;
	struct mwv207_fpriv *fpriv = to_fpriv(filp);

	if (args->resv || args->flags)
		return -EINVAL;

	switch (args->op) {
	case 0:
		return mwv207_ctx_create(dev, &fpriv->ctx_mgr, args->flags, &args->handle);
	case 1:
		return mwv207_ctx_destroy(dev, &fpriv->ctx_mgr, args->handle);
	default:
		return -EINVAL;
	}
}

int mwv207_ctx_mgr_init(struct drm_device *dev, struct mwv207_ctx_mgr *mgr)
{
	mutex_init(&mgr->lock);
	idr_init(&mgr->handle_table);
	return 0;
}

void mwv207_ctx_mgr_fini(struct drm_device *dev, struct mwv207_ctx_mgr *mgr)
{
	struct mwv207_ctx *ctx;
	uint32_t id;

	idr_for_each_entry(&mgr->handle_table, ctx, id)
		mwv207_ctx_put(ctx);

	idr_destroy(&mgr->handle_table);
	mutex_destroy(&mgr->lock);
}

int mwv207_kctx_init(struct mwv207_device *jdev)
{
	struct drm_gpu_scheduler *scheds[1];
	int i;

	for (i = 0; i < 1; i++) {
		scheds[i] = jdev->sched_dma[i];
	}

	jdev->dma_entity = devm_kzalloc(jdev->dev, sizeof(struct drm_sched_entity), GFP_KERNEL);
	if (!jdev->dma_entity)
		return -ENOMEM;

	return drm_sched_entity_init(jdev->dma_entity, DRM_SCHED_PRIORITY_NORMAL, &scheds[0], i, NULL);
}

void mwv207_kctx_fini(struct mwv207_device *jdev)
{
	drm_sched_entity_destroy(jdev->dma_entity);
}

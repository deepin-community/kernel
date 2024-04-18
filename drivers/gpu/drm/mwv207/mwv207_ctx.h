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
#ifndef MWV207_CTX_H_QFMRESBO
#define MWV207_CTX_H_QFMRESBO
#include <drm/drm_file.h>
#include <drm/gpu_scheduler.h>
#include "mwv207.h"

struct mwv207_ctx {
	struct kref refcnt;

	struct drm_sched_entity *entities[11];

	struct drm_sched_entity **entity_3d;
	struct drm_sched_entity **entity_dec;
	struct drm_sched_entity **entity_enc;
	struct drm_sched_entity **entity_2d;
	struct drm_sched_entity **entity_dma;

	struct mwv207_device *jdev;
	atomic_t guilty;
};

struct mwv207_ctx *mwv207_ctx_lookup(struct drm_device *dev,
		struct drm_file *filp, u32 handle);
int mwv207_ctx_put(struct mwv207_ctx *ctx);

int  mwv207_ctx_ioctl(struct drm_device *dev, void *data,
			    struct drm_file *filp);

int mwv207_ctx_mgr_init(struct drm_device *dev, struct mwv207_ctx_mgr *mgr);
void mwv207_ctx_mgr_fini(struct drm_device *dev, struct mwv207_ctx_mgr *mgr);

int mwv207_kctx_init(struct mwv207_device *jdev);
void mwv207_kctx_fini(struct mwv207_device *jdev);

#endif

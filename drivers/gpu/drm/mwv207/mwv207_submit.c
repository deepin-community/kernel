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
#include <linux/uaccess.h>
#include <linux/sync_file.h>
#include <linux/file.h>
#include "mwv207_drm.h"
#include "mwv207_submit.h"
#include "mwv207_sched.h"
#include "mwv207_ctx.h"
#include "mwv207_bo.h"
#include "mwv207_gem.h"

static inline int mwv207_submit_select_engine(struct drm_device *dev, struct mwv207_job *mjob,
		struct drm_mwv207_submit *args)
{
	struct mwv207_device *jdev = ddev_to_jdev(dev);
	struct drm_sched_entity **entity_array;
	int id_max, id;

	switch (args->engine_type) {
	case 0:
		entity_array = &mjob->ctx->entity_3d[0];
		id_max = jdev->nr_3d;
		break;
	case 1:
		entity_array = &mjob->ctx->entity_dec[0];
		id_max = jdev->nr_dec;
		break;
	case 2:
		entity_array = &mjob->ctx->entity_enc[0];
		id_max = jdev->nr_enc;
		break;
	case 3:
		entity_array = &mjob->ctx->entity_2d[0];
		id_max = jdev->nr_2d;
		break;
	case 4:
		entity_array = &mjob->ctx->entity_dma[0];
		id_max = jdev->nr_dma;
		mjob->is_dma = true;
		break;

	default:
		id_max = 0;
	}
	if (id_max <= 0)
		return -ENODEV;

	if (args->engine_id == 0xffffffff)
		id = id_max;
	else if (args->engine_id >= 0 && args->engine_id < id_max)
		id = args->engine_id;
	else
		return -ENODEV;
	mjob->engine_entity = entity_array[id];
	return 0;
}

static int mwv207_submit_init_job_reloc(struct mwv207_job *mjob, struct drm_mwv207_submit *args)
{
	if ((int)args->nr_relocs < 0)
		return -EINVAL;
	mjob->nr_relocs = (int)args->nr_relocs;
	if (mjob->nr_relocs == 0)
		return 0;
	if (args->relocs == 0)
		return -EINVAL;

	mjob->relocs = kvmalloc_array(mjob->nr_relocs,
			sizeof(struct drm_mwv207_submit_reloc), GFP_KERNEL);

	return copy_from_user(mjob->relocs, (void __user *)args->relocs,
			sizeof(struct drm_mwv207_submit_reloc) * mjob->nr_relocs);
}

static int mwv207_submit_init_job_bo(struct mwv207_job *mjob,
		struct drm_mwv207_submit *args, struct drm_file *filp)
{
	struct drm_mwv207_bo_acc ubos_stack[32];
	struct drm_mwv207_bo_acc *ubos = ubos_stack;
	struct ttm_validate_buffer *tvb;
	struct drm_gem_object *gobj;
	int ret = 0, i;

	if ((int)args->nr_bos < 0)
		return -EINVAL;

	mjob->nr_bos = (int)args->nr_bos;
	if (mjob->nr_bos == 0) {
		if (mjob->nr_cmd_dat)
			goto setup_mtvb;
		return 0;
	}

	if (!args->bos)
		return -EINVAL;

	if (mjob->nr_bos > 32)
		ubos = kvmalloc_array(mjob->nr_bos, sizeof(*ubos), GFP_KERNEL);
	if (unlikely(!ubos))
		return -ENOMEM;

	ret = copy_from_user(ubos, (void __user *)args->bos,
			mjob->nr_bos * sizeof(*ubos));
	if (ret)
		goto out;
setup_mtvb:
	mjob->mtvb = kvcalloc(mjob->nr_bos + mjob->nr_cmd_dat,
			sizeof(struct mwv207_tvb), GFP_KERNEL);
	if (!mjob->mtvb) {
		ret = -ENOMEM;
		goto out;
	}
	spin_lock(&filp->table_lock);
	for (i = 0; i < mjob->nr_bos; i++) {
		tvb = &mjob->mtvb[i].base;
		tvb->num_shared = (ubos[i].flags & 0x00000001) ? 0 : 1;
		gobj = idr_find(&filp->object_idr, ubos[i].handle);
		if (unlikely(!gobj)) {
			spin_unlock(&filp->table_lock);
			ret = -ENOENT;
			goto out;
		}
		tvb->bo = &mwv207_bo_ref(mwv207_bo_from_gem(gobj))->tbo;
		list_add_tail(&tvb->head, &mjob->tvblist);
	}
	spin_unlock(&filp->table_lock);
	for (i = 0; i < mjob->nr_cmd_dat; i++) {
		tvb = &mjob->mtvb[mjob->nr_bos + i].base;
		tvb->num_shared = 0;
		tvb->bo = &mwv207_bo_ref(mjob->cmd_dats[i].jbo)->tbo;
		list_add_tail(&tvb->head, &mjob->tvblist);
	}
out:
	if (unlikely(ubos != ubos_stack))
		kvfree(ubos);

	return ret;
}

static int mwv207_submit_init_job_cmd(struct mwv207_job *mjob, struct drm_mwv207_submit *args)
{
	mjob->cmd_size = args->cmd_size;
	if ((int)mjob->cmd_size <= 0)
		return -EINVAL;

	if (mjob->cmd_size > 0x20000)
		return -E2BIG;

	if (args->cmds == 0)
		return -EINVAL;

	if (mjob->is_dma)
		mjob->cmds = kmalloc(args->cmd_size, GFP_KERNEL);
	else
		mjob->cmds = kvmalloc(args->cmd_size, GFP_KERNEL);
	if (!mjob->cmds)
		return -ENOMEM;

	return copy_from_user(mjob->cmds, (void __user *)args->cmds, mjob->cmd_size);
}

static int mwv207_submit_init_job_cmd_dat(struct drm_device *dev,
		struct mwv207_job *mjob, struct drm_mwv207_submit *args)
{
	struct drm_mwv207_submit_cmd_dat udats_stack[8];
	struct drm_mwv207_submit_cmd_dat *udats = udats_stack;
	struct mwv207_device *jdev = ddev_to_jdev(dev);
	struct mwv207_submit_cmd_dat *mdat;
	int ret, i;

	if ((int)args->nr_cmd_dat < 0)
		return -EINVAL;
	mjob->nr_cmd_dat = args->nr_cmd_dat;
	if (mjob->nr_cmd_dat == 0)
		return 0;
	if (args->cmd_dats == 0)
		return -EINVAL;
	if (mjob->nr_cmd_dat > 8)
		udats = kvmalloc_array(mjob->nr_cmd_dat,
				sizeof(struct drm_mwv207_submit_cmd_dat), GFP_KERNEL);

	if (unlikely(!udats))
		return -ENOMEM;

	mjob->cmd_dats = kvcalloc(mjob->nr_cmd_dat,
			sizeof(struct mwv207_submit_cmd_dat), GFP_KERNEL);
	if (!mjob->cmd_dats) {
		ret = -ENOMEM;
		goto out;
	}

	ret = copy_from_user(udats, (void __user *)args->cmd_dats,
			mjob->nr_cmd_dat * sizeof(struct drm_mwv207_submit_cmd_dat));
	if (ret)
		goto out;

	for (i = 0; i < mjob->nr_cmd_dat; ++i) {
		if ((int)udats[i].nr_relocs < 0) {
			ret = -EINVAL;
			goto out;
		}
		if ((int)udats[i].dat_size <= 0 || udats[i].dat == 0) {
			ret = -EINVAL;
			goto out;
		}
		if (udats[i].nr_relocs != 0 && udats[i].relocs == 0) {
			ret = -EINVAL;
			goto out;
		}
		if (udats[i].dat_size > 0x20000) {
			ret = -E2BIG;
			goto out;
		}
		mdat = &mjob->cmd_dats[i];
		mdat->nr_relocs = udats[i].nr_relocs;
		if (mdat->nr_relocs) {
			mdat->relocs = kvmalloc_array(mdat->nr_relocs,
					sizeof(struct drm_mwv207_submit_reloc), GFP_KERNEL);
			if (!mdat->relocs) {
				ret = -ENOMEM;
				goto out;
			}
			ret = copy_from_user(mdat->relocs, (void __user *)udats[i].relocs,
					mdat->nr_relocs * sizeof(struct drm_mwv207_submit_reloc));
			if (ret)
				goto out;
		}
		mdat->dat_size = udats[i].dat_size;
		mdat->dat = kvmalloc(mdat->dat_size, GFP_KERNEL);
		if (!mdat->dat) {
			ret = -ENOMEM;
			goto out;
		}
		ret = copy_from_user(mdat->dat, (void __user *)udats[i].dat, mdat->dat_size);
		if (ret)
			goto out;
		ret = mwv207_bo_create(jdev, mdat->dat_size, 0x1000, ttm_bo_type_device,
				0x2, (1<<0),
				&mdat->jbo);
		if (ret)
			goto out;
	}

out:
	if (unlikely(udats != udats_stack))
		kvfree(udats);
	return ret;
}

static int mwv207_submit_init_job(struct drm_device *dev, struct mwv207_job *mjob,
		struct drm_mwv207_submit *args, struct drm_file *filp)
{
	int ret;

	get_task_comm(mjob->comm, current);

	mjob->ctx = mwv207_ctx_lookup(dev, filp, args->ctx);
	if (!mjob->ctx)
		return -ENOENT;
	ret = mwv207_submit_select_engine(dev, mjob, args);
	if (ret)
		return ret;
	ret = mwv207_submit_init_job_cmd_dat(dev, mjob, args);
	if (ret)
		return ret;
	ret = mwv207_submit_init_job_bo(mjob, args, filp);
	if (ret)
		return ret;
	ret = mwv207_submit_init_job_reloc(mjob, args);
	if (ret)
		return ret;
	ret = mwv207_submit_init_job_cmd(mjob, args);
	if (ret)
		return ret;

	return 0;
}

static struct mwv207_job *mwv207_submit_init(struct drm_device *dev, struct drm_file *filp,
		struct drm_mwv207_submit *args)
{
	struct mwv207_job *mjob;
	int ret;

	mjob = mwv207_job_alloc();
	if (!mjob)
		return ERR_PTR(-ENOMEM);

	ret = mwv207_submit_init_job(dev, mjob, args, filp);
	if (ret)
		goto err;

	return mjob;
err:
	mwv207_job_put(mjob);
	return ERR_PTR(ret);
}

static int mwv207_submit_patch_entry(struct mwv207_job *mjob,
		struct drm_mwv207_submit_reloc *reloc,
		struct mwv207_bo *jbo, char *dat, u32 dat_size)
{
	u64 addr, bo_size;

	if (reloc->pad)
		return -EINVAL;

	bo_size = mwv207_bo_size(jbo);
	if (reloc->bo_offset >= bo_size)
		return -ERANGE;
	addr = mwv207_bo_gpu_phys(jbo) + reloc->bo_offset;

	switch (reloc->type & 0xffff) {
	case 0:
		if (reloc->cmd_offset + 8 > dat_size)
			return -ERANGE;
		*(u64 *)(dat + reloc->cmd_offset) = addr;
		break;
	case 1:
		if (reloc->cmd_offset + 4 > dat_size)
			return -ERANGE;
		*(u32 *)(dat + reloc->cmd_offset) = addr & 0xffffffff;
		break;
	case 2:
		if (reloc->cmd_offset + 4 > dat_size)
			return -ERANGE;
		*(u32 *)(dat + reloc->cmd_offset) = addr >> 32;
		break;
	case 3:
		if (reloc->cmd_offset + 2 > dat_size)
			return -ERANGE;
		*(u16 *)(dat + reloc->cmd_offset) = (addr >> 16) & 0xffff;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int mwv207_submit_patch_single(struct mwv207_job *mjob,
		struct drm_mwv207_submit_reloc *reloc, char *dat, u32 dat_size)
{
	struct mwv207_bo *jbo;

	if (reloc->type & 0x80000000) {
		if (reloc->idx >= mjob->nr_cmd_dat)
			return -EINVAL;
		jbo = mwv207_job_bo(mjob, reloc->idx + mjob->nr_bos);
	} else {
		if (reloc->idx >= mjob->nr_bos)
			return -EINVAL;
		jbo = mwv207_job_bo(mjob, reloc->idx);
	}

	return  mwv207_submit_patch_entry(mjob, reloc, jbo, dat, dat_size);
}

static int mwv207_submit_patch_dma_entry(struct mwv207_job *mjob,
		struct mwv207_dma_loc *loc, struct mwv207_bo *jbo,
		u64 width, u64 height)
{
	struct mwv207_ttm_tt *gtt;

	if (loc->stride == 0 || loc->pg_nr_type != 0 ||  width == 0 || height == 0)
		return -EINVAL;

	switch (jbo->tbo.resource->mem_type) {
	case TTM_PL_VRAM:
		loc->base = mwv207_bo_gpu_phys(jbo);
		loc->pg_nr_type = MWV207_DMA_NR_PAGES_VRAM(PFN_UP(jbo->tbo.resource->size));
		break;
	case TTM_PL_TT:
		gtt = to_gtt(jbo->tbo.ttm);
		loc->base = (u64)gtt->ttm.dma_address;
		loc->pg_nr_type = MWV207_DMA_NR_PAGES_RAM(PFN_UP(jbo->tbo.resource->size));
		break;
	default:
		BUG_ON(1);
		break;
	}

	if (loc->offset >= mwv207_bo_size(jbo))
		return -ERANGE;
	if (loc->offset + loc->stride * (height - 1) + width <= loc->offset)
		return -ERANGE;
	if (loc->offset + loc->stride * (height - 1) + width > mwv207_bo_size(jbo))
		return -ERANGE;

	if ((loc->offset % loc->stride) + width > loc->stride)
		return -ERANGE;

	return 0;
}

static int mwv207_submit_patch_dma(struct drm_device *dev, struct mwv207_job *mjob)
{
	struct mwv207_bo *src_bo, *dst_bo, *jbo;
	struct drm_mwv207_submit_reloc *reloc;
	struct mwv207_dma_cmd *dma_cmd;
	int i, ret;

	if (mjob->nr_bos != 2 || mjob->nr_relocs != 2 || mjob->nr_cmd_dat)
		return -EINVAL;
	if (mjob->cmd_size != sizeof(struct mwv207_dma_cmd))
		return -EINVAL;

	src_bo = dst_bo = NULL;
	for (i = 0; i < mjob->nr_relocs; i++) {
		reloc = &mjob->relocs[i];
		if (reloc->type & 0x80000000)
			return -EINVAL;
		if ((reloc->type & 0xffff) != 0)
			return -EINVAL;
		if (reloc->bo_offset)
			return -EINVAL;
		jbo = mwv207_job_bo(mjob, reloc->idx);
		if (reloc->cmd_offset == offsetof(struct mwv207_dma_cmd, src.base))
			src_bo = jbo;
		else if (reloc->cmd_offset == offsetof(struct mwv207_dma_cmd, dst.base))
			dst_bo = jbo;
	}
	if (!src_bo || !dst_bo || src_bo == dst_bo)
		return -EINVAL;

	dma_cmd = (struct mwv207_dma_cmd *)mjob->cmds;
	ret = mwv207_submit_patch_dma_entry(mjob, &dma_cmd->src, src_bo,
			dma_cmd->width, dma_cmd->height);
	if (ret)
		return ret;
	return mwv207_submit_patch_dma_entry(mjob, &dma_cmd->dst, dst_bo,
			dma_cmd->width, dma_cmd->height);
}

static int mwv207_submit_patch(struct drm_device *dev, struct mwv207_job *mjob)
{
	struct mwv207_bo *jbo;
	struct ttm_operation_ctx ctx = {
		.interruptible = true,
		.no_wait_gpu = false
	};
	int i, j, ret;
	void *ptr;

	if (mjob->is_dma)
		return mwv207_submit_patch_dma(dev, mjob);

	for (i = 0; i < mjob->nr_relocs; ++i) {
		ret = mwv207_submit_patch_single(mjob, &mjob->relocs[i],
				mjob->cmds, mjob->cmd_size);
		if (ret)
			return ret;
	}

	for (i = 0; i < mjob->nr_cmd_dat; ++i) {
		if (mjob->cmd_dats[i].nr_relocs) {
			for (j = 0; j < mjob->cmd_dats[i].nr_relocs; j++) {
				ret = mwv207_submit_patch_single(mjob, &mjob->cmd_dats[i].relocs[j],
						mjob->cmd_dats[i].dat, mjob->cmd_dats[i].dat_size);
				if (ret)
					return ret;
			}
		}

		jbo = mjob->cmd_dats[i].jbo;

		ret = ttm_bo_wait_ctx(&jbo->tbo, &ctx);
		if (ret)
			return ret;

		ret = mwv207_bo_kmap_reserved(jbo, &ptr);
		if (ret)
			return ret;
		memcpy_toio(ptr, mjob->cmd_dats[i].dat, mjob->cmd_dats[i].dat_size);
		mwv207_bo_kunmap_reserved(jbo);
	}

	return 0;
}

static int mwv207_submit_validate(struct drm_device *dev, struct mwv207_job *mjob)
{
	struct ttm_operation_ctx ctx = {
		.interruptible = true,
		.no_wait_gpu = false
	};
	struct mwv207_bo *jbo;
	int i, ret;

	for (i = 0; i < mjob->nr_bos + mjob->nr_cmd_dat; ++i) {
		jbo = mwv207_job_bo(mjob, i);

		if (jbo->tbo.pin_count)
			continue;

		if (mjob->is_dma) {
			if (jbo->tbo.resource->mem_type != TTM_PL_SYSTEM)
				continue;

			mwv207_bo_placement_from_domain(jbo, 0x4, 0);
		} else {

			jbo->flags |= (1<<2);
			if (i >= mjob->nr_bos)
				jbo->flags |= (1<<0);
			mwv207_bo_placement_from_domain(jbo, 0x2, 0);
		}

		ret = ttm_bo_validate(&jbo->tbo, &jbo->placement, &ctx);
		if (ret)
			return ret;
	}

	return 0;
}

static int mwv207_submit_attach_dependency(struct mwv207_job *mjob, struct drm_mwv207_submit *args)
{
	struct drm_gem_object *gobj;
	struct mwv207_tvb *mtvb;
	bool write;
	int ret;

	mwv207_for_each_mtvb(mtvb, mjob) {
		gobj = &mtvb->base.bo->base;
		write = mtvb->base.num_shared == 0 ? 1 : 0;

		ret = drm_sched_job_add_implicit_dependencies(&mjob->base, gobj, write);
		if (ret)
			return ret;
	}

	if (unlikely(args->flags & 0x00000001)) {
		mjob->fence_in = sync_file_get_fence(args->fence_fd);
		if (!mjob->fence_in)
			return -EINVAL;
	}

	return 0;
}

static int mwv207_submit_commit(struct mwv207_job *mjob,
		struct drm_mwv207_submit *args, struct dma_fence **fence)
{
	if (args->flags & 0x00000002) {
		struct sync_file *sync_file;
		int fd;

		fd = get_unused_fd_flags(O_CLOEXEC);
		if (fd < 0)
			return fd;
		sync_file = sync_file_create(&mjob->base.s_fence->finished);
		if (!sync_file) {
			put_unused_fd(fd);
			return -ENOMEM;
		}
		fd_install(fd, sync_file->file);
		args->fence_fd = fd;
	}

	*fence = &mjob->base.s_fence->finished;

	mwv207_job_get(mjob);
	drm_sched_entity_push_job(&mjob->base);
	return 0;
}

int mwv207_submit_ioctl(struct drm_device *dev, void *data,
			    struct drm_file *filp)
{
	struct drm_mwv207_submit *args = data;
	struct ww_acquire_ctx ticket;
	struct mwv207_job *mjob;
	struct dma_fence *fence = NULL;
	int ret;

	if (args->padding || (args->flags & ~0x00000003))
		return -EINVAL;
	if (args->cmd_size == 0 || args->cmds == 0)
		return -EINVAL;

	mjob = mwv207_submit_init(dev, filp, args);
	if (IS_ERR(mjob))
		return PTR_ERR(mjob);

	ret = drm_sched_job_init(&mjob->base, mjob->engine_entity, mjob->ctx);
	if (ret)
		return ret;

	drm_sched_job_arm(&mjob->base);

	ret = ttm_eu_reserve_buffers(&ticket, &mjob->tvblist, true, NULL);
	if (ret)
		goto put_job;
	ret = mwv207_submit_validate(dev, mjob);
	if (ret)
		goto unreserve;
	ret = mwv207_submit_patch(dev, mjob);
	if (ret)
		goto unreserve;
	ret = mwv207_submit_attach_dependency(mjob, args);
	if (ret)
		goto unreserve;
	ret = mwv207_submit_commit(mjob, args, &fence);
	if (ret)
		goto unreserve;
unreserve:
	if (likely(fence))
		ttm_eu_fence_buffer_objects(&ticket, &mjob->tvblist, fence);
	else
		ttm_eu_backoff_reservation(&ticket, &mjob->tvblist);
put_job:
	mwv207_job_put(mjob);

	return ret;
}

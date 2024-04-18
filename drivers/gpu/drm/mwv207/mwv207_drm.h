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
#ifndef __MWV207_DRM_H__
#define __MWV207_DRM_H__

#include "drm/drm.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**********************GEM interface*************************/

struct drm_mwv207_gem_create_in {
	__u64 size;
	__u64 alignment;
	__u32 preferred_domain;
	__u32 flags;
};
struct drm_mwv207_gem_create_out {
	__u32 handle;
	__u32 resv;
};
union drm_mwv207_gem_create {
	struct drm_mwv207_gem_create_in in;
	struct drm_mwv207_gem_create_out out;
};

struct drm_mwv207_gem_mmap_in {
	__u32 handle;
	__u32 pad;
};
struct drm_mwv207_gem_mmap_out {
	__u64 offset;
};
union drm_mwv207_gem_mmap {
	struct drm_mwv207_gem_mmap_in in;
	struct drm_mwv207_gem_mmap_out out;
};

struct drm_mwv207_gem_wait {
	__u32 handle;
	__u32 op;
	__s64 timeout;
};

/**********************context interface*************************/
struct drm_mwv207_ctx {
	__u32 op;
	__u32 flags;
	__u32 handle;
	__u32 resv;
};

/**********************submission interface*************************
 * 1. a submission is sent to a single ctx
 * 2. a submission optionally contains a relocation table and a bo table
 * 3. bo table should not contain duplicates;
 * 4. cmd buffer size should be limited to 16K
 */

struct drm_mwv207_submit {
	__u32 ctx;
	__u32 engine_type;
	__u32 engine_id;
	__u32 flags;
	__u32 fence_fd;    /* in/out fence fd, according to MWV207_SUBMIT_FLAG_FENCE_IN/OUT */
	__u32 cmd_size;
	__u32 nr_relocs;
	__u32 nr_bos;
	__u32 nr_cmd_dat;
	__u32 padding;
	__u64 cmds;
	__u64 relocs;
	__u64 bos;
	__u64 cmd_dats;
	/* syncobj support, reserved for vulkan
	 * __u32 nr_in_syncobjs;
	 * __u32 nr_out_syncobjs;
	 * __u64 in_syncobjs;
	 * __u64 out_syncobjs;
	 */
};

/* in_syncobjs/out_syncobjs array element, reserved for vulkan */
struct drm_mwv207_submit_syncobj {
	__u32 handle;
	__u32 flags;
	__u64 point;
};

struct drm_mwv207_bo_acc {
	__u32 handle;
	__u32 flags;
};

struct drm_mwv207_submit_reloc {
	__u32 idx;    /* bo/cmd_dat idx in submit bos/cmd_dats */
	__u32 cmd_offset;
	__u64 bo_offset;
	__u32 type;
	__u32 pad;
};

struct drm_mwv207_submit_cmd_dat {
	__u32 nr_relocs;
	__u32 dat_size;
	__u64 relocs;
	__u64 dat;
};

/**************** card capability interface *****************/
/* XXX: keys are user/kernel ABI, DO THINK TWICE when
 * add keys, once added it can never be removed.
 */
enum drm_mwv207_info_key {
	DRM_MWV207_ACTIVE_3D_NR = 0,
	DRM_MWV207_ACTIVE_DEC_NR,
	DRM_MWV207_ACTIVE_ENC_NR,
	DRM_MWV207_ACTIVE_2D_NR,
	DRM_MWV207_ACTIVE_DMA_NR,
};

struct drm_mwv207_info_rec {
	__u32 key;
	__u32 val;
};

struct drm_mwv207_info {
	__u32 op;
	__u32 nr_recs;
	__u64 recs;
};

#define DRM_IOCTL_MWV207_INFO        DRM_IOWR(DRM_COMMAND_BASE + 0x00, struct drm_mwv207_info)
#define DRM_IOCTL_MWV207_GEM_CREATE  DRM_IOWR(DRM_COMMAND_BASE + 0x01, union drm_mwv207_gem_create)
#define DRM_IOCTL_MWV207_GEM_MMAP    DRM_IOWR(DRM_COMMAND_BASE + 0x02, union drm_mwv207_gem_mmap)
#define DRM_IOCTL_MWV207_GEM_WAIT    DRM_IOWR(DRM_COMMAND_BASE + 0x03, struct drm_mwv207_gem_wait)
#define DRM_IOCTL_MWV207_CTX         DRM_IOWR(DRM_COMMAND_BASE + 0x04, struct drm_mwv207_ctx)
#define DRM_IOCTL_MWV207_SUBMIT      DRM_IOWR(DRM_COMMAND_BASE + 0x05, struct drm_mwv207_submit)
#if defined(__cplusplus)
}
#endif

#endif

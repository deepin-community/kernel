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
#ifndef MWV207_GEM_H_TJTW9M4R
#define MWV207_GEM_H_TJTW9M4R
#include <linux/version.h>
#include <drm/drm_device.h>
#include <linux/types.h>
#include "mwv207.h"

extern const struct drm_gem_object_funcs mwv207_gem_object_funcs;

int mwv207_gem_dumb_create(struct drm_file *file, struct drm_device *dev,
		     struct drm_mode_create_dumb *args);

void mwv207_gem_free_object(struct drm_gem_object *obj);

int  mwv207_gem_create_ioctl(struct drm_device *dev, void *data,
			    struct drm_file *filp);

int  mwv207_gem_mmap_ioctl(struct drm_device *dev, void *data,
			    struct drm_file *filp);

int  mwv207_gem_wait_ioctl(struct drm_device *dev, void *data,
			    struct drm_file *filp);

#define mwv207_gem_object_put drm_gem_object_put

#endif

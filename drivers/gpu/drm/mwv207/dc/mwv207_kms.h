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
#ifndef MWV207_KMS_H_VBCHTKVP
#define MWV207_KMS_H_VBCHTKVP

#include <drm/drm_gem.h>
#include "mwv207.h"

int mwv207_framebuffer_init(struct mwv207_device *jdev, struct drm_framebuffer *fb,
		const struct drm_mode_fb_cmd2 *mode_cmd, struct drm_gem_object *gobj);

int mwv207_kms_init(struct mwv207_device *jdev);
void mwv207_kms_fini(struct mwv207_device *jdev);

int mwv207_kms_suspend(struct mwv207_device *jdev);
int mwv207_kms_resume(struct mwv207_device *jdev);
#endif

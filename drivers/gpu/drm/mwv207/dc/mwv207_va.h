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
#include <linux/irq.h>
#ifndef MWV207_VA_H_BSY8LF4F
#define MWV207_VA_H_BSY8LF4F

struct drm_crtc;
int mwv207_va_init(struct mwv207_device *jdev);

void mwv207_crtc_prepare_vblank(struct drm_crtc *crtc);
irqreturn_t mwv207_va_handle_vblank(int irq, void *data);
#endif

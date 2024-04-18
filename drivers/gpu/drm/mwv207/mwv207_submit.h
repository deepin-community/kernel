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
#ifndef MWV207_SUBMIT_H_2S1GH9LD
#define MWV207_SUBMIT_H_2S1GH9LD

int  mwv207_submit_ioctl(struct drm_device *dev, void *data,
			    struct drm_file *filp);

#endif

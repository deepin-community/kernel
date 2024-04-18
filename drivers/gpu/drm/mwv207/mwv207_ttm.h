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
#ifndef MWV207_TTM_H_TJTW9M4R
#define MWV207_TTM_H_TJTW9M4R
#include <linux/version.h>
#include <drm/drm_device.h>
#include <linux/types.h>
#include "mwv207.h"

int mwv207_bo_fault_reserve_notify(struct ttm_buffer_object *bo);

#endif

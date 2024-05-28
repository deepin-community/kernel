/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023 Montage Technology. All rights reserved.
 */

#ifndef __TSSE_DEV_DRV_H__
#define __TSSE_DEV_DRV_H__
#define TSSE_DEV_NAME "tsse"

// TODO: need to support full qpairs
#define TSSE_PF_MAX_QPAIR_NUM 16

#define TSSE_PF_MAX_IRQ_NUM 96
#define TSSE_PF_QPAIR_START_IRQ_VECTOR 32

#define TSSE_SRIOV_PF_MAX_QPAIR_NUM 0
#define TSSE_SRIOV_PF_MAX_IRQ_NUM 16

#define TSSE_PF_QPAIR_REG_BASE 0x5700000

#include "tsse_dev.h"

#endif

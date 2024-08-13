/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_COMMON_H
#define SSS_NIC_COMMON_H

#include <linux/types.h>

#include "sss_kernel.h"
#include "sss_version.h"

#define SSSNIC_DRV_NAME			"sssnic"
#define SSSNIC_DRV_VERSION		SSS_VERSION_STR

#define SSSNIC_FUNC_IS_VF(hwdev)	(sss_get_func_type(hwdev) == SSS_FUNC_TYPE_VF)

#define SSSNIC_MODERATONE_DELAY		HZ

#define SSSNIC_LP_PKT_CNT			64

#endif

/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023-2024 Montage Technology. All rights reserved.
 */

#ifndef __TSSE_HDNDLE_H__
#define __TSSE_HDNDLE_H__

#include <linux/types.h>
#include <linux/iommu.h>

int tsse_get_available_handle(void);
struct iommu_domain *tsse_get_domain_by_handle(int handle);
#endif

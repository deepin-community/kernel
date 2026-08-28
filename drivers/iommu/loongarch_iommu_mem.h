/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Loongson IOMMU Driver
 *
 * Copyright (C) 2026-2027 Loongson Technology Ltd.
 * Author:	Xianglai Li <lixianglai@loongson.cn>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef LOONGARCH_IOMMU_MEM_H
#define LOONGARCH_IOMMU_MEM_H

#define iommu_virt_to_phys(address) TO_PHYS((unsigned long)address)
#define iommu_phys_to_virt(address) ((void *)TO_UNCACHE((unsigned long)address))

void *loongarch_iommu_alloc_page(void);
void loongarch_iommu_free_page(void *page);
#endif

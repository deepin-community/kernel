/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved. */
#ifndef CQM_MEMSEC_H
#define CQM_MEMSEC_H

#include <linux/pci.h>
#include "hinic3_hwdev.h"
#include "hinic3_crm.h"
#include "cqm_define.h"

#define CQM_GET_MEMSEC_CTX_GPA 19
#define CQM_INT_ADDR_SHIFT 32
#define CQM_SECURE_MEM_ALIGNED_SIZE (2 * 1024 * 1024)

bool cqm_need_secure_mem(void *hwdev);
void *cqm_get_secure_mem_pages(struct hinic3_hwdev *handle, u32 order, dma_addr_t *pa_base);
void cqm_free_secure_mem_pages(struct hinic3_hwdev *handle, void *va, u32 order);
int cqm_secure_mem_init(void *ex_handle);
int cqm_secure_mem_deinit(void *ex_handle);
void hinic3_memsec_proc_init(void *hwdev);
void hinic3_memsec_proc_deinit(void *hwdev);

#endif /* CQM_MEMSEC_H */

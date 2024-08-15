/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_COMMON_H
#define SSS_COMMON_H

#include <linux/types.h>

#include "sss_hw_common.h"

int sss_dma_zalloc_coherent_align(void *dev_hdl, u64 size, u64 align,
				  unsigned int flag, struct sss_dma_addr_align *mem_align);

void sss_dma_free_coherent_align(void *dev_hdl, struct sss_dma_addr_align *mem_align);

int sss_check_handler_timeout(void *priv_data, sss_wait_handler_t handler,
			      u32 wait_total_ms, u32 wait_once_us);

#endif

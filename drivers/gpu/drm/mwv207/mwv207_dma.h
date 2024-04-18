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
#ifndef MWV207_EDMA_H_EPAWCJ4N
#define MWV207_EDMA_H_EPAWCJ4N
#include "mwv207.h"

struct mwv207_dma;
struct mwv207_dma *mwv207_dma_create(struct device *dev,
		void __iomem *regbase, u8 alignment);
void mwv207_dma_destroy(struct mwv207_dma *dma);

int mwv207_dma_to_vram(struct mwv207_dma *dma, u64 vram_addr, void *va, u64 size);
int mwv207_dma_to_ram(struct mwv207_dma *dma, void *va, u64 vram_addr, u64 size);
int mwv207_dma_vram_vram(struct mwv207_dma *dma, u64 dst_vram_addr, u64 src_vram_addr, u64 size);

#endif

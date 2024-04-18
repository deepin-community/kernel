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
#ifndef MWV207_VBIOS_H_D2JHN3FG
#define MWV207_VBIOS_H_D2JHN3FG

struct mwv207_device;

enum pll_id {
	MWV207_PLL_GPU = 0,
	MWV207_PLL_GRAPH0,
	MWV207_PLL_GRAPH1,
	MWV207_PLL_GRAPH2,
	MWV207_PLL_GRAPH3,
	MWV207_PLL_GRAPH4,
	MWV207_PLL_HD,
	MWV207_PLL_GU3D0,
	MWV207_PLL_EDP,
	MWV207_PLL_GU3D1,
	MWV207_PLL_RESERVED,
	MWV207_PLL_GU2D_PLL,
	MWV207_PLL_DDR,
	MWV207_PLL_COUNT
};

struct mwv207_vdat {
	const void *dat;
	u32 len;
};

int mwv207_vbios_set_pll(struct mwv207_device *jdev, int pll_id,  u32 kfreq);

int mwv207_vbios_get_pll(struct mwv207_device *jdev, int pll_idx, unsigned long *kfreq);

const struct mwv207_vdat *mwv207_vbios_vdat(struct mwv207_device *jdev, u32 key);

void mwv207_vbios_init(struct mwv207_device *jdev);
void mwv207_vbios_fini(struct mwv207_device *jdev);
#endif

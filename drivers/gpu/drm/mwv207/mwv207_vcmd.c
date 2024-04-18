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
#include <linux/time.h>
#include <linux/delay.h>
#include "mwv207.h"
#include "mwv207_vbios.h"

#define MWV207_PLL_TIMEOUT        msecs_to_jiffies(1000)

#define MWV207_PLL_STATE_INDEX(id)    (0x100 * ((id) / 6) + 0x20 * ((id) % 6))
#define MWV207_PLL_STATE_BASE(id)     (0x9b0000 + 0xA00 + MWV207_PLL_STATE_INDEX(id))

#define MWV207_PLL_FBDIV_REFDIV(id)      (MWV207_PLL_STATE_BASE(id) + 0x00)
#define MWV207_PLL_FRAC(id)              (MWV207_PLL_STATE_BASE(id) + 0x04)
#define MWV207_PLL_POSTDIV(id)           (MWV207_PLL_STATE_BASE(id) + 0x08)

static inline int mwv207_vcmd_idle(struct mwv207_device *jdev)
{
	u32 state;

	state = jdev_read(jdev, (0x9b0144)) >> 28;

	return state == 0;
}

static inline int mwv207_vcmd_wait_idle(struct mwv207_device *jdev)
{
	int i;

	for (i = 0; i < 1000 && !mwv207_vcmd_idle(jdev); i++)
		usleep_range(1000, 1001);
	return mwv207_vcmd_idle(jdev) ? 0 : -ETIMEDOUT;
}

static inline void mwv207_vcmd_req(struct mwv207_device *jdev, int cmd,
		int idx, u32 value, int enable)
{
	u32 req;

	req  = cmd << 28;
	req |= idx << 24;
	req |= value << 1;
	req |= enable ? 1 : 0;

	jdev_write(jdev, (0x9b0140), req);
}

static inline void mwv207_vcmd_start(struct mwv207_device *jdev)
{
	jdev_write(jdev, (0x9b0144), 1 << 28);
}

static inline int mwv207_vcmd_result(struct mwv207_device *jdev)
{
	return (jdev_read(jdev, (0x9b0144)) >> 24) & 0xf;
}

static int mwv207_vcmd_execute(struct mwv207_device *jdev, int cmd,
		int idx, u32 value, int enable)
{
	int ret;

	mutex_lock(&jdev->vbios.vcmd_lock);

	ret = mwv207_vcmd_wait_idle(jdev);
	if (ret)
		goto unlock;

	mwv207_vcmd_req(jdev, cmd, idx, value, enable);

	mwv207_vcmd_start(jdev);

	ret = mwv207_vcmd_wait_idle(jdev);
	if (ret)
		goto unlock;

	ret = mwv207_vcmd_result(jdev);
	switch (ret) {
	case 0:
		goto unlock;
	case 1:
		pr_err("unsupport pll type!)");
		break;
	case 2:

		pr_info_once("pll set, invalid freq!");
		break;
	case 3:
		pr_err("pll busy!");
		break;
	case 4:
		pr_err("pll set, invalid argument!");
		break;
	case 9:
		pr_err("PLL REQ cmd code unsupport!");
		break;
	default:
		pr_err("unknown pll error! (code 0x%x)", ret);
		break;
	}
	ret = -EIO;
unlock:
	mutex_unlock(&jdev->vbios.vcmd_lock);
	return ret;
}

int mwv207_vbios_set_pll(struct mwv207_device *jdev, int pll_idx, u32 kfreq)
{
	if (pll_idx >= MWV207_PLL_COUNT)
		return -EINVAL;

	return mwv207_vcmd_execute(jdev, 1, pll_idx,
			kfreq, kfreq ? 1 : 0);
}

int mwv207_vbios_get_pll(struct mwv207_device *jdev, int pll_idx, unsigned long *kfreq)
{
	u32 port, state1, state2, state3;
	u32 refdiv, fbintdiv, fbracdiv, postdiv1, postdiv2;
	u32 postdiv_mask, postdiv_offset;
	u64 freq;

	if (pll_idx >= MWV207_PLL_COUNT)
		return -EINVAL;

	port = pll_idx ? 0 : 1;

	mutex_lock(&jdev->vbios.vcmd_lock);
	state1 = jdev_read(jdev, MWV207_PLL_FBDIV_REFDIV(pll_idx));
	state2 = jdev_read(jdev, MWV207_PLL_FRAC(pll_idx));
	state3 = jdev_read(jdev, MWV207_PLL_POSTDIV(pll_idx));
	mutex_unlock(&jdev->vbios.vcmd_lock);

	refdiv   = state1 & 0x3f;
	fbintdiv = (state1 & 0xfff0000) >> 16;
	fbracdiv = state2 & 0xffffff;
	postdiv1 = 4;

	postdiv_offset = 8 * (port % 4);
	postdiv_mask   = 0x7f << postdiv_offset;
	postdiv2       = (state3 & postdiv_mask) >> postdiv_offset;

	if (!refdiv || !postdiv1 || !postdiv2) {
		dev_dbg(jdev->dev, "get pll failed");
		return -EINVAL;
	}

	freq = 100000 * 100000ull / refdiv;
	freq = freq * fbintdiv + ((freq * fbracdiv) >> 24);
	freq /= postdiv1;
	freq /= postdiv2;
	freq = DIV_ROUND_CLOSEST_ULL(freq, 100000);

	*kfreq = freq;

	dev_dbg(jdev->dev, "get pll result: freq %llu kHz\n", freq);

	return 0;
}


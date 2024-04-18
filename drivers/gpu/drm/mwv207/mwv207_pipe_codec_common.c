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
#include "mwv207_pipe_codec_common.h"

static void pipe_codec_sw_pp_submit_ex(struct pipe_codec_sw_pp *sw_pp, struct mwv207_pipe *pipe,
		u64 vram, u32 reg, int nr)
{
	if (sw_pp->wb_cnt >= JPIPE_WRITE_ENTRY_SIZE) {
		dev_dbg(pipe->jdev->dev, "mwv207: too much write back entry");
		return;
	}

	sw_pp->wb[sw_pp->wb_cnt].vram = vram;
	sw_pp->wb[sw_pp->wb_cnt].reg  = reg;
	sw_pp->wb[sw_pp->wb_cnt].nr   = nr;

	sw_pp->wb_cnt++;
	sw_pp->pending = true;
}

void pipe_codec_sw_pp_reset(struct pipe_codec_sw_pp *sw_pp)
{
	sw_pp->wb_cnt = 0;
}

void pipe_codec_sw_pp_excute(struct pipe_codec_sw_pp *sw_pp, struct mwv207_pipe *pipe)
{
	int i, j;

	if (!sw_pp->pending)
		return;

	for (i = 0; i < sw_pp->wb_cnt; i++) {
		for (j = 0; j < sw_pp->wb[i].nr && sw_pp->wb[i].reg + j * 4 <= pipe->iosize - 4; j++)
			sw_pp->wb_buf[j] = pipe_read(pipe, sw_pp->wb[i].reg + j * 4);

		if (!j)
			continue;

		jdev_write_vram(pipe->jdev, sw_pp->wb[i].vram, &sw_pp->wb_buf[0], j * 4);
	}

	wmb();

	sw_pp->wb_cnt = 0;
	sw_pp->pending = false;
}

void pipe_codec_sw_pp_submit(struct pipe_codec_sw_pp *sw_pp, struct mwv207_pipe *pipe,
		struct mwv207_job *mjob, int pos, int *cmdlen)
{
	int nr, reg;
	u64 vram;
	u32 cmd;

	*cmdlen = 16;

	if (pos + 16 > mjob->cmd_size) {
		dev_dbg(pipe->jdev->dev,
				"mwv207: cmd %d write back out of range", pos);
		return;
	}

	cmd = *(u32 *)(mjob->cmds + pos);
	reg = (cmd & 0xffff) * 4;
	nr  = (cmd >> 16) & 0x3ff;
	if (nr == 0)
		nr = 1024;

	vram = *(u64 *)(mjob->cmds + pos + 8);
	pipe_codec_sw_pp_submit_ex(sw_pp, pipe, vram, reg, nr);
}


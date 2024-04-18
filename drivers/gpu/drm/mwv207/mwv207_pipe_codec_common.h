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
#include "mwv207_sched.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define JPIPE_WRITE_ENTRY_SIZE  16
#define JPIPE_ENC_REG_NUM       500
#define JPIPE_DEC_REG_NUM       503
#define JPIPE_REG_NUM           MAX(JPIPE_ENC_REG_NUM, JPIPE_DEC_REG_NUM)

struct pipe_codec_wb_entry {
	u64 vram;
	u32 reg;
	u32 nr;
};

struct pipe_codec_sw_pp {
	struct pipe_codec_wb_entry wb[JPIPE_WRITE_ENTRY_SIZE];
	u32  wb_buf[JPIPE_REG_NUM];
	int  wb_cnt;
	bool pending;
};

void pipe_codec_sw_pp_reset(struct pipe_codec_sw_pp *sw_pp);
void pipe_codec_sw_pp_excute(struct pipe_codec_sw_pp *sw_pp, struct mwv207_pipe *pipe);
void pipe_codec_sw_pp_submit(struct pipe_codec_sw_pp *sw_pp, struct mwv207_pipe *pipe,
		struct mwv207_job *mjob, int pos, int *cmdlen);


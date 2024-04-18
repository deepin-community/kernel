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

#ifndef __MWV207_DEVFREQ_H__
#define __MWV207_DEVFREQ_H__

struct mwv207_pipe;

void mwv207_devfreq_unregister(struct mwv207_pipe *pipe);
int mwv207_devfreq_register(struct mwv207_pipe *pipe, unsigned long max_freq);

void mwv207_devfreq_record_busy(struct mwv207_pipe *pipe);
void mwv207_devfreq_record_idle(struct mwv207_pipe *pipe);

int mwv207_devfreq_suspend(struct mwv207_pipe *pipe);
int mwv207_devfreq_resume(struct mwv207_pipe *pipe);

#endif

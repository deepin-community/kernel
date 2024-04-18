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
#ifndef MWV207_IRQ_H_Z8YGVNB2
#define MWV207_IRQ_H_Z8YGVNB2

struct mwv207_device;

int  mwv207_irq_init(struct mwv207_device *jdev);
void mwv207_irq_fini(struct mwv207_device *jdev);

void mwv207_irq_suspend(struct mwv207_device *jdev);
void mwv207_irq_resume(struct mwv207_device *jdev);
#endif

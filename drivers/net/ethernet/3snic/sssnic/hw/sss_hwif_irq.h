/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWIF_IRQ_H
#define SSS_HWIF_IRQ_H

#include "sss_hwdev.h"

int sss_init_irq_info(struct sss_hwdev *dev);
void sss_deinit_irq_info(struct sss_hwdev *dev);

#endif

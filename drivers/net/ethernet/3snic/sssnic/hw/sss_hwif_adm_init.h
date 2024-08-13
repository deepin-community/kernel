/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWIF_ADM_INIT_H
#define SSS_HWIF_ADM_INIT_H

#include "sss_hwdev.h"

int sss_hwif_init_adm(struct sss_hwdev *hwdev);
void sss_hwif_deinit_adm(struct sss_hwdev *hwdev);
void sss_complete_adm_event(struct sss_hwdev *hwdev);

#endif

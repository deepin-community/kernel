/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWIF_CTRLQ_INIT_H
#define SSS_HWIF_CTRLQ_INIT_H

#include "sss_hwdev.h"

int sss_init_ctrlq_channel(struct sss_hwdev *hwdev);
void sss_deinit_ctrlq_channel(struct sss_hwdev *hwdev);
int sss_reinit_ctrlq_ctx(struct sss_hwdev *hwdev);
int sss_wait_ctrlq_stop(struct sss_hwdev *hwdev);
void sss_ctrlq_flush_sync_cmd(struct sss_hwdev *hwdev);

#endif

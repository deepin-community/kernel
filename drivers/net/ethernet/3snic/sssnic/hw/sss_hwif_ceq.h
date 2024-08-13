/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWIF_CEQ_H
#define SSS_HWIF_CEQ_H

#include "sss_hw_ceq.h"
#include "sss_ceq_info.h"
#include "sss_hwdev.h"

int sss_ceq_register_cb(void *hwdev, void *data,
			enum sss_ceq_event ceq_event, sss_ceq_event_handler_t event_handler);
void sss_ceq_unregister_cb(void *hwdev, enum sss_ceq_event ceq_event);
int sss_hwif_init_ceq(struct sss_hwdev *hwdev);
void sss_hwif_deinit_ceq(struct sss_hwdev *hwdev);
void sss_dump_ceq_info(struct sss_hwdev *hwdev);
int sss_init_ceq_msix_attr(struct sss_hwdev *hwdev);

#endif

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWIF_AEQ_H
#define SSS_HWIF_AEQ_H

#include "sss_hw_irq.h"
#include "sss_hw_aeq.h"
#include "sss_hwdev.h"
#include "sss_aeq_info.h"

void sss_deinit_aeq(struct sss_hwdev *hwdev);
void sss_get_aeq_irq(struct sss_hwdev *hwdev,
		     struct sss_irq_desc *irq_array, u16 *irq_num);
void sss_dump_aeq_info(struct sss_hwdev *hwdev);
int sss_aeq_register_hw_cb(void *hwdev, void *pri_handle,
			   enum sss_aeq_hw_event event, sss_aeq_hw_event_handler_t event_handler);
void sss_aeq_unregister_hw_cb(void *hwdev, enum sss_aeq_hw_event event);
int sss_aeq_register_swe_cb(void *hwdev, void *pri_handle,
			    enum sss_aeq_sw_event event,
			    sss_aeq_sw_event_handler_t sw_event_handler);
void sss_aeq_unregister_swe_cb(void *hwdev, enum sss_aeq_sw_event event);
int sss_hwif_init_aeq(struct sss_hwdev *hwdev);
void sss_hwif_deinit_aeq(struct sss_hwdev *hwdev);
int sss_init_aeq_msix_attr(struct sss_hwdev *hwdev);
u8 sss_sw_aeqe_handler(void *dev, u8 aeq_event, u8 *data);

#endif

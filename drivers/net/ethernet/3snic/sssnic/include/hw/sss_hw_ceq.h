/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HW_CEQ_H
#define SSS_HW_CEQ_H

enum sss_ceq_event {
	SSS_NIC_CTRLQ = 0x3,
	SSS_NIC_SQ,
	SSS_NIC_RQ,
	SSS_CEQ_EVENT_MAX,
};

#endif

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HW_AEQ_H
#define SSS_HW_AEQ_H

enum sss_aeq_hw_event {
	SSS_HW_FROM_INT = 0,
	SSS_MBX_FROM_FUNC = 1,
	SSS_MSG_FROM_MGMT = 2,
	SSS_ADM_RSP = 3,
	SSS_ADM_MSG_STS = 4,
	SSS_MBX_SEND_RSLT = 5,
	SSS_AEQ_EVENT_MAX
};

enum sss_aeq_sw_event {
	SSS_STL_EVENT = 0,
	SSS_STF_EVENT = 1,
	SSS_AEQ_SW_EVENT_MAX
};

enum sss_ucode_event_type {
	SSS_INTERN_ERR = 0x0,
	SSS_CHN_BUSY = 0x7,
	SSS_ERR_MAX = 0x8,
};

#endif

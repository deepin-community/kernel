/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWIF_MGMT_COMMON_H
#define SSS_HWIF_MGMT_COMMON_H

#define SSS_ASYNC_MSG_FLAG			0x8

#define SSS_PF_MGMT_BUF_LEN_MAX	2048UL

#define SSS_MSG_TO_MGMT_LEN_MAX		2016

#define SSS_SEG_LEN					48

#define SSS_MGMT_SEQ_ID_MAX			\
		(ALIGN(SSS_MSG_TO_MGMT_LEN_MAX, SSS_SEG_LEN) / SSS_SEG_LEN)

#define SSS_MGMT_LAST_SEG_LEN_MAX	\
			(SSS_PF_MGMT_BUF_LEN_MAX - SSS_SEG_LEN * SSS_MGMT_SEQ_ID_MAX)

#endif

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_TOOL_SM_H
#define SSS_TOOL_SM_H
#include "sss_pci_global.h"
#include "sss_tool_comm.h"
#include "sss_tool_hw.h"

#ifndef HTONL
#define HTONL(x) \
	((((x) & 0x000000ff) << 24) | \
	 (((x) & 0x0000ff00) << 8) | \
	 (((x) & 0x00ff0000) >> 8) | \
	 (((x) & 0xff000000) >> 24))
#endif

int sss_tool_msg_to_sm(struct sss_hal_dev *hal_dev, struct sss_tool_msg *msg,
		       void *in_buf, u32 in_len, void *out_buf, u32 *out_len);

#endif

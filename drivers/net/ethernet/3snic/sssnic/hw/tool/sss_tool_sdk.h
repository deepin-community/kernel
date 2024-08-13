/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_TOOL_SDK_H
#define SSS_TOOL_SDK_H

#include "sss_tool_comm.h"
#include "sss_tool_hw.h"
#include "sss_hw.h"

long sss_tool_free_card_mem(int id);

int sss_tool_msg_to_hw(struct sss_hal_dev *hal_dev, struct sss_tool_msg *tool_msg,
		       void *buf_in, u32 in_size, void *buf_out, u32 *out_size);

#endif

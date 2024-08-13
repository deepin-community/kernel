/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_TOOL_CHIP_H
#define SSS_TOOL_CHIP_H
#include "sss_hw.h"
#include "sss_tool_comm.h"
#include "sss_tool_hw.h"

int sss_tool_msg_to_mpu(struct sss_hal_dev *hal_dev, struct sss_tool_msg *tool_msg,
			void *buf_in, u32 in_size, void *buf_out, u32 *out_size);
int sss_tool_msg_to_npu(struct sss_hal_dev *hal_dev, struct sss_tool_msg *tool_msg,
			void *buf_in, u32 in_size, void *buf_out, u32 *out_size);

#endif

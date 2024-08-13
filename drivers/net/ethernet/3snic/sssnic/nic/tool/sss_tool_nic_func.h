/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_TOOL_NIC_FUNC_H
#define SSS_TOOL_NIC_FUNC_H

int sss_tool_ioctl(void *uld_dev, u32 cmd, const void *in_buf,
		   u32 in_len, void *out_buf, u32 *out_len);

#endif

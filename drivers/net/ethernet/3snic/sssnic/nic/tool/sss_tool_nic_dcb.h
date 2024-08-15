/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_TOOL_NIC_DCB_H
#define SSS_TOOL_NIC_DCB_H

int sss_tool_dcb_mt_qos_map(struct sss_nic_dev *nic_dev, const void *in_buf,
			    u32 in_len, void *out_buf, u32 *out_len);

int sss_tool_dcb_mt_dcb_state(struct sss_nic_dev *nic_dev, const void *in_buf,
			      u32 in_len, void *out_buf, u32 *out_len);

int sss_tool_dcb_mt_hw_qos_get(struct sss_nic_dev *nic_dev, const void *in_buf,
			       u32 in_len, void *out_buf, u32 *out_len);

#endif

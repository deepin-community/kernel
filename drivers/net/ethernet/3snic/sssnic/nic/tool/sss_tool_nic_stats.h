/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_TOOL_NIC_STATS_H
#define SSS_TOOL_NIC_STATS_H

int sss_tool_clear_func_stats(struct sss_nic_dev *nic_dev, const void *in_buf,
			      u32 in_len, void *out_buf, u32 *out_len);

int sss_tool_get_sset_count(struct sss_nic_dev *nic_dev, const void *in_buf,
			    u32 in_len, void *out_buf, u32 *out_len);

int sss_tool_get_sset_stats(struct sss_nic_dev *nic_dev, const void *in_buf,
			    u32 in_len, void *out_buf, u32 *out_len);

#endif

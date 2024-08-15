/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_TOOL_NIC_QP_INFO_H
#define SSS_TOOL_NIC_QP_INFO_H

int sss_tool_get_tx_info(struct sss_nic_dev *nic_dev, const void *in_buf,
			 u32 in_len, void *out_buf, u32 *out_len);

int sss_tool_get_rx_info(struct sss_nic_dev *nic_dev, const void *in_buf,
			 u32 in_len, void *out_buf, u32 *out_len);

int sss_tool_get_tx_wqe_info(struct sss_nic_dev *nic_dev, const void *in_buf,
			     u32 in_len, void *out_buf, u32 *out_len);

int sss_tool_get_rx_wqe_info(struct sss_nic_dev *nic_dev, const void *in_buf,
			     u32 in_len, void *out_buf, u32 *out_len);

int sss_tool_get_rx_cqe_info(struct sss_nic_dev *nic_dev, const void *in_buf,
			     u32 in_len, void *out_buf, u32 *out_len);

int sss_tool_get_q_num(struct sss_nic_dev *nic_dev, const void *in_buf, u32 in_len,
		       void *out_buf, u32 *out_len);

int sss_tool_get_inter_num(struct sss_nic_dev *nic_dev, const void *in_buf,
			   u32 in_len, void *out_buf, u32 *out_len);

#endif

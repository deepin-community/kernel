/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_TOOL_NIC_PHY_ATTR_H
#define SSS_TOOL_NIC_PHY_ATTR_H

int sss_tool_get_loopback_mode(struct sss_nic_dev *nic_dev, const void *in_buf,
			       u32 in_len, void *out_buf, u32 *out_len);

int sss_tool_set_loopback_mode(struct sss_nic_dev *nic_dev, const void *in_buf,
			       u32 in_len, void *out_buf, u32 *out_len);

int sss_tool_set_link_mode(struct sss_nic_dev *nic_dev, const void *in_buf,
			   u32 in_len, void *out_buf, u32 *out_len);

int sss_tool_set_pf_bw_limit(struct sss_nic_dev *nic_dev, const void *in_buf,
			     u32 in_len, void *out_buf, u32 *out_len);

int sss_tool_get_pf_bw_limit(struct sss_nic_dev *nic_dev, const void *in_buf,
			     u32 in_len, void *out_buf, u32 *out_len);

int sss_tool_get_netdev_name(struct sss_nic_dev *nic_dev, const void *in_buf,
			     u32 in_len, void *out_buf, u32 *out_len);

int sss_tool_get_netdev_tx_timeout(struct sss_nic_dev *nic_dev, const void *in_buf,
				   u32 in_len, void *out_buf, u32 *out_len);

int sss_tool_set_netdev_tx_timeout(struct sss_nic_dev *nic_dev, const void *in_buf,
				   u32 in_len, void *out_buf, u32 *out_len);

int sss_tool_get_xsfp_present(struct sss_nic_dev *nic_dev, const void *in_buf,
			      u32 in_len, void *out_buf, u32 *out_len);

int sss_tool_get_xsfp_info(struct sss_nic_dev *nic_dev, const void *in_buf,
			   u32 in_len, void *out_buf, u32 *out_len);

#endif

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_NTUPLE_H
#define SSS_NIC_NTUPLE_H

#include <linux/types.h>
#include <linux/ethtool.h>

#include "sss_nic_dev_define.h"

void sss_nic_flush_tcam(struct sss_nic_dev *nic_dev);

int sss_nic_ethtool_update_flow(struct sss_nic_dev *nic_dev,
				struct ethtool_rx_flow_spec *fs);

int sss_nic_ethtool_delete_flow(struct sss_nic_dev *nic_dev, u32 location);

int sss_nic_ethtool_get_flow(const struct sss_nic_dev *nic_dev,
			     struct ethtool_rxnfc *info, u32 location);

int sss_nic_ethtool_get_all_flows(const struct sss_nic_dev *nic_dev,
				  struct ethtool_rxnfc *info, u32 *rule_locs);

bool sss_nic_validate_channel_setting_in_ntuple(const struct sss_nic_dev *nic_dev, u32 q_num);

#endif

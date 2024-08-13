/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_DCB_H
#define SSS_NIC_DCB_H

#include "sss_kernel.h"
#include "sss_nic_dcb_define.h"

enum SSSNIC_DCB_FLAGS {
	SSSNIC_DCB_UP_COS_SETTING,
	SSSNIC_DCB_TRAFFIC_STOPPED,
};

enum sss_nic_dcb_trust {
	DCB_PCP,
	DCB_DSCP,
};

u8 sss_nic_get_user_cos_num(struct sss_nic_dev *nic_dev);
u8 sss_nic_get_valid_cos_map(struct sss_nic_dev *nic_dev);
int sss_nic_dcb_init(struct sss_nic_dev *nic_dev);
int sss_nic_update_dcb_cfg(struct sss_nic_dev *nic_dev);
void sss_nic_update_sq_cos(struct sss_nic_dev *nic_dev, u8 dcb_en);
void sss_nic_update_qp_cos_map(struct sss_nic_dev *nic_dev, u8 cos_num);
void sss_nic_sync_dcb_cfg(struct sss_nic_dev *nic_dev,
			  const struct sss_nic_dcb_config *dcb_config);

#endif

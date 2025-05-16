/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_DCB_H
#define HINIC3_DCB_H

#include "ossl_knl.h"

enum HINIC3_DCB_FLAGS {
	HINIC3_DCB_UP_COS_SETTING,
	HINIC3_DCB_TRAFFIC_STOPPED,
};

struct hinic3_cos_cfg {
	u8	up;
	u8	bw_pct;
	u8	tc_id;
	u8	prio_sp; /* 0 - DWRR, 1 - SP */
};

struct hinic3_tc_cfg {
	u8	bw_pct;
	u8	prio_sp; /* 0 - DWRR, 1 - SP */
	u16	rsvd;
};

#define PCP_MAX_UP 8
#define DSCP_MAC_UP 64
#define DBG_DFLT_DSCP_VAL  0xFF

struct hinic3_dcb_config {
	u8	trust; /* pcp, dscp */
	u8	default_cos;
	u8	pcp_user_cos_num;
	u8	pcp_valid_cos_map;
	u8	dscp_user_cos_num;
	u8	dscp_valid_cos_map;
	u8	pcp2cos[PCP_MAX_UP];
	u8	dscp2cos[DSCP_MAC_UP];

	u8	cos_qp_offset[NIC_DCB_COS_MAX];
	u8	cos_qp_num[NIC_DCB_COS_MAX];
};

u8 hinic3_get_dev_user_cos_num(struct hinic3_nic_dev *nic_dev);
u8 hinic3_get_dev_valid_cos_map(struct hinic3_nic_dev *nic_dev);
int hinic3_dcb_init(struct hinic3_nic_dev *nic_dev);
void hinic3_dcb_deinit(struct hinic3_nic_dev *nic_dev);
void hinic3_dcb_reset_hw_config(struct hinic3_nic_dev *nic_dev);
int hinic3_configure_dcb(struct net_device *netdev);
int hinic3_setup_cos(struct net_device *netdev, u8 cos, u8 netif_run);
void hinic3_dcbcfg_set_pfc_state(struct hinic3_nic_dev *nic_dev, u8 pfc_state);
u8 hinic3_dcbcfg_get_pfc_state(struct hinic3_nic_dev *nic_dev);
void hinic3_dcbcfg_set_pfc_pri_en(struct hinic3_nic_dev *nic_dev,
				  u8 pfc_en_bitmap);
u8 hinic3_dcbcfg_get_pfc_pri_en(struct hinic3_nic_dev *nic_dev);
int hinic3_dcbcfg_set_ets_up_tc_map(struct hinic3_nic_dev *nic_dev,
				    const u8 *up_tc_map);
void hinic3_dcbcfg_get_ets_up_tc_map(struct hinic3_nic_dev *nic_dev,
				     u8 *up_tc_map);
int hinic3_dcbcfg_set_ets_tc_bw(struct hinic3_nic_dev *nic_dev,
				const u8 *tc_bw);
void hinic3_dcbcfg_get_ets_tc_bw(struct hinic3_nic_dev *nic_dev, u8 *tc_bw);
void hinic3_dcbcfg_set_ets_tc_prio_type(struct hinic3_nic_dev *nic_dev,
					u8 tc_prio_bitmap);
void hinic3_dcbcfg_get_ets_tc_prio_type(struct hinic3_nic_dev *nic_dev,
					u8 *tc_prio_bitmap);
int hinic3_dcbcfg_set_up_bitmap(struct hinic3_nic_dev *nic_dev);
void hinic3_update_tx_db_cos(struct hinic3_nic_dev *nic_dev, u8 dcb_en);

void hinic3_update_qp_cos_cfg(struct hinic3_nic_dev *nic_dev, u8 num_cos);
void hinic3_vport_down(struct hinic3_nic_dev *nic_dev);
int hinic3_vport_up(struct hinic3_nic_dev *nic_dev);
int hinic3_configure_dcb_hw(struct hinic3_nic_dev *nic_dev, u8 dcb_en);
#endif

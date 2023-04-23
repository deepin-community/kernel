/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright (c) 2023 Hisilicon Limited. */

#ifndef __HNS3_EXT_H
#define __HNS3_EXT_H
#include <linux/types.h>
#include "hns3_enet.h"
#include "hnae3_ext.h"

#define HNS3_PFC_STORM_PARA_DIR_RX 0
#define HNS3_PFC_STORM_PARA_DIR_TX 1
#define HNS3_PFC_STORM_PARA_DISABLE 0
#define HNS3_PFC_STORM_PARA_ENABLE 1
#define HNS3_PFC_STORM_PARA_PERIOD_MIN 5
#define HNS3_PFC_STORM_PARA_PERIOD_MAX 2000
#define HNS3_MAX_TX_TIMEOUT 600

#define nic_set_8211_phy_reg nic_set_phy_reg
#define nic_get_8211_phy_reg nic_get_phy_reg
#define nic_set_8521_phy_reg(ndev, page, reg_addr, data) \
	nic_set_phy_reg(ndev, 0, page, reg_addr, data)
#define nic_get_8521_phy_reg(ndev, page, reg_addr, data) \
	nic_get_phy_reg(ndev, 0, page, reg_addr, data)

#define nic_get_cdr_flash_status(ndev, status)	\
	nic_get_port_fault_status(ndev, HNAE3_FAULT_TYPE_CDR_FLASH, status)
#define nic_get_hilink_ref_los(ndev, status)	\
	nic_get_port_fault_status(ndev, HNAE3_FAULT_TYPE_HILINK_REF_LOS, status)

int nic_netdev_match_check(struct net_device *netdev);
void nic_chip_recover_handler(struct net_device *ndev,
			      enum hnae3_event_type_custom event_t);
int nic_set_pfc_storm_para(struct net_device *ndev, u32 dir, u32 enable,
			   u32 period_ms, u32 times, u32 recovery_period_ms);
int nic_get_pfc_storm_para(struct net_device *ndev, u32 dir, u32 *enable,
			   u32 *period_ms, u32 *times, u32 *recovery_period_ms);
int nic_set_notify_pkt_param(struct net_device *ndev,
			     struct hnae3_notify_pkt_param *param);
int nic_set_notify_pkt_start(struct net_device *ndev);
int nic_set_torus_param(struct net_device *ndev, struct hnae3_torus_param *param);
int nic_get_torus_param(struct net_device *ndev, struct hnae3_torus_param *param);
int nic_clean_stats64(struct net_device *ndev, struct rtnl_link_stats64 *stats);
int nic_set_cpu_affinity(struct net_device *ndev, cpumask_t *affinity_mask);
int nic_get_chipid(struct net_device *ndev, u32 *chip_id);
int nic_get_mac_id(struct net_device *ndev, u32 *mac_id);
int nic_get_io_die_id(struct net_device *ndev, u32 *io_die_id);
int nic_get_chip_num(struct net_device *ndev, u32 *chip_num);
int nic_get_io_die_num(struct net_device *ndev, u32 *io_die_num);
int nic_get_port_num_of_die(struct net_device *ndev, u32 *port_num);
int nic_get_port_num_per_chip(struct net_device *ndev, u32 *port_num);
int nic_set_tx_timeout(struct net_device *ndev, int tx_timeout);
int nic_get_sfp_present(struct net_device *ndev, int *present);
int nic_set_sfp_state(struct net_device *ndev, bool en);
int nic_disable_net_lane(struct net_device *ndev);
int nic_get_net_lane_status(struct net_device *ndev, u32 *status);
int nic_disable_clock(struct net_device *ndev);
int nic_set_pfc_time_cfg(struct net_device *ndev, u16 time);
int nic_get_port_fault_status(struct net_device *ndev, u32 fault_type, u32 *status);
int nic_get_port_wire_type(struct net_device *ndev, u32 *wire_type);
int nic_set_mac_state(struct net_device *ndev, int enable);
int nic_set_led(struct net_device *ndev, u32 type, u32 status);
int nic_get_led_signal(struct net_device *ndev, struct hnae3_lamp_signal *signal);
int nic_get_phy_reg(struct net_device *ndev, u32 page_select_addr,
		    u16 page, u32 reg_addr, u16 *data);
int nic_set_phy_reg(struct net_device *ndev, u32 page_select_addr,
		    u16 page, u32 reg_addr, u16 data);
#endif

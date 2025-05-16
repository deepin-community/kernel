/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2022. All rights reserved.
 * @file hinic3_srv_nic.h
 * @details nic service interface
 * History       :
 * 1.Date        : 2018/3/8
 *   Modification: Created file
 */

#ifndef HINIC3_SRV_NIC_H
#define HINIC3_SRV_NIC_H

#include <linux/netdevice.h>
#include "mag_mpu_cmd.h"
#include "mag_mpu_cmd_defs.h"
#include "hinic3_lld.h"
#include "nic_cfg_comm.h"
#include "nic_mpu_cmd_defs.h"

enum hinic3_queue_type {
	HINIC3_SQ,
	HINIC3_RQ,
	HINIC3_MAX_QUEUE_TYPE
};

struct hinic3_lld_dev *hinic3_get_lld_dev_by_netdev(struct net_device *netdev);
struct net_device *hinic3_get_netdev_by_lld(struct hinic3_lld_dev *lld_dev);

struct hinic3_event_link_info {
	u8 valid;
	u8 port_type;
	u8 autoneg_cap;
	u8 autoneg_state;
	u8 duplex;
	u8 speed;
};

enum link_err_type {
	LINK_ERR_MODULE_UNRECOGENIZED,
	LINK_ERR_NUM,
};

enum port_module_event_type {
	HINIC3_PORT_MODULE_CABLE_PLUGGED,
	HINIC3_PORT_MODULE_CABLE_UNPLUGGED,
	HINIC3_PORT_MODULE_LINK_ERR,
	HINIC3_PORT_MODULE_MAX_EVENT,
};

struct hinic3_port_module_event {
	enum port_module_event_type type;
	enum link_err_type err_type;
};

struct hinic3_dcb_info {
	u8 dcb_on;
	u8 default_cos;
	u8 up_cos[NIC_DCB_COS_MAX];
};

enum hinic3_nic_event_type {
	EVENT_NIC_LINK_DOWN,
	EVENT_NIC_LINK_UP,
	EVENT_NIC_PORT_MODULE_EVENT,
	EVENT_NIC_DCB_STATE_CHANGE,
	EVENT_NIC_BOND_DOWN,
	EVENT_NIC_BOND_UP,
	EVENT_NIC_OUTBAND_CFG,
};

/* *
 * @brief hinic3_set_mac - set mac address
 * @param hwdev: device pointer to hwdev
 * @param mac_addr: mac address from hardware
 * @param vlan_id: vlan id
 * @param func_id: function index
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_set_mac(void *hwdev, const u8 *mac_addr, u16 vlan_id, u16 func_id, u16 channel);

/* *
 * @brief hinic3_del_mac - delete mac address
 * @param hwdev: device pointer to hwdev
 * @param mac_addr: mac address from hardware
 * @param vlan_id: vlan id
 * @param func_id: function index
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_del_mac(void *hwdev, const u8 *mac_addr, u16 vlan_id, u16 func_id, u16 channel);

/* *
 * @brief hinic3_set_vport_enable - set function valid status
 * @param hwdev: device pointer to hwdev
 * @param func_id: global function index
 * @param enable: 0-disable, 1-enable
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_set_vport_enable(void *hwdev, u16 func_id, bool enable, u16 channel);

/* *
 * @brief hinic3_set_port_enable - set port status
 * @param hwdev: device pointer to hwdev
 * @param enable: 0-disable, 1-enable
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_set_port_enable(void *hwdev, bool enable, u16 channel);

/* *
 * @brief hinic3_flush_qps_res - flush queue pairs resource in hardware
 * @param hwdev: device pointer to hwdev
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_flush_qps_res(void *hwdev);

/* *
 * @brief hinic3_cache_out_qps_res - cache out queue pairs wqe resource in hardware
 * @param hwdev: device pointer to hwdev
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_cache_out_qps_res(void *hwdev);

/* *
 * @brief hinic3_init_nic_hwdev - init nic hwdev
 * @param hwdev: device pointer to hwdev
 * @param pcidev_hdl: pointer to pcidev or handler
 * @param dev_hdl: pointer to pcidev->dev or handler, for sdk_err() or
 * dma_alloc()
 * @param rx_buff_len: rx_buff_len is receive buffer length
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_init_nic_hwdev(void *hwdev, void *pcidev_hdl, void *dev_hdl, u16 rx_buff_len);

/* *
 * @brief hinic3_free_nic_hwdev - free nic hwdev
 * @param hwdev: device pointer to hwdev
 * @retval zero: success
 * @retval non-zero: failure
 */
void hinic3_free_nic_hwdev(void *hwdev);

/* *
 * @brief hinic3_get_speed - set link speed
 * @param hwdev: device pointer to hwdev
 * @param port_info: link speed
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_get_speed(void *hwdev, enum mag_cmd_port_speed *speed, u16 channel);

int hinic3_get_dcb_state(void *hwdev, struct hinic3_dcb_state *dcb_state);

int hinic3_get_pf_dcb_state(void *hwdev, struct hinic3_dcb_state *dcb_state);

int hinic3_get_cos_by_pri(void *hwdev, u8 pri, u8 *cos);

/* *
 * @brief hinic3_create_qps - create queue pairs
 * @param hwdev: device pointer to hwdev
 * @param num_qp: number of queue pairs
 * @param sq_depth: sq depth
 * @param rq_depth: rq depth
 * @param qps_msix_arry: msix info
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_create_qps(void *hwdev, u16 num_qp, u32 sq_depth, u32 rq_depth,
		      struct irq_info *qps_msix_arry);

/* *
 * @brief hinic3_destroy_qps - destroy queue pairs
 * @param hwdev: device pointer to hwdev
 */
void hinic3_destroy_qps(void *hwdev);

/* *
 * @brief hinic3_get_nic_queue - get nic queue
 * @param hwdev: device pointer to hwdev
 * @param q_id: queue index
 * @param q_type: queue type
 * @retval queue address
 */
void *hinic3_get_nic_queue(void *hwdev, u16 q_id, enum hinic3_queue_type q_type);

/* *
 * @brief hinic3_init_qp_ctxts - init queue pair context
 * @param hwdev: device pointer to hwdev
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_init_qp_ctxts(void *hwdev);

/* *
 * @brief hinic3_free_qp_ctxts - free queue pairs
 * @param hwdev: device pointer to hwdev
 */
void hinic3_free_qp_ctxts(void *hwdev);

/* *
 * @brief  hinic3_pf_set_vf_link_state pf set vf link state
 * @param hwdev: device pointer to hwdev
 * @param vf_link_forced: set link forced
 * @param link_state: Set link state, This parameter is valid only when vf_link_forced is true
 * @retval zero: success
 * @retval non-zero: failure
 */
int hinic3_pf_set_vf_link_state(void *hwdev, bool vf_link_forced, bool link_state);

#endif

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HW_EXPORT_H
#define SSS_HW_EXPORT_H

#include <linux/types.h>

#include "sss_hw_irq.h"
#include "sss_hw_svc_cap.h"
#include "sss_hw_event.h"

int sss_chip_set_msix_attr(void *hwdev,
			   struct sss_irq_cfg intr_cfg, u16 channel);

/* *
 * @brief sss_chip_clear_msix_resend_bit - clear msix resend bit
 * @param hwdev: device pointer to hwdev
 * @param msix_id: msix id
 * @param clear_en: 1-clear
 */
void sss_chip_clear_msix_resend_bit(void *hwdev, u16 msix_id, bool clear_en);

/**
 * @brief sss_chip_reset_function - reset func
 * @param hwdev: device pointer to hwdev
 * @param func_id: global function index
 * @param flag: reset flag
 * @param channel: channel id
 */
int sss_chip_reset_function(void *hwdev, u16 func_id, u64 flag, u16 channel);

/**
 * @brief sss_chip_set_root_ctx - set root context
 * @param hwdev: device pointer to hwdev
 * @param rq_depth: rq depth
 * @param sq_depth: sq depth
 * @param rx_size: rx buffer size
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 **/
int sss_chip_set_root_ctx(void *hwdev,
			  u32 rq_depth, u32 sq_depth, int rx_size, u16 channel);

/**
 * @brief sss_chip_clean_root_ctx - clean root context
 * @param hwdev: device pointer to hwdev
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 **/
int sss_chip_clean_root_ctx(void *hwdev, u16 channel);

/* *
 * @brief sss_get_mgmt_version - get management cpu version
 * @param hwdev: device pointer to hwdev
 * @param buf: output management version
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int sss_get_mgmt_version(void *hwdev, u8 *buf, u8 buf_size, u16 channel);

/**
 * @brief sss_chip_set_func_used_state - set function service used state
 * @param hwdev: device pointer to hwdev
 * @param service_type: service type
 * @param state: function used state
 * @param channel: channel id
 * @retval zero: success
 * @retval non-zero: failure
 */
int sss_chip_set_func_used_state(void *hwdev,
				 u16 service_type, bool state, u16 channel);

bool sss_get_nic_capability(void *hwdev, struct sss_nic_service_cap *capability);

/* *
 * @brief sss_support_nic - function support nic
 * @param hwdev: device pointer to hwdev
 * @param cap: nic service capbility
 * @retval true: function support nic
 * @retval false: function not support nic
 */
bool sss_support_nic(void *hwdev);

bool sss_support_ppa(void *hwdev, struct sss_ppa_service_cap *cap);

/* *
 * @brief sss_get_max_sq_num - get max queue number
 * @param hwdev: device pointer to hwdev
 * @retval non-zero: max queue number
 * @retval zero: failure
 */
u16 sss_get_max_sq_num(void *hwdev);

/* *
 * @brief sss_get_phy_port_id - get physical port id
 * @param hwdev: device pointer to hwdev
 * @retval physical port id
 */
u8 sss_get_phy_port_id(void *hwdev); /* Obtain sss_service_cap.port_id */

/* *
 * @brief sss_get_max_vf_num - get vf number
 * @param hwdev: device pointer to hwdev
 * @retval non-zero: vf number
 * @retval zero: failure
 */
u16 sss_get_max_vf_num(void *hwdev); /* Obtain sss_service_cap.max_vf */

/* *
 * @brief sss_get_cos_valid_bitmap - get cos valid bitmap
 * @param hwdev: device pointer to hwdev
 * @retval non-zero: valid cos bit map
 * @retval zero: failure
 */
int sss_get_cos_valid_bitmap(void *hwdev, u8 *func_cos_bitmap, u8 *port_cos_bitmap);

/* *
 * @brief sss_alloc_irq - alloc irq
 * @param hwdev: device pointer to hwdev
 * @param service_type: service type
 * @param alloc_array: alloc irq info
 * @param alloc_num: alloc number
 * @retval zero: failure
 * @retval non-zero: success
 */
u16 sss_alloc_irq(void *hwdev, enum sss_service_type service_type,
		  struct sss_irq_desc *alloc_array, u16 alloc_num);

/* *
 * @brief sss_free_irq - free irq
 * @param hwdev: device pointer to hwdev
 * @param service_type: service type
 * @param irq_id: irq id
 */
void sss_free_irq(void *hwdev, enum sss_service_type service_type, u32 irq_id);

/* *
 * @brief sss_register_dev_event - register hardware event
 * @param hwdev: device pointer to hwdev
 * @param data: private data will be used by the callback
 * @param callback: callback function
 */
void sss_register_dev_event(void *hwdev, void *data, sss_event_handler_t callback);

/* *
 * @brief sss_unregister_dev_event - unregister hardware event
 * @param dev: device pointer to hwdev
 */
void sss_unregister_dev_event(void *dev);

/* *
 * @brief sss_get_dev_present_flag - get chip present flag
 * @param hwdev: device pointer to hwdev
 * @retval 1: chip is present
 * @retval 0: chip is absent
 */
int sss_get_dev_present_flag(const void *hwdev);

/* *
 * @brief sss_get_max_pf_num - get global max pf number
 */
u8 sss_get_max_pf_num(void *hwdev);

u16 sss_nic_intr_num(void *hwdev);

/* *
 * @brief sss_get_chip_present_state - get card present state
 * @param hwdev: device pointer to hwdev
 * @param present_state: return card present state
 * @retval zero: success
 * @retval non-zero: failure
 */
int sss_get_chip_present_state(void *hwdev, bool *present_state);

/**
 * @brief sss_fault_event_report - report fault event
 * @param hwdev: device pointer to hwdev
 * @param src: fault event source, reference to enum sss_fault_source_type
 * @param level: fault level, reference to enum sss_fault_err_level
 */
void sss_fault_event_report(void *hwdev, u16 src, u16 level);

/**
 * @brief sss_register_service_adapter - register service adapter
 * @param hwdev: device pointer to hwdev
 * @param service_type: service type
 * @param service_adapter: service adapter
 * @retval zero: success
 * @retval non-zero: failure
 **/
int sss_register_service_adapter(void *hwdev, enum sss_service_type service_type,
				 void *service_adapter);

/**
 * @brief sss_unregister_service_adapter - unregister service adapter
 * @param hwdev: device pointer to hwdev
 * @param service_type: service type
 **/
void sss_unregister_service_adapter(void *hwdev,
				    enum sss_service_type service_type);

/**
 * @brief sss_get_service_adapter - get service adapter
 * @param hwdev: device pointer to hwdev
 * @param service_type: service type
 * @retval non-zero: success
 * @retval null: failure
 **/
void *sss_get_service_adapter(void *hwdev, enum sss_service_type service_type);

/**
 * @brief sss_do_event_callback - evnet callback to notify service driver
 * @param hwdev: device pointer to hwdev
 * @param event: event info to service driver
 */
void sss_do_event_callback(void *hwdev, struct sss_event_info *event);

/**
 * @brief sss_update_link_stats - link event stats
 * @param hwdev: device pointer to hwdev
 * @param link_state: link status
 */
void sss_update_link_stats(void *hwdev, bool link_state);
#endif

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_MAG_CFG_H
#define SSS_NIC_MAG_CFG_H

#include <linux/types.h>

#include "sss_nic_cfg_mag_define.h"
#include "sss_nic_io_define.h"
#include "sss_nic_dev_define.h"

enum port_module_event_type {
	SSSNIC_PORT_MODULE_CABLE_PLUGGED,
	SSSNIC_PORT_MODULE_CABLE_UNPLUGGED,
	SSSNIC_PORT_MODULE_LINK_ERR,
	SSSNIC_PORT_MODULE_MAX_EVENT,
};

enum link_err_type {
	LINK_ERR_MODULE_UNRECOGENIZED,
	LINK_ERR_NUM,
};

struct sss_nic_port_module_event {
	enum port_module_event_type type;
	enum link_err_type err_type;
};

int sss_nic_set_hw_port_state(struct sss_nic_dev *nic_dev, bool enable, u16 channel);

int sss_nic_get_hw_link_state(struct sss_nic_dev *nic_dev, u8 *link_state);

void sss_nic_notify_all_vf_link_state(struct sss_nic_io *nic_io, u8 link_status);

int sss_nic_get_hw_port_info(struct sss_nic_dev *nic_dev, struct sss_nic_port_info *port_info,
			     u16 channel);

int sss_nic_get_phy_port_stats(struct sss_nic_dev *nic_dev, struct sss_nic_mag_port_stats *stats);

int sss_nic_set_link_settings(struct sss_nic_dev *nic_dev,
			      struct sss_nic_link_ksettings *settings);

int sss_nic_set_hw_led_state(struct sss_nic_dev *nic_dev, enum sss_nic_mag_led_type type,
			     enum sss_nic_mag_led_mode mode);

int sss_nic_set_loopback_mode(struct sss_nic_dev *nic_dev, u8 mode, u8 enable);

int sss_nic_set_autoneg(struct sss_nic_dev *nic_dev, bool enable);

int sss_nic_get_sfp_type(struct sss_nic_dev *nic_dev, u8 *sfp_type, u8 *sfp_type_ext);
int sss_nic_get_sfp_eeprom(struct sss_nic_dev *nic_dev, u8 *data, u32 len);

int sss_nic_set_link_follow_state(struct sss_nic_dev *nic_dev,
				  enum sss_nic_link_follow_status status);

void sss_nic_notify_vf_link_state(struct sss_nic_io *nic_io,
				  u16 vf_id, u8 link_status);

int sss_nic_vf_mag_event_handler(void *hwdev, u16 cmd,
				 void *buf_in, u16 in_size, void *buf_out,
				 u16 *out_size);

void sss_nic_pf_mag_event_handler(void *pri_handle, u16 cmd,
				  void *buf_in, u16 in_size, void *buf_out,
				  u16 *out_size);

int sss_nic_pf_mag_mbx_handler(void *hwdev,
			       u16 vf_id, u16 cmd, void *buf_in, u16 in_size,
			       void *buf_out, u16 *out_size);

int sss_nic_get_sfp_info(struct sss_nic_dev *nic_dev,
			 struct sss_nic_mbx_get_xsfp_info *xsfp_info);

bool sss_nic_if_sfp_absent(struct sss_nic_dev *nic_dev);

int sss_nic_get_loopback_mode(struct sss_nic_dev *nic_dev, u8 *mode, u8 *enable);

#endif

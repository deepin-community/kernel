// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/if_vlan.h>
#include <linux/ethtool.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/module.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_nic_io.h"
#include "sss_nic_cfg.h"
#include "sss_nic_mag_cfg.h"
#include "sss_nic_io_define.h"
#include "sss_nic_event.h"

struct sss_nic_event_link_info {
	u8 valid;
	u8 port_type;
	u8 autoneg_cap;
	u8 autoneg_state;
	u8 duplex;
	u8 speed;
};

#define SSSNIC_LOOP_MODE_MIN 1
#define SSSNIC_LOOP_MODE_MAX 6

#define SSSNIC_LOOP_MODE_IS_INVALID(mode) \
	(unlikely(((mode) > SSSNIC_LOOP_MODE_MAX) || ((mode) < SSSNIC_LOOP_MODE_MIN)))

#define SSSNIC_LINK_INFO_VALID 1

static int sss_nic_mag_msg_to_mgmt_sync(void *hwdev, u16 cmd, void *in_buf,
					u16 in_size, void *out_buf, u16 *out_size);
static int sss_nic_mag_msg_to_mgmt_sync_ch(void *hwdev, u16 cmd, void *in_buf,
					   u16 in_size, void *out_buf, u16 *out_size, u16 channel);

int sss_nic_set_hw_port_state(struct sss_nic_dev *nic_dev, bool enable, u16 channel)
{
	struct sss_nic_mbx_set_port_mag_state port_state = {0};
	u16 out_len = sizeof(port_state);
	int ret;

	if (!nic_dev)
		return -EINVAL;

	if (sss_get_func_type(nic_dev->hwdev) == SSS_FUNC_TYPE_VF)
		return 0;

	port_state.state = enable ? (SSSNIC_MAG_OPCODE_TX_ENABLE | SSSNIC_MAG_OPCODE_RX_ENABLE) :
			   SSSNIC_MAG_OPCODE_PORT_DISABLE;
	port_state.function_id = sss_get_global_func_id(nic_dev->hwdev);

	ret = sss_nic_mag_msg_to_mgmt_sync_ch(nic_dev->hwdev, SSSNIC_MAG_OPCODE_SET_PORT_ENABLE,
					      &port_state, sizeof(port_state),
					      &port_state, &out_len, channel);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &port_state)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to set port state, ret: %d, state: 0x%x, out_len: 0x%x, channel: 0x%x\n",
			ret, port_state.head.state, out_len, channel);
		return -EIO;
	}

	return 0;
}

int sss_nic_get_phy_port_stats(struct sss_nic_dev *nic_dev, struct sss_nic_mag_port_stats *stats)
{
	struct sss_nic_mbx_mag_port_stats_info stats_info = {0};
	struct sss_nic_mbx_mag_port_stats *port_stats = NULL;
	u16 out_len = sizeof(*port_stats);
	int ret;

	port_stats = kzalloc(sizeof(*port_stats), GFP_KERNEL);
	if (!port_stats)
		return -ENOMEM;

	stats_info.port_id = sss_get_phy_port_id(nic_dev->hwdev);

	ret = sss_nic_mag_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MAG_OPCODE_GET_PORT_STAT,
					   &stats_info, sizeof(stats_info),
					   port_stats, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, port_stats)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to get port statistics, ret: %d, state: 0x%x, out_len: 0x%x\n",
			ret, port_stats->head.state, out_len);
		ret = -EIO;
		goto out;
	}

	memcpy(stats, &port_stats->counter, sizeof(*stats));

out:
	kfree(port_stats);

	return ret;
}

int sss_nic_set_autoneg(struct sss_nic_dev *nic_dev, bool enable)
{
	struct sss_nic_link_ksettings settings = {0};
	u32 valid_bitmap = 0;

	valid_bitmap |= SSSNIC_LINK_SET_AUTONEG;
	settings.valid_bitmap = valid_bitmap;
	settings.autoneg = enable ? SSSNIC_PORT_CFG_AN_ON : SSSNIC_PORT_CFG_AN_OFF;

	return sss_nic_set_link_settings(nic_dev, &settings);
}

static int sss_nic_cfg_loopback_mode(struct sss_nic_dev *nic_dev, u8 opcode,
				     u8 *mode, u8 *enable)
{
	struct sss_nic_mbx_loopback_mode loopback_mode = {0};
	u16 out_len = sizeof(loopback_mode);
	int ret;

	if (opcode == SSS_MGMT_MSG_SET_CMD) {
		loopback_mode.mode = *mode;
		loopback_mode.en = *enable;
	}
	loopback_mode.opcode = opcode;
	loopback_mode.port_id = sss_get_phy_port_id(nic_dev->hwdev);

	ret = sss_nic_mag_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MAG_OPCODE_CFG_LOOPBACK_MODE,
					   &loopback_mode, sizeof(loopback_mode),
					   &loopback_mode, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &loopback_mode)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to %s loopback mode, ret: %d, state: 0x%x, out_len: 0x%x\n",
			opcode == SSS_MGMT_MSG_SET_CMD ? "set" : "get",
			ret, loopback_mode.head.state, out_len);
		return -EIO;
	}

	if (opcode == SSS_MGMT_MSG_GET_CMD) {
		*enable = loopback_mode.en;
		*mode = loopback_mode.mode;
	}

	return 0;
}

int sss_nic_set_loopback_mode(struct sss_nic_dev *nic_dev, u8 lp_mode, u8 enable)
{
	if (SSSNIC_LOOP_MODE_IS_INVALID(lp_mode)) {
		nic_err(nic_dev->dev_hdl, "Invalid loopback mode %u to set\n",
			lp_mode);
		return -EINVAL;
	}

	return sss_nic_cfg_loopback_mode(nic_dev, SSS_MGMT_MSG_SET_CMD, &lp_mode, &enable);
}

int sss_nic_get_loopback_mode(struct sss_nic_dev *nic_dev, u8 *mode, u8 *enable)
{
	if (!nic_dev || !mode || !enable)
		return -EINVAL;

	return sss_nic_cfg_loopback_mode(nic_dev, SSS_MGMT_MSG_GET_CMD, mode,
					enable);
}

int sss_nic_set_hw_led_state(struct sss_nic_dev *nic_dev, enum sss_nic_mag_led_type led_type,
			     enum sss_nic_mag_led_mode led_mode)
{
	struct sss_nic_mbx_set_led_cfg led_info = {0};
	u16 out_len = sizeof(led_info);
	int ret;

	led_info.mode = led_mode;
	led_info.type = led_type;
	led_info.function_id = sss_get_global_func_id(nic_dev->hwdev);

	ret = sss_nic_mag_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MAG_OPCODE_SET_LED_CFG,
					   &led_info, sizeof(led_info), &led_info, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &led_info)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to set led state, ret: %d, state: 0x%x, out_len: 0x%x\n",
			ret, led_info.head.state, out_len);
		return -EIO;
	}

	return 0;
}

int sss_nic_get_hw_port_info(struct sss_nic_dev *nic_dev,
			     struct sss_nic_port_info *port_info, u16 channel)
{
	struct sss_nic_mbx_get_port_info mbx_port_info = {0};
	u16 out_len = sizeof(mbx_port_info);
	int ret;

	if (!nic_dev || !port_info)
		return -EINVAL;

	mbx_port_info.port_id = sss_get_phy_port_id(nic_dev->hwdev);

	ret = sss_nic_mag_msg_to_mgmt_sync_ch(nic_dev->hwdev, SSSNIC_MAG_OPCODE_GET_PORT_INFO,
					      &mbx_port_info, sizeof(mbx_port_info),
					      &mbx_port_info, &out_len, channel);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &mbx_port_info)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to get port info, ret: %d, state: 0x%x, out_len: 0x%x, channel: 0x%x\n",
			ret, mbx_port_info.head.state, out_len, channel);
		return -EIO;
	}

	port_info->advertised_mode = mbx_port_info.advertised_mode;
	port_info->duplex = mbx_port_info.duplex;
	port_info->autoneg_cap = mbx_port_info.an_support;
	port_info->fec = mbx_port_info.fec;
	port_info->autoneg_state = mbx_port_info.an_en;
	port_info->port_type = mbx_port_info.wire_type;
	port_info->supported_mode = mbx_port_info.supported_mode;
	port_info->speed = mbx_port_info.speed;

	return 0;
}

int sss_nic_set_link_settings(struct sss_nic_dev *nic_dev,
			      struct sss_nic_link_ksettings *settings)
{
	struct sss_nic_mbx_mag_set_port_cfg port_cfg = {0};
	u16 out_len = sizeof(port_cfg);
	int ret;

	port_cfg.autoneg = settings->autoneg;
	port_cfg.port_id = sss_get_phy_port_id(nic_dev->hwdev);
	port_cfg.fec = settings->fec;
	port_cfg.config_bitmap = settings->valid_bitmap;
	port_cfg.speed = settings->speed;

	ret = sss_nic_mag_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MAG_OPCODE_SET_PORT_CFG,
					   &port_cfg, sizeof(port_cfg), &port_cfg, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &port_cfg)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to set link settings, ret: %d, state: 0x%x, out_len: 0x%x\n",
			ret, port_cfg.head.state, out_len);
		return -EIO;
	}

	return port_cfg.head.state;
}

int sss_nic_get_hw_link_state(struct sss_nic_dev *nic_dev, u8 *out_state)
{
	struct sss_nic_mbx_get_link_state link_state = {0};
	u16 out_len = sizeof(link_state);
	int ret;

	if (!nic_dev || !out_state)
		return -EINVAL;

	link_state.port_id = sss_get_phy_port_id(nic_dev->hwdev);

	ret = sss_nic_mag_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MAG_OPCODE_LINK_STATUS,
					   &link_state, sizeof(link_state), &link_state, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &link_state)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to get link state, ret: %d, state: 0x%x, out_len: 0x%x\n",
			ret, link_state.head.state, out_len);
		return -EIO;
	}

	*out_state = link_state.status;

	return 0;
}

void sss_nic_notify_vf_link_state(struct sss_nic_io *nic_io, u16 vf_id, u8 state)
{
	struct sss_nic_mbx_get_link_state link_state = {0};
	u16 out_len = sizeof(link_state);
	u16 id = SSSNIC_HW_VF_ID_TO_OS(vf_id);
	int ret;

	link_state.status = state;
	link_state.port_id = sss_get_phy_port_id(nic_io->hwdev);
	ret = sss_mbx_send_to_vf(nic_io->hwdev, vf_id, SSS_MOD_TYPE_SSSLINK,
				 SSSNIC_MAG_OPCODE_LINK_STATUS,
				 &link_state, sizeof(link_state),
				 &link_state, &out_len, 0, SSS_CHANNEL_NIC);
	if (ret == SSS_MBX_ERRCODE_UNKNOWN_DES_FUNC) {
		sss_nic_dettach_vf(nic_io, vf_id);
		nic_warn(nic_io->dev_hdl, "VF %d not initialize, need to disconnect it\n", id);
	} else if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &link_state)) {
		nic_err(nic_io->dev_hdl,
			"Fail to send VF %d the link state change event, ret:%d, state:0x%x, out_len:0x%x\n",
			id, ret, link_state.head.state, out_len);
	}
}

void sss_nic_notify_all_vf_link_state(struct sss_nic_io *nic_io, u8 state)
{
	struct sss_nic_vf_info *vf_info = NULL;
	u16 vf_id;

	nic_io->link_status = state;
	for (vf_id = 1; vf_id <= nic_io->max_vf_num; vf_id++) {
		vf_info = &nic_io->vf_info_group[SSSNIC_HW_VF_ID_TO_OS(vf_id)];
		if (vf_info->link_forced || !vf_info->attach)
			continue;
		sss_nic_notify_vf_link_state(nic_io, vf_id, state);
	}
}

static int sss_nic_get_vf_link_status_handler(struct sss_nic_io *nic_io,
					      u16 vf_id, void *buf_in, u16 in_len,
					      void *buf_out, u16 *out_len)
{
	u16 id = SSSNIC_HW_VF_ID_TO_OS(vf_id);
	struct sss_nic_mbx_get_link_state *link_state = buf_out;
	struct sss_nic_vf_info *vf_info_group = nic_io->vf_info_group;
	bool link_up = vf_info_group[id].link_up;
	bool link_forced = vf_info_group[id].link_forced;

	if (link_forced)
		link_state->status = link_up ? SSSNIC_LINK_UP : SSSNIC_LINK_DOWN;
	else
		link_state->status = nic_io->link_status;

	link_state->head.state = SSS_MGMT_CMD_SUCCESS;
	*out_len = sizeof(*link_state);

	return 0;
}

static void sss_nic_get_link_info(struct sss_nic_io *nic_io,
				  const struct sss_nic_mbx_get_link_state *link_state,
				  struct sss_nic_event_link_info *link_info)
{
	struct sss_nic_port_info port_info = {0};
	int ret;

	/* link event reported only after set vport enable */
	if (sss_get_func_type(nic_io->hwdev) == SSS_FUNC_TYPE_VF ||
	    link_state->status == SSSNIC_LINK_DOWN)
		return;

	ret = sss_nic_get_hw_port_info(nic_io->nic_dev, &port_info, SSS_CHANNEL_NIC);
	if (ret != 0) {
		nic_warn(nic_io->dev_hdl, "Fail to get port info\n");
		return;
	}

	link_info->valid = SSSNIC_LINK_INFO_VALID;
	link_info->duplex = port_info.duplex;
	link_info->port_type = port_info.port_type;
	link_info->speed = port_info.speed;
	link_info->autoneg_state = port_info.autoneg_state;
	link_info->autoneg_cap = port_info.autoneg_cap;
}

static void sss_nic_link_status_event_handler(struct sss_nic_io *nic_io,
					      void *buf_in, u16 in_len,
					      void *buf_out, u16 *out_len)
{
	struct sss_nic_mbx_get_link_state *in_link_state = buf_in;
	struct sss_nic_mbx_get_link_state *out_link_state = buf_out;
	struct sss_event_info event_info = {0};
	struct sss_nic_event_link_info *link_info = (void *)event_info.event_data;

	nic_info(nic_io->dev_hdl, "Link status report received, func_id: %u, status: %u\n",
		 sss_get_global_func_id(nic_io->hwdev), in_link_state->status);

	sss_update_link_stats(nic_io->hwdev, in_link_state->status);

	sss_nic_get_link_info(nic_io, in_link_state, link_info);

	event_info.type = (in_link_state->status == SSSNIC_LINK_DOWN) ?
			  SSSNIC_EVENT_LINK_DOWN : SSSNIC_EVENT_LINK_UP;
	event_info.service = SSS_EVENT_SRV_NIC;
	sss_do_event_callback(nic_io->hwdev, &event_info);

	if (sss_get_func_type(nic_io->hwdev) == SSS_FUNC_TYPE_VF)
		return;

	*out_len = sizeof(*out_link_state);
	out_link_state->head.state = SSS_MGMT_CMD_SUCCESS;
	sss_nic_notify_all_vf_link_state(nic_io, in_link_state->status);
}

static void sss_nic_cable_plug_event_handler(struct sss_nic_io *nic_io,
					     void *in_buf, u16 in_size,
					     void *out_buf, u16 *out_size)
{
	struct sss_nic_mag_wire_event *in_wire_event = in_buf;
	struct sss_nic_mag_wire_event *out_wire_event = out_buf;
	struct sss_nic_cache_port_sfp *routine_cmd = NULL;
	struct sss_event_info event_info = {0};
	struct sss_nic_port_module_event *module_event = (void *)event_info.event_data;

	routine_cmd = &nic_io->mag_cfg.rt_cmd;
	mutex_lock(&nic_io->mag_cfg.sfp_mutex);
	routine_cmd->mpu_send_sfp_info = false;
	routine_cmd->mpu_send_sfp_abs = false;
	mutex_unlock(&nic_io->mag_cfg.sfp_mutex);

	*out_size = sizeof(*out_wire_event);
	out_wire_event->head.state = SSS_MGMT_CMD_SUCCESS;

	event_info.service = SSS_EVENT_SRV_NIC;
	event_info.type = SSSNIC_EVENT_PORT_MODULE_EVENT;
	module_event->type = (in_wire_event->status != SSNSIC_PORT_PRESENT) ?
			     SSSNIC_PORT_MODULE_CABLE_PLUGGED : SSSNIC_PORT_MODULE_CABLE_UNPLUGGED;

	sss_do_event_callback(nic_io->hwdev, &event_info);
}

static void sss_nic_port_sfp_event_handler(struct sss_nic_io *nic_io,
					   void *in_buf, u16 in_size, void *out_buf, u16 *out_size)
{
	struct sss_nic_mbx_get_xsfp_info *in_xsfp_info = in_buf;
	struct sss_nic_cache_port_sfp *routine_cmd = NULL;

	if (in_size != sizeof(*in_xsfp_info)) {
		nic_err(nic_io->dev_hdl, "Invalid in_size: %u, should be %ld\n",
			in_size, sizeof(*in_xsfp_info));
		return;
	}

	routine_cmd = &nic_io->mag_cfg.rt_cmd;
	mutex_lock(&nic_io->mag_cfg.sfp_mutex);
	routine_cmd->mpu_send_sfp_info = true;
	memcpy(&routine_cmd->std_sfp_info, in_xsfp_info, sizeof(*in_xsfp_info));
	mutex_unlock(&nic_io->mag_cfg.sfp_mutex);
}

static void sss_nic_port_sfp_absent_event_handler(struct sss_nic_io *nic_io,
						  void *in_buf, u16 in_size,
						  void *out_buf, u16 *out_size)
{
	struct sss_nic_mbx_get_xsfp_present *in_xsfp_present = in_buf;
	struct sss_nic_cache_port_sfp *routine_cmd = NULL;

	if (in_size != sizeof(*in_xsfp_present)) {
		nic_err(nic_io->dev_hdl, "Invalid in_size: %u, should be %ld\n",
			in_size, sizeof(*in_xsfp_present));
		return;
	}

	routine_cmd = &nic_io->mag_cfg.rt_cmd;
	mutex_lock(&nic_io->mag_cfg.sfp_mutex);
	routine_cmd->mpu_send_sfp_abs = true;
	memcpy(&routine_cmd->abs, in_xsfp_present, sizeof(*in_xsfp_present));
	mutex_unlock(&nic_io->mag_cfg.sfp_mutex);
}

bool sss_nic_if_sfp_absent(struct sss_nic_dev *nic_dev)
{
	int ret;
	bool sfp_abs_state;
	struct sss_nic_cache_port_sfp *routine_cmd = NULL;
	u8 port_id = sss_get_phy_port_id(nic_dev->hwdev);
	struct sss_nic_mbx_get_xsfp_present xsfp_present = {0};
	u16 out_len = sizeof(xsfp_present);

	routine_cmd = &nic_dev->nic_io->mag_cfg.rt_cmd;
	mutex_lock(&nic_dev->nic_io->mag_cfg.sfp_mutex);
	if (routine_cmd->mpu_send_sfp_abs) {
		if (routine_cmd->abs.head.state) {
			mutex_unlock(&nic_dev->nic_io->mag_cfg.sfp_mutex);
			return true;
		}

		sfp_abs_state = (bool)routine_cmd->abs.abs_status;
		mutex_unlock(&nic_dev->nic_io->mag_cfg.sfp_mutex);
		return sfp_abs_state;
	}
	mutex_unlock(&nic_dev->nic_io->mag_cfg.sfp_mutex);

	xsfp_present.port_id = port_id;
	ret = sss_nic_mag_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MAG_OPCODE_GET_XSFP_PRESENT,
					   &xsfp_present, sizeof(xsfp_present), &xsfp_present,
					   &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &xsfp_present)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to get port%u sfp absent status, ret: %d, status: 0x%x, out_len: 0x%x\n",
			port_id, ret, xsfp_present.head.state, out_len);
		return true;
	}

	return !!xsfp_present.abs_status;
}

int sss_nic_get_sfp_info(struct sss_nic_dev *nic_dev,
			 struct sss_nic_mbx_get_xsfp_info *xsfp_info)
{
	int ret;
	u16 out_len = sizeof(*xsfp_info);
	struct sss_nic_cache_port_sfp *routine_cmd = NULL;

	if (!nic_dev || !xsfp_info)
		return -EINVAL;

	routine_cmd = &nic_dev->nic_io->mag_cfg.rt_cmd;
	mutex_lock(&nic_dev->nic_io->mag_cfg.sfp_mutex);
	if (routine_cmd->mpu_send_sfp_info) {
		if (routine_cmd->std_sfp_info.head.state) {
			mutex_unlock(&nic_dev->nic_io->mag_cfg.sfp_mutex);
			return -EIO;
		}

		memcpy(xsfp_info, &routine_cmd->std_sfp_info, sizeof(*xsfp_info));
		mutex_unlock(&nic_dev->nic_io->mag_cfg.sfp_mutex);
		return 0;
	}
	mutex_unlock(&nic_dev->nic_io->mag_cfg.sfp_mutex);

	xsfp_info->port_id = sss_get_phy_port_id(nic_dev->hwdev);
	ret = sss_nic_mag_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MAG_OPCODE_GET_XSFP_INFO,
					   xsfp_info, sizeof(*xsfp_info), xsfp_info, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, xsfp_info)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to get port%u sfp eeprom information, ret: %d, status: 0x%x, out_len: 0x%x\n",
			sss_get_phy_port_id(nic_dev->hwdev), ret,
			xsfp_info->head.state, out_len);
		return -EIO;
	}

	return 0;
}

int sss_nic_get_sfp_eeprom(struct sss_nic_dev *nic_dev, u8 *data, u32 len)
{
	struct sss_nic_mbx_get_xsfp_info xsfp_info = {0};
	int ret;

	if (!nic_dev || !data)
		return -EINVAL;

	if (sss_nic_if_sfp_absent(nic_dev))
		return -ENXIO;

	ret = sss_nic_get_sfp_info(nic_dev, &xsfp_info);
	if (ret != 0)
		return ret;

	memcpy(data, xsfp_info.sfp_info, len);

	return  0;
}

int sss_nic_get_sfp_type(struct sss_nic_dev *nic_dev, u8 *sfp_type, u8 *sfp_type_ext)
{
	struct sss_nic_cache_port_sfp *routine_cmd = NULL;
	u8 sfp_data[SSSNIC_STD_SFP_INFO_MAX_SIZE];
	int ret;

	if (!nic_dev || !sfp_type || !sfp_type_ext)
		return -EINVAL;

	if (sss_nic_if_sfp_absent(nic_dev))
		return -ENXIO;

	routine_cmd = &nic_dev->nic_io->mag_cfg.rt_cmd;

	mutex_lock(&nic_dev->nic_io->mag_cfg.sfp_mutex);
	if (routine_cmd->mpu_send_sfp_info) {
		if (routine_cmd->std_sfp_info.head.state) {
			mutex_unlock(&nic_dev->nic_io->mag_cfg.sfp_mutex);
			return -EIO;
		}

		*sfp_type_ext = routine_cmd->std_sfp_info.sfp_info[1];
		*sfp_type = routine_cmd->std_sfp_info.sfp_info[0];
		mutex_unlock(&nic_dev->nic_io->mag_cfg.sfp_mutex);
		return 0;
	}
	mutex_unlock(&nic_dev->nic_io->mag_cfg.sfp_mutex);

	ret = sss_nic_get_sfp_eeprom(nic_dev, (u8 *)sfp_data, SSSNIC_STD_SFP_INFO_MAX_SIZE);
	if (ret != 0)
		return ret;

	*sfp_type = sfp_data[0];
	*sfp_type_ext = sfp_data[1];

	return 0;
}

int sss_nic_set_link_follow_state(struct sss_nic_dev *nic_dev,
				  enum sss_nic_link_follow_status state)
{
	int ret;
	struct sss_nic_mbx_set_link_follow link_follow = {0};
	u16 out_len = sizeof(link_follow);

	link_follow.function_id = sss_get_global_func_id(nic_dev->hwdev);
	link_follow.follow = state;

	ret = sss_nic_mag_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MAG_OPCODE_SET_LINK_FOLLOW,
					   &link_follow, sizeof(link_follow),
					   &link_follow, &out_len);
	if ((link_follow.head.state != SSS_MGMT_CMD_UNSUPPORTED && link_follow.head.state != 0) ||
	    ret != 0 || out_len == 0) {
		nic_err(nic_dev->dev_hdl,
			"Fail to set link status follow, ret: %d, state: 0x%x, out size: 0x%x\n",
			ret, link_follow.head.state, out_len);
		return -EFAULT;
	}

	return link_follow.head.state;
}

static const struct sss_nic_vf_msg_handler g_sss_nic_vf_mag_cmd_proc[] = {
	{
		.opcode = SSSNIC_MAG_OPCODE_LINK_STATUS,
		.msg_handler = sss_nic_get_vf_link_status_handler,
	},
};

static const struct sss_nic_vf_msg_handler *sss_nic_get_vf_mag_cmd_proc(u16 opcode)
{
	u16 i;
	u16 cmd_num = ARRAY_LEN(g_sss_nic_vf_mag_cmd_proc);

	for (i = 0; i < cmd_num; i++)
		if (g_sss_nic_vf_mag_cmd_proc[i].opcode == opcode)
			return &g_sss_nic_vf_mag_cmd_proc[i];

	return NULL;
}

/* pf/ppf handler mbx msg from vf */
int sss_nic_pf_mag_mbx_handler(void *hwdev,
			       u16 vf_id, u16 cmd, void *in_buf, u16 in_size,
			       void *out_buf, u16 *out_size)
{
	const struct sss_nic_vf_msg_handler *handler = NULL;
	struct sss_nic_io *nic_io;

	if (!hwdev)
		return -EFAULT;

	nic_io = sss_get_service_adapter(hwdev, SSS_SERVICE_TYPE_NIC);
	if (!nic_io)
		return -EINVAL;

	handler = sss_nic_get_vf_mag_cmd_proc(cmd);
	if (handler)
		return handler->msg_handler(nic_io, vf_id,
					    in_buf, in_size, out_buf, out_size);

	nic_warn(nic_io->dev_hdl, "NO function found for mag cmd: %u received from vf id: %u\n",
		 cmd, vf_id);

	return -EINVAL;
}

static struct nic_event_handler g_sss_nic_mag_cmd_proc[] = {
	{
		.opcode = SSSNIC_MAG_OPCODE_LINK_STATUS,
		.event_handler = sss_nic_link_status_event_handler,
	},

	{
		.opcode = SSSNIC_MAG_OPCODE_WIRE_EVENT,
		.event_handler = sss_nic_cable_plug_event_handler,
	},

	{
		.opcode = SSSNIC_MAG_OPCODE_GET_XSFP_INFO,
		.event_handler = sss_nic_port_sfp_event_handler,
	},

	{
		.opcode = SSSNIC_MAG_OPCODE_GET_XSFP_PRESENT,
		.event_handler = sss_nic_port_sfp_absent_event_handler,
	},
};

static const struct nic_event_handler *sss_nic_get_mag_cmd_proc(u16 opcode)
{
	u16 i;
	u16 cmd_num = ARRAY_LEN(g_sss_nic_mag_cmd_proc);

	for (i = 0; i < cmd_num; i++)
		if (g_sss_nic_mag_cmd_proc[i].opcode == opcode)
			return &g_sss_nic_mag_cmd_proc[i];

	return NULL;
}

static int _sss_nic_mag_event_handler(void *hwdev, u16 cmd,
				      void *in_buf, u16 in_size, void *out_buf, u16 *out_size)
{
	const struct nic_event_handler *handler = NULL;
	struct sss_nic_io *nic_io = NULL;
	struct sss_mgmt_msg_head *out_msg_head = NULL;

	if (!hwdev)
		return -EINVAL;

	nic_io = sss_get_service_adapter(hwdev, SSS_SERVICE_TYPE_NIC);
	if (!nic_io)
		return -EINVAL;

	*out_size = 0;

	handler = sss_nic_get_mag_cmd_proc(cmd);
	if (handler) {
		handler->event_handler(nic_io, in_buf, in_size, out_buf, out_size);
		return 0;
	}

	out_msg_head = out_buf;
	out_msg_head->state = SSS_MGMT_CMD_UNSUPPORTED;
	*out_size = sizeof(*out_msg_head);

	nic_warn(nic_io->dev_hdl, "Invalid mag event cmd: %u\n", cmd);

	return 0;
}

int sss_nic_vf_mag_event_handler(void *hwdev, u16 cmd, void *in_buf, u16 in_size,
				 void *out_buf, u16 *out_size)
{
	return _sss_nic_mag_event_handler(hwdev, cmd, in_buf, in_size, out_buf, out_size);
}

/* pf/ppf handler mgmt cpu report ssslink event */
void sss_nic_pf_mag_event_handler(void *hwdev, u16 cmd, void *in_buf, u16 in_size,
				  void *out_buf, u16 *out_size)
{
	_sss_nic_mag_event_handler(hwdev, cmd, in_buf, in_size, out_buf, out_size);
}

static int _sss_nic_mag_msg_to_mgmt_sync(void *hwdev, u16 cmd,
					 void *in_buf, u16 in_size,
					 void *out_buf, u16 *out_size, u16 channel)
{
	if (sss_get_func_type(hwdev) == SSS_FUNC_TYPE_VF)
		if (sss_nic_get_vf_mag_cmd_proc(cmd))
			return sss_mbx_send_to_pf(hwdev, SSS_MOD_TYPE_SSSLINK, cmd,
						  in_buf, in_size, out_buf, out_size, 0, channel);

	return sss_sync_mbx_send_msg(hwdev, SSS_MOD_TYPE_SSSLINK,
				     cmd, in_buf, in_size, out_buf, out_size, 0, channel);
}

static int sss_nic_mag_msg_to_mgmt_sync(void *hwdev, u16 cmd,
					void *in_buf, u16 in_size, void *out_buf, u16 *out_size)
{
	return _sss_nic_mag_msg_to_mgmt_sync(hwdev, cmd, in_buf, in_size,
					     out_buf, out_size, SSS_CHANNEL_NIC);
}

static int sss_nic_mag_msg_to_mgmt_sync_ch(void *hwdev, u16 cmd,
					   void *in_buf, u16 in_size,
					   void *out_buf, u16 *out_size, u16 channel)
{
	return _sss_nic_mag_msg_to_mgmt_sync(hwdev, cmd, in_buf, in_size,
					     out_buf, out_size, channel);
}

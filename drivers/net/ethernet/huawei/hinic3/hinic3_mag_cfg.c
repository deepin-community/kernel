// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

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

#include "ossl_knl.h"
#include "hinic3_crm.h"
#include "hinic3_hw.h"
#include "mag_mpu_cmd.h"
#include "mag_mpu_cmd_defs.h"
#include "hinic3_nic_io.h"
#include "hinic3_nic_cfg.h"
#include "hinic3_srv_nic.h"
#include "hinic3_nic.h"
#include "hinic3_common.h"
#include "mag_mpu_cmd_defs.h"

#define BIFUR_RESOURCE_PF_SSID	0x5a1
#define CAP_INFO_MAX_LEN	512
#define DEVICE_VENDOR_MAX_LEN		17
#define READ_RSFEC_REGISTER_DELAY_TIME_MS		500

struct parse_tlv_info g_page_info;
struct drv_tag_mag_cmd_get_xsfp_tlv_rsp g_xsfp_tlv_info;

static int mag_msg_to_mgmt_sync(void *hwdev, u16 cmd, void *buf_in, u16 in_size,
				void *buf_out, u16 *out_size);
static int mag_msg_to_mgmt_sync_ch(void *hwdev, u16 cmd, void *buf_in,
				   u16 in_size, void *buf_out, u16 *out_size,
				   u16 channel);

int hinic3_set_port_enable(void *hwdev, bool enable, u16 channel)
{
	struct mag_cmd_set_port_enable en_state;
	u16 out_size = sizeof(en_state);
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (!hwdev)
		return -EINVAL;

	if (hinic3_func_type(hwdev) == TYPE_VF)
		return 0;

	memset(&en_state, 0, sizeof(en_state));

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	en_state.function_id = hinic3_global_func_id(hwdev);
	en_state.state = enable ? MAG_CMD_TX_ENABLE | MAG_CMD_RX_ENABLE :
				MAG_CMD_PORT_DISABLE;

	err = mag_msg_to_mgmt_sync_ch(hwdev, MAG_CMD_SET_PORT_ENABLE, &en_state,
				      sizeof(en_state), &en_state, &out_size,
				      channel);
	if (err || !out_size || en_state.head.status) {
		nic_err(nic_io->dev_hdl, "Failed to set port state, err: %d, status: 0x%x, out size: 0x%x, channel: 0x%x\n",
			err, en_state.head.status, out_size, channel);
		return -EIO;
	}

	return 0;
}
EXPORT_SYMBOL(hinic3_set_port_enable);

int hinic3_get_phy_port_stats(void *hwdev, struct mag_cmd_port_stats *stats)
{
	struct mag_cmd_get_port_stat *port_stats = NULL;
	struct mag_cmd_port_stats_info stats_info;
	u16 out_size = sizeof(*port_stats);
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	port_stats = kzalloc(sizeof(*port_stats), GFP_KERNEL);
	if (!port_stats)
		return -ENOMEM;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io) {
		err = -EINVAL;
		goto out;
	}

	memset(&stats_info, 0, sizeof(stats_info));
	stats_info.port_id = hinic3_physical_port_id(hwdev);

	err = mag_msg_to_mgmt_sync(hwdev, MAG_CMD_GET_PORT_STAT,
				   &stats_info, sizeof(stats_info),
				   port_stats, &out_size);
	if (err || !out_size || port_stats->head.status) {
		nic_err(nic_io->dev_hdl,
			"Failed to get port statistics, err: %d, status: 0x%x, out size: 0x%x\n",
			err, port_stats->head.status, out_size);
		err = -EIO;
		goto out;
	}

	memcpy(stats, &port_stats->counter, sizeof(*stats));

out:
	kfree(port_stats);

	return err;
}
EXPORT_SYMBOL(hinic3_get_phy_port_stats);

int hinic3_get_phy_rsfec_stats(void *hwdev, struct mag_cmd_rsfec_stats *stats)
{
	struct mag_cmd_get_mag_cnt *port_stats = NULL;
	struct mag_cmd_get_mag_cnt stats_info;
	u16 out_size = sizeof(*port_stats);
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (!hwdev || !stats)
		return -EINVAL;

	port_stats = kzalloc(sizeof(*port_stats), GFP_KERNEL);
	if (!port_stats)
		return -ENOMEM;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io) {
		err = -EINVAL;
		goto out;
	}

	memset(&stats_info, 0, sizeof(stats_info));
	stats_info.port_id = hinic3_physical_port_id(hwdev);

	err = mag_msg_to_mgmt_sync(hwdev, MAG_CMD_GET_MAG_CNT,
				   &stats_info, sizeof(stats_info),
				   port_stats, &out_size);
	if (err || !out_size || port_stats->head.status) {
		nic_err(nic_io->dev_hdl,
			"Failed to get rsfec statistics, err: %d, status: 0x%x, out size: 0x%x\n",
			err, port_stats->head.status, out_size);
		err = -EIO;
		goto out;
	}
	/* 读2遍, 清除误码残留 */
	msleep(READ_RSFEC_REGISTER_DELAY_TIME_MS);

	err = mag_msg_to_mgmt_sync(hwdev, MAG_CMD_GET_MAG_CNT, &stats_info,
				   sizeof(stats_info),
	port_stats, &out_size);
	if (err || !out_size || port_stats->head.status) {
		nic_err(nic_io->dev_hdl,
			"Failed to get rsfec statistics, err: %d, status: 0x%x, out size: 0x%x\n",
			err, port_stats->head.status, out_size);
		err = -EIO;
		goto out;
	}

	memcpy(stats, &port_stats->mag_csr[MAG_RX_RSFEC_ERR_CW_CNT],
	       sizeof(u32));

out:
	kfree(port_stats);

	return err;
}
EXPORT_SYMBOL(hinic3_get_phy_rsfec_stats);

int hinic3_set_port_funcs_state(void *hwdev, bool enable)
{
	return 0;
}

int hinic3_reset_port_link_cfg(void *hwdev)
{
	return 0;
}

int hinic3_force_port_relink(void *hwdev)
{
	return 0;
}

int hinic3_set_autoneg(void *hwdev, bool enable)
{
	struct hinic3_link_ksettings settings = {0};
	struct hinic3_nic_io *nic_io = NULL;
	u32 set_settings = 0;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	set_settings |= HILINK_LINK_SET_AUTONEG;
	settings.valid_bitmap = set_settings;
	settings.autoneg = enable ? PORT_CFG_AN_ON : PORT_CFG_AN_OFF;

	return hinic3_set_link_settings(hwdev, &settings);
}

static int hinic3_cfg_loopback_mode(struct hinic3_nic_io *nic_io, u8 opcode,
				    u8 *mode, u8 *enable)
{
	struct mag_cmd_cfg_loopback_mode lp;
	u16 out_size = sizeof(lp);
	int err;

	memset(&lp, 0, sizeof(lp));
	lp.port_id = hinic3_physical_port_id(nic_io->hwdev);
	lp.opcode = opcode;
	if (opcode == MGMT_MSG_CMD_OP_SET) {
		lp.lp_mode = *mode;
		lp.lp_en = *enable;
	}

	err = mag_msg_to_mgmt_sync(nic_io->hwdev, MAG_CMD_CFG_LOOPBACK_MODE,
				   &lp, sizeof(lp), &lp, &out_size);
	if (err || !out_size || lp.head.status) {
		nic_err(nic_io->dev_hdl,
			"Failed to %s loopback mode, err: %d, status: 0x%x, out size: 0x%x\n",
			opcode == MGMT_MSG_CMD_OP_SET ? "set" : "get",
			err, lp.head.status, out_size);
		return -EIO;
	}

	if (opcode == MGMT_MSG_CMD_OP_GET) {
		*mode = lp.lp_mode;
		*enable = lp.lp_en;
	}

	return 0;
}

int hinic3_get_loopback_mode(void *hwdev, u8 *mode, u8 *enable)
{
	struct hinic3_nic_io *nic_io = NULL;

	if (!hwdev || !mode || !enable)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	return hinic3_cfg_loopback_mode(nic_io, MGMT_MSG_CMD_OP_GET, mode,
					enable);
}

#define LOOP_MODE_MIN 1
#define LOOP_MODE_MAX 6
int hinic3_set_loopback_mode(void *hwdev, u8 mode, u8 enable)
{
	struct hinic3_nic_io *nic_io = NULL;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	if (mode < LOOP_MODE_MIN || mode > LOOP_MODE_MAX) {
		nic_err(nic_io->dev_hdl, "Invalid loopback mode %u to set\n",
			mode);
		return -EINVAL;
	}

	return hinic3_cfg_loopback_mode(nic_io, MGMT_MSG_CMD_OP_SET, &mode,
					&enable);
}

int hinic3_set_led_status(void *hwdev, enum mag_led_type type,
			  enum mag_led_mode mode)
{
	struct hinic3_nic_io *nic_io = NULL;
	struct mag_cmd_set_led_cfg led_info;
	u16 out_size = sizeof(led_info);
	int err;

	if (!hwdev)
		return -EFAULT;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	memset(&led_info, 0, sizeof(led_info));

	led_info.function_id = hinic3_global_func_id(hwdev);
	led_info.type = type;
	led_info.mode = mode;

	err = mag_msg_to_mgmt_sync(hwdev, MAG_CMD_SET_LED_CFG, &led_info,
				   sizeof(led_info), &led_info, &out_size);
	if (err || led_info.head.status || !out_size) {
		nic_err(nic_io->dev_hdl, "Failed to set led status, err: %d, status: 0x%x, out size: 0x%x\n",
			err, led_info.head.status, out_size);
		return -EIO;
	}

	return 0;
}

int hinic3_get_port_info(void *hwdev, struct nic_port_info *port_info,
			 u16 channel)
{
	struct mag_cmd_get_port_info port_msg;
	u16 out_size = sizeof(port_msg);
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (!hwdev || !port_info)
		return -EINVAL;

	memset(&port_msg, 0, sizeof(port_msg));

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	port_msg.port_id = hinic3_physical_port_id(hwdev);

	err = mag_msg_to_mgmt_sync_ch(hwdev, MAG_CMD_GET_PORT_INFO, &port_msg,
				      sizeof(port_msg), &port_msg, &out_size,
				      channel);
	if (err || !out_size || port_msg.head.status) {
		nic_err(nic_io->dev_hdl,
			"Failed to get port info, err: %d, status: 0x%x, out size: 0x%x, channel: 0x%x\n",
			err, port_msg.head.status, out_size, channel);
		return -EIO;
	}

	port_info->autoneg_cap = port_msg.an_support;
	port_info->autoneg_state = port_msg.an_en;
	port_info->duplex = port_msg.duplex;
	port_info->port_type = port_msg.wire_type;
	port_info->speed = port_msg.speed;
	port_info->fec = port_msg.fec;
	port_info->lanes = port_msg.lanes;
	port_info->supported_mode = port_msg.supported_mode;
	port_info->advertised_mode = port_msg.advertised_mode;
	port_info->supported_fec_mode = port_msg.supported_fec_mode;
	/* switch Gbps to Mbps */
	port_info->bond_speed = (u32)port_msg.bond_speed * RATE_MBPS_TO_GBPS;
	return 0;
}

int hinic3_get_speed(void *hwdev, enum mag_cmd_port_speed *speed, u16 channel)
{
	struct nic_port_info port_info = {0};
	int err;

	if (!hwdev || !speed)
		return -EINVAL;

	err = hinic3_get_port_info(hwdev, &port_info, channel);
	if (err)
		return err;

	*speed = port_info.speed;

	return 0;
}
EXPORT_SYMBOL(hinic3_get_speed);

int hinic3_set_link_settings(void *hwdev,
			     struct hinic3_link_ksettings *settings)
{
	struct mag_cmd_set_port_cfg info;
	u16 out_size = sizeof(info);
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (!hwdev || !settings)
		return -EINVAL;

	memset(&info, 0, sizeof(info));

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	info.port_id = hinic3_physical_port_id(hwdev);
	info.config_bitmap = settings->valid_bitmap;
	info.autoneg = settings->autoneg;
	info.speed = settings->speed;
	info.fec = settings->fec;

	err = mag_msg_to_mgmt_sync(hwdev, MAG_CMD_SET_PORT_CFG, &info,
				   sizeof(info), &info, &out_size);
	if (err || !out_size || info.head.status) {
		nic_err(nic_io->dev_hdl, "Failed to set link settings, err: %d, status: 0x%x, out size: 0x%x\n",
			err, info.head.status, out_size);
		return -EIO;
	}

	return info.head.status;
}

int hinic3_get_link_state(void *hwdev, u8 *link_state)
{
	struct mag_cmd_get_link_status get_link;
	u16 out_size = sizeof(get_link);
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (!hwdev || !link_state)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	memset(&get_link, 0, sizeof(get_link));
	get_link.port_id = hinic3_physical_port_id(hwdev);

	err = mag_msg_to_mgmt_sync(hwdev, MAG_CMD_GET_LINK_STATUS, &get_link,
				   sizeof(get_link), &get_link, &out_size);
	if (err || !out_size || get_link.head.status) {
		nic_err(nic_io->dev_hdl, "Failed to get link state, err: %d, status: 0x%x, out size: 0x%x\n",
			err, get_link.head.status, out_size);
		return -EIO;
	}

	*link_state = get_link.status;

	return 0;
}

void hinic3_notify_vf_link_status(struct hinic3_nic_io *nic_io,
				  u16 vf_id, u8 link_status)
{
	struct mag_cmd_get_link_status link;
	struct vf_data_storage *vf_infos = nic_io->vf_infos;
	u16 out_size = sizeof(link);
	int err;

	memset(&link, 0, sizeof(link));
	if (vf_infos[HW_VF_ID_TO_OS(vf_id)].registered) {
		link.status = link_status;
		link.port_id = hinic3_physical_port_id(nic_io->hwdev);
		err = hinic3_mbox_to_vf_no_ack(nic_io->hwdev, vf_id,
					       HINIC3_MOD_HILINK,
					       MAG_CMD_GET_LINK_STATUS, &link,
					       sizeof(link), &link, &out_size,
					       HINIC3_CHANNEL_NIC);
		if (err == MBOX_ERRCODE_UNKNOWN_DES_FUNC) {
			nic_warn(nic_io->dev_hdl, "VF%d not initialized, disconnect it\n",
				 HW_VF_ID_TO_OS(vf_id));
			hinic3_unregister_vf(nic_io, vf_id);
			return;
		}
		if (err || !out_size || link.head.status)
			nic_err(nic_io->dev_hdl,
				"Send link change event to VF %d failed, err: %d, status: 0x%x, out_size: 0x%x\n",
				HW_VF_ID_TO_OS(vf_id), err, link.head.status, out_size);
	}
}

void hinic3_notify_all_vfs_link_changed(void *hwdev, u8 link_status)
{
	struct hinic3_nic_io *nic_io = NULL;
	u16 i;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return;
	nic_io->link_status = link_status;
	for (i = 1; i <= nic_io->max_vfs; i++) {
		if (!nic_io->vf_infos[HW_VF_ID_TO_OS(i)].link_forced)
			hinic3_notify_vf_link_status(nic_io, i, link_status);
	}
}

static char *g_hw_to_char_fec[HILINK_FEC_MAX_TYPE] = {
				"not set", "rsfec", "basefec",
				"nofec", "llrsfec"};
static char *g_hw_to_speed_info[PORT_SPEED_UNKNOWN] = {
				"not set", "10MB", "100MB", "1GB", "10GB",
				"25GB", "40GB", "50GB", "100GB", "200GB"};
static char *g_hw_to_an_state_info[PORT_CFG_AN_OFF + 1] = {
				"not set", "on", "off"};

struct port_type_table {
	u32 port_type;
	char *port_type_name;
};

static const struct port_type_table port_optical_type_table_s[] = {
	{.port_type = LINK_PORT_UNKNOWN,	.port_type_name = "UNKNOWN"},
	{.port_type = LINK_PORT_OPTICAL_MM,	.port_type_name = "optical_sr"},
	{.port_type = LINK_PORT_OPTICAL_SM,	.port_type_name = "optical_lr"},
	{.port_type = LINK_PORT_PAS_COPPER,	.port_type_name = "copper"},
	{.port_type = LINK_PORT_ACC,		.port_type_name = "ACC"},
	{.port_type = LINK_PORT_BASET,		.port_type_name = "baset"},
	{.port_type = LINK_PORT_AOC,		.port_type_name = "AOC"},
	{.port_type = LINK_PORT_ELECTRIC,	.port_type_name = "electric"},
	{.port_type = LINK_PORT_BACKBOARD_INTERFACE,	.port_type_name = "interface"},
};

static char *get_port_type_name(u32 type)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(port_optical_type_table_s); i++) {
		if (type == port_optical_type_table_s[i].port_type)
			return port_optical_type_table_s[i].port_type_name;
	}
	return "UNKNOWN TYPE";
}

static void get_port_type(struct hinic3_nic_io *nic_io,
			  struct mag_cmd_event_port_info *info,
			  char **port_type)
{
	if (info->port_type <= LINK_PORT_BACKBOARD_INTERFACE)
		*port_type = get_port_type_name(info->port_type);
	else
		sdk_info(nic_io->dev_hdl, "Unknown port type: %u\n", info->port_type);
}

static int get_port_temperature_power(struct mag_cmd_event_port_info *info,
				      char *str)
{
	char cap_info[CAP_INFO_MAX_LEN];

	memset(cap_info, 0, sizeof(cap_info));
	snprintf(cap_info, CAP_INFO_MAX_LEN, "%s, %s, Temperature: %u", str,
		 info->sfp_type ? "QSFP" : "SFP", info->cable_temp);

	if (info->sfp_type)
		snprintf(str,  CAP_INFO_MAX_LEN, "%s, rx power: %uuw %uuW %uuW %uuW",
			 cap_info, info->power[0x0], info->power[0x1],
			 info->power[0x2], info->power[0x3]);
	else
		snprintf(str, CAP_INFO_MAX_LEN, "%s, rx power: %uuW, tx power: %uuW",
			 cap_info, info->power[0x0], info->power[0x1]);

	return 0;
}

static void print_cable_info(struct hinic3_nic_io *nic_io, struct mag_cmd_event_port_info *info)
{
	char tmp_str[CAP_INFO_MAX_LEN] = {0};
	char tmp_vendor[DEVICE_VENDOR_MAX_LEN] = {0};
	char *port_type = "Unknown port type";
	int i;
	int err = 0;

	if (info->gpio_insert) {
		sdk_info(nic_io->dev_hdl, "Cable unpresent\n");
		return;
	}

	get_port_type(nic_io, info, &port_type);

	for (i = sizeof(info->vendor_name) - 1; i >= 0; i--) {
		if (info->vendor_name[i] == ' ')
			info->vendor_name[i] = '\0';
		else
			break;
	}

	memcpy(tmp_vendor, info->vendor_name, sizeof(info->vendor_name));
	snprintf(tmp_str, CAP_INFO_MAX_LEN, "Vendor: %s, %s, length: %um, max_speed: %uGbps",
		 tmp_vendor, port_type, info->cable_length, info->max_speed);

	if (info->port_type == LINK_PORT_OPTICAL_MM ||
	    info->port_type == LINK_PORT_AOC) {
		err = get_port_temperature_power(info, tmp_str);
		if (err)
			return;
	}

	sdk_info(nic_io->dev_hdl, "Cable information: %s\n", tmp_str);
}

static void print_link_info(struct hinic3_nic_io *nic_io,
			    struct mag_cmd_event_port_info *info,
			    enum hinic3_nic_event_type type)
{
	char *fec = "None";
	char *speed = "None";
	char *an_state = "None";

	if (info->fec < HILINK_FEC_MAX_TYPE)
		fec = g_hw_to_char_fec[info->fec];
	else
		sdk_info(nic_io->dev_hdl, "Unknown fec type: %u\n", info->fec);

	if (info->an_state > PORT_CFG_AN_OFF) {
		sdk_info(nic_io->dev_hdl, "an_state %d is invalid",
			 info->an_state);
		return;
	}

	an_state = g_hw_to_an_state_info[info->an_state];

	if (info->speed >= PORT_SPEED_UNKNOWN) {
		sdk_info(nic_io->dev_hdl, "speed %u is invalid", info->speed);
		return;
	}

	speed = g_hw_to_speed_info[info->speed];
	sdk_info(nic_io->dev_hdl, "Link information: speed %s, %s, autoneg %s",
		 speed, fec, an_state);
}

void print_port_info(struct hinic3_nic_io *nic_io,
		     struct mag_cmd_event_port_info *port_info,
		     enum hinic3_nic_event_type type)
{
	print_cable_info(nic_io, port_info);

	print_link_info(nic_io, port_info, type);

	if (type == EVENT_NIC_LINK_UP)
		return;

	sdk_info(nic_io->dev_hdl, "PMA ctrl: %s, tx %s, rx %s, PMA fifo reg: 0x%x, PMA signal ok reg: 0x%x, RF/LF status reg: 0x%x\n",
		 port_info->pma_ctrl == 1 ? "off" : "on",
		 port_info->tx_enable ? "enable" : "disable",
		 port_info->rx_enable ? "enable" : "disable", port_info->pma_fifo_reg,
		 port_info->pma_signal_ok_reg, port_info->rf_lf);
	sdk_info(nic_io->dev_hdl, "alos: %u, rx_los: %u, PCS 64 66b reg: 0x%x, PCS link: 0x%x, MAC link: 0x%x PCS_err_cnt: 0x%x\n",
		 port_info->alos, port_info->rx_los, port_info->pcs_64_66b_reg,
		 port_info->pcs_link, port_info->pcs_mac_link,
		 port_info->pcs_err_cnt);
	sdk_info(nic_io->dev_hdl, "his_link_machine_state = 0x%08x, cur_link_machine_state = 0x%08x\n",
		 port_info->his_link_machine_state,
		 port_info->cur_link_machine_state);
}

static int hinic3_get_vf_link_status_msg_handler(struct hinic3_nic_io *nic_io,
						 u16 vf_id, void *buf_in,
						 u16 in_size, void *buf_out,
						 u16 *out_size)
{
	struct vf_data_storage *vf_infos = nic_io->vf_infos;
	struct mag_cmd_get_link_status *get_link = buf_out;
	bool link_forced, link_up;

	link_forced = vf_infos[HW_VF_ID_TO_OS(vf_id)].link_forced;
	link_up = vf_infos[HW_VF_ID_TO_OS(vf_id)].link_up;

	if (link_forced)
		get_link->status = link_up ?
					HINIC3_LINK_UP : HINIC3_LINK_DOWN;
	else
		get_link->status = nic_io->link_status;

	get_link->head.status = 0;
	*out_size = sizeof(*get_link);

	return 0;
}

int hinic3_refresh_nic_cfg(void *hwdev, struct nic_port_info *port_info)
{
	/* TO DO */
	return 0;
}

static void get_port_info(void *hwdev,
			  const struct mag_cmd_get_link_status *link_status,
			  struct hinic3_event_link_info *link_info)
{
	struct nic_port_info port_info = {0};
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io) {
		pr_err("nic_io is NULL\n");
		return;
	}
	if (hinic3_func_type(hwdev) != TYPE_VF && link_status->status) {
		err = hinic3_get_port_info(hwdev, &port_info, HINIC3_CHANNEL_NIC);
		if (err) {
			nic_warn(nic_io->dev_hdl, "Failed to get port info\n");
		} else {
			link_info->valid = 1;
			link_info->port_type = port_info.port_type;
			link_info->autoneg_cap = port_info.autoneg_cap;
			link_info->autoneg_state = port_info.autoneg_state;
			link_info->duplex = port_info.duplex;
			link_info->speed = port_info.speed;
			hinic3_refresh_nic_cfg(hwdev, &port_info);
		}
	}
}

static void link_status_event_handler(void *hwdev, void *buf_in,
				      u16 in_size, void *buf_out, u16 *out_size)
{
	struct mag_cmd_get_link_status *link_status = NULL;
	struct mag_cmd_get_link_status *ret_link_status = NULL;
	struct hinic3_event_info event_info = {0};
	struct hinic3_event_link_info *link_info = (void *)event_info.event_data;
	struct hinic3_nic_io *nic_io = NULL;
#ifndef __UEFI__
	struct pci_dev *pdev = NULL;
#endif
	/* Ignore link change event */
	if (hinic3_is_bm_slave_host(hwdev))
		return;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io) {
		pr_err("nic_io is NULL\n");
		return;
	}

	link_status = buf_in;
	sdk_info(nic_io->dev_hdl, "Link status report received, func_id: %u, status: %u\n",
		 hinic3_global_func_id(hwdev), link_status->status);

	hinic3_link_event_stats(hwdev, link_status->status);

	/* link event reported only after set vport enable */
	get_port_info(hwdev, link_status, link_info);

	event_info.service = EVENT_SRV_NIC;
	event_info.type = link_status->status ?
				EVENT_NIC_LINK_UP : EVENT_NIC_LINK_DOWN;

	hinic3_event_callback(hwdev, &event_info);

#ifndef __UEFI__
	if (nic_io->pcidev_hdl) {
		pdev = nic_io->pcidev_hdl;
		if (pdev->subsystem_device == BIFUR_RESOURCE_PF_SSID) {
			return;
		}
	}
#endif

	if (hinic3_func_type(hwdev) != TYPE_VF) {
		hinic3_notify_all_vfs_link_changed(hwdev, link_status->status);
		ret_link_status = buf_out;
		ret_link_status->head.status = 0;
		*out_size = sizeof(*ret_link_status);
	}
}

static void port_info_event_printf(void *hwdev, void *buf_in, u16 in_size,
				   void *buf_out, u16 *out_size)
{
	struct mag_cmd_event_port_info *port_info = buf_in;
	struct hinic3_nic_io *nic_io = NULL;
	struct hinic3_event_info event_info;
	enum hinic3_nic_event_type type;

	if (!hwdev) {
		pr_err("hwdev is NULL\n");
		return;
	}

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io) {
		pr_err("nic_io is NULL\n");
		return;
	}

	if (in_size != sizeof(*port_info)) {
		sdk_info(nic_io->dev_hdl, "Invalid port info message size %d, should be %ld\n",
			 in_size, sizeof(*port_info));
		return;
	}

	((struct mag_cmd_event_port_info *)buf_out)->head.status = 0;

	type = port_info->event_type;
	if (type < EVENT_NIC_LINK_DOWN || type > EVENT_NIC_LINK_UP) {
		sdk_info(nic_io->dev_hdl, "Invalid hilink info report, type: %d\n",
			 type);
		return;
	}

	print_port_info(nic_io, port_info, type);

	memset(&event_info, 0, sizeof(event_info));
	event_info.service = EVENT_SRV_NIC;
	event_info.type = type;

	*out_size = sizeof(*port_info);

	hinic3_event_callback(hwdev, &event_info);
}

void hinic3_notify_vf_bond_status(struct hinic3_nic_io *nic_io,
				  u16 vf_id, u8 bond_status)
{
	struct mag_cmd_get_bond_status bond;
	struct vf_data_storage *vf_infos = nic_io->vf_infos;
	u16 out_size = sizeof(bond);
	int err;

	memset(&bond, 0, sizeof(bond));
	if (vf_infos[HW_VF_ID_TO_OS(vf_id)].registered) {
		bond.status = bond_status;
		err = hinic3_mbox_to_vf_no_ack(nic_io->hwdev, vf_id,
					       HINIC3_MOD_HILINK,
					       MAG_CMD_GET_BOND_STATUS, &bond,
					       sizeof(bond), &bond, &out_size,
					       HINIC3_CHANNEL_NIC);
		if (err == MBOX_ERRCODE_UNKNOWN_DES_FUNC) {
			nic_warn(nic_io->dev_hdl, "VF %u not initialized, disconnect it\n",
				 HW_VF_ID_TO_OS(vf_id));
			hinic3_unregister_vf(nic_io, vf_id);
			return;
		}
		if (err || !out_size || bond.head.status)
			nic_err(nic_io->dev_hdl,
				"Send bond change event to VF %hu failed, err: %d, status: 0x%x, out_size: 0x%x\n",
				HW_VF_ID_TO_OS(vf_id), err, bond.head.status,
				out_size);
	}
}

void hinic3_notify_all_vfs_bond_changed(void *hwdev, u8 bond_status)
{
	struct hinic3_nic_io *nic_io = NULL;
	u16 i;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	nic_io->link_status = bond_status;
	for (i = 1; i <= nic_io->max_vfs; i++)
		hinic3_notify_vf_bond_status(nic_io, i, bond_status);
}

static void bond_status_event_handler(void *hwdev, void *buf_in,
				      u16 in_size, void *buf_out, u16 *out_size)
{
	struct mag_cmd_get_bond_status *bond_status = NULL;
	struct hinic3_event_info event_info = {};
	struct hinic3_nic_io *nic_io = NULL;
	struct mag_cmd_get_bond_status *ret_bond_status = NULL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);

	bond_status = (struct mag_cmd_get_bond_status *)buf_in;
	sdk_info(nic_io->dev_hdl, "bond status report received, func_id: %u, status: %u\n",
		 hinic3_global_func_id(hwdev), bond_status->status);

	event_info.service = EVENT_SRV_NIC;
	event_info.type = bond_status->status ?
				EVENT_NIC_BOND_UP : EVENT_NIC_BOND_DOWN;

	hinic3_event_callback(hwdev, &event_info);

	if (hinic3_func_type(hwdev) != TYPE_VF) {
		hinic3_notify_all_vfs_bond_changed(hwdev, bond_status->status);
		ret_bond_status = buf_out;
		ret_bond_status->head.status = 0;
		*out_size = sizeof(*ret_bond_status);
	}
}

static void cable_plug_event(void *hwdev, void *buf_in, u16 in_size,
			     void *buf_out, u16 *out_size)
{
	struct mag_cmd_wire_event *plug_event = buf_in;
	struct hinic3_port_routine_cmd *rt_cmd = NULL;
	struct hinic3_port_routine_cmd_extern *rt_cmd_ext = NULL;
	struct hinic3_nic_io *nic_io = NULL;
	struct hinic3_event_info event_info;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io) {
		pr_err("nic_io is NULL\n");
		return;
	}

	rt_cmd = &nic_io->nic_cfg.rt_cmd;
	rt_cmd_ext = &nic_io->nic_cfg.rt_cmd_ext;

	mutex_lock(&nic_io->nic_cfg.sfp_mutex);
	rt_cmd->mpu_send_sfp_abs = false;
	rt_cmd->mpu_send_sfp_info = false;
	rt_cmd_ext->mpu_send_xsfp_tlv_info = false;
	mutex_unlock(&nic_io->nic_cfg.sfp_mutex);

	memset(&event_info, 0, sizeof(event_info));
	event_info.service = EVENT_SRV_NIC;
	event_info.type = EVENT_NIC_PORT_MODULE_EVENT;
	((struct hinic3_port_module_event *)(void *)event_info.event_data)->type =
				plug_event->status ? HINIC3_PORT_MODULE_CABLE_PLUGGED :
				HINIC3_PORT_MODULE_CABLE_UNPLUGGED;

	*out_size = sizeof(*plug_event);
	plug_event = buf_out;
	plug_event->head.status = 0;

	hinic3_event_callback(hwdev, &event_info);
}

static void port_sfp_info_event(void *hwdev, void *buf_in, u16 in_size,
				void *buf_out, u16 *out_size)
{
	struct mag_cmd_get_xsfp_info *sfp_info = buf_in;
	struct hinic3_port_routine_cmd *rt_cmd = NULL;
	struct hinic3_port_routine_cmd_extern *rt_cmd_ext = NULL;
	struct hinic3_nic_io *nic_io = NULL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return;
	if (in_size != sizeof(*sfp_info)) {
		sdk_err(nic_io->dev_hdl, "Invalid sfp info cmd, length: %u, should be %ld\n",
			in_size, sizeof(*sfp_info));
		return;
	}

	rt_cmd = &nic_io->nic_cfg.rt_cmd;
	rt_cmd_ext = &nic_io->nic_cfg.rt_cmd_ext;
	mutex_lock(&nic_io->nic_cfg.sfp_mutex);
	memcpy(&rt_cmd->std_sfp_info, sfp_info,
	       sizeof(struct mag_cmd_get_xsfp_info));
	rt_cmd->mpu_send_sfp_info = true;
	rt_cmd_ext->mpu_send_xsfp_tlv_info = false;
	mutex_unlock(&nic_io->nic_cfg.sfp_mutex);
}

static void port_xsfp_tlv_info_event(void *hwdev, void *buf_in, u16 in_size,
				     void *buf_out, u16 *out_size)
{
	struct drv_tag_mag_cmd_get_xsfp_tlv_rsp  *xsfp_tlv_info = buf_in;
	struct hinic3_port_routine_cmd *rt_cmd = NULL;
	struct hinic3_port_routine_cmd_extern *rt_cmd_ext = NULL;
	struct hinic3_nic_io *nic_io = NULL;
	size_t cpy_len = in_size  - sizeof(struct mgmt_msg_head) -
			 XSFP_TLV_PRE_INFO_LEN;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return;

	if (cpy_len > XSFP_CMIS_INFO_MAX_SIZE) {
		sdk_err(nic_io->dev_hdl, "invalid cpy_len(%lu)\n", cpy_len);
		return;
	}
	rt_cmd = &nic_io->nic_cfg.rt_cmd;
	rt_cmd_ext = &nic_io->nic_cfg.rt_cmd_ext;
	mutex_lock(&nic_io->nic_cfg.sfp_mutex);
	rt_cmd_ext->std_xsfp_tlv_info.port_id = xsfp_tlv_info->port_id;
	memcpy(&rt_cmd_ext->std_xsfp_tlv_info.tlv_buf[0],
	       &xsfp_tlv_info->tlv_buf[0], cpy_len);
	rt_cmd->mpu_send_sfp_info = false;
	rt_cmd_ext->mpu_send_xsfp_tlv_info = true;
	mutex_unlock(&nic_io->nic_cfg.sfp_mutex);
}

static void port_sfp_abs_event(void *hwdev, void *buf_in, u16 in_size,
			       void *buf_out, u16 *out_size)
{
	struct mag_cmd_get_xsfp_present *sfp_abs = buf_in;
	struct hinic3_port_routine_cmd *rt_cmd = NULL;
	struct hinic3_nic_io *nic_io = NULL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return;
	if (in_size != sizeof(*sfp_abs)) {
		sdk_err(nic_io->dev_hdl, "Invalid sfp absent cmd, length: %u, should be %ld\n",
			in_size, sizeof(*sfp_abs));
		return;
	}

	rt_cmd = &nic_io->nic_cfg.rt_cmd;
	mutex_lock(&nic_io->nic_cfg.sfp_mutex);
	memcpy(&rt_cmd->abs, sfp_abs, sizeof(struct mag_cmd_get_xsfp_present));
	rt_cmd->mpu_send_sfp_abs = true;
	mutex_unlock(&nic_io->nic_cfg.sfp_mutex);
}

bool hinic3_if_sfp_absent(void *hwdev)
{
	struct hinic3_nic_io *nic_io = NULL;
	struct hinic3_port_routine_cmd *rt_cmd = NULL;
	struct mag_cmd_get_xsfp_present sfp_abs;
	u8 port_id = hinic3_physical_port_id(hwdev);
	u16 out_size = sizeof(sfp_abs);
	int err;
	bool sfp_abs_status = 0;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return true;
	memset(&sfp_abs, 0, sizeof(sfp_abs));

	rt_cmd = &nic_io->nic_cfg.rt_cmd;
	mutex_lock(&nic_io->nic_cfg.sfp_mutex);
	if (rt_cmd->mpu_send_sfp_abs) {
		if (rt_cmd->abs.head.status) {
			mutex_unlock(&nic_io->nic_cfg.sfp_mutex);
			return true;
		}

		sfp_abs_status = (bool)rt_cmd->abs.abs_status;
		mutex_unlock(&nic_io->nic_cfg.sfp_mutex);
		return sfp_abs_status;
	}
	mutex_unlock(&nic_io->nic_cfg.sfp_mutex);

	sfp_abs.port_id = port_id;
	err = mag_msg_to_mgmt_sync(hwdev, MAG_CMD_GET_XSFP_PRESENT,
				   &sfp_abs, sizeof(sfp_abs), &sfp_abs,
				   &out_size);
	if (sfp_abs.head.status || err || !out_size) {
		nic_err(nic_io->dev_hdl,
			"Failed to get port%u sfp absent status, err: %d, status: 0x%x, out size: 0x%x\n",
			port_id, err, sfp_abs.head.status, out_size);
		return true;
	}

	return (sfp_abs.abs_status == 0 ? false : true);
}

int hinic3_get_sfp_tlv_info(void *hwdev, struct drv_tag_mag_cmd_get_xsfp_tlv_rsp
			    *sfp_tlv_info,
			    const struct tag_mag_cmd_get_xsfp_tlv_req
			    *sfp_tlv_info_req)
{
	struct hinic3_nic_io *nic_io = NULL;
	struct hinic3_port_routine_cmd_extern *rt_cmd_ext = NULL;
	u16 out_size = sizeof(*sfp_tlv_info);
	int err;

	if (!hwdev || !sfp_tlv_info)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	rt_cmd_ext = &nic_io->nic_cfg.rt_cmd_ext;
	mutex_lock(&nic_io->nic_cfg.sfp_mutex);
	if (rt_cmd_ext->mpu_send_xsfp_tlv_info) {
		if (rt_cmd_ext->std_xsfp_tlv_info.head.status != 0) {
			mutex_unlock(&nic_io->nic_cfg.sfp_mutex);
			return -EIO;
		}

		memcpy(sfp_tlv_info, &rt_cmd_ext->std_xsfp_tlv_info,
		       sizeof(*sfp_tlv_info));
		mutex_unlock(&nic_io->nic_cfg.sfp_mutex);
		return 0;
	}

	mutex_unlock(&nic_io->nic_cfg.sfp_mutex);

	err = mag_msg_to_mgmt_sync(hwdev, MAG_CMD_GET_XSFP_TLV_INFO,
				   (void *)sfp_tlv_info_req,
				   sizeof(*sfp_tlv_info_req),
				   sfp_tlv_info, &out_size);
	if (sfp_tlv_info->head.status != 0 || err != 0 || out_size == 0) {
		nic_err(nic_io->dev_hdl,
			"Failed to get port%u tlv sfp eeprom information, err: %d, status: 0x%x, out size: 0x%x\n",
			hinic3_physical_port_id(hwdev), err,
			sfp_tlv_info->head.status, out_size);
		return -EIO;
	}

	return 0;
}

static int hinic3_trans_cmis_get_page_pos(u32 page_id, u32 content_len, u32 *pos)
{
	if (page_id <= QSFP_CMIS_PAGE_03H) {
		*pos = (page_id * content_len);
		return 0;
	}

	if (page_id == QSFP_CMIS_PAGE_11H) {
		*pos = (QSFP_CMIS_PAGE_04H * content_len);
		return 0;
	}

	if (page_id == QSFP_CMIS_PAGE_12H) {
		*pos = (QSFP_CMIS_PAGE_05H * content_len);
		return 0;
	}

	return -EINVAL;
}

static int hinic3_get_page_key_info(struct mgmt_tlv_info *tlv_info,
				    struct parse_tlv_info *page_info, u8 idx,
				    u32 *total_len)
{
	u8 *src_addr = NULL;
	u8 *dst_addr = NULL;
	u8 *tmp_addr = NULL;
	u32 page_id = 0;
	u32 content_len = 0;
	u32 src_pos = 0;
	int ret;

	page_id = MGMT_TLV_GET_U32(tlv_info->value);
	content_len = tlv_info->length - MGMT_TLV_U32_SIZE;
	if (page_id == QSFP_CMIS_PAGE_00H) {
		tmp_addr = (u8 *)(tlv_info + 1);
		page_info->id = *(tmp_addr + MGMT_TLV_U32_SIZE);
	}

	ret = hinic3_trans_cmis_get_page_pos(page_id, content_len, &src_pos);
	if (ret != 0)
		return ret;

	src_addr = page_info->tlv_page_info + src_pos;
	tmp_addr = (u8 *)(tlv_info + 1);
	dst_addr = tmp_addr + MGMT_TLV_U32_SIZE;
	memcpy(src_addr, dst_addr, content_len);

	if (idx < XSFP_CMIS_PARSE_PAGE_NUM)
		page_info->tlv_page_num[idx] = page_id;

	*total_len += content_len;

	return 0;
}

static int hinic3_trans_cmis_tlv_info_to_buf(u8 *sfp_tlv_info,
					     struct parse_tlv_info *page_info)
{
	struct mgmt_tlv_info *tlv_info = NULL;
	u8 *tlv_buf = sfp_tlv_info;
	u8 idx = 0;
	u32 total_len = 0;
	int ret = 0;
	bool need_continue = true;

	if (!sfp_tlv_info || !page_info)
		return -EIO;

	while (need_continue) {
		tlv_info = (struct mgmt_tlv_info *)tlv_buf;
		switch (tlv_info->type) {
		case MAG_XSFP_TYPE_PAGE:
			ret = hinic3_get_page_key_info(tlv_info, page_info,
						       idx, &total_len);
			if (ret != 0) {
				pr_err("lib_get_page_key_info fail,ret:0x%x.\n",
				       ret);
				break;
			}
			idx++;
			break;

		case MAG_XSFP_TYPE_WIRE_TYPE:
			page_info->wire_type = MGMT_TLV_GET_U32(&tlv_info->value);
			break;

		case MAG_XSFP_TYPE_END:
			need_continue = false;
			break;

		default:
			break;
		}

		tlv_buf += (sizeof(struct mgmt_tlv_info) + tlv_info->length);
	}

	page_info->tlv_page_info_len = total_len;

	return 0;
}

int hinic3_get_tlv_xsfp_eeprom(void *hwdev, u8 *data, u32 len)
{
	int err = 0;
	struct tag_mag_cmd_get_xsfp_tlv_req xsfp_tlv_info_req = {0};

	xsfp_tlv_info_req.rsp_buf_len = XSFP_CMIS_INFO_MAX_SIZE;
	xsfp_tlv_info_req.port_id = hinic3_physical_port_id(hwdev);
	err = hinic3_get_sfp_tlv_info(hwdev, &g_xsfp_tlv_info,
				      &xsfp_tlv_info_req);
	if (err != 0)
		return err;

	err = hinic3_trans_cmis_tlv_info_to_buf(g_xsfp_tlv_info.tlv_buf,
						&g_page_info);
	if (err)
		return -ENOMEM;

	memcpy(data, g_page_info.tlv_page_info, len);

	return (err == 0) ? 0 : -ENOMEM;
}

int hinic3_get_sfp_info(void *hwdev, struct mag_cmd_get_xsfp_info *sfp_info)
{
	struct hinic3_nic_io *nic_io = NULL;
	struct hinic3_port_routine_cmd *rt_cmd = NULL;
	u8 sfp_info_status = 0;
	u16 out_size = sizeof(*sfp_info);
	int err;

	if (!hwdev || !sfp_info)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	rt_cmd = &nic_io->nic_cfg.rt_cmd;
	sfp_info_status = rt_cmd->std_sfp_info.head.status;
	mutex_lock(&nic_io->nic_cfg.sfp_mutex);
	if (rt_cmd->mpu_send_sfp_info) {
		if (sfp_info_status != 0) {
			mutex_unlock(&nic_io->nic_cfg.sfp_mutex);
			return (sfp_info_status == HINIC3_MGMT_CMD_UNSUPPORTED)
				? HINIC3_MGMT_CMD_UNSUPPORTED : -EIO;
		}

		memcpy(sfp_info, &rt_cmd->std_sfp_info, sizeof(*sfp_info));
		mutex_unlock(&nic_io->nic_cfg.sfp_mutex);
		return 0;
	}
	mutex_unlock(&nic_io->nic_cfg.sfp_mutex);

	sfp_info->port_id = hinic3_physical_port_id(hwdev);
	err = mag_msg_to_mgmt_sync(hwdev, MAG_CMD_GET_XSFP_INFO, sfp_info,
				   sizeof(*sfp_info), sfp_info, &out_size);
	if (sfp_info->head.status == HINIC3_MGMT_CMD_UNSUPPORTED)
		return HINIC3_MGMT_CMD_UNSUPPORTED;

	if (sfp_info->head.status == HINIC3_MGMT_CMD_UNSUPPORTED)
		return -EOPNOTSUPP;
	if (sfp_info->head.status != 0 || err != 0 || out_size == 0) {
		nic_err(nic_io->dev_hdl,
			"Failed to get port%u sfp eeprom information, err: %d, status: 0x%x, out size: 0x%x\n",
			hinic3_physical_port_id(hwdev), err,
			sfp_info->head.status, out_size);
		return -EIO;
	}

	return 0;
}

int hinic3_get_sfp_eeprom(void *hwdev, u8 *data, u32 len)
{
	struct mag_cmd_get_xsfp_info sfp_info;
	int err;

	if (!hwdev || !data || len > PAGE_SIZE)
		return -EINVAL;

	if (hinic3_if_sfp_absent(hwdev))
		return -ENXIO;

	memset(&sfp_info, 0, sizeof(sfp_info));

	err = hinic3_get_sfp_info(hwdev, &sfp_info);
	if (err)
		return err;

	memcpy(data, sfp_info.sfp_info, sizeof(sfp_info.sfp_info));

	return  0;
}

int hinic3_get_sfp_type(void *hwdev, u8 *sfp_type, u8 *sfp_type_ext)
{
	struct hinic3_nic_io *nic_io = NULL;
	struct hinic3_port_routine_cmd *rt_cmd = NULL;
	u8 sfp_data[STD_SFP_INFO_MAX_SIZE];
	int err = 0;

	if (!hwdev || !sfp_type || !sfp_type_ext)
		return -EINVAL;

	if (hinic3_if_sfp_absent(hwdev))
		return -ENXIO;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	rt_cmd = &nic_io->nic_cfg.rt_cmd;

	mutex_lock(&nic_io->nic_cfg.sfp_mutex);
	if (rt_cmd->mpu_send_sfp_info) {
		if (rt_cmd->std_sfp_info.head.status == 0) {
			*sfp_type = rt_cmd->std_sfp_info.sfp_info[0];
			*sfp_type_ext = rt_cmd->std_sfp_info.sfp_info[1];
			mutex_unlock(&nic_io->nic_cfg.sfp_mutex);
			return 0;
		}

		if (rt_cmd->std_sfp_info.head.status != HINIC3_MGMT_CMD_UNSUPPORTED) {
			mutex_unlock(&nic_io->nic_cfg.sfp_mutex);
			return -EIO;
		}

		err = HINIC3_MGMT_CMD_UNSUPPORTED; /* cmis */
	}
	mutex_unlock(&nic_io->nic_cfg.sfp_mutex);

	if (err == 0) {
		err = hinic3_get_sfp_eeprom(hwdev, (u8 *)sfp_data,
					    STD_SFP_INFO_MAX_SIZE);
	} else {
		/* mpu_send_sfp_info is false */
		err = hinic3_get_tlv_xsfp_eeprom(hwdev, (u8 *)sfp_data,
						 STD_SFP_INFO_MAX_SIZE);
	}

	if (err == HINIC3_MGMT_CMD_UNSUPPORTED)
		err = hinic3_get_tlv_xsfp_eeprom(hwdev, (u8 *)sfp_data,
						 STD_SFP_INFO_MAX_SIZE);

	if (err)
		return err;

	*sfp_type = sfp_data[0];
	*sfp_type_ext = sfp_data[1];

	return 0;
}

int hinic3_set_link_status_follow(void *hwdev, enum hinic3_link_follow_status status)
{
	struct mag_cmd_set_link_follow follow;
	struct hinic3_nic_io *nic_io = NULL;
	u16 out_size = sizeof(follow);
	int err;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	if (status >= HINIC3_LINK_FOLLOW_STATUS_MAX) {
		nic_err(nic_io->dev_hdl, "Invalid link follow status: %d\n", status);
		return -EINVAL;
	}

	memset(&follow, 0, sizeof(follow));
	follow.function_id = hinic3_global_func_id(hwdev);
	follow.follow = status;

	err = mag_msg_to_mgmt_sync(hwdev, MAG_CMD_SET_LINK_FOLLOW, &follow,
				   sizeof(follow), &follow, &out_size);
	if ((follow.head.status != HINIC3_MGMT_CMD_UNSUPPORTED && follow.head.status) ||
	    err || !out_size) {
		nic_err(nic_io->dev_hdl, "Failed to set link status follow port status, err: %d, status: 0x%x, out size: 0x%x\n",
			err, follow.head.status, out_size);
		return -EFAULT;
	}

	return follow.head.status;
}

int hinic3_update_pf_bw(void *hwdev)
{
	struct nic_port_info port_info = {0};
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (hinic3_func_type(hwdev) == TYPE_VF || !HINIC3_SUPPORT_RATE_LIMIT(hwdev))
		return 0;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	err = hinic3_get_port_info(hwdev, &port_info, HINIC3_CHANNEL_NIC);
	if (err) {
		nic_err(nic_io->dev_hdl, "Failed to get port info\n");
		return -EIO;
	}

	err = hinic3_set_pf_rate(hwdev, port_info.speed);
	if (err) {
		nic_err(nic_io->dev_hdl, "Failed to set pf bandwidth\n");
		return err;
	}

	return 0;
}

int hinic3_set_pf_bw_limit(void *hwdev, u32 bw_limit)
{
	struct hinic3_nic_io *nic_io = NULL;
	u32 old_bw_limit;
	u8 link_state = 0;
	int err;

	if (!hwdev)
		return -EINVAL;

	if (hinic3_func_type(hwdev) == TYPE_VF)
		return 0;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	if (bw_limit > MAX_LIMIT_BW) {
		nic_err(nic_io->dev_hdl, "Invalid bandwidth: %u\n", bw_limit);
		return -EINVAL;
	}

	err = hinic3_get_link_state(hwdev, &link_state);
	if (err) {
		nic_err(nic_io->dev_hdl, "Failed to get link state\n");
		return -EIO;
	}

	if (!link_state) {
		nic_err(nic_io->dev_hdl, "Link status must be up when setting pf tx rate\n");
		return -EINVAL;
	}

	if (nic_io->direct == HINIC3_NIC_TX) {
		old_bw_limit = nic_io->nic_cfg.pf_bw_tx_limit;
		nic_io->nic_cfg.pf_bw_tx_limit = bw_limit;
	} else {
		old_bw_limit = nic_io->nic_cfg.pf_bw_rx_limit;
		nic_io->nic_cfg.pf_bw_rx_limit = bw_limit;
	}

	err = hinic3_update_pf_bw(hwdev);
	if (err) {
		if (nic_io->direct == HINIC3_NIC_TX)
			nic_io->nic_cfg.pf_bw_tx_limit = old_bw_limit;
		else
			nic_io->nic_cfg.pf_bw_rx_limit = old_bw_limit;
	}

	return err;
}

static const struct vf_msg_handler vf_mag_cmd_handler[] = {
	{
		.cmd = MAG_CMD_GET_LINK_STATUS,
		.handler = hinic3_get_vf_link_status_msg_handler,
	},
};

/* pf/ppf handler mbox msg from vf */
int hinic3_pf_mag_mbox_handler(void *hwdev, u16 vf_id,
			       u16 cmd, void *buf_in, u16 in_size,
			       void *buf_out, u16 *out_size)
{
	u32 index, cmd_size = ARRAY_LEN(vf_mag_cmd_handler);
	struct hinic3_nic_io *nic_io = NULL;
	const struct vf_msg_handler *handler = NULL;

	if (!hwdev)
		return -EFAULT;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;


	for (index = 0; index < cmd_size; index++) {
		handler = &vf_mag_cmd_handler[index];
		if (cmd == handler->cmd)
			return handler->handler(nic_io, vf_id, buf_in, in_size,
						buf_out, out_size);
	}

	nic_warn(nic_io->dev_hdl, "NO handler for mag cmd: %u received from vf id: %u\n",
		 cmd, vf_id);

	return -EINVAL;
}

static struct nic_event_handler mag_cmd_handler[] = {
	{
		.cmd = MAG_CMD_GET_LINK_STATUS,
		.handler = link_status_event_handler,
	},

	{
		.cmd = MAG_CMD_EVENT_PORT_INFO,
		.handler = port_info_event_printf,
	},

	{
		.cmd = MAG_CMD_WIRE_EVENT,
		.handler = cable_plug_event,
	},

	{
		.cmd = MAG_CMD_GET_XSFP_INFO,
		.handler = port_sfp_info_event,
	},

	{
		.cmd = MAG_CMD_GET_XSFP_PRESENT,
		.handler = port_sfp_abs_event,
	},

	{
		.cmd = MAG_CMD_GET_BOND_STATUS,
		.handler = bond_status_event_handler,
	},

	{
		.cmd = MAG_CMD_GET_XSFP_TLV_INFO,
		.handler = port_xsfp_tlv_info_event,
	},
};

static int hinic3_mag_event_handler(void *hwdev, u16 cmd,
				    void *buf_in, u16 in_size, void *buf_out,
				    u16 *out_size)
{
	struct hinic3_nic_io *nic_io = NULL;
	u32 size = ARRAY_LEN(mag_cmd_handler);
	u32 i;

	if (!hwdev)
		return -EINVAL;

	*out_size = 0;
	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	for (i = 0; i < size; i++) {
		if (cmd == mag_cmd_handler[i].cmd) {
			mag_cmd_handler[i].handler(hwdev, buf_in, in_size,
						   buf_out, out_size);
			return 0;
		}
	}

	/* can't find this event cmd */
	sdk_warn(nic_io->dev_hdl, "Unsupported mag event, cmd: %u\n", cmd);
	*out_size = sizeof(struct mgmt_msg_head);
	((struct mgmt_msg_head *)buf_out)->status = HINIC3_MGMT_CMD_UNSUPPORTED;

	return 0;
}

int hinic3_vf_mag_event_handler(void *hwdev, u16 cmd,
				void *buf_in, u16 in_size, void *buf_out,
				u16 *out_size)
{
	return hinic3_mag_event_handler(hwdev, cmd, buf_in, in_size,
					buf_out, out_size);
}

/* pf/ppf handler mgmt cpu report hilink event */
void hinic3_pf_mag_event_handler(void *pri_handle, u16 cmd,
				 void *buf_in, u16 in_size, void *buf_out,
				 u16 *out_size)
{
	hinic3_mag_event_handler(pri_handle, cmd, buf_in, in_size,
				 buf_out, out_size);
}

static int _mag_msg_to_mgmt_sync(void *hwdev, u16 cmd, void *buf_in,
				 u16 in_size, void *buf_out, u16 *out_size,
				 u16 channel)
{
	u32 i, cmd_cnt = ARRAY_LEN(vf_mag_cmd_handler);
	bool cmd_to_pf = false;

	if (hinic3_func_type(hwdev) == TYPE_VF &&
	    !hinic3_is_slave_host(hwdev)) {
		for (i = 0; i < cmd_cnt; i++) {
			if (cmd == vf_mag_cmd_handler[i].cmd) {
				cmd_to_pf = true;
				break;
			}
		}
	}

	if (cmd_to_pf)
		return hinic3_mbox_to_pf(hwdev, HINIC3_MOD_HILINK, cmd, buf_in,
					 in_size, buf_out, out_size, 0,
					 channel);

	return hinic3_msg_to_mgmt_sync(hwdev, HINIC3_MOD_HILINK, cmd, buf_in,
				       in_size, buf_out, out_size, 0, channel);
}

static int mag_msg_to_mgmt_sync(void *hwdev, u16 cmd, void *buf_in, u16 in_size,
				void *buf_out, u16 *out_size)
{
	return _mag_msg_to_mgmt_sync(hwdev, cmd, buf_in, in_size, buf_out,
				     out_size, HINIC3_CHANNEL_NIC);
}

static int mag_msg_to_mgmt_sync_ch(void *hwdev, u16 cmd, void *buf_in,
				   u16 in_size, void *buf_out, u16 *out_size,
				   u16 channel)
{
	return _mag_msg_to_mgmt_sync(hwdev, cmd, buf_in, in_size, buf_out,
				     out_size, channel);
}

#if defined(ETHTOOL_GFECPARAM) && defined(ETHTOOL_SFECPARAM)
struct fecparam_value_map {
	u8 hinic3_fec_offset;
	u8 hinic3_fec_value;
	u8 ethtool_fec_value;
};

static void fecparam_convert(u32 opcode, u8 in_fec_param, u8 *out_fec_param)
{
	u8 i;
	u8 fec_value_table_lenth;
	struct fecparam_value_map fec_value_table[] = {
	{PORT_FEC_NOT_SET,  BIT(PORT_FEC_NOT_SET),  ETHTOOL_FEC_NONE},
	{PORT_FEC_RSFEC,    BIT(PORT_FEC_RSFEC),    ETHTOOL_FEC_RS},
	{PORT_FEC_BASEFEC,  BIT(PORT_FEC_BASEFEC),  ETHTOOL_FEC_BASER},
	{PORT_FEC_NOFEC,    BIT(PORT_FEC_NOFEC),    ETHTOOL_FEC_OFF},
#ifdef ETHTOOL_FEC_LLRS
	{PORT_FEC_LLRSFEC,  BIT(PORT_FEC_LLRSFEC),  ETHTOOL_FEC_LLRS},
#endif
	{PORT_FEC_AUTO,     BIT(PORT_FEC_AUTO),     ETHTOOL_FEC_AUTO}
	};

	*out_fec_param = 0;
	fec_value_table_lenth = (u8)(sizeof(fec_value_table) / sizeof(struct fecparam_value_map));

	if (opcode == MAG_CMD_OPCODE_SET) {
		for (i = 0; i < fec_value_table_lenth; i++) {
			if ((in_fec_param &
			     fec_value_table[i].ethtool_fec_value) != 0)
				/* The MPU uses the offset to determine the FEC mode. */
				*out_fec_param =
					fec_value_table[i].hinic3_fec_offset;
		}
	}

	if (opcode == MAG_CMD_OPCODE_GET) {
		for (i = 0; i < fec_value_table_lenth; i++) {
			if ((in_fec_param &
			     fec_value_table[i].hinic3_fec_value) != 0)
				*out_fec_param |=
					fec_value_table[i].ethtool_fec_value;
	}
	}
}

/* When the ethtool is used to set the FEC mode */
static bool check_fecparam_is_valid(u8 fec_param)
{
	if (fec_param == ETHTOOL_FEC_RS ||
#ifdef ETHTOOL_FEC_LLRS
	    fec_param == ETHTOOL_FEC_LLRS ||
#endif
	    fec_param == ETHTOOL_FEC_BASER ||
	    fec_param == ETHTOOL_FEC_OFF) {
		return true;
	}
	return false;
}

int set_fecparam(void *hwdev, u8 fecparam)
{
	struct mag_cmd_cfg_fec_mode fec_msg = {0};
	struct hinic3_nic_io *nic_io = NULL;
	u16 out_size = sizeof(fec_msg);
	u8 advertised_fec = 0;
	int err;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	if (check_fecparam_is_valid(fecparam) == false) {
		nic_err(nic_io->dev_hdl, "fec param is invalid, failed to set fec param\n");
		return -EINVAL;
	}
	fecparam_convert(MAG_CMD_OPCODE_SET, fecparam, &advertised_fec);
	fec_msg.opcode = MAG_CMD_OPCODE_SET;
	fec_msg.port_id = hinic3_physical_port_id(hwdev);
	fec_msg.advertised_fec = advertised_fec;
	err = mag_msg_to_mgmt_sync_ch(hwdev, MAG_CMD_CFG_FEC_MODE,
				      &fec_msg, sizeof(fec_msg),
				      &fec_msg, &out_size, HINIC3_CHANNEL_NIC);
	if (err != 0 || fec_msg.head.status != 0)
		return -EINVAL;
	return 0;
}

int get_fecparam(void *hwdev, u8 *advertised_fec, u8 *supported_fec)
{
	struct mag_cmd_cfg_fec_mode fec_msg = {0};
	struct hinic3_nic_io *nic_io = NULL;
	u16 out_size = sizeof(fec_msg);
	int err;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	fec_msg.opcode = MAG_CMD_OPCODE_GET;
	fec_msg.port_id = hinic3_physical_port_id(hwdev);
	err = mag_msg_to_mgmt_sync_ch(hwdev, MAG_CMD_CFG_FEC_MODE,
				      &fec_msg, sizeof(fec_msg),
				      &fec_msg, &out_size, HINIC3_CHANNEL_NIC);
	if (err != 0 || fec_msg.head.status != 0)
		return -EINVAL;

	/* fec_msg.advertised_fec: bit offset,
	 *value is BIT(fec_msg.advertised_fec);  fec_msg.supported_fec: value
	 */
	fecparam_convert(MAG_CMD_OPCODE_GET, BIT(fec_msg.advertised_fec),
			 advertised_fec);
	fecparam_convert(MAG_CMD_OPCODE_GET, fec_msg.supported_fec,
			 supported_fec);
	return 0;
}
#endif

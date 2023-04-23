// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023 Hisilicon Limited.

#include "hclge_main.h"
#include "hnae3.h"
#include "hnae3_ext.h"
#include "hclge_cmd.h"
#include "hclge_ext.h"
#include "hclge_tm.h"

static nic_event_fn_t nic_event_call;

/* We use a lock to ensure that the address of the nic_event_call function
 * is valid when it is called. Avoid null pointer exceptions caused by
 * external unregister during invoking.
 */
static DEFINE_MUTEX(hclge_nic_event_lock);

static int hclge_set_pfc_storm_para(struct hclge_dev *hdev, void *data,
				    size_t length)
{
	struct hclge_pfc_storm_para_cmd *para_cmd;
	struct hnae3_pfc_storm_para *para;
	struct hclge_desc desc;
	int ret;

	if (length != sizeof(struct hnae3_pfc_storm_para))
		return -EINVAL;

	hclge_cmd_setup_basic_desc(&desc, HCLGE_OPC_CFG_PAUSE_STORM_PARA,
				   false);
	para = (struct hnae3_pfc_storm_para *)data;
	para_cmd = (struct hclge_pfc_storm_para_cmd *)desc.data;
	para_cmd->dir = cpu_to_le32(para->dir);
	para_cmd->enable = cpu_to_le32(para->enable);
	para_cmd->period_ms = cpu_to_le32(para->period_ms);
	para_cmd->times = cpu_to_le32(para->times);
	para_cmd->recovery_period_ms = cpu_to_le32(para->recovery_period_ms);

	ret = hclge_cmd_send(&hdev->hw, &desc, 1);
	if (ret)
		dev_err(&hdev->pdev->dev,
			"failed to set pfc storm para, ret = %d\n", ret);
	return ret;
}

static int hclge_get_pfc_storm_para(struct hclge_dev *hdev, void *data,
				    size_t length)
{
	struct hclge_pfc_storm_para_cmd *para_cmd;
	struct hnae3_pfc_storm_para *para;
	struct hclge_desc desc;
	int ret;

	if (length != sizeof(struct hnae3_pfc_storm_para))
		return -EINVAL;

	hclge_cmd_setup_basic_desc(&desc, HCLGE_OPC_CFG_PAUSE_STORM_PARA, true);
	para = (struct hnae3_pfc_storm_para *)data;
	para_cmd = (struct hclge_pfc_storm_para_cmd *)desc.data;
	para_cmd->dir = cpu_to_le32(para->dir);
	ret = hclge_cmd_send(&hdev->hw, &desc, 1);
	if (ret) {
		dev_err(&hdev->pdev->dev,
			"failed to get pfc storm para, ret = %d\n", ret);
		return ret;
	}

	para->enable = le32_to_cpu(para_cmd->enable);
	para->period_ms = le32_to_cpu(para_cmd->period_ms);
	para->times = le32_to_cpu(para_cmd->times);
	para->recovery_period_ms = le32_to_cpu(para_cmd->recovery_period_ms);

	return 0;
}

static int hclge_notify_packet_para_cmd_send(struct hclge_dev *hdev,
					     struct hclge_notify_pkt_param_cmd *param_cmd)
{
#define HCLGE_NOTIFY_PKT_DESC_NUM 4

	struct hclge_desc desc[HCLGE_NOTIFY_PKT_DESC_NUM];
	u32 i, desc_data_len;

	desc_data_len = ARRAY_SIZE(desc[0].data);
	for (i = 0; i < HCLGE_NOTIFY_PKT_DESC_NUM; i++) {
		hclge_cmd_setup_basic_desc(&desc[i], HCLGE_OPC_SET_NOTIFY_PKT,
					   false);
		if (i != HCLGE_NOTIFY_PKT_DESC_NUM - 1)
			desc[i].flag |= cpu_to_le16(HCLGE_COMM_CMD_FLAG_NEXT);
	}

	for (i = 0; i < HCLGE_NOTIFY_PKT_DESC_NUM * desc_data_len; i++)
		desc[i / desc_data_len].data[i % desc_data_len] =
						     *((__le32 *)param_cmd + i);

	return hclge_cmd_send(&hdev->hw, desc, HCLGE_NOTIFY_PKT_DESC_NUM);
}

static int hclge_set_notify_packet_para(struct hclge_dev *hdev,
					void *data, size_t length)
{
	struct hnae3_notify_pkt_param *param = (struct hnae3_notify_pkt_param *)data;
	struct hclge_notify_pkt_param_cmd param_cmd;
	u32 i, pkt_cfg = 0;
	int ret;

	if (length != sizeof(struct hnae3_notify_pkt_param))
		return -EINVAL;

	if (!hnae3_ae_dev_notify_pkt_supported(hdev->ae_dev))
		return -EOPNOTSUPP;

	if (param->enable)
		pkt_cfg = HCLGE_NOTIFY_PARA_CFG_PKT_EN;
	hnae3_set_field(pkt_cfg, HCLGE_NOTIFY_PARA_CFG_PKT_NUM_M,
			HCLGE_NOTIFY_PARA_CFG_PKT_NUM_S, param->num);

	param_cmd.cfg = cpu_to_le32(pkt_cfg);
	param_cmd.ipg = cpu_to_le32(param->ipg);
	for (i = 0; i < ARRAY_SIZE(param_cmd.data); i++)
		param_cmd.data[i] = cpu_to_le32(*((u32 *)param->data + i));

	hnae3_set_bit(param_cmd.vld_cfg, 0, 1);
	hnae3_set_bit(param_cmd.vld_ipg, 0, 1);
	hnae3_set_bit(param_cmd.vld_data, 0, 1);

	ret = hclge_notify_packet_para_cmd_send(hdev, &param_cmd);
	if (ret) {
		dev_err(&hdev->pdev->dev,
			"failed to set notify packet content, ret = %d\n", ret);
		return ret;
	}

	param->init = 1;
	memcpy(&hdev->notify_param, param, sizeof(*param));
	return 0;
}

static int hclge_set_notify_packet_start(struct hclge_dev *hdev,
					 void *data, size_t length)
{
	u32 pkt_cfg = HCLGE_NOTIFY_PARA_CFG_START_EN;
	struct hclge_notify_pkt_param_cmd param_cmd;
	int ret;

	if (!hnae3_ae_dev_notify_pkt_supported(hdev->ae_dev))
		return -EOPNOTSUPP;

	memset(&param_cmd, 0, sizeof(param_cmd));
	param_cmd.cfg = cpu_to_le32(pkt_cfg);
	hnae3_set_bit(param_cmd.vld_cfg, 0, 1);

	ret = hclge_notify_packet_para_cmd_send(hdev, &param_cmd);
	if (ret)
		dev_err(&hdev->pdev->dev,
			"failed to send notify packet, ret = %d\n", ret);
	return ret;
}

static int hclge_torus_cfg_switch(struct hclge_dev *hdev, bool is_rocee,
				  bool enabled)
{
	struct hclge_mac_vlan_switch_cmd *req;
	struct hclge_desc desc;
	int ret;

	hclge_cmd_setup_basic_desc(&desc, HCLGE_OPC_CONFIG_SWITCH_PARAM, true);
	req = (struct hclge_mac_vlan_switch_cmd *)desc.data;
	req->roce_sel = is_rocee ? 1 : 0;
	/* set 0 to let firmware choose current function */
	req->func_id = 0;
	ret = hclge_cmd_send(&hdev->hw, &desc, 1);
	if (ret) {
		dev_err(&hdev->pdev->dev,
			"failed to get switch param, ret = %d\n", ret);
		return ret;
	}

	hnae3_set_bit(req->switch_param, HCLGE_SWITCH_ALW_LPBK_B, 1);
	hnae3_set_bit(req->switch_param, HCLGE_SWITCH_ALW_LCL_LPBK_B, 0);
	hnae3_set_bit(req->switch_param, HCLGE_SWITCH_ANTI_SPOOF_B, enabled);
	if (!is_rocee)
		hnae3_set_bit(req->switch_param, HCLGE_SWITCH_ALW_DST_OVRD_B,
			      enabled);

	hclge_comm_cmd_reuse_desc(&desc, false);
	ret = hclge_cmd_send(&hdev->hw, &desc, 1);
	if (ret)
		dev_err(&hdev->pdev->dev,
			"failed to set switch param, ret = %d\n", ret);

	return ret;
}

static int hclge_torus_cfg_vlan_filter(struct hclge_dev *hdev,
				       bool enabled)
{
	struct hclge_vlan_filter_ctrl_cmd *req;
	struct hclge_desc desc;
	int ret;

	hclge_cmd_setup_basic_desc(&desc, HCLGE_OPC_CONFIG_VLAN_FILTER, true);
	req = (struct hclge_vlan_filter_ctrl_cmd *)desc.data;
	req->vlan_type = HCLGE_FILTER_TYPE_PORT;
	ret = hclge_cmd_send(&hdev->hw, &desc, 1);
	if (ret) {
		dev_err(&hdev->pdev->dev,
			"failed to get torus vlan filter, ret = %d\n", ret);
		return ret;
	}

	hnae3_set_bit(req->vlan_fe, HCLGE_VLAN_FE_NIC_INGRESS, !enabled);
	hnae3_set_bit(req->vlan_fe, HCLGE_VLAN_FE_ROCEE_INGRESS, !enabled);
	req->vlan_type = HCLGE_FILTER_TYPE_PORT;

	hclge_comm_cmd_reuse_desc(&desc, false);
	ret = hclge_cmd_send(&hdev->hw, &desc, 1);
	if (ret)
		dev_err(&hdev->pdev->dev,
			"failed to set torus vlan filter, ret = %d\n", ret);

	return ret;
}

static int hclge_torus_cfg(struct hclge_dev *hdev,
			   struct hnae3_torus_param *param)
{
	struct hclge_torus_cfg_cmd *req;
	struct hclge_desc desc;
	u32 lan_fwd_tc_cfg = 0;
	u32 lan_port_pair = 0;
	int ret;

	hclge_cmd_setup_basic_desc(&desc, HCLGE_OPC_CONFIG_1D_TORUS, true);
	req = (struct hclge_torus_cfg_cmd *)desc.data;
	ret = hclge_cmd_send(&hdev->hw, &desc, 1);
	if (ret) {
		dev_err(&hdev->pdev->dev,
			"failed to get torus config, ret = %d\n", ret);
		return ret;
	}

	req->lan_port_pair = cpu_to_le32(param->mac_id &
					 HCLGE_TORUS_MAC_ID_MASK);
	hnae3_set_bit(lan_port_pair, HCLGE_UC_LAN_PAIR_EN, 1);
	hnae3_set_bit(lan_port_pair, HCLGE_MC_BC_LAN_PAIR_EN, 1);
	hnae3_set_bit(lan_port_pair, HCLGE_LLDP_LAN_PAIR_EN, 1);
	hnae3_set_bit(lan_port_pair, HCLGE_TC2VLANPRI_MAPPING_EN, 1);
	hnae3_set_bit(lan_port_pair, HCLGE_TORUS_LPBK_DROP_EN, 1);
	if (param->enable)
		req->lan_port_pair |= cpu_to_le32(lan_port_pair);

	if (!param->is_node0) {
		req->lan_fwd_tc_cfg &= cpu_to_le32(~HCLGE_TORUS_TC1_DROP_EN);
		lan_fwd_tc_cfg &= ~HCLGE_TOURS_TCX_MAP_TCY_MASK;
		lan_fwd_tc_cfg |= HCLGE_TOURS_TCX_MAP_TCY_INIT &
				  HCLGE_TOURS_TCX_MAP_TCY_MASK;
		req->lan_fwd_tc_cfg |= cpu_to_le32(lan_fwd_tc_cfg);
	} else {
		req->lan_fwd_tc_cfg |= cpu_to_le32(HCLGE_TORUS_TC1_DROP_EN);
		lan_fwd_tc_cfg &= ~HCLGE_TOURS_TCX_MAP_TCY_MASK;
		lan_fwd_tc_cfg |= HCLGE_TOURS_TCX_MAP_TCY_NODE0_INIT &
				  HCLGE_TOURS_TCX_MAP_TCY_MASK;
		req->lan_fwd_tc_cfg |= cpu_to_le32(lan_fwd_tc_cfg);
	}

	req->torus_en = cpu_to_le32(param->enable);
	hclge_comm_cmd_reuse_desc(&desc, false);
	ret = hclge_cmd_send(&hdev->hw, &desc, 1);
	if (ret)
		dev_err(&hdev->pdev->dev, "failed to set torus, ret = %d\n",
			ret);

	return ret;
}

static int hclge_set_torus_param(struct hclge_dev *hdev, void *data,
				 size_t length)
{
	struct hnae3_torus_param *param = (struct hnae3_torus_param *)data;
	int ret;

	if (hdev->ae_dev->dev_version == HNAE3_DEVICE_VERSION_V4)
		return -EOPNOTSUPP;

	if (length != sizeof(struct hnae3_torus_param))
		return -EINVAL;

	ret = hclge_torus_cfg_switch(hdev, false, !!param->enable);
	if (ret) {
		dev_err(&hdev->pdev->dev,
			"failed to config nic switch param, ret = %d\n", ret);
		return ret;
	}

	ret = hclge_torus_cfg_switch(hdev, true, !!param->enable);
	if (ret) {
		dev_err(&hdev->pdev->dev,
			"failed to config roce switch param, ret = %d\n", ret);
		return ret;
	}

	ret = hclge_torus_cfg_vlan_filter(hdev, !!param->enable);
	if (ret)
		return ret;

	ret = hclge_torus_cfg(hdev, param);
	if (ret)
		return ret;

	hdev->torus_param = *param;
	return 0;
}

static int hclge_get_torus_param(struct hclge_dev *hdev, void *data,
				 size_t length)
{
	struct hnae3_torus_param *param = (struct hnae3_torus_param *)data;
	struct hclge_torus_cfg_cmd *req;
	struct hclge_desc desc;
	int ret;

	if (hdev->ae_dev->dev_version == HNAE3_DEVICE_VERSION_V4)
		return -EOPNOTSUPP;

	if (length != sizeof(struct hnae3_torus_param))
		return -EINVAL;

	hclge_cmd_setup_basic_desc(&desc, HCLGE_OPC_CONFIG_1D_TORUS, true);
	req = (struct hclge_torus_cfg_cmd *)desc.data;
	ret = hclge_cmd_send(&hdev->hw, &desc, 1);
	if (ret) {
		dev_err(&hdev->pdev->dev,
			"failed to get torus param, ret = %d\n", ret);
		return ret;
	}

	param->mac_id =
		le32_to_cpu(req->lan_port_pair) & HCLGE_TORUS_MAC_ID_MASK;
	param->enable = le32_to_cpu(req->torus_en);

	return 0;
}

static int hclge_clean_stats64(struct hclge_dev *hdev, void *data,
			       size_t length)
{
	struct hnae3_knic_private_info *kinfo;
	struct hclge_comm_tqp *tqp;
	int i;

	kinfo = &hdev->vport[0].nic.kinfo;
	for (i = 0; i < kinfo->num_tqps; i++) {
		tqp = container_of(kinfo->tqp[i], struct hclge_comm_tqp, q);
		memset(&tqp->tqp_stats, 0, sizeof(struct hclge_comm_tqp_stats));
	}
	memset(&hdev->mac_stats, 0, sizeof(struct hclge_mac_stats));
	return 0;
}

static int hclge_get_info_from_cmd(struct hclge_dev *hdev,
				   struct hclge_desc *desc, u32 num, int opcode)
{
	u32 i;

	for (i = 0; i < num; i++) {
		hclge_cmd_setup_basic_desc(desc + i, opcode, true);
		if (i != num - 1)
			desc[i].flag |= cpu_to_le16(HCLGE_COMM_CMD_FLAG_NEXT);
	}

	return hclge_cmd_send(&hdev->hw, desc, num);
}

static int hclge_get_extend_port_id_info(struct hclge_dev *hdev,
					 void *data, size_t length)
{
	struct hane3_port_ext_id_info *info;
	struct hclge_id_info_cmd *info_cmd;
	struct hclge_desc desc;
	int ret;

	if (length != sizeof(struct hane3_port_ext_id_info))
		return -EINVAL;

	ret = hclge_get_info_from_cmd(hdev, &desc, 1, HCLGE_OPC_CHIP_ID_GET);
	if (ret) {
		dev_err(&hdev->pdev->dev,
			"failed to get extend port id info, ret = %d\n",
			ret);
		return ret;
	}

	info_cmd = (struct hclge_id_info_cmd *)desc.data;
	info = (struct hane3_port_ext_id_info *)data;
	info->chip_id = le32_to_cpu(info_cmd->chip_id);
	info->mac_id = le32_to_cpu(info_cmd->mac_id);
	info->io_die_id = le32_to_cpu(info_cmd->io_die_id);
	return 0;
}

static int hclge_get_extend_port_num_info(struct hclge_dev *hdev,
					  void *data, size_t length)
{
	struct hane3_port_ext_num_info *num_info;
	struct hclge_num_info_cmd *resp;
	struct hclge_desc desc;
	int ret;

	if (length != sizeof(struct hane3_port_ext_num_info))
		return -EINVAL;

	ret = hclge_get_info_from_cmd(hdev, &desc, 1, HCLGE_OPC_GET_CHIP_NUM);
	if (ret) {
		dev_err(&hdev->pdev->dev,
			"failed to get extend port number info, ret = %d\n", ret);
		return ret;
	}

	resp = (struct hclge_num_info_cmd *)(desc.data);
	num_info = (struct hane3_port_ext_num_info *)data;
	num_info->chip_num = le32_to_cpu(resp->chip_num);
	num_info->io_die_num = le32_to_cpu(resp->io_die_num);
	return 0;
}

static int hclge_get_port_num(struct hclge_dev *hdev, void *data,
			      size_t length)
{
	struct hclge_port_num_info_cmd *resp;
	struct hclge_desc desc;
	int ret;

	if (length != sizeof(u32))
		return -EINVAL;

	ret = hclge_get_info_from_cmd(hdev, &desc, 1, HCLGE_OPC_GET_PORT_NUM);
	if (ret) {
		dev_err(&hdev->pdev->dev,
			"failed to get port number, ret = %d\n", ret);
		return ret;
	}

	resp = (struct hclge_port_num_info_cmd *)(desc.data);
	*(u32 *)data = le32_to_cpu(resp->port_num);
	return 0;
}

static int hclge_get_sfp_present(struct hclge_dev *hdev, void *data,
				 size_t length)
{
	struct hclge_sfp_present_cmd *resp;
	struct hclge_desc desc;
	int ret;

	if (length != sizeof(u32))
		return -EINVAL;

	ret = hclge_get_info_from_cmd(hdev, &desc, 1, HCLGE_OPC_SFP_GET_PRESENT);
	if (ret) {
		dev_err(&hdev->pdev->dev, "failed to get sfp present, ret = %d\n", ret);
		return ret;
	}

	resp = (struct hclge_sfp_present_cmd *)desc.data;
	*(u32 *)data = le32_to_cpu(resp->sfp_present);
	return 0;
}

static int hclge_set_sfp_state(struct hclge_dev *hdev, void *data,
			       size_t length)
{
	struct hclge_sfp_enable_cmd *req;
	struct hclge_desc desc;
	u32 state;
	int ret;

	if (length != sizeof(u32))
		return -EINVAL;

	state = *(u32 *)data;
	hclge_cmd_setup_basic_desc(&desc, HCLGE_OPC_SFP_SET_STATUS, false);
	req = (struct hclge_sfp_enable_cmd *)desc.data;
	req->sfp_enable = cpu_to_le32(state);

	ret = hclge_cmd_send(&hdev->hw, &desc, 1);
	if (ret)
		dev_err(&hdev->pdev->dev,
			"failed to set sfp state, ret = %d\n", ret);

	return ret;
}

static int hclge_set_net_lane_status(struct hclge_dev *hdev,
				     u32 enable)
{
	struct hclge_desc desc;
	int ret;

	hclge_cmd_setup_basic_desc(&desc, HCLGE_OPC_DISABLE_NET_LANE, false);
	desc.data[0] = cpu_to_le32(enable);

	ret = hclge_cmd_send(&hdev->hw, &desc, 1);
	if (ret)
		dev_err(&hdev->pdev->dev,
			"failed to set net lane status, ret = %d\n", ret);

	return ret;
}

static int hclge_disable_net_lane(struct hclge_dev *hdev, void *data,
				  size_t length)
{
	return hclge_set_net_lane_status(hdev, 0);
}

static int hclge_get_net_lane_status(struct hclge_dev *hdev, void *data,
				     size_t length)
{
	struct hclge_desc desc;
	int ret;

	if (length != sizeof(u32))
		return -EINVAL;

	ret = hclge_get_info_from_cmd(hdev, &desc, 1, HCLGE_OPC_DISABLE_NET_LANE);
	if (ret) {
		dev_err(&hdev->pdev->dev,
			"failed to get net lane status, ret = %d\n", ret);
		return ret;
	}

	*(u32 *)data = le32_to_cpu(desc.data[0]);
	return 0;
}

static int hclge_disable_nic_clock(struct hclge_dev *hdev, void *data,
				   size_t length)
{
	struct hclge_desc desc;
	int ret;

	hclge_cmd_setup_basic_desc(&desc, HCLGE_OPC_CONFIG_NIC_CLOCK, false);
	desc.data[0] = 0;

	ret = hclge_cmd_send(&hdev->hw, &desc, 1);
	if (ret)
		dev_err(&hdev->pdev->dev,
			"failed to disable nic clock, ret = %d\n", ret);
	return ret;
}

static int hclge_set_pause_trans_time(struct hclge_dev *hdev, void *data,
				      size_t length)
{
	struct hclge_cfg_pause_param_cmd *pause_param;
	struct hclge_desc desc;
	u16 pause_trans_time;
	int ret;

	if (length != sizeof(u16))
		return -EINVAL;

	pause_param = (struct hclge_cfg_pause_param_cmd *)desc.data;
	ret = hclge_get_info_from_cmd(hdev, &desc, 1, HCLGE_OPC_CFG_MAC_PARA);
	if (ret) {
		dev_err(&hdev->pdev->dev,
			"failed to get pause cfg info, ret = %d\n", ret);
		return ret;
	}

	pause_trans_time = *(u16 *)data;
	if (pause_trans_time == le16_to_cpu(pause_param->pause_trans_time))
		return 0;

	ret = hclge_pause_param_cfg(hdev, pause_param->mac_addr,
				    pause_param->pause_trans_gap,
				    pause_trans_time);
	if (ret) {
		dev_err(&hdev->pdev->dev,
			"failed to set pause trans time, ret = %d\n", ret);
		return ret;
	}

	hdev->tm_info.pause_time = pause_trans_time;
	return 0;
}

static int hclge_get_hilink_ref_los(struct hclge_dev *hdev, void *data,
				    size_t length)
{
	struct hclge_port_fault_cmd *fault_cmd;
	struct hclge_desc desc;
	int ret;

	if (length != sizeof(struct hnae3_port_fault))
		return -EINVAL;

	fault_cmd = (struct hclge_port_fault_cmd *)desc.data;
	ret = hclge_get_info_from_cmd(hdev, &desc, 1, HCLGE_OPC_CFG_GET_HILINK_REF_LOS);
	if (ret) {
		dev_err(&hdev->pdev->dev,
			"failed to get hilink ref los, ret = %d\n", ret);
		return ret;
	}

	*(u32 *)data = le32_to_cpu(fault_cmd->fault_status);
	return 0;
}

static int hclge_get_port_fault_status(struct hclge_dev *hdev, void *data,
				       size_t length)
{
	struct hclge_port_fault_cmd *fault_cmd;
	struct hnae3_port_fault *para;
	struct hclge_desc desc;
	int ret;

	if (length != sizeof(struct hnae3_port_fault))
		return -EINVAL;

	para = (struct hnae3_port_fault *)data;
	fault_cmd = (struct hclge_port_fault_cmd *)desc.data;
	hclge_cmd_setup_basic_desc(&desc, HCLGE_OPC_GET_PORT_FAULT_STATUS, true);
	fault_cmd->port_type = cpu_to_le32(para->fault_type);
	ret = hclge_cmd_send(&hdev->hw, &desc, 1);
	if (ret) {
		dev_err(&hdev->pdev->dev,
			"failed to get port fault status, type = %u, ret = %d\n",
			para->fault_type, ret);
		return ret;
	}

	para->fault_status = le32_to_cpu(fault_cmd->fault_status);

	return 0;
}

static int hclge_get_port_wire_type(struct hclge_dev *hdev, void *data,
				    size_t length)
{
	u8 module_type;

	if (length != sizeof(u32))
		return -EINVAL;

	hclge_get_media_type(&hdev->vport[0].nic, NULL, &module_type);
	*(u32 *)data = module_type;
	return 0;
}

static void hclge_set_phy_state(struct hclge_dev *hdev, bool enable)
{
	struct phy_device *phydev = hdev->hw.mac.phydev;

	if (!phydev)
		return;

	if (enable && (phydev->state == PHY_READY || phydev->state == PHY_HALTED))
		phy_start(phydev);
	else if (!enable && (phy_is_started(phydev) || phydev->state == PHY_DOWN ||
			     phydev->state == PHY_ERROR))
		phy_stop(phydev);
}

static int hclge_set_mac_state(struct hclge_dev *hdev, void *data,
			       size_t length)
{
	bool enable;
	int ret;

	if (length != sizeof(int))
		return -EINVAL;

	enable = !!*(int *)data;
	ret = hclge_cfg_mac_mode(hdev, enable);

	if (!ret && !hclge_comm_dev_phy_imp_supported(hdev->ae_dev))
		hclge_set_phy_state(hdev, enable);

	return ret;
}

static int hclge_set_led(struct hclge_dev *hdev, void *data,
			 size_t length)
{
	struct hclge_lamp_signal_cmd *para_cmd;
	struct hnae3_led_state_para *para;
	struct hclge_desc desc;
	int ret;

	if (length != sizeof(struct hnae3_led_state_para))
		return -EINVAL;

	hclge_cmd_setup_basic_desc(&desc, HCLGE_OPC_SET_LED, false);
	para = (struct hnae3_led_state_para *)data;
	para_cmd = (struct hclge_lamp_signal_cmd *)desc.data;
	para_cmd->type = cpu_to_le32(para->type);
	para_cmd->status = cpu_to_le32(para->status);

	ret = hclge_cmd_send(&hdev->hw, &desc, 1);
	if (ret)
		dev_err(&hdev->pdev->dev, "failed to set led, ret = %d\n", ret);

	return ret;
}

static int hclge_get_led_signal(struct hclge_dev *hdev, void *data,
				size_t length)
{
	struct hclge_lamp_signal_cmd *signal_cmd;
	struct hnae3_lamp_signal *signal;
	struct hclge_desc desc;
	int ret;

	if (length != sizeof(struct hnae3_lamp_signal))
		return -EINVAL;

	ret = hclge_get_info_from_cmd(hdev, &desc, 1, HCLGE_OPC_SET_LED);
	if (ret) {
		dev_err(&hdev->pdev->dev,
			"failed to get led signal, ret = %d\n", ret);
		return ret;
	}

	signal = (struct hnae3_lamp_signal *)data;
	signal_cmd = (struct hclge_lamp_signal_cmd *)desc.data;
	signal->error = signal_cmd->error;
	signal->locate = signal_cmd->locate;
	signal->activity = signal_cmd->activity;
	return 0;
}

static int hclge_def_phy_opt(struct mii_bus *mdio_bus, u32 phy_addr,
			     u16 reg_addr, u16 *data,
			     enum hclge_phy_op_code opt_type)
{
	int ret;

	if (opt_type == PHY_OP_READ) {
		ret = mdio_bus->read(mdio_bus, phy_addr, reg_addr);
		if (ret >= 0) {
			*data = (u16)ret;
			ret = 0;
		}
	} else {
		ret = mdio_bus->write(mdio_bus, phy_addr, reg_addr, *data);
	}
	return ret;
}

static int hclge_phy_reg_opt(struct hclge_dev *hdev,
			     struct hnae3_phy_para *para,
			     enum hclge_phy_op_code opt_type)
{
	struct mii_bus *mdio_bus = hdev->hw.mac.mdio_bus;
	u32 phy_addr = hdev->hw.mac.phy_addr;
	bool need_page_select = false;
	u16 cur_page;
	int ret;

	/* operate flow:
	 * 1 record current page addr
	 * 2 jump to operated page
	 * 3 operate register(read or write)
	 * 4 come back to the page recorded in the first step.
	 */
	mutex_lock(&mdio_bus->mdio_lock);

	/* check if page select is needed and record current page addr.
	 * no need to change page when read page 0
	 */
	if (opt_type != PHY_OP_READ || para->page != 0) {
		ret = mdio_bus->read(mdio_bus, phy_addr,
				     para->page_select_addr);
		if (ret < 0) {
			dev_err(&hdev->pdev->dev,
				"failed to read current phy %u reg page\n",
				phy_addr);
			mutex_unlock(&mdio_bus->mdio_lock);
			return ret;
		}
		cur_page = (u16)ret;
		need_page_select = cur_page != para->page;
	}

	/* jump to operated page */
	if (need_page_select) {
		ret = mdio_bus->write(mdio_bus, phy_addr,
				      para->page_select_addr, para->page);
		if (ret < 0) {
			mutex_unlock(&mdio_bus->mdio_lock);
			dev_err(&hdev->pdev->dev,
				"failed to change phy %u page %u to page %u\n",
				phy_addr, cur_page, para->page);
			return ret;
		}
	}

	/* operate register(read or write) */
	ret = hclge_def_phy_opt(mdio_bus, phy_addr, para->reg_addr, &para->data,
				opt_type);
	if (ret < 0)
		dev_err(&hdev->pdev->dev,
			"failed to %s phy %u page %u reg %u\n, ret = %d",
			opt_type == PHY_OP_READ ? "read" : "write",
			phy_addr, para->page, para->reg_addr, ret);

	/* come back to the page recorded in the first step. */
	if (need_page_select) {
		ret = mdio_bus->write(mdio_bus, phy_addr,
				      para->page_select_addr, cur_page);
		if (ret < 0)
			dev_err(&hdev->pdev->dev,
				"failed to restore phy %u reg page %u\n",
				phy_addr, cur_page);
	}

	mutex_unlock(&mdio_bus->mdio_lock);

	return ret;
}

static int hclge_8521_phy_ext_opt(struct mii_bus *mdio_bus, u32 phy_addr,
				  u16 reg_addr, u16 *data,
				  enum hclge_phy_op_code opt_type)
{
#define EXT_REG_ADDR 0x1e
#define EXT_DATA_ADDR 0x1f
	int ret;

	ret = mdio_bus->write(mdio_bus, phy_addr, EXT_REG_ADDR, reg_addr);
	if (ret < 0)
		return ret;

	return hclge_def_phy_opt(mdio_bus, phy_addr, EXT_DATA_ADDR, data,
				 opt_type);
}

static int hclge_8521_phy_mmd_opt(struct mii_bus *mdio_bus, u32 phy_addr,
				  u32 reg_addr, u16 *data,
				  enum hclge_phy_op_code opt_type)
{
#define MMD_REG_ADDR 0xd
#define MMD_DATA_ADDR 0xe
	u16 mmd_index;
	u16 mmd_reg;
	int ret;

	mmd_index = reg_addr >> 16U;
	mmd_reg = reg_addr & 0xFFFF;

	ret = mdio_bus->write(mdio_bus, phy_addr, MMD_REG_ADDR, mmd_index);
	if (ret < 0)
		return ret;
	ret = mdio_bus->write(mdio_bus, phy_addr, MMD_DATA_ADDR, mmd_reg);
	if (ret < 0)
		return ret;
	ret = mdio_bus->write(mdio_bus, phy_addr, MMD_REG_ADDR,
			      mmd_index | 0x4000);
	if (ret < 0)
		return ret;

	return hclge_def_phy_opt(mdio_bus, phy_addr, MMD_DATA_ADDR, data,
				 opt_type);
}

static void hclge_8521_phy_restores_to_utp_mii(struct hclge_dev *hdev,
					       struct mii_bus *mdio_bus,
					       u32 phy_addr)
{
	u16 phy_mii_region_val = 0x6;
	u16 utp_region_val = 0x0;
	int ret;

	ret = hclge_8521_phy_ext_opt(mdio_bus, phy_addr,
				     HCLGE_8521_PHY_SMI_SDS_ADDR,
				     &utp_region_val, PHY_OP_WRITE);
	if (ret)
		dev_err(&hdev->pdev->dev,
			"failed to choose phy space, ret = %d\n", ret);

	ret = hclge_8521_phy_ext_opt(mdio_bus, phy_addr,
				     HCLGE_8521_PHY_LDS_MII_ADDR,
				     &phy_mii_region_val, PHY_OP_WRITE);
	if (ret)
		dev_err(&hdev->pdev->dev,
			"failed to choose phy MII, ret = %d\n", ret);
}

static int hclge_8521_phy_utp_mii_opt(struct hnae3_phy_para *para,
				      struct mii_bus *mdio_bus, u32 phy_addr,
				      enum hclge_phy_op_code opt_type)
{
	u16 phy_mii_region_val = 0x6;
	u16 utp_region_val = 0x0;
	int ret;

	ret = hclge_8521_phy_ext_opt(mdio_bus, phy_addr,
				     HCLGE_8521_PHY_SMI_SDS_ADDR,
				     &utp_region_val, PHY_OP_WRITE);
	if (ret)
		return ret;

	ret = hclge_8521_phy_ext_opt(mdio_bus, phy_addr,
				     HCLGE_8521_PHY_LDS_MII_ADDR,
				     &phy_mii_region_val, PHY_OP_WRITE);
	if (ret)
		return ret;

	return hclge_def_phy_opt(mdio_bus, phy_addr, (u16)para->reg_addr,
				 &para->data, opt_type);
}

static int hclge_8521_phy_utp_mmd_opt(struct hnae3_phy_para *para,
				      struct mii_bus *mdio_bus, u32 phy_addr,
				      enum hclge_phy_op_code opt_type)
{
	u16 utp_region_val = 0x0;
	int ret;

	ret = hclge_8521_phy_ext_opt(mdio_bus, phy_addr,
				     HCLGE_8521_PHY_SMI_SDS_ADDR,
				     &utp_region_val, PHY_OP_WRITE);
	if (ret)
		return ret;

	return hclge_8521_phy_mmd_opt(mdio_bus, phy_addr, para->reg_addr,
				      &para->data, opt_type);
}

static int hclge_8521_phy_utp_lds_opt(struct hnae3_phy_para *para,
				      struct mii_bus *mdio_bus, u32 phy_addr,
				      enum hclge_phy_op_code opt_type)
{
	u16 lds_mii_region_val = 0x4;
	u16 utp_region_val = 0x0;
	int ret;

	ret = hclge_8521_phy_ext_opt(mdio_bus, phy_addr,
				     HCLGE_8521_PHY_SMI_SDS_ADDR,
				     &utp_region_val, PHY_OP_WRITE);
	if (ret)
		return ret;

	ret = hclge_8521_phy_ext_opt(mdio_bus, phy_addr,
				     HCLGE_8521_PHY_LDS_MII_ADDR,
				     &lds_mii_region_val, PHY_OP_WRITE);
	if (ret)
		return ret;

	return hclge_def_phy_opt(mdio_bus, phy_addr, (u16)para->reg_addr,
				 &para->data, opt_type);
}

static int hclge_8521_phy_utp_ext_opt(struct hnae3_phy_para *para,
				      struct mii_bus *mdio_bus, u32 phy_addr,
				      enum hclge_phy_op_code opt_type)
{
	u16 utp_region_val = 0x0;
	int ret;

	ret = hclge_8521_phy_ext_opt(mdio_bus, phy_addr,
				     HCLGE_8521_PHY_SMI_SDS_ADDR,
				     &utp_region_val, PHY_OP_WRITE);
	if (ret)
		return ret;

	return hclge_8521_phy_ext_opt(mdio_bus, phy_addr, (u16)para->reg_addr,
				      &para->data, opt_type);
}

static int hclge_8521_phy_sds_mii_opt(struct hnae3_phy_para *para,
				      struct mii_bus *mdio_bus, u32 phy_addr,
				      enum hclge_phy_op_code opt_type)
{
	u16 sds_region_val = 0x2;
	int ret;

	ret = hclge_8521_phy_ext_opt(mdio_bus, phy_addr,
				     HCLGE_8521_PHY_SMI_SDS_ADDR,
				     &sds_region_val, PHY_OP_WRITE);
	if (ret)
		return ret;

	return hclge_def_phy_opt(mdio_bus, phy_addr, (u16)para->reg_addr,
				 &para->data, opt_type);
}

static int hclge_8521_phy_sds_ext_opt(struct hnae3_phy_para *para,
				      struct mii_bus *mdio_bus, u32 phy_addr,
				      enum hclge_phy_op_code opt_type)
{
	u16 sds_region_val = 0x2;
	int ret;

	ret = hclge_8521_phy_ext_opt(mdio_bus, phy_addr,
				     HCLGE_8521_PHY_SMI_SDS_ADDR,
				     &sds_region_val, PHY_OP_WRITE);
	if (ret)
		return ret;

	return hclge_8521_phy_ext_opt(mdio_bus, phy_addr, (u16)para->reg_addr,
				      &para->data, opt_type);
}

static int hclge_8521_phy_opt(struct hclge_dev *hdev,
			      struct hnae3_phy_para *para,
			      enum hclge_phy_op_code opt_type)
{
	struct mii_bus *mdio_bus = hdev->hw.mac.mdio_bus;
	u32 phy_addr = hdev->hw.mac.phy_addr;
	int ret;

	mutex_lock(&mdio_bus->mdio_lock);
	switch (para->page) {
	case HCLGE_PHY_REGION_UTP_MII:
		ret = hclge_8521_phy_utp_mii_opt(para, mdio_bus,
						 phy_addr, opt_type);
		break;
	case HCLGE_PHY_REGION_UTP_MMD:
		ret = hclge_8521_phy_utp_mmd_opt(para, mdio_bus,
						 phy_addr, opt_type);
		break;
	case HCLGE_PHY_REGION_UTP_LDS:
		ret = hclge_8521_phy_utp_lds_opt(para, mdio_bus,
						 phy_addr, opt_type);
		break;
	case HCLGE_PHY_REGION_UTP_EXT:
		ret = hclge_8521_phy_utp_ext_opt(para, mdio_bus,
						 phy_addr, opt_type);
		break;
	case HCLGE_PHY_REGION_SDS_MII:
		ret = hclge_8521_phy_sds_mii_opt(para, mdio_bus,
						 phy_addr, opt_type);
		break;
	case HCLGE_PHY_REGION_SDS_EXT:
		ret = hclge_8521_phy_sds_ext_opt(para, mdio_bus,
						 phy_addr, opt_type);
		break;
	case HCLGE_PHY_REGION_COM_REG:
		ret = hclge_8521_phy_ext_opt(mdio_bus, phy_addr,
					     (u16)para->reg_addr,
					     &para->data, opt_type);
		break;
	default:
		dev_err(&hdev->pdev->dev, "invalid reg region: %d\n",
			para->page);
		mutex_unlock(&mdio_bus->mdio_lock);
		return -EINVAL;
	}

	if (ret)
		dev_err(&hdev->pdev->dev,
			"phy operation failed %d, reg_region: %d, data: 0x%x\n",
			ret, para->page, para->data);

	/* Set the region to UTP MII after operating the 8521 phy register */
	hclge_8521_phy_restores_to_utp_mii(hdev, mdio_bus, phy_addr);
	mutex_unlock(&mdio_bus->mdio_lock);
	return ret;
}

static int hclge_check_phy_opt_param(struct hclge_dev *hdev, void *data,
				     size_t length)
{
	struct hnae3_phy_para *para = (struct hnae3_phy_para *)data;
	struct hclge_mac *mac = &hdev->hw.mac;

	if (length != sizeof(*para))
		return -EINVAL;

	if (mac->media_type != HNAE3_MEDIA_TYPE_COPPER) {
		dev_err(&hdev->pdev->dev, "this is not a copper port");
		return -EOPNOTSUPP;
	}

	if (hnae3_dev_phy_imp_supported(hdev))
		return 0;

	if (!mac->phydev) {
		dev_err(&hdev->pdev->dev, "this net device has no phy");
		return -EINVAL;
	}

	if (!mac->mdio_bus) {
		dev_err(&hdev->pdev->dev, "this net device has no mdio bus");
		return -EINVAL;
	}

	return 0;
}

static int hclge_8211_phy_indirect_opt(struct hclge_dev *hdev,
				       struct hnae3_phy_para *para,
				       struct mii_bus *mdio_bus, u32 phy_addr,
				       enum hclge_phy_op_code opt_type)
{
	u32 indirect_reg_data;
	int ret;

	/* select indirect page 0xa43 */
	ret = mdio_bus->write(mdio_bus, phy_addr, para->page_select_addr,
			      HCLGE_8211_PHY_INDIRECT_PAGE);
	if (ret < 0) {
		dev_err(&hdev->pdev->dev,
			"failed to change phy %u indirect page 0xa43\n",
			phy_addr);
		return ret;
	}
	/* indirect access addr = page_no*16 + 2*(reg_no%16) */
	indirect_reg_data = (para->page << 4) + ((para->reg_addr % 16) << 1);
	ret = mdio_bus->write(mdio_bus, phy_addr, HCLGE_8211_PHY_INDIRECT_REG,
			      indirect_reg_data);
	if (ret < 0) {
		dev_err(&hdev->pdev->dev,
			"failed to write phy %u indirect reg\n", phy_addr);
		return ret;
	}

	ret = hclge_def_phy_opt(mdio_bus, phy_addr,
				HCLGE_8211_PHY_INDIRECT_DATA, &para->data,
				opt_type);
	if (ret < 0)
		dev_err(&hdev->pdev->dev,
			"failed to %s phy %u indirect data\n, ret = %d",
			opt_type == PHY_OP_READ ? "read" : "write",
			phy_addr, ret);

	return ret;
}

static int hclge_8211_phy_need_indirect_access(u16 page)
{
	if (page >= HCLGE_8211_PHY_INDIRECT_RANGE1_S &&
	    page <= HCLGE_8211_PHY_INDIRECT_RANGE1_E)
		return true;
	else if (page >= HCLGE_8211_PHY_INDIRECT_RANGE2_S &&
		 page <= HCLGE_8211_PHY_INDIRECT_RANGE2_E)
		return true;

	return false;
}

static int hclge_8211_phy_reg_opt(struct hclge_dev *hdev,
				  struct hnae3_phy_para *para,
				  enum hclge_phy_op_code opt_type)
{
	struct mii_bus *mdio_bus = hdev->hw.mac.mdio_bus;
	u32 phy_addr = hdev->hw.mac.phy_addr;
	u16 save_page;
	int ret;

	mutex_lock(&mdio_bus->mdio_lock);
	ret = mdio_bus->read(mdio_bus, phy_addr, para->page_select_addr);
	if (ret < 0) {
		dev_err(&hdev->pdev->dev,
			"failed to record phy %u reg page\n", phy_addr);
		mutex_unlock(&mdio_bus->mdio_lock);
		return ret;
	}
	save_page = ret;
	ret = hclge_8211_phy_indirect_opt(hdev, para, mdio_bus, phy_addr,
					  opt_type);
	if (ret)
		dev_err(&hdev->pdev->dev,
			"failed to indirect access 8211 phy %u\n", phy_addr);
	ret = mdio_bus->write(mdio_bus, phy_addr, para->page_select_addr,
			      save_page);
	if (ret < 0)
		dev_err(&hdev->pdev->dev,
			"failed to restore phy %u reg page %u\n",
			phy_addr, save_page);
	mutex_unlock(&mdio_bus->mdio_lock);

	return ret;
}

static int hclge_rw_8211_phy_reg(struct hclge_dev *hdev,
				 struct hnae3_phy_para *para,
				 enum hclge_phy_op_code opt_type)
{
	if (hclge_8211_phy_need_indirect_access(para->page))
		return hclge_8211_phy_reg_opt(hdev, para, opt_type);

	return hclge_phy_reg_opt(hdev, para, opt_type);
}

/* used when imp support phy drvier */
static int hclge_read_phy_reg_with_page(struct hclge_dev *hdev, u16 page,
					u16 reg_addr, u16 *val)
{
	struct hclge_phy_reg_cmd *req;
	struct hclge_desc desc;
	int ret;

	hclge_cmd_setup_basic_desc(&desc, HCLGE_OPC_PHY_REG, true);

	req = (struct hclge_phy_reg_cmd *)desc.data;
	req->reg_addr = cpu_to_le16(reg_addr);
	req->type = HCLGE_PHY_RW_WITH_PAGE;
	req->page = cpu_to_le16(page);

	ret = hclge_cmd_send(&hdev->hw, &desc, 1);
	if (ret) {
		dev_err(&hdev->pdev->dev,
			"failed to read phy page %u reg %u, ret = %d\n",
			page, reg_addr, ret);
		return ret;
	}

	*val = le16_to_cpu(req->reg_val);
	return 0;
}

/* used when imp support phy drvier */
static int hclge_write_phy_reg_with_page(struct hclge_dev *hdev, u16 page,
					 u16 reg_addr, u16 val)
{
	struct hclge_phy_reg_cmd *req;
	struct hclge_desc desc;
	int ret;

	hclge_cmd_setup_basic_desc(&desc, HCLGE_OPC_PHY_REG, false);

	req = (struct hclge_phy_reg_cmd *)desc.data;
	req->reg_addr = cpu_to_le16(reg_addr);
	req->type = HCLGE_PHY_RW_WITH_PAGE;
	req->page = cpu_to_le16(page);
	req->reg_val = cpu_to_le16(val);

	ret = hclge_cmd_send(&hdev->hw, &desc, 1);
	if (ret)
		dev_err(&hdev->pdev->dev,
			"failed to write phy page %u reg %u, ret = %d\n",
			page, reg_addr, ret);

	return ret;
}

static int hclge_rw_phy_reg_with_page(struct hclge_dev *hdev,
				      struct hnae3_phy_para *para,
				      enum hclge_phy_op_code opt_type)
{
	if (opt_type == PHY_OP_READ)
		return hclge_read_phy_reg_with_page(hdev, para->page,
						    para->reg_addr,
						    &para->data);

	return hclge_write_phy_reg_with_page(hdev, para->page, para->reg_addr,
					     para->data);
}

static int hclge_rw_phy_reg(struct hclge_dev *hdev, void *data,
			    size_t length, enum hclge_phy_op_code opt_type)
{
	struct hnae3_phy_para *para = (struct hnae3_phy_para *)data;
	struct hclge_mac *mac = &hdev->hw.mac;
	u32 phy_id;
	int ret;

	ret = hclge_check_phy_opt_param(hdev, data, length);
	if (ret < 0)
		return ret;

	if (hnae3_dev_phy_imp_supported(hdev))
		return hclge_rw_phy_reg_with_page(hdev, para, opt_type);

	phy_id = mac->phydev->phy_id & HCLGE_PHY_ID_MASK;
	switch (phy_id) {
	case HCLGE_PHY_ID_FOR_RTL8211:
		return hclge_rw_8211_phy_reg(hdev, para, opt_type);
	case HCLGE_PHY_ID_FOR_YT8521:
		return hclge_8521_phy_opt(hdev, para, opt_type);
	case HCLGE_PHY_ID_FOR_MVL1512:
	default:
		return hclge_phy_reg_opt(hdev, para, opt_type);
	}
}

static int hclge_get_phy_reg(struct hclge_dev *hdev, void *data, size_t length)
{
	return hclge_rw_phy_reg(hdev, data, length, PHY_OP_READ);
}

static int hclge_set_phy_reg(struct hclge_dev *hdev, void *data, size_t length)
{
	return hclge_rw_phy_reg(hdev, data, length, PHY_OP_WRITE);
}

static void hclge_ext_resotre_config(struct hclge_dev *hdev)
{
	if (hdev->reset_type != HNAE3_IMP_RESET &&
	    hdev->reset_type != HNAE3_GLOBAL_RESET)
		return;

	if (hdev->notify_param.init)
		hclge_set_notify_packet_para(hdev, &hdev->notify_param,
					     sizeof(hdev->notify_param));

	hclge_set_torus_param(hdev, &hdev->torus_param,
			      sizeof(hdev->torus_param));
}

static int hclge_set_reset_task(struct hclge_dev *hdev, void *data,
				size_t length)
{
	u32 *reset_level = (u32 *)data;

	if (length != sizeof(u32))
		return -EINVAL;

	dev_warn(&hdev->pdev->dev, "reset level is %u\n", *reset_level);

	/* request reset & schedule reset task */
	set_bit(*reset_level, &hdev->reset_request);
	hclge_reset_task_schedule(hdev);
	return 0;
}

int hclge_ext_call_event(struct hclge_dev *hdev,
			 enum hnae3_event_type_custom event_t)
{
	if (event_t >= HNAE3_INVALID_EVENT_CUSTOM)
		return -EINVAL;

	mutex_lock(&hclge_nic_event_lock);
	if (!nic_event_call) {
		mutex_unlock(&hclge_nic_event_lock);
		return -EOPNOTSUPP;
	}

	nic_event_call(hdev->vport[0].nic.netdev, event_t);
	mutex_unlock(&hclge_nic_event_lock);
	return 0;
}

int nic_register_event(nic_event_fn_t event_call)
{
	if (!event_call) {
		pr_err("hns3: register event handle is null\n");
		return -EINVAL;
	}

	mutex_lock(&hclge_nic_event_lock);
	if (nic_event_call) {
		mutex_unlock(&hclge_nic_event_lock);
		pr_err("hns3: event already register\n");
		return -EBUSY;
	}

	nic_event_call = event_call;

	mutex_unlock(&hclge_nic_event_lock);
	pr_info("hns3: event register success\n");
	return 0;
}
EXPORT_SYMBOL(nic_register_event);

int nic_unregister_event(void)
{
	mutex_lock(&hclge_nic_event_lock);
	nic_event_call = NULL;

	mutex_unlock(&hclge_nic_event_lock);
	pr_info("hns3: event unregister success\n");
	return 0;
}
EXPORT_SYMBOL(nic_unregister_event);

static int hclge_nic_call_event(struct hclge_dev *hdev, void *data,
				size_t length)
{
#define ERROR_EVENT_TYPE_NUM 4

	u32 event_type[ERROR_EVENT_TYPE_NUM] = {
		HNAE3_PPU_POISON_CUSTOM,
		HNAE3_IMP_RESET_CUSTOM,
		HNAE3_IMP_RD_POISON_CUSTOM,
		HNAE3_ROCEE_AXI_RESP_CUSTOM,
	};
	u32 *index = (u32 *)data;

	if (length != sizeof(u32))
		return -EINVAL;

	if ((*index) >= ERROR_EVENT_TYPE_NUM)
		return 0;

	return hclge_ext_call_event(hdev, event_type[*index]);
}

static enum hnae3_event_type_custom
hclge_get_reset_fail_type(enum hnae3_reset_type reset_type)
{
	const struct hclge_reset_fail_type_map fail_type_map[] = {
		{HNAE3_FUNC_RESET, HNAE3_FUNC_RESET_FAIL_CUSTOM},
		{HNAE3_GLOBAL_RESET, HNAE3_GLOBAL_RESET_FAIL_CUSTOM},
		{HNAE3_IMP_RESET, HNAE3_IMP_RESET_FAIL_CUSTOM},
	};
	u32 i;

	for (i = 0; i < ARRAY_SIZE(fail_type_map); i++)
		if (fail_type_map[i].reset_type == reset_type)
			return fail_type_map[i].custom_type;

	return HNAE3_INVALID_EVENT_CUSTOM;
}

static void hclge_report_reset_fail_custom(struct hclge_dev *hdev)
{
#define HCLGE_RESET_MAX_FAIL_CNT_CUSTOM 1

	u32 max_fail_custom_cnt = HCLGE_RESET_MAX_FAIL_CNT;

	mutex_lock(&hclge_nic_event_lock);
	if (nic_event_call)
		max_fail_custom_cnt = HCLGE_RESET_MAX_FAIL_CNT_CUSTOM;
	mutex_unlock(&hclge_nic_event_lock);

	if (hdev->rst_stats.reset_fail_cnt < max_fail_custom_cnt)
		return;

	dev_err(&hdev->pdev->dev, "failed to report reset!\n");
	hclge_ext_call_event(hdev, hclge_get_reset_fail_type(hdev->reset_type));
}

void hclge_ext_reset_end(struct hclge_dev *hdev, bool done)
{
	if (!done) {
		hclge_report_reset_fail_custom(hdev);
		return;
	}

	hclge_ext_resotre_config(hdev);
	hclge_ext_call_event(hdev, HNAE3_RESET_DONE_CUSTOM);
	dev_info(&hdev->pdev->dev, "report reset done!\n");
}

static const hclge_priv_ops_fn hclge_ext_func_arr[] = {
	[HNAE3_EXT_OPC_RESET] = hclge_set_reset_task,
	[HNAE3_EXT_OPC_EVENT_CALLBACK] = hclge_nic_call_event,
	[HNAE3_EXT_OPC_GET_PFC_STORM_PARA] = hclge_get_pfc_storm_para,
	[HNAE3_EXT_OPC_SET_PFC_STORM_PARA] = hclge_set_pfc_storm_para,
	[HNAE3_EXT_OPC_SET_NOTIFY_PARAM] = hclge_set_notify_packet_para,
	[HNAE3_EXT_OPC_SET_NOTIFY_START] = hclge_set_notify_packet_start,
	[HNAE3_EXT_OPC_SET_TORUS_PARAM] = hclge_set_torus_param,
	[HNAE3_EXT_OPC_GET_TORUS_PARAM] = hclge_get_torus_param,
	[HNAE3_EXT_OPC_CLEAN_STATS64] = hclge_clean_stats64,
	[HNAE3_EXT_OPC_GET_PORT_EXT_ID_INFO] = hclge_get_extend_port_id_info,
	[HNAE3_EXT_OPC_GET_PORT_EXT_NUM_INFO] = hclge_get_extend_port_num_info,
	[HNAE3_EXT_OPC_GET_PORT_NUM] = hclge_get_port_num,
	[HNAE3_EXT_OPC_GET_PRESENT] = hclge_get_sfp_present,
	[HNAE3_EXT_OPC_SET_SFP_STATE] = hclge_set_sfp_state,
	[HNAE3_EXT_OPC_DISABLE_LANE] = hclge_disable_net_lane,
	[HNAE3_EXT_OPC_GET_LANE_STATUS] = hclge_get_net_lane_status,
	[HNAE3_EXT_OPC_DISABLE_CLOCK] = hclge_disable_nic_clock,
	[HNAE3_EXT_OPC_SET_PFC_TIME] = hclge_set_pause_trans_time,
	[HNAE3_EXT_OPC_GET_HILINK_REF_LOS] = hclge_get_hilink_ref_los,
	[HNAE3_EXT_OPC_GET_PORT_FAULT_STATUS] = hclge_get_port_fault_status,
	[HNAE3_EXT_OPC_GET_PORT_TYPE] = hclge_get_port_wire_type,
	[HNAE3_EXT_OPC_SET_MAC_STATE] = hclge_set_mac_state,
	[HNAE3_EXT_OPC_SET_LED] = hclge_set_led,
	[HNAE3_EXT_OPC_GET_LED_SIGNAL] = hclge_get_led_signal,
	[HNAE3_EXT_OPC_GET_PHY_REG] = hclge_get_phy_reg,
	[HNAE3_EXT_OPC_SET_PHY_REG] = hclge_set_phy_reg,
};

int hclge_ext_ops_handle(struct hnae3_handle *handle, int opcode,
			 void *data, size_t length)
{
	struct hclge_vport *vport = hclge_get_vport(handle);
	int cmd_num = ARRAY_SIZE(hclge_ext_func_arr);
	struct hclge_dev *hdev = vport->back;
	hclge_priv_ops_fn ext_opcode_func;

	if (opcode >= cmd_num) {
		dev_err(&hdev->pdev->dev, "invalid opcode %d\n", opcode);
		return -EINVAL;
	}

	ext_opcode_func = hclge_ext_func_arr[opcode];
	if (!ext_opcode_func) {
		dev_err(&hdev->pdev->dev, "unsupported opcode %d\n", opcode);
		return -EOPNOTSUPP;
	}

	return ext_opcode_func(hdev, data, length);
}

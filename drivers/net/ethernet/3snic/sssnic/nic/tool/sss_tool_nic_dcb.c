// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [TOOL]" fmt

#include "sss_nic_cfg.h"
#include "sss_nic_dcb.h"
#include "sss_tool_comm.h"
#include "sss_tool_nic.h"
#include "sss_nic_rx_init.h"
#include "sss_nic_netdev_ops_api.h"

#define SSS_TOOL_DBG_DFLT_DSCP_VAL  0xFF

static int sss_tool_update_pcp_cfg(struct sss_nic_dev *nic_dev,
				   const struct sss_tool_qos_dev_cfg *qos_cfg)
{
	u8 valid_cos_bitmap = 0;
	u8 cos_num = 0;
	int i;

	if (!(qos_cfg->cfg_bitmap & SSS_TOOL_MSG_QOS_DEV_PCP2COS))
		return 0;

	for (i = 0; i < SSSNIC_DCB_UP_MAX; i++) {
		if (!(nic_dev->dft_func_cos_bitmap & BIT(qos_cfg->pcp2cos[i]))) {
			tool_err("Invalid pcp cos:%u, func cos valid map is %u",
				 qos_cfg->pcp2cos[i], nic_dev->dft_func_cos_bitmap);
			return -EINVAL;
		}

		if ((BIT(qos_cfg->pcp2cos[i]) & valid_cos_bitmap) == 0) {
			cos_num++;
			valid_cos_bitmap |= (u8)BIT(qos_cfg->pcp2cos[i]);
		}
	}

	nic_dev->backup_dcb_cfg.pcp_valid_cos_map = valid_cos_bitmap;
	nic_dev->backup_dcb_cfg.pcp_user_cos_num = cos_num;
	memcpy(nic_dev->backup_dcb_cfg.pcp2cos, qos_cfg->pcp2cos, sizeof(qos_cfg->pcp2cos));

	return 0;
}

static int sss_tool_update_dscp_cfg(struct sss_nic_dev *nic_dev,
				    const struct sss_tool_qos_dev_cfg *qos_cfg)
{
	u8 valid_cos_bitmap = 0;
	u8 cos_num = 0;
	u8 cos;
	int i;

	if (!(qos_cfg->cfg_bitmap & SSS_TOOL_MSG_QOS_DEV_DSCP2COS))
		return 0;

	for (i = 0; i < SSSNIC_DCB_IP_PRI_MAX; i++) {
		if (qos_cfg->dscp2cos[i] != SSS_TOOL_DBG_DFLT_DSCP_VAL)
			cos = qos_cfg->dscp2cos[i];
		else
			cos = nic_dev->backup_dcb_cfg.dscp2cos[i];

		if (cos >= SSSNIC_DCB_UP_MAX || !(nic_dev->dft_func_cos_bitmap & BIT(cos))) {
			tool_err("Invalid dscp cos:%u, func cos valid map is %u",
				 cos, nic_dev->dft_func_cos_bitmap);
			return -EINVAL;
		}

		if ((BIT(cos) & valid_cos_bitmap) == 0) {
			cos_num++;
			valid_cos_bitmap |= (u8)BIT(cos);
		}
	}

	for (i = 0; i < SSSNIC_DCB_IP_PRI_MAX; i++) {
		if (qos_cfg->dscp2cos[i] != SSS_TOOL_DBG_DFLT_DSCP_VAL)
			nic_dev->backup_dcb_cfg.dscp2cos[i] = qos_cfg->dscp2cos[i];
		else
			nic_dev->backup_dcb_cfg.dscp2cos[i] = nic_dev->hw_dcb_cfg.dscp2cos[i];
	}

	nic_dev->backup_dcb_cfg.dscp_valid_cos_map = valid_cos_bitmap;
	nic_dev->backup_dcb_cfg.dscp_user_cos_num = cos_num;

	return 0;
}

static int sss_tool_update_pcp_dscp_cfg(struct sss_nic_dev *nic_dev,
					const struct sss_tool_qos_dev_cfg *qos_cfg)
{
	int ret;

	ret = sss_tool_update_pcp_cfg(nic_dev, qos_cfg);
	if (ret != 0) {
		tool_err("Fail to update pcp cfg\n");
		return ret;
	}

	ret = sss_tool_update_dscp_cfg(nic_dev, qos_cfg);
	if (ret != 0)
		tool_err("Fail to update dscp cfg\n");

	return ret;
}

static int sss_tool_update_wanted_qos_cfg(struct sss_nic_dev *nic_dev,
					  const void *in_buf)
{
	const struct sss_tool_qos_dev_cfg *qos_cfg = in_buf;
	u8 valid_cos_bitmap;
	u8 cos_num;
	int ret;

	if (qos_cfg->cfg_bitmap & SSS_TOOL_MSG_QOS_DEV_TRUST) {
		if (qos_cfg->trust > DCB_DSCP) {
			tool_err("Invalid trust:%u of qos cfg\n", qos_cfg->trust);
			return -EINVAL;
		}

		nic_dev->backup_dcb_cfg.trust = qos_cfg->trust;
	}

	if (qos_cfg->cfg_bitmap & SSS_TOOL_MSG_QOS_DEV_DFT_COS) {
		if (!(BIT(qos_cfg->dft_cos) & nic_dev->dft_func_cos_bitmap)) {
			tool_err("Invalid default cos:%u of qos cfg\n", qos_cfg->dft_cos);
			return -EINVAL;
		}

		nic_dev->backup_dcb_cfg.default_cos = qos_cfg->dft_cos;
	}

	ret = sss_tool_update_pcp_dscp_cfg(nic_dev, qos_cfg);
	if (ret != 0)
		return ret;

	if (nic_dev->backup_dcb_cfg.trust != DCB_PCP) {
		valid_cos_bitmap = nic_dev->backup_dcb_cfg.dscp_valid_cos_map;
		cos_num = nic_dev->backup_dcb_cfg.dscp_user_cos_num;
	} else {
		valid_cos_bitmap = nic_dev->backup_dcb_cfg.pcp_valid_cos_map;
		cos_num = nic_dev->backup_dcb_cfg.pcp_user_cos_num;
	}

	if (SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_DCB_ENABLE)) {
		if (cos_num > nic_dev->qp_res.qp_num) {
			tool_err("Invalid cos num, DCB is on, cos num:%d need less than channel num:%u\n",
				 cos_num, nic_dev->qp_res.qp_num);
			return -EOPNOTSUPP;
		}
	}

	if (!(BIT(nic_dev->backup_dcb_cfg.default_cos) & valid_cos_bitmap)) {
		tool_info("Success to update cos %u to %u\n",
			  nic_dev->backup_dcb_cfg.default_cos, (u8)fls(valid_cos_bitmap) - 1);
		nic_dev->backup_dcb_cfg.default_cos = (u8)fls(valid_cos_bitmap) - 1;
	}

	return 0;
}

static int sss_tool_set_tx_cos_state(struct sss_nic_dev *nic_dev, u8 dcb_en)
{
	int ret;
	u8 i;
	struct sss_nic_dcb_info dcb_info = {0};
	struct sss_nic_dcb_config *dcb_cfg = &nic_dev->hw_dcb_cfg;

	dcb_info.trust = dcb_cfg->trust;
	dcb_info.default_cos = dcb_cfg->default_cos;
	dcb_info.dcb_on = dcb_en;

	if (!dcb_en) {
		memset(dcb_info.dscp2cos, dcb_cfg->default_cos, sizeof(dcb_info.dscp2cos));
		memset(dcb_info.pcp2cos, dcb_cfg->default_cos, sizeof(dcb_info.pcp2cos));

	} else {
		for (i = 0; i < SSSNIC_DCB_IP_PRI_MAX; i++)
			dcb_info.dscp2cos[i] = dcb_cfg->dscp2cos[i];
		for (i = 0; i < SSSNIC_DCB_COS_MAX; i++)
			dcb_info.pcp2cos[i] = dcb_cfg->pcp2cos[i];
	}

	ret = sss_nic_set_dcb_info(nic_dev->nic_io, &dcb_info);
	if (ret != 0)
		tool_err("Fail to set dcb state\n");

	return ret;
}

static int sss_tool_configure_dcb_hw(struct sss_nic_dev *nic_dev, u8 dcb_en)
{
	int ret;
	u8 user_cos_num = sss_nic_get_user_cos_num(nic_dev);

	ret = sss_nic_set_hw_dcb_state(nic_dev, 1, dcb_en);
	if (ret != 0) {
		tool_err("Fail to set dcb state\n");
		return ret;
	}

	sss_nic_update_qp_cos_map(nic_dev, user_cos_num);
	sss_nic_update_sq_cos(nic_dev, dcb_en);

	if (SSSNIC_FUNC_IS_VF(nic_dev->hwdev)) {
		/* VF does not support DCB, use the default cos */
		nic_dev->hw_dcb_cfg.default_cos = (u8)fls(nic_dev->dft_func_cos_bitmap) - 1;

		return 0;
	}

	ret = sss_tool_set_tx_cos_state(nic_dev, dcb_en);
	if (ret != 0) {
		tool_err("Fail to set tx cos state\n");
		goto set_tx_cos_fail;
	}

	ret = sss_nic_update_rx_rss(nic_dev);
	if (ret != 0) {
		tool_err("Fail to configure rx\n");
		goto update_rx_rss_fail;
	}

	if (!dcb_en)
		SSSNIC_CLEAR_NIC_DEV_FLAG(nic_dev, SSSNIC_DCB_ENABLE);
	else
		SSSNIC_SET_NIC_DEV_FLAG(nic_dev, SSSNIC_DCB_ENABLE);

	return 0;
update_rx_rss_fail:
	sss_tool_set_tx_cos_state(nic_dev, dcb_en ? 0 : 1);

set_tx_cos_fail:
	sss_nic_update_sq_cos(nic_dev, dcb_en ? 0 : 1);
	sss_nic_set_hw_dcb_state(nic_dev->hwdev, 1, dcb_en ? 0 : 1);

	return ret;
}

static int sss_tool_setup_cos(struct net_device *netdev, u8 cos)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	if (cos > nic_dev->max_cos_num) {
		tool_err("Invalid num_tc: %u more then max cos: %u\n", cos, nic_dev->max_cos_num);
		return -EINVAL;
	}

	if (cos && SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_SAME_RXTX)) {
		tool_err("Fail to enable DCB while Symmetric RSS is enabled\n");
		return -EOPNOTSUPP;
	}

	return sss_tool_configure_dcb_hw(nic_dev, cos ? 1 : 0);
}

static void sss_tool_change_qos_cfg(struct sss_nic_dev *nic_dev,
				    const struct sss_nic_dcb_config *dcb_cfg)
{
	u8 user_cos_num = sss_nic_get_user_cos_num(nic_dev);

	sss_nic_sync_dcb_cfg(nic_dev, dcb_cfg);
	sss_nic_update_qp_cos_map(nic_dev, user_cos_num);

	clear_bit(SSSNIC_DCB_UP_COS_SETTING, &nic_dev->dcb_flags);
}

static int sss_tool_dcbcfg_set_up_bitmap(struct sss_nic_dev *nic_dev)
{
	int ret;
	u8 user_cos_num = sss_nic_get_user_cos_num(nic_dev);
	struct sss_nic_dcb_config old_dcb_cfg;
	bool netif_run = false;

	memcpy(&old_dcb_cfg, &nic_dev->hw_dcb_cfg, sizeof(struct sss_nic_dcb_config));

	if (!memcmp(&nic_dev->backup_dcb_cfg, &old_dcb_cfg, sizeof(struct sss_nic_dcb_config))) {
		tool_info("Valid up bitmap is the same, nothing has to change\n");
		return 0;
	}

	rtnl_lock();
	if (netif_running(nic_dev->netdev)) {
		sss_nic_vport_down(nic_dev);
		netif_run = true;
	}

	if (test_and_set_bit(SSSNIC_DCB_UP_COS_SETTING, &nic_dev->dcb_flags)) {
		tool_warn("Cos up map setup in inprocess, please try again later\n");
		ret = -EFAULT;
		goto set_qos_cfg_fail;
	}

	sss_tool_change_qos_cfg(nic_dev, &nic_dev->backup_dcb_cfg);

	if (SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_DCB_ENABLE)) {
		ret = sss_tool_setup_cos(nic_dev->netdev, user_cos_num);
		if (ret != 0)
			goto setup_cos_fail;
	}

	if (netif_run) {
		ret = sss_nic_vport_up(nic_dev);
		if (ret != 0)
			goto vport_up_fail;
	}

	rtnl_unlock();

	return 0;

vport_up_fail:
	if (SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_DCB_ENABLE))
		sss_tool_setup_cos(nic_dev->netdev, user_cos_num ? 0 : user_cos_num);

setup_cos_fail:
	sss_tool_change_qos_cfg(nic_dev, &old_dcb_cfg);

set_qos_cfg_fail:
	if (netif_run)
		sss_nic_vport_up(nic_dev);

	rtnl_unlock();

	return ret;
}

int sss_tool_dcb_mt_qos_map(struct sss_nic_dev *nic_dev, const void *in_buf,
			    u32 in_len, void *out_buf, u32 *out_len)
{
	int ret;
	u8 i;
	struct sss_tool_qos_dev_cfg *qos_out = out_buf;

	if (!out_buf || !out_len || !in_buf) {
		tool_err("Invalid param, use null pointer\n");
		return -EFAULT;
	}

	if (in_len != sizeof(*qos_out) || *out_len != sizeof(*qos_out)) {
		tool_err("Invalid in len: %u or outlen: %u is not equal to %lu\n",
			 in_len, *out_len, sizeof(*qos_out));
		return -EINVAL;
	}

	memcpy(qos_out, in_buf, sizeof(*qos_out));
	qos_out->head.status = 0;
	if (qos_out->op_code & SSS_TOOL_DCB_OPCODE_WR) {
		memcpy(&nic_dev->backup_dcb_cfg, &nic_dev->hw_dcb_cfg,
		       sizeof(struct sss_nic_dcb_config));
		ret = sss_tool_update_wanted_qos_cfg(nic_dev, in_buf);
		if (ret != 0) {
			qos_out->head.status = SSS_TOOL_EINVAL;
			return 0;
		}

		ret = sss_tool_dcbcfg_set_up_bitmap(nic_dev);
		if (ret != 0)
			qos_out->head.status = SSS_TOOL_EIO;
	} else {
		for (i = 0; i < SSSNIC_DCB_IP_PRI_MAX; i++)
			qos_out->dscp2cos[i] = nic_dev->hw_dcb_cfg.dscp2cos[i];
		for (i = 0; i < SSSNIC_DCB_UP_MAX; i++)
			qos_out->pcp2cos[i] = nic_dev->hw_dcb_cfg.pcp2cos[i];
		qos_out->trust = nic_dev->hw_dcb_cfg.trust;
		qos_out->dft_cos = nic_dev->hw_dcb_cfg.default_cos;
	}

	return 0;
}

int sss_tool_dcb_mt_dcb_state(struct sss_nic_dev *nic_dev, const void *in_buf,
			      u32 in_len, void *out_buf, u32 *out_len)
{
	int ret;
	u8 user_cos_num = sss_nic_get_user_cos_num(nic_dev);
	struct sss_tool_dcb_state *dcb_out = out_buf;
	const struct sss_tool_dcb_state *dcb_in = in_buf;

	if (!in_buf || !out_buf || !out_len) {
		tool_err("Invalid param, use null pointer\n");
		return -EFAULT;
	}

	if (in_len != sizeof(*dcb_in) || *out_len != sizeof(*dcb_out)) {
		tool_err("Invalid in len: %u or out len: %u is not equal to %lu\n",
			 in_len, *out_len, sizeof(*dcb_in));
		return -EINVAL;
	}

	memcpy(dcb_out, dcb_in, sizeof(*dcb_in));
	dcb_out->head.status = 0;

	if (!(dcb_in->op_code & SSS_TOOL_DCB_OPCODE_WR)) {
		dcb_out->state = !!SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_DCB_ENABLE);
		return 0;
	}

	if (SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_DCB_ENABLE) == dcb_in->state)
		return 0;

	if (dcb_in->state && user_cos_num > nic_dev->qp_res.qp_num) {
		tool_err("Fail to mt dcb state, cos num %u larger than channel num %u\n",
			 user_cos_num, nic_dev->qp_res.qp_num);
		return -EOPNOTSUPP;
	}

	rtnl_lock();
	if (netif_running(nic_dev->netdev)) {
		sss_nic_vport_down(nic_dev);
		ret = sss_tool_setup_cos(nic_dev->netdev, dcb_in->state ? user_cos_num : 0);
		if (ret != 0) {
			sss_nic_vport_up(nic_dev);
			rtnl_unlock();
			return ret;
		}

		ret = sss_nic_vport_up(nic_dev);
		if (ret != 0) {
			sss_tool_setup_cos(nic_dev->netdev, dcb_in->state ? 0 : user_cos_num);
			sss_nic_vport_up(nic_dev);
		}

		rtnl_unlock();
		return ret;
	}

	ret = sss_tool_setup_cos(nic_dev->netdev, dcb_in->state ? user_cos_num : 0);
	rtnl_unlock();

	return ret;
}

int sss_tool_dcb_mt_hw_qos_get(struct sss_nic_dev *nic_dev, const void *in_buf,
			       u32 in_len, void *out_buf, u32 *out_len)
{
	struct sss_tool_qos_cos_cfg *out_cfg = out_buf;
	const struct sss_tool_qos_cos_cfg *in_cfg = in_buf;

	if (!in_buf || !out_buf || !out_len) {
		tool_err("Invalid param, use null pointer\n");
		return -EFAULT;
	}

	if (in_len != sizeof(*in_cfg) || *out_len != sizeof(*out_cfg)) {
		tool_err("Invalid in len: %u or out len: %u is not equal to %lu\n",
			 in_len, *out_len, sizeof(*in_cfg));
		return -EINVAL;
	}

	memcpy(out_cfg, in_cfg, sizeof(*in_cfg));
	out_cfg->func_max_cos_num = nic_dev->max_cos_num;
	out_cfg->head.status = 0;
	out_cfg->port_cos_bitmap = (u8)nic_dev->dft_port_cos_bitmap;
	out_cfg->func_cos_bitmap = (u8)nic_dev->dft_func_cos_bitmap;
	out_cfg->port_id = sss_get_phy_port_id(nic_dev->hwdev);

	return 0;
}

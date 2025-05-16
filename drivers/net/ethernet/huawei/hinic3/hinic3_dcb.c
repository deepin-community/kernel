// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>

#include "hinic3_crm.h"
#include "hinic3_lld.h"
#include "hinic3_nic_cfg.h"
#include "hinic3_srv_nic.h"
#include "hinic3_nic_dev.h"
#include "hinic3_dcb.h"

#define MAX_BW_PERCENT		100

u8 hinic3_get_dev_user_cos_num(struct hinic3_nic_dev *nic_dev)
{
	struct hinic3_dcb *dcb = nic_dev->dcb;

	if (dcb->hw_dcb_cfg.trust == HINIC3_DCB_PCP)
		return dcb->hw_dcb_cfg.pcp_user_cos_num;
	if (dcb->hw_dcb_cfg.trust == HINIC3_DCB_DSCP)
		return dcb->hw_dcb_cfg.dscp_user_cos_num;
	return 0;
}

u8 hinic3_get_dev_valid_cos_map(struct hinic3_nic_dev *nic_dev)
{
	struct hinic3_dcb *dcb = nic_dev->dcb;

	if (dcb->hw_dcb_cfg.trust == HINIC3_DCB_PCP)
		return dcb->hw_dcb_cfg.pcp_valid_cos_map;
	if (dcb->hw_dcb_cfg.trust == HINIC3_DCB_DSCP)
		return dcb->hw_dcb_cfg.dscp_valid_cos_map;
	return 0;
}

void hinic3_update_qp_cos_cfg(struct hinic3_nic_dev *nic_dev, u8 num_cos)
{
	struct hinic3_dcb_config *hw_dcb_cfg = &nic_dev->dcb->hw_dcb_cfg;
	struct hinic3_dcb_config *wanted_dcb_cfg =
		&nic_dev->dcb->wanted_dcb_cfg;
	u8 valid_cos_map = hinic3_get_dev_valid_cos_map(nic_dev);
	u8 cos_qp_num, cos_qp_offset = 0;
	u8 i, remainder, num_qp_per_cos;

	if (num_cos == 0 || nic_dev->q_params.num_qps == 0)
		return;

	num_qp_per_cos = (u8)(nic_dev->q_params.num_qps / num_cos);
	remainder = nic_dev->q_params.num_qps % num_cos;

	memset(hw_dcb_cfg->cos_qp_offset, 0, sizeof(hw_dcb_cfg->cos_qp_offset));
	memset(hw_dcb_cfg->cos_qp_num, 0, sizeof(hw_dcb_cfg->cos_qp_num));

	for (i = 0; i < PCP_MAX_UP; i++) {
		if (BIT(i) & valid_cos_map) {
			cos_qp_num = num_qp_per_cos + ((remainder > 0) ?
				 (remainder--, 1) : 0);

			hw_dcb_cfg->cos_qp_offset[i] = cos_qp_offset;
			hw_dcb_cfg->cos_qp_num[i] = cos_qp_num;
			hinic3_info(nic_dev, drv, "cos %u, cos_qp_offset=%u cos_qp_num=%u\n",
				    i, cos_qp_offset, cos_qp_num);

			cos_qp_offset += cos_qp_num;
			valid_cos_map -= (u8)BIT(i);
		}
	}

	memcpy(wanted_dcb_cfg->cos_qp_offset, hw_dcb_cfg->cos_qp_offset,
	       sizeof(hw_dcb_cfg->cos_qp_offset));
	memcpy(wanted_dcb_cfg->cos_qp_num, hw_dcb_cfg->cos_qp_num,
	       sizeof(hw_dcb_cfg->cos_qp_num));
}

void hinic3_update_tx_db_cos(struct hinic3_nic_dev *nic_dev, u8 dcb_en)
{
	struct hinic3_dcb_config *hw_dcb_cfg = &nic_dev->dcb->hw_dcb_cfg;
	u8 i;
	u16 start_qid, q_num;

	hinic3_set_txq_cos(nic_dev, 0, nic_dev->q_params.num_qps,
			   hw_dcb_cfg->default_cos);
	if (!dcb_en)
		return;

	for (i = 0; i < NIC_DCB_COS_MAX; i++) {
		q_num = (u16)hw_dcb_cfg->cos_qp_num[i];
		if (q_num) {
			start_qid = (u16)hw_dcb_cfg->cos_qp_offset[i];

			hinic3_set_txq_cos(nic_dev, start_qid, q_num, i);
			hinic3_info(nic_dev, drv, "update tx db cos, start_qid %u, q_num=%u cos=%u\n",
				    start_qid, q_num, i);
		}
	}
}

static int hinic3_set_tx_cos_state(struct hinic3_nic_dev *nic_dev, u8 dcb_en)
{
	struct hinic3_dcb *dcb = nic_dev->dcb;
	struct hinic3_dcb_config *hw_dcb_cfg = &dcb->hw_dcb_cfg;
	struct hinic3_dcb_state dcb_state = {0};
	u8 i;
	int err;
	u32 pcp2cos_size = sizeof(dcb_state.pcp2cos);
	u32 dscp2cos_size = sizeof(dcb_state.dscp2cos);

	dcb_state.dcb_on = dcb_en;
	dcb_state.default_cos = hw_dcb_cfg->default_cos;
	dcb_state.trust = hw_dcb_cfg->trust;

	if (dcb_en) {
		for (i = 0; i < NIC_DCB_COS_MAX; i++)
			dcb_state.pcp2cos[i] = hw_dcb_cfg->pcp2cos[i];
		for (i = 0; i < NIC_DCB_IP_PRI_MAX; i++)
			dcb_state.dscp2cos[i] = hw_dcb_cfg->dscp2cos[i];
	} else {
		memset(dcb_state.pcp2cos, hw_dcb_cfg->default_cos,
		       pcp2cos_size);
		memset(dcb_state.dscp2cos, hw_dcb_cfg->default_cos,
		       dscp2cos_size);
	}

	err = hinic3_set_dcb_state(nic_dev->hwdev, &dcb_state);
	if (err)
		hinic3_err(nic_dev, drv, "Failed to set dcb state\n");

	return err;
}

int hinic3_configure_dcb_hw(struct hinic3_nic_dev *nic_dev, u8 dcb_en)
{
	int err;
	u8 user_cos_num = hinic3_get_dev_user_cos_num(nic_dev);

	err = hinic3_sync_dcb_state(nic_dev->hwdev, 1, dcb_en);
	if (err) {
		hinic3_err(nic_dev, drv, "Set dcb state failed\n");
		return err;
	}

	hinic3_update_qp_cos_cfg(nic_dev, user_cos_num);
	hinic3_update_tx_db_cos(nic_dev, dcb_en);

	err = hinic3_set_tx_cos_state(nic_dev, dcb_en);
	if (err) {
		hinic3_err(nic_dev, drv, "Set tx cos state failed\n");
		goto set_tx_cos_fail;
	}

	err = hinic3_rx_configure(nic_dev->netdev, dcb_en);
	if (err) {
		hinic3_err(nic_dev, drv, "rx configure failed\n");
		goto rx_configure_fail;
	}

	if (dcb_en) {
		set_bit(HINIC3_DCB_ENABLE, &nic_dev->flags);
		set_bit(HINIC3_DCB_ENABLE, &nic_dev->nic_vram->flags);
	} else {
		clear_bit(HINIC3_DCB_ENABLE, &nic_dev->flags);
		clear_bit(HINIC3_DCB_ENABLE, &nic_dev->nic_vram->flags);
	}
	return 0;
rx_configure_fail:
	hinic3_set_tx_cos_state(nic_dev, dcb_en ? 0 : 1);

set_tx_cos_fail:
	hinic3_update_tx_db_cos(nic_dev, dcb_en ? 0 : 1);
	hinic3_sync_dcb_state(nic_dev->hwdev, 1, dcb_en ? 0 : 1);

	return err;
}

int hinic3_setup_cos(struct net_device *netdev, u8 cos, u8 netif_run)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	struct hinic3_dcb *dcb = nic_dev->dcb;
	int err;

	if (cos && test_bit(HINIC3_SAME_RXTX, &nic_dev->flags)) {
		nicif_err(nic_dev, drv, netdev, "Failed to enable DCB while Symmetric RSS is enabled\n");
		return -EOPNOTSUPP;
	}

	if (cos > dcb->cos_config_num_max) {
		nicif_err(nic_dev, drv, netdev,
			  "Invalid num_tc: %u, max cos: %u\n",
			  cos, dcb->cos_config_num_max);
		return -EINVAL;
	}

	err = hinic3_configure_dcb_hw(nic_dev, cos ? 1 : 0);
	if (err)
		return err;

	return 0;
}

static u8 get_cos_num(u8 hw_valid_cos_bitmap)
{
	u8 support_cos = 0;
	u8 i;

	for (i = 0; i < NIC_DCB_COS_MAX; i++)
		if (hw_valid_cos_bitmap & BIT(i))
			support_cos++;

	return support_cos;
}

static void hinic3_sync_dcb_cfg(struct hinic3_nic_dev *nic_dev,
				const struct hinic3_dcb_config *dcb_cfg)
{
	struct hinic3_dcb_config *hw_dcb_cfg = &nic_dev->dcb->hw_dcb_cfg;

	memcpy(hw_dcb_cfg, dcb_cfg, sizeof(struct hinic3_dcb_config));
}

static int init_default_dcb_cfg(struct hinic3_nic_dev *nic_dev,
				struct hinic3_dcb_config *dcb_cfg)
{
	struct hinic3_dcb *dcb = nic_dev->dcb;
	u8 i, hw_dft_cos_map, port_cos_bitmap, dscp_ind;
	int err;
	int is_in_kexec;

	err = hinic3_cos_valid_bitmap(nic_dev->hwdev,
				      &hw_dft_cos_map, &port_cos_bitmap);
	if (err) {
		hinic3_err(nic_dev, drv, "None cos supported\n");
		return -EFAULT;
	}

	is_in_kexec = vram_get_kexec_flag();

	dcb->func_dft_cos_bitmap = hw_dft_cos_map;
	dcb->port_dft_cos_bitmap = port_cos_bitmap;

	dcb->cos_config_num_max = get_cos_num(hw_dft_cos_map);

	if (is_in_kexec == 0) {
		dcb_cfg->trust = HINIC3_DCB_PCP;
		dcb_cfg->default_cos = (u8)fls(dcb->func_dft_cos_bitmap) - 1;
	} else {
		dcb_cfg->trust = nic_dev->dcb->hw_dcb_cfg.trust;
		dcb_cfg->default_cos = nic_dev->dcb->hw_dcb_cfg.default_cos;
	}
	dcb_cfg->pcp_user_cos_num = dcb->cos_config_num_max;
	dcb_cfg->dscp_user_cos_num = dcb->cos_config_num_max;
	dcb_cfg->pcp_valid_cos_map = hw_dft_cos_map;
	dcb_cfg->dscp_valid_cos_map = hw_dft_cos_map;

	for (i = 0; i < NIC_DCB_COS_MAX; i++) {
		dcb_cfg->pcp2cos[i] = hw_dft_cos_map & BIT(i) ?
				      i : (u8)fls(dcb->func_dft_cos_bitmap) - 1;
		for (dscp_ind = 0; dscp_ind < NIC_DCB_COS_MAX; dscp_ind++)
			dcb_cfg->dscp2cos[i * NIC_DCB_DSCP_NUM + dscp_ind] = dcb_cfg->pcp2cos[i];
	}

	return 0;
}

void hinic3_dcb_reset_hw_config(struct hinic3_nic_dev *nic_dev)
{
	struct hinic3_dcb_config dft_cfg = {0};

	init_default_dcb_cfg(nic_dev, &dft_cfg);
	hinic3_sync_dcb_cfg(nic_dev, &dft_cfg);

	hinic3_info(nic_dev, drv, "Reset DCB configuration done\n");
}

int hinic3_configure_dcb(struct net_device *netdev)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	int err;

	err = hinic3_sync_dcb_state(nic_dev->hwdev, 1,
				    test_bit(HINIC3_DCB_ENABLE, &nic_dev->flags)
				    ? 1 : 0);
	if (err) {
		hinic3_err(nic_dev, drv, "Set dcb state failed\n");
		return err;
	}

	if (test_bit(HINIC3_DCB_ENABLE, &nic_dev->flags))
		hinic3_sync_dcb_cfg(nic_dev, &nic_dev->dcb->wanted_dcb_cfg);
	else
		hinic3_dcb_reset_hw_config(nic_dev);

	return 0;
}

static int hinic3_dcb_alloc(struct hinic3_nic_dev *nic_dev)
{
	u16 func_id;
	int is_use_vram;

	is_use_vram = get_use_vram_flag();
	if (is_use_vram) {
		func_id = hinic3_global_func_id(nic_dev->hwdev);
		snprintf(nic_dev->dcb_name, VRAM_NAME_MAX_LEN,
			 "%s%hu%s", VRAM_CQM_GLB_FUNC_BASE, func_id,
			 VRAM_NIC_DCB);

		nic_dev->dcb = (struct hinic3_dcb *)hi_vram_kalloc(nic_dev->dcb_name,
			sizeof(*nic_dev->dcb));
		if (!nic_dev->dcb) {
			hinic3_err(nic_dev, drv, "Failed to vram alloc dcb.\n");
			return -EFAULT;
		}
	} else {
		nic_dev->dcb = kzalloc(sizeof(*nic_dev->dcb), GFP_KERNEL);
		if (!nic_dev->dcb)
			return -EFAULT;
	}

	return 0;
}

static void hinic3_dcb_free(struct hinic3_nic_dev *nic_dev)
{
	int is_use_vram;

	is_use_vram = get_use_vram_flag();
	if (is_use_vram)
		hi_vram_kfree((void *)nic_dev->dcb, nic_dev->dcb_name,
			      sizeof(*nic_dev->dcb));
	else
		kfree(nic_dev->dcb);
	nic_dev->dcb = NULL;
}

void hinic3_dcb_deinit(struct hinic3_nic_dev *nic_dev)
{
	if (test_bit(HINIC3_DCB_ENABLE, &nic_dev->flags))
		hinic3_sync_dcb_state(nic_dev->hwdev, 1, 0);

	hinic3_dcb_free(nic_dev);
}

int hinic3_dcb_init(struct hinic3_nic_dev *nic_dev)
{
	struct hinic3_dcb_config *hw_dcb_cfg = NULL;
	int err;
	u8 dcb_en = test_bit(HINIC3_DCB_ENABLE, &nic_dev->flags) ? 1 : 0;

	err = hinic3_dcb_alloc(nic_dev);
	if (err != 0) {
		hinic3_err(nic_dev, drv, "Dcb alloc failed.\n");
		return err;
	}

	hw_dcb_cfg = &nic_dev->dcb->hw_dcb_cfg;
	err = init_default_dcb_cfg(nic_dev, hw_dcb_cfg);
	if (err) {
		hinic3_err(nic_dev, drv,
			   "Initialize dcb configuration failed\n");
		hinic3_dcb_free(nic_dev);
		return err;
	}

	memcpy(&nic_dev->dcb->wanted_dcb_cfg, hw_dcb_cfg,
	       sizeof(struct hinic3_dcb_config));

	hinic3_info(nic_dev, drv, "Support num cos %u, default cos %u\n",
		    nic_dev->dcb->cos_config_num_max, hw_dcb_cfg->default_cos);

	err = hinic3_set_tx_cos_state(nic_dev, dcb_en);
	if (err) {
		hinic3_err(nic_dev, drv, "Set tx cos state failed\n");
		hinic3_dcb_free(nic_dev);
		return err;
	}

	return 0;
}

static int change_qos_cfg(struct hinic3_nic_dev *nic_dev,
			  const struct hinic3_dcb_config *dcb_cfg)
{
	struct net_device *netdev = nic_dev->netdev;
	struct hinic3_dcb *dcb = nic_dev->dcb;
	int err = 0;
	u8 user_cos_num = hinic3_get_dev_user_cos_num(nic_dev);

	if (test_and_set_bit(HINIC3_DCB_UP_COS_SETTING, &dcb->dcb_flags)) {
		nicif_warn(nic_dev, drv, netdev,
			   "Cos_up map setting in inprocess, please try again later\n");
		return -EFAULT;
	}

	hinic3_sync_dcb_cfg(nic_dev, dcb_cfg);

	hinic3_update_qp_cos_cfg(nic_dev, user_cos_num);

	clear_bit(HINIC3_DCB_UP_COS_SETTING, &dcb->dcb_flags);

	return err;
}

int hinic3_dcbcfg_set_up_bitmap(struct hinic3_nic_dev *nic_dev)
{
	struct hinic3_dcb *dcb = nic_dev->dcb;
	int err, rollback_err;
	u8 netif_run = 0;
	struct hinic3_dcb_config old_dcb_cfg;
	u8 user_cos_num = hinic3_get_dev_user_cos_num(nic_dev);

	memcpy(&old_dcb_cfg, &dcb->hw_dcb_cfg,
	       sizeof(struct hinic3_dcb_config));

	if (!memcmp(&dcb->wanted_dcb_cfg, &old_dcb_cfg,
		    sizeof(struct hinic3_dcb_config))) {
		nicif_info(nic_dev, drv, nic_dev->netdev,
			   "Same valid up bitmap, don't need to change anything\n");
		return 0;
	}

	if (netif_running(nic_dev->netdev)) {
		netif_run = 1;
		hinic3_vport_down(nic_dev);
	}

	err = change_qos_cfg(nic_dev, &dcb->wanted_dcb_cfg);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Set cos_up map to hw failed\n");
		goto change_qos_cfg_fail;
	}

	if (test_bit(HINIC3_DCB_ENABLE, &nic_dev->flags)) {
		err = hinic3_setup_cos(nic_dev->netdev,
				       user_cos_num, netif_run);
		if (err)
			goto set_err;
	}

	if (netif_run) {
		err = hinic3_vport_up(nic_dev);
		if (err)
			goto vport_up_fail;
	}

	return 0;

vport_up_fail:
	if (test_bit(HINIC3_DCB_ENABLE, &nic_dev->flags))
		hinic3_setup_cos(nic_dev->netdev, user_cos_num
				 ? 0 : user_cos_num, netif_run);

set_err:
	rollback_err = change_qos_cfg(nic_dev, &old_dcb_cfg);
	if (rollback_err)
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Failed to rollback qos configure\n");

change_qos_cfg_fail:
	if (netif_run)
		hinic3_vport_up(nic_dev);

	return err;
}

// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

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

#include "sss_hw.h"
#include "sss_nic_cfg.h"
#include "sss_nic_vf_cfg.h"
#include "sss_nic_mag_cfg.h"
#include "sss_nic_rss_cfg.h"
#include "sss_nic_dev_define.h"
#include "sss_nic_dcb.h"
#include "sss_nic_tx.h"
#include "sss_nic_rx.h"

u8 sss_nic_get_user_cos_num(struct sss_nic_dev *nic_dev)
{
	if (nic_dev->hw_dcb_cfg.trust == 1)
		return nic_dev->hw_dcb_cfg.dscp_user_cos_num;
	if (nic_dev->hw_dcb_cfg.trust == 0)
		return nic_dev->hw_dcb_cfg.pcp_user_cos_num;
	return 0;
}

u8 sss_nic_get_valid_cos_map(struct sss_nic_dev *nic_dev)
{
	if (nic_dev->hw_dcb_cfg.trust == 1)
		return nic_dev->hw_dcb_cfg.dscp_valid_cos_map;
	if (nic_dev->hw_dcb_cfg.trust == 0)
		return nic_dev->hw_dcb_cfg.pcp_valid_cos_map;
	return 0;
}

void sss_nic_update_qp_cos_map(struct sss_nic_dev *nic_dev, u8 cos_num)
{
	u8 cur_cos_num = 0;
	u8 cos_per_qp_num;
	u8 qp_num;
	u8 qp_offset;
	u8 i;
	u8 remain;
	struct sss_nic_dcb_config *dcb_config = &nic_dev->hw_dcb_cfg;
	u8 valid_cos_map;

	if (cos_num == 0)
		return;

	cos_per_qp_num = (u8)(nic_dev->qp_res.qp_num / cos_num);
	if (cos_per_qp_num == 0)
		return;

	remain = nic_dev->qp_res.qp_num % cos_per_qp_num;
	valid_cos_map = sss_nic_get_valid_cos_map(nic_dev);

	memset(dcb_config->cos_qp_num, 0, sizeof(dcb_config->cos_qp_num));
	memset(dcb_config->cos_qp_offset, 0, sizeof(dcb_config->cos_qp_offset));

	for (i = 0; i < SSSNIC_PCP_UP_MAX; i++) {
		if (BIT(i) & valid_cos_map) {
			qp_num = cos_per_qp_num;
			qp_offset = (u8)(cur_cos_num * cos_per_qp_num);

			if (cur_cos_num < remain) {
				qp_offset += cur_cos_num;
				qp_num++;
			} else {
				qp_offset += remain;
			}

			valid_cos_map -= (u8)BIT(i);
			cur_cos_num++;

			dcb_config->cos_qp_num[i] = qp_num;
			dcb_config->cos_qp_offset[i] = qp_offset;
			sss_nic_info(nic_dev, drv, "Qp info: cos %u, qp_offset=%u qp_num=%u\n",
				     i, qp_offset, qp_num);
		}
	}

	memcpy(nic_dev->backup_dcb_cfg.cos_qp_num, dcb_config->cos_qp_num,
	       sizeof(dcb_config->cos_qp_num));
	memcpy(nic_dev->backup_dcb_cfg.cos_qp_offset, dcb_config->cos_qp_offset,
	       sizeof(dcb_config->cos_qp_offset));
}

static void sss_nic_set_sq_cos(struct sss_nic_dev *nic_dev,
			       u16 qid_start, u16 qid_end, u8 cos)
{
	u16 qid;

	for (qid = qid_start; qid < qid_end; qid++)
		nic_dev->sq_desc_group[qid].cos = cos;
}

void sss_nic_update_sq_cos(struct sss_nic_dev *nic_dev, u8 dcb_en)
{
	u8 i;
	u16 q_num;
	u16 qid_start;
	u16 qid_end;

	sss_nic_set_sq_cos(nic_dev, 0, nic_dev->qp_res.qp_num,
			   nic_dev->hw_dcb_cfg.default_cos);

	if (dcb_en == 0)
		return;

	for (i = 0; i < SSSNIC_DCB_COS_MAX; i++) {
		q_num = (u16)nic_dev->hw_dcb_cfg.cos_qp_num[i];
		if (q_num == 0)
			continue;

		qid_start = (u16)nic_dev->hw_dcb_cfg.cos_qp_offset[i];
		qid_end = qid_start + q_num;
		sss_nic_set_sq_cos(nic_dev, qid_start, qid_end, i);
		sss_nic_info(nic_dev, drv, "Update tx db cos, qid_start=%u, qid_end=%u cos=%u\n",
			     qid_start, qid_end, i);
	}
}

static int sss_nic_init_tx_cos_info(struct sss_nic_dev *nic_dev)
{
	int ret;
	struct sss_nic_dcb_info dcb_info = {0};
	struct sss_nic_dcb_config *dcb_config = &nic_dev->hw_dcb_cfg;

	dcb_info.default_cos = dcb_config->default_cos;
	dcb_info.trust = dcb_config->trust;
	memset(dcb_info.dscp2cos, dcb_config->default_cos, sizeof(dcb_info.dscp2cos));
	memset(dcb_info.pcp2cos, dcb_config->default_cos, sizeof(dcb_info.pcp2cos));

	ret = sss_nic_set_dcb_info(nic_dev->nic_io, &dcb_info);
	if (ret != 0)
		sss_nic_err(nic_dev, drv, "Fail to set dcb state, ret: %d\n", ret);

	return ret;
}

static u8 sss_nic_get_cos_num(u8 cos_bitmap)
{
	u8 i;
	u8 cos_count = 0;

	for (i = 0; i < SSSNIC_DCB_COS_MAX; i++)
		if (cos_bitmap & BIT(i))
			cos_count++;

	return cos_count;
}

void sss_nic_sync_dcb_cfg(struct sss_nic_dev *nic_dev,
			  const struct sss_nic_dcb_config *dcb_config)
{
	struct sss_nic_dcb_config *hw_config = &nic_dev->hw_dcb_cfg;

	memcpy(hw_config, dcb_config, sizeof(*dcb_config));
}

static int sss_nic_init_dcb_cfg(struct sss_nic_dev *nic_dev,
				struct sss_nic_dcb_config *dcb_config)
{
	u8 func_cos_bitmap;
	u8 port_cos_bitmap;
	int ret;
	u8 i;
	u8 j;

	ret = sss_get_cos_valid_bitmap(nic_dev->hwdev, &func_cos_bitmap, &port_cos_bitmap);
	if (ret != 0) {
		sss_nic_err(nic_dev, drv, "Fail to get cos valid bitmap, ret: %d\n", ret);
		return -EFAULT;
	}

	nic_dev->max_cos_num = sss_nic_get_cos_num(func_cos_bitmap);
	nic_dev->dft_port_cos_bitmap = port_cos_bitmap;
	nic_dev->dft_func_cos_bitmap = func_cos_bitmap;

	dcb_config->dscp_user_cos_num = nic_dev->max_cos_num;
	dcb_config->pcp_user_cos_num = nic_dev->max_cos_num;
	dcb_config->dscp_valid_cos_map = func_cos_bitmap;
	dcb_config->pcp_valid_cos_map = func_cos_bitmap;
	dcb_config->trust = DCB_PCP;
	dcb_config->default_cos = (u8)fls(nic_dev->dft_func_cos_bitmap) - 1;

	for (i = 0; i < SSSNIC_DCB_COS_MAX; i++) {
		dcb_config->pcp2cos[i] = func_cos_bitmap & BIT(i) ? i : dcb_config->default_cos;
		for (j = 0; j < SSSNIC_DCB_COS_MAX; j++)
			dcb_config->dscp2cos[i * SSSNIC_DCB_DSCP_NUM + j] = dcb_config->pcp2cos[i];
	}

	return 0;
}

static void sss_nic_reset_dcb_config(struct sss_nic_dev *nic_dev)
{
	memset(&nic_dev->hw_dcb_cfg, 0, sizeof(nic_dev->hw_dcb_cfg));
	sss_nic_init_dcb_cfg(nic_dev, &nic_dev->hw_dcb_cfg);
	sss_nic_info(nic_dev, drv, "Success to reset bcb confg\n");
}

int sss_nic_update_dcb_cfg(struct sss_nic_dev *nic_dev)
{
	int ret;

	ret = sss_nic_set_hw_dcb_state(nic_dev, SSSNIC_MBX_OPCODE_SET_DCB_STATE,
				       !!SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_DCB_ENABLE));
	if (ret != 0) {
		sss_nic_err(nic_dev, drv, "Fail to set dcb state, ret: %d\n", ret);
		return ret;
	}

	if (SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_DCB_ENABLE))
		sss_nic_sync_dcb_cfg(nic_dev, &nic_dev->backup_dcb_cfg);
	else
		sss_nic_reset_dcb_config(nic_dev);

	return 0;
}

int sss_nic_dcb_init(struct sss_nic_dev *nic_dev)
{
	int ret;
	struct sss_nic_dcb_config *dcb_config = &nic_dev->hw_dcb_cfg;

	if (SSSNIC_FUNC_IS_VF(nic_dev->hwdev)) {
		dcb_config->default_cos = (u8)fls(nic_dev->dft_func_cos_bitmap) - 1;
		return 0;
	}

	ret = sss_nic_init_dcb_cfg(nic_dev, dcb_config);
	if (ret != 0) {
		sss_nic_err(nic_dev, drv, "Fail to init dcb, ret: %d\n", ret);
		return ret;
	}
	sss_nic_info(nic_dev, drv, "Support num cos %u, default cos %u\n",
		     nic_dev->max_cos_num, dcb_config->default_cos);

	memcpy(&nic_dev->backup_dcb_cfg, &nic_dev->hw_dcb_cfg, sizeof(nic_dev->hw_dcb_cfg));

	ret = sss_nic_init_tx_cos_info(nic_dev);
	if (ret != 0) {
		sss_nic_err(nic_dev, drv, "Fail to set tx cos info, ret: %d\n", ret);
		return ret;
	}

	return 0;
}

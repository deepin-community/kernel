/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_RSS_CFG_H
#define SSS_NIC_RSS_CFG_H

#include <linux/types.h>

#include "sss_nic_cfg_rss_define.h"

int sss_nic_set_rss_type(struct sss_nic_dev *nic_dev, struct sss_nic_rss_type rss_type);

int sss_nic_get_rss_type(struct sss_nic_dev *nic_dev, struct sss_nic_rss_type *rss_type);

int sss_nic_set_rss_hash_engine(struct sss_nic_dev *nic_dev, u8 hash_engine);

int sss_nic_rss_hash_engine(struct sss_nic_dev *nic_dev, u8 cmd, u8 *hash_engine);

int sss_nic_config_rss_to_hw(struct sss_nic_dev *nic_dev,
			     u8 cos_num, u8 *prio_tc, u16 qp_num, u8 rss_en);

int sss_nic_init_hw_rss(struct sss_nic_dev *nic_dev, u16 qp_num);

int sss_nic_set_rss_hash_key(struct sss_nic_dev *nic_dev, const u8 *hash_key);

int sss_nic_cfg_rss_hash_key(struct sss_nic_dev *nic_dev, u8 opcode, u8 *hash_key);

int sss_nic_set_rss_indir_tbl(struct sss_nic_dev *nic_dev, const u32 *indir_tbl);

int sss_nic_get_rss_indir_tbl(struct sss_nic_dev *nic_dev, u32 *indir_tbl);

#endif

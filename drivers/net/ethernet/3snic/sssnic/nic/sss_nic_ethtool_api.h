/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_ETHTOOL_API_H
#define SSS_NIC_ETHTOOL_API_H

#include <linux/netdevice.h>

#define SSSNIC_PRIV_FLAG_SYMM_RSS		BIT(0)
#define SSSNIC_PRIV_FLAG_LINK_UP		BIT(1)
#define SSSNIC_PRIV_FLAG_RQ_RECOVERY	BIT(2)

#define SSSNIC_COALESCE_ALL_QUEUE		0xFFFF

#define SSSNIC_SFP_TYPE_EXT_FLAG		0x3

typedef void (*sss_nic_get_module_info_t)(struct ethtool_modinfo *modinfo, u8 sfp_type_ext);

enum sss_nic_lp_test_type {
	SSSNIC_INTERNAL_LP_TEST = 0,
	SSSNIC_EXTERNAL_LP_TEST = 1,
	SSSNIC_LP_TEST_TYPE_MAX = 2,
};

enum module_type {
	SSSNIC_MODULE_TYPE_SFP = 0x3,
	SSSNIC_MODULE_TYPE_QSFP = 0x0C,
	SSSNIC_MODULE_TYPE_QSFP_PLUS = 0x0D,
	SSSNIC_MODULE_TYPE_QSFP28 = 0x11,
	SSSNIC_MODULE_TYPE_MAX,
};

void sss_nic_update_qp_depth(struct sss_nic_dev *nic_dev,
			     u32 sq_depth, u32 rq_depth);
int sss_nic_check_ringparam_valid(struct net_device *netdev,
				  const struct ethtool_ringparam *ringparam);
void sss_nic_intr_coal_to_ethtool_coal(struct ethtool_coalesce *ethtool_coal,
				       struct sss_nic_intr_coal_info *nic_coal);
int sss_nic_ethtool_get_coalesce(struct net_device *netdev,
				 struct ethtool_coalesce *ethtool_coal, u16 queue);
int sss_nic_set_hw_intr_coal(struct sss_nic_dev *nic_dev,
			     u16 qid, struct sss_nic_intr_coal_info *coal);
int sss_nic_check_coal_param_support(struct net_device *netdev,
				     const struct ethtool_coalesce *coal);
int sss_nic_check_coal_param_valid(struct net_device *netdev,
				   const struct ethtool_coalesce *coal);
int sss_nic_check_coal_param_range(struct net_device *netdev,
				   const struct ethtool_coalesce *coal);
int sss_nic_coalesce_check(struct net_device *netdev,
			   const struct ethtool_coalesce *coal);
int sss_nic_set_coal_param_to_hw(struct sss_nic_dev *nic_dev,
				 struct sss_nic_intr_coal_info *intr_coal_info, u16 queue);
void sss_nic_coalesce_align_check(struct net_device *netdev,
				  struct ethtool_coalesce *coal);
void sss_nic_coalesce_change_check(struct net_device *netdev,
				   struct ethtool_coalesce *coal, u16 queue);
void sss_nic_ethtool_coalesce_to_intr_coal_info(struct sss_nic_intr_coal_info *nic_coal,
						struct ethtool_coalesce *ethtool_coal);
int sss_nic_ethtool_set_coalesce(struct net_device *netdev,
				 struct ethtool_coalesce *coal, u16 queue);
void sss_nic_module_type_sfp(struct ethtool_modinfo *modinfo,
			     u8 sfp_type_ext);
void sss_nic_module_type_qsfp(struct ethtool_modinfo *modinfo,
			      u8 sfp_type_ext);
void sss_nic_module_type_qsfp_plus(struct ethtool_modinfo *modinfo, u8 sfp_type_ext);
void sss_nic_module_type_qsfp28(struct ethtool_modinfo *modinfo,
				u8 sfp_type_ext);
int sss_nic_set_rq_recovery_flag(struct net_device *netdev,
				 u32 flag);
int sss_nic_set_symm_rss_flag(struct net_device *netdev, u32 flag);
void sss_nic_force_link_up(struct sss_nic_dev *nic_dev);
int sss_nic_force_link_down(struct sss_nic_dev *nic_dev);
int sss_nic_set_force_link_flag(struct net_device *netdev, u32 flag);
void sss_nic_loop_test(struct net_device *netdev,
		       struct ethtool_test *eth_test, u64 *data);

#endif

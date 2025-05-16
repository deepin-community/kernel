/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_NIC_PROF_H
#define	HINIC3_NIC_PROF_H
#include <linux/socket.h>

#include <linux/types.h>

#include "hinic3_nic_cfg.h"

struct hinic3_nic_prof_attr {
	void			*priv_data;
	char			netdev_name[IFNAMSIZ];
};

struct hinic3_nic_dev;

#ifdef static
#undef static
#define LLT_STATIC_DEF_SAVED
#endif

static inline char *hinic3_get_dft_netdev_name_fmt(struct hinic3_nic_dev *nic_dev)
{
	if (nic_dev->prof_attr)
		return nic_dev->prof_attr->netdev_name;

	return NULL;
}

#ifdef CONFIG_MODULE_PROF
int hinic3_set_master_dev_state(struct hinic3_nic_dev *nic_dev, u32 flag);
u32 hinic3_get_link(struct net_device *dev)
int hinic3_config_port_mtu(struct hinic3_nic_dev *nic_dev, u32 mtu);
int hinic3_config_port_mac(struct hinic3_nic_dev *nic_dev, struct sockaddr *saddr);
#else
static inline int hinic3_set_master_dev_state(struct hinic3_nic_dev *nic_dev, u32 flag)
{
	return 0;
}

static inline int hinic3_config_port_mtu(struct hinic3_nic_dev *nic_dev, u32 mtu)
{
	return hinic3_set_port_mtu(nic_dev->hwdev, (u16)mtu);
}

static inline int hinic3_config_port_mac(struct hinic3_nic_dev *nic_dev, struct sockaddr *saddr)
{
	return hinic3_update_mac(nic_dev->hwdev, nic_dev->netdev->dev_addr, saddr->sa_data, 0,
				 hinic3_global_func_id(nic_dev->hwdev));
}

#endif

void hinic3_init_nic_prof_adapter(struct hinic3_nic_dev *nic_dev);
void hinic3_deinit_nic_prof_adapter(struct hinic3_nic_dev *nic_dev);

#endif

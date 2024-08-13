/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_NETDEV_OPS_API_H
#define SSS_NIC_NETDEV_OPS_API_H

#include <linux/types.h>
#include <linux/netdevice.h>
#include <linux/netlink.h>

#include "sss_kernel.h"
#ifdef HAVE_XDP_SUPPORT
#include <linux/bpf.h>
#endif
#include "sss_hw.h"
#include "sss_nic_dev_define.h"
#include "sss_nic_io_define.h"

typedef void (*sss_nic_reopen_handler_t)(struct sss_nic_dev *nic_dev,
		const void *priv_data);

int sss_nic_dev_resource_init(struct sss_nic_dev *nic_dev);
void sss_nic_dev_resource_deinit(struct sss_nic_dev *nic_dev);
int sss_nic_qp_resource_init(struct sss_nic_dev *nic_dev,
			     struct sss_nic_qp_info *qp_info,
			     struct sss_nic_qp_resource *qp_res);
void sss_nic_qp_resource_deinit(struct sss_nic_dev *nic_dev,
				struct sss_nic_qp_info *qp_info,
				struct sss_nic_qp_resource *qp_res);
int sss_nic_open_dev(struct sss_nic_dev *nic_dev,
		     struct sss_nic_qp_info *qp_info,
		     struct sss_nic_qp_resource *qp_res);
void sss_nic_close_dev(struct sss_nic_dev *nic_dev,
		       struct sss_nic_qp_info *qp_info);
int sss_nic_vport_up(struct sss_nic_dev *nic_dev);
void sss_nic_vport_down(struct sss_nic_dev *nic_dev);
int sss_nic_update_channel_setting(struct sss_nic_dev *nic_dev,
				   struct sss_nic_qp_resource *qp_res,
				   sss_nic_reopen_handler_t reopen_handler,
				   const void *priv_data);
u16 sss_nic_select_queue_by_hash_func(struct net_device *dev, struct sk_buff *skb,
				      unsigned int num_tx_queues);
u8 sss_nic_get_cos(struct sss_nic_dev *nic_dev, struct sk_buff *skb);
int sss_nic_set_feature(struct sss_nic_dev *nic_dev, netdev_features_t old_feature,
			netdev_features_t new_feature);

int sss_nic_enable_netdev_feature(struct sss_nic_dev *nic_dev);

#ifdef IFLA_VF_MAX
int sss_nic_set_hw_vf_vlan(struct sss_nic_dev *nic_dev,
			   u16 cur_vlanprio, int vf, u16 vlan, u8 qos);
#endif

#ifdef HAVE_XDP_SUPPORT
#define SSSNIC_XDP_MAX_MTU(nic_dev) ((nic_dev)->rx_buff_len - (ETH_HLEN + ETH_FCS_LEN + VLAN_HLEN))
#ifdef HAVE_NDO_BPF_NETDEV_BPF
int sss_nic_setup_xdp(struct sss_nic_dev *nic_dev, struct netdev_bpf *xdp);
#else
int sss_nic_setup_xdp(struct sss_nic_dev *nic_dev, struct netdev_xdp *xdp);
#endif
#endif
void sss_nic_get_tx_stats(struct sss_nic_dev *nic_dev,
			  struct rtnl_link_stats64 *stats);
void sss_nic_get_rx_stats(struct sss_nic_dev *nic_dev,
			  struct rtnl_link_stats64 *stats);

u32 sss_nic_get_io_stats_size(const struct sss_nic_dev *nic_dev);

#endif

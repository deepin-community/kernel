/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_CFG_H
#define SSS_NIC_CFG_H

#include <linux/types.h>
#include <linux/netdevice.h>

#include "sss_nic_cfg_define.h"
#include "sss_nic_dev_define.h"

#define SSSNIC_SUPPORT_FEATURE(nic_io, feature) \
	((nic_io)->feature_cap & SSSNIC_F_##feature)
#define SSSNIC_SUPPORT_CSUM(nic_io) SSSNIC_SUPPORT_FEATURE(nic_io, CSUM)
#define SSSNIC_SUPPORT_SCTP_CRC(nic_io) SSSNIC_SUPPORT_FEATURE(nic_io, SCTP_CRC)
#define SSSNIC_SUPPORT_TSO(nic_io) SSSNIC_SUPPORT_FEATURE(nic_io, TSO)
#define SSSNIC_SUPPORT_UFO(nic_io) SSSNIC_SUPPORT_FEATURE(nic_io, UFO)
#define SSSNIC_SUPPORT_LRO(nic_io) SSSNIC_SUPPORT_FEATURE(nic_io, LRO)
#define SSSNIC_SUPPORT_RSS(nic_io) SSSNIC_SUPPORT_FEATURE(nic_io, RSS)
#define SSSNIC_SUPPORT_RXVLAN_FILTER(nic_io) \
	SSSNIC_SUPPORT_FEATURE(nic_io, RX_VLAN_FILTER)
#define SSSNIC_SUPPORT_VLAN_OFFLOAD(nic_io) \
	(SSSNIC_SUPPORT_FEATURE(nic_io, RX_VLAN_STRIP) && \
	 SSSNIC_SUPPORT_FEATURE(nic_io, TX_VLAN_INSERT))
#define SSSNIC_SUPPORT_VXLAN_OFFLOAD(nic_io) \
	SSSNIC_SUPPORT_FEATURE(nic_io, VXLAN_OFFLOAD)
#define SSSNIC_SUPPORT_IPSEC_OFFLOAD(nic_io) \
	SSSNIC_SUPPORT_FEATURE(nic_io, IPSEC_OFFLOAD)
#define SSSNIC_SUPPORT_FDIR(nic_io) SSSNIC_SUPPORT_FEATURE(nic_io, FDIR)
#define SSSNIC_SUPPORT_PROMISC(nic_io) SSSNIC_SUPPORT_FEATURE(nic_io, PROMISC)
#define SSSNIC_SUPPORT_ALLMULTI(nic_io) SSSNIC_SUPPORT_FEATURE(nic_io, ALLMULTI)
#define SSSNIC_SUPPORT_VF_MAC(nic_io) SSSNIC_SUPPORT_FEATURE(nic_io, VF_MAC)
#define SSSNIC_SUPPORT_RATE_LIMIT(nic_io) SSSNIC_SUPPORT_FEATURE(nic_io, RATE_LIMIT)
#define SSSNIC_SUPPORT_RXQ_RECOVERY(nic_io) SSSNIC_SUPPORT_FEATURE(nic_io, RXQ_RECOVERY)

int sss_nic_set_mac(struct sss_nic_dev *nic_dev, const u8 *mac_addr,
		    u16 vlan_id, u16 func_id, u16 channel);

int sss_nic_del_mac(struct sss_nic_dev *nic_dev, const u8 *mac_addr,
		    u16 vlan_id, u16 func_id, u16 channel);

int sss_nic_add_tcam_rule(struct sss_nic_dev *nic_dev, struct sss_nic_tcam_rule_cfg *tcam_rule);
int sss_nic_del_tcam_rule(struct sss_nic_dev *nic_dev, u32 index);

int sss_nic_alloc_tcam_block(struct sss_nic_dev *nic_dev, u16 *index);
int sss_nic_free_tcam_block(struct sss_nic_dev *nic_dev, u16 *index);

int sss_nic_set_fdir_tcam_rule_filter(struct sss_nic_dev *nic_dev, bool enable);

int sss_nic_flush_tcam_rule(struct sss_nic_dev *nic_dev);

int sss_nic_update_mac(struct sss_nic_dev *nic_dev, u8 *new_mac);

int sss_nic_get_default_mac(struct sss_nic_dev *nic_dev, u8 *mac_addr);

int sss_nic_set_dev_mtu(struct sss_nic_dev *nic_dev, u16 new_mtu);

int sss_nic_get_vport_stats(struct sss_nic_dev *nic_dev,
			    u16 func_id, struct sss_nic_port_stats *stats);

int sss_nic_force_drop_tx_pkt(struct sss_nic_dev *nic_dev);

int sss_nic_set_rx_mode(struct sss_nic_dev *nic_dev, u32 rx_mode);

int sss_nic_set_rx_vlan_offload(struct sss_nic_dev *nic_dev, bool en);

int sss_nic_set_rx_lro_state(struct sss_nic_dev *nic_dev, bool en, u32 timer, u32 max_pkt_len);

int sss_nic_config_vlan(struct sss_nic_dev *nic_dev, u8 opcode, u16 vlan_id);

int sss_nic_set_hw_vport_state(struct sss_nic_dev *nic_dev,
			       u16 func_id, bool enable, u16 channel);

int sss_nic_set_dcb_info(struct sss_nic_io *nic_io, struct sss_nic_dcb_info *dcb_info);

int sss_nic_set_hw_dcb_state(struct sss_nic_dev *nic_dev, u8 op_code, u8 state);

int sss_nic_clear_hw_qp_resource(struct sss_nic_dev *nic_dev);

int sss_nic_get_hw_pause_info(struct sss_nic_dev *nic_dev, struct sss_nic_pause_cfg *pause_config);

int sss_nic_set_hw_pause_info(struct sss_nic_dev *nic_dev, struct sss_nic_pause_cfg pause_config);

int sss_nic_set_vlan_fliter(struct sss_nic_dev *nic_dev, bool en);

int sss_nic_update_mac_vlan(struct sss_nic_dev *nic_dev,
			    u16 old_vlan, u16 new_vlan, int vf_id);

int sss_nic_cache_out_qp_resource(struct sss_nic_io *nic_io);

int sss_nic_set_feature_to_hw(struct sss_nic_io *nic_io);

void sss_nic_update_nic_feature(struct sss_nic_dev *nic_dev, u64 feature);

int sss_nic_io_init(struct sss_nic_dev *nic_dev);

void sss_nic_io_deinit(struct sss_nic_dev *nic_dev);

int sss_nic_rq_hw_pc_info(struct sss_nic_dev *nic_dev,
			  struct sss_nic_rq_pc_info *out_info, u16 num_qps, u16 wqe_type);
int sss_nic_set_pf_rate(struct sss_nic_dev *nic_dev, u8 speed);

#endif

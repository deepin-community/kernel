/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_VF_CFG_H
#define SSS_NIC_VF_CFG_H

#include "sss_nic_cfg_vf_define.h"
#include "sss_nic_io_define.h"

#define SSSNIC_GET_VF_SPOOFCHK(nic_io, vf_id)	\
		(!!(nic_io)->vf_info_group[vf_id].spoofchk)

int sss_nic_set_vf_spoofchk(struct sss_nic_io *nic_io, u16 vf_id, bool spoofchk);

int sss_nic_create_vf_vlan(struct sss_nic_io *nic_io, int vf_id, u16 vlan, u8 qos);

int sss_nic_destroy_vf_vlan(struct sss_nic_io *nic_io, int vf_id);

u16 sss_nic_vf_info_vlan_prio(struct sss_nic_io *nic_io, int vf_id);

int sss_nic_set_vf_tx_rate_limit(struct sss_nic_io *nic_io, u16 vf_id, u32 min_rate, u32 max_rate);

void sss_nic_get_vf_attribute(struct sss_nic_io *nic_io, u16 vf_id,
			      struct ifla_vf_info *ifla_vf);

int sss_nic_set_vf_link_state(struct sss_nic_io *nic_io, u16 vf_id, int link);

void sss_nic_clear_all_vf_info(struct sss_nic_io *nic_io);

#ifdef HAVE_NDO_SET_VF_TRUST
bool sss_nic_get_vf_trust(struct sss_nic_io *nic_io, int vf_id);
int sss_nic_set_vf_trust(struct sss_nic_io *nic_io, u16 vf_id, bool trust);
#endif

int sss_nic_set_vf_vlan(struct sss_nic_io *nic_io, u8 opcode, u16 vid,
			u8 qos, int vf_id);

int sss_nic_register_io_callback(struct sss_nic_io *nic_io);

void sss_nic_unregister_io_callback(struct sss_nic_io *nic_io);

int sss_nic_init_pf_vf_info(struct sss_nic_io *nic_io);

void sss_nic_deinit_pf_vf_info(struct sss_nic_io *nic_io);

#endif

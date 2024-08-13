/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_CFG_VF_DEFINE_H
#define SSS_NIC_CFG_VF_DEFINE_H

#include <linux/types.h>

#define SSSNIC_OS_VF_ID_TO_HW(os_vf_id) ((os_vf_id) + 1)
#define SSSNIC_HW_VF_ID_TO_OS(hw_vf_id) ((hw_vf_id) - 1)

#define SSSNIC_VLAN_PRIORITY_SHIFT 13

#define SSSNIC_CONFIG_ALL_QUEUE_VLAN_CTX 0xFFFF

#define SSSNIC_GET_VLAN_PRIO(vlan, qos)                                        \
	((u16)((vlan) | ((qos) << SSSNIC_VLAN_PRIORITY_SHIFT)))

struct sss_nic_vlan_ctx {
	u32 func_id;
	u32 qid; /* if qid = 0xFFFF, config current function all queue */
	u32 tag;
	u32 mode;
	u32 sel;
};

#endif

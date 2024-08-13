/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_QP_DEFINE_H
#define SSS_NIC_QP_DEFINE_H

#include <linux/types.h>

#include "sss_kernel.h"
#include "sss_hw_common.h"

struct sss_nic_cqe {
	u32 state;
	u32 vlan_len;

	u32 offload_type;
	u32 hash;
	u32 xid;
	u32 decrypt_desc;
	u32 rsvd6;
	u32 pkt_desc;
};

struct sss_nic_normal_rqe {
	u32 bd_hi_addr;
	u32 bd_lo_addr;
	u32 cqe_hi_addr;
	u32 cqe_lo_addr;
};

struct sss_nic_sge_section {
	struct sss_sge sge;
	u32 rsvd;
};

struct sss_nic_extend_rqe {
	struct sss_nic_sge_section bd_sect;
	struct sss_nic_sge_section cqe_sect;
};

struct sss_nic_rqe {
	union {
		struct sss_nic_normal_rqe normal_rqe;
		struct sss_nic_extend_rqe extend_rqe;
	};
};

#endif

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_MGMT_INFO_H
#define SSS_MGMT_INFO_H

#include <linux/types.h>

#include "sss_hw_svc_cap.h"
#include "sss_eq_info.h"
#include "sss_irq_info.h"

struct sss_dev_sf_svc_attr {
	u8 rdma_en;
	u8 rsvd[3];
};

enum sss_intr_type {
	SSS_INTR_TYPE_MSIX,
	SSS_INTR_TYPE_MSI,
	SSS_INTR_TYPE_INT,
	SSS_INTR_TYPE_NONE,

	/* PXE,OVS need single thread processing,
	 * synchronization messages must use poll wait mechanism interface
	 */
};

/* device service capability */
struct sss_service_cap {
	struct sss_dev_sf_svc_attr sf_svc_attr;
	u16 svc_type; /* user input service type */
	u16 chip_svc_type; /* HW supported service type, reference to sss_servic_bit_define */

	u8 host_id;
	u8 ep_id;
	u8 er_id; /* PF/VF's ER */
	u8 port_id; /* PF/VF's physical port */

	/* Host global resources */
	u16 host_total_function;
	u8 pf_num;
	u8 pf_id_start;
	u16 vf_num; /* max numbers of vf in current host */
	u16 vf_id_start;
	u8 host_oq_id_mask_val;
	u8 host_valid_bitmap;
	u8 master_host_id;
	u8 srv_multi_host_mode;

	u8 timer_pf_num;
	u8 timer_pf_id_start;
	u16 timer_vf_num;
	u16 timer_vf_id_start;
	u8 flexq_en;
	u8 resvd;

	u8 cos_valid_bitmap;
	u8 port_cos_valid_bitmap;
	u16 max_vf; /* max VF number that PF supported */
	u16 pseudo_vf_start_id;
	u16 pseudo_vf_num;
	u32 pseudo_vf_max_pctx;
	u16 pseudo_vf_bfilter_start_addr;
	u16 pseudo_vf_bfilter_len;

	u16 pseudo_vf_cfg_num;
	u16 virtio_vq_size;

	/* DO NOT get interrupt_type from firmware */
	enum sss_intr_type intr_type;

	u8 sf_en; /* stateful business status */
	u8 timer_en; /* 0:disable, 1:enable */
	u8 bloomfilter_en; /* 0:disable, 1:enable */
	u8 lb_mode;
	u8 smf_pg;
	u8 rsvd[3];

	u32 max_connect_num; /* PF/VF maximum connection number(1M) */

	/* The maximum connections which can be stick to cache memory, max 1K */
	u16 max_stick2cache_num;

	/* Starting address in cache memory for bloom filter, 64Bytes aligned */
	u16 bfilter_start_addr;

	/* Length for bloom filter, aligned on 64Bytes. The size is length*64B.
	 * Bloom filter memory size + 1 must be power of 2.
	 * The maximum memory size of bloom filter is 4M
	 */
	u16 bfilter_len;

	/* The size of hash bucket tables, align on 64 entries.
	 * Be used to AND (&) the hash value. Bucket Size +1 must be power of 2.
	 * The maximum number of hash bucket is 4M
	 */
	u16 hash_bucket_num;

	struct sss_nic_service_cap		nic_cap; /* NIC capability */
	struct sss_rdma_service_cap		rdma_cap; /* RDMA capability */
	struct sss_fc_service_cap		fc_cap; /* FC capability */
	struct sss_toe_service_cap		toe_cap; /* ToE capability */
	struct sss_ovs_service_cap		ovs_cap; /* OVS capability */
	struct sss_ipsec_service_cap	ipsec_cap; /* IPsec capability */
	struct sss_ppa_service_cap		ppa_cap; /* PPA capability */
	struct sss_vbs_service_cap		vbs_cap; /* VBS capability */
};

struct sss_svc_cap_info {
	u32 func_id;
	struct sss_service_cap cap;
};

struct sss_mgmt_info {
	void *hwdev;
	struct sss_service_cap	svc_cap;
	struct sss_eq_info		eq_info; /* CEQ */
	struct sss_irq_info		irq_info; /* IRQ */
	u32						func_seq_num; /* temporary */
};

#endif

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_IO_DEFINE_H
#define SSS_NIC_IO_DEFINE_H

#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>

#include "sss_kernel.h"
#include "sss_hw_wq.h"
#include "sss_nic_dcb_define.h"
#include "sss_nic_cfg_mag_define.h"

struct sss_nic_vf_info {
	u8 user_mac[ETH_ALEN];
	u8 drv_mac[ETH_ALEN];
	u16 qp_num;
	u16 pf_vlan;

	u8 pf_qos;
	u8 rsvd0[3];
	u32 extra_feature;

	u32 min_rate;
	u32 max_rate;

	u8 specified_mac;
	u8 attach;
	u8 trust;
	u8 spoofchk;
	u8 link_forced;
	u8 link_up; /* only valid if VF link is forced */
	u8 rsvd1[2];
};

struct sss_nic_io_queue {
	struct sss_wq wq;
	union {
		u8 wqe_type; /* for rq */
		u8 owner; /* for sq */
	};
	u8	rsvd1;
	u16	rsvd2;

	u16 qid;
	u16 msix_id;

	u8 __iomem *db_addr;

	union {
		struct {
			void *ci_addr;
		} tx;

		struct {
			u16 *pi_vaddr;
			dma_addr_t pi_daddr;
		} rx;
	};
} ____cacheline_aligned;

struct sss_nic_io {
	void			*hwdev;
	void			*pcidev_hdl;
	void			*dev_hdl;
	void			*nic_dev;

	struct sss_nic_io_queue		*sq_group;
	struct sss_nic_io_queue		*rq_group;

	u16				active_qp_num;
	u16				max_qp_num;

	u8				link_status;
	u8				rsvd1[3];

	void			*ci_base_vaddr;
	dma_addr_t		ci_base_daddr;

	u8 __iomem		*sq_db_addr;
	u8 __iomem		*rq_db_addr;

	u16				rx_buff_len;
	u16				max_vf_num;

	struct sss_nic_vf_info		*vf_info_group;

	u64				feature_cap;

	struct sss_nic_dcb_info		dcb_info;

	struct sss_nic_mag_cfg		mag_cfg;
};

struct sss_nic_qp_info {
	u16	qp_num;
	u8	resvd[6];

	u32	sq_depth;
	u32	rq_depth;

	struct sss_nic_io_queue *sq_group;
	struct sss_nic_io_queue *rq_group;
};

#endif

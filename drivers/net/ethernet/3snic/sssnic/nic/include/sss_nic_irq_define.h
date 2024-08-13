/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_IRQ_DEFINE_H
#define SSS_NIC_IRQ_DEFINE_H

#include <linux/types.h>
#include <linux/netdevice.h>

#include "sss_kernel.h"
#include "sss_hw_common.h"

struct sss_nic_irq_cfg {
	struct net_device	*netdev;
	u16			msix_id; /* PCIe MSIX id */
	u16			rsvd1;
	u32			irq_id; /* OS IRQ id */
	char			irq_name[IFNAMSIZ + 16];
	struct napi_struct	napi;
	cpumask_t		affinity_mask;
	void			*sq;
	void			*rq;
};

struct sss_nic_intr_coal_info {
	u8	pending_limt;
	u8	coalesce_timer;
	u8	resend_timer;

	u64	pkt_rate_low;
	u8	rx_usecs_low;
	u8	rx_pending_limt_low;
	u64	pkt_rate_high;
	u8	rx_usecs_high;
	u8	rx_pending_limt_high;

	u8	user_set_intr_coal_flag;
};

#endif

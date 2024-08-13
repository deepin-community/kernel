/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HW_IRQ_H
#define SSS_HW_IRQ_H

#include <linux/types.h>

enum sss_msix_auto_mask {
	SSS_CLR_MSIX_AUTO_MASK,
	SSS_SET_MSIX_AUTO_MASK,
};

enum sss_msix_state {
	SSS_MSIX_ENABLE,
	SSS_MSIX_DISABLE,
};

struct sss_irq_desc {
	u16 msix_id; /* PCIe MSIX id */
	u16 rsvd;
	u32 irq_id; /* OS IRQ id */
};

struct sss_irq_cfg {
	u32 lli_set;
	u32 coalesc_intr_set;
	u16 msix_id;
	u8 lli_credit;
	u8 lli_timer;
	u8 pending;
	u8 coalesc_timer;
	u8 resend_timer;
};

#endif

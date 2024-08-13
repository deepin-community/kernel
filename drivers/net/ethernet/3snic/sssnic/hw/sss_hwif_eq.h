/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWIF_EQ_H
#define SSS_HWIF_EQ_H

#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>

#include "sss_hw.h"
#include "sss_hwdev.h"
#include "sss_csr.h"

#define SSS_EQ_UPDATE_CI_STEP			64

#define SSS_TASK_PROCESS_EQE_LIMIT		1024

#define SSS_MIN_EQ_PAGE_SIZE			0x1000 /* min eq page size 4K Bytes */
#define SSS_MAX_EQ_PAGE_SIZE			0x400000 /* max eq page size 4M Bytes */

#define SSS_EQ_USLEEP_LOW_LIMIT			900
#define SSS_EQ_USLEEP_HIG_LIMIT			1000

#define SSS_EQ_IRQ_ID(eq) ((eq)->irq_desc.msix_id)

#define SSS_GET_EQ_ELEM(eq, id)		\
	(((u8 *)(eq)->page_array[(id) / (eq)->num_entry_per_pg].align_vaddr) + \
	(u32)(((id) & ((eq)->num_entry_per_pg - 1)) * (eq)->entry_size))

#define	SSS_EQ_VALID_SHIFT				31
#define SSS_EQ_WRAPPED(eq)				((u32)(eq)->wrap << SSS_EQ_VALID_SHIFT)

#define SSS_AEQ_MAX_PAGE				4
#define SSS_CEQ_MAX_PAGE				8

#define SSS_AEQE_SIZE					64
#define SSS_CEQE_SIZE					4

#define SSS_EQ_CI_REG_ADDR(eq)	\
			(((eq)->type == SSS_AEQ) ? \
			SSS_CSR_AEQ_CI_ADDR : SSS_CSR_CEQ_CI_ADDR)

#define SSS_EQ_PI_REG_ADDR(eq)		\
			(((eq)->type == SSS_AEQ) ? \
			SSS_CSR_AEQ_PI_ADDR : SSS_CSR_CEQ_PI_ADDR)

#define SSS_EQ_MSIX_RESEND_TIMER_CLEAR	1

#define SSS_EQ_ELEM_DESC_TYPE_SHIFT		0
#define SSS_EQ_ELEM_DESC_SRC_SHIFT		7
#define SSS_EQ_ELEM_DESC_SIZE_SHIFT		8
#define SSS_EQ_ELEM_DESC_WRAPPED_SHIFT	31

#define SSS_EQ_ELEM_DESC_TYPE_MASK		0x7FU
#define SSS_EQ_ELEM_DESC_SRC_MASK		0x1U
#define SSS_EQ_ELEM_DESC_SIZE_MASK		0xFFU
#define SSS_EQ_ELEM_DESC_WRAPPED_MASK	0x1U

#define SSS_GET_EQE_DESC(val, member)	\
			(((val) >> SSS_EQ_ELEM_DESC_##member##_SHIFT) & \
				SSS_EQ_ELEM_DESC_##member##_MASK)

#define SSS_PAGE_IN_4K(page_size)		((page_size) >> 12)
#define SSS_SET_EQ_HW_PAGE_SIZE(eq)		((u32)ilog2(SSS_PAGE_IN_4K((eq)->page_size)))

enum sss_eq_intr_mode {
	SSS_INTR_MODE_ARMED,
	SSS_INTR_MODE_ALWAY,
};

enum sss_eq_ci_arm_state {
	SSS_EQ_NOT_ARMED,
	SSS_EQ_ARMED,
};

#define SSS_EQ_ARM_STATE(unfinish)                                             \
	((unfinish) ? SSS_EQ_NOT_ARMED : SSS_EQ_ARMED)

#define SSS_EQ_INTR_COALESC				1
#define SSS_EQ_INTR_COALESC_TIMER_CFG	0xFF
#define SSS_EQ_INTR_RESEND_TIMER_CFG	7

void sss_increase_eq_ci(struct sss_eq *eq);
int sss_init_eq(struct sss_hwdev *hwdev, struct sss_eq *eq,
		struct sss_irq_desc *entry);
void sss_deinit_eq(struct sss_eq *eq);
void sss_chip_set_eq_ci(struct sss_eq *eq, u32 arm_state);
void sss_init_eq_intr_info(struct sss_irq_cfg *intr_info);

#endif

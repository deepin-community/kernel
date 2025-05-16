/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_EQS_H
#define HINIC3_EQS_H

#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>

#include "hinic3_common.h"
#include "hinic3_crm.h"
#include "hinic3_hw.h"
#include "hinic3_hwdev.h"

#define HINIC3_MAX_AEQS			4
#define HINIC3_MAX_CEQS			32

#define HINIC3_AEQ_MAX_PAGES		4
#define HINIC3_CEQ_MAX_PAGES		8

#define HINIC3_AEQE_SIZE		64
#define HINIC3_CEQE_SIZE		4

#define HINIC3_AEQE_DESC_SIZE		4
#define HINIC3_AEQE_DATA_SIZE		\
			(HINIC3_AEQE_SIZE - HINIC3_AEQE_DESC_SIZE)

#define HINIC3_DEFAULT_AEQ_LEN		0x10000
#define HINIC3_DEFAULT_CEQ_LEN		0x10000

#define HINIC3_MIN_EQ_PAGE_SIZE		0x1000   /* min eq page size 4K Bytes */
#define HINIC3_MAX_EQ_PAGE_SIZE		0x400000 /* max eq page size 4M Bytes */

#define HINIC3_MIN_AEQ_LEN		64
#define HINIC3_MAX_AEQ_LEN		\
	((HINIC3_MAX_EQ_PAGE_SIZE / HINIC3_AEQE_SIZE) * HINIC3_AEQ_MAX_PAGES)

#define HINIC3_MIN_CEQ_LEN		64
#define HINIC3_MAX_CEQ_LEN		\
	((HINIC3_MAX_EQ_PAGE_SIZE / HINIC3_CEQE_SIZE) * HINIC3_CEQ_MAX_PAGES)
#define	HINIC3_CEQ_ID_CMDQ		0

#define EQ_IRQ_NAME_LEN			64

#define EQ_USLEEP_LOW_BOUND		900
#define EQ_USLEEP_HIG_BOUND		1000

enum hinic3_eq_type {
	HINIC3_AEQ,
	HINIC3_CEQ
};

enum hinic3_eq_intr_mode {
	HINIC3_INTR_MODE_ARMED,
	HINIC3_INTR_MODE_ALWAYS,
};

enum hinic3_eq_ci_arm_state {
	HINIC3_EQ_NOT_ARMED,
	HINIC3_EQ_ARMED,
};

struct hinic3_eq {
	struct hinic3_hwdev		*hwdev;
	u16				q_id;
	u16				rsvd1;
	enum hinic3_eq_type		type;
	u32				page_size;
	u32				orig_page_size;
	u32				eq_len;

	u32				cons_idx;
	u16				wrapped;
	u16				rsvd2;

	u16				elem_size;
	u16				num_pages;
	u32				num_elem_in_pg;

	struct irq_info			eq_irq;
	char				irq_name[EQ_IRQ_NAME_LEN];

	struct hinic3_dma_addr_align	*eq_pages;

	struct work_struct		aeq_work;
	struct tasklet_struct		ceq_tasklet;

	u64				hard_intr_jif;
	u64				soft_intr_jif;

	u64				rsvd3;
};

struct hinic3_aeq_elem {
	u8	aeqe_data[HINIC3_AEQE_DATA_SIZE];
	u32	desc;
};

enum hinic3_aeq_cb_state {
	HINIC3_AEQ_HW_CB_REG = 0,
	HINIC3_AEQ_HW_CB_RUNNING,
	HINIC3_AEQ_SW_CB_REG,
	HINIC3_AEQ_SW_CB_RUNNING,
};

struct hinic3_aeqs {
	struct hinic3_hwdev     *hwdev;

	hinic3_aeq_hwe_cb       aeq_hwe_cb[HINIC3_MAX_AEQ_EVENTS];
	void                    *aeq_hwe_cb_data[HINIC3_MAX_AEQ_EVENTS];
	hinic3_aeq_swe_cb       aeq_swe_cb[HINIC3_MAX_AEQ_SW_EVENTS];
	void                    *aeq_swe_cb_data[HINIC3_MAX_AEQ_SW_EVENTS];
	unsigned long           aeq_hw_cb_state[HINIC3_MAX_AEQ_EVENTS];
	unsigned long           aeq_sw_cb_state[HINIC3_MAX_AEQ_SW_EVENTS];

	struct hinic3_eq        aeq[HINIC3_MAX_AEQS];
	u16                     num_aeqs;
	u16                     rsvd1;
	u32                     rsvd2;

	struct workqueue_struct *workq;
};

enum hinic3_ceq_cb_state {
	HINIC3_CEQ_CB_REG = 0,
	HINIC3_CEQ_CB_RUNNING,
};

struct hinic3_ceqs {
	struct hinic3_hwdev     *hwdev;

	hinic3_ceq_event_cb     ceq_cb[HINIC3_MAX_CEQ_EVENTS];
	void                    *ceq_cb_data[HINIC3_MAX_CEQ_EVENTS];
	void                    *ceq_data[HINIC3_MAX_CEQ_EVENTS];
	unsigned long	        ceq_cb_state[HINIC3_MAX_CEQ_EVENTS];

	struct hinic3_eq        ceq[HINIC3_MAX_CEQS];
	u16                     num_ceqs;
	u16                     rsvd1;
	u32                     rsvd2;
};

int hinic3_aeqs_init(struct hinic3_hwdev *hwdev, u16 num_aeqs,
		     struct irq_info *msix_entries);

void hinic3_aeqs_free(struct hinic3_hwdev *hwdev);

int hinic3_ceqs_init(struct hinic3_hwdev *hwdev, u16 num_ceqs,
		     struct irq_info *msix_entries);

void hinic3_ceqs_free(struct hinic3_hwdev *hwdev);

void hinic3_get_ceq_irqs(struct hinic3_hwdev *hwdev, struct irq_info *irqs,
			 u16 *num_irqs);

void hinic3_get_aeq_irqs(struct hinic3_hwdev *hwdev, struct irq_info *irqs,
			 u16 *num_irqs);

void hinic3_dump_ceq_info(struct hinic3_hwdev *hwdev);

void hinic3_dump_aeq_info(struct hinic3_hwdev *hwdev);

int hinic3_reschedule_eq(struct hinic3_hwdev *hwdev, enum hinic3_eq_type type,
			 u16 eq_id);
#endif

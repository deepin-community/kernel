// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/mutex.h>

#include "sss_kernel.h"
#include "sss_hwdev.h"
#include "sss_eq_info.h"
#include "sss_hw_svc_cap.h"
#include "sss_hw_irq.h"
#include "sss_hw_aeq.h"
#include "sss_hw_export.h"
#include "sss_hwif_aeq.h"
#include "sss_hw_common.h"
#include "sss_hwif_eq.h"
#include "sss_hwif_api.h"
#include "sss_hwif_export.h"
#include "sss_csr.h"

#define SSS_DEF_AEQ_DEPTH		0x10000

#define SSS_MIN_AEQ_DEPTH		64
#define SSS_MAX_AEQ_DEPTH		\
	((SSS_MAX_EQ_PAGE_SIZE / SSS_AEQE_SIZE) * SSS_AEQ_MAX_PAGE)

#define SSS_AEQE_DESC_SIZE		4
#define SSS_AEQE_DATA_SIZE		(SSS_AEQE_SIZE - SSS_AEQE_DESC_SIZE)

struct sss_aeq_elem {
	u8	aeqe_data[SSS_AEQE_DATA_SIZE];
	u32	desc;
};

#define SSS_GET_AEQ_ELEM(aeq, id)		\
			((struct sss_aeq_elem *)SSS_GET_EQ_ELEM((aeq), (id)))

#define SSS_GET_CUR_AEQ_ELEM(aeq)		SSS_GET_AEQ_ELEM((aeq), (aeq)->ci)

#define SSS_GET_AEQ_SW_EVENT(type)		\
			(((type) >= SSS_ERR_MAX) ? \
			SSS_STF_EVENT : SSS_STL_EVENT)

#define SSS_AEQ_CTRL_0_INTR_ID_SHIFT		0
#define SSS_AEQ_CTRL_0_DMA_ATTR_SHIFT		12
#define SSS_AEQ_CTRL_0_PCI_INTF_ID_SHIFT	20
#define SSS_AEQ_CTRL_0_INTR_MODE_SHIFT		31

#define SSS_AEQ_CTRL_0_INTR_ID_MASK			0x3FFU
#define SSS_AEQ_CTRL_0_DMA_ATTR_MASK		0x3FU
#define SSS_AEQ_CTRL_0_PCI_INTF_ID_MASK		0x7U
#define SSS_AEQ_CTRL_0_INTR_MODE_MASK		0x1U

#define SSS_SET_AEQ_CTRL_0(val, member)		\
			(((val) & SSS_AEQ_CTRL_0_##member##_MASK) << \
				SSS_AEQ_CTRL_0_##member##_SHIFT)

#define SSS_CLEAR_AEQ_CTRL_0(val, member)		\
			((val) & (~(SSS_AEQ_CTRL_0_##member##_MASK << \
				SSS_AEQ_CTRL_0_##member##_SHIFT)))

#define SSS_AEQ_CTRL_1_SIZE_SHIFT			0
#define SSS_AEQ_CTRL_1_ELEM_SIZE_SHIFT		24
#define SSS_AEQ_CTRL_1_PAGE_SIZE_SHIFT		28

#define SSS_AEQ_CTRL_1_SIZE_MASK				0x1FFFFFU
#define SSS_AEQ_CTRL_1_ELEM_SIZE_MASK		0x3U
#define SSS_AEQ_CTRL_1_PAGE_SIZE_MASK		0xFU

#define SSS_SET_AEQ_CTRL_1(val, member)		\
			(((val) & SSS_AEQ_CTRL_1_##member##_MASK) << \
				SSS_AEQ_CTRL_1_##member##_SHIFT)

#define SSS_CLEAR_AEQ_CTRL_1(val, member)	\
			((val) & (~(SSS_AEQ_CTRL_1_##member##_MASK << \
				SSS_AEQ_CTRL_1_##member##_SHIFT)))

#define SSS_ELEM_SIZE_IN_32B(aeq)		(((aeq)->entry_size) >> 5)
#define SSS_SET_EQ_HW_E_SIZE(aeq)		((u32)ilog2(SSS_ELEM_SIZE_IN_32B(aeq)))

#define SSS_AEQ_WQ_NAME					"sss_eqs"

#define SSS_AEQ_NAME					"sss_aeq"

#define SSS_AEQ_TO_INFO(eq)				\
			container_of((eq) - (eq)->qid, struct sss_aeq_info, aeq[0])

#define SSS_AEQ_DMA_ATTR_DEF	0

enum sss_aeq_cb_state {
	SSS_AEQ_HW_CB_REG = 0,
	SSS_AEQ_HW_CB_RUNNING,
	SSS_AEQ_SW_CB_REG,
	SSS_AEQ_SW_CB_RUNNING,
};

static u32 aeq_depth = SSS_DEF_AEQ_DEPTH;
module_param(aeq_depth, uint, 0444);
MODULE_PARM_DESC(aeq_depth,
		 "aeq depth, valid range is " __stringify(SSS_MIN_AEQ_DEPTH)
		 " - " __stringify(SSS_MAX_AEQ_DEPTH));

static void sss_chip_set_aeq_intr(struct sss_eq *aeq)
{
	u32 val;
	struct sss_hwif *hwif = SSS_TO_HWDEV(aeq)->hwif;

	val = sss_chip_read_reg(hwif, SSS_CSR_AEQ_CTRL_0_ADDR);

	val = SSS_CLEAR_AEQ_CTRL_0(val, INTR_ID) &
	      SSS_CLEAR_AEQ_CTRL_0(val, DMA_ATTR) &
	      SSS_CLEAR_AEQ_CTRL_0(val, PCI_INTF_ID) &
	      SSS_CLEAR_AEQ_CTRL_0(val, INTR_MODE);

	val |= SSS_SET_AEQ_CTRL_0(SSS_EQ_IRQ_ID(aeq), INTR_ID) |
	       SSS_SET_AEQ_CTRL_0(SSS_AEQ_DMA_ATTR_DEF, DMA_ATTR) |
	       SSS_SET_AEQ_CTRL_0(SSS_GET_HWIF_PCI_INTF_ID(hwif), PCI_INTF_ID) |
	       SSS_SET_AEQ_CTRL_0(SSS_INTR_MODE_ARMED, INTR_MODE);

	sss_chip_write_reg(hwif, SSS_CSR_AEQ_CTRL_0_ADDR, val);
}

static void sss_chip_set_aeq_size(struct sss_eq *aeq)
{
	u32 val;
	struct sss_hwif *hwif = SSS_TO_HWDEV(aeq)->hwif;

	val = SSS_SET_AEQ_CTRL_1(aeq->len, SIZE) |
	      SSS_SET_AEQ_CTRL_1(SSS_SET_EQ_HW_E_SIZE(aeq), ELEM_SIZE) |
	      SSS_SET_AEQ_CTRL_1(SSS_SET_EQ_HW_PAGE_SIZE(aeq), PAGE_SIZE);

	sss_chip_write_reg(hwif, SSS_CSR_AEQ_CTRL_1_ADDR, val);
}

static u32 sss_chip_init_aeq_attr(void *aeq)
{
	sss_chip_set_aeq_intr(aeq);
	sss_chip_set_aeq_size(aeq);

	return 0;
}

static void sss_init_aeqe_desc(void *data)
{
	u32 i;
	u32 init_val;
	struct sss_aeq_elem *aeqe = NULL;
	struct sss_eq *aeq = (struct sss_eq *)data;

	init_val = cpu_to_be32(SSS_EQ_WRAPPED(aeq));
	for (i = 0; i < aeq->len; i++) {
		aeqe = SSS_GET_AEQ_ELEM(aeq, i);
		aeqe->desc = init_val;
	}

	/* write all aeq desc */
	wmb();
}

static irqreturn_t sss_aeq_intr_handle(int irq, void *data)
{
	struct sss_eq *aeq = (struct sss_eq *)data;
	struct sss_aeq_info *aeq_info = SSS_AEQ_TO_INFO(aeq);

	sss_chip_clear_msix_resend_bit(aeq->hwdev, SSS_EQ_IRQ_ID(aeq),
				       SSS_EQ_MSIX_RESEND_TIMER_CLEAR);

	queue_work_on(WORK_CPU_UNBOUND, aeq_info->workq, &aeq->aeq_work);

	return IRQ_HANDLED;
}

static void sss_aeq_event_handle(struct sss_eq *aeq, u32 desc)
{
	u32 size;
	u32 event;
	u8 data[SSS_AEQE_DATA_SIZE];
	enum sss_aeq_hw_event hw_event;
	enum sss_aeq_sw_event sw_event;
	struct sss_aeq_info *aeq_info = SSS_AEQ_TO_INFO(aeq);
	struct sss_aeq_elem *aeqe;

	aeqe = SSS_GET_CUR_AEQ_ELEM(aeq);
	hw_event = SSS_GET_EQE_DESC(desc, TYPE);
	SSS_TO_HWDEV(aeq)->aeq_stat.cur_recv_cnt++;

	if (SSS_GET_EQE_DESC(desc, SRC)) {
		event = hw_event;
		sw_event = SSS_GET_AEQ_SW_EVENT(event);

		memcpy(data, aeqe->aeqe_data, SSS_AEQE_DATA_SIZE);
		sss_be32_to_cpu(data, SSS_AEQE_DATA_SIZE);
		set_bit(SSS_AEQ_SW_CB_RUNNING, &aeq_info->sw_event_handler_state[sw_event]);

		if (aeq_info->sw_event_handler[sw_event] &&
		    test_bit(SSS_AEQ_SW_CB_REG, &aeq_info->sw_event_handler_state[sw_event]))
			aeq_info->sw_event_handler[sw_event](aeq_info->sw_event_data[sw_event],
					hw_event, data);

		clear_bit(SSS_AEQ_SW_CB_RUNNING, &aeq_info->sw_event_handler_state[sw_event]);

		return;
	}

	if (hw_event < SSS_AEQ_EVENT_MAX) {
		memcpy(data, aeqe->aeqe_data, SSS_AEQE_DATA_SIZE);
		sss_be32_to_cpu(data, SSS_AEQE_DATA_SIZE);

		size = SSS_GET_EQE_DESC(desc, SIZE);
		set_bit(SSS_AEQ_HW_CB_RUNNING, &aeq_info->hw_event_handler_state[hw_event]);

		if (aeq_info->hw_event_handler[hw_event] &&
		    test_bit(SSS_AEQ_HW_CB_REG, &aeq_info->hw_event_handler_state[hw_event]))
			aeq_info->hw_event_handler[hw_event](aeq_info->hw_event_data[hw_event],
					data, size);

		clear_bit(SSS_AEQ_HW_CB_RUNNING, &aeq_info->hw_event_handler_state[hw_event]);

		return;
	}
	sdk_warn(SSS_TO_HWDEV(aeq)->dev_hdl, "Unknown aeq event %d\n", hw_event);
}

static bool sss_aeq_irq_handle(struct sss_eq *aeq)
{
	struct sss_aeq_elem *elem = NULL;
	u32 desc;
	u32 i;
	u32 eqe_cnt = 0;

	for (i = 0; i < SSS_TASK_PROCESS_EQE_LIMIT; i++) {
		elem = SSS_GET_CUR_AEQ_ELEM(aeq);

		/* Data in HW is in Big endian Format */
		desc = be32_to_cpu(elem->desc);

		/* HW updates wrap bit, when it adds eq element event */
		if (SSS_GET_EQE_DESC(desc, WRAPPED) == aeq->wrap)
			return false;

		dma_rmb();

		sss_aeq_event_handle(aeq, desc);

		sss_increase_eq_ci(aeq);

		if (++eqe_cnt >= SSS_EQ_UPDATE_CI_STEP) {
			eqe_cnt = 0;
			sss_chip_set_eq_ci(aeq, SSS_EQ_NOT_ARMED);
		}
	}

	return true;
}

static void sss_aeq_irq_work(struct work_struct *work)
{
	bool unfinish;
	struct sss_eq *aeq = container_of(work, struct sss_eq, aeq_work);
	struct sss_aeq_info *aeq_info = SSS_AEQ_TO_INFO(aeq);

	unfinish = sss_aeq_irq_handle(aeq);
	sss_chip_set_eq_ci(aeq, SSS_EQ_ARM_STATE(unfinish));

	if (unfinish)
		queue_work_on(WORK_CPU_UNBOUND, aeq_info->workq, &aeq->aeq_work);
}

static void sss_init_aeq_para(struct sss_eq *aeq, u16 qid)
{
	aeq->init_desc_handler = sss_init_aeqe_desc;
	aeq->init_attr_handler = sss_chip_init_aeq_attr;
	aeq->irq_handler = sss_aeq_intr_handle;
	aeq->name = SSS_AEQ_NAME;
	INIT_WORK(&aeq->aeq_work, sss_aeq_irq_work);

	aeq->qid = qid;
	aeq->len = aeq_depth;
	aeq->type = SSS_AEQ;
	aeq->entry_size = SSS_AEQE_SIZE;
}

static int sss_init_aeq(struct sss_hwdev *hwdev,
			u16 aeq_num, struct sss_irq_desc *irq)
{
	u16 i;
	u16 qid;
	int ret;
	struct sss_aeq_info *aeq_info = NULL;

	aeq_info = kzalloc(sizeof(*aeq_info), GFP_KERNEL);
	if (!aeq_info)
		return -ENOMEM;

	hwdev->aeq_info = aeq_info;
	aeq_info->hwdev = hwdev;
	aeq_info->num = aeq_num;

	aeq_info->workq = alloc_workqueue(SSS_AEQ_WQ_NAME, WQ_MEM_RECLAIM, SSS_MAX_AEQ);
	if (!aeq_info->workq) {
		ret = -ENOMEM;
		sdk_err(hwdev->dev_hdl, "Fail to alloc aeq workqueue\n");
		goto alloc_workq_err;
	}

	if (aeq_depth < SSS_MIN_AEQ_DEPTH || aeq_depth > SSS_MAX_AEQ_DEPTH) {
		sdk_warn(hwdev->dev_hdl, "Invalid aeq_depth value %u, adjust to %d\n",
			 aeq_depth, SSS_DEF_AEQ_DEPTH);
		aeq_depth = SSS_DEF_AEQ_DEPTH;
	}

	for (qid = 0; qid < aeq_num; qid++) {
		sss_init_aeq_para(&aeq_info->aeq[qid], qid);
		ret = sss_init_eq(hwdev, &aeq_info->aeq[qid], &irq[qid]);
		if (ret != 0) {
			sdk_err(hwdev->dev_hdl, "Fail to init aeq %u\n", qid);
			goto init_aeq_err;
		}
	}

	for (qid = 0; qid < aeq_num; qid++)
		sss_chip_set_msix_state(hwdev, irq[qid].msix_id, SSS_MSIX_ENABLE);

	return 0;

init_aeq_err:
	for (i = 0; i < qid; i++)
		sss_deinit_eq(&aeq_info->aeq[i]);

	destroy_workqueue(aeq_info->workq);

alloc_workq_err:
	kfree(aeq_info);
	hwdev->aeq_info = NULL;

	return ret;
}

void sss_deinit_aeq(struct sss_hwdev *hwdev)
{
	struct sss_aeq_info *aeq_info = hwdev->aeq_info;
	enum sss_aeq_hw_event aeq_event;
	enum sss_aeq_sw_event sw_aeq_event;
	u16 qid;

	for (qid = 0; qid < aeq_info->num; qid++)
		sss_deinit_eq(&aeq_info->aeq[qid]);

	for (sw_aeq_event = SSS_STL_EVENT;
	     sw_aeq_event < SSS_AEQ_SW_EVENT_MAX; sw_aeq_event++)
		sss_aeq_unregister_swe_cb(hwdev, sw_aeq_event);

	for (aeq_event = SSS_HW_FROM_INT;
	     aeq_event < SSS_AEQ_EVENT_MAX; aeq_event++)
		sss_aeq_unregister_hw_cb(hwdev, aeq_event);

	destroy_workqueue(aeq_info->workq);

	kfree(aeq_info);
	hwdev->aeq_info = NULL;
}

void sss_get_aeq_irq(struct sss_hwdev *hwdev,
		     struct sss_irq_desc *irq_array, u16 *irq_num)
{
	struct sss_aeq_info *aeq_info = hwdev->aeq_info;
	u16 qid;

	for (qid = 0; qid < aeq_info->num; qid++) {
		irq_array[qid].irq_id = aeq_info->aeq[qid].irq_desc.irq_id;
		irq_array[qid].msix_id =
			aeq_info->aeq[qid].irq_desc.msix_id;
	}

	*irq_num = aeq_info->num;
}

void sss_dump_aeq_info(struct sss_hwdev *hwdev)
{
	struct sss_aeq_elem *aeqe = NULL;
	struct sss_eq *aeq = NULL;
	u32 addr;
	u32 ci;
	u32 pi;
	u32 ctrl0;
	u32 id;
	int qid;

	for (qid = 0; qid < hwdev->aeq_info->num; qid++) {
		aeq = &hwdev->aeq_info->aeq[qid];
		/* Indirect access should set qid first */
		sss_chip_write_reg(SSS_TO_HWDEV(aeq)->hwif,
				   SSS_EQ_INDIR_ID_ADDR(aeq->type), aeq->qid);
		wmb(); /* make sure set qid firstly */

		addr = SSS_CSR_AEQ_CTRL_0_ADDR;
		ctrl0 = sss_chip_read_reg(hwdev->hwif, addr);
		id = sss_chip_read_reg(hwdev->hwif, SSS_EQ_INDIR_ID_ADDR(aeq->type));

		addr = SSS_EQ_CI_REG_ADDR(aeq);
		ci = sss_chip_read_reg(hwdev->hwif, addr);
		addr = SSS_EQ_PI_REG_ADDR(aeq);
		pi = sss_chip_read_reg(hwdev->hwif, addr);
		aeqe = SSS_GET_CUR_AEQ_ELEM(aeq);
		sdk_err(hwdev->dev_hdl,
			"Aeq id: %d, id: %u, ctrl0: 0x%08x, ci: 0x%08x, pi: 0x%x, work_state: 0x%x, wrap: %u, desc: 0x%x swci:0x%x\n",
			qid, id, ctrl0, ci, pi, work_busy(&aeq->aeq_work),
			aeq->wrap, be32_to_cpu(aeqe->desc), aeq->ci);
	}

	sss_dump_chip_err_info(hwdev);
}

int sss_aeq_register_hw_cb(void *hwdev, void *pri_handle,
			   enum sss_aeq_hw_event event, sss_aeq_hw_event_handler_t event_handler)
{
	struct sss_aeq_info *aeq_info = NULL;

	if (!hwdev || !event_handler || event >= SSS_AEQ_EVENT_MAX)
		return -EINVAL;

	aeq_info = SSS_TO_AEQ_INFO(hwdev);
	aeq_info->hw_event_handler[event] = event_handler;
	aeq_info->hw_event_data[event] = pri_handle;
	set_bit(SSS_AEQ_HW_CB_REG, &aeq_info->hw_event_handler_state[event]);

	return 0;
}

void sss_aeq_unregister_hw_cb(void *hwdev, enum sss_aeq_hw_event event)
{
	struct sss_aeq_info *aeq_info = NULL;

	if (!hwdev || event >= SSS_AEQ_EVENT_MAX)
		return;

	aeq_info = SSS_TO_AEQ_INFO(hwdev);
	clear_bit(SSS_AEQ_HW_CB_REG, &aeq_info->hw_event_handler_state[event]);
	while (test_bit(SSS_AEQ_HW_CB_RUNNING, &aeq_info->hw_event_handler_state[event]))
		usleep_range(SSS_EQ_USLEEP_LOW_LIMIT, SSS_EQ_USLEEP_HIG_LIMIT);
	aeq_info->hw_event_handler[event] = NULL;
}

int sss_aeq_register_swe_cb(void *hwdev, void *pri_handle,
			    enum sss_aeq_sw_event event,
			    sss_aeq_sw_event_handler_t sw_event_handler)
{
	struct sss_aeq_info *aeq_info = NULL;

	if (!hwdev || !sw_event_handler || event >= SSS_AEQ_SW_EVENT_MAX)
		return -EINVAL;

	aeq_info = SSS_TO_AEQ_INFO(hwdev);
	aeq_info->sw_event_handler[event] = sw_event_handler;
	aeq_info->sw_event_data[event] = pri_handle;
	set_bit(SSS_AEQ_SW_CB_REG, &aeq_info->sw_event_handler_state[event]);

	return 0;
}

void sss_aeq_unregister_swe_cb(void *hwdev, enum sss_aeq_sw_event event)
{
	struct sss_aeq_info *aeq_info = NULL;

	if (!hwdev || event >= SSS_AEQ_SW_EVENT_MAX)
		return;

	aeq_info = SSS_TO_AEQ_INFO(hwdev);
	clear_bit(SSS_AEQ_SW_CB_REG, &aeq_info->sw_event_handler_state[event]);
	while (test_bit(SSS_AEQ_SW_CB_RUNNING,
			&aeq_info->sw_event_handler_state[event]))
		usleep_range(SSS_EQ_USLEEP_LOW_LIMIT, SSS_EQ_USLEEP_HIG_LIMIT);
	aeq_info->sw_event_handler[event] = NULL;
}

int sss_hwif_init_aeq(struct sss_hwdev *hwdev)
{
	u16 i;
	u16 aeq_num;
	u16 act_num = 0;
	int ret;
	struct sss_irq_desc irq_array[SSS_MAX_AEQ] = {0};

	aeq_num = SSS_GET_HWIF_AEQ_NUM(hwdev->hwif);
	if (aeq_num > SSS_MAX_AEQ) {
		sdk_warn(hwdev->dev_hdl, "Adjust aeq_num to %d\n", SSS_MAX_AEQ);
		aeq_num = SSS_MAX_AEQ;
	}

	act_num = sss_alloc_irq(hwdev, SSS_SERVICE_TYPE_INTF, irq_array, aeq_num);
	if (act_num == 0) {
		sdk_err(hwdev->dev_hdl, "Fail to alloc irq, aeq_num: %u\n", aeq_num);
		return -ENOMEM;
	}

	if (act_num < aeq_num) {
		sdk_warn(hwdev->dev_hdl, "Adjust aeq_num to %u\n", act_num);
		aeq_num = act_num;
	}

	ret = sss_init_aeq(hwdev, aeq_num, irq_array);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init aeq\n");
		goto init_aeqs_err;
	}

	return 0;

init_aeqs_err:
	for (i = 0; i < aeq_num; i++)
		sss_free_irq(hwdev, SSS_SERVICE_TYPE_INTF, irq_array[i].irq_id);

	return ret;
}

void sss_hwif_deinit_aeq(struct sss_hwdev *hwdev)
{
	u16 i;
	u16 irq_num;
	struct sss_irq_desc irq_array[SSS_MAX_AEQ] = {0};

	sss_get_aeq_irq(hwdev, irq_array, &irq_num);

	sss_deinit_aeq(hwdev);

	for (i = 0; i < irq_num; i++)
		sss_free_irq(hwdev, SSS_SERVICE_TYPE_INTF, irq_array[i].irq_id);
}

int sss_init_aeq_msix_attr(struct sss_hwdev *hwdev)
{
	int i;
	int ret;
	struct sss_aeq_info *aeq_info = hwdev->aeq_info;
	struct sss_irq_cfg intr_info = {0};

	sss_init_eq_intr_info(&intr_info);

	for (i = aeq_info->num - 1; i >= 0; i--) {
		intr_info.msix_id = SSS_EQ_IRQ_ID(&aeq_info->aeq[i]);
		ret = sss_chip_set_eq_msix_attr(hwdev, &intr_info, SSS_CHANNEL_COMM);
		if (ret != 0) {
			sdk_err(hwdev->dev_hdl, "Fail to set msix attr for aeq %d\n", i);
			return -EFAULT;
		}
	}

	return 0;
}

u8 sss_sw_aeqe_handler(void *dev, u8 aeq_event, u8 *data)
{
	struct sss_hwdev *hwdev = (struct sss_hwdev *)dev;

	if (!hwdev)
		return 0;

	sdk_err(hwdev->dev_hdl, "Received ucode aeq event, type: 0x%x, data: 0x%llx\n",
		aeq_event, *((u64 *)data));

	if (aeq_event < SSS_ERR_MAX)
		atomic_inc(&hwdev->hw_stats.nic_ucode_event_stats[aeq_event]);

	return 0;
}

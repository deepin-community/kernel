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
#include "sss_hw_ceq.h"
#include "sss_hw_export.h"
#include "sss_hwif_ceq.h"
#include "sss_hw_common.h"
#include "sss_hwif_eq.h"
#include "sss_hwif_api.h"
#include "sss_hwif_export.h"

#define SSS_DEF_CEQ_DEPTH					8192

#define SSS_CEQ_NAME						"sss_ceq"

#define SSS_CEQ_CTRL_0_INTR_ID_SHIFT		0
#define SSS_CEQ_CTRL_0_DMA_ATTR_SHIFT		12
#define SSS_CEQ_CTRL_0_LIMIT_KICK_SHIFT		20
#define SSS_CEQ_CTRL_0_PCI_INTF_ID_SHIFT	24
#define SSS_CEQ_CTRL_0_PAGE_SIZE_SHIFT		27
#define SSS_CEQ_CTRL_0_INTR_MODE_SHIFT		31

#define SSS_CEQ_CTRL_0_INTR_ID_MASK			0x3FFU
#define SSS_CEQ_CTRL_0_DMA_ATTR_MASK		0x3FU
#define SSS_CEQ_CTRL_0_LIMIT_KICK_MASK		0xFU
#define SSS_CEQ_CTRL_0_PCI_INTF_ID_MASK		0x3U
#define SSS_CEQ_CTRL_0_PAGE_SIZE_MASK		0xF
#define SSS_CEQ_CTRL_0_INTR_MODE_MASK		0x1U

#define SSS_SET_CEQ_CTRL_0(val, member)		\
			(((val) & SSS_CEQ_CTRL_0_##member##_MASK) << \
			SSS_CEQ_CTRL_0_##member##_SHIFT)

#define SSS_CEQ_CTRL_1_LEN_SHIFT			0
#define SSS_CEQ_CTRL_1_GLB_FUNC_ID_SHIFT	20

#define SSS_CEQ_CTRL_1_LEN_MASK				0xFFFFFU
#define SSS_CEQ_CTRL_1_GLB_FUNC_ID_MASK		0xFFFU

#define SSS_SET_CEQ_CTRL_1(val, member)	\
			(((val) & SSS_CEQ_CTRL_1_##member##_MASK) << \
			SSS_CEQ_CTRL_1_##member##_SHIFT)

#define SSS_CEQ_DMA_ATTR_DEF				0

#define SSS_MIN_CEQ_DEPTH					64
#define SSS_MAX_CEQ_DEPTH					\
	((SSS_MAX_EQ_PAGE_SIZE / SSS_CEQE_SIZE) * SSS_CEQ_MAX_PAGE)

#define SSS_GET_CEQ_ELEM(ceq, id) ((u32 *)SSS_GET_EQ_ELEM((ceq), (id)))

#define SSS_GET_CUR_CEQ_ELEM(ceq) SSS_GET_CEQ_ELEM((ceq), (ceq)->ci)

#define SSS_CEQE_TYPE_SHIFT			23
#define SSS_CEQE_TYPE_MASK			0x7

#define SSS_CEQE_TYPE(type)                                                    \
	(((type) >> SSS_CEQE_TYPE_SHIFT) & SSS_CEQE_TYPE_MASK)

#define SSS_CEQE_DATA_MASK			0x3FFFFFF
#define SSS_CEQE_DATA(data)			((data) & SSS_CEQE_DATA_MASK)

#define SSS_CEQ_TO_INFO(eq)			\
		container_of((eq) - (eq)->qid, struct sss_ceq_info, ceq[0])

#define CEQ_LMT_KICK_DEF			0

enum sss_ceq_cb_state {
	SSS_CEQ_CB_REG = 0,
	SSS_CEQ_CB_RUNNING,
};

static u32 ceq_depth = SSS_DEF_CEQ_DEPTH;
module_param(ceq_depth, uint, 0444);
MODULE_PARM_DESC(ceq_depth,
		 "ceq depth, valid range is " __stringify(SSS_MIN_CEQ_DEPTH)
		 " - " __stringify(SSS_MAX_CEQ_DEPTH));

static u32 tasklet_depth = SSS_TASK_PROCESS_EQE_LIMIT;
module_param(tasklet_depth, uint, 0444);
MODULE_PARM_DESC(tasklet_depth,
		 "The max number of ceqe can be processed in tasklet, default = 1024");

void sss_init_ceqe_desc(void *data)
{
	u32 i;
	u32 init_val;
	u32 *ceqe = NULL;
	struct sss_eq *ceq = (struct sss_eq *)data;

	init_val = cpu_to_be32(SSS_EQ_WRAPPED(ceq));
	for (i = 0; i < ceq->len; i++) {
		ceqe = SSS_GET_CEQ_ELEM(ceq, i);
		*(ceqe) = init_val;
	}

	/* write all ceq desc */
	wmb();
}

static u32 sss_chip_init_ceq_attr(void *data)
{
	u32 val;
	u32 len;
	struct sss_eq *ceq = (struct sss_eq *)data;
	struct sss_hwif *hwif = SSS_TO_HWDEV(ceq)->hwif;

	val = SSS_SET_CEQ_CTRL_0(SSS_EQ_IRQ_ID(ceq), INTR_ID) |
	      SSS_SET_CEQ_CTRL_0(SSS_CEQ_DMA_ATTR_DEF, DMA_ATTR)	|
	      SSS_SET_CEQ_CTRL_0(CEQ_LMT_KICK_DEF, LIMIT_KICK) |
	      SSS_SET_CEQ_CTRL_0(SSS_GET_HWIF_PCI_INTF_ID(hwif), PCI_INTF_ID) |
	      SSS_SET_CEQ_CTRL_0(SSS_SET_EQ_HW_PAGE_SIZE(ceq), PAGE_SIZE) |
	      SSS_SET_CEQ_CTRL_0(SSS_INTR_MODE_ARMED, INTR_MODE);
	len = SSS_SET_CEQ_CTRL_1(ceq->len, LEN);

	return sss_chip_set_ceq_attr(SSS_TO_HWDEV(ceq), ceq->qid, val, len);
}

irqreturn_t sss_ceq_intr_handle(int irq, void *data)
{
	struct sss_eq *ceq = (struct sss_eq *)data;

	ceq->hw_intr_jiffies = jiffies;

	sss_chip_clear_msix_resend_bit(ceq->hwdev, SSS_EQ_IRQ_ID(ceq),
				       SSS_EQ_MSIX_RESEND_TIMER_CLEAR);

	tasklet_schedule(&ceq->ceq_tasklet);

	return IRQ_HANDLED;
}

static void sss_ceqe_handler(struct sss_eq *ceq, u32 ceqe)
{
	u32 ceqe_data = SSS_CEQE_DATA(ceqe);
	enum sss_ceq_event ceq_event = SSS_CEQE_TYPE(ceqe);
	struct sss_ceq_info *ceq_info = SSS_CEQ_TO_INFO(ceq);

	if (ceq_event >= SSS_CEQ_EVENT_MAX) {
		sdk_err(SSS_TO_HWDEV(ceq)->dev_hdl, "Unknown ceq_event:%d, ceqe_data: 0x%x\n",
			ceq_event, ceqe_data);
		return;
	}

	set_bit(SSS_CEQ_CB_RUNNING, &ceq_info->event_handler_state[ceq_event]);

	if (ceq_info->event_handler[ceq_event] &&
	    test_bit(SSS_CEQ_CB_REG, &ceq_info->event_handler_state[ceq_event]))
		ceq_info->event_handler[ceq_event](ceq_info->event_handler_data[ceq_event],
						   ceqe_data);

	clear_bit(SSS_CEQ_CB_RUNNING, &ceq_info->event_handler_state[ceq_event]);
}

static bool sss_ceq_irq_handle(struct sss_eq *ceq)
{
	u32 elem;
	u32 eqe_cnt = 0;
	u32 i;

	for (i = 0; i < tasklet_depth; i++) {
		elem = *(SSS_GET_CUR_CEQ_ELEM(ceq));
		elem = be32_to_cpu(elem);

		/* HW updates wrap bit, when it adds eq element event */
		if (SSS_GET_EQE_DESC(elem, WRAPPED) == ceq->wrap)
			return false;

		sss_ceqe_handler(ceq, elem);

		sss_increase_eq_ci(ceq);

		if (++eqe_cnt >= SSS_EQ_UPDATE_CI_STEP) {
			eqe_cnt = 0;
			sss_chip_set_eq_ci(ceq, SSS_EQ_NOT_ARMED);
		}
	}

	return true;
}

static void sss_ceq_tasklet(ulong ceq_data)
{
	bool unfinish;
	struct sss_eq *ceq = (struct sss_eq *)ceq_data;

	ceq->sw_intr_jiffies = jiffies;
	unfinish = sss_ceq_irq_handle(ceq);
	sss_chip_set_eq_ci(ceq, SSS_EQ_ARM_STATE(unfinish));

	if (unfinish)
		tasklet_schedule(&ceq->ceq_tasklet);
}

static void sss_init_ceq_para(struct sss_eq *ceq, u16 qid)
{
	ceq->init_desc_handler = sss_init_ceqe_desc;
	ceq->init_attr_handler = sss_chip_init_ceq_attr;
	ceq->irq_handler = sss_ceq_intr_handle;
	ceq->name = SSS_CEQ_NAME;
	tasklet_init(&ceq->ceq_tasklet, sss_ceq_tasklet, (ulong)ceq);

	ceq->qid = qid;
	ceq->len = ceq_depth;
	ceq->type = SSS_CEQ;
	ceq->entry_size = SSS_CEQE_SIZE;
}

static int sss_init_ceq(struct sss_hwdev *hwdev,
			struct sss_irq_desc *irq_array, u16 irq_num)
{
	u16 i;
	u16 qid;
	int ret;
	struct sss_ceq_info *ceq_info = NULL;

	ceq_info = kzalloc(sizeof(*ceq_info), GFP_KERNEL);
	if (!ceq_info)
		return -ENOMEM;

	ceq_info->hwdev = hwdev;
	ceq_info->num = irq_num;
	hwdev->ceq_info = ceq_info;

	if (tasklet_depth == 0) {
		sdk_warn(hwdev->dev_hdl,
			 "Invalid tasklet_depth can not be zero, adjust to %d\n",
			 SSS_TASK_PROCESS_EQE_LIMIT);
		tasklet_depth = SSS_TASK_PROCESS_EQE_LIMIT;
	}

	if (ceq_depth < SSS_MIN_CEQ_DEPTH || ceq_depth > SSS_MAX_CEQ_DEPTH) {
		sdk_warn(hwdev->dev_hdl,
			 "Invalid ceq_depth %u out of range, adjust to %d\n",
			 ceq_depth, SSS_DEF_CEQ_DEPTH);
		ceq_depth = SSS_DEF_CEQ_DEPTH;
	}

	for (qid = 0; qid < irq_num; qid++) {
		sss_init_ceq_para(&ceq_info->ceq[qid], qid);
		ret = sss_init_eq(hwdev, &ceq_info->ceq[qid], &irq_array[qid]);
		if (ret != 0) {
			sdk_err(hwdev->dev_hdl, "Fail to init ceq %u\n", qid);
			goto init_ceq_err;
		}
	}

	for (qid = 0; qid < irq_num; qid++)
		sss_chip_set_msix_state(hwdev, irq_array[qid].msix_id, SSS_MSIX_ENABLE);

	return 0;

init_ceq_err:
	for (i = 0; i < qid; i++)
		sss_deinit_eq(&ceq_info->ceq[i]);

	kfree(ceq_info);
	hwdev->ceq_info = NULL;

	return ret;
}

static void sss_get_ceq_irq(struct sss_hwdev *hwdev, struct sss_irq_desc *irq,
			    u16 *irq_num)
{
	u16 i;
	struct sss_ceq_info *ceq_info = hwdev->ceq_info;

	for (i = 0; i < ceq_info->num; i++) {
		irq[i].msix_id = ceq_info->ceq[i].irq_desc.msix_id;
		irq[i].irq_id = ceq_info->ceq[i].irq_desc.irq_id;
	}

	*irq_num = ceq_info->num;
}

int sss_hwif_init_ceq(struct sss_hwdev *hwdev)
{
	u16 i;
	u16 ceq_num;
	u16 act_num = 0;
	int ret;
	struct sss_irq_desc irq_desc[SSS_MAX_CEQ] = {0};

	ceq_num = SSS_GET_HWIF_CEQ_NUM(hwdev->hwif);
	if (ceq_num > SSS_MAX_CEQ) {
		sdk_warn(hwdev->dev_hdl, "Adjust ceq num to %d\n", SSS_MAX_CEQ);
		ceq_num = SSS_MAX_CEQ;
	}

	act_num = sss_alloc_irq(hwdev, SSS_SERVICE_TYPE_INTF, irq_desc, ceq_num);
	if (act_num == 0) {
		sdk_err(hwdev->dev_hdl, "Fail to alloc irq, ceq_num: %u\n", ceq_num);
		return -EINVAL;
	}

	if (act_num < ceq_num) {
		sdk_warn(hwdev->dev_hdl, "Adjust ceq num to %u\n", act_num);
		ceq_num = act_num;
	}

	ret = sss_init_ceq(hwdev, irq_desc, ceq_num);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init ceq, ret:%d\n", ret);
		goto init_ceq_err;
	}

	return 0;

init_ceq_err:
	for (i = 0; i < act_num; i++)
		sss_free_irq(hwdev, SSS_SERVICE_TYPE_INTF, irq_desc[i].irq_id);

	return ret;
}

static void sss_deinit_ceq(struct sss_hwdev *hwdev)
{
	u16 i;
	struct sss_ceq_info *ceq_info = hwdev->ceq_info;
	enum sss_ceq_event event;

	for (i = 0; i < ceq_info->num; i++)
		sss_deinit_eq(&ceq_info->ceq[i]);

	for (event = SSS_NIC_CTRLQ; event < SSS_CEQ_EVENT_MAX; event++)
		sss_ceq_unregister_cb(hwdev, event);

	kfree(ceq_info);
	hwdev->ceq_info = NULL;
}

void sss_hwif_deinit_ceq(struct sss_hwdev *hwdev)
{
	int i;
	u16 irq_num = 0;
	struct sss_irq_desc irq[SSS_MAX_CEQ] = {0};

	sss_get_ceq_irq(hwdev, irq, &irq_num);

	sss_deinit_ceq(hwdev);

	for (i = 0; i < irq_num; i++)
		sss_free_irq(hwdev, SSS_SERVICE_TYPE_INTF, irq[i].irq_id);
}

void sss_dump_ceq_info(struct sss_hwdev *hwdev)
{
	struct sss_eq *ceq_info = NULL;
	u32 addr;
	u32 ci;
	u32 pi;
	int qid;

	for (qid = 0; qid < hwdev->ceq_info->num; qid++) {
		ceq_info = &hwdev->ceq_info->ceq[qid];
		/* Indirect access should set qid first */
		sss_chip_write_reg(SSS_TO_HWDEV(ceq_info)->hwif,
				   SSS_EQ_INDIR_ID_ADDR(ceq_info->type), ceq_info->qid);
		wmb(); /* make sure set qid firstly */

		addr = SSS_EQ_CI_REG_ADDR(ceq_info);
		ci = sss_chip_read_reg(hwdev->hwif, addr);
		addr = SSS_EQ_PI_REG_ADDR(ceq_info);
		pi = sss_chip_read_reg(hwdev->hwif, addr);
		sdk_err(hwdev->dev_hdl,
			"Ceq id: %d, ci: 0x%08x, sw_ci: 0x%08x, pi: 0x%x, tasklet_state: 0x%lx, wrap: %u, ceqe: 0x%x\n",
			qid, ci, ceq_info->ci, pi, tasklet_state(&ceq_info->ceq_tasklet),
			ceq_info->wrap, be32_to_cpu(*(SSS_GET_CUR_CEQ_ELEM(ceq_info))));

		sdk_err(hwdev->dev_hdl, "Ceq last response hard interrupt time: %u\n",
			jiffies_to_msecs(jiffies - ceq_info->hw_intr_jiffies));
		sdk_err(hwdev->dev_hdl, "Ceq last response soft interrupt time: %u\n",
			jiffies_to_msecs(jiffies - ceq_info->sw_intr_jiffies));
	}

	sss_dump_chip_err_info(hwdev);
}

int sss_ceq_register_cb(void *hwdev, void *data,
			enum sss_ceq_event ceq_event, sss_ceq_event_handler_t event_handler)
{
	struct sss_ceq_info *ceq_info = NULL;

	if (!hwdev || ceq_event >= SSS_CEQ_EVENT_MAX)
		return -EINVAL;

	ceq_info = SSS_TO_CEQ_INFO(hwdev);
	ceq_info->event_handler_data[ceq_event] = data;
	ceq_info->event_handler[ceq_event] = event_handler;
	set_bit(SSS_CEQ_CB_REG, &ceq_info->event_handler_state[ceq_event]);

	return 0;
}

void sss_ceq_unregister_cb(void *hwdev, enum sss_ceq_event ceq_event)
{
	struct sss_ceq_info *ceq_info = NULL;

	if (!hwdev || ceq_event >= SSS_CEQ_EVENT_MAX)
		return;

	ceq_info = SSS_TO_CEQ_INFO(hwdev);
	clear_bit(SSS_CEQ_CB_REG, &ceq_info->event_handler_state[ceq_event]);
	while (test_bit(SSS_CEQ_CB_RUNNING,
			&ceq_info->event_handler_state[ceq_event]))
		usleep_range(SSS_EQ_USLEEP_LOW_LIMIT, SSS_EQ_USLEEP_HIG_LIMIT);
	ceq_info->event_handler[ceq_event] = NULL;
}

int sss_init_ceq_msix_attr(struct sss_hwdev *hwdev)
{
	u16 i;
	int ret;
	struct sss_ceq_info *ceq_info = hwdev->ceq_info;
	struct sss_irq_cfg intr_info = {0};

	sss_init_eq_intr_info(&intr_info);

	for (i = 0; i < ceq_info->num; i++) {
		intr_info.msix_id = SSS_EQ_IRQ_ID(&ceq_info->ceq[i]);
		ret = sss_chip_set_msix_attr(hwdev, intr_info, SSS_CHANNEL_COMM);
		if (ret != 0) {
			sdk_err(hwdev->dev_hdl, "Fail to set msix attr for ceq %u\n", i);
			return -EFAULT;
		}
	}

	return 0;
}

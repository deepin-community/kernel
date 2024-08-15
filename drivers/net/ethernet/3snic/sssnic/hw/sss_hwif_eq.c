// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/spinlock.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_common.h"
#include "sss_hwdev.h"
#include "sss_hwif_api.h"
#include "sss_csr.h"
#include "sss_hwif_eq.h"

#define SSS_EQ_CI_SIMPLE_INDIR_CI_SHIFT			0
#define SSS_EQ_CI_SIMPLE_INDIR_ARMED_SHIFT		21
#define SSS_EQ_CI_SIMPLE_INDIR_AEQ_ID_SHIFT		30
#define SSS_EQ_CI_SIMPLE_INDIR_CEQ_ID_SHIFT		24

#define SSS_EQ_CI_SIMPLE_INDIR_CI_MASK			0x1FFFFFU
#define SSS_EQ_CI_SIMPLE_INDIR_ARMED_MASK		0x1U
#define SSS_EQ_CI_SIMPLE_INDIR_AEQ_ID_MASK		0x3U
#define SSS_EQ_CI_SIMPLE_INDIR_CEQ_ID_MASK		0xFFU

#define SSS_SET_EQ_CI_SIMPLE_INDIR(val, member)		\
			(((val) & SSS_EQ_CI_SIMPLE_INDIR_##member##_MASK) << \
			SSS_EQ_CI_SIMPLE_INDIR_##member##_SHIFT)

#define SSS_EQ_WRAPPED_SHIFT	20

#define SSS_EQ_CI(eq)	((eq)->ci | \
			((u32)(eq)->wrap << SSS_EQ_WRAPPED_SHIFT))

#define SSS_EQ_CI_SIMPLE_INDIR_REG_ADDR(eq)	\
			(((eq)->type == SSS_AEQ) ? \
			SSS_CSR_AEQ_CI_SIMPLE_INDIR_ADDR : \
			SSS_CSR_CEQ_CI_SIMPLE_INDIR_ADDR)

#define SSS_EQ_HI_PHYS_ADDR_REG(type, pg_num)	\
			((u32)((type == SSS_AEQ) ? \
			SSS_AEQ_PHY_HI_ADDR_REG(pg_num) : \
			SSS_CEQ_PHY_HI_ADDR_REG(pg_num)))

#define SSS_EQ_LO_PHYS_ADDR_REG(type, pg_num)	\
			((u32)((type == SSS_AEQ) ? \
			SSS_AEQ_PHY_LO_ADDR_REG(pg_num) : \
			SSS_CEQ_PHY_LO_ADDR_REG(pg_num)))

#define SSS_GET_EQ_PAGES_NUM(eq, size)	\
			((u16)(ALIGN((u32)((eq)->len * (eq)->entry_size), \
				(size)) / (size)))

#define SSS_GET_EQ_MAX_PAGES(eq)		\
			((eq)->type == SSS_AEQ ? SSS_AEQ_MAX_PAGE : \
			SSS_CEQ_MAX_PAGE)

#define SSS_GET_EQE_NUM(eq, pg_size)	((pg_size) / (u32)(eq)->entry_size)

#define SSS_EQE_NUM_IS_ALIGN(eq)	((eq)->num_entry_per_pg & ((eq)->num_entry_per_pg - 1))

void sss_chip_set_eq_ci(struct sss_eq *eq, u32 arm_state)
{
	u32 val;

	if (eq->qid != 0 && SSS_TO_HWDEV(eq)->poll)
		arm_state = SSS_EQ_NOT_ARMED;

	val = SSS_SET_EQ_CI_SIMPLE_INDIR(arm_state, ARMED) |
	      SSS_SET_EQ_CI_SIMPLE_INDIR(SSS_EQ_CI(eq), CI);

	if (eq->type == SSS_AEQ)
		val |= SSS_SET_EQ_CI_SIMPLE_INDIR(eq->qid, AEQ_ID);
	else
		val |= SSS_SET_EQ_CI_SIMPLE_INDIR(eq->qid, CEQ_ID);

	sss_chip_write_reg(SSS_TO_HWDEV(eq)->hwif, SSS_EQ_CI_SIMPLE_INDIR_REG_ADDR(eq), val);
}

static void sss_chip_set_eq_page_addr(struct sss_eq *eq,
				      u16 page_id, struct sss_dma_addr_align *dma_addr)
{
	u32 addr;

	addr = SSS_EQ_HI_PHYS_ADDR_REG(eq->type, page_id);
	sss_chip_write_reg(SSS_TO_HWDEV(eq)->hwif, addr,
			   upper_32_bits(dma_addr->align_paddr));

	addr = SSS_EQ_LO_PHYS_ADDR_REG(eq->type, page_id);
	sss_chip_write_reg(SSS_TO_HWDEV(eq)->hwif, addr,
			   lower_32_bits(dma_addr->align_paddr));
}

static int sss_chip_init_eq_attr(struct sss_eq *eq)
{
	u32 i;
	int ret;

	for (i = 0; i < eq->page_num; i++)
		sss_chip_set_eq_page_addr(eq, i, &eq->page_array[i]);

	ret = eq->init_attr_handler(eq);
	if (ret != 0)
		return ret;

	sss_chip_set_eq_ci(eq, SSS_EQ_ARMED);

	return 0;
}

static u32 sss_init_eqe_desc(struct sss_eq *eq)
{
	eq->num_entry_per_pg = SSS_GET_EQE_NUM(eq, eq->page_size);
	if (SSS_EQE_NUM_IS_ALIGN(eq)) {
		sdk_err(SSS_TO_HWDEV(eq)->dev_hdl, "Number element in eq page is not align\n");
		return -EINVAL;
	}

	eq->init_desc_handler(eq);

	return 0;
}

static int sss_alloc_eq_dma_page(struct sss_eq *eq, u16 id)
{
	int ret;

	ret = sss_dma_zalloc_coherent_align(SSS_TO_HWDEV(eq)->dev_hdl, eq->page_size,
					    SSS_MIN_EQ_PAGE_SIZE, GFP_KERNEL, &eq->page_array[id]);
	if (ret != 0) {
		sdk_err(SSS_TO_HWDEV(eq)->dev_hdl, "Alloc eq page fail, pg index: %u\n", id);
		return ret;
	}

	return 0;
}

static void sss_free_eq_dma_page(struct sss_eq *eq, u16 max_id)
{
	int i;

	for (i = 0; i < max_id; i++)
		sss_dma_free_coherent_align(SSS_TO_DEV(eq->hwdev), &eq->page_array[i]);
}

static int sss_alloc_eq_page(struct sss_eq *eq)
{
	u16 page_id;
	int ret;
	struct sss_hwdev *hwdev = SSS_TO_HWDEV(eq);

	eq->page_array = kcalloc(eq->page_num, sizeof(*eq->page_array), GFP_KERNEL);
	if (!eq->page_array)
		return -ENOMEM;

	for (page_id = 0; page_id < eq->page_num; page_id++) {
		ret = sss_alloc_eq_dma_page(eq, page_id);
		if (ret != 0)
			goto alloc_dma_err;
	}

	ret = sss_init_eqe_desc(eq);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init eqe\n");
		goto alloc_dma_err;
	}

	return 0;

alloc_dma_err:
	sss_free_eq_dma_page(eq, page_id);
	kfree(eq->page_array);
	eq->page_array = NULL;

	return ret;
}

static void sss_free_eq_page(struct sss_eq *eq)
{
	u16 i;
	struct sss_hwdev *hwdev = SSS_TO_HWDEV(eq);

	for (i = 0; i < eq->page_num; i++)
		sss_dma_free_coherent_align(hwdev->dev_hdl, &eq->page_array[i]);

	kfree(eq->page_array);
	eq->page_array = NULL;
}

static inline u32 sss_get_eq_page_size(const struct sss_eq *eq)
{
	u32 total_size;
	u32 count;

	total_size = ALIGN((eq->len * eq->entry_size),
			   SSS_MIN_EQ_PAGE_SIZE);
	if (total_size <= (SSS_GET_EQ_MAX_PAGES(eq) * SSS_MIN_EQ_PAGE_SIZE))
		return SSS_MIN_EQ_PAGE_SIZE;

	count = (u32)(ALIGN((total_size / SSS_GET_EQ_MAX_PAGES(eq)),
			    SSS_MIN_EQ_PAGE_SIZE) / SSS_MIN_EQ_PAGE_SIZE);

	/* round up to nearest power of two */
	count = 1U << (u8)fls((int)(count - 1));

	return ((u32)SSS_MIN_EQ_PAGE_SIZE) * count;
}

static int sss_request_eq_irq(struct sss_eq *eq, struct sss_irq_desc *entry)
{
	struct pci_dev *pdev = SSS_TO_HWDEV(eq)->pcidev_hdl;

	snprintf(eq->irq_name, sizeof(eq->irq_name), "%s%u@pci:%s",
		 eq->name, eq->qid, pci_name(pdev));

	return request_irq(entry->irq_id, eq->irq_handler, 0UL, eq->irq_name, eq);
}

static void sss_chip_reset_eq(struct sss_eq *eq)
{
	struct sss_hwdev *hwdev = eq->hwdev;
	struct sss_hwif	*hwif = hwdev->hwif;

	sss_chip_write_reg(hwif, SSS_EQ_INDIR_ID_ADDR(eq->type), eq->qid);

	/* make sure set qid firstly*/
	wmb();

	if (eq->type == SSS_AEQ)
		sss_chip_write_reg(hwif, SSS_CSR_AEQ_CTRL_1_ADDR, 0);
	else
		sss_chip_set_ceq_attr(hwdev, eq->qid, 0, 0);

	/* make sure write ctrl reg secondly */
	wmb();

	sss_chip_write_reg(hwif, SSS_EQ_PI_REG_ADDR(eq), 0);
}

static int sss_init_eq_page_size(struct sss_eq *eq)
{
	eq->page_size = sss_get_eq_page_size(eq);
	eq->old_page_size = eq->page_size;
	eq->page_num = SSS_GET_EQ_PAGES_NUM(eq, eq->page_size);

	if (eq->page_num > SSS_GET_EQ_MAX_PAGES(eq)) {
		sdk_err(SSS_TO_HWDEV(eq)->dev_hdl, "Number pages: %u too many pages for eq\n",
			eq->page_num);
		return -EINVAL;
	}

	return 0;
}

void sss_increase_eq_ci(struct sss_eq *eq)
{
	if (!eq)
		return;

	eq->ci++;

	if (eq->ci == eq->len) {
		eq->ci = 0;
		eq->wrap = !eq->wrap;
	}
}

int sss_init_eq(struct sss_hwdev *hwdev, struct sss_eq *eq,
		struct sss_irq_desc *entry)
{
	int ret = 0;

	eq->hwdev = hwdev;
	eq->irq_desc.irq_id = entry->irq_id;
	eq->irq_desc.msix_id = entry->msix_id;

	ret = sss_init_eq_page_size(eq);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init eq params\n");
		return ret;
	}

	ret = sss_alloc_eq_page(eq);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to alloc eq page\n");
		return ret;
	}

	sss_chip_reset_eq(eq);

	ret = sss_chip_init_eq_attr(eq);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init eq attr\n");
		goto out;
	}

	ret = sss_request_eq_irq(eq, entry);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to request eq irq, err: %d\n", ret);
		goto out;
	}

	sss_chip_set_msix_state(hwdev, SSS_EQ_IRQ_ID(eq), SSS_MSIX_DISABLE);

	return 0;

out:
	sss_free_eq_page(eq);
	return ret;
}

void sss_deinit_eq(struct sss_eq *eq)
{
	struct sss_irq_desc *irq = &eq->irq_desc;

	sss_chip_set_msix_state(SSS_TO_HWDEV(eq), SSS_EQ_IRQ_ID(eq), SSS_MSIX_DISABLE);

	synchronize_irq(irq->irq_id);

	free_irq(irq->irq_id, eq);

	sss_chip_write_reg(SSS_TO_HWDEV(eq)->hwif, SSS_EQ_INDIR_ID_ADDR(eq->type), eq->qid);

	/* make sure disable msix */
	wmb();

	if (eq->type == SSS_AEQ) {
		cancel_work_sync(&eq->aeq_work);
		sss_chip_write_reg(SSS_TO_HWDEV(eq)->hwif, SSS_CSR_AEQ_CTRL_1_ADDR, 0);
	} else {
		tasklet_kill(&eq->ceq_tasklet);
		sss_chip_set_ceq_attr(SSS_TO_HWDEV(eq), eq->qid, 0, 0);
	}

	eq->ci = sss_chip_read_reg(SSS_TO_HWDEV(eq)->hwif, SSS_EQ_PI_REG_ADDR(eq));
	sss_chip_set_eq_ci(eq, SSS_EQ_NOT_ARMED);

	sss_free_eq_page(eq);
}

void sss_init_eq_intr_info(struct sss_irq_cfg *intr_info)
{
	intr_info->coalesc_intr_set = SSS_EQ_INTR_COALESC;
	intr_info->coalesc_timer = SSS_EQ_INTR_COALESC_TIMER_CFG;
	intr_info->resend_timer = SSS_EQ_INTR_RESEND_TIMER_CFG;
}

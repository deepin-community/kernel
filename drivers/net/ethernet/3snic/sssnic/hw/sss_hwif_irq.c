// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/mutex.h>

#include "sss_kernel.h"
#include "sss_hwdev.h"
#include "sss_hw_svc_cap.h"

#define SSS_GET_NEED_IRQ_NUM(hwif, intr_num) \
	(SSS_GET_HWIF_MSIX_EN(hwif) ? (SSS_GET_HWIF_AEQ_NUM(hwif) + \
	SSS_GET_HWIF_CEQ_NUM(hwif) + (hwif)->attr.sq_num) : (intr_num))

#define SSS_MIN_VECTOR	2

static int sss_alloc_irq_info(struct sss_hwdev *hwdev)
{
	u16 total_num = SSS_GET_HWIF_IRQ_NUM(hwdev->hwif);
	u16 need_num = SSS_GET_NEED_IRQ_NUM(hwdev->hwif, total_num);
	struct sss_mgmt_info *mgmt_info = hwdev->mgmt_info;
	struct sss_irq_info *irq_info = &mgmt_info->irq_info;

	if (total_num == 0) {
		sdk_err(hwdev->dev_hdl, "Mgmt irq info: intr total_num = 0, msix_flex_en %d\n",
			SSS_GET_HWIF_MSIX_EN(hwdev->hwif));
		return -EFAULT;
	}

	if (need_num > total_num) {
		sdk_warn(hwdev->dev_hdl, "Mgmt irq info: intr total_num %d < need_num %d, msix_flex_en %d\n",
			 total_num, need_num, SSS_GET_HWIF_MSIX_EN(hwdev->hwif));
		need_num = total_num;
	}

	irq_info->irq = kcalloc(total_num, sizeof(*irq_info->irq), GFP_KERNEL);
	if (!irq_info->irq)
		return -ENOMEM;

	irq_info->max_num = need_num;

	return 0;
}

static void sss_free_irq_info(struct sss_hwdev *hwdev)
{
	kfree(hwdev->mgmt_info->irq_info.irq);
	hwdev->mgmt_info->irq_info.irq = NULL;
}

int sss_init_irq_info(struct sss_hwdev *hwdev)
{
	u16 i = 0;
	u16 irq_num;
	int enable_irq_num;
	int ret;
	struct sss_mgmt_info *mgmt_info = hwdev->mgmt_info;
	struct sss_irq *irq = NULL;
	struct msix_entry *entry = NULL;

	ret = sss_alloc_irq_info(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to alloc irq info, err: %d\n", ret);
		return ret;
	}

	irq_num = mgmt_info->irq_info.max_num;
	entry = kcalloc(irq_num, sizeof(*entry), GFP_KERNEL);
	if (!entry) {
		sss_free_irq_info(hwdev);
		return -ENOMEM;
	}

	for (i = 0; i < irq_num; i++)
		entry[i].entry = i;

	enable_irq_num = pci_enable_msix_range(hwdev->pcidev_hdl, entry,
					       SSS_MIN_VECTOR, irq_num);
	if (enable_irq_num < 0) {
		kfree(entry);
		sss_free_irq_info(hwdev);
		sdk_err(hwdev->dev_hdl, "Fail to alloc msix entries with threshold 2. enabled_irq: %d\n",
			enable_irq_num);
		return -ENOMEM;
	}

	irq_num = (u16)enable_irq_num;
	mgmt_info->irq_info.total_num = irq_num;
	mgmt_info->irq_info.free_num = irq_num;
	mgmt_info->svc_cap.intr_type = SSS_INTR_TYPE_MSIX;

	irq = mgmt_info->irq_info.irq;
	for (i = 0; i < irq_num; i++) {
		irq[i].desc.msix_id = entry[i].entry;
		irq[i].desc.irq_id = entry[i].vector;
		irq[i].type = SSS_SERVICE_TYPE_MAX;
		irq[i].busy = SSS_CFG_FREE;
	}

	mutex_init(&mgmt_info->irq_info.irq_mutex);

	sdk_info(hwdev->dev_hdl, "Success to request %u msix vector.\n", irq_num);
	kfree(entry);

	return 0;
}

void sss_deinit_irq_info(struct sss_hwdev *hwdev)
{
	struct sss_service_cap *svc_cap = &hwdev->mgmt_info->svc_cap;
	struct sss_irq_info *irq_info = &hwdev->mgmt_info->irq_info;

	if (irq_info->free_num != irq_info->total_num)
		sdk_err(hwdev->dev_hdl, "Fail to reclaim all irq and eq, please check\n");

	if (svc_cap->intr_type == SSS_INTR_TYPE_MSIX)
		pci_disable_msix(hwdev->pcidev_hdl);
	else if (svc_cap->intr_type == SSS_INTR_TYPE_MSI)
		pci_disable_msi(hwdev->pcidev_hdl);

	sss_free_irq_info(hwdev);
}

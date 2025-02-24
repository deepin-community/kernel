// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023-2024 Montage Technology. All rights reserved.
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>

#include "tsse_ipc.h"
#include "tsse_ipc_drv.h"
#include "tsse_ipc_setup.h"
#include "tsse_ipc_msg.h"
#include "tsse_ipc_service.h"
#include "tsse_ipc_hash.h"
#include "tsse_ipc_epid.h"
#ifndef DISABLE_FW
#include "tsse_fw_service.h"
#endif
#include "tsse_dev.h"

/**
 * ipc_hw_init()- Enable main2host interrupt, cleanup interrupt
 * set value in host2main and main2host.
 * @hw_ipc: pointer to a structure used for IPC
 */
static void ipc_hw_init(struct tsse_ipc *hw_ipc)
{
	writel(0x1, hw_ipc->virt_addr + MAIN2HOST_INTR_ENABLE_OFFSET);
	writel(0x0, hw_ipc->virt_addr + HOST2MAIN_INTR_SET_OFFSET);
	writel(0x0, hw_ipc->virt_addr + MAIN2HOST_INTR_SET_OFFSET);
}

static void tsse_ipc_bh_handler(unsigned long data)
{
	struct tsse_ipc *tsseipc = (struct tsse_ipc *)data;
	void __iomem *d2h_msg = tsseipc->virt_addr + MAIN2HOST_IPC_OFFSET;
	int ret = ipc_d2h_msg_dispatch(tsseipc, d2h_msg);

	if (ret)
		dev_err(tsseipc->dev, "%s: device message callback result: %d\n",
			__func__, ret);
}

static irqreturn_t tsse_ipc_d2h_irqhandler(int irq, void *dev_id)
{
	struct tsse_ipc *tsseipc = (struct tsse_ipc *)dev_id;

	writel(0x0, tsseipc->virt_addr + MAIN2HOST_INTR_SET_OFFSET);
	tasklet_schedule(&tsseipc->ipc_handle);
	dev_err(tsseipc->dev, "irq%d\n", irq);
	return IRQ_HANDLED;
}

#ifndef DISABLE_FW
static int host_init_msg(int handle)
{
	uint32_t cmd = IPC_BASIC_CMD_HOST_INIT;

	return ipc_h2d_msg_send_legacy(handle, IPC_MESSAGE_BASIC, &cmd, sizeof(uint32_t));
}
#endif

int tsse_ipc_init(struct pci_dev *pdev)
{
	struct tsse_dev *tdev = pci_to_tsse_dev(pdev);
	struct tsse_ipc *ipc;
	int rc;

	ipc = devm_kzalloc(&pdev->dev, sizeof(*ipc), GFP_KERNEL);
	if (ipc == NULL)
		return -ENOMEM;
	tdev->ipc = ipc;
	ipc->pdev = pdev;
	ipc->dev = &pdev->dev;
	ipc->virt_addr = TSSE_DEV_BARS(tdev)[2].virt_addr;
	ipc->im_inited = 0;

	mutex_init(&ipc->list_lock);
	tasklet_init(&(ipc->ipc_handle), tsse_ipc_bh_handler, (ulong)(ipc));

	rc = request_threaded_irq(pci_irq_vector(pdev, 0), NULL,
				  tsse_ipc_d2h_irqhandler, IRQF_SHARED,
				  "pf-ipc", ipc);
	if (rc) {
		dev_err(&pdev->dev, "request_threaded_irq failed: %d\n", rc);
		return rc;
	}
	ipc_hw_init(ipc);
#ifndef DISABLE_FW
	ipc->d2h_handlers[IPC_MESSAGE_BOOT] = fw_service;
	rc = host_init_msg(tdev->id);
	if (rc) {
		dev_err(&pdev->dev, "host_init_msg failed: %d\n", rc);
		tsse_ipc_deinit(tdev);
		return rc;
	}
#endif
	return rc;
}

void tsse_ipc_deinit(void *tdev_t)
{
	struct tsse_ipc *tsseipc;
	struct pci_dev *pdev;
	struct tsse_dev *tdev;

	tdev = tdev_t;
	tsseipc = tdev->ipc;
	pdev = tsseipc->pdev;
	if (tsseipc) {
		tsse_im_shutdown_for_dev(tdev);
		free_irq(pci_irq_vector(pdev, 0), tdev->ipc);
		tdev->ipc = NULL;
	}
	tsse_service_info_hash_remove_all();
	tsse_service_handle_hash_remove_all();
}

#ifndef DISABLE_FW
int tsse_fw_manual_load_ipc(struct pci_dev *pdev)
{
	struct tsse_dev *tdev = pci_to_tsse_dev(pdev);
	struct tsse_ipc *ipc = tdev->ipc;
	int rc = -EFAULT;

	if (ipc) {
		rc = host_init_msg(tdev->id);
		if (rc)
			dev_err(&pdev->dev, "host_init_msg failed: %d\n", rc);
	}
	return rc;
}
#endif

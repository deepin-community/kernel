// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023 Montage Technology. All rights reserved.
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include "tsse_ipc.h"
#include "tsse_dev.h"
#include "tsse_service.h"

/**
 * get_msginf() - Create ipc_msg and read message from BAR.
 * Return the pointer to ipc_msg, the caller is responsible for free it.
 * @d2h: device2host memory pointer
 * Return: new ipc_msg pointer, which points to message read from device
*/
static struct ipc_msg *get_msginf(void __iomem *d2h)
{
	uint32_t u_len = 0;
	struct ipc_msg *msg = NULL;

	uint8_t *device_msg_data = NULL;
	struct ipc_header *ipc_info = (struct ipc_header *)d2h;

	// The memory layout in d2h should at least contains:
	// ipc_header, msg_info and fw_load (message body)
	if (ipc_info->i_len < sizeof(struct ipc_header) +
		sizeof(struct msg_info) + sizeof(struct fw_load)) {
		pr_info("%s(): msg format error\n", __func__);
		return NULL;
	}
	u_len = ipc_info->i_len - sizeof(struct ipc_header);
	msg = (struct ipc_msg *)(kzalloc(sizeof(struct ipc_msg) + u_len,
					      GFP_ATOMIC));
	if (!msg) {
		pr_info("%s(): ipc_msg kzalloc failed\n", __func__);
		return NULL;
	}

	msg->header.inst_id = ipc_info->inst_id;
	msg->header.tgid = ipc_info->tgid;
	msg->header.i_len = ipc_info->i_len;

	device_msg_data = (uint8_t *)(d2h + sizeof(struct ipc_header));
	memcpy_fromio((uint8_t *)msg->i_data, device_msg_data, u_len);

	return msg;
}

static irqreturn_t tsse_ipc_d2h_irqhandler(int irq, void *dev_id)
{
	struct tsse_ipc *tsseipc = (struct tsse_ipc *)dev_id;

	writel(0x0, tsseipc->virt_addr + MAIN2HOST_INTR_SET_OFFSET);
	tasklet_hi_schedule(&tsseipc->ipc_handle);
	dev_err(tsseipc->dev, "irq%d\n", irq);
	return IRQ_HANDLED;
}

bool check_send_enbit(struct tsse_ipc *tsseipc)
{
	u32 int_reg;

	int_reg = readl(tsseipc->virt_addr + HOST2MAIN_INTR_SET_OFFSET);
	if ((int_reg & IPC_REGISTER_INT_SET) == 0)
		return true;
	else
		return false;
}

void notify_device(struct tsse_ipc *tsseipc)
{
	writel(0x1, tsseipc->virt_addr + HOST2MAIN_INTR_SET_OFFSET);
	return;

}

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

static int ipc_init_msg(struct tsse_ipc *tsseipc)
{
	u8 *h2d;
	u32 int_reg;
	u32 cmd_len;
	u32 i_len;
	struct ipc_msg *msg;
	struct msg_info *info_msg;

	cmd_len = sizeof(uint32_t);
	i_len = sizeof(struct ipc_header) + sizeof(struct msg_info) + cmd_len;
	msg = (struct ipc_msg *)(kzalloc(i_len, GFP_ATOMIC));

	if (!msg) {
		pr_info("%s(): msg kzalloc failed\n", __func__);
		return -EFAULT;
	}
	msg->header.i_len = i_len;
	info_msg = (struct msg_info *)msg->i_data;
	info_msg->msg_class = IPC_MESSAGE_BASIC;
	*(uint32_t *)((uint8_t *)msg->i_data + sizeof(struct msg_info)) = IPC_BASIC_CMD_HOST_INIT;

	mutex_lock(&tsseipc->list_lock);
	int_reg = readl(tsseipc->virt_addr + HOST2MAIN_INTR_SET_OFFSET);
	if ((int_reg & IPC_REGISTER_INT_SET) != 0) {
		mutex_unlock(&tsseipc->list_lock);
		kfree(msg);
		return -EFAULT;
	}
	h2d = (u8 *)(tsseipc->virt_addr + HOST2MAIN_IPC_OFFSET);

	memcpy_toio(h2d, msg, sizeof(struct ipc_header));
	memcpy_toio(h2d + sizeof(struct ipc_header), (u8 *)msg->i_data,
		    sizeof(struct msg_info) + sizeof(uint32_t));

	writel(0x1, tsseipc->virt_addr + HOST2MAIN_INTR_SET_OFFSET);
	mutex_unlock(&tsseipc->list_lock);
	kfree(msg);

	return 0;
}

static void tsse_ipc_bh_handler(unsigned long data)
{
	struct tsse_ipc *tsseipc = (struct tsse_ipc *)data;

	void __iomem *d2h_payload = tsseipc->virt_addr + MAIN2HOST_IPC_OFFSET;
	struct ipc_msg *msg = get_msginf(d2h_payload);

	if (!msg) {
		dev_err(tsseipc->dev, "get_msginf is NULL\n");
		return;
	}
	if (service_rout(tsseipc, msg))
		dev_err(tsseipc->dev, "illegal message class\n");
	kfree(msg);
}

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

	mutex_init(&ipc->list_lock);
	tasklet_init(&(ipc->ipc_handle), tsse_ipc_bh_handler,
		     (ulong)(ipc));

	rc = request_threaded_irq(pci_irq_vector(pdev, 0), NULL,
				  tsse_ipc_d2h_irqhandler, IRQF_SHARED,
				  "pf-ipc", ipc);
	if (rc) {
		dev_err(&pdev->dev, "request_threaded_irq failed\n");
		return rc;
	}
	ipc_hw_init(ipc);
	rc = ipc_init_msg(ipc);
	if (rc) {
		dev_err(&pdev->dev, "ipc_init_msg failed\n");
		tsse_ipc_deinit(tdev);
	}
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
		free_irq(pci_irq_vector(pdev, 0), tdev->ipc);
		tdev->ipc = NULL;
	}
}

int tsse_fw_manual_load_ipc(struct pci_dev *pdev)
{
	struct tsse_dev *tdev = pci_to_tsse_dev(pdev);
	struct tsse_ipc *ipc = tdev->ipc;
	int rc = -EFAULT;

	if (ipc) {
		ipc_hw_init(ipc);
		rc = ipc_init_msg(ipc);
		if (rc)
			dev_err(&pdev->dev, "ipc_init_msg failed\n");
	}
	return rc;
}

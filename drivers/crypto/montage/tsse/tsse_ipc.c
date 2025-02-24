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

#include "tsse_ipc.h"
#include "tsse_ipc_setup.h"
#include "tsse_ipc_drv.h"
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
	// ipc_header, msg_info
	if (ipc_info->i_len < sizeof(struct ipc_header) +
		sizeof(struct msg_info)) {
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

/**
 * tsse_write_msg() - do write msg from host to device
 * @tsseipc: pointer to structure used for IPC in current device
 * @msg_class: type for the IPC message
 * @msg_payload: pointer to actual content that caller wants to send
 * @payload_length: length of msg_payload
 * Return: 0 on success, error code otherwise
 */
static int tsse_write_msg(struct tsse_ipc *tsseipc, uint32_t msg_class,
						void *msg_payload, uint32_t payload_length)
{
	u8 *h2d;
	u32 int_reg;
	u32 comm_msg_length;
	struct ipc_msg *msg;
	struct msg_info *info_msg;

	comm_msg_length = sizeof(struct ipc_header) + sizeof(struct msg_info);
	msg = (struct ipc_msg *)(kzalloc(comm_msg_length, GFP_ATOMIC));

	if (!msg) {
		pr_info("%s(): msg kzalloc failed\n", __func__);
		return -ENOMEM;
	}
	msg->header.i_len = comm_msg_length + payload_length;
	info_msg = (struct msg_info *)msg->i_data;
	info_msg->msg_class = msg_class;

	mutex_lock(&tsseipc->list_lock);
	int_reg = readl(tsseipc->virt_addr + HOST2MAIN_INTR_SET_OFFSET);
	if ((int_reg & IPC_REGISTER_INT_SET) != 0) {
		mutex_unlock(&tsseipc->list_lock);
		kfree(msg);
		return -EAGAIN;
	}
	h2d = (u8 *)(tsseipc->virt_addr + HOST2MAIN_IPC_OFFSET);

	ipc_memcpy_to_io(h2d, (u8 *)msg, comm_msg_length);
	ipc_memcpy_to_io(h2d + comm_msg_length, (u8 *)msg_payload, payload_length);

	writel(0x1, tsseipc->virt_addr + HOST2MAIN_INTR_SET_OFFSET);
	mutex_unlock(&tsseipc->list_lock);
	kfree(msg);

	return 0;
}

int ipc_d2h_legacy_msg_process(struct tsse_ipc *tsseipc, void __iomem *d2h_msg)
{
	int ret;
	struct ipc_msg *msg = get_msginf(d2h_msg);

	if (!msg) {
		dev_err(tsseipc->dev, "get_msginf is NULL\n");
	}
	ret = service_rout(tsseipc, msg);
	kfree(msg);
	return ret;
}

/**
 * ipc_h2d_msg_send_legacy() - send message from host to device
 * @handle: handle to TSSE device
 * @msg_class: type for the IPC message
 * @msg_payload: pointer to actual content that caller wants to send
 * @length: length of msg_payload
 * Return: 0 on success, error code otherwise
 */
int ipc_h2d_msg_send_legacy(int handle, uint32_t msg_class,
			void *msg_payload, uint32_t length)
{
	struct tsse_dev *tdev;
	struct tsse_ipc *tsseipc;
	tsse_d2h_ipc_handler ipc_handler;

	tdev = tsse_get_dev_by_handle(handle);
	if (!tdev)
		return -ENODEV;

	if (!msg_payload || !length) {
		pr_err("%s %d: invalid msg payload\n", __func__, __LINE__);
		return -EINVAL;
	}
	tsseipc = tdev->ipc;
	ipc_handler = tsseipc->d2h_handlers[msg_class];
	if ((msg_class >= IPC_MESSAGE_CLASS_NUM) ||
		(msg_class != IPC_MESSAGE_BASIC && !ipc_handler)) {
		pr_err("%s %d: invalid msg class\n", __func__, __LINE__);
		return -EINVAL;
	}
	return tsse_write_msg(tsseipc, msg_class, msg_payload, length);
}

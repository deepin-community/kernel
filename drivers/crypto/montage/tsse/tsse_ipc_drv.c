// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023-2024 Montage Technology. All rights reserved.
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>

#include "tsse_ipc.h"
#include "tsse_ipc_drv.h"
#include "tsse_ipc_service.h"
#include "tsse_ipc_epid.h"
#include "tsse_dev.h"

#define ALIGN_TO_4_BYTES(length) (((length) + 3) & ~0x3)

static int ipc_d2h_new_msg_process(struct tsse_ipc *tsseipc, void __iomem *d2h_msg)
{
	struct tsse_ipc_msg *ipc_msg = (struct tsse_ipc_msg *)d2h_msg;
	void *payload;
	u32 msg_len;
	u32 header_len;
	u32 payload_len;
	int ret;
	u64 epid;

	msg_len = ipc_msg->msg_len;
	header_len = sizeof(struct tsse_ipc_msg);
	payload_len = msg_len - header_len;
	epid = ipc_msg->epid;

	if (msg_len < header_len || msg_len > IPC_MAX_DATA_LEN) {
		pr_err("%s %d: invalid msg len: %u in resp\n", __func__, __LINE__, msg_len);
		return -EINVAL;
	}
	payload = kzalloc(payload_len, GFP_ATOMIC);
	if (!payload)
		return -ENOMEM;
	memcpy_fromio(payload, (u8 *)d2h_msg + header_len, payload_len);
	if (ipc_msg->type == TSSE_IPC_TYPE_RING_SETUP_RSP)
		ret = ipc_ring_setup_resp_receive(payload, payload_len);
	else
		ret = tsse_service_msg_receive(epid, payload, payload_len);
	kfree(payload);
	return ret;
}

static struct tsse_ipc_msg *ipc_h2d_msg_header_create(u64 epid, u32 payload_length)
{
	struct tsse_ipc_msg *header = (struct tsse_ipc_msg *)(
		kzalloc(sizeof(struct tsse_ipc_msg), GFP_ATOMIC));
	if (header) {
		if (GET_SERVICE_ID(epid) == EPID_MANAGE_SERVICE_ID) {
			if (GET_APP_SPECIFIC_ID(epid) == TSSE_IPC_SPECIFIC_RING_SETUP_REQ)
				header->type = TSSE_IPC_TYPE_RING_SETUP_REQ;
			else if (GET_APP_SPECIFIC_ID(epid) == TSSE_IPC_SPECIFIC_RING_SETUP_RSP)
				header->type = TSSE_IPC_TYPE_RING_SETUP_RSP;
			else
				header->type = TSSE_IPC_TYPE_SERVICE;
		} else {
			header->type = TSSE_IPC_TYPE_SERVICE;
		}
		header->msg_len = sizeof(struct tsse_ipc_msg) + payload_length;
		header->rev = 0;
		header->epid = epid;
	}
	return header;
}

int ipc_h2d_msg_send(int device_handle, u64 epid, void *msg_payload, u32 length)
{
	struct tsse_dev *tdev;
	struct tsse_ipc *tsseipc;
	struct tsse_ipc_msg *header;
	u8 *h2d;
	u32 int_reg;
	u32 header_size;

	tdev = tsse_get_dev_by_handle(device_handle);
	if (!tdev)
		return -ENODEV;

	if (!msg_payload || !length) {
		pr_err("%s %d: invalid msg payload\n", __func__, __LINE__);
		return -EINVAL;
	}
	header_size = sizeof(struct tsse_ipc_msg);
	if (length + header_size > IPC_MAX_DATA_LEN) {
		pr_err("%s %d length too large: %u\n", __func__, __LINE__, length);
		return -EINVAL;
	}
	tsseipc = tdev->ipc;
	mutex_lock(&tsseipc->list_lock);
	int_reg = readl(tsseipc->virt_addr + HOST2MAIN_INTR_SET_OFFSET);
	if ((int_reg & IPC_REGISTER_INT_SET) != 0) {
		mutex_unlock(&tsseipc->list_lock);
		return -EAGAIN;
	}
	header = ipc_h2d_msg_header_create(epid, length);
	if (!header) {
		mutex_unlock(&tsseipc->list_lock);
		pr_err("%s(): msg header kzalloc failed\n", __func__);
		return -ENOMEM;
	}
	h2d = (u8 *)(tsseipc->virt_addr + HOST2MAIN_IPC_OFFSET);
	ipc_memcpy_to_io(h2d, (u8 *)header, header_size);
	ipc_memcpy_to_io(h2d + header_size, msg_payload, length);

	writel(0x1, tsseipc->virt_addr + HOST2MAIN_INTR_SET_OFFSET);
	mutex_unlock(&tsseipc->list_lock);
	kfree(header);
	return 0;
}

int ipc_d2h_msg_dispatch(struct tsse_ipc *tsseipc, void __iomem *d2h_msg)
{
	u16 type = (u16) cpu_to_le32(readl(d2h_msg));

	switch (type) {
	case TSSE_IPC_TYPE_LEGACY:
		return ipc_d2h_legacy_msg_process(tsseipc, d2h_msg);
	case TSSE_IPC_TYPE_SERVICE:
	case TSSE_IPC_TYPE_RING_SETUP_RSP:
		return ipc_d2h_new_msg_process(tsseipc, d2h_msg);
	default:
		pr_err("%s %d: invalid msg type: %u\n", __func__, __LINE__, type);
		return -EINVAL;
	}
}

void ipc_memcpy_to_io(u8 *addr, u8 *src, u32 len)
{
	memcpy_toio(addr, src, len);
}

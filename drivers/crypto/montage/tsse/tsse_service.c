// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023-2024 Montage Technology. All rights reserved.
 */
#include <linux/errno.h>
#include "tsse_service.h"
#include "tsse_dev.h"

int service_rout(struct tsse_ipc *tsseipc, struct ipc_msg *msg)
{
	struct msg_info *info;
	struct tsse_dev *tdev;
	tsse_d2h_ipc_handler d2h_handler;
	void *payload;
	uint32_t msg_class;
	uint32_t comm_msg_length;
	uint32_t payload_length;
	int ret;

	info = (struct msg_info *)msg->i_data;
	msg_class = info->msg_class;
	d2h_handler = tsseipc->d2h_handlers[msg_class];

	if (!d2h_handler) {
		dev_err(tsseipc->dev, "%s %d: no d2h handler for msg [%u]\n",
			__func__, __LINE__, msg_class);
		return -EFAULT;
	}
	tdev = pci_to_tsse_dev(tsseipc->pdev);
	if (!tdev) {
		dev_err(tsseipc->dev, "%s %d: no related dev info for ipc\n",
			__func__, __LINE__);
		return -EFAULT;
	}
	comm_msg_length = sizeof(struct ipc_header) + sizeof(struct msg_info);
	payload = (void *) ((uint8_t *)msg + comm_msg_length);
	payload_length = msg->header.i_len - comm_msg_length;

	ret = d2h_handler(tdev->id, payload, payload_length);
	return ret;
}

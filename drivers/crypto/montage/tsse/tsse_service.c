// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023 Montage Technology. All rights reserved.
 */
#include <linux/errno.h>
#include "tsse_service.h"

int service_rout(struct tsse_ipc *tsseipc, struct ipc_msg *msg)
{
	struct msg_info *info;
	uint32_t msg_class;
	int ret = 0;

	info = (struct msg_info *)msg->i_data;
	msg_class = info->msg_class;
	switch (msg_class) {
	case IPC_MESSAGE_BOOT:
		fw_service(tsseipc, msg);
		break;

	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

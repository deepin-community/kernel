/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023-2024 Montage Technology. All rights reserved.
 */

#ifndef __TM_HOST_IPC_H__
#define __TM_HOST_IPC_H__

#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include "tsse_ipc_setup.h"

struct ipc_header {
	uint32_t inst_id;
	pid_t tgid;
	uint32_t i_len;
	uint32_t pasid : 20;
	uint32_t reserved_1 : 4;
	uint32_t pasid_en : 8;

	uint32_t reserved[2];
};

struct ipc_msg {
	struct ipc_header header;
	uint32_t i_data[];
};

struct fw_load {
	uint32_t command;
	uint32_t result;
	uint8_t name[32];
	uint32_t offset;
	uint32_t size;
};

struct msg_info {
	uint32_t host_id;
	uint32_t msg_class;
	uint32_t flags;
	uint32_t reserved[3];
};

struct ipc_layout {
	struct ipc_header header;
	struct msg_info info;
};

int ipc_h2d_msg_send_legacy(int handle, uint32_t msg_class, void *msg_payload, uint32_t length);
int ipc_d2h_legacy_msg_process(struct tsse_ipc *tsseipc, void *msg);

#endif

/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023-2024 Montage Technology. All rights reserved.
 */

#ifndef __TSSE_IPC_DRV_H__
#define __TSSE_IPC_DRV_H__

#include <linux/types.h>
#include "tsse_ipc_setup.h"

#define TSSE_IPC_SPECIFIC_RING_SETUP_REQ 100
#define TSSE_IPC_SPECIFIC_RING_SETUP_RSP 101

#pragma pack(push, 4)
struct tsse_ipc_msg {
	u16 type;
	u16 msg_len;
	u32 rev;
	u64 epid;
	u8 data[];
};
#pragma pack(pop)

enum tsse_ipc_type {
	TSSE_IPC_TYPE_LEGACY = 0,
	TSSE_IPC_TYPE_SERVICE,
	TSSE_IPC_TYPE_RING_SETUP_REQ,
	TSSE_IPC_TYPE_RING_SETUP_RSP
};

int ipc_h2d_msg_send(int device_handle, u64 epid, void *msg_payload, u32 length);
int ipc_d2h_msg_dispatch(struct tsse_ipc *tsseipc, void __iomem *d2h_msg);
void ipc_memcpy_to_io(u8 *addr, u8 *src, u32 len);

#endif

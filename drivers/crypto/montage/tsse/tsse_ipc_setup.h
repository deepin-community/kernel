/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023-2024 Montage Technology. All rights reserved.
 */

#ifndef __TSSE_IPC_SETUP_H__
#define __TSSE_IPC_SETUP_H__

#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/completion.h>

#define HOST2MAIN_INTR_SET_OFFSET 0x2000
#define HOST2MAIN_INTR_ENABLE_OFFSET 0x2004
#define HOST2MAIN_ACK_INTR_CLR_OFFSET 0x2008
#define HOST2MAIN_ACK_INTR_ENABLE_OFFSET 0x200c
#define HOST2MAIN_VLD_INTR_STATUS_OFFSET 0x2010
#define HOST2MAIN_ACK_INTR_STATUS_OFFSET 0x2014
#define MSIX_MASK_EN_REG_OFFSET 0x2020
#define INTR_MASK_BIT_OFFSET 0x2024
#define INTR_PENDING_BIT_OFFSET 0x2028
#define HOST2MAIN_IPC_OFFSET 0x2400

#define MAIN2HOST_INTR_SET_OFFSET 0x3000
#define MAIN2HOST_INTR_ENABLE_OFFSET 0x3004
#define MAIN2HOST_ACK_INTR_CLR_OFFSET 0x3008
#define MAIN2HOST_ACK_INTR_ENABLE_OFFSET 0x300c
#define MAIN2HOST_VEN_MSI_FUNC_NUM_OFFSET 0x3010
#define MAIN2HOST_VEN_MSI_VFUNC_ACTIVE_OFFSET 0x3014
#define MAIN2HOST_IPC_OFFSET 0x3400

#define IPC_REGISTER_INT_SET BIT(0)
#define IPC_REGISTER_INT_MASK BIT(1)

#define IPC_MAX_DATA_LEN 1024

typedef int (*tsse_d2h_ipc_handler)(int handle, void *msg_payload, uint32_t payload_length);

enum IPC_BASIC_CMD {
	IPC_BASIC_CMD_HOST_INIT = 0x1,
	IPC_BASIC_CMD_PING = 0x2
};

enum IPC_BOOT_CMD {
	IPC_BOOT_CMD_GET_FIRMWARE = 0x1
};

enum IPC_MESSAGE_CLASS {
	IPC_MESSAGE_BASIC = 1,
	IPC_MESSAGE_BOOT,
	IPC_MESSAGE_CLASS_NUM,
};

struct tsse_ipc {
	struct device *dev;
	struct pci_dev *pdev;
	void __iomem *virt_addr;
	struct mutex list_lock;
	struct tasklet_struct ipc_handle;
	tsse_d2h_ipc_handler d2h_handlers[IPC_MESSAGE_CLASS_NUM];
	u32 im_inited;
};

int tsse_ipc_init(struct pci_dev *pdev);
void tsse_ipc_deinit(void *tdev_t);
int tsse_fw_manual_load_ipc(struct pci_dev *pdev);
int tsse_ipc_services_init(struct pci_dev *pdev);

#endif

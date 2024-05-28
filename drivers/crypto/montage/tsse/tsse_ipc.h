/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023 Montage Technology. All rights reserved.
 */

#ifndef __TM_HOST_IPC_H__
#define __TM_HOST_IPC_H__

#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>

#define TSSE_PASID_SVA

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

struct tsse_ipc {
	struct device *dev;
	struct pci_dev *pdev;
	void __iomem *virt_addr;
	struct mutex list_lock;
	struct tasklet_struct ipc_handle;
};

int tsse_ipc_init(struct pci_dev *pdev);
void tsse_ipc_deinit(void *tdev);
int tsse_fw_manual_load_ipc(struct pci_dev *pdev);
bool check_send_enbit(struct tsse_ipc *tsseipc);
void notify_device(struct tsse_ipc *tsseipc);
#endif

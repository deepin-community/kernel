// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023 Montage Technology. All rights reserved.
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/mm.h>
#include <linux/pci.h>
#include <linux/string.h>
#include <linux/firmware.h>

#include "tsse_dev.h"
#include "tsse_service.h"

#define SEARCH_PATTERN "MT_CFG_BUILD_VERSION_DETAIL"
#define SPACE_CH ' '

static int fw_send_msg(struct tsse_ipc *tsseipc, struct ipc_msg *msg)
{
	u8 *h2d;
	u32 int_reg;

	mutex_lock(&tsseipc->list_lock);

	int_reg = readl(tsseipc->virt_addr + HOST2MAIN_INTR_SET_OFFSET);
	if ((int_reg & IPC_REGISTER_INT_SET) != 0) {
		mutex_unlock(&tsseipc->list_lock);
		return -EFAULT;
	}
	if (msg->header.i_len < sizeof(struct ipc_header) +
		sizeof(struct msg_info) + sizeof(struct fw_load)) {
		dev_err(tsseipc->dev, "msg format error\n");
		return -EFAULT;
	}
	h2d = (u8 *)(tsseipc->virt_addr + HOST2MAIN_IPC_OFFSET);
	memcpy_toio(h2d, msg, sizeof(struct ipc_header));
	memcpy_toio(h2d + sizeof(struct ipc_header), (u8 *)msg->i_data,
		    msg->header.i_len - sizeof(struct ipc_header));
	writel(0x1, tsseipc->virt_addr + HOST2MAIN_INTR_SET_OFFSET);

	dev_info(tsseipc->dev, "notify device to get firmware\n");
	mutex_unlock(&tsseipc->list_lock);
	return 0;
}

/**
 * get_firmware_version() - Get version information from firmware
 * @fw: firmware pointer
 * @fw_version_out: firmware version string output
 * Return: 0 on success, error code otherwise
*/
int get_firmware_version(const struct firmware *fw, char *fw_version_out)
{
	const char *pattern = SEARCH_PATTERN;
	const uint8_t *fw_buffer = fw->data;
	uint32_t pattern_i = 0, buffer_i = 0;
	uint32_t pattern_len = strlen(pattern); // Not include "\0"
	uint32_t version_start = 0;
	uint32_t version_len = 0;

	while (buffer_i < fw->size) {
		if (pattern[pattern_i] == (char) fw_buffer[buffer_i]) {
			buffer_i++;
			pattern_i++;
		}
		if (pattern_i == pattern_len) {
			break;	// pattern found
		} else if ((buffer_i < fw->size) &&
			 (pattern[pattern_i] != (char) fw_buffer[buffer_i])) {
			// mismatch after pattern_i matches
			if (pattern_i != 0) {
				// since the pattern has no common prefix, when mismatch,
				// the next compare should start from pattern beginning
				pattern_i = 0;
			} else {
				buffer_i++;
			}
		}
	}
	if (pattern_i == pattern_len) {
		buffer_i++;
		version_start = buffer_i;
		while (buffer_i < fw->size) {
			if (fw_buffer[buffer_i] == SPACE_CH) {
				version_len = buffer_i - version_start;
				if (version_len >= TSSE_FW_VERSION_LEN - 1)
					version_len = TSSE_FW_VERSION_LEN - 2;
				strscpy(fw_version_out, fw_buffer + version_start, version_len + 1);
				return 0;
			}
			buffer_i++;
		}
	}
	return -EINVAL;
}

/**
 * fw_service() - Firmware service to handle IPC message from mainCPU.
 * It will write init or manual load firmware to PCIe BAR and send message back.
 * @tsseipc_t: pointer to a structure used for IPC
 * @msg_t: pointer to IPC message
*/
void fw_service(void *tsseipc_t, void *msg_t)
{
	void __iomem *fw;
	uint32_t size;
	uint32_t task_offset;
	struct fw_load *fw_task;
	struct tsse_dev *tdev;
	struct tsse_ipc *tsseipc = (struct tsse_ipc *)tsseipc_t;
	struct ipc_msg *msg = (struct ipc_msg *)msg_t;

	task_offset = sizeof(struct msg_info);
	fw_task = (struct fw_load *)((uint8_t *)msg->i_data + task_offset);
	tdev = pci_to_tsse_dev(tsseipc->pdev);

	if (!tdev || !tdev->fw) {
		fw_task->result = 1;
		fw_task->size = 0;
		dev_info(tsseipc->dev, "firmware loading failed\n");
		if (fw_send_msg(tsseipc, msg))
			dev_err(tsseipc->dev, "notify device failed\n");
		return;
	}

	fw_task->result = 0;
	fw_task->size = tdev->fw->size;
	size = tdev->fw->size;
	fw = tsseipc->virt_addr + fw_task->offset + FW_BASE;

	memcpy_toio((u8 *)fw, tdev->fw->data, size);
	dev_info(tsseipc->dev, "firmware loading done\n");
	if (fw_send_msg(tsseipc, msg))
		dev_err(tsseipc->dev, "notify device failed\n");

	if (tdev->fw_version_exist)
		dev_info(tsseipc->dev, "firmware version: %s\n", tdev->fw_version);

	if (tdev->fw) {
		release_firmware(tdev->fw);
		tdev->fw = NULL;
		memset(tdev->fw_version, 0, TSSE_FW_VERSION_LEN);
		tdev->fw_version_exist = false;
	}
}

/**
 * tsse_fw_load() - Load firmware from /lib/firmware
 * @pdev: pci device
 * @name: firmware file name
 * @fw: pointer to firmware pointer
 * Return: 0 on success, error code otherwise
*/
int tsse_fw_load(struct pci_dev *pdev, const char *name, const struct firmware **fw)
{
	int result;

	result = request_firmware(fw, name, &pdev->dev);
	if (result)
		dev_err(&pdev->dev, "%s failed for %s\n", __func__, name);
	return result;
}

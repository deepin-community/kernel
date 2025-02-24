// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023-2024 Montage Technology. All rights reserved.
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
#include "tsse_ipc.h"
#include "tsse_service.h"
#include "tsse_fw_service.h"

#define SEARCH_PATTERN "MT_CFG_BUILD_VERSION_DETAIL"
#define SPACE_CH ' '

static int fw_send_msg(struct tsse_dev *tdev, struct fw_load *fw_task)
{
	struct tsse_ipc *tsseipc = tdev->ipc;

	dev_dbg(tsseipc->dev, "notify device\n");
	return ipc_h2d_msg_send_legacy(tdev->id, IPC_MESSAGE_BOOT, fw_task, sizeof(struct fw_load));
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
 * @handle: handle to TSSE device
 * @msg_payload: pointer to IPC message payload
 * @length: length of the msg_payload
 * Return: 0 on success, error code otherwise
 */
int fw_service(int handle, void *msg_payload, uint32_t length)
{
	void __iomem *fw;
	struct tsse_dev *tdev;
	struct tsse_ipc *tsseipc;
	struct fw_load *fw_task;

	if (!msg_payload || !length) {
		pr_err("%s %d: invalid input parameter\n", __func__, __LINE__);
		return -EINVAL;
	}
	tdev = tsse_get_dev_by_handle(handle);
	if (!tdev)
		return -ENODEV;

	tsseipc = tdev->ipc;
	fw_task = (struct fw_load *) msg_payload;
	if (!tdev->fw) {
		fw_task->result = 1;
		fw_task->size = 0;
		dev_info(tsseipc->dev, "firmware loading failed\n");
		if (fw_send_msg(tdev, fw_task))
			dev_err(tsseipc->dev, "notify device failed\n");
		return -ENOENT;
	}

	fw_task->result = 0;
	fw_task->size = tdev->fw->size;
	fw = tsseipc->virt_addr + fw_task->offset + FW_BASE;

	memcpy_toio((u8 *)fw, tdev->fw->data, tdev->fw->size);
	dev_info(tsseipc->dev, "firmware loading done\n");
	if (fw_send_msg(tdev, fw_task))
		dev_err(tsseipc->dev, "notify device failed\n");

	if (tdev->fw_version_exist)
		dev_info(tsseipc->dev, "firmware version: %s\n", tdev->fw_version);

	if (tdev->fw) {
		release_firmware(tdev->fw);
		tdev->fw = NULL;
		memset(tdev->fw_version, 0, TSSE_FW_VERSION_LEN);
		tdev->fw_version_exist = false;
	}
	return 0;
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
	struct tsse_dev *tdev = pci_to_tsse_dev(pdev);

	result = request_firmware_into_buf(fw, name, &pdev->dev,
		tdev->fw_data, TSSE_FIRMWARE_MAX_LENGTH);
	if (result)
		dev_err(&pdev->dev, "%s failed for %s: %d\n", __func__, name, result);
	return result;
}

/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023-2024 Montage Technology. All rights reserved.
 */

#ifndef __TSSE_FW_SERVICE_H__
#define __TSSE_FW_SERVICE_H__

#include <linux/firmware.h>

#define FW_BASE 0x7000000
#define TSSE_FIRMWARE "tsse_firmware.bin"
#define TSSE_FIRMWARE_MAX_LENGTH (1024 * 1024)

int fw_service(int handle, void *msg_payload, uint32_t length);
int tsse_fw_load(struct pci_dev *pdev, const char *name, const struct firmware **fw);
int get_firmware_version(const struct firmware *fw, char *fw_version_out);
#endif

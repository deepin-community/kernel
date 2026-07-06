/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2024 Phytium Technology Co., Ltd.*
 */

#ifndef USB_TCM_PK_H
#define USB_TCM_PK_H

#define USB_TCM_TEXT_CMD_0                     0x54000000
#define USB_TCM_TEXT_CMD_1                     0x54000001
#define USB_TCM_TEXT_CMD_2                     0x54000002

struct usb_tcm_pk {
	unsigned int cmd;
	unsigned int len;
	unsigned int data[14];
};

#endif

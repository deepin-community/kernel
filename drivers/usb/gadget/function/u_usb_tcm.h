/* SPDX-License-Identifier: GPL-2.0 */
/*
 * u_usb_tcm.h
 *
 * Utility definitions for the hid function
 *
 * Copyright (C) 2024 Phytium Technology Co., Ltd.*
 *
 */

#ifndef U_USB_TCM_H
#define U_USB_TCM_H

#include <linux/usb/composite.h>

struct f_usb_tcm_opts {
	struct usb_function_instance	func_inst;
	int				minor;
	/*
	 * Protect the data form concurrent access by read/write
	 * and create symlink/remove symlink.
	 */
	struct mutex			lock;
	int				refcnt;
};

int g_usb_tcm_setup(struct usb_gadget *g, int count);
void g_usb_tcm_cleanup(void);
#endif

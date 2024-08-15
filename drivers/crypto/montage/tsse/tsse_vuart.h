/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023 Montage Technology. All rights reserved.
 */

#ifndef __TSSE_VUART_H__
#define __TSSE_VUART_H__

#include <linux/pci.h>

#define RLS_VUART_OFFSET (0x680000)
#define RLS_VUART_IRQ_NUM (10)
#define TSSE_VUART_MAX_DEV (64)
#define TSSE_VUART_BITMAP_SIZE (ALIGN(TSSE_VUART_MAX_DEV, 64) / 64)

int vuart_register(void);
void vuart_unregister(void);
int vuart_init_port(struct pci_dev *pdev);
void vuart_uninit_port(struct pci_dev *pdev);

#endif

/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023 Montage Technology. All rights reserved.
 */

#ifndef __TSSE_IRQ_H__
#define __TSSE_IRQ_H__

#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>

#include <linux/irq.h>
#include <linux/interrupt.h>
#include "tsse_dev.h"

static inline void tsse_dev_free_irq_vectors(struct tsse_dev *tdev)
{
	pci_free_irq_vectors(tdev->tsse_pci_dev.pci_dev);
}

int tsse_dev_alloc_irq_vectors(struct tsse_dev *tdev);

#endif

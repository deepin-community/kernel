// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023 Montage Technology. All rights reserved.
 */

#include <linux/msi.h>
#include <linux/delay.h>
#include "tsse_dev.h"
#include "tsse_irq.h"

#undef TSSE_IRQ_DBG

int tsse_dev_alloc_irq_vectors(struct tsse_dev *tdev)
{
	int request_num = tdev->num_irqs;
	int irq_num = pci_alloc_irq_vectors(tdev->tsse_pci_dev.pci_dev,
					    request_num, request_num,
					    PCI_IRQ_MSIX);

	if (irq_num < 0) {
		dev_err(TSSEDEV_TO_DEV(tdev),
			"%s %d :failed to alloc MSIX interrupt vectors\n",
			__func__, __LINE__);
		return irq_num;
	}

	return 0;
}

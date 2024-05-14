// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023 Loongson Technology Corporation Limited
 */

#include <linux/pci.h>

#include <video/nomodeset.h>

#include "loongson_module.h"

static int loongson_modeset = -1;
MODULE_PARM_DESC(modeset, "Disable/Enable modesetting");
module_param_named(modeset, loongson_modeset, int, 0400);

int loongson_vblank = 1;
MODULE_PARM_DESC(vblank, "Disable/Enable hw vblank support");
module_param_named(vblank, loongson_vblank, int, 0400);

static int __init loongson_module_init(void)
{
	struct pci_dev *pdev = NULL;

	while ((pdev = pci_get_class(PCI_CLASS_DISPLAY_VGA << 8, pdev))) {
		/*
		 * Multiple video card workaround
		 *
		 * This integrated video card will always be selected as
		 * default boot device by vgaarb subsystem.
		 */
		if (pdev->vendor != PCI_VENDOR_ID_LOONGSON || pdev->device == 0x1a05) {
			pr_info("Discrete graphic card detected, abort\n");
			return 0;
		}
	}

	if (!loongson_modeset || video_firmware_drivers_only())
		return -ENODEV;

	return pci_register_driver(&lsdc_pci_driver);
}
module_init(loongson_module_init);

static void __exit loongson_module_exit(void)
{
	pci_unregister_driver(&lsdc_pci_driver);
}
module_exit(loongson_module_exit);

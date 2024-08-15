/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HW_ULD_DRIVER_H
#define SSS_HW_ULD_DRIVER_H

#include "sss_hw_event.h"
#include "sss_hw_svc_cap.h"

struct sss_hal_dev {
	struct pci_dev *pdev;
	void *hwdev;
};

struct sss_uld_info {
	/* When it is unnessary to initialize the uld dev,
	 * @probe needs to return 0 and uld_dev is set to NULL;
	 * if uld_dev is NULL, @remove will not be called when uninstalling
	 */
	int (*probe)(struct sss_hal_dev *hal_dev, void **uld_dev, char *uld_dev_name);
	void (*remove)(struct sss_hal_dev *hal_dev, void *uld_dev);
	int (*suspend)(struct sss_hal_dev *hal_dev, void *uld_dev, pm_message_t state);
	int (*resume)(struct sss_hal_dev *hal_dev, void *uld_dev);
	void (*event)(struct sss_hal_dev *hal_dev, void *uld_dev,
		      struct sss_event_info *event);
	int (*ioctl)(void *uld_dev, u32 cmd, const void *buf_in, u32 in_size,
		     void *buf_out, u32 *out_size);
};

/* sss_register_uld - register an upper driver
 * @type: uld service type
 * @uld_info: uld callback
 *
 * Registers an upper-layer driver.
 * Traverse existing devices and call @probe to initialize the uld device.
 */
int sss_register_uld(enum sss_service_type type, struct sss_uld_info *uld_info);

/**
 * sss_unregister_uld - unregister an upper driver
 * @type: uld service type
 *
 * Traverse existing devices and call @remove to uninstall the uld device.
 * Unregisters an existing upper-layer driver.
 */
void sss_unregister_uld(enum sss_service_type type);
#endif

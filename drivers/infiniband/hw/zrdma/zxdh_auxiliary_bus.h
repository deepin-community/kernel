/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef _AUXILIARY_BUS_H_
#define _AUXILIARY_BUS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/device.h>
#include <linux/mod_devicetable.h>

#define ZXDH_AUXILIARY_NAME_SIZE 32

struct zxdh_auxiliary_device_id {
	char name[ZXDH_AUXILIARY_NAME_SIZE];
	kernel_ulong_t driver_data;
};

struct zxdh_auxiliary_device {
	struct device dev;
	const char *name;
	uint32_t id;
};

/**
 * struct zxdh_auxiliary_driver - Definition of an auxiliary bus driver
 * @probe: Called when a matching device is added to the bus.
 * @remove: Called when device is removed from the bus.
 * @shutdown: Called at shut-down time to quiesce the device.
 * @suspend: Called to put the device to sleep mode. Usually to a power state.
 * @resume: Called to bring a device from sleep mode.
 * @name: Driver name.
 * @driver: Core driver structure.
 * @id_table: Table of devices this driver should match on the bus.
 *
 * Auxiliary drivers follow the standard driver model convention, where
 * discovery/enumeration is handled by the core, and drivers provide probe()
 * and remove() methods. They support power management and shutdown
 * notifications using the standard conventions.
 *
 * Auxiliary drivers register themselves with the bus by calling
 * zxdh_auxiliary_driver_register(). The id_table contains the match_names of
 * auxiliary devices that a driver can bind with.
 *
 * .. code-block:: c
 *
 *         static const struct zxdh_auxiliary_device_id my_auxiliary_id_table[] = {
 *           { .name = "foo_mod.foo_dev" },
 *                 {},
 *         };
 *
 *         MODULE_DEVICE_TABLE(zxdh_auxiliary, my_auxiliary_id_table);
 *
 *         struct zxdh_auxiliary_driver my_drv = {
 *                 .name = "myauxiliarydrv",
 *                 .id_table = my_auxiliary_id_table,
 *                 .probe = my_drv_probe,
 *                 .remove = my_drv_remove
 *         };
 */
struct zxdh_auxiliary_driver {
	int32_t (*probe)(struct zxdh_auxiliary_device *auxdev,
			 const struct zxdh_auxiliary_device_id *id);
	int32_t (*remove)(struct zxdh_auxiliary_device *auxdev);
	void (*shutdown)(struct zxdh_auxiliary_device *auxdev);
	int32_t (*suspend)(struct zxdh_auxiliary_device *auxdev,
			   pm_message_t state);
	int32_t (*resume)(struct zxdh_auxiliary_device *auxdev);
	const char *name;
	struct device_driver driver;
	const struct zxdh_auxiliary_device_id *id_table;
};

int32_t zxdh_aux_drv_register(struct zxdh_auxiliary_driver *auxdrv,
			      struct module *owner, const char *modname);
#define zxdh_auxiliary_driver_register(auxdrv) \
	zxdh_aux_drv_register(auxdrv, THIS_MODULE, KBUILD_MODNAME)

void zxdh_auxiliary_driver_unregister(struct zxdh_auxiliary_driver *auxdrv);

#ifdef __cplusplus
}
#endif

#endif /* _AUXILIARY_BUS_H_ */

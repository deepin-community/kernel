
#ifndef _AUXILIARY_BUS_H_
#define _AUXILIARY_BUS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/device.h>
#include <linux/mod_devicetable.h>
#include <linux/dinghai/kcompat.h>

#define ZXDH_AUXILIARY_NAME_SIZE 32
#define ZXDH_AUXILIARY_MODULE_PREFIX "dinghai10e_auxiliary:"

struct zxdh_auxiliary_device_id
{
    char name[ZXDH_AUXILIARY_NAME_SIZE];
    kernel_ulong_t driver_data;
};

struct zxdh_auxiliary_device {
    struct device dev;
    const char *name;
    uint32_t id;
    uint32_t adev_type;
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
    int32_t (*probe)(struct zxdh_auxiliary_device *auxdev, const struct zxdh_auxiliary_device_id *id);
#if defined(HAVE_BUS_TYPE_REMOVE_RETURN_VOID) || defined(ZXDH_ADAPT_REDHAT_9_2)
    void (*remove)(struct zxdh_auxiliary_device *auxdev);
#else
    int32_t (*remove)(struct zxdh_auxiliary_device *auxdev);
#endif
    void (*shutdown)(struct zxdh_auxiliary_device *auxdev);
    int32_t (*suspend)(struct zxdh_auxiliary_device *auxdev, pm_message_t state);
    int32_t (*resume)(struct zxdh_auxiliary_device *auxdev);
    const char *name;
    struct device_driver driver;
    const struct zxdh_auxiliary_device_id *id_table;
};

static inline void *zxdh_auxiliary_get_drvdata(struct zxdh_auxiliary_device *auxdev)
{
    return dev_get_drvdata(&auxdev->dev);
}

static inline void zxdh_auxiliary_set_drvdata(struct zxdh_auxiliary_device *auxdev, void *data)
{
    dev_set_drvdata(&auxdev->dev, data);
}

static inline struct zxdh_auxiliary_device *zxdh_to_auxiliary_dev(struct device *dev)
{
    return container_of(dev, struct zxdh_auxiliary_device, dev);
}
#ifdef AUXILIAR_MATCH_DRV_CONST
static inline struct zxdh_auxiliary_driver *zxdh_to_auxiliary_drv(const struct device_driver *drv)
#else
static inline struct zxdh_auxiliary_driver *zxdh_to_auxiliary_drv(struct device_driver *drv)
#endif
{
    return container_of(drv, struct zxdh_auxiliary_driver, driver);
}

int32_t zxdh_auxiliary_device_init(struct zxdh_auxiliary_device *auxdev);
int32_t zxdh_aux_dev_add(struct zxdh_auxiliary_device *auxdev, const char *modname);
#define zxdh_auxiliary_device_add(auxdev) zxdh_aux_dev_add(auxdev, KBUILD_MODNAME)

static inline void zxdh_auxiliary_device_uninit(struct zxdh_auxiliary_device *auxdev)
{
    put_device(&auxdev->dev);
}

static inline void zxdh_auxiliary_device_delete(struct zxdh_auxiliary_device *auxdev)
{
    device_del(&auxdev->dev);
}

int32_t zxdh_aux_drv_register(struct zxdh_auxiliary_driver *auxdrv, struct module *owner,
                              const char *modname);
#define zxdh_auxiliary_driver_register(auxdrv) \
    zxdh_aux_drv_register(auxdrv, THIS_MODULE, KBUILD_MODNAME)

void zxdh_auxiliary_driver_unregister(struct zxdh_auxiliary_driver *auxdrv);

/**
 * module_auxiliary_driver() - Helper macro for registering an auxiliary driver
 * @__auxiliary_driver: auxiliary driver struct
 *
 * Helper macro for auxiliary drivers which do not do anything special in
 * module init/exit. This eliminates a lot of boilerplate. Each module may only
 * use this macro once, and calling it replaces module_init() and module_exit()
 *
 * .. code-block:: c
 *
 *    module_auxiliary_driver(my_drv);
 */
#define module_auxiliary_driver(__auxiliary_driver) \
    module_driver(__auxiliary_driver, zxdh_auxiliary_driver_register, zxdh_auxiliary_driver_unregister)

#if defined(HAVE_LINUX_DEVICE_BUS_H) || defined(HAVE_BUS_FIND_DEVICE_GET_CONST) && !defined(NEED_XARRAY)
#if (KYLIN_RELEASE_CODE != KYLIN_RELEASE_VERSION(10,4))
struct zxdh_auxiliary_device *zxdh_auxiliary_find_device(struct device *start,
                                                         const void *data,
                                                         int32_t (*match)(struct device *dev, const void *data));
#else
struct zxdh_auxiliary_device *
zxdh_auxiliary_find_device(struct device *start,
                           void *data,
                           int32_t (*match)(struct device *dev, void *data));
#endif /*KYLIN_RELEASE_VERSION*/
#else
struct zxdh_auxiliary_device *
zxdh_auxiliary_find_device(struct device *start,
                           void *data,
                           int32_t (*match)(struct device *dev, void *data));
#endif /* HAVE_LINUX_DEVICE_BUS_H */

#ifdef __cplusplus
}
#endif

#endif /* _AUXILIARY_BUS_H_ */

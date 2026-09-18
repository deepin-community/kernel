
#ifdef pr_fmt
#undef pr_fmt
#endif

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/dinghai/driver.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include <linux/string.h>
#include <linux/dinghai/zxdh_auxiliary_bus.h>
#include <linux/acpi.h>
#include <linux/dinghai/kcompat.h>
#ifdef CONFIG_COMPAT_AUXILIARY_EXTERNAL_INIT
#include "base.h"
#endif

#ifdef CGS_V5_693
	/* 3.10 内核不支持 dev_pm_domain_attach */
	#undef HAVE_DEV_PM_DOMAIN_ATTACH
#endif

static const struct zxdh_auxiliary_device_id *zxdh_auxiliary_match_id(const struct zxdh_auxiliary_device_id *id,
                                                                      const struct zxdh_auxiliary_device *auxdev)
{
    for (; id->name[0]; id++)
    {
        const char *p = strrchr(dev_name(&auxdev->dev), '.');
        int32_t match_size;

        if (!p)
        {
            continue;
        }
        match_size = p - dev_name(&auxdev->dev);

        /* use dev_name(&auxdev->dev) prefix before last '.' char to match to */
        if (strlen(id->name) == match_size &&
            !strncmp(dev_name(&auxdev->dev), id->name, match_size))
        {
            return id;
        }
    }

    return NULL;
}

#ifdef AUXILIAR_MATCH_DRV_CONST
static int32_t zxdh_auxiliary_match(struct device *dev, const struct device_driver *drv)
#else
static int32_t zxdh_auxiliary_match(struct device *dev, struct device_driver *drv)
#endif
{
    struct zxdh_auxiliary_device *auxdev = zxdh_to_auxiliary_dev(dev);
    struct zxdh_auxiliary_driver *auxdrv = zxdh_to_auxiliary_drv(drv);

    return !!zxdh_auxiliary_match_id(auxdrv->id_table, auxdev);
}


#if defined(HAVE_CONST_DEVICE) || defined(HAVE_MEMBER_DEVICE_TYPE_CONST)
static int32_t zxdh_auxiliary_uevent(const struct device *dev, struct kobj_uevent_env *env)
#else
static int32_t zxdh_auxiliary_uevent(struct device *dev, struct kobj_uevent_env *env)
#endif
{
    const char *name;
    const char *p;

    name = dev_name(dev);
    p = strrchr(name, '.');

    return add_uevent_var(env, "MODALIAS=%s%.*s", ZXDH_AUXILIARY_MODULE_PREFIX, (int32_t)(p - name), name);
}

static const struct dev_pm_ops zxdh_auxiliary_dev_pm_ops = {
    SET_RUNTIME_PM_OPS(pm_generic_runtime_suspend, pm_generic_runtime_resume, NULL)
    SET_SYSTEM_SLEEP_PM_OPS(pm_generic_suspend, pm_generic_resume)
};

static int32_t zxdh_auxiliary_bus_probe(struct device *dev)
{
    struct zxdh_auxiliary_driver *auxdrv = zxdh_to_auxiliary_drv(dev->driver);
    struct zxdh_auxiliary_device *auxdev = zxdh_to_auxiliary_dev(dev);
    int32_t ret = 0;

#ifdef HAVE_DEV_PM_DOMAIN_ATTACH
    ret = dev_pm_domain_attach(dev, true);

    /* In case of old kernels 4.17 and below do nothing in case of
     * failure of ENODEV */
    if (ret == -ENODEV)
    {
        ret = 0;
    }

    if (ret != 0)
    {
        LOG_WARN("Failed to attach to PM Domain : %d\n", ret);
        return ret;
    }
#else
    acpi_dev_pm_attach(dev, true);
#endif

    ret = auxdrv->probe(auxdev, zxdh_auxiliary_match_id(auxdrv->id_table, auxdev));
    if (ret != 0)
#ifdef HAVE_DEV_PM_DOMAIN_ATTACH
        dev_pm_domain_detach(dev, true);
#else
        acpi_dev_pm_detach(dev, true);
#endif

    return ret;
}

#if defined(HAVE_BUS_TYPE_REMOVE_RETURN_VOID) || defined(ZXDH_ADAPT_REDHAT_9_2) || defined(ZXDH_ADAPT_ANOLIS_6_6)
static void zxdh_auxiliary_bus_remove(struct device *dev)
#else
static int32_t zxdh_auxiliary_bus_remove(struct device *dev)
#endif
{
    struct zxdh_auxiliary_driver *auxdrv = zxdh_to_auxiliary_drv(dev->driver);
    struct zxdh_auxiliary_device *auxdev = zxdh_to_auxiliary_dev(dev);

    if (auxdrv->remove)
    {
        auxdrv->remove(auxdev);
    }
#ifdef HAVE_DEV_PM_DOMAIN_ATTACH
    dev_pm_domain_detach(dev, true);
#else
    acpi_dev_pm_detach(dev, true);
#endif

#if !defined(HAVE_BUS_TYPE_REMOVE_RETURN_VOID) && !defined(ZXDH_ADAPT_REDHAT_9_2) && !defined(ZXDH_ADAPT_ANOLIS_6_6)
    return 0;
#endif
}

static void zxdh_auxiliary_bus_shutdown(struct device *dev)
{
    struct zxdh_auxiliary_driver *auxdrv = NULL;
    struct zxdh_auxiliary_device *auxdev = NULL;

    if (dev->driver)
    {
        auxdrv = zxdh_to_auxiliary_drv(dev->driver);
        auxdev = zxdh_to_auxiliary_dev(dev);
    }

    if (auxdrv && auxdrv->shutdown)
    {
        auxdrv->shutdown(auxdev);
    }
}

static struct bus_type zxdh_auxiliary_bus_type = {
    .name     = "dinghai10e_auxiliary",
    .probe    = zxdh_auxiliary_bus_probe,
    .remove   = zxdh_auxiliary_bus_remove,
    .shutdown = zxdh_auxiliary_bus_shutdown,
    .match    = zxdh_auxiliary_match,
    .uevent   = zxdh_auxiliary_uevent,
    .pm       = &zxdh_auxiliary_dev_pm_ops,
};

/**
 * zxdh_auxiliary_device_init - check zxdh_auxiliary_device and initialize
 * @auxdev: auxiliary device struct
 *
 * This is the second step in the three-step process to register an
 * zxdh_auxiliary_device.
 *
 * When this function returns an error code, then the device_initialize will
 * *not* have been performed, and the caller will be responsible to free any
 * memory allocated for the zxdh_auxiliary_device in the error path directly.
 *
 * It returns 0 on success.  On success, the device_initialize has been
 * performed.  After this point any error unwinding will need to include a call
 * to zxdh_auxiliary_device_uninit().  In this post-initialize error scenario, a call
 * to the device's .release callback will be triggered, and all memory clean-up
 * is expected to be handled there.
 */
int32_t zxdh_auxiliary_device_init(struct zxdh_auxiliary_device *auxdev)
{
    struct device *dev = &auxdev->dev;

    if (!dev->parent)
    {
        LOG_ERR("zxdh_auxiliary_device has a NULL dev->parent\n");
        return -EINVAL;
    }

    if (!auxdev->name)
    {
        LOG_ERR("zxdh_auxiliary_device has a NULL name\n");
        return -EINVAL;
    }

    dev->bus = &zxdh_auxiliary_bus_type;
    device_initialize(&auxdev->dev);

    return 0;
}
EXPORT_SYMBOL_GPL(zxdh_auxiliary_device_init);

/**
 * zxdh_aux_dev_add - add an auxiliary bus device
 * @auxdev: auxiliary bus device to add to the bus
 * @modname: name of the parent device's driver module
 *
 * This is the third step in the three-step process to register an
 * zxdh_auxiliary_device.
 *
 * This function must be called after a successful call to
 * zxdh_auxiliary_device_init(), which will perform the device_initialize.  This
 * means that if this returns an error code, then a call to
 * zxdh_auxiliary_device_uninit() must be performed so that the .release callback
 * will be triggered to free the memory associated with the zxdh_auxiliary_device.
 *
 * The expectation is that users will call the "zxdh_auxiliary_device_add" macro so
 * that the caller's KBUILD_MODNAME is automatically inserted for the modname
 * parameter.  Only if a user requires a custom name would this version be
 * called directly.
 */
int32_t zxdh_aux_dev_add(struct zxdh_auxiliary_device *auxdev, const char *modname)
{
    struct device *dev = &auxdev->dev;
    int32_t ret        = 0;

    if (!modname)
    {
        LOG_ERR( "zxdh auxiliary device modname is NULL\n");
        return -EINVAL;
    }

    ret = dev_set_name(dev, "%s.%s.%d", modname, auxdev->name, auxdev->id);
    if (ret != 0)
    {
        LOG_ERR( "zxdh auxiliary device dev_set_name failed: %d\n", ret);
        return ret;
    }

    ret = device_add(dev);
    if (ret != 0)
    {
        LOG_ERR( "adding zxdh auxiliary device failed!: %d\n", ret);
    }

    return ret;
}
EXPORT_SYMBOL_GPL(zxdh_aux_dev_add);

/**
 * zxdh_auxiliary_find_device - auxiliary device iterator for locating a particular device.
 * @start: Device to begin with
 * @data: Data to pass to match function
 * @match: Callback function to check device
 *
 * This function returns a reference to a device that is 'found'
 * for later use, as determined by the @match callback.
 *
 * The reference returned should be released with put_device().
 *
 * The callback should return 0 if the device doesn't match and non-zero
 * if it does.  If the callback returns non-zero, this function will
 * return to the caller and not iterate over any more devices.
 */
#if defined(HAVE_LINUX_DEVICE_BUS_H) || defined(HAVE_BUS_FIND_DEVICE_GET_CONST) && !defined(NEED_XARRAY)
#if (KYLIN_RELEASE_CODE != KYLIN_RELEASE_VERSION(10,4))
struct zxdh_auxiliary_device *
zxdh_auxiliary_find_device(struct device *start,
                           const void *data,
                           int32_t (*match)(struct device *dev, const void *data))
#else
struct zxdh_auxiliary_device *
zxdh_auxiliary_find_device(struct device *start,
                           void *data,
                           int32_t (*match)(struct device *dev, void *data))
#endif /*KYLIN_RELEASE_VERSION*/
#else
struct zxdh_auxiliary_device *
zxdh_auxiliary_find_device(struct device *start,
                           void *data,
                           int32_t (*match)(struct device *dev, void *data))
#endif /* HAVE_BUS_FIND_DEVICE_GET_CONST || HAVE_LINUX_DEVICE_BUS_H */
{
    struct device *dev = NULL;

    dev = bus_find_device(&zxdh_auxiliary_bus_type, start, data, match);
    if (dev == NULL)
    {
        return NULL;
    }

    return zxdh_to_auxiliary_dev(dev);
}
EXPORT_SYMBOL_GPL(zxdh_auxiliary_find_device);

/**
 * zxdh_aux_drv_register - register a driver for auxiliary bus devices
 * @auxdrv: zxdh_auxiliary_driver structure
 * @owner: owning module/driver
 * @modname: KBUILD_MODNAME for parent driver
 *
 * The expectation is that users will call the "zxdh_auxiliary_driver_register"
 * macro so that the caller's KBUILD_MODNAME is automatically inserted for the
 * modname parameter.  Only if a user requires a custom name would this version
 * be called directly.
 */
int32_t zxdh_aux_drv_register(struct zxdh_auxiliary_driver *auxdrv,
                              struct module *owner, const char *modname)
{
    int32_t ret = 0;

    if (WARN_ON(!auxdrv->probe) || WARN_ON(!auxdrv->id_table))
    {
        return -EINVAL;
    }

    if (auxdrv->name)
    {
        auxdrv->driver.name = kasprintf(GFP_KERNEL, "%s.%s", modname, auxdrv->name);
    }
    else
    {
        auxdrv->driver.name = kasprintf(GFP_KERNEL, "%s", modname);
    }
    if (!auxdrv->driver.name)
    {
        return -ENOMEM;
    }

    auxdrv->driver.owner = owner;
    auxdrv->driver.bus = &zxdh_auxiliary_bus_type;
    auxdrv->driver.mod_name = modname;

    ret = driver_register(&auxdrv->driver);
    if (ret)
    {
        kfree(auxdrv->driver.name);
    }

    return ret;
}
EXPORT_SYMBOL_GPL(zxdh_aux_drv_register);

/**
 * zxdh_auxiliary_driver_unregister - unregister a driver
 * @auxdrv: zxdh_auxiliary_driver structure
 */
void zxdh_auxiliary_driver_unregister(struct zxdh_auxiliary_driver *auxdrv)
{
    driver_unregister(&auxdrv->driver);
    kfree(auxdrv->driver.name);
}
EXPORT_SYMBOL_GPL(zxdh_auxiliary_driver_unregister);

#ifdef CONFIG_COMPAT_AUXILIARY_EXTERNAL_INIT
void __init zxdh_auxiliary_bus_init(void)
{
    WARN_ON(bus_register(&zxdh_auxiliary_bus_type));
}
#else
static int32_t __init zxdh_auxiliary_bus_init(void)
{
    return bus_register(&zxdh_auxiliary_bus_type);
}

static void __exit zxdh_auxiliary_bus_exit(void)
{
    bus_unregister(&zxdh_auxiliary_bus_type);
}

module_init(zxdh_auxiliary_bus_init);
module_exit(zxdh_auxiliary_bus_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Auxiliary Bus");
MODULE_INFO(supported, "external");
MODULE_AUTHOR("David Ertman <david.m.ertman@intel.com>");
MODULE_AUTHOR("Kiran Patil <kiran.patil@intel.com>");
#endif

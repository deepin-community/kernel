#include <linux/dinghai/devlink.h>
#include <linux/dinghai/driver.h>

#ifdef CGS_V5_693
/*
 * For CGSL V5 693 (RHEL 7.4 / 3.10.0-693) kernel,
 * devlink_register() is not exported as a symbol for external modules.
 * Provide a stub function that does nothing.
 */
int32_t zxdh_devlink_register(struct devlink *devlink, struct device *dev)
{
    /* devlink_register is not supported on CGS_V5_693 */
    return 0;
}
#else
#ifdef HAVE_DEVLINK_REGISTER_GET_1_PARAMS
    int32_t zxdh_devlink_register(struct devlink *devlink)
#else
    int32_t zxdh_devlink_register(struct devlink *devlink, struct device *dev)
#endif
{
    struct dh_core_dev *dh_dev = devlink_priv(devlink);
    int32_t err = 0;

#ifdef HAVE_DEVLINK_REGISTER_GET_1_PARAMS
        devlink_register(devlink);
#else
        devlink_register(devlink, dev);
#endif

    err = dh_dev->devlink_ops->params_register(devlink);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "params_register failed: %d\n", err);
        return err;
    }

    return err;
}
#endif

struct devlink *zxdh_devlink_alloc(struct device *dev, struct devlink_ops *dh_devlink_ops, size_t priv_size)
{
#ifdef CGS_V5_693
    /*
     * For CGSL V5 693 (RHEL 7.4 / 3.10.0-693) kernel,
     * devlink_alloc() is not exported as a symbol for external modules.
     * Directly allocate memory using kzalloc and initialize the structure.
     */
    struct devlink *devlink;
    size_t size = sizeof(struct devlink) + sizeof(struct dh_core_dev) + priv_size;

    devlink = kzalloc(size, GFP_KERNEL);
    if (devlink) {
        devlink->ops = dh_devlink_ops;
        devlink->dev = dev;
    }
    return devlink;
#else
#ifdef HAVE_DEVLINK_ALLOC_GET_1_PARAMS
    return devlink_alloc(dh_devlink_ops, sizeof(struct dh_core_dev) + priv_size);
#else
    return devlink_alloc(dh_devlink_ops, sizeof(struct dh_core_dev) + priv_size, dev);
#endif
#endif
}

void zxdh_devlink_free(struct devlink *devlink)
{
#ifdef CGS_V5_693
    /*
     * For CGSL V5 SP693 (RHEL 7.4 / 3.10.0-693) kernel,
     * devlink_free() is not exported as a symbol for external modules.
     * Directly use kfree() to free the devlink structure.
     */
    kfree(devlink);
#else
    devlink_free(devlink);
#endif
}

#ifdef CGS_V5_693
/*
 * For CGSL V5 693 (RHEL 7.4 / 3.10.0-693) kernel,
 * devlink_unregister() is not exported as a symbol for external modules.
 * Provide a stub function that does nothing.
 */
void zxdh_devlink_unregister(struct devlink *devlink)
{
    /* devlink_unregister is not supported on CGS_V5_693 */
}
#else
void zxdh_devlink_unregister(struct devlink *devlink)
{
    struct dh_core_dev *dev = devlink_priv(devlink);

    /*
     * Since Linux 5.16 (and some 5.14-based vendor kernels, such as RHEL 9.2),
     * devlink_params_unregister() is expected to be called only when
     * the devlink instance is not registered anymore, otherwise
     * devlink_params_unregister() may trigger WARN_ON().
     *
     * To keep compatibility with older kernels, use different orders
     * depending on kernel version:
     * - For kernels >= 5.14: call devlink_unregister() first, then
     *   params_unregister()
     * - For kernels < 5.14: keep the original order unchanged
     */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,0))
    devlink_unregister(devlink);
    dev->devlink_ops->params_unregister(devlink);
#else
    dev->devlink_ops->params_unregister(devlink);
    devlink_unregister(devlink);
#endif
}
#endif

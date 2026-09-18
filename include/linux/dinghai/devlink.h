#ifndef __ZXDH_DEVLINK_H__
#define __ZXDH_DEVLINK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <net/devlink.h>
#include <linux/dinghai/driver.h>
#include <linux/types.h>

struct devlink *zxdh_devlink_alloc(struct device *dev, struct devlink_ops *dh_devlink_ops, size_t priv_size);
void zxdh_devlink_free(struct devlink *devlink);

#ifdef HAVE_DEVLINK_REGISTER_GET_1_PARAMS
int32_t zxdh_devlink_register(struct devlink *devlink);
#else
int32_t zxdh_devlink_register(struct devlink *devlink, struct device *dev);
#endif

void zxdh_devlink_unregister(struct devlink *devlink);

static inline struct net *dh_core_net(struct dh_core_dev *dev)
{
    return devlink_net(priv_to_devlink(dev));
}

#ifdef __cplusplus
}
#endif

#endif
#include <linux/dinghai/driver.h>
#include <linux/dinghai/devlink_compat.h>
#include <net/devlink.h>
#include "en_sf_devlink.h"

struct devlink_ops dh_sf_devlink_ops = {

};

enum {
    DH_SF_PARAMS_MAX,
};

static int32_t __attribute__((unused)) sample_check(struct dh_core_dev *dev)
{
    return 1;
}

enum dh_sf_devlink_param_id {
    DH_SF_DEVLINK_PARAM_ID_BASE = DEVLINK_PARAM_GENERIC_ID_MAX,
    DH_SF_DEVLINK_PARAM_ID_SAMPLE,
};

#ifndef CGS_V5_693
#if defined(ZXDH_ADAPT_REDHAT_9_6) || defined(DEVLINK_SAMEPLE_SET_HAVE_ACK)
static int32_t dh_devlink_sample_set(struct devlink *devlink, uint32_t id,
                              struct devlink_param_gset_ctx *ctx, struct netlink_ext_ack *ack)
#else
static int32_t dh_devlink_sample_set(struct devlink *devlink, uint32_t id,
                              struct devlink_param_gset_ctx *ctx)
#endif
{
    struct dh_core_dev * __attribute__((unused)) dev = devlink_priv(devlink);

    return 0;
}

static int32_t dh_devlink_sample_get(struct devlink *devlink, uint32_t id,
                              struct devlink_param_gset_ctx *ctx)
{
    struct dh_core_dev * __attribute__((unused)) dev = devlink_priv(devlink);

    return 0;
}
#endif

#ifdef HAVE_DEVLINK_PARAM_REGISTER
static const struct devlink_params {
    const char *name;
    int32_t (*check)(struct dh_core_dev *dev);
    struct devlink_param param;
} devlink_params[] = {
    [DH_SF_PARAMS_MAX] = { .name = "sample",
                           .check = &sample_check,
                           .param = DEVLINK_PARAM_DRIVER(DH_SF_DEVLINK_PARAM_ID_SAMPLE,
                                                          "sample", DEVLINK_PARAM_TYPE_BOOL,
                                                          BIT(DEVLINK_PARAM_CMODE_RUNTIME),dh_devlink_sample_get,
                                                          dh_devlink_sample_set,
                                                          NULL),
                          }
};

static int32_t params_register(struct devlink *devlink)
{
    int32_t i = 0;
    int32_t err = 0;
    struct dh_core_dev *dh_dev = devlink_priv(devlink);

    for (i = 0; i < ARRAY_SIZE(devlink_params); i++)
    {
        if(devlink_params[i].check(dh_dev))
        {
            err = devlink_param_register(devlink, &devlink_params[i].param);
            if (err)
            {
               goto rollback;
            }
        }
    }

    return 0;

rollback:
    if (i == 0)
    {
        return err;
    }

    for (; i > 0; i--)
    {
        devlink_param_unregister(devlink, &devlink_params[i].param);
    }

    return err;
}

static int32_t params_unregister(struct devlink *devlink)
{
    int32_t i = 0;

    for (i = 0; i < ARRAY_SIZE(devlink_params); i++)
    {
        devlink_param_unregister(devlink, &devlink_params[i].param);
    }

    return 0;
}
#else
#ifndef CGS_V5_693
static struct devlink_param devlink_params [] = {
    [DH_SF_PARAMS_MAX] = DEVLINK_PARAM_DRIVER(DH_SF_DEVLINK_PARAM_ID_SAMPLE,
                                                          "sample", DEVLINK_PARAM_TYPE_BOOL,
                                                          BIT(DEVLINK_PARAM_CMODE_RUNTIME),dh_devlink_sample_get,
                                                          dh_devlink_sample_set,
                                                          NULL),
};

static int32_t params_register(struct devlink *devlink)
{
    struct dh_core_dev * __attribute__((unused)) dh_dev = devlink_priv(devlink);
    int32_t err = 0;

    err = devlink_params_register(devlink, devlink_params, ARRAY_SIZE(devlink_params));

    return err;
}
static int32_t params_unregister(struct devlink *devlink)
{
    devlink_params_unregister(devlink, devlink_params, ARRAY_SIZE(devlink_params));

    return 0;
}
#else
/* Stubs for when HAVE_DEVLINK_PARAM_REGISTER is not defined (older kernels) */
static inline int32_t params_register(struct devlink *devlink) { return 0; }
static inline int32_t params_unregister(struct devlink *devlink) { return 0; }
#endif
#endif

struct dh_core_devlink_ops dh_sf_core_devlink_ops = {
    .params_register   =  params_register,
    .params_unregister =  params_unregister
};
#ifndef __EN_MPF_CFG_SF_H__
#define __EN_MPF_CFG_SF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/dinghai/zxdh_auxiliary_bus.h>
#include <linux/dinghai/driver.h>

struct cfg_sf_ops {

};

struct cfg_sf_dev {
    struct zxdh_auxiliary_device adev;
    struct dh_core_dev *dh_dev;
    struct cfg_sf_ops *ops;
};

int32_t zxdh_mpf_sf_driver_register(void);
void zxdh_mpf_sf_driver_uregister(void);


#ifdef __cplusplus
}
#endif


#endif
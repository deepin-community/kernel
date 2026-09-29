#include <linux/kernel.h>
#include <linux/dinghai/zxdh_auxiliary_bus.h>
#include <linux/dinghai/driver.h>

#include "en_mpf_cfg_sf.h"

static int32_t zxdh_cfg_resume(struct zxdh_auxiliary_device *adev)
{
    return 0;
}

static int32_t zxdh_cfg_suspend(struct zxdh_auxiliary_device *adev, pm_message_t state)
{
    return 0;
}

static int32_t zxdh_cfg_probe(struct zxdh_auxiliary_device *adev,
                              const struct zxdh_auxiliary_device_id *id)
{
    struct cfg_sf_dev * __attribute__((unused)) cfg_sf_dev = container_of(adev, struct cfg_sf_dev, adev);

    return 0;
}

static int32_t zxdh_cfg_remove(struct zxdh_auxiliary_device *adev)
{
    return 0;
}

static const struct zxdh_auxiliary_device_id zxdh_cfg_id_table[] = {
    { .name = ZXDH_EN_SF_NAME ".mpf_cfg", },
    {},
};

//MODULE_DEVICE_TABLE(auxiliary_zxdh_id_table, zxdh_cfg_id_table);

static struct zxdh_auxiliary_driver zxdh_cfg_driver = {
    .name     = "mpf_cfg",
    .probe    = zxdh_cfg_probe,
    .remove   = zxdh_cfg_remove,
    .suspend  = zxdh_cfg_suspend,
    .resume   = zxdh_cfg_resume,
    .id_table = zxdh_cfg_id_table,
};

int32_t zxdh_mpf_sf_driver_register(void)
{
    return zxdh_auxiliary_driver_register(&zxdh_cfg_driver);;
}

void zxdh_mpf_sf_driver_uregister(void)
{
    zxdh_auxiliary_driver_unregister(&zxdh_cfg_driver);;
}

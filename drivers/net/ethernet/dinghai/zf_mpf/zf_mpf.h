#ifndef __ZXDH_ZF_MPF_H__
#define __ZXDH_ZF_MPF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/workqueue.h>
#include <linux/dinghai/driver.h>
#include <linux/dinghai/log.h>
#include "gdma.h"

#define ZXDH_MPF_VENDOR_ID                  0x1cf2
#define ZXDH_MPF_DEVICE_ID0                 0x8044
#define ZXDH_MPF_DEVICE_ID1                 0x806a

#define ZXDH_BAR1_CHAN_OFFSET               0x2000//0x7801000
#define ZXDH_BAR2_CHAN_OFFSET               0x3000//0x7802000
#define VERSION_OF_ZF_MPF_OFFSET            0x5438
#define FW_FEATURE_OF_ZF_MPF_OFFSET         0x1004
#define FW_FEATURE_SUPPORT_MASK             0x10000
#define ZF_MPF_COMPAT_ITEM                  8

struct dh_en_mpf_dev {
    uint16_t ep_bdf;
    uint16_t pcie_id;
    uint16_t vport;

    uint64_t pci_ioremap_addr;

    struct work_struct dh_np_sdk_from_risc;
    struct work_struct dh_np_sdk_from_pf;

    struct zf_gdma_dev *gdev;
};

struct fw_compat_version
{
    uint8_t major;
    uint8_t fw_minor;
    uint8_t drv_minor;
    uint16_t patch;
};

struct version_compat_reg
{
    uint8_t version_compat_item;
    uint8_t major;
    uint8_t fw_minor;
    uint8_t drv_minor;
    uint16_t patch;
    uint8_t rsv[2];
};

#ifdef __cplusplus
}
#endif

#endif /* __ZXDH_EN_MPF_H__ */
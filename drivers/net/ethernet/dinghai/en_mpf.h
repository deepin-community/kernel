#ifndef __ZXDH_EN_MPF_H__
#define __ZXDH_EN_MPF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/workqueue.h>
#include <linux/dinghai/driver.h>

#define ZXDH_MPF_VENDOR_ID 0x1111
#define ZXDH_MPF_DEVICE_ID 0x1041

#define ZXDH_BAR1_CHAN_OFFSET               0x2000//0x7801000
#define ZXDH_BAR2_CHAN_OFFSET               0x3000//0x7802000

struct dh_en_mpf_dev {
    uint16_t ep_bdf;
    uint16_t pcie_id;
    uint16_t vport;

    uint64_t pci_ioremap_addr;

    struct work_struct dh_np_sdk_from_risc;
    struct work_struct dh_np_sdk_from_pf;
};

#ifdef __cplusplus
}
#endif

#endif /* __ZXDH_EN_MPF_H__ */
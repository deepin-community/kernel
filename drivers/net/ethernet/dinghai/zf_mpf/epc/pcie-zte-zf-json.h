#ifndef __PCIE_ZTE_ZF_JSON_H
#define __PCIE_ZTE_ZF_JSON_H

#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/err.h>
#include <linux/namei.h>
#include <linux/dcache.h>
#include <linux/fs.h>

#include "pcie-zte-zf-epc.h"

#ifdef _cplusplus
extern "C" {
#endif
#define ZXDH_SYSFS_DIR                  "zxdh_sysfs"
#define ZXDH_SYSFS_PATH                 "/sys/zxdh_sysfs"

struct dpu_pf_cfg {
    u8 cmd_id;
    u8 ep_id;
    u8 pf_id;
    u8 pf_enable;
    u8 dev_type;
    u32 vendor_id;
    u32 device_id;
    u32 max_vf;
};

#ifdef __cplusplus
}
#endif

#endif
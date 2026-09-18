#ifndef _ZXDH_PCIE_COMMON_H_
#define _ZXDH_PCIE_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/msi.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/dinghai/log.h>

#define PCIE_SUCCESS 0
#define PCIE_FAILED -1

#define KERNEL_VFIO_VFIO_TEST_NAME          "/dev/vfio/vfio"
#define PCI_DEVICES_DIR                     "/sys/bus/pci/devices"
#define SYS_BUS_PCI_DIR                     "/sys/bus/pci"
#define PCI_PHYSFN_DRV_PATH                 "physfn/driver"
#define FILE_PATH_LEN                       100

#define DOMAIN_LEN                          4
#define BUS_LEN                             2
#define DEVICE_LEN                          2
#define FUNC_LEN                            1

#define BDF_NO_BUS_NO_MASK                  (0xFF << 8)
#define BDF_NO_DEV_NO_MASK                  (0x1F << 3)
#define BDF_NO_FUNC_NO_MASK                 (0x7 << 0)

#define BDF_F_START_BIT                     0
#define BDF_D_START_BIT                     3
#define BDF_B_START_BIT                     8
#define VERSION_OF_HPF_OFFSET               0x5440
#define FW_FEATURE_OF_ZF_MPF_OFFSET         0x1004
#define FW_FEATURE_SUPPORT_MASK             0x10000
#define HPF_COMPAT_ITEM                     9

struct domain_bdf {
    char domain[DOMAIN_LEN + 1];
    char bus[BUS_LEN + 1];
    char device[DEVICE_LEN + 1];
    char func[FUNC_LEN + 1];
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

struct pci_dev *zxdh_get_pci_device(u32 domain, u32 bdf);
int fill_domain_bdf_str(const char *token, char *dst_str, const int len);
int parse_bdf(char *str);

#ifdef __cplusplus
}
#endif

#endif

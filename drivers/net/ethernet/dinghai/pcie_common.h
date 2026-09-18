#ifndef __ZXDH_PCIE_COMMON_H__
#define __ZXDH_PCIE_COMMON_H__
#include <linux/dinghai/driver.h>
#include <linux/random.h>

#ifdef PCIE_VPD_STRUCT_COMPAT
    #define VPD_STRUCT_COMPAT_V1
#endif

#define ZXDH_PCIE_BAR0_OFF          0x10
#define ZXDH_PCIE_BAR1_OFF          0x14
#define PCIE_NORMAL_BAR_MAX_NUMS    6

#define PCIE_OK                     (0)
#define PCIE_ERR                    (-1)

#define LOW32(x)                    ((u32)(x))
#define HIGH32(x)                   ((u32)((x) >> 32))
#define SRIOV_BAR_OFF(n)            (PCI_SRIOV_BAR + (n) * 4)

#define VF_BAR_ADDR_STORE           (0)
#define VF_BAR_ADDR_RELOAD          (1)

struct pcie_sriov_bar_info {
    u32 bar_addr[6];
};

#ifdef VPD_STRUCT_COMPAT_V1
    // 内核中暴露pci_vpd接口体
#else
struct pci_vpd {
    const struct pci_vpd_ops *ops;
    struct bin_attribute *attr; /* Descriptor for sysfs VPD entry */
    struct mutex lock;
    unsigned int len;
    u16 flag;
    u8 cap;
    unsigned int busy : 1;
    unsigned int valid : 1;
};
#endif

struct pci_sriov {
    int pos;                                   /* Capability position */
    int nres;                                  /* Number of resources */
    u32 cap;                                   /* SR-IOV Capabilities */
    u16 ctrl;                                  /* SR-IOV Control */
    u16 total_VFs;                             /* Total VFs associated with the PF */
    u16 initial_VFs;                           /* Initial VFs associated with the PF */
    u16 num_VFs;                               /* Number of VFs available */
    u16 offset;                                /* First VF Routing ID offset */
    u16 stride;                                /* Following VF stride */
    u16 vf_device;                             /* VF device ID */
    u32 pgsz;                                  /* Page size for BAR alignment */
    u8 link;                                   /* Function Dependency Link */
    u8 max_VF_buses;                           /* Max buses consumed by VFs */
    u16 driver_max_VFs;                        /* Max num VFs driver supports */
    struct pci_dev *dev;                       /* Lowest numbered PF */
    struct pci_dev *self;                      /* This PF */
    u32 class;                                 /* VF device */
    u8 hdr_type;                               /* VF header type */
    u16 subsystem_vendor;                      /* VF subsystem vendor */
    u16 subsystem_device;                      /* VF subsystem device */
    resource_size_t barsz[PCI_SRIOV_NUM_BARS]; /* VF BAR size */
    bool drivers_autoprobe;                    /* Auto probing of VFs by driver */
};

/* 看门狗复位场景,该接口为自愈配置回填是否成功的判断;在dpdie复位等其他场景,该接口返回值与配置回填是否成功无关 */
int zxdh_pf_pcie_config_reload_check(struct dh_core_dev *dh_dev);

int is_zxdh_pf_vf_enable(struct dh_core_dev *dh_dev);
void clear_pf_sriov_status(struct dh_core_dev *dh_dev);

void set_pci_vpd_len_to_max(struct dh_core_dev *dh_dev, size_t vpd_max_size);


/**
 * @brief Set or Get vf bar addr object
 *
 * @param dh_dev:           dh_dev
 * @param bar_info:         SRIOV BAR信息
 * @param opt:              0为读取，其余为配置
 * @return int              0为成功，其余异常
 */
int handle_vf_bar_addr(struct dh_core_dev *dh_dev, struct pcie_sriov_bar_info *bar_info, u32 opt);

#endif
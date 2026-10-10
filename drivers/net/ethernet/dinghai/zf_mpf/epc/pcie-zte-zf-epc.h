#ifndef __PCIE_ZTE_ZF_EPC_H
#define __PCIE_ZTE_ZF_EPC_H

#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/dinghai/dh_cmd.h>
#include <linux/dinghai/driver.h>
#include <linux/dinghai/log.h>
#include <linux/export.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/msi.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/pci-ep-cfs.h>
#include <linux/pci-epc.h>
#include <linux/pci-epf.h>
#include <linux/pci-p2pdma.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/t10-pi.h>

#include "../zf_mpf.h"

#ifdef _cplusplus
extern "C" {
#endif

#define ZF_DISABLE                    0x0
#define ZF_ENABLE                     0x1
#define CFG_FILE_MODE_ENABLE          0

// ZF INFO
#define PCI_VENDOR_ID_ZTE             0x1CF2
#define PCI_DID_DPUA_VSOCK_VF         0x8038
#define PCI_DID_DPUB_VSOCK_VF         0x8039
#define PCIE_DPU_EP_CLASS_NAME        32

#define PCIE_DPU_EP_NUMS              4
#define EP_ID_LEN                     4
#define PCIE_DPU_PF_NUMS              8
#define PCIE_DPU_IATU_NUMS            41
#define PCIE_VF_BARS_OFF              7
#define PCIE_BAR0_ADDR_SET(off)       (((off) & 0xFFFF) | (((off) & 0xFFFFFFFFFFFF0000) << EP_ID_LEN))

#define PCIE_DPU_PF_INITIAL_ID        0x80371CF2
#define PCIE_DPU_PF_DEFAUTL_ID1       0x10011AF4
#define PCIE_DPU_PF_DEFAUTL_ID2       0x80531CF2
#define PCIE_DPU_PF_DEFAUTL_ID3       0x80371CF2
#define PCIE_DPU_PF_DEFAUTL_ID4       0x80331CF2
#define PCIE_DPU_PF_DEFAUTL_CLASSCODE 0x02000000

// dpu func_no&addr set
#define PCIE_DPU_EP_FUNC_IS_VF        BIT(7)
#define PCIE_DPU_EP_GET_PF_NO         0x7F
#define DBI_VF_CFG_OFFSET_BIT         4
#define VF_ACT_BIT                    BIT(3)
#define isPF(func_no)                 ((func_no & PCIE_DPU_EP_FUNC_IS_VF) ? 0 : 1)

#define ZF_PREFIX_ADDR                0x9000000000000000
#define DEFAULT_DBI_ATU_OFFSET        0x6000000
#define PCIE_DPU_EP_DBI_SIZE          0x8000000
#define PCIE_DPU_EP_OUTBOUND_SIZE     0x40000000
#define PCIE_DPU_EP_DBI2_OFFSET       0x2000000
#define PCIE_DPU_MPF_CSR_OFFSET       0x14000
#define PCIE_DPU_MPF_CSR_ADDR(offset) PCIE_BAR0_ADDR_SET(PCIE_DPU_MPF_CSR_OFFSET + offset)
#define PCIE_DPU_EP_FUNC_CFG_SIZE     0X1000
#define PCIE_DPU_EP_CSR_SIZE          0x2000
#define PCIE_DPU_EP_CSR_LTSSM_ADDR    0x150
#define LTSSM_EN_VAL                  0x11
#define PCIE_DPU_EP_CSR_PRST_ADDR     0x448
#define PCIE_DPU_EP_CSR_VIRT_ADDR     0x1200
#define BAR4_DEFAULT_SIZE             0x10000

// epc features
#define PCIE_DPU_EP_REAERVED_BAR      0x30
#define PCIE_DPU_EP_BAR_FIXED_64BIT   0x15
#define PCIE_DPU_EP_ALIGN             0x1000

#define EP_ID_SHIFT                   16
#define EP_ADDR_MASK                  ((1 << EP_ID_SHIFT) - 1)
#define EP_DPU_PA(addr, ep_id)                                     \
    ((addr & ~EP_ADDR_MASK) << EP_ID_LEN | (addr & EP_ADDR_MASK) | \
     ep_id << EP_ID_SHIFT)

// epc cdev
#define PCIE_EPC_DRIVER_MAJOR 0
#define PCIE_EPC_DRIVER_NAME  "pcie_cfg_driver"
#define PCIE_EPC_DRIVER_CLASS "pcie_cfg_class"
#define PCIE_EPC_IOCTL_TYPE   '>'
#define PCIE_EPC_IOCTL_MAGIC  100 /* random */
#define PCIE_EPC_GET_CONFIG   (_IO(PCIE_EPC_IOCTL_TYPE, PCIE_EPC_IOCTL_MAGIC + 0))
#define PCIE_EPC_SET_CONFIG   (_IO(PCIE_EPC_IOCTL_TYPE, PCIE_EPC_IOCTL_MAGIC + 1))

// bar msg
#define RECV_BUFFER_SIZE      60
#define BAR_MSG_HEADER_SIZE   4

// PCIE Register
#if 1
#define LINK_WAIT_MAX_RETRIES                    10
#define LINK_WAIT_USLEEP_MIN                     90000
#define LINK_WAIT_USLEEP_MAX                     100000
#define LINK_WAIT_MAX_IATU_RETRIES               0x10
#define LINK_WAIT_IATU                           1

// PCIE_PORT_DEBUG1 cap
#define PCIE_PORT_DEBUG1                         0x72C
#define PCIE_PORT_DEBUG1_ZTE_ZF_LINK_UP          BIT(4)
#define PCIE_PORT_DEBUG1_LINK_IN_TRAINING        BIT(29)

// PCIE CAP
#define PCIE_ECAP_POINTER_OFF                    0x100
#define PCIE_ECAP_VSEC_ID                        0x0B

#define PCIE_NEXT_BAR_OFFSET                     0x4
#define PCIE_DEFAULT_BAR_FLAG                    (BIT(3) | BIT(2))
#define PCIE_SRIOV_ECAP_DEVICE_ID                0x1a
#define PCIE_SRIOV_ECAP_BAR0_OFFSET              0x24
#define PCIE_SRIOV_ECAP_BAR4_OFFSET              0x34
#define PCIE_SRIOV_CTRL                          0x08
#define PCIE_SRIOV_TOTAL_VFS                     0x0e
#define PCIE_SRIOV_CTRL_VFE                      0x01

#define PCIE_MSI_ADDR_LO                         0x820
#define PCIE_MSI_ADDR_HI                         0x824
#define PCIE_MSI_INTR0_ENABLE                    0x828
#define PCIE_MSI_INTR0_MASK                      0x82C
#define PCIE_MSI_INTR0_STATUS                    0x830

#define PCIE_PORT_MULTI_LANE_CTRL                0x8C0
#define PORT_MLTI_UPCFG_SUPPORT                  BIT(7)

/* ATU register*/
#define PCIE_ATU_CR1                             0x904
#define PCIE_ATU_TYPE_MEM                        0x0
#define PCIE_ATU_TYPE_IO                         0x2
#define PCIE_ATU_FUNC_NUM(pf)                    ((pf) << 20)
#define PCIE_ATU_FUNC_NUM_MASK                   0xF00000
#define PCIE_ATU_BAR_NUM_MASK                    0x700
#define PCIE_ATU_ENABLE                          BIT(31)
#define PCIE_ATU_BAR_MODE_ENABLE                 BIT(30)
#define PCIE_ATU_CFG_SHIFT_MODE                  BIT(28)
#define PCIE_ATU_DMA_BYPSS                       BIT(27)
#define PCIE_ATU_FUNC_NUM_MATCH_EN               BIT(19)
#define PCIE_ATU_VFBAR_MATCH_MODE_ENABLE         BIT(26)
#define PCIE_ATU_VF_MATCH_ENABLE                 BIT(20)
#define PCIE_ATU_OB_VF_ACTIVE                    BIT(31)

/*MSIX register*/
#define MSIX_ADDRESS_MATCH_LOW_OFF               0x940
#define MSI_ADDRESS_MATCH_EN                     BIT(0)
#define MSIX_ADDRESS_MATCH_HIGH_OFF              0x944
#define MSIX_DOORBELL_OFF                        0x948
#define MSIX_DOORBELL_PF                         24
#define MSIX_DOORBELL_PF_MASK                    0x1F
#define MSIX_DOORBELL_VF                         16
#define MSIX_DOORBELL_VF_MASK                    0xFF
#define MSIX_DOORBELL_VF_ACTIVE                  BIT(15)
#define MSIX_DOORBELL_VECTOR                     0
#define MSIX_DOORBELL_VECTOR_MASK                0x7FF

#define LINK_WAIT_DMA                            20

#define PCIE_MISC_CONTROL_1_OFF                  0x8BC
#define PCIE_DBI_RO_WR_EN                        BIT(0)

#define PCIE_MSIX_DOORBELL                       0x948
#define PCIE_MSIX_DOORBELL_PF_SHIFT              24
#define PCIE_MSIX_DOORBELL_VF_SHIFT              16
#define MSIX_DOORBELL_VF_ACTIVE                  BIT(15)

/* Register address builder */
#define PCIE_GET_ATU_OUTB_UNR_REG_OFFSET(region) ((region) << 9)
#define PCIE_GET_ATU_INB_UNR_REG_OFFSET(region)  (((region) << 9) | BIT(8))

#define PCIE_ATU_UNR_REGION_CTRL1                0x00
#define PCIE_ATU_UNR_REGION_CTRL2                0x04
#define PCIE_ATU_UNR_LOWER_BASE                  0x08
#define PCIE_ATU_UNR_UPPER_BASE                  0x0C
#define PCIE_ATU_UNR_LOWER_LIMIT                 0x10
#define PCIE_ATU_UNR_LOWER_TARGET                0x14
#define PCIE_ATU_UNR_UPPER_TARGET                0x18
#define PCIE_ATU_UNR_REGION_CTRL3                0x1c
#define PCIE_ATU_UNR_UPPER_LIMIT                 0x20
#define PCIE_ATU_INCREASE_REGION_SIZE            BIT(13)

#endif

enum pcie_dpu_func_type {
    PCIE_FUNC_TYPE_PF = 0,
    PCIE_FUNC_TYPE_VF,
    PCIE_FUNC_TYPE_NUM
};

enum pcie_dpu_as_type {
    PCIE_DPU_AS_UNKNOWN,
    PCIE_DPU_AS_MEM,
    PCIE_DPU_AS_IO,
};

enum pcie_dpu_region_type {
    PCIE_DPU_REGION_UNKNOWN,
    PCIE_DPU_REGION_INBOUND,
    PCIE_DPU_REGION_OUTBOUND,
};

enum pcie_epcbar_msg_cmd {
    CMD_GET_CONFIG = 0,
    CMD_SET_CONFIG,
    CMD_SET_IATU,
    CMD_GET_PF_INFO_BY_FILE,
    BAR_MSG_CMD_END,
};

struct pcie_dpu_ep_func {
    struct list_head list;
    u8 func_no;
    u8 vfunc_no;
    u8 msi_cap;  /* MSI capability offset */
    u8 msix_cap; /* MSI-X capability offset */
};

struct pcie_pf_cfg_info {
    u32 vendorid;
    u32 deviceid;
    u32 class_revision;
    u32 subsys_vendor_id;
    u32 subsys_id;
};

struct pcie_dpu_ep {
    int ep_id;
    int permissible_pf_map;
    struct pci_epc *epc;
    struct platform_device *zf_pdev;
    struct platform_device *zf_pdev_dma;
    struct dma_device *wr_dd;
    struct dma_device *rd_dd;

    void __iomem *dbi_base;
    void __iomem *atu_base;

    struct list_head func_list;
    int bar_to_atu[PCIE_DPU_PF_NUMS][(PCI_STD_NUM_BARS * 2) + 1];
    int vf_total_num[PCIE_DPU_PF_NUMS];
    phys_addr_t *ob_src_addr;
    unsigned long *ib_window_map;
    unsigned long *ob_window_map;
    spinlock_t ib_window_lock;
    u32 num_ib_windows;
    u32 num_ob_windows;

    // 配置信息保存
    struct pcie_pf_cfg_info cfg_info[PCIE_DPU_PF_NUMS];
};

struct pcie_zf_ep {
    struct pci_dev *mpf_pdev;
    unsigned long dbi_paddr;
    unsigned long mpf_paddr;
    unsigned long vsock_paddr;
    unsigned long ob_size;
    void __iomem *dbi_vaddr;
    void __iomem *vsock_vaddr;
    void __iomem *mpf_vaddr;
    int dpu_ep_num;
    struct pcie_dpu_ep **dpu_ep_array;
    struct device_driver *dma_driver;
};

struct pcie_epc_ioctl_pf_cfg {
    u8 cmd_id;
    u8 ep_id;
    u8 pf_id;
    u16 vendorid;
    u16 deviceid;
    u16 deviceid_vf;
    u16 subsys_vendor_id;
    u16 subclass_code;
    u16 progif_code;
    u32 pf_bar0_size;
    u32 pf_bar2_size;
    u32 vf_bar0_size;
    u32 vf_bar2_size;
};

struct pcie_epc_iatu_cfg {
    u8 cmd_id;
    u8 ep_id;
    u8 pf_id;
    u8 is_pf;
    u8 bar_index;
    u8 iatu_win;
    u64 target_addr;
};

struct pcie_epc_ioctl_sel {
    u32 ioctl_cmd;
    int (*ioctl_func)(unsigned long long arg);
};

int pcie_zte_zf_epc_module_init(struct dh_core_dev *dh_dev, const struct pci_device_id *id);
int pcie_zf_dma_init(struct pcie_dpu_ep *dpu_dev, struct pci_dev *pdev);
void pcie_zte_zf_epc_free(struct dh_core_dev *dh_dev);
void pcie_zf_dma_free(struct pcie_dpu_ep *dpu_dev, struct pci_dev *pdev);
int zf_pcie_get_hdma_chan(struct pci_epc *epc, u8 func_no, u8 vfunc_no, struct dma_chan **rchan, struct dma_chan **wchan);
u32 cfg_phy_rmw(u64 phy_addr, u32 value, u32 mask);
int pcie_zte_epc_ob_read(struct pci_epc *epc, phys_addr_t phys_addr, unsigned int size, unsigned int *val);

#ifdef __cplusplus
}
#endif

#endif
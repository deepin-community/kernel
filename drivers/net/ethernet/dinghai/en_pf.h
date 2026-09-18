#ifndef __ZXDH_EN_PF_H__
#define __ZXDH_EN_PF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/dinghai/kcompat.h>
#include <linux/dinghai/device.h>
#include <linux/dinghai/driver.h>
#ifndef CGS_V5_693
#include <linux/refcount.h>
#endif
#include "plcr.h"
#ifdef CONFIG_DINGHAI_TSN
#include "en_tsn/zxdh_tsn.h"
#endif
#include "pcie_common.h"
#include <linux/dinghai/dinghai_irq.h>

#ifdef NEED_SYSFS_EMIT
int sysfs_emit(char *buf, const char *fmt, ...);
#endif

/* Common configuration */
#define ZXDH_PCI_CAP_COMMON_CFG 1
/* Notifications */
#define ZXDH_PCI_CAP_NOTIFY_CFG 2
/* ISR access */
#define ZXDH_PCI_CAP_ISR_CFG    3
/* Device specific configuration */
#define ZXDH_PCI_CAP_DEVICE_CFG 4
/* PCI configuration access */
#define ZXDH_PCI_CAP_PCI_CFG    5

#define ZXDH_PF_MAX_BAR_VAL     0x5
#define ZXDH_PF_ALIGN4          4
#define ZXDH_PF_ALIGN2          2
#define ZXDH_PF_MAP_MINLEN2     2

#define ZXDH_DEV_MAC_HIGH_OFFSET 4
#define ZXDH_DEV_SPEED_OFFSET 0x4c
#define ZXDH_DEV_DUPLEX_OFFSET 0x50
#define ZXDH_FW_COMPAT_OFFSET 0x5400
#define ZXDH_QUEUE_INFO_OFFSET 0x5480
#define ZXDH_FW_VERSION_OFFSET 0x530c
#define ZXDH_VENDOR_OFFSET 0x1B8C
#define ZXDH_FW_CAP_OFFSET 0x1000
#define ZXDH_CROSS_QUEUE_OFFSET        (ZXDH_FW_CAP_OFFSET + 0xB9C)
#define ZXDH_CROSS_QUEUE_ENABLE        (1)

#define ZXDH_FWSHARE_BASE_ADDR         0x5000
#define ZXDH_DEV_QUEUE_INFO_OFFSET     (ZXDH_FWSHARE_BASE_ADDR + 0x6a0)
#define ZXDH_PF_QUEUE_INFO_OFFSET      (ZXDH_FWSHARE_BASE_ADDR + 0x740)
#define ZXDH_VF_QUEUE_PAIRS_OFFSET     (ZXDH_FWSHARE_BASE_ADDR + 0x490)
#define ZXDH_VF_QUEUE_USER_OFFSET      (ZXDH_FWSHARE_BASE_ADDR + 0x744)   //内核态写，vf加载用户态程序时读
#define ZXDH_VF_MAX_QUEUE_USER_OFFSET  (ZXDH_FWSHARE_BASE_ADDR + 0x743)   //固件写:vf加载用户态程序时最大支持的队列对数
#define ZXDH_OVS_PF_VFID_OFFSET        (ZXDH_FWSHARE_BASE_ADDR + 0x77C)   //I511 OVS_PF的VFID
#define ZXDH_DEV_RO_OFFSET             (ZXDH_FWSHARE_BASE_ADDR + 0x3A0)   //存储RO功能状态
#define ZXDH_DEV_400G_MAC_STATS        (ZXDH_FWSHARE_BASE_ADDR + 0xb00)   //存储RO功能状态
#define ZXDH_CONFIG_BOND_CONFIG_OFFSET (ZXDH_FWSHARE_BASE_ADDR + 0x3EC)   //存储dhtool配置的网卡硬BOND配置【共4字节，使用低20个bit】
#define ZXDH_ACTIVE_BOND_CONFIG_OFFSET (ZXDH_FWSHARE_BASE_ADDR + 0x3F0)   //存储驱动当前使用的网卡硬BOND配置【共4字节，使用低20个bit】
#define ZXDH_PSN_FEATURE_INFO_OFFSET   (0x1016 )   //psn功能相关信息

#define ZXDH_NP_EXT_STATS_OFFSET        (ZXDH_FWSHARE_BASE_ADDR + 0xA00)
#define ZXDH_NP_EXT_STATS_SIZE          (512)

#define EPID_MASK_BIT                   (12)
#define PFID_MASK_BIT                   (8)
#define EPID_GEN_FROM_VPORT(a)          (((a) & ~BIT(15)) >> EPID_MASK_BIT)
#define GLOBAL_PF_IDX(a,b)              ((a) * ZXDH_PF_NUM_PER_EP + (((b) & ZXDH_PF_IDX_MASK) >> PFID_MASK_BIT))
#define GLOBAL_VF_IDX(a,b)              ((a) * ZXDH_VF_NUM_MAX + ((b) & ZXDH_VF_IDX_MASK))
#define ZXDH_MAX_QPS_NUM                (8)

#define ZXDH_CFG_NPSDK_TYPE     7
#define ZXDH_STOP_PXE_MODE      1

#define ZXDH_PANNEL_PORT_MAX    (10)

#define TO_EP4_ADDR(addr)       (((addr & 0xFFFFFFFFFFFF0000) << 4) | (addr & 0xFFFF))

#define ZXDH_EP_NUM             4
#define ZXDH_PF_NUM             40
#define ZXDH_QUEUE_PAIRS_MAX    32
#define ZXDH_VF_IDX_MASK        0xff
#define ZXDH_PF_IDX_MASK        0x700

#define RISC_TYPE_READ          0
#define RISC_FIELD_PANEL_ID     5
#define MAX_PANEL_ID            9

#define GET_COREDEV_TYPE(pdev)     \
    ((pdev->device == ZXDH_VF_DEVICE_ID) || (pdev->device == ZXDH_VF_E310_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_E312_DEVICE_ID) || (pdev->device == ZXDH_UPF_VF_I512_DEVICE_ID) || \
    (pdev->device == ZXDH_INICA_RDMA_VF_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_DPUB_RDMA_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_E316_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_E316_XPU_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_E316L_XPU_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_E312_XPU_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_E311_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_I511_DEVICE_ID) || \
    (pdev->device == ZXDH_INICD_NE0_VF_DEVICE_ID) || \
    (pdev->device == ZXDH_INICD_NE1_VF_DEVICE_ID) || \
    (pdev->device == ZXDH_INICD_NE2_VF_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_E310_RDMA_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_E310S_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_E312S_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_E312_RDMA_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_I510_SRIOV_SEC_DEVICE_ID) || \
    (pdev->device == CTC_VF_B512Y_DEVICE_ID) || \
    (pdev->device == CTC_VF_B522Y_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_E312S_D_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_E310_CMCC_DEVICE_ID) || \
    (pdev->device == ZXDH_INICE_RDMA_VF_DEVICE_ID) || \
    (pdev->device == ZXDH_INICF_RDMA_VF_DEVICE_ID) || \
    (pdev->device == ZXDH_INICG_RDMA_VF_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_E318_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_E318_XPU_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_DPUB_ROCE_RDMA0_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_DPUB_ROCE_RDMA1_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_E200_DEVICE_ID) || \
    (pdev->device == ZXDH_INICA_SRIOV_VF_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_SPNC52_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_DS25GE52_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_E316L_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_LNKX_2X200G_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_LNKX_1X400G_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_YUANZHI_2X25G_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_YUANZHI_2X100G_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_YUANZHI_2X100_OCP_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_YUANZHI_1X200G_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_YUANZHI_2X200G_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_YUANZHI_1X400G_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_MUCSE_2X200G_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_MUCSE_1X400G_DEVICE_ID) || \
    (pdev->device == ZXDH_VF_E310_03N00_DEVICE_ID)) ? \
    DH_COREDEV_VF : DH_COREDEV_PF  \


#define ZXDH_AUX_COMP_FLAG      1
#define ZXDH_AUX_COMP_FLAG_CHECK(pf_dev) \
    do {                                                   \
        if (pf_dev->aux_comp_flag != ZXDH_AUX_COMP_FLAG)   \
        {                                                  \
            return;                          \
        }                                                  \
    } while (0)

#define PORT_FLAGS_ALLOC_STAT      (1 << 0)

struct dh_core_dev;

struct zxdh_pf_adev {
    struct zxdh_auxiliary_device *adev;
    int32_t aux_idx;
};

struct zxdh_pannle_port
{
    uint8_t pannel_id;
    uint8_t phyport;
    uint8_t link_check_bit;
    uint8_t flags;
}  __attribute__((packed));

struct nic_sn_info
{
    uint8_t fixed_sn_valid;
    uint8_t pseudo_sn_valid;
    uint8_t pseudoed_before;
    uint8_t rsv[1];
#define SN_CODE_LENGTH  (12)
    uint8_t sn_code[SN_CODE_LENGTH];
}__attribute__((packed));

struct zxdh_port_resource
{
    uint8_t pannel_num;
    uint8_t bond_num;
    uint8_t bond_idx;
    uint8_t rsv;
    struct zxdh_pannle_port port[ZXDH_PANNEL_PORT_MAX];
}  __attribute__((packed));

#define DH_HEALTH_ATTR_NUM (5)
struct zxdh_core_health {
    struct timer_list timer;
    struct core_health riscv;
    struct core_health m7;
    uint64_t m7_log_offset;
    uint64_t riscv_crdump_size;
    unsigned long synd;
    uint8_t fatal;
    uint8_t health_version;
    uint16_t pstate_change_cnt;
    uint16_t recovery_cnt;
    bool health_supported;
    bool reset_done;
    uint8_t fatal_detect_cnt;
    uint8_t next_load_index;
    uint16_t synd_statics[64];
    /* wq spinlock to synchronize draining */
    spinlock_t wq_lock;
    struct workqueue_struct *wq;
    unsigned long flags;
    struct work_struct m7_bbx_saving_work;
    struct work_struct riscv_log_saving_work;
    struct work_struct riscv_bbx_saving_work;
    struct delayed_work ep_power_state_check_work;
    struct work_struct fw_fatal_err_work;
    struct work_struct dh_reset_work;
    struct kobj_attribute attrs[DH_HEALTH_ATTR_NUM];
};

struct zxdh_fw_compat {
    uint8_t module_id;
    uint8_t major;
    int8_t fw_minor;
    uint8_t drv_minor;
    uint16_t patch;
    uint16_t rsv;
} __attribute__((packed));

/* If feature bits are added, add the following enumeration types. */
enum fw_feature_bit
{
    FW_FEATURE_COMPAT = 0,
    FW_FEATURE_RDMA   = 1,
    FW_FEATURE_STD    = 2,
    FW_FEATURE_NPSTAT = 3,
    FW_FEATURE_SEC    = 4,
    FW_FEATURE_QUEUE_RESET = 5,
    FW_FEATURE_PFM    = 6,
    FW_FEATURE_RO_CFG = 10,
    FW_FEATURE_COREDUMP = 16,
    FW_FEATURE_400G_MAC_STATS = 20,
    FW_FEATURE_DHTS_BOND_CFG = 21,
    FW_FEATURE_SPLIT_PACKED = 22,
    FW_FEATURE_PRIO_STATS = 23,
    FW_FEATURE_VPD_CFG    = 25,
    FW_FEATURE_NP_NPPU_TCAM_PFC_MAP = 27,
    FW_FEATURE_VXLAN_TSO_CKSUM = 32,
    FW_FEATURE_MAX,
};

#define FW_FEATURE_GET(value, bit) (((value) >> (bit)) & 1)

struct firmware_capability
{
    union
    {
        struct
        {
            uint8_t ddr_aval        : 1;
            uint8_t multihost_aval  : 1;
            uint8_t riscv_init_done : 1;
            uint8_t os_type         : 3;    /* default:0,cgel:1,zios:2,cgel_to_zios:3 */
            uint8_t board_type;             /* enum dh_board_type */
            uint8_t scen_type : 2;
            uint8_t is_lowlatency : 1;
            uint8_t bond_pf_pnl_num;

            uint8_t stat_power_mask;
            uint8_t ctrl_power_mask;
            uint64_t fw_feature;            /* enum fw_feature_bit */
            uint16_t fw_feature_extra;      /* enum fw_feature_bit */
            uint32_t pf_rate_default;
        }__attribute__((packed));
        uint8_t rsv[3000];
    };
    uint32_t pcie_feature;
}__attribute__((packed));

enum pcie_feature_bit
{
    PCIE_FW_EXT = 0,
    PCIE_HP = 1,
    PCIE_MULTI_SOCKET = 2,
    PCIE_FEATURE_MAX,
};

struct zxdh_pf_queue_info {
    uint8_t pf_qp;          /* 配置的PF队列队数 */
    uint8_t vf_qp;          /* 配置的VF队列队数 */
} __attribute__((packed));

/* pf域队列总池子,以全局pf编号为索引 */
struct zxdh_dev_queue_info {
    uint16_t total_qp;      /* PF及其下VF总共支持的队列队数 */
    uint16_t start_id;      /* 起始队列号*/
} __attribute__((packed));

struct zxdh_np_ext_stats {
    uint32_t rx_vport2np_packets;
} __attribute__((packed));

struct rdma_irq_info {
    bool is_request;
    uint32_t irq;
    void *data;
};

enum {
    act_pf_dev_info,
    act_aux_dev_info,
};

struct dhinfo_sysfs {
    struct kobj_attribute attr;
};

struct zxdh_pf_device {
    struct list_head virtqueues;

    struct zxdh_pf_pci_common_cfg __iomem *common;
    /* Device-specific data (non-legacy mode)  */
    /* Base of vq notifications (non-legacy mode). */
    void __iomem *device;
    void __iomem *notify_base;
    void __iomem *pf_sriov_cap_base;
    /* Physical base of vq notifications */
    resource_size_t notify_pa;
    /* Where to read and clear interrupt */
    uint8_t __iomem *isr;
    /* So we can sanity-check accesses. */
    size_t notify_len;
    size_t device_len;
    /* Capability for when we need to map notifications per-vq. */
    int32_t notify_map_cap;
    /* Multiply queue_notify_off by this value. (non-legacy mode). */
    uint32_t notify_offset_multiplier;
    int32_t modern_bars;

    uint64_t pci_ioremap_addr[6];
    uint64_t qtlb_offset;
    struct pcie_sriov_bar_info bar_info;

    uint32_t speed;
    uint32_t autoneg_enable;
    uint32_t supported_speed_modes;
    uint32_t advertising_speed_modes;
    uint8_t  duplex;
    bool bar_chan_valid;
    uint16_t pcie_id;
    uint16_t slot_id;
    uint16_t vport;
    struct zxdh_vf_item *vf_item;
    uint16_t num_vfs;
    bool vepa;
    uint8_t  phy_port;

    bool link_up;
    bool fast_unload;
    struct work_struct riscv_ready_work;
    struct work_struct riscv2pf_msg_proc_work;
    struct work_struct vf2pf_msg_proc_work;
    struct work_struct link_info_irq_update_vf_bond_pf_work;
    struct work_struct init_vf_link_info_work;
    struct work_struct riscv_ext_pps_work;
    struct work_struct riscv_local_pps_work;
    struct work_struct mac_info_pf_work;
    struct work_struct rdma_dev_proc_work;
    struct work_struct rdma_dev_event_work;

    uint64_t sriov_bar_size;

    struct zxdh_plcr_table plcr_table;
    struct zxdh_sriov_sysfs sriov;
    struct zxdh_pf_adev *adevs_table;
    int32_t adevs_num;

    struct zxdh_port_resource port_resource;
    struct zxdh_lag *ldev;
    int32_t pannel_port_num;

    /* initialization completion flag */
    uint8_t aux_comp_flag;
    uint8_t bond_num;
    struct zxdh_ptp_private *ptp;
#ifdef CONFIG_DINGHAI_TSN
    struct zxdh_tsn_private *tsn;
#endif
    struct zxdh_ipv6_mac_tbl *ip6mac_tbl;
    uint32_t dev_cfg_bar_off;
    struct zxdh_core_health health;
    struct zxdh_fw_compat fw_compat;
    uint8_t sn_code[SN_CODE_LENGTH];
    uint8_t board_type;
    uint8_t product_type;
    uint8_t vq_pairs;
    uint64_t mcode_feature;
    struct firmware_capability fwcap;
    struct zxdh_np_ext_stats np_ext_stats;
    uint16_t epbdf;
    uint32_t rp_sbdf;
    bool is_multi_ep;
    bool is_hwbond;
    bool is_rdma_aux_plug;
    bool is_primary_port;
    uint64_t spec_sbdf;    /* special bdf， 用于rdma持久化配置文件路径创建 */
    bool quick_remove;
    uint8_t panel_id;
    bool is_special_bond;
    bool is_bond_slave;
    uint16_t bond_link_info;
    uint8_t no_bondpf_panel; //不属于bond pf的面板个数
    uint8_t bond_port_type; //硬bond port类型（两口板型/三口及以上板型）
    struct mutex irq_lock;
    struct rdma_irq_info rdma_irqs[ZXDH_RDMA_CHANNELS_NUM];
    bool packed_status;
    uint16_t pre_bond_qidx;
    struct dhinfo_sysfs dhinfo;
};

struct slot_id_array {
    DPP_PF_INFO_T pf_info;
    uint8_t sn_code[SN_CODE_LENGTH];
    uint8_t refcnt;
    uint8_t board_type;
};

struct zxdh_ipv6_mac_entry {
    spinlock_t lock;
    refcount_t refcnt;
    struct list_head list;
    uint8_t ipv6_mac[ETH_ALEN];
};

struct zxdh_ipv6_mac_tbl {
    unsigned int ip6mact_size;
    struct mutex mlock;
    struct list_head ip6mac_free_head;
    void *ip6mac_entry_list;
    struct list_head hash_list[];
};

bool zxdh_pf_is_bond(struct dh_core_dev *dh_dev);
bool zxdh_pf_is_upf(struct dh_core_dev *dh_dev);
int32_t zxdh_pf_msg_send_cmd(struct dh_core_dev *dh_dev, uint16_t module_id, void *msg, void *ack, \
                             struct zxdh_bar_extra_para *para);
struct zxdh_vf_item *zxdh_pf_get_vf_item(struct dh_core_dev *dh_dev, uint16_t vf_idx);
int zxdh_pf_get_pannel_port_num(struct dh_core_dev *dh_dev);
void zxdh_pf_set_vf_mac_reg(struct zxdh_pf_device *pf_dev, uint8_t *mac, int32_t vf_id);
void zxdh_unload_one(struct dh_core_dev *dh_dev);
int zxdh_load_one(struct dh_core_dev *dh_dev);
int zxdh_pf_status_ok(struct dh_core_dev *dh_dev);
int zxdh_vf_wait_pf_ok(struct dh_core_dev *dh_dev);
void zxdh_pf_set_bond_num(struct dh_core_dev *dh_dev, bool add);
int zxdh_pf_pcie_config_store(struct dh_core_dev *dh_dev);
int32_t zxdh_pf_get_hp_irq_ctrl_status(struct dh_core_dev *dev);
int32_t zxdh_pf_rp_config_init(struct dh_core_dev *dev);
uint32_t zxdh_pf_get_dev_type(struct dh_core_dev *dh_dev);
void zxdh_pf_get_vf_mac(struct dh_core_dev *dh_dev, uint8_t *mac, int32_t vf_id);
bool zxdh_pf_is_pcie_feature_support(struct dh_core_dev *dh_dev, uint32_t feature);
void zxdh_pf_set_bond_link_info(struct dh_core_dev *dh_dev, uint16_t bond_link_info);
void zxdh_pf_set_bond_slave_flag(struct dh_core_dev *dh_dev, bool is_bond_slave);
int create_directory_recursion(const char *path);
int get_zxdh_log_dir(char *buffer, size_t buf_size);
uint8_t zxdh_health_check_fatal_sensors(struct zxdh_core_health *health);
int wait_vital(struct dh_core_dev *dh_dev);
int zxdh_health_wait_dh_ok(struct dh_core_dev *dh_dev);
bool is_sn_invalid(uint8_t sn_code[]);

#ifdef __cplusplus
}
#endif

#endif

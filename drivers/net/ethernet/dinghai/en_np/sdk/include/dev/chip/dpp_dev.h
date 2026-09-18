/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_dev.h
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 完成日期 : 2014/02/10
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1: 代码规范性修改
* 修改日期:  2014/02/10
* 版 本 号:
* 修 改 人:  丁金凤
* 修改内容:
***************************************************************/

#ifndef _DPP_DEV_H_
#define _DPP_DEV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/pci.h>
#include "zxic_common.h"
#include "dpp_type_api.h"
#define DEV_HASH_FUNC_ID_NUM          (4)

#define DPP_KEYSIG_DEBUG           (1)
#define DPP_DEV_CHANNEL_MAX        (2)  /* 最多支持6个芯片 */
#define DPP_DEV_PPU_CLS_MAX        (6)  /* 芯片支持4级cluster */
#define DPP_DEV_PPU_INSTR_REG_NUM  (3)  /* 芯片支持4级cluster 共2个指令空间*/

#define DPP_DEV_ME_MAX             (8)  /* 每级cluster支持8个me */
#define DPP_DEV_SDT_ID_MAX         (256U)
#define DPP_DTB_QUEUE_MAX          (128)

#define DPP_CHIP_DPP               (0x279221)

#define X86_ADDR_2_ARRCH64(X86_ADDR) (((X86_ADDR & (~0xFFFF)) << 4) | (X86_ADDR & 0xFFFF))

#define DPP_PCIE_SLOT_MAX          (64)
#define DPP_PCIE_CHANNEL_MAX       (64)
#define DPP_PCIE_CHANNEL_ID(VPORT) (((((VPORT) & 0x7000) >> 9) | (((VPORT) & 0x0700) >> 8)) & 0x3F)

#define DEV_ID(DEV)                (((DPP_DEV_T *)(DEV))->device_id)
#define DEV_PCIE_SLOT(DEV)         (((DPP_DEV_T *)(DEV))->pcie_channel.slot)
#define DEV_PCIE_VPORT(DEV)        (((DPP_DEV_T *)(DEV))->pcie_channel.vport)
#define DEV_PCIE_DEV(DEV)          (((DPP_DEV_T *)(DEV))->pcie_channel.device)
#define DEV_PCIE_ADDR(DEV)         (((DPP_DEV_T *)(DEV))->pcie_channel.base_addr)
#define DEV_PCIE_OFFSET_ADDR(DEV)  (((DPP_DEV_T *)(DEV))->pcie_channel.offset_addr)
#define DEV_PCIE_ID(DEV)           (((DPP_DEV_T *)(DEV))->pcie_channel.pcie_id)
#define DEV_PCIE_LOCK(DEV)         (((DPP_DEV_T *)(DEV))->pcie_channel.device_lock)
#define DEV_PCIE_BAR_MSG_NUM(DEV)  (((DPP_DEV_T *)(DEV))->pcie_channel.bar_msg_num)

#define DEV_PCIE_MSG_OFFSET_ADDR   (0x2000)
#define DEV_PCIE_MSG_ADDR(DEV)     (DEV_PCIE_ADDR(DEV) + DEV_PCIE_MSG_OFFSET_ADDR)
#define DEV_PCIE_REG_ADDR(DEV)     (DEV_PCIE_ADDR(DEV) + DEV_PCIE_OFFSET_ADDR(DEV) - SYS_NP_BASE_ADDR1)

typedef struct dpp_pf_info_t
{
    ZXIC_UINT16 slot;
    ZXIC_UINT16 vport;
} DPP_PF_INFO_T;

typedef struct dpp_pcie_channel_t
{
    ZXIC_UINT16 is_used;       /* 0空闲，1已使用 */
    ZXIC_UINT16 slot;
    ZXIC_UINT16 vport;
    ZXIC_UINT16 pcie_id;
    ZXIC_ADDR_T base_addr;
    ZXIC_ADDR_T offset_addr;
    struct pci_dev* device;
    ZXIC_MUTEX_T*   device_lock;
    ZXIC_UINT32 bar_msg_num;
    ZXIC_UINT32 hash_index;
    ZXIC_UINT32 dev_status;
    ZXIC_UINT32 dump_dma_size;
    ZXIC_ADDR_T dump_dma_phy_addr;
    ZXIC_ADDR_T dump_dma_vir_addr;
} DPP_PCIE_CHANNEL_T;

typedef struct dpp_dev_t
{
    ZXIC_UINT32 device_id;
    DPP_PCIE_CHANNEL_T pcie_channel;
} DPP_DEV_T;

/**  底层设备硬件读写接口指针*/
typedef DPP_STATUS (*DPP_DEV_WRITE_FUNC)(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 size, ZXIC_UINT32 *p_data);
typedef DPP_STATUS (*DPP_DEV_READ_FUNC)(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 size, ZXIC_UINT32 *p_data);
typedef DPP_STATUS (*DPP_ACCESS_SWITCH_FUNC)(ZXIC_UINT32 dev_id, ZXIC_UINT32 access_type);

/**  设备访问类型*/
typedef enum dpp_dev_access_type_e
{
    DPP_DEV_ACCESS_TYPE_PCIE = 0, /**<  @brief PCIe访问*/
    DPP_DEV_ACCESS_TYPE_RISCV = 1, /**<  @brief RISCV访问*/
} DPP_DEV_ACCESS_TYPE_E;

/**  设备类型*/
typedef enum dpp_dev_type_e
{
    DPP_DEV_TYPE_SIM  = 0,  /**<  @brief 仿真器设备*/
    DPP_DEV_TYPE_VCS  = 1,  /**<  @brief VCS设备*/
    DPP_DEV_TYPE_CHIP = 2,  /**<  @brief asci芯片设备*/
    DPP_DEV_TYPE_FPGA = 3,  /**<  @brief fpga设备*/
    DPP_DEV_TYPE_PCIE_ACC = 4,
    DPP_DEV_TYPE_INVALID,
} DPP_DEV_TYPE_E;

/**  设备版本*/
typedef enum dpp_chip_version_e
{
    DPP_CHIP_VERSION_DPP   = 0U,  /**<  @brief DPP */
    DPP_CHIP_VERSION_DPP_P = 1U,  /**<  @brief DPP+ */
    DPP_CHIP_VERSION_INVALID,
} DPP_CHIP_VERSION_E;

/**  互斥锁类型*/
typedef enum dpp_dev_mutex_type_e
{
    DPP_DEV_MUTEX_T_REG   = 0,  /**<  @brief 寄存器操作互斥锁       */
    DPP_DEV_MUTEX_T_OAM   = 1,  /**<  @brief OAM模块操作互斥锁      */
    DPP_DEV_MUTEX_T_ETM   = 2,  /**<  @brief ETM模块操作互斥锁       */
    DPP_DEV_MUTEX_T_DDR   = 4,  /**<  @brief DDR模块操作互斥锁      */
    DPP_DEV_MUTEX_T_IND   = 5,  /**<  @brief RAM间接读写操作互斥锁  */
    DPP_DEV_MUTEX_T_ETCAM = 6,  /**<  @brief ETCAM间接读写操作互斥锁*/
    DPP_DEV_MUTEX_T_MMU   = 7,  /**<  @brief MMU间接读写操作互斥锁  */
    DPP_DEV_MUTEX_T_CAR0  = 8,  /**<  @brief CAR0模块操作互斥锁     */
    DPP_DEV_MUTEX_T_ALG       = 9, /**<  @brief ALG间接读写操作互斥锁  */
    DPP_DEV_MUTEX_T_NPPU      = 10, /**<  @brief nppu间接读写操作互斥锁  */
    DPP_DEV_MUTEX_T_SMMU0     = 11, /**<  @brief smmu0 模块操作互斥锁  */
    DPP_DEV_MUTEX_T_SMMU1     = 12, /**<  @brief smmu1 模块操作互斥锁  */
    DPP_DEV_MUTEX_T_ETM_2ND   = 13, /**<  @brief ETM模块二层间接表操作互斥锁 */
    DPP_DEV_MUTEX_T_LPM       = 14, /**<  @brief LPM模块操作互斥锁 */
    DPP_DEV_MUTEX_T_CRM_TEMP  = 15, /**<  @brief 温度获取操作互斥锁 */
    DPP_DEV_MUTEX_T_SIM       = 16, /**<  @brief 仿真器socket通信操作互斥锁 */
    DPP_DEV_MUTEX_T_DTB       = 17, /**<  @brief DTB队列操作互斥锁 */
    DPP_DEV_MUTEX_T_DTB_RB    = 18,
    DPP_DEV_MUTEX_T_PKTRX_MF_GLB_CFG_0    = 19,
    DPP_DEV_MUTEX_T_PKTRX_MF_GLB_CFG_1    = 20,
    DPP_DEV_MUTEX_T_PKTRX_MF_GLB_CFG_2    = 21,
    DPP_DEV_MUTEX_T_PKTRX_MF_GLB_CFG_3    = 22,
    DPP_DEV_MUTEX_T_SELF_RECOVER          = 23,/**<  @brief 自愈恢复互斥锁 */
    DPP_DEV_MUTEX_T_PKTRX_MF_UDF_CFG      = 24,
    DPP_DEV_MUTEX_T_MAX
} DPP_DEV_MUTEX_TYPE_E;

typedef enum module_init_e
{
    MODULE_INIT_NPPU = 0,  /**<  @brief 01:pktrx  0x2:pbu  0x4:odma*/
    MODULE_INIT_PPU,
    MODULE_INIT_SE,    /**<  @brief corresponding bits indicacte SE submodule, 0:smmu0 1:smmu1 2:alg 3:as 4:etcam 5:stat 6:reg_fifo 31:SE_init_done*/
    MODULE_INIT_ETM,   /**<  @brief bit31:tm  bit0:olif  bit1:cgavd  bit2:tmmu  bit3:shap  bit4:crdt  bit5:qmu */
    MODULE_INIT_DLB,   
    MODULE_INIT_TRPG,   
    MODULE_INIT_TSN,   
    MODULE_INIT_MAX
} MODULE_INIT_E;

typedef struct dpp_dev_cfg_t
{
    ZXIC_UINT32 device_id;           /* 设备号 */
    DPP_DEV_TYPE_E dev_type;    /* 设备类型: 0-SIM, 1-VCS, 2-CHIP(ASIC), 3-FPGA. */
    ZXIC_UINT32 chip_ver;            /* 设备类型: 0-DPP, 1-DPP+ */
    ZXIC_UINT32 access_type;         /* 访问类型: 0-PCIe, 1-RISCV. */
    ZXIC_ADDR_T pcie_addr;           /* PCIe映射地址 */
    ZXIC_ADDR_T riscv_addr;           /* RISCV映射地址 */
    ZXIC_ADDR_T dma_vir_addr;        /* DMA空间映射地址 */
    ZXIC_ADDR_T dma_phy_addr;        /* 芯片地址相对偏移 */
    ZXIC_UINT32 init_flags[MODULE_INIT_MAX];
    DPP_DEV_WRITE_FUNC p_pcie_write_fun;  /**  PCIe硬件写回调函数 */
    DPP_DEV_READ_FUNC  p_pcie_read_fun;   /**  PCIe硬件读回调函数 */
    DPP_DEV_WRITE_FUNC p_riscv_write_fun;  /**  RISCV硬件写回调函数 */
    DPP_DEV_READ_FUNC  p_riscv_read_fun;   /**  RISCV硬件读回调函数 */  
    ZXIC_MUTEX_T    reg_opr_mutex;     /**  寄存器操作互斥量 */
    ZXIC_MUTEX_T    oam_mutex;         /**  OAM硬件操作互斥锁  */
    ZXIC_MUTEX_T    etm_mutex;         /**  ETM硬件操作互斥锁   */
    ZXIC_MUTEX_T    ddr_mutex;         /**  DDR硬件操作互斥锁  */
    ZXIC_MUTEX_T    ind_mutex;         /**  RAM间接操作互斥锁  */
    ZXIC_MUTEX_T    etcam_mutex;       /**  ETCAM硬件操作互斥锁 */
    ZXIC_MUTEX_T    car0_mutex;        /**  CAR0硬件操作互斥锁 */
    ZXIC_MUTEX_T    alg_mutex;         /**  alg硬件操作互斥锁*/
    ZXIC_MUTEX_T    nppu_mutex;        /**  nppu硬件操作互斥锁*/
    ZXIC_MUTEX_T    smmu0_mutex;         /**  smmu0 硬件操作互斥锁*/
    ZXIC_MUTEX_T    smmu1_mutex;         /**  smmu1 硬件操作互斥锁*/
    ZXIC_MUTEX_T    etm_2nd_mutex;       /**  ETM 二层间接表 硬件操作互斥锁*/
    ZXIC_MUTEX_T    lpm_mutex;           /**  lpm配置 操作互斥锁*/
    ZXIC_MUTEX_T    crm_temp_mutex;      /**  温度获取 操作互斥锁*/
    ZXIC_MUTEX_T    sim_mutex;           /**  仿真器socket通信 操作互斥锁*/
    ZXIC_MUTEX_T    dtb_mutex;           /** DTB操作互斥锁*/
    ZXIC_MUTEX_T    pktrx_mf_glb_cfg_mutex_0;           /** PKTRX全局配置区互斥锁*/
    ZXIC_MUTEX_T    pktrx_mf_glb_cfg_mutex_1;           /** PKTRX全局配置区互斥锁*/
    ZXIC_MUTEX_T    pktrx_mf_glb_cfg_mutex_2;           /** PKTRX全局配置区互斥锁*/
    ZXIC_MUTEX_T    pktrx_mf_glb_cfg_mutex_3;           /** PKTRX全局配置区互斥锁*/
    ZXIC_MUTEX_T    self_recover_mutex;           /** 自愈恢复互斥锁*/
    ZXIC_MUTEX_T    pktrx_mf_udf_cfg_mutex;             /** PKTRX用户配置区互斥锁*/
    ZXIC_MUTEX_T    hash_mutex[DPP_PCIE_SLOT_MAX][DEV_HASH_FUNC_ID_NUM];  /**  hash插入 操作互斥锁*/
    ZXIC_MUTEX_T    dtb_rb_mutex[DPP_DTB_QUEUE_MAX];  /**  hash插入 操作互斥锁*/
    ZXIC_MUTEX_T    dtb_queue_mutex[DPP_DTB_QUEUE_MAX];   /* DTB模块队列操作互斥锁 */
    ZXIC_SPIN_LOCK_T    dtb_queue_spin_lock[DPP_PCIE_SLOT_MAX][DPP_DTB_QUEUE_MAX];   /* DTB模块队列操作自旋锁 */
    DPP_PCIE_CHANNEL_T pcie_channel[DPP_PCIE_SLOT_MAX][DPP_PCIE_CHANNEL_MAX];
    ZXIC_VOID       *p_std_nic_res[DPP_PCIE_SLOT_MAX];      /* 标卡流表资源 */ 
    ZXIC_UINT32      bar_msg_num[DPP_PCIE_SLOT_MAX];        /*bar通道消息个数*/
} DPP_DEV_CFG_T;

typedef struct dpp_dev_mngr_t
{
    ZXIC_UINT32         device_num;  /* 设备数目 */
    ZXIC_UINT32         is_init;
    DPP_DEV_CFG_T  *p_dev_array[DPP_DEV_CHANNEL_MAX];
} DPP_DEV_MGR_T;

DPP_STATUS dpp_dev_init(ZXIC_VOID);
DPP_STATUS dpp_dev_add(ZXIC_UINT32  dev_id,
                       DPP_DEV_TYPE_E dev_type,
                       DPP_DEV_ACCESS_TYPE_E  access_type,
                       ZXIC_ADDR_T  pcie_addr,
                       ZXIC_ADDR_T  riscv_addr,
                       ZXIC_ADDR_T  dma_vir_addr,
                       ZXIC_ADDR_T  dma_phy_addr,
                       DPP_DEV_WRITE_FUNC p_pcie_write_fun,
                       DPP_DEV_READ_FUNC  p_pcie_read_fun,
                       DPP_DEV_WRITE_FUNC p_riscv_write_fun,
                       DPP_DEV_READ_FUNC  p_riscv_read_fun);
DPP_STATUS dpp_dev_del(ZXIC_UINT32 dev_id);
DPP_STATUS dpp_dev_get(DPP_PF_INFO_T* pf_info, DPP_DEV_T *dev);
DPP_STATUS dpp_dev_pcie_channel_add(DPP_PF_INFO_T* pf_info, struct pci_dev* p_dev);
DPP_STATUS dpp_dev_pcie_channel_del(DPP_PF_INFO_T* pf_info);
DPP_STATUS dpp_dev_status_update(DPP_DEV_T *dev, ZXIC_UINT32 dev_status);
ZXIC_VOID *dpp_dev_get_se_res_ptr(DPP_DEV_T *dev);
ZXIC_VOID dpp_dev_set_se_res_ptr(DPP_DEV_T *dev, ZXIC_VOID *se_ptr);
DPP_STATUS dpp_dev_opr_mutex_get(DPP_DEV_T *dev, ZXIC_UINT32 type, ZXIC_MUTEX_T **p_mutex_out);
DPP_STATUS dpp_dev_dtb_opr_mutex_get(DPP_DEV_T *dev, ZXIC_UINT32 type, ZXIC_UINT32 index, ZXIC_MUTEX_T **p_mutex_out);
DPP_STATUS dpp_dev_pcie_default_write(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 size, ZXIC_UINT32 *p_data);
DPP_STATUS dpp_dev_pcie_default_read(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 size, ZXIC_UINT32 *p_data);
DPP_STATUS dpp_dev_write_channel(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 size, ZXIC_UINT32 *p_data);
DPP_STATUS dpp_dev_read_channel(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 size, ZXIC_UINT32 *p_data);
DPP_STATUS dpp_dev_hash_opr_mutex_get(DPP_DEV_T *dev, ZXIC_UINT32 fun_id, ZXIC_MUTEX_T **p_mutex_out);
DPP_STATUS dpp_dev_hash_opr_mutex_create(DPP_DEV_T *dev);
DPP_STATUS dpp_dev_hash_opr_mutex_destroy(DPP_DEV_T *dev);
DPP_STATUS dpp_dev_last_check(DPP_DEV_T *dev, ZXIC_UINT32 *last_flag);
DPP_STATUS dpp_soft_hash_index_set(DPP_DEV_T *dev,ZXIC_UINT32 hash_index);
DPP_STATUS dpp_soft_hash_index_get(DPP_DEV_T *dev,ZXIC_UINT32 *hash_index);
DPP_STATUS dpp_dev_dump_dma_mem_get(DPP_DEV_T *dev,ZXIC_UINT32 *p_dma_size, 
                                   ZXIC_UINT64 *p_dma_phy_addr, 
                                   ZXIC_UINT64 *p_dma_vir_addr);DPP_STATUS dpp_dev_dtb_opr_spin_lock_get(DPP_DEV_T *dev, ZXIC_UINT32 queue_id, ZXIC_SPIN_LOCK_T **p_spin_out);

#ifdef __cplusplus
}
#endif

#endif

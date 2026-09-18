#ifndef _DPP_MODULE_H_
#define _DPP_MODULE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "zxic_common.h"
#include "dpp_dev.h"
#include "dpp_type_api.h"

/**  for PPU*/
#ifdef DPP_TEST_BAORD_SSP2T
#define DPP_PPU_CLUSTER_NUM              (1)
#else
#define DPP_PPU_CLUSTER_NUM              (6)
#endif

#define DPP_PHYPORT_NUM                  (119)     /**  物理端口号个数(0-118) */

#define DPP_PPU_ME_EXCEPTION_MAX         (8)
#define DPP_PPU_CLUSTER_MEPERCLS_NUM     (8)
#define DPP_PPU_CLUSTER_SPACE_SIZE       (0x00008000)
#define DPP_MAX_CHCHK_NUM                (50)

#define DPP_TRPG_PORT_NUM                (32)
#define DPP_TRPG_PORT_SPACE_SIZE         (0x10000)

#define DPP_TRPG_RAM_NUM                 (16)
#define DPP_TRPG_RAM_SPACE_SIZE          (0x10000)

#define DPP_TSN_PORT_NUM                (4)
#define DPP_TSN_PORT_SPACE_SIZE         (0x4000)
#define DPP_PRIO_COS_NUM                (8)             /** 最大COS优先级个数 */

typedef enum dpp_module_e
{
    CFG = 1,             /**<  @brief 1*/
    NPPU,                /**<  @brief 2*/
    PPU,                 /**<  @brief 3*/
    ETM,                 /**<  @brief 4*/
    STAT,                /**<  @brief 5*/
    CAR,                 /**<  @brief 6*/
    SE,                  /**<  @brief 7*/
    SMMU0       = SE,    /**<  @brief 7*/
    SMMU1       = SE,    /**<  @brief 7*/
    DTB,                 /**<  @brief 8*/
    TRPG ,               /**<  @brief 9*/
    TSN ,                /**<  @brief 10*/
    AXI ,                /**<  @brief 11*/
    PTPTM ,              /**<  @brief 12*/
    DTB4K,               /**<  @brief 13*/
    STAT4K,              /**<  @brief 14*/
    PPU4K,               /**<  @brief 15*/
    SE4K,                /**<  @brief 16*/
    SMMU14K,             /**<  @brief 17*/
    MODULE_MAX           
} DPP_MODULE_E;

typedef enum module_tm_e
{
    MODULE_TM_CFGMT = 0,
    MODULE_TM_OLIF  = 1,
    MODULE_TM_CGAVD = 2,
    MODULE_TM_TMMU  = 3,
    MODULE_TM_SHAP  = 4,
    MODULE_TM_CRDT  = 5,
    MODULE_TM_QMU   = 6,
    MODULE_TM_MAX
} MODULE_TM_E;

/*NP 在PICE中的偏移地址*/
#define SYS_VF_NP_BASE_OFFSET      0X0000000000
/*NP 在DPU中的基地址*/

#if DPP_DEV_VPCI_EN
#define SYS_NP_BASE_ADDR           0x6300000000
#define SYS_NP_BASE_ADDR0          0x14000000
#define SYS_NP_BASE_ADDR1          0x16000000
#else
#define SYS_NP_BASE_ADDR           0x6218000000
#define SYS_NP_BASE_ADDR0          0x00000000
#define SYS_NP_BASE_ADDR1          0x02000000
#endif

/**  sub system base address*/
typedef enum sys_base_addr_e
{
    SYS_NPPU_BASE_ADDR               = (SYS_NP_BASE_ADDR0 + 0x00000000),
    SYS_PPU_BASE_ADDR                = (SYS_NP_BASE_ADDR0 + 0x00080000),
    SYS_ETM_BASE_ADDR                = (SYS_NP_BASE_ADDR0 + 0x00180000),
    SYS_STAT_BASE_ADDR               = (SYS_NP_BASE_ADDR0 + 0x00200000),
    SYS_SE_BASE_ADDR                 = (SYS_NP_BASE_ADDR0 + 0x00280000),
    SYS_SE_SMMU0_BASE_ADDR           = (SYS_NP_BASE_ADDR0 + 0x00300000),
    SYS_SE_SMMU1_BASE_ADDR           = (SYS_NP_BASE_ADDR0 + 0x00310000),
//    SYS_TRPG_BASE_ADDR               = (SYS_NP_BASE_ADDR0 + 0x00320000),
    SYS_CFG_BASE_ADDR                = (SYS_NP_BASE_ADDR0 + 0x00330000),
    SYS_PTP0_BASE_ADDR               = (SYS_NP_BASE_ADDR0 + 0x00340000),
    SYS_PTP1_BASE_ADDR               = (SYS_NP_BASE_ADDR0 + 0x00344000),
    SYS_TSN_BASE_ADDR                = (SYS_NP_BASE_ADDR0 + 0x00350000),
    SYS_TRPG_BASE_ADDR               = (SYS_NP_BASE_ADDR0 + 0x00400000),

    SYS_DTB_BASE_ADDR                = (SYS_NP_BASE_ADDR1 + 0x00000000),
    SYS_AXIM0_BASE_ADDR              = (SYS_NP_BASE_ADDR1 + 0x01400000),
    SYS_AXIM1_BASE_ADDR              = (SYS_NP_BASE_ADDR1 + 0x01408000),
    SYS_AXI_CONV_BASE_ADDR           = (SYS_NP_BASE_ADDR1 + 0x01410000),
    SYS_AXIS_BASE_ADDR               = (SYS_NP_BASE_ADDR1 + 0x01418000),
    SYS_TLB_BASE_ADDR                = (SYS_NP_BASE_ADDR1 + 0x01420000),
    
    SYS_MAX_BASE_ADDR                = 0x20000000,
} SYS_BASE_ADDR_E;

/**  module base address*/
typedef enum module_base_addr_e
{
    /* CFG         */
    MODULE_CFG_PCIE_BASE_ADDR        = 0x00000000,
    MODULE_CFG_DMA_BASE_ADDR         = 0x00001000,
    MODULE_CFG_CSR_BASE_ADDR         = 0x00003000,

    /* NPPU       */
    MODULE_NPPU_MR_CFG_BASE_ADDR           = 0x00000000,
    MODULE_NPPU_PKTRX_CFG_BASE_ADDR        = 0x00000800,
    MODULE_NPPU_PKTRX_STAT_BASE_ADDR       = 0x00001000,
    MODULE_NPPU_IDMA_CFG_BASE_ADDR         = 0x00001800,
    MODULE_NPPU_IDMA_STAT_BASE_ADDR        = 0x00002000,
    MODULE_NPPU_PBU_CFG_BASE_ADDR          = 0x00002800,
    MODULE_NPPU_PBU_STAT_BASE_ADDR         = 0x00003000,
    MODULE_NPPU_ISU_CFG_BASE_ADDR          = 0x00003800,
    MODULE_NPPU_ISU_STAT_BASE_ADDR         = 0x00004000,
    MODULE_NPPU_ODMA_CFG_BASE_ADDR         = 0x00004800,
    MODULE_NPPU_ODMA_STAT_BASE_ADDR        = 0x00005000,
    MODULE_NPPU_OAM_CFG_BASE_ADDR          = 0x00005800,
    MODULE_NPPU_OAM_STAT_BASE_ADDR         = 0x00006000,
    MODULE_NPPU_OAM_INT_IDX0_BASE_ADDR     = 0x00006800,
    MODULE_NPPU_OAM_INT_IDX1_BASE_ADDR     = 0x00007000,

    /* PPU         */
    MODULE_PPU_CSR_BASE_ADDR         = 0x00000000,
    MODULE_PPU_DBG_BASE_ADDR         = 0x00000800,
    MODULE_CLUSTER0_BASE_ADDR        = 0x00008000,
    MODULE_CLUSTER1_BASE_ADDR        = 0x00010000,
    MODULE_CLUSTER2_BASE_ADDR        = 0x00018000,
    MODULE_CLUSTER3_BASE_ADDR        = 0x00020000,

    /* TM          */
    MODULE_TM_CFGMT_BASE_ADDR        = 0x00000000,
    MODULE_TM_OLIF_BASE_ADDR         = 0x00020000,
    MODULE_TM_CGAVD_BASE_ADDR        = 0x00030000,
    MODULE_TM_TMMU_BASE_ADDR         = 0x00040000,
    MODULE_TM_SHAP_BASE_ADDR         = 0x00050000,
    MODULE_TM_CRDT_BASE_ADDR         = 0x00060000,
    MODULE_TM_QMU_BASE_ADDR          = 0x00070000,

    /* STAT        */
    MODULE_STAT_CAR0_BASE_ADDR       = 0x00000000,
    MODULE_STAT_ETCAM_BASE_ADDR      = 0x00002000,
    MODULE_STAT_GLBL_BASE_ADDR       = 0x00003000,

    /* SE          */
    MODULE_SE_ALG_BASE_ADDR          = 0x00000000,         
    MODULE_SE_KSCHD_BASE_ADDR        = 0x00004000,        
    MODULE_SE_RSCHD_BASE_ADDR        = 0x00008000,         
    MODULE_SE_PARSER_BASE_ADDR       = 0x0000c000,        
    MODULE_SE_AS_BASE_ADDR           = 0x00010000,        
    MODULE_SE_CFG_BASE_ADDR          = 0x00014000, 
    
    /* SMMU0          */
    MODULE_SE_SMMU0_BASE_ADDR        = 0x00000000,         

    /* SMMU1          */
    MODULE_SE_SMMU1_BASE_ADDR        = 0x00000000,         
    MODULE_SE_CMMU_BASE_ADDR         = 0x00004000,    

    /* DTB          */
    MODULE_DTB_ENQ_BASE_ADDR        = 0x00000000,         
    MODULE_DTB_CFG_BASE_ADDR        = 0x01000000, 
    MODULE_DTB_DDOS_BASE_ADDR       = 0x01010000,
    MODULE_DTB_RAM_BASE_ADDR        = 0x01100000,

    /* TRPG          */
    MODULE_TRPG_RX_BASE_ADDR                = 0x00000000,         
    MODULE_TRPG_TX_BASE_ADDR                = 0x00400000, 
    MODULE_TRPG_TX_GLB_BASE_ADDR            = 0x00600000,
    MODULE_TRPG_TX_ETM_PORT_BASE_ADDR       = 0x00610000,
    MODULE_TRPG_RX_RAM_BASE_ADDR            = 0x00200000,
    MODULE_TRPG_TX_RAM_BASE_ADDR            = 0x00620000,
    MODULE_TRPG_TX_ETM_RAM_BASE_ADDR        = 0x00710000,
    MODULE_TRPG_TX_TODTIME_RAM_BASE_ADDR    = 0x00720000,
    


    /* TSN          */
    MODULE_TSN_PORT0_BASE_ADDR      = 0x00000000,
    MODULE_TSN_PORT1_BASE_ADDR      = 0x00004000,
    MODULE_TSN_PORT2_BASE_ADDR      = 0x00008000,
    MODULE_TSN_PORT3_BASE_ADDR      = 0x0000C000,
      
} MODULE_BASE_ADDR_E;

DPP_STATUS dpp_read(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 *p_data);
DPP_STATUS dpp_write(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 *p_data);

DPP_STATUS dpp_se_read(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 *p_data);
DPP_STATUS dpp_se_write(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 *p_data);

DPP_STATUS dpp_se_alg_read(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 *p_data);
DPP_STATUS dpp_se_alg_write(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 *p_data);

DPP_STATUS dpp_ppu_read(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 *p_data);
DPP_STATUS dpp_ppu_write(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 *p_data);

#ifdef __cplusplus
}
#endif

#endif

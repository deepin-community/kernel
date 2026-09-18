/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : zxic_slt.h
* 文件标识 : 
* 内容摘要 : 
* 其它说明 : 
* 当前版本 : 
* 作    者 : PJ
* 完成日期 : 2022/03/26
* DEPARTMENT: ASIC_FPGA_R&D_Dept 
* MANUAL_PERCENT: 100%   
 
* 修改记录1: 
* 修改日期:  
* 版 本 号:  
* 修 改 人:  
* 修改内容:  
***************************************************************/

#ifndef __ZXIC_SLT_H__
#define __ZXIC_SLT_H__

#ifdef __cplusplus
extern "C" {
#endif


#define ZXIC_SLT_CASE_PASS                        (0U)
#define ZXIC_SLT_CASE_FAILURE                     (1U)

#define ZXIC_SLT_ERR_INFO_LEN                     (255U)
#define ZXIC_SLT_CASE_MAX_NUM                     (255U)
#define ZXIC_SLT_DEV_ID_MAX_NUM                   (4U)
#define ZXIC_SLT_CHIP_ID_LEN                      (3U)

/* 封装类型，不同产品含义不同，暂时定义最大支持5个*/
#define ZXIC_SLT_MAX_ENCAP                        (5U)


/* BIN_CODE 编码*/
#define ZXIC_SLT_BIN_CODE_PASS                    ZXIC_SLT_CASE_PASS
#define ZXIC_SLT_BIN_CODE_SERDES_FAIL             (1U)/* sredes类检测*/
#define ZXIC_SLT_BIN_CODE_FLOW_FAIL               (2U)/* pvt类检测 FLOW通流类*/
#define ZXIC_SLT_BIN_CODE_RAM_FAIL                (3U)/* RAM类检测*/
#define ZXIC_SLT_BIN_CODE_HBM_DDR_FAIL            (4U)/* hbm类检测*/
#define ZXIC_SLT_BIN_CODE_OTHER_FAIL              (5U)/* PLL EFUSE PCIE RISCV */
#define ZXIC_SLT_BIN_CODE_MIX_FAIL                (6U)/* 上述多种类型的混合*/




/* 测试用例全集case_no定义*/
typedef enum zxic_slt_case_no_e
{
    ZXIC_SLT_CASE_PLL_LOCK_STATUS_CHECK       = (0x08U),
    /* efuse锁定相关测试用例 */
    ZXIC_SLT_CASE_EFUSE_CHECK                 = (0x09U),




    /* serdes用例编号范围0x10~0x7f*/
    /* lifx(LIF0 LIF1 LIF2)接口serdes prbs相关测试用例  0x10~0x2f*/
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_1G         = (0x10U),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_3G         = (0x11U),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_5G         = (0x12U),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_6G         = (0x13U),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_9G         = (0x14U),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_10G        = (0x15U),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_12G        = (0x16U),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_15G        = (0x17U),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_20G        = (0x18U),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_25G        = (0x19U),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_26G        = (0x1aU),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_27G        = (0x1bU),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_28G        = (0x1cU),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_30G        = (0x1dU),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_50G        = (0x1fU),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_51G        = (0x20U),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_53G        = (0x21U),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_55G        = (0x22U),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_56G        = (0x23U),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_57G        = (0x24U),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_98G        = (0x25U),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_106G       = (0x26U),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_111G       = (0x27U),
    ZXIC_SLT_CASE_SERDES_LIFX_PRBS_112G       = (0x28U),


    /* lifc接口serdes相关测试用例 0x30~0x37*/
    ZXIC_SLT_CASE_SERDES_LIFC_PRBS_1G         = (0x30U),
    ZXIC_SLT_CASE_SERDES_LIFC_PRBS_10G        = (0x31U),
    ZXIC_SLT_CASE_SERDES_LIFC_PRBS_25G        = (0x32U),
    ZXIC_SLT_CASE_SERDES_LIFC_PRBS_53G        = (0x33U),

    /* lifc eth 接口发包测试相关测试用例 0x38~0x3f*/
    ZXIC_SLT_CASE_PORT_LIFC_SD_1G            = (0x38U),
    ZXIC_SLT_CASE_PORT_LIFC_SD_10G           = (0x39U),
    ZXIC_SLT_CASE_PORT_LIFC_SD_25G           = (0x3aU),
    ZXIC_SLT_CASE_PORT_LIFC_SD_53G           = (0x3bU),

    /* 外部查找的interlaken-la接口*/
    ZXIC_SLT_CASE_SERDES_INTERLAKEN_LA       = (0x3fU),


    /* lif0 eth 接口发包测试相关测试用例 0x40~0x5f*/
    ZXIC_SLT_CASE_PORT_LIFO_SD_1G         = (0x40U),
    ZXIC_SLT_CASE_PORT_LIFO_SD_5G         = (0x41U),
    ZXIC_SLT_CASE_PORT_LIFO_SD_10G        = (0x42U),
    ZXIC_SLT_CASE_PORT_LIFO_SD_12G        = (0x43U),
    ZXIC_SLT_CASE_PORT_LIFO_SD_25G        = (0x44U),
    ZXIC_SLT_CASE_PORT_LIFO_SD_26G        = (0x45U),
    ZXIC_SLT_CASE_PORT_LIFO_SD_51G        = (0x46U),
    ZXIC_SLT_CASE_PORT_LIFO_SD_53G        = (0x47U),
    ZXIC_SLT_CASE_PORT_LIFO_SD_106G       = (0x48U),
    ZXIC_SLT_CASE_PORT_LIFO_SD_112G       = (0x49U),
    ZXIC_SLT_CASE_PORT_LIFO_SD_20G        = (0x4AU),


    /* lif0 intlk接口发包测试相关测试用例 0x60~0x6f*/
    ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_3G     =  (0x60U),
    ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_6G     =  (0x61U),
    ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_10G    =  (0x62U),
    ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_12G    =  (0x63U),
    ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_25G    =  (0x64U),
    ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_28G    =  (0x65U),
    ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_30G    =  (0x66U),
    ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_50G    =  (0x67U),
    ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_51G    =  (0x68U),
    ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_53G    =  (0x69U),
    ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_56G    =  (0x6aU),
    ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_106G   =  (0x6bU),
    ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_112G   =  (0x6cU),


    /* 交换侧lif1 FEC测试相关测试用例 0x70~0x7f*/
    ZXIC_SLT_CASE_SERDES_LIF1_FEC_25G        = (0x70U),
    ZXIC_SLT_CASE_SERDES_LIF1_FEC_26G        = (0x71U),
    ZXIC_SLT_CASE_SERDES_LIF1_FEC_28G        = (0x72U),
    ZXIC_SLT_CASE_SERDES_LIF1_FEC_50G        = (0x73U),
    ZXIC_SLT_CASE_SERDES_LIF1_FEC_53G        = (0x74U),
    ZXIC_SLT_CASE_SERDES_LIF1_FEC_56G        = (0x75U),
    ZXIC_SLT_CASE_SERDES_LIF1_FEC_98G        = (0x76U),
    ZXIC_SLT_CASE_SERDES_LIF1_FEC_106G       = (0x77U),
    ZXIC_SLT_CASE_SERDES_LIF1_FEC_112G       = (0x78U),




    /*片内ram用例编号范围0x80~0x87*/
    /* CPU可读写寄存器测试用例 */
    ZXIC_SLT_CASE_REG_CPU_W_R                      =  (0x80U),
    /* CPU可读写ram测试用例 */
    ZXIC_SLT_CASE_RAM_CPU_W_R                      =  (0x81U),
    /* RAM表项读写测试(微码)*/
    ZXIC_SLT_CASE_RAM_MC_W_R                       =  (0x82U),
   /* RAM表项读写测试 memtest86(微码)*/
    ZXIC_SLT_CASE_RAM_MC_MEMTEST86                 =  (0x83U),




    /* HBM/DDR用例编号范围 0x90~0x9f*/
    /*mbist测试*/
    ZXIC_SLT_CASE_HBM_DDR_CTRL_BIST                =  (0x90U),
    /* HBM/DDR表项CPU读写测试*/
    ZXIC_SLT_CASE_HBM_DDR_CPU_W_R                  =  (0x91U),
    /* HBM/DDR表项读写测试(微码)*/
    ZXIC_SLT_CASE_HBM_DDR_MC_W_R                   =  (0x92U),
    /* HBM/DDR表项读写测试 memtest86(微码)*/
    ZXIC_SLT_CASE_HBM_DDR_MC_MEMTEST86             =  (0x93U),
    /* HBM/DDR TM覆盖测试*/
    ZXIC_SLT_CASE_HBM_DDR_TM_TEST                  =  (0x94U),





    /* 发流测试和全业务PVT测试用例编号范围 0xa0~0xaf*/
    /* 数据路径测试用例 */
    ZXIC_SLT_CASE_PKT_FLOW_NOM                      =  (0xA0U),
    /* OAM发包测试用例 */
    ZXIC_SLT_CASE_OAM_SEND_PKT                      =  (0xA1U),
    /* se查表全集测试 */
    ZXIC_SLT_CASE_PKT_FLOW_SE_FULL                  =  (0xA2U),
    /* tm全队列测试用例 */
    ZXIC_SLT_CASE_PKT_FLOW_TM_FULL                  =  (0xA3U),
    /* LIFC口测试用例 */
    ZXIC_SLT_CASE_PKT_FLOW_LIFC                     =  (0xA4U),

    /* 芯片PVT 功耗、温度和电压测试用例 */
    ZXIC_SLT_CASE_PVT_EXCEED_POWER_TEST             =  (0xA8U),
    ZXIC_SLT_CASE_PVT_EXCEED_TEM_VOLT_TEST          =  (0xA9U),
    ZXIC_SLT_CASE_PVT_EXCEED_HBM_TEMPER_TEST        =  (0xAaU),





    /* 其他测试用例编号范围0xb0~0xff*/
    /* 调压测试*/
    ZXIC_SLT_CASE_VOLT_TEST                         =  (0xB0U),
    /* 时钟频偏测试 */
    ZXIC_SLT_CASE_CLK_TEST                          =  (0xB1U),

    /* PCIE压力测试 */
    ZXIC_SLT_CASE_PCIE_W_R_TEST                     =  (0xB2U),
    /* 下述两个测试用例必须放在最后执行 */
    /* PCIE建链测试 */
    ZXIC_SLT_CASE_PCIE_TEST                         =  (0xB3U),
    /* RISCV测试用例 */
    ZXIC_SLT_CASE_RISCV_W_R_TEST                     =  (0xB4U),

    ZXIC_SLT_CASE_MAX,
} ZXIC_SLT_CASE_NO_E;


#if ZXIC_REAL("ERR_CODE")
#define ZXIC_SLT_RC_BASE                              (0x100U)


 #define ZXIC_SLT_TEST_CASE_FNC_POINT_NULL            (ZXIC_SLT_RC_BASE | 0x1)  
 #define ZXIC_SLT_CASE_PLL_LOCK_STATUS_CHECK_FAIL     (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PLL_LOCK_STATUS_CHECK)  
 #define ZXIC_SLT_CASE_EFUSE_CHECK_FAIL               (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_EFUSE_CHECK)  
 
 /* lifx*/                                            
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_1G_FAIL        (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_1G)  
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_3G_FAIL        (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_3G)  
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_5G_FAIL        (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_5G)     
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_6G_FAIL        (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_6G)     
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_9G_FAIL        (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_9G)     
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_10G_FAIL       (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_10G)     
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_12G_FAIL       (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_12G)     
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_15G_FAIL       (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_15G)     
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_20G_FAIL       (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_20G)     
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_25G_FAIL       (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_25G)     
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_26G_FAIL       (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_26G)     
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_27G_FAIL       (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_27G)     
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_28G_FAIL       (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_28G)     
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_30G_FAIL       (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_30G)     
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_50G_FAIL       (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_50G)     
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_51G_FAIL       (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_51G)     
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_53G_FAIL       (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_53G)     
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_55G_FAIL       (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_55G)     
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_56G_FAIL       (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_56G)  
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_57G_FAIL       (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_57G)     
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_98G_FAIL       (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_98G)     
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_106G_FAIL      (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_106G)     
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_111G_FAIL      (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_111G)     
 #define ZXIC_SLT_CASE_SERDES_LIFX_PRBS_112G_FAIL      (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFX_PRBS_112G)     
     
 /* lifc*/                                            
 #define ZXIC_SLT_CASE_SERDES_LIFC_PRBS_1G_FAIL        (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFC_PRBS_1G)   
 #define ZXIC_SLT_CASE_SERDES_LIFC_PRBS_10G_FAIL       (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFC_PRBS_10G)     
 #define ZXIC_SLT_CASE_SERDES_LIFC_PRBS_25G_FAIL       (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFC_PRBS_25G)     
 #define ZXIC_SLT_CASE_SERDES_LIFC_PRBS_53G_FAIL       (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIFC_PRBS_53G)     
   
 /* lifc */                                           
 #define ZXIC_SLT_CASE_PORT_LIFC_SD_1G_FAIL            (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFC_SD_1G )  
 #define ZXIC_SLT_CASE_PORT_LIFC_SD_10G_FAIL           (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFC_SD_10G)  
 #define ZXIC_SLT_CASE_PORT_LIFC_SD_25G_FAIL           (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFC_SD_25G)   
 #define ZXIC_SLT_CASE_PORT_LIFC_SD_53G_FAIL           (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFC_SD_53G)  
    
 /* lif0 ETH*/                                        
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_1G_FAIL           (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_1G)  
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_5G_FAIL           (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_5G)  
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_10G_FAIL          (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_10G)  
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_12G_FAIL          (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_12G)  
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_25G_FAIL          (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_25G)  
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_26G_FAIL          (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_26G)  
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_51G_FAIL          (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_51G)   
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_53G_FAIL          (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_53G)   
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_106G_FAIL         (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_106G)   
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_112G_FAIL         (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_112G) 
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_20G_FAIL          (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_20G)   
    
 /* lif0 intlk*/                                      
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_3G_FAIL       (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_3G  )  
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_6G_FAIL       (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_6G  )  
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_10G_FAIL      (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_10G )    
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_12G_FAIL      (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_12G )  
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_25G_FAIL      (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_25G )  
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_28G_FAIL      (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_28G )  
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_30G_FAIL      (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_30G )  
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_50G_FAIL      (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_50G )  
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_51G_FAIL      (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_51G )  
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_53G_FAIL      (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_53G )  
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_56G_FAIL      (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_56G )  
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_106G_FAIL     (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_106G)  
 #define ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_112G_FAIL     (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PORT_LIFO_SD_ILK_112G )  


/* lif1*/                                              
#define ZXIC_SLT_CASE_SERDES_LIF1_FEC_25G_FAIL   (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIF1_FEC_25G)  
#define ZXIC_SLT_CASE_SERDES_LIF1_FEC_26G_FAIL   (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIF1_FEC_26G)    
#define ZXIC_SLT_CASE_SERDES_LIF1_FEC_28G_FAIL   (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIF1_FEC_28G)   
#define ZXIC_SLT_CASE_SERDES_LIF1_FEC_50G_FAIL   (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIF1_FEC_50G) 
#define ZXIC_SLT_CASE_SERDES_LIF1_FEC_53G_FAIL   (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIF1_FEC_53G)   
#define ZXIC_SLT_CASE_SERDES_LIF1_FEC_56G_FAIL   (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIF1_FEC_56G)
#define ZXIC_SLT_CASE_SERDES_LIF1_FEC_98G_FAIL   (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIF1_FEC_98G)  
#define ZXIC_SLT_CASE_SERDES_LIF1_FEC_106G_FAIL  (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIF1_FEC_106G)   
#define ZXIC_SLT_CASE_SERDES_LIF1_FEC_112G_FAIL  (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_LIF1_FEC_112G) 

/* 外部查找的interlaken-la接口*/
#define ZXIC_SLT_CASE_SERDES_INTELAKEN_LA_FAIL   (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_SERDES_INTELAKEN_LA) 

 
 /*片内ram读写测试*/                                   
 #define ZXIC_SLT_CASE_RAM_CPU_W_R_FAIL               (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_RAM_CPU_W_R )  
 #define ZXIC_SLT_CASE_REG_CPU_W_R_FAIL               (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_REG_CPU_W_R  )  
 #define ZXIC_SLT_CASE_RAM_MC_W_R_FAIL                (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_RAM_MC_W_R  )  
 #define ZXIC_SLT_CASE_RAM_MC_MEMTEST86_FAIL          (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_RAM_MC_MEMTEST86 )  

 /* HBM/DDR */
 #define ZXIC_SLT_CASE_HBM_DDR_CTRL_BIST_FAIL         (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_HBM_DDR_CTRL_BIST    )  
 #define ZXIC_SLT_CASE_HBM_DDR_MC_W_R_FAIL            (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_HBM_DDR_MC_W_R       )  
 #define ZXIC_SLT_CASE_HBM_DDR_MC_MEMTEST86_FAIL      (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_HBM_DDR_MC_MEMTEST86 )  
 #define ZXIC_SLT_CASE_HBM_DDR_CPU_W_R_FAIL           (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_HBM_DDR_CPU_W_R )  
 #define ZXIC_SLT_CASE_HBM_DDR_TM_TEST_FAIL           (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_HBM_DDR_TM_TEST      )  


 /* 发流测试和全业务PVT测试*/
 #define ZXIC_SLT_CASE_PKT_FLOW_NOM_FAIL               (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PKT_FLOW_NOM              )  
 #define ZXIC_SLT_CASE_OAM_SEND_PKT_FAIL               (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_OAM_SEND_PKT              )  
 #define ZXIC_SLT_CASE_PKT_FLOW_SE_FULL_FAIL           (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PKT_FLOW_SE_FULL          )  
 #define ZXIC_SLT_CASE_PKT_FLOW_TM_FULL_FAIL           (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PKT_FLOW_TM_FULL          )   
 #define ZXIC_SLT_CASE_PKT_FLOW_LIFC_FAIL              (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PKT_FLOW_LIFC  )

 #define ZXIC_SLT_CASE_PVT_EXCEED_POWER_TEST_FAIL      (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PVT_EXCEED_POWER_TEST     )  
 #define ZXIC_SLT_CASE_PVT_EXCEED_TEM_VOLT_TEST_FAIL   (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PVT_EXCEED_TEM_VOLT_TEST  )  
 #define ZXIC_SLT_CASE_PVT_EXCEED_HBM_TEMPER_TEST_FAIL (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PVT_EXCEED_HBM_TEMPER_TEST)  


 #define ZXIC_SLT_CASE_PCIE_TEST_FAIL                  (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PCIE_TEST    )  
 #define ZXIC_SLT_CASE_PCIE_W_R_TEST_FAIL              (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_PCIE_W_R_TEST    )  
 #define ZXIC_SLT_CASE_RISCV_W_R_TEST_FAIL              (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_RISCV_W_R_TEST)  
 #define ZXIC_SLT_CASE_VOLT_TEST_FAIL                  (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_VOLT_TEST    )  
 #define ZXIC_SLT_CASE_CLK_TEST_FAIL                   (ZXIC_SLT_RC_BASE | ZXIC_SLT_CASE_CLK_TEST)  

#endif 


typedef struct zxic_slt_err_info_mng_t
{
    ZXIC_UINT32         err_code;      /*参见ERR_CODE定义*/
    const ZXIC_CHAR *   err_info;
} ZXIC_SLT_ERR_INFO_MNG_T;


typedef struct zxic_slt_init_ctrl_t
{
    ZXIC_ADDR_T pcie_vir_baddr;               /**<  @brief PCIe映射虚拟基地址*/
    ZXIC_ADDR_T riscv_vir_baddr;               /**<  @brief RISCV映射虚拟基地址 */
    ZXIC_ADDR_T dma_vir_baddr;                /**<  @brief DMA映射虚拟地址*/
    ZXIC_ADDR_T dma_phy_baddr;                /**<  @brief DMA内存物理地址*/
}ZXIC_SLT_INIT_CTRL_T;


typedef struct zxic_slt_case_para_t

{
    ZXIC_UINT32 chip_id[ZXIC_SLT_CHIP_ID_LEN];      /* 返回对应芯片ID */
    ZXIC_UINT32 case_num;                           /* 返回执行SLT 用例数 */
    ZXIC_UINT32 case_no[ZXIC_SLT_CASE_MAX_NUM];     /* 返回执行SLT 用例编号列表 */
    ZXIC_UINT32 case_result[ZXIC_SLT_CASE_MAX_NUM]; /* 返回执行SLT 用例对应结果，成功返回值或错误码 */
    ZXIC_UINT32 bin_code;                           /* 返回 SLT 执行分BIN码 */	
    ZXIC_DOUBLE temp;                               /* 返回 SLT 用例测试前初始温度 */
    ZXIC_DOUBLE volt;                               /* 返回 SLT 用例测试前核电压 */

} ZXIC_SLT_CASE_PARA_T;



/*case 函数标准定义*/
typedef ZXIC_RTN32 (*ZXIC_SLT_INST_CASE_FN)(ZXIC_UINT32 device_id);

typedef struct zxic_slt_case_mng_t
{
    const ZXIC_CHAR *        case_name;     /*用例名称*/
    ZXIC_UINT32              case_no;       /*参见ZXIC_SLT_CASE_NO_E定义*/
    ZXIC_SLT_INST_CASE_FN    inst_case_fn; /*单个case函数指针*/
} ZXIC_SLT_CASE_MNG_T;



/* 功耗类型定义*/
typedef enum zxic_slt_power_type_t
{
    ZXIC_SLT_POWER_VCC_SSP4_CORE                =  (0x0U),
    ZXIC_SLT_POWER_VCC0V75_SSP4_SERDES_DVDD     =  (0x1U),
    ZXIC_SLT_POWER_VCC0V75_SSP4_SERDES_AVDDL    =  (0x2U),
    ZXIC_SLT_POWER_VCC1V2_SSP4_SERDES_AVDDH     =  (0x3U),
    ZXIC_SLT_POWER_VCC1V2_SSP4_HBM_VDDQ         =  (0x4U),

    ZXIC_SLT_POWER_VCC0V75_SSP4_AVDD            =  (0x5U),
    ZXIC_SLT_POWER_VCC1V2_SSP4_AVDD             =  (0x6U),
    ZXIC_SLT_POWER_VCC1V2_SSP4_GPIO_DVDD        =  (0x7U),
    ZXIC_SLT_POWER_VCC2V5_SSP4_VPP              =  (0x8U),

    ZXIC_SLT_POWER_TYPE_MAX,

}ZXIC_SLT_POWER_TYPE_T;

typedef struct zxic_slt_power_t
{

   ZXIC_DOUBLE volt;   /* 电压值 */
   ZXIC_DOUBLE cur;    /* 电流值 */
   ZXIC_UINT32 type;   /* 功耗编码，参见ZXIC_SLT_POWER_TYPE_T定义 */
}ZXIC_SLT_POWER_T;

typedef struct zxic_slt_power_para_t
{
   ZXIC_SLT_POWER_T *p_para;
   ZXIC_UINT32 num;  /* 有效功耗个数 */
}ZXIC_SLT_POWER_PARA_T;



/* 电压类型定义*/
typedef enum zxic_slt_volt_type_t
{
    ZXIC_SLT_VOLT_VCC_SSP4_CORE                =  (0x0U),
    ZXIC_SLT_VOLT_VCC0V75_SSP4_SERDES_DVDD     =  (0x1U),
    ZXIC_SLT_VOLT_VCC0V75_SSP4_SERDES_AVDDL    =  (0x2U),
    ZXIC_SLT_VOLT_VCC1V2_SSP4_SERDES_AVDDH     =  (0x3U),
    ZXIC_SLT_VOLT_VCC1V2_SSP4_HBM_VDDQ         =  (0x4U),
    
    ZXIC_SLT_VOLT_TYPE_MAX,

}ZXIC_SLT_VOLT_TYPE_T;

typedef struct zxic_slt_volt_t
{

   ZXIC_UINT32 percent;   /* 调压上下浮动值，百分比*100,例如上调1%，传入数值1*/
   ZXIC_UINT32 flag;      /* 正偏：0   负偏：1 */
   ZXIC_UINT32 vol_type;  /* 电压类型，参见ZXIC_SLT_VOLT_TYPE_T定义 */
}ZXIC_SLT_VOLT_T;

typedef struct zxic_slt_volt_para_t
{
   ZXIC_SLT_VOLT_T *p_para;
   ZXIC_UINT32 num;  /* 有效调压个数 */
}ZXIC_SLT_VOLT_PARA_T;



/* 时钟类型定义*/
typedef enum zxic_slt_clk_type_t
{
    ZXIC_SLT_CLK_AU5327_A          =  (0x0U),/*SSP4 SOCKET 单板AU5327#1 HOST_100M*/
    ZXIC_SLT_CLK_AU5327_B          =  (0x1U),/*SSP4 SOCKET 单板AU5327#1 PLL_SYS_CLK/SERDES 156.25M*/
    ZXIC_SLT_CLK_AU5327_C          =  (0x2U),/*SSP4 SOCKET 单板AU5327#1 PLL_LOCAL_CLK  156.25M*/
    ZXIC_SLT_CLK_AU5327_D          =  (0x3U),/*SSP4 SOCKET 单板AU5327#1 PLL_TS_CLK 125M*/
    
    ZXIC_SLT_CLK_TYPE_MAX,


}ZXIC_SLT_CLK_TYPE_T;

typedef struct zxic_slt_clk_t
{

   ZXIC_UINT32 value;     /* 时钟拉偏值，单位HZ(PPM),1PPM = 1HZ（1MHZ=1000KHZ=1000000HZ）*/
   ZXIC_UINT32 flag;      /* 正偏：0   负偏：1 */
   ZXIC_UINT32 clk_type;  /* 时钟类型，参见ZXIC_SLT_CLK_TYPE_T定义 */
}ZXIC_SLT_CLK_T;

typedef struct zxic_slt_clk_para_t
{
   ZXIC_SLT_CLK_T *p_para;
   ZXIC_UINT32 num;  /* 有效拉偏时钟个数 */
}ZXIC_SLT_CLK_PARA_T;



#if ZXIC_REAL("define_for_bsp")
/***********************************************************/
/** 功耗获取函数
* @param   ZXIC_SLT_CASE_GETPOWER_FN   
* @param   device_id   
* @param   p_para   
*
* @return  
* @remark  无
* @see     
* @author  PJ      @date  2022/04/28
************************************************************/
typedef ZXIC_RTN32 (*ZXIC_SLT_CASE_GETPOWER_FN)(ZXIC_UINT32 device_id, ZXIC_SLT_POWER_PARA_T *p_para);


/***********************************************************/
/** riscv测试函数
* @param   ZXIC_SLT_CASE_RISCV_FN   
* @param   device_id   
* @param   reg_addr   寄存器地址
* @param   wr_data   写入寄存器的值
* @param   rd_data   期望从寄存器中读出的值
*
* @return  
* @remark  无
* @see     
* @author  PJ      @date  2022/04/28
************************************************************/
typedef ZXIC_RTN32 (*ZXIC_SLT_CASE_RISCV_FN)(ZXIC_UINT32 device_id, ZXIC_UINT32 reg_addr, ZXIC_UINT32 wr_data, ZXIC_UINT32 rd_data);


/***********************************************************/
/** PCIE测试函数
* @param   ZXIC_SLT_CASE_PCIE_FN   
* @param   device_id   
* @param   times  pcie建链测试次数  
*
* @return  
* @remark  无
* @see     
* @author  PJ      @date  2022/04/28
************************************************************/
typedef ZXIC_RTN32 (*ZXIC_SLT_CASE_PCIE_FN)(ZXIC_UINT32 device_id, ZXIC_UINT32 times);


/***********************************************************/
/** 调电压函数
* @param   ZXIC_SLT_CASE_ADJUST_VOLT_FN   
* @param   device_id   
* @param   p_para   
*
* @return  
* @remark  无
* @see     
* @author  PJ      @date  2022/04/28
************************************************************/
typedef ZXIC_RTN32 (*ZXIC_SLT_CASE_ADJUST_VOLT_FN)(ZXIC_UINT32 device_id,ZXIC_SLT_VOLT_PARA_T *p_para);


/***********************************************************/
/** 调时钟函数
* @param   ZXIC_SLT_CASE_ADJUST_CLK_FN   
* @param   device_id   
* @param   p_para   
*
* @return  
* @remark  无
* @see     
* @author  PJ      @date  2022/04/28
************************************************************/
typedef ZXIC_RTN32 (*ZXIC_SLT_CASE_ADJUST_CLK_FN)(ZXIC_UINT32 device_id,ZXIC_SLT_CLK_PARA_T *p_para);


/*BSP回调函数定义*/
typedef struct zxic_slt_bsp_register_fn_t
{
    ZXIC_SLT_CASE_GETPOWER_FN        zxic_slt_power_fn;        
    ZXIC_SLT_CASE_RISCV_FN            zxic_slt_riscv_fn;          
    ZXIC_SLT_CASE_PCIE_FN            zxic_slt_pcie_fn;         
    ZXIC_SLT_CASE_ADJUST_VOLT_FN     zxic_slt_volt_fn;         
    ZXIC_SLT_CASE_ADJUST_CLK_FN      zxic_slt_clk_fn;          

} ZXIC_SLT_BSP_REGISTER_FN_T;

#endif

#if ZXIC_REAL("define_for_sdk")
/***********************************************************/
/** 初始化函数
* @param   ZXIC_SLT_INIT_FN   
* @param   device_id   
* @param   chip_type  用于区分不同封装类型  
* @param   p_init_ctrl   参见ZXIC_SLT_INIT_CTRL_T定义
*
* @return  
* @remark  无
* @see     
* @author  PJ      @date  2022/04/22
************************************************************/
typedef ZXIC_RTN32 (*ZXIC_SLT_INIT_FN)(ZXIC_UINT32 device_id,ZXIC_UINT32 chip_type,ZXIC_SLT_INIT_CTRL_T *p_init_ctrl);




/* Chip_ID 组成结构说明：
** Chip_ID = efuse里面的wafer批次号+wafer片号+x坐标+Y坐标组合而成
** wafer批次号: 48bit lot_id5~0
** wafer片号:   5bit  wafer_no
** x坐标:		8bit  x_addr
** Y坐标:		8bit  y_addr
** chip_id2 = lot_id5[7:3]
** chip_id1 = lot_id5[2:0] + lot_id4 + lot_id3 + lot_id2 + lot_id1[7:3]
** chip_id0 = lot_id1[2:0] + lot_id0 + wafer_no[4:0] + x_addr + y_addr*/

/***********************************************************/
/** 执行测试用例之前的预处理函数,用来获取chip_id,温度，电压信息
* @param   ZXIC_SLT_PRE_FN   
* @param   device_id   
* @param   chip_type  用于区分不同封装类型 
* @param   p_chip_id   返回芯片的chip_id信息(参见上述格式说明）
* @param   p_temp  返回 SLT 用例测试前初始温度 
* @param   p_volt  返回 SLT 用例测试前核电压 
*
* @return  
* @remark  无
* @see     
* @author  PJ      @date  2022/03/29
************************************************************/
typedef ZXIC_RTN32 (*ZXIC_SLT_PRE_FN)(ZXIC_UINT32 device_id,ZXIC_UINT32 chip_type,ZXIC_UINT32 *p_chip_id, ZXIC_DOUBLE *p_temp,ZXIC_DOUBLE *p_volt);

/***********************************************************/
/** 用例执行函数
* @param   ZXIC_SLT_ALL_CASE_FN   
* @param   device_id   
* @param   chip_type  用于区分不同封装类型 
* @param   p_case_num  返回执行SLT 用例数 
* @param   p_case_no   返回执行SLT 用例编号列表
* @param   p_case_result   返回执行SLT 用例对应结果，成功返回值或错误码
*
* @return  
* @remark  无
* @see     
* @author  PJ      @date  2022/03/29
************************************************************/
typedef ZXIC_RTN32 (*ZXIC_SLT_ALL_CASE_FN)(ZXIC_UINT32 device_id,ZXIC_UINT32 chip_type,ZXIC_UINT32 *p_case_num,ZXIC_UINT32 *p_case_no,ZXIC_UINT32 *p_case_result);

/*SDK回调函数定义*/
typedef struct zxic_slt_register_fn_t
{
    ZXIC_SLT_INIT_FN           zxic_slt_init_fn;         /* 初始化函数 */
    ZXIC_SLT_PRE_FN            zxic_slt_pre_fn;          /* 预处理函数 */
    ZXIC_SLT_ALL_CASE_FN       zxic_slt_all_case_fn;     /* 用例执行函数*/
    
} ZXIC_SLT_REGISTER_FN_T;

#endif



#if ZXIC_REAL("fn for sdk")
/***********************************************************/
/** 回调功能注册函数，不区分芯片ID，仅注册一次即可，提供给SDK设置
* @return
* @remark  无
* @see
* @author  PJ      @date  2020/05/03
************************************************************/
ZXIC_RTN32 zxic_slt_register_fn_set(ZXIC_SLT_REGISTER_FN_T *pExcCall);

/***********************************************************/
/** 获取POWER函数
* @param   pExcCall
*
* @return
* @remark  无
* @see
* @author  PJ      @date  2020/05/03
************************************************************/
ZXIC_RTN32 zxic_slt_callback_getpower(ZXIC_VOID *pExcCall);

/***********************************************************/
/** 获取MODI函数
* @param   pExcCall
*
* @return
* @remark  无
* @see
* @author  PJ      @date  2020/05/03
************************************************************/
ZXIC_RTN32 zxic_slt_callback_riscv(ZXIC_VOID *pExcCall);

/***********************************************************/
/** 获PCIE函数
* @param   pExcCall
*
* @return
* @remark  无
* @see
* @author  PJ      @date  2020/05/03
************************************************************/
ZXIC_RTN32 zxic_slt_callback_pcie(ZXIC_VOID *pExcCall);

/***********************************************************/
/** 获取VOLT函数
* @param   pExcCall
*
* @return
* @remark  无
* @see
* @author  PJ      @date  2020/05/03
************************************************************/
ZXIC_RTN32 zxic_slt_callback_adjust_volt(ZXIC_VOID *pExcCall);

/***********************************************************/
/** 获取CLK函数
* @param   pExcCall
*
* @return
* @remark  无
* @see
* @author  PJ      @date  2020/05/03
************************************************************/
ZXIC_RTN32 zxic_slt_callback_adjust_clk(ZXIC_VOID *pExcCall);


#endif


#if ZXIC_REAL("fn for bsp")

/***********************************************************/
/** 回调功能注册函数，不区分芯片ID，仅注册一次即可，提供给SDK设置
* @return
* @remark  无
* @see
* @author  PJ      @date  2020/05/03
************************************************************/
ZXIC_RTN32 zxic_slt_bsp_register_fn_set(ZXIC_SLT_BSP_REGISTER_FN_T *pExcCall);


/***********************************************************/
/** 设置是否删除log标记,提供给BSP调用
* @return
* @remark  无
* @see
* @author  PJ      @date  2020/05/03
************************************************************/
ZXIC_VOID zxic_slt_log_rm_flag_set(ZXIC_UINT32 log_rm_flag);


/***********************************************************/
/** 执行SDK SLT 所有用例测试,提供给BSP调用
* @param   dev_id   
* @param   chip_type   芯片封装类型 
* @param   p_init_ctrl  详细参见ZXIC_SLT_INIT_CTRL_T说明
* @param   p_case_para  详细参见ZXIC_SLT_CASE_PARA_T说明
*
* @return  
* @remark  无
* @see     
* @author  PJ      @date  2022/03/29
************************************************************/
ZXIC_RTN32 zxic_slt_all_case_test(ZXIC_UINT32 dev_id,ZXIC_UINT32 chip_type, ZXIC_SLT_INIT_CTRL_T *p_init_ctrl,ZXIC_SLT_CASE_PARA_T *p_case_para);

#endif



#ifdef __cplusplus
}
#endif

#endif /* end __ZXIC_SLT_H__ */


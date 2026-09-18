/**************************************************************
* 版权所�? (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_se.h
* 文件标识 : 
* 内容摘要 : 
* 其它说明 : 
* 当前版本 : 
* �?    �? : 王春�?
* 完成日期 : 2014/03/11
* DEPARTMENT: ASIC_FPGA_R&D_Dept 
* MANUAL_PERCENT: 100%   
 
* 修改记录1: 
* 修改日期:  
* �? �? �?:  
* �? �? �?:  
* 修改内容:  
***************************************************************/

#ifndef _DPP_SE_H_
#define _DPP_SE_H_


#ifdef __cplusplus
extern "C"{
#endif


#include "dpp_se_api.h"

#define DPP_HASH_ID_MIN         (0)
#define DPP_HASH_ID_MAX         (3)
#define DPP_HASH_ID_NUM         (4)

#define HASH_BULK_ID_MIN        (0)
#define HASH_BULK_ID_MAX        (7)
#define HASH_BULK_NUM           (8)    /* 每个Hash引擎存储资源划分的块�? */

#define CRC_POLY_SEL_MIN        (0)
#define CRC_POLY_SEL_MAX        (3)

#define DPP_LPM_ID_MIN          (4)
#define DPP_LPM_ID_MAX          (5)
#define DPP_LPM_ID_NUM          (2)

#define DPP_ETCAM_ID_MIN        (0)
#define DPP_ETCAM_ID_MAX        (0)
#define DPP_ETCAM_ID_NUM        (1)

#define DPP_AGE_TBL_ID_MIN      (0)
#define DPP_AGE_TBL_ID_MAX      (15)

#define DPP_SMMU1_DDR_GRP_NUM               (1)

//#define DPP_SMMU1_DIR_TBL_BANK_MAX          (15)
#define DPP_SMMU1_DIR_TBL_INDEX_MAX         (255)
//#define DPP_SMMU1_UN_DIR_TBL_BANK_MAX       (29)

#define DPP_SMMU1_HASH_TBL_INDEX_BASE       (1)
#define DPP_SMMU1_HASH_TBL_INDEX_MAX        (31)
#define DPP_SMMU1_LPM_TBL_INDEX_BASE        (33)
#define DPP_SMMU1_LPM_TBL_INDEX_MAX         (3)
#define DPP_SMMU1_OAM_TBL_INDEX_BASE        (37)
#define DPP_SMMU1_FTM_TBL_INDEX_BASE        (38)
#define DPP_SMMU1_ETM_TBL_INDEX_BASE        (39)
#define DPP_SMMU1_DIR_TBL_INDEX_BASE        (40)


#define DPP_SMMU1_SINGLE_BNAK_MAX_ADDR      ((1<<25) - 1)
#define DPP_SMMU1_SINGLE_BANK_MAX_BADDR     (DPP_SMMU1_SINGLE_BNAK_MAX_ADDR >> 12)
#define DPP_SMMU1_TOTAL_BANK_NUM            (8)
#define DPP_SMMU1_TOTAL_MAX_ADDR            (0xffffffff)
#define DPP_SMMU1_TOTAL_MAX_BADDR           (DPP_SMMU1_TOTAL_MAX_ADDR >> 12)
#define DPP_SMMU1_BADDR_MASK                (0x7ffff800)
#define DPP_SMMU1_DDR_GROUP_NUM             (1)
#define DPP_SMMU1_BANK_COPY_MAX_NUM         (16)
#define DPP_SMMU1_READ_REG_MAX_NUM          (16)
#define DPP_DIR_TBL_BUF_MAX_NUM             (DPP_SMMU1_READ_REG_MAX_NUM)

/*hash ext crc cfg*/
#define HASH_ECC_EN_BT_START          (2)
#define HASH_ECC_EN_BT_WIDTH          (1)
#define HASH_BANK_COPY_BT_START       (3)
#define HASH_BANK_COPY_BT_WIDTH       (3)
#define HASH_BASE_ADDR_BT_START       (6)
#define HASH_BASE_ADDR_BT_WIDTH       (15)
/*hash learn  tbl cfg*/
#define LEARN_HASH_TBL_BT_START       (0)
#define LEARN_HASH_TBL_BT_WIDTH       (19)

#define DPP_SMMU0_MCAST_TBL_MAX_GROUP       (0xffff)

#define DPP_SMMU0_CAR0_MONO_POS             (0)
#define DPP_SMMU0_CAR0_MONO_LEN             (1)
#define DPP_SMMU0_CAR0_EN_POS               (1)
#define DPP_SMMU0_CAR0_EN_LEN               (1)
#define DPP_SMMU0_CAR1_MONO_POS             (2)
#define DPP_SMMU0_CAR1_MONO_LEN             (1)
#define DPP_SMMU0_CAR1_EN_POS               (3)
#define DPP_SMMU0_CAR1_EN_LEN               (1)

#define DPP_SMMU0_LPM_AS_TBL_ID_MAX         (7)
#define DPP_SMMU0_LPM_AS_TBL_ID_NUM         (8)

#define DPP_SMMU0_MCAST_DATA_VLD_POS        (16)
#define DPP_SMMU0_MCAST_DATA_VLD_LEN        (1)
#define DPP_SMMU0_MCAST_CNT_POS             (0)
#define DPP_SMMU0_MCAST_CNT_LEN             (16)

#define DPP_SMMU0_INDIER_RDWR_OFFSET_NUM    (4)
#define DPP_SMMU0_READ_REG_MAX_NUM          (4)

/**  SMMU0 调度 fifo ecc 使能标志 */
#define DPP_SMMU0_PPU_FIFO_POS               (12)
#define DPP_SMMU0_PPU_FIFO_LEN               (8)
#define DPP_SMMU0_STAT_FIFO_POS              (11)
#define DPP_SMMU0_STAT_FIFO_LEN              (1)
#define DPP_SMMU0_DMA_FIFO_POS               (10)
#define DPP_SMMU0_DMA_FIFO_LEN               (1)
#define DPP_SMMU0_ODMA_FIFO_POS              (6)
#define DPP_SMMU0_ODMA_FIFO_LEN              (4)
#define DPP_SMMU0_MCAST_FIFO_POS             (5)
#define DPP_SMMU0_MCAST_FIFO_LEN             (1)
#define DPP_SMMU0_ETCAM_FIFO_POS             (1)
#define DPP_SMMU0_ETCAM_FIFO_LEN             (4)
#define DPP_SMMU0_LPM_FIFO_POS               (0)
#define DPP_SMMU0_LPM_FIFO_LEN               (1)

#define DPP_SMMU0_CTRL_ECC_CFG_POS           (0)
#define DPP_SMMU0_CTRL_ECC_CFG_LEN           (3)

#define DPP_SMMU0_RSCHD_RAM_POS              (0)
#define DPP_SMMU0_RSCHD_RAM_LEN              (1)

#define DPP_SMMU0_ERAM_ECC_CFG_POS           (0)
#define DPP_SMMU0_ERAM_ECC_CFG_LEN           (24)

#define DPP_SMMU0_WR_ARB_ECC_CFG_POS         (0)
#define DPP_SMMU0_WR_ARB_ECC_CFG_LEN         (1)

/* smmu0 int0 reg bit define */
#define SMMU0_INT0_DMA_ORDFIFO_START        (0)
#define SMMU0_INT0_DMA_ORDFIFO_LEN          (1)
#define SMMU0_INT0_ODMA_ORDFIFO_START       (1)
#define SMMU0_INT0_ODMA_ORDFIFO_LEN         (1)
#define SMMU0_INT0_MCAST_ORDFIFO_START      (2)
#define SMMU0_INT0_MCAST_ORDFIFO_LEN        (1)

#define DPP_ERAM128_BADDR_MASK              (0x3FFFF80)/* modified for dpp+ 25bit 2018-09-27*/

#define DPP_SE_SMMU1_MAX_BADDR_NO_SHARE     ((1<<20)-1)
#define DPP_SE_SMMU1_MAX_BADDR_SHARE        ((1<<13)-1)
#define DPP_SE_SMMU1_MAX_ADDR               ((1<<30)-1)

#define DPP_SE_SMMU1_BANK_NUM_POS           (16)
#define DPP_SE_SMMU1_BANK_NUM_LEN           (5)

#define DPP_SE_SMMU1_SHARE_TYPE_POS         (21)
#define DPP_SE_SMMU1_SHARE_TYPE_LEN         (2)

#define DPP_SE_SMMU1_RR_STATE_POS           (0)
#define DPP_SE_SMMU1_RR_STATE_LEN           (15)

#define DPP_SE_CFG_PPU_INFO_POS             (0)
#define DPP_SE_CFG_PPU_INFO_LEN             (12)

#define DPP_SE_CFG_DPI_FLAG_POS             (12)
#define DPP_SE_CFG_DPI_FLAG_LEN             (1)

#define DPP_SE_CFG_WR_FLAG_POS              (13)
#define DPP_SE_CFG_WR_FLAG_LEN              (1)


/** 中断相关 */
#define DPP_SE_ALG_SCHD_INT_NUM             (14)
#define DPP_SE_ALG_ZBLK_ECC_INT_NUM         (32)
#define DPP_SE_ALG_HASH0_INT_NUM            (8)
#define DPP_SE_ALG_HASH1_INT_NUM            (8)
#define DPP_SE_ALG_HASH2_INT_NUM            (8)
#define DPP_SE_ALG_HASH3_INT_NUM            (8)
#define DPP_SE_ALG_LPM_INT_NUM              (10)

#define DPP_SMMU0_CLS_NUM                   (6)
#define DPP_SMMU0_STAT_NUM                  (10)
#define DPP_SMMU0_AS_ETCAM_NUM              (DPP_ETCAM_ID_NUM)
#define DPP_SMMU0_PLCR_NUM                  (1)
#define DPP_SMMU0_ERAM_BLOCK_NUM            (32)

#define DPP_SMMU1_SCH_CNT                   (4)/*sch 调度次数 完成6->4调度*/
//#define DPP_SMMU1_GRP_CNT                   (DPP_SMMU1_DDR_GRP_NUM)
#define DPP_SMMU1_GRP_CNT                   (8) /*与reg n_size保持一�?*/
#define DPP_SMMU1_DIR_CHANNEL_CNT           (4)

#define DPP_PARSE_MEX_CHANNEL_NUM           (6)
#define DPP_PARSE_KSCHD_CHANNEL_NUM         (6)
#define DPP_RSCHD_PPU_CHANNEL_NUM           (6)

typedef enum smmu1_stat_type_e
{
    STAT_TYPE_PPU     = 0,
    STAT_TYPE_OAM     = 1,
    STAT_TYPE_MAX,
}SMMU1_STAT_TYPE_E;

typedef enum alg_lpm_type_e
{
    ALG_LPM_V4    = 1,
    ALG_LPM_V6    = 2,
    ALG_LPM_V4_AS = 3,
    ALG_LPM_V6_AS = 4,
    ALG_LPM_MAX
}ALG_LPM_TYPE_E;

typedef enum se_ddr_bank_info_e
{
    SE_DDR_BKINFO_LPM4    = 0,
    SE_DDR_BKINFO_LPM6    = 1,
    SE_DDR_BKINFO_LPM4_AS = 2,
    SE_DDR_BKINFO_LPM6_AS = 3,
}SE_DDR_BANK_INFO_E;

typedef enum alg_zblk_serv_type_e
{
    ALG_ZBLK_SERV_LPM = 0,
    ALG_ZBLK_SERV_HASH,
}ALG_ZBLK_SERV_TYPE_E;

typedef enum cmmu_ddr3_bank_enable_e
{
    CMMU_DDR3_BANK_DISABLE = 0,
    CMMU_DDR3_BANK_ENABLE,
}CMMU_DDR3_BANK_ENABLE_E;

/** TM统计读片外位�? */
typedef enum stat_tm_rd_ddr_mode_e
{
    STAT_TM_RD_DDR_MODE_128 = 0,
    STAT_TM_RD_DDR_MODE_256 = 1,
    STAT_TM_RD_DDR_MODE_512 = 2,
    STAT_TM_RD_DDR_MODE_MAX,
}STAT_TM_RD_DDR_MODE_E;

typedef enum stat_tm_rd_clr_mode_e
{
    STAT_TM_RD_CLR_MODE_UNCLR     = 0,  /**<  @brief 正常读，读完数据不清�?   */
    STAT_TM_RD_CLR_MODE_CLR       = 1,  /**<  @brief 读清模式*/
    STAT_TM_RD_CLR_MODE_MAX,
}STAT_TM_RD_CLR_MODE_E;

typedef enum se_ddr_map_flag_e
{
    VIR_TO_PHY_FLAG       = 0,  /**<  @brief 虚拟地址/bank到物理地址/bank的映�?   */
    PHY_TO_VIR_FLAG       = 1,  /**<  @brief 物理地址/bank到虚拟地址/bank的映�?   */
}SE_DDR_MAP_FLAG_E;

/**  module se*/
typedef enum module_init_se_e
{
    MODULE_INIT_SE_SMMU0 = 0,
    MODULE_INIT_SE_SMMU1,
    MODULE_INIT_SE_ALG,
    MODULE_INIT_SE_AS,
    MODULE_INIT_SE_ETCAM,
    MODULE_INIT_SE_STAT,
    MODULE_INIT_SE_FIFO,
    MODULE_INIT_SE_MAX
} MODULE_INIT_SE_E;

typedef struct smmu1_kschd_hash_ddr_cfg_t
{
    ZXIC_UINT32 baddr;
    ZXIC_UINT32 crcen;
    ZXIC_UINT32 mode;
}SMMU1_KSCHD_HASH_DDR_CFG_T;

typedef struct smmu1_kschd_lpm_ddr_cfg_t
{
    ZXIC_UINT32 baddr;
    ZXIC_UINT32 bankcopy;
    ZXIC_UINT32 crcen;
    ZXIC_UINT32 flag;  /* 0-256, 1-384 */
    ZXIC_UINT32 as_baddr;
    ZXIC_UINT32 as_bankcopy;
    ZXIC_UINT32 as_crcen;
    ZXIC_UINT32 as_mode;
}SMMU1_KSCHD_LPM_DDR_CFG_T;

typedef struct dpp_lpm_as_eram_info_t
{
    ZXIC_UINT32 as_baddr;
    ZXIC_UINT32 as_mode;
}DPP_LPM_AS_ERAM_INFO_T;

typedef struct dpp_lpm_res_info_t
{
    DPP_LPM_AS_ERAM_INFO_T as_eram_info[DPP_SMMU0_LPM_AS_TBL_ID_NUM];
    ZXIC_UINT32 v4_ddr_baddr;
    ZXIC_UINT32 v4_as_ddr_baddr;
    ZXIC_UINT32 v4_as_rsp_len;
    ZXIC_UINT32 v6_ddr_baddr;
    ZXIC_UINT32 v6_as_ddr_baddr;
    ZXIC_UINT32 v6_as_rsp_len;
}DPP_LPM_RES_INFO_T;

/*--------------------------------------------------------调试打印计数--------------------------------------------------------*/

/*smmu0 调试打印计数 开�?*/
typedef struct dpp_smmu0_dbg_cnt_t
{
    ZXIC_UINT32 smmu0_rcv_as_age_req_cnt;
    ZXIC_UINT32 smmu0_rcv_parse_req_cnt;
    ZXIC_UINT32 smmu0_cpu_ind_rd_rsp_cnt;
    ZXIC_UINT32 smmu0_cpu_ind_rd_req_cnt;
    ZXIC_UINT32 smmu0_cpu_ind_wr_req_cnt;
    
    ZXIC_UINT32 smmu0_to_plcr_rsp_cnt[DPP_SMMU0_PLCR_NUM];
    ZXIC_UINT32 smmu0_rcv_plcr_req_cnt[DPP_SMMU0_PLCR_NUM];
    
    ZXIC_UINT32 smmu0_to_lpm_as_rsp_cnt;
    ZXIC_UINT32 smmu0_rcv_lpm_as_req_cnt;
    
    ZXIC_UINT32 smmu0_to_as_etacm_rsp_cnt[DPP_SMMU0_AS_ETCAM_NUM];/* 与fc是反�? */
    ZXIC_UINT32 smmu0_rcv_as_etacm_req_cnt[DPP_SMMU0_AS_ETCAM_NUM];

    ZXIC_UINT32 smmu0_to_ppu_mc_rsp_cnt;
    ZXIC_UINT32 smmu0_rcv_ppu_mc_req_cnt;
    ZXIC_UINT32 smmu0_to_odma_tdm_mc_rsp_cnt;
    ZXIC_UINT32 smmu0_rcv_odma_tdm_mc_req_cnt;
    ZXIC_UINT32 smmu0_to_odma_rsp_cnt;
    ZXIC_UINT32 smmu0_rcv_odma_req_cnt;
    ZXIC_UINT32 smmu0_to_dma_rsp_cnt;
    ZXIC_UINT32 smmu0_rcv_dma_req_cnt;
    
    ZXIC_UINT32 smmu0_to_stat_rsp_cnt[DPP_SMMU0_STAT_NUM];
    ZXIC_UINT32 smmu0_rcv_stat_req_cnt[DPP_SMMU0_STAT_NUM];
    ZXIC_UINT32 smmu0_to_ppu_rsp_cnt[DPP_SMMU0_CLS_NUM];
    ZXIC_UINT32 smmu0_rcv_ppu_req_cnt[DPP_SMMU0_CLS_NUM];

    ZXIC_UINT32 smmu0_rcv_ftm_stat_req0_cnt;
    ZXIC_UINT32 smmu0_rcv_ftm_stat_req1_cnt;
    ZXIC_UINT32 smmu0_rcv_etm_stat_req0_cnt;
    ZXIC_UINT32 smmu0_rcv_etm_stat_req1_cnt;

    ZXIC_UINT32 smmu0_block_rd_cnt[DPP_SMMU0_ERAM_BLOCK_NUM];
    ZXIC_UINT32 smmu0_block_wr_cnt[DPP_SMMU0_ERAM_BLOCK_NUM];

}DPP_SMMU0_DBG_CNT_T;

typedef struct dpp_smmu0_dbg_fc_cnt_t
{
    ZXIC_UINT32 smmu0_to_as_age_req_fc_cnt;
    ZXIC_UINT32 smmu0_to_parse_req_fc_cnt;
    ZXIC_UINT32 smmu0_rcv_wr_arb_cpu_fc_cnt;
    ZXIC_UINT32 smmu0_to_as_lpm_req_fc_cnt;
    ZXIC_UINT32 smmu0_rcv_as_lpm_rsp_fc_cnt;
    ZXIC_UINT32 smmu0_to_as_etacm_req_fc_cnt[DPP_SMMU0_AS_ETCAM_NUM];
    ZXIC_UINT32 smmu0_rcv_as_etacm_rsp_fc_cnt[DPP_SMMU0_AS_ETCAM_NUM];
    ZXIC_UINT32 smmu0_to_ppu_mc_req_fc_cnt;
    ZXIC_UINT32 smmu0_rcv_ppu_mc_rsp_fc_cnt;
    ZXIC_UINT32 smmu0_rcv_odma_tdm_mc_rsp_fc_cnt;
    ZXIC_UINT32 smmu0_to_odma_tdm_mc_req_fc_cnt;
    ZXIC_UINT32 smmu0_to_odma_req_fc_cnt;
    ZXIC_UINT32 smmu0_to_dma_req_fc_cnt;
    ZXIC_UINT32 smmu0_to_stat_req_fc_cnt[DPP_SMMU0_STAT_NUM];
    ZXIC_UINT32 smmu0_rcv_stat_rsp_fc_cnt[DPP_SMMU0_STAT_NUM];
    ZXIC_UINT32 smmu0_to_ppu_req_fc_cnt[DPP_SMMU0_CLS_NUM];
    ZXIC_UINT32 smmu0_rcv_ppu_rsp_fc_cnt[DPP_SMMU0_CLS_NUM];
}DPP_SMMU0_DBG_FC_CNT_T;
/*smmu0 调试打印计数 结束*/

/*smmu1 计数�?*/
typedef struct dpp_smmu1_dbg_cnt_t
{
    ZXIC_UINT32 ctrl_to_cash_fc_cnt[DPP_SMMU1_GRP_CNT];
    ZXIC_UINT32 cash_to_ctrl_req_cnt[DPP_SMMU1_GRP_CNT];
    ZXIC_UINT32 rschd_to_cache_fc_cnt[DPP_SMMU1_GRP_CNT];
    ZXIC_UINT32 cash_to_cache_rsp_cnt[DPP_SMMU1_GRP_CNT];
    ZXIC_UINT32 cash_to_ctrl_fc_cnt[DPP_SMMU1_GRP_CNT];
    ZXIC_UINT32 ctrl_to_cash_rsp_cnt[DPP_SMMU1_GRP_CNT];
    ZXIC_UINT32 kschd_to_cache_req_cnt[DPP_SMMU1_GRP_CNT];
    ZXIC_UINT32 cache_to_kschd_fc_cnt[DPP_SMMU1_GRP_CNT];
    ZXIC_UINT32 dma_to_smmu1_rd_req_cnt;
    ZXIC_UINT32 oam_to_kschd_req_cnt;
    ZXIC_UINT32 oam_rr_state_rsp_cnt;
    ZXIC_UINT32 oam_clash_info_cnt;
    ZXIC_UINT32 oam_to_rr_req_cnt;
    ZXIC_UINT32 lpm_as_to_kschd_req_cnt;
    ZXIC_UINT32 lpm_as_rr_state_rsp_cnt;
    ZXIC_UINT32 lpm_as_clash_info_cnt;
    ZXIC_UINT32 lpm_as_to_rr_req_cnt;
    ZXIC_UINT32 lpm_to_kschd_req_cnt;
    ZXIC_UINT32 lpm_rr_state_rsp_cnt;
    ZXIC_UINT32 lpm_clash_info_cnt;
    ZXIC_UINT32 lpm_to_rr_req_cnt;
    ZXIC_UINT32 hash_to_kschd_req_cnt[DPP_HASH_ID_NUM];
    ZXIC_UINT32 hash_rr_state_rsp_cnt[DPP_HASH_ID_NUM];
    ZXIC_UINT32 hash_clash_info_cnt[DPP_HASH_ID_NUM];
    ZXIC_UINT32 hash_to_rr_req_cnt[DPP_HASH_ID_NUM];
    ZXIC_UINT32 dir_to_kschd_req_cnt[DPP_SMMU1_DIR_CHANNEL_CNT];
    ZXIC_UINT32 dir_clash_info_cnt[DPP_SMMU1_DIR_CHANNEL_CNT];
    ZXIC_UINT32 dir_tbl_wr_req_cnt;
    ZXIC_UINT32 warbi_to_dir_tbl_warbi_fc_cnt;
    ZXIC_UINT32 dir_to_bank_rr_req_cnt[DPP_SMMU1_DIR_CHANNEL_CNT];
    ZXIC_UINT32 kschd_to_dir_fc_cnt[DPP_SMMU1_DIR_CHANNEL_CNT];
    ZXIC_UINT32 dir_rr_state_rsp_cnt[DPP_SMMU1_DIR_CHANNEL_CNT];
    ZXIC_UINT32 wr_done_to_warbi_fc_cnt;
    ZXIC_UINT32 wr_done_ptr_req_cnt;
    ZXIC_UINT32 ctrl_to_warbi_fc_cnt[DPP_SMMU1_GRP_CNT];
    ZXIC_UINT32 warbi_to_ctrl_wr_req_cnt[DPP_SMMU1_GRP_CNT];
    ZXIC_UINT32 warbi_to_cash_wr_req_cnt[DPP_SMMU1_GRP_CNT];
    ZXIC_UINT32 warbi_to_cpu_wr_fc_cnt;
    ZXIC_UINT32 cpu_wr_req_cnt;
    ZXIC_UINT32 ctrl_to_cpu_rd_rsp_cnt[DPP_SMMU1_GRP_CNT];
    ZXIC_UINT32 cpu_to_ctrl_rd_req_cnt[DPP_SMMU1_GRP_CNT];
    ZXIC_UINT32 cpu_rd_dir_tbl_rsp_cnt;
    ZXIC_UINT32 cpu_to_dir_tbl_rd_wr_req_cnt;
    ZXIC_UINT32 smmu1_to_mmu_rsp_fc_cnt[DPP_SMMU1_GRP_CNT];
    ZXIC_UINT32 mmu_to_smmu1_rd_rsp_cnt[DPP_SMMU1_GRP_CNT];
    ZXIC_UINT32 mmu_to_smmu1_rd_fc_cnt[DPP_SMMU1_GRP_CNT];
    ZXIC_UINT32 smmu1_to_mmu_rd_req_cnt[DPP_SMMU1_GRP_CNT];
    ZXIC_UINT32 mmu_to_smmu1_wr_fc_cnt[DPP_SMMU1_GRP_CNT];
    ZXIC_UINT32 smmu1_to_mmu_wr_req_cnt[DPP_SMMU1_GRP_CNT];
    ZXIC_UINT32 se_to_smmu1_wr_rsp_fc_cnt;
    ZXIC_UINT32 smmu1_to_se_wr_rsp_cnt;
    ZXIC_UINT32 ddr_wr_rsp_cnt[DPP_SMMU1_GRP_CNT];
    ZXIC_UINT32 smmu1_to_as_fc_cnt;
    ZXIC_UINT32 as_to_smmu1_wr_req_cnt;
    ZXIC_UINT32 smmu1_to_se_parser_fc_cnt;
    ZXIC_UINT32 se_parser_to_smmu1_req_cnt;/* 微码写ddr直接�? */
    ZXIC_UINT32 smmu1_to_etm_wr_fc_cnt;
    ZXIC_UINT32 etm_wr_req_cnt;
    ZXIC_UINT32 smmu1_to_ftm_wr_fc_cnt;
    ZXIC_UINT32 ftm_wr_req_cnt;
    ZXIC_UINT32 smmu1_to_state_wr_fc_cnt;
    ZXIC_UINT32 state_wr_req_cnt;
    ZXIC_UINT32 se_to_dma_rsp_cnt;
    ZXIC_UINT32 se_to_dma_fc_cnt;
    ZXIC_UINT32 oam_to_smmu1_fc_cnt;
    ZXIC_UINT32 smmu1_to_oam_rsp_cnt;
    ZXIC_UINT32 smmu1_to_oam_fc_cnt;
    ZXIC_UINT32 oam_to_smmu1_req_cnt;
    ZXIC_UINT32 smmu1_to_etm_rsp_cnt;
    ZXIC_UINT32 smmu1_to_ftm_rsp_cnt;
    ZXIC_UINT32 smmu1_to_etm_fc_cnt;
    ZXIC_UINT32 etm_to_smmu1_req_cnt;
    ZXIC_UINT32 smmu1_to_ftm_fc_cnt;
    ZXIC_UINT32 ftm_to_smmu1_req_cnt;
    ZXIC_UINT32 smmu1_to_stat_rsp_cnt;
    ZXIC_UINT32 smmu1_to_stat_fc_cnt;
    ZXIC_UINT32 stat_to_smmu1_req_cnt;   /* cmmu */
    ZXIC_UINT32 lpm_as_to_smmu1_fc_cnt;
    ZXIC_UINT32 lpm_to_smmu1_fc_cnt;
    ZXIC_UINT32 smmu1_to_lpm_as_rsp_cnt;
    ZXIC_UINT32 smmu1_to_lpm_rsp_cnt;
    ZXIC_UINT32 smmu1_to_lpm_as_fc_cnt;
    ZXIC_UINT32 smmu1_to_lpm_fc_cnt;
    ZXIC_UINT32 lpm_as_to_smmu1_req_cnt;
    ZXIC_UINT32 lpm_to_smmu1_req_cnt;
    ZXIC_UINT32 hash_to_smmu1_fc_cnt[DPP_HASH_ID_NUM];
    ZXIC_UINT32 smmu1_to_hash_rsp_cnt[DPP_HASH_ID_NUM];
    ZXIC_UINT32 smmu1_to_hash_fc_cnt[DPP_HASH_ID_NUM];
    ZXIC_UINT32 hash_to_smmu1_cnt[DPP_HASH_ID_NUM];
    ZXIC_UINT32 se_to_smmu1_dir_rsp_fc_cnt[DPP_SMMU1_DIR_CHANNEL_CNT];
    ZXIC_UINT32 smmu1_to_se_dir_rsp_cnt[DPP_SMMU1_DIR_CHANNEL_CNT];
    ZXIC_UINT32 smmu1_to_se_dir_fc_cnt[DPP_SMMU1_DIR_CHANNEL_CNT];
    ZXIC_UINT32 se_to_smmu1_dir_cnt[DPP_SMMU1_DIR_CHANNEL_CNT];
    ZXIC_UINT32 cache_to_rschd_rsp_cnt[DPP_SMMU1_GRP_CNT];
}DPP_SMMU1_DBG_CNT_T;

/*parser 计数�?*/
typedef struct se_parser_dbg_cnt_t
{
    ZXIC_UINT32 mex_req_cnt[DPP_PARSE_MEX_CHANNEL_NUM];
    ZXIC_UINT32 kschd_req_cnt[DPP_PARSE_KSCHD_CHANNEL_NUM];
    ZXIC_UINT32 kschd_parser_fc_cnt[DPP_PARSE_KSCHD_CHANNEL_NUM];
    ZXIC_UINT32 se_ppu_mex_fc_cnt[DPP_PARSE_MEX_CHANNEL_NUM];
    ZXIC_UINT32 smmu0_marc_fc_cnt;
    ZXIC_UINT32 smmu0_marc_key_cnt;
    ZXIC_UINT32 smmu1_key_cnt;
    ZXIC_UINT32 smmu1_parser_fc_cnt;
    ZXIC_UINT32 marc_tab_type_err_mex_cnt[DPP_PARSE_MEX_CHANNEL_NUM];
    ZXIC_UINT32 eram_fulladdr_drop_cnt;
}SE_PARSER_DBG_CNT_T;

/*kschd 计数�?*/
typedef struct se_kschd_dbg_cnt_t
{
    ZXIC_UINT32 parser_kschd_key_cnt[DPP_PARSE_KSCHD_CHANNEL_NUM];
    ZXIC_UINT32 kschd_smmu1_key_cnt[DPP_SMMU1_SCH_CNT];
    ZXIC_UINT32 kschd_to_as_hash0_key_cnt;
    ZXIC_UINT32 kschd_to_as_hash1_key_cnt;
    ZXIC_UINT32 kschd_to_as_hash2_key_cnt;
    ZXIC_UINT32 kschd_to_as_hash3_key_cnt;
    ZXIC_UINT32 kschd_to_as_lpm_key_cnt;
    ZXIC_UINT32 kschd_to_as_etacm0_key_cnt;
    ZXIC_UINT32 kschd_to_as_etacm1_key_cnt;
    ZXIC_UINT32 kschd_to_as_pbu_key_cnt;
    ZXIC_UINT32 kschd_to_parser_fc_cnt[DPP_PARSE_KSCHD_CHANNEL_NUM];
    ZXIC_UINT32 smmu1_kschd_fc_cnt[DPP_SMMU1_SCH_CNT];
    ZXIC_UINT32 kschd_rcv_as_hash0_fc_cnt;
    ZXIC_UINT32 kschd_rcv_as_hash1_fc_cnt;
    ZXIC_UINT32 kschd_rcv_as_hash2_fc_cnt;
    ZXIC_UINT32 kschd_rcv_as_hash3_fc_cnt;
    ZXIC_UINT32 kschd_rcv_as_lpm_fc_cnt;
    ZXIC_UINT32 kschd_rcv_as_etacm0_fc_cnt;
    ZXIC_UINT32 kschd_rcv_as_etacm1_fc_cnt;
    ZXIC_UINT32 kschd_rcv_as_pbu_fc_cnt;
}SE_KSCHD_DBG_CNT_T;

/*rschd 计数�?*/
typedef struct se_rschd_dbg_cnt_t
{
    ZXIC_UINT32 se_ppu_mex_rsp_cnt[DPP_RSCHD_PPU_CHANNEL_NUM];
    ZXIC_UINT32 rschd_rcv_as_hash0_rsp_cnt;
    ZXIC_UINT32 rschd_rcv_as_hash1_rsp_cnt;
    ZXIC_UINT32 rschd_rcv_as_hash2_rsp_cnt;
    ZXIC_UINT32 rschd_rcv_as_hash3_rsp_cnt;
    ZXIC_UINT32 rschd_rcv_as_lpm_rsp_cnt;
    ZXIC_UINT32 rschd_rcv_as_etacm0_rsp_cnt;
    ZXIC_UINT32 rschd_rcv_as_etacm1_rsp_cnt;
    ZXIC_UINT32 rschd_rcv_as_pbu_rsp_cnt;
    ZXIC_UINT32 smmu1_rschd_rsp_cnt[DPP_SMMU1_SCH_CNT];
    ZXIC_UINT32 ppu_se_mex_fc_cnt[DPP_RSCHD_PPU_CHANNEL_NUM];
    ZXIC_UINT32 rschd_to_as_hash0_fc_cnt;
    ZXIC_UINT32 rschd_to_as_hash1_fc_cnt;
    ZXIC_UINT32 rschd_to_as_hash2_fc_cnt;
    ZXIC_UINT32 rschd_to_as_hash3_fc_cnt;
    ZXIC_UINT32 rschd_to_as_lpm_fc_cnt;
    ZXIC_UINT32 rschd_to_as_etacm0_fc_cnt;
    ZXIC_UINT32 rschd_to_as_etacm1_fc_cnt;
    ZXIC_UINT32 rschd_to_as_pbu_fc_cnt;
    ZXIC_UINT32 rschd_smmu1_rdy_cnt[DPP_SMMU1_SCH_CNT];
    ZXIC_UINT32 rschd_rcv_smmu0_wr_done_cnt;
    ZXIC_UINT32 rschd_to_smmu0_wr_done_fc_cnt;
    ZXIC_UINT32 rschd_rcv_smmu1_wr_done_cnt;
    ZXIC_UINT32 rschd_to_smmu1_wr_done_fc_cnt;
    ZXIC_UINT32 rschd_rcv_alg_wr_done_cnt;
    ZXIC_UINT32 rschd_to_alg_wr_done_fc_cnt;
}SE_RSCHD_DBG_CNT_T;

/*cmmu 计数�?*/
typedef struct se_cmmu_dbg_cnt_t
{
    ZXIC_UINT32 stat_cmmu_req_cnt;
    ZXIC_UINT32 cmmu_stat_fc_cnt;
    ZXIC_UINT32 smmu1_cmmu_wr_fc_cnt;
    ZXIC_UINT32 smmu1_cmmu_rd_fc_cnt;
}SE_CMMU_DBG_CNT_T;
/*cmmu req计数�? 结束*/

/*se_as模块 计数�?*/
typedef struct se_as_dbg_cnt_t
{
    ZXIC_UINT32 hash_wr_req_cnt[DPP_HASH_ID_NUM];
    ZXIC_UINT32 smmu0_etcam_fc_cnt[DPP_ETCAM_ID_NUM];
    ZXIC_UINT32 etcam_smmu0_req_cnt[DPP_ETCAM_ID_NUM];
    ZXIC_UINT32 smmu0_etcam_rsp_cnt[DPP_ETCAM_ID_NUM];
    ZXIC_UINT32 as_hla_hash_key_cnt[DPP_HASH_ID_NUM];
    ZXIC_UINT32 as_hla_lpm_key_cnt;
    ZXIC_UINT32 alg_as_hash_rsp_cnt[DPP_HASH_ID_NUM];
    ZXIC_UINT32 alg_as_hash_smf_rsp_cnt[DPP_HASH_ID_NUM];/* 命中计数 */
    ZXIC_UINT32 alg_as_lpm_rsp_cnt;
    ZXIC_UINT32 alg_as_lpm_smf_rsp_cnt;/* 命中计数 */
    ZXIC_UINT32 as_pbu_key_cnt;
    ZXIC_UINT32 pbu_se_dpi_rsp_dat_cnt;
    ZXIC_UINT32 as_etcam_ctrl_req_cnt[DPP_ETCAM_ID_NUM];
    ZXIC_UINT32 etcam_ctrl_as_index_cnt[DPP_ETCAM_ID_NUM];/* 有效返回计数 */
    ZXIC_UINT32 etcam_ctrl_as_hit_cnt[DPP_ETCAM_ID_NUM];
    ZXIC_UINT32 as_smmu0_req_cnt;
    ZXIC_UINT32 learn_hla_wr_cnt;/* 硬件学习 */
    ZXIC_UINT32 as_smmu1_req_cnt;
    ZXIC_UINT32 se_cfg_mac_dat_cnt;/* 到DMA计数 */
    ZXIC_UINT32 alg_as_hash_fc_cnt[DPP_HASH_ID_NUM];
    ZXIC_UINT32 alg_as_lpm_fc_cnt;
    ZXIC_UINT32 as_alg_hash_fc_cnt[DPP_HASH_ID_NUM];
    ZXIC_UINT32 as_alg_lpm_fc_cnt;
    ZXIC_UINT32 as_pbu_fc_cnt;
    ZXIC_UINT32 pbu_se_dpi_key_fc_cnt;
    ZXIC_UINT32 as_etcam_ctrl_fc_cnt[DPP_ETCAM_ID_NUM];
    ZXIC_UINT32 etcam_ctrl_as_fc_cnt[DPP_ETCAM_ID_NUM];
    ZXIC_UINT32 smmu0_as_mac_age_fc_cnt;
    ZXIC_UINT32 alg_learn_fc_cnt;
    ZXIC_UINT32 smmu1_as_fc_cnt;
    ZXIC_UINT32 cfg_se_mac_fc_cnt;
}SE_AS_DBG_CNT_T;


/* hash 是否响应dma流控使能*/
typedef enum se_as_hash_dma_fc_en_e
{
    HASH_EN_DMA_FC     = 0,
    HASH_UN_EN_DMA_FC  = 1,
}SE_AS_HASH_DMA_FC_EN_E;
/*se_as模块 计数�? 结束*/

/**  alg模块调试计数*/
typedef struct se_alg_dbg_cnt_t
{
    ZXIC_UINT32 hash_key_cnt[4];
    ZXIC_UINT32 hash_rsp_cnt[4];
    ZXIC_UINT32 hash_hit_cnt[4];
    ZXIC_UINT32 hash_space_vld_cnt[4];
    ZXIC_UINT32 hash_ddr3_req_vld_cnt[4];
    ZXIC_UINT32 hash_ddr3_rsp_vld_cnt[4];

    ZXIC_UINT32 lpm_key_cnt;
    ZXIC_UINT32 lpm_rsp_cnt;
    ZXIC_UINT32 lpm_hit_cnt;
    ZXIC_UINT32 lpm_key_ddr3_req_vld_cnt; /**<  @brief LPM_SMMU1_P4口用于键�?*/
    ZXIC_UINT32 lpm_key_ddr3_rsp_vld_cnt; /**<  @brief LPM_SMMU1_P4口用于键�?*/
    ZXIC_UINT32 lpm_as_ddr3_req_vld_cnt;  /**<  @brief LPM_SMMU1_P5口用于关联结�?*/
    ZXIC_UINT32 lpm_as_ddr3_rsp_vld_cnt;  /**<  @brief LPM_SMMU1_P5口用于关联结�?*/
}SE_ALG_DBG_CNT_T;

typedef struct se_alg_dbg_excp_cnt_t
{
    ZXIC_UINT32 schd_learn_fifo_int_cnt;
    ZXIC_UINT32 schd_hash_fifo_int_cnt[4];
    ZXIC_UINT32 schd_lpm_fifo_int_cnt;
    ZXIC_UINT32 schd_learn_fifo_parity_err_cnt;
    ZXIC_UINT32 schd_hash_fifo_parity_err_cnt[4];
    ZXIC_UINT32 schd_lpm_fifo_parity_err_cnt;
    ZXIC_UINT32 rd_init_cft_cnt;                 /**<  @brief 初始化过程中出现的CPU读命令冲突计�?*/
    ZXIC_UINT32 zblk_ecc_err_cnt[32];            /**<  @brief 32个zblock的ecc错误计数*/
    ZXIC_UINT32 zcam_hash_parity_err_cnt[4];     /**<  @brief 4个hash业务口的parity错误计数*/
    ZXIC_UINT32 zcam_lpm_err_cnt;                /**<  @brief LPM业务口错误计�?*/

    ZXIC_UINT32 hash_sreq_fifo_parity_err_cnt[4];
    ZXIC_UINT32 hash_sreq_fifo_int_cnt[4];
    ZXIC_UINT32 hash_key_fifo_int_cnt[4];
    ZXIC_UINT32 hash_int_rsp_fifo_parity_err_cnt[4];
    ZXIC_UINT32 hash_ext_rsp_fifo_parity_err_cnt[4];
    ZXIC_UINT32 hash_ext_rsp_fifo_int_cnt[4];
    ZXIC_UINT32 hash_int_rsp_fifo_int_cnt[4];

    ZXIC_UINT32 lpm_ext_rsp_fifo_int_cnt;
    ZXIC_UINT32 lpm_ext_v6_fifo_int_cnt;
    ZXIC_UINT32 lpm_ext_v4_fifo_int_cnt;
    ZXIC_UINT32 lpm_ext_addr_fifo_int_cnt;
    ZXIC_UINT32 lpm_ext_v4_fifo_parity_err_cnt;
    ZXIC_UINT32 lpm_ext_v6_fifo_parity_err_cnt;
    ZXIC_UINT32 lpm_ext_rsp_fifo_parity_err_cnt;
    ZXIC_UINT32 lpm_as_req_fifo_int_cnt;
    ZXIC_UINT32 lpm_as_int_rsp_fifo_int_cnt;
}SE_ALG_DBG_EXCP_CNT_T;


#define LPM_HW_DAT_BUFF_SIZE_MAX (16 * 1024)
typedef enum {
    LPM_DAT_WR_TYPE_DMA     = 1UL,
    LPM_DAT_WR_TYPE_REG     = 2UL,
}LPM_DAT_WR_TYPE;

typedef enum {
    LPM_DAT_ZECLL     = 1UL,
    LPM_DAT_ZREG      = 2UL,
    LPM_DAT_DDR       = 3UL,  
    LPM_DAT_DDR_RST   = 4UL,        
    LPM_DAT_ERAM_RST  = 5UL, 
    LPM_DAT_TYPE_MAX, 
}ROUTE_DAT_TYPE;

typedef struct _lpm_hw_dat_ddr
{
    ZXIC_UINT32         dat_type;     /* ROUTE_DAT_TYPE :LPM_DAT_DDR/LPM_DAT_DDR_RST */
    ZXIC_UINT32         v4v6_flag;    /* ALG_LPM_TYPE_E */
    ZXIC_UINT32         lpm_wr_vld;   /* 0-WR 1-RD */
    ZXIC_UINT32         tbl_id;       /* 对应复制通道 */    
    ZXIC_UINT32         base_addr;    /* 19b */
    ZXIC_UINT32         index;        /* by rw_len */
    ZXIC_UINT32         ecc_en;   
    ZXIC_UINT32         rw_len;       /* SMMU1_DDR_WRT_MODE_E */
    ZXIC_UINT8          data[512/8];  /* 左对�? */   
}ROUTE_HW_DAT_DDR;

typedef struct _lpm_hw_dat_zcam
{
    ZXIC_UINT32         dat_type;       /* ROUTE_DAT_TYPE :LPM_DAT_ZREG/LPM_DAT_ZECLL */
    ZXIC_UINT32         ram_reg_flag;   /* 0-reg 1-cell */
    ZXIC_UINT32         rw_addr;        /* by 512b */
    ZXIC_UINT8          data[512/8];    /* 左对�? */  
}ROUTE_HW_DAT_ZCAM;

typedef struct _lpm_hw_dat_eram
{
    ZXIC_UINT32         dat_type;   /* ROUTE_DAT_TYPE :LPM_DAT_ERAM_RST */
    ZXIC_UINT32         base_addr;  /* by 128b */
    ZXIC_UINT32         index;      /* by rw_len*/
    ZXIC_UINT32         rw_len;     /* DPP_ERAM128_TBL_MODE_E */
    ZXIC_UINT8          data[128/8];/* 左对�? */      
}ROUTE_HW_DAT_ERAM;

typedef struct ppu_stat_cfg_t
{
    ZXIC_UINT32 eram_baddr;           /*片内统计基地址，单位128bit*/
    ZXIC_UINT32 eram_depth;           /*片内深度，单位128bit*/
    ZXIC_UINT32 ddr_base_addr;            /*片外统计基地址(PPU和OAM的片外计数共用此基地址)，单位2k*256bit*/
    ZXIC_UINT32 ppu_addr_offset;      /*PPU统计偏移地址,单位128bit*/
}PPU_STAT_CFG_T;

/***********************************************************/
/** dpp hash的smmu1属性设�?
* @param   dev_id       设备�?
* @param   hash_id      hash引擎�?
* @param   tbl_id       hash表号
* @param   ecc_en       ecc使能
* @param   baddr        ddr基地址
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  �?
* @see     
* @author  ls      @date  2016/04/12
************************************************************/
DPP_STATUS dpp_se_smmu1_hash_tbl_cfg_set(DPP_DEV_T *dev,
                                         ZXIC_UINT32 hash_id,
                                         ZXIC_UINT32 tbl_id,
                                         ZXIC_UINT32 ecc_en,
                                         ZXIC_UINT32 baddr);

/** 获取hash算法访问DDR空间的属性，从软件获取(待优化)
* @param   dev_id    设备号
* @param   hash_id      hash引擎号
* @param   bulk_id      Hash引擎存储资源划分块数的ID号
* @param   p_ecc_en     使能ECC校验
* @param   p_base_addr  DDR空间基地址
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  tf      @date  2016/06/15
************************************************************/
DPP_STATUS dpp_se_smmu1_hash_tbl_soft_cfg_get(DPP_DEV_T *dev,
                                              ZXIC_UINT32 hash_id,
                                              ZXIC_UINT32 bulk_id,
                                              ZXIC_UINT32 *p_ecc_en,
                                              ZXIC_UINT32 *p_base_addr);

DPP_STATUS dpp_se_zblk_serv_cfg_set(DPP_DEV_T *dev, ZXIC_UINT32 zblk_idx, ZXIC_UINT32 serv_sel, ZXIC_UINT32 hash_id, ZXIC_UINT32 enable);

DPP_STATUS dpp_se_zcell_mono_cfg_set(DPP_DEV_T *dev, 
                                     ZXIC_UINT32 zblk_idx, 
                                     ZXIC_UINT32 zcell0_tbl_id, 
                                     ZXIC_UINT32 zcell0_mono_flag, 
                                     ZXIC_UINT32 zcell1_tbl_id,
                                     ZXIC_UINT32 zcell1_mono_flag,
                                     ZXIC_UINT32 zcell2_tbl_id,
                                     ZXIC_UINT32 zcell2_mono_flag,
                                     ZXIC_UINT32 zcell3_tbl_id,
                                     ZXIC_UINT32 zcell3_mono_flag);

DPP_STATUS dpp_se_zcell_mono_cfg_get(DPP_DEV_T *dev, 
                                     ZXIC_UINT32 zblk_idx, 
                                     ZXIC_UINT32 *zcell0_tbl_id, 
                                     ZXIC_UINT32 *zcell0_mono_flag, 
                                     ZXIC_UINT32 *zcell1_tbl_id,
                                     ZXIC_UINT32 *zcell1_mono_flag,
                                     ZXIC_UINT32 *zcell2_tbl_id,
                                     ZXIC_UINT32 *zcell2_mono_flag,
                                     ZXIC_UINT32 *zcell3_tbl_id,
                                     ZXIC_UINT32 *zcell3_mono_flag);

DPP_STATUS dpp_se_zreg_mono_cfg_set(DPP_DEV_T *dev,           
                                    ZXIC_UINT32 zblk_idx,            
                                    ZXIC_UINT32 zreg0_tbl_id,        
                                    ZXIC_UINT32 zreg0_mono_flag,     
                                    ZXIC_UINT32 zreg1_tbl_id,        
                                    ZXIC_UINT32 zreg1_mono_flag,     
                                    ZXIC_UINT32 zreg2_tbl_id,        
                                    ZXIC_UINT32 zreg2_mono_flag,     
                                    ZXIC_UINT32 zreg3_tbl_id,        
                                    ZXIC_UINT32 zreg3_mono_flag);

DPP_STATUS dpp_se_zreg_mono_cfg_get(DPP_DEV_T *dev, 
                                     ZXIC_UINT32 zblk_idx, 
                                     ZXIC_UINT32 *zreg0_tbl_id, 
                                     ZXIC_UINT32 *zreg0_mono_flag, 
                                     ZXIC_UINT32 *zreg1_tbl_id,
                                     ZXIC_UINT32 *zreg1_mono_flag,
                                     ZXIC_UINT32 *zreg2_tbl_id,
                                     ZXIC_UINT32 *zreg2_mono_flag,
                                     ZXIC_UINT32 *zreg3_tbl_id,
                                     ZXIC_UINT32 *zreg3_mono_flag);
DPP_STATUS dpp_se_hash_zcam_mono_flags_set(DPP_DEV_T *dev, 
                                           ZXIC_UINT32 hash0_mono_flag, 
                                           ZXIC_UINT32 hash1_mono_flag, 
                                           ZXIC_UINT32 hash2_mono_flag,
                                           ZXIC_UINT32 hash3_mono_flag);

DPP_STATUS dpp_se_hash_zcam_mono_flags_get(DPP_DEV_T *dev, 
                                           ZXIC_UINT32 *hash0_mono_flag, 
                                           ZXIC_UINT32 *hash1_mono_flag, 
                                           ZXIC_UINT32 *hash2_mono_flag,
                                           ZXIC_UINT32 *hash3_mono_flag);

DPP_STATUS dpp_se_hash_ext_cfg_set(DPP_DEV_T *dev, ZXIC_UINT32 hash_id, ZXIC_UINT32 ext_mode, ZXIC_UINT32 flag);
DPP_STATUS dpp_se_hash_ext_cfg_get(DPP_DEV_T *dev, ZXIC_UINT32 hash_id, ZXIC_UINT32 *p_content_type, ZXIC_UINT32 *p_flag);
DPP_STATUS dpp_se_hash_tbl_depth_set(DPP_DEV_T *dev, 
                                     ZXIC_UINT32 hash_id, 
                                     ZXIC_UINT32 hash_tbl0_depth,          
                                     ZXIC_UINT32 hash_tbl1_depth,          
                                     ZXIC_UINT32 hash_tbl2_depth,          
                                     ZXIC_UINT32 hash_tbl3_depth,          
                                     ZXIC_UINT32 hash_tbl4_depth,      
                                     ZXIC_UINT32 hash_tbl5_depth,      
                                     ZXIC_UINT32 hash_tbl6_depth,      
                                     ZXIC_UINT32 hash_tbl7_depth);
DPP_STATUS dpp_se_hash_tbl_depth_get(DPP_DEV_T *dev, 
                                     ZXIC_UINT32 hash_id, 
                                     ZXIC_UINT32 *hash_tbl0_depth,          
                                     ZXIC_UINT32 *hash_tbl1_depth,          
                                     ZXIC_UINT32 *hash_tbl2_depth,          
                                     ZXIC_UINT32 *hash_tbl3_depth,          
                                     ZXIC_UINT32 *hash_tbl4_depth,      
                                     ZXIC_UINT32 *hash_tbl5_depth,      
                                     ZXIC_UINT32 *hash_tbl6_depth,      
                                     ZXIC_UINT32 *hash_tbl7_depth);

#ifdef __cplusplus
}
#endif

#endif

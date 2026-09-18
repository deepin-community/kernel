/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_stat_cfg.h
* 文件标识 : 
* 内容摘要 : 
* 其它说明 : 
* 当前版本 : 
* 作    者 : ls
* 完成日期 : 2016/03/29
* DEPARTMENT: ASIC_FPGA_R&D_Dept 
* MANUAL_PERCENT: 100%   
 
* 修改记录1: 
* 修改日期:  
* 版 本 号:  
* 修 改 人:  
* 修改内容:  
***************************************************************/
#ifndef _DPP_STAT_CFG_H_
#define _DPP_STAT_CFG_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "dpp_stat_api.h"

#if ZXIC_REAL("Variable definition")

#define         DPP_STAT_TM_PORT_MAX                (4)
#define         DPP_STAT_ETM_ADDR_MAX               (9*1024)
#define         DPP_STAT_FTM_ADDR_MAX               (2048)	
#define         DPP_STAT_IND_WR_MODE                (0)
#define         DPP_STAT_IND_RD_MODE                (1)
#define         DPP_STAT_TM_MOV_PERIOD_MAX          (0xff)

#define         DPP_STAT_WIDTH_3_MAX_VALUE          ((1<<3) - 1)
#define         DPP_STAT_WIDTH_4_MAX_VALUE          ((1<<4) - 1)
#define         DPP_STAT_PPU_ERAM_DEPTH_MAX         (0x7ffff)
#define         DPP_STAT_PPU_ERAM_BADDR_MAX         (0x7ffff)
#define         DPP_STAT_PPU_DDR_BADDR_MAX          (0x4ffffff)

#define         DPP_STAT_OAM_ERAM_BADDR_MAX         (0x7ffff)
#define         DPP_STAT_OAM_DDR_BADDR_MAX          (0x4ffffff)

#define         DPP_STAT_PLCR_ERAM_BADDR_MAX        (0x7ffff)
#define         DPP_STAT_PLCR_ID                (0)

#define         DPP_STAT_TM_FLAG_FTM_PKT_EN         (0)
#define         DPP_STAT_TM_FLAG_ETM_PKT_EN         (1)
#define         DPP_STAT_TM_FLAG_ERAM_EN            (2)

#define         DPP_STAT_PPU_STAT_CHANNEL_NUM       (16)
#define         DPP_STAT_PPU_MEX_NUM                (6)
#define         CMMU_DDR_DIR_CPY_NUM                (15)


/** stat TM类型 */
typedef enum dpp_stat_tm_type_e
{
    DPP_STAT_TM_TYPE_ETM = 0,   /*<@brief FTM模式 */
    DPP_STAT_TM_TYPE_FTM = 1,   /*<@brief ETM模式 */
    DPP_STAT_TM_TYPE_MAX
}DPP_STAT_TM_TYPE_E;


typedef enum dpp_stat_tm_store_mode_e
{
    DPP_STAT_TM_STORE_MODE_ERAM = 0,    /* 片内存储模式 */
    DPP_STAT_TM_STORE_MODE_MIX  = 1,    /* 混合存储模式 */
    DPP_STAT_TM_STORE_MODE_MAX,
}DPP_STAT_TM_STORE_MODE_E;

typedef enum dpp_stat_etm_depth_mode_e
{
    DPP_STAT_ETM_DEPTH_ERAM_1K = 0,
    DPP_STAT_ETM_DEPTH_ERAM_4K = 1,
    DPP_STAT_ETM_DEPTH_ERAM_5K = 2,
    DPP_STAT_ETM_DEPTH_ERAM_8K = 3,
    DPP_STAT_ETM_DEPTH_ERAM_9K = 4,
    DPP_STAT_ETM_DEPTH_MIX_2K  = 5,
    DPP_STAT_ETM_DEPTH_MIX_8K  = 6,
    DPP_STAT_ETM_DEPTH_MIX_9K  = 7,
    DPP_STAT_ETM_DEPTH_MAX,
}DPP_STAT_ETM_DEPTH_MODE_E;

typedef enum stat_store_mode_e
{
    STAT_STORE_MODE_IN_ERAM = 0,
    STAT_STORE_MODE_IN_DDR  = 1,
    STAT_STORE_MODE_MAX,
}STAT_STORE_MODE_E;

typedef enum stat_oam_type_e
{
    STAT_OAM_TYPE_ERAM     = 0,
    STAT_OAM_TYPE_LM_ERAM  = 1,
    STAT_OAM_TYPE_DDR      = 2,
    STAT_OAM_TYPE_MAX,
}STAT_OAM_TYPE_E;

typedef struct dpp_stat_dbg_cnt_t
{
    ZXIC_UINT32 stat_to_smmu0_rsp_fc_cnt[DPP_STAT_PPU_STAT_CHANNEL_NUM];
    ZXIC_UINT32 stat_rcv_smmu0_req_fc_cnt[DPP_STAT_PPU_STAT_CHANNEL_NUM];
    ZXIC_UINT32 stat_to_ppu_req_fc_cnt[DPP_STAT_PPU_MEX_NUM];
    ZXIC_UINT32 stat_rcv_ppu_rsp_fc_cnt[DPP_STAT_PPU_MEX_NUM];
    ZXIC_UINT32 stat_rcv_se_etm_wr_fc_cnt;
    ZXIC_UINT32 stat_rcv_se_etm_rd_fc_cnt;
    ZXIC_UINT32 stat_rcv_se_ftm_wr_fc_cnt;
    ZXIC_UINT32 stat_rcv_se_ftm_rd_fc_cnt;
    ZXIC_UINT32 stat_to_etm_deq_fc_cnt;
    ZXIC_UINT32 stat_to_etm_enq_fc_cnt;
    ZXIC_UINT32 stat_to_ftm_deq_fc_cnt;
    ZXIC_UINT32 stat_to_ftm_enq_fc_cnt;
    ZXIC_UINT32 stat_to_oam_lm_fc_cnt;
    ZXIC_UINT32 stat_rcv_oam_lm_fc_cnt;
    ZXIC_UINT32 stat_to_oam_fc_cnt;
    ZXIC_UINT32 stat_rcv_cmmu_fc_cnt;
    ZXIC_UINT32 stat_to_cmmu_req_cnt;
    ZXIC_UINT32 stat_rcv_smmu0_rsp_cnt[DPP_STAT_PPU_STAT_CHANNEL_NUM];
    ZXIC_UINT32 stat_to_smmu0_req_cnt[DPP_STAT_PPU_STAT_CHANNEL_NUM];
    ZXIC_UINT32 stat_plcr_rcv_smmu0_rsp1_cnt;
    ZXIC_UINT32 stat_plcr_rcv_smmu0_rsp0_cnt;
    ZXIC_UINT32 stat_plcr_to_smmu0_req1_cnt;
    ZXIC_UINT32 stat_plcr_to_smmu0_req0_cnt;
    ZXIC_UINT32 stat_to_ppu_mex_rsp_cnt[DPP_STAT_PPU_MEX_NUM];
    ZXIC_UINT32 stat_oam_lm_rsp_cnt;
    ZXIC_UINT32 stat_rcv_oam_lm_req_cnt;
    ZXIC_UINT32 stat_rcv_oam_req_cnt;
    ZXIC_UINT32 stat_rcv_ppu_mex_key_cnt[DPP_STAT_PPU_MEX_NUM];
    ZXIC_UINT32 stat_rcv_se_etm_rsp_cnt;
    ZXIC_UINT32 stat_rcv_etm_se_wr_req_cnt;
    ZXIC_UINT32 stat_rcv_etm_se_rd_req_cnt;
    ZXIC_UINT32 stat_rcv_se_ftm_rsp_cnt;
    ZXIC_UINT32 stat_to_ftm_se_wr_req_cnt;
    ZXIC_UINT32 stat_to_ftm_se_rd_req_cnt;
    ZXIC_UINT32 stat_rcv_ftm_smmu0_req_cnt0;
    ZXIC_UINT32 stat_rcv_ftm_smmu0_req_cnt1;
    ZXIC_UINT32 stat_rcv_etm_smmu0_req_cnt0;
    ZXIC_UINT32 stat_rcv_etm_smmu0_req_cnt1;
    ZXIC_UINT32 ppu_no_exist_opcd_ex_cnt[DPP_STAT_PPU_MEX_NUM];
    ZXIC_UINT32 stat_rcv_tm_eram_cpu_rsp_cnt;
    ZXIC_UINT32 cpu_rd_eram_req_cnt;
    ZXIC_UINT32 cpu_wr_eram_req_cnt;
    ZXIC_UINT32 tm_stat_ddr_cpu_rsp_cnt;
    ZXIC_UINT32 cpu_rd_ddr_req_cnt;
    ZXIC_UINT32 cpu_wr_ddr_req_cnt;
}DPP_STAT_DBG_CNT_T;

#endif

/***********************************************************/
/** 获取ppu统计片内深度
* @param   dev_id               设备号
* @param   p_ppu_eram_depth     ppu统计片内深度
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/03/31
************************************************************/
DPP_STATUS dpp_stat_ppu_eram_depth_get(DPP_DEV_T *dev,
                                       ZXIC_UINT32* p_ppu_eram_depth);


/***********************************************************/
/** 获取ppu统计 ERAM基地址
* @param   dev_id               设备号
* @param   p_ppu_eram_baddr     ppu统计片内基地址
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/03/31
************************************************************/
DPP_STATUS dpp_stat_ppu_eram_baddr_get(DPP_DEV_T *dev,
                                       ZXIC_UINT32* p_ppu_eram_baddr);

/***********************************************************/
/** 获取ppu统计 ddr基地址
* @param   dev_id               设备号
* @param   p_ppu_ddr_baddr      ppu统计片外基地址
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see     
* @author  ls      @date  2016/03/31
************************************************************/
DPP_STATUS dpp_stat_ppu_ddr_baddr_get(DPP_DEV_T *dev,
                                      ZXIC_UINT32* p_ppu_ddr_baddr);

#ifdef __cplusplus
}
#endif

#endif

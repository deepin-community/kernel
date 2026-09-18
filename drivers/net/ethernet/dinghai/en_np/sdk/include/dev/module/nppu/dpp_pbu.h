/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_pbu.h
* 文件标识 : 
* 内容摘要 : 
* 其它说明 : 
* 当前版本 : 
* 作    者 : djf
* 完成日期 : 2014/04/14
* DEPARTMENT: ASIC_FPGA_R&D_Dept 
* MANUAL_PERCENT: 100%   
 
* 修改记录1: 
* 修改日期:  
* 版 本 号:  
* 修 改 人:  
* 修改内容:  
***************************************************************/
#ifndef  _DPP_PBU_H_
#define  _DPP_PBU_H_

#include "dpp_nppu_reg.h"
#include "dpp_pbu_api.h"

#if ZXIC_REAL("macro")
/******************************************************************************
 *                             START: 宏定义                              *
 *****************************************************************************/
#define DPP_PBU_IND_CMD_WRT_FLAG     (0)
#define DPP_PBU_IND_CMD_RD_FLAG        (1)

#define PBU_FILE_PATH ("sa500t_pbu_output.txt")
#define PBU_FILE_PATH_RAM ("sa500t_pbu_output_ram.txt")

#define DPP_PBU_PORT_NUM         (119)           /** 物理端口号个数(0-128) */ 
#define DPP_PBU_PORT_TH_MAX      (16380)         /** IDMA指针阈值最大值 */

#define DPP_PBU_LIF0_PORT_NUM    (48)            /** lif0 48个通道对应的物理端口号个数(0-47)  */ 
#define DPP_PBU_LIF1_PORT_NUM    (56)            /** lif1 8个通道对应的物理端口号个数(48~55)  */ 
#define DPP_PBU_TM_LOOP_PORT_NUM    (113)            /** TM还回通道113  */ 


#define DPP_PBU_COS_NUM          (8)             /** COS个数*/
#define DPP_PBU_ALL_FTM_LINK_TH_NUM          (6)             /** COS个数*/
#define DPP_PBU_IDMA_MAX_TH      (16380)         /** IDMA指针阈值最大值 */
#define DPP_PBU_LIF_MAX_TH       (16384)         /** LIF指针阈值最大值 */
#define DPP_PBU_MC_MAX_TH        (16380)         /** 组播指针阈值最大值 */
#define DPP_PBU_PORT_COS_MAX_TH  (16380)         /** 单个端口指针阈值最大值 */
#define DPP_PBU_MC_MAX_DIFF_TH   (255)           /** 组播指针申请逻辑复制和微码复制直接的阈值差最大值 */ 
#define DPP_PBU_MC_MIN_DIFF_TH   (5)             /** 组播指针申请逻辑复制和微码复制直接的阈值差最小值 */
#define DPP_PBU_CAP_DATA_MODE_MIN   (0)          /** PBU抓包数据模式最小值 */ 
#define DPP_PBU_CAP_DATA_MODE_MAX   (12)         /** PBU抓包数据模式最大值 */ 

#define DPP_PBU_CAP_FILTER_ADDR         (0x185)
#define DPP_PBU_CAP_PKT_NUM      (64)            /** 抓包的最大分片个数 */

/******************************************************************************
 *                              END: 宏定义                               *
 *****************************************************************************/ 
#endif


#if ZXIC_REAL("struct")


typedef enum npe_pbu_ptr_rotate_e{
    DPP_PBU_TOTAL_PTR_ROTATE = 0X1, /**<  @brief 全局指针翻转*/
    DPP_PBU_MC_PTR_ROTATE    = 0X2,    /**<  @brief 组播指针翻转*/    
    DPP_PBU_PORT_PTR_ROTATE  = 0X4,  /**<  @brief 端口指针翻转*/
 
}DPP_PBU_PTR_ROTATE_E;


/******************************************************************************
 *                            START: 类型定义                             *
 *****************************************************************************/  
typedef struct dpp_pbu_cnt_para_t
{
    ZXIC_UINT32 total_cnt;  /* 总指针计数 */
    ZXIC_UINT32 idma_pub_cnt;  /* idma共享指针计数 */
    ZXIC_UINT32 lif_pub_cnt;  /* lif共享指针计数 */
    ZXIC_UINT32 mc_total_cnt;  /* mc总指针计数 */
} DPP_PBU_CNT_PARA_T;
/*
typedef struct dpp_pbu_module_fc_rdy_t
{
    ZXIC_UINT32 pbu_oam_send_fc_rdy;
    ZXIC_UINT32 pbu_lif_ctrl_rdy;
    ZXIC_UINT32 pbu_odma_fc_rdy;
    ZXIC_UINT32 pbu_tm_fc_rdy;
    ZXIC_UINT32 pbu_idma_cos_rdy;
}DPP_PBU_MODULE_FC_RDY_T;
*/

typedef struct dpp_mf_info_t
{
    ZXIC_CHAR* name;
    ZXIC_UINT32 start_bit;
    ZXIC_UINT32 end_bit;
}DPP_MF_INFO_T;


typedef enum dpp_pbu_ind_mem_id_e
{
    DPP_PBU_IDMATH_RAM= 0,          /**< @brief 端口阈值RAM，可读可写 */
    DPP_PBU_MACTH_RAM = 1,          /**< @brief 端口COS阈值RAM，可读可写 */
    DPP_PBU_CFG_IND_MEM_ID_INVALID = 2,      /**< @brief PBU CFG模块使用的内部表的个数 */
} DPP_PBU_CFG_IND_MEM_ID_E;

typedef enum dpp_pbu_stat_ind_mem_id_e
{
    DPP_PBU_PORT_CNT = 1,           /**< @brief 端口的指针计数，只读 */
    DPP_PBU_STAT = 2,               /**< @brief PBU调试计数，只读 */
    DPP_PBU_IFB_CFG = 3,            /**< @brief IFB中192字节报文，只读 */
    DPP_PBU_CAPTURE_CFG = 4,        /**< @brief 报文抓包 可读可写 */
    DPP_PBU_PORT_PUB_CNT = 5,
    DPP_PBU_IND_MEM_ID_INVALID = 6,     /**< @brief PBU模块使用的内部表的个数*/
} DPP_PBU_STAT_IND_MEM_ID_E;

typedef enum dpp_idma_stat_ind_mem_id_e
{
    DPP_IDMA_STAT_RAM = 0,        
    DPP_IDMA_DEBUG_RAM = 1,             
    DPP_IDMA_IND_MEM_ID_INVALID = 2,     /**< @brief IDMA模块使用的内部表的个数*/
} DPP_IDMA_STAT_IND_MEM_ID_E;

typedef enum dpp_pbu_other_cnt_id_e
{
    DPP_PBU_IDMA_PTR_REQ_CNT  = 0,          /**< @brief idma指针申请计数，含无效申请 */
    DPP_PBU_IDMA_RFD_WR_CNT   = 1,          /**< @brief idma写RFD计数 */
    DPP_PBU_IDMA_IFB_WR1_CNT  = 2,          /**< @brief idma写ifb高64字节计数 */
    DPP_PBU_IDMA_IFB_WR2_CNT  = 3,          /**< @brief idma写ifb低128字节计数 */
    DPP_PBU_PPU_IFB_RD_CNT    = 4,          /**< @brief ppu读ifb申请计数 */
    DPP_PBU_IFB_PPU_RDRSP_CNT = 5,          /**< @brief ifb返回给ppu包头计数 */
    DPP_PBU_ODMA_RECY_PTR_CNT = 6,          /**< @brief odma指针回收个数 */
    DPP_PBU_PPU_PF_REQ0_CNT   = 7,          /**< @brief ppu微码复制申请指针计数 */
    DPP_PBU_PBU_PF_RSP0_CNT   = 8,          /**< @brief pbu返回微码复制申请指针计数 */
    DPP_PBU_PPU_PF_REQ1_CNT   = 9,          /**< @brief ppu逻辑复制申请指针计数 */
    DPP_PBU_PBU_PF_RSP1_CNT   = 10,         /**< @brief pbu返回逻辑复制申请指针计数 */
    DPP_PBU_PPU_USE_PTR_CNT   = 11,         /**< @brief idma有效申请指针计数 */
    DPP_PBU_PPU_WRBK_CNT      = 12,         /**< @brief ppu回写计数 */
    DPP_PBU_PPU_REORDER_RSP_CNT  = 13,      /**< @brief pbu返回ppu回写参数计数 */
    DPP_PBU_SE_PBU_KEY_VLD_CNT   = 14,      /**< @brief 深度解析请求计数 */
    DPP_PBU_PBU_SE_RSP_VLD_CNT   = 15,      /**< @brief 深度解析返回计数 */
    DPP_PBU_ODMA_IFB_RD1_CNT     = 16,      /**< @brief odma读ifb接口1读请求计数 */
    DPP_PBU_ODMA_IFB_RD2_CNT     = 17,      /**< @brief odma读ifb接口2读请求计数 */
    DPP_PBU_IDMA_O_ISU_PKT_CNT   = 18,      /**< @brief idma输出报文总计数 */
    DPP_PBU_IDMA_O_ISU_EPKT_CNT  = 19,      /**< @brief idma输出带error标记的报文总计数 */
    DPP_PBU_IDMA_DISPKT_CNT      = 20,      /**< @brief idma丢弃报文总数 */
    DPP_PBU_OTHER_CNT_ID_INVALID,           /**< @brief PBU模块other计数的个数*/
} DPP_PBU_OTHER_CNT_ID_E;


typedef struct dpp_pbu_port_ptr_cnt_t
{
    ZXIC_UINT32 peak_port_cnt; /**< @brief 端口指针峰值 */
    ZXIC_UINT32 current_port_cnt;/**< @brief 端口指针占用 */
} DPP_PBU_PORT_PTR_CNT_T;

typedef struct dpp_pbu_ifb_data_t
{
    ZXIC_UINT32 pbu_ifb_data[64];/**< @brief ifb数据 */
} DPP_PBU_IFB_DATA_T;


typedef struct dpp_pbu_all_ftm_link_th_t
{
    ZXIC_UINT32 total_congest_th[7];   /**<  @brief 拥塞阈值*/
} DPP_PBU_ALL_FTM_LINK_TH_T;

typedef struct
{
    ZXIC_UINT32 pbu_lif_group0_pfc_rdy[12];
} DPP_PBU_LIF_GROUP_PFC_RDY;



/******************************************************************************
 *                             END: 类型定义                              *
 *****************************************************************************/ 
#endif

#if 1
/******************************************************************************
 *                            START: 函数声明                             *
 *****************************************************************************/
#if 0
DPP_STATUS dpp_pbu_idma_public_th_set(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 th);
DPP_STATUS dpp_pbu_idma_public_th_get(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 *p_th);
DPP_STATUS dpp_pbu_lif_public_th_set(ZXIC_UINT32 dev_id,
                                         ZXIC_UINT32 th);
DPP_STATUS dpp_pbu_lif_public_th_get(ZXIC_UINT32 dev_id,
                                         ZXIC_UINT32 *p_th);
DPP_STATUS dpp_pbu_idma_total_th_set(ZXIC_UINT32 dev_id,
                                         ZXIC_UINT32 th);
DPP_STATUS dpp_pbu_idma_total_th_get(ZXIC_UINT32 dev_id,
                                         ZXIC_UINT32 *p_th);
DPP_STATUS dpp_pbu_lif_total_th_set(ZXIC_UINT32 dev_id,
                                        ZXIC_UINT32 th);
DPP_STATUS dpp_pbu_lif_total_th_get(ZXIC_UINT32 dev_id,
                                        ZXIC_UINT32 *p_th);
DPP_STATUS dpp_pbu_mc_total_th_set(ZXIC_UINT32 dev_id,
                                       ZXIC_UINT32 th);
DPP_STATUS dpp_pbu_mc_total_th_get(ZXIC_UINT32 dev_id,
                                       ZXIC_UINT32 *p_th);

DPP_STATUS dpp_pbu_mc_cos_para_get(ZXIC_UINT32 dev_id,
                                       DPP_PBU_MC_COS_PARA_T *p_para);
DPP_STATUS dpp_pbu_sa_ip_en_set(ZXIC_UINT32 dev_id,
                                    ZXIC_UINT32 en);
DPP_STATUS dpp_pbu_sa_ip_en_get(ZXIC_UINT32 dev_id,
                                    ZXIC_UINT32 *p_en);
DPP_STATUS dpp_pbu_cnt_ovfl_mode_set(ZXIC_UINT32 dev_id,
                                         ZXIC_UINT32 mode);
DPP_STATUS dpp_pbu_cnt_ovfl_mode_get(ZXIC_UINT32 dev_id,
                                         ZXIC_UINT32 *p_mode);
DPP_STATUS dpp_pbu_cnt_rdclr_mode_set(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 mode);
DPP_STATUS dpp_pbu_cnt_rdclr_mode_get(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 *p_mode);
DPP_STATUS dpp_pbu_mc_diff_th_set(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 th);
DPP_STATUS dpp_pbu_mc_diff_th_get(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 *p_th);
DPP_STATUS dpp_pbu_peak_port_cnt_clr_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 port_no, ZXIC_UINT32 peak_port_cnt_clr);
DPP_STATUS dpp_pbu_peak_port_cnt_clr_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 port_no, ZXIC_UINT32* p_peak_port_cnt_clr);
DPP_STATUS dpp_pbu_all_ftm_crdt_th_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 port_no, DPP_NPPU_PBU_CFG_ALL_FTM_CRDT_TH_T* p_all_ftm_crdt_th);
DPP_STATUS dpp_pbu_all_ftm_crdt_th_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 port_no, DPP_NPPU_PBU_CFG_ALL_FTM_CRDT_TH_T* p_all_ftm_crdt_th);
DPP_STATUS dpp_pbu_all_ftm_link_th_set(ZXIC_UINT32 dev_id,
                                       DPP_PBU_ALL_FTM_LINK_TH_T *p_para);
DPP_STATUS dpp_pbu_all_ftm_link_th_get(ZXIC_UINT32 dev_id,
                                       DPP_PBU_ALL_FTM_LINK_TH_T *p_para);
DPP_STATUS dpp_pbu_ftm_total_congest_th_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 total_congest_th);
DPP_STATUS dpp_pbu_ftm_total_congest_th_get(ZXIC_UINT32 dev_id, ZXIC_UINT32* total_congest_th);
DPP_STATUS dpp_pbu_crdt_mode_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 crdt_mode);
DPP_STATUS dpp_pbu_crdt_mode_get(ZXIC_UINT32 dev_id, ZXIC_UINT32* p_crdt_mode);
DPP_STATUS dpp_pbu_ind_reg_mode_status_check(ZXIC_UINT32 dev_id, ZXIC_UINT32 sub_module_ind_status_reg, ZXIC_UINT32 sleep_time);
DPP_STATUS dpp_pbu_ind_cmd_reg_set(ZXIC_UINT32 dev_id,
                                       ZXIC_UINT32 mem_addr,
                                       ZXIC_UINT32 mem_id,
                                       ZXIC_UINT32 wrt_rd_flag);
DPP_STATUS dpp_pbu_ind_data_reg_set(ZXIC_UINT32 dev_id,
                                        ZXIC_UINT32 len,
                                        ZXIC_UINT32 *p_data,
                                        ZXIC_UINT32 data_reg_base);
DPP_STATUS dpp_pbu_ind_data_reg_get(ZXIC_UINT32 dev_id,
                                        ZXIC_UINT32 len,
                                        ZXIC_UINT32 *p_data,
                                        ZXIC_UINT32 data_reg_base);
DPP_STATUS dpp_pbu_cfg_ind_wrt(ZXIC_UINT32 dev_id,
                               ZXIC_UINT32 mem_addr,
                               DPP_PBU_CFG_IND_MEM_ID_E mem_id,
                               ZXIC_UINT32 len,
                               ZXIC_UINT32 *p_data);
DPP_STATUS dpp_pbu_cfg_ind_rd(ZXIC_UINT32 dev_id,
                              ZXIC_UINT32 mem_addr,
                              DPP_PBU_CFG_IND_MEM_ID_E mem_id,
                              ZXIC_UINT32 len,
                              ZXIC_UINT32 *p_data);
#endif                              
DPP_STATUS dpp_pbu_port_th_get(DPP_DEV_T *dev,
                               ZXIC_UINT32 port_id,
                               DPP_PBU_PORT_TH_PARA_T *p_para);
DPP_STATUS dpp_pbu_port_cos_th_get(DPP_DEV_T *dev,
                                   ZXIC_UINT32 port_id,
                                   DPP_PBU_PORT_COS_TH_PARA_T *p_para);
DPP_STATUS dpp_pbu_pfc_delay_time_set(DPP_DEV_T *dev, ZXIC_UINT64 delayTime);
DPP_STATUS dpp_pbu_pfc_delay_time_get(DPP_DEV_T *dev, ZXIC_UINT64 *delayTime);
#if 0
DPP_STATUS dpp_pbu_cnt_clr_all(ZXIC_UINT32 dev_id);
DPP_STATUS dpp_pbu_int_flag_prt(ZXIC_UINT32 dev_id);
DPP_STATUS dpp_pbu_stat_cnt_para_get(ZXIC_UINT32 dev_id,
                                     DPP_PBU_CNT_PARA_T *p_para);
DPP_STATUS dpp_pbu_stat_thram_init_done_check(ZXIC_UINT32 dev_id,
                                              ZXIC_UINT32 *p_rdy);
DPP_STATUS dpp_pbu_stat_fptr_init_done_check(ZXIC_UINT32 dev_id,
                                             ZXIC_UINT32 *p_rdy);
DPP_STATUS dpp_pbu_fc_rdy_get(ZXIC_UINT32 dev_id,
                              DPP_NPPU_PBU_STAT_PBU_FC_RDY_T* p_pbu_module_fc);
DPP_STATUS dpp_pbu_lif_fc_rdy_get(ZXIC_UINT32 dev_id,
                              DPP_NPPU_PBU_STAT_PBU_LIF_GROUP0_RDY0_T* p_pbu_group0_rdy0);
DPP_STATUS dpp_pbu_lif_pfc_rdy_get(ZXIC_UINT32 dev_id, DPP_PBU_LIF_GROUP_PFC_RDY *p_lif_group0_pfc_rdy);   
DPP_STATUS dpp_pbu_pktrx_mr_pfc_get(ZXIC_UINT32 dev_id, ZXIC_UINT32 *p_pbu_pktrx_mr_pfc);                                    
DPP_STATUS dpp_pbu_stat_reg_mode_status_check(ZXIC_UINT32 dev_id, ZXIC_UINT32 sub_module_ind_status_reg, ZXIC_UINT32 sleep_time);
DPP_STATUS dpp_pbu_stat_ind_cmd_reg_set(ZXIC_UINT32 dev_id,
                                        ZXIC_UINT32 mem_addr,
                                        ZXIC_UINT32 mem_id,
                                        ZXIC_UINT32 wrt_rd_flag);
DPP_STATUS dpp_pbu_stat_ind_data_reg_set(ZXIC_UINT32 dev_id,
                                         ZXIC_UINT32 len,
                                         ZXIC_UINT32 *p_data);
DPP_STATUS dpp_pbu_stat_ind_data_reg_get(ZXIC_UINT32 dev_id,
                                         ZXIC_UINT32 len,
                                         ZXIC_UINT32 *p_data);
DPP_STATUS dpp_pbu_stat_ind_wrt(ZXIC_UINT32 dev_id,
                                ZXIC_UINT32 mem_addr,
                                DPP_PBU_STAT_IND_MEM_ID_E mem_id,
                                ZXIC_UINT32 len,
                                ZXIC_UINT32 *p_data);
DPP_STATUS dpp_pbu_stat_ind_rd(ZXIC_UINT32 dev_id,
                               ZXIC_UINT32 mem_addr,
                               DPP_PBU_STAT_IND_MEM_ID_E mem_id,
                               ZXIC_UINT32 len,
                               ZXIC_UINT32 *p_data);
DPP_STATUS dpp_pbu_stat_port_ptr_cnt_get(ZXIC_UINT32 dev_id,
                                         ZXIC_UINT32 port_id,
                                         DPP_PBU_PORT_PTR_CNT_T *p_port_ptr_cnt);
DPP_STATUS dpp_pbu_stat_ifb_req_vld_cnt_get(ZXIC_UINT32 dev_id,
                                            ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_pbu_stat_ifb_rsp_vld_cnt_get(ZXIC_UINT32 dev_id,
                                            ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_pbu_stat_odma_recy_ptr_cnt_get(ZXIC_UINT32 dev_id,
                                              ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_pbu_stat_mcode_pf_req_cnt_get(ZXIC_UINT32 dev_id,
                                             ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_pbu_stat_mcode_pf_rsp_cnt_get(ZXIC_UINT32 dev_id,
                                             ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_pbu_stat_logic_pf_req_cnt_get(ZXIC_UINT32 dev_id,
                                             ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_pbu_stat_logic_pf_rsp_cnt_get(ZXIC_UINT32 dev_id,
                                             ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_pbu_stat_ppu_use_ptr_pulse_cnt_get(ZXIC_UINT32 dev_id,
                                                  ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_pbu_stat_ppu_wb_vld_cnt_get(ZXIC_UINT32 dev_id,
                                           ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_pbu_stat_ppu_reorder_para_cnt_get(ZXIC_UINT32 dev_id,
                                                 ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_pbu_stat_se_dpi_key_vld_cnt_get(ZXIC_UINT32 dev_id,
                                               ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_pbu_stat_se_dpi_rsp_vld_cnt_get(ZXIC_UINT32 dev_id,
                                               ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_pbu_stat_odma_ifb_rd1_cnt_get(ZXIC_UINT32 dev_id,
                                             ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_pbu_stat_odma_ifb_rd2_cnt_get(ZXIC_UINT32 dev_id,
                                             ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_pbu_stat_mcode_pf_no_rsp_cnt_get(ZXIC_UINT32 dev_id,
                                                ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_pbu_stat_logic_pf_no_rsp_cnt_get(ZXIC_UINT32 dev_id,
                                                ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_pbu_stat_ifb_data_get(ZXIC_UINT32 dev_id,
                                     DPP_PBU_IFB_DATA_T *p_data);
DPP_STATUS dpp_pbu_stat_port_public_ptr_cnt_get(ZXIC_UINT32 dev_id,
                                                ZXIC_UINT32 port_no,
                                                ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_idma_cfg_cnt_ovfl_mode_set(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 mode);
DPP_STATUS dpp_idma_cfg_cnt_ovfl_mode_get(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 *p_mode);
DPP_STATUS dpp_idma_cfg_cnt_rdclr_mode_set(ZXIC_UINT32 dev_id,
                                           ZXIC_UINT32 mode);
DPP_STATUS dpp_idma_cfg_cnt_rdclr_mode_get(ZXIC_UINT32 dev_id,
                                           ZXIC_UINT32 *p_mode);
DPP_STATUS dpp_idma_stat_ind_reg_mode_status_check(ZXIC_UINT32 dev_id, ZXIC_UINT32 sub_module_ind_status_reg, ZXIC_UINT32 sleep_time);
DPP_STATUS dpp_idma_stat_ind_cmd_reg_set(ZXIC_UINT32 dev_id,
                                         ZXIC_UINT32 mem_addr,
                                         ZXIC_UINT32 mem_id,
                                         ZXIC_UINT32 wrt_rd_flag);
DPP_STATUS dpp_idma_stat_ind_data_reg_set(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 *p_data);
DPP_STATUS dpp_idma_stat_ind_data_reg_get(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 *p_data);
DPP_STATUS dpp_idma_stat_ind_wrt(ZXIC_UINT32 dev_id,
                                 ZXIC_UINT32 mem_addr,
                                 DPP_IDMA_STAT_IND_MEM_ID_E mem_id,
                                 ZXIC_UINT32 *p_data);
DPP_STATUS dpp_idma_stat_ind_rd(ZXIC_UINT32 dev_id,
                                ZXIC_UINT32 mem_addr,
                                DPP_IDMA_STAT_IND_MEM_ID_E mem_id,
                                ZXIC_UINT32 *p_data);
DPP_STATUS dpp_idma_stat_to_isu_total_cnt_get(ZXIC_UINT32 dev_id,
                                              ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_idma_stat_to_isu_err_total_cnt_get(ZXIC_UINT32 dev_id,
                                                  ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_idma_stat_disc_total_cnt_get(ZXIC_UINT32 dev_id,
                                            ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_idma_stat_port_to_isu_cnt_get(ZXIC_UINT32 dev_id,
                                             ZXIC_UINT32 port_no,
                                             ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_idma_stat_port_to_isu_err_cnt_get(ZXIC_UINT32 dev_id,
                                                 ZXIC_UINT32 port_no,
                                                 ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_idma_stat_port_disc_cnt_get(ZXIC_UINT32 dev_id,
                                           ZXIC_UINT32 port_no,
                                           ZXIC_UINT32 *p_cnt);
DPP_STATUS dpp_pbu_cap_data_mode_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 mod_data);
DPP_STATUS dpp_pbu_pkt_capture_start(ZXIC_UINT32 dev_id, ZXIC_UINT32 pkt_num);
DPP_STATUS dpp_pbu_pkt_capture_stop(ZXIC_UINT32 dev_id);
DPP_STATUS dpp_pbu_pkt_capture_mode_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 mode);
DPP_STATUS dpp_pbu_pkt_capture_cnt(ZXIC_UINT32 dev_id, ZXIC_UINT32* pkt_num);
DPP_STATUS dpp_pbu_pkt_capture_dbg_print(ZXIC_UINT32 dev_id);
DPP_STATUS dpp_pbu_pkt_capture_cnt_print(ZXIC_UINT32 dev_id);
DPP_STATUS dpp_pbu_pkt_capture_print_one(ZXIC_UINT32 dev_id, ZXIC_UINT32 pkt_no);
DPP_STATUS dpp_pbu_pkt_capture_print(ZXIC_UINT32 dev_id,
                                     ZXIC_UINT32 pkt_start_no,
                                     ZXIC_UINT32 pkt_end_no);
DPP_STATUS dpp_pbu_pkt_capture_print_all(ZXIC_UINT32 dev_id);
DPP_STATUS dpp_pbu_pkt_capture_filter_set_bit(ZXIC_UINT32 dev_id,
                                              ZXIC_UINT32 bit_no,
                                              ZXIC_UINT32 filter_mode
                                             );
DPP_STATUS dpp_pbu_pkt_capture_filter_set(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 bit_start_no,
                                          ZXIC_UINT32 bit_end_no,
                                          ZXIC_UINT32 filter_mode);
DPP_STATUS dpp_pbu_pkt_capture_filter_mask_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 start, ZXIC_UINT32 end, ZXIC_UINT32 mask, ZXIC_UINT32 data);
DPP_STATUS dpp_pbu_pkt_capture_filter_pkt_mask_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 start, ZXIC_UINT32 end, ZXIC_UINT32 mask, ZXIC_UINT32 data);
DPP_STATUS dpp_pbu_pkt_capture_filter_sport_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 port);
DPP_STATUS dpp_pbu_pkt_capture_filter_dport_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 port);
DPP_STATUS dpp_pbu_pkt_capture_filter_clr_bit(ZXIC_UINT32 dev_id,
                                              ZXIC_UINT32 bit_no);
DPP_STATUS dpp_pbu_pkt_capture_filter_clr(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 bit_start_no,
                                          ZXIC_UINT32 bit_end_no);
DPP_STATUS dpp_pbu_pkt_capture_filter_clr_all(ZXIC_UINT32 dev_id);
/***********************************************************/
/** 抓包报文分片打印by mf bit
* @param   dev_id    芯片ID
* @param   pkt_no    要打印的分片号
*
* @return
* @remark  无
* @see
* @author  czd      @date  2015/04/29
************************************************************/
DPP_STATUS dpp_pbu_pkt_capture_print_one_mf_by_bit(ZXIC_UINT32 dev_id, ZXIC_UINT32 pkt_no);

DPP_STATUS dpp_pbu_glb_mgr_get(ZXIC_UINT32 dev_id,
                               ZXIC_UINT32 *p_flag,
                               ZXIC_UINT32 *p_size,
                               ZXIC_UINT8 **pp_data_buff);
DPP_STATUS dpp_pbu_glb_mgr_set(ZXIC_UINT32 dev_id,
                               ZXIC_UINT32 size,
                               ZXIC_UINT8 *p_data_buff);
DPP_STATUS dpp_pbu_glb_size_get(ZXIC_UINT32 dev_id,
                                ZXIC_UINT32 *p_size);

ZXIC_UINT32 sim_soc_mf_field_print(ZXIC_UINT8 *pkt_buff);


DPP_STATUS dpp_pbu_pkt_capture_print_mf_by_bit(ZXIC_UINT32 dev_id,
                                     ZXIC_UINT32 pkt_start_no,
                                     ZXIC_UINT32 pkt_end_no);
DPP_STATUS dpp_pbu_pkt_capture_print_one_simple_mf_by_bit(ZXIC_UINT32 dev_id, ZXIC_UINT32 pkt_no);

DPP_STATUS dpp_pbu_pkt_capture_print_all_mf_by_bit(ZXIC_UINT32 dev_id);

#endif

/******************************************************************************
 *                             END: 函数声明                              *
 *****************************************************************************/
#endif

#endif /* _DPP_PBU_H_ */
/* 必须有个空行，否则可能编不过 */




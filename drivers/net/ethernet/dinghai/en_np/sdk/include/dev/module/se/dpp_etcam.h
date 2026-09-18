/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_etcam.h
* 文件标识 : 
* 内容摘要 : 
* 其它说明 : 
* 当前版本 : 
* 完成日期 : 2014/04/03
* DEPARTMENT: ASIC_FPGA_R&D_Dept 
* MANUAL_PERCENT: 100%   
 
* 修改记录1: 
* 修改日期:  
* 版 本 号:  
* 修 改 人:  
* 修改内容:  
***************************************************************/

#ifndef _DPP_ETCAM_H_
#define _DPP_ETCAM_H_

#ifdef __cplusplus
extern "C"{
#endif

#define DPP_ETCAM_BLOCK_NUM           (8)   /* eTcam中Block的数目 16:dpp+的etcam block 数目 */
#define DPP_ETCAM_TBLID_NUM           (8)    /* eTcam支持的业务表号 */
#define DPP_ETCAM_RAM_NUM             (8)    /* Block内部RAM的个数 */
#define DPP_ETCAM_RAM_WIDTH           (80U)   /* Block内部单个RAM的宽度，比特为单位 */

#ifdef DPP_FPGA_TEST_BAORD_SA500FT
#define DPP_ETCAM_RAM_DEPTH           (16U)   /* Block内部单个RAM的深度 */
#else
#define DPP_ETCAM_RAM_DEPTH           (512U)  /* Block内部单个RAM的深度 */
#endif

#define DPP_ETCAM_WR_MASK_MAX         (((ZXIC_UINT32)1<<DPP_ETCAM_RAM_NUM)-1)/*255*/
#define DPP_ETCAM_WIDTH_MIN           (DPP_ETCAM_RAM_WIDTH)                     /* eTcam最小数据位宽，比特为单位 */
#define DPP_ETCAM_WIDTH_MAX           (DPP_ETCAM_RAM_NUM * DPP_ETCAM_RAM_WIDTH) /* eTcam最大数据位宽，比特为单位 */

#define DPP_ETCAM_DEFAULT_MIN         (0)
#define DPP_ETCAM_ONE_BIT_MAX         (1)

#define DPP_ETCAM_PORT_NUM            (1)

typedef enum dpp_etcam_data_type_e
{
    DPP_ETCAM_DTYPE_MASK = 0,
    DPP_ETCAM_DTYPE_DATA = 1,
}DPP_ETCAM_DATA_TYPE_E;

/** etcam 条目vld信息  */
typedef struct dpp_etcam_entry_vld_t
{
    ZXIC_UINT8 vld;           /** <@brief 标示每个条目的使用状态，每bit为指示当前条目是否被占用 */
    ZXIC_UINT8 rsv[3];        /** <@brief 数据对齐，勿需关心 */
}DPP_ETCAM_ENTRY_VLD_T;

/* error code */
#define DPP_STAT_ETCAM_RC_BASE         (0x6000)
#define DPP_ETCAM_RC_INVALID_PARA      (DPP_STAT_ETCAM_RC_BASE | 0x0)

/* macro function */
#define DPP_ETCAM_ENTRY_SIZE_GET(entry_mode) \
    (((ZXIC_UINT32)DPP_ETCAM_RAM_WIDTH << (3 - entry_mode)) / 8)

/* api */
DPP_STATUS dpp_etcam_dm_to_xy(DPP_ETCAM_ENTRY_T *p_dm, DPP_ETCAM_ENTRY_T *p_xy, ZXIC_UINT32 len);

DPP_STATUS dpp_etcam_xy_to_dm(DPP_ETCAM_ENTRY_T *p_dm, DPP_ETCAM_ENTRY_T *p_xy, ZXIC_UINT32 len);

DPP_STATUS dpp_etcam_block_tbl_id_set(DPP_DEV_T *dev, ZXIC_UINT32 block_idx, ZXIC_UINT32 tbl_id);

DPP_STATUS dpp_etcam_block_tbl_id_get(DPP_DEV_T *dev, ZXIC_UINT32 block_idx, ZXIC_UINT32 *p_tbl_id);

DPP_STATUS dpp_etcam_block_baddr_set(DPP_DEV_T *dev, ZXIC_UINT32 block_idx, ZXIC_UINT32 base_addr);

DPP_STATUS dpp_etcam_block_baddr_get(DPP_DEV_T *dev, ZXIC_UINT32 block_idx, ZXIC_UINT32 *p_base_addr);

DPP_STATUS dpp_etcam_cpu_afull_get(DPP_DEV_T *dev, ZXIC_UINT32 block_idx, ZXIC_UINT32 *p_cpu_afull);

DPP_STATUS dpp_etcam_ind_cmd_set(DPP_DEV_T *dev,
                                 ZXIC_UINT32 addr,
                                 ZXIC_UINT32 block_idx,
                                 ZXIC_UINT32 data_or_mask,
                                 ZXIC_UINT32 wr_mask,
                                 ZXIC_UINT32 opr_type,
                                 ZXIC_UINT32 tacm_reg_flag,
                                 ZXIC_UINT32 row_mask_flag,
                                 ZXIC_UINT32 vben,
                                 ZXIC_UINT32 vbit);

/***********************************************************/
/** 添加eTcam表条目
* @param   dev_id     设备号
* @param   addr       每个block中的ram地址，位宽为8*80bit
* @param   block_idx  block编号，范围0~15
* @param   wr_mask    写表掩码，共8bit，每bit控制ram中对应位置的80bit数据是否有效
* @param   p_entry    条目数据，data和mask
*
* @return
* @remark  无
* @see
* @author  wcl      @date  2014/04/03
************************************************************/
DPP_STATUS dpp_etcam_entry_add(DPP_DEV_T *dev,
                               ZXIC_UINT32 addr,
                               ZXIC_UINT32 block_idx,
                               ZXIC_UINT32 wr_mask,
                               ZXIC_UINT32 opr_type,
                               DPP_ETCAM_ENTRY_T *p_entry);

/***********************************************************/
/** 删除eTcam表项条目
* @param   dev_id     设备号
* @param   addr       每个block中的ram地址，位宽为8*80bit
* @param   block_idx  block的编号，范围0~15
* @param   wr_mask    写表掩码，共8bit，每bit控制ram中对应位置的80bit数据是否有效
*
* @return
* @remark  无
* @see
* @author  wcl      @date  2014/04/03
************************************************************/
DPP_STATUS dpp_etcam_entry_del(DPP_DEV_T *dev,
                               ZXIC_UINT32 addr,
                               ZXIC_UINT32 block_idx,
                               ZXIC_UINT32 wr_mask);

ZXIC_UINT32 dpp_etcam_entry_cmp(DPP_ETCAM_ENTRY_T *p_entry_dm, DPP_ETCAM_ENTRY_T *p_entry_xy);

ZXIC_UINT32 dpp_etcam_ind_data_reg_opr_mask_get(ZXIC_UINT32 mask);

#if ZXIC_REAL("调试计数")

typedef struct dpp_etcam_port_cnt
{
    ZXIC_UINT32 as_etcam_req_cnt;
    ZXIC_UINT32 etcam_as_index_cnt;
    ZXIC_UINT32 etcam_not_hit_cnt;
}DPP_ETCAM_PORT_CNT_T;

typedef struct dpp_etcam_dbg_cnt
{
    DPP_ETCAM_PORT_CNT_T dpp_etcam_port_cnt[DPP_ETCAM_PORT_NUM];
    ZXIC_UINT32 table_id_not_match_cnt;
    ZXIC_UINT32 table_id_clash01_cnt;
}DPP_ETCAM_DBG_CNT_T;

#endif

#ifdef __cplusplus
}
#endif

#endif

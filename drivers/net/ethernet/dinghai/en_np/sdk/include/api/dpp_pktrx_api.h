/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_pbu_api.h
* 文件标识 : pbu模块对外数据类型定义和接口函数声明
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : djf
* 完成日期 : 2015/02/04
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/

#ifndef _DPP_PKTRX_API_H_
#define _DPP_PKTRX_API_H_

#include "zxic_common.h"
#include "dpp_module.h"
#include "dpp_reg.h"

#define PKTRX_IND_CMD_WRT_FLAG            (0)    /** 写命令 */
#define PKTRX_IND_CMD_RD_FLAG             (1)    /** 读命令 */

#define TRPG_EXT_PORT_NUM_BEGIN                 (0U)
#define TRPG_EXT_PORT_NUM_END                   (9U)

#define DPP_MCODE_FEATURE_LIST_NUM        (6U)
#define DPP_PKTRX_ICU_TCAM_NUM            (512)
#define STARTPC_MASK                      (0x7FFF)

#define DPP_PKTRX_TCAM_INDEX_GET(index)\
  (((index)>255)?((index)-256):((index)))

#define DPP_PKTRX_TCAM_MEMID_GET(index)\
 (((index) > 255)?(TCAM_MEM_ID_1):(TCAM_MEM_ID_0))

#define DPP_PKTRX_TCAM_RESULT_MEMID_GET(index)\
 (((index) > 255)?(TCAM_RESULT_MEM_ID_1):(TCAM_RESULT_MEM_ID_0))

#define DPP_PKTRX_TCAM_REGID_GET(index)\
 (((index) > 255)?(NPPU_PKTRX_CFG_TCAM_1_VLDr):(NPPU_PKTRX_CFG_TCAM_0_VLDr))

typedef enum dpp_pktrx_table_mem_id_e
{
    PHYPORT_TAB_0_MEM_ID = 0,      /** 物理端口属性表0 */
    PHYPORT_TAB_1_MEM_ID = 1,      /** 物理端口属性表1 */
    PHYPORT_TAB_2_MEM_ID = 2,      /** 物理端口属性表2 */
    TCAM_MEM_ID_0 = 3,        /** FLOWTCAM表0 */
    TCAM_MEM_ID_1 = 4,        /** FLOWTCAM表1 */
    TCAM_RESULT_MEM_ID_0 = 5, /** FLOWTCAM结果表0 */
    TCAM_RESULT_MEM_ID_1 = 6, /** FLOWTCAM结果表1 */
    PKT_CAPTURE_MEM_ID = 7,        /** 抓包 */
    MEM_ID_MUX_NUM = 8,            /** PKTRX模块使用的内部表的个数*/
} DPP_PKTRX_TBL_MEM_ID_E;

typedef struct dpp_pktrx_phyport_udf_table_t
{
    ZXIC_UINT32 port_based_user_data[4];     /**< @brief 用户自定义表数据 */
}DPP_PKTRX_PHYPORT_UDF_TABLE_T;

typedef struct dpp_pktrx_tcam_dt_table_t
{

    ZXIC_UINT32 tcam_key_mode;                /**< @brief 查表类型 1b  0:cos 1:start_pc+flownum */
    ZXIC_UINT32 tcam_key_mask_mode;
    ZXIC_UINT32 tcam_key_port_num;            /**<  @brief  端口类型 7b*/
    ZXIC_UINT32 tcam_key_mask_port_num;            
    ZXIC_UINT32 tcam_key_dmac_h24;               /**< @brief 目的mac高24bit */
    ZXIC_UINT32 tcam_key_mask_dmac_h24;
    ZXIC_UINT32 tcam_key_dmac_l24;                /**< @brief 目的mac低24bit */
    ZXIC_UINT32 tcam_key_mask_dmac_l24;
    ZXIC_UINT32 tcam_key_l3type;                /**< @brief l3_类型字段 16b*/
    ZXIC_UINT32 tcam_key_mask_l3type;
    ZXIC_UINT32 tcam_key_priority;                  /**< @brief priority 3b*/
    ZXIC_UINT32 tcam_key_mask_priority;
    ZXIC_UINT32 tcam_key_cfi;                  /**< @brief cfi 1b */
    ZXIC_UINT32 tcam_key_mask_cfi;
    ZXIC_UINT32 tcam_key_ex_vlanid;                  /**< @brief ex_vlanid 12b */
    ZXIC_UINT32 tcam_key_mask_ex_vlanid;
    ZXIC_UINT32 tcam_key_udf_h8;                  /**< @brief udf高8bit(udf8) */
    ZXIC_UINT32 tcam_key_mask_udf_h8;
    ZXIC_UINT32 tcam_key_udf_m32;         /**< @brief udf中间32bit(udf7 udf6 udf5 udf4) */
    ZXIC_UINT32 tcam_key_mask_udf_m32;               
    ZXIC_UINT32 tcam_key_udf_l32;         /**< @brief udf低32bit(udf3 udf2 udf1 udf0) */
    ZXIC_UINT32 tcam_key_mask_udf_l32;          

    ZXIC_UINT32 tcam_result_flownum;         /**< @brief 结果表flownum  8b*/
    ZXIC_UINT32 tcam_result_vld;                 /**< @brief 结果表vld 1b*/
    ZXIC_UINT32 tcam_result_table_type;         /**< @brief 结果表table_type 1b 0:pc  1:cos  */
    ZXIC_UINT32 tcam_result_pc_or_cos;      /**< @brief 结果表start_pc 14b 或者 cos 3b 由table_type决定*/

}DPP_PKTRX_TCAM_DT_TABLE_T;

/***********************************************************/
/**全局配置寄存器设置
* @param   dev_id
* @param   p_mcode_glb_cfg
*
* @return
* @remark  无
* @see
* @author  czd      @date  2016/04/27
************************************************************/
ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_set_0(DPP_DEV_T *dev, ZXIC_UINT32 glb_cfg_data_0);
ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_set_1(DPP_DEV_T *dev, ZXIC_UINT32 glb_cfg_data_1);
ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_set_2(DPP_DEV_T *dev, ZXIC_UINT32 glb_cfg_data_2);
ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_set_3(DPP_DEV_T *dev, ZXIC_UINT32 glb_cfg_data_3);
ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_get_0(DPP_DEV_T *dev, ZXIC_UINT32 *p_glb_cfg_data_0);
ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_get_1(DPP_DEV_T *dev, ZXIC_UINT32 *p_glb_cfg_data_1);
ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_get_2(DPP_DEV_T *dev, ZXIC_UINT32 *p_glb_cfg_data_2);
ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_get_3(DPP_DEV_T *dev, ZXIC_UINT32 *p_glb_cfg_data_3);
ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_write_0(DPP_DEV_T *dev, ZXIC_UINT32 start_bit_no, ZXIC_UINT32 end_bit_no,
                                            ZXIC_UINT32 glb_cfg_data_0);
ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_write_1(DPP_DEV_T *dev, ZXIC_UINT32 start_bit_no, ZXIC_UINT32 end_bit_no,
                                            ZXIC_UINT32 glb_cfg_data_1);
DPP_STATUS dpp_pktrx_udf_table_get(DPP_DEV_T *dev, ZXIC_UINT32 index, DPP_PKTRX_PHYPORT_UDF_TABLE_T *p_phyport_user_info);
DPP_STATUS dpp_pktrx_udf_table_set(DPP_DEV_T *dev, ZXIC_UINT32 index, DPP_PKTRX_PHYPORT_UDF_TABLE_T *p_phyport_user_info);
DPP_STATUS dpp_pktrx_tcam_table_set(DPP_DEV_T *dev, ZXIC_UINT32 index, DPP_PKTRX_TCAM_DT_TABLE_T *p_tcam_info);
DPP_STATUS dpp_pktrx_tcam_item_enable(DPP_DEV_T *dev, ZXIC_UINT32 index);
#endif
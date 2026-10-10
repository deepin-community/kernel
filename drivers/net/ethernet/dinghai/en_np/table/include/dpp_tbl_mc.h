/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_tbl_mc.h
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 :
* 完成日期 : 2014/01/27
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/

#ifndef DPP_TBL_MC_H
#define DPP_TBL_MC_H

#include "zxic_common.h"
#include "dpp_type_api.h"

#define MC_TABLE_SIZE           (1028)
#define MC_GROUP_NUM            (4)
#define MC_MEMBER_NUM_IN_GROUP  (64)

typedef struct dpp_vport_mc_info_t
{
    ZXIC_UINT32 is_valid;
    ZXIC_UINT8  mac[6];
    ZXIC_UINT32 mc_pf_enable;
    ZXIC_UINT64 mc_bitmap[MC_GROUP_NUM];
} DPP_VPORT_MC_INFO_T;

typedef struct dpp_vport_mc_table_t
{
    DPP_VPORT_MC_INFO_T *mc_info;
} DPP_VPORT_MC_TABLE_T;

#endif

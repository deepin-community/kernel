/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_tbl_tm.h
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

#ifndef DPP_TBL_TM_H
#define DPP_TBL_TM_H

#include "zxic_common.h"
#include "dpp_type_api.h"
#include "dpp_dev.h"

#define TM_BASE_QUEUE_VALID (0x1000)
#define TRUST_MODE_VALID    (0x10)
#define UP_VALID            (0x10)
#define TC_VALID            (0x10)
#define TM_SWITCH_ON        (1)
#define TM_SWITCH_OFF        (0)

ZXIC_UINT32 dpp_tm_flowid_pport_table_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 port, ZXIC_UINT32 flow_id);
ZXIC_UINT32 dpp_tm_flowid_pport_table_del(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 port);
ZXIC_UINT32 dpp_tm_pport_trust_mode_table_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 port, ZXIC_UINT32 mode);
ZXIC_UINT32 dpp_tm_pport_trust_mode_table_del(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 port);
ZXIC_UINT32 dpp_tm_pport_dscp_map_table_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 port, ZXIC_UINT32 dscp_id, ZXIC_UINT32 up_id);
ZXIC_UINT32 dpp_tm_pport_dscp_map_table_del(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 port, ZXIC_UINT32 dscp_id);
ZXIC_UINT32 dpp_tm_pport_up_map_table_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 port, ZXIC_UINT32 up_id, ZXIC_UINT32 tc_id);
ZXIC_UINT32 dpp_tm_pport_up_map_table_del(DPP_PF_INFO_T* pf_info, ZXIC_UINT32  port, ZXIC_UINT32 up_id);
ZXIC_UINT32 dpp_tm_pport_mcode_switch_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 port, ZXIC_UINT32 mode);
ZXIC_UINT32 dpp_tm_pport_mcode_switch_del(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 port);

#endif
/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_tbl_comm.h
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

#ifndef DPP_TBL_COMM_H
#define DPP_TBL_COMM_H

#include "zxic_common.h"
#include "dpp_type_api.h"
#include "dpp_tbl_mc.h"
#include "dpp_tbl_mac.h"
#include "dpp_tbl_qid.h"
#include "dpp_tbl_port.h"
#include "dpp_tbl_bc.h"
#include "dpp_tbl_promisc.h"

#define VF_ACTIVE(VPORT)            ((VPORT & 0x0800) >> 11)
#define EPID(VPORT)                 ((VPORT & 0x7000) >> 12)
#define FUNC_NUM(VPORT)             ((VPORT & 0x0700) >> 8) 
#define VFUNC_NUM(VPORT)            ((VPORT & 0x00FF)) 

#define PF_VQM_VFID_OFFSET          (1152)
#define IS_PF(VPORT)                (!VF_ACTIVE(VPORT))
#define VQM_VFID(VPORT)             (IS_PF(VPORT) ? \
                                    (PF_VQM_VFID_OFFSET + (EPID(VPORT) * 8) + FUNC_NUM(VPORT)) : \
                                    ((EPID(VPORT) * 256) + VFUNC_NUM(VPORT)))

#define OWNER_PF_VQM_VFID(VPORT)    (PF_VQM_VFID_OFFSET + (EPID(VPORT) * 8) + FUNC_NUM(VPORT))
#define OWNER_PF_VPORT(VPORT)       (((EPID(VPORT)) << 12) | ((FUNC_NUM(VPORT)) << 8))

#define VQM_VFID_MAX_NUM            (2048)

typedef struct dpp_vport_mgr_t
{
    DPP_VPORT_BC_TABLE_T        bc_table;
    DPP_VPORT_MC_TABLE_T        mc_table;
    DPP_VPORT_PROMISC_TABLE_T   uc_promisc_table;
    DPP_VPORT_PROMISC_TABLE_T   mc_promisc_table;
    ZXIC_MUTEX_T*               table_lock[DPP_DEV_SDT_ID_MAX];
} DPP_VPORT_MGR_T;

typedef struct
{
    uint8_t addr[6];
    uint16_t vport;
    uint16_t sriov_vlan_tpid;
    uint16_t sriov_vlan_id;
} MAC_VPORT_INFO;

typedef struct
{
    uint8_t mc_addr[6];
    uint16_t pf_flag;
} MC_PF_FLAG_MGR;

ZXIC_UINT32 dpp_data_print(ZXIC_UINT8 *data, ZXIC_UINT32 len);
ZXIC_UINT32 dpp_vport_attr_value_show(ZXIC_VOID);
ZXIC_UINT32 dpp_vport_mgr_init(DPP_PF_INFO_T* pf_info);
ZXIC_UINT32 dpp_vport_mgr_release(DPP_PF_INFO_T* pf_info);
ZXIC_UINT32 dpp_vport_table_lock(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_MUTEX_T** table_lock);
ZXIC_UINT32 dpp_vport_table_unlock(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no);
ZXIC_UINT32 dpp_vport_bc_table_get(DPP_PF_INFO_T* pf_info, DPP_VPORT_BC_TABLE_T** bc_table);
ZXIC_UINT32 dpp_vport_mc_table_get(DPP_PF_INFO_T* pf_info, DPP_VPORT_MC_TABLE_T** mc_table);
ZXIC_UINT32 dpp_vport_uc_promisc_table_get(DPP_PF_INFO_T* pf_info, DPP_VPORT_PROMISC_TABLE_T** promisc_table);
ZXIC_UINT32 dpp_vport_mc_promisc_table_get(DPP_PF_INFO_T* pf_info, DPP_VPORT_PROMISC_TABLE_T** promisc_table);
ZXIC_UINT32 dpp_vport_get_by_vqm_vfid(ZXIC_UINT16 pf_vport, ZXIC_UINT32 vqm_vfid, ZXIC_UINT16* vport);
ZXIC_UINT32 dpp_vport_get_by_mc_bitmap(ZXIC_UINT16 pf_vport, ZXIC_UINT32 group_id, ZXIC_UINT64 mc_bitmap, ZXIC_UINT16 vport[64], ZXIC_UINT32* vport_num);
BOOLEAN dpp_vport_in_mc_bitmap(ZXIC_UINT32 vport,ZXIC_UINT64 mc_bitmap);
#endif

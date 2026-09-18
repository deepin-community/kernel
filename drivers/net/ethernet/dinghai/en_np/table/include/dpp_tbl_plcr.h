#ifndef DPP_TBL_PLCR_H
#define DPP_TBL_PLCR_H

#include "dpp_dev.h"
#include "dpp_tbl_comm.h"

ZXIC_UINT32 dpp_vport_egress_meter_en_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 enable);
ZXIC_UINT32 dpp_vport_ingress_meter_en_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 enable);
ZXIC_UINT32 dpp_vport_egress_meter_mode_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 mode);  //0表示模式2: 通过vport映射flow_id;1表示模式1: 通过队列映射flow_id
ZXIC_UINT32 dpp_vport_ingress_meter_mode_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 mode); //0表示模式2: 通过vport映射flow_id;1表示模式1: 通过队列映射flow_id
ZXIC_UINT32 dpp_vport_egress_meter_en_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *enable);
ZXIC_UINT32 dpp_vport_ingress_meter_en_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *enable);
ZXIC_UINT32 dpp_vport_egress_meter_mode_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *mode);
ZXIC_UINT32 dpp_vport_ingress_meter_mode_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *mode);

#endif
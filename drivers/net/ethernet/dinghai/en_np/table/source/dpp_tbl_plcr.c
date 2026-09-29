#include "dpp_tbl_plcr.h"
#include "dpp_dev.h"
#include "dpp_drv_sdt.h"
#include "dpp_drv_eram.h"
#include "dpp_dtb.h"
#include "dpp_apt_se_api.h"
#include "dpp_tbl_api.h"
     
ZXIC_UINT32 dpp_vport_egress_meter_en_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 enable)
{
    ZXIC_UINT32 rc   = DPP_OK;
    ZXIC_UINT32 attr = SRIOV_VPORT_NP_EGRESS_METER_EN_OFF;

    rc = dpp_vport_attr_set(pf_info, attr, enable & 0x1);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_attr_set");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_vport_egress_meter_en_set);

ZXIC_UINT32 dpp_vport_egress_meter_en_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *enable)
{
    ZXIC_UINT32 rc   = DPP_OK;
    ZXDH_SRIOV_VPORT_T port_attr_entry = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(enable);

    rc = dpp_vport_attr_get(pf_info, &port_attr_entry);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_attr_get");

    *enable = port_attr_entry.np_egress_meter_enable;

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x egress_meter_enable_status: %u success.\n",
                                        pf_info->slot, pf_info->vport, *enable);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_vport_egress_meter_en_get);

ZXIC_UINT32 dpp_vport_ingress_meter_en_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 enable)
{
    ZXIC_UINT32 rc   = DPP_OK;
    ZXIC_UINT32 attr = SRIOV_VPORT_NP_INGRESS_METER_EN_OFF;

    rc = dpp_vport_attr_set(pf_info, attr, enable & 0x1);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_attr_set");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_vport_ingress_meter_en_set);

ZXIC_UINT32 dpp_vport_ingress_meter_en_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *enable)
{
    ZXIC_UINT32 rc   = DPP_OK;
    ZXDH_SRIOV_VPORT_T port_attr_entry = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(enable);

    rc = dpp_vport_attr_get(pf_info, &port_attr_entry);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_attr_get");

    *enable = port_attr_entry.np_ingress_meter_enable;

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x ingress_meter_enable_status: %u success.\n",
                                        pf_info->slot, pf_info->vport, *enable);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_vport_ingress_meter_en_get);

ZXIC_UINT32 dpp_vport_egress_meter_mode_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 mode)
{
    ZXIC_UINT32 rc   = DPP_OK;
    ZXIC_UINT32 attr = SRIOV_VPORT_NP_EGRESS_MODE;

    rc = dpp_vport_attr_set(pf_info, attr, mode & 0x1);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_attr_set");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_vport_egress_meter_mode_set);

ZXIC_UINT32 dpp_vport_egress_meter_mode_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *mode)
{
    ZXIC_UINT32 rc   = DPP_OK;
    ZXDH_SRIOV_VPORT_T port_attr_entry = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(mode);

    rc = dpp_vport_attr_get(pf_info, &port_attr_entry);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_attr_get");

    *mode = port_attr_entry.np_egress_meter_mode;

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x egress_meter_mode_status: %u success.\n",
                                        pf_info->slot, pf_info->vport, *mode);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_vport_egress_meter_mode_get);

ZXIC_UINT32 dpp_vport_ingress_meter_mode_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 mode)
{
    ZXIC_UINT32 rc   = DPP_OK;
    ZXIC_UINT32 attr = SRIOV_VPORT_NP_INGRESS_MODE;

    rc = dpp_vport_attr_set(pf_info, attr, mode & 0x1);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_attr_set");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_vport_ingress_meter_mode_set);

ZXIC_UINT32 dpp_vport_ingress_meter_mode_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *mode)
{
    ZXIC_UINT32 rc   = DPP_OK;
    ZXDH_SRIOV_VPORT_T port_attr_entry = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(mode);

    rc = dpp_vport_attr_get(pf_info, &port_attr_entry);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_attr_get");

    *mode = port_attr_entry.np_ingress_meter_mode;

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x ingress_meter_mode_status: %u success.\n",
                                        pf_info->slot, pf_info->vport, *mode);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_vport_ingress_meter_mode_get);

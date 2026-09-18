#include "dpp_tbl_tm.h"
#include "dpp_dev.h"
#include "dpp_drv_sdt.h"
#include "dpp_drv_eram.h"
#include "dpp_drv_qos.h"
#include "dpp_dtb.h"
#include "dpp_apt_se_api.h"
#include "dpp_tbl_api.h"

ZXIC_UINT32 dpp_tm_flowid_pport_table_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 port, ZXIC_UINT32 flow_id)
{
    ZXIC_UINT32 rc   = DPP_OK;
    ZXIC_UINT32 attr = UPLINK_PHY_PORT_TM_BASE_QUEUE;

    rc = dpp_uplink_phy_attr_set(pf_info, port, attr, flow_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_uplink_phy_attr_set");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_tm_flowid_pport_table_set);

ZXIC_UINT32 dpp_tm_flowid_pport_table_del(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 port)
{
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_tm_flowid_pport_table_set(pf_info, port, TM_BASE_QUEUE_VALID);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tm_flowid_pport_table_set");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_tm_flowid_pport_table_del);

ZXIC_UINT32 dpp_tm_pport_trust_mode_table_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 port, ZXIC_UINT32 mode)
{
    ZXIC_UINT32 rc   = DPP_OK;
    ZXIC_UINT32 attr = UPLINK_PHY_PORT_TRUST_MODE;

    rc = dpp_uplink_phy_attr_set(pf_info, port, attr, mode);
    ZXIC_COMM_CHECK_RC(rc, "dpp_uplink_phy_attr_set");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_tm_pport_trust_mode_table_set);

ZXIC_UINT32 dpp_tm_pport_trust_mode_table_del(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 port)
{
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_tm_pport_trust_mode_table_set(pf_info, port, TRUST_MODE_VALID);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tm_pport_trust_mode_table_set");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_tm_pport_trust_mode_table_del);

ZXIC_UINT32 dpp_tm_pport_dscp_map_table_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 port, ZXIC_UINT32 dscp_id, ZXIC_UINT32 up_id)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 sdt_no = ZXDH_SDT_DSCP_TO_UP_TABLE;
    ZXIC_UINT32 index  = 0x3ff & ((port << 6) | (dscp_id & 0x3f));
    ZXIC_UINT32 rc     = DPP_OK;

    ZXDH_DSCP_TO_UP_T dscp_to_up = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u index: %u up_id: %u start.\n",
                                                    pf_info->slot, pf_info->vport, sdt_no, index, up_id);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    dscp_to_up.hit_flag    = 1;
    dscp_to_up.up = up_id;

    rc = dpp_apt_dtb_eram_insert(&dev, queue, sdt_no, index, &dscp_to_up);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_eram_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u index: %u up_id: %u success.\n",
                                                    pf_info->slot, pf_info->vport, sdt_no, index, up_id);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_tm_pport_dscp_map_table_set);

ZXIC_UINT32 dpp_tm_pport_dscp_map_table_del(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 port, ZXIC_UINT32 dscp_id)
{
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_tm_pport_dscp_map_table_set(pf_info, port, dscp_id, UP_VALID);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tm_pport_dscp_map_table_set");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_tm_pport_dscp_map_table_del);

ZXIC_UINT32 dpp_tm_pport_up_map_table_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 port, ZXIC_UINT32 up_id, ZXIC_UINT32 tc_id)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 sdt_no = ZXDH_SDT_UP_TO_TC_TABLE;
    ZXIC_UINT32 index  = 0x7F & ((port << 3) | (up_id & 0x7));
    ZXIC_UINT32 rc     = DPP_OK;

    ZXDH_UP_TO_TC_T up_to_tc = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u index: %u tc_id: %u start.\n",
                                                    pf_info->slot, pf_info->vport, sdt_no, index, tc_id);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    up_to_tc.hit_flag    = 1;
    up_to_tc.tc = tc_id;

    rc = dpp_apt_dtb_eram_insert(&dev, queue, sdt_no, index, &up_to_tc);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_eram_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u index: %u tc_id: %u success.\n",
                                                    pf_info->slot, pf_info->vport, sdt_no, index, tc_id);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_tm_pport_up_map_table_set);

ZXIC_UINT32 dpp_tm_pport_up_map_table_del(DPP_PF_INFO_T* pf_info, ZXIC_UINT32  port, ZXIC_UINT32 up_id)
{
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_tm_pport_up_map_table_set(pf_info, port, up_id, TC_VALID);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tm_pport_dscp_map_table_set");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_tm_pport_up_map_table_del);

ZXIC_UINT32 dpp_tm_pport_mcode_switch_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 port, ZXIC_UINT32 mode)
{
    ZXIC_UINT32 rc   = DPP_OK;
    ZXIC_UINT32 attr = UPLINK_PHY_PORT_TM_SHAPE_ENABLE;

    rc = dpp_uplink_phy_attr_set(pf_info, port, attr, mode);
    ZXIC_COMM_CHECK_RC(rc, "dpp_uplink_phy_attr_set");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_tm_pport_mcode_switch_set);

ZXIC_UINT32 dpp_tm_pport_mcode_switch_del(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 port)
{
    ZXIC_UINT32 rc = DPP_OK;

    rc = dpp_tm_pport_mcode_switch_set(pf_info, port, TM_SWITCH_OFF);
    ZXIC_COMM_CHECK_RC(rc, "dpp_tm_pport_mcode_switch_set");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_tm_pport_mcode_switch_del);

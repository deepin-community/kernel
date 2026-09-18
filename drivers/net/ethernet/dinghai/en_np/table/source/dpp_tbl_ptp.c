#include "dpp_drv_init.h"
#include "dpp_drv_acl.h"
#include "dpp_drv_hash.h"
#include "dpp_drv_eram.h"
#include "dpp_drv_sdt.h"
#include "dpp_dev.h"
#include "dpp_dtb.h"
#include "dpp_hash.h"
#include "dpp_dtb_table.h"
#include "dpp_dtb_table_api.h"
#include "dpp_tbl_api.h"
#include "dpp_tbl_comm.h"
#include "dpp_tbl_ptp.h"

ZXIC_UINT32 dpp_ptp_port_vfid_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 ptp_port_vfid)
{
    ZXIC_UINT32 rc   = DPP_OK;
    ZXIC_UINT32 attr = UPLINK_PHY_PORT_PTP_PORT_VFID;

    ZXDH_SRIOV_VPORT_T port_attr_entry = {0};

    rc = dpp_vport_attr_get(pf_info, &port_attr_entry);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_attr_get");

    rc = dpp_uplink_phy_attr_set(pf_info, port_attr_entry.uplink_phy_port_id, attr, ptp_port_vfid);
    ZXIC_COMM_CHECK_RC(rc, "dpp_uplink_phy_attr_set");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_ptp_port_vfid_set);

ZXIC_UINT32 dpp_ptp_tc_enable_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 ptp_tc_enable)
{
    ZXIC_UINT32 rc   = DPP_OK;
    ZXIC_UINT32 attr = UPLINK_PHY_PORT_PTP_TC_ENABLE;

    ZXDH_SRIOV_VPORT_T port_attr_entry = {0};

    rc = dpp_vport_attr_get(pf_info, &port_attr_entry);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_attr_get");

    rc = dpp_uplink_phy_attr_set(pf_info, port_attr_entry.uplink_phy_port_id, attr, ptp_tc_enable);
    ZXIC_COMM_CHECK_RC(rc, "dpp_uplink_phy_attr_set");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_ptp_tc_enable_set);

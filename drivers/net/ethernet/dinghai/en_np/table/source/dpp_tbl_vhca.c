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
#include "dpp_tbl_port.h"

ZXIC_UINT32 dpp_vport_vhca_id_add(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 vhca_id)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 sdt_no = ZXDH_SDT_VHCA_TABLE;

    ZXDH_VHCA_T vhca_entry = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u vhca_id: %u start.\n",
                                                pf_info->slot, pf_info->vport, sdt_no, vhca_id);

    ZXIC_COMM_MEMSET(&vhca_entry, 0, sizeof(ZXDH_VHCA_T));

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    vhca_entry.valid = 1;
    vhca_entry.vqm_vfid = VQM_VFID(pf_info->vport);

    rc = dpp_apt_dtb_eram_insert(&dev, queue, sdt_no, vhca_id, &vhca_entry);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_eram_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u vhca_id: %u success.\n",
                                                  pf_info->slot, pf_info->vport, sdt_no, vhca_id);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_vport_vhca_id_add);

ZXIC_UINT32 dpp_vport_vhca_id_del(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 vhca_id)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 sdt_no = ZXDH_SDT_VHCA_TABLE;
    ZXIC_UINT32 rc     = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u vhca_id: %u start.\n",
                                                pf_info->slot, pf_info->vport, sdt_no, vhca_id);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_apt_dtb_eram_clear(&dev, queue, sdt_no, vhca_id);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_eram_clear", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u vhca_id: %u success.\n",
                                                  pf_info->slot, pf_info->vport, sdt_no, vhca_id);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_vport_vhca_id_del);
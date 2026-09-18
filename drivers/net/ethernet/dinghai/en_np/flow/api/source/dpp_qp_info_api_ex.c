#include "dpp_flow_comm.h"
#include "dpp_tbl_qp_info.h"
#include "dpp_dev.h"


ZXIC_UINT32 dpp_tbl_qp_info_add_ex(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXDH_QP_INFO_T* pData)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 queue = 0;
    DPP_DEV_T dev = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(pData);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_apt_dtb_hash_simu_mcode_insert_ex(&dev, queue, sdt_no, pData);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_hash_simu_mcode_insert_ex", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_tbl_qp_info_add_ex);

ZXIC_UINT32 dpp_tbl_qp_info_del_ex(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXDH_QP_INFO_T* pData)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 queue = 0;
    DPP_DEV_T dev = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(pData);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_apt_dtb_hash_simu_mcode_delete_ex(&dev, queue, sdt_no, pData);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_hash_simu_mcode_delete_ex", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_tbl_qp_info_del_ex);

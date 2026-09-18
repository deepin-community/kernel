#include "dpp_flow_comm.h"
#include "dpp_tbl_fd_cfg.h"
#include "dpp_dev.h"


ZXIC_UINT32 dpp_tbl_fd_cfg_add(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 handle, ZXDH_FD_CFG_T *p_fd_cfg)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_fd_cfg);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_apt_dtb_acl_entry_insert_ex(&dev, queue, sdt_no, handle, p_fd_cfg);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_acl_entry_insert_ex", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_tbl_fd_cfg_add);

ZXIC_UINT32 dpp_tbl_fd_cfg_del(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 handle)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_apt_dtb_acl_entry_del_ex(&dev, queue, sdt_no, handle);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_acl_entry_del_ex", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_tbl_fd_cfg_del);

ZXIC_UINT32 dpp_tbl_fd_cfg_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 handle, ZXDH_FD_CFG_T *p_fd_cfg)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_fd_cfg);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_apt_dtb_acl_entry_get_ex(&dev, queue, sdt_no, handle, p_fd_cfg);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_acl_entry_get_ex", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_tbl_fd_cfg_get);

ZXIC_UINT32 dpp_tbl_fd_cfg_search(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 handle, ZXDH_FD_CFG_T *p_fd_cfg)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_fd_cfg);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_apt_dtb_acl_entry_search_ex(&dev, queue, sdt_no, handle, p_fd_cfg);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_acl_entry_search_ex", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_tbl_fd_cfg_search);

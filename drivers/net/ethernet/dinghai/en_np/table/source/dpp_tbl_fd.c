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
#include "dpp_tbl_comm.h"
#include "dpp_tbl_fd.h"
#include "dpp_tbl_stat.h"

ZXIC_UINT32 dpp_fd_acl_index_request(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *p_index)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 sdt_no = ZXDH_SDT_FD_CFG_TABLE;
    ZXIC_UINT32 rc     = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_index);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_acl_index_request(&dev, sdt_no, pf_info->vport, p_index);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_index_request", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u index: %u.\n",
                    pf_info->slot, pf_info->vport, sdt_no, *p_index);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_fd_acl_index_request);

ZXIC_UINT32 dpp_fd_acl_index_release(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 sdt_no = ZXDH_SDT_FD_CFG_TABLE;
    ZXIC_UINT32 rc     = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    
    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_acl_index_release(&dev, sdt_no, pf_info->vport, index);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_index_release", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u index: %u.\n",
                    pf_info->slot, pf_info->vport, sdt_no, index);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_fd_acl_index_release);

ZXIC_UINT32 dpp_fd_acl_entry_add(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 handle, ZXIC_UINT8 *key, ZXIC_UINT8 *key_mask, ZXIC_UINT8 *result)
{
    DPP_DEV_T dev = {0};
    DPP_DTB_ACL_ENTRY_INFO_T fd_entry = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 sdt_no = ZXDH_SDT_FD_CFG_TABLE;
    ZXIC_UINT32 element_id = 0;
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(key);
    ZXIC_COMM_CHECK_POINT(key_mask);
    ZXIC_COMM_CHECK_POINT(result);

    ZXIC_COMM_MEMSET_S(&fd_entry, sizeof(DPP_DTB_ACL_ENTRY_INFO_T), 0, sizeof(DPP_DTB_ACL_ENTRY_INFO_T));
    fd_entry.handle = handle;
    fd_entry.key_data = key;
    fd_entry.key_mask = key_mask;
    fd_entry.p_as_rslt = result;
    
    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_acl_dma_insert(&dev, queue, sdt_no, 1, &fd_entry, &element_id);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_dma_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_fd_acl_entry_add);

ZXIC_UINT32 dpp_fd_acl_entry_del(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 handle)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 sdt_no = ZXDH_SDT_FD_CFG_TABLE;
    ZXIC_UINT32 element_id = 0;
    ZXIC_UINT8 data[DPP_ETCAM_WIDTH_MAX / 8] = {0xff};
    ZXIC_UINT8 mask[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 as_rlt[16] = {0};
    DPP_DTB_ACL_ENTRY_INFO_T fd_entry = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_MEMSET(&fd_entry, 0, sizeof(DPP_DTB_ACL_ENTRY_INFO_T));
    ZXIC_COMM_MEMSET_S(data, sizeof(data), 0xff, sizeof(data));
    ZXIC_COMM_MEMSET_S(mask, sizeof(mask), 0x0, sizeof(mask));
    ZXIC_COMM_MEMSET_S(as_rlt, sizeof(as_rlt), 0xff, sizeof(as_rlt));

    fd_entry.handle = handle;
    fd_entry.key_data = data;
    fd_entry.key_mask = mask;
    fd_entry.p_as_rslt = as_rlt;

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_acl_dma_insert(&dev, queue, sdt_no, 1, &fd_entry, &element_id);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_dma_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_fd_acl_entry_del);

ZXIC_UINT32 dpp_fd_acl_entry_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 handle, ZXIC_UINT8 *key, ZXIC_UINT8 *key_mask, ZXIC_UINT8 *result)
{
    DPP_DEV_T dev = {0};
    DPP_DTB_ACL_ENTRY_INFO_T fd_entry = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 sdt_no = ZXDH_SDT_FD_CFG_TABLE;
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(key);
    ZXIC_COMM_CHECK_POINT(key_mask);
    ZXIC_COMM_CHECK_POINT(result);

    ZXIC_COMM_MEMSET_S(&fd_entry, sizeof(DPP_DTB_ACL_ENTRY_INFO_T), 0, sizeof(DPP_DTB_ACL_ENTRY_INFO_T));
    fd_entry.handle = handle;
    fd_entry.key_data = key;
    fd_entry.key_mask = key_mask;
    fd_entry.p_as_rslt = result;
    
    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_etcam_data_get(&dev, queue, sdt_no,&fd_entry);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_etcam_data_get", DEV_PCIE_LOCK(&dev));
    
    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_fd_acl_entry_get);

ZXIC_UINT32 dpp_fd_acl_entry_search(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 handle, ZXIC_UINT8 *key, ZXIC_UINT8 *key_mask, ZXIC_UINT8 *result)
{
    DPP_DEV_T dev = {0};
    DPP_DTB_ACL_ENTRY_INFO_T fd_entry = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 sdt_no = ZXDH_SDT_FD_CFG_TABLE;
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(key);
    ZXIC_COMM_CHECK_POINT(key_mask);
    ZXIC_COMM_CHECK_POINT(result);

    ZXIC_COMM_MEMSET_S(&fd_entry, sizeof(DPP_DTB_ACL_ENTRY_INFO_T), 0, sizeof(DPP_DTB_ACL_ENTRY_INFO_T));
    fd_entry.handle = handle;
    fd_entry.key_data = key;
    fd_entry.key_mask = key_mask;
    fd_entry.p_as_rslt = result;
    
    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_acl_data_get(&dev, queue, sdt_no,&fd_entry);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_data_get", DEV_PCIE_LOCK(&dev));
    
    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_fd_acl_entry_search);

ZXIC_UINT32 dpp_fd_acl_all_delete(DPP_PF_INFO_T* pf_info)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 sdt_no = ZXDH_SDT_FD_CFG_TABLE;

    ZXIC_COMM_CHECK_POINT(pf_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_acl_offline_delete(&dev, queue, sdt_no, pf_info->vport, DPP_STAT_FD_ACL_CNT_ERAM_BAADDR, 1);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_offline_delete", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_fd_acl_all_delete);

ZXIC_UINT32 dpp_fd_acl_stat_clear(DPP_PF_INFO_T* pf_info)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 sdt_no = ZXDH_SDT_FD_CFG_TABLE;

    ZXIC_COMM_CHECK_POINT(pf_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_acl_stat_clr_by_vport(&dev, queue, sdt_no, pf_info->vport, 1, DPP_STAT_FD_ACL_CNT_ERAM_BAADDR);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_stat_clr_by_vport", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_fd_acl_stat_clear);

ZXIC_UINT32 dpp_fd_acl_index_dump(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *p_index_array, ZXIC_UINT32 *index_num)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 sdt_no = ZXDH_SDT_FD_CFG_TABLE;
    ZXIC_UINT32 eram_sdt_no = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    eram_sdt_no = dpp_apt_get_sdt_partner(&dev, sdt_no);
    ZXIC_COMM_CHECK_INDEX(eram_sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    rc = dpp_vport_table_lock(pf_info, eram_sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_acl_index_parse(&dev, queue, eram_sdt_no, pf_info->vport, index_num, p_index_array);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_index_parse", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, eram_sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_fd_acl_index_dump);
#include "dpp_drv_init.h"
#include "dpp_drv_eram.h"
#include "dpp_drv_sdt.h"
#include "dpp_dev.h"
#include "dpp_dtb.h"
#include "dpp_dtb_table.h"
#include "dpp_tbl_api.h"

ZXIC_UINT32 dpp_eram_entry_insert(DPP_PF_INFO_T* pf_info,ZXIC_UINT32 sdt_no,ZXIC_UINT32 index,ZXIC_UINT8 *p_data)
{
    DPP_DEV_T dev = {0};
    DPP_DTB_ERAM_ENTRY_INFO_T eram_entry = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 element_id = 0;
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_data);

    ZXIC_COMM_MEMSET_S(&eram_entry, sizeof(DPP_DTB_ERAM_ENTRY_INFO_T), 0, sizeof(DPP_DTB_ERAM_ENTRY_INFO_T));
    eram_entry.index = index;
    eram_entry.p_data = (ZXIC_UINT32 *)p_data;
    
    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_eram_dma_write(&dev, queue, sdt_no, 1, &eram_entry, &element_id);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_dma_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_eram_entry_insert);

ZXIC_UINT32 dpp_eram_entry_delete(DPP_PF_INFO_T* pf_info,ZXIC_UINT32 sdt_no,ZXIC_UINT32 index)
{
    DPP_DEV_T dev = {0};
    ZXIC_UINT8 data[DPP_SMMU0_READ_REG_MAX_NUM*4]      = {0};
    DPP_DTB_ERAM_ENTRY_INFO_T eram_entry = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 element_id = 0;
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_MEMSET_S(&eram_entry, sizeof(DPP_DTB_ERAM_ENTRY_INFO_T), 0, sizeof(DPP_DTB_ERAM_ENTRY_INFO_T));
    ZXIC_COMM_MEMSET_S(data, sizeof(data), 0, sizeof(data));
    eram_entry.index = index;
    eram_entry.p_data = (ZXIC_UINT32 *)data;
    
    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_eram_dma_write(&dev, queue, sdt_no, 1, &eram_entry, &element_id);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_dma_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_eram_entry_delete);

ZXIC_UINT32 dpp_eram_entry_get(DPP_PF_INFO_T* pf_info,ZXIC_UINT32 sdt_no,ZXIC_UINT32 index,ZXIC_UINT8 *p_data)
{
    DPP_DEV_T dev = {0};
    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 rc = DPP_OK;
    DPP_DTB_ERAM_ENTRY_INFO_T eram_entry = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_data);

    ZXIC_COMM_MEMSET_S(&eram_entry, sizeof(DPP_DTB_ERAM_ENTRY_INFO_T), 0, sizeof(DPP_DTB_ERAM_ENTRY_INFO_T));
    eram_entry.index = index;
    eram_entry.p_data = (ZXIC_UINT32 *)p_data;
    
    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_eram_data_get(&dev, queue, sdt_no, &eram_entry);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_dma_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_eram_entry_get);

ZXIC_UINT32 dpp_hash_entry_insert(DPP_PF_INFO_T* pf_info,ZXIC_UINT32 sdt_no,ZXIC_UINT8 *key,ZXIC_UINT8 *rst)
{
    DPP_DEV_T dev = {0};
    DPP_DTB_HASH_ENTRY_INFO_T hash_entry = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 element_id = 0;
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(key);
    ZXIC_COMM_CHECK_POINT(rst);

    ZXIC_COMM_MEMSET_S(&hash_entry, sizeof(DPP_DTB_HASH_ENTRY_INFO_T), 0, sizeof(DPP_DTB_HASH_ENTRY_INFO_T));
    hash_entry.p_actu_key = key;
    hash_entry.p_rst = rst;
    
    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_hash_dma_insert(&dev, queue, sdt_no, 1, &hash_entry, &element_id);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_dma_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_hash_entry_insert);

ZXIC_UINT32 dpp_hash_entry_delete(DPP_PF_INFO_T* pf_info,ZXIC_UINT32 sdt_no,ZXIC_UINT8 *key)
{
    DPP_DEV_T dev = {0};
    DPP_DTB_HASH_ENTRY_INFO_T hash_entry = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 element_id = 0;
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(key);

    ZXIC_COMM_MEMSET_S(&hash_entry, sizeof(DPP_DTB_HASH_ENTRY_INFO_T), 0, sizeof(DPP_DTB_HASH_ENTRY_INFO_T));
    hash_entry.p_actu_key = key;
    
    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_hash_dma_delete(&dev, queue, sdt_no, 1, &hash_entry, &element_id);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_hash_dma_delete", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_hash_entry_delete);

ZXIC_UINT32 dpp_hash_entry_get(DPP_PF_INFO_T* pf_info,ZXIC_UINT32 sdt_no, ZXIC_UINT8 *key,ZXIC_UINT8 *rst, ZXIC_UINT32 srch_mode)
{
    DPP_DEV_T dev = {0};
    DPP_DTB_HASH_ENTRY_INFO_T hash_entry = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(key);
    ZXIC_COMM_CHECK_POINT(rst);

    ZXIC_COMM_MEMSET_S(&hash_entry, sizeof(DPP_DTB_HASH_ENTRY_INFO_T), 0, sizeof(DPP_DTB_HASH_ENTRY_INFO_T));
    hash_entry.p_actu_key = key;
    hash_entry.p_rst = rst;
    
    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_hash_data_get(&dev, queue, sdt_no, &hash_entry, srch_mode);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_hash_data_get", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_hash_entry_get);

ZXIC_UINT32 dpp_hash_entry_flush(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, HASH_FLUSH_MODE_ENUM flush_mode)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_DEV_T dev = {0};
    ZXIC_UINT32 queue  = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    if(flush_mode == HASH_FLUSH_ONLINE_MODE)
    {
        rc = dpp_dtb_hash_online_delete(&dev,queue,sdt_no);
        ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_acl_entry_flush_all", DEV_PCIE_LOCK(&dev));
    }
    else
    {
        rc = dpp_dtb_hash_offline_delete(&dev,queue,sdt_no);
        ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_acl_entry_flush_all", DEV_PCIE_LOCK(&dev));
    }

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_hash_entry_flush);

ZXIC_UINT32 dpp_hash_entry_soft_delete_by_sdt(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");
    
    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_hash_soft_delete_by_sdt(&dev,sdt_no);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_acl_entry_flush_all", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");
    
    return DPP_OK;
}
EXPORT_SYMBOL(dpp_hash_entry_soft_delete_by_sdt);


ZXIC_UINT32 dpp_acl_entry_add(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 entry_num, DPP_ACL_ENTRY_INFO_T *p_acl_entry_info)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 element_id = 0;
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_acl_entry_info);
    
    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_acl_dma_insert(&dev, queue, sdt_no, entry_num, (DPP_DTB_ACL_ENTRY_INFO_T *)p_acl_entry_info, &element_id);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_dma_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_acl_entry_add);

ZXIC_UINT32 dpp_acl_entry_del(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 handle)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 element_id = 0;
    ZXIC_UINT8 data[DPP_ETCAM_WIDTH_MAX / 8] = {0xff};
    ZXIC_UINT8 mask[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 as_rlt[16] = {0};
    DPP_DTB_ACL_ENTRY_INFO_T entry = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_MEMSET_S(&entry, sizeof(DPP_DTB_ACL_ENTRY_INFO_T), 0, sizeof(DPP_DTB_ACL_ENTRY_INFO_T));
    ZXIC_COMM_MEMSET_S(data, sizeof(data), 0xff, sizeof(data));
    ZXIC_COMM_MEMSET_S(mask, sizeof(mask), 0x0, sizeof(mask));
    ZXIC_COMM_MEMSET_S(as_rlt, sizeof(as_rlt), 0xff, sizeof(as_rlt));

    entry.handle = handle;
    entry.key_data = data;
    entry.key_mask = mask;
    entry.p_as_rslt = as_rlt;

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_acl_dma_insert(&dev, queue, sdt_no, 1, &entry, &element_id);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_dma_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_acl_entry_del);

ZXIC_UINT32 dpp_acl_entry_del_by_key(DPP_PF_INFO_T* pf_info, 
                                  ZXIC_UINT32 sdt_no, 
                                  ZXIC_UINT32 entry_num, 
                                  DPP_ACL_ENTRY_INFO_T *p_acl_entry_info)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_DEV_T dev = {0};
    ZXIC_UINT32 queue  = 0;
    DPP_DTB_ACL_ENTRY_INFO_T *p_entry_info = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_acl_entry_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    p_entry_info = (DPP_DTB_ACL_ENTRY_INFO_T *)p_acl_entry_info;
    rc = dpp_dtb_acl_delete_by_key(&dev,queue,sdt_no,pf_info->vport,entry_num,p_entry_info);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_delete_by_key", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_acl_entry_del_by_key);

ZXIC_UINT32 dpp_acl_entry_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, DPP_ACL_ENTRY_INFO_T *p_acl_entry_info)
{
    DPP_DEV_T dev = {0};
    DPP_DTB_ACL_ENTRY_INFO_T entry = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_acl_entry_info);
    ZXIC_COMM_CHECK_POINT(p_acl_entry_info->key_data);
    ZXIC_COMM_CHECK_POINT(p_acl_entry_info->key_mask);
    ZXIC_COMM_CHECK_POINT(p_acl_entry_info->p_as_rslt);

    ZXIC_COMM_MEMSET_S(&entry, sizeof(DPP_DTB_ACL_ENTRY_INFO_T), 0, sizeof(DPP_DTB_ACL_ENTRY_INFO_T));
    entry.handle = p_acl_entry_info->handle;
    entry.key_data = p_acl_entry_info->key_data;
    entry.key_mask = p_acl_entry_info->key_mask;
    entry.p_as_rslt = p_acl_entry_info->p_as_rslt;
    
    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_etcam_data_get(&dev, queue, sdt_no,&entry);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_etcam_data_get", DEV_PCIE_LOCK(&dev));
    
    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_acl_entry_get);

ZXIC_UINT32 dpp_acl_entry_search_by_key(DPP_PF_INFO_T* pf_info, 
                                  ZXIC_UINT32 sdt_no, 
                                  ZXIC_UINT32 entry_num, 
                                  DPP_ACL_ENTRY_INFO_T *p_acl_entry_info)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_DEV_T dev = {0};
    ZXIC_UINT32 queue  = 0;
    DPP_DTB_ACL_ENTRY_INFO_T *p_entry_info = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_acl_entry_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    p_entry_info = (DPP_DTB_ACL_ENTRY_INFO_T *)p_acl_entry_info;
    rc = dpp_dtb_acl_search_by_key(&dev,queue,sdt_no,pf_info->vport,entry_num,p_entry_info);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_search_by_key", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_acl_entry_search_by_key);

ZXIC_UINT32 dpp_acl_entry_dump_by_vport(DPP_PF_INFO_T* pf_info, 
                                  ZXIC_UINT32 sdt_no, 
                                  ZXIC_UINT32 *entry_num, 
                                  DPP_ACL_ENTRY_INFO_T *p_acl_entry_info)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_DEV_T dev = {0};
    ZXIC_UINT32 queue  = 0;
    DPP_DTB_ACL_ENTRY_INFO_T *p_entry_info = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_acl_entry_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    p_entry_info = (DPP_DTB_ACL_ENTRY_INFO_T *)p_acl_entry_info;
    rc = dpp_dtb_acl_entry_dump_by_vport(&dev,queue,sdt_no,pf_info->vport,entry_num,p_entry_info);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_entry_dump_by_vport", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_acl_entry_dump_by_vport);

ZXIC_UINT32 dpp_acl_entry_dump_all(DPP_PF_INFO_T* pf_info, 
                                  ZXIC_UINT32 sdt_no, 
                                  ZXIC_UINT32 *entry_num, 
                                  DPP_ACL_ENTRY_INFO_T *p_acl_entry_info)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_DEV_T dev = {0};
    ZXIC_UINT32 queue  = 0;
    DPP_DTB_ACL_ENTRY_INFO_T *p_entry_info = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_acl_entry_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    p_entry_info = (DPP_DTB_ACL_ENTRY_INFO_T *)p_acl_entry_info;
    rc = dpp_dtb_acl_dump(&dev, queue, sdt_no, (ZXIC_UINT8 *)p_entry_info, entry_num);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_dump", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_acl_entry_dump_all);

ZXIC_UINT32 dpp_acl_entry_flush_by_vport(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_DEV_T dev = {0};
    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 exist_flag = 0;
    DPP_APT_SE_RES_T *p_se_res = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(&dev);
    ZXIC_COMM_CHECK_POINT(p_se_res);

    rc = dpp_apt_sdt_is_exist(p_se_res,DPP_SDT_TBLT_eTCAM,sdt_no,&exist_flag);
    ZXIC_COMM_CHECK_RC(rc, "dpp_apt_sdt_is_exist");
    if(!exist_flag)
    {
        ZXIC_COMM_TRACE_NOTICE("sdt_no:%d is not exsit, can not flush acl.\n", sdt_no);
        return DPP_OK;
    }

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_acl_entry_flush_by_vport(&dev,queue,sdt_no,pf_info->vport);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_entry_flush_by_vport", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_acl_entry_flush_by_vport);

ZXIC_UINT32 dpp_acl_entry_del_by_index(DPP_PF_INFO_T* pf_info, 
                                  ZXIC_UINT32 sdt_no, 
                                  ZXIC_UINT32 entry_num, 
                                  DPP_ACL_ENTRY_INFO_T *p_acl_entry_info)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_DEV_T dev = {0};
    ZXIC_UINT32 queue  = 0;
    DPP_DTB_ACL_ENTRY_INFO_T *p_entry_info = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_acl_entry_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    p_entry_info = (DPP_DTB_ACL_ENTRY_INFO_T *)p_acl_entry_info;
    rc = dpp_dtb_acl_del_by_index(&dev,queue,sdt_no,pf_info->vport,entry_num,p_entry_info);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_del_by_index", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_acl_entry_del_by_index);

ZXIC_UINT32 dpp_acl_entry_search_by_index(DPP_PF_INFO_T* pf_info, 
                                  ZXIC_UINT32 sdt_no, 
                                  ZXIC_UINT32 entry_num, 
                                  DPP_ACL_ENTRY_INFO_T *p_acl_entry_info)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_DEV_T dev = {0};
    ZXIC_UINT32 queue  = 0;
    DPP_DTB_ACL_ENTRY_INFO_T *p_entry_info = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_acl_entry_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    p_entry_info = (DPP_DTB_ACL_ENTRY_INFO_T *)p_acl_entry_info;
    rc = dpp_dtb_acl_search_by_index(&dev,queue,sdt_no,pf_info->vport,entry_num,p_entry_info);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_del_by_index", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_acl_entry_search_by_index);

ZXIC_UINT32 dpp_acl_entry_flush_all(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_DEV_T dev = {0};
    ZXIC_UINT32 queue  = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_acl_entry_flush_all(&dev,queue,sdt_no);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_acl_entry_flush_all", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_acl_entry_flush_all);

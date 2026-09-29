#include "dpp_drv_init.h"
#include "dpp_drv_acl.h"
#include "dpp_drv_hash.h"
#include "dpp_drv_eram.h"
#include "dpp_drv_sdt.h"
#include "dpp_dev.h"
#include "dpp_dtb.h"
#include "dpp_sdt.h"
#include "dpp_hash.h"
#include "dpp_dtb_table.h"
#include "dpp_dtb_table_api.h"
#include "dpp_tbl_comm.h"
#include "dpp_tbl_mac.h"
#include "dpp_tbl_api.h"

ZXIC_VOID dpp_rdma_trans_item_print(ZXDH_RDMA_TRANS_T *rdma_trans)
{
    ZXIC_COMM_TRACE_NOTICE("key--mac: %02x:%02x:%02x:%02x:%02x:%02x.\n", 
                               rdma_trans->key.mac_addr[0], rdma_trans->key.mac_addr[1],
                               rdma_trans->key.mac_addr[2], rdma_trans->key.mac_addr[3],
                               rdma_trans->key.mac_addr[4], rdma_trans->key.mac_addr[5]);
    ZXIC_COMM_TRACE_NOTICE("key--rsv: 0x%02x\n", rdma_trans->key.rsv);

    ZXIC_COMM_TRACE_NOTICE("entry--rdma_vhca_id: 0x%02x\n", rdma_trans->entry.rdma_vhca_id);
    ZXIC_COMM_TRACE_NOTICE("entry--rsv:          0x%02x\n", rdma_trans->entry.rsv);
    ZXIC_COMM_TRACE_NOTICE("entry--hit_flag:     0x%02x\n", rdma_trans->entry.hit_flag);
}

ZXIC_UINT32 dpp_add_rdma_trans_item(DPP_PF_INFO_T* pf_info, ZXIC_CONST ZXIC_VOID *mac, ZXIC_CONST ZXIC_UINT16 vhcaId)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue      = 0;
    ZXIC_UINT32 sdt_no     = ZXDH_SDT_RDMA_ENTRY_TABLE;
    ZXIC_UINT32 rc         = DPP_OK;

    ZXDH_RDMA_TRANS_T rdma_trans = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(mac);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x start.\n", pf_info->slot, pf_info->vport);

    ZXIC_COMM_MEMSET(&rdma_trans, 0, sizeof(ZXDH_RDMA_TRANS_T));

    ZXIC_COMM_MEMCPY(rdma_trans.key.mac_addr, mac, 6);
    rdma_trans.entry.rdma_vhca_id = vhcaId & 0x3ff;
    rdma_trans.entry.hit_flag = 0x00;

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_apt_dtb_hash_insert(&dev, queue, sdt_no, &rdma_trans);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_hash_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u rdma_vhca_id: %u mac: %02x:%02x:%02x:%02x:%02x:%02x success.\n",
                                                                        pf_info->slot, pf_info->vport, sdt_no,
                                                                        rdma_trans.entry.rdma_vhca_id,
                                                                     rdma_trans.key.mac_addr[0], rdma_trans.key.mac_addr[1],
                                                                     rdma_trans.key.mac_addr[2], rdma_trans.key.mac_addr[3],
                                                                     rdma_trans.key.mac_addr[4], rdma_trans.key.mac_addr[5]);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_add_rdma_trans_item);

ZXIC_UINT32 dpp_del_rdma_trans_item(DPP_PF_INFO_T* pf_info, ZXIC_CONST ZXIC_VOID *mac)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue      = 0;
    ZXIC_UINT32 sdt_no     = ZXDH_SDT_RDMA_ENTRY_TABLE;
    ZXIC_UINT32 rc         = DPP_OK;

    ZXDH_RDMA_TRANS_T rdma_trans = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(mac);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x start.\n", pf_info->slot, pf_info->vport);

    ZXIC_COMM_MEMSET(&rdma_trans, 0, sizeof(ZXDH_RDMA_TRANS_T));

    ZXIC_COMM_MEMCPY(rdma_trans.key.mac_addr, mac, 6);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_apt_dtb_hash_delete(&dev, queue, sdt_no, &rdma_trans);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_hash_delete", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u rdma_vhca_id: %u mac: %02x:%02x:%02x:%02x:%02x:%02x success.\n",
                                                          pf_info->slot, pf_info->vport, sdt_no,
                                                          rdma_trans.entry.rdma_vhca_id,
                                                          rdma_trans.key.mac_addr[0], rdma_trans.key.mac_addr[1],
                                                          rdma_trans.key.mac_addr[2], rdma_trans.key.mac_addr[3],
                                                          rdma_trans.key.mac_addr[4], rdma_trans.key.mac_addr[5]);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_del_rdma_trans_item);

ZXIC_UINT32 dpp_search_rdma_trans_item(DPP_PF_INFO_T* pf_info, ZXIC_CONST ZXIC_VOID *mac, ZXIC_UINT32 *vhca_id)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 queue = 0;
    ZXIC_UINT32 sdt_no = ZXDH_SDT_RDMA_ENTRY_TABLE;
    ZXIC_UINT32 srch_mode   = HASH_SRH_MODE_HDW;
    ZXIC_UINT8 key_valid = 1;
    ZXIC_UINT8 actKey[HASH_KEY_MAX] = {0};
    ZXIC_UINT8 actRst[HASH_RST_MAX] = {0};

    DPP_DEV_T dev = {0};
    ZXDH_RDMA_TRANS_T rdma_trans = {0};
    DPP_DTB_HASH_ENTRY_INFO_T p_dtb_hash_entry = {0};
    DPP_HASH_ENTRY entry = {0};
    DPP_SDTTBL_HASH_T sdt_hash_info = {0};  /*SDT内容*/
    SE_APT_CALLBACK_T *pAptCallback = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(mac);
    ZXIC_COMM_CHECK_POINT(vhca_id);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x start.\n", pf_info->slot, pf_info->vport);

    ZXIC_COMM_MEMSET_S(&rdma_trans, sizeof(ZXDH_RDMA_TRANS_T), 0, sizeof(ZXDH_RDMA_TRANS_T));
    ZXIC_COMM_MEMCPY_S(rdma_trans.key.mac_addr, 6, mac, 6);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(&dev, sdt_no, &sdt_hash_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_soft_sdt_tbl_get");

    pAptCallback = dpp_apt_get_func(&dev, sdt_no);
    ZXIC_COMM_CHECK_POINT(pAptCallback);

    entry.p_key = actKey;
    entry.p_rst = actRst;
    entry.p_key[0] = DPP_GET_HASH_KEY_CTRL(key_valid, sdt_hash_info.hash_table_width, sdt_hash_info.hash_table_id);

    //结构体转码流
    rc = pAptCallback->se_func_info.hashFunc.hash_set_func(&rdma_trans, &entry);
    ZXIC_COMM_CHECK_RC(rc, "hash_set_func");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    p_dtb_hash_entry.p_actu_key = &entry.p_key[1];
    p_dtb_hash_entry.p_rst = entry.p_rst;
    rc = dpp_dtb_hash_data_get(&dev, queue, sdt_no, &p_dtb_hash_entry, srch_mode);

    if (rc != DPP_OK)
    {
        if(rc == DPP_HASH_RC_SRH_FAIL)
        {
            ZXIC_COMM_TRACE_NOTICE("There is no such rdma hash!\n");
            rc = dpp_vport_table_unlock(pf_info, sdt_no);
            ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");
            return DPP_HASH_RC_SRH_FAIL;
        }
        rc = dpp_vport_table_unlock(pf_info, sdt_no);
        ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");
        return DPP_ERR;
    }

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    //码流转结构体
    rc = pAptCallback->se_func_info.hashFunc.hash_get_func(&rdma_trans, &entry);
    ZXIC_COMM_CHECK_RC(rc, "hash_get_func");

    *vhca_id = rdma_trans.entry.rdma_vhca_id;

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u rdma_vhca_id: %u mac: %02x:%02x:%02x:%02x:%02x:%02x success.\n",
                                                          pf_info->slot, pf_info->vport, sdt_no,
                                                          rdma_trans.entry.rdma_vhca_id,
                                                          rdma_trans.key.mac_addr[0], rdma_trans.key.mac_addr[1],
                                                          rdma_trans.key.mac_addr[2], rdma_trans.key.mac_addr[3],
                                                          rdma_trans.key.mac_addr[4], rdma_trans.key.mac_addr[5]);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_search_rdma_trans_item);

ZXIC_UINT32 dpp_rdma_trans_item_soft_delete(DPP_PF_INFO_T* pf_info)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 sdt_no     = ZXDH_SDT_RDMA_ENTRY_TABLE;
    ZXIC_UINT32 last_flag  = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x start.\n", pf_info->slot, pf_info->vport);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dev_last_check(&dev, &last_flag);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_last_check");

    if(last_flag)
    {
        rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
        ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
        ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

        rc = dpp_hash_soft_delete_by_sdt(&dev, sdt_no);
        ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_hash_soft_delete_by_sdt", DEV_PCIE_LOCK(&dev));

        rc = dpp_vport_table_unlock(pf_info, sdt_no);
        ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");
    }

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x success.\n", pf_info->slot, pf_info->vport);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_rdma_trans_item_soft_delete);



ZXIC_UINT32 dpp_get_rdma_trans_item_pos(DPP_PF_INFO_T* pf_info, ZXIC_CONST ZXIC_VOID *mac, DPP_HASH_ZCAM_POS_INFO *p_pos_info)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT8  srh_succ   = 0;
    ZXIC_UINT32 rc         = DPP_OK;
    ZXIC_UINT32 sdt_no     = ZXDH_SDT_RDMA_ENTRY_TABLE;
    
    ZXDH_RDMA_TRANS_T rdma_trans = {0};
    
    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(mac);

    ZXIC_COMM_MEMSET_S(&rdma_trans, sizeof(ZXDH_RDMA_TRANS_T), 0, sizeof(ZXDH_RDMA_TRANS_T));
    ZXIC_COMM_MEMCPY_S(rdma_trans.key.mac_addr, 6, mac, 6);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_hash_zcam_pos_get(&dev, sdt_no, &rdma_trans, p_pos_info, &srh_succ);
    ZXIC_COMM_CHECK_RC(rc, "dpp_hash_zcam_pos_get");

    if(!srh_succ)
    {
        ZXIC_COMM_TRACE_NOTICE("Hash search key fail!\n");
        return DPP_HASH_RC_SRH_FAIL;
    }

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_get_rdma_trans_item_pos);
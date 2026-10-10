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
#include "dpp_tbl_mac.h"
#include "dpp_tbl_api.h"
#include "dpp_apt_se.h"
#include "dpp_sdt.h"


ZXIC_VOID dpp_l2_entry_print(ZXDH_L2_ENTRY_T *l2_entry)
{
    ZXIC_COMM_TRACE_NOTICE("key--vlan_id: 0x%04x\n", l2_entry->key.sriov_vlan_id);
    ZXIC_COMM_TRACE_NOTICE("key--vlan_tpid: 0x%04x\n", l2_entry->key.sriov_vlan_tpid);
    ZXIC_COMM_TRACE_NOTICE("key--mac: %02x:%02x:%02x:%02x:%02x:%02x.\n",
                               l2_entry->key.dmac_addr[0], l2_entry->key.dmac_addr[1],
                               l2_entry->key.dmac_addr[2], l2_entry->key.dmac_addr[3],
                               l2_entry->key.dmac_addr[4], l2_entry->key.dmac_addr[5]);

    ZXIC_COMM_TRACE_NOTICE("entry--vqm_vfid: 0x%02x\n", l2_entry->entry.vqm_vfid);
    ZXIC_COMM_TRACE_NOTICE("entry--rsv: 0x%02x\n", l2_entry->entry.rsv);
    ZXIC_COMM_TRACE_NOTICE("entry--hit_flag: 0x%02x\n", l2_entry->entry.hit_flag);
}

ZXIC_UINT32 dpp_add_mac(DPP_PF_INFO_T* pf_info, ZXIC_CONST ZXIC_VOID *mac, ZXIC_UINT16 sriov_vlan_tpid, ZXIC_UINT16 sriov_vlan_id)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue      = 0;
    ZXIC_UINT32 sdt_no     = 0;
    ZXIC_UINT32 hash_index = 0;
    ZXIC_UINT32 rc         = DPP_OK;

    ZXDH_L2_ENTRY_T l2_entry = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(mac);

    ZXIC_COMM_MEMSET(&l2_entry, 0, sizeof(ZXDH_L2_ENTRY_T));
    ZXIC_COMM_MEMCPY(l2_entry.key.dmac_addr, mac, 6);
    l2_entry.key.sriov_vlan_tpid = sriov_vlan_tpid;
    l2_entry.key.sriov_vlan_id = sriov_vlan_id;

    l2_entry.entry.vqm_vfid = VQM_VFID(pf_info->vport);
    l2_entry.entry.hit_flag = 0x00;

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sriov_vlan_tpid: 0x%04x sriov_vlan_id: 0x%04x mac: %02x:%02x:%02x:%02x:%02x:%02x start.\n", 
                                                            pf_info->slot, pf_info->vport,
                                                            l2_entry.key.sriov_vlan_tpid, l2_entry.key.sriov_vlan_id,
                                                            l2_entry.key.dmac_addr[0], l2_entry.key.dmac_addr[1],
                                                            l2_entry.key.dmac_addr[2], l2_entry.key.dmac_addr[3],
                                                            l2_entry.key.dmac_addr[4], l2_entry.key.dmac_addr[5]);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_hash_index_get(pf_info, &hash_index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_hash_index_get");

    sdt_no = ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT0 + hash_index;

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_apt_dtb_hash_insert(&dev, queue, sdt_no, &l2_entry);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_hash_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sriov_vlan_tpid: 0x%04x sriov_vlan_id: 0x%04x mac: %02x:%02x:%02x:%02x:%02x:%02x success.\n", 
                                                            pf_info->slot, pf_info->vport,
                                                            l2_entry.key.sriov_vlan_tpid, l2_entry.key.sriov_vlan_id,
                                                            l2_entry.key.dmac_addr[0], l2_entry.key.dmac_addr[1],
                                                            l2_entry.key.dmac_addr[2], l2_entry.key.dmac_addr[3],
                                                            l2_entry.key.dmac_addr[4], l2_entry.key.dmac_addr[5]);
    return DPP_OK;
}
EXPORT_SYMBOL(dpp_add_mac);

ZXIC_UINT32 dpp_del_mac(DPP_PF_INFO_T* pf_info, ZXIC_CONST ZXIC_VOID *mac, ZXIC_UINT16 sriov_vlan_tpid, ZXIC_UINT16 sriov_vlan_id)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue      = 0;
    ZXIC_UINT32 sdt_no     = 0;
    ZXIC_UINT32 hash_index = 0;
    ZXIC_UINT32 rc         = DPP_OK;

    ZXDH_L2_ENTRY_T l2_entry = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(mac);

    ZXIC_COMM_MEMSET(&l2_entry, 0, sizeof(ZXDH_L2_ENTRY_T));
    ZXIC_COMM_MEMCPY(l2_entry.key.dmac_addr, mac, 6);
    l2_entry.key.sriov_vlan_tpid = sriov_vlan_tpid;
    l2_entry.key.sriov_vlan_id = sriov_vlan_id;

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sriov_vlan_tpid: 0x%04x sriov_vlan_id: 0x%04x mac: %02x:%02x:%02x:%02x:%02x:%02x start.\n", 
                                                            pf_info->slot, pf_info->vport,
                                                            l2_entry.key.sriov_vlan_tpid, l2_entry.key.sriov_vlan_id,
                                                            l2_entry.key.dmac_addr[0], l2_entry.key.dmac_addr[1],
                                                            l2_entry.key.dmac_addr[2], l2_entry.key.dmac_addr[3],
                                                            l2_entry.key.dmac_addr[4], l2_entry.key.dmac_addr[5]);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_hash_index_get(pf_info, &hash_index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_hash_index_get");

    sdt_no = ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT0 + hash_index;

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_apt_dtb_hash_delete(&dev, queue, sdt_no, &l2_entry);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_hash_delete", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sriov_vlan_tpid: 0x%04x sriov_vlan_id: 0x%04x mac: %02x:%02x:%02x:%02x:%02x:%02x success.\n", 
                                                            pf_info->slot, pf_info->vport,
                                                            l2_entry.key.sriov_vlan_tpid, l2_entry.key.sriov_vlan_id,
                                                            l2_entry.key.dmac_addr[0], l2_entry.key.dmac_addr[1],
                                                            l2_entry.key.dmac_addr[2], l2_entry.key.dmac_addr[3],
                                                            l2_entry.key.dmac_addr[4], l2_entry.key.dmac_addr[5]);
    return DPP_OK;
}
EXPORT_SYMBOL(dpp_del_mac);

ZXIC_UINT32 dpp_batch_add_unicast_mac(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 mac_num, ZXIC_CONST ZXIC_VOID *l2key)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue       = 0;
    ZXIC_UINT32 sdt_no      = 0;
    ZXIC_UINT32 hash_index  = 0;
    ZXIC_UINT32 entry_index = 0;
    ZXIC_UINT32 rc          = DPP_OK;

    ZXDH_L2_ENTRY_T *p_multi_l2_entry = NULL;
    ZXDH_L2_ENTRY_T *p_one_l2_entry = NULL;
    ZXDH_L2_FWD_KEY *p_mac_key = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(l2key);
    ZXIC_COMM_CHECK_INDEX_LOWER(mac_num,1);

    p_multi_l2_entry = (ZXDH_L2_ENTRY_T *)ZXIC_COMM_MALLOC(mac_num * sizeof(ZXDH_L2_ENTRY_T));
    ZXIC_COMM_CHECK_POINT(p_multi_l2_entry);
    ZXIC_COMM_MEMSET_S(p_multi_l2_entry, mac_num * sizeof(ZXDH_L2_ENTRY_T), 0, mac_num * sizeof(ZXDH_L2_ENTRY_T));

    for(entry_index = 0;entry_index<mac_num;entry_index++)
    {
        p_one_l2_entry = p_multi_l2_entry + entry_index;
        p_mac_key = (ZXDH_L2_FWD_KEY *)l2key+entry_index;
        p_one_l2_entry->entry.vqm_vfid = VQM_VFID(pf_info->vport);
        p_one_l2_entry->entry.hit_flag = 0x00;
        ZXIC_COMM_MEMCPY_S((ZXIC_UINT8 *)&(p_one_l2_entry->key), sizeof(ZXDH_L2_FWD_KEY), p_mac_key, sizeof(ZXDH_L2_FWD_KEY));
        ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x tpid: 0x%04x vlan_id: 0x%04x mac: %02x:%02x:%02x:%02x:%02x:%02x start.\n",
                                                            pf_info->slot, pf_info->vport,
                                                            p_one_l2_entry->key.sriov_vlan_tpid, p_one_l2_entry->key.sriov_vlan_id,
                                                            p_one_l2_entry->key.dmac_addr[0], p_one_l2_entry->key.dmac_addr[1],
                                                            p_one_l2_entry->key.dmac_addr[2], p_one_l2_entry->key.dmac_addr[3],
                                                            p_one_l2_entry->key.dmac_addr[4], p_one_l2_entry->key.dmac_addr[5]);

    }

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_dev_get", p_multi_l2_entry);

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_dtb_queue_id_get", p_multi_l2_entry);

    rc = dpp_vport_hash_index_get(pf_info, &hash_index);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_vport_hash_index_get", p_multi_l2_entry);

    sdt_no = ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT0 + hash_index;

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_vport_table_lock", p_multi_l2_entry);
    ZXIC_COMM_CHECK_POINT_MEMORY_FREE(DEV_PCIE_LOCK(&dev),p_multi_l2_entry);

    rc = dpp_apt_dtb_multi_hash_insert(&dev, queue, sdt_no, mac_num,sizeof(ZXDH_L2_ENTRY_T),p_multi_l2_entry);
    ZXIC_COMM_FREE(p_multi_l2_entry);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_hash_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_batch_add_unicast_mac);

ZXIC_UINT32 dpp_batch_del_unicast_mac(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 mac_num, ZXIC_CONST ZXIC_VOID *l2key)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue      = 0;
    ZXIC_UINT32 sdt_no     = 0;
    ZXIC_UINT32 hash_index = 0;
    ZXIC_UINT32 entry_index = 0;
    ZXIC_UINT32 rc         = DPP_OK;

    ZXDH_L2_ENTRY_T *p_multi_l2_entry = NULL;
    ZXDH_L2_ENTRY_T *p_one_l2_entry = NULL;
    ZXDH_L2_FWD_KEY *p_mac_key = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(l2key);
    ZXIC_COMM_CHECK_INDEX_LOWER(mac_num,1);

    p_multi_l2_entry = (ZXDH_L2_ENTRY_T *)ZXIC_COMM_MALLOC(mac_num * sizeof(ZXDH_L2_ENTRY_T));
    ZXIC_COMM_CHECK_POINT(p_multi_l2_entry);
    ZXIC_COMM_MEMSET_S(p_multi_l2_entry, mac_num * sizeof(ZXDH_L2_ENTRY_T), 0, mac_num * sizeof(ZXDH_L2_ENTRY_T));

    for(entry_index = 0;entry_index<mac_num;entry_index++)
    {
        p_one_l2_entry = p_multi_l2_entry + entry_index;
        p_mac_key = (ZXDH_L2_FWD_KEY *)l2key+entry_index;
        ZXIC_COMM_MEMCPY_S((ZXIC_UINT8 *)&(p_one_l2_entry->key), sizeof(ZXDH_L2_FWD_KEY), p_mac_key, sizeof(ZXDH_L2_FWD_KEY));
        ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x tpid: 0x%04x vlan_id: 0x%04x mac: %02x:%02x:%02x:%02x:%02x:%02x start.\n", 
                                                            pf_info->slot, pf_info->vport,
                                                            p_one_l2_entry->key.sriov_vlan_tpid, p_one_l2_entry->key.sriov_vlan_id,
                                                            p_one_l2_entry->key.dmac_addr[0], p_one_l2_entry->key.dmac_addr[1],
                                                            p_one_l2_entry->key.dmac_addr[2], p_one_l2_entry->key.dmac_addr[3],
                                                            p_one_l2_entry->key.dmac_addr[4], p_one_l2_entry->key.dmac_addr[5]);
    }

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_dev_get", p_multi_l2_entry);

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_dtb_queue_id_get", p_multi_l2_entry);

    rc = dpp_vport_hash_index_get(pf_info, &hash_index);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_vport_hash_index_get", p_multi_l2_entry);

    sdt_no = ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT0 + hash_index;

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_vport_table_lock", p_multi_l2_entry);
    ZXIC_COMM_CHECK_POINT_MEMORY_FREE(DEV_PCIE_LOCK(&dev), p_multi_l2_entry);

    rc = dpp_apt_dtb_multi_hash_delete(&dev, queue, sdt_no, mac_num, sizeof(ZXDH_L2_ENTRY_T),p_multi_l2_entry);
    ZXIC_COMM_FREE(p_multi_l2_entry);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_multi_hash_delete", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_batch_del_unicast_mac);

ZXIC_UINT32 dpp_unicast_mac_search(DPP_PF_INFO_T* pf_info, ZXIC_CONST ZXIC_VOID *mac, ZXIC_UINT16 sriov_vlan_tpid, ZXIC_UINT16 sriov_vlan_id, ZXIC_UINT16 *current_vport)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT8 key_valid   = 1;
    ZXIC_UINT32 hash_index = 0;
    ZXIC_UINT32 sdt_no     = 0;
    ZXIC_UINT32 queue      = 0;
    ZXIC_UINT32 srch_mode   = HASH_SRH_MODE_HDW;
    ZXIC_UINT32 rc         = DPP_OK;

    DPP_HASH_ENTRY entry = {0};
    DPP_SDTTBL_HASH_T sdt_hash_info = {0};  /*SDT内容*/
    ZXIC_UINT8 actKey[HASH_KEY_MAX] = {0};
    ZXIC_UINT8 actRst[HASH_RST_MAX] = {0};
    ZXDH_L2_ENTRY_T l2_entry = {0};
    DPP_DTB_HASH_ENTRY_INFO_T p_dtb_hash_entry = {0};

    SE_APT_CALLBACK_T *pAptCallback = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(mac);
    ZXIC_COMM_CHECK_POINT(current_vport);

    ZXIC_COMM_MEMSET(&l2_entry, 0, sizeof(ZXDH_L2_ENTRY_T));
    ZXIC_COMM_MEMCPY(l2_entry.key.dmac_addr, mac, 6);
    l2_entry.key.sriov_vlan_tpid = sriov_vlan_tpid;
    l2_entry.key.sriov_vlan_id = sriov_vlan_id;

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x vlan_tpid: 0x%04x vlan_id: 0x%04x.\n",
                            pf_info->slot, pf_info->vport, l2_entry.key.sriov_vlan_tpid, l2_entry.key.sriov_vlan_id);
    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x mac: %02x:%02x:%02x:%02x:%02x:%02x start.\n",
                                pf_info->slot, pf_info->vport,  l2_entry.key.dmac_addr[0], l2_entry.key.dmac_addr[1],
                                l2_entry.key.dmac_addr[2], l2_entry.key.dmac_addr[3], l2_entry.key.dmac_addr[4], l2_entry.key.dmac_addr[5]);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_vport_hash_index_get(pf_info, &hash_index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_hash_index_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    sdt_no = ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT0 + hash_index;

    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(&dev, sdt_no, &sdt_hash_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_soft_sdt_tbl_get");

    pAptCallback = dpp_apt_get_func(&dev, sdt_no);
    ZXIC_COMM_CHECK_POINT(pAptCallback);

    entry.p_key = actKey;
    entry.p_rst = actRst;
    entry.p_key[0] = DPP_GET_HASH_KEY_CTRL(key_valid, sdt_hash_info.hash_table_width, sdt_hash_info.hash_table_id);

    //结构体转码流
    rc = pAptCallback->se_func_info.hashFunc.hash_set_func(&l2_entry,&entry);
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
            ZXIC_COMM_TRACE_NOTICE("There is no such hash!\n");
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
    rc = pAptCallback->se_func_info.hashFunc.hash_get_func(&l2_entry, &entry);
    ZXIC_COMM_CHECK_RC(rc, "hash_get_func");

    rc = dpp_vport_get_by_vqm_vfid(OWNER_PF_VPORT(pf_info->vport), l2_entry.entry.vqm_vfid, current_vport);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_get_by_vqm_vfid");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u vqm_vfid: %u vport: 0x%04x success.\n",
                                 pf_info->slot, pf_info->vport, sdt_no, l2_entry.entry.vqm_vfid, *current_vport);
    return DPP_OK;
}
EXPORT_SYMBOL(dpp_unicast_mac_search);

ZXIC_UINT32 dpp_unicast_mac_dump(DPP_PF_INFO_T* pf_info, MAC_VPORT_INFO *p_mac_arr, ZXIC_UINT32 *p_mac_num)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 rc     = DPP_OK;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 hash_index = 0;
    ZXIC_UINT32 sdt_no = 0;
    ZXIC_UINT32 current_vqm_vfid = 0;
    ZXIC_UINT16 current_vport = 0;

    ZXIC_UINT32 max_item_num = DTB_DUMP_UNICAST_MAC_DUMP_NUM;
    ZXIC_UINT32 entryNum = 0;
    ZXDH_L2_ENTRY_T *pL2DataArr = NULL;
    ZXDH_L2_ENTRY_T *p_l2_temp_entry = NULL;
    MAC_VPORT_INFO *p_temp_mac_info = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_mac_num);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_hash_index_get(pf_info, &hash_index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_hash_index_get");
    sdt_no = ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT0 + hash_index;

    rc = dpp_hash_max_item_num_get(&dev,sdt_no,&max_item_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_hash_max_item_num_get");

    pL2DataArr = (ZXDH_L2_ENTRY_T *)ZXIC_COMM_VMALLOC(max_item_num * sizeof(ZXDH_L2_ENTRY_T));
    ZXIC_COMM_CHECK_POINT_NO_ASSERT(pL2DataArr);

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE(rc, "dpp_vport_table_lock", pL2DataArr);
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE(DEV_PCIE_LOCK(&dev), pL2DataArr);

    rc = dpp_apt_dtb_hash_table_unicast_mac_dump(&dev, queue, sdt_no, pL2DataArr, &entryNum);
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE_UNLOCK_NO_ASSERT(rc, "dpp_apt_dtb_hash_table_unicast_mac_dump", pL2DataArr, DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE(rc, "dpp_vport_table_unlock", pL2DataArr);

    ZXIC_COMM_TRACE_NOTICE("unicast mac dump num:0x%x\n", entryNum);

    if(p_mac_arr != ZXIC_NULL)
    {
        for(index = 0; index < entryNum; index ++)
        {
            p_l2_temp_entry = pL2DataArr + index;
            p_temp_mac_info = p_mac_arr + index;

            ZXIC_COMM_TRACE_NOTICE("l2 entry index:0x%x\n", index);
            dpp_l2_entry_print(p_l2_temp_entry);

            //从l2中获取mac地址给mac_info，再转换vport信息
            ZXIC_COMM_MEMCPY(p_temp_mac_info->addr, p_l2_temp_entry->key.dmac_addr, 6);
            current_vqm_vfid = p_l2_temp_entry->entry.vqm_vfid;

            p_temp_mac_info->sriov_vlan_tpid = p_l2_temp_entry->key.sriov_vlan_tpid;
            p_temp_mac_info->sriov_vlan_id = p_l2_temp_entry->key.sriov_vlan_id;

            rc = dpp_vport_get_by_vqm_vfid(OWNER_PF_VPORT(pf_info->vport),  current_vqm_vfid, &current_vport);
            ZXIC_COMM_CHECK_RC_MEMORY_VFREE_UNLOCK_NO_ASSERT(rc, "dpp_vport_get_by_vqm_vfid", pL2DataArr, DEV_PCIE_LOCK(&dev));
            p_temp_mac_info->vport = current_vport;
            ZXIC_COMM_TRACE_NOTICE("current_vqm_vfid:0x%x --> current_vport:0x%x\n", current_vqm_vfid, current_vport);
        }
    }

    *p_mac_num = entryNum;

    ZXIC_COMM_VFREE(pL2DataArr);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_unicast_mac_dump);

ZXIC_UINT32 dpp_unicast_mac_transfer(DPP_PF_INFO_T* pf_info, DPP_PF_INFO_T* new_pf_info)
{
    DPP_DEV_T dev = {0};
    DPP_DEV_T new_dev = {0};
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 hash_index = 0;
    ZXIC_UINT32 sdt_no = 0;
    ZXIC_UINT32 current_vqm_vfid = 0;
    ZXIC_UINT16 current_vport = 0;

    ZXIC_UINT32 max_item_num = DTB_DUMP_UNICAST_MAC_DUMP_NUM;
    ZXIC_UINT32 entryNum = 0;
    ZXIC_UINT32 transfer_num = 0;
    ZXDH_L2_ENTRY_T *pL2DataArr = NULL;
    ZXDH_L2_ENTRY_T *pL2DataArrNew = NULL;
    ZXDH_L2_ENTRY_T *p_l2_temp_entry = NULL;
    ZXDH_L2_ENTRY_T *p_l2_entry_new = NULL;
    ZXIC_UINT32 rc     = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(new_pf_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dev_get(new_pf_info, &new_dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    if((0 != ZXIC_COMM_MEMCMP(&dev, &new_dev,sizeof(DPP_DEV_T)))
        || IS_PF(pf_info->vport) || IS_PF(new_pf_info->vport))
    {
        ZXIC_COMM_TRACE_ERROR("current slot[0x%x] vport[0x%x] & new slot[0x%x] vport[0x%x] belong to different pf or slot\n",
                               dev.pcie_channel.slot,
                               dev.pcie_channel.vport,
                               new_dev.pcie_channel.slot,
                               new_dev.pcie_channel.vport);
        ZXIC_COMM_TRACE_ERROR("current vport is %s, transfre vport is %s\n",
                               IS_PF(pf_info->vport) ? "PF":"VF",
                               IS_PF(new_pf_info->vport) ? "PF":"VF");
        return DPP_ERR;
    }

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_hash_index_get(pf_info, &hash_index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_hash_index_get");
    sdt_no = ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT0 + hash_index;

    rc = dpp_hash_max_item_num_get(&dev,sdt_no,&max_item_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_hash_max_item_num_get");

    pL2DataArr = (ZXDH_L2_ENTRY_T *)ZXIC_COMM_VMALLOC(max_item_num * sizeof(ZXDH_L2_ENTRY_T));
    ZXIC_COMM_CHECK_POINT_NO_ASSERT(pL2DataArr);

    pL2DataArrNew = (ZXDH_L2_ENTRY_T *)ZXIC_COMM_VMALLOC(max_item_num * sizeof(ZXDH_L2_ENTRY_T));
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE_NO_ASSERT(pL2DataArrNew, pL2DataArr);

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE2PTR_NO_ASSERT(rc, "dpp_vport_table_lock", pL2DataArrNew, pL2DataArr);
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE2PTR_NO_ASSERT(DEV_PCIE_LOCK(&dev), pL2DataArrNew, pL2DataArr);

    rc = dpp_apt_dtb_hash_table_unicast_mac_dump(&dev, queue, sdt_no, pL2DataArr, &entryNum);
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE2PTR_UNLOCK_NO_ASSERT(rc, "dpp_apt_dtb_hash_table_unicast_mac_dump", pL2DataArrNew, pL2DataArr, DEV_PCIE_LOCK(&dev));

    for(index = 0; index < entryNum; index++)
    {
        p_l2_temp_entry = pL2DataArr + index;
        current_vqm_vfid = p_l2_temp_entry->entry.vqm_vfid;
        rc = dpp_vport_get_by_vqm_vfid(OWNER_PF_VPORT(pf_info->vport),  current_vqm_vfid, &current_vport);
        ZXIC_COMM_CHECK_RC_MEMORY_VFREE2PTR_UNLOCK_NO_ASSERT(rc, "dpp_vport_get_by_vqm_vfid", pL2DataArrNew, pL2DataArr, DEV_PCIE_LOCK(&dev));
        ZXIC_COMM_TRACE_NOTICE("[%u] slot[%u],vport[0x%x] cur_vqm_vfid[%u] cur_vqm_vport[0x%x]\n",
                                index, pf_info->slot,pf_info->vport,current_vqm_vfid, current_vport);
        if(pf_info->vport==current_vport)
        {
            p_l2_entry_new = pL2DataArrNew + transfer_num;
            ZXIC_COMM_MEMCPY_S(p_l2_entry_new,sizeof(ZXDH_L2_ENTRY_T),p_l2_temp_entry,sizeof(ZXDH_L2_ENTRY_T));
            p_l2_entry_new->entry.vqm_vfid = VQM_VFID(new_pf_info->vport);
            ZXIC_COMM_TRACE_NOTICE("[%u]:new_vport=0x%x new_vqm_vfid=0x%x\n",
                            transfer_num, new_pf_info->vport,p_l2_entry_new->entry.vqm_vfid);
            transfer_num++;
        }
    }

    if(transfer_num==0)
    {
        ZXIC_COMM_VFREE(pL2DataArrNew);
        ZXIC_COMM_VFREE(pL2DataArr);
        ZXIC_COMM_TRACE_NOTICE("transfer num is 0!\n");
        rc = dpp_vport_table_unlock(pf_info, sdt_no);
        ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

        return DPP_OK;
    }

    rc = dpp_apt_dtb_multi_hash_insert(&dev, queue, sdt_no, transfer_num,sizeof(ZXDH_L2_ENTRY_T),pL2DataArrNew);
    ZXIC_COMM_VFREE(pL2DataArrNew);
    ZXIC_COMM_VFREE(pL2DataArr);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_multi_hash_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_unicast_mac_transfer);

ZXIC_UINT32 dpp_unicast_all_mac_delete(DPP_PF_INFO_T* pf_info)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 sdt_no = 0;
    ZXIC_UINT32 hash_index = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x start.\n", pf_info->slot, pf_info->vport);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_hash_index_get(pf_info, &hash_index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_hash_index_get");

    sdt_no = ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT0 + hash_index;

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_hash_offline_delete(&dev, queue, sdt_no);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_hash_offline_delete", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x success.\n", pf_info->slot, pf_info->vport);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_unicast_all_mac_delete);

ZXIC_UINT32 dpp_unicast_all_mac_online_delete(DPP_PF_INFO_T* pf_info)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 sdt_no = 0;
    ZXIC_UINT32 hash_index = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x start.\n", pf_info->slot, pf_info->vport);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_hash_index_get(pf_info, &hash_index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_hash_index_get");

    sdt_no = ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT0 + hash_index;

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_hash_online_delete(&dev, queue, sdt_no);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_hash_online_delete", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x success.\n", pf_info->slot, pf_info->vport);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_unicast_all_mac_online_delete);

ZXIC_UINT32 dpp_unicast_mac_max_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *max_num)
{
    ZXIC_UINT32 rc         = DPP_OK;
    ZXIC_UINT32 sdt_no     = 0;
    ZXIC_UINT32 hash_index = 0;

    DPP_DEV_T dev                         = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(max_num);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x start.\n", pf_info->slot, pf_info->vport);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_vport_hash_index_get(pf_info, &hash_index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_hash_index_get");

    sdt_no = ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT0 + hash_index;

    rc = dpp_hash_max_item_num_get(&dev, sdt_no, max_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_hash_max_item_num_get");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x max_num: 0x%x get succ.\n",
                            pf_info->slot, pf_info->vport, *max_num);
    return DPP_OK;
}
EXPORT_SYMBOL(dpp_unicast_mac_max_get);

ZXIC_UINT32 dpp_unicast_all_mac_soft_delete(DPP_PF_INFO_T* pf_info)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 sdt_no = 0;
    ZXIC_UINT32 hash_index = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x start.\n", pf_info->slot, pf_info->vport);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_soft_hash_index_get(&dev, &hash_index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_soft_hash_index_get");
    sdt_no = ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT0 + hash_index;

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_hash_soft_delete_by_sdt(&dev, sdt_no);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_hash_soft_delete_by_sdt", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt:%u success.\n", pf_info->slot, pf_info->vport, sdt_no);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_unicast_all_mac_soft_delete);
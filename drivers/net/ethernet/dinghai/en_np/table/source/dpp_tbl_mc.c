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
#include "dpp_tbl_mc.h"
#include "dpp_tbl_bc.h"
#include "dpp_tbl_api.h"
#include "dpp_sdt.h"

ZXIC_VOID dpp_mc_entry_print(ZXDH_MC_T *mc_entry)
{
    ZXIC_COMM_TRACE_NOTICE("key--mc_mac: %02x:%02x:%02x:%02x:%02x:%02x.\n",
                               mc_entry->key.mc_mac[0], mc_entry->key.mc_mac[1],
                               mc_entry->key.mc_mac[2], mc_entry->key.mc_mac[3],
                               mc_entry->key.mc_mac[4], mc_entry->key.mc_mac[5]);
    ZXIC_COMM_TRACE_NOTICE("key--group_id: 0x%02x\n", mc_entry->key.group_id);
    ZXIC_COMM_TRACE_NOTICE("key--rsv: 0x%02x\n", mc_entry->key.rsv);

    ZXIC_COMM_TRACE_NOTICE("entry--mc_bitmap: 0x%016llx\n", mc_entry->entry.mc_bitmap);
    ZXIC_COMM_TRACE_NOTICE("entry--rsv2: 0x%02x\n", mc_entry->entry.rsv2);
    ZXIC_COMM_TRACE_NOTICE("entry--rsv1: 0x%02x\n", mc_entry->entry.rsv1);
    ZXIC_COMM_TRACE_NOTICE("entry--mc_pf_enable: 0x%02x\n", mc_entry->entry.mc_pf_enable);
    ZXIC_COMM_TRACE_NOTICE("entry--hit_flag: 0x%02x\n", mc_entry->entry.hit_flag);
}

ZXIC_UINT32 dpp_vport_mc_info_add(DPP_PF_INFO_T* pf_info, ZXIC_CONST ZXIC_VOID *mac, DPP_VPORT_MC_INFO_T *mc_info)
{
    ZXIC_UINT32 index     = 0;
    ZXIC_UINT32 group_id  = 0;
    ZXIC_UINT32 first_free_flag = 0;
    ZXIC_UINT32 find_flag = 0;
    ZXIC_UINT32 mc_index = 0;
    ZXIC_UINT32 vfunc_num = 0;
    ZXIC_UINT32 rc        = DPP_OK;
    DPP_VPORT_MC_TABLE_T* mc_table = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(mac);
    ZXIC_COMM_CHECK_POINT(mc_info);

    rc = dpp_vport_mc_table_get(pf_info, &mc_table);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_mc_table_get");
    ZXIC_COMM_CHECK_POINT(mc_table);

    vfunc_num = VFUNC_NUM(pf_info->vport);
    ZXIC_COMM_CHECK_INDEX(vfunc_num, 0, (MC_GROUP_NUM * MC_MEMBER_NUM_IN_GROUP) - 1);

    group_id  = vfunc_num / MC_MEMBER_NUM_IN_GROUP;
    ZXIC_COMM_CHECK_INDEX(group_id, 0, MC_GROUP_NUM - 1);

    for (index = 0; index < MC_TABLE_SIZE; index++)
    {
        if ((first_free_flag==0) && (mc_table->mc_info[index].is_valid == 0))
        {
            first_free_flag = 1;
            mc_index = index;
        }

        /*update*/
        if((mc_table->mc_info[index].is_valid==1) &&
            (ZXIC_COMM_MEMCMP(mc_table->mc_info[index].mac, mac, 6) == 0))
        {
            find_flag = 1;
            mc_index = index;
            break;
        }
    }

    if((first_free_flag==0) && (find_flag==0))
    {
        return DPP_RC_TBL_IS_FULL;
    }

    mc_table->mc_info[mc_index].is_valid = 1;
    ZXIC_COMM_MEMCPY(mc_table->mc_info[mc_index].mac, mac, 6);
    if (IS_PF(pf_info->vport))
    {
        mc_table->mc_info[mc_index].mc_pf_enable = 1;
    }
    else
    {
        mc_table->mc_info[mc_index].mc_bitmap[group_id] |= ((ZXIC_UINT64)(1) << (MC_MEMBER_NUM_IN_GROUP - 1 -
                                                                                 (vfunc_num % MC_MEMBER_NUM_IN_GROUP)));
    }
    ZXIC_COMM_MEMCPY_S(mc_info,sizeof(DPP_VPORT_MC_INFO_T),&mc_table->mc_info[mc_index],sizeof(DPP_VPORT_MC_INFO_T));

    ZXIC_COMM_TRACE_NOTICE("mc_index:%u mc_bitmap:0x%016llx\n",mc_index, mc_table->mc_info[mc_index].mc_bitmap[group_id]);

    return DPP_OK;

}
ZXIC_UINT32 dpp_vport_mc_info_del(DPP_PF_INFO_T* pf_info, ZXIC_CONST ZXIC_VOID *mac)
{
    ZXIC_UINT32 index     = 0;
    ZXIC_UINT32 group_id  = 0;
    ZXIC_UINT32 vfunc_num = 0;
    ZXIC_UINT32 rc        = DPP_OK;

    DPP_VPORT_MC_TABLE_T* mc_table = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(mac);

    rc = dpp_vport_mc_table_get(pf_info, &mc_table);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_mc_table_get");
    ZXIC_COMM_CHECK_POINT(mc_table);

    vfunc_num = VFUNC_NUM(pf_info->vport);
    ZXIC_COMM_CHECK_INDEX(vfunc_num, 0, (MC_GROUP_NUM * MC_MEMBER_NUM_IN_GROUP) - 1);

    group_id  = vfunc_num / MC_MEMBER_NUM_IN_GROUP;
    ZXIC_COMM_CHECK_INDEX(group_id, 0, MC_GROUP_NUM - 1);

    for (index = 0; index < MC_TABLE_SIZE; index++)
    {
        if (mc_table->mc_info[index].is_valid == 0)
        {
            continue;
        }
        if (ZXIC_COMM_MEMCMP(mc_table->mc_info[index].mac, mac, 6) == 0)
        {
            if (IS_PF(pf_info->vport))
            {
                mc_table->mc_info[index].mc_pf_enable = 0;
            }
            else
            {
                mc_table->mc_info[index].mc_bitmap[group_id] &= ~((ZXIC_UINT64)(1) << (MC_MEMBER_NUM_IN_GROUP - 1 -
                                                                                  (vfunc_num % MC_MEMBER_NUM_IN_GROUP)));
            }

            if ((mc_table->mc_info[index].mc_bitmap[0] == 0) && (mc_table->mc_info[index].mc_bitmap[1] == 0) &&
                (mc_table->mc_info[index].mc_bitmap[2] == 0) && (mc_table->mc_info[index].mc_bitmap[3] == 0) &&
                (mc_table->mc_info[index].mc_pf_enable == 0))
            {
                mc_table->mc_info[index].is_valid = 0;
                ZXIC_COMM_MEMSET(mc_table->mc_info[index].mac, 0x00, 6);
            }

            break;
        }
    }

    return DPP_OK;
}

ZXIC_UINT32 dpp_vport_mc_info_update(ZXIC_UINT32 slot, ZXIC_UINT32 vport, ZXIC_UINT32 new_vport,
                                     ZXIC_CONST ZXIC_VOID *mac, DPP_VPORT_MC_INFO_T *mc_info)
{
    ZXIC_UINT32 rc            = DPP_OK;
    ZXIC_UINT32 index         = 0;
    ZXIC_UINT32 group_id      = 0;
    ZXIC_UINT32 vfunc_num     = 0;
    ZXIC_UINT32 new_group_id  = 0;
    ZXIC_UINT32 new_vfunc_num = 0;

    DPP_PF_INFO_T pf_info          = {slot,vport};
    DPP_PF_INFO_T new_pf_info      = {slot,new_vport};
    DPP_VPORT_MC_TABLE_T* mc_table = NULL;

    ZXIC_COMM_CHECK_POINT(mac);
    ZXIC_COMM_CHECK_POINT(mc_info);

    rc = dpp_vport_mc_table_get(&pf_info, &mc_table);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_mc_table_get");
    ZXIC_COMM_CHECK_POINT(mc_table);

    vfunc_num = VFUNC_NUM(pf_info.vport);
    ZXIC_COMM_CHECK_INDEX(vfunc_num, 0, (MC_GROUP_NUM * MC_MEMBER_NUM_IN_GROUP) - 1);
    group_id  = vfunc_num / MC_MEMBER_NUM_IN_GROUP;
    ZXIC_COMM_CHECK_INDEX(group_id, 0, MC_GROUP_NUM - 1);

    new_vfunc_num = VFUNC_NUM(new_pf_info.vport);
    ZXIC_COMM_CHECK_INDEX(new_vfunc_num, 0, (MC_GROUP_NUM * MC_MEMBER_NUM_IN_GROUP) - 1);
    new_group_id  = new_vfunc_num / MC_MEMBER_NUM_IN_GROUP;
    ZXIC_COMM_CHECK_INDEX(new_group_id, 0, MC_GROUP_NUM - 1);

    ZXIC_COMM_TRACE_NOTICE("vfunc_num:%u,group_id:%u,new_vfunc_num:%u,new_group_id:%u\n",
                            vfunc_num,group_id,new_vfunc_num,new_group_id);

    for (index = 0; index < MC_TABLE_SIZE; index++)
    {
        if((mc_table->mc_info[index].is_valid==1) &&
            (ZXIC_COMM_MEMCMP(mc_table->mc_info[index].mac, mac, 6) == 0))
        {
            if (IS_PF(new_pf_info.vport))
            {
                mc_table->mc_info[index].mc_pf_enable = 1;
            }
            else
            {
                ZXIC_COMM_TRACE_NOTICE("[%u] mc_bitmap=0x%016llx\n",index,mc_table->mc_info[index].mc_bitmap[group_id]);
                mc_table->mc_info[index].mc_bitmap[group_id] &= ~((ZXIC_UINT64)(1) << (MC_MEMBER_NUM_IN_GROUP - 1 -
                                                                                  (vfunc_num % MC_MEMBER_NUM_IN_GROUP)));
                ZXIC_COMM_TRACE_NOTICE("mc_bitmap(del vport)=0x%016llx\n",mc_table->mc_info[index].mc_bitmap[new_group_id]);
                mc_table->mc_info[index].mc_bitmap[new_group_id] |= ((ZXIC_UINT64)(1) << (MC_MEMBER_NUM_IN_GROUP - 1 -
                                                                   (new_vfunc_num % MC_MEMBER_NUM_IN_GROUP)));
                ZXIC_COMM_TRACE_NOTICE("mc_bitmap(add new vport)=0x%016llx\n",mc_table->mc_info[index].mc_bitmap[new_group_id]);
            }
            ZXIC_COMM_MEMCPY_S(mc_info,sizeof(DPP_VPORT_MC_INFO_T),&mc_table->mc_info[index],sizeof(DPP_VPORT_MC_INFO_T));

            ZXIC_COMM_TRACE_NOTICE("update success !");
            return DPP_OK;
        }
    }

    ZXIC_COMM_TRACE_ERROR("update fail !");
    return DPP_ERR;
}

ZXIC_UINT32 dpp_vport_mc_table_insert(DPP_PF_INFO_T* pf_info, ZXIC_CONST ZXIC_VOID *mac)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 index      = 0;
    ZXIC_UINT32 sdt_no     = 0;
    ZXIC_UINT32 queue      = 0;
    ZXIC_UINT32 group_id   = 0;
    ZXIC_UINT32 hash_index = 0;
    ZXIC_UINT32 rc         = DPP_OK;

    ZXDH_MC_T mc_entry = {0};
    DPP_VPORT_MC_TABLE_T* mc_table = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(mac);

    ZXIC_COMM_MEMSET(&mc_entry, 0, sizeof(ZXDH_MC_T));

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_hash_index_get(pf_info, &hash_index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_hash_index_get");

    sdt_no = ZXDH_SDT_MC_TABLE_PHYPORT0 + hash_index;

    rc = dpp_vport_mc_table_get(pf_info, &mc_table);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_mc_table_get");
    ZXIC_COMM_CHECK_POINT(mc_table);

    for (index = 0; index < MC_TABLE_SIZE; index++)
    {
        if (mc_table->mc_info[index].is_valid == 0)
        {
            continue;
        }
        if (ZXIC_COMM_MEMCMP(mc_table->mc_info[index].mac, mac, 6) != 0)
        {
            continue;
        }
        for (group_id = 0; group_id < MC_GROUP_NUM; group_id++)
        {
            ZXIC_COMM_MEMCPY(mc_entry.key.mc_mac, mac, 6);
            mc_entry.key.group_id = group_id;
            mc_entry.entry.hit_flag = 0x00;
            mc_entry.entry.mc_pf_enable = mc_table->mc_info[index].mc_pf_enable;
            mc_entry.entry.mc_bitmap = mc_table->mc_info[index].mc_bitmap[group_id];

            rc = dpp_apt_dtb_hash_insert(&dev, queue, sdt_no, &mc_entry);
            ZXIC_COMM_CHECK_RC(rc, "dpp_apt_dtb_hash_insert");

            ZXIC_COMM_TRACE_NOTICE("slot %u vport: 0x%04x sdt_no: %u group_id: %u.\n",
                                                pf_info->slot, pf_info->vport, sdt_no, group_id);
            ZXIC_COMM_TRACE_NOTICE("mac: %02x:%02x:%02x:%02x:%02x:%02x.\n",
                                                                mc_entry.key.mc_mac[0], mc_entry.key.mc_mac[1],
                                                                mc_entry.key.mc_mac[2], mc_entry.key.mc_mac[3],
                                                                mc_entry.key.mc_mac[4], mc_entry.key.mc_mac[5]);
            ZXIC_COMM_TRACE_NOTICE("mc_bitmap: %016llx.\n", mc_entry.entry.mc_bitmap);
        }
        return DPP_OK;
    }

    for (group_id = 0; group_id < MC_GROUP_NUM; group_id++)
    {
        ZXIC_COMM_MEMCPY(mc_entry.key.mc_mac, mac, 6);
        mc_entry.key.group_id = group_id;

        rc = dpp_apt_dtb_hash_delete(&dev, queue, sdt_no, &mc_entry);
        ZXIC_COMM_CHECK_RC(rc, "dpp_apt_dtb_hash_delete");

        ZXIC_COMM_TRACE_NOTICE("delete mc table.\n");
        ZXIC_COMM_TRACE_NOTICE("slot %u vport: 0x%04x sdt_no: %u group_id: %u.\n",
                                                pf_info->slot, pf_info->vport, sdt_no, group_id);
        ZXIC_COMM_TRACE_NOTICE("mac: %02x:%02x:%02x:%02x:%02x:%02x.\n",
                                                                mc_entry.key.mc_mac[0], mc_entry.key.mc_mac[1],
                                                                mc_entry.key.mc_mac[2], mc_entry.key.mc_mac[3],
                                                                mc_entry.key.mc_mac[4], mc_entry.key.mc_mac[5]);
    }

    return DPP_OK;
}

ZXIC_UINT32 dpp_multi_mac_add_member(DPP_PF_INFO_T* pf_info, ZXIC_CONST ZXIC_VOID *mac)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue = 0;
    ZXIC_UINT32 sdt_no = 0;
    ZXIC_UINT32 hash_index = 0;
    ZXIC_UINT32 rc = DPP_OK;
    DPP_VPORT_MC_INFO_T mc_info = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x start.\n", pf_info->slot, pf_info->vport);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_hash_index_get(pf_info, &hash_index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_hash_index_get");

    sdt_no = ZXDH_SDT_MC_TABLE_PHYPORT0 + hash_index;

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_mc_info_add(pf_info, mac, &mc_info);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_vport_mc_info_add", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_mc_table_insert(pf_info, mac);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_vport_mc_table_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x success.\n", pf_info->slot, pf_info->vport);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_multi_mac_add_member);

ZXIC_UINT32 dpp_multi_mac_del_member(DPP_PF_INFO_T* pf_info, ZXIC_CONST ZXIC_VOID *mac)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue = 0;
    ZXIC_UINT32 sdt_no = 0;
    ZXIC_UINT32 hash_index = 0;
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x start.\n", pf_info->slot, pf_info->vport);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_hash_index_get(pf_info, &hash_index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_hash_index_get");

    sdt_no = ZXDH_SDT_MC_TABLE_PHYPORT0 + hash_index;

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_mc_info_del(pf_info, mac);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_vport_mc_info_del", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_mc_table_insert(pf_info, mac);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_vport_mc_table_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x success.\n", pf_info->slot, pf_info->vport);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_multi_mac_del_member);
ZXIC_UINT32 dpp_batch_add_multicast_mac(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 mac_num, ZXIC_CONST ZXIC_VOID *mac)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue       = 0;
    ZXIC_UINT32 sdt_no      = 0;
    ZXIC_UINT32 hash_index  = 0;
    ZXIC_UINT32 group_index = 0;
    ZXIC_UINT32 entry_index = 0;
    ZXIC_UINT32 rc          = DPP_OK;

    ZXDH_MC_T *p_batch_entry = NULL;
    ZXDH_MC_T *p_one_entry = NULL;
    ZXIC_UINT8 *p_mac = NULL;
    DPP_VPORT_MC_INFO_T mc_info = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(mac);
    ZXIC_COMM_CHECK_INDEX_LOWER(mac_num,1);

    p_batch_entry = (ZXDH_MC_T *)ZXIC_COMM_MALLOC(MC_GROUP_NUM * mac_num * sizeof(ZXDH_MC_T));
    ZXIC_COMM_CHECK_POINT(p_batch_entry);
    ZXIC_COMM_MEMSET_S(p_batch_entry, MC_GROUP_NUM * mac_num * sizeof(ZXDH_MC_T), 0, MC_GROUP_NUM * mac_num * sizeof(ZXDH_MC_T));

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_dev_get", p_batch_entry);

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_dtb_queue_id_get", p_batch_entry);

    rc = dpp_vport_hash_index_get(pf_info, &hash_index);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_vport_hash_index_get", p_batch_entry);

    sdt_no = ZXDH_SDT_MC_TABLE_PHYPORT0 + hash_index;

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_vport_table_lock", p_batch_entry);
    ZXIC_COMM_CHECK_POINT_MEMORY_FREE(DEV_PCIE_LOCK(&dev),p_batch_entry);

    for(entry_index = 0;entry_index<mac_num;entry_index++)
    {
        p_mac = (ZXIC_UINT8 *)mac+entry_index*6;
        rc = dpp_vport_mc_info_add(pf_info, p_mac, &mc_info);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE_UNLOCK_NO_ASSERT(rc, "dpp_vport_mc_info_add", p_batch_entry, DEV_PCIE_LOCK(&dev));

        for(group_index=0;group_index<MC_GROUP_NUM;group_index++)
        {
            p_one_entry = p_batch_entry + MC_GROUP_NUM * entry_index + group_index;
            p_one_entry->entry.hit_flag = 0x00;
            p_one_entry->entry.mc_bitmap = mc_info.mc_bitmap[group_index];
            p_one_entry->entry.mc_pf_enable = mc_info.mc_pf_enable;
            p_one_entry->key.group_id = group_index;
            ZXIC_COMM_MEMCPY_S(p_one_entry->key.mc_mac, 6, p_mac, 6);
            ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x group_id: %u mac: %02x:%02x:%02x:%02x:%02x:%02x bitmap:0x%016llx start.\n",
                                    pf_info->slot, pf_info->vport,p_one_entry->key.group_id,
                                    p_one_entry->key.mc_mac[0], p_one_entry->key.mc_mac[1],
                                    p_one_entry->key.mc_mac[2], p_one_entry->key.mc_mac[3],
                                    p_one_entry->key.mc_mac[4], p_one_entry->key.mc_mac[5],
                                    p_one_entry->entry.mc_bitmap);
        }
    }

    rc = dpp_apt_dtb_multi_hash_insert(&dev, queue, sdt_no, MC_GROUP_NUM*mac_num,sizeof(ZXDH_MC_T),p_batch_entry);
    ZXIC_COMM_FREE(p_batch_entry);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_hash_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x success.\n", pf_info->slot, pf_info->vport);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_batch_add_multicast_mac);

ZXIC_UINT32 dpp_batch_del_multicast_mac(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 mac_num, ZXIC_CONST ZXIC_VOID *mac)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue       = 0;
    ZXIC_UINT32 sdt_no      = 0;
    ZXIC_UINT32 hash_index  = 0;
    ZXIC_UINT32 entry_index = 0;
    ZXIC_UINT32 rc          = DPP_OK;

    ZXIC_UINT8 *p_mac = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(mac);
    ZXIC_COMM_CHECK_INDEX_LOWER(mac_num,1);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_hash_index_get(pf_info, &hash_index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_hash_index_get");

    sdt_no = ZXDH_SDT_MC_TABLE_PHYPORT0 + hash_index;

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    for(entry_index = 0;entry_index<mac_num;entry_index++)
    {
        p_mac = (ZXIC_UINT8 *)mac+entry_index*6;
        rc = dpp_vport_mc_info_del(pf_info, p_mac);
        ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_vport_mc_info_del", DEV_PCIE_LOCK(&dev));

        rc = dpp_vport_mc_table_insert(pf_info, p_mac);
        ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_vport_mc_table_insert", DEV_PCIE_LOCK(&dev));
    }

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x success.\n", pf_info->slot, pf_info->vport);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_batch_del_multicast_mac);

/*vport belong to the same group*/
ZXIC_UINT32 dpp_multicast_mac_transfer_intra_group(DPP_DEV_T *dev, ZXIC_UINT32 slot, ZXIC_UINT32 vport, ZXIC_UINT32 new_vport)
{
    ZXIC_UINT32 rc                   = DPP_OK;
    ZXIC_UINT32 queue                = 0;
    ZXIC_UINT32 hash_index           = 0;
    ZXIC_UINT32 index                = 0;
    ZXIC_UINT32 sdt_no               = 0;
    ZXIC_UINT32 group_id             = 0;
    ZXIC_UINT32 max_item_num         = DTB_DUMP_MULTICAST_MAC_DUMP_NUM;
    ZXIC_UINT32 entryNum             = 0;
    ZXIC_UINT32 transfer_num         = 0;
    ZXDH_MC_T *pMcDataArr            = NULL;
    ZXDH_MC_T *pMcDataArrNew         = NULL;
    ZXDH_MC_T *p_mc_temp_entry       = NULL;
    ZXDH_MC_T *p_mc_entry_new        = NULL;

    DPP_PF_INFO_T pf_info            = {slot,vport};
    DPP_VPORT_MC_INFO_T mc_info      = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    rc = dpp_dtb_queue_id_get(dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_hash_index_get(&pf_info, &hash_index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_hash_index_get");
    sdt_no = ZXDH_SDT_MC_TABLE_PHYPORT0 + hash_index;

    rc = dpp_hash_max_item_num_get(dev,sdt_no,&max_item_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_hash_max_item_num_get");

    group_id  = VFUNC_NUM(pf_info.vport) / MC_MEMBER_NUM_IN_GROUP;
    ZXIC_COMM_CHECK_INDEX(group_id, 0, MC_GROUP_NUM - 1);

    pMcDataArr = (ZXDH_MC_T *)ZXIC_COMM_VMALLOC(max_item_num * sizeof(ZXDH_MC_T));
    ZXIC_COMM_CHECK_POINT_NO_ASSERT(pMcDataArr);

    pMcDataArrNew = (ZXDH_MC_T *)ZXIC_COMM_VMALLOC(max_item_num * sizeof(ZXDH_MC_T));
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE_NO_ASSERT(pMcDataArrNew, pMcDataArr);

    rc = dpp_vport_table_lock(&pf_info, sdt_no, &DEV_PCIE_LOCK(dev));
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE2PTR_NO_ASSERT(rc, "dpp_vport_table_lock", pMcDataArrNew, pMcDataArr);
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE2PTR_NO_ASSERT(DEV_PCIE_LOCK(dev), pMcDataArrNew, pMcDataArr);

    rc = dpp_apt_dtb_hash_table_multicast_mac_dump(dev, queue, sdt_no, pMcDataArr, &entryNum);
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE2PTR_UNLOCK_NO_ASSERT(rc, "dpp_apt_dtb_hash_table_unicast_mac_dump", pMcDataArrNew, pMcDataArr, DEV_PCIE_LOCK(dev));

    for(index = 0; index < entryNum; index++)
    {
        p_mc_temp_entry = pMcDataArr + index;
        if(TRUE==dpp_vport_in_mc_bitmap(vport,p_mc_temp_entry->entry.mc_bitmap))
        {
            rc = dpp_vport_mc_info_update(slot,vport,new_vport,p_mc_temp_entry->key.mc_mac,&mc_info);
            ZXIC_COMM_CHECK_RC_MEMORY_VFREE2PTR_UNLOCK_NO_ASSERT(rc, "dpp_vport_mc_info_update",
                                                            pMcDataArrNew, pMcDataArr, DEV_PCIE_LOCK(dev));
            p_mc_entry_new = pMcDataArrNew + transfer_num;
            ZXIC_COMM_MEMCPY_S(p_mc_entry_new,sizeof(ZXDH_MC_T),p_mc_temp_entry,sizeof(ZXDH_MC_T));
            p_mc_entry_new->entry.hit_flag = 0;
            p_mc_entry_new->entry.mc_pf_enable = mc_info.mc_pf_enable;
            p_mc_entry_new->entry.mc_bitmap = mc_info.mc_bitmap[group_id];
            ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x new_vport: 0x%04x mac: %02x:%02x:%02x:%02x:%02x:%02x start.\n",
                                    slot,vport,new_vport,
                                    p_mc_entry_new->key.mc_mac[0], p_mc_entry_new->key.mc_mac[1],
                                    p_mc_entry_new->key.mc_mac[2], p_mc_entry_new->key.mc_mac[3],
                                    p_mc_entry_new->key.mc_mac[4], p_mc_entry_new->key.mc_mac[5]);
            ZXIC_COMM_TRACE_NOTICE("group_id: %u mc_bitmap: 0x%016llx new_group_id: %u new_mc_bitmap: 0x%016llx\n",
                                p_mc_temp_entry->key.group_id,p_mc_temp_entry->entry.mc_bitmap,
                                p_mc_entry_new->key.group_id, p_mc_entry_new->entry.mc_bitmap);
            transfer_num++;
        }
    }

    if(transfer_num==0)
    {
        ZXIC_COMM_VFREE(pMcDataArrNew);
        ZXIC_COMM_VFREE(pMcDataArr);
        ZXIC_COMM_TRACE_NOTICE("transfer num is 0!\n");
        rc = dpp_vport_table_unlock(&pf_info, sdt_no);
        ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

        return DPP_OK;
    }

    rc = dpp_apt_dtb_multi_hash_insert(dev, queue, sdt_no, transfer_num,sizeof(ZXDH_MC_T),pMcDataArrNew);
    ZXIC_COMM_VFREE(pMcDataArrNew);
    ZXIC_COMM_VFREE(pMcDataArr);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_multi_hash_insert", DEV_PCIE_LOCK(dev));

    rc = dpp_vport_table_unlock(&pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}

/*vport belong to the different group*/
ZXIC_UINT32 dpp_multicast_mac_transfer_inter_group(DPP_DEV_T *dev, ZXIC_UINT32 slot, ZXIC_UINT32 vport, ZXIC_UINT32 new_vport)
{
    ZXIC_UINT32 rc                   = DPP_OK;
    ZXIC_UINT32 queue                = 0;
    ZXIC_UINT32 hash_index           = 0;
    ZXIC_UINT32 index                = 0;
    ZXIC_UINT32 sdt_no               = 0;
    ZXIC_UINT32 group_id             = 0;
    ZXIC_UINT32 new_group_id         = 0;
    ZXIC_UINT32 max_item_num         = DTB_DUMP_MULTICAST_MAC_DUMP_NUM;
    ZXIC_UINT32 entryNum             = 0;
    ZXDH_MC_T *pMcDataArr            = NULL;
    ZXDH_MC_T *pMcDataArrNew         = NULL;
    ZXDH_MC_T *p_mc_temp_entry       = NULL;
    ZXDH_MC_T *p_mc_entry_new        = NULL;

    DPP_PF_INFO_T pf_info            = {slot,vport};
    DPP_VPORT_MC_INFO_T mc_info      = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    rc = dpp_dtb_queue_id_get(dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_hash_index_get(&pf_info, &hash_index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_hash_index_get");
    sdt_no = ZXDH_SDT_MC_TABLE_PHYPORT0 + hash_index;

    rc = dpp_hash_max_item_num_get(dev,sdt_no,&max_item_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_hash_max_item_num_get");

    group_id  = VFUNC_NUM(pf_info.vport) / MC_MEMBER_NUM_IN_GROUP;
    ZXIC_COMM_CHECK_INDEX(group_id, 0, MC_GROUP_NUM - 1);

    new_group_id  = VFUNC_NUM(new_vport) / MC_MEMBER_NUM_IN_GROUP;
    ZXIC_COMM_CHECK_INDEX(new_group_id, 0, MC_GROUP_NUM - 1);

    pMcDataArr = (ZXDH_MC_T *)ZXIC_COMM_VMALLOC(max_item_num * sizeof(ZXDH_MC_T));
    ZXIC_COMM_CHECK_POINT_NO_ASSERT(pMcDataArr);

    pMcDataArrNew = (ZXDH_MC_T *)ZXIC_COMM_VMALLOC(max_item_num * sizeof(ZXDH_MC_T));
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE_NO_ASSERT(pMcDataArrNew, pMcDataArr);

    rc = dpp_vport_table_lock(&pf_info, sdt_no, &DEV_PCIE_LOCK(dev));
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE2PTR_NO_ASSERT(rc, "dpp_vport_table_lock", pMcDataArrNew, pMcDataArr);
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE2PTR_NO_ASSERT(DEV_PCIE_LOCK(dev), pMcDataArrNew, pMcDataArr);

    rc = dpp_apt_dtb_hash_table_multicast_mac_dump(dev, queue, sdt_no, pMcDataArr, &entryNum);
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE2PTR_UNLOCK_NO_ASSERT(rc, "dpp_apt_dtb_hash_table_unicast_mac_dump", pMcDataArrNew, pMcDataArr, DEV_PCIE_LOCK(dev));

    for(index = 0; index < entryNum; index++)
    {
        p_mc_temp_entry = pMcDataArr + index;
        p_mc_entry_new = pMcDataArrNew + index;
        ZXIC_COMM_MEMCPY_S(p_mc_entry_new,sizeof(ZXDH_MC_T),p_mc_temp_entry,sizeof(ZXDH_MC_T));
        p_mc_entry_new->entry.hit_flag = 0;
        if(TRUE==dpp_vport_in_mc_bitmap(vport,p_mc_temp_entry->entry.mc_bitmap))
        {
            rc = dpp_vport_mc_info_update(slot,vport,new_vport,p_mc_temp_entry->key.mc_mac,&mc_info);
            ZXIC_COMM_CHECK_RC_MEMORY_VFREE2PTR_UNLOCK_NO_ASSERT(rc, "dpp_vport_mc_info_update",
                                                            pMcDataArrNew, pMcDataArr, DEV_PCIE_LOCK(dev));
            p_mc_entry_new->key.group_id = new_group_id;
            p_mc_entry_new->entry.mc_pf_enable = mc_info.mc_pf_enable;
            p_mc_entry_new->entry.mc_bitmap = mc_info.mc_bitmap[new_group_id];
        }
        else
        {
            if(p_mc_temp_entry->key.group_id==new_group_id)
            {
                p_mc_entry_new->key.group_id = group_id;
            }
        }

        ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x new_vport: 0x%04x mac: %02x:%02x:%02x:%02x:%02x:%02x start.\n",
                                slot,vport,new_vport,
                                p_mc_entry_new->key.mc_mac[0], p_mc_entry_new->key.mc_mac[1],
                                p_mc_entry_new->key.mc_mac[2], p_mc_entry_new->key.mc_mac[3],
                                p_mc_entry_new->key.mc_mac[4], p_mc_entry_new->key.mc_mac[5]);
        ZXIC_COMM_TRACE_NOTICE("group_id: %u mc_bitmap: 0x%016llx new_group_id: %u new_mc_bitmap: 0x%016llx\n",
                                p_mc_temp_entry->key.group_id,p_mc_temp_entry->entry.mc_bitmap,
                                p_mc_entry_new->key.group_id, p_mc_entry_new->entry.mc_bitmap);
    }

    if(entryNum==0)
    {
        ZXIC_COMM_VFREE(pMcDataArrNew);
        ZXIC_COMM_VFREE(pMcDataArr);
        ZXIC_COMM_TRACE_NOTICE("transfer num is 0!\n");
        rc = dpp_vport_table_unlock(&pf_info, sdt_no);
        ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

        return DPP_OK;
    }

    rc = dpp_apt_dtb_multi_hash_delete(dev, queue, sdt_no, entryNum,sizeof(ZXDH_MC_T),pMcDataArr);
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE2PTR_UNLOCK_NO_ASSERT(rc, "dpp_apt_dtb_multi_hash_delete", pMcDataArrNew, pMcDataArr, DEV_PCIE_LOCK(dev));

    rc = dpp_apt_dtb_multi_hash_insert(dev, queue, sdt_no, entryNum,sizeof(ZXDH_MC_T),pMcDataArrNew);
    ZXIC_COMM_VFREE(pMcDataArrNew);
    ZXIC_COMM_VFREE(pMcDataArr);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_multi_hash_insert", DEV_PCIE_LOCK(dev));

    rc = dpp_vport_table_unlock(&pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    return DPP_OK;
}

ZXIC_UINT32 dpp_multicast_mac_transfer(DPP_PF_INFO_T* pf_info, DPP_PF_INFO_T* new_pf_info)
{
    DPP_DEV_T dev = {0};
    DPP_DEV_T new_dev = {0};
    ZXIC_UINT32 group_id = 0;
    ZXIC_UINT32 new_group_id = 0;

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

    group_id = VFUNC_NUM(pf_info->vport)/MC_MEMBER_NUM_IN_GROUP;
    new_group_id = VFUNC_NUM(new_pf_info->vport)/MC_MEMBER_NUM_IN_GROUP;
    if(group_id==new_group_id)
    {
        rc  = dpp_multicast_mac_transfer_intra_group(&dev,pf_info->slot,pf_info->vport,new_pf_info->vport);
        ZXIC_COMM_CHECK_RC(rc, "dpp_multicast_mac_transfer_intra_group");
    }
    else
    {
        rc = dpp_multicast_mac_transfer_inter_group(&dev,pf_info->slot,pf_info->vport,new_pf_info->vport);
        ZXIC_COMM_CHECK_RC(rc, "dpp_multicast_mac_transfer_inter_group");
    }

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x new_vport: 0x%04x success.\n",
                            pf_info->slot, pf_info->vport, new_pf_info->vport);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_multicast_mac_transfer);

ZXIC_UINT32 dpp_mc_pf_flag_add(MC_PF_FLAG_MGR * p_flag_mgr, ZXIC_UINT32 index, ZXIC_UINT8 *mc_addr)
{
    ZXIC_COMM_MEMCPY(p_flag_mgr[index].mc_addr, mc_addr, 6);
    p_flag_mgr[index].pf_flag = 1;
    return DPP_OK;
}

ZXIC_UINT32 dpp_mc_pf_flag_search(MC_PF_FLAG_MGR * p_flag_mgr, ZXIC_UINT8 *mc_addr)
{
    ZXIC_UINT32 index = 0;

    for(index = 0; index < MC_TABLE_SIZE; index ++)
    {
        if (ZXIC_COMM_MEMCMP(p_flag_mgr[index].mc_addr, mc_addr, 6) == 0)
        {
            break;
        }
    }

    if (index == MC_TABLE_SIZE)
    {
        ZXIC_COMM_TRACE_NOTICE("dpp_mc_pf_flag_search failed\n");
        return DPP_ERR;
    }

    return DPP_OK;
}

ZXIC_UINT32 dpp_multicast_mac_dump(DPP_PF_INFO_T* pf_info, MAC_VPORT_INFO *p_mac_arr, ZXIC_UINT32 *p_mac_num)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 rc     = DPP_OK;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 mac_info_index = 0;
    ZXIC_UINT32 num = 0;
    ZXIC_UINT32 queue = 0;
    ZXIC_UINT32 sdt_no = 0;
    ZXIC_UINT32 hash_index = 0;
    ZXIC_UINT32 current_group_id = 0;
    ZXIC_UINT64 current_mc_bitmap = 0;
    ZXIC_UINT32 current_mc_pf_enable = 0;

    ZXIC_UINT16 current_vport[64] = {0};
    ZXIC_UINT32 current_vport_num = 0;

    MC_PF_FLAG_MGR *p_mc_pf_flag_mgr = NULL;
    ZXIC_UINT32 pf_flag_count = 0;

    ZXIC_UINT32 max_item_num = DTB_DUMP_MULTICAST_MAC_DUMP_NUM;
    ZXIC_UINT32 entryNum = 0;

    ZXDH_MC_T *pMcDataArr = NULL;
    ZXDH_MC_T *p_mc_temp_entry = NULL;
    MAC_VPORT_INFO *p_temp_mac_info = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_mac_arr);
    ZXIC_COMM_CHECK_POINT(p_mac_num);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_hash_index_get(pf_info, &hash_index);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_hash_index_get");
    sdt_no = ZXDH_SDT_MC_TABLE_PHYPORT0 + hash_index;

    rc = dpp_hash_max_item_num_get(&dev,sdt_no,&max_item_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_hash_max_item_num_get");

    pMcDataArr = (ZXDH_MC_T *)ZXIC_COMM_VMALLOC(max_item_num * sizeof(ZXDH_MC_T));
    ZXIC_COMM_CHECK_POINT_NO_ASSERT(pMcDataArr);

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE(rc, "dpp_vport_table_lock", pMcDataArr);
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE(DEV_PCIE_LOCK(&dev), pMcDataArr);

    rc = dpp_apt_dtb_hash_table_multicast_mac_dump(&dev, queue, sdt_no, pMcDataArr, &entryNum);
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE_UNLOCK_NO_ASSERT(rc, "dpp_apt_dtb_hash_table_multicast_mac_dump", pMcDataArr, DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE(rc, "dpp_vport_table_unlock", pMcDataArr);

    ZXIC_COMM_TRACE_NOTICE("multicast mac dump num:0x%x\n", entryNum);

    p_mc_pf_flag_mgr = (MC_PF_FLAG_MGR *)ZXIC_COMM_VMALLOC(MC_TABLE_SIZE * sizeof(MC_PF_FLAG_MGR));
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE_NO_ASSERT(p_mc_pf_flag_mgr, pMcDataArr);
    ZXIC_COMM_MEMSET(p_mc_pf_flag_mgr, 0x00, MC_TABLE_SIZE * sizeof(MC_PF_FLAG_MGR));

    for(index = 0; index < entryNum; index ++)
    {
        p_mc_temp_entry = pMcDataArr + index;

        ZXIC_COMM_TRACE_NOTICE("mc entry index:0x%x\n", index);
        dpp_mc_entry_print(p_mc_temp_entry);

        // 从mc中获取mac地址给mac_info，再转换group_id 和 bitmap信息
        current_group_id = p_mc_temp_entry->key.group_id;
        current_mc_bitmap = p_mc_temp_entry->entry.mc_bitmap;
        current_mc_pf_enable = p_mc_temp_entry->entry.mc_pf_enable;

        if(current_mc_pf_enable)
        {
            p_temp_mac_info = p_mac_arr + mac_info_index;
            if(dpp_mc_pf_flag_search(p_mc_pf_flag_mgr, p_mc_temp_entry->key.mc_mac))
            {
                //没查中
                dpp_mc_pf_flag_add(p_mc_pf_flag_mgr, pf_flag_count, p_mc_temp_entry->key.mc_mac);
                pf_flag_count++;

                ZXIC_COMM_MEMCPY(p_temp_mac_info->addr, p_mc_temp_entry->key.mc_mac, 6);
                p_temp_mac_info->vport = OWNER_PF_VPORT(pf_info->vport);

                mac_info_index = mac_info_index + 1;
            }
        }

        rc = dpp_vport_get_by_mc_bitmap(OWNER_PF_VPORT(pf_info->vport), current_group_id, current_mc_bitmap, current_vport, &current_vport_num);
        ZXIC_COMM_CHECK_RC_MEMORY_VFREE2PTR_NO_ASSERT(rc, "dpp_vport_get_by_mc_bitmap", pMcDataArr, p_mc_pf_flag_mgr);

        ZXIC_COMM_TRACE_NOTICE("index %d get vf num %d\n", index, current_vport_num);

        for(num = 0; num < current_vport_num; num++)
        {
            p_temp_mac_info = p_mac_arr + mac_info_index;
            ZXIC_COMM_MEMCPY(p_temp_mac_info->addr, p_mc_temp_entry->key.mc_mac, 6);
            p_temp_mac_info->vport = current_vport[num];
            mac_info_index = mac_info_index + 1;
        }
        ZXIC_COMM_TRACE_NOTICE("mac_info_index 0x%x\n", mac_info_index);

    }

    ZXIC_COMM_VFREE(pMcDataArr);
    ZXIC_COMM_VFREE(p_mc_pf_flag_mgr);

    *p_mac_num = mac_info_index;

    ZXIC_COMM_TRACE_NOTICE("dump mac num: 0x%x\n", *p_mac_num);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_multicast_mac_dump);

ZXIC_UINT32 dpp_multicast_all_mac_delete(DPP_PF_INFO_T* pf_info)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 queue = 0;
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

    sdt_no = ZXDH_SDT_MC_TABLE_PHYPORT0 + hash_index;

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
EXPORT_SYMBOL(dpp_multicast_all_mac_delete);

ZXIC_UINT32 dpp_multicast_all_mac_online_delete(DPP_PF_INFO_T* pf_info)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 queue = 0;
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

    sdt_no = ZXDH_SDT_MC_TABLE_PHYPORT0 + hash_index;

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
EXPORT_SYMBOL(dpp_multicast_all_mac_online_delete);

ZXIC_UINT32 dpp_multicast_mac_max_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *max_num)
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
    sdt_no = ZXDH_SDT_MC_TABLE_PHYPORT0 + hash_index;

    rc = dpp_hash_max_item_num_get(&dev,sdt_no,max_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_hash_max_item_num_get");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x max_num: 0x%x get succ.\n",
                           pf_info->slot, pf_info->vport, *max_num);
    return DPP_OK;
}
EXPORT_SYMBOL(dpp_multicast_mac_max_get);

ZXIC_UINT32 dpp_multicast_all_mac_soft_delete(DPP_PF_INFO_T* pf_info)
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
    sdt_no = ZXDH_SDT_MC_TABLE_PHYPORT0 + hash_index;

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
EXPORT_SYMBOL(dpp_multicast_all_mac_soft_delete);

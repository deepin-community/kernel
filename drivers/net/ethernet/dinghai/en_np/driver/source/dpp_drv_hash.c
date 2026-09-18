#include "dpp_apt_se_api.h"
#include "dpp_se_api.h"
#include "dpp_apt_se.h"
#include "dpp_sdt.h"
#include "dpp_hash.h"
#include "dpp_dtb_table.h"
#include "dpp_drv_sdt.h"
#include "dpp_drv_hash.h"
#include "dpp_kernel_init.h"

static SE_APT_HASH_CONVERT_T g_se_hash_callback[] =
{
    {
        ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT0,
        dpp_apt_set_l2entry_data,
        dpp_apt_get_l2entry_data
    },
    {
        ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT1,
        dpp_apt_set_l2entry_data,
        dpp_apt_get_l2entry_data
    },
    {
        ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT2,
        dpp_apt_set_l2entry_data,
        dpp_apt_get_l2entry_data
    },
    {
        ZXDH_SDT_L2_ENTRY_TABLE_PHYPORT3,
        dpp_apt_set_l2entry_data,
        dpp_apt_get_l2entry_data
    },
    {
        ZXDH_SDT_MC_TABLE_PHYPORT0,
        dpp_apt_set_mc_data,
        dpp_apt_get_mc_data
    },
    {
        ZXDH_SDT_MC_TABLE_PHYPORT1,
        dpp_apt_set_mc_data,
        dpp_apt_get_mc_data
    },
    {
        ZXDH_SDT_MC_TABLE_PHYPORT2,
        dpp_apt_set_mc_data,
        dpp_apt_get_mc_data
    },
    {
        ZXDH_SDT_MC_TABLE_PHYPORT3,
        dpp_apt_set_mc_data,
        dpp_apt_get_mc_data
    },
    {
        ZXDH_SDT_RDMA_ENTRY_TABLE,
        dpp_apt_set_rdma_trans_data,
        dpp_apt_get_rdma_trans_data
    }
};

SE_APT_HASH_CONVERT_T *se_hash_callback_get(ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 num = 0;

    num = sizeof(g_se_hash_callback)/sizeof(SE_APT_HASH_CONVERT_T);
    for(index=0;index<num;index++)
    {
        if(g_se_hash_callback[index].sdt_no == sdt_no)
        {
            return &g_se_hash_callback[index];
        }
    }

    return NULL;
}

ZXIC_UINT32 dpp_apt_set_l2entry_data(ZXIC_VOID *pData, DPP_HASH_ENTRY *pEntry)
{
    ZXIC_UINT32 key = 0;
    ZXIC_UINT32 rst = 0;
    ZXDH_L2_ENTRY_T *pL2Entry = NULL;

    ZXIC_COMM_CHECK_POINT(pData);
    ZXIC_COMM_CHECK_POINT(pEntry);
    ZXIC_COMM_CHECK_POINT(pEntry->p_key);
    ZXIC_COMM_CHECK_POINT(pEntry->p_rst);

    pL2Entry = (ZXDH_L2_ENTRY_T *)pData;
  
    ZXIC_COMM_MEMCPY(pEntry->p_key + 1, pL2Entry->key.dmac_addr, 6);

    ZXIC_COMM_UINT32_WRITE_BITS(key, pL2Entry->key.sriov_vlan_tpid, 16, 16);
    ZXIC_COMM_UINT32_WRITE_BITS(key, pL2Entry->key.sriov_vlan_id, 0, 16);
    zxic_comm_swap((ZXIC_UINT8 *)&key, sizeof(ZXIC_UINT32));
    ZXIC_COMM_MEMCPY(pEntry->p_key + 7, &key,sizeof(ZXIC_UINT32));

    ZXIC_COMM_UINT32_WRITE_BITS(rst, pL2Entry->entry.hit_flag, 31, 1);
    ZXIC_COMM_UINT32_WRITE_BITS(rst, pL2Entry->entry.rsv, 11, 20);
    ZXIC_COMM_UINT32_WRITE_BITS(rst, pL2Entry->entry.vqm_vfid, 0, 11);

    zxic_comm_swap((ZXIC_UINT8 *)&rst,sizeof(ZXIC_UINT32));
    ZXIC_COMM_MEMCPY(pEntry->p_rst,&rst,sizeof(ZXIC_UINT32)); 

    return DPP_OK; 
}

ZXIC_UINT32 dpp_apt_get_l2entry_data(ZXIC_VOID *pData, DPP_HASH_ENTRY *pEntry)
{  
    ZXDH_L2_ENTRY_T *pL2Entry = NULL;

    ZXIC_UINT32 key = 0;

    ZXIC_COMM_CHECK_POINT(pData);
    ZXIC_COMM_CHECK_POINT(pEntry);
    ZXIC_COMM_CHECK_POINT(pEntry->p_rst);

    pL2Entry = (ZXDH_L2_ENTRY_T *)pData;

    key = *(ZXIC_UINT32 *)(pEntry->p_key + 7);
    zxic_comm_swap((ZXIC_UINT8 *)&key,sizeof(ZXIC_UINT32));
    ZXIC_COMM_UINT32_GET_BITS(pL2Entry->key.sriov_vlan_tpid, key, 16, 16);
    ZXIC_COMM_UINT32_GET_BITS(pL2Entry->key.sriov_vlan_id, key, 0, 16);

    ZXIC_COMM_MEMCPY(pL2Entry->key.dmac_addr, pEntry->p_key + 1, 6);

    zxic_comm_swap(pEntry->p_rst, sizeof(ZXIC_UINT32));
    ZXIC_COMM_UINT32_GET_BITS(pL2Entry->entry.hit_flag, *(ZXIC_UINT32 *)pEntry->p_rst, 31, 1);
    ZXIC_COMM_UINT32_GET_BITS(pL2Entry->entry.rsv, *(ZXIC_UINT32 *)pEntry->p_rst, 11, 20);
    ZXIC_COMM_UINT32_GET_BITS(pL2Entry->entry.vqm_vfid, *(ZXIC_UINT32 *)pEntry->p_rst, 0, 11);

    return DPP_OK;   
}

ZXIC_UINT32 dpp_apt_set_mc_data(ZXIC_VOID *pData, DPP_HASH_ENTRY *pEntry)
{
    ZXIC_UINT32 key = 0;
    ZXIC_UINT32 rst = 0;

    ZXDH_MC_T *mc_table = NULL;

    ZXIC_COMM_CHECK_POINT(pData);
    ZXIC_COMM_CHECK_POINT(pEntry);
    ZXIC_COMM_CHECK_POINT(pEntry->p_key);
    ZXIC_COMM_CHECK_POINT(pEntry->p_rst);

    mc_table = (ZXDH_MC_T *)pData;

    ZXIC_COMM_UINT32_WRITE_BITS(key, mc_table->key.rsv, 18, 14);
    ZXIC_COMM_UINT32_WRITE_BITS(key, mc_table->key.group_id, 16, 2);

    zxic_comm_swap((ZXIC_UINT8 *)&key, sizeof(ZXIC_UINT32));
    ZXIC_COMM_MEMCPY(pEntry->p_key + 1, &key, sizeof(ZXIC_UINT32));

    ZXIC_COMM_MEMCPY(pEntry->p_key + 3, mc_table->key.mc_mac, 6);

    ZXIC_COMM_UINT32_WRITE_BITS(rst, mc_table->entry.hit_flag, 31, 1);
    ZXIC_COMM_UINT32_WRITE_BITS(rst, mc_table->entry.mc_pf_enable, 30, 1);
    ZXIC_COMM_UINT32_WRITE_BITS(rst, mc_table->entry.rsv1, 0, 30);
    zxic_comm_swap((ZXIC_UINT8 *)&rst, sizeof(ZXIC_UINT32));
    ZXIC_COMM_MEMCPY(pEntry->p_rst, &rst, sizeof(ZXIC_UINT32));

    ZXIC_COMM_UINT32_WRITE_BITS(rst, mc_table->entry.rsv2, 0, 32);
    zxic_comm_swap((ZXIC_UINT8 *)&rst, sizeof(ZXIC_UINT32));
    ZXIC_COMM_MEMCPY(pEntry->p_rst + 4, &rst, sizeof(ZXIC_UINT32));
    
    rst = mc_table->entry.mc_bitmap >> 32;
    zxic_comm_swap((ZXIC_UINT8 *)&rst, sizeof(ZXIC_UINT32));
    ZXIC_COMM_MEMCPY(pEntry->p_rst + 8, &rst, sizeof(ZXIC_UINT32));

    rst = mc_table->entry.mc_bitmap;
    zxic_comm_swap((ZXIC_UINT8 *)&rst, sizeof(ZXIC_UINT32));
    ZXIC_COMM_MEMCPY(pEntry->p_rst + 12, &rst, sizeof(ZXIC_UINT32));

    return DPP_OK;
}

ZXIC_UINT32 dpp_apt_get_mc_data(ZXIC_VOID *pData, DPP_HASH_ENTRY *pEntry)
{
    ZXIC_UINT32 key = 0;
    ZXIC_UINT32 rst = 0;

    ZXDH_MC_T *mc_table = NULL;

    ZXIC_COMM_CHECK_POINT(pData);
    ZXIC_COMM_CHECK_POINT(pEntry);
    ZXIC_COMM_CHECK_POINT(pEntry->p_key);
    ZXIC_COMM_CHECK_POINT(pEntry->p_rst);

    mc_table = (ZXDH_MC_T *)pData;

    key = *(ZXIC_UINT32 *)(pEntry->p_key + 1);
    zxic_comm_swap((ZXIC_UINT8 *)&key, sizeof(ZXIC_UINT32));
    ZXIC_COMM_UINT32_GET_BITS(mc_table->key.rsv, key, 18, 14);
    ZXIC_COMM_UINT32_GET_BITS(mc_table->key.group_id, key, 16, 2);

    ZXIC_COMM_MEMCPY(mc_table->key.mc_mac, pEntry->p_key + 3, 6);

    zxic_comm_swap(pEntry->p_rst, sizeof(ZXIC_UINT32));
    ZXIC_COMM_UINT32_GET_BITS(mc_table->entry.hit_flag, *(ZXIC_UINT32 *)pEntry->p_rst, 31, 1);
    ZXIC_COMM_UINT32_GET_BITS(mc_table->entry.mc_pf_enable, *(ZXIC_UINT32 *)pEntry->p_rst, 30, 1);
    ZXIC_COMM_UINT32_GET_BITS(mc_table->entry.rsv1, *(ZXIC_UINT32 *)pEntry->p_rst, 0, 30);

    zxic_comm_swap(pEntry->p_rst + 4, sizeof(ZXIC_UINT32));
    ZXIC_COMM_UINT32_GET_BITS(mc_table->entry.rsv2, *(ZXIC_UINT32 *)(pEntry->p_rst + 4), 0, 32);

    zxic_comm_swap(pEntry->p_rst + 8, sizeof(ZXIC_UINT32));
    ZXIC_COMM_UINT32_GET_BITS(rst, *(ZXIC_UINT32 *)(pEntry->p_rst + 8), 0, 32);
    mc_table->entry.mc_bitmap = (((ZXIC_UINT64)rst) << 32);

    zxic_comm_swap(pEntry->p_rst + 12, sizeof(ZXIC_UINT32));
    ZXIC_COMM_UINT32_GET_BITS(rst, *(ZXIC_UINT32 *)(pEntry->p_rst + 12), 0, 32);
    mc_table->entry.mc_bitmap |= rst;

    return DPP_OK;
}

ZXIC_UINT32 dpp_apt_set_rdma_trans_data(ZXIC_VOID *pData, DPP_HASH_ENTRY *pEntry)
{
    ZXIC_UINT32 key = 0;
    ZXIC_UINT32 rst = 0;

    ZXDH_RDMA_TRANS_T *rdma_trans_table = NULL;

    ZXIC_COMM_CHECK_POINT(pData);
    ZXIC_COMM_CHECK_POINT(pEntry);
    ZXIC_COMM_CHECK_POINT(pEntry->p_key);
    ZXIC_COMM_CHECK_POINT(pEntry->p_rst);

    rdma_trans_table = (ZXDH_RDMA_TRANS_T *)pData;

    ZXIC_COMM_MEMCPY(pEntry->p_key + 1, &key, 2);

    ZXIC_COMM_MEMCPY(pEntry->p_key + 3, rdma_trans_table->key.mac_addr, 6);

    ZXIC_COMM_UINT32_WRITE_BITS(rst, rdma_trans_table->entry.hit_flag, 31, 1);
    ZXIC_COMM_UINT32_WRITE_BITS(rst, rdma_trans_table->entry.rsv, 10, 21);
    ZXIC_COMM_UINT32_WRITE_BITS(rst, rdma_trans_table->entry.rdma_vhca_id, 0, 10);

    zxic_comm_swap((ZXIC_UINT8 *)&rst, sizeof(ZXIC_UINT32));
    ZXIC_COMM_MEMCPY(pEntry->p_rst, &rst, sizeof(ZXIC_UINT32)); 

    return DPP_OK;
}

ZXIC_UINT32 dpp_apt_get_rdma_trans_data(ZXIC_VOID *pData, DPP_HASH_ENTRY *pEntry)
{
    ZXDH_RDMA_TRANS_T *rdma_trans_table = NULL;

    ZXIC_COMM_CHECK_POINT(pData);
    ZXIC_COMM_CHECK_POINT(pEntry);
    ZXIC_COMM_CHECK_POINT(pEntry->p_rst);

    rdma_trans_table = (ZXDH_RDMA_TRANS_T *)pData;

    ZXIC_COMM_MEMCPY(rdma_trans_table->key.mac_addr, pEntry->p_key + 3, 6);

    zxic_comm_swap(pEntry->p_rst, sizeof(ZXIC_UINT32));
    ZXIC_COMM_UINT32_GET_BITS(rdma_trans_table->entry.hit_flag, *(ZXIC_UINT32 *)pEntry->p_rst, 31, 1);
    ZXIC_COMM_UINT32_GET_BITS(rdma_trans_table->entry.rsv, *(ZXIC_UINT32 *)pEntry->p_rst, 10, 21);
    ZXIC_COMM_UINT32_GET_BITS(rdma_trans_table->entry.rdma_vhca_id, *(ZXIC_UINT32 *)pEntry->p_rst, 0, 10);

    return DPP_OK;
}

DPP_STATUS dpp_apt_dtb_hash_table_unicast_mac_dump(DPP_DEV_T *dev, 
                                                   ZXIC_UINT32 queue_id, 
                                                   ZXIC_UINT32 sdt_no, 
                                                   ZXDH_L2_ENTRY_T *pHashDataArr, 
                                                   ZXIC_UINT32 *p_entry_num)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 max_item_num = DTB_DUMP_UNICAST_MAC_DUMP_NUM;
    ZXIC_UINT32 index = 0; 
    ZXIC_UINT32 entryNum = 0;
    ZXIC_UINT8* pDumpData = NULL;
    ZXIC_UINT8 *pKey = NULL;
    ZXIC_UINT8 *pRst = NULL;

    DPP_HASH_ENTRY *p_dump_hash_entry = NULL;
    DPP_HASH_ENTRY *p_temp_entry = NULL;
    DPP_SDTTBL_HASH_T sdt_hash_info = {0};  /*SDT内容*/
    SE_APT_CALLBACK_T *pAptCallback = NULL;
    ZXDH_L2_ENTRY_T *p_l2_mac_entry = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(pHashDataArr);
    ZXIC_COMM_CHECK_POINT(p_entry_num);

    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_hash_info);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_soft_sdt_tbl_get");

    rc = dpp_hash_max_item_num_get(dev,sdt_no,&max_item_num);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_hash_max_item_num_get");

    pAptCallback = dpp_apt_get_func(dev, sdt_no);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pAptCallback);

    //分配空间
    pDumpData = (ZXIC_UINT8 *)ZXIC_COMM_VMALLOC(max_item_num * sizeof(DPP_HASH_ENTRY));
    ZXIC_COMM_CHECK_POINT_NO_ASSERT(pDumpData);
    pKey = (ZXIC_UINT8 *)ZXIC_COMM_VMALLOC(max_item_num * HASH_KEY_MAX);
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE_NO_ASSERT(pKey, pDumpData);
    pRst = (ZXIC_UINT8 *)ZXIC_COMM_VMALLOC(max_item_num * HASH_RST_MAX);
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE2PTR_NO_ASSERT(pRst, pKey, pDumpData);

    ZXIC_COMM_MEMSET_S(pDumpData, max_item_num * sizeof(DPP_HASH_ENTRY), 0x0, max_item_num * sizeof(DPP_HASH_ENTRY));
    ZXIC_COMM_MEMSET_S(pKey, max_item_num * HASH_KEY_MAX, 0x0, max_item_num * HASH_KEY_MAX);
    ZXIC_COMM_MEMSET_S(pRst, max_item_num * HASH_RST_MAX, 0x0, max_item_num * HASH_RST_MAX);

    p_dump_hash_entry = (DPP_HASH_ENTRY *)pDumpData;
    for(index = 0; index < max_item_num; index++)
    {
        p_temp_entry = p_dump_hash_entry + index;
        p_temp_entry->p_key = pKey + index * HASH_KEY_MAX;
        p_temp_entry->p_rst = pRst + index * HASH_RST_MAX;
    }

    rc = dpp_dtb_hash_dump(dev,queue_id,sdt_no,pDumpData,&entryNum);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_VFREE3PTR_NO_ASSERT(DEV_ID(dev), rc, "dpp_apt_dtb_hash_dump", pRst, pKey, pDumpData);
    ZXIC_COMM_TRACE_INFO("dpp_dtb_hash_table_only_zcam_dump unicast entry_num: %d\n", entryNum);

    for(index = 0; index < entryNum; index++)
    {
        p_temp_entry = p_dump_hash_entry + index;
        p_l2_mac_entry = pHashDataArr + index;
        // //打印数据
        dpp_dtb_data_print(p_temp_entry->p_key, DPP_GET_ACTU_KEY_BY_SIZE(sdt_hash_info.key_size) + 1);
        dpp_dtb_data_print(p_temp_entry->p_rst, 4 * (0x1 << sdt_hash_info.rsp_mode));

        rc = pAptCallback->se_func_info.hashFunc.hash_get_func(p_l2_mac_entry, p_temp_entry);
        ZXIC_COMM_CHECK_DEV_RC_MEMORY_VFREE3PTR_NO_ASSERT(DEV_ID(dev), rc, "hash_set_func", pRst, pKey, pDumpData);
    }

    *p_entry_num = entryNum;

    ZXIC_COMM_VFREE(pKey);
    ZXIC_COMM_VFREE(pRst);
    ZXIC_COMM_VFREE(pDumpData);

    return DPP_OK;
}

DPP_STATUS dpp_apt_dtb_hash_table_multicast_mac_dump(DPP_DEV_T *dev, 
                                            ZXIC_UINT32 queue_id, 
                                            ZXIC_UINT32 sdt_no, 
                                            ZXDH_MC_T *pHashDataArr, 
                                            ZXIC_UINT32 *p_entry_num)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 max_item_num = DTB_DUMP_MULTICAST_MAC_DUMP_NUM;
    ZXIC_UINT32 index = 0; 
    ZXIC_UINT32 entryNum = 0;
    ZXIC_UINT8* pDumpData = NULL;
    ZXIC_UINT8 *pKey = NULL;
    ZXIC_UINT8 *pRst = NULL;

    DPP_HASH_ENTRY *p_dump_hash_entry = NULL;
    DPP_HASH_ENTRY *p_temp_entry = NULL;
    DPP_SDTTBL_HASH_T sdt_hash_info = {0};  /*SDT内容*/
    SE_APT_CALLBACK_T *pAptCallback = NULL;
    ZXDH_MC_T *p_multicast_mac_data = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(pHashDataArr);
    ZXIC_COMM_CHECK_POINT(p_entry_num);

    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_hash_info);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_soft_sdt_tbl_get");

    rc = dpp_hash_max_item_num_get(dev,sdt_no,&max_item_num);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_hash_max_item_num_get");

    pAptCallback = dpp_apt_get_func(dev, sdt_no);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pAptCallback);

    //分配空间
    pDumpData = (ZXIC_UINT8 *)ZXIC_COMM_VMALLOC(max_item_num * sizeof(DPP_HASH_ENTRY));
    ZXIC_COMM_CHECK_POINT_NO_ASSERT(pDumpData);
    pKey = (ZXIC_UINT8 *)ZXIC_COMM_VMALLOC(max_item_num * HASH_KEY_MAX);
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE_NO_ASSERT(pKey, pDumpData);
    pRst = (ZXIC_UINT8 *)ZXIC_COMM_VMALLOC(max_item_num * HASH_RST_MAX);
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE2PTR_NO_ASSERT(pRst, pKey, pDumpData);

    ZXIC_COMM_MEMSET_S(pDumpData, max_item_num * sizeof(DPP_HASH_ENTRY), 0x0, max_item_num * sizeof(DPP_HASH_ENTRY));
    ZXIC_COMM_MEMSET_S(pKey, max_item_num * HASH_KEY_MAX, 0x0, max_item_num * HASH_KEY_MAX);
    ZXIC_COMM_MEMSET_S(pRst, max_item_num * HASH_RST_MAX, 0x0, max_item_num * HASH_RST_MAX);

    p_dump_hash_entry = (DPP_HASH_ENTRY *)pDumpData;
    for(index = 0; index < max_item_num; index++)
    {
        p_temp_entry = p_dump_hash_entry + index;
        p_temp_entry->p_key = pKey + index * HASH_KEY_MAX;
        p_temp_entry->p_rst = pRst + index * HASH_RST_MAX;
    }

    rc = dpp_dtb_hash_dump(dev,queue_id,sdt_no,pDumpData,&entryNum);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_VFREE3PTR_NO_ASSERT(DEV_ID(dev), rc, "dpp_apt_dtb_hash_dump", pRst, pKey, pDumpData);

    ZXIC_COMM_TRACE_INFO("dpp_dtb_hash_table_only_zcam_dump multicast entry_num: %d\n", entryNum);

    for(index = 0; index < entryNum; index++)
    {
        p_temp_entry = p_dump_hash_entry + index;
        p_multicast_mac_data = pHashDataArr + index;
        // //打印数据
        dpp_dtb_data_print(p_temp_entry->p_key, DPP_GET_ACTU_KEY_BY_SIZE(sdt_hash_info.key_size) + 1);
        dpp_dtb_data_print(p_temp_entry->p_rst, 4 * (0x1 << sdt_hash_info.rsp_mode));

        rc = pAptCallback->se_func_info.hashFunc.hash_get_func(p_multicast_mac_data, p_temp_entry);
        ZXIC_COMM_CHECK_DEV_RC_MEMORY_VFREE3PTR_NO_ASSERT(DEV_ID(dev), rc, "hash_get_func", pRst, pKey, pDumpData);
    }

    *p_entry_num = entryNum;

    ZXIC_COMM_VFREE(pKey);
    ZXIC_COMM_VFREE(pRst);
    ZXIC_COMM_VFREE(pDumpData);

    return DPP_OK;
}


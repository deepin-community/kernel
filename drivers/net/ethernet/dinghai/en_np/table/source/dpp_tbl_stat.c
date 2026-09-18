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
#include "dpp_stat_api.h"
#include "dpp_tbl_api.h"
#include "dpp_tbl_comm.h"
#include "dpp_tbl_stat.h"
#include "dpp_agent_channel.h"

DPP_STAT_RECODE_T g_std_nic_stat_res[DPP_PCIE_SLOT_MAX] = {0};

ZXIC_UINT64 *dpp_std_nic_stat_res_get(DPP_PF_INFO_T* pf_info)
{
    if(pf_info == NULL)
    {
        ZXIC_COMM_TRACE_ERROR("pf_info is NULL\n");
        return NULL;
    }
    return g_std_nic_stat_res[pf_info->slot].p_dpp_stat_recode;
}

ZXIC_UINT32 dpp_std_nic_stat_recode_init(DPP_PF_INFO_T* pf_info)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT16  slot = pf_info->slot;
    ZXIC_UINT64 *p_std_nic_stat_res = NULL;
    DPP_DEV_T dev = {0};
    DPP_APT_SE_RES_T *p_se_res = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(&dev);
    ZXIC_COMM_CHECK_POINT(p_se_res);


    if (g_std_nic_stat_res[slot].p_dpp_stat_recode == NULL)
    {
        p_std_nic_stat_res = (ZXIC_UINT64 *)ZXIC_COMM_VMALLOC(p_se_res->stat_cfg.eram_depth * 2 * (sizeof(ZXIC_UINT64)));
        ZXIC_COMM_CHECK_POINT(p_std_nic_stat_res);
        ZXIC_COMM_MEMSET_S(p_std_nic_stat_res, p_se_res->stat_cfg.eram_depth * 2 * (sizeof(ZXIC_UINT64)), 0, p_se_res->stat_cfg.eram_depth * 2 * (sizeof(ZXIC_UINT64)));

        g_std_nic_stat_res[slot].p_dpp_stat_recode = p_std_nic_stat_res;
    }

    ZXIC_COMM_TRACE_NOTICE("slot %d stat_recode_init done, stat depth(128bit): 0x%x\n", slot, p_se_res->stat_cfg.eram_depth);
        
    return rc;
}

ZXIC_UINT32 dpp_std_nic_stat_recode_uninit(DPP_PF_INFO_T* pf_info)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 last_flag = 0;
    ZXIC_UINT16  slot = pf_info->slot;
    DPP_DEV_T dev = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dev_last_check(&dev, &last_flag);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dev_last_check");

    if(last_flag)
    {
        if (NULL != g_std_nic_stat_res[slot].p_dpp_stat_recode)
        {
            ZXIC_COMM_VFREE(g_std_nic_stat_res[slot].p_dpp_stat_recode);
            g_std_nic_stat_res[slot].p_dpp_stat_recode = NULL;
        }
    }

    ZXIC_COMM_TRACE_NOTICE("[%s] slot[%d] success.\n", __FUNCTION__, slot);

    return rc;
}

ZXIC_UINT32 dpp_std_nic_stat_unlock_cnt(DPP_PF_INFO_T* pf_info)
{
    ZXIC_UINT64 * p_std_nic_stat_res = dpp_std_nic_stat_res_get(pf_info);

    ZXIC_COMM_CHECK_POINT(pf_info);

    if(p_std_nic_stat_res == NULL)
    {
        ZXIC_COMM_TRACE_ERROR("slot %d std_nic_stat_res is NULL\n", pf_info->slot);
        return DPP_ERR;
    }

    g_std_nic_stat_res[pf_info->slot].unlock_cnt++;

    return DPP_OK;
}

ZXIC_UINT32 diag_dpp_std_nic_stat_unlock_cnt_prt(ZXIC_UINT32 slot)
{
    ZXIC_COMM_CHECK_INDEX(slot, 0, DPP_PCIE_SLOT_MAX - 1);

    ZXIC_COMM_PRINT("slot[%d] unlock cnt 0x%llu.\n", slot, g_std_nic_stat_res[slot].unlock_cnt);
    return DPP_OK;
}

ZXIC_UINT32 dpp_std_nic_stat_recode_set_64bit(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index_64bit, ZXIC_UINT64 value)
{
    ZXIC_UINT64 * p_std_nic_stat_res = dpp_std_nic_stat_res_get(pf_info);

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_INDEX(index_64bit, 0, DPP_STAT_ERAM_DEPTH_64BIT - 1);

    if(p_std_nic_stat_res == NULL)
    {
        ZXIC_COMM_TRACE_ERROR("slot %d std_nic_stat_res is NULL\n", pf_info->slot);
        return DPP_ERR;
    }

    p_std_nic_stat_res[index_64bit] = value;

    return DPP_OK;
}

ZXIC_UINT32 dpp_std_nic_stat_recode_get_64bit(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index_64bit, ZXIC_UINT64 *p_value)
{
    ZXIC_UINT64 * p_std_nic_stat_res = dpp_std_nic_stat_res_get(pf_info);

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_INDEX(index_64bit, 0, DPP_STAT_ERAM_DEPTH_64BIT - 1);

    if(p_std_nic_stat_res == NULL)
    {
        ZXIC_COMM_TRACE_ERROR("slot %d std_nic_stat_res is NULL\n", pf_info->slot);
        return DPP_ERR;
    }

    *p_value = p_std_nic_stat_res[index_64bit];

    return DPP_OK;
}

ZXIC_UINT32 dpp_std_nic_stat_recode_set_128bit(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index_128bit, ZXIC_UINT64 value_h, ZXIC_UINT64 value_l)
{
    ZXIC_UINT64 * p_std_nic_stat_res = dpp_std_nic_stat_res_get(pf_info);

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_INDEX(index_128bit, 0, DPP_STAT_ERAM_DEPTH_128BIT - 1);

    if(p_std_nic_stat_res == NULL)
    {
        ZXIC_COMM_TRACE_ERROR("slot %d std_nic_stat_res is NULL\n", pf_info->slot);
        return DPP_ERR;
    }

    p_std_nic_stat_res[index_128bit * 2] = value_h;
    p_std_nic_stat_res[index_128bit * 2 + 1] = value_l;

    return DPP_OK;
}

ZXIC_UINT32 dpp_std_nic_stat_recode_get_128bit(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index_128bit, ZXIC_UINT64 *p_value_h, ZXIC_UINT64 *p_value_l)
{
    ZXIC_UINT64 * p_std_nic_stat_res = dpp_std_nic_stat_res_get(pf_info);

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_INDEX(index_128bit, 0, DPP_STAT_ERAM_DEPTH_128BIT - 1);

    if(p_std_nic_stat_res == NULL)
    {
        ZXIC_COMM_TRACE_ERROR("slot %d std_nic_stat_res is NULL\n", pf_info->slot);
        return DPP_ERR;
    }

    *p_value_h = p_std_nic_stat_res[index_128bit * 2];
    *p_value_l = p_std_nic_stat_res[index_128bit * 2 + 1];

    return DPP_OK;
}

ZXIC_UINT32 dpp_stat_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue   = 0;
    ZXIC_UINT32 buff[2] = {0};
    ZXIC_UINT32 rc      = DPP_OK;
    DPP_APT_SE_RES_T *p_se_res = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_TRACE_DEBUG("slot: %u vport: 0x%04x index: %u mode: %u start.\n",
                                                                pf_info->slot, pf_info->vport, index, mode);

    ZXIC_COMM_CHECK_POINT(p_cnt);
    ZXIC_COMM_CHECK_INDEX(mode, STAT_RD_CLR_MODE_UNCLR, STAT_RD_CLR_MODE_CLR);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get_by_func(&dev, DPP_DTB_QUEUE_TYPE_STAT, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get_by_func");

    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(&dev);
    ZXIC_COMM_CHECK_POINT(p_se_res);
    ZXIC_COMM_CHECK_INDEX_LOWER(p_se_res->stat_cfg.eram_depth, 1);
    ZXIC_COMM_CHECK_INDEX(index, 0, p_se_res->stat_cfg.eram_depth*2 - 1);
    if (mode == STAT_RD_CLR_MODE_CLR)
    {
        rc = dpp_stat_ppu_cnt_get(&dev, STAT_64_MODE, index, STAT_RD_CLR_MODE_CLR, buff);
        ZXIC_COMM_CHECK_RC(rc, "dpp_stat_ppu_cnt_get");
    }
    else
    {
        rc = dpp_dtb_spin_eram_stat_data_get(&dev, queue, p_se_res->stat_cfg.eram_baddr, 
                                                 ERAM128_TBL_64b, index, buff);
        if(rc == ZXIC_SPIN_LOCK_TRYLOCK_FAIL)
        {
            rc = dpp_std_nic_stat_unlock_cnt(pf_info);
            ZXIC_COMM_CHECK_RC(rc, "dpp_std_nic_stat_unlock_cnt");

            rc = dpp_std_nic_stat_recode_get_64bit(pf_info, index, p_cnt);
            ZXIC_COMM_CHECK_RC(rc, "dpp_std_nic_stat_recode_get_64bit");

            return DPP_OK;
        }
        ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_spin_eram_stat_data_get");
    }

    *p_cnt = ((ZXIC_UINT64)buff[0] << 32) | buff[1];

    rc = dpp_std_nic_stat_recode_set_64bit(pf_info, index, *p_cnt); 
    ZXIC_COMM_CHECK_RC(rc, "dpp_std_nic_stat_recode_set_64bit");

    ZXIC_COMM_TRACE_DEBUG("slot: %u vport: 0x%04x index: %u mode: %u cnt: %llu success.\n",
                                            pf_info->slot, pf_info->vport, index, mode, *p_cnt);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_cnt_get);

ZXIC_UINT32 dpp_stat_cnt_get_128(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue   = 0;
    ZXIC_UINT32 buff[4] = {0};
    ZXIC_UINT32 rc      = DPP_OK;
    DPP_APT_SE_RES_T *p_se_res = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_TRACE_DEBUG("slot: %u vport: 0x%04x index: %u mode: %u start.\n",
                                                                    pf_info->slot, pf_info->vport, index, mode);

    ZXIC_COMM_CHECK_POINT(p_pkB_cnt);
    ZXIC_COMM_CHECK_POINT(p_pk_cnt);
    ZXIC_COMM_CHECK_INDEX(mode, STAT_RD_CLR_MODE_UNCLR, STAT_RD_CLR_MODE_CLR);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get_by_func(&dev, DPP_DTB_QUEUE_TYPE_STAT, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get_by_func");

    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(&dev);
    ZXIC_COMM_CHECK_POINT(p_se_res);
    ZXIC_COMM_CHECK_INDEX_LOWER(p_se_res->stat_cfg.eram_depth, 1);
    ZXIC_COMM_CHECK_INDEX(index, 0, p_se_res->stat_cfg.eram_depth - 1);
	
    if (mode == STAT_RD_CLR_MODE_CLR)
    {
        rc = dpp_stat_ppu_cnt_get(&dev, STAT_128_MODE, index, STAT_RD_CLR_MODE_CLR, buff);
        ZXIC_COMM_CHECK_RC(rc, "dpp_stat_ppu_cnt_get");
    }
    else
    {
        rc = dpp_dtb_spin_eram_stat_data_get(&dev, queue, p_se_res->stat_cfg.eram_baddr, 
                                                 ERAM128_TBL_128b, index, buff);
        if(rc == ZXIC_SPIN_LOCK_TRYLOCK_FAIL)
        {
            rc = dpp_std_nic_stat_unlock_cnt(pf_info);
            ZXIC_COMM_CHECK_RC(rc, "dpp_std_nic_stat_unlock_cnt");

            rc = dpp_std_nic_stat_recode_get_128bit(pf_info, index, p_pk_cnt, p_pkB_cnt);
            ZXIC_COMM_CHECK_RC(rc, "dpp_std_nic_stat_recode_get_128bit");

            return DPP_OK;
        }
        ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_spin_eram_stat_data_get");
    }

    *p_pk_cnt = ((ZXIC_UINT64)buff[0] << 32) | buff[1];
    *p_pkB_cnt = ((ZXIC_UINT64)buff[2] << 32) | buff[3];

    rc = dpp_std_nic_stat_recode_set_128bit(pf_info, index, *p_pk_cnt, *p_pkB_cnt);  
    ZXIC_COMM_CHECK_RC(rc, "dpp_std_nic_stat_recode_set_128bit"); 

    ZXIC_COMM_TRACE_DEBUG("slot: %u vport: 0x%04x index: %u mode: %u h64_cnt: %llu success.\n",
                                                        pf_info->slot, pf_info->vport, index, mode, *p_pk_cnt);
    ZXIC_COMM_TRACE_DEBUG("slot: %u vport: 0x%04x index: %u mode: %u l64_cnt: %llu success.\n",
                                                        pf_info->slot, pf_info->vport, index, mode, *p_pkB_cnt);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_cnt_get_128);

ZXIC_UINT32 dpp_stat_item_cnt_get(DPP_PF_INFO_T* pf_info, 
                            ZXIC_UINT32 stat_item_no, 
                            ZXIC_UINT32 index, 
                            ZXIC_UINT32 rd_mode, 
                            DPP_STAT_VALUE_U *p_stat_value)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 exist_flag = 0;
    DPP_DEV_T dev = {0};
    DPP_APT_STAT_ITEM_T *p_stat_item = NULL;
    DPP_APT_SE_RES_T *p_se_res = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_stat_value);
    ZXIC_COMM_CHECK_INDEX(rd_mode, STAT_RD_CLR_MODE_UNCLR, STAT_RD_CLR_MODE_CLR);
    ZXIC_COMM_CHECK_INDEX_UPPER(stat_item_no,STAT_ITEM_MAX_NUM-1);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(&dev);
    ZXIC_COMM_CHECK_POINT(p_se_res);

    rc = dpp_apt_sdt_is_exist(p_se_res,DPP_SDT_TBLT_eRAM,ZXDH_SDT_STAT_ATTR_TABLE,&exist_flag);
    ZXIC_COMM_CHECK_RC(rc, "dpp_apt_sdt_is_exist");
    if(exist_flag==0)
    {
        ZXIC_COMM_TRACE_INFO("The firmware not support stat item table!\n");
        return DPP_RC_TABLE_SDT_NOT_EXIST;
    }

    ZXIC_COMM_TRACE_INFO("stat_item_no=%u\n",stat_item_no);
    p_stat_item = &p_se_res->stat_item[stat_item_no];
    if(!(p_stat_item->valid))
    {
        ZXIC_COMM_TRACE_INFO("stat item[%u] is invalid!\n",stat_item_no);
        return DPP_RC_TABLE_STAT_ITEM_INVALID;
    }

    ZXIC_COMM_CHECK_INDEX_LOWER(p_stat_item->depth,1);
    ZXIC_COMM_CHECK_INDEX(index, 0, p_stat_item->depth - 1);
    
    ZXIC_COMM_MEMSET_S(p_stat_value,sizeof(DPP_STAT_VALUE_U),0x0,sizeof(DPP_STAT_VALUE_U));
    if(p_stat_item->mode==STAT_64_MODE)
    {
        rc = dpp_stat_cnt_get(pf_info, index + p_stat_item->addr_offset, rd_mode, &p_stat_value->stat_cnt_64);
        ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get");
    }
    else
    {
        rc = dpp_stat_cnt_get_128(pf_info, 
                                 index + p_stat_item->addr_offset, 
                                 rd_mode, 
                                 &p_stat_value->stat_cnt_128.bytes,
                                 &p_stat_value->stat_cnt_128.pkts);
        ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get_128");
    }

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_item_cnt_get);

ZXIC_UINT32 dpp_stat_mc_packet_rx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_cnt);

    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_RX_PF_MULTICAST_PKTS, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_cnt = stat_value.stat_cnt_64;
        return rc;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }
  
    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_MC_PACKET_RX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get(pf_info, index + DPP_STAT_MC_PACKET_RX_CNT_ERAM_BAADDR, mode, p_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_mc_packet_rx_cnt_get);

ZXIC_UINT32 dpp_stat_bc_packet_rx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_RX_VF_BROADCAST_PKTS, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_cnt = stat_value.stat_cnt_64;
        return rc;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_BC_PACKET_RX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get(pf_info, index + DPP_STAT_BC_PACKET_RX_CNT_ERAM_BAADDR, mode, p_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_bc_packet_rx_cnt_get);

ZXIC_UINT32 dpp_stat_1588_packet_rx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_RX_1588_PKTS, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_cnt = stat_value.stat_cnt_64;
        return rc;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_1588_PACKET_RX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get(pf_info, index + DPP_STAT_1588_PACKET_RX_CNT_ERAM_BAADDR, mode, p_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_1588_packet_rx_cnt_get);

ZXIC_UINT32 dpp_stat_1588_packet_tx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_TX_1588_PKTS, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_cnt = stat_value.stat_cnt_64;
        return rc;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_1588_PACKET_TX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get(pf_info, index + DPP_STAT_1588_PACKET_TX_CNT_ERAM_BAADDR, mode, p_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_1588_packet_tx_cnt_get);

ZXIC_UINT32 dpp_stat_1588_packet_drop_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_1588_DROP_PKTS, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_cnt = stat_value.stat_cnt_64;
        return rc;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_1588_PACKET_DROP_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get(pf_info, index + DPP_STAT_1588_PACKET_DROP_CNT_ERAM_BAADDR, mode, p_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_1588_packet_drop_cnt_get);

ZXIC_UINT32 dpp_stat_1588_enc_packet_rx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_1588_DRS_NP_ENCRYPT_PKTS, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_cnt = stat_value.stat_cnt_64;
        return rc;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_1588_ENC_PACKET_RX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get(pf_info, index + DPP_STAT_1588_ENC_PACKET_RX_CNT_ERAM_BAADDR, mode, p_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_1588_enc_packet_rx_cnt_get);

ZXIC_UINT32 dpp_stat_1588_enc_packet_tx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_1588_NP_DRS_ENCRYPT_PKTS, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_cnt = stat_value.stat_cnt_64;
        return rc;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_1588_ENC_PACKET_TX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get(pf_info, index + DPP_STAT_1588_ENC_PACKET_TX_CNT_ERAM_BAADDR, mode, p_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_1588_enc_packet_tx_cnt_get);

ZXIC_UINT32 dpp_stat_spoof_packet_drop_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_SPOOF_DROP_PKTS, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_cnt = stat_value.stat_cnt_64;
        return rc;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_SPOOF_PACKET_DROP_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get(pf_info, index + DPP_STAT_SPOOF_PACKET_DROP_CNT_ERAM_BAADDR, mode, p_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_spoof_packet_drop_cnt_get);

ZXIC_UINT32 dpp_stat_mcode_packet_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_MCODE_PPU_PKTS, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_cnt = stat_value.stat_cnt_64;
        return rc;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_MCODE_PACKET_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get(pf_info, index + DPP_STAT_MCODE_PACKET_CNT_ERAM_BAADDR, mode, p_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get");


    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_mcode_packet_cnt_get);

ZXIC_UINT32 dpp_stat_port_RDMA_packet_msg_tx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_pkB_cnt);
    ZXIC_COMM_CHECK_POINT(p_pk_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_RDMA_TX_STAT, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_pkB_cnt = stat_value.stat_cnt_128.bytes;
        *p_pk_cnt = stat_value.stat_cnt_128.pkts;
        return rc;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_PORT_RDMA_PACKET_TX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get_128(pf_info, index + DPP_STAT_PORT_RDMA_PACKET_TX_CNT_ERAM_BAADDR, mode, p_pkB_cnt, p_pk_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get_128");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_port_RDMA_packet_msg_tx_cnt_get);

ZXIC_UINT32 dpp_stat_port_RDMA_packet_msg_rx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_pkB_cnt);
    ZXIC_COMM_CHECK_POINT(p_pk_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_RDMA_RX_STAT, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_pkB_cnt = stat_value.stat_cnt_128.bytes;
        *p_pk_cnt = stat_value.stat_cnt_128.pkts;
        return rc;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_PORT_RDMA_PACKET_RX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get_128(pf_info, index + DPP_STAT_PORT_RDMA_PACKET_RX_CNT_ERAM_BAADDR, mode, p_pkB_cnt, p_pk_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get_128");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_port_RDMA_packet_msg_rx_cnt_get);

ZXIC_UINT32 dpp_stat_plcr_packet_drop_tx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_pkB_cnt);
    ZXIC_COMM_CHECK_POINT(p_pk_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_NIC_OUT_RATE_LIMIT_DROP_STAT, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_pkB_cnt = stat_value.stat_cnt_128.bytes;
        *p_pk_cnt = stat_value.stat_cnt_128.pkts;
        return rc;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_PLCR_PACKET_DROP_TX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get_128(pf_info, index + DPP_STAT_PLCR_PACKET_DROP_TX_CNT_ERAM_BAADDR, mode, p_pkB_cnt, p_pk_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get_128");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_plcr_packet_drop_tx_cnt_get);

ZXIC_UINT32 dpp_stat_plcr_packet_drop_rx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_pkB_cnt);
    ZXIC_COMM_CHECK_POINT(p_pk_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_NIC_IN_RATE_LIMIT_DROP_STAT, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_pkB_cnt = stat_value.stat_cnt_128.bytes;
        *p_pk_cnt = stat_value.stat_cnt_128.pkts;
        return rc;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_PLCR_PACKET_DROP_RX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get_128(pf_info, index + DPP_STAT_PLCR_PACKET_DROP_RX_CNT_ERAM_BAADDR, mode, p_pkB_cnt, p_pk_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get_128");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_plcr_packet_drop_rx_cnt_get);

ZXIC_UINT32 dpp_stat_MTU_packet_msg_tx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_pkB_cnt);
    ZXIC_COMM_CHECK_POINT(p_pk_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_TX_MTU_DROP_STAT, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_pkB_cnt = stat_value.stat_cnt_128.bytes;
        *p_pk_cnt = stat_value.stat_cnt_128.pkts;
        return rc;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_MTU_PACKET_DROP_TX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get_128(pf_info, index + DPP_STAT_MTU_PACKET_DROP_TX_CNT_ERAM_BAADDR, mode, p_pkB_cnt, p_pk_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get_128");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_MTU_packet_msg_tx_cnt_get);

ZXIC_UINT32 dpp_stat_MTU_packet_msg_rx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_pkB_cnt);
    ZXIC_COMM_CHECK_POINT(p_pk_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_RX_MTU_DROP_STAT, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_pkB_cnt = stat_value.stat_cnt_128.bytes;
        *p_pk_cnt = stat_value.stat_cnt_128.pkts;
        return rc;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_MTU_PACKET_DROP_RX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get_128(pf_info, index + DPP_STAT_MTU_PACKET_DROP_RX_CNT_ERAM_BAADDR, mode, p_pkB_cnt, p_pk_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get_128");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_MTU_packet_msg_rx_cnt_get);

ZXIC_UINT32 dpp_stat_port_uc_packet_rx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_pkB_cnt);
    ZXIC_COMM_CHECK_POINT(p_pk_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_NP_PORT_UNICAST_RX_STAT, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_pkB_cnt = stat_value.stat_cnt_128.bytes;
        *p_pk_cnt = stat_value.stat_cnt_128.pkts;
        return rc;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_PORT_UC_PACKET_RX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get_128(pf_info, index + DPP_STAT_PORT_UC_PACKET_RX_CNT_ERAM_BAADDR, mode, p_pkB_cnt, p_pk_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get_128");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_port_uc_packet_rx_cnt_get);

ZXIC_UINT32 dpp_stat_port_uc_packet_tx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_pkB_cnt);
    ZXIC_COMM_CHECK_POINT(p_pk_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_NP_PORT_UNICAST_TX_STAT, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_pkB_cnt = stat_value.stat_cnt_128.bytes;
        *p_pk_cnt = stat_value.stat_cnt_128.pkts;
        return rc;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_PORT_UC_PACKET_TX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get_128(pf_info, index + DPP_STAT_PORT_UC_PACKET_TX_CNT_ERAM_BAADDR, mode, p_pkB_cnt, p_pk_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get_128");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_port_uc_packet_tx_cnt_get);

ZXIC_UINT32 dpp_stat_port_mc_packet_rx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_pkB_cnt);
    ZXIC_COMM_CHECK_POINT(p_pk_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_NP_PORT_MULTICAST_RX_STAT, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_pkB_cnt = stat_value.stat_cnt_128.bytes;
        *p_pk_cnt = stat_value.stat_cnt_128.pkts;
        return rc;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_PORT_MC_PACKET_RX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get_128(pf_info, index + DPP_STAT_PORT_MC_PACKET_RX_CNT_ERAM_BAADDR, mode, p_pkB_cnt, p_pk_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get_128");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_port_mc_packet_rx_cnt_get);

ZXIC_UINT32 dpp_stat_port_mc_packet_tx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_pkB_cnt);
    ZXIC_COMM_CHECK_POINT(p_pk_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_NP_PORT_MULTICAST_TX_STAT, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_pkB_cnt = stat_value.stat_cnt_128.bytes;
        *p_pk_cnt = stat_value.stat_cnt_128.pkts;
        return rc;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_PORT_MC_PACKET_TX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get_128(pf_info, index + DPP_STAT_PORT_MC_PACKET_TX_CNT_ERAM_BAADDR, mode, p_pkB_cnt, p_pk_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get_128");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_port_mc_packet_tx_cnt_get);

ZXIC_UINT32 dpp_stat_port_bc_packet_rx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_pkB_cnt);
    ZXIC_COMM_CHECK_POINT(p_pk_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_NP_PORT_BROADCAST_RX_STAT, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_pkB_cnt = stat_value.stat_cnt_128.bytes;
        *p_pk_cnt = stat_value.stat_cnt_128.pkts;
        return rc;
    }
     else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_PORT_BC_PACKET_RX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get_128(pf_info, index + DPP_STAT_PORT_BC_PACKET_RX_CNT_ERAM_BAADDR, mode, p_pkB_cnt, p_pk_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get_128");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_port_bc_packet_rx_cnt_get);

ZXIC_UINT32 dpp_stat_port_bc_packet_tx_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_pkB_cnt);
    ZXIC_COMM_CHECK_POINT(p_pk_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_NP_PORT_BROADCAST_TX_STAT, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_pkB_cnt = stat_value.stat_cnt_128.bytes;
        *p_pk_cnt = stat_value.stat_cnt_128.pkts;
        return rc;
    }
     else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_PORT_BC_PACKET_TX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get_128(pf_info, index + DPP_STAT_PORT_BC_PACKET_TX_CNT_ERAM_BAADDR, mode, p_pkB_cnt, p_pk_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get_128");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_port_bc_packet_tx_cnt_get);

ZXIC_UINT32 dpp_stat_fd_stat_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_pkB_cnt, ZXIC_UINT64 *p_pk_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_pkB_cnt);
    ZXIC_COMM_CHECK_POINT(p_pk_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_FD_FLOW_STAT, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_pkB_cnt = stat_value.stat_cnt_128.bytes;
        *p_pk_cnt = stat_value.stat_cnt_128.pkts;
        return rc;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_FD_ACL_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get_128(pf_info, index + DPP_STAT_FD_ACL_CNT_ERAM_BAADDR, mode, p_pkB_cnt, p_pk_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get_128");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_fd_stat_cnt_get);

ZXIC_UINT32 dpp_stat_asn_phyport_rx_pkt_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_ASN_PHYPORT_RX_PKTS, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_cnt = stat_value.stat_cnt_64;
        return rc;
    }
     else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_ASN_PHYPORT_RX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get(pf_info, index + DPP_STAT_ASN_PHYPORT_RX_CNT_ERAM_BAADDR, mode, p_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_asn_phyport_rx_pkt_cnt_get);

ZXIC_UINT32 dpp_stat_psn_phyport_tx_pkt_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_PSN_PHYPORT_TX_PKTS, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_cnt = stat_value.stat_cnt_64;
        return rc;
    }
     else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_PSN_PHYPORT_TX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get(pf_info, index + DPP_STAT_PSN_PHYPORT_TX_CNT_ERAM_BAADDR, mode, p_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_psn_phyport_tx_pkt_cnt_get);

ZXIC_UINT32 dpp_stat_psn_phyport_rx_pkt_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_PSN_PHYPORT_RX_PKTS, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_cnt = stat_value.stat_cnt_64;
        return rc;
    }
     else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_PSN_PHYPORT_RX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get(pf_info, index + DPP_STAT_PSN_PHYPORT_RX_CNT_ERAM_BAADDR, mode, p_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_psn_phyport_rx_pkt_cnt_get);

ZXIC_UINT32 dpp_stat_psn_ack_phyport_tx_pkt_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_PSN_ACK_PHYPORT_TX_PKTS, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_cnt = stat_value.stat_cnt_64;
        return rc;
    }
     else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_PSN_ACK_PHYPORT_TX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get(pf_info, index + DPP_STAT_PSN_ACK_PHYPORT_TX_CNT_ERAM_BAADDR, mode, p_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_psn_ack_phyport_tx_pkt_cnt_get);

ZXIC_UINT32 dpp_stat_psn_ack_phyport_rx_pkt_cnt_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, ZXIC_UINT32 mode, ZXIC_UINT64 *p_cnt)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_STAT_VALUE_U stat_value = {0};

    ZXIC_COMM_CHECK_POINT(p_cnt);

    /*对接新固件流程*/
    rc = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_PSN_ACK_PHYPORT_RX_PKTS, index, mode, &stat_value);
    if(rc == DPP_OK)
    {
        *p_cnt = stat_value.stat_cnt_64;
        return rc;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("dpp_stat_item_cnt_get,rc=0x%x\n",rc);
    }

    /*对接老固件流程*/
    ZXIC_COMM_CHECK_INDEX(index, 0, DPP_STAT_PSN_ACK_PHYPORT_RX_CNT_ERAM_DEPTH - 1);

    rc = dpp_stat_cnt_get(pf_info, index + DPP_STAT_PSN_ACK_PHYPORT_RX_CNT_ERAM_BAADDR, mode, p_cnt);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_cnt_get");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_stat_psn_ack_phyport_rx_pkt_cnt_get);

ZXIC_UINT32 dpp_channel_stat_info_get(DPP_PF_INFO_T* pf_info, DPP_AGENT_CHANNEL_STAT_INFO_T *p_stats_info, DPP_PRIO_STAT_DATA_T *p_stats_data)
{
    ZXIC_UINT32  rc = 0;
    DPP_DEV_T dev = {0};
    ZXIC_MUTEX_T *p_mutex = NULL;

    ZXIC_COMM_CHECK_POINT(p_stats_info);
    ZXIC_COMM_CHECK_POINT(p_stats_data);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dev_opr_mutex_get(&dev, DPP_DEV_MUTEX_T_REG, &p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(&dev), rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(&dev), rc, "zxic_comm_mutex_lock");

    rc = dpp_agent_channel_prio_stat_get(&dev, p_stats_info, p_stats_data);
    ZXIC_COMM_CHECK_DEV_RC_UNLOCK(DEV_ID(&dev), rc, "dpp_agent_channel_prio_stat_get", p_mutex);

    rc = zxic_comm_mutex_unlock(p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(&dev), rc, "zxic_comm_mutex_unlock");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_channel_stat_info_get);
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
#include "dpp_tbl_mc.h"
#include "dpp_tbl_bc.h"
#include "dpp_tbl_promisc.h"
#include "dpp_tbl_comm.h"
#include "dpp_tbl_api.h"
#include "dpp_sdt.h"

static DPP_VPORT_MGR_T g_vport_mgr[DPP_PCIE_SLOT_MAX][DPP_PCIE_CHANNEL_MAX] = {0};

ZXIC_UINT32 dpp_data_print(ZXIC_UINT8 *data, ZXIC_UINT32 len)
{
    ZXIC_UINT32 i             = 0;
    ZXIC_UINT32 loop_cnt      = len / 16;
    ZXIC_UINT32 last_line_len = len % 16;

    ZXIC_COMM_TRACE_NOTICE("-----------------------------------------------\n");
    for (i = 0; i < loop_cnt; i++)
    {
        ZXIC_COMM_TRACE_NOTICE("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
               *(data + (i * 16) + 0), *(data + (i * 16) + 1), *(data + (i * 16) + 2), *(data + (i * 16) + 3),
               *(data + (i * 16) + 4), *(data + (i * 16) + 5), *(data + (i * 16) + 6), *(data + (i * 16) + 7),
               *(data + (i * 16) + 8), *(data + (i * 16) + 9), *(data + (i * 16) + 10), *(data + (i * 16) + 11),
               *(data + (i * 16) + 12), *(data + (i * 16) + 13), *(data + (i * 16) + 14), *(data + (i * 16) + 15));
    }
    if (last_line_len != 0)
    {
        if (last_line_len == 1)
        {
            ZXIC_COMM_TRACE_NOTICE("%02x\n", *(data + (i * 16) + 0));
        }
        else if (last_line_len == 2)
        {
            ZXIC_COMM_TRACE_NOTICE("%02x %02x\n", *(data + (i * 16) + 0), *(data + (i * 16) + 1));
        }
        else if (last_line_len == 3)
        {
            ZXIC_COMM_TRACE_NOTICE("%02x %02x %02x\n", *(data + (i * 16) + 0), *(data + (i * 16) + 1), *(data + (i * 16) + 2));
        }
        else if (last_line_len == 4)
        {
            ZXIC_COMM_TRACE_NOTICE("%02x %02x %02x %02x\n",
                    *(data + (i * 16) + 0), *(data + (i * 16) + 1), *(data + (i * 16) + 2), *(data + (i * 16) + 3));
        }
        else if (last_line_len == 5)
        {
            ZXIC_COMM_TRACE_NOTICE("%02x %02x %02x %02x %02x\n",
                    *(data + (i * 16) + 0), *(data + (i * 16) + 1), *(data + (i * 16) + 2), *(data + (i * 16) + 3),
                    *(data + (i * 16) + 4));
        }
        else if (last_line_len == 6)
        {
            ZXIC_COMM_TRACE_NOTICE("%02x %02x %02x %02x %02x %02x\n",
                    *(data + (i * 16) + 0), *(data + (i * 16) + 1), *(data + (i * 16) + 2), *(data + (i * 16) + 3),
                    *(data + (i * 16) + 4), *(data + (i * 16) + 5));
        }
        else if (last_line_len == 7)
        {
            ZXIC_COMM_TRACE_NOTICE("%02x %02x %02x %02x %02x %02x %02x\n",
                    *(data + (i * 16) + 0), *(data + (i * 16) + 1), *(data + (i * 16) + 2), *(data + (i * 16) + 3),
                    *(data + (i * 16) + 4), *(data + (i * 16) + 5), *(data + (i * 16) + 6));
        }
        else if (last_line_len == 8)
        {
            ZXIC_COMM_TRACE_NOTICE("%02x %02x %02x %02x %02x %02x %02x %02x\n",
                    *(data + (i * 16) + 0), *(data + (i * 16) + 1), *(data + (i * 16) + 2), *(data + (i * 16) + 3),
                    *(data + (i * 16) + 4), *(data + (i * 16) + 5), *(data + (i * 16) + 6), *(data + (i * 16) + 7));
        }
        else if (last_line_len == 9)
        {
            ZXIC_COMM_TRACE_NOTICE("%02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                    *(data + (i * 16) + 0), *(data + (i * 16) + 1), *(data + (i * 16) + 2), *(data + (i * 16) + 3),
                    *(data + (i * 16) + 4), *(data + (i * 16) + 5), *(data + (i * 16) + 6), *(data + (i * 16) + 7),
                    *(data + (i * 16) + 8));
        }
        else if (last_line_len == 10)
        {
            ZXIC_COMM_TRACE_NOTICE("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                    *(data + (i * 16) + 0), *(data + (i * 16) + 1), *(data + (i * 16) + 2), *(data + (i * 16) + 3),
                    *(data + (i * 16) + 4), *(data + (i * 16) + 5), *(data + (i * 16) + 6), *(data + (i * 16) + 7),
                    *(data + (i * 16) + 8), *(data + (i * 16) + 9));
        }
        else if (last_line_len == 11)
        {
            ZXIC_COMM_TRACE_NOTICE("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                    *(data + (i * 16) + 0), *(data + (i * 16) + 1), *(data + (i * 16) + 2), *(data + (i * 16) + 3),
                    *(data + (i * 16) + 4), *(data + (i * 16) + 5), *(data + (i * 16) + 6), *(data + (i * 16) + 7),
                    *(data + (i * 16) + 8), *(data + (i * 16) + 9), *(data + (i * 16) + 10));
        }
        else if (last_line_len == 12)
        {
            ZXIC_COMM_TRACE_NOTICE("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                    *(data + (i * 16) + 0), *(data + (i * 16) + 1), *(data + (i * 16) + 2), *(data + (i * 16) + 3),
                    *(data + (i * 16) + 4), *(data + (i * 16) + 5), *(data + (i * 16) + 6), *(data + (i * 16) + 7),
                    *(data + (i * 16) + 8), *(data + (i * 16) + 9), *(data + (i * 16) + 10), *(data + (i * 16) + 11));
        }
        else if (last_line_len == 13)
        {
            ZXIC_COMM_TRACE_NOTICE("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                    *(data + (i * 16) + 0), *(data + (i * 16) + 1), *(data + (i * 16) + 2), *(data + (i * 16) + 3),
                    *(data + (i * 16) + 4), *(data + (i * 16) + 5), *(data + (i * 16) + 6), *(data + (i * 16) + 7),
                    *(data + (i * 16) + 8), *(data + (i * 16) + 9), *(data + (i * 16) + 10), *(data + (i * 16) + 11),
                    *(data + (i * 16) + 12));
        }
        else if (last_line_len == 14)
        {
            ZXIC_COMM_TRACE_NOTICE("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                    *(data + (i * 16) + 0), *(data + (i * 16) + 1), *(data + (i * 16) + 2), *(data + (i * 16) + 3),
                    *(data + (i * 16) + 4), *(data + (i * 16) + 5), *(data + (i * 16) + 6), *(data + (i * 16) + 7),
                    *(data + (i * 16) + 8), *(data + (i * 16) + 9), *(data + (i * 16) + 10), *(data + (i * 16) + 11),
                    *(data + (i * 16) + 12), *(data + (i * 16) + 13));
        }
        else if (last_line_len == 15)
        {
            ZXIC_COMM_TRACE_NOTICE("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                    *(data + (i * 16) + 0), *(data + (i * 16) + 1), *(data + (i * 16) + 2), *(data + (i * 16) + 3),
                    *(data + (i * 16) + 4), *(data + (i * 16) + 5), *(data + (i * 16) + 6), *(data + (i * 16) + 7),
                    *(data + (i * 16) + 8), *(data + (i * 16) + 9), *(data + (i * 16) + 10), *(data + (i * 16) + 11),
                    *(data + (i * 16) + 12), *(data + (i * 16) + 13), *(data + (i * 16) + 14));
        }
    }
    ZXIC_COMM_TRACE_NOTICE("-----------------------------------------------\n");

    return DPP_OK;
}

ZXIC_UINT32 dpp_vport_mgr_init(DPP_PF_INFO_T* pf_info)
{
    ZXIC_UINT16 slot = 0;
    ZXIC_UINT16 channel_id = 0;
    ZXIC_UINT32 sdt_no = 0;
    ZXIC_UINT32 rc = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);

    slot = pf_info->slot;
    ZXIC_COMM_CHECK_INDEX(slot, 0, DPP_PCIE_SLOT_MAX - 1);

    channel_id = DPP_PCIE_CHANNEL_ID(pf_info->vport);
    ZXIC_COMM_CHECK_INDEX(channel_id, 0, DPP_PCIE_CHANNEL_MAX - 1);

    ZXIC_COMM_MEMSET(&g_vport_mgr[slot][channel_id], 0x00, sizeof(DPP_VPORT_MGR_T));

    g_vport_mgr[slot][channel_id].mc_table.mc_info = ZXIC_COMM_MALLOC(sizeof(DPP_VPORT_MC_INFO_T) * MC_TABLE_SIZE);
    ZXIC_COMM_CHECK_POINT(g_vport_mgr[slot][channel_id].mc_table.mc_info);
    ZXIC_COMM_MEMSET(g_vport_mgr[slot][channel_id].mc_table.mc_info, 0x00, sizeof(DPP_VPORT_MC_INFO_T) * MC_TABLE_SIZE);

    for (sdt_no = 0; sdt_no < DPP_DEV_SDT_ID_MAX; sdt_no++)
    {
        g_vport_mgr[slot][channel_id].table_lock[sdt_no] = ZXIC_COMM_MALLOC(sizeof(ZXIC_MUTEX_T));
        ZXIC_COMM_CHECK_POINT(g_vport_mgr[slot][channel_id].table_lock[sdt_no]);
        ZXIC_COMM_MEMSET(g_vport_mgr[slot][channel_id].table_lock[sdt_no], 0x00, sizeof(ZXIC_MUTEX_T));

        rc = zxic_comm_mutex_create(g_vport_mgr[slot][channel_id].table_lock[sdt_no]);
        ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_create");
    }

    return DPP_OK;
}

ZXIC_UINT32 dpp_vport_mgr_release(DPP_PF_INFO_T* pf_info)
{
    ZXIC_UINT16 slot = 0;
    ZXIC_UINT16 channel_id = 0;
    ZXIC_UINT32 sdt_no = 0;
    ZXIC_UINT32 rc = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);

    slot = pf_info->slot;
    ZXIC_COMM_CHECK_INDEX(slot, 0, DPP_PCIE_SLOT_MAX - 1);

    channel_id = DPP_PCIE_CHANNEL_ID(pf_info->vport);
    ZXIC_COMM_CHECK_INDEX(channel_id, 0, DPP_PCIE_CHANNEL_MAX - 1);

    ZXIC_COMM_FREE(g_vport_mgr[slot][channel_id].mc_table.mc_info);

    for (sdt_no = 0; sdt_no < DPP_DEV_SDT_ID_MAX; sdt_no++)
    {
        rc = zxic_comm_mutex_destroy(g_vport_mgr[slot][channel_id].table_lock[sdt_no]);
        ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_destroy");
        ZXIC_COMM_FREE(g_vport_mgr[slot][channel_id].table_lock[sdt_no]);
    }

    ZXIC_COMM_MEMSET(&g_vport_mgr[slot][channel_id], 0x00, sizeof(DPP_VPORT_MGR_T));

    return DPP_OK;
}

ZXIC_UINT32 dpp_vport_table_lock(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_MUTEX_T** table_lock)
{
    ZXIC_UINT16 slot = 0;
    ZXIC_UINT16 channel_id = 0;
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_INDEX(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_POINT(table_lock);

    *table_lock = NULL;

    slot = pf_info->slot;
    ZXIC_COMM_CHECK_INDEX(slot, 0, DPP_PCIE_SLOT_MAX - 1);

    channel_id = DPP_PCIE_CHANNEL_ID(pf_info->vport);
    ZXIC_COMM_CHECK_INDEX(channel_id, 0, DPP_PCIE_CHANNEL_MAX - 1);

    rc = zxic_comm_mutex_lock(g_vport_mgr[slot][channel_id].table_lock[sdt_no]);
    ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_lock");

    *table_lock = g_vport_mgr[slot][channel_id].table_lock[sdt_no];

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u table lock.\n",
                                                         pf_info->slot, pf_info->vport, sdt_no);
    return DPP_OK;
}

ZXIC_UINT32 dpp_vport_table_unlock(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT16 slot = 0;
    ZXIC_UINT16 channel_id = 0;
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_INDEX(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    slot = pf_info->slot;
    ZXIC_COMM_CHECK_INDEX(slot, 0, DPP_PCIE_SLOT_MAX - 1);

    channel_id = DPP_PCIE_CHANNEL_ID(pf_info->vport);
    ZXIC_COMM_CHECK_INDEX(channel_id, 0, DPP_PCIE_CHANNEL_MAX - 1);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u table unlock.\n",
                                                           pf_info->slot, pf_info->vport, sdt_no);

    rc = zxic_comm_mutex_unlock(g_vport_mgr[slot][channel_id].table_lock[sdt_no]);
    ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_unlock");
    
    return DPP_OK;
}

ZXIC_UINT32 dpp_vport_bc_table_get(DPP_PF_INFO_T* pf_info, DPP_VPORT_BC_TABLE_T** bc_table)
{
    ZXIC_UINT16 slot = 0;
    ZXIC_UINT16 channel_id = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(bc_table);

    slot = pf_info->slot;
    ZXIC_COMM_CHECK_INDEX(slot, 0, DPP_PCIE_SLOT_MAX - 1);

    channel_id = DPP_PCIE_CHANNEL_ID(pf_info->vport);
    ZXIC_COMM_CHECK_INDEX(channel_id, 0, DPP_PCIE_CHANNEL_MAX - 1);

    *bc_table = &g_vport_mgr[slot][channel_id].bc_table;

    return DPP_OK;
}

ZXIC_UINT32 dpp_vport_mc_table_get(DPP_PF_INFO_T* pf_info, DPP_VPORT_MC_TABLE_T** mc_table)
{
    ZXIC_UINT16 slot = 0;
    ZXIC_UINT16 channel_id = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(mc_table);

    slot = pf_info->slot;
    ZXIC_COMM_CHECK_INDEX(slot, 0, DPP_PCIE_SLOT_MAX - 1);

    channel_id = DPP_PCIE_CHANNEL_ID(pf_info->vport);
    ZXIC_COMM_CHECK_INDEX(channel_id, 0, DPP_PCIE_CHANNEL_MAX - 1);

    *mc_table = &g_vport_mgr[slot][channel_id].mc_table;

    return DPP_OK;
}

ZXIC_UINT32 dpp_vport_uc_promisc_table_get(DPP_PF_INFO_T* pf_info, DPP_VPORT_PROMISC_TABLE_T** promisc_table)
{
    ZXIC_UINT16 slot = 0;
    ZXIC_UINT16 channel_id = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(promisc_table);

    slot = pf_info->slot;
    ZXIC_COMM_CHECK_INDEX(slot, 0, DPP_PCIE_SLOT_MAX - 1);

    channel_id = DPP_PCIE_CHANNEL_ID(pf_info->vport);
    ZXIC_COMM_CHECK_INDEX(channel_id, 0, DPP_PCIE_CHANNEL_MAX - 1);

    *promisc_table = &g_vport_mgr[slot][channel_id].uc_promisc_table;

    return DPP_OK;
}

ZXIC_UINT32 dpp_vport_mc_promisc_table_get(DPP_PF_INFO_T* pf_info, DPP_VPORT_PROMISC_TABLE_T** promisc_table)
{
    ZXIC_UINT16 slot = 0;
    ZXIC_UINT16 channel_id = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(promisc_table);

    slot = pf_info->slot;
    ZXIC_COMM_CHECK_INDEX(slot, 0, DPP_PCIE_SLOT_MAX - 1);

    channel_id = DPP_PCIE_CHANNEL_ID(pf_info->vport);
    ZXIC_COMM_CHECK_INDEX(channel_id, 0, DPP_PCIE_CHANNEL_MAX - 1);

    *promisc_table = &g_vport_mgr[slot][channel_id].mc_promisc_table;

    return DPP_OK;
}

ZXIC_UINT32 dpp_vport_get_by_vqm_vfid(ZXIC_UINT16 pf_vport, ZXIC_UINT32 vqm_vfid, ZXIC_UINT16* vport)
{
    ZXIC_COMM_CHECK_POINT(vport);

    if (vqm_vfid >= PF_VQM_VFID_OFFSET)
    {
        *vport = pf_vport;
    }
    else
    {
        *vport = ((EPID(pf_vport) << 12) | 0x800 | (FUNC_NUM(pf_vport) << 8) | (vqm_vfid - (EPID(pf_vport) * 256)));
    }
    return DPP_OK;
}

ZXIC_UINT32 dpp_vport_get_by_mc_bitmap(ZXIC_UINT16 pf_vport, ZXIC_UINT32 group_id, ZXIC_UINT64 mc_bitmap,
                                                         ZXIC_UINT16 vport[64], ZXIC_UINT32* p_vport_num)
{
    ZXIC_UINT32 i = 0;

    ZXIC_UINT32 vport_num = 0;

    ZXIC_COMM_CHECK_POINT(vport);
    ZXIC_COMM_CHECK_POINT(p_vport_num);

    for (i = 0; i < MC_MEMBER_NUM_IN_GROUP; i++)
    {
        if ((mc_bitmap >> i) & 1)
        {
            vport[vport_num] = ((EPID(pf_vport) << 12) | 0x800 | (FUNC_NUM(pf_vport) << 8) |
                                ((group_id * MC_MEMBER_NUM_IN_GROUP) + MC_MEMBER_NUM_IN_GROUP - 1 - i));
            vport_num ++;
        }
    }

    *p_vport_num = vport_num;

    return DPP_OK;
}

BOOLEAN dpp_vport_in_mc_bitmap(ZXIC_UINT32 vport,ZXIC_UINT64 mc_bitmap)
{
    ZXIC_UINT32 bit_index  = 0;

    bit_index = VFUNC_NUM(vport) % MC_MEMBER_NUM_IN_GROUP;

    if((VF_ACTIVE(vport)) 
        && ((mc_bitmap >> (MC_MEMBER_NUM_IN_GROUP - 1 - bit_index)) & 1))
    {
        return TRUE;
    }

    return FALSE;
}

ZXIC_UINT32 dpp_acl_index_request(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 *p_index)
{
    DPP_DEV_T dev = {0};

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
EXPORT_SYMBOL(dpp_acl_index_request);

ZXIC_UINT32 dpp_acl_index_release(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 index)
{
    DPP_DEV_T dev = {0};

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
EXPORT_SYMBOL(dpp_acl_index_release);

ZXIC_UINT32 dpp_acl_index_unused_num(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 *p_unused_num)
{
    DPP_DEV_T dev = {0};
    ZXIC_UINT32 queue = 0;

    ZXIC_UINT32 rc     = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_unused_num);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_dtb_acl_index_unused_num(&dev, queue, sdt_no, p_unused_num);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_index_unused_num", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u unused_num: %u.\n",
                    pf_info->slot, pf_info->vport, sdt_no, *p_unused_num);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_acl_index_unused_num);

ZXIC_UINT32 dpp_acl_index_dump(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 *index_num, ZXIC_UINT32 *p_index_array)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue  = 0;
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
EXPORT_SYMBOL(dpp_acl_index_dump);

ZXIC_UINT32 dpp_acl_index_max_num(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 *p_max_num)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 eram_sdt_no = 0;
    DPP_DEV_T dev = {0};
    DPP_SDTTBL_ERAM_T sdt_eram = {0};  /*SDT内容*/

    ZXIC_COMM_CHECK_POINT(p_max_num);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    eram_sdt_no = dpp_apt_get_sdt_partner(&dev, sdt_no);
    ZXIC_COMM_CHECK_INDEX(eram_sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    rc = dpp_soft_sdt_tbl_get(&dev, eram_sdt_no, &sdt_eram);
    ZXIC_COMM_CHECK_RC(rc, "dpp_soft_sdt_tbl_get");

    *p_max_num = sdt_eram.eram_table_depth;
    return DPP_OK;
}
EXPORT_SYMBOL(dpp_acl_index_max_num);

ZXIC_UINT32 dpp_hash_item_num_by_soft(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 *p_item_num)
{
    ZXIC_UINT32 rc = DPP_OK;
    DPP_DEV_T dev = {0};

    ZXIC_COMM_CHECK_POINT(p_item_num);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_hash_item_num_by_soft(&dev, sdt_no, p_item_num);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_hash_item_num_by_soft");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_hash_item_num_by_soft);
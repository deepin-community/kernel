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
#include "dpp_tbl_api.h"
#include "dpp_tbl_comm.h"
#include "dpp_tbl_promisc.h"

ZXIC_UINT32 dpp_vport_promisc_info_set(DPP_PF_INFO_T* pf_info, DPP_VPORT_PROMISC_TABLE_T* promisc_table, ZXIC_UINT32 enable)
{
    ZXIC_UINT32 group_id  = 0;
    ZXIC_UINT32 vfunc_num = 0;
    ZXIC_UINT64 bitmap_mask = 0;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(promisc_table);
    ZXIC_COMM_CHECK_INDEX(enable, 0, 1);

    vfunc_num = VFUNC_NUM(pf_info->vport);
    ZXIC_COMM_CHECK_INDEX(vfunc_num, 0, (PROMISC_GROUP_NUM * PROMISC_MEMBER_NUM_IN_GROUP) - 1);

    group_id  = vfunc_num / PROMISC_MEMBER_NUM_IN_GROUP;
    ZXIC_COMM_CHECK_INDEX(group_id, 0, PROMISC_GROUP_NUM - 1);

    bitmap_mask = ((ZXIC_UINT64)(1) << (PROMISC_MEMBER_NUM_IN_GROUP - 1 - (vfunc_num % PROMISC_MEMBER_NUM_IN_GROUP)));

    if (IS_PF(pf_info->vport))
    {
        promisc_table->promisc_info.pf_enable = enable;
    }
    else
    {
        promisc_table->promisc_info.bitmap[group_id] = (enable == 1) ?
                                                       (promisc_table->promisc_info.bitmap[group_id] | bitmap_mask) :
                                                       (promisc_table->promisc_info.bitmap[group_id] & ~bitmap_mask);
    }

    return DPP_OK;
}

ZXIC_UINT32 dpp_vport_promisc_table_insert(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, DPP_VPORT_PROMISC_TABLE_T* promisc_table)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 group_id = 0;
    ZXIC_UINT32 index    = 0;
    ZXIC_UINT32 queue    = 0;
    ZXIC_UINT32 rc       = DPP_OK;

    ZXDH_PROMISC_T promisc_entry   = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(promisc_table);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    for (group_id = 0; group_id < PROMISC_GROUP_NUM; group_id++)
    {
        index = (((OWNER_PF_VQM_VFID(pf_info->vport) - PF_VQM_VFID_OFFSET) << 2)| group_id);
        promisc_entry.hit_flag = 1;
        promisc_entry.pf_enable = promisc_table->promisc_info.pf_enable;
        promisc_entry.bitmap = promisc_table->promisc_info.bitmap[group_id];

        rc = dpp_apt_dtb_eram_insert(&dev, queue, sdt_no, index, &promisc_entry);
        ZXIC_COMM_CHECK_RC(rc, "dpp_apt_dtb_eram_insert");

        ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u group_id: %u index: 0x%02x.\n",
                                        pf_info->slot, pf_info->vport, sdt_no, group_id, index);
        ZXIC_COMM_TRACE_NOTICE("pf_enable: %u bitmap: %02x %02x %02x %02x %02x %02x %02x %02x.\n",
                                                                  promisc_entry.pf_enable,
                                                                  *((ZXIC_UINT8*)(&promisc_entry.bitmap) + 7),
                                                                  *((ZXIC_UINT8*)(&promisc_entry.bitmap) + 6),
                                                                  *((ZXIC_UINT8*)(&promisc_entry.bitmap) + 5),
                                                                  *((ZXIC_UINT8*)(&promisc_entry.bitmap) + 4),
                                                                  *((ZXIC_UINT8*)(&promisc_entry.bitmap) + 3),
                                                                  *((ZXIC_UINT8*)(&promisc_entry.bitmap) + 2),
                                                                  *((ZXIC_UINT8*)(&promisc_entry.bitmap) + 1),
                                                                  *((ZXIC_UINT8*)(&promisc_entry.bitmap) + 0));
    }

    return DPP_OK;
}

ZXIC_UINT32 dpp_vport_uc_promisc_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 enable)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 sdt_no = ZXDH_SDT_UC_PROMISC_TABLE;
    ZXIC_UINT32 rc = DPP_OK;
    DPP_VPORT_PROMISC_TABLE_T* promisc_table = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x enable: %u start.\n",
                                            pf_info->slot, pf_info->vport, enable);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_uc_promisc_table_get(pf_info, &promisc_table);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_vport_uc_promisc_table_get", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_promisc_info_set(pf_info, promisc_table, enable);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_vport_promisc_info_set", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_promisc_table_insert(pf_info, sdt_no, promisc_table);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_vport_promisc_table_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x enable: %u success.\n",
                                            pf_info->slot, pf_info->vport, enable);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_vport_uc_promisc_set);

ZXIC_UINT32 dpp_vport_mc_promisc_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 enable)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 sdt_no = ZXDH_SDT_MC_PROMISC_TABLE;
    ZXIC_UINT32 rc = DPP_OK;
    DPP_VPORT_PROMISC_TABLE_T* promisc_table = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x enable: %u start.\n",
                                            pf_info->slot, pf_info->vport, enable);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_mc_promisc_table_get(pf_info, &promisc_table);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_vport_mc_promisc_table_get", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_promisc_info_set(pf_info, promisc_table, enable);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_vport_promisc_info_set", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_promisc_table_insert(pf_info, sdt_no, promisc_table);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_vport_promisc_table_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x enable: %u success.\n",
                                            pf_info->slot, pf_info->vport, enable);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_vport_mc_promisc_set);

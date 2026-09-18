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
#include "dpp_tbl_bc.h"

ZXIC_UINT32 dpp_vport_bc_info_add(DPP_PF_INFO_T* pf_info)
{
    ZXIC_UINT32 group_id  = 0;
    ZXIC_UINT32 vfunc_num = 0;
    ZXIC_UINT32 rc        = DPP_OK;

    DPP_VPORT_BC_TABLE_T* bc_table = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);

    rc = dpp_vport_bc_table_get(pf_info, &bc_table);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_bc_table_get");
    ZXIC_COMM_CHECK_POINT(bc_table);

    vfunc_num = VFUNC_NUM(pf_info->vport);
    ZXIC_COMM_CHECK_INDEX(vfunc_num, 0, (BC_GROUP_NUM * BC_MEMBER_NUM_IN_GROUP) - 1);

    group_id  = vfunc_num / BC_MEMBER_NUM_IN_GROUP;
    ZXIC_COMM_CHECK_INDEX(group_id, 0, BC_GROUP_NUM - 1);

    bc_table->bc_info.bc_bitmap[group_id] |= ((ZXIC_UINT64)(1) << (BC_MEMBER_NUM_IN_GROUP - 1 -
                                                                  (vfunc_num % BC_MEMBER_NUM_IN_GROUP)));

    return DPP_OK;
}

ZXIC_UINT32 dpp_vport_bc_info_del(DPP_PF_INFO_T* pf_info)
{
    ZXIC_UINT32 group_id  = 0;
    ZXIC_UINT32 vfunc_num = 0;
    ZXIC_UINT32 rc        = DPP_OK;

    DPP_VPORT_BC_TABLE_T* bc_table = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);

    rc = dpp_vport_bc_table_get(pf_info, &bc_table);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_bc_table_get");
    ZXIC_COMM_CHECK_POINT(bc_table);

    vfunc_num = VFUNC_NUM(pf_info->vport);
    ZXIC_COMM_CHECK_INDEX(vfunc_num, 0, (BC_GROUP_NUM * BC_MEMBER_NUM_IN_GROUP) - 1);

    group_id  = vfunc_num / BC_MEMBER_NUM_IN_GROUP;
    ZXIC_COMM_CHECK_INDEX(group_id, 0, BC_GROUP_NUM - 1);

    bc_table->bc_info.bc_bitmap[group_id] &= ~((ZXIC_UINT64)(1) << (BC_MEMBER_NUM_IN_GROUP - 1 -
                                                                   (vfunc_num % BC_MEMBER_NUM_IN_GROUP)));

    return DPP_OK;
}

ZXIC_UINT32 dpp_vport_bc_info_clear_all(DPP_PF_INFO_T* pf_info)
{
    ZXIC_UINT32 group_id = 0;
    ZXIC_UINT32 rc       = DPP_OK;

    DPP_VPORT_BC_TABLE_T* bc_table = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);

    rc = dpp_vport_bc_table_get(pf_info, &bc_table);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_bc_table_get");
    ZXIC_COMM_CHECK_POINT(bc_table);

    for (group_id = 0; group_id < BC_GROUP_NUM; group_id++)
    {
        bc_table->bc_info.bc_bitmap[group_id] = 0;
    }

    return DPP_OK;
}

ZXIC_UINT32 dpp_vport_bc_table_insert(DPP_PF_INFO_T* pf_info)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 group_id = 0;
    ZXIC_UINT32 index    = 0;
    ZXIC_UINT32 sdt_no   = ZXDH_SDT_BC_TABLE;
    ZXIC_UINT32 queue    = 0;
    ZXIC_UINT32 rc       = DPP_OK;

    ZXDH_BC_T bc_entry   = {0};
    DPP_VPORT_BC_TABLE_T *bc_table = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_bc_table_get(pf_info, &bc_table);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_bc_table_get");
    ZXIC_COMM_CHECK_POINT(bc_table);

    for (group_id = 0; group_id < BC_GROUP_NUM; group_id++)
    {
        index = (((OWNER_PF_VQM_VFID(pf_info->vport) - PF_VQM_VFID_OFFSET) << 2)| group_id);
        bc_entry.hit_flag = 1;
        bc_entry.bc_bitmap = bc_table->bc_info.bc_bitmap[group_id];

        rc = dpp_apt_dtb_eram_insert(&dev, queue, sdt_no, index, &bc_entry);
        ZXIC_COMM_CHECK_RC(rc, "dpp_apt_dtb_eram_insert");

        ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u group_id: %u index: 0x%02x.\n",
                                                     pf_info->slot, pf_info->vport, sdt_no, group_id, index);
        ZXIC_COMM_TRACE_NOTICE("bc_bitmap: %02x %02x %02x %02x %02x %02x %02x %02x.\n",
                                                                  *((ZXIC_UINT8*)(&bc_entry.bc_bitmap) + 7),
                                                                  *((ZXIC_UINT8*)(&bc_entry.bc_bitmap) + 6),
                                                                  *((ZXIC_UINT8*)(&bc_entry.bc_bitmap) + 5),
                                                                  *((ZXIC_UINT8*)(&bc_entry.bc_bitmap) + 4),
                                                                  *((ZXIC_UINT8*)(&bc_entry.bc_bitmap) + 3),
                                                                  *((ZXIC_UINT8*)(&bc_entry.bc_bitmap) + 2),
                                                                  *((ZXIC_UINT8*)(&bc_entry.bc_bitmap) + 1),
                                                                  *((ZXIC_UINT8*)(&bc_entry.bc_bitmap) + 0));
    }

    return DPP_OK;
}

ZXIC_UINT32 dpp_vport_bond_pf(DPP_PF_INFO_T* pf_info)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 sdt_no = ZXDH_SDT_BC_TABLE;
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x start.\n",
                                                            pf_info->slot, pf_info->vport);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    if (IS_PF(pf_info->vport))
    {
        rc = dpp_vport_bc_info_clear_all(pf_info);
        ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_vport_bc_info_clear_all", DEV_PCIE_LOCK(&dev));
    }
    else
    {
        rc = dpp_vport_bc_info_add(pf_info);
        ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_vport_bc_info_add", DEV_PCIE_LOCK(&dev));
    }

    rc = dpp_vport_bc_table_insert(pf_info);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_vport_bc_table_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x success.\n", pf_info->slot, pf_info->vport);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_vport_bond_pf);

ZXIC_UINT32 dpp_vport_unbond_pf(DPP_PF_INFO_T* pf_info)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 sdt_no = ZXDH_SDT_BC_TABLE;
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_INDEX_EQUAL(IS_PF(pf_info->vport), 1);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x start.\n", pf_info->slot, pf_info->vport);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_bc_info_del(pf_info);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_vport_bc_info_del", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_bc_table_insert(pf_info);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_vport_bc_table_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x success.\n", pf_info->slot, pf_info->vport);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_vport_unbond_pf);

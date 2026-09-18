#include "dpp_drv_init.h"
#include "dpp_drv_acl.h"
#include "dpp_drv_hash.h"
#include "dpp_drv_eram.h"
#include "dpp_drv_sdt.h"
#include "dpp_dev.h"
#include "dpp_ppu.h"
#include "dpp_dtb.h"
#include "dpp_hash.h"
#include "dpp_dtb_table.h"
#include "dpp_dtb_table_api.h"
#include "dpp_tbl_comm.h"
#include "dpp_tbl_qid.h"

ZXIC_UINT32 dpp_rxfh_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *queue_list, ZXIC_UINT32 queue_num)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 i        = 0;
    ZXIC_UINT32 queue    = 0;
    ZXIC_UINT32 group_id = 0;
    ZXIC_UINT32 sdt_no   = ZXDH_SDT_RSS_TO_VQID_TABLE;
    ZXIC_UINT32 index    = 0;
    ZXIC_UINT32 rc       = DPP_OK;

    ZXDH_RSS_TO_VQID_T rss_to_vqid_entry = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(queue_list);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x start.\n", pf_info->slot, pf_info->vport);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    index = VQM_VFID(pf_info->vport) * RSS_TO_VQID_GROUP_NUM;
    rss_to_vqid_entry.hit_flag = 1;

    for (group_id = 0; group_id < RSS_TO_VQID_GROUP_NUM; group_id++)
    {
        for (i = 0; i < 8; i++)
        {
            rss_to_vqid_entry.vqm_qid[i] = queue_list[((group_id * 8) + i) % queue_num];
        }

        rc = dpp_apt_dtb_eram_insert(&dev, queue, sdt_no, index + group_id, &rss_to_vqid_entry);
        ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_eram_insert", DEV_PCIE_LOCK(&dev));

        ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u index: 0x%04x.\n",
                                                    pf_info->slot, pf_info->vport, sdt_no, index + group_id);

        ZXIC_COMM_TRACE_NOTICE("vqm_qid0: 0x%04x vqm_qid1: 0x%04x vqm_qid2: 0x%04x vqm_qid3: 0x%04x.\n",
                                                               rss_to_vqid_entry.vqm_qid[0], rss_to_vqid_entry.vqm_qid[1],
                                                               rss_to_vqid_entry.vqm_qid[2], rss_to_vqid_entry.vqm_qid[3]);
        ZXIC_COMM_TRACE_NOTICE("vqm_qid4: 0x%04x vqm_qid5: 0x%04x vqm_qid6: 0x%04x vqm_qid7: 0x%04x.\n",
                                                               rss_to_vqid_entry.vqm_qid[4], rss_to_vqid_entry.vqm_qid[5],
                                                               rss_to_vqid_entry.vqm_qid[6], rss_to_vqid_entry.vqm_qid[7]);
    }

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x success.\n", pf_info->slot, pf_info->vport);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_rxfh_set);

ZXIC_UINT32 dpp_rxfh_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 *queue_list, ZXIC_UINT32 queue_num)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 i        = 0;
    ZXIC_UINT32 queue = 0;
    ZXIC_UINT32 group_id = 0;
    ZXIC_UINT32 sdt_no   = ZXDH_SDT_RSS_TO_VQID_TABLE;
    ZXIC_UINT32 index    = 0;
    ZXIC_UINT32 rc       = DPP_OK;

    ZXDH_RSS_TO_VQID_T rss_to_vqid_entry = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(queue_list);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x start.\n", pf_info->slot, pf_info->vport);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    index = VQM_VFID(pf_info->vport) * RSS_TO_VQID_GROUP_NUM;
    for (group_id = 0; group_id < RSS_TO_VQID_GROUP_NUM; group_id++)
    {
        rc = dpp_apt_dtb_eram_get(&dev, queue, sdt_no, index + group_id, &rss_to_vqid_entry);
        ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_eram_get", DEV_PCIE_LOCK(&dev));
        ZXIC_COMM_CHECK_INDEX_NOT_EQUAL_UNLOCK(rss_to_vqid_entry.hit_flag, 1, DEV_PCIE_LOCK(&dev));

        for (i = 0; i < 8; i++)
        {
            queue_list[((group_id * 8) + i) % queue_num] = rss_to_vqid_entry.vqm_qid[i];
        }
    }

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x success.\n", pf_info->slot, pf_info->vport);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_rxfh_get);

ZXIC_UINT32 dpp_rxfh_del(DPP_PF_INFO_T* pf_info)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue    = 0;
    ZXIC_UINT32 group_id = 0;
    ZXIC_UINT32 sdt_no   = ZXDH_SDT_RSS_TO_VQID_TABLE;
    ZXIC_UINT32 index    = 0;
    ZXIC_UINT32 rc       = DPP_OK;

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x start.\n", pf_info->slot, pf_info->vport);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    index = VQM_VFID(pf_info->vport) * RSS_TO_VQID_GROUP_NUM;
    for (group_id = 0; group_id < RSS_TO_VQID_GROUP_NUM; group_id++)
    {
        rc = dpp_apt_dtb_eram_clear(&dev, queue, sdt_no, index + group_id);
        ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_eram_clear", DEV_PCIE_LOCK(&dev));
    }

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x success.\n", pf_info->slot, pf_info->vport);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_rxfh_del);

ZXIC_UINT32 dpp_thash_key_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 *hash_key, ZXIC_UINT32 key_num)
{
    ZXIC_UINT32 rc = DPP_OK;

    DPP_DEV_T dev = {0};
    DPP_PPU_PPU_COP_THASH_RSK_T *thash = (DPP_PPU_PPU_COP_THASH_RSK_T*)hash_key;

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_CHECK_POINT(thash);
    ZXIC_COMM_CHECK_INDEX_LOWER(key_num, (ZXIC_UINT32)sizeof(DPP_PPU_PPU_COP_THASH_RSK_T));

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_ppu_ppu_cop_thash_rsk_set(&dev, thash);
    ZXIC_COMM_CHECK_RC(rc, "dpp_ppu_ppu_cop_thash_rsk_set");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_thash_key_set);

ZXIC_UINT32 dpp_thash_key_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT8 *hash_key, ZXIC_UINT32 key_num)
{
    ZXIC_UINT32 rc = DPP_OK;

    DPP_DEV_T dev = {0};
    DPP_PPU_PPU_COP_THASH_RSK_T *thash = (DPP_PPU_PPU_COP_THASH_RSK_T*)hash_key;

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_CHECK_POINT(thash);
    ZXIC_COMM_CHECK_INDEX_LOWER(key_num, (ZXIC_UINT32)sizeof(DPP_PPU_PPU_COP_THASH_RSK_T));

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_ppu_ppu_cop_thash_rsk_get(&dev, thash);
    ZXIC_COMM_CHECK_RC(rc, "dpp_ppu_ppu_cop_thash_rsk_get");

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_thash_key_get);
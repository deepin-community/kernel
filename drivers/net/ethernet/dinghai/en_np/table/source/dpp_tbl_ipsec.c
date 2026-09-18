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
#include "dpp_tbl_ipsec.h"

ZXIC_UINT32 dpp_ipsec_enc_entry_add(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index, 
                                    ZXIC_UINT8 *sip, ZXIC_UINT8 *dip,
                                    ZXIC_UINT8 *sip_mask, ZXIC_UINT8 *dip_mask,
                                    ZXIC_UINT32 is_ipv4, ZXIC_UINT32 sa_id)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 sdt_no = ZXDH_SDT_IPSEC_ENC_TABLE;
    ZXIC_UINT32 rc     = DPP_OK;

    ZXDH_IPSEC_ENC_T ipsec_enc_entry = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(sip);
    ZXIC_COMM_CHECK_POINT(dip);
    ZXIC_COMM_CHECK_POINT(sip_mask);
    ZXIC_COMM_CHECK_POINT(dip_mask);

    ZXIC_COMM_MEMSET(&ipsec_enc_entry, 0, sizeof(ZXDH_IPSEC_ENC_T));

    ipsec_enc_entry.index = index;

    ZXIC_COMM_MEMCPY(ipsec_enc_entry.key.sip, sip, is_ipv4? 4 : 16);
    ZXIC_COMM_MEMCPY(ipsec_enc_entry.key.dip, dip, is_ipv4? 4 : 16);
    ZXIC_COMM_MEMCPY(ipsec_enc_entry.mask.sip, sip_mask, is_ipv4? 4 : 16);
    ZXIC_COMM_MEMCPY(ipsec_enc_entry.mask.dip, dip_mask, is_ipv4? 4 : 16);

    ipsec_enc_entry.entry.sa_id = sa_id;

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_apt_dtb_acl_entry_insert(&dev, queue, sdt_no, &ipsec_enc_entry);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_acl_entry_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u index: %u sa_id: %u is_ipv4: %u.\n",
                    pf_info->slot, pf_info->vport, sdt_no, index, sa_id, is_ipv4);

    ZXIC_COMM_TRACE_NOTICE("sip:\n");
    dpp_data_print(sip, is_ipv4? 4 : 16);

    ZXIC_COMM_TRACE_NOTICE("sip_mask:\n");
    dpp_data_print(sip_mask, is_ipv4? 4 : 16);

    ZXIC_COMM_TRACE_NOTICE("dip:\n");
    dpp_data_print(dip, is_ipv4? 4 : 16);

    ZXIC_COMM_TRACE_NOTICE("dip_mask:\n");
    dpp_data_print(dip_mask, is_ipv4? 4 : 16);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_ipsec_enc_entry_add);

ZXIC_UINT32 dpp_ipsec_enc_entry_del(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 index)
{
    DPP_DEV_T dev = {0};

    ZXIC_UINT32 queue  = 0;
    ZXIC_UINT32 sdt_no = ZXDH_SDT_IPSEC_ENC_TABLE;
    ZXIC_UINT32 rc     = DPP_OK;

    ZXDH_IPSEC_ENC_T ipsec_enc_entry = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ZXIC_COMM_MEMSET(&ipsec_enc_entry, 0, sizeof(ZXDH_IPSEC_ENC_T));
    ZXIC_COMM_MEMSET(&ipsec_enc_entry.mask, 0xFF, sizeof(ZXDH_IPSEC_ENC_MASK));

    ipsec_enc_entry.index = index;

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    rc = dpp_vport_table_lock(pf_info, sdt_no, &DEV_PCIE_LOCK(&dev));
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_lock");
    ZXIC_COMM_CHECK_POINT(DEV_PCIE_LOCK(&dev));

    rc = dpp_apt_dtb_acl_entry_insert(&dev, queue, sdt_no, &ipsec_enc_entry);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_apt_dtb_acl_entry_insert", DEV_PCIE_LOCK(&dev));

    rc = dpp_vport_table_unlock(pf_info, sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_vport_table_unlock");

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x sdt_no: %u index: %u.\n",
                    pf_info->slot, pf_info->vport, sdt_no, index);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_ipsec_enc_entry_del);

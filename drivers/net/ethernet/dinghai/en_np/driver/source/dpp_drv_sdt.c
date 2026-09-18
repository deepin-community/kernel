#include "dpp_dev.h"
#include "dpp_sdt.h"
#include "dpp_dtb.h"
#include "dpp_se_api.h"
#include "dpp_apt_se_api.h"
#include "dpp_dtb_table.h"
#include "dpp_dtb_table_api.h"
#include "dpp_drv_sdt.h"

DPP_STATUS dpp_sdt_tbl_type_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 *tbl_type)
{
    DPP_STATUS rc = DPP_OK;
    DPP_DEV_T dev = {0};

    DPP_SDT_TBL_DATA_T sdt_tbl = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(tbl_type);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_sdt_tbl_data_get(&dev, sdt_no, &sdt_tbl);
    ZXIC_COMM_CHECK_RC(rc, "dpp_sdt_tbl_data_get");

    ZXIC_COMM_UINT32_GET_BITS((*tbl_type), sdt_tbl.data_high32, DPP_SDT_H_TBL_TYPE_BT_POS, DPP_SDT_H_TBL_TYPE_BT_LEN);

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_sdt_tbl_type_get);

DPP_STATUS dpp_sdt_tbl_info_dump(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXDH_SDT_TBL_INFO_T *p_sdt_info)
{
    DPP_STATUS rc = DPP_OK;
    DPP_DEV_T dev = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(p_sdt_info);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_soft_sdt_tbl_get(&dev, sdt_no, &(p_sdt_info->sdt_tbl_info));
    ZXIC_COMM_CHECK_RC(rc, "dpp_soft_sdt_tbl_get");

    rc = dpp_sdt_tbl_type_get(pf_info, sdt_no, &(p_sdt_info->tbl_type));
    ZXIC_COMM_CHECK_RC(rc, "dpp_soft_sdt_tbl_get");

    switch (p_sdt_info->tbl_type)
    {
        case DPP_SDT_TBLT_eRAM:
            p_sdt_info->tbl_depth = p_sdt_info->sdt_tbl_info.eram.eram_table_depth;
            break;
        case DPP_SDT_TBLT_DDR3:
            p_sdt_info->tbl_depth = 0;
            break;
        case DPP_SDT_TBLT_HASH:
            rc = dpp_hash_max_item_num_get(&dev, sdt_no, &(p_sdt_info->tbl_depth));
            ZXIC_COMM_CHECK_RC(rc, "dpp_hash_max_item_num_get");
            break;
        case DPP_SDT_TBLT_LPM:
            p_sdt_info->tbl_depth = p_sdt_info->sdt_tbl_info.lpm.lpm_table_depth;
            break;
        case DPP_SDT_TBLT_eTCAM:
            p_sdt_info->tbl_depth = p_sdt_info->sdt_tbl_info.etcam.etcam_table_depth;
            break;
        case DPP_SDT_TBLT_PORTTBL:
            p_sdt_info->tbl_depth = 0;
            break;
        default:
            return DPP_ERR;
    }
    return DPP_OK;
}
EXPORT_SYMBOL(dpp_sdt_tbl_info_dump);

DPP_STATUS dpp_sdt_tbl_item_dump(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXDH_SDT_TBL_ITEM_DATA_T *data, ZXIC_UINT32 *num)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 tbl_type = 0;
    ZXIC_UINT32 queue_id = 0;

    DPP_DEV_T dev = {0};

    DPP_SDTTBL_ERAM_T sdt_tbl_eram = {0};
    DPP_DTB_ERAM_ENTRY_INFO_T *p_dump_eram_entry = NULL;

    ZXIC_UINT32 hash_max_item_num = 0;
    DPP_DTB_HASH_ENTRY_INFO_T *p_dump_hash_entry = NULL;

    DPP_SDTTBL_ETCAM_T sdt_tbl_etcam = {0};
    DPP_DTB_ACL_ENTRY_INFO_T *p_dump_etcam_entry = NULL;

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(data);
    ZXIC_COMM_CHECK_POINT(num);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_sdt_tbl_type_get(pf_info, sdt_no, &tbl_type);
    ZXIC_COMM_CHECK_RC(rc, "dpp_sdt_tbl_type_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    switch (tbl_type)
    {
        case DPP_SDT_TBLT_eRAM:
            rc = dpp_soft_sdt_tbl_get(&dev, sdt_no, &sdt_tbl_eram);
            ZXIC_COMM_CHECK_RC(rc, "dpp_soft_sdt_tbl_get");

            p_dump_eram_entry = (DPP_DTB_ERAM_ENTRY_INFO_T *)ZXIC_COMM_MALLOC(sdt_tbl_eram.eram_table_depth * sizeof(DPP_DTB_ERAM_ENTRY_INFO_T));
            ZXIC_COMM_CHECK_POINT(p_dump_eram_entry);
            ZXIC_COMM_MEMSET_S(p_dump_eram_entry, sdt_tbl_eram.eram_table_depth * sizeof(DPP_DTB_ERAM_ENTRY_INFO_T), 0,
                                                  sdt_tbl_eram.eram_table_depth * sizeof(DPP_DTB_ERAM_ENTRY_INFO_T));
            for (i = 0; i < sdt_tbl_eram.eram_table_depth; i++)
            {
                p_dump_eram_entry[i].p_data = (ZXIC_UINT32 *)(data[i].eram.rst);
            }

            rc = dpp_dtb_eram_dump(&dev, queue_id, sdt_no, (ZXIC_UINT8 *)p_dump_eram_entry, num);
            ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_dtb_eram_dump", p_dump_eram_entry);

            for (i = 0; i < (*num); i++)
            {
                data[i].eram.index = p_dump_eram_entry[i].index;
                zxic_comm_swap(data[i].eram.rst, ARRAY_SIZE(data[i].eram.rst));
            }
            ZXIC_COMM_FREE(p_dump_eram_entry);
            break;
        case DPP_SDT_TBLT_HASH:
            rc = dpp_hash_max_item_num_get(&dev, sdt_no, &hash_max_item_num);
            ZXIC_COMM_CHECK_RC(rc, "dpp_hash_max_item_num_get");

            p_dump_hash_entry = (DPP_DTB_HASH_ENTRY_INFO_T *)ZXIC_COMM_MALLOC(hash_max_item_num * sizeof(DPP_DTB_HASH_ENTRY_INFO_T));
            ZXIC_COMM_CHECK_POINT(p_dump_hash_entry);
            ZXIC_COMM_MEMSET_S(p_dump_hash_entry, hash_max_item_num * sizeof(DPP_DTB_HASH_ENTRY_INFO_T), 0,
                                                  hash_max_item_num * sizeof(DPP_DTB_HASH_ENTRY_INFO_T));
            for (i = 0; i < hash_max_item_num; i++)
            {
                p_dump_hash_entry[i].p_actu_key = data[i].hash.key;
                p_dump_hash_entry[i].p_rst = data[i].hash.rst;
            }

            rc = dpp_dtb_hash_dump(&dev, queue_id, sdt_no, (ZXIC_UINT8 *)p_dump_hash_entry, num);
            ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_dtb_hash_dump", p_dump_hash_entry);
            ZXIC_COMM_FREE(p_dump_hash_entry);
            break;
        case DPP_SDT_TBLT_eTCAM:
            rc = dpp_soft_sdt_tbl_get(&dev, sdt_no, &sdt_tbl_etcam);
            ZXIC_COMM_CHECK_RC(rc, "dpp_soft_sdt_tbl_get");

            p_dump_etcam_entry = (DPP_DTB_ACL_ENTRY_INFO_T *)ZXIC_COMM_MALLOC(sdt_tbl_etcam.etcam_table_depth * sizeof(DPP_DTB_ACL_ENTRY_INFO_T));
            ZXIC_COMM_CHECK_POINT(p_dump_etcam_entry);
            ZXIC_COMM_MEMSET_S(p_dump_etcam_entry, sdt_tbl_etcam.etcam_table_depth * sizeof(DPP_DTB_ACL_ENTRY_INFO_T), 0,
                                                   sdt_tbl_etcam.etcam_table_depth * sizeof(DPP_DTB_ACL_ENTRY_INFO_T));
            for (i = 0; i < sdt_tbl_etcam.etcam_table_depth; i++)
            {
                p_dump_etcam_entry[i].key_data = data[i].etcam.key_data;
                p_dump_etcam_entry[i].key_mask = data[i].etcam.key_mask;
                p_dump_etcam_entry[i].p_as_rslt = data[i].etcam.as_rst;
            }

            rc = dpp_dtb_acl_dump(&dev, queue_id, sdt_no, (ZXIC_UINT8 *)p_dump_etcam_entry, num);
            ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_dtb_acl_dump", p_dump_etcam_entry);

            for (i = 0; i < (*num); i++)
            {
                data[i].etcam.handle = p_dump_etcam_entry[i].handle;
                zxic_comm_swap(data[i].etcam.as_rst, ARRAY_SIZE(data[i].etcam.as_rst));
            }
            ZXIC_COMM_FREE(p_dump_etcam_entry);
            break;
        default:
            return DPP_ERR;
    }

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_sdt_tbl_item_dump);

DPP_STATUS dpp_sdt_tbl_item_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 index, ZXDH_SDT_TBL_ITEM_DATA_T *data)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 tbl_type = 0;
    ZXIC_UINT32 queue_id = 0;

    DPP_DEV_T dev = {0};
    DPP_DTB_ERAM_ENTRY_INFO_T dump_eram_entry = {0};
    DPP_DTB_ACL_ENTRY_INFO_T dump_etcam_entry = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(data);

    rc = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_get");

    rc = dpp_sdt_tbl_type_get(pf_info, sdt_no, &tbl_type);
    ZXIC_COMM_CHECK_RC(rc, "dpp_sdt_tbl_type_get");

    rc = dpp_dtb_queue_id_get(&dev, &queue_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");

    switch (tbl_type)
    {
        case DPP_SDT_TBLT_eRAM:
            dump_eram_entry.index = index;
            dump_eram_entry.p_data = (ZXIC_UINT32 *)(data->eram.rst);

            rc = dpp_dtb_eram_data_get(&dev, queue_id, sdt_no, &dump_eram_entry);
            ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_eram_data_get");

            data->eram.index = dump_eram_entry.index;
            zxic_comm_swap(data->eram.rst, ARRAY_SIZE(data->eram.rst));

            break;
        case DPP_SDT_TBLT_eTCAM:
            dump_etcam_entry.handle = index;
            dump_etcam_entry.key_data = data->etcam.key_data;
            dump_etcam_entry.key_mask = data->etcam.key_mask;
            dump_etcam_entry.p_as_rslt = data->etcam.as_rst;

            rc = dpp_dtb_etcam_data_get(&dev, queue_id, sdt_no, &dump_etcam_entry);
            ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_etcam_data_get");

            data->etcam.handle = dump_etcam_entry.handle;
            zxic_comm_swap(data->etcam.as_rst, ARRAY_SIZE(data->etcam.as_rst));

            break;
        default:
            return DPP_ERR;
    }

    return DPP_OK;
}
EXPORT_SYMBOL(dpp_sdt_tbl_item_get);

/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_sdt.c
* 文件标识 : sdt配置接口实现文件
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/

#include "zxic_common.h"

#include "dpp_dev.h"
#include "dpp_se.h"
#include "dpp_sdt_def.h"
#include "dpp_sdt_mgr.h"
#include "dpp_sdt.h"

/** 获取低n位数据 */
#define DPP_SDT_GET_LOW_DATA(source_value, low_width)        (source_value &((1<<low_width)-1))

#if ZXIC_REAL("function for FCM_FTM ")
/***********************************************************/
/** 初始化SDT表配置管理
* @param   dev_num       设备数目
* @param   dev_id_array  设备dev_id数组
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_sdt_init(ZXIC_UINT32 dev_num, ZXIC_UINT32 *dev_id_array)
{
    DPP_STATUS rc = 0;
    ZXIC_UINT32 i = 0;

    ZXIC_COMM_CHECK_INDEX(dev_num, 1, DPP_DEV_CHANNEL_MAX);
    ZXIC_COMM_CHECK_POINT(dev_id_array);

    for (i = 0; i < dev_num; i++)
    {  
        ZXIC_COMM_CHECK_INDEX(*(dev_id_array + i), 0, DPP_DEV_CHANNEL_MAX - 1);
    }

    dpp_sdt_mgr_init();

    for (i = 0; i < dev_num; i++)
    {
        rc = dpp_sdt_mgr_create(dev_id_array[i]);
        ZXIC_COMM_CHECK_RC(rc, "dpp_sdt_mgr_create");
    }

    return DPP_OK;
}

/***********************************************************/
/** 解析从硬件读取的64bit SDT属性
* @param   sdt_hig32   硬件表中存储的SDT属性高32bit
* @param   sdt_low32   硬件表中存储的SDT属性低32bit
* @param   p_sdt_info  解析之后的SDT属性，根据SDT属性中的table_type确定此ZXIC_VOID型指针对应的数据结构, 包括: \n
*                      DPP_SDTTBL_ERAM_T、DPP_SDTTBL_DDR3_T、DPP_SDTTBL_HASH_T、DPP_SDTTBL_LPM_T、\n
*                      DPP_SDTTBL_ETCAM_T、DPP_SDTTBL_PORTTBL_T。
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_sdt_tbl_data_parser(DPP_DEV_T *dev, ZXIC_UINT32 sdt_hig32, ZXIC_UINT32 sdt_low32, ZXIC_VOID *p_sdt_info)
{
    ZXIC_UINT32 tmp = 0;
    ZXIC_UINT32 tbl_type = 0;
    ZXIC_UINT32 clutch_en = 0;

    DPP_SDTTBL_ERAM_T *p_sdt_eram = NULL;
    DPP_SDTTBL_DDR3_T *p_sdt_ddr3 = NULL;
    DPP_SDTTBL_HASH_T *p_sdt_hash = NULL;
    DPP_SDTTBL_LPM_T *p_sdt_lpm = NULL;
    DPP_SDTTBL_ETCAM_T *p_sdt_etcam = NULL;
    DPP_SDTTBL_PORTTBL_T *p_sdt_porttbl = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_sdt_info);

    ZXIC_COMM_UINT32_GET_BITS(tbl_type,  sdt_hig32, DPP_SDT_H_TBL_TYPE_BT_POS,     DPP_SDT_H_TBL_TYPE_BT_LEN);
    ZXIC_COMM_UINT32_GET_BITS(clutch_en, sdt_low32, DPP_SDT_L_CLUTCH_EN_BT_POS,    DPP_SDT_L_CLUTCH_EN_BT_LEN);

    /* 根据表类型解析数据 */
    switch (tbl_type)
    {
        case DPP_SDT_TBLT_eRAM:
        {
            p_sdt_eram = (DPP_SDTTBL_ERAM_T *)p_sdt_info;
            p_sdt_eram->table_type = tbl_type;
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_eram->eram_mode,          sdt_hig32, DPP_SDT_H_ERAM_MODE_BT_POS,          DPP_SDT_H_ERAM_MODE_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_eram->eram_base_addr,     sdt_hig32, DPP_SDT_H_ERAM_BASE_ADDR_BT_POS,     DPP_SDT_H_ERAM_BASE_ADDR_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_eram->eram_table_depth,   sdt_low32, DPP_SDT_L_ERAM_TABLE_DEPTH_BT_POS,   DPP_SDT_L_ERAM_TABLE_DEPTH_BT_LEN);
            p_sdt_eram->eram_clutch_en = clutch_en;
            break;
        }

        case DPP_SDT_TBLT_DDR3:
        {
            p_sdt_ddr3 = (DPP_SDTTBL_DDR3_T *)p_sdt_info;
            p_sdt_ddr3->table_type = tbl_type;
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_ddr3->ddr3_base_addr,     sdt_hig32, DPP_SDT_H_DDR3_BASE_ADDR_BT_POS,   DPP_SDT_H_DDR3_BASE_ADDR_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_ddr3->ddr3_share_type,    sdt_hig32, DPP_SDT_H_DDR3_SHARE_TYPE_BT_POS,  DPP_SDT_H_DDR3_SHARE_TYPE_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_ddr3->ddr3_rw_len,        sdt_hig32, DPP_SDT_H_DDR3_RW_LEN_BT_POS,      DPP_SDT_H_DDR3_RW_LEN_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(tmp,                            sdt_hig32, DPP_SDT_H_DDR3_SDT_NUM_BT_POS,     DPP_SDT_H_DDR3_SDT_NUM_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_ddr3->ddr3_sdt_num,       sdt_low32, DPP_SDT_L_DDR3_SDT_NUM_BT_POS,     DPP_SDT_L_DDR3_SDT_NUM_BT_LEN);
            p_sdt_ddr3->ddr3_sdt_num += (tmp << DPP_SDT_L_DDR3_SDT_NUM_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_ddr3->ddr3_ecc_en,        sdt_low32, DPP_SDT_L_DDR3_ECC_EN_BT_POS,      DPP_SDT_L_DDR3_ECC_EN_BT_LEN);
            p_sdt_ddr3->ddr3_clutch_en = clutch_en;
            break;
        }

        case DPP_SDT_TBLT_HASH:
        {
            p_sdt_hash = (DPP_SDTTBL_HASH_T *)p_sdt_info;
            p_sdt_hash->table_type = tbl_type;
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_hash->hash_id,            sdt_hig32, DPP_SDT_H_HASH_ID_BT_POS,            DPP_SDT_H_HASH_ID_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_hash->hash_table_width,   sdt_hig32, DPP_SDT_H_HASH_TABLE_WIDTH_BT_POS,   DPP_SDT_H_HASH_TABLE_WIDTH_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_hash->key_size,           sdt_hig32, DPP_SDT_H_HASH_KEY_SIZE_BT_POS,      DPP_SDT_H_HASH_KEY_SIZE_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_hash->hash_table_id,      sdt_hig32, DPP_SDT_H_HASH_TABLE_ID_BT_POS,      DPP_SDT_H_HASH_TABLE_ID_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_hash->learn_en,           sdt_hig32, DPP_SDT_H_LEARN_EN_BT_POS,           DPP_SDT_H_LEARN_EN_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_hash->keep_alive,         sdt_hig32, DPP_SDT_H_KEEP_ALIVE_BT_POS,         DPP_SDT_H_KEEP_ALIVE_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(tmp,                            sdt_hig32, DPP_SDT_H_KEEP_ALIVE_BADDR_BT_POS,   DPP_SDT_H_KEEP_ALIVE_BADDR_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_hash->keep_alive_baddr,   sdt_low32, DPP_SDT_L_KEEP_ALIVE_BADDR_BT_POS,   DPP_SDT_L_KEEP_ALIVE_BADDR_BT_LEN);
            p_sdt_hash->keep_alive_baddr += (tmp << DPP_SDT_L_KEEP_ALIVE_BADDR_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_hash->rsp_mode,           sdt_low32, DPP_SDT_L_RSP_MODE_BT_POS,           DPP_SDT_L_RSP_MODE_BT_LEN);
            p_sdt_hash->hash_clutch_en = clutch_en;
            break;
        }

        case DPP_SDT_TBLT_LPM:
        {
            p_sdt_lpm = (DPP_SDTTBL_LPM_T *)p_sdt_info;
            p_sdt_lpm->table_type = tbl_type;
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_lpm->lpm_v46_id,          sdt_hig32, DPP_SDT_H_LPM_V46ID_BT_POS,         DPP_SDT_H_LPM_V46ID_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_lpm->rsp_mode,            sdt_hig32, DPP_SDT_H_LPM_RSP_MODE_BT_POS,      DPP_SDT_H_LPM_RSP_MODE_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_lpm->lpm_table_depth,     sdt_low32, DPP_SDT_L_LPM_TABLE_DEPTH_BT_POS,   DPP_SDT_L_LPM_TABLE_DEPTH_BT_LEN);
            p_sdt_lpm->lpm_clutch_en = clutch_en;
            break;
        }

        case DPP_SDT_TBLT_eTCAM:
        {
            p_sdt_etcam = (DPP_SDTTBL_ETCAM_T *)p_sdt_info;
            p_sdt_etcam->table_type = tbl_type;
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_etcam->etcam_id,          sdt_hig32, DPP_SDT_H_ETCAM_ID_BT_POS,              DPP_SDT_H_ETCAM_ID_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_etcam->etcam_key_mode,    sdt_hig32, DPP_SDT_H_ETCAM_KEY_MODE_BT_POS,        DPP_SDT_H_ETCAM_KEY_MODE_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_etcam->etcam_table_id,    sdt_hig32, DPP_SDT_H_ETCAM_TABLE_ID_BT_POS,        DPP_SDT_H_ETCAM_TABLE_ID_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_etcam->no_as_rsp_mode,    sdt_hig32, DPP_SDT_H_ETCAM_NOAS_RSP_MODE_BT_POS,   DPP_SDT_H_ETCAM_NOAS_RSP_MODE_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_etcam->as_en,             sdt_hig32, DPP_SDT_H_ETCAM_AS_EN_BT_POS,           DPP_SDT_H_ETCAM_AS_EN_BT_LEN);

            ZXIC_COMM_UINT32_GET_BITS(tmp,                            sdt_hig32, DPP_SDT_H_ETCAM_AS_ERAM_BADDR_BT_POS,     DPP_SDT_H_ETCAM_AS_ERAM_BADDR_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_etcam->as_eram_baddr,     sdt_low32, DPP_SDT_L_ETCAM_AS_ERAM_BADDR_BT_POS,     DPP_SDT_L_ETCAM_AS_ERAM_BADDR_BT_LEN);
            ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(p_sdt_etcam->as_eram_baddr, (tmp << DPP_SDT_L_ETCAM_AS_ERAM_BADDR_BT_LEN));
            p_sdt_etcam->as_eram_baddr += (tmp << DPP_SDT_L_ETCAM_AS_ERAM_BADDR_BT_LEN);

            ZXIC_COMM_UINT32_GET_BITS(p_sdt_etcam->as_rsp_mode,       sdt_low32, DPP_SDT_L_ETCAM_AS_RSP_MODE_BT_POS,     DPP_SDT_L_ETCAM_AS_RSP_MODE_BT_LEN);
            ZXIC_COMM_UINT32_GET_BITS(p_sdt_etcam->etcam_table_depth, sdt_low32, DPP_SDT_L_ETCAM_TABLE_DEPTH_BT_POS,     DPP_SDT_L_ETCAM_TABLE_DEPTH_BT_LEN);
            p_sdt_etcam->etcam_clutch_en = clutch_en;
            break;
        }

        case DPP_SDT_TBLT_PORTTBL:
        {
            p_sdt_porttbl = (DPP_SDTTBL_PORTTBL_T *)p_sdt_info;
            p_sdt_porttbl->table_type = tbl_type;
            p_sdt_porttbl->porttbl_clutch_en = clutch_en;
            break;
        }

        default:
        {
            ZXIC_COMM_TRACE_ERROR("SDT table_type[ %d ] is invalid!\n", tbl_type);
            ZXIC_COMM_ASSERT(0);
            return DPP_ERR;
        }
    }

    return DPP_OK;
}

/***********************************************************/
/** 从软件缓存中获取table data信息
* @param   dev_id      设备号
* @param   sdt_no      业务表对应的sdt号,0-255
* @param   p_sdt_data  sdt表信息
*
* @return  
* @remark  无
* @see
* @author  lim      @date  2020/04/16
************************************************************/
DPP_STATUS dpp_sdt_tbl_data_get(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no, DPP_SDT_TBL_DATA_T *p_sdt_data)
{
    return dpp_sdt_mgr_sdt_item_srh(dev, sdt_no, &p_sdt_data->data_high32, &p_sdt_data->data_low32);
}

/***********************************************************/
/** 从软件缓存中获取sdt信息
* @param   device_id   设备号
* @param   sdt_no      业务表对应的sdt号
* @param   p_sdt_info  写入的SDT属性。由table_type确定此ZXIC_VOID型指针对应的数据结构, 包括: \n
*                      DPP_SDTTBL_ERAM_T、DPP_SDTTBL_DDR_T、DPP_SDTTBL_HASH_T、DPP_SDTTBL_LPM_T、
*                      DPP_SDTTBL_ETCAM_T。
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  lim      @date  2020/04/16
************************************************************/
DPP_STATUS dpp_soft_sdt_tbl_get(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no, ZXIC_VOID *p_sdt_info)
{
    DPP_STATUS  rc       = 0;

    DPP_SDT_TBL_DATA_T sdt_tbl = {0};

    ZXIC_COMM_CHECK_INDEX(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_sdt_info);

    /** 从软件缓存读取sdt信息 */
    rc = dpp_sdt_tbl_data_get(dev, sdt_no, &sdt_tbl);
    ZXIC_COMM_CHECK_RC(rc, "dpp_sdt_tbl_data_get");

    rc = dpp_sdt_tbl_data_parser(dev, sdt_tbl.data_high32, sdt_tbl.data_low32, p_sdt_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_sdt_tbl_data_parser");
        
    return rc;
}

/***********************************************************/
/** 写SDT属性表条目到硬件表，同时向8个cluster写入
* @param   dev_id   设备号
* @param   sdt_no      业务表对应的sdt号
* @param   table_type  SDT属性中的表类型，取值参考DPP_SDT_TABLE_TYPE_E的定义(仅添加操作时有效)
* @param   p_sdt_info  写入的SDT属性(仅添加操作时有效)。由table_type确定此ZXIC_VOID型指针对应的数据结构, 包括: \n
*                      DPP_SDTTBL_ERAM_T、DPP_SDTTBL_DDR_T、DPP_SDTTBL_HASH_T、DPP_SDTTBL_LPM_T、\n
*                      DPP_SDTTBL_ETCAM_T、DPP_SDTTBL_PORTTBL_T。
* @param   opr_type    操作类型: 0-添加条目，1-删除条目.
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_sdt_tbl_write(DPP_DEV_T *dev,
                             ZXIC_UINT32 sdt_no,
                             ZXIC_UINT32 table_type,
                             ZXIC_VOID *p_sdt_info,
                             ZXIC_UINT32 opr_type)
{
#ifdef DPP_FLOW_HW_INIT
    ZXIC_UINT32 i = 0;
#endif
    DPP_STATUS rtn = 0;
    DPP_SDT_TBL_DATA_T sdt_tbl = {0};
    DPP_SDTTBL_ERAM_T *p_sdt_eram = NULL;
    DPP_SDTTBL_DDR3_T *p_sdt_ddr3 = NULL;
    DPP_SDTTBL_HASH_T *p_sdt_hash = NULL;
    DPP_SDTTBL_LPM_T *p_sdt_lpm = NULL;
    DPP_SDTTBL_ETCAM_T *p_sdt_etcam = NULL;
    DPP_SDTTBL_PORTTBL_T *p_sdt_porttbl = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    ZXIC_COMM_MEMSET_S(&sdt_tbl, sizeof(DPP_SDT_TBL_DATA_T), 0, sizeof(DPP_SDT_TBL_DATA_T));

    /* 根据表类型解析数据 */
    if (opr_type)
    {
        rtn = dpp_sdt_mgr_sdt_item_del(dev, sdt_no);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_sdt_mgr_sdt_item_del");
    }
    else
    {
        /* add sdt item*/
        ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_sdt_info);
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), table_type, DPP_SDT_TBLT_eRAM, DPP_SDT_TBLT_PORTTBL);

        switch (table_type)
        {
            case DPP_SDT_TBLT_eRAM:
            {
                p_sdt_eram = (DPP_SDTTBL_ERAM_T *)p_sdt_info;
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32, p_sdt_eram->eram_mode,           DPP_SDT_H_ERAM_MODE_BT_POS,             DPP_SDT_H_ERAM_MODE_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32, p_sdt_eram->eram_base_addr,      DPP_SDT_H_ERAM_BASE_ADDR_BT_POS,        DPP_SDT_H_ERAM_BASE_ADDR_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_low32,  p_sdt_eram->eram_table_depth,    DPP_SDT_L_ERAM_TABLE_DEPTH_BT_POS,      DPP_SDT_L_ERAM_TABLE_DEPTH_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_low32,  p_sdt_eram->eram_clutch_en,      DPP_SDT_L_CLUTCH_EN_BT_POS,             DPP_SDT_L_CLUTCH_EN_BT_LEN);
                break;
            }

            case DPP_SDT_TBLT_DDR3:
            {
                p_sdt_ddr3 = (DPP_SDTTBL_DDR3_T *)p_sdt_info;

                /** 添加操作必须保证sdt号和ddr存储的sdt号一致 */
                ZXIC_COMM_ASSERT(sdt_no == p_sdt_ddr3->ddr3_sdt_num );

                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32, p_sdt_ddr3->ddr3_base_addr,      DPP_SDT_H_DDR3_BASE_ADDR_BT_POS,        DPP_SDT_H_DDR3_BASE_ADDR_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32, p_sdt_ddr3->ddr3_share_type,     DPP_SDT_H_DDR3_SHARE_TYPE_BT_POS,       DPP_SDT_H_DDR3_SHARE_TYPE_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32, p_sdt_ddr3->ddr3_rw_len,         DPP_SDT_H_DDR3_RW_LEN_BT_POS,           DPP_SDT_H_DDR3_RW_LEN_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32,
                                      ((p_sdt_ddr3->ddr3_sdt_num) >> DPP_SDT_L_DDR3_SDT_NUM_BT_LEN),
                                      DPP_SDT_H_DDR3_SDT_NUM_BT_POS,
                                      DPP_SDT_H_DDR3_SDT_NUM_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_low32,
                                      DPP_SDT_GET_LOW_DATA((p_sdt_ddr3->ddr3_sdt_num),      DPP_SDT_L_DDR3_SDT_NUM_BT_LEN),
                                      DPP_SDT_L_DDR3_SDT_NUM_BT_POS,
                                      DPP_SDT_L_DDR3_SDT_NUM_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_low32,  p_sdt_ddr3->ddr3_ecc_en,         DPP_SDT_L_DDR3_ECC_EN_BT_POS,           DPP_SDT_L_DDR3_ECC_EN_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_low32,  p_sdt_ddr3->ddr3_clutch_en,      DPP_SDT_L_CLUTCH_EN_BT_POS,             DPP_SDT_L_CLUTCH_EN_BT_LEN);
                break;
            }

            case DPP_SDT_TBLT_HASH:
            {
                p_sdt_hash = (DPP_SDTTBL_HASH_T *)p_sdt_info;
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32, p_sdt_hash->hash_id,             DPP_SDT_H_HASH_ID_BT_POS,               DPP_SDT_H_HASH_ID_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32, p_sdt_hash->hash_table_width,    DPP_SDT_H_HASH_TABLE_WIDTH_BT_POS,      DPP_SDT_H_HASH_TABLE_WIDTH_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32, p_sdt_hash->key_size,            DPP_SDT_H_HASH_KEY_SIZE_BT_POS,         DPP_SDT_H_HASH_KEY_SIZE_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32, p_sdt_hash->hash_table_id,       DPP_SDT_H_HASH_TABLE_ID_BT_POS,         DPP_SDT_H_HASH_TABLE_ID_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32, p_sdt_hash->learn_en,            DPP_SDT_H_LEARN_EN_BT_POS,              DPP_SDT_H_LEARN_EN_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32, p_sdt_hash->keep_alive,          DPP_SDT_H_KEEP_ALIVE_BT_POS,            DPP_SDT_H_KEEP_ALIVE_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32,
                                      ((p_sdt_hash->keep_alive_baddr) >> DPP_SDT_L_KEEP_ALIVE_BADDR_BT_LEN),
                                      DPP_SDT_H_KEEP_ALIVE_BADDR_BT_POS,
                                      DPP_SDT_H_KEEP_ALIVE_BADDR_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_low32,
                                      DPP_SDT_GET_LOW_DATA((p_sdt_hash->keep_alive_baddr), DPP_SDT_L_KEEP_ALIVE_BADDR_BT_LEN),
                                      DPP_SDT_L_KEEP_ALIVE_BADDR_BT_POS,
                                      DPP_SDT_L_KEEP_ALIVE_BADDR_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_low32,   p_sdt_hash->rsp_mode,           DPP_SDT_L_RSP_MODE_BT_POS,              DPP_SDT_L_RSP_MODE_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_low32,  p_sdt_hash->hash_clutch_en,      DPP_SDT_L_CLUTCH_EN_BT_POS,             DPP_SDT_L_CLUTCH_EN_BT_LEN);
                break;
            }

            case DPP_SDT_TBLT_LPM:
            {
                p_sdt_lpm = (DPP_SDTTBL_LPM_T *)p_sdt_info;
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32, p_sdt_lpm->lpm_v46_id,           DPP_SDT_H_LPM_V46ID_BT_POS,             DPP_SDT_H_LPM_V46ID_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32, p_sdt_lpm->rsp_mode,             DPP_SDT_H_LPM_RSP_MODE_BT_POS,          DPP_SDT_H_LPM_RSP_MODE_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_low32,  p_sdt_lpm->lpm_table_depth,      DPP_SDT_L_LPM_TABLE_DEPTH_BT_POS,       DPP_SDT_L_LPM_TABLE_DEPTH_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_low32,  p_sdt_lpm->lpm_clutch_en,        DPP_SDT_L_CLUTCH_EN_BT_POS,             DPP_SDT_L_CLUTCH_EN_BT_LEN);
                break;
            }

            case DPP_SDT_TBLT_eTCAM:
            {
                p_sdt_etcam = (DPP_SDTTBL_ETCAM_T *)p_sdt_info;
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32, p_sdt_etcam->etcam_id,           DPP_SDT_H_ETCAM_ID_BT_POS,              DPP_SDT_H_ETCAM_ID_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32, p_sdt_etcam->etcam_key_mode,     DPP_SDT_H_ETCAM_KEY_MODE_BT_POS,        DPP_SDT_H_ETCAM_KEY_MODE_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32, p_sdt_etcam->etcam_table_id,     DPP_SDT_H_ETCAM_TABLE_ID_BT_POS,        DPP_SDT_H_ETCAM_TABLE_ID_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32, p_sdt_etcam->no_as_rsp_mode,     DPP_SDT_H_ETCAM_NOAS_RSP_MODE_BT_POS,   DPP_SDT_H_ETCAM_NOAS_RSP_MODE_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32, p_sdt_etcam->as_en,              DPP_SDT_H_ETCAM_AS_EN_BT_POS,           DPP_SDT_H_ETCAM_AS_EN_BT_LEN);

                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32,
                                      ((p_sdt_etcam->as_eram_baddr) >> DPP_SDT_L_ETCAM_AS_ERAM_BADDR_BT_LEN),
                                      DPP_SDT_H_ETCAM_AS_ERAM_BADDR_BT_POS,
                                      DPP_SDT_H_ETCAM_AS_ERAM_BADDR_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_low32,
                                      DPP_SDT_GET_LOW_DATA((p_sdt_etcam->as_eram_baddr), DPP_SDT_L_ETCAM_AS_ERAM_BADDR_BT_LEN),
                                      DPP_SDT_L_ETCAM_AS_ERAM_BADDR_BT_POS,
                                      DPP_SDT_L_ETCAM_AS_ERAM_BADDR_BT_LEN);

                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_low32, p_sdt_etcam->as_rsp_mode,         DPP_SDT_L_ETCAM_AS_RSP_MODE_BT_POS,     DPP_SDT_L_ETCAM_AS_RSP_MODE_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_low32,  p_sdt_etcam->etcam_table_depth,  DPP_SDT_L_ETCAM_TABLE_DEPTH_BT_POS,     DPP_SDT_L_ETCAM_TABLE_DEPTH_BT_LEN);
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_low32,  p_sdt_etcam->etcam_clutch_en,    DPP_SDT_L_CLUTCH_EN_BT_POS,             DPP_SDT_L_CLUTCH_EN_BT_LEN);
                break;
            }

            case DPP_SDT_TBLT_PORTTBL:
            {
                p_sdt_porttbl = (DPP_SDTTBL_PORTTBL_T *)p_sdt_info;
                ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_low32,  p_sdt_porttbl->porttbl_clutch_en, DPP_SDT_L_CLUTCH_EN_BT_POS,             DPP_SDT_L_CLUTCH_EN_BT_LEN);
                break;
            }

            default:
            {
                ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "SDT table_type[ %d ] is invalid!\n", table_type);
                return DPP_ERR;
            }
        }

        ZXIC_COMM_UINT32_WRITE_BITS(sdt_tbl.data_high32, table_type, DPP_SDT_H_TBL_TYPE_BT_POS, DPP_SDT_H_TBL_TYPE_BT_LEN);

        /* 缓存到软件 */
        rtn = dpp_sdt_mgr_sdt_item_add(dev, sdt_no, sdt_tbl.data_high32, sdt_tbl.data_low32);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_sdt_mgr_sdt_item_add");
    }
#ifdef DPP_FLOW_HW_INIT
    for (i = 0; i < DPP_PPU_CLUSTER_NUM; i++)
    {
        /*cluster 未启用,不需要配置该cluster相关的寄存器*/
        if (!dpp_ppu_cls_use_get(DEV_ID(dev), i))
        {
            continue;
        }

        rtn = dpp_ppu_sdt_tbl_write(dev, i, sdt_no, &sdt_tbl);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rtn, "dpp_ppu_sdt_tbl_write");
    }
#endif
    return DPP_OK;
}

#endif

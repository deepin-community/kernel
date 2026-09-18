/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_apt_se_eram.c
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : 陈勤00181032
* 完成日期 : 2023/02/22
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/

#include "dpp_apt_se.h"
#include "dpp_dev.h"
#include "dpp_sdt.h"
#include "dpp_dtb_table.h"
#include "dpp_dtb_table_api.h"
#include "dpp_apt_se.h"

/***********************************************************/
/** eram表资源初始化
* @param   dev_id  设备号 
* @param   tbl_num  需初始化的eram表个数
* @param   pEramTbl  eram资源信息，包括SDT配置信息，直接表读取位宽和结构体码流转换回调函数
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_eram_res_init(DPP_DEV_T *dev,ZXIC_UINT32 tbl_num,DPP_APT_ERAM_TABLE_T *pEramTbl)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT8 index = 0;
    DPP_APT_ERAM_TABLE_T *pTempEramTbl = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_UPPER(tbl_num, DPP_DEV_SDT_ID_MAX);
    ZXIC_COMM_CHECK_POINT(pEramTbl);

    for(index = 0;index < tbl_num;index++)
    {
        pTempEramTbl = pEramTbl + index;
        rc = dpp_sdt_tbl_write(dev,
                        pTempEramTbl->sdtNo,
                        pTempEramTbl->eRamSdt.table_type,
                        &(pTempEramTbl->eRamSdt),
                        SDT_OPER_ADD);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_sdt_tbl_write");

        rc = dpp_apt_set_callback(dev,
                        pTempEramTbl->sdtNo,
                        pTempEramTbl->eRamSdt.table_type,
                        (ZXIC_VOID *)pTempEramTbl);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_apt_set_callback");
    }

    return DPP_OK;
}

/***********************************************************/
/** dtb eram表项插入/更新
* @param   dev_id  设备号 
* @param   sdt_no  SDT号 0~255
* @param   index   条目index，索引范围随wrt_mode模式不同
* @param   pData   插入表项内容，由业务确定
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_dtb_eram_insert(DPP_DEV_T *dev,ZXIC_UINT32 queue_id,ZXIC_UINT32 sdt_no,ZXIC_UINT32 index,void *pData)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 element_id = 0;
    ZXIC_UINT32 dump_data[4] = {0};
    
    SE_APT_CALLBACK_T *pAptCallback = NULL;
    DPP_DTB_ERAM_ENTRY_INFO_T dtb_eram_entry = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    ZXIC_COMM_MEMSET(dump_data, 0x00, sizeof(dump_data));

    pAptCallback = dpp_apt_get_func(dev, sdt_no);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pAptCallback);

    rc = pAptCallback->se_func_info.eramFunc.eram_set_func(pData, dump_data);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "eram_set_func");

    //dtb配表
    dtb_eram_entry.index = index;
    dtb_eram_entry.p_data = dump_data;
    rc = dpp_dtb_eram_dma_write(dev, 
                                      queue_id, 
                                      sdt_no, 
                                      1, 
                                      &dtb_eram_entry,
                                      &element_id);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_eram_dma_write");

    return rc;
}

/***********************************************************/
/** eram表项数据获取,从软件缓存中获取
* @param   dev_id  设备号 
* @param   sdt_no  SDT号 0~255
* @param   index   条目index，索引范围随wrt_mode模式不同
* @param   pData   出参，返回业务表项内容
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_dtb_eram_get(DPP_DEV_T *dev, ZXIC_UINT32 queue_id, ZXIC_UINT32 sdt_no, ZXIC_UINT32 index, void *pData)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 dump_data[4] = {0};
    SE_APT_CALLBACK_T *pAptCallback = NULL;
    DPP_DTB_ERAM_ENTRY_INFO_T dump_eram_entry = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    ZXIC_COMM_MEMSET(dump_data, 0x00, sizeof(dump_data));

    pAptCallback = dpp_apt_get_func(dev, sdt_no);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pAptCallback);

    dump_eram_entry.index = index;
    dump_eram_entry.p_data = dump_data;
    rc = dpp_dtb_eram_data_get(dev, queue_id, sdt_no, &dump_eram_entry);
    ZXIC_COMM_CHECK_RC_NO_ASSERT( rc, "dpp_dtb_eram_data_get");

    rc = pAptCallback->se_func_info.eramFunc.eram_get_func(pData, dump_eram_entry.p_data);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "eram_get_func");

    return rc;
}

/***********************************************************/
/** eram表项删除,软件维护删除
* @param   dev_id  设备号 
* @param   sdt_no  SDT号 0~255
* @param   index   条目index，索引范围随wrt_mode模式不同
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_dtb_eram_clear(DPP_DEV_T *dev,ZXIC_UINT32 queue_id,ZXIC_UINT32 sdt_no,ZXIC_UINT32 index)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 element_id = 0;
    ZXIC_UINT32 dump_data[4] = {0};

    SE_APT_CALLBACK_T *pAptCallback = NULL;
    DPP_DTB_ERAM_ENTRY_INFO_T dtb_eram_entry = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    ZXIC_COMM_MEMSET(dump_data, 0x00, sizeof(dump_data));

    pAptCallback = dpp_apt_get_func(dev, sdt_no);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pAptCallback);

    //dtb配表
    dtb_eram_entry.index = index;
    dtb_eram_entry.p_data = dump_data;
    rc = dpp_dtb_eram_dma_write(dev, 
                                      queue_id, 
                                      sdt_no, 
                                      1, 
                                      &dtb_eram_entry,
                                      &element_id);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_eram_dma_write");

    return rc;
}

/***********************************************************/
/** eram表项flush
* @param   dev_id  设备号 
* @param   sdt_no  SDT号 0~255
* @param   index   条目index，索引范围随wrt_mode模式不同
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_dtb_eram_flush(DPP_DEV_T *dev, ZXIC_UINT32 queue_id, ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    rc = dpp_dtb_eram_table_flush(dev, queue_id, sdt_no);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_eram_table_flush");
    ZXIC_COMM_TRACE_ERROR("dpp_apt_dtb_eram_flush sdt_no %d done.\n", sdt_no);

    return rc;
}

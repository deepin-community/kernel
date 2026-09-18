/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_apt_se_acl.c
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
#include "dpp_acl.h"
#include "dpp_dtb_table.h"

static DPP_ACL_CFG_EX_T *g_apt_acl_cfg[DPP_PCIE_SLOT_MAX] = {NULL};

DPP_ACL_CFG_EX_T *dpp_apt_get_acl_cfg(DPP_DEV_T *dev)
{
    ZXIC_UINT32 slot = 0;
    if(NULL==dev)
    {
        return NULL;
    }
    slot = DEV_PCIE_SLOT(dev);
    if(slot<DPP_PCIE_SLOT_MAX)
    {
        return g_apt_acl_cfg[slot];
    }
    return NULL;
}

/***********************************************************/
/** acl资源初始化
* @param   dev_id  设备号 
* @param   tbl_num     etcam对应的sdt表个数
* @param   pAclTblRes  acl表资源信息，包括SDT配置信息，acl资源(条目数，存放方式和占用的block)和结构体码流转换回调函数
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_acl_res_init(DPP_DEV_T *dev,ZXIC_UINT32 tbl_num,DPP_APT_ACL_TABLE_T *pAclTblRes)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT8 index = 0;
    ZXIC_UINT32 slot = 0;
    DPP_APT_ACL_TABLE_T *pTempAclTbl = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_UPPER(tbl_num, DPP_ETCAM_TBLID_NUM);
    ZXIC_COMM_CHECK_POINT(pAclTblRes);

    slot = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_INDEX(slot, 0, DPP_PCIE_SLOT_MAX - 1);

    if(NULL==g_apt_acl_cfg[slot])
    {
        g_apt_acl_cfg[slot] = (DPP_ACL_CFG_EX_T *)ZXIC_COMM_MALLOC(sizeof(DPP_ACL_CFG_EX_T));
        ZXIC_COMM_CHECK_POINT(g_apt_acl_cfg[slot]);
    }

    rc = dpp_acl_cfg_init_ex(dev, g_apt_acl_cfg[slot],
                        (ZXIC_VOID *)ZXIC_COMM_VAL_TO_PTR(DEV_ID(dev)),
                        DPP_ACL_FLAG_ETCAM0_EN,
                        NULL);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_acl_cfg_init_ex");

    for(index = 0;index < tbl_num;index++)
    {
        pTempAclTbl = pAclTblRes + index;
        rc = dpp_sdt_tbl_write(dev,
                        pTempAclTbl->sdtNo,
                        pTempAclTbl->aclSdt.table_type,
                        &(pTempAclTbl->aclSdt),
                        SDT_OPER_ADD);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_sdt_tbl_write");

         rc = dpp_acl_tbl_init_ex(g_apt_acl_cfg[slot],
                                    pTempAclTbl->aclSdt.etcam_table_id,
                                    pTempAclTbl->aclSdt.as_en,
                                    pTempAclTbl->aclRes.entry_num,
                                    pTempAclTbl->aclRes.pri_mode,
                                    pTempAclTbl->aclSdt.etcam_key_mode,
                                    pTempAclTbl->aclSdt.as_rsp_mode,
                                    pTempAclTbl->aclSdt.as_eram_baddr,
                                    pTempAclTbl->aclRes.block_num,
                                    pTempAclTbl->aclRes.block_index);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_acl_tbl_init_ex");

        rc = dpp_apt_set_callback(dev,
                        pTempAclTbl->sdtNo,
                        pTempAclTbl->aclSdt.table_type,
                        (ZXIC_VOID *)pTempAclTbl);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_apt_set_callback");

    }

    return DPP_OK;
}

/***********************************************************/
/** acl软件资源释放
* @param   dev  设备号 
* @return  
* @remark  无
* @see     
* @author  cq      @date  2025/06/30
************************************************************/
DPP_STATUS dpp_apt_acl_soft_res_uninit(DPP_DEV_T *dev)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 table_id = 0;
    ZXIC_UINT32 as_enable = 0;
    ZXIC_UINT32 pri_mode = 0;
    ZXIC_UINT32 slot = 0;
    DPP_ACL_CFG_EX_T *p_acl_cfg = NULL;
    DPP_ACL_TBL_CFG_T *p_tbl_cfg = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    slot = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_INDEX(slot, 0, DPP_PCIE_SLOT_MAX - 1);

    rc = dpp_acl_cfg_get(dev,&p_acl_cfg);//获取ACL表资源配置
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_cfg_get");

    if(!p_acl_cfg->acl_etcamids.is_valid)
    {
        ZXIC_COMM_TRACE_ERROR("etcam is not init!\n");
        return DPP_ACL_RC_ETCAMID_NOT_INIT;
    }

    for(table_id=DPP_ACL_TBL_ID_MIN;table_id<=DPP_ACL_TBL_ID_MAX;table_id++)
    {
        p_tbl_cfg = p_acl_cfg->acl_tbls + table_id;
        if (!p_tbl_cfg->is_used)
        {
            ZXIC_COMM_TRACE_DEBUG("table_id[ %d ] is not used!\n", table_id);
            continue;
        }

        rc = (DPP_STATUS)zxic_comm_rb_destroy(&(p_tbl_cfg->acl_rb));
        ZXIC_COMM_CHECK_RC(rc, "zxic_comm_rb_destroy");

        rc = zxic_comm_indexfill_destroy(&(p_tbl_cfg->index_mng));
        ZXIC_COMM_CHECK_RC(rc, "zxic_comm_indexfill_destroy");
        
        as_enable = p_tbl_cfg->as_enable;
        if(as_enable)
        {
            if(p_tbl_cfg->as_rslt_buff)
            {
                ZXIC_COMM_FREE(p_tbl_cfg->as_rslt_buff);
                p_tbl_cfg->as_rslt_buff = NULL;
            }
        }

        pri_mode = p_tbl_cfg->pri_mode;
        if ((pri_mode == DPP_ACL_PRI_EXPLICIT) || (pri_mode == DPP_ACL_PRI_IMPLICIT))
        {
            if(p_tbl_cfg->acl_key_buff)
            {
                ZXIC_COMM_FREE(p_tbl_cfg->acl_key_buff);
                p_tbl_cfg->acl_key_buff = NULL;
            }
        }

        if(p_tbl_cfg->block_array)
        {
            ZXIC_COMM_FREE(p_tbl_cfg->block_array);
            p_tbl_cfg->block_array = NULL;
        }
    }

    if(NULL != g_apt_acl_cfg[slot])
    {
       ZXIC_COMM_FREE(g_apt_acl_cfg[slot]);
       g_apt_acl_cfg[slot] = NULL;
       dpp_acl_cfg_set(dev,NULL);
    }

    return DPP_OK;
}

/***********************************************************/
/** acl表项插入/更新
* @param   dev_id  设备号 
* @param   sdt_no  sdt号 0~255
* @param   pData   业务插入表项内容，具体结构体由业务确定(结构体的第一个字段必须为index)，SDK不感知
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_dtb_acl_entry_insert(DPP_DEV_T *dev,ZXIC_UINT32 queue_id, ZXIC_UINT32 sdt_no, void *pData)
{
    ZXIC_UINT8 data[DPP_ETCAM_WIDTH_MAX/8] = {0};   /*640bit*/
    ZXIC_UINT8 mask[DPP_ETCAM_WIDTH_MAX/8] = {0};   /*640bit*/
    ZXIC_UINT8 rst[16] = {0};   /*128bit*/
    ZXIC_UINT32 element_id = 0;
    ZXIC_UINT32 rc = DPP_OK;

    DPP_ACL_ENTRY_EX_T aclEntry = {0};
    DPP_DTB_ACL_ENTRY_INFO_T tDtbAclEntry = {0};
    //DPP_SDTTBL_ETCAM_T sdt_acl_info = {0};  /*SDT内容*/
    SE_APT_CALLBACK_T *pAptCallback = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_POINT(pData);

    ZXIC_COMM_MEMSET(data, 0x0, sizeof(data));
    ZXIC_COMM_MEMSET(mask, 0x0, sizeof(mask));
    ZXIC_COMM_MEMSET(rst, 0x0, sizeof(rst));
    ZXIC_COMM_MEMSET(&aclEntry, 0x0, sizeof(DPP_ACL_ENTRY_EX_T));
    ZXIC_COMM_MEMSET(&tDtbAclEntry, 0x0, sizeof(DPP_DTB_ACL_ENTRY_INFO_T));

    aclEntry.key_data = data;
    aclEntry.key_mask = mask;
    aclEntry.p_as_rslt = rst;
    
    pAptCallback = dpp_apt_get_func(dev, sdt_no);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pAptCallback);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pAptCallback->se_func_info.aclFunc.acl_set_func);

    rc = pAptCallback->se_func_info.aclFunc.acl_set_func((void *)pData, &aclEntry);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "acl_entry_func");

    tDtbAclEntry.handle = aclEntry.pri;
    tDtbAclEntry.key_data = aclEntry.key_data;
    tDtbAclEntry.key_mask = aclEntry.key_mask;
    tDtbAclEntry.p_as_rslt = aclEntry.p_as_rslt;

    rc = dpp_dtb_acl_dma_insert(dev, queue_id, sdt_no, 1, &tDtbAclEntry, &element_id);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_acl_dma_insert");

    return rc;
}

/***********************************************************/
/** acl表项删除
* @param   dev_id  设备号 
* @param   sdt_no  sdt号 0~255
* @param   pData   删除业务表项内容，仅需填入index信息
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/09/25
************************************************************/
DPP_STATUS dpp_apt_dtb_acl_entry_del(DPP_DEV_T *dev,ZXIC_UINT32 queue_id, ZXIC_UINT32 sdt_no, void *pData)
{
    ZXIC_UINT8 data[DPP_ETCAM_WIDTH_MAX/8] = {0};   /*640bit*/
    ZXIC_UINT8 mask[DPP_ETCAM_WIDTH_MAX/8] = {0};   /*640bit*/
    ZXIC_UINT8 rst[16] = {0};   /*128bit*/
    ZXIC_UINT32 element_id = 0;
    ZXIC_UINT32 rc = DPP_OK;

    DPP_ACL_ENTRY_EX_T aclEntry = {0};
    DPP_DTB_ACL_ENTRY_INFO_T tDtbAclEntry = {0};
    SE_APT_CALLBACK_T *pAptCallback = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_POINT(pData);

    ZXIC_COMM_MEMSET(data, 0xff, sizeof(data));
    ZXIC_COMM_MEMSET(mask, 0x0, sizeof(mask));
    ZXIC_COMM_MEMSET(rst, 0x0, sizeof(rst));
    ZXIC_COMM_MEMSET(&aclEntry, 0x0, sizeof(DPP_ACL_ENTRY_EX_T));
    ZXIC_COMM_MEMSET(&tDtbAclEntry, 0x0, sizeof(DPP_DTB_ACL_ENTRY_INFO_T));

    pAptCallback = dpp_apt_get_func(dev, sdt_no);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pAptCallback);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pAptCallback->se_func_info.aclFunc.acl_set_func);

    rc = pAptCallback->se_func_info.aclFunc.acl_set_func((void *)pData, &aclEntry);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "acl_entry_func");

    tDtbAclEntry.handle = aclEntry.pri;
    tDtbAclEntry.key_data = data;
    tDtbAclEntry.key_mask = mask;
    tDtbAclEntry.p_as_rslt = rst;

    rc = dpp_dtb_acl_dma_insert(dev, queue_id, sdt_no, 1, &tDtbAclEntry, &element_id);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_acl_dma_insert");

    return rc;
}

/***********************************************************/
/** acl表项查找(handle+data+mask有效)
* @param   dev      设备
* @param   queue_id 队列号
* @param   sdt_no   sdt号 0~255
* @param   pData    查找表项
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/09/21
************************************************************/
DPP_STATUS dpp_apt_dtb_acl_entry_search(DPP_DEV_T *dev,ZXIC_UINT32 queue_id, ZXIC_UINT32 sdt_no, void *pData)
{
    ZXIC_UINT8 data[DPP_ETCAM_WIDTH_MAX/8] = {0};   /*640bit*/
    ZXIC_UINT8 mask[DPP_ETCAM_WIDTH_MAX/8] = {0};   /*640bit*/
    ZXIC_UINT8 rst[16] = {0};   /*128bit*/
    ZXIC_UINT32 rc = DPP_OK;

    DPP_ACL_ENTRY_EX_T aclEntry = {0};
    DPP_DTB_ACL_ENTRY_INFO_T tDtbAclEntry = {0};
    //DPP_SDTTBL_ETCAM_T sdt_acl_info = {0};  /*SDT内容*/
    SE_APT_CALLBACK_T *pAptCallback = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_POINT(pData);

    ZXIC_COMM_MEMSET(data, 0x0, sizeof(data));
    ZXIC_COMM_MEMSET(mask, 0x0, sizeof(mask));
    ZXIC_COMM_MEMSET(rst, 0x0, sizeof(rst));
    ZXIC_COMM_MEMSET(&aclEntry, 0x0, sizeof(DPP_ACL_ENTRY_EX_T));
    ZXIC_COMM_MEMSET(&tDtbAclEntry, 0x0, sizeof(DPP_DTB_ACL_ENTRY_INFO_T));

    aclEntry.key_data = data;
    aclEntry.key_mask = mask;
    aclEntry.p_as_rslt = rst;

    pAptCallback = dpp_apt_get_func(dev, sdt_no);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pAptCallback);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pAptCallback->se_func_info.aclFunc.acl_set_func);

    rc = pAptCallback->se_func_info.aclFunc.acl_set_func((void *)pData, &aclEntry);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "acl_entry_func");

    tDtbAclEntry.handle = aclEntry.pri;
    tDtbAclEntry.key_data = aclEntry.key_data;
    tDtbAclEntry.key_mask = aclEntry.key_mask;
    tDtbAclEntry.p_as_rslt = aclEntry.p_as_rslt;

    rc = dpp_dtb_acl_data_get(dev, queue_id, sdt_no, &tDtbAclEntry);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_acl_data_get");

    aclEntry.pri = tDtbAclEntry.handle;
    aclEntry.key_data = tDtbAclEntry.key_data;
    aclEntry.key_mask = tDtbAclEntry.key_mask;
    aclEntry.p_as_rslt = tDtbAclEntry.p_as_rslt;

    rc = pAptCallback->se_func_info.aclFunc.acl_get_func((void *)pData, &aclEntry);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "acl_entry_func");

    return rc;
}

/***********************************************************/
/** 根据handle获取到acl表项信息
* @param   dev      设备
* @param   queue_id 队列号
* @param   sdt_no   sdt号 0~255
* @param   pData    查找表项
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/09/21
************************************************************/
DPP_STATUS dpp_apt_dtb_acl_entry_get(DPP_DEV_T *dev,ZXIC_UINT32 queue_id, ZXIC_UINT32 sdt_no, void *pData)
{
    ZXIC_UINT8 data[DPP_ETCAM_WIDTH_MAX/8] = {0};   /*640bit*/
    ZXIC_UINT8 mask[DPP_ETCAM_WIDTH_MAX/8] = {0};   /*640bit*/
    ZXIC_UINT8 rst[16] = {0};   /*128bit*/
    ZXIC_UINT32 rc = DPP_OK;

    DPP_ACL_ENTRY_EX_T aclEntry = {0};
    DPP_DTB_ACL_ENTRY_INFO_T tDtbAclEntry = {0};
    SE_APT_CALLBACK_T *pAptCallback = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_POINT(pData);

    ZXIC_COMM_MEMSET(data, 0x0, sizeof(data));
    ZXIC_COMM_MEMSET(mask, 0x0, sizeof(mask));
    ZXIC_COMM_MEMSET(rst, 0x0, sizeof(rst));
    ZXIC_COMM_MEMSET(&aclEntry, 0x0, sizeof(DPP_ACL_ENTRY_EX_T));
    ZXIC_COMM_MEMSET(&tDtbAclEntry, 0x0, sizeof(DPP_DTB_ACL_ENTRY_INFO_T));

    pAptCallback = dpp_apt_get_func(dev, sdt_no);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pAptCallback);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pAptCallback->se_func_info.aclFunc.acl_set_func);

    rc = pAptCallback->se_func_info.aclFunc.acl_set_func((void *)pData, &aclEntry);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "acl_entry_func");

    tDtbAclEntry.handle = aclEntry.pri;
    tDtbAclEntry.key_data = data;
    tDtbAclEntry.key_mask = mask;
    tDtbAclEntry.p_as_rslt = rst;

    rc = dpp_dtb_etcam_data_get(dev, queue_id, sdt_no, &tDtbAclEntry);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_etcam_data_get");

    aclEntry.pri = tDtbAclEntry.handle;
    aclEntry.key_data = tDtbAclEntry.key_data;
    aclEntry.key_mask = tDtbAclEntry.key_mask;
    aclEntry.p_as_rslt = tDtbAclEntry.p_as_rslt;
    rc = pAptCallback->se_func_info.aclFunc.acl_get_func((void *)pData, &aclEntry);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "acl_entry_func");

    return rc;
}

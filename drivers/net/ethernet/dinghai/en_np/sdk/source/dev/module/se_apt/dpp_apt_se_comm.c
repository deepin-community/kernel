/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_apt_se_common.c
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
#include "dpp_dtb_table_api.h"
#include "dpp_dtb_table.h"
#include "dpp_kernel_init.h"
#include "dpp_drv_sdt.h"
#include "dpp_tbl_comm.h"
#include "dpp_dtb_cfg.h"
#include "dpp_dtb.h"

#define DTB_QUEUE_ACK_SIZE (16)
#define DTB_QUEUE_ELEMENT_NUM (32)
#define DTB_QUEUE_ELEMENT_DATA_SIZE (16*1024 + DTB_QUEUE_ACK_SIZE)//16k+16
#define DTB_QUEUE_DATA_SIZE (DTB_QUEUE_ELEMENT_DATA_SIZE *  DTB_QUEUE_ELEMENT_NUM)
#define DTB_QUEUE_ELEMENT_DUMP_SIZE (16*1024 + DTB_QUEUE_ACK_SIZE)//16K+16
#define DTB_QUEUE_DUMP_SIZE (DTB_QUEUE_ELEMENT_DUMP_SIZE * DTB_QUEUE_ELEMENT_NUM)
#define DTB_QUEUE_DMA_SIZE (DTB_QUEUE_DATA_SIZE + DTB_QUEUE_DUMP_SIZE)


SE_APT_CALLBACK_T  g_apt_se_callback[DPP_PCIE_SLOT_MAX][DPP_DEV_SDT_ID_MAX] = {{{0}}};

/***********************************************************/
/** 根据设备id和sdt号获取指针
* @param   p_new_key      新键值
* @param   p_old_key      旧键值
* @param   key_len        键值长度
* @return  比较结果
* @remark  无
* @see
* @author  chenqin00181032      @date  2023/02/24
************************************************************/
ZXIC_SINT32 dpp_apt_table_key_cmp(void *p_new_key, void *p_old_key, ZXIC_UINT32 key_len)
{
    ZXIC_COMM_CHECK_POINT(p_new_key);
    ZXIC_COMM_CHECK_POINT(p_old_key);
    /* 仅比较index */
    ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW_NO_ASSERT(key_len, (ZXIC_UINT32)ZXIC_SIZEOF(ZXIC_UINT32));
    return ZXIC_COMM_MEMCMP((ZXIC_UINT32 *)p_new_key, (ZXIC_UINT32 *)p_old_key,ZXIC_SIZEOF(ZXIC_UINT32));
}

/***********************************************************/
/** 根据设备id和sdt号获取指针
* @param   dev_id      设备号
* @param   sdt_no      业务表对应的sdt号
* @return  
* @remark  无
* @see
* @author  chenqin00181032      @date  2023/02/24
************************************************************/
SE_APT_CALLBACK_T *dpp_apt_get_func(DPP_DEV_T *dev,ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT32 slot = 0;

    ZXIC_COMM_CHECK_POINT_RETURN_NULL(dev);
    ZXIC_COMM_CHECK_INDEX_RETURN_NULL_NO_ASSERT(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    slot = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_INDEX_RETURN_NULL_NO_ASSERT(slot, 0, DPP_PCIE_SLOT_MAX - 1);

    return &g_apt_se_callback[slot][sdt_no];
}

/***********************************************************/
/** 根据设备id和sdt号获取级联sdt
* @param   dev_id      设备号
* @param   sdt_no      业务表对应的sdt号
* @return  
* @remark  无
* @see
* @author  cq      @date  2024/09/19
************************************************************/
ZXIC_UINT32 dpp_apt_get_sdt_partner(DPP_DEV_T *dev,ZXIC_UINT32 sdt_no)
{
    SE_APT_CALLBACK_T *pAptCallback = NULL;

    pAptCallback = dpp_apt_get_func(dev,sdt_no);
    if(ZXIC_NULL==pAptCallback)
    {
        return ZXIC_UINT32_MAX;
    }

    if(DPP_SDT_TBLT_eTCAM==pAptCallback->table_type) 
    {
        return  pAptCallback->se_func_info.aclFunc.sdt_partner;
    }

    return ZXIC_UINT32_MAX;
}

/***********************************************************/
/** 保存回调参数信息
* @param   dev_id      设备号
* @param   sdt_no      业务表对应的sdt号
* @param   table_type  SDT属性中的表类型，取值参考DPP_SDT_TABLE_TYPE_E的定义(仅添加操作时有效)
* @param   pData       需保存的回调信息，由table_type确定此ZXIC_VOID型指针对应的数据结果
* @return  
* @remark  无
* @see
* @author  chenqin00181032      @date  2023/02/24
************************************************************/
DPP_STATUS dpp_apt_set_callback(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no, ZXIC_UINT32 table_type,ZXIC_VOID *pData)
{
    SE_APT_CALLBACK_T *aptFunc = NULL;

    aptFunc = dpp_apt_get_func(dev,sdt_no);
    ZXIC_COMM_CHECK_POINT(aptFunc);

    aptFunc->sdtNo = sdt_no;
    aptFunc->table_type = table_type;

    switch (table_type)
    {
        case DPP_SDT_TBLT_eRAM:
        {
            aptFunc->se_func_info.eramFunc.opr_mode = ((DPP_APT_ERAM_TABLE_T *)pData)->opr_mode;
            aptFunc->se_func_info.eramFunc.rd_mode = ((DPP_APT_ERAM_TABLE_T *)pData)->rd_mode;
            aptFunc->se_func_info.eramFunc.eram_set_func = ((DPP_APT_ERAM_TABLE_T *)pData)->eram_set_func;
            aptFunc->se_func_info.eramFunc.eram_get_func = ((DPP_APT_ERAM_TABLE_T *)pData)->eram_get_func;
            break;
        } 
		case DPP_SDT_TBLT_DDR3:
        {
            aptFunc->se_func_info.ddrFunc.ddr_tbl_depth = ((DPP_APT_DDR_TABLE_T *)pData)->ddr_table_depth;
            aptFunc->se_func_info.ddrFunc.ddr_set_func = ((DPP_APT_DDR_TABLE_T *)pData)->ddr_set_func;
            aptFunc->se_func_info.ddrFunc.ddr_get_func = ((DPP_APT_DDR_TABLE_T *)pData)->ddr_get_func;
            break;
        }
        case DPP_SDT_TBLT_HASH:
        {
            aptFunc->se_func_info.hashFunc.hash_set_func = ((DPP_APT_HASH_TABLE_T *)pData)->hash_set_func;
            aptFunc->se_func_info.hashFunc.hash_get_func = ((DPP_APT_HASH_TABLE_T *)pData)->hash_get_func;
            break;
        }
        case DPP_SDT_TBLT_eTCAM:
        {
            aptFunc->se_func_info.aclFunc.sdt_partner = ((DPP_APT_ACL_TABLE_T *)pData)->sdt_partner;
            aptFunc->se_func_info.aclFunc.acl_set_func = ((DPP_APT_ACL_TABLE_T *)pData)->acl_set_func;
            aptFunc->se_func_info.aclFunc.acl_get_func = ((DPP_APT_ACL_TABLE_T *)pData)->acl_get_func;
            break;
        }
        default:
        {
            ZXIC_COMM_TRACE_ERROR("dpp_apt_se_set_callback table_type[ %d ] is invalid!\n", table_type);
            return DPP_ERR;
        }
    }

    return DPP_OK;
}

DPP_STATUS dpp_apt_sw_list_insert(ZXIC_RB_CFG *rb_cfg,void *pData,ZXIC_UINT32 len)
{
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT8 *p_rb_key          = NULL;
    ZXIC_RB_TN *p_rb_new     = NULL;
    ZXIC_RB_TN *p_rb_rtn     = NULL;
    ZXIC_UINT32 rc               = DPP_OK;

    ZXIC_COMM_CHECK_POINT(rb_cfg);
    ZXIC_COMM_CHECK_POINT(pData);

    p_rb_key = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(len);
    ZXIC_COMM_CHECK_POINT(p_rb_key);
    ZXIC_COMM_MEMSET(p_rb_key,0x0,len);
    ZXIC_COMM_MEMCPY(p_rb_key,pData,len);

    p_rb_new = (ZXIC_RB_TN*)ZXIC_COMM_MALLOC(sizeof(ZXIC_RB_TN));
    if (NULL == (p_rb_new))
    {
        ZXIC_COMM_FREE(p_rb_key);
        ZXIC_COMM_TRACE_ERROR("\n ICM %s:%d[Error:POINT NULL] !\n", __FILE__, __LINE__);
        return ZXIC_PAR_CHK_POINT_NULL;
    }
    ZXIC_COMM_MEMSET(p_rb_new, 0, ZXIC_SIZEOF(ZXIC_RB_TN));
    INIT_RBT_TN(p_rb_new, p_rb_key);

    rc = zxic_comm_rb_insert(rb_cfg, p_rb_new, &p_rb_rtn);
    if(rc == ZXIC_RBT_RC_UPDATE)
    {
        ZXIC_COMM_CHECK_POINT(p_rb_rtn);
        ZXIC_COMM_MEMCPY(p_rb_rtn->p_key,pData,len);
        ZXIC_COMM_FREE(p_rb_new);
        ZXIC_COMM_FREE(p_rb_key);
        ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "update exist entry!\n");
        return DPP_OK;
    }

    return rc;
}

DPP_STATUS dpp_apt_sw_list_search(ZXIC_RB_CFG *rb_cfg,void *pData,ZXIC_UINT32 len)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_RB_TN *p_rb_rtn     = NULL;

    ZXIC_COMM_CHECK_POINT(rb_cfg);
    
    rc = zxic_comm_rb_search(rb_cfg,pData,&p_rb_rtn);
    if(DPP_OK != rc)
    {
        return rc;
    }
    //ZXIC_COMM_CHECK_RC_NO_ASSERT( rc, "zxic_comm_rb_search");

    ZXIC_COMM_MEMCPY(pData,p_rb_rtn->p_key,len);
    return rc;
}

DPP_STATUS dpp_apt_sw_list_delete(ZXIC_RB_CFG *rb_cfg,void *pData,ZXIC_UINT32 len)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_RB_TN *p_rb_rtn     = NULL;

    ZXIC_COMM_CHECK_POINT(rb_cfg);

    rc = zxic_comm_rb_delete(rb_cfg,pData,&p_rb_rtn);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "zxic_comm_rb_delete");
    ZXIC_COMM_FREE(p_rb_rtn->p_key);
    ZXIC_COMM_FREE(p_rb_rtn);

    return rc;
}

DPP_STATUS dpp_apt_get_zblock_index(ZXIC_UINT32 zblock_bitmap,ZXIC_UINT32 *zblk_idx)
{
    ZXIC_UINT32 index0 = 0;
    ZXIC_UINT32 index1 = 0;

    ZXIC_COMM_CHECK_POINT(zblk_idx);

    for (index0 = 0; index0 < 32;index0++)
    {
        if((zblock_bitmap>>index0)&0x1)
        {
            *(zblk_idx + index1) = index0;
            index1++;
        }
    }

    return DPP_OK;
}

DPP_STATUS dpp_dtb_queue_req_and_init(DPP_DEV_T *dev, ZXIC_UINT32 *p_queue_id, DPP_DTB_QUEUE_TYPE_E type)
{
    ZXIC_UINT32 rc                       = DPP_OK;
    ZXIC_UINT32 queue_id                 = 0;
    ZXIC_UINT32 dma_size                 = 2 * DTB_QUEUE_DMA_SIZE;
    ZXIC_UINT16 vport                    = 0;
    ZXIC_UINT32 vector                   = 0;
    DTB_QUEUE_DMA_ADDR_INFO tDmaAddrInfo = {0};

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);
    ZXIC_COMM_CHECK_POINT_NO_ASSERT(p_queue_id);

    vport = DEV_PCIE_VPORT(dev);

    //申请队列
    rc = dpp_dtb_queue_requst_ex(dev, "pf", &queue_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_requst");

    /*分配下表DMA内存*/
    ZXIC_COMM_MEMSET_S(&tDmaAddrInfo, sizeof(DTB_QUEUE_DMA_ADDR_INFO), 0x0, sizeof(DTB_QUEUE_DMA_ADDR_INFO));
    rc = dpp_dtb_queue_dma_mem_alloc(dev, queue_id, dma_size);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_dma_mem_alloc");

    rc = dpp_dtb_queue_dma_mem_get(dev, queue_id, &tDmaAddrInfo);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_dma_mem_get");

    // 配置下表地址空间
    rc = dpp_dtb_queue_down_table_addr_set(dev, queue_id, 
                    tDmaAddrInfo.dma_phy_addr, tDmaAddrInfo.dma_vir_addr);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_down_table_addr_set");

    // 配置dump表地址空间
    rc = dpp_dtb_queue_dump_table_addr_set(dev, queue_id,
                                           tDmaAddrInfo.dma_phy_addr + DTB_QUEUE_DMA_SIZE,
                                           tDmaAddrInfo.dma_vir_addr + DTB_QUEUE_DMA_SIZE);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_dump_table_addr_set");

    rc = dpp_dtb_user_info_set(dev, queue_id, vport, vector);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_user_info_set");

    rc = dpp_dtb_queue_func_type_set(dev, queue_id, type);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_func_type_set");

    rc =  dpp_dtb_queue_init_flag_set(dev, queue_id, DPP_DTB_QUEUE_INIT);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_init_flag_set");

    *p_queue_id = queue_id;

    return DPP_OK;
}

DPP_STATUS dpp_apt_dtb_res_init(DPP_DEV_T *dev)
{
    ZXIC_UINT32 rc                       = DPP_OK;
    ZXIC_UINT32 table_queue_id           = 0;
    ZXIC_UINT32 stat_queue_id           = 0;
    
    ZXIC_MUTEX_T *p_self_recover_mutex   = NULL;
    DPP_DEV_MUTEX_TYPE_E mutex           = 0;

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);

    mutex = DPP_DEV_MUTEX_T_SELF_RECOVER;
    rc = dpp_dev_opr_mutex_get(dev, (ZXIC_UINT32)mutex, &p_self_recover_mutex);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_self_recover_mutex);
    ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_lock");

    rc = dpp_dtb_queue_req_and_init(dev, &table_queue_id, DPP_DTB_QUEUE_TYPE_TABLE);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_queue_req_and_init", p_self_recover_mutex);

    rc = dpp_dtb_queue_req_and_init(dev, &stat_queue_id, DPP_DTB_QUEUE_TYPE_STAT);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_queue_req_and_init", p_self_recover_mutex);

    rc = dpp_dtb_base_timeout_cal(dev);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_base_timeout_cal", p_self_recover_mutex);

    rc = zxic_comm_mutex_unlock(p_self_recover_mutex);
    ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_unlock");

    ZXIC_COMM_PRINT("vport 0x%x request table queue %d stat queue %d\n", DEV_PCIE_VPORT(dev), table_queue_id, stat_queue_id);

    return DPP_OK;
}

/***********************************************************/
/** 根据设备id对表项公共参数初始化
* @param   dev_id      设备号
* @return  
* @remark  无
* @see
* @author  cq      @date  2023/11/09
************************************************************/
DPP_STATUS dpp_apt_se_callback_init(DPP_DEV_T *dev)
{
    ZXIC_UINT32 sdt_no = 0;
    ZXIC_UINT32 slot = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    slot = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(slot, 0, DPP_PCIE_SLOT_MAX - 1);

    for(sdt_no=0;sdt_no<DPP_DEV_SDT_ID_MAX;sdt_no++)
    {
        ZXIC_COMM_MEMSET(&g_apt_se_callback[slot][sdt_no],0x0,sizeof(SE_APT_CALLBACK_T));
    }

    return DPP_OK;
}

/***********************************************************/
/** 释放sdt资源以及适配资源
* @param   dev_id      设备号
* @param   sdt_no      sdt号
* @return  
* @remark  无
* @see
* @author  cq      @date  2023/11/09
************************************************************/
DPP_STATUS dpp_apt_sdt_res_deinit(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    /*sdt资源初始化，包括硬件和软件*/
    rc = dpp_sdt_tbl_write(dev,sdt_no,0,NULL,1);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_sdt_tbl_write"); 

    /*适配资源初始化*/
    ZXIC_COMM_MEMSET(&g_apt_se_callback[DEV_ID(dev)][sdt_no],0x0,sizeof(SE_APT_CALLBACK_T));
    
    return DPP_OK;
}

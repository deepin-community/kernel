/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_apt_se_hash.c
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : chenqin00181032
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
#include "dpp_hash.h"
#include "dpp_sdt.h"
#include "dpp_dtb_cfg.h"
#include "dpp_dtb_table.h"
#include "dpp_dtb_table_api.h"

static DPP_SE_CFG *g_apt_se_cfg[DPP_PCIE_SLOT_MAX] = {NULL};

DPP_SE_CFG *dpp_apt_get_se_cfg(DPP_DEV_T *dev)
{
    ZXIC_UINT32 slot = 0;
    if(NULL==dev)
    {
        return NULL;
    }
    slot = DEV_PCIE_SLOT(dev);
    if(slot<DPP_PCIE_SLOT_MAX)
    {
        return g_apt_se_cfg[slot];
    }
    return NULL;
}
/***********************************************************/
/** hash表全局资源初始化
* @param   dev_id  设备号 
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_hash_global_res_init(DPP_DEV_T *dev)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 slot = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    slot = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_INDEX(slot,0,DPP_PCIE_SLOT_MAX-1);
    if(NULL==g_apt_se_cfg[slot])
    {
        g_apt_se_cfg[slot] = (DPP_SE_CFG *)ZXIC_COMM_MALLOC(sizeof(DPP_SE_CFG));
        ZXIC_COMM_CHECK_POINT(g_apt_se_cfg[slot]);
        rc = dpp_se_init(dev, g_apt_se_cfg[slot]);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_se_init");

        rc = dpp_se_client_init(g_apt_se_cfg[slot], ZXIC_COMM_VAL_TO_PTR(DEV_ID(dev)));
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_se_client_init");

        rc = dpp_dev_hash_opr_mutex_create(dev);
        ZXIC_COMM_CHECK_RC(rc, "dpp_dev_hash_opr_mutex_create");
    }

    return rc;
}

/***********************************************************/
/** hash表全局资源去初始化
* @param   dev_id  设备号 
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/08/01
************************************************************/
DPP_STATUS dpp_apt_hash_global_res_uninit(DPP_DEV_T *dev)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 slot = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    slot = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_INDEX(slot,0,DPP_PCIE_SLOT_MAX-1);
    if(g_apt_se_cfg[slot] != NULL)
    {
        ZXIC_COMM_FREE(g_apt_se_cfg[slot]);
        g_apt_se_cfg[slot] = NULL;
    }

    rc = dpp_dev_hash_opr_mutex_destroy(dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_hash_opr_mutex_destroy");
    return rc;
}

/***********************************************************/
/** hash引擎初始化
* @param   dev_id  设备号 
* @param   func_num     需初始化的hash引擎个数 1~4
* @param   pHashFuncRes  每个hash引擎分配的zblock个数和编号，以及分配模式(混合模式或者纯片内模式)
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_hash_func_res_init(DPP_DEV_T *dev,ZXIC_UINT32 func_num,DPP_APT_HASH_FUNC_RES_T *pHashFuncRes)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 zblk_idx[32] = {0};
    DPP_APT_HASH_FUNC_RES_T *pHashFuncResTemp = NULL;
    DPP_SE_CFG *p_se_cfg = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_UPPER(func_num, HASH_FUNC_ID_NUM);
    ZXIC_COMM_CHECK_POINT(pHashFuncRes);

    p_se_cfg = dpp_apt_get_se_cfg(dev);
    ZXIC_COMM_CHECK_POINT(p_se_cfg);
    for (index = 0; index < func_num;index++)
    {
        ZXIC_COMM_MEMSET(zblk_idx,0x0,sizeof(zblk_idx));
        pHashFuncResTemp = pHashFuncRes+index;
        rc = dpp_apt_get_zblock_index(pHashFuncResTemp->zblk_bitmap,zblk_idx);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_apt_get_zblock_index");

        rc = dpp_hash_init(p_se_cfg, 
                            pHashFuncResTemp->func_id, 
                            pHashFuncResTemp->zblk_num, 
                            zblk_idx, 
                            pHashFuncResTemp->ddr_dis);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_hash_init");
    }

    return rc;

}

/***********************************************************/
/** hash引擎初始化(删除硬件数据)
* @param   dev_id  设备号 
* @param   func_num     需初始化的hash引擎个数 1~4
* @param   pHashFuncRes  每个hash引擎分配的zblock个数和编号，以及分配模式(混合模式或者纯片内模式)
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_hash_func_flush_hardware_all(DPP_DEV_T *dev, 
                                            ZXIC_UINT32 func_num, 
                                            DPP_APT_HASH_FUNC_RES_T *pHashFuncRes, 
                                            ZXIC_UINT32 queue_id)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 index = 0;
    DPP_APT_HASH_FUNC_RES_T *pHashFuncResTemp = NULL;
    DPP_SE_CFG *p_se_cfg = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_UPPER(func_num, HASH_FUNC_ID_NUM);
    p_se_cfg = dpp_apt_get_se_cfg(dev);
    ZXIC_COMM_CHECK_POINT(p_se_cfg);
    for (index = 0; index < func_num; index++)
    {
        pHashFuncResTemp = pHashFuncRes + index;

        rc = dpp_dtb_zcam_space_clr(dev, p_se_cfg, queue_id, pHashFuncResTemp->func_id);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_zcam_space_clr");
    }

    return rc;
    
}

/***********************************************************/
/** hash引擎bulk空间初始化
* @param   dev_id  设备号 
* @param   bulk_num     需初始化的bulk表个数 1~32
* @param   pBulkRes  zcell和zreg资源占用信息，如果是混合模式，需进行DDR资源分配
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_hash_bulk_res_init(DPP_DEV_T *dev,ZXIC_UINT32 bulk_num,DPP_APT_HASH_BULK_RES_T *pBulkRes)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 dev_id = 0;
    DPP_APT_HASH_BULK_RES_T *pHashBulkResTemp = NULL;
    DPP_SE_CFG *p_se_cfg = NULL;
    DPP_HASH_DDR_RESC_CFG_T ddr_resc_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_UPPER(bulk_num, HASH_FUNC_ID_NUM*HASH_BULK_NUM);
    ZXIC_COMM_CHECK_POINT(pBulkRes);
    p_se_cfg = dpp_apt_get_se_cfg(dev);
    ZXIC_COMM_CHECK_POINT(p_se_cfg);

    for (index = 0; index < bulk_num;index++)
    {
        ZXIC_COMM_MEMSET(&ddr_resc_cfg,0x0,sizeof(DPP_HASH_DDR_RESC_CFG_T));
        pHashBulkResTemp = pBulkRes+index;
        
        ddr_resc_cfg.ddr_baddr = pHashBulkResTemp->ddr_baddr;
        ddr_resc_cfg.ddr_item_num = pHashBulkResTemp->ddr_item_num;
        ddr_resc_cfg.ddr_width_mode = pHashBulkResTemp->ddr_width_mode;
        ddr_resc_cfg.ddr_crc_sel = pHashBulkResTemp->ddr_crc_sel;
        ddr_resc_cfg.ddr_ecc_en = pHashBulkResTemp->ddr_ecc_en;
        
        rc = dpp_hash_bulk_init(p_se_cfg,
                            pHashBulkResTemp->func_id,
                            pHashBulkResTemp->bulk_id,
                            &ddr_resc_cfg,
                            pHashBulkResTemp->zcell_num,
                            pHashBulkResTemp->zreg_num);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_hash_bulk_init");
    }

    return rc;
}

/***********************************************************/
/** hash业务表属性初始化
* @param   dev_id  设备号 
* @param   tbl_num     需初始化的业务表表个数 1~128
* @param   pHashTbl  sdt配置信息，初始化标记和业务结构体码流转换函数
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_hash_tbl_res_init(DPP_DEV_T *dev,ZXIC_UINT32 tbl_num,DPP_APT_HASH_TABLE_T *pHashTbl)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 index = 0;
    DPP_APT_HASH_TABLE_T *pHashTblTemp = NULL;
    DPP_SE_CFG *p_se_cfg = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_UPPER(tbl_num, HASH_FUNC_ID_NUM*HASH_TBL_ID_NUM);
    ZXIC_COMM_CHECK_POINT(pHashTbl);
    p_se_cfg = dpp_apt_get_se_cfg(dev);
    ZXIC_COMM_CHECK_POINT(p_se_cfg);

    for (index = 0; index < tbl_num;index++)
    {
        pHashTblTemp = pHashTbl+ index;
        rc = dpp_sdt_tbl_write(dev,
                                   pHashTblTemp->sdtNo,
                                   pHashTblTemp->hashSdt.table_type,
                                   &pHashTblTemp->hashSdt,
                                   SDT_OPER_ADD);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_sdt_tbl_write");

        rc = dpp_hash_tbl_id_info_init(p_se_cfg,
                                           pHashTblTemp->hashSdt.hash_id,
                                           pHashTblTemp->hashSdt.hash_table_id,
                                           pHashTblTemp->tbl_flag,
                                           pHashTblTemp->hashSdt.hash_table_width,
                                           pHashTblTemp->hashSdt.key_size);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_hash_tbl_id_info_init_ex");

        rc = dpp_apt_set_callback(dev,
                        pHashTblTemp->sdtNo,
                        pHashTblTemp->hashSdt.table_type,
                        (ZXIC_VOID *)pHashTblTemp);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_apt_set_callback");
    }

    return rc;
}

/***********************************************************/
/** dtb hash表项插入/更新
* @param   dev_id  设备号 
* @param   sdt_no  sdt号 0~255
* @param   pData   插入hash表项信息,由业务确定
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_dtb_hash_insert(DPP_DEV_T *dev,ZXIC_UINT32 queue_id,ZXIC_UINT32 sdt_no,void *pData)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT8 key_valid = 1;
    DPP_HASH_ENTRY entry = {0};
    DPP_SDTTBL_HASH_T sdt_hash_info = {0};  /*SDT内容*/
    ZXIC_UINT8 aucKey[HASH_KEY_MAX] = {0};
    ZXIC_UINT8 aucRst[HASH_RST_MAX] = {0};
    SE_APT_CALLBACK_T *pAptCallback = NULL;
    DPP_DTB_HASH_ENTRY_INFO_T tDtbHashEntry = {0};
    ZXIC_UINT32 element_id = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_hash_info);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_soft_sdt_tbl_get");
    
    pAptCallback = dpp_apt_get_func(dev, sdt_no);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pAptCallback);

    entry.p_key = aucKey;
    entry.p_rst = aucRst;
    ZXIC_COMM_MEMSET(entry.p_key,0x0,sizeof(aucKey));
    ZXIC_COMM_MEMSET(entry.p_rst,0x0,sizeof(aucRst));
    entry.p_key[0] = DPP_GET_HASH_KEY_CTRL(key_valid, 
                                            sdt_hash_info.hash_table_width, 
                                            sdt_hash_info.hash_table_id);
    rc = pAptCallback->se_func_info.hashFunc.hash_set_func(pData,&entry);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "hash_set_func");

    tDtbHashEntry.p_actu_key = &entry.p_key[1];
    tDtbHashEntry.p_rst = entry.p_rst;

    rc = dpp_dtb_hash_dma_insert(dev, queue_id,sdt_no,1,&tDtbHashEntry,&element_id);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_hash_dma_insert");
                                   
    return rc;
}

/***********************************************************/
/** dtb hash表项删除
* @param   dev_id  设备号 
* @param   sdt_no  sdt号 0~255
* @param   pData   删除hash表项信息,由业务传入
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
DPP_STATUS dpp_apt_dtb_hash_delete(DPP_DEV_T *dev,ZXIC_UINT32 queue_id,ZXIC_UINT32 sdt_no,void *pData)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT8 key_valid = 1;
    DPP_HASH_ENTRY entry = {0};
    DPP_SDTTBL_HASH_T sdt_hash_info = {0};  /*SDT内容*/
    ZXIC_UINT8 aucKey[HASH_KEY_MAX] = {0}; 
    ZXIC_UINT8 aucRst[HASH_RST_MAX] = {0};
    SE_APT_CALLBACK_T *pAptCallback = NULL;
    DPP_DTB_HASH_ENTRY_INFO_T tDtbHashEntry = {0};
    // ZXIC_UINT32 queue_id = 0;
    ZXIC_UINT32 element_id = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_hash_info);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_soft_sdt_tbl_get");
    
    pAptCallback = dpp_apt_get_func(dev, sdt_no);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pAptCallback);

    entry.p_key = aucKey;
    entry.p_rst = aucRst;
    ZXIC_COMM_MEMSET(entry.p_key,0x0,sizeof(aucKey));
    ZXIC_COMM_MEMSET(entry.p_rst,0x0,sizeof(aucRst));
    entry.p_key[0] = DPP_GET_HASH_KEY_CTRL(key_valid, 
                                            sdt_hash_info.hash_table_width, 
                                            sdt_hash_info.hash_table_id);
    rc = pAptCallback->se_func_info.hashFunc.hash_set_func(pData,&entry);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "hash_set_func");
    
    ZXIC_COMM_MEMSET(&tDtbHashEntry,0x0,sizeof(DPP_DTB_HASH_ENTRY_INFO_T));
    ZXIC_COMM_MEMSET(entry.p_rst,0x0,sizeof(aucRst));
    tDtbHashEntry.p_actu_key = &entry.p_key[1];
    tDtbHashEntry.p_rst = entry.p_rst;
    rc = dpp_dtb_hash_dma_delete(dev, queue_id,sdt_no,1,&tDtbHashEntry,&element_id);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_hash_dma_delete");

    return rc;
}

/***********************************************************/
/** 软件查找存储在ZCAM空间的hash表项的zcam位置
* @param   dev           设备号，支持多芯片 
* @param   sdt_no        SDT表号
* @param   pData         查找键值信息(查找成功后，填充rst)
* @param   p_pos_info    出参，zcam位置信息
* @param   p_srh_succ    出参，查找是否成功
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
DPP_STATUS dpp_hash_zcam_pos_get(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no, void *pData, DPP_HASH_ZCAM_POS_INFO *p_pos_info, ZXIC_UINT8 *p_srh_succ)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT8 key_valid = 1;
    DPP_HASH_CFG *p_hash_cfg = NULL;
    ZXIC_RB_TN *p_rb_tn_rtn = NULL;
    SE_ITEM_CFG *p_item = NULL;
    ZXIC_MUTEX_T *p_hash_mutex = NULL;
    DPP_HASH_RBKEY_INFO *p_rbkey = NULL;
    DPP_HASH_RBKEY_INFO srh_rbkey = {0};
    HASH_ENTRY_CFG hash_entry_cfg = {0}; 
    DPP_HASH_ENTRY hash_entry = {0};   
    ZXIC_UINT8 aucKey[HASH_KEY_MAX] = {0};
    ZXIC_UINT8 aucRst[HASH_RST_MAX] = {0};

    SE_APT_CALLBACK_T *pAptCallback = NULL;

    //从sdt_no中获取hash配置
    rc = dpp_hash_get_hash_info_from_sdt(dev, sdt_no, &hash_entry_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_hash_get_hash_info_from_sdt");

    p_hash_cfg = hash_entry_cfg.p_hash_cfg;
    ZXIC_COMM_CHECK_POINT(p_hash_cfg);

    pAptCallback = dpp_apt_get_func(dev, sdt_no);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pAptCallback);

    hash_entry.p_key = aucKey;
    hash_entry.p_rst = aucRst;
    ZXIC_COMM_MEMSET_S(hash_entry.p_key, sizeof(aucKey), 0, sizeof(aucKey));
    ZXIC_COMM_MEMSET_S(hash_entry.p_rst, sizeof(aucRst), 0, sizeof(aucRst));

    hash_entry.p_key[0] = DPP_GET_HASH_KEY_CTRL(key_valid, 
                                                hash_entry_cfg.key_type, 
                                                hash_entry_cfg.table_id);
    rc = pAptCallback->se_func_info.hashFunc.hash_set_func(pData, &hash_entry);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "hash_set_func");

    ZXIC_COMM_MEMSET_S(&srh_rbkey, sizeof(DPP_HASH_RBKEY_INFO), 0, sizeof(DPP_HASH_RBKEY_INFO));
    ZXIC_COMM_CHECK_INDEX_UPPER(hash_entry_cfg.key_by_size, HASH_KEY_MAX);
    ZXIC_COMM_MEMCPY(srh_rbkey.key, hash_entry.p_key, hash_entry_cfg.key_by_size);

    rc = dpp_dev_hash_opr_mutex_get(dev, p_hash_cfg->fun_id, &p_hash_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dev_opr_mutex_get"); 
    rc = zxic_comm_mutex_lock(p_hash_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_lock"); 

    rc = zxic_comm_rb_search(&p_hash_cfg->hash_rb, (ZXIC_VOID *)&srh_rbkey, (ZXIC_VOID *)(&p_rb_tn_rtn));
    if(ZXIC_RBT_RC_SRHFAIL==rc)
    {
        ZXIC_COMM_TRACE_NOTICE("zxic_comm_rb_search fail. \n");
        rc = zxic_comm_mutex_unlock(p_hash_mutex);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
        return DPP_OK;
    }

    ZXIC_COMM_CHECK_DEV_POINT_UNLOCK(DEV_ID(dev), p_rb_tn_rtn, p_hash_mutex);
    p_rbkey = p_rb_tn_rtn->p_key;
    ZXIC_COMM_CHECK_DEV_POINT_UNLOCK(DEV_ID(dev), p_rbkey, p_hash_mutex);
    p_item = p_rbkey->p_item_info;
    ZXIC_COMM_CHECK_DEV_POINT_UNLOCK(DEV_ID(dev), p_item, p_hash_mutex);

    rc = dpp_dtb_hash_software_item_check(&hash_entry,
                                          hash_entry_cfg.key_by_size,
                                          hash_entry_cfg.rst_by_size,
                                          p_item);
    if(DPP_OK == rc)
    {
        *p_srh_succ = 1;
        p_hash_cfg->hash_stat.search_ok++;
    }

    rc = zxic_comm_mutex_unlock(p_hash_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");

    p_pos_info->entry_addr = p_item->hw_addr;
    p_pos_info->entry_pos  = p_rbkey->entry_pos;
    p_pos_info->entry_size = p_rbkey->entry_size;

    return DPP_OK;
}


/***********************************************************/
/** dtb hash表项批量插入/更新
* @param   dev_id     设备号 
* @param   queue_id   队列id
* @param   sdt_no     sdt号 0~255
* @param   entry_num  插入条目数
* @param   entry_size 插入条目结构体大小
* @param   pData      插入hash表项信息,由业务确定
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/10/23
************************************************************/
DPP_STATUS dpp_apt_dtb_multi_hash_insert(DPP_DEV_T *dev,
                                          ZXIC_UINT32 queue_id,
                                          ZXIC_UINT32 sdt_no,
                                          ZXIC_UINT32 entry_num,
                                          ZXIC_UINT32 entry_size, 
                                          ZXIC_VOID *pData)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT8 key_valid = 1;
    ZXIC_UINT32 entry_index = 0;
    DPP_HASH_ENTRY entry = {0};
    DPP_SDTTBL_HASH_T sdt_hash_info = {0};  /*SDT内容*/
    ZXIC_UINT8 aucKey[HASH_KEY_MAX] = {0};
    ZXIC_UINT8 aucRst[HASH_RST_MAX] = {0};

    SE_APT_CALLBACK_T *pAptCallback = NULL;
    DPP_DTB_HASH_ENTRY_INFO_T *p_oneHashEntry = NULL;
    DPP_DTB_HASH_ENTRY_INFO_T *p_multiHashEntry = NULL;
    ZXIC_UINT8 *p_key = NULL;
    ZXIC_UINT8 *p_rst = NULL;
    ZXIC_UINT8 *p_temp_data = NULL;
    ZXIC_UINT32 element_id = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_LOWER(entry_num, 1);

    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_hash_info);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_soft_sdt_tbl_get");
    
    pAptCallback = dpp_apt_get_func(dev, sdt_no);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pAptCallback);

    entry.p_key = aucKey;
    entry.p_rst = aucRst;
    
    p_multiHashEntry = (DPP_DTB_HASH_ENTRY_INFO_T *)ZXIC_COMM_MALLOC(entry_num * sizeof(DPP_DTB_HASH_ENTRY_INFO_T));
    ZXIC_COMM_CHECK_POINT(p_multiHashEntry);
    p_key = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(entry_num * HASH_KEY_MAX);
    ZXIC_COMM_CHECK_POINT_MEMORY_FREE(p_key, p_multiHashEntry);
    p_rst = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(entry_num * HASH_RST_MAX);
    ZXIC_COMM_CHECK_POINT_MEMORY_FREE2PTR_NO_ASSERT(p_rst, p_key, p_multiHashEntry);
    ZXIC_COMM_MEMSET_S(p_multiHashEntry,entry_num * sizeof(DPP_DTB_HASH_ENTRY_INFO_T),0x0,entry_num * sizeof(DPP_DTB_HASH_ENTRY_INFO_T));
    ZXIC_COMM_MEMSET_S(p_key,entry_num * HASH_KEY_MAX,0x0,entry_num * HASH_KEY_MAX);
    ZXIC_COMM_MEMSET_S(p_rst,entry_num * HASH_RST_MAX,0x0,entry_num * HASH_RST_MAX);

    for(entry_index = 0; entry_index<entry_num; entry_index++)
    {
        ZXIC_COMM_MEMSET_S(entry.p_key,HASH_KEY_MAX, 0x0,sizeof(aucKey));
        ZXIC_COMM_MEMSET_S(entry.p_rst,HASH_RST_MAX, 0x0,sizeof(aucRst));
        entry.p_key[0] = DPP_GET_HASH_KEY_CTRL(key_valid, 
                                            sdt_hash_info.hash_table_width, 
                                            sdt_hash_info.hash_table_id);
        p_temp_data = (ZXIC_UINT8 *)pData + entry_index * entry_size;
        rc = pAptCallback->se_func_info.hashFunc.hash_set_func((ZXIC_VOID *)p_temp_data,&entry);
        ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE3PTR_NO_ASSERT(DEV_ID(dev), rc, "hash_set_func", p_rst, p_key, p_multiHashEntry);
        
        p_oneHashEntry = p_multiHashEntry + entry_index;
        p_oneHashEntry->p_actu_key = p_key + entry_index * HASH_KEY_MAX;
        p_oneHashEntry->p_rst = p_rst + entry_index * HASH_RST_MAX;
        ZXIC_COMM_MEMCPY_S(p_oneHashEntry->p_actu_key,HASH_KEY_MAX,&entry.p_key[1],sizeof(aucKey)-1);
        ZXIC_COMM_MEMCPY_S(p_oneHashEntry->p_rst,HASH_RST_MAX,entry.p_rst,sizeof(aucRst));
    }

    rc = dpp_dtb_hash_dma_insert(dev, queue_id,sdt_no,entry_num,p_multiHashEntry,&element_id);
    ZXIC_COMM_FREE(p_rst);
    ZXIC_COMM_FREE(p_key);
    ZXIC_COMM_FREE(p_multiHashEntry);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_hash_dma_insert");
                                   
    return rc;
}

/***********************************************************/
/** dtb hash表项批量删除
* @param   dev_id     设备号 
* @param   queue_id   队列id
* @param   sdt_no     sdt号 0~255
* @param   entry_num  删除条目数
* @param   entry_size 删除条目结构体大小
* @param   pData      删除hash表项信息,由业务确定
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/10/23
************************************************************/
DPP_STATUS dpp_apt_dtb_multi_hash_delete(DPP_DEV_T *dev,
                                          ZXIC_UINT32 queue_id,
                                          ZXIC_UINT32 sdt_no,
                                          ZXIC_UINT32 entry_num,
                                          ZXIC_UINT32 entry_size, 
                                          ZXIC_VOID *pData)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT8 key_valid = 1;
    ZXIC_UINT32 entry_index = 0;
    DPP_HASH_ENTRY entry = {0};
    DPP_SDTTBL_HASH_T sdt_hash_info = {0};  /*SDT内容*/
    ZXIC_UINT8 aucKey[HASH_KEY_MAX] = {0};
    ZXIC_UINT8 aucRst[HASH_RST_MAX] = {0};
    SE_APT_CALLBACK_T *pAptCallback = NULL;
    DPP_DTB_HASH_ENTRY_INFO_T *p_oneHashEntry = NULL;
    DPP_DTB_HASH_ENTRY_INFO_T *p_multiHashEntry = NULL;
    ZXIC_UINT8 *p_key = NULL;
    ZXIC_UINT8 *p_rst = NULL;
    ZXIC_UINT8 *p_temp_data = NULL;
    ZXIC_UINT32 element_id = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_LOWER(entry_num, 1);

    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_hash_info);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_soft_sdt_tbl_get");
    
    pAptCallback = dpp_apt_get_func(dev, sdt_no);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pAptCallback);

    entry.p_key = aucKey;
    entry.p_rst = aucRst;
    
    p_multiHashEntry = (DPP_DTB_HASH_ENTRY_INFO_T *)ZXIC_COMM_MALLOC(entry_num * sizeof(DPP_DTB_HASH_ENTRY_INFO_T));
    ZXIC_COMM_CHECK_POINT(p_multiHashEntry);
    p_key = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(entry_num * HASH_KEY_MAX);
    ZXIC_COMM_CHECK_POINT_MEMORY_FREE(p_key, p_multiHashEntry);
    p_rst = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(entry_num * HASH_RST_MAX);
    ZXIC_COMM_CHECK_POINT_MEMORY_FREE2PTR_NO_ASSERT(p_rst, p_key, p_multiHashEntry);
    ZXIC_COMM_MEMSET_S(p_multiHashEntry,entry_num * sizeof(DPP_DTB_HASH_ENTRY_INFO_T),0x0,entry_num * sizeof(DPP_DTB_HASH_ENTRY_INFO_T));
    ZXIC_COMM_MEMSET_S(p_key,entry_num * HASH_KEY_MAX,0x0,entry_num * HASH_KEY_MAX);
    ZXIC_COMM_MEMSET_S(p_rst,entry_num * HASH_RST_MAX,0x0,entry_num * HASH_RST_MAX);

    for(entry_index = 0; entry_index<entry_num; entry_index++)
    {
        ZXIC_COMM_MEMSET_S(entry.p_key,HASH_KEY_MAX, 0x0,sizeof(aucKey));
        ZXIC_COMM_MEMSET_S(entry.p_rst,HASH_RST_MAX, 0x0,sizeof(aucRst));
        entry.p_key[0] = DPP_GET_HASH_KEY_CTRL(key_valid, 
                                            sdt_hash_info.hash_table_width, 
                                            sdt_hash_info.hash_table_id);
        p_temp_data = (ZXIC_UINT8 *)pData + entry_index * entry_size;
        rc = pAptCallback->se_func_info.hashFunc.hash_set_func((ZXIC_VOID *)p_temp_data,&entry);
        ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE3PTR_NO_ASSERT(DEV_ID(dev), rc, "hash_set_func", p_rst, p_key, p_multiHashEntry);
        
        p_oneHashEntry = p_multiHashEntry + entry_index;
        p_oneHashEntry->p_actu_key = p_key + entry_index * HASH_KEY_MAX;
        p_oneHashEntry->p_rst = p_rst + entry_index * HASH_RST_MAX;
        ZXIC_COMM_MEMCPY_S(p_oneHashEntry->p_actu_key,HASH_KEY_MAX,&entry.p_key[1],sizeof(aucKey)-1);
    }

    rc = dpp_dtb_hash_dma_delete(dev, queue_id,sdt_no,entry_num,p_multiHashEntry,&element_id);
    ZXIC_COMM_FREE(p_rst);
    ZXIC_COMM_FREE(p_key);
    ZXIC_COMM_FREE(p_multiHashEntry);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_hash_dma_delete");
                                   
    return rc;
}

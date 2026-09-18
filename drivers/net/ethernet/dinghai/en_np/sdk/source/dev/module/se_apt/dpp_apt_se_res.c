/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_apt_se_res.c
* 文件标识 :
* 内容摘要 : 所有表资源获取接口文件
* 其它说明 :
* 当前版本 :
* 作    者 : cq
* 完成日期 : 2024/11/14
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/

#include "dpp_apt_se_api.h"
#include "dpp_apt_se.h"
#include "dpp_stat_api.h"
#include "dpp_agent_channel.h"
#include "dpp_sdt.h"
#include "dpp_dtb_table_api.h"
#include "dpp_drv_eram.h"
#include "dpp_drv_sdt.h"
#include "dpp_dtb_table.h"
#include "dpp_dtb.h"
#include "dpp_kernel_init.h"

ZXIC_UINT32 dpp_get_se_buff_size(ZXIC_UINT32 opr)
{
    ZXIC_UINT32 buff_size = 0;

    switch (opr) 
    {
        case HASH_FUNC_BULK_REQ:
        {
            buff_size = ZXIC_SIZEOF(SE_HASH_FUNC_BULK_T);
            break;
        }
        case HASH_TBL_REQ:
        {
            buff_size = ZXIC_SIZEOF(SE_HASH_TBL_T);
            break;
        }
        case ERAM_TBL_REQ:
        {
            buff_size = ZXIC_SIZEOF(SE_ERAM_TBL_T);
            break;
        }
        case ACL_TBL_REQ:
        {
            buff_size = ZXIC_SIZEOF(SE_ACL_TBL_T);
            break;
        }
        case LPM_TBL_REQ:
        {
            buff_size = ZXIC_SIZEOF(SE_LPM_TBL_T);
            break;
        }
        case DDR_TBL_REQ:
        {
            buff_size = ZXIC_SIZEOF(SE_DDR_TBL_T);
            break;
        }
        case STAT_CFG_REQ:
        {
            buff_size = ZXIC_SIZEOF(SE_STAT_CFG_T);
            break;
        }
        default:
            break;     
    }

    return buff_size;
}

static DPP_STATUS dpp_hash_func_bulk_set(DPP_APT_HASH_RES_INIT_T *pHashResInit, SE_HASH_FUNC_BULK_T *p_func_bulk)
{
    ZXIC_UINT32 index  = 0;
    DPP_APT_HASH_FUNC_RES_T *p_func_res = NULL;
    DPP_APT_HASH_BULK_RES_T *p_bulk_res = NULL;

    ZXIC_COMM_CHECK_POINT(pHashResInit);
    ZXIC_COMM_CHECK_POINT(p_func_bulk);
    ZXIC_COMM_CHECK_POINT(pHashResInit->func_res);
    ZXIC_COMM_CHECK_POINT(pHashResInit->bulk_res);
    ZXIC_COMM_CHECK_INDEX_UPPER(p_func_bulk->func_num,HASH_FUNC_MAX_NUM);
    ZXIC_COMM_CHECK_INDEX_UPPER(p_func_bulk->bulk_num,HASH_BULK_MAX_NUM);

    pHashResInit->func_num = p_func_bulk->func_num;
    pHashResInit->bulk_num = p_func_bulk->bulk_num;
    for(index = 0;index<(pHashResInit->func_num);index++)
    {
        p_func_res = pHashResInit->func_res + index;

        p_func_res->func_id     = p_func_bulk->fun[index].func_id;
        p_func_res->ddr_dis     = p_func_bulk->fun[index].ddr_dis;
        p_func_res->zblk_num    = p_func_bulk->fun[index].zblk_num;
        p_func_res->zblk_bitmap = p_func_bulk->fun[index].zblk_bitmap;
    }

    for(index = 0;index<(pHashResInit->bulk_num);index++)
    {
        p_bulk_res = pHashResInit->bulk_res + index;

        p_bulk_res->func_id        = p_func_bulk->bulk[index].func_id;
        p_bulk_res->bulk_id        = p_func_bulk->bulk[index].bulk_id;
        p_bulk_res->zcell_num      = p_func_bulk->bulk[index].zcell_num;
        p_bulk_res->zreg_num       = p_func_bulk->bulk[index].zreg_num;
        p_bulk_res->ddr_baddr      = p_func_bulk->bulk[index].ddr_baddr;
        p_bulk_res->ddr_item_num   = p_func_bulk->bulk[index].ddr_item_num;
        p_bulk_res->ddr_width_mode = p_func_bulk->bulk[index].ddr_width_mode;
        p_bulk_res->ddr_crc_sel    = p_func_bulk->bulk[index].ddr_crc_sel;
        p_bulk_res->ddr_ecc_en     = p_func_bulk->bulk[index].ddr_ecc_en;
    }

    return DPP_OK;
}

static DPP_STATUS dpp_hash_tbl_set(DPP_APT_HASH_RES_INIT_T *pHashResInit, SE_HASH_TBL_T *p_hash_tbl)
{
    ZXIC_UINT32 index  = 0;
    DPP_APT_HASH_TABLE_T  *p_tbl_res = NULL;

    ZXIC_COMM_CHECK_POINT(pHashResInit);
    ZXIC_COMM_CHECK_POINT(p_hash_tbl);
    ZXIC_COMM_CHECK_INDEX_UPPER(p_hash_tbl->tbl_num,HASH_TABLE_MAX_NUM);
    
    pHashResInit->tbl_num = p_hash_tbl->tbl_num;
    for(index = 0;index<(pHashResInit->tbl_num);index++)
    {
        p_tbl_res = pHashResInit->tbl_res + index;

        p_tbl_res->sdtNo                    = p_hash_tbl->table[index].sdtNo;
        p_tbl_res->sdt_partner              = p_hash_tbl->table[index].sdt_partner;
        p_tbl_res->tbl_flag                 = p_hash_tbl->table[index].tbl_flag;
        p_tbl_res->hashSdt.table_type       = p_hash_tbl->table[index].hashSdt.table_type;
        p_tbl_res->hashSdt.hash_id          = p_hash_tbl->table[index].hashSdt.hash_id;
        p_tbl_res->hashSdt.hash_table_width = p_hash_tbl->table[index].hashSdt.hash_table_width;
        p_tbl_res->hashSdt.key_size         = p_hash_tbl->table[index].hashSdt.key_size;
        p_tbl_res->hashSdt.hash_table_id    = p_hash_tbl->table[index].hashSdt.hash_table_id;
        p_tbl_res->hashSdt.learn_en         = p_hash_tbl->table[index].hashSdt.learn_en;
        p_tbl_res->hashSdt.keep_alive       = p_hash_tbl->table[index].hashSdt.keep_alive;
        p_tbl_res->hashSdt.keep_alive_baddr = p_hash_tbl->table[index].hashSdt.keep_alive_baddr;
        p_tbl_res->hashSdt.rsp_mode         = p_hash_tbl->table[index].hashSdt.rsp_mode;
        p_tbl_res->hashSdt.hash_clutch_en   = p_hash_tbl->table[index].hashSdt.hash_clutch_en;
    }

    return DPP_OK;
}

static DPP_STATUS dpp_eram_tbl_set(DPP_APT_ERAM_RES_INIT_T *pEramResInit, SE_ERAM_TBL_T *p_eram_tbl)
{
    ZXIC_UINT32 index  = 0;
    DPP_APT_ERAM_TABLE_T *p_eram_res = NULL;

    ZXIC_COMM_CHECK_POINT(pEramResInit);
    ZXIC_COMM_CHECK_POINT(p_eram_tbl);
    ZXIC_COMM_CHECK_POINT(pEramResInit->eram_res);
    ZXIC_COMM_CHECK_INDEX_UPPER(p_eram_tbl->tbl_num,ERAM_MAX_NUM);

    pEramResInit->tbl_num = p_eram_tbl->tbl_num;
    for(index = 0;index<(pEramResInit->tbl_num);index++)
    {
        p_eram_res = pEramResInit->eram_res + index;

        p_eram_res->sdtNo                    = p_eram_tbl->eram[index].sdtNo;
        p_eram_res->opr_mode                 = p_eram_tbl->eram[index].opr_mode;
        p_eram_res->rd_mode                  = p_eram_tbl->eram[index].rd_mode;
        p_eram_res->eRamSdt.table_type       = p_eram_tbl->eram[index].eRamSdt.table_type;
        p_eram_res->eRamSdt.eram_mode        = p_eram_tbl->eram[index].eRamSdt.eram_mode;
        p_eram_res->eRamSdt.eram_base_addr   = p_eram_tbl->eram[index].eRamSdt.eram_base_addr;
        p_eram_res->eRamSdt.eram_table_depth = p_eram_tbl->eram[index].eRamSdt.eram_table_depth;
        p_eram_res->eRamSdt.eram_clutch_en   = p_eram_tbl->eram[index].eRamSdt.eram_clutch_en;
    }

    return DPP_OK;
}

static DPP_STATUS dpp_acl_tbl_set(DPP_APT_ACL_RES_INIT_T *pAclResInit, SE_ACL_TBL_T *p_acl_tbl)
{
    ZXIC_UINT32 index  = 0;
    DPP_APT_ACL_TABLE_T *p_acl_res = NULL;

    ZXIC_COMM_CHECK_POINT(pAclResInit);
    ZXIC_COMM_CHECK_POINT(p_acl_tbl);
    ZXIC_COMM_CHECK_POINT(pAclResInit->acl_res);
    ZXIC_COMM_CHECK_INDEX_UPPER(p_acl_tbl->tbl_num,ETCAM_MAX_NUM);

    pAclResInit->tbl_num = p_acl_tbl->tbl_num;
    for(index = 0;index<(p_acl_tbl->tbl_num);index++)
    {
        p_acl_res = pAclResInit->acl_res + index;

        p_acl_res->sdtNo                     = p_acl_tbl->acl[index].sdtNo;
        p_acl_res->sdt_partner               = p_acl_tbl->acl[index].sdt_partner;
        p_acl_res->aclRes.block_num          = p_acl_tbl->acl[index].aclRes.block_num;
         p_acl_res->aclRes.entry_num         = p_acl_tbl->acl[index].aclRes.entry_num;
        p_acl_res->aclRes.pri_mode           = p_acl_tbl->acl[index].aclRes.pri_mode;
        ZXIC_COMM_MEMCPY_S(p_acl_res->aclRes.block_index,sizeof(ZXIC_UINT32)*DPP_ETCAM_BLOCK_NUM,
                            p_acl_tbl->acl[index].aclRes.block_index,sizeof(ZXIC_UINT32)*ETCAM_BLOCK_NUM);
        p_acl_res->aclSdt.table_type        = p_acl_tbl->acl[index].aclSdt.table_type;
        p_acl_res->aclSdt.etcam_id          = p_acl_tbl->acl[index].aclSdt.etcam_id;
        p_acl_res->aclSdt.etcam_key_mode    = p_acl_tbl->acl[index].aclSdt.etcam_key_mode;
        p_acl_res->aclSdt.etcam_table_id    = p_acl_tbl->acl[index].aclSdt.etcam_table_id;
        p_acl_res->aclSdt.no_as_rsp_mode    = p_acl_tbl->acl[index].aclSdt.no_as_rsp_mode;
        p_acl_res->aclSdt.as_en             = p_acl_tbl->acl[index].aclSdt.as_en;
        p_acl_res->aclSdt.as_eram_baddr     = p_acl_tbl->acl[index].aclSdt.as_eram_baddr;
        p_acl_res->aclSdt.as_rsp_mode       = p_acl_tbl->acl[index].aclSdt.as_rsp_mode;
        p_acl_res->aclSdt.etcam_table_depth = p_acl_tbl->acl[index].aclSdt.etcam_table_depth;
        p_acl_res->aclSdt.etcam_clutch_en   = p_acl_tbl->acl[index].aclSdt.etcam_clutch_en;
    }

    return DPP_OK;
}
#if 0
static DPP_STATUS dpp_lpm_tbl_set(DPP_APT_LPM_RES_INIT_T *pLpmResInit, SE_LPM_TBL_T *p_lpm_tbl)
{
    ZXIC_UINT32 index  = 0;
    DPP_APT_LPM_TABLE_T *p_lpm_res = NULL;

    ZXIC_COMM_CHECK_POINT(pLpmResInit);
    ZXIC_COMM_CHECK_POINT(p_lpm_tbl);
    ZXIC_COMM_CHECK_POINT(pLpmResInit->glb_res);
    ZXIC_COMM_CHECK_POINT(pLpmResInit->lpm_res);
    ZXIC_COMM_CHECK_INDEX_UPPER(p_lpm_tbl->tbl_num,LPM_MAX_NUM);

    pLpmResInit->glb_res->lpm_flags             = p_lpm_tbl->glb_res.lpm_flags;
    pLpmResInit->glb_res->zblk_num              = p_lpm_tbl->glb_res.zblk_num;
    pLpmResInit->glb_res->zblk_bitmap           = p_lpm_tbl->glb_res.zblk_bitmap;
    pLpmResInit->glb_res->mono_ipv4_zblk_num    = p_lpm_tbl->glb_res.mono_ipv4_zblk_num;
    pLpmResInit->glb_res->mono_ipv4_zblk_bitmap = p_lpm_tbl->glb_res.mono_ipv4_zblk_bitmap;
    pLpmResInit->glb_res->mono_ipv6_zblk_num    = p_lpm_tbl->glb_res.mono_ipv6_zblk_num;
    pLpmResInit->glb_res->mono_ipv6_zblk_bitmap = p_lpm_tbl->glb_res.mono_ipv6_zblk_bitmap;
    pLpmResInit->glb_res->ddr4_item_num         = p_lpm_tbl->glb_res.ddr4_item_num;
    pLpmResInit->glb_res->ddr4_baddr            = p_lpm_tbl->glb_res.ddr4_baddr;
    pLpmResInit->glb_res->ddr4_base_offset      = p_lpm_tbl->glb_res.ddr4_base_offset;
    pLpmResInit->glb_res->ddr4_ecc_en           = p_lpm_tbl->glb_res.ddr4_ecc_en;
    pLpmResInit->glb_res->ddr6_item_num         = p_lpm_tbl->glb_res.ddr6_item_num;
    pLpmResInit->glb_res->ddr6_baddr            = p_lpm_tbl->glb_res.ddr6_baddr;
    pLpmResInit->glb_res->ddr6_base_offset      = p_lpm_tbl->glb_res.ddr6_base_offset;
    pLpmResInit->glb_res->ddr6_ecc_en           = p_lpm_tbl->glb_res.ddr6_ecc_en;
    pLpmResInit->tbl_num                        = p_lpm_tbl->tbl_num;
    
    for(index = 0;index<(p_lpm_tbl->tbl_num);index++)
    {
        p_lpm_res =  pLpmResInit->lpm_res + index;

        p_lpm_res->sdtNo                  = p_lpm_tbl->lpm_res[index].sdtNo;
        p_lpm_res->lpmSdt.table_type      = p_lpm_tbl->lpm_res[index].lpmSdt.table_type;
        p_lpm_res->lpmSdt.lpm_v46_id      = p_lpm_tbl->lpm_res[index].lpmSdt.lpm_v46_id;
        p_lpm_res->lpmSdt.rsp_mode        = p_lpm_tbl->lpm_res[index].lpmSdt.rsp_mode;
        p_lpm_res->lpmSdt.lpm_table_depth = p_lpm_tbl->lpm_res[index].lpmSdt.lpm_table_depth;
        p_lpm_res->lpmSdt.lpm_clutch_en   = p_lpm_tbl->lpm_res[index].lpmSdt.lpm_clutch_en;
        ZXIC_COMM_MEMCPY_S(p_lpm_res->as_eram_cfg,sizeof(DPP_ROUTE_AS_ERAM_T)*DPP_SMMU0_LPM_AS_TBL_ID_NUM,
                         p_lpm_tbl->lpm_res[index].as_eram_cfg,sizeof(ROUTE_AS_ERAM_T)*SMMU0_LPM_AS_TBL_ID_NUM);

        p_lpm_res->as_ddr_cfg.baddr       = p_lpm_tbl->lpm_res[index].as_ddr_cfg.baddr;
        p_lpm_res->as_ddr_cfg.rsp_len     = p_lpm_tbl->lpm_res[index].as_ddr_cfg.rsp_len;
        p_lpm_res->as_ddr_cfg.ecc_en      = p_lpm_tbl->lpm_res[index].as_ddr_cfg.ecc_en; 
    }

    return DPP_OK;  
}

static DPP_STATUS dpp_ddr_tbl_set(DPP_APT_DDR_RES_INIT_T *pDdrResInit, SE_DDR_TBL_T *p_ddr_tbl)
{
    ZXIC_UINT32 index  = 0;
    DPP_APT_DDR_TABLE_T *p_ddr_res = NULL;

    ZXIC_COMM_CHECK_POINT(pDdrResInit);
    ZXIC_COMM_CHECK_POINT(p_ddr_tbl);
    ZXIC_COMM_CHECK_INDEX_UPPER(p_ddr_tbl->tbl_num,DDR_MAX_NUM);

    pDdrResInit->tbl_num = p_ddr_tbl->tbl_num;
    for(index = 0;index<(p_ddr_tbl->tbl_num);index++)
    {
        p_ddr_res = pDdrResInit->ddr_res + index;

        p_ddr_res->sdtNo                   = p_ddr_tbl->ddr[index].sdtNo;
        p_ddr_res->ddr_table_depth         = p_ddr_tbl->ddr[index].ddr_table_depth;
        p_ddr_res->eDdrSdt.table_type      = p_ddr_tbl->ddr[index].eDdrSdt.table_type;
        p_ddr_res->eDdrSdt.ddr3_base_addr  = p_ddr_tbl->ddr[index].eDdrSdt.ddr3_base_addr;
        p_ddr_res->eDdrSdt.ddr3_share_type = p_ddr_tbl->ddr[index].eDdrSdt.ddr3_share_type;
        p_ddr_res->eDdrSdt.ddr3_rw_len     = p_ddr_tbl->ddr[index].eDdrSdt.ddr3_rw_len;
        p_ddr_res->eDdrSdt.ddr3_sdt_num    = p_ddr_tbl->ddr[index].eDdrSdt.ddr3_sdt_num;
        p_ddr_res->eDdrSdt.ddr3_ecc_en     = p_ddr_tbl->ddr[index].eDdrSdt.ddr3_ecc_en;
        p_ddr_res->eDdrSdt.ddr3_clutch_en  = p_ddr_tbl->ddr[index].eDdrSdt.ddr3_clutch_en; 
    }

    return DPP_OK;
}
#endif
static DPP_STATUS dpp_stat_cfg_set(DPP_APT_STAT_RES_INIT_T *pStatResInit, SE_STAT_CFG_T *p_stat_cfg)
{
    ZXIC_COMM_CHECK_POINT(pStatResInit);
    ZXIC_COMM_CHECK_POINT(p_stat_cfg);
    
    pStatResInit->eram_baddr     = p_stat_cfg->eram_baddr;
    pStatResInit->eram_depth     = p_stat_cfg->eram_depth;
    pStatResInit->ddr_baddr      = p_stat_cfg->ddr_baddr;
    pStatResInit->ppu_ddr_offset = p_stat_cfg->ppu_ddr_offset;

    return DPP_OK;
}

/***********************************************************/
/** stat资源初始化
* @param   dev  设备 
* @param   stat_res_init  stat资源
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/02/22
************************************************************/
static DPP_STATUS dpp_apt_stat_res_init(DPP_DEV_T *dev, DPP_APT_STAT_RES_INIT_T *stat_res_init)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id,stat_res_init);

    // init stat
    rc = dpp_stat_ppu_eram_baddr_set(dev, stat_res_init->eram_baddr);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_ppu_eram_baddr_set");

    rc = dpp_stat_ppu_eram_depth_set(dev, stat_res_init->eram_depth); /* unit: 128bits */
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_ppu_eram_depth_set");

    return rc;
}

/***********************************************************/
/** 消息通道获取hash表func/bulk信息
* @param   dev            NP设备
* @param   type           资源类型0:标卡 1:非标卡
* @param   pHashResInit   获取到的func/bulk信息
* @return
* @remark  无
* @see
* @author  cq      @date  2024/11/11
************************************************************/
ZXIC_UINT32 dpp_agent_hash_func_bulk_get(DPP_DEV_T *dev, ZXIC_UINT32 type, DPP_APT_HASH_RES_INIT_T *pHashResInit)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 opr = HASH_FUNC_BULK_REQ;
    ZXIC_UINT32 sub_type = RES_STD_NIC_MSG;
    ZXIC_UINT32 buff_size = 0;
    ZXIC_MUTEX_T *p_dtb_mutex = NULL;
    ZXIC_UINT32 *p_rsp_buff = NULL;
    SE_HASH_FUNC_BULK_T *p_func_bulk = NULL;
    DPP_DEV_MUTEX_TYPE_E mutex = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, pHashResInit);
    
    //使用代理通道要加锁
    mutex = DPP_DEV_MUTEX_T_DTB;
    rc = dpp_dev_opr_mutex_get(dev, (ZXIC_UINT32)mutex, &p_dtb_mutex);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_dtb_mutex);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "zxic_comm_mutex_lock");
    
    buff_size = dpp_get_se_buff_size(opr)+sizeof(ZXIC_UINT32);
    p_rsp_buff = (ZXIC_UINT32 *)ZXIC_COMM_MALLOC(buff_size);
    ZXIC_COMM_CHECK_DEV_POINT_UNLOCK(dev_id, p_rsp_buff, p_dtb_mutex);
    ZXIC_COMM_MEMSET_S(p_rsp_buff,buff_size,0x0,buff_size);

    sub_type = (type==SE_STD_NIC_RES_TYPE) ? RES_STD_NIC_MSG:RES_OFFLOAD_MSG;

    rc = dpp_agent_channel_se_res_get(dev, sub_type, opr, p_rsp_buff,buff_size);
    if( rc != DPP_OK)
    {
        ZXIC_COMM_FREE(p_rsp_buff);
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "hash func&bulk res get fail rc=0x%x. \n",rc);
        rc = zxic_comm_mutex_unlock(p_dtb_mutex);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_unlock");
        return DPP_ERR;
    }

    rc = zxic_comm_mutex_unlock(p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_unlock", p_rsp_buff);

    p_func_bulk = (SE_HASH_FUNC_BULK_T *)(p_rsp_buff + 1);
    rc = dpp_hash_func_bulk_set(pHashResInit,p_func_bulk);
    ZXIC_COMM_FREE(p_rsp_buff);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_hash_func_bulk_set");
    
    return DPP_OK;
}

/***********************************************************/
/** 消息通道获取hash表table信息
* @param   devId          NP设备号
* @param   type           资源类型0:标卡 1:非标卡
* @param   pHashResInit   获取到的table信息
* @return
* @remark  无
* @see
* @author  cq      @date  2024/11/11
************************************************************/
ZXIC_UINT32 dpp_agent_hash_tbl_get(DPP_DEV_T *dev, ZXIC_UINT32 type, DPP_APT_HASH_RES_INIT_T *pHashResInit)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 opr = HASH_TBL_REQ;
    ZXIC_UINT32 sub_type = RES_STD_NIC_MSG;
    ZXIC_UINT32 buff_size = 0;
    ZXIC_MUTEX_T *p_dtb_mutex = NULL;
    ZXIC_UINT32 *p_rsp_buff = NULL;
    SE_HASH_TBL_T *p_hash_tbl = NULL;
    DPP_DEV_MUTEX_TYPE_E mutex = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, pHashResInit);

    mutex = DPP_DEV_MUTEX_T_DTB;
    rc = dpp_dev_opr_mutex_get(dev, (ZXIC_UINT32)mutex, &p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_lock");

    buff_size = dpp_get_se_buff_size(opr)+sizeof(ZXIC_UINT32);
    p_rsp_buff = (ZXIC_UINT32 *)ZXIC_COMM_MALLOC(buff_size);
    ZXIC_COMM_CHECK_DEV_POINT_UNLOCK(dev_id, p_rsp_buff, p_dtb_mutex);

    sub_type = (type==SE_STD_NIC_RES_TYPE) ? RES_STD_NIC_MSG:RES_OFFLOAD_MSG;
    rc = dpp_agent_channel_se_res_get(dev, sub_type, opr, p_rsp_buff,buff_size);
    if( rc != DPP_OK)
    {
        ZXIC_COMM_FREE(p_rsp_buff);
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "hash table res get fail rc=0x%x. \n",rc);
        rc = zxic_comm_mutex_unlock(p_dtb_mutex);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_unlock");
        return DPP_ERR;
    }

    rc = zxic_comm_mutex_unlock(p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_unlock",p_rsp_buff);

    p_hash_tbl = (SE_HASH_TBL_T *)(p_rsp_buff + 1);
    rc = dpp_hash_tbl_set(pHashResInit,p_hash_tbl);
    ZXIC_COMM_FREE(p_rsp_buff);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_hash_tbl_set");

    return DPP_OK;
}

/***********************************************************/
/** 消息通道获取eram表table信息
* @param   devId          NP设备号
* @param   type           资源类型0:标卡 1:非标卡
* @param   pEramResInit   获取到的eram信息
* @return
* @remark  无
* @see
* @author  cq      @date  2024/11/11
************************************************************/
static ZXIC_UINT32 dpp_agent_eram_tbl_get(DPP_DEV_T *dev, ZXIC_UINT32 type, DPP_APT_ERAM_RES_INIT_T *pEramResInit)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 opr = ERAM_TBL_REQ;
    ZXIC_UINT32 sub_type = RES_STD_NIC_MSG;
    ZXIC_UINT32 buff_size = 0;
    ZXIC_MUTEX_T *p_dtb_mutex = NULL;
    ZXIC_UINT32 *p_rsp_buff = NULL;
    SE_ERAM_TBL_T *p_eram_tbl = NULL;
    DPP_DEV_MUTEX_TYPE_E mutex = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, pEramResInit);

    mutex = DPP_DEV_MUTEX_T_DTB;
    rc = dpp_dev_opr_mutex_get(dev, (ZXIC_UINT32)mutex, &p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_lock");

    buff_size = dpp_get_se_buff_size(opr)+sizeof(ZXIC_UINT32);
    p_rsp_buff = (ZXIC_UINT32 *)ZXIC_COMM_MALLOC(buff_size);
    ZXIC_COMM_CHECK_DEV_POINT_UNLOCK(dev_id, p_rsp_buff, p_dtb_mutex);

    sub_type = (type==SE_STD_NIC_RES_TYPE) ? RES_STD_NIC_MSG:RES_OFFLOAD_MSG;
    rc = dpp_agent_channel_se_res_get(dev, sub_type, opr, p_rsp_buff,buff_size);
    if( rc != DPP_OK)
    {
        ZXIC_COMM_FREE(p_rsp_buff);
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "eram table res get fail rc=0x%x. \n",rc);
        rc = zxic_comm_mutex_unlock(p_dtb_mutex);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_unlock");
        return DPP_ERR;
    }

    rc = zxic_comm_mutex_unlock(p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_unlock",p_rsp_buff);

    p_eram_tbl = (SE_ERAM_TBL_T *)(p_rsp_buff + 1);
    rc = dpp_eram_tbl_set(pEramResInit,p_eram_tbl);
    ZXIC_COMM_FREE(p_rsp_buff);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_eram_tbl_set");
    
    return DPP_OK;
}

/***********************************************************/
/** 消息通道获取acl表table信息
* @param   devId          NP设备号
* @param   type           资源类型0:标卡 1:非标卡
* @param   pAclResInit   获取到的acl信息
* @return
* @remark  无
* @see
* @author  cq      @date  2024/11/11
************************************************************/
static ZXIC_UINT32 dpp_agent_acl_tbl_get(DPP_DEV_T *dev, ZXIC_UINT32 type, DPP_APT_ACL_RES_INIT_T *pAclResInit)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 opr = ACL_TBL_REQ;
    ZXIC_UINT32 sub_type = RES_STD_NIC_MSG;
    ZXIC_UINT32 buff_size = 0;
    ZXIC_MUTEX_T *p_dtb_mutex = NULL;
    ZXIC_UINT32 *p_rsp_buff = NULL;
    SE_ACL_TBL_T *p_acl_tbl = NULL;
    DPP_DEV_MUTEX_TYPE_E mutex = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, pAclResInit);

    mutex = DPP_DEV_MUTEX_T_DTB;
    rc = dpp_dev_opr_mutex_get(dev, (ZXIC_UINT32)mutex, &p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_lock");

    buff_size = dpp_get_se_buff_size(opr)+sizeof(ZXIC_UINT32);
    p_rsp_buff = (ZXIC_UINT32 *)ZXIC_COMM_MALLOC(buff_size);
    ZXIC_COMM_CHECK_DEV_POINT_UNLOCK(dev_id, p_rsp_buff, p_dtb_mutex);

    sub_type = (type==SE_STD_NIC_RES_TYPE) ? RES_STD_NIC_MSG:RES_OFFLOAD_MSG;
    rc = dpp_agent_channel_se_res_get(dev, sub_type, opr, p_rsp_buff,buff_size);
    if( rc != DPP_OK)
    {
        ZXIC_COMM_FREE(p_rsp_buff);
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "acl table res get fail rc=0x%x. \n",rc);
        rc = zxic_comm_mutex_unlock(p_dtb_mutex);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_unlock");
        return DPP_ERR;
    }

    rc = zxic_comm_mutex_unlock(p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_unlock",p_rsp_buff);

    p_acl_tbl = (SE_ACL_TBL_T *)(p_rsp_buff + 1);
    rc = dpp_acl_tbl_set(pAclResInit, p_acl_tbl);
    ZXIC_COMM_FREE(p_rsp_buff);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_acl_tbl_set");

    return DPP_OK;
}

#if 0
/***********************************************************/
/** 消息通道获取lpm表table信息
* @param   devId          NP设备号
* @param   type           资源类型0:标卡 1:非标卡
* @param   pLpmResInit   获取到的lpm信息
* @return
* @remark  无
* @see
* @author  cq      @date  2024/11/11
************************************************************/
static ZXIC_UINT32 dpp_agent_lpm_tbl_get(DPP_DEV_T *dev, ZXIC_UINT32 type, DPP_APT_LPM_RES_INIT_T *pLpmResInit)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 opr = LPM_TBL_REQ;
    ZXIC_UINT32 sub_type = RES_STD_NIC_MSG;
    ZXIC_UINT32 buff_size = 0;
    ZXIC_MUTEX_T *p_dtb_mutex = NULL;
    ZXIC_UINT32 *p_rsp_buff = NULL;
    SE_LPM_TBL_T *p_lpm_tbl = NULL;
    DPP_DEV_MUTEX_TYPE_E mutex = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, pLpmResInit);

    mutex = DPP_DEV_MUTEX_T_DTB;
    rc = dpp_dev_opr_mutex_get(dev, (ZXIC_UINT32)mutex, &p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_lock");

    buff_size = dpp_get_se_buff_size(opr)+sizeof(ZXIC_UINT32);
    p_rsp_buff = (ZXIC_UINT32 *)ZXIC_COMM_MALLOC(buff_size);
    ZXIC_COMM_CHECK_DEV_POINT_UNLOCK(dev_id, p_rsp_buff, p_dtb_mutex);

    sub_type = (type==SE_STD_NIC_RES_TYPE) ? RES_STD_NIC_MSG:RES_OFFLOAD_MSG;
    rc = dpp_agent_channel_se_res_get(dev, sub_type, opr, p_rsp_buff,buff_size);
    if( rc != DPP_OK)
    {
        ZXIC_COMM_FREE(p_rsp_buff);
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "lpm table res get fail rc=0x%x. \n",rc);
        rc = zxic_comm_mutex_unlock(p_dtb_mutex);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_unlock");
        return DPP_ERR;
    }

    rc = zxic_comm_mutex_unlock(p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_unlock",p_rsp_buff);

    p_lpm_tbl = (SE_LPM_TBL_T *)(p_rsp_buff + 1);
    rc = dpp_lpm_tbl_set(pLpmResInit, p_lpm_tbl);
    ZXIC_COMM_FREE(p_rsp_buff);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_lpm_tbl_set");

    return DPP_OK;
}

/***********************************************************/
/** 消息通道获取ddr表table信息
* @param   devId          NP设备号
* @param   type           资源类型0:标卡 1:非标卡
* @param   pDdrResInit   获取到的ddr信息
* @return
* @remark  无
* @see
* @author  cq      @date  2024/11/11
************************************************************/
static ZXIC_UINT32 dpp_agent_ddr_tbl_get(DPP_DEV_T *dev, ZXIC_UINT32 type, DPP_APT_DDR_RES_INIT_T *pDdrResInit)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 opr = DDR_TBL_REQ;
    ZXIC_UINT32 sub_type = RES_STD_NIC_MSG;
    ZXIC_UINT32 buff_size = 0;
    ZXIC_MUTEX_T *p_dtb_mutex = NULL;
    ZXIC_UINT32 *p_rsp_buff = NULL;
    SE_DDR_TBL_T *p_ddr_tbl = NULL;
    DPP_DEV_MUTEX_TYPE_E mutex = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, pDdrResInit);

    mutex = DPP_DEV_MUTEX_T_DTB;
    rc = dpp_dev_opr_mutex_get(dev, (ZXIC_UINT32)mutex, &p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_lock");

    buff_size = dpp_get_se_buff_size(opr)+sizeof(ZXIC_UINT32);
    p_rsp_buff = (ZXIC_UINT32 *)ZXIC_COMM_MALLOC(buff_size);
    ZXIC_COMM_CHECK_DEV_POINT_UNLOCK(dev_id, p_rsp_buff, p_dtb_mutex);

    sub_type = (type==SE_STD_NIC_RES_TYPE) ? RES_STD_NIC_MSG:RES_OFFLOAD_MSG;
    rc = dpp_agent_channel_se_res_get(dev, sub_type, opr, p_rsp_buff,buff_size);
    if( rc != DPP_OK)
    {
        ZXIC_COMM_FREE(p_rsp_buff);
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ddr table res get fail rc=0x%x. \n",rc);
        rc = zxic_comm_mutex_unlock(p_dtb_mutex);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_unlock");
        return DPP_ERR;
    }

    rc = zxic_comm_mutex_unlock(p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_unlock",p_rsp_buff);

    p_ddr_tbl = (SE_DDR_TBL_T *)(p_rsp_buff + 1);
    rc = dpp_ddr_tbl_set(pDdrResInit, p_ddr_tbl);
    ZXIC_COMM_FREE(p_rsp_buff);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_ddr_tbl_set");

    return DPP_OK;
}
#endif

/***********************************************************/
/** 消息通道获取stat cfg信息
* @param   devId          NP设备号
* @param   type           资源类型0:标卡 1:非标卡
* @param   pStatCfgInit   获取到的统计信息
* @return
* @remark  无
* @see
* @author  cq      @date  2024/11/11
************************************************************/
static ZXIC_UINT32 dpp_agent_stat_cfg_get(DPP_DEV_T *dev, ZXIC_UINT32 type, DPP_APT_STAT_RES_INIT_T *pStatCfgInit)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 opr = STAT_CFG_REQ;
    ZXIC_UINT32 sub_type = RES_STD_NIC_MSG;
    ZXIC_UINT32 buff_size = 0;
    ZXIC_MUTEX_T *p_dtb_mutex = NULL;
    ZXIC_UINT32 *p_rsp_buff = NULL;
    SE_STAT_CFG_T *p_stat_cfg = NULL;
    DPP_DEV_MUTEX_TYPE_E mutex = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, pStatCfgInit);

    mutex = DPP_DEV_MUTEX_T_DTB;
    rc = dpp_dev_opr_mutex_get(dev, (ZXIC_UINT32)mutex, &p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_lock");

    buff_size = dpp_get_se_buff_size(opr)+sizeof(ZXIC_UINT32);
    p_rsp_buff = (ZXIC_UINT32 *)ZXIC_COMM_MALLOC(buff_size);
    ZXIC_COMM_CHECK_DEV_POINT_UNLOCK(dev_id, p_rsp_buff, p_dtb_mutex);

    sub_type = (type==SE_STD_NIC_RES_TYPE) ? RES_STD_NIC_MSG:RES_OFFLOAD_MSG;
    rc = dpp_agent_channel_se_res_get(dev, sub_type, opr, p_rsp_buff,buff_size);
    if( rc != DPP_OK)
    {
        ZXIC_COMM_FREE(p_rsp_buff);
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "stat res get fail rc=0x%x. \n",rc);
        rc = zxic_comm_mutex_unlock(p_dtb_mutex);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_unlock");
        return DPP_ERR;
    }

    rc = zxic_comm_mutex_unlock(p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_unlock",p_rsp_buff);

    p_stat_cfg = (SE_STAT_CFG_T *)(p_rsp_buff + 1);
    rc = dpp_stat_cfg_set(pStatCfgInit, p_stat_cfg);
    ZXIC_COMM_FREE(p_rsp_buff);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_stat_cfg_set");

    return DPP_OK;
}

/***********************************************************/
/** 分配流表资源缓存空间
* @param   dev            NP设备
* @param 
* @return
* @remark  无
* @see
* @author  cq      @date  2024/11/11
************************************************************/
DPP_STATUS dpp_se_res_mem_alloc(DPP_DEV_T *dev)
{
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT16 slot = 0;
    DPP_APT_SE_RES_T *p_se_res = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    slot = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, slot, 0, DPP_PCIE_SLOT_MAX - 1);

    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(dev);
    if(p_se_res == NULL)
    {
        p_se_res = (DPP_APT_SE_RES_T *)ZXIC_COMM_MALLOC(sizeof(DPP_APT_SE_RES_T));
        ZXIC_COMM_CHECK_DEV_POINT(dev_id,p_se_res);
        ZXIC_COMM_MEMSET_S(p_se_res,sizeof(DPP_APT_SE_RES_T),0x0,sizeof(DPP_APT_SE_RES_T));
        dpp_dev_set_se_res_ptr(dev,(ZXIC_VOID *)p_se_res);
    }
    
    return DPP_OK;
}

/***********************************************************/
/** 释放流表资源缓存空间
* @param   dev            NP设备
* @param 
* @return
* @remark  无
* @see
* @author  cq      @date  2024/11/11
************************************************************/
DPP_STATUS dpp_se_res_mem_free(DPP_DEV_T *dev)
{
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT16 slot = 0;
    DPP_APT_SE_RES_T *p_se_res = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    slot = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, slot, 0, DPP_PCIE_SLOT_MAX - 1);

    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(dev);
    if(p_se_res)
    {
        ZXIC_COMM_FREE(p_se_res);
        dpp_dev_set_se_res_ptr(dev,NULL);
    }
    
    return DPP_OK;
}

/***********************************************************/
/** 消息通道获取指定类型的所有流表资源
* @param   dev            NP设备
* @param 
* @return
* @remark  无
* @see
* @author  cq      @date  2024/11/11
************************************************************/
DPP_STATUS dpp_agent_se_res_get(DPP_DEV_T *dev)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 type = SE_STD_NIC_RES_TYPE;
    DPP_APT_SE_RES_T *p_se_res = NULL;
    DPP_APT_HASH_RES_INIT_T hash_res = {0};
    DPP_APT_ERAM_RES_INIT_T eram_res = {0};
    DPP_APT_ACL_RES_INIT_T acl_res = {0};
    //DPP_APT_LPM_RES_INIT_T lpm_res = {0};
    //DPP_APT_DDR_RES_INIT_T ddr_res = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);

    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(dev);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id,p_se_res);

    if(p_se_res->valid)
    {
        ZXIC_COMM_PRINT("slot[0x%x] res status ready\n",DEV_PCIE_SLOT(dev));
        return DPP_OK;
    }
    
    ZXIC_COMM_MEMSET_S(&hash_res,sizeof(DPP_APT_HASH_RES_INIT_T),0x0,sizeof(DPP_APT_HASH_RES_INIT_T));
    ZXIC_COMM_MEMSET_S(&eram_res,sizeof(DPP_APT_ERAM_RES_INIT_T),0x0,sizeof(DPP_APT_ERAM_RES_INIT_T));
    ZXIC_COMM_MEMSET_S(&acl_res,sizeof(DPP_APT_ACL_RES_INIT_T),0x0,sizeof(DPP_APT_ACL_RES_INIT_T));
    //ZXIC_COMM_MEMSET_S(&lpm_res,sizeof(DPP_APT_LPM_RES_INIT_T),0x0,sizeof(DPP_APT_LPM_RES_INIT_T));
    //ZXIC_COMM_MEMSET_S(&ddr_res,sizeof(DPP_APT_DDR_RES_INIT_T),0x0,sizeof(DPP_APT_DDR_RES_INIT_T));

    hash_res.func_res = p_se_res->hash_func; 
    hash_res.bulk_res = p_se_res->hash_bulk;
    hash_res.tbl_res = p_se_res->hash_tbl;
    rc = dpp_agent_hash_func_bulk_get(dev, type, &hash_res);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc,"dpp_agent_hash_func_bulk_get");
    rc = dpp_agent_hash_tbl_get(dev, type, &hash_res);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc,"dpp_agent_hash_tbl_get");
    p_se_res->hash_func_num = hash_res.func_num;
    p_se_res->hash_bulk_num = hash_res.bulk_num;
    p_se_res->hash_tbl_num = hash_res.tbl_num;

    eram_res.eram_res = p_se_res->eram_tbl;
    rc = dpp_agent_eram_tbl_get(dev, type, &eram_res);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc,"dpp_agent_eram_tbl_get");
    p_se_res->eram_num = eram_res.tbl_num;

    acl_res.acl_res = p_se_res->acl_tbl;
    rc = dpp_agent_acl_tbl_get(dev, type, &acl_res);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc,"dpp_agent_acl_tbl_get");
    p_se_res->acl_num = acl_res.tbl_num;

    #if 0
    lpm_res.glb_res = &p_se_res->lpm_global_res;
    lpm_res.lpm_res = p_se_res->lpm_tbl;
    rc = dpp_agent_lpm_tbl_get(dev, type, &lpm_res);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc,"dpp_agent_lpm_tbl_get");
    p_se_res->lpm_num = lpm_res.tbl_num;

    ddr_res.ddr_res = p_se_res->ddr_tbl;
    rc = dpp_agent_ddr_tbl_get(dev, type, &ddr_res);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc,"dpp_agent_ddr_tbl_get");
    p_se_res->ddr_num = ddr_res.tbl_num;
    #endif

    rc = dpp_agent_stat_cfg_get(dev, type, &(p_se_res->stat_cfg));
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc,"dpp_agent_stat_cfg_get");

    p_se_res->valid = 1;

    return DPP_OK;
}

/***********************************************************/
/** 初始化流表资源
* @param   dev            NP设备
* @param   
* @return
* @remark  无
* @see
* @author  cq      @date  2024/11/11
************************************************************/
DPP_STATUS dpp_se_res_init(DPP_DEV_T *dev)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    DPP_APT_SE_RES_T *p_se_res = NULL;
    DPP_APT_HASH_RES_INIT_T tHashResInit = {0};
    DPP_APT_ERAM_RES_INIT_T tEramResInit = {0};
    DPP_APT_ACL_RES_INIT_T tAclResInit = {0};
    //DPP_APT_DDR_RES_INIT_T tDdrResInit = {0};
    //DPP_APT_LPM_RES_INIT_T tLpmResInit = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);

    ZXIC_COMM_MEMSET(&tHashResInit,0x0,sizeof(DPP_APT_HASH_RES_INIT_T));
    ZXIC_COMM_MEMSET(&tEramResInit,0x0,sizeof(DPP_APT_ERAM_RES_INIT_T));
    ZXIC_COMM_MEMSET(&tAclResInit,0x0,sizeof(DPP_APT_ACL_RES_INIT_T));
    //ZXIC_COMM_MEMSET(&tDdrResInit,0x0,sizeof(DPP_APT_DDR_RES_INIT_T));
    //ZXIC_COMM_MEMSET(&tLpmResInit,0x0,sizeof(DPP_APT_LPM_RES_INIT_T));

    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(dev);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id,p_se_res);
    if(!p_se_res->valid)
    {
        ZXIC_COMM_TRACE_ERROR("dpp_se_res_init:res invlaid!\n");
        return DPP_ERR;
    }

    tHashResInit.func_num = p_se_res->hash_func_num;
    tHashResInit.bulk_num = p_se_res->hash_bulk_num;
    tHashResInit.tbl_num = p_se_res->hash_tbl_num;
    tHashResInit.func_res = p_se_res->hash_func;
    tHashResInit.bulk_res = p_se_res->hash_bulk;
    tHashResInit.tbl_res = p_se_res->hash_tbl;
    tEramResInit.tbl_num = p_se_res->eram_num;
    tEramResInit.eram_res = p_se_res->eram_tbl;
    tAclResInit.tbl_num = p_se_res->acl_num;
    tAclResInit.acl_res = p_se_res->acl_tbl;
    //tLpmResInit.tbl_num = p_se_res->lpm_num;
    //tLpmResInit.glb_res = &p_se_res->lpm_global_res;
    //tLpmResInit.lpm_res = p_se_res->lpm_tbl;
    //tDdrResInit.tbl_num = p_se_res->ddr_num;
    //tDdrResInit.ddr_res = p_se_res->ddr_tbl;

    // hash init
    rc = dpp_apt_hash_global_res_init(dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_apt_hash_global_res_init");
    
    if(tHashResInit.func_num)
    {
        rc = dpp_apt_hash_func_res_init(dev, tHashResInit.func_num, tHashResInit.func_res);
        ZXIC_COMM_CHECK_RC(rc, "dpp_apt_hash_func_res_init");
    }
    
    if(tHashResInit.bulk_num)
    {
        rc = dpp_apt_hash_bulk_res_init(dev, tHashResInit.bulk_num, tHashResInit.bulk_res);
        ZXIC_COMM_CHECK_RC(rc, "dpp_apt_hash_bulk_res_init");
    }

    // tbl-res must be initialized after fun-res and buld-res
    if(tHashResInit.tbl_num)
    {
        rc = dpp_apt_hash_tbl_res_init(dev, tHashResInit.tbl_num, tHashResInit.tbl_res);
        ZXIC_COMM_CHECK_RC(rc, "dpp_apt_hash_tbl_res_init");
    }

    // eram init
    if(tEramResInit.tbl_num)
    {
        rc = dpp_apt_eram_res_init(dev, tEramResInit.tbl_num, tEramResInit.eram_res);
        ZXIC_COMM_CHECK_RC(rc, "dpp_apt_eram_res_init");
    }

    // init acl
    if(tAclResInit.tbl_num)
    {
        rc = dpp_apt_acl_res_init(dev, tAclResInit.tbl_num, tAclResInit.acl_res);
        ZXIC_COMM_CHECK_RC(rc, "dpp_apt_acl_res_init");
    }

    // init stat
    rc = dpp_apt_stat_res_init(dev, &(p_se_res->stat_cfg));
    ZXIC_COMM_CHECK_RC(rc, "dpp_apt_stat_res_init");

    return DPP_OK;
}

/***********************************************************/
/** 消息通道获取指定类型流表资源&流表资源初始化
* @param   dev            NP设备
* @param
* @return
* @remark  无
* @see
* @author  cq      @date  2024/11/11
************************************************************/
DPP_STATUS dpp_se_res_get_and_init(DPP_DEV_T *dev)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);

    rc = dpp_agent_se_res_get(dev);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_agent_se_res_get");

    rc = dpp_se_res_init(dev);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_res_init");

    return DPP_OK;
}

/***********************************************************/
/** 获取sdt对应的hash最大设计条目数
* @param   dev            NP设备
* @param   sdt_no   
* @param   max_num        出参，用户可配置条目数上限
* @param
* @return
* @remark  无
* @see
* @author  cq      @date  2024/12/03
************************************************************/
DPP_STATUS dpp_hash_max_item_num_get(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no, ZXIC_UINT32 *max_num)
{
    DPP_STATUS rc        = DPP_OK;
    ZXIC_UINT32 func_id  = 0;
    ZXIC_UINT32 bulk_id  = 0;
    ZXIC_UINT32 index    = 0;
    DPP_SDTTBL_HASH_T sdt_hash_info = {0};
    DPP_APT_SE_RES_T *p_se_res = NULL;
    DPP_APT_HASH_BULK_RES_T *p_bulk       = NULL;
    DPP_APT_HASH_BULK_RES_T *p_temp_bulk  = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(max_num);

    ZXIC_COMM_MEMSET_S(&sdt_hash_info,sizeof(DPP_SDTTBL_HASH_T),0x0,sizeof(DPP_SDTTBL_HASH_T));
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_hash_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_soft_sdt_tbl_get");

    func_id = sdt_hash_info.hash_id;
    bulk_id = (sdt_hash_info.hash_table_id>>2)&0x7;

    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(dev);
    ZXIC_COMM_CHECK_POINT(p_se_res); 

    p_bulk = p_se_res->hash_bulk;
    for(index=0;index<HASH_BULK_MAX_NUM;index++)
    {
        p_temp_bulk = p_bulk + index;
        ZXIC_COMM_CHECK_POINT(p_temp_bulk);
        if((p_temp_bulk->func_id==func_id) && (p_temp_bulk->bulk_id==bulk_id))
        {
            *max_num =  p_temp_bulk->ddr_item_num;
            return DPP_OK;
        }
    }
    return DPP_ERR;
}

/***********************************************************/
/** 获取sdt对应的可dump到的最多hash条目数
* @param   dev            NP设备
* @param   sdt_no   
* @param   max_num        出参，用户可dump出的最多条目数
* @param
* @return
* @remark  无
* @see
* @author  cq      @date  2025/08/02
************************************************************/
DPP_STATUS dpp_hash_dump_max_item_num_get(DPP_DEV_T *dev, ZXIC_UINT32 sdt_no, ZXIC_UINT32 *max_num)
{
    DPP_STATUS rc              = DPP_OK;
    ZXIC_UINT32 func_id        = 0;
    ZXIC_UINT32 bulk_id        = 0;
    ZXIC_UINT32 index          = 0;
    ZXIC_UINT32 ram_entry_num  = 0;
    ZXIC_UINT32 zcell_num      = 0;
    ZXIC_UINT32 zreg_num       = 0;
    ZXIC_UINT32 entry_size     = 0;
    ZXIC_UINT32 item_width     = SE_ITEM_WIDTH_MAX;
    DPP_SDTTBL_HASH_T sdt_hash_info = {0};
    DPP_APT_SE_RES_T *p_se_res = NULL;
    DPP_APT_HASH_BULK_RES_T *p_bulk       = NULL;
    DPP_APT_HASH_BULK_RES_T *p_temp_bulk  = NULL;
    DPP_APT_HASH_FUNC_RES_T *p_hash_func  = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(max_num);

    ZXIC_COMM_MEMSET_S(&sdt_hash_info,sizeof(DPP_SDTTBL_HASH_T),0x0,sizeof(DPP_SDTTBL_HASH_T));
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_hash_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_soft_sdt_tbl_get");

    func_id = sdt_hash_info.hash_id;
    bulk_id = (sdt_hash_info.hash_table_id>>2)&0x7;
    entry_size = DPP_GET_HASH_ENTRY_SIZE(sdt_hash_info.hash_table_width);
    ZXIC_COMM_CHECK_INDEX_EQUAL(entry_size, 0);

    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(dev);
    ZXIC_COMM_CHECK_POINT(p_se_res); 

    p_bulk = p_se_res->hash_bulk;
    for(index=0;index<HASH_BULK_MAX_NUM;index++)
    {
        p_temp_bulk = p_bulk + index;
        ZXIC_COMM_CHECK_POINT(p_temp_bulk);
        if((p_temp_bulk->func_id==func_id) && (p_temp_bulk->bulk_id==bulk_id))
        {
            zcell_num = p_temp_bulk->zcell_num;
            zreg_num = p_temp_bulk->zreg_num;
            ram_entry_num = item_width/entry_size;
            if((zcell_num>0) && (zreg_num>0))
            {
                *max_num = (zcell_num * SE_RAM_DEPTH + zreg_num) * ram_entry_num; 
                ZXIC_COMM_TRACE_NOTICE("dpp_hash_dump_max_item_num_get(mono) max_num=%u\n",*max_num);
                return DPP_OK;
            }
        }
    }

    for(index=0;index<HASH_FUNC_MAX_NUM;index++)
    {
        p_hash_func = p_se_res->hash_func + index;
        ZXIC_COMM_CHECK_POINT(p_hash_func);
        if(p_hash_func->func_id == func_id)
        {
            *max_num = (p_hash_func->zblk_num) * (SE_ZCELL_NUM * SE_RAM_DEPTH + SE_ZREG_NUM) * ITEM_ENTRY_NUM_4;
            ZXIC_COMM_TRACE_NOTICE("dpp_hash_dump_max_item_num_get max_num=%u\n",*max_num);
            return DPP_OK;
        }
    }
            
    
    return DPP_ERR;
}

/***********************************************************/
/** 解析统计项的信息
* @param   dev              NP设备
* @param   sdt_no           统计属性对应的sdt号
* @param   entry_num        dump出的条目数
* @param   p_dump_data_arr  dump出的数据
* @param   p_stat_item_num  出参，统计项个数
* @param   p_stat_item      出参，解析后的统计项信息
* @param
* @return
* @remark  无
* @see
* @author  cq      @date  2025/02/15
************************************************************/
static DPP_STATUS dpp_stat_tbl_parse(DPP_DEV_T *dev,
                            ZXIC_UINT32 sdt_no,
                            ZXIC_UINT32 entry_num,
                            DPP_DTB_ERAM_ENTRY_INFO_T *p_dump_data_arr,
                            ZXIC_UINT32 *p_stat_item_num,
                            DPP_APT_STAT_ITEM_T *p_stat_item)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 valid_item_num = 0;
    ZXIC_UINT32 *p_data = NULL;
    SE_APT_CALLBACK_T *pAptCallback = NULL;
    DPP_APT_STAT_ITEM_T *p_temp_stat_item = NULL;
    ZXDH_STAT_ATTR_T stat_attr = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_dump_data_arr);
    ZXIC_COMM_CHECK_POINT(p_stat_item_num);
    ZXIC_COMM_CHECK_POINT(p_stat_item);

    pAptCallback = dpp_apt_get_func(dev, sdt_no);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pAptCallback);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pAptCallback->se_func_info.eramFunc.eram_get_func);

    for(i=0;(i<entry_num)&&(i<STAT_ITEM_MAX_NUM);i++)
    {
        p_data = p_dump_data_arr[i].p_data;
        ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);
        
        rc = pAptCallback->se_func_info.eramFunc.eram_get_func((ZXIC_VOID *)&stat_attr, p_data);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "eram_get_func");

        if(stat_attr.valid)
        {
            valid_item_num++;
            p_temp_stat_item = p_stat_item + i;
            p_temp_stat_item->valid = 1;
            p_temp_stat_item->mode = stat_attr.mode;
            p_temp_stat_item->addr_offset = stat_attr.addr_offset;
            p_temp_stat_item->depth = stat_attr.depth;
        }
    }

    *p_stat_item_num = valid_item_num;

    return DPP_OK;
}

/***********************************************************/
/** 获取统计项的信息
* @param   dev               NP设备
* @param   p_se_res          流表资源
* @param
* @return
* @remark  无
* @see
* @author  cq      @date  2025/02/15
************************************************************/
DPP_STATUS dpp_stat_tbl_get(DPP_DEV_T *dev,DPP_APT_SE_RES_T *p_se_res)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 queue_id = 0;
    ZXIC_UINT32 stat_sdt_no = ZXDH_SDT_STAT_ATTR_TABLE;
    ZXIC_UINT32 exist_flag = 0;
    ZXIC_UINT32 eram_table_depth = 0;
    ZXIC_UINT32 byte_num = 0;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 entry_num = 0;
    DPP_SDTTBL_ERAM_T sdt_eram = {0};  /*SDT内容*/
    DPP_DTB_ERAM_ENTRY_INFO_T *p_dump_data_arr = NULL;
    ZXIC_UINT8 *data_buff = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id,p_se_res);
    
    rc = dpp_apt_sdt_is_exist(p_se_res,DPP_SDT_TBLT_eRAM,stat_sdt_no,&exist_flag);
    ZXIC_COMM_CHECK_RC(rc, "dpp_apt_sdt_is_exist");
    if(!exist_flag)
    {
        ZXIC_COMM_PRINT("sdt_no:%d is not exsit, can not get stat item info.\n", stat_sdt_no);
        return DPP_OK;
    }

    rc = dpp_soft_sdt_tbl_get(dev, stat_sdt_no, &sdt_eram);
    ZXIC_COMM_CHECK_RC(rc, "dpp_soft_sdt_tbl_get");

    rc = dpp_dtb_queue_id_get(dev, &queue_id);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_queue_id_get");
    
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sdt_eram.eram_mode, ERAM128_TBL_64b, ERAM128_TBL_128b);
    byte_num = (sdt_eram.eram_mode == ERAM128_TBL_64b) ? 8 : 16;
    eram_table_depth = sdt_eram.eram_table_depth; 
    p_dump_data_arr = (DPP_DTB_ERAM_ENTRY_INFO_T *)ZXIC_COMM_MALLOC(eram_table_depth * sizeof(DPP_DTB_ERAM_ENTRY_INFO_T));
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dump_data_arr);
    ZXIC_COMM_MEMSET_S(p_dump_data_arr, eram_table_depth * sizeof(DPP_DTB_ERAM_ENTRY_INFO_T), 0, eram_table_depth * sizeof(DPP_DTB_ERAM_ENTRY_INFO_T));

    data_buff = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(byte_num*eram_table_depth);
    ZXIC_COMM_CHECK_DEV_POINT_MEMORY_FREE(dev_id, data_buff, p_dump_data_arr);
    ZXIC_COMM_MEMSET_S(data_buff, eram_table_depth * byte_num, 0, eram_table_depth * byte_num);

    for(i = 0; i < eram_table_depth; i++)
    {
        p_dump_data_arr[i].index = i;
        p_dump_data_arr[i].p_data = (ZXIC_UINT32 *)(data_buff+i*byte_num);
    }

    rc = dpp_dtb_eram_dump(dev, queue_id, stat_sdt_no, (ZXIC_UINT8 *)p_dump_data_arr, &entry_num);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE2PTR_NO_ASSERT(dev_id, rc, "dpp_dtb_eram_dump", data_buff, p_dump_data_arr);

    /*解析dump出来的eram数据*/
    rc = dpp_stat_tbl_parse(dev, stat_sdt_no, entry_num,p_dump_data_arr, &p_se_res->stat_item_num, p_se_res->stat_item);
    ZXIC_COMM_FREE(data_buff);
    ZXIC_COMM_FREE(p_dump_data_arr);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_tbl_parse");

    return DPP_OK; 
}

/***********************************************************/
/** 查看当前sdt号是否存在
* @param   p_se_res  表资源
* @param   sdt_type  表类型
* @param   sdt_no    sdt号
* @return  
* @remark  无
* @see     
* @author  cq      @date  2025/02/17
************************************************************/
DPP_STATUS dpp_apt_sdt_is_exist(DPP_APT_SE_RES_T *p_se_res, DPP_SDT_TABLE_TYPE_E sdt_type, ZXIC_UINT32 sdt_no,ZXIC_UINT32 *p_is_exist)
{
    ZXIC_UINT8 index = 0;

    ZXIC_COMM_CHECK_POINT(p_se_res);

    *p_is_exist = 0;
    if(sdt_type==DPP_SDT_TBLT_eRAM)
    {
        for(index=0;(index<p_se_res->eram_num)&&(index<ERAM_MAX_NUM);index++)
        {
            if(p_se_res->eram_tbl[index].sdtNo==sdt_no)
            {
                *p_is_exist = 1;
                break;
            }
        }
    }
    else if(sdt_type==DPP_SDT_TBLT_eTCAM)
    {
        for(index=0;(index<p_se_res->acl_num)&&(index<ETCAM_MAX_NUM);index++)
        {
            if(p_se_res->acl_tbl[index].sdtNo==sdt_no)
            {
                *p_is_exist = 1;
                break;
            }
        }
    }
    else if(sdt_type==DPP_SDT_TBLT_HASH)
    {
        for(index=0;(index<p_se_res->hash_tbl_num)&&(index<HASH_TABLE_MAX_NUM);index++)
        {
            if(p_se_res->hash_tbl[index].sdtNo==sdt_no)
            {
                *p_is_exist = 1;
                break;
            }
        }
    }

    return DPP_OK;
}

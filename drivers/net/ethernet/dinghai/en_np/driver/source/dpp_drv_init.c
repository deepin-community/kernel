#include "dpp_apt_se_api.h"
#include "dpp_stat_api.h"
#include "dpp_drv_init.h"
#include "dpp_drv_acl.h"
#include "dpp_drv_hash.h"
#include "dpp_drv_eram.h"
#include "dpp_hash.h"
#include "dpp_apt_se.h"
#include "dpp_tbl_pkt_cap.h"
#include "dpp_dtb_table_api.h"
#include "dpp_tbl_api.h"
extern DPP_DEV_MGR_T *dpp_dev_mgr_get(ZXIC_VOID);

#define DPP_FLOW_INIT_START       ((ZXIC_UINT32)(0))
#define DPP_FLOW_INIT_SUCCESS     ((ZXIC_UINT32)(1))

#define DPP_FLOW_INIT_STATUS_CHECK(dev)\
    do{\
        if((DEV_PCIE_SLOT(dev)<DPP_PCIE_SLOT_MAX)&&(DPP_FLOW_INIT_SUCCESS != dpp_flow_init_status[DEV_PCIE_SLOT(dev)]))\
        {\
           return DPP_OK;\
        }\
    }while(0)

#define DPP_FLOW_INIT_SUCCESS_STATUS_CHECK(dev)\
    do{\
        if((DEV_PCIE_SLOT(dev)<DPP_PCIE_SLOT_MAX)&&(DPP_FLOW_INIT_SUCCESS == dpp_flow_init_status[DEV_PCIE_SLOT(dev)]))\
        {\
           return DPP_OK;\
        }\
    }while(0)

ZXIC_UINT32 dpp_flow_init_status[DPP_PCIE_SLOT_MAX] = {DPP_FLOW_INIT_START};

ZXIC_VOID dpp_flow_init_status_init(ZXIC_VOID)
{
    ZXIC_COMM_MEMSET_S(dpp_flow_init_status,sizeof(dpp_flow_init_status),DPP_FLOW_INIT_START,sizeof(dpp_flow_init_status));
}

DPP_STATUS dpp_flow_init_status_set(DPP_DEV_T *dev, ZXIC_UINT32 status)
{
    ZXIC_UINT32 slot = 0;
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(status, DPP_FLOW_INIT_START, DPP_FLOW_INIT_SUCCESS);
    slot = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_INDEX(slot, 0, (DPP_PCIE_SLOT_MAX-1));

    dpp_flow_init_status[slot] = status;

    return DPP_OK; 
}

static DPP_STATUS dpp_drv_se_func_set(DPP_APT_SE_RES_T *p_se_res)
{
    ZXIC_UINT32 index = 0;
    SE_APT_ERAM_CONVERT_T *pAptEramCov = NULL;
    SE_APT_ACL_CONVERT_T *pAptAclCov = NULL;
    SE_APT_HASH_CONVERT_T *pAptHashCov = NULL;
    DPP_APT_ERAM_TABLE_T *pTempEramTbl = NULL;
    DPP_APT_ACL_TABLE_T *pTempAclTbl = NULL;
    DPP_APT_HASH_TABLE_T *pTempHashTbl = NULL;

    ZXIC_COMM_CHECK_POINT(p_se_res);

    for(index=0;index<(p_se_res->eram_num);index++)
    {
        pTempEramTbl = &(p_se_res->eram_tbl[index]);
        pAptEramCov = se_eram_callback_get(pTempEramTbl->sdtNo);
        if(pAptEramCov)
        {
            pTempEramTbl->eram_set_func = pAptEramCov->eram_set_func;
            pTempEramTbl->eram_get_func = pAptEramCov->eram_get_func;
        }
    }
    for(index=0;index<(p_se_res->acl_num);index++)
    {
        pTempAclTbl = &(p_se_res->acl_tbl[index]);
        pAptAclCov = se_acl_callback_get(pTempAclTbl->sdtNo);
        if(pAptAclCov)
        {
            pTempAclTbl->acl_set_func = pAptAclCov->acl_set_func;
            pTempAclTbl->acl_get_func = pAptAclCov->acl_get_func;
        }
    }
    for(index=0;index<(p_se_res->hash_tbl_num);index++)
    {
        pTempHashTbl = &(p_se_res->hash_tbl[index]);
        pAptHashCov = se_hash_callback_get(pTempHashTbl->sdtNo);
        if(pAptHashCov)
        {
            pTempHashTbl->hash_set_func = pAptHashCov->hash_set_func;
            pTempHashTbl->hash_get_func = pAptHashCov->hash_get_func;
        }
    }
    return DPP_OK;
}

DPP_STATUS dpp_bar_msg_num_init(DPP_DEV_T *dev)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 bar_msg_num = 0xFFFFFFFF;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT16 slot = 0;
    DPP_DEV_CFG_T *p_dev_info = NULL;
    DPP_DEV_MGR_T *p_dev_mgr  = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id, DPP_DEV_CHANNEL_MAX - 1);
    DPP_FLOW_INIT_SUCCESS_STATUS_CHECK(dev);

    slot = dev->pcie_channel.slot;
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, slot, 0, DPP_PCIE_SLOT_MAX - 1);

    p_dev_mgr = dpp_dev_mgr_get();
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_mgr);
    if (!p_dev_mgr->is_init)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ErrorCode[ 0x%x]: Device Manager is not init!!!\n",
                                                                     DPP_RC_DEV_MGR_NOT_INIT);
        return DPP_RC_DEV_MGR_NOT_INIT;
    }
    
    p_dev_info = p_dev_mgr->p_dev_array[dev_id];
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dev_info);

    rc = dpp_pcie_bar_msg_num_get(dev,&bar_msg_num);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_pcie_bar_msg_num_get");

    p_dev_info->bar_msg_num[slot] = bar_msg_num;
    dev->pcie_channel.bar_msg_num = bar_msg_num;
    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x bar_msg_num: %u.\n", slot, dev->pcie_channel.vport,bar_msg_num);
    return DPP_OK;
}

DPP_STATUS dpp_flow_init(DPP_DEV_T *dev)
{
    DPP_STATUS rc = DPP_OK;
    DPP_PF_INFO_T pf_info = {0};
    DPP_APT_SE_RES_T *p_se_res = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_PCIE_SLOT(dev), 0, DPP_PCIE_SLOT_MAX - 1);
    DPP_FLOW_INIT_SUCCESS_STATUS_CHECK(dev);

    pf_info.slot = dev->pcie_channel.slot;
    pf_info.vport = dev->pcie_channel.vport;

    ZXIC_COMM_TRACE_NOTICE("slot:%d start.\n", DEV_PCIE_SLOT(dev));

    rc = dpp_se_res_mem_alloc(dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_se_res_mem_alloc");

    rc = dpp_agent_se_res_get(dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_agent_se_res_get");

    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(dev);
    ZXIC_COMM_CHECK_POINT(p_se_res);

    rc = dpp_drv_se_func_set(p_se_res);
    ZXIC_COMM_CHECK_RC(rc, "dpp_drv_se_func_set");

    // hash init
    rc = dpp_apt_hash_global_res_init(dev);
    ZXIC_COMM_CHECK_RC(rc, "dpp_apt_hash_global_res_init");

    rc = dpp_apt_hash_func_res_init(dev, p_se_res->hash_func_num, p_se_res->hash_func);
    ZXIC_COMM_CHECK_RC(rc, "dpp_apt_hash_func_res_init");

    rc = dpp_apt_hash_bulk_res_init(dev, p_se_res->hash_bulk_num, p_se_res->hash_bulk);
    ZXIC_COMM_CHECK_RC(rc, "dpp_apt_hash_bulk_res_init");

    // tbl-res must be initialized after fun-res and buld-res
    rc = dpp_apt_hash_tbl_res_init(dev, p_se_res->hash_tbl_num, p_se_res->hash_tbl);
    ZXIC_COMM_CHECK_RC(rc, "dpp_apt_hash_tbl_res_init");

    // eram init
    rc = dpp_apt_eram_res_init(dev, p_se_res->eram_num, p_se_res->eram_tbl);
    ZXIC_COMM_CHECK_RC(rc, "dpp_apt_eram_res_init");

    // init acl
    rc = dpp_apt_acl_res_init(dev, p_se_res->acl_num, p_se_res->acl_tbl);
    ZXIC_COMM_CHECK_RC(rc, "dpp_apt_acl_res_init");

#ifdef DPP_FLOW_HW_INIT
    rc = dpp_stat_ppu_eram_baddr_set(dev, p_se_res->stat_cfg.eram_baddr);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_ppu_eram_baddr_set");

    rc = dpp_stat_ppu_eram_depth_set(dev, p_se_res->stat_cfg.eram_depth); //表项深度以128bit为单位
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_ppu_eram_depth_set");
#endif

    rc = dpp_pkt_capture_init(&pf_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_pkt_capture_init");

    rc = dpp_stat_tbl_get(dev,p_se_res);
    ZXIC_COMM_CHECK_RC(rc, "dpp_stat_tbl_get");

    rc = dpp_flow_init_status_set(dev,DPP_FLOW_INIT_SUCCESS);
    ZXIC_COMM_CHECK_RC(rc, "dpp_flow_init_status_set");

    ZXIC_COMM_TRACE_NOTICE("slot: %u success.\n", DEV_PCIE_SLOT(dev));

    return DPP_OK;
}

DPP_STATUS dpp_flow_uninit(DPP_DEV_T *dev)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 slot = 0;
    ZXIC_UINT32 last_flag = 0;
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = dev->pcie_channel.slot;
    pf_info.vport = dev->pcie_channel.vport;

    rc = dpp_dev_last_check(dev,&last_flag);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dev_last_check");

    slot = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_INDEX(slot, 0, (DPP_PCIE_SLOT_MAX-1));

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x last_flag: %u start.\n", slot,DEV_PCIE_VPORT(dev),last_flag);

    if(last_flag)
    {
        rc = dpp_hash_soft_uninstall(dev);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_hash_soft_uninstall");

        rc = dpp_apt_hash_global_res_uninit(dev);
        ZXIC_COMM_CHECK_RC(rc, "dpp_apt_hash_global_res_uninit");

        rc = dpp_apt_acl_soft_res_uninit(dev);
        ZXIC_COMM_CHECK_RC(rc, "dpp_apt_acl_global_res_uninit");

        rc = dpp_se_res_mem_free(dev);
        ZXIC_COMM_CHECK_RC(rc, "dpp_se_res_mem_free");

        dpp_flow_init_status[slot] = DPP_FLOW_INIT_START;
    }
    else
    {
        rc = dpp_unicast_all_mac_soft_delete(&pf_info);
        ZXIC_COMM_CHECK_RC_NONE(rc, "dpp_unicast_all_mac_soft_delete");

        rc = dpp_multicast_all_mac_soft_delete(&pf_info);
        ZXIC_COMM_CHECK_RC_NONE(rc, "dpp_multicast_all_mac_soft_delete"); 
    } 
    
    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x last_flag: %u success.\n", slot, DEV_PCIE_VPORT(dev), last_flag);

    return DPP_OK;
}

DPP_STATUS dpp_flow_data_all_flush(DPP_DEV_T *dev, ZXIC_UINT32 queue_id)
{
    DPP_STATUS rc = DPP_OK;
    DPP_APT_SE_RES_T *p_se_res = NULL;

    p_se_res = (DPP_APT_SE_RES_T *)dpp_dev_get_se_res_ptr(dev);
    ZXIC_COMM_CHECK_POINT(p_se_res);
    rc = dpp_apt_hash_func_flush_hardware_all(dev, p_se_res->hash_func_num, p_se_res->hash_func, queue_id);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_apt_hash_func_flush_hardware");

    return rc;
}


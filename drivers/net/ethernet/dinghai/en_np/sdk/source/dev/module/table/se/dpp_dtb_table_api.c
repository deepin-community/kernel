#include "zxic_common.h"
#include "dpp_dev.h"
#include "dpp_dtb_table_api.h"
#include "dpp_dtb_cfg.h"
#include "dpp_apt_se.h"
#include "zxic_comm_rb_tree.h"
#include "dpp_dtb_table.h"
#include "dpp_sdt.h"
#include "dpp_agent_channel.h"
#include "dpp_dtb.h"
#include "dpp_kernel_init.h"

#define EP_NUM (5)
static ZXIC_UINT32 msix_interrupt_mode = 1;//1为dbi中断，0为match中断
static ZXIC_RB_CFG g_dtb_queue_dump_addr_rb[DPP_DEV_SLOT_MAX][DPP_DEV_CHANNEL_MAX][DPP_DTB_QUEUE_NUM_MAX] = {{{{0}}}};
static ZXIC_UINT32 hardware_ep_id[EP_NUM] = {5, 6, 7, 8, 9};

typedef struct dpp_dtb_dump_addr_info_t
{
    ZXIC_UINT32 sdt_no;
    ZXIC_UINT64 phyAddr;
    ZXIC_UINT64 virAddr; 
    ZXIC_UINT32 size;
}DPP_DTB_DUMP_ADDR_INFO_T;

ZXIC_UINT32 dpp_dtb_ep_id_get(ZXIC_UINT32 soft_ep_id)
{
    return hardware_ep_id[soft_ep_id];
}

ZXIC_RB_CFG* dpp_dtb_dump_addr_rb_get(DPP_DEV_T *dev, ZXIC_UINT32 queue_id)
{
    ZXIC_COMM_CHECK_POINT_RETURN_NULL(dev);
    ZXIC_COMM_CHECK_INDEX_RETURN_NULL(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_RETURN_NULL(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    return &g_dtb_queue_dump_addr_rb[DEV_PCIE_SLOT(dev)][DEV_ID(dev)][queue_id];
}

ZXIC_UINT32 dpp_dtb_dump_addr_rb_init(DPP_DEV_T *dev, ZXIC_UINT32 queue_id)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_RB_CFG *p_dtb_dump_addr_rb = NULL;

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);

    p_dtb_dump_addr_rb = dpp_dtb_dump_addr_rb_get(dev, queue_id);

    rc = zxic_comm_rb_init(p_dtb_dump_addr_rb, 0, ZXIC_SIZEOF(DPP_DTB_DUMP_ADDR_INFO_T), dpp_apt_table_key_cmp);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "zxic_comm_rb_init");

    return rc;
}

ZXIC_UINT32 dpp_dtb_dump_addr_rb_destroy(DPP_DEV_T *dev, ZXIC_UINT32 queue_id)
{
    ZXIC_UINT32 rc = DPP_OK;

    D_NODE *p_node = NULL;
    ZXIC_RB_TN *p_rb_tn = NULL;
    DPP_DTB_DUMP_ADDR_INFO_T *p_rbkey = NULL;
    D_HEAD *p_head_dtb_rb = NULL;
    ZXIC_RB_CFG *p_dtb_dump_addr_rb = NULL;

    ZXIC_UINT32 sdt_no = 0;

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);

    p_dtb_dump_addr_rb = dpp_dtb_dump_addr_rb_get(dev, queue_id);
    ZXIC_COMM_CHECK_POINT_NO_ASSERT(p_dtb_dump_addr_rb);

    p_head_dtb_rb = &p_dtb_dump_addr_rb->tn_list;
    ZXIC_COMM_CHECK_POINT_NO_ASSERT(p_head_dtb_rb);

    while(p_head_dtb_rb->used)
    {
        p_node = p_head_dtb_rb->p_next;
        p_rb_tn = (ZXIC_RB_TN *)p_node->data;
        p_rbkey = (DPP_DTB_DUMP_ADDR_INFO_T *)p_rb_tn->p_key;

        sdt_no = p_rbkey->sdt_no;
        rc = dpp_dtb_dump_sdt_addr_clear(dev, queue_id, sdt_no);
        if (DPP_HASH_RC_DEL_SRHFAIL == rc)
        {
            ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "dtb dump delete key is not exist, std:%d\n", sdt_no);
        }
        else
        {
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_dump_sdt_addr_clear");
        }
    }

    rc  =  zxic_comm_rb_destroy(p_dtb_dump_addr_rb);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "zxic_comm_rb_init");

    return rc;
}

/*dump地址信息获取*/
ZXIC_UINT32 dpp_dtb_dump_sdt_addr_get(DPP_DEV_T *dev,
                                      ZXIC_UINT32 queue_id,  
                                      ZXIC_UINT32 sdt_no, 
                                      ZXIC_UINT64 *phy_addr, 
                                      ZXIC_UINT64 *vir_addr, 
                                      ZXIC_UINT32 *size)
{
    ZXIC_UINT32 rc = DPP_OK;

    DPP_DTB_DUMP_ADDR_INFO_T dtb_dump_addr_info = {0};
    ZXIC_RB_CFG *p_dtb_dump_addr_rb = NULL;

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);

    dtb_dump_addr_info.sdt_no = sdt_no;
    p_dtb_dump_addr_rb = dpp_dtb_dump_addr_rb_get(dev, queue_id);
    rc = dpp_apt_sw_list_search(p_dtb_dump_addr_rb, &dtb_dump_addr_info, sizeof(DPP_DTB_DUMP_ADDR_INFO_T));
    if (rc == DPP_OK)
    {
        ZXIC_COMM_TRACE_INFO("search sdt_no %d success.\n", sdt_no);
    }
    else
    {
        ZXIC_COMM_TRACE_ERROR("search sdt_no %d fail.\n", sdt_no);
        return rc;
    }

    *phy_addr = dtb_dump_addr_info.phyAddr;
    *vir_addr = dtb_dump_addr_info.virAddr;
    *size = dtb_dump_addr_info.size;

    ZXIC_COMM_TRACE_INFO("queue    :%d\n", queue_id); 
    ZXIC_COMM_TRACE_INFO("sdt_no   :%d\n", sdt_no); 
    ZXIC_COMM_TRACE_INFO("phyAddr  :0x%016llx\n", dtb_dump_addr_info.phyAddr); 
    ZXIC_COMM_TRACE_INFO("vir_addr :0x%016llx\n", dtb_dump_addr_info.virAddr); 
    ZXIC_COMM_TRACE_INFO("size     :0x%x\n", dtb_dump_addr_info.size); 

    return rc;
}

/***********************************************************/
/** DTB通道申请
* @param   devId       NP设备号
* @param   pName       申请DTB通道的唯一设备名(最大32字符)
* @param   pQueueId    申请到的DTB通道编号
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/07/03
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_requst(DPP_DEV_T *dev, ZXIC_CONST ZXIC_UINT8* pName, 
                                 ZXIC_UINT16 vPort, 
                                 ZXIC_UINT32 *pQueueId)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 queue_id = 0;
    ZXIC_MUTEX_T *p_dtb_mutex = NULL;
    DPP_DEV_MUTEX_TYPE_E mutex = 0;
    ZXIC_UINT32 vport_info = (ZXIC_UINT32)vPort;

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pName);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pQueueId);

    //使用代理通道要加锁
    mutex = DPP_DEV_MUTEX_T_DTB;
    rc = dpp_dev_opr_mutex_get(dev, (ZXIC_UINT32)mutex, &p_dtb_mutex);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_dtb_mutex);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "zxic_comm_mutex_lock");

    rc = dpp_agent_channel_dtb_queue_request(dev, pName, vport_info, &queue_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_agent_channel_dtb_queue_request", p_dtb_mutex);

    rc = zxic_comm_mutex_unlock(p_dtb_mutex);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "zxic_comm_mutex_unlock");

    rc = dpp_dtb_dump_addr_rb_init(dev, queue_id);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_dump_addr_rb_init");

    *pQueueId = queue_id;

    ZXIC_COMM_PRINT("dpp_dtb_queue_requst:slot %d name %s vport 0x%x queue_id %d\n", DEV_PCIE_SLOT(dev), pName, vport_info, queue_id);

    return rc;
}

/***********************************************************/
/** DTB通道队列申请(申请到的队列在软件维护和硬件维护上都处于初态)
* @param   devId       NP设备号
* @param   pName       申请DTB通道的唯一设备名(最大32字符)
* @param   pQueueId    申请到的DTB通道编号
* @return
* @remark  无
* @see
* @author  cq      @date  2025/06/05
************************************************************/
DPP_STATUS dpp_dtb_queue_requst_ex(DPP_DEV_T *dev,ZXIC_CONST ZXIC_UINT8* pName, ZXIC_UINT32 *p_queue_id)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 dev_id                   = 0;
    ZXIC_UINT32 init_flag                = 0;
    ZXIC_UINT32 queue_id                 = 0;
    ZXIC_UINT32 count                    = 0;
    ZXIC_UINT32 vport                    = 0;
    ZXIC_MUTEX_T *p_dtb_mutex            = NULL;
    DPP_DEV_MUTEX_TYPE_E mutex           = DPP_DEV_MUTEX_T_DTB;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(pName);
    ZXIC_COMM_CHECK_POINT(p_queue_id);

    vport = DEV_PCIE_VPORT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);

    //使用代理通道要加锁
    rc = dpp_dev_opr_mutex_get(dev, (ZXIC_UINT32)mutex, &p_dtb_mutex);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_dtb_mutex);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "zxic_comm_mutex_lock");

    do
    {
        rc = dpp_agent_channel_dtb_queue_request(dev, pName, vport, &queue_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(dev_id, rc, "dpp_agent_channel_dtb_queue_request", p_dtb_mutex);

        /*从固件申请到的队列，在软件上已占用，则释放当前队列，重新申请*/
        rc = dpp_dtb_queue_init_flag_get(dev, queue_id, &init_flag);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(dev_id, rc, "dpp_dtb_queue_init_flag_get", p_dtb_mutex);

        if(init_flag)
        {
            rc = dpp_agent_channel_dtb_queue_release(dev, pName, queue_id);
            ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(dev_id, rc, "dpp_agent_channel_dtb_queue_release", p_dtb_mutex);

            ZXIC_COMM_TRACE_NOTICE("slot[%u] vport[0x%x] count[%u] release queue[%u] succ!\n",DEV_PCIE_SLOT(dev),vport,count,queue_id);
        }
        else
        {
            ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x count: %u request queue: %u success!\n",DEV_PCIE_SLOT(dev),vport,count, queue_id);
            break;
        }
        count++;
    }while((init_flag==1) && (count<DPP_DTB_QUEUE_NUM_MAX));

    rc = zxic_comm_mutex_unlock(p_dtb_mutex);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "zxic_comm_mutex_unlock");

    if(init_flag)
    {
        ZXIC_COMM_TRACE_ERROR("slot[%u] vport[0x%x] request queue fail!\n",DEV_PCIE_SLOT(dev),vport);
        return DPP_ERR;
    }

    rc = dpp_dtb_dump_addr_rb_init(dev, queue_id);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_dump_addr_rb_init");

    *p_queue_id = queue_id;

    return DPP_OK;
}

/***********************************************************/
/** DTB通道释放
* @param   devId       NP设备号
* @param   pName       要释放DTB通道的唯一设备名(最大32字符)
* @param   pQueueId    要释放的DTB通道编号
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/07/03
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_release(DPP_DEV_T *dev, ZXIC_CONST ZXIC_UINT8* pName, 
                                  ZXIC_UINT32 queueId)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_MUTEX_T *p_dtb_mutex = NULL;
    DPP_DEV_MUTEX_TYPE_E mutex = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), queueId, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pName);

    ZXIC_COMM_TRACE_INFO("dpp_dtb_queue_release queue %u start.\n", queueId);

    //使用代理通道要加锁
    mutex = DPP_DEV_MUTEX_T_DTB;
    rc = dpp_dev_opr_mutex_get(dev, (ZXIC_UINT32)mutex, &p_dtb_mutex);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_dtb_mutex);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "zxic_comm_mutex_lock");

    rc = dpp_agent_channel_dtb_queue_release(dev, pName, queueId);

    if(rc == DPP_RC_DTB_QUEUE_NOT_ALLOC)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "dtb slot: %u vport: 0x%04x queue %u not request.\n", DEV_PCIE_SLOT(dev), DEV_PCIE_VPORT(dev), queueId);
    }

    if(rc == DPP_RC_DTB_QUEUE_NAME_ERROR)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "dtb slot: %u vport: 0x%04x queue %u name error.\n", DEV_PCIE_SLOT(dev), DEV_PCIE_VPORT(dev), queueId);
    }

    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_agent_channel_dtb_queue_release", p_dtb_mutex);

    rc = zxic_comm_mutex_unlock(p_dtb_mutex);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "zxic_comm_mutex_unlock");

    rc = dpp_dtb_queue_id_free(dev, queueId);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_queue_id_free");

    rc =  dpp_dtb_dump_addr_rb_destroy(dev, queueId);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_dump_addr_rb_destroy");

    ZXIC_COMM_PRINT("slot: %u vport: 0x%04x release queue: %u success.\n", DEV_PCIE_SLOT(dev), DEV_PCIE_VPORT(dev), queueId);

    return rc;
}

/***********************************************************/
/** DTB通道释放(增加锁保护)
* @param   devId       NP设备号
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/07/03
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_release_ex(DPP_DEV_T *dev)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 queue_id = 0;
    ZXIC_MUTEX_T *p_self_recover_mutex  = NULL;
    DPP_DEV_MUTEX_TYPE_E mutex = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 queue_num    = 0;
    ZXIC_UINT32 queue_arr[DPP_DTB_QUEUE_NUM_MAX] = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    mutex = DPP_DEV_MUTEX_T_SELF_RECOVER;
    rc = dpp_dev_opr_mutex_get(dev, (ZXIC_UINT32)mutex, &p_self_recover_mutex);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_self_recover_mutex);
    ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_lock");

    rc = dpp_dtb_queue_id_search_by_vport(dev, queue_arr, &queue_num);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_queue_id_search_by_vport",p_self_recover_mutex);

    for(i = 0; i < queue_num; i++)
    {
        queue_id = queue_arr[i];
        rc = dpp_dtb_queue_release(dev, "pf", queue_id);
        if(rc == DPP_OK)
        {
            rc = dpp_dtb_queue_dma_mem_release(dev, queue_id);
            ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_queue_dma_mem_release",p_self_recover_mutex);
        }
        else
        {
            ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_queue_release",p_self_recover_mutex);
        }
    }

    rc = zxic_comm_mutex_unlock(p_self_recover_mutex);
    ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_unlock");

    return DPP_OK;
}

/***********************************************************/
/** DTB队列软件资源释放
* @param   dev       NP设备
* @return
* @remark  无
* @see
* @author  cq      @date  2025/06/06
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_release_soft(DPP_DEV_T *dev)
{
    ZXIC_UINT32 rc             = DPP_OK;
    ZXIC_UINT32 queue_id       = 0;
    ZXIC_MUTEX_T *p_self_recover_mutex  = NULL;
    DPP_DEV_MUTEX_TYPE_E mutex = 0;
    DPP_DTB_MGR_T *p_dtb_mgr   = ZXIC_NULL;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 queue_num    = 0;
    ZXIC_UINT32 queue_arr[DPP_DTB_QUEUE_NUM_MAX] = {0};
    
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev),queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);

    p_dtb_mgr = dpp_dtb_mgr_get(DEV_PCIE_SLOT(dev), DEV_ID(dev));
    if (p_dtb_mgr == ZXIC_NULL)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "slot %d ErrorCode[0x%x]: DTB Manager is not exist!!!\n", DEV_PCIE_SLOT(dev), DPP_RC_DTB_MGR_NOT_EXIST);
        return DPP_RC_DTB_MGR_NOT_EXIST;
    }

    mutex = DPP_DEV_MUTEX_T_SELF_RECOVER;
    rc = dpp_dev_opr_mutex_get(dev, (ZXIC_UINT32)mutex, &p_self_recover_mutex);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_self_recover_mutex);
    ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_lock");

    rc = dpp_dtb_queue_id_search_by_vport(dev, queue_arr, &queue_num);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_queue_id_search_by_vport",p_self_recover_mutex);

    for(i = 0; i < queue_num; i++)
    {
        queue_id = queue_arr[i];
        
        rc = dpp_dtb_queue_init_flag_set(dev, queue_id, DPP_DTB_QUEUE_UNINIT);
        ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_queue_init_flag_set",p_self_recover_mutex);

        rc =  dpp_dtb_dump_addr_rb_destroy(dev, queue_id);
        ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_dump_addr_rb_destroy",p_self_recover_mutex);

        p_dtb_mgr->queue_info[queue_id].func_type = DPP_DTB_QUEUE_TYPE_INVALID;
        p_dtb_mgr->queue_info[queue_id].slot_id   = 0xFFFFFFFF;
        p_dtb_mgr->queue_info[queue_id].vport     = 0XFFFFFFFF;

        ZXIC_COMM_MEMSET_S(&(p_dtb_mgr->queue_info[queue_id].tab_up), sizeof(DPP_DTB_TAB_UP_INFO_T), 0, sizeof(DPP_DTB_TAB_UP_INFO_T));
        ZXIC_COMM_MEMSET_S(&(p_dtb_mgr->queue_info[queue_id].tab_down), sizeof(DPP_DTB_TAB_DOWN_INFO_T), 0, sizeof(DPP_DTB_TAB_DOWN_INFO_T));

        rc = dpp_dtb_queue_dma_mem_release(dev, queue_id);
        ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_queue_dma_mem_release",p_self_recover_mutex);

        
    }

    rc = zxic_comm_mutex_unlock(p_self_recover_mutex);
    ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_unlock");

    return DPP_OK;
}

/***********************************************************/
/** 通知固件通道信息，保证固件与驱动配置一致
* @param   devId       NP设备号
* @param   pName       要释放DTB通道的唯一设备名(最大32字符)
* @param   vPort       端口号
* @param   pQueueId    同步队列
* @return
* @remark  无
* @see
* @author  cq      @date  2024/11/06
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_sync_cfg(DPP_DEV_T *dev, ZXIC_CONST ZXIC_UINT8* pName, 
                                  ZXIC_UINT16 vPort,
                                  ZXIC_UINT32 queueId)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_MUTEX_T *p_dtb_mutex = NULL;
    DPP_DEV_MUTEX_TYPE_E mutex = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), queueId, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), pName);

    ZXIC_COMM_TRACE_INFO("dpp_dtb_queue_sync_cfg:queue_id %d\n", queueId);

    //使用代理通道要加锁
    mutex = DPP_DEV_MUTEX_T_DTB;
    rc = dpp_dev_opr_mutex_get(dev, (ZXIC_UINT32)mutex, &p_dtb_mutex);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_dtb_mutex);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "zxic_comm_mutex_lock");

    rc = dpp_agent_channel_dtb_queue_sync_cfg(dev, pName, vPort, queueId);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_agent_channel_dtb_queue_sync_cfg", p_dtb_mutex);
    
    rc = zxic_comm_mutex_unlock(p_dtb_mutex);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "zxic_comm_mutex_unlock");

    rc = dpp_dtb_user_info_set(dev, queueId, vPort, 0);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_user_info_set");

    ZXIC_COMM_PRINT("dpp_dtb_queue_sync_cfg:slot %d name %s vport 0x%x queue_id %d\n", DEV_PCIE_SLOT(dev), pName, vPort, queueId);

    return rc;
}

/***********************************************************/
/** DTB通道用户信息配置
* @param   devId      NP设备号
* @param   queueId    DTB通道编号
* @param   vPort      vport信息
* @param   vector     中断号
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/07/03
************************************************************/
ZXIC_UINT32 dpp_dtb_user_info_set(DPP_DEV_T *dev,
                                  ZXIC_UINT32 queueId, 
                                  ZXIC_UINT16 vPort, 
                                  ZXIC_UINT32 vector)
{
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_UINT32 temp_epid;
    ZXIC_UINT32 temp_func_active;
    ZXIC_UINT32 temp_func_num;
    ZXIC_UINT32 temp_vfunc_num;
    ZXIC_UINT32 virtioPort = (ZXIC_UINT32)vPort;

    DPP_DTB_QUEUE_VM_INFO_T vm_info = {0};
    DPP_DTB_MGR_T *p_dtb_mgr = ZXIC_NULL;

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), queueId, 0, DPP_DTB_QUEUE_NUM_MAX - 1);

    p_dtb_mgr = dpp_dtb_mgr_get(DEV_PCIE_SLOT(dev), DEV_ID(dev));
    if (p_dtb_mgr == ZXIC_NULL)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "ErrorCode[0x%x]: DTB Manager is not exist!!!\n", DPP_RC_DTB_MGR_NOT_EXIST);
        return DPP_RC_DTB_MGR_NOT_EXIST;
    }

    ZXIC_COMM_UINT32_GET_BITS(temp_epid, virtioPort, VPORT_EPID_BT_START, VPORT_EPID_BT_LEN);
    ZXIC_COMM_UINT32_GET_BITS(temp_func_active, virtioPort, VPORT_FUNC_ACTIVE_BT_START, VPORT_FUNC_ACTIVE_BT_LEN);
    ZXIC_COMM_UINT32_GET_BITS(temp_func_num, virtioPort, VPORT_FUNC_NUM_BT_START, VPORT_FUNC_NUM_BT_LEN);
    ZXIC_COMM_UINT32_GET_BITS(temp_vfunc_num, virtioPort, VPORT_VFUNC_NUM_BT_START, VPORT_VFUNC_NUM_BT_LEN);

    rc = dpp_dtb_queue_vm_info_get(dev, queueId, &vm_info);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_queue_vm_info_get");

    vm_info.dbi_en = msix_interrupt_mode;
    vm_info.epid = dpp_dtb_ep_id_get(temp_epid);
    vm_info.vfunc_num = temp_vfunc_num;
    vm_info.func_num = temp_func_num;
    vm_info.vfunc_active = temp_func_active;
    vm_info.vector = vector;

    ZXIC_COMM_TRACE_NOTICE("queue %d vport 0x%x, vector:%x\n", queueId, vPort, vector);
    ZXIC_COMM_TRACE_NOTICE("dbi_en 0x%x\n", vm_info.dbi_en);
    ZXIC_COMM_TRACE_NOTICE("epid 0x%x\n", vm_info.epid);
    ZXIC_COMM_TRACE_NOTICE("vfunc_num 0x%x\n", vm_info.vfunc_num);
    ZXIC_COMM_TRACE_NOTICE("func_num 0x%x\n", vm_info.func_num);
    ZXIC_COMM_TRACE_NOTICE("vfunc_active 0x%x\n", vm_info.vfunc_active);

    p_dtb_mgr->queue_info[queueId].vport = vPort;
    p_dtb_mgr->queue_info[queueId].vector = vector;

    rc = dpp_dtb_queue_vm_info_set(dev, queueId, &vm_info);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_queue_vm_info_set");

    return rc;
}

/***********************************************************/
/** DTB通道用户信息配置
* @param   devId      NP设备号
* @param   queueId    DTB通道编号
* @param   func_type  队列功能类型，0：流表队列，1：统计队列
* @return
* @remark  无
* @see
* @author  cbb      @date  2025/11/21
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_func_type_set(DPP_DEV_T *dev,
                                        ZXIC_UINT32 queueId, 
                                        ZXIC_UINT32 func_type)
{
    ZXIC_UINT32 rc = DPP_OK;

    DPP_DTB_MGR_T *p_dtb_mgr = ZXIC_NULL;

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), queueId, 0, DPP_DTB_QUEUE_NUM_MAX - 1);

    p_dtb_mgr = dpp_dtb_mgr_get(DEV_PCIE_SLOT(dev), DEV_ID(dev));
    if (p_dtb_mgr == ZXIC_NULL)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "ErrorCode[0x%x]: DTB Manager is not exist!!!\n", DPP_RC_DTB_MGR_NOT_EXIST);
        return DPP_RC_DTB_MGR_NOT_EXIST;
    }

    p_dtb_mgr->queue_info[queueId].func_type = func_type;

    return rc;
}

/***********************************************************/
/** DTB通道下表空间地址设置,空间大小[32*(16+16*1024)B]
* @param   devId      NP设备号
* @param   queueId    DTB通道编号
* @param   phyAddr    物理地址
* @param   virAddr    虚拟地址
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/07/03
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_down_table_addr_set(DPP_DEV_T *dev,
                                              ZXIC_UINT32 queueId, 
                                              ZXIC_UINT64 phyAddr, 
                                              ZXIC_UINT64 virAddr)
{
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), queueId, 0, DPP_DTB_QUEUE_NUM_MAX - 1);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x queue: %u phyAddr: 0x%llx\n", DEV_PCIE_SLOT(dev), DEV_PCIE_VPORT(dev), queueId, phyAddr);
    ZXIC_COMM_TRACE_NOTICE("slot: %d queue: %d virAddr: 0x%llx\n", DEV_PCIE_SLOT(dev), queueId, virAddr);

    rc = dpp_dtb_down_channel_addr_set(dev, 
                                       queueId,
                                       phyAddr,
                                       virAddr,
                                       0);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_down_channel_addr_set");

    return rc;
}

/***********************************************************/
/** DTB通道dump空间地址设置,空间大小[32*(16+16*1024)B]
* @param   devId      NP设备号
* @param   pName      要释放DTB通道的设备名
* @param   phyAddr    物理地址
* @param   virAddr    虚拟地址
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/07/03
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_dump_table_addr_set(DPP_DEV_T *dev,
                                              ZXIC_UINT32 queueId, 
                                              ZXIC_UINT64 phyAddr, 
                                              ZXIC_UINT64 virAddr)
{
    ZXIC_UINT32 rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), queueId, 0, DPP_DTB_QUEUE_NUM_MAX - 1);

    ZXIC_COMM_TRACE_NOTICE("slot: %u vport: 0x%04x queue: %u phyAddr: 0x%llx\n", DEV_PCIE_SLOT(dev), DEV_PCIE_VPORT(dev), queueId, phyAddr);
    ZXIC_COMM_TRACE_NOTICE("slot %d queue %d virAddr 0x%llx\n", DEV_PCIE_SLOT(dev), queueId, virAddr);

    rc = dpp_dtb_dump_channel_addr_set(dev, 
                                       queueId, 
                                       phyAddr, 
                                       virAddr, 
                                       0);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_dump_channel_addr_set");

    return rc;
}

/***********************************************************/
/** 大批量dump一个流表使用的地址空间配置
* @param   devId       NP设备号
* @param   queueId     DTB队列编号
* @param   sdtNo       流表std号
* @param   phyAddr     物理地址
* @param   virAddr     虚拟地址
* @param   size        (最大64MB)
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/07/03
************************************************************/
ZXIC_UINT32 dpp_dtb_dump_sdt_addr_set(DPP_DEV_T *dev,
                                      ZXIC_UINT32 queueId,  
                                      ZXIC_UINT32 sdtNo, 
                                      ZXIC_UINT64 phyAddr, 
                                      ZXIC_UINT64 virAddr, 
                                      ZXIC_UINT32 size)
{
    ZXIC_UINT32 rc = DPP_OK;

    DPP_DTB_DUMP_ADDR_INFO_T dtb_dump_addr_info = {0};
    ZXIC_RB_CFG *p_dtb_dump_addr_rb = NULL;

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), queueId, 0, DPP_DTB_QUEUE_NUM_MAX - 1);

    ZXIC_COMM_TRACE_INFO("slotId  :0x%x\n", DEV_PCIE_SLOT(dev));
    ZXIC_COMM_TRACE_INFO("queueId :0x%x\n", queueId);
    ZXIC_COMM_TRACE_INFO("sdtNo   :0x%x\n", sdtNo);
    ZXIC_COMM_TRACE_INFO("phyAddr :0x%016llx\n", phyAddr); 
    ZXIC_COMM_TRACE_INFO("virAddr :0x%016llx\n", virAddr);
    ZXIC_COMM_TRACE_INFO("size    :0x%x\n", size);

    dtb_dump_addr_info.sdt_no = sdtNo;
    dtb_dump_addr_info.phyAddr = phyAddr;
    dtb_dump_addr_info.virAddr = virAddr;
    dtb_dump_addr_info.size = size;

    p_dtb_dump_addr_rb = dpp_dtb_dump_addr_rb_get(dev, queueId);
    ZXIC_COMM_CHECK_POINT_NO_ASSERT(p_dtb_dump_addr_rb);
    ZXIC_COMM_CHECK_INDEX_UPPER_NO_ASSERT(p_dtb_dump_addr_rb->key_size, (ZXIC_UINT32)sizeof(DPP_DTB_DUMP_ADDR_INFO_T));

    rc = dpp_apt_sw_list_insert(p_dtb_dump_addr_rb, &dtb_dump_addr_info, sizeof(DPP_DTB_DUMP_ADDR_INFO_T));
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_apt_sw_list_insert");

    return rc;
}

/***********************************************************/
/** 清除大批量dump一个流表使用的地址空间配置
* @param   devId        NP设备号
* @param   queueId      DTB队列编号
* @param   sdtNo        流表std号
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/07/03
************************************************************/
ZXIC_UINT32 dpp_dtb_dump_sdt_addr_clear(DPP_DEV_T *dev,
                                        ZXIC_UINT32 queueId, 
                                        ZXIC_UINT32 sdtNo)
{
    ZXIC_UINT32 rc = DPP_OK;

    DPP_DTB_DUMP_ADDR_INFO_T dtb_dump_addr_info = {0};
    ZXIC_RB_CFG *p_dtb_dump_addr_rb = NULL;

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), queueId, 0, DPP_DTB_QUEUE_NUM_MAX - 1);

    dtb_dump_addr_info.sdt_no = sdtNo;

    p_dtb_dump_addr_rb = dpp_dtb_dump_addr_rb_get(dev, queueId);
    rc = dpp_apt_sw_list_delete(p_dtb_dump_addr_rb, &dtb_dump_addr_info, sizeof(DPP_DTB_DUMP_ADDR_INFO_T));
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_apt_sw_list_delete");

    return rc;
}

/***********************************************************/
/** 释放当前sdt下的所有流表(硬件方式)
* (适用于进程启动后，仅配置流表资源，软件未配置流表，但需要删除硬件上已配置流表的场景) 
* @param   dev  
* @param   queue_id  队列号
* @param   sdt_no  sdt号 
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/12/04
************************************************************/
DPP_STATUS dpp_dtb_hash_offline_delete(DPP_DEV_T *dev,
                                        ZXIC_UINT32 queue_id,
                                        ZXIC_UINT32 sdt_no)

{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 entryNum = 0;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 max_item_num = DTB_DUMP_MULTICAST_MAC_DUMP_NUM;
    ZXIC_UINT8* pDumpData = NULL;
    ZXIC_UINT8 *pKey = NULL;
    ZXIC_UINT8 *pRst = NULL;
    ZXIC_UINT32 element_id = 0;
    
    DPP_DTB_HASH_ENTRY_INFO_T *p_dtb_hash_entry = NULL;
    DPP_DTB_HASH_ENTRY_INFO_T *p_temp_entry = NULL;
    DPP_SDTTBL_HASH_T sdt_hash_info = {0};  /*SDT内容*/

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);

    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_hash_info);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_soft_sdt_tbl_get");

    rc = dpp_hash_max_item_num_get(dev,sdt_no,&max_item_num);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_hash_max_item_num_get");

    pDumpData = (ZXIC_UINT8 *)ZXIC_COMM_VMALLOC(max_item_num*sizeof(DPP_DTB_HASH_ENTRY_INFO_T));
    ZXIC_COMM_CHECK_POINT_NO_ASSERT(pDumpData);
    pKey = (ZXIC_UINT8 *)ZXIC_COMM_VMALLOC(max_item_num * HASH_KEY_MAX);
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE_NO_ASSERT(pKey, pDumpData);
    pRst = (ZXIC_UINT8 *)ZXIC_COMM_VMALLOC(max_item_num * HASH_RST_MAX);
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE2PTR_NO_ASSERT(pRst, pKey, pDumpData);

    ZXIC_COMM_MEMSET_S(pDumpData,max_item_num*sizeof(DPP_DTB_HASH_ENTRY_INFO_T),0x0,max_item_num*sizeof(DPP_DTB_HASH_ENTRY_INFO_T));
    ZXIC_COMM_MEMSET_S(pKey,max_item_num*HASH_KEY_MAX,0x0,max_item_num*HASH_KEY_MAX);
    ZXIC_COMM_MEMSET_S(pRst,max_item_num*HASH_RST_MAX,0x0,max_item_num*HASH_RST_MAX);

    p_dtb_hash_entry = (DPP_DTB_HASH_ENTRY_INFO_T *)pDumpData;
    for(index = 0;index<max_item_num;index++)
    {
        p_temp_entry = p_dtb_hash_entry + index;
        p_temp_entry->p_actu_key = pKey + index * HASH_KEY_MAX;
        p_temp_entry->p_rst = pRst + index * HASH_RST_MAX;
    }

    rc = dpp_dtb_hash_dump(dev, queue_id, sdt_no, pDumpData, &entryNum);
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE3PTR_NO_ASSERT(rc, "dpp_dtb_hash_dump", pRst, pKey, pDumpData);

    ZXIC_COMM_TRACE_INFO("dpp_dtb_hash_dump valid entry num is %u\n",entryNum);

    for(index = 0; index < entryNum; index++)
    {
        p_temp_entry = p_dtb_hash_entry + index;
        // //打印数据
        dpp_dtb_data_print(p_temp_entry->p_actu_key, DPP_GET_ACTU_KEY_BY_SIZE(sdt_hash_info.key_size) + 1);
        dpp_dtb_data_print(p_temp_entry->p_rst, 4 * (0x1 << sdt_hash_info.rsp_mode));
    }

    rc = dpp_dtb_hash_dma_delete_hardware(dev, queue_id, sdt_no, entryNum, p_dtb_hash_entry, &element_id);
    ZXIC_COMM_VFREE(p_dtb_hash_entry);
    ZXIC_COMM_VFREE(pKey);
    ZXIC_COMM_VFREE(pRst);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_hash_dma_delete_hardware"); 
               
    return DPP_OK;
}

/***********************************************************/
/** 在线释放当前sdt下的所有流表表项
* (适用于进程正常退出前删除表项，软件上有存储表项)
* @param   dev_id  设备号 
* @param   queue_id  队列号
* @param   sdt_no  sdt号 
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/12/04
************************************************************/
DPP_STATUS dpp_dtb_hash_online_delete(DPP_DEV_T *dev,
                                        ZXIC_UINT32 queue_id,
                                        ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT8 key_valid = 0;
    ZXIC_UINT32 table_id = 0;
    ZXIC_UINT32 key_type = 0;
    ZXIC_UINT32 element_id = 0;

    D_NODE *p_node = NULL;
    ZXIC_RB_TN *p_rb_tn = NULL;
    D_HEAD *p_head_hash_rb = NULL;
    DPP_HASH_CFG *p_hash_cfg = NULL;
    DPP_HASH_RBKEY_INFO *p_rbkey = NULL;
    HASH_ENTRY_CFG  hash_entry_cfg = {0}; 
    DPP_DTB_HASH_ENTRY_INFO_T hashEntry = {0};
    
    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);


    //从sdt_no中获取hash配置
    rc = dpp_hash_get_hash_info_from_sdt(dev, sdt_no, &hash_entry_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_hash_get_hash_info_from_sdt");

    p_hash_cfg = hash_entry_cfg.p_hash_cfg;
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_hash_cfg);

    p_head_hash_rb = &p_hash_cfg->hash_rb.tn_list;
    p_node = p_head_hash_rb->p_next;
    while(p_node)
    {
        p_rb_tn = (ZXIC_RB_TN *)p_node->data;
        p_rbkey = (DPP_HASH_RBKEY_INFO *)p_rb_tn->p_key;
        hashEntry.p_actu_key = p_rbkey->key+1;
        hashEntry.p_rst = p_rbkey->key;

        key_valid = DPP_GET_HASH_KEY_VALID(p_rbkey->key);
        table_id = DPP_GET_HASH_TBL_ID(p_rbkey->key);
        key_type = DPP_GET_HASH_KEY_TYPE(p_rbkey->key);
        if((!key_valid) 
            || (table_id != hash_entry_cfg.table_id)
            || (key_type != hash_entry_cfg.key_type))
        {
            p_node = p_node->next; 
            continue;
        }
        p_node = p_node->next;   

        rc = dpp_dtb_hash_dma_delete(dev, queue_id,sdt_no,1,&hashEntry,&element_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_dtb_hash_dma_delete");
    }

    return DPP_OK;
}

/***********************************************************/
/** 获取hash表项个数(从软件红黑树获取)
* @param   dev_id  设备号 
* @param   sdt_no  sdt号 
* @return  
* @remark  无
* @see     
* @author        @date  2026/06/12
************************************************************/
DPP_STATUS dpp_dtb_hash_item_num_by_soft(DPP_DEV_T *dev,
                                        ZXIC_UINT32 sdt_no,
                                        ZXIC_UINT32 *item_num)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT8 key_valid = 0;
    ZXIC_UINT32 table_id = 0;
    ZXIC_UINT32 key_type = 0;
    ZXIC_UINT32 hash_item_num = 0;

    D_NODE *p_node = NULL;
    ZXIC_RB_TN *p_rb_tn = NULL;
    D_HEAD *p_head_hash_rb = NULL;
    DPP_HASH_CFG *p_hash_cfg = NULL;
    DPP_HASH_RBKEY_INFO *p_rbkey = NULL;
    HASH_ENTRY_CFG  hash_entry_cfg = {0}; 
    DPP_DTB_HASH_ENTRY_INFO_T hashEntry = {0};
    ZXIC_MUTEX_T *p_hash_mutex = NULL;
    
    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, item_num);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    //从sdt_no中获取hash配置
    rc = dpp_hash_get_hash_info_from_sdt(dev, sdt_no, &hash_entry_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_hash_get_hash_info_from_sdt");

    p_hash_cfg = hash_entry_cfg.p_hash_cfg;
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_hash_cfg);

    rc = dpp_dev_hash_opr_mutex_get(dev, p_hash_cfg->fun_id, &p_hash_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_dev_hash_opr_mutex_get"); 
    rc = zxic_comm_mutex_lock(p_hash_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_mutex_lock"); 

    p_head_hash_rb = &p_hash_cfg->hash_rb.tn_list;
    p_node = p_head_hash_rb->p_next;
    while(p_node)
    {
        p_rb_tn = (ZXIC_RB_TN *)p_node->data;
        p_rbkey = (DPP_HASH_RBKEY_INFO *)p_rb_tn->p_key;
        hashEntry.p_actu_key = p_rbkey->key+1;
        hashEntry.p_rst = p_rbkey->key;

        key_valid = DPP_GET_HASH_KEY_VALID(p_rbkey->key);
        table_id = DPP_GET_HASH_TBL_ID(p_rbkey->key);
        key_type = DPP_GET_HASH_KEY_TYPE(p_rbkey->key);
        if((!key_valid) 
            || (table_id != hash_entry_cfg.table_id)
            || (key_type != hash_entry_cfg.key_type))
        {
            p_node = p_node->next; 
            continue;
        }
        hash_item_num++;
        p_node = p_node->next;   
    }

    rc = zxic_comm_mutex_unlock(p_hash_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_mutex_unlock"); 

    *item_num = hash_item_num;

    return DPP_OK;
}

/***********************************************************/
/** acl index资源申请
* @param   dev             NP设备
* @param   sdt_no          流表sdt号(0~255)
* @param   vport           端口号
* @param   p_index         申请到的索引值，acl下表时使用
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/14
************************************************************/  
DPP_STATUS dpp_dtb_acl_index_request(DPP_DEV_T *dev,
                                     ZXIC_UINT32 sdt_no,
                                     ZXIC_UINT32 vport,
                                     ZXIC_UINT32 *p_index)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 eram_sdt_no = 0;
    ZXIC_MUTEX_T *p_dtb_mutex = NULL;
    DPP_DEV_MUTEX_TYPE_E mutex = 0;
    DPP_SDTTBL_ETCAM_T sdt_acl = {0};  /*SDT内容*/
    DPP_SDTTBL_ERAM_T sdt_eram = {0};  /*SDT内容*/

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_index);

    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_acl);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_soft_sdt_tbl_get");
    if(sdt_acl.table_type != DPP_SDT_TBLT_eTCAM)
    {
        ZXIC_COMM_TRACE_ERROR("SDT[%d] table_type[ %d ] is not etcam table!\n", sdt_no, sdt_acl.table_type);
        return DPP_ERR;
    }

    eram_sdt_no = dpp_apt_get_sdt_partner(dev, sdt_no);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, eram_sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, eram_sdt_no, &sdt_eram);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_soft_sdt_tbl_get");
    if(sdt_eram.table_type != DPP_SDT_TBLT_eRAM)
    {
        ZXIC_COMM_TRACE_ERROR("SDT[%d] table_type[ %d ] is not eram table!\n", eram_sdt_no, sdt_eram.table_type);
        return DPP_ERR;
    }

    //使用代理通道要加锁
    mutex = DPP_DEV_MUTEX_T_DTB;
    rc = dpp_dev_opr_mutex_get(dev, (ZXIC_UINT32)mutex, &p_dtb_mutex);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_dtb_mutex);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "zxic_comm_mutex_lock");

    rc = dpp_agent_channel_acl_index_request(dev, sdt_no, vport, &index);
    if(rc == DPP_ACL_RC_INDEX_RES_FULL)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "acl index is full. \n");
    }
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_agent_channel_acl_index_request", p_dtb_mutex);

    rc = zxic_comm_mutex_unlock(p_dtb_mutex);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "zxic_comm_mutex_unlock");

    *p_index = index;

    ZXIC_COMM_PRINT("dpp_dtb_acl_index_request:slot %u vport 0x%04x index %u.\n", DEV_PCIE_SLOT(dev), vport, index);

    return rc;
}

/***********************************************************/
/** acl index资源释放
* @param   dev           NP设备
* @param   sdt_no          流表sdt号(0~255)
* @param   vport           端口号
* @param   index           需要释放的索引值
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/14
************************************************************/  
DPP_STATUS dpp_dtb_acl_index_release(DPP_DEV_T *dev,
                                     ZXIC_UINT32 sdt_no,
                                     ZXIC_UINT32 vport,
                                     ZXIC_UINT32 index)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 eram_sdt_no = 0;
    ZXIC_MUTEX_T *p_dtb_mutex = NULL;
    DPP_DEV_MUTEX_TYPE_E mutex = 0;
    DPP_SDTTBL_ETCAM_T sdt_acl = {0};  /*SDT内容*/
    DPP_SDTTBL_ERAM_T sdt_eram = {0};  /*SDT内容*/

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_acl);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_soft_sdt_tbl_get");
    if(sdt_acl.table_type != DPP_SDT_TBLT_eTCAM)
    {
        ZXIC_COMM_TRACE_ERROR("SDT[%d] table_type[ %d ] is not etcam table!\n", sdt_no, sdt_acl.table_type);
        return DPP_ERR;
    }

    eram_sdt_no = dpp_apt_get_sdt_partner(dev, sdt_no);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, eram_sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, eram_sdt_no, &sdt_eram);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_soft_sdt_tbl_get");
    if(sdt_eram.table_type != DPP_SDT_TBLT_eRAM)
    {
        ZXIC_COMM_TRACE_ERROR("SDT[%d] table_type[ %d ] is not eram table!\n", eram_sdt_no, sdt_eram.table_type);
        return DPP_ERR;
    }

    ZXIC_COMM_CHECK_DEV_INDEX_UPPER(dev_id, index, sdt_eram.eram_table_depth);

    //使用代理通道要加锁
    mutex = DPP_DEV_MUTEX_T_DTB;
    rc = dpp_dev_opr_mutex_get(dev, (ZXIC_UINT32)mutex, &p_dtb_mutex);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_dtb_mutex);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "zxic_comm_mutex_lock");

    rc = dpp_agent_channel_acl_index_release(dev, ACL_INDEX_RELEASE, sdt_no, vport, index);
    if(rc == DPP_ACL_RC_SRH_FAIL)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "slot %d vport 0x%x index %d is not exist. \n",DEV_PCIE_SLOT(dev), vport, index);
    }
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(dev_id, rc, "dpp_agent_channel_acl_index_release", p_dtb_mutex);

    rc = zxic_comm_mutex_unlock(p_dtb_mutex);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "zxic_comm_mutex_unlock");

    ZXIC_COMM_PRINT("dpp_dtb_acl_index_release:slot %u vport 0x%04x index %u.\n", DEV_PCIE_SLOT(dev), vport, index);

    return rc;
}

/***********************************************************/
/** 离线删除与vport关联的acl表项和索引值
* @param   dev_id          NP设备号
* @param   queue_id        dtb通道队列号(0~127)
* @param   sdt_no          流表sdt号(0~255)
* @param   vport           端口号
* @param   counter_id      统计编号，对应微码中的address
* @param   rd_mode         统计读取方式 0:64bit 1:128bit
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/14
************************************************************/  
DPP_STATUS dpp_dtb_acl_offline_delete(DPP_DEV_T *dev,
                                      ZXIC_UINT32 queue_id,
                                      ZXIC_UINT32 sdt_no,
                                      ZXIC_UINT32 vport,
                                      ZXIC_UINT32 counter_id,
                                      ZXIC_UINT32 rd_mode)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 index_num = 0;
    ZXIC_UINT32 eram_sdt_no = 0;
    ZXIC_UINT32 *p_index_array = NULL;
  
    DPP_SDTTBL_ETCAM_T sdt_acl = {0};  /*SDT内容*/
    DPP_SDTTBL_ERAM_T sdt_eram = {0};  /*SDT内容*/
    
    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_acl);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_soft_sdt_tbl_get");
    if(sdt_acl.table_type != DPP_SDT_TBLT_eTCAM)
    {
        ZXIC_COMM_TRACE_ERROR("SDT[%d] table_type[ %d ] is not etcam table!\n", sdt_no, sdt_acl.table_type);
        return DPP_ERR;
    }

    eram_sdt_no = dpp_apt_get_sdt_partner(dev, sdt_no);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, eram_sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    rc = dpp_soft_sdt_tbl_get(dev, eram_sdt_no, &sdt_eram);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_soft_sdt_tbl_get");
    if(sdt_eram.table_type != DPP_SDT_TBLT_eRAM)
    {
        ZXIC_COMM_TRACE_ERROR("SDT[%d] table_type[ %d ] is not eram table!\n", eram_sdt_no, sdt_eram.table_type);
        return DPP_ERR;
    }  

    p_index_array = (ZXIC_UINT32 *)ZXIC_COMM_MALLOC(sizeof(ZXIC_UINT32)*(sdt_eram.eram_table_depth));
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_index_array);

    /*解析有效的valid数据*/
    rc = dpp_dtb_acl_index_parse(dev,queue_id,eram_sdt_no,vport,&index_num,p_index_array);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE(dev_id, rc, "dpp_dtb_acl_index_parse", p_index_array);

    if(!index_num)
    {
        ZXIC_COMM_TRACE_INFO("SDT[%d] vport[0x%x] item num is zero!\n", sdt_no, vport);
        ZXIC_COMM_FREE(p_index_array);
        return DPP_OK;
    }
    
    /*delete acl数据*/
    rc = dpp_dtb_acl_data_clear(dev,queue_id,sdt_no,index_num,p_index_array);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE(dev_id, rc, "dpp_dtb_acl_data_clear", p_index_array);

    /*delete eram数据*/
    rc = dpp_dtb_eram_data_clear(dev, queue_id, eram_sdt_no, index_num,p_index_array);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE(dev_id, rc, "dpp_dtb_eram_data_clear", p_index_array);

    /*删除统计数据*/
    rc = dpp_dtb_eram_stat_data_clear(dev, queue_id, counter_id, rd_mode, index_num, p_index_array);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE(dev_id, rc, "dpp_dtb_eram_stat_data_clear", p_index_array);

    ZXIC_COMM_FREE(p_index_array);

    /*发送消息给固件清除index标志位*/
    rc = dpp_dtb_acl_index_release_by_vport(dev,sdt_no,vport);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_dtb_acl_index_release_by_vport");

    ZXIC_COMM_PRINT("dpp_dtb_acl_offline_delete:slot %u vport 0x%04x acl_num %u.\n", DEV_PCIE_SLOT(dev), vport, index_num);

    return rc;
} 

/***********************************************************/
/** 统计计数读清
* @param   dev              NP设备
* @param   queue_id         队列号
* @param   rd_mode          读取位宽模式，参见STAT_CNT_MODE_E，0-64bit，1-128bit
* @param   start_count_id   统计起始编号，对应微码中的address
* @param   num              统计项个数
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/14
************************************************************/  
DPP_STATUS dpp_dtb_stat_ppu_cnt_clr(DPP_DEV_T *dev, 
                                     ZXIC_UINT32 queue_id,
                                     STAT_CNT_MODE_E rd_mode,
                                     ZXIC_UINT32 start_count_id,
                                     ZXIC_UINT32 num)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 *p_index_array = NULL;

    
    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_LOWER(num,1);
    
    p_index_array = (ZXIC_UINT32 *)ZXIC_COMM_MALLOC(sizeof(ZXIC_UINT32)*num);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_index_array);

    for(i=0;i<num;i++)
    {
        p_index_array[i] = i;
    }

     /*删除统计数据*/
    rc = dpp_dtb_eram_stat_data_clear(dev, queue_id, start_count_id, rd_mode, num, p_index_array);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE(dev_id, rc, "dpp_dtb_eram_stat_data_clear", p_index_array);

    ZXIC_COMM_FREE(p_index_array);
    
    return rc;    
}

/***********************************************************/
/** 清除指定vport的对应的stat统计(dtb方式)
* @param   dev             NP设备
* @param   queue_id        队列号
* @param   sdt_no          流表sdt号(0~255)
* @param   vport           端口号
* @param   rd_mode         读取位宽模式，参见STAT_CNT_MODE_E，0-64bit，1-128bit
* @param   start_counter_id 统计起始编号，对应微码中的address
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/14
************************************************************/  
DPP_STATUS dpp_dtb_acl_stat_clr_by_vport(DPP_DEV_T *dev,
                                         ZXIC_UINT32 queue_id,
                                         ZXIC_UINT32 sdt_no,
                                         ZXIC_UINT32 vport,
                                         STAT_CNT_MODE_E rd_mode,
                                         ZXIC_UINT32 start_counter_id)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 eram_sdt_no = 0;
    ZXIC_UINT32 index_num = 0;
    ZXIC_UINT32 *p_index_array = NULL;
    
    DPP_SDTTBL_ETCAM_T sdt_etcam_info = {0};  /*SDT内容*/
    DPP_SDTTBL_ERAM_T sdt_eram = {0};  /*SDT内容*/
   
    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_etcam_info);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_soft_sdt_tbl_get");
    if(sdt_etcam_info.table_type != DPP_SDT_TBLT_eTCAM)
    {
        ZXIC_COMM_TRACE_ERROR("SDT[%d] table_type[ %d ] is not etcam table!\n", sdt_no, sdt_etcam_info.table_type);
        return DPP_ERR;
    }

    eram_sdt_no = dpp_apt_get_sdt_partner(dev, sdt_no);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, eram_sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    rc = dpp_soft_sdt_tbl_get(dev, eram_sdt_no, &sdt_eram);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_soft_sdt_tbl_get");
    if(sdt_eram.table_type != DPP_SDT_TBLT_eRAM)
    {
        ZXIC_COMM_TRACE_ERROR("SDT[%d] table_type[ %d ] is not eram table!\n", eram_sdt_no, sdt_eram.table_type);
        return DPP_ERR;
    } 

    p_index_array = (ZXIC_UINT32 *)ZXIC_COMM_MALLOC(sizeof(ZXIC_UINT32)*(sdt_eram.eram_table_depth));
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_index_array);

    /*解析有效的valid数据*/
    rc = dpp_dtb_acl_index_parse(dev,queue_id,eram_sdt_no,vport,&index_num,p_index_array);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE(dev_id, rc, "dpp_dtb_acl_index_parse", p_index_array);

    if(!index_num)
    {
        ZXIC_COMM_TRACE_INFO("SDT[%d] vport[0x%x] item num is zero!\n", sdt_no, vport);
        ZXIC_COMM_FREE(p_index_array);
        return DPP_OK;
    } 

    /*删除统计数据*/
    rc = dpp_dtb_eram_stat_data_clear(dev, queue_id, start_counter_id, rd_mode, index_num, p_index_array);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE(dev_id, rc, "dpp_dtb_eram_stat_data_clear", p_index_array);

    ZXIC_COMM_FREE(p_index_array);

    ZXIC_COMM_PRINT(" dpp_dtb_acl_stat_clr_by_vport sdt_no[%u] start_index[0x%x] vport[0x%x]\n", 
                       sdt_no,start_counter_id,vport);

    return DPP_OK; 
}

/***********************************************************/
/** 获取指定acl的空余条目数
* @param   dev_id          NP设备号
* @param   queue_id        dtb通道队列号(0~127)
* @param   sdt_no          流表sdt号(0~255)
* @param   p_unused_num    当前空余的条目数
* @return
* @remark  无
* @see
* @author  cq      @date  2025/12/25
************************************************************/  
DPP_STATUS dpp_dtb_acl_index_unused_num(DPP_DEV_T *dev,
                                      ZXIC_UINT32 queue_id,
                                      ZXIC_UINT32 sdt_no,
                                      ZXIC_UINT32 *p_unused_num)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 index_num = 0;
    ZXIC_UINT32 eram_sdt_no = 0;
    ZXIC_UINT32 *p_index_array = NULL;
  
    DPP_SDTTBL_ETCAM_T sdt_acl = {0};  /*SDT内容*/
    DPP_SDTTBL_ERAM_T sdt_eram = {0};  /*SDT内容*/
    
    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_POINT(p_unused_num);

    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_acl);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_soft_sdt_tbl_get");
    if(sdt_acl.table_type != DPP_SDT_TBLT_eTCAM)
    {
        ZXIC_COMM_TRACE_ERROR("SDT[%d] table_type[ %d ] is not etcam table!\n", sdt_no, sdt_acl.table_type);
        return DPP_ERR;
    }

    eram_sdt_no = dpp_apt_get_sdt_partner(dev, sdt_no);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, eram_sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    rc = dpp_soft_sdt_tbl_get(dev, eram_sdt_no, &sdt_eram);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_soft_sdt_tbl_get");
    if(sdt_eram.table_type != DPP_SDT_TBLT_eRAM)
    {
        ZXIC_COMM_TRACE_ERROR("SDT[%d] table_type[ %d ] is not eram table!\n", eram_sdt_no, sdt_eram.table_type);
        return DPP_ERR;
    }  

    p_index_array = (ZXIC_UINT32 *)ZXIC_COMM_MALLOC(sizeof(ZXIC_UINT32)*(sdt_eram.eram_table_depth));
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_index_array);

    /*解析有效的valid数据*/
    rc = dpp_dtb_acl_index_parse_all(dev,queue_id,eram_sdt_no,&index_num,p_index_array);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE(dev_id, rc, "dpp_dtb_acl_index_parse", p_index_array);

    *p_unused_num = (index_num < sdt_eram.eram_table_depth) ? (sdt_eram.eram_table_depth - index_num) : 0;
    ZXIC_COMM_FREE(p_index_array);

    return rc;
} 

/***********************************************************/
/** 消息通道获取pcie bar消息数目
* @param   dev               NP设备
* @param   p_bar_msg_num     出参，获取pcie bar数目
* @return
* @remark  无
* @see
* @author  cq      @date  2024/11/16
************************************************************/  
DPP_STATUS dpp_pcie_bar_msg_num_get(DPP_DEV_T *dev,ZXIC_UINT32 *p_bar_msg_num)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_MUTEX_T *p_dtb_mutex = NULL;
    DPP_DEV_MUTEX_TYPE_E mutex = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_bar_msg_num);

    mutex = DPP_DEV_MUTEX_T_DTB;
    rc = dpp_dev_opr_mutex_get(dev, (ZXIC_UINT32)mutex, &p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_lock");

    rc = dpp_agent_channel_pcie_bar_request(dev, p_bar_msg_num);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(dev_id, rc, "dpp_agent_channel_pcie_bar_request", p_dtb_mutex);

    rc = zxic_comm_mutex_unlock(p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_unlock");

    return DPP_OK;
}

/***********************************************************/
/** 通过key值删除指定vport下的acl表项,并释放申请的index索引
* @param   dev               NP设备
* @param   queue_id          队列号
* @param   sdt_no            sdt号
* @param   vport             端口号
* @param   entry_num         删除表项数目
* @param   p_acl_entry_arr   删除表项键值,查找到，删除并置位handle，未查找到，handle置位为无效值
* @return
* @remark  无
* @see
* @author  cq      @date  2025/12/26
************************************************************/
ZXIC_UINT32 dpp_dtb_acl_delete_by_key(DPP_DEV_T *dev,
                                    ZXIC_UINT32 queue_id,
                                    ZXIC_UINT32 sdt_no,
                                    ZXIC_UINT32 vport, 
                                    ZXIC_UINT32 entry_num, 
                                    DPP_DTB_ACL_ENTRY_INFO_T *p_acl_entry_arr)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 acl_depth = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 eram_sdt_no = 0;
    ZXIC_UINT32 match_num = 0;
    ZXIC_UINT32 dump_entry_num = 0;
    ZXIC_UINT32 dump_len = 0;
    ZXIC_UINT32 data_len = DPP_ETCAM_WIDTH_MAX / 8;
    ZXIC_UINT32 rst_len = 16;
    ZXIC_UINT8 *p_data = NULL;
    ZXIC_UINT8 *p_mask = NULL;
    ZXIC_UINT8 *p_rst = NULL;
    ZXIC_UINT32 *p_match_index_array = NULL;
    ZXIC_UINT32 *p_rel_flag_array = NULL;
    DPP_SDTTBL_ETCAM_T sdt_etcam = {0};  /*SDT内容*/
    DPP_DTB_ACL_ENTRY_INFO_T *p_dump_entry = NULL;
    
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_acl_entry_arr);
    
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_etcam);
    ZXIC_COMM_CHECK_RC(rc, "dpp_soft_sdt_tbl_get");
    acl_depth = sdt_etcam.etcam_table_depth;

    eram_sdt_no = dpp_apt_get_sdt_partner(dev, sdt_no);
    ZXIC_COMM_CHECK_INDEX(eram_sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    dump_len = acl_depth * (sizeof(DPP_DTB_ACL_ENTRY_INFO_T)+2*data_len+rst_len);
    p_dump_entry = (DPP_DTB_ACL_ENTRY_INFO_T *)ZXIC_COMM_VMALLOC(dump_len);
    ZXIC_COMM_CHECK_POINT(p_dump_entry);
    ZXIC_COMM_MEMSET_S(p_dump_entry,dump_len,0x0,dump_len);

    p_data = (ZXIC_UINT8 *)(p_dump_entry + acl_depth);
    p_mask = p_data + acl_depth * data_len;
    p_rst = p_mask + acl_depth * data_len;
    for(i=0;i<acl_depth;i++)
    {
        p_dump_entry[i].key_data = p_data + i * data_len;
        p_dump_entry[i].key_mask = p_mask + i * data_len;
        p_dump_entry[i].p_as_rslt = p_rst + i * rst_len;
    }

    rc = dpp_dtb_acl_entry_dump_by_vport(dev, queue_id, sdt_no, vport,  &dump_entry_num, p_dump_entry);
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE(rc, "dpp_dtb_acl_entry_dump_by_vport", p_dump_entry);

    p_match_index_array = (ZXIC_UINT32 *)ZXIC_COMM_VMALLOC(sizeof(ZXIC_UINT32)*acl_depth*2);
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE(p_match_index_array, p_dump_entry);
    p_rel_flag_array = p_match_index_array + acl_depth;
    ZXIC_COMM_MEMSET_S(p_match_index_array,sizeof(ZXIC_UINT32)*acl_depth*2, 0x0,sizeof(ZXIC_UINT32)*acl_depth*2);

    for(i=0;i<entry_num;i++)
    {
        match_num = 0;
        p_acl_entry_arr[i].handle = ACL_HANDLE_INVALID;
        for(j=0;j<dump_entry_num;j++)
        {
            if((p_dump_entry[j].handle != ACL_HANDLE_INVALID)
                && (0 == ZXIC_COMM_MEMCMP(p_dump_entry[j].key_data, p_acl_entry_arr[i].key_data, data_len))
                && (0 == ZXIC_COMM_MEMCMP(p_dump_entry[j].key_mask, p_acl_entry_arr[i].key_mask, data_len)))
            {
                if(p_acl_entry_arr[i].handle == ACL_HANDLE_INVALID)
                {
                    p_acl_entry_arr[i].handle = p_dump_entry[j].handle;
                }
                p_match_index_array[match_num++] = p_dump_entry[j].handle;
                p_rel_flag_array[j] = 1;
                ZXIC_COMM_TRACE_NOTICE("delete entry[%u] match dump entry[%u]\n",i,j);
            }
        }
        
        if(match_num)
        {
            rc = dpp_dtb_acl_data_clear(dev, queue_id, sdt_no, match_num, p_match_index_array);
            ZXIC_COMM_CHECK_RC_MEMORY_VFREE2PTR_NO_ASSERT(rc, "dpp_dtb_acl_data_clear",p_match_index_array, p_dump_entry);

            /*delete eram数据*/
            rc = dpp_dtb_eram_data_clear(dev, queue_id, eram_sdt_no, match_num, p_match_index_array);
            ZXIC_COMM_CHECK_RC_MEMORY_VFREE2PTR_NO_ASSERT(rc, "dpp_dtb_eram_data_clear", p_match_index_array, p_dump_entry);
        }
    }

    for(i=0;i<acl_depth;i++)
    {
        if(p_rel_flag_array[i])
        {
            rc |= dpp_dtb_acl_index_release(dev,sdt_no,vport,i);
        }
    }

    ZXIC_COMM_VFREE(p_match_index_array);
    ZXIC_COMM_VFREE(p_dump_entry);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_acl_index_release");

    return DPP_OK;
}

/***********************************************************/
/** 通过key值查找指定vport下的acl表项
* @param   dev               NP设备
* @param   queue_id          队列号
* @param   sdt_no            sdt号
* @param   vport             端口号
* @param   entry_num         查找表项数目
* @param   p_acl_entry_arr   查找表项键值,查找到，置位handle信息，并且将as_rlt信息拷贝到该表项中
* @return
* @remark  无
* @see
* @author  cq      @date  2025/12/26
************************************************************/ 
ZXIC_UINT32 dpp_dtb_acl_search_by_key(DPP_DEV_T *dev,
                                    ZXIC_UINT32 queue_id,
                                    ZXIC_UINT32 sdt_no,
                                    ZXIC_UINT32 vport, 
                                    ZXIC_UINT32 entry_num, 
                                    DPP_DTB_ACL_ENTRY_INFO_T *p_acl_entry_arr)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 acl_depth = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 dump_entry_num = 0;
    ZXIC_UINT32 dump_len = 0;
    ZXIC_UINT32 data_len = DPP_ETCAM_WIDTH_MAX / 8;
    ZXIC_UINT32 rst_len = 16;
    ZXIC_UINT8 *p_data = NULL;
    ZXIC_UINT8 *p_mask = NULL;
    ZXIC_UINT8 *p_rst = NULL;
    DPP_SDTTBL_ETCAM_T sdt_etcam = {0};  /*SDT内容*/
    DPP_DTB_ACL_ENTRY_INFO_T *p_dump_entry = NULL;
    
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_acl_entry_arr);
    
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_etcam);
    ZXIC_COMM_CHECK_RC(rc, "dpp_soft_sdt_tbl_get");
    acl_depth = sdt_etcam.etcam_table_depth;

    dump_len = acl_depth * (sizeof(DPP_DTB_ACL_ENTRY_INFO_T)+2*data_len+rst_len);
    p_dump_entry = (DPP_DTB_ACL_ENTRY_INFO_T *)ZXIC_COMM_VMALLOC(dump_len);
    ZXIC_COMM_CHECK_POINT(p_dump_entry);
    p_data = (ZXIC_UINT8 *)(p_dump_entry + acl_depth);
    p_mask = p_data + acl_depth * data_len;
    p_rst = p_mask + acl_depth * data_len;
    for(i=0;i<acl_depth;i++)
    {
        p_dump_entry[i].handle = ACL_HANDLE_INVALID;
        p_dump_entry[i].key_data = p_data + i * data_len;
        p_dump_entry[i].key_mask = p_mask + i * data_len;
        p_dump_entry[i].p_as_rslt = p_rst + i * rst_len;
    }

    rc = dpp_dtb_acl_entry_dump_by_vport(dev, queue_id, sdt_no, vport, &dump_entry_num, p_dump_entry);
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE(rc, "dpp_dtb_acl_entry_dump_by_vport", p_dump_entry);

    for(i=0;i<entry_num;i++)
    {
        p_acl_entry_arr[i].handle = ACL_HANDLE_INVALID;
        for(j=0;j<dump_entry_num;j++)
        {
            if((p_dump_entry[j].handle != ACL_HANDLE_INVALID)
                && (0 == ZXIC_COMM_MEMCMP(p_dump_entry[j].key_data, p_acl_entry_arr[i].key_data, data_len))
                && (0 == ZXIC_COMM_MEMCMP(p_dump_entry[j].key_mask, p_acl_entry_arr[i].key_mask, data_len)))
            {
                p_acl_entry_arr[i].handle = p_dump_entry[j].handle;
                ZXIC_COMM_MEMCPY_S(p_acl_entry_arr[i].p_as_rslt, rst_len, p_dump_entry[j].p_as_rslt, rst_len);
                break;
            }
        }
    }

    ZXIC_COMM_VFREE(p_dump_entry);

    return DPP_OK;
}

/***********************************************************/
/** dump指定vport下的acl表项
* @param   dev               NP设备
* @param   queue_id          队列号
* @param   sdt_no            sdt号
* @param   vport             端口号
* @param   entry_num         dump表项数目
* @param   p_acl_entry_arr   dump表项信息，若不属于当前vport，则将handle置为无效值
* @return
* @remark  无
* @see
* @author  cq      @date  2025/12/26
************************************************************/ 
ZXIC_UINT32 dpp_dtb_acl_entry_dump_by_vport(DPP_DEV_T *dev,
                                    ZXIC_UINT32 queue_id,
                                    ZXIC_UINT32 sdt_no,
                                    ZXIC_UINT32 vport, 
                                    ZXIC_UINT32 *p_entry_num, 
                                    DPP_DTB_ACL_ENTRY_INFO_T *p_acl_entry_arr)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 eram_sdt_no = 0;
    ZXIC_UINT32 eram_depth = 0;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 entry_index = 0;
    ZXIC_UINT32 index_num = 0;
    ZXIC_UINT32 *p_index_array = NULL;
    DPP_SDTTBL_ERAM_T sdt_eram = {0};  /*SDT内容*/

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_entry_num);
    ZXIC_COMM_CHECK_POINT(p_acl_entry_arr);

    eram_sdt_no = dpp_apt_get_sdt_partner(dev, sdt_no);
    ZXIC_COMM_CHECK_INDEX(eram_sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    
    rc = dpp_soft_sdt_tbl_get(dev, eram_sdt_no, &sdt_eram);
    ZXIC_COMM_CHECK_RC(rc, "dpp_soft_sdt_tbl_get");
    eram_depth = sdt_eram.eram_table_depth;

    p_index_array = ZXIC_COMM_MALLOC(sizeof(ZXIC_UINT32)*eram_depth);
    ZXIC_COMM_CHECK_POINT(p_index_array);

    rc = dpp_dtb_acl_index_parse(dev, queue_id, eram_sdt_no, vport, &index_num, p_index_array);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_dtb_acl_index_parse", p_index_array);

    rc = dpp_dtb_acl_dump(dev, queue_id, sdt_no, (ZXIC_UINT8 *)p_acl_entry_arr, p_entry_num);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_dtb_acl_dump", p_index_array);

    for(entry_index=0; entry_index<(*p_entry_num); entry_index++)
    {
        for(index=0; index<index_num; index++)
        {
            if(p_acl_entry_arr[entry_index].handle==p_index_array[index])
            {
                break;
            }
        }
        
        /*索引表里不包括此表项，则将handle置为无效值*/
        if(index == index_num)
        {
            p_acl_entry_arr[entry_index].handle = ACL_HANDLE_INVALID;
        } 
    }

    ZXIC_COMM_FREE(p_index_array);

    return DPP_OK;
}

/***********************************************************/
/** flush指定vport下的acl表项,释放索引表中对应的handle
* @param   dev               NP设备
* @param   queue_id          队列号
* @param   sdt_no            sdt号
* @param   vport             端口号
* @return
* @remark  无
* @see
* @author  cq      @date  2025/12/26
************************************************************/ 
ZXIC_UINT32 dpp_dtb_acl_entry_flush_by_vport(DPP_DEV_T *dev,
                                    ZXIC_UINT32 queue_id,
                                    ZXIC_UINT32 sdt_no,
                                    ZXIC_UINT32 vport)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 eram_sdt_no = 0;
    ZXIC_UINT32 eram_depth = 0;
    ZXIC_UINT32 index_num = 0;
    ZXIC_UINT32 *p_index_array = NULL;
    DPP_SDTTBL_ERAM_T sdt_eram = {0};  /*SDT内容*/

    ZXIC_COMM_CHECK_POINT(dev);

    eram_sdt_no = dpp_apt_get_sdt_partner(dev, sdt_no);
    ZXIC_COMM_CHECK_INDEX(eram_sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    
    rc = dpp_soft_sdt_tbl_get(dev, eram_sdt_no, &sdt_eram);
    ZXIC_COMM_CHECK_RC(rc, "dpp_soft_sdt_tbl_get");
    eram_depth = sdt_eram.eram_table_depth;

    p_index_array = ZXIC_COMM_MALLOC(sizeof(ZXIC_UINT32)*eram_depth);
    ZXIC_COMM_CHECK_POINT(p_index_array);

    rc = dpp_dtb_acl_index_parse(dev, queue_id, eram_sdt_no, vport, &index_num, p_index_array);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_dtb_acl_index_parse", p_index_array);

    if(!index_num)
    {
        ZXIC_COMM_TRACE_INFO("SDT[%d] vport[0x%x] item num is zero!\n", sdt_no, vport);
        ZXIC_COMM_FREE(p_index_array);
        return DPP_OK;
    }

    /*delete acl数据*/
    rc = dpp_dtb_acl_data_clear(dev,queue_id,sdt_no,index_num,p_index_array);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_dtb_acl_data_clear", p_index_array);

    /*delete eram数据*/
    rc = dpp_dtb_eram_data_clear(dev, queue_id, eram_sdt_no, index_num,p_index_array);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_dtb_eram_data_clear", p_index_array);

    ZXIC_COMM_FREE(p_index_array);

    /*发送消息给固件清除index标志位*/
    rc = dpp_dtb_acl_index_release_by_vport(dev,sdt_no,vport);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_acl_index_release_by_vport");

    return DPP_OK;
}

/***********************************************************/
/** flush所有acl表项,释放索引表中对应的handle
* @param   dev               NP设备
* @param   queue_id          队列号
* @param   sdt_no            sdt号
* @return
* @remark  无
* @see
* @author  cq      @date  2025/12/26
************************************************************/ 
ZXIC_UINT32 dpp_dtb_acl_entry_flush_all(DPP_DEV_T *dev,
                                    ZXIC_UINT32 queue_id,
                                    ZXIC_UINT32 sdt_no)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 eram_sdt_no = 0;
    ZXIC_UINT32 eram_depth = 0;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 *p_index_array = NULL;
    DPP_SDTTBL_ERAM_T sdt_eram = {0};  /*SDT内容*/

    ZXIC_COMM_CHECK_POINT(dev);

    eram_sdt_no = dpp_apt_get_sdt_partner(dev, sdt_no);
    ZXIC_COMM_CHECK_INDEX(eram_sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    
    rc = dpp_soft_sdt_tbl_get(dev, eram_sdt_no, &sdt_eram);
    ZXIC_COMM_CHECK_RC(rc, "dpp_soft_sdt_tbl_get");
    eram_depth = sdt_eram.eram_table_depth;

    p_index_array = ZXIC_COMM_MALLOC(sizeof(ZXIC_UINT32)*eram_depth);
    ZXIC_COMM_CHECK_POINT(p_index_array);

    for(index=0;index<eram_depth;index++)
    {
        p_index_array[index] = index;
    }

    /*delete acl数据*/
    rc = dpp_dtb_acl_data_clear(dev,queue_id,sdt_no,eram_depth,p_index_array);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_dtb_acl_data_clear", p_index_array);

    /*delete eram数据*/
    rc = dpp_dtb_eram_data_clear(dev, queue_id, eram_sdt_no, eram_depth,p_index_array);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_dtb_eram_data_clear", p_index_array);

    ZXIC_COMM_FREE(p_index_array);

    /*发送消息给固件清除index标志位*/
    rc = dpp_dtb_acl_index_release_all(dev,sdt_no);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_acl_index_release_all");

    return DPP_OK;
}

/***********************************************************/
/** 删除指定vport指定index的acl表项
* @param   dev               NP设备
* @param   queue_id          队列号
* @param   sdt_no            sdt号
* @param   vport             端口号
* @return
* @remark  无
* @see
* @author  cq      @date  2025/12/26
************************************************************/ 
ZXIC_UINT32 dpp_dtb_acl_del_by_index(DPP_DEV_T *dev,
                                    ZXIC_UINT32 queue_id,
                                    ZXIC_UINT32 sdt_no,
                                    ZXIC_UINT32 vport,
                                    ZXIC_UINT32 entry_num, 
                                    DPP_DTB_ACL_ENTRY_INFO_T *p_acl_entry_arr)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 eram_sdt_no = 0;
    ZXIC_UINT32 eram_depth = 0;
    ZXIC_UINT32 index_num = 0;
    ZXIC_UINT32 match_num = 0;
    ZXIC_UINT32 handle = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 *p_index_array = NULL;
    ZXIC_UINT32 *p_match_index_array = NULL;
    ZXIC_UINT32 *p_rel_index_array = NULL;
    DPP_SDTTBL_ERAM_T sdt_eram = {0};  /*SDT内容*/

    ZXIC_COMM_CHECK_POINT(dev);

    eram_sdt_no = dpp_apt_get_sdt_partner(dev, sdt_no);
    ZXIC_COMM_CHECK_INDEX(eram_sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    
    rc = dpp_soft_sdt_tbl_get(dev, eram_sdt_no, &sdt_eram);
    ZXIC_COMM_CHECK_RC(rc, "dpp_soft_sdt_tbl_get");
    eram_depth = sdt_eram.eram_table_depth;

    p_index_array = ZXIC_COMM_MALLOC(sizeof(ZXIC_UINT32)*eram_depth*3);
    ZXIC_COMM_CHECK_POINT(p_index_array);
    p_match_index_array = p_index_array + eram_depth;
    p_rel_index_array = p_match_index_array + eram_depth;
    ZXIC_COMM_MEMSET_S(p_index_array,sizeof(ZXIC_UINT32)*eram_depth*3, 0x0,sizeof(ZXIC_UINT32)*eram_depth*3);

    rc = dpp_dtb_acl_index_parse(dev, queue_id, eram_sdt_no, vport, &index_num, p_index_array);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_dtb_acl_index_parse", p_index_array);

    for(i=0;i<entry_num;i++)
    {
        for(j=0;j<index_num;j++)
        {
            if( (p_index_array[j] < eram_depth)
                && (p_index_array[j] == p_acl_entry_arr[i].handle))
            {
                handle = p_index_array[j];
                if(p_rel_index_array[handle]==0)
                {
                    p_rel_index_array[handle] = 1;
                    p_match_index_array[match_num++]=handle;
                }
                break;
            }
        }

        /*索引表里不包括此表项，则将handle置为无效值*/
        if(j == index_num)
        {
            p_acl_entry_arr[i].handle = ACL_HANDLE_INVALID;
        } 
    }

    if(match_num)
    {
        rc = dpp_dtb_acl_data_clear(dev, queue_id, sdt_no, match_num, p_match_index_array);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_dtb_acl_data_clear",p_index_array);

        /*delete eram数据*/
        rc = dpp_dtb_eram_data_clear(dev, queue_id, eram_sdt_no, match_num, p_match_index_array);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_dtb_eram_data_clear", p_index_array);
    }

    for(i=0;i<match_num;i++)
    {
        rc |= dpp_dtb_acl_index_release(dev,sdt_no,vport,p_match_index_array[i]);
    }
    ZXIC_COMM_FREE(p_index_array);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dtb_acl_index_release");

    return DPP_OK;
}

/***********************************************************/
/** 查找指定vport指定index的acl表项
* @param   dev               NP设备
* @param   queue_id          队列号
* @param   sdt_no            sdt号
* @param   vport             端口号
* @return
* @remark  无
* @see
* @author  cq      @date  2025/12/26
************************************************************/ 
ZXIC_UINT32 dpp_dtb_acl_search_by_index(DPP_DEV_T *dev,
                                    ZXIC_UINT32 queue_id,
                                    ZXIC_UINT32 sdt_no,
                                    ZXIC_UINT32 vport,
                                    ZXIC_UINT32 entry_num, 
                                    DPP_DTB_ACL_ENTRY_INFO_T *p_acl_entry_arr)
{
    ZXIC_UINT32 rc = DPP_OK;
    ZXIC_UINT32 acl_depth = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 dump_entry_num = 0;
    ZXIC_UINT32 dump_len = 0;
    ZXIC_UINT32 data_len = DPP_ETCAM_WIDTH_MAX / 8;
    ZXIC_UINT32 rst_len = 16;
    ZXIC_UINT8 *p_data = NULL;
    ZXIC_UINT8 *p_mask = NULL;
    ZXIC_UINT8 *p_rst = NULL;
    DPP_SDTTBL_ETCAM_T sdt_etcam = {0};  /*SDT内容*/
    DPP_DTB_ACL_ENTRY_INFO_T *p_dump_entry = NULL;
    
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_acl_entry_arr);
    
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_etcam);
    ZXIC_COMM_CHECK_RC(rc, "dpp_soft_sdt_tbl_get");
    acl_depth = sdt_etcam.etcam_table_depth;

    dump_len = acl_depth * (sizeof(DPP_DTB_ACL_ENTRY_INFO_T)+2*data_len+rst_len);
    p_dump_entry = (DPP_DTB_ACL_ENTRY_INFO_T *)ZXIC_COMM_VMALLOC(dump_len);
    ZXIC_COMM_CHECK_POINT(p_dump_entry);
    p_data = (ZXIC_UINT8 *)(p_dump_entry + acl_depth);
    p_mask = p_data + acl_depth * data_len;
    p_rst = p_mask + acl_depth * data_len;
    for(i=0;i<acl_depth;i++)
    {
        p_dump_entry[i].handle = ACL_HANDLE_INVALID;
        p_dump_entry[i].key_data = p_data + i * data_len;
        p_dump_entry[i].key_mask = p_mask + i * data_len;
        p_dump_entry[i].p_as_rslt = p_rst + i * rst_len;
    }

    rc = dpp_dtb_acl_entry_dump_by_vport(dev, queue_id, sdt_no, vport, &dump_entry_num, p_dump_entry);
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE(rc, "dpp_dtb_acl_entry_dump_by_vport", p_dump_entry);

    for(i=0;i<entry_num;i++)
    {
        index = p_acl_entry_arr[i].handle;
        if(p_dump_entry[index].handle != ACL_HANDLE_INVALID)
        {
            ZXIC_COMM_MEMCPY_S(p_acl_entry_arr[i].key_data, data_len, p_dump_entry[index].key_data, data_len);
            ZXIC_COMM_MEMCPY_S(p_acl_entry_arr[i].key_mask, data_len, p_dump_entry[index].key_mask, data_len);
            ZXIC_COMM_MEMCPY_S(p_acl_entry_arr[i].p_as_rslt, rst_len, p_dump_entry[index].p_as_rslt, rst_len);
        }
        else
        {
            p_acl_entry_arr[i].handle = ACL_HANDLE_INVALID;
        }
    }

    ZXIC_COMM_VFREE(p_dump_entry);

    return DPP_OK;
}

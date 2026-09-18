/**************************************************************
* 版权所有(C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_dtb.c
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : zab
* 完成日期 : 2022/08/26
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
#include "dpp_type_api.h"
#include "dpp_dtb_cfg.h"
#include "dpp_dtb.h"
#include "dpp_dtb_table.h"
#include "dpp_hash.h"
#include "dpp_dtb_reg.h"
#include "dpp_reg_api.h"
#include "dpp_reg_info.h"
#include "dpp_se_api.h"

#if ZXIC_REAL("DTB")

ZXIC_CHAR *g_dpp_dtb_name[] =
{
    "DOWN TAB",
    "UP TAB",
};

DPP_DTB_MGR_T *p_dpp_dtb_mgr[DPP_PCIE_SLOT_MAX][DPP_DEV_CHANNEL_MAX]   = {{NULL}};

/**dtb超时时间 单位us*/
ZXIC_UINT32 g_dtb_base_timeout = 50;
static ZXIC_UINT32 g_dtb_down_overtime = 4*50*1000; //200ms
static ZXIC_UINT32 g_dtb_dump_overtime = 5*1000*1000;//5s

/*配置dtb调试函数*/
static ZXIC_UINT32 g_dtb_debug_fun_en = 0;
static ZXIC_UINT32 g_dtb_print_en = 0;
static ZXIC_UINT32 g_dtb_soft_perf_test = 0;
static ZXIC_UINT32 g_dtb_hardware_perf_test = 0;

/*dtb下表/dump使能标记*/
static ZXIC_UINT32 g_dtb_func_switch_en = 1;
ZXIC_UINT32 dtb_table_function_switch_get(ZXIC_VOID)
{
    return g_dtb_func_switch_en;
}

ZXIC_UINT32 dtb_table_function_switch_enable(ZXIC_VOID)
{
    g_dtb_func_switch_en = 1;
    return 0;
}
EXPORT_SYMBOL(dtb_table_function_switch_enable);

ZXIC_UINT32 dtb_table_function_switch_disable(ZXIC_VOID)
{
    g_dtb_func_switch_en = 0;
    return 0;
}
EXPORT_SYMBOL(dtb_table_function_switch_disable);

/**使能dtb调试函数*/
ZXIC_UINT32 dpp_dtb_debug_fun_enable(ZXIC_VOID)
{
    g_dtb_debug_fun_en = 1;
    return 0;
}

/**去使能dtb调试函数*/
ZXIC_UINT32 dpp_dtb_debug_fun_disable(ZXIC_VOID)
{
    g_dtb_debug_fun_en = 0;
    return 0;
}

/**获取dtb调试函数*/
ZXIC_UINT32 dpp_dtb_debug_fun_get(ZXIC_VOID)
{
    return g_dtb_debug_fun_en;
}

/**使能dtb打印函数*/
ZXIC_UINT32 dpp_dtb_prt_enable(ZXIC_VOID)
{
    g_dtb_print_en = 1;
    return 0;
}

/**去使能dtb打印函数*/
ZXIC_UINT32 dpp_dtb_prt_disable(ZXIC_VOID)
{
    g_dtb_print_en = 0;
    return 0;
}

/**获取dtb打印函数*/
ZXIC_UINT32 dpp_dtb_prt_get(ZXIC_VOID)
{
    return g_dtb_print_en;
}

ZXIC_UINT32 dpp_dtb_soft_perf_test_set(ZXIC_UINT32 value)
{
    g_dtb_soft_perf_test = value;
    return 0;
}

ZXIC_UINT32 dpp_dtb_soft_perf_test_get(ZXIC_VOID)
{
    return g_dtb_soft_perf_test;
}

ZXIC_UINT32 dpp_dtb_hardware_perf_test_set(ZXIC_UINT32 value)
{
    g_dtb_hardware_perf_test = value;
    return 0;
}

ZXIC_UINT32 dpp_dtb_hardware_perf_test_get(ZXIC_VOID)
{
    return g_dtb_hardware_perf_test;
}

ZXIC_UINT32 dpp_dtb_down_table_overtime_set(ZXIC_UINT32 times_s)
{
    g_dtb_down_overtime = times_s;
    return 0;
}

ZXIC_UINT32 dpp_dtb_down_table_overtime_get(ZXIC_VOID)
{
    return g_dtb_down_overtime;
}

ZXIC_UINT32 dpp_dtb_dump_table_overtime_set(ZXIC_UINT32 times_s)
{
    g_dtb_dump_overtime = times_s;
    return 0;
}

ZXIC_UINT32 dpp_dtb_dump_table_overtime_get(ZXIC_VOID)
{
    return g_dtb_dump_overtime;
}

ZXIC_VOID dpp_dtb_overtime_prt(ZXIC_VOID)
{
    ZXIC_COMM_PRINT("g_dtb_down_overtime:%u\n", dpp_dtb_down_table_overtime_get());
    ZXIC_COMM_PRINT("g_dtb_dump_overtime:%u\n", dpp_dtb_dump_table_overtime_get());
}

static ZXIC_UINT32 dpp_dtb_get_completion_timeout_value(struct pci_dev *p_dev, ZXIC_UINT16 *p_timeout)
{
    ZXIC_SINT32 pcie_cap = 0;
    ZXIC_UINT16 data = 0;

    pcie_cap = pci_find_capability(p_dev, PCI_CAP_ID_EXP);
    if (!pcie_cap)
    {
        ZXIC_COMM_TRACE_ERROR("Can not find PCI Express CAP!\n");
        return DPP_ERR;
    }

    pci_read_config_word(p_dev, pcie_cap + PCI_EXP_DEVCTL2, &data);
    
    *p_timeout = data & PCI_EXP_DEVCTL2_COMP_TIMEOUT;
    
    return DPP_OK;
}

ZXIC_UINT32 dpp_dtb_base_timeout_cal(DPP_DEV_T *dev)
{
    DPP_STATUS rc = 0;
    ZXIC_UINT32 base_time = 50; //50ms
    ZXIC_UINT16 cpl_timeout = 0;

    ZXIC_COMM_CHECK_POINT(dev);

    rc = dpp_dtb_get_completion_timeout_value(DEV_PCIE_DEV(dev), &cpl_timeout);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_get_completion_timeout_value");

    if(cpl_timeout < 0x5)
    {
        base_time = 50;
    }
    else if(cpl_timeout == 0x5)
    {
        base_time = 55;
    }
    else
    {
        base_time = 210;
    }

    if(g_dtb_base_timeout < base_time)
    {
        g_dtb_base_timeout = base_time;
    }

    dpp_dtb_down_table_overtime_set(4 * g_dtb_base_timeout * 1000);

    return DPP_OK;
}

#if ZXIC_REAL("MGR")
/***********************************************************/
/** 创建DTB的管理结构
* @param   dev_id       设备号，支持多芯片  
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_mgr_create(ZXIC_UINT32 slot_id, ZXIC_UINT32 dev_id)
{
    ZXIC_COMM_CHECK_INDEX(slot_id, 0, DPP_PCIE_SLOT_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    
    if (p_dpp_dtb_mgr[slot_id][dev_id] != ZXIC_NULL)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "slot %d ErrorCode[0x%x]: DTB Manager is not exist!!!\n", slot_id, DPP_RC_DTB_MGR_NOT_EXIST);
        return DPP_RC_DTB_MGR_EXIST;
    }
    
    p_dpp_dtb_mgr[slot_id][dev_id] = (DPP_DTB_MGR_T *)ZXIC_COMM_MALLOC(sizeof(DPP_DTB_MGR_T));
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dpp_dtb_mgr[slot_id][dev_id]);
    
    ZXIC_COMM_MEMSET(p_dpp_dtb_mgr[slot_id][dev_id], 0, sizeof(DPP_DTB_MGR_T));

    ZXIC_COMM_TRACE_NOTICE("dpp_dtb_mgr_create:slot %d dev %d done!!!", slot_id, dev_id);
    
    return DPP_OK;
}

/***********************************************************/
/** 注销DTB的管理结构
* @param   dev_id       设备号，支持多芯片   
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_mgr_destory_all(ZXIC_VOID)
{
    ZXIC_UINT32 slot_id = 0;
    ZXIC_UINT32 dev_id = 0;
    
    for(slot_id = 0; slot_id < DPP_DEV_SLOT_MAX; slot_id++)
    {
        for(dev_id = 0; dev_id < DPP_DEV_CHANNEL_MAX; dev_id++)
        {
            dpp_dtb_mgr_destory(slot_id, dev_id);
        }
    }
    
    return DPP_OK;
}

ZXIC_UINT32 dpp_dtb_mgr_destory(ZXIC_UINT32 slot_id, ZXIC_UINT32 dev_id)
{
    ZXIC_COMM_CHECK_INDEX(slot_id, 0, DPP_PCIE_SLOT_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    
    if (p_dpp_dtb_mgr[slot_id][dev_id] != ZXIC_NULL)
    {
        ZXIC_COMM_FREE(p_dpp_dtb_mgr[slot_id][dev_id]);
        p_dpp_dtb_mgr[slot_id][dev_id] = ZXIC_NULL;
        ZXIC_COMM_TRACE_NOTICE("slot %d dev %d done!!!", slot_id, dev_id);
    }
    
    return DPP_OK;
}

/***********************************************************/
/** 重置DTB管理结构
* @param   dev_id       设备号，支持多芯片   
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_mgr_reset(ZXIC_UINT32 slot_id, ZXIC_UINT32 dev_id)
{
    ZXIC_COMM_CHECK_INDEX(slot_id, 0, DPP_PCIE_SLOT_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    
    if (p_dpp_dtb_mgr[slot_id][dev_id] == ZXIC_NULL)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "slot %d ErrorCode[0x%x]: dtb manager is not exist!!!\n", slot_id, DPP_RC_DTB_MGR_NOT_EXIST);
        return DPP_RC_DTB_MGR_NOT_EXIST;
    }

    ZXIC_COMM_MEMSET(p_dpp_dtb_mgr[slot_id][dev_id], 0, sizeof(DPP_DTB_MGR_T));
    
    return DPP_OK;
}

/***********************************************************/
/** 获取DMA管理结构
* @param   dev_id               设备号，支持多芯片
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
DPP_DTB_MGR_T *dpp_dtb_mgr_get(ZXIC_UINT32 slot_id, ZXIC_UINT32 dev_id)
{
    if ((slot_id >= DPP_PCIE_SLOT_MAX) ||  (dev_id >= DPP_DEV_CHANNEL_MAX))
    {
        return ZXIC_NULL;
    }
    else
    {
        return p_dpp_dtb_mgr[slot_id][dev_id];
    }
}
#endif

#if ZXIC_REAL("QUEUE_ADDR")
/***********************************************************/
/** 获得下表队列中某元素的物理地址
* @param   dev_id                         设备号，支持多芯片  
* @param   queue_id                       队列号0-127
* @param   element_id                     队列中元素号0-31
* @param   p_element_start_addr_h         元素起始高32位地址
* @param   p_element_start_addr_l         元素起始低32位地址
* @param   p_element_table_addr_h         下表内容开始高32位地址
* @param   p_element_table_addr_l         下表内容开始低32位地址
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_down_table_elemet_addr_get(DPP_DEV_T *dev, 
                                              ZXIC_UINT32 queue_id, 
                                              ZXIC_UINT32 element_id,
                                              ZXIC_UINT32 *p_element_start_addr_h,
                                              ZXIC_UINT32 *p_element_start_addr_l,
                                              ZXIC_UINT32 *p_element_table_addr_h,
                                              ZXIC_UINT32 *p_element_table_addr_l)
{
    ZXIC_UINT32 addr_h = 0;
    ZXIC_UINT32 addr_l = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), element_id, 0, DPP_DTB_QUEUE_ITEM_NUM_MAX - 1);

    addr_h = (DPP_DTB_TAB_DOWN_PHY_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, element_id) >> 32) & 0xffffffff;
    addr_l = DPP_DTB_TAB_DOWN_PHY_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, element_id) & 0xffffffff;

    *p_element_start_addr_h = addr_h;
    *p_element_start_addr_l = addr_l;

    addr_h = ((DPP_DTB_TAB_DOWN_PHY_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, element_id) + DPP_DTB_ITEM_ACK_SIZE) >> 32) & 0xffffffff;
    addr_l = (DPP_DTB_TAB_DOWN_PHY_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, element_id) + DPP_DTB_ITEM_ACK_SIZE) & 0xffffffff;

    *p_element_table_addr_h = addr_h;
    *p_element_table_addr_l = addr_l;

    return DPP_OK;
} 

/***********************************************************/
/** 获得队列中某元素dump的物理地址
* @param   dev_id                         设备号，支持多芯片  
* @param   queue_id                       队列号0-127
* @param   element_id                     队列中元素号0-31
* @param   p_element_start_addr_h         元素起始高32位地址
* @param   p_element_start_addr_l         元素起始低32位地址
* @param   p_element_dump_addr_h          dump描述符开始高32位地址
* @param   p_element_dump_addr_l          dump描述符开始低32位地址
* @param   p_element_table_info_addr_h    表内容开始高32位地址
* @param   p_element_table_info_addr_l    表内容开始低32位地址
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_dump_table_elemet_addr_get(DPP_DEV_T *dev, 
                                               ZXIC_UINT32 queue_id, 
                                               ZXIC_UINT32 element_id,
                                               ZXIC_UINT32 *p_element_start_addr_h,
                                               ZXIC_UINT32 *p_element_start_addr_l,
                                               ZXIC_UINT32 *p_element_dump_addr_h,
                                               ZXIC_UINT32 *p_element_dump_addr_l,
                                               ZXIC_UINT32 *p_element_table_info_addr_h,
                                               ZXIC_UINT32 *p_element_table_info_addr_l)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 addr_h = 0;
    ZXIC_UINT32 addr_l = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), element_id, 0, DPP_DTB_QUEUE_ITEM_NUM_MAX - 1);

    addr_h = ((DPP_DTB_TAB_UP_PHY_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, element_id)) >> 32) & 0xffffffff;
    addr_l = (DPP_DTB_TAB_UP_PHY_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, element_id)) & 0xffffffff;

    *p_element_start_addr_h = addr_h;
    *p_element_start_addr_l = addr_l;

    addr_h = ((DPP_DTB_TAB_UP_PHY_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, element_id)+ DPP_DTB_ITEM_ACK_SIZE) >> 32) & 0xffffffff;
    addr_l = (DPP_DTB_TAB_UP_PHY_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, element_id)+ DPP_DTB_ITEM_ACK_SIZE) & 0xffffffff;

    *p_element_dump_addr_h = addr_h;
    *p_element_dump_addr_l = addr_l;

    rc = dpp_dtb_tab_up_item_addr_get(dev, queue_id, element_id, p_element_table_info_addr_h, p_element_table_info_addr_l);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_tab_up_item_addr_get");

    return DPP_OK;
}

#endif

#if ZXIC_REAL("MEM_RW")
/***********************************************************/
/** 内存32bits写入函数
* @param   dev_id           设备号，支持多芯片  
* @param   addr             地址偏移
* @param   data             写入数据
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_wr32(DPP_DEV_T *dev, 
                        ZXIC_ADDR_T addr, 
                        ZXIC_UINT32 data)  
{
    ZXIC_UINT32 value = data;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), data, 0, ZXIC_UINT32_MAX);
    
    if (IS_ERR_OR_NULL((ZXIC_UINT32 *)addr))
    {
        ZXIC_COMM_TRACE_ERROR("slot %d vport: 0x%04x virAddr 0x%llx invalid!!!\n",DEV_PCIE_SLOT(dev), DEV_PCIE_VPORT(dev), addr);
        return DPP_RC_DTB_ADDR_INVALID;
    }

    if (!zxic_comm_is_big_endian()) 
    {
        value = ZXIC_COMM_CONVERT32(value);
    }
    
    *((ZXIC_VOL ZXIC_UINT32 *)(addr)) = value;
    
    return DPP_OK;
}

/***********************************************************/
/** 内存32bits读取函数
* @param   dev_id           设备号，支持多芯片  
* @param   addr             读取地址
* @param   p_data           读取数据
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_rd32(DPP_DEV_T *dev,
                        ZXIC_ADDR_T addr, 
                        ZXIC_UINT32 *p_data)      
{
    ZXIC_UINT32 value = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);

    if (IS_ERR_OR_NULL((ZXIC_UINT32 *)addr))
    {
        ZXIC_COMM_TRACE_ERROR("slot %d vport: 0x%04x virAddr 0x%llx invalid!!!\n",DEV_PCIE_SLOT(dev), DEV_PCIE_VPORT(dev), addr);
        return DPP_RC_DTB_ADDR_INVALID;
    }

    // dpp_flush_cache();//片内DDR使用

    value = *((ZXIC_VOL ZXIC_UINT32 *)(addr));

    if (!zxic_comm_is_big_endian())
    {
        value = ZXIC_COMM_CONVERT32(value);
    }

    *p_data = value;

    return DPP_OK;
}
#endif

/***********************************************************/
/** 检查dev设备的dtb队列是否在工作状态
* @param   dev       设备 
* @return  
* @remark  无
* @see     
* @author  cbb      @date  2026/06/25
************************************************************/
ZXIC_UINT32 dpp_dtb_dev_status_check(DPP_DEV_T *dev)
{
    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);
    
    if((!(dev->pcie_channel.dev_status)) || (!dtb_table_function_switch_get()))
    {
        ZXIC_COMM_TRACE_NOTICE("slot[%u] vport[0x%x] dev status off!\n", dev->pcie_channel.slot, dev->pcie_channel.vport);
        return ZXIC_PAR_CHK_DEV_STATUS_OFF;
    }

    return DPP_OK;
}

#if ZXIC_REAL("ACK_RW")
/***********************************************************/
/** 读取BD表条目信息
* @param   dev_id       芯片的id号  
* @param   queue_id     队列号,范围0-127
* @param   dir_flag     方向,1-上送表项,0-下发表项    
* @param   index        条目索引,范围0-31 
* @param   pos          一个item里面的4个32位，pos对应的是第几个ZXICP_WORD32，
*                       取值为0，1，2，3      
* @param   p_data       读取的数据,大端格式  
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2019/11/2
************************************************************/
ZXIC_UINT32 dpp_dtb_item_ack_rd(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 dir_flag,
                    ZXIC_UINT32 index, 
                    ZXIC_UINT32 pos,
                    ZXIC_UINT32 *p_data)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_ADDR_T addr = 0;
    ZXIC_UINT32 val = 0;
    
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), dir_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), index, 0, DPP_DTB_QUEUE_ITEM_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), pos, 0, 3);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_PCIE_SLOT(dev), 0, DPP_PCIE_SLOT_MAX - 1);

    rc = dpp_dtb_dev_status_check(dev);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_dev_status_check");

    if(DPP_DTB_QUEUE_INIT_FLAG_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id) == 0)
    {
        ZXIC_COMM_TRACE_ERROR("dtb slot %d queue %d is not init.\n", DEV_PCIE_SLOT(dev), queue_id);
        return DPP_RC_DTB_QUEUE_IS_NOT_INIT;
    }

    if (dir_flag == DPP_DTB_DIR_UP_TYPE)
    {
        addr = DPP_DTB_TAB_UP_VIR_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, index) + pos * 4;
    }
    else
    {
        addr = DPP_DTB_TAB_DOWN_VIR_ADDR_GET(DEV_PCIE_SLOT(dev),DEV_ID(dev), queue_id, index) + pos * 4;
    }
    
    rc = dpp_dtb_rd32(dev, addr, &val);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_rd32");

    *p_data = val;

    return DPP_OK;
}

/***********************************************************/
/** 向BD表条目指定位置写入值
* @param   dev_id       芯片的id号  
* @param   queue_id     队列号,范围0-127
* @param   dir_flag     方向,1-上送表项,0-下发表项  
* @param   index        条目索引,范围0-31 
* @param   pos          一个item里面的4个32位，pos对应的是第几个ZXICP_WORD32，
*                       取值为0，1，2，3      
* @param   data         读取的数据,大端格式 
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2019/11/2
************************************************************/
ZXIC_UINT32 dpp_dtb_item_ack_wr(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 dir_flag,
                    ZXIC_UINT32 index, 
                    ZXIC_UINT32 pos,
                    ZXIC_UINT32 data)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_ADDR_T addr = 0;
    
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), dir_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), index, 0, DPP_DTB_QUEUE_ITEM_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), pos, 0, 3);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_PCIE_SLOT(dev), 0, DPP_PCIE_SLOT_MAX - 1);

    rc = dpp_dtb_dev_status_check(dev);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_dev_status_check");

    if(DPP_DTB_QUEUE_INIT_FLAG_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id) == 0)
    {
        ZXIC_COMM_TRACE_ERROR("dtb slot %d queue %d is not init.\n", DEV_PCIE_SLOT(dev), queue_id);
        return DPP_RC_DTB_QUEUE_IS_NOT_INIT;
    }

    if (dir_flag == DPP_DTB_DIR_UP_TYPE)
    {
        addr = DPP_DTB_TAB_UP_VIR_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, index) + pos * 4;
    }
    else
    {
        addr = DPP_DTB_TAB_DOWN_VIR_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, index) + pos * 4;
    }

    rc = dpp_dtb_wr32(dev, addr, data);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_wr32");

    return DPP_OK;
}

/***********************************************************/
/** 在dtb初始化中使用，直接读取BD表条目信息
* @param   dev_id       芯片的id号  
* @param   queue_id     队列号,范围0-127
* @param   dir_flag     方向,1-上送表项,0-下发表项    
* @param   index        条目索引,范围0-31 
* @param   pos          一个item里面的4个32位，pos对应的是第几个ZXICP_WORD32，
*                       取值为0，1，2，3      
* @param   p_data       读取的数据,大端格式  
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2019/11/2
************************************************************/
ZXIC_UINT32 dpp_dtb_init_item_ack_rd(DPP_DEV_T *dev,
                                    ZXIC_UINT32 queue_id,
                                    ZXIC_UINT32 dir_flag,
                                    ZXIC_UINT32 index, 
                                    ZXIC_UINT32 pos,
                                    ZXIC_UINT32 *p_data)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_ADDR_T addr = 0;
    ZXIC_UINT32 val = 0;
    
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), dir_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), index, 0, DPP_DTB_QUEUE_ITEM_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), pos, 0, 3);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);

    if (dir_flag == DPP_DTB_DIR_UP_TYPE)
    {
        addr = DPP_DTB_TAB_UP_VIR_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, index) + pos * 4;
    }
    else
    {
        addr = DPP_DTB_TAB_DOWN_VIR_ADDR_GET(DEV_PCIE_SLOT(dev),DEV_ID(dev), queue_id, index) + pos * 4;
    }
    
    rc = dpp_dtb_rd32(dev, addr, &val);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_rd32");

    *p_data = val;

    return DPP_OK;
}

/***********************************************************/
/** 在dtb初始化中使用，向BD表条目指定位置写入值
* @param   dev_id       芯片的id号  
* @param   queue_id     队列号,范围0-127
* @param   dir_flag     方向,1-上送表项,0-下发表项  
* @param   index        条目索引,范围0-31 
* @param   pos          一个item里面的4个32位，pos对应的是第几个ZXICP_WORD32，
*                       取值为0，1，2，3      
* @param   data         读取的数据,大端格式 
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2019/11/2
************************************************************/
ZXIC_UINT32 dpp_dtb_init_item_ack_wr(DPP_DEV_T *dev,
                                    ZXIC_UINT32 queue_id,
                                    ZXIC_UINT32 dir_flag,
                                    ZXIC_UINT32 index, 
                                    ZXIC_UINT32 pos,
                                    ZXIC_UINT32 data)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_ADDR_T addr = 0;
    
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), dir_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), index, 0, DPP_DTB_QUEUE_ITEM_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), pos, 0, 3);

    if (dir_flag == DPP_DTB_DIR_UP_TYPE)
    {
        addr = DPP_DTB_TAB_UP_VIR_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, index) + pos * 4;
    }
    else
    {
        addr = DPP_DTB_TAB_DOWN_VIR_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, index) + pos * 4;
    }

    rc = dpp_dtb_wr32(dev, addr, data);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_wr32");

    return DPP_OK;
}
/***********************************************************/
/** 打印队列指定条目ACK信息
* @param   dev_id       芯片的id号  
* @param   queue_id     队列号，范围0-127  
* @param   dir_flag     方向,0-down,1-up    
* @param   index        条目索引 
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2022/08/30
************************************************************/
ZXIC_UINT32 dpp_dtb_item_ack_prt(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 dir_flag,
                    ZXIC_UINT32 index)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 ack_data[4] = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), dir_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), index, 0, DPP_DTB_QUEUE_ITEM_NUM_MAX -1);

    for (i = 0; i < DPP_DTB_ITEM_ACK_SIZE / 4; i++)
    {
         rc = dpp_dtb_item_ack_rd(dev, queue_id, dir_flag, index, i, ack_data + i);
         ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_item_ack_rd");
    }
    
    ZXIC_COMM_PRINT("\n=====> [%s] BD INFO:", g_dpp_dtb_name[dir_flag]);
    ZXIC_COMM_PRINT("\n[ index : %u] : 0x%08x 0x%08x 0x%08x 0x%08x \n", index, ack_data[0], ack_data[1], ack_data[2], ack_data[3]);

    return DPP_OK;
}

/***********************************************************/
/** 打印队列指定条目BUFF的数据
* @param   dev_id       芯片的id号  
* @param   queue_id     队列号，范围0-127   
* @param   dir_flag     方向,0-down,1-up    
* @param   index        条目索引  
* @param   len          读取数据长度 4字节为单位 
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2022/08/30
************************************************************/
ZXIC_UINT32 dpp_dtb_item_buff_prt(DPP_DEV_T *dev,
                                  ZXIC_UINT32 queue_id,
                                  ZXIC_UINT32 dir_flag,
                                  ZXIC_UINT32 index, 
                                  ZXIC_UINT32 len)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 *p_item_buff = ZXIC_NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), dir_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), index, 0, DPP_DTB_QUEUE_ITEM_NUM_MAX -1);

    p_item_buff = ZXIC_COMM_MALLOC((len * sizeof(ZXIC_UINT32)) % ZXIC_COMM_WORD32_MASK);
    if (p_item_buff == ZXIC_NULL)
    {
        ZXIC_COMM_PRINT("Alloc dtb item buffer faild!!!\n");
        return DPP_RC_DTB_MEMORY_ALLOC_ERR;
    }

    ZXIC_COMM_MEMSET(p_item_buff, 0, len * sizeof(ZXIC_UINT32));

    rc = dpp_dtb_item_buff_rd(dev, queue_id, dir_flag, index, 0, len, p_item_buff);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE(DEV_ID(dev), rc, "dpp_dtb_item_buff_rd", p_item_buff);
    
    ZXIC_COMM_PRINT("\n=====> [%s] BUFF INFO:", g_dpp_dtb_name[dir_flag]);
    for(i = 0, j = 0; i < len; i++, j++)
    {
        if(j % 4 == 0)
        { 
            ZXIC_COMM_PRINT("\n0x%08x ", (*(p_item_buff + i)));                                   
        }
        else
        {
            ZXIC_COMM_PRINT("0x%08x ", (*(p_item_buff + i)));
        }
    }
    ZXIC_COMM_PRINT("\n");

    ZXIC_COMM_FREE(p_item_buff);

    return DPP_OK;
}

#endif


#if ZXIC_REAL("BUFF_RW")

/***********************************************************/
/** 读取dtb条目指向BUFF的数据
* @param   dev_id       芯片的id号  
* @param   queue_id     队列号,范围0-127
* @param   dir_flag     方向,1-上送表项,0-下发表项  
* @param   index        条目索引,范围0-31 
* @param   pos          相对BUFF起始地址的偏移,单位32bit;      
* @param   p_data       读取的数据,大端格式 
* @param   len          读取数据长度,单位32bit; 
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2019/11/2
************************************************************/
ZXIC_UINT32 dpp_dtb_item_buff_rd(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 dir_flag,
                    ZXIC_UINT32 index, 
                    ZXIC_UINT32 pos,
                    ZXIC_UINT32 len,
                    ZXIC_UINT32 *p_data)
{
    ZXIC_ADDR_T addr = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), dir_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), index, 0, DPP_DTB_QUEUE_ITEM_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), pos, 0, 3);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);

    if(DPP_DTB_QUEUE_INIT_FLAG_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id) == 0)
    {
        ZXIC_COMM_TRACE_ERROR("dtb slot %d queue %d is not init.\n", DEV_PCIE_SLOT(dev), queue_id);
        return DPP_RC_DTB_QUEUE_IS_NOT_INIT;
    }

    if (dir_flag == DPP_DTB_DIR_UP_TYPE)
    {
        if (DPP_DTB_TAB_UP_USER_PHY_ADDR_FLAG_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, index) == DPP_DTB_TAB_UP_USER_ADDR_TYPE)
        {
            addr = DPP_DTB_TAB_UP_USER_VIR_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, index) + pos * 4;
            DPP_DTB_TAB_UP_USER_ADDR_FLAG_SET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, index, 0);//为什么设置为0？
        }
        else
        {
            addr = DPP_DTB_TAB_UP_VIR_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, index) + DPP_DTB_ITEM_ACK_SIZE + pos * 4;
        }
    }
    else
    {
        addr = DPP_DTB_TAB_DOWN_VIR_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, index) + DPP_DTB_ITEM_ACK_SIZE + pos * 4;
    }

    ZXIC_COMM_MEMCPY_S(p_data, len * 4, (ZXIC_UINT8 *)(addr), len * 4);

    zxic_comm_swap((ZXIC_UINT8*)p_data, len * 4);

    return DPP_OK;
}

/***********************************************************/
/** 向BD表条目指向的BUFF指定位置写入值
* @param   dev_id       芯片的id号  
* @param   queue_id     队列号,范围0-127
* @param   dir_flag     方向,1-上送表项,0-下发表项  
* @param   index        条目索引,范围0-31   
* @param   pos          相对BUFF起始地址的偏移,单位32bit;       
* @param   p_data       读取的数据,大端格式 
* @param   len          写入数据长度,单位32bit; 
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2019/11/2
************************************************************/
ZXIC_UINT32 dpp_dtb_item_buff_wr(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 dir_flag,
                    ZXIC_UINT32 index, 
                    ZXIC_UINT32 pos,
                    ZXIC_UINT32 len,
                    ZXIC_UINT32 *p_data)
{
    ZXIC_ADDR_T addr = 0;
    
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), dir_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), index, 0, DPP_DTB_QUEUE_ITEM_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), pos, 0, 3);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);

    if(DPP_DTB_QUEUE_INIT_FLAG_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id) == 0)
    {
        ZXIC_COMM_TRACE_ERROR("dtb slot %d queue %d is not init.\n", DEV_PCIE_SLOT(dev), queue_id);
        return DPP_RC_DTB_QUEUE_IS_NOT_INIT;
    }

    if (dir_flag == DPP_DTB_DIR_UP_TYPE)
    {
        addr = DPP_DTB_TAB_UP_VIR_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, index) + DPP_DTB_ITEM_ACK_SIZE + pos * 4;
    }
    else
    {
        addr = DPP_DTB_TAB_DOWN_VIR_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, index) + DPP_DTB_ITEM_ACK_SIZE + pos * 4;
    }

    ZXIC_COMM_MEMCPY_S((ZXIC_UINT8 *)(addr), len * 4, p_data, len * 4);

    // dpp_flush_cache();//片内ddr使用

    return DPP_OK;
}
#endif

#if ZXIC_REAL("API")
#if ZXIC_REAL("TAB_DOWN")

/** dtb info print*/
ZXIC_UINT32 dpp_dtb_info_print(DPP_DEV_T *dev, 
                               ZXIC_UINT32 queue_id, 
                               ZXIC_UINT32 item_index,
                               DPP_DTB_QUEUE_ITEM_INFO_T *item_info)
{
    ZXIC_ADDR_T element_start_addr = 0;
    ZXIC_ADDR_T ack_start_addr = 0;
    ZXIC_ADDR_T data_addr = 0;
    ZXIC_UINT32 i = 0;

    ZXIC_COMM_CHECK_POINT(dev);

    ZXIC_COMM_PRINT("dpp_dtb_info_print: slot %d queue: %d, element:%d,  %s table info is:\n", DEV_PCIE_SLOT(dev), queue_id, item_index,
                                        (item_info->cmd_type) ? "up" : "down");
    ZXIC_COMM_PRINT("cmd_vld    : %d\n", item_info->cmd_vld);
    ZXIC_COMM_PRINT("cmd_type   : %s\n", (item_info->cmd_type) ? "up" : "down");
    ZXIC_COMM_PRINT("int_en     : %d\n", item_info->int_en);
    ZXIC_COMM_PRINT("data_len   : %d\n", item_info->data_len);
    ZXIC_COMM_PRINT("data_hddr  : 0x%08x\n", item_info->data_hddr);
    ZXIC_COMM_PRINT("data_laddr : 0x%08x\n", item_info->data_laddr);

    if (item_info->cmd_type == DPP_DTB_DIR_UP_TYPE)
    {
        if (DPP_DTB_TAB_UP_USER_PHY_ADDR_FLAG_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, item_index) == DPP_DTB_TAB_UP_USER_ADDR_TYPE)
        {
            ack_start_addr = DPP_DTB_TAB_UP_USER_VIR_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, item_index);
        }
        ack_start_addr = DPP_DTB_TAB_UP_VIR_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, item_index);
        element_start_addr = DPP_DTB_TAB_UP_VIR_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, item_index) + DPP_DTB_ITEM_ACK_SIZE;
    }
    else
    {
        ack_start_addr = DPP_DTB_TAB_DOWN_VIR_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, item_index);
        element_start_addr = DPP_DTB_TAB_DOWN_VIR_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, item_index) + DPP_DTB_ITEM_ACK_SIZE;
    }
    ZXIC_COMM_PRINT("dtb data:\n");

    ZXIC_COMM_PRINT("ack info: 0x%08x 0x%08x 0x%08x 0x%08x \n", 
                    ZXIC_COMM_CONVERT32(*((ZXIC_UINT32 *)(ack_start_addr + 4 * 0))),
                    ZXIC_COMM_CONVERT32(*((ZXIC_UINT32 *)(ack_start_addr + 4 * 1))),
                    ZXIC_COMM_CONVERT32(*((ZXIC_UINT32 *)(ack_start_addr + 4 * 2))),
                    ZXIC_COMM_CONVERT32(*((ZXIC_UINT32 *)(ack_start_addr + 4 * 3))));
    
    for (i = 0; i < item_info->data_len; i++)
    { 
        data_addr = element_start_addr + 16 * i;//16字节为一行

        ZXIC_COMM_PRINT("row_%d:", i);
        ZXIC_COMM_PRINT("0x%08x 0x%08x 0x%08x 0x%08x ",
                        ZXIC_COMM_CONVERT32(*((ZXIC_UINT32 *)(data_addr + 4 * 0))),
                        ZXIC_COMM_CONVERT32(*((ZXIC_UINT32 *)(data_addr + 4 * 1))),
                        ZXIC_COMM_CONVERT32(*((ZXIC_UINT32 *)(data_addr + 4 * 2))),
                        ZXIC_COMM_CONVERT32(*((ZXIC_UINT32 *)(data_addr + 4 * 3))
                        ));

        ZXIC_COMM_PRINT("\n");
    }

    ZXIC_COMM_PRINT("dpp dtb info print end.\n");
    return DPP_OK;
}

/***********************************************************/
/** 配置下发配置数据信息
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列号，范围0-31 
* @param   int_flag             中断标志，0-无，1-有 
* @param   data_len             数据长度，单位32bit;
* @param   p_data               待下发数据 
* @param   p_item_index         返回使用的条目编号 
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_tab_down_info_set(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 int_flag,
                    ZXIC_UINT32 data_len,
                    ZXIC_UINT32 *p_data,
                    ZXIC_UINT32 *p_item_index)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 queue_en = 0;
    ZXIC_UINT32 ack_value = 0;
    ZXIC_UINT32 item_index = 0;
    ZXIC_UINT32 unused_item_num = 0;
    DPP_DTB_QUEUE_ITEM_INFO_T item_info = {0};
    ZXIC_MUTEX_T *p_mutex = NULL;
    ZXIC_UINT32 ack_rst = 0;
    
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), int_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), data_len, 4, 0xffc);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_item_index);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_PCIE_SLOT(dev), 0, DPP_PCIE_SLOT_MAX - 1);

    rc = dpp_dtb_dev_status_check(dev);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_dev_status_check");

    if(DPP_DTB_QUEUE_INIT_FLAG_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id) == 0)
    {
        ZXIC_COMM_TRACE_ERROR("dtb slot %d queue %d is not init.\n", DEV_PCIE_SLOT(dev), queue_id);
        return DPP_RC_DTB_QUEUE_IS_NOT_INIT;
    }

    rc = dpp_dev_dtb_opr_mutex_get(dev, DPP_DEV_MUTEX_T_DTB, queue_id, &p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dev_dtb_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev),rc,"zxic_comm_mutex_lock");

    /*
     * 流程
     * 0.检测队列是否使能
     * 1.检测当前队列是否已初始化;
     * 2.获取硬件队列剩余情况，大于0正常下发，等于0返回失败；
     * 3.获取软件缓存空闲情况；
     * 4.将下表配置数据填入buff中；
     * 5.将ack字段填入0x11111100;
     * 6.将数据信息填入硬件触发寄存器中;
     *
     */
#if 0
    if(dpp_dtb_mode_is_debug(dev))
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "the queue %d is open debug mode!", queue_id);
        rc = zxic_comm_mutex_unlock(p_mutex);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
        return DPP_RC_DTB_OPEN_DEBUG_MODE;
    }
#endif

    rc = dpp_dtb_queue_enable_get(dev, queue_id, &queue_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_dtb_queue_enable_get", p_mutex);
    if(!queue_en)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "the slot %d queue %d is not enable!", DEV_PCIE_SLOT(dev), queue_id);
        rc = zxic_comm_mutex_unlock(p_mutex);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev),rc,"zxic_comm_mutex_unlock");
        return DPP_RC_DTB_QUEUE_NOT_ENABLE;
    }

    if (data_len % 4 != 0)
    {   
        rc = zxic_comm_mutex_unlock(p_mutex);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev),rc,"zxic_comm_mutex_unlock");
        /* 硬件规定数据必须是16字节为单位 */
        return DPP_RC_DTB_PARA_INVALID;
    }

    rc = dpp_dtb_queue_unused_item_num_get(dev, queue_id, &unused_item_num);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_dtb_queue_unused_item_num_get", p_mutex);

    if (unused_item_num == 0)
    {
        rc = zxic_comm_mutex_unlock(p_mutex);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev),rc,"zxic_comm_mutex_unlock");
        return DPP_RC_DTB_QUEUE_ITEM_HW_EMPTY;
    }

    for (i = 0; i < DPP_DTB_QUEUE_ITEM_NUM_MAX; i++)
    {
        item_index = DPP_DTB_TAB_DOWN_WR_INDEX_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id) % DPP_DTB_QUEUE_ITEM_NUM_MAX;
            
        rc = dpp_dtb_item_ack_rd(dev, queue_id, DPP_DTB_DIR_DOWN_TYPE, item_index, 0, &ack_value);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_dtb_item_ack_rd", p_mutex);

        DPP_DTB_TAB_DOWN_WR_INDEX_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id)++;

        ack_rst = (ack_value >> 8) & 0xFFFFFF;

        if (ack_rst == DPP_DTB_TAB_ACK_UNUSED_MASK || ack_rst == DPP_DTB_TAB_DOWN_ACK_VLD_MASK)
        {
            break;
        }
    }

    if (i == DPP_DTB_QUEUE_ITEM_NUM_MAX)
    {
        rc = zxic_comm_mutex_unlock(p_mutex);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev),rc,"zxic_comm_mutex_unlock");
        return DPP_RC_DTB_QUEUE_ITEM_SW_EMPTY;
    }
    
    rc = dpp_dtb_item_buff_wr(dev, queue_id, DPP_DTB_DIR_DOWN_TYPE, item_index, 0, data_len, p_data);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_dtb_item_buff_wr", p_mutex);

    rc = dpp_dtb_item_ack_wr(dev, queue_id, DPP_DTB_DIR_DOWN_TYPE, item_index, 0, DPP_DTB_TAB_ACK_IS_USING_MASK);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_dtb_item_ack_wr", p_mutex);

    item_info.cmd_vld = 1;
    item_info.cmd_type = DPP_DTB_DIR_DOWN_TYPE;
    item_info.int_en = int_flag;
    item_info.data_len = data_len / 4;
    item_info.data_hddr = ((DPP_DTB_TAB_DOWN_PHY_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, item_index)>>4)>>32) & 0xffffffff;
    item_info.data_laddr = (DPP_DTB_TAB_DOWN_PHY_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, item_index)>>4) & 0xffffffff;

    if(item_info.data_len < DPP_DTB_LEN_MIN || item_info.data_len > DPP_DTB_DOWN_LEN)
    {
        ZXIC_COMM_PRINT("DTB DATA_LEN :0x%08x.\n",item_info.data_len);
        rc = dpp_dtb_item_ack_wr(dev, queue_id, DPP_DTB_DIR_DOWN_TYPE, item_index, 0, DPP_DTB_TAB_ACK_UNUSED_MASK);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_item_ack_wr");
        rc = zxic_comm_mutex_unlock(p_mutex);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev),rc,"zxic_comm_mutex_unlock");
        return DPP_RC_DTB_PARA_INVALID;
    }

    if(dpp_dtb_prt_get())
    {
        dpp_dtb_info_print(dev, queue_id, item_index,  &item_info);
    }

    if(dpp_dtb_soft_perf_test_get())
    {
        *p_item_index = item_index;
        rc = zxic_comm_mutex_unlock(p_mutex);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev),rc,"zxic_comm_mutex_unlock");
        return DPP_OK;
    }

    rc = dpp_dtb_queue_item_info_set(dev, queue_id, &item_info);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_dtb_queue_item_info_set", p_mutex);
    *p_item_index = item_index;

    rc = zxic_comm_mutex_unlock(p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev),rc,"zxic_comm_mutex_unlock");
    
    return DPP_OK;
}

/***********************************************************/
/** 打印下表队列中指定元素的地址、ACK及下表中前768bit的数据
* @param   dev_id       芯片的id号  
* @param   queue_id     队列号，范围0-127    
* @param   element_id   元素号，范围0-31  
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2022/08/30
************************************************************/
ZXIC_UINT32 dpp_dtb_down_table_element_info_prt(DPP_DEV_T *dev,
                                                ZXIC_UINT32 queue_id,
                                                ZXIC_UINT32 element_id)
{
    ZXIC_UINT32 rc = 0;

    ZXIC_UINT32 element_start_addr_h = 0;
    ZXIC_UINT32 element_start_addr_l = 0;
    ZXIC_UINT32 element_table_addr_h = 0;
    ZXIC_UINT32 element_table_addr_l = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), element_id, 0, DPP_DTB_QUEUE_ITEM_NUM_MAX - 1);

    rc = dpp_dtb_down_table_elemet_addr_get(dev, 
                                queue_id, 
                                element_id, 
                                &element_start_addr_h, 
                                &element_start_addr_l,
                                &element_table_addr_h, 
                                &element_table_addr_l);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_elemet_addr_get");

    ZXIC_COMM_DBGCNT32_PRINT("slot_id", DEV_PCIE_SLOT(dev));
    ZXIC_COMM_DBGCNT32_PRINT("queue_id", queue_id);
    ZXIC_COMM_DBGCNT32_PRINT("element_id", element_id);
    ZXIC_COMM_DBGCNT32_PRINT("element_start_addr_h", element_start_addr_h);
    ZXIC_COMM_DBGCNT32_PRINT("element_start_addr_l", element_start_addr_l);
    ZXIC_COMM_DBGCNT32_PRINT("element_table_addr_h", element_table_addr_h);
    ZXIC_COMM_DBGCNT32_PRINT("element_table_addr_l", element_table_addr_l);
    
    /*打印element ack*/
    rc = dpp_dtb_item_ack_prt(dev, queue_id, DPP_DTB_DIR_DOWN_TYPE, element_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_item_ack_prt");

    rc = dpp_dtb_item_buff_prt(dev, queue_id, DPP_DTB_DIR_DOWN_TYPE, element_id, 24);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_item_buff_prt");

    return DPP_OK;
}

/***********************************************************/
/** 一个元素down成功状态检查
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列号，范围0-31 
* @param   element_id           条目编号 
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_tab_down_success_status_check(DPP_DEV_T *dev,
                                                  ZXIC_UINT32 queue_id,
                                                  ZXIC_UINT32 element_id)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 rd_cnt = 0;
    ZXIC_UINT32 ack_value = 0;
    ZXIC_UINT32 success_flag = 0;
    ZXIC_UINT32 dtb_interrupt_status = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), element_id, 0, DPP_DTB_QUEUE_ITEM_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_PCIE_SLOT(dev), 0, DPP_PCIE_SLOT_MAX - 1);

    dtb_interrupt_status = dpp_dtb_interrupt_status_get();

    rc = dpp_dtb_dev_status_check(dev);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_dev_status_check");

    if(DPP_DTB_QUEUE_INIT_FLAG_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id) == 0)
    {
        ZXIC_COMM_TRACE_ERROR("dtb slot %d queue %d is not init.\n", DEV_PCIE_SLOT(dev), queue_id);
        return DPP_RC_DTB_QUEUE_IS_NOT_INIT;
    }

    if(dpp_dtb_soft_perf_test_get() || dpp_dtb_hardware_perf_test_get())
    {
        rc = dpp_dtb_item_ack_wr(dev, queue_id, DPP_DTB_DIR_DOWN_TYPE, element_id, 0, DPP_DTB_TAB_ACK_UNUSED_MASK);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_item_ack_wr");
        return rc;
    }
    
    if(dpp_dtb_debug_fun_get())
    {
        return DPP_OK;
    }

    while(!success_flag)
    {
        rc = dpp_dtb_item_ack_rd(dev, queue_id, DPP_DTB_DIR_DOWN_TYPE, element_id, 0, &ack_value);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_item_ack_rd");

        ZXIC_COMM_TRACE_DEBUG("dpp_dtb_item_ack_rd ack_value:0x%08x\n", ack_value);

        if (((ack_value >> 8) & 0xffffff) == DPP_DTB_TAB_DOWN_ACK_VLD_MASK)
        {
            success_flag = 1;
            break;
        }

        if (rd_cnt > dpp_dtb_down_table_overtime_get())
        {
            ZXIC_COMM_TRACE_ERROR("Error!!! dpp dtb down slot [%d] vport [0x%x] queue [%d] item [%d] ack success is overtime!\n", DEV_PCIE_SLOT(dev), DEV_PCIE_VPORT(dev), queue_id, element_id);

            ZXIC_COMM_PRINT("-------------------------------------------------------------------\n");
            ZXIC_COMM_PRINT("                     dtb down table info                           \n");
            ZXIC_COMM_PRINT("-------------------------------------------------------------------\n");
            rc = dpp_dtb_down_table_element_info_prt(dev, queue_id, element_id);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_down_table_element_info_prt");

            rc = dpp_dtb_item_ack_wr(dev, queue_id, DPP_DTB_DIR_DOWN_TYPE, element_id, 0, DPP_DTB_TAB_ACK_UNUSED_MASK);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_item_ack_wr");

            ZXIC_COMM_PRINT("-------------------------------------------------------------------\n");
            ZXIC_COMM_PRINT("                          dtb reg info                             \n");
            ZXIC_COMM_PRINT("-------------------------------------------------------------------\n");

            rc = diag_dpp_dtb_axi_last_operate_info_prt(dev);
            ZXIC_COMM_CHECK_DEV_RC(0, rc, "diag_dpp_dtb_axi_last_operate_info_prt");

            rc = diag_dpp_dtb_channels_state_info_prt(dev);
            ZXIC_COMM_CHECK_DEV_RC(0, rc, "diag_dpp_dtb_channels_state_info_prt");

            rc = diag_dpp_dtb_channels_axi_resp_err_cnt_prt(dev);
            ZXIC_COMM_CHECK_DEV_RC(0, rc, "diag_dpp_dtb_channels_axi_resp_err_cnt_prt");

            return DPP_RC_DTB_OVER_TIME;
        }

        rd_cnt++;
        zxic_comm_udelay(1);
    }

    if (dtb_interrupt_status)
    {
        /*清中断*/
        rc = dpp_dtb_finish_interrupt_event_state_clr(dev, queue_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_finish_interrupt_event_state_clr");
    }

    if ((ack_value & 0xff) != DPP_DTB_TAB_ACK_SUCCESS_MASK)
    {
        rc = dpp_dtb_item_ack_wr(dev, queue_id, DPP_DTB_DIR_DOWN_TYPE, element_id, 0, DPP_DTB_TAB_ACK_UNUSED_MASK);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_item_ack_wr");
        return ack_value & 0xff;
    }

    rc = dpp_dtb_item_ack_wr(dev, queue_id, DPP_DTB_DIR_DOWN_TYPE, element_id, 0, DPP_DTB_TAB_ACK_UNUSED_MASK);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_item_ack_wr");

    return rc;
}

#endif
#if ZXIC_REAL("TAB_UP")
/***********************************************************/
/** dump队列空闲条目获取
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列号，范围0-31 
* @param   p_item_index         返回使用的条目编号 
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_tab_up_free_item_get(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 *p_item_index)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 ack_value = 0;
    ZXIC_UINT32 item_index = 0;
    ZXIC_UINT32 unused_item_num = 0;
    ZXIC_MUTEX_T *p_mutex = NULL;
    ZXIC_UINT32 ack_rst = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1); 
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_item_index);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_PCIE_SLOT(dev), 0, DPP_PCIE_SLOT_MAX - 1);

    rc = dpp_dtb_dev_status_check(dev);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_dev_status_check");

    if(DPP_DTB_QUEUE_INIT_FLAG_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id) == 0)
    {
        ZXIC_COMM_TRACE_ERROR("dtb slot %d queue %d is not init.\n", DEV_PCIE_SLOT(dev), queue_id);
        return DPP_RC_DTB_QUEUE_IS_NOT_INIT;
    }

    rc = dpp_dev_dtb_opr_mutex_get(dev, DPP_DEV_MUTEX_T_DTB, queue_id, &p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dev_dtb_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev),rc,"zxic_comm_mutex_lock");

    /*
     * 流程
     * 1.获取硬件队列剩余情况，大于0正常上送，等于0返回失败；
     * 2.获取软件缓存空闲情况；
     *
     */

    rc = dpp_dtb_queue_unused_item_num_get(dev, queue_id, &unused_item_num);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_dtb_queue_unused_item_num_get", p_mutex);

    if (unused_item_num == 0)
    {
        rc = zxic_comm_mutex_unlock(p_mutex);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev),rc,"zxic_comm_mutex_unlock");
        return DPP_RC_DTB_QUEUE_ITEM_HW_EMPTY;
    }

    for (i = 0; i < DPP_DTB_QUEUE_ITEM_NUM_MAX; i++)
    {
        item_index = DPP_DTB_TAB_UP_WR_INDEX_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id) % DPP_DTB_QUEUE_ITEM_NUM_MAX;
        
        rc = dpp_dtb_item_ack_rd(dev, queue_id, DPP_DTB_DIR_UP_TYPE, item_index, 0, &ack_value);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_dtb_item_ack_rd", p_mutex);

        DPP_DTB_TAB_UP_WR_INDEX_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id)++;

        ack_rst = (ack_value >> 8) & 0xFFFFFF;

        if (ack_rst == DPP_DTB_TAB_ACK_UNUSED_MASK || ack_rst == DPP_DTB_TAB_UP_ACK_VLD_MASK)
        {
            break;
        }
    }

    if (i == DPP_DTB_QUEUE_ITEM_NUM_MAX)
    {
        rc = zxic_comm_mutex_unlock(p_mutex);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev),rc,"zxic_comm_mutex_unlock");
        return DPP_RC_DTB_QUEUE_ITEM_SW_EMPTY;
    }

    rc = dpp_dtb_item_ack_wr(dev, queue_id, DPP_DTB_DIR_UP_TYPE, item_index, 0, DPP_DTB_TAB_ACK_IS_USING_MASK);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_dtb_item_buff_wr", p_mutex);

    *p_item_index = item_index;

    rc = zxic_comm_mutex_unlock(p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev),rc,"zxic_comm_mutex_unlock");
    
    return DPP_OK;
}

/***********************************************************/
/** 获取dump指定条目物理地址
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列编号,范围:0-127  
* @param   item_index           条目编号,范围0-31  
* @param   p_phy_haddr          物理地址高32bit  
* @param   p_phy_laddr          物理地址低32bit  
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_tab_up_item_addr_get(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 item_index,
                    ZXIC_UINT32 *p_phy_haddr,
                    ZXIC_UINT32 *p_phy_laddr)
{
    ZXIC_ADDR_T addr = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), item_index, 0, DPP_DTB_QUEUE_ITEM_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_phy_haddr);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_phy_laddr);

    if(DPP_DTB_QUEUE_INIT_FLAG_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id) == 0)
    {
        ZXIC_COMM_TRACE_ERROR("dtb slot %d queue %d is not init.\n", DEV_PCIE_SLOT(dev), queue_id);
        return DPP_RC_DTB_QUEUE_IS_NOT_INIT;
    }
    
    if (DPP_DTB_TAB_UP_USER_PHY_ADDR_FLAG_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, item_index) == DPP_DTB_TAB_UP_USER_ADDR_TYPE)
    {
        addr = DPP_DTB_TAB_UP_USER_PHY_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, item_index);
    }
    else
    {
        addr = DPP_DTB_TAB_UP_PHY_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, item_index) + DPP_DTB_ITEM_ACK_SIZE;
    }
    
    
    /*将地址转换成16字节为单位*/
    // addr = addr >> 4;

    *p_phy_haddr = (addr >> 32) & 0xffffffff;
    *p_phy_laddr = addr & 0xffffffff;

    return DPP_OK;
}

/***********************************************************/
/** 获取指定dump条目一定地址偏移的物理地址
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列编号,范围:0-127  
* @param   item_index           条目编号,范围0-31  
* @param   p_phy_haddr          物理地址高32bit  
* @param   p_phy_laddr          物理地址低32bit  
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_tab_up_item_offset_addr_get(DPP_DEV_T *dev,
                                                ZXIC_UINT32 queue_id,
                                                ZXIC_UINT32 item_index,
                                                ZXIC_UINT32 addr_offset,
                                                ZXIC_UINT32 *p_phy_haddr,
                                                ZXIC_UINT32 *p_phy_laddr)
{
    ZXIC_ADDR_T addr = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), item_index, 0, DPP_DTB_QUEUE_ITEM_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_phy_haddr);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_phy_laddr);

    if(DPP_DTB_QUEUE_INIT_FLAG_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id) == 0)
    {
        ZXIC_COMM_TRACE_ERROR("dtb slot %d queue %d is not init.\n", DEV_PCIE_SLOT(dev), queue_id);
        return DPP_RC_DTB_QUEUE_IS_NOT_INIT;
    }
    
    if (DPP_DTB_TAB_UP_USER_PHY_ADDR_FLAG_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, item_index) == DPP_DTB_TAB_UP_USER_ADDR_TYPE)
    {
        addr = DPP_DTB_TAB_UP_USER_PHY_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, item_index);
    }
    else
    {
        addr = DPP_DTB_TAB_UP_PHY_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, item_index) + DPP_DTB_ITEM_ACK_SIZE;
    }

    addr = addr + addr_offset;

    *p_phy_haddr = (addr >> 32) & 0xffffffff;
    *p_phy_laddr = addr & 0xffffffff;

    return DPP_OK;
}

/***********************************************************/
/** 设置dump指定条目空间地址，用于用户自定义空间传输
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列编号,范围:0-127  
* @param   item_index           条目编号,范围0-31  
* @param   phy_haddr            物理地址高  
* @param   vir_laddr            虚拟地址低  
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_tab_up_item_user_addr_set(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 item_index,
                    ZXIC_ADDR_T phy_addr,
                    ZXIC_ADDR_T vir_addr)
{
    DPP_DTB_MGR_T *p_dtb_mgr = ZXIC_NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), item_index, 0, DPP_DTB_QUEUE_ITEM_NUM_MAX - 1);

    p_dtb_mgr = dpp_dtb_mgr_get(DEV_PCIE_SLOT(dev), DEV_ID(dev));
    if (p_dtb_mgr == ZXIC_NULL)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "slot %d ErrorCode[0x%x]: DTB Manager is not exist!!!\n", DEV_PCIE_SLOT(dev), DPP_RC_DTB_MGR_NOT_EXIST);
        return DPP_RC_DTB_MGR_NOT_EXIST;
    }
    
    if(DPP_DTB_QUEUE_INIT_FLAG_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id) == 0)
    {
        ZXIC_COMM_TRACE_ERROR("dtb slot %d queue %d is not init.\n", DEV_PCIE_SLOT(dev), queue_id);
        return DPP_RC_DTB_QUEUE_IS_NOT_INIT;
    }

    p_dtb_mgr->queue_info[queue_id].tab_up.user_addr[item_index].phy_addr = phy_addr;
    p_dtb_mgr->queue_info[queue_id].tab_up.user_addr[item_index].vir_addr = vir_addr;
    p_dtb_mgr->queue_info[queue_id].tab_up.user_addr[item_index].user_flag = DPP_DTB_TAB_UP_USER_ADDR_TYPE;

    return DPP_OK;
}

/***********************************************************/
/** 清除用户dump指定条目空间地址
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列编号,范围:0-127  
* @param   item_index           条目编号,范围0-31   
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_tab_up_item_user_addr_clr(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 item_index)
{
    DPP_DTB_MGR_T *p_dtb_mgr = ZXIC_NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), item_index, 0, DPP_DTB_QUEUE_ITEM_NUM_MAX - 1);

    p_dtb_mgr = dpp_dtb_mgr_get(DEV_PCIE_SLOT(dev), DEV_ID(dev));
    if (p_dtb_mgr == ZXIC_NULL)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "slot %d ErrorCode[0x%x]: DTB Manager is not exist!!!\n", DEV_PCIE_SLOT(dev), DPP_RC_DTB_MGR_NOT_EXIST);
        return DPP_RC_DTB_MGR_NOT_EXIST;
    }
    
    if(DPP_DTB_QUEUE_INIT_FLAG_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id) == 0)
    {
        ZXIC_COMM_TRACE_ERROR("dtb slot %d queue %d is not init.\n", DEV_PCIE_SLOT(dev), queue_id);
        return DPP_RC_DTB_QUEUE_IS_NOT_INIT;
    }

    p_dtb_mgr->queue_info[queue_id].tab_up.user_addr[item_index].phy_addr = 0;
    p_dtb_mgr->queue_info[queue_id].tab_up.user_addr[item_index].vir_addr = 0;
    p_dtb_mgr->queue_info[queue_id].tab_up.user_addr[item_index].user_flag = DPP_DTB_TAB_UP_NOUSER_ADDR_TYPE;

    return DPP_OK;
}


/***********************************************************/
/** dump配置描述符信息设置
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列号，范围0-31 
* @param   item_index           返回使用的条目编号 
* @param   int_flag             中断标志，0-无，1-有 
* @param   data_len             数据长度，单位32bit;
* @param   desc_len             描述符长度，单位32bit;
* @param   p_desc_data          待下发描述符 
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_tab_up_info_set(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 item_index,
                    ZXIC_UINT32 int_flag,
                    ZXIC_UINT32 data_len,
                    ZXIC_UINT32 desc_len,
                    ZXIC_UINT32 *p_desc_data)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 queue_en = 0;
    DPP_DTB_QUEUE_ITEM_INFO_T item_info = {0};
    ZXIC_MUTEX_T *p_mutex = NULL;
    
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), item_index, 0, DPP_DTB_QUEUE_ITEM_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), desc_len, 0, 0x400);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), int_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_desc_data);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_PCIE_SLOT(dev), 0, DPP_PCIE_SLOT_MAX - 1);

    if(DPP_DTB_QUEUE_INIT_FLAG_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id) == 0)
    {
        ZXIC_COMM_TRACE_ERROR("dtb slot %d queue %d is not init.\n", DEV_PCIE_SLOT(dev), queue_id);
        return DPP_RC_DTB_QUEUE_IS_NOT_INIT;
    }

    rc = dpp_dev_dtb_opr_mutex_get(dev, DPP_DEV_MUTEX_T_DTB, queue_id, &p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dev_dtb_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev),rc,"zxic_comm_mutex_lock");

    /*
     * 流程
     * 0.检测队列是否使能
     * 1.获取硬件队列剩余情况，大于0正常下发，等于0返回失败；
     * 2.获取软件缓存空闲情况；
     * 3.将dump描述符写入buff中;
     * 4.将ack字段填入0x11111100;
     * 5.将数据信息填入硬件触发寄存器中;
     *
     */
#if 0
    if(dpp_dtb_mode_is_debug(dev))
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "the queue %d is open debug mode!", queue_id);
        rc = zxic_comm_mutex_unlock(p_mutex);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
        return DPP_RC_DTB_OPEN_DEBUG_MODE;
    }
#endif

    rc = dpp_dtb_queue_enable_get(dev, queue_id, &queue_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_dtb_queue_enable_get", p_mutex);
    if(!queue_en)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "the slot %d queue %d is not enable!", DEV_PCIE_SLOT(dev), queue_id);
        rc = zxic_comm_mutex_unlock(p_mutex);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev),rc,"zxic_comm_mutex_unlock");
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "the queue %d is not enable!", queue_id);
        return DPP_RC_DTB_QUEUE_NOT_ENABLE;
    }

    if (desc_len % 4 != 0)
    {   
        rc = zxic_comm_mutex_unlock(p_mutex);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev),rc,"zxic_comm_mutex_unlock");
        /* 硬件规定数据必须是16字节为单位 */
        return DPP_RC_DTB_PARA_INVALID;
    }

    rc = dpp_dtb_item_buff_wr(dev, queue_id, DPP_DTB_DIR_UP_TYPE, item_index, 0, desc_len, p_desc_data);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_dtb_item_buff_wr", p_mutex);


    rc = dpp_dtb_item_ack_wr(dev, queue_id, DPP_DTB_DIR_UP_TYPE, item_index, 0, DPP_DTB_TAB_ACK_IS_USING_MASK);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_dtb_item_ack_wr", p_mutex);


    DPP_DTB_TAB_UP_DATA_LEN_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, item_index) = data_len;

    item_info.cmd_vld = 1;
    item_info.cmd_type = DPP_DTB_DIR_UP_TYPE;
    item_info.int_en = int_flag;
    item_info.data_len = desc_len / 4;
    item_info.data_hddr = ((DPP_DTB_TAB_UP_PHY_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, item_index)>>4) >> 32) & 0xffffffff;
    item_info.data_laddr = (DPP_DTB_TAB_UP_PHY_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, item_index)>>4) & 0xffffffff;

    if(dpp_dtb_prt_get())
    {
        dpp_dtb_info_print(dev, queue_id, item_index,  &item_info);
    }
    
    rc = dpp_dtb_queue_item_info_set(dev, queue_id, &item_info);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(DEV_ID(dev), rc, "dpp_dtb_queue_item_info_set", p_mutex);

    rc = zxic_comm_mutex_unlock(p_mutex);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev),rc,"zxic_comm_mutex_unlock");
    
    return DPP_OK;
}

/***********************************************************/
/** 打印队列中指定元素的dump地址、ACK及下表中前768bit的数据
* @param   dev_id       芯片的id号  
* @param   queue_id     队列号，范围0-127    
* @param   element_id   元素号，范围0-31  
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2022/08/30
************************************************************/
ZXIC_UINT32 dpp_dtb_dump_table_element_info_prt(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 element_id)
{
    ZXIC_UINT32 rc = 0;

    ZXIC_UINT32 element_start_addr_h = 0;
    ZXIC_UINT32 element_start_addr_l = 0;
    ZXIC_UINT32 element_dump_addr_h = 0;
    ZXIC_UINT32 element_dump_addr_l = 0;
    ZXIC_UINT32 element_table_info_addr_h = 0;
    ZXIC_UINT32 element_table_info_addr_l = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), element_id, 0, DPP_DTB_QUEUE_ITEM_NUM_MAX - 1);

    rc = dpp_dtb_dump_table_elemet_addr_get(dev, 
                                            queue_id, 
                                            element_id,
                                            &element_start_addr_h,
                                            &element_start_addr_l,
                                            &element_dump_addr_h,
                                            &element_dump_addr_l,
                                            &element_table_info_addr_h,
                                            &element_table_info_addr_l);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_dump_table_elemet_addr_get");
    
    ZXIC_COMM_DBGCNT32_PRINT("slot_id", DEV_PCIE_SLOT(dev));
    ZXIC_COMM_DBGCNT32_PRINT("queue_id", queue_id);
    ZXIC_COMM_DBGCNT32_PRINT("element_id", element_id);
    ZXIC_COMM_DBGCNT32_PRINT("element_start_addr_h", element_start_addr_h);
    ZXIC_COMM_DBGCNT32_PRINT("element_start_addr_l", element_start_addr_l);
    ZXIC_COMM_DBGCNT32_PRINT("element_dump_addr_h", element_dump_addr_h);
    ZXIC_COMM_DBGCNT32_PRINT("element_dump_addr_l", element_dump_addr_l);
    ZXIC_COMM_DBGCNT32_PRINT("element_table_info_addr_h", element_table_info_addr_h);
    ZXIC_COMM_DBGCNT32_PRINT("element_table_info_addr_l", element_table_info_addr_l);

    /*打印element ack*/
    rc = dpp_dtb_item_ack_prt(dev, queue_id, DPP_DTB_DIR_UP_TYPE, element_id);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_item_ack_prt");

    rc = dpp_dtb_item_buff_prt(dev, queue_id, DPP_DTB_DIR_UP_TYPE, element_id, 32);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_item_buff_prt");

    return DPP_OK;
}

/***********************************************************/
/** 一个元素dump成功状态检查
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列号，范围0-31 
* @param   element_id           条目编号 
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_tab_up_success_status_check(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 element_id)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 rd_cnt = 0;
    ZXIC_UINT32 ack_value = 0;
    ZXIC_UINT32 success_flag = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), element_id, 0, DPP_DTB_QUEUE_ITEM_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_PCIE_SLOT(dev), 0, DPP_PCIE_SLOT_MAX - 1);

    if(dpp_dtb_soft_perf_test_get() || dpp_dtb_hardware_perf_test_get())
    {
        return rc;
    }
    rc = dpp_dtb_dev_status_check(dev);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_dev_status_check");

    if(DPP_DTB_QUEUE_INIT_FLAG_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id) == 0)
    {
        ZXIC_COMM_TRACE_ERROR("dtb slot %d queue %d is not init.\n", DEV_PCIE_SLOT(dev), queue_id);
        return DPP_RC_DTB_QUEUE_IS_NOT_INIT;
    }

    while(!success_flag)
    {
        rc = dpp_dtb_item_ack_rd(dev, queue_id, DPP_DTB_DIR_UP_TYPE, element_id, 0, &ack_value);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_item_ack_rd");

        if ((((ack_value >> 8) & 0xffffff) == DPP_DTB_TAB_UP_ACK_VLD_MASK) && 
             ((ack_value & 0xff) == DPP_DTB_TAB_ACK_SUCCESS_MASK))
        {
            success_flag = 1;
            break;
        }

        if (rd_cnt > dpp_dtb_dump_table_overtime_get())
        {
            ZXIC_COMM_TRACE_ERROR("Error!!! dpp dtb dump slot [%d] vport [0x%x] queue [%d] item [%d] ack success is overtime!\n", DEV_PCIE_SLOT(dev), DEV_PCIE_VPORT(dev), queue_id, element_id);

            ZXIC_COMM_PRINT("-------------------------------------------------------------------\n");
            ZXIC_COMM_PRINT("                     dtb dump table info                           \n");
            ZXIC_COMM_PRINT("-------------------------------------------------------------------\n");
            rc = dpp_dtb_dump_table_element_info_prt(dev, queue_id, element_id);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_dump_table_element_info_prt");

            rc = dpp_dtb_item_ack_wr(dev, queue_id, DPP_DTB_DIR_UP_TYPE, element_id, 0, DPP_DTB_TAB_ACK_UNUSED_MASK);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_item_ack_wr");

            ZXIC_COMM_PRINT("-------------------------------------------------------------------\n");
            ZXIC_COMM_PRINT("                          dtb reg info                             \n");
            ZXIC_COMM_PRINT("-------------------------------------------------------------------\n");

            rc = diag_dpp_dtb_axi_last_operate_info_prt(dev);
            ZXIC_COMM_CHECK_DEV_RC(0, rc, "diag_dpp_dtb_axi_last_operate_info_prt");

            rc = diag_dpp_dtb_channels_state_info_prt(dev);
            ZXIC_COMM_CHECK_DEV_RC(0, rc, "diag_dpp_dtb_channels_state_info_prt");

            rc = diag_dpp_dtb_channels_axi_resp_err_cnt_prt(dev);
            ZXIC_COMM_CHECK_DEV_RC(0, rc, "diag_dpp_dtb_channels_axi_resp_err_cnt_prt");
            
            return DPP_ERR;
        }

        rd_cnt++;
        zxic_comm_udelay(1);
    }

    return rc;
}

/***********************************************************/
/** 获取dump数据
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列号，范围0-31 
* @param   item_index           数据对应的的条目编号 
* @param   data_len             数据长度，单位32bit; 
* @param   p_data               dump数据 
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_tab_up_data_get(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 item_index,
                    ZXIC_UINT32 data_len,
                    ZXIC_UINT32 *p_data)
{
    ZXIC_UINT32 rc = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), item_index, 0, DPP_DTB_QUEUE_ITEM_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);

    if(DPP_DTB_QUEUE_INIT_FLAG_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id) == 0)
    {
        ZXIC_COMM_TRACE_ERROR("dtb slot %d queue %d is not init.\n", DEV_PCIE_SLOT(dev), queue_id);
        return DPP_RC_DTB_QUEUE_IS_NOT_INIT;
    }

    if(dpp_dtb_hardware_perf_test_get())
    {
        return rc;
    }

    rc = dpp_dtb_item_buff_rd(dev, 
                    queue_id, 
                    DPP_DTB_DIR_UP_TYPE, 
                    item_index, 
                    0, 
                    data_len, 
                    p_data);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_item_buff_rd");

    if(dpp_dtb_debug_fun_get())
    {
        return DPP_OK;
    }

    rc = dpp_dtb_item_ack_wr(dev, queue_id, DPP_DTB_DIR_UP_TYPE, item_index, 0, DPP_DTB_TAB_ACK_UNUSED_MASK);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_item_ack_wr");
    
    return DPP_OK;
}

#endif
/***********************************************************/
/** dtb队列down初始化
* @param   dev_id           设备号，支持多芯片 
* @param   queue_id         队列号
* @param   p_queue_cfg      队列配置参数，具体见DPP_DTB_QUEUE_CFG_T结构体类型
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_down_init(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    DPP_DTB_QUEUE_CFG_T *p_queue_cfg)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 ack_vale = 0;
    ZXIC_UINT32 tab_down_item_size = 0;
    DPP_DTB_MGR_T *p_dtb_mgr = ZXIC_NULL;
    
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_queue_cfg);

    p_dtb_mgr = dpp_dtb_mgr_get(DEV_PCIE_SLOT(dev), DEV_ID(dev));
    if (p_dtb_mgr == ZXIC_NULL)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "slot %d ErrorCode[0x%x]: DTB Manager is not exist!!!\n", DEV_PCIE_SLOT(dev), DPP_RC_DTB_MGR_NOT_EXIST);
        return DPP_RC_DTB_MGR_NOT_EXIST;
    }

    /*
     * 流程：
     * 1.寻找空闲队列；
     * 2.若找到：
     *      1）配置硬件信息；
     *      2）配置软件缓存信息；
     *      3）初始化条目ack缓存信息,空闲约定:up-0x000000XX,down-0x000000XX；
     *                            忙约定:up-0x111111XX,down-0x111111XX;
     *                            完成约定:up-0x555555XX,down-0x5a5a5aXX;
     * 3.若未找到：返回错误信息；
     */

    p_dtb_mgr->queue_info[queue_id].slot_id = DEV_PCIE_SLOT(dev);
    p_dtb_mgr->queue_info[queue_id].vport = DEV_PCIE_VPORT(dev);

    tab_down_item_size = (p_queue_cfg->down_item_size == 0) ? DPP_DTB_ITEM_SIZE : p_queue_cfg->down_item_size;
    
    p_dtb_mgr->queue_info[queue_id].tab_down.item_size = tab_down_item_size;
    p_dtb_mgr->queue_info[queue_id].tab_down.start_phy_addr = p_queue_cfg->down_start_phy_addr;
    p_dtb_mgr->queue_info[queue_id].tab_down.start_vir_addr = p_queue_cfg->down_start_vir_addr;
    p_dtb_mgr->queue_info[queue_id].tab_down.wr_index = 0;
    p_dtb_mgr->queue_info[queue_id].tab_down.rd_index = 0;

    for (i = 0; i < DPP_DTB_QUEUE_ITEM_NUM_MAX; i++)
    {
        rc = dpp_dtb_init_item_ack_wr(dev, queue_id, DPP_DTB_DIR_DOWN_TYPE, i, 0, DPP_DTB_TAB_ACK_CHECK_VALUE);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_item_ack_wr");
    }

    for(i = 0; i < DPP_DTB_QUEUE_ITEM_NUM_MAX; i++)
    {
        rc = dpp_dtb_init_item_ack_rd(dev, queue_id, DPP_DTB_DIR_DOWN_TYPE, i, 0, &ack_vale);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_item_ack_rd");
        if(ack_vale != DPP_DTB_TAB_ACK_CHECK_VALUE)
        {
            ZXIC_COMM_PRINT("dtb slot [%d] queue [%d] down init faild, mem err!!!\n", DEV_PCIE_SLOT(dev), queue_id);
            return DPP_RC_DTB_MEMORY_ALLOC_ERR;
        }
    }

    ZXIC_COMM_MEMSET((ZXIC_UINT8 *)(p_queue_cfg->down_start_vir_addr), 0, tab_down_item_size * DPP_DTB_QUEUE_ITEM_NUM_MAX);

    ZXIC_COMM_TRACE_NOTICE("dtb slot [%d] queue [%d] down init success!!!\n", DEV_PCIE_SLOT(dev), queue_id);
    
    return DPP_OK;
}

/***********************************************************/
/** dtb队列dump初始化
* @param   dev_id           设备号，支持多芯片 
* @param   queue_id         队列号
* @param   p_queue_cfg      队列配置参数，具体见DPP_DTB_QUEUE_CFG_T结构体类型
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_dump_init(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    DPP_DTB_QUEUE_CFG_T *p_queue_cfg)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 ack_vale = 0;
    ZXIC_UINT32 tab_up_item_size = 0;
    DPP_DTB_MGR_T *p_dtb_mgr = ZXIC_NULL;
    ZXIC_UINT32 slot_id = 0;
    
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_queue_cfg);

    slot_id = (ZXIC_UINT32)DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_INDEX(slot_id, 0, DPP_DEV_SLOT_MAX - 1);

    p_dtb_mgr = dpp_dtb_mgr_get(slot_id, DEV_ID(dev));
    if (p_dtb_mgr == ZXIC_NULL)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "slot %d ErrorCode[0x%x]: DTB Manager is not exist!!!\n", DEV_PCIE_SLOT(dev), DPP_RC_DTB_MGR_NOT_EXIST);
        return DPP_RC_DTB_MGR_NOT_EXIST;
    }

    /*
     * 流程：
     * 1.寻找空闲队列；
     * 2.若找到：
     *      1）配置硬件信息；
     *      2）配置软件缓存信息；
     *      3）初始化条目ack缓存信息,空闲约定:up-0x000000XX,down-0x000000XX；
     *                            忙约定:up-0x111111XX,down-0x111111XX;
     *                            完成约定:up-0x555555XX,down-0x5a5a5aXX;
     * 3.若未找到：返回错误信息；
     */

    p_dtb_mgr->queue_info[queue_id].slot_id = DEV_PCIE_SLOT(dev);
    p_dtb_mgr->queue_info[queue_id].vport = DEV_PCIE_VPORT(dev);

    tab_up_item_size = (p_queue_cfg->up_item_size == 0) ? DPP_DTB_ITEM_SIZE : p_queue_cfg->up_item_size;
    
    p_dtb_mgr->queue_info[queue_id].tab_up.item_size = tab_up_item_size;
    p_dtb_mgr->queue_info[queue_id].tab_up.start_phy_addr = p_queue_cfg->up_start_phy_addr;
    p_dtb_mgr->queue_info[queue_id].tab_up.start_vir_addr = p_queue_cfg->up_start_vir_addr;
    p_dtb_mgr->queue_info[queue_id].tab_up.wr_index = 0;
    p_dtb_mgr->queue_info[queue_id].tab_up.rd_index = 0;

    for (i = 0; i < DPP_DTB_QUEUE_ITEM_NUM_MAX; i++)
    {
        rc = dpp_dtb_init_item_ack_wr(dev, queue_id, DPP_DTB_DIR_UP_TYPE, i, 0, DPP_DTB_TAB_ACK_CHECK_VALUE);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_item_ack_wr");
    }

    for(i = 0; i < DPP_DTB_QUEUE_ITEM_NUM_MAX; i++)
    {
        rc = dpp_dtb_init_item_ack_rd(dev, queue_id, DPP_DTB_DIR_UP_TYPE, i, 0, &ack_vale);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_item_ack_rd");
        if(ack_vale != DPP_DTB_TAB_ACK_CHECK_VALUE)
        {
            ZXIC_COMM_PRINT("dtb slot [%d] queue [%d] init faild, mem err!!!\n", DEV_PCIE_SLOT(dev), queue_id);
            return DPP_RC_DTB_MEMORY_ALLOC_ERR;
        }
    }

    ZXIC_COMM_MEMSET((ZXIC_UINT8 *)(p_queue_cfg->up_start_vir_addr), 0, tab_up_item_size * DPP_DTB_QUEUE_ITEM_NUM_MAX);

    ZXIC_COMM_TRACE_NOTICE("dtb slot [%d] queue [%d] up init success!!!\n", DEV_PCIE_SLOT(dev), queue_id);
    
    return DPP_OK;
}

/***********************************************************/
/** dtb队列down 空间地址配置
* @param   channelId    dtb通道号 
* @param   phyAddr      down物理地址
* @param   virAddr      down虚拟地址
* @param   size         空间大小 0:使用系统默认值16K+16
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_down_channel_addr_set(DPP_DEV_T *dev,
                                          ZXIC_UINT32 channelId, 
                                          ZXIC_UINT64 phyAddr, 
                                          ZXIC_UINT64 virAddr, 
                                          ZXIC_UINT32 size)
{
    ZXIC_UINT32 rc = 0;

    DPP_DTB_QUEUE_CFG_T down_queue_cfg = {0};

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);

    down_queue_cfg.down_start_phy_addr = phyAddr;
    down_queue_cfg.down_start_vir_addr = virAddr;
    down_queue_cfg.down_item_size = size;

   rc = dpp_dtb_queue_down_init(dev,
                                channelId,
                                &down_queue_cfg);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_queue_down_init");

    return rc;
}

/***********************************************************/
/** dtb队列dump 空间地址配置
* @param   channelId    dtb通道号 
* @param   phyAddr      dump物理地址
* @param   virAddr      dump虚拟地址
* @param   size         空间大小
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_dump_channel_addr_set(DPP_DEV_T *dev,
                                          ZXIC_UINT32 channelId, 
                                          ZXIC_UINT64 phyAddr, 
                                          ZXIC_UINT64 virAddr, 
                                          ZXIC_UINT32 size)
{
    ZXIC_UINT32 rc = 0;

    DPP_DTB_QUEUE_CFG_T dump_queue_cfg = {0};

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);

    dump_queue_cfg.up_start_phy_addr = phyAddr;
    dump_queue_cfg.up_start_vir_addr = virAddr;
    dump_queue_cfg.up_item_size = size;

    rc = dpp_dtb_queue_dump_init(dev,
                                 channelId,
                                 &dump_queue_cfg);

    return rc;
}

ZXIC_UINT32 dpp_dtb_queue_init_flag_set(DPP_DEV_T *dev, 
                                        ZXIC_UINT32 queue_id, 
                                        ZXIC_UINT32 flag)
{
    ZXIC_UINT32 slot_id = 0;
    DPP_DTB_MGR_T *p_dtb_mgr = ZXIC_NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);

    slot_id = (ZXIC_UINT32)DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_INDEX(slot_id, 0, DPP_DEV_SLOT_MAX - 1);

    p_dtb_mgr = dpp_dtb_mgr_get(slot_id, DEV_ID(dev));
    if (p_dtb_mgr == ZXIC_NULL)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "slot %d ErrorCode[0x%x]: DTB Manager is not exist!!!\n", DEV_PCIE_SLOT(dev), DPP_RC_DTB_MGR_NOT_EXIST);
        return DPP_RC_DTB_MGR_NOT_EXIST;
    }

    p_dtb_mgr->queue_info[queue_id].init_flag = flag;

    return DPP_OK;
}

/***********************************************************/
/** 释放队列资源
* @param   dev_id           设备号，支持多芯片  
* @param   queue_id         分配到的队列号;
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2021/02/23
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_id_free(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 item_num = 0;
    DPP_DTB_MGR_T *p_dtb_mgr = ZXIC_NULL;
    
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev),queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);

    p_dtb_mgr = dpp_dtb_mgr_get(DEV_PCIE_SLOT(dev), DEV_ID(dev));
    if (p_dtb_mgr == ZXIC_NULL)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "slot %d ErrorCode[0x%x]: DTB Manager is not exist!!!\n", DEV_PCIE_SLOT(dev), DPP_RC_DTB_MGR_NOT_EXIST);
        return DPP_RC_DTB_MGR_NOT_EXIST;
    }

    rc = dpp_dtb_queue_unused_item_num_get(dev, queue_id, &item_num);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_queue_unused_item_num_get");
    
    if (item_num != DPP_DTB_QUEUE_ITEM_NUM_MAX)
    {
        return DPP_RC_DTB_QUEUE_IS_WORKING;
    }

    rc = dpp_dtb_queue_init_flag_set(dev, queue_id, DPP_DTB_QUEUE_UNINIT);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_queue_init_flag_set");

    p_dtb_mgr->queue_info[queue_id].func_type = DPP_DTB_QUEUE_TYPE_INVALID;
    p_dtb_mgr->queue_info[queue_id].slot_id   = 0xFFFFFFFF;
    p_dtb_mgr->queue_info[queue_id].vport     = 0XFFFFFFFF;

    ZXIC_COMM_MEMSET(&(p_dtb_mgr->queue_info[queue_id].tab_up), 0, sizeof(DPP_DTB_TAB_UP_INFO_T));
    ZXIC_COMM_MEMSET(&(p_dtb_mgr->queue_info[queue_id].tab_down), 0, sizeof(DPP_DTB_TAB_DOWN_INFO_T));

    return DPP_OK;
}

/***********************************************************/
/** dtb初始化
* @param   dev_id       设备号，支持多芯片   
*
* @return  
* @remark  无
* @see     
* @author  zab      @date  2022/08/30
************************************************************/
ZXIC_UINT32 dpp_dtb_init(DPP_DEV_T *dev)
{
    ZXIC_UINT32 rc = 0;
    DPP_DTB_MGR_T *p_dtb_mgr = ZXIC_NULL;

    ZXIC_COMM_CHECK_POINT(dev);

    p_dtb_mgr = dpp_dtb_mgr_get(DEV_PCIE_SLOT(dev), DEV_ID(dev));
    if (p_dtb_mgr == ZXIC_NULL)
    {
        rc = dpp_dtb_mgr_create(DEV_PCIE_SLOT(dev), DEV_ID(dev));
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_mgr_create");

        p_dtb_mgr = dpp_dtb_mgr_get(DEV_PCIE_SLOT(dev), DEV_ID(dev));
        if (p_dtb_mgr == ZXIC_NULL)
        {
            ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "slot %d ErrorCode[0x%x]: DTB Manager is not exist!!!\n", DEV_PCIE_SLOT(dev), DPP_RC_DTB_MGR_NOT_EXIST);
            return DPP_RC_DTB_MGR_NOT_EXIST;
        }
    }

    return DPP_OK;
}

/***********************************************************/
/** 根据vport查找相应的队列号
* @param   dev_id       设备号，支持多芯片   
* @param   vport        vport信息
* @param   p_queue_arr 找到到队列数组
* @param   p_num          找到的队列个数
* @return  
* @remark  无
* @see     
* @author  cbb      @date  2023/09/13
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_id_search_by_vport(DPP_DEV_T *dev,
                                             ZXIC_UINT32 *p_queue_arr,
                                             ZXIC_UINT32 *p_num)
{
    ZXIC_UINT32 queue_id = 0;
    ZXIC_UINT32 count = 0;

    DPP_DTB_MGR_T *p_dtb_mgr = ZXIC_NULL;

    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_queue_arr);

    p_dtb_mgr = dpp_dtb_mgr_get(DEV_PCIE_SLOT(dev), DEV_ID(dev));
    if (p_dtb_mgr == ZXIC_NULL)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "slot %d ErrorCode[0x%x]: DTB Manager is not exist!!!\n", DEV_PCIE_SLOT(dev), DPP_RC_DTB_MGR_NOT_EXIST);
        return DPP_RC_DTB_MGR_NOT_EXIST;
    }

    for(queue_id = 0; queue_id < DPP_DTB_QUEUE_NUM_MAX; queue_id++)
    {
        if(DPP_DTB_QUEUE_INIT_FLAG_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id) != 0)
        {
            if(DEV_PCIE_VPORT(dev) == DPP_DTB_QUEUE_VPORT_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id))
            {
                p_queue_arr[count] = queue_id;
                count++;
            }
        }
    }

    if (count == 0)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "slot %d ErrorCode[0x%x]: vport 0x%04x no queue not found!!!\n", 
                                                   DEV_PCIE_SLOT(dev), DPP_RC_DTB_QUEUE_NOT_ALLOC, DEV_PCIE_VPORT(dev));
        return DPP_RC_DTB_QUEUE_NOT_ALLOC;
    }

    *p_num = count;

    return DPP_OK;
}

/***********************************************************/
/** 根据vport查找相应的下表队列号
* @param   dev_id       设备号，支持多芯片   
* @param   vport        vport信息
* @param   p_queue_arr 找到到队列数组
* @param   p_num          找到的队列个数
* @return  
* @remark  无
* @see     
* @author  cbb      @date  2023/09/13
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_id_get(DPP_DEV_T *dev, ZXIC_UINT32 *queue)
{
    ZXIC_UINT32 i      = 0;
    ZXIC_UINT32 num    = 0;
    ZXIC_UINT32 rc     = DPP_OK;

    ZXIC_UINT32 queue_arr[DPP_DTB_QUEUE_NUM_MAX] = {0};

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);

    rc = dpp_dtb_dev_status_check(dev);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_dev_status_check");
    
    rc = dpp_dtb_queue_id_search_by_vport(dev, queue_arr, &num);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_queue_id_search_by_vport");

    for(i = 0; i < num; i++)
    {
        if(DPP_DTB_QUEUE_TYPE_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_arr[i]) == DPP_DTB_QUEUE_TYPE_TABLE)
        {
            *queue = queue_arr[i];
            return DPP_OK;
        }
    }

    return DPP_ERR;
}

/***********************************************************/
/** 根据vport查找相应的统计队列号
* @param   dev_id       设备号，支持多芯片   
* @param   p_stat_queue 统计队列号
* @return  
* @remark  无
* @see     
* @author  cbb      @date  2025/11/21
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_id_get_by_func(DPP_DEV_T *dev, ZXIC_UINT32 func_type, ZXIC_UINT32 *p_stat_queue)
{
    ZXIC_UINT32 i      = 0;
    ZXIC_UINT32 num    = 0;
    ZXIC_UINT32 rc     = DPP_OK;

    ZXIC_UINT32 queue_arr[DPP_DTB_QUEUE_NUM_MAX] = {0};

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);

    rc = dpp_dtb_dev_status_check(dev);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_dev_status_check");
    
    rc = dpp_dtb_queue_id_search_by_vport(dev, queue_arr, &num);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_queue_id_search_by_vport");

    for(i = 0; i < num; i++)
    {
        if(DPP_DTB_QUEUE_TYPE_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_arr[i]) == func_type)
        {
            *p_stat_queue = queue_arr[i];
            return DPP_OK;
        }
    }

    return DPP_ERR;
}


/***********************************************************/
/** 获取当前队列有效标识
* @param   dev          设备
* @param   queue        队列id
* @param   init_flag    出参 0:当前队列未被使用 1：当前队列已被vport使用
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/11/06
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_valid_flag_get(DPP_DEV_T *dev, ZXIC_UINT32 queue, ZXIC_UINT32 *valid_flag)
{
    ZXIC_UINT32 vport = 0;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 slot_id = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(valid_flag);

    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id,queue, 0, DPP_DTB_QUEUE_NUM_MAX - 1);

    slot_id = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id,slot_id, 0, DPP_DEV_SLOT_MAX - 1);

    vport = DEV_PCIE_VPORT(dev);
    *valid_flag = 0;
    if(DPP_DTB_QUEUE_INIT_FLAG_GET(DEV_PCIE_SLOT(dev), dev_id, queue)
        &&(DPP_DTB_QUEUE_VPORT_GET(DEV_PCIE_SLOT(dev), dev_id, queue) == vport))
    {
        *valid_flag = 1;
    }

    return DPP_OK;
}

/***********************************************************/
/** 获取当前队列初始化标识
* @param   dev          设备
* @param   queue        队列id
* @param   init_flag    出参 0:未初始化 1:已初始化
* @return  
* @remark  无
* @see     
* @author  cq      @date  2024/11/06
************************************************************/
ZXIC_UINT32 dpp_dtb_queue_init_flag_get(DPP_DEV_T *dev, ZXIC_UINT32 queue, ZXIC_UINT32 *init_flag)
{
    ZXIC_UINT32 vport = 0;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 slot_id = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(init_flag);

    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id,queue, 0, DPP_DTB_QUEUE_NUM_MAX - 1);

    slot_id = DEV_PCIE_SLOT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id,slot_id, 0, DPP_DEV_SLOT_MAX - 1);

    vport = DEV_PCIE_VPORT(dev);
    *init_flag = 0;
    if(DPP_DTB_QUEUE_INIT_FLAG_GET(DEV_PCIE_SLOT(dev), dev_id, queue))
    {
        *init_flag = 1;
    }

    return DPP_OK;
}



#endif
#endif

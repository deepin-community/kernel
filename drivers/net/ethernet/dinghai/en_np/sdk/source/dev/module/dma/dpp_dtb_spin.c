/**************************************************************
* 版权所有(C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_dtb_spin.c
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : cbb
* 完成日期 : 2025/11/21
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

#if ZXIC_REAL("SPIN_TAB_UP")

/***********************************************************/
/** dump队列空闲条目获取------spin锁
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列号，范围0-31 
* @param   p_item_index         返回使用的条目编号 
* @return  
* @remark  无
* @see     
* @author  cbb      @date  2025/11/22
************************************************************/
ZXIC_UINT32 dpp_dtb_spin_tab_up_free_item_get(DPP_DEV_T *dev,
                                             ZXIC_UINT32 queue_id,
                                             ZXIC_UINT32 *p_item_index)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 ack_value = 0;
    ZXIC_UINT32 item_index = 0;
    ZXIC_UINT32 unused_item_num = 0;
    ZXIC_UINT32 ack_rst = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_item_index);

    rc = dpp_dtb_dev_status_check(dev);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_dev_status_check");

    /*
     * 流程
     * 1.获取硬件队列剩余情况，大于0正常上送，等于0返回失败；
     * 2.获取软件缓存空闲情况；
     *
     */
    if(DPP_DTB_QUEUE_INIT_FLAG_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id) == 0)
    {
        ZXIC_COMM_TRACE_ERROR("dtb slot %d queue %d is not init.\n", DEV_PCIE_SLOT(dev), queue_id);
        return DPP_RC_DTB_QUEUE_IS_NOT_INIT;
    }
    
    rc = dpp_dtb_queue_unused_item_num_get(dev, queue_id, &unused_item_num);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_queue_unused_item_num_get");

    if (unused_item_num == 0)
    {
        return DPP_RC_DTB_QUEUE_ITEM_HW_EMPTY;
    }

    for (i = 0; i < DPP_DTB_QUEUE_ITEM_NUM_MAX; i++)
    {
        item_index = DPP_DTB_TAB_UP_WR_INDEX_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id) % DPP_DTB_QUEUE_ITEM_NUM_MAX;
        
        rc = dpp_dtb_item_ack_rd(dev, queue_id, DPP_DTB_DIR_UP_TYPE, item_index, 0, &ack_value);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_item_ack_rd");

        DPP_DTB_TAB_UP_WR_INDEX_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id)++;

        ack_rst = (ack_value >> 8) & 0xFFFFFF;

        if (ack_rst == DPP_DTB_TAB_ACK_UNUSED_MASK || ack_rst == DPP_DTB_TAB_UP_ACK_VLD_MASK)
        {
            break;
        }
    }

    if (i == DPP_DTB_QUEUE_ITEM_NUM_MAX)
    {
        return DPP_RC_DTB_STAT_QUEUE_ITEM_SW_EMPTY;
    }

    rc = dpp_dtb_item_ack_wr(dev, queue_id, DPP_DTB_DIR_UP_TYPE, item_index, 0, DPP_DTB_TAB_ACK_IS_USING_MASK);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_item_buff_wr");

    *p_item_index = item_index;

    return DPP_OK;
}

/***********************************************************/
/** dump配置描述符信息设置------ spin锁
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
* @author  cbb      @date  2025/11/22
************************************************************/
ZXIC_UINT32 dpp_dtb_spin_tab_up_info_set(DPP_DEV_T *dev,
                    ZXIC_UINT32 queue_id,
                    ZXIC_UINT32 item_index,
                    ZXIC_UINT32 data_len,
                    ZXIC_UINT32 desc_len,
                    ZXIC_UINT32 *p_desc_data)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 queue_en = 0;
    DPP_DTB_QUEUE_ITEM_INFO_T item_info = {0};
    
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), item_index, 0, DPP_DTB_QUEUE_ITEM_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), desc_len, 0, 0x400);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_desc_data);

    /*
     * 流程
     * 0.检测队列是否使能
     * 1.将dump描述符写入buff中;
     * 2.将ack字段填入0x11111100;
     * 3.将数据信息填入硬件触发寄存器中;
     *
     */

    rc = dpp_dtb_dev_status_check(dev);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_dev_status_check");

    if(DPP_DTB_QUEUE_INIT_FLAG_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id) == 0)
    {
        ZXIC_COMM_TRACE_ERROR("dtb slot %d queue %d is not init.\n", DEV_PCIE_SLOT(dev), queue_id);
        return DPP_RC_DTB_QUEUE_IS_NOT_INIT;
    }

    if (desc_len % 4 != 0)
    {   
        /* 硬件规定数据必须是16字节为单位 */
        return DPP_RC_DTB_PARA_INVALID;
    }

    rc = dpp_dtb_queue_enable_get(dev, queue_id, &queue_en);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_queue_enable_get");
    if(!queue_en)
    {
        // ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "the slot %d queue %d is not enable!", DEV_PCIE_SLOT(dev), queue_id);
        return DPP_RC_DTB_STAT_QUEUE_NOT_ENABLE;
    }

    rc = dpp_dtb_item_buff_wr(dev, queue_id, DPP_DTB_DIR_UP_TYPE, item_index, 0, desc_len, p_desc_data);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_item_buff_wr");


    rc = dpp_dtb_item_ack_wr(dev, queue_id, DPP_DTB_DIR_UP_TYPE, item_index, 0, DPP_DTB_TAB_ACK_IS_USING_MASK);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_item_ack_wr");


    DPP_DTB_TAB_UP_DATA_LEN_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, item_index) = data_len;

    item_info.cmd_vld = 1;
    item_info.cmd_type = DPP_DTB_DIR_UP_TYPE;
    item_info.int_en = dpp_dtb_interrupt_status_get();
    item_info.data_len = desc_len / 4;
    item_info.data_hddr = ((DPP_DTB_TAB_UP_PHY_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, item_index)>>4) >> 32) & 0xffffffff;
    item_info.data_laddr = (DPP_DTB_TAB_UP_PHY_ADDR_GET(DEV_PCIE_SLOT(dev), DEV_ID(dev), queue_id, item_index)>>4) & 0xffffffff;

    if(dpp_dtb_prt_get())
    {
        dpp_dtb_info_print(dev, queue_id, item_index,  &item_info);
    }
    
    rc = dpp_dtb_queue_item_info_set(dev, queue_id, &item_info);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_queue_item_info_set");
    
    return DPP_OK;
}

/***********************************************************/
/** 一个元素dump成功状态检查-----------spin锁
* @param   dev_id               设备号，支持多芯片
* @param   queue_id             队列号，范围0-31 
* @param   element_id           条目编号 
*
* @return  
* @remark  无
* @see     
* @author  cbb      @date  2025/11/22
************************************************************/
ZXIC_UINT32 dpp_dtb_spin_tab_up_success_status_check(DPP_DEV_T *dev,
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

        if (rd_cnt > dpp_dtb_down_table_overtime_get())
        {
            // ZXIC_COMM_TRACE_ERROR("Error!!! dpp dtb dump slot [%d] vport [0x%x] queue [%d] item [%d] ack success is overtime!\n", DEV_PCIE_SLOT(dev), DEV_PCIE_VPORT(dev), queue_id, element_id);

            // ZXIC_COMM_PRINT("-------------------------------------------------------------------\n");
            // ZXIC_COMM_PRINT("                     dtb dump table info                           \n");
            // ZXIC_COMM_PRINT("-------------------------------------------------------------------\n");
            // rc = dpp_dtb_dump_table_element_info_prt(dev, queue_id, element_id);
            // ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_dump_table_element_info_prt");

            rc = dpp_dtb_item_ack_wr(dev, queue_id, DPP_DTB_DIR_UP_TYPE, element_id, 0, DPP_DTB_TAB_ACK_UNUSED_MASK);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_item_ack_wr");
            
            return DPP_RC_DTB_STAT_OVER_TIME;
        }

        rd_cnt++;
        zxic_comm_udelay(1);
    }

    return rc;
}

/** 下发dump描述符，dump出数据 
* @param   dev_id       设备号
* @param   queue_id     队列号
* @param   queue_element_id   元素编号
* @param   p_dump_info  dump描述符
* @param   data_len     dump出数据的长度(32bit为单位)
* @param   desc_len     dump描述符的长度(32bit为单位)
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_spin_write_dump_desc_info(DPP_DEV_T *dev,
                                ZXIC_UINT32 queue_id, 
                                ZXIC_UINT32 queue_element_id,
                                ZXIC_UINT32 *p_dump_info,
                                ZXIC_UINT32 data_len,
                                ZXIC_UINT32 desc_len)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(dev);

    if((!(dev->pcie_channel.dev_status)) || (!dtb_table_function_switch_get()))
    {
        ZXIC_COMM_TRACE_NOTICE("slot[%u] vport[0x%x] dev status off!\n",dev->pcie_channel.slot,dev->pcie_channel.vport);
        dpp_dtb_item_ack_wr(dev, queue_id, DPP_DTB_DIR_UP_TYPE, queue_element_id, 0, DPP_DTB_TAB_ACK_UNUSED_MASK);
        return ZXIC_PAR_CHK_DEV_STATUS_OFF;
    }
    
    /*下发dump描述符*/
    rc = dpp_dtb_spin_tab_up_info_set(dev,
                                    queue_id,
                                    queue_element_id,
                                    data_len,
                                    desc_len,
                                    p_dump_info);                              
    if(DPP_OK != rc)
    {
        // ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "the queue %d element id %d dump info set failed!", queue_id, queue_element_id);
        dpp_dtb_item_ack_wr(dev, queue_id, DPP_DTB_DIR_UP_TYPE, queue_element_id, 0, DPP_DTB_TAB_ACK_UNUSED_MASK);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_tab_up_info_set");
    }

    return DPP_OK;
}

/** smmu0 dump 只写一个dump描述符的接口------spin锁
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   base_addr 要dump的内容的基地址，以128bit为单位
* @param   depth     dump的深度以128bit为单位
* @param   p_data    dump出数据缓存(128bit * depth)
* @param   element_id   返回下表使用的元素id
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_spin_se_smmu0_dma_dump(DPP_DEV_T *dev, 
                                      ZXIC_UINT32 queue_id, 
                                      ZXIC_UINT32 base_addr, 
                                      ZXIC_UINT32 depth,
                                      ZXIC_UINT32 *p_data,
                                      ZXIC_UINT32 *element_id
                                      )
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dump_dst_phy_haddr = 0;
    ZXIC_UINT32 dump_dst_phy_laddr = 0;
    ZXIC_UINT32 queue_item_index = 0;
    ZXIC_UINT32 data_len = 0;
    ZXIC_UINT32 desc_len = 0;
    ZXIC_UINT8 form_buff[DTB_TABLE_CMD_SIZE_BIT / 8] = {0};
    ZXIC_SPIN_LOCK_T *p_spin_lock = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_LOWER(depth, 1);

    rc = dpp_dev_dtb_opr_spin_lock_get(dev, queue_id, &p_spin_lock);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dev_dtb_opr_spin_lock_get");

    rc = zxic_comm_spin_try_lock(p_spin_lock);
    if(rc == ZXIC_SPIN_LOCK_TRYLOCK_FAIL)
    {
        return ZXIC_SPIN_LOCK_TRYLOCK_FAIL;
    }
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_spin_try_lock");

    /*获取队列中可用的元素编号*/
    rc = dpp_dtb_spin_tab_up_free_item_get(dev, queue_id, &queue_item_index);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_SPIN_UNLOCK(DEV_ID(dev), rc, "dpp_dtb_spin_tab_up_free_item_get", p_spin_lock);

    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_PRINT("dump smmu0:queue %d,item_index: %d\n",queue_id, queue_item_index);
    }

    *element_id = queue_item_index;//保存获取的item_index
    
    /*获取地址*/
    rc = dpp_dtb_tab_up_item_addr_get(dev, queue_id, queue_item_index, &dump_dst_phy_haddr, &dump_dst_phy_laddr);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_SPIN_UNLOCK(DEV_ID(dev), rc, "dpp_dtb_tab_up_item_addr_get", p_spin_lock);

    rc = dpp_dtb_smmu0_dump_info_write(dev,
                                       base_addr,
                                       depth,
                                       dump_dst_phy_haddr,
                                       dump_dst_phy_laddr,
                                       (ZXIC_UINT32 *)form_buff);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_SPIN_UNLOCK(DEV_ID(dev), rc, "dpp_dtb_smmu0_dump_info_write", p_spin_lock);
    
    /*组装下表命令格式*/

    data_len = depth * 128 / 32;
    desc_len = DTB_LEN_POS_SETP / 4;

    rc = dpp_dtb_spin_write_dump_desc_info(dev, queue_id, queue_item_index, (ZXIC_UINT32 *)form_buff, data_len, desc_len);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_SPIN_UNLOCK(DEV_ID(dev), rc, "dpp_dtb_write_dump_desc_info", p_spin_lock);

    rc = zxic_comm_spin_unlock(p_spin_lock);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_spin_unlock");

    /*查询是否dump完成，完成后取出数据*/
    rc = dpp_dtb_spin_tab_up_success_status_check(dev, queue_id, queue_item_index);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_tab_up_success_status_check");

    /*取出数据*/
    rc = dpp_dtb_tab_up_data_get(dev, queue_id, queue_item_index, data_len, p_data);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_tab_up_data_get");

    return DPP_OK;
}

/** dtb dump eram直接表表项内容
* @param   dev_id       设备号
* @param   queue_id     队列号
* @param   sdt_no       eram表sdt表号
* @param   p_dump_eram_entry  eram数据结构，数据已分配相应内存
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_spin_eram_stat_data_get(DPP_DEV_T *dev, 
                                      ZXIC_UINT32 queue_id, 
                                      ZXIC_UINT32 base_addr, 
                                      ZXIC_UINT32 rd_mode, 
                                      ZXIC_UINT32 index,
                                      ZXIC_UINT32 *p_data)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 eram_dump_base_addr = 0;
    ZXIC_UINT32 row_index = 0;
    ZXIC_UINT32 col_index = 0;
    ZXIC_UINT32 temp_data[4] = {0};
    ZXIC_UINT32 element_id = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev),p_data);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DTB_QUEUE_MAX - 1);

    rc = dtb_eram_index_cal(dev, rd_mode, index, &row_index, &col_index);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dtb_eram_index_cal");

    eram_dump_base_addr = base_addr + row_index;

    rc = dpp_dtb_spin_se_smmu0_dma_dump(dev, 
                                queue_id, 
                                eram_dump_base_addr, 
                                1,
                                temp_data,
                                &element_id);
    if(rc == ZXIC_SPIN_LOCK_TRYLOCK_FAIL)
    {
        return ZXIC_SPIN_LOCK_TRYLOCK_FAIL;
    }
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_spin_se_smmu0_dma_dump");

    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_PRINT("dtb dump eram done, the element id is %d.\n", element_id);
    }

    //提取数据
    switch (rd_mode)
    {
        case ERAM128_TBL_128b:
        {
            ZXIC_COMM_MEMCPY(p_data, temp_data, (128 / 8));
            break;
        }

        case ERAM128_TBL_64b:
        {
            ZXIC_COMM_MEMCPY(p_data, temp_data + ((1 - col_index) << 1), (64 / 8));
            break;
        }

        case ERAM128_TBL_1b:
        {
            ZXIC_COMM_UINT32_GET_BITS(p_data[0], *(temp_data + (3 - col_index / 32)), (col_index % 32), 1);
            break;
        }
    }

    return rc;
}


#endif
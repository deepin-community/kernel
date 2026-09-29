#include "zxic_private_top.h"
#include "zxic_common.h"
#include "dpp_dtb_table_api.h"
#include "dpp_dtb_table.h"
#include "dpp_se_api.h"
#include "dpp_se_cfg.h"
#include "dpp_hash_crc.h"
#include "dpp_hash.h"
#include "dpp_acl.h"
#include "dpp_etcam.h"
#include "dpp_se.h"
#include "dpp_dtb.h"
#include "dpp_dtb_cfg.h"
#include "dpp_dev.h"
#include "dpp_sdt.h"
#include "dpp_dtb.h"
#include "dpp_apt_se.h"
#include "dpp_hash.h"
#include "dpp_agent_channel.h"
#include "dpp_kernel_init.h"
#include "dpp_stat_cfg.h"

extern ZXIC_UINT32 g_lpm_hw_dat_buf[LPM_HW_DAT_BUFF_SIZE_MAX];
extern ZXIC_UINT32 g_lpm_hw_dat_offset;

DPP_DTB_MIXED_TABLE_T *p_dtb_mixed_table_mgr = NULL;

ZXIC_UINT32 g_dpp_dtb_int_enable =  DISABLE;//默认中断不使能
ZXIC_UINT32 g_dtb_srh_mode = 1; //默认硬件模式 0：软件查找 1：硬件查找

static ZXIC_UINT32 g_dtb_cmd_endian = 0; //0:小端 1：大端


DPP_DTB_FIELD_T g_dtb_ddr_table_cmd_info[] = 
{
    {"valid", 127, 1},
    {"type_mode", 126, 3},
    {"rw_len", 123, 2},
    {"v46_flag", 121, 1},
    {"lpm_wr_vld", 120, 1},
    {"baddr", 119, 20},
    {"ecc_en", 99, 1},
    {"rw_addr", 29, 30},
};

DPP_DTB_FIELD_T g_dtb_eram_table_cmd_1_info[] = 
{
    {"valid", 127, 1},
    {"type_mode", 126, 3},
    {"data_mode",123, 2},
    {"cpu_wr", 121, 1},
    {"cpu_rd", 120, 1},
    {"cpu_rd_mode", 119, 1},
    {"addr", 113, 26},
    {"data_h",0,1},
};

DPP_DTB_FIELD_T g_dtb_eram_table_cmd_64_info[] = 
{
    {"valid", 127, 1},
    {"type_mode", 126, 3},
    {"data_mode",123, 2},
    {"cpu_wr", 121, 1},
    {"cpu_rd", 120, 1},
    {"cpu_rd_mode", 119, 1},
    {"addr", 113, 26},
    {"data_h",63,32},
    {"data_l",31,32},
};

DPP_DTB_FIELD_T g_dtb_eram_table_cmd_128_info[] = 
{
    {"valid", 127, 1},
    {"type_mode", 126, 3},
    {"data_mode",123, 2},
    {"cpu_wr", 121, 1},
    {"cpu_rd", 120, 1},
    {"cpu_rd_mode", 119, 1},
    {"addr", 113, 26},
};

DPP_DTB_FIELD_T g_dtb_zcam_table_cmd_info[] = 
{
    {"valid", 127, 1},
    {"type_mode", 126, 3},
    {"ram_reg_flag",123, 1},
    {"zgroup_id", 122, 2},
    {"zblock_id", 120, 3},
    {"zcell_id", 117, 2},
    {"mask", 115, 4},
    {"sram_addr", 111, 9},
};

DPP_DTB_FIELD_T g_dtb_etcam_table_cmd_info[] = 
{
    {"valid", 127, 1},
    {"type_mode", 126, 3},
    {"block_sel",123, 3},
    {"init_en", 120, 1},
    {"row_or_col_msk", 119, 1},
    {"vben", 118, 1},
    {"reg_tcam_flag", 117, 1},
    {"uload", 116, 8},
    {"rd_wr",108, 1},
    {"wr_mode", 107, 8},
    {"data_or_mask", 99, 1},
    {"addr", 98, 9},
    {"vbit", 89, 8},
};

DPP_DTB_FIELD_T g_dtb_mc_hash_table_cmd_info[] = 
{
    {"valid", 127, 1},
    {"type_mode", 126, 3},
    {"std_h", 63, 32},
    {"std_l", 31, 32},
};


DPP_DTB_TABLE_T g_dpp_dtb_table_info[] = 
{
    {
        "ddr",
        DTB_TABLE_DDR,
        8,
        g_dtb_ddr_table_cmd_info,
    },
    {
        "eram 1 bit",
        DTB_TABLE_ERAM_1,
        8,
        g_dtb_eram_table_cmd_1_info,
    },
    {
        "eram 64 bit",
        DTB_TABLE_ERAM_64,
        9,
        g_dtb_eram_table_cmd_64_info,
    },
    {
        "eram 128 bit",
        DTB_TABLE_ERAM_128,
        7,
        g_dtb_eram_table_cmd_128_info,
    },
    {
        "zcam",
        DTB_TABLE_ZCAM,
        8,
        g_dtb_zcam_table_cmd_info,
    },
    {
        "etcam",
        DTB_TABLE_ETCAM,
        13,
        g_dtb_etcam_table_cmd_info,
    },
    {
        "mc_hash",
        DTB_TABLE_MC_HASH,
        4,
        g_dtb_mc_hash_table_cmd_info
    },
};


DPP_DTB_FIELD_T g_dtb_eram_dump_cmd_info[] = 
{
    {"valid", 127, 1},
    {"up_type", 126, 2},
    {"base_addr", 106, 19},
    {"tb_depth", 83, 20},
    {"tb_dst_addr_h", 63, 32},
    {"tb_dst_addr_l", 31, 32},
};

DPP_DTB_FIELD_T g_dtb_ddr_dump_cmd_info[] = 
{
    {"valid", 127, 1},
    {"up_type", 126, 2},
    {"base_addr", 117, 30},
    {"tb_depth", 83, 20},
    {"tb_dst_addr_h", 63, 32},
    {"tb_dst_addr_l", 31, 32},

};

DPP_DTB_FIELD_T g_dtb_zcam_dump_cmd_info[] = 
{
    {"valid", 127, 1},
    {"up_type", 126, 2},
    {"zgroup_id", 124, 2},
    {"zblock_id", 122, 3},
    {"ram_reg_flag", 119, 1},
    {"z_reg_cell_id", 118, 2},
    {"sram_addr", 116, 9},
    {"tb_depth", 97, 10},
    {"tb_width", 65, 2},
    {"tb_dst_addr_h", 63, 32},
    {"tb_dst_addr_l", 31, 32},

};

DPP_DTB_FIELD_T g_dtb_etcam_dump_cmd_info[] = 
{
    {"valid", 127, 1},
    {"up_type", 126, 2},
    {"block_sel", 124, 3},
    {"addr", 121, 9},
    {"rd_mode", 112, 8},
    {"data_or_mask", 104, 1},
    {"tb_depth", 91, 10},
    {"tb_width", 81, 2},
    {"tb_dst_addr_h", 63, 32},
    {"tb_dst_addr_l", 31, 32},

};


DPP_DTB_TABLE_T g_dpp_dtb_dump_info[] = 
{
    {
        "eram",
        DTB_DUMP_ERAM,
        6,
        g_dtb_eram_dump_cmd_info,
    },
    {
        "ddr",
        DTB_DUMP_DDR,
        6,
        g_dtb_ddr_dump_cmd_info,
    },
    {
        "zcam",
        DTB_DUMP_ZCAM,
        11,
        g_dtb_zcam_dump_cmd_info,
    },
    {
        "etcam",
        DTB_DUMP_ETCAM,
        10,
        g_dtb_etcam_dump_cmd_info,
    },
};

/** dtb 中断配置
* @param   int_enable  中断使能
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_interrupt_status_set(ZXIC_UINT32 int_enable)
{
    g_dpp_dtb_int_enable = int_enable;
    
    return DPP_OK;
}

/** dtb 中断获取
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
ZXIC_UINT32 dpp_dtb_interrupt_status_get(ZXIC_VOID)
{
   return g_dpp_dtb_int_enable;
}

/** dtb cmd 大小端设置
* @param   int_enable  中断使能
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_cmd_endian_status_set(ZXIC_UINT32 endian)
{
    g_dtb_cmd_endian = endian;
    
    return DPP_OK;
}

/** dtb cmd 大小端获取
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_cmd_endian_status_get(ZXIC_VOID)
{   
    return g_dtb_cmd_endian;
}

/***********************************************************/
/** 获取 DTB 表属性信息
* @param   table_type  DTB表类型
*
* @return  
* @remark  无
* @see
* @author  cbb      @date  2022/09/02
************************************************************/
DPP_DTB_TABLE_T *dpp_table_info_get(ZXIC_UINT32 table_type)
{
    ZXIC_COMM_CHECK_INDEX_RETURN_NULL(table_type, 0, DTB_TABLE_ENUM_MAX - 1);

    return (&g_dpp_dtb_table_info[table_type]);
}

/***********************************************************/
/** 获取 DTB dump表属性信息
* @param   up_type  DTB表类型
*
* @return  
* @remark  无
* @see
* @author  cbb      @date  2022/09/02
************************************************************/
DPP_DTB_TABLE_T *dpp_dump_info_get(ZXIC_UINT32 up_type)
{
    ZXIC_COMM_CHECK_INDEX_RETURN_NULL(up_type, 0, DTB_DUMP_ENUM_MAX - 1);

    return (&g_dpp_dtb_dump_info[up_type]);
}

/*组装128bit数据格式接口*/
/** dtb写下表128bit格式数据
* @param   dev_id    设备号
* @param   table_type  dtb表类型
* @param   p_cmd_data  表格式命令数据
* @param   p_cmd_buff  表格式命令数据缓存
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_write_table_cmd(ZXIC_UINT32 dev_id, 
                                   DPP_DTB_TABLE_INFO_E table_type, 
                                   ZXIC_VOID *p_cmd_data,
                                   ZXIC_VOID *p_cmd_buff
                                   )
{
    DPP_STATUS          rc = DPP_OK;
    ZXIC_UINT32         field_cnt = 0;
    
    DPP_DTB_TABLE_T     *p_table_info;
    DPP_DTB_FIELD_T     *p_field_info = NULL;
    ZXIC_UINT32         temp_data = 0;

    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, table_type, 0, DTB_TABLE_ENUM_MAX - 1);
    ZXIC_COMM_CHECK_POINT(p_cmd_data);
    ZXIC_COMM_CHECK_POINT(p_cmd_buff);

    p_table_info = dpp_table_info_get(table_type);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_table_info);
    p_field_info = p_table_info->p_fields;
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_field_info);

    /* 提取各字段数据，按各字段实际bit位宽进行拼装 */
    for (field_cnt = 0; field_cnt < p_table_info->field_num; field_cnt++)
    {
        temp_data = *((ZXIC_UINT32 *)p_cmd_data + field_cnt) & ZXIC_COMM_GET_BIT_MASK(ZXIC_UINT32,  p_field_info[field_cnt].len);

        rc = zxic_comm_write_bits_ex((ZXIC_UINT8 *)p_cmd_buff,
                                DTB_TABLE_CMD_SIZE_BIT,
                                temp_data,
                                p_field_info[field_cnt].lsb_pos,
                                p_field_info[field_cnt].len);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "zxic_comm_write_bits");
    }

    return DPP_OK;
}

/** dtb写dump表128bit格式数据
* @param   dev_id    设备号
* @param   dump_type  dtb dump表类型
* @param   p_cmd_data  dump表格式命令数据
* @param   p_cmd_buff  dump表格式命令数据缓存
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_write_dump_cmd(ZXIC_UINT32 dev_id, 
                                   DPP_DTB_DUMP_INFO_E dump_type, 
                                   ZXIC_VOID *p_cmd_data,
                                   ZXIC_VOID *p_cmd_buff)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32         field_cnt = 0;
    
    DPP_DTB_TABLE_T     *p_table_info;
    DPP_DTB_FIELD_T     *p_field_info = NULL;
    ZXIC_UINT32         temp_data = 0;

    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dump_type, 0, DTB_DUMP_ENUM_MAX - 1);
    ZXIC_COMM_CHECK_POINT(p_cmd_data);
    ZXIC_COMM_CHECK_POINT(p_cmd_buff);

    p_table_info = dpp_dump_info_get(dump_type);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_table_info);
    p_field_info = p_table_info->p_fields;
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_field_info);

    /* 提取各字段数据，按各字段实际bit位宽进行拼装 */
    for (field_cnt = 0; field_cnt < p_table_info->field_num; field_cnt++)
    {

        temp_data = *((ZXIC_UINT32 *)p_cmd_data + field_cnt) & ZXIC_COMM_GET_BIT_MASK(ZXIC_UINT32,  p_field_info[field_cnt].len);

        rc = zxic_comm_write_bits_ex((ZXIC_UINT8 *)p_cmd_buff,
                        DTB_TABLE_CMD_SIZE_BIT,
                        temp_data,
                        p_field_info[field_cnt].lsb_pos,
                        p_field_info[field_cnt].len);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "zxic_comm_write_bits");
    }

    return DPP_OK;
}


/** 将下表格式+数据写入缓存中
 * entry，data_in_cmd_flag，如果为1，直接拷贝cmd，不拷贝data
* @param   p_data_buff    存放数据的buff头指针
* @param   addr_offset  当前数据要写入的位置（相对于buff头的偏移）
* @param   entry  要写入缓存的数据指针
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30/
************************************************************/
DPP_STATUS dpp_dtb_data_write(ZXIC_UINT8 * p_data_buff, 
                              ZXIC_UINT32 addr_offset, 
                              DPP_DTB_ENTRY_T *entry)
{
    ZXIC_UINT8*  p_cmd = p_data_buff + addr_offset;
    ZXIC_UINT32 cmd_size = DTB_TABLE_CMD_SIZE_BIT / 8;

    ZXIC_UINT8* p_data = p_cmd + cmd_size;
    ZXIC_UINT32 data_size = entry->data_size;

    ZXIC_UINT8 * cmd = (ZXIC_UINT8 *)entry->cmd;
    ZXIC_UINT8 * data = (ZXIC_UINT8 *)entry->data;

    ZXIC_COMM_CHECK_POINT(p_data_buff);
    ZXIC_COMM_CHECK_POINT(entry);

    /*写入命令数据*/
    ZXIC_COMM_MEMCPY_S(p_cmd, cmd_size , cmd, cmd_size);

    /*写入数据*/
    if(!entry->data_in_cmd_flag)
    {
        zxic_comm_swap(data, data_size);
        ZXIC_COMM_MEMCPY_S(p_data, data_size, data, data_size);
    }

    return DPP_OK;
}

/** 下表数据写入命令寄存器
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   down_table_len   数据长度,单位:字节;
* @param   p_down_table_buff 下表数据缓存
* @param   p_element_id   返回下表使用的元素id
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_write_down_table_data(DPP_DEV_T *dev, 
                                      ZXIC_UINT32 queue_id, 
                                      ZXIC_UINT32 down_table_len,
                                      ZXIC_UINT8* p_down_table_buff,
                                      ZXIC_UINT32 *p_element_id
                                      )
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 dtb_interrupt_status = 0;
    ZXIC_UINT32 dtb_down_check_times = 2;
    ZXIC_UINT32 element_id = 0;

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);

    rc = dpp_dtb_dev_status_check(dev);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_dev_status_check");

    dtb_interrupt_status = dpp_dtb_interrupt_status_get();

    while(dtb_down_check_times)
    {
        rc = dpp_dtb_tab_down_info_set(dev, 
                                       queue_id, 
                                       dtb_interrupt_status, 
                                       down_table_len / 4, 
                                       (ZXIC_UINT32 *)p_down_table_buff, 
                                       &element_id);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc,"dpp_dtb_tab_down_info_set");

        rc = dpp_dtb_tab_down_success_status_check(dev, queue_id, element_id);
        
        if(rc != DPP_RC_DTB_OVER_TIME)
        {
            break;
        }

        dtb_down_check_times --;

        if(dtb_down_check_times > 0)
        {
            ZXIC_COMM_PRINT("DTB DOWN TABLE OVERTIME, DOWN TABLE AGAIN----%d!\n", dtb_down_check_times);
        }

    }

    *p_element_id = element_id;

    ZXIC_COMM_TRACE_INFO("down slot: %d, queue_id: %d, element id: %d\n", DEV_PCIE_SLOT(dev), queue_id, *p_element_id);

    return rc;
}

/** 计算eram 128bit为单位的index
* @param   dev_id        设备号
* @param   eram_mode     eram 位宽模式
* @param   index         以eram_mode为单位的index
* @param   p_row_index   出参，行
* @param   p_col_index   出参，列
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dtb_eram_index_cal(DPP_DEV_T *dev, 
                              ZXIC_UINT32 eram_mode, 
                              ZXIC_UINT32 index,
                              ZXIC_UINT32 *p_row_index, 
                              ZXIC_UINT32* p_col_index)
{
    DPP_STATUS  rc = DPP_OK;

    ZXIC_UINT32 row_index = 0;
    ZXIC_UINT32 col_index = 0;

    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_row_index);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_col_index);

    switch (eram_mode)
    {
        case ERAM128_TBL_128b:
        {
            row_index = index;
            break;
        }

        case ERAM128_TBL_64b:
        {
            row_index = (index >> 1);
            col_index = index & 0x1;
            break;
        }

        case ERAM128_TBL_1b:
        {
            row_index = (index >> 7);
            col_index = index & 0x7F;
            break;
        }
    }

    *p_row_index = row_index;
    *p_col_index = col_index;

    return rc;
}

#if ZXIC_REAL("DTB BASE INTERFACE")

/** smmu0数据组装函数,输入格式字段和数据，输出一个entry
* @param   dev_id    芯片id
* @param   mode  位宽模式
* @param   addr  1bit为单位的索引
* @param   p_data  要写入数据
* @param   p_entry  组装好的条目(已分配好空间)
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30/
************************************************************/
DPP_STATUS dpp_dtb_smmu0_write_entry_data(DPP_DEV_T *dev, 
                                            ZXIC_UINT32 mode, 
                                            ZXIC_UINT32 addr, 
                                            ZXIC_UINT32 *p_data, 
                                            DPP_DTB_ENTRY_T *p_entry)
{

    DPP_STATUS  rc = DPP_OK;
    DPP_DTB_ERAM_TABLE_FORM_T dtb_eram_form_info = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev),DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_entry);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), mode, ERAM128_OPR_128b, ERAM128_OPR_32b);

    dtb_eram_form_info.valid = DTB_TABLE_VALID;
    dtb_eram_form_info.type_mode = DTB_TABLE_MODE_ERAM;
    dtb_eram_form_info.data_mode = mode;
    dtb_eram_form_info.cpu_wr = 1;
    dtb_eram_form_info.addr = addr;
    dtb_eram_form_info.cpu_rd = 0;
    dtb_eram_form_info.cpu_rd_mode = 0;

    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_PRINT("dpp eram form info: \n");
        ZXIC_COMM_DBGCNT32_PRINT("valid", dtb_eram_form_info.valid);
        ZXIC_COMM_DBGCNT32_PRINT("type_mode", dtb_eram_form_info.type_mode);
        ZXIC_COMM_DBGCNT32_PRINT("data_mode", dtb_eram_form_info.data_mode);
        ZXIC_COMM_DBGCNT32_PRINT("cpu_wr", dtb_eram_form_info.cpu_wr);
        ZXIC_COMM_DBGCNT32_PRINT("addr", dtb_eram_form_info.addr);
        ZXIC_COMM_DBGCNT32_PRINT("cpu_rd", dtb_eram_form_info.cpu_rd);
        ZXIC_COMM_DBGCNT32_PRINT("cpu_rd_mode", dtb_eram_form_info.cpu_rd_mode);
    }

    /*清空p_entry中数据*/
    if(ERAM128_OPR_128b == mode)
    {
        p_entry->data_in_cmd_flag = 0;
        p_entry->data_size = 128 / 8;

        rc = dpp_dtb_write_table_cmd(DEV_ID(dev), DTB_TABLE_ERAM_128, &dtb_eram_form_info, p_entry->cmd);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_write_table_cmd");

        ZXIC_COMM_MEMCPY(p_entry->data, p_data,128 / 8);
    }
    else if(ERAM128_OPR_64b == mode)
    {
        p_entry->data_in_cmd_flag = 1;
        p_entry->data_size  = 64 / 8;
        dtb_eram_form_info.data_l = *(p_data + 1);
        dtb_eram_form_info.data_h = *(p_data);

        rc = dpp_dtb_write_table_cmd(DEV_ID(dev), DTB_TABLE_ERAM_64, &dtb_eram_form_info, p_entry->cmd);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_write_table_cmd");

    }else if(ERAM128_OPR_1b == mode)
    {
        p_entry->data_in_cmd_flag = 1;
        p_entry->data_size  = 1;
        dtb_eram_form_info.data_h = *(p_data);

        rc = dpp_dtb_write_table_cmd(DEV_ID(dev), DTB_TABLE_ERAM_1, &dtb_eram_form_info, p_entry->cmd);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_write_table_cmd");
    }
    
    return DPP_OK;
}

/** smmu1数据组装函数,输入格式字段和数据，输出一个entry
* @param   dev_id    芯片id
* @param   rw_len  位宽 0-128bit, 1-256bit, 2-384bit, 3-512bit
* @param   v46_flag  ipv4,ipv6flag 
* @param   lpm_wr_vld  lpm表写标识 lpm表数据时为1
* @param   base_addr  基地址 以4K*128为单位
* @param   index  以位宽为单位的索引
* @param   ecc_en  ecc使能
* @param   p_data  要写入数据
* @param   p_entry  组装好的条目(已分配好空间) data空间大小为512bit
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30/
************************************************************/
DPP_STATUS dpp_dtb_smmu1_write_entry_data(ZXIC_UINT32 dev_id, 
                                        ZXIC_UINT32 rw_len,
                                        ZXIC_UINT32 v46_flag,
                                        ZXIC_UINT32 lpm_wr_vld,
                                        ZXIC_UINT32 base_addr,
                                        ZXIC_UINT32 index,
                                        ZXIC_UINT32 ecc_en,
                                        ZXIC_UINT8 *p_data, 
                                        DPP_DTB_ENTRY_T *p_entry)
{
    DPP_STATUS rc = DPP_OK;

    DPP_DTB_DDR_TABLE_FORM_T dtb_ddr_form_info = {0}; /*DDR表格式*/

    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_UPPER(rw_len,SMMU1_DDR_WRT_512b);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_data);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_entry);

        /*DTB下表格式内容填充*/
    dtb_ddr_form_info.valid = DTB_TABLE_VALID;
    dtb_ddr_form_info.type_mode = DTB_TABLE_MODE_DDR;
    dtb_ddr_form_info.rw_len = rw_len;
    dtb_ddr_form_info.v46_flag = v46_flag;
    dtb_ddr_form_info.lpm_wr_vld = lpm_wr_vld;
    dtb_ddr_form_info.baddr = base_addr;
    dtb_ddr_form_info.ecc_en = ecc_en;
    dtb_ddr_form_info.rw_addr = index;

    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_PRINT("dtb_ddr_form_info:\n");
        ZXIC_COMM_DBGCNT32_PRINT("valid", dtb_ddr_form_info.valid);
        ZXIC_COMM_DBGCNT32_PRINT("type_mode", dtb_ddr_form_info.type_mode);
        ZXIC_COMM_DBGCNT32_PRINT("rw_len", dtb_ddr_form_info.rw_len);
        ZXIC_COMM_DBGCNT32_PRINT("v46_flag", dtb_ddr_form_info.v46_flag);
        ZXIC_COMM_DBGCNT32_PRINT("lpm_wr_vld", dtb_ddr_form_info.lpm_wr_vld);
        ZXIC_COMM_DBGCNT32_PRINT("baddr",  dtb_ddr_form_info.baddr);
        ZXIC_COMM_DBGCNT32_PRINT("ecc_en", dtb_ddr_form_info.ecc_en);
        ZXIC_COMM_DBGCNT32_PRINT("rw_addr", dtb_ddr_form_info.rw_addr);
    }

    p_entry->data_in_cmd_flag = 0;
    p_entry->data_size = DTB_LEN_POS_SETP * (rw_len + 1);

    rc = dpp_dtb_write_table_cmd(dev_id, DTB_TABLE_DDR, &dtb_ddr_form_info, p_entry->cmd);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_write_table_cmd");

    ZXIC_COMM_MEMCPY(p_entry->data, p_data, DTB_LEN_POS_SETP * (rw_len + 1));
    
    return DPP_OK;
}


/** zcam数据组装函数,输入格式字段和数据，输出一个entry
* @param   dev_id    芯片id
* @param   reg_sram_flag  读写ZCAM寄存器/sram标志位：1'b1：读写寄存器 1'b0：读写sram
* @param   zgroup_id  
* @param   zblock_id  
* @param   zcell_id  
* @param   sram_addr  512bit为单位的地址
* @param   mask  写掩码：mask[3:0]分别对应CPU写数据的4个128-bit
*                       mask[0]对应[127:0],1'b0为不写该128-bit，1'b1为写；
*                       mask[1]对应[255:128]；
*                       mask[2]对应[383:256]；
*                       mask[3]对应[512:384]
* @param   p_data  要写入数据
* @param   p_entry  组装好的条目(已分配好空间) data空间大小为512bit
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30/
************************************************************/
DPP_STATUS dpp_dtb_zcam_write_entry_data(ZXIC_UINT32 dev_id, 
                                        ZXIC_UINT32 reg_sram_flag,
                                        ZXIC_UINT32 zgroup_id,
                                        ZXIC_UINT32 zblock_id,
                                        ZXIC_UINT32 zcell_id,
                                        ZXIC_UINT32 sram_addr,
                                        ZXIC_UINT32 mask,
                                        ZXIC_UINT8 *p_data, 
                                        DPP_DTB_ENTRY_T *p_entry)
{
    DPP_STATUS rc = DPP_OK;

    DPP_DTB_ZCAM_TABLE_FORM_T dtb_zcam_form_info = {0}; /*ZCAM表格式*/

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, zgroup_id, 0, SE_ZGRP_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, zblock_id, 0, ZBLK_NUM_PER_ZGRP - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, zcell_id, 0, SE_ZCELL_NUM - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_data);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_entry);

    dtb_zcam_form_info.valid = DTB_TABLE_VALID;
    dtb_zcam_form_info.type_mode = DTB_TABLE_MODE_ZCAM;
    dtb_zcam_form_info.ram_reg_flag = reg_sram_flag;
    dtb_zcam_form_info.zgroup_id = zgroup_id;
    dtb_zcam_form_info.zblock_id = zblock_id;
    dtb_zcam_form_info.zcell_id = zcell_id;
    dtb_zcam_form_info.mask = mask;
    dtb_zcam_form_info.sram_addr = sram_addr & 0x1FF;

    p_entry->data_in_cmd_flag = 0;
    p_entry->data_size = DTB_LEN_POS_SETP * (DTB_ZCAM_LEN_SIZE - 1);

    rc = dpp_dtb_write_table_cmd(dev_id, DTB_TABLE_ZCAM, &dtb_zcam_form_info, p_entry->cmd);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_write_table_cmd");

    ZXIC_COMM_MEMCPY(p_entry->data, p_data, DTB_LEN_POS_SETP * (DTB_ZCAM_LEN_SIZE - 1));

    return DPP_OK;
}


/** etcam数据组装函数,输入格式字段和数据，输出一个entry
* @param   dev_id    芯片id
* @param   block_idx         block索引 0 - 7
* @param   row_or_col_msk    1 write row mask reg  0:write col mask reg
* @param   vben              enable the valid bit addressed by addr
* @param   reg_tcam_flag     1:配置内部row_col_mask寄存器 0：读写tcam
* @param   flush             使能标识删除对应addr的表项条目，（80bit为单位，含义与wr_mode一一对应）
* @param   rd_wr             读写标志 0写 1读
* @param   wr_mode           写入掩码，最高8bit，对应bit为1代表对应的80bit的数据
* @param   data_or_mask      数据或掩码标志 1：写x(data),0:写y(mask)
* @param   ram_addr              etcam地址（0-511）
* @param   vbit              valid bit input
* @param   p_data            要写入数据
* @param   p_entry           组装好的条目(已分配好空间) data空间大小为640bit
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30/
************************************************************/
DPP_STATUS dpp_dtb_etcam_write_entry_data(DPP_DEV_T *dev, 
                                        ZXIC_UINT32 block_idx,
                                        ZXIC_UINT32 row_or_col_msk,
                                        ZXIC_UINT32 vben,
                                        ZXIC_UINT32 reg_tcam_flag,
                                        ZXIC_UINT32 flush,
                                        ZXIC_UINT32 rd_wr,
                                        ZXIC_UINT32 wr_mode,
                                        ZXIC_UINT32 data_or_mask,
                                        ZXIC_UINT32 ram_addr,
                                        ZXIC_UINT32 vbit,
                                        ZXIC_UINT8 *p_data, 
                                        DPP_DTB_ENTRY_T *p_entry)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 offset = 0;
    ZXIC_UINT8 *p_temp = NULL;

    ZXIC_UINT8  buff[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    DPP_DTB_ETCAM_TABLE_FORM_T dtb_etcam_form_info = {0}; /*etcam表格式*/

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), block_idx, 0, DPP_ETCAM_BLOCK_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), row_or_col_msk, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), vben, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), reg_tcam_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flush, 0, DPP_ETCAM_WR_MASK_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), rd_wr, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), wr_mode, 0, DPP_ETCAM_WR_MASK_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), data_or_mask, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), ram_addr, 0, DPP_ETCAM_RAM_DEPTH - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), vbit, 0, 0xff);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_entry);

    /*data etcam 下表格式内容填充*/
    dtb_etcam_form_info.valid = DTB_TABLE_VALID;
    dtb_etcam_form_info.type_mode = DTB_TABLE_MODE_ETCAM;
    dtb_etcam_form_info.block_sel = block_idx;
    dtb_etcam_form_info.init_en  = 0;
    dtb_etcam_form_info.row_or_col_msk = row_or_col_msk;
    dtb_etcam_form_info.vben = vben;
    dtb_etcam_form_info.reg_tcam_flag = reg_tcam_flag;
    dtb_etcam_form_info.uload = flush;
    dtb_etcam_form_info.rd_wr = rd_wr;//0:写;1读
    dtb_etcam_form_info.wr_mode = wr_mode;
    dtb_etcam_form_info.data_or_mask = data_or_mask;
    dtb_etcam_form_info.addr = ram_addr;
    dtb_etcam_form_info.vbit = vbit;

    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_PRINT("dpp etcam form info: \n");
        ZXIC_COMM_DBGCNT32_PRINT("valid", dtb_etcam_form_info.valid);
        ZXIC_COMM_DBGCNT32_PRINT("type_mode", dtb_etcam_form_info.type_mode);
        ZXIC_COMM_DBGCNT32_PRINT("block_sel", dtb_etcam_form_info.block_sel);
        ZXIC_COMM_DBGCNT32_PRINT("init_en", dtb_etcam_form_info.init_en);
        ZXIC_COMM_DBGCNT32_PRINT("row_or_col_msk", dtb_etcam_form_info.row_or_col_msk);
        ZXIC_COMM_DBGCNT32_PRINT("vben", dtb_etcam_form_info.vben);
        ZXIC_COMM_DBGCNT32_PRINT("reg_tcam_flag", dtb_etcam_form_info.reg_tcam_flag);
        ZXIC_COMM_DBGCNT32_PRINT("uload", dtb_etcam_form_info.uload);
        ZXIC_COMM_DBGCNT32_PRINT("rd_wr", dtb_etcam_form_info.rd_wr);
        ZXIC_COMM_DBGCNT32_PRINT("wr_mode", dtb_etcam_form_info.wr_mode);
        ZXIC_COMM_DBGCNT32_PRINT("data_or_mask", dtb_etcam_form_info.data_or_mask);
        ZXIC_COMM_DBGCNT32_PRINT("addr", dtb_etcam_form_info.addr);
        ZXIC_COMM_DBGCNT32_PRINT("vbit", dtb_etcam_form_info.vbit);
    }

    p_entry->data_in_cmd_flag = 0;
    p_entry->data_size = DTB_LEN_POS_SETP * (DTB_ETCAM_LEN_SIZE - 1);

    rc = dpp_dtb_write_table_cmd(DEV_ID(dev), DTB_TABLE_ETCAM, &dtb_etcam_form_info, p_entry->cmd);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_write_table_cmd");

    p_temp = p_data;

    /*将数据写入ETCAM表项处理*/
    /* 160bit key: high 80bit in tcam_ram1, low 80bit in tcam_ram0, and so on. */
    for (i = 0; i < DPP_ETCAM_RAM_NUM; i++)
    {
        offset = i * ((ZXIC_UINT32)DPP_ETCAM_WIDTH_MIN / 8);

        if ((wr_mode >> (DPP_ETCAM_RAM_NUM - 1 - i)) & 0x1)
        {
            ZXIC_COMM_MEMCPY(buff + offset, p_temp, DPP_ETCAM_WIDTH_MIN / 8);
            p_temp += DPP_ETCAM_WIDTH_MIN / 8;
        }
    }

    zxic_comm_swap((ZXIC_UINT8 *)buff, DTB_LEN_POS_SETP * (DTB_ETCAM_LEN_SIZE - 1));

    ZXIC_COMM_MEMCPY(p_entry->data, buff, DTB_LEN_POS_SETP * (DTB_ETCAM_LEN_SIZE - 1));

    return DPP_OK;
}

/** 写eram，输出一个entry
* @param   dev_id     芯片id
* @param   base_addr  基地址，以128bit为单位
* @param   index      条目索引，以mode为单位
* @param   wr_mode    数据位宽模式,支持0：128bit 1：64bit  2:1bit
* @param   p_data     待写入的数据
* @param   p_entry    组装好的数据
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30/
************************************************************/
DPP_STATUS dpp_dtb_se_smmu0_ind_write(DPP_DEV_T *dev,
                                  ZXIC_UINT32 base_addr,
                                  ZXIC_UINT32 index,
                                  ZXIC_UINT32 wrt_mode,
                                  ZXIC_UINT32 *p_data,
                                  DPP_DTB_ENTRY_T *p_entry)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 temp_idx = 0;
    ZXIC_UINT32 dtb_ind_addr = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev),DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_entry);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), wrt_mode, ERAM128_OPR_128b, ERAM128_OPR_1b);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), base_addr, 0, SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1);

    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_PRINT("dpp_dtb_se_smmu0_ind_write: \n");
        ZXIC_COMM_PRINT("base addr: 0x%08x\n",base_addr);
        ZXIC_COMM_PRINT("index: 0x%08x\n",index);
        ZXIC_COMM_PRINT("write mode: %d 0-128bit 1-64bit  2-1bit\n",wrt_mode);
    }

    switch (wrt_mode)
    {
        case ERAM128_OPR_128b:
        {
            if((0xFFFFFFFF - (base_addr)) < (index))
            {
                ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev),  "ICM %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, base_addr, index);

                return ZXIC_PAR_CHK_INVALID_INDEX;
            }
            if (base_addr + index > SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1)
            {
                ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev),  "dpp_dtb_se_smmu0_ind_write : index out of range !\n");
                return DPP_ERR;
            }

            temp_idx = index << 7;

            break;
        }

        case ERAM128_OPR_64b:
        {
            if ((base_addr + (index >> 1)) > SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1)
            {
                ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev),  "dpp_dtb_se_smmu0_ind_write : index out of range !\n");
                return DPP_ERR;
            }

            temp_idx = index << 6;

            break;
        }

        case ERAM128_OPR_1b:
        {
            if ((base_addr + (index >> 7)) > SE_SMMU0_ERAM_ADDR_NUM_TOTAL - 1)
            {
                ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev),  "dpp_dtb_se_smmu0_ind_write : index out of range !\n");
                return DPP_ERR;
            }

            temp_idx = index;
        }
    }

    if((0xFFFFFFFF - (temp_idx)) < ((base_addr << 7) & DPP_ERAM128_BADDR_MASK))
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev),  "ICM %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, temp_idx, ((base_addr << 7) & DPP_ERAM128_BADDR_MASK));
        return ZXIC_PAR_CHK_INVALID_INDEX;
    }

    dtb_ind_addr = ((base_addr << 7) & DPP_ERAM128_BADDR_MASK) + temp_idx;

    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_DBGCNT32_PRINT(" dtb eram item 1bit addr", dtb_ind_addr);
    }
    
    rc = dpp_dtb_smmu0_write_entry_data(dev, 
                          wrt_mode, 
                          dtb_ind_addr,  
                          p_data,  
                          p_entry);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_smmu0_write_entry_data");

    return DPP_OK;
}

/** dtb写smmu0中的数据，数据长度在16K范围内
* @param   dev_id          设备号
* @param   queue_id        队列号
* @param   smmu0_base_addr smmu0基地址，以128bit为单位
* @param   smmu0_wr_mode   smmu0写模式，参考DPP_ERAM128_OPR_MODE_E，仅支持128bit、64bit、1bit模式
* @param   entry_num       下发的条目数
* @param   p_entry_arr     待下发表项内容结构体数组指针
* @param   element_id      返回下表使用的元素id
* @return
* @remark  无
* @see
* @author  cbb      @date  2024/01/04
************************************************************/
DPP_STATUS dpp_dtb_smmu0_data_write_cycle(DPP_DEV_T *dev,
                                         ZXIC_UINT32 queue_id,
                                         ZXIC_UINT32 smmu0_base_addr,
                                         ZXIC_UINT32 smmu0_wr_mode,
                                         ZXIC_UINT32 entry_num,
                                         DPP_DTB_ERAM_ENTRY_INFO_T *p_entry_arr,
                                         ZXIC_UINT32 *element_id)
{
    DPP_STATUS  rc = DPP_OK;

    ZXIC_UINT32 item_cnt = 0;
    ZXIC_UINT32 addr_offset = 0;
    ZXIC_UINT32 dtb_len = 0;
    ZXIC_UINT32 index = 0;

    ZXIC_UINT32 *p_entry_data = NULL;
    ZXIC_UINT8 *table_data_buff = NULL;
    ZXIC_UINT32 entry_data_buff[4] = {0};
    ZXIC_UINT8 cmd_buff[DTB_TABLE_CMD_SIZE_BIT / 8] = {0};
    DPP_DTB_ENTRY_T   dtb_one_entry = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_LOWER(DEV_ID(dev), entry_num, 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_entry_arr);

    //分配下表数据缓存 16K
    table_data_buff = (ZXIC_UINT8 *) ZXIC_COMM_MALLOC(DPP_DTB_TABLE_DATA_BUFF_SIZE * sizeof(ZXIC_UINT8));
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), table_data_buff);
    ZXIC_COMM_MEMSET(table_data_buff, 0, DPP_DTB_TABLE_DATA_BUFF_SIZE * sizeof(ZXIC_UINT8));

    dtb_one_entry.cmd = cmd_buff;
    dtb_one_entry.data = (ZXIC_UINT8 *)entry_data_buff;

    for(item_cnt = 0; item_cnt < entry_num; ++item_cnt)
    {
        p_entry_data = (ZXIC_UINT32 *)p_entry_arr[item_cnt].p_data;
        ZXIC_COMM_CHECK_POINT_MEMORY_FREE_NO_ASSERT(p_entry_data, table_data_buff);
        index = p_entry_arr[item_cnt].index;
        
        //将一个数据写入entry
        rc = dpp_dtb_se_smmu0_ind_write(dev,
                                        smmu0_base_addr, 
                                        index, 
                                        smmu0_wr_mode,
                                        p_entry_data,
                                        &dtb_one_entry);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_se_smmu0_ind_write", table_data_buff);

        switch (smmu0_wr_mode)
        {
            case ERAM128_OPR_128b:
            {
                dtb_len += 2; 
                addr_offset = item_cnt * DTB_LEN_POS_SETP * 2;
                break;
            }

            case ERAM128_OPR_64b:
            {
                dtb_len += 1; 
                addr_offset = item_cnt * DTB_LEN_POS_SETP;
                break;
            }

            case ERAM128_OPR_1b:
            {
                dtb_len += 1;
                addr_offset = item_cnt * DTB_LEN_POS_SETP;
                break;
            }
        }

        /*将表格式和数据写入缓存buff中*/
        rc = dpp_dtb_data_write(table_data_buff, addr_offset, &dtb_one_entry);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_data_write", table_data_buff);
        ZXIC_COMM_MEMSET(cmd_buff, 0, DTB_TABLE_CMD_SIZE_BIT / 8);
        ZXIC_COMM_MEMSET(entry_data_buff, 0, 4 * sizeof(ZXIC_UINT32));
    }
     
    rc = dpp_dtb_write_down_table_data(dev,  
                                       queue_id, 
                                       dtb_len * 16,
                                       table_data_buff,
                                       element_id);
    ZXIC_COMM_FREE(table_data_buff);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_write_down_table_data");

    return DPP_OK;
}

/** dtb写smmu0中的数据，数据长度不限
* @param   dev_id          设备号
* @param   queue_id        队列号
* @param   smmu0_base_addr smmu0基地址，以128bit为单位
* @param   smmu0_wr_mode   smmu0写模式，参考DPP_ERAM128_OPR_MODE_E，仅支持128bit、64bit、1bit模式
* @param   entry_num       下发的条目数
* @param   p_entry_arr     待下发表项内容结构体数组指针
* @param   element_id      返回下表使用的元素id
* @return
* @remark  无
* @see
* @author  cbb      @date  2024/01/04
************************************************************/
DPP_STATUS dpp_dtb_smmu0_data_write(DPP_DEV_T *dev,
                                    ZXIC_UINT32 queue_id,
                                    ZXIC_UINT32 smmu0_base_addr,
                                    ZXIC_UINT32 smmu0_wr_mode,
                                    ZXIC_UINT32 entry_num,
                                    DPP_DTB_ERAM_ENTRY_INFO_T *p_entry_arr,
                                    ZXIC_UINT32 *element_id)
{
    DPP_STATUS  rc = DPP_OK;

    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 entry_num_max = 0;
    ZXIC_UINT32 entry_cycle = 0;
    ZXIC_UINT32 entry_remains = 0;

    DPP_DTB_ERAM_ENTRY_INFO_T *p_entry = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_entry_arr);

    switch (smmu0_wr_mode)
    {
        case ERAM128_OPR_128b:
        {
            entry_num_max = 0x1ff;
            break;
        }

        case ERAM128_OPR_64b:
        {
            entry_num_max = 0x3ff;
            break;
        }

        case ERAM128_OPR_1b:
        {
            entry_num_max = 0x3ff;
            break;
        }
    }

    ZXIC_COMM_CHECK_INDEX_EQUAL(entry_num_max, 0);
    entry_cycle = entry_num / entry_num_max;
    entry_remains = entry_num % entry_num_max;

    for(i = 0; i < entry_cycle; ++i)
    {
        p_entry = p_entry_arr + entry_num_max * i;
        rc = dpp_dtb_smmu0_data_write_cycle(dev,
                                            queue_id,
                                            smmu0_base_addr,
                                            smmu0_wr_mode,
                                            entry_num_max,
                                            p_entry,
                                            element_id);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_smmu0_data_write_cycle");

        ZXIC_COMM_TRACE_INFO("slot [%d] queue [%d] element_id [%d] done.\n",DEV_PCIE_SLOT(dev), queue_id, *element_id);

    }

    if(entry_remains)
    {
        p_entry = p_entry_arr + entry_num_max * entry_cycle;
        rc = dpp_dtb_smmu0_data_write_cycle(dev,
                                            queue_id,
                                            smmu0_base_addr,
                                            smmu0_wr_mode,
                                            entry_remains,
                                            p_entry,
                                            element_id);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_smmu0_data_write_cycle");

        ZXIC_COMM_TRACE_INFO("slot [%d] queue [%d] element_id [%d] done.\n",DEV_PCIE_SLOT(dev), queue_id, *element_id);
    }

    return DPP_OK;
}

/** dtb flush smmu0中的数据，数据长度16K以内
* @param   dev_id          设备号
* @param   queue_id        队列号
* @param   smmu0_base_addr smmu0基地址，以128bit为单位
* @param   smmu0_wr_mode   smmu0写模式，参考DPP_ERAM128_OPR_MODE_E，仅支持128bit、64bit、1bit模式
* @param   start_index     flush开始的条目
* @param   entry_num       下发的条目数
* @param   element_id      返回下表使用的元素id
* @return
* @remark  无
* @see
* @author  cbb      @date  2024/01/04
************************************************************/
DPP_STATUS dpp_dtb_smmu0_flush_cycle(DPP_DEV_T *dev, 
                                     ZXIC_UINT32 queue_id,
                                     ZXIC_UINT32 smmu0_base_addr,
                                     ZXIC_UINT32 smmu0_wr_mode,
                                     ZXIC_UINT32 start_index,
                                     ZXIC_UINT32 entry_num,
                                     ZXIC_UINT32 *element_id)
{
    DPP_STATUS  rc = DPP_OK;

    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 current_index = 0;
    ZXIC_UINT32 entry_data_buff[4] = {0};
    DPP_DTB_ERAM_ENTRY_INFO_T *p_entry_arr = NULL;

    ZXIC_COMM_CHECK_POINT(dev);

    p_entry_arr = (DPP_DTB_ERAM_ENTRY_INFO_T *)ZXIC_COMM_MALLOC(entry_num * sizeof(DPP_DTB_ERAM_ENTRY_INFO_T));
    ZXIC_COMM_CHECK_POINT(p_entry_arr);
    ZXIC_COMM_MEMSET(p_entry_arr, 0, entry_num * sizeof(DPP_DTB_ERAM_ENTRY_INFO_T));

    for(index = 0; index < entry_num; index++)
    {
        current_index = start_index + index;

        p_entry_arr[index].index = current_index;
        p_entry_arr[index].p_data = entry_data_buff;
    }

    rc = dpp_dtb_smmu0_data_write_cycle(dev,
                                        queue_id,
                                        smmu0_base_addr,
                                        smmu0_wr_mode,
                                        entry_num,
                                        p_entry_arr,
                                        element_id); 
    ZXIC_COMM_FREE(p_entry_arr);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_smmu1_data_write_cycle");

    return rc;
}

/** dtb flush smmu0中的数据,大数据量
* @param   dev_id          设备号
* @param   queue_id        队列号
* @param   smmu0_base_addr smmu0基地址，以128bit为单位
* @param   smmu0_wr_mode   smmu0写模式，参考DPP_ERAM128_OPR_MODE_E，仅支持128bit、64bit、1bit模式
* @param   start_index     flush开始的条目
* @param   entry_num       下发的条目数
* @param   element_id      返回下表使用的元素id
* @return
* @remark  无
* @see
* @author  cbb      @date  2024/01/04
************************************************************/
DPP_STATUS dpp_dtb_smmu0_flush(DPP_DEV_T *dev,
                               ZXIC_UINT32 queue_id,
                               ZXIC_UINT32 smmu0_base_addr,
                               ZXIC_UINT32 smmu0_wr_mode,
                               ZXIC_UINT32 start_index,
                               ZXIC_UINT32 entry_num,
                               ZXIC_UINT32 *element_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 entry_num_max = 0;
    ZXIC_UINT32 entry_cycle = 0;
    ZXIC_UINT32 entry_remains = 0;
    ZXIC_UINT32 temp_start_index = 0;
    ZXIC_UINT32 i = 0;

    ZXIC_COMM_CHECK_POINT(dev);

    switch (smmu0_wr_mode)
    {
        case ERAM128_OPR_128b:
        {
            entry_num_max = 0x1ff;
            break;
        }

        case ERAM128_OPR_64b:
        {
            entry_num_max = 0x3ff;
            break;
        }

        case ERAM128_OPR_1b:
        {
            entry_num_max = 0x3ff;
            break;
        }
    }

    ZXIC_COMM_CHECK_INDEX_EQUAL(entry_num_max, 0);
    entry_cycle = entry_num / entry_num_max;
    entry_remains = entry_num % entry_num_max;

    for(i = 0; i < entry_cycle; ++i)
    {
        temp_start_index = entry_num_max * i + start_index;

        rc = dpp_dtb_smmu0_flush_cycle(dev, 
                                       queue_id, 
                                       smmu0_base_addr, 
                                       smmu0_wr_mode, 
                                       temp_start_index,
                                       entry_num_max, 
                                       element_id);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_smmu0_flush_cycle");

        ZXIC_COMM_TRACE_INFO("dpp_dtb_smmu0_flush_cycle[%d] element_id = %d\n",i,*element_id);
    }

    if(entry_remains)
    {
        temp_start_index = entry_num_max * entry_cycle + start_index;
        rc = dpp_dtb_smmu0_flush_cycle(dev, 
                                       queue_id, 
                                       smmu0_base_addr, 
                                       smmu0_wr_mode, 
                                       temp_start_index,
                                       entry_remains, 
                                       element_id);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_smmu0_flush_cycle");

        ZXIC_COMM_TRACE_INFO("dpp_dtb_smmu0_flush_cycle: element_id = %d\n",*element_id);
    }

    return 0;
}

/** ddr直接表 写数据
* @param   dev_id    芯片id
* @param   base_addr  基地址，以4K*128bit为单位
* @param   rw_len  数据位宽模式，位宽 0-128bit, 1-256bit, 2-384bit, 3-512bit,取值参考SMMU1_DDR_WRT_MODE_E的定义
* @param   index   条目索引，以mode为单位
* @param   ecc_en   直接表ECC使能位
* @param   p_data   待写入的数据指针
* @param   p_entry   组装好的条目(已分配好空间)
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30/
************************************************************/
DPP_STATUS dpp_dtb_ddr_dir_table_data_write(ZXIC_UINT32 dev_id, 
                                        ZXIC_UINT32 base_addr,
                                        ZXIC_UINT32 rw_len,
                                        ZXIC_UINT32 index,
                                        ZXIC_UINT32 ecc_en,
                                        ZXIC_UINT8 *p_data, 
                                        DPP_DTB_ENTRY_T *p_entry)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 ipv4_v6_flag = 0;
    ZXIC_UINT32 lpm_vld = 0;

    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_UPPER(rw_len,SMMU1_DDR_WRT_512b);
    ZXIC_COMM_CHECK_INDEX_UPPER(ecc_en,1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_data);

    rc =  dpp_dtb_smmu1_write_entry_data(dev_id, 
                                        rw_len,
                                        ipv4_v6_flag,
                                        lpm_vld,
                                        base_addr,
                                        index,
                                        ecc_en,
                                        p_data, 
                                        p_entry);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_smmu1_write_entry_data");

    return DPP_OK;
}


/** ddr直接表 写数据
* @param   dev_id    芯片id
* @param   base_addr  基地址，以4K*128bit为单位
* @param   rw_len  数据位宽模式，位宽 0-128bit, 1-256bit, 2-384bit, 3-512bit,取值参考SMMU1_DDR_WRT_MODE_E的定义
* @param   index   条目索引，以mode为单位
* @param   ecc_en   ecc是否使能
* @param   p_data   待写入的数据指针
* @param   p_entry   组装好的条目(已分配好空间)
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30/
************************************************************/
DPP_STATUS dpp_dtb_ddr_hash_table_data_write(ZXIC_UINT32 dev_id, 
                                        ZXIC_UINT32 base_addr,
                                        ZXIC_UINT32 rw_len,
                                        ZXIC_UINT32 index,
                                        ZXIC_UINT32 ecc_en,
                                        ZXIC_UINT8 *p_data, 
                                        DPP_DTB_ENTRY_T *p_entry)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 ipv4_v6_flag = 0;
    ZXIC_UINT32 lpm_vld = 0;

    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_UPPER(rw_len,SMMU1_DDR_WRT_512b);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_data);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_entry);

    rc =  dpp_dtb_smmu1_write_entry_data(dev_id, 
                                        rw_len,
                                        ipv4_v6_flag,
                                        lpm_vld,
                                        base_addr,
                                        index,
                                        ecc_en,
                                        p_data, 
                                        p_entry);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_smmu1_write_entry_data");

    return DPP_OK;
}

/** dtb 通道 se alg间接写zcam空间
* @param   dev_id    芯片id
* @param   addr  基地址，以4K*128bit为单位
* @param   p_data   待写入的数据指针
* @param   p_entry   组装好的条目(已分配好空间)
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30/
************************************************************/
DPP_STATUS dpp_dtb_se_alg_zcam_data_write(ZXIC_UINT32 dev_id, 
                                        ZXIC_UINT32 addr,
                                        ZXIC_UINT8 *p_data, 
                                        DPP_DTB_ENTRY_T *p_entry)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 reg_sram_flag = 0;
    ZXIC_UINT32 zgroup_id = 0;
    ZXIC_UINT32 zblock_id = 0;
    ZXIC_UINT32 zcell_id = 0;
    ZXIC_UINT32 mask = 0;
    ZXIC_UINT32 sram_addr = 0;

    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_data);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_entry);

    mask = (addr >> 17) & 0xF;
    reg_sram_flag = (addr >> 16) & 0x1;
    zgroup_id = (addr >> 14) & 0x3;
    zblock_id = (addr >> 11) & 0x7;
    zcell_id = (addr >> 9) & 0x3;
    sram_addr = addr & 0x1FF;

    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_PRINT("dpp_dtb_se_alg_zcam_data_write: \n");
        ZXIC_COMM_DBGCNT32_PRINT("addr", addr);
        ZXIC_COMM_DBGCNT32_PRINT("mask", mask);
        ZXIC_COMM_DBGCNT32_PRINT("reg_sram_flag", reg_sram_flag);
        ZXIC_COMM_DBGCNT32_PRINT("zgroup_id", zgroup_id);
        ZXIC_COMM_DBGCNT32_PRINT("zblock_id", zblock_id);
        ZXIC_COMM_DBGCNT32_PRINT("zcell_id", zcell_id);
        ZXIC_COMM_DBGCNT32_PRINT("sram_addr", sram_addr);
    }

    rc = dpp_dtb_zcam_write_entry_data(dev_id, 
                                         reg_sram_flag,
                                         zgroup_id,
                                         zblock_id,
                                         zcell_id,
                                         sram_addr,
                                         mask,
                                         p_data, 
                                         p_entry);

    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_zcam_write_entry_data");

    return DPP_OK;       
}

#endif

#if ZXIC_REAL("DOWN_TABLE")
/** 将HASH表数据格式写入form_buff中
* @param   p_hash_cfg    hash表配置指针
* @param   p_rbkey_rtn   红黑树新节点信息指针
* @param   entry_data    条目数据缓存缓存
* @param   opr_mode      配置模式 0:add/update 1:delete
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_hash_form_write(DPP_HASH_CFG *p_hash_cfg, 
                               DPP_HASH_RBKEY_INFO *p_rbkey_new,
                               ZXIC_UINT32 actu_key_size,
                               DPP_DTB_ENTRY_T *p_entry,
                               ZXIC_UINT32 opr_mode)
{
    ZXIC_UINT8 table_id = 0;
    ZXIC_UINT32 key_type = 0;
    ZXIC_UINT32 key_by_size = 0;
    ZXIC_UINT32 rst_by_size = 0;
    ZXIC_UINT32 byte_offset = 0;

    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 bulk_id = 0;
    ZXIC_UINT32 temp_mask = 0;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 ddr_wr_mode = 0;
    ZXIC_UINT32 addr;

    D_NODE *p_entry_dn = NULL;
    HASH_DDR_CFG *p_ddr_cfg = NULL;
    SE_ITEM_CFG *p_item_info = NULL;
    DPP_HASH_RBKEY_INFO *p_rbkey = NULL;
    ZXIC_UINT8 entry_data[SE_ENTRY_WIDTH_MAX] = {0};

    ZXIC_COMM_CHECK_POINT(p_hash_cfg);
    ZXIC_COMM_CHECK_POINT(p_rbkey_new);

    dev_id = p_hash_cfg->p_se_info->dev_id;
    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id,DPP_DEV_CHANNEL_MAX - 1);

    ZXIC_COMM_MEMSET(entry_data,0x0,sizeof(entry_data));
    p_item_info = p_rbkey_new->p_item_info;
    ZXIC_COMM_CHECK_POINT(p_item_info);

    if (p_item_info->item_type == ITEM_DDR_256 || p_item_info->item_type == ITEM_DDR_512)//hash条目存在DDR中
    {
        table_id = DPP_GET_HASH_TBL_ID(p_rbkey_new->key);
        bulk_id = ((table_id >> 2) & 0x7);
        ZXIC_COMM_CHECK_INDEX_UPPER(bulk_id,HASH_BULK_NUM - 1);
        p_ddr_cfg = p_hash_cfg->p_bulk_ddr_info[bulk_id];
        key_type = DPP_GET_HASH_KEY_TYPE(p_rbkey_new->key);
        key_by_size = DPP_GET_KEY_SIZE(actu_key_size);

        switch (key_type)
        {
            case HASH_KEY_128b:
            {
                rst_by_size = 16U - DPP_GET_ACTU_KEY_BY_SIZE(actu_key_size) - HASH_KEY_CTR_SIZE;
                break;
            }
            case HASH_KEY_256b:
            {
                rst_by_size = 32U - DPP_GET_ACTU_KEY_BY_SIZE(actu_key_size) - HASH_KEY_CTR_SIZE;
                break;
            }
            case HASH_KEY_512b:
            {
                rst_by_size = 64U - DPP_GET_ACTU_KEY_BY_SIZE(actu_key_size) - HASH_KEY_CTR_SIZE;
                break;
            }
            default:
            {
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n ErrorCode[%x]: Invalid key type.", DPP_HASH_RC_INVALID_KEY_TYPE);
                return DPP_HASH_RC_INVALID_KEY_TYPE;
            }

        }

        /* 计算index */
        if (DDR_WIDTH_256b == p_ddr_cfg->width_mode)
        {
            if (HASH_KEY_128b == key_type)
            {
                index = (p_item_info->hw_addr << 1) + p_rbkey_new->entry_pos;
            }
            else if (HASH_KEY_256b == key_type)
            {
                index = p_item_info->hw_addr;
            }
        }
        else if (DDR_WIDTH_512b == p_ddr_cfg->width_mode)
        {
            if (HASH_KEY_128b == key_type)
            {
                index = (p_item_info->hw_addr << 2) + p_rbkey_new->entry_pos;
            }
            else if (HASH_KEY_256b == key_type)
            {
                index = (p_item_info->hw_addr << 2) + p_rbkey_new->entry_pos;
                index = index >> 1;
            }
            else
            {
                index = p_item_info->hw_addr;
            }
        }

        ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "ddr index(unit by key_type) is 0x%x \n", index);
        
        ddr_wr_mode = DPP_GET_DDR_WR_MODE(key_type);
        
        /*将数据组合成一个数组中并写入缓存中进行保存*/
        if(DTB_ITEM_ADD_OR_UPDATE == opr_mode)
        {
            ZXIC_COMM_MEMCPY(entry_data, p_rbkey_new->key, key_by_size);
            ZXIC_COMM_MEMCPY(entry_data + key_by_size, p_rbkey_new->rst, ((rst_by_size > HASH_RST_MAX) ? HASH_RST_MAX : rst_by_size));
            zxic_comm_swap(entry_data, SE_ENTRY_WIDTH_MAX);
        }

        dpp_dtb_ddr_hash_table_data_write(dev_id,
                                          p_ddr_cfg->ddr_baddr,
                                          ddr_wr_mode,
                                          index,
                                          p_ddr_cfg->ddr_ecc_en,
                                          entry_data,
                                          p_entry); 
        ZXIC_COMM_TRACE_DEBUG("entry_data is:");

        for (i = 0; i < SE_ENTRY_WIDTH_MAX; i++)
        {
            ZXIC_COMM_TRACE_DEBUG("0x%02x ", entry_data[i]);
        }

        ZXIC_COMM_TRACE_DEBUG("\n");

    }
    else /* hash条目存在zcam上*/
    {
        /*写ZCAM表格式*/
        ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "zcam p_item_info->hw_addr is 0x%x \n", p_item_info->hw_addr);
        addr = p_item_info->hw_addr;
 
        /*将数据组合成一个数组中并写入缓存中进行保存*/
        p_entry_dn = p_item_info->item_list.p_next;

        while (p_entry_dn)
        {
            p_rbkey = (DPP_HASH_RBKEY_INFO *)(p_entry_dn->data);
            // table_id = DPP_GET_HASH_TBL_ID(p_rbkey->key);
            key_type = DPP_GET_HASH_KEY_TYPE(p_rbkey->key);
            key_by_size = DPP_GET_KEY_SIZE(actu_key_size);
            ZXIC_COMM_CHECK_INDEX_UPPER(key_by_size,HASH_KEY_MAX);
            rst_by_size = DPP_GET_RST_SIZE(key_type, actu_key_size);

            byte_offset = p_rbkey->entry_pos * HASH_ENTRY_POS_STEP;
            ZXIC_COMM_MEMCPY(entry_data + byte_offset, p_rbkey->key, key_by_size);
            ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id, byte_offset, key_by_size);
            byte_offset += key_by_size;
            ZXIC_COMM_MEMCPY(entry_data + byte_offset, p_rbkey->rst, ((rst_by_size > HASH_RST_MAX) ? HASH_RST_MAX : rst_by_size));

            temp_mask |=  ((((1U << (p_rbkey->entry_size/16U)) - 1U) << (4U - p_rbkey->entry_size/16U - p_rbkey->entry_pos)) & 0xF);//计算掩码

            p_entry_dn = p_entry_dn->next;
        }

        zxic_comm_swap(entry_data, SE_ENTRY_WIDTH_MAX);

        dpp_dtb_se_alg_zcam_data_write(dev_id,
                                       addr,
                                       entry_data,
                                       p_entry);

        ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "zcam_item_data is:");

        for (i = 0; i < SE_ITEM_WIDTH_MAX; i++)
        {
            ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "0x%02x ", entry_data[i]);
        }

        ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "\n");
    }

    return DPP_OK;
}


/** 增加hash表条目时写buff函数
* @param   dev_id              设备号
* @param   p_hash_entry_cfg    该hash条目的格式内容信息
* @param   p_data_buff         保存用于dtb下表数据的buff
* @param   p_dtb_len           下发完该hash表条目后dtb数据的长度
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/06/03
************************************************************/
DPP_STATUS dpp_dtb_add_hash_buf_write(ZXIC_UINT32 dev_id,
                               HASH_ENTRY_CFG *p_hash_entry_cfg,
                               ZXIC_UINT8 *p_data_buff,
                               ZXIC_UINT32 *p_dtb_len)
{
    DPP_STATUS rc = DPP_OK;
    SE_ITEM_CFG *p_item_info = NULL;
    ZXIC_UINT32 ddr_wr_mode = 0;
    ZXIC_UINT32 addr_offset = 0;
    DPP_DTB_ENTRY_T dtb_one_entry = {0};  /*条目结构*/
    ZXIC_UINT8 cmd_buff[DTB_TABLE_CMD_SIZE_BIT / 8] = {0};  /*命令格式缓存*/
    ZXIC_UINT8 hash_entry_data[SE_ENTRY_WIDTH_MAX] = {0}; 

    ZXIC_COMM_CHECK_POINT(p_hash_entry_cfg);
    ZXIC_COMM_CHECK_POINT(p_hash_entry_cfg->p_rbkey_new);
    ZXIC_COMM_CHECK_POINT(p_dtb_len);

    ZXIC_COMM_MEMSET(cmd_buff,0x0,sizeof(cmd_buff));
    ZXIC_COMM_MEMSET(hash_entry_data,0x0,sizeof(hash_entry_data));
    dtb_one_entry.cmd = cmd_buff;
    dtb_one_entry.data = hash_entry_data;

     /*将位置信息和数据信息写入buff中*/
    rc = dpp_dtb_hash_form_write(p_hash_entry_cfg->p_hash_cfg, 
                                 p_hash_entry_cfg->p_rbkey_new, 
                                 p_hash_entry_cfg->actu_key_size, 
                                 &dtb_one_entry,
                                 DTB_ITEM_ADD_OR_UPDATE); /*计算一个条目的位置信息，并保存起来*/
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_dtb_hash_form_write");
    
    p_item_info = p_hash_entry_cfg->p_rbkey_new->p_item_info;
    ZXIC_COMM_CHECK_POINT(p_item_info);
    ddr_wr_mode = DPP_GET_DDR_WR_MODE(p_hash_entry_cfg->key_type);

    /*将数据写入缓存中，并更新长度统计*/
    addr_offset = (*p_dtb_len) * DTB_LEN_POS_SETP;
    if(p_item_info->item_type == ITEM_DDR_256 || p_item_info->item_type == ITEM_DDR_512)
    {
        dtb_one_entry.data_size = DTB_LEN_POS_SETP * (ddr_wr_mode + 1);
        (*p_dtb_len) += (ddr_wr_mode + 2);//dtb长度进行计数
    }
    else/*ram or reg*/
    {
        dtb_one_entry.data_size = DTB_LEN_POS_SETP * (DTB_ZCAM_LEN_SIZE - 1);
        (*p_dtb_len) += DTB_ZCAM_LEN_SIZE;//dtb长度进行计数
    }

    rc = dpp_dtb_data_write(p_data_buff, addr_offset, &dtb_one_entry);  /*将数据写入缓存中*/
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_data_write");

    return rc;
}

/** 删除hash表条目时写buff函数
* @param   dev_id              设备号
* @param   p_hash_entry_cfg    该hash条目的格式内容信息
* @param   p_data_buff         保存用于dtb下表数据的buff
* @param   p_dtb_len           下发完该hash表条目后dtb数据的长度
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/06/03
************************************************************/
DPP_STATUS dpp_dtb_delete_hash_buf_write(ZXIC_UINT32 dev_id,
                               HASH_ENTRY_CFG *p_hash_entry_cfg,
                               ZXIC_UINT8 *p_data_buff,
                               ZXIC_UINT32 *p_dtb_len)
{
    DPP_STATUS rc = DPP_OK;
    SE_ITEM_CFG *p_item_info = NULL;
    ZXIC_UINT32 ddr_wr_mode = 0;
    ZXIC_UINT32 addr_offset = 0;
    DPP_DTB_ENTRY_T dtb_one_entry = {0};  /*条目结构*/
    ZXIC_UINT8 cmd_buff[DTB_TABLE_CMD_SIZE_BIT / 8] = {0};  /*命令格式缓存*/
    ZXIC_UINT8 hash_entry_data[SE_ENTRY_WIDTH_MAX] = {0}; 

    ZXIC_COMM_CHECK_POINT(p_hash_entry_cfg);
    ZXIC_COMM_CHECK_POINT(p_hash_entry_cfg->p_rbkey_new);
    ZXIC_COMM_CHECK_POINT(p_dtb_len);

    ZXIC_COMM_MEMSET(cmd_buff,0x0,sizeof(cmd_buff));
    ZXIC_COMM_MEMSET(hash_entry_data,0x0,sizeof(hash_entry_data));
    dtb_one_entry.cmd = cmd_buff;
    dtb_one_entry.data = hash_entry_data;

     /*将位置信息和数据信息写入buff中*/
    rc = dpp_dtb_hash_form_write(p_hash_entry_cfg->p_hash_cfg, 
                                 p_hash_entry_cfg->p_rbkey_new, 
                                 p_hash_entry_cfg->actu_key_size, 
                                 &dtb_one_entry,
                                 DTB_ITEM_DELETE); /*计算一个条目的位置信息，并保存起来*/
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_dtb_hash_form_write");
    
    p_item_info = p_hash_entry_cfg->p_rbkey_new->p_item_info;
    ZXIC_COMM_CHECK_POINT(p_item_info);
    ddr_wr_mode = DPP_GET_DDR_WR_MODE(p_hash_entry_cfg->key_type);

    /*将数据写入缓存中，并更新长度统计*/
    addr_offset = (*p_dtb_len) * DTB_LEN_POS_SETP;
    if(p_item_info->item_type == ITEM_DDR_256 || p_item_info->item_type == ITEM_DDR_512)
    {
        dtb_one_entry.data_size = DTB_LEN_POS_SETP * (ddr_wr_mode + 1);
        (*p_dtb_len) += (ddr_wr_mode + 2);//dtb长度进行计数
    }
    else/*ram or reg*/
    {
        dtb_one_entry.data_size = DTB_LEN_POS_SETP * (DTB_ZCAM_LEN_SIZE - 1);
        (*p_dtb_len) += DTB_ZCAM_LEN_SIZE;//dtb长度进行计数
    }

    // ZXIC_COMM_MEMSET(dtb_one_entry.data, 0x0, dtb_one_entry.data_size);//清除下表数据

    rc = dpp_dtb_data_write(p_data_buff, addr_offset, &dtb_one_entry);  /*将数据写入缓存中*/
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_data_write");

    return rc;
}



/***********************************************************/
/** dtb 添加eTcam表条目,将etcam条目内容写入到entry中
* @param   dev_id     设备号
* @param   addr       每个block中的ram地址，位宽为8*80bit
* @param   block_idx  block编号，范围0~7
* @param   wr_mask    写表掩码，共8bit，每bit控制ram中对应位置的80bit数据是否有效
* @param   opr_type   etcam操作类型，详见 DPP_ETCAM_OPR_TYPE_E
* @param   p_entry    条目数据，data和mask
* @param   p_entry_data   组装好的数据条目(已分配好空间)
* @param   p_entry_mask   组装好的掩码条目(已分配好空间)
*
* @return
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_dtb_etcam_entry_add(DPP_DEV_T *dev, 
                               ZXIC_UINT32 addr,
                               ZXIC_UINT32 block_idx,
                               ZXIC_UINT32 wr_mask,
                               ZXIC_UINT32 opr_type,
                               DPP_ETCAM_ENTRY_T *p_entry,
                               DPP_DTB_ENTRY_T *p_entry_data,
                               DPP_DTB_ENTRY_T *p_entry_mask
                               )
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT8 temp_data[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 temp_mask[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    DPP_ETCAM_ENTRY_T entry_xy = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), addr, 0, DPP_ETCAM_RAM_DEPTH - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), block_idx, 0, DPP_ETCAM_BLOCK_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), wr_mask, 0, DPP_ETCAM_WR_MASK_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), opr_type, DPP_ETCAM_OPR_DM, DPP_ETCAM_OPR_XY);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_entry);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_entry_data);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_entry_mask);

    ZXIC_COMM_ASSERT(p_entry->p_data && p_entry->p_mask);

    entry_xy.p_data = temp_data;
    entry_xy.p_mask = temp_mask;

    if (opr_type == DPP_ETCAM_OPR_DM)
    {
        /* convert user D/M data to X/Y */
        rc = dpp_etcam_dm_to_xy(p_entry, &entry_xy, DPP_ETCAM_ENTRY_SIZE_GET(p_entry->mode));
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_etcam_dm_to_xy"); 
    }
    else
    {
        ZXIC_COMM_MEMCPY(entry_xy.p_data, p_entry->p_data, DPP_ETCAM_ENTRY_SIZE_GET(p_entry->mode));//复制出实际的数据
        ZXIC_COMM_MEMCPY(entry_xy.p_mask, p_entry->p_mask, DPP_ETCAM_ENTRY_SIZE_GET(p_entry->mode));
    }

    /*组装data格式*/
    rc = dpp_dtb_etcam_write_entry_data(dev, 
                                        block_idx,
                                        0,
                                        1,
                                        0,
                                        0,
                                        0,//0:写;1读
                                        wr_mask,
                                        DPP_ETCAM_DTYPE_DATA,
                                        addr,
                                        0,
                                        entry_xy.p_data, 
                                        p_entry_data);

    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_etcam_write_entry_data"); 

       /*组装mask格式*/
    rc =  dpp_dtb_etcam_write_entry_data(dev, 
                                        block_idx,
                                        0,
                                        1,
                                        0,
                                        0,
                                        0,
                                        wr_mask ,
                                        DPP_ETCAM_DTYPE_MASK ,
                                        addr ,
                                        0xFF,
                                        entry_xy.p_mask, 
                                        p_entry_mask);

   ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_etcam_write_entry_data"); 

   return DPP_OK;
}

/** dtb写eRam表
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   sdt_no    eram表sdt表号
* @param   entry_cnt 下发的条目数
* @param   p_entry_arr   待下发表项内容结构体数组指针
* @param   element_id   返回下表使用的元素id
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_eram_dma_write_cycle(DPP_DEV_T *dev, 
                                      ZXIC_UINT32 queue_id, 
                                      ZXIC_UINT32 sdt_no, 
                                      ZXIC_UINT32 entry_num, 
                                      DPP_DTB_ERAM_ENTRY_INFO_T *p_entry_arr,
                                      ZXIC_UINT32 *element_id
                                      )
{
    DPP_STATUS  rc = DPP_OK;
    
    ZXIC_UINT32 wrt_mode;
    ZXIC_UINT32 base_addr;
    ZXIC_UINT32 index;
    ZXIC_UINT32 item_cnt = 0;
    ZXIC_UINT32 addr_offset = 0;
    ZXIC_UINT32 *p_entry_data = NULL;
    ZXIC_UINT32 dtb_len = 0;
    ZXIC_UINT32 eram_table_depth = 0;

    ZXIC_UINT8 *table_data_buff = NULL;
    ZXIC_UINT32 entry_data_buff[4] = {0};
    ZXIC_UINT8 cmd_buff[DTB_TABLE_CMD_SIZE_BIT / 8] = {0};

    DPP_SDTTBL_ERAM_T sdt_eram_info = {0};
    DPP_DTB_ENTRY_T   dtb_one_entry = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_LOWER(DEV_ID(dev), entry_num, 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_entry_arr);

    //分配下表数据缓存 16K
    table_data_buff = (ZXIC_UINT8 *) ZXIC_COMM_MALLOC(DPP_DTB_TABLE_DATA_BUFF_SIZE * sizeof(ZXIC_UINT8));
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), table_data_buff);
    ZXIC_COMM_MEMSET(table_data_buff, 0, DPP_DTB_TABLE_DATA_BUFF_SIZE * sizeof(ZXIC_UINT8));

    dtb_one_entry.cmd = cmd_buff;
    dtb_one_entry.data = (ZXIC_UINT8 *)entry_data_buff;

    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_eram_info);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE(DEV_ID(dev), rc, "dpp_soft_sdt_tbl_get", table_data_buff);
    base_addr = sdt_eram_info.eram_base_addr;
    wrt_mode = sdt_eram_info.eram_mode;//3:128
    eram_table_depth = sdt_eram_info.eram_table_depth;
    ZXIC_COMM_CHECK_DEV_INDEX_LOWER_MEMORY_FREE(DEV_ID(dev), eram_table_depth, 1, table_data_buff);

    switch (wrt_mode)
    {
        case ERAM128_TBL_128b:
        {
            wrt_mode = ERAM128_OPR_128b;
            break;
        }

        case ERAM128_TBL_64b:
        {
            wrt_mode = ERAM128_OPR_64b;
            break;
        }

        case ERAM128_TBL_1b:
        {
            wrt_mode = ERAM128_OPR_1b;
            break;
        }
    }

    for(item_cnt = 0; item_cnt < entry_num; ++item_cnt)
    {
        p_entry_data = (ZXIC_UINT32 *)p_entry_arr[item_cnt].p_data;
        ZXIC_COMM_CHECK_DEV_POINT_MEMORY_FREE(DEV_ID(dev), p_entry_data, table_data_buff);
        index = p_entry_arr[item_cnt].index;
        ZXIC_COMM_CHECK_DEV_INDEX_MEMORY_FREE_NO_ASSERT(DEV_ID(dev), index, 0, eram_table_depth - 1, table_data_buff);

        if(dpp_dtb_prt_get())
        {
            ZXIC_COMM_PRINT("the item index is %d !\n", index);
        }
        
        //将一个数据写入entry
        rc = dpp_dtb_se_smmu0_ind_write(dev,
                                   base_addr, 
                                   index, 
                                   wrt_mode,
                                   p_entry_data,
                                   &dtb_one_entry);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_se_smmu0_ind_write", table_data_buff);

        switch (wrt_mode)
        {
            case ERAM128_OPR_128b:
            {
                dtb_len += 2; 
                addr_offset = item_cnt * DTB_LEN_POS_SETP * 2;
                break;
            }

            case ERAM128_OPR_64b:
            {
                dtb_len += 1; 
                addr_offset = item_cnt * DTB_LEN_POS_SETP;
                break;
            }

            case ERAM128_OPR_1b:
            {
                dtb_len += 1;
                addr_offset = item_cnt * DTB_LEN_POS_SETP;
                break;
            }
        }

        /*将表格式和数据写入缓存buff中*/
        rc = dpp_dtb_data_write(table_data_buff, addr_offset, &dtb_one_entry);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_data_write", table_data_buff);
        ZXIC_COMM_MEMSET(cmd_buff, 0, DTB_TABLE_CMD_SIZE_BIT / 8);
        ZXIC_COMM_MEMSET(entry_data_buff, 0, 4 * sizeof(ZXIC_UINT32));
    }
     
    rc = dpp_dtb_write_down_table_data(dev,  
                                     queue_id, 
                                     dtb_len * 16,
                                     table_data_buff,
                                      element_id
                                      );
    ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_write_down_table_data", table_data_buff);
     
    ZXIC_COMM_FREE(table_data_buff);

    return DPP_OK;
}


/** dtb写ddr中的数据，数据长度在16K范围内
* @param   dev_id          设备号
* @param   queue_id        队列号
* @param   ddr_base_addr   ddr基地址，以4K*128bit为单位
* @param   ddr_wr_mode     ddr写模式 0-128bit, 1-256bit, 2-384bit, 3-512bit,取值参考SMMU1_DDR_WRT_MODE_E的定义
* @param   ddr_ecc_en      ddr ECC使能
* @param   entry_num       下发的条目数
* @param   p_entry_arr     待下发表项内容结构体数组指针
* @param   element_id      返回下表使用的元素id
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_smmu1_data_write_cycle(DPP_DEV_T *dev, 
                                          ZXIC_UINT32 queue_id, 
                                          ZXIC_UINT32 ddr_base_addr, 
                                          ZXIC_UINT32 ddr_wr_mode, 
                                          ZXIC_UINT32 ddr_ecc_en,
                                          ZXIC_UINT32 entry_num, 
                                          DPP_DTB_DDR_ENTRY_INFO_T *p_entry_arr,
                                          ZXIC_UINT32 *element_id)
{
    DPP_STATUS  rc = DPP_OK;
    
    ZXIC_UINT32 index;
    ZXIC_UINT32 item_cnt = 0;
    ZXIC_UINT32 addr_offset = 0;
    ZXIC_UINT32 *p_entry_data = NULL;
    ZXIC_UINT32 dtb_len = 0;
    ZXIC_UINT8 *table_data_buff = NULL;
    ZXIC_UINT8 cmd_buff[DTB_TABLE_CMD_SIZE_BIT / 8] = {0};  /*命令格式缓存*/
    ZXIC_UINT32 entry_data_buff[DPP_DIR_TBL_BUF_MAX_NUM] = {0};

    DPP_DTB_ENTRY_T   dtb_one_entry = {0};  /*条目结构*/

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_LOWER(DEV_ID(dev), entry_num, 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_entry_arr);

    //分配下表数据缓存 16K
    table_data_buff = (ZXIC_UINT8 *) ZXIC_COMM_MALLOC(DPP_DTB_TABLE_DATA_BUFF_SIZE * sizeof(ZXIC_UINT8));
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), table_data_buff);
    ZXIC_COMM_MEMSET(table_data_buff, 0, DPP_DTB_TABLE_DATA_BUFF_SIZE * sizeof(ZXIC_UINT8));

    dtb_one_entry.cmd = cmd_buff;
    dtb_one_entry.data = (ZXIC_UINT8 *)entry_data_buff;

     /*对每一个条目进行处理*/
    for(item_cnt = 0; item_cnt < entry_num; ++item_cnt)
    {
        p_entry_data = (ZXIC_UINT32 *)p_entry_arr[item_cnt].p_data;
        ZXIC_COMM_CHECK_POINT_MEMORY_FREE_NO_ASSERT(p_entry_data, table_data_buff);

        index = p_entry_arr[item_cnt].index;

        //将一个数据写入entry
        rc = dpp_dtb_ddr_dir_table_data_write(DEV_ID(dev), 
                                        ddr_base_addr,
                                        ddr_wr_mode,
                                        index,
                                        ddr_ecc_en,
                                        (ZXIC_UINT8 *)p_entry_data, 
                                        &dtb_one_entry);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_ddr_dir_table_data_write", table_data_buff);

        dtb_len += (ddr_wr_mode + 2);
        addr_offset = item_cnt * (ddr_wr_mode + 2) * DTB_LEN_POS_SETP;

        /*数据写入缓存中*/
        rc = dpp_dtb_data_write(table_data_buff, addr_offset, &dtb_one_entry);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_data_write", table_data_buff);
        ZXIC_COMM_MEMSET(cmd_buff, 0, DTB_TABLE_CMD_SIZE_BIT / 8);
        ZXIC_COMM_MEMSET(entry_data_buff, 0, DPP_DIR_TBL_BUF_MAX_NUM * sizeof(ZXIC_UINT32));
    }
    
    rc = dpp_dtb_write_down_table_data(dev,  
                                      queue_id, 
                                      dtb_len * 16,
                                      table_data_buff,
                                      element_id
                                      );
    ZXIC_COMM_FREE(table_data_buff);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_write_down_table_data");
    
    return DPP_OK;
}

/** dtb写HASH表,在插入条目时如果冲突，则对冲突条目进行记录
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   sdt_no    hash表sdt表号
* @param   entry_cnt 下发的条目数
* @param   p_entry_arr   待下发表项内容结构体数组指针
* @param   element_id   返回下表使用的元素id
* @return
* @remark  无 是否是有一个写不成功就返回，还是继续进行下一个条目并记录错误的条目
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_hash_dma_insert_cycle(DPP_DEV_T *dev, 
                                   ZXIC_UINT32 queue_id, 
                                   ZXIC_UINT32 sdt_no, 
                                   ZXIC_UINT32 entry_num, 
                                   DPP_DTB_HASH_ENTRY_INFO_T *p_arr_hash_entry,
                                   ZXIC_UINT32 *element_id)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT8 temp_key[HASH_KEY_MAX] = {0};
    ZXIC_UINT8 end_flag = 0;
    // ZXIC_UINT32 ddr_wr_mode = 0;
    ZXIC_UINT32 item_cnt = 0;
    ZXIC_UINT8  key_valid = 1;
    ZXIC_UINT32 dtb_len = 0;
    ZXIC_UINT8 key[HASH_KEY_MAX] = {0};
    ZXIC_UINT8 rst[HASH_RST_MAX] = {0};

    DPP_SE_CFG *p_se_cfg = NULL;
    ZXIC_RB_TN *p_rb_tn_new = NULL;
    ZXIC_RB_TN *p_rb_tn_rtn = NULL;
    DPP_HASH_CFG *p_hash_cfg = NULL;
    DPP_HASH_RBKEY_INFO *p_rbkey_new = NULL;
    FUNC_ID_INFO *p_func_info = NULL;
    HASH_ENTRY_CFG  hash_entry_cfg = {0}; 
    DPP_SDTTBL_HASH_T sdt_hash_info = {0};  /*SDT内容*/
    DPP_HASH_ENTRY entry = {0}; /* hash条目结构体*/
    ZXIC_UINT8 *p_data_buff = NULL;
    ZXIC_MUTEX_T *p_hash_mutex = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_LOWER(DEV_ID(dev), entry_num, 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_arr_hash_entry);


    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_hash_info);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_soft_sdt_tbl_read");

    hash_entry_cfg.fun_id   = sdt_hash_info.hash_id; /*hash引擎*/
    ZXIC_COMM_CHECK_INDEX(hash_entry_cfg.fun_id, HASH_FUNC_ID_MIN, HASH_FUNC_ID_NUM - 1);
    hash_entry_cfg.table_id = sdt_hash_info.hash_table_id; /*hash表号*/
    ZXIC_COMM_CHECK_INDEX_UPPER(hash_entry_cfg.table_id,HASH_TBL_ID_NUM - 1);
    hash_entry_cfg.bulk_id = ((hash_entry_cfg.table_id >> 2) & 0x7);
    ZXIC_COMM_CHECK_INDEX_UPPER(hash_entry_cfg.bulk_id,HASH_BULK_NUM - 1);
    hash_entry_cfg.key_type = sdt_hash_info.hash_table_width; /*表宽度*/
    ZXIC_COMM_CHECK_INDEX(hash_entry_cfg.key_type, HASH_KEY_128b, HASH_KEY_512b);
    hash_entry_cfg.actu_key_size = sdt_hash_info.key_size; /*业务表键值长度*/
    ZXIC_COMM_CHECK_INDEX(hash_entry_cfg.actu_key_size, HASH_ACTU_KEY_MIN, HASH_ACTU_KEY_MAX);
    hash_entry_cfg.key_by_size = DPP_GET_KEY_SIZE(hash_entry_cfg.actu_key_size);
    hash_entry_cfg.rst_by_size = DPP_GET_RST_SIZE(hash_entry_cfg.key_type, hash_entry_cfg.actu_key_size);

     /* 取出se配置 */
    rc = dpp_se_cfg_get(dev, &p_se_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_se_cfg_get");
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_se_cfg);
    p_func_info = DPP_GET_FUN_INFO(p_se_cfg, hash_entry_cfg.fun_id);
    DPP_SE_CHECK_FUN(p_func_info, hash_entry_cfg.fun_id, FUN_HASH);
    hash_entry_cfg.p_hash_cfg = (DPP_HASH_CFG *)p_func_info->fun_ptr;
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), hash_entry_cfg.p_hash_cfg);

     /*条目数上限检查*/
    // ddr_wr_mode = DPP_GET_DDR_WR_MODE(hash_entry_cfg.key_type);
    // ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), entry_num, 1, DTB_DATA_SIZE_BIT / ( (ddr_wr_mode + 2) * DTB_TABLE_CMD_SIZE_BIT));

    entry.p_key = key;
    entry.p_rst = rst;
    entry.p_key[0] = (ZXIC_UINT8)(((key_valid & 0x1) << 7) | ((hash_entry_cfg.key_type & 0x3) << 5) | (hash_entry_cfg.table_id & 0x1f));
    
    p_data_buff = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(DPP_DTB_TABLE_DATA_BUFF_SIZE);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data_buff);
    ZXIC_COMM_MEMSET(p_data_buff,0x0,DPP_DTB_TABLE_DATA_BUFF_SIZE);

    rc = dpp_dev_hash_opr_mutex_get(dev, hash_entry_cfg.fun_id, &p_hash_mutex);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dev_opr_mutex_get",p_data_buff); 
    rc = zxic_comm_mutex_lock(p_hash_mutex);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "zxic_comm_mutex_lock",p_data_buff); 

    /*对每一个条目进行处理*/
    for(item_cnt = 0; item_cnt < entry_num; ++item_cnt)
    {
        end_flag = 0;
         /*组装数据*/
        ZXIC_COMM_MEMCPY(&entry.p_key[1], p_arr_hash_entry[item_cnt].p_actu_key, hash_entry_cfg.actu_key_size);
        ZXIC_COMM_MEMCPY(&entry.p_rst[0], p_arr_hash_entry[item_cnt].p_rst, ((hash_entry_cfg.rst_by_size > HASH_RST_MAX) ? HASH_RST_MAX : hash_entry_cfg.rst_by_size));
     
        rc = dpp_hash_red_black_node_alloc(dev,&p_rb_tn_new,&p_rbkey_new);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE_UNLOCK_NO_ASSERT(rc, "dpp_hash_red_black_node_alloc", p_data_buff, p_hash_mutex);
        ZXIC_COMM_MEMCPY(p_rbkey_new->key, entry.p_key, hash_entry_cfg.key_by_size);
        hash_entry_cfg.p_rbkey_new = p_rbkey_new; 
        hash_entry_cfg.p_rb_tn_new = p_rb_tn_new;
        
        rc = dpp_hash_rb_insert(dev,&hash_entry_cfg,&entry);
        if(DPP_OK != rc)
        {
            if(DPP_HASH_RC_ADD_UPDATE == rc)
            {
                /*将位置信息和数据信息写入buff中*/
                rc = dpp_dtb_add_hash_buf_write(DEV_ID(dev),&hash_entry_cfg,p_data_buff,&dtb_len);
                ZXIC_COMM_CHECK_RC_MEMORY_FREE_UNLOCK_NO_ASSERT(rc, "dpp_dtb_add_hash_buf_write", p_data_buff, p_hash_mutex);
            }
            continue;
        }
    
        /*insert new hash item*/
        /*1 first form the new key(calc crc)*/
        rc = dpp_hash_set_crc_key(dev,&hash_entry_cfg,&entry,temp_key);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE_UNLOCK_NO_ASSERT(rc, "dpp_hash_set_crc_key", p_data_buff, p_hash_mutex);

        /*2 if DDR is valid, first insert into DDR.*/
        p_hash_cfg = hash_entry_cfg.p_hash_cfg;
        if (p_hash_cfg->ddr_valid)
        {
            rc = dpp_hash_insert_ddr(dev,&hash_entry_cfg,temp_key,&end_flag);
            ZXIC_COMM_CHECK_RC_MEMORY_FREE_UNLOCK_NO_ASSERT(rc, "dpp_hash_insert_ddr", p_data_buff, p_hash_mutex);
        }

        /*3 if insert into DDR is fail, insert into ZCAM. */
        if (!end_flag)
        {
            rc = dpp_hash_insert_zcell(dev,p_se_cfg,&hash_entry_cfg,temp_key,&end_flag);
            ZXIC_COMM_CHECK_RC_MEMORY_FREE_UNLOCK_NO_ASSERT(rc, "dpp_hash_insert_zcell", p_data_buff, p_hash_mutex);
        }

        /*4 if insert into ZCAM is fail, insert into ZBLK Reg. */
        if (!end_flag)
        {
            rc = dpp_hash_insert_zreg(dev,&hash_entry_cfg,temp_key,&end_flag);
            ZXIC_COMM_CHECK_RC_MEMORY_FREE_UNLOCK_NO_ASSERT(rc, "dpp_hash_insert_zreg", p_data_buff, p_hash_mutex);
        }

        if (!end_flag)
        {
            p_hash_cfg->hash_stat.insert_fail++;
            /* recycle rb tree node */
            ZXIC_COMM_MEMCPY(temp_key, entry.p_key, hash_entry_cfg.key_by_size);
            rc = zxic_comm_rb_delete(&p_hash_cfg->hash_rb, p_rbkey_new, &p_rb_tn_rtn);
            ZXIC_COMM_CHECK_RC_MEMORY_FREE_UNLOCK_NO_ASSERT(rc, "zxic_comm_rb_delete", p_data_buff, p_hash_mutex);
            ZXIC_COMM_ASSERT(p_rb_tn_new == p_rb_tn_rtn);
            ZXIC_COMM_FREE(p_rbkey_new);
            ZXIC_COMM_FREE(p_rb_tn_rtn); 
            ZXIC_COMM_FREE(p_data_buff); 
            rc = zxic_comm_mutex_unlock(p_hash_mutex);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
            dpp_dtb_data_print(temp_key, hash_entry_cfg.key_by_size);   
            ZXIC_COMM_TRACE_ERROR("DPP_HASH_RC_TBL_FULL.\n");
            return DPP_RC_DTB_DOWN_HASH_CONFLICT;
        }

        /*将位置信息和数据信息写入buff中*/
        rc = dpp_dtb_add_hash_buf_write(DEV_ID(dev),&hash_entry_cfg,p_data_buff,&dtb_len);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE_UNLOCK_NO_ASSERT(rc, "dpp_dtb_add_hash_buf_write", p_data_buff, p_hash_mutex);
        
        p_hash_cfg->hash_stat.insert_ok++;
        p_hash_cfg->hash_stat.insert_table[hash_entry_cfg.table_id].sum++;
    }

    rc = zxic_comm_mutex_unlock(p_hash_mutex);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "zxic_comm_mutex_unlock", p_data_buff); 
    
    rc = dpp_dtb_write_down_table_data(dev,  
                                     queue_id, 
                                     dtb_len * 16,
                                     p_data_buff,
                                     element_id);
    ZXIC_COMM_FREE(p_data_buff);                                   
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_write_down_table_data");

    return DPP_OK;
}


/** dtb写ACL表 （SPECIFY模式，条目中指定handle，支持级联64bit/128bit）
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   sdt_no    ACL表sdt表号
* @param   entry_cnt 下发的条目数
* @param   p_entry_arr   待下发表项内容结构体数组指针
* @param   element_id    返回下表使用的元素id
* @return
* @remark  无 是否是有一个写不成功就返回，还是继续进行下一个条目并记录错误的条目
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_acl_dma_insert_cycle(DPP_DEV_T *dev, 
                                      ZXIC_UINT32 queue_id, 
                                      ZXIC_UINT32 sdt_no, 
                                      ZXIC_UINT32 entry_num, 
                                      DPP_DTB_ACL_ENTRY_INFO_T *p_acl_entry_arr,
                                      ZXIC_UINT32 *element_id
                                      )
{
    /*
    1、条目的handle值是指定的；
    2、根据级联配置在etcam中保存级联结果值
    */
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 etcam_key_mode;
    ZXIC_UINT32 as_eram_baddr;
    ZXIC_UINT32 as_enable;
    ZXIC_UINT32 etcam_table_id;
    ZXIC_UINT32 etcam_as_mode;
    ZXIC_UINT32 block_idx = 0;
    ZXIC_UINT32 ram_addr = 0;
    ZXIC_UINT32 etcam_wr_mode = 0;
    ZXIC_UINT32 eram_wrt_mode = 0;
    ZXIC_UINT32 eram_index;

    ZXIC_UINT32 item_cnt = 0;
    ZXIC_UINT32 addr_offset_bk = 0;
    ZXIC_UINT32 dtb_len = 0;
    ZXIC_UINT32 as_addr_offset = 0;
    ZXIC_UINT32 as_dtb_len = 0;

    DPP_ACL_CFG_EX_T *p_acl_cfg = NULL;
    DPP_ACL_TBL_CFG_T *p_tbl_cfg = NULL;
    DPP_DTB_ACL_ENTRY_INFO_T *p_acl_entry = NULL;
    ZXIC_UINT32 *p_as_eram_data = NULL;
    ZXIC_UINT8 *table_data_buff = NULL;
    DPP_ETCAM_ENTRY_T etcam_entry = {0};

    ZXIC_UINT8 entry_data_buff[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 entry_mask_buff[DPP_ETCAM_WIDTH_MAX / 8] = {0}; 
    ZXIC_UINT32 as_eram_data_buff[4] = {0};   
    ZXIC_UINT8 entry_data_cmd_buff[DTB_TABLE_CMD_SIZE_BIT / 8] = {0};
    ZXIC_UINT8 entry_mask_cmd_buff[DTB_TABLE_CMD_SIZE_BIT / 8] = {0};
    ZXIC_UINT8 as_eram_cmd_buff[DTB_TABLE_CMD_SIZE_BIT / 8] = {0};

    DPP_SDTTBL_ETCAM_T sdt_etcam_info = {0}; 
    DPP_DTB_ENTRY_T entry_data = {0};
    DPP_DTB_ENTRY_T entry_mask = {0};
    DPP_DTB_ENTRY_T dtb_as_data_entry = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_LOWER(DEV_ID(dev), entry_num, 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_acl_entry_arr);

    entry_data.cmd = entry_data_cmd_buff;
    entry_data.data = (ZXIC_UINT8 *)entry_data_buff;

    entry_mask.cmd = entry_mask_cmd_buff;
    entry_mask.data = (ZXIC_UINT8 *)entry_mask_buff;

    dtb_as_data_entry.cmd = as_eram_cmd_buff;
    dtb_as_data_entry.data = (ZXIC_UINT8 *)as_eram_data_buff;

    //分配下表数据缓存 16K
    table_data_buff = (ZXIC_UINT8 *) ZXIC_COMM_MALLOC(DPP_DTB_TABLE_DATA_BUFF_SIZE * sizeof(ZXIC_UINT8));
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), table_data_buff);
    ZXIC_COMM_MEMSET(table_data_buff, 0, DPP_DTB_TABLE_DATA_BUFF_SIZE * sizeof(ZXIC_UINT8));

    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_etcam_info);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE(DEV_ID(dev), rc, "dpp_soft_sdt_tbl_get", table_data_buff);
    etcam_key_mode = sdt_etcam_info.etcam_key_mode;
    ZXIC_COMM_CHECK_INDEX_MEMORY_FREE_NO_ASSERT(etcam_key_mode, DPP_ACL_KEY_640b, DPP_ACL_KEY_80b, table_data_buff);
    etcam_as_mode = sdt_etcam_info.as_rsp_mode;
    ZXIC_COMM_CHECK_INDEX_MEMORY_FREE_NO_ASSERT(etcam_as_mode, DPP_ACL_AS_MODE_64b, DPP_ACL_AS_MODE_INVALID - 1, table_data_buff);
    etcam_table_id = sdt_etcam_info.etcam_table_id;
    ZXIC_COMM_CHECK_INDEX_MEMORY_FREE_NO_ASSERT(etcam_table_id, DPP_ACL_TBL_ID_MIN, DPP_ACL_TBL_ID_MAX, table_data_buff);
    as_enable = sdt_etcam_info.as_en;
    as_eram_baddr = sdt_etcam_info.as_eram_baddr;

    //级联数据下发
    if(as_enable)
    {
        //mode转换
        switch (etcam_as_mode)
        {
            case ERAM128_TBL_128b:
            {
                eram_wrt_mode = ERAM128_OPR_128b;
                break;
            }

            case ERAM128_TBL_64b:
            {
                eram_wrt_mode = ERAM128_OPR_64b;
                break;
            }

            case ERAM128_TBL_1b:
            {
                eram_wrt_mode = ERAM128_OPR_1b;
                break;
            }
        }
    }    

    rc = dpp_acl_cfg_get(dev,&p_acl_cfg);//获取ACL表资源配置
    ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_acl_cfg_get", table_data_buff);
    ZXIC_COMM_CHECK_POINT_MEMORY_FREE(p_acl_cfg, table_data_buff);

    p_tbl_cfg = p_acl_cfg->acl_tbls + etcam_table_id;//得到该acl表的资源配置

    if (!p_tbl_cfg->is_used)
    {
        ZXIC_COMM_TRACE_ERROR("table[ %d ] is not init!\n", etcam_table_id);
        ZXIC_COMM_FREE(table_data_buff);
        ZXIC_COMM_ASSERT(0);
        return DPP_ACL_RC_TBL_NOT_INIT;
    }

    /*对每一个条目进行处理*/
    for(item_cnt = 0; item_cnt < entry_num; ++item_cnt)
    {
         p_acl_entry = p_acl_entry_arr + item_cnt;
         
        /* write etcam key */
        etcam_entry.mode = p_tbl_cfg->key_mode;
        etcam_entry.p_data = p_acl_entry->key_data;
        etcam_entry.p_mask = p_acl_entry->key_mask;

        /*计算地址等信息*/
        rc = dpp_acl_hdw_addr_get(p_tbl_cfg, p_acl_entry->handle, &block_idx, &ram_addr, &etcam_wr_mode);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_acl_hdw_addr_get", table_data_buff);

        rc = dpp_dtb_etcam_entry_add(dev,
                               ram_addr,
                               block_idx,
                               etcam_wr_mode,
                               DPP_ETCAM_OPR_DM,
                               &etcam_entry,
                               &entry_data,
                               &entry_mask
                               );

        ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_etcam_entry_add", table_data_buff);

        /*data数据写入缓存*/
        dtb_len += DTB_ETCAM_LEN_SIZE;
        rc = dpp_dtb_data_write(table_data_buff, addr_offset_bk, &entry_data);  /*将数据写入缓存中*/
        ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_data_write", table_data_buff);
        ZXIC_COMM_MEMSET(entry_data_cmd_buff, 0, DTB_TABLE_CMD_SIZE_BIT / 8);
        ZXIC_COMM_MEMSET(entry_data_buff, 0, DPP_ETCAM_WIDTH_MAX / 8);
        addr_offset_bk = addr_offset_bk + DTB_ETCAM_LEN_SIZE * DTB_LEN_POS_SETP; /*在缓存中相对于0的offset*/

        /*mask数据写入缓存*/
        dtb_len += DTB_ETCAM_LEN_SIZE;
        rc = dpp_dtb_data_write(table_data_buff, addr_offset_bk, &entry_mask);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_data_write", table_data_buff);
        ZXIC_COMM_MEMSET(entry_mask_cmd_buff, 0, DTB_TABLE_CMD_SIZE_BIT / 8);
        ZXIC_COMM_MEMSET(entry_mask_buff, 0, 640/8);
        addr_offset_bk = addr_offset_bk + DTB_ETCAM_LEN_SIZE * DTB_LEN_POS_SETP; /*在缓存中相对于0的offset*/ 
        
        //处理级联数据
        if(as_enable)
        {
            p_as_eram_data = (ZXIC_UINT32 *)(p_acl_entry->p_as_rslt);
            ZXIC_COMM_CHECK_DEV_POINT_MEMORY_FREE_NO_ASSERT(DEV_ID(dev), p_as_eram_data, table_data_buff);
            eram_index = p_acl_entry->handle;
            rc = dpp_dtb_se_smmu0_ind_write(dev,
                            as_eram_baddr, 
                            eram_index, 
                            eram_wrt_mode,
                            p_as_eram_data,
                            &dtb_as_data_entry);
            ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_se_smmu0_ind_write", table_data_buff);

            switch (eram_wrt_mode)
            {
                case ERAM128_OPR_128b:
                {
                    as_dtb_len = 2;
                    as_addr_offset = DTB_LEN_POS_SETP * 2;
                    break;
                }

                case ERAM128_OPR_64b:
                {
                    as_dtb_len = 1;
                    as_addr_offset = DTB_LEN_POS_SETP;
                    break;
                }

                case ERAM128_OPR_1b:
                {
                    as_dtb_len = 1;
                    as_addr_offset = DTB_LEN_POS_SETP;
                    break;
                }
            }

            /*将表格式和数据写入缓存buff中*/
            rc = dpp_dtb_data_write(table_data_buff, addr_offset_bk, &dtb_as_data_entry);
            addr_offset_bk = addr_offset_bk + as_addr_offset; /*在缓存中相对于0的offset*/
            dtb_len += as_dtb_len;
            ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_data_write", table_data_buff);
            ZXIC_COMM_MEMSET(as_eram_cmd_buff, 0, DTB_TABLE_CMD_SIZE_BIT / 8);
            ZXIC_COMM_MEMSET(as_eram_data_buff, 0, 4 * sizeof(ZXIC_UINT32));
        }
    }

    rc = dpp_dtb_write_down_table_data(dev,  
                                    queue_id, 
                                    dtb_len * 16,
                                    table_data_buff,
                                    element_id
                                    );
    ZXIC_COMM_FREE(table_data_buff);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_write_down_table_data");

    return DPP_OK;
}

#endif /*DTB BASE INTERFACE*/

#if ZXIC_REAL("DTB_DOWN_INTERFACE")
/** dtb写eRam表--支持大数据量
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   sdt_no    eram表sdt表号
* @param   entry_cnt 下发的条目数
* @param   p_entry_arr   待下发表项内容结构体数组指针
* @param   element_id   返回下表使用的元素id
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_eram_dma_write(DPP_DEV_T *dev, 
                                      ZXIC_UINT32 queue_id, 
                                      ZXIC_UINT32 sdt_no, 
                                      ZXIC_UINT32 entry_num, 
                                      DPP_DTB_ERAM_ENTRY_INFO_T *p_entry_arr,
                                      ZXIC_UINT32 *element_id
                                      )
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 wrt_mode;
    ZXIC_UINT32 entry_num_max = 0;
    ZXIC_UINT32 entry_cycle = 0;
    ZXIC_UINT32 entry_remains = 0;
    ZXIC_UINT32 i = 0;
    DPP_DTB_ERAM_ENTRY_INFO_T *p_entry = NULL;

    DPP_SDTTBL_ERAM_T sdt_eram_info = {0};

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);

    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_eram_info);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_soft_sdt_tbl_get");

    wrt_mode = sdt_eram_info.eram_mode;//3:128

    switch (wrt_mode)
    {
        case ERAM128_TBL_128b:
        {
            entry_num_max = 0x1ff;
            break;
        }

        case ERAM128_TBL_64b:
        {
            entry_num_max = 0x3ff;
            break;
        }

        case ERAM128_TBL_1b:
        {
            entry_num_max = 0x3ff;
            break;
        }
    }

    ZXIC_COMM_CHECK_INDEX_EQUAL(entry_num_max, 0);
    entry_cycle = entry_num / entry_num_max;
    entry_remains = entry_num % entry_num_max;

    for(i = 0; i < entry_cycle; ++i)
    {
        p_entry = p_entry_arr + entry_num_max * i;
        rc = dpp_dtb_eram_dma_write_cycle(dev,
                                     queue_id,
                                     sdt_no,
                                     entry_num_max,
                                     p_entry,
                                     element_id);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_eram_dma_write_cycle");

        ZXIC_COMM_TRACE_INFO("slot [%d] queue [%d] element_id [%d] done.\n",DEV_PCIE_SLOT(dev), queue_id, *element_id);
    }

    if(entry_remains)
    {
        p_entry = p_entry_arr + entry_num_max * entry_cycle;
        rc = dpp_dtb_eram_dma_write_cycle(dev,
                                     queue_id,
                                     sdt_no,
                                     entry_remains,
                                     p_entry,
                                     element_id);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_eram_dma_write_cycle");

        ZXIC_COMM_TRACE_INFO("slot [%d] queue [%d] element_id [%d] done.\n",DEV_PCIE_SLOT(dev), queue_id, *element_id);

    }
    return DPP_OK;
}

/** 模拟微码写hash表，输出一个entry
* @param   dev_id    芯片id
* @param   sdt_high   sdt高32bit
* @param   sdt_low    sdt低32bit
* @param   delete_en  hash flag中delete使能
* @param   dma_en     hash flag中dma上送使能
* @param   p_data            key+rst
* @param   p_entry           组装好的条目(已分配好空间) data空间大小为512bit
*
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30/
************************************************************/
DPP_STATUS dpp_dtb_kschd_hash_write_entry_data(ZXIC_UINT32 dev_id,
                                               ZXIC_UINT32 sdt_high,
                                               ZXIC_UINT32 sdt_low,
                                               ZXIC_UINT32 delete_en,
                                               ZXIC_UINT32 dma_en,
                                               ZXIC_UINT32 *p_data,
                                               DPP_DTB_ENTRY_T *p_entry)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 hash_flag = 0;
    ZXIC_UINT32 i = 0;

    DPP_DTB_MC_HASH_TABLE_FORM_T dtb_mc_hash_form_info = {0};/*模拟微码写表格式*/
    DPP_DTB_MC_HASH_KEY_T mc_hash_key = {0};

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, delete_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dma_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_data);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_entry);

    /* 表格式写入缓存*/
    dtb_mc_hash_form_info.valid = DTB_TABLE_VALID;
    dtb_mc_hash_form_info.type_mode = DTB_TABLE_MODE_MC_HASH;
    dtb_mc_hash_form_info.std_h = sdt_high;
    dtb_mc_hash_form_info.std_l = sdt_low;

    rc = dpp_dtb_write_table_cmd(dev_id, DTB_TABLE_MC_HASH, &dtb_mc_hash_form_info, p_entry->cmd);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_dtb_write_table_cmd");

    p_entry->data_in_cmd_flag = 0;
    p_entry->data_size = DTB_LEN_POS_SETP * (DTB_MC_HASH_LEN_SIZE - 1);

    //组装hash key数据
    mc_hash_key.hash_key[0] = p_data[0] >> 8;
    hash_flag = hash_flag | delete_en;
    hash_flag = hash_flag | (dma_en << 1);
    mc_hash_key.hash_key[0] |=  hash_flag << 24;

    for (i = 0; i < 15; i++)
    {
        mc_hash_key.hash_key[i + 1] = ((p_data[i] & 0xff) << 24) + (p_data[i + 1] >> 8);
    }

    ZXIC_COMM_MEMCPY(p_entry->data,(ZXIC_UINT8 *)(&mc_hash_key), DTB_LEN_POS_SETP * (DTB_MC_HASH_LEN_SIZE - 1));

    return DPP_OK;
}

/** dtb模拟微码写HASH表
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   sdt_no    hash表sdt表号
* @param   entry_cnt 下发的条目数
* @param   p_arr_hash_entry   待下发表项内容结构体数组指针
* @param   element_id   返回下表使用的元素id
* @return
* @remark
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_hash_simu_mcode_write(DPP_DEV_T *dev,
                                   ZXIC_UINT32 queue_id,
                                   ZXIC_UINT32 sdt_no,
                                   ZXIC_UINT32 delete_en,
                                   ZXIC_UINT32 dma_en,
                                   ZXIC_UINT32 entry_num,
                                   DPP_DTB_HASH_ENTRY_INFO_T *p_arr_hash_entry,
                                   ZXIC_UINT32 *element_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 addr_offset = 0;
    ZXIC_UINT32 dtb_len = 0;
    ZXIC_UINT32 entry_buff[SE_ENTRY_WIDTH_MAX / 4] = {0};
    ZXIC_UINT8 entry_cmd_buff[DTB_TABLE_CMD_SIZE_BYTE] = {0};
    ZXIC_UINT8 entry_data_buff[SE_ENTRY_WIDTH_MAX] = {0};
    ZXIC_UINT8 *table_data_buff = NULL;
    DPP_DTB_ENTRY_T dtb_entry = {0};
    DPP_SDT_TBL_DATA_T sdt_tbl = {0};
    DPP_SDTTBL_HASH_T sdt_hash_info = {0};  /*SDT内容*/

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(delete_en, 0, 1);
    ZXIC_COMM_CHECK_INDEX(dma_en, 0, 1);
    ZXIC_COMM_CHECK_INDEX(entry_num, 1, 0xCC);
    ZXIC_COMM_CHECK_POINT(p_arr_hash_entry);
    ZXIC_COMM_CHECK_POINT(element_id);

    /** 从软件缓存读取sdt信息 */
    rc = dpp_sdt_tbl_data_get(dev, sdt_no, &sdt_tbl);
    ZXIC_COMM_CHECK_RC(rc, "dpp_sdt_tbl_data_get");

    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_hash_info);
    ZXIC_COMM_CHECK_RC(rc, "dpp_soft_sdt_tbl_get");

    dtb_entry.cmd = entry_cmd_buff;
    dtb_entry.data = entry_data_buff;

    //分配下表数据缓存 16K
    table_data_buff = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(DPP_DTB_TABLE_DATA_BUFF_SIZE);
    ZXIC_COMM_CHECK_POINT(table_data_buff);
    ZXIC_COMM_MEMSET_S(table_data_buff, DPP_DTB_TABLE_DATA_BUFF_SIZE, 0, DPP_DTB_TABLE_DATA_BUFF_SIZE);

    for(i = 0; i < entry_num; i++)
    {
        ZXIC_COMM_MEMCPY((ZXIC_UINT8 *)entry_buff, p_arr_hash_entry[i].p_actu_key, sdt_hash_info.key_size); //写入key
        ZXIC_COMM_MEMCPY((ZXIC_UINT8 *)entry_buff + sdt_hash_info.key_size, p_arr_hash_entry[i].p_rst, ((1 << sdt_hash_info.rsp_mode) * 4)); //写入rst

        for (j = 0; j < (SE_ENTRY_WIDTH_MAX / 4); j++)
        {
            zxic_comm_swap((ZXIC_UINT8 *)&entry_buff[j], sizeof(ZXIC_UINT32));
        }

        rc = dpp_dtb_kschd_hash_write_entry_data(DEV_ID(dev),
                                                sdt_tbl.data_high32,
                                                sdt_tbl.data_low32,
                                                delete_en,
                                                dma_en,
                                                entry_buff,
                                                &dtb_entry);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, "dpp_dtb_kschd_hash_write_entry_data", table_data_buff);

        dtb_len += DTB_MC_HASH_LEN_SIZE;
        addr_offset = i * DTB_MC_HASH_LEN_SIZE * DTB_LEN_POS_SETP; /*在缓存中相对于0的offset*/

        rc = dpp_dtb_data_write(table_data_buff, addr_offset, &dtb_entry);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_data_write", table_data_buff);

        ZXIC_COMM_MEMSET_S(entry_cmd_buff, DTB_TABLE_CMD_SIZE_BYTE, 0, DTB_TABLE_CMD_SIZE_BYTE);
        ZXIC_COMM_MEMSET_S(entry_data_buff, SE_ENTRY_WIDTH_MAX, 0, SE_ENTRY_WIDTH_MAX);
    }

    rc = dpp_dtb_write_down_table_data(dev,
                                       queue_id,
                                       dtb_len * 16,
                                       table_data_buff,
                                       element_id);
    ZXIC_COMM_FREE(table_data_buff);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_write_down_table_data");

    return DPP_OK;
}

/** dtb模拟微码插入HASH表项
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   sdt_no    hash表sdt表号
* @param   entry_cnt 下发的条目数
* @param   p_arr_hash_entry   待下发表项内容结构体数组指针
* @param   element_id   返回下表使用的元素id
* @return
* @remark
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_hash_simu_mcode_insert(DPP_DEV_T *dev,
                                   ZXIC_UINT32 queue_id,
                                   ZXIC_UINT32 sdt_no,
                                   ZXIC_UINT32 entry_num,
                                   DPP_DTB_HASH_ENTRY_INFO_T *p_arr_hash_entry,
                                   ZXIC_UINT32 *element_id)
{
    DPP_STATUS  rc = DPP_OK;

    rc = dpp_dtb_hash_simu_mcode_write(dev, queue_id, sdt_no, 0, 0, entry_num, p_arr_hash_entry, element_id);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_hash_simu_mcode_write");

    return DPP_OK;
}

/** dtb模拟微码删除HASH表项
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   sdt_no    hash表sdt表号
* @param   entry_cnt 下发的条目数
* @param   p_arr_hash_entry   待下发表项内容结构体数组指针
* @param   element_id   返回下表使用的元素id
* @return
* @remark
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_hash_simu_mcode_delete(DPP_DEV_T *dev,
                                   ZXIC_UINT32 queue_id,
                                   ZXIC_UINT32 sdt_no,
                                   ZXIC_UINT32 entry_num,
                                   DPP_DTB_HASH_ENTRY_INFO_T *p_arr_hash_entry,
                                   ZXIC_UINT32 *element_id)
{
    DPP_STATUS  rc = DPP_OK;

    rc = dpp_dtb_hash_simu_mcode_write(dev, queue_id, sdt_no, 1, 0, entry_num, p_arr_hash_entry, element_id);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_hash_simu_mcode_write");

    return DPP_OK;
}

DPP_STATUS dpp_dtb_hash_dma_insert(DPP_DEV_T *dev, 
                                   ZXIC_UINT32 queue_id, 
                                   ZXIC_UINT32 sdt_no, 
                                   ZXIC_UINT32 entry_num, 
                                   DPP_DTB_HASH_ENTRY_INFO_T *p_arr_hash_entry,
                                   ZXIC_UINT32 *element_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 entry_num_max = 0;
    ZXIC_UINT32 entry_cycle = 0;
    ZXIC_UINT32 entry_remains = 0;
    ZXIC_UINT32 i = 0;
    DPP_DTB_HASH_ENTRY_INFO_T *p_entry = NULL;

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);

    entry_num_max = 0xcc;//按512bit数据进行计算

    entry_cycle = entry_num / entry_num_max;
    entry_remains = entry_num % entry_num_max;

    for(i = 0; i < entry_cycle; ++i)
    {
        p_entry = p_arr_hash_entry + entry_num_max * i;
        rc = dpp_dtb_hash_dma_insert_cycle(dev,
                                     queue_id,
                                     sdt_no,
                                     entry_num_max,
                                     p_entry,
                                     element_id);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_hash_dma_insert_cycle");
        ZXIC_COMM_TRACE_INFO("slot [%d] queue [%d] element_id [%d] done.\n",DEV_PCIE_SLOT(dev), queue_id, *element_id);
    }

    if(entry_remains)
    {
        p_entry = p_arr_hash_entry + entry_num_max * entry_cycle;
        rc = dpp_dtb_hash_dma_insert_cycle(dev,
                                     queue_id,
                                     sdt_no,
                                     entry_remains,
                                     p_entry,
                                     element_id);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_hash_dma_insert_cycle");
        ZXIC_COMM_TRACE_INFO("slot [%d] queue [%d] element_id [%d] done.\n",DEV_PCIE_SLOT(dev), queue_id, *element_id);
    }

    return DPP_OK;
}

DPP_STATUS dpp_dtb_hash_dma_delete(DPP_DEV_T *dev, 
                                   ZXIC_UINT32 queue_id, 
                                   ZXIC_UINT32 sdt_no, 
                                   ZXIC_UINT32 entry_num, 
                                   DPP_DTB_HASH_ENTRY_INFO_T *p_arr_hash_entry,
                                   ZXIC_UINT32 *element_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 entry_num_max = 0;
    ZXIC_UINT32 entry_cycle = 0;
    ZXIC_UINT32 entry_remains = 0;
    ZXIC_UINT32 i = 0;
    DPP_DTB_HASH_ENTRY_INFO_T *p_entry = NULL;

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);

    entry_num_max = 0xcc;//按512bit数据进行计算

    entry_cycle = entry_num / entry_num_max;
    entry_remains = entry_num % entry_num_max;

    for(i = 0; i < entry_cycle; ++i)
    {
        p_entry = p_arr_hash_entry + entry_num_max * i;
        rc = dpp_dtb_hash_dma_delete_cycle(dev,
                                     queue_id,
                                     sdt_no,
                                     entry_num_max,
                                     p_entry,
                                     element_id);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_hash_dma_delete_cycle");
        ZXIC_COMM_TRACE_INFO("slot [%d] queue [%d] element_id [%d] done.\n",DEV_PCIE_SLOT(dev), queue_id, *element_id);
    }

    if(entry_remains)
    {
        p_entry = p_arr_hash_entry + entry_num_max * entry_cycle;
        rc = dpp_dtb_hash_dma_delete_cycle(dev,
                                     queue_id,
                                     sdt_no,
                                     entry_remains,
                                     p_entry,
                                     element_id);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_hash_dma_delete_cycle");
        ZXIC_COMM_TRACE_INFO("slot [%d] queue [%d] element_id [%d] done.\n",DEV_PCIE_SLOT(dev), queue_id, *element_id);
    }

    return DPP_OK;
}

/** dtb删除HASH表
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   sdt_no    hash表sdt表号
* @param   entry_cnt 删除的条目数
* @param   p_entry_arr   待下发表项内容结构体数组指针
* @param   element_id   返回下表使用的元素id
* @return
* @remark  无 
* @see
* @author        @date  2023/03/14
************************************************************/
DPP_STATUS dpp_dtb_hash_dma_delete_cycle(DPP_DEV_T *dev, 
                                   ZXIC_UINT32 queue_id, 
                                   ZXIC_UINT32 sdt_no, 
                                   ZXIC_UINT32 entry_num, 
                                   DPP_DTB_HASH_ENTRY_INFO_T *p_arr_hash_entry,
                                   ZXIC_UINT32 *element_id)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT8 key_valid = 1;
    ZXIC_UINT32 item_cnt = 0;
    // ZXIC_UINT32 ddr_wr_mode = 0;
    ZXIC_UINT32 dtb_len = 0;
    
    SE_ITEM_CFG *p_item = NULL;
    ZXIC_RB_TN *p_rb_tn_rtn = NULL;
    DPP_HASH_CFG *p_hash_cfg = NULL;
    DPP_HASH_RBKEY_INFO *p_rbkey_rtn = NULL;
    DPP_HASH_RBKEY_INFO temp_rbkey = {{0}};
    ZXIC_UINT8 *p_data_buff = NULL;
    ZXIC_MUTEX_T *p_hash_mutex = NULL;

    HASH_ENTRY_CFG  hash_entry_cfg = {0}; 

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX_LOWER(DEV_ID(dev), entry_num, 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_arr_hash_entry);


    //从sdt_no中获取hash配置
    rc = dpp_hash_get_hash_info_from_sdt(dev, sdt_no, &hash_entry_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_hash_get_hash_info_from_sdt");

    p_hash_cfg = hash_entry_cfg.p_hash_cfg;
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_hash_cfg);

     /*条目数上限检查*/
    // ddr_wr_mode = DPP_GET_DDR_WR_MODE(hash_entry_cfg.key_type);
    // ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), entry_num, 1, DTB_DATA_SIZE_BIT / ( (ddr_wr_mode + 2) * DTB_TABLE_CMD_SIZE_BIT));

    p_data_buff = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(DPP_DTB_TABLE_DATA_BUFF_SIZE);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data_buff);
    ZXIC_COMM_MEMSET(p_data_buff,0x0,DPP_DTB_TABLE_DATA_BUFF_SIZE);

    rc = dpp_dev_hash_opr_mutex_get(dev, p_hash_cfg->fun_id, &p_hash_mutex);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dev_opr_mutex_get",p_data_buff); 
    rc = zxic_comm_mutex_lock(p_hash_mutex);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "zxic_comm_mutex_lock",p_data_buff); 
    
      /*对每一个条目进行处理*/
    for(item_cnt = 0; item_cnt < entry_num; ++item_cnt)
    {
        ZXIC_COMM_MEMSET(&temp_rbkey,0x0,sizeof(DPP_HASH_RBKEY_INFO));
        temp_rbkey.key[0] = (ZXIC_UINT8)(((key_valid & 0x1) << 7) | ((hash_entry_cfg.key_type & 0x3) << 5) | (hash_entry_cfg.table_id & 0x1f));
        ZXIC_COMM_MEMCPY(&temp_rbkey.key[1], p_arr_hash_entry[item_cnt].p_actu_key, hash_entry_cfg.actu_key_size);
        rc = zxic_comm_rb_delete(&p_hash_cfg->hash_rb, &temp_rbkey, &p_rb_tn_rtn);
        if (ZXIC_RBT_RC_SRHFAIL == rc)
        {
            p_hash_cfg->hash_stat.delete_fail++;
            ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "Error!there is not item in hash!\n");
            continue;
        }

        if(NULL == p_rb_tn_rtn)
        {
            ZXIC_COMM_FREE(p_data_buff);
            rc = zxic_comm_mutex_unlock(p_hash_mutex);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
            return ZXIC_PAR_CHK_POINT_NULL;
        }

        p_rbkey_rtn = (DPP_HASH_RBKEY_INFO *)(p_rb_tn_rtn->p_key);
        ZXIC_COMM_MEMSET(p_rbkey_rtn->rst,0x0,sizeof(p_rbkey_rtn->rst));
        hash_entry_cfg.p_rbkey_new = p_rbkey_rtn;
        hash_entry_cfg.p_rb_tn_new = p_rb_tn_rtn;

        p_item = p_rbkey_rtn->p_item_info;
        rc = zxic_comm_double_link_del(&(p_rbkey_rtn->entry_dn), &(p_item->item_list));
        if(DPP_OK != rc)
        {
            ZXIC_COMM_FREE(p_data_buff);
            rc = zxic_comm_mutex_unlock(p_hash_mutex);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
            return rc;
        }
        p_item->wrt_mask &= ~(DPP_GET_HASH_ENTRY_MASK(p_rbkey_rtn->entry_size, p_rbkey_rtn->entry_pos)) & 0xF;

        /*将位置信息和数据信息写入buff中*/
        rc = dpp_dtb_delete_hash_buf_write(DEV_ID(dev),&hash_entry_cfg,p_data_buff,&dtb_len);
        if(DPP_OK != rc)
        {
            ZXIC_COMM_FREE(p_data_buff);
            rc = zxic_comm_mutex_unlock(p_hash_mutex);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
            return rc;
        }

        if (0 == p_item->item_list.used)
        {
            if ((ITEM_DDR_256 == p_item->item_type) || (ITEM_DDR_512 == p_item->item_type))
            {
                /* modify coverity by yinxh 2021.03.10*,以256bit为单位。暂不考虑512bit的情况*/
                if((p_item->item_index)>(p_hash_cfg->p_bulk_ddr_info[hash_entry_cfg.bulk_id]->item_num))
                {
                    ZXIC_COMM_FREE(p_data_buff);
                    rc = zxic_comm_mutex_unlock(p_hash_mutex);
                    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "zxic_comm_mutex_unlock");
                    return ZXIC_PAR_CHK_INVALID_INDEX;
                }
                p_hash_cfg->p_bulk_ddr_info[hash_entry_cfg.bulk_id]->p_item_array[p_item->item_index] = NULL;
                ZXIC_COMM_FREE(p_item);
            }
            else
            {
                p_item->valid = 0;
            }
        }

        ZXIC_COMM_FREE(p_rbkey_rtn);
        ZXIC_COMM_FREE(p_rb_tn_rtn);

        p_hash_cfg->hash_stat.delete_ok++;
        p_hash_cfg->hash_stat.insert_table[hash_entry_cfg.table_id].delete++;
    }

    rc = zxic_comm_mutex_unlock(p_hash_mutex);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "zxic_comm_mutex_unlock",p_data_buff);
    
    if(dtb_len)
    {
        rc = dpp_dtb_write_down_table_data(dev,  
                                     queue_id, 
                                     dtb_len * 16,
                                     p_data_buff,
                                     element_id);                               
        ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_write_down_table_data",p_data_buff);
    }
    ZXIC_COMM_FREE(p_data_buff);  

    return DPP_OK;
}

/** dtb删除所有的HASH表(硬件查找，硬件删除)
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   sdt_no    hash表sdt表号
* @param   entry_cnt 删除的条目数
* @param   p_entry_arr   待下发表项内容结构体数组指针
* @param   element_id   返回下表使用的元素id
* @return
* @remark  无 
* @see
* @author   cq     @date  2023/12/02
************************************************************/
DPP_STATUS dpp_dtb_hash_dma_delete_hardware(DPP_DEV_T *dev, 
                                   ZXIC_UINT32 queue_id, 
                                   ZXIC_UINT32 sdt_no, 
                                   ZXIC_UINT32 entry_num, 
                                   DPP_DTB_HASH_ENTRY_INFO_T *p_arr_hash_entry,
                                   ZXIC_UINT32 *element_id)
{
    DPP_STATUS rc = DPP_OK;
    // ZXIC_UINT32 ddr_wr_mode = 0;
    ZXIC_UINT32 item_cnt = 0;
    ZXIC_UINT32 dtb_len = 0;
    ZXIC_UINT8 srh_succ = 0;
    ZXIC_UINT8 key[HASH_KEY_MAX] = {0};
    ZXIC_UINT8 rst[HASH_RST_MAX] = {0};
    ZXIC_UINT8 entry_cmd[DTB_TABLE_CMD_SIZE_BIT / 8] = {0};
    ZXIC_UINT8 entry_data[DPP_ETCAM_WIDTH_MAX/8] = {0};   /*兼容所有表项，按照最大位宽分配，acl最大为640bit(80B)*/
    ZXIC_UINT8 *p_data_buff = NULL;
    ZXIC_UINT32 dev_id = 0;

    DPP_HASH_ENTRY  entry = {0};
    DPP_HASH_CFG *p_hash_cfg = NULL;
    DPP_DTB_ENTRY_T   dtb_one_entry = {0};
    HASH_ENTRY_CFG  hash_entry_cfg = {0}; 

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);
    dev_id = DEV_ID(dev);

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_arr_hash_entry);

    dtb_one_entry.cmd = entry_cmd;
    dtb_one_entry.data = entry_data;
    entry.p_key = key;
    entry.p_rst = rst;

    //从sdt_no中获取hash配置
    rc = dpp_hash_get_hash_info_from_sdt(dev, sdt_no, &hash_entry_cfg);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_hash_get_hash_info_from_sdt");

    p_hash_cfg = hash_entry_cfg.p_hash_cfg;
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_hash_cfg);
    
    /*条目数上限检查*/
    // ddr_wr_mode = DPP_GET_DDR_WR_MODE(hash_entry_cfg.key_type);
    // ZXIC_COMM_CHECK_DEV_INDEX(dev_id, entry_num, 1, DTB_DATA_SIZE_BIT / ( (ddr_wr_mode + 2) * DTB_TABLE_CMD_SIZE_BIT));

    p_data_buff = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(DPP_DTB_TABLE_DATA_BUFF_SIZE);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_data_buff);
    ZXIC_COMM_MEMSET(p_data_buff,0x0,DPP_DTB_TABLE_DATA_BUFF_SIZE);
    
      /*对每一个条目进行处理*/
    for(item_cnt = 0; item_cnt < entry_num; ++item_cnt)
    {
        srh_succ = 0;
        ZXIC_COMM_MEMSET(key,0x0,sizeof(key));
        ZXIC_COMM_MEMSET(rst,0x0,sizeof(rst));
        ZXIC_COMM_MEMSET(entry_cmd,0x0,sizeof(entry_cmd));
        ZXIC_COMM_MEMSET(entry_data,0x0,sizeof(entry_data));
        ZXIC_COMM_MEMCPY(entry.p_key, p_arr_hash_entry[item_cnt].p_actu_key, hash_entry_cfg.key_by_size);

        rc = dpp_dtb_hash_zcam_delete_hardware(dev, 
                                                  queue_id,
                                                  &hash_entry_cfg,
                                                  &entry,
                                                  &dtb_one_entry,
                                                  &srh_succ);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_hash_zcam_delete_hardware",p_data_buff);

        if(srh_succ)
        {
            rc = dpp_dtb_data_write(p_data_buff, 0, &dtb_one_entry);
            ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_data_write",p_data_buff); 
            dtb_len = dtb_one_entry.data_size/DTB_LEN_POS_SETP + 1;

            rc = dpp_dtb_write_down_table_data(dev,  
                                     queue_id, 
                                     dtb_len * DTB_LEN_POS_SETP,
                                     p_data_buff,
                                     element_id);                               
            ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_write_down_table_data",p_data_buff);
        }
    }

    ZXIC_COMM_FREE(p_data_buff);  

    return DPP_OK;
}

/***********************************************************/
/** 将hash表DDR存储位宽(按照256bit/512bit划分)转换为hash在DDR中的实际位置(按照128/256/512bit划分)
* @param   index          hash表DDR存储位宽索引(按照256bit/512bit划分)
* @param   width_mode     DDR位宽(256bit/512bit)
* @param   key_type       hash表项位宽DPP_HASH_KEY_TYPE
* @param   byte_offset    hash表项在一个DDR存储位宽(256bit/512bit)里的偏移,单位为字节
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
ZXIC_UINT32 dpp_ddr_index_calc(ZXIC_UINT32 index,
                               ZXIC_UINT32 width_mode,
                               ZXIC_UINT32 key_type,
                               ZXIC_UINT32 byte_offset)
{
    ZXIC_UINT32 ddr_index = 0;      /*按照hash位宽128/256/512bit进行划分*/
    ZXIC_UINT32 entry_size = 0;
    
    entry_size = DPP_GET_HASH_ENTRY_SIZE(key_type);
    if(0==entry_size)
    {
        return ddr_index;
    }

    if(DDR_WIDTH_256b==width_mode)
    {
        if (HASH_KEY_128b == key_type)
        {
            ddr_index = (index<<1) + ((byte_offset/entry_size)&0x1);
        }
        else if (HASH_KEY_256b == key_type)
        {
            ddr_index = index;
        }
    }
    else if(DDR_WIDTH_512b==width_mode)
    {
        if (HASH_KEY_128b == key_type)
        {
            ddr_index = (index<<2) + ((byte_offset/entry_size)&0x3);
        }
        else if (HASH_KEY_256b == key_type)
        {
            ddr_index = (index<<1) + ((byte_offset/entry_size)&0x1);
        }
        else if (HASH_KEY_512b == key_type)
        {
            ddr_index = index;
        }
    }

    return ddr_index;
}

/***********************************************************/
/** 查找存储在ZCAM空间的hash表项并删除(硬件处理)
* @param   dev_id       设备号，支持多芯片 
* @param   queue_id     队列id 
* @param   p_hash_entry_cfg hash表项配置信息
* @param   p_hash_entry    查找键值信息
* @param   p_entry         dtb描述符
* @param   p_srh_succ      出参，查找是否成功
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
ZXIC_UINT32 dpp_dtb_hash_zcam_delete_hardware(DPP_DEV_T *dev, 
                                 ZXIC_UINT32 queue_id,
                                 HASH_ENTRY_CFG  *p_hash_entry_cfg,
                                 DPP_HASH_ENTRY  *p_hash_entry,
                                 DPP_DTB_ENTRY_T *p_entry,
                                 ZXIC_UINT8 *p_srh_succ)
{
    DPP_STATUS rc = DPP_OK;
    DPP_HASH_RBKEY_INFO srh_rbkey = {0};
    DPP_HASH_CFG *p_hash_cfg = NULL;
    SE_ZCELL_CFG *p_zcell = NULL;
    SE_ZBLK_CFG *p_zblk = NULL;

    ZXIC_UINT32 zblk_idx = 0;
    ZXIC_UINT32 pre_zblk_idx = 0xFFFFFFFF;/* -1; */
    ZXIC_UINT16 crc16_value = 0;
    ZXIC_UINT32 zcell_id = 0;
    ZXIC_UINT32 item_idx = 0;
    ZXIC_UINT32 element_id = 0;
    ZXIC_UINT8 byte_offset = 0;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 addr = 0;
    ZXIC_UINT32 i    = 0;
    ZXIC_UINT8 srh_succ     = 0;
    ZXIC_UINT8 temp_key[HASH_KEY_MAX] = {0};
    ZXIC_UINT8  rd_buff[SE_ITEM_WIDTH_MAX]   = {0};

    D_NODE *p_zblk_dn = NULL;
    D_NODE *p_zcell_dn = NULL;
    DPP_SE_CFG *p_se_cfg = NULL;

    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id,DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_POINT(p_hash_entry_cfg);
    ZXIC_COMM_CHECK_POINT(p_srh_succ);
    ZXIC_COMM_CHECK_POINT(p_hash_entry);
    ZXIC_COMM_CHECK_POINT(p_hash_entry->p_key);
    ZXIC_COMM_CHECK_POINT(p_hash_entry->p_rst);

    ZXIC_COMM_MEMSET(rd_buff,0x0,sizeof(rd_buff));
    ZXIC_COMM_MEMSET(&srh_rbkey,0x0,sizeof(DPP_HASH_RBKEY_INFO));
    ZXIC_COMM_CHECK_INDEX_UPPER(p_hash_entry_cfg->key_by_size, HASH_KEY_MAX);
    ZXIC_COMM_MEMCPY(srh_rbkey.key,p_hash_entry->p_key,p_hash_entry_cfg->key_by_size);
   
    p_hash_cfg = p_hash_entry_cfg->p_hash_cfg;
    ZXIC_COMM_CHECK_POINT(p_hash_cfg);
    
    /* 取出se配置 */
    rc = dpp_se_cfg_get(dev, &p_se_cfg);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_cfg_get");
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_se_cfg);
    
    ZXIC_COMM_CHECK_INDEX_UPPER(p_hash_entry_cfg->key_by_size, HASH_KEY_MAX);
    rc = dpp_hash_set_crc_key(dev,p_hash_entry_cfg,p_hash_entry,temp_key);
    ZXIC_COMM_CHECK_DEV_RC(dev_id,rc,"hash_set_crc_key");

    /*查找所有的zcell*/
    p_zcell_dn = p_hash_cfg->hash_shareram.zcell_free_list.p_next;
    while (p_zcell_dn)
    {
        p_zcell = (SE_ZCELL_CFG *)p_zcell_dn->data;
        zblk_idx = GET_ZBLK_IDX(p_zcell->zcell_idx);
        ZXIC_COMM_CHECK_DEV_INDEX(dev_id, zblk_idx, 0, SE_ZBLK_NUM - 1);
        p_zblk = &(p_se_cfg->zblk_info[zblk_idx]);

        if (zblk_idx != pre_zblk_idx)
        {
            pre_zblk_idx = zblk_idx;
            crc16_value = p_hash_cfg->p_hash16_fun(temp_key, p_hash_entry_cfg->key_by_size, p_zblk->hash_arg);
        }

        zcell_id = GET_ZCELL_IDX(p_zcell->zcell_idx);
        item_idx = GET_ZCELL_CRC_VAL(zcell_id, crc16_value);
        ZXIC_COMM_CHECK_DEV_INDEX(dev_id, item_idx, 0, SE_RAM_DEPTH - 1);
        addr = ZBLK_ITEM_ADDR_CALC(p_zcell->zcell_idx, item_idx);
        rc =  dpp_dtb_se_zcam_dma_dump(dev, 
                                       queue_id, 
                                       addr, 
                                       DTB_DUMP_ZCAM_512b, 
                                       1,
                                       (ZXIC_UINT32 *)rd_buff, 
                                       &element_id);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_se_zcam_dma_dump");
        zxic_comm_swap(rd_buff,sizeof(rd_buff));
        
        rc = dpp_dtb_hash_data_parse(ITEM_RAM,p_hash_entry_cfg->key_by_size,p_hash_entry,rd_buff,&byte_offset);
        if(DPP_OK == rc)
        {
            ZXIC_COMM_TRACE_DEBUG("Hash search hardware succ in zcell. \n");
            srh_succ = 1;
            p_hash_cfg->hash_stat.search_ok++;
            break;
        }
        
        p_zcell_dn = p_zcell_dn->next;
    }

    /*zcell查找失败，则查找所有的zreg*/
    if(0==srh_succ)
    {
        p_zblk_dn = p_hash_cfg->hash_shareram.zblk_list.p_next;
        while (p_zblk_dn)
        {
            p_zblk = (SE_ZBLK_CFG *)p_zblk_dn->data;
            zblk_idx = p_zblk->zblk_idx;

            for (i = 0; i < SE_ZREG_NUM; i++)
            {
                item_idx = i;
                addr = ZBLK_HASH_LIST_REG_ADDR_CALC(zblk_idx, item_idx);
                rc =  dpp_dtb_se_zcam_dma_dump(dev, 
                                       queue_id, 
                                       addr, 
                                       DTB_DUMP_ZCAM_512b, 
                                       1,
                                       (ZXIC_UINT32 *)rd_buff, 
                                       &element_id);
                ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_se_zcam_dma_dump");
                zxic_comm_swap(rd_buff,sizeof(rd_buff));

                rc = dpp_dtb_hash_data_parse(ITEM_RAM,p_hash_entry_cfg->key_by_size,p_hash_entry,rd_buff,&byte_offset);
                if(DPP_OK == rc)
                {
                    ZXIC_COMM_TRACE_DEBUG("Hash search hardware succ in zreg. \n");
                    srh_succ = 1;
                    p_hash_cfg->hash_stat.search_ok++;
                    break;
                }
            }
            p_zblk_dn = p_zblk_dn->next;
        }
    }

    /*查找成功，则将对应表项删除*/
    if(srh_succ)
    {
        ZXIC_COMM_CHECK_INDEX_UPPER(byte_offset, SE_ITEM_WIDTH_MAX - 1);
        ZXIC_COMM_MEMSET_S(rd_buff+byte_offset,SE_ITEM_WIDTH_MAX - byte_offset, 0x0, DPP_GET_HASH_ENTRY_SIZE(p_hash_entry_cfg->key_type));
        zxic_comm_swap(rd_buff,sizeof(rd_buff));
        rc = dpp_dtb_se_alg_zcam_data_write(dev_id,
                                       addr,
                                       rd_buff,
                                       p_entry);
        ZXIC_COMM_CHECK_DEV_RC(dev_id,rc,"dpp_dtb_se_alg_zcam_data_write");
        p_hash_cfg->hash_stat.delete_ok++;
        ZXIC_COMM_CHECK_INDEX_UPPER(p_hash_entry_cfg->table_id,HASH_TBL_ID_NUM - 1);
        p_hash_cfg->hash_stat.insert_table[p_hash_entry_cfg->table_id].delete++;
    }

    *p_srh_succ = srh_succ;

    return DPP_OK;
}

/** dtb写ACL表 （SPECIFY模式，条目中指定handle，支持级联64bit/128bit）
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   sdt_no    ACL表sdt表号
* @param   entry_num 下发的条目数
* @param   p_acl_entry_arr   待下发表项内容结构体数组指针
* @param   element_id    返回下表使用的元素id
* @return
* @remark  无 是否是有一个写不成功就返回，还是继续进行下一个条目并记录错误的条目
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_acl_dma_insert(DPP_DEV_T *dev,  
                                      ZXIC_UINT32 queue_id, 
                                      ZXIC_UINT32 sdt_no, 
                                      ZXIC_UINT32 entry_num, 
                                      DPP_DTB_ACL_ENTRY_INFO_T *p_acl_entry_arr,
                                      ZXIC_UINT32 *element_id
                                      )
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 as_enable;
    ZXIC_UINT32 etcam_as_mode;
    ZXIC_UINT32 entry_num_max = 0;
    ZXIC_UINT32 entry_cycle = 0;
    ZXIC_UINT32 entry_remains = 0;
    ZXIC_UINT32 i = 0;
    DPP_DTB_ACL_ENTRY_INFO_T *p_entry = NULL;

    DPP_SDTTBL_ETCAM_T sdt_etcam_info = {0};  /*SDT内容*/

    ZXIC_COMM_CHECK_POINT(dev);
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_etcam_info);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_soft_sdt_tbl_get");

    as_enable = sdt_etcam_info.as_en;
    etcam_as_mode = sdt_etcam_info.as_rsp_mode;

    if(!as_enable)
    {
        entry_num_max = 0x55;
    }else 
    {
        if(etcam_as_mode == ERAM128_TBL_128b)
        {
            entry_num_max = 0x49;
        }else
        {
            entry_num_max = 0x4e;
        }
    }

    entry_cycle = entry_num / entry_num_max;
    entry_remains = entry_num % entry_num_max;

    for(i = 0; i < entry_cycle; ++i)
    {
        p_entry = p_acl_entry_arr + entry_num_max * i;
        rc = dpp_dtb_acl_dma_insert_cycle(dev,
                                     queue_id,
                                     sdt_no,
                                     entry_num_max,
                                     p_entry,
                                     element_id);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_acl_dma_insert_cycle");

        ZXIC_COMM_TRACE_INFO("slot [%d] queue [%d] element_id [%d] done.\n",DEV_PCIE_SLOT(dev), queue_id, *element_id);
    }

    if(entry_remains)
    {
        p_entry = p_acl_entry_arr + entry_num_max * entry_cycle;
        rc = dpp_dtb_acl_dma_insert_cycle(dev,
                                     queue_id,
                                     sdt_no,
                                     entry_remains,
                                     p_entry,
                                     element_id);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_acl_dma_insert_cycle");

        ZXIC_COMM_TRACE_INFO("slot [%d] queue [%d] element_id [%d] done.\n",DEV_PCIE_SLOT(dev), queue_id, *element_id);
    }

    return DPP_OK;   
}

#endif

#if ZXIC_REAL("DTB DUMP BASE")

/** smmu0 组装dump描述符函数
* @param   dev_id         设备号
* @param   base_addr      smmu0空间基地址，以128bit为单位
* @param   depth          dump的深度以128bit为单位
* @param   addr_high32    dump缓存地址高32bit
* @param   addr_low32     dump缓存地址低32bit
* @param   p_dump_info    dump描述符指针（已分配好空间128bit）
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_smmu0_dump_info_write(DPP_DEV_T *dev,  
                                      ZXIC_UINT32 base_addr, 
                                      ZXIC_UINT32 depth,
                                      ZXIC_UINT32 addr_high32,
                                      ZXIC_UINT32 addr_low32,
                                      ZXIC_UINT32 *p_dump_info)
{
    DPP_STATUS rc = DPP_OK;

    DPP_DTB_ERAM_DUMP_FORM_T dtb_eram_dump_form_info = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_dump_info); 

    //组装dump描述符
    dtb_eram_dump_form_info.valid = DTB_TABLE_VALID;
    dtb_eram_dump_form_info.up_type = DTB_DUMP_MODE_ERAM;
    dtb_eram_dump_form_info.base_addr = base_addr;
    dtb_eram_dump_form_info.tb_depth = depth;
    dtb_eram_dump_form_info.tb_dst_addr_h = addr_high32;
    dtb_eram_dump_form_info.tb_dst_addr_l = addr_low32;

    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_PRINT("dpp_dtb_smmu0_dump_info_write: \n");
        ZXIC_COMM_DBGCNT32_PRINT("valid", dtb_eram_dump_form_info.valid);
        ZXIC_COMM_DBGCNT32_PRINT("up_type", dtb_eram_dump_form_info.up_type);
        ZXIC_COMM_DBGCNT32_PRINT("base_addr", dtb_eram_dump_form_info.base_addr);
        ZXIC_COMM_DBGCNT32_PRINT("tb_depth",  dtb_eram_dump_form_info.tb_depth);
        ZXIC_COMM_DBGCNT32_PRINT("tb_dst_addr_h", dtb_eram_dump_form_info.tb_dst_addr_h);
        ZXIC_COMM_DBGCNT32_PRINT("tb_dst_addr_l", dtb_eram_dump_form_info.tb_dst_addr_l); 
    }

    /*组装dump描述符*/
    rc = dpp_dtb_write_dump_cmd(DEV_ID(dev), DTB_DUMP_ERAM, &dtb_eram_dump_form_info, p_dump_info);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_write_dump_cmd");

    return rc;
}

/** smmu1 组装dump描述符函数
* @param   dev_id         设备号
* @param   base_addr      dump基地址，以512it为单位
* @param   depth          dump的深度以512bit为单位
* @param   addr_high32    dump缓存地址高32bit
* @param   addr_low32     dump缓存地址低32bit
* @param   p_dump_info    dump描述符指针（已分配好空间128bit）
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_smmu1_dump_info_write(DPP_DEV_T *dev, 
                                      ZXIC_UINT32 base_addr, 
                                      ZXIC_UINT32 depth,
                                      ZXIC_UINT32 addr_high32,
                                      ZXIC_UINT32 addr_low32,
                                      ZXIC_UINT32 *p_dump_info
                                      )
{
    DPP_STATUS rc = DPP_OK;

    DPP_DTB_DDR_DUMP_FORM_T dtb_ddr_dump_form_info = {0}; 

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_dump_info); 

    //组装dump描述符
    dtb_ddr_dump_form_info.valid = DTB_TABLE_VALID;
    dtb_ddr_dump_form_info.up_type = DTB_DUMP_MODE_DDR;
    dtb_ddr_dump_form_info.base_addr = base_addr;
    dtb_ddr_dump_form_info.tb_depth = depth;
    dtb_ddr_dump_form_info.tb_dst_addr_h = addr_high32;
    dtb_ddr_dump_form_info.tb_dst_addr_l = addr_low32;

    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_PRINT("dpp_dtb_smmu1_dump_info_write: \n");
        ZXIC_COMM_DBGCNT32_PRINT("valid", dtb_ddr_dump_form_info.valid);
        ZXIC_COMM_DBGCNT32_PRINT("up_type", dtb_ddr_dump_form_info.up_type);
        ZXIC_COMM_DBGCNT32_PRINT("base_addr", dtb_ddr_dump_form_info.base_addr);
        ZXIC_COMM_DBGCNT32_PRINT("tb_depth",  dtb_ddr_dump_form_info.tb_depth);
        ZXIC_COMM_DBGCNT32_PRINT("tb_dst_addr_h", dtb_ddr_dump_form_info.tb_dst_addr_h);
        ZXIC_COMM_DBGCNT32_PRINT("tb_dst_addr_l", dtb_ddr_dump_form_info.tb_dst_addr_l); 
    }

    /*组装dump描述符*/
    rc = dpp_dtb_write_dump_cmd(DEV_ID(dev), DTB_DUMP_DDR, &dtb_ddr_dump_form_info, p_dump_info);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_write_dump_cmd");

    return rc;
}

/** zcam 组装dump描述符函数
* @param   dev_id         设备号
* @param   addr           zcam内部ram地址，包括掩码(512bit为单位)
* @param   tb_width       dump数据的宽度 00:128bit 01:256bit 10:512bit
* @param   depth          dump的深度以512bit为单位
* @param   addr_high32    dump缓存地址高32bit
* @param   addr_low32     dump缓存地址低32bit
* @param   p_dump_info    dump描述符指针（已分配好空间128bit）
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_zcam_dump_info_write(DPP_DEV_T *dev, 
                                      ZXIC_UINT32 addr, 
                                      ZXIC_UINT32 tb_width,
                                      ZXIC_UINT32 depth,
                                      ZXIC_UINT32 addr_high32,
                                      ZXIC_UINT32 addr_low32,
                                      ZXIC_UINT32 *p_dump_info
                                      )
{
    DPP_STATUS rc = DPP_OK;

    DPP_DTB_ZCAM_DUMP_FORM_T dtb_zcam_dump_form_info = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_dump_info); 

    //组装dump描述符
    dtb_zcam_dump_form_info.valid = DTB_TABLE_VALID;
    dtb_zcam_dump_form_info.up_type = DTB_DUMP_MODE_ZCAM;
    dtb_zcam_dump_form_info.tb_width = tb_width;
    dtb_zcam_dump_form_info.sram_addr = addr & 0x1FF;//512bit为单位
    dtb_zcam_dump_form_info.ram_reg_flag = (addr >> 16) & 0x1;
    dtb_zcam_dump_form_info.z_reg_cell_id = (addr >> 9) & 0x3;
    dtb_zcam_dump_form_info.zblock_id = (addr >> 11) & 0x7;
    dtb_zcam_dump_form_info.zgroup_id = (addr >> 14) & 0x3;
    dtb_zcam_dump_form_info.tb_depth = depth;
    dtb_zcam_dump_form_info.tb_dst_addr_h = addr_high32;
    dtb_zcam_dump_form_info.tb_dst_addr_l = addr_low32;

    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_PRINT("dpp_dtb_zcam_dump_info_write: \n");
        ZXIC_COMM_DBGCNT32_PRINT("addr", addr);
        ZXIC_COMM_DBGCNT32_PRINT("valid", dtb_zcam_dump_form_info.valid);
        ZXIC_COMM_DBGCNT32_PRINT("up_type", dtb_zcam_dump_form_info.up_type);
        ZXIC_COMM_DBGCNT32_PRINT("zgroup_id", dtb_zcam_dump_form_info.zgroup_id);
        ZXIC_COMM_DBGCNT32_PRINT("zblock_id", dtb_zcam_dump_form_info.zblock_id);
        ZXIC_COMM_DBGCNT32_PRINT("reg_sram_flag", dtb_zcam_dump_form_info.ram_reg_flag);
        ZXIC_COMM_DBGCNT32_PRINT("zcell_id", dtb_zcam_dump_form_info.z_reg_cell_id);
        ZXIC_COMM_DBGCNT32_PRINT("tb_width", dtb_zcam_dump_form_info.tb_width);
        ZXIC_COMM_DBGCNT32_PRINT("sram_addr", dtb_zcam_dump_form_info.sram_addr);
        ZXIC_COMM_DBGCNT32_PRINT("tb_depth", dtb_zcam_dump_form_info.tb_depth);
        ZXIC_COMM_DBGCNT32_PRINT("tb_dst_addr_h", dtb_zcam_dump_form_info.tb_dst_addr_h);
        ZXIC_COMM_DBGCNT32_PRINT("tb_dst_addr_l", dtb_zcam_dump_form_info.tb_dst_addr_l); 
    }

    //将描述符写入dump缓存中
    rc = dpp_dtb_write_dump_cmd(DEV_ID(dev), DTB_DUMP_ZCAM, &dtb_zcam_dump_form_info, p_dump_info);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_write_dump_cmd");

    return rc;
}

/** etcam 组装dump描述符函数
* @param   dev_id         设备号
* @param   p_etcam_dump_info      etcam dump内容信息
* @param   addr_high32    dump缓存地址高32bit
* @param   addr_low32     dump缓存地址低32bit
* @param   p_dump_info    dump描述符指针（已分配好空间128bit）
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_etcam_dump_info_write(DPP_DEV_T *dev,  
                                      ETCAM_DUMP_INFO_T *p_etcam_dump_info,
                                      ZXIC_UINT32 addr_high32,
                                      ZXIC_UINT32 addr_low32,
                                      ZXIC_UINT32 *p_dump_info
                                      )
{
    DPP_STATUS rc = DPP_OK;

    DPP_DTB_ETCAM_DUMP_FORM_T dtb_etcam_dump_form_info = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_dump_info); 
    ZXIC_COMM_CHECK_POINT(p_etcam_dump_info); 

    //组装dump描述符
    dtb_etcam_dump_form_info.valid = DTB_TABLE_VALID;
    dtb_etcam_dump_form_info.up_type = DTB_DUMP_MODE_ETCAM;
    dtb_etcam_dump_form_info.block_sel = p_etcam_dump_info->block_sel;
    dtb_etcam_dump_form_info.addr = p_etcam_dump_info->addr;
    dtb_etcam_dump_form_info.rd_mode = p_etcam_dump_info->rd_mode;
    dtb_etcam_dump_form_info.data_or_mask = p_etcam_dump_info->data_or_mask;
    dtb_etcam_dump_form_info.tb_depth = p_etcam_dump_info->tb_depth;
    dtb_etcam_dump_form_info.tb_width = p_etcam_dump_info->tb_width;//00:80bit 01:160bit 10:320bit 11:640bit
    dtb_etcam_dump_form_info.tb_dst_addr_h = addr_high32;
    dtb_etcam_dump_form_info.tb_dst_addr_l = addr_low32;

    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_PRINT("dpp_dtb_etcam_dump_info_write: \n");
        ZXIC_COMM_DBGCNT32_PRINT("valid", dtb_etcam_dump_form_info.valid);
        ZXIC_COMM_DBGCNT32_PRINT("up_type", dtb_etcam_dump_form_info.up_type);
        ZXIC_COMM_DBGCNT32_PRINT("block_sel", dtb_etcam_dump_form_info.block_sel);
        ZXIC_COMM_DBGCNT32_PRINT("addr",  dtb_etcam_dump_form_info.addr);
        ZXIC_COMM_DBGCNT32_PRINT("rd_mode", dtb_etcam_dump_form_info.rd_mode);
        ZXIC_COMM_DBGCNT32_PRINT("data_or_mask", dtb_etcam_dump_form_info.data_or_mask);
        ZXIC_COMM_DBGCNT32_PRINT("tb_depth", dtb_etcam_dump_form_info.tb_depth);
        ZXIC_COMM_DBGCNT32_PRINT("tb_width",  dtb_etcam_dump_form_info.tb_width);
        ZXIC_COMM_DBGCNT32_PRINT("tb_dst_addr_h", dtb_etcam_dump_form_info.tb_dst_addr_h);
        ZXIC_COMM_DBGCNT32_PRINT("tb_dst_addr_l", dtb_etcam_dump_form_info.tb_dst_addr_l); 
    }

    //将描述符写入dump缓存中
    rc = dpp_dtb_write_dump_cmd(DEV_ID(dev), DTB_DUMP_ETCAM, &dtb_etcam_dump_form_info, p_dump_info);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_write_dump_cmd");

    return rc;
}

/** eram 生成dump entry，输入dump描述符，输出一个entry
* @param   dev_id         设备号
* @param   base_addr      smmu0空间基地址，以128bit为单位
* @param   depth          dump的深度以128bit为单位
* @param   addr_high32    dump缓存地址高32bit
* @param   addr_low32     dump缓存地址低32bit
* @param   p_entry        entry指针（已分配好cmd空间128bit,用来存放dump描述符）
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_smmu0_dump_entry(DPP_DEV_T *dev,  
                                    ZXIC_UINT32 base_addr, 
                                    ZXIC_UINT32 depth,
                                    ZXIC_UINT32 addr_high32,
                                    ZXIC_UINT32 addr_low32,
                                    DPP_DTB_ENTRY_T *p_entry)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_entry);

    rc = dpp_dtb_smmu0_dump_info_write(dev, 
                                       base_addr, 
                                       depth,
                                       addr_high32,
                                       addr_low32,
                                       (ZXIC_UINT32 *)p_entry->cmd);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_smmu0_dump_info_write");
    p_entry->data_in_cmd_flag = 1;

    return rc;
}

/** smmu1生成dump entry，输入dump描述符，输出一个entry
* @param   dev_id         设备号
* @param   base_addr      dump基地址，以128bit为单位
* @param   depth          dump的深度以512bit为单位
* @param   addr_high32    dump缓存地址高32bit
* @param   addr_low32     dump缓存地址低32bit
* @param   p_entry    entry指针（已分配好cmd空间128bit,用来存放dump描述符）
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_smmu1_dump_entry(DPP_DEV_T *dev, 
                                    ZXIC_UINT32 base_addr, 
                                    ZXIC_UINT32 depth,
                                    ZXIC_UINT32 addr_high32,
                                    ZXIC_UINT32 addr_low32,
                                    DPP_DTB_ENTRY_T *p_entry)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_entry);

    rc = dpp_dtb_smmu1_dump_info_write(dev, 
                                       base_addr, 
                                       depth,
                                       addr_high32,
                                       addr_low32,
                                       (ZXIC_UINT32 *)p_entry->cmd);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_smmu1_dump_info_write");
    p_entry->data_in_cmd_flag = 1;

    return rc;
}

/** zcam 组装dump描述符函数
* @param   dev_id         设备号
* @param   addr           zcam内部ram地址，包括掩码(512bit为单位)
* @param   tb_width       dump数据的宽度 00:128bit 01:256bit 10:512bit
* @param   depth          dump的深度以512bit为单位
* @param   addr_high32    dump缓存地址高32bit
* @param   addr_low32     dump缓存地址低32bit
* @param   p_entry    entry指针（已分配好cmd空间128bit,用来存放dump描述符）
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_zcam_dump_entry(DPP_DEV_T *dev, 
                                   ZXIC_UINT32 addr, 
                                   ZXIC_UINT32 tb_width,
                                   ZXIC_UINT32 depth,
                                   ZXIC_UINT32 addr_high32,
                                   ZXIC_UINT32 addr_low32,
                                   DPP_DTB_ENTRY_T *p_entry)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_entry);

    rc = dpp_dtb_zcam_dump_info_write(dev, 
                                      addr,
                                      tb_width, 
                                      depth,
                                      addr_high32,
                                      addr_low32,
                                      (ZXIC_UINT32 *)p_entry->cmd);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_zcam_dump_info_write");
    p_entry->data_in_cmd_flag = 1;

    return rc;
}

/** etcam 生成dump entry，输入dump描述符，输出一个entry
* @param   dev_id         设备号
* @param   p_etcam_dump_info      etcam dump内容信息
* @param   addr_high32    dump缓存地址高32bit
* @param   addr_low32     dump缓存地址低32bit
* @param   p_entry        entry指针（已分配好cmd空间128bit,用来存放dump描述符）
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_etcam_dump_entry(DPP_DEV_T *dev,   
                                    ETCAM_DUMP_INFO_T *p_etcam_dump_info,
                                    ZXIC_UINT32 addr_high32,
                                    ZXIC_UINT32 addr_low32,
                                    DPP_DTB_ENTRY_T *p_entry)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_entry);

    rc = dpp_dtb_etcam_dump_info_write(dev,
                                       p_etcam_dump_info,
                                       addr_high32,
                                       addr_low32,
                                       (ZXIC_UINT32 *)p_entry->cmd);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_etcam_dump_info_write");
    p_entry->data_in_cmd_flag = 1;

    return rc;
}


/** 下发dump描述符，dump出数据 
* @param   dev_id       设备号
* @param   queue_id     队列号
* @param   queue_element_id   元素编号
* @param   p_dump_info  dump描述符
* @param   data_len     dump出数据的长度(32bit为单位)
* @param   desc_len     dump描述符的长度(32bit为单位)
* @param   p_dump_data  dump出的数据缓存
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_write_dump_desc_info(DPP_DEV_T *dev,
                                ZXIC_UINT32 queue_id, 
                                ZXIC_UINT32 queue_element_id,
                                ZXIC_UINT32 *p_dump_info,
                                ZXIC_UINT32 data_len,
                                ZXIC_UINT32 desc_len,
                                ZXIC_UINT32 *p_dump_data )
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 dtb_interrupt_status = 0;

    ZXIC_COMM_CHECK_POINT(dev);

    if((!(dev->pcie_channel.dev_status)) || (!dtb_table_function_switch_get()))
    {
        ZXIC_COMM_TRACE_NOTICE("slot[%u] vport[0x%x] dev status off!\n",dev->pcie_channel.slot,dev->pcie_channel.vport);
        dpp_dtb_item_ack_wr(dev, queue_id, DPP_DTB_DIR_UP_TYPE, queue_element_id, 0, DPP_DTB_TAB_ACK_UNUSED_MASK);
        return ZXIC_PAR_CHK_DEV_STATUS_OFF;
    }

    /*获取中断配置*/
    dtb_interrupt_status = dpp_dtb_interrupt_status_get();
    
    /*下发dump描述符*/
    rc = dpp_dtb_tab_up_info_set(dev,
                                    queue_id,
                                    queue_element_id,
                                    dtb_interrupt_status,
                                    data_len,
                                    desc_len,
                                    p_dump_info);
    if(DPP_OK != rc)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "the queue %d element id %d dump info set failed!", queue_id, queue_element_id);
        dpp_dtb_item_ack_wr(dev, queue_id, DPP_DTB_DIR_UP_TYPE, queue_element_id, 0, DPP_DTB_TAB_ACK_UNUSED_MASK);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_tab_up_info_set");
    }

    /*查询是否dump完成，完成后取出数据 需要在dpp_dtb.c中补充查询一个元素ack是否成功函数*/
   if(!dtb_interrupt_status)
   {

        rc = dpp_dtb_tab_up_success_status_check(dev, queue_id, queue_element_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_tab_up_success_status_check");
 
        /*取出数据*/
        rc = dpp_dtb_tab_up_data_get(dev, queue_id, queue_element_id, data_len, p_dump_data);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_tab_up_data_get");
   }else
   {
        rc = dpp_dtb_tab_up_success_status_check(dev, queue_id, queue_element_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_tab_up_success_status_check");

        /*取出数据*/
        rc = dpp_dtb_tab_up_data_get(dev, queue_id, queue_element_id, data_len, p_dump_data);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_tab_up_data_get");
        /*清中断*/
        dpp_dtb_finish_interrupt_event_state_clr(dev, queue_id);
   }

    return DPP_OK;
}


/** smmu0 dump 只写一个dump描述符的接口
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
DPP_STATUS dpp_dtb_se_smmu0_dma_dump(DPP_DEV_T *dev, 
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

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_LOWER(depth, 1);

    /*获取队列中可用的元素编号*/
    rc = dpp_dtb_tab_up_free_item_get(dev, queue_id, &queue_item_index);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_tab_up_free_item_get");

    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_PRINT("dump smmu0:queue %d,item_index: %d\n",queue_id, queue_item_index);
    }

    *element_id = queue_item_index;//保存获取的item_index
    
    /*获取地址*/
    rc = dpp_dtb_tab_up_item_addr_get(dev, queue_id, queue_item_index, &dump_dst_phy_haddr, &dump_dst_phy_laddr);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_tab_up_item_addr_get");
    
    rc = dpp_dtb_smmu0_dump_info_write(dev,
                                       base_addr,
                                       depth,
                                       dump_dst_phy_haddr,
                                       dump_dst_phy_laddr,
                                       (ZXIC_UINT32 *)form_buff);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_smmu0_dump_info_write");
    
    /*组装下表命令格式*/

    data_len = depth * 128 / 32;
    desc_len = DTB_LEN_POS_SETP / 4;

    rc = dpp_dtb_write_dump_desc_info(dev, queue_id, queue_item_index, (ZXIC_UINT32 *)form_buff, data_len, desc_len, p_data);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_write_dump_desc_info");

    return DPP_OK;
}

/** dtb dump DDR表项内容 返回内容以512bit为单位
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   base_addr dump基地址，以512bit为单位
* @param   depth     dump的深度以512bit为单位
* @param   p_data    dump出数据缓存(512bit * depth)
* @param   element_id   返回下表使用的元素id
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_se_smmu1_dma_dump(DPP_DEV_T *dev,  
                                      ZXIC_UINT32 queue_id, 
                                      ZXIC_UINT32 base_addr, 
                                      ZXIC_UINT32 depth,
                                      ZXIC_UINT32 *p_data,
                                      ZXIC_UINT32 *element_id)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 dump_dst_phy_haddr = 0;
    ZXIC_UINT32 dump_dst_phy_laddr = 0;

    ZXIC_UINT32 queue_item_index = 0;
    ZXIC_UINT32 data_len = 0;
    ZXIC_UINT32 desc_len = 0;

    ZXIC_UINT8 form_buff[DTB_TABLE_CMD_SIZE_BIT / 8] = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_LOWER(depth, 1);

    /*获取队列元素*/
    rc = dpp_dtb_tab_up_free_item_get(dev, queue_id, &queue_item_index);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_tab_up_free_item_get");

    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_PRINT("dump smmu1:queue %d,item_index: %d\n",queue_id, queue_item_index);
    }

    *element_id = queue_item_index;//保存获取的item_index
    
    /*获取dump dma phy地址*/
    rc = dpp_dtb_tab_up_item_addr_get(dev, queue_id, queue_item_index, &dump_dst_phy_haddr, &dump_dst_phy_laddr);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_tab_up_item_addr_get");

    rc = dpp_dtb_smmu1_dump_info_write(dev,
                                       base_addr,
                                       depth,
                                       dump_dst_phy_haddr,
                                       dump_dst_phy_laddr,
                                       (ZXIC_UINT32 *)form_buff);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_smmu1_dump_info_write"); 

    /*组装下表命令格式*/
    data_len = depth * 512 / 32;//要提取的数据长度，以32bit为单位
    desc_len = DTB_LEN_POS_SETP / 4;//描述符长度，以32bit为单位

    rc = dpp_dtb_write_dump_desc_info(dev, queue_id, queue_item_index, (ZXIC_UINT32 *)form_buff, data_len, desc_len, p_data);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_write_dump_desc_info");

    return DPP_OK;
}

/** dtb dump zcam表项内容
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   addr      zcam内部ram地址，包括掩码
* @param   tb_width  dump数据的宽度
* @param   depth     dump的深度以tb_width为单位
* @param   p_data    dump出数据缓存(tb_width * depth)
* @param   element_id   返回下表使用的元素id
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_se_zcam_dma_dump(DPP_DEV_T *dev, 
                                      ZXIC_UINT32 queue_id, 
                                      ZXIC_UINT32 addr, 
                                      ZXIC_UINT32 tb_width,
                                      ZXIC_UINT32 depth,
                                      ZXIC_UINT32 *p_data,
                                      ZXIC_UINT32 *element_id)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 dump_dst_phy_haddr = 0;
    ZXIC_UINT32 dump_dst_phy_laddr = 0;

    ZXIC_UINT32 queue_item_index = 0;
    ZXIC_UINT32 data_len = 0;
    ZXIC_UINT32 desc_len = 0;
    ZXIC_UINT32 tb_width_len = 0;

    ZXIC_UINT8 form_buff[DTB_TABLE_CMD_SIZE_BIT / 8] = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), tb_width, DTB_DUMP_ZCAM_128b, DTB_DUMP_ZCAM_RSV - 1);
    ZXIC_COMM_CHECK_INDEX_LOWER(depth, 1);

    /*获取队列元素*/
    rc = dpp_dtb_tab_up_free_item_get(dev, queue_id, &queue_item_index);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_tab_up_free_item_get");

    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_PRINT("dump smmu0:queue %d,item_index: %d\n",queue_id, queue_item_index);
    }

    *element_id = queue_item_index;//保存获取的item_index
    
    /*获取dump dma phy地址*/
    rc = dpp_dtb_tab_up_item_addr_get(dev, queue_id, queue_item_index, &dump_dst_phy_haddr, &dump_dst_phy_laddr);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_tab_up_item_addr_get");

    rc = dpp_dtb_zcam_dump_info_write(dev,
                                      addr,
                                      tb_width,
                                      depth,
                                      dump_dst_phy_haddr,
                                      dump_dst_phy_laddr,
                                      (ZXIC_UINT32 *)form_buff);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_zcam_dump_info_write");

    /*组装下表命令格式*/
    tb_width_len = DTB_LEN_POS_SETP << tb_width;
    data_len = depth * tb_width_len / 4;//要提取的数据长度，以32bit为单位
    desc_len = DTB_LEN_POS_SETP / 4;//描述符长度，以32bit为单位

    rc = dpp_dtb_write_dump_desc_info(dev, queue_id, queue_item_index, (ZXIC_UINT32 *)form_buff, data_len, desc_len, p_data);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_write_dump_desc_info");

    return DPP_OK;

}

#endif

#if ZXIC_REAL("DTB GET")
/***********************************************************/
/** 配置数据获取模式
* @param   srh_mode   0:软件获取  1:硬件获取
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
ZXIC_VOID dpp_dtb_srh_mode_set(ZXIC_UINT32 srh_mode)
{
    g_dtb_srh_mode = srh_mode;
}

/***********************************************************/
/** 获取查找方式
* @param   
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
ZXIC_UINT32 dpp_dtb_srh_mode_get(ZXIC_VOID)
{
    return g_dtb_srh_mode;
}

/***********************************************************/
/** hash表项查找校验(软件获取数据)
* @param   p_entry         入参：hash表项键值 出参：hash表项结果
* @param   key_by_size     键值大小 
* @param   rst_by_size     返回值大小
* @param   p_item_info     512bit单元数据
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
DPP_STATUS dpp_dtb_hash_software_item_check(DPP_HASH_ENTRY *p_entry,
                                        ZXIC_UINT32 key_by_size,
                                        ZXIC_UINT32 rst_by_size,
                                        SE_ITEM_CFG *p_item_info)
{
    ZXIC_UINT8 srh_succ = 0;
    ZXIC_UINT8 temp_key_type = 0;
    ZXIC_UINT8 srh_key_type = 0;
    D_NODE *p_entry_dn = NULL;
    DPP_HASH_RBKEY_INFO *p_rbkey = NULL;

    ZXIC_COMM_CHECK_INDEX(key_by_size, 1, HASH_KEY_MAX);
    ZXIC_COMM_CHECK_POINT(p_entry);
    ZXIC_COMM_CHECK_POINT(p_entry->p_key);
    ZXIC_COMM_CHECK_POINT(p_entry->p_rst);
    ZXIC_COMM_CHECK_POINT(p_item_info);
    
    srh_key_type = DPP_GET_HASH_KEY_TYPE(p_entry->p_key);
    p_entry_dn = p_item_info->item_list.p_next;
    while (p_entry_dn)
    {
        p_rbkey = (DPP_HASH_RBKEY_INFO *)p_entry_dn->data;
        ZXIC_COMM_CHECK_POINT(p_rbkey);
        ZXIC_COMM_CHECK_POINT(p_rbkey->key);
        ZXIC_COMM_CHECK_POINT(p_rbkey->rst);

        ZXIC_COMM_ASSERT(p_rbkey->p_item_info == p_item_info);

        temp_key_type = DPP_GET_HASH_KEY_TYPE(p_rbkey->key);

        if (DPP_GET_HASH_KEY_VALID(p_rbkey->key) && (srh_key_type == temp_key_type))
        {
            if (0 == ZXIC_COMM_MEMCMP(p_entry->p_key, p_rbkey->key, key_by_size))
            {
                srh_succ = 1;
                break;
            }
        }

        p_entry_dn = p_entry_dn->next;
    }

    if (NULL == p_rbkey)
    {
        return ZXIC_PAR_CHK_POINT_NULL;
    }

    if(!srh_succ)
    {
        ZXIC_COMM_TRACE_DEBUG("dpp_dtb_hash_software_item_check fail!\n");
        return DPP_HASH_RC_MATCH_ITEM_FAIL;
    }

    /* copy result */
    ZXIC_COMM_MEMCPY(p_entry->p_rst, p_rbkey->rst, (rst_by_size > HASH_RST_MAX) ? HASH_RST_MAX : rst_by_size);

    return DPP_OK;
}

/** hash表项解析，在512bit存储单元里查找并比对
* @param   item_type        
* @param   key_by_size     键值大小 
* @param   p_entry         入参：hash表项键值 出参：hash表项结果
* @param   p_item_data    查找键值信息
* @param   p_data_offset    查找键值在512bit里的偏移位置,单位为字节
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
ZXIC_UINT32 dpp_dtb_hash_data_parse(ZXIC_UINT32 item_type, 
                                   ZXIC_UINT32 key_by_size,
                                   DPP_HASH_ENTRY *p_entry,
                                   ZXIC_UINT8  *p_item_data,
                                   ZXIC_UINT8  *p_data_offset)
{
    ZXIC_UINT32 data_offset = 0;
    ZXIC_UINT8 temp_key_valid = 0;
    ZXIC_UINT8 temp_key_type = 0;
    ZXIC_UINT32 temp_entry_size = 0;
    ZXIC_UINT8 srh_key_type = 0;
    ZXIC_UINT32 srh_entry_size = 0;
    ZXIC_UINT32 rst_by_size = 0;
    ZXIC_UINT8 srh_succ = 0;
    ZXIC_UINT32 item_width = SE_ITEM_WIDTH_MAX;
    ZXIC_UINT8 *p_srh_key = NULL;
    ZXIC_UINT8 *p_temp_key = NULL;
    ZXIC_UINT32 i = 0;
    
    ZXIC_COMM_CHECK_POINT(p_item_data);
    ZXIC_COMM_CHECK_POINT(p_entry);
    ZXIC_COMM_CHECK_POINT(p_entry->p_key);
    ZXIC_COMM_CHECK_POINT(p_data_offset);
    ZXIC_COMM_CHECK_INDEX_UPPER(key_by_size, HASH_KEY_MAX);

    if (ITEM_DDR_256 == item_type)
    {
        item_width = item_width / 2;
    }

    p_temp_key = p_item_data;
    p_srh_key = p_entry->p_key;
    srh_key_type = DPP_GET_HASH_KEY_TYPE(p_srh_key);
    srh_entry_size = DPP_GET_HASH_ENTRY_SIZE(srh_key_type);

    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_PRINT("srh_key:0x");

        for(i = 0; i < key_by_size; i++)
        {
            ZXIC_COMM_PRINT("%02x ", p_srh_key[i]);
        }
        ZXIC_COMM_PRINT("\n");
    }

    while (data_offset < item_width)
    {
        temp_key_valid = DPP_GET_HASH_KEY_VALID(p_temp_key);
        temp_key_type = DPP_GET_HASH_KEY_TYPE(p_temp_key);

        if (temp_key_valid && (srh_key_type == temp_key_type))
        {
            if (0 == ZXIC_COMM_MEMCMP(p_srh_key, p_temp_key, key_by_size))
            {
                ZXIC_COMM_TRACE_DEBUG("Hash search hardware successfully. \n");
                srh_succ = 1;
                break;
            }

            data_offset += srh_entry_size;
        }
        else if (temp_key_valid && (srh_key_type != temp_key_type))
        {
            temp_entry_size = DPP_GET_HASH_ENTRY_SIZE(temp_key_type);
            ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(data_offset, temp_entry_size);
            if(temp_entry_size == 0)
            {
                ZXIC_COMM_TRACE_ERROR("key type %d srh entry size is zero\n", temp_key_type);
                return DPP_ERR;
            }
            data_offset += temp_entry_size;
        }
        else
        {
            data_offset += 16;/* 偏移最小的步长128bit */
        }

        p_temp_key = p_item_data;
        p_temp_key += data_offset;
    }

    if(!srh_succ)
    {
        ZXIC_COMM_TRACE_DEBUG("Hash search hardware fail. \n");
        return DPP_HASH_RC_MATCH_ITEM_FAIL;
    }

    /* copy result */
    rst_by_size = srh_entry_size - key_by_size;
    ZXIC_COMM_MEMCPY(p_entry->p_rst, p_temp_key + key_by_size, (rst_by_size > HASH_RST_MAX) ? HASH_RST_MAX : rst_by_size);
    *p_data_offset = data_offset;

    return DPP_OK;
}

/***********************************************************/
/** 查找存储在DDR空间的hash表项
* @param   dev_id       设备号，支持多芯片 
* @param   queue_id     队列id 
* @param   p_hash_entry_cfg hash表项配置信息
* @param   p_hash_entry    查找键值信息
* @param   p_srh_succ      出参，查找是否成功
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
ZXIC_UINT32 dpp_dtb_hash_ddr_get(DPP_DEV_T *dev, 
                                 ZXIC_UINT32 queue_id,
                                 HASH_ENTRY_CFG  *p_hash_entry_cfg,
                                 DPP_HASH_ENTRY  *p_hash_entry,
                                 ZXIC_UINT8 *p_srh_succ)
{
    DPP_STATUS           rc           = DPP_OK;   
    ZXIC_UINT32          item_idx     = 0xFFFFFFFF;/* -1 */
    ZXIC_UINT32          item_type    = 0;
    ZXIC_UINT32          crc_value    = 0;
    ZXIC_UINT32          index        = 0;
    ZXIC_UINT32          index_offset = 0;
    ZXIC_UINT32          hw_addr      = 0;  /*单位512bit*/
    ZXIC_UINT32          base_addr    = 0;  /*单位2k*256bit*/ 
    ZXIC_UINT32          ecc_en       = 0;
    ZXIC_UINT32          element_id   = 0;
    ZXIC_UINT32          byte_len     = 0;
    ZXIC_UINT8           byte_offset  = 0;
    ZXIC_UINT8 temp_key[HASH_KEY_MAX] = {0};
    HASH_DDR_CFG         *p_ddr_cfg   = NULL;
    DPP_HASH_CFG         *p_hash_cfg = NULL;
    ZXIC_UINT32          rd_buff[DPP_DIR_TBL_BUF_MAX_NUM]   = {0};
    ZXIC_UINT8           temp_data[DPP_DIR_TBL_BUF_MAX_NUM*4] = {0};
    SE_ITEM_CFG *p_item = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev),DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_POINT(p_hash_entry_cfg);
    ZXIC_COMM_CHECK_POINT(p_hash_entry);
    ZXIC_COMM_CHECK_POINT(p_srh_succ);

    ZXIC_COMM_MEMSET((ZXIC_UINT8 *)rd_buff,0x0,sizeof(rd_buff));
    ZXIC_COMM_MEMSET(temp_data,0x0,sizeof(temp_data));

    rc = dpp_se_smmu1_hash_tbl_soft_cfg_get(dev,
                                            p_hash_entry_cfg->fun_id,
                                            p_hash_entry_cfg->bulk_id,
                                            &ecc_en,
                                            &base_addr);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_se_smmu1_hash_tbl_soft_cfg_get");

    p_hash_cfg = p_hash_entry_cfg->p_hash_cfg;
    ZXIC_COMM_CHECK_POINT(p_hash_cfg);
    ZXIC_COMM_CHECK_INDEX_UPPER(p_hash_entry_cfg->bulk_id,HASH_BULK_ID_MAX);
    p_ddr_cfg = p_hash_cfg->p_bulk_ddr_info[p_hash_entry_cfg->bulk_id];
    ZXIC_COMM_CHECK_POINT(p_ddr_cfg);

    ZXIC_COMM_CHECK_INDEX_UPPER(p_hash_entry_cfg->key_by_size, HASH_KEY_MAX);
    rc = dpp_hash_set_crc_key(dev,p_hash_entry_cfg,p_hash_entry,temp_key);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev),rc,"hash_set_crc_key");

    crc_value = p_hash_cfg->p_hash32_fun(temp_key, p_hash_entry_cfg->key_by_size, p_ddr_cfg->hash_ddr_arg);
    item_idx = crc_value % p_ddr_cfg->item_num;
    index = p_ddr_cfg->hw_baddr + item_idx;
    if (DDR_WIDTH_512b == p_ddr_cfg->width_mode)
    {
        item_type = ITEM_DDR_512;
        hw_addr = (base_addr<<10) + index;
        index_offset = 0;
        byte_len = 512/8;
    }
    else
    {
        item_type = ITEM_DDR_256;
        hw_addr = (base_addr<<10) + (index>>1);
        index_offset = index & 0x1;
        byte_len = 256/8;
    }

    ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), 
               "Hash search in ITEM_DDR_%s, CRC32 index is: 0x%x.\n", ((item_type == ITEM_DDR_256) ? "256" : "512"), 
                item_idx);
    if(dpp_dtb_srh_mode_get())
    {
        rc = dpp_dtb_se_smmu1_dma_dump(dev, 
                                   queue_id, 
                                   hw_addr, 
                                   1,
                                   rd_buff,
                                   &element_id);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc,"dpp_dtb_se_smmu1_dma_dump");

        zxic_comm_swap((ZXIC_UINT8 *)(rd_buff+index_offset*8), byte_len);
        ZXIC_COMM_MEMCPY(temp_data, rd_buff+index_offset*8, byte_len);

        rc = dpp_dtb_hash_data_parse(item_type,p_hash_entry_cfg->key_by_size,p_hash_entry,temp_data,&byte_offset);
        ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_hash_data_parse");
   
        ZXIC_COMM_TRACE_DEBUG("Hash search hardware succ in ddr. \n"); 
    }
    else
    {
        ZXIC_COMM_CHECK_INDEX_UPPER(item_idx, p_ddr_cfg->item_num - 1);
        p_item = p_ddr_cfg->p_item_array[item_idx];
        ZXIC_COMM_CHECK_POINT_NO_ASSERT(p_item);
        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev), p_ddr_cfg->hw_baddr, item_idx);
        p_item->hw_addr = p_ddr_cfg->hw_baddr+item_idx;
        p_item->item_type = item_type;
        p_item->item_index = item_idx;
    
        rc = dpp_dtb_hash_software_item_check(p_hash_entry,
                                    p_hash_entry_cfg->key_by_size,
                                    p_hash_entry_cfg->rst_by_size,
                                    p_item);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_hash_software_item_check");
    }

    *p_srh_succ = 1;
    p_hash_cfg->hash_stat.search_ok++;

    return DPP_OK;
}

/***********************************************************/
/** 查找存储在ZCAM空间的hash表项
* @param   dev_id       设备号，支持多芯片 
* @param   queue_id     队列id 
* @param   p_hash_entry_cfg hash表项配置信息
* @param   p_hash_entry    查找键值信息
* @param   p_srh_succ      出参，查找是否成功
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
ZXIC_UINT32 dpp_dtb_hash_zcam_get(DPP_DEV_T *dev, 
                                 ZXIC_UINT32 queue_id,
                                 HASH_ENTRY_CFG  *p_hash_entry_cfg,
                                 DPP_HASH_ENTRY  *p_hash_entry,
                                 ZXIC_UINT32 srh_mode,
                                 ZXIC_UINT8 *p_srh_succ)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev),DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_POINT(p_hash_entry_cfg);
    ZXIC_COMM_CHECK_POINT(p_srh_succ);
    ZXIC_COMM_CHECK_POINT(p_hash_entry);


    if(HASH_SRH_MODE_HDW==srh_mode)
    {
        rc = dpp_dtb_hash_zcam_get_hardware(dev,queue_id,p_hash_entry_cfg,p_hash_entry,p_srh_succ);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_hash_zcam_get_hardware");
    }
    else
    {
        rc = dpp_dtb_hash_get_software(dev,p_hash_entry_cfg,p_hash_entry,p_srh_succ);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_hash_get_software"); 
    }

    return DPP_OK;
}

/***********************************************************/
/** 硬件查找存储在ZCAM空间的hash表项
* @param   dev_id       设备号，支持多芯片 
* @param   queue_id     队列id 
* @param   p_hash_entry_cfg hash表项配置信息
* @param   p_hash_entry    查找键值信息(查找成功，填充rst信息)
* @param   p_srh_succ      出参，查找是否成功
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
DPP_STATUS dpp_dtb_hash_zcam_get_hardware(DPP_DEV_T *dev, 
                                 ZXIC_UINT32 queue_id,
                                 HASH_ENTRY_CFG  *p_hash_entry_cfg,
                                 DPP_HASH_ENTRY  *p_hash_entry,
                                 ZXIC_UINT8 *p_srh_succ)
{
    DPP_STATUS rc = DPP_OK;
    DPP_HASH_CFG *p_hash_cfg = NULL;
    SE_ZCELL_CFG *p_zcell = NULL;
    SE_ZBLK_CFG *p_zblk = NULL;

    ZXIC_UINT32 zblk_idx = 0;
    ZXIC_UINT32 pre_zblk_idx = 0xFFFFFFFF;/* -1; */
    ZXIC_UINT16 crc16_value = 0;
    ZXIC_UINT32 zcell_id = 0;
    ZXIC_UINT32 item_idx = 0;
    ZXIC_UINT32 element_id = 0;
    ZXIC_UINT32 byte_offset = 0;
    ZXIC_UINT32 addr = 0;
    ZXIC_UINT32 i    = 0;
    ZXIC_UINT8 srh_succ     = 0;
    ZXIC_UINT8 temp_key[HASH_KEY_MAX] = {0};
    ZXIC_UINT8  rd_buff[SE_ITEM_WIDTH_MAX]   = {0};

    D_NODE *p_zblk_dn = NULL;
    D_NODE *p_zcell_dn = NULL;
    DPP_SE_CFG *p_se_cfg = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev),DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_POINT(p_hash_entry_cfg);
    ZXIC_COMM_CHECK_POINT(p_srh_succ);
    ZXIC_COMM_CHECK_POINT(p_hash_entry);
    ZXIC_COMM_CHECK_POINT(p_hash_entry->p_key);
    ZXIC_COMM_CHECK_POINT(p_hash_entry->p_rst);

    ZXIC_COMM_MEMSET_S(rd_buff, sizeof(rd_buff), 0, sizeof(rd_buff));
    
    p_hash_cfg = p_hash_entry_cfg->p_hash_cfg;
    ZXIC_COMM_CHECK_POINT(p_hash_cfg);
    
    /* 取出se配置 */
    p_se_cfg = p_hash_entry_cfg->p_se_cfg;
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_se_cfg);
    
    rc = dpp_hash_set_crc_key(dev,p_hash_entry_cfg,p_hash_entry,temp_key);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev),rc,"hash_set_crc_key");

    /*查找所有的zcell*/
    p_zcell_dn = p_hash_cfg->hash_shareram.zcell_free_list.p_next;
    while (p_zcell_dn)
    {
        p_zcell = (SE_ZCELL_CFG *)p_zcell_dn->data;
        zblk_idx = GET_ZBLK_IDX(p_zcell->zcell_idx);
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), zblk_idx, 0, SE_ZBLK_NUM - 1);
        p_zblk = &(p_se_cfg->zblk_info[zblk_idx]);

        if (zblk_idx != pre_zblk_idx)
        {
            pre_zblk_idx = zblk_idx;
            crc16_value = p_hash_cfg->p_hash16_fun(temp_key, p_hash_entry_cfg->key_by_size, p_zblk->hash_arg);
        }

        zcell_id = GET_ZCELL_IDX(p_zcell->zcell_idx);
        item_idx = GET_ZCELL_CRC_VAL(zcell_id, crc16_value);
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), item_idx, 0, SE_RAM_DEPTH - 1);
        addr = ZBLK_ITEM_ADDR_CALC(p_zcell->zcell_idx, item_idx);
        rc =  dpp_dtb_se_zcam_dma_dump(dev, 
                                       queue_id, 
                                       addr, 
                                       DTB_DUMP_ZCAM_512b, 
                                       1,
                                       (ZXIC_UINT32 *)rd_buff, 
                                       &element_id);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_se_zcam_dma_dump");
        zxic_comm_swap(rd_buff,sizeof(rd_buff));
        
        rc = dpp_dtb_hash_data_parse(ITEM_RAM,p_hash_entry_cfg->key_by_size,p_hash_entry,rd_buff,(ZXIC_UINT8 *)(&byte_offset));
        if(DPP_OK == rc)
        {
            ZXIC_COMM_TRACE_DEBUG("Hash search hardware succ in zcell. \n");
            srh_succ = 1;
            p_hash_cfg->hash_stat.search_ok++;
            break;
        }
        
        p_zcell_dn = p_zcell_dn->next;
    }

    /*zcell查找失败，则查找所有的zreg*/
    if(0==srh_succ)
    {
        p_zblk_dn = p_hash_cfg->hash_shareram.zblk_list.p_next;
        while (p_zblk_dn)
        {
            p_zblk = (SE_ZBLK_CFG *)p_zblk_dn->data;
            zblk_idx = p_zblk->zblk_idx;

            for (i = 0; i < SE_ZREG_NUM; i++)
            {
                item_idx = i;
                addr = ZBLK_HASH_LIST_REG_ADDR_CALC(zblk_idx, item_idx);
                rc =  dpp_dtb_se_zcam_dma_dump(dev, 
                                       queue_id, 
                                       addr, 
                                       DTB_DUMP_ZCAM_512b, 
                                       1,
                                       (ZXIC_UINT32 *)rd_buff, 
                                       &element_id);
                ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_se_zcam_dma_dump");
                zxic_comm_swap(rd_buff,sizeof(rd_buff));

                rc = dpp_dtb_hash_data_parse(ITEM_RAM,p_hash_entry_cfg->key_by_size,p_hash_entry,rd_buff,(ZXIC_UINT8 *)(&byte_offset));
                if(DPP_OK == rc)
                {
                    ZXIC_COMM_TRACE_DEBUG("Hash search hardware succ in zreg. \n");
                    srh_succ = 1;
                    p_hash_cfg->hash_stat.search_ok++;
                    break;
                }
            }
            p_zblk_dn = p_zblk_dn->next;
        }
    }

    *p_srh_succ = srh_succ;
    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_PRINT("dump data:\n");
        for(i = 0; i < SE_ITEM_WIDTH_MAX; i++)
        {
            ZXIC_COMM_PRINT("%02x ",rd_buff[i]);
            if((i+1)%16==0)
            {
                ZXIC_COMM_PRINT("\n");
            }
        }
        ZXIC_COMM_PRINT("\n");
    }

    return DPP_OK;
}

/***********************************************************/
/** 软件查找存储在ZCAM空间的hash表项
* @param   dev_id       设备号，支持多芯片 
* @param   p_hash_entry_cfg hash表项配置信息
* @param   p_hash_entry    查找键值信息(查找成功后，填充rst)
* @param   p_srh_succ      出参，查找是否成功
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
DPP_STATUS dpp_dtb_hash_get_software(DPP_DEV_T *dev, 
                                 HASH_ENTRY_CFG  *p_hash_entry_cfg,
                                 DPP_HASH_ENTRY  *p_hash_entry,
                                 ZXIC_UINT8 *p_srh_succ)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;

    DPP_HASH_RBKEY_INFO srh_rbkey = {0};
    DPP_HASH_RBKEY_INFO *p_rbkey = NULL;
    ZXIC_RB_TN *p_rb_tn_rtn = NULL;
    SE_ITEM_CFG *p_item = NULL;
    DPP_HASH_CFG *p_hash_cfg = NULL;
    ZXIC_MUTEX_T *p_hash_mutex = NULL;
    DPP_SE_CFG *p_se_cfg = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(dev_id,DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_POINT(p_hash_entry_cfg);
    ZXIC_COMM_CHECK_POINT(p_srh_succ);
    ZXIC_COMM_CHECK_POINT(p_hash_entry);
    ZXIC_COMM_CHECK_POINT(p_hash_entry->p_key);
    ZXIC_COMM_CHECK_POINT(p_hash_entry->p_rst);

    ZXIC_COMM_MEMSET_S(&srh_rbkey, sizeof(DPP_HASH_RBKEY_INFO), 0, sizeof(DPP_HASH_RBKEY_INFO));
    ZXIC_COMM_CHECK_INDEX_UPPER(p_hash_entry_cfg->key_by_size, HASH_KEY_MAX);
    ZXIC_COMM_MEMCPY(srh_rbkey.key,p_hash_entry->p_key,p_hash_entry_cfg->key_by_size);

    p_hash_cfg = p_hash_entry_cfg->p_hash_cfg;
    ZXIC_COMM_CHECK_POINT(p_hash_cfg);

    rc = dpp_se_cfg_get(dev, &p_se_cfg);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_cfg_get");
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_se_cfg);


    rc = dpp_dev_hash_opr_mutex_get(dev, p_hash_cfg->fun_id, &p_hash_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_dev_opr_mutex_get"); 
    rc = zxic_comm_mutex_lock(p_hash_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_mutex_lock"); 

    rc = zxic_comm_rb_search(&p_hash_cfg->hash_rb, (ZXIC_VOID *)&srh_rbkey, (ZXIC_VOID *)(&p_rb_tn_rtn));
    if(ZXIC_RBT_RC_SRHFAIL==rc)
    {
        ZXIC_COMM_TRACE_DEBUG("zxic_comm_rb_search fail. \n");

        rc = zxic_comm_mutex_unlock(p_hash_mutex);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_mutex_unlock");

        return DPP_OK;
    }
    
    ZXIC_COMM_CHECK_DEV_POINT_UNLOCK(dev_id,p_rb_tn_rtn,p_hash_mutex);
    p_rbkey = p_rb_tn_rtn->p_key;
    ZXIC_COMM_CHECK_DEV_POINT_UNLOCK(dev_id, p_rbkey,p_hash_mutex);
    p_item = p_rbkey->p_item_info;
    ZXIC_COMM_CHECK_DEV_POINT_UNLOCK(dev_id, p_item,p_hash_mutex);

    rc = dpp_dtb_hash_software_item_check(p_hash_entry,
                                        p_hash_entry_cfg->key_by_size,
                                        p_hash_entry_cfg->rst_by_size,
                                        p_item);
    if(DPP_OK == rc)
    {
        ZXIC_COMM_TRACE_DEBUG("Hash search software succ. \n");
        *p_srh_succ = 1;
        p_hash_cfg->hash_stat.search_ok++;
    }

    rc = zxic_comm_mutex_unlock(p_hash_mutex);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_mutex_unlock");

    return DPP_OK;
}


/** dtb根据etcam key计算dump出数据占用的数据长度
* @param   dev_id                 设备号
* @param   etcam_key_mode         etcam条目位宽，参照DPP_ETCAM_ENTRY_MODE_E定义
* @param   p_etcam_dump_len       dtb dump出etcam的数据长度(单位:字节)
* @param   p_etcam_dump_inerval   dump出etcam数据的间隔数据长度(单位：字节)
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dtb_etcam_dump_data_len(DPP_DEV_T *dev, 
                                   ZXIC_UINT32 etcam_key_mode, 
                                   ZXIC_UINT32 *p_etcam_dump_len, 
                                   ZXIC_UINT32* p_etcam_dump_inerval)
{
    ZXIC_UINT32 dump_data_len = 0;
    ZXIC_UINT8 etcam_dump_inerval = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_etcam_dump_len);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_etcam_dump_inerval);

    if(DPP_ETCAM_KEY_640b == etcam_key_mode)
    {
        dump_data_len = 5 * DTB_LEN_POS_SETP;
        etcam_dump_inerval = 0;
    }
    else if(DPP_ETCAM_KEY_320b == etcam_key_mode)
    {
        dump_data_len = 3 * DTB_LEN_POS_SETP;
        etcam_dump_inerval = 8;
    }
    else if(DPP_ETCAM_KEY_160b == etcam_key_mode)
    {
        dump_data_len = 2 * DTB_LEN_POS_SETP;
        etcam_dump_inerval = 12;
    }
    else if(DPP_ETCAM_KEY_80b == etcam_key_mode)
    {
        dump_data_len = 1 * DTB_LEN_POS_SETP;
        etcam_dump_inerval = 6;
    }

    *p_etcam_dump_len = dump_data_len;
    *p_etcam_dump_inerval = etcam_dump_inerval;

    return DPP_OK;
}

/** dtb 从dump数据中提取到xy数据
* @param   dev_id                 设备号
* @param   p_data                 dump出的etcam data 指针
* @param   p_mask                 dump出的etcam mask 指针
* @param   p_etcam_dump_len       dtb dump出etcam的数据长度(单位:字节)
* @param   p_etcam_dump_inerval   dump出etcam数据的间隔数据长度(单位：字节)
* @param   p_entry_xy             保存etcam xy数据（已分配内存）
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_get_etcam_xy_from_dump_data(DPP_DEV_T *dev, 
                                               ZXIC_UINT8 *p_data, 
                                               ZXIC_UINT8 *p_mask, 
                                               ZXIC_UINT32 etcam_dump_len,
                                               ZXIC_UINT32 etcam_dump_inerval, 
                                               DPP_ETCAM_ENTRY_T *p_entry_xy)
{
    ZXIC_UINT8 *p_entry_data = NULL;
    ZXIC_UINT8 *p_entry_mask = NULL;

    //先对数据进行翻转再进行数据的截取，最后进行数据的复制
    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_mask);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_entry_xy);

    zxic_comm_swap(p_data, etcam_dump_len);
    zxic_comm_swap(p_mask, etcam_dump_len);

    p_entry_data = p_data + etcam_dump_inerval;
    p_entry_mask = p_mask + etcam_dump_inerval;

    ZXIC_COMM_MEMCPY(p_entry_xy->p_data, p_entry_data, DPP_ETCAM_ENTRY_SIZE_GET(p_entry_xy->mode));
    ZXIC_COMM_MEMCPY(p_entry_xy->p_mask, p_entry_mask, DPP_ETCAM_ENTRY_SIZE_GET(p_entry_xy->mode));

    return DPP_OK;
}

/** dtb dump etcam直接表表项内容 级联eram支持64bit/128bit
* @param   dev_id             设备号
* @param   p_in_data          长度为640bit的输入数据
* @param   rd_mode            读模式
* @param   p_out_data         按照读模式读出的数据
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_etcam_ind_data_get(DPP_DEV_T *dev, ZXIC_UINT8 *p_in_data, ZXIC_UINT32 rd_mode, ZXIC_UINT8 *p_out_data)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 i = 0;
    ZXIC_UINT8 *p_temp = NULL;
    ZXIC_UINT32 offset = 0;
    ZXIC_UINT8  buff[DPP_ETCAM_WIDTH_MAX / 8] = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_in_data);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_out_data);
    
    p_temp = p_out_data;
    ZXIC_COMM_MEMCPY(buff, p_in_data, DPP_ETCAM_WIDTH_MAX / 8);

    zxic_comm_swap(buff, DPP_ETCAM_WIDTH_MAX / 8);

    for (i = 0; i < DPP_ETCAM_RAM_NUM; i++)
    {
        offset = i * (DPP_ETCAM_WIDTH_MIN / 8);

        if ((rd_mode >> (DPP_ETCAM_RAM_NUM - 1 - i)) & 0x1)
        {
            ZXIC_COMM_MEMCPY(p_temp, buff + offset, DPP_ETCAM_WIDTH_MIN / 8);
            p_temp += DPP_ETCAM_WIDTH_MIN / 8;
        }
    }

    return rc;
}

/** dtb dump etcam表项内容
* @param   dev_id           设备号
* @param   queue_id         队列号
* @param   block_idx        block的索引，范围0-7
* @param   addr             单个block中的RAM地址，范围0~511
* @param   rd_mode          读block RAM行的位图，共8bit，每比特表示一个RAM中的80bit
* @param   opr_type         读取的数据类型，0:data/mask格式，1：xy格式     
* @param   as_en            级联eram使能
* @param   as_eram_baddr    级联eram基地址
* @param   as_eram_index    级联eram 条目index
* @param   as_rsp_mode      级联返回位宽，参见DPP_DIAG_ERAM_MODE_E
* @param   p_entry          读取的etcam数据
* @param   p_as_rslt        读取的级联数据(分配128bit存储空间)
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_etcam_entry_get(DPP_DEV_T *dev, 
                                   ZXIC_UINT32 queue_id,
                                   ZXIC_UINT32 block_idx, 
                                   ZXIC_UINT32 addr,
                                   ZXIC_UINT32 rd_mode,
                                   ZXIC_UINT32 opr_type, 
                                   ZXIC_UINT32 as_en,
                                   ZXIC_UINT32 as_eram_baddr,
                                   ZXIC_UINT32 as_eram_index,
                                   ZXIC_UINT32 as_rsp_mode,//128:3   64:2
                                   DPP_ETCAM_ENTRY_T *p_entry,
                                   ZXIC_UINT8    *p_as_rslt)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 etcam_key_mode = 0;

    ZXIC_UINT8 temp_data[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 temp_mask[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    DPP_ETCAM_ENTRY_T entry_xy = {0};

    ZXIC_UINT32 etcam_data_dst_phy_haddr = 0;
    ZXIC_UINT32 etcam_data_dst_phy_laddr = 0;
    ZXIC_UINT32 etcam_mask_dst_phy_haddr = 0;
    ZXIC_UINT32 etcam_mask_dst_phy_laddr = 0;
    ZXIC_UINT32 as_rst_dst_phy_haddr = 0;
    ZXIC_UINT32 as_rst_dst_phy_laddr = 0;

    ZXIC_UINT32 dump_element_id = 0;
    ZXIC_UINT32 etcam_dump_one_data_len = 0;
    ZXIC_UINT32 etcam_dump_inerval = 0;
    ZXIC_UINT32 dtb_desc_addr_offset = 0;
    ZXIC_UINT32 dump_data_len = 0;
    ZXIC_UINT32 dtb_desc_len = 0;

    ZXIC_UINT32 eram_dump_base_addr = 0;
    ZXIC_UINT32 row_index = 0;
    ZXIC_UINT32 col_index = 0;

    ZXIC_UINT8 *p_data = NULL;
    ZXIC_UINT8 *p_mask = NULL;
    ZXIC_UINT8 *p_rst = NULL;
    ZXIC_UINT8  *temp_dump_out_data = NULL;
    ZXIC_UINT8 *dump_info_buff = NULL;
    ETCAM_DUMP_INFO_T etcam_dump_info = {0};
    DPP_DTB_ENTRY_T   dtb_dump_entry = {0};
    ZXIC_UINT8 cmd_buff[DTB_TABLE_CMD_SIZE_BIT / 8] = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), addr, 0, DPP_ETCAM_RAM_DEPTH - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), block_idx, 0, DPP_ETCAM_BLOCK_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), rd_mode, 0, DPP_ETCAM_WR_MASK_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), opr_type, DPP_ETCAM_OPR_DM, DPP_ETCAM_OPR_XY);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_entry);

    dump_info_buff = (ZXIC_UINT8 *) ZXIC_COMM_MALLOC(DPP_DTB_TABLE_DUMP_INFO_BUFF_SIZE * sizeof(ZXIC_UINT8));
    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dump_info_buff);
    ZXIC_COMM_MEMSET(dump_info_buff, 0, DPP_DTB_TABLE_DUMP_INFO_BUFF_SIZE * sizeof(ZXIC_UINT8));

    dtb_dump_entry.cmd = cmd_buff;

    entry_xy.p_data = temp_data;
    entry_xy.p_mask = temp_mask;

    etcam_key_mode = p_entry->mode;

    etcam_dump_info.block_sel = block_idx;
    etcam_dump_info.addr = addr;
    etcam_dump_info.tb_width = 3 - etcam_key_mode;
    etcam_dump_info.rd_mode = rd_mode;
    etcam_dump_info.tb_depth = 1;

    rc = dpp_dtb_tab_up_free_item_get(dev, queue_id, &dump_element_id);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_tab_up_free_item_get", dump_info_buff);
    ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "table up item queue_element_id is: %d.\n",dump_element_id);

    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_PRINT("dump etcam:queue %d,element id: %d\n",queue_id, dump_element_id);
    }

    rc = dtb_etcam_dump_data_len(dev, etcam_key_mode, &etcam_dump_one_data_len, &etcam_dump_inerval);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dtb_etcam_dump_data_len", dump_info_buff);

    //etcam data dump描述符
    etcam_dump_info.data_or_mask = DPP_ETCAM_DTYPE_DATA;
    rc = dpp_dtb_tab_up_item_offset_addr_get(dev,
                                             queue_id,
                                             dump_element_id,
                                             dump_data_len,
                                             &etcam_data_dst_phy_haddr,
                                             &etcam_data_dst_phy_laddr);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_tab_up_item_offset_addr_get", dump_info_buff);
    rc = dpp_dtb_etcam_dump_entry(dev,
                                  &etcam_dump_info,
                                  etcam_data_dst_phy_haddr,
                                  etcam_data_dst_phy_laddr,
                                  &dtb_dump_entry);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_etcam_dump_entry", dump_info_buff);

    rc = dpp_dtb_data_write(dump_info_buff, dtb_desc_addr_offset, &dtb_dump_entry);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_data_write", dump_info_buff);
    ZXIC_COMM_MEMSET(cmd_buff, 0, DTB_TABLE_CMD_SIZE_BIT / 8);
    dtb_desc_len += 1;
    dtb_desc_addr_offset += DTB_LEN_POS_SETP;
    dump_data_len += etcam_dump_one_data_len;

    //etcam mask dump描述符
    etcam_dump_info.data_or_mask = DPP_ETCAM_DTYPE_MASK;
    rc = dpp_dtb_tab_up_item_offset_addr_get(dev,
                                             queue_id,
                                             dump_element_id,
                                             dump_data_len,
                                             &etcam_mask_dst_phy_haddr,
                                             &etcam_mask_dst_phy_laddr);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_tab_up_item_offset_addr_get", dump_info_buff);
    rc = dpp_dtb_etcam_dump_entry(dev,
                                  &etcam_dump_info,
                                  etcam_mask_dst_phy_haddr,
                                  etcam_mask_dst_phy_laddr,
                                  &dtb_dump_entry);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_etcam_dump_entry", dump_info_buff);
    rc = dpp_dtb_data_write(dump_info_buff, dtb_desc_addr_offset, &dtb_dump_entry);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_data_write", dump_info_buff);
    ZXIC_COMM_MEMSET(cmd_buff, 0, DTB_TABLE_CMD_SIZE_BIT / 8);
    dtb_desc_len += 1;
    dtb_desc_addr_offset += DTB_LEN_POS_SETP;
    dump_data_len += etcam_dump_one_data_len;

    //级联数据dump描述符生成
    if(as_en)
    {
        rc = dtb_eram_index_cal(dev, as_rsp_mode, as_eram_index, &row_index, &col_index);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dtb_eram_index_cal", dump_info_buff);

        eram_dump_base_addr = as_eram_baddr + row_index;
        ZXIC_COMM_TRACE_INFO("eram_dump_base_addr : 0x%x\n",eram_dump_base_addr);
        rc = dpp_dtb_tab_up_item_offset_addr_get(dev,
                                             queue_id,
                                             dump_element_id,
                                             dump_data_len,
                                             &as_rst_dst_phy_haddr,
                                             &as_rst_dst_phy_laddr);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_tab_up_item_offset_addr_get", dump_info_buff);

        rc = dpp_dtb_smmu0_dump_entry(dev, 
                                      eram_dump_base_addr, 
                                      1,
                                      as_rst_dst_phy_haddr,
                                      as_rst_dst_phy_laddr,
                                      &dtb_dump_entry);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_smmu0_dump_entry", dump_info_buff);
        rc = dpp_dtb_data_write(dump_info_buff, dtb_desc_addr_offset, &dtb_dump_entry);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_data_write", dump_info_buff);
        ZXIC_COMM_MEMSET(cmd_buff, 0, DTB_TABLE_CMD_SIZE_BIT / 8);
        dtb_desc_len += 1;
        dtb_desc_addr_offset += DTB_LEN_POS_SETP;
        dump_data_len += DTB_LEN_POS_SETP;
    }

    //申请data空间
    temp_dump_out_data = (ZXIC_UINT8 *) ZXIC_COMM_MALLOC(dump_data_len * sizeof(ZXIC_UINT8));
    ZXIC_COMM_CHECK_POINT_MEMORY_FREE_NO_ASSERT(temp_dump_out_data, dump_info_buff);
    ZXIC_COMM_MEMSET(temp_dump_out_data, 0, dump_data_len * sizeof(ZXIC_UINT8));
    p_data = temp_dump_out_data;
    /*下发dump描述符*/
    rc = dpp_dtb_write_dump_desc_info(dev,
                                      queue_id,
                                      dump_element_id,
                                      (ZXIC_UINT32 *)dump_info_buff,
                                      dump_data_len / 4,
                                      dtb_desc_len * 4,
                                      (ZXIC_UINT32 *)temp_dump_out_data);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE2PTR_NO_ASSERT(rc,"dpp_dtb_write_dump_desc_info", dump_info_buff, temp_dump_out_data);

    //解析数据
    p_data = temp_dump_out_data;
    p_mask = p_data + etcam_dump_one_data_len;

    rc = dpp_dtb_get_etcam_xy_from_dump_data(dev, 
                                             p_data, 
                                             p_mask, 
                                             etcam_dump_one_data_len,
                                             etcam_dump_inerval, 
                                             &entry_xy);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE2PTR_NO_ASSERT(rc, "dpp_dtb_get_etcam_xy_from_dump_data", dump_info_buff, temp_dump_out_data);

    if (opr_type == DPP_ETCAM_OPR_DM)
    {
        /* convert hardware data X/Y to user D/M  */
        rc = dpp_etcam_xy_to_dm(p_entry, &entry_xy, DPP_ETCAM_ENTRY_SIZE_GET(p_entry->mode));
        ZXIC_COMM_CHECK_RC_MEMORY_FREE2PTR_NO_ASSERT(rc, "dpp_etcam_xy_to_dm", dump_info_buff, temp_dump_out_data);
    }
    else
    {
        ZXIC_COMM_MEMCPY(p_entry->p_data, entry_xy.p_data, DPP_ETCAM_ENTRY_SIZE_GET(p_entry->mode));
        ZXIC_COMM_MEMCPY(p_entry->p_mask, entry_xy.p_mask, DPP_ETCAM_ENTRY_SIZE_GET(p_entry->mode));
    }

    if(as_en)
    {
        //解析dump数据
        p_rst = p_mask + etcam_dump_one_data_len;
        ZXIC_COMM_MEMCPY(p_as_rslt, p_rst, (128 / 8));
    }

    ZXIC_COMM_FREE(dump_info_buff);
    ZXIC_COMM_FREE(temp_dump_out_data);

    return rc;
}
#endif

#if ZXIC_REAL("DTB GET INTERFACE")
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
DPP_STATUS dpp_dtb_eram_data_get(DPP_DEV_T *dev, 
                                 ZXIC_UINT32 queue_id, 
                                 ZXIC_UINT32 sdt_no, 
                                 DPP_DTB_ERAM_ENTRY_INFO_T *p_dump_eram_entry)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 rd_mode = 0;
    ZXIC_UINT32 eram_base_addr = 0;
    ZXIC_UINT32 eram_table_depth = 0;
    ZXIC_UINT32 eram_dump_base_addr = 0;
    ZXIC_UINT32 row_index = 0;
    ZXIC_UINT32 col_index = 0;
    ZXIC_UINT32 temp_data[4] = {0};
    ZXIC_UINT32 element_id = 0;

    ZXIC_UINT32 index = p_dump_eram_entry->index;
    ZXIC_UINT32 *p_data = p_dump_eram_entry->p_data;

    DPP_SDTTBL_ERAM_T sdt_eram_info = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_eram_info);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_soft_sdt_tbl_get");
    eram_base_addr = sdt_eram_info.eram_base_addr;
    rd_mode = sdt_eram_info.eram_mode;//0:1bit;2:64bit;3:128,
    eram_table_depth = sdt_eram_info.eram_table_depth;
    ZXIC_COMM_CHECK_DEV_INDEX_LOWER_NO_ASSERT(DEV_ID(dev), eram_table_depth, 1);
    ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(DEV_ID(dev), index, 0, eram_table_depth - 1);

    rc = dtb_eram_index_cal(dev, rd_mode, index, &row_index, &col_index);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dtb_eram_index_cal");

    eram_dump_base_addr = eram_base_addr + row_index;

    rc = dpp_dtb_se_smmu0_dma_dump(dev, 
                                queue_id, 
                                eram_dump_base_addr, 
                                1,
                                temp_data,
                                &element_id);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_se_smmu0_dma_dump");
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
DPP_STATUS dpp_dtb_eram_stat_data_get(DPP_DEV_T *dev, 
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

    rc = dpp_dtb_se_smmu0_dma_dump(dev, 
                                queue_id, 
                                eram_dump_base_addr, 
                                1,
                                temp_data,
                                &element_id);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_se_smmu0_dma_dump");
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

/** dtb dump ddr直接表表项内容
* @param   dev_id       设备号
* @param   queue_id     队列号
* @param   sdt_no       eram表sdt表号
* @param   p_dump_ddr_entry  ddr 数据结构，数据已分配相应内存
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_ddr_data_get(DPP_DEV_T *dev, 
                                 ZXIC_UINT32 queue_id, 
                                 ZXIC_UINT32 sdt_no, 
                                 DPP_DTB_DDR_ENTRY_INFO_T *p_dump_ddr_entry)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 rd_mode = 0;
    ZXIC_UINT32 ddr_base_addr = 0;
    ZXIC_UINT32 ddr_dump_base_addr_512bit = 0;
    ZXIC_UINT32 row_index = 0;
    ZXIC_UINT32 col_index = 0;
    ZXIC_UINT32 rd_buff[DPP_DIR_TBL_BUF_MAX_NUM] = {0};
    ZXIC_UINT32 element_id = 0;

    ZXIC_UINT32 index = p_dump_ddr_entry->index;
    ZXIC_UINT32 *p_data = p_dump_ddr_entry->p_data;

    DPP_SDTTBL_DDR3_T sdt_ddr_info = {0};  /*SDT内容*/

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_dump_ddr_entry);

    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_ddr_info);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_soft_sdt_tbl_get");
    ddr_base_addr = sdt_ddr_info.ddr3_base_addr;
    ZXIC_COMM_CHECK_INDEX_UPPER(ddr_base_addr,DPP_SMMU1_TOTAL_MAX_BADDR);
    rd_mode = sdt_ddr_info.ddr3_rw_len;

    if(SMMU1_DDR_SRH_128b == rd_mode)
    {
        row_index = index >> 2;
        col_index = ((index & 0x3)) << 2;
    }
    else if(SMMU1_DDR_SRH_256b == rd_mode)
    {
        row_index = index >> 1;
        col_index = ((index & 0x1)) << 3;
    }
    else if(SMMU1_DDR_SRH_512b == rd_mode)
    {
        row_index = index;
        col_index = 0;
    }

    ddr_dump_base_addr_512bit = (ddr_base_addr << 10) + row_index;/**转换成512bit为单位的地址*/

    rc = dpp_dtb_se_smmu1_dma_dump(dev, 
                                  queue_id, 
                                  ddr_dump_base_addr_512bit, 
                                  1,
                                  rd_buff,
                                  &element_id);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc,"dpp_dtb_se_smmu1_dma_dump");

    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_PRINT("dump ddr done ,the element is is %d\n",element_id);
    }

    ZXIC_COMM_MEMCPY((ZXIC_UINT8 *)p_data, rd_buff+col_index, DTB_LEN_POS_SETP << rd_mode);

    return rc;
}


/** 根据键值查找hash表
* @param   dev_id       设备号，支持多芯片 
* @param   queue_id     队列id 
* @param   sdt_no       0~255 
* @param   p_dtb_hash_entry    出参，返回描述符信息
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
ZXIC_UINT32 dpp_dtb_hash_data_get(DPP_DEV_T *dev, 
                                ZXIC_UINT32 queue_id,
                                ZXIC_UINT32 sdt_no,
                                DPP_DTB_HASH_ENTRY_INFO_T *p_dtb_hash_entry,
                                ZXIC_UINT32 srh_mode)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT8 srh_succ = 0;
    ZXIC_UINT8 key_valid = 1;
    DPP_HASH_CFG *p_hash_cfg = NULL;
    HASH_ENTRY_CFG  hash_entry_cfg = {0}; 
    DPP_HASH_ENTRY hash_entry = {0};
    ZXIC_UINT8 aucKey[HASH_KEY_MAX] = {0};
    ZXIC_UINT8 aucRst[HASH_RST_MAX] = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_POINT(p_dtb_hash_entry);
    ZXIC_COMM_CHECK_POINT(p_dtb_hash_entry->p_actu_key);
    ZXIC_COMM_CHECK_POINT(p_dtb_hash_entry->p_rst);

    ZXIC_COMM_MEMSET_S(&hash_entry, sizeof(DPP_HASH_ENTRY), 0, sizeof(DPP_HASH_ENTRY));

    //从sdt_no中获取hash配置
    rc = dpp_hash_get_hash_info_from_sdt(dev, sdt_no, &hash_entry_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_hash_get_hash_info_from_sdt");
    
    p_hash_cfg = hash_entry_cfg.p_hash_cfg;
    ZXIC_COMM_CHECK_POINT(p_hash_cfg);

    hash_entry.p_key = aucKey;
    hash_entry.p_rst = aucRst;
    ZXIC_COMM_MEMSET_S(hash_entry.p_key, sizeof(aucKey), 0, sizeof(aucKey));
    ZXIC_COMM_MEMSET_S(hash_entry.p_rst, sizeof(aucRst), 0, sizeof(aucRst));
    hash_entry.p_key[0] = DPP_GET_HASH_KEY_CTRL(key_valid, 
                                            hash_entry_cfg.key_type, 
                                            hash_entry_cfg.table_id);
    ZXIC_COMM_CHECK_INDEX(hash_entry_cfg.actu_key_size, HASH_ACTU_KEY_MIN, HASH_ACTU_KEY_MAX);
    ZXIC_COMM_MEMCPY(&hash_entry.p_key[1],p_dtb_hash_entry->p_actu_key,hash_entry_cfg.actu_key_size);
   
    // if(p_hash_cfg->ddr_valid)
    // {
    //     rc  = dpp_dtb_hash_ddr_get(dev_id,queue_id,&hash_entry_cfg,&hash_entry,srh_mode,&srh_succ);  
    //     ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_dtb_hash_ddr_get");
    // }


    rc  = dpp_dtb_hash_zcam_get(dev,queue_id,&hash_entry_cfg,&hash_entry,srh_mode,&srh_succ);  
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_dtb_hash_zcam_get"); 
 

    if(!srh_succ)
    {
        p_hash_cfg->hash_stat.search_fail++;
        ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "Hash search key fail!\n");
        return DPP_HASH_RC_SRH_FAIL;
    }

    ZXIC_COMM_MEMCPY(p_dtb_hash_entry->p_rst,hash_entry.p_rst,1<<(hash_entry_cfg.rsp_mode + 2));

    return DPP_OK;
}

/** dtb 通过key和mask获取ACL表级联结果
* @param   dev_id       设备号
* @param   queue_id     队列号
* @param   sdt_no       eram表sdt表号
* @param   p_dump_acl_entry  etcam 数据结构，数据已分配相应内存,需要输入key和mask
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_acl_data_get(DPP_DEV_T *dev,
                                ZXIC_UINT32 queue_id, 
                                ZXIC_UINT32 sdt_no, 
                                DPP_DTB_ACL_ENTRY_INFO_T *p_dump_acl_entry)
{
    DPP_STATUS  rc = DPP_OK;

    ZXIC_UINT32 block_idx = 0;
    ZXIC_UINT32 ram_addr = 0;
    ZXIC_UINT32 etcam_wr_mode = 0;

    ZXIC_UINT32 etcam_key_mode = 0;
    ZXIC_UINT32 etcam_table_id = 0;
    ZXIC_UINT32 as_enable = 0;
    ZXIC_UINT32 as_eram_baddr = 0;
    ZXIC_UINT32 etcam_as_mode = 0;

    ZXIC_UINT32 row_index = 0;
    ZXIC_UINT32 col_index = 0;

    DPP_ETCAM_ENTRY_T etcam_entry_dm = {0};
    DPP_ETCAM_ENTRY_T etcam_entry_xy = {0};
    ZXIC_UINT32 as_eram_data[4] = {0};
    ZXIC_UINT8 temp_data[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 temp_mask[DPP_ETCAM_WIDTH_MAX / 8] = {0};

    DPP_ACL_CFG_EX_T *p_acl_cfg = NULL;
    DPP_ACL_TBL_CFG_T *p_tbl_cfg = NULL;

    DPP_SDTTBL_ETCAM_T sdt_etcam_info = {0};  /*SDT内容*/

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_dump_acl_entry);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_dump_acl_entry->key_data);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_dump_acl_entry->key_mask);

    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_etcam_info);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_soft_sdt_tbl_get");
    etcam_key_mode = sdt_etcam_info.etcam_key_mode;
    etcam_as_mode = sdt_etcam_info.as_rsp_mode;
    etcam_table_id = sdt_etcam_info.etcam_table_id;
    as_enable = sdt_etcam_info.as_en;
    as_eram_baddr = sdt_etcam_info.as_eram_baddr;

    if(as_enable)
    {
        ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_dump_acl_entry->p_as_rslt); 
    }

    etcam_entry_xy.mode = etcam_key_mode;
    etcam_entry_xy.p_data = temp_data;
    etcam_entry_xy.p_mask = temp_mask;
    etcam_entry_dm.mode = etcam_key_mode;
    etcam_entry_dm.p_data = p_dump_acl_entry->key_data;
    etcam_entry_dm.p_mask = p_dump_acl_entry->key_mask;

    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_PRINT("acl get DM:/n");
        dpp_acl_data_print(etcam_entry_dm.p_data, etcam_entry_dm.p_mask,  etcam_entry_dm.mode);
    }

    rc = dpp_acl_cfg_get(dev,&p_acl_cfg);//获取ACL表资源配置
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_cfg_get");
    ZXIC_COMM_CHECK_POINT(p_acl_cfg);

    p_tbl_cfg = p_acl_cfg->acl_tbls + etcam_table_id;//得到该acl表的资源配置

    if (!p_tbl_cfg->is_used)
    {
        ZXIC_COMM_TRACE_ERROR("table[ %d ] is not init!\n", etcam_table_id);
        ZXIC_COMM_ASSERT(0);
        return DPP_ACL_RC_TBL_NOT_INIT;
    }

    /*计算地址等信息*/
    rc = dpp_acl_hdw_addr_get(p_tbl_cfg, p_dump_acl_entry->handle, &block_idx, &ram_addr, &etcam_wr_mode);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_acl_hdw_addr_get");

    rc = dpp_dtb_etcam_entry_get(dev, 
                                 queue_id,
                                 block_idx, 
                                 ram_addr,
                                 etcam_wr_mode,
                                 DPP_ETCAM_OPR_XY, 
                                 as_enable,
                                 as_eram_baddr,
                                 p_dump_acl_entry->handle,
                                 etcam_as_mode,//128:3   64:2
                                 &etcam_entry_xy,
                                 (ZXIC_UINT8 *)as_eram_data);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_etcam_entry_get");

    if (dpp_etcam_entry_cmp(&etcam_entry_dm, &etcam_entry_xy) == 0)
    {
        ZXIC_COMM_PRINT("Acl table[ %d ] search in hardware success: handle[ 0x%x ], block[ %d ], ram_addr[ %d ], rd_mode[ %x ].\n",
                    p_tbl_cfg->table_id, p_dump_acl_entry->handle, block_idx, ram_addr, etcam_wr_mode);
    }
    else
    {
        ZXIC_COMM_PRINT("Acl table[ %d ] search in hardware fail: handle[ 0x%x ], block[ %d ], ram_addr[ %d ], rd_mode[ %x ].\n",
                    p_tbl_cfg->table_id, p_dump_acl_entry->handle, block_idx, ram_addr, etcam_wr_mode);

        return DPP_ERR;
    }

    if(as_enable)
    {
        rc = dtb_eram_index_cal(dev, etcam_as_mode, p_dump_acl_entry->handle, &row_index, &col_index);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dtb_eram_index_cal");
        switch (etcam_as_mode)
        {
            case ERAM128_TBL_128b:
            {
                ZXIC_COMM_MEMCPY(p_dump_acl_entry->p_as_rslt, as_eram_data, (128 / 8));
                break;
            }

            case ERAM128_TBL_64b:
            {
                ZXIC_COMM_MEMCPY(p_dump_acl_entry->p_as_rslt, as_eram_data + ((1 - col_index) << 1), (64 / 8));
                break;
            }

            case ERAM128_TBL_1b:
            {
                ZXIC_COMM_UINT32_GET_BITS(*(ZXIC_UINT32 *)p_dump_acl_entry->p_as_rslt, *(as_eram_data + (3 - col_index / 32)), (col_index % 32), 1);
                break;
            }
        }
    }

    return rc;
}

/** dtb etcam 数据get接口，通过handle值获取etcam数据
* @param   dev_id       设备号
* @param   queue_id     队列号
* @param   sdt_no       eram表sdt表号
* @param   p_dump_acl_entry  etcam 数据结构，数据已分配相应内存
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_etcam_data_get(DPP_DEV_T *dev,
                                  ZXIC_UINT32 queue_id, 
                                  ZXIC_UINT32 sdt_no, 
                                  DPP_DTB_ACL_ENTRY_INFO_T *p_dump_acl_entry)
{
    DPP_STATUS  rc = DPP_OK;

    ZXIC_UINT32 block_idx = 0;
    ZXIC_UINT32 ram_addr = 0;
    ZXIC_UINT32 etcam_wr_mode = 0;

    ZXIC_UINT32 etcam_key_mode = 0;
    ZXIC_UINT32 etcam_table_id = 0;
    ZXIC_UINT32 as_enable = 0;
    ZXIC_UINT32 as_eram_baddr = 0;
    ZXIC_UINT32 etcam_as_mode = 0;

    ZXIC_UINT32 row_index = 0;
    ZXIC_UINT32 col_index = 0;

    DPP_ETCAM_ENTRY_T etcam_entry_dm = {0};
    ZXIC_UINT32 as_eram_data[4] = {0};

    ZXIC_UINT8 temp_data[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 temp_mask[DPP_ETCAM_WIDTH_MAX / 8] = {0};

    DPP_ACL_CFG_EX_T *p_acl_cfg = NULL;
    DPP_ACL_TBL_CFG_T *p_tbl_cfg = NULL;

    DPP_SDTTBL_ETCAM_T sdt_etcam_info = {0};  /*SDT内容*/

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_dump_acl_entry);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_dump_acl_entry->key_data);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_dump_acl_entry->key_mask);

    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_etcam_info);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_soft_sdt_tbl_get");
    etcam_key_mode = sdt_etcam_info.etcam_key_mode;
    etcam_as_mode = sdt_etcam_info.as_rsp_mode;
    etcam_table_id = sdt_etcam_info.etcam_table_id;
    as_enable = sdt_etcam_info.as_en;
    as_eram_baddr = sdt_etcam_info.as_eram_baddr;

    if(as_enable)
    {
        ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_dump_acl_entry->p_as_rslt); 
    }

    etcam_entry_dm.mode = etcam_key_mode;
    etcam_entry_dm.p_data = temp_data;
    etcam_entry_dm.p_mask = temp_mask;

    rc = dpp_acl_cfg_get(dev,&p_acl_cfg);//获取ACL表资源配置
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_cfg_get");
    ZXIC_COMM_CHECK_POINT(p_acl_cfg);

    p_tbl_cfg = p_acl_cfg->acl_tbls + etcam_table_id;//得到该acl表的资源配置

    if (!p_tbl_cfg->is_used)
    {
        ZXIC_COMM_TRACE_ERROR("table[ %d ] is not init!\n", etcam_table_id);
        ZXIC_COMM_ASSERT(0);
        return DPP_ACL_RC_TBL_NOT_INIT;
    }

    /*计算地址等信息*/
    rc = dpp_acl_hdw_addr_get(p_tbl_cfg, p_dump_acl_entry->handle, &block_idx, &ram_addr, &etcam_wr_mode);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_acl_hdw_addr_get");

    rc = dpp_dtb_etcam_entry_get(dev, 
                                 queue_id,
                                 block_idx, 
                                 ram_addr,
                                 etcam_wr_mode,
                                 DPP_ETCAM_OPR_DM, 
                                 as_enable,
                                 as_eram_baddr,
                                 p_dump_acl_entry->handle,
                                 etcam_as_mode,//128:3   64:2
                                 &etcam_entry_dm,
                                 (ZXIC_UINT8 *)as_eram_data);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_etcam_entry_get");

    ZXIC_COMM_MEMCPY(p_dump_acl_entry->key_data, etcam_entry_dm.p_data, DPP_ETCAM_ENTRY_SIZE_GET(etcam_key_mode));
    ZXIC_COMM_MEMCPY(p_dump_acl_entry->key_mask, etcam_entry_dm.p_mask, DPP_ETCAM_ENTRY_SIZE_GET(etcam_key_mode));

    if(as_enable)
    {
        rc = dtb_eram_index_cal(dev, etcam_as_mode, p_dump_acl_entry->handle, &row_index, &col_index);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dtb_eram_index_cal");
        switch (etcam_as_mode)
        {
            case ERAM128_TBL_128b:
            {
                ZXIC_COMM_MEMCPY(p_dump_acl_entry->p_as_rslt, as_eram_data, (128 / 8));
                break;
            }

            case ERAM128_TBL_64b:
            {
                ZXIC_COMM_MEMCPY(p_dump_acl_entry->p_as_rslt, as_eram_data + ((1 - col_index) << 1), (64 / 8));
                break;
            }

            case ERAM128_TBL_1b:
            {
                ZXIC_COMM_UINT32_GET_BITS(*(ZXIC_UINT32 *)p_dump_acl_entry->p_as_rslt, *(as_eram_data + (3 - col_index / 32)), (col_index % 32), 1);
                break;
            }
        }
    }

    return rc;
}


#endif /**DTB GET INTERFACE*/

#if ZXIC_REAL("DTB FLUSH INTERFACE")

/***********************************************************/
/** DTB dd  整个流表清空Flush 
* @param   dev_id          设备号
* @param   queue_id        队列号
* @param   ddr_base_addr   ddr基地址，以4K*128bit为单位
* @param   ddr_wr_mode     ddr写模式 0-128bit, 1-256bit, 2-384bit, 3-512bit,取值参考SMMU1_DDR_WRT_MODE_E的定义
* @param   ddr_ecc_en      ddr ECC使能
* @param   start_index     flush开始的条目
* @param   entry_num       下发的条目数
* @param   element_id      返回下表使用的元素id
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/07/03
************************************************************/  
DPP_STATUS dpp_dtb_smmu1_flush_cycle(DPP_DEV_T *dev, 
                                     ZXIC_UINT32 queue_id, 
                                     ZXIC_UINT32 ddr_base_addr, 
                                     ZXIC_UINT32 ddr_wr_mode, 
                                     ZXIC_UINT32 ddr_ecc_en,
                                     ZXIC_UINT32 start_index,
                                     ZXIC_UINT32 entry_num, 
                                     ZXIC_UINT32 *element_id)
{
    DPP_STATUS  rc = DPP_OK;

    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 current_index = 0;
    ZXIC_UINT32 ddr_entry_len = 0;
    ZXIC_UINT32 *data_buff = NULL;
    DPP_DTB_DDR_ENTRY_INFO_T *p_entry_arr = NULL;

    ZXIC_COMM_CHECK_POINT(dev);

    ddr_entry_len = 4 * (ddr_wr_mode + 1);//计算数据长度

    p_entry_arr = (DPP_DTB_DDR_ENTRY_INFO_T *)ZXIC_COMM_MALLOC(entry_num * sizeof(DPP_DTB_DDR_ENTRY_INFO_T));
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_entry_arr);

    data_buff = (ZXIC_UINT32 *)ZXIC_COMM_MALLOC(ddr_entry_len * sizeof(ZXIC_UINT32));
    ZXIC_COMM_CHECK_DEV_POINT_MEMORY_FREE_NO_ASSERT(DEV_ID(dev), data_buff, p_entry_arr);

    ZXIC_COMM_MEMSET(data_buff, 0, ddr_entry_len * sizeof(ZXIC_UINT32));

    // 下表数据准备
    for(index = 0; index < entry_num; index++)
    {
        current_index = start_index + index;

        p_entry_arr[index].index = current_index;
        p_entry_arr[index].p_data = data_buff;
    }

    rc = dpp_dtb_smmu1_data_write_cycle(dev, 
                                        queue_id, 
                                        ddr_base_addr, 
                                        ddr_wr_mode, 
                                        ddr_ecc_en,
                                        entry_num, 
                                        p_entry_arr,
                                        element_id);
    ZXIC_COMM_FREE(data_buff);
    ZXIC_COMM_FREE(p_entry_arr);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_smmu1_data_write_cycle");

    return rc;
}

/***********************************************************/
/** DTB dd  整个流表清空Flush 
* @param   dev_id          设备号
* @param   queue_id        队列号
* @param   ddr_base_addr   ddr基地址，以4K*128bit为单位
* @param   ddr_wr_mode     ddr写模式 0-128bit, 1-256bit, 2-384bit, 3-512bit,取值参考SMMU1_DDR_WRT_MODE_E的定义
* @param   ddr_ecc_en      ddr ECC使能
* @param   start_index     flush开始的条目
* @param   entry_num       下发的条目数
* @param   element_id      返回下表使用的元素id
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/07/03
************************************************************/  
DPP_STATUS dpp_dtb_smmu1_flush(DPP_DEV_T *dev,
                               ZXIC_UINT32 queue_id, 
                               ZXIC_UINT32 ddr_base_addr, 
                               ZXIC_UINT32 ddr_wr_mode, 
                               ZXIC_UINT32 ddr_ecc_en,
                               ZXIC_UINT32 entry_num, 
                               ZXIC_UINT32 *element_id)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 entry_num_max = 0;
    ZXIC_UINT32 entry_cycle = 0;
    ZXIC_UINT32 entry_remains = 0;
    ZXIC_UINT32 start_index = 0;
    ZXIC_UINT32 i = 0;

    ZXIC_COMM_CHECK_POINT(dev);

    ZXIC_COMM_TRACE_INFO("ddr_base_addr is %d.\n", ddr_base_addr);
    ZXIC_COMM_TRACE_INFO("ddr_wr_mode is %d.\n", ddr_wr_mode);
    ZXIC_COMM_TRACE_INFO("entry_num is %d.\n", entry_num);

    switch (ddr_wr_mode)
    {
        case SMMU1_DDR_WRT_128b://128bit
        {
            entry_num_max = 0x1ff;
            break;
        }

        case SMMU1_DDR_WRT_256b://256bit
        {
            entry_num_max = 0x155;
            break;
        }

        case SMMU1_DDR_WRT_384b://384bit
        {
            entry_num_max = 0xcc;
            break;
        }

        case SMMU1_DDR_WRT_512b://512bit
        {
            entry_num_max = 0xcc;
            break;
        }
    }

    ZXIC_COMM_CHECK_INDEX_EQUAL(entry_num_max, 0);
    entry_cycle = entry_num / entry_num_max;
    entry_remains = entry_num % entry_num_max;

    for(i = 0; i < entry_cycle; ++i)
    {
        start_index = entry_num_max * i;

        rc = dpp_dtb_smmu1_flush_cycle(dev, 
                                       queue_id, 
                                       ddr_base_addr, 
                                       ddr_wr_mode, 
                                       ddr_ecc_en,
                                       start_index,
                                       entry_num_max, 
                                       element_id);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_smmu1_flush_cycle");

        ZXIC_COMM_TRACE_INFO("dpp_dtb_smmu1_flush_cycle[%d] element_id = %d\n",i,*element_id);
    }

    if(entry_remains)
    {
        start_index = entry_num_max * entry_cycle;
        rc = dpp_dtb_smmu1_flush_cycle(dev, 
                                       queue_id, 
                                       ddr_base_addr, 
                                       ddr_wr_mode, 
                                       ddr_ecc_en,
                                       start_index,
                                       entry_remains, 
                                       element_id);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_smmu1_flush_cycle");

        ZXIC_COMM_TRACE_INFO("dpp_dtb_smmu1_flush_cycle: element_id = %d\n",*element_id);
    }

    return DPP_OK;
}


/***********************************************************/
/** flush指定ZCELL指定范围空间
* @param   dev_id      设备id
* @param   queue_id    队列id
* @param   zcell_id    zcell id 0~127
* @param   start_index 起始索引 0~511
* @param   num         条目数
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
DPP_STATUS dpp_dtb_hash_zcell_range_clr(DPP_DEV_T *dev, 
                                        ZXIC_UINT32 queue_id, 
                                        ZXIC_UINT32 zcell_id, 
                                        ZXIC_UINT32 start_index, 
                                        ZXIC_UINT32 num)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 zcdep = 0;
    ZXIC_UINT32 addr = 0;
    ZXIC_UINT32 addr_offset = 0;
    ZXIC_UINT32 dtb_len = 0;
    ZXIC_UINT32 data[512 / 32] = {0};
    DPP_DTB_ENTRY_T entry = {0};
    ZXIC_UINT32 entry_data_buff[512 / 32] = {0};
    ZXIC_UINT8 entry_cmd_buff[DTB_TABLE_CMD_SIZE_BYTE] = {0};
    ZXIC_UINT8 *p_data_buff = NULL;
    ZXIC_UINT32 element_id = 0;
    ZXIC_UINT32 end_index = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev), DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), zcell_id, 0, SE_ZCELL_TOTAL_NUM - 1);
    ZXIC_COMM_CHECK_INDEX_UPPER(start_index,SE_RAM_DEPTH - 1);

    entry.cmd = entry_cmd_buff;
    entry.data = (ZXIC_UINT8 *)entry_data_buff;
    ZXIC_COMM_MEMSET(entry_cmd_buff, 0, sizeof(entry_cmd_buff));
    ZXIC_COMM_MEMSET(entry_data_buff, 0, sizeof(entry_data_buff));
    ZXIC_COMM_MEMSET(data, 0, sizeof(data));

    p_data_buff = (ZXIC_UINT8 *) ZXIC_COMM_MALLOC(DPP_DTB_TABLE_DATA_BUFF_SIZE * sizeof(ZXIC_UINT8));
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data_buff);
    ZXIC_COMM_MEMSET(p_data_buff, 0, DPP_DTB_TABLE_DATA_BUFF_SIZE * sizeof(ZXIC_UINT8));

    end_index = start_index+num;
    end_index = (end_index>SE_RAM_DEPTH) ? SE_RAM_DEPTH:end_index;
    
    for(zcdep=start_index;zcdep<end_index;zcdep++)
    {
        addr = ZBLK_ITEM_ADDR_CALC(zcell_id, zcdep);
        rc = dpp_dtb_se_alg_zcam_data_write(DEV_ID(dev), addr, (ZXIC_UINT8 *)data, &entry);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_se_alg_zcam_data_write", p_data_buff);

        dtb_len += DTB_ZCAM_LEN_SIZE;
        addr_offset = (zcdep-start_index) * DTB_ZCAM_LEN_SIZE * DTB_LEN_POS_SETP; /*在缓存中相对于0的offset*/
        rc = dpp_dtb_data_write(p_data_buff, addr_offset, &entry);
        ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_data_write", p_data_buff);
        ZXIC_COMM_MEMSET(entry_cmd_buff, 0x0, sizeof(entry_cmd_buff));
        ZXIC_COMM_MEMSET(entry_data_buff, 0x0, sizeof(entry_data_buff));
    }

    rc = dpp_dtb_write_down_table_data(dev,  
                                     queue_id, 
                                     dtb_len * 16,
                                     p_data_buff,
                                     &element_id);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_write_down_table_data", p_data_buff);

    ZXIC_COMM_FREE(p_data_buff);
    return DPP_OK;
}

/***********************************************************/
/** flush指定ZCELL空间
* @param   dev_id     设备id
* @param   queue_id   队列id
* @param   zcell_id   zcell id 0~127
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
DPP_STATUS dpp_dtb_hash_zcell_clr(DPP_DEV_T *dev, ZXIC_UINT32 queue_id, ZXIC_UINT32 zcell_id)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 max_num = 0xcc;
    ZXIC_UINT32 start_index = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev), DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), zcell_id, 0, SE_ZCELL_TOTAL_NUM - 1);
    ZXIC_COMM_CHECK_INDEX(queue_id, 0, DTB_QUEUE_MAX - 1);

    start_index = 0;
    rc = dpp_dtb_hash_zcell_range_clr(dev, 
                                      queue_id, 
                                      zcell_id, 
                                      start_index, 
                                      max_num);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_hash_zcell_range_clr");
    
    start_index = max_num;
    rc = dpp_dtb_hash_zcell_range_clr(dev, 
                                      queue_id, 
                                      zcell_id, 
                                      start_index, 
                                      max_num);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_hash_zcell_range_clr");
    
    start_index = max_num*2;
    rc = dpp_dtb_hash_zcell_range_clr(dev, 
                                      queue_id, 
                                      zcell_id, 
                                      start_index, 
                                      max_num);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_hash_zcell_range_clr");

    return DPP_OK;
}

/***********************************************************/
/** flush指定ZREG空间
* @param   dev_id     设备id
* @param   queue_id   队列id
* @param   zblk_id    zblock id 0~31
* @param   zreg_id    zreg id 0~3
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
DPP_STATUS dpp_dtb_hash_zreg_clr(DPP_DEV_T *dev, ZXIC_UINT32 queue_id, ZXIC_UINT32 zblk_id, ZXIC_UINT32 zreg_id)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 addr = 0;
    ZXIC_UINT32 wr_data[16] = {0};
    ZXIC_UINT32 dtb_len = 0;

    DPP_DTB_ENTRY_T entry = {0};
    ZXIC_UINT32 entry_data_buff[512 / 32] = {0};
    ZXIC_UINT8 entry_cmd_buff[DTB_TABLE_CMD_SIZE_BYTE] = {0};
    ZXIC_UINT8 *p_data_buff = NULL;
    ZXIC_UINT32 element_id = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev), DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), zblk_id, 0, SE_ZBLK_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), zreg_id, 0, SE_ZREG_NUM - 1);
    ZXIC_COMM_CHECK_INDEX(queue_id, 0, DTB_QUEUE_MAX - 1);

    entry.cmd = entry_cmd_buff;
    entry.data = (ZXIC_UINT8 *)entry_data_buff;
    ZXIC_COMM_MEMSET(entry_cmd_buff, 0, sizeof(entry_cmd_buff));
    ZXIC_COMM_MEMSET(entry_data_buff, 0, sizeof(entry_data_buff));
    ZXIC_COMM_MEMSET(wr_data,0x0,sizeof(wr_data));

    addr = ZBLK_HASH_LIST_REG_ADDR_CALC(zblk_id, zreg_id);
    rc = dpp_dtb_se_alg_zcam_data_write(DEV_ID(dev), addr, (ZXIC_UINT8 *)wr_data, &entry);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_se_alg_zcam_data_write");

    p_data_buff = (ZXIC_UINT8 *) ZXIC_COMM_MALLOC(DPP_DTB_TABLE_DATA_BUFF_SIZE * sizeof(ZXIC_UINT8));
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data_buff);
    ZXIC_COMM_MEMSET(p_data_buff, 0x0, DPP_DTB_TABLE_DATA_BUFF_SIZE * sizeof(ZXIC_UINT8));

    dtb_len += DTB_ZCAM_LEN_SIZE;
    rc = dpp_dtb_data_write(p_data_buff, 0, &entry);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_data_write",p_data_buff);

    rc = dpp_dtb_write_down_table_data(dev,  
                                     queue_id, 
                                     dtb_len * 16,
                                     p_data_buff,
                                     &element_id);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_write_down_table_data",p_data_buff);

    ZXIC_COMM_FREE(p_data_buff);
    return DPP_OK;
}

/***********************************************************/
/** flush指定ZCAM空间(独占模式)
* @param   p_se_cfg   全局数据结构
* @param   queue_id   队列id
* @param   fun_id     hash引擎0~3
* @param   bulk_id    bulk id 0~7
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
DPP_STATUS dpp_dtb_specify_zcam_space_clr(DPP_DEV_T *dev,
                                           DPP_SE_CFG *p_se_cfg,
                                           ZXIC_UINT32 queue_id,
                                           ZXIC_UINT32 fun_id,
                                           ZXIC_UINT32 bulk_id)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 zblock_id = 0;

    D_NODE *p_zblk_dn = NULL;
    SE_ZBLK_CFG *p_zblk = NULL;
    SE_ZREG_CFG *p_zreg = NULL;
    SE_ZCELL_CFG *p_zcell = NULL;
    DPP_HASH_CFG *p_hash_cfg = NULL;
    FUNC_ID_INFO *p_func_info = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev),p_se_cfg);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), fun_id, 0, HASH_FUNC_ID_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), bulk_id, 0, HASH_BULK_NUM - 1);
    ZXIC_COMM_CHECK_INDEX(queue_id, 0, DTB_QUEUE_MAX - 1);
    
    // dev_id = p_se_cfg->dev_id;
    p_func_info = DPP_GET_FUN_INFO(p_se_cfg, fun_id);
    DPP_SE_CHECK_FUN(p_func_info, fun_id, FUN_HASH);
    p_hash_cfg = (DPP_HASH_CFG *)p_func_info->fun_ptr;
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev),p_hash_cfg);
    p_zblk_dn = p_hash_cfg->hash_shareram.zblk_list.p_next;
    
    while (p_zblk_dn)
    {
        p_zblk = (SE_ZBLK_CFG *)p_zblk_dn->data;
        zblock_id = p_zblk->zblk_idx;

        for (i = 0; i < SE_ZCELL_NUM; i++)
        {
            p_zcell = &(p_zblk->zcell_info[i]);
            if ((p_zcell->flag & DPP_ZCELL_FLAG_IS_MONO) && (p_zcell->bulk_id == bulk_id))
            {
                rc = dpp_dtb_hash_zcell_clr(dev, queue_id, p_zcell->zcell_idx);
                ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_hash_zcell_clr");
                ZXIC_COMM_TRACE_DEV_INFO(DEV_ID(dev), "the Zblock[%d]'s Mono Zcell_id :%d \n", zblock_id, p_zcell->zcell_idx);
            }
        }

        for (i = 0; i < SE_ZREG_NUM; i++)
        {
            p_zreg = &(p_zblk->zreg_info[i]);
            if ((p_zreg->flag & DPP_ZREG_FLAG_IS_MONO) && (p_zreg->bulk_id == bulk_id))
            {
                rc = dpp_dtb_hash_zreg_clr(dev, queue_id, zblock_id, i);
                ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "npe_hash_zreg_clr");
                ZXIC_COMM_TRACE_DEV_INFO(DEV_ID(dev), "the Zblock[%d]'s Mono Zreg_id :%d \n", zblock_id, i);
            }
        }

        p_zblk_dn = p_zblk_dn->next;
    }
    
    return DPP_OK;
}

/***********************************************************/
/** flush当前hash引擎占用的ZCAM空间
* @param   p_se_cfg   全局数据结构
* @param   queue_id   队列id
* @param   fun_id     hash引擎0~3
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
DPP_STATUS dpp_dtb_zcam_space_clr(DPP_DEV_T *dev,
                                  DPP_SE_CFG *p_se_cfg,
                                  ZXIC_UINT32 queue_id,
                                  ZXIC_UINT32 fun_id)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 zblock_id = 0;

    D_NODE *p_zblk_dn = NULL;
    SE_ZBLK_CFG *p_zblk = NULL;
    SE_ZCELL_CFG *p_zcell = NULL;
    DPP_HASH_CFG *p_hash_cfg = NULL;
    FUNC_ID_INFO *p_func_info = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev),p_se_cfg);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), fun_id, 0, HASH_FUNC_ID_NUM - 1);
    ZXIC_COMM_CHECK_INDEX(queue_id, 0, DTB_QUEUE_MAX - 1);

    // dev_id = p_se_cfg->dev_id;
    p_func_info = DPP_GET_FUN_INFO(p_se_cfg, fun_id);
    DPP_SE_CHECK_FUN(p_func_info, fun_id, FUN_HASH);
    p_hash_cfg = (DPP_HASH_CFG *)p_func_info->fun_ptr;
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev),p_hash_cfg);
    p_zblk_dn = p_hash_cfg->hash_shareram.zblk_list.p_next;
    
    while (p_zblk_dn)
    {
        p_zblk = (SE_ZBLK_CFG *)p_zblk_dn->data;
        zblock_id = p_zblk->zblk_idx;
        for (index = 0; index < SE_ZCELL_NUM; index++)
        {
            p_zcell = &(p_zblk->zcell_info[index]);
            rc = dpp_dtb_hash_zcell_clr(dev, queue_id, p_zcell->zcell_idx);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_hash_zcell_clr");
        }

        for (index = 0; index < SE_ZREG_NUM; index++)
        {
            rc = dpp_dtb_hash_zreg_clr(dev, queue_id, zblock_id, index);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_hash_zreg_clr");
        }
        p_zblk_dn = p_zblk_dn->next;
    }

    return DPP_OK;
}


/***********************************************************/
/** 清除指定空间的hash表项(清除软件配置)
* @param   p_se_cfg
* @param   hash_id
* @param   bulk_id
*
* @return
* @remark  无
* @see
* @author  cq      @date  2023/08/18
************************************************************/
DPP_STATUS dpp_hash_specify_entry_delete(DPP_DEV_T *dev, DPP_SE_CFG *p_se_cfg,ZXIC_UINT32 hash_id, ZXIC_UINT32 bulk_id)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT8 key_valid = 0;
    ZXIC_UINT32 table_id = 0;
    ZXIC_UINT32 temp_bulk_id = 0;

    D_NODE *p_node = NULL;
    ZXIC_RB_TN *p_rb_tn = NULL;
    D_HEAD *p_head_hash_rb = NULL;
    DPP_HASH_CFG *p_hash_cfg = NULL;
    FUNC_ID_INFO *p_func_info = NULL;
    DPP_HASH_RBKEY_INFO *p_rbkey = NULL;
    DPP_HASH_RBKEY_INFO *p_rbkey_rtn = NULL;
    ZXIC_RB_TN *p_rb_tn_rtn = NULL;
    SE_ITEM_CFG *p_item = NULL;

    ZXIC_COMM_CHECK_POINT(p_se_cfg);
    ZXIC_COMM_CHECK_INDEX(hash_id, 0, HASH_FUNC_ID_NUM - 1);
    ZXIC_COMM_CHECK_INDEX(bulk_id, 0, HASH_BULK_NUM - 1);

    dev_id = p_se_cfg->dev_id;
    p_func_info = DPP_GET_FUN_INFO(p_se_cfg, hash_id);
    DPP_SE_CHECK_FUN(p_func_info, hash_id, FUN_HASH);


    p_hash_cfg = (DPP_HASH_CFG *)p_func_info->fun_ptr;
    p_head_hash_rb = &p_hash_cfg->hash_rb.tn_list;

    p_node = p_head_hash_rb->p_next;
    while(p_node)
    {
        p_rb_tn = (ZXIC_RB_TN *)p_node->data;
        p_rbkey = (DPP_HASH_RBKEY_INFO *)p_rb_tn->p_key;
        key_valid = DPP_GET_HASH_KEY_VALID(p_rbkey->key);
        table_id = DPP_GET_HASH_TBL_ID(p_rbkey->key);
        temp_bulk_id = ((table_id >> 2) & 0x7);
        if((!key_valid) || (temp_bulk_id != bulk_id))
        {
            p_node = p_node->next; 
            continue;
        }
        
        p_node = p_node->next;           /*zxic_comm_rb_delete删除操作之前执行*/
        rc = zxic_comm_rb_delete(&p_hash_cfg->hash_rb, p_rbkey, &p_rb_tn_rtn);
        if (ZXIC_RBT_RC_SRHFAIL == rc)
        {
            p_hash_cfg->hash_stat.delete_fail++;
            ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, "Error!there is not item in hash!\n");
            return DPP_HASH_RC_DEL_SRHFAIL;
        }

        ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_rb_tn_rtn);
        p_rbkey_rtn = (DPP_HASH_RBKEY_INFO *)(p_rb_tn_rtn->p_key);
        p_item = p_rbkey_rtn->p_item_info;
        
        rc = zxic_comm_double_link_del(&(p_rbkey_rtn->entry_dn), &(p_item->item_list));
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "zxic_comm_double_link_del");
        p_item->wrt_mask &= ~(DPP_GET_HASH_ENTRY_MASK(p_rbkey_rtn->entry_size, p_rbkey_rtn->entry_pos)) & 0xF;

        if (0 == p_item->item_list.used)
        {
            if ((ITEM_DDR_256 == p_item->item_type) || (ITEM_DDR_512 == p_item->item_type))
            {
                /* modify coverity by yinxh 2021.03.10*,以256bit为单位。暂不考虑512bit的情况*/
                ZXIC_COMM_CHECK_INDEX_UPPER(p_item->item_index, p_hash_cfg->p_bulk_ddr_info[bulk_id]->item_num);
                p_hash_cfg->p_bulk_ddr_info[bulk_id]->p_item_array[p_item->item_index] = NULL;
                ZXIC_COMM_FREE(p_item);
            }
            else
            {
                p_item->valid = 0;
            }
        }

        ZXIC_COMM_FREE(p_rbkey_rtn);
        ZXIC_COMM_FREE(p_rb_tn_rtn);
        p_hash_cfg->hash_stat.delete_ok++;
        p_hash_cfg->hash_stat.insert_table[table_id].delete++;
    }

    return DPP_OK;
}

/***********************************************************/
/** flush指定eram空间
* @param   dev_id     设备id
* @param   queue_id   队列id
* @param   sdt_no     sdt号
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
DPP_STATUS dpp_dtb_eram_table_flush(DPP_DEV_T *dev,
                               ZXIC_UINT32 queue_id,
                               ZXIC_UINT32 sdt_no)
{
    DPP_STATUS rc         = DPP_OK;
    ZXIC_UINT32 eram_depth = 0;
    ZXIC_UINT32 element_id = 0;
    ZXIC_UINT32 index      = 0;
    ZXIC_UINT8 *pBuff      = NULL;
    DPP_DTB_ERAM_ENTRY_INFO_T *p_entry_arr = NULL;
    DPP_DTB_ERAM_ENTRY_INFO_T *p_temp_entry_arr = NULL;
    DPP_SDTTBL_ERAM_T sdt_eram           = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    rc = dpp_soft_sdt_tbl_get(dev,sdt_no,&sdt_eram);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_soft_sdt_tbl_get");

    eram_depth = sdt_eram.eram_table_depth;
    p_entry_arr = (DPP_DTB_ERAM_ENTRY_INFO_T *)ZXIC_COMM_MALLOC(eram_depth*sizeof(DPP_DTB_ERAM_ENTRY_INFO_T));
    ZXIC_COMM_CHECK_POINT(p_entry_arr);
    pBuff = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(eram_depth*4*sizeof(ZXIC_UINT32));
    ZXIC_COMM_CHECK_POINT_MEMORY_FREE(pBuff,p_entry_arr);
    ZXIC_COMM_MEMSET((ZXIC_UINT8 *)p_entry_arr,0x0,eram_depth*sizeof(DPP_DTB_ERAM_ENTRY_INFO_T));
    ZXIC_COMM_MEMSET(pBuff,0x0,eram_depth*4*sizeof(ZXIC_UINT32));
    for(index=0;index<eram_depth;index++)
    {
        p_temp_entry_arr = p_entry_arr+index;
        p_temp_entry_arr->index = index;
        p_temp_entry_arr->p_data = (ZXIC_UINT32 *)(pBuff + (index * 4 * sizeof(ZXIC_UINT32)));
    }

    rc = dpp_dtb_eram_dma_write(dev, 
                                queue_id,
                                sdt_no, 
                                eram_depth, 
                                p_entry_arr,
                                &element_id);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE2PTR_NO_ASSERT(rc, "dpp_dtb_eram_dma_write",pBuff,p_entry_arr);

    ZXIC_COMM_FREE(pBuff);
    ZXIC_COMM_FREE(p_entry_arr);
    return DPP_OK;
}

/***********************************************************/
/** flush指定hash空间(DDR/ZCAM)
* @param   dev_id     设备id
* @param   queue_id   队列id
* @param   sdt_no     sdt号
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/08/02
************************************************************/
DPP_STATUS dpp_dtb_hash_table_flush(DPP_DEV_T *dev,
                               ZXIC_UINT32 queue_id,
                               ZXIC_UINT32 sdt_no)
{
    DPP_STATUS rc         = DPP_OK;
    ZXIC_UINT8 bulk_id    = 0;
    ZXIC_UINT8 table_id   = 0;
    ZXIC_UINT32 ddr_baddr = 0;
    ZXIC_UINT32 ddr_item_num = 0;
    ZXIC_UINT32 ddr_tbl_wr_mode = 0;
    ZXIC_UINT32 element_id = 0;

    DPP_SE_CFG *p_se_cfg = NULL;
    FUNC_ID_INFO *p_func_info = NULL;
    DPP_HASH_CFG *p_hash_cfg = NULL;
    HASH_DDR_CFG *p_ddr_cfg = NULL;
    DPP_SDTTBL_HASH_T sdt_hash           = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    rc = dpp_soft_sdt_tbl_get(dev,sdt_no,&sdt_hash);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_soft_sdt_tbl_get");

     /* 取出se配置 */
    rc = dpp_se_cfg_get(dev, &p_se_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_se_cfg_get");
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_se_cfg);
    p_func_info = DPP_GET_FUN_INFO(p_se_cfg, sdt_hash.hash_id);
    DPP_SE_CHECK_FUN(p_func_info, sdt_hash.hash_id, FUN_HASH);

    p_hash_cfg = (DPP_HASH_CFG *)p_func_info->fun_ptr;
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_hash_cfg);
    table_id = sdt_hash.hash_table_id;
    bulk_id = ((table_id >> 2) & 0x7);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), bulk_id, 0, HASH_BULK_NUM - 1);
    
    /*独占模式下才执行flush操作*/
    if(!p_hash_cfg->bulk_ram_mono[bulk_id])
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "fush error:hash[%u] bulk[%u] is not monopolize!\n", sdt_hash.hash_id, bulk_id);
        return DPP_HASH_RC_INVALID_PARA;
    }
    
    /*混合模式，先清除DDR空间*/
    if(p_hash_cfg->ddr_valid)
    {
        p_ddr_cfg = p_hash_cfg->p_bulk_ddr_info[bulk_id];
        ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_ddr_cfg);
        ddr_baddr = p_ddr_cfg->ddr_baddr;
        ddr_item_num = p_ddr_cfg->item_num;/*ddr存储单元数目，以DDR位宽为单位*/
        ddr_tbl_wr_mode = SMMU1_DDR_WRT_256b;
        if(DDR_WIDTH_512b == p_ddr_cfg->width_mode)
        {
           ddr_tbl_wr_mode =  SMMU1_DDR_WRT_512b;
        }

        ZXIC_COMM_TRACE_INFO("ddr_baddr is %d.\n", ddr_baddr);
        ZXIC_COMM_TRACE_INFO("ddr_item_num is %d.\n", ddr_item_num);
        ZXIC_COMM_TRACE_INFO("ddr_tbl_wr_mode is %d.\n", ddr_tbl_wr_mode);
       
        rc = dpp_dtb_smmu1_flush(dev, 
                             queue_id, 
                             ddr_baddr, 
                             ddr_tbl_wr_mode, 
                             p_ddr_cfg->ddr_ecc_en,
                             ddr_item_num, 
                             &element_id);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_smmu1_flush");
    }
    ZXIC_COMM_TRACE_INFO("dpp_dtb_hash_table_flush: DDR DONE!!!\n");
    
    /*清除zcam空间*/
    rc = dpp_dtb_specify_zcam_space_clr(dev, p_se_cfg,queue_id,sdt_hash.hash_id,bulk_id);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_specify_zcam_space_clr");
    ZXIC_COMM_TRACE_INFO("dpp_dtb_specify_zcam_space_clr DONE!!!\n");

    /*软件删除表项*/
    rc = dpp_hash_specify_entry_delete(dev,p_se_cfg,sdt_hash.hash_id,bulk_id);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_hash_specify_entry_delete");
    ZXIC_COMM_TRACE_INFO("dpp_hash_specify_entry_delete DONE!!!\n");
    
    return DPP_OK;
}

/***********************************************************/
/** 清除hash表资源(硬件和软件，硬件通过dtb方式清除)
* @param   dev_id     设备id
* @param   queue_id   队列id
* @param   hash_id    hash引擎 0~3
* @return  
* @remark  无
* @see     
* @author  cq      @date  2023/09/26
************************************************************/
DPP_STATUS dpp_dtb_hash_all_entry_delete(DPP_DEV_T *dev, ZXIC_UINT32 queue_id, ZXIC_UINT32 hash_id)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 bulk_id = 0;
    ZXIC_UINT32 ddr_baddr = 0;
    ZXIC_UINT32 ddr_item_num = 0;
    ZXIC_UINT32 ddr_tbl_wr_mode = 0;
    ZXIC_UINT32 element_id = 0;

    DPP_SE_CFG *p_se_cfg = NULL;
    FUNC_ID_INFO *p_func_info = NULL;
    DPP_HASH_CFG *p_hash_cfg = NULL;
    HASH_DDR_CFG *p_ddr_cfg = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(hash_id, 0, HASH_FUNC_ID_NUM - 1);

    /* 取出se配置 */
    rc = dpp_se_cfg_get(dev, &p_se_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_se_cfg_get");
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_se_cfg);
    p_func_info = DPP_GET_FUN_INFO(p_se_cfg, hash_id);
    DPP_SE_CHECK_FUN(p_func_info, hash_id, FUN_HASH);

    p_hash_cfg = (DPP_HASH_CFG *)p_func_info->fun_ptr;
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_hash_cfg);

    /*混合模式，先清除DDR空间*/
    if(p_hash_cfg->ddr_valid)
    {
        for(bulk_id = 0; bulk_id < HASH_BULK_NUM; bulk_id++)
        {
            p_ddr_cfg = p_hash_cfg->p_bulk_ddr_info[bulk_id];
            if(NULL==p_ddr_cfg)
            {
                continue;
            }
            ddr_baddr = p_ddr_cfg->ddr_baddr;
            ddr_item_num = p_ddr_cfg->item_num;/*ddr存储单元数目，以DDR位宽为单位*/
            ddr_tbl_wr_mode = SMMU1_DDR_WRT_256b;
            if(DDR_WIDTH_512b == p_ddr_cfg->width_mode)
            {
               ddr_tbl_wr_mode =  SMMU1_DDR_WRT_512b;
            }

            ZXIC_COMM_TRACE_INFO("ddr_baddr is %d.\n", ddr_baddr);
            ZXIC_COMM_TRACE_INFO("ddr_item_num is %d.\n", ddr_item_num);
            ZXIC_COMM_TRACE_INFO("ddr_tbl_wr_mode is %d.\n", ddr_tbl_wr_mode);
       
            rc = dpp_dtb_smmu1_flush(dev, 
                             queue_id, 
                             ddr_baddr, 
                             ddr_tbl_wr_mode, 
                             p_ddr_cfg->ddr_ecc_en,
                             ddr_item_num, 
                             &element_id);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_smmu1_flush");
        }
    }

    /*清除hash引擎占用的整个zcam空间*/
    rc = dpp_dtb_zcam_space_clr(dev, p_se_cfg, queue_id, hash_id);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_dtb_specify_zcam_space_clr");
    ZXIC_COMM_PRINT("dpp_dtb_zcam_space_clr hash id: %d DONE!!!\n", hash_id);

    /*软件删除表项*/
    rc = dpp_hash_soft_all_entry_delete(p_se_cfg, hash_id);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_hash_soft_all_entry_delete");
    ZXIC_COMM_PRINT("dpp_hash_soft_all_entry_delete hash id %d DONE!!!\n", hash_id);
    
    return DPP_OK;
}

/***********************************************************/
/** DTB etcam 整个流表清空Flush 
* @param   devId      NP设备号
* @param   queueId    DTB队列编号
* @param   sdtNo      流表std号
* @return
* @remark  无
* @see
* @author  cbb      @date  2023/07/03
************************************************************/  
DPP_STATUS dpp_dtb_etcam_table_flush(DPP_DEV_T *dev,
                                     ZXIC_UINT32 queue_id,
                                     ZXIC_UINT32 sdt_no)
{   
    // 按照std 号的深度进行etcam的内容的清理；
    DPP_STATUS  rc = DPP_OK;

    ZXIC_UINT32 data_byte_size = 0;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 etcam_key_mode = 0;
    ZXIC_UINT32 as_enable = 0;
    ZXIC_UINT32 etcam_table_depth = 0;
    ZXIC_UINT32 element_id = 0;

    DPP_SDTTBL_ETCAM_T sdt_etcam_info = {0};  /*SDT内容*/

    ZXIC_UINT8 *data_buff = NULL;
    ZXIC_UINT8 *mask_buff = NULL;
    ZXIC_UINT32 *eram_buff = NULL;
    DPP_DTB_ACL_ENTRY_INFO_T *p_entry_arr = NULL;

    ZXIC_COMM_CHECK_POINT(dev);

    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_etcam_info);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_soft_sdt_tbl_get");

    etcam_key_mode = sdt_etcam_info.etcam_key_mode;
    as_enable = sdt_etcam_info.as_en;
    etcam_table_depth = sdt_etcam_info.etcam_table_depth;

    data_byte_size = DPP_ETCAM_ENTRY_SIZE_GET(etcam_key_mode);//80/40

    //组装数据
    p_entry_arr = (DPP_DTB_ACL_ENTRY_INFO_T *)ZXIC_COMM_MALLOC(etcam_table_depth * sizeof(DPP_DTB_ACL_ENTRY_INFO_T));
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_entry_arr);
    ZXIC_COMM_MEMSET(p_entry_arr, 0, etcam_table_depth * sizeof(DPP_DTB_ACL_ENTRY_INFO_T));

    data_buff = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(data_byte_size * sizeof(ZXIC_UINT8));
    ZXIC_COMM_CHECK_DEV_POINT_MEMORY_FREE_NO_ASSERT(DEV_ID(dev), data_buff, p_entry_arr);
    ZXIC_COMM_MEMSET(data_buff, 0xFF, data_byte_size * sizeof(ZXIC_UINT8));

    mask_buff = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(data_byte_size * sizeof(ZXIC_UINT8));
    ZXIC_COMM_CHECK_DEV_POINT_MEMORY_FREE2PTR_NO_ASSERT(DEV_ID(dev), mask_buff, p_entry_arr, data_buff);
    ZXIC_COMM_MEMSET(mask_buff, 0, data_byte_size * sizeof(ZXIC_UINT8));

    if(as_enable)
    {
        eram_buff = (ZXIC_UINT32 *)ZXIC_COMM_MALLOC(4 * sizeof(ZXIC_UINT32));
        ZXIC_COMM_CHECK_DEV_POINT_MEMORY_FREE3PTR_NO_ASSERT(0, eram_buff, mask_buff, p_entry_arr, data_buff);
        ZXIC_COMM_MEMSET(eram_buff, 0, 4 * sizeof(ZXIC_UINT32));
    }

    for(index = 0; index < etcam_table_depth; index++)
    {
        p_entry_arr[index].handle = index;
        p_entry_arr[index].key_data = data_buff;
        p_entry_arr[index].key_mask = mask_buff;

        if(as_enable)
        {
            p_entry_arr[index].p_as_rslt = (ZXIC_UINT8 *)eram_buff;
        }
    }

    rc = dpp_dtb_acl_dma_insert(dev,
                            queue_id,
                            sdt_no,
                            etcam_table_depth,
                            p_entry_arr,
                            &element_id);
    ZXIC_COMM_FREE(data_buff);
    ZXIC_COMM_FREE(mask_buff);
    if(eram_buff)
    {
        ZXIC_COMM_FREE(eram_buff);
    }
    ZXIC_COMM_FREE(p_entry_arr);
    ZXIC_COMM_CHECK_DEV_RC(0, rc, "dpp_dtb_acl_dma_insert");
    
    return rc;
}

#endif

#if ZXIC_REAL("DTB DUMP INTERFACE")

/***********************************************************/
/** 根据队列号和sdt号分配元素id和dma地址
* @param   dev_id         设备id   
* @param   queue_id       队列id 0~127
* @param   sdt_no         sdt号  0~255
* @param   element_id     出参，元素id
* @return
* @remark  无
* @see
* @author  cq      @date  2023/08/22
************************************************************/  
DPP_STATUS dpp_dtb_dump_addr_set(DPP_DEV_T *dev,
                                 ZXIC_UINT32 queue_id,
                                 ZXIC_UINT32 sdt_no, 
                                 ZXIC_UINT32 *element_id)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dump_element_id = 0;
    ZXIC_UINT64 phy_addr = 0;
    ZXIC_UINT64 vir_addr = 0;
    ZXIC_UINT32 size     = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);

    /*获取dump目的地址*/
    rc = dpp_dtb_dump_sdt_addr_get( dev,
                                    queue_id,  
                                    sdt_no, 
                                    &phy_addr, 
                                    &vir_addr, 
                                    &size);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_dump_sdt_addr_get");
    ZXIC_COMM_MEMSET((ZXIC_UINT8 *)vir_addr, 0, size);
    
    /*获取队列可用元素*/
    rc = dpp_dtb_tab_up_free_item_get(dev, queue_id, &dump_element_id);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_tab_up_free_item_get");
    ZXIC_COMM_TRACE_DEV_INFO(DEV_ID(dev), "table up item queue_element_id is: %d.\n",dump_element_id);

    /*在相应队列的元素上配置用户dma地址信息*/
    rc = dpp_dtb_tab_up_item_user_addr_set(dev,
                                           queue_id,
                                           dump_element_id,
                                           phy_addr,
                                           vir_addr);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_tab_up_item_addr_set"); 

    *element_id = dump_element_id;

    return DPP_OK;
}

/***********************************************************/
/** 将dump数据解析为hash条目格式
* @param   p_hash_entry_cfg  hash配置参数   
* @param   item_type         条目类型,见枚举SE_ITEM_TYPE
* @param   pdata             dump数据
* @param   dump_len          dump有效长度
* @param   pOutData          出参，连续内存，hash条目信息DPP_HASH_ENTRY
* @param   p_item_num        出参，解析出的hash有效条目数
* @return
* @remark  无
* @see
* @author  cq      @date  2023/08/22
************************************************************/  
DPP_STATUS dpp_dtb_dump_hash_parse(HASH_ENTRY_CFG *p_hash_entry_cfg,
                                    ZXIC_UINT32 item_type,
                                    ZXIC_UINT8 *pdata,
                                    ZXIC_UINT32 dump_len,
                                    ZXIC_UINT8 *pOutData,
                                    ZXIC_UINT32 *p_item_num)
{
    ZXIC_UINT32 item_num = 0;
    ZXIC_UINT32 data_offset = 0;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT8 temp_key_valid = 0;
    ZXIC_UINT8 temp_key_type = 0;
    ZXIC_UINT8 temp_tbl_id   = 0;
    ZXIC_UINT32 srh_entry_size = 0;
    ZXIC_UINT32 item_width = SE_ITEM_WIDTH_MAX;
    ZXIC_UINT8 *p_temp_key = NULL;
    ZXIC_UINT8 *p_hash_item = NULL;
    DPP_HASH_ENTRY  *p_dtb_hash_entry = NULL;
    DPP_HASH_ENTRY  *p_temp_entry = NULL;
    
    ZXIC_COMM_CHECK_POINT(p_hash_entry_cfg);
    ZXIC_COMM_CHECK_POINT(pdata);
    ZXIC_COMM_CHECK_POINT(pOutData);
    ZXIC_COMM_CHECK_POINT(p_item_num);

    if (ITEM_DDR_256 == item_type)
    {
        item_width = item_width / 2;
    }

    p_dtb_hash_entry = (DPP_HASH_ENTRY *)pOutData;
    srh_entry_size = DPP_GET_HASH_ENTRY_SIZE(p_hash_entry_cfg->key_type);
    if(srh_entry_size == 0)
    {
        ZXIC_COMM_TRACE_ERROR("key type %d srh entry size is zero\n", p_hash_entry_cfg->key_type);
        return DPP_ERR;
    }
    
    for(index = 0; index < (dump_len / item_width); index++)
    {
        data_offset = 0;
        p_hash_item = pdata + index * item_width;
        while(data_offset < item_width)
        {
            p_temp_key = p_hash_item + data_offset;
            temp_key_valid = DPP_GET_HASH_KEY_VALID(p_temp_key);
            temp_key_type = DPP_GET_HASH_KEY_TYPE(p_temp_key);
            temp_tbl_id = DPP_GET_HASH_TBL_ID(p_temp_key);
            if(temp_key_valid && (temp_key_type == p_hash_entry_cfg->key_type) 
               && (temp_tbl_id == p_hash_entry_cfg->table_id))
            {
                p_temp_entry = p_dtb_hash_entry + item_num;
                ZXIC_COMM_CHECK_POINT(p_temp_entry);
                ZXIC_COMM_CHECK_POINT(p_temp_entry->p_key);
                ZXIC_COMM_CHECK_POINT(p_temp_entry->p_rst);
                ZXIC_COMM_MEMCPY(p_temp_entry->p_key, p_temp_key, p_hash_entry_cfg->key_by_size + 1); 
                ZXIC_COMM_MEMCPY(p_temp_entry->p_rst, p_temp_key + 1 + p_hash_entry_cfg->key_by_size, p_hash_entry_cfg->rst_by_size);
                item_num++;
            }
            
            data_offset += srh_entry_size;
        }
    }

    *p_item_num = item_num;
    return DPP_OK;
}

/** dtb dump eram整个表项内容 输入128bit单位index，输出128bit为单位的数据
* @param   dev_id      设备号
* @param   queue_id    队列号
* @param   sdt_no      eram表sdt表号
* @param   start_index 要dump的起始index 单位是128bit
* @param   depth       dump的深度以128bit为单位
* @param   p_data      dump出数据缓存(大小128bit * depth)
* @param   element_id   返回下表使用的元素id
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
DPP_STATUS dpp_dtb_sdt_eram_table_dump(DPP_DEV_T *dev, 
                                      ZXIC_UINT32 queue_id, 
                                      ZXIC_UINT32 sdt_no, 
                                      ZXIC_UINT32 start_index, 
                                      ZXIC_UINT32 depth,
                                      ZXIC_UINT32 *p_data,
                                      ZXIC_UINT32 *element_id
                                      )
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 eram_base_addr  = 0;
    ZXIC_UINT32 dump_addr_128bit = 0;
    ZXIC_UINT32 dump_item_index = 0;
    ZXIC_UINT32 dump_data_len = 0;
    ZXIC_UINT32 dump_desc_len = 0;

    ZXIC_ADDR_T dump_sdt_phy_addr = 0;
    ZXIC_ADDR_T dump_sdt_vir_addr = 0;
    ZXIC_UINT32 dump_addr_size = 0;

    ZXIC_UINT32 dump_dst_phy_haddr = 0;
    ZXIC_UINT32 dump_dst_phy_laddr = 0;

    ZXIC_UINT8 form_buff[DTB_TABLE_CMD_SIZE_BIT / 8] = {0};

    /*获取sdt信息*/
    DPP_SDTTBL_ERAM_T sdt_eram    = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), element_id);

    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_eram);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_soft_sdt_tbl_get");

    eram_base_addr = sdt_eram.eram_base_addr;
    dump_addr_128bit = eram_base_addr + start_index;

    //获取dump目的地址
    rc = dpp_dtb_dump_sdt_addr_get( dev,
                                    queue_id,  
                                    sdt_no, 
                                    &dump_sdt_phy_addr, 
                                    &dump_sdt_vir_addr, 
                                    &dump_addr_size);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_dump_sdt_addr_get"); 

    ZXIC_COMM_MEMSET((ZXIC_UINT8 *)dump_sdt_vir_addr, 0, dump_addr_size);

    rc = dpp_dtb_tab_up_free_item_get(dev, queue_id, &dump_item_index);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_tab_up_free_item_get");
    ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "dump queue id %d, element_id is: %d.\n",queue_id, dump_item_index);

    *element_id = dump_item_index;//保存获取的item_index

    ZXIC_COMM_TRACE_INFO("eram dump eram_base_addr %x\n",eram_base_addr);
    ZXIC_COMM_TRACE_INFO("eram dump start_index %x\n",start_index);
    ZXIC_COMM_TRACE_INFO("eram dump queue %d,item_index: %d\n",queue_id, dump_item_index);

    //在相应队列的元素上配置用户dma地址信息
    rc = dpp_dtb_tab_up_item_user_addr_set(dev,
                                           queue_id,
                                           dump_item_index,
                                           dump_sdt_phy_addr,
                                           dump_sdt_vir_addr);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_tab_up_item_addr_set"); 

    /*获取地址*/
    rc = dpp_dtb_tab_up_item_addr_get(dev, queue_id, dump_item_index, &dump_dst_phy_haddr, &dump_dst_phy_laddr);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_tab_up_item_addr_get");
    
    rc = dpp_dtb_smmu0_dump_info_write(dev,
                                       dump_addr_128bit,
                                       depth,
                                       dump_dst_phy_haddr,
                                       dump_dst_phy_laddr,
                                       (ZXIC_UINT32 *)form_buff);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_smmu0_dump_info_write");

    /*组装下表命令格式*/
    dump_data_len = depth * 128 / 32;
    dump_desc_len = DTB_LEN_POS_SETP / 4;

    if(dump_data_len * 4 > dump_addr_size)
    {
        ZXIC_COMM_TRACE_ERROR("eram dump size is too small!\n");
        return DPP_RC_DTB_DUMP_SIZE_SMALL;
    }

    rc = dpp_dtb_write_dump_desc_info(dev, queue_id, dump_item_index, (ZXIC_UINT32 *)form_buff, dump_data_len, dump_desc_len, p_data);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_write_dump_desc_info");

    return DPP_OK;
}

/** dtb dump eram直接表表项内容 支持64bit/128bit
* @param   dev_id    设备号
* @param   queue_id  队列号
* @param   sdt_no    eram表sdt表号
* @param   start_index 要dump的起始index，单位是sdt_no该表的mode
* @param   p_dump_data_arr 本次dump出的数据，数据格式与下表格式相同
* @param   entry_num       本次dump实际的条目数
* @param   next_start_index  下次dump是开始的index  
* @param   finish_flag       整个表dump完成标志，1表示完成，0表示未完成
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
 DPP_STATUS dpp_dtb_eram_table_dump(DPP_DEV_T *dev,
                                    ZXIC_UINT32 queue_id,
                                    ZXIC_UINT32 sdt_no,
                                    DPP_DTB_DUMP_INDEX_T start_index,
                                    DPP_DTB_ERAM_ENTRY_INFO_T* p_dump_data_arr,
                                    ZXIC_UINT32 *entry_num,
                                    DPP_DTB_DUMP_INDEX_T *next_start_index,
                                    ZXIC_UINT32 *finish_flag)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 dump_mode = 0;
    ZXIC_UINT32 eram_table_depth = 0;
    ZXIC_UINT32 start_index_128bit = 0;
    ZXIC_UINT32 row_index = 0;
    ZXIC_UINT32 col_index = 0;
    ZXIC_UINT32 dump_depth_128bit = 0;
    ZXIC_UINT32 dump_depth = 0;
    ZXIC_UINT32 element_id = 0;
    ZXIC_UINT8* dump_data_buff = NULL;
    ZXIC_UINT8* temp_data = NULL;
    ZXIC_UINT32 remain = 0;
    ZXIC_UINT32 *buff = NULL;

    DPP_DTB_ERAM_ENTRY_INFO_T* p_dump_user_data = NULL;
    DPP_SDTTBL_ERAM_T sdt_eram    = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_eram);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_soft_sdt_tbl_get");

    dump_mode = sdt_eram.eram_mode;//0:1bit;2:64bit;3:128,
    eram_table_depth = sdt_eram.eram_table_depth;

    rc = dtb_eram_index_cal(dev, dump_mode, eram_table_depth, &dump_depth_128bit, &col_index);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dtb_eram_index_cal");

    rc = dtb_eram_index_cal(dev, dump_mode, start_index.index, &start_index_128bit, &col_index);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dtb_eram_index_cal");

    dump_depth = dump_depth_128bit - start_index_128bit;
    
    dump_data_buff = (ZXIC_UINT8 *) ZXIC_COMM_VMALLOC(dump_depth * DTB_LEN_POS_SETP);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), dump_data_buff);
    ZXIC_COMM_MEMSET(dump_data_buff, 0, dump_depth * DTB_LEN_POS_SETP);

    rc = dpp_dtb_sdt_eram_table_dump(dev, 
                                queue_id, 
                                sdt_no, 
                                start_index_128bit, 
                                dump_depth, 
                                (ZXIC_UINT32 *)dump_data_buff, 
                                &element_id);
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE_NO_ASSERT(rc, "dpp_dtb_sdt_eram_table_dump", dump_data_buff);
    ZXIC_COMM_TRACE_INFO(" dpp_dtb_sdt_eram_table_dump done queue %d element %d.\n", queue_id, element_id);

    if (dump_mode == ERAM128_TBL_128b)
    {
        for(i = 0; i < dump_depth; i++)
        {
            p_dump_user_data = p_dump_data_arr + i;
            temp_data = dump_data_buff + i * DTB_LEN_POS_SETP;
            if ((p_dump_user_data == NULL) || (p_dump_user_data->p_data == NULL))
            {
                ZXIC_COMM_TRACE_ERROR("eram index 0x%x data user buff is NULL!\n",start_index.index + i);
                ZXIC_COMM_VFREE(dump_data_buff);
                return DPP_ERR;
            }
            
            p_dump_user_data->index = start_index.index + i;
            ZXIC_COMM_MEMCPY(p_dump_user_data->p_data, temp_data, (128 / 8));
        }
    }
    else if(dump_mode == ERAM128_TBL_64b)
    {
        remain =  start_index.index % 2;
        for(i = 0; i < eram_table_depth - start_index.index; i++)
        {
            rc = dtb_eram_index_cal(dev, dump_mode, remain, &row_index, &col_index);
            ZXIC_COMM_CHECK_RC_MEMORY_VFREE_NO_ASSERT(rc, "dtb_eram_index_cal", dump_data_buff);
            temp_data = dump_data_buff + row_index * DTB_LEN_POS_SETP;

            buff = (ZXIC_UINT32*)temp_data;
            p_dump_user_data = p_dump_data_arr + i;
            if (p_dump_user_data->p_data == NULL)
            {
                ZXIC_COMM_TRACE_ERROR("eram index 0x%x data point is NULL!\n",start_index.index + i);
                ZXIC_COMM_VFREE(dump_data_buff);
                return DPP_ERR;
            }

            p_dump_user_data->index = start_index.index + i;
            ZXIC_COMM_MEMCPY(p_dump_user_data->p_data, buff + ((1 - col_index) << 1), (64 / 8));

            remain ++;
        }
    }
    
    *entry_num = eram_table_depth - start_index.index;
    *finish_flag = 1;
    ZXIC_COMM_TRACE_INFO(" eram table dump entry num %d, finish flag %d\n\n", *entry_num, *finish_flag);

    ZXIC_COMM_VFREE(dump_data_buff);

    return DPP_OK;
}

/***********************************************************/
/** dump eram表内容
* @param   dev            设备
* @param   queue_id       队列id 0~127
* @param   sdt_no         sdt号  0~255
* @param   pDumpData      出参，dump数据，内存由用户分配，结构体DPP_HASH_ENTRY
* @param   entryNum       出参，dump出的有效hash条目
* @return
* @remark  无
* @see
* @author  cq      @date  2025/04/03
************************************************************/
DPP_STATUS dpp_dtb_eram_dump(DPP_DEV_T *dev,
                                ZXIC_UINT32 queue_id,
                                ZXIC_UINT32 sdt_no,
                                ZXIC_UINT8  *pDumpData,
                                ZXIC_UINT32 *p_entry_num)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT64 dma_phy_addr = 0;
    ZXIC_UINT64 dma_vir_addr = 0;
    ZXIC_UINT32 dma_size = DTB_SDT_DUMP_SIZE/2;
    ZXIC_MUTEX_T *p_mutex = NULL;
    DPP_DTB_DUMP_INDEX_T start_index = {0};
    DPP_DTB_DUMP_INDEX_T next_start_index = {0};
    ZXIC_UINT32 finish_flag = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(pDumpData);
    ZXIC_COMM_CHECK_POINT(p_entry_num);

    rc = dpp_dev_dtb_opr_mutex_get(dev, DPP_DEV_MUTEX_T_DTB_RB, queue_id, &p_mutex);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_dtb_opr_mutex_get");
    rc = zxic_comm_mutex_lock(p_mutex);
    ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_lock");

    rc = dpp_dev_dump_dma_mem_get(dev, &dma_size, &dma_phy_addr, &dma_vir_addr);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dev_dump_dma_mem_get", p_mutex);

    rc = dpp_dtb_dump_sdt_addr_set(dev, queue_id, sdt_no, dma_phy_addr, dma_vir_addr, dma_size);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_dump_sdt_addr_set", p_mutex);

    start_index.index = 0;
    rc = dpp_dtb_eram_table_dump(dev, queue_id, sdt_no, start_index, (DPP_DTB_ERAM_ENTRY_INFO_T *)pDumpData,
                                                              p_entry_num, &next_start_index, &finish_flag);
    rc |= dpp_dtb_dump_sdt_addr_clear(dev, queue_id, sdt_no);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_eram_table_dump", p_mutex);

    rc = zxic_comm_mutex_unlock(p_mutex);
    ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_unlock");

    return DPP_OK;
}

/***********************************************************/
/** dump指定hash表占用的zcam数据
* @param   dev_id         设备id   
* @param   queue_id       队列id 0~127
* @param   sdt_no         sdt号  0~255
* @param   fun_id         需dump的zblock个数
* @param   bulk_id        zblock索引
* @param   p_data         出参，dump数据
* @param   p_dump_len     出参，dump长度
* @return
* @remark  无
* @see
* @author  cq      @date  2023/08/22
************************************************************/  
DPP_STATUS dpp_dtb_sdt_hash_zcam_mono_space_dump(DPP_DEV_T *dev,
                                                ZXIC_UINT32 queue_id,
                                                ZXIC_UINT32 sdt_no, 
                                                ZXIC_UINT32 fun_id,
                                                ZXIC_UINT32 bulk_id,
                                                ZXIC_UINT8  *p_data,
                                                ZXIC_UINT32 *p_dump_len)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 zblock_id = 0;
    ZXIC_UINT32 zcell_id   = 0;
    ZXIC_UINT32 start_addr  = 0;
    ZXIC_UINT32 dtb_desc_len = 0;
    ZXIC_UINT32 dump_pa_h   = 0;
    ZXIC_UINT32 dump_pa_l   = 0;
    ZXIC_UINT32 dma_addr_offset = 0;
    ZXIC_UINT32 desc_addr_offset = 0;
    ZXIC_UINT32 element_id  = 0;
    ZXIC_UINT8  *p_dump_desc_buf = NULL;

    DPP_SE_CFG *p_se_cfg = NULL;
    D_NODE *p_zblk_dn = NULL;
    SE_ZBLK_CFG *p_zblk = NULL;
    SE_ZREG_CFG *p_zreg = NULL;
    SE_ZCELL_CFG *p_zcell = NULL;
    DPP_HASH_CFG *p_hash_cfg = NULL;
    FUNC_ID_INFO *p_func_info = NULL;

    DPP_DTB_ENTRY_T   dtb_dump_entry = {0};
    ZXIC_UINT8 cmd_buff[DTB_TABLE_CMD_SIZE_BIT / 8] = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), fun_id, 0, HASH_FUNC_ID_NUM - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), bulk_id, 0, HASH_BULK_NUM - 1);
    ZXIC_COMM_CHECK_INDEX(queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_POINT(p_data);
    ZXIC_COMM_CHECK_POINT(p_dump_len);

    rc = dpp_dtb_dump_addr_set(dev, queue_id, sdt_no, &element_id);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_dump_addr_set");

    p_dump_desc_buf = (ZXIC_UINT8 *) ZXIC_COMM_MALLOC(DPP_DTB_TABLE_DUMP_INFO_BUFF_SIZE * sizeof(ZXIC_UINT8));
    ZXIC_COMM_CHECK_POINT(p_dump_desc_buf);
    ZXIC_COMM_MEMSET(p_dump_desc_buf, 0, DPP_DTB_TABLE_DUMP_INFO_BUFF_SIZE * sizeof(ZXIC_UINT8));

    dtb_dump_entry.cmd = cmd_buff;

    rc = dpp_se_cfg_get(dev, &p_se_cfg);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE_NO_ASSERT(DEV_ID(dev), rc, "dpp_se_cfg_get", p_dump_desc_buf);
    ZXIC_COMM_CHECK_DEV_POINT_MEMORY_FREE_NO_ASSERT(DEV_ID(dev), p_se_cfg, p_dump_desc_buf);
    
    p_func_info = DPP_GET_FUN_INFO(p_se_cfg, fun_id);
    DPP_SE_CHECK_FUN_MEMORY_FREE(p_func_info, fun_id, FUN_HASH, p_dump_desc_buf);
    p_hash_cfg = (DPP_HASH_CFG *)p_func_info->fun_ptr;
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_hash_cfg);
    p_zblk_dn = p_hash_cfg->hash_shareram.zblk_list.p_next;

    while (p_zblk_dn)
    {
        p_zblk = (SE_ZBLK_CFG *)p_zblk_dn->data;
        zblock_id = p_zblk->zblk_idx;

        //mono zcell dump
        for (i = 0; i < SE_ZCELL_NUM; i++)
        {
            p_zcell = &(p_zblk->zcell_info[i]);

            if ((p_zcell->flag & DPP_ZCELL_FLAG_IS_MONO) && (p_zcell->bulk_id == bulk_id))
            {
                zcell_id = p_zcell->zcell_idx;

                start_addr = ZBLK_ITEM_ADDR_CALC(zcell_id, 0);

                /*获取dma上送指定条目的物理地址*/
                rc = dpp_dtb_tab_up_item_offset_addr_get(dev,
                                                         queue_id,
                                                         element_id,
                                                         dma_addr_offset,
                                                         &dump_pa_h,
                                                         &dump_pa_l);
                ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_tab_up_item_offset_addr_get",p_dump_desc_buf);

                rc = dpp_dtb_zcam_dump_entry(dev, start_addr, DTB_DUMP_ZCAM_512b, SE_RAM_DEPTH, dump_pa_h, dump_pa_l, &dtb_dump_entry);
                ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_zcam_dump_entry",p_dump_desc_buf);

                rc = dpp_dtb_data_write(p_dump_desc_buf, desc_addr_offset, &dtb_dump_entry);
                ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_zcam_dump_entry",p_dump_desc_buf);

                dtb_desc_len++;
                dma_addr_offset += SE_RAM_DEPTH * 512 / 8;
                desc_addr_offset += DTB_LEN_POS_SETP;

                ZXIC_COMM_TRACE_DEV_INFO(DEV_ID(dev), "the Zblock[%d]'s bulk_id:%d Mono Zcell_id :%d \n", zblock_id, bulk_id, zcell_id);
            }
        }

        //mono zreg dump
        for (i = 0; i < SE_ZREG_NUM; i++)
        {
            p_zreg = &(p_zblk->zreg_info[i]);

            if ((p_zreg->flag & DPP_ZREG_FLAG_IS_MONO) && (p_zreg->bulk_id == bulk_id))
            {   
                start_addr = ZBLK_HASH_LIST_REG_ADDR_CALC(zblock_id, i);

                rc = dpp_dtb_tab_up_item_offset_addr_get(dev,
                                                        queue_id,
                                                        element_id,
                                                        dma_addr_offset,
                                                        &dump_pa_h,
                                                        &dump_pa_l);
                ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_tab_up_item_offset_addr_get",p_dump_desc_buf);

                rc = dpp_dtb_zcam_dump_entry(dev, start_addr, DTB_DUMP_ZCAM_512b, 1, dump_pa_h, dump_pa_l, &dtb_dump_entry);
                ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_zcam_dump_entry",p_dump_desc_buf);

                rc = dpp_dtb_data_write(p_dump_desc_buf, desc_addr_offset, &dtb_dump_entry);
                ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, "dpp_dtb_zcam_dump_entry",p_dump_desc_buf);

                dtb_desc_len++;
                dma_addr_offset += 512 / 8;
                desc_addr_offset += DTB_LEN_POS_SETP;

                ZXIC_COMM_TRACE_DEV_INFO(DEV_ID(dev), "the Zblock[%d]'s bulk_id:%d Mono Zreg_id :%d \n", zblock_id, p_zreg->bulk_id, i);
            }
        }

        p_zblk_dn = p_zblk_dn->next;
    }

    rc = dpp_dtb_write_dump_desc_info(dev,
                                      queue_id,
                                      element_id,
                                      (ZXIC_UINT32 *)p_dump_desc_buf,
                                      dma_addr_offset / 4,
                                      dtb_desc_len * 4,
                                      (ZXIC_UINT32 *)p_data);
    ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc,"dpp_dtb_write_dump_desc_info",p_dump_desc_buf);
    ZXIC_COMM_TRACE_INFO(" dpp_dtb_hash_table_zcam_dump done queue %d element %d.\n", queue_id, element_id);
    
    zxic_comm_swap(p_data, dma_addr_offset);
    ZXIC_COMM_FREE(p_dump_desc_buf);
    
    *p_dump_len = dma_addr_offset;
    
    return DPP_OK;
}

/***********************************************************/
/** 只dump hash表的zcam内容
* @param   dev_id         设备id   
* @param   queue_id       队列id 0~127
* @param   sdt_no         sdt号  0~255
* @param   pDumpData      出参，dump数据，内存由用户分配，结构体DPP_HASH_ENTRY
* @param   entryNum       出参，dump出的有效hash条目
* @return
* @remark  无
* @see
* @author  cq      @date  2023/08/22
************************************************************/  
DPP_STATUS dpp_dtb_hash_table_only_zcam_dump(DPP_DEV_T *dev,
                                            ZXIC_UINT32 queue_id,
                                            ZXIC_UINT32 sdt_no,
                                            ZXIC_UINT8* pDumpData,
                                            ZXIC_UINT32 *entryNum)
{
    DPP_STATUS  rc = DPP_OK;

    ZXIC_UINT8 *p_data = NULL;
    ZXIC_UINT32 data_len = 0;
    ZXIC_UINT32 entry_num = 0;
    ZXIC_UINT32 bulk_id = 0;
    DPP_SE_CFG *p_se_cfg = NULL;
    FUNC_ID_INFO *p_func_info = NULL;
    DPP_HASH_CFG *p_hash_cfg = NULL;
    DPP_SDTTBL_HASH_T sdt_hash           = {0};
    HASH_ENTRY_CFG  hash_entry_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(queue_id, 0, DTB_QUEUE_MAX - 1);
    ZXIC_COMM_CHECK_INDEX(sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_POINT(pDumpData);
    ZXIC_COMM_CHECK_POINT(entryNum);

    ZXIC_COMM_TRACE_INFO("dump hash sdt no: %d\n", sdt_no);

    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_hash);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_soft_sdt_tbl_get");

     /* 取出se配置 */
    rc = dpp_se_cfg_get(dev, &p_se_cfg);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(DEV_ID(dev), rc, "dpp_se_cfg_get");
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_se_cfg);
    p_func_info = DPP_GET_FUN_INFO(p_se_cfg, sdt_hash.hash_id);
    DPP_SE_CHECK_FUN(p_func_info, sdt_hash.hash_id, FUN_HASH);
    p_hash_cfg = (DPP_HASH_CFG *)p_func_info->fun_ptr;
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_hash_cfg);

    ZXIC_COMM_MEMSET(&hash_entry_cfg, 0x0, sizeof(HASH_ENTRY_CFG));
    hash_entry_cfg.key_by_size = sdt_hash.key_size;
    hash_entry_cfg.key_type = sdt_hash.hash_table_width;
    hash_entry_cfg.rst_by_size = 1 << (sdt_hash.rsp_mode + 2);
    hash_entry_cfg.table_id = sdt_hash.hash_table_id;
    bulk_id = ((hash_entry_cfg.table_id >> 2) & 0x7);
    ZXIC_COMM_CHECK_INDEX_UPPER(bulk_id, HASH_BULK_NUM - 1);

    //申请data空间
    p_data = (ZXIC_UINT8 *)ZXIC_COMM_VMALLOC(DTB_DMUP_DATA_MAX);
    ZXIC_COMM_CHECK_POINT(p_data);
    ZXIC_COMM_MEMSET_S(p_data, DTB_DMUP_DATA_MAX, 0, DTB_DMUP_DATA_MAX);

    rc = dpp_dtb_sdt_hash_zcam_mono_space_dump(dev,
                                                queue_id,
                                                sdt_no, 
                                                sdt_hash.hash_id,
                                                bulk_id,
                                                p_data,
                                                &data_len);
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE_NO_ASSERT(rc, "dpp_dtb_sdt_hash_zcam_mono_space_dump", p_data);

    rc = dpp_dtb_dump_hash_parse(&hash_entry_cfg,
                                ITEM_RAM,
                                p_data,
                                data_len,
                                pDumpData,
                                &entry_num);
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE_NO_ASSERT(rc, "dpp_dtb_dump_hash_parse",p_data);

    *entryNum = entry_num;

    ZXIC_COMM_TRACE_INFO("hash table dump entry num %d end.\n\n", *entryNum);

    ZXIC_COMM_VFREE(p_data);

    return rc;
}

/***********************************************************/
/** dump hash表内容
* @param   dev            设备   
* @param   queue_id       队列id 0~127
* @param   sdt_no         sdt号  0~255
* @param   pDumpData      出参，dump数据，内存由用户分配，结构体DPP_HASH_ENTRY
* @param   entryNum       出参，dump出的有效hash条目
* @return
* @remark  无
* @see
* @author  cq      @date  2025/04/03
************************************************************/  
DPP_STATUS dpp_dtb_hash_dump(DPP_DEV_T *dev, 
                                ZXIC_UINT32 queue_id, 
                                ZXIC_UINT32 sdt_no, 
                                ZXIC_UINT8  *pDumpData, 
                                ZXIC_UINT32 *p_entry_num)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT64 dma_phy_addr = 0;
    ZXIC_UINT64 dma_vir_addr = 0;
    ZXIC_UINT32 dma_size = DTB_SDT_DUMP_SIZE;
    ZXIC_MUTEX_T *p_mutex = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(pDumpData);
    ZXIC_COMM_CHECK_POINT(p_entry_num);

    rc = dpp_dev_dtb_opr_mutex_get(dev, DPP_DEV_MUTEX_T_DTB_RB, queue_id, &p_mutex);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_dtb_opr_mutex_get");
    rc = zxic_comm_mutex_lock(p_mutex);
    ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_lock");

    rc = dpp_dev_dump_dma_mem_get(dev, &dma_size, &dma_phy_addr, &dma_vir_addr);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dev_dump_dma_mem_get",p_mutex);

    rc = dpp_dtb_dump_sdt_addr_set(dev, queue_id, sdt_no, dma_phy_addr, dma_vir_addr, dma_size);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_dump_sdt_addr_set", p_mutex);

    rc = dpp_dtb_hash_table_only_zcam_dump(dev, queue_id, sdt_no, pDumpData, p_entry_num);
    rc |= dpp_dtb_dump_sdt_addr_clear(dev, queue_id, sdt_no);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_hash_table_only_zcam_dump", p_mutex);
    rc = zxic_comm_mutex_unlock(p_mutex);
    ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_unlock");

    return DPP_OK;
}

/** dtb dump etcam直接表表项内容 级联eram支持64bit/128bit
* @param   dev_id             设备号
* @param   queue_id           队列号
* @param   sdt_no             acl表sdt表号
* @param   start_index        要dump的起始index，单位是sdt_no该表的mode
* @param   p_dump_data_arr    本次dump出的数据，数据格式与下表格式相同
* @param   entry_num          本次dump实际的条目数
* @param   next_start_index   下次dump是开始的index  
* @param   finish_flag        整个表dump完成标志，1表示完成，0表示未完成
* @return
* @remark  无
* @see
* @author  cbb      @date  2022/08/30
************************************************************/
 DPP_STATUS dpp_dtb_acl_table_dump(DPP_DEV_T *dev,
                               ZXIC_UINT32 queue_id,
                               ZXIC_UINT32 sdt_no,
                               DPP_DTB_DUMP_INDEX_T start_index,
                               DPP_DTB_ACL_ENTRY_INFO_T* p_dump_data_arr,
                               ZXIC_UINT32 *entry_num,
                               DPP_DTB_DUMP_INDEX_T *next_start_index,
                               ZXIC_UINT32 *finish_flag)
{
    DPP_STATUS  rc = DPP_OK;

    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 handle = 0;

    ZXIC_UINT32 dump_element_id = 0;

    ZXIC_UINT8 *temp_dump_out_data = NULL;
    ZXIC_UINT8 *dump_info_buff = NULL;
    ZXIC_UINT8 *p_data_start = NULL;
    ZXIC_UINT8 *p_data_640bit = NULL;
    ZXIC_UINT8 *p_mask_start = NULL;
    ZXIC_UINT8 *p_mask_640bit = NULL;
    ZXIC_UINT8 *p_rst_start = NULL;
    ZXIC_UINT8 *p_rst_128bit = NULL;
    ZXIC_UINT32 *eram_buff = NULL;


    ZXIC_UINT32 addr_640bit = 0;
    ZXIC_UINT32 rd_mask = 0;
    ZXIC_UINT32 dump_eram_depth_128bit = 0;
    ZXIC_UINT32 eram_row_index = 0;
    ZXIC_UINT32 eram_col_index = 0;

    ZXIC_UINT8 cmd_buff[DTB_TABLE_CMD_SIZE_BIT / 8] = {0};
    ZXIC_UINT8 xy_data[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 xy_mask[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 dm_data[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    ZXIC_UINT8 dm_mask[DPP_ETCAM_WIDTH_MAX / 8] = {0};
    DPP_ETCAM_ENTRY_T entry_xy = {0};
    DPP_ETCAM_ENTRY_T entry_dm = {0};
    DPP_DTB_ACL_ENTRY_INFO_T* p_dump_user_data = NULL;

    ZXIC_UINT32 block_num = 0;
    ZXIC_UINT32 etcam_key_mode = 0;
    ZXIC_UINT32 etcam_table_id = 0;
    ZXIC_UINT32 as_enable = 0;
    ZXIC_UINT32 as_eram_baddr = 0;
    ZXIC_UINT32 etcam_as_mode = 0;
    ZXIC_UINT32 etcam_table_depth = 0;
    ZXIC_UINT32 block_idx = 0;

    ZXIC_UINT32 etcam_data_dst_phy_haddr = 0;
    ZXIC_UINT32 etcam_data_dst_phy_laddr = 0;
    ZXIC_UINT32 etcam_mask_dst_phy_haddr = 0;
    ZXIC_UINT32 etcam_mask_dst_phy_laddr = 0;
    ZXIC_UINT32 as_rst_dst_phy_haddr = 0;
    ZXIC_UINT32 as_rst_dst_phy_laddr = 0;

    ZXIC_UINT32 dtb_desc_addr_offset = 0;
    ZXIC_UINT32 dump_data_len = 0;
    ZXIC_UINT32 dtb_desc_len = 0;

    ZXIC_UINT32 etcam_data_len_offset = 0;
    ZXIC_UINT32 etcam_mask_len_offset = 0;

    DPP_ACL_CFG_EX_T *p_acl_cfg = NULL;
    DPP_ACL_TBL_CFG_T *p_tbl_cfg = NULL;

    DPP_SDTTBL_ETCAM_T sdt_etcam_info = {0};  /*SDT内容*/
    ETCAM_DUMP_INFO_T etcam_dump_info = {0};
    DPP_DTB_ENTRY_T   dtb_dump_entry = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    dtb_dump_entry.cmd = cmd_buff;
    entry_xy.p_data = xy_data;
    entry_xy.p_mask = xy_mask;
    entry_dm.p_data = dm_data;
    entry_dm.p_mask = dm_mask;

    //从sdt_no中获取SDT配置
    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_etcam_info);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_soft_sdt_tbl_get");
    etcam_key_mode = sdt_etcam_info.etcam_key_mode;
    etcam_as_mode = sdt_etcam_info.as_rsp_mode;
    etcam_table_id = sdt_etcam_info.etcam_table_id;
    as_enable = sdt_etcam_info.as_en;
    as_eram_baddr = sdt_etcam_info.as_eram_baddr;
    etcam_table_depth = sdt_etcam_info.etcam_table_depth;

    rc = dpp_acl_cfg_get(dev,&p_acl_cfg);//获取ACL表资源配置
    ZXIC_COMM_CHECK_RC(rc, "dpp_acl_cfg_get");
    ZXIC_COMM_CHECK_POINT(p_acl_cfg);

    p_tbl_cfg = p_acl_cfg->acl_tbls + etcam_table_id;//得到该acl表的资源配置

    if (!p_tbl_cfg->is_used)
    {
        ZXIC_COMM_TRACE_ERROR("table[ %d ] is not init!\n", etcam_table_id);
        ZXIC_COMM_ASSERT(0);
        return DPP_ACL_RC_TBL_NOT_INIT;
    }

    block_num = p_tbl_cfg->block_num;

    rc = dpp_dtb_dump_addr_set(dev, queue_id, sdt_no, &dump_element_id);
    ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "dpp_dtb_dump_addr_set");

    ZXIC_COMM_TRACE_INFO("etcam_key_mode: 0x%x\n", etcam_key_mode);
    ZXIC_COMM_TRACE_INFO("etcam_table_id: 0x%x\n", etcam_table_id);
    ZXIC_COMM_TRACE_INFO("as_enable: 0x%x\n", as_enable);
    ZXIC_COMM_TRACE_INFO("as_eram_baddr: 0x%x\n", as_eram_baddr);
    ZXIC_COMM_TRACE_INFO("etcam_as_mode: 0x%x\n", etcam_as_mode);
    ZXIC_COMM_TRACE_INFO("block_num: 0x%x\n", block_num);
    ZXIC_COMM_TRACE_INFO("dump_element_id: 0x%x\n", dump_element_id);

    dump_info_buff = (ZXIC_UINT8 *) ZXIC_COMM_VMALLOC(DPP_DTB_TABLE_DUMP_INFO_BUFF_SIZE * sizeof(ZXIC_UINT8));
    ZXIC_COMM_CHECK_POINT_NO_ASSERT(dump_info_buff);
    ZXIC_COMM_MEMSET(dump_info_buff, 0, DPP_DTB_TABLE_DUMP_INFO_BUFF_SIZE * sizeof(ZXIC_UINT8));

    //etcam  data dump描述符，以block为单位，深度是512 起始地址是0  rd_mode = 0xff
    for(i = 0; i < block_num; i++)
    {
        block_idx = p_tbl_cfg->block_array[i];

        ZXIC_COMM_TRACE_INFO("block_idx: %d\n", block_idx);

        etcam_dump_info.block_sel = block_idx;
        etcam_dump_info.addr = 0;
        etcam_dump_info.tb_width = 3;
        etcam_dump_info.rd_mode = 0xFF;
        etcam_dump_info.tb_depth = DPP_ETCAM_RAM_DEPTH;
        etcam_dump_info.data_or_mask = DPP_ETCAM_DTYPE_DATA;

        rc = dpp_dtb_tab_up_item_offset_addr_get(dev,
                                             queue_id,
                                             dump_element_id,
                                             dump_data_len,
                                             &etcam_data_dst_phy_haddr,
                                             &etcam_data_dst_phy_laddr);
        ZXIC_COMM_CHECK_RC_MEMORY_VFREE_NO_ASSERT(rc, "dpp_dtb_tab_up_item_offset_addr_get", dump_info_buff);
        rc = dpp_dtb_etcam_dump_entry(dev,
                                    &etcam_dump_info,
                                    etcam_data_dst_phy_haddr,
                                    etcam_data_dst_phy_laddr,
                                    &dtb_dump_entry);
        ZXIC_COMM_CHECK_RC_MEMORY_VFREE_NO_ASSERT(rc, "dpp_dtb_etcam_dump_entry", dump_info_buff);
        rc = dpp_dtb_data_write(dump_info_buff, dtb_desc_addr_offset, &dtb_dump_entry);
        ZXIC_COMM_CHECK_RC_MEMORY_VFREE_NO_ASSERT(rc, "dpp_dtb_data_write", dump_info_buff);
        ZXIC_COMM_MEMSET(cmd_buff, 0, DTB_TABLE_CMD_SIZE_BIT / 8);

        dtb_desc_len += 1;
        dtb_desc_addr_offset += DTB_LEN_POS_SETP;
        dump_data_len += DPP_ETCAM_RAM_DEPTH * 640 / 8;
    }

    etcam_data_len_offset = dump_data_len;
    // etcam mask dump描述符，以block为单位，深度是512 起始地址是0  rd_mode = 0xff
    for(i = 0; i < block_num; i++)
    {
        block_idx = p_tbl_cfg->block_array[i];

        ZXIC_COMM_TRACE_INFO("mask: block_idx: %d\n", block_idx);

        etcam_dump_info.block_sel = block_idx;
        etcam_dump_info.addr = 0;
        etcam_dump_info.tb_width = 3;
        etcam_dump_info.rd_mode = 0xFF;
        etcam_dump_info.tb_depth = DPP_ETCAM_RAM_DEPTH;
        etcam_dump_info.data_or_mask = DPP_ETCAM_DTYPE_MASK;

        rc = dpp_dtb_tab_up_item_offset_addr_get(dev,
                                                queue_id,
                                                dump_element_id,
                                                dump_data_len,
                                                &etcam_mask_dst_phy_haddr,
                                                &etcam_mask_dst_phy_laddr);
        ZXIC_COMM_CHECK_RC_MEMORY_VFREE_NO_ASSERT(rc, "dpp_dtb_tab_up_item_offset_addr_get", dump_info_buff);
        rc = dpp_dtb_etcam_dump_entry(dev,
                                    &etcam_dump_info,
                                    etcam_mask_dst_phy_haddr,
                                    etcam_mask_dst_phy_laddr,
                                    &dtb_dump_entry);
        ZXIC_COMM_CHECK_RC_MEMORY_VFREE_NO_ASSERT(rc, "dpp_dtb_etcam_dump_entry", dump_info_buff);
        rc = dpp_dtb_data_write(dump_info_buff, dtb_desc_addr_offset, &dtb_dump_entry);
        ZXIC_COMM_CHECK_RC_MEMORY_VFREE_NO_ASSERT(rc, "dpp_dtb_data_write", dump_info_buff);
        ZXIC_COMM_MEMSET(cmd_buff, 0, DTB_TABLE_CMD_SIZE_BIT / 8);

        dtb_desc_len += 1;
        dtb_desc_addr_offset += DTB_LEN_POS_SETP;
        dump_data_len += DPP_ETCAM_RAM_DEPTH * 640 / 8;
    }
    etcam_mask_len_offset = dump_data_len;

    //补充级联描述符
    if(as_enable)
    {
        rc = dtb_eram_index_cal(dev, etcam_as_mode, etcam_table_depth, &dump_eram_depth_128bit, &eram_col_index);
        ZXIC_COMM_CHECK_RC_MEMORY_VFREE_NO_ASSERT(rc, "dtb_eram_index_cal", dump_info_buff);

        rc = dpp_dtb_tab_up_item_offset_addr_get(dev,
                                                 queue_id,
                                                 dump_element_id,
                                                 dump_data_len,
                                                 &as_rst_dst_phy_haddr,
                                                 &as_rst_dst_phy_laddr);
        ZXIC_COMM_CHECK_RC_MEMORY_VFREE_NO_ASSERT(rc, "dpp_dtb_tab_up_item_offset_addr_get", dump_info_buff);

        rc = dpp_dtb_smmu0_dump_entry(dev, 
                                as_eram_baddr, 
                                dump_eram_depth_128bit,
                                as_rst_dst_phy_haddr,
                                as_rst_dst_phy_laddr,
                                &dtb_dump_entry);
        ZXIC_COMM_CHECK_RC_MEMORY_VFREE_NO_ASSERT(rc, "dpp_dtb_smmu0_dump_entry", dump_info_buff);
        rc = dpp_dtb_data_write(dump_info_buff, dtb_desc_addr_offset, &dtb_dump_entry);
        ZXIC_COMM_CHECK_RC_MEMORY_VFREE_NO_ASSERT(rc, "dpp_dtb_data_write", dump_info_buff);
        ZXIC_COMM_MEMSET(cmd_buff, 0, DTB_TABLE_CMD_SIZE_BIT / 8);
        dtb_desc_len += 1;
        dtb_desc_addr_offset += DTB_LEN_POS_SETP;
        dump_data_len += dump_eram_depth_128bit * 128 / 8;
    }

    ZXIC_COMM_TRACE_INFO("dtb_desc_len: 0x%x\n", dtb_desc_len);
    ZXIC_COMM_TRACE_INFO("dtb_desc_addr_offset: 0x%x\n", dtb_desc_addr_offset);
    ZXIC_COMM_TRACE_INFO("dump_data_len: 0x%x\n", dump_data_len);

    //申请data空间
    temp_dump_out_data = (ZXIC_UINT8 *) ZXIC_COMM_VMALLOC(dump_data_len * sizeof(ZXIC_UINT8));
    ZXIC_COMM_CHECK_POINT_MEMORY_VFREE_NO_ASSERT(temp_dump_out_data, dump_info_buff);
    ZXIC_COMM_MEMSET(temp_dump_out_data, 0, dump_data_len * sizeof(ZXIC_UINT8));

    /*下发dump描述符*/
    rc = dpp_dtb_write_dump_desc_info(dev,
                                      queue_id,
                                      dump_element_id,
                                      (ZXIC_UINT32 *)dump_info_buff,
                                      dump_data_len / 4,
                                      dtb_desc_len * 4,
                                      (ZXIC_UINT32 *)temp_dump_out_data);
    ZXIC_COMM_VFREE(dump_info_buff);
    ZXIC_COMM_CHECK_RC_MEMORY_VFREE_NO_ASSERT(rc,"dpp_dtb_write_dump_desc_info", temp_dump_out_data);

    //解析数据
    p_data_start = temp_dump_out_data;
    p_mask_start = temp_dump_out_data + etcam_data_len_offset;
    if(as_enable)
    {
        p_rst_start = temp_dump_out_data + etcam_mask_len_offset;
    }

    for (handle = 0; handle < etcam_table_depth; handle++)
    {
        p_dump_user_data = p_dump_data_arr + handle;

        if ((p_dump_user_data == NULL) || (p_dump_user_data->key_data == NULL) || (p_dump_user_data->key_mask == NULL))
        {
            ZXIC_COMM_TRACE_ERROR("etcam handle 0x%x data user buff is NULL!\n", handle);
            ZXIC_COMM_VFREE(temp_dump_out_data);
            return DPP_ERR;
        }

        if(as_enable)
        {
            if(p_dump_user_data->p_as_rslt == NULL)
            {
                ZXIC_COMM_TRACE_ERROR("etcam handle 0x%x as data buff is NULL!\n", handle);
                ZXIC_COMM_VFREE(temp_dump_out_data);
                return DPP_ERR;
            }
        }

        p_dump_user_data->handle = handle;

        addr_640bit = handle / (1U << etcam_key_mode);
        rd_mask = (((1U << (8U >> etcam_key_mode)) - 1) << ((8U >> etcam_key_mode) * (handle % (1U << etcam_key_mode)))) & 0xFF;

        p_data_640bit = p_data_start + addr_640bit * 640 / 8;
        p_mask_640bit = p_mask_start + addr_640bit * 640 / 8;

        dpp_dtb_etcam_ind_data_get(dev, p_data_640bit, rd_mask, entry_xy.p_data);
        dpp_dtb_etcam_ind_data_get(dev, p_mask_640bit, rd_mask, entry_xy.p_mask);

        rc = dpp_etcam_xy_to_dm(&entry_dm, &entry_xy, DPP_ETCAM_ENTRY_SIZE_GET(etcam_key_mode));
        ZXIC_COMM_CHECK_RC_MEMORY_VFREE_NO_ASSERT(rc, "dpp_etcam_xy_to_dm", temp_dump_out_data);

        ZXIC_COMM_MEMCPY(p_dump_user_data->key_data, entry_dm.p_data, DPP_ETCAM_ENTRY_SIZE_GET(etcam_key_mode));
        ZXIC_COMM_MEMCPY(p_dump_user_data->key_mask, entry_dm.p_mask, DPP_ETCAM_ENTRY_SIZE_GET(etcam_key_mode));

        if(as_enable)
        {
            rc = dtb_eram_index_cal(dev, etcam_as_mode, handle, &eram_row_index, &eram_col_index);
            ZXIC_COMM_CHECK_RC_MEMORY_VFREE_NO_ASSERT(rc, "dtb_eram_index_cal", temp_dump_out_data);
            p_rst_128bit = p_rst_start + eram_row_index * DTB_LEN_POS_SETP;

            eram_buff = (ZXIC_UINT32*)p_rst_128bit;
            
            if(etcam_as_mode == ERAM128_TBL_128b)
            {   
                ZXIC_COMM_MEMCPY(p_dump_user_data->p_as_rslt, eram_buff, (128 / 8));
            }
            else if(etcam_as_mode == ERAM128_TBL_64b)
            {   
                ZXIC_COMM_MEMCPY(p_dump_user_data->p_as_rslt, eram_buff + ((1 - eram_col_index) << 1), (64 / 8));
            }
        }

    }
   
    *entry_num = etcam_table_depth;
    *finish_flag = 1;
    ZXIC_COMM_TRACE_INFO(" etcam table dump entry num %d, finish flag %d\n\n", *entry_num, *finish_flag);

    ZXIC_COMM_VFREE(temp_dump_out_data);

    return DPP_OK;
}

/***********************************************************/
/** dump eram表内容
* @param   dev            设备
* @param   queue_id       队列id 0~127
* @param   sdt_no         sdt号  0~255
* @param   pDumpData      出参，dump数据，内存由用户分配，结构体DPP_HASH_ENTRY
* @param   entryNum       出参，dump出的有效hash条目
* @return
* @remark  无
* @see
* @author  cq      @date  2025/04/03
************************************************************/
DPP_STATUS dpp_dtb_acl_dump(DPP_DEV_T *dev,
                                ZXIC_UINT32 queue_id,
                                ZXIC_UINT32 sdt_no,
                                ZXIC_UINT8  *pDumpData,
                                ZXIC_UINT32 *p_entry_num)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT64 dma_phy_addr = 0;
    ZXIC_UINT64 dma_vir_addr = 0;
    ZXIC_UINT32 dma_size = DTB_SDT_DUMP_SIZE;
    ZXIC_MUTEX_T *p_mutex = NULL;
    DPP_DTB_DUMP_INDEX_T start_index = {0};
    DPP_DTB_DUMP_INDEX_T next_start_index = {0};
    ZXIC_UINT32 finish_flag = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(pDumpData);
    ZXIC_COMM_CHECK_POINT(p_entry_num);

    rc = dpp_dev_dtb_opr_mutex_get(dev, DPP_DEV_MUTEX_T_DTB_RB, queue_id, &p_mutex);
    ZXIC_COMM_CHECK_RC(rc, "dpp_dev_dtb_opr_mutex_get");
    rc = zxic_comm_mutex_lock(p_mutex);
    ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_lock");

    rc = dpp_dev_dump_dma_mem_get(dev, &dma_size, &dma_phy_addr, &dma_vir_addr);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dev_dump_dma_mem_get", p_mutex);

    rc = dpp_dtb_dump_sdt_addr_set(dev, queue_id, sdt_no, dma_phy_addr, dma_vir_addr, dma_size);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_dump_sdt_addr_set", p_mutex);

    start_index.index = 0;
    rc = dpp_dtb_acl_table_dump(dev, queue_id, sdt_no, start_index, (DPP_DTB_ACL_ENTRY_INFO_T *)pDumpData,
                                                              p_entry_num, &next_start_index, &finish_flag);
    rc |= dpp_dtb_dump_sdt_addr_clear(dev, queue_id, sdt_no);
    ZXIC_COMM_CHECK_RC_UNLOCK(rc, "dpp_dtb_acl_table_dump", p_mutex);

    rc = zxic_comm_mutex_unlock(p_mutex);
    ZXIC_COMM_CHECK_RC(rc, "zxic_comm_mutex_unlock");

    return DPP_OK;
}

/***********************************************************/
/** 释放vport下的所有index
* @param   dev_id          NP设备号
* @param   sdt_no          流表sdt号(0~255)
* @param   vport           端口号
* @param   index           需要释放的索引值
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/14
************************************************************/  
DPP_STATUS dpp_dtb_acl_index_release_by_vport(DPP_DEV_T *dev,
                                     ZXIC_UINT32 sdt_no,
                                     ZXIC_UINT32 vport)
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

    mutex = DPP_DEV_MUTEX_T_DTB;
    rc = dpp_dev_opr_mutex_get(dev, (ZXIC_UINT32)mutex, &p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_lock");

    rc = dpp_agent_channel_acl_index_release(dev, ACL_INDEX_VPORT_REL, sdt_no, vport, 0);
    if(rc == DPP_ACL_RC_SRH_FAIL)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ACL_INDEX_VPORT_REL[vport:0x%x] index is not exist. \n", vport);
    }
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(dev_id, rc, "dpp_agent_channel_acl_index_release", p_dtb_mutex);

    rc = zxic_comm_mutex_unlock(p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_unlock");
    
    return rc;    
}

/***********************************************************/
/** 释放网卡上的所有index
* @param   dev_id          NP设备号
* @param   sdt_no          流表sdt号(0~255)
* @return
* @remark  无
* @see
* @author  cq      @date  2026/01/12
************************************************************/  
DPP_STATUS dpp_dtb_acl_index_release_all(DPP_DEV_T *dev,
                                     ZXIC_UINT32 sdt_no)
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

    mutex = DPP_DEV_MUTEX_T_DTB;
    rc = dpp_dev_opr_mutex_get(dev, (ZXIC_UINT32)mutex, &p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_lock");

    rc = dpp_agent_channel_acl_index_release(dev, ACL_INDEX_ALL_REL, sdt_no, 0, 0);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(dev_id, rc, "dpp_agent_channel_acl_index_release", p_dtb_mutex);

    rc = zxic_comm_mutex_unlock(p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_unlock");
    
    return rc;    
}

/***********************************************************/
/** 获取当前vport下分配的所有index
* @param   dev             设备
* @param   queue_id        队列号
* @param   eram_sdt_no     维护index的eram直接表号
* @param   vport           端口号
* @param   index_num       出参，当前vport分配的index个数
* @param   p_index_array   出参，当前vport分配的index数组
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/18
************************************************************/
DPP_STATUS dpp_dtb_acl_index_parse(DPP_DEV_T *dev,
                                ZXIC_UINT32 queue_id,
                                ZXIC_UINT32 eram_sdt_no,
                                ZXIC_UINT32 vport, 
                                ZXIC_UINT32 *index_num,
                                ZXIC_UINT32 *p_index_array)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 eram_table_depth = 0;
    ZXIC_UINT32 byte_num = 0;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 entry_num = 0;
    ZXIC_UINT32 valid_entry_num = 0;
    ZXIC_UINT8  valid = 0;
    ZXIC_UINT32  temp_vport = 0;
    DPP_SDTTBL_ERAM_T sdt_eram = {0};  /*SDT内容*/
    DPP_DTB_ERAM_ENTRY_INFO_T *p_dump_data_arr = NULL;
    ZXIC_UINT8 *data_buff = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id,index_num);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id,p_index_array);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, eram_sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    
    rc = dpp_soft_sdt_tbl_get(dev, eram_sdt_no, &sdt_eram);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_soft_sdt_tbl_get");
    
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sdt_eram.eram_mode, ERAM128_TBL_64b, ERAM128_TBL_128b);
    byte_num = (sdt_eram.eram_mode == ERAM128_TBL_64b) ? 8 : 16;
    eram_table_depth = sdt_eram.eram_table_depth; 
    p_dump_data_arr = (DPP_DTB_ERAM_ENTRY_INFO_T *)ZXIC_COMM_MALLOC(eram_table_depth * sizeof(DPP_DTB_ERAM_ENTRY_INFO_T));
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dump_data_arr);
    ZXIC_COMM_MEMSET(p_dump_data_arr, 0, eram_table_depth * sizeof(DPP_DTB_ERAM_ENTRY_INFO_T));

    data_buff = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(byte_num*eram_table_depth);
    ZXIC_COMM_CHECK_DEV_POINT_MEMORY_FREE(dev_id, data_buff, p_dump_data_arr);
    ZXIC_COMM_MEMSET(data_buff, 0, eram_table_depth * byte_num);

    for(i = 0; i < eram_table_depth; i++)
    {
        p_dump_data_arr[i].index = i;
        p_dump_data_arr[i].p_data = (ZXIC_UINT32 *)(data_buff+i*byte_num);
    }

    rc = dpp_dtb_eram_dump(dev, queue_id, eram_sdt_no, (ZXIC_UINT8 *)p_dump_data_arr, &entry_num);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE2PTR_NO_ASSERT(dev_id, rc, "dpp_dtb_eram_dump", data_buff, p_dump_data_arr);

    for(i=0; i<entry_num; i++)
    {
        valid = (p_dump_data_arr[i].p_data[0] >> 31) & 0x1;
        temp_vport = p_dump_data_arr[i].p_data[0]&0x7fffffff;
        if(valid && (temp_vport == vport))
        {
            p_index_array[valid_entry_num] = i;
            valid_entry_num++;
        }
    }
    
    *index_num = valid_entry_num;
    ZXIC_COMM_FREE(data_buff);
    ZXIC_COMM_FREE(p_dump_data_arr);

    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_PRINT("dpp_dtb_acl_index_parse vport=0x%x index_num=%u,index:\n", vport,valid_entry_num);
        for(i=0;i<valid_entry_num;i++)
        {
            ZXIC_COMM_PRINT("[%u] ",p_index_array[i]);
            if((i+1)%16==0)
            {
                ZXIC_COMM_PRINT("\n");
            }
        }
        ZXIC_COMM_PRINT("\n");
    }
    
    return DPP_OK;
}

/***********************************************************/
/** 获取芯片已分配的所有index
* @param   dev             设备
* @param   queue_id        队列号
* @param   eram_sdt_no     维护index的eram直接表号
* @param   index_num       出参，当前vport分配的index个数
* @param   p_index_array   出参，当前vport分配的index数组
* @return
* @remark  无
* @see
* @author  cq      @date  2025/12/25
************************************************************/
DPP_STATUS dpp_dtb_acl_index_parse_all(DPP_DEV_T *dev,
                                ZXIC_UINT32 queue_id,
                                ZXIC_UINT32 eram_sdt_no,
                                ZXIC_UINT32 *index_num,
                                ZXIC_UINT32 *p_index_array)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 eram_table_depth = 0;
    ZXIC_UINT32 byte_num = 0;
    ZXIC_UINT32 dump_len = 0;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 entry_num = 0;
    ZXIC_UINT32 valid_entry_num = 0;
    ZXIC_UINT8  valid = 0;
    DPP_SDTTBL_ERAM_T sdt_eram = {0};  /*SDT内容*/
    DPP_DTB_ERAM_ENTRY_INFO_T *p_dump_data_arr = NULL;
    ZXIC_UINT8 *data_buff = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id,index_num);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id,p_index_array);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, eram_sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    
    rc = dpp_soft_sdt_tbl_get(dev, eram_sdt_no, &sdt_eram);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_soft_sdt_tbl_get");
    
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sdt_eram.eram_mode, ERAM128_TBL_64b, ERAM128_TBL_128b);
    byte_num = (sdt_eram.eram_mode == ERAM128_TBL_64b) ? 8 : 16;
    eram_table_depth = sdt_eram.eram_table_depth; 
    dump_len = eram_table_depth * sizeof(DPP_DTB_ERAM_ENTRY_INFO_T);
    p_dump_data_arr = (DPP_DTB_ERAM_ENTRY_INFO_T *)ZXIC_COMM_MALLOC(dump_len);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_dump_data_arr);
    ZXIC_COMM_MEMSET_S(p_dump_data_arr, dump_len, 0, dump_len);

    data_buff = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(byte_num*eram_table_depth);
    ZXIC_COMM_CHECK_DEV_POINT_MEMORY_FREE(dev_id, data_buff, p_dump_data_arr);
    ZXIC_COMM_MEMSET_S(data_buff, eram_table_depth * byte_num, 0, eram_table_depth * byte_num);

    for(i = 0; i < eram_table_depth; i++)
    {
        p_dump_data_arr[i].index = i;
        p_dump_data_arr[i].p_data = (ZXIC_UINT32 *)(data_buff+i*byte_num);
    }

    rc = dpp_dtb_eram_dump(dev, queue_id, eram_sdt_no, (ZXIC_UINT8 *)p_dump_data_arr, &entry_num);
    ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE2PTR_NO_ASSERT(dev_id, rc, "dpp_dtb_eram_dump", data_buff, p_dump_data_arr);

    for(i=0; i<entry_num; i++)
    {
        valid = (p_dump_data_arr[i].p_data[0] >> 31) & 0x1;
        if(valid)
        {
            p_index_array[valid_entry_num] = i;
            valid_entry_num++;
        }
    }
    
    *index_num = valid_entry_num;
    ZXIC_COMM_FREE(data_buff);
    ZXIC_COMM_FREE(p_dump_data_arr);

    if(dpp_dtb_prt_get())
    {
        ZXIC_COMM_PRINT("dpp_dtb_acl_index_parse index_num=%u,index:\n", valid_entry_num);
        for(i=0;i<valid_entry_num;i++)
        {
            ZXIC_COMM_PRINT("[%u] ",p_index_array[i]);
            if((i+1)%16==0)
            {
                ZXIC_COMM_PRINT("\n");
            }
        }
        ZXIC_COMM_PRINT("\n");
    }
    
    return DPP_OK;
}

/***********************************************************/
/** 清除指定index的所有eram表项
* @param   dev             设备
* @param   queue_id        队列号
* @param   sdt_no          维护index的eram直接表号
* @param   index_num       当前vport分配的index个数
* @param   p_index_array   当前vport分配的index数组
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/18
************************************************************/
DPP_STATUS dpp_dtb_eram_data_clear(DPP_DEV_T *dev, 
                                  ZXIC_UINT32 queue_id, 
                                  ZXIC_UINT32 sdt_no, 
                                  ZXIC_UINT32 index_num, 
                                  ZXIC_UINT32 *p_index_array)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 element_id = 0;
    ZXIC_UINT32 i = 0;

    DPP_DTB_ERAM_ENTRY_INFO_T *p_eram_data_arr = NULL;
    ZXIC_UINT8 *data_buff = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id,p_index_array);

    p_eram_data_arr = (DPP_DTB_ERAM_ENTRY_INFO_T *)ZXIC_COMM_MALLOC(index_num * sizeof(DPP_DTB_ERAM_ENTRY_INFO_T));
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_eram_data_arr);
    ZXIC_COMM_MEMSET(p_eram_data_arr, 0, index_num * sizeof(DPP_DTB_ERAM_ENTRY_INFO_T));

    data_buff = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(4*sizeof(ZXIC_UINT32));
    ZXIC_COMM_CHECK_DEV_POINT_MEMORY_FREE(dev_id, data_buff, p_eram_data_arr);
    ZXIC_COMM_MEMSET(data_buff, 0, 4*sizeof(ZXIC_UINT32));

    for(i = 0; i < index_num; i++)
    {
        p_eram_data_arr[i].index = p_index_array[i];
        p_eram_data_arr[i].p_data = (ZXIC_UINT32 *)data_buff;
    }

    rc = dpp_dtb_eram_dma_write(dev, queue_id, sdt_no, index_num, p_eram_data_arr,&element_id);
    ZXIC_COMM_FREE(data_buff);
    ZXIC_COMM_FREE(p_eram_data_arr);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_dtb_eram_dma_write");

    return DPP_OK;
}

/***********************************************************/
/** dtb方式清除指定index的所有统计项
* @param   dev             设备
* @param   queue_id        队列号
* @param   counter_id      统计编号，对应微码中的address
* @param   rd_mode         统计读取方式 0:64bit 1:128bit
* @param   sdt_no          维护index的eram直接表号
* @param   index_num       当前vport分配的index个数
* @param   p_index_array   当前vport分配的index数组
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/18
************************************************************/
DPP_STATUS dpp_dtb_eram_stat_data_clear(DPP_DEV_T *dev, 
                                  ZXIC_UINT32 queue_id, 
                                  ZXIC_UINT32 counter_id,
                                  STAT_CNT_MODE_E rd_mode,
                                  ZXIC_UINT32 index_num, 
                                  ZXIC_UINT32 *p_index_array)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 element_id = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 wrt_mode = 0;
    ZXIC_UINT32 start_addr = 0;
    ZXIC_UINT32 counter_id_128bit = 0;
    ZXIC_UINT32 ppu_eram_baddr = 0;

    DPP_DTB_ERAM_ENTRY_INFO_T *p_eram_data_arr = NULL;
    ZXIC_UINT8 *data_buff = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id,p_index_array);

    rc = dpp_stat_ppu_eram_baddr_get(dev,
                                     &ppu_eram_baddr);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_ppu_eram_baddr_get");

    p_eram_data_arr = (DPP_DTB_ERAM_ENTRY_INFO_T *)ZXIC_COMM_MALLOC(index_num * sizeof(DPP_DTB_ERAM_ENTRY_INFO_T));
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_eram_data_arr);
    ZXIC_COMM_MEMSET(p_eram_data_arr, 0, index_num * sizeof(DPP_DTB_ERAM_ENTRY_INFO_T));

    data_buff = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(4*sizeof(ZXIC_UINT32));
    ZXIC_COMM_CHECK_DEV_POINT_MEMORY_FREE(dev_id, data_buff, p_eram_data_arr);
    ZXIC_COMM_MEMSET(data_buff, 0, 4*sizeof(ZXIC_UINT32));

    for(i = 0; i < index_num; i++)
    {
        p_eram_data_arr[i].index = p_index_array[i];
        p_eram_data_arr[i].p_data = (ZXIC_UINT32 *)data_buff;
    }
    
    wrt_mode = (rd_mode==STAT_128_MODE)? ERAM128_OPR_128b : ERAM128_OPR_64b;
    counter_id_128bit = (rd_mode==STAT_128_MODE) ?  counter_id : (counter_id>>1);
    start_addr = ppu_eram_baddr + counter_id_128bit;
    ZXIC_COMM_TRACE_INFO("dpp_dtb_eram_stat_data_clear:ppu_eram_baddr=0x%x start_addr=0x%x\n",ppu_eram_baddr,start_addr);
    rc = dpp_dtb_smmu0_data_write(dev,
                                  queue_id,
                                  start_addr,
                                  wrt_mode,
                                  index_num,
                                  p_eram_data_arr,
                                  &element_id);
    ZXIC_COMM_FREE(data_buff);
    ZXIC_COMM_FREE(p_eram_data_arr);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_dtb_smmu0_data_write");

    return DPP_OK;
}

/***********************************************************/
/** 清除指定index的所有acl表项
* @param   dev             设备
* @param   queue_id        队列号
* @param   sdt_no          acl表项的sdt号
* @param   index_num       当前vport分配的index个数
* @param   p_index_array   当前vport分配的index数组
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/18
************************************************************/
DPP_STATUS dpp_dtb_acl_data_clear(DPP_DEV_T *dev, 
                                  ZXIC_UINT32 queue_id, 
                                  ZXIC_UINT32 sdt_no, 
                                  ZXIC_UINT32 index_num, 
                                  ZXIC_UINT32 *p_index_array)
{
    DPP_STATUS  rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_UINT32 data_byte_size = 0;
    ZXIC_UINT32 index = 0;
    ZXIC_UINT32 etcam_key_mode = 0;
    ZXIC_UINT32 as_enable = 0;
    ZXIC_UINT32 etcam_table_depth = 0;
    ZXIC_UINT32 element_id = 0;

    DPP_SDTTBL_ETCAM_T sdt_etcam_info = {0};  /*SDT内容*/

    ZXIC_UINT8 *data_buff = NULL;
    ZXIC_UINT8 *mask_buff = NULL;
    ZXIC_UINT32 *eram_buff = NULL;
    DPP_DTB_ACL_ENTRY_INFO_T *p_entry_arr = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, queue_id, 0, DPP_DTB_QUEUE_NUM_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sdt_no, 0, DPP_DEV_SDT_ID_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_index_array);

    rc = dpp_soft_sdt_tbl_get(dev, sdt_no, &sdt_etcam_info);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_soft_sdt_tbl_get");

    etcam_key_mode = sdt_etcam_info.etcam_key_mode;
    as_enable = sdt_etcam_info.as_en;
    etcam_table_depth = sdt_etcam_info.etcam_table_depth;
    data_byte_size = DPP_ETCAM_ENTRY_SIZE_GET(etcam_key_mode);//80/40
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, index_num, 1, etcam_table_depth);

    //组装数据
    p_entry_arr = (DPP_DTB_ACL_ENTRY_INFO_T *)ZXIC_COMM_MALLOC(index_num * sizeof(DPP_DTB_ACL_ENTRY_INFO_T));
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_entry_arr);
    ZXIC_COMM_MEMSET_S(p_entry_arr, index_num * sizeof(DPP_DTB_ACL_ENTRY_INFO_T), 0, index_num * sizeof(DPP_DTB_ACL_ENTRY_INFO_T));

    data_buff = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(data_byte_size );
    ZXIC_COMM_CHECK_DEV_POINT_MEMORY_FREE(dev_id, data_buff, p_entry_arr);
    ZXIC_COMM_MEMSET_S(data_buff, data_byte_size, 0, data_byte_size);

    mask_buff = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(data_byte_size);
    ZXIC_COMM_CHECK_DEV_POINT_MEMORY_FREE2PTR_NO_ASSERT(dev_id, mask_buff, p_entry_arr, data_buff);
    ZXIC_COMM_MEMSET_S(mask_buff, data_byte_size, 0, data_byte_size);

    if(as_enable)
    {
        eram_buff = (ZXIC_UINT32 *)ZXIC_COMM_MALLOC(4 * sizeof(ZXIC_UINT32));
        ZXIC_COMM_CHECK_DEV_POINT_MEMORY_FREE3PTR_NO_ASSERT(dev_id, eram_buff, p_entry_arr, data_buff, mask_buff);
        ZXIC_COMM_MEMSET_S(eram_buff, 4 * sizeof(ZXIC_UINT32), 0, 4 * sizeof(ZXIC_UINT32));
    }

    for(index = 0; index < index_num; index++)
    {
        p_entry_arr[index].handle = p_index_array[index];
        p_entry_arr[index].key_data = data_buff;
        p_entry_arr[index].key_mask = mask_buff;

        if(as_enable)
        {
            p_entry_arr[index].p_as_rslt = (ZXIC_UINT8 *)eram_buff;
        }
    }

    rc = dpp_dtb_acl_dma_insert(dev,
                            queue_id,
                            sdt_no,
                            index_num,
                            p_entry_arr,
                            &element_id);
    ZXIC_COMM_FREE(data_buff);
    ZXIC_COMM_FREE(mask_buff);
    if(eram_buff)
    {
        ZXIC_COMM_FREE(eram_buff);
    }
    ZXIC_COMM_FREE(p_entry_arr);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_dtb_acl_data_clear");
    
    return rc;
}

/***********************************************************/
/** 指定vport的统计计数读清
* @param   dev              NP设备
* @param   sdt_no           流表sdt号(0~255)
* @param   vport            端口号
* @param   rd_mode          读取位宽模式，参见STAT_CNT_MODE_E，0-64bit，1-128bit
* @param   start_counter_id 统计起始编号，对应微码中的address
* @return
* @remark  无
* @see
* @author  cq      @date  2024/09/14
************************************************************/  
DPP_STATUS dpp_dtb_acl_stat_cnt_clr(DPP_DEV_T *dev,
                                     ZXIC_UINT32 sdt_no,
                                     ZXIC_UINT32 vport,
                                     STAT_CNT_MODE_E rd_mode,
                                     ZXIC_UINT32 start_counter_id)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 dev_id = 0;
    ZXIC_MUTEX_T *p_dtb_mutex = NULL;
    DPP_DEV_MUTEX_TYPE_E mutex = 0;
  
    ZXIC_COMM_CHECK_POINT(dev);
    dev_id = DEV_ID(dev);
    ZXIC_COMM_CHECK_INDEX(dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    
    mutex = DPP_DEV_MUTEX_T_DTB;
    rc = dpp_dev_opr_mutex_get(dev, (ZXIC_UINT32)mutex, &p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "dpp_dev_opr_mutex_get");

    rc = zxic_comm_mutex_lock(p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_lock");

    rc = dpp_agent_channel_acl_stat_clr(dev, sdt_no, vport, start_counter_id, rd_mode);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(dev_id, rc, "dpp_agent_channel_acl_stat_clr", p_dtb_mutex);

    rc = zxic_comm_mutex_unlock(p_dtb_mutex);
    ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id, rc, "zxic_comm_mutex_unlock");

    return rc;    
}

#endif

#if ZXIC_REAL("DTB DEBUG PRINT")

/*打印buff中数据，size 单位 字节*/
ZXIC_VOID dpp_data_buff_print(ZXIC_UINT8 *buff, ZXIC_UINT32 size)
{

    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT8 *temp_buff = NULL;

    ZXIC_COMM_PRINT("buff data is:\n");

    for (i = 0; i < size / 4 / 4; i++)
    {
        temp_buff = buff + 16 * i;
        for(j = 0; j < 4; j++)
        {
            ZXIC_COMM_PRINT("0x%08x  ", *((ZXIC_UINT32*)(temp_buff + 4 * j)));
        } 
        ZXIC_COMM_PRINT("\n");
    }

    ZXIC_COMM_PRINT("\n");
}

ZXIC_VOID dpp_acl_data_print(ZXIC_UINT8 *p_data, ZXIC_UINT8 *p_mask, ZXIC_UINT32 etcam_mode)
{ 
        int i = 0;
        int data_len = 0;

        data_len = DPP_ETCAM_ENTRY_SIZE_GET(etcam_mode);

        if(data_len > 80)
        {
            return;
        }

        ZXIC_COMM_PRINT("%s:", "data");

        for (i = 0; i < data_len; i++)
        {
            if ((i % 10) == 0)
            {
                ZXIC_COMM_PRINT("\n");
            }

            ZXIC_COMM_PRINT("%02x", p_data[i]);
        }

        ZXIC_COMM_PRINT("\n");

        ZXIC_COMM_PRINT("%s:", "mask");

        for (i = 0; i < data_len; i++)
        {
            if ((i % 10) == 0)
            {
                ZXIC_COMM_PRINT("\n");
            }

            ZXIC_COMM_PRINT("%02x", p_mask[i]);
        }
        ZXIC_COMM_PRINT("\n");
}

ZXIC_VOID dpp_dtb_data_print(ZXIC_UINT8 *p_data, ZXIC_UINT32 len)
{
    int i = 0;
    int cycle = len / 4;
    int remain = len % 4;

    ZXIC_COMM_TRACE_INFO("%s:", "data:\n");

    for(i = 0; i < cycle; i++)
    {
        ZXIC_COMM_TRACE_INFO("0x%02x %02x %02x %02x \n", p_data[4 * i],  p_data[4 * i + 1],  p_data[4 * i + 2],  p_data[4 * i + 3]);
    }

    for(i = 0; i < remain; i++)
    {
        ZXIC_COMM_TRACE_INFO("0x%02x", p_data[cycle * 4 + i]);
    }
    ZXIC_COMM_TRACE_INFO("\n");
}

#endif

/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_reg.c
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 : 
* 完成日期 : 2014/02/12
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/

#include "zxic_common.h"
#include "dpp_type_api.h"
#include "dpp_module.h"
#include "dpp_dev.h"
#include "dpp_reg_api.h"
#include "dpp_reg_info.h"
#include "dpp_agent_channel.h"
#include "dpp_pci.h"

#define REG_DATA_MAX      (512/32)

static DPP_REG_OFFSET_ADDR g_module_offset_addr[] = 
{
    {DTB4K,   BAR_4K_DTB,   SYS_DTB_BASE_ADDR      + MODULE_DTB_ENQ_BASE_ADDR},
    {STAT4K,  BAR_4K_ETCAM, SYS_STAT_BASE_ADDR     + MODULE_STAT_ETCAM_BASE_ADDR},
    {PPU4K,   BAR_4K_CLS0,  SYS_PPU_BASE_ADDR      + MODULE_CLUSTER0_BASE_ADDR + 0x4000},
    {SE4K,    BAR_4K_SE,    SYS_SE_BASE_ADDR       + MODULE_SE_ALG_BASE_ADDR},
    {SMMU14K, BAR_4K_SMMU1, SYS_SE_SMMU1_BASE_ADDR + MODULE_SE_SMMU1_BASE_ADDR }
};

/***********************************************************/
/** 获取寄存器属性
* @param   reg_no  寄存器编号
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_REG_T *dpp_reg_info_get(ZXIC_UINT32 reg_no)
{
    ZXIC_COMM_CHECK_INDEX_RETURN_NULL(reg_no, 0, REG_ENUM_MAX_VALUE - 1);

    return (&g_dpp_reg_info[reg_no]);
}

/***********************************************************/
/** 根据寄存器编号获得寄存器芯片内绝对地址
* @param   reg_no
* @param   m_offset
* @param   n_offset
*
* @return
* @remark  无
* @see
************************************************************/
ZXIC_UINT32 dpp_reg_get_reg_addr(ZXIC_UINT32 reg_no, ZXIC_UINT32 m_offset, ZXIC_UINT32 n_offset)
{
    ZXIC_UINT32     addr        = 0;
    DPP_REG_T  *p_reg_info = NULL;

    ZXIC_COMM_CHECK_INDEX(reg_no, 0, REG_ENUM_MAX_VALUE - 1);

    p_reg_info = dpp_reg_info_get(reg_no);
    ZXIC_COMM_CHECK_POINT(p_reg_info); 

    /* 计算写地址 */
    addr = p_reg_info->addr;

    if (p_reg_info->array_type & DPP_REG_UNI_ARRAY)
    {
        if (n_offset > (p_reg_info->n_size - 1))
            ZXIC_COMM_TRACE_ERROR("reg n_offset is out of range, reg_no:%d, n:%d, size:%d\n",
                                 reg_no, n_offset, p_reg_info->n_size - 1);

        ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(addr, n_offset * p_reg_info->n_step);
        addr += n_offset * p_reg_info->n_step;
    }
    else if (p_reg_info->array_type & DPP_REG_BIN_ARRAY)
    {
        if ((n_offset > (p_reg_info->n_size - 1)) || (m_offset > (p_reg_info->m_size - 1)))
            ZXIC_COMM_TRACE_ERROR("reg n_offset or m_offset is out of range, reg_no:%d, n:%d, n_size:%d, m:%d, m_size:%d,\n",
                                 reg_no, n_offset, p_reg_info->n_size - 1, m_offset, p_reg_info->m_size - 1);

        ZXIC_COMM_CHECK_INDEX_MUL_OVERFLOW_NO_ASSERT(m_offset, p_reg_info->m_step);
        ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT((m_offset * (p_reg_info->m_step)), (n_offset * (p_reg_info->n_step)));
        ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(addr, (m_offset * (p_reg_info->m_step)) + (n_offset * (p_reg_info->n_step)));
        addr += (m_offset * (p_reg_info->m_step)) + (n_offset * (p_reg_info->n_step));
    }

    return addr;
}

/***********************************************************/
/** 判断是否为4K寄存器
* @param   reg_module
*
* @return  
* @remark  无
* @see
* @author  cq      @date  2023/11/29
************************************************************/
BOOLEAN dpp_4k_reg(ZXIC_UINT32 reg_module)
{
    if((DTB4K<=reg_module)&&(SMMU14K>=reg_module))
    {
        return ZXIC_TRUE;
    }

    return ZXIC_FALSE;
}

/***********************************************************/
/** 获取NP对应模块的映射地址偏移(riscv或者非4K寄存器不做转换，host根据映射情况做转换)
* @param   dev_id
* @param   reg_module
* @param   flags  标志位，DPP_REG_FLAG_INDIRECT DPP_REG_FLAG_DIRECT
* @param   addr
*
* @return  映射地址
* @remark  无
* @see
* @author  cq      @date  2023/11/29
************************************************************/
ZXIC_UINT32 dpp_reg_addr_convert(ZXIC_UINT32 dev_id, ZXIC_UINT32 reg_module,ZXIC_UINT32 flags, ZXIC_UINT32 addr)
{
    ZXIC_UINT32 convert_addr = addr;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 cluster_index = 0;
    ZXIC_UINT32 index_4k = 0;
    ZXIC_UINT32 size_4k = 4096;
    ZXIC_UINT32 module_addr_offset = 0;
    ZXIC_UINT32 dtb_addr_offset = SYS_DTB_BASE_ADDR + MODULE_DTB_ENQ_BASE_ADDR;

    if(DPP_REG_FLAG_INDIRECT == flags)
    {
        return addr;
    }

    for(i=0;i<(sizeof(g_module_offset_addr)/sizeof(DPP_REG_OFFSET_ADDR));i++)
    {
        if(reg_module==g_module_offset_addr[i].reg_module)
        {
            module_addr_offset = g_module_offset_addr[i].addr_offset;
            if(PPU4K==reg_module)
            {
                cluster_index = (addr-module_addr_offset)/DPP_PPU_CLUSTER_SPACE_SIZE;
                index_4k = g_module_offset_addr[i].index_4k + cluster_index;
                module_addr_offset += cluster_index * DPP_PPU_CLUSTER_SPACE_SIZE;
            }
            else
            {
                index_4k = g_module_offset_addr[i].index_4k;
            }
            convert_addr = ((addr + (size_4k * index_4k) + dtb_addr_offset)>module_addr_offset) 
                        ? (addr + (size_4k * index_4k) + dtb_addr_offset - module_addr_offset) : addr;
        }
    }
    
    return convert_addr;
}

/***********************************************************/
/** 通用寄存器写函数
* @param   dev_id  设备号，支持多芯片
* @param   reg_no 寄存器编号
* @param   m_offset 二元寄存器的m偏移
* @param   n_offset 一元寄存器或二元寄存器的n偏移
* @param   p_data 数据指针
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_reg_write(DPP_DEV_T *dev, ZXIC_UINT32 reg_no, ZXIC_UINT32 m_offset, ZXIC_UINT32 n_offset, ZXIC_VOID *p_data)
{
    DPP_STATUS rc = 0;
    ZXIC_UINT32     i = 0;
    ZXIC_UINT32     addr = 0;

#ifdef DPP_FLOW_HW_INIT
    ZXIC_UINT32     convert_addr = 0;
#endif

    ZXIC_UINT32     p_buff[REG_DATA_MAX] = {0};
    ZXIC_UINT32     temp_data = 0;
    ZXIC_UINT32     reg_type = 0;
    ZXIC_UINT32     reg_module = 0;
    ZXIC_UINT32     reg_width = 0;
    DPP_REG_T *p_reg_info = NULL;
    DPP_FIELD_T *p_field_info = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev), DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), reg_no, 0, REG_ENUM_MAX_VALUE - 1);
    ZXIC_COMM_CHECK_POINT(p_data);

    p_reg_info = dpp_reg_info_get(reg_no);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_reg_info);
    p_field_info = p_reg_info->p_fields;
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_field_info);
    reg_type = p_reg_info->flags;
    reg_module = p_reg_info->module_no;
    reg_width = p_reg_info->width;
    ZXIC_COMM_CHECK_INDEX_UPPER(reg_width, REG_DATA_MAX*4);

#ifndef ZXIC_OS_WIN
#ifdef  DPP_FOR_LLT
    if (dpp_stump_reg_en_check(DEV_ID(dev), reg_no) && (p_reg_info->flags == DPP_REG_FLAG_DIRECT || p_reg_info->flags == DPP_REG_FLAG_WO | DPP_REG_FLAG_DIRECT))
    {
        rc = dpp_stump_reg_write(DEV_ID(dev),
                                 reg_no,
                                 m_offset,
                                 n_offset,
                                 p_data);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stump_reg_write");

        return DPP_OK;
    }
#endif
#endif

    /* 提取各字段数据，按各字段实际bit位宽进行拼装 */
    for (i = 0; i < p_reg_info->field_num; i++)
    {
        if (p_field_info[i].len <= 32)
        {
            /* lint -e64 */
            temp_data = *((ZXIC_UINT32 *)p_data + i) & ZXIC_COMM_GET_BIT_MASK(ZXIC_UINT32,  p_field_info[i].len);
            rc = zxic_comm_write_bits_ex((ZXIC_UINT8 *)p_buff,
                                p_reg_info->width * 8,
                                temp_data,
                                p_field_info[i].msb_pos,
                                p_field_info[i].len);
            /* lint +e64 */
            ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "zxic_comm_write_bits_ex");
        }
    }    
    
    ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "zxic_comm_write_bits_ex data = 0x%08x.\n", p_buff[0]);
    /* 若host cpu为小端字节序，则以4字节为单位对数据进行字节序转换 */
    if(!zxic_comm_is_big_endian())
    {
        for (i = 0; i < ((p_reg_info->width) / 4); i++)
        {
            p_buff[i] = ZXIC_COMM_CONVERT32(p_buff[i]);

            /* for debug */
            ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "ZXIC_COMM_CONVERT32 data = 0x%08x.\n", p_buff[i]);
        }
    }
    
    /* 计算写地址 */
    addr = dpp_reg_get_reg_addr(reg_no, m_offset, n_offset);/* 通过寄存器编号等三个参数获得寄存器的基地址 */
    
    ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "reg_no = %d. m_offset = %d n_offset = %d\n", reg_no,m_offset,n_offset);
    ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "baseaddr = 0x%08x.\n", addr);

#ifdef DPP_FLOW_HW_INIT
    if(dpp_4k_reg(reg_module))
    {
        /* 调用寄存器写接口 */
        convert_addr = dpp_reg_addr_convert(DEV_ID(dev), reg_module, reg_type, addr);
        ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_reg_info->p_write_fun);
        rc = p_reg_info->p_write_fun(dev, convert_addr, p_buff);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "p_reg_info->p_write_fun");
    }
#else
    if(DTB4K == reg_module)
    {
        /* 调用寄存器写接口 */
        ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_reg_info->p_write_fun);
        rc = p_reg_info->p_write_fun(dev, addr, p_buff);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "p_reg_info->p_write_fun");
    }
#endif
    else
    {
        /* 调用代理通道寄存器写接口 */
        rc = dpp_agent_channel_reg_write(dev, reg_type, reg_no, reg_width, addr, p_buff); 
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_agent_channel_reg_write");  
    }

    return DPP_OK;
}

/***********************************************************/
/** 通用寄存器读函数
* @param   dev_id 设备号，支持多芯片
* @param   reg_no 寄存器编号
* @param   m_offset 二元寄存器的m偏移
* @param   n_offset 一元寄存器或二元寄存器的n偏移
* @param   p_data 数据指针
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_reg_read(DPP_DEV_T *dev, ZXIC_UINT32 reg_no, ZXIC_UINT32 m_offset, ZXIC_UINT32 n_offset, ZXIC_VOID *p_data)
{
    DPP_STATUS rc = 0;
    ZXIC_UINT32     i  = 0;
    ZXIC_UINT32     addr = 0;
#ifdef DPP_FLOW_HW_INIT
    ZXIC_UINT32     convert_addr = 0;
#endif
    ZXIC_UINT32     reg_type = 0;
    ZXIC_UINT32     p_buff[REG_DATA_MAX] = {0};
    ZXIC_UINT32     reg_module = 0;
    ZXIC_UINT32     reg_width = 0;
    DPP_REG_T *p_reg_info = NULL;
    DPP_FIELD_T *p_field_info = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev), DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), reg_no, 0, REG_ENUM_MAX_VALUE - 1);
    ZXIC_COMM_CHECK_POINT(p_data);

    p_reg_info = dpp_reg_info_get(reg_no);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_reg_info);
    p_field_info = p_reg_info->p_fields;
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_field_info);
    reg_type = p_reg_info->flags;
    reg_module = p_reg_info->module_no;
    reg_width = p_reg_info->width;
    ZXIC_COMM_CHECK_INDEX_UPPER(reg_width, REG_DATA_MAX*4);

#ifndef ZXIC_OS_WIN 
#ifdef DPP_FOR_LLT
    if (dpp_stump_reg_en_check(DEV_ID(dev), reg_no) && (p_reg_info->flags == DPP_REG_FLAG_DIRECT || p_reg_info->flags == DPP_REG_FLAG_WO | DPP_REG_FLAG_DIRECT))
    {
        rc = dpp_stump_reg_read(DEV_ID(dev),
                                reg_no,
                                m_offset,
                                n_offset,
                                p_data);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stump_reg_read");

        return DPP_OK;
    }
#endif
#endif

    /* 计算读地址 */
    addr = dpp_reg_get_reg_addr(reg_no, m_offset, n_offset);
#ifdef DPP_FLOW_HW_INIT
    if(dpp_4k_reg(reg_module))
    {
        /* 调用寄存器读接口 */
        convert_addr = dpp_reg_addr_convert(DEV_ID(dev), reg_module, reg_type, addr);
        ZXIC_COMM_CHECK_POINT(p_reg_info->p_read_fun);
        rc = p_reg_info->p_read_fun(dev, convert_addr, p_buff);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "p_reg_info->p_read_fun");
    }
#else
    if(DTB4K == reg_module)
    {
        /* 调用寄存器读接口 */
        ZXIC_COMM_CHECK_POINT(p_reg_info->p_read_fun);
        rc = p_reg_info->p_read_fun(dev, addr, p_buff);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "p_reg_info->p_read_fun");
    }
#endif
    else
    {
        /* 调用代理通道寄存器读接口 */
        rc = dpp_agent_channel_reg_read(dev, reg_type, reg_no, reg_width, addr, p_buff);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_agent_channel_reg_read");
    }

    /* 若host cpu为小端字节序，则以4字节为单位对数据进行字节序转换 */
    if (!zxic_comm_is_big_endian())
    {
        for (i = 0; i < ((p_reg_info->width) / 4); i++)
        {
            /* for debug */
            
            //printf("dpp_reg_read data = 0x%08x.\n", p_buff[i]);
            ZXIC_COMM_TRACE_DEV_DEBUG(DEV_ID(dev), "dpp_reg_read data = 0x%08x.\n", p_buff[i]);

            p_buff[i] = ZXIC_COMM_CONVERT32(p_buff[i]);
        }
    }

    /* 提取各字段数据，每字段以ZXIC_UINT32形式返回 */
    for (i = 0; i < p_reg_info->field_num; i++)
    {
        /* lint -e64 */
        rc = zxic_comm_read_bits_ex((ZXIC_UINT8 *)p_buff,
                              p_reg_info->width * 8,
                              (ZXIC_UINT32 *)p_data + i,
                              p_field_info[i].msb_pos,
                              p_field_info[i].len);
        ZXIC_COMM_CHECK_RC_NO_ASSERT(rc, "zxic_comm_read_bits_ex");
        /* lint +e64 */
    }

    return DPP_OK;
}

/***********************************************************/
/** 通过寄存器编号配置寄存器，仅适用于32bit位宽
    的常规寄存器
* @param   dev_id  设备号
* @param   reg_no     寄存器编号
* @param   data       数据，32bit
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  XXX      @date  2019/07/10
************************************************************/
DPP_STATUS dpp_reg_write32(DPP_DEV_T *dev, ZXIC_UINT32 reg_no, ZXIC_UINT32 data)
{
    DPP_STATUS  rc  = 0;
    ZXIC_UINT32      addr = 0;
    DPP_REG_T   *p_reg_info   = NULL;
    ZXIC_UINT32      value = data;
    ZXIC_UINT32      j = 0;
    ZXIC_UINT32      k = 0;
    ZXIC_UINT32      m_size = 0;
    ZXIC_UINT32      n_size = 0;
    ZXIC_UINT32      reg_type = 0;
    // ZXIC_UINT32      reg_module = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev), DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), reg_no, 0, REG_ENUM_MAX_VALUE - 1);

    p_reg_info = dpp_reg_info_get(reg_no);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_reg_info); 
    reg_type = p_reg_info->flags;
    // reg_module = p_reg_info->module_no;

    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_reg_info->width, 4, 4);       /* width must be 32bit */

    m_size = (p_reg_info->m_size == 0) ? (1) : (p_reg_info->m_size);
    n_size = (p_reg_info->n_size == 0) ? (1) : (p_reg_info->n_size);

    /* 计算读地址 */
    for (j = 0; j < m_size; j++)
    {
        for (k = 0; k < n_size; k++)
        {
#ifndef ZXIC_OS_WIN
#ifdef  DPP_FOR_LLT
            if (dpp_stump_reg_en_check(DEV_ID(dev), reg_no) && (p_reg_info->flags == DPP_REG_FLAG_DIRECT))
            {
                rc = dpp_stump_reg_write(DEV_ID(dev),
                                         reg_no,
                                         j,
                                         k,
                                         &data);
                ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stump_reg_write");

                return DPP_OK;
            }
#endif
#endif
            /* 计算写地址 */
            addr = dpp_reg_get_reg_addr(reg_no, j, k);

            /* 调用代理通道寄存器写接口 */
            rc = dpp_agent_channel_reg_write(dev, reg_type, reg_no, 4, addr, &value);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_agent_channel_reg_write");
        }
    }

    return DPP_OK;
}

/***********************************************************/
/** 通过寄存器编号读取寄存器的值，仅适用于32bit位宽的常规寄存器
* @param   dev_id   设备号
* @param   reg_no   寄存器编号
* @param   m_offset   二元寄存器的m偏移
* @param   n_offset   一元寄存器或二元寄存器的n偏移
* @param   p_data   出参，返回读取寄存器数值
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  石金锋      @date  2015/03/09
************************************************************/
DPP_STATUS dpp_reg_read32(DPP_DEV_T *dev, ZXIC_UINT32 reg_no, ZXIC_UINT32 m_offset, ZXIC_UINT32 n_offset, ZXIC_UINT32 *p_data)
{
    DPP_STATUS rc = 0;
    ZXIC_UINT32 addr = 0;
    ZXIC_UINT32 reg_type = 0;
    // ZXIC_UINT32 reg_module = 0;
    ZXIC_UINT32 p_buff[REG_DATA_MAX] = {0};

    DPP_REG_T *p_reg_info = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX_UPPER(DEV_ID(dev), DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), reg_no, 0, REG_ENUM_MAX_VALUE - 1);
    ZXIC_COMM_CHECK_POINT(p_data);

    p_reg_info = dpp_reg_info_get(reg_no);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_reg_info); 
    reg_type = p_reg_info->flags;
    // reg_module = p_reg_info->module_no;

    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_reg_info->width, 4, 4);       /* width must be 32bit */

#ifndef ZXIC_OS_WIN
#ifdef DPP_FOR_LLT
    if (dpp_stump_reg_en_check(DEV_ID(dev), reg_no) && (p_reg_info->flags == DPP_REG_FLAG_DIRECT))
    {
        rc = dpp_stump_reg_read(DEV_ID(dev),
                                reg_no,
                                m_offset,
                                n_offset,
                                p_data);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stump_reg_read");

        return DPP_OK;
    }
#endif
#endif

    /* 计算读地址 */
    addr = dpp_reg_get_reg_addr(reg_no, m_offset, n_offset);

    /* 调用代理通道寄存器读接口 */
    rc =dpp_agent_channel_reg_read(dev, reg_type, reg_no, 4, addr, p_buff);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_agent_channel_reg_read");
    *p_data = p_buff[0];

    return DPP_OK;
}

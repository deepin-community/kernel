/**************************************************************
* 版权所有 (C)2013-2020, 深圳市中兴通讯股份有限公司
* 文件名称 :        zxic_common.c
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者   :    xuchenxi_10235594
* 完成日期 :        2020/07/20
* DEPARTMENT:   有线开发四部-系统软件团队
* MANUAL_PERCENT: 0%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/
#include "zxic_common.h"

#if ZXIC_REAL("全局变量")
ZXIC_UINT32      g_zxic_malloc_num         = 0;
ZXIC_UINT32      g_zxic_malloc_size        = 0;     /* 单位字节 */
ZXIC_UINT32      g_zxic_vmalloc_num         = 0;
ZXIC_UINT32      g_zxic_vmalloc_size        = 0;     /* 单位字节 */
ZXIC_UINT32      g_zxic_byte_swap_en       = 1;
ZXIC_UINT32      g_zxic_comm_channel_max   = 4;
#endif

#if ZXIC_REAL("内存")
/***********************************************************/
/** 释放内存
* @param   p_data
*
* @return
* @remark  无
* @see
* @author  ChenWei10088471      @date  2014/02/07
************************************************************/
ZXIC_VOID ic_comm_free_record(void)
{
    if (g_zxic_malloc_num > 0)
    {
        g_zxic_malloc_num--;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("Note:g_zxicp_malloc_num is zero now\n");
    }
}

/***********************************************************/
/** 释放内存
* @param   p_data
*
* @return
* @remark  无
* @see
* @author  cq      @date  2025/06/30
************************************************************/
ZXIC_VOID ic_comm_vfree_record(void)
{
    if (g_zxic_vmalloc_num > 0)
    {
        g_zxic_vmalloc_num--;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("Note:g_zxicp_vmalloc_num is zero now\n");
    }
}

/***********************************************************/
/** 分配内存
* @param   size
*
* @return
* @remark  无
* @see
* @author  ChenWei10088471      @date  2014/02/07
************************************************************/
ZXIC_VOID* ic_comm_malloc_memory(ZXIC_UINT32 size)
{
    /* 独立安全测评 限定申请内存的大小 */
    if(size > ZXIC_MALLOC_MAX_B_SIZE)
    {
        ZXIC_COMM_TRACE_ERROR("malloc size err, size more than 200M \n");
        return ZXIC_NULL;
    }
    if (g_zxic_malloc_num < ZXIC_UINT32_MAX)
    {
        g_zxic_malloc_num++;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("Note:g_zxicp_malloc_num is maxvalue now, reset 0\n");
        g_zxic_malloc_num = 0;
    }

    if (g_zxic_malloc_size < (ZXIC_UINT32_MAX - size))
    {
        g_zxic_malloc_size += size;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("Note:g_zxic_malloc_size[0x%x] and size[0x%x] sum is over maxvalue now, reset 0\n", g_zxic_malloc_size, size);
        g_zxic_malloc_size = 0;
    }
    
    return kmalloc(size,GFP_KERNEL); 
}

/***********************************************************/
/** 分配内存
* @param   size
*
* @return
* @remark  无
* @see
* @author  cq      @date  2025/06/30
************************************************************/
ZXIC_VOID* ic_comm_vmalloc_memory(ZXIC_UINT32 size)
{
    /* 独立安全测评 限定申请内存的大小 */
    if(size > ZXIC_MALLOC_MAX_B_SIZE)
    {
        ZXIC_COMM_TRACE_ERROR("malloc size err, size more than 200M \n");
        return ZXIC_NULL;
    }
    if (g_zxic_vmalloc_num < ZXIC_UINT32_MAX)
    {
        g_zxic_vmalloc_num++;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("Note:g_zxicp_vmalloc_num is maxvalue now, reset 0\n");
        g_zxic_vmalloc_num = 0;
    }

    if (g_zxic_vmalloc_size < (ZXIC_UINT32_MAX - size))
    {
        g_zxic_vmalloc_size += size;
    }
    else
    {
        ZXIC_COMM_TRACE_INFO("Note:g_zxic_vmalloc_size[0x%x] and size[0x%x] sum is over maxvalue now, reset 0\n", g_zxic_vmalloc_size, size);
        g_zxic_vmalloc_size = 0;
    }
    
    return vmalloc(size);
}
#endif /* 内存 */

#if ZXIC_REAL("延时")
/***********************************************************/
/** 毫秒级延时
* @param   milliseconds
*
* @return
* @remark  无
* @see
* @author  ChenWei10088471      @date  2014/02/07
************************************************************/
// ZXIC_VOID zxic_comm_sleep(ZXIC_UINT32 milliseconds)
// {
//     /* 打桩测试不需要延时       modify by zhangjintao 2022.01.21 */
// #ifndef ZXIC_FOR_LLT
// #ifdef ZXIC_OS_WIN
//     Sleep(milliseconds);
// #else
//     ZXIC_COMM_CHECK_INDEX_MUL_OVERFLOW_NONE((ZXIC_UINT32)milliseconds, 1000);

//     msleep(milliseconds);
// #endif
// #endif
// }

/***********************************************************/
/** 微秒级延时,互坼锁中使用
* @param   milliseconds
*
* @return
* @remark  
* @see
* @author  fyl      @date  2020/04/09
************************************************************/
ZXIC_VOID zxic_comm_udelay(ZXIC_UINT32 microseconds)
{
#ifndef ZXIC_FOR_LLT

    ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW_NONE(microseconds, 1);

    udelay(microseconds);
#endif
}

/***********************************************************/
/** 毫秒级延时,互坼锁中使用
* @param   milliseconds
*
* @return
* @remark  无
* @see
* @author  ChenWei10088471      @date  2014/02/07
************************************************************/
ZXIC_VOID zxic_comm_delay(ZXIC_UINT32 milliseconds)
{
#ifndef ZXIC_FOR_LLT
    // ZXIC_UINT32 i = 0;

    ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW_RETURN(milliseconds, 1);
    // while (--milliseconds != 0)
    // {
    //     for (i = 0; i < 600; i++);
    // }
    mdelay(milliseconds);
#endif
}

/***********************************************************/
/** LINUX  :毫秒级延时
    WINDOWS:毫秒级延时
* @param   millisecond
*
* @return
* @remark  无
* @see
* @author  ChenWei10088471      @date  2014/02/07
************************************************************/
ZXIC_VOID zxic_comm_msleep(ZXIC_UINT32 millisecond)
{
#ifndef ZXIC_FOR_LLT
#ifdef ZXIC_OS_WIN
    Sleep(millisecond);
#else
    msleep(millisecond);
#endif
#endif
}

/***********************************************************/
/**获取时间函数，毫秒
* @param   total
* @param   masklen
*
* @return
* @remark  无
* @see
* @author  ChenWei10088471      @date  2014/02/07
************************************************************/
ZXIC_DOUBLE zxic_comm_get_ticks_ms()
{
#ifdef ZXIC_OS_WIN
    return (ZXIC_FLOAT)GetTickCount();
#else
#ifdef CGS_V5_693
    struct timespec tv = {0};

    /* CGS_V5_693 所在内核无 get_timespec64，使用 getnstimeofday 获取当前时间 */
    getnstimeofday(&tv);
    return (ZXIC_DOUBLE)1000 * tv.tv_sec + (ZXIC_DOUBLE)tv.tv_nsec / 1000;
#else
    struct timespec64 tv = {0};
    get_timespec64(&tv, ZXIC_NULL);
    return (ZXIC_DOUBLE)1000 * tv.tv_sec + (ZXIC_DOUBLE)tv.tv_nsec / 1000;
#endif
#endif
}
#endif

#if ZXIC_REAL("字节序")
/***********************************************************/
/** 判断CPU的大小端字节序
* @param   ZXIC_VOID
*
* @return  0-小端；1-大端
* @remark  无
* @see
* @author  ChenWei10088471      @date  2014/02/07
************************************************************/
ZXIC_RTN32 zxic_comm_is_big_endian(ZXIC_VOID)
{
    ZXIC_ENDIAN_U c_data;

    c_data.a = 1;

    if (c_data.b == 1)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/***********************************************************/
/** 字节序转换，以4字节为单位进行转序
* @param   p_uc_data
* @param   dw_byte_len
*
* @return
* @remark  无
* @see
* @author  ChenWei10088471      @date  2014/02/07
************************************************************/
ZXIC_VOID zxic_comm_swap(ZXIC_UINT8 *p_uc_data, ZXIC_UINT32 dw_byte_len)
{
    ZXIC_UINT32      dw_byte_num    = 0;
    ZXIC_UINT8       uc_byte_mode   = 0;
    ZXIC_UINT32       uc_is_big_flag = 0;
    ZXIC_UINT32      i              = 0;
    ZXIC_UINT16     *p_w_tmp        = ZXIC_NULL;
    ZXIC_UINT32     *p_dw_tmp       = ZXIC_NULL;

    if (g_zxic_byte_swap_en)
    {

        p_dw_tmp = (ZXIC_UINT32 *)(p_uc_data);

        uc_is_big_flag = zxic_comm_is_big_endian();

        if (uc_is_big_flag)
        {
            return;
        }
        else
        {
            dw_byte_num  = dw_byte_len >> 2;
            uc_byte_mode = dw_byte_len % 4 & 0xff;

            for (i = 0; i < dw_byte_num; i++)
            {
                (*p_dw_tmp) = ZXIC_COMM_CONVERT32(*p_dw_tmp);
                p_dw_tmp++;
            }

            if (uc_byte_mode > 1)
            {
                p_w_tmp = (ZXIC_UINT16 *)(p_dw_tmp);
                (*p_w_tmp) = ZXIC_COMM_CONVERT16(*p_w_tmp);
            }
        }
    }

    return;
}

/***********************************************************/
/** WORD32拼装成WORD64
* @param   hi   高32bit
* @param   lo   低32bit
*
* @return  WORD64
* @remark  无
* @see
* @author  pj      @date  2019/10/22
************************************************************/
ZXIC_UINT64 ZXIC_COMM_COUNTER64_BUILD(ZXIC_UINT32 hi, ZXIC_UINT32 lo)
{
    ZXIC_UINT64 value = hi;

    value = value << 32;
    value = value | lo;

    return value;
}
#endif

#if ZXIC_REAL("bit操作")
/***********************************************************/
/** 将数据写入缓存区的指定bit位置，一次只能写入32bit的数据
    p_base的低字节存放数据的低比特，高字节存放数据的高比特。
* @param   p_base 数据缓存区指针
* @param   base_size_bit 缓存总的bit位宽
* @param   data 数据, 比特顺序左低右高，小端比特序
* @param   start_bit 起始bit位置(必须小于结束bit位置)
* @param   end_bit 结束bit位置
*
* @return
* @remark  exp:
                ZXIC_UINT8 data0[4] = {0x22, 0x44, 0x66, 0x88};
                zxic_comm_write_bits(data0, 32, 0xAABBCC, 0,23);
                ZXIC_COMM_PRINT("0x%02X %02X %02X %02X \n", data0[0], data0[1], data0[2], data0[3]);
                输出:0xAA BB CC 88

* @see
* @author  ChenWei10088471      @date  2014/02/07
************************************************************/
ZXIC_RTN32 zxic_comm_write_bits(ZXIC_UINT8 * p_base, 
                           ZXIC_UINT32 base_size_bit, 
                           ZXIC_UINT32 data, 
                           ZXIC_UINT32 start_bit, 
                           ZXIC_UINT32 end_bit)
{
    ZXIC_UINT32   len               =  0;
    ZXIC_UINT32   start_byte_index  =  0;
    ZXIC_UINT32   end_byte_index    =  0;
    ZXIC_UINT8     mask_value        =  0;
    ZXIC_UINT32   byte_num          =  0;
    ZXIC_UINT32   buffer_size       =  0;

    if (0 != (base_size_bit % 8))
    {
        ZXIC_COMM_TRACE_ERROR("\n buffer must be:%d", __LINE__);
        //assert(0);
        return ZXIC_BIT_STREAM_INDEX_ERR;
    }

    if (start_bit > end_bit)
    {
        ZXIC_COMM_TRACE_ERROR("\nend_bit cannot be less than start_bit:%d", __LINE__);
        //assert(0);
        return ZXIC_BIT_STREAM_INDEX_ERR;
    }

    if (base_size_bit < end_bit)
    {
        ZXIC_COMM_TRACE_ERROR("\nend_bit exceeds the base_size!line:%d,base_size_bit:%d end_bit:%d",
                             __LINE__, base_size_bit, end_bit);
        //assert(0);
        return ZXIC_BIT_STREAM_INDEX_ERR;
    }

    ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW(end_bit, start_bit);
    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW(end_bit - start_bit, 1);

    len = end_bit - start_bit + 1;
    buffer_size = base_size_bit / 8;

    /*寄存器位宽需要时2的次方，如不是，需要累加到最近的2的次方*/
    /*用于解决KW检查中zxic_comm_write_bits_ex出现的内存泄漏错误*/
    ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW(buffer_size, 1);
    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW(buffer_size, 1);

    while (0 != (buffer_size & (buffer_size - 1)))
    {
        buffer_size += 1;
    }

    if (buffer_size != base_size_bit / 8)
    {
        ZXIC_COMM_TRACE_ALL("\n buffer size[0x%x] is not 2^n: add up to [0x%x]", base_size_bit / 8, buffer_size);
    }



    /*一次只能写32bit*/
    ZXIC_COMM_CHECK_INDEX(len, 1, 32);

    if (data > (ZXIC_UINT32)(0xffffffff >> (32 - len)))
    {
        ZXIC_COMM_PRINT("\nValue is too big to write in the bit field!:%d,data:%x,len:%x",
                  __LINE__, data, (ZXIC_UINT32)(0xffffffff >> (32 - len)));
        return ZXIC_BIT_STREAM_DATA_TOO_BIG;
    }

    end_byte_index     = (end_bit    >> 3);
    start_byte_index   = (start_bit  >> 3);

    if (start_byte_index == end_byte_index)
    {
        mask_value  = ((0xFE << (7 - (start_bit & 7))) & 0xff);
        mask_value |= (((1 << (7 - (end_bit  & 7))) - 1)& 0xff);
        p_base[end_byte_index] &= mask_value;
        p_base[end_byte_index] |= (((data << (7 - (end_bit & 7)))) & 0xff);
        return ZXIC_OK;
    }

    if (7 != (end_bit & 7))
    {
        mask_value = ((0x7f >> (end_bit  & 7)) & 0xff);
        p_base[end_byte_index] &= mask_value;
        p_base[end_byte_index] |= ((data << (7 - (end_bit & 7))) & 0xff);
        end_byte_index--;
        data >>= 1 + (end_bit  & 7);
    }

    for (byte_num = end_byte_index; byte_num > start_byte_index; byte_num--)
    {
        /* critical */
        p_base[byte_num & (buffer_size - 1)] = data & 0xff;
        data >>= 8;
    }

    mask_value        = ((0xFE << (7 - (start_bit  & 7))) & 0xff);
    p_base[byte_num] &= mask_value;
    p_base[byte_num] |= data;

    return ZXIC_OK;
}

/***********************************************************/
/** 从缓存区中读取指定bit位置的数据，一次最多只能读32bit
* @param   p_base 数据缓存区指针
* @param   base_size_bit 缓存区总的bit位宽
* @param   p_data 返回数据的指针
* @param   start_bit 起始bit位置(必须小于结束bit位置)
* @param   end_bit 结束bit位置
*
* @return
* @remark  exp:
            ZXIC_UINT8  data0[4] = {0x22, 0x44, 0x66, 0x88};
            ZXIC_UINT32 test_a = 0;
            zxic_comm_read_bits(data0, 32, &test_a, 0, 23);
            输出: test_a = 0x224466
* @see
* @author  ChenWei10088471      @date  2014/02/07
************************************************************/
ZXIC_RTN32 zxic_comm_read_bits(ZXIC_UINT8* p_base, 
                          ZXIC_UINT32 base_size_bit, 
                          ZXIC_UINT32* p_data, 
                          ZXIC_UINT32 start_bit, 
                          ZXIC_UINT32 end_bit)
{
    ZXIC_UINT32   len               =  0;
    ZXIC_UINT32   start_byte_index  =  0;
    ZXIC_UINT32   end_byte_index    =  0;
    ZXIC_UINT32   byte_num          =  0;
    ZXIC_UINT32   buffer_size       =  0;

    if (0 != (base_size_bit % 8))
    {
        ZXIC_COMM_TRACE_ERROR("\n buffer must be:%d", __LINE__);
        return ZXIC_BIT_STREAM_INDEX_ERR;
    }

    if (start_bit > end_bit)
    {
        ZXIC_COMM_TRACE_ERROR("\nend_bit cannot be less than start_bit:%d", __LINE__);
        return ZXIC_BIT_STREAM_INDEX_ERR;
    }

    if (base_size_bit < end_bit)
    {
        ZXIC_COMM_TRACE_ERROR("\nend_bit exceeds the base_size:%d,end_bit:%d", __LINE__, end_bit);
        return ZXIC_BIT_STREAM_INDEX_ERR;
    }

    len = end_bit - start_bit + 1;
    buffer_size = base_size_bit / 8;

    /*寄存器位宽需要时2的次方，如不是，需要累加到最近的2的次方*/
    /*用于解决KW检查中zxic_comm_read_bits_ex出现的内存泄漏错误*/
    ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW(buffer_size, 1);

    while (0 != (buffer_size & (buffer_size - 1)))
    {
        buffer_size += 1;
    }

    if (buffer_size != base_size_bit / 8)
    {
        ZXIC_COMM_TRACE_ALL("\n buffer size[0x%x] is not 2^n: add up to [0x%x]", base_size_bit / 8, buffer_size);
    }

    /*先将返回的数据清零*/
    *p_data = 0;

    /*一次最多只能读32bit*/
    ZXIC_COMM_CHECK_INDEX(len, 1, 32);

    end_byte_index     = (end_bit    >> 3);
    start_byte_index   = (start_bit  >> 3);

    if (start_byte_index == end_byte_index)
    {
        *p_data = (ZXIC_UINT32)(((p_base[start_byte_index] >> (7U - (end_bit & 7))) & (0xff >> (8U - len))) & 0xff);
        return ZXIC_OK;
    }

    if (start_bit & 7)
    {
        *p_data = (p_base[start_byte_index] & (0xff >> (start_bit & 7))) & ZXIC_UINT8_MASK;
        start_byte_index++;
    }

    for (byte_num = start_byte_index; byte_num < end_byte_index; byte_num++)
    {
        *p_data <<= 8;
        
        ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW(*p_data, p_base[byte_num]);      
        *p_data  += p_base[byte_num];
    }

    *p_data <<= 1 + (end_bit & 7);
    /* critical */
    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW(*p_data, (((p_base[byte_num & (buffer_size - 1)] & (0xff << (7 - (end_bit  & 7)))) >> (7 - (end_bit  & 7))) & 0xff));
    *p_data  += ((p_base[byte_num & (buffer_size - 1)] & (0xff << (7 - (end_bit  & 7)))) >> (7 - (end_bit  & 7))) & 0xff;

    return ZXIC_OK;
}

/***********************************************************/
/** 比特流拼装，比特流的形式为: p_base的低字节存放
    数据的高比特，高字节存放数据的低比特。
* @param   p_base
* @param   base_size_bit
* @param   data           数据, 比特顺序左高右低，大端比特序
* @param   msb_start_pos  数据最高比特的位置
* @param   len            数据长度
*
* @return
* @remark  exp:
                ZXIC_UINT8 data0[4] = {0x22, 0x44, 0x66, 0x88};
                zxic_comm_write_bits_ex(data0, 32, 0x123456, 23,24);
                ZXIC_COMM_PRINT("0x%02X %02X %02X %02X \n", data0[0], data0[1], data0[2], data0[3]);
                输出：0x22 12 34 56

* @see
************************************************************/
ZXIC_RTN32 zxic_comm_write_bits_ex(ZXIC_UINT8 * p_base, 
                              ZXIC_UINT32 base_size_bit, 
                              ZXIC_UINT32 data, 
                              ZXIC_UINT32 msb_start_pos, 
                              ZXIC_UINT32 len)
{
    ZXIC_RTN32 rtn = ZXIC_OK;
    ZXIC_COMM_CHECK_POINT(p_base);

    ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW(base_size_bit, 1);
    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW(base_size_bit - 1, msb_start_pos);
    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW(base_size_bit - 1 - msb_start_pos, len);
    ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW(base_size_bit - 1 - msb_start_pos + len, 1);

    rtn = zxic_comm_write_bits(p_base,
                         base_size_bit,
                         data,
                         (base_size_bit - 1 - msb_start_pos),
                         (base_size_bit - 1 - msb_start_pos + len - 1));

    return rtn;
}

/***********************************************************/
/** 从比特流中读取数据，比特流的形式为: p_base的低字节存放
    数据的高比特，高字节存放数据的低比特。
* @param   p_base
* @param   base_size_bit
* @param   p_data
* @param   msb_start_pos
* @param   len
*
* @return
* @remark  exp:
                ZXIC_UINT8 data0[4] = {0x22, 0x44, 0x66, 0x88};
                zxic_comm_read_bits_ex(data0, 32, &test_a, 23, 24);
                ZXIC_COMM_PRINT("0test_a = 0x%x \n", test_a);
                输出: 0test_a = 0x446688
                
* @see
************************************************************/
ZXIC_RTN32 zxic_comm_read_bits_ex(ZXIC_UINT8 * p_base, 
                             ZXIC_UINT32 base_size_bit, 
                             ZXIC_UINT32 * p_data, 
                             ZXIC_UINT32 msb_start_pos, 
                             ZXIC_UINT32 len)
{
    ZXIC_RTN32 rtn = ZXIC_OK;
    ZXIC_COMM_CHECK_POINT(p_base);
    ZXIC_COMM_CHECK_POINT(p_data);

    ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW(base_size_bit, 1);
    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW(base_size_bit - 1, msb_start_pos);
    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW(base_size_bit - 1 - msb_start_pos, len);
    ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW(base_size_bit - 1 - msb_start_pos + len, 1);

    rtn = zxic_comm_read_bits(p_base,
                        base_size_bit,
                        p_data,
                        (base_size_bit - 1 - msb_start_pos),
                        (base_size_bit - 1 - msb_start_pos + len - 1));
    return rtn;
}
#endif /*  */

#if ZXIC_REAL("字符串")
/***********************************************************/
/** 
* @param   buffer   目标字符串
* @param   sizeofbuf  sizeofbuffer
* @param   count  要拷贝字节数
* @param   format
*
* @return  待拷贝的实际字符串长度
* @remark  snprintf
* @see     
* @author  sj      @date  2020/12/09
************************************************************/
ZXIC_SINT32 ic_comm_snprintf_s(ZXIC_CHAR *buffer, ZXIC_SIZE_T sizeofbuf, ZXIC_SIZE_T count, const ZXIC_CHAR *format, ...)
{
    va_list ap;
    ZXIC_SINT32 ret = -1;
    
    if ((ZXIC_NULL == buffer)||(ZXIC_NULL == format))
    { 
        ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);
        return ret;
    }
    if (!count)
    {    
        ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:count err]\n", __FILE__, __LINE__);       
        return ret;
    }
    va_start(ap, format);
    ret = ZXIC_COMM_VSNPRINTF_S(buffer, sizeofbuf, count, format, ap);  
    va_end(ap);
    
    if (ret == -1)
    {
        ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:snprintf_s err]\n", __FILE__, __LINE__);
    }    
    return ret;
}
/***********************************************************/
/** 
* @param   buffer   目标字符串
* @param   sizeofbuf  sizeofbuffer
* @param   count  要拷贝字节数
* @param   format
*
* @return  待拷贝的实际字符串长度
* @remark  vsnprintf
* @see     
* @author  sj      @date  2020/12/09
************************************************************/
ZXIC_SINT32 ic_comm_vsnprintf_s(ZXIC_CHAR *buffer, ZXIC_SIZE_T sizeofbuf, ZXIC_SIZE_T count, const ZXIC_CHAR *format, va_list ap)
{
    ZXIC_SINT32 ret = -1;
    
    if ((ZXIC_NULL == buffer)||(ZXIC_NULL == format))
    { 
        ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);
        return ret;
    }
    if (!count)
    {    
        ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:count err]\n", __FILE__, __LINE__);       
        return ret;
    }
    if (count < sizeofbuf)
    {
        sizeofbuf = count;
    }  
#ifdef ZXIC_OS_WIN
    ret = _vsnprintf(buffer, sizeofbuf, format, ap);
#else
    ret = vsnprintf(buffer, sizeofbuf, format, ap);
#endif
    if (ret == -1)
    {
        ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:vsnprintf err]\n", __FILE__, __LINE__);
    }  
    return ret;
}

/***********************************************************/
/** 
* @param   pcDst 　　 目的地址
* @param   dwMaxSize　目的长度
* @param   pcSrc 　　 源地址
* @param   dwCount　  要复制的最大字符数
*
* @return  
* @remark  将源地址的字符拷贝到目的字符数组;
* @see     
* @author  
************************************************************/
ZXIC_CHAR *ic_comm_strncpy_s( ZXIC_CHAR *pcDst, size_t dwMaxSize, ZXIC_CONST ZXIC_CHAR *pcSrc, size_t dwCount )
{
    size_t  dwIndex = 1, dwCopyNum = dwCount;
    ZXIC_CHAR   *pcResult = pcDst;

    ZXIC_COMM_CHECK_RC_POINT_NO_PRINT(pcDst, pcResult);
    ZXIC_COMM_CHECK_RC_POINT_NO_PRINT(pcSrc, pcResult);

    if (( dwMaxSize <= 1 ) || (dwMaxSize > ZXIC_COMM_MEMORY_MAX_B_SIZE) || ( dwCount == 0 ) )
    {
        return pcResult;
    }

    /* 计算拷贝的字符长度，不含结束符 */
    if ( dwCount >= dwMaxSize )
    {
        dwCopyNum = dwMaxSize - 1;
    }

    if (ic_comm_getAbsValue((unsigned ZXIC_CHAR*)pcDst, (ZXIC_CONST ZXIC_UINT8*)pcSrc) < dwCopyNum)
    {
        return pcResult;
    }

    while ( '\0' != ( *pcDst++ = *pcSrc++ ) )
    {
        /* 判断拷贝字符数是否等于dwCopyNum，等于就退出 */
        /* 由于判断放在循环体内，进行判断前已经拷贝了一次，所以i初试值为1 */
        if ( dwIndex++ >= dwCopyNum )
        {
            *pcDst = '\0';

            return pcResult;
        }
    }

    /* 本处的处理是为了保持和库函数中的解释一致，对于源串长度小于dwCopyNum，剩余部分全部填0 */
    while ( dwIndex++ <= dwCopyNum )
    {
        *pcDst++ = '\0';
    }

    return pcResult;
}

ZXIC_SIZE_T ic_comm_getAbsValue(ZXIC_UINT8* dest, ZXIC_CONST ZXIC_UINT8* src)
{
    return dest > src ? (dest - src) : (src - dest);
}
/***********************************************************/
/** 
* @param   dest 　　目的地址
* @param   src 　源地址
* @param   n　　 要复制的长度
*
* @return  
* @remark  从源地址拷贝若干字节的长度到目的内存处
* @see     
* @author  
************************************************************/
ZXIC_RTN32 ic_comm_memcpy(ZXIC_VOID* dest, ZXIC_CONST ZXIC_VOID* src, size_t n)
{
    ZXIC_COMM_CHECK_POINT(dest);
    ZXIC_COMM_CHECK_POINT(src);
    
    /* memcpy 的大小限制在200M */
    if(n > 200 * 1024 * 1024)
    {
        return ZXIC_PAR_CHK_INVALID_PARA;
    }

    if (ic_comm_getAbsValue((ZXIC_UINT8*)dest, (ZXIC_CONST ZXIC_UINT8*)src) < n)
    {
        return ZXIC_ERR;
    }
#ifdef ZXIC_OS_WIN
    memcpy(dest, src, n);
#else
    //__memcpy_chk(dest, src, n, n);
    memcpy(dest,src,n);
#endif
    return ZXIC_OK;
}
/***********************************************************/
/** 
* @param   dest 　　目的地址
* @param   dest_len　目的长度
* @param   src 　源地址
* @param   n　　 要复制的长度
*
* @return  
* @remark  从源地址拷贝若干字节的长度到目的内存处,增加源目的长度之间的检查
* @see     
* @author  
************************************************************/
ZXIC_RTN32 ic_comm_memcpy_s(ZXIC_VOID* dest, size_t dest_len, const void* src, size_t n)
{
    ZXIC_COMM_CHECK_POINT(dest);
    ZXIC_COMM_CHECK_POINT(src);

    /* memcpy 的大小限制在200M */
    if(n > 200 * 1024 * 1024)
    {
        return ZXIC_PAR_CHK_INVALID_PARA;
    }

    if (ic_comm_getAbsValue((ZXIC_UINT8 *)dest, (ZXIC_CONST ZXIC_UINT8*)src) < n)
    {
        return ZXIC_PAR_CHK_ARGIN_ERROR;
    }
#ifdef ZXIC_OS_WIN
    if (dest_len < n)
    {
        return ZXIC_ERR;
    }
    memcpy(dest, src, n);
#else
    //__memcpy_chk(dest, src, n, dest_len);
    memcpy(dest, src, n);
#endif
    return ZXIC_OK;
}

/***********************************************************/
/** 
* @param   pcDst 　　 目的地址
* @param   dwMaxSize　目的长度
* @param   pcSrc 　　 源地址
* @param   dwCount 　 待连接的字符数
*
* @return  
* @remark  字符串连接函数，将pcSrc的dwCount字符复制到pcDst的字符串后，覆盖"\0";
* @see     
* @author  
************************************************************/
ZXIC_CHAR *ic_comm_strncat_s( ZXIC_CHAR *pcDst, size_t dwMaxSize, ZXIC_CONST ZXIC_CHAR *pcSrc, size_t dwCount )
{
    ZXIC_SIZE_T  dwIndex = 1, dwCopyNum = 0;
    ZXIC_CHAR   *pcResult = pcDst;

    ZXIC_COMM_CHECK_RC_POINT_NO_PRINT(pcDst, pcResult);
    ZXIC_COMM_CHECK_RC_POINT_NO_PRINT(pcSrc, pcResult);

    if (( dwMaxSize == 0 ) || (dwMaxSize > ZXIC_COMM_MEMORY_MAX_B_SIZE) || ( dwCount == 0 ) )
    {
        return pcResult;
    }

    /* 计算目的串的长度 */
    while ( ( *pcDst++ != '\0' ) && ( ++dwCopyNum < dwMaxSize ) )
    {
    }

    if ( dwCopyNum >= dwMaxSize )
    {
        return pcResult;
    }

    dwCopyNum = dwMaxSize - dwCopyNum;  /* 计算剩余的缓冲区长度 */

    /* 计算拷贝的字符长度，不含结束符 */
    if ( dwCount >= dwCopyNum )
    {
        dwCopyNum = dwCopyNum - 1;
    }
    else
    {
        dwCopyNum = dwCount;
    }

    if (dwCopyNum == 0) /* 修正pcDst 空间刚好满导致内存越界的问题 */
    {
        return pcResult;
    }

    pcDst --;

    if (ic_comm_getAbsValue((unsigned ZXIC_CHAR*)pcDst, (const unsigned ZXIC_CHAR*)pcSrc) < dwCopyNum)
    {
        return pcResult;
    }

    while ( '\0' != ( *pcDst++ = *pcSrc++ ) )
    {
        /* 判断拷贝字符数是否等于dwCopyNum，等于就退出 */
        /* 由于判断放在循环体内，进行判断前已经拷贝了一次，所以i初试值为1 */
        if ( dwIndex++ >= dwCopyNum )
        {
            *pcDst = '\0';

            return pcResult;
        }
    }

    /* 本处的处理是为了保持和库函数中的解释一致，对于源串长度小于dwCopyNum，剩余部分全部填0 */
    while ( dwIndex++ <= dwCopyNum )
    {
        *pcDst++ = '\0';
    }

    return pcResult;
}

/***********************************************************/
/** 
* @param   str 　　字符串首地址
* @param   MaxCount　可返回的最大长度,若计算的长度大于该长度，则返回MaxCount；
*
* @return  
* @remark  计算字符串的长度
* @see     
* @author  
************************************************************/
ZXIC_SIZE_T ic_comm_strnlen_s( const ZXIC_CHAR *str, ZXIC_SIZE_T MaxCount)
{
    return (str == 0)? 0: ZXIC_COMM_STRNLEN(str, MaxCount);
}

ZXIC_VOID ic_comm_memset_s(void* dest, ZXIC_SIZE_T dmax, ZXIC_UINT8 c, ZXIC_SIZE_T n)
{
    if ((ZXIC_NULL == dest) || (dmax > ZXIC_COMM_MEMORY_MAX_B_SIZE) || (0 == n) || (n > dmax))
    {
        ZXIC_COMM_TRACE_ERROR("zxic_memset_s para err:ptr is null or size err.\n");
        return;
    }
    memset(dest, c, n);
}

ZXIC_SINT32 ic_comm_memcmp(void* str1, const void* str2, ZXIC_SIZE_T n)
{
    if ((ZXIC_NULL == str1) || (ZXIC_NULL == str2)|| (n > ZXIC_COMM_MEMORY_MAX_B_SIZE) || (0 == n))
    {   
        ZXIC_COMM_TRACE_ERROR("zxic_memcmp para err:ptr is null or size more than 200M.\n");
        return 0x7fffffff;
    }
    return memcmp(str1, str2, n);
}
#endif

ZXIC_RTN32 zxic_comm_random()
{

#ifdef ZXIC_OS_WIN
    /* return RtlGenRandom(); Modify by wcl, 20200424, Windows版本编译失败,先改回rand */
    return rand();
#else
    ZXIC_UINT8 buff[4] = {0};
    ZXIC_UINT32 ticks = 0;
    ZXIC_UINT32 random_d = 0;
#ifdef CGS_V5_693
    struct timespec tv;
#else
    struct timespec64 tv;
#endif
    ZXIC_UINT32 result_len = 0;
    struct file *fd = NULL;
    loff_t pos = 0;

    //fd =  open("/dev/urandom", O_RDONLY);
    fd = filp_open("/dev/urandom",O_RDONLY,0);

    if (NULL == fd)
    {
#ifdef CGS_V5_693
        getnstimeofday(&tv);
#else
        get_timespec64(&tv, ZXIC_NULL);
#endif
        ticks = ((tv.tv_sec & ZXIC_UINT32_MAX) + (tv.tv_nsec & ZXIC_UINT32_MAX)) & ZXIC_UINT32_MAX;
        random_d = ticks;
    }
    else if (
#ifdef CGS_V5_693
             (result_len = (kernel_read(fd, pos, buff, ZXIC_SIZEOF(buff)) & 0xFFFFFFFF)) != ZXIC_SIZEOF(buff)
#else
             (result_len = (kernel_read(fd, buff, ZXIC_SIZEOF(buff), &pos) & 0xFFFFFFFF)) != ZXIC_SIZEOF(buff)
#endif
            )
    {
#ifdef CGS_V5_693
        getnstimeofday(&tv);
#else
        get_timespec64(&tv, ZXIC_NULL);
#endif
        ticks = ((tv.tv_sec & ZXIC_UINT32_MAX) + (tv.tv_nsec & ZXIC_UINT32_MAX)) & ZXIC_UINT32_MAX;
        random_d = ticks;
        filp_close(fd,NULL);
    }
    else
    {
        random_d = (((buff[0] << 24) & 0xff000000) |
                    ((buff[1] << 16) & 0x00ff0000) |
                    ((buff[2] << 8)  & 0x0000ff00) |
                     (buff[3] & 0x000000ff));
        filp_close(fd,NULL);
    }

    return random_d;
#endif

}

/***********************************************************/
/** 设置最大通道数
* @param   dev_max      设置的最大设备数
*
* @return
* @remark  无
* @see
* @author  lihk      @date  2021/03/31
************************************************************/
ZXIC_VOID zxic_comm_channel_max_set(ZXIC_UINT32 dev_max)
{
    g_zxic_comm_channel_max = dev_max;
}

/***********************************************************/
/** 获取最大通道数
* @param
*
* @return
* @remark  无
* @see
* @author  lihk      @date  2021/03/31
************************************************************/
ZXIC_RTN32 zxic_comm_channel_max_get(ZXIC_VOID)
{
    return g_zxic_comm_channel_max;
}

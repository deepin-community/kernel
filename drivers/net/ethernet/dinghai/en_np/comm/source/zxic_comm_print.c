/**************************************************************
* 版权所有 (C)2013-2020, 深圳市中兴通讯股份有限公司
* 文件名称 : zxic_comm_print.c
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者   : xuchenxi_10235594
* 完成日期 : 2020/07/20
* DEPARTMENT: 有线开发四部-系统软件团队
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/
#include "zxic_common.h"
#include "zxic_private.h"
#include "log.h"

#if ZXIC_REAL("全局变量定义")
ZXIC_UINT32  g_zxic_print_level        = ZXIC_TRACE_ERROR_PRINT;    /*打印级别,默认为INFO*/
ZXIC_UINT32  g_zxic_print_en           = 1;                         /*界面打印控制开关*/
#endif

#define ZXIC_COMM_TRACE_BUFFER_SIZE    (512)

#if ZXIC_REAL("开关")
/***********************************************************/
/** 设置打印开关，决定ZXIC_COMM_PRINT等调试打印函数是否输出到屏幕
* @param   enable  0-不打印到屏幕，1-打印到屏幕
*
* @return
* @remark  无
* @see
* @author  zhaisyu      @date  2018/10/25
************************************************************/
ZXIC_VOID zxic_comm_set_print_en(ZXIC_UINT32 enable)
{
    g_zxic_print_en = enable;
}

ZXIC_RTN32 zxic_comm_get_print_en(ZXIC_VOID)
{
    return g_zxic_print_en;
}

/***********************************************************/
/** 设置Debug打印级别
* @param   debug_level 0打印级别最低，4打印级别最高，即打印的东西最多
*
* @return
* @remark  无
* @see
* @author  ChenWei10088471      @date  2014/02/07
************************************************************/
ZXIC_VOID zxic_comm_set_print_level(ZXIC_UINT32 debug_level)
{
    g_zxic_print_level = debug_level;
}

/***********************************************************/
/** 获取Debug打印级别
* @param   ZXIC_VOID
*
* @return
* @remark  无
* @see
* @author  ChenWei10088471      @date  2014/02/07
************************************************************/
ZXIC_RTN32 zxic_comm_get_print_level(ZXIC_VOID)
{
    return g_zxic_print_level;
}

#endif

#if ZXIC_REAL("打印函数")
#if 0
ZXIC_VOID ZXIC_COMM_PRINT(ZXIC_CONST ZXIC_CHAR *format, ...)
{
    va_list   ap;
    ZXIC_CHAR szBuffer[ZXIC_COMM_TRACE_BUFFER_SIZE];

    ZXIC_COMM_ASSERT(format);

    va_start(ap, format);
    {
        ZXIC_COMM_VSNPRINTF_S(szBuffer, ZXIC_SIZEOF(szBuffer), ZXIC_SIZEOF(szBuffer), format, ap);
        if (g_zxic_print_en)
        {
            DH_LOG_INFO(MODULE_NP, "%s", szBuffer);
        }
    }
    va_end(ap);
}

/***********************************************************/
/** Error信息打印函数
* @param   format
*
* @return
* @remark  无
* @see
* @author  ChenWei10088471      @date  2014/02/07
************************************************************/
ZXIC_VOID ZXIC_COMM_TRACE_ERROR(ZXIC_CONST ZXIC_CHAR *format, ...)
{
    va_list   ap;
    ZXIC_CHAR szBuffer[ZXIC_COMM_TRACE_BUFFER_SIZE];

    ZXIC_COMM_ASSERT(format);

    if (zxic_comm_get_print_level() == 0 || zxic_comm_get_print_level() >= ZXIC_TRACE_INVALID_PRINT)
    {
        return;
    }

    va_start(ap, format);
    {
        ZXIC_COMM_VSNPRINTF_S(szBuffer, ZXIC_SIZEOF(szBuffer), ZXIC_SIZEOF(szBuffer), format, ap);
        if (zxic_comm_get_print_en())
        {
            DH_LOG_ERR(MODULE_NP, "%s", szBuffer);
        }
    }
    va_end(ap);
}

/***********************************************************/
/** Notice信息打印函数
* @param   format
*
* @return
* @remark  无
* @see
* @author  ChenWei10088471      @date  2014/02/07
************************************************************/
ZXIC_VOID ZXIC_COMM_TRACE_NOTICE(ZXIC_CONST ZXIC_CHAR *format, ...)
{
    va_list   ap;
    ZXIC_CHAR szBuffer[ZXIC_COMM_TRACE_BUFFER_SIZE];

    ZXIC_COMM_ASSERT(format);

    if (zxic_comm_get_print_level() == 0 || zxic_comm_get_print_level() >= ZXIC_TRACE_INVALID_PRINT)
    {
        return;
    }

    if (zxic_comm_get_print_level() >= ZXIC_TRACE_NOTICE_PRINT)
    {
        va_start(ap, format);
        {
            ZXIC_COMM_VSNPRINTF_S(szBuffer, ZXIC_SIZEOF(szBuffer), ZXIC_SIZEOF(szBuffer), format, ap);
            if (zxic_comm_get_print_en())
            {
                DH_LOG_INFO(MODULE_NP, "%s", szBuffer);
            }
        }
        va_end(ap);
    }
}

/***********************************************************/
/** Info信息打印函数
* @param   format
*
* @return
* @remark  无
* @see
* @author  ChenWei10088471      @date  2014/02/07
************************************************************/
ZXIC_VOID ZXIC_COMM_TRACE_INFO(ZXIC_CONST ZXIC_CHAR *format, ...)
{
    va_list   ap;
    ZXIC_CHAR szBuffer[ZXIC_COMM_TRACE_BUFFER_SIZE];

    ZXIC_COMM_ASSERT(format);

    if (zxic_comm_get_print_level() == 0 || zxic_comm_get_print_level() >= ZXIC_TRACE_INVALID_PRINT)
    {
        return;
    }

    if (zxic_comm_get_print_level() >= ZXIC_TRACE_INFO_PRINT)
    {
        va_start(ap, format);
        {
            ZXIC_COMM_VSNPRINTF_S(szBuffer, ZXIC_SIZEOF(szBuffer), ZXIC_SIZEOF(szBuffer), format, ap);
            if (zxic_comm_get_print_en())
            {
                DH_LOG_INFO(MODULE_NP, "%s", szBuffer);
            }
        }
        va_end(ap);
    }
}

/***********************************************************/
/** Debug信息打印函数
* @param   format
*
* @return
* @remark  无
* @see
* @author  ChenWei10088471      @date  2014/02/07
************************************************************/
ZXIC_VOID ZXIC_COMM_TRACE_DEBUG(ZXIC_CONST ZXIC_CHAR *format, ...)
{
    va_list   ap;
    ZXIC_CHAR szBuffer[ZXIC_COMM_TRACE_BUFFER_SIZE];

    ZXIC_COMM_ASSERT(format);

    if (zxic_comm_get_print_level() == 0 || zxic_comm_get_print_level() >= ZXIC_TRACE_INVALID_PRINT)
    {
        return;
    }

    if (zxic_comm_get_print_level() >= ZXIC_TRACE_DEBUG_PRINT)
    {
        va_start(ap, format);
        {
            ZXIC_COMM_VSNPRINTF_S(szBuffer, ZXIC_SIZEOF(szBuffer), ZXIC_SIZEOF(szBuffer), format, ap);
            if (zxic_comm_get_print_en())
            {
                DH_LOG_DEBUG(MODULE_NP, "%s", szBuffer);
            }
        }
        va_end(ap);
    }
}

/***********************************************************/
/** 所有调试信息打印函数
* @param   format
*
* @return
* @remark  无
* @see
* @author  ChenWei10088471      @date  2014/02/07
************************************************************/
ZXIC_VOID ZXIC_COMM_TRACE_ALL(ZXIC_CONST ZXIC_CHAR *format, ...)
{
    va_list   ap;
    ZXIC_CHAR szBuffer[ZXIC_COMM_TRACE_BUFFER_SIZE];

    ZXIC_COMM_ASSERT(format);

    if (zxic_comm_get_print_level() == 0 || zxic_comm_get_print_level() >= ZXIC_TRACE_INVALID_PRINT)
    {
        return;
    }

    if (zxic_comm_get_print_level() >= ZXIC_TRACE_ALL_PRINT)
    {
        va_start(ap, format);
        {
            ZXIC_COMM_VSNPRINTF_S(szBuffer, ZXIC_SIZEOF(szBuffer), ZXIC_SIZEOF(szBuffer), format, ap);
            if (zxic_comm_get_print_en())
            {
                DH_LOG_DEBUG(MODULE_NP, "%s", szBuffer);
            }
        }
        va_end(ap);
    }
}

/***********************************************************/
/** 支持多芯片的Error信息打印函数
*   打印级别为1及以上时执行打印
* @param   dev_id
* @param   format
*
* @return
* @remark  无
* @see
* @author  xcx                  @date  2020/07/20
************************************************************/
ZXIC_VOID ZXIC_COMM_TRACE_DEV_ERROR(ZXIC_UINT32 dev_id, ZXIC_CONST ZXIC_CHAR *format, ...)
{
    va_list   ap;
    ZXIC_CHAR szBuffer[ZXIC_COMM_TRACE_BUFFER_SIZE - 32] = {0};
    ZXIC_CHAR devBuffer[ZXIC_COMM_TRACE_BUFFER_SIZE]     = {0};

    ZXIC_COMM_ASSERT(format);

    if (zxic_comm_get_print_level() == 0 || zxic_comm_get_print_level() >= ZXIC_TRACE_INVALID_PRINT)
    {
        return;
    }

    if (zxic_comm_get_print_level() >= ZXIC_TRACE_ERROR_PRINT)
    {
        ZXIC_COMM_SNPRINTF_S(devBuffer, ZXIC_SIZEOF(devBuffer), ZXIC_SIZEOF(devBuffer), "Dev_id[%u]_ERROR: ", dev_id);

        va_start(ap, format);
        {
            ZXIC_COMM_VSNPRINTF_S(szBuffer, ZXIC_SIZEOF(szBuffer), ZXIC_SIZEOF(szBuffer), format, ap);
            ZXIC_COMM_STRNCAT_S(devBuffer, ZXIC_SIZEOF(devBuffer), szBuffer, ZXIC_COMM_STRNLEN_S(szBuffer, ZXIC_SIZEOF(szBuffer)));

            if (zxic_comm_get_print_en())
            {
                DH_LOG_ERR(MODULE_NP, "%s", devBuffer);
            }
        }
        va_end(ap);
    }
}

/***********************************************************/
/** 支持多芯片的Info信息打印函数
*   打印级别为2及以上时执行打印
* @param   dev_id
* @param   format
*
* @return
* @remark  无
* @see
* @author  wcl                  @date  2018/09/08
************************************************************/
ZXIC_VOID ZXIC_COMM_TRACE_DEV_NOTICE(ZXIC_UINT32 dev_id, ZXIC_CONST ZXIC_CHAR *format, ...)
{
    va_list   ap;
    ZXIC_CHAR szBuffer[ZXIC_COMM_TRACE_BUFFER_SIZE - 32] = {0};
    ZXIC_CHAR devBuffer[ZXIC_COMM_TRACE_BUFFER_SIZE]     = {0};

    ZXIC_COMM_ASSERT(format);

    if (zxic_comm_get_print_level() == 0 || zxic_comm_get_print_level() >= ZXIC_TRACE_INVALID_PRINT)
    {
        return;
    }

    if (zxic_comm_get_print_level() >= ZXIC_TRACE_NOTICE_PRINT)
    {
        ZXIC_COMM_SNPRINTF_S(devBuffer, ZXIC_SIZEOF(devBuffer), ZXIC_SIZEOF(devBuffer), "Dev_id[%u]_NOTICE: ", dev_id);

        va_start(ap, format);
        {
            ZXIC_COMM_VSNPRINTF_S(szBuffer, ZXIC_SIZEOF(szBuffer), ZXIC_SIZEOF(szBuffer), format, ap);
            ZXIC_COMM_STRNCAT_S(devBuffer, ZXIC_SIZEOF(devBuffer), szBuffer, ZXIC_COMM_STRNLEN_S(szBuffer, ZXIC_SIZEOF(szBuffer)));

            if (zxic_comm_get_print_en())
            {
                DH_LOG_INFO(MODULE_NP, "%s", devBuffer);
            }
        }
        va_end(ap);
    }
}

/***********************************************************/
/** 支持多芯片的Info信息打印函数
*   打印级别为2及以上时执行打印
* @param   dev_id
* @param   format
*
* @return
* @remark  无
* @see
* @author  wcl                  @date  2018/09/08
************************************************************/
ZXIC_VOID ZXIC_COMM_TRACE_DEV_INFO(ZXIC_UINT32 dev_id, ZXIC_CONST ZXIC_CHAR *format, ...)
{
    va_list   ap;
    ZXIC_CHAR szBuffer[ZXIC_COMM_TRACE_BUFFER_SIZE - 32] = {0};
    ZXIC_CHAR devBuffer[ZXIC_COMM_TRACE_BUFFER_SIZE]     = {0};

    ZXIC_COMM_ASSERT(format);

    if (zxic_comm_get_print_level() == 0 || zxic_comm_get_print_level() >= ZXIC_TRACE_INVALID_PRINT)
    {
        return;
    }

    if (zxic_comm_get_print_level() >= ZXIC_TRACE_INFO_PRINT)
    {
        ZXIC_COMM_SNPRINTF_S(devBuffer, ZXIC_SIZEOF(devBuffer), ZXIC_SIZEOF(devBuffer), "Dev_id[%u]_INFO: ", dev_id);

        va_start(ap, format);
        {
            ZXIC_COMM_VSNPRINTF_S(szBuffer, ZXIC_SIZEOF(szBuffer), ZXIC_SIZEOF(szBuffer), format, ap);
            ZXIC_COMM_STRNCAT_S(devBuffer, ZXIC_SIZEOF(devBuffer), szBuffer, ZXIC_COMM_STRNLEN_S(szBuffer, ZXIC_SIZEOF(szBuffer)));

            if (zxic_comm_get_print_en())
            {
                DH_LOG_INFO(MODULE_NP, "%s", devBuffer);
            }
        }
        va_end(ap);
    }
}

/***********************************************************/
/** 支持多芯片的Debug信息打印函数
* @param   dev_id
* @param   format
*
* @return
* @remark  无
* @see
* @author  wcl                  @date  2018/09/08
************************************************************/
ZXIC_VOID ZXIC_COMM_TRACE_DEV_DEBUG(ZXIC_UINT32 dev_id,ZXIC_CONST ZXIC_CHAR *format, ...)
{
    va_list   ap;
    ZXIC_CHAR szBuffer[ZXIC_COMM_TRACE_BUFFER_SIZE - 32] = {0};
    ZXIC_CHAR devBuffer[ZXIC_COMM_TRACE_BUFFER_SIZE]     = {0};

    ZXIC_COMM_ASSERT(format);

    if (zxic_comm_get_print_level() == 0 || zxic_comm_get_print_level() >= ZXIC_TRACE_INVALID_PRINT)
    {
        return;
    }

    if (zxic_comm_get_print_level() >= ZXIC_TRACE_DEBUG_PRINT)
    {
        ZXIC_COMM_SNPRINTF_S(devBuffer, ZXIC_SIZEOF(devBuffer), ZXIC_SIZEOF(devBuffer), "Dev_id[%u]_DEBUG: ", dev_id);

        va_start(ap, format);
        {
            ZXIC_COMM_VSNPRINTF_S(szBuffer, ZXIC_SIZEOF(szBuffer), ZXIC_SIZEOF(szBuffer), format, ap);
            ZXIC_COMM_STRNCAT_S(devBuffer, ZXIC_SIZEOF(devBuffer), szBuffer, ZXIC_COMM_STRNLEN_S(szBuffer, ZXIC_SIZEOF(szBuffer)));

            if (zxic_comm_get_print_en())
            {
                DH_LOG_DEBUG(MODULE_NP, "%s", devBuffer);
            }
        }
        va_end(ap);
    }
}

/***********************************************************/
/** 支持多芯片的所有调试信息打印函数
* @param   dev_id
* @param   format
*
* @return
* @remark  无
* @see
* @author  wcl                  @date  2018/09/08
************************************************************/
ZXIC_VOID ZXIC_COMM_TRACE_DEV_ALL(ZXIC_UINT32 dev_id, ZXIC_CONST ZXIC_CHAR *format, ...)
{
    va_list   ap;
    ZXIC_CHAR szBuffer[ZXIC_COMM_TRACE_BUFFER_SIZE - 32] = {0};
    ZXIC_CHAR devBuffer[ZXIC_COMM_TRACE_BUFFER_SIZE]     = {0};

    ZXIC_COMM_ASSERT(format);

    if (zxic_comm_get_print_level() == 0 || zxic_comm_get_print_level() >= ZXIC_TRACE_INVALID_PRINT)
    {
        return;
    }

    if (zxic_comm_get_print_level() >= ZXIC_TRACE_ALL_PRINT)
    {
        ZXIC_COMM_SNPRINTF_S(devBuffer, ZXIC_SIZEOF(devBuffer), ZXIC_SIZEOF(devBuffer), "Dev_id[%u]_ALL:", dev_id);

        va_start(ap, format);
        {
            ZXIC_COMM_VSNPRINTF_S(szBuffer, ZXIC_SIZEOF(szBuffer), ZXIC_SIZEOF(szBuffer), format, ap);
            ZXIC_COMM_STRNCAT_S(devBuffer, ZXIC_SIZEOF(devBuffer), szBuffer, ZXIC_COMM_STRNLEN_S(szBuffer, ZXIC_SIZEOF(szBuffer)));

            if (zxic_comm_get_print_en())
            {
                DH_LOG_DEBUG(MODULE_NP, "%s", devBuffer);
            }
        }
        va_end(ap);
    }
}
#endif
#endif

#if ZXIC_REAL("")
#if 0
ZXIC_VOID ZXIC_COMM_DBGCNT64_PRINT(ZXIC_CONST ZXIC_CHAR * name, ZXIC_UINT64 value)
{
    ZXIC_CHAR temp_buff[50] = {0};

    if (-1 == ZXIC_COMM_SNPRINTF_S(temp_buff, 50, 50, "0x%016llx", value))
    {
        return;
    }

    ZXIC_COMM_PRINT("%-50s  : %18s\n", name, temp_buff);
}

ZXIC_VOID ZXIC_COMM_DBGCNT32_PRINT(ZXIC_CONST ZXIC_CHAR * name, ZXIC_UINT32 value)
{
    ZXIC_CHAR temp_buff[50] = {0};

    if (-1 == ZXIC_COMM_SNPRINTF_S(temp_buff, 50, 50, "0x%08x", value))
    {
        return;
    }

    ZXIC_COMM_PRINT("%-50s  : %18s\n", name, temp_buff);
}

/** 双参数打印 */
ZXIC_VOID ZXIC_COMM_DBGCNT32_PAR_PRINT(ZXIC_CONST ZXIC_CHAR * name, ZXIC_UINT32 parm, ZXIC_UINT32 value)
{
    ZXIC_CHAR temp_buff[50] = {0};
    ZXIC_CHAR vlaue_buff[18] = {0};

    if (-1 == ZXIC_COMM_SNPRINTF_S(temp_buff, 50, 50, name, parm))
    {
        return;
    }

    if (-1 == ZXIC_COMM_SNPRINTF_S(vlaue_buff, 18, 18, "0x%08x", value))
    {
        return;
    }

    ZXIC_COMM_PRINT("%-50s  : %18s\n", temp_buff, vlaue_buff);
}
#endif
#endif
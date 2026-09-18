/**************************************************************
* 版权所有 (C)2013-2020, 深圳市中兴通讯股份有限公司
* 文件名称 : zxic_common.c
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
#include "zxic_private_top.h"
#include "zxic_common.h"
#include "dpp_type_api.h"
#include <linux/ctype.h>
//#include <stdlib.h>
//#include <stdio.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/errno.h>
//#include <assert.h>
//#include <linux/stdarg.h>
//#include <linux/math.h>
#include <linux/stat.h>

#ifdef ZXIC_OS_WIN
#include <Windows.h>
#include <time.h>
#include <direct.h>
#include <io.h>
#pragma warning (disable:4996)
#else
//#include <unistd.h>
#include <linux/types.h>
#include <linux/socket.h>
#include <linux/selection.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/fcntl.h>
#endif 

#if ZXIC_REAL("参数检查函数定义")
/***********************************************************/
/**
* @param   val
* @param   min
* @param   max
*
* @return
* @remark  无
* @see
* @author  XXX      @date  2019/07/13
************************************************************/
ZXIC_RTN32 zxic_comm_index_check(ZXIC_UINT32 val, ZXIC_UINT32 min, ZXIC_UINT32 max)
{
    if (min <= max)
    {
        if (0 == min)
        {
            if ((val) > (max))
            {
                return ZXIC_PAR_CHK_INVALID_INDEX;
            }
        }
        else
        {
            if ((val) < (min) || (val) > (max))
            {
                return ZXIC_PAR_CHK_INVALID_INDEX;
            }
        }
    }
    else
    {
        return ZXIC_PAR_CHK_INVALID_RANGE;
    }

    return ZXIC_OK;
}

/***********************************************************/
/**
* @param   dev_id
* @param   val
* @param   min
* @param   max
*
* @return
* @remark  无
* @see
* @author  PJ      @date  2019/07/13
************************************************************/
ZXIC_RTN32 zxic_comm_dev_index_check(ZXIC_UINT32 dev_id, 
                                     ZXIC_UINT32 val, 
                                     ZXIC_UINT32 min, 
                                     ZXIC_UINT32 max)
{
    if (min <= max)
    {
        if (0 == min)
        {
            if ((val) > (max))
            {
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ZXIC %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);
                return ZXIC_PAR_CHK_INVALID_INDEX;
            }
        }
        else
        {
            if ((val) < (min) || (val) > (max))
            {
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ZXIC %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);
                return ZXIC_PAR_CHK_INVALID_INDEX;
            }
        }
    }
    else
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "ZXIC %s:%d[Error:RANGE INVALID] [val=0x%x,min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);
        return ZXIC_PAR_CHK_INVALID_RANGE;
    }

    return ZXIC_OK;
}

/***********************************************************/
/** 校验码
* @param
*
* @return
* @remark  无
* @see
* @author  cq     @date  2025/05/23
************************************************************/
ZXIC_RTN32 zxic_comm_errcode_check(ZXIC_UINT32 error_code)
{
    if((error_code==ZXIC_PAR_CHK_BAR_ABNORMAL) || (error_code==ZXIC_PAR_CHK_DEV_STATUS_OFF) ||
       (error_code==DPP_RC_DTB_STAT_QUEUE_NOT_ENABLE) || (error_code==DPP_RC_DTB_STAT_OVER_TIME) ||
       (error_code==DPP_RC_DTB_STAT_QUEUE_ITEM_SW_EMPTY))
    {
        return ZXIC_OK;
    }

    return ZXIC_PAR_CHK_INVALID_INDEX;

}

#endif
















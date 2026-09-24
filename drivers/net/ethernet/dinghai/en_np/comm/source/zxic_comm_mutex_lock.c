/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : zxic_sal.c
* 文件标识 : 
* 内容摘要 : 
* 其它说明 : 
* 当前版本 : 
* 完成日期 : 2014/02/07
* DEPARTMENT: ASIC_FPGA_R&D_Dept 
* MANUAL_PERCENT: 100%   
 
* 修改记录1: 
* 修改日期:  
* 版 本 号:  
* 修 改 人:  
* 修改内容:  
***************************************************************/

#include "zxic_common.h"
#include "zxic_private.h"

#ifdef ZXIC_OS_WIN
#define ZXIC_MUTEX_WAITTIME_MAX       (INFINITE)  /* 互斥锁最大等待时间 */
#else
#define ZXIC_MUTEX_WAITTIME_MAX       (5000)  /* 互斥锁最大等待时间:5000ms */
#endif

/***********************************************************/
/** 初始化互斥量
* @param   p_mutex 互斥量   
*
* @return  
* @remark  无
* @see     
************************************************************/
ZXIC_RTN32 zxic_comm_mutex_create(ZXIC_MUTEX_T *p_mutex)
{
    // ZXIC_SINT32 rc = 0;
    
    ZXIC_COMM_CHECK_POINT(p_mutex);
    
#ifdef ZXIC_OS_WIN
    p_mutex->mutex = CreateMutex(ZXIC_NULL, ZXIC_FALSE, ZXIC_NULL);
    if(p_mutex->mutex == 0)
    {
        ZXIC_COMM_TRACE_ERROR("\nErrCode[ 0x%x ]: Create mutex failed.", ZXIC_MUTEX_LOCK_INIT_FAIL);
        return ZXIC_MUTEX_LOCK_INIT_FAIL;
    }
#else
    /*rc = pthread_mutex_init(&p_mutex->mutex, NULL);
    if(rc != 0)
    {
        ZXIC_COMM_TRACE_ERROR("\nErrCode[ 0x%x ]: Create mutex failed", ZXIC_MUTEX_LOCK_INIT_FAIL);
        return ZXIC_MUTEX_LOCK_INIT_FAIL;
    }*/
    mutex_init(&p_mutex->mutex);


#endif

    return ZXIC_OK;
}

/***********************************************************/
/** 互斥量加锁
* @param   p_mutex   
*
* @return  
* @remark  无
* @see     
* @date  2014/02/07
************************************************************/
ZXIC_RTN32 zxic_comm_mutex_lock(ZXIC_MUTEX_T *p_mutex)
{
    ZXIC_SINT32 rc = 0; 
#ifndef ZXIC_FOR_FUZZER    
    ZXIC_COMM_CHECK_POINT(p_mutex);
    
    #ifdef ZXIC_OS_WIN
        switch(WaitForSingleObject(p_mutex->mutex, ZXIC_MUTEX_WAITTIME_MAX))
        {
            case (WAIT_OBJECT_0):
            {
                /* wait mutex success. */
                break;
            }
            default:
            {
                ZXIC_COMM_TRACE_ERROR("\nErrCode[ 0x%x ]: WaitForSingleObject failed.", ZXIC_MUTEX_LOCK_LOCK_FAIL);
                ZXIC_COMM_ASSERT(0);
                return ZXIC_MUTEX_LOCK_LOCK_FAIL;
            }
        }
    #else
        /*rc = pthread_mutex_lock(&p_mutex->mutex);
        if(rc != 0)
        {
            ZXIC_COMM_TRACE_ERROR("\nErrCode[ 0x%x ]: Get mutex lock fail.", ZXIC_MUTEX_LOCK_LOCK_FAIL);
            //ZXIC_COMM_ASSERT(0);
            //return ZXIC_MUTEX_LOCK_LOCK_FAIL;
            return rc;
        }*/
        mutex_lock(&p_mutex->mutex);
    #endif
#endif

    return rc;
}

/***********************************************************/
/** 互斥量解锁
* @param   p_mutex   
*
* @return  
* @remark  无
* @see     
************************************************************/
ZXIC_RTN32 zxic_comm_mutex_unlock(ZXIC_MUTEX_T *p_mutex)
{
    ZXIC_SINT32 rc = 0; 
#ifndef ZXIC_FOR_FUZZER 

    ZXIC_COMM_CHECK_POINT(p_mutex);
        
    #ifdef ZXIC_OS_WIN
        if(!ReleaseMutex(p_mutex->mutex))
        {
            ZXIC_COMM_TRACE_ERROR("\nErrCode[ 0x%x ]: ReleaseMutex failed.", ZXIC_MUTEX_LOCK_ULOCK_FAIL);
            return ZXIC_MUTEX_LOCK_ULOCK_FAIL;
        }
    #else
        /*rc = pthread_mutex_unlock(&p_mutex->mutex);
        if(rc != 0)
        {
            ZXIC_COMM_TRACE_ERROR("\nErrCode[ 0x%x ]: Release mutex lock fail.", ZXIC_MUTEX_LOCK_ULOCK_FAIL);
            return ZXIC_MUTEX_LOCK_ULOCK_FAIL;
        }*/
        mutex_unlock(&p_mutex->mutex);
    #endif
#endif

    return rc;
}

/***********************************************************/
/** 销毁互斥量
* @param   p_mutex   
*
* @return  
* @remark  无
* @see     
************************************************************/
ZXIC_RTN32 zxic_comm_mutex_destroy(ZXIC_MUTEX_T *p_mutex)
{
    // ZXIC_SINT32 rc = 0;
    
    ZXIC_COMM_CHECK_POINT(p_mutex);
    
#ifdef ZXIC_OS_WIN
    if(p_mutex->mutex == 0)
    {
        ZXIC_COMM_TRACE_ERROR("\nErrCode[ 0x%x ]: Destroy mutex failed.", ZXIC_MUTEX_LOCK_DESTROY_FAIL);
        return ZXIC_MUTEX_LOCK_DESTROY_FAIL;
    }
    CloseHandle(p_mutex->mutex);
#else
    /*rc = pthread_mutex_destroy(&p_mutex->mutex);
    if(rc != 0)
    {
        ZXIC_COMM_TRACE_ERROR("\nErrCode[ 0x%x ]: Destroy mutex fail", ZXIC_MUTEX_LOCK_DESTROY_FAIL);
        return ZXIC_MUTEX_LOCK_DESTROY_FAIL;
    }*/
    mutex_destroy(&p_mutex->mutex);
#endif

    return ZXIC_OK;
}
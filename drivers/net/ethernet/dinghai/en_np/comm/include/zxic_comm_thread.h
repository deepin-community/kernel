/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : zxic_comm_thread.h
* 文件标识 : 
* 内容摘要 : 
* 其它说明 : 
* 当前版本 : 
* 完成日期 : 2014/02/08
* DEPARTMENT: ASIC_FPGA_R&D_Dept 
* MANUAL_PERCENT: 100%   
 
* 修改记录1: 
* 修改日期:  
* 版 本 号:  
* 修 改 人:  
* 修改内容:  
***************************************************************/

#ifndef _ZXIC_COMM_THREAD_H_
#define _ZXIC_COMM_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ZXIC_OS_LINUX
#include <linux/sched.h>
#include <linux/limits.h>
#endif

#define THREAD_NAME_MAX                 (64)
#define ZXIC_THREAD_TIME_INFINITE       (0xFFFFFFFF)  /* Infinite timeout */

typedef ZXIC_VOID* (*ZXIC_THREAD_FUNC) (ZXIC_VOID*);

/* Thread ID */
typedef struct zxic_comm_thread_id_t
{
#ifdef ZXIC_OS_WIN
    HANDLE  id;
#else
    //pthread_t id;
    int id;
#endif
}ZXIC_THREAD_ID_T;

/* Thread CreateFlag */
#define ZXIC_THREAD_FLAG_DETACH             (1 << 0)
#define ZXIC_THREAD_FLAG_EXPLICIT_SCHED     (1 << 1)

typedef struct zxic_comm_thread_info_t
{
    ZXIC_CHAR         name[THREAD_NAME_MAX];  /* 线程名   */
    ZXIC_UINT32       priority;               /* 优先级   */
    ZXIC_UINT32       stack_size;             /* 初始栈大小，以字节为单位 */
    ZXIC_UINT32       create_flag;            /* 线程标志 */
    ZXIC_THREAD_ID_T  id;                     /* 线程ID   */
    ZXIC_THREAD_FUNC  thread_func;            /* 线程函数 */
    ZXIC_VOID*        p_arg;                  /* 线程入参 */
    ZXIC_UINT32       is_valid;               /* 是否有效 */
}ZXIC_THREAD_INFO_T;

/* API */
ZXIC_RTN32 zxic_comm_thread_info_init(ZXIC_VOID);
ZXIC_RTN32 zxic_comm_thread_info_add(ZXIC_THREAD_ID_T*     p_thread_id,
                                     ZXIC_CONST ZXIC_CHAR* p_name,
                                     ZXIC_UINT32           priority,
                                     ZXIC_UINT32           stack_size,
                                     ZXIC_UINT32           create_flag,
                                     ZXIC_THREAD_FUNC      p_thread_func,
                                     ZXIC_VOID*            p_arg,
                                     ZXIC_UINT32*          p_info_index);
ZXIC_RTN32 zxic_comm_thread_info_del(ZXIC_THREAD_ID_T *p_thread_id);
ZXIC_RTN32 zxic_comm_thread_info_print(ZXIC_VOID);

ZXIC_RTN32 zxic_comm_thread_create(ZXIC_CONST ZXIC_CHAR *p_name, 
                              ZXIC_UINT32            priority,
                              ZXIC_UINT32            stack_size,
                              ZXIC_UINT32            create_flag,
                              ZXIC_THREAD_FUNC       thread_func,
                              ZXIC_VOID              *p_arg,
                              ZXIC_THREAD_ID_T       *p_thread_id);

ZXIC_RTN32 zxic_comm_thread_exit(ZXIC_VOID);
ZXIC_RTN32 zxic_comm_thread_wait(ZXIC_THREAD_ID_T  *p_thread_id, ZXIC_DWORD wait_time);
ZXIC_RTN32 zxic_comm_thread_close_handle(ZXIC_THREAD_ID_T *p_thread_id);

#ifdef __cplusplus
}
#endif


#endif


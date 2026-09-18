/**************************************************************
* 版权所有 (C)2013-2020, 深圳市中兴通讯股份有限公司
* 文件名称      :
* 文件标识      :
* 内容摘要      :   集合几个文件用到的定义，不对外部开放
* 其它说明      :
* 当前版本      :
* 作    者      :
* 完成日期      : 2020/07/20
* DEPARTMENT: 有线开发四部-系统软件团队
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/
#ifndef __ZXIC_PRIVATE_H__
#define __ZXIC_PRIVATE_H__

#if ZXIC_REAL("日志相关")
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/fs.h>
#include <linux/spinlock.h>

#define ZXIC_TRACE_LOG_FILE_GZ_MAX_CNT  (50) /* 最大压缩后日志文件数 */

typedef struct zxic_log_file_info
{
    char       fname[50];
    struct file       *p_log_fp;
    unsigned int     f_size;    /* 日志文件大小上限，以字节为单位 */
}ZXIC_LOG_FILE_INFO;


#endif /* ZXIC_REAL("日志相关") */

#if ZXIC_REAL("打印相关")
 typedef enum
 {
    ZXIC_TRACE_PRINT       = 0,
    ZXIC_TRACE_ERROR_PRINT = 1,
    ZXIC_TRACE_NOTICE_PRINT   ,
    ZXIC_TRACE_INFO_PRINT     ,
    ZXIC_TRACE_DEBUG_PRINT    ,
    ZXIC_TRACE_ALL_PRINT      ,
    ZXIC_TRACE_INVALID_PRINT
 } ZXIC_TRACE_LEVEL;

 typedef enum zxic_log_file_type_e
 {
    ZXIC_LOG_SDK       = 0,  /*SDK日常配置记录*/
    ZXIC_LOG_INIT      = 1,  /*初始化日志记录*/
    ZXIC_LOG_LIF       = 2,  /*LIF日志记录*/
    ZXIC_LOG_SERDES    = 3,  /*SERDES日志记录*/
    ZXIC_LOG_SE_ERAM   = 4,  /*SE ERAM日志记录*/
    ZXIC_LOG_SE_HBM    = 5,  /*SE HBM日志记录*/ 
    ZXIC_LOG_SE_OTHER  = 6,  /*SE other （se 部分模块）日志记录*/
    ZXIC_LOG_REG       = 7,  /*寄存器和打桩信息日志记录，特殊使用*/
    ZXIC_LOG_DEBUG     = 8,  /*打印，诊断计数打印等日志记录*/
    ZXIC_LOG_DUMP      = 9,  /*捞数据日志记录，特殊使用*/
    ZXIC_LOG_SE_LPM_SAMPLE_V4    = 10,  /*SE LPM_V4样本打印记录*/
    ZXIC_LOG_SE_LPM_SAMPLE_V6    = 11,  /*SE LPM_V4样本打印记录*/
    ZXIC_LOG_UT_DETAIL     = 12,  /* ut check 失败信息 */
    ZXIC_LOG_UT_RESULT     = 13,  /* utcheck 统计信息 */
    ZXIC_LOG_SE_HASH   = 14,  /*SE HASH日志记录*/
    ZXIC_LOG_SE_ACL    = 15,  /*SE ACL日志记录*/
    ZXIC_LOG_SLT       = 16,  /*SLT 日志记录*/
    ZXIC_LOG_SDS_COMM  = 17,  /*SERDES COMM 库日志记录 */
    ZXIC_LOG_THREAD    = 31,  /*多线程log打印,注意多线程占用了31b~26b*/
    ZXIC_LOG_MAX,
 } ZXIC_LOG_FILE_TYPE_E;

#define ZXIC_THREAD_ID_NUM_MAX      (2048)

#define ZXIC_MALLOC_MAX_B_SIZE (0xC800000U) /* 200M */

#endif /* ZXIC_REAL("打印相关") */

#if ZXIC_REAL("互斥锁相关")
typedef struct zxic_mutex_t
{
#ifdef ZXIC_OS_WIN
    HANDLE          mutex;
#else
    struct mutex mutex;


#endif
}ZXIC_MUTEX_T;
#endif

#if ZXIC_REAL("自旋锁相关")
typedef struct zxic_spin_lock_t
{
    spinlock_t spin_lock;
}ZXIC_SPIN_LOCK_T;
#endif

#if ZXIC_REAL("信号量相关")
typedef struct zxic_sem_t
{
    #ifdef ZXIC_OS_WIN
    HANDLE          sem;
    #else
    struct semaphore sem;
    #endif
}ZXIC_SEM_T;
#endif

void *ic_comm_sdk_print_regist(void);
unsigned int ic_comm_callback_print_get(void* pExcCall);
unsigned int ic_comm_callback_err_log_get(void* pExcCall);
void ic_comm_set_os_callback(ZXIC_OS_CALLBACK *p_os_cb);
void ic_comm_malloc_record(unsigned int size);
void ic_comm_free_record(void);
/***********************************************************/
/** 从path_name所指的目录中查找符合参数的执行文件，找到后便执行该文件，
    然后将第二个参数argv传给该欲执行的文件,请参照execv用法,异常时候返回-1
* @return
* @remark  无
* @see
* @author  pj      @date  2020/03/30
************************************************************/
int zxic_system(const char *path_name, char *const argv[]);

#endif /* end  __ZXIC_COMMON_TOP_H__ */

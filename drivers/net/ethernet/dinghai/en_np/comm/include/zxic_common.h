/**************************************************************
* 版权所有 (C)2013-2020, 深圳市中兴通讯股份有限公司
* 文件名称      :       zxic_common.h
* 文件标识      :
* 内容摘要      :       大部分的项目,只需要感知这一个头文件即可,
                    其包括了变量定义/打印/日志/参数校验/互斥锁/bit流拼接等常用定义和功能
* 其它说明      :       编译宏：ZXIC_OS_WIN/ZXIC_RELEASE/MACRO_CPU64
* 当前版本      :
* 作    者    :
* 完成日期      :       2020/07/20
* DEPARTMENT:       有线开发四部-系统软件团队
* MANUAL_PERCENT:   100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/
#ifndef __ZXIC_COMMON_H__
#define __ZXIC_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
编译宏说明
ZXIC_OS_WIN :若不导入该宏,则默认为 ZXIC_OS_LINUX
ZXIC_RELEASE:若不导入该宏,则默认为 ZXIC_DEBUG
MACRO_CPU64 :若不导入该宏,则默认为32位操作系统
*/

#include "zxic_private_top.h"
#include "zxic_private.h"
//#include <ctype.h>
//#include <stdlib.h>
//#include <stdio.h>
#include <linux/stddef.h>
#include <linux/string.h>
//#include <errno.h>
//#include <assert.h>
#if defined(ZXDH_ADAPT_REDHAT_9_2) || defined(ZXDH_ADAPT_ANOLIS_6_6) || defined(ZXDH_ADAPT_REDHAT_9_1) || defined(HAVE_USE_STDARG_HEADER)\
    || defined(KYLIN_V11_6_6)
#include <linux/stdarg.h>
#else
#include <stdarg.h>
#endif
//#include <math.h>
#include <linux/stat.h>
//#include <stdint.h>
#include "log.h"

#ifdef ZXIC_OS_WIN              /* 编译宏导入 */
#include <Windows.h>
#include <time.h>
#include <direct.h>
#include <io.h>
#pragma warning (disable:4996)
#else
#include <linux/unistd.h>
#include <linux/types.h>
#include <linux/socket.h>
//#include <select.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/fcntl.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/dma-mapping.h>
#endif

#if ZXIC_REAL("数据类型定义")
/* 无符号整数 */
#define ZXIC_UINT8              unsigned char
#define ZXIC_UINT16             unsigned short

#ifndef ZXIC_UINT32
#define ZXIC_UINT32             unsigned int
#endif

#ifndef ZXIC_UINT64
#define ZXIC_UINT64             unsigned long long
#endif
#define ZXIC_DWORD              unsigned long      /* 慎用,注意:WIN系统:32bits       LINUX系统:64bits */
#define ZXIC_SIZE_T             size_t             /* 32位系统:uint32 64位系统:uint64 */

/* 有符号整数 */
#define ZXIC_CHAR               char               /* char默认为unsigned还是signed取决于编译器,定义字符可用 */
#define ZXIC_SINT8              signed char
#define ZXIC_SINT16             signed short

#ifndef ZXIC_SINT32
#define ZXIC_SINT32             signed int
#endif

#ifndef ZXIC_SINT64
#define ZXIC_SINT64             signed long long  
#endif

#define ZXIC_SDWORD             long

/* 浮点数 */
//#define ZXIC_FLOAT              float
//#define ZXIC_DOUBLE             double
//#define ZXIC_LDOUBLE            long double
#define ZXIC_FLOAT              int
#define ZXIC_DOUBLE             long
#define ZXIC_LDOUBLE            long long

/* 文件类型 */
typedef struct file             ZXIC_FILE;

/*  */
#define ZXIC_VOL                volatile
#define ZXIC_VOID               void
#define ZXIC_CONST              const
#define ZXIC_RTN32              ZXIC_UINT32

#ifdef MACRO_CPU64              /* 64位系统编译宏导入,慎重修改 */
#define ZXIC_ADDR_T             ZXIC_UINT64
#define ZXIC_SIZEOF(x)          (sizeof(x) & 0xFFFFFFFFU)  /* 在64位环境下，sizeof的长度是64b */
#define ZXIC_SIZEOF_T(x)        ((ZXIC_UINT32)(sizeof(x) & 0xFFFFFFFF))

#else
#define ZXIC_ADDR_T             ZXIC_UINT32
#define ZXIC_SIZEOF(x)          (sizeof(x))
#define ZXIC_SIZEOF_T(x)        (sizeof(x))
#endif

/* 特殊变量 */
#define ZXIC_NULL               (0)
#define ZXIC_OK                 (0U)               /* 后面加U,防止32位系统64位系统常量的默认长度不一致 */
#define ZXIC_ERR                (1U)
#define ZXIC_TRUE               (1U)
#define ZXIC_FALSE              (0U)
#define ZXIC_UINT8_MAX          (0xFFU)
#define ZXIC_UINT32_MAX         (0xFFFFFFFFU)
#define ZXIC_ULONG_MAX          (0xFFFFFFFFFFFFFFFFUL)
#define ZXIC_SINT_MAX           (0x7FFFFFFF)
#define ZXIC_SINT_MIN           (-ZXIC_SINT_MAX - 1)


#define ZXIC_UINT64_MASK        (0xFFFFFFFFFFFFFFFFULL)
#define ZXIC_UINT32_MASK        (0xFFFFFFFFU)
#define ZXIC_UINT16_MASK        (0xFFFFU)
#define ZXIC_UINT8_MASK         (0xFFU)
#endif  /* END 数据类型定义 */

#if ZXIC_REAL("宏函数定义")
#define ZXIC_COMM_MEMCMP             ic_comm_memcmp
#define ZXIC_COMM_MEMSET             memset
#define ZXIC_COMM_MEMMOVE            memmove
#define ZXIC_COMM_MEMSET_S           ic_comm_memset_s
#define ZXIC_COMM_MEMCPY             ic_comm_memcpy
#define ZXIC_COMM_MEMCPY_S           ic_comm_memcpy_s
#define ZXIC_COMM_STRLEN             strlen
#define ZXIC_COMM_STRNLEN            strnlen
#define ZXIC_COMM_STRNLEN_S          ic_comm_strnlen_s
#define ZXIC_COMM_STRCPY             strcpy
#define ZXIC_COMM_STRCPY_S           ic_comm_strcpy_s
#define ZXIC_COMM_STRNCPY            strncpy
#define ZXIC_COMM_STRNCPY_S          ic_comm_strncpy_s
#define ZXIC_COMM_STRCMP             strcmp
#define ZXIC_COMM_STRNCMP            ic_comm_strncmp
#define ZXIC_COMM_STRTOK             strtok
#define ZXIC_COMM_STRTOK_S           ic_comm_strtok_s
#define ZXIC_COMM_STRCAT_S           ic_comm_strcat_s
#define ZXIC_COMM_STRNCAT_S          ic_comm_strncat_s

//#define ZXIC_COMM_FOPEN              fopen
#define ZXIC_COMM_FOPEN              filp_open
//#define ZXIC_COMM_FCLOSE             fclose
#define ZXIC_COMM_FCLOSE(a)          filp_close(a,NULL)
#define ZXIC_COMM_FGETS              fgets
#define ZXIC_COMM_FPUTS              fputs
#define ZXIC_COMM_FREAD              fread

// #define ZXIC_COMM_FPRINTF            fprintf
// #define ZXIC_COMM_FPRINTF(a,b,c)     printk(b,c)      
#define ZXIC_COMM_SSCANF             ic_comm_sscanf
#define ZXIC_COMM_FSCANF             (void)fscanf
#define ZXIC_COMM_SNPRINTF_S         ic_comm_snprintf_s
#define ZXIC_COMM_VSNPRINTF_S        ic_comm_vsnprintf_s

#define ZXIC_COMM_TIME               time
#define ZXIC_COMM_ATOI               atoi

#ifdef ZXIC_OS_WIN
#define ZXIC_COMM_ACCESS             _access
#define ZXIC_COMM_SNPRINTF           _snprintf
#define ZXIC_COMM_VSNPRINTF          _vsnprintf
#define ZXIC_COMM_GETPID             _getpid
#else
#define ZXIC_COMM_ACCESS              kern_path
/*#define ZXIC_COMM_SNPRINTF           snprintf*/
#define ZXIC_COMM_SNPRINTF(a,b,c...)    __snprintf_chk(a,b,0,b,c)
#define ZXIC_COMM_VSNPRINTF          vsnprintf
#define ZXIC_COMM_GETPID             getpid
#endif

#ifdef MACRO_CPU64                   /* 64位系统编译宏导入,慎重修改 */
#define ZXIC_COMM_PTR_TO_VAL(p)      ((ZXIC_UINT64)(p))
#define ZXIC_COMM_VAL_TO_PTR(v)      ((ZXIC_VOID *)((ZXIC_UINT64)(v)))
#define ZXIC_SSIZE_T   ZXIC_SINT64
#else
#define ZXIC_COMM_PTR_TO_VAL(p)      ((ZXIC_UINT32)(p))
#define ZXIC_COMM_VAL_TO_PTR(v)      ((ZXIC_VOID *)(long)((ZXIC_UINT32)(v)))
#define ZXIC_SSIZE_T   ZXIC_SINT32

#endif

#ifdef ZXIC_OS_WIN
#define ZXIC_COMM_STRCASECMP        stricmp
#else
#define ZXIC_COMM_STRCASECMP        strcasecmp
#endif

#define ZXIC_COMM_FFLUSH            (ZXIC_VOID)fflush
#define ZXIC_COMM_SPRINTF           (ZXIC_VOID)sprintf



#ifdef ZXIC_RELEASE                   /* 编译宏控制,默认编译为DEBUG版本 */
#define ZXIC_COMM_ASSERT(x)
#else
    #ifdef ZXIC_FOR_FUZZER
        #define ZXIC_COMM_ASSERT(x)
    #else
        #define ZXIC_COMM_ASSERT(x)
    #endif
#endif

#endif  /* END 宏函数定义 */

#define ZXIC_COMM_MEMORY_MAX_B_SIZE (200 * 1024 * 1024) /* 200M */
#define ZXIC_COMM_STRNLEN_MAX           (0xffffffff)

#if ZXIC_REAL("打印-print")
#if ZXIC_REAL("开关")
ZXIC_VOID  zxic_comm_set_print_en   (ZXIC_UINT32 enable);
ZXIC_RTN32 zxic_comm_get_print_en   (ZXIC_VOID);
/* Started by AICoder, pid:ldf5ec7fe856d61143b90995c03df908ffa58f2c */
ZXIC_VOID  zxic_comm_set_print_level(ZXIC_UINT32 debug_level);
/* Ended by AICoder, pid:ldf5ec7fe856d61143b90995c03df908ffa58f2c */
ZXIC_RTN32 zxic_comm_get_print_level(ZXIC_VOID);
#endif
#if ZXIC_REAL("功能")
#if 0
ZXIC_VOID ZXIC_COMM_PRINT(ZXIC_CONST ZXIC_CHAR *format, ...);
ZXIC_VOID ZXIC_COMM_TRACE_ERROR(ZXIC_CONST ZXIC_CHAR *format, ...);
ZXIC_VOID ZXIC_COMM_TRACE_NOTICE(ZXIC_CONST ZXIC_CHAR *format, ...);
ZXIC_VOID ZXIC_COMM_TRACE_INFO(ZXIC_CONST ZXIC_CHAR *format, ...);
ZXIC_VOID ZXIC_COMM_TRACE_DEBUG(ZXIC_CONST ZXIC_CHAR *format, ...);
ZXIC_VOID ZXIC_COMM_TRACE_ALL(ZXIC_CONST ZXIC_CHAR *format, ...);

ZXIC_VOID ZXIC_COMM_TRACE_DEV_ERROR(ZXIC_UINT32 dev_id, ZXIC_CONST ZXIC_CHAR *format, ...);
ZXIC_VOID ZXIC_COMM_TRACE_DEV_NOTICE(ZXIC_UINT32 dev_id, ZXIC_CONST ZXIC_CHAR *format, ...);
ZXIC_VOID ZXIC_COMM_TRACE_DEV_INFO(ZXIC_UINT32 dev_id, ZXIC_CONST ZXIC_CHAR *format, ...);
ZXIC_VOID ZXIC_COMM_TRACE_DEV_DEBUG(ZXIC_UINT32 dev_id,ZXIC_CONST ZXIC_CHAR *format, ...);
ZXIC_VOID ZXIC_COMM_TRACE_DEV_ALL(ZXIC_UINT32 dev_id, ZXIC_CONST ZXIC_CHAR *format, ...);

ZXIC_VOID ZXIC_COMM_DBGCNT64_PRINT       (ZXIC_CONST ZXIC_CHAR * name, ZXIC_UINT64 value);
ZXIC_VOID ZXIC_COMM_DBGCNT32_PRINT       (ZXIC_CONST ZXIC_CHAR * name, ZXIC_UINT32 value);
ZXIC_VOID ZXIC_COMM_DBGCNT32_PAR_PRINT   (ZXIC_CONST ZXIC_CHAR * name, ZXIC_UINT32 parm, ZXIC_UINT32 value);
#endif 
#define ZXIC_COMM_PRINT(fmt, arg...) \
    do{\
        if (zxic_comm_get_print_en())\
        {\
            DH_LOG_INFO(MODULE_NP, fmt, ##arg);\
        }\
    }while(0) 

#define ZXIC_COMM_TRACE_ERROR(fmt, arg...) \
    do{\
        if (zxic_comm_get_print_en())\
        {\
            DH_LOG_ERR(MODULE_NP, fmt, ##arg);\
        }\
    }while(0) 

#define ZXIC_COMM_TRACE_NOTICE(fmt, arg...) \
    do{\
        if (zxic_comm_get_print_level() >= ZXIC_TRACE_NOTICE_PRINT)\
        {\
            if (zxic_comm_get_print_en())\
            {\
                DH_LOG_INFO(MODULE_NP, "[%s] "fmt, __func__, ##arg);\
            }\
        }\
    }while(0) 

#define ZXIC_COMM_TRACE_INFO(fmt, arg...) \
    do{\
        if (zxic_comm_get_print_level() >= ZXIC_TRACE_INFO_PRINT)\
        {\
            if (zxic_comm_get_print_en())\
            {\
                DH_LOG_INFO(MODULE_NP, "[%s] "fmt, __func__, ##arg);\
            }\
        }\
    }while(0) 

#define ZXIC_COMM_TRACE_DEBUG(fmt, arg...) \
    do{\
        if (zxic_comm_get_print_level() >= ZXIC_TRACE_DEBUG_PRINT)\
        {\
            if (zxic_comm_get_print_en())\
            {\
                DH_LOG_DEBUG(MODULE_NP, fmt, ##arg);\
            }\
        }\
    }while(0) 

#define ZXIC_COMM_TRACE_ALL(fmt, arg...) \
    do{\
        if (zxic_comm_get_print_level() >= ZXIC_TRACE_ALL_PRINT)\
        {\
            if (zxic_comm_get_print_en())\
            {\
                DH_LOG_DEBUG(MODULE_NP, fmt, ##arg);\
            }\
        }\
    }while(0) 

#define ZXIC_COMM_TRACE_DEV_ERROR(dev_id, fmt, arg...) \
    do{\
        if (zxic_comm_get_print_level() >= ZXIC_TRACE_ERROR_PRINT)\
        {\
            if (zxic_comm_get_print_en())\
            {\
                DH_LOG_ERR(MODULE_NP, "Dev_id[%u]_ERROR: " fmt, dev_id, ##arg);\
            }\
        }\
    }while(0) 

#define ZXIC_COMM_TRACE_DEV_NOTICE(dev_id, fmt, arg...) \
    do{\
        if (zxic_comm_get_print_level() >= ZXIC_TRACE_NOTICE_PRINT)\
        {\
            if (zxic_comm_get_print_en())\
            {\
                DH_LOG_INFO(MODULE_NP, "[%s]Dev_id[%u]_NOTICE: " fmt, __func__, dev_id, ##arg);\
            }\
        }\
    }while(0) 

#define ZXIC_COMM_TRACE_DEV_INFO(dev_id, fmt, arg...) \
    do{\
        if (zxic_comm_get_print_level() >= ZXIC_TRACE_INFO_PRINT)\
        {\
            if (zxic_comm_get_print_en())\
            {\
                DH_LOG_INFO(MODULE_NP, "[%s]Dev_id[%u]_INFO: " fmt, __func__, dev_id, ##arg);\
            }\
        }\
    }while(0) 

#define ZXIC_COMM_TRACE_DEV_DEBUG(dev_id, fmt, arg...) \
    do{\
        if (zxic_comm_get_print_level() >= ZXIC_TRACE_DEBUG_PRINT)\
        {\
            if (zxic_comm_get_print_en())\
            {\
                DH_LOG_DEBUG(MODULE_NP, "Dev_id[%u]_DEBUG: " fmt, dev_id, ##arg);\
            }\
        }\
    }while(0) 

#define ZXIC_COMM_TRACE_DEV_ALL(dev_id, fmt, arg...) \
    do{\
        if (zxic_comm_get_print_level() >= ZXIC_TRACE_ALL_PRINT)\
        {\
            if (zxic_comm_get_print_en())\
            {\
                DH_LOG_DEBUG(MODULE_NP, "Dev_id[%u]_ALL:" fmt, dev_id, ##arg);\
            }\
        }\
    }while(0) 

#define ZXIC_COMM_DBGCNT64_PRINT(name, value) \
    do{\
        ZXIC_COMM_PRINT("%-50s  : 0x%016llx\n", name, value);\
    }while(0) 

#define ZXIC_COMM_DBGCNT32_PRINT(name, value) \
    do{\
        ZXIC_COMM_PRINT("%-50s  : 0x%08x\n", name, value);\
    }while(0) 

#endif
#endif

#if ZXIC_REAL("互斥锁")
ZXIC_RTN32 zxic_comm_mutex_create (ZXIC_MUTEX_T *p_mutex);
ZXIC_RTN32 zxic_comm_mutex_lock   (ZXIC_MUTEX_T *p_mutex);
ZXIC_RTN32 zxic_comm_mutex_unlock (ZXIC_MUTEX_T *p_mutex);
ZXIC_RTN32 zxic_comm_mutex_destroy(ZXIC_MUTEX_T *p_mutex);
#endif

#if ZXIC_REAL("自旋锁")
ZXIC_RTN32 zxic_comm_spin_create(ZXIC_SPIN_LOCK_T *p_spin);
ZXIC_RTN32 zxic_comm_spin_lock(ZXIC_SPIN_LOCK_T *p_spin);
ZXIC_RTN32 zxic_comm_spin_try_lock(ZXIC_SPIN_LOCK_T *p_spin);
ZXIC_RTN32 zxic_comm_spin_unlock(ZXIC_SPIN_LOCK_T *p_spin);
#endif

#if ZXIC_REAL("信号量")
ZXIC_RTN32 zxic_comm_sem_create(ZXIC_SEM_T *p_sem, ZXIC_SINT32 share, ZXIC_SINT32 IniCount, ZXIC_SINT32 MaxCount);
ZXIC_RTN32 zxic_comm_sem_release(ZXIC_SEM_T *p_sem);
ZXIC_RTN32 zxic_comm_sem_wait(ZXIC_SEM_T *p_sem);
#endif

#if ZXIC_REAL("参数检查")

#if ZXIC_REAL("NO DEV_ID & ASSERT")

#define ZXIC_COMM_CHECK_RC(rc,becall)\
    do{\
        if(ZXIC_OK != rc)\
        {\
           if(ZXIC_OK != zxic_comm_errcode_check(rc))\
           {\
               ZXIC_COMM_TRACE_ERROR("\n ZXIC  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,becall);\
           }\
           ZXIC_COMM_ASSERT(0);\
           return rc;\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_RC_MEMORY_FREE(rc, becall, ptr)\
    do{\
         if (ZXIC_OK != rc)\
         {\
            if(ZXIC_OK != zxic_comm_errcode_check(rc))\
            {\
                ZXIC_COMM_TRACE_ERROR("\n ZXICP  %s:%d [ErrorCode:0x%x], %s Call %s Fail!\n", __FILE__, __LINE__, rc, __FUNCTION__, becall);\
            }\
            ZXIC_COMM_FREE(ptr);\
            ZXIC_COMM_ASSERT(0);\
            return rc;\
         }\
    } while(0)

#define ZXIC_COMM_CHECK_RC_MEMORY_VFREE(rc, becall, ptr)\
    do{\
         if (ZXIC_OK != rc)\
         {\
            if(ZXIC_OK != zxic_comm_errcode_check(rc))\
            {\
                ZXIC_COMM_TRACE_ERROR("\n ZXICP  %s:%d [ErrorCode:0x%x], %s Call %s Fail!\n", __FILE__, __LINE__, rc, __FUNCTION__, becall);\
            }\
            ZXIC_COMM_VFREE(ptr);\
            ZXIC_COMM_ASSERT(0);\
            return rc;\
         }\
    } while(0)

#define ZXIC_COMM_CHECK_POINT(point)\
    do{\
       if(ZXIC_NULL == (point))\
       {\
          ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
          ZXIC_COMM_ASSERT(0);\
          return ZXIC_PAR_CHK_POINT_NULL;\
       }\
    }while(0)
    
#define ZXIC_COMM_CHECK_POINT_MEMORY_FREE(point, ptr)\
   do{\
      if(ZXIC_NULL == (point))\
      {\
         ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
         ZXIC_COMM_FREE(ptr);\
         ZXIC_COMM_ASSERT(0);\
         return ZXIC_PAR_CHK_POINT_NULL;\
      }\
   }while(0)

#define ZXIC_COMM_CHECK_POINT_MEMORY_VFREE(point, ptr)\
   do{\
      if(ZXIC_NULL == (point))\
      {\
         ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
         ZXIC_COMM_VFREE(ptr);\
         ZXIC_COMM_ASSERT(0);\
         return ZXIC_PAR_CHK_POINT_NULL;\
      }\
   }while(0)
#define ZXIC_COMM_CHECK_POINT_EMEMORY_FREE2PTR(point, ptr0, ptr1)\
   do{\
      if(ZXIC_NULL == (point))\
      {\
         ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
         ZXIC_COMM_FREE(ptr0);\
         ZXIC_COMM_FREE(ptr1);\
         ZXIC_COMM_ASSERT(0);\
         return ZXIC_PAR_CHK_POINT_NULL;\
      }\
   }while(0)
#define ZXIC_COMM_CHECK_POINT_RETURN_NULL(point)\
    do{\
       if(ZXIC_NULL == (point))\
       {\
          ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
          ZXIC_COMM_ASSERT(0);\
          return ZXIC_NULL;\
       }\
    }while(0)

#define ZXIC_COMM_CHECK_INDEX(val,min,max)\
    do{\
        if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_index_check(val, min, max))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
        else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_index_check(val, min, max))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_RANGE;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_EQUAL(val,equal)\
    do{\
        if(val == equal)\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [equal=0x%x] !\n", __FILE__, __LINE__, val, equal);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_EQUAL_RETURN_OK(val,equal)\
    do{\
        if(val == equal)\
        {\
            return ZXIC_OK;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_NOT_EQUAL(val,equal)\
    do{\
        if(val != equal)\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [equal=0x%x] !\n", __FILE__, __LINE__, val, equal);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_NOT_EQUAL_UNLOCK(val,equal,mutex)\
    do{\
        if(val != equal)\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [equal=0x%x] !\n", __FILE__, __LINE__, val, equal);\
            if(0 != zxic_comm_mutex_unlock(mutex))\
            {   \
                ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
            }\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_UPPER(val,max)\
    do{\
        if((val) > (max))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [max=0x%x] !\n", __FILE__, __LINE__, val, max);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)    
#define ZXIC_COMM_CHECK_INDEX_LOWER(val,min)\
    do{\
        if((val) < (min))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x] !\n", __FILE__, __LINE__, val, min);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_LOWER_UNLOCK(val,min,mutex)\
    do{\
        if((val) < (min))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x] !\n", __FILE__, __LINE__, val, min);\
            if(0 != zxic_comm_mutex_unlock(mutex))\
            {   \
                ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
            }\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_BOTH(val,min,max)\
    do{\
        if(((val) < (min)) || ((val) > (max)))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_UPPER_MEMORY_FREE(val,max,ptr)\
    do{\
        if((val) > (max))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [max=0x%x] !\n", __FILE__, __LINE__, val, max);\
            ZXIC_COMM_FREE(ptr);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0) 
#define ZXIC_COMM_CHECK_INDEX_LOWER_MEMORY_FREE(val,min,ptr)\
    do{\
        if((val) < (min))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x] !\n", __FILE__, __LINE__, val, min);\
            ZXIC_COMM_FREE(ptr);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_BOTH_MEMORY_FREE(val,min,max,ptr)\
    do{\
        if(((val) < (min)) || ((val) > (max)))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_FREE(ptr);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_MEMORY_FREE(val0,val1,ptr)\
    do{\
        if((ZXIC_UINT32_MAX - (val0)) < (val1))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            ZXIC_COMM_FREE(ptr);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)  

#define ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW(val0,val1)\
    do{\
        if((ZXIC_UINT32_MAX - (val0)) < (val1))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)  


#define ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_64(val0,val1)\
do{\
    if((ZXIC_ULONG_MAX - (val0)) < (val1))\
    {\
        ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%016llx] INVALID] [val1=0x%016llx] !\n", __FILE__, __LINE__, val0, val1);\
        ZXIC_COMM_ASSERT(0);\
        return ZXIC_PAR_CHK_INVALID_INDEX;\
    }\
}while(0)  

#define ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_UNLOCK(val0, val1, mutex)\
    do{\
        if((ZXIC_UINT32_MAX - (val0)) < (val1))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            if(0 != zxic_comm_mutex_unlock(mutex))\
            {\
                ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
            }\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW(val0,val1)\
    do{\
        if((val0) < (val1))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW_64(val0,val1)\
do{\
    if((val0) < (val1))\
    {\
        ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%016llx] INVALID] [val1=0x%016llx] !\n", __FILE__, __LINE__, val0, val1);\
        ZXIC_COMM_ASSERT(0);\
        return ZXIC_PAR_CHK_INVALID_INDEX;\
    }\
}while(0)

#define ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW_UNLOCK(val0, val1, mutex)\
do{\
if((val0) < (val1))\
{\
    ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
    if(0 != zxic_comm_mutex_unlock(mutex))\
    {\
        ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
    }\
    ZXIC_COMM_ASSERT(0);\
    return ZXIC_PAR_CHK_INVALID_INDEX;\
}\
}while(0)

#define ZXIC_COMM_CHECK_INDEX_MUL_OVERFLOW(val0,val1)\
do{\
    if(((val0) > 0) && ((ZXIC_UINT32_MAX / (val0)) < (val1)))\
    {\
        ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
        ZXIC_COMM_ASSERT(0);\
        return ZXIC_PAR_CHK_INVALID_INDEX;\
    }\
}while(0)  
#define ZXIC_COMM_CHECK_INDEX_MUL_OVERFLOW_64(val0,val1)\
do{\
    if(((val0) > 0) && ((ZXIC_ULONG_MAX / (val0)) < (val1)))\
    {\
        ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%016llx] INVALID] [val1=0x%016llx] !\n", __FILE__, __LINE__, val0, val1);\
        ZXIC_COMM_ASSERT(0);\
        return ZXIC_PAR_CHK_INVALID_INDEX;\
    }\
}while(0)  

#define ZXIC_COMM_CHECK_INDEX_MUL_OVERFLOW_UNLOCK(val0, val1, mutex)\
do{\
  if(((val0) > 0) && ((ZXIC_UINT32_MAX / (val0)) < (val1)))\
  {\
      ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
      if(0 != zxic_comm_mutex_unlock(mutex))\
      {\
          ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
      }\
      ZXIC_COMM_ASSERT(0);\
      return ZXIC_PAR_CHK_INVALID_INDEX;\
  }\
}while(0)

#define ZXIC_COMM_CHECK_INDEX_RETURN_NULL(val,min,max)\
    do{\
        if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_index_check(val, min, max))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_NULL;\
        }\
        else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_index_check(val, min, max))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_NULL;\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_RC_UNLOCK(rc, becall, mutex)\
    do{\
       if(ZXIC_OK != rc)\
       {\
          if(ZXIC_OK != zxic_comm_errcode_check(rc))\
          {\
              ZXIC_COMM_TRACE_ERROR("\n ZXIC  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,becall);\
          }\
          if(0 != zxic_comm_mutex_unlock(mutex))\
          {   \
              ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
          }   \
          ZXIC_COMM_ASSERT(0);\
          return rc;\
       }\
    }while(0)

#define ZXIC_COMM_CHECK_RC_CLOSE_FP(rc, becall, fp)\
    do{\
        if(ZXIC_OK != rc)\
        {\
            if(ZXIC_OK != zxic_comm_errcode_check(rc))\
            {\
                ZXIC_COMM_TRACE_ERROR("\n ZXIC  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,becall);\
            }\
            if(ZXIC_COMM_FCLOSE(fp))\
            {\
                ZXIC_COMM_TRACE_ERROR("\n ZXIC  %s:%d !-- %s close file Fail!\n",__FILE__,__LINE__,__FUNCTION__);\
            }\
            ZXIC_COMM_ASSERT(0);\
            return rc;\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_POINT_CLOSE_FP(point, fp)\
    do{\
      if(NULL == (point))\
      {\
          ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
          if (ZXIC_COMM_FCLOSE(fp))\
          {\
              ZXIC_COMM_TRACE_ERROR("\n ZXIC  %s:%d !-- %s close file Fail!\n",__FILE__,__LINE__,__FUNCTION__);\
          }\
          ZXIC_COMM_ASSERT(0);\
          return ZXIC_PAR_CHK_POINT_NULL;\
      }\
    }while(0)\

          
#define ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_RETURN(val0, val1)\
          do{\
              if((0xFFFFFFFF - (val0)) < (val1))\
              {\
                  ZXIC_COMM_TRACE_ERROR("ICM %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
                  return;\
              }\
          }while(0)
          
#define ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW_RETURN(val0, val1)\
          do{\
              if(val0 < val1)\
              {\
                  ZXIC_COMM_TRACE_ERROR("ICM %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
                  return;\
              }\
          }while(0)
          
#define ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_CLOSE_FP_NO_ASSERT(dev_id, val0, val1, fp)\
              do{\
                  if((0xFFFFFFFF - (val0)) < (val1))\
                  {\
                     ZXIC_COMM_TRACE_DEV_ERROR(dev_id,  "ICM %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
                      if (ZXIC_COMM_FCLOSE(fp))\
                      {\
                          ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n ICM  %s:%d !-- %s close file Fail!\n",__FILE__,__LINE__,__FUNCTION__);\
                      }\
                     return ZXIC_PAR_CHK_INVALID_INDEX;\
                  }\
              }while(0)
          
#define ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_CLOSE_FP_NO_ASSERT(dev_id, val0, val1, fp)\
                      do{\
                          if((val0 > 0) && ((0xFFFFFFFF / (val0)) < (val1)))\
                          {\
                              ZXIC_COMM_TRACE_DEV_ERROR(dev_id,  "ICM %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
                              if (ZXIC_COMM_FCLOSE(fp))\
                              {\
                                  ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n ICM  %s:%d !-- %s close file Fail!\n",__FILE__,__LINE__,__FUNCTION__);\
                              }\
                              return ZXIC_PAR_CHK_INVALID_INDEX;\
                          }\
                      }while(0)

#endif

#if ZXIC_REAL("NO DEV_ID & NO ASSERT")
#define ZXIC_COMM_CHECK_RC_NO_ASSERT(rc,becall)\
    do{\
        if(ZXIC_OK != rc)\
        {\
           if(ZXIC_OK != zxic_comm_errcode_check(rc))\
           {\
               ZXIC_COMM_TRACE_ERROR("\n ZXIC  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,becall);\
           }\
           return rc;\
        }\
      }while(0)

#define ZXIC_COMM_CHECK_RC_NO_ASSERT_UNLOCK(rc, becall, mutex)\
    do{\
        if(ZXIC_OK != rc)\
        {\
           if(ZXIC_OK != zxic_comm_errcode_check(rc))\
           {\
               ZXIC_COMM_TRACE_ERROR("\n ZXIC  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,becall);\
           }\
           if(0 != zxic_comm_mutex_unlock(mutex))\
           {   \
               ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
           }   \
           return rc;\
        }\
      }while(0)

#define ZXIC_COMM_CHECK_RC_MEMORY_FREE_NO_ASSERT(rc, becall, ptr)\
  do{\
       if (ZXIC_OK != rc)\
       {\
          if(ZXIC_OK != zxic_comm_errcode_check(rc))\
          {\
              ZXIC_COMM_TRACE_ERROR("\n ZXICP  %s:%d [ErrorCode:0x%x], %s Call %s Fail!\n", __FILE__, __LINE__, rc, __FUNCTION__, becall);\
          }\
          ZXIC_COMM_FREE(ptr);\
          return rc;\
       }\
  }while(0)

#define ZXIC_COMM_CHECK_RC_MEMORY_VFREE_NO_ASSERT(rc, becall, ptr)\
  do{\
       if (ZXIC_OK != rc)\
       {\
          if(ZXIC_OK != zxic_comm_errcode_check(rc))\
          {\
              ZXIC_COMM_TRACE_ERROR("\n ZXICP  %s:%d [ErrorCode:0x%x], %s Call %s Fail!\n", __FILE__, __LINE__, rc, __FUNCTION__, becall);\
          }\
          ZXIC_COMM_VFREE(ptr);\
          return rc;\
       }\
  }while(0)

#define ZXIC_COMM_CHECK_RC_MEMORY_FREE_UNLOCK_NO_ASSERT(rc, becall, ptr, mutex)\
  do{\
       if (ZXIC_OK != rc)\
       {\
          if(ZXIC_OK != zxic_comm_errcode_check(rc))\
          {\
              ZXIC_COMM_TRACE_ERROR("\n ZXICP  %s:%d [ErrorCode:0x%x], %s Call %s Fail!\n", __FILE__, __LINE__, rc, __FUNCTION__, becall);\
          }\
          ZXIC_COMM_FREE(ptr);\
          if(0 != zxic_comm_mutex_unlock(mutex))\
          {   \
                ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
          }   \
          return rc;\
       }\
  }while(0)

#define ZXIC_COMM_CHECK_RC_MEMORY_VFREE_UNLOCK_NO_ASSERT(rc, becall, ptr, mutex)\
  do{\
       if (ZXIC_OK != rc)\
       {\
          if(ZXIC_OK != zxic_comm_errcode_check(rc))\
          {\
              ZXIC_COMM_TRACE_ERROR("\n ZXICP  %s:%d [ErrorCode:0x%x], %s Call %s Fail!\n", __FILE__, __LINE__, rc, __FUNCTION__, becall);\
          }\
          ZXIC_COMM_VFREE(ptr);\
          if(0 != zxic_comm_mutex_unlock(mutex))\
          {   \
                ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
          }   \
          return rc;\
       }\
  }while(0)

#define ZXIC_COMM_CHECK_RC_MEMORY_FREE2PTR_NO_ASSERT(rc, becall, ptr1, ptr2)\
    do{\
          if (ZXIC_OK != rc)\
          {\
              if(ZXIC_OK != zxic_comm_errcode_check(rc))\
              {\
                  ZXIC_COMM_TRACE_ERROR("\n ZXICP  %s:%d [ErrorCode:0x%x], %s Call %s Fail!\n", __FILE__, __LINE__, rc, __FUNCTION__, becall);\
              }\
              ZXIC_COMM_FREE(ptr1);\
              ZXIC_COMM_FREE(ptr2);\
              return rc;\
          }\
      }while(0)

#define ZXIC_COMM_CHECK_RC_MEMORY_VFREE2PTR_NO_ASSERT(rc, becall, ptr1, ptr2)\
    do{\
          if (ZXIC_OK != rc)\
          {\
              if(ZXIC_OK != zxic_comm_errcode_check(rc))\
              {\
                  ZXIC_COMM_TRACE_ERROR("\n ZXICP  %s:%d [ErrorCode:0x%x], %s Call %s Fail!\n", __FILE__, __LINE__, rc, __FUNCTION__, becall);\
              }\
              ZXIC_COMM_VFREE(ptr1);\
              ZXIC_COMM_VFREE(ptr2);\
              return rc;\
          }\
      }while(0)

#define ZXIC_COMM_CHECK_RC_MEMORY_FREE2PTR_UNLOCK_NO_ASSERT(rc, becall, ptr1, ptr2, mutex)\
    do{\
          if (ZXIC_OK != rc)\
          {\
              if(ZXIC_OK != zxic_comm_errcode_check(rc))\
              {\
                  ZXIC_COMM_TRACE_ERROR("\n ZXICP  %s:%d [ErrorCode:0x%x], %s Call %s Fail!\n", __FILE__, __LINE__, rc, __FUNCTION__, becall);\
              }\
              ZXIC_COMM_FREE(ptr1);\
              ZXIC_COMM_FREE(ptr2);\
              if(0 != zxic_comm_mutex_unlock(mutex))\
              {   \
                  ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
              }   \
              return rc;\
          }\
      }while(0)

#define ZXIC_COMM_CHECK_RC_MEMORY_VFREE2PTR_UNLOCK_NO_ASSERT(rc, becall, ptr1, ptr2, mutex)\
    do{\
          if (ZXIC_OK != rc)\
          {\
              if(ZXIC_OK != zxic_comm_errcode_check(rc))\
              {\
                  ZXIC_COMM_TRACE_ERROR("\n ZXICP  %s:%d [ErrorCode:0x%x], %s Call %s Fail!\n", __FILE__, __LINE__, rc, __FUNCTION__, becall);\
              }\
              ZXIC_COMM_VFREE(ptr1);\
              ZXIC_COMM_VFREE(ptr2);\
              if(0 != zxic_comm_mutex_unlock(mutex))\
              {   \
                  ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
              }   \
              return rc;\
          }\
      }while(0)
#define ZXIC_COMM_CHECK_RC_MEMORY_FREE3PTR_NO_ASSERT(rc, becall, ptr1, ptr2, ptr3)\
    do{\
          if (ZXIC_OK != rc)\
          {\
              if(ZXIC_OK != zxic_comm_errcode_check(rc))\
              {\
                  ZXIC_COMM_TRACE_ERROR("\n ZXICP  %s:%d [ErrorCode:0x%x], %s Call %s Fail!\n", __FILE__, __LINE__, rc, __FUNCTION__, becall);\
              }\
              ZXIC_COMM_FREE(ptr1);\
              ZXIC_COMM_FREE(ptr2);\
              ZXIC_COMM_FREE(ptr3);\
              return rc;\
          }\
      }while(0)

#define ZXIC_COMM_CHECK_RC_MEMORY_VFREE3PTR_NO_ASSERT(rc, becall, ptr1, ptr2, ptr3)\
    do{\
          if (ZXIC_OK != rc)\
          {\
              if(ZXIC_OK != zxic_comm_errcode_check(rc))\
              {\
                  ZXIC_COMM_TRACE_ERROR("\n ZXICP  %s:%d [ErrorCode:0x%x], %s Call %s Fail!\n", __FILE__, __LINE__, rc, __FUNCTION__, becall);\
              }\
              ZXIC_COMM_VFREE(ptr1);\
              ZXIC_COMM_VFREE(ptr2);\
              ZXIC_COMM_VFREE(ptr3);\
              return rc;\
          }\
      }while(0)

#define ZXIC_COMM_CHECK_RC_MEMORY_FREE3PTR_UNLOCK_NO_ASSERT(rc, becall, ptr1, ptr2, ptr3, mutex)\
    do{\
          if (ZXIC_OK != rc)\
          {\
              if(ZXIC_OK != zxic_comm_errcode_check(rc))\
              {\
                  ZXIC_COMM_TRACE_ERROR("\n ZXICP  %s:%d [ErrorCode:0x%x], %s Call %s Fail!\n", __FILE__, __LINE__, rc, __FUNCTION__, becall);\
              }\
              ZXIC_COMM_FREE(ptr1);\
              ZXIC_COMM_FREE(ptr2);\
              ZXIC_COMM_FREE(ptr3);\
              if(0 != zxic_comm_mutex_unlock(mutex))\
              {   \
                  ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
              }   \
              return rc;\
          }\
      }while(0)

#define ZXIC_COMM_CHECK_POINT_NO_ASSERT(point)\
    do{\
         if(ZXIC_NULL == (point))\
         {\
            ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
            return ZXIC_PAR_CHK_POINT_NULL;\
         }\
     }while(0)

#define ZXIC_COMM_CHECK_POINT_CLOSE_FP_NO_ASSERT(point, fp)\
     do{\
       if(NULL == (point))\
       {\
           ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
           if (ZXIC_COMM_FCLOSE(fp))\
           {\
               ZXIC_COMM_TRACE_ERROR("\n ZXIC  %s:%d !-- %s close file Fail!\n",__FILE__,__LINE__,__FUNCTION__);\
           }\
           return ZXIC_PAR_CHK_POINT_NULL;\
       }\
     }while(0)

#define ZXIC_COMM_CHECK_POINT_MEMORY_FREE_NO_ASSERT(point,ptr)\
    do{\
         if(ZXIC_NULL == (point))\
         {\
            ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
            ZXIC_COMM_FREE(ptr);\
            return ZXIC_PAR_CHK_POINT_NULL;\
         }\
     }while(0)

#define ZXIC_COMM_CHECK_POINT_MEMORY_VFREE_NO_ASSERT(point,ptr)\
    do{\
         if(ZXIC_NULL == (point))\
         {\
            ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
            ZXIC_COMM_VFREE(ptr);\
            return ZXIC_PAR_CHK_POINT_NULL;\
         }\
     }while(0)

#define ZXIC_COMM_CHECK_POINT_MEMORY_FREE2PTR_NO_ASSERT(point, ptr1, ptr2)\
    do{\
          if(ZXIC_NULL == (point))\
          {\
              ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
              ZXIC_COMM_FREE(ptr1);\
              ZXIC_COMM_FREE(ptr2);\
              return ZXIC_PAR_CHK_POINT_NULL;\
          }\
      }while(0)

#define ZXIC_COMM_CHECK_POINT_MEMORY_VFREE2PTR_NO_ASSERT(point, ptr1, ptr2)\
    do{\
          if(ZXIC_NULL == (point))\
          {\
              ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
              ZXIC_COMM_VFREE(ptr1);\
              ZXIC_COMM_VFREE(ptr2);\
              return ZXIC_PAR_CHK_POINT_NULL;\
          }\
      }while(0)

#define ZXIC_COMM_CHECK_POINT_MEMORY_FREE3PTR_NO_ASSERT(point, ptr1, ptr2, ptr3)\
    do{\
          if(ZXIC_NULL == (point))\
          {\
              ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
              ZXIC_COMM_FREE(ptr1);\
              ZXIC_COMM_FREE(ptr2);\
              ZXIC_COMM_FREE(ptr3);\
              return ZXIC_PAR_CHK_POINT_NULL;\
          }\
      }while(0)

#define ZXIC_COMM_CHECK_INDEX_NO_ASSERT(val,min,max)\
   do{\
       if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_index_check(val, min, max))\
       {\
           ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
           return ZXIC_PAR_CHK_INVALID_INDEX;\
       }\
       else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_index_check(val, min, max))\
       {\
           ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
           return ZXIC_PAR_CHK_INVALID_RANGE;\
       }\
   }while (0)

#define ZXIC_COMM_CHECK_INDEX_NO_ASSERT_UNLOCK(val, min, max, mutex)\
   do{\
       if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_index_check(val, min, max))\
       {\
           ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
           if(0 != zxic_comm_mutex_unlock(mutex))\
           {   \
               ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
           }   \
           return ZXIC_PAR_CHK_INVALID_INDEX;\
       }\
       else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_index_check(val, min, max))\
       {\
           ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
           if(0 != zxic_comm_mutex_unlock(mutex))\
           {   \
               ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
           }   \
           return ZXIC_PAR_CHK_INVALID_RANGE;\
       }\
    }while(0)

#define ZXIC_COMM_CHECK_INDEX_UPPER_NO_ASSERT(val, max)\
   do{\
       if((val) > (max))\
       {\
           ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [max=0x%x] !\n", __FILE__, __LINE__, val, max);\
           return ZXIC_PAR_CHK_INVALID_INDEX;\
       }\
   }while(0)    

#define ZXIC_COMM_CHECK_INDEX_UPPER_NO_ASSERT_UNLOCK(val, max, mutex)\
  do{\
      if((val) > (max))\
      {\
          ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [max=0x%x] !\n", __FILE__, __LINE__, val, max);\
          if(0 != zxic_comm_mutex_unlock(mutex))\
          {   \
              ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
          }   \
          return ZXIC_PAR_CHK_INVALID_INDEX;\
      }\
  }while(0)   
#define ZXIC_COMM_CHECK_INDEX_UPPER_MEMORY_FREE_NO_ASSERT(val, max, ptr)\
 do{\
     if((val) > (max))\
     {\
         ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [max=0x%x] !\n", __FILE__, __LINE__, val, max);\
         ZXIC_COMM_FREE(ptr);\
         return ZXIC_PAR_CHK_INVALID_INDEX;\
     }\
 }while(0) 

#define ZXIC_COMM_CHECK_INDEX_LOWER_NO_ASSERT(val,min)\
   do{\
       if((val) < (min))\
       {\
           ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x] !\n", __FILE__, __LINE__, val, min);\
           return ZXIC_PAR_CHK_INVALID_INDEX;\
       }\
   }while(0)

#define ZXIC_COMM_CHECK_INDEX_LOWER_NO_ASSERT_UNLOCK(val, min, mutex)\
  do{\
      if((val) < (min))\
      {\
          ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x] !\n", __FILE__, __LINE__, val, min);\
          if(0 != zxic_comm_mutex_unlock(mutex))\
          {   \
              ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
          }   \
          return ZXIC_PAR_CHK_INVALID_INDEX;\
      }\
  }while(0)

#define ZXIC_COMM_CHECK_INDEX_LOWER_MEMORY_FREE_NO_ASSERT(val,min,ptr)\
   do{\
       if((val) < (min))\
       {\
           ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x] !\n", __FILE__, __LINE__, val, min);\
           ZXIC_COMM_FREE(ptr);\
           return ZXIC_PAR_CHK_INVALID_INDEX;\
       }\
   }while(0)       
#define ZXIC_COMM_CHECK_INDEX_BOTH_NO_ASSERT(val, min, max)\
   do{\
       if(((val) < (min)) || ((val) > (max)))\
       {\
           ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
           return ZXIC_PAR_CHK_INVALID_INDEX;\
       }\
   }while(0) 

#define ZXIC_COMM_CHECK_INDEX_BOTH_NO_ASSERT_UNLOCK(val, min, max, mutex)\
  do{\
      if(((val) < (min)) || ((val) > (max)))\
      {\
          ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
          if(0 != zxic_comm_mutex_unlock(mutex))\
          {   \
              ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
          }   \
          return ZXIC_PAR_CHK_INVALID_INDEX;\
      }\
  }while(0) 
#define ZXIC_COMM_CHECK_INDEX_BOTH_MEMORY_FREE_NO_ASSERT(val, min, max, ptr)\
   do{\
       if(((val) < (min)) || ((val) > (max)))\
       {\
           ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
           ZXIC_COMM_FREE(ptr);\
           return ZXIC_PAR_CHK_INVALID_INDEX;\
       }\
   }while(0)       
#define ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT(val0, val1)\
      do{\
          if((ZXIC_UINT32_MAX - (val0)) < (val1))\
          {\
              ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
              return ZXIC_PAR_CHK_INVALID_INDEX;\
          }\
      }while(0)

#define ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_64_NO_ASSERT(val0,val1)\
      do{\
          if((ZXIC_ULONG_MAX - (val0)) < (val1))\
          {\
              ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
              return ZXIC_PAR_CHK_INVALID_INDEX;\
          }\
      }while(0)  

#define ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NO_ASSERT_UNLOCK(val0, val1, mutex)\
do{\
    if((ZXIC_UINT32_MAX - (val0)) < (val1))\
    {\
        ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
        if(0 != zxic_comm_mutex_unlock(mutex))\
        {\
            ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
        }\
        return ZXIC_PAR_CHK_INVALID_INDEX;\
    }\
}while(0)

#define ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW_NO_ASSERT(val0, val1)\
       do{\
          if((val0) < (val1))\
          {\
              ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
              return ZXIC_PAR_CHK_INVALID_INDEX;\
          }\
       }while(0)

#define ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW_NO_ASSERT_UNLOCK(val0, val1, mutex)\
 do{\
    if((val0) < (val1))\
    {\
        ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
        if(0 != zxic_comm_mutex_unlock(mutex))\
        {\
            ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
        }\
        return ZXIC_PAR_CHK_INVALID_INDEX;\
    }\
 }while(0)

#define ZXIC_COMM_CHECK_INDEX_MUL_OVERFLOW_NO_ASSERT(val0, val1)\
       do{\
           if(((val0) > 0) && ((ZXIC_UINT32_MAX / (val0)) < (val1)))\
           {\
              ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
              return ZXIC_PAR_CHK_INVALID_INDEX;\
           }\
       }while(0)

#define ZXIC_COMM_CHECK_INDEX_MUL_OVERFLOW_NO_ASSERT_UNLOCK(val0, val1, mutex)\
      do{\
          if(((val0) > 0) && ((ZXIC_UINT32_MAX / (val0)) < (val1)))\
          {\
             ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
              if(0 != zxic_comm_mutex_unlock(mutex))\
              {\
                  ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
              }\
             return ZXIC_PAR_CHK_INVALID_INDEX;\
          }\
      }while(0)
#define ZXIC_COMM_CHECK_INDEX_RETURN_VOID_NO_ASSERT(val,min,max)\
  do{\
      if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_index_check(val, min, max))\
      {\
          ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
          return ;\
      }\
      else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_index_check(val, min, max))\
      {\
          ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
          return ;\
      }\
  }while (0)

#define ZXIC_COMM_CHECK_INDEX_RETURN_NULL_NO_ASSERT(val,min,max)\
       do{\
           if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_index_check(val, min, max))\
           {\
               ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
               return ZXIC_NULL;\
           }\
           else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_index_check(val, min, max))\
           {\
               ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
               return ZXIC_NULL;\
           }\
       }while(0)

#define ZXIC_COMM_CHECK_INDEX_CLOSE_FP_NO_ASSERT(val,min,max,fp)\
    do{\
        if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_index_check(val, min, max))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            if (ZXIC_COMM_FCLOSE(fp))\
            {\
                ZXIC_COMM_TRACE_ERROR("\n ZXIC  %s:%d !-- %s close file Fail!\n",__FILE__,__LINE__,__FUNCTION__);\
            }\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
        else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_index_check(val, min, max))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            if (ZXIC_COMM_FCLOSE(fp))\
            {\
                ZXIC_COMM_TRACE_ERROR("\n ZXIC  %s:%d !-- %s close file Fail!\n",__FILE__,__LINE__,__FUNCTION__);\
            }\
            return ZXIC_PAR_CHK_INVALID_RANGE;\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_INDEX_MEMORY_FREE_NO_ASSERT(val,min,max,ptr)\
    do{\
          if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_index_check(val, min, max))\
          {\
              ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
              ZXIC_COMM_FREE(ptr);\
              return ZXIC_PAR_CHK_INVALID_INDEX;\
          }\
          else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_index_check(val, min, max))\
          {\
              ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
              ZXIC_COMM_FREE(ptr);\
              return ZXIC_PAR_CHK_INVALID_RANGE;\
          }\
      }while(0)
#define ZXIC_COMM_CHECK_INDEX_MEMORY_FREE2PTR_NO_ASSERT(val, min, max, ptr1, ptr2)\
    do{\
          if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_index_check(val, min, max))\
          {\
              ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
              ZXIC_COMM_FREE(ptr1);\
              ZXIC_COMM_FREE(ptr2);\
              return ZXIC_PAR_CHK_INVALID_INDEX;\
          }\
          else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_index_check(val, min, max))\
          {\
              ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
              ZXIC_COMM_FREE(ptr1);\
              ZXIC_COMM_FREE(ptr2);\
              return ZXIC_PAR_CHK_INVALID_RANGE;\
          }\
      }while(0)
#define ZXIC_COMM_CHECK_INDEX_MEMORY_FREE3PTR_NO_ASSERT(val, min, max, ptr1, ptr2, ptr3)\
    do{\
          if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_index_check(val, min, max))\
          {\
              ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
              ZXIC_COMM_FREE(ptr1);\
              ZXIC_COMM_FREE(ptr2);\
              ZXIC_COMM_FREE(ptr3);\
              return ZXIC_PAR_CHK_INVALID_INDEX;\
          }\
          else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_index_check(val, min, max))\
          {\
              ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
              ZXIC_COMM_FREE(ptr1);\
              ZXIC_COMM_FREE(ptr2);\
              ZXIC_COMM_FREE(ptr3);\
              return ZXIC_PAR_CHK_INVALID_RANGE;\
          }\
      }while(0)

#endif

#if ZXIC_REAL("DEV_ID & ASSERT")

#define ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, becall)\
   do{\
       if(ZXIC_OK != rc)\
       {\
          if(ZXIC_OK != zxic_comm_errcode_check(rc))\
          {\
              ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ZXIC  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,becall);\
          }\
          ZXIC_COMM_ASSERT(0);\
          return rc;\
       }\
   }while(0)
#define ZXIC_COMM_CHECK_DEV_RC_NULL(dev_id,rc,becall)\
       do{\
           if(ZXIC_OK != rc)\
           {\
               if(ZXIC_OK != zxic_comm_errcode_check(rc))\
               {\
                   ZXIC_COMM_TRACE_ERROR("\n ZXIC  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,becall);\
               }\
               ZXIC_COMM_ASSERT(0);\
               return NULL;\
           }\
       }while(0)

#define ZXIC_COMM_CHECK_DEV_RC_UNLOCK(dev_id,rc, becall, mutex)\
    do{\
         if(ZXIC_OK != rc)\
         {\
            if(ZXIC_OK != zxic_comm_errcode_check(rc))\
            {\
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ZXIC  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,becall);\
            }\
            if(0 != zxic_comm_mutex_unlock(mutex))\
            {   \
                ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
            }   \
            ZXIC_COMM_ASSERT(0);\
            return rc;\
         }\
    }while(0)

#define ZXIC_COMM_CHECK_DEV_RC_CLOSE_FP(dev_id, rc, becall, fp)\
    do{\
        if(ZXIC_OK != rc)\
        {\
            if(ZXIC_OK != zxic_comm_errcode_check(rc))\
            {\
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n ZXIC  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,becall);\
            }\
            if(ZXIC_COMM_FCLOSE(fp))\
            {\
                ZXIC_COMM_TRACE_ERROR("\n ZXIC  %s:%d !-- %s close file Fail!\n",__FILE__,__LINE__,__FUNCTION__);\
            }\
            ZXIC_COMM_ASSERT(0);\
            return rc;\
        }\
    }while(0)
                
#define ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE(dev_id, rc, becall, ptr)\
    do{\
         if (ZXIC_OK != rc)\
         {\
            if(ZXIC_OK != zxic_comm_errcode_check(rc))\
            {\
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n ZXICP  %s:%d [ErrorCode:0x%x], %s Call %s Fail!\n", __FILE__, __LINE__, rc, __FUNCTION__, becall);\
            }\
            ZXIC_COMM_FREE(ptr);\
            ZXIC_COMM_ASSERT(0);\
            return rc;\
         }\
    } while (0)

#define ZXIC_COMM_CHECK_DEV_POINT(dev_id, point)\
    do{\
         if(NULL == (point))\
         {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_POINT_NULL;\
         }\
     }while(0)
#define ZXIC_COMM_CHECK_DEV_POINT_MEMORY_FREE(dev_id, point, ptr)\
   do{\
      if(ZXIC_NULL == (point))\
      {\
         ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
         ZXIC_COMM_FREE(ptr);\
         ZXIC_COMM_ASSERT(0);\
         return ZXIC_PAR_CHK_POINT_NULL;\
      }\
   }while(0)
#define ZXIC_COMM_CHECK_DEV_POINT_EMEMORY_FREE2PTR(dev_id, point, ptr0, ptr1)\
   do{\
      if(ZXIC_NULL == (point))\
      {\
         ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
         ZXIC_COMM_FREE(ptr0);\
         ZXIC_COMM_FREE(ptr1);\
         ZXIC_COMM_ASSERT(0);\
         return ZXIC_PAR_CHK_POINT_NULL;\
      }\
   }while(0)     
#define ZXIC_COMM_CHECK_DEV_POINT_RETURN_NULL(dev_id,point)\
        do{\
             if(NULL == (point))\
             {\
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
                ZXIC_COMM_ASSERT(0);\
                return ZXIC_NULL;\
             }\
         }while(0)

#define ZXIC_COMM_CHECK_DEV_POINT_UNLOCK(dev_id,point,mutex)\
    do{\
         if(NULL == (point))\
         {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
            if(0 != zxic_comm_mutex_unlock(mutex))\
            {   \
                ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
            }   \
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_POINT_NULL;\
         }\
    }while(0)

#define ZXIC_COMM_CHECK_DEV_POINT_CLOSE_FP(dev_id, point, fp)\
    do{\
        if(NULL == (point))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
            if (ZXIC_COMM_FCLOSE(fp))\
            {\
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n ZXIC  %s:%d !-- %s close file Fail!\n",__FILE__,__LINE__,__FUNCTION__);\
            }\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_POINT_NULL;\
        }\
    }while(0)\

#define ZXIC_COMM_CHECK_DEV_INDEX(dev_id,val,min,max)\
    do{\
        if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_dev_index_check(dev_id, val, min, max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
        else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_dev_index_check(dev_id, val, min, max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_RANGE;\
        }\
     }while(0)
#define ZXIC_COMM_CHECK_DEV_INDEX_UPPER(dev_id,val,max)\
    do{\
        if((val) > (max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [max=0x%x] !\n", __FILE__, __LINE__, val, max);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)    
#define ZXIC_COMM_CHECK_DEV_INDEX_LOWER(dev_id,val,min)\
    do{\
        if((val) < (min))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x] !\n", __FILE__, __LINE__, val, min);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_DEV_INDEX_BOTH(dev_id,val,min,max)\
    do{\
        if(((val) < (min)) || ((val) > (max)))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_DEV_INDEX_UPPER_MEMORY_FREE(dev_id,val,max,ptr)\
    do{\
        if((val) > (max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [max=0x%x] !\n", __FILE__, __LINE__, val, max);\
            ZXIC_COMM_FREE(ptr);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)    
#define ZXIC_COMM_CHECK_DEV_INDEX_LOWER_MEMORY_FREE(dev_id,val,min,ptr)\
    do{\
        if((val) < (min))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x] !\n", __FILE__, __LINE__, val, min);\
            ZXIC_COMM_FREE(ptr);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_DEV_INDEX_BOTH_MEMORY_FREE(dev_id,val,min,max,ptr)\
    do{\
        if(((val) < (min)) || ((val) > (max)))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_FREE(ptr);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_DEV_INDEX_RETURN_NULL(dev_id,val,min,max)\
    do{\
        if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_index_check(val, min, max))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_NULL;\
        }\
        else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_index_check(val, min, max))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_NULL;\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_DEV_ID(dev_id)\
    do{\
        if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_index_check(dev_id, 0, zxic_comm_channel_max_get() - 1))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, dev_id, 0, zxic_comm_channel_max_get() - 1);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
        else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_index_check(dev_id, 0, zxic_comm_channel_max_get() - 1))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, dev_id, 0, zxic_comm_channel_max_get() - 1);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_RANGE;\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW(dev_id,val0,val1)\
    do{\
        if((ZXIC_UINT32_MAX - (val0)) < (val1))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)
        
#define ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_64(dev_id,val0,val1)\
    do{\
        if((ZXIC_ULONG_MAX - (val0)) < (val1))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[val0=0x%016llx] INVALID] [val1=0x%016llx] !\n", __FILE__, __LINE__, val0, val1);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)
        
#define ZXIC_COMM_CHECK_DEV_INDEX_SUB_OVERFLOW(dev_id, val0, val1)\
    do{\
        if((val0) < (val1))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)  
        
#define ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW(dev_id,val0,val1)\
    do{\
        if(((val0) > 0) && ((ZXIC_UINT32_MAX / (val0)) < (val1)))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)  

#define ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_64(dev_id, val0, val1)\
        do{\
            if(((val0) > 0) && ((0xFFFFFFFFFFFFFFFF / (val0)) < (val1)))\
            {\
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[val0=0x%016llx] INVALID] [val1=0x%016llx] !\n", __FILE__, __LINE__, val0, val1);\
                ZXIC_COMM_ASSERT(0);\
                return ZXIC_PAR_CHK_INVALID_INDEX;\
            }\
        }while(0) 


#endif

#if ZXIC_REAL("DEV_ID & NO ASSERT")
#define ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT(dev_id,rc,becall)\
    do{\
        if(ZXIC_OK != rc)\
        {\
           if(ZXIC_OK != zxic_comm_errcode_check(rc))\
           {\
               ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ZXIC  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,becall);\
           }\
           return rc;\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_UNLOCK(dev_id,rc, becall, mutex)\
    do{\
         if(ZXIC_OK != rc)\
         {\
            if(ZXIC_OK != zxic_comm_errcode_check(rc))\
            {\
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ZXIC  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,becall);\
            }\
            if(0 != zxic_comm_mutex_unlock(mutex))\
            {   \
                ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
            }   \
            return rc;\
         }\
    }while(0)

#define ZXIC_COMM_CHECK_DEV_RC_NO_ASSERT_SPIN_UNLOCK(dev_id,rc, becall, spin)\
    do{\
         if(ZXIC_OK != rc)\
         {\
            if(ZXIC_OK != zxic_comm_errcode_check(rc))\
            {\
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ZXIC  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,becall);\
            }\
            if(0 != zxic_comm_spin_unlock(spin))\
            {   \
                ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u spin unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
            }   \
            return rc;\
         }\
    }while(0)

#define ZXIC_COMM_CHECK_DEV_RC_CLOSE_FP_NO_ASSERT(dev_id, rc, becall, fp)\
    do{\
        if(ZXIC_OK != rc)\
        {\
            if(ZXIC_OK != zxic_comm_errcode_check(rc))\
            {\
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ZXIC  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,becall);\
            }\
            if (ZXIC_COMM_FCLOSE(fp))\
            {\
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n ZXIC  %s:%d !-- %s close file Fail!\n",__FILE__,__LINE__,__FUNCTION__);\
            }\
            return rc;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE_NO_ASSERT(dev_id, rc, becall, ptr)\
do{\
     if (ZXIC_OK != rc)\
     {\
        if(ZXIC_OK != zxic_comm_errcode_check(rc))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n ZXICP  %s:%d [ErrorCode:0x%x], %s Call %s Fail!\n", __FILE__, __LINE__, rc, __FUNCTION__, becall);\
        }\
        ZXIC_COMM_FREE(ptr);\
        return rc;\
     }\
} while (0)
#define ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE2PTR_NO_ASSERT(dev_id, rc, becall, ptr1, ptr2)\
do{\
     if (ZXIC_OK != rc)\
     {\
        if(ZXIC_OK != zxic_comm_errcode_check(rc))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n ZXICP  %s:%d [ErrorCode:0x%x], %s Call %s Fail!\n", __FILE__, __LINE__, rc, __FUNCTION__, becall);\
        }\
        ZXIC_COMM_FREE(ptr1);\
        ZXIC_COMM_FREE(ptr2);\
        return rc;\
     }\
} while (0)
#define ZXIC_COMM_CHECK_DEV_RC_MEMORY_FREE3PTR_NO_ASSERT(dev_id, rc, becall, ptr1, ptr2, ptr3)\
do{\
     if (ZXIC_OK != rc)\
     {\
        if(ZXIC_OK != zxic_comm_errcode_check(rc))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n ZXICP  %s:%d [ErrorCode:0x%x], %s Call %s Fail!\n", __FILE__, __LINE__, rc, __FUNCTION__, becall);\
        }\
        ZXIC_COMM_FREE(ptr1);\
        ZXIC_COMM_FREE(ptr2);\
        ZXIC_COMM_FREE(ptr3);\
        return rc;\
     }\
} while (0)

#define ZXIC_COMM_CHECK_DEV_RC_MEMORY_VFREE3PTR_NO_ASSERT(dev_id, rc, becall, ptr1, ptr2, ptr3)\
do{\
     if (ZXIC_OK != rc)\
     {\
        if(ZXIC_OK != zxic_comm_errcode_check(rc))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n ZXICP  %s:%d [ErrorCode:0x%x], %s Call %s Fail!\n", __FILE__, __LINE__, rc, __FUNCTION__, becall);\
        }\
        ZXIC_COMM_VFREE(ptr1);\
        ZXIC_COMM_VFREE(ptr2);\
        ZXIC_COMM_VFREE(ptr3);\
        return rc;\
     }\
} while (0)

#define ZXIC_COMM_CHECK_DEV_POINT_NO_ASSERT(dev_id,point)\
    do{\
         if(NULL == (point))\
         {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
            return ZXIC_PAR_CHK_POINT_NULL;\
         }\
     }while(0)

#define ZXIC_COMM_CHECK_DEV_POINT_RETURN_NULL_NO_ASSERT(dev_id,point)\
     do{\
        if(ZXIC_NULL == (point))\
        {\
           ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
           return ZXIC_NULL;\
        }\
     }while(0)

#define ZXIC_COMM_CHECK_DEV_POINT_CLOSE_FP_NO_ASSERT(dev_id, point, fp)\
    do{\
        if(NULL == (point))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
            if (ZXIC_COMM_FCLOSE(fp))\
            {\
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n ZXIC  %s:%d !-- %s close file Fail!\n",__FILE__,__LINE__,__FUNCTION__);\
            }\
            return ZXIC_PAR_CHK_POINT_NULL;\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_DEV_POINT_MEMORY_FREE_NO_ASSERT(dev_id,point,ptr)\
    do{\
         if(ZXIC_NULL == (point))\
         {\
            ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
            ZXIC_COMM_FREE(ptr);\
            return ZXIC_PAR_CHK_POINT_NULL;\
         }\
     }while(0)

#define ZXIC_COMM_CHECK_DEV_POINT_MEMORY_FREE2PTR_NO_ASSERT(dev_id, point, ptr1, ptr2)\
    do{\
          if(ZXIC_NULL == (point))\
          {\
              ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
              ZXIC_COMM_FREE(ptr1);\
              ZXIC_COMM_FREE(ptr2);\
              return ZXIC_PAR_CHK_POINT_NULL;\
          }\
      }while(0)

#define ZXIC_COMM_CHECK_DEV_POINT_MEMORY_FREE3PTR_NO_ASSERT(dev_id, point, ptr1, ptr2, ptr3)\
    do{\
          if(ZXIC_NULL == (point))\
          {\
              ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
              ZXIC_COMM_FREE(ptr1);\
              ZXIC_COMM_FREE(ptr2);\
              ZXIC_COMM_FREE(ptr3);\
              return ZXIC_PAR_CHK_POINT_NULL;\
          }\
      }while(0)

#define ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT(dev_id,val,min,max)\
    do{\
        if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_dev_index_check(dev_id, val, min, max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
        else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_dev_index_check(dev_id, val, min, max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            return ZXIC_PAR_CHK_INVALID_RANGE;\
        }\
     }while(0)

#define ZXIC_COMM_CHECK_DEV_INDEX_NO_ASSERT_UNLOCK(dev_id, val, min, max, mutex)\
    do{\
        if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_dev_index_check(dev_id, val, min, max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            if(0 != zxic_comm_mutex_unlock(mutex))\
            {   \
                ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
            }   \
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
        else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_dev_index_check(dev_id, val, min, max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            if(0 != zxic_comm_mutex_unlock(mutex))\
            {   \
                ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
            }   \
            return ZXIC_PAR_CHK_INVALID_RANGE;\
        }\
     }while(0)
  
#define ZXIC_COMM_CHECK_DEV_INDEX_UPPER_NO_ASSERT(dev_id,val,max)\
    do{\
        if((val) > (max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [max=0x%x] !\n", __FILE__, __LINE__, val, max);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)    

#define ZXIC_COMM_CHECK_DEV_INDEX_UPPER_NO_ASSERT_UNLOCK(dev_id, val, max, mutex)\
    do{\
        if((val) > (max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [max=0x%x] !\n", __FILE__, __LINE__, val, max);\
            if(0 != zxic_comm_mutex_unlock(mutex))\
            {   \
                ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
            }   \
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)  
#define ZXIC_COMM_CHECK_DEV_INDEX_UPPER_MEMORY_FREE_NO_ASSERT(dev_id,val,max,ptr)\
    do{\
        if((val) > (max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [max=0x%x] !\n", __FILE__, __LINE__, val, max);\
            ZXIC_COMM_FREE(ptr);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)   

        
#define ZXIC_COMM_CHECK_DEV_INDEX_LOWER_NO_ASSERT(dev_id,val,min)\
    do{\
        if((val) < (min))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x] !\n", __FILE__, __LINE__, val, min);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_DEV_INDEX_LOWER_NO_ASSERT_UNLOCK(dev_id, val, min, mutex)\
    do{\
        if((val) < (min))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x] !\n", __FILE__, __LINE__, val, min);\
            if(0 != zxic_comm_mutex_unlock(mutex))\
            {   \
                ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
            }   \
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_DEV_INDEX_LOWER_MEMORY_FREE_NO_ASSERT(dev_id,val,min,ptr)\
    do{\
        if((val) < (min))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x] !\n", __FILE__, __LINE__, val, min);\
            ZXIC_COMM_FREE(ptr);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)

        
#define ZXIC_COMM_CHECK_DEV_INDEX_BOTH_NO_ASSERT(dev_id,val,min,max)\
    do{\
        if(((val) < (min)) || ((val) > (max)))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_DEV_INDEX_BOTH_NO_ASSERT_UNLOCK(dev_id, val, min, max, mutex)\
    do{\
        if(((val) < (min)) || ((val) > (max)))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            if(0 != zxic_comm_mutex_unlock(mutex))\
            {   \
                ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
            }   \
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_DEV_INDEX_BOTH_MEMORY_FREE_NO_ASSERT(dev_id,val,min,max,ptr)\
    do{\
        if(((val) < (min)) || ((val) > (max)))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_FREE(ptr);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_DEV_INDEX_RETURN_NULL_NO_ASSERT(dev_id,val,min,max)\
    do{\
        if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_index_check(val, min, max))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            return ZXIC_NULL;\
        }\
        else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_index_check(val, min, max))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            return ZXIC_NULL;\
        }\
    }while(0)


#define ZXIC_COMM_CHECK_DEV_INDEX_MEMORY_FREE_NO_ASSERT(dev_id,val,min,max,ptr)\
    do{\
        if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_dev_index_check(dev_id, val, min, max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_FREE(ptr);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
        else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_dev_index_check(dev_id, val, min, max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_FREE(ptr);\
            return ZXIC_PAR_CHK_INVALID_RANGE;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_DEV_INDEX_MEMORY_FREE2PTR_NO_ASSERT(dev_id, val, min, max, ptr1, ptr2)\
    do{\
        if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_dev_index_check(dev_id, val, min, max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_FREE(ptr1);\
            ZXIC_COMM_FREE(ptr2);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
        else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_dev_index_check(dev_id, val, min, max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_FREE(ptr1);\
            ZXIC_COMM_FREE(ptr2);\
            return ZXIC_PAR_CHK_INVALID_RANGE;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_DEV_INDEX_MEMORY_FREE3PTR_NO_ASSERT(dev_id, val, min, max, ptr1, ptr2, ptr3)\
    do{\
        if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_dev_index_check(dev_id, val, min, max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_FREE(ptr1);\
            ZXIC_COMM_FREE(ptr2);\
            ZXIC_COMM_FREE(ptr3);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
        else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_dev_index_check(dev_id, val, min, max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_FREE(ptr1);\
            ZXIC_COMM_FREE(ptr2);\
            ZXIC_COMM_FREE(ptr3);\
            return ZXIC_PAR_CHK_INVALID_RANGE;\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_DEV_ID_NO_ASSERT(dev_id)\
    do{\
        if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_index_check(dev_id, 0, zxic_comm_channel_max_get() - 1))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, dev_id, 0, zxic_comm_channel_max_get() - 1);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
        else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_index_check(dev_id, 0, zxic_comm_channel_max_get() - 1))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, dev_id, 0, zxic_comm_channel_max_get() - 1);\
            return ZXIC_PAR_CHK_INVALID_RANGE;\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_DEV_ID_RETURN_NULL_NO_ASSERT(dev_id)\
    do{\
        if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_index_check(dev_id, 0, zxic_comm_channel_max_get() - 1))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, dev_id, 0, zxic_comm_channel_max_get() - 1);\
            return ZXIC_NULL;\
        }\
        else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_index_check(dev_id, 0, zxic_comm_channel_max_get() - 1))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, dev_id, 0, zxic_comm_channel_max_get() - 1);\
            return ZXIC_NULL;\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_DEV_INDEX_SUB_OVERFLOW_NO_ASSERT(dev_id, val0, val1)\
    do{\
        if((val0) < (val1))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)  

#define ZXIC_COMM_CHECK_DEV_INDEX_SUB_OVERFLOW_NO_ASSERT_UNLOCK(dev_id, val0, val1, mutex)\
    do{\
        if((val0) < (val1))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            if(0 != zxic_comm_mutex_unlock(mutex))\
            {\
                ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
            }\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)  

#define ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id,val0,val1)\
    do{\
        if((ZXIC_UINT32_MAX - (val0)) < (val1))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT_UNLOCK(dev_id,val0,val1,mutex)\
    do{\
        if((ZXIC_UINT32_MAX - (val0)) < (val1))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            if(0 != zxic_comm_mutex_unlock(mutex))\
            {\
                ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
            }\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_64_NO_ASSERT(dev_id,val0,val1)\
    do{\
        if((ZXIC_ULONG_MAX - (val0)) < (val1))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[val0=0x%016llx] INVALID] [val1=0x%016llx] !\n", __FILE__, __LINE__, val0, val1);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id,val0,val1)\
    do{\
        if(((val0) > 0) && ((ZXIC_UINT32_MAX / (val0)) < (val1)))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)  

#define ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT_UNLOCK(dev_id,val0,val1,mutex)\
    do{\
        if(((val0) > 0) && ((ZXIC_UINT32_MAX / (val0)) < (val1)))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            if(0 != zxic_comm_mutex_unlock(mutex))\
            {\
                ZXIC_COMM_TRACE_ERROR("File: [%s],Function:[%s],Line:%u mutex unlock failed!-->Return ERROR\n", __FILE__, __FUNCTION__, __LINE__);\
            }\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_64_NO_ASSERT(dev_id,val0,val1)\
    do{\
        if(((val0) > 0) && ((ZXIC_ULONG_MAX/ (val0)) < (val1)))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            return ZXIC_PAR_CHK_INVALID_INDEX;\
        }\
    }while(0)  

#endif
#if ZXIC_REAL("return no code")
#define ZXIC_COMM_CHECK_RC_RETURN_NONE(rc,becall)\
    do{\
        if(ZXIC_OK != rc)\
        {\
           if(ZXIC_OK != zxic_comm_errcode_check(rc))\
           {\
               ZXIC_COMM_TRACE_ERROR("\n ZXIC  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,becall);\
           }\
           ZXIC_COMM_ASSERT(0);\
           return;\
        }\
    }while(0)

/* 不带返回值,用于VOID类型函数 */
#define ZXIC_COMM_CHECK_POINT_RETURN_NONE(point)\
    do{\
       if(ZXIC_NULL == (point))\
       {\
          ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
          ZXIC_COMM_ASSERT(0);\
          return;\
       }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_RETURN_NONE(val0,val1)\
    do{\
        if((ZXIC_UINT32_MAX - (val0)) < (val1))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            ZXIC_COMM_ASSERT(0);\
            return;\
        }\
    }while(0)  
#define ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW_RETURN_NONE(val0,val1)\
    do{\
        if((val0) < (val1))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            ZXIC_COMM_ASSERT(0);\
            return;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_MUL_OVERFLOW_RETURN_NONE(val0,val1)\
    do{\
        if(((val0) > 0) && ((ZXIC_UINT32_MAX / (val0)) < (val1)))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            ZXIC_COMM_ASSERT(0);\
            return;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_UPPER_RETURN_NONE(val,max)\
    do{\
        if((val) > (max))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [max=0x%x] !\n", __FILE__, __LINE__, val, max);\
            ZXIC_COMM_ASSERT(0);\
            return;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_LOWER_RETURN_NONE(val,min)\
    do{\
        if((val) < (min))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x] !\n", __FILE__, __LINE__, val, min);\
            ZXIC_COMM_ASSERT(0);\
            return;\
        }\
    }while(0)    
#define ZXIC_COMM_CHECK_INDEX_BOTH_RETURN_NONE(val,min,max)\
    do{\
        if(((val) < (min)) || ((val) > (max)))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_ASSERT(0);\
            return;\
        }\
    }while(0)              
#define ZXIC_COMM_CHECK_INDEX_UPPER_MEMORY_FREE_RETURN_NONE(val,max,ptr)\
    do{\
        if((val) > (max))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [max=0x%x] !\n", __FILE__, __LINE__, val, max);\
            ZXIC_COMM_FREE(ptr);\
            ZXIC_COMM_ASSERT(0);\
            return;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_LOWER_MEMORY_FREE_RETURN_NONE(val,min,ptr)\
    do{\
        if((val) < (min))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x] !\n", __FILE__, __LINE__, val, min);\
            ZXIC_COMM_FREE(ptr);\
            ZXIC_COMM_ASSERT(0);\
            return;\
        }\
    }while(0)    
#define ZXIC_COMM_CHECK_INDEX_BOTH_MEMORY_FREE_RETURN_NONE(val,min,max,ptr)\
    do{\
        if(((val) < (min)) || ((val) > (max)))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_FREE(ptr);\
            ZXIC_COMM_ASSERT(0);\
            return;\
        }\
    }while(0) 

#define ZXIC_COMM_CHECK_DEV_RC_RETURN_NONE(dev_id, rc, becall)\
    do{\
        if(ZXIC_OK != rc)\
        {\
           if(ZXIC_OK != zxic_comm_errcode_check(rc))\
           {\
               ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ZXIC  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,becall);\
           }\
           ZXIC_COMM_ASSERT(0);\
            return;\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_DEV_POINT_RETURN_NONE(dev_id,point)\
  do{\
       if(NULL == (point))\
       {\
          ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
          ZXIC_COMM_ASSERT(0);\
          return;\
       }\
   }while(0)

#define ZXIC_COMM_CHECK_DEV_INDEX_UPPER_RETURN_NONE(dev_id,val,max)\
    do{\
        if((val) > (max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [max=0x%x] !\n", __FILE__, __LINE__, val, max);\
            ZXIC_COMM_ASSERT(0);\
            return;\
        }\
    }while(0)      
#define ZXIC_COMM_CHECK_DEV_INDEX_LOWER_RETURN_NONE(dev_id,val,min)\
    do{\
        if((val) < (min))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x] !\n", __FILE__, __LINE__, val, min);\
            ZXIC_COMM_ASSERT(0);\
            return;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_DEV_INDEX_BOTH_RETURN_NONE(dev_id,val,min,max)\
    do{\
        if(((val) < (min)) || ((val) > (max)))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_ASSERT(0);\
            return;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_DEV_INDEX_UPPER_MEMORY_FREE_RETURN_NONE(dev_id,val,max,ptr)\
    do{\
        if((val) > (max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [max=0x%x] !\n", __FILE__, __LINE__, val, max);\
            ZXIC_COMM_FREE(ptr);\
            ZXIC_COMM_ASSERT(0);\
            return;\
        }\
    }while(0)      
#define ZXIC_COMM_CHECK_DEV_INDEX_LOWER_MEMORY_FREE_RETURN_NONE(dev_id,val,min,ptr)\
    do{\
        if((val) < (min))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x] !\n", __FILE__, __LINE__, val, min);\
            ZXIC_COMM_FREE(ptr);\
            ZXIC_COMM_ASSERT(0);\
            return;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_DEV_INDEX_BOTH_MEMORY_FREE_RETURN_NONE(dev_id,val,min,max,ptr)\
    do{\
        if(((val) < (min)) || ((val) > (max)))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_FREE(ptr);\
            ZXIC_COMM_ASSERT(0);\
            return;\
        }\
    }while(0)    
#define ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_RETURN_NONE(dev_id,val0,val1)\
    do{\
        if((ZXIC_UINT32_MAX - (val0)) < (val1))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            ZXIC_COMM_ASSERT(0);\
            return;\
        }\
    }while(0)
        
#define ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_64_RETURN_NONE(dev_id,val0,val1)\
    do{\
        if((ZXIC_ULONG_MAX - (val0)) < (val1))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[val0=0x%016llx] INVALID] [val1=0x%016llx] !\n", __FILE__, __LINE__, val0, val1);\
            ZXIC_COMM_ASSERT(0);\
            return;\
        }\
    }while(0)
        
#define ZXIC_COMM_CHECK_DEV_INDEX_SUB_OVERFLOW_RETURN_NONE(dev_id, val0, val1)\
    do{\
        if((val0) < (val1))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            ZXIC_COMM_ASSERT(0);\
            return;\
        }\
    }while(0)  
        
#define ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_RETURN_NONE(dev_id,val0,val1)\
    do{\
        if(((val0) > 0) && ((ZXIC_UINT32_MAX / (val0)) < (val1)))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            ZXIC_COMM_ASSERT(0);\
            return;\
        }\
    }while(0)     
#endif

#if ZXIC_REAL("return no code & no assert")
#define ZXIC_COMM_CHECK_RC_RETURN_NONE_NO_ASSERT(rc,becall)\
    do{\
        if(ZXIC_OK != rc)\
        {\
           if(ZXIC_OK != zxic_comm_errcode_check(rc))\
           {\
               ZXIC_COMM_TRACE_ERROR("\n ZXIC  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,becall);\
           }\
           return;\
        }\
    }while(0)

/* 不带返回值,用于VOID类型函数 */
#define ZXIC_COMM_CHECK_POINT_RETURN_NONE_NO_ASSERT(point)\
    do{\
       if(ZXIC_NULL == (point))\
       {\
          ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
          return;\
       }\
    }while(0) 
#define ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_RETURN_NONE_NO_ASSERT(val0,val1)\
    do{\
        if((ZXIC_UINT32_MAX - (val0)) < (val1))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            return;\
        }\
    }while(0)  
#define ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW_RETURN_NONE_NO_ASSERT(val0,val1)\
    do{\
        if((val0) < (val1))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            return;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_MUL_OVERFLOW_RETURN_NONE_NO_ASSERT(val0,val1)\
    do{\
        if(((val0) > 0) && ((ZXIC_UINT32_MAX / (val0)) < (val1)))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            return;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_UPPER_RETURN_NONE_NO_ASSERT(val,max)\
    do{\
        if((val) > (max))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [max=0x%x] !\n", __FILE__, __LINE__, val, max);\
            return;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_LOWER_RETURN_NONE_NO_ASSERT(val,min)\
    do{\
        if((val) < (min))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x] !\n", __FILE__, __LINE__, val, min);\
            return;\
        }\
    }while(0)    
#define ZXIC_COMM_CHECK_INDEX_BOTH_RETURN_NONE_NO_ASSERT(val,min,max)\
    do{\
        if(((val) < (min)) || ((val) > (max)))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            return;\
        }\
    }while(0)              
#define ZXIC_COMM_CHECK_INDEX_UPPER_MEMORY_FREE_RETURN_NONE_NO_ASSERT(val,max,ptr)\
    do{\
        if((val) > (max))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [max=0x%x] !\n", __FILE__, __LINE__, val, max);\
            ZXIC_COMM_FREE(ptr);\
            return;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_LOWER_MEMORY_FREE_RETURN_NONE_NO_ASSERT(val,min,ptr)\
    do{\
        if((val) < (min))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x] !\n", __FILE__, __LINE__, val, min);\
            ZXIC_COMM_FREE(ptr);\
            return;\
        }\
    }while(0)    
#define ZXIC_COMM_CHECK_INDEX_BOTH_MEMORY_FREE_RETURN_NONE_NO_ASSERT(val,min,max,ptr)\
    do{\
        if(((val) < (min)) || ((val) > (max)))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_FREE(ptr);\
            return;\
        }\
    }while(0) 

#define ZXIC_COMM_CHECK_DEV_RC_RETURN_NONE_NO_ASSERT(dev_id, rc, becall)\
    do{\
        if(ZXIC_OK != rc)\
        {\
            if(ZXIC_OK != zxic_comm_errcode_check(rc))\
            {\
               ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ZXIC  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,becall);\
            }\
            return;\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_DEV_POINT_RETURN_NONE_NO_ASSERT(dev_id,point)\
  do{\
       if(NULL == (point))\
       {\
          ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
          return;\
       }\
   }while(0)

#define ZXIC_COMM_CHECK_DEV_INDEX_UPPER_RETURN_NONE_NO_ASSERT(dev_id,val,max)\
    do{\
        if((val) > (max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [max=0x%x] !\n", __FILE__, __LINE__, val, max);\
            return;\
        }\
    }while(0)      
#define ZXIC_COMM_CHECK_DEV_INDEX_LOWER_RETURN_NONE_NO_ASSERT(dev_id,val,min)\
    do{\
        if((val) < (min))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x] !\n", __FILE__, __LINE__, val, min);\
            return;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_DEV_INDEX_BOTH_RETURN_NONE_NO_ASSERT(dev_id,val,min,max)\
    do{\
        if(((val) < (min)) || ((val) > (max)))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            return;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_DEV_INDEX_UPPER_MEMORY_FREE_RETURN_NONE_NO_ASSERT(dev_id,val,max,ptr)\
    do{\
        if((val) > (max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [max=0x%x] !\n", __FILE__, __LINE__, val, max);\
            ZXIC_COMM_FREE(ptr);\
            return;\
        }\
    }while(0)      
#define ZXIC_COMM_CHECK_DEV_INDEX_LOWER_MEMORY_FREE_RETURN_NONE_NO_ASSERT(dev_id,val,min,ptr)\
    do{\
        if((val) < (min))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x] !\n", __FILE__, __LINE__, val, min);\
            ZXIC_COMM_FREE(ptr);\
            return;\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_DEV_INDEX_BOTH_MEMORY_FREE_RETURN_NONE_NO_ASSERT(dev_id,val,min,max,ptr)\
    do{\
        if(((val) < (min)) || ((val) > (max)))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
            ZXIC_COMM_FREE(ptr);\
            return;\
        }\
    }while(0)    
#define ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_RETURN_NONE_NO_ASSERT(dev_id,val0,val1)\
    do{\
        if((ZXIC_UINT32_MAX - (val0)) < (val1))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            return;\
        }\
    }while(0)
        
#define ZXIC_COMM_CHECK_DEV_INDEX_SUB_OVERFLOW_RETURN_NONE_NO_ASSERT(dev_id, val0, val1)\
    do{\
        if((val0) < (val1))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            return;\
        }\
    }while(0)  
        
#define ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_RETURN_NONE_NO_ASSERT(dev_id,val0,val1)\
    do{\
        if(((val0) > 0) && ((ZXIC_UINT32_MAX / (val0)) < (val1)))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            return;\
        }\
    }while(0)     
#endif

#if ZXIC_REAL("no return")
#define ZXIC_COMM_CHECK_RC_NONE(rc,becall)\
    do{\
        if(ZXIC_OK != rc)\
        {\
           if(ZXIC_OK != zxic_comm_errcode_check(rc))\
           {\
               ZXIC_COMM_TRACE_ERROR("\n ZXIC  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,becall);\
           }\
           ZXIC_COMM_ASSERT(0);\
        }\
    }while(0)

/* 不带返回值,用于VOID类型函数 */
#define ZXIC_COMM_CHECK_POINT_NONE(point)\
    do{\
       if(ZXIC_NULL == (point))\
       {\
          ZXIC_COMM_TRACE_ERROR("\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
          ZXIC_COMM_ASSERT(0);\
       }\
    }while(0)

/* 不带返回值,用于VOID类型函数 */
#define ZXIC_COMM_CHECK_INDEX_NONE(val,min,max)\
    do{\
        if (ZXIC_OK != zxic_comm_index_check(val, min, max));\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_UPPER_NONE(val,max)\
    do{\
        if((val) > (max))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [max=0x%x] !\n", __FILE__, __LINE__, val, max);\
        }\
    }while(0)  
#define ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_NONE(val0,val1)\
    do{\
        if((ZXIC_UINT32_MAX - (val0)) < (val1))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            ZXIC_COMM_ASSERT(0);\
        }\
    }while(0)  
#define ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW_NONE(val0,val1)\
    do{\
        if((val0) < (val1))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            ZXIC_COMM_ASSERT(0);\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_MUL_OVERFLOW_NONE(val0,val1)\
    do{\
        if(((val0) > 0) && ((ZXIC_UINT32_MAX / (val0)) < (val1)))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, val0, val1);\
            ZXIC_COMM_ASSERT(0);\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_LOWER_NONE(val,min)\
    do{\
        if((val) < (min))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x] !\n", __FILE__, __LINE__, val, min);\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_INDEX_BOTH_NONE(val,min,max)\
    do{\
        if(((val) < (min)) || ((val) > (max)))\
        {\
            ZXIC_COMM_TRACE_ERROR("\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
        }\
    }while(0)              

#define ZXIC_COMM_CHECK_DEV_RC_NONE(dev_id, rc, becall)\
    do{\
        if(ZXIC_OK != rc)\
        {\
           if(ZXIC_OK != zxic_comm_errcode_check(rc))\
           {\
               ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ZXIC  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,becall);\
           }\
           ZXIC_COMM_ASSERT(0);\
        }\
    }while(0)

#define ZXIC_COMM_CHECK_DEV_POINT_NONE(dev_id,point)\
  do{\
       if(NULL == (point))\
       {\
          ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ZXIC %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);\
          ZXIC_COMM_ASSERT(0);\
       }\
   }while(0)

#define ZXIC_COMM_CHECK_DEV_INDEX_NONE(dev_id,val,min,max)\
    do{\
        if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_dev_index_check(dev_id, val, min, max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
        }\
        else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_dev_index_check(dev_id, val, min, max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
        }\
     }while(0)
#define ZXIC_COMM_CHECK_DEV_INDEX_UPPER_NONE(dev_id,val,max)\
    do{\
        if((val) > (max))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [max=0x%x] !\n", __FILE__, __LINE__, val, max);\
        }\
    }while(0)      
#define ZXIC_COMM_CHECK_DEV_INDEX_LOWER_NONE(dev_id,val,min)\
    do{\
        if((val) < (min))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x] !\n", __FILE__, __LINE__, val, min);\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_DEV_INDEX_BOTH_NONE(dev_id,val,min,max)\
    do{\
        if(((val) < (min)) || ((val) > (max)))\
        {\
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "\n %s:%d[Error:VALUE[0x%x] INVALID] [min=0x%x,max=0x%x] !\n", __FILE__, __LINE__, val, min, max);\
        }\
    }while(0)
#define ZXIC_COMM_CHECK_DEV_RC_INT(dev_id,check_rc,rc,becall)\
   do{\
       if(check_rc < 0)\
       {\
          ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ICM  %s:%d [ErrorCode:0x%x] [rc:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,check_rc, rc,__FUNCTION__,becall);\
          ZXIC_COMM_ASSERT(0);\
          return rc;\
       }\
   }while(0)

#endif
#if ZXIC_REAL("no print")
#define ZXIC_COMM_CHECK_RC_NO_PRINT(rc,error_code)\
do{\
    if(ZXIC_OK != rc)\
    {\
       ZXIC_COMM_ASSERT(0);\
       return error_code;\
    }\
}while(0)
#define ZXIC_COMM_CHECK_RC_UNLOCK_NO_PRINT(rc, p_mutex, error_code)\
do{\
    if(ZXIC_OK != rc)\
    {\
       (ZXIC_VOID)zxic_comm_mutex_unlock(p_mutex);\
       ZXIC_COMM_ASSERT(0);\
       return error_code;\
    }\
}while(0)

#define ZXIC_COMM_CHECK_POINT_NO_PRINT(point)\
do{\
   if(ZXIC_NULL == (point))\
   {\
      ZXIC_COMM_ASSERT(0);\
      return ZXIC_PAR_CHK_POINT_NULL;\
   }\
}while(0)
#define ZXIC_COMM_CHECK_POINT_NO_PRINT_UNLOCK(point,p_mutex)\
do{\
   if(ZXIC_NULL == (point))\
   {\
      (ZXIC_VOID)zxic_comm_mutex_unlock(p_mutex);\
      ZXIC_COMM_ASSERT(0);\
      return ZXIC_PAR_CHK_POINT_NULL;\
   }\
}while(0)

#define ZXIC_COMM_CHECK_RC_POINT_NO_PRINT(point,rc)\
   do{\
      if(ZXIC_NULL == (point))\
      {\
         ZXIC_COMM_ASSERT(0);\
         return rc;\
      }\
   }while(0)

#define ZXIC_COMM_CHECK_INDEX_NO_PRINT_UNLOCK(val,min,max,p_mutex)\
do{\
    if(ZXIC_PAR_CHK_INVALID_INDEX == zxic_comm_index_check(val, min, max))\
    {\
        (ZXIC_VOID)zxic_comm_mutex_unlock(p_mutex);\
        ZXIC_COMM_ASSERT(0);\
        return ZXIC_PAR_CHK_INVALID_INDEX;\
    }\
    else if(ZXIC_PAR_CHK_INVALID_RANGE == zxic_comm_index_check(val, min, max))\
    {\
        (ZXIC_VOID)zxic_comm_mutex_unlock(p_mutex);\
        ZXIC_COMM_ASSERT(0);\
        return ZXIC_PAR_CHK_INVALID_RANGE;\
    }\
}while(0)
#define ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW_NO_PRINT_UNLOCK(val0, val1, mutex)\
 do{\
    if((val0) < (val1))\
    {\
        if(0 != zxic_comm_mutex_unlock(mutex))\
        {\
            ZXIC_COMM_ASSERT(0);\
            return ZXIC_PAR_CHK_INVALID_PARA;\
        }\
        ZXIC_COMM_ASSERT(0);\
        return ZXIC_PAR_CHK_INVALID_INDEX;\
    }\
 }while(0)

#endif

#endif /* ZXIC_REAL("参数检查") */


//#ifdef ZXIC_FOR_LLT
#if ZXIC_REAL("UT_TEST")
#define ZXIC_CHECK_DEV_UT_RC(dev_id, rc, val, becall)\
    do{\
        if (val != rc)\
        {\
           ZXIC_COMM_PRINT("\n ZXICP %s:%d [ErrorCode:0x%x], %s Call %s Fail!\n", __FILE__, __LINE__, rc, __FUNCTION__, becall);\
           zxic_comm_ut_detail_trace_dev_error(dev_id, "\n ZXICP %s:%d [ErrorCode:0x%x], %s Call %s Fail!\n", __FILE__, __LINE__, rc, __FUNCTION__, becall);\
           return ZXIC_E_LLT_CHECK;\
        }\
    } while (0)
#endif

#if ZXIC_REAL("字节序")
/* DWORD变量字节序转换 */
#define ZXIC_COMM_CONVERT32(dw_data) \
                                        ((((dw_data)&0xff)<<24)|(((dw_data)&0xff00)<<8)\
                                        |(((dw_data)&0xff0000)>>8)|(((dw_data)&0xff000000)>>24))
/* WORD变量字节序转换 */
#define ZXIC_COMM_CONVERT16(w_data) \
                                        ((((w_data)&0xff)<<8)|(((w_data)&0xff00)>>8))

/* WORD变量字节序转换 */
#define ZXIC_COMM_CONVERT32_16b(w_data) \
                                        ((((w_data)&0xffff)<<16)|(((w_data)&0xffff0000)>>16))

ZXIC_VOID  zxic_comm_swap_en_set(ZXIC_UINT32 enable);
ZXIC_RTN32 zxic_comm_swap_en_get(ZXIC_VOID);
ZXIC_RTN32 zxic_comm_is_big_endian(ZXIC_VOID);
ZXIC_RTN32 zxic_comm_endian_prt(ZXIC_VOID);
ZXIC_VOID  zxic_comm_swap(ZXIC_UINT8 *p_uc_data, ZXIC_UINT32 dw_byte_len);
ZXIC_VOID  zxic_comm_swap_16b(ZXIC_UINT8 *p_uc_data, ZXIC_UINT32 dw_byte_len);
ZXIC_UINT64 ZXIC_COMM_COUNTER64_BUILD(ZXIC_UINT32 hi, ZXIC_UINT32 lo);
#endif

#if ZXIC_REAL("内存")
ZXIC_UINT32 zxic_comm_get_malloc_num(ZXIC_VOID);
ZXIC_UINT32 zxic_comm_get_malloc_size(ZXIC_VOID);
ZXIC_VOID   zxic_clr_malloc_num(ZXIC_VOID);

#define ZXIC_COMM_FREE(p_data) \
do{\
    if (p_data != NULL)\
    {\
        ic_comm_free_record();\
        kfree(p_data);\
        p_data = ZXIC_NULL; \
    }\
}while(0)

#define ZXIC_COMM_VFREE(p_data) \
do{\
    if (p_data != NULL)\
    {\
        ic_comm_vfree_record();\
        vfree(p_data);\
        p_data = ZXIC_NULL; \
    }\
}while(0)

#define ZXIC_COMM_MALLOC(size)  ic_comm_malloc_memory(size)

#define ZXIC_COMM_VMALLOC(size)  ic_comm_vmalloc_memory(size)

#define ZXIC_COMM_ALLOC_MEMORY(ptr,size) \
    do{\
        (ptr) = ZXIC_COMM_MALLOC(size);\
        ZXIC_COMM_CHECK_POINT(ptr);\
        ZXIC_COMM_MEMSET((ptr),0,size);\
    }while(0)
#define ZXIC_COMM_ALLOC_MEMORY_DEV(dev_id,ptr,size) \
    do{\
        (ptr) = ZXIC_COMM_MALLOC(size);\
        ZXIC_COMM_CHECK_DEV_POINT(dev_id,ptr);\
        ZXIC_COMM_MEMSET((ptr),0,size);\
    }while(0)


#endif

#if ZXIC_REAL("延时")
// ZXIC_VOID zxic_comm_sleep(ZXIC_UINT32 milliseconds);
ZXIC_VOID zxic_comm_msleep(ZXIC_UINT32 millisecond);
ZXIC_VOID zxic_comm_udelay(ZXIC_UINT32 microseconds);
ZXIC_VOID zxic_comm_delay(ZXIC_UINT32 milliseconds);
ZXIC_DOUBLE zxic_comm_get_ticks_s(ZXIC_VOID);
ZXIC_DOUBLE zxic_comm_get_ticks_ms(ZXIC_VOID);
ZXIC_DOUBLE zxic_get_ticks_uses(ZXIC_VOID);
#endif

#if ZXIC_REAL("bit操作")

/* ZXIC_UINT32 写某几bit的值 */
#define ZXIC_COMM_MASK_BIT(intType,_bitNum_)\
    (( intType )( 0x1U << (_bitNum_) ))

#define ZXIC_COMM_GET_BIT_MASK( _intType_, _bitQnt_ )\
    ( (_intType_)( ( (_bitQnt_) < 32 ) \
      ? ( (_intType_) ZXIC_COMM_MASK_BIT( _intType_ , ( (_bitQnt_) & 0x1F ) ) - 1 ) \
      : ((_intType_)(0xffffffff)) ) )

#define ZXIC_COMM_UINT32_WRITE_BITS( _uiDst_, _uiSrc_, _uiStartPos_, _uiLen_ )\
do{\
    ( _uiDst_ ) = ( ( _uiDst_ ) & ~( ZXIC_COMM_GET_BIT_MASK( ZXIC_UINT32 , ( _uiLen_ ) ) << ( _uiStartPos_ ) ) ) |\
                ( ( ( _uiSrc_ ) & ZXIC_COMM_GET_BIT_MASK( ZXIC_UINT32 , ( _uiLen_ ) ) ) << ( _uiStartPos_ ) );\
}while(0)

#define ZXIC_COMM_UINT32_WRITE_BITS_ZERO(_uiDst_, _uiStartPos_, _uiLen_ )\
do{\
    ( _uiDst_ ) = ( ( _uiDst_ ) & ~( ZXIC_COMM_GET_BIT_MASK( ZXIC_UINT32 , ( _uiLen_ ) ) << ( _uiStartPos_ ) ) );\
}while(0)

#define ZXIC_COMM_UINT32_GET_BITS(_uiDst_ ,_uiSrc_, _uiStartPos_, _uiLen_)\
do{\
    ( _uiDst_ ) =( ( (_uiSrc_) >> (_uiStartPos_) ) & \
                ( ZXIC_COMM_GET_BIT_MASK( ZXIC_UINT32 , (_uiLen_) ) ) );\
  }while(0)

#define ZXIC_COMM_UINT32_GET_RETURN_BITS(_uiSrc_, _uiStartPos_, _uiLen_)\
    ( ( (_uiSrc_) >> (_uiStartPos_) ) & \
                ( ZXIC_COMM_GET_BIT_MASK( ZXIC_UINT32 , (_uiLen_) ) ) )\

/* ZXIC_UINT64 写某几bit的值 */
#define ZXIC_COMM_MASK_BIT_64(intType,_bitNum_)\
    (( intType )( 0x1ULL << (_bitNum_) ))

#define ZXIC_COMM_GET_64_BIT_MASK( _intType_, _bitQnt_ )\
    ( (_intType_)( ( (_bitQnt_) < 64 ) \
      ? ( (_intType_) ZXIC_COMM_MASK_BIT_64( _intType_ , ( (_bitQnt_) & 0x3F ) ) - 1 ) \
      : ((_intType_)(0xFFFFFFFFFFFFFFFFULL)) ) )

#define ZXIC_COMM_UINT64_WRITE_BITS( _uiDst_, _uiSrc_, _uiStartPos_, _uiLen_ )\
do{\
    ( _uiDst_ ) = ( ( _uiDst_ ) & ~( ZXIC_COMM_GET_64_BIT_MASK( ZXIC_UINT64 , ( _uiLen_ ) ) << ( _uiStartPos_ ) ) ) |\
                ( ( ( _uiSrc_ ) & ZXIC_COMM_GET_64_BIT_MASK( ZXIC_UINT64 , ( _uiLen_ ) ) ) << ( _uiStartPos_ ) );\
}while(0)

/* Base type for declarations */
#define ZXIC_COMM_BITDCL        ZXIC_UINT32
#define ZXIC_COMM_BITWID        (32)

/* (internal) Number of ZXICP_BITDCLs needed to contain _max bits */
#define NPE_BITDCLSIZE(_max)    (((_max) + ZXIC_COMM_BITWID - 1) / ZXIC_COMM_BITWID)

/* Size for giving to malloc and memset to handle _max bits */
#define ZXIC_COMM_BITALLOCSIZE(_max) (NPE_BITDCLSIZE(_max) * sizeof (ZXIC_COMM_BITDCL))


/* (internal) Generic operation macro on bit array _a, with bit _b */
#define NPE_BITOP(_a, _b, _op)  (((_a)[(_b) / ZXIC_COMM_BITWID]) _op (1U << ((_b) % ZXIC_COMM_BITWID)))

/* Specific operations */
#define ZXIC_COMM_BITGET(_a, _b)    NPE_BITOP(_a, _b, &)
#define ZXIC_COMM_BITSET(_a, _b)    NPE_BITOP(_a, _b, |=)
#define ZXIC_COMM_BITCLR(_a, _b)    NPE_BITOP(_a, _b, &= ~)


ZXIC_RTN32 zxic_comm_bit_count(ZXIC_UINT32 data);
ZXIC_RTN32 zxic_comm_read_bits(ZXIC_UINT8 * p_base, ZXIC_UINT32 base_size_bit, ZXIC_UINT32 * p_data, ZXIC_UINT32 start_bit, ZXIC_UINT32 end_bit);
ZXIC_RTN32 zxic_comm_write_bits(ZXIC_UINT8 * p_base, ZXIC_UINT32 base_size_bit, ZXIC_UINT32 data, ZXIC_UINT32 start_bit, ZXIC_UINT32 end_bit);
ZXIC_RTN32 zxic_comm_write_bits_ex(ZXIC_UINT8 * p_base, ZXIC_UINT32 base_size_bit, ZXIC_UINT32 data, ZXIC_UINT32 msb_start_pos, ZXIC_UINT32 len);
ZXIC_RTN32 zxic_comm_read_bits_ex(ZXIC_UINT8 * p_base, ZXIC_UINT32 base_size_bit, ZXIC_UINT32 * p_data, ZXIC_UINT32 msb_start_pos, ZXIC_UINT32 len);
ZXIC_RTN32 zxic_comm_write_bits_op(ZXIC_UINT8* p_src_dat, ZXIC_UINT32 src_size_bit, ZXIC_UINT32 input_data, ZXIC_UINT32 start_bit, ZXIC_UINT32 end_bit);
ZXIC_RTN32 zxic_comm_read_bits_op(ZXIC_UINT8*  p_src_dat, ZXIC_UINT32  src_size_bit, ZXIC_UINT32* p_out_data, ZXIC_UINT32  start_bit, ZXIC_UINT32  end_bit);
#endif

#if ZXIC_REAL("算数运算")
ZXIC_UINT64 zxic_comm_get_gcd (ZXIC_UINT64 a, ZXIC_UINT64 b);
ZXIC_RTN32 zxic_comm_multi_big_integer(ZXIC_CONST ZXIC_CHAR* num1,ZXIC_CONST ZXIC_CHAR* num2,ZXIC_CHAR* str_num);
ZXIC_RTN32 zxic_comm_div_big_integer(ZXIC_CONST ZXIC_CHAR *num1,ZXIC_CONST ZXIC_CHAR *num2,ZXIC_UINT32 *quo_val);
ZXIC_SINT32 zxic_comm_sub_stract(ZXIC_SINT32 *p1,ZXIC_SINT32 *p2,ZXIC_SINT32 len1,ZXIC_SINT32 len2 );
ZXIC_SINT32 zxic_comm_cmpm_calc(ZXIC_UINT64 cm_cal, ZXIC_UINT64 pm_cal, ZXIC_UINT32* cm, ZXIC_UINT32* pm);
ZXIC_VOID zxic_comm_pm_cm_cal(ZXIC_UINT64  cm_y, ZXIC_UINT64  pm_y, ZXIC_UINT32* cm, ZXIC_UINT32* pm);
#endif

#if ZXIC_REAL("字符串")
ZXIC_RTN32 zxic_comm_strcasecmp(ZXIC_CHAR *str1, ZXIC_CHAR* str2);
ZXIC_UINT8 zxic_comm_char_to_hex(ZXIC_UINT8 c);
ZXIC_DWORD zxic_comm_ipaddr_to_dword(ZXIC_CONST ZXIC_CHAR *p_addr);
ZXIC_RTN32 zxic_comm_char_to_number(ZXIC_CHAR a, ZXIC_CHAR b, ZXIC_UINT8 *number);
ZXIC_CHAR *zxic_comm_strlower(ZXIC_CHAR *str);
ZXIC_RTN32 ic_comm_check_str_size(ZXIC_CHAR* str);
ZXIC_SIZE_T ic_comm_getAbsValue(ZXIC_UINT8* dest, ZXIC_CONST ZXIC_UINT8* src);
ZXIC_VOID ic_comm_memset_s(void* dest, ZXIC_SIZE_T dmax, ZXIC_UINT8 c, ZXIC_SIZE_T n);
ZXIC_SINT32 ic_comm_memcmp(void* str1, const void* str2, ZXIC_SIZE_T n);
ZXIC_SINT32 ic_comm_strncmp(const ZXIC_CHAR* str1, const ZXIC_CHAR* str2, ZXIC_SIZE_T n);


#endif

#if ZXIC_REAL("dma内存分配")
#define ZXIC_DMA_PHY_ADDR dma_addr_t
ZXIC_RTN32 zxic_comm_dma_mem_malloc(ZXIC_ADDR_T *vir_addr,ZXIC_ADDR_T *phy_addr,ZXIC_UINT32 dma_size);
ZXIC_RTN32 zxic_comm_dma_mem_free(ZXIC_ADDR_T vir_addr,ZXIC_ADDR_T phy_addr,ZXIC_UINT32 dma_size);
#endif

#if ZXIC_REAL("OTHER")
#define MIN_VAL(x,y)                   ((x)<=(y) ? (x) : (y))
#define MAX_VAL(x,y)                   ((x)<=(y) ? (y) : (x))
#define ZXIC_COMM_DM_TO_X(d, m)        ((d) & ~(m))
#define ZXIC_COMM_DM_TO_Y(d, m)        (~(d) & ~(m))
#define ZXIC_COMM_XY_TO_MASK(x, y)     (~(x) & ~(y))
#define ZXIC_COMM_XY_TO_DATA(x, y)     (x)             /* valid only when mask is 0 */
#define ZXIC_RD_CNT_MAX                 (50)

/* 新增常用数据掩码 */
#define ZXIC_COMM_WORD64_MASK         (0xFFFFFFFFFFFFFFFFULL)
#define ZXIC_COMM_WORD32_MASK         (0xFFFFFFFFU)
#define ZXIC_COMM_WORD16_MASK         (0xFFFFU)
#define ZXIC_COMM_BYTE_MASK           (0xFFU)

ZXIC_RTN32 ZXIC_COMM_GET_MASK_VALUE(ZXIC_UINT32 total, ZXIC_UINT32 masklen);
ZXIC_UINT32 zxic_comm_random(ZXIC_VOID);
ZXIC_VOID zxic_comm_channel_max_set(ZXIC_UINT32 dev_max);

ZXIC_RTN32 zxic_comm_channel_max_get(ZXIC_VOID);

ZXIC_VOID zxic_comm_dbgcnt64_select_print(const ZXIC_CHAR * name, ZXIC_UINT64 value, ZXIC_UINT32 prt_mode);

ZXIC_VOID zxic_comm_dbgcnt64_select_par_print(const ZXIC_CHAR * name, ZXIC_UINT32 parm, ZXIC_UINT64 value, ZXIC_UINT32 prt_mode);

ZXIC_VOID zxic_comm_dbgcnt32_select_print(const ZXIC_CHAR * name, ZXIC_UINT32 value, ZXIC_UINT32 prt_mode);

ZXIC_VOID zxic_comm_dbgcnt32_select_par_print(const ZXIC_CHAR * name, ZXIC_UINT32 parm, ZXIC_UINT32 value, ZXIC_UINT32 prt_mode);

#define ZXIC_COMM_CHECK_ADD_CNT(x,y) (((0xffffffff - (x)) < (y)) ? ((y) - (0xffffffff - (x)) - 1) : ((x) + (y)))
#define ZXIC_COMM_CHECK_ADD_CNT_WORD64(x,y) (((0xffffffffffffffff - (x)) < (y)) ? ((y) - (0xffffffffffffffff - (x)) - 1) : ((x) + (y)))
#endif

#if ZXIC_REAL("UT_TEST")

ZXIC_VOID zxic_comm_ut_detail_info_trace(const ZXIC_CHAR *format, ...);

ZXIC_VOID zxic_comm_ut_result_info_trace(const ZXIC_CHAR *format, ...);

ZXIC_VOID zxic_comm_ut_detail_trace_error(const ZXIC_CHAR *format, ...);

ZXIC_VOID zxic_comm_ut_detail_trace_dev_error(ZXIC_UINT32 dev_id, const ZXIC_CHAR *format, ...);

#endif




#if ZXIC_REAL("头文件")
#include "zxic_comm_double_link.h"
#include "zxic_comm_doublelink_index.h"
#include "zxic_comm_liststack.h"
#include "zxic_comm_avl_tree.h"
#include "zxic_comm_rb_tree.h"
#include "zxic_comm_index_ctrl.h"
#include "zxic_comm_index_reserve.h"
#include "zxic_comm_index_fill.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* end __ZXIC_COMMON_H__ */


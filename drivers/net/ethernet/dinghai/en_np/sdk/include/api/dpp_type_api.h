/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_error.h
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 :
* 完成日期 : 2014/01/27
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/

#ifndef _DPP_TYPE_API_H_
#define _DPP_TYPE_API_H_

#ifdef __cplusplus
extern "C" {
#endif


//#define DPP_FOR_IDE      /* IDE仿真器Driver编译控制宏，通过VC工具导入，不允许手动更改此行 */
//#define DPP_FOR_LLT      /* DPP 打桩控制宏，通过Makefile编译脚本导入，不允许手动更改此行 "make ssp4_test_64 DPPLLT=1"*/


#ifndef DPP_FOR_LLT
/* write hardware enable */
#define     DPP_HW_OPR_EN       (1)
#else
/* write hardware disable */
#define     DPP_HW_OPR_EN       (1)
#endif

#ifndef DPP_FOR_LLT
    #ifndef DPP_FOR_RISCV
        #ifndef DPP_FOR_PCIE
        /* dev VPCI enalbe */
            #define     DPP_DEV_VPCI_EN     (1)
        #else
            #define     DPP_DEV_VPCI_EN     (0)
        #endif
    #else
        #define     DPP_DEV_VPCI_EN     (0)
    #endif
#else
    #define     DPP_DEV_VPCI_EN     (0)
#endif


/* CPU软复位调试时开启 */
#define     DPP_CPU_SOFT_RESET  (0)

#if defined(ZXIC_OS_WIN)
#define DPP_OS_WIN32
#elif defined(ZXIC_OS_LINUX)
#define DPP_OS_LINUX
#endif


#ifdef ZXIC_OS_WIN
#define SDT_FILE_PATH         "..\\tools\\midware\\table_info.txt"
#else
#define SDT_FILE_PATH         "table_info.txt"
#endif


#ifndef BOOL
#define BOOL   int
#endif

#ifndef BOOLEAN
#define BOOLEAN unsigned char
#endif

#ifndef DPP_STATUS
#define DPP_STATUS               ZXIC_UINT32
#endif

#ifndef DPP_OK
#define DPP_OK                   (0)
#endif

#ifndef DPP_ERR
#define DPP_ERR                  (1)
#endif


#define DPP_RD_CNT_MAX        (100)     /* 最大读完成状态次数 */

#define DPP_PRT_BIT_LEN_1     (1)
#define DPP_PRT_BIT_LEN_2     (2)
#define DPP_PRT_BIT_LEN_8     (8)
#define DPP_PRT_BIT_LEN_16    (16)
#define DPP_PRT_BIT_LEN_32    (32)


/* agent module error code */
#define DPP_RC_AGENT_BASE                (0x100)
#define DPP_RC_AGENT_INVALID_PARAMS      (DPP_RC_AGENT_BASE | 0x0)
#define DPP_RC_AGENT_INVALID_RANGES      (DPP_RC_AGENT_BASE | 0x1)
#define DPP_RC_AGENT_CALL_FUN_FAIL       (DPP_RC_AGENT_BASE | 0x2)
#define DPP_RC_AGENT_GET_POINTER_FAIL    (DPP_RC_AGENT_BASE | 0x3)
#define DPP_RC_AGENT_SEND_MSG_FAIL       (DPP_RC_AGENT_BASE | 0x4)
#define DPP_RC_AGENT_MSG_TYPE_INVALID    (DPP_RC_AGENT_BASE | 0x5)

/* common module error code */
#define DPP_RC_COMMON_BASE               (0x200)
#define DPP_RC_COMMON_INVALID_PARAMS     (DPP_RC_COMMON_BASE | 0x0)
#define DPP_RC_COMMON_INVALID_RANGES     (DPP_RC_COMMON_BASE | 0x1)
#define DPP_RC_COMMON_CALL_FUN_FAIL      (DPP_RC_COMMON_BASE | 0x2)
#define DPP_RC_COMMON_GET_POINTER_FAIL   (DPP_RC_COMMON_BASE | 0x3)
#define DPP_RC_COMMON_SEND_MSG_FAIL      (DPP_RC_COMMON_BASE | 0x4)
#define DPP_RC_COMMON_MSG_TYPE_INVALID   (DPP_RC_COMMON_BASE | 0x5)
#define DPP_RC_COMMON_MEMCPY_S_INVALID_PTR (DPP_RC_COMMON_BASE | 0x6)
#define DPP_RC_COMMON_MEMCPY_S_MEM_OVERLAP (DPP_RC_COMMON_BASE | 0x7)
#define DPP_RC_COMMON_CLOSE_FAIL         (DPP_RC_COMMON_BASE | 0x8)

/* config module error code */
#define DPP_RC_CONFIG_BASE               (0x300)
#define DPP_RC_CONFIG_PARA_INVALID       (DPP_RC_CONFIG_BASE | 0x0)
#define DPP_RC_CONFIG_MSG_TYPE_INVALID   (DPP_RC_CONFIG_BASE | 0x1)
#define DPP_RC_CONFIG_MEM_NO_INVALID     (DPP_RC_CONFIG_BASE | 0x2)

/* debug module error code */
#define DPP_RC_DBG_BASE                  (0x400)
#define DPP_RC_DBG_INVALID_PARAMS        (DPP_RC_DBG_BASE | 0x0)
#define DPP_RC_DBG_INVALID_RANGES        (DPP_RC_DBG_BASE | 0x1)
#define DPP_RC_DBG_CALL_FUN_FAIL         (DPP_RC_DBG_BASE | 0x2)
#define DPP_RC_DBG_GET_POINTER_FAIL      (DPP_RC_DBG_BASE | 0x3)
#define DPP_RC_DBG_SEND_MSG_FAIL         (DPP_RC_DBG_BASE | 0x4)
#define DPP_RC_DBG_MEM_NOT_ALLOC         (DPP_RC_DBG_BASE | 0x5)

/* download module error code */
#define DPP_RC_LOAD_BASE                 (0x500)
#define DPP_RC_LOAD_INVALID_PARAMS       (DPP_RC_LOAD_BASE | 0x0)
#define DPP_RC_LOAD_INVALID_RANGES       (DPP_RC_LOAD_BASE | 0x1)
#define DPP_RC_LOAD_CALL_FUN_FAIL        (DPP_RC_LOAD_BASE | 0x2)
#define DPP_RC_LOAD_GET_POINTER_FAIL     (DPP_RC_LOAD_BASE | 0x3)
#define DPP_RC_LOAD_MSG_SEND_FAIL        (DPP_RC_LOAD_BASE | 0x4)
#define DPP_RC_LOAD_MSG_TYPE_INVALID     (DPP_RC_LOAD_BASE | 0x5)
#define DPP_RC_LOAD_DEV_NOT_EXIST        (DPP_RC_LOAD_BASE | 0x6)

/* device module error code */
#define DPP_RC_DEV_BASE                  (0x600)
#define DPP_RC_DEV_PARA_INVALID          (DPP_RC_DEV_BASE | 0x0)
#define DPP_RC_DEV_RANGE_INVALID         (DPP_RC_DEV_BASE | 0x1)
#define DPP_RC_DEV_CALL_FUNC_FAIL        (DPP_RC_DEV_BASE | 0x2)
#define DPP_RC_DEV_TYPE_INVALID          (DPP_RC_DEV_BASE | 0x3)
#define DPP_RC_DEV_CONNECT_FAIL          (DPP_RC_DEV_BASE | 0x4)
#define DPP_RC_DEV_MSG_INVALID           (DPP_RC_DEV_BASE | 0x5)
#define DPP_RC_DEV_NOT_EXIST             (DPP_RC_DEV_BASE | 0x6)
#define DPP_RC_DEV_MGR_NOT_INIT          (DPP_RC_DEV_BASE | 0x7)
#define DPP_RC_DEV_CFG_NOT_INIT          (DPP_RC_DEV_BASE | 0x8)
#define DPP_RC_DEV_STAUS_NOT_ON          (DPP_RC_DEV_BASE | 0x9)
#define DPP_RC_DEV_DMA_MEM_ALLOC_FAIL    (DPP_RC_DEV_BASE | 0xa)
#define DPP_RC_DEV_DMA_MEM_GET_FAIL      (DPP_RC_DEV_BASE | 0xb)

/* env(xOS) module error code */
#define DPP_RC_ENV_BASE                  (0x700)
#define DPP_RC_ENV_PARA_INVALID          (DPP_RC_ENV_BASE | 0x0)
#define DPP_RC_ENV_RANGE_INVALID         (DPP_RC_ENV_BASE | 0x1)
#define DPP_RC_ENV_CALL_FUNC_FAIL        (DPP_RC_ENV_BASE | 0x2)
#define DPP_RC_ENV_SOCKET_FAIL           (DPP_RC_ENV_BASE | 0x3)
#define DPP_RC_ENV_SOCKET_FULL           (DPP_RC_ENV_BASE | 0x4)
#define DPP_RC_ENV_THREAD_FAIL           (DPP_RC_ENV_BASE | 0x5)
#define DPP_RC_ENV_NOT_INIT              (DPP_RC_ENV_BASE | 0x6)
#define DPP_RC_ENV_TABLE_FULL            (DPP_RC_ENV_BASE | 0x7)
#define DPP_RC_ENV_MUTEX_FAIL            (DPP_RC_ENV_BASE | 0x8)
#define DPP_RC_ENV_SOCKET_NOT_EXIST      (DPP_RC_ENV_BASE | 0x9)

/* table module error code */
#define DPP_RC_TABLE_BASE                (0x800)
#define DPP_RC_TABLE_PARA_INVALID        (DPP_RC_TABLE_BASE | 0x0)
#define DPP_RC_TABLE_RANGE_INVALID       (DPP_RC_TABLE_BASE | 0x1)
#define DPP_RC_TABLE_CALL_FUNC_FAIL      (DPP_RC_TABLE_BASE | 0x2)
#define DPP_RC_TABLE_SDT_MSG_INVALID     (DPP_RC_TABLE_BASE | 0x3)
#define DPP_RC_TABLE_SDT_MGR_INVALID     (DPP_RC_TABLE_BASE | 0x4)
#define DPP_RC_TABLE_IF_VALUE_FAIL       (DPP_RC_TABLE_BASE | 0x5)
#define DPP_RC_TABLE_SDT_NOT_EXIST       (DPP_RC_TABLE_BASE | 0x6)
#define DPP_RC_TABLE_STAT_ITEM_INVALID   (DPP_RC_TABLE_BASE | 0x7)


/* stat module error code */
#define DPP_RC_STAT_BASE                 (0x900)
#define DPP_RC_STAT_INIT_ERR             (DPP_RC_STAT_BASE | 0x1)
#define DPP_RC_STAT_TM_INIT_ERR          (DPP_RC_STAT_BASE | 0x2)
#define DPP_RC_STAT_TM_DIS_EN            (DPP_RC_STAT_BASE | 0x3)
#define DPP_RC_STAT_TM_TYPE_ERR          (DPP_RC_STAT_BASE | 0xe)   /* TM 计数类型错误*/
#define DPP_RC_STAT_TM_DEPTH_ERR         (DPP_RC_STAT_BASE | 0xf)   /* TM 计数深度错误*/

/* appl module error code */
#define DPP_RC_APPL_BASE                 (0xa00)
#define DPP_RC_APPL_PARA_INVALID         (DPP_RC_APPL_BASE | 0x0)
#define DPP_RC_APPL_RANGE_INVALID        (DPP_RC_APPL_BASE | 0x1)
#define DPP_RC_APPL_CALL_FUNC_FAIL       (DPP_RC_APPL_BASE | 0x2)

/* reg module error code */
#define DPP_RC_MODULE_BASE               (0xb00)
#define DPP_RC_MODULE_PARA_INVALID       (DPP_RC_MODULE_BASE | 0x0)
#define DPP_RC_MODULE_RANGE_INVALID      (DPP_RC_MODULE_BASE | 0x1)
#define DPP_RC_MODULE_CALL_FUNC_FAIL     (DPP_RC_MODULE_BASE | 0x2)
#define DPP_RC_MODULE_ENUM_TYPE_FAIL     (DPP_RC_MODULE_BASE | 0x3)
#define DPP_RC_MODULE_BCDR_WR_FAIL       (DPP_RC_MODULE_BASE | 0x4)

/* reg module error code */
#define DPP_RC_REG_BASE                  (0xc00)
#define DPP_RC_REG_PARA_INVALID          (DPP_RC_REG_BASE | 0x0)
#define DPP_RC_REG_RANGE_INVALID         (DPP_RC_REG_BASE | 0x1)
#define DPP_RC_REG_CALL_FUNC_FAIL        (DPP_RC_REG_BASE | 0x2)
#define DPP_RC_REG_FIELD_OVERFLOW        (DPP_RC_REG_BASE | 0x3)

/* DTB module error code */
#define DPP_RC_DTB_BASE                     (0xd00)
#define DPP_RC_DTB_MGR_EXIST                (DPP_RC_DTB_BASE | 0x0)
#define DPP_RC_DTB_MGR_NOT_EXIST            (DPP_RC_DTB_BASE | 0x1)
#define DPP_RC_DTB_QUEUE_RES_EMPTY          (DPP_RC_DTB_BASE | 0x2)
#define DPP_RC_DTB_QUEUE_BUFF_SIZE_ERR      (DPP_RC_DTB_BASE | 0x3)
#define DPP_RC_DTB_QUEUE_ITEM_HW_EMPTY      (DPP_RC_DTB_BASE | 0x4)
#define DPP_RC_DTB_QUEUE_ITEM_SW_EMPTY      (DPP_RC_DTB_BASE | 0x5)
#define DPP_RC_DTB_TAB_UP_BUFF_EMPTY        (DPP_RC_DTB_BASE | 0x6)
#define DPP_RC_DTB_TAB_DOWN_BUFF_EMPTY      (DPP_RC_DTB_BASE | 0x7)
#define DPP_RC_DTB_TAB_UP_TRANS_ERR         (DPP_RC_DTB_BASE | 0x8)
#define DPP_RC_DTB_TAB_DOWN_TRANS_ERR       (DPP_RC_DTB_BASE | 0x9)
#define DPP_RC_DTB_QUEUE_IS_WORKING         (DPP_RC_DTB_BASE | 0xa)
#define DPP_RC_DTB_QUEUE_IS_NOT_INIT        (DPP_RC_DTB_BASE | 0xb)
#define DPP_RC_DTB_MEMORY_ALLOC_ERR         (DPP_RC_DTB_BASE | 0xc)
#define DPP_RC_DTB_PARA_INVALID             (DPP_RC_DTB_BASE | 0xd)
#define DPP_RC_DMA_RANGE_INVALID            (DPP_RC_DTB_BASE | 0xe)
#define DPP_RC_DMA_RCV_DATA_EMPTY           (DPP_RC_DTB_BASE | 0xf)
#define DPP_RC_DTB_LPM_INSERT_FAIL          (DPP_RC_DTB_BASE | 0x10)
#define DPP_RC_DTB_LPM_DELETE_FAIL          (DPP_RC_DTB_BASE | 0x11)
#define DPP_RC_DTB_DOWN_LEN_INVALID         (DPP_RC_DTB_BASE | 0x12)
#define DPP_RC_DTB_DOWN_HASH_CONFLICT       (DPP_RC_DTB_BASE | 0x13)   //3347
#define DPP_RC_DTB_QUEUE_NOT_ALLOC          (DPP_RC_DTB_BASE | 0x14) 
#define DPP_RC_DTB_QUEUE_NAME_ERROR         (DPP_RC_DTB_BASE | 0x15) 
#define DPP_RC_DTB_DUMP_SIZE_SMALL          (DPP_RC_DTB_BASE | 0x16) 
#define DPP_RC_DTB_SEARCH_VPORT_QUEUE_ZERO  (DPP_RC_DTB_BASE | 0x17) 
#define DPP_RC_DTB_QUEUE_NOT_ENABLE         (DPP_RC_DTB_BASE | 0x18)
#define DPP_RC_DTB_OPEN_DEBUG_MODE          (DPP_RC_DTB_BASE | 0x19)
#define DPP_RC_DTB_OVER_TIME                (DPP_RC_DTB_BASE | 0x1a)   //3354
#define DPP_RC_DTB_TABLE_SWITCH_NOT_ENABLE  (DPP_RC_DTB_BASE | 0x1b)
#define DPP_RC_DTB_BAR_ABNORMAL             (DPP_RC_DTB_BASE | 0x1c)
#define DPP_RC_DTB_STAT_QUEUE_NOT_ENABLE    (DPP_RC_DTB_BASE | 0x1d)
#define DPP_RC_DTB_STAT_OVER_TIME           (DPP_RC_DTB_BASE | 0x1e)
#define DPP_RC_DTB_STAT_QUEUE_ITEM_SW_EMPTY (DPP_RC_DTB_BASE | 0x1f)
#define DPP_RC_DTB_ADDR_INVALID             (DPP_RC_DTB_BASE | 0x20)

/* lif module error code */
#define DPP_RC_LIF_BASE                  (0xe00)
#define DPP_RC_LIF_PARA_INVALID             (DPP_RC_LIF_BASE | 0x0)
#define DPP_RC_LIF_RANGE_INVALID            (DPP_RC_LIF_BASE | 0x1)
#define DPP_RC_LIF_CALL_FUNC_FAIL           (DPP_RC_LIF_BASE | 0x2)
#define DPP_RC_LIF_OPER_MODE_ERR            (DPP_RC_LIF_BASE | 0x3)
#define DPP_RC_SERDES_ARRAY_OVERFLOW        (DPP_RC_LIF_BASE | 0x4)
#define DPP_RC_RCV_PLL_LOCK_TIMEOUT         (DPP_RC_LIF_BASE | 0x5)
#define DPP_RC_LIF_TIMEOUT                  (DPP_RC_LIF_BASE | 0x7)
#define DPP_RC_LIF_NO_PORT                  (DPP_RC_LIF_BASE | 0x8)
#define DPP_RC_BCDR_PARA_ERR                (DPP_RC_LIF_BASE | 0x9)
#define DPP_RC_LIF_NOT_SUPPORT              (DPP_RC_LIF_BASE | 0xa)

#define DPP_RC_CTRLCH_BASE                   (0xf00)
#define DPP_RC_CTRLCH_MSG_LEN_ZERO           (DPP_RC_CTRLCH_BASE | 0x0)
#define DPP_RC_CTRLCH_MSG_PRO_ERR            (DPP_RC_CTRLCH_BASE | 0x1)
#define DPP_RC_CTRLCH_MSG_TYPE_NOT_SUPPORT   (DPP_RC_CTRLCH_BASE | 0x2) /*消息类型不支持*/
#define DPP_RC_CTRLCH_MSG_OPER_NOT_SUPPORT   (DPP_RC_CTRLCH_BASE | 0x3) /*消息操作不支持*/
#define DPP_RC_CTRLCH_MSG_DROP               (DPP_RC_CTRLCH_BASE | 0x4) /*消息操作不支持*/

/* ddr module error code */
#define DPP_RC_DDR_BASE                  (0x1000)
#define DPP_RC_DDR_TIME_OUT          (DPP_RC_DDR_BASE | 0x0)
#define DPP_RC_DDR_TRAIN_FAIL        (DPP_RC_DDR_BASE | 0x1)
#define DPP_RC_DDR_TYPE_ERR          (DPP_RC_DDR_BASE | 0x2)
#define DPP_RC_DDR_LOGCHK_ERR        (DPP_RC_DDR_BASE | 0x3)
#define DPP_RC_DDR_PARA_ERR          (DPP_RC_DDR_BASE | 0x4)
#define DPP_RC_DDR_DAMAGED           (DPP_RC_DDR_BASE | 0x5)
#define DPP_RC_DDR_DISABLE           (DPP_RC_DDR_BASE | 0x6)
#define DPP_RC_DDR_ARRAY_OVERFLOW    (DPP_RC_DDR_BASE | 0x7)
#define DPP_RC_DDR_INVAL_TRAIN_MODE  (DPP_RC_DDR_BASE | 0x8)
#define DPP_RC_DDR_INVAL_VREF_MODE   (DPP_RC_DDR_BASE | 0x9)
#define DPP_RC_DDR_INTR_ERR          (DPP_RC_DDR_BASE | 0xa)
#define DPP_RC_DDR_BIST_FAIL         (DPP_RC_DDR_BASE | 0xb)
#define DPP_RC_DDR_SOFTCHK_FAIL      (DPP_RC_DDR_BASE | 0xb)
#define DPP_RC_DDR_PLL_LOCK_TIMEOUT  (DPP_RC_DDR_BASE | 0x0)

/* tlb module error code */
#define DPP_RC_TLB_BASE               (0x2000)
#define DPP_RC_TLB_MGR_EXIST          (DPP_RC_TLB_BASE | 0x0)
#define DPP_RC_TLB_MGR_NOT_EXIST      (DPP_RC_TLB_BASE | 0x1)

/* se alg error code */
#define DPP_SE_RC_BASE            (0x50000)
#define DPP_SE_RC_ZBLK_FULL          (DPP_SE_RC_CFG_BASE | 0x1)
#define DPP_SE_RC_FUN_INVALID        (DPP_SE_RC_CFG_BASE | 0x2)
#define DPP_SE_RC_PARA_INVALID       (DPP_SE_RC_CFG_BASE | 0x3)

#define DPP_SE_RC_CFG_BASE        (DPP_SE_RC_BASE | 0x1000)
#define DPP_SE_RC_ACL_BASE        (DPP_SE_RC_BASE | 0x2000)
#define DPP_SE_RC_LPM_BASE        (DPP_SE_RC_BASE | 0x3000)
#define DPP_LPM_TBL_INVALID       (DPP_SE_RC_LPM_BASE | 0x01)
#define DPP_LPM_MODE_INVALID      (DPP_SE_RC_LPM_BASE | 0x02)
#define DPP_LPM_FUNID_INVALID     (DPP_SE_RC_LPM_BASE | 0x03)
#define DPP_LPM_RAM_FULL          (DPP_SE_RC_LPM_BASE | 0x04)
#define DPP_LPM_UPDATE            (DPP_SE_RC_LPM_BASE | 0x05)
#define DPP_LPM_SRHFAIL           (DPP_SE_RC_LPM_BASE | 0x06)
#define DPP_LPM_KEY_INVALID       (DPP_SE_RC_LPM_BASE | 0x07)
#define DPP_LPM_DEF_REG_FULL      (DPP_SE_RC_LPM_BASE | 0x08) /*16组默认路由寄存器插满*/
#define DPP_LPM_DEF_REG_NO_HIT    (DPP_SE_RC_LPM_BASE | 0x09) /*16组默认路由寄存器未匹配删除条目*/
#define DPP_LPM_DEF_REG_UPDATE    (DPP_SE_RC_LPM_BASE | 0x0A)
#define DPP_LPM_DIV_GO_ON         (DPP_SE_RC_LPM_BASE | 0x0B)
#define DPP_LPM_DDR_FULL          (DPP_SE_RC_LPM_BASE | 0x0C)
#define DPP_LPM_DDR_PARA_INVALID  (DPP_SE_RC_LPM_BASE | 0x0D)

#define DPP_SE_RC_HASH_BASE       (DPP_SE_RC_BASE | 0x4000)
#define DPP_HASH_RC_INVALID_FUNCINFO          (DPP_SE_RC_HASH_BASE | 0x1)
#define DPP_HASH_RC_INVALID_ZBLCK             (DPP_SE_RC_HASH_BASE | 0x2)
#define DPP_HASH_RC_INVALID_ZCELL             (DPP_SE_RC_HASH_BASE | 0x3)
#define DPP_HASH_RC_INVALID_KEY               (DPP_SE_RC_HASH_BASE | 0x4)
#define DPP_HASH_RC_INVALID_TBL_ID_INFO       (DPP_SE_RC_HASH_BASE | 0x5)
#define DPP_HASH_RC_RB_TREE_FULL              (DPP_SE_RC_HASH_BASE | 0x6)
#define DPP_HASH_RC_INVALID_KEY_TYPE          (DPP_SE_RC_HASH_BASE | 0x7)
#define DPP_HASH_RC_ADD_UPDATE                (DPP_SE_RC_HASH_BASE | 0x8)
#define DPP_HASH_RC_DEL_SRHFAIL               (DPP_SE_RC_HASH_BASE | 0x9)
#define DPP_HASH_RC_ITEM_FULL                 (DPP_SE_RC_HASH_BASE | 0xa)
#define DPP_HASH_RC_INVALID_DDR_WIDTH_MODE    (DPP_SE_RC_HASH_BASE | 0xb)
#define DPP_HASH_RC_INVALID_PARA              (DPP_SE_RC_HASH_BASE | 0xc)
#define DPP_HASH_RC_TBL_FULL                  (DPP_SE_RC_HASH_BASE | 0xd)
#define DPP_HASH_RC_SRH_FAIL                  (DPP_SE_RC_HASH_BASE | 0xe)
#define DPP_HASH_RC_MATCH_ITEM_FAIL           (DPP_SE_RC_HASH_BASE | 0xf)
#define DPP_HASH_RC_DDR_WIDTH_MODE_ERR        (DPP_SE_RC_HASH_BASE | 0x10)
#define DPP_HASH_RC_INVALID_ITEM_TYPE         (DPP_SE_RC_HASH_BASE | 0x11)
#define DPP_HASH_RC_REPEAT_INIT               (DPP_SE_RC_HASH_BASE | 0x12)

#define DPP_SE_RC_ETCAM_BASE       (DPP_SE_RC_BASE | 0x5000)
#define DPP_ETCAM_RC_TBL_INVALID              (DPP_SE_RC_ETCAM_BASE | 0x1)
#define DPP_ETCAM_RC_TBL_OVERFLOW             (DPP_SE_RC_ETCAM_BASE | 0x2)

#define DPP_ACL_RC_BASE          (0x60000)
#define DPP_ACL_RC_INVALID_TBLID       (DPP_ACL_RC_BASE | 0x0)
#define DPP_ACL_RC_INVALID_BLOCKNUM    (DPP_ACL_RC_BASE | 0x1)
#define DPP_ACL_RC_INVALID_BLOCKID     (DPP_ACL_RC_BASE | 0x2)
#define DPP_ACL_RC_TBL_NOT_INIT        (DPP_ACL_RC_BASE | 0x3)
#define DPP_ACL_RC_ETCAMID_NOT_INIT    (DPP_ACL_RC_BASE | 0x4)
#define DPP_ACL_RC_AS_ERAM_NOT_ENOUGH  (DPP_ACL_RC_BASE | 0x5)
#define DPP_ACL_RC_RB_TREE_FULL        (DPP_ACL_RC_BASE | 0x6)
#define DPP_ACL_RC_TABLE_FULL          (DPP_ACL_RC_BASE | 0x7)
#define DPP_ACL_RC_INVALID_PARA        (DPP_ACL_RC_BASE | 0x8)
#define DPP_ACL_RC_DEL_SRHFAIL         (DPP_ACL_RC_BASE | 0x9)
#define DPP_ACL_RC_TABLE_UPDATE        (DPP_ACL_RC_BASE | 0xa)
#define DPP_ACL_RC_SRH_FAIL            (DPP_ACL_RC_BASE | 0xb)
#define DPP_ACL_RC_INDEX_MGR_NOT_EXIST (DPP_ACL_RC_BASE | 0xc)
#define DPP_ACL_RC_INDEX_RES_FULL      (DPP_ACL_RC_BASE | 0xd)

#define DPP_SE_RC_CAR_BASE       (0x1100)
#define DPP_RC_CAR_TIME_OUT           (DPP_SE_RC_CAR_BASE | 0x1)
#define DPP_RC_CAR_QUEUE_OUTRANGE     (DPP_SE_RC_CAR_BASE | 0x2)
#define DPP_RC_CAR_RB_TREE_FULL       (DPP_SE_RC_CAR_BASE | 0x3)
#define DPP_RC_CAR_RB_TREE_UPDATE     (DPP_SE_RC_CAR_BASE | 0x4)
#define DPP_RC_CAR_RB_TREE_DEL_FAIL   (DPP_SE_RC_CAR_BASE | 0x5)
#define DPP_RC_CAR_RB_TREE_SRH_FAIL   (DPP_SE_RC_CAR_BASE | 0x6)
#define DPP_RC_CAR_RB_TREE_GET_FAIL   (DPP_SE_RC_CAR_BASE | 0x7)
#define DPP_RC_CAR_INIT_FAIL          (DPP_SE_RC_CAR_BASE | 0x8)
#define DPP_RC_CAR_UN_INIT            (DPP_SE_RC_CAR_BASE | 0x9)
#define DPP_RC_CAR_PROFILE_UNSET      (DPP_SE_RC_CAR_BASE | 0xa)
#define DPP_RC_CAR_LISTSTACK_MNG_FULL (DPP_SE_RC_CAR_BASE | 0xb)


/*system critical item check error code*/
#define DPP_SYSTEM_CHECK_RC_BASE          (0x70000)
#define DPP_SYSTEM_CHECK_CRM_RC_BASE         (DPP_SYSTEM_CHECK_RC_BASE | 0x1000)
#define DPP_SYSTEM_CHECK_LIF0_RC_BASE         (DPP_SYSTEM_CHECK_RC_BASE | 0x2000)
#define DPP_SYSTEM_CHECK_PKTRX_RC_BASE         (DPP_SYSTEM_CHECK_RC_BASE | 0x3000)
#define DPP_SYSTEM_CHECK_PBU_RC_BASE         (DPP_SYSTEM_CHECK_RC_BASE | 0x4000)
#define DPP_SYSTEM_CHECK_PPU_RC_BASE         (DPP_SYSTEM_CHECK_RC_BASE | 0x5000)
#define DPP_SYSTEM_CHECK_ODMA_RC_BASE         (DPP_SYSTEM_CHECK_RC_BASE | 0x6000)
#define DPP_SYSTEM_CHECK_CFG_RC_BASE         (DPP_SYSTEM_CHECK_RC_BASE | 0x7000)
#define DPP_SYSTEM_CHECK_TM_RC_BASE         (DPP_SYSTEM_CHECK_RC_BASE | 0x8000)
#define DPP_SYSTEM_CHECK_TM_READY_RC_BASE         (DPP_SYSTEM_CHECK_RC_BASE | 0x100)
#define DPP_SYSTEM_CHECK_TM_SAIP_RC_BASE         (DPP_SYSTEM_CHECK_RC_BASE | 0x200)
#define DPP_SYSTEM_CHECK_TM_ILIF_RC_BASE         (DPP_SYSTEM_CHECK_TM_RC_BASE | 0x300)
#define DPP_SYSTEM_CHECK_TM_OLIF_RC_BASE         (DPP_SYSTEM_CHECK_TM_RC_BASE | 0x400)
#define DPP_SYSTEM_CHECK_TM_TMMU_RC_BASE         (DPP_SYSTEM_CHECK_TM_RC_BASE | 0x500)
#define DPP_SYSTEM_CHECK_TM_SHAPE_RC_BASE         (DPP_SYSTEM_CHECK_TM_RC_BASE | 0x600)
#define DPP_SYSTEM_CHECK_TM_CRDT_RC_BASE         (DPP_SYSTEM_CHECK_TM_RC_BASE | 0x700)
#define DPP_SYSTEM_CHECK_TM_CGAVD_RC_BASE         (DPP_SYSTEM_CHECK_TM_RC_BASE | 0x800)
#define DPP_SYSTEM_CHECK_TM_QMU_RC_BASE          (DPP_SYSTEM_CHECK_TM_RC_BASE | 0x900)

/*system critical item check error code*/
#define DPP_SERDES_CHECK_RC_BASE          (0x80000)
#define DPP_SERDES_CHECK_PLL_BASE          (DPP_SERDES_CHECK_RC_BASE | 0x1000)
#define DPP_SERDES_CHECK_PLL_A_BASE          (DPP_SERDES_CHECK_PLL_BASE | 0x100)
#define DPP_SERDES_CHECK_PLL_B_BASE          (DPP_SERDES_CHECK_PLL_BASE | 0x200)
#define DPP_SERDES_CHECK_PHYREADY_BASE     (DPP_SERDES_CHECK_RC_BASE | 0x2000)
#define DPP_SERDES_CHECK_TX_PHYREADY_BASE    (DPP_SERDES_CHECK_PHYREADY_BASE | 0x100)
#define DPP_SERDES_CHECK_RX_PHYREADY_BASE    (DPP_SERDES_CHECK_PHYREADY_BASE | 0x200)
#define DPP_SERDES_CHECK_BERTOK_BASE     (DPP_SERDES_CHECK_RC_BASE | 0x3000)

/*TM RAM检测项返回错误码定义*/
#define DPP_TM_CHECK_RC_BASE          (0x100000)
#define DPP_TM_CHECK_RAM_RC_BASE          (DPP_TM_CHECK_RC_BASE|0x100000)
#define DPP_RC_TM_CRDT_INS_BUSY              (DPP_TM_CHECK_RC_BASE|0x100)

/*SAIP RAM检测项返回错误码定义*/
#define DPP_SA_CHECK_RC_BASE          (0x110000)
#define DPP_SA_CHECK_RAM_RC_BASE          (DPP_SA_CHECK_RC_BASE|0x110000)

/*soft reset check error code*/
#define DPP_SOFT_RESET_CHECK_BASE          (0x1300)
#define DPP_SOFT_RESET_CHECK_WR_FAIL         (DPP_SOFT_RESET_CHECK_BASE | 0x0)
#define DPP_SOFT_RESET_CHECK_RD_FAIL         (DPP_SOFT_RESET_CHECK_BASE | 0x1)
#define DPP_SOFT_RESET_CHECK_WR_SIZE_ERR     (DPP_SOFT_RESET_CHECK_BASE | 0x2)
#define DPP_SOFT_RESET_CHECK_RD_SIZE_ERR     (DPP_SOFT_RESET_CHECK_BASE | 0x3)
#define DPP_SOFT_RESET_CHECK_BACKUP_ERR      (DPP_SOFT_RESET_CHECK_BASE | 0x4)

//add by yinxh at 2018-1-15
//soft interrupt check error code
#define DPP_SOFT_INT_CHECK_BASE               (0x1400)
#define DPP_SOFT_INT_CHECK_INVALID            (DPP_SOFT_INT_CHECK_BASE | 0x0)
#define DPP_SOFT_INT_CHECK_NOT_INIT           (DPP_SOFT_INT_CHECK_BASE | 0x1)
#define DPP_SOFT_INT_CHECK_SRH_FAIL           (DPP_SOFT_INT_CHECK_BASE | 0x2)
#define DPP_SOFT_INT_CHECK_RB_TREE_FULL       (DPP_SOFT_INT_CHECK_BASE | 0x3)


#ifdef __cplusplus
}
#endif

#endif



#ifndef __PS3_MGR_EVT__
#define __PS3_MGR_EVT__

#include "ps3_mgr_evt_raidhba.h"
#include "ps3_mgr_evt_swexp.h"
#include "ps3lib/ps3lib_event.h"


#pragma pack(1)

#if (defined(PS3_PRODUCT_EXPANDER) || defined(PS3_PRODUCT_SWITCH))
#define PS3_EVT_ATTR(evtcode)   (PS3_EVT_ATTR_EXTEND(evtcode))
#else 
#define PS3_EVT_ATTR(evtcode)   (ps3EvtCodeExtendToNormal(PS3_EVT_ATTR_EXTEND(evtcode)))
#endif


typedef Ps3LibPdAttrInfo_s MgrPdAttrInfo_s;

typedef Ps3LibPdPreFailInfo_s MgrPdPreFailInfo_s;

typedef Ps3LibSparePdInfo_s MgrSparePdInfo_s;

typedef Ps3LibVdAttrInfo_s MgrVdAttrInfo_s;

typedef Ps3LibDiskPFCfgModifyEvtInfo_s MgrDiskPFCfgModifyEvtInfo_s;


#define MAX_VD_NAME_BYTES (16)
typedef Ps3LibVdBaseSetting_s MgrVdBaseSetting_s;

typedef Ps3LibVdPropertiesInfo_s MgrVdPropertiesInfo_s;

typedef Ps3LibVdStateChangeInfo_s MgrVdStateChangeInfo_s;

typedef Ps3LibVdCreateEvtInfo_s MgrVdCreateInfo_s;

typedef Ps3LibDgAttrInfo_s MgrDgAttrInfo_s;

typedef Ps3LibExpanderInfo_s MgrExpanderInfo_s;

enum {
    MGR_EVT_FUNCTION0 = 0,
    MGR_EVT_FUNCTION1 = 1,
    MGR_EVT_FUNCTION_COMMON = 250,
};

typedef Ps3LibCtrlAttrInfo_s MgrCtrlAttrInfo_s;


typedef Ps3LibCtrlRebootInfo_s MgrCtrlRebootInfo_s;


typedef Ps3LibPdBatchEvtInfo_s MgrPdBatchEvtInfo_s;


typedef Ps3LibVdBatchEvtInfo_s MgrVdBatchEvtInfo_s;


typedef Ps3LibVdBbmBatchEvtInfo_s MgrVdBbmBatchEvtInfo_s;


typedef Ps3LibCtrlBatchEvtInfo_s MgrCtrlBatchEvtInfo_s;


typedef Ps3LibCfgAttrInfo_s MgrCfgAttrInfo_s;
#define MGR_CTRL_AUTOCONFIG_EVTDATA_SIZE  PS3LIB_CTRL_AUTOCONFIG_EVTDATA_SIZE
typedef Ps3LibCfgAutoConfig_s MgrCfgAutoConfig_s;


typedef Ps3LibBgtRebuildInfo_s MgrBgtRebuildInfo_s;


#define FGI_MODE_LEN PS3LIB_FGI_MODE_LEN
typedef Ps3LibBgtInitEvtInfo_s MgrBgtInitEvtInfo_s;


typedef Ps3LibBgtEraseEvtInfo_s MgrBgtEraseEvtInfo_s;


typedef Ps3LibPhyEvtInfo_s MgrPhyEvtInfo_s;

#define BBM_ERRTBL_NAME_LEN PS3LIB_BBM_ERRTBL_NAME_LEN

typedef Ps3LibVdBbmEvtInfo_s MgrVdBbmEvtInfo_s;

typedef Ps3LibRwDdtEvtInfo_s MgrRwDdtEvtInfo_s;

typedef Ps3LibFlushEvtInfo_s MgrFlushEvtInfo_s;

typedef Ps3LibBgtCcEvtInfo_s MgrBgtCcEvtInfo_s;

typedef Ps3LibBgtPrEvtInfo_s MgrBgtPrEvtInfo_s;


typedef Ps3LibBbuEvtInfo_s MgrBbuEvtInfo_s;

typedef Ps3LibMigrationInfo_s MgrMigrationInfo_s;


typedef Ps3LibUkeyEvtInfo_s MgrUkeyEvtInfo_s;


typedef Ps3LibExpEvtInfo_s MgrExpEvtInfo_s;


typedef Ps3LibOemInfo_s MgrOemInfo_s;


typedef Ps3LibBplaneEvtInfo_s BplaneEvtInfo_s;

typedef Ps3LibEccEvtInfo_s MgrEccEvtInfo_s;


typedef Ps3LibTempEvtInfo_s MgrTempEvtInfo_s;

typedef Ps3LibIoCmdType_s IoCmdType_s;


typedef Ps3LibSenseDataEvtInfo_s MgrSenseDataEvtInfo_s;

typedef Ps3LibPdDownloadInfo_s MgrPdDownloadInfo_s;

typedef Ps3LibSanitizeEvtInfo_s MgrSanitizeEvtInfo_s;

typedef Ps3LibFormatEvtInfo_s MgrFormatEvtInfo_s;

typedef Ps3LibSnapshotEvtInfo_s MgrSnapshotEvtInfo_s;

typedef Ps3LibReportEvtData_u MgrReportEvtData_u;

typedef enum Ps3HardResetEvtMoudle
{
    HARD_RESET_BY_HOST  = 0,
    HARD_RESET_BY_BACKEND,
    HARD_RESET_BY_FRONTEND,
} Ps3HardResetEvtMoudle_e;

typedef enum SporStatus {
    SPOR_DUMP_SUCCESS,
    SPOR_DUMP_RUNNING,
    SPOR_DUMP_NVSRAM_INIT_FAILED,
    SPOR_DUMP_NVSRAM_WR_FAILED,
    SPOR_DUMP_ONF_INIT_FAILED,
    SPOR_DUMP_MM_INIT_FAILED,
    SPOR_DUMP_CM_INIT_FAILED,
    SPOR_DUMP_FAILED,
    SPOR_LOAD_SUCCESS,
    SPOR_LOAD_NVSRAM_INIT_FAILED,
    SPOR_LOAD_NVSRAM_WR_FAILED,
    SPOR_LOAD_ONF_INIT_FAILED,
    SPOR_LOAD_MM_INIT_FAILED,
    SPOR_LOAD_CM_INIT_FAILED,
    SPOR_LOAD_FAILED,
    SPOR_STATUS_NULL = 0xFF,
}SporStatus_e;

#pragma pack()


typedef enum MgrEventModule
{
    PS3_EVT_HOST_DRV_X2 = 1,  
    PS3_EVT_HOST_DRV_X16,  
    PS3_EVT_HOST_DRV_VD_X2,   
    PS3_EVT_HOST_DRV_VD_X16,   
    PS3_EVT_METDATA,       
    PS3_EVT_DEV_MANAGE,    
    PS3_EVT_IOC_ALARM_PWM, 
    PS3_EVT_IOC_ALARM_LED, 
    PS3_EVT_IOC_ALARM,     
    PS3_EVT_IOC_MGR,       
    PS3_EVT_HOST_SIM,      
    PS3_EVT_HOST_SIM_X2,
    PS3_EVT_HOST_SIM_X16,
    PS3_EVT_MODULE_MAX,
}MgrEventModule_e;

typedef struct PS3EventFilter
{
    U8 eventType;          
    U8 eventCodeCnt;       
    U8 reserved[6];
    U16 eventCodeTable[0]; 
}PS3EventFilter_s;

typedef void (* eventNoticeCb)(void *pCtxt);

typedef S32 (* mgrEventNoticeFunc)(U32 eventCode, MgrReportEvtData_u *pEventDesc,
            void *pCtxt, eventNoticeCb noticeCountOp);
typedef void (* eventPublishCb)(void *pPublishInfo);

S32 mgrEvtCancleSubscribe(U8 moduleId);

S32 mgrDrvEvtWebSubsCancel(U8 funcType);

#endif

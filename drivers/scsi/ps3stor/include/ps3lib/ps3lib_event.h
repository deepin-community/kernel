
#ifndef __PS3LIB_EVENT_H__
#define __PS3LIB_EVENT_H__

#if defined(__cplusplus)
extern "C" {
#endif

#define PS3LIB_MAX_VD_NAME_BYTES             (16)  
#define PS3LIB_CTRL_AUTOCONFIG_EVTDATA_SIZE  (8)   
#define PS3LIB_BBM_ERRTBL_NAME_LEN           (6)   
#define PS3LIB_EVT_DESC_MAX_LEN              (4096)
#define PS3LIB_FGI_MODE_LEN                  (5)
#define PS3LIB_EVT_LOG_INFO_MAX_SIZE         (116)
#define PS3LIB_EXP_EVENT_DATA_COLLECT_MAX_NUM (256)
#define PS3LIB_MAX_EVENT_REG_CNT              (128)  

enum {
    PS3LIB_EVT_LOG_OLDEST,       
    PS3LIB_EVT_LOG_LATEST,       
    PS3LIB_EVT_LOG_LAST_CLEAR,   
    PS3LIB_EVT_LOG_LAST_REBOOT,  
    PS3LIB_EVT_LOG_LAST_SHUTDOWN,
    PS3LIB_EVT_LOG_FATAL_OLDEST, 
    PS3LIB_EVT_LOG_FATAL_LATEST, 
    PS3LIB_EVT_LOG_LAST_MAX,
};

typedef enum Ps3LibEpEventLevel
{
    PS3LIB_EVT_CLASS_UNKNOWN   = 0b0000,   
    PS3LIB_EVT_CLASS_DEBUG     = 0b0011,   
    PS3LIB_EVT_CLASS_PROCESS   = 0b0101,   
    PS3LIB_EVT_CLASS_INFO      = 0b0001,   
    PS3LIB_EVT_CLASS_WARNING   = 0b0010,   
    PS3LIB_EVT_CLASS_CRITICAL  = 0b0100,   
    PS3LIB_EVT_CLASS_FATAL     = 0b1000,   
    PS3LIB_EVT_CLASS_MAX,
} Ps3LibEpEventLevel_e;

enum
{ 
    PS3LIB_CTRL_EVT_SAS_INFO_LOCAL = 1,
    PS3LIB_CTRL_EVT_PD_COUNT_LOCAL = 2,
    PS3LIB_CTRL_EVT_VD_COUNT_LOCAL = 3,
    PS3LIB_CTRL_EVT_CTRL_INFO_LOCAL = 4,
    PS3LIB_CTRL_EVT_PD_ATTR_LOCAL = 5,
    PS3LIB_CTRL_EVT_VD_ATTR_LOCAL = 6,
    PS3LIB_CTRL_EVT_DG_INFO_LOCAL = 7,
    PS3LIB_CTRL_EVT_BBU_INFO_LOCAL = 8,
    PS3LIB_CTRL_EVT_CONFIG_LOCAL = 9,
    PS3LIB_CTRL_EVT_IO_INFO_LOCAL = 10,
    PS3LIB_CTRL_EVT_UKEY_INFO_LOCAL = 11,
    PS3LIB_CTRL_EVT_HWR_INFO_LOCAL = 12,
    PS3LIB_CTRL_EVT_ALARM_INFO_LOCAL = 13,
    PS3LIB_CTRL_EVT_ECC_INFO_LOCAL = 14,
    PS3LIB_CTRL_EVT_UPGRADE_INFO_LOCAL = 15,
    PS3LIB_CTRL_EVT_TEMP_INFO_LOCAL = 16,
    PS3LIB_CTRL_EVT_PD_ATTR_EXTEND_LOCAL = 17,
    PS3LIB_CTRL_EVT_DEFAULT_UNUSED_LOCAL,
    PS3LIB_CTRL_EVT_MAX_TYPE_LOCAL,
}; 

typedef struct Ps3LibEvtLogRdEntry {
    U32 loopCnt;       
    U32 seqNum;        
    U32 offset;        
    U32 timeStampBySec;
    U32 size;          
} Ps3LibEvtLogRdEntry_s;

typedef struct Ps3LibEvtLogRdInfo {
    Ps3LibEvtLogRdEntry_s persistInfo[PS3LIB_EVT_LOG_LAST_MAX];
} Ps3LibEvtLogRdInfo_s;

typedef struct Ps3LibEvtPersistInfo {
    U32               regionSz[2];  
    Ps3LibEvtLogRdInfo_s persist;   
} Ps3LibEvtPersistInfo_s;

typedef struct Ps3LibEvtLogHeader {
    U32 magic;         
    U32 seqNum;        
    U32 size    : 8;   
    U32 funcType  : 2; 
    U32 conFlag : 1;   
    U32 evtCode : 12;  
    U32 level : 4;     
    U32 type : 5;      
    U32 timeStampBySec;
} Ps3LibEvtLogHeader_s;

#pragma pack(1)

typedef struct Ps3LibPdAttrInfo {
    U32 checkSum    : 8;        
    U32 enclosureId : 8;        
    U32 phyId       : 8;        
    U32 evtVersion  : 8;        
    U16 phyDiskID;              
    U16 softChan    : 4;        
    U16 devID       : 12;       
    U16 slotId;                 
    U16 oldState    : 4;        
    U16 newState    : 4;        
    U16 isEnclPd    : 1;        
    U16 pad1        : 7;        
    U16 arrayId     : 8;        
    U16 rowId       : 8;        
    U16 prevState   : 8;        
    U16 curState    : 8;        
    U64 sasAddr;                
}Ps3LibPdAttrInfo_s;

typedef struct Ps3LibSparePdInfo {
    Ps3LibPdAttrInfo_s baseInfo;
    U8 dedicatedDgCnt;   
    U8 reserved[3];      
    U16 dedicatedDgId[8];
}Ps3LibSparePdInfo_s;

typedef struct Ps3LibVdAttrInfo {
    U32 magicNum;                  
    U16 virtDiskID;                
    U16 softChan    : 4,           
        devID       : 12;          
    U16 diskGrpId;                 
    U16 locked      : 1,           
        pad         : 15;          
}Ps3LibVdAttrInfo_s;

typedef struct Ps3LibDiskPFCfgModifyEvtInfo {
    U8 modifyCfgDataType;
    U8 funcIsEnable:1;         
    U8 pad : 7;                
    U16 preFailPollTimeMin;    
}Ps3LibDiskPFCfgModifyEvtInfo_s;

typedef struct Ps3LibVdBaseSetting {
    U32 accessPolicy            :2, 
        hidden                  :1, 
        defaultWriteCachePolicy :2, 
        currentWriteCachePolicy :1, 
        defaultReadCachePolicy  :1, 
        currentReadCachePolicy  :1, 
        diskCachePolicy         :2, 
        ioPolicy                :1, 
        noBgi                   :1, 
        emulationType           :2, 
        unmap                   :1, 
        cbSize                  :2, 
        cbMode                  :3, 
        encryption              :1, 
        rebootNoVerify          :1, 
        rsv                     :10;
    U8  vdName[PS3LIB_MAX_VD_NAME_BYTES];
    U64 size;                       
}Ps3LibVdBaseSetting_s;

typedef struct Ps3LibVdPropertiesInfo {
    Ps3LibVdAttrInfo_s baseInfo;       
    Ps3LibVdBaseSetting_s oldSetting;  
    Ps3LibVdBaseSetting_s newSetting;  
}Ps3LibVdPropertiesInfo_s;

typedef struct Ps3LibVdStateChangeInfo {
    Ps3LibVdAttrInfo_s baseInfo;       
    U8 oldVdState;                     
    U8 newVdState;                     
    U8 reserved[2];                    
}Ps3LibVdStateChangeInfo_s;

typedef struct Ps3LibVdCreateEvtInfo {
    Ps3LibVdAttrInfo_s baseInfo;       
    Ps3LibVdBaseSetting_s setting;     
}Ps3LibVdCreateEvtInfo_s;

typedef struct Ps3LibCtrlAttrInfo {
    U32 supportUnevenSpans  : 1;
    U32 supportJbodSecure   : 1;
    U32 supportCrashDump    : 1;
    U32 supportNvmePassthru : 1;
    U32 supportDirectCmd    : 1;
    U32 supportAcceleration : 1;
    U32 supportNcq          : 1;
    U32 reserved1           : 25;
    U32 reserved2[1];            
    U64 oldSysTime;
    U64 newSysTIme;
    U64 monoSysTime;            
    U32 newSysTimeYear;         
    U32 newSysTimeMon;          
    U32 newSysTimeDay;          
    U32 newSysTimeHour;         
    U32 newSysTimeMin;          
    U32 newSysTimeSec;          
    U64 cfgNum;
    U8  *pValue;       
    U32 len;           
}Ps3LibCtrlAttrInfo_s;

typedef struct Ps3LibCtrlRebootInfo {
    U16 ctrlBootMode;
    U16 ctrlShutDownReason;





    U32 ctrlBootCount;    
}Ps3LibCtrlRebootInfo_s;

typedef struct Ps3LibDgAttrInfo {
    U16 dgId;               
    U16 reserved[3];
}Ps3LibDgAttrInfo_s;

typedef struct Ps3LibExpanderInfo {
    U8 EnclId;  
    U8 reserved[7];
}Ps3LibExpanderInfo_s;

typedef struct Ps3LibCfgAttrInfo {
    U16   profileId;
    U8 reserved[2];
}Ps3LibCfgAttrInfo_s;

typedef struct Ps3LibCfgAutoConfig {
    S8   cfgName[PS3LIB_CTRL_AUTOCONFIG_EVTDATA_SIZE];
}Ps3LibCfgAutoConfig_s;

typedef struct Ps3LibBgtRebuildInfo {
    U16 newPDFlatId;     
    U16 newEnclosureId;  
    U16 newSlotId;       
    U16 oldPDFlatId;     
    U16 oldEnclosureId;  
    U16 oldSlotId;       
    U16 virtDiskId;      
    U16 devId;           
    U32 remainSecs;      
    U64 errorPba;        
    U64 errorLba;        
    U8  progressPercent; 
    U8  rebuildRate;     
    U8  enableMoveback;  
    U8  autoRebuild;     
    U8  eghs;            
    U8  enablePdm;                  
    U8  pdmSupportReadyPd;          
    U8  reserved[1];
    U32 pdmTimerInterval;           
} Ps3LibBgtRebuildInfo_s;

typedef struct Ps3LibBgtInitEvtInfo {
    U32 aliveSec;      
    U32 progressRate;  
    U16 dgId;          
    U16 virtDiskId;    
    U16 pdFlatId;      
    U16 enclosureId;   
    U16 slotId;        
    U8  cpuRate;       
    S8  mode[PS3LIB_FGI_MODE_LEN];
    U64 mediumErrLba;  
    U64 mediumErrPba;  
    U16 MediumErrPdFlatId;
    U16 softChan : 4;  
    U16 devID    : 12; 
} Ps3LibBgtInitEvtInfo_s;

typedef struct Ps3LibBgtEraseEvtInfo {
    U32 aliveSec;      
    U32 progressRate;  
    U16 dgId;          
    U16 virtDiskId;    
    U16 pdFlatId;      
    U16 enclosureId;   
    U16 slotId;        
    U16 devId;         
} Ps3LibBgtEraseEvtInfo_s;

typedef struct Ps3LibBgtCcEvtInfo {
    U32 aliveSecs;      
    U8  progressPercent;
    U8  ccRate;
    U8  mode;
    U8  resered;
    U16 virtDiskID;     
    U16 diskGroupID;
    U32 inconsistStrip;
    U16 devId;
    U16 faultDiskID;
    U16 enclosureId;
    U16 slotId;
    U64 pdErrLba;
    U64 vdErrLba;
}Ps3LibBgtCcEvtInfo_s;

typedef struct Ps3LibBgtPrEvtInfo {
    U32 aliveSecs;      
    U16 enclosureId;    
    U16 slotId;         
    U8  progressPercent;
    U8  prRate;
    U16 virtDiskID;     
    U16 pdFlatId;
    U16 diskGroupID;
    U16 dgStatus;
    U8 reserved[2];
    U64 errLba;
}Ps3LibBgtPrEvtInfo_s;

typedef struct Ps3LibPhyEvtInfo {
    U32 enclosureId:8,  
        slotId:8,       
        phyId:8,        
        reserved:8;
} Ps3LibPhyEvtInfo_s;

typedef struct Ps3LibVdBbmEvtInfo {
    U64 lba;           
    U64 pba;           
    U16 lbaLen;        
    U16 dgId;          
    U16 virtDiskId;    
    U16 percentErrTbl; 
    U16 devId;         
    S8  errTblName[PS3LIB_BBM_ERRTBL_NAME_LEN];
    U16 pdFlatId;      
    U16 enclosureId;   
    U16 slotId;        
    U16 reserved;
} Ps3LibVdBbmEvtInfo_s;

typedef struct Ps3LibRwDdtEvtInfo {
    U16 virtDiskID;     
    U16 diskGroupID;    
    U32 vdLen;          
    U64 vdLba;          
} Ps3LibRwDdtEvtInfo_s;

typedef struct Ps3LibFlushEvtInfo {
    U16 opcode;        
    U16 dgId;          
    U16 virtDiskId;    
    U16 minVdId;       
    U16 devId;         
    U8  reserved[6];
    U64 vdIdMap[3];    
} Ps3LibFlushEvtInfo_s;

typedef struct Ps3LibMigrationInfo {
    U8  migrRate;
    U8  resv;
    U16 diskGroupID;
    U32 percent;
    U32 aliveSec;
    U16 currVdId;
    U8 reserved[2];
} Ps3LibMigrationInfo_s;

typedef struct Ps3LibVdBbmBatchEvtInfo
{
    U32 count;
    Ps3LibVdBbmEvtInfo_s vdBbmEvtInfo[0];
} Ps3LibVdBbmBatchEvtInfo_s;

typedef struct Ps3LibVdBatchEvtInfo
{
    U32 count;
    Ps3LibVdAttrInfo_s vdInfo[0];
}Ps3LibVdBatchEvtInfo_s;

typedef struct Ps3LibPdBatchEvtInfo
{
    U32 count;
    Ps3LibPdAttrInfo_s pdInfo[0];
}Ps3LibPdBatchEvtInfo_s;

typedef struct Ps3LibCtrlBatchEvtInfo
{
    U32 count;
    Ps3LibCtrlAttrInfo_s ctrlInfo[0];
}Ps3LibCtrlBatchEvtInfo_s;

typedef struct Ps3libBatchEvtInfoCommon
{
    U32 count;
    S8 batchInfo[0];
}Ps3libBatchEvtInfoCommon_s;

typedef struct Ps3LibBbuEvtInfo {
    U8  absent : 1;                      
    U8  overTemp : 1;                    
    U8  overVol : 1;                     
    U8  overCur : 1;                     
    U8  overLoad : 1;                    
    U8  lifeisOver : 1;                  
    U8  reserved : 2;
    U8  status;                          
    U8  chargeStatus;                    
    U8  learnStage;                      
    S16  batTemperature;                  
    U16  batVoltage;                      
    S16  batCurrent;                      
    U8  reserved1[2];
} Ps3LibBbuEvtInfo_s;

typedef struct Ps3LibUkeyEvtInfo {
    U8  ukeyStatus;                   
    U8  reserved[3];
} Ps3LibUkeyEvtInfo_s;

typedef struct Ps3LibExpEvtInfo {
    U64 expanderSasAddr;
    U64 attachedSasAddr;
    U8  phyId[8];       
} Ps3LibExpEvtInfo_s;

typedef struct Ps3LibEccEvtInfo {
    U32 eccErrObj;
    U32 eccSingleBitCntInc;
    U8  eccErrCntThreshold;
    U8  pad[3];
    U16 eccClearPeriod;
    U8  eccType;
    U8  eccErrSubObj;
    U64 eccMutilErrAddr;
    U32 eccEvtVersion;
} Ps3LibEccEvtInfo_s;

typedef struct Ps3LibTempEvtInfo {
    U32 tempType;
    S32 tempErrThreshold[4];
    S32 temperature;
} Ps3LibTempEvtInfo_s;

typedef struct Ps3LibIoCmdType {
    U8 cmdType;
    U8 rsv[9];
} Ps3LibIoCmdType_s;

typedef struct Ps3LibDeviceResetEvtInfo {
    U16 enclosureId;
    U16 slotId;     
    U16 phyDiskID;  
    U16 resetType;
    U64 sasAddress;
} Ps3LibDeviceResetEvtInfo_s;

typedef struct Ps3LibSenseDataEvtInfo {
    U16 enclosureId;  
    U16 slotId;       
    U16 phyDiskID;    
    union {
        U8 cdb[10];  
        Ps3LibIoCmdType_s ioCmdType;
    };
    U8 ioFormat;
    U8 palErr;
    U8 dataPre;
    U8 scsiStatus;
    U8 skStatus;
    U8 sk;
    U8 asc;
    U8 ascq;
    U64 path;
} Ps3LibSenseDataEvtInfo_s;

typedef struct Ps3LibPdDownloadInfo {
    U32 downloadMode;
    S32 isSuccess;
    U16 phyDiskID;              
    U16 softChan    : 4;       
    U16 devID       : 12;       
    U16 enclosureId;  
    U16 slotId;       
}Ps3LibPdDownloadInfo_s;

typedef struct Ps3LibSanitizeEvtInfo {
    Ps3LibPdAttrInfo_s baseInfo;
    U32 aliveSecs;      
    U8  progressPercent;
    U8  pad[3];
}Ps3LibSanitizeEvtInfo_s;

typedef struct Ps3LibFormatEvtInfo {
    Ps3LibPdAttrInfo_s baseInfo;
    U32 aliveSecs;      
    U8  progressPercent;
    U8  pad[3];
}Ps3LibFormatEvtInfo_s;

typedef struct Ps3LibSnapshotEvtInfo {
    U8  snapCount; 
    U8  pad[3];    
}Ps3LibSnapshotEvtInfo_s;

typedef struct Ps3LibPdPreFailInfo {
    U32 checkSum    : 8;       
    U32 oldState    : 4;       
    U32 newState    : 4;       
    U32 diskType    : 4;       
    U32 pad         : 12;
    U16 phyDiskID;              
    U16 softChan    : 4;       
    U16 devID       : 12;       
    U16 enclosureId;           
    U16 slotId;                
    U32 historyErrBitMap;    
    U32 errBitMap;    
    S8  vendor[8];
    S8  diskSerialNum[24];
}Ps3LibPdPreFailInfo_s;

typedef struct Ps3LibNvDataInvaildInfo {
    U32  nvDataIDBitMap[16];   
    U16  bitmapSize;           
    U16  invaildCount;         
} Ps3LibNvDataInvaildInfo_s;

typedef struct Ps3LibSpeedNegoInfo {
    U16 enclosureId;   
    U16 slotId;        
    S8 isPcie;
    S8 speed;          
    U16 pad;
}Ps3LibSpeedNegoInfo_s;

typedef struct Ps3LibOemInfo {
    S8  oemData[PS3LIB_EVT_LOG_INFO_MAX_SIZE];
} Ps3LibOemInfo_s;

typedef struct Ps3LibBplaneEvtInfo {
    S8  bplaneData[PS3LIB_EVT_LOG_INFO_MAX_SIZE];
} Ps3LibBplaneEvtInfo_s;

typedef union Ps3LibReportEvtData
{
    Ps3LibPdAttrInfo_s    pdInfo;
    Ps3LibSparePdInfo_s   sparePdInfo;
    Ps3LibVdAttrInfo_s    vdInfo;
    Ps3LibVdPropertiesInfo_s vdChange;
    Ps3LibVdStateChangeInfo_s vdStateChangeInfo;
    Ps3LibVdCreateEvtInfo_s  vdCreate;
    Ps3LibCtrlAttrInfo_s  ctrlInfo;
    Ps3LibCtrlRebootInfo_s  ctrlRebootInfo;
    Ps3LibDgAttrInfo_s    dgInfo;
    Ps3LibExpanderInfo_s  expanderInfo;
    Ps3LibCfgAttrInfo_s   cfgInfo;
    Ps3LibCfgAutoConfig_s   autoConfigInfo;

    Ps3LibBgtRebuildInfo_s bgtRebuildInfo;
    Ps3LibBgtInitEvtInfo_s bgtInitEvtInfo;
    Ps3LibBgtEraseEvtInfo_s bgtEraseEvtInfo;
    Ps3LibBgtCcEvtInfo_s   bgtCcInfo;

    Ps3LibBgtPrEvtInfo_s   bgtPrInfo;

    Ps3LibPhyEvtInfo_s     phyInfo;

    Ps3LibVdBbmEvtInfo_s  vdBbmEvtInfo;
    Ps3LibRwDdtEvtInfo_s  dataVdInfo;

    Ps3LibFlushEvtInfo_s   flushEvtInfo;

    Ps3LibMigrationInfo_s  bgtMigrInfo;

    Ps3LibVdBatchEvtInfo_s   batchVdInfo;
    Ps3LibPdBatchEvtInfo_s   batchPdInfo;
    Ps3LibCtrlBatchEvtInfo_s batchCtrlInfo;
    Ps3LibVdBbmBatchEvtInfo_s batchBbmInfo;
    Ps3libBatchEvtInfoCommon_s *pBatchCommonInfo;
    Ps3LibBbuEvtInfo_s        bbuEvtInfo;
    Ps3LibUkeyEvtInfo_s       ukeyInfo;

    Ps3LibExpEvtInfo_s        expEvtInfo;
    Ps3LibOemInfo_s           oemEvtInfo;
    Ps3LibBplaneEvtInfo_s     bplaneEvtInfo;
    Ps3LibEccEvtInfo_s        eccEvtInfo;
    Ps3LibTempEvtInfo_s       tempEvtInfo;
    Ps3LibDeviceResetEvtInfo_s deviceResetEvtInfo;
    Ps3LibSenseDataEvtInfo_s  senseDataEvtInfo;
    Ps3LibPdDownloadInfo_s    pdDldEvtInfo;
    Ps3LibSanitizeEvtInfo_s   sanitizeInfo;
    Ps3LibFormatEvtInfo_s     formatInfo;
    Ps3LibSnapshotEvtInfo_s   snapShotInfo;
    Ps3LibPdPreFailInfo_s     pdPrefailInfo;
    Ps3LibDiskPFCfgModifyEvtInfo_s  diskPFCfgModifyEvtInfo;
    Ps3LibNvDataInvaildInfo_s nvDataInvaildInfo;
    Ps3LibSpeedNegoInfo_s     speedNegoInfo;


    U64 value;


    U8 data[PS3LIB_EVT_LOG_INFO_MAX_SIZE];
}Ps3LibReportEvtData_u;
#pragma pack()

typedef struct Ps3LibEvtLogEntry {
    U32                seqNum;
    Ps3LibEvtLogHeader_s  head;   
    Ps3LibReportEvtData_u evtInfo;
    CtrlId_t           ctrlId;    
    CtrlId_t           regCtrlId; 
    U32                registerId;
    U32                pad;       
} Ps3LibEvtLogEntry_s;

typedef struct Ps3LibEvtLogList {
    U32                   count;       
    Ps3LibEvtLogEntry_s   evtEntry[0]; 
} Ps3LibEvtLogList_s;

typedef struct Ps3LibEvtErrDataEntry {
    U32 beforeSeqNum;
    U32 errDataLen;  
    U8 *errData;     
} Ps3LibEvtErrDataEntry_s;

typedef struct Ps3LibEvtLog {
    Ps3LibEvtPersistInfo_s   evtPerInfo;  
    U32                      evtCount;    
    U32                      errCount;    
    Ps3LibEvtLogEntry_s *    evtEntryList;
    Ps3LibEvtErrDataEntry_s *errDataList; 
} Ps3LibEvtLog_s;

typedef struct Ps3LibEventDataCollectionKV {
    char key[PS3LIB_EXP_EVENT_DATA_COLLECT_MAX_NUM];  
    char val[PS3LIB_EXP_EVENT_DATA_COLLECT_MAX_NUM];  
} Ps3LibEventDataCollectionKV_s;

typedef struct Ps3LibEventDataCollection {
    struct Ps3LibEventDataCollectionKV kv[PS3LIB_EXP_EVENT_DATA_COLLECT_MAX_NUM];
} Ps3LibEventDataCollection_s;

enum {
    PS3LIB_EXPANDER_EVENT_TYPE = 0,      
    PS3LIB_SWITCH_EVENT_TYPE   = 1,      
    PS3LIB_RAID_HBA_EVENT_TYPE = 0xff,   
};

typedef struct Ps3LibEvtPrintFunc{
    S8 const *(*evtCode2Str)(U32 opCode);
    S8 const *(*evtLoca2Str)(U8 locate);
    const S8 *(*getEvtDesc)(Ps3LibEvtLogEntry_s *event, S32 len, S8 *buff, S32 buffLen);
    S32(*getEvtData)
    (Ps3LibEventDataCollection_s *eventDataCollection, Ps3LibEvtLogEntry_s *event, S32 len, S8 *buff, S32 buffLen);
}Ps3LibEvtPrintFunc_t;

Ps3LibEvtPrintFunc_t *ps3libEventPrintFunc(CtrlId_t ctrlId, U8 eventType);

Ps3Errno ps3libCtrlEvtlogPerGet(CtrlId_t ctrlId, Ps3LibEvtPersistInfo_s *evtlogRdInfo);

Ps3Errno ps3libEventLogGet(CtrlId_t ctrlId, U32 sinceSeqNum, Ps3LibEvtLog_s **ppEvtLog);

void ps3libEventLogDestroy(Ps3LibEvtLog_s *pEvtLog);

S32 ps3libEvtLevelCompare(U8 levelA, U8 levelB);

Ps3Errno ps3libCtrlEventLogsDelete(CtrlId_t ctrlId);

U32 ps3libEventUinqueIdToCtrlId(U32 uniqueId);



#if defined(__cplusplus)
}
#endif

#endif

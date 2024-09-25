
#ifndef _PS3_HTP_DEV_H_
#define _PS3_HTP_DEV_H_

#include "ps3_htp_def.h"

#define  PS3_MAX_CHANNEL_NUM    15
#define  PS3_MAX_RANDOM_NUM     32
#define  PS3_MAX_IV_NUM         16
#define  PS3_SECURITY_CIPHER_NUM_MAX     2
#define  PS3_STABLE_WRITES_MASK     (0x1)


struct PS3IocCtrlProp {
    U32 enableSnapshot          : 1;
    U32 enableSoftReset   : 1;
    U32 reserved1:30;
    U32 reserved2;
};


struct PS3IocCtrlCapable {
    U32 supportUnevenSpans  : 1;
    U32 supportJbodSecure   : 1;
    U32 supportNvmePassthru : 1;
    U32 supportDirectCmd    : 1;
    U32 supportAcceleration : 1;
    U32 supportSataDirectCmd: 1;
    U32 supportSataNcq      : 1;
    U32 reserved1           : 25;
    U32 reserved2[3];
};

#define PS3_IOC_CLUSTER_SERIAL_NO_SIZE 16

struct PS3ChannelAttr {
    U16 channelType : 4;   
    U16 maxDevNum   : 12;  
};

struct PS3ChannelInfo {
    U8 channelNum;   
    U8 reserved;
    struct PS3ChannelAttr channels[PS3_MAX_CHANNEL_NUM];
};

struct PS3QosInfo {
	U16 tfifoDepth;       
	U16 sataHddQuota;       
	U16 sataSsdQuota;       
	U16 sasHddQuota;      
	U16 sasSsdQuota;     
	U16 nvmeVdQuota;      
	U16 nvmeDirectQuota;  
	U16 nvmeNormalQuota;  
};


struct PS3IocCtrlInfo {

    U16 maxVdCount;

    U16 maxPdCount;


    U32 maxSectors;
    struct PS3IocCtrlProp properties;
    struct PS3IocCtrlCapable capabilities;

    U8 scsiTaskAbortTimeout;
    U8 scsiTaskResetTimeout;

    U16 offsetOfVDID;
    U8 reserved1[2];

    U16 cancelTimeOut;


    U32 vdIOThreshold;

    U8 iocPerfMode;

    U8 vdQueueNum;

    U8 ioTimeOut;
    U8 reserved2[1];

    struct PS3ChannelInfo channelInfo;

    struct PS3QosInfo qosInfo;
    U16 isotoneTimeOut;
    U8 reserved3[2];

    
    U8 reserved4[32];

};

struct PS3Dev {
    union {
        U16 phyDiskID;             
        U16 virtDiskID;            
    };
    U16 softChan    : 4;           
    U16 devID       : 12;          
};

typedef union PS3DiskDev {
    U32 diskID;                    
    struct PS3Dev ps3Dev;          
}PS3DiskDev_u;


struct PS3DiskDevPos {
    union {
        struct {
            U8  checkSum;           
            U8  enclId;             
            U8  phyId;              
        };
        U32 diskMagicNum;           
    };
    PS3DiskDev_u diskDev;           
};


struct PS3PhyDevice {
    struct PS3DiskDevPos diskPos;  
    U8 diskState;                  
    U8 configFlag;                 
    U8 driverType:4;               
    U8 mediumType:4;               
    U8 reserved;
    U8 reserved1[4];
};


struct PS3VirtDevice {
    struct PS3DiskDevPos diskPos;  
    U8 accessPolicy;
    U8 isHidden;                   
    U8 diskState;                  
    U8 reserved;
    U8 reserved1[4];
};

union PS3Device {
    struct PS3PhyDevice pd;        
    struct PS3VirtDevice vd;       
};

struct PS3DevList {
    U16 count;                     
    U8 reserved[6];
    union PS3Device devs[0];       
};

struct PS3PDInfo {
    struct PS3DiskDevPos diskPos;  
    U8 diskState;                  
    U8 configFlag;                 
    U8 driverType:4;               
    U8 mediumType:4;               
    U8 scsiInterfaceType;          
    U8 taskAbortTimeout;           
    U8 taskResetTimeout;           
    union {
        struct {
            U8 supportNCQ:1;       
            U8 protect:1;          
            U8 reserved:6;         
        };
        U8 pdFlags;                
    };
    U8 reserved1;
    U16 sectorSize;                
    U8 reserved2[2];
    U8 enclId;                     
    U8 phyId;                      
    U8 dmaAddrAlignShift;          
    U8 dmaLenAlignShift;           
    U8 reserved3[4];
    U32 maxIOSize;                 
    U32 devQueDepth;               
    U16 normalQuota;               
    U16 directQuota;               
    U8 reserved4[20];
};


struct PS3Extent   {
    PS3DiskDev_u phyDiskID;        
    U8  state;                     
    U8  reserved[3];
};

struct PS3Span {
    U32 spanStripeDataSize;        
    U8 spanState;                  
    U8 spanPdNum;                  
    U8 reserved[2];
    struct PS3Extent extent[PS3_MAX_PD_COUNT_IN_SPAN]; 
};

struct PS3VDEntry {
    struct PS3DiskDevPos diskPos;  
    U16 sectorSize;                
    U16 stripSize;                 
    U32 stripeDataSize;            
    U16 physDrvCnt;                
    U16 diskGrpId;                 
    U8 accessPolicy;               

    U8 reserved1;
    U8 dmaAddrAlignShift;          
    U8 dmaLenAlignShift;           
    U8 isDirectEnable:1;           
    U8 isHidden:1;                 
    U8 isNvme:1;                   
    U8 isSsd:1;                    
    U8 bdev_bdi_cap:2;             
    U8 isWriteDirectEnable:1;      
    U8 reserved2:1;
    U8 raidLevel;                  
    U8 spanCount;                  
    U8 diskState;                  
    U16 umapBlkDescCnt:3;          
    U16 umapNumblk:13;             
    U16 dev_busy_scale; 		
    U64 startLBA;                  
    U64 extentSize;                
    U64 mapBlock;                  
    U64 capacity;                  
    U8 isTaskMgmtEnable;           
    U8 taskAbortTimeout;           
    U8 taskResetTimeout;           
    U8 mapBlockVer;                
    U32 maxIOSize;                 
    U32 devQueDepth;               
    U16 virtDiskSeq;               
    U16 normalQuota;               
    U16 directQuota;               
    U16 reserved4[21];
    struct PS3Span span[PS3_MAX_SPAN_IN_VD];   

};

struct PS3VDInfo {
    U16 count;                     
    U8 reserved[6];
    struct PS3VDEntry vds[0];      
};

struct PS3DrvSysInfo {
    U8 version;
    U8 systemIDLen;
    U8 reserved[6];
    U8 systemID[PS3_DRV_SYSTEM_ID_MAX_LEN];
};


struct PS3PhyInfo {
    U64 sasAddr;                       
    U64 attachedSasAddr;               
    U8 phyId;                          
    U8 negLinkRate;                    
    U8 slotId;                         
    U8 attachDevType;                  
    U8 initiatorPortProtocol:4;        
    U8 targetPortProtocols:4;          
    U8 attachInitiatorPortProtocol:4;  
    U8 attachTargetPortProtocols:4;    
    U8 minLinkRateHw:4;                
    U8 maxLinkRateHw:4;                
    U8 minLinkRate:4;                  
    U8 maxLinkRate:4;                  
    U8 enable:1;                       
    U8 reserve:7;
    U8 reserved[7];
};


struct PS3ExpanderInfo {
    U64 sasAddr;        
    U64 parentSasAddr;  
    U8 parentId;        
    U8 enclID;          
    U8 devType;         
    U8 phyCount;        
    U8 reserved[4];
};


struct PS3Expanders {
    U8 count;                           
    U8 reserved[7];
    U64 hbaSasAddr[3];                  
    struct PS3ExpanderInfo expanders[0];
};

struct PS3BiosInfo {
    U8 biosState;                       
    U8 biosMode;                        

    U8 biosAbs;
    U8 devMaxNum;                       
};

struct PS3BootDriveInfo {
    U8        hasBootDrive        :1;
    U8        isPD                :1;
    U8        reserved_9          :6;
    U8        enclID  ;
    U16       slotID  ;
    U16       vdID    ;
    U8        pad[2];
};

struct PS3RandomInfo {
    U8 randomNum[PS3_MAX_RANDOM_NUM];
    U8 iv[PS3_MAX_IV_NUM];
};

struct PS3SecurityPwHead {
    U8    cipherNum;
    U32   cipherLegth[PS3_SECURITY_CIPHER_NUM_MAX];
    U32   cipherOffset[PS3_SECURITY_CIPHER_NUM_MAX];
    U8    iv[PS3_MAX_IV_NUM];
};

#endif 

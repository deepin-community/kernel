
#ifndef _PS3_HTP_REQFRAME_H_
#define _PS3_HTP_REQFRAME_H_

#include "ps3_htp_def.h"
#include "ps3_htp_dev.h"
#include "ps3_htp_sas.h"
#include "ps3_htp_req_frame_hw.h"

enum {
    PS3_FRAME_SGE_BUFSIZE       = 4096,
    PS3_FRAME_SGE_SHIFT         = 12,  
    PS3_FRAME_REQ_SGE_NUM_FE    = 8,   
    PS3_FRAME_REQ_PRP_NUM_FE    = 2,   
    PS3_FRAME_REQ_SGE_NUM_HW    = 8,   
    PS3_FRAME_REQ_PRP_NUM_HW    = 2,   
    PS3_FRAME_REQ_SGE_NUM_MGR   = 11,  
    PS3_FRAME_REQ_EXT_SGE_MIN   = 2,   
    PS3_FRAME_CDB_BUFLEN        = 32,  
    PS3_FRAME_LUN_BUFLEN        = 8,   
    PS3_DEBUG_MEM_ARRAY_MAX_NUM = 16,  
    PS3_MAX_DMA_MEM_SIZE        = 4096, 
    PS3_MAX_DEBUG_MEM_SIZE_PARA = 65536,
    PS3_DRV_NAME_MAX_LEN        = 32,   
    PS3_DRV_VERSION_MAX_LEN     = 24,   
};

enum {
    PS3_DATA_DIRECTION_WRITE = 0,     
    PS3_DATA_DIRECTION_READ  = 1,     
};

enum {
        PS3_REQFRAME_FORMAT_FRONTEND = 0,      
        PS3_REQFRAME_FORMAT_SAS = 1,    
        PS3_REQFRAME_FORMAT_SATA = 2,   
        PS3_REQFRAME_FORMAT_NVME = 3,   
};

enum {
    PS3_LINUX_FRAME        = 0,  
    PS3_WINDOWS_FRAME      = 1,  
};

enum {
    PS3_COMPAT_VER_DEFAULT      = 0,   		
    PS3_COMPAT_VER_1            = 1,    	
    PS3_COMPAT_VER_MAX    	    = 0xffff,   
};

typedef struct PS3Sge {
    U64 addr;              
    U32 length;            
    U32 reserved1 : 30;
    U32 lastSge   : 1;     
    U32 ext       : 1;     
} PS3Sge_s;

typedef struct PS3Prp {
    U64 prp1;             
    U64 prp2;             
} PS3Prp_s;



typedef struct PS3SoftwareZone {
    U64 virtDiskLba;       
    U32 numBlocks;         
    U8 opcode;             
    U8 sglOffset;          
    U8 sglFormat   : 2;    
    U8 isResendCmd : 1;    
    U8 reserved1   : 5;
    U8 reserved2;
    U16 subOpcode;         
    U16 sgeCount    : 9;   
    U16 reserved3   : 7;
    U8 reserved4[4];
} PS3SoftwareZone_s;

typedef struct PS3ReqFrameHead
{
    U8  cmdType;           
    U8  cmdSubType;        
    U16 cmdFrameID;        
    union{
        struct {
            U32 noReplyWord    : 1;  
            U32 dataFormat     : 1;  
            U32 reqFrameFormat : 2;  
            U32 mapBlockVer    : 2;  
            U32 isWrite        : 1;  
            U32 isStream1      : 1;	 
            U32 reserved       : 24;
        };
        U32 control;
    };
    PS3DiskDev_u devID;     
    U16 timeout;            
    U16 virtDiskSeq;        
    U16 reserved1[4];       
    U64 traceID;            
} PS3ReqFrameHead_s;

typedef struct PS3HwReqFrame {
    PS3ReqFrameHead_s reqHead;     
    PS3SoftwareZone_s softwareZone;
    U8 reserved[8];
    union {
        IODT_V1_s sasReqFrame;     
        PS3NvmeReqFrame_u nvmeReqFrame;    
    };
    PS3Sge_s sgl[PS3_FRAME_REQ_SGE_NUM_FE];
} PS3HwReqFrame_s;

typedef struct PS3VDAccAttr {
    U64 firstPdStartLba;   
    U8  firstSpanNo;       
    U8  fisrtSeqInSpan;    
    U8  secondSeqInSapn;   
    U8  thirdSeqInSapn;    
    U8  clineCount;        
    U8  isAccActive : 1;   
    U8  isStream    : 1;   
    U8  reserved1   : 6;
	U16 ioOutStandingCnt;	   
    U8  reserved2[16];
} PS3VDAccAttr_s;


typedef struct PS3FrontEndReqFrame {
    PS3ReqFrameHead_s reqHead;     
    U8  cdb[PS3_FRAME_CDB_BUFLEN]; 
    PS3VDAccAttr_s vdAccAttr;      
    U32 dataXferLen;               
    U8  reserved[25];              
    U8  sgeOffset;                 
    U16 sgeCount;                  
    union {
        PS3Sge_s sgl[PS3_FRAME_REQ_SGE_NUM_FE];  
        PS3Prp_s prp;                             
    };
} PS3FrontEndReqFrame_s;

struct PS3MgrDev {
    struct PS3DiskDevPos devID;
    U16 num;
    U8  devType;                       
    U8  reserved[17];
};

struct PS3MgrEvent {
    U32 eventTypeMap;                  
    U32 eventTypeMapProcResult;        
    U32 eventLevel;                    
    U8  reserved[20];
};


struct PS3SasMgr {
    U64 sasAddr;       
    U8 enclID;         
    U8 startPhyID;     
    U8 phyCount;       
    U8 reserved1;
    U16 reqLen;        
    U8 reserved2[2];
};


struct PS3SasPhySet {
    U64 sasAddr;        
    U8 phyID;           
    U8 minLinkRate;     
    U8 maxLinkRate;     
    U8 phyCtrl;         
    U8 reserved[3];
};

union PS3MgrReqDiffValue {
    U8  word[32];
    U16 originalCmdFrameID;            
    U8 eventStart;                     
    struct PS3MgrDev dev;
    struct PS3MgrEvent event;
    struct PS3SasMgr sasMgr;
    struct PS3SasPhySet phySet;
    U8 unLoadType;                    
    BOOL isRetry;                     
};


typedef struct PS3MgrReqFrame      {
    PS3ReqFrameHead_s reqHead;         
    U16 sgeCount;                      
    U8  sgeOffset;                     
    U8  syncFlag;                      
    U16 timeout;                       
    U8  abortFlag;                     
    U8  pendingFlag;                   
    union PS3MgrReqDiffValue value;    
    U8 osType;                         
    U8  suspend_type;
    U8  reserved[6];
    struct PS3Sge sgl[PS3_FRAME_REQ_SGE_NUM_MGR];  
}PS3MgrReqFrame_s;


typedef struct PS3MgrTaskReqFrame {
    PS3ReqFrameHead_s reqHead;         
    U16 taskID;                        
    U8  lun[PS3_FRAME_LUN_BUFLEN];     
    U8  abortedCmdType;                
    U8  reserved[5];
} PS3MgrTaskReqFrame_s;


union PS3ReqFrame {
    PS3MgrTaskReqFrame_s  taskReq;     
    PS3MgrReqFrame_s      mgrReq;      
    PS3FrontEndReqFrame_s frontendReq; 
    PS3HwReqFrame_s       hwReq;       
    U8 word[256];                      
};


typedef struct PS3DrvInfo {
    char drvName[PS3_DRV_NAME_MAX_LEN];      
    char drvVersion[PS3_DRV_VERSION_MAX_LEN];
    U64  bus;                                
    U8   dev:5;                              
    U8   func:3;                             
    U8   domain_support:1;                   
    U8   reserved:7;
    U16  compatVer;                          
    U32  domain;                             
    U8   reserved1[56];
} PS3DrvInfo_s;


enum {
    PS3_MEM_TYPE_UNKNOWN = 0,
    PS3_MEM_TYPE_SO      = 1,
    PS3_MEM_TYPE_RO      = 2,
};


typedef struct PS3HostMemInfo {
    U64 startAddr;    
    U64 endAddr;      
    U8  type;         
    U8  reserved[7];
} PS3HostMemInfo_s;

struct PS3InitReqFrame {
    PS3ReqFrameHead_s reqHead; 
    U8  ver;                   
    U8 reserved0;
    U16  length;                
    U8  operater;              
    U8  pageSize;              
    U8 pciIrqType;             
    U8 osType;                 
    U8 reserved1[6];
    U16 msixVector;            
    U64 timeStamp;             
    U64 reqFrameBufBaseAddr;   
    U64 hostMemInfoBaseAddr;   
    U32 hostMemInfoNum;        
    U8 reserved2[20];

    U64 replyFifoDescBaseAddr;

    U64 respFrameBaseAddr;
    U32 eventTypeMap;          
    U16 reqFrameMaxNum;        
    U16 respFrameMaxNum;       
    U64 filterTableAddr;       
    U32 filterTableLen;        
    U16 bufSizePerRespFrame;   
    U8 hilMode;      
    U8 reserved3[33];
    U64 systemInfoBufAddr;
    U64 debugMemArrayAddr;
    U32 debugMemArrayNum;      
    U32 dumpDmaBufLen;
    U64 dumpDmaBufAddr;
    U32 dumpIsrSN;
    U16 drvInfoBufLen;         
    U8 reserverd4[2];
    U64 drvInfoBufAddr;        
    U8 reserved5[36];
    U32 respStatus;            
};

#endif     

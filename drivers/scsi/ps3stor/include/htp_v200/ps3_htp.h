
#ifndef _PS3_HTP_H_
#define _PS3_HTP_H_

#include "ps3_htp_def.h"
#include "ps3_htp_dev.h"
#include "ps3_htp_req_frame_hw.h"

#define PS3_DUMP_CTRL_COPY_FINISH	  0x1
#define PS3_DUMP_CTRL_DUMP_ABORT	  0x2
#define PS3_DUMP_CTRL_DUMP_FW_LOG         0x3
#define PS3_DUMP_CTRL_DUMP_BAR_DATA       0x4
#define PS3_DUMP_CTRL_DUMP_CORE_FILE      0x5  
#define PS3_DUMP_CTRL_DUMP_END            0x6  
#define PS3_DUMP_CTRL_DUMP_INT_READY      0x7  
#define PS3_DUMP_DMA_DONE                (0x1)
#define PS3_DUMP_DMA_ABORT               (0x1 << 6)
#define PS3_DUMP_DATA_UNIT_SIZE          (0x400)
#define PS3_DREICT_SENSE_DATA_BUF_SIZE    72   

#define PS3_ATU_FLAG_LOW_BITS_MASK	  (0x0000FFFF)
#define PS3_ATU_FLAG_HIGH_BITS_MASK	  (0xFFFFFFFFFFFF0000)
#define PS3_ATU_FLAG_DRIVER_SET 	  (0xC0DE)

#define PS3_IOCTL_VERSION (0x2000000)

typedef enum{
    HIL_MODEL_SW = 0,      
    HIL_MODEL_HW,          
    HIL_MODEL_HW_ENHANCED, 
    HIL_MODEL_SW_ASSIST,   
}hilModel_e;

enum Ps3DumpType{
	PS3_DUMP_TYPE_UNKNOWN	= 0,
	PS3_DUMP_TYPE_CRASH     = 1,       
	PS3_DUMP_TYPE_FW_LOG    = 2,       
	PS3_DUMP_TYPE_BAR_DATA  = 3,       
};

enum Ps3DumpState {
	PS3_DUMP_STATE_INVALID	 = 0,      
	PS3_DUMP_STATE_PRE_ABORT,          
	PS3_DUMP_STATE_ABORTED,            
	PS3_DUMP_STATE_START,              
	PS3_DUMP_STATE_COPYING,            
	PS3_DUMP_STATE_COPY_DONE,          
	PS3_DUMP_STATE_READY = 7,          
};
enum Ps3CtrlSecurityState {
	PS3_CTRL_SECURITY_STATE_DECRYPT = 0,   
	PS3_CTRL_SECURITY_STATE_ENCRYPT,       
};

struct Ps3DumpNotifyInfo {
	S32 dumpType;                     
};


struct PS3LinkErrInfo {
    U32 invalidDwordCount;
    U32 runningDisparityErrCount;
    U32 lossOfDwordSyncCount;
    U32 phyResetProblemCount;
};

enum PhyCtrl {
    PS3_SAS_CTRL_UNKNOWN = 0,
    PS3_SAS_CTRL_RESET = 1,
    PS3_SAS_CTRL_RESET_HARD = 2,
    PS3_SAS_CTRL_DISABLE = 3
};


enum {
	PS3_UNLOAD_SUB_TYPE_RESERVED = 0,
	PS3_UNLOAD_SUB_TYPE_REMOVE   = 1,
	PS3_UNLOAD_SUB_TYPE_SHUTDOWN = 2,
	PS3_UNLOAD_SUB_TYPE_SUSPEND  = 3,
};


enum {
	PS3_SUSPEND_TYPE_NONE  = 0,
	PS3_SUSPEND_TYPE_SLEEP     = 1,
	PS3_SUSPEND_TYPE_HIBERNATE = 2,
};

static inline const S8 * namePhyCtrl(enum PhyCtrl e)
{
    static const S8 *myNames[] = {
    [PS3_SAS_CTRL_UNKNOWN]       = "PS3_SAS_CTRL_UNKNOWN",
    [PS3_SAS_CTRL_RESET]         = "PS3_SAS_CTRL_RESET",
    [PS3_SAS_CTRL_RESET_HARD]    = "PS3_SAS_CTRL_RESET_HARD",
    [PS3_SAS_CTRL_DISABLE]       = "PS3_SAS_CTRL_DISABLE"
    };

    return myNames[e];
}


struct PS3InitCmdWord {
    union {
        struct {
            U32 type        : 2;    
            U32 reserved1   : 1;
            U32 direct      : 2;    
            U32 reserved2   : 27;   
        };
        U32 lowAddr;
    };

    U32 highAddr;              
};

typedef struct PS3CmdWord {
    U16 type        : 2;       
    U16 reserved1   : 2;
    U16 direct      : 2;       
    U16 isrSN       : 8;       
    U16 reserved2   : 2;
    U16 cmdFrameID  : 13;      
    U16 reserved3   : 3;
    U16 phyDiskID   : 12;      
    U16 reserved4   : 4;
    U16 virtDiskID  : 8;       
    U16 reserved5   : 4;
    U16 qMask       : 4;
} PS3CmdWord_s;


struct PS3CmdWordSw {
    U32 type        : 2;       
    U32 noReplyWord : 1;       
    U32 cmdFrameID  : 13;      
    U32 isrSN       : 8;       
    U32 cmdIndex    : 8;       
};


union PS3CmdWordU32 {
	struct PS3CmdWordSw cmdWord;
	U32 val;
};


union PS3DefaultCmdWord {
    struct PS3CmdWord cmdWord;
    union {
        struct{
            U32 low;           
            U32 high;          
        } u;
        U64 words;
    };
};

enum{
    PS3_ISR_ACC_MODE_LATENCY = 0,
    PS3_ISR_ACC_MODE_SSD_IOPS,   
    PS3_ISR_ACC_MODE_HDD_IOPS,   
	PS3_ISR_ACC_MODE_IOPS_VER0 = 2,  
	PS3_ISR_ACC_MODE_DEV_IOPS,   
    PS3_ISR_ACC_MODE_MAX,
};
struct PS3ReplyFifoDesc {
    U64 ReplyFifoBaseAddr;     
    U32 irqNo;                 
    U16 depthReplyFifo;        
    U8 isrAccMode;             
    U8 reserved;
};

typedef struct PS3ReplyWord {
    U16 type        : 2;      
    U16 diskType    : 1;      
    U16 reserved1   : 1;
    U16 mode        : 2;
    U16 reserved2   : 10;
    U16 cmdFrameID  : 13;     
    U16 reserved3   : 2;
    U16 reserved4   : 1;      
    U16 retStatus   : 15;     
    U16 retType     : 1;
    U16 reserved5  : 12;
    U16 qMask       : 4;
} PS3ReplyWord_s;


struct PS3MgrTaskRespInfo {
    U8  iocStatus;             
    U8  reserved1;
    U16 iocLogInfo;            
    U32 terminationCnt;        
    U32 respInfo;              
    U32 reserved2;
};


struct PS3MgrCmdReplyRespInfo {
    U8  cmdReplyStatus;        
    U8  reserved[15];
};


union PS3RespDetails {
    U32 xfer_cnt;                              
    U32 respData[4];                           
    struct PS3MgrTaskRespInfo  taskMgrRespInfo;
    struct PS3MgrCmdReplyRespInfo replyCmdRespInfo; 
};


typedef struct PS3SasDirectRespStatus {
    U32 status:8;      
    U32 dataPres:2;    
    U32 reserved:22;
} PS3SasStatus_s;


typedef struct Ps3SasDirectRespFrameIU {
    union {
        U8 reserved0[8];
        U64 mediumErrorLba;    
    };
    U8 reserved1[2];
    U8 dataPres;              
    U8 status;                
    union {
        U32 reserved2;
        U32 xfer_cnt;              
    };
    U32 senseDataLen;
    U32 respDataLen;         
    U8 data[PS3_SENSE_BUFFER_SIZE];
    U8 reserved3[8];
} Ps3SasDirectRespFrameIU_s;


typedef struct PS3NormalRespFrame {
    union PS3RespDetails respDetail;
    U8  reserved1[8];
    U8  sense[PS3_SENSE_BUFFER_SIZE];  
    U8  type;
    U8  reserved2[3];
    U8  respStatus;
    U8  dataPre;              
    U8  reserved3[2];
} PS3NormalRespFrame_s;

typedef union PS3RespFrame{
    Ps3SasDirectRespFrameIU_s sasRespFrame;
    PS3NormalRespFrame_s normalRespFrame;
}PS3RespFrame_u;


typedef struct Ps3DebugMemEntry {
    U64 debugMemAddr;         
    U32 debugMemSize;         
    U32 reserved;
} Ps3DebugMemEntry_s;


typedef struct PS3NvmeCmdStatus {
    union {
        struct {
            U16 sc  : 8;     
            U16 sct : 3;     
            U16 crd : 2;     
            U16 m   : 1;     
            U16 dnr : 1;     
            U16 p   : 1;     
        };
        U16 cmdStatus;
    };
} PS3NvmeCmdStatus_s;

#endif     

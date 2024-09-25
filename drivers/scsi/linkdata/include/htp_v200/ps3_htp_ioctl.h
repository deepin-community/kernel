
#ifndef _PS3_HTP_IOCTL_H_
#define _PS3_HTP_IOCTL_H_

#include "ps3_htp.h"
#include "ps3_htp_reqframe.h"



#define PS3_MAX_IOCTL_SGE_NUM           16
#define PS3_IOCTL_SENSE_SIZE            96
#define PS3_IOCTL_MAX_FRAME_SIZE        128

#ifndef PS3_SCSI_HOST_PROC_NAME
#define PS3_SCSI_HOST_PROC_NAME  "ps3stor"
#endif

#define PS3_PRODUCT_MODEL  "ps3stor"

#ifndef PS3_SCSI_HOST_PROC_NAME_V100
#define PS3_SCSI_HOST_PROC_NAME_V100  "ps3"
#endif
#define PS3_PRODUCT_MODEL_V100  "ps3"

#define PS3_PSW_PRODUCT_MODEL  "psw"

struct PS3CmdIoctlHeader {
    U8  cmdType;
    U8  version;
    U16 deviceId;
    U16 cmdSubType;        
    U8  cmdResult;
    U8  sglOffset;         
    U16 index;
    U16 control;           
    U32 sgeCount;
    U16 timeout;
    U8  sglChainOffset;    
    U8  syncFlag;
    U32 abortCmdFrameId;
};

union PS3IoctlFrame {
    U8  value[PS3_IOCTL_MAX_FRAME_SIZE];
    struct PS3CmdIoctlHeader header;
};

#ifdef _WINDOWS

#define PS3_IOCTL_SIG "ps3stor"
#define PS3_IOCTL_FUNCTION 0x801
#define PS3_DEBUG_CLI_FUNCTION 0x802

#define PS3_CTL_CODE CTL_CODE(FILE_DEVICE_MASS_STORAGE, PS3_IOCTL_FUNCTION, \
	METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define PS3_DBG_CLI_CODE CTL_CODE(FILE_DEVICE_MASS_STORAGE, PS3_DEBUG_CLI_FUNCTION, \
	METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

struct PS3IoctlSyncCmd {
    U16				hostId;
    U16				sglOffset; 
    U16				sgeCount;
    U16				reserved;
    U64				traceId;
    struct PS3Sge		Sgl[PS3_MAX_IOCTL_SGE_NUM];
    UCHAR			data[0];
};

typedef struct _PS3_IO_CONTROL {
    SRB_IO_CONTROL	SrbHeader;
    ULONG               reserved;
    struct PS3IoctlSyncCmd ps3Ioctl;
}PS3_IO_CONTRL, * PPS3_IO_CONTROL;
#else

#define PS3_CMD_IOCTL_SYNC_CMD _IOWR('M', 1, struct PS3IoctlSyncCmd)

#ifndef __WIN32__
#define PS3_EVENT_NOTICE_SIG (SIGRTMIN+7)
enum{
    PS3_IOCTL_CMD_NORMAL = 0,
    PS3_IOCTL_CMD_WEB_SUBSCRIBE,
};

struct PS3IoctlSyncCmd {
    U16 hostId;
    U16 sglOffset;     
    U16 sgeCount;      
    U16 reserved1;
    U32 resultCode;    
    U8  reserved2[4];
    U8  sense[PS3_IOCTL_SENSE_SIZE];
    U64 traceId;       
    U8  reserved3[120];
    union PS3IoctlFrame msg;
    struct PS3Sge sgl[PS3_MAX_IOCTL_SGE_NUM];
};
#endif

struct PS3IoctlAsynCmd {
    U16 hostId;
    U16 reserved1;

    U32 seqNum;

    U16 eventLevel;

    U16 eventType;
    U8  reserved2[4];
};

#endif

#endif 

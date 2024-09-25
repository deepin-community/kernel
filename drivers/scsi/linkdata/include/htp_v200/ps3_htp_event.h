
#ifndef _PS3_HTP_EVENT_H_
#define _PS3_HTP_EVENT_H_

#include "ps3_htp_def.h"
#include "ps3_htp_dev.h"
#include "ps3_mgr_evt.h"
#include "ps3_evtcode_trans.h"

#define PS3_EVENT_DETAIL_BUF_MAX    (20)   


enum PS3EventLevel {
    PS3_EVENT_LEVEL_INFO,      
    PS3_EVENT_LEVEL_WARN,      
    PS3_EVENT_LEVEL_CRITICAL,  
};

#if 0

MgrEvtType_e {
    PS3_EVENT_TYPE_NONE             = 0X00000000,      

    PS3_EVENT_TYPE_PD_COUNT_CHANGE  = 0X00000001,

    PS3_EVENT_TYPE_VD_COUNT_CHANGE  = 0X00000002,
    PS3_EVENT_TYPE_CTRL_INFO_CHANGE = 0X00000004,      
    PS3_EVENT_TYPE_PD_ATTR_CHANGE   = 0X00000008,      
    PS3_EVENT_TYPE_VD_ATTR_CHANGE   = 0X00000010,      
    PS3_EVENT_TYPE_OTHER            = 0X00000020,      
    PS3_EVENT_TYPE_ALL              = 0XFFFFFFFF,
};
#endif


struct PS3EventDetail {
    U32 eventCode;                     
    U32 timestamp;                     
    MgrEvtType_e  eventType;           
    union {
        struct PS3DiskDevPos devicePos;
        U8 EnclId;                     
    };
};

struct PS3EventInfo {
    U32 eventTypeMap;      
    U32 eventCount;        

    struct PS3EventDetail  eventDetail[PS3_EVENT_DETAIL_BUF_MAX];
    U8 reserved[8];
};
#if 0

enum PS3EventCode {
    PS3_EVENT_CODE_NONE             = 0X00000000,      

    PS3_EVENT_CODE_PD_ADD           = 0X00000001,
    PS3_EVENT_CODE_PD_DEL           = 0X00000002,
    PS3_EVENT_CODE_PD_CHANGE        = 0X00000003,
    PS3_EVENT_CODE_SES_ADD          = 0X00000004,
    PS3_EVENT_CODE_SES_DEL          = 0X00000005,
    PS3_EVENT_CODE_PD_COUNT_CHANGE  = 0X00000006,

    PS3_EVENT_CODE_VD_ADD           = 0X00000011,
    PS3_EVENT_CODE_VD_DEL           = 0X00000012,
    PS3_EVENT_CODE_VD_COUNT_CHANGE  = 0X00000013,

    PS3_EVENT_CODE_PD_ATTR_CHANGE   = 0X00000021,

    PS3_EVENT_CODE_PD_INFO_CHANGE   = 0X00000022,

    PS3_EVENT_CODE_OTHER            = 0X00000040,
};
#endif
#endif 

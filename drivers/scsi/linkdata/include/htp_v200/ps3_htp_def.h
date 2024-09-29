
#ifndef _PS3_HTP_DEF_H_
#define _PS3_HTP_DEF_H_

#include "ps3_types.h"

typedef unsigned char           Bool;




#define PCIE_DMA_HOST_ADDR_BIT_POS (44)

#define PCIE_DMA_HOST_ADDR_BIT_POS_SET(addr) \
	(((1ULL) << (PCIE_DMA_HOST_ADDR_BIT_POS)) | (addr))

#define PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR(addr) \
	((~((1ULL) << (PCIE_DMA_HOST_ADDR_BIT_POS))) & (addr))

#define PCIE_DMA_HOST_ADDR_BIT_POS_F0 (54)
#define PCIE_DMA_HOST_ADDR_BIT_POS_F1 (53)
#define PCIE_DMA_HOST_ADDR_BIT_POS_VALID (52)

#define PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(bit_pos, addr) \
	((addr) + ((1ULL) << (bit_pos)))

#define PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, addr) \
	((addr) - ((1ULL) << (bit_pos)))


enum PS3FWDiagKey {
    PS3_FW_DIAG_FLUSH   = 0X00,    
    PS3_FW_DIAG_1ST_KEY = 0x52,    
    PS3_FW_DIAG_2ND_KEY = 0x5F,    
    PS3_FW_DIAG_3RD_KEY = 0x55,    
    PS3_FW_DIAG_4TH_KEY = 0x5F,    
    PS3_FW_DIAG_5TH_KEY = 0x52,    
    PS3_FW_DIAG_6TH_KEY = 0x45,    
    PS3_FW_DIAG_7TH_KEY = 0x41,    
    PS3_FW_DIAG_8TH_KEY = 0x44,    
    PS3_FW_DIAG_9TH_KEY = 0x59,
};


enum PS3FWStateAct {
    PS3_FW_STATE_ACT_INIT_READY     = 0X00000001,  
};


enum PS3RegDoorBellType {
    PS3_REG_DOORBELL_STATE_TO_READY = 1, 
    PS3_REG_DOORBELL_STATE_TO_FAULT = 2, 
    PS3_REG_DOORBELL_STATE_TO_HALT  = 3, 
};


enum PS3FWSoftResetAct {
    PS3_FW_STATE_ACT_SHALLOW_SOFT_RESET = 0X00000001,
    PS3_FW_STATE_ACT_DEEP_SOFT_RESET    = 0X00000002,
};

enum PS3PerfModeType {
    PS3_PERF_MODE_BALANCE   = 0,
    PS3_PERF_MODE_IOPS      = 1,
    PS3_PERF_MODE_LATENCY   = 2,
};


enum PS3BitPos {
    PS3_BIT_POS_DEFAULT = 0,
    PS3_BIT_POS_44     = 1,
    PS3_BIT_POS_45     = 2,
    PS3_BIT_POS_46     = 3,
    PS3_BIT_POS_47     = 4,
    PS3_BIT_POS_48     = 5,
    PS3_BIT_POS_49     = 6,
    PS3_BIT_POS_50     = 7,
    PS3_BIT_POS_51     = 8,
    PS3_BIT_POS_52     = 9,
    PS3_BIT_POS_53     = 10,
    PS3_BIT_POS_54     = 11,
};


enum PS3CmdType {
    PS3_CMD_INIT_IOC                = 0x0,
    PS3_CMD_VD_SCSI_IO_RW           = 0x1,
    PS3_CMD_VD_SCSI_IO_NORW         = 0x2,
    PS3_CMD_PD_SCSI_IO_RW           = 0x3,
    PS3_CMD_PD_SCSI_IO_NORW         = 0x4,
    PS3_CMD_MANAGEMENT              = 0x5,
    PS3_CMD_SCSI_TASK_MANAGEMENT    = 0x6,
    PS3_CMD_IOCTL                   = 0x7,
    PS3_CMD_SAS_MANAGEMENT          = 0x8,
    PS3_CMD_COUNT                   = 0x9,
    PS3_CMD_INVALID                 = 0xff,
};

enum {
        PS3_SGL  = 0x0,
        PS3_PRP  = 0x1,
};

enum {
    PS3_CMD_OPERATOR_TYPE_HOST  = 0x0,
    PS3_CMD_OPERATOR_TYPE_UEFI  = 0x1,
    PS3_CMD_OPERATOR_TYPE_IOC   = 0x2,
};


enum {
    PS3_PCI_IRQ_LEGACY = 0,
    PS3_PCI_IRQ_MSI = 1,
    PS3_PCI_IRQ_MSIX = 2,
};

#define PS3_DRV_SYSTEM_ID_MAX_LEN   64
#define PS3_SENSE_BUFFER_SIZE	   (96)
#define PS3_RESP_FRAME_BUFFER_SIZE  (128)


#define PS3_REQUEST_CONTROL_DIR_NONE    0x00
#define PS3_REQUEST_CONTROL_DIR_READ    0x01
#define PS3_REQUEST_CONTROL_DIR_WRITE   0x02
#define PS3_REQUEST_CONTROL_DIR_BOTH    0x03


#define PS3_MAX_PD_COUNT_IN_SPAN        32
#define PS3_MAX_SPAN_IN_VD              8

#define PS3_IOC_INIT_STATE_MASK     0xFFFF
#define PS3_IOC_RECOVERY_COUNT_MASK     0xFFFFFFFF


#define PS3_START_STATE_SPACE       0x0100    


#define PS3_DRV_MGR_FLUSH_RETRY_MAX_COUNT 1


#define  MAX_MGR_CMD_TOTAL_COUNT	(16)

#define PS3_HOT_RESET_OFFSET       (HIL_REG1_PS3_REGISTER_F_PS3_DEBUG10_ADDR - HIL_REG1_PS3_REGISTER_F_BASEADDR)

typedef union HilRegPs3RegisterHotReset{
    U64 val;
    struct{

        U8 isHotReset             : 1;   
        U8 reserved0              : 7;   
        U8 reserved1[7]              ;   
    }reg;
}HilRegPs3RegisterHotReset_u;


#define PS3_ATU_SUPPORT_OFFSET    (HIL_REG1_PS3_REGISTER_F_PS3_DEBUG8_ADDR - HIL_REG1_PS3_REGISTER_F_BASEADDR)

typedef union HilRegPs3RegisterFPs3AtuSupport{
    volatile U64 val;
    struct{

        U8 bitPos                    ;   
        U8 reserved[7]               ;   
    }reg;
}HilRegPs3RegisterFPs3AtuSupport_u;


#define PS3_CAN_HARD_RESET_OFFSET    (HIL_REG1_PS3_REGISTER_F_PS3_DEBUG9_ADDR - HIL_REG1_PS3_REGISTER_F_BASEADDR)

typedef union HilRegPs3RegisterFPs3CanHardReset{
    volatile U64 val;
    struct{

        U8 canHardReset           : 1;   
        U8 reserved0              : 7;   
        U8 reserved1[7]              ;   
    }reg;
}HilRegPs3RegisterFPs3CanHardReset_u;


typedef enum PS3FWRunState {
    PS3_FW_STATE_UNDEFINED = 0x00,            
    PS3_FW_STATE_START     = 0x01,            
    PS3_FW_STATE_READY     = 0x02,            
    PS3_FW_STATE_WAIT      = 0x03,            
    PS3_FW_STATE_RUNNING   = 0x04,            
    PS3_FW_STATE_FLUSHING  = 0x05,            
    PS3_FW_STATE_RESET     = 0x06,            
    PS3_FW_STATE_CRITICAL  = 0x09,            
    PS3_FW_STATE_FAULT     = 0xE0,            
    PS3_FW_STATE_HALT      = 0xF0,            
    PS3_FW_STATE_END,                         
    PS3_FW_STATE_MASK      = 0xFF,
    PS3_FW_STATE_WDT_MASK  = 0xFF0000FF,      
}PS3FWRunState_e;


typedef enum PS3FWStartState {
    PS3_START_STATE_UNDEFINED     = 0x0000,   
    PS3_START_STATE_INIT_BASE     = 0x0100,   
    PS3_START_STATE_INIT_HARDWARE = 0x0200,   
    PS3_START_STATE_INIT_SOFTWARE = 0x0300,   
    PS3_START_STATE_INIT_DATAPATH = 0x0400,   
    PS3_START_STATE_INIT_THREAD   = 0x0500,   
    PS3_START_STATE_SCAN_DEVICE   = 0x0600,   
    PS3_START_STATE_FLUSH_CACHE   = 0x0700,   
    PS3_START_STATE_INIT_RESET    = 0x0800,   
    PS3_START_STATE_FINISHED      = 0x0900,   
    PS3_START_STATE_MASK          = 0xFF00,
    PS3_START_STATE_WDT_MASK      = 0xFF00FF00,
}PS3FWStartState_e;


#define PS3_FW_RESET_FLAG             (0X00000001) 
#define PS3_FW_DIAG_ENABLE            (0X00000001) 
#define PS3_FW_HARD_RESET_ACT         (0X00000001) 


#define PS3_FW_MAX_CMD_MASK               (0X0000FFFF)
#define PS3_FW_MAX_MSIX_VECTORS_MASK      (0X0000FFFF)
#define PS3_FW_MAX_CHAIN_SIZE_MASK        (0XFFFFFFFF)
#define PS3_FW_MAX_RAID_MAP_SIZE_MASK     (0XFFFFFFFF)
#define PS3_FW_MAX_NVME_PAGE_SIZE_MASK    (0xFFFFFFFF)


#define PS3_FW_INTERRUPT_STATUS_MASK          (0X00000001)
#define PS3_FW_INTERRUPT_CMD_INTR_CAP_MASK    (0X00000004)
#define PS3_FW_INTERRUPT_CMD_MSI_CAP_MASK     (0X00000002)
#define PS3_FW_INTERRUPT_CMD_MSIX_CAP_MASK    (0X00000001)
#define PS3_FW_INTERRUPT_CLEAR_MASK           (0X00000001)


enum PS3FWFeatureSupportMask {
    PS3_FW_FEATURE_SUPPORT_SYNC_CACHE = 0X00000001,
    PS3_FW_FEATURE_SUPPORT_DMA64      = 0X00000002,
};


enum PS3FWCtrlMask {
    PS3_FW_CTRL_CMD_TRIGGER_SNAPSHOT               = 0X00000001,
    PS3_FW_CTRL_CMD_CRASHDUMP_COLLECTION_DONE      = 0X00000002,
    PS3_FW_CTRL_CMD_CRASHDUMP_DMA_CLEAR            = 0X00000004,
};


enum PS3FWCtrlStatusMask {
    PS3_FW_CTRL_STATUS_CRASHDUMP_DONE              = 0X00000001,
    PS3_FW_CTRL_STATUS_RSVR                        = 0X00000002,
    PS3_FW_CTRL_STATUS_CRASHDUMP_MAP               = 0X00000004,
};


enum PS3CmdTrigger {
    PS3_CMD_TRIGGER_UNLOAD              = 0X0001,
    PS3_CMD_TRIGGER_UNLOAD_SUSPEND      = 0X0002,
};


enum PS3RegCmdState {
    PS3_DOORBELL_DONE              = 0X0001,
};


enum PS3Debug12Mask {
    PS3_DEBUG12__HOT_RESET              = 0X00000001,
};


enum PS3MgrControlFlag {
    PS3_REQUEST_CONTROL_SKIP_REFIRE = 0x0,
    PS3_REQUEST_CONTROL_SENSE32     = 0x1,
    PS3_REQUEST_CONTROL_SENSE64     = 0x2,
};


enum PS3TaskCmdSubType {
    PS3_TASK_CMD_SCSI_TASK_ABORT,
    PS3_TASK_CMD_SCSI_TASK_RESET,
    PS3_TASK_CMD_COUNT,

    PS3_TASK_CMD_INVALID = 0xffff,
};


enum PS3MgrCmdSubType {
    PS3_MGR_CMD_GET_CTRL_INFO   = 0x0,
    PS3_MGR_CMD_UNLOAD,
    PS3_MGR_CMD_SUBSCRIBE_EVENT,
    PS3_MGR_CMD_GET_VD_LIST,
    PS3_MGR_CMD_GET_PD_LIST,
    PS3_MGR_CMD_GET_VD_INFO,
    PS3_MGR_CMD_GET_PD_INFO,
    PS3_MGR_CMD_GET_BOOTDRIVE_INFO,
    PS3_MGR_CMD_GET_BIOS_INFO,

    PS3_MGR_CMD_GET_SNAPSHOT_ATTR,
    PS3_MGR_CMD_SET_CRASH_DUMP,
    PS3_MGR_CMD_CANCEL,
    PS3_MGR_CMD_ABORT,
    PS3_MGR_CMD_DEV_ADD_ACK,
    PS3_MGR_CMD_DEV_DEL_DONE,

    PS3_SAS_SMP_REQUEST,
    PS3_SAS_GET_LINK_ERR,
    PS3_SAS_PHY_CTRL,
    PS3_SAS_GET_EXPANDERS,
    PS3_SAS_GET_PHY_INFO,
    PS3_SAS_GET_EXPANDER_INFO,

    PS3_MGR_CMD_AUTODUMP_NOTIFY,

    PS3_MGR_CMD_SECURITY_RANDOM_GET,
    PS3_MGR_CMD_SECURITY_PASSWORD,
    PS3_MGR_CMD_WEBSUBSCRIBE_EVENT,

    PS3_MGR_CMD_PRESERVED_INFO_GET,
    PS3_MGR_CMD_GET_PD_SN_LIST,
    PS3_MGR_CMD_PD_REF_CLEAR,
    PS3_MGR_CMD_COUNT,
    PS3_MGR_CMD_INVALID = 0xff
};

static inline const S8 * namePS3MgrCmdSubType(enum PS3MgrCmdSubType type)
{
    static const S8 *myNames[] = {
    [PS3_MGR_CMD_GET_CTRL_INFO]     = "PS3_MGR_CMD_GET_CTRL_INFO",
    [PS3_MGR_CMD_UNLOAD]            = "PS3_MGR_CMD_UNLOAD",
    [PS3_MGR_CMD_SUBSCRIBE_EVENT]   = "PS3_MGR_CMD_SUBSCRIBE_EVENT",
    [PS3_MGR_CMD_GET_VD_LIST]       = "PS3_MGR_CMD_GET_VD_LIST",
    [PS3_MGR_CMD_GET_PD_LIST]       = "PS3_MGR_CMD_GET_PD_LIST",
    [PS3_MGR_CMD_GET_VD_INFO]       = "PS3_MGR_CMD_GET_VD_INFO",
    [PS3_MGR_CMD_GET_PD_INFO]       = "PS3_MGR_CMD_GET_PD_INFO",
    [PS3_MGR_CMD_GET_BOOTDRIVE_INFO] = "PS3_MGR_CMD_GET_BOOTDRIVE_INFO",
    [PS3_MGR_CMD_GET_BIOS_INFO]      = "PS3_MGR_CMD_GET_BIOS_INFO",

    [PS3_MGR_CMD_GET_SNAPSHOT_ATTR] = "PS3_MGR_CMD_GET_SNAPSHOT_ATTR",
    [PS3_MGR_CMD_SET_CRASH_DUMP]    = "PS3_MGR_CMD_SET_CRASH_DUMP",
    [PS3_MGR_CMD_CANCEL]            = "PS3_MGR_CMD_CANCEL",
    [PS3_MGR_CMD_ABORT]             = "PS3_MGR_CMD_ABORT",
    [PS3_MGR_CMD_DEV_ADD_ACK]       = "PS3_MGR_CMD_DEV_ADD_ACK",
    [PS3_MGR_CMD_DEV_DEL_DONE]      = "PS3_MGR_CMD_DEV_DEL_DONE",

    [PS3_SAS_SMP_REQUEST]           = "PS3_SAS_SMP_REQUEST",
    [PS3_SAS_GET_LINK_ERR]          = "PS3_SAS_GET_LINK_ERR",
    [PS3_SAS_PHY_CTRL]              = "PS3_SAS_PHY_CTRL",
    [PS3_SAS_GET_EXPANDERS]         = "PS3_SAS_GET_EXPANDERS",
    [PS3_SAS_GET_PHY_INFO]          = "PS3_SAS_GET_PHY_INFO",
    [PS3_SAS_GET_EXPANDER_INFO]     = "PS3_SAS_GET_EXPANDER_INFO",
    [PS3_MGR_CMD_AUTODUMP_NOTIFY]   = "PS3_MGR_CMD_AUTODUMP_NOTIFY",

    [PS3_MGR_CMD_SECURITY_RANDOM_GET]  = "PS3_MGR_CMD_SECURITY_RANDOM_GET",
    [PS3_MGR_CMD_SECURITY_PASSWORD]    = "PS3_MGR_CMD_SECURITY_PASSWORD",
    [PS3_MGR_CMD_WEBSUBSCRIBE_EVENT]    = "PS3_MGR_CMD_WEBSUBSCRIBE_EVENT",

    [PS3_MGR_CMD_PRESERVED_INFO_GET]   = "PS3_MGR_CMD_PRESERVED_INFO_GET",
    [PS3_MGR_CMD_GET_PD_SN_LIST]       = "PS3_MGR_CMD_GET_PD_SN_LIST",
    [PS3_MGR_CMD_PD_REF_CLEAR]         = "PS3_MGR_CMD_PD_REF_CLEAR",
	[PS3_MGR_CMD_COUNT] = "PS3_MGR_CMD_INVALID",
    };

	if (type < PS3_MGR_CMD_COUNT && type < (sizeof(myNames) / sizeof(myNames[0]))) {
		return myNames[type];
	}

	return "PS3_MGR_CMD_INVALID";

}


enum PS3CmdIocErrCode {
    PS3_IOC_ERR_CODE_OK             = 0x00,
    PS3_IOC_ERR_CODE_ERR            = 0x01,
    PS3_IOC_STATE_INVALID_STATUS    = 0xFFFF,
};

typedef enum PS3CmdStatusCode {
    SCSI_STATUS_GOOD                 = 0x00,
    SCSI_STATUS_CHECK_CONDITION      = 0x02,
    SCSI_STATUS_CONDITION_MET        = 0x04,
    SCSI_STATUS_BUSY                 = 0x08,
    SCSI_STATUS_RESERVATION_CONFLICT = 0x18,
    SCSI_STATUS_TASK_SET_FULL        = 0x28,
    SCSI_STATUS_ACA_ACTIVE           = 0x30,
    SCSI_STATUS_TASK_ABORTED         = 0x40,

    PS3_STATUS_DEVICE_NOT_FOUND      = 0x80,
    PS3_STATUS_IO_ABORTED            = 0x81,
    PS3_STATUS_REQ_ILLEGAL           = 0x82,
    PS3_STATUS_RESET_FAIL            = 0x83,
    PS3_STATUS_VD_OFFLINE            = 0x84,
    PS3_STATUS_ACCESS_BLOCK          = 0x85,
    PS3_STATUS_INTERNAL_SOFT_ERR     = 0x86,
    PS3_STATUS_INTERNAL_ERR          = 0x87,
    PS3_STATUS_HOST_NOT_FOUND        = 0x88,
    PS3_STATUS_HOST_RESET            = 0x89,
    PS3_STATUS_PCI_RECOVERY          = 0x8A,
    PS3_STATUS_VD_MEMBER_OFFLINE     = 0x8B,
    PS3_STATUS_UNDERRUN              = 0x8C,
    PS3_STATUS_OVERRUN               = 0x8D,
    PS3_STATUS_DIF_GRD_ERROR         = 0x8E,
    PS3_STATUS_DIF_REF_ERROR         = 0x8F,
    PS3_STATUS_DIF_APP_ERROR         = 0x90,
    PS3_STATUS_ACCESS_RO             = 0x91,
} PS3CmdStatusCode_e;


enum PS3CmdWordType {
    PS3_CMDWORD_TYPE_INIT   = 0x00,
    PS3_CMDWORD_TYPE_ABORT  = 0x00,  
    PS3_CMDWORD_TYPE_MGR    = 0x01,
    PS3_CMDWORD_TYPE_READ   = 0x02,
    PS3_CMDWORD_TYPE_WRITE  = 0x03,
};

#define PS3_CMD_TYPE_IS_RW(type) \
	((type) == PS3_CMDWORD_TYPE_READ || (type) == PS3_CMDWORD_TYPE_WRITE)

enum PS3ReqFrameCtrl {
    PS3_DATA_BUF_SGL      = 0x00,

    PS3_DATA_BUF_PRP      = 0x02,
};

enum PS3CmdWordDirect {
    PS3_CMDWORD_DIRECT_NORMAL   = 0x00,
    PS3_CMDWORD_DIRECT_RESERVE  = 0x01,
    PS3_CMDWORD_DIRECT_OK       = 0x02,
    PS3_CMDWORD_DIRECT_ADVICE   = 0x03,
};

enum PS3CmdWordPort {
    PS3_CMDWORD_PORT_SAS0       = 0x00,
    PS3_CMDWORD_PORT_SAS1       = 0x01,
    PS3_CMDWORD_PORT_NVME       = 0x02,
    PS3_CMDWORD_PORT_RESERVE    = 0x03,
};

enum PS3CmdWordFormat {
    PS3_CMDWORD_FORMAT_FRONTEND = 0x00,
    PS3_CMDWORD_FORMAT_HARDWARE = 0x01,
};

enum PS3ReplyWordFlag {
    PS3_REPLY_WORD_FLAG_SUCCESS         = 0x00,
    PS3_REPLY_WORD_FLAG_FAIL            = 0X01,
    PS3_REPLY_WORD_FLAG_REPEAT_REPLY    = 0x0F,
    PS3_REPLY_WORD_FLAG_INVALID         = 0X7FFF,
};

enum PS3ReplyWordMask {
    PS3_REPLY_WORD_MASK_FLAG = 0X7FFF,
};

enum PS3ReplyWordMode{
    PS3_REPLY_WORD_MODE_NORMAL                        = 0x00,
    PS3_REPLY_WORD_MODE_DIRECT_ADVICE_TO_NORMAL       = 0X01,
    PS3_REPLY_WORD_MODE_DIRECT_OK                     = 0X02,
    PS3_REPLY_WORD_MODE_DIRECT_ADVICE_TO_DIRECT       = 0X03,
};

enum PS3RetType{
    PS3_NOT_HARD_RET    = 0x00,
    PS3_HARD_RET        = 0X01,
};

enum PS3CmdWordVer {
    PS3_CMDWORD_VER_0           = 0x0,
    PS3_CMDWORD_VER_1           = 0x1,
    PS3_CMDWORD_VER_UPDATING    = 0x2,
    PS3_CMDWORD_VER_INVALID     = 0x3,
};

enum PS3CmdWordNoReplyWord {
    PS3_CMD_WORD_NEED_REPLY_WORD = 0,
    PS3_CMD_WORD_NO_REPLY_WORD   = 1,
};

enum PS3ChannelType {
    PS3_CHAN_TYPE_UNKNOWN   = 0,
    PS3_CHAN_TYPE_VD        = 1,
    PS3_CHAN_TYPE_PD        = 2,
};

enum PS3DiskType {
    PS3_DISK_TYPE_UNKNOWN   = 0,
    PS3_DISK_TYPE_VD        = 1,
    PS3_DISK_TYPE_PD        = 2,
};

#define PS3_CONTROL_PAGE_TYPE_BIT_OFFSET (0x1)   
#define PS3_CONTROL_PAGE_TYPE_BIT_NUM    (0x1)   
#define PS3_CONTROL_PAGE_TYPE_MASK   (0x1)

enum PS3PageType {
    PS3_CONTROL_PAGE_TYPE_OF_SGE = 0,
    PS3_CONTROL_PAGE_TYPE_OF_PRP = 1,
    PS3_CONTROL_PAGE_TYPE_MAX    = 2,
};
static inline const S8 * namePS3DiskType(enum PS3DiskType e)
{
    static const S8 *myNames[] = {
    [PS3_DISK_TYPE_UNKNOWN]   = "PS3_DISK_TYPE_UNKNOWN",
    [PS3_DISK_TYPE_VD]        = "PS3_DISK_TYPE_VD",
    [PS3_DISK_TYPE_PD]        = "PS3_DISK_TYPE_PD"
    };

    return myNames[e];
}

static inline const S8 * namePS3ChannelType(enum PS3ChannelType e)
{
    static const S8 *myNames[] = {
    [PS3_CHAN_TYPE_UNKNOWN]   = "PS3_CHAN_TYPE_UNKNOWN",
    [PS3_CHAN_TYPE_VD]        = "PS3_CHAN_TYPE_VD",
    [PS3_CHAN_TYPE_PD]        = "PS3_CHAN_TYPE_PD"
    };

    return myNames[e];
}


enum PS3DrvMgrErrorCode {
	PS3_DRV_MGR_TIMEOUT    = 1,        
	PS3_DRV_MGR_UNRUNING,              
	PS3_DRV_MGR_INVAL_CMD,             
	PS3_DRV_MGR_NORESOURCE,            
	PS3_DRV_MGR_INVAL_PARAM,           
	PS3_DRV_MGR_DEV_NOEXIST,           
	PS3_DRV_MGR_DEV_DATA_ERR,          
	PS3_DRV_MGR_BUSY,                  
	PS3_DRV_MGR_EVT_REPEAT,            
	PS3_DRV_MGR_EVT_CANCLE_ERR,        
	PS3_DRV_MGR_FLUSH_FAIELD,          
	PS3_DRV_MGR_SMP_BACKEND_ERR,       
	PS3_DRV_MGR_LINK_GET_BACKEND_ERR,  
	PS3_DRV_MGR_PHY_CTL_BACKEND_ERR,   
	PS3_DRV_MGR_RESTART_COMMAND_RSP,   
	PS3_DRV_MGR_TM_FAILED,             
};


enum PS3IoctlRetCode {
    PS3_IOCTL_STATUS_OK              = 0,        
    PS3_IOCTL_STATUS_INVALID_REQ     = 1,        
    PS3_IOCTL_STATUS_NO_HBA          = 2,        
    PS3_IOCTL_STATUS_BUSY            = 3,        
    PS3_IOCTL_STATUS_NOT_READY       = 4,        
    PS3_IOCTL_STATUS_INVALIED_PARAM  = 5,        
    PS3_IOCTL_STATUS_REQ_ERR         = 6,        
    PS3_IOCTL_STATUS_NEED_RETRY      = 7,        
};

#endif 

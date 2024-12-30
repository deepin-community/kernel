
#ifndef _NVME_SPEC_H
#define _NVME_SPEC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ps3_types.h>

#define NVME_REG32_ERR_VAL            ((U32)(-1))
#define NVME_REG64_ERR_VAL            ((U64)(-1))
#define NVME_MAX_IO_QUEUES            (65536)
#define NVME_MAX_NUM_ADMIN_QD         (4096)
#define NVME_MAX_NUM_IO_QD            (65536)
#define NVME_MIN_NUM_ADMIN_QD         (2)
#define NVME_MIN_NUM_IO_QD            (MASK#define NVME_NSSR_VAL                 (0x4e564d65)
#define NVME_CMD_SET_SUPPORT          BIT(0)
#define NVME_INVALID_CID              (0xFFFF)
#define NVME_MAX_WRITE_ZEROES         (0xFFFF) 
#define NVME_NSID_MASK                (0xFFFFFFFF)

#define NVME_GET_SECTOR_SIZE(data) (1 << (data)->lbaf[(data)->flbas.format].lbads)


#define NVME_GET_SQ_DOOR_BELL_OFFSET(qid, strd)  (0x1000 + 2 * (qid) * (4 << (strd)))

#define NVME_GET_CQ_DOOR_BELL_OFFSET(qid, strd)  (0x1000 + (2 * (qid) + 1) * (4 << (strd)))

#define NVME_SANITIZE_MAX_PROG 0xFFFF

#define NVME_CAP_CSS_NVM  1

#define NVME_MPS_TO_PAGESIZE(mpsMin)   (1u << (12 + (mpsMin)))

#define NVME_PAGESIZE_TO_MPS(pageSize) (nvmeU64Log2(pageSize) - 12)


typedef union NvmeCapReg {
    U64 rawVal;
    struct {
        U32 mqes : 16;
        U32 cqr : 1;  
        U32 ams : 2;  
        U32 rsvd : 5;
        U32 timeout : 8; 
        U32 dbStride : 4;

        U32 nssrSupport : 1;
        U32 cmdSetSupport : 8;
        U32 bootPartSupport : 1;
        U32 rsvd1 : 2;
        U32 mpsMin : 4;
        U32 mpsMax : 4;
        U32 pmrSupport : 1;
        U32 cmbSupport : 1;
        U32 rsvd3 : 6;
    };
} NvmeCapReg_u;


typedef union NvmeVsReg {
    U32 rawVal;
    struct {
        U8 vsTerity;   
        U8 vsMinorNum; 
        U16 vsMajorNum;
    };
} NvmeVsReg_u;



#define NVME_VERSION(mjr, mnr, ter)                                            \
    (((uint32_s)(mjr) << 16) | ((uint32_s)(mnr) << 8) | (uint32_s)(ter))


#define NVME_IO_SQ_ENTRY_SIZE    6

#define NVME_IO_CQ_ENTRY_SIZE    4


#define NVME_SHUT_DOWN_NO_EFFECT        0x0

#define NVME_SHUT_DOWN_NOTIFY           0x1

#define NVME_SHUT_DOWN_ABRUT_NOTIFY     0x2

#define NVME_CSS_NVM_CMD_SET            0x0

typedef union NvmeCCReg {
    U32 rawVal;
    struct {

        U32 en : 1;
        U32 rsvd : 3;
        U32 css : 3;
        U32 mps : 4;
        U32 ams : 3;
        U32 shutdownNotify : 2;

        U32 ioSqEntrySz : 4;
        U32 ioCqEntrySz : 4;
        U32 rsvd1 : 8;
    };
} NvmeCCReg_u;


#define NVME_SHST_NORMAL       0x0
#define NVME_SHST_OCCURRING    0x1
#define NVME_SHST_COMPLETE     0x2

typedef union NvmeCstsReg {
    U32 rawVal;
    struct {
        U32 rdy : 1;
        U32 cfs : 1;
        U32 shutdownStatus : 2;
        U32 nssrOccur : 1;
        U32 proccessPaused : 1;
        U32 rsvd : 26;
    };
} NvmeCstsReg_u;


typedef union NvmeAqaReg {
    U32 rawVal;
    struct {
        U32 adminSqSz : 12;
        U32 rsvd : 4;
        U32 adminCqSz : 12;
        U32 rsvd1 : 4;
    };
} NvmeAqaReg_u;


typedef union NvmeCmbLocReg {
    U32 rawVal;
    struct {
        U32 bir : 3;
        U32 cmbQMixMemSupport : 1;
        U32 cmbQPhyDiscontiSupport : 1;
        U32 cmbDataPtrMixLocSupport : 1;
        U32 cmbDataPtrCmdIndentLocSupport : 1;
        U32 cmbDataMetaMemMixSupport : 1;
        U32 cmbQDwordAlignSupport : 1;
        U32 rsvd : 3;
        U32 offset : 20;
    };
} NvmeCmbLocReg_u;


typedef union NvmeCmbSzReg {
    U32 rawVal;
    struct {
        U32 sqSupport : 1;
        U32 cqSupport : 1;
        U32 prpListSupport : 1;
        U32 readSupport : 1;
        U32 writeSupport : 1;
        U32 rsvd : 3;
        U32 szUnit : 4;
        U32 size : 20;
    };
} NvmeCmbSzReg_u;


typedef struct NvmeControllerReg {
    NvmeCapReg_u cap;
    NvmeVsReg_u vs;
    U32 intmskSet;
    U32 intmskClr;
    NvmeCCReg_u cc;
    U32 rsvd0;
    NvmeCstsReg_u csts;
    U32 nssrCtrl;
    NvmeAqaReg_u aqa;
    U64 asq;
    U64 acq;
    NvmeCmbLocReg_u cmbLoc;
    NvmeCmbSzReg_u cmbSz;



} NvmeControllerReg_s;


typedef struct DBEntry {
    U32 sqT;
    U32 cqH;
} DBEntry_s;


enum NvmeSglDescSubType {
    NVME_SGL_SUBTYPE_ADDRESS   = 0x0,
    NVME_SGL_SUBTYPE_OFFSET    = 0x1,
    NVME_SGL_SUBTYPE_TRANSPORT = 0xa,
};

typedef struct GenericSglDesc{
    U32 rsvd;
    U32 rsvd1 : 24;
    U32 subType : 4;
    U32 type : 4;
} GenericSglDesc_s;


typedef struct UnKeyedSglDesc{
    U32 len;
    U32 rsvd : 24;
    U32 subType : 4;
    U32 type : 4;
} UnKeyedSglDesc_s;


typedef struct KeyedSglDesc{
    U64 len : 24;
    U64 key : 32;
    U64 subType : 4;
    U64 type : 4;
} KeyedSglDesc_s;


typedef struct NvmeSglDesc {
    U64 addr;
    union {
        GenericSglDesc_s genSgl;
        UnKeyedSglDesc_s unkeyedSgl;
        KeyedSglDesc_s keyedSgl;
    };
} NvmeSglDesc_s;




struct NvmeSanitize {

    U32 sanact : 3;

    U32 ause : 1;

    U32 owpass : 4;

    U32 oipbp : 1;

    U32 ndas : 1;

    U32 reserved : 22;
};



enum NvmeSanitizeAction {

    NVME_SANITIZE_EXIT_FAILURE_MODE = 0x1,

    NVME_SANITIZE_BLOCK_ERASE = 0x2,

    NVME_SANITIZE_OVERWRITE = 0x3,

    NVME_SANITIZE_CRYPTO_ERASE = 0x4,
};

typedef struct NvmeCommonCmdEntry {

    U16 opc   : 8; 
    U16 fuse  : 2; 
    U16 rsvd  : 4;
    U16 psdt  : 2;
    U16 cid;


    U32 nsId;


    U64 rsvd1;


    U64 metaPtr;


    union {
        struct {
            U64 prp1;
            union {
                U64 prp2;
                struct {
                    U32 metaDataLen;
                    U32 dataLen;
                };
            };
        };
        NvmeSglDesc_s sgl;
    };


    union {

        struct {
            U64 slba;
            U32 nlba : 16;
            U32 res1 : 8;
            U32 stc : 1;   
            U32 res2 : 1;
            U32 prinfo : 4;
            U32 fua : 1;   
            U32 lr : 1;    
        };

        struct {
            struct NvmeSanitize sanitizeDw10;
            U32 overwritePattern;
        };
        struct {
            U32 dw10;
            U32 dw11;
            U32 dw12;
            U32 dw13;
            U32 dw14;
            U32 dw15;
        };
    };
} NvmeCommonCmdEntry_s;

#ifndef _WINDOWS
typedef struct __attribute__((packed)) NvmeCmdStatus {
#else
#pragma pack(1)
typedef struct NvmeCmdStatus {
#endif
    union {
        struct {
            U16 p   : 1;     
            U16 sc  : 8;     
            U16 sct : 3;     
            U16 crd : 2;     
            U16 m   : 1;     
            U16 dnr : 1;     
        };
        U16 cmdStatus;
    };
} NvmeCmdStatus_s;

#ifdef _WINDOWS
#pragma pack()
#endif


#define NVME_GET_IO_SQ_NUM(cmdSpec) (((cmdSpec) & 0xFFFF) + 1)

#define NVME_GET_IO_CQ_NUM(cmdSpec) (((cmdSpec) >> 16) + 1)


#define NVME_GET_POWER_STATE(cmdSpec) (((cmdSpec) & 0X1F))

typedef struct NvmeCplEntry {

    U32 cmdSpec;

    U32 rsvd;

    U16 sqHead;
    U16 sqId;  

    U16 cid;
    NvmeCmdStatus_s status;
} NvmeCplEntry_s;


enum NvmeAdminOpcode {
    NVME_OPC_DELETE_IO_SQ = 0x00,
    NVME_OPC_CREATE_IO_SQ = 0x01,
    NVME_OPC_GET_LOG_PAGE = 0x02,

    NVME_OPC_DELETE_IO_CQ = 0x04,
    NVME_OPC_CREATE_IO_CQ = 0x05,
    NVME_OPC_IDENTIFY     = 0x06,

    NVME_OPC_ABORT        = 0x08,
    NVME_OPC_SET_FEATURES = 0x09,
    NVME_OPC_GET_FEATURES = 0x0a,

    NVME_OPC_ASYNC_EVENT_REQUEST = 0x0c,
    NVME_OPC_NS_MANAGEMENT       = 0x0d,

    NVME_OPC_FIRMWARE_COMMIT           = 0x10,
    NVME_OPC_FIRMWARE_IMAGE_DOWNLOAD   = 0x11,
    NVME_OPC_DEVICE_SELF_TEST          = 0x14,
    NVME_OPC_NS_ATTACHMENT             = 0x15,
    NVME_OPC_KEEP_ALIVE                = 0x18,
    NVME_OPC_DIRECTIVE_SEND            = 0x19,
    NVME_OPC_DIRECTIVE_RECEIVE         = 0x1a,
    NVME_OPC_VIRTUALIZATION_MANAGEMENT = 0x1c,
    NVME_OPC_NVME_MI_SEND              = 0x1d,
    NVME_OPC_NVME_MI_RECEIVE           = 0x1e,
    NVME_OPC_DOORBELL_BUFFER_CONFIG    = 0x7c,
    NVME_OPC_FORMAT_NVM                = 0x80,
    NVME_OPC_SECURITY_SEND             = 0x81,
    NVME_OPC_SECURITY_RECEIVE          = 0x82,
    NVME_OPC_SANITIZE                  = 0x84,
};


enum NvmeIOOpcode {
    NVME_OPC_FLUSH = 0x00,
    NVME_OPC_WRITE = 0x01,
    NVME_OPC_READ  = 0x02,

    NVME_OPC_WRITE_UNCORRECTABLE = 0x04,
    NVME_OPC_COMPARE             = 0x05,

    NVME_OPC_WRITE_ZEROES       = 0x08,
    NVME_OPC_DATASET_MANAGEMENT = 0x09,
    NVME_OPC_VERIFY = 0x0C,

    NVME_OPC_RESERVATION_REGISTER = 0x0d,
    NVME_OPC_RESERVATION_REPORT   = 0x0e,

    NVME_OPC_RESERVATION_ACQUIRE = 0x11,
    NVME_OPC_RESERVATION_RELEASE = 0x15,
};


enum NvmeStatusCodeType {
    NVME_SCT_GENERIC          = 0x0,
    NVME_SCT_COMMAND_SPECIFIC = 0x1,
    NVME_SCT_MEDIA_ERROR      = 0x2,
    NVME_SCT_PATH             = 0x3,

    NVME_SCT_VENDOR_SPECIFIC = 0x7,
};


enum NvmeGenCmdSC {
    NVME_SC_SUCCESS                     = 0x00,
    NVME_SC_INVALID_OPCODE              = 0x01,
    NVME_SC_INVALID_FIELD               = 0x02,
    NVME_SC_COMMAND_ID_CONFLICT         = 0x03,
    NVME_SC_DATA_TRANSFER_ERROR         = 0x04,
    NVME_SC_ABORTED_POWER_LOSS          = 0x05,
    NVME_SC_INTERNAL_DEVICE_ERROR       = 0x06,
    NVME_SC_ABORTED_BY_REQUEST          = 0x07,
    NVME_SC_ABORTED_SQ_DELETION         = 0x08,
    NVME_SC_ABORTED_FAILED_FUSED        = 0x09,
    NVME_SC_ABORTED_MISSING_FUSED       = 0x0a,
    NVME_SC_INVALID_NAMESPACE_OR_FORMAT = 0x0b,
    NVME_SC_COMMAND_SEQUENCE_ERROR      = 0x0c,
    NVME_SC_INVALID_SGL_SEG_DESCRIPTOR  = 0x0d,
    NVME_SC_INVALID_NUM_SGL_DESCIRPTORS = 0x0e,
    NVME_SC_DATA_SGL_LENGTH_INVALID     = 0x0f,
    NVME_SC_METADATA_SGL_LENGTH_INVALID = 0x10,
    NVME_SC_SGL_DESCRIPTOR_TYPE_INVALID = 0x11,
    NVME_SC_INVALID_CONTROLLER_MEM_BUF  = 0x12,
    NVME_SC_INVALID_PRP_OFFSET          = 0x13,
    NVME_SC_ATOMIC_WRITE_UNIT_EXCEEDED  = 0x14,
    NVME_SC_OPERATION_DENIED            = 0x15,
    NVME_SC_INVALID_SGL_OFFSET          = 0x16,

    NVME_SC_HOSTID_INCONSISTENT_FORMAT         = 0x18,
    NVME_SC_KEEP_ALIVE_EXPIRED                 = 0x19,
    NVME_SC_KEEP_ALIVE_INVALID                 = 0x1a,
    NVME_SC_ABORTED_PREEMPT                    = 0x1b,
    NVME_SC_SANITIZE_FAILED                    = 0x1c,
    NVME_SC_SANITIZE_IN_PROGRESS               = 0x1d,
    NVME_SC_SGL_DATA_BLOCK_GRANULARITY_INVALID = 0x1e,
    NVME_SC_COMMAND_INVALID_IN_CMB             = 0x1f,
    NVME_SC_NAMESPACE_IS_WRITE_PROTECTED       = 0x20,
    NVME_SC_COMMAND_INTERRUPTED			       = 0x21,
    NVME_SC_TRANSIENT_TRANSPORT_ERROR          = 0x22,

    NVME_SC_LBA_OUT_OF_RANGE     = 0x80,
    NVME_SC_CAPACITY_EXCEEDED    = 0x81,
    NVME_SC_NAMESPACE_NOT_READY  = 0x82,
    NVME_SC_RESERVATION_CONFLICT = 0x83,
    NVME_SC_FORMAT_IN_PROGRESS   = 0x84,
};


enum NvmeCmdSpecSC {
    NVME_CSC_COMPLETION_QUEUE_INVALID     = 0x00,
    NVME_CSC_INVALID_QUEUE_IDENTIFIER     = 0x01,
    NVME_CSC_MAXIMUM_QUEUE_SIZE_EXCEEDED  = 0x02,
    NVME_CSC_ABORT_COMMAND_LIMIT_EXCEEDED = 0x03,

    NVME_CSC_ASYNC_EVENT_REQUEST_LIMIT_EXCEEDED = 0x05,
    NVME_CSC_INVALID_FIRMWARE_SLOT              = 0x06,
    NVME_CSC_INVALID_FIRMWARE_IMAGE             = 0x07,
    NVME_CSC_INVALID_INTERRUPT_VECTOR           = 0x08,
    NVME_CSC_INVALID_LOG_PAGE                   = 0x09,
    NVME_CSC_INVALID_FORMAT                     = 0x0a,
    NVME_CSC_FIRMWARE_REQ_CONVENTIONAL_RESET    = 0x0b,
    NVME_CSC_INVALID_QUEUE_DELETION             = 0x0c,
    NVME_CSC_FEATURE_ID_NOT_SAVEABLE            = 0x0d,
    NVME_CSC_FEATURE_NOT_CHANGEABLE             = 0x0e,
    NVME_CSC_FEATURE_NOT_NAMESPACE_SPECIFIC     = 0x0f,
    NVME_CSC_FIRMWARE_REQ_NVM_RESET             = 0x10,
    NVME_CSC_FIRMWARE_REQ_RESET                 = 0x11,
    NVME_CSC_FIRMWARE_REQ_MAX_TIME_VIOLATION    = 0x12,
    NVME_CSC_FIRMWARE_ACTIVATION_PROHIBITED     = 0x13,
    NVME_CSC_OVERLAPPING_RANGE                  = 0x14,
    NVME_CSC_NAMESPACE_INSUFFICIENT_CAPACITY    = 0x15,
    NVME_CSC_NAMESPACE_ID_UNAVAILABLE           = 0x16,

    NVME_CSC_NAMESPACE_ALREADY_ATTACHED      = 0x18,
    NVME_CSC_NAMESPACE_IS_PRIVATE            = 0x19,
    NVME_CSC_NAMESPACE_NOT_ATTACHED          = 0x1a,
    NVME_CSC_THINPROVISIONING_NOT_SUPPORTED  = 0x1b,
    NVME_CSC_CONTROLLER_LIST_INVALID         = 0x1c,
    NVME_CSC_DEVICE_SELF_TEST_IN_PROGRESS    = 0x1d,
    NVME_CSC_BOOT_PARTITION_WRITE_PROHIBITED = 0x1e,
    NVME_CSC_INVALID_CTRLR_ID                = 0x1f,
    NVME_CSC_INVALID_SECONDARY_CTRLR_STATE   = 0x20,
    NVME_CSC_INVALID_NUM_CTRLR_RESOURCES     = 0x21,
    NVME_CSC_INVALID_RESOURCE_ID             = 0x22,

    NVME_CSC_CONFLICTING_ATTRIBUTES      = 0x80,
    NVME_CSC_INVALID_PROTECTION_INFO     = 0x81,
    NVME_CSC_ATTEMPTED_WRITE_TO_RO_RANGE = 0x82,
};


enum NvmeMediaErrSC {
    NVME_MSC_WRITE_FAULTS                   = 0x80,
    NVME_MSC_UNRECOVERED_READ_ERROR         = 0x81,
    NVME_MSC_GUARD_CHECK_ERROR              = 0x82,
    NVME_MSC_APPLICATION_TAG_CHECK_ERROR    = 0x83,
    NVME_MSC_REFERENCE_TAG_CHECK_ERROR      = 0x84,
    NVME_MSC_COMPARE_FAILURE                = 0x85,
    NVME_MSC_ACCESS_DENIED                  = 0x86,
    NVME_MSC_DEALLOCATED_OR_UNWRITTEN_BLOCK = 0x87,
};

enum NvmeStatusCodes {
    NVME_SUCCESS                = 0x0000,
    NVME_INVALID_OPCODE         = 0x0001,
    NVME_INVALID_FIELD          = 0x0002,
    NVME_CID_CONFLICT           = 0x0003,
    NVME_DATA_TRAS_ERROR        = 0x0004,
    NVME_POWER_LOSS_ABORT       = 0x0005,
    NVME_INTERNAL_DEV_ERROR     = 0x0006,
    NVME_CMD_ABORT_REQ          = 0x0007,
    NVME_CMD_ABORT_SQ_DEL       = 0x0008,
    NVME_CMD_ABORT_FAILED_FUSE  = 0x0009,
    NVME_CMD_ABORT_MISSING_FUSE = 0x000a,
    NVME_INVALID_NSID           = 0x000b,
    NVME_CMD_SEQ_ERROR          = 0x000c,
    NVME_INVALID_SGL_SEG_DESCR  = 0x000d,
    NVME_INVALID_NUM_SGL_DESCRS = 0x000e,
    NVME_DATA_SGL_LEN_INVALID   = 0x000f,
    NVME_MD_SGL_LEN_INVALID     = 0x0010,
    NVME_SGL_DESCR_TYPE_INVALID = 0x0011,
    NVME_INVALID_USE_OF_CMB     = 0x0012,
    NVME_INVALID_PRP_OFFSET     = 0x0013,
    NVME_CMD_SET_CMB_REJECTED   = 0x002b,
    NVME_INVALID_CMD_SET        = 0x002c,
    NVME_LBA_RANGE              = 0x0080,
    NVME_CAP_EXCEEDED           = 0x0081,
    NVME_NS_NOT_READY           = 0x0082,
    NVME_NS_RESV_CONFLICT       = 0x0083,
    NVME_FORMAT_IN_PROGRESS     = 0x0084,
    NVME_INVALID_CQID           = 0x0100,
    NVME_INVALID_QID            = 0x0101,
    NVME_MAX_QSIZE_EXCEEDED     = 0x0102,
    NVME_ACL_EXCEEDED           = 0x0103,
    NVME_RESERVED               = 0x0104,
    NVME_AER_LIMIT_EXCEEDED     = 0x0105,
    NVME_INVALID_FW_SLOT        = 0x0106,
    NVME_INVALID_FW_IMAGE       = 0x0107,
    NVME_INVALID_IRQ_VECTOR     = 0x0108,
    NVME_INVALID_LOG_ID         = 0x0109,
    NVME_INVALID_FORMAT         = 0x010a,
    NVME_FW_REQ_RESET           = 0x010b,
    NVME_INVALID_QUEUE_DEL      = 0x010c,
    NVME_FID_NOT_SAVEABLE       = 0x010d,
    NVME_FEAT_NOT_CHANGEABLE    = 0x010e,
    NVME_FEAT_NOT_NS_SPEC       = 0x010f,
    NVME_FW_REQ_SUSYSTEM_RESET  = 0x0110,
    NVME_NS_ALREADY_ATTACHED    = 0x0118,
    NVME_NS_PRIVATE             = 0x0119,
    NVME_NS_NOT_ATTACHED        = 0x011A,
    NVME_NS_CTRL_LIST_INVALID   = 0x011C,
    NVME_CONFLICTING_ATTRS      = 0x0180,
    NVME_INVALID_PROT_INFO      = 0x0181,
    NVME_WRITE_TO_RO            = 0x0182,
    NVME_CMD_SIZE_LIMIT         = 0x0183,
    NVME_ZONE_BOUNDARY_ERROR    = 0x01b8,
    NVME_ZONE_FULL              = 0x01b9,
    NVME_ZONE_READ_ONLY         = 0x01ba,
    NVME_ZONE_OFFLINE           = 0x01bb,
    NVME_ZONE_INVALID_WRITE     = 0x01bc,
    NVME_ZONE_TOO_MANY_ACTIVE   = 0x01bd,
    NVME_ZONE_TOO_MANY_OPEN     = 0x01be,
    NVME_ZONE_INVAL_TRANSITION  = 0x01bf,
    NVME_WRITE_FAULT            = 0x0280,
    NVME_UNRECOVERED_READ       = 0x0281,
    NVME_E2E_GUARD_ERROR        = 0x0282,
    NVME_E2E_APP_ERROR          = 0x0283,
    NVME_E2E_REF_ERROR          = 0x0284,
    NVME_CMP_FAILURE            = 0x0285,
    NVME_ACCESS_DENIED          = 0x0286,
    NVME_DULB                   = 0x0287,
    NVME_MORE                   = 0x2000,
    NVME_DNR                    = 0x4000,
    NVME_NO_COMPLETE            = 0xffff,
};

enum NvmeQprio {
    NVME_QPRIO_URGENT = 0x0,
    NVME_QPRIO_HIGH   = 0x1,
    NVME_QPRIO_MEDIUM = 0x2,
    NVME_QPRIO_LOW    = 0x3
};

enum NvmeDataTransfer {

    NVME_DATA_NONE = 0,

    NVME_DATA_HOST_TO_CONTROLLER = 1,

    NVME_DATA_CONTROLLER_TO_HOST = 2,

    NVME_DATA_BIDIRECTIONAL = 3
};

static inline enum NvmeDataTransfer NvmeOpcGetDataTransfer(U8 opc)
{
    return (enum NvmeDataTransfer)(opc & 3);
}

enum NvmeFeat {



    NVME_FEAT_ARBITRATION = 0x01,

    NVME_FEAT_POWER_MANAGEMENT = 0x02,

    NVME_FEAT_LBA_RANGE_TYPE = 0x03,

    NVME_FEAT_TEMPERATURE_THRESHOLD = 0x04,

    NVME_FEAT_ERROR_RECOVERY = 0x05,

    NVME_FEAT_VOLATILE_WRITE_CACHE = 0x06,

    NVME_FEAT_NUMBER_OF_QUEUES = 0x07,

    NVME_FEAT_INTERRUPT_COALESCING = 0x08,

    NVME_FEAT_INTERRUPT_VECTOR_CONFIGURATION = 0x09,

    NVME_FEAT_WRITE_ATOMICITY = 0x0A,

    NVME_FEAT_ASYNC_EVENT_CONFIGURATION = 0x0B,

    NVME_FEAT_AUTONOMOUS_POWER_STATE_TRANSITION = 0x0C,

    NVME_FEAT_HOST_MEM_BUFFER = 0x0D,
    NVME_FEAT_TIMESTAMP       = 0x0E,

    NVME_FEAT_KEEP_ALIVE_TIMER = 0x0F,

    NVME_FEAT_HOST_CONTROLLED_THERMAL_MANAGEMENT = 0x10,

    NVME_FEAT_NON_OPERATIONAL_POWER_STATE_CONFIG = 0x11,






    NVME_FEAT_SOFTWARE_PROGRESS_MARKER = 0x80,


    NVME_FEAT_HOST_IDENTIFIER      = 0x81,
    NVME_FEAT_HOST_RESERVE_MASK    = 0x82,
    NVME_FEAT_HOST_RESERVE_PERSIST = 0x83,




};


enum NvmeDsmAttribute {
    NVME_DSM_ATTR_INTEGRAL_READ  = 0x1,
    NVME_DSM_ATTR_INTEGRAL_WRITE = 0x2,
    NVME_DSM_ATTR_DEALLOCATE     = 0x4,
};

struct NvmePowerState {
    U16 mp;

    U8 reserved1;

    U8 mps : 1; 
    U8 nops : 1;
    U8 reserved2 : 6;

    U32 enlat;
    U32 exlat;

    U8 rrt : 5;
    U8 reserved3 : 3;

    U8 rrl : 5;
    U8 reserved4 : 3;

    U8 rwt : 5;
    U8 reserved5 : 3;

    U8 rwl : 5;
    U8 reserved6 : 3;

    U8 reserved7[16];
};


typedef enum NvmeIdentifyCns {

    NVME_IDENTIFY_NS = 0x00,


    NVME_IDENTIFY_CTRLR = 0x01,


    NVME_IDENTIFY_ACTIVE_NS_LIST = 0x02,


    NVME_IDENTIFY_NS_ID_DESCRIPTOR_LIST = 0x03,


    NVME_IDENTIFY_ALLOCATED_NS_LIST = 0x10,


    NVME_IDENTIFY_NS_ALLOCATED = 0x11,



    NVME_IDENTIFY_NS_ATTACHED_CTRLR_LIST = 0x12,


    NVME_IDENTIFY_CTRLR_LIST = 0x13,


    NVME_IDENTIFY_PRIMARY_CTRLR_CAP = 0x14,


    NVME_IDENTIFY_SECONDARY_CTRLR_LIST = 0x15,


    NVME_IDENTIFY_NS_GRANULARITY_LIST = 0x16,
} NvmeIdentifyCns_e;


enum NvmfCtrlrModel {

    NVMF_CTRLR_MODEL_DYNAMIC = 0,


    NVMF_CTRLR_MODEL_STATIC = 1,
};

#define NVME_CTRLR_SN_LEN 20
#define NVME_CTRLR_MN_LEN 40
#define NVME_CTRLR_FR_LEN 8


enum NvmeSglsSupported {

    NVME_SGLS_NOT_SUPPORTED = 0,


    NVME_SGLS_SUPPORTED = 1,


    NVME_SGLS_SUPPORTED_DWORD_PCIE_ALIGNED = 2,
};


enum NvmeFlushBroadcast {

    NVME_FLUSH_BROADCAST_NOT_INDICATED = 0,




    NVME_FLUSH_BROADCAST_NOT_SUPPORTED = 2,


    NVME_FLUSH_BROADCAST_SUPPORTED = 3
};
#ifndef _WINDOWS
struct __attribute__((packed)) __attribute__((aligned)) NvmeCtrlrData {
#else
#pragma pack(1)
struct NvmeCtrlrData {
#endif


    U16 vid;


    U16 ssvid;


    S8 sn[NVME_CTRLR_SN_LEN];


    S8 mn[NVME_CTRLR_MN_LEN];


    U8 fr[NVME_CTRLR_FR_LEN];


    U8 rab;


    U8 ieee[3];



    U8 cmic;


    U8 mdts;


    U16 cntlid;


    union NvmeVsReg ver;


    U32 rtd3r;


    U32 rtd3e;


    U32 oaes;


    U32 ctratt;

    U8 reserved_100[12];


    U8 fguid[16];

    U8 reserved_128[128];




    U16 oacs;


    U8 acl;


    U8 aerl;


    U8 frmw;


    U8 lpe;


    U8 elpe;


    U8 npss;


    U8 avscc;


    U8 apsta;


    U16 wctemp;


    U16 cctemp;


    U16 mtfa;


    U32 hmpre;


    U32 hmmin;


    U64 tnvmcap[2];


    U64 unvmcap[2];


    U32 rpmbs;


    U16 edstt;


    U8 dsto;

    U8 fwug;

    U16 kas;


    U16 hctma;


    U16 mntmt;


    U16 mxtmt;


    union {
        U32 sanicap;
        struct {
            U32 ces:1;
            U32 bes:1;
            U32 ows:1;
            U32 snicapRsvd1:26;
            U32 ndi:1;
            U32 nodmmas:2;
        };
    };

    U8 reserved3[180];




    U8 sqes;


    U8 cqes;

    U16 maxcmd;


    U32 nn;


    U16 oncs;


    U16 fuses;


    U8 fna;


    U8 vwc;


    U16 awun;


    U16 awupf;


    U8 nvscc;

    U8 reserved531;


    U16 acwu;

    U16 reserved534;


    U32 sgls;

    U8 reserved4[228];

    U8 subnqn[256];

    U8 reserved5[768];


    struct {

        U32 ioccsz;


        U32 iorcsz;


        U16 icdoff;


        U8 ctrattr;


        U8 msdbd;

        U8 reserved[244];
    };


    struct NvmePowerState psd[32];


    U8 vs[1024];
};
#ifdef _WINDOWS
#pragma pack()
#endif

#ifndef _WINDOWS
struct __attribute__((packed)) NvmePrimaryCtrlCapabilities {
#else
#pragma pack(1)
struct NvmePrimaryCtrlCapabilities {
#endif

    U16 cntlid;

    U16 portid;

    U8 crt;
    U8 reserved[27];

    U32 vqfrt;

    U32 vqrfa;

    U16 vqrfap;

    U16 vqprt;


    U16 vqfrsm;

    U16 vqgran;
    U8 reserved1[16];


    U32 vifrt;


    U32 virfa;


    U16 virfap;

    U16 viprt;


    U16 vifrsm;

    U16 vigran;
    U8 reserved2[4016];
};


#ifdef _WINDOWS
#pragma pack()
#endif
#ifndef _WINDOWS
struct __attribute__((packed)) NvmeSecondaryCtrlEntry {
#else
#pragma pack(1)
struct NvmeSecondaryCtrlEntry {
#endif

    U16 scid;

    U16 pcid;

    U8 scs;
    U8 reserved[3];

    U16 vfn;


    U16 nvq;


    U16 nvi;
    U8 reserved1[18];
};

#ifdef _WINDOWS
#pragma pack()
#endif
#ifndef _WINDOWS
struct __attribute__((packed)) NvmeSecondaryCtrlList {
#else
#pragma pack(1)
struct NvmeSecondaryCtrlList {
#endif

    U8 number;
    U8 reserved[31];
    struct NvmeSecondaryCtrlEntry entries[127];
};

#ifdef _WINDOWS
#pragma pack()
#endif
struct NvmeNsData {

    U64 nsze;


    U64 ncap;


    U64 nuse;


    U8 nsfeat;


    U8 nlbaf;
    union {
        struct {
            U8 format    : 4;
            U8 extended  : 1;
            U8 reserved2 : 3;
        } flbas;
        U8 flbasRaw;
    };


    U8 mc;


    union {
        struct {
            U8 type1 : 1;
            U8 type2 : 1;
            U8 type3 : 1;
            U8 piBeforeMeta : 1;
            U8 piAfterMeta : 1;
            U8 reserved2 : 3;
        }; 
        U8 rawData;
    } dpc;


    union {
        struct {
            U8 type : 3;
            U8 isPiBefore : 1;
            U8 reserved2 : 4;
        };
        U8 rawData;
    } dps; 


    U8 nmic;


    U8 nsrescap;

    U8 fpi;


    U8 dlfeat;


    U16 nawun;


    U16 nawupf;


    U16 nacwu;


    U16 nabsn;


    U16 nabo;


    U16 nabspf;


    U16 noiob;


    U64 nvmcap[2];

    U8 reserved64[40];


    U8 nguid[16];


    U64 eui64;

    struct {
        U32 ms        : 16;
        U32 lbads     : 8; 
        U32 rp        : 2; 
        U32 reserved6 : 6;
    } lbaf[16]; 

    U8 reserved6[192];

    U8 vendorSpecific[3712];
};



enum NvmeDeallocLogicalBlockReadValue {

    NVME_DEALLOC_NOT_REPORTED = 0,


    NVME_DEALLOC_READ_00 = 1,


    NVME_DEALLOC_READ_FF = 2,
};


enum NvmeReservationType {



    NVME_RESERVE_WRITE_EXCLUSIVE = 0x1,


    NVME_RESERVE_EXCLUSIVE_ACCESS = 0x2,


    NVME_RESERVE_WRITE_EXCLUSIVE_REG_ONLY = 0x3,


    NVME_RESERVE_EXCLUSIVE_ACCESS_REG_ONLY = 0x4,


    NVME_RESERVE_WRITE_EXCLUSIVE_ALL_REGS = 0x5,


    NVME_RESERVE_EXCLUSIVE_ACCESS_ALL_REGS = 0x6,


};

struct NvmeReservationAcquireData {

    U64 crkey;

    U64 prkey;
};




enum NvmeReservationAcquireAction {
    NVME_RESERVE_ACQUIRE       = 0x0,
    NVME_RESERVE_PREEMPT       = 0x1,
    NVME_RESERVE_PREEMPT_ABORT = 0x2,
};
#ifndef _WINDOWS
struct __attribute__((packed)) NvmeReservationStatusData {
#else
#pragma pack(1)
struct NvmeReservationStatusData {
#endif

    U32 gen;

    U8 rtype;

    U16 regctl;
    U16 reserved1;

    U8 ptpls;
    U8 reserved[14];
};
#ifdef _WINDOWS
#pragma pack()
#endif

#ifndef _WINDOWS
struct __attribute__((packed)) NvmeReservationStatusExtendedData {
#else
#pragma pack(1)
struct NvmeReservationStatusExtendedData {
#endif
    struct NvmeReservationStatusData data;
    U8 reserved[40];
};


#ifdef _WINDOWS
#pragma pack()
#endif
#ifndef _WINDOWS
struct __attribute__((packed)) NvmeRegisteredCtrlrData {
#else
#pragma pack(1)
struct NvmeRegisteredCtrlrData {
#endif

    U16 cntlid;
    U8 rcsts;
    U8 reserved2[5];

    U64 hostid;

    U64 rkey;
};

#ifdef _WINDOWS
#pragma pack()
#endif
#ifndef _WINDOWS
struct __attribute__((packed)) NvmeRegisteredCtrlrExtendedData {
#else
#pragma pack(1)
struct NvmeRegisteredCtrlrExtendedData {
#endif

    U16 cntlid;

    U8 rcsts;
    U8 reserved2[5];

    U64 rkey;

    U8 hostid[16];
    U8 reserved3[32];
};


#ifdef _WINDOWS
#pragma pack()
#endif


enum NvmeReservationRegisterCptpl {
    NVME_RESERVE_PTPL_NO_CHANGES         = 0x0,
    NVME_RESERVE_PTPL_CLEAR_POWER_ON     = 0x2,
    NVME_RESERVE_PTPL_PERSIST_POWER_LOSS = 0x3,
};


enum NvmeReservationRegisterAction {
    NVME_RESERVE_REGISTER_KEY   = 0x0,
    NVME_RESERVE_UNREGISTER_KEY = 0x1,
    NVME_RESERVE_REPLACE_KEY    = 0x2,
};

struct NvmeReservationRegisterData {

    U64 crkey;

    U64 nrkey;
};



struct NvmeReservationKeyData {

    U64 crkey;
};



enum NvmeReservationReleaseAction {
    NVME_RESERVE_RELEASE = 0x0,
    NVME_RESERVE_CLEAR   = 0x1,
};


enum NvmeReservationNotificationLogPageType {
    NVME_RESERVATION_LOG_PAGE_EMPTY = 0x0,
    NVME_REGISTRATION_PREEMPTED     = 0x1,
    NVME_RESERVATION_RELEASED       = 0x2,
    NVME_RESERVATION_PREEMPTED      = 0x3,
};


struct NvmeReservationNotificationLog {

    U64 logPageCount;

    U8 type;

    U8 numAvailLogPages;
    U8 reserved[2];
    U32 nsid;
    U8 reserved1[48];
};




#define NVME_REGISTRATION_PREEMPTED_MASK (1U << 1)

#define NVME_RESERVATION_RELEASED_MASK (1U << 2)

#define NVME_RESERVATION_PREEMPTED_MASK (1U << 3)




#define NVME_LOG_PAGE_DW10_SPICE(dw10, lid,  logLen) (dw10) = ((((logLen) >> 2) - 1) << 16) | (lid)
#define NVME_LOG_PAGE_RETAIN_BIT_SET(dw10)          (dw10) |= 1UL << 15


enum NvmeLogPage {



    NVME_LOG_ERROR = 0x01,


    NVME_LOG_HEALTH_INFORMATION = 0x02,


    NVME_LOG_FIRMWARE_SLOT = 0x03,


    NVME_LOG_CHANGED_NS_LIST = 0x04,


    NVME_LOG_COMMAND_EFFECTS_LOG = 0x05,


    NVME_LOG_DEVICE_SELF_TEST = 0x06,


    NVME_LOG_TELEMETRY_HOST_INITIATED = 0x07,


    NVME_LOG_TELEMETRY_CTRLR_INITIATED = 0x08,




    NVME_LOG_DISCOVERY = 0x70,




    NVME_LOG_RESERVATION_NOTIFICATION = 0x80,


    NVME_LOG_SANITIZE_STATUS = 0x81,




};

enum {
	NVME_NO_LOG_LSP       = 0x0,
	NVME_NO_LOG_LPO       = 0x0,
	NVME_LOG_ANA_LSP_RGO  = 0x1,
	NVME_TELEM_LSP_CREATE = 0x1,
};


struct NvmeErrorInformationEntry {
    U64 errorCount;
    U16 sqid;
    U16 cid;
    struct NvmeCmdStatus status;
    U16 errorLocation;
    U64 lba;
    U32 nsid;
    U8 vendorSpecific;
    U8 trtype;
    U8 reserved30[2];
    U64 commandSpecific;
    U16 trtypeSpecific;
    U8 reserved42[22];
};


union NvmeCriticalWarningState {
    U8 raw;

    struct {
        U8 availableSpare       : 1,
           temperature          : 1,
           deviceReliability    : 1,
           readOnly             : 1,
           volatileMemoryBackup : 1,
           persistentMemRO      : 1,
           reserved             : 2;
    };
};



#ifndef _WINDOWS
typedef struct __attribute__((packed)) __attribute__((aligned))
NvmeHealthInformationPage {
#else
#pragma pack(1)
typedef struct NvmeHealthInformationPage {
#endif
    union NvmeCriticalWarningState criticalWarning;

    U16 temperature;
    U8 availableSpare;
    U8 availableSpareThreshold;
    U8 percentageUsed;
    U8 enduGrpCritWarnSumry;

    U8 reserved[25];


    U64 dataUnitsRead[2];

    U64 dataUnitsWritten[2];

    U64 hostReadCommands[2];
    U64 hostWriteCommands[2];

    U64 controllerBusyTime[2];
    U64 powerCycles[2];
    U64 powerOnHours[2];
    U64 unsafeShutdowns[2];
    U64 mediaErrors[2];
    U64 numErrorInfoLogEntries[2];

    U32 warningTempTime;
    U32 criticalTempTime;
    U16 tempSensor[8];

    U8 reserved2[296];
}NvmeHealthInformationPage_s;


#ifdef _WINDOWS
#pragma pack()
#endif

struct NvmeCmdsAndEffectEntry {

    U16 csupp : 1;


    U16 lbcc : 1;


    U16 ncc : 1;


    U16 nic : 1;


    U16 ccc : 1;

    U16 reserved1 : 11;

    U16 cse : 3;

    U16 reserved2 : 13;
};


struct NvmeCmdsAndEffectLogPage {

    struct NvmeCmdsAndEffectEntry adminCmdsSupported[256];


    struct NvmeCmdsAndEffectEntry ioCmdsSupported[256];

    U8 reserved0[2048];
};



#ifndef _WINDOWS
struct __attribute__((packed)) NvmeSelfTestResEntry {
#else
#pragma pack(1)
struct NvmeSelfTestResEntry {
#endif
    U8  operationRes : 4,   
        selfTestCode : 4;   
    U8  segmentNum;         
    U8  nsIdValid : 1,      
        flbaValid : 1,      
        sctValid  : 1,      
        scValid   : 1,      
        reserved1 : 4;
    U8  reserved2;
    U64 poh;           
    U32 nsId;          
    U64 failingLba;    
    U8  sct       : 3, 
        reserved3 : 5;
    U8  sc;            
    U16 vendorSpecific;
}; 
#ifdef _WINDOWS
#pragma pack()
#endif
#ifndef _WINDOWS
typedef struct __attribute__((packed)) NvmeSelfTestLogPage {
#else
#pragma pack(1)
typedef struct NvmeSelfTestLogPage {
#endif

    U8  currtOpt    : 4,   
        reserved1   : 4;
    U8  currtComplt : 7,   
        reserved2   : 1;
    U16 reserved3;
    struct NvmeSelfTestResEntry newSelfTestRes[20];
}NvmeSelfTestLogPage_s;
#ifdef _WINDOWS
#pragma pack()
#endif


struct NvmeTelemetryLogPageHdr {

    U8 lpi;
    U8 rsvd[4];
    U8 ieeeOui[3];

    U16 dalb1;

    U16 dalb2;

    U16 dalb3;
    U8 rsvd1[368];

    U8 ctrlrAvail;

    U8 ctrlrGen;

    U8 rsnident[128];
    U8 telemetryDatablock[0];
};



enum NvmeSanitizeStatusType {
    NVME_NEVER_BEEN_SANITIZED       = 0x0,
    NVME_RECENT_SANITIZE_SUCCESSFUL = 0x1,
    NVME_SANITIZE_IN_PROGRESS       = 0x2,
    NVME_SANITIZE_FAILED            = 0x3,
};


struct NvmeSanitizeStatusSstat {
    U16 status : 3;
    U16 completePass : 5;
    U16 globalDataErase : 1;
    U16 reserved : 7;
};


struct NvmeSanitizeStatusLogPage {

    U16 sprog;

    struct NvmeSanitizeStatusSstat sstat;

    U32 scdw10;

    U32 etOverwrite;

    U32 etBlockErase;

    U32 etCryptoErase;
    U8 reserved[492];
};




enum NvmeAsyncEventType {

    NVME_ASYNC_EVENT_TYPE_ERROR = 0x0,

    NVME_ASYNC_EVENT_TYPE_SMART = 0x1,

    NVME_ASYNC_EVENT_TYPE_NOTICE = 0x2,



    NVME_ASYNC_EVENT_TYPE_IO = 0x6,

    NVME_ASYNC_EVENT_TYPE_VENDOR = 0x7,
};


enum NvmeAsyncEventInfoError {

    NVME_ASYNC_EVENT_WRITE_INVALID_DB = 0x0,

    NVME_ASYNC_EVENT_INVALID_DB_WRITE = 0x1,

    NVME_ASYNC_EVENT_DIAGNOSTIC_FAILURE = 0x2,

    NVME_ASYNC_EVENT_PERSISTENT_INTERNAL = 0x3,

    NVME_ASYNC_EVENT_TRANSIENT_INTERNAL = 0x4,

    NVME_ASYNC_EVENT_FW_IMAGE_LOAD = 0x5,


};


enum NvmeAsyncEventInfoSmart {

    NVME_ASYNC_EVENT_SUBSYSTEM_RELIABILITY = 0x0,

    NVME_ASYNC_EVENT_TEMPERATURE_THRESHOLD = 0x1,

    NVME_ASYNC_EVENT_SPARE_BELOW_THRESHOLD = 0x2,


};


enum NvmeAsyncEventInfoNotice {

    NVME_ASYNC_EVENT_NS_ATTR_CHANGED = 0x0,

    NVME_ASYNC_EVENT_FW_ACTIVATION_START = 0x1,

    NVME_ASYNC_EVENT_TELEMETRY_LOG_CHANGED = 0x2,


};


enum NvmeAsyncEventInfoNvmCommandSet {

    NVME_ASYNC_EVENT_RESERVATION_LOG_AVAIL = 0x0,

    NVME_ASYNC_EVENT_SANITIZE_COMPLETED = 0x1,


};


union NvmeAsyncEventCompletion {
    U32 raw;
    struct {
        U32 asyncEventType : 3;
        U32 reserved1 : 5;
        U32 asyncEventInfo : 8;
        U32 logPageIdentifier : 8;
        U32 reserved2 : 8;
    };
};



union NvmeFeatArbitration {
    U32 raw;
    struct {

        U32 ab : 3;

        U32 reserved : 5;


        U32 lpw : 8;


        U32 mpw : 8;


        U32 hpw : 8;
    };
};



union NvmeFeatPowerManagement {
    U32 raw;
    struct {

        U32 ps : 5;


        U32 wh : 3;

        U32 reserved : 24;
    };
};



union NvmeFeatLbaRangeType {
    U32 raw;
    struct {

        U32 num : 6;

        U32 reserved : 26;
    };
};



union NvmeFeatTemperatureThreshold {
    U32 raw;
    struct {

        U32 tmpth : 16;


        U32 tmpsel : 4;


        U32 thsel : 2;

        U32 reserved : 10;
    };
};




union NvmeFeatErrorRecovery {
    U32 raw;
    struct {

        U32 tler : 16;


        U32 dulbe : 1;

        U32 reserved : 15;
    };
};



union NvmeFeatVolatileWriteCache {
    U32 raw;
    struct {

        U32 wce : 1;

        U32 reserved : 31;
    };
};



union NvmeFeatNumberOfQueues {
    U32 raw;
    struct {

        U32 nsqr : 16;


        U32 ncqr : 16;
    };
};



union NvmeFeatInterruptCoalescing {
    U32 raw;
    struct {

        U32 thr : 8;


        U32 time : 8;

        U32 reserved : 16;
    };
};




union NvmeFeatInterruptVectorConfiguration {
    U32 raw;
    struct {

        U32 iv : 16;


        U32 cd : 1;

        U32 reserved : 15;
    };
};




union NvmeFeatWriteAtomicity {
    U32 raw;
    struct {

        U32 dn : 1;

        U32 reserved : 31;
    };
};




union NvmeFeatAsyncEventConfiguration {
    U32 raw;
    struct {
        union NvmeCriticalWarningState critWarn;
        U32 nsAttrNotice : 1;
        U32 fwActivationNotice : 1;
        U32 telemetryLogNotice : 1;
        U32 reserved : 21;
    };
};



#define NvmeAsyncEventConfig NvmeFeatAsyncEventConfiguration



union NvmeFeatAutonomousPowerStateTransition {
    U32 raw;
    struct {

        U32 apste : 1;

        U32 reserved : 31;
    };
};




union NvmeFeatHostMemBuffer {
    U32 raw;
    struct {

        U32 ehm : 1;


        U32 mr : 1;

        U32 reserved : 30;
    };
};



union NvmeFeatKeepAliveTimer {

    U32 kato;
};




union NvmeFeatHostControlledThermalManagement {
    U32 raw;
    struct {

        U32 tmt2 : 16;


        U32 tmt1 : 16;
    };
};





union NvmeFeatNonOperationalPowerStateConfig {
    U32 raw;
    struct {

        U32 noppme : 1;

        U32 reserved : 31;
    };
};





union NvmeFeatSoftwareProgressMarker {
    U32 raw;
    struct {

        U32 pbslc : 8;
        U32 reserved : 24;
    };
};




union NvmeFeatHostIdentifier {
    U32 raw;
    struct {

        U32 exhid : 1;

        U32 reserved : 31;
    };
};



struct NvmeFirmwarePage {
    U8 afi;

    U8 reserved[7];
    U8 revision[7][8];
    U8 reserved2[448];
};



enum NvmeNsAttachType {

    NVME_NS_CTRLR_ATTACH = 0x0,


    NVME_NS_CTRLR_DETACH = 0x1,


};


enum NvmeNsManagementType {

    NVME_NS_MANAGEMENT_CREATE = 0x0,


    NVME_NS_MANAGEMENT_DELETE = 0x1,


};

struct NvmeNsList {
    U32 nsList[1024];
};


enum NvmeNidt {

    NVME_NIDT_EUI64 = 0x01,


    NVME_NIDT_NGUID = 0x02,


    NVME_NIDT_UUID = 0x03,
};

struct NvmeNsIdDesc {

    U8 nidt;


    U8 nidl;

    U8 reserved2;
    U8 reserved3;


    U8 nid[];
};


typedef struct NvmeCtrlrList {
    U16 ctrlrCount;
    U16 ctrlrList[2047];
}NvmeCtrlrList_s;


enum NvmeSecureEraseSetting {
    NVME_FMT_NVM_SES_NO_SECURE_ERASE = 0x0,
    NVME_FMT_NVM_SES_USER_DATA_ERASE = 0x1,
    NVME_FMT_NVM_SES_CRYPTO_ERASE    = 0x2,
};

enum NvmePiLocation {
    NVME_FMT_NVM_PROTECTION_AT_TAIL = 0x0,
    NVME_FMT_NVM_PROTECTION_AT_HEAD = 0x1,
};

enum NvmePiType {
    NVME_FMT_NVM_PROTECTION_DISABLE = 0x0,
    NVME_FMT_NVM_PROTECTION_TYPE1   = 0x1,
    NVME_FMT_NVM_PROTECTION_TYPE2   = 0x2,
    NVME_FMT_NVM_PROTECTION_TYPE3   = 0x3,
};

enum NvmeMetadataSetting {
    NVME_FMT_NVM_METADATA_TRANSFER_AS_BUFFER = 0x0,
    NVME_FMT_NVM_METADATA_TRANSFER_AS_LBA    = 0x1,
};

struct NvmeFormat {
    U32 lbaf : 4;
    U32 ms : 1;
    U32 pi : 3;
    U32 pil : 1;
    U32 ses : 3;
    U32 reserved : 20;
};


struct NvmeProtectionInfo {
    U16 guard;
    U16 appTag;
    U32 refTag;
};



enum NvmeFwCommitAction {
    NVME_FW_COMMIT_REPLACE_IMG = 0x0,
    NVME_FW_COMMIT_REPLACE_AND_ENABLE_IMG = 0x1,
    NVME_FW_COMMIT_ENABLE_IMG = 0x2,
    NVME_FW_COMMIT_RUN_IMG = 0x3,
};


enum NvmeDeviceSelfTestCode {
    NVME_SHORT_DEVICE_SELF_TEST_OPT = 0x1,
    NVME_EXTENDED_DEVICE_SELF_TEST_OPT = 0x2,
    NVME_ABORT_DEVICE_SELF_TEST_OPT = 0xF,
};


struct NvmeFwCommit {
    U32 fs : 3;
    U32 ca : 3;
    U32 reserved : 26;
};


#define NVME_CPL_IS_SUCCESS(cpl) (!NVME_CMD_STATUS_NOT_SUCCESS(cpl))

#define NVME_CMD_STATUS_NOT_SUCCESS(cpl)                                       \
        ((cpl)->status.sc != NVME_SC_SUCCESS ||                                \
         (cpl)->status.sct != NVME_SCT_GENERIC)

#define NVME_STATUS_CMD_ABORT_REQUEST(cpl)                                     \
        ((cpl)->status.sc == NVME_SC_ABORTED_BY_REQUEST &&                     \
         (cpl)->status.sct == NVME_SCT_GENERIC)

#define NVME_CPL_PI_IS_ERROR(cpl)                                              \
    ((cpl)->status.sct == NVME_SCT_MEDIA_ERROR &&                              \
     ((cpl)->status.sc == NVME_SC_GUARD_CHECK_ERROR ||                         \
      (cpl)->status.sc == NVME_SC_APPLICATION_TAG_CHECK_ERROR ||               \
      (cpl)->status.sc == NVME_SC_REFERENCE_TAG_CHECK_ERROR))



#define NVME_IO_FLAGS_PRCHK_REFTAG (1U << 26)

#define NVME_IO_FLAGS_PRCHK_APPTAG (1U << 27)

#define NVME_IO_FLAGS_PRCHK_GUARD (1U << 28)

#define NVME_IO_FLAGS_PRACT             (1U << 29)
#define NVME_IO_FLAGS_FORCE_UNIT_ACCESS (1U << 30)
#define NVME_IO_FLAGS_LIMITED_RETRY     (1U << 31)

#define NVME_FORMAT_SES_BIT_MASK    0x7UL   
#define NVME_FORMAT_SES_BIT_SHIFT   9
#define NVME_LBAF_MASK              0xFUL
#define NVME_FORMAT_SES_TYPE_NONE   0x0
#define NVME_FORMAT_SES_TYPE_USER_DATA_ERASE  0x1
#define NVME_FORMAT_SES_TYPE_CRYPTO_ERASE 0x2
typedef union NvmeFormatCmdDw10 {
    struct  {
        U32   lbaf:4;
        U32   mset:1;
        U32   pi:3;
        U32   pil:1;
        U32   ses:1;
        U32   reseved:3;
    };
    U32 rawVal;
} NvmeFormatCmdDw10_u;

#ifdef __cplusplus
}
#endif

#endif

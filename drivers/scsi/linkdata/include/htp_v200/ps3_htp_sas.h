#ifndef _PS3_IODT_H_
#define _PS3_IODT_H_

#include "ps3_htp_def.h"
    

enum IodtProtocolType {
    PROTOCOL_SMP  = 0b000,
    PROTOCOL_SSP  = 0b001,
    PROTOCOL_STP  = 0b010,
    PROTOCOL_DIRT = 0b111,
};

enum IodtFrameType {
    FRAMETYPE_SSP_CMD      = 0b001,
    FRAMETYPE_SSP_SMP      = 0b001,
    FRAMETYPE_SATA_NONDATA = 0b001,

    FRAMETYPE_SSP_TASK = 0b010,
    FRAMETYPE_SATA_PIO = 0b010,

    FRAMETYPE_SATA_DMA = 0b011,

    FRAMETYPE_SATA_FPDMA = 0b100,

    FRAMETYPE_SATA_ATAPI = 0b101,
    FRAMETYPE_DIRECT = 0b111,
};
    
enum IodtIuSrc {
    IU_SRC_MEM   = 0b00,
    IU_SRC_IODT  = 0b01,
    IU_SRC_TUPLE = 0b10,
    IU_SRC_SATA  = 0b11,
};

enum IodtSgeMode {
    IODT_SGEMODE_DIRECT = 0x0,
    IODT_SGEMODE_SGL    = 0x1,
};

enum IodtEedpMode {
    EEDP_MODE_CHECK   = 0x0,
    EEDP_MODE_INSERT  = 0x1,
    EEDP_MODE_REPLACE = 0x2,
    EEDP_MODE_RMV     = 0x3,
};

enum AbortCtrl {

    ABT_SASSATA_TASK  = 0b00,
    ABT_SAS_TASKSET   = 0b01,
    ABT_LOCAL_BY_DISK = 0b10,
    ABT_LOCAL_BY_PORT = 0b11,

    ABT_MGT_IO           = 0b00,
    ABT_SOFTRESET        = 0b01,
    ABT_READ_NCQ_ERR_LOG = 0b10,
    ABT_SMP              = 0b11,
};

enum DirectFlag {
    DIRECT_FLAG_NORMAL = 0b00,
    DIRECT_FLAG_DIRECT = 0b10,
};

enum CmdWordType {
    CMD_WORD_TYPE_ABORT = 0b00,
    CMD_WORD_TYPE_MGT   = 0b01,
    CMD_WORD_TYPE_READ  = 0b10,
    CMD_WORD_TYPE_WRITE = 0b11,
};

enum SmpFrameType {
    SMP_REQ  = 0x40,
    SMP_RESP = 0x41,
};

enum eedpMode {
    EEDP_NONE = 0b0000,
    EEDP_CHK  = 0b0001,
    EEDP_INST = 0b0010,
    EEDP_REPL = 0b0100,
    EEDP_RMV  = 0b1000,
};

typedef union IoDmaCfg {
    struct {
        U8 eedpEn : 1;
        U8 eedpSgMod : 1;
        U8 twoSglMod : 1;
        U8 sgMode : 1;
        U8 eedpMode : 4;
    };
    U8 byte;
} IoDmaCfg_u;


typedef struct __attribute__((packed)) SspTaskFrameIu {
    U64 LUN;
    U16 reserved0;
    U8 function;
    U8 reserved1;
    U16 manageTag;
    U8 reserved2[14];
} SspTaskFrameIu_s;


typedef union __attribute__((packed)) SmpFrameIu {
    struct {
        U8 frameType;
        U8 reqestBytes[31];
    };
    U8 smpIURaw[32];
} SmpFrameIu_s;


typedef struct __attribute__((packed)) DfifoWordCommon {

    union {
        struct {
            U16 type : 2;
            U16 rsv1 : 2;
            U16 direct : 2; 
            U16 rFifoID : 4;
            U16 rsv2 : 6;
        };
        U16 WD0;
    };


    union{

        struct {
            union{

                struct {
                    U16 darID : 13;
                    U16 rsv3 : 2;

                    U16 function : 1;
                };

                struct{
                    U16 manageIptt : 13;
                };
            };  
        };

        struct {
            union{

                struct{
                    U16 reqFrameID : 13;  
                };


                struct{
                    U16 manageReqFrameID : 13;
                };
            };
        };
        U16 WD1;
    };


    union {
        struct {
            U16 phyDiskID : 12;
            U16 rsv4 : 2;
            U16 abortCtrl : 2;
        };
        U16 WD2;
    };
}DfifoWordCommon_s;

typedef struct __attribute__((packed)) IODT_V1 {
    union {

        struct __attribute__((packed)) {
            union {
                struct {
                    union {
                        struct {
                            U8 protocolType : 3;
                            U8 frameType : 3;   
                            U8 iuSrc : 2;       
                        };
                        U8 byte0;
                    };

                    IoDmaCfg_u dmaCfg;
                };
                U16 config;
            };




            U16 cmdLen : 9;
            U16 rsv0 : 7;

            union {
                struct {
                    U32 taskDarID : 13;
                    U32 : 19;
                };
                struct {
                    U32 dataBufLenDWAlign : 24;

                    U32 rsvd0 : 1;
                    U32 cmdDir : 1;
                    U32 refTagEn : 1;  
                    U32 appTagEn : 1;  
                    U32 guardTagEn : 1;
                    U32 refTagInc : 1;
                    U32 rsv1 : 1;
                    U32 aborted : 1;
                };
            };
        };
        U64 QW0;
    };


    union {
        struct __attribute__((packed)) {
            DfifoWordCommon_s commonWord;
            U16 rsv2 : 1;
            U16 sataCtl : 1;
            U16 rsv3 : 2;
            U16 sasCtl : 1;
            U16 sataByteBlock0 : 1;
            U16 sataByteBlock1 : 1;
            U16 rsv4 : 9;
        };
        U64 QW1;
    };


    union {
        U64 dataBaseAddr;
        U64 QW2;
    };


    union {
        U64 eedpBaseAddr;
        U64 QW3;
    };


    union {

        struct {
            U64 cmdIUAddr;
            U64 rsv9;

            U64 refTag : 32;
            U64 appTag : 16;
            U64 rsv10 : 16;

            U64 rsv11;
        } A;

        union {
            U8 cdb[32];
            SspTaskFrameIu_s taskIU;
            SmpFrameIu_s smpIU;
        } B;


        struct {
            U64 opCode : 8;
            U64 rsv12 : 56;

            U64 lba : 48;
            U64 rsv13 : 16;

            U64 refTag : 32;
            U64 appTag : 16;
            U64 rsv14 : 16; 

            U64 rsv15;
        } C;

        struct {
            U32 ataCmd : 8;
            U32 ataDev : 8;
            U32 ataCtl : 8;
            U32 ataIcc : 8;
            U32 ataSecCnt : 16;
            U32 ataFeature : 16;

            union {
                struct {
                    U64 ataLba : 48;
                    U64 rsv16 : 16;
                };
                struct {
                    U8 lba0;
                    U8 lba1;
                    U8 lba2;
                    U8 lba3;
                    U8 lba4;
                    U8 lba5;
                };
                U8 lba[6];
            };

            U32 ataAuxiliary;
            U32 rsv17;

            U64 rsv18;
        } D;
    };
} IODT_V1_s;

enum {
	CMD_LEN_THR = 32,
	CMD_LEN_S = 7,
	CMD_LEN_L = 11,
};

#endif
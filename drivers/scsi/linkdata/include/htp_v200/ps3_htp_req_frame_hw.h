#ifndef _PS3_HTP_REQ_FRAME_HW_H_
#define _PS3_HTP_REQ_FRAME_HW_H_

#include "ps3_htp_def.h"

#define ENCODE_CCS_XFERLEN(x)  (((U32)(x) >> 2)) 
#define DECODE_CCS_XFERLEN(x)  (((U32)(x) << 2)) 

#ifdef _WINDOWS
#define __attribute__(x)
#pragma pack(push, 1)
#endif


struct PS3NvmeSglDesc {
    U64 addr;
    union {
        struct {
            U8 reserved[7];
            U8 subtype : 4;
            U8 type : 4;   
        } generic;         

        struct {
            U32 length;
            U8 reserved[3];
            U8 subtype : 4;
            U8 type : 4;   
        } unkeyed;         

        struct {
            U64 length : 24;
            U64 key : 32;   
            U64 subtype : 4;
            U64 type : 4;   
        } keyed;
    };
};

struct PS3NvmeCmdDw0_9 {
    U16 opcode : 8;
    U16 fuse : 2;
    U16 reserved1 : 4;
    U16 psdt : 2;
    U16 cID;

    U32 nsID;

    U32 reserved2;
    U32 reserved3;

    U64 mPtr;


    union {
        struct {
            U64 prp1;              
            U64 prp2;              
        } prp;                     
        struct PS3NvmeSglDesc sgl1;

    } dPtr;
};


typedef struct PS3NvmeCommonCmd {
    struct PS3NvmeCmdDw0_9 cDW0_9;

    U32 cDW10;
    U32 cDW11;
    U32 cDW12;
    U32 cDW13;
    U32 cDW14;
    U32 cDW15;
} PS3NvmeCommonCmd_s;

typedef struct PS3NvmeRWCmd {
    struct PS3NvmeCmdDw0_9 cDW0_9;

    U32 sLbaLo;
    U32 sLbaHi;
    U32 numLba;
    U32 cDW13; 
    U32 cDW14; 
    U32 cDW15; 
} PS3NvmeRWCmd_s;


typedef union PS3NvmeReqFrame {
    PS3NvmeCommonCmd_s commonReqFrame;
    PS3NvmeRWCmd_s rwReqFrame;
} PS3NvmeReqFrame_u;

#ifdef _WINDOWS
#pragma pack(pop)
#endif

#endif 

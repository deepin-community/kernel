//*****************************************************************************
//  Copyright (c) 2021 Glenfly Tech Co., Ltd.. 
//  All Rights Reserved.
//
//  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Glenfly Tech Co., Ltd..;
//  the contents of this file may not be disclosed to third parties, copied or
//  duplicated in any form, in whole or in part, without the prior written
//  permission of Glenfly Tech Co., Ltd..
//
//  The copyright of the source code is protected by the copyright laws of the People's
//  Republic of China and the related laws promulgated by the People's Republic of China
//  and the international covenant(s) ratified by the People's Republic of China. 
//*****************************************************************************

#ifndef _GPCPBE_REGISTER_H
#define _GPCPBE_REGISTER_H


#ifndef        GPCPBE_BLOCKBASE_INF
#define    GPCPBE_BLOCKBASE_INF
#define    BLOCK_GPCPBE_VERSION 1
#define    BLOCK_GPCPBE_TIMESTAMP "2018/6/8 17:55:28"
#define    GPCPBE_BLOCK                                               0x15
#define    GPCPBE_REG_START                                           0x0
#define    GPCPBE_REG_END                                             0x1C8
#define    GPCPBE_REG_LIMIT                                           0x1C8
#endif


#define        Reg_Sto_Cfg_Offset                                         0x0
#define        Reg_Sto_Prim_Info_Offset                                   0x1
#define        Reg_Sto_Cnt_Cfg_Offset                                     0x2
#define        Reg_Sto_Stm_Buf_Mask_Offset                                0x3
#define        Reg_Sto_Reserved0_Offset                                   0x4
#define        Reg_Sto_Buf_Offset                                         0x8
#define        Reg_Sto_Stm_Cnt_Offset                                     0x38
#define        Reg_Sto_Stm_Map_Offset                                     0x48


typedef enum
{
    STO_CFG_STO_EN_DISABLED                  = 0,
    STO_CFG_STO_EN_ENABLED                   = 1,

} STO_CFG_STO_EN;
typedef enum
{
    STO_CFG_STREAM0_EN_DISABLE               = 0,
    STO_CFG_STREAM0_EN_ENABLE                = 1,
} STO_CFG_STREAM0_EN;
typedef enum
{
    STO_CFG_STREAM1_EN_DISABLE               = 0,
    STO_CFG_STREAM1_EN_ENABLE                = 1,
} STO_CFG_STREAM1_EN;
typedef enum
{
    STO_CFG_STREAM2_EN_DISABLE               = 0,
    STO_CFG_STREAM2_EN_ENABLE                = 1,
} STO_CFG_STREAM2_EN;
typedef enum
{
    STO_CFG_STREAM3_EN_DISABLE               = 0,
    STO_CFG_STREAM3_EN_ENABLE                = 1,
} STO_CFG_STREAM3_EN;
typedef enum
{
    STO_CFG_BUFFER0_EN_DISABLE               = 0,
    STO_CFG_BUFFER0_EN_ENABLE                = 1,
} STO_CFG_BUFFER0_EN;
typedef enum
{
    STO_CFG_BUFFER1_EN_DISABLE               = 0,
    STO_CFG_BUFFER1_EN_ENABLE                = 1,
} STO_CFG_BUFFER1_EN;
typedef enum
{
    STO_CFG_BUFFER2_EN_DISABLE               = 0,
    STO_CFG_BUFFER2_EN_ENABLE                = 1,
} STO_CFG_BUFFER2_EN;
typedef enum
{
    STO_CFG_BUFFER3_EN_DISABLE               = 0,
    STO_CFG_BUFFER3_EN_ENABLE                = 1,
} STO_CFG_BUFFER3_EN;
typedef enum
{
    STO_CNT_CFG_STM0_PRIM_CNT_OFF            = 0,
    STO_CNT_CFG_STM0_PRIM_CNT_ON             = 1,
} STO_CNT_CFG_STM0_PRIM_CNT;
typedef enum
{
    STO_CNT_CFG_STM1_PRIM_CNT_OFF            = 0,
    STO_CNT_CFG_STM1_PRIM_CNT_ON             = 1,
} STO_CNT_CFG_STM1_PRIM_CNT;
typedef enum
{
    STO_CNT_CFG_STM2_PRIM_CNT_OFF            = 0,
    STO_CNT_CFG_STM2_PRIM_CNT_ON             = 1,
} STO_CNT_CFG_STM2_PRIM_CNT;
typedef enum
{
    STO_CNT_CFG_STM3_PRIM_CNT_OFF            = 0,
    STO_CNT_CFG_STM3_PRIM_CNT_ON             = 1,
} STO_CNT_CFG_STM3_PRIM_CNT;
typedef enum
{
    STO_CNT_CFG_OFFSET0_W_EN_OFF             = 0,
    STO_CNT_CFG_OFFSET0_W_EN_ON              = 1,
} STO_CNT_CFG_OFFSET0_W_EN;
typedef enum
{
    STO_CNT_CFG_OFFSET1_W_EN_OFF             = 0,
    STO_CNT_CFG_OFFSET1_W_EN_ON              = 1,
} STO_CNT_CFG_OFFSET1_W_EN;
typedef enum
{
    STO_CNT_CFG_OFFSET2_W_EN_OFF             = 0,
    STO_CNT_CFG_OFFSET2_W_EN_ON              = 1,
} STO_CNT_CFG_OFFSET2_W_EN;
typedef enum
{
    STO_CNT_CFG_OFFSET3_W_EN_OFF             = 0,
    STO_CNT_CFG_OFFSET3_W_EN_ON              = 1,
} STO_CNT_CFG_OFFSET3_W_EN;
typedef enum
{
    STO_STM_MAPPING_C0_V_INVALID             = 0,
    STO_STM_MAPPING_C0_V_VALID               = 1,
} STO_STM_MAPPING_C0_V;
typedef enum
{
    STO_STM_MAPPING_C1_V_INVALID             = 0,
    STO_STM_MAPPING_C1_V_VALID               = 1,
} STO_STM_MAPPING_C1_V;





typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Sto_En                    : 1;
        unsigned int Stream0_En                : 1;
        unsigned int Stream1_En                : 1;
        unsigned int Stream2_En                : 1;
        unsigned int Stream3_En                : 1;
        unsigned int Buffer0_En                : 1;
        unsigned int Buffer1_En                : 1;
        unsigned int Buffer2_En                : 1;
        unsigned int Buffer3_En                : 1;
        unsigned int Reserved                  : 23;
    } reg;
} Reg_Sto_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Prim_Vtx_Num              : 6;

        unsigned int Vtx_Out_Size              : 6;

        unsigned int Reserved                  : 20;
    } reg;
} Reg_Sto_Prim_Info;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Stm0_Prim_Cnt             : 1;
        unsigned int Stm1_Prim_Cnt             : 1;
        unsigned int Stm2_Prim_Cnt             : 1;
        unsigned int Stm3_Prim_Cnt             : 1;
        unsigned int Offset0_W_En              : 1;
        unsigned int Offset1_W_En              : 1;
        unsigned int Offset2_W_En              : 1;
        unsigned int Offset3_W_En              : 1;
        unsigned int Reserved                  : 24;
    } reg;
} Reg_Sto_Cnt_Cfg;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Stm0_Buf_Mask             : 4;

        unsigned int Stm1_Buf_Mask             : 4;

        unsigned int Stm2_Buf_Mask             : 4;

        unsigned int Stm3_Buf_Mask             : 4;

        unsigned int Reserved                  : 16;
    } reg;
} Reg_Sto_Stm_Buf_Mask;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Sto_Reserved0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Addr_Low                  : 32;

    } reg;
} Reg_Sto_Buf_Addr0;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Addr_High                 : 8;

        unsigned int Reserved                  : 24;
    } reg;
} Reg_Sto_Buf_Addr1;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Range                     : 32;
    } reg;
} Reg_Sto_Buf_Range;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Stride                    : 12;

        unsigned int Reserved                  : 20;
    } reg;
} Reg_Sto_Buf_Stride;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Offset                    : 32;

    } reg;
} Reg_Sto_Buf_Start;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Line_Num                  : 4;


        unsigned int Reserved                  : 28;
    } reg;
} Reg_Sto_Buf_Map_Usage;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Comp_Mask                 : 32;

    } reg;
} Reg_Sto_Buf_Map_Mask;

typedef struct _Group_Sto_Buf
{
    Reg_Sto_Buf_Addr0                reg_Sto_Buf_Addr0;
    Reg_Sto_Buf_Addr1                reg_Sto_Buf_Addr1;
    Reg_Sto_Buf_Range                reg_Sto_Buf_Range;
    Reg_Sto_Buf_Stride               reg_Sto_Buf_Stride;
    Reg_Sto_Buf_Start                reg_Sto_Buf_Start;
    Reg_Sto_Buf_Map_Usage            reg_Sto_Buf_Map_Usage;
    Reg_Sto_Buf_Map_Mask             reg_Sto_Buf_Map_Mask[6];
} Reg_Sto_Buf_Group;

typedef union
{
    unsigned int uint[2];
    struct
    {
        unsigned long long   Count             : 64;

    } reg;
} Reg_Sto_Prim_Written_Total;

typedef union
{
    unsigned int uint[2];
    struct
    {
        unsigned long long   Count             : 64;


    } reg;
} Reg_Sto_Prim_Needed_Total;

typedef struct _Group_Sto_Stm_Cnt
{
    Reg_Sto_Prim_Written_Total       reg_Sto_Prim_Written_Total;
    Reg_Sto_Prim_Needed_Total        reg_Sto_Prim_Needed_Total;
} Reg_Sto_Stm_Cnt_Group;

typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int C0_V                      : 1;
        unsigned int C0_Buf                    : 2;
        unsigned int C0_Addr                   : 8;
        unsigned int Reserved0                 : 5;
        unsigned int C1_V                      : 1;
        unsigned int C1_Buf                    : 2;
        unsigned int C1_Addr                   : 8;
        unsigned int Reserved1                 : 5;
    } reg;
} Reg_Sto_Stm_Mapping;

typedef struct _Group_Sto_Stm_Map
{
    Reg_Sto_Stm_Mapping              reg_Sto_Stm_Mapping[96];
} Reg_Sto_Stm_Map_Group;

typedef struct _Gpcpbe_regs
{
    Reg_Sto_Cfg                      reg_Sto_Cfg;
    Reg_Sto_Prim_Info                reg_Sto_Prim_Info;
    Reg_Sto_Cnt_Cfg                  reg_Sto_Cnt_Cfg;
    Reg_Sto_Stm_Buf_Mask             reg_Sto_Stm_Buf_Mask;
    Reg_Sto_Reserved0                reg_Sto_Reserved0[4];
    Reg_Sto_Buf_Group                reg_Sto_Buf[4];
    Reg_Sto_Stm_Cnt_Group            reg_Sto_Stm_Cnt[4];
    Reg_Sto_Stm_Map_Group            reg_Sto_Stm_Map[4];
} Gpcpbe_regs;

#endif

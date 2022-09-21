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

#ifndef _TASBE_REG_H
#define _TASBE_REG_H


#ifndef        TASBE_BLOCKBASE_INF
#define    TASBE_BLOCKBASE_INF
#define    BLOCK_TASBE_VERSION 1
#define    BLOCK_TASBE_TIMESTAMP "10/13/2017 10:19:33 AM"
#define    TASBE_BLOCK                                                0x5
#define    TASBE_REG_START                                            0x0
#define    TASBE_REG_END                                              0x8
#define    TASBE_REG_LIMIT                                            0x8
#endif


#define        Reg_Tasbe_Reserved_Offset                                  0x1


typedef enum
{
    TASBE_CTRL_TBR_EN_DISABLE                = 0,
    TASBE_CTRL_TBR_EN_ENABLE                 = 1,
} TASBE_CTRL_TBR_EN;
typedef enum
{
    TASBE_CTRL_TBR_MODE_PER_DRAW             = 0,
    TASBE_CTRL_TBR_MODE_PER_FRAME            = 1,
} TASBE_CTRL_TBR_MODE;
typedef enum
{
    TASBE_CTRL_TILE_SIZE_256X128             = 0,
    TASBE_CTRL_TILE_SIZE_128X128             = 1,
    TASBE_CTRL_TILE_SIZE_128X64              = 2,
    TASBE_CTRL_TILE_SIZE_64X64               = 3,
    TASBE_CTRL_TILE_SIZE_64X32               = 4,
    TASBE_CTRL_TILE_SIZE_32X32               = 5,
    TASBE_CTRL_TILE_SIZE_32X16               = 6,
    TASBE_CTRL_TILE_SIZE_16X16               = 7,
    TASBE_CTRL_TILE_SIZE_16X8                = 8,
    TASBE_CTRL_TILE_SIZE_8X8                 = 9,
} TASBE_CTRL_TILE_SIZE;





typedef union
{
    unsigned int uint;
    struct
    {
        unsigned int Reserved                  : 32;
    } reg;
} Reg_Tasbe_Reserved;

typedef struct _Tasbe_regs
{
    struct
    {
        unsigned int Tbr_En                    : 1;
        unsigned int Tbr_Mode                  : 1;
        unsigned int Tile_Size                 : 4;
        unsigned int Attr_Num                  : 6;
        unsigned int Msaa_State                : 3;

        unsigned int Aa_En                     : 1;
        unsigned int Raster_Mode               : 1;
        unsigned int Reserved                  : 15;
    }  reg_Tasbe_Ctrl;
    Reg_Tasbe_Reserved               reg_Tasbe_Reserved[7];
} Tasbe_regs;

#endif

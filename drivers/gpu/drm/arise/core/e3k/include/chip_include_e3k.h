/*
 * Copyright © 2021 Glenfly Tech Co., Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */
#ifndef __CHIP_INCLUDE_E3K_H__
#define __CHIP_INCLUDE_E3K_H__

#undef __inline
#undef _inline
#undef __int64

#define __inline static __inline__
#define _inline  static __inline__
#define  __int64 long long

#if !defined(XFree86Server)

/* why define ULONG/DWORD as unsigned int in linux ?
 * long is 32-bit type in window, and window use ULONG/DWORD as a 32-bit value,
 * but in linux its size equal to OS ptr size. different between 32/64-bit OS,
 * So we can not define ULONG to unsigned long in linux, but define it as unsigned int for compatible
 */
typedef unsigned int   ULONG;
typedef unsigned int   DWORD;
typedef int            BOOL;
typedef unsigned char  BYTE;
typedef unsigned int   UINT;
typedef unsigned short WORD;
typedef unsigned int   U32;
typedef void           VOID;
typedef float          FLOAT;
typedef unsigned long long UINT64;

#endif

#include "./Chip/surface_format.h"
#include "./Chip/registerDef.h"
#include "./Chip/registercommands.h"
#include "stm_context_e3k.h"

typedef enum
{
    HWM_SYNC_VS_SLOT  = 0,
    HWM_SYNC_STO_SLOT = 1,
    HWM_SYNC_Z_SLOT = 2,
    HWM_SYNC_PS_SLOT  = 3,
    HWM_SYNC_WBU_SLOT = 4,
    HWM_SYNC_MXU_SLOT = 5,
    HWM_SYNC_L2_SLOT  = 6,
    HWM_SYNC_MAX_SLOT = HWM_SYNC_L2_SLOT + 1,
    HWM_SYNC_CONTEXT_LAST = HWM_SYNC_MAX_SLOT,
    HWM_SYNC_UMD_FLUSH_SLOT    = 7,
    HWM_SYNC_DRAIN_3DPIPE_SLOT = 8,
    // Above are slots used by 3D UMD
    HWM_SYNC_KMD_SLOT = 9,
    HWM_SYNC_SWITCH_SLOT  = 10,
    // Above are stored in context buffer

    //Slot used by video
    HWM_SYNC_VCP0_FE_SLOT = 16,
    HWM_SYNC_VCP0_BE_SLOT = 17,
    HWM_SYNC_VCP1_FE_SLOT = 18,
    HWM_SYNC_VCP1_BE_SLOT = 19,
    HWM_SYNC_VPP_SLOT     = 20,
    HWM_SYNC_VCP_BANDWIDTH = 21,
} HWM_SYNC_SLOT_E3K;


#define FENCE_VALUE_RESET                   0x000000FF
#define FENCE_VALUE_BEGIN_DMA               0x00000010
#define FENCE_VALUE_END_DMA_CSPFENCE        0x00000021
#define FENCE_VALUE_END_DMA_STOFENCE        0x00000022
#define FENCE_VALUE_CACHE_DMA_DFENCE        0x00000031
#define FENCE_VALUE_CACHE_DMA_ZFENCE        0x00000032
#define FENCE_VALUE_CACHE_DMA_UAVFEFENCE    0x00000033
#define FENCE_VALUE_CACHE_DMA_UAVBEFENCE    0x00000034
#define FENCE_VALUE_CACHE_DMA_PRECSPFENCE   0x00000035
#define FENCE_VALUE_CACHE_DMA_POSTCSPFENCE  0x00000036
#define FENCE_VALUE_CACHE_DMA_STOFENCE      0x00000037
#define FENCE_VALUE_CACHE_DMA_L2C_FENCE     0x00000038

#define FENCE_VALUE_INIT                    0x00000006
#define FENCE_VALUE_DUMMY_RB                0x00000007
#define FENCE_VALUE_RANGESET                0x00000003

#define FENCE_VALUE_FFC_D_FECNE (0x00000050)
#define FENCE_VALUE_2D_BLT_DST  (0x00000060)
#define FENCE_VALUE_3D_BLT_DST  (0x00000070)
#define FENCE_VALUE_3D_BLT_SRC  (0x00000071)




#define REG_FS_OUT_MAX_ATTRIBUTES_NUM           48

#define REG_FS_OUT_MAPPING_ATTRIBUTES_BITS      6
#define REG_FS_OUT_MAPPING_ATTRIBUTE_NUM        4
#define REG_FS_OUT_MAPPING_REGS_NUM             12

#define REG_FS_OUT_MASK_ATTRIBUTES_BITS         4
#define REG_FS_OUT_MASK_ATTRIBUTES_NUM          8
#define REG_FS_OUT_MASK_REGS_NUM                6

#define REG_FS_OUT_COMP_ATTRIBUTES_BITS         2
#define REG_FS_OUT_COMP_ATTRIBUTES_NUM          16
#define REG_FS_OUT_COMP_REGS_NUM                3


typedef enum ELITE_SHADER_STAGE
{
    ELITE_VS_STAGE  = 1,
    ELITE_HS_STAGE ,
    ELITE_DS_STAGE ,
    ELITE_GS_STAGE ,
    ELITE_PS_STAGE ,
}ELITE_SHADER_STAGE;

#define NEW_DUMP            1
#define MAX_T_SHARP_NUMBER  128
#define MAX_S_SHARP_NUMBER  16
#define MAX_TS_SHARP_NUMBER 32
#define MAX_U_SHARP_NUMBER  32

//**********************************************************************************************
//
//  RingBuffer command layout:
//
//    common pwm header and tail:
//    header: indicate which block/slice following dma will use
//    pwmIndicatorPair0
//
//    tail: indicate that this dma is finished
//    pwmIndicator3DPair1
//
//    Case 0 (From Render):
//        [offset] [command]
//      0x00-0x01:  Restore DMA/Restore CMD
//      0x02-0x03:  CommandDMA
//      0x04-0x05:  CacheFlushDMA
//      0x06-0x07:  Save DMA/Save CMD
//      0x08-0x0B:  Fence
//      0x0C-0x0E:  Skip for RB size 512bit
//      0x0F     :  Special Fence for possible switch to High Rb
//
//    Case 1(From Present/BuildPagingBuffer/RenderKM):
//        [offset] [command]
//      0x00-0x01:  Skip
//      0x02-0x03:  CommandDMA
//      0x04-0x05:  CacheFlushDMA
//      0x06-0x07:  Skip
//      0x08-0x0B:  Fence
//      0x0C-0x0E:  Skip for RB size 512bit
//      0x0F     :  Special Fence for possible switch to High Rb
//
//    Case 2 (Fence only, from Preempt or NullRendering):
//        [offset] [command]
//      0x00-0x07:  Skip
//      0x08-0x0B:  Fence
//      0x0C-0x0E:  Skip for RB size 512bit
//      0x0F     :  Special Fence for possible switch to High Rb
//**********************************************************************************************
typedef union RINGBUFFER_COMMANDS_E3K
{
    // Case 0
    struct
    {
        // 0x00-0x01: PWM Indicator Head and PWM Adj Trigger, and 1 followed dw for slice mask
        Cmd_Blk_Cmd_Csp_Indicator           pwmTrigger;
        Cmd_Blk_Cmd_Csp_Indicator_Dword1    trigger_Dw;
#if NEW_DUMP
        // 0x2-0x06: Skip_1  //According to HW's request, when slice_mask changed, driver should insert 8dw skip cmd after indicator cmd
        DWORD                               Skip_1[5];

        // 0x07-0x0A: InitDMA when slice switch, refer to SLICESWITCH_INIT_COMMANDS_E3K.c0
        DWORD                               SliceSwitchInitCommands[4];

        // 0x0B-0x0E: Restore DMA
        Cmd_Dma                             RestoreDMA;
        DWORD                               RestoreDMA_Address_L;
        DWORD                               RestoreDMA_Address_H;
        DWORD                               RestoreContext_Address;

        //0x0F-0x12:HangDumpDMA
        Cmd_Dma                             HangDumpContextSaveDMA;
        DWORD                               HangDumpContextSaveDMA_Address_L;
        DWORD                               HangDumpContextSaveDMA_Address_H;
        DWORD                               HangDumpContextSaveContext_Address;
#else
        // 0x2-0x0A: Skip_1
        DWORD                               Skip_1[9];

        // 0x0B-0x0E: InitDMA when slice switch, refer to SLICESWITCH_INIT_COMMANDS_E3K.c0
        DWORD                               SliceSwitchInitCommands[4];

        // 0x0F-0x12: Restore DMA
        Cmd_Dma                             RestoreDMA;
        DWORD                               RestoreDMA_Address_L;
        DWORD                               RestoreDMA_Address_H;
        DWORD                               RestoreContext_Address;

#endif
        // 0x13-0x15: CommandDMA
        Cmd_Dma                             CommandDMA;
        DWORD                               CommandDMA_Address_L;
        DWORD                               CommandDMA_Address_H;

        // 0x16-0x18: CacheFlushDMA
        Cmd_Dma                             CacheFlushDMA;
        DWORD                               CacheFlushDMA_Address_L;
        DWORD                               CacheFlushDMA_Address_H;

        // 0x19-0x1C: Save DMA
        Cmd_Dma                             SaveDMA;
        DWORD                               SaveDMA_Address_L;
        DWORD                               SaveDMA_Address_H;
        DWORD                               SaveContext_Address;

        // 0x1D-0x21: ExternalFence
        Cmd_Fence                           Fence;
        DWORD                               Fence_Address_L;
        DWORD                               Fence_Address_H;
        DWORD                               Fence_Data_L;
        DWORD                               Fence_Data_H;

        // 0x22-0x26: ExternalFence
        Cmd_Fence                           Fence_1;
        DWORD                               Fence_Address_L_1;
        DWORD                               Fence_Address_H_1;
        DWORD                               Fence_Data_L_1;
        DWORD                               Fence_Data_H_1;

        // 0x27-0x3B: ExternalFence
        Cmd_Fence                           Fence_2;
        DWORD                               Fence_Address_L_2;
        DWORD                               Fence_Address_H_2;
        DWORD                               Fence_Data_L_2;
        DWORD                               Fence_Data_H_2;

        // 0x3C-0x40: ExternalFence
        Cmd_Fence                           Fence_3;
        DWORD                               Fence_Address_L_3;
        DWORD                               Fence_Address_H_3;
        DWORD                               Fence_Data_L_3;
        DWORD                               Fence_Data_H_3;

        // 0x41-0x45: ExternalFence
        Cmd_Fence                           Fence_4;
        DWORD                               Fence_Address_L_4;
        DWORD                               Fence_Address_H_4;
        DWORD                               Fence_Data_L_4;
        DWORD                               Fence_Data_H_4;

        // 0x46-0x4A: ExternalFence
        Cmd_Fence                           Fence_5;
        DWORD                               Fence_Address_L_5;
        DWORD                               Fence_Address_H_5;
        DWORD                               Fence_Data_L_5;
        DWORD                               Fence_Data_H_5;

        // 0x27 - 0x31
        DWORD                               Skip_2[11];

        // 0x32-0x33:  PWM Indicator Tail, and followed dw
        Cmd_Blk_Cmd_Csp_Indicator           pwmTriggerOff;
        Cmd_Blk_Cmd_Csp_Indicator_Dword1    triggerOff_Dw;
    } c0;
    // Case 1
    struct
    {
        // 0x00-0x01: PWM Indicator Head and PWM Adj Trigger, and 1 followed dw for slice mask
        Cmd_Blk_Cmd_Csp_Indicator           pwmTrigger;
        Cmd_Blk_Cmd_Csp_Indicator_Dword1    trigger_Dw;

        // 0x02-0x06: Skip_1
        DWORD                               Skip_1[5];

        // 0x07-0x12: InitDMA when slice switch, refer to SLICESWITCH_INIT_COMMANDS_E3K.c1
        DWORD                               SliceSwitchInitCommands[12];

        // 0x13-0x15: CommandDMA
        Cmd_Dma                             CommandDMA;
        DWORD                               CommandDMA_Address_L;
        DWORD                               CommandDMA_Address_H;

        // 0x16-0x18: CacheFlushDMA
        Cmd_Dma                             CacheFlushDMA;
        DWORD                               CacheFlushDMA_Address_L;
        DWORD                               CacheFlushDMA_Address_H;

        // 0x19-0x1C: Skip_2
        Cmd_Skip                            Skip_2;
        DWORD                               Dummy_2[3];

        // 0x1D-0x21: ExternalFence
        Cmd_Fence                           Fence;
        DWORD                               Fence_Address_L;
        DWORD                               Fence_Address_H;
        DWORD                               Fence_Data_L;
        DWORD                               Fence_Data_H;

        // 0x22-0x26: ExternalFence
        Cmd_Fence                           Fence_1;
        DWORD                               Fence_Address_L_1;
        DWORD                               Fence_Address_H_1;
        DWORD                               Fence_Data_L_1;
        DWORD                               Fence_Data_H_1;

        // 0x27-0x3B: ExternalFence
        Cmd_Fence                           Fence_2;
        DWORD                               Fence_Address_L_2;
        DWORD                               Fence_Address_H_2;
        DWORD                               Fence_Data_L_2;
        DWORD                               Fence_Data_H_2;

        // 0x22-0x26: ExternalFence
        Cmd_Fence                           Fence_3;
        DWORD                               Fence_Address_L_3;
        DWORD                               Fence_Address_H_3;
        DWORD                               Fence_Data_L_3;
        DWORD                               Fence_Data_H_3;

        // 0x27-0x3B: ExternalFence
        Cmd_Fence                           Fence_4;
        DWORD                               Fence_Address_L_4;
        DWORD                               Fence_Address_H_4;
        DWORD                               Fence_Data_L_4;
        DWORD                               Fence_Data_H_4;

        // 0x22-0x26: ExternalFence
        Cmd_Fence                           Fence_5;
        DWORD                               Fence_Address_L_5;
        DWORD                               Fence_Address_H_5;
        DWORD                               Fence_Data_L_5;
        DWORD                               Fence_Data_H_5;

        // 0x27 - 0x31
        DWORD                               Skip_3[11];

        // 0x32-0x33:  PWM Indicator Tail, and followed dw
        Cmd_Blk_Cmd_Csp_Indicator           pwmTriggerOff;
        Cmd_Blk_Cmd_Csp_Indicator_Dword1    triggerOff_Dw;
    } c1;
    // Case 2
    struct
    {
        // 0x00-0x01: PWM Indicator Head and PWM Adj Trigger, and 1 followed dw for slice mask
        Cmd_Blk_Cmd_Csp_Indicator           pwmTrigger;
        Cmd_Blk_Cmd_Csp_Indicator_Dword1    trigger_Dw;

        // 0x02-0x1C: Skip_1
        DWORD                               Skip_1[0x1B];

        // 0x1D-0x21: ExternalFence
        Cmd_Fence                           Fence;
        DWORD                               Fence_Address_L;
        DWORD                               Fence_Address_H;
        DWORD                               Fence_Data_L;
        DWORD                               Fence_Data_H;

        // 0x22-0x26: ExternalFence
        Cmd_Fence                           Fence_1;
        DWORD                               Fence_Address_L_1;
        DWORD                               Fence_Address_H_1;
        DWORD                               Fence_Data_L_1;
        DWORD                               Fence_Data_H_1;

        // 0x27-0x3B: ExternalFence
        Cmd_Fence                           Fence_2;
        DWORD                               Fence_Address_L_2;
        DWORD                               Fence_Address_H_2;
        DWORD                               Fence_Data_L_2;
        DWORD                               Fence_Data_H_2;

        // 0x22-0x26: ExternalFence
        Cmd_Fence                           Fence_3;
        DWORD                               Fence_Address_L_3;
        DWORD                               Fence_Address_H_3;
        DWORD                               Fence_Data_L_3;
        DWORD                               Fence_Data_H_3;

        // 0x27-0x3B: ExternalFence
        Cmd_Fence                           Fence_4;
        DWORD                               Fence_Address_L_4;
        DWORD                               Fence_Address_H_4;
        DWORD                               Fence_Data_L_4;
        DWORD                               Fence_Data_H_4;

        // 0x22-0x26: ExternalFence
        Cmd_Fence                           Fence_5;
        DWORD                               Fence_Address_L_5;
        DWORD                               Fence_Address_H_5;
        DWORD                               Fence_Data_L_5;
        DWORD                               Fence_Data_H_5;

        // 0x27 - 0x31
        DWORD                               Skip_2[11];

        // 0x32-0x33:  PWM Indicator Tail, and followed dw
        Cmd_Blk_Cmd_Csp_Indicator           pwmTriggerOff;
        Cmd_Blk_Cmd_Csp_Indicator_Dword1    triggerOff_Dw;
    } c2;
}RINGBUFFER_COMMANDS_E3K;

typedef union SLICESWITCH_INIT_COMMANDS_E3K
{
    // Case 0
    struct
    {
        Cmd_Dma                             RestoreShadowDMA;
        DWORD                               RestoreShadowDMA_Address_L;
        DWORD                               RestoreShadowDMA_Address_H;
        DWORD                               RestoreShadowContext_Address;
    } c0;
    // Case 1
    struct
    {
        Cmd_Dma                             SaveDMA;
        DWORD                               SaveDMA_Address_L;
        DWORD                               SaveDMA_Address_H;
        DWORD                               SaveContext_Address;

        Cmd_Dma                             RestoreShadowDMA;
        DWORD                               RestoreShadowDMA_Address_L;
        DWORD                               RestoreShadowDMA_Address_H;
        DWORD                               RestoreShadowContext_Address;

        Cmd_Dma                             RestoreDMA;
        DWORD                               RestoreDMA_Address_L;
        DWORD                               RestoreDMA_Address_H;
        DWORD                               RestoreContext_Address;
    } c1;
} SLICESWITCH_INIT_COMMANDS_E3K;

#define HWM_SYNC_2DBLT_DST      0x2180000A
#define HWM_SYNC_EU_FE          0x31b10002
#define HWM_SYNC_EU_BE          0x31b20006
#define HWM_SYNC_D              0x31b30008
#define HWM_SYNC_Z              0x31b4000C
#define HWM_SYNC_STO                   0x01110007

#define HWM_SYNC_USAGETYPE_MASK 0x000000FF
#define HwmUsageType(Usage)     (Usage&HWM_SYNC_USAGETYPE_MASK)

#define BCI_MAX_COORD_E3K                  0x3fff

#define HWM_SYNC_USAGETYPE_MASK 0x000000FF
#define HWM_SYNC_MAX_TYPE       0x10  // must be consistent with the following definitions
#define HWM_SYNC_MAX_READTYPE   0x10  // must be consistent with the following definitions

#define HWM_SYNC_READ           0x00000000
#define HWM_SYNC_WRITE          0x01000000

#define HWM_SYNC_MODE_MASK      0xF0000000
#define HWM_SYNC_ACCESS_MASK    0x0F000000
#define HWM_SYNC_USAGEID_MASK   0x00FF0000

#define HWM_SYNC_INVALIDATE_L2_CACHE   0x011A000E
#define HWM_SYNC_FFC_D_FLUSH           0x01160008
#define HWM_SYNC_QUERY_DUMP     0x218e000D
#define HWM_SYNC_AUTOCLEAR      0x218c000D

#endif  /*__CHIP_INCLUDE_E3K_H__*/


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

#ifndef __VIDSCH_DEBUG_HANG_E3K_H__
#define __VIDSCH_DEBUG_HANG_E3K_H__

#include "vidsch_engine_e3k.h"

typedef enum DEBUG_BUS_SCOPE {
    WHOLE_SCOPE_DEBUG_BUS  = 0,
    CHIP1_SCOPE_DEBUG_BUS = 1,
    CHIP2_SCOPE_DEBUG_BUS = 2,
    CHIP3_SCOPE_DEBUG_BUS = 3,    
}DEBUG_BUS_SCOPE;

typedef struct debug_bus_info
{
    char             group_name[32];
    DEBUG_BUS_SCOPE  scope;
    unsigned int     group_start_offset;
    unsigned int     group_end_offset;
}debug_bus_info;

static const debug_bus_info  debug_bus_info_e3k[] =
{
    "ZXVD0 group"     ,       CHIP1_SCOPE_DEBUG_BUS,    0x2000,   0x201F,
    "CMU group"       ,       CHIP1_SCOPE_DEBUG_BUS,    0x2800,   0x29FF,
    "MIU2 group"      ,       CHIP1_SCOPE_DEBUG_BUS,    0x3000,   0x301F,
    "BIU0 group"      ,       CHIP1_SCOPE_DEBUG_BUS,    0x3800,   0x3809,
    "BIU1 group"      ,       CHIP1_SCOPE_DEBUG_BUS,    0x3C00,   0x3F7F,

    "ZXVD1 group"     ,       CHIP2_SCOPE_DEBUG_BUS,    0x2000,   0x201F,
    "MXUB group"      ,       CHIP2_SCOPE_DEBUG_BUS,    0x2800,   0x28FF,
    "MIU1 group"      ,       CHIP2_SCOPE_DEBUG_BUS,    0x3000,   0x301F,
    "L2 group"        ,       CHIP2_SCOPE_DEBUG_BUS,    0x3800,   0x387F,

    "VPP group"       ,       CHIP3_SCOPE_DEBUG_BUS,    0x2000,   0x207F,
    "MIU0 group"      ,       CHIP3_SCOPE_DEBUG_BUS,    0x2800,   0x281F,
    "DIU group"       ,       CHIP3_SCOPE_DEBUG_BUS,    0x3000,   0x3EE6,  
 
    "SLICE0 EUC group",       WHOLE_SCOPE_DEBUG_BUS,    0x000,    0x0FF,
    "SLICE0 PEA group",       WHOLE_SCOPE_DEBUG_BUS,    0x100,    0x17F,
    "SLICE0 PEB group",       WHOLE_SCOPE_DEBUG_BUS,    0x180,    0x1FF,
    "SLICE0 TU  group",       WHOLE_SCOPE_DEBUG_BUS,    0x200,    0x23F,
    "SLICE0 FFU group",       WHOLE_SCOPE_DEBUG_BUS,    0x280,    0x2FF,

    "SLICE1 EUC group",       WHOLE_SCOPE_DEBUG_BUS,    0x400,    0x4FF,
    "SLICE1 PEA group",       WHOLE_SCOPE_DEBUG_BUS,    0x500,    0x57F,
    "SLICE1 PEB group",       WHOLE_SCOPE_DEBUG_BUS,    0x580,    0x5FF,
    "SLICE1 TU  group",       WHOLE_SCOPE_DEBUG_BUS,    0x600,    0x63F,
    "SLICE1 FFU group",       WHOLE_SCOPE_DEBUG_BUS,    0x680,    0x6FF,

    "SLICE2 EUC group",       WHOLE_SCOPE_DEBUG_BUS,    0x800,    0x8FF,
    "SLICE2 PEA group",       WHOLE_SCOPE_DEBUG_BUS,    0x900,    0x97F,
    "SLICE2 PEB group",       WHOLE_SCOPE_DEBUG_BUS,    0x980,    0x9FF,
    "SLICE2 TU  group",       WHOLE_SCOPE_DEBUG_BUS,    0xA00,    0xA3F,
    "SLICE2 FFU group",       WHOLE_SCOPE_DEBUG_BUS,    0xA80,    0xAFF,

    "SLICE3 EUC group",       WHOLE_SCOPE_DEBUG_BUS,    0xC00,    0xCFF,
    "SLICE3 PEA group",       WHOLE_SCOPE_DEBUG_BUS,    0xD00,    0xD7F,
    "SLICE3 PEB group",       WHOLE_SCOPE_DEBUG_BUS,    0xD80,    0xDFF,
    "SLICE3 TU  group",       WHOLE_SCOPE_DEBUG_BUS,    0xE00,    0xE3F,
    "SLICE3 FFU group",       WHOLE_SCOPE_DEBUG_BUS,    0xE80,    0xEFF,

    "CENTRAL SPTFE group",    WHOLE_SCOPE_DEBUG_BUS,    0x1000,    0x103F,
    "CENTRAL SGTBE group",    WHOLE_SCOPE_DEBUG_BUS,    0x1800,    0x183F,

};

#define SAVE_BEFORE_PREHANG                 1 //for prehangdump:save resource,DMA,CONTEXT
#define SAVE_AFTER_PREHANG                  2 //for prehangdump:save ringbuffer and modify DMA.context restore,fence commands
#define SAVE_BEFORE_POSTHANG                3 //for posthangdump:save CONTEXT
#define SAVE_AFTER_POSTHANG                 4 //for posthangdump:save resource,DMA,ringbuffer and modify DMA.context restore,fence commands
#define SAVE_RING_BUFFER                    5 //save ringbuffer only.
#define SAVE_FORMAT_BUFFER                  6 //save formatbuffer only.


// Debug hang fence offset
#define DH_FENCE_OFFSET         (0x40)
#define DH_FENCE_VALUE          (0x12345678)
#define DH_XBUFFER_SIZE         (0x10000)
#define DUPLICATE_TIME_OUT      (10) //10s

//multi dma number
#define     MAX_HANG_DUMP_DATA_POOL_NUMBER        6

#define     HANG_DUMP_CONTEXT_BUFFER_OFFSET        1
#define     HANG_DUMP_DMA_BUFFER_OFFSET            2
#define     HANG_DUMP_RING_BUFFER_OFFSET           3
#define     HANG_DUMP_DH_COMMON_INFO_OFFSET        4
#define     HANG_DUMP_FORMAT_BUFFER_OFFSET         5

typedef struct dh_file_info_e3k
{
    large_inter FbNotCpuVisibleSize;
    unsigned int  FbCpuVisibleSize;
    unsigned int  dma_for_hang_size;
    unsigned int  context_for_hang_size;
    unsigned int  ring_for_hang_size;
}dh_file_info_e3k;

typedef struct dh_common_info_e3k
{
    unsigned int    current_hang_dump_index;
    unsigned int    hang_dump_counter;
    unsigned int    hang_dma_index;
    unsigned long long hang_dma_fence_id;
    unsigned int    hang_rb_index;
    unsigned long long fence_id[MAX_HANG_DUMP_DATA_POOL_NUMBER];
    unsigned int    rb_index[MAX_HANG_DUMP_DATA_POOL_NUMBER];
}dh_common_info_e3k;


typedef struct dh_rb_info_e3k
{
    EngineSatus_e3k status;
    unsigned int         last_rb_size;
    unsigned int         last_rb_index;
}dh_rb_info_e3k;

extern int use_hw_dump;

extern void vidsch_duplicate_hang_e3k(adapter_t *adapter);
extern void vidsch_dump_hang_e3k(adapter_t *adapter);
extern int vidsch_save_misc_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr,  unsigned int what_to_save);
extern void vidsch_handle_dbg_hang_dump_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr, task_dma_t *task_dma);

typedef union
{
    struct
    {
        unsigned int wait                  : 1; // wait after submit.
        unsigned int print_hang            : 1; // print out logged hang info.
        unsigned int reset_hw              : 1; // reset hardware.
        unsigned int pre_hang_dump         : 1; // before hang,Save DMA,RingBuffer,context and all resources into framebuffer mirror
        unsigned int post_hang_dump        : 1; // After hang,Save DMA,RingBuffer,context and all resources into framebuffer mirror
        unsigned int duplicate_hang        : 1; // resore the enviroment saved by PreHangDump/PostHangDump to duplcate the hang
        unsigned int internal_dump_hw      : 1; // Dump framebuffer mirror to file so that duplicat the hang .
        unsigned int internal_block_submit : 1; // block sumbit during replaying
        unsigned int print_debug_bus       : 1; // print out debug bus info.
        unsigned int start_test_dump       : 1; // start to test hang dump code
        unsigned int multi_dma_number      : 5; // 0-single dma
        unsigned int Reserved              : 17;
    };
    unsigned int uint;
} reg_debug_mode_e3k;

extern reg_debug_mode_e3k debug_mode_e3k;
extern void vidsch_display_debugbus_info_e3k(adapter_t *adapter, struct os_printer *p, int video);
#endif

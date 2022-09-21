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

#ifndef __VIDSCH_ENGINE_E3K_H
#define __VIDSCH_ENGINE_E3K_H

#include "vidsch.h"
#include "vidschi.h"
#include "ring_buffer.h"
#include "chip_include_e3k.h"
#include "vidsch_blt_e3k.h"

#define EXTERNAL_FENCE_DWSIZE      (5)
#define ALIGNED_8(Value)           (((Value) + 7) & ~7)
#define ALIGNMENT_4K                   (0x1000 - 1)
#define BEGIN_END_BUF_SIZE_E3K     \
        ((sizeof(RB_PREDEFINE_DMA) + ALIGNMENT_4K) & (~ ALIGNMENT_4K))

#define RING_BUFFER_SIZE_E3K       (64 * 4096) //need align to 4K?
#define KKK_ARGUMENT_BUFFER_SIZE   (0x10000)
#define FENCE_BUFFER_SIZE_E3K      (0x00001000)

#define VIDEO_BRIDGE_BUF_SIZE      (0x800000)   //reserve 8M bridge buffer used as video FE output for one video core


#define HW_CONTEXT_COUNT_E3K       (2)
#define HW_CONTEXT_E3K_ALIGN       (0x4000 -1)//Align to 16K for HW requirement
#define HW_CONTEXT_SIZE_E3K    \
             ( ( sizeof(CONTEXT_BUFFER_E3K) + HW_CONTEXT_E3K_ALIGN ) &( ~HW_CONTEXT_E3K_ALIGN ) )

#define E3K_ENGINE_HW_QUEUE_SIZE   5

#define _3DBLT_DATA_RESERVE_SIZE    (1024 * 16)

typedef enum
{
    RB_INDEX_GFXL = 0,       //Graphics High Ring Buffer Index;
    RB_INDEX_GFXH = 1,              //Graphics Low Ring Buffer Index;
    RB_INDEX_CSH  = 2,                //CS High Ring Buffer Index
    RB_INDEX_CSL0 = 3,               //CS  No1 RingBuffer Index
    RB_INDEX_CSL1 = 4,               //CS  No2 RingBuffer Index
    RB_INDEX_CSL2 = 5,               //CS  No3 RingBuffer Index
    RB_INDEX_CSL3 = 6,               //CS  No4 RingBuffer Index
    RB_INDEX_VCP0 = 7,
    RB_INDEX_VCP1 = 8,
    RB_INDEX_VPP  = 9,
    RB_NUM        = 10,
} RB_NODE_E3K;

typedef enum
{
    SUBMIT_CMD_HWCONTEX_SWITCH = 0,
    SUBMIT_CMD_NO_HWCONTEXT_SWITCH = 1,
    SUBMIT_CMD_FENCE_ONLY = 2,
    SUBMIT_CMD_LAST = 3,
}SUBMIT_CMD_TYPE;

 _inline int EngineRbOffset(DWORD RbIndex)
{
    unsigned int RegRbOffset = 0;
    
    if ( RbIndex == RB_INDEX_VCP0 || RbIndex == RB_INDEX_VCP1 ||RbIndex == RB_INDEX_VPP)
    {
        RegRbOffset = Reg_Vcp_Ring_Buf_Offset + (RbIndex -RB_INDEX_VCP0) * 4;
    }
    else
    {
         RegRbOffset = Reg_Ring_Buf_Offset + RbIndex * 4;
    }
    return RegRbOffset;
}

typedef struct
{
    vidsch_mgr_t        *vidsch;

    unsigned long long  *fence_buffer_virt_addr; /*fence buffer addr */
    unsigned long long  fence_buffer_phy_addr;  /* fence buffer addr */

    unsigned long long  *fence_buffer_virt_addr_fake; /*fence buffer addr, patch for fence isr lost issue*/
    unsigned long long  fence_buffer_phy_addr_fake;  /* fence buffer addr, patch for fence isr lost issue*/

    unsigned int        *ring_buf_virt_addr;
    unsigned long long  ring_buf_phys_addr;

    ring_buffer_t       ring;

#if 1//GF_TRACE_HW_HANG
    unsigned int       *last_ring_buffer;
    unsigned int        last_ring_buffer_size;
#endif

    void               *share;

    EngineSatus_e3k     dumped_engine_status;           // save status to dump to file //common

    unsigned long long  dma_for_hang;                   // Start Offset of DMA reserved for Hang //common
    unsigned long long  context_for_hang;               // Start Offset of Context reserved for Hang //common 
    unsigned char       *last_ring_buffer_for_begin_dma;     // point to RB where context resotre CMD is saved
    unsigned char       *last_ring_buffer_for_end_dma;       // point to RB where context save CMD is saved
    unsigned long long  context_buffer_to_restore_addr; // the address of context to restore
    unsigned long long  ring_buffer_for_hang;           // Start Offset of RingBuffer reserved for Hang

    unsigned int        last_ring_buffer_engine_index;
    unsigned char       *last_ring_buffer_for_dma;      // point to RingBuffer where DMA kicked off CMD is saved
    unsigned char       *last_ring_buffer_for_fence;    // point to RingBuffer where Fence CMD is saved
    unsigned char       *last_dma_buffer;               // last dma buffer location
    unsigned int        last_dma_buffer_size_uint;
 
}engine_e3k_t;

typedef struct 
{
    engine_e3k_t         common;
    unsigned int         is_high_engine;

    unsigned long long       context_buf_phy_addr; //for low, it's template buffer for high/low, for High it's hwctx for low save/restore 

}engine_gfx_e3k_t;

typedef struct
{
    engine_e3k_t        common;

    unsigned int        AddrTblUpdateCECMD[10];

    unsigned int        addr_table_phys_addr;

    unsigned int        FreeIdxSize;
    unsigned int       *FreeIdxList;
}engine_paging_e3k_t;


typedef struct engine_share_e3k
{
    unsigned int        ref_cnt;

    gf_vm_area_t        *begin_end_vma; //save restore dma vma struct, high rb needs memcopy to rb cmd, so keep this vm struct.
    unsigned long long  begin_end_buffer_phy_addr;
    unsigned long long  context_buf_phy_addr;//high and low use same hwctx template
    
    struct os_mutex     *_3dblt_data_lock;
    void                *_3dblt_data_shadow;    // it's an system memory cache, used for restore
    unsigned int        _3dblt_data_size;
    unsigned long long  _3dblt_data_phy_addr;

    //CSLx patch need ringbuffer phy addr
    unsigned long long       ring_buffer_phy_addr[RB_NUM];

    BITBLT_REGSETTING_E3K _2dblt_cmd_e3k;
    FASTCLEAR_REGSETTING_E3K fast_clear_cmd_e3k;

    RINGBUFFER_COMMANDS_E3K   RingBufferCommands[SUBMIT_CMD_LAST];
    unsigned int  internal_fence_value[8]; //patch video engine issue.

    struct _vidmm_segment_memory *ring_buffer_for_hang;
    struct _vidmm_segment_memory *dma_buffer_for_hang;
    struct _vidmm_segment_memory *context_buffer_for_hang;
    struct _vidmm_segment_memory *mirror_buffer_for_hang;
    struct _vidmm_segment_memory *transfer_buffer_for_hang;
    struct _vidmm_segment_memory *flush_fifo_buffer;
    struct _vidmm_segment_memory *bl_buffer;
    void * bl_buffer_backup;		
    gf_vm_area_t                *fb_vma;

    struct os_spinlock *lock; //use for pretect internal fence value;

}engine_share_e3k_t;

typedef engine_e3k_t engine_cs_e3k_t;
typedef engine_e3k_t engine_vcp_e3k_t;
typedef engine_e3k_t engine_vpp_e3k_t;


typedef struct _GF_RINGBUFFER_HEADER_E3K
{
    UINT TailOffset;         // rb tail pointer, byte offset to tail.
    UINT HeadOffset;         // rb head pointer, byte offset to head.
    UINT Size;
    UINT ContextSaveBase;    // the context save area base address.
    UINT Reserved[12];       // The whole header is 512-bits, 16DW.  Make sure to update
} GF_RINGBUFFER_HEADER_E3K;

extern int engine_gfx_low_init_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr);
extern int engine_gfx_high_init_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr);
extern int engine_gfx_high_init_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr);
extern int engine_cs_init_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr);
extern int engine_vcp_init_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr);
extern int engine_vpp_init_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr);
extern void engine_gfx_low_restore_e3k(vidsch_mgr_t *sch_mgr, unsigned int pm);
extern void engine_gfx_high_restore_e3k(vidsch_mgr_t *sch_mgr, unsigned int pm);
extern void enginei_init_ring_buffer_commands_e3k(engine_gfx_e3k_t *engine);

extern void  enginei_enable_ring_buffer_e3k(engine_e3k_t *engine);
extern unsigned int *enginei_get_ring_buffer_space_e3k(engine_e3k_t *engine, int size);
extern void  enginei_do_kickoff_cmd_e3k(engine_e3k_t *engine);
extern int enginei_init_hardware_context_e3k(engine_gfx_e3k_t *engine);

extern unsigned long long engine_update_fence_id_e3k(vidsch_mgr_t *sch_mgr);
extern int engine_submit_task_dma_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr, task_dma_t *task_dma);
extern int engine_destroy_e3k(vidsch_mgr_t *vidsch);
extern int engine_cil2_misc_e3k(vidsch_mgr_t *sch_mgr, krnl_cil2_misc_t *misc);

extern int engine_save_e3k(vidsch_mgr_t *vidsch);
extern void engine_restore_e3k(vidsch_mgr_t *vidsch, unsigned int pm);

extern void vidschi_reset_adapter_e3k(adapter_t *adapter);
extern int  vidsch_reset_hw_e3k(vidsch_mgr_t * sch_mgr);
extern void vidschi_init_hw_settings_e3k(adapter_t *adapter);
extern int  vidschi_get_slot_count_e3k(adapter_t *adapter);
extern int  vidsch_patch_e3k(adapter_t *adapter, task_dma_t *task_dma);
extern int  vidsch_render_e3k(gpu_context_t *context, task_dma_t *task_dma);

extern void vidsch_dump_hang_info_e3k(vidsch_mgr_t * sch_mgr);
extern void vidsch_dump_hang_e3k(adapter_t *adapter);
extern void vidsch_display_debugbus_info_e3k(adapter_t *adapter, struct os_printer *p, int video);

#ifdef GF_HW_NULL
void engine_set_fence_id_e3k(vidsch_mgr_t *sch_mgr, unsigned long long fence_id);
#endif

#endif //#ifndef __VIDSCH_ENGINE_E3K_H















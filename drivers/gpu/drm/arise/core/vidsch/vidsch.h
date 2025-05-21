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
#ifndef __VIDSCH_H__
#define __VIDSCH_H__

#include "vidmm.h"
#include "context.h"
#include "list.h"
#include "kernel_interface.h"

/* scheduler allocation, record allocation infomation for one dma */
typedef struct _vidsch_allocation
{
    vidmm_allocation_t  *mm_allocation;        /* vidmm allocation pointer */
    unsigned long long  phy_addr;
    struct
    {
        unsigned int    write_operation :   1;      /* if current dma write to allocation */
        unsigned int    segment_id      :   5;
        unsigned int    reserved        :   26;
    };
}vidsch_allocation_t;

typedef struct _vidsch_sync_object_reference
{
    struct vidsch_sync_object  *sync_object;        /* vidmm allocation pointer */
    unsigned int               patch_offset;
    unsigned long long         fence_value;
}vidsch_sync_object_reference_t;

/* arguments of vidsch manager interface render */
typedef struct _vidsch_render
{
    unsigned int    command_length;                 /* in: command length, in bytes */

    unsigned int    allocation_list_size;           /* in: allocation list size */
    unsigned int    patch_location_list_size;       /* in: patch location list size */
    unsigned int    sync_object_list_size;

    gf_render_flags_t flags;                           /* in: render flags */

    unsigned int    queued_buffer_count;            /* out: task queue count when render, for debug */

    unsigned long long     render_counter;          /* in: for trace events */

    gf_allocation_list_t    *allocation_list;
    gf_syncobj_list_t       *sync_object_list;
    gf_patchlocation_list_t *patch_location_list;

    unsigned int            cmdbuf_count;           /* in                   */
    gf_cmdbuf_t            *cmdbuf_array;          /* in                   */
}vidsch_render_t;

typedef struct dma_node
{
    list_node_t        *list_node;
    unsigned char      *virt_base_addr;
    unsigned long long gpu_phy_base_addr;
    unsigned int       size;
    unsigned int       cmd_size;
}dma_node_t;

typedef enum
{
    DRAW_2D_DMA = 1,
    DRAW_3D_DMA = 2,
    DRAW_3D_BLT = 3,
    PAGING_DMA  = 4,
}DMA_TYPE;

/* task type */
typedef enum _task_type
{
    task_type_render       = 1,                     /* task dma */
    task_type_sync_wait    = 2,                     /* task sync */
    task_type_sync_signal  = 3,
    task_type_paging       = 4,
    task_type_reserve      = 5,
}task_type_t;

/* self task desciption struct*/
typedef struct _task_desc
{
    struct list_head        list_item;
    struct list_head        schedule_node;
    task_type_t             type;
    union
    {
        struct task_dma     *task_render;
        struct task_paging  *task_paging;
        struct task_signal  *task_signal;
        struct task_wait    *task_wait;
    };
    gpu_context_t      *context;         /* gpu_context */
    void               *dma_fence;
    unsigned int       submitted      : 1;
    unsigned int       fake_submitted : 1;
    unsigned int       reset_dropped  : 1;
    unsigned int       need_dma_fence : 1;
    unsigned int       large_dma      : 1;
    int                ref_cnt;
    int                cmd_size;
    unsigned long long global_task_id;
    unsigned long long context_task_id;
    unsigned long long timestamp;
    unsigned long long fence_id;
    gf_render_flags_t  Flags;
    int                hang_index;
    unsigned long long hang_context;
}task_desc_t;

/* dma information, used in render, patch, submit */
typedef struct task_dma
{
    task_desc_t         desc;
    DMA_TYPE            dma_type;
    hw_ctxbuf_t         *hw_ctx_info;
    unsigned int        engine_index;                       /* the engine the dma will be submitted to */
    dma_node_t          *dma_buffer_node;
    unsigned int        command_length;
    unsigned int        submit_start_offset;                /* submit start offset */
    unsigned int        submit_end_offset;                  /* submit end offset */

    gf_allocation_list_t        *allocation_list_raw;
    vidsch_allocation_t         *sch_allocation_list;       /* vidsch allocation list */
    unsigned int                allocation_list_size;       /* mm, sch allocation list size */

    gf_patchlocation_list_t     *patch_location_list;       /* patch location list */
    unsigned int                patch_location_list_size;   /* patch location list size */
    unsigned int                patch_start_offset;         /* patch start offset */
    unsigned int                patch_end_offset;           /* patch end offset */

    gf_syncobj_list_t           *sync_object_list_raw;
    vidsch_sync_object_reference_t *sync_object_list;
    unsigned int                sync_object_list_size;

    struct
    {
        unsigned int    need_hwctx_switch    :   1;
        unsigned int    need_paging          :   1;              /* need paging */
        unsigned int    need_split           :   1;              /* need split */
        unsigned int    paging               :   1;              /* paging dma buffer */
        unsigned int    null_rendering       :   1;              /* update fence only */
        unsigned int    dump_miu             :   1;              /* indicate miu counter dump*/
        unsigned int    submit_failed        :   1;              /* submit fail */
        unsigned int    prepare_submit       :   1;
        unsigned int    prepare_succeeded    :   1;
        unsigned int    reserved             :  23;
    };

    unsigned long long render_counter;                       /* for trace events */
    unsigned long long timestamp_ms;
}task_dma_t;

typedef struct task_paging
{
    //struct list_head           list_item;
    task_desc_t                desc;
    dma_node_t                 *dma;
    unsigned int               command_length;

    struct
    {
        unsigned int           paging_type    :  2; /* 0 means fill, 1 means paging in, 2 means paging out*/
        unsigned int           reserved       : 30;
    };

    int                        must_success;
    int                        save_restore;
    int                        paging_allocation_num;
    int                        paging_allocation_list_size;
    vidmm_paging_allocation_t *paging_allocation_list;
}task_paging_t;

typedef struct
{
    DMA_TYPE      dma_type;
    gpu_context_t *context;
    unsigned int  command_length;
    unsigned int  allocation_count;
    unsigned int  patch_location_count;
    unsigned int  sync_object_count;
    gf_render_flags_t Flags;
}vidsch_allocate_task_dma_t;

/* export interface for other modules */

extern int vidsch_create(adapter_t *adapter);
extern int vidsch_destroy(adapter_t *adapter);

extern int vidsch_render(gpu_context_t *gpu_context, vidsch_render_t *vidsch_render);
extern int vidsch_create_sync_object(gpu_device_t *gpu_device, gf_create_fence_sync_object_t *create, int binding);
extern int vidsch_destroy_sync_object(adapter_t *adapter, gpu_device_t *gpu_device, unsigned int hSync);
extern void vidsch_sync_obj_inc_reference(adapter_t *adapter, unsigned int hSync);
extern void vidsch_sync_obj_dec_reference(adapter_t *adapter, unsigned int hSync);
extern void vidsch_force_signal_fence_sync_object(adapter_t *adapter, unsigned int hSync, unsigned long long fence_value);
extern int vidsch_signal_sync_object(gpu_context_t *gpu_context, unsigned int hSync);
extern int vidsch_wait_sync_object(gpu_context_t *gpu_context, gf_wait_fence_sync_object_t *wait);
extern int vidsch_destroy_remained_sync_obj(gpu_device_t *device);
extern int vidsch_fence_value(gpu_device_t *device, gf_fence_value_t *value);
extern int vidsch_is_fence_signaled(adapter_t *adapter, unsigned int hSync, unsigned long long wait_value);

extern void vidsch_update_engine_idle_status(adapter_t *adapter, unsigned int engine_mask);


extern int  vidsch_is_fence_back(adapter_t *adapter, unsigned char engine_index, unsigned long long fence_id);
extern void vidsch_wait_fence_back(adapter_t *adapter, unsigned char engine_index, unsigned long long fence_id);
#define  vidsch_wait_fence_interrupt vidsch_wait_fence_back
extern unsigned long long vidsch_get_current_returned_fence(adapter_t *adapter, int engine);

extern void vidsch_wait_chip_idle(adapter_t *adapter, unsigned int engine_mask);
extern void vidsch_wait_engine_idle(adapter_t *adapter, int idx);

extern int vidsch_save(adapter_t *adapter);
extern void vidsch_restore(adapter_t *adapter);
extern void vidsch_dvfs_power_flag_reset(adapter_t *adapter);
extern void vidsch_set_power_state(adapter_t *adapter, unsigned int state, unsigned int holding_ms, unsigned int force);
extern task_dma_t *vidsch_allocate_task_dma(adapter_t *adapter, unsigned int engine_index, vidsch_allocate_task_dma_t *dma_arg);

extern task_paging_t *vidsch_allocate_paging_task(adapter_t *adapter, int dma_size, int allocation_num);
extern void vidsch_submit_paging_task(adapter_t *adapter, task_paging_t *paging_task);

extern void vidsch_release_completed_tasks(adapter_t *adapter, int engine_index, unsigned long long *uncompleted_dma);

extern void vidsch_task_inc_reference(adapter_t *adapter, int engine_index, task_desc_t *desc);
extern void vidsch_task_dec_reference(adapter_t *adapter, int engine_index, task_desc_t *desc);

extern int  vidsch_request_fence_space(adapter_t *adapter, void *fence_buf, unsigned long long **virt_addr, unsigned long long *gpu_addr);
extern int  vidsch_release_fence_space(adapter_t *adapter, void *fence_buf, unsigned long long gpu_addr);

extern int  vidsch_notify_interrupt(adapter_t *adapter, unsigned int interrupt_events);

extern int  vidsch_cil2_misc(gpu_device_t *device, krnl_cil2_misc_t *misc);

extern void vidsch_dump(struct os_printer *p, adapter_t *adapter);
extern int vidsch_dump_info(struct os_seq_file *seq_file, adapter_t *adapter);
extern int vidsch_dump_debugbus(struct os_printer *p, adapter_t *adapter);

extern int  vidsch_allocation_in_hw_queue(adapter_t *adapter, unsigned int engine_mask, vidmm_allocation_t *allocation);
extern int  vidsch_is_allocation_idle(adapter_t *adapter, unsigned int engine_mask, vidmm_allocation_t *allocation);
extern void vidsch_wait_allocation_idle(adapter_t *adapter, unsigned int engine_mask, int read_only, vidmm_allocation_t *allocation);


extern void vidsch_get_set_reg(adapter_t *adapter, gf_query_info_t *info);
extern void vidsch_set_miu_reg(adapter_t *adapter,gf_query_info_t *info);
extern void vidsch_read_miu_reg(adapter_t *adapter,gf_query_info_t *info);
extern int vidsch_query_info(adapter_t *adapter, gf_query_info_t *info);
extern void vidsch_force_wakup(adapter_t *adapter);

#endif

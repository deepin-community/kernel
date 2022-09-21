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

#ifndef __CONTEXT_H__
#define __CONTEXT_H__
#include "core_import.h"
#include "util.h"
#include "list.h"

#define USER_MODE_DMA_SIZE      64 * 1024                       /* Set to 64KB to pass MaxContexts test */
#define SYNCOBJECTLIST_SIZE     256
#define ALLOCATIONLIST_SIZE     (USER_MODE_DMA_SIZE / 64) * 4   /* Set to 2KB for every 64KB of command buffer to pass MaxContexts test */
#define PATCHLOCATIONLIST_SIZE  (USER_MODE_DMA_SIZE / 64) * 16   /* Set to 2KB for every 64KB of command buffer to pass MaxContexts test */
#define MAX_SYNC_OBJECT_SIZE_PER_DEVICE  512*4
#define CONTEXT_TASK_QUEUE_LENGTH       8
typedef struct
{
    unsigned int                node_ordinal;
    unsigned int                engine_affinity;
    unsigned int                flags;
}create_context_t;

typedef struct hw_ctxbuf
{
    struct list_head   list_node;

    int index;
    int phys_index;//global context index, adapter function region
    int is_initialized;
    int is_invalid;

    unsigned long long  context_buffer_address;
    unsigned int  *context_buffer;
}hw_ctxbuf_t;

typedef struct gpu_device gpu_device_t;

#define GF_MAX_PNAME_LEN 64
typedef struct gpu_context
{
    struct list_head   list_node;
    char               pname[GF_MAX_PNAME_LEN];

    unsigned int       handle;
    int                engine_index;
    gpu_device_t       *device;
    unsigned long      pid;
    unsigned long      tid;

    unsigned long long task_id;
    unsigned long long last_submit_to_sw;
    unsigned long long render_counter;

    unsigned long long wait_delta_check_start; //use for server wait timeout
    unsigned long long wait_delta_check_end;

    unsigned int       high_priority : 1;
    int                mmio_flip;
    int                is_kernel;
    unsigned int       context_ctrl;
    struct os_sema     *context_sema;

    struct os_atomic   *ref_cnt; //dma_ref_cnt

    /* for OGL context, one context can use multi-hwctx.
     * OGL will explictly call add_hwctx to support multi-hwctx,
     * for D3D context == hwctx, no need to add_hwctx, 
     * future will refine vpm code, let video go ogl way to intergate code
     * and delete this per context hw_ctx. 
     * now this only for video client use, OGL use hwctx in device
     */
    hw_ctxbuf_t     hw_ctx;
}gpu_context_t;

typedef struct __di_context
{
    struct list_head  list_node;

    gpu_device_t      *device;       

    unsigned int      handle;

    int               hw_idx;
    int               stream_id; /* video stream id for debug */
}di_context_t;



struct gpu_device
{
    struct list_head         list_node;

    adapter_t                *adapter;
    void                     *filp;

    unsigned int             handle;
    unsigned long            pid;
    unsigned long            tid;

    char                     pname[GF_MAX_PNAME_LEN];

    struct os_mutex          *lock;

    /* resource used in this device */
    struct list_head         context_list;
    struct list_head         hwctx_list;
    struct list_head         sync_obj_list;
    struct list_head         allocation_open_instance_list;
    struct list_head         display_stream_list;
    struct list_head         di_context_list; /* deinterlace hwctx list */

    int                      create_num; // device create resource (device, context, hwctx, sync_obj, allocation) num
                                         // since all resource use device ptr and device->lock so device struct and
                                         // lock only can be free when this num = 0, note we also treat device itself as a num,
                                         // when destroy_device called num--, then other resource destroy only can free struct after
                                         // destroy deivce called when num-- lead num == 0. currently only for allocation, device

    int                      task_dropped; //if any context in this device dropped task.
                                           //if so, need force set wait fence sync obj signal to avoid dead lock
                                           //since fence obj signal cmd always put in gerneal dma task.
    
    unsigned long long        present_count_gpu_addr;
    volatile unsigned long long    *current_present_count;

    unsigned long long             last_present_count;
    struct os_atomic         *sync_obj_count; //for limit sync object size per device
    unsigned char             video_seq_index[16];
    unsigned char             video_core_index_cnt[2];
};


typedef struct
{
    struct list_head         list_node;        //allocation mode link in device->allocation_open_instance_list
    struct list_head         device_ref_node;  //ref device node linked in allocation->device_ref_list; 
    gpu_device_t             *device;          //reference device,  which device reference this allocation, or which device call this open.
    struct _vidmm_allocation *allocation;
    unsigned int             ref_cnt;          //record how many times this device open this allocation.
}cm_allocation_open_instance_t;

static inline void cm_device_lock(gpu_device_t *device)
{
    gf_mutex_lock(device->lock);
}

static inline void cm_device_unlock(gpu_device_t *device)
{
    gf_mutex_unlock(device->lock);
}

extern gpu_device_t* cm_create_device(adapter_t *adapter, void *process_context);
extern void cm_destroy_device(adapter_t *adapter, gpu_device_t *device);
extern void cm_destroy_remained_device(adapter_t *adapter);
extern gpu_context_t* cm_create_context(gpu_device_t *device, create_context_t *create_context, int is_kernel);
extern void cm_destroy_context(gpu_device_t *device, gpu_context_t *context);
extern void cm_wait_context_idle(gpu_context_t *context);
extern int cm_add_hwctx_buffer(gpu_device_t *device, gpu_context_t *context, unsigned int hw_buf_index);
extern int cm_remove_hwctx_buffer(gpu_device_t *device, gpu_context_t *context, unsigned int hw_buf_index);
extern hw_ctxbuf_t *cm_get_hwctx_buffer(gpu_device_t *device, gpu_context_t *context, unsigned int index);

extern void cm_reset_command_header(gpu_context_t *context);

extern int  cm_create_di_mgr(adapter_t *adapter);
extern void cm_destroy_di_mgr(adapter_t *adapter);

extern di_context_t *cm_create_di_context(gpu_device_t *device, gf_create_di_context_t *create);
extern void          cm_destroy_di_context(gpu_device_t *device, di_context_t *context);

extern int cm_save(adapter_t *adapter, int need_save_memory);
extern void cm_restore(adapter_t *adapter);

extern int  cm_device_add_reference(gpu_device_t *device, struct _vidmm_allocation *allocation);
extern int  cm_device_del_reference(gpu_device_t *device, struct _vidmm_allocation *allocation);

extern void cm_device_lock_allocation(gpu_device_t *device, struct _vidmm_allocation *allocation);
extern void cm_device_unlock_allocation(gpu_device_t *device, struct _vidmm_allocation *allocation);

extern void cm_device_create_num_inc(gpu_device_t *device);
extern void cm_device_create_num_dec(gpu_device_t *device);

extern void cm_dump_resource(adapter_t *adapter);
extern void cm_dump_device_alloctions(struct os_seq_file *seq_file, adapter_t *adapter,  gpu_device_t *device);
#endif


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
#ifndef __KERNEL_INTERFACE_H__
#define __KERNEL_INTERFACE_H__

#include "gf_def.h"
#include "gf_types.h"
#include "core_errno.h"
#include "kernel_import.h"

typedef struct
{
    struct gpu_device *device;
    struct gpu_context *context;
    gf_cil2_misc_t *gf_misc;
} krnl_cil2_misc_t;

struct krnl_adapter_init_info_s
{
    int minor_index;
/* init control  parmetes */
    int gf_pwm_mode ;/* control pwm of gf */
    int gf_dfs_mode ;/* control dvfs of gf */
    int gf_worker_thread_enable;/* control worker thread on/off */
    int gf_recovery_enable ; /* enable recovery when hw hang */
    int gf_hang_dump;/*0-disable, 1-pre hang, 2-post hang, 3-duplicate hang */
    int gf_run_on_qt; /* control wether run on QT */
    int gf_flag_buffer_verify ;/*0 - disable, 1 - enable */
    int gf_vesa_tempbuffer_enable ; /* control wether reserve memory during boot */

    int miu_channel_size;//0/1/2 for 256B/512B/1kb Swizzle
    int gf_backdoor_enable;
    //gpc/slice setting
    unsigned int chip_slice_mask;//// CHIP_SLICE_MASK own 12 bits(should be set 0x001 ~ 0xfff), driver use this to set the Slice_Mask which HW used.

    int gf_local_size_g;
    int gf_local_size_m;
    int gf_pcie_size_g;
    int gf_pcie_size_m;
    int debugfs_mask;
};
typedef struct krnl_adapter_init_info_s krnl_adapter_init_info_t;

typedef  struct
{
    struct  //get from adapter
    {
        unsigned int  ven_dev;
        unsigned int  family_id;
        unsigned int  generic_id;
        unsigned int  chip_id;
        unsigned char*   mmio;
        unsigned int  mmio_size;
        unsigned int  primary;
        unsigned long long  fb_bus_addr;
        unsigned int  fb_total_size;
        unsigned int  patch_fence_intr_lost:1;
        unsigned int  init_render:1;
        union
        {
            unsigned int  adp_flags;
            struct
            {
                unsigned int  run_on_qt:1;
                unsigned int  Reserved:31;
            };
        };
    };
    struct   //set to adapter
    {
        unsigned int low_top_addr;
        unsigned int snoop_only;
        unsigned int chan_num;
        unsigned int avai_mem_size_mb; //in Mega Bytes
        unsigned int total_mem_size_mb; //in Mega Bytes
        unsigned int chip_slice_mask;
        unsigned int hdaudio_to_local;
        unsigned int non_simul_chip;
    };
}adapter_info_t;

/*
** misc things
*/
#define OS_CALLBACK_POST_EVENT         1
#define OS_CALLBACK_POST_SYNCOBJ_EVENT 2
#define OS_CALLBACK_DRM_CB             3

typedef struct
{
    void* (*pre_init_adapter)(void *pdev, krnl_adapter_init_info_t *info, krnl_import_func_list_t *import);
    void  (*init_adapter)(void *adp, int reserved_vmem, void *disp_info);
    void (*deinit_adapter)(void *data);
    void  (*get_adapter_info)(void* adp, adapter_info_t*  adapter_info);
    void (*update_adapter_info)(void* adp, adapter_info_t*  adapter_info, krnl_adapter_init_info_t* a_info);
    void (*dump_resource)(struct os_printer *p, void *data, int index, int iga_index);
    void (*debugfs_dump)(struct os_seq_file *seq_file, void *data, int type, void* arg);
    void (*final_cleanup)(void *data, unsigned int gpu_device);
    void (*wait_chip_idle)(void *adapter);
    void (*wait_allocation_idle)(void *data, gf_wait_allocation_idle_t *wait_allocation);
    int (*get_allocation_state)(void *data, gf_get_allocation_state_t *state);
    int (*save_state)(void *adapter, int need_save_memory);
    int  (*restore_state)(void *adapter);
    void (*get_map_allocation_info)(void *data, unsigned int hAllocation, gf_map_argu_t *map);

    int (*query_info)(void* data, gf_query_info_t *info);
    int  (*create_device)(void *data, void *filp, unsigned int *hDevice);
    void (*update_device_name)(void *data, unsigned int device);
    void (*destroy_device)(void *data, unsigned int hDevice);
    int  (*create_allocation)(void *data, gf_create_allocation_t *create_data, void *bo);
    int  (*create_allocation_list)(void *data, gf_create_resource_t *create_data, void **bos);
    int  (*create_allocation_from_pages)(void *data, gf_create_allocation_t *create_data, struct os_pages_memory *pages, void *bo);
    void (*destroy_allocation)(void *data, gf_destroy_allocation_t *destroy_data);
    void (*prepare_and_mark_unpagable)(void *data, unsigned int handle, gf_open_allocation_t *info);
    void (*mark_pagable)(void *data, unsigned int handle);
    int (*set_callback_func)(void *data, int type, void *func, void *argu);
    int (*begin_perf_event)(void *data, gf_begin_perf_event_t *begin_perf_event);
    int (*end_perf_event)(void *data, gf_end_perf_event_t *end_perf_event);
    int (*get_perf_event)(void *data, gf_get_perf_event_t *get_event);
    int (*send_perf_event)(void *data, gf_perf_event_t *perf_event);
    int (*get_perf_status)(void *data, gf_perf_status_t *perf_status);
    int (*begin_miu_dump_perf_event)(void *data, gf_begin_miu_dump_perf_event_t *begin_miu_perf_event);
    int (*end_miu_dump_perf_event)(void *data, gf_end_miu_dump_perf_event_t *end_miu_perf_event);
    int (*set_miu_reg_list_perf_event)(void *data, gf_miu_reg_list_perf_event_t *miu_reg_list);
    int (*get_miu_dump_perf_event)(void *data, gf_get_miu_dump_perf_event_t *get_miu_dump);
    int (*direct_get_miu_dump_perf_event)(void *data, gf_direct_get_miu_dump_perf_event_t *direct_get_miu);
    int (*create_context)(void *data, gf_create_context_t *create_context, enum gf_mem_space mem_space);
    int (*destroy_context)(void *data, gf_destroy_context_t *destroy_context);
    int (*render)(void *data, gf_render_t *render);
    int (*create_di_context)(void *data, gf_create_di_context_t *create);
    int (*destroy_di_context)(void *data, gf_destroy_di_context_t *destroy);
    int (*create_fence_sync_object)(void *data, gf_create_fence_sync_object_t *gf_create, int binding);
    int (*destroy_fence_sync_object)(void *data, gf_destroy_fence_sync_object_t *gf_destroy);
    int (*wait_fence_sync_object)(void *data, gf_wait_fence_sync_object_t *gf_wait);
    int (*fence_value)(void *data, gf_fence_value_t *gf_value);
    int (*is_fence_object_signaled)(void *data, unsigned int fence_sync_object, unsigned long long wait_value);
    int (*is_fence_back)(void *data, unsigned char engine_index, unsigned long long fence_id);
    int (*add_hw_ctx_buf)(void *data, gf_add_hw_ctx_buf_t *add);
    int (*rm_hw_ctx_buf)(void *data, gf_rm_hw_ctx_buf_t *rm);
    int (*cil2_misc)(void *data, gf_cil2_misc_t *misc);
    struct os_pages_memory* (*get_allocation_pages)(void *data, int handle);

    int (*notify_interrupt)(void *data, unsigned int interrupt_event);
    void (*perf_event_add_isr_event)(void *data, gf_perf_event_t *perf_event);
    void (*perf_event_add_event)(void *data, gf_perf_event_t *perf_event);

    void (*dump_hang_info_flag)(void *data, int flag);
    void (*ctl_flags_set)(void *data,unsigned int num,unsigned int mask,unsigned int value);
    int (*hwq_process_vsync_event)(void *data, unsigned long long time);
    void (*task_timeout_update)(void* data, unsigned long long *value, int update);
    void (*reset_dvfs_power_flag)(void* data);
    void (*disp_state_update)(void* data, unsigned int state);
    int (*get_power_state)(void* data, unsigned int *state);
    int (*set_power_state)(void* data, unsigned int state, unsigned int holding_ms, unsigned int force, unsigned int lcok, unsigned int unlock);
} core_interface_t;

extern core_interface_t *gf_core_interface;

#endif




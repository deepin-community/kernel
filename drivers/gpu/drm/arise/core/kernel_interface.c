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
#include "kernel_interface.h"
#include "gf_adapter.h"
#include "context.h"
#include "global.h"
#include "powermgr.h"
#include "vidmm.h"
#include "vidsch.h"
#include "perfevent.h"

krnl_import_func_list_t *gf = NULL;

static int krnl_create_device(void *data, void *filp, unsigned int *hDevice);
static int krnl_create_context(void *data, gf_create_context_t *create_context, enum gf_mem_space mem_space);
static void krnl_destroy_device(void *data, unsigned int hDevice);

static int krnl_cil2_misc(void *data, gf_cil2_misc_t* gf_misc)
{
    adapter_t *adapter = data;
    krnl_cil2_misc_t misc;

    misc.gf_misc = gf_misc;
    misc.context = get_from_handle(&adapter->hdl_mgr, gf_misc->context);
    misc.device = get_from_handle(&adapter->hdl_mgr, gf_misc->device);

    return vidsch_cil2_misc(misc.device, &misc);
}

static void* krnl_pre_init_adapter(void *pdev, krnl_adapter_init_info_t *info, krnl_import_func_list_t *import)
{
    adapter_t *adapter     = NULL;
    platform_caps_t caps   = {0};
    unsigned int enabled = FALSE;

    gf = import;

    util_init_log();

    adapter = gf_calloc(sizeof(adapter_t));

    if(adapter == NULL)
    {
        gf_error("allocate adapter fail!\n");

        return NULL;
    }

    adapter->paging_lock        = gf_create_mutex();
    adapter->device_list_lock   = gf_create_mutex();
    adapter->gart_table_lock    = gf_create_mutex();
    adapter->lock               = gf_create_spinlock(0);

    adapter->vcp_index_cnt[VCP0_INDEX] = 0;
    adapter->vcp_index_cnt[VCP1_INDEX] = 0;
    adapter->start_index = MIN_VIDEO_SEQ_INDEX;
    adapter->end_index = MAX_VIDEO_SEQ_INDEX_ADD1;
    gf_memset((void*)(adapter->bVideoSeqIndex), 0, sizeof(adapter->bVideoSeqIndex));

    list_init_head(&adapter->device_list);
    list_init_head(&adapter->sync_obj_list);

    adapter->index = info->minor_index;
    adapter->os_device.pdev = pdev;

    adapter->ctl_flags.worker_thread_enable = TRUE;
    adapter->ctl_flags.split_enable         = TRUE;
    adapter->hw_caps.fence_interrupt_enable = TRUE;
    adapter->ctl_flags.recovery_enable        = FALSE;
    adapter->ctl_flags.dump_hang_info_to_file = FALSE;
    adapter->ctl_flags.swap_enable            = FALSE;
    adapter->ctl_flags.perf_event_enable      = FALSE;
    adapter->ctl_flags.hwq_event_enable       = FALSE;
    adapter->ctl_flags.local_for_display_only = TRUE;

    gf_query_platform_caps(pdev, &caps);

    adapter->sys_caps.iommu_enabled  = caps.iommu_support;
    adapter->hw_caps.address_mode_64bit = !caps.system_need_dma32;
    adapter->os_page_size  = caps.page_size;
    adapter->os_page_shift = caps.page_shift;
    adapter->suspend_blt_mode = caps.suspend_blt_mode;

    glb_get_pci_config_info(adapter);

    glb_init_chip_id(adapter,info);

    handle_mgr_create(&adapter->hdl_mgr);

    return  adapter;
}

static  void krnl_init_adapter(void* adp, int reserved_vmem, void *disp_info)
{
    adapter_t * adapter     = (adapter_t*)adp;

    adapter->disp_info = disp_info;

    vidmm_init(adapter, reserved_vmem);

    vidsch_create(adapter);

    cm_create_di_mgr(adapter);

    gf_register_trace_events();

    perf_event_init(adapter);

    gf_hwq_event_init(adapter);

    gf_info("adapter->ctl_flags.worker_thread_enable %x\n", adapter->ctl_flags.worker_thread_enable);

    gf_info("sys caps: os page size :0x%x, os page shift:0x%x, iommu_support:%x\n",
            adapter->os_page_size, adapter->os_page_shift,
            adapter->sys_caps.iommu_enabled);

    gf_info("hw cfg: chip slice mask :%x, miu channel index:%x, miu channel size index:%x, backdoor:%d\n",
            adapter->hw_caps.chip_slice_mask, adapter->hw_caps.miu_channel_num,
            adapter->hw_caps.miu_channel_size, adapter->hw_caps.gf_backdoor_enable);

    gf_info("hw caps: secure:%d, snoop:%d, 4K_page:%d, 64K_page:%d, Paging:%d, CG:%d, DFS:%d\n",
         adapter->hw_caps.secure_range_enable, adapter->hw_caps.support_snooping, adapter->hw_caps.page_4k_enable,
         adapter->hw_caps.page_64k_enable, adapter->ctl_flags.paging_enable,
         adapter->pwm_level.EnableClockGating, adapter->hw_caps.dfs_enable);

    gf_info("power caps: ClockGating:%d, PowerGating:%d PowerSwitch:%d\n",
        adapter->pwm_level.EnableClockGating, adapter->pwm_level.EnablePowerGating, adapter->pwm_level.EnablePowerSwitch);

    gf_info("Ctrl: Recovery:%d, WK-thread:%d, Hang-Dump:%d, RunOnQT:%d, PwmMode:0x%x, NonsnoopEnable:%d\n",
        adapter->ctl_flags.recovery_enable, adapter->ctl_flags.worker_thread_enable,
        adapter->ctl_flags.hang_dump, adapter->ctl_flags.run_on_qt, adapter->pm_caps.pwm_mode, adapter->hw_caps.snoop_only ? 0 : 1);

}

static void krnl_deinit_adapter(void *data)
{
    adapter_t *adapter = data;

    perf_event_deinit(adapter);
    gf_hwq_event_deinit(adapter);
    gf_unregister_trace_events();

    cm_destroy_remained_device(adapter);

    vidsch_destroy(adapter);

    vidmm_destroy(adapter);

    cm_destroy_di_mgr(adapter);

    handle_mgr_destroy(&adapter->hdl_mgr);

    gf_destroy_mutex(adapter->gart_table_lock);

    gf_destroy_mutex(adapter->paging_lock);

    gf_destroy_mutex(adapter->device_list_lock);

    gf_destroy_spinlock(adapter->lock);

    glb_fini_bus_config(adapter);

    gf_free(adapter);
}


static  void krnl_get_adapter_info(void*  adp, adapter_info_t*  adapter_info)
{
    adapter_t *adapter = (adapter_t *)adp;
    if(adapter && adapter_info)
    {
        adapter_info->ven_dev = (((unsigned int)adapter->bus_config.vendor_id) << 16 ) + adapter->bus_config.device_id;
        adapter_info->family_id = adapter->family_id;
        adapter_info->generic_id = adapter->generic_id;
        adapter_info->chip_id = adapter->chip_id;
        adapter_info->mmio  = adapter->mmio;
        adapter_info->mmio_size = adapter->mmio_vma->size;
        adapter_info->primary = adapter->primary;
        adapter_info->fb_bus_addr = adapter->vidmm_bus_addr;
        adapter_info->fb_total_size = adapter->Visible_vram_size;
        adapter_info->run_on_qt = adapter->ctl_flags.run_on_qt;
        adapter_info->patch_fence_intr_lost = (adapter->hw_patch_mask0 & PATCH_FENCE_INTERRUPT_LOST) ? 1 : 0;
    }
}

static  void  krnl_update_adapter_info(void* adp, adapter_info_t*  adapter_info, krnl_adapter_init_info_t* a_info)
{
    adapter_t *adapter = (adapter_t *)adp;
    if(adapter && adapter_info)
    {
        adapter->hw_caps.snoop_only = (adapter_info->snoop_only)? 1 : 0;
        adapter->low_top_address = adapter_info->low_top_addr;
        adapter->hw_caps.miu_channel_num = adapter_info->chan_num;
        adapter->hw_caps.chip_slice_mask = adapter_info->chip_slice_mask;
        adapter->hw_caps.fb_hdaudio_enable = (adapter_info->hdaudio_to_local)? 1 : 0;
        adapter->hw_caps.non_simul_chip = (adapter_info->non_simul_chip)? 1 : 0;

        //use default memory size
        if(adapter->Real_vram_size == 0)
        {
            adapter->Real_vram_size = adapter_info->total_mem_size_mb;
            adapter->Real_vram_size <<= 20;
            gf_info("use total_mem_size_mb as local memory\n");
        }
        else if((adapter->Real_vram_size>>20) > adapter_info->total_mem_size_mb)
        {
            gf_info("use gf_local_size as local memory, Real_vram_size:%lldM, total_mem_size_mb:%lldM\n", (adapter->Real_vram_size>>20), adapter_info->total_mem_size_mb);
            gf_warning("set local memory out range actual memory size, force adjust actual memory");
            adapter->Real_vram_size = adapter_info->total_mem_size_mb;
            adapter->Real_vram_size <<= 20;
        }
        else
        {
            gf_info("use params gf_local_size_g to set local memory, not total_mem_size_mb\n");
        }

        adapter->UnAval_vram_size = (adapter_info->total_mem_size_mb - adapter_info->avai_mem_size_mb);
        adapter->UnAval_vram_size <<= 20;
        gf_info("video memory size: %lldM.\n", adapter->Real_vram_size >> 20);
        gf_info("video unav size:   %lldM.\n", adapter->UnAval_vram_size >> 20);

        //slice mask setting from kmd module para, for debug use.
        if(a_info->chip_slice_mask)
        {
            adapter->hw_caps.chip_slice_mask    = a_info->chip_slice_mask;
        }
        //if hang dump , force set slice mask is 0x01
        if (adapter->ctl_flags.hang_dump)
        {
            adapter->hw_caps.chip_slice_mask = 0x01;
        }
        gf_info("krnl_update_adapter_info chip_slice_mask: 0x%x\n", adapter->hw_caps.chip_slice_mask);
    }
}

static void krnl_dump_resource(struct os_printer *p, void *data, int dump_index, int iga_index)
{
    adapter_t *adapter = data;

    switch(dump_index)
    {
       case 0:
           util_print_log();
           break;
       case 1:
           vidmm_dump_resource(adapter);
           break;
       case 2:
           break;
       case 3:
           vidsch_dump(p, adapter);
           break;
       case 4:
           cm_dump_resource(adapter);
           break;
       case 5:
           util_print_log();
           vidmm_dump_resource(adapter);
           vidsch_dump(p,adapter);
           cm_dump_resource(adapter);
           break;
       case 6:
            adapter->display_debugbus_flag = !adapter->display_debugbus_flag;
            gf_info("adapter->display_debugbus_flag = %d\n", adapter->display_debugbus_flag);
            break;
       case 7:
            if (adapter->kickoff_error.error & 0x1)
            {
                gf_error("flush fifo issue index:%x\n", adapter->kickoff_error.RbIndex);
                gf_error("write value: %x, read value: %x\n", adapter->kickoff_error.last_send_fence_id,
                    adapter->kickoff_error.flush_fifo_buffer_value);
                adapter->kickoff_error.error &= ~0x1;
            }

            if (adapter->kickoff_error.error & 0x2)
            {
                gf_error("idx:%d, kick off:%x, read:%x\n", adapter->kickoff_error.RbIndex,
                    adapter->kickoff_error.tail, adapter->kickoff_error.offset);
                adapter->kickoff_error.error &= ~0x2;
            }
            break;
       case 0x7000:
           vidsch_force_wakup(adapter);
           break;
       default:
           break;
    }
}

static void krnl_debugfs_dump(struct os_seq_file *seq_file, void *data, int type, void* arg)
{
    adapter_t *adapter = data;
    switch(type)
    {
        case DEBUGFS_NODE_DEVICE:
        {
            unsigned int handle = *((unsigned int*)arg);

            gpu_device_t *device  = get_from_handle(&adapter->hdl_mgr, handle);

            if(device != NULL)
            {
                cm_dump_device_alloctions(seq_file, adapter, device);
            }
            break;
        }

        case DEBUGFS_NODE_HEAP:
        {
            int id = *((int*)arg);

            vidmm_dump_heap(seq_file, adapter, id);
            break;
        }
        case DEBUGFS_NODE_INFO:
            vidsch_dump_info(seq_file, adapter);
            break;

        case DEBUGFS_NODE_MEMTRACK:
        {
            int pid = *((int *)arg);
            vidmm_dump_memtrack(seq_file, adapter, pid);
            break;
        }

        case DEBUGFS_NODE_VIDSCH:
        {
            struct os_printer *p = (struct os_printer *) arg;
            vidsch_dump(p, adapter);
            break;
        }

        case DEBUGFS_NODE_DEBUGBUS:
        {
            struct os_printer *p = (struct os_printer *) arg;
            vidsch_dump_debugbus(p, adapter);
            break;
        }
        default:
            break;
    }
}


static int krnl_create_device(void *data, void *filp, unsigned int *hDevice)
{
    adapter_t    *adapter = data;
    gpu_device_t *device;

    int status = E_INVALIDARG;

    device = cm_create_device(adapter, filp);

    if((device != NULL) && (hDevice != NULL))
    {
        *hDevice = device->handle;
        gf_memset((void*)(device->video_core_index_cnt), 0, sizeof(device->video_core_index_cnt));
        gf_memset((void*)(device->video_seq_index), 0, sizeof(device->video_seq_index));
        status = S_OK;
    }

    return status;
}

static void krnl_destroy_device(void *data, unsigned int hDevice)
{
    adapter_t    *adapter = data;
    gpu_device_t *device  = get_from_handle(&adapter->hdl_mgr, hDevice);
    int i, j;
    if(device != NULL)
    {
        if(device->video_core_index_cnt[VCP0_INDEX])
            adapter->vcp_index_cnt[VCP0_INDEX] -= device->video_core_index_cnt[VCP0_INDEX];
        if(device->video_core_index_cnt[VCP1_INDEX])
            adapter->vcp_index_cnt[VCP1_INDEX] -= device->video_core_index_cnt[VCP1_INDEX];

        for(i = 0; i < 16; i++)
        {
            if(device->video_seq_index[i] == 0)
                continue;

            for (j = 0; j < 8; j++)
            {
                if((device->video_seq_index[i] & (0x1 << j)) != 0)
                    adapter->bVideoSeqIndex[i * 8 + j] = 0;
            }
            device->video_seq_index[i] = 0;
        }
        cm_destroy_device(adapter, device);
    }
    else
    {
        gf_error("%s: invalid device handle %x.\n", __func__, hDevice);
    }

}

static void krnl_final_cleanup(void *data, unsigned int gpu_device)
{
    adapter_t    *adapter = data;
    gpu_device_t *device  = get_from_handle(&adapter->hdl_mgr, gpu_device);
    int i, j;
    if(device != NULL)
    {
        if(device->video_core_index_cnt[VCP0_INDEX])
            adapter->vcp_index_cnt[VCP0_INDEX] -= device->video_core_index_cnt[VCP0_INDEX];
        if(device->video_core_index_cnt[VCP1_INDEX])
            adapter->vcp_index_cnt[VCP1_INDEX] -= device->video_core_index_cnt[VCP1_INDEX];

        for(i = 0; i < 16; i++)
        {
            if(device->video_seq_index[i] == 0)
                continue;

            for (j = 0; j < 8; j++)
            {
                if((device->video_seq_index[i] & (0x1 << j)) != 0)
                    adapter->bVideoSeqIndex[i * 8 + j] = 0;
            }
            device->video_seq_index[i] = 0;
        }
    }
    cm_destroy_device(adapter, device);
}

static void krnl_wait_chip_idle(void *data)
{
    adapter_t *adapter = data;

    vidsch_wait_chip_idle(adapter, ALL_ENGINE_MASK);
}

static int krnl_create_allocation(void *data, gf_create_allocation_t *create_data, void *bo)
{
    adapter_t     *adapter = data;
    gpu_device_t  *device  = get_from_handle(&adapter->hdl_mgr, create_data->device);
    vidmm_create_allocation_arg_t create = {0};
    vidmm_rename_create_t rename_create = {0,};
    vidmm_gf_create_t gf_create = {0, };
    int result = S_OK;

    if(device == NULL)
    {
        gf_error("%s: invalid device handle %x.\n", __func__, create_data->device);

        result = E_INVALIDARG;

        return result;
    }

    if (create_data->reference)
    {
        vidmm_allocation_t  *reference = get_from_handle(&adapter->hdl_mgr, create_data->reference);

        rename_create.reference     = reference;
        create.allocation_count     = 1;
        create.create_type          = VIDMM_CREATE_TYPE_RENAME;
        create.rename_list          = &rename_create;
    }
    else
    {
        gf_create.create_data       = create_data;
        create.allocation_count     = 1;
        create.create_type          = VIDMM_CREATE_TYPE_GF;
        create.create_list          = &gf_create;
    }

    create.bos          = &bo;

    result = vidmm_create_allocation(device, &create);

    if (create.create_type == VIDMM_CREATE_TYPE_RENAME)
    {
        create_data->size       = rename_create.Size;
        create_data->allocation = rename_create.hAllocation;
    }

    return result;
}

static int krnl_create_allocation_from_pages(void *data, gf_create_allocation_t *create_data, struct os_pages_memory *pages, void *bo)
{
    adapter_t     *adapter = data;
    gpu_device_t  *device  = get_from_handle(&adapter->hdl_mgr, create_data->device);
    vidmm_create_allocation_arg_t create = {0};
    vidmm_gf_create_t gf_create = {0, };
    int result = S_OK;

    if(device == NULL)
    {
        gf_error("%s: invalid device handle %x.\n", __func__, create_data->device);

        result = E_INVALIDARG;

        return result;
    }

    gf_create.create_data       = create_data;
    gf_create.import_pages_mem  = pages;

    create.create_type          = VIDMM_CREATE_TYPE_GF;
    create.allocation_count     = 1;
    create.create_list          = &gf_create;
    create.bos                  = &bo;

    result = vidmm_create_allocation(device, &create);

    return result;
}

static int krnl_create_allocation_list(void *data, gf_create_resource_t *create_data, void **bos)
{
    adapter_t        *adapter = data;
    gpu_device_t     *device  = get_from_handle(&adapter->hdl_mgr, create_data->device);
    vidmm_create_allocation_arg_t create = {0};

    create.create_type      = VIDMM_CREATE_TYPE_ESCAPE;
    create.allocation_count = create_data->NumAllocations;
    create.info_list        = ptr64_to_ptr(create_data->pAllocationInfo);
    create.bos = bos;

    return vidmm_create_allocation(device, &create);
}

static void krnl_destroy_allocation(void *data, gf_destroy_allocation_t *destroy_data)
{
    adapter_t          *adapter    = data;
    gpu_device_t       *device     = get_from_handle(&adapter->hdl_mgr, destroy_data->device);
    vidmm_allocation_t *allocation = get_from_handle(&adapter->hdl_mgr, destroy_data->allocation);

    vidmm_destroy_allocatin_arg_t destroy = {0};

    if (device == NULL && allocation != NULL)
    {
        device = allocation->device;
    }

    if(device == NULL)
    {
        gf_error("%s: invalid device handle %x.\n", __func__, destroy_data->device);

        return;
    }

    if(allocation == NULL)
    {
        gf_error("%s: invalid allocation handle %x.\n", __func__, destroy_data->allocation);

        return;
    }

    destroy.allocation_count = 1;
    destroy.allocation_list  = &allocation;

    vidmm_destroy_allocation(device, &destroy);
}

static void krnl_get_map_allocation_info(void *data, unsigned int hAllocation, gf_map_argu_t *map)
{
    adapter_t          *adapter = data;
    vidmm_allocation_t *allocation = get_from_handle(&adapter->hdl_mgr, hAllocation);

    return vidmm_get_map_allocation_info(adapter, allocation, map);
}

static void krnl_wait_allocation_idle(void *data, gf_wait_allocation_idle_t *wait_allocation)
{
    adapter_t          *adapter    = data;
    vidmm_allocation_t *allocation = get_from_handle(&adapter->hdl_mgr, wait_allocation->hAllocation);

    gf_begin_section_trace_event("wait_allocation_idle");
    gf_counter_trace_event("arg_allocation", wait_allocation->hAllocation);

    vidsch_wait_allocation_idle(adapter, wait_allocation->engine_mask, 0, allocation);

    gf_end_section_trace_event(0);
}

static int krnl_save_state(void *data, int need_save_memory)
{
    adapter_t *adapter = data;
    return  pm_save_state(adapter, need_save_memory);
}

static int krnl_restore_state(void *data)
{
    adapter_t *adapter = data;
    int        result  = S_OK;

    result = pm_restore_state(adapter);

    return result;
}

static int krnl_query_info(void* data, gf_query_info_t *info)
{
    adapter_t     *adapter = data;
    int           status   = 0;
    int           search_num = 0;
    gpu_device_t *device  = NULL;

    switch (info->type)
    {
    case GF_QUERY_TOTAL_VRAM_SIZE:
    case GF_QUERY_RESERV_VRAM_SIZE:
    case GF_QUERY_CPU_VISIBLE_VRAM_SIZE:
    case GF_QUERY_LOCAL_VRAM_TYPE:
    case GF_QUERY_HEIGHT_ALIGN:
    case GF_QUERY_SEGMENT_FREE_SIZE:
    case GF_QUERY_SEGMENT_MEM_INFO:
    case GF_QUERY_ALLOCATION_INFO:
    case GF_QUERY_ALLOCATION_INFO_KMD:
    case GF_QUERY_LOCAL_ALLOCATION_MAX_SIZE:
    case GF_SET_PERF_SWAP_HINT:
    case GF_GET_PERF_SWAP_HINT:
        status = vidmm_query_info(adapter, info);
        break;
    case GF_QUERY_ENGINE_USAGE_STATUS:
        {
            gf_hwq_info   hwq_info_info={0} ;
            hwq_info_info.Usage_3D=adapter->usage_3d;
            hwq_info_info.Usage_VCP=adapter->usage_vcp;
            hwq_info_info.Usage_VPP=adapter->usage_vpp;
            gf_copy_to_user(info->buf, &hwq_info_info, sizeof(gf_hwq_info));
        }
        break;
    case GF_QUERY_ENGINE_USAGE_STATUS_EXT:
        {
            gfx_hwq_info_ext   tmp_hwq_info_ext={0} ;
            hwq_get_hwq_info_ext(adapter,&tmp_hwq_info_ext);
            gf_copy_to_user(info->buf, &tmp_hwq_info_ext, sizeof(gfx_hwq_info_ext));
        }
        break;
    case GF_QUERY_HW_HANG:
    case GF_QUERY_PENDING_FRAME_NUM:
    case GF_QUERY_GET_VIDEO_BRIDGE_BUFFER:
    case GF_QUERY_GPU_TIME_STAMP:
    case GF_SET_MIU_REGISTER_U32:
    case GF_QUERY_MIU_REGISTER_U32:
    case GF_QUERY_REGISTER_U32:
    case GF_SET_REGISTER_U32:
    case GF_QUERY_VCP_INDEX:
    case GF_QUERY_PROCESS_INFO:
        status = vidsch_query_info(adapter, info);
        break;

    case GF_QUERY_PAGE_SWIZZLE_SUPPORT:
        info->value = 0;
        break;

    case GF_QUERY_CHIP_ID:
        info->value = adapter->chip_id;
        break;

    case GF_QUERY_VENDOR_ID:
        info->value = adapter->bus_config.vendor_id;
        break;

    case GF_QUERY_DEVICE_ID:
        info->value = adapter->bus_config.device_id;
        break;

    case GF_QUERY_REVISION_ID:
        info->value = adapter->bus_config.revision_id;
        break;

    case GF_QUERY_CHIP_SLICE_MASK:
        info->value = adapter->hw_caps.chip_slice_mask;
        break;

    case GF_QUERY_SECURED_ON:
        info->value = adapter->sys_caps.secure_on;
        break;

    case GF_QUERY_RET_VCP_INDEX:
        if (info->value > VCP1_INDEX || info->value < VCP0_INDEX)
        {
            gf_error("unknown arg (%d) for GF_QUERY_RET_VCP_INDEX\n", info->argu);
            status = -1;
        }
        else
        {
            adapter->vcp_index_cnt[info->value]--;
            device = get_from_handle(&adapter->hdl_mgr, info->argu);
            if(device)
                device->video_core_index_cnt[info->value]--;
        }
        break;

    case GF_QUERY_VIDEO_SEQ_INDEX:
        while(adapter->start_index < adapter->end_index)
        {
            if(adapter->bVideoSeqIndex[adapter->start_index] == 0)
            {
                info->value = adapter->start_index;
                adapter->bVideoSeqIndex[adapter->start_index] = 1;
                device = get_from_handle(&adapter->hdl_mgr, info->argu);
                if(device)
                {
                    device->video_seq_index[adapter->start_index / 8] |= 0x1 << (adapter->start_index % 8);
                    adapter->start_index = (adapter->start_index + 1) % MAX_VIDEO_SEQ_INDEX_ADD1;
                }
                status = 0;
                break;
            }

            adapter->start_index++;
            if(adapter->start_index == adapter->end_index)
            {
                search_num++;
                adapter->start_index = MIN_VIDEO_SEQ_INDEX;
            }

            if(search_num == 2)
            {
               status = -1;
               break;
            }
        }
        break;

    case GF_QUERY_RET_VIDEO_SEQ_INDEX:
        if(info->value >= MIN_VIDEO_SEQ_INDEX && (info->value < MAX_VIDEO_SEQ_INDEX_ADD1))
        {
            adapter->bVideoSeqIndex[info->value] = 0;
            device = get_from_handle(&adapter->hdl_mgr, info->argu);
            if(device)
                device->video_seq_index[info->value/8] &= ~(0x1 << (info->value % 8));
        }
        break;

    case GF_QUERY_ADAPTER_INFO:
        {
            adapter_t           *adapter   = data;
            gf_adapter_info_t   adapter_info = {0};
            bus_config_t        *bus_info  = &adapter->bus_config;

            adapter_info.bus_config.DeviceID    = bus_info->device_id;
            adapter_info.bus_config.VendorID    = bus_info->vendor_id;
            adapter_info.bus_config.Command     = bus_info->command;
            adapter_info.bus_config.Status      = bus_info->status;
            adapter_info.bus_config.RevisionID  = bus_info->revision_id;
            adapter_info.bus_config.ProgIf      = bus_info->prog_if;
            adapter_info.bus_config.SubClass    = bus_info->sub_class;
            adapter_info.bus_config.BaseClass   = bus_info->base_class;
            adapter_info.bus_config.LatencyTimer= bus_info->latency_timer;
            adapter_info.bus_config.HeaderType  = bus_info->header_type;
            adapter_info.bus_config.BIST        = bus_info->bist;
            adapter_info.bus_config.LinkStatus  = bus_info->link_status;

            adapter_info.bus_config.ulBaseAddresses[0] = 0;
            adapter_info.bus_config.ulBaseAddresses[1] = 0;
            adapter_info.bus_config.ulBaseAddresses[2] = 0;
            adapter_info.bus_config.ulBaseAddresses[3] = 0;
            adapter_info.bus_config.ulBaseAddresses[4] = 0;
            adapter_info.bus_config.ulBaseAddresses[5] = 0;

            adapter_info.bus_config.CacheLineSize      = bus_info->cache_line_size;
            adapter_info.bus_config.SubsystemVendorID  = bus_info->sub_sys_vendor_id;

            adapter_info.bus_config.memorySize         = adapter->Real_vram_size - adapter->UnAval_vram_size;
            adapter_info.chipslicemask                 = adapter->hw_caps.chip_slice_mask;
            adapter_info.gpucount                      = 1;
            adapter_info.osversion                     = 0;
            adapter_info.bVideoOnly                    = adapter->hw_caps.video_only;
            adapter_info.bCTEDumpEnable                = 0;
            adapter_info.non_simul_chip                = adapter->hw_caps.non_simul_chip;

            adapter_info.segmentMapTable[0]            = 0;
            adapter_info.segmentMapTable[1]            = 1;//fb low
            adapter_info.segmentMapTable[4]            = 2;//non-snoop
            adapter_info.segmentMapTable[3]            = 3;//snoop
            adapter_info.segmentMapTable[2]            = 4;//fb high

            gf_copy_to_user(info->buf, &adapter_info, sizeof(gf_adapter_info_t));
        }

        break;

    case GF_QUERY_ACTIVE_ENGINE_COUNT:
        info->value = adapter->active_engine_count;
        break;

    case GF_QUERY_VCP_INFO:
        {
            int index = (info->argu << 16) >> 16;
            int op    = (info->argu >> 16);

            if(index < 0 || index >= VCP_INFO_COUNT) {
                gf_error("invalid index-%d\n", index);
                return -1;
            }

            switch(op) {
                case 0:
                {
                    int i;
                    for(i=0; i<VCP_INFO_COUNT; i++) {
                        if(adapter->vcp_info[i].enable == 0) {
                            gf_copy_from_user(&adapter->vcp_info[i], info->buf, sizeof(gf_vcp_info));
                            adapter->vcp_info[i].enable = 1;
                            index = i;
                            break;
                        }
                    }

                    break;
                }
                case 1:
                    adapter->vcp_info[index].enable = 0;
                    adapter->vcp_info[index].pid = 0;
                    break;
                case 2:
                    gf_copy_to_user(info->buf, &adapter->vcp_info[index], sizeof(gf_vcp_info));
                    break;
                case 4:
                {
                    int i;
                    for(i=0; i<VCP_INFO_COUNT; i++) {
                        adapter->vcp_info[i].enable = 0;
                        adapter->vcp_info[i].pid = 0;
                    }
                    break;
                }
                case 3:
                default:
                    gf_copy_from_user(&adapter->vcp_info[index], info->buf, sizeof(gf_vcp_info) - 2*sizeof(int));//don't overwrite last two parameter
                    break;
            }

            info->argu = index;
        }
        break;

    case GF_QUERY_DIAGS:
        {
            status = vidmm_query_info(adapter, info);
            info->diags.device_id = adapter->bus_config.device_id;
            info->diags.vendor_id = adapter->bus_config.vendor_id;
            info->diags.pci_link_speed = adapter->bus_config.link_status & 0xf;
            info->diags.pci_link_width = (adapter->bus_config.link_status & 0x3f0) >> 4;
        }
        break;
    default:
        gf_assert(0, "unknown query type");
        status = -1;
        break;
    }

    return status;
}

static int krnl_set_callback_func(void *data, int type, void *func, void *argu)
{
    adapter_t     *adapter = data;
    int           status   = S_OK;

    switch (type)
    {
    case OS_CALLBACK_POST_EVENT:
        adapter->post_event_argu = argu;
        adapter->post_event      = func;
        break;
    case OS_CALLBACK_POST_SYNCOBJ_EVENT:
        adapter->post_sync_event_argu = argu;
        adapter->post_sync_event      = func;
        break;
    case OS_CALLBACK_DRM_CB:
        adapter->drm_cb = func;
        adapter->drm_cb_argu = argu;
        break;
    default:
        gf_error("unknown callback func type: %d.\n", type);
        status = E_INVALIDARG;
        break;
    }

    return status;
}

static int krnl_begin_perf_event(void *data, gf_begin_perf_event_t *begin_perf_event)
{
    adapter_t    *adapter = data;
    int result = 0;

    result = perf_event_begin(adapter, begin_perf_event);

    return result;
}

static int krnl_begin_miu_dump_perf_event(void *data, gf_begin_miu_dump_perf_event_t *begin_miu_perf_event)
{
    adapter_t *adapter = data;
    int result = 0;

    result = perf_event_begin_miu_dump(adapter, begin_miu_perf_event);

    return result;
}

static int krnl_end_perf_event(void *data, gf_end_perf_event_t *end_perf_event)
{
    adapter_t    *adapter = data;
    int result = 0;

    result = perf_event_end(adapter, end_perf_event);

    return result;
}

static int krnl_end_miu_dump_perf_event(void *data, gf_end_miu_dump_perf_event_t *end_miu_perf_event)
{
    adapter_t *adapter = data;
    int result = 0;

    result = perf_event_end_miu_dump(adapter, end_miu_perf_event);

    return result;
}

static int krnl_get_miu_dump_perf_event(void *data, gf_get_miu_dump_perf_event_t *get_miu_dump)
{
    adapter_t *adapter = data;
    int result = 0;

    result = perf_event_get_miu_event(adapter, get_miu_dump);

    return result;
}

static int krnl_direct_get_miu_dump_perf_event(void *data, gf_direct_get_miu_dump_perf_event_t *direct_get_dump)
{
    adapter_t *adapter = data;
    int result = 0;

    result = perf_event_direct_get_miu_dump_event(adapter, direct_get_dump);

    return result;
}

static int krnl_get_perf_event(void *data, gf_get_perf_event_t *get_event)
{
    adapter_t    *adapter = data;
    int result = 0;

    result = perf_event_get_event(adapter, get_event);

    return result;
}

static int krnl_set_miu_reg_list_perf_event(void *data, gf_miu_reg_list_perf_event_t *miu_reg_list)
{
    adapter_t *adapter = data;
    int result = 0;

    result = perf_event_set_miu_reg_list(adapter, miu_reg_list);

    return result;
}

static int krnl_send_perf_event(void *data, gf_perf_event_t *perf_event)
{
    adapter_t    *adapter = data;
    int result = 0;

    result = perf_event_add_event(adapter, perf_event);

    return result;
}

static int krnl_get_perf_status(void *data, gf_perf_status_t *perf_status)
{
    adapter_t   *adapter = data;
    int result = 0;

    result = perf_event_get_status(adapter, perf_status);

    return result;
}

static int krnl_create_context(void *data, gf_create_context_t *create_context, enum gf_mem_space mem_space)
{
    adapter_t        *adapter = data;
    gpu_device_t     *device  = get_from_handle(&adapter->hdl_mgr, create_context->device);
    gpu_context_t    *context = NULL;
    create_context_t create   = {0};
    int is_kernel = (mem_space == GF_MEM_KERNEL) ? 1 : 0;

    /**
     * TODO: remove this later.
    */
    if (!(create_context->device & 0xFF000000))
    {
        gf_info("type not correct, refuse to create context\n");
        return E_FAIL;
    }

    if (!device || !device->adapter)
    {
        gf_info("invalid device, refuse to create context\n");
        return E_FAIL;
    }
    //multi-engine
    create.node_ordinal    = create_context->engine_index;
    create.flags           = create_context->flags;

    context = cm_create_context(device, &create, is_kernel);

    if(context)
    {
        create_context->context                  = context->handle;
        return S_OK;
    }

    return E_FAIL;
}

static int krnl_destroy_context(void *data, gf_destroy_context_t *destroy_context)
{
    adapter_t     *adapter = data;
    gpu_device_t  *device  = get_from_handle(&adapter->hdl_mgr, destroy_context->device);
    gpu_context_t *context = get_from_handle(&adapter->hdl_mgr, destroy_context->context);

    cm_destroy_context(device, context);

    return S_OK;
}

static int krnl_create_di_context(void *data, gf_create_di_context_t *create)
{
    adapter_t        *adapter = data;
    gpu_device_t     *device  = get_from_handle(&adapter->hdl_mgr, create->device);
    di_context_t     *context;

    int status = E_FAIL;

    if(device == NULL)
    {
        gf_error("bad argument device: %x, %p.\n", create->device, device);

        return E_FAIL;
    }

    context = cm_create_di_context(device, create);

    if(context != NULL)
    {
        create->context = context->handle;
        create->hw_idx  = context->hw_idx;

        status = S_OK;
    }

    return status;
}

static int krnl_destroy_di_context(void *data, gf_destroy_di_context_t *destroy_context)
{
    adapter_t     *adapter = data;
    gpu_device_t  *device  = get_from_handle(&adapter->hdl_mgr, destroy_context->device);
    di_context_t  *context = get_from_handle(&adapter->hdl_mgr, destroy_context->context);

    if((device == NULL) || (context == NULL))
    {
        gf_error("bad argument: device: %x, %p. context: %x, %p.\n",
            destroy_context->device, device, destroy_context->context, context);

        return E_FAIL;
    }

    cm_destroy_di_context(device, context);

    return S_OK;
}

static int krnl_render(void *data, gf_render_t *render)
{
    int           ret;
    adapter_t     *adapter     = data;
    gpu_context_t *gpu_context = get_from_handle(&adapter->hdl_mgr, render->context);
    vidsch_render_t render_data = {0};

    render_data.command_length           = render->command_length;
    render_data.flags                    = render->flags;
    render_data.allocation_list_size     = render->allocation_count;
    render_data.patch_location_list_size = render->patch_location_count;
    render_data.sync_object_list_size    = render->sync_object_count;
    render_data.allocation_list          = (void*)(unsigned long)render->allocation_list;
    render_data.patch_location_list      = (void*)(unsigned long)render->patch_location_list;
    render_data.sync_object_list         = (void*)(unsigned long)render->sync_object_list;
    render_data.cmdbuf_count             = render->cmdbuf_count;
    render_data.render_counter           = gpu_context->render_counter++;
    render_data.cmdbuf_array             = (void*)(unsigned long)render->cmdbuf_array;

    ret = vidsch_render(gpu_context, &render_data);

    return ret;
}

static int krnl_create_fence_sync_object(void *data, gf_create_fence_sync_object_t *create, int binding)
{
    adapter_t     *adapter = data;
    gpu_device_t  *device  = get_from_handle(&adapter->hdl_mgr, create->device);
    unsigned int  status   = E_FAIL;

    status = vidsch_create_sync_object(device, create, binding);

    return status;
}

static int krnl_destroy_fence_sync_object(void *data, gf_destroy_fence_sync_object_t *destroy)
{
    adapter_t     *adapter = data;
    gpu_device_t  *device  = get_from_handle(&adapter->hdl_mgr, destroy->device);
    unsigned int  status   = S_OK;

    status = vidsch_destroy_sync_object(adapter, device, destroy->fence_sync_object);

    return status;
}

static int krnl_wait_fence_sync_object(void *data, gf_wait_fence_sync_object_t *wait)
{
    adapter_t     *adapter = data;
    gpu_context_t *context = get_from_handle(&adapter->hdl_mgr, wait->context);
    unsigned int  status   = S_OK;

    status = vidsch_wait_sync_object(context, wait);

    return status;
}

static int krnl_is_fence_object_signaled(void *data, unsigned int fence_sync_object, unsigned long long wait_value)
{
    adapter_t     *adapter = data;
    return vidsch_is_fence_signaled(adapter,fence_sync_object,wait_value);
}

static int krnl_is_fence_back(void *data, unsigned char engine_index, unsigned long long fence_id)
{
    adapter_t     *adapter = data;

    return vidsch_is_fence_back(adapter, engine_index, fence_id);
}

static int krnl_fence_value(void *data, gf_fence_value_t *value)
{
    adapter_t     *adapter = data;
    gpu_device_t  *device  = get_from_handle(&adapter->hdl_mgr, value->device);
    unsigned int  status   = E_FAIL;

    status = vidsch_fence_value(device, value);

    return status;
}

static int krnl_get_allocation_state(void *data, gf_get_allocation_state_t *state)
{
    adapter_t          *adapter      = data;
    vidmm_allocation_t *allocation   = get_from_handle(&adapter->hdl_mgr, state->hAllocation);
    unsigned int engine_mask = state->engine_mask;

    if (!allocation)
        return E_INVALIDARG;

    if (!vidsch_is_allocation_idle(adapter, engine_mask, allocation))
    {
        state->state = 1;
    }
    else
    {
        state->state = 0;
    }

    return 0;
}


static int krnl_add_hw_ctx_buf(void *data, gf_add_hw_ctx_buf_t *add)
{
    adapter_t        *adapter = data;
    gpu_device_t     *device  = get_from_handle(&adapter->hdl_mgr, add->device);
    gpu_context_t    *context = get_from_handle(&adapter->hdl_mgr, add->context);
    int ret = 0;

    ret = cm_add_hwctx_buffer(device, context, add->hw_ctx_buf_index);

    return ret;
}

static int  krnl_rm_hw_ctx_buf(void *data, gf_rm_hw_ctx_buf_t *rm)
{
    adapter_t        *adapter = data;
    gpu_device_t     *device  = get_from_handle(&adapter->hdl_mgr, rm->device);
    gpu_context_t    *context = get_from_handle(&adapter->hdl_mgr, rm->context);
    int ret = 0;

    ret = cm_remove_hwctx_buffer(device, context, rm->hw_ctx_buf_index);

    return ret;
}

static struct os_pages_memory* krnl_get_allocation_pages(void *data, int handle)
{
    adapter_t        *adapter = data;

    vidmm_allocation_t *allocation = get_from_handle(&adapter->hdl_mgr, handle);

    return allocation->pages_mem;
}

static int krnl_notify_interrupt(void *data, unsigned int interrupt_event)
{
    adapter_t        *adapter = data;

    return vidsch_notify_interrupt(adapter, interrupt_event);
}

static void krnl_prepare_and_mark_unpagable(void *data, unsigned int handle, gf_open_allocation_t *info)
{
    adapter_t           *adapter = data;
    vidmm_allocation_t  *allocation = get_from_handle(&adapter->hdl_mgr, handle);

    vidmm_prepare_and_mark_unpagable(adapter, allocation, info);
}

static void krnl_mark_pagable(void *data, unsigned int handle)
{
    adapter_t           *adapter = data;
    vidmm_allocation_t  *allocation = get_from_handle(&adapter->hdl_mgr, handle);

    vidmm_mark_pagable(adapter, allocation);
}

static void krnl_perf_event_add_isr_event(void *data, gf_perf_event_t *perf_event)
{
    adapter_t       *adapter = data;
    if (adapter->ctl_flags.perf_event_enable && adapter->perf_event_mgr)
    {
        perf_event_add_isr_event(adapter, perf_event);
    }
}

static void krnl_perf_event_add_event(void *data, gf_perf_event_t *perf_event)
{
    adapter_t       *adapter = data;
    if (adapter->ctl_flags.perf_event_enable && adapter->perf_event_mgr)
    {
        perf_event_add_event(adapter, perf_event);
    }
}

static int krnl_hwq_process_vsync_event(void *data,unsigned long long time)
{
    adapter_t       *adapter = data;
    int ret =0;
    if (adapter->ctl_flags.hwq_event_enable && adapter->hwq_event_mgr)
    {
        ret=hwq_process_vsync_event(adapter,time);
    }
    return ret;
}

static void krnl_update_device_name(void *data, unsigned int device)
{
    adapter_t     *adapter = data;
    gpu_device_t  *gpu_device  = get_from_handle(&adapter->hdl_mgr, device);

    if (gpu_device)
    {
        gpu_device->pid = gf_get_current_pid();
        gpu_device->tid = gf_get_current_tid();
        gf_get_current_pname(gpu_device->pname, GF_MAX_PNAME_LEN);
    }
}

static void krnl_dump_hang_info_flag(void *data, int flag)
{
    adapter_t     *adapter = data;

    adapter->display_debugbus_flag = flag;
    gf_info("adapter->display_debugbus_flag = %d\n", flag);
}

static void krnl_ctl_flags_set(void *data,unsigned int num,unsigned int mask,unsigned int value)
{
    adapter_t *adapter = data;
    unsigned int *ctl_flags = (unsigned int *)&adapter->ctl_flags;
    if( num > (sizeof(adapter->ctl_flags)/sizeof(unsigned int)) || num <=0 ) return;

    ctl_flags   += (num-1);
    *ctl_flags  &= ~mask;
    *ctl_flags  |= value;
}

static void krnl_task_timeout_update(void* data, unsigned long long *value, int update)
{
    adapter_t *adapter = data;

    if (update)
        adapter->hw_hang_fast_timeout_ns = *value * 1000000;
    else
        *value = gf_do_div(adapter->hw_hang_fast_timeout_ns, 1000000);
}

static void krnl_reset_dvfs_power_flag(void* data)
{
    adapter_t *adapter = data;

    vidsch_dvfs_power_flag_reset(adapter);
}

static void krnl_disp_state_update(void *data, unsigned int state)
{
    adapter_t *adapter = data;

    adapter->disp_state = state;
}

static int krnl_get_power_state(void *data, unsigned int *state)
{
    adapter_t *adapter = data;

    if (!adapter->pwm_level.EnablePowerSwitch)
        return -1;

    if (state)
        *state = adapter->power_state;

    return 0;
}

static int krnl_set_power_state(void *data, unsigned int state, unsigned int holding_ms, unsigned int force, unsigned int lock, unsigned int unlock)
{
    adapter_t *adapter = data;

    if (!adapter->pwm_level.EnablePowerSwitch)
        return -1;

    if (state == ENG_POWER_STATE_HOLD)
        adapter->power_state_hold = !adapter->power_state_hold;

    if (adapter->power_state_hold)
        return 0;

    if (state >= ENG_POWER_STATE_AUTO)
    {
        state = ENG_POWER_STATE_AUTO;
        lock = FALSE;
        unlock = TRUE;
    }

    if (lock || unlock)
        adapter->ctl_flags.boost = TRUE;

    vidsch_set_power_state(adapter, state, holding_ms, force);

    if (lock)
        adapter->ctl_flags.boost = FALSE;

    return 0;
}

static core_interface_t gfe3k_gpu_core = {
#define INTERFACE(item) .item = krnl_##item
    INTERFACE(pre_init_adapter),
    INTERFACE(init_adapter),
    INTERFACE(deinit_adapter),
    INTERFACE(get_adapter_info),
    INTERFACE(update_adapter_info),
    INTERFACE(dump_resource),
    INTERFACE(debugfs_dump),
    INTERFACE(final_cleanup),
    INTERFACE(wait_chip_idle),
    INTERFACE(wait_allocation_idle),
    INTERFACE(get_allocation_state),
    INTERFACE(save_state),
    INTERFACE(restore_state),
    INTERFACE(query_info),
    INTERFACE(create_device),
    INTERFACE(destroy_device),
    INTERFACE(create_allocation),
    INTERFACE(create_allocation_list),
    INTERFACE(create_allocation_from_pages),
    INTERFACE(destroy_allocation),
    INTERFACE(set_callback_func),
    INTERFACE(begin_perf_event),
    INTERFACE(end_perf_event),
    INTERFACE(get_perf_event),
    INTERFACE(send_perf_event),
    INTERFACE(get_perf_status),
    INTERFACE(begin_miu_dump_perf_event),
    INTERFACE(end_miu_dump_perf_event),
    INTERFACE(set_miu_reg_list_perf_event),
    INTERFACE(get_miu_dump_perf_event),
    INTERFACE(direct_get_miu_dump_perf_event),
    INTERFACE(create_context),
    INTERFACE(destroy_context),
    INTERFACE(render),
    INTERFACE(create_di_context),
    INTERFACE(destroy_di_context),
    INTERFACE(create_fence_sync_object),
    INTERFACE(destroy_fence_sync_object),
    INTERFACE(wait_fence_sync_object),
    INTERFACE(fence_value),
    INTERFACE(is_fence_object_signaled),
    INTERFACE(is_fence_back),
    INTERFACE(add_hw_ctx_buf),
    INTERFACE(rm_hw_ctx_buf),
    INTERFACE(cil2_misc),
    INTERFACE(get_allocation_pages),
    INTERFACE(notify_interrupt),
    INTERFACE(get_map_allocation_info),
    INTERFACE(prepare_and_mark_unpagable),
    INTERFACE(mark_pagable),
    INTERFACE(perf_event_add_isr_event),
    INTERFACE(perf_event_add_event),
    INTERFACE(update_device_name),
    INTERFACE(dump_hang_info_flag),
    INTERFACE(ctl_flags_set),
    INTERFACE(hwq_process_vsync_event),
    INTERFACE(task_timeout_update),
    INTERFACE(reset_dvfs_power_flag),
    INTERFACE(disp_state_update),
    INTERFACE(get_power_state),
    INTERFACE(set_power_state),
#undef INTERFACE
};

core_interface_t *gf_core_interface = &gfe3k_gpu_core;

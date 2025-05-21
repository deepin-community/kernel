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
#include "gf_adapter.h"
#include "global.h"
#include "context.h"
#include "vidsch.h"
#include "vidmm.h"
#include "vidmmi.h"

static unsigned long long gf_get_current_time(void)
{
    long time_sec = 0, time_usec = 0;
    unsigned long long ts = 0;

    gf_getsecs(&time_sec, &time_usec);

    ts =  time_sec * 1000 + gf_do_div(time_usec, 1000);

    return ts;
}

gpu_device_t* cm_create_device(adapter_t *adapter, void *filp)
{
    gpu_device_t *device;

    device = gf_calloc(sizeof(gpu_device_t));

    if(device == NULL)
    {
        gf_error("create gpu device: alloc memory failed!\n");
        return NULL;
    }

    device->adapter  = adapter;
    device->filp     = filp;
    device->pid      = gf_get_current_pid();
    device->tid      = gf_get_current_tid();
    device->handle   = add_handle(&adapter->hdl_mgr, HDL_TYPE_DEVICE, device);
    device->lock     = gf_create_mutex();
    device->sync_obj_count = gf_create_atomic(0);

    gf_get_current_pname(device->pname, GF_MAX_PNAME_LEN);

    list_init_head(&device->context_list);
    list_init_head(&device->hwctx_list);
    list_init_head(&device->sync_obj_list);
    list_init_head(&device->allocation_open_instance_list);
    list_init_head(&device->display_stream_list);
    list_init_head(&device->di_context_list);

    cm_device_create_num_inc(device);

    /* add device to adapter device list */
    gf_mutex_lock(adapter->device_list_lock);

    list_add_tail(&device->list_node, &adapter->device_list);

    gf_mutex_unlock(adapter->device_list_lock);

    return device;
}

void cm_destroy_device(adapter_t *adapter, gpu_device_t *device)
{
    gpu_context_t   *context = NULL, *context_next;
    hw_ctxbuf_t     *hwctx = NULL,   *hwctx_next;
    di_context_t    *dictx = NULL,   *dictx_next;

    /* put destory_context first, since destroy context default need wait chip idle
     * so no need call wait chip idle here.
     */
    /* destroy exist context */
    list_for_each_entry_safe(context, context_next, &device->context_list, list_node)
    {
        cm_destroy_context(device, context);
    }

    /* destroy exist hwctx */
    list_for_each_entry_safe(hwctx, hwctx_next, &device->hwctx_list, list_node)
    {
        cm_remove_hwctx_buffer(device, NULL, hwctx->index);
    }

    /* destroy exist di_context */
    list_for_each_entry_safe(dictx, dictx_next, &device->di_context_list, list_node)
    {
        cm_destroy_di_context(device, dictx);
    }

    /* destroy exist allocation */

    /* NOTE:
     * here can not use list_for_each_entry_safe,
     * but always get node in head because destroy allocation, not only free itself
     * but also try to free renamed allocation, so the list->next, maybe freed
     * after destroy curr allocation.
     */
#if 0
    gf_assert(list_empty(&device->allocation_open_instance_list), "instance_list not empty");
    while(!list_empty(&device->allocation_open_instance_list))
    {
        vidmm_destroy_allocatin_arg_t destroy     = {0};

        cm_allocation_open_instance_t *instance   = list_entry(device->allocation_open_instance_list.next,
                                                               cm_allocation_open_instance_t,
                                                               list_node);
        vidmm_allocation_t            *allocation = instance->allocation;

        destroy.allocation_count = 1;
        destroy.allocation_list  = &allocation;

        //this allocation only be opened in current device, force destroy it
        cm_device_lock_allocation(device, allocation);
        allocation->ref_count = allocation->ref_count - instance->ref_cnt + 1;
        cm_device_unlock_allocation(device, allocation);



        // reset allocation reference count to 1, so will delete this instance when destory allocation.
        instance->ref_cnt = 1;

#if 0
        /* if want trace user mode allocation leak pls open code, all allocation freed here is leak */
        gf_info("allocation:%x, open device:%x, create device:%x, perferred_segment:%x, size: %d. AT:%d\n",
            allocation->handle, device->handle, allocation->device->handle, allocation->preferred_segment.value,
            allocation->orig_size, allocation->at_type);
#endif
        vidmm_destroy_allocation(device, &destroy);
    }
#endif

    vidmm_destroy_defer_allocation(adapter);

    /* destroy exist sync_obj */
    vidsch_destroy_remained_sync_obj(device);

    /* remove device from adapter device list */
    gf_mutex_lock(adapter->device_list_lock);

    list_del(&device->list_node);

    gf_mutex_unlock(adapter->device_list_lock);

    cm_device_create_num_dec(device);

}

void cm_destroy_remained_device(adapter_t *adapter)
{
    gpu_device_t    *device      = NULL;
    gpu_device_t    *device_next = NULL;

    //destroy exist device when deinit adapter
    list_for_each_entry_safe(device, device_next, &adapter->device_list, list_node)
    {
        cm_destroy_device(adapter, device);
    }
}

inline int cm_get_hw_ringbuffer_index(adapter_t *adapter, unsigned int node_ordinal)
{
    int  i;

    for(i = 0; i < adapter->active_engine_count; i++)
    {
        if(adapter->engine_index_table[i].node_ordinal == node_ordinal)
        {
            break;
        }
    }

    gf_assert(i < adapter->active_engine_count, "i < adapter->active_engine_count");
    return i;
}

void cm_reset_command_header(gpu_context_t *context)
{
}

gpu_context_t* cm_create_context(gpu_device_t *device, create_context_t *create_context, int is_kernel)
{
    gpu_context_t       *context    = gf_calloc(sizeof(gpu_context_t));

    if(context == NULL)
    {
        gf_error("create gpu context fail: out of memory!\n");
        return NULL;
    }

    context->device       = device;
    context->pid          = gf_get_current_pid();
    context->tid          = gf_get_current_tid();
    gf_get_current_pname(context->pname, GF_MAX_PNAME_LEN);
    context->handle       = add_handle(&device->adapter->hdl_mgr, HDL_TYPE_CONTEXT, context);
    context->ref_cnt      = gf_create_atomic(0);
    context->engine_index = cm_get_hw_ringbuffer_index(device->adapter, create_context->node_ordinal);
    context->is_kernel    = is_kernel;

    cm_device_lock(device);

    list_add(&context->list_node, &device->context_list);

    cm_device_unlock(device);

    cm_reset_command_header(context);

    return context;
}

void cm_wait_context_idle(gpu_context_t *context)
{
    adapter_t * adapter = context->device->adapter;
    int times = 0, idle = FALSE;

    while(times++ < adapter->context_destroy_timeout)
    {
        if(gf_atomic_read(context->ref_cnt) != 0)
        {
            vidsch_release_completed_tasks(adapter, context->engine_index, NULL);

            gf_msleep(10);
        }
        else
        {
            idle = TRUE;
            break;
        }
    }

    /* if not idle, seems something wrong, maybe sync wait a never returned value, set task dropped skip wait */
    if(!idle)
    {
        context->device->task_dropped = TRUE;

        for(times = 0; times < 1000; times++)
        {
            /* notify a INT_FENCE to let vidsch reschedule the task. since we
             * set task_dropped here then, schedule will dropped the task not submitted
             */
            vidsch_notify_interrupt(context->device->adapter, 0);

            gf_msleep(10);

            if(gf_atomic_read(context->ref_cnt) == 0)
            {
                idle = TRUE;
                break;
            }
        }

        if(!idle)
        {
            vidsch_dump(NULL,adapter);
        }
        gf_warning("wait context not idle\n");
    }
}


void cm_destroy_context(gpu_device_t *device, gpu_context_t *context)
{
    cm_wait_context_idle(context);

    remove_handle(&device->adapter->hdl_mgr, context->handle);

    cm_device_lock(device);

    list_del(&context->list_node);

    cm_device_unlock(device);

    context->device = NULL;

    gf_destroy_atomic(context->ref_cnt);

    gf_free(context);
}

inline static unsigned int cmi_acquire_physical_context_index(adapter_t  *adapter)
{
    unsigned char* context_mask = adapter->context_buffer_inuse;
    unsigned int   mask_num     = 0xFFFF/8;
    unsigned int   i,j;
    unsigned int   physical_context_index = 0xFFFF;//no phys contex index, no using deferred context

    for( i=0; i<mask_num; i++ )
    {
        if( context_mask[i] == 0xFF ) continue;

        for( j=0; j<8; j++ )
        {
            if( context_mask[i] & (1<<j) ) continue;
            physical_context_index = i*8+j;
            context_mask[i] |= (1<<j);
            break;
        }

        if( physical_context_index ) break;
    }

    return physical_context_index;
}

inline static void cmi_release_physical_context_index(adapter_t *adapter, unsigned int physical_context_index)
{
    if( physical_context_index && physical_context_index < 0xFFFF )
    {
        unsigned int i = physical_context_index/8;
        unsigned int j = physical_context_index%8;
        adapter->context_buffer_inuse[i] &= ~(1<<j);
    }
}

int cm_add_hwctx_buffer(gpu_device_t *device, gpu_context_t *context, unsigned int hw_buf_index)
{
    adapter_t   *adapter       = device->adapter;
    hw_ctxbuf_t *hw_ctx_buffer = gf_calloc(sizeof(hw_ctxbuf_t));

    if(hw_ctx_buffer == NULL)
    {
        return E_OUTOFMEMORY;
    }

    hw_ctx_buffer->index      = hw_buf_index;
    hw_ctx_buffer->phys_index = cmi_acquire_physical_context_index(adapter);

    cm_device_lock(device);

    list_add(&hw_ctx_buffer->list_node, &device->hwctx_list);

    cm_device_unlock(device);

    return S_OK;
}

int cm_remove_hwctx_buffer(gpu_device_t *device, gpu_context_t *context, unsigned int hwctx_index)
{
    hw_ctxbuf_t *hw_ctx  = NULL;
    int          status  = E_FAIL;

    cm_device_lock(device);

    list_for_each_entry(hw_ctx, &device->hwctx_list, list_node)
    {
        if(hw_ctx->index == hwctx_index)
        {
            list_del(&hw_ctx->list_node);

            status = S_OK;

            break;
        }
    }

    cm_device_unlock(device);

    if(status == S_OK)
    {
        cmi_release_physical_context_index(device->adapter, hw_ctx->phys_index);

        gf_free(hw_ctx);
    }

    return status;
}

hw_ctxbuf_t *cm_get_hwctx_buffer(gpu_device_t *device, gpu_context_t *context, unsigned int hwctx_index)
{
    hw_ctxbuf_t *hw_ctx      = NULL;
    hw_ctxbuf_t *match_hwctx = NULL;

    if(hwctx_index == 0)
    {
        return &context->hw_ctx;
    }

    cm_device_lock(device);

    list_for_each_entry(hw_ctx, &device->hwctx_list, list_node)
    {
        if(hw_ctx->index == hwctx_index)
        {
            match_hwctx = hw_ctx;
            break;
        }
    }

    cm_device_unlock(device);

    return match_hwctx;
}

int  cm_save(adapter_t *adapter, int need_save_memory)
{
    vidmm_mgr_t                   *mm_mgr = adapter->mm_mgr;
    vidmm_segment_t               *segment = NULL;
    unsigned int                  idx, priority;
    int i = 0;
    int result = S_OK;

    unsigned int             allocation_cnt = 0;
    unsigned long long ts_enter =  gf_get_current_time();
    unsigned long long ts_leave =  0;

    /* not add lock to protect device list here, since save only used in suspend,
     * no multi-thread issue here
     */
    for(i = 0; i < adapter->active_engine_count; i++)
    {
        vidsch_wait_engine_idle(adapter, i);
    }

    for (idx = 1; idx < mm_mgr->segment_cnt; idx++)
    {
        segment = &mm_mgr->segment[idx];
        if (!segment->flags.require_system_pages)
        {
            vidmm_allocation_t *allocation = NULL;

            for (priority = PDISCARD; priority < PALL; priority++)
            {
                list_for_each_entry(allocation, &segment->pagable_resident_list[priority], list_item)
                {
                    allocation_cnt++;

                    result = vidmm_save_allocation(allocation->device, allocation);
                    if (result)
                    {
                         return result;
                    }
                }
            }

            list_for_each_entry(allocation, &segment->unpagable_resident_list, list_item)
            {
                allocation_cnt++;

                result = vidmm_save_allocation(allocation->device, allocation);
                if (result)
                {
                     return result;
                }
            }
        }
    }


    ts_leave = gf_get_current_time();

    gf_info("%s() total cost %llums, allocation_cnt-%d\n", __func__, ts_leave -ts_enter, allocation_cnt);

    return result;
}

void cm_restore(adapter_t *adapter)
{
    vidmm_mgr_t                   *mm_mgr = adapter->mm_mgr;
    vidmm_segment_t               *segment = NULL;
    unsigned int                  idx, priority;

    unsigned int             allocation_cnt = 0;
    unsigned long long ts_enter =  gf_get_current_time();
    unsigned long long ts_leave =  0;

    /* not add lock to protect device list here, since save only used in resume,
     * no multi-thread issue here
     */

    if (adapter->suspend_blt_mode == 2)
    {
        /* TODO: remove this */
        gf_msleep(500);
        gf_info("pending sleep finish\n");
    }

    for (idx = 1; idx < mm_mgr->segment_cnt; idx++)
    {
        segment = &mm_mgr->segment[idx];
        if (!segment->flags.require_system_pages)
        {
            vidmm_allocation_t *allocation = NULL;

            for (priority = PDISCARD; priority < PALL; priority++)
            {
                list_for_each_entry(allocation, &segment->pagable_resident_list[priority], list_item)
                {
                    allocation_cnt++;
                    vidmm_restore_allocation(allocation->device, allocation);
                }
            }

            list_for_each_entry(allocation, &segment->unpagable_resident_list, list_item)
            {
                allocation_cnt++;
                vidmm_restore_allocation(allocation->device, allocation);
            }
        }
    }

    ts_leave = gf_get_current_time();

    gf_info("%s() total cost %llums, allocation_cnt-%d\n", __func__, ts_leave -ts_enter, allocation_cnt);
}

int cm_device_add_reference(gpu_device_t *device, struct _vidmm_allocation *allocation)
{
    cm_allocation_open_instance_t *instance  = gf_calloc(sizeof(cm_allocation_open_instance_t));
    cm_allocation_open_instance_t *reference = NULL, *ite_instance = NULL;

    instance->allocation = allocation;
    instance->device     = device;

    cm_device_lock_allocation(device, allocation);

    list_for_each_entry(ite_instance, &allocation->device_ref_list, device_ref_node)
    {
        if(ite_instance->device == device)
        {
            reference = ite_instance;
            break;
         }
     }

    if(reference != NULL)
    {
        reference->ref_cnt ++;
    }
    else
    {
        instance->ref_cnt = 1;

        list_add_tail(&instance->device_ref_node, &allocation->device_ref_list);

        list_add_tail(&instance->list_node, &device->allocation_open_instance_list);
    }

    cm_device_unlock_allocation(device, allocation);

    if(reference != NULL)
    {
        gf_free(instance);
    }

    return S_OK;

}

int cm_device_del_reference(gpu_device_t *device, struct _vidmm_allocation *allocation)
{
    cm_allocation_open_instance_t *instance = NULL, *reference = NULL;

    int result = E_FAIL;

    cm_device_lock_allocation(device, allocation);

    list_for_each_entry(instance, &allocation->device_ref_list, device_ref_node)
    {
        if(instance->device == device)
        {
            reference = instance;

            break;
        }
    }

    if(reference != NULL)
    {
        reference->ref_cnt--;

        if(reference->ref_cnt == 0)
        {
            list_del(&reference->list_node);

            list_del(&reference->device_ref_node);
        }
        else
        {
            reference = NULL;
        }

        result = S_OK;
    }

    cm_device_unlock_allocation(device, allocation);

    if(reference != NULL)
    {
        gf_free(reference);
    }

    return result;
}

void cm_device_create_num_inc(gpu_device_t *device)
{
    cm_device_lock(device);

    device->create_num++;

    cm_device_unlock(device);
}

void cm_device_create_num_dec(gpu_device_t *device)
{
    int free_device = FALSE;

    cm_device_lock(device);

    device->create_num--;

    if(device->create_num == 0)
    {
        free_device = TRUE;
    }

    cm_device_unlock(device);

    if(free_device)
    {
        gf_destroy_mutex(device->lock);
        gf_destroy_atomic(device->sync_obj_count);

        remove_handle(&device->adapter->hdl_mgr, device->handle);

        gf_free(device);
    }
}

void cm_device_lock_allocation(gpu_device_t *device, struct _vidmm_allocation *allocation)
{
    cm_device_lock(device);

    gf_spin_lock(allocation->lock);
}

void cm_device_unlock_allocation(gpu_device_t *device, struct _vidmm_allocation *allocation)
{
    gf_spin_unlock(allocation->lock);

    cm_device_unlock(device);
}

void cm_dump_resource(adapter_t *adapter)
{
    gpu_device_t  *device = NULL, *device_next;
    gpu_context_t *context = NULL, *context_next;

    gf_info("-----------current device info & context info.------------------\n");

    list_for_each_entry_safe(device, device_next, &adapter->device_list, list_node)
    {
        gf_info("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
        gf_info("**device:  %x, pid: %ld, create tid: %ld, create allocation num: %4d, sync obj count %d.\n",
            device->handle, device->pid, device->tid, device->create_num - 1, gf_atomic_read(device->sync_obj_count));

        list_for_each_entry_safe(context, context_next, &device->context_list, list_node)
        {
            gf_info("context: %x, on engine: %d, tid: %ld, device: %x", context->handle, context->engine_index, context->tid, device->handle);
        }

#if 0
        gf_info("## device %x opened allocation list.\n", device->handle);

        list_for_each_entry_safe(instance, instance_next, &device->allocation_open_instance_list, list_node)
        {
            gf_info("allocation:%x, open device:%x, create device:%x\n",
                instance->allocation->handle, device->handle, instance->allocation->device->handle);
        }
#endif
        gf_info("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
    }
}

void cm_dump_device_alloctions(struct os_seq_file *seq_file, adapter_t *adapter,  gpu_device_t *device)
{
    cm_allocation_open_instance_t *instance = NULL, *instance_next;
    int i = 0;
    int cnt = vidmm_get_segment_count(adapter);

    gf_seq_printf(seq_file, "          pid         dev         client\n");
    gf_seq_printf(seq_file, "   %10d    %8x         %s     \n", device->pid, device->handle, device->pname);

    cm_device_lock(device);

    for(i = 0; i < cnt; i++)
    {
        unsigned int num=0, size=0;
        gf_seq_printf(seq_file, "----------------------------------------------------------------------\n");
        list_for_each_entry_safe(instance, instance_next, &device->allocation_open_instance_list, list_node)
        {
            vidmm_allocation_t            *allocation = instance->allocation;

            if(allocation->segment_id != i)
            {
                continue;
            }

            gf_seq_printf(seq_file, "allo: %x, dev: %x, fmt: %8d-%8d-%d-%3d, W-H-P: %8d-%8d-%8d, size: %6dk, gvm: %llx, AT:0x%02x, status: %8x, ref_cnt: %d. pid: %d, tid: %d, proc: %s\n",
                allocation->handle, allocation->device->handle,
                allocation->hw_format, allocation->compress_format, allocation->flag.swizzled, allocation->bit_count,
                allocation->width, allocation->height, allocation->pitch,
                util_align(allocation->orig_size, util_max(allocation->alignment, adapter->os_page_size)) >> 10,
                allocation->phys_addr, allocation->at_type,
                allocation->status.temp_unpagable, instance->ref_cnt,
                allocation->device->pid, allocation->device->tid, allocation->device->pname);

            num++;
            size += util_align(allocation->orig_size, util_max(allocation->alignment, adapter->os_page_size));
        }

        gf_seq_printf(seq_file, "\n");
        gf_seq_printf(seq_file, "heap%d     allocation num:%.8d, total size %.8dk\n", i, num, size >> 10);
    }

    gf_seq_printf(seq_file, "----------------------------------------------------------------------\n");

    cm_device_unlock(device);
}


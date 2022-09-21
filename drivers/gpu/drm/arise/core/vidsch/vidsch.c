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

#include "gf_adapter.h"
#include "global.h"
#include "vidmm.h"
#include "vidsch.h"
#include "vidsch_sync.h"
#include "vidschi.h"
#include "vidsch_submit.h"
#include "vidsch_workerthread.h"
#include "perfevent.h"

static vidsch_fence_buffer_t *vidschi_create_fence_buffer(adapter_t *adapter, unsigned int segment_id, int buffer_size);
static void vidschi_destroy_fence_buffer(adapter_t *adapter, vidsch_fence_buffer_t *fence_buf);

extern vidschedule_chip_func_t vidschedule_chip_func;

void vidsch_force_wakup(adapter_t *adapter)
{
    vidsch_mgr_t *vidsch = NULL;
    int i = 0;

    for(i = 0; i < adapter->active_engine_count; i++)
    {
        vidsch = adapter->sch_mgr[i];

        if (!vidsch) continue;

        if (vidsch->worker_thread)
        {
            util_wakeup_event_thread(vidsch->worker_thread);
        }
    }
}

int vidsch_create(adapter_t *adapter)
{
    vidschedule_t           *schedule              = NULL;
    vidsch_mgr_t            *sch_mgr               = NULL;
    vidmm_segment_memory_t  *dma_reserved_memory   = NULL;
    vidmm_segment_memory_t  *local_reserved_memory = NULL;
    vidmm_segment_memory_t  *pcie_reserved_memory  = NULL;
    vidsch_query_private_t   query_data            = {0};
    vidmm_map_flags_t        flags                 = {0};
    int                      i                     = 0;
    int                      ret                   = S_OK;

    /* initialize GPU schedule */
    schedule = gf_calloc(sizeof(vidschedule_t));

    schedule->adapter = adapter;
    for(i=0; i<MAX_CORE_CRTCS; i++)
    {
        schedule->pending_frame_num[i] = gf_create_atomic(0);
    }

    adapter->schedule = schedule;


    schedule->chip_func = &vidschedule_chip_func;

    schedule->rw_lock = gf_create_rwsema();

    schedule->dvfs_status_lock = gf_create_spinlock(0);
    schedule->power_status_lock = gf_create_spinlock(0);

    ret = vidsch_query_chip(adapter, &query_data);

    adapter->active_engine_count = query_data.engine_count;

    adapter->fence_buf = vidschi_create_fence_buffer(adapter, query_data.fence_buffer_segment_id, 68*1024);

    /* initilize GPU engine */
    for (i = 0; i < query_data.engine_count; i++)
    {
        if(query_data.engine_ctrl[i] & ENGINE_CTRL_DISABLE)
        {
            continue;
        }

        if(query_data.engine_caps[i] & ENGINE_CAPS_PAGING)
        {
            adapter->paging_engine_index = i;
        }
       
        sch_mgr = gf_calloc(sizeof(vidsch_mgr_t));

        gf_assert(sch_mgr != NULL, "sch_mgr != NULL");
 
        adapter->sch_mgr[i] = sch_mgr;

        sch_mgr->adapter         = adapter;
        sch_mgr->engine_caps     = query_data.engine_caps[i];
        sch_mgr->engine_ctrl     = query_data.engine_ctrl[i];
        sch_mgr->engine_index    = i;
        sch_mgr->chip_func       = query_data.engine_func[i];
        sch_mgr->init_submit     = 1;
        sch_mgr->hw_queue_size   = query_data.engine_hw_queue_size[i];

        list_init_head(&sch_mgr->allocated_task_list);
        list_init_head(&sch_mgr->submitted_task_list);

        sch_mgr->last_send_fence_id = 0;
        sch_mgr->returned_fence_id  = 0;
        sch_mgr->fence_event        = gf_create_event(0);
        sch_mgr->fence_lock         = gf_create_spinlock(0);
        sch_mgr->power_status_lock  = gf_create_spinlock(0);

        sch_mgr->engine_lock        = gf_create_mutex();

        //gf_info("schedule_serialize[%d] = %d\n", i, query_data.schedule_serialize[i]);

        if (query_data.schedule_serialize[i])
        {
            sch_mgr->task_id_lock       = gf_create_mutex();
        }
        else
        {
            sch_mgr->task_id_lock       = NULL;
        }

        sch_mgr->dma_buffer_segment_id = query_data.dma_segment[i];

        /* allocate dma buffer  */
        dma_reserved_memory = vidmm_allocate_segment_memory(adapter, sch_mgr->dma_buffer_segment_id, query_data.dma_buffer_size[i], 1);

        /* map dma buffer */
        gf_memset(&flags, 0, sizeof(flags));

        flags.write_only = 1;
        flags.mem_space  = GF_MEM_KERNEL;

        vidmm_map_segment_memory(adapter, NULL, dma_reserved_memory, &flags);

        sch_mgr->dma_reserved_memory = dma_reserved_memory;
        /* initial dma heap */
        sch_mgr->dma_heap = gf_calloc(sizeof(heap_t));

        heap_init(sch_mgr->dma_heap, i, dma_reserved_memory->gpu_virt_addr, query_data.dma_buffer_size[i], adapter->os_page_size);

        sch_mgr->task_list_lock = gf_create_mutex();

        /* reserved memory for chip private initialization use */
        if(query_data.local_memory_size[i])
        {
            /* allocate local reserved memory */
            local_reserved_memory = vidmm_allocate_segment_memory(adapter, query_data.local_segment_id, query_data.local_memory_size[i], 1);
            
            /* map local reserved memory */
            gf_memset(&flags, 0, sizeof(flags));

            flags.mem_space  = GF_MEM_KERNEL;

            vidmm_map_segment_memory(adapter, NULL, local_reserved_memory, &flags);

            sch_mgr->local_reserved_memory = local_reserved_memory;

            gf_debug("engine %d local reserved_memory segment_id:%d  gpu_virt_addr:%x\n", i, query_data.local_segment_id, local_reserved_memory->gpu_virt_addr);

        }

        if(query_data.pcie_memory_size[i])
        {
            /* allocate pcie reserved memory */
            pcie_reserved_memory = vidmm_allocate_segment_memory(adapter, query_data.pcie_segment_id, query_data.pcie_memory_size[i], 1);

            /* map pcie reserved memory */
            gf_memset(&flags, 0, sizeof(flags));
            flags.write_only = 1;
            flags.mem_space  = GF_MEM_KERNEL;

            vidmm_map_segment_memory(adapter, NULL, pcie_reserved_memory, &flags);
			
            gf_memset(pcie_reserved_memory->vma->virt_addr, 0, query_data.pcie_memory_size[i]);

            sch_mgr->pcie_reserved_memory = pcie_reserved_memory;

            gf_debug("engine %d pcie reserved_memory segment_id:%d  gpu_virt_addr:%x\n", i, query_data.pcie_segment_id, pcie_reserved_memory->gpu_virt_addr);
        }

        ret = sch_mgr->chip_func->initialize(adapter, sch_mgr);

        if (ret != S_OK)
        {
            goto exit;
        }

        vidschi_init_task_pool(&sch_mgr->normal_task_pool, TRUE);
        vidschi_init_task_pool(&sch_mgr->emergency_task_pool, TRUE);
        sch_mgr->total_task_num = gf_create_atomic(0);

        if (adapter->ctl_flags.worker_thread_enable && (sch_mgr->engine_ctrl & ENGINE_CTRL_THREAD_ENABLE))
        {
            char thread_name[256];
            sch_mgr->normal_pool_sema    = gf_create_sema(PENDING_TASK_QUEUE_LENGTH);
            sch_mgr->emergency_pool_sema = gf_create_sema(PENDING_TASK_QUEUE_LENGTH);
            gf_vsprintf(thread_name, "kthread-%d", i);
            sch_mgr->worker_thread = util_create_event_thread(vidsch_worker_thread_event_handler, sch_mgr, thread_name, 0);
        }

        sch_mgr->engine_dvfs_power_on = TRUE; //default is on
        sch_mgr->hang_index = -1; //default is invalid value
        sch_mgr->hang_fence_id = 0; //default is invalid value
        sch_mgr->hang_hw_copy_mem = 0; //default is 0
    }

    adapter->hw_reset_lock = gf_create_mutex();

    vidschi_init_daemon_thread(adapter);

    //vidschi_dump_buffer(adapter);
exit:
    return ret;
}

/* check if fence is back in target engine */
int vidsch_is_fence_back(adapter_t *adapter, unsigned char engine_index, unsigned long long fence_id)
{
    vidsch_mgr_t  *sch_mgr = adapter->sch_mgr[engine_index];
    int           ret      = FALSE;

    if (sch_mgr->returned_fence_id >= fence_id)
    {
        ret = TRUE;
    } 

    return ret;
}


void vidsch_update_engine_idle_status(adapter_t *adapter, unsigned int engine_mask)
{
    vidsch_mgr_t  *sch_mgr  = NULL;
    vidschedule_t *schedule = adapter->schedule;
    unsigned long long timestamp  = 0ll;

    unsigned int i = 0;
    unsigned int task_list_empty;
    unsigned int work_thread_idel;
    unsigned int fence_back;
    unsigned long flags;

    schedule->busy_engine_mask = 0;

    for(i = 0; i < adapter->active_engine_count; i++)
    {

        if((engine_mask & (1<<i)) == 0) continue;

        sch_mgr = adapter->sch_mgr[i];
        if(sch_mgr == NULL) continue;

        //IRQ cann't handle MUTEX
        //task_list_empty = list_empty(&sch_mgr->allocated_task_list) && list_empty(&sch_mgr->submitted_task_list);
        work_thread_idel = (sch_mgr->emergency_task_pool.task_num == 0 && sch_mgr->normal_task_pool.task_num == 0);


        flags = gf_spin_lock_irqsave(sch_mgr->fence_lock);
        fence_back = sch_mgr->returned_fence_id >= sch_mgr->last_send_fence_id ? 1:0;
        gf_spin_unlock_irqrestore(sch_mgr->fence_lock,flags);

        gf_get_nsecs(&timestamp);
        timestamp = gf_do_div(timestamp, 1000000);

        //task_list_empty &&
        if(work_thread_idel && fence_back && !sch_mgr->hang_hw_copy_mem)
        {
            sch_mgr->last_idle = timestamp;
            sch_mgr->completely_idle = 1;
            sch_mgr->idle_elapse = sch_mgr->last_idle - sch_mgr->last_busy;
            schedule->busy_engine_mask &= ~(1 << i);
        }
        else
        {
            sch_mgr->last_busy = timestamp;
            sch_mgr->completely_idle = 0;
            sch_mgr->idle_elapse = 0;
            schedule->busy_engine_mask |= 1 << i;
        }
    }
}

int vidsch_is_fence_back_condition(void *argu)
{
    vidsch_wait_fence_t *wait_fence = argu;

    return vidsch_is_fence_back(wait_fence->adapter, wait_fence->engine, wait_fence->fence_id);
}

int inline vidschi_allocation_in_cmd_queue(unsigned char engine_index, vidmm_allocation_t *allocation)
{
    return allocation->render_count[engine_index] == 0 ? FALSE : TRUE;
}

int inline vidschi_allocation_in_write_cmd_queue(unsigned char engine_index, vidmm_allocation_t *allocation)
{
    return allocation->write_render_count[engine_index] == 0 ? FALSE : TRUE;
}

static int vidsch_is_allocation_idle_condition(void *argu)
{
    int ret;
    vidsch_wait_fence_t *wait_fence = argu;
    int engine_index = wait_fence->engine;
    vidmm_allocation_t *allocation = (vidmm_allocation_t*)wait_fence->allocation;

    gf_spin_lock(allocation->lock);

    if (wait_fence->read_only)
    {
        ret = !vidschi_allocation_in_write_cmd_queue(engine_index, allocation) &&
            vidsch_is_fence_back(wait_fence->adapter, engine_index, allocation->write_fence_id[engine_index]);
    }
    else
    {
        ret = !vidschi_allocation_in_cmd_queue(engine_index, allocation) &&
            vidsch_is_fence_back(wait_fence->adapter, engine_index, allocation->fence_id[engine_index]);
    }

    gf_spin_unlock(allocation->lock);
    

    return ret;
}

/** vidsch_wait_fence_interrupt implemented by vidsch_wait_fence_back.
 *  So if use vidsch_wait_fence_interrupt, before wait check, we need 
 *  log current fence id, if check failed, we can directly wait 
 *  fence_id + 1 back. 
 **/
extern void vidschi_notify_fence_interrupt(adapter_t *adapter, unsigned int engine_mask);

void vidsch_wait_fence_back(adapter_t *adapter, unsigned char engine_index, unsigned long long fence_id)
{
    vidschedule_t       *schedule        = adapter->schedule;
    vidsch_mgr_t        *sch_mgr         = adapter->sch_mgr[engine_index];    
    vidsch_wait_fence_t  wait_fence      = {0};
    condition_func_t     condition       = vidsch_is_fence_back_condition;
    gf_event_status_t   e_status        = GF_EVENT_UNKNOWN;
    int                  timeout         = adapter->hw_caps.fence_interrupt_enable ? 2000*10:2;
    int timeout_cnt = 0, max_timeout     = 8000 *100 /timeout;

    wait_fence.adapter  = adapter;
    wait_fence.engine   = engine_index;
    wait_fence.fence_id = fence_id;

    while(e_status != GF_EVENT_BACK)
    {
        e_status = gf_wait_event_thread_safe(sch_mgr->fence_event, condition, &wait_fence, timeout);

        /* general timeout should not happen, but only HW hang, or interrupt disabled(we only update fence id on ISR)
         * So if interrutp disabled, we need a place to update fence too, we add here. 
         */
        
        /*
         * Notice: pls do not add any printk to here, consider this: when system reboot, interrupt already disabled,  
         * we need use timeout to update the fence, if fb enabled and printk add here, it will lead a dead loop like 
         * this, we loop to wait hw idle, but we add some draw on this loop, this lead this loop never end.
         */
        if(e_status == GF_EVENT_TIMEOUT)
        {   
            timeout_cnt++;

            if (timeout_cnt > max_timeout)
            {
                gf_info("FENCE not back. engine index %x, wait fence id %lld, last send: %lld, last_return: %lld\n", 
                    engine_index, fence_id, sch_mgr->last_send_fence_id, sch_mgr->returned_fence_id);

                gf_dump_stack();

                max_timeout = 0;
                break;
            }

            /* timeout happen, force notify a interrupt to update fence id */
            vidsch_notify_interrupt(adapter, 0);
        }
    }
}

static void vidschi_wait_allocation_idle(adapter_t *adapter, unsigned char engine_index, vidmm_allocation_t *allocation, int read_only)
{
    vidschedule_t       *schedule        = adapter->schedule;
    vidsch_mgr_t        *sch_mgr         = adapter->sch_mgr[engine_index];    
    vidsch_wait_fence_t  wait_fence      = {0};
    condition_func_t     condition       = &vidsch_is_allocation_idle_condition;
    gf_event_status_t   e_status        = GF_EVENT_UNKNOWN;
    int                  timeout         = adapter->hw_caps.fence_interrupt_enable ? 2000*10:2;
    int timeout_cnt = 0, max_timeout     = 8000 *100 /timeout;

    wait_fence.adapter  = adapter;
    wait_fence.engine   = engine_index;
    wait_fence.allocation = allocation;
    wait_fence.read_only = read_only;

    while(e_status != GF_EVENT_BACK)
    {
        e_status = gf_wait_event_thread_safe(sch_mgr->fence_event, condition, &wait_fence, timeout);

        /* general timeout should not happen, but only HW hang, or interrupt disabled(we only update fence id on ISR)
         * So if interrutp disabled, we need a place to update fence too, we add here. 
         */
        
        /*
         * Notice: pls do not add any printk to here, consider this: when system reboot, interrupt already disabled,  
         * we need use timeout to update the fence, if fb enabled and printk add here, it will lead a dead loop like 
         * this, we loop to wait hw idle, but we add some draw on this loop, this lead this loop never end.
         */
        if(e_status == GF_EVENT_TIMEOUT)
        {   
            timeout_cnt++;

            if (timeout_cnt > max_timeout)
            {
                gf_info("Wait idle timeout. engine index %x, allocation %x\n",
                    engine_index, allocation->handle);

                gf_dump_stack();

                max_timeout = 0;
            }

            /* timeout happen, force notify a interrupt to update fence id */
            vidsch_notify_interrupt(adapter, 0);
        }
    }
}

void vidsch_wait_engine_idle(adapter_t *adapter, int idx)
{
    vidsch_mgr_t *sch_mgr = adapter->sch_mgr[idx];

    if (!sch_mgr)
        return;

    if(sch_mgr->worker_thread)
    {
        vidschi_wait_worker_thread_idle(sch_mgr);
    }
       
    vidsch_wait_fence_back(sch_mgr->adapter, sch_mgr->engine_index, sch_mgr->last_send_fence_id);

}


void vidschi_flush_gpu_cache(adapter_t *adapter)
{
    task_paging_t *paging_task                        = vidsch_allocate_paging_task(adapter, 4*1024, 1);

    paging_task->paging_allocation_num                = 0;
    paging_task->command_length                       = 16;
    paging_task->dma->size                            = 4*1024;

    gf_memset(paging_task->dma->virt_base_addr, 0, paging_task->command_length);

    vidsch_submit_paging_task(adapter, paging_task);
}

void vidsch_wait_chip_idle(adapter_t *adapter, unsigned int engine_mask)
{
    int i = 0;

    if (adapter->hw_caps.video_only)
    {
        engine_mask &= ((1 << 7) | (1 << 8) | (1 << 9));
    }

    for(i = 0; i < adapter->active_engine_count; i++)
    {
        if((engine_mask & ( 1 << i)) && (adapter->sch_mgr[i] != NULL))
        {
            vidsch_wait_engine_idle(adapter, i);
        }
    }
}

int vidsch_destroy(adapter_t *adapter)
{
    vidschedule_t           *schedule        = adapter->schedule;
    vidsch_mgr_t            *sch_mgr         = NULL;
    vidmm_segment_memory_t  *reserved_memory = NULL;
    int                      i               = 0;

    vidschi_deinit_daemon_thread(adapter);

    gf_destroy_mutex(adapter->hw_reset_lock);

    for(i = 0; i < MAX_CORE_CRTCS; i++)
    {
        gf_destroy_atomic(schedule->pending_frame_num[i]);
    }

    for (i = 0; i < MAX_ENGINE_COUNT; i++)
    {
        sch_mgr = adapter->sch_mgr[i];

        if (sch_mgr == NULL)
        {
            continue;
        }

        /* wait worker thread idle and kill it */
        vidschi_wait_worker_thread_idle(sch_mgr);

        vidsch_release_completed_tasks(adapter, sch_mgr->engine_index, NULL);

        vidschi_release_allocated_tasks(sch_mgr);

        gf_destroy_event(sch_mgr->fence_event);
        gf_destroy_spinlock(sch_mgr->fence_lock);
        gf_destroy_spinlock(sch_mgr->power_status_lock);

        util_destroy_event_thread(sch_mgr->worker_thread);

        gf_destroy_sema(sch_mgr->normal_pool_sema);
        gf_destroy_sema(sch_mgr->emergency_pool_sema);

        vidschi_fini_task_pool(&sch_mgr->normal_task_pool);
        vidschi_fini_task_pool(&sch_mgr->emergency_task_pool);

        gf_destroy_atomic(sch_mgr->total_task_num);

        gf_destroy_mutex(sch_mgr->task_list_lock);

        /* free reserved video and pcie memory */
        if(sch_mgr->pcie_reserved_memory)
        {
            reserved_memory = sch_mgr->pcie_reserved_memory;

            vidmm_unmap_segment_memory(adapter, reserved_memory, GF_MEM_KERNEL);

            vidmm_release_segment_memory(adapter, reserved_memory);

            sch_mgr->pcie_reserved_memory = NULL;
        }

        if(sch_mgr->local_reserved_memory)
        {
            reserved_memory = sch_mgr->local_reserved_memory;

            vidmm_unmap_segment_memory(adapter, reserved_memory, GF_MEM_KERNEL);

            vidmm_release_segment_memory(adapter,reserved_memory);

            sch_mgr->local_reserved_memory = NULL;
        }

        if(sch_mgr->dma_reserved_memory)
        {
            reserved_memory = sch_mgr->dma_reserved_memory;

            vidmm_unmap_segment_memory(adapter, reserved_memory, GF_MEM_KERNEL);

            vidmm_release_segment_memory(adapter, reserved_memory);

            sch_mgr->dma_reserved_memory = NULL;
        }

        if(sch_mgr->dma_heap)
        {
            heap_destroy(sch_mgr->dma_heap);
            gf_free(sch_mgr->dma_heap);
            sch_mgr->dma_heap = NULL;
        }

        if(sch_mgr->private_data)
        {
            sch_mgr->chip_func->destroy(sch_mgr);
        }

        gf_destroy_mutex(sch_mgr->engine_lock);

        if (sch_mgr->task_id_lock)
        {
            gf_destroy_mutex(sch_mgr->task_id_lock);
        }

        gf_free(sch_mgr);
        adapter->sch_mgr[i] = NULL;
    }

    vidschi_destroy_fence_buffer(adapter, adapter->fence_buf);


    gf_destroy_rwsema(schedule->rw_lock);

    gf_destroy_spinlock(schedule->dvfs_status_lock);
    gf_destroy_spinlock(schedule->power_status_lock);

    gf_free(schedule);

    adapter->schedule = NULL;

    return S_OK;
}

unsigned long long vidsch_get_current_returned_fence(adapter_t *adapter, int engine)
{
    vidsch_mgr_t *vidsch = adapter->sch_mgr[engine];

    return vidsch->returned_fence_id;
}

int vidsch_save(adapter_t *adapter)
{
    vidsch_mgr_t *vidsch = NULL;
    int i = 0;
    int result = 0;
    vidsch_wait_chip_idle(adapter, ALL_ENGINE_MASK);

    for(i = 0; i < adapter->active_engine_count; i++)
    {
        vidsch = adapter->sch_mgr[i];

        if(vidsch == NULL)  continue;

        result = vidsch->chip_func->save(vidsch);
        if (result)
        {
             return result;
        }
    }
    return result;
}
#ifdef _DEBUG_ 
extern void vdisch_info_dump_interrupt(adapter_t   * adapter);
#endif

void vidsch_restore(adapter_t *adapter)
{
    vidsch_mgr_t *vidsch = NULL;
    int i = 0;

    for(i = 0; i < adapter->active_engine_count; i++)
    {
        vidsch = adapter->sch_mgr[i];

        if(vidsch == NULL)  continue;

        vidsch->chip_func->restore(vidsch, TRUE);

        vidsch->init_submit = TRUE;

    }

#ifdef _DEBUG_   
    vdisch_info_dump_interrupt(adapter);
#endif

}

static vidsch_fence_buffer_t *vidschi_create_fence_buffer(adapter_t *adapter, unsigned int segment_id, int buffer_size)
{
    vidsch_fence_buffer_t   *fence_buf       = gf_calloc(sizeof(vidsch_fence_buffer_t));
    vidmm_segment_memory_t  *reserved_memory = NULL;
    vidmm_map_flags_t        map_flags       = {0};
    unsigned short           total_num       = 0;

    reserved_memory = vidmm_allocate_segment_memory(adapter, segment_id, buffer_size, 0);

    // issue 14961 exists if fence buf is put into 3G ~ 3G+32M
    if (0xc0000000 <= reserved_memory->gpu_virt_addr && reserved_memory->gpu_virt_addr < 0xc2000000)
    {
        vidmm_release_segment_memory(adapter, reserved_memory);
        reserved_memory = vidmm_allocate_segment_memory(adapter, segment_id, buffer_size, 1);
        if (0xc0000000 <= reserved_memory->gpu_virt_addr && reserved_memory->gpu_virt_addr < 0xc2000000)
            gf_error("issue 14961 exists if fence buf is put into 3G ~ 3G+32M!\n");
    }

    /* map fence buffer */
    map_flags.read_only  = 1;
    map_flags.cache_type = GF_MEM_UNCACHED;
    map_flags.mem_space  = GF_MEM_KERNEL;

    vidmm_map_segment_memory(adapter, NULL, reserved_memory, &map_flags);

    /* per fence use a 64-bit slot */
    total_num = reserved_memory->list_node->aligned_size / sizeof(unsigned long long);

    //gf_info("create fence buffer total num %d \n", total_num);
    //gf_info("fence buffer segment_id:%d, gpu_virt_addr:%llx\n", segment_id, reserved_memory->gpu_virt_addr);

    fence_buf->reserved_memory = reserved_memory;

    fence_buf->bitmap_lock = gf_create_spinlock(0);
    fence_buf->bitmap_size = total_num >> 3;
    fence_buf->bitmap      = gf_calloc(fence_buf->bitmap_size);
    fence_buf->total_num   = 
    fence_buf->left_num    = total_num;
    fence_buf->free_start  = 0;
 
    return fence_buf;
}

static void vidschi_destroy_fence_buffer(adapter_t *adapter, vidsch_fence_buffer_t *fence_buf)
{
    vidmm_segment_memory_t *reserved_memory = fence_buf->reserved_memory;

    vidmm_unmap_segment_memory(adapter, reserved_memory, GF_MEM_KERNEL);

    vidmm_release_segment_memory(adapter, reserved_memory);

    gf_destroy_spinlock(fence_buf->bitmap_lock);

    gf_free(fence_buf->bitmap);

    gf_free(fence_buf);
}

int vidsch_request_fence_space(adapter_t *adapter, unsigned long long **virt_addr, unsigned long long *gpu_addr)
{
    vidsch_fence_buffer_t   *fence_buf       = adapter->fence_buf;
    vidmm_segment_memory_t  *reserved_memory = fence_buf->reserved_memory;
    int                      num             = 0;
    int                      status          = E_FAIL;

    if(fence_buf->left_num == 0)
    {
        gf_error("fence_buf no enough space.\n");
    }

    gf_spin_lock(fence_buf->bitmap_lock);

    num = gf_find_next_zero_bit(fence_buf->bitmap, fence_buf->total_num, fence_buf->free_start);

    if(num < fence_buf->total_num)
    {
        gf_set_bit(num, fence_buf->bitmap);

        fence_buf->left_num--;
        fence_buf->free_start = (num + 1) % fence_buf->total_num;

        *gpu_addr   = reserved_memory->gpu_virt_addr + (num << 3);
        *virt_addr  = (unsigned long long*)reserved_memory->vma->virt_addr + num;
        **virt_addr = 0ll;

        status = S_OK;
    }

    gf_spin_unlock(fence_buf->bitmap_lock);

    return status;
}

int vidsch_release_fence_space(adapter_t *adapter, unsigned long long gpu_addr)
{
    vidsch_fence_buffer_t   *fence_buf       = adapter->fence_buf;
    vidmm_segment_memory_t  *reserved_memory = fence_buf->reserved_memory;

    int num = (gpu_addr - reserved_memory->gpu_virt_addr) >> 3;

    gf_spin_lock(fence_buf->bitmap_lock);

    fence_buf->left_num++;

    gf_clear_bit(num, fence_buf->bitmap);

    gf_spin_unlock(fence_buf->bitmap_lock);

    return S_OK;
}

int vidschi_can_prepare_task_dma(vidsch_mgr_t *sch_mgr, task_dma_t *task)
{
    unsigned long flags;

    flags = gf_spin_lock_irqsave(sch_mgr->fence_lock);

    if((sch_mgr->uncomplete_task_num + sch_mgr->prepare_task_num) < sch_mgr->hw_queue_size)
    {
        sch_mgr->prepare_task_num++;
        task->prepare_submit = TRUE;
    }
    else
    {
        task->prepare_submit = FALSE;
    }

    gf_spin_unlock_irqrestore(sch_mgr->fence_lock, flags);

    return task->prepare_submit;
}

unsigned long long vidschi_inc_send_fence_id(vidsch_mgr_t *sch_mgr, int dec_prepare_num)
{
    unsigned long long fence_id;
    unsigned long      flags;

    int can_prepare = FALSE;

    flags = gf_spin_lock_irqsave(sch_mgr->fence_lock);

    sch_mgr->last_send_fence_id++;

    sch_mgr->uncomplete_task_num = sch_mgr->last_send_fence_id - sch_mgr->returned_fence_id;

    //Arise HW Limitation on VPP Engine:
    //when fence/blc/qdump followed word hit blc/qdump/fence command type, wrong FSM might be triggered.
    //the value of fence should be limitated are (include high & low part):
    //0xcxxx,xxxx
    if(sch_mgr->engine_index == 9/*RB_INDEX_VPP*/)
    {
        if((sch_mgr->last_send_fence_id & 0xF0000000) == 0xc0000000)
        {
            sch_mgr->last_send_fence_id += 0x10000000;
        }

        if((sch_mgr->last_send_fence_id & 0xF000000000000000) == 0xc000000000000000)
        {
            sch_mgr->last_send_fence_id += 0x1000000000000000;
        }
    }

    fence_id = sch_mgr->last_send_fence_id;

    if(dec_prepare_num)
    {
        sch_mgr->prepare_task_num--;
    }

    gf_spin_unlock_irqrestore(sch_mgr->fence_lock, flags);

    return fence_id;
}

/* update fence id will returned latest returned fence id */

static int vidschi_update_fence_id(vidsch_mgr_t *sch_mgr, unsigned long long *curr_returned)
{
    unsigned long long new_fence, old_fence;
    unsigned long      flags;

    int  update_new = FALSE;

    flags = gf_spin_lock_irqsave(sch_mgr->fence_lock);

    new_fence = sch_mgr->chip_func->update_fence_id(sch_mgr);

    old_fence = sch_mgr->returned_fence_id;

    /* check if new_returned valid, if not skipped it */
    if(((new_fence >  sch_mgr->returned_fence_id) &&
       (new_fence <= sch_mgr->last_send_fence_id)) &&
       (old_fence <= sch_mgr->last_send_fence_id))
    {
        sch_mgr->returned_fence_id   = new_fence;
        sch_mgr->uncomplete_task_num = sch_mgr->last_send_fence_id - sch_mgr->returned_fence_id;

        update_new = TRUE;
    }
    else
    {
        new_fence = sch_mgr->returned_fence_id;
    }

    gf_spin_unlock_irqrestore(sch_mgr->fence_lock, flags);

    if(new_fence > old_fence)
    {
        gf_get_nsecs(&sch_mgr->returned_timestamp);
    }

    *curr_returned = new_fence;

    return update_new;
}


static void vidschi_notify_vsync_interrupt(adapter_t *adapter, unsigned int engine_mask, unsigned crtc_interrupt)
{
    vidschedule_t       *schedule = adapter->schedule;

    vidsch_mgr_t        *sch_mgr  = NULL;
    unsigned int        engine    = 0;
    unsigned int        i         = 0;
    

    for(engine = 0; engine < adapter->active_engine_count; engine++)
    {
        if((1 << engine) & engine_mask)
        {
            sch_mgr = adapter->sch_mgr[engine];

            if(sch_mgr == NULL) continue;

            /* task flip need wait vsync to submit, so wakeup worker_thread */
            if((sch_mgr->emergency_task_pool.task_num > 0) || 
               (sch_mgr->normal_task_pool.task_num > 0))
            {
                util_wakeup_event_thread(sch_mgr->worker_thread);
            }
        }
    }
}

void vidschi_notify_fence_interrupt(adapter_t *adapter, unsigned int engine_mask)
{
    vidschedule_t *schedule = adapter->schedule;
    vidsch_mgr_t  *sch_mgr  = NULL;
    unsigned int  engine    = 0;
    gf_perf_event_t perf_event = {0};
    unsigned long long returned_fence_id = 0ll;
    unsigned long long timestamp;

    int new_fence = FALSE;
    int i = 0;

    if (adapter->ctl_flags.perf_event_enable)
    {
        gf_get_nsecs(&timestamp);
        perf_event.header.timestamp_high = timestamp >> 32;
        perf_event.header.timestamp_low = (timestamp) & 0xffffffff;

        perf_event.header.size = sizeof(gf_perf_event_dma_buffer_completed_t);
        perf_event.header.type = GF_PERF_EVENT_DMA_BUFFER_COMPLETED;
    }

    for(engine = 0; engine < adapter->active_engine_count; engine++)
    {
        if((1 << engine) & engine_mask)
        {
            sch_mgr = adapter->sch_mgr[engine];

            if(sch_mgr == NULL) continue;

            new_fence = vidschi_update_fence_id(sch_mgr, &returned_fence_id);

            if(new_fence)
            {
                static int fence_counter = 0;
                gf_fence_back_trace_event(sch_mgr->engine_index, returned_fence_id);
                gf_counter_trace_event("fence interrupt", fence_counter = !fence_counter);
            }

            if (adapter->ctl_flags.perf_event_enable && new_fence)
            {
                perf_event.dma_buffer_completed.engine_idx = engine;
                perf_event.dma_buffer_completed.fence_id_low = returned_fence_id & 0xffffffff;
                perf_event.dma_buffer_completed.fence_id_high = (returned_fence_id >> 32) & 0xffffffff;
                perf_event_add_isr_event(adapter, &perf_event);
            }

            gf_wake_up_event(sch_mgr->fence_event);
            if (gf_atomic_read(sch_mgr->total_task_num) > 0)
            {
                util_wakeup_event_thread(sch_mgr->worker_thread);
            }
        }
    }


    if (adapter->post_sync_event) 
    {
        gf_get_nsecs(&timestamp);
        adapter->post_sync_event(adapter->post_sync_event_argu,0,timestamp);
    }

    vidschi_notify_dma_fence(adapter);
}

void vidschi_notify_pwr_interrupt(adapter_t *adapter, unsigned int interrupt_event)
{

}

int vidsch_notify_interrupt(adapter_t *adapter, unsigned int interrupt_event)
{
    vidschedule_t  *schedule = adapter->schedule;

    if(schedule->disable_schedule)
    {
       return S_OK;
    }

    vidschi_notify_fence_interrupt(adapter, ALL_ENGINE_MASK);

    //do cg and dfs when fence back.
    if(!adapter->hw_caps.video_only && 
        !adapter->hw_caps.gfx_only )
    {
        if(adapter->hw_caps.dfs_enable)
            vidsch_try_dvfs_tuning(adapter);

        vidschi_try_power_tuning(adapter, TRUE);
    }

    return S_OK;
}

int vidsch_allocation_in_hw_queue(adapter_t *adapter, unsigned int engine_mask, vidmm_allocation_t *allocation)
{
    int result = FALSE;
    unsigned char i = 0;

    for(i = 0; i < adapter->active_engine_count; i++)
    {
        if((engine_mask & (0x1 <<i)) && (adapter->sch_mgr[i] != NULL))
        {
            if(!vidsch_is_fence_back(adapter, i, allocation->fence_id[i]))
            {
                result = TRUE;
                break;
            }
        }
    }

    return result;
}


int vidsch_is_allocation_idle(adapter_t *adapter, unsigned int engine_mask, vidmm_allocation_t *allocation)
{
    int result = TRUE;
    int i = 0;

    if (adapter->hw_caps.video_only)
    {
        engine_mask &= ((1 << 7) | (1 << 8) | (1 << 9));
    }

    for(i = 0; i < adapter->active_engine_count; i++)
    {
        if((engine_mask & (0x1 <<i)) && (adapter->sch_mgr[i] != NULL))
        {
            if( vidschi_allocation_in_cmd_queue(i, allocation) ||
              (!vidschi_allocation_in_cmd_queue(i, allocation) && !vidsch_is_fence_back(adapter, i, allocation->fence_id[i])))
            {
                result = FALSE;
                break;
            }
        }
    }

    return result;
}

/*
 * why add wait_allocation idle in vidsch module, because only vidsch can know the allocation idle status.
 */

void vidsch_wait_allocation_idle(adapter_t *adapter, unsigned int engine_mask, int read_only, vidmm_allocation_t *allocation)
{
    unsigned long long current_fence = 0x0ll;
    unsigned long long fence_id      = 0x0ll;
    unsigned char      engine_index  = 0;

    if (adapter->hw_caps.video_only)
    {
        engine_mask &= ((1 << 7)|(1 << 8)|(1 << 9));
    }

    for(engine_index = 0; engine_index < adapter->active_engine_count; engine_index++)
    {
        if(!(engine_mask & (1 << engine_index)) || (adapter->sch_mgr[engine_index] == NULL))
        {
            continue;
        }

        vidschi_wait_allocation_idle(adapter, engine_index, allocation, read_only);
    }
}

void vidsch_get_timestamp(adapter_t *adapter, unsigned long long *time_stamp)
{
    vidsch_mgr_t *sch_mgr = adapter->sch_mgr[0];
    return sch_mgr->chip_func->get_timestamp(adapter, time_stamp);
}

void vidsch_set_miu_reg(adapter_t *adapter,gf_query_info_t *info)
{
    vidsch_mgr_t *sch_mgr= adapter->sch_mgr[0];
    if(sch_mgr!=NULL)
    {
        sch_mgr->chip_func->set_miu_reg(adapter, info);

    }

}

void vidsch_read_miu_reg(adapter_t *adapter,gf_query_info_t *info)
{
    vidsch_mgr_t *sch_mgr= adapter->sch_mgr[0];
    if(sch_mgr!=NULL)
    {
        sch_mgr->chip_func->read_miu_reg(adapter, info);

    }

}

void vidsch_dump(struct os_printer *p, adapter_t *adapter)
{
    vidschedule_t *schedule = adapter->schedule;
    vidsch_mgr_t  *sch_mgr  = NULL;

    int i = 0;

    for(i; i < MAX_ENGINE_COUNT; i++)
    {
        sch_mgr = adapter->sch_mgr[i];

        if(sch_mgr == NULL) continue;

        if(sch_mgr->worker_thread == NULL) continue;

        gf_printf(p, "\n");
        gf_printf(p, "Engine: %x, last send: %lld, returned: %lld. global_task_id: %lld, last_submit_task_id: %lld\n", 
            sch_mgr->engine_index, sch_mgr->last_send_fence_id, sch_mgr->returned_fence_id, sch_mgr->task_id, sch_mgr->last_submit_task_id);

        vidschi_dump_task_pool(p, adapter, &sch_mgr->emergency_task_pool, "Emergency");

        vidschi_dump_task_pool(p, adapter, &sch_mgr->normal_task_pool, "Normal");
    }
}

int vidsch_query_hw_hang(adapter_t *adapter, gf_query_info_t *info)
{

    vidschedule_t *schedule = adapter->schedule;

    info->value = schedule->hw_hang;

    schedule->hw_hang = 0;

    //gf_info("%s() hw_hang-0x%x\n", __func__, info->value);

    return 0;
}


int vidsch_query_info(adapter_t *adapter, gf_query_info_t *info)
{
    int  ret = 0;

    switch(info->type)
    {
        case GF_QUERY_HW_HANG:
            ret = vidsch_query_hw_hang(adapter, info);
            break;
        case GF_QUERY_GET_VIDEO_BRIDGE_BUFFER:
            info->value64 =  adapter->sch_mgr[info->argu + RB_INDEX_VIDEO_START]->local_reserved_memory->gpu_virt_addr;
            break;
        case GF_QUERY_GPU_TIME_STAMP:
            vidsch_get_timestamp(adapter, &info->value64);
            break;
        case GF_SET_MIU_REGISTER_U32:
            vidsch_set_miu_reg(adapter,info);
            break;
        case GF_QUERY_MIU_REGISTER_U32:
            vidsch_read_miu_reg(adapter,info);
            break;
        default:
            gf_assert(0, "vidsch_query_info");
    }

    return ret;
}

int vidsch_cil2_misc(gpu_device_t *device, krnl_cil2_misc_t *misc)
{
    adapter_t       *adapter = device->adapter;
    vidsch_mgr_t    *sch_mgr = adapter->sch_mgr[0];

    if (misc->context)
    {
        sch_mgr = adapter->sch_mgr[misc->context->engine_index];
    }

    return sch_mgr->chip_func->cil2_misc(sch_mgr, misc);
}

int vidsch_dump_info(struct os_seq_file *seq_file, adapter_t *adapter)
{
    vidschedule_t *schedule = adapter->schedule;

    gf_seq_printf(seq_file, "dvfs_auto =%d, dvfs_force=%d\n", adapter->pm_caps.dvfs_auto, adapter->pm_caps.dvfs_force);
    if (schedule->chip_func->dump_info)
    {
        schedule->chip_func->dump_info(seq_file, adapter);
    }

    return 0;
}

int vidsch_dump_debugbus(struct os_printer *p, adapter_t *adapter)
{
    vidschedule_t *schedule = adapter->schedule;

    if (schedule->chip_func->dump_debugbus)
    {
        schedule->chip_func->dump_debugbus(p, adapter);
    }
    return 0;
}


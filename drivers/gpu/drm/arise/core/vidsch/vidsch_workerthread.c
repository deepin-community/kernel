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
#include "vidsch.h"
#include "vidschi.h"
#include "vidsch_workerthread.h"
#include "vidsch_sync.h"
#include "vidsch_submit.h"
#include "context.h"
#include "perfevent.h"

void vidschi_dump_task_pool(struct os_printer *p, adapter_t *adapter, pending_task_pool_t *pool, const char *log_str)
{
    task_desc_t   *task      = NULL;
    gpu_context_t *context   = NULL;

    pending_task_pool_t dump_pool;

    int idx = 0;

    vidschi_init_task_pool(&dump_pool, FALSE);

    vidschi_splice_task_pool(&dump_pool, pool);

    if(dump_pool.task_num <= 0)   return;

    gf_printf(p, "********* Begin Dump %s, task_num: %d.\n", log_str, dump_pool.task_num);

    list_for_each_entry(task, &dump_pool.task_list, schedule_node)
    {
        vidschi_dump_task(p, adapter, task, idx++, FALSE);
    }

    vidschi_splice_task_pool(pool, &dump_pool);

    vidschi_fini_task_pool(&dump_pool);
}

int vidschi_try_submit_pending_task(vidsch_mgr_t *sch_mgr, pending_task_pool_t *pool)
{
    adapter_t     *adapter   = sch_mgr->adapter;
    vidschedule_t *schedule  = adapter->schedule;
    task_desc_t   *task      = NULL;
    gpu_context_t *context   = NULL;

    pending_task_pool_t schedule_pool;
    pending_task_pool_t unready_pool;

    unsigned long long task_id   = 0ll;
    unsigned long long timestamp = 0ll;
    unsigned long long global_task_id = 0ll;
    int submited_count = 0;

    int submitted = FALSE, signaled = FALSE;

    vidschi_init_task_pool(&schedule_pool, TRUE);
    vidschi_init_task_pool(&unready_pool, FALSE);

    vidschi_splice_task_pool(&schedule_pool, pool);
    submited_count = schedule_pool.task_num;

    while(!list_empty(&schedule_pool.task_list))
    {
        task = vidschi_fetch_first_task_from_pool(&schedule_pool);

        if(!vidschi_can_submit_task(task) ||
            (sch_mgr->task_id_lock && task->global_task_id != sch_mgr->last_submit_task_id + 1))
        {
            vidschi_add_task_to_pool(&unready_pool, task);
            continue;
        }

        context = task->context;
        task_id = task->context_task_id;
        global_task_id = task->global_task_id;
        submitted = FALSE;

        switch (task->type)
        {
        case task_type_sync_wait:

            gf_get_nsecs(&timestamp);

            if(vidschi_try_wait(task->task_wait, FALSE) == S_OK)
            {
                submitted = TRUE;
            }
            else if(vidschi_is_server_wait_timeout(task->task_wait, timestamp))
            {
                /* if timeout, treat it as signaled.*/
                submitted = TRUE;
            }

            if(submitted)
            {
                context->last_submit_to_sw = task_id;
                if(sch_mgr->task_id_lock)
                {
                    gf_assert(sch_mgr->last_submit_task_id == global_task_id - 1, "wait sch_mgr->last_submit_task_id == global_task_id - 1");
                    sch_mgr->last_submit_task_id = global_task_id;
                }
                gf_task_submit_trace_event(context->engine_index, context->handle, task_id, task->type, 0, GF_SYNC_OBJ_WAIT_ON_SERVER);
                vidschi_release_wait_task(task->task_wait);
            }

            if ((adapter->ctl_flags.perf_event_enable)&&(submitted == TRUE))
            {
                gf_perf_event_t   perf_event;

                gf_memset(&perf_event, 0, sizeof(gf_perf_event_t));
                gf_get_nsecs(&timestamp);

                perf_event.header.timestamp_high = timestamp >> 32;
                perf_event.header.timestamp_low  = (timestamp) & 0xffffffff;

                perf_event.header.size                       = sizeof(gf_perf_event_wait_on_server_finish_t);
                perf_event.header.type                       = GF_PERF_EVENT_WAIT_ON_SERVER_FINISH;
                perf_event.wait_on_server_finish.engine_idx  = context->engine_index;
                perf_event.wait_on_server_finish.gpu_context = context->handle;
                perf_event.wait_on_server_finish.task_id_high= task_id >> 32;
                perf_event.wait_on_server_finish.task_id_low = task_id & 0xffffffff;;

                perf_event_add_event(adapter, &perf_event);
             }

            break;

        case task_type_sync_signal:
 
            if(vidschi_try_signal(task->task_signal) == S_OK)
            {
                signaled  = TRUE;
                submitted = TRUE;

                context->last_submit_to_sw = task_id;
                if(sch_mgr->task_id_lock)
                {
                    gf_assert(sch_mgr->last_submit_task_id == global_task_id - 1, "signal sch_mgr->last_submit_task_id == global_task_id - 1");
                    sch_mgr->last_submit_task_id = global_task_id;
                }

                gf_task_submit_trace_event(context->engine_index, context->handle, task_id, task->type, 0, 0);

                gf_free(task);
            }
            break;

        case task_type_render:

            if (vidschi_can_prepare_in_worker_thread(sch_mgr, task->task_render))
            {
                vidschi_prepare_and_submit_dma(sch_mgr, task->task_render);

                context->last_submit_to_sw = task_id;

                if (sch_mgr->task_id_lock)
                {
                    gf_assert(sch_mgr->last_submit_task_id == global_task_id - 1, "render sch_mgr->last_submit_task_id == global_task_id - 1");
                    sch_mgr->last_submit_task_id = global_task_id;
                }

                submitted = TRUE;
            }

            break;

        default:
            submitted = TRUE;
            gf_error("unkonw task type: %d.\n", task->type);
            break;
        }

        if(submitted)
        {
            if(pool == &sch_mgr->normal_task_pool)
            {
                gf_up(sch_mgr->normal_pool_sema);
                if(context->context_ctrl)
                {
                    gf_up(context->context_sema);
                }
                
            }
            else if(pool == &sch_mgr->emergency_task_pool)
            {
                gf_up(sch_mgr->emergency_pool_sema);
                if(context->context_ctrl)
                {
                    gf_up(context->context_sema);

                }
                
            }
        }
        else
        {
            vidschi_add_task_to_pool(&unready_pool, task);
        }
    }

    /* check if still have task unready, merge it to sch_mgr pending_queue */
    if(unready_pool.task_num > 0)
    {
        submited_count -= unready_pool.task_num;
        vidschi_splice_task_pool(pool, &unready_pool);
    }

#if 0
    if(signaled)
    {
        /* try wakeup all thread for none fence syncobj*/
    }
#endif

    vidschi_fini_task_pool(&schedule_pool);
    vidschi_fini_task_pool(&unready_pool);

    gf_atomic_sub(sch_mgr->total_task_num, submited_count);
    return S_OK;
}


/* 
** worker thread will be wakeup in three place:
** 1 main thread when add task to task queue
** 2 fence interrupt handler, trigger flip queue
** 3 v-sync interrupt handler, trigger task queue and flip queue 
*/
int vidsch_worker_thread_event_handler(void *data, gf_event_status_t ret)
{
    vidsch_mgr_t        *sch_mgr   = data;
    adapter_t           *adapter   = sch_mgr->adapter;
    pending_task_pool_t *emergency = &sch_mgr->emergency_task_pool;
    pending_task_pool_t *normal    = &sch_mgr->normal_task_pool;

    int try_freeze_num = 0;

try_again:

    if(emergency->task_num > 0)
    {
        vidschi_try_submit_pending_task(sch_mgr, emergency);
    }

    if(normal->task_num > 0)
    {
        vidschi_try_submit_pending_task(sch_mgr, normal);
    }

    sch_mgr->worker_thread->can_freeze = FALSE;

    if(gf_freezing())
    {
        /* if need freezing, we should cleanup task in thread firstly */
        if(!vidschi_worker_thread_idle(sch_mgr))
        {
            gf_info("*** ENGINE[%d] wait render finished: emergency: %d. normal: %dm try_freeze_num: %d\n", 
                sch_mgr->engine_index, emergency->task_num, normal->task_num, try_freeze_num);

            gf_msleep(10); //relase CPU wait 10ms, and try_again

            try_freeze_num++;

            if(try_freeze_num > 100)
            {
                gf_info("*** render thread [%d] try cleanup queue: %d.\n", sch_mgr->engine_index, try_freeze_num);

                vidschi_dump_hang_info(adapter, 1<<sch_mgr->engine_index);
            }

            goto try_again;
        }
        else
        {
            sch_mgr->worker_thread->can_freeze = TRUE;

            gf_info("sleep render thread %d.\n", sch_mgr->engine_index);
        }
    }

    return 0;
}


/* when call this function, main thread will block itself to add task to queue */
/* deprecated func, need delete later */
void vidschi_wait_worker_thread_idle(vidsch_mgr_t *sch_mgr)
{
    while (!vidschi_worker_thread_idle(sch_mgr))
    {
        gf_msleep(1);
    }
}

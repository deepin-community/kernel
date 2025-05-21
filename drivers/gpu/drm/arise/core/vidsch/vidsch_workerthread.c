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
#include "vidsch.h"
#include "vidschi.h"
#include "vidsch_workerthread.h"
#include "vidsch_sync.h"
#include "vidsch_submit.h"
#include "perfevent.h"

void vidschi_dump_task_pool(struct os_printer *p, adapter_t *adapter, pending_task_pool_t *pool, const char *log_str)
{
    task_desc_t   *task      = NULL;

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

static int vidschi_try_submit_pending_task(vidsch_mgr_t *sch_mgr, pending_task_pool_t *pool)
{
    adapter_t     *adapter   = sch_mgr->adapter;
    task_desc_t   *task      = NULL;
    gpu_context_t *context   = NULL;

    pending_task_pool_t schedule_pool;
    pending_task_pool_t unready_pool;

    unsigned long long task_id   = 0ll;
    unsigned long long timestamp = 0ll;
    unsigned long long global_task_id = 0ll;
    int submited_count = 0;

    int submitted = FALSE;

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
                gf_task_submit_trace_event(adapter->index << 16 | context->engine_index, context->handle, task_id, task->type, 0, GF_SYNC_OBJ_WAIT_ON_SERVER);
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
                submitted = TRUE;

                context->last_submit_to_sw = task_id;
                if(sch_mgr->task_id_lock)
                {
                    gf_assert(sch_mgr->last_submit_task_id == global_task_id - 1, "signal sch_mgr->last_submit_task_id == global_task_id - 1");
                    sch_mgr->last_submit_task_id = global_task_id;
                }

                gf_task_submit_trace_event(adapter->index << 16 | context->engine_index, context->handle, task_id, task->type, 0, 0);

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
    pending_task_pool_t *normal    = &sch_mgr->normal_task_pool;

    int try_freeze_num = 0;

try_again:

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
            gf_info("*** ENGINE[%d] wait render finished: normal: %dm try_freeze_num: %d\n",
                sch_mgr->engine_index, normal->task_num, try_freeze_num);

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

            gf_debug("sleep render thread %d.\n", sch_mgr->engine_index);
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

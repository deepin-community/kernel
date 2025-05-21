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
#include "vidsch_submit.h"

static int vidschi_reset_hw(adapter_t *adapter, unsigned int hang_engines)
{
    vidsch_mgr_t  *sch_mgr = adapter->sch_mgr[util_get_lsb_position(hang_engines)];
    unsigned int  i;
    unsigned int  ret = S_OK;

    gf_disable_interrupt(adapter->os_device.pdev);

    ret = sch_mgr->chip_func->reset_hw(sch_mgr);

    if (ret == S_OK)
    {
        for (i = 0; i < MAX_ENGINE_COUNT; i++)
        {
            sch_mgr = adapter->sch_mgr[i];

            if(sch_mgr == NULL) continue;

            vidschi_set_uncompleted_task_dropped(sch_mgr, -1ll);

            vidsch_release_completed_tasks(adapter, i, NULL);
        }
    }

    //gf_info("reset finished.\n");

    if(sch_mgr != NULL)
    {
        sch_mgr->init_submit = TRUE;
    }

    gf_enable_interrupt(adapter->os_device.pdev);

    return S_OK;
}

void vidschi_dump_hang_info(adapter_t *adapter, unsigned int hang_engines)
{
    vidsch_mgr_t *sch_mgr = adapter->sch_mgr[util_get_lsb_position(hang_engines)];

    if(sch_mgr != NULL && sch_mgr->chip_func->dump_hang_info != NULL)
    {
        sch_mgr->chip_func->dump_hang_info(sch_mgr);
    }
}

static void vidschi_dump_hang(adapter_t *adapter, unsigned int hang_engines)
{
    vidsch_mgr_t *sch_mgr = adapter->sch_mgr[util_get_lsb_position(hang_engines)];

    if(sch_mgr != NULL && sch_mgr->chip_func->dump_hang != NULL)
    {
        sch_mgr->chip_func->dump_hang(adapter);
    }
}

static int vidschi_check_hang_and_recovery(adapter_t *adapter)
{
    vidsch_mgr_t  *sch_mgr  = NULL;
    vidschedule_t *schedule = adapter->schedule;
    task_desc_t   *hang_tasks[MAX_ENGINE_COUNT];

    unsigned int i, hang_engines = 0;

    for (i = 0; i < MAX_ENGINE_COUNT; i++)
    {
        sch_mgr       = adapter->sch_mgr[i];
        hang_tasks[i] = NULL;

        if(sch_mgr == NULL) continue;

        hang_tasks[i] = vidschi_uncompleted_task_exceed_wait_time(sch_mgr, adapter->hw_hang_max_timeout_ns);

        if(hang_tasks[i] != NULL)
        {
            gf_info("checked timeout type:%d, fence_id:%d", hang_tasks[i]->type, hang_tasks[i]->fence_id);
            vidschi_dump_task(NULL,adapter, hang_tasks[i], 0, FALSE);
            if(adapter->ctl_flags.hang_dump == 2 && hang_tasks[i]->type == task_type_paging)
            {
                gf_error("vidschi_check_hang_and_recovery not support paging task dump\n");
            }

            vidsch_task_dec_reference(adapter, sch_mgr->engine_index, hang_tasks[i]);
            sch_mgr->hang_index = hang_tasks[i]->hang_index;
            sch_mgr->hang_fence_id = hang_tasks[i]->fence_id;

            hang_tasks[i] = NULL;
            hang_engines |= 1 << i;
        }
    }

    vidschi_try_power_tuning(adapter, FALSE);


    schedule->hw_hang |= hang_engines;

    if (hang_engines != 0)
    {
        for (i=0; i<MAX_ENGINE_COUNT; i++)
        {
            if(hang_engines & (1 << i))
            {
                sch_mgr = adapter->sch_mgr[i];
                break;
            }
        }
        schedule->disable_schedule = TRUE;

        //vidmm_dump_resource(sch_mgr->adapter);

        //gf_info("vidsch_reset_engines, need reset hw, hang_engines:0x%x \n", hang_engines);
        gf_info("timeout engine index %x, returned fence id %lld, send fence id %lld\n",
            i, sch_mgr->returned_fence_id, sch_mgr->last_send_fence_id);
        vidschi_dump_hang_info(adapter, hang_engines);
#if 1
        if(adapter->ctl_flags.recovery_enable == TRUE)
        {
            int status = 0;
            //gf_info("before hw rest\n");
            gf_console_lock(1);
            status = vidschi_reset_hw(adapter, hang_engines);
            gf_console_lock(0);

            if(status == S_OK)
            {
                schedule->hw_hang = 0;
                if(adapter->ctl_flags.hang_dump == 2)
                {
                    vidschi_dump_hang(adapter, hang_engines);
                }
            }

            gf_msleep(10);

            schedule->disable_schedule = FALSE;

            vidsch_notify_interrupt(adapter, 0);
        }
        else
#endif
        {
             gf_msleep(100000);//ensure write to file done
             gf_info("trigger assert, hang detected but recovery not enabled, stop kernel here to avoid endless hang.\n");
             gf_assert(0, "trigger assert, hang detected");

        }
    }
    return 0;
}

static int vidsch_daemon_engine_status(void *data, gf_event_status_t ret)
{
    vidschedule_t *schedule = data;
    adapter_t *adapter = schedule->adapter;
    unsigned long long timestamp;

    gf_get_nsecs(&timestamp);
    gf_core_interface->hwq_process_vsync_event(adapter, timestamp);
    schedule->chip_func->boost(adapter, ENG_POWER_STATE_AUTO, ENG_POWER_MIN_HOLDING_TIME_MS, FALSE);

    return 0;
}

static int vidsch_daemon_delay_allocation(void *data, gf_event_status_t ret)
{
    vidschedule_t *schedule = data;
    adapter_t     *adapter  = schedule->adapter;

    vidmm_destroy_defer_allocation(adapter);
    return 0;
}

static int vidsch_daemon_check_hang(void *data, gf_event_status_t ret)
{
    vidschedule_t *schedule = data;
    adapter_t     *adapter  = schedule->adapter;
    unsigned long long timestamp;

 //   vidmm_destroy_defer_allocation(adapter);
#if 0
    //vidschi_try_power_tuning(adapter);

    if(!adapter->hw_caps.video_only &&
        !adapter->hw_caps.gfx_only &&
        adapter->hw_caps.dfs_enable)
    {
        vidsch_try_dvfs_tuning(adapter);
    }
#endif
    gf_get_nsecs(&timestamp);
    gf_core_interface->hwq_process_vsync_event(adapter, timestamp);

    vidschi_check_hang_and_recovery(adapter);

    return 0;
}

/* init schedule daemon thread, mainly contains power gating.
 * hw reset function also be called in this daemon thread */
int vidschi_init_daemon_thread(adapter_t *adapter)
{
    vidschedule_t *schedule = adapter->schedule;
    schedule->daemon_thread_check_hang =
            util_create_event_thread(vidsch_daemon_check_hang, schedule, "daemon_check_hang", DAEMON_THREAD_INTERVAL);
    schedule->daemon_thread_destory_allocation =
            util_create_event_thread(vidsch_daemon_delay_allocation, schedule, "daemon_delay_allocation", DAEMON_THREAD_INTERVAL);
    schedule->daemon_thread_power_control =
            util_create_event_thread(vidsch_daemon_engine_status, schedule, "daemon_engine_status", DAEMON_THREAD_INTERVAL / 5);

    return 0;
}

int vidschi_deinit_daemon_thread(adapter_t *adapter)
{
    vidschedule_t *schedule = adapter->schedule;

    if (schedule->daemon_thread_check_hang)
    {
        util_destroy_event_thread(schedule->daemon_thread_check_hang);
    }

    if (schedule->daemon_thread_destory_allocation)
    {
        util_destroy_event_thread(schedule->daemon_thread_destory_allocation);
    }

    if (schedule->daemon_thread_power_control)
    {
        util_destroy_event_thread(schedule->daemon_thread_power_control);
    }

    return 0;
}


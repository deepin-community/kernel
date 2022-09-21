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
#include "vidsch_submit.h"

int vidschi_reset_hw(adapter_t *adapter, unsigned int hang_engines)
{
    vidsch_mgr_t  *sch_mgr = adapter->sch_mgr[util_get_lsb_position(hang_engines)];
    unsigned int  i;

    gf_disable_interrupt(adapter->os_device.pdev);

    sch_mgr->chip_func->reset_hw(sch_mgr);

    for (i = 0; i < MAX_ENGINE_COUNT; i++)
    {
        sch_mgr = adapter->sch_mgr[i];

        if(sch_mgr == NULL) continue;

        vidschi_set_uncompleted_task_dropped(sch_mgr, -1ll);

        vidsch_release_completed_tasks(adapter, i, NULL);
    }

    gf_info("reset finished.\n");

    /* TODO: present count need refine to go general sync logic, then no need here*/
    {
        gpu_device_t *device      = NULL;

        list_for_each_entry(device, &adapter->device_list, list_node)
        {
            if(device->current_present_count)
            {
                *device->current_present_count = device->last_present_count;
            }
        }
    }

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

void vidschi_dump_hang(adapter_t *adapter, unsigned int hang_engines)
{
    vidsch_mgr_t *sch_mgr = adapter->sch_mgr[util_get_lsb_position(hang_engines)];

    if(sch_mgr != NULL && sch_mgr->chip_func->dump_hang != NULL)
    {
        sch_mgr->chip_func->dump_hang(adapter);
    }
}

int vidschi_check_hang_and_recovery(adapter_t *adapter)
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

        hang_tasks[i] = vidschi_uncompleted_task_exceed_wait_time(sch_mgr, 
               adapter->ctl_flags.run_on_qt ? HW_HANG_MAX_TIMEOUT_NS_ON_QT : HW_HANG_MAX_TIMEOUT_NS);

        if(hang_tasks[i] != NULL)
        {
            gf_info("vidschi_check_hang_and_recovery type:%d, fence_id:%d", hang_tasks[i]->type, hang_tasks[i]->fence_id);
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

        //dispmgr_dump(adapter);

        //vidmm_dump_resource(sch_mgr->adapter);

        gf_info("vidsch_reset_engines, need reset hw, hang_engines:0x%x \n", hang_engines);
        gf_info("vidschi_check_hang_and_recovery engine index %x, returned fence id %lld, send fence id %lld\n",
            i, sch_mgr->returned_fence_id, sch_mgr->last_send_fence_id);
        vidschi_dump_hang_info(adapter, hang_engines);
#if 1
        if(adapter->ctl_flags.recovery_enable == TRUE)
        {
            int status = 0;
            gf_info("before hw rest\n");
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

int vidsch_daemon_delay_allocation(void *data, gf_event_status_t ret)
{
    vidschedule_t *schedule = data;
    adapter_t     *adapter  = schedule->adapter;

    vidmm_destroy_defer_allocation(adapter);
    return 0;
}
int vidsch_daemon_check_hang(void *data, gf_event_status_t ret)
{
    vidschedule_t *schedule = data;
    adapter_t     *adapter  = schedule->adapter;

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

    return 0;
}


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
#include "gf_def.h"
#include "perfeventi.h"
#include "perfevent.h"

#define   PERF_EVENT_TRACE_ENABLE     0

#if PERF_EVENT_TRACE_ENABLE
#define perf_event_trace    gf_info
#else
#define perf_event_trace(args...)
#endif

#define PERF_EVENT_MAX_ISR_EVENT_BUFFER_SIZE    (512*1024)

extern perf_chip_func_t perfevent_chip_func;

int perf_event_init(adapter_t *adapter)
{
    perf_event_mgr_t *perf_event_mgr;
    int ret = 0;
    
    perf_event_mgr = gf_calloc(sizeof(perf_event_mgr_t));
    gf_assert(perf_event_mgr != NULL, "perf_event_mgr != NULL");

    perf_event_mgr->event_list_lock = gf_create_spinlock(0);
    gf_assert(perf_event_mgr->event_list_lock != NULL, "perf_event_mgr->event_list_lock != NULL");

    perf_event_mgr->miu_event_list_lock = gf_create_spinlock(0);
    gf_assert(perf_event_mgr->miu_event_list_lock != NULL, "perf_event_mgr->miu_event_list_lock != NULL");

    list_init_head(&perf_event_mgr->event_list);
    list_init_head(&perf_event_mgr->miu_event_list);

    perf_event_mgr->isr_event_buffer = gf_calloc(PERF_EVENT_MAX_ISR_EVENT_BUFFER_SIZE);
    gf_assert(perf_event_mgr->isr_event_buffer != NULL, "perf_event_mgr->isr_event_buffer != NULL");
    perf_event_mgr->isr_event_tail = 0;
    perf_event_mgr->isr_event_head = 0;
    perf_event_mgr->isr_event_lock = gf_create_spinlock(0);

    perf_event_mgr->miu_table_lock = gf_create_spinlock(0);
    gf_assert(perf_event_mgr->miu_table_lock != NULL, "perf_event_mgr->miu_table_lock != NULL");

    perf_event_mgr->chip_func = &perfevent_chip_func;

    adapter->perf_event_mgr = perf_event_mgr;

    return ret;
}

int perf_event_deinit(adapter_t *adapter)
{
    perf_event_mgr_t *perf_event_mgr = adapter->perf_event_mgr;
    perf_event_node *event_node;
    perf_event_node *event_node_next;

    gf_assert(perf_event_mgr->perf_started == FALSE, "perf_event_mgr->perf_started == FALSE");

    list_for_each_entry_safe(event_node, event_node_next, &perf_event_mgr->event_list, list_node)
    {
        list_del(&event_node->list_node);
        gf_free(event_node);
    }

    // clean miu list.
    list_for_each_entry_safe(event_node, event_node_next, &perf_event_mgr->miu_event_list, list_node)
    {
        list_del(&event_node->list_node);
        gf_free(event_node);
    }

    gf_destroy_spinlock(perf_event_mgr->event_list_lock);
    perf_event_mgr->event_list_lock = NULL;

    gf_free(perf_event_mgr->isr_event_buffer);
    perf_event_mgr->isr_event_buffer = NULL;

    gf_destroy_spinlock(perf_event_mgr->isr_event_lock);
    perf_event_mgr->isr_event_lock = NULL;

    gf_destroy_spinlock(perf_event_mgr->miu_table_lock);
    perf_event_mgr->miu_table_lock = NULL;

    gf_destroy_spinlock(perf_event_mgr->miu_event_list_lock);
    perf_event_mgr->miu_event_list_lock = NULL;

    gf_free(perf_event_mgr);
    adapter->perf_event_mgr = NULL;

    return 0;
}

int perf_event_begin(adapter_t *adapter, gf_begin_perf_event_t *begin)
{
    perf_event_mgr_t *perf_event_mgr = adapter->perf_event_mgr;
    perf_event_node *event_node;
    perf_event_node *event_node_next;

    gf_assert(adapter->ctl_flags.perf_event_enable, "adapter->ctl_flags.perf_event_enable");
    gf_assert(perf_event_mgr->perf_started == FALSE, "perf_event_mgr->perf_started == FALSE");

    //resetting buffer
    perf_event_mgr->isr_event_head = 0;
    perf_event_mgr->isr_event_tail = 0;
    perf_event_mgr->isr_event_buffer_size = 0;

    perf_event_mgr->lost_event_number = 0;
    perf_event_mgr->event_number = 0;

    list_for_each_entry_safe(event_node, event_node_next, &perf_event_mgr->event_list, list_node)
    {
        list_del(&event_node->list_node);
        gf_free(event_node);
    }

    perf_event_mgr->perf_started = TRUE;
    perf_event_mgr->max_event_number = begin->max_event_num;

    perf_event_trace("perf_event_begin, max_event_number:%x\n", begin->max_event_num);

    return 0;
}

int perf_event_begin_miu_dump(adapter_t *adapter, gf_begin_miu_dump_perf_event_t *miu_begin)
{
   perf_event_mgr_t *perf_event_mgr = adapter->perf_event_mgr;

   perf_event_trace("perf_event_begin_miu_dump enter\n");

   gf_assert(adapter->ctl_flags.perf_event_enable == TRUE, "adapter->ctl_flags.perf_event_enable == TRUE");

   if(adapter->ctl_flags.miu_counter_enable == TRUE)
   {
       gf_info("krnl miu dump already begin, call miu begin twice?\n");
   }

   adapter->ctl_flags.miu_counter_enable = TRUE;

   perf_event_mgr->miu_max_event_number = miu_begin->max_event_num;
   perf_event_mgr->miu_event_number = 0;
   perf_event_mgr->miu_lost_event_number = 0;

   perf_event_trace("perf_event_begin_miu_dump exit\n");

   return 0;
}

int perf_event_end(adapter_t *adapter, gf_end_perf_event_t *end)
{
    perf_event_mgr_t *perf_event_mgr = adapter->perf_event_mgr;

    gf_assert(perf_event_mgr->perf_started == TRUE, "perf_event_mgr->perf_started == TRUE");
    perf_event_mgr->perf_started = FALSE;

    perf_event_trace("perf_event_end\n");

    return 0;
}

int perf_event_end_miu_dump(adapter_t *adapter, gf_end_miu_dump_perf_event_t *miu_end)
{
    perf_event_mgr_t *perf_event_mgr    = adapter->perf_event_mgr;
    perf_event_node *event_node         = NULL;
    perf_event_node *event_node_next    = NULL;

    perf_event_trace("perf_event_end_miu_dump enter\n");

    if(adapter->ctl_flags.miu_counter_enable == FALSE)
    {
        gf_info("krnl miu dump already end, call miu end twice?\n");
    }

    adapter->ctl_flags.miu_counter_enable = FALSE;

    // free miu event list.
    gf_spin_lock(perf_event_mgr->miu_event_list_lock);

    list_for_each_entry_safe(event_node, event_node_next, &perf_event_mgr->miu_event_list, list_node)
    {
        list_del(&event_node->list_node);
        gf_free(event_node);
    }

    perf_event_mgr->miu_event_number = 0;
    perf_event_mgr->miu_lost_event_number = 0;
    perf_event_mgr->miu_lost_event_time = 0;

    gf_spin_unlock(perf_event_mgr->miu_event_list_lock);

    perf_event_trace("perf_event_end_miu_dump exit\n");

    return 0;
}


int perf_event_add_miu_event(adapter_t *adapter, unsigned int gpu_context, unsigned long long task_id, unsigned long long fence_id, int task_end)
{
    perf_event_mgr_t *perf_event_mgr    = adapter->perf_event_mgr;
    gf_perf_event_t *miu_perf_event    = NULL;
    perf_event_node *event_node         = NULL;
    gf_miu_list_item_t *miu_table      = NULL;
    unsigned long long timestamp        = 0;

    unsigned int miu_table_size = 0, perf_event_size = 0;
    int ret = 0, list_full = 0;

    miu_table_size  = perf_event_mgr->miu_table_length*sizeof(gf_miu_list_item_t);
    perf_event_size = sizeof(gf_perf_event_miu_counter_dump_t) + miu_table_size;

    perf_event_trace("perf_event_add_miu_event enter\n");

    if (adapter->ctl_flags.miu_counter_enable == FALSE)
    {
        ret = -1;
        goto exit;
    }

    if (perf_event_mgr->miu_lost_event_number > 0)
    {
        perf_event_mgr->miu_lost_event_number ++;
        ret = -1;
        goto exit;
    }

    event_node = gf_calloc(perf_event_size + sizeof(struct list_head));
    if (event_node == NULL)
    {
        gf_info("perf_event_mgr: alloc memroy fail for miu node.\n");
        ret = -1;
        goto exit;
    }

    miu_perf_event = (gf_perf_event_t*)(&event_node->perf_event);

    // fill miu perf event, then copy event info to event node.
    miu_perf_event->header.size = perf_event_size;
    miu_perf_event->header.type = GF_PERF_EVENT_MIU_COUNTER;
    miu_perf_event->miu_counter.counter_buffer_offset = sizeof(gf_perf_event_miu_counter_dump_t);
    miu_perf_event->miu_counter.buffer_length = perf_event_mgr->miu_table_length;

    miu_perf_event->miu_counter.gpu_context = gpu_context;
    miu_perf_event->miu_counter.task_id_high = task_id >> 32;
    miu_perf_event->miu_counter.task_id_low = (task_id) & 0xffffffff;
    miu_perf_event->miu_counter.fence_id_high = fence_id >> 32;
    miu_perf_event->miu_counter.fence_id_low = (fence_id) & 0xffffffff;

    gf_get_nsecs(&timestamp);
    miu_perf_event->header.timestamp_high = timestamp >> 32;
    miu_perf_event->header.timestamp_low = (timestamp) & 0xffffffff;

    // miu list item array start address.
    miu_table = (gf_miu_list_item_t*)((char*)&event_node->perf_event + sizeof(gf_perf_event_miu_counter_dump_t));

    gf_spin_lock(perf_event_mgr->miu_table_lock);
    gf_memcpy(miu_table, perf_event_mgr->miu_table, miu_table_size);
    gf_spin_unlock(perf_event_mgr->miu_table_lock);

    if(miu_table == NULL)
    {
        ret = -1;
        goto exit;
    }

    // run miu command from user space.
    perf_event_mgr->chip_func->direct_get_miu_counter(adapter, miu_table, perf_event_mgr->miu_table_length, task_end);

    perf_event_trace("add event type: %x, pid %x, tid %x\n", perf_event->header.type, perf_event->header.pid, perf_event->header.tid);

    gf_spin_lock(perf_event_mgr->miu_event_list_lock);

    if (perf_event_mgr->miu_event_number < perf_event_mgr->miu_max_event_number)
    {
        list_add_tail(&event_node->list_node, &perf_event_mgr->miu_event_list);
        perf_event_mgr->miu_event_number++;
    }
    else
    {
        list_full = 1;
        perf_event_mgr->miu_lost_event_number++;
        if(perf_event_mgr->miu_lost_event_number == 1)
        {
            gf_get_nsecs(&perf_event_mgr->miu_lost_event_time);
        }

        gf_info("perf_event_mgr: miu perf event lost.\n");
    }

    gf_spin_unlock(perf_event_mgr->miu_event_list_lock);

    if(list_full == 1)
    {
        gf_free(event_node);
        ret = -1;
        goto exit;
    }

    perf_event_trace("add miu perf event num:%x\n", perf_event_mgr->miu_event_number);

    perf_event_trace("perf_event_add_miu_event exit.\n");

exit:
    return ret;

}

int perf_event_add_event(adapter_t *adapter, gf_perf_event_t *perf_event)
{
    perf_event_mgr_t *perf_event_mgr = adapter->perf_event_mgr;
    perf_event_node *event_node;
    int ret = 0;

    if (perf_event_mgr->perf_started == FALSE)
    {
        ret = -1;
        goto exit;
    }

    if (perf_event_mgr->lost_event_number > 0)
    {
        perf_event_mgr->lost_event_number++;
        ret = -1;
        goto exit;
    }

    event_node = gf_calloc(perf_event->header.size + sizeof(struct list_head));
    if (event_node == NULL)
    {
        gf_info("perf_event_mgr: alloc memory fail.\n");
        ret = -1;
        goto exit;
    }

    gf_memcpy(&event_node->perf_event, perf_event, perf_event->header.size);

    perf_event_trace("add event type: %x, pid %x, tid %x\n", perf_event->header.type, perf_event->header.pid, perf_event->header.tid);

    gf_spin_lock(perf_event_mgr->event_list_lock);

    if (perf_event_mgr->event_number < perf_event_mgr->max_event_number)
    {
        list_add_tail(&event_node->list_node, &perf_event_mgr->event_list);
        perf_event_mgr->event_number++;
    }
    else
    {
        perf_event_mgr->lost_event_number++;

        if (perf_event_mgr->lost_event_number == 1)
        {
            gf_get_nsecs(&perf_event_mgr->lost_event_time);
        }

        gf_info("perf_event_mgr: event lost\n");
    }

    gf_spin_unlock(perf_event_mgr->event_list_lock);

    if (perf_event_mgr->lost_event_number > 0)
    {
        if (event_node->list_node.next != &perf_event_mgr->event_list)
        {
            gf_free(event_node);
            ret = -1;
            goto exit;
        }
    }

    perf_event_trace("add perf event num:%x\n", perf_event_mgr->event_number);

    if (perf_event->header.type == GF_PERF_EVENT_DMA_BUFFER_QUEUED)
    {
       perf_event_trace("add event dma queue dma idx low: %x, high: %x\n", perf_event->dma_buffer_queued.dma_idx_low, perf_event->dma_buffer_queued.dma_idx_high);
       perf_event_trace("add event dma queue time low: %x, high: %x\n", perf_event->header.timestamp_low, perf_event->header.timestamp_high);
    }

exit:
    return ret;
}

int perf_event_get_miu_event(adapter_t *adapter, gf_get_miu_dump_perf_event_t *get_perf_event)
{
    perf_event_mgr_t *perf_event_mgr    = adapter->perf_event_mgr;
    perf_event_node *event_node         = NULL;
    perf_event_node *event_node_next    = NULL;
    gf_perf_event_header_t *perf_event = NULL;
    int event_fill_num                  = 0;
    char *dst_buf                       = get_perf_event->event_buffer;

    int i = 0, ret = 0;

    perf_event_trace("miu perf_event_get_event enter\n");

    if (perf_event_mgr->perf_started == FALSE)
    {
        ret = -1;
        goto exit;
    }

    gf_spin_lock(perf_event_mgr->miu_event_list_lock);

    if (perf_event_mgr->miu_event_number > 2)
    {
        event_fill_num = perf_event_mgr->miu_event_number - 1;
    }

    gf_spin_unlock(perf_event_mgr->miu_event_list_lock);

    if (event_fill_num == 0)
    {
        get_perf_event->event_filled_num = 0;
        get_perf_event->event_buffer_filled_size = 0;
        get_perf_event->event_lost_num = 0;

        goto exit;
    }

    list_for_each_entry_safe(event_node, event_node_next, &perf_event_mgr->miu_event_list, list_node)
    {
        if(get_perf_event->max_event_buffer_size < event_node->perf_event.size)
        {
            gf_info("insufficient buffer to contain miu counter data!\n");
            ret = -1;
            break;
        }

        gf_copy_to_user(dst_buf, &event_node->perf_event, event_node->perf_event.size);
        dst_buf += event_node->perf_event.size;

        perf_event = &event_node->perf_event;

        list_del(&event_node->list_node);

        gf_free(event_node);
        i++;

        if (i >= get_perf_event->max_event_num ||
            i >= event_fill_num)
        {
            break;
        }
    }

    get_perf_event->event_filled_num = i;
    get_perf_event->event_buffer_filled_size = util_get_ptr_span(dst_buf, get_perf_event->event_buffer);

    gf_spin_lock(perf_event_mgr->miu_event_list_lock);

    get_perf_event->event_lost_num = perf_event_mgr->miu_lost_event_number;
    get_perf_event->event_lost_timestamp = perf_event_mgr->miu_lost_event_time;

    perf_event_mgr->miu_event_number -= i;
    perf_event_mgr->miu_lost_event_number = 0;
    perf_event_mgr->miu_lost_event_time = 0;

    gf_spin_unlock(perf_event_mgr->miu_event_list_lock);

exit:
   return ret;

}

int perf_event_get_event(adapter_t *adapter, gf_get_perf_event_t *get_perf_event)
{
    perf_event_mgr_t *perf_event_mgr = adapter->perf_event_mgr;
    perf_event_node *event_node;
    perf_event_node *event_node_next;
    int event_fill_num = 0;
    int i = 0;
    int ret = 0;
    char *dst_buf = get_perf_event->event_buffer;
    gf_perf_event_header_t *perf_event;

    perf_event_trace("perf_event_get_event enter\n");

    if (perf_event_mgr->perf_started == FALSE)
    {
        ret = -1;
        goto exit;
    }

    gf_spin_lock(perf_event_mgr->event_list_lock);
    
    if (perf_event_mgr->event_number > 2)
    {
        event_fill_num = perf_event_mgr->event_number - 1;
    }

    gf_spin_unlock(perf_event_mgr->event_list_lock);

    if (event_fill_num == 0)
    {
        get_perf_event->event_filled_num = 0;
        get_perf_event->event_buffer_filled_size = 0;
        get_perf_event->event_lost_num = 0;

        if(perf_event_mgr->isr_event_num == 0)
        {
            goto exit;
        }
        else
        {
            goto isr_event;
        }
    }

    list_for_each_entry_safe(event_node, event_node_next, &perf_event_mgr->event_list, list_node)
    {
        gf_copy_to_user(dst_buf, &event_node->perf_event, event_node->perf_event.size);
        dst_buf += event_node->perf_event.size;

        perf_event = &event_node->perf_event;

        list_del(&event_node->list_node);

        gf_free(event_node);
        i++;

        if (i >= get_perf_event->max_event_num ||
            i >= event_fill_num)
        {
            break;
        }
    }

    get_perf_event->event_filled_num = i;
    get_perf_event->event_buffer_filled_size = dst_buf - (char *)get_perf_event->event_buffer;

    gf_spin_lock(perf_event_mgr->event_list_lock);

    get_perf_event->event_lost_num = perf_event_mgr->lost_event_number;
    get_perf_event->event_lost_timestamp = perf_event_mgr->lost_event_time;

    perf_event_mgr->event_number -= i;
    perf_event_mgr->lost_event_number = 0;
    perf_event_mgr->lost_event_time = 0;

    gf_spin_unlock(perf_event_mgr->event_list_lock);

isr_event:
    ret = perf_event_get_isr_event(adapter, get_perf_event);

    perf_event_trace("perf_event_get exit, event_filled_num: %x, isr_event_num: %x\n",
        get_perf_event->event_filled_num, get_perf_event->isr_filled_num);

exit:
   return ret;
}

int perf_event_add_isr_event(adapter_t *adapter, gf_perf_event_t *perf_event)
{
    perf_event_mgr_t *perf_event_mgr = adapter->perf_event_mgr;
    unsigned long flags = 0;
    char *buf = NULL;
    int ret = 0;
    int partial_copy_size;

    if (perf_event_mgr->perf_started == FALSE)
    {
        ret = -1;
        goto exit;
    }

    flags = gf_spin_lock_irqsave(perf_event_mgr->isr_event_lock);

    if (perf_event_mgr->isr_event_buffer_size + perf_event->header.size < PERF_EVENT_MAX_ISR_EVENT_BUFFER_SIZE)
    {
        buf = perf_event_mgr->isr_event_buffer + perf_event_mgr->isr_event_tail;
        if(buf + perf_event->header.size < perf_event_mgr->isr_event_buffer + PERF_EVENT_MAX_ISR_EVENT_BUFFER_SIZE)
        {
            gf_memcpy(buf, perf_event, perf_event->header.size);
        }
        else
        {
            partial_copy_size = perf_event_mgr->isr_event_buffer + PERF_EVENT_MAX_ISR_EVENT_BUFFER_SIZE - buf;
            gf_memcpy(buf, perf_event, partial_copy_size);

            buf = perf_event_mgr->isr_event_buffer;
            gf_memcpy(buf, (char*)perf_event + partial_copy_size, perf_event->header.size - partial_copy_size);
        }

        perf_event_mgr->isr_event_tail = 
           (perf_event_mgr->isr_event_tail + perf_event->header.size) % PERF_EVENT_MAX_ISR_EVENT_BUFFER_SIZE;
        perf_event_mgr->isr_event_buffer_size += perf_event->header.size;
        perf_event_mgr->isr_event_num++;

        perf_event_trace("add isr event, added size:%x, tail:%x, number:%x\n", perf_event_mgr->isr_event_buffer_size, 
           perf_event_mgr->isr_event_tail, perf_event_mgr->isr_event_num);
    }
    else
    {
        //gf_info("perf_event_mgr: dma completed event lost, should not happen.\n");
    }

    gf_spin_unlock_irqrestore(perf_event_mgr->isr_event_lock, flags);

    perf_event_trace("add isr event time low:%x, time high:%x, event type :%x\n", 
        perf_event->header.timestamp_low, perf_event->header.timestamp_high, perf_event->header.type);

exit:
    return ret;
}

int perf_event_get_isr_event(adapter_t *adapter, gf_get_perf_event_t *get_perf_event)
{
    perf_event_mgr_t *perf_event_mgr = adapter->perf_event_mgr;
    unsigned long flags = 0;
    char *buf = NULL;
    char *buf_dst = get_perf_event->isr_event_buffer;
    int  event_fill_number = 0;
    int  event_fill_size = 0;
    int  fill_size_partial = 0;
    int  ret = 0;

    perf_event_trace("start get_isr_event, event number:%x, size: %x\n", perf_event_mgr->isr_event_num, perf_event_mgr->isr_event_buffer_size);

    if (perf_event_mgr->perf_started == FALSE)
    {
        ret = -1;
        goto exit;
    }

    flags = gf_spin_lock_irqsave(perf_event_mgr->isr_event_lock);

    if (perf_event_mgr->isr_event_num)
    {
        event_fill_number = perf_event_mgr->isr_event_num;
        event_fill_size = perf_event_mgr->isr_event_buffer_size;
    }
    else
    {
        perf_event_mgr->isr_event_num = 0;
        perf_event_mgr->isr_event_buffer_size = 0;
        ret = 0;

        gf_spin_unlock_irqrestore(perf_event_mgr->isr_event_lock, flags);

        goto exit;
    }

    gf_spin_unlock_irqrestore(perf_event_mgr->isr_event_lock, flags);

    if (event_fill_number)
    {
        if (perf_event_mgr->isr_event_head + event_fill_size <= PERF_EVENT_MAX_ISR_EVENT_BUFFER_SIZE)
        {
            buf = perf_event_mgr->isr_event_buffer + perf_event_mgr->isr_event_head;
            gf_copy_to_user(buf_dst, buf, perf_event_mgr->isr_event_buffer_size);
        }
        else
        {
            fill_size_partial = PERF_EVENT_MAX_ISR_EVENT_BUFFER_SIZE - perf_event_mgr->isr_event_head;

            buf = perf_event_mgr->isr_event_buffer + perf_event_mgr->isr_event_head;
            gf_copy_to_user(buf_dst, buf, fill_size_partial);

            buf = perf_event_mgr->isr_event_buffer;
            buf_dst += fill_size_partial;
            gf_copy_to_user(buf_dst, buf, event_fill_size);
        }
    }

    perf_event_mgr->isr_event_head = 
        (perf_event_mgr->isr_event_head + event_fill_size) % PERF_EVENT_MAX_ISR_EVENT_BUFFER_SIZE;

    flags = gf_spin_lock_irqsave(perf_event_mgr->isr_event_lock);

    perf_event_mgr->isr_event_num -= event_fill_number;
    perf_event_mgr->isr_event_buffer_size -= event_fill_size;

    gf_spin_unlock_irqrestore(perf_event_mgr->isr_event_lock, flags);

    get_perf_event->isr_filled_num = event_fill_number;
    get_perf_event->isr_filled_size = event_fill_size;

exit:
    perf_event_trace("perf event get isr num:%x, head:%x\n", event_fill_number, perf_event_mgr->isr_event_head);

    return ret;
}

int perf_event_get_status(adapter_t *adapter, gf_perf_status_t *get_status)
{
    perf_event_mgr_t *perf_event_mgr = adapter->perf_event_mgr;
    int ret = 0;

    get_status->started = perf_event_mgr->perf_started;
    get_status->miu_started = adapter->ctl_flags.miu_counter_enable;

    return ret;
}

int perf_event_set_miu_reg_list(adapter_t *adapter, gf_miu_reg_list_perf_event_t *miu_reg_list)
{
    perf_event_mgr_t *perf_event_mgr        = adapter->perf_event_mgr;
    gf_miu_list_item_t *miu_table          = NULL;
    unsigned int miu_table_size             = sizeof(gf_miu_list_item_t)*miu_reg_list->miu_table_length;
    int ret                                 = 0;

    perf_event_trace("perf_event_set_miu_reg_list enter\n");

    miu_table = gf_calloc(miu_table_size);
    gf_assert(miu_table != NULL, "miu_table != NULL");

    // get miu table from user space, which will contains write/read command to/from miu regsiter.
    gf_copy_from_user(miu_table, miu_reg_list->miu_table, miu_table_size);

    gf_spin_lock(perf_event_mgr->miu_table_lock);
    if(perf_event_mgr->miu_table != NULL)
    {
        gf_free(perf_event_mgr->miu_table);
    }
    perf_event_mgr->miu_table = miu_table;
    perf_event_mgr->miu_table_length = miu_reg_list->miu_table_length;
    gf_spin_unlock(perf_event_mgr->miu_table_lock);

    perf_event_trace("perf_event_set_miu_reg_list exit\n");

    return ret;
}

int perf_event_direct_get_miu_dump_event(adapter_t *adapter, gf_direct_get_miu_dump_perf_event_t *get_perf_event)
{
    perf_event_mgr_t     *perf_event_mgr     = adapter->perf_event_mgr;
    gf_miu_list_item_t  *local_miu_table    = NULL;
    unsigned long long   timestamp           = 0;
    unsigned int         miu_table_size      = 0;

    perf_event_trace("perf_event_direct_get_miu_dump_event enter\n");

    miu_table_size = sizeof(gf_miu_list_item_t)*(get_perf_event->miu_table_length);

    local_miu_table = gf_calloc(miu_table_size);
    gf_assert(local_miu_table != NULL, "local_miu_table != NULL");

    gf_copy_from_user(local_miu_table, get_perf_event->miu_table, miu_table_size);

    gf_get_nsecs(&timestamp);
    get_perf_event->timestamp_high = timestamp >> 32;
    get_perf_event->timestamp_low = (timestamp) & 0xffffffff;

    perf_event_mgr->chip_func->direct_get_miu_counter(adapter, local_miu_table, get_perf_event->miu_table_length, 0);

    gf_copy_to_user(get_perf_event->miu_table, local_miu_table, miu_table_size);

    gf_free(local_miu_table);

    perf_event_trace("perf_event_direct_get_miu_dump_event exit\n");

    return 0;
}

int perf_event_util_miu_counter_dump(adapter_t *adapter, gf_miu_list_item_t *miu_list, unsigned int list_length, int force_read)
{
    perf_event_mgr_t   *perf_event_mgr = adapter->perf_event_mgr;

    return perf_event_mgr->chip_func->direct_get_miu_counter(adapter, miu_list, list_length, force_read);
}

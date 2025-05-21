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
#include "perfeventi.h"
#include "perfevent.h"

#define   PERF_EVENT_TRACE_ENABLE     0

#if PERF_EVENT_TRACE_ENABLE
#define perf_event_trace    gf_info
#else
#define perf_event_trace(args...)
#endif

#define PERF_EVENT_MAX_ISR_EVENT_BUFFER_SIZE    (512*1024)



int perf_event_init(adapter_t *adapter)
{
    perf_event_mgr_t *perf_event_mgr;
    int ret = 0;

    perf_event_mgr = gf_calloc(sizeof(perf_event_mgr_t));

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

    perf_event_mgr->chip_func = &perf_chip_func;

    adapter->perf_event_mgr = perf_event_mgr;

    return ret;
}

int perf_event_deinit(adapter_t *adapter)
{
    perf_event_mgr_t *perf_event_mgr = adapter->perf_event_mgr;
    perf_event_node *event_node = NULL;
    perf_event_node *event_node_next;

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
    perf_event_node *event_node = NULL;
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
    event_fill_num = perf_event_mgr->miu_event_number;
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
    perf_event_node *event_node = NULL;
    perf_event_node *event_node_next;
    int event_fill_num = 0;
    int i = 0;
    int ret = 0;
    char *dst_buf = get_perf_event->event_buffer;

    perf_event_trace("perf_event_get_event enter\n");

    if (perf_event_mgr->perf_started == FALSE)
    {
        ret = -1;
        goto exit;
    }

    gf_spin_lock(perf_event_mgr->event_list_lock);
    event_fill_num = perf_event_mgr->event_number;
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


int gf_hwq_event_init(adapter_t *adapter)
{
    int ret = 0;
    hwq_event_mgr_t *hwq_event_mgr = NULL;
    adapter->usage_3d=0;
    adapter->usage_vcp=0;
    adapter->usage_vpp=0;

    hwq_event_mgr = gf_calloc(sizeof(hwq_event_mgr_t));
    hwq_event_mgr->sample_time=1000000000;
    hwq_event_mgr->hwq_event = (hwq_event_info *)gf_calloc(adapter->active_engine_count * sizeof(hwq_event_info));

    adapter->hwq_event_mgr=hwq_event_mgr;

    return ret;
}



int gf_hwq_event_deinit(adapter_t *adapter)
{
    hwq_event_mgr_t *hwq_event_mgr;

    hwq_event_mgr=adapter->hwq_event_mgr;
    if(hwq_event_mgr)
    {
        gf_free(hwq_event_mgr->hwq_event);
        hwq_event_mgr->hwq_event = NULL;
        gf_free(hwq_event_mgr);
        adapter->hwq_event_mgr = NULL;

    }

    return 0;
}


int hwq_process_complete_event(adapter_t *adapter, unsigned long long time, unsigned int fence_id,unsigned int engine)
{
    hwq_event_mgr_t *hwq_event_mgr      =adapter->hwq_event_mgr;
    hwq_event_info   *p_hwq_event;

    int ret = 0;

    if (hwq_event_mgr->hwq_started == 0)
    {
        return 0;
    }

    p_hwq_event      = (hwq_event_info*)(hwq_event_mgr->hwq_event)+engine;

    if(p_hwq_event->submit_fence_id==fence_id)
    {
        p_hwq_event->engine_status.status=ENGINE_IDLE;
        p_hwq_event->last_busy2idletime=time;
    }
    else
    {
        p_hwq_event->engine_status.status=ENGINE_BUSY;
    }

    p_hwq_event->complete_fence_id=fence_id;
    p_hwq_event->engine_status.active =1;

    return ret;
}

int hwq_process_submit_event(adapter_t *adapter, unsigned long long time, unsigned int fence_id,unsigned int engine)
{
    hwq_event_mgr_t *hwq_event_mgr                      =adapter->hwq_event_mgr;
    hwq_event_info                       *p_hwq_event;
    unsigned long long                    s_idle_time;
    unsigned long long                    e_idle_time;
    int ret = 0;

    if (hwq_event_mgr->hwq_started == 0)
    {
        return 0;
    }
    p_hwq_event = (hwq_event_info*)(hwq_event_mgr->hwq_event)+engine;

    if(p_hwq_event->engine_status.status==ENGINE_IDLE)
    {
        s_idle_time=p_hwq_event->last_busy2idletime;

        if(hwq_event_mgr->start_time > s_idle_time)
        {
            s_idle_time     =  hwq_event_mgr->start_time ;
        }
        e_idle_time         = time;
        p_hwq_event->idle_time+=(e_idle_time - s_idle_time);

    }
    p_hwq_event->submit_fence_id=fence_id;
    p_hwq_event->engine_status.active =1;
    p_hwq_event->engine_status.status =ENGINE_BUSY;

    return ret;
}


int hwq_process_vsync_event(adapter_t *adapter, unsigned long long time)
{
    gf_hwq_info  hwq_info;
    hwq_event_mgr_t *hwq_event_mgr = adapter->hwq_event_mgr;
    perf_event_mgr_t   *perf_event_mgr = adapter->perf_event_mgr;
    unsigned long long sample_time;

    unsigned int  engine           = 0;
    hwq_event_info       *p_hwq_event;
    unsigned long long   s_idle_time;
    unsigned long long   e_idle_time;
    int ret=0;

    if(!(adapter->ctl_flags.hwq_event_enable))
    {
        return 0;
    }

    hwq_get_hwq_info(adapter,&hwq_info);

    if(hwq_event_mgr==NULL)
    {
        return -1;
    }
    if(hwq_event_mgr->hwq_started==0)
    {
       return 0;
    }
    sample_time=1000000000;
    if(time-hwq_event_mgr->start_time<sample_time)
    {
        return 0;
    }

    e_idle_time=time;

    for(engine = 0; engine < adapter->active_engine_count; engine++)
    {
        p_hwq_event   = (hwq_event_info*)(hwq_event_mgr->hwq_event)+engine;
        if(p_hwq_event->engine_status.active==0)
        {
            if(p_hwq_event->engine_status.status==ENGINE_IDLE || p_hwq_event->engine_status.status==ENGINE_NONE)
            {
                 p_hwq_event->engine_usage=0;
            }
            else if(p_hwq_event->engine_status.status==ENGINE_BUSY)
            {
                 p_hwq_event->engine_usage=100;
            }
            p_hwq_event->idle_time=0;
            continue;
        }

        if(p_hwq_event->engine_status.status==ENGINE_IDLE)
        {
            s_idle_time         = p_hwq_event->last_busy2idletime;

            if(hwq_event_mgr->start_time > s_idle_time)
            {
                s_idle_time     =  hwq_event_mgr->start_time ;
            }
            p_hwq_event->idle_time+=(e_idle_time - s_idle_time);
        }

        if (time != hwq_event_mgr->start_time)
        {
            p_hwq_event->engine_usage = 100 - gf_do_div(p_hwq_event->idle_time*100, time - hwq_event_mgr->start_time);
            //gf_info("\n EngineNum=%d engine_usage ALL=%llu%%\n",engine,p_hwq_event->engine_usage);
        }

        p_hwq_event->idle_time=0;
        p_hwq_event->engine_status.active=0;
    }

    gf_memset(&hwq_info,0,sizeof(gf_hwq_info));

    ret = perf_event_mgr->chip_func->calculate_engine_usage(adapter, &hwq_info);

    adapter->usage_3d = hwq_info.Usage_3D;
    adapter->usage_vcp = hwq_info.Usage_VCP;
    adapter->usage_vpp = hwq_info.Usage_VPP;
    //gf_info(" hwq_process_vsync_event adapter->usage_3d=%d adapter->usage_vcp = %d \n", adapter->usage_3d, adapter->usage_vcp );
    hwq_event_mgr->start_time=time;
    return ret;
}

int hwq_get_hwq_info(void *adp,gf_hwq_info *hwq_info)
{
    adapter_t *adapter = adp;
    hwq_event_mgr_t *hwq_event_mgr = adapter->hwq_event_mgr;
    int ret=0;
    unsigned long long         time=0;
    unsigned int               sample_time=0;

    if( !(adapter->ctl_flags.hwq_event_enable) || !(adapter->hwq_event_mgr) )
    {
        return -1;
    }

    if(hwq_event_mgr->hwq_started==0)
    {
        gf_get_nsecs(&time);
        hwq_event_mgr->start_time=time;
        hwq_event_mgr->hwq_started=1;
    }

    if(sample_time<50)
    {
       sample_time=50;
    }

    if(hwq_event_mgr->sample_time!=sample_time*1000ull*1000)
    {
        hwq_event_mgr->sample_time=sample_time*1000ull*1000;
    }

    //gf_util tool will get gpu usage through this inteface.
    hwq_info->Usage_3D = adapter->usage_3d;
    hwq_info->Usage_VCP = adapter->usage_vcp;
    hwq_info->Usage_VPP = adapter->usage_vpp;

    return ret;
}

int hwq_get_hwq_info_ext(adapter_t *adapter,gfx_hwq_info_ext *phwq_info_ext)
{
    perf_event_mgr_t   *perf_event_mgr = adapter->perf_event_mgr;
    return perf_event_mgr->chip_func->calculate_engine_usage_ext(adapter, phwq_info_ext);
}

int hwq_get_video_info(void *adp, gf_video_info *video_info)
{
    int ret = 0, i;
    adapter_t *adapter = (adapter_t *)adp;

    unsigned long long interval_ms = 0;
    unsigned long long time = 0;
    static unsigned long long last_time[VCP_INFO_COUNT] = {0},last_decodetime[VCP_INFO_COUNT] = {0};
    static unsigned int frame_num[VCP_INFO_COUNT] = {0},decode_framenum[VCP_INFO_COUNT] = {0},bit_size[VCP_INFO_COUNT] = {0};

    if( !(adapter->ctl_flags.hwq_event_enable) || !(adapter->hwq_event_mgr) )
    {
        return -1;
    }

    i = video_info->index;
    if (!adapter->vcp_info[i].enable) { //inital again
        last_time[i] = 0;
        frame_num[i] = 0;
        bit_size[i] = 0;
        return 0;
    }
    video_info->pid[i] = adapter->vcp_info[i].pid;
    video_info->width[i] = adapter->vcp_info[i].DecodeWidth;
    video_info->height[i] = adapter->vcp_info[i].DecodeHeight;
    video_info->TotalDecodeFrameNum[i] = adapter->vcp_info[i].TotalDecodeFrameNum;
    video_info->TotalRenderFrameNum[i] = adapter->vcp_info[i].TotalRenderFrameNum;
    switch(adapter->vcp_info[i].DecodeApi)
    {
        case 0:
            gf_strncpy(video_info->decodeapi[i], "VAAPI", gf_strlen("VAAPI"));
            break;
        case 1:
            gf_strncpy(video_info->decodeapi[i], "VDPAU", gf_strlen("VDPAU"));
            break;
    }

    switch(adapter->vcp_info[i].PresentApi)
    {
        case 1:
            gf_strncpy(video_info->presentapi[i], "VAAPI", gf_strlen("VAAPI"));
            break;
        case 2:
            gf_strncpy(video_info->presentapi[i], "VDPAU", gf_strlen("VDPAU"));
            break;
        case 3:
            gf_strncpy(video_info->presentapi[i], "GPU", gf_strlen("GPU"));
            break;
        default:
            gf_strncpy(video_info->presentapi[i], "UNKNOW", gf_strlen("UNKNOW"));
            break;
    }

    switch(adapter->vcp_info[i].dwDecodeType)
    {
        case 128:
            gf_strncpy(video_info->codec[i], "H264ENC", gf_strlen("H264ENC"));
            break;
        case 134:
            gf_strncpy(video_info->codec[i], "HEVCENC", gf_strlen("HEVCENC"));
            break;
        case 132:
            gf_strncpy(video_info->codec[i], "JPEGENC", gf_strlen("JPEGENC"));
            break;
        case 0:
            gf_strncpy(video_info->codec[i], "MPEG2", gf_strlen("MPEG2"));
            break;
        case 1:
            gf_strncpy(video_info->codec[i], "MPEG4", gf_strlen("MPEG4"));
            break;
        case 2:
            gf_strncpy(video_info->codec[i], "VC1/WMV9", gf_strlen("VC1/WMV9"));
            break;
        case 3:
            gf_strncpy(video_info->codec[i], "H264CAVLC", gf_strlen("H264CAVLC"));
            break;
        case 4:
            gf_strncpy(video_info->codec[i], "H264CABAC", gf_strlen("H264CABAC"));
            break;
        case 5:
            gf_strncpy(video_info->codec[i], "AVS", gf_strlen("AVS"));
            break;
        case 7:
            gf_strncpy(video_info->codec[i], "VP8", gf_strlen("VP8"));
            break;
        case 9:
            gf_strncpy(video_info->codec[i], "JPEG", gf_strlen("JPEG"));
            break;
        case 10:
            gf_strncpy(video_info->codec[i], "HEVC", gf_strlen("HEVC"));
            break;
        case 11:
            gf_strncpy(video_info->codec[i], "H263", gf_strlen("H263"));
            break;
        case 13:
            gf_strncpy(video_info->codec[i], "AVS2", gf_strlen("AVS2"));
            break;
        default:
            gf_strncpy(video_info->codec[i], "UNKNOWN", gf_strlen("UNKNOWN"));
            break;
    }

    gf_get_nsecs(&time);
    if (last_time[i] == 0) //first time
    {
        last_time[i] = time;
        frame_num[i] = adapter->vcp_info[i].TotalRenderFrameNum;
        bit_size[i] = adapter->vcp_info[i].TotalBitstreamSize;
        video_info->bitrate[i] = 0;
        video_info->presentspeed[i] = 0;
        last_decodetime[i] = adapter->vcp_info[i].TotalDecodetime;
        decode_framenum[i] = adapter->vcp_info[i].TotalDecodeFrameNum;
        return ret;
    }
    interval_ms = gf_do_div(time - last_time[i], 1000000);
    if (interval_ms < 1 || adapter->vcp_info[i].TotalDecodetime == last_decodetime[i])
        return 0;

    video_info->presentspeed[i] = gf_do_div((adapter->vcp_info[i].TotalRenderFrameNum - frame_num[i]) * 1000, interval_ms);
    video_info->decodespeed[i] = gf_do_div((adapter->vcp_info[i].TotalDecodeFrameNum - decode_framenum[i]) * 1000, adapter->vcp_info[i].TotalDecodetime -last_decodetime[i]);
    video_info->bitrate[i] = gf_do_div((adapter->vcp_info[i].TotalBitstreamSize - bit_size[i]) * 8 * 1000, interval_ms * 1024);
    last_time[i] = time;
    frame_num[i] = adapter->vcp_info[i].TotalRenderFrameNum;
    bit_size[i] = adapter->vcp_info[i].TotalBitstreamSize;
    last_decodetime[i] = adapter->vcp_info[i].TotalDecodetime;
    decode_framenum[i] = adapter->vcp_info[i].TotalDecodeFrameNum;

    //gf_info("---presentspeed = %d,  bitrate %d kbit/s, pid %d\n", video_info->presentspeed[i], video_info->bitrate[i], video_info->pid[i]);
    return ret;
}

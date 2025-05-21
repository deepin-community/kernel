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
#ifndef __PERF_EVENTI_H__
#define __PERF_EVENTI_H__

#include "gf_adapter.h"
#include "list.h"


typedef struct _perf_chip_func
{
    int (*direct_get_miu_counter)(adapter_t *adapter, gf_miu_list_item_t *miu_table, unsigned int miu_table_length,int force_read);
    int  (*calculate_engine_usage)(adapter_t *, gf_hwq_info *);
    int  (*calculate_engine_usage_ext)(adapter_t *, gfx_hwq_info_ext *);
}perf_chip_func_t;

extern perf_chip_func_t perf_chip_func;

typedef struct _perf_event_node
{
    struct list_head        list_node;
    gf_perf_event_header_t perf_event;
}perf_event_node;

typedef struct _perf_event_mgr
{
    int perf_started;

    struct list_head event_list;
    int event_number;
    int max_event_number;
    int lost_event_number;
    unsigned long long lost_event_time;
    struct os_spinlock  *event_list_lock;

    char *isr_event_buffer;
    int isr_event_tail;         //offset in bytes
    int isr_event_head;         //offset in bytes
    int isr_event_buffer_size;  //size in bytes
    int isr_event_num;
    struct os_spinlock  *isr_event_lock;

    gf_miu_list_item_t     *miu_table;
    unsigned int            miu_table_length;
    struct os_spinlock      *miu_table_lock;

    struct list_head        miu_event_list;
    unsigned int            miu_event_number;
    unsigned int            miu_max_event_number;
    unsigned int            miu_lost_event_number;
    unsigned long long      miu_lost_event_time;
    struct os_spinlock      *miu_event_list_lock;

    perf_chip_func_t        *chip_func;

}perf_event_mgr_t;

#define ENGINE_NONE 0x0
#define ENGINE_IDLE 0x1
#define ENGINE_BUSY 0x2
 typedef struct engine_status
{
    unsigned int  active         :1;
    unsigned int  status         :2;
    unsigned int  reserved       :29;
}engine_status_t;


typedef struct _hwq_event_info
{

    unsigned int  submit_fence_id;
    unsigned int  complete_fence_id;
    unsigned long long last_busy2idletime;

    engine_status_t engine_status;

    unsigned long long idle_time;
    unsigned long long engine_usage;


}hwq_event_info;


typedef struct _hwq_event_mgr
{
    int hwq_started;
    unsigned long long start_time;
    unsigned long long sample_time;

    hwq_event_info *hwq_event;
}hwq_event_mgr_t;

#endif


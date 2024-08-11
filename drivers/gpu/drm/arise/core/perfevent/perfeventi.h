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

#ifndef __PERF_EVENTI_H__
#define __PERF_EVENTI_H__

#include "gf_adapter.h"
#include "gf_def.h"
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


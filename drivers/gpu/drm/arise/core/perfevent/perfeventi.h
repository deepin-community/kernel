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

}perf_chip_func_t;

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

#endif


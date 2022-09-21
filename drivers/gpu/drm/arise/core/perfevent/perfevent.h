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

#ifndef __PERF_EVENT_H__
#define __PERF_EVENT_H__

#include "gf_def.h"

extern int perf_event_init(adapter_t *adapter);
extern int perf_event_deinit(adapter_t *adapter);
extern int perf_event_begin(adapter_t *adapter, gf_begin_perf_event_t *begin);
extern int perf_event_end(adapter_t *adapter, gf_end_perf_event_t *end);
extern int perf_event_add_event(adapter_t *adapter, gf_perf_event_t *perf_event);
extern int perf_event_get_event(adapter_t *adapter, gf_get_perf_event_t *get_perf_event);
extern int perf_event_get_status(adapter_t *adapter, gf_perf_status_t *get_status);
extern int perf_event_add_isr_event(adapter_t *adapter, gf_perf_event_t *perf_event);
extern int perf_event_get_isr_event(adapter_t *adapter, gf_get_perf_event_t *get_perf_event);

extern int perf_event_begin_miu_dump(adapter_t *adapter, gf_begin_miu_dump_perf_event_t *miu_begin);
extern int perf_event_end_miu_dump(adapter_t *adapter, gf_end_miu_dump_perf_event_t *miu_end);
extern int perf_event_set_miu_reg_list(adapter_t *adapter, gf_miu_reg_list_perf_event_t *miu_reg_list);
extern int perf_event_add_miu_event(adapter_t *adapter, unsigned int gpu_context, unsigned long long task_id, unsigned long long fence_id, int task_end);
extern int perf_event_get_miu_event(adapter_t *adapter, gf_get_miu_dump_perf_event_t *get_miu_perf_event);
extern int perf_event_util_miu_counter_dump(adapter_t *adapter, gf_miu_list_item_t *miu_list, unsigned int list_length, int fore_read);
extern int perf_event_direct_get_miu_dump_event(adapter_t *adapter, gf_direct_get_miu_dump_perf_event_t *current_miu);

#endif


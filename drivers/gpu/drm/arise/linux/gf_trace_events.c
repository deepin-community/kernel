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

#include "os_interface.h"
#include "gf.h"

#define CREATE_TRACE_POINTS
#include "gf_trace.h"


/* os interface for trace points */
void gf_fence_back_trace_event(int engine_index, unsigned long long fence_id)
{
    trace_gfx_fence_back(engine_index, fence_id);
}

void gf_task_create_trace_event(int engine_index, unsigned int context,
                                unsigned long long task_id, unsigned int task_type)
{
    trace_gfx_task_create(engine_index, context, task_id, task_type);
}

void gf_task_submit_trace_event(int engine_index, unsigned int context,
                                unsigned long long task_id, unsigned int task_type,
                                unsigned long long fence_id, unsigned int args)
{
    trace_gfx_task_submit(engine_index, context, task_id, task_type, fence_id, args);
}

void gf_begin_section_trace_event(const char* desc)
{
    trace_gfx_begin_section(desc);
}

void gf_end_section_trace_event(int result)
{
    trace_gfx_end_section(result);
}

void gf_counter_trace_event(const char* desc, unsigned int value)
{
    trace_gfx_counter(desc, value);
}

/* register/unregister probe functions */
void gf_register_trace_events(void)
{

}

void gf_unregister_trace_events(void)
{

}


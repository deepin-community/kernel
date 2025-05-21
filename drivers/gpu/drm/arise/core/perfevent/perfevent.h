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
#ifndef __PERF_EVENT_H__
#define __PERF_EVENT_H__

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

extern int gf_hwq_event_init(adapter_t *adapter);
extern int gf_hwq_event_deinit(adapter_t *adapter);
extern int hwq_process_complete_event(adapter_t *adapter, unsigned long long time, unsigned int fence_id,unsigned int engine);
extern int hwq_process_submit_event(adapter_t *adapter, unsigned long long time, unsigned int fence_id,unsigned int engine);
extern int hwq_process_vsync_event(adapter_t *adapter, unsigned long long time);


extern int hwq_get_hwq_info(void *adp,gf_hwq_info *hwq_info);
extern int hwq_get_video_info(void *adp, gf_video_info *video_info);
extern int hwq_get_hwq_info_ext(adapter_t *adapter,gfx_hwq_info_ext *phwq_info_ext);

#endif


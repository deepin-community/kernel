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
#ifndef __PM_DFS_E3K_H__

#include "gf_adapter.h"
#include "vidsch.h"
#include "vidschi.h"

int vidsch_power_clock_on_off_e3k(vidsch_mgr_t *sch_mgr, unsigned int off);
int vidsch_power_clock_on_off_vcp(vidsch_mgr_t *sch_mgr, unsigned int off);
void vidsch_power_tuning_e3k(adapter_t *adapter, unsigned int gfx_only);
void vidsch_power_switch_e3k(adapter_t *adapter, unsigned int power_state, unsigned int holding_ms, unsigned int force, unsigned int sync_wait);
void vidsch_boost_e3k(adapter_t *adapter, unsigned int power_state, unsigned int holding_ms, unsigned int force);
#endif

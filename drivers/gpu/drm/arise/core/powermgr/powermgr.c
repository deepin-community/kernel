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
#include "vidsch.h"
#include "vidmm.h"
#include "global.h"
#include "vidschi.h"
#include "powermgr.h"

int pm_save_state(adapter_t *adapter, int need_save_memory)
{
    int ret = S_OK;
    struct vidschedule *schedule = adapter->schedule;

    schedule->chip_func->boost(adapter, ENG_POWER_STATE_E0, 60 * 1000, FALSE);

    adapter->ctl_flags.boost = FALSE;
    adapter->in_suspend_resume = TRUE;

    ret = cm_save(adapter, need_save_memory);

    if (ret != S_OK)
    {
        return ret;
    }

    ret = vidsch_save(adapter);

    if (ret != S_OK)
    {
        return ret;
    }

    ret = vidmm_save(adapter);

    adapter->in_suspend_resume = FALSE;

    return ret;
    /* system will suspend, set our card mode as uninitialize */
}


//temp use only, will remove after resume stable.
static inline void util_print_time(char *info)
{
    long time_sec = 0, time_usec = 0;

    gf_getsecs(&time_sec, &time_usec);
    gf_info("%s %ld(ms)\n",info,(time_sec * 1000 + gf_do_div(time_usec, 1000)));
}

int pm_restore_state(adapter_t *adapter)
{
    int ret = S_OK;

    adapter->in_suspend_resume = TRUE;

    //temp use only, will remove all this function called after resume stable.
    util_print_time("pm_restore_state enter, cur time");

    glb_init_chip_interface(adapter);
    util_print_time("glb_init_chip_interface finish, cur time");

    vidmm_restore(adapter);
    util_print_time("vidmm_restore finish, cur time");

    vidsch_restore(adapter);
    util_print_time("vidsch_restore finish, cur time");

    cm_restore(adapter);
    util_print_time("cm_restore finish, cur time");

    adapter->ctl_flags.boost = TRUE;
    adapter->in_suspend_resume = FALSE;

    return ret;
}

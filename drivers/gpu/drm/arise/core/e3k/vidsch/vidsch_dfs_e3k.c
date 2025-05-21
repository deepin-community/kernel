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
#include "vidsch_dfs_e3k.h"
#include "vidsch.h"
#include "vidschi.h"
#include "chip_include_e3k.h"
#include "vidsch_engine_e3k.h"
#include "register_e3k.h"

#define GFX_ENGINE_MASK (1 << RB_INDEX_GFXL | 1 << RB_INDEX_GFXH)

static unsigned char vidsch_cacl_available_gpc_bit(adapter_t * adapter)
{
    unsigned char bits = 0;

    if(adapter->hw_caps.chip_slice_mask &0xF00) bits |=0x10;
    if(adapter->hw_caps.chip_slice_mask &0x0F0) bits |=0x20;
    if(adapter->hw_caps.chip_slice_mask &0x00F) bits |=0x40;

    return bits;
}

int vidsch_power_clock_on_off_e3k(vidsch_mgr_t *sch_mgr, unsigned int off)
{
    adapter_t * adapter = sch_mgr->adapter;
    vidschedule_t *schedule = adapter->schedule;
    unsigned char used_gpc_mask_bit = vidsch_cacl_available_gpc_bit(adapter);
    unsigned long flags = 0;

    flags = gf_spin_lock_irqsave(schedule->power_status_lock);
    //power_trace("[CG] used_gpc_mask_bit:%x\n", used_gpc_mask_bit);

    if(off)
    {
        switch(sch_mgr->engine_index)
        {
            case RB_INDEX_GFXL:
            case RB_INDEX_GFXH:
                write_reg_e3k(adapter->mmio, CR_C, 0x01, 0, 0x8F);
                break;
            case RB_INDEX_VCP0:
                write_reg_e3k(adapter->mmio, CR_C, 0x01, 0, 0xF7);
                break;
            case RB_INDEX_VCP1:
                write_reg_e3k(adapter->mmio, CR_C, 0x01, 0, 0xFB);
                break;
            case RB_INDEX_VPP:
                write_reg_e3k(adapter->mmio, CR_C, 0x01, 0, 0xFD);
                break;
            default:
                  break;
        }
        power_trace("[CG] engine %d do cg power off,  CR01:%x\n", sch_mgr->engine_index, read_reg_e3k(adapter->mmio, CR_C, 0x01));
    }
    else
    {
        switch(sch_mgr->engine_index)
        {
            case RB_INDEX_GFXL:
            case RB_INDEX_GFXH:
                write_reg_e3k(adapter->mmio, CR_C, 0x01, (0x70 & used_gpc_mask_bit), 0x8F);
                break;
            case RB_INDEX_VCP0:
                write_reg_e3k(adapter->mmio, CR_C, 0x01, 8, 0xF7);
                break;
            case RB_INDEX_VCP1:
                write_reg_e3k(adapter->mmio, CR_C, 0x01, 4, 0xFB);
                break;
            case RB_INDEX_VPP:
                write_reg_e3k(adapter->mmio, CR_C, 0x01, 2, 0xFD);
                break;
            default:
                  break;
        }
        power_trace("[CG] engine %d do cg power on,  CR01:%x\n", sch_mgr->engine_index, read_reg_e3k(adapter->mmio, CR_C, 0x01));
    }

    gf_spin_unlock_irqrestore(schedule->power_status_lock, flags);

    return S_OK;
}

int vidsch_power_clock_on_off_vcp(vidsch_mgr_t *sch_mgr, unsigned int off)
{
    adapter_t * adapter = sch_mgr->adapter;
    vidschedule_t *schedule = adapter->schedule;
    unsigned long flags = 0;

    flags = gf_spin_lock_irqsave(schedule->power_status_lock);
    //power_trace("[CG] used_gpc_mask_bit:%x\n", used_gpc_mask_bit);

    if(off)
    {
        write_reg_e3k(adapter->mmio, CR_C, 0x01, 0, 0xF7);//VCP0
        if(adapter->chip_id < CHIP_ARISE1020)//CHIP_ARISE1020 only has one vcp core
        {
            write_reg_e3k(adapter->mmio, CR_C, 0x01, 0, 0xFB);//VCP1
        }
        power_trace("[CG] engine %d do cg power off,  CR01:%x\n", sch_mgr->engine_index, read_reg_e3k(adapter->mmio, CR_C, 0x01));
    }
    else
    {
        write_reg_e3k(adapter->mmio, CR_C, 0x01, 8, 0xF7);//VCP0
        if(adapter->chip_id < CHIP_ARISE1020)//CHIP_ARISE1020 only has one vcp core
        {
            write_reg_e3k(adapter->mmio, CR_C, 0x01, 4, 0xFB);//VCP1
        }
        power_trace("[CG] engine %d do cg power on,  CR01:%x\n", sch_mgr->engine_index, read_reg_e3k(adapter->mmio, CR_C, 0x01));
    }

    gf_spin_unlock_irqrestore(schedule->power_status_lock, flags);

    return S_OK;
}

int vidsch_engine_power_state_e3k(adapter_t *adapter)
{
    unsigned int power_state;
    unsigned char overclock = 0;

    if ((adapter->usage_vpp > 90) || (adapter->usage_vcp > 90) || (adapter->usage_3d > 90))
        overclock = 1;

    power_state = adapter->power_state;

    if (adapter->usage_vcp || adapter->usage_vpp)
        power_state =  power_state > ENG_POWER_STATE_E1 ? ENG_POWER_STATE_E1 : power_state;

    if (adapter->disp_state == 3)
        power_state =  power_state > ENG_POWER_STATE_E3 ? ENG_POWER_STATE_E3 : power_state;

    if (overclock)
    {
        if (power_state > ENG_POWER_STATE_E0)
            power_state -= 1;
    }
    else
    {
        if (power_state == ENG_POWER_STATE_E0)
        {
            if ((adapter->usage_vpp < 45) && (adapter->usage_vcp < 45) && (adapter->usage_3d < 45))
                power_state = ENG_POWER_STATE_E1;
        }
        else if (power_state == ENG_POWER_STATE_E1)
        {
            if (!adapter->usage_vpp && !adapter->usage_vcp)
                power_state = ENG_POWER_STATE_E2;
        }
        else if (power_state == ENG_POWER_STATE_E2)
        {
            if (adapter->usage_3d < 20)
                power_state = ENG_POWER_STATE_E3;
        }
        else if (power_state == ENG_POWER_STATE_E3)
        {
            if (adapter->disp_state == 2)
                power_state = ENG_POWER_STATE_E4;
            else if (adapter->disp_state == 1)
                power_state = ENG_POWER_STATE_E5;
        }
        else if (power_state == ENG_POWER_STATE_E4)
        {
            if (adapter->disp_state == 3)
                power_state = ENG_POWER_STATE_E3;
            else if (adapter->disp_state == 1)
                power_state = ENG_POWER_STATE_E5;
        }
        else if (power_state == ENG_POWER_STATE_E5)
        {
            if (adapter->disp_state == 3)
                power_state = ENG_POWER_STATE_E3;
            else if (adapter->disp_state == 2)
                power_state = ENG_POWER_STATE_E4;
        }
        else
        {
            power_state = ENG_POWER_STATE_E0;
        }
    }

    return power_state;
}

static int vidsch_engine_power_switch_wait(adapter_t *adapter)
{
    unsigned int ret = -1, value, tmp, retry;

    retry = 100;
    while (retry > 0)
    {
        tmp = gf_read32(adapter->mmio + 0x8E000);
        value = (tmp & 0x00ff0000) >> 16;
        if (value == 0x22 || !value)
        {
            ret = 0;
            break;
        }
        else if (value == 0x33)
        {
            break;
        }

        retry--;
        gf_udelay(1000);
    }

    return ret;
}

int vidsch_engine_power_state_check_e3k(adapter_t *adapter, int *switching)
{
    unsigned int power_state, value, tmp;
    static unsigned int max_fail = 10;
    unsigned long long timestamp = 0ll;

    tmp = gf_read32(adapter->mmio + 0x8E000);

    value = (tmp & 0x00ff0000) >> 16;

    if (switching)
    {
        *switching = !((value == 0x00) || (value == 0x22));

        if (*switching)
            return -1;
    }

    power_state = (tmp & 0xff000000) >> 24;

    switch (power_state)
    {
    case 0x00:
        power_state = ENG_POWER_STATE_E0;
        break;

    case 0x10:
        power_state = ENG_POWER_STATE_E1;
        break;

    case 0x12:
        power_state = ENG_POWER_STATE_E2;
        break;

    case 0x22:
        power_state = ENG_POWER_STATE_E3;
        break;

    case 0x24:
        power_state = ENG_POWER_STATE_E4;
        break;

    case 0x44:
        power_state = ENG_POWER_STATE_E5;
        break;

    default:
        power_state = ENG_POWER_STATE_NONE;
        break;
    }

    if (power_state != adapter->power_state)
    {
        gf_error("power state check error last: E%u current: E%u\n", adapter->power_state, power_state);

        if (max_fail > 0)
        {
            max_fail--;
        }
        else
        {
            vidsch_engine_power_switch_wait(adapter);

            tmp = gf_read32(adapter->mmio + 0x8E000);
            tmp &= 0xff00ff00;
            tmp |= 0x00110000;

            gf_write32(adapter->mmio + 0x8E000, tmp);
            gf_write32(adapter->mmio + 0x8E080, 0x1);

            gf_get_nsecs(&timestamp);
            timestamp = gf_do_div(timestamp, 1000000);

            adapter->ctl_flags.boost = FALSE;
            adapter->power_state = ENG_POWER_STATE_E0;
            adapter->power_holding_time = timestamp + 10 * 1000;
            vidsch_engine_power_switch_wait(adapter);

            max_fail = 10;

            gf_error("too many power state check fail, disable power state switch.\n");
        }

        return -1;
    }

    return 0;
}

void vidsch_power_switch_e3k(adapter_t *adapter, unsigned int power_state, unsigned int holding_ms, unsigned int force, unsigned int sync_wait)
{
    unsigned long long timestamp = 0ll;
    unsigned int switching = 0, index = 0, tmp = 0, ret;
    unsigned char value;
    vidschedule_t *schedule = adapter->schedule;
    unsigned long flags = 0;

    if (!adapter->ctl_flags.boost || !adapter->pwm_level.EnablePowerSwitch)
        return;

    gf_get_nsecs(&timestamp);
    timestamp = gf_do_div(timestamp, 1000000);

    flags = gf_spin_lock_irqsave(schedule->power_status_lock);

    vidsch_engine_power_state_check_e3k(adapter, &switching);

    if (!force)
    {
        if (adapter->power_state == power_state)
        {
            if (timestamp + holding_ms > adapter->power_holding_time)
                adapter->power_holding_time = timestamp + holding_ms;

            goto exit_unlock;
        }

        if ((adapter->power_state < power_state) && (timestamp < adapter->power_holding_time))
            goto exit_unlock;
    }

    if (sync_wait && switching)
        vidsch_engine_power_switch_wait(adapter);
    else if (!sync_wait && switching)
        goto exit_unlock;

    switch (power_state)
    {
    case ENG_POWER_STATE_E0:
        value = 0x00;
        break;

    case ENG_POWER_STATE_E1:
        value = 0x10;
        break;

    case ENG_POWER_STATE_E2:
        value = 0x12;
        break;

    case ENG_POWER_STATE_E3:
        value = 0x22;
        break;

    case ENG_POWER_STATE_E4:
        value = 0x24;
        break;

    case ENG_POWER_STATE_E5:
        value = 0x44;
        break;

    default:
        value = 0x00;
        power_state = ENG_POWER_STATE_E0;
        break;
    }

    tmp = gf_read32(adapter->mmio + 0x8E000);
    tmp &= 0xff00ff00;
    tmp |= 0x00110000;
    tmp |= value;

    gf_write32(adapter->mmio + 0x8E000, tmp);
    gf_write32(adapter->mmio + 0x8E080, 0x1);

    gf_get_nsecs(&timestamp);
    timestamp = gf_do_div(timestamp, 1000000);

    if (timestamp + holding_ms > adapter->power_holding_time)
        adapter->power_holding_time = timestamp + holding_ms;

    adapter->power_state = power_state;
    gf_spin_unlock_irqrestore(schedule->power_status_lock, flags);

    ret = vidsch_engine_power_switch_wait(adapter);
    if (ret)
        gf_error("switch power state timeout\n\n");

    gf_get_nsecs(&timestamp);
    timestamp = gf_do_div(timestamp, 1000000);

    gf_debug("switch to E%u done, ts:0x%llu\n", power_state, timestamp);

    return;

exit_unlock:

    gf_spin_unlock_irqrestore(schedule->power_status_lock, flags);
    return;
}

static int vidsch_engine_boost_allow(adapter_t *adapter, unsigned int power_state)
{
    unsigned long long timestamp = 0ll;
    vidschedule_t *schedule = adapter->schedule;
    unsigned int allow = FALSE;

    if (!adapter->ctl_flags.boost || !adapter->pwm_level.EnablePowerSwitch)
        return allow;

    gf_get_nsecs(&timestamp);
    timestamp = gf_do_div(timestamp, 1000000);

    if (adapter->power_state == power_state)
        return FALSE;

    if (adapter->power_state > power_state)
        return TRUE;

    if (timestamp > adapter->power_holding_time)
        allow = TRUE;

    return allow;
}

void vidsch_boost_e3k(adapter_t *adapter, unsigned int power_state, unsigned int holding_ms, unsigned int force)
{
    vidschedule_t *schedule = adapter->schedule;
    vidsch_mgr_t *sch_mgr;
    unsigned long long timestamp = 0ll;
    unsigned long flags = 0;
    unsigned int i, sync_wait = FALSE, allow = FALSE;

    if (!adapter->ctl_flags.boost || !adapter->pwm_level.EnablePowerSwitch)
        return;

    if (power_state == ENG_POWER_STATE_AUTO)
        goto auto_boost;

    flags = gf_spin_lock_irqsave(schedule->power_status_lock);
    if (adapter->power_state > power_state)
    {
        gf_debug("try boost to E%u\n", power_state);
        allow = TRUE;
        sync_wait = TRUE;
        gf_spin_unlock_irqrestore(schedule->power_status_lock, flags);
        goto boost;
    }
    else if (adapter->power_state == power_state)
    {
        gf_get_nsecs(&timestamp);
        timestamp = gf_do_div(timestamp, 1000000);
        if (timestamp + holding_ms > adapter->power_holding_time)
            adapter->power_holding_time = timestamp + holding_ms;
    }
    else if (force)
    {
        allow = TRUE;
        sync_wait = TRUE;
        gf_spin_unlock_irqrestore(schedule->power_status_lock, flags);
        goto boost;
    }

    gf_spin_unlock_irqrestore(schedule->power_status_lock, flags);

auto_boost:
    power_state = vidsch_engine_power_state_e3k(adapter);
    allow = vidsch_engine_boost_allow(adapter, power_state);

boost:
    if (!allow)
        return;

    for (i = 0; i < adapter->active_engine_count; i++)
    {
        sch_mgr= adapter->sch_mgr[i];
        if (!sch_mgr)
            continue;

        gf_mutex_lock(sch_mgr->engine_lock);
    }

    for (i = 0; i < adapter->active_engine_count; i++)
    {
        sch_mgr= adapter->sch_mgr[i];
        if (!sch_mgr)
            continue;

        vidsch_wait_fence_back(adapter, sch_mgr->engine_index, sch_mgr->last_send_fence_id);
    }

    vidsch_power_switch_e3k(adapter, power_state, holding_ms, force, sync_wait);

    for (i = 0; i < adapter->active_engine_count; i++)
    {
        sch_mgr= adapter->sch_mgr[i];
        if (!sch_mgr)
            continue;

        gf_mutex_unlock(sch_mgr->engine_lock);
    }
}

void vidsch_power_tuning_e3k(adapter_t *adapter, unsigned int gfx_only)
{
    vidschedule_t *schedule = adapter->schedule;
    vidsch_mgr_t  *sch_mgr = NULL;
    int engine_index;
    unsigned int engine_mask = gfx_only ? GFX_ENGINE_MASK : ALL_ENGINE_MASK;

    if (!adapter->pwm_level.EnableClockGating)
    {
        return;
    }

    for (engine_index = adapter->active_engine_count-1; engine_index >= 0; engine_index--)
    {
        int ret = S_OK;
        unsigned long flags=0;

        if ((engine_mask & (1<<engine_index)) == 0) continue;

        sch_mgr = adapter->sch_mgr[engine_index];

        if (sch_mgr == NULL)  continue;

        vidsch_update_engine_idle_status(adapter, (1 << engine_index));
        if (sch_mgr->completely_idle)
        {
            flags = gf_spin_lock_irqsave(sch_mgr->power_status_lock);
            if (sch_mgr->chip_func->power_clock &&
                sch_mgr->engine_dvfs_power_on &&
                vidsch_is_fence_back(sch_mgr->adapter, sch_mgr->engine_index, sch_mgr->last_send_fence_id))
            {
                if(ret == S_OK)
                {
                   //gf_info("power off: last_send_fence_id=%d. \n", sch_mgr->last_send_fence_id);
                   sch_mgr->chip_func->power_clock(sch_mgr, TRUE);
                   sch_mgr->engine_dvfs_power_on = FALSE;
                }
            }
            gf_spin_unlock_irqrestore(sch_mgr->power_status_lock, flags);
        }
    }
}



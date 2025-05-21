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
#include "global.h"
#include "register_e3k.h"

void glb_init_chip_interface(adapter_t *adapter)
{
    write_reg_e3k(adapter->mmio, CR_C, 0x01, 0xFF, 0x00); //ensure power on
    gf_info("init engine power status: 0x%x\n", read_reg_e3k(adapter->mmio, CR_C, 0x01));
}

void glb_init_power_caps(adapter_t *adapter)
{
    gf_warning("%s(): is blank, maybe implemented in other function()\n", util_remove_name_suffix(__func__));
}

int glb_detect_power_switch(adapter_t *adapter)
{
    int enabled = FALSE;
    unsigned int value, tmp, retry;

    gf_write32(adapter->mmio + 0x8E080, 0x1);

    retry = 100;
    while (retry > 0)
    {
        tmp = gf_read32(adapter->mmio + 0x8E000);
        value = (tmp & 0x00ff0000) >> 16;
        if (value == 0x22)
        {
            enabled = TRUE;
            break;
        }

        retry--;
        gf_udelay(1000);
    }

    return enabled;
}


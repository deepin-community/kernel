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
#include "gf_params.h"
#include "gf_driver.h"

struct gf_params gf_modparams __read_mostly = {
    .gf_fb_mode = NULL,
#ifdef GF_HW_NULL
    .gf_fb = 0,
#else
    .gf_fb = 1,
#endif
    .gf_pwm_mode = 0x101,
    .gf_dfs_mode = 0,
    .gf_worker_thread_enable = 1,
    .gf_recovery_enable = 1,
    .gf_hang_dump = 0,            /*0-disable, 1-pre hang, 2-post hang, 3-duplicate hang */
    .gf_run_on_qt = 0,
    .gf_flag_buffer_verify = 1,  /*0 - disable, 1 - enable */

    .gf_vesa_tempbuffer_enable = 0, /* control wether reserve memory during boot */

    .miu_channel_size = 0,   /* 0/1/2 for 256B/512B/1kb Swizzle */
    .gf_backdoor_enable = 1,

    //gpc/slice setting
    .chip_slice_mask = 0, /* 0x001 ~ 0xfff or 0, if none zero set, driver will use this setting, otherwise use value from bios*/
    .gf_local_size_g = 0,//adjust local memory size, 0 use real local memory
    .gf_local_size_m = 0,
    .gf_pcie_size_g = 0,//adjust pcie memory size, 0 use real pcie memory
    .gf_pcie_size_m = 0,
    .gf_runtime_pm = 0,
    .debugfs_mask   =  0x1,
    .misc_control_flag = 0x111,
};

#define gf_param_named(name, T, perm, desc) \
    module_param_named(name, gf_modparams.name, T, perm); \
    MODULE_PARM_DESC(name, desc)

gf_param_named(gf_fb, int, 0600, "enable gf drm fb 0=disable, 1=enable");
gf_param_named(gf_fb_mode, charp, 0600, "The fb mode, like string 1920x1080@60");
gf_param_named(gf_vesa_tempbuffer_enable, int, 0444, "control wether reserve memory during boot");
gf_param_named(gf_pwm_mode, int, 0444,"control power mode");
gf_param_named(gf_dfs_mode, int, 0444, "control of dfs");
gf_param_named(gf_worker_thread_enable, int, 0444, "enable work thread to submit");
gf_param_named(gf_recovery_enable, int, 0444, "enable recovery");
gf_param_named(gf_hang_dump, int, 0444, "0-disable, 1-pre hang, 2-post hang, 3-duplicate hang");
gf_param_named(gf_flag_buffer_verify, int, 0444, "");
gf_param_named(gf_run_on_qt, int, 0444, "");
gf_param_named(miu_channel_size, int, 0444, "0/1/2 for 256B/512B/1kb Swizzle");
gf_param_named(gf_backdoor_enable, int, 0444, "enable backdoor");
gf_param_named(chip_slice_mask, int, 0444, "0x001 ~ 0xfff or 0, if none zero set, driver will use this setting, otherwise use value from bios");
gf_param_named(gf_local_size_g, int, 0444, "manual set the local vram size, uint in GB, the size should not larger than real vram size");
gf_param_named(gf_local_size_m, int, 0444, "manual set the local vram size, uint in MB, the size should not larger than real vram size");
gf_param_named(gf_pcie_size_g, int, 0444, "manual set the pcie vram size, uint in GB, the size should not larger than real vram size");
gf_param_named(gf_pcie_size_m, int, 0444, "manual set the pcie vram size, uint in MB, the size should not larger than real vram size");
gf_param_named(gf_runtime_pm, int, 0444, "0-disable, 1-enable");
gf_param_named(debugfs_mask, int, 0444, "debugfs control bits");
gf_param_named(misc_control_flag, int, 0444, "misc control flag");




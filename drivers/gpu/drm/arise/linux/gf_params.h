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

#ifndef __GF_PARAMS_H__
#define __GF_PARAMS_H__
#include <linux/cache.h> /* for __read_mostly */

struct gf_params {
    char *gf_fb_mode;
    int  gf_fb;

    int gf_pwm_mode ; /*0-disable CG, 1-enable CG, 0x10-enable 3D auto mode, 0x20-enable VCP auto mode, 0x40-enable VPP auto mode*/
    int gf_dfs_mode ;/* control dvfs of gf, 0-disable , 1-enable kmd auto tuning */
    int gf_worker_thread_enable;/* control worker thread on/off */
    int gf_recovery_enable ; /* enable recovery when hw hang */
    int gf_hang_dump;/*0-disable, 1-pre hang, 2-post hang, 3-duplicate hang */
    int gf_run_on_qt; /* control wether run on QT */
    int gf_flag_buffer_verify ;/*0 - disable, 1 - enable */
    int gf_vesa_tempbuffer_enable ; /* control wether reserve memory during boot */

    int miu_channel_num;    // 0/1/2 for 1/2/3 Miu channel, Elite 3000 support up to 3 MIU channel
    int miu_channel_size;   //0/1/2 for 256B/512B/1kb Swizzle
    int gf_backdoor_enable;

    //gpc/slice setting
    unsigned int chip_slice_mask;//// CHIP_SLICE_MASK own 12 bits(should be set 0x001 ~ 0xfff), driver use this to set the Slice_Mask which HW used.
    int mem_size;  // 1/2/3/4 for 4G/8G/12G/16G

    int gf_local_size_g;
    int gf_local_size_m;
    int gf_pcie_size_g;
    int gf_pcie_size_m;
    int gf_runtime_pm;
    int debugfs_mask;    //debugfs mask, bit0: gem_enable
    int misc_control_flag;
};

extern struct gf_params gf_modparams;
#endif

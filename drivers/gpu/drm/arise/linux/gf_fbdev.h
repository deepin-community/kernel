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

#ifndef _H_GF_FBDEV_H
#define _H_GF_FBDEV_H
#include "gf_drmfb.h"
#include "gf_device_debug.h"

struct gf_fbdev
{
    unsigned int gpu_device;
    struct drm_fb_helper helper;
    struct drm_gf_framebuffer *fb;
    gf_device_debug_info_t *debug;
};

#define to_gf_fbdev(helper) container_of(helper, struct gf_fbdev, helper)


void gf_fbdev_disable_vesa(gf_card_t *gf);
int gf_fbdev_init(gf_card_t *gf);
int gf_fbdev_deinit(gf_card_t *gf);
void gf_fbdev_set_suspend(gf_card_t *gf, int state);
void gf_fbdev_poll_changed(struct drm_device *dev);
#endif

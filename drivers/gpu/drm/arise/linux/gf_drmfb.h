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

#ifndef  _GF_DRMFB_H_
#define _GF_DRMFB_H_

#include "gf_disp.h"
#include "gf_gem.h"
#include "gf_gem_priv.h"

struct drm_gf_framebuffer
{
    struct drm_framebuffer base;
    struct drm_gf_gem_object *obj;
};
#define to_gfb(fb) container_of((fb), struct drm_gf_framebuffer, base)

struct drm_framebuffer *
gf_fb_create(struct drm_device *dev,
                              struct drm_file *file_priv,
                              #if DRM_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)  || defined (PHYTIUM_2000)
                              const struct drm_mode_fb_cmd2 *mode_cmd
                              #else
                              struct drm_mode_fb_cmd2 *mode_cmd
                              #endif
                              );

void gf_cleanup_fb(struct drm_plane *plane,  struct drm_plane_state *old_state);

struct drm_gf_framebuffer*
__gf_framebuffer_create(struct drm_device *dev,
                        struct drm_mode_fb_cmd2 *mode_cmd,
                        struct drm_gf_gem_object *obj);
#endif

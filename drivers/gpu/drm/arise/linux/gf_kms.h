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

#ifndef  _GF_KMS_
#define _GF_KMS_

#include <linux/version.h>

#ifndef DRM_VERSION_CODE
#define DRM_VERSION_CODE LINUX_VERSION_CODE
#endif


#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_plane_helper.h>
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
#include <drm/drm_atomic_uapi.h>
#endif

#include <drm/drm_crtc_helper.h>

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
#include <drm/drm_probe_helper.h>
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 5, 0)
#include <drm/drm_fourcc.h>
#include <drm/drm_vblank.h>
#include <drm/drm_irq.h>
#endif
#include <drm/drm_fb_helper.h>

#ifndef  container_of
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) (type *)((char *)(ptr) - offsetof(type, member))
#endif

#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
#define to_gf_atomic_state(x) container_of(x, gf_ato_state_t, base_ato_state)
#define to_gf_crtc_state(x)  container_of(x, gf_crtc_state_t, base_cstate)
#define to_gf_plane_state(x)  container_of(x, gf_plane_state_t, base_pstate)
#define to_gf_conn_state(x)  container_of(x, gf_connector_state_t, base_conn_state)
#endif

#define to_gf_crtc(x)  container_of(x, gf_crtc_t, base_crtc)
#define to_gf_connector(x)  container_of(x, gf_connector_t, base_connector)
#define to_gf_plane(x)  container_of(x, gf_plane_t, base_plane)
#define to_gf_encoder(x)  container_of(x, gf_encoder_t, base_encoder)

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
#define DRM_MODE_BLEND_PREMULTI		0
#define DRM_MODE_BLEND_COVERAGE		1
#define DRM_MODE_BLEND_PIXEL_NONE	2
#endif

#define  DRM_MODE_BLEND_CURR_LAYER_ALPHA  0
#define  DRM_MODE_BLEND_LOWER_LAYER_ALPHA 1

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 18, 0)
#define DRM_BLEND_ALPHA_OPAQUE		0xffff
#endif

typedef  struct 
{
    struct drm_crtc    base_crtc;
    unsigned int       pipe;
    int                crtc_dpms;
    int                support_scale;
    unsigned int       scaler_width;
    unsigned int       scaler_height;
    unsigned int       dst_width;
    unsigned int       dst_height;
    unsigned int       plane_cnt;
    struct gf_flip_work *flip_work;
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
    struct drm_gf_gem_object *cursor_bo;
    unsigned short  cursor_x, cursor_y;
    unsigned short  cursor_w, cursor_h;
    unsigned int  lut_entry[256];  //(b | g << 8 | r << 16) for 8bit lut or (b | g << 10 | r << 20) for 10bit lut
#endif
}gf_crtc_t;

typedef struct
{
    struct drm_encoder  base_encoder;
    int                 output_type;
    int                 enc_dpms;
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
    gf_crtc_t*  new_crtc;
#endif
}gf_encoder_t;

typedef struct
{
    struct drm_connector  base_connector;
    int                   output_type;
    int                   monitor_type;
    int                   hda_codec_index;
    int                   hpd_int_bit;
    int                   hpd_enable;
    unsigned char*        edid_data;
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
    gf_encoder_t*  new_encoder;
#endif
    struct
    {
        int support_audio  : 1;
        int compare_edid  : 1;
        int edid_changed  : 1;
        int reserved       : 29;
    };
    int                   hdcp_index;
    int                   hdcp_enable;
    int                   detected; //fot hdcp cts
    int                   hpd_out; //for hdcp cts
}gf_connector_t;

typedef  struct
{
    struct drm_plane base_plane;
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
    unsigned int src_pos;
    unsigned int src_size;
    unsigned int dst_pos;
    unsigned int dst_size;
#else    
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
    struct drm_property *blend_mode_property;
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 18, 0)
    struct drm_property *alpha_property;
#endif
#endif
    struct drm_property *alpha_source_prop;
#endif
    int              crtc_index;
    int              plane_type;
    int              can_window  : 1;  //except PS
    int              can_up_scale  : 1;
    int              can_down_scale : 1;
    int              is_cursor : 1;
}gf_plane_t;

typedef struct
{
    struct drm_framebuffer *fb;
    int  crtc;
    int  stream_type;
    int  crtc_x;
    int  crtc_y;
    int  crtc_w;
    int  crtc_h;
    int  src_x;
    int  src_y;
    int  src_w;
    int  src_h;
#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    struct 
    {
        int  blend_mode;
        int  blend_alpha_source;
        union 
        {
            unsigned int  const_alpha;
            unsigned int  plane_alpha;
            unsigned int  color_key;
        };
    };
#endif
}gf_crtc_flip_t;
//int disp_cbios_crtc_flip(disp_info_t *disp_info, struct drm_crtc *crtc, struct drm_framebuffer *fb,
//        int stream_type, int visible, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y, int dst_w, int dst_h);

typedef struct
{
    int crtc;
    int vsync_on;
    int pos_x;
    int pos_y;
    int width;
    int height;
    struct drm_gf_gem_object *bo;
}gf_cursor_update_t;

#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)

typedef struct
{
    struct drm_connector_state base_conn_state;
}gf_connector_state_t;

typedef  struct
{
    struct drm_crtc_state  base_cstate;
    struct
    {
        int   scale_change : 1;
        int   dst_change    : 1;
        int   reserved      : 30;
    };
}gf_crtc_state_t;

typedef struct
{
    struct drm_plane_state base_pstate;
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 20, 0)
    unsigned int pixel_blend_mode;
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 18, 0)
    unsigned int alpha;
#endif
#endif
    unsigned int alpha_source;
    unsigned int color_key;
    struct 
    {
        unsigned int  disable       : 1;
        unsigned int  legacy_cursor : 1;
        unsigned int  reserved      : 30;
    };
}gf_plane_state_t;

typedef  struct 
{
    struct drm_atomic_state base_ato_state;
}gf_ato_state_t;

#endif

#endif


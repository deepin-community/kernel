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
#ifndef __GF_SPLICE_H__
#define __GF_SPLICE_H__

#include "gf_disp.h"
#include "gf_cbios.h"
#include "gf_crtc.h"

#define GF_SPLICE_CURSOR_PLANE 4

#define GF_SPLICE_PRIMARY_PLANE 0

#define GF_SPLICE_PLANE_NUM 1

#define GF_DRM_TIMESTAMP_MAXRETRIES 3

#define GF_SPLICE_CURSOR_ACTIVE 1

typedef struct gf_splice_source_cfg
{
    bool enable;
    unsigned int crtc_x;
    unsigned int crtc_y;
    unsigned int crtc_w;
    unsigned int crtc_h;
    struct drm_display_mode drm_mode;
    struct drm_display_mode hw_mode;
}gf_splice_source_cfg_t;

typedef struct gf_splice_source
{
    unsigned int output_type;
    unsigned int cursor_status;
    struct drm_connector *connector;
    gf_splice_source_cfg_t config;
    gf_splice_source_cfg_t restored_config;
    bool has_restored;
    unsigned int last_vblank;
}gf_splice_source_t;

typedef struct gf_splice_target_cfg
{
    unsigned int mode_x;
    unsigned int mode_y;
    unsigned int mode_rate;
    unsigned int vblank_source;
}gf_splice_target_cfg_t;

typedef struct gf_splice_target
{
    struct drm_connector *connector;
    struct drm_crtc *crtc;
    unsigned int crtc_index;
    gf_splice_target_cfg_t config;
}gf_splice_target_t;

typedef struct gf_splice_manager
{
    gf_splice_source_t *sources;
    unsigned int source_num;

    gf_splice_target_t target;

    int splice_enable;
    void *private;

    int trigger_value;
    struct work_struct splice_trigger_work;
}gf_splice_manager_t;

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 19, 0)

static inline void gf_splice_duplicate_connector_state(struct drm_connector_state *new_state, struct drm_connector_state *old_state)
{
    gf_connector_state_t *gf_new_state = to_gf_conn_state(new_state);
    gf_connector_state_t *gf_old_state = to_gf_conn_state(old_state);

    gf_new_state->splice_active = gf_old_state->splice_active;
    gf_new_state->splice_mode_x = gf_old_state->splice_mode_x;
    gf_new_state->splice_mode_y = gf_old_state->splice_mode_y;
    gf_new_state->splice_mode_rate = gf_old_state->splice_mode_rate;
    gf_new_state->splice_crtc_x = gf_old_state->splice_crtc_x;
    gf_new_state->splice_crtc_y = gf_old_state->splice_crtc_y;
    gf_new_state->splice_crtc_w = gf_old_state->splice_crtc_w;
    gf_new_state->splice_crtc_h = gf_old_state->splice_crtc_h;
    gf_new_state->splice_trigger = gf_old_state->splice_trigger;
}

static inline int gf_splice_set_connector_property(struct drm_connector_state *state,
                                                   struct drm_property *property,
                                                   uint64_t val)
{
    gf_connector_state_t* gf_state = to_gf_conn_state(state);
    struct drm_device *dev = state->connector->dev;
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    int ret = 0;

    if (property == disp_info->splice_active_prop && to_gf_connector(state->connector)->output_type != DISP_OUTPUT_SPLICE)
    {
        gf_state->splice_active = val;
    }
    else if (property == disp_info->mode_x_prop)
    {
        gf_state->splice_mode_x = val;
    }
    else if (property == disp_info->mode_y_prop)
    {
        gf_state->splice_mode_y = val;
    }
    else if (property == disp_info->mode_rate_prop)
    {
        gf_state->splice_mode_rate = val;
    }
    else if (property == disp_info->crtc_x_prop)
    {
        gf_state->splice_crtc_x = val;
    }
    else if (property == disp_info->crtc_y_prop)
    {
        gf_state->splice_crtc_y = val;
    }
    else if (property == disp_info->crtc_h_prop)
    {
        gf_state->splice_crtc_h = val;
    }
    else if (property == disp_info->crtc_w_prop)
    {
        gf_state->splice_crtc_w = val;
    }
    else if(property == disp_info->splice_trigger_prop && to_gf_connector(state->connector)->output_type == DISP_OUTPUT_SPLICE)
    {
        gf_state->splice_trigger = val;
    }
    else
    {
        ret = -EINVAL;
    }

    return ret;
}

static inline int gf_splice_get_connector_property(const struct drm_connector_state *state,
                                                   struct drm_property *property,
                                                   uint64_t *val)
{
    gf_connector_state_t* gf_state = to_gf_conn_state(state);
    struct drm_device *dev = state->connector->dev;
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    int ret = 0;

    if (property == disp_info->splice_active_prop)
    {
        *val = gf_state->splice_active;
    }
    else if (property == disp_info->mode_x_prop)
    {
        *val = gf_state->splice_mode_x;
    }
    else if (property == disp_info->mode_y_prop)
    {
        *val = gf_state->splice_mode_y;
    }
    else if (property == disp_info->mode_rate_prop)
    {
        *val = gf_state->splice_mode_rate;
    }
    else if (property == disp_info->crtc_x_prop)
    {
        *val = gf_state->splice_crtc_x;
    }
    else if (property == disp_info->crtc_y_prop)
    {
        *val = gf_state->splice_crtc_y;
    }
    else if (property == disp_info->crtc_w_prop)
    {
        *val = gf_state->splice_crtc_w;
    }
    else if (property == disp_info->crtc_h_prop)
    {
        *val = gf_state->splice_crtc_h;
    }
    else if(property == disp_info->splice_trigger_prop)
    {
        *val = gf_state->splice_trigger;
    }
    else
    {
        ret = -EINVAL;
    }

    return ret;
}

static inline bool is_splice_target_crtc(struct drm_device *dev, pipe_t pipe)
{
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_splice_manager_t *splice_manager = disp_info->splice_manager;
    unsigned int target_crtc_index = splice_manager->target.crtc_index;

    return !!(pipe == target_crtc_index);
}

int gf_splice_manager_init(disp_info_t *disp_info, unsigned int crtc_index);
void gf_splice_manager_deinit(disp_info_t *disp_info);

void gf_atomic_commit_connector_prop(struct drm_device *dev, struct drm_atomic_state *old_state);

struct drm_crtc* gf_splice_get_crtc_by_source(struct drm_device *dev, gf_splice_source_t *source);

bool is_connector_work_in_splice_mode(struct drm_connector *connector);
bool is_crtc_work_in_splice_mode(struct drm_crtc *crtc);
bool is_plane_work_in_splice_mode(struct drm_plane *plane);
bool is_splice_target_active_in_drm(struct drm_device *dev);

void gf_splice_attach_connector_property(struct drm_connector *connector);

#if DRM_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
u32 gf_splice_get_vblank_counter(struct drm_device *dev, pipe_t pipe);
int gf_splice_enable_vblank(struct drm_device *dev, pipe_t pipe);
void gf_splice_disable_vblank(struct drm_device *dev, pipe_t pipe);
#endif

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 13, 0)
int gf_splice_get_vblank_timestamp(struct drm_device *dev,
                                   pipe_t pipe,
                                   int *max_error,
                                   struct timeval *time,
                                   unsigned flags);
#elif DRM_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
bool gf_splice_get_vblank_timestamp(struct drm_device *dev,
                                    pipe_t pipe,
                                    int *max_error,
                                #if DRM_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
                                    ktime_t *vblank_time,
                                #else
                                    struct timeval *vblank_time,
                                #endif
                                    bool in_vblank_irq);
#else
bool gf_splice_get_vblank_timestamp(struct drm_crtc *crtc,
                                    int *max_error,
                                    ktime_t *vblank_time,
                                    bool in_vblank_irq);
#endif

int gf_splice_handle_source_status(struct drm_connector *connector);

#else

static inline int gf_splice_manager_init(disp_info_t *disp_info, unsigned int crtc_index)
{
    return 0;
}

static inline void gf_splice_manager_deinit(disp_info_t *disp_info)
{
}

static inline void gf_atomic_commit_connector_prop(struct drm_device *dev, struct drm_atomic_state *old_state)
{
}

static inline struct drm_crtc* gf_splice_get_crtc_by_source(struct drm_device *dev, gf_splice_source_t *source)
{
    return NULL;
}

static inline void gf_splice_attach_connector_property(struct drm_connector *connector)
{
    return;
}

static inline u32 gf_splice_get_vblank_counter(struct drm_device *dev, pipe_t pipe)
{
    return 0;
}

static inline int gf_splice_enable_vblank(struct drm_device *dev, pipe_t pipe)
{
    return 0;
}

static inline void gf_splice_disable_vblank(struct drm_device *dev, pipe_t pipe)
{
    return;
}

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 13, 0)
static inline int gf_splice_get_vblank_timestamp(struct drm_device *dev,
                                                 pipe_t pipe,
                                                 int *max_error,
                                                 struct timeval *time,
                                                 unsigned flags)
{
    return 0;
}
#else
#if DRM_VERSION_CODE < KERNEL_VERSION(5, 7, 0)
static inline bool gf_splice_get_vblank_timestamp(struct drm_device *dev,
                                                  pipe_t pipe,
                                                  int *max_error,
                                        #if DRM_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
                                                  ktime_t *vblank_time,
                                        #else
                                                  struct timeval *vblank_time,
                                        #endif
                                                  bool in_vblank_irq)
{
    return true;
}
#else
static inline bool gf_splice_get_vblank_timestamp(struct drm_crtc *crtc,
                                                  int *max_error,
                                                  ktime_t *vblank_time,
                                                  bool in_vblank_irq)
{

    return true;
}
#endif

#endif

static inline bool is_connector_work_in_splice_mode(struct drm_connector *connector)
{
    return false;
}

static inline bool is_crtc_work_in_splice_mode(struct drm_crtc *crtc)
{
    return false;
}

static inline  bool is_plane_work_in_splice_mode(struct drm_plane *plane)
{
    return false;
}

static inline bool is_splice_target_active_in_drm(struct drm_device *dev)
{
    return false;
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
static inline void gf_splice_duplicate_connector_state(struct drm_connector_state *new_state, struct drm_connector_state *old_state)
{
    return;
}

static inline int gf_splice_set_connector_property(struct drm_connector_state *state,
                                                   struct drm_property *property,
                                                   uint64_t val)
{
    return 0;
}

static inline int gf_splice_get_connector_property(const struct drm_connector_state *state,
                                                   struct drm_property *property,
                                                   uint64_t *val)
{

    return 0;
}
#endif

static inline int gf_splice_handle_source_status(struct drm_connector *connector)
{
    return 0;
}


static inline bool is_splice_target_crtc(struct drm_device *dev, pipe_t pipe)
{
    return false;
}

#endif


#endif

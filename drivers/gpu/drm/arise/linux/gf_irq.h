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

#ifndef  _GF_IRQ_
#define _GF_IRQ_

#include "gf_disp.h"
#include "gf_cbios.h"
#include "gf_crtc.h"

#define PCI_EN_MEM_SPACE 0x02

#define OUTPUT_POLL_PERIOD  (HZ)

typedef struct _irq_chip_funcs
{
	int (*get_intr_enable_mask)(disp_info_t* disp_info);
	void (*set_intr_enable_mask)(disp_info_t* disp_info, int intr_mask);
	void (*enable_msi)(disp_info_t* disp_info);
	void (*disable_msi)(disp_info_t* disp_info);
	int (*disable_interrupt)(disp_info_t* disp_info);
	void (*enable_interrupt)(disp_info_t* disp_info, int intr_mask);
	int (*get_interrupt_mask)(disp_info_t* disp_info);
}irq_chip_funcs_t;

extern irq_chip_funcs_t irq_chip_funcs;

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
u32 gf_get_vblank_counter(struct drm_crtc *crtc);
int  gf_enable_vblank(struct drm_crtc *crtc);
void  gf_disable_vblank(struct drm_crtc *crtc);
#else
u32 gf_get_vblank_counter(struct drm_device *dev, pipe_t pipe);
int  gf_enable_vblank(struct drm_device *dev, pipe_t pipe);
void  gf_disable_vblank(struct drm_device *dev, pipe_t pipe);
#endif

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 13, 0)
int gf_get_vblank_timestamp(struct drm_device *dev, pipe_t pipe,
			 int *max_error, struct timeval *time, unsigned flags);
#endif

int gf_get_crtc_scanoutpos(struct drm_device *dev, unsigned int pipe,
		     unsigned int flags, int *vpos, int *hpos,
		     ktime_t *stime, ktime_t *etime,
		     const struct drm_display_mode *mode);

int gf_get_crtc_scanoutpos_kernel_4_8(struct drm_device *dev, unsigned int pipe,
		     unsigned int flags, int *vpos, int *hpos,
		     ktime_t *stime, ktime_t *etime);

int gf_legacy_get_crtc_scanoutpos(struct drm_device *dev, int pipe, unsigned int flags,
				     int *vpos, int *hpos, ktime_t *stime, ktime_t *etime);

bool gf_get_crtc_scanoutpos_kernel_4_10(struct drm_device *dev, unsigned int pipe,
             bool in_vblank_irq, int *vpos, int *hpos,
             ktime_t *stime, ktime_t *etime,
             const struct drm_display_mode *mode);

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
bool gf_crtc_get_scanout_position(struct drm_crtc *crtc,
                                  bool in_vblank_irq, int *vpos, int *hpos,
                                  ktime_t *stime, ktime_t *etime,
                                  const struct drm_display_mode *mode);
#endif

void  gf_irq_preinstall(struct drm_device *dev);

int  gf_irq_postinstall(struct drm_device *dev);

irqreturn_t gf_irq_handle(int irq, void *arg);

void gf_irq_uninstall (struct drm_device *dev);

void gf_hot_plug_intr_ctrl(disp_info_t* disp_info, unsigned int intr, int enable);

void gf_hotplug_work_func(struct work_struct *work);

void gf_dp_irq_work_func(struct work_struct *work);

void gf_hda_work_func(struct work_struct* work);

void gf_hdcp_work_func(struct work_struct* work);

void gf_output_poll_work_func(struct work_struct *work);

void gf_poll_enable(disp_info_t* disp_info);

void gf_poll_disable(disp_info_t *disp_info);

void gf_disp_enable_interrupt(disp_info_t*  disp_info, int irq_inst_uninst);

void gf_disp_disable_interrupt(disp_info_t*  disp_info, int irq_inst_uninst);

void gf_video_interrupt_handle(disp_info_t*  disp_info, unsigned int video_int_mask);

#endif

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

#ifndef __GF_DRIVER_H
#define __GF_DRIVER_H

#include "gf.h"
#include "gf_def.h"
#include "gf_types.h"
#include "os_interface.h"
#include "kernel_interface.h"
#include "gf_device_debug.h"

#define __STR(x)    #x
#define STR(x)      __STR(x)

typedef struct gf_file gf_file_t;

typedef int (*gf_ioctl_t)(gf_file_t* priv, unsigned int cmd, unsigned long arg);

typedef int (*irq_func_t)(void*);

typedef struct
{
    void              *adapter;
#ifdef GF_PCIE_BUS
    struct pci_dev    *pdev;
#else
    struct platform_device *pdev;
#endif
    struct drm_device *drm_dev;
    adapter_info_t    adapter_info;
    void*             disp_info;
    char              busId[64];
    int               len;
    int               index;

    struct os_mutex   *lock;

    int               support_msi;

    struct tasklet_struct fence_notify;

    int               gfb_enable;
    int               reserved_vmem;

    void                *debugfs_dev;
    struct device       *hwmon_dev;
    struct device       *pci_device;
    struct gf_dma_fence_driver *fence_drv;
    void                *fbdev;
    struct krnl_adapter_init_info_s a_info;
    unsigned long       umd_trace_tags; // usermode trace tags, 0 to disable.
#ifdef __aarch64__    
	unsigned int platform_type;
#endif
}gf_card_t;


struct gf_file
{
    struct drm_file   *parent_file;
    gf_card_t        *card;

    void              *map;
    unsigned int      gpu_device;

    gf_device_debug_info_t     *debug;

    unsigned int      server_index;

    int               hold_lock;
    int               freezable;
    struct os_mutex   *lock;
};

extern char *gf_fb_mode;
extern int   gf_fb;

extern struct class *gf_class;
extern gf_ioctl_t   gf_ioctls[];
extern gf_ioctl_t   gf_ioctls_compat[];

extern int  gf_card_init(gf_card_t *gf, void *pdev);
extern int  gf_card_deinit(gf_card_t *gf);
extern void gf_card_pre_init(gf_card_t *gf, void *pdev);
extern int  gf_init_modeset(struct drm_device *dev);
extern void  gf_deinit_modeset(struct drm_device *dev);
extern int gf_debugfs_crtc_dump(struct seq_file* file, struct drm_device* dev, int index);
extern int gf_debugfs_clock_dump(struct seq_file* file, struct drm_device* dev);
extern int gf_debugfs_displayinfo_dump(struct seq_file* file, struct drm_device* dev);
extern int gf_get_chip_temp(void*  dispi);


extern void gf_interrupt_init(gf_card_t *gf);
extern void gf_interrupt_reinit(gf_card_t *gf);
extern void gf_interrupt_deinit(gf_card_t *gf);

extern int  gf_map_system_io(struct vm_area_struct *vma, gf_map_argu_t *map);
extern int  gf_map_system_ram(struct vm_area_struct *vma, gf_map_argu_t *map);

extern void gf_enable_interrupt(void *pdev);
extern void gf_disable_interrupt(void *pdev);

extern void gf_init_bus_id(gf_card_t *gf);
extern int  gf_register_driver(void);
extern void gf_unregister_driver(void);
extern int  gf_register_interrupt(gf_card_t *gf, void *isr);
extern void gf_unregister_interrupt(gf_card_t *gf);

extern int  gf_create_device(int card, unsigned int *device);
extern void gf_destroy_device(int card, unsigned int device);


extern int gf_ioctl_wait_chip_idle(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_acquire_aperture(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_release_aperture(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_flush_allocation(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_set_interactive(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_wait_allocation_idle(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_video_get_hw_caps(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_query_info(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_create_device(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_destroy_device(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_begin_perf_event(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_begin_miu_dump_perf_event(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_end_perf_event(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_end_miu_dump_perf_event(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_get_perf_event(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_send_perf_event(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_get_perf_status(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_destroy_context(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_create_di_context(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_destroy_di_context(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_query_chip_id(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_hdcp_op(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_create_fence_sync_object(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_destroy_fence_sync_object(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_wait_fence_sync_object(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_fence_value(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_create_fence_fd(gf_file_t* priv, unsigned int cmd, unsigned long arg);

extern int gf_ioctl_wait_context_idle(gf_file_t* priv, unsigned int cmd, unsigned long arg);

extern int gf_ioctl_set_power_perf_mode(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_gfx_stm(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_get_allocation_state(gf_file_t* priv, unsigned int cmd, unsigned long arg);
extern int gf_mmap(struct file *filp, struct vm_area_struct *vma);
extern int gf_ioctl_cil2_misc(gf_file_t *priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_gem_create_allocation(gf_file_t *priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_gem_create_context(gf_file_t *priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_gem_map_gtt(gf_file_t *priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_gem_begin_cpu_access(gf_file_t *priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_gem_end_cpu_access(gf_file_t *priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_render(gf_file_t *priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_gem_create_resource(gf_file_t *priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_add_hw_ctx_buf(gf_file_t *priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_rm_hw_ctx_buf(gf_file_t *priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_set_miu_reg_list_perf_event(gf_file_t *priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_get_miu_dump_perf_event(gf_file_t *priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_direct_get_miu_dump_perf_event(gf_file_t *priv, unsigned int cmd, unsigned long arg);
extern int gf_ioctl_dvfs_set(gf_file_t *priv, unsigned cmd, unsigned long arg);
extern int gf_ioctl_query_dvfs_clamp(gf_file_t *priv, unsigned cmd, unsigned long arg);
#endif


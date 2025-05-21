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

#define GF_S4_RESUME  0x01
#define GF_SHUT_DOWN 0x02

typedef struct gf_file gf_file_t;

typedef int (*gf_ioctl_t)(struct drm_device *dev, void *data,struct drm_file *filp);

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

    int               reserved_vmem;

    void                *debugfs_dev;
    struct device       *hwmon_dev;
    struct device       *pci_device;
    struct gf_dma_fence_driver *fence_drv;
    void                *fbdev;
    struct krnl_adapter_init_info_s a_info;
    struct proc_dir_entry *procfs_entry;
    struct proc_dir_entry *procfs_gpuinfo_entry;
    unsigned long       umd_trace_tags; // usermode trace tags, 0 to disable.
    struct os_pages_memory *trace_buffer;
    gf_vm_area_t        *trace_buffer_vma;

    unsigned int flags;
    unsigned int fps_count;
    unsigned int rxa_blt_scn_cnt;
    unsigned int misc_control_flag;
    unsigned long       allocation_trace_tags; // allocation trace tags, 0 to disable.
    unsigned int video_irq_info_all;
    int runtime_pm;
    unsigned long long primary_addr[MAX_CORE_CRTCS];
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
extern struct drm_ioctl_desc gf_ioctls[];
extern struct drm_ioctl_desc gf_ioctls_compat[];

extern int  gf_card_init(gf_card_t *gf, void *pdev);
extern int  gf_card_deinit(gf_card_t *gf);
extern int  gf_init_modeset(struct drm_device *dev);
extern void  gf_deinit_modeset(struct drm_device *dev);
extern int gf_debugfs_crtc_dump(struct seq_file* file, struct drm_device* dev, int index);
extern int gf_debugfs_clock_dump(struct seq_file* file, struct drm_device* dev);
extern int gf_debugfs_displayinfo_dump(struct seq_file* file, struct drm_device* dev);
extern int gf_get_chip_temp(void*  dispi);
extern unsigned int gf_calc_double_standard_mclk(unsigned int mclk);
extern void gf_chip_fan_init_legacy(void*  dispi,int index);
extern int gf_get_chip_fanspeed(void*  dispi, int index);
extern int gf_get_chip_temp_legacy(void*  dispi);
extern int gf_get_chip_fanspeed_legacy(void*  dispi, int index);

extern void gf_splice_manager_handle_hpd(struct work_struct* work);

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

extern int gf_mmap(struct file *filp, struct vm_area_struct *vma);

extern int gf_ioctl_wait_chip_idle(struct drm_device *dev, void *data,struct drm_file *filp);

extern int gf_ioctl_wait_allocation_idle(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_query_info(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_create_device(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_destroy_device(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_begin_perf_event(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_begin_miu_dump_perf_event(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_end_perf_event(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_end_miu_dump_perf_event(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_get_perf_event(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_send_perf_event(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_get_perf_status(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_create_context(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_destroy_context(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_create_di_context(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_destroy_di_context(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_query_chip_id(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_create_fence_sync_object(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_destroy_fence_sync_object(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_wait_fence_sync_object(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_fence_value(struct drm_device *dev, void *data,struct drm_file *filp);

extern int gf_ioctl_wait_context_idle(struct drm_device *dev, void *data,struct drm_file *filp);

extern int gf_ioctl_get_allocation_state(struct drm_device *dev, void *data,struct drm_file *filp);

extern int gf_ioctl_cil2_misc(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_gem_create_allocation(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_gem_create_context(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_gem_map_gtt(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_gem_begin_cpu_access(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_gem_end_cpu_access(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_render(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_gem_create_resource(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_add_hw_ctx_buf(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_rm_hw_ctx_buf(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_set_miu_reg_list_perf_event(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_get_miu_dump_perf_event(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_direct_get_miu_dump_perf_event(struct drm_device *dev, void *data,struct drm_file *filp);
extern int gf_ioctl_kms_get_pipe_from_crtc(struct drm_device *dev, void *data,struct drm_file *filp);
#endif


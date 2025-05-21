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
#include "os_interface.h"
#include "gf_driver.h"
#include "gf_ioctl.h"
#include "gf_debugfs.h"
#include "gf_fence.h"
#include "gf_irq.h"
#include "gf_fbdev.h"
#include "gf_params.h"


extern int gf_run_on_qt;
extern const struct attribute *gf_os_gpu_info;
extern const struct attribute_group gf_sysfs_group;
extern struct bin_attribute gf_sysfs_trace_attr;

static int gf_init_procfs_entry(gf_card_t *gf);
static int gf_deinit_procfs_entry(gf_card_t *gf);

pgprot_t os_get_pgprot_val(unsigned int *cache_type, pgprot_t old_prot, int io_map)
{
    pgprot_t prot = old_prot;

    if(*cache_type == GF_MEM_UNCACHED)
    {
        prot = pgprot_noncached(old_prot);
    }
    else if(*cache_type == GF_MEM_WRITE_COMBINED)
    {
#ifdef CONFIG_X86
#ifdef CONFIG_X86_PAT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,19,0)
        prot = __pgprot((pgprot_val(old_prot) & ~ _PAGE_CACHE_MASK) | cachemode2protval(_PAGE_CACHE_MODE_WC));
#else
        prot = __pgprot((pgprot_val(old_prot) & ~ _PAGE_CACHE_MASK) | _PAGE_CACHE_WC);
#endif
#else
        prot = pgprot_noncached(old_prot);

        *cache_type = GF_MEM_UNCACHED;
#endif
#else
        prot = pgprot_writecombine(old_prot);
#endif

    }
#ifdef GF_AHB_BUS
    else if(*cache_type == GF_MEM_WRITE_BACK)
    {
        // for snoop path, current linux kennel defalut is not WA+WB
        prot = __pgprot((pgprot_val(old_prot)&(~(L_PTE_MT_MASK))) | (L_PTE_MT_WRITEBACK | L_PTE_MT_WRITEALLOC));
    }
#endif
    return prot;
}

#ifndef __frv__
int gf_map_system_io(struct vm_area_struct* vma, gf_map_argu_t *map)
{
    unsigned int  cache_type = map->flags.cache_type;
#if LINUX_VERSION_CODE >= 0x020612
    vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);
#endif
    vma->vm_page_prot = os_get_pgprot_val(&cache_type, vma->vm_page_prot, 1);

    map->flags.cache_type = cache_type;

    if (remap_pfn_range(vma, vma->vm_start,
                vma->vm_pgoff,
                vma->vm_end - vma->vm_start,
                vma->vm_page_prot))
    {
        return -EAGAIN;
    }

    return 0;
}
#if 1
int gf_map_system_ram(struct vm_area_struct* vma, gf_map_argu_t *map)
{
    struct os_pages_memory *memory = map->memory;
    unsigned long start = vma->vm_start;
    unsigned long pfn;
    unsigned int  cache_type;

    int i;
    int start_page  = _ALIGN_DOWN(map->offset, PAGE_SIZE) / PAGE_SIZE;
    int end_page    = start_page + PAGE_ALIGN(map->size) / PAGE_SIZE;

#if DRM_VERSION_CODE <= 0x02040e
    vma->vm_flags |= VM_LOCKED;
#elif DRM_VERSION_CODE < 0x030700
    vma->vm_flags |= VM_RESERVED;
#else
#if DRM_VERSION_CODE < KERNEL_VERSION(6,3,0)
    vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
#else
    vm_flags_set(vma, VM_DONTEXPAND | VM_DONTDUMP);
#endif
#endif

    cache_type = map->flags.cache_type;

#if LINUX_VERSION_CODE >= 0x020612
    vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);
#endif
    vma->vm_page_prot = os_get_pgprot_val(&cache_type, vma->vm_page_prot, 0);
    for (i = start_page; i < end_page; i++)
    {
        struct page *pg = memory->pages[i];
        pfn = page_to_pfn(pg);
        if(remap_pfn_range(vma, start, pfn, PAGE_SIZE, vma->vm_page_prot))
        {
            return -EAGAIN;
        }
        start += PAGE_SIZE;
    }

    return 0;
}
#endif
#endif

static int gf_notify_fence_interrupt(gf_card_t *gf)
{
    return gf_core_interface->notify_interrupt(gf->adapter, INT_FENCE);
}

void gf_interrupt_init(gf_card_t *gf)
{
    tasklet_init(&gf->fence_notify, (void*)gf_notify_fence_interrupt, (unsigned long)gf);
    tasklet_disable(&gf->fence_notify);
    tasklet_enable(&gf->fence_notify);
}

void gf_interrupt_reinit(gf_card_t *gf)
{
    tasklet_enable(&gf->fence_notify);
}

void gf_interrupt_deinit(gf_card_t *gf)
{
    tasklet_disable(&gf->fence_notify);
    synchronize_irq(to_pci_dev(gf->drm_dev->dev)->irq);
    tasklet_kill(&gf->fence_notify);
}

void gf_enable_interrupt(void *pdev_)
{
    struct pci_dev *pdev = pdev_;
    struct drm_device *dev = pci_get_drvdata(pdev);
    gf_card_t *gf = dev->dev_private;

    gf_disp_enable_interrupt(gf->disp_info, 0);
}

void gf_disable_interrupt(void *pdev_)
{
    struct pci_dev *pdev = pdev_;
    struct drm_device *dev = pci_get_drvdata(pdev);
    gf_card_t *gf = dev->dev_private;

    gf_disp_disable_interrupt(gf->disp_info, 0);
}

static void gf_disable_async_suspend(gf_card_t *gf)
{
    struct pci_dev *pdev = gf->pdev;
    struct pci_dev *pdev_audio = NULL;
    unsigned int devfn_audio;

    pdev->dev.power.async_suspend = 0;

    devfn_audio = PCI_DEVFN(PCI_SLOT(pdev->devfn), 1);
    pdev_audio = pci_get_slot(pdev->bus, devfn_audio);

    if(pdev_audio)
    {
        gf_info("found audio device, devfn = 0x%x \n", pdev_audio->devfn);
        pdev_audio->dev.power.async_suspend = 0;
    }
    else
    {
        gf_error("Can't find audio device.\n");
    }

}

static void gf_allocate_trace_buffer(gf_card_t *gf)
{
    alloc_pages_flags_t alloc_flags = {0};
    gf_map_argu_t      map_argu = {0};
    int i;
    unsigned int page_cnt;

    alloc_flags.fixed_page = 1;
    alloc_flags.need_dma_map = 0;
    alloc_flags.need_zero  = 1;
    alloc_flags.need_flush = 1;
    gf->trace_buffer = gf_allocate_pages_memory_priv(gf->pdev, PAGE_SIZE, PAGE_SIZE, alloc_flags);
    if (!gf->trace_buffer)
    {
        gf->trace_buffer_vma = NULL;
        return;
    }

    map_argu.memory = gf->trace_buffer;
    map_argu.flags.mem_space = GF_MEM_KERNEL;
    map_argu.flags.read_only = false;
    map_argu.flags.mem_type = GF_SYSTEM_RAM;
    map_argu.size = gf->trace_buffer->size;
    map_argu.offset = 0;
    map_argu.flags.cache_type = GF_MEM_WRITE_BACK;
    gf->trace_buffer_vma = gf_map_pages_memory_priv(NULL, &map_argu);

    // gf_info("gf_allocate_trace_buffer: trace_buffer_vma->virt_addr=%px, pfn=0x%x\n",
    //     gf->trace_buffer_vma->virt_addr, page_to_pfn(gf->trace_buffer->pages[0]));
}

static void gf_free_trace_buffer(gf_card_t *gf)
{
    if (gf->trace_buffer_vma)
    {
        gf_unmap_pages_memory_priv(gf->trace_buffer_vma);
        gf->trace_buffer_vma = NULL;
    }
    if (gf->trace_buffer)
    {
        gf_free_pages_memory_priv(gf->pdev, gf->trace_buffer);
        gf->trace_buffer = NULL;
    }
}

static krnl_import_func_list_t gf_export =
{
    .udelay                        = gf_udelay,
    .do_div                        = gf_do_div,
    .msleep                        = gf_msleep,
    .getsecs                       = gf_getsecs,
    .get_nsecs                     = gf_get_nsecs,
    .assert                        = gf_assert,
    .dump_stack                    = gf_dump_stack,
    .copy_from_user                = gf_copy_from_user,
    .copy_to_user                  = gf_copy_to_user,
    .memset                        = gf_memset,
    .memcpy                        = gf_memcpy,
    .memcmp_priv                   = gf_memcmp,
    .byte_copy                     = gf_byte_copy,
    .strcmp                        = gf_strcmp,
    .strcpy                        = gf_strcpy,
    .strncmp                       = gf_strncmp,
    .strncpy                       = gf_strncpy,
    .strlen                        = gf_strlen,
    .read64                        = gf_read64,
    .read32                        = gf_read32,
    .read16                        = gf_read16,
    .read8                         = gf_read8,
    .write32                       = gf_write32,
    .write16                       = gf_write16,
    .write8                        = gf_write8,
    .file_open                     = gf_file_open,
    .file_close                    = gf_file_close,
    .file_read                     = gf_file_read,
    .file_write                    = gf_file_write,
    .vsprintf                      = gf_vsprintf,
    .vsnprintf                     = gf_vsnprintf,
    .sscanf                        = gf_sscanf,
    .printk                        = gf_printk,
    .cb_printk                     = gf_cb_printk,
    .seq_printf                    = gf_seq_printf,
    .find_first_zero_bit           = gf_find_first_zero_bit,
    .find_next_zero_bit            = gf_find_next_zero_bit,
    .set_bit                       = gf_set_bit,
    .clear_bit                     = gf_clear_bit,
    .create_thread                 = gf_create_thread,
    .destroy_thread                = gf_destroy_thread,
    .thread_should_stop            = gf_thread_should_stop,
    .create_atomic                 = gf_create_atomic,
    .destroy_atomic                = gf_destroy_atomic,
    .atomic_read                   = gf_atomic_read,
    .atomic_inc                    = gf_atomic_inc,
    .atomic_dec                    = gf_atomic_dec,
    .atomic_add                    = gf_atomic_add,
    .atomic_sub                    = gf_atomic_sub,
    .create_mutex                  = gf_create_mutex,
    .destroy_mutex                 = gf_destroy_mutex,
    .mutex_lock                    = gf_mutex_lock,
    .mutex_lock_killable           = gf_mutex_lock_killable,
    .mutex_trylock                 = gf_mutex_trylock,
    .mutex_unlock                  = gf_mutex_unlock,
    .create_sema                   = gf_create_sema,
    .destroy_sema                  = gf_destroy_sema,
    .down                          = gf_down,
    .down_trylock                  = gf_down_trylock,
    .up                            = gf_up,
    .create_rwsema                 = gf_create_rwsema,
    .destroy_rwsema                = gf_destroy_rwsema,
    .down_read                     = gf_down_read,
    .down_write                    = gf_down_write,
    .up_read                       = gf_up_read,
    .up_write                      = gf_up_write,
    .create_spinlock               = gf_create_spinlock,
    .destroy_spinlock              = gf_destroy_spinlock,
    .spin_lock                     = gf_spin_lock,
    .spin_try_lock                 = gf_spin_try_lock,
    .spin_unlock                   = gf_spin_unlock,
    .spin_lock_irqsave             = gf_spin_lock_irqsave,
    .spin_unlock_irqrestore        = gf_spin_unlock_irqrestore,
    .create_event                  = gf_create_event,
    .destroy_event                 = gf_destroy_event,
    .wait_event_thread_safe        = gf_wait_event_thread_safe,
    .wait_event                    = gf_wait_event,
    .wake_up_event                 = gf_wake_up_event,
    .thread_wait                   = gf_thread_wait,
    .create_wait_queue             = gf_create_wait_queue,
    .thread_wake_up                = gf_thread_wake_up,
    .try_to_freeze                 = gf_try_to_freeze,
    .freezable                     = gf_freezable,
    .clear_freezable               = gf_clear_freezable,
    .set_freezable                 = gf_set_freezable,
    .freezing                      = gf_freezing,
    .get_current_pid               = gf_get_current_pid,
    .get_current_tid               = gf_get_current_tid,
    .get_current_pname             = gf_get_current_pname,
    .flush_cache                   = gf_flush_cache,
    .inv_cache                     = gf_inv_cache,
    .pages_memory_for_each_continues = gf_pages_memory_for_each_continues,
    .ioremap                       = gf_ioremap,
    .iounmap_priv                  = gf_iounmap,
    .mtrr_add                      = gf_mtrr_add,
    .mtrr_del                      = gf_mtrr_del,
    .get_mem_info                  = gf_get_mem_info,
    .is_own_pages                  = gf_is_own_pages,
    .pages_memory_swapout          = gf_pages_memory_swapout,
    .pages_memory_swapin           = gf_pages_memory_swapin,
    .release_file_storage          = gf_release_file_storage,
    .get_bus_config                = gf_get_bus_config,
    .get_platform_config           = gf_get_platform_config,

    .malloc_track                  = gf_malloc_track,
    .calloc_track                  = gf_calloc_track,
    .free_track                    = gf_free_track,
    .malloc_priv                   = gf_malloc_priv,
    .calloc_priv                   = gf_calloc_priv,
    .free_priv                     = gf_free_priv,
    .allocate_pages_memory_track   = gf_allocate_pages_memory_track,
    .free_pages_memory_track       = gf_free_pages_memory_track,
    .allocate_pages_memory_priv    = gf_allocate_pages_memory_priv,
    .free_pages_memory_priv        = gf_free_pages_memory_priv,

    .map_pages_memory_track        = gf_map_pages_memory_track,
    .unmap_pages_memory_track      = gf_unmap_pages_memory_track,
    .map_pages_memory_priv         = gf_map_pages_memory_priv,
    .unmap_pages_memory_priv       = gf_unmap_pages_memory_priv,
    .map_io_memory_track           = gf_map_io_memory_track,
    .unmap_io_memory_track         = gf_unmap_io_memory_track,
    .map_io_memory_priv            = gf_map_io_memory_priv,
    .unmap_io_memory_priv          = gf_unmap_io_memory_priv,
    .register_trace_events         = gf_register_trace_events,
    .unregister_trace_events       = gf_unregister_trace_events,
    .task_create_trace_event       = gf_task_create_trace_event,
    .task_submit_trace_event       = gf_task_submit_trace_event,
    .fence_back_trace_event        = gf_fence_back_trace_event,
    .begin_section_trace_event     = gf_begin_section_trace_event,
    .end_section_trace_event       = gf_end_section_trace_event,
    .counter_trace_event           = gf_counter_trace_event,

    .query_platform_caps           = gf_query_platform_caps,

    .console_lock                  = gf_console_lock,
    .enable_interrupt              = gf_enable_interrupt,
    .disable_interrupt             = gf_disable_interrupt,
    .os_printf                     = gf_printf,
    .disp_wait_idle                = gf_disp_wait_idle,
    .mb                            = gf_mb,
    .rmb                           = gf_rmb,
    .wmb                           = gf_wmb,
    .flush_wc                      = gf_flush_wc,
    .dsb                           = gf_dsb,
};

/***/
#include <linux/proc_fs.h>

#ifdef NO_PROC_CREATE_FUNC
static int gf_read_proc(char *page, char **start, off_t off,
               int count, int *eof, void *data)
{

    gf_card_t    *gf = data;
    struct os_printer p = gf_info_printer(gf);
    gf_core_interface->dump_resource(&p, gf->adapter, 5, 0);

    return 0;
}

static int gf_gpuinfo_read_proc(char *page, char **start, off_t off,
               int count, int *eof, void *data)
{
    return 0;
}

#else
static ssize_t gf_proc_read(struct file *filp, char *buf, size_t count, loff_t *offp)
{
#if DRM_VERSION_CODE < KERNEL_VERSION(5, 17, 0)
    gf_card_t *gf = PDE_DATA(file_inode(filp));
#else
    gf_card_t *gf = pde_data(file_inode(filp));
#endif
    struct os_printer p = gf_info_printer(gf);
    gf_core_interface->dump_resource(&p, gf->adapter, 1, 0);

    return 0;
}

extern int hwq_get_hwq_info(void *adp, gf_hwq_info *hwq_info);
static ssize_t gf_gpuinfo_proc_read(struct file *filp, char *buf, size_t count, loff_t *offp)
{
#if DRM_VERSION_CODE < KERNEL_VERSION(5, 17, 0)
    gf_card_t *gf = PDE_DATA(file_inode(filp));
#else
    gf_card_t *gf = pde_data(file_inode(filp));
#endif

    gf_hwq_info hwq_info;
    gf_query_info_t query_info;
    bus_config_t bus_config;
    struct pci_dev *pdev = gf->pdev;
    adapter_info_t *adapter_info = &gf->adapter_info;
    disp_info_t *disp_info = (disp_info_t *)gf->disp_info;

    int ret = 0;
    size_t len = 0;
    char *buffer = NULL, *memory_type = "DDR4", *pmp_version = NULL, *product_name = NULL, *type_dp = "\0";
    char *output_type_vga_hdmi = "VGA, HDMI", *output_type_full = "VGA, HDMI, DP", *hdmi_resolution = "3840 x 2160", *vga_resolution = "1920 x 1080", *output_type = output_type_vga_hdmi;
    unsigned int mclk, coreclk, eclk, free_mem, mem_usage, usage_3d, usage_vcp, usage_vpp;
    unsigned int subsystemid, technology, pixel_fillrate, texture_fillrate;
    unsigned int pci_gen, pci_width;
    unsigned int memory_width = 64, output_cnt = 2, hdmi_fps;
    int temp, vbios_version = 0, pmp_datelen = 0, pmp_timelen = 0;
    unsigned char *fwname = NULL;
    int fw_version = 0;

    if (!offp || *offp)
        goto exit;

    buffer = gf_calloc(1024);
    if (!buffer)
        goto exit_nomem;

    gf_core_interface->ctl_flags_set(gf->adapter, 1, (1UL << 16), (1 << 16));
    gf_core_interface->ctl_flags_set(gf->adapter, 1, (1UL << 9), (1 << 9));

    gf_get_bus_config(pdev, &bus_config);
    pci_width = (bus_config.link_status >> 4) & 0x1F;
    pci_gen = bus_config.link_status & 0xF;

    if(DISP_OK == disp_cbios_get_clock(disp_info, GF_QUERY_MCLK, &mclk))
        mclk = gf_calc_double_standard_mclk(mclk);
    else
        mclk = 0;

    if(DISP_OK == disp_cbios_get_clock(disp_info, GF_QUERY_CORE_CLOCK, &coreclk))
        coreclk /= 1000;
    else
        coreclk = 0;

    if(DISP_OK == disp_cbios_get_clock(disp_info, GF_QUERY_ENGINE_CLOCK, &eclk))
        eclk /= 1000;
    else
        eclk = 0;

    query_info.type = GF_QUERY_SEGMENT_FREE_SIZE;
    query_info.argu = 1;//low
    gf_core_interface->query_info(gf->adapter, &query_info);
    free_mem = query_info.signed_value;

    query_info.type = GF_QUERY_SEGMENT_FREE_SIZE;
    query_info.argu = 4;//high 4G-
    gf_core_interface->query_info(gf->adapter, &query_info);
    free_mem += query_info.signed_value;

    query_info.type = GF_QUERY_SEGMENT_FREE_SIZE;
    query_info.argu = 5;//high 4G+
    gf_core_interface->query_info(gf->adapter, &query_info);
    free_mem += query_info.signed_value;

    free_mem /= 1024;

    mem_usage = ((adapter_info->total_mem_size_mb - free_mem) * 100) / adapter_info->total_mem_size_mb;

    memory_width = adapter_info->chan_num * memory_width;

    temp = gf_get_chip_temp(gf->disp_info);

    ret = hwq_get_hwq_info(gf->adapter, &hwq_info);
    if (!ret)
    {
        usage_3d = hwq_info.Usage_3D;
        usage_vcp = hwq_info.Usage_VCP;
        usage_vpp = hwq_info.Usage_VPP;
    }
    else
    {
        usage_3d = usage_vcp = usage_vpp = 0;
    }

    switch (pdev->device)
    {
    case 0x3d00:
        hdmi_fps = 60;
        subsystemid = 0x10C0;
        technology = 28;
        pixel_fillrate = 96 * eclk;
        texture_fillrate = pixel_fillrate *2;
        product_name = "Arise-GT10C0";
        break;

    case 0x3d02:
        hdmi_fps = 30;
        subsystemid = 0x1020;
        technology = 28;
        pixel_fillrate = 16 * eclk;
        texture_fillrate = pixel_fillrate *2;
        product_name = "Arise1020";
        break;

    case 0x3d03:
        hdmi_fps = 60;
        subsystemid = 0x1040;
        technology = 28;
        pixel_fillrate = 32 * eclk;
        texture_fillrate = pixel_fillrate *2;
        product_name = "Arise-GT1040";
        break;

    case 0x3d04:
        hdmi_fps = 30;
        subsystemid = 0x1010;
        technology = 28;
        pixel_fillrate = 8 * eclk;
        texture_fillrate = pixel_fillrate *2;
        product_name = "Arise1010";
        break;

    case 0x3d06:
        hdmi_fps = 60;
        subsystemid = 0x10C0;
        technology = 28;
        pixel_fillrate = 96 * eclk;
        texture_fillrate = pixel_fillrate *2;
        product_name = "Arise-GT10C0t";
        if (adapter_info->chan_num == 3)
        {
            output_cnt = 4;
            output_type = output_type_full;
        }
        break;

    case 0x3d07:
        hdmi_fps = 60;
        subsystemid = 0x2030;
        technology = 28;
        pixel_fillrate = 32 * eclk;
        texture_fillrate = pixel_fillrate *2;
        product_name = "Arise2030";
        output_cnt = 3;
        output_type = output_type_full;
        type_dp = "/DP";
        break;

    case 0x3d08:
        hdmi_fps = 60;
        subsystemid = 0x2020;
        technology = 28;
        pixel_fillrate = 32 * eclk;
        texture_fillrate = pixel_fillrate *2;
        product_name = "Arise2020";
        output_cnt = 3;
        output_type = output_type_full;
        type_dp = "/DP";
        break;

    case 0x3d0e:
        hdmi_fps = 60;
        subsystemid = 0x10D0;
        technology = 28;
        pixel_fillrate = 96 * eclk;
        texture_fillrate = pixel_fillrate *2;
        product_name = "Arise10D0";
        break;

    default:
        hdmi_fps = 60;
        subsystemid = 0;
        technology = 0;
        pixel_fillrate = 0;
        texture_fillrate = 0;
        product_name = "Arise N/A";
        break;
    }

    pmp_version = disp_info->pmp_version;
    vbios_version = disp_info->vbios_version;
    fwname = disp_info->firmware_name;
    fw_version = disp_info->firmware_version;

    if (pmp_version)
    {
        pmp_datelen = gf_strlen(pmp_version) + 1;
        pmp_timelen = gf_strlen(pmp_version + pmp_datelen) + 1;
    }

    len += sprintf(buffer + len, "Vendor : %s\n", DRIVER_VENDOR);
    len += sprintf(buffer + len, "Product Name : %s\n", product_name);
    len += sprintf(buffer + len, "Vendor ID : %X\n", pdev->vendor);
    len += sprintf(buffer + len, "Device ID : %X\n", pdev->device);
    //len += sprintf(buffer + len, "Subsystemid : %X\n", subsystemid);
    len += sprintf(buffer + len, "Technology : %u nm\n", technology);
    len += sprintf(buffer + len, "Bus Type : PCIE%d.0 x%u\n", pci_gen, pci_width);
    len += sprintf(buffer + len, "Driver Version : %02x.%02x.%02x%s\n", DRIVER_MAJOR, DRIVER_MINOR, DRIVER_PATCHLEVEL, DRIVER_CLASS);
    if (*fwname)
    {
        len += sprintf(buffer + len, "Firmware Version : %02x.%02x.%02x.%02x\n", (fw_version>>24)&0xff, (fw_version>>16)&0xff, (fw_version>>8)&0xff, fw_version&0xff);
        len += sprintf(buffer + len, "Firmware Name : %s\n", fwname);
    }

    len += sprintf(buffer + len, "Pixel Fillrate : %d MPixel/s\n", pixel_fillrate);
    len += sprintf(buffer + len, "Texture Fillrate : %d MTexel/s\n", texture_fillrate);
    len += sprintf(buffer + len, "Memory Type : %s\n", memory_type);
    len += sprintf(buffer + len, "Memory Width : %d bits\n", memory_width);
    len += sprintf(buffer + len, "Memory Size : %d MB\n", adapter_info->total_mem_size_mb);
    len += sprintf(buffer + len, "Memory ddr Remain Size : %d MB\n", free_mem);
    len += sprintf(buffer + len, "Memory Clock Freq : %d MHz\n", mclk / 2);
    len += sprintf(buffer + len, "Memory Transfer Rates : %d MT/s\n", mclk);
    len += sprintf(buffer + len, "GPU Work Frequency : %d MHz\n", coreclk);
    len += sprintf(buffer + len, "Realtime Temperature : %d Degree\n", temp);
    len += sprintf(buffer + len, "Max Display Port : %d\n", output_cnt);
    len += sprintf(buffer + len, "Support Display Type : %s\n", output_type);
    len += sprintf(buffer + len, "HDMI%s Max Display Resolution : %s@%dHz\n", type_dp, hdmi_resolution, hdmi_fps);
    len += sprintf(buffer + len, "VGA Max Display Resolution : %s@60Hz\n", vga_resolution);

    len += sprintf(buffer + len, "GPU Memory Usage : %d%%\n", mem_usage);
    len += sprintf(buffer + len, "GPU Use Rate(3d) : %d%%\n", usage_3d);
    len += sprintf(buffer + len, "GPU Use Rate(video) : %d%%\n", usage_vcp);
    len += sprintf(buffer + len, "GPU Use Rate(vpp) : %d%%\n", usage_vpp);

    ret = copy_to_user(buf, buffer, len);
    gf_free(buffer);

    if (ret)
        goto exit_err;

    *offp += len;

    return len;

exit_err:
    return -EFAULT;

exit_nomem:
    return -ENOMEM;

exit:
    return 0;
}

#define GF_DUMP_FENCE_INDEX           0x5000
#define GF_CLEAR_FENCE_INDEX          0x6000
#define GF_FORCE_WAKEUP               0x7000

static ssize_t gf_proc_write(struct file *filp, const char __user *buf, size_t count, loff_t *offp)
{
#if DRM_VERSION_CODE < KERNEL_VERSION(5, 17, 0)
    gf_card_t *gf = PDE_DATA(file_inode(filp));
#else
    gf_card_t *gf = pde_data(file_inode(filp));
#endif

    char mode = '0';
    int dump_index = 0;
    unsigned int ret = 0;
    char temp[20] = {0};
    unsigned int crtc = 0;
    unsigned long value = 0;

    //maybe later we can change it to using gf_seq_file_printer
    struct os_printer p = gf_info_printer(gf);

    if(count > 1 && count < 20)
    {
        ret = copy_from_user(&temp[0], buf, count);
        temp[count-1] = '\0';

        if(count == 2)
        {
            mode = temp[0];

            if(mode >= '0' && mode <= '9')
            {
                dump_index = (mode - '0');
            }
            else if((mode >= 'a' && mode < 'e'))
            {
                dump_index =  mode - 'a' + 0xa;
            }
            else if (mode == 'f')
            {
                dump_index = GF_DUMP_FENCE_INDEX;
            }
            else if (mode == 'g')
            {
                dump_index = GF_CLEAR_FENCE_INDEX;
            }
            else if (mode == 'w')
            {
                dump_index = GF_FORCE_WAKEUP;
            }
            else
            {
               gf_error("echo error proc mode, should between 0 and d \n");
               return -EFAULT;
            }
        }
        else
        {
            value = simple_strtoul(&temp[1], NULL, 16);

            gf_info("value %d \n", value);
         }
    }
    else
    {
        gf_error("proc write pass too long param, should less than 20\n");
    }

    if (dump_index == GF_DUMP_FENCE_INDEX)
    {
        gf_dma_track_fences_dump(gf);
    }
    else if (dump_index == GF_CLEAR_FENCE_INDEX)
    {
        gf_dma_track_fences_clear(gf);
    }
    else if((dump_index >=0 && dump_index <=7))
    {
        gf_core_interface->dump_resource(&p, gf->adapter, dump_index, 0);
    }
    else if (dump_index == GF_FORCE_WAKEUP)
    {
        gf_core_interface->notify_interrupt(gf->adapter, INT_FENCE);
        gf_core_interface->dump_resource(&p, gf->adapter, dump_index, 0);
    }

    return count;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
static const struct proc_ops gf_proc_fops =
{
    .proc_read  = gf_proc_read,
    .proc_write = gf_proc_write,
};

static const struct proc_ops gf_gpuinfo_proc_fops =
{
    .proc_read  = gf_gpuinfo_proc_read,
};

#else
static const struct file_operations gf_proc_fops =
{
    .owner = THIS_MODULE,
    .read  = gf_proc_read,
    .write = gf_proc_write,
};

static const struct file_operations gf_gpuinfo_proc_fops =
{
    .owner = THIS_MODULE,
    .read  = gf_gpuinfo_proc_read,
};

#endif
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)

static int temperature_mode = 0;
static ssize_t gf_show_temperature(struct device *dev,
                      struct device_attribute *attr,
                      char *buf)
{
    gf_card_t *gf_card = dev_get_drvdata(dev);
    int temp = 0;

    if (!temperature_mode)
        temp = gf_get_chip_temp(gf_card->disp_info);
    else
        temp = gf_get_chip_temp_legacy(gf_card->disp_info);

    return snprintf(buf, PAGE_SIZE, "%d *C\n", temp);
}

static ssize_t gf_temperature_mode_set(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf,
                    size_t count)
{
    unsigned int enable = 0;
    int ret;

    ret = sscanf(buf, "%u", &enable);
    if (ret <= 0)
        return -EINVAL;

    if (enable == 1)
    {
        temperature_mode = 1;
        gf_info("set temperature mode to legacy\n");
    }
    else
    {
        temperature_mode = 0;
        gf_info("set temperature mode to normal\n");
    }

    return count;
}

static int fan_mode = 0;
static ssize_t gf_show_fan0_input(struct device *dev,
                      struct device_attribute *attr,
                      char *buf)
{
    gf_card_t *gf_card = dev_get_drvdata(dev);
    static int fanspeed = 0, initflag = 1;
    static ktime_t stime, etime;

    if (!fan_mode)
    {
        fanspeed = gf_get_chip_fanspeed(gf_card->disp_info, 0);
    }
    else
    {
        etime = ktime_get();
        if (ktime_to_ns(stime) == 0)
            stime = etime;

        initflag = 0;

        gf_chip_fan_init_legacy(gf_card->disp_info, 0);

        //Read Fan speed interval
        if (ktime_ms_delta(etime, stime) > 100)
        {
            fanspeed = gf_get_chip_fanspeed_legacy(gf_card->disp_info, 0);
            stime = etime;
        }
    }

    return snprintf(buf, PAGE_SIZE, "%d %%\n", fanspeed);
}

static ssize_t gf_show_fan1_input(struct device *dev,
                      struct device_attribute *attr,
                      char *buf)
{
    gf_card_t *gf_card = dev_get_drvdata(dev);
    static int fanspeed = 0, initflag = 1;
    static ktime_t stime, etime;

    if (!fan_mode)
    {
        fanspeed = gf_get_chip_fanspeed(gf_card->disp_info, 1);
    }
    else
    {
        etime = ktime_get();
        if (ktime_to_ns(stime) == 0)
            stime = etime;

        initflag = 0;

        gf_chip_fan_init_legacy(gf_card->disp_info, 1);

        //Read Fan speed interval
        if (ktime_ms_delta(etime, stime) > 100)
        {
            fanspeed = gf_get_chip_fanspeed_legacy(gf_card->disp_info, 1);
            stime = etime;
        }
    }

    return snprintf(buf, PAGE_SIZE, "%d %%\n", fanspeed);
}

static ssize_t gf_fan_mode_set(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf,
                    size_t count)
{
    unsigned int enable = 0;
    int ret;

    ret = sscanf(buf, "%u", &enable);
    if (ret <= 0)
        return -EINVAL;

    if (enable == 1)
    {
        fan_mode = 1;
        gf_info("set fan mode to legacy\n");
    }
    else
    {
        fan_mode = 0;
        gf_info("set fan mode to normal\n");
    }

    return count;
}

static SENSOR_DEVICE_ATTR(temps1_input, S_IRUGO | S_IWUSR, gf_show_temperature, gf_temperature_mode_set, 0);
static SENSOR_DEVICE_ATTR(fans0_input, S_IRUGO | S_IWUSR, gf_show_fan0_input, gf_fan_mode_set, 0);
static SENSOR_DEVICE_ATTR(fans1_input, S_IRUGO | S_IWUSR, gf_show_fan1_input, gf_fan_mode_set, 0);

static struct attribute *gf_hwmon_attrs[] = {
    &sensor_dev_attr_temps1_input.dev_attr.attr,
    &sensor_dev_attr_fans0_input.dev_attr.attr,
    &sensor_dev_attr_fans1_input.dev_attr.attr,
    NULL,
};

ATTRIBUTE_GROUPS(gf_hwmon);

#endif

int gf_card_init(gf_card_t *gf, void *pdev)
{
    int ret = 0;

    gf_init_procfs_entry(gf);

    gf_init_bus_id(gf);

#ifdef GF_PCIE_BUS
    gf->support_msi = 1;
#else
    gf->support_msi = 0;
#endif

    gf->adapter = gf_core_interface->pre_init_adapter(pdev, &gf->a_info, &gf_export);

    if(gf->adapter == NULL)
    {
        ret = -1;

        gf_error("init adapter failed\n");
    }

    gf_init_modeset(gf->drm_dev);

    gf_disable_async_suspend(gf);

    gf_core_interface->init_adapter(gf->adapter, gf->reserved_vmem, gf->disp_info);

#ifndef GF_HW_NULL
    gf_interrupt_init(gf);
#endif

    gf->lock = gf_create_mutex();

    ret = sysfs_create_files(&gf->pdev->dev.kobj, &gf_os_gpu_info);
    if(ret)
    {
        gf_warning("Could not create device attr\n");
    }

    ret = sysfs_create_group(&gf->pdev->dev.kobj, &gf_sysfs_group);
    if(ret)
    {
        gf_warning("Could not create attr group\n");
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
    gf->hwmon_dev = hwmon_device_register_with_groups(&gf->pdev->dev, STR(DRIVER_NAME), gf, gf_hwmon_groups);
    if(!gf->hwmon_dev)
    {
        gf_info("Register hwmon sense failed!\n");
    }
#endif

    gf_allocate_trace_buffer(gf);
    ret = sysfs_create_bin_file(&gf->pdev->dev.kobj, &gf_sysfs_trace_attr);
    if(ret)
    {
        gf_warning("Could not create sysfs trace attr\n");
    }

    gf->fps_count = 0;
    gf->rxa_blt_scn_cnt = 0;
    gf->misc_control_flag = gf_modparams.misc_control_flag;
    gf->allocation_trace_tags = 0;
    gf->video_irq_info_all = 0;
    gf->runtime_pm = gf_modparams.gf_runtime_pm;
    gf->flags = 0;

    return ret;
}

int gf_card_deinit(gf_card_t *gf)
{
    char *name;

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
    gf_fbdev_deinit(gf);
#endif

    gf_core_interface->wait_chip_idle(gf->adapter);

    gf_deinit_modeset(gf->drm_dev);

#ifndef GF_HW_NULL
    gf_interrupt_deinit(gf);
#endif

    gf_core_interface->deinit_adapter(gf->adapter);

    gf->len = 0;

    if(gf->lock)
    {
        gf_destroy_mutex(gf->lock);
        gf->lock = NULL;
    }

    if(gf->debugfs_dev)
    {
        gf_debugfs_destroy((gf_debugfs_device_t*)(gf->debugfs_dev));
        gf->debugfs_dev = NULL;
    }

    sysfs_remove_files(&gf->pdev->dev.kobj, &gf_os_gpu_info);

    sysfs_remove_group(&gf->pdev->dev.kobj, &gf_sysfs_group);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
    if(gf->hwmon_dev)
    {
        hwmon_device_unregister(gf->hwmon_dev);
        gf->hwmon_dev = NULL;
    }
#endif

    if(gf->procfs_entry)
    {
        gf_deinit_procfs_entry(gf);
        gf->procfs_entry = NULL;
    }

    sysfs_remove_bin_file(&gf->pdev->dev.kobj, &gf_sysfs_trace_attr);
    gf_free_trace_buffer(gf);

    gf->fps_count = 0;
    return 0;
}

static int gf_init_procfs_entry(gf_card_t *gf)
{
    char name[64] = "";

    gf_vsnprintf(name, 64, "driver/dri%d", gf->index);

#ifdef NO_PROC_CREATE_FUNC
    //create_proce_read_entry is not used any more in Linux 3.10 and above. use proc_create_data instead.
    gf->procfs_entry = create_proc_read_entry(name, 0, NULL, gf_read_proc, gf);

    gf_vsnprintf(name, 64, "gpuinfo_%d", gf->index);
    gf->procfs_gpuinfo_entry = create_proc_read_entry(name, 0, NULL, gf_gpuinfo_read_proc, gf);
#else
    gf->procfs_entry = proc_create_data(name, 0, NULL, &gf_proc_fops, gf);

    gf_vsnprintf(name, 64, "gpuinfo_%d", gf->index);
    gf->procfs_gpuinfo_entry = proc_create_data(name, 0, NULL, &gf_gpuinfo_proc_fops, gf);
    gf_vsnprintf(name, 64, "driver/dri%d", gf->index);
#endif

    gf_info("%s() name-%s, procfs_entry-0x%x\n", GF_FUNC_NAME(__func__), name, gf->procfs_entry);

    return 0;
}

static int gf_deinit_procfs_entry(gf_card_t *gf)
{
    char name[64] = "";

    gf_vsnprintf(name, 64, "driver/dri%d", gf->index);
#ifdef NO_PROC_CREATE_FUNC
    //this path is not verified yet
    remove_proc_entry(name, gf->procfs_entry);

    gf_vsnprintf(name, 64, "gpuinfo_%d", gf->index);
    remove_proc_entry(name, gf->procfs_gpuinfo_entry);
#else
    proc_remove(gf->procfs_entry);
    proc_remove(gf->procfs_gpuinfo_entry);
#endif

    return 0;
}



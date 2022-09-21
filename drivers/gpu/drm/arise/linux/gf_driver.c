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

#include "os_interface.h"
#include "gf_driver.h"
#include "gf_ioctl.h"
#include "gf_debugfs.h"
#include "gf_fence.h"
#include "gf_irq.h"
#include "gf_fbdev.h"


extern int gf_run_on_qt;

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
    unsigned int  cache_type, curr_block_offset;
	
    int i;
    int start_page  = _ALIGN_DOWN(map->offset, PAGE_SIZE) / PAGE_SIZE;
    int end_page    = start_page + PAGE_ALIGN(map->size) / PAGE_SIZE;

    
#if LINUX_VERSION_CODE <= 0x02040e
    vma->vm_flags |= VM_LOCKED;
#elif LINUX_VERSION_CODE < 0x030700
    vma->vm_flags |= VM_RESERVED;
#else
    vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
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
    synchronize_irq(gf->drm_dev->irq);
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

static krnl_import_func_list_t gf_export =
{
    .udelay                        = gf_udelay,
    .begin_timer                   = gf_begin_timer,
    .end_timer                     = gf_end_timer,
    .do_div                        = gf_do_div,
    .msleep                        = gf_msleep,
    .getsecs                       = gf_getsecs,
    .get_nsecs                     = gf_get_nsecs,
    .assert                        = gf_assert,
    .dump_stack                    = gf_dump_stack,
    .copy_from_user                = gf_copy_from_user,
    .copy_to_user                  = gf_copy_to_user,
    .slowbcopy_tobus               = gf_slowbcopy_tobus,
    .slowbcopy_frombus             = gf_slowbcopy_frombus,
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
    .memsetio                      = gf_memsetio,
    .memcpy_fromio                 = gf_memcpy_fromio,
    .memcpy_toio                   = gf_memcpy_toio,
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
    .getsecs                       = gf_getsecs,
    .get_nsecs                     = gf_get_nsecs,
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
    .create_wait_queue         = gf_create_wait_queue,
    .wait_for_events             = thread_wait_for_events,
    .thread_wake_up                = gf_thread_wake_up,
    ._thread_wake_up              = general_thread_wake_up,
    .dump_stack                    = gf_dump_stack,
    .try_to_freeze                 = gf_try_to_freeze,
    .freezable                     = gf_freezable,
    .clear_freezable               = gf_clear_freezable,
    .set_freezable                 = gf_set_freezable,
    .freezing                      = gf_freezing,
    .get_current_pid               = gf_get_current_pid,
    .get_current_tid               = gf_get_current_tid,
    .flush_cache                   = gf_flush_cache,
    .inv_cache                     = gf_inv_cache,
    .pages_memory_for_each_continues = gf_pages_memory_for_each_continues,
    .ioremap                       = gf_ioremap,
    .iounmap_priv                  = gf_iounmap,
    .mtrr_add                      = gf_mtrr_add,
    .mtrr_del                      = gf_mtrr_del,
    .get_mem_info                  = gf_get_mem_info,
    .is_own_pages                  = gf_is_own_pages,
    .register_shrinker             = gf_register_shrinker,
    .unregister_shrinker           = gf_unregister_shrinker,
    .pages_memory_swapout          = gf_pages_memory_swapout,
    .pages_memory_swapin           = gf_pages_memory_swapin,
    .release_file_storage          = gf_release_file_storage,
    .get_bus_config                = gf_get_bus_config,
    .get_platform_config           = gf_get_platform_config,
    .get_command_status16          = gf_get_command_status16,
    .write_command_status16        = gf_write_command_status16,
    .get_command_status32          = gf_get_command_status32,
    .write_command_status32        = gf_write_command_status32,
    .get_rom_start_addr            = gf_get_rom_start_addr,
    .get_rom_save_addr             = gf_get_rom_save_addr,
    .write_rom_save_addr           = gf_write_rom_save_addr,
    .get_bar1                      = gf_get_bar1,
    .logmsg_init                   = gf_logmsg_init,
    .logmsg                        = gf_logmsg,
    .logmsg_deinit                 = gf_logmsg_deinit,

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
#else
static ssize_t gf_proc_read(struct file *filp, char *buf, size_t count, loff_t *offp)
{
    gf_card_t *gf = PDE_DATA(file_inode(filp));
    struct os_printer p = gf_info_printer(gf);
    gf_core_interface->dump_resource(&p, gf->adapter, 1, 0);

    return 0;
}

#define GF_DUMP_FENCE_INDEX           0x5000
#define GF_CLEAR_FENCE_INDEX          0x6000
#define GF_FORCE_WAKEUP               0x7000

#define GF_DUMP_COMPRESS_PS_INDEX    (0x100 + GF_STREAM_PS)
#define GF_DUMP_COMPRESS_SS_INDEX    (0x100 + GF_STREAM_SS)
#define GF_DUMP_COMPRESS_TS_INDEX    (0x100 + GF_STREAM_TS)
#define GF_DUMP_COMPRESS_4S_INDEX    (0x100 + GF_STREAM_4S)
#define GF_DUMP_COMPRESS_5S_INDEX    (0x100 + GF_STREAM_5S)
#define GF_DUMP_COMPRESS_6S_INDEX    (0x100 + GF_STREAM_6S)
#define GF_DUMP_COMPRESS_MAX_INDEX   (0x100 + GF_STREAM_MAX)


static ssize_t gf_proc_write(struct file *filp, const char __user *buf, size_t count, loff_t *offp)
{
    gf_card_t *gf = PDE_DATA(file_inode(filp));

    char mode = '0';
    int dump_index = 0;
    unsigned int ret = 0;
    char temp[20] = {0};
    unsigned int interval = 0, crtc = 0;
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

            if((temp[0] == 'c' || temp[0] == 'C') &&
                    (temp[2] == 's' || temp[2] == 'S'))
            {
                if(gf_strlen(temp) != 4)
                {
                    gf_info("invalid flag buffer dump parameter!\n");
                }

                crtc = simple_strtoul(&temp[1], NULL, 16);
                dump_index = simple_strtoul(&temp[3], NULL, 16);

                dump_index += GF_DUMP_COMPRESS_PS_INDEX;
            }
            else
            {
               gf_error("invalid index\n");
            }
         }
    }
    else
    {
        gf_error("proc write pass too long param, should less then 20\n");
    }

    if (dump_index == GF_DUMP_FENCE_INDEX)
    {
        gf_dma_track_fences_dump(gf);
    }
    else if (dump_index == GF_CLEAR_FENCE_INDEX)
    {
        gf_dma_track_fences_clear(gf);
    }
    else if((dump_index >=0 && dump_index <=5))
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

#else
static const struct file_operations gf_proc_fops =
{
    .owner = THIS_MODULE,
    .read  = gf_proc_read,
    .write = gf_proc_write,
};
#endif
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)

static ssize_t gf_show_temperature(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	gf_card_t *gf_card = dev_get_drvdata(dev);
	int temp = gf_get_chip_temp(gf_card->disp_info);

	return snprintf(buf, PAGE_SIZE, "%d *C\n", temp);
}

static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, gf_show_temperature, NULL, 0);

static struct attribute *gf_hwmon_attrs[] = {
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	NULL,
};

ATTRIBUTE_GROUPS(gf_hwmon);

#endif

void gf_card_pre_init(gf_card_t *gf, void *pdev)
{
    gf->pdev  = pdev;
    
    gf->gfb_enable = 0;
#ifdef CONFIG_FB
    gf->gfb_enable = gf_fb;
#endif

    if(gf->gfb_enable)
    {
         gf_fbdev_disable_vesa(gf);
    }

    return;
}

int gf_card_init(gf_card_t *gf, void *pdev)
{
    unsigned short command = 0;
    int ret = 0, primary;

    char *name = NULL;;
    gf_init_bus_id(gf);

    gf_get_command_status16(pdev, &command);

    primary = (command & 0x01) ? 1 : 0;

    /* we can enable gfb primay and no fb driver enable by default */
    if(!primary)
    {
        gf->gfb_enable = FALSE;
    }

#ifdef GF_PCIE_BUS
    gf->support_msi = 1;
#else
    gf->support_msi = 0;
#endif
#ifdef __aarch64__    
    gf->platform_type=read_cpuid_part_number() ;
#endif
    gf->adapter = gf_core_interface->pre_init_adapter(pdev, &gf->a_info, &gf_export);

    if(gf->adapter == NULL)
    {
        ret = -1;

        gf_error("init adapter failed\n");
    }

    gf_init_modeset(gf->drm_dev);

    gf_core_interface->init_adapter(gf->adapter, gf->reserved_vmem, gf->disp_info);

#ifndef GF_HW_NULL
    gf_interrupt_init(gf);
#endif

    gf->lock = gf_create_mutex();

    gf->debugfs_dev = gf_debugfs_create(gf, gf->drm_dev->primary->debugfs_root);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
    gf->hwmon_dev = hwmon_device_register_with_groups(&gf->pdev->dev, STR(DRIVER_NAME), gf, gf_hwmon_groups);
    if(!gf->hwmon_dev)
    {
        gf_info("Register hwmon sense failed!\n");
    }
#endif

    if(gf->gfb_enable)
    {
        gf_fbdev_init(gf);
    }

    name = (char *)gf_calloc(64);
    gf_vsnprintf(name, 64, "driver/dri%d", gf->index);
#ifdef NO_PROC_CREATE_FUNC
    //create_proce_read_entry is not used any more in Linux 3.10 and above. use proc_create_data instead.
    create_proc_read_entry(name, 0, NULL, gf_read_proc, gf);
#else
    proc_create_data(name, 0, NULL, &gf_proc_fops, gf);
#endif
    gf_free(name);

    return ret;
}

int gf_card_deinit(gf_card_t *gf)
{
    char *name;
    if(gf->gfb_enable)
    {
        gf_fbdev_deinit(gf);
    }

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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
    if(gf->hwmon_dev)
    {
        hwmon_device_unregister(gf->hwmon_dev);
        gf->hwmon_dev = NULL;
    }
#endif
    name = (char *)gf_calloc(64);
    if(name){
        gf_vsnprintf(name, 64, "driver/dri%d", gf->index);
#ifdef NO_PROC_CREATE_FUNC
#else
        remove_proc_entry(name, NULL);
#endif
        gf_free(name);
    }
    return 0;
}


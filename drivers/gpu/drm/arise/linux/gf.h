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

#ifndef __GF_H__
#define __GF_H__

#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/shmem_fs.h>
#include <linux/writeback.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/leds.h>
#include <linux/platform_device.h>
#include <linux/console.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/iommu.h>
#include <linux/pagemap.h>
#include <linux/vmalloc.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/anon_inodes.h>
#include <linux/list.h>
#include <linux/input.h>
#include <linux/rwsem.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/anon_inodes.h>
#include <linux/dma-mapping.h>
#include <linux/types.h>
#include <linux/log2.h>


#ifndef DRM_VERSION_CODE
#define DRM_VERSION_CODE LINUX_VERSION_CODE
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(5,5,0)
#include <linux/mman.h>
#include <drm/drm_device.h>
#include <drm/drm_file.h>
#include <drm/drm_ioctl.h>
#include <drm/drm_print.h>
#include <drm/drm_fourcc.h>
#else
#include <drm/drmP.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,13,0)
#include <linux/set_memory.h>
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4,12,0)
#include <asm/set_memory.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
#include <uapi/linux/sched/types.h>
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4,11,0)
#include <linux/sched/types.h>
#endif

#define __GF_MUTEX_DEBUG__

#ifdef CONFIG_PCI
#include <linux/pci.h>
#endif

#ifdef CONFIG_ARM_AMBA
#include <linux/amba/bus.h>
#endif

#if defined(CONFIG_GENERIC_GPIO)
#include <linux/gpio.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#endif

#ifdef CONFIG_SYNC
#ifndef __aarch64__
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
#include <linux/sync.h>
#elif defined(YHQILIN) && LINUX_VERSION_CODE == KERNEL_VERSION(4,4,131)
#else
#include <../drivers/staging/android/sync.h>
#endif
#endif
#endif

#if defined(__alpha__) || defined(__powerpc__)
#include <asm/pgtable.h>    /* For pte_wrprotect */
#endif

#ifdef __alpha__
#include <asm/current.h>
#endif

#include <asm/io.h>
#include <asm/mman.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/pgalloc.h>
#ifdef CONFIG_MTRR
#include <asm/mtrr.h>
#endif

//#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
#if defined(CONFIG_VGA_ARB)
#include <linux/vgaarb.h>
#endif
//#endif
#endif

#ifdef CONFIG_FB
#include <linux/fb.h>
#endif

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39)
#include <linux/smp_lock.h>    /* For (un)lock_kernel */
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
#include <linux/freezer.h>
#endif

#if defined(__i386__) || defined(__x86_64__)
//#include <asm/i387.h>
#endif
#include <asm/div64.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,41)
#include <linux/tqueue.h>
#else
#include <linux/workqueue.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35)
#define HAS_SET_PAGES_ARRAY_WC
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0)
#define HAS_VM_MMAP
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,25)
#define cap_t(x) (x).cap[0]
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
#define NO_PROC_CREATE_FUNC
#endif


#if LINUX_VERSION_CODE < KERNEL_VERSION(3,6,0)
#define SEQ_PRINTF
#endif

#ifndef IRQF_SHARED
#define IRQF_SHARED SA_SHIRQ
#endif

#ifndef PM_EVENT_SLEEP
#define PM_EVENT_SLEEP PM_EVENT_SUSPEND
#endif


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
#define acquire_console_sem console_lock
#define release_console_sem console_unlock
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35)
#define HAS_SHRINKER
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
#define HAS_SHRINK_CONTROL
#endif 
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)
#define os_shmem_read_mapping_page(mapping, index, data) shmem_read_mapping_page(mapping, index)
#else
#define os_shmem_read_mapping_page(mapping, index, data) read_mapping_page(mapping, index, data)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,6)

#define OS_KM_USER0  0
#define OS_KM_USER1  0

#define os_kmap_atomic(a, b)   kmap_atomic(a)
#define os_kunmap_atomic(a, b) kunmap_atomic(a)

#else

#define OS_KM_USER0  KM_USER0
#define OS_KM_USER1  KM_USER1

#define os_kmap_atomic(a, b)   kmap_atomic(a, b)
#define os_kunmap_atomic(a, b) kunmap_atomic(a, b)

#endif

#define GF_FUNC_NAME(name) (((name)[0] == 'z') && ((name[1] == 'x')))?((name) + 3):(name)

struct gf_dma_addr_t
{
    dma_addr_t dma_addr;
};

struct os_atomic
{
    atomic_t counter;
};

struct os_spinlock
{
    spinlock_t lock;
};

struct os_sema
{
    struct semaphore lock;
};

struct os_rwsema
{
    struct rw_semaphore lock;
};

#ifndef mutex_init
struct os_mutex
{
    struct semaphore lock;
};
#define mutex_init(lock)           sema_init(lock, 1)
#define mutex_destroy(lock)        sema_init(lock, -99)
#define mutex_lock(lock)           down(lock)
#define mutex_trylock(lock)        (down_trylock(lock) ? 0 : 1)
#define mutex_unlock(lock)         up(lock)
#define mutex_lock_killable(lock)  down_killable(lock)
#else
struct os_mutex
{
    struct mutex lock;
};
#endif

struct os_file
{
    struct file  *filp;
};

struct os_seq_file
{
    struct seq_file  *seq_file;
};

#ifndef _ALIGN_DOWN
#define _ALIGN_DOWN(addr, size)   ((addr)&(~((size)-1)))
#endif

struct os_pages_memory
{
    unsigned int size;
    unsigned int page_size;
    unsigned int need_flush:1;
    unsigned int need_zero:1;
    unsigned int fixed_page:1;
    unsigned int page_4K_64K:1;
    unsigned int noswap:1;
    unsigned int dma32:1; //alloc in low 4G phys addr
    unsigned int shared:1;
    unsigned int userptr:1;
    unsigned int has_dma_map:1;
    unsigned int ref_cnt;
    struct sg_table *st;
    unsigned char *cache_type_per_page;
    struct page **pages;    // it's flatten of st
    struct page **pages_temp;   // it's temp workspace, avoid many gf_alloc(page_array)
};

struct os_wait_event
{
    atomic_t           state;
    wait_queue_head_t  wait_queue;
};

struct os_wait_queue
{
    wait_queue_head_t  wait_queue;
};

struct os_shrinker
{
    struct shrinker shrinker;
    void            *shrink_callback_func;
    void            *shrink_callback_argu;
};


struct os_printer
{
 	void (*printfn)(struct os_printer *p, struct va_format *vaf);
	void *arg;    
};


#ifdef CONFIG_X86_PAT
static inline int os_set_page_uc(struct page *pg)
{
    int retval;
#ifdef HAS_SET_PAGES_ARRAY_WC
    retval = set_pages_array_uc(&pg, 1);
#else
    retval = set_memory_uc((unsigned long)page_address(pg), 1);
#endif

    if(retval)
    {
        printk(KERN_ERR "gf" "set page to uc failed.\n");
    }

    return retval;
}

static inline int os_set_page_wc(struct page *pg)
{
    int retval;
#ifdef HAS_SET_PAGES_ARRAY_WC
    retval = set_pages_array_wc(&pg, 1);
#else
    retval = set_memory_wc((unsigned long)page_address(pg), 1);
#endif

    if(retval)
    {
        printk(KERN_ERR "gf" "set page to wc failed.\n");
    }

    return retval;
}

static inline int os_set_page_wb(struct page *pg)
{
    int retval;
#ifdef HAS_SET_PAGES_ARRAY_WC
    retval = set_pages_array_wb(&pg, 1);
#else
    retval = set_memory_wb((unsigned long)page_address(pg), 1);
#endif

    if(retval)
    {
        printk(KERN_ERR "gf" "set page to wb failed.\n");
    }

    return retval;
}
#endif

extern pgprot_t os_get_pgprot_val(unsigned int *cache_type, pgprot_t old_prot, int io_map);
extern struct file *os_shmem_file_setup(const char *name, loff_t size, unsigned long flags);

extern unsigned int svc_system_read_pmu_reg(unsigned int reg);
extern void svc_system_write_pmu_reg(unsigned int reg, unsigned int value);
extern int svc_system_is_secureboot(void);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
static inline struct proc_dir_entry* create_proc_read_entry(const char *name, ...)
{
    return NULL;
}
#endif

#endif


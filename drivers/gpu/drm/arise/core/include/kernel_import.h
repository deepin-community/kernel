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
#ifndef __KERNEL_IMPORT_H__
#define __KERNEL_IMPORT_H__
#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

//enable/disable  mem track
#define GF_MALLOC_TRACK       0
#define GF_ALLOC_PAGE_TRACK   0
#define GF_MAP_PAGES_TRACK    0
#define GF_MAP_IO_TRACK       0
#define GF_MEM_TRACK_RESULT_TO_FILE    0


#define OS_ACCMODE 0x00000003
#define OS_RDONLY  0x00000000
#define OS_WRONLY  0x00000001
#define OS_RDWR    0x00000002
#define OS_CREAT   0x00000100
#define OS_APPEND  0x00002000
#define OS_LARGEFILE 0x00100000

#define DEBUGFS_NODE_DEVICE                         1
#define DEBUGFS_NODE_HEAP                           2
#define DEBUGFS_NODE_INFO                           3
#define DEBUGFS_NODE_MEMTRACK                       4
#define DEBUGFS_NODE_DVFS                           5
#define DEBUGFS_NODE_VIDSCH                         6
#define DEBUGFS_NODE_CG                             7
#define DEBUGFS_NODE_DEBUGBUS                       8
#define DEBUGFS_NODE_SWAP                           9


struct os_seq_file;

typedef struct
{
    unsigned short  vendor_id;
    unsigned short  device_id;
    unsigned short  command;
    unsigned short  status;
    unsigned char   revision_id;
    unsigned char   prog_if;
    unsigned char   sub_class;
    unsigned char   base_class;
    unsigned char   cache_line_size;
    unsigned char   latency_timer;
    unsigned char   header_type;
    unsigned char   bist;
    unsigned short  sub_sys_vendor_id;
    unsigned short  sub_sys_id;
    unsigned short  link_status;
    /*force set to unsigned long long*/
    unsigned long long    reg_start_addr[5];
    unsigned long long    reg_end_addr[5];
    unsigned long long    mem_start_addr[5];
    unsigned long long    mem_end_addr[5];
    unsigned long long    secure_start_addr[5];
    unsigned long long    secure_end_addr[5];
    int             secure_on;
} bus_config_t;

typedef struct
{
    unsigned long totalram; //gf can used system pages num
    unsigned long freeram;
}mem_info_t;

#define GF_CPU_CACHE_UNKNOWN               0
#define GF_CPU_CACHE_VIVT                  1
#define GF_CPU_CACHE_VIPT_ALIASING         2
#define GF_CPU_CACHE_VIPT_NONALIASING      3
#define GF_CPU_CACHE_PIPT                  4

typedef struct
{
    unsigned int dcache_type;
    unsigned int icache_type;
    unsigned int iommu_support;
    unsigned int  page_size;
    unsigned int  page_shift;
    unsigned int  system_need_dma32;
    unsigned int suspend_blt_mode;
}platform_caps_t;


/*****************************************************************************/
typedef struct
{
    void *pdev;
}os_device_t;

typedef int (*condition_func_t)(void *argu);

enum gf_mem_type
{
    GF_SYSTEM_IO             = 0x01,
    GF_SYSTEM_RAM            = 0x02,
};

enum gf_mem_space
{
    GF_MEM_KERNEL = 0x01,
    GF_MEM_USER   = 0x02,
};

enum gf_mem_attribute
{
    GF_MEM_WRITE_BACK     = 0x01,
    GF_MEM_WRITE_THROUGH  = 0x02,
    GF_MEM_WRITE_COMBINED = 0x03,
    GF_MEM_UNCACHED       = 0x04,
};

typedef struct
{
    unsigned int dma32       :1;
    unsigned int need_flush  :1;
    unsigned int need_zero   :1;
    unsigned int fixed_page  :1; /*fixed page size*/
    unsigned int page_4K_64K :1; /*page can allocate 64k*4k, perfer 64k*/
    unsigned int need_dma_map:1;

    int          page_size;      /*page size when fixed page flags set*/
}alloc_pages_flags_t;

typedef struct
{
    union
    {
        struct
        {
            unsigned char mem_type;
            unsigned char mem_space;
            unsigned char cache_type;
            unsigned char read_only  :1;
            unsigned char write_only :1;
        };
        unsigned int  value;
    };
}gf_map_flags_t;

typedef struct
{
    gf_map_flags_t flags;
    union
    {
        struct os_pages_memory *memory;
        unsigned long long     phys_addr;
    };
    unsigned int  offset;
    unsigned long size;
}gf_map_argu_t;

typedef struct __gf_vm_area
{
    gf_map_flags_t flags;
    unsigned int    ref_cnt;
    unsigned int    size;
    unsigned long   owner;
    unsigned int    need_flush_cache;
    void            *virt_addr;
    struct __gf_vm_area *next;
}gf_vm_area_t;

#define GF_LOCKED       0
#define GF_LOCK_FAILED  1

typedef enum
{
    GF_EVENT_UNKNOWN = 0,
    GF_EVENT_BACK    = 1, /* condtion meet event back */
    GF_EVENT_TIMEOUT = 2, /* wait timeout */
    GF_EVENT_SIGNAL  = 3, /* wait interrupt by a signal */
}gf_event_status_t;


typedef enum
{
    WAIT_EVENT0 = 0,
    WAIT_EVENT1 = 1,
    WAIT_EVENT2 = 2,
    WAIT_EVENT3 = 3,
    WAIT_EVENT_MAX = 4,
}wakeup_event;

typedef struct
{
    gf_event_status_t  status;
    wakeup_event       event;
}event_wait_status;

struct general_wait_event
{
    int                  need_queue;
    unsigned int         event;
    struct os_spinlock*  event_lock;
};

typedef int (*util_event_handler_u)(void *data, event_wait_status state);

typedef int (*gf_thread_func_t)(void *data);


typedef struct gf_drm_callback
{
    struct {
        void* (*create)(void *argu, unsigned int engine_index, unsigned long long initialize_value);
        void (*attach_buffer)(void *argu, void *buffer, void *fence, int readonly);
        void (*update_value)(void *argu, void *fence, unsigned long long value);
        void (*release)(void *argu, void *fence);
        void (*notify_event)(void *argu);

        void* (*dma_sync_object_create)(void *driver, void *bo, int write, int engine, void (*callback)(void*), void* arg);
        long (*dma_sync_object_wait)(void *driver, void *sync_obj, unsigned long long timeout);
        void (*dma_sync_object_release)(void *sync_obj);
        int (*dma_sync_object_is_signaled)(void *sync_obj);
        void (*dma_sync_object_dump)(void *sync_obj);
    } fence;

    struct {
        unsigned int (*get_from_handle)(void *file, unsigned int handle);
    } gem;

    struct {
        void (*check_touch_primary) (void* driver, void *bo);
    } kms;
} gf_drm_callback_t;

struct os_printer;
typedef struct
{
    void (*udelay)(unsigned long long usecs_num);
    unsigned long long (*do_div)(unsigned long long x, unsigned long long y);
    void  (*msleep)(int num);
    void  (*getsecs)(long *secs, long *usecs);
    void  (*get_nsecs)(unsigned long long *nsecs);
    void  (*assert)(int match, const char *msg);
    void  (*dump_stack)(void);
    int   (*copy_from_user)(void* to, const void* from, unsigned long size);
    int   (*copy_to_user)(void* to, const void* from, unsigned long size);
    void* (*memset)(void* s, int c, unsigned long count);
    void* (*memcpy)(void* d, const void* s, unsigned long count);
    int   (*memcmp_priv)(const void *s1, const void *s2, unsigned long count);
    void  (*byte_copy)(char* dst, char* src, int len);
    int   (*strcmp)(const char *s1, const char *s2);
    char* (*strcpy)(char *d, const char *s);
    int   (*strncmp)(const char *s1, const char *s2, unsigned long count);
    char* (*strncpy)(char *d, const char *s, unsigned long count);
    unsigned long (*strlen)(char *s);

/****************************** IO access functions*********************************/
    unsigned long long   (*read64)(void* addr);
    unsigned int   (*read32)(void* addr);
    unsigned short (*read16)(void* addr);
    unsigned char  (*read8)(void* addr);

    void  (*write32)(void* addr, unsigned int val);
    void  (*write16)(void* addr, unsigned short val);
    void  (*write8)(void* addr, unsigned char val);

    struct os_file* (*file_open)(const char *path, int flags, unsigned short mode);
    void  (*file_close)(struct os_file *file);
    int   (*file_read)(struct os_file *file, void *buf, unsigned long size, unsigned long long *read_pos);
    int   (*file_write)(struct os_file *file, void *buf, unsigned long size);

/*****************************************************************************/
    int   (*vsprintf)(char *buf, const char *fmt, ...);
    int   (*vsnprintf)(char *buf, unsigned long size, const char *fmt, ...);
    int   (*sscanf)(char *buf, char *fmt, ...);
    void  (*printk)(unsigned int msglevel, const char* fmt, ...);
    void  (*cb_printk)(const char* msg);

    int   (*seq_printf)(struct os_seq_file* file, const char *f, ...);

    void   (*os_printf)(struct os_printer *p, const char *f, ...);

    void* (*malloc_priv)(unsigned long size);
    void* (*calloc_priv)(unsigned long size);
    void  (*free_priv)(void* addr);

    void* (*malloc_track)(unsigned long size, const char *file, unsigned int line);
    void* (*calloc_track)(unsigned long size, const char *file, unsigned int line);
    void  (*free_track)(void* addr, const char *file, unsigned int line);

/* bit ops */
    int   (*find_first_zero_bit)(void *buf, unsigned int size);
    int   (*find_next_zero_bit)(void *buf, unsigned int size, int offset);
    void  (*set_bit)(unsigned int nr, void *buf);
    void  (*clear_bit)(unsigned int nr, void *buf);

    struct os_atomic* (*create_atomic)(int val);
    void  (*destroy_atomic)(struct os_atomic *atomic);
    int   (*atomic_read)(struct os_atomic *atomic);
    void  (*atomic_inc)(struct os_atomic *atomic);
    void  (*atomic_dec)(struct os_atomic *atomic);
    int   (*atomic_add)(struct os_atomic *atomic, int v);
    int   (*atomic_sub)(struct os_atomic *atomic, int v);

    struct os_mutex* (*create_mutex)(void);
    void  (*destroy_mutex)(struct os_mutex *mutex);
    void  (*mutex_lock)(struct os_mutex *mutex);
    int   (*mutex_lock_killable)(struct os_mutex *mutex);
    int   (*mutex_trylock)(struct os_mutex *mutex);
    void  (*mutex_unlock)(struct os_mutex *mutex);

    struct os_sema* (*create_sema)(int val);
    void  (*destroy_sema)(struct os_sema *sem);
    void  (*down)(struct os_sema *sem);
    int   (*down_trylock)(struct os_sema *sem);
    void  (*up)(struct os_sema *sem);

    struct os_rwsema *(*create_rwsema)(void);
    void (*destroy_rwsema)(struct os_rwsema *sem);
    void (*down_read)(struct os_rwsema *sem);
    void (*down_write)(struct os_rwsema *sem);
    void (*up_read)(struct os_rwsema *sem);
    void (*up_write)(struct os_rwsema *sem);

    struct os_spinlock* (*create_spinlock)(int type);
    void  (*destroy_spinlock)(struct os_spinlock *spin);
    void  (*spin_lock)(struct os_spinlock *spin);
    int   (*spin_try_lock)(struct os_spinlock *spin);
    void  (*spin_unlock)(struct os_spinlock *spin);
    unsigned long (*spin_lock_irqsave)(struct os_spinlock *spin);
    void  (*spin_unlock_irqrestore)(struct os_spinlock *spin, unsigned long flags);

    struct os_wait_event* (*create_event)(int task_id);
    void  (*destroy_event)(void *event);
    gf_event_status_t (*wait_event_thread_safe)(struct os_wait_event *event, condition_func_t condition, void *argu, int msec);
    gf_event_status_t (*wait_event)(struct os_wait_event *event, int msec);
    void  (*wake_up_event)(struct os_wait_event *event);

    void* (*create_thread)(gf_thread_func_t func, void *data, const char *thread_name);
    void  (*destroy_thread)(void* thread);
    int   (*thread_should_stop)(void);
    struct os_wait_queue* (*create_wait_queue)(void);

    gf_event_status_t (*thread_wait)(struct os_wait_event *event, int msec);
    void  (*thread_wake_up)(struct os_wait_event *event);

    int   (*try_to_freeze)(void);
    int   (*freezable)(void);
    void  (*clear_freezable)(void);

    void  (*set_freezable)(void);
    int   (*freezing)(void);
    unsigned long (*get_current_pid)(void);
    unsigned long (*get_current_tid)(void);
    void (*get_current_pname)(char *, int);

    void  (*flush_cache)(void *pdev, gf_vm_area_t *vma, struct os_pages_memory* memory, unsigned int offset, unsigned int size);
    void  (*inv_cache)(void *pdev, gf_vm_area_t *vma, struct os_pages_memory* memory, unsigned int offset, unsigned int size);
    struct os_pages_memory* (*allocate_pages_memory_priv)(void *pdev, int size, int page_size, alloc_pages_flags_t alloc_flags);
    void  (*free_pages_memory_priv)(void *pdev, struct os_pages_memory *memory);

    struct os_pages_memory* (*allocate_pages_memory_track)(void *pdev, int size, int page_size, alloc_pages_flags_t alloc_flags, const char *file, unsigned int line);
    void  (*free_pages_memory_track)(void *pdev, struct os_pages_memory *memory, const char *file, unsigned int line);
    void (*pages_memory_for_each_continues)(struct os_pages_memory *memory, void *arg,
        int (*cb)(void *arg, int page_start, int page_cnt, unsigned long long dma_addr));

    gf_vm_area_t *(*map_pages_memory_priv)(void *filp, gf_map_argu_t *map_argu);
    void  (*unmap_pages_memory_priv)(gf_vm_area_t *vm_area);
    gf_vm_area_t *(*map_io_memory_priv)(void *filp, gf_map_argu_t *map_argu);
    void  (*unmap_io_memory_priv)(gf_vm_area_t *vm_area);
    gf_vm_area_t *(*map_pages_memory_track)(void *filp, gf_map_argu_t *map_argu, const char *file, unsigned int line);
    void  (*unmap_pages_memory_track)(gf_vm_area_t *vm_area, const char *file, unsigned int line);
    gf_vm_area_t *(*map_io_memory_track)(void *filp, gf_map_argu_t *map_argu, const char *file, unsigned int line);
    void  (*unmap_io_memory_track)(gf_vm_area_t *vm_area, const char *file, unsigned int line);
    void* (*ioremap)(unsigned int io_base, unsigned int size);
    void  (*iounmap_priv)(void *map_address);

    int   (*mtrr_add)(unsigned long start, unsigned long size);
    int   (*mtrr_del)(int reg, unsigned long base, unsigned long size);

    int   (*get_mem_info)(mem_info_t *mem);

    int    (*is_own_pages)(struct os_pages_memory *pages_memory);
    void* (*pages_memory_swapout)(struct os_pages_memory *pages_memory);
    int   (*pages_memory_swapin)(struct os_pages_memory *pages_memory, void *file);
    void  (*release_file_storage)(void *file);

    void  (*get_bus_config)(void *dev, bus_config_t *bus);
    int   (*get_platform_config)(void *dev, const char* config_name, int *buffer, int length);

    void  (*register_trace_events)(void);
    void  (*unregister_trace_events)(void);
    void  (*task_create_trace_event)(int engine_index, unsigned int context,
                                     unsigned long long task_id, unsigned int task_type);
    void  (*task_submit_trace_event)(int engine_index, unsigned int context,
                                     unsigned long long task_id, unsigned int task_type,
                                     unsigned long long fence_id, unsigned int args);
    void  (*fence_back_trace_event)(int engine_index, unsigned long long fence_id);
    void  (*begin_section_trace_event)(const char* desc);
    void  (*end_section_trace_event)(int result);
    void  (*counter_trace_event)(const char* desc, unsigned long long value);

    int   (*query_platform_caps)(void *pdev, platform_caps_t *caps);

    void  (*enable_interrupt)(void *pdev);
    void  (*disable_interrupt)(void *pdev);
    void  (*console_lock)(int);
    int    (*disp_wait_idle)(void *disp_info);

    /* barrier */
    void (*mb)(void);
    void (*rmb)(void);
    void (*wmb)(void);
    void (*flush_wc)(void);
    void (*dsb)(void);

#ifdef VMI_MODE
    void (*set_pages_addr)(struct os_pages_memory *mem, unsigned long long addr);
#endif
}krnl_import_func_list_t;


#define GF_DRV_DEBUG     0x00
#define GF_DRV_WARNING   0x01
#define GF_DRV_INFO      0x02
#define GF_DRV_ERROR     0x03
#define GF_DRV_EMERG     0x04

#endif /*__KERNEL_IMPORT_H__*/


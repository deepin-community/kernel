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
#ifndef __GF_VIDMM_H__
#define __GF_VIDMM_H__

#include "heap_manager.h"
#include "list.h"
#include "context.h"

#define SEGMENT_ID_INVALID    0x0
#define SEGMENT_ID_LOCAL    0x1

#define MAX_SEGMENT_ID        0xF
#define MAX_APERTURES         0x10

#define FLAG_BUFFER_RANGE_NUM 1

typedef union _vidmm_segment_desc_flags_t
{
    struct
    {
    unsigned int require_system_pages  :1; // if need allocate pages after vm range allocate, since some chip have phys memory on gpu chip,
                                           // these phys_addr same with vm_addr, no need allocate phys memory on this,
                                           // this flags also implicit gpu need gart to access these phys page.
    unsigned int system_phys_mem_reserved :1; //reserved system physical mem
    unsigned int chip_phys_mem_reserved   :1; //reserved on chip physical mem
    unsigned int system_pages_reserved    :1; //system allocated pages mem, will allocated these pages when init
    unsigned int system_pages_un_reserved :1; //normal system mem

    unsigned int cpu_visible           :1; // set if this gpu vm range described phys addr can be mapped to cpu vm range and accessed by CPU
    unsigned int support_aperture      :1; // do this segment support aperture
    unsigned int support_snoop         :1;
    unsigned int support_manual_flush  :1;

    unsigned int mtrr                  :1;  //cpu side feature, if this flag set, we can set this GPU memory range as wc by add_mtrr.
    unsigned int small_heap_available  :1;
    unsigned int page_64kb_enable      :1;
    unsigned int secure_range          :1;
    };
    unsigned int value;
}vidmm_segment_desc_flags_t;


/*
*  segment:
*
*   |<--------------------- gpu_vm_size ------------------->|
*   |<--reserved_vm_size-->|
*   +----------------------+--------------------------------+
*   |       reserved_vmem  |              heap              |
*   +----------------------+--------------------------------+
*   ^                      ^
*   gpu_vm_start          reserved_vm_end
*
*/
typedef struct _vidmm_segment
{
    unsigned int                segment_id;         //segment id
    const char                  *segment_name;
    vidmm_segment_desc_flags_t  flags;
    unsigned int                segment_alignment;      //elite dynamic: 4M, dst/exc/elite pcie/ahb: 4k
    unsigned char               *cpu_vm_start;
    unsigned long long          gpu_vm_start;
    unsigned long long          reserved_vm_end;
    unsigned int                reserved_vm_size;
    unsigned long long          gpu_vm_size;
    unsigned long long          small_heap_size;
    unsigned long long          small_heap_max_allocate_size;

    unsigned long long          phys_addr_start;    //used for reserved system memory for elite

    int                         mtrr_reg; //used in x86 when mtrr bit in flag.
    heap_t                      heap;
    heap_t                      small_heap; //heap used allocate small and unpagable allocation.

    struct list_head            pagable_resident_list[PALL];
    struct list_head            unpagable_resident_list;

    long long                   pagable_size;   /* pagable memory size */
    long long                   unpagable_size; /* unpable memory size */

    long long                   reserved_size;  /* unpagable reserved from this segment */

    int                         pagable_num;   /* pagable alloacation num*/
    int                         unpagable_num; /* unpagable allocation num*/

    struct os_mutex             *lock;
    struct os_mutex             *secure_lock;

    struct os_pages_memory      *reserved_pages;
} vidmm_segment_t;

typedef struct _vidmm_segment_memory
{
   unsigned int            segment_id;
   unsigned long long      gpu_virt_addr;
   unsigned int            snooping_enabled;  // usr this flag to support per page snoop
   list_node_t            *list_node;
   struct os_pages_memory *pages_mem;
   gf_vm_area_t          *vma;
} vidmm_segment_memory_t;

typedef struct vidmm_map_flags
{
    unsigned char mem_space;
    unsigned char cache_type;
    unsigned char read_only  :1;
    unsigned char write_only :1;
}vidmm_map_flags_t;

typedef struct _vidmm_segment_preference
{
    union
    {
        struct
        {
            unsigned int segment_id_0 : 5;                // 0x0000001F
            unsigned int direction_0 :  1;                // 0x00000020
            unsigned int segment_id_1 : 5;                // 0x000007C0
            unsigned int direction_1 :  1;                // 0x00000800
            unsigned int segment_id_2 : 5;                // 0x0001F000
            unsigned int direction_2 :  1;                // 0x00020000
            unsigned int segment_id_3 : 5;                // 0x007C0000
            unsigned int direction_3 :  1;                // 0x00800000
            unsigned int segment_id_4 : 5;                // 0x1F000000
            unsigned int direction_4 :  1;                // 0x20000000
            unsigned int reserved   :   2;                // 0xC0000000
        };
        unsigned int value;
    };
} vidmm_segment_preference_t;

typedef struct _vidmm_allocation_flag
{
    union
    {
        struct
        {
            unsigned int cpu_visible    : 1;
            unsigned int primary        : 1;//primary means must force local
            unsigned int swizzled       : 1;
            unsigned int none_snooping  : 1;
            unsigned int unpagable      : 1;
            unsigned int fence_sync     : 1; /* binding sync object to this allocation */
            unsigned int secured        : 1;
            unsigned int compressed     : 1;
            unsigned int try_secure     : 1;
            unsigned int video          : 1;
            unsigned int camera         : 1;
            unsigned int cacheable      : 1;
            unsigned int force_clear    : 1;//force clear when create
            unsigned int overlay        : 1;// overlay means can't put to snoopable segment
            unsigned int bVideoInternal : 1;//e3k video ALWAYS set this flag for resource assignment indication.
            unsigned int bHwctxBuffer   : 1;//used in aarch64 platform to put 2d hwctx buffer in local heap to improve the performance
        };
        unsigned int value;
    };
} vidmm_allocation_flag_t;

typedef struct _vidmm_allocation_status
{
    union
    {
        struct
        {
            unsigned int   submit_ref_cnt          : 16;
            unsigned int   force_unpagable         : 5;
            unsigned int   wait_for_destroy        : 1;
            unsigned int   reserved0               : 10;
        };

        unsigned int temp_unpagable;
    };
    union
    {
        struct
        {
            unsigned int    need_restore            : 1; /* means local video memory saved to pages_mem, used restore when resume */
            unsigned int    reserved1               :31;
        };
        unsigned int value2;
    };
} vidmm_allocation_status_t;

/* since we support paging, so allocation segment memory things unstable. but how to access it. there are two method.
 * 1. hold paging lock and access. like paging process.
 * 2. first set allocation is temp_unpagable, and then wait paging idle, then the segment members are stable, lock allocation go this way
 */

typedef struct _vidmm_allocation
{
    struct list_head            list_item;               // item may in segment pagable list, unpagable list, accord allocation status
    struct list_head            destroy_list_item;       // use for defer destroy list
    struct list_head            device_ref_list;         // list for which device referenced this allocation;

    unsigned int                handle;
    gpu_device_t                *device;
    unsigned int                alignment;
    unsigned int                orig_size;               // require size from user mode
    unsigned int                size;                    // allocation aligned size
    vidmm_segment_preference_t  preferred_segment_raw;   // raw use store preferred_segment client requested.
    vidmm_segment_preference_t  preferred_segment;       // for some hw limitation, use requested segment we can not use.
                                                         // we need validate the raw, here is the real one used to allocate.
    unsigned int                priority;
    vidmm_allocation_flag_t     flag;                    //unchangeable allocation flags.
    int                         ref_count;
    unsigned int                segment_id;
    unsigned int                backup_segment_id;
    unsigned long long          phys_addr;               // deprecated, we will rename it to gpu_virt_addr. phys_addr is physical address of memory
    unsigned long long          cpu_phys_addr;           // only take effect when allocation is allocated from IO local
    unsigned int                snooping_enabled;        // default snoop disabled, only enable it when use, like when lock.
    struct os_pages_memory      *pages_mem;
    void                        *file_storage;           // shmem file cache
    list_node_t                 *list_node;
    unsigned long long          fence_id[MAX_ENGINE_COUNT];
    unsigned long long          write_fence_id[MAX_ENGINE_COUNT];
    unsigned long long          last_paging;             // last paging fence id
    vidmm_allocation_status_t   status;                  //dynamic changed status during allocation's life
    int                         render_count[MAX_ENGINE_COUNT];
    int                         write_render_count[MAX_ENGINE_COUNT];
    struct os_spinlock          *lock;

    /* chip related attributes */
    unsigned char   compress_format;
    unsigned int    bit_count;
    unsigned int    width;
    unsigned int    height;
    unsigned int    aligned_width;
    unsigned int    aligned_height;
    unsigned int    tiled_width;
    unsigned int    tiled_height;
    unsigned int    pitch;
    unsigned int    hw_format;
    unsigned int    at_type;

    void                *bo;
    unsigned int         sync_obj;    // binding fence sync object to this allocation, this sync obj create/destroy just follow allocation's
    unsigned long long  fence_addr;
    list_node_t         *bl_node;
    unsigned int        slot_index;
    int                 perf_swap_hint;
}vidmm_allocation_t;

typedef struct _vidmm_paging_allocation
{
    list_node_t        *list_node;
    int                segment_id;
    unsigned int       size;
    unsigned long long gpu_virt_addr;

    vidmm_allocation_t *allocation;
    vidmm_allocation_t *temp_allocation;
}vidmm_paging_allocation_t;

typedef struct _vidmm_rename_create_arg
{
    vidmm_allocation_t  *reference;     // input
    unsigned int        hAllocation;    // output
    unsigned long long  Size;           // output
} vidmm_rename_create_t;

typedef struct _vidmm_gf_create_arg
{
    gf_create_allocation_t  *create_data;
    struct os_pages_memory  *import_pages_mem;
} vidmm_gf_create_t;

typedef gf_create_alloc_info_t  vidmm_escape_create_t;

#define VIDMM_CREATE_TYPE_ESCAPE    (1)
#define VIDMM_CREATE_TYPE_GF        (2)
#define VIDMM_CREATE_TYPE_RENAME    (3)

typedef struct _vidmm_create_allocation_arg
{
    unsigned int    create_type;
    unsigned int    allocation_count;

    union {
        vidmm_gf_create_t       *create_list;   // for gf_create
        vidmm_escape_create_t   *info_list;     // for escape_create
        vidmm_rename_create_t   *rename_list;   // for rename_create
    };

    void              **bos;     // input, used to pass gem_obj to core
} vidmm_create_allocation_arg_t;

typedef struct _vidmm_gart_table_info
{
    list_node_t     *list_node;
    unsigned int    segment_id;
    vidmm_segment_memory_t *segment_memory;
    void            *virt_addr;
    void            *pcie_addr;
    unsigned long long phys_addr;
    unsigned long long gart_start_addr; //the start address to be gart
    unsigned int    size;   /* in bytes */
    unsigned int    page_count;
    unsigned int    pcie_count;
    int             dirty;
    unsigned long long    gart_table_dirty_addr;
    unsigned long long    gart_table_dirty_mask;
    unsigned int    page_key_ram[512];
    void            *backup;
    void            *chip_private;
} vidmm_gart_table_info_t;

typedef struct _vidmm_destroy_allocatin_arg
{
    int                allocation_count;
    vidmm_allocation_t **allocation_list;
} vidmm_destroy_allocatin_arg_t;

typedef struct vidmm_get_allocation_info
{
   vidmm_allocation_t    *allocation;
   unsigned long long    gpu_vm_addr;
   unsigned int          tiled;
   unsigned int          pitch;
   unsigned int          snoop;
   unsigned int          segment_id;
   unsigned int          hw_format;
   unsigned int          ForceLocal;
}vidmm_get_allocation_info_t;


extern int  vidmm_init(adapter_t *adapter, int reserved_vmem);
extern void vidmm_destroy(adapter_t *adapter);
extern int vidmm_save(adapter_t *adapter);
extern void vidmm_restore(adapter_t *adapter);
extern int vidmm_save_allocation(gpu_device_t *device, vidmm_allocation_t *allocation);
extern void vidmm_restore_allocation(gpu_device_t *device, vidmm_allocation_t *allocation);
extern int  vidmm_create_allocation(gpu_device_t *device, vidmm_create_allocation_arg_t *data);
extern void vidmm_destroy_allocation(gpu_device_t *device, vidmm_destroy_allocatin_arg_t *data);
extern int vidmm_segment_unresident_allocations(adapter_t *adapter, unsigned int segment_id);
extern int  vidmm_resident_one_allocation(gpu_device_t *device, void *ptask, vidmm_paging_allocation_t *paging_allocation, unsigned int);
extern void vidmm_map_gart_table(adapter_t *adapter, vidmm_allocation_t *allocation, int snooping);
extern void vidmm_destroy_defer_allocation(adapter_t *adapter);
extern void vidmm_try_decrease_heap_size(adapter_t *adapter);
extern void vidmm_validate_allocation_memory(adapter_t*, vidmm_allocation_t *allocation);
extern int vidmm_allocate_super_page(adapter_t *adapter, unsigned int preferred_segment_id, unsigned int size);
extern int vidmm_get_allocation_info(adapter_t *adapter, vidmm_get_allocation_info_t *info);
extern int vidmm_query_info(adapter_t *adapter, gf_query_info_t *info);
extern vidmm_segment_t* vidmm_get_segment_by_id(adapter_t *adapter, unsigned int segment_id);
extern heap_t * vidmm_get_burst_length_heap(adapter_t *adapter);
extern int vidmm_get_segment_count(adapter_t *adapter);
extern vidmm_segment_memory_t *vidmm_allocate_segment_memory(adapter_t *adapter, unsigned int segment_id, unsigned int size, int direction);
extern void vidmm_release_segment_memory(adapter_t *adapter, vidmm_segment_memory_t *segment_memory);
extern void* vidmm_map_segment_memory(adapter_t *adapter, void *filp, vidmm_segment_memory_t *segment_memory, vidmm_map_flags_t *map);
extern void vidmm_unmap_segment_memory(adapter_t *adapter, vidmm_segment_memory_t *segment_memory, enum gf_mem_space mem_space);
extern void vidmm_release_temp_paging_memory(adapter_t *adapter, void *ptask);
extern void vidmm_dump_allocation(adapter_t *adapter);
extern int  vidmm_allocate_mirror_super_page_elt(adapter_t *adapter, unsigned int count, unsigned int segment_id);
extern void vidmm_destroy_mirror_super_page_elt(adapter_t *adapter, unsigned int count, unsigned int segment_id);
extern void vidmm_add_allocation_to_resident_list(adapter_t *adapter, vidmm_allocation_t *allocation);
extern void vidmm_remove_allocation_from_resident_list(adapter_t *adapter, vidmm_allocation_t *allocation);
extern void vidmm_add_allocation_to_cache_list(adapter_t *adapter, vidmm_allocation_t *allocation);
extern void vidmm_remove_allocation_from_cache_list(adapter_t *adapter, vidmm_allocation_t *allocation);
extern void vidmm_dump_resource(adapter_t *adapter);
extern void vidmm_dump_heap(struct os_seq_file *seq_file, adapter_t *adapter, int id);
extern void vidmm_dump_memtrack(struct os_seq_file *seq_file, adapter_t *adapter, int pid);
extern void vidmm_dump_allocation_to_file(char *file, unsigned int offset, unsigned int hAllocation, adapter_t *adapter);
extern void vidmm_dump_allocation_content(vidmm_allocation_t *allocation);
extern void vidmm_get_map_allocation_info(adapter_t *adapter, vidmm_allocation_t *allocation, gf_map_argu_t *map);


extern void vidmm_unreference_allocation(adapter_t *adapter, gpu_device_t *device, vidmm_allocation_t *allocation);

extern void vidmm_reference_allocation(adapter_t *adapter, gpu_device_t *device, vidmm_allocation_t *allocation);

extern void* vidmm_get_from_gem_handle(adapter_t *adapter, gpu_device_t *device, unsigned int gem_handle);
extern void vidmm_mark_pagable(adapter_t *adapter, vidmm_allocation_t *allocation);
extern void vidmm_prepare_and_mark_unpagable(adapter_t *adapter, vidmm_allocation_t *allocation, gf_open_allocation_t *info);
extern void vidmm_fill_allocation_info(adapter_t *adapter, vidmm_allocation_t *allocation, gf_open_allocation_t *info);
extern void vidmm_dump_flagbuffer_to_file(char *file_name, vidmm_allocation_t *allocation);
extern void vidmm_dump_bl_heap(adapter_t *adapter);
#endif


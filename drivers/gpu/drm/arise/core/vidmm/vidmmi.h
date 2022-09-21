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

#ifndef __GF_VIDMMI_H__
#define __GF_VIDMMI_H__

#include "list.h"


typedef struct _vidmm_describe_allocation
{
    vidmm_allocation_t *allocation;

    vidmm_gf_create_t       *create_gf;     // for gf_create
    vidmm_escape_create_t   *create_info;   // for escape_create
} vidmm_describe_allocation_t;

typedef enum _build_paging_buffer_operation
{
    BUILDING_PAGING_OPERATION_TRANSFER  = 1,
    BUILDING_PAGING_OPERATION_FILL      = 2,
} build_paging_buffer_operation_t;

typedef struct _vidmm_private_build_paging_buffer_arg
{
    vidmm_allocation_t *allocation;
    void *dma_buffer;
    unsigned int dma_size;
    build_paging_buffer_operation_t operation;
    unsigned int multi_pass_offset;

    union
    {
        struct
        {
            unsigned int    transfer_offset;
            unsigned int    transfer_size;
            struct
            {
                unsigned int segment_id;
                unsigned long long phy_addr;
                unsigned int compress_format;
            } src;
            struct
            {
                unsigned int segment_id;
                unsigned long long phy_addr;
                unsigned int compress_format;
            } dst;
        } transfer;
        struct
        {
            unsigned int    fill_size;
            unsigned int    fill_pattern;
            struct
            {
                unsigned int segment_id;
                unsigned long long phy_addr;
            };
        } fill;
    };
} vidmm_private_build_paging_buffer_arg_t;

typedef struct _vidmm_segment_desc_t
{
    unsigned int               segment_id;     //segment id
    const char                 *segment_name;
    vidmm_segment_desc_flags_t flags;     
    unsigned int               segment_alignment;  //elite dynamic: 4M, dst/exc/elite pcie/ahb: 4k
    unsigned long long         gpu_vm_start;
    unsigned long long         gpu_vm_size;
    unsigned int               reserved_vm_size;
    unsigned long long         small_heap_size;
    unsigned long long         small_heap_max_allocate_size;
    unsigned long long         phys_addr_start;    //used for local video memory or reserved system memory
    struct list_head           pagable_allocation_list[PALL];
    struct os_mutex            *lock; 
} vidmm_segment_desc_t;

typedef struct _vidmm_chip_segment_info_t
{
    unsigned int          segment_cnt;
    vidmm_segment_desc_t  segment_desc[MAX_SEGMENT_ID];
    unsigned long long    cpu_visible_vidmm_size;
    unsigned long long    cpu_unvisible_vidmm_size;
    unsigned int          paging_segment_id;
    unsigned int          paging_segment_size;
} vidmm_chip_segment_info_t;

typedef struct _vidmm_chip_func
{
    void (*query_segment_info)(adapter_t*, vidmm_chip_segment_info_t*);
    void (*mem_setting)(adapter_t *);
    void (*init_gart_table)(adapter_t*);
    void (*init_svm_gart_table)(adapter_t*);
    void (*deinit_svm_gart_table)(adapter_t*);    
    void (*deinit_gart_table)(adapter_t*);
    int  (*query_gart_table_info)(adapter_t*);
    int  (*describe_allocation)(adapter_t *, vidmm_describe_allocation_t *);
    int  (*build_paging_buffer)(adapter_t*, vidmm_private_build_paging_buffer_arg_t*);
    void (*map_gart_table)(adapter_t*, vidmm_allocation_t*, int);
    void (*map_svm_gart_table)(adapter_t*, vidmm_allocation_t*, int);
    void (*unmap_gart_table)(adapter_t*, vidmm_allocation_t*);    
    void (*unmap_svm_gart_table)(adapter_t*, vidmm_allocation_t*);
    void (*set_snooping)(adapter_t *, vidmm_allocation_t*, int);
    int  (*get_allocation_info)(adapter_t *, vidmm_get_allocation_info_t *);
    void (*restore)(adapter_t *);
    int  (*query_info)(struct _vidmm_mgr *, gf_query_info_t *);
} vidmm_chip_func_t;

typedef struct _vidmm_mgr
{
    adapter_t       *adapter;

    unsigned int      segment_cnt;
    vidmm_segment_t  *segment;

    vidmm_segment_t    *paging_segment;
    list_node_t        *paging_segment_node;

    unsigned int       secure_segment_id;
    int                 shrink_segment_id; //only swap cache_segment and the maxmize pcie heap
    struct os_shrinker *shrinker;

    unsigned long   max_pages_num;   // max system pages can used for vidmm
    unsigned long   emergency_pages; // must success can allocate from this
    unsigned long   used_pages_num;  // already used pages in our vidmm
    unsigned long   own_pages_num;   // allocate page by this driver 
    unsigned long   swap_page_num;
    int swap_gmem ;

    unsigned long long cpu_visible_vidmm_size;
    unsigned long long cpu_unvisible_vidmm_size;
    unsigned int    pcie_segment_size;
    unsigned int    reserved_vmem;
    unsigned char   aperture_used[MAX_APERTURES];

    struct os_mutex     *hw_lock; /* lock for aperture, pages */

    struct list_head    paging_task_list;
    struct list_head    defer_destroy_list;

    struct os_mutex     *list_lock;   /* list lock for paging_task, defer_list*/
   
    struct os_pages_memory* dummy_page;

    //chip private funcs
    vidmm_chip_func_t   *chip_func;

    void      *flag_buffer;
    heap_t    blheap;
} vidmm_mgr_t;

extern vidmm_chip_func_t   vidmm_chip_func;


extern int  vidmmi_allocate_video_memory_try(vidmm_mgr_t *mm_mgr, vidmm_allocation_t *allocation, vidmm_segment_preference_t *preferred_segment);
extern void vidmmi_release_video_memory(vidmm_mgr_t *mm_mgr, vidmm_allocation_t *allocation);
extern void vidmmi_release_segment_memory(vidmm_mgr_t *mm_mgr, int segment_id, list_node_t *list_node);

extern int  vidmmi_allocate_system_memory(adapter_t *adapter, vidmm_allocation_t *allocation, int must_success, int paging_locked);
extern void vidmmi_release_system_memory(adapter_t *adapter, vidmm_allocation_t *allocation);

extern void vidmmi_try_destroy_one_allocation(gpu_device_t *device, vidmm_allocation_t *allocation);

extern int  vidmmi_segment_unresident_allocations(vidmm_mgr_t *mm_mgr, vidmm_segment_t *segment, int must_success);
extern int  vidmmi_prepare_one_allocation(gpu_device_t *device, vidmm_allocation_t *allocation);

extern int  vidmmi_do_swap_out(vidmm_mgr_t *mm_mgr, int need_pages);

extern void vidmmi_dump_video_memory(vidmm_mgr_t *mm_mgr, unsigned int segment_id);
extern void vidmmi_destroy_allocation(adapter_t *adapter, vidmm_allocation_t *allocation);

extern void vidmmi_dump_segments(adapter_t *adapter);

extern unsigned int vidmmi_query_max_size_of_local_allocation(adapter_t *adapter);
#endif


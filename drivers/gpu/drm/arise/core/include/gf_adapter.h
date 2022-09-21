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

#ifndef __GF_ADAPTER_H__
#define __GF_ADAPTER_H__

#include "handle_manager.h"
#include "heap_manager.h"
#include "core_errno.h"
#include "util.h"
#include "list.h"
#include "core_import.h"
#include "gf_chip_id.h"
#include "gf_types.h"
#include "gf_def.h"

//#define ENABLE_UNSNOOPABLE_PCIE 0
#if defined(__aarch64__)
//arm64 only can pass snoop path for diffcult to flush page cache now, will remove this if all is ready
#define ENABLE_UNSNOOPABLE_WROKAROUND 	1
#else
#define ENABLE_UNSNOOPABLE_WROKAROUND 	0
#endif

#define ACPI_TEMP_CODE 0
#define NEW_CBIOS_FUNC_SUPPORT 1

#define VIDMM_TRACE_ENABLE        0
#define VIDSCH_TRACE_ENABLE       0
#define DISPMGR_TRACE_ENABLE      0
#define MISC_DEBUG_INFO_ENABLE    0
#define SEGMENT_TRACE_ENABLE      0
#define VIDSCH_SYNC_TRACE_ENABLE  0
#define DISPLAY_TRACE_ENABLE      0
#define VIDSCH_POWER_DEBUG        0

#define GF_TRACE_HW_HANG         0
#define DEBUG_TEST                0
#define DEBUG_HIGH_4G_MEM         0
#define HIGH_4G_MEM_RESERVE_SIZE  (0x10000000) //64M
#define TEST_VIDEO_RING_BUFFER    0
#define TEST_POWER_MANAGE         0
#define RESERVE_16M_FOR_HDA       1

#define DEFAULT_PRESENT_COUNT     3

#if VIDMM_TRACE_ENABLE
#define vidmm_trace    gf_info
#else
#define vidmm_trace(args...)
#endif

#if VIDSCH_TRACE_ENABLE
#define vidsch_trace    gf_info
#else
#define vidsch_trace(args...)
#endif

#if DISPMGR_TRACE_ENABLE
#define dispmgr_trace    gf_info
#else
#define dispmgr_trace(args...)
#endif

#if SEGMENT_TRACE_ENABLE
#define segment_trace   gf_info
#else
#define segment_trace(args...)
#endif

#if VIDSCH_SYNC_TRACE_ENABLE
#define sync_trace   gf_info
#else
#define sync_trace(args...)
#endif

#if DISPLAY_TRACE_ENABLE
#define display_trace gf_info
#else
#define display_trace(args...)
#endif

#if VIDSCH_POWER_DEBUG
#define power_trace gf_info
#else
#define power_trace(args...)
#endif

#define VCP0_INDEX      0
#define VCP1_INDEX      1
#define VCP_INDEX_COUNT 2
#define VCP_INFO_COUNT  17
#define MAX_VIDEO_SEQ_INDEX_ADD1 128
#define MIN_VIDEO_SEQ_INDEX      0
//it's from RB_INDEX_VCP0
#define RB_INDEX_VIDEO_START     7

typedef struct engine_index_info
{
    unsigned int    node_ordinal;
    unsigned int    engine_affinity;
} engine_index_info_t;

/* our hw feature put here*/
typedef struct hw_caps
{
    /* hw general features*/
    unsigned int fence_interrupt_enable:    1;
    unsigned int support_snooping   :1;
    unsigned int address_mode_64bit :1;
    unsigned int hw_patch_enable    :1;

    unsigned int video_only         :1;   // fpga option
    unsigned int gfx_only           :1;   // fpga option
    unsigned int local_only         :1;   // only has local memory
    unsigned int snoop_only         :1;   //without non-snoopy path

    unsigned int page_64k_enable    :1;
    unsigned int page_4k_enable     :1;
    unsigned int secure_range_enable:1;
    unsigned int anti_hang_enable   :1; //anti hang

    unsigned int svm_enable         :1; //OCL share virtual memory
    unsigned int dfs_enable         :1;
    unsigned int reserved           :18;

    int             miu_channel_num;  //should be 1~3
    int             miu_channel_size;
    unsigned int    chip_slice_mask;
}hw_caps_t;

/* our system feature put here*/
typedef struct sys_caps
{
    unsigned int bus_pcie_inuse     :1;
    unsigned int bus_ahb_inuse      :1;
    unsigned int secure_on          :1;
    unsigned int iommu_enabled      :1;
    unsigned int reserved           :28;
}sys_caps_t;

/* our power feature put here*/
typedef struct power_caps
{
    union
    {
        unsigned int pwm_auto;
        struct 
        {
            unsigned int gfx_cg_auto    :1;
            unsigned int vcp_cg_auto    :1;
            unsigned int vpp_cg_auto    :1;
            unsigned int gfx_pg_auto    :1;
            unsigned int vcp_pg_auto    :1;
            unsigned int vpp_pg_auto    :1;
            unsigned int auto_resv      :26;
        };
    
    };
    union
    {
        unsigned int pwm_manual;
        struct
        {
            unsigned int gfx_cg_manual  :1;
            unsigned int vcp_cg_manual  :1;
            unsigned int vpp_cg_manual  :1;
            unsigned int gfx_pg_manual  :1;
            unsigned int vcp_pg_manual  :1;
            unsigned int vpp_pg_manual  :1;
            unsigned int manual_rsv     :26;
        };
    };
    unsigned int pwm_mode;
    union
    {
        unsigned int dvfs_auto;
        struct
        {
            unsigned int gfx_dvfs_auto  :1;
            unsigned int vcp_dvfs_auto  :1;
            unsigned int vpp_dvfs_auto  :1;
            unsigned int dvfs_auto_rsv  :29;
        };
    };
    union
    {
        unsigned int dvfs_force;
        struct
        {
            unsigned int gfx_dvfs_force :1;
            unsigned int vcp_dvfs_force :1;
            unsigned int vpp_dvfs_force :1;
            unsigned int dvfs_force_rsv :29;
        };
    };
    unsigned int dvfs_interrupt;
    union
    {
        unsigned int slice_balance;
        struct
        {
            unsigned int slice_balance_auto  :1;//auto mode
            unsigned int slice_balance_force :1;//simulate auto mode
            unsigned int slice_force_enable  :1;//force mode
            unsigned int slice_force_value   :1;//force mode value
            unsigned int slice_rsv           :28;
        };
    };
}power_caps_t;


/* adapter control flag put here */
typedef struct ctl_flags
{
    unsigned int  paging_enable         :1;
    unsigned int  swap_enable           :1;  /* swap pages to shmem */
    unsigned int  split_enable          :1;
    unsigned int  worker_thread_enable  :1;
    unsigned int  recovery_enable       :1;  /* reset hw if hang, must disable it if debug hang */
    unsigned int  hotplug_thread_enable :1;
    unsigned int  hotplug_polling_enable:1;  /* CRT not support HW hotplug interrupt, use thread polling to detect it*/
    unsigned int  compare_edid          :1;  /* do compare edid when do polling */
    unsigned int  manual_flush_cache    :1;  /* if this flag set, default use wb for none snoop segment, and flush cache before GPU use manual,
                                                for ARM use since arm snoop performance not good */
    unsigned int  dump_hang_info_enable  :1;
    unsigned int  dump_hang_info_to_file :1;
    unsigned int  hang_dump              :2;
    unsigned int  wfd_enable             :1; /* wifi display */
    unsigned int  perf_event_enable      :1;
    unsigned int  miu_counter_enable     :1;
    unsigned int  power_test_en          :1;
    unsigned int  flag_buffer_verify     :1;
    unsigned int  local_for_display_only :1;
    unsigned int  debug_secure           :2;
    unsigned int  rename_enable          :1;
    unsigned int  submit_to_queue        :1; /* if true, submit task to queue directly. only used when worker thread enable!!! */
    unsigned int  one_shot_enable        :1;
    unsigned int  run_on_qt              :1;
    unsigned int  reboot_patch_enable    :1;
    unsigned int  vesa_tempbuffer_enable :1;
    unsigned int  force_3dblt            :1;
    unsigned int  reserved               :4;
}ctl_flags_t;

#define PATCH_E2UMA_FENCE_ID_LOST   (1 << 0)
#define PATCH_FENCE_INTERRUPT_LOST  (1 << 1)
#define PATCH_E2UMA_HW66 (1<<2)

typedef struct
{
    int EnableClockGating;
    int EnablePowerGating;
    int DonotInitPowerSet;
}pwm_level_t;

typedef struct
{
    unsigned int                max_present_count;
    
    ctl_flags_t                 ctl_flags;
    sys_caps_t                  sys_caps;
    hw_caps_t                   hw_caps;
    pwm_level_t                 pwm_level;
    power_caps_t                pm_caps;
    unsigned int                hw_patch_mask0;
    unsigned int                hw_patch_mask1;

    void                        *post_event_argu;
    int (*post_event)(void *argu, unsigned int event_mask);

    void                        *post_rc_event_argu;
    int (*post_rc_event)(void *argu, unsigned char ui_cmd_id, int value);

    void                        *post_sync_event_argu;
    int (*post_sync_event)(void *argu, unsigned int arg0, unsigned long long time);

    gf_drm_callback_t          *drm_cb;
    void                        *drm_cb_argu;

    os_device_t                 os_device;
 
    unsigned char               *mmio;
    unsigned int                *bci_base;
    gf_vm_area_t               *mmio_vma;
    struct os_pages_memory      *mmio_mem;

    //pmu_mmio
    unsigned long long          pmu_phys_addr; //use for secure register access.
    unsigned char               *pmu_mmio;
    gf_vm_area_t               *pmu_mmio_vma;

    //gpio
    unsigned char               *gpio;
    gf_vm_area_t               *gpio_vma;

    //pmc
    unsigned long long          pmc_phys_addr; //use for secure register access.
    unsigned char               *pmc;
    gf_vm_area_t               *pmc_vma;

    bus_config_t                bus_config;
    handle_mgr_t                hdl_mgr;
    unsigned int                primary;
    unsigned int                index;
    
    unsigned int                family_id;
    unsigned int                chip_id;
    unsigned int                generic_id;
    unsigned short              link_width;

    unsigned long long          physical_bus_base_length[2];
    unsigned long long          vidmm_bus_addr;

    unsigned int                os_page_size;
    unsigned int                os_page_shift;

    /* total memory size in gpu, include reserved */
    /* when hang dump is on, this is half of orignal value */
    unsigned long long          Visible_vram_size; //only cpu visiable size

    unsigned char               context_buffer_inuse[0xFFFF];

    struct os_mutex             *paging_lock;  /* lock for paging */

    struct os_mutex             *device_list_lock;  /* lock for paging */

    struct os_spinlock          *lock;   /* adapter lock for device list and sync_obj list */

    struct list_head            sync_obj_list; /* defer destroy sync obj list */
    struct list_head            device_list;   /* device create in this adapter*/

    struct di_mgr               *dmgr;      /* deinterlace mgr */
    struct _vidmm_mgr           *mm_mgr;

    int                         paging_engine_index;

    struct vidsch_fence_buffer  *fence_buf;
    struct _vidsch_mgr          *sch_mgr[MAX_ENGINE_COUNT];
    struct vidschedule          *schedule;
    int                         active_engine_count;
    engine_index_info_t         engine_index_table[MAX_ENGINE_COUNT];

    struct os_mutex             *hw_reset_lock;
    unsigned int                 hw_reset_times;
    
    /* isr */
    struct _vidmm_gart_table_info *gart_table;
    struct _vidmm_gart_table_info *gart_table_64k;

    //since e3k has 3 level page table, L3 is on chip 16*256*32bit. 
    struct _vidmm_gart_table_info *gart_table_L3;
    struct _vidmm_gart_table_info *gart_table_L2;

    struct _vidmm_gart_table_info *gart_table_SVM_L3;
    struct _vidmm_gart_table_info *gart_table_SVM_L2;
    unsigned long long            dummy_page_addr; /*dummy page addr for gart table*/

    struct _perf_event_mgr      *perf_event_mgr;

    unsigned long long          Real_vram_size;     // Total Video Mem
    unsigned long long          UnAval_vram_size;   // for 3 miu, need dig out some mem from end of vram.    
    unsigned int                fb_phy_addr;        // for DUMA

    void                        *private_data;

    void                        *flag_buffer_virt_addr; // for elite flag buffer debug

    struct os_mutex             *gart_table_lock;

    int                         boot_noise_stop;
    void                        *vesafb_tempbuffer;
    void                        *mem_unavailable_for_3miu;   //AVAILABLE

    unsigned long long          low_top_address;

    unsigned long long          fb_shadow_2d;

    int                         vcp_index_cnt[VCP_INDEX_COUNT];
    gf_vcp_info                 vcp_info[VCP_INFO_COUNT];
    unsigned  char              bVideoSeqIndex[MAX_VIDEO_SEQ_INDEX_ADD1];
    int                         start_index;
    int                         end_index;

#if DEBUG_HIGH_4G_MEM
    void                        *Reserve_Buffer;
#endif
    
    int                         log_fd;
    void                      *disp_info;
    unsigned long long          pcie_vram_size;   //set pcie memory size

#if RESERVE_16M_FOR_HDA
    void                        *reserve_16m_buffer;
#endif
} adapter_t;

#endif


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

#ifndef __GF_IOCTL_H__
#define __GF_IOCTL_H__

#if defined(__linux__)
#include <drm/drm.h>
#include <asm/ioctl.h>
#define GF_IOCTL_NR(n)         _IOC_NR(n)
#define GF_IOCTL_TYPE(n)       _IOC_TYPE(n)
#define GF_IOC_VOID            _IOC_NONE
#define GF_IOC_READ            _IOC_READ
#define GF_IOC_WRITE           _IOC_WRITE
#define GF_IOC_READWRITE       _IOC_READ | _IOC_WRITE
#define GF_IOC(dir, group, nr, size) _IOC(dir, group, nr, size)
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
#if defined(__FreeBSD__) && defined(IN_MODULE)
#undef ioctl
#include <sys/ioccom.h>
#define ioctl(a,b,c)            xf86ioctl(a,b,c)
#else
#include <sys/ioccom.h>
#endif
#define GF_IOCTL_NR(n)         ((n) & 0xff)
#define GF_IOCTL_TYPE(n)       (((n) >> 8) & 0xff)
#define GF_IOC_VOID            IOC_VOID
#define GF_IOC_READ            IOC_OUT
#define GF_IOC_WRITE           IOC_IN
#define GF_IOC_READWRITE       IOC_INOUT
#define GF_IOC(dir, group, nr, size) _IOC(dir, group, nr, size)
#endif

#ifndef __user
#define __user
#endif

enum gf_ioctl_nr
{
    ioctl_nr_begin_perf_event = 0,
    ioctl_nr_end_perf_event = 1,
    ioctl_nr_get_perf_event = 2,
    ioctl_nr_send_perf_event = 3,
    ioctl_nr_get_perf_status = 4,
    ioctl_nr_begin_miu_dump_perf_event = 5,
    ioctl_nr_end_miu_dump_perf_event = 6,
    ioctl_nr_set_miu_reg_list_perf_event = 7,
    ioctl_nr_get_miu_dump_perf_event = 8,
    ioctl_nr_direct_get_miu_dump_perf_event = 9,
    ioctl_nr_perf_reserved = 10,

    ioctl_nr_wait_chip_idle = 11,
    ioctl_nr_wait_allocation_idle = 12,
    ioctl_nr_query_info = 13,
    ioctl_nr_create_device = 14,
    ioctl_nr_destroy_device = 15,
    ioctl_nr_create_context = 16,
    ioctl_nr_destroy_context = 17,
    ioctl_nr_render = 18,
    ioctl_nr_create_fence_sync_object = 19,
    ioctl_nr_destroy_fence_sync_object = 20,
    ioctl_nr_wait_fence_sync_object = 21,
    ioctl_nr_fence_value = 22,
    ioctl_nr_get_allocation_state = 23,

    ioctl_nr_drm_gem_create_allocation = 24,
    ioctl_nr_drm_gem_create_resource = 25,
    ioctl_nr_drm_gem_map = 26,
    ioctl_nr_drm_begin_cpu_access = 27,
    ioctl_nr_drm_end_cpu_access = 28,
    ioctl_nr_kms_get_pipe_from_crtc = 29,

    ioctl_nr_get_hw_context = 30,
    ioctl_nr_add_hw_context = 31,
    ioctl_nr_rm_hw_context = 32,

    ioctl_nr_cil2_misc = 33,

    ioctl_nr_video = 36,
    ioctl_nr_create_di_context = 37,
    ioctl_nr_destroy_di_context = 38,
    ioctl_nr_set_interactive = 39,

    /* new ioctl MUST add before 'ioctl_nr_total_num' which used to count total ioctl nums */
    ioctl_nr_total_num
};

#define GF_IOCTL_BASE                     'd'
#define GF_IO(nr)                         _IO(GF_IOCTL_BASE, nr)
#define GF_IOR(nr, type)                  _IOR(GF_IOCTL_BASE, nr, type)
#define GF_IOW(nr, type)                  _IOW(GF_IOCTL_BASE, nr, type)
#define GF_IOWR(nr, type)                 _IOWR(GF_IOCTL_BASE, nr, type)

#define GF_IOCTL_WAIT_CHIP_IDLE           GF_IO(DRM_COMMAND_BASE + ioctl_nr_wait_chip_idle)
#define GF_IOCTL_ESCAPE_CALL              GF_IOW(DRM_COMMAND_BASE + ioctl_nr_escape_call, long)
#define GF_IOCTL_WAIT_ALLOCATION_IDLE     GF_IOW(DRM_COMMAND_BASE + ioctl_nr_wait_allocation_idle, gf_wait_allocation_idle_t)
#define GF_IOCTL_QUERY_INFO               GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_query_info, gf_query_info_t)
#define GF_IOCTL_CREATE_DEVICE            GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_create_device, gf_create_device_t)
#define GF_IOCTL_DESTROY_DEVICE           GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_destroy_device, unsigned int)
#define GF_IOCTL_BEGIN_PERF_EVENT         GF_IOW(DRM_COMMAND_BASE + ioctl_nr_begin_perf_event, gf_begin_perf_event_t)
#define GF_IOCTL_END_PERF_EVENT           GF_IOW(DRM_COMMAND_BASE + ioctl_nr_end_perf_event, gf_end_perf_event_t)
#define GF_IOCTL_GET_PERF_EVENT           GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_get_perf_event, gf_get_perf_event_t)
#define GF_IOCTL_SEND_PERF_EVENT          GF_IOW(DRM_COMMAND_BASE + ioctl_nr_send_perf_event, gf_perf_event_t)
#define GF_IOCTL_GET_PERF_STATUS          GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_get_perf_status, gf_perf_status_t)
#define GF_IOCTL_BEGIN_MIU_DUMP_PERF_EVENT GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_begin_miu_dump_perf_event, gf_begin_miu_dump_perf_event_t)
#define GF_IOCTL_END_MIU_DUMP_PERF_EVENT  GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_end_miu_dump_perf_event, gf_end_miu_dump_perf_event_t)
#define GF_IOCTL_SET_MIU_REG_LIST_PERF_EVENT GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_set_miu_reg_list_perf_event, gf_miu_reg_list_perf_event_t)
#define GF_IOCTL_GET_MIU_DUMP_PERF_EVENT  GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_get_miu_dump_perf_event, gf_get_miu_dump_perf_event_t)
#define GF_IOCTL_DIRECT_GET_MIU_DUMP_PERF_EVENT GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_direct_get_miu_dump_perf_event, gf_direct_get_miu_dump_perf_event_t)
#define GF_IOCTL_CREATE_CONTEXT           GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_create_context, gf_create_context_t)
#define GF_IOCTL_DESTROY_CONTEXT          GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_destroy_context, gf_destroy_context_t)
#define GF_IOCTL_RENDER                   GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_render, gf_render_t)
#define GF_IOCTL_CREATE_FENCE_SYNC_OBJECT GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_create_fence_sync_object, gf_create_fence_sync_object_t)
#define GF_IOCTL_DESTROY_FENCE_SYNC_OBJECT GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_destroy_fence_sync_object, gf_destroy_fence_sync_object_t)
#define GF_IOCTL_WAIT_FENCE_SYNC_OBJECT   GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_wait_fence_sync_object, gf_wait_fence_sync_object_t)
#define GF_IOCTL_FENCE_VALUE              GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_fence_value, gf_fence_value_t)
#define GF_IOCTL_CREATE_DI_CONTEXT        GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_create_di_context, gf_create_di_context_t)
#define GF_IOCTL_DESTROY_DI_CONTEXT       GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_destroy_di_context, gf_destroy_di_context_t)
#define GF_IOCTL_RENDER                   GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_render, gf_render_t)
#define GF_IOCTL_GET_ALLOCATION_STATE     GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_get_allocation_state, gf_get_allocation_state_t)


#define GF_IOCTL_DRM_GEM_CREATE_ALLOCATION GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_drm_gem_create_allocation, gf_create_allocation_t)
#define GF_IOCTL_DRM_GEM_CREATE_RESOURCE   GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_drm_gem_create_resource, gf_create_resource_t)
#define GF_IOCTL_DRM_GEM_MAP_GTT           GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_drm_gem_map, gf_drm_gem_map_t)
#define GF_IOCTL_ADD_HW_CTX_BUF            GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_add_hw_context, gf_add_hw_ctx_buf_t)
#define GF_IOCTL_RM_HW_CTX_BUF             GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_rm_hw_context, gf_rm_hw_ctx_buf_t)
#define GF_IOCTL_DRM_BEGIN_CPU_ACCESS      GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_drm_begin_cpu_access, gf_drm_gem_begin_cpu_access_t)
#define GF_IOCTL_DRM_END_CPU_ACCESS        GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_drm_end_cpu_access, gf_drm_gem_end_cpu_access_t)
#define GF_IOCTL_KMS_GET_PIPE_FROM_CRTC    GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_kms_get_pipe_from_crtc, gf_kms_get_pipe_from_crtc_t)

#define GF_IOCTL_CIL2_MISC                 GF_IOWR(DRM_COMMAND_BASE + ioctl_nr_cil2_misc, gf_cil2_misc_t)

#endif //__GF_IOCTL_H__

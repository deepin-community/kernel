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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#define stat_t struct stat
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <stdarg.h>
#include <assert.h>
#include <libdrm/drm.h>
#include <xf86drm.h>

#include "gf_keinterface.h"
#include "gf_ioctl.h"

#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

#define GF_DEV_UID         0
#define GF_DEV_GID         0

#define GF_DEV_DIRMODE             777
//(S_IXOTH | S_IXUSR | S_IXGRP | S_ISUID | S_ISGID | S_IWOTH | S_IROTH | S_IWGRP | S_IRGRP | S_IWUSR | S_IRUSR)
#define GF_DEV_MODE         (S_IWOTH | S_IROTH | S_IWGRP | S_IRGRP | S_IWUSR | S_IRUSR)


struct drm_version_gf{
    int major;
    int minor;
    int patchlevel;
    size_t name_len;
    char* name;
    size_t date_len;
    char* date;
    size_t desc_len;
    char* desc;
};

int gfCreateContext(int fd, gf_create_context_t *create)
{
    if(ioctl(fd, GF_IOCTL_CREATE_CONTEXT, create))
    {
        return -errno;
    }

    return 0;
}

int gfDestroyContext(int fd, gf_destroy_context_t *destroy)
{
    if(ioctl(fd, GF_IOCTL_DESTROY_CONTEXT, destroy))
    {
        return -errno;
    }

    return 0;
}

int gfRender(int fd, gf_render_t *render)
{
    if(ioctl(fd, GF_IOCTL_RENDER, render))
    {
        return -errno;
    }

    return 0;
}

int gfCreateDevice(int fd, gf_create_device_t* create_device)
{   
    if(ioctl(fd, GF_IOCTL_CREATE_DEVICE, create_device))
    {
        return -errno;
    }

    return 0;
}

int gfDestroyDevice(int fd, unsigned int device)
{
    if(ioctl(fd, GF_IOCTL_DESTROY_DEVICE, &device))
    {
        return -errno;
    }

    return 0;
}

/*
** For now, we always set sub_device_index is zero.
** 
*/
int gfWaitChipIdle(int fd)
{
    if(ioctl(fd, GF_IOCTL_WAIT_CHIP_IDLE))
    {
        return -errno;
    }

    return 0;
}

int gfWaitAllocationIdle(int fd, gf_wait_allocation_idle_t *wait_allocation)
{
    if(ioctl(fd, GF_IOCTL_WAIT_ALLOCATION_IDLE, wait_allocation))
    {
        return -errno;
    }
   
    return 0;
}

int gfQueryInfo(int fd, gf_query_info_t *info)
{
    if(ioctl(fd, GF_IOCTL_QUERY_INFO, info))
    {
        return -errno;
    }

    return 0;
}

int gfBeginPerfEvent(int fd, gf_begin_perf_event_t *begin_perf_event)
{
    if(ioctl(fd, GF_IOCTL_BEGIN_PERF_EVENT, begin_perf_event))
    {
        return -errno;
    }

    return 0;
}

int gfEndPerfEvent(int fd, gf_end_perf_event_t *end_perf_event)
{
    if(ioctl(fd, GF_IOCTL_END_PERF_EVENT, end_perf_event))
    {
        return -errno;
    }

    return 0;
}

int gfGetPerfEvent(int fd, gf_get_perf_event_t *get_perf_event)
{
    if(ioctl(fd, GF_IOCTL_GET_PERF_EVENT, get_perf_event))
    {
        return -errno;
    }

    return 0;
}

int gfSendPerfEvent(int fd, gf_perf_event_t *perf_event)
{
    if(ioctl(fd, GF_IOCTL_SEND_PERF_EVENT, perf_event))
    {
        return -errno;
    }

    return 0;
}

int gfGetPerfStatus(int fd, gf_perf_status_t *perf_status)
{
    if(ioctl(fd, GF_IOCTL_GET_PERF_STATUS, perf_status))
    {
        return -errno;
    }

    return 0;
}

int gfBeginMiuDumpPerfEvent(int fd, gf_begin_miu_dump_perf_event_t *begin_miu_perf_event)
{
    if(ioctl(fd, GF_IOCTL_BEGIN_MIU_DUMP_PERF_EVENT, begin_miu_perf_event))
    {
        return -errno;
    }

    return 0;
}

int gfEndMiuDumpPerfEvent(int fd, gf_end_miu_dump_perf_event_t *end_miu_perf_event)
{
    if(ioctl(fd, GF_IOCTL_END_MIU_DUMP_PERF_EVENT, end_miu_perf_event))
    {
        return -errno;
    }

    return 0;
}

int gfSetMiuRegListPerfEvent(int fd, gf_miu_reg_list_perf_event_t *miu_reg_list)
{
    if(ioctl(fd, GF_IOCTL_SET_MIU_REG_LIST_PERF_EVENT, miu_reg_list))
    {
        return -errno;
    }

    return 0;
}

int gfGetMiuDumpPerfEvent(int fd, gf_get_miu_dump_perf_event_t *get_miu_dump)
{
    if(ioctl(fd, GF_IOCTL_GET_MIU_DUMP_PERF_EVENT, get_miu_dump))
    {
        return -errno;
    }

    return 0;
}

int gfDirectGetMiuDumpPerfEvent(int fd, gf_direct_get_miu_dump_perf_event_t *direct_get_dump)
{
    if(ioctl(fd, GF_IOCTL_DIRECT_GET_MIU_DUMP_PERF_EVENT, direct_get_dump))
    {
        return -errno;
    }

    return 0;
}


int gfDvfsSet(int fd, gf_dvfs_set_t * set)
{
    if (ioctl(fd, GF_IOCTL_DVFS_SET, set))
    {
        return -errno;
    }

    return 0;
}

int gfQueryDvfsClamp(int fd, gf_dvfs_clamp_status_t * dvfs_clamp)
{
    if (ioctl(fd, GF_IOCTL_QUERY_DVFS_CLAMP, dvfs_clamp))
    {
        return -errno;
    }

    return 0;
}

int gfCreateFenceSyncObject(int fd, gf_create_fence_sync_object_t *create)
{
    if(ioctl(fd, GF_IOCTL_CREATE_FENCE_SYNC_OBJECT, create))
    {
        return -errno;
    }

    return 0;
}

int gfDestroyFenceSyncObject(int fd, gf_destroy_fence_sync_object_t *destroy)
{
    if(ioctl(fd, GF_IOCTL_DESTROY_FENCE_SYNC_OBJECT, destroy))
    {
        return -errno;
    }

    return 0;
}

int gfWaitFenceSyncObject(int fd, gf_wait_fence_sync_object_t *wait)
{
    if(ioctl(fd, GF_IOCTL_WAIT_FENCE_SYNC_OBJECT, wait))
    {
        return -errno;
    }

    return 0;
}

int gfFenceValue(int fd, gf_fence_value_t *fence)
{
    if(ioctl(fd, GF_IOCTL_FENCE_VALUE, fence))
    {
        return -errno;
    }

    return 0;
}

int gfCreateDIContext(int fd, gf_create_di_context_t *create_context)
{
    if(ioctl(fd, GF_IOCTL_CREATE_DI_CONTEXT, create_context))
    {
        return -errno;
    }
 
    return 0;
}

int gfDestroyDIContext(int fd, gf_destroy_di_context_t *destroy_context)
{
    if(ioctl(fd, GF_IOCTL_DESTROY_DI_CONTEXT, destroy_context))
    {
        return -errno;
    }

    return 0;
}

int gfGetAllocationState(int fd, gf_get_allocation_state_t *state)
{
    if (ioctl(fd, GF_IOCTL_GET_ALLOCATION_STATE, state))
    {
        return -errno;
    }

    return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// interface for gem
//    LeonHe: invert op is not defined
////////////////////////////////////////////////////////////////////////////////////////////////////
int gfGemCreateAllocation(int fd, gf_create_allocation_t *create)
{
    if(ioctl(fd, GF_IOCTL_DRM_GEM_CREATE_ALLOCATION, create))
    {
        return -errno;
    }

    return 0;
}

int gfGemDestroyObject(int fd, unsigned int handle)
{
    struct drm_gem_close close={0};

    close.handle = handle;

    drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &close);

    return 0;
}

int gfGemCreateResource(int fd, gf_create_resource_t *create)
{
    if(ioctl(fd, GF_IOCTL_DRM_GEM_CREATE_RESOURCE, create))
    {
        return -errno;
    }

    return 0;
}

int gfGemBeginCpuAccess(int fd, gf_drm_gem_begin_cpu_access_t *begin)
{
    if (ioctl(fd, GF_IOCTL_DRM_BEGIN_CPU_ACCESS, begin))
    {
        return -errno;
    }

    return 0;
}

int gfGemEndCpuAccess(int fd, gf_drm_gem_end_cpu_access_t *end)
{
    if (ioctl(fd, GF_IOCTL_DRM_END_CPU_ACCESS, end))
    {
        return -errno;
    }

    return 0;
}

int gfGemMapGtt(int fd, gf_drm_gem_map_t *map)
{
    if(ioctl(fd, GF_IOCTL_DRM_GEM_MAP_GTT, map))
    {
        return -errno;
    }

    return 0;
}

int gfGemUnmap(int fd)
{
    assert(0);

    return 0;
}



int gfAddHwCtxBuf(int fd, gf_add_hw_ctx_buf_t *add)
{
    if(ioctl(fd, GF_IOCTL_ADD_HW_CTX_BUF, add))
    {
        return -errno;
    }

    return 0;
}

int gfRmHwCtxBuf(int fd, gf_rm_hw_ctx_buf_t *rm)
{
    if(ioctl(fd, GF_IOCTL_RM_HW_CTX_BUF, rm))
    {
        return -errno;
    }

    return 0;
}

int gfCIL2Misc(int fd,gf_cil2_misc_t * misc)
{
    if( ioctl(fd, GF_IOCTL_CIL2_MISC, misc) )
    {
        return -errno;
    }

    return 0;
}



int gfGetPipeFromCRTC(int fd, gf_kms_get_pipe_from_crtc_t *get_pipe)
{
    if (ioctl(fd, GF_IOCTL_KMS_GET_PIPE_FROM_CRTC, get_pipe))
    {
        return -errno;
    }

    return 0;
}

static struct kernel_interface kinterface_struct_v2 = {
    .WaitChipIdle = gfWaitChipIdle,
    .WaitAllocationIdle = gfWaitAllocationIdle,
    .QueryInfo = gfQueryInfo,
    .CreateDevice = gfCreateDevice,
    .DestroyDevice = gfDestroyDevice,
    .BeginPerfEvent = gfBeginPerfEvent,
    .EndPerfEvent = gfEndPerfEvent,
    .GetPerfEvent = gfGetPerfEvent,
    .SendPerfEvent = gfSendPerfEvent,
    .GetPerfStatus = gfGetPerfStatus,
    .BeginMiuDumpPerfEvent = gfBeginMiuDumpPerfEvent,
    .EndMiuDumpPerfEvent = gfEndMiuDumpPerfEvent,
    .SetMiuRegListPerfEvent = gfSetMiuRegListPerfEvent,
    .GetMiuDumpPerfEvent = gfGetMiuDumpPerfEvent,
    .DirectGetMiuDumpPerfEvent = gfDirectGetMiuDumpPerfEvent,
    .DvfsSet = gfDvfsSet,
    .QueryDvfsClamp = gfQueryDvfsClamp,    
    .CreateContext = gfCreateContext,
    .DestroyContext = gfDestroyContext,
    .Render = gfRender,
    .CreateFenceSyncObject = gfCreateFenceSyncObject,
    .DestroyFenceSyncObject = gfDestroyFenceSyncObject,
    .WaitFenceSyncObject = gfWaitFenceSyncObject,
    .FenceValue = gfFenceValue,
    .CreateDIContext = gfCreateDIContext,
    .DestroyDIContext = gfDestroyDIContext,
    .AddHwCtxBuf = gfAddHwCtxBuf,
    .RmHwCtxBuf = gfRmHwCtxBuf,
    .CIL2Misc = gfCIL2Misc,
    .GemDestroyObject = gfGemDestroyObject,
    .GemCreateAllocation = gfGemCreateAllocation,
    .GemCreateResource = gfGemCreateResource,
    .GemMapGtt = gfGemMapGtt,
    .GemUnmap = gfGemUnmap,
    .GetAllocationState = gfGetAllocationState,
    .GetPipeFromCRTC = gfGetPipeFromCRTC,
    .GemBeginCpuAccess = gfGemBeginCpuAccess,
    .GemEndCpuAccess = gfGemEndCpuAccess,
};

__attribute__((visibility("default")))
struct kernel_interface* CONCAT(kinterface_v2,DRIVER_NAME) = &kinterface_struct_v2;

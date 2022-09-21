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

#ifndef _gf_ke_interface_h_
#define _gf_ke_interface_h_

#include <stdio.h>
#include <stdlib.h>
#include "gf_def.h"
#include "gf_types.h"
#include "gf_version.h"

#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif
#endif

#define __STR(x) #x
#define STR(x) __STR(x)

#undef __CONCAT
#undef CONCAT
#define __CONCAT(x,y) x##y
#define CONCAT(x,y)     __CONCAT(x,y)

typedef enum _ENUM_RET
{ 
    ENUM_RET_SUCCESS = 0,
    ENUM_RET_FAILURE = 1,
    ENUM_RET_CONTINUE = 2,
    ENUM_RET_LAST,
}ENUM_RET;

#ifdef __INTERFACE_IMPLEMENT_DEFINED__
EXTERN_C int gfWaitChipIdle(int fd);
EXTERN_C int gfWaitAllocationIdle(int fd, gf_wait_allocation_idle_t *wait);

EXTERN_C int gfQueryInfo(int fd, gf_query_info_t *info);

EXTERN_C int gfCreateDevice(int fd, gf_create_device_t* create_device);

EXTERN_C int gfDestroyDevice(int fd, unsigned int device);


EXTERN_C int gfBeginPerfEvent(int fd, gf_begin_perf_event_t *begin_perf_event);

EXTERN_C int gfEndPerfEvent(int fd, gf_end_perf_event_t *end_perf_event);

EXTERN_C int gfGetPerfEvent(int fd, gf_get_perf_event_t *get_perf_event);

EXTERN_C int gfSendPerfEvent(int fd, gf_perf_event_t *perf_event);

EXTERN_C int gfGetPerfStatus(int fd, gf_perf_status_t *perf_status);

EXTERN_C int gfBeginMiuDumpPerfEvent(int fd, gf_begin_miu_dump_perf_event_t *begin_miu_perf_event);

EXTERN_C int gfEndMiuDumpPerfEvent(int fd, gf_end_miu_dump_perf_event_t *end_miu_perf_event);

EXTERN_C int gfSetMiuRegListPerfEvent(int fd, gf_miu_reg_list_perf_event_t *miu_reg_list);

EXTERN_C int gfGetMiuDumpPerfEvent(int fd, gf_get_miu_dump_perf_event_t *get_miu_dump);

EXTERN_C int gfDirectGetMiuDumpPerfEvent(int fd, gf_direct_get_miu_dump_perf_event_t *direct_get_dump);

EXTERN_C int gfDvfsSet(int fd, gf_dvfs_set_t * set);
EXTERN_C int gfQueryDvfsClamp(int fd, gf_dvfs_clamp_status_t * dvfs_clamp);


EXTERN_C int gfCreateContext(int fd,gf_create_context_t * create_context);

EXTERN_C int gfDestroyContext(int fd,gf_destroy_context_t * destroy_context);

EXTERN_C int gfRender(int fd,gf_render_t * render);

EXTERN_C int gfCreateFenceSyncObject(int fd, gf_create_fence_sync_object_t *create);

EXTERN_C int gfDestroyFenceSyncObject(int fd, gf_destroy_fence_sync_object_t *destroy);

EXTERN_C int gfWaitFenceSyncObject(int fd, gf_wait_fence_sync_object_t *wait);

EXTERN_C int gfFenceValue(int fd, gf_fence_value_t *fence);

EXTERN_C int gfCreateDIContext(int fd,gf_create_di_context_t * create_context);

EXTERN_C int gfDestroyDIContext(int fd,gf_destroy_di_context_t * destroy_context);

//--------------------
// Some uitilities
//------------------------
EXTERN_C int gfAddHwCtxBuf(int fd, gf_add_hw_ctx_buf_t * add);
EXTERN_C int gfRmHwCtxBuf(int fd, gf_rm_hw_ctx_buf_t * rm);
EXTERN_C int gfCIL2Misc(int fd,gf_cil2_misc_t * misc);

///////////////////////////////////////////////////////////////////////////////////
EXTERN_C int gfGemDestroyObject(int fd, unsigned int handle);
EXTERN_C int gfGemCreateAllocation(int fd, gf_create_allocation_t *create);
EXTERN_C int gfGemCreateResource(int fd, gf_create_resource_t *create);
EXTERN_C int gfGemMapGtt(int fd, gf_drm_gem_map_t *map);
EXTERN_C int gfGemUnmap(int fd);

EXTERN_C int gfGetAllocationState(int fd, gf_get_allocation_state_t *state);
EXTERN_C int gfGetPipeFromCRTC(int fd, gf_kms_get_pipe_from_crtc_t *get_pipe);
EXTERN_C int gfGemBeginCpuAccess(int fd, gf_drm_gem_begin_cpu_access_t *begin);
EXTERN_C int gfGemEndCpuAccess(int fd, gf_drm_gem_end_cpu_access_t *end);

#else

#define gfWaitChipIdle CONCAT(kinterface_v2,DRIVER_NAME)->WaitChipIdle
#define gfWaitAllocationIdle CONCAT(kinterface_v2,DRIVER_NAME)->WaitAllocationIdle
#define gfQueryInfo CONCAT(kinterface_v2,DRIVER_NAME)->QueryInfo
#define gfCreateDevice CONCAT(kinterface_v2,DRIVER_NAME)->CreateDevice
#define gfDestroyDevice CONCAT(kinterface_v2,DRIVER_NAME)->DestroyDevice
#define gfBeginPerfEvent CONCAT(kinterface_v2,DRIVER_NAME)->BeginPerfEvent
#define gfEndPerfEvent CONCAT(kinterface_v2,DRIVER_NAME)->EndPerfEvent
#define gfGetPerfEvent CONCAT(kinterface_v2,DRIVER_NAME)->GetPerfEvent
#define gfSendPerfEvent CONCAT(kinterface_v2,DRIVER_NAME)->SendPerfEvent
#define gfGetPerfStatus CONCAT(kinterface_v2,DRIVER_NAME)->GetPerfStatus
#define gfBeginMiuDumpPerfEvent CONCAT(kinterface_v2,DRIVER_NAME)->BeginMiuDumpPerfEvent
#define gfEndMiuDumpPerfEvent CONCAT(kinterface_v2,DRIVER_NAME)->EndMiuDumpPerfEvent
#define gfSetMiuRegListPerfEvent CONCAT(kinterface_v2,DRIVER_NAME)->SetMiuRegListPerfEvent
#define gfGetMiuDumpPerfEvent CONCAT(kinterface_v2,DRIVER_NAME)->GetMiuDumpPerfEvent
#define gfDirectGetMiuDumpPerfEvent CONCAT(kinterface_v2,DRIVER_NAME)->DirectGetMiuDumpPerfEvent
#define gfDvfsSet CONCAT(kinterface_v2,DRIVER_NAME)->DvfsSet
#define gfQueryDvfsClamp CONCAT(kinterface_v2,DRIVER_NAME)->QueryDvfsClamp
#define gfCreateContext CONCAT(kinterface_v2,DRIVER_NAME)->CreateContext
#define gfDestroyContext CONCAT(kinterface_v2,DRIVER_NAME)->DestroyContext
#define gfRender CONCAT(kinterface_v2,DRIVER_NAME)->Render
#define gfCreateFenceSyncObject CONCAT(kinterface_v2,DRIVER_NAME)->CreateFenceSyncObject
#define gfDestroyFenceSyncObject CONCAT(kinterface_v2,DRIVER_NAME)->DestroyFenceSyncObject
#define gfWaitFenceSyncObject CONCAT(kinterface_v2,DRIVER_NAME)->WaitFenceSyncObject
#define gfFenceValue CONCAT(kinterface_v2,DRIVER_NAME)->FenceValue
#define gfCreateDIContext CONCAT(kinterface_v2,DRIVER_NAME)->CreateDIContext
#define gfDestroyDIContext CONCAT(kinterface_v2,DRIVER_NAME)->DestroyDIContext
#define gfAddHwCtxBuf CONCAT(kinterface_v2,DRIVER_NAME)->AddHwCtxBuf
#define gfRmHwCtxBuf CONCAT(kinterface_v2,DRIVER_NAME)->RmHwCtxBuf
#define gfCIL2Misc CONCAT(kinterface_v2,DRIVER_NAME)->CIL2Misc
#define gfGemDestroyObject CONCAT(kinterface_v2,DRIVER_NAME)->GemDestroyObject
#define gfGemCreateAllocation CONCAT(kinterface_v2,DRIVER_NAME)->GemCreateAllocation
#define gfGemCreateResource CONCAT(kinterface_v2,DRIVER_NAME)->GemCreateResource
#define gfGemMapGtt CONCAT(kinterface_v2,DRIVER_NAME)->GemMapGtt
#define gfGemUnmap CONCAT(kinterface_v2,DRIVER_NAME)->GemUnmap
#define gfGetAllocationState CONCAT(kinterface_v2,DRIVER_NAME)->GetAllocationState
#define gfGetPipeFromCRTC CONCAT(kinterface_v2,DRIVER_NAME)->GetPipeFromCRTC
#define gfGemBeginCpuAccess CONCAT(kinterface_v2,DRIVER_NAME)->GemBeginCpuAccess
#define gfGemEndCpuAccess CONCAT(kinterface_v2,DRIVER_NAME)->GemEndCpuAccess

#endif

struct kernel_interface {
    int  (*WaitChipIdle) (int fd);
    int  (*WaitAllocationIdle) (int fd, gf_wait_allocation_idle_t *wait);
    int  (*QueryInfo) (int fd, gf_query_info_t *info);
    int  (*CreateDevice) (int fd, gf_create_device_t* create_device);
    int  (*DestroyDevice) (int fd, unsigned int device);
    int  (*BeginPerfEvent) (int fd, gf_begin_perf_event_t *begin_perf_event);
    int  (*EndPerfEvent) (int fd, gf_end_perf_event_t *end_perf_event);
    int  (*GetPerfEvent) (int fd, gf_get_perf_event_t *get_perf_event);
    int  (*SendPerfEvent) (int fd, gf_perf_event_t *perf_event);
    int  (*GetPerfStatus) (int fd, gf_perf_status_t *perf_status);
    int  (*BeginMiuDumpPerfEvent) (int fd, gf_begin_miu_dump_perf_event_t *begin_miu_perf_event);
    int  (*EndMiuDumpPerfEvent) (int fd, gf_end_miu_dump_perf_event_t *end_miu_perf_event);
    int  (*SetMiuRegListPerfEvent) (int fd, gf_miu_reg_list_perf_event_t *miu_reg_list);
    int  (*GetMiuDumpPerfEvent) (int fd, gf_get_miu_dump_perf_event_t *get_miu_dump);
    int  (*DirectGetMiuDumpPerfEvent) (int fd, gf_direct_get_miu_dump_perf_event_t *direct_get_dump);
    int  (*DvfsSet) (int fd, gf_dvfs_set_t * set);
    int  (*QueryDvfsClamp) (int fd, gf_dvfs_clamp_status_t * dvfs_clamp);	
    int  (*CreateContext) (int fd,gf_create_context_t * create_context);
    int  (*DestroyContext) (int fd,gf_destroy_context_t * destroy_context);
    int  (*Render) (int fd,gf_render_t * render);
    int  (*CreateFenceSyncObject) (int fd, gf_create_fence_sync_object_t *create);
    int  (*DestroyFenceSyncObject) (int fd, gf_destroy_fence_sync_object_t *destroy);
    int  (*WaitFenceSyncObject) (int fd, gf_wait_fence_sync_object_t *wait);
    int  (*FenceValue) (int fd, gf_fence_value_t *fence);
    int  (*CreateDIContext) (int fd,gf_create_di_context_t * create_context);
    int  (*DestroyDIContext) (int fd,gf_destroy_di_context_t * destroy_context);
    int  (*AddHwCtxBuf) (int fd, gf_add_hw_ctx_buf_t * add);
    int  (*RmHwCtxBuf) (int fd, gf_rm_hw_ctx_buf_t * rm);
    int  (*CIL2Misc) (int fd,gf_cil2_misc_t * misc);
    int  (*GemDestroyObject) (int fd, unsigned int handle);
    int  (*GemCreateAllocation) (int fd, gf_create_allocation_t *create);
    int  (*GemCreateResource) (int fd, gf_create_resource_t *create);
    int  (*GemMapGtt) (int fd, gf_drm_gem_map_t *map);
    int  (*GemUnmap) (int fd);
    int  (*GetAllocationState) (int fd, gf_get_allocation_state_t *state);
    int  (*GetPipeFromCRTC) (int fd, gf_kms_get_pipe_from_crtc_t *get_pipe);
    int  (*GemBeginCpuAccess) (int fd, gf_drm_gem_begin_cpu_access_t *begin);
    int  (*GemEndCpuAccess) (int fd, gf_drm_gem_end_cpu_access_t *end);	
};
EXTERN_C struct kernel_interface * CONCAT(kinterface_v2,DRIVER_NAME);
#endif


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

#ifndef _H_GF_BUFMGR_H
#define _H_GF_BUFMGR_H

#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif
#endif

#include "gf_def.h"
#include "gf_types.h"
#include "gf_keinterface.h"
#include "gf_list.h"

#if defined(DEBUG)
#include <syslog.h>
#endif

#ifdef DEBUG
    #define BUFMGR_LOG_TAG                "gpu[bufmgr]:"
    #define bufmgr_info(fmt, args...)     syslog(LOG_INFO, BUFMGR_LOG_TAG fmt, ##args)
    #define bufmgr_dbg(fmt, args...)      syslog(LOG_DEBUG, BUFMGR_LOG_TAG fmt, ##args)
    #define bufmgr_err(fmt, args...)      syslog(LOG_ERR, BUFMGR_LOG_TAG fmt, ##args)
    #define bufmgr_assert(expr)           do {if (!(expr)) { \
                                                syslog(LOG_CRIT, BUFMGR_LOG_TAG "Assert (%s) failed at %s:%d", #expr, __FILE__, __LINE__); exit(-1); \
                                            }}while(0)
#else
    #define bufmgr_info
    #define bufmgr_dbg
    #define bufmgr_err
    #define bufmgr_assert
#endif

typedef struct _gf_bo              gf_bo_t;
typedef struct _gf_bo_alloc_arg    gf_bo_alloc_arg_t;
typedef struct _gf_resource        gf_resource_t;
typedef struct _gf_bufmgr          gf_bufmgr_t;
typedef struct _gf_rename_resource gf_rename_resource_t;

struct _gf_bo_alloc_arg
{
    unsigned int    width;
    unsigned int    height;
    unsigned int    usage_mask;
    gf_format      format;
    gf_access_hint access_hint;
    unsigned int    unpagable      :1;
    unsigned int    tiled          :1;
    unsigned int    primary        :1;
    unsigned int    compressed     :1;
    void *          user_ptr;
    unsigned int    user_buf_size;
};

struct _gf_bo
{
    struct hlist_node link;
    gf_bufmgr_t    *bufmgr;

    unsigned int    refcount;
    unsigned int    mapcount;

    unsigned int    size;
    unsigned int    width;
    unsigned int    height;

    unsigned int    alignment;

    unsigned int    hw_format;
    unsigned int    compress_format;
    unsigned int    bl_slot_index;

    unsigned int    unpagable   :1;
    unsigned int    tiled       :1;
    unsigned int    secured     :1;
    unsigned int    has_pages   :1;
    unsigned int    cpu_visible :1;
    unsigned int    force_clear :1;
    unsigned int    maybe_shared:1;

    unsigned int    bit_cnt;
    unsigned int    pitch;
    unsigned long long    gpu_virt_addr;  // gpu phy address
    unsigned long long    cpu_phy_addr;
    unsigned int    allocation;
    unsigned int    name;
    unsigned long long  gtt_offset;

    unsigned int    sync_obj;
    unsigned long long    fence_addr;

    unsigned int    segment_id;

    unsigned int    use_aperture;
    void            *virt_addr;  // mapped user space address
    int             fourCC;         //for video
    int             invalid_layout;

    struct list_head res_link;
    gf_resource_t  *resource;      // bind to one resource

    struct list_head rename_link;
    gf_rename_resource_t *rename;
    gf_bo_alloc_arg_t alloc_arg;

    unsigned int     core_handle;   // should only used for debug
};


typedef union map_flags {
    struct {
        unsigned int acquire_aperture       :1;
        unsigned int read_only              :1;
        unsigned int write_only             :1;
        unsigned int cache_type             :3;
        unsigned int discard                :1;
        unsigned int no_existing_reference  :1;
        unsigned int delay_map              :1;
    };
    unsigned int value;
} map_flags;

typedef struct
{
    map_flags flags;
    void *virt_addr;
    unsigned int allocation;
    unsigned int prefault_num;
} map_args_t;

struct _gf_resource
{
    gf_bufmgr_t        *bufmgr;
    unsigned int        handle;
    struct list_head    link;
    struct list_head    bo_list;
};

struct _gf_rename_resource
{
    unsigned int        max_length;

    unsigned int        allocation_num;

    struct list_head    reference_list;
    struct list_head    unreference_list;
};

#define BO_HASH_SIZE    (32)
#define BO_HASH(handle) ((unsigned int)(handle) % BO_HASH_SIZE)
struct _gf_bufmgr
{
    int fd;
    int enable_lock;
    unsigned int hDevice;

    int refcount;
    struct list_head link;

    pthread_mutex_t mutex;
    unsigned int resource_id;
    struct list_head resource_list;
    struct hlist_head bo_hash[BO_HASH_SIZE];
};

#ifdef __INTERFACE_IMPLEMENT_DEFINED__

EXTERN_C gf_bufmgr_t *gf_bufmgr_init(int fd);
EXTERN_C void gf_bufmgr_destroy(gf_bufmgr_t *bufmgr);

EXTERN_C gf_bo_t *gf_bo_alloc(gf_bufmgr_t *bufmgr, gf_bo_alloc_arg_t *arg);
EXTERN_C gf_bo_t *gf_bo_rename_alloc(gf_bufmgr_t *bufmgr, gf_bo_t *ref);
EXTERN_C gf_bo_t *gf_bo_get_from_handle(gf_bufmgr_t *bufmgr, unsigned int handle);
EXTERN_C gf_bo_t *gf_bo_create_from_handle(gf_bufmgr_t *bufmgr, unsigned int handle, int width, int height, int pitch, int bit_cnt, int format);
EXTERN_C gf_bo_t *gf_bo_create_from_userptr(gf_bufmgr_t *bufmgr, void *userptr, int size, int width, int height, int pitch, int format);
EXTERN_C gf_bo_t *gf_bo_create_from_fd(gf_bufmgr_t *bufmgr, int fd, int width, int height, int pitch, int bit_cnt, int format);
EXTERN_C int gf_bo_relayout(gf_bo_t *bo, int width, int height, int pitch, int bit_cnt, int format);

EXTERN_C void gf_bo_reference(gf_bo_t *bo);
EXTERN_C void gf_bo_unreference(gf_bo_t *bo);
EXTERN_C int gf_bo_is_busy(gf_bo_t *bo);
EXTERN_C int gf_bo_map(gf_bo_t *bo, map_args_t *arg);
EXTERN_C int gf_bo_unmap(gf_bo_t *bo);
EXTERN_C int gf_bo_get_fd(gf_bo_t *bo, int *fd);
EXTERN_C void gf_bo_wait_idle(gf_bo_t *bo);
EXTERN_C int gf_bo_dump_bmp(gf_bo_t *bo, const char *filename);

EXTERN_C unsigned int gf_bo_get_name(gf_bo_t *bo);
EXTERN_C gf_bo_t *gf_bo_create_from_name(gf_bufmgr_t *bufmgr, unsigned int name, int width, int height, int pitch, int bit_cnt, int format);

EXTERN_C int gf_bo_create_resource (
        gf_bufmgr_t           *bufmgr,
        unsigned int           device,
        unsigned int           allocation_num,
        gf_create_alloc_info_t *allocation_info,
        unsigned int           create_resource_flag,
        unsigned int           *resource_handle
);

EXTERN_C int gf_bo_destroy_resource(
        gf_bufmgr_t    *bufmgr,
        unsigned int    device,
        unsigned int    allocation_num,
        unsigned int    *allocation_info,
        unsigned int    resource_handle
);

#else

#define gf_bufmgr_init bufmgr_interface_v2->bufmgr_init
#define gf_bufmgr_destroy bufmgr_interface_v2->bufmgr_destroy
#define gf_bo_alloc bufmgr_interface_v2->bo_alloc
#define gf_bo_rename_alloc bufmgr_interface_v2->bo_rename_alloc
#define gf_bo_get_from_handle bufmgr_interface_v2->bo_get_from_handle
#define gf_bo_create_from_handle bufmgr_interface_v2->bo_create_from_handle
#define gf_bo_create_from_userptr bufmgr_interface_v2->bo_create_from_userptr
#define gf_bo_create_from_fd bufmgr_interface_v2->bo_create_from_fd
#define gf_bo_relayout bufmgr_interface_v2->bo_relayout
#define gf_bo_reference bufmgr_interface_v2->bo_reference
#define gf_bo_unreference bufmgr_interface_v2->bo_unreference
#define gf_bo_is_busy bufmgr_interface_v2->bo_is_busy
#define gf_bo_map bufmgr_interface_v2->bo_map
#define gf_bo_unmap bufmgr_interface_v2->bo_unmap
#define gf_bo_get_fd bufmgr_interface_v2->bo_get_fd
#define gf_bo_wait_idle bufmgr_interface_v2->bo_wait_idle
#define gf_bo_dump_bmp bufmgr_interface_v2->bo_dump_bmp
#define gf_bo_get_name bufmgr_interface_v2->bo_get_name
#define gf_bo_create_from_name bufmgr_interface_v2->bo_create_from_name
#define gf_bo_create_resource bufmgr_interface_v2->bo_create_resource
#define gf_bo_destroy_resource bufmgr_interface_v2->bo_destroy_resource

#endif

struct bufmgr_interface {
    gf_bufmgr_t * (*bufmgr_init) (int fd);
    void  (*bufmgr_destroy) (gf_bufmgr_t *bufmgr);
    gf_bo_t * (*bo_alloc) (gf_bufmgr_t *bufmgr, gf_bo_alloc_arg_t *arg);
    gf_bo_t * (*bo_rename_alloc) (gf_bufmgr_t *bufmgr, gf_bo_t *ref);
    gf_bo_t * (*bo_get_from_handle) (gf_bufmgr_t *bufmgr, unsigned int handle);
    gf_bo_t * (*bo_create_from_handle) (gf_bufmgr_t *bufmgr, unsigned int handle, int width, int height, int pitch, int bit_cnt, int format);
    gf_bo_t * (*bo_create_from_userptr) (gf_bufmgr_t *bufmgr, void *userptr, int size, int width, int height, int pitch, int format);
    gf_bo_t * (*bo_create_from_fd) (gf_bufmgr_t *bufmgr, int fd, int width, int height, int pitch, int bit_cnt, int format);
    int  (*bo_relayout) (gf_bo_t *bo, int width, int height, int pitch, int bit_cnt, int format);
    void  (*bo_reference) (gf_bo_t *bo);
    void  (*bo_unreference) (gf_bo_t *bo);
    int  (*bo_is_busy) (gf_bo_t *bo);
    int  (*bo_map) (gf_bo_t *bo, map_args_t *arg);
    int  (*bo_unmap) (gf_bo_t *bo);
    int  (*bo_get_fd) (gf_bo_t *bo, int *fd);
    void  (*bo_wait_idle) (gf_bo_t *bo);
    int  (*bo_dump_bmp) (gf_bo_t *bo, const char *filename);
    unsigned int  (*bo_get_name) (gf_bo_t *bo);
    gf_bo_t * (*bo_create_from_name) (gf_bufmgr_t *bufmgr, unsigned int name, int width, int height, int pitch, int bit_cnt, int format);

    int (*bo_create_resource) (
        gf_bufmgr_t           *bufmgr,
        unsigned int           device,
        unsigned int           allocation_num,
        gf_create_alloc_info_t *allocation_info,
        unsigned int           create_resource_flag,
        unsigned int           *resource_handle
    );

    int (*bo_destroy_resource)(
        gf_bufmgr_t    *bufmgr,
        unsigned int    device,
        unsigned int    allocation_num,
        unsigned int    *allocation_info,
        unsigned int    resource_handle
    );
};
EXTERN_C struct bufmgr_interface *bufmgr_interface_v2;

#endif

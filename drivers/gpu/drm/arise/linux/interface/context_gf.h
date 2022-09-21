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

#ifndef __CONTEXT_GF_H__
#define __CONTEXT_GF_H__

#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif
#endif

#include <stddef.h>
#include <stdarg.h>
#include "gf_list.h"

typedef struct _GF_COMMAND_HEADER
{
    unsigned int        unused[3];  // this pad just for easy check dma after hang (skip first 4 dwords)
    struct
    {
        unsigned int    Flag2dCmd    :1; // Xserver 2d command
        unsigned int    Flag3dbltCmd :1; // Xserver composite 3d blt command
        unsigned int    ContainLpdpCmd :1;
    };
} GF_COMMAND_HEADER;

typedef enum
{
    GF_CMD_HINT_2D       = 1,
    GF_CMD_HINT_3DBLT    = 2,
    GF_CMD_HINT_LPDPBLT  = 3
}GF_CMD_HINT;

typedef struct
{
    void *ptr;
    int size;
    struct list_head link;

    void (*free)(void *arg);
    void *arg;
} buf_node_t;

#define HOOK_BEFORE_FLUSH   1
#define HOOK_AFTER_FLUSH    2
#define HOOK_ADD_ALLOCATION 3

typedef struct _context_gf
{
    int                     fd;
    unsigned int            device;
    unsigned int            context;

    int                     engine_index;
    unsigned int            *command_buffer;

    struct _GF_COMMAND_HEADER       *header;
    struct gf_allocation_list       *allocation_list;
    struct gf_patchlocation_list    *patch_location_list;
    struct gf_syncobj_list          *sync_object_list;

    int                     in_request;
    int                     command_buffer_size;
    int                     allocation_list_size;
    int                     patch_location_list_size;
    int                     sync_object_list_size;

    int                     split_offset;
    int                     command_buffer_free_size;
    int                     allocation_list_free_index;
    int                     patch_location_list_free_index;
    int                     sync_object_list_free_index;

    int                     cmdbuf_size;
    int                     cmdbuf_list_cnt;
    struct list_head        cmdbuf_list;

    struct list_head        free_node_list;
    buf_node_t              cache[64];

    void**                  patch_buffer;   /* offset:value pair */
    int                     patch_cnt;

    int                     flush_count;
    int                     freeze_count;
    struct list_head        hook_list;    // callback list
}context_gf_t;

#ifdef __INTERFACE_IMPLEMENT_DEFINED__

EXTERN_C context_gf_t *gf_create_context(int fd, unsigned int device, int engine_index);
EXTERN_C void           gf_destroy_context(context_gf_t *context);
EXTERN_C unsigned int  *gf_context_request_space(context_gf_t *context, int cmd_size, int cmd_hint);

EXTERN_C void           gf_context_add_buffer(context_gf_t *context, void *cmdbuf, int cmd_size, 
                            int cmd_hint, void (*free)(void *), void *);

EXTERN_C void           gf_context_release_space(context_gf_t *context, unsigned int *last_cmd);
EXTERN_C void           gf_context_add_allocation(
                            context_gf_t *context, unsigned int allocation,
                            unsigned int *ref_cmd, unsigned int allocation_offset,
                            int slot_ID, int pagable, int write_op, int driver_ID);
EXTERN_C void           gf_context_add_sync_object(
                            context_gf_t *context, unsigned int sync_object,
                            unsigned int *fence_location, unsigned long long fence_value);
EXTERN_C void           gf_context_flush(context_gf_t *context);

EXTERN_C void           gf_context_add_hook(context_gf_t *context, int type, void (*proc)(context_gf_t*, void*, va_list), void *arg);

EXTERN_C int            gf_context_remove_hook(context_gf_t *context, int type, void *proc);

EXTERN_C int            gf_context_freeze(context_gf_t *context);
EXTERN_C int            gf_context_unfreeze(context_gf_t *context);

#else

#define gf_create_context context_interface_v2->create_context
#define gf_destroy_context context_interface_v2->destroy_context
#define gf_context_request_space context_interface_v2->context_request_space
#define gf_context_release_space context_interface_v2->context_release_space
#define gf_context_flush context_interface_v2->context_flush
#define gf_context_add_buffer context_interface_v2->context_add_buffer
#define gf_context_add_allocation context_interface_v2->context_add_allocation
#define gf_context_add_sync_object context_interface_v2->context_add_sync_object

#define gf_context_add_hook context_interface_v2->context_add_hook
#define gf_context_remove_hook context_interface_v2->context_remove_hook

#define gf_context_freeze    context_interface_v2->context_freeze
#define gf_context_unfreeze    context_interface_v2->context_unfreeze

#endif

struct context_interface {
    context_gf_t * (*create_context) (int fd, unsigned int device, int engine_index);
    void            (*destroy_context) (context_gf_t *context);
    unsigned int  * (*context_request_space) (context_gf_t *context, int cmd_size, int cmd_hint);
    void            (*context_release_space) (context_gf_t *context, unsigned int *last_cmd);
    void            (*context_flush) (context_gf_t *context);
    void           (*context_add_buffer)(context_gf_t *context, void *cmdbuf, int cmd_size, 
                            int cmd_hint, void (*free)(void *), void *);
    void           (*context_add_allocation)(
                            context_gf_t *context, unsigned int allocation,
                            unsigned int *ref_cmd, unsigned int allocation_offset,
                            int slot_ID, int pagable, int write_op, int driver_ID);
    void           (*context_add_sync_object)(
                            context_gf_t *context, unsigned int sync_object,
                            unsigned int *fence_location, unsigned long long fence_value);

    void           (*context_add_hook)(context_gf_t *context, int type, void (*proc)(context_gf_t*, void*, va_list), void *arg);
    int            (*context_remove_hook)(context_gf_t *context, int type, void *proc);
    int            (*context_freeze)(context_gf_t *context);
    int            (*context_unfreeze)(context_gf_t *context);
};
EXTERN_C struct context_interface *context_interface_v2;

#endif /*__CONTEXT_GF_H__*/

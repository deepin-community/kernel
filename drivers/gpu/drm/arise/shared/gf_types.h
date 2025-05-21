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
#ifndef __GF_TYPES_H__
#define __GF_TYPES_H__

#include "gf_def.h"

typedef enum
{
    GF_USAGE_DISPLAY_SURFACE   = 0x00000001,
    GF_USAGE_OVERLAY_SURFACE   = 0x00000002,
    GF_USAGE_CURSOR_SURFACE    = 0x00000004,
    GF_USAGE_RENDER_TARGET     = 0x00000008,
    GF_USAGE_TEXTURE_BUFFER    = 0x00000010,
    GF_USAGE_SHADER_BUFFER     = 0x00000020,
    GF_USAGE_DATA_BUFFER       = 0x00000040,
    GF_USAGE_TEMP_BUFFER       = 0x00000080,
    GF_USAGE_COMPOSER_SURFACE  = 0x00000100,
    GF_USAGE_SHADOW_SURFACE    = 0x00000200,
    GF_USAGE_VIDEO             = 0x00000400,
    GF_USAGE_VIDEO_DECODER     = 0x00000400,
    GF_USAGE_CAMERA            = 0x00000800,
    GF_USAGE_VIDEO_ENCODER     = 0x00001000,
    GF_USAGE_RENDER_SCRIPT     = 0x00002000,

    GF_USAGE_CREATE_PIXMAP     = 0x01000000,
    GF_USAGE_MIU_COUNTER       = 0x02000000,

    GF_USAGE_FORCE_PCIE        = 0x00004000,
    GF_USAGE_FORCE_LOCAL       = 0x00008000,
    GF_USAGE_HWCTX_BUFFER      = 0x00010000,
    GF_USAGE_FRAMEBUFFER       = 0x08000000,
    GF_USAGE_TRY_TILED         = 0x80000000,
}gf_usage_bit;

typedef enum
{
    GF_ACCESS_UNKNOWN    = 0,
    GF_ACCESS_GPU_ONLY   = 1,
    GF_ACCESS_GPU_ALMOST,
    GF_ACCESS_CPU_ALMOST,
//    GF_ACCESS_CPU_ONLY,
}gf_access_hint;

/* GF Format description:
    All the format define as big endian byte order
    the first named component is in the least-significant bits
    This format  as same as HSF_XXX
    For example:
    GF_FORMAT_R8G8B8A8_UNORM
        char *pR8G8B8A8 =
        pR8G8B8A8[0] = r;  //R
        pR8G8B8A8[1] = g; //G
        pR8G8B8A8[2] = b; //B
        pR8G8B8A8[3] = a; //A

    GF_FORMAT_B5G6R5_UNORM
    unsigned short * p565Color
        *p565Color = b | r << 5 | g << 11;

*/
typedef enum
{
    GF_FORMAT_A8_UNORM       = 1,
    GF_FORMAT_B5G6R5_UNORM   = 2,        /*rmask 0xf800; gmask 0x7e0; bmask 0x1f*/
    GF_FORMAT_B5G5R5A1_UNORM = 3,
    GF_FORMAT_A1B5G5R5_UNORM = 4,
    GF_FORMAT_B4G4R4A4_UNORM = 5,
    GF_FORMAT_A4B4G4R4_UNORM = 6,
    GF_FORMAT_B8G8R8A8_UNORM = 7,      /*rmask 0x00ff0000; gmask 0x0000ff00; bmask 0x000000ff; amask 0xff000000*/
    GF_FORMAT_B8G8R8X8_UNORM = 8,
    GF_FORMAT_R8G8B8A8_UNORM = 9,      /*rmask 0x000000ff; gmask 0x0000ff00; bmask 0x00ff0000; amask 0xff000000*/
    GF_FORMAT_R8G8B8X8_UNORM = 10,
    GF_FORMAT_A8R8G8B8_UNORM = 11,
    GF_FORMAT_YUY2           = 12,
    GF_FORMAT_NV12_LINEAR    = 13,
    GF_FORMAT_NV12_TILED     = 14,
    GF_FORMAT_NV21_LINEAR    = 15,
    GF_FORMAT_YV12           = 16,
    GF_FORMAT_FLOAT32        = 17,
    GF_FORMAT_UINT32         = 18,
    GF_FORMAT_INT32          = 19,
    GF_FORMAT_R8_UNORM       = 20,

    GF_FORMAT_RAW10          = 21,
    GF_FORMAT_RAW16          = 22,
    GF_FORMAT_RAW_OPAQUE     = 23,
    GF_FORMAT_B10G10R10A2_UNORM = 24,
    GF_FORMAT_R8G8_UNORM     = 25,
    GF_FORMAT_R16_UNORM     = 26,
    GF_FORMAT_R16G16_UNORM     = 27,
    GF_FORMAT_P010             = 28,
    GF_FORMAT_L8_UNORM         = 29,
    GF_FORMAT_L8A8_UNORM       = 30,
    GF_FORMAT_R8G8B8_UNORM     = 31,
    GF_FORMAT_R4G4B4A4_UNORM   = 32,
    GF_FORMAT_R5G5B5A1_UNORM   = 33,
    GF_FORMAT_D16_UNORM        = 34,
    GF_FORMAT_S8_UINT          = 35,

    GF_FORMAT_B8G8R8A8_SRGB    = 36,
    GF_FORMAT_AYUV_VIDEO       = 37,
}gf_format;

typedef struct
{
    unsigned int device;            /* in */
    unsigned int reference;         /* in, for rename create, if set this field, ignore all follow input field */
    unsigned int width;             /* in */
    unsigned int height;            /* in */
    unsigned int usage_mask;        /* in */
    gf_format   format;            /* in */
    gf_ptr64_t   user_ptr;         /* in */
    unsigned int user_buf_size;     /* in */
    gf_access_hint access_hint;    /* in */
    unsigned int fence_sync  : 1;   /* in */
    unsigned int unpagable   : 1;   /* in/out */
    unsigned int tiled       : 1;   /* in/out */
    unsigned int secured     : 1;   /* in/out */
    unsigned int compressed  : 1;   /* in/out */
    unsigned int primary     : 1;   /* in/out */
    unsigned int has_pages   : 1;   /* out */
    unsigned int force_clear : 1;   /* out */

    unsigned int hw_format;         /* out */
    unsigned int size;              /* out */
    unsigned int width_aligned;     /* out */
    unsigned int height_aligned;    /* out */
    unsigned int bit_cnt;           /* out */
    unsigned int pitch;             /* out */
    unsigned int tiled_width;       /* out */
    unsigned int tiled_height;      /* out */
    unsigned int allocation;        /* out */

    unsigned int sync_obj;          /* out */
    unsigned int compress_format;   /* out */
    unsigned int buffer_fd;         /* out */
    unsigned int segment_id;        /* out */
    unsigned long long gpu_virt_addr;     /* out */
    unsigned long long fence_addr;        /* out */
}gf_create_allocation_t;

typedef struct
{
    unsigned int device;            /* in */
    unsigned int allocation;        /* in */

    unsigned int size;              /* out */
    unsigned int alignment;         /* out */
    unsigned int width;             /* out */
    unsigned int height;            /* out */
    unsigned int aligned_width;     /* out */
    unsigned int aligned_height;    /* out */
    unsigned int tiled_width;       /* out */
    unsigned int tiled_height;      /* out */
    unsigned int bit_cnt;           /* out */
    unsigned int pitch;             /* out */
    unsigned int unpagable : 1;     /* out */
    unsigned int tiled     : 1;     /* out */
    unsigned int secured   : 1;     /* out */
    unsigned int snoop     : 1;     /* out */
    unsigned int local     : 1;     /* out */
    unsigned int ForceLocal: 1;     /* out */
    unsigned int has_pages : 1;     /* out */
    unsigned int cpu_visible    :1; /* out */
    unsigned int force_clear : 1;   /* out */

    unsigned int compress_format;   /* out */
    unsigned int hw_format;         /* out */
    unsigned int segment_id;        /* out */
    unsigned int bl_slot_index;     /* out */

    unsigned int sync_obj;          /* out */
    unsigned long long gpu_virt_addr;     /* out */
    unsigned long long cpu_phy_addr;      /* out */
    unsigned long long fence_addr;        /* out */
}gf_open_allocation_t;

typedef struct
{
    unsigned int device;            /* in */
    unsigned int allocation;        /* in */
}gf_destroy_allocation_t;

typedef struct
{
    unsigned int device;         /* out */
}gf_create_device_t;

typedef struct
{
    unsigned int  device;                   /* in   */
    unsigned int  engine_index;             /* in   */
    unsigned int  flags;                    /* in   */
    unsigned int  context;                  /* out  */
}gf_create_context_t;

typedef struct
{
    unsigned int device;              /* in   */
    unsigned int context;             /* in   */
}gf_destroy_context_t;

typedef struct
{
    unsigned int  device;              /* in   */
    unsigned int  stream_id;           /* in video stream id */
    unsigned int  context;             /* out  */
    unsigned int  hw_idx;              /* out  */
}gf_create_di_context_t;

typedef struct
{
    unsigned int device;              /* in   */
    unsigned int context;             /* in   */
}gf_destroy_di_context_t;

typedef struct
{
    gf_ptr64_t     ptr;
    unsigned int    size;
    unsigned int    pad;
}gf_cmdbuf_t;

typedef struct
{
    unsigned int    ValidHead;
    unsigned int    HWCtxBufIndex;
    unsigned int    Pad;                            // for alignment request
    unsigned int    GF_Interface_Version;

    union
    {
        struct
        {
            unsigned int    Contain3dCmd                :1; // keep consistent with source_new, not used for elite
            unsigned int    ContainLpdpCmd              :1;
            unsigned int    CLCSOnly                    :1; // opencl without 2D/3D blt/L2 invalidate
            unsigned int    Flag2dCmd                   :1; // Xserver 2d command
            unsigned int    Flag3dbltCmd                :1; // Xserver composite 3d blt command
            unsigned int    DvfsForceLevel              :3;
            unsigned int    InitializeContext           :1;
            unsigned int    ContainDIP                  :1;
            unsigned int    TraceDMA                    :1;

            unsigned int    resize_command_buffer       :1; /* resize command buffer after render */
            unsigned int    resize_allocation_list      :1; /* resize allocation list after render */
            unsigned int    resize_patch_location_list  :1; /* resize patch location list after render */
            unsigned int    null_rendering              :1; /* null rendering, refer to WDK */
            unsigned int    normal_recovery             :1; /* recovery normally if engine hang */
            unsigned int    ForceSlice                  :1; /* ForceSlice */
            unsigned int    Reserved                    :15;
        };
        unsigned int flags;
    };
} gf_render_flags_t;

typedef struct gf_allocation_list
{
    unsigned int       hAllocation;
    union
    {
        struct
        {
            unsigned int     WriteOperation       : 1;    // 0x00000001
            unsigned int     DoNotRetireInstance : 1;    // 0x00000002
            unsigned int     Reserved             :30;    // 0xFFFFFFFC
        };
        unsigned int         Value;
    };
} gf_allocation_list_t;

typedef struct gf_patchlocation_list
{
    unsigned int AllocationIndex;
    union {
        struct
        {
            unsigned int SlotId          : 24;   // 0x00FFFFFF
            unsigned int Reserved        : 8;    // 0xFF000000
        };
        unsigned int     Value;
    };

    unsigned int     DriverId;
    unsigned int     AllocationOffset;
    unsigned int     PatchOffset;
    unsigned int     SplitOffset;
} gf_patchlocation_list_t;

typedef struct gf_syncobj_list
{
    unsigned int hSyncObject;
    unsigned int PatchOffset;
    unsigned long long FenceValue;
} gf_syncobj_list_t;

typedef struct
{
    unsigned int            context;                        /* in                   */
    unsigned int            command_length;                 /* in(number of bytes)  */
    unsigned int            allocation_count;               /* in                   */
    unsigned int            patch_location_count;           /* in                   */
    unsigned int            sync_object_count;              /* in                   */
    unsigned int            cmdbuf_count;                   /* in                   */
    gf_render_flags_t       flags;
    unsigned int            pad;
    gf_ptr64_t              allocation_list;
    gf_ptr64_t              patch_location_list;
    gf_ptr64_t              sync_object_list;
    gf_ptr64_t              cmdbuf_array;
}gf_render_t;

/**************fence sync object relate***************/
#define GF_SYNC_OBJ_TYPE_MUTEX            1
#define GF_SYNC_OBJ_TYPE_SEMPAPHORE       2
#define GF_SYNC_OBJ_TYPE_FENCE            3
#define GF_SYNC_OBJ_TYPE_CPU_NOTIFICATION 4
#define GF_SYNC_OBJ_TYPE_DMAFENCE         15
typedef struct
{
    unsigned int       type;
    unsigned int       device;                                /* in  */
    unsigned int       fence_sync_object;                     /* out */
    unsigned int       pad;
    union {
        struct {
            unsigned long long init_value;                    /* in  */
            unsigned long long fence_addr;                    /* out */
        } fence;

        struct {
            unsigned int        init_state;
        } mutex;

        struct {
            unsigned int        event;
        } cpu_notification;

        struct {
            unsigned int        max_count;
            unsigned int        init_count;
        } semaphore;
    };
}gf_create_fence_sync_object_t;


typedef struct
{
    unsigned int       device;                   /* in */
    unsigned int       fence_sync_object;        /* in */
}gf_destroy_fence_sync_object_t;

#define GF_SYNC_OBJ_ERROR                -1
#define GF_SYNC_OBJ_INVALID_ARGU         -2
#define GF_SYNC_OBJ_ALREAD_SIGNALED       1
#define GF_SYNC_OBJ_TIMEOUT_EXPIRED       2
#define GF_SYNC_OBJ_CONDITION_SATISFIED   3
#define GF_SYNC_OBJ_WAIT_ON_SERVER        4
#define GF_SYNC_OBJ_UNSIGNALED            5

typedef struct
{
    unsigned int       context;                  /* in */
    unsigned int       fence_sync_object;        /* in */
    unsigned long long timeout;                  /* in nano_sec */  //timeout == 0, no wait just return current object status
    unsigned long long fence_value;              /* in */
    unsigned int       server_wait;              /* in */
    int                status;                   /* out */
}gf_wait_fence_sync_object_t;

#define GF_SET_FENCE_VALUE   0x01
#define GF_GET_FENCE_VALUE   0x02

typedef struct
{
    unsigned int       device;                   /* in */
    unsigned int       opcode;                   /* in */
    unsigned int       fence_sync_object;        /* in */
    int                appid;                    /* out */
    int                flip_limit;               /* out */
    int                discard_sync;             /* in & out*/
    unsigned long long fence_value;              /* out */
}gf_fence_value_t;

typedef struct
{
    unsigned int       context;                  /* in */
    unsigned int       server_wait;              /* in */
    unsigned long long timeout;                  /* in nano_sec */  //timeout == 0, no wait just return current object status
    int                status;                   /* out */
    int                pad;                   /* out */
}gf_wait_fence_t;

#define gf_align_down(val, align)  ((val) & ~((align) - 1))
#define gf_align_up(val, align)    (((val) + ((align) - 1)) & ~((align) - 1))

typedef enum
{
    PDISCARD  =  0,
    PLOW,
    PNORMAL,
    PHIGH,
    PMAX,
    PALL,
} gf_allocation_priority_t;

typedef struct
{
    unsigned int        Size;               // inout
    unsigned int        HwFormat;           // in
    unsigned int        HWCompressFormat;   // in
    unsigned int        AtType;             // in
    unsigned int        Priority;           // in
    unsigned int        Alignment;          // in
    unsigned int        PreferredSegment;   // in
    unsigned int        BitCount;           // in
    unsigned int        Width;              // in
    unsigned int        Height;             // in
    unsigned int        Pitch;              // in

    unsigned int        MaximumRenamingListLength;  // in

    unsigned int        bSecurity   :1;     // in
    unsigned int        CpuVisible  :1;     // in
    unsigned int        Primary     :1;     // in
    unsigned int        Swizzled    :1;     // in
    unsigned int        Video       :1;     // in
    unsigned int        Overlay     :1;     // in
    unsigned int        Camera      :1;     // in
    unsigned int        bNonSnoop   :1;     // in
    unsigned int        bVideoInternal  :1; // in
    unsigned int        Unpagable   :1;     // in
    unsigned int        FenceSync   :1;     // in, TODO(deleted)

    unsigned int        hAllocation;        // out
    unsigned int        FenceSyncObject;    // out
    unsigned int        pad;
    unsigned long long  FenceAddress;       // out

    unsigned long long  PhysAddr;           // out
    unsigned long long  Address;            // out
} gf_create_alloc_info_t;

typedef struct
{
    unsigned int        device;
    unsigned int        NumAllocations;
    gf_ptr64_t          pAllocationInfo;
} gf_create_resource_t;

typedef struct
{
    unsigned int handle;    // in
    unsigned int offset;    // in
    unsigned int size;      // in
    unsigned int readonly:1;// in
} gf_drm_gem_begin_cpu_access_t;

typedef struct
{
    unsigned int handle;
} gf_drm_gem_end_cpu_access_t;

typedef struct
{
    unsigned int gem_handle;
    unsigned int pad;
    unsigned long long offset;
    unsigned int prefault_num;
    unsigned int delay_map;
} gf_drm_gem_map_t;

typedef struct
{
    unsigned int crtc_id;   /* in */
    unsigned int pipe;      /* out */
} gf_kms_get_pipe_from_crtc_t;

typedef struct
{
    unsigned int    device;                /* in */
    unsigned int    context;               /* in */
    unsigned int    hw_ctx_buf_index;          /* in */
} gf_add_hw_ctx_buf_t;

typedef struct
{
    unsigned int    device;                /* in */
    unsigned int    context;               /* in */
    unsigned int    hw_ctx_buf_index;          /* in */
} gf_rm_hw_ctx_buf_t;

#define GF_MISC_GET_3DBLT_CODE  (1)
typedef struct
{
    unsigned int size;          // output
    unsigned int capacity;      // output
    unsigned long long base;    // output
} gf_misc_get_3dblt_code_t;

#define GF_MISC_SET_3DBLT_CODE  (2)
typedef struct
{
    unsigned int size;          // input
    unsigned int pad;           // for alignment
    gf_ptr64_t data;            // input
} gf_misc_set_3dblt_code_t;

#define GF_MISC_MAP_GPUVA       (3)
typedef struct
{
    unsigned int allocation;    // input
    unsigned int pad;           // for alignment
    unsigned long long gpu_va;  // input
} gf_misc_map_gpuva_t;

#define GF_MISC_VIDEO_FENCE_GET (4)
typedef struct
{
    unsigned int index;         // input
    unsigned int fence;         // output
} gf_misc_video_fence_get_t;

#define GF_MISC_VIDEO_FENCE_CLEAR (5)
typedef struct
{
    unsigned int index; // input
} gf_misc_video_fence_clear_t;

typedef struct
{
    unsigned int op_code;
    unsigned int device;
    unsigned int context;
    unsigned int pad;   // for alignment
    union {
        gf_misc_get_3dblt_code_t    get_3dblt_code;
        gf_misc_set_3dblt_code_t    set_3dblt_code;
        gf_misc_map_gpuva_t         map_gpuva;
        gf_misc_video_fence_get_t   video_fence_get;
        gf_misc_video_fence_clear_t video_fence_clear;
    };
} gf_cil2_misc_t;

#define GF_DEBUGFS_GEM_ENABLE  0x1
#define GF_DEBUGFS_FAKE_HANG   0x2

#endif /* __GF_TYPES_H__*/


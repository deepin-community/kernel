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

#include "gf_version.h"

#undef TRACE_SYSTEM
#define TRACE_SYSTEM DRIVER_NAME

#if !defined(_GFX_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _GFX_TRACE_H

#if GF_TRACE_EVENT
#include <linux/tracepoint.h>
#include "gf_fence.h"
#include "gf_gem.h"
#include "gf_kms.h"


TRACE_EVENT(gfx_task_create,

    TP_PROTO(int engine_index, unsigned int context, unsigned long long task_id, unsigned int task_type),

    TP_ARGS(engine_index, context, task_id, task_type),

    TP_STRUCT__entry(
        __field(int, engine_index)
        __field(unsigned int, context)
        __field(unsigned long long, task_id)
        __field(unsigned int, task_type)
    ),

    TP_fast_assign(
        __entry->engine_index = engine_index;
        __entry->context = context;
        __entry->task_id = task_id;
        __entry->task_type = task_type;
   ),

    TP_printk("engine_index=%d, context=0x%x, task_id=%llu, task_type=%u",
        __entry->engine_index, __entry->context, __entry->task_id, __entry->task_type)
);

TRACE_EVENT(gfx_task_submit,

    TP_PROTO(int engine_index, unsigned int context, unsigned long long task_id, unsigned int task_type, unsigned long long fence_id, unsigned int args),

    TP_ARGS(engine_index, context, task_id, task_type, fence_id, args),

    TP_STRUCT__entry(
        __field(int, engine_index)
        __field(unsigned int, context)
        __field(unsigned long long, task_id)
        __field(unsigned int, task_type)
        __field(unsigned long long, fence_id)
        __field(unsigned int, args)
    ),

    TP_fast_assign(
        __entry->engine_index = engine_index;
        __entry->context = context;
        __entry->task_id = task_id;
        __entry->task_type = task_type;
        __entry->fence_id = fence_id;
        __entry->args = args;
   ),

    TP_printk("engine_index=%d, context=0x%x, task_id=%llu, task_type=%u, fence_id=%llu, args=%u",
        __entry->engine_index, __entry->context, __entry->task_id, __entry->task_type, __entry->fence_id, __entry->args)
);

TRACE_EVENT(gfx_fence_back,

    TP_PROTO(unsigned long engine_index, unsigned long long fence_id),

    TP_ARGS(engine_index, fence_id),

    TP_STRUCT__entry(
        __field(unsigned long, engine_index)
        __field(unsigned long long, fence_id)
    ),

    TP_fast_assign(
        __entry->engine_index = engine_index;
        __entry->fence_id = fence_id;
    ),

    TP_printk("engine_index=%lu, fence_id=%llu", __entry->engine_index, __entry->fence_id)
);

TRACE_EVENT(gfx_vblank_intrr,
    TP_PROTO(unsigned int index, unsigned int cnt),

    TP_ARGS(index, cnt),

    TP_STRUCT__entry(
        __field(unsigned int, index)
        __field(unsigned int, cnt)
    ),

    TP_fast_assign(
        __entry->index = index;
        __entry->cnt = cnt;
    ),

    TP_printk("crtc_index=%d, vbl_count=%d", __entry->index, __entry->cnt)
);

TRACE_EVENT(gfx_vblank_onoff,
    TP_PROTO(int index, int on),

    TP_ARGS(index, on),

    TP_STRUCT__entry(
        __field(int, index)
        __field(int, on)
    ),

    TP_fast_assign(
        __entry->index = index;
        __entry->on = on;
    ),

    TP_printk("crtc_index=%d, vblank on=%d", __entry->index, __entry->on)
);

TRACE_EVENT(gfx_drm_gem_create_object,
        TP_PROTO(long long card_index, struct drm_gf_gem_object *obj),
        TP_ARGS(card_index, obj),

        TP_STRUCT__entry(
            __field(unsigned long long, allocation)
            __field(unsigned int, size)
            __field(unsigned int, width)
            __field(unsigned int, height)
            __field(unsigned int, bit_cnt)
            __field(unsigned int, pitch)
            __field(unsigned int, segment_id)
            __field(unsigned int, hw_format)
            __field(unsigned int, compress_format)
            __field(unsigned int, tiled)
            ),

        TP_fast_assign(
            __entry->allocation = card_index << 32 | obj->core_handle;
            __entry->size = obj->info.size;
            __entry->width = obj->info.width;
            __entry->height = obj->info.height;
            __entry->bit_cnt = obj->info.bit_cnt;
            __entry->pitch = obj->info.pitch;
            __entry->segment_id = obj->info.segment_id;
            __entry->hw_format = obj->info.hw_format;
            __entry->compress_format = obj->info.compress_format;
            __entry->tiled = obj->info.tiled;
            ),

        TP_printk("allocation=%llx, size=%d, width=%d, height=%d, bit_cnt=%d, pitch=%d, segment_id=%d, hw_format=%d, compress_format=%d, tiled=%d",
            __entry->allocation, __entry->size, __entry->width, __entry->height, __entry->bit_cnt, __entry->pitch,
            __entry->segment_id, __entry->hw_format, __entry->compress_format, __entry->tiled)
);

TRACE_EVENT(gfx_drm_gem_release_object,
        TP_PROTO(long long card_index, struct drm_gf_gem_object *obj),
        TP_ARGS(card_index, obj),

        TP_STRUCT__entry(
            __field(unsigned long long, allocation)
            ),

        TP_fast_assign(
            __entry->allocation = card_index << 32 | obj->core_handle;
            ),

        TP_printk("allocation=%llx", __entry->allocation)
);

TRACE_EVENT(gfx_gem_prime_import,
        TP_PROTO(long long card_index, struct dma_buf *dma_buf, struct drm_gf_gem_object* obj),
        TP_ARGS(card_index, dma_buf, obj),

        TP_STRUCT__entry(
            __field(struct dma_buf *, dma_buf)
            __field(unsigned long long, allocation)
            __field(unsigned int, size)
            __field(unsigned int, width)
            __field(unsigned int, height)
            __field(unsigned int, bit_cnt)
            __field(unsigned int, pitch)
            __field(unsigned int, segment_id)
            __field(unsigned int, hw_format)
            __field(unsigned int, compress_format)
            __field(unsigned int, tiled)
            ),

        TP_fast_assign(
            __entry->dma_buf = dma_buf;
            __entry->allocation = card_index << 32 | obj->core_handle;
            __entry->size = obj->info.size;
            __entry->width = obj->info.width;
            __entry->height = obj->info.height;
            __entry->bit_cnt = obj->info.bit_cnt;
            __entry->pitch = obj->info.pitch;
            __entry->segment_id = obj->info.segment_id;
            __entry->hw_format = obj->info.hw_format;
            __entry->compress_format = obj->info.compress_format;
            __entry->tiled = obj->info.tiled;
            ),

        TP_printk("dma_buf=%p, allocation=%llx, size=%d, width=%d, height=%d, bit_cnt=%d, pitch=%d, segment_id=%d, hw_format=%d, compress_format=%d, tiled=%d",
            __entry->dma_buf, __entry->allocation, __entry->size, __entry->width, __entry->height, __entry->bit_cnt, __entry->pitch,
            __entry->segment_id, __entry->hw_format, __entry->compress_format, __entry->tiled)
);

TRACE_EVENT(gfx_gem_prime_export,
        TP_PROTO(long long card_index, struct dma_buf *dma_buf, struct drm_gf_gem_object* obj),
        TP_ARGS(card_index, dma_buf, obj),

        TP_STRUCT__entry(
            __field(struct dma_buf *, dma_buf)
            __field(unsigned long long, allocation)
            ),

        TP_fast_assign(
            __entry->dma_buf = dma_buf;
            __entry->allocation = card_index << 32 | obj->core_handle;
            ),

        TP_printk("dma_buf=%p, allocation=%llx", __entry->dma_buf, __entry->allocation)
);

TRACE_EVENT(gfx_dma_fence_wait,
        TP_PROTO(dma_fence_t *fence, bool intr, signed long timeout),
        TP_ARGS(fence, intr, timeout),

        TP_STRUCT__entry(
            __field(dma_fence_t *, fence)
            __field(bool , intr)
            __field(signed long , timeout)
            ),

        TP_fast_assign(
            __entry->fence = fence;
            __entry->intr = intr;
            __entry->timeout = timeout;
            ),

        TP_printk("fence=%p, intr=%d, timeout=%ld", __entry->fence, __entry->intr, __entry->timeout)
);

TRACE_EVENT(gfx_dma_fence_enable_signaling,
        TP_PROTO(dma_fence_t *fence),
        TP_ARGS(fence),

        TP_STRUCT__entry(
            __field(dma_fence_t *, fence)
            ),

        TP_fast_assign(
            __entry->fence = fence;
            ),

        TP_printk("fence=%p", __entry->fence)
);

TRACE_EVENT(gfx_gem_object_begin_cpu_access,
        TP_PROTO(long long card_index, struct drm_gf_gem_object *obj, long timeout, int write),
        TP_ARGS(card_index, obj, timeout, write),

        TP_STRUCT__entry(
            __field(unsigned long long, allocation)
            __field(long, timeout)
            __field(int, write)
            ),

        TP_fast_assign(
            __entry->allocation = card_index << 32 | obj->core_handle;
            __entry->timeout = timeout;
            __entry->write = write;
            ),

        TP_printk("allocation=%llx, timeout=%ld, write=%d", __entry->allocation, __entry->timeout, __entry->write)
);

TRACE_EVENT(gfx_gem_object_end_cpu_access,
        TP_PROTO(long long card_index, struct drm_gf_gem_object *obj),
        TP_ARGS(card_index, obj),

        TP_STRUCT__entry(
            __field(unsigned long long, allocation)
            ),

        TP_fast_assign(
            __entry->allocation = card_index << 32 | obj->core_handle;
            ),

        TP_printk("allocation=%llx", __entry->allocation)
);

TRACE_EVENT(gfx_atomic_flush,
    TP_PROTO(long pipe, void *e),
    TP_ARGS(pipe, e),

    TP_STRUCT__entry(
        __field(long, pipe)
        __field(void *, e)
        ),

    TP_fast_assign(
        __entry->pipe = pipe;
        __entry->e = e
        ),

    TP_printk("flush pipe=%ld,e=%p ", __entry->pipe, __entry->e)
);

TRACE_EVENT(gfx_crtc_flip,
    TP_PROTO(int crtc, const gf_crtc_flip_t *flip_arg, const struct drm_gf_gem_object *obj),
    TP_ARGS(crtc, flip_arg, obj),

    TP_STRUCT__entry(
        __field(int, crtc)
        __field(int, stream_type)
        __field(int, async_flip)
        __field(unsigned int, allocation)
        ),

    TP_fast_assign(
        __entry->crtc = crtc;
        __entry->stream_type = flip_arg->stream_type;
#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
        __entry->async_flip = flip_arg->async_flip;
#else
        __entry->async_flip = 0;
#endif
        __entry->allocation = obj ? obj->core_handle : 0;
        ),

    TP_printk("crtc=%d, stream_type=%d, allocation=%x, async_flip=%d", __entry->crtc, __entry->stream_type,
              __entry->allocation, __entry->async_flip)
);

TRACE_EVENT(gfx_begin_section,
    TP_PROTO(const char *desc),
    TP_ARGS(desc),

    TP_STRUCT__entry(
        __array(char, desc, 32)
    ),

    TP_fast_assign(
        strncpy(__entry->desc, desc, 32);
    ),

    TP_printk("begin=%s", __entry->desc)
);

TRACE_EVENT(gfx_end_section,
    TP_PROTO(int result),
    TP_ARGS(result),

    TP_STRUCT__entry(
        __field(int, result)
    ),

    TP_fast_assign(
        __entry->result = result;
    ),

    TP_printk("end=%d", __entry->result)
);

TRACE_EVENT(gfx_counter,
    TP_PROTO(const char *desc, unsigned long long value),
    TP_ARGS(desc, value),

    TP_STRUCT__entry(
        __array(char, desc, 32)
        __field(unsigned long long, value)
    ),

    TP_fast_assign(
        strncpy(__entry->desc, desc, 32);
        __entry->value = value;
    ),

    TP_printk("desc=%s, val=0x%llx", __entry->desc, __entry->value)
);

/* This part must be outside protection */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE gf_trace
#include <trace/define_trace.h>

#else

#define trace_gfx_task_create(args...) do {} while(0)
#define trace_gfx_task_submit(args...) do {} while(0)
#define trace_gfx_fence_back(args...) do {} while(0)
#define trace_gfx_vblank_intrr(args...) do {} while(0)
#define trace_gfx_vblank_onoff(args...) do {} while(0)

#define trace_gfx_drm_gem_create_object(args...) do {} while(0)
#define trace_gfx_drm_gem_release_object(args...) do {} while(0)
#define trace_gfx_gem_object_begin_cpu_access(args...) do {} while(0)
#define trace_gfx_gem_object_end_cpu_access(args...) do {} while(0)
#define trace_gfx_gem_prime_import(args...) do {} while(0)
#define trace_gfx_gem_prime_export(args...) do {} while(0)
#define trace_gfx_dma_fence_wait(args...) do {} while(0)
#define trace_gfx_dma_fence_enable_signaling(args...) do {} while(0)
#define trace_gfx_atomic_flush(args...) do {} while(0)
#define trace_gfx_crtc_flip(args...) do {} while(0)

#define trace_gfx_begin_section(args...) do {} while(0)
#define trace_gfx_end_section(args...) do {} while(0)
#define trace_gfx_counter(args...) do {} while(0)

#endif

#endif

/*_GFX_TRACE_H */



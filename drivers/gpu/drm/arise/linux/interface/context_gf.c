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

#include "context_gf.h"
#include "gf_keinterface.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>


typedef struct
{
    struct list_head link;
    int type;
    void (*proc)(struct _context_gf*, void *, va_list);
    void *arg;
} ctx_hook_t;

int gf_context_get_current_pid()
{
   return (int) getpid();
}

int gf_context_get_current_tid()
{
   return (int) syscall(__NR_gettid);
}

unsigned long long gf_context_get_current_system_time()
{
    unsigned long long ns;
    struct timespec now;

    memset(&now, 0, sizeof(struct timespec));
    clock_gettime(CLOCK_MONOTONIC, &now);
    ns = (unsigned long long)(now.tv_sec) * 1000000000 + now.tv_nsec;
    return ns;
}

int gf_context_is_perf_event_start(int fd)
{
    gf_perf_status_t status;
    memset(&status, 0, sizeof(gf_perf_status_t));
    gfGetPerfStatus(fd, &status);

    return status.started;
}

void gf_context_send_flush_enter_perf_event(int fd)
{
    gf_perf_event_t perf_event;
    gf_perf_event_cm_flush_enter_t *event = (gf_perf_event_cm_flush_enter_t *) &perf_event;
    unsigned long long ts;
    
    memset(&perf_event, 0, sizeof(gf_perf_event_t));

    event->header.size = sizeof(gf_perf_event_cm_flush_enter_t);
    event->header.type = GF_PERF_EVENT_HWC_FLUSH_ENTER;
    event->header.pid = gf_context_get_current_pid();
    event->header.tid = gf_context_get_current_tid();
    ts = gf_context_get_current_system_time();
    event->header.timestamp_low = ts & 0xffffffff;
    event->header.timestamp_high = ts >> 32;

    gfSendPerfEvent(fd, &perf_event);
}

void gf_context_send_flush_exit_perf_event(int fd)
{
    gf_perf_event_t perf_event;
    gf_perf_event_cm_flush_exit_t *event = (gf_perf_event_cm_flush_exit_t *) &perf_event;
    unsigned long long ts;
    
    memset(&perf_event, 0, sizeof(gf_perf_event_t));

    event->header.size = sizeof(gf_perf_event_cm_flush_exit_t);
    event->header.type = GF_PERF_EVENT_HWC_FLUSH_EXIT;
    event->header.pid = gf_context_get_current_pid();
    event->header.tid = gf_context_get_current_tid();
    ts = gf_context_get_current_system_time();
    event->header.timestamp_low = ts & 0xffffffff;
    event->header.timestamp_high = ts >> 32;

    gfSendPerfEvent(fd, &perf_event);
}

static void gf_context_call_hook(context_gf_t *context, int type, ...)
{
    ctx_hook_t *hook;

    list_for_each_entry(hook, &context->hook_list, link)
    {
        if (hook->type == 0 || hook->type == type)
        {
            va_list ap;
            va_start(ap, type);
            hook->proc(context, hook->arg, ap);
            va_end(ap);
        }
    }
}

static inline buf_node_t *gf_context_alloc_bufnode(context_gf_t *context)
{
    buf_node_t *node = list_first_entry_or_null(&context->free_node_list, buf_node_t, link);

    if (node)
    {
        list_del(&node->link);
    }
    else
    {
        node = malloc(sizeof(*node));
        INIT_LIST_HEAD(&node->link);
    }

    return node;
}

static inline void gf_context_release_bufnode(context_gf_t *context, buf_node_t *node)
{
    if ((char*)node >= (char*)context->cache &&
        (char*)node < (char*)context->cache + sizeof(context->cache))
    {
        list_add(&node->link, &context->free_node_list);
    }
    else
    {
        free(node);
    }
}

static inline void gf_context_reset(context_gf_t *context)
{
    while(!list_empty(&context->cmdbuf_list))
    {
        buf_node_t *node = list_first_entry(&context->cmdbuf_list, buf_node_t, link);
        list_del(&node->link);
        if (node->free)
        {
            node->free(node->arg);
        }
        gf_context_release_bufnode(context, node);
    }

    context->command_buffer_free_size       = context->command_buffer_size - sizeof(*context->header);
    context->allocation_list_free_index     = 1; //reserver one for HWCTX
    context->patch_location_list_free_index = 1;
    context->sync_object_list_free_index    = 0;
    context->split_offset                   = 0;
    context->cmdbuf_list_cnt                = 0;
    context->cmdbuf_size                    = 0;
    context->patch_cnt                = 0;

    memset(context->header, 0, sizeof(*context->header));
    gf_context_add_buffer(context, context->header, sizeof(*context->header), 0, NULL, NULL);
}

static void gf_context_flush_internal(context_gf_t *context)
{
    int i;
    int is_perf_start = gf_context_is_perf_event_start(context->fd);
    buf_node_t *node;
    gf_render_t render;
    gf_cmdbuf_t _stack_cmdbuf_array[8];
    gf_cmdbuf_t *cmdbuf = _stack_cmdbuf_array;

    assert(context->patch_cnt == 0);
    assert(!context->in_request);

    gf_context_call_hook(context, HOOK_BEFORE_FLUSH);

    if(context->cmdbuf_size > sizeof(GF_COMMAND_HEADER))
    {
        GF_COMMAND_HEADER *header = NULL;

        ++context->flush_count;
        if(is_perf_start)
        {
            gf_context_send_flush_enter_perf_event(context->fd);
        }

        if (context->cmdbuf_list_cnt > sizeof(_stack_cmdbuf_array) / sizeof(_stack_cmdbuf_array[0]))
        {
            cmdbuf = malloc(sizeof(gf_cmdbuf_t) * context->cmdbuf_list_cnt);
        }

        memset(&render, 0, sizeof(gf_render_t));
        render.context              = context->context;
        render.command_length       = context->cmdbuf_size;
        render.allocation_count     = context->allocation_list_free_index;
        render.patch_location_count = context->patch_location_list_free_index;
        render.sync_object_count    = context->sync_object_list_free_index;
        render.allocation_list      = ptr_to_ptr64(context->allocation_list);
        render.patch_location_list  = ptr_to_ptr64(context->patch_location_list);
        render.sync_object_list     = ptr_to_ptr64(context->sync_object_list);

        i = 0;
        list_for_each_entry(node, &context->cmdbuf_list, link)
        {
            if (i > 0 && (unsigned long)cmdbuf[i-1].ptr + cmdbuf[i-1].size == (unsigned long)node->ptr)
            {
                cmdbuf[i-1].size += node->size;
                continue;
            }
            else
            {
                cmdbuf[i].ptr   = ptr_to_ptr64(node->ptr);
                cmdbuf[i].size  = node->size;
                i++;
            }
        }

        render.cmdbuf_count         = i;
        render.cmdbuf_array         = ptr_to_ptr64(cmdbuf);

        header = (GF_COMMAND_HEADER*)ptr64_to_ptr(cmdbuf[0].ptr);
        render.flags.Flag2dCmd = header->Flag2dCmd;
        render.flags.Flag3dbltCmd= header->Flag3dbltCmd;
        render.flags.ContainLpdpCmd = header->ContainLpdpCmd;
        memset(ptr64_to_ptr(cmdbuf[0].ptr), 0, sizeof(GF_COMMAND_HEADER));

        gfRender(context->fd, &render);

        if (cmdbuf != _stack_cmdbuf_array)
        {
            free(cmdbuf);
        }

        if(is_perf_start)
        { 
            gf_context_send_flush_exit_perf_event(context->fd);
        }
    }

    /* TODO : add hwctx support */
    gf_context_reset(context);

    gf_context_call_hook(context, HOOK_AFTER_FLUSH);
}

void gf_context_flush(context_gf_t *context)
{
    if (context->freeze_count == 0)
    {
        gf_context_flush_internal(context);
    }
}

void gf_context_add_buffer(context_gf_t *context, void *cmdbuf, int cmd_size, 
                            int cmd_hint, void (*free_proc)(void *), void *arg)
{
    int i;
    assert(!context->in_request);

    if(cmd_hint == GF_CMD_HINT_LPDPBLT)
    {
        context->header->ContainLpdpCmd = 1;
    }
    else if(cmd_hint == GF_CMD_HINT_2D)
    {
        context->header->Flag2dCmd = 1;
    }
    else if(cmd_hint == GF_CMD_HINT_3DBLT)
    {
        context->header->Flag3dbltCmd = 1;
    }

    for (i = 0; i < context->patch_cnt; i++)
    {
        *(unsigned int*)context->patch_buffer[2*i] = ((char*)context->patch_buffer[2*i+1] - (char*)cmdbuf) + context->cmdbuf_size;
    }
    context->patch_cnt = 0;

    if (!free_proc && !list_empty(&context->cmdbuf_list))
    {
        buf_node_t *last = list_last_entry(&context->cmdbuf_list, buf_node_t, link);
        if (!last->free &&
            ((char*)last->ptr + last->size == cmdbuf))
        {
            last->size += cmd_size;
            goto done;
        }
    }

    buf_node_t *node = gf_context_alloc_bufnode(context);
    node->ptr   = cmdbuf;
    node->size  = cmd_size;
    node->free  = free_proc;
    node->arg  = arg;

    list_add_tail(&node->link, &context->cmdbuf_list);
    context->cmdbuf_list_cnt++;

done:
    context->cmdbuf_size += cmd_size;
}

unsigned int *gf_context_request_space(context_gf_t *context, int cmd_size, int cmd_hint)
{
    assert(!context->in_request);
    assert(context->patch_cnt == 0);

    if (context->cmdbuf_size + cmd_size > context->command_buffer_size)
    {
        gf_context_flush_internal(context);
    }

    if(cmd_hint == GF_CMD_HINT_LPDPBLT)
    {
        context->header->ContainLpdpCmd = 1;
    }
    else if(cmd_hint == GF_CMD_HINT_2D)
    {
        context->header->Flag2dCmd = 1;
    }
    else if(cmd_hint == GF_CMD_HINT_3DBLT)
    {
        context->header->Flag3dbltCmd = 1;
    }

    context->in_request = 1;
    return context->command_buffer + (context->command_buffer_size - context->command_buffer_free_size)/sizeof(unsigned int);
}

void gf_context_release_space(context_gf_t *context, unsigned int *last_cmd)
{
    int curr_cmd_size = (last_cmd - context->command_buffer) * sizeof(unsigned int);

    assert(context->in_request);
    assert(context->patch_cnt == 0);

    if(curr_cmd_size > context->command_buffer_size)
    {
        assert(0);
    }

    context->in_request = 0;

    gf_context_add_buffer(context, 
            context->command_buffer + (context->command_buffer_size - context->command_buffer_free_size)/sizeof(unsigned int),
            curr_cmd_size - (context->command_buffer_size - context->command_buffer_free_size), 0, NULL, NULL);

    context->command_buffer_free_size = context->command_buffer_size - curr_cmd_size;
}

void gf_context_add_allocation(
         context_gf_t *context, unsigned int allocation,
         unsigned int *ref_cmd, unsigned int allocation_offset,
         int slot_ID, int pagable, int write_op, int driver_ID)
{
    gf_allocation_list_t    *allocation_tracer = NULL;
    gf_patchlocation_list_t *patch_location    = NULL;
    int idx, bExist = 0;

    if((context->allocation_list_free_index >= context->allocation_list_size) ||
       (context->patch_location_list_free_index >= context->patch_location_list_size))
    {
        assert(0);
    }

    gf_context_call_hook(context, HOOK_ADD_ALLOCATION, allocation,
            ref_cmd, allocation_offset, slot_ID, pagable, write_op, driver_ID);

    for(idx = 0; idx < context->allocation_list_free_index; idx++)
    {
        allocation_tracer = &context->allocation_list[idx];
        if(allocation_tracer->hAllocation == allocation)
        {
            bExist = 1;
            allocation_tracer->WriteOperation |= write_op;

            break;
        }
    }

    if(!bExist)
    {
        idx = context->allocation_list_free_index++;

        allocation_tracer = &context->allocation_list[idx];

        memset(allocation_tracer, 0, sizeof(gf_allocation_list_t));

        allocation_tracer->hAllocation    = allocation;
        allocation_tracer->WriteOperation = write_op;
    }

    if(pagable)
    {
        patch_location = &context->patch_location_list[context->patch_location_list_free_index];

        memset(patch_location, 0, sizeof(gf_patchlocation_list_t));

        patch_location->AllocationIndex  = idx;
        patch_location->SlotId           = slot_ID;
        patch_location->DriverId         = driver_ID;
        patch_location->AllocationOffset = allocation_offset;
        if (context->in_request)
        {
            patch_location->PatchOffset      = (ref_cmd - context->command_buffer) * sizeof(unsigned int) -
                                                (context->command_buffer_size - context->command_buffer_free_size) + context->cmdbuf_size;
        }
        else
        {
            patch_location->PatchOffset      = 0;
            context->patch_buffer[2*context->patch_cnt]    = &patch_location->PatchOffset;
            context->patch_buffer[2*context->patch_cnt+1]  = ref_cmd;
            context->patch_cnt++;
        }
        patch_location->SplitOffset      = context->split_offset;
        context->patch_location_list_free_index++;
    }
}

void gf_context_add_sync_object(
         context_gf_t *context, unsigned int sync_object,
         unsigned int *fence_location, unsigned long long fence_value)
{
    gf_syncobj_list_t *sync_tracer = NULL;

    if(context->sync_object_list_free_index >= context->sync_object_list_size)
    {
        assert(0);
    }

    sync_tracer = &context->sync_object_list[context->sync_object_list_free_index++];

    sync_tracer->hSyncObject = sync_object;
    if (context->in_request)
    {
        sync_tracer->PatchOffset      = (fence_location - context->command_buffer) * sizeof(unsigned int) -
                                            (context->command_buffer_size - context->command_buffer_free_size) + context->cmdbuf_size;
    }
    else
    {
        sync_tracer->PatchOffset      = 0;
        context->patch_buffer[2*context->patch_cnt]    = &sync_tracer->PatchOffset;
        context->patch_buffer[2*context->patch_cnt+1]  = fence_location;
        context->patch_cnt++;
    }
    sync_tracer->FenceValue  = fence_value;
}


context_gf_t *gf_create_context(int fd, unsigned int device, int engine_index)
{
    context_gf_t        *context = calloc(1, sizeof(context_gf_t));
    gf_create_context_t create;
    int i;
    int status;

    if(context == NULL) return NULL;

    memset(&create, 0, sizeof(create));

    create.device       = device;
    create.engine_index = engine_index;
    create.flags              = 0; //LeonHe: fix-me, need determine which flags?

    status = gfCreateContext(fd, &create);

    if(status)
    {
        free(context);

        return NULL;
    }

    context->fd                       = fd;
    context->device                   = device;
    context->engine_index             = engine_index;
    context->context                  = create.context;

    context->command_buffer_size      = 64 * 1024; // bytes
    context->command_buffer           = calloc(1, context->command_buffer_size);

    context->allocation_list_size     = 1024 * 4;   // 1024 * 4 * sizeof(gf_allocation_list_t) bytes
    context->allocation_list          = calloc(1, context->allocation_list_size * sizeof(gf_allocation_list_t));

    context->patch_location_list_size = 1024 * 16;  // 1024 * 16 * sizeof(gf_patchlocation_list_t) bytes
    context->patch_location_list      = calloc(1, context->patch_location_list_size * sizeof(gf_patchlocation_list_t));

    context->sync_object_list_size    = 256;        // 256 * sizeof(gf_syncobj_list_t) bytes
    context->sync_object_list         = calloc(1, context->sync_object_list_size * sizeof(gf_syncobj_list_t));

    context->patch_buffer             = malloc(2 * context->patch_location_list_size * sizeof(void*));
    context->freeze_count             = 0;

    INIT_LIST_HEAD(&context->hook_list);
    INIT_LIST_HEAD(&context->cmdbuf_list);
    INIT_LIST_HEAD(&context->free_node_list);
    for (i = 0; i < sizeof(context->cache) / sizeof(context->cache[0]); i++)
    {
        list_add(&context->cache[i].link, &context->free_node_list);
    }

    context->header                   = (GF_COMMAND_HEADER *)context->command_buffer;
    gf_context_reset(context);
    return context;
}

void gf_destroy_context(context_gf_t *context)
{
    gf_destroy_context_t destroy = {0, };

    gf_context_reset(context);

    destroy.device = context->device;
    destroy.context = context->context;

    gfDestroyContext(context->fd, &destroy);

    if (context->command_buffer)
    {
        free(context->command_buffer);
    }

    if (context->allocation_list)
    {
        free(context->allocation_list);
    }

    if (context->patch_location_list)
    {
        free(context->patch_location_list);
    }

    if (context->sync_object_list)
    {
        free(context->sync_object_list);
    }

    if (context->patch_buffer)
    {
        free(context->patch_buffer);
    }


    free(context);
}


void gf_context_add_hook(context_gf_t *context, int type, void (*proc)(context_gf_t*, void*, va_list), void *arg)
{
    ctx_hook_t *hook = calloc(1, sizeof(*hook));
    INIT_LIST_HEAD(&hook->link);

    hook->type = type;
    hook->proc = proc;
    hook->arg = arg;
    list_add_tail(&hook->link, &context->hook_list);
}

int gf_context_remove_hook(context_gf_t *context, int type, void *proc)
{
    int count = 0;
    ctx_hook_t *hook, *next;

    list_for_each_entry_safe(hook, next, &context->hook_list, link)
    {
        if ((type == 0 || hook->type == type) && hook->proc == proc)
        {
            list_del(&hook->link);
            free(hook);
            ++count;
        }
    }

    return count;
}

int gf_context_freeze(context_gf_t *context)
{
    return ++context->freeze_count;
}

int gf_context_unfreeze(context_gf_t *context)
{
    return --context->freeze_count;
}

static struct context_interface context_interface_struct_v2 = {
    .create_context = gf_create_context,
    .destroy_context = gf_destroy_context,
    .context_request_space = gf_context_request_space,
    .context_release_space = gf_context_release_space,
    .context_flush = gf_context_flush,
    .context_add_buffer = gf_context_add_buffer,
    .context_add_allocation = gf_context_add_allocation,
    .context_add_sync_object = gf_context_add_sync_object,
    .context_add_hook = gf_context_add_hook,
    .context_remove_hook = gf_context_remove_hook,
    .context_freeze = gf_context_freeze,
    .context_unfreeze = gf_context_unfreeze,
};

__attribute__((visibility("default")))
struct context_interface *context_interface_v2 = &context_interface_struct_v2;

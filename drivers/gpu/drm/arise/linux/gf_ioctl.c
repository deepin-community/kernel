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
#include "gf_ioctl.h"
#include "os_interface.h"
#include "kernel_interface.h"
#include "gf_debugfs.h"
#include "gf_gem.h"
#include "gf_gem_priv.h"
#include "gf_disp.h"

int gf_ioctl_wait_chip_idle(struct drm_device *dev, void *data,struct drm_file *filp)
{
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;

    gf_core_interface->wait_chip_idle(gf->adapter);

    return 0;
}


int gf_ioctl_wait_allocation_idle(struct drm_device *dev, void *data,struct drm_file *filp)
{
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_wait_allocation_idle_t *wait_allocation = (gf_wait_allocation_idle_t *)data;

    wait_allocation->hAllocation = gf_gem_get_core_handle(priv, wait_allocation->hAllocation);
    gf_core_interface->wait_allocation_idle(gf->adapter, wait_allocation);

    return 0;
}

int gf_ioctl_query_info(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = 0;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_query_info_t *info = (gf_query_info_t *)data;
    unsigned int saved = 0;

    switch(info->type)
    {
    case GF_QUERY_ALLOCATION_INFO:
    case GF_SET_PERF_SWAP_HINT:
    case GF_GET_PERF_SWAP_HINT:
        saved = info->argu;
        info->argu = gf_gem_get_core_handle(priv, info->argu);
        break;
    case GF_QUERY_ENGINE_CLOCK:
        {
            if (DISP_OK == disp_cbios_get_clock(gf->disp_info, GF_QUERY_CORE_CLOCK, &info->value))
            {
                info->value /= 1000;//cbios clock is KHZ, change to MHZ
                goto done;
            }
            else
            {
                return -1;
            }
        }
    case GF_QUERY_DIAGS:
        {
            disp_cbios_get_clock(gf->disp_info, GF_QUERY_MCLK, &info->diags.memory_clk);
            disp_cbios_get_clock(gf->disp_info, GF_QUERY_ENGINE_CLOCK, &info->diags.engine_clk);
            disp_cbios_get_clock(gf->disp_info, GF_QUERY_VCLK, &info->diags.video_clk);

            info->diags.memory_clk = gf_calc_double_standard_mclk(info->diags.memory_clk) / 2;
        }
    }
    ret = gf_core_interface->query_info(gf->adapter, info);
    if (saved)
    {
        info->argu = saved;
    }
done:
    return ret;
}

int gf_ioctl_create_device(struct drm_device *dev, void *data,struct drm_file *filp)
{
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_create_device_t *create_device = (gf_create_device_t *)data;

    gf_assert(priv->gpu_device != 0, GF_FUNC_NAME(__func__));
    create_device->device = priv->gpu_device;
    gf_core_interface->update_device_name(gf->adapter, priv->gpu_device);
    if (priv->debug) {
        priv->debug->user_pid = gf_get_current_pid();
    }

    return 0;
}

int gf_ioctl_destroy_device(struct drm_device *dev, void *data,struct drm_file *filp)
{
    gf_file_t *priv = filp->driver_priv;
    unsigned int *device = (unsigned int *)data;

    gf_assert(*device == priv->gpu_device, GF_FUNC_NAME(__func__));

    return 0;
}

int gf_ioctl_send_perf_event(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = 0;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_perf_event_t perf_event = {0};
    gf_perf_event_header_t *perf_event_header = (gf_perf_event_header_t *)data;

    if (perf_event_header->size > sizeof(gf_perf_event_t))
    {
        return -1;
    }

    if (perf_event_header->size >= sizeof(gf_perf_event_header_t))
    {
        gf_memcpy((void *)&perf_event, data, perf_event_header->size);
        ret = gf_core_interface->send_perf_event(gf->adapter, &perf_event);
    }
    else
    {
        return -1;
    }


    return ret;
}


int gf_ioctl_get_perf_status(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = 0;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_perf_status_t *perf_status = (gf_perf_status_t *)data;

    ret = gf_core_interface->get_perf_status(gf->adapter, perf_status);
    return ret;
}

int gf_ioctl_create_di_context(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = 0;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_create_di_context_t *create_context = (gf_create_di_context_t *)data;

    ret = gf_core_interface->create_di_context(gf->adapter, create_context);
    return ret;
}

int gf_ioctl_destroy_di_context(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = 0;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_destroy_di_context_t *destroy_context = (gf_destroy_di_context_t *)data;

    ret = gf_core_interface->destroy_di_context(gf->adapter, destroy_context);
    return ret;
}

int gf_ioctl_render(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = 0;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_cmdbuf_t stack_cmdbufs[8];
    gf_cmdbuf_t __user *cmdbuf;
    gf_render_t *render = (gf_render_t *)data;

    cmdbuf = (gf_cmdbuf_t __user*)(unsigned long)render->cmdbuf_array;
    if (render->cmdbuf_count > sizeof(stack_cmdbufs)/sizeof(stack_cmdbufs[0]))
    {
        render->cmdbuf_array = (gf_ptr64_t)(unsigned long)gf_malloc(render->cmdbuf_count * sizeof(gf_cmdbuf_t));
    }
    else
    {
        render->cmdbuf_array = (gf_ptr64_t)(unsigned long)stack_cmdbufs;
    }
    gf_copy_from_user((void*)(unsigned long)render->cmdbuf_array, cmdbuf, sizeof(gf_cmdbuf_t) * render->cmdbuf_count);
    ret = gf_core_interface->render(gf->adapter, render);
    if ((unsigned long)render->cmdbuf_array != (unsigned long)stack_cmdbufs)
    {
        gf_free((void*)(unsigned long)render->cmdbuf_array);
    }

    return ret;
}

int gf_ioctl_create_fence_sync_object(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = 0;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;

    gf_create_fence_sync_object_t *create = (gf_create_fence_sync_object_t *)data;

    ret = gf_core_interface->create_fence_sync_object(gf->adapter, create, FALSE);
    return ret;
}

int gf_ioctl_destroy_fence_sync_object(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = 0;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;


    gf_destroy_fence_sync_object_t *destroy = (gf_destroy_fence_sync_object_t *)data;

    ret = gf_core_interface->destroy_fence_sync_object(gf->adapter, destroy);
    return ret;
}

int gf_ioctl_wait_fence_sync_object(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = 0;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;


    gf_wait_fence_sync_object_t *wait = (gf_wait_fence_sync_object_t *)data;

    ret = gf_core_interface->wait_fence_sync_object(gf->adapter, wait);

    return ret;
}

int gf_ioctl_fence_value(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = 0;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;


    gf_fence_value_t *value = (gf_fence_value_t *)data;

    ret = gf_core_interface->fence_value(gf->adapter, value);
    return ret;
}


int gf_ioctl_get_allocation_state(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = -1;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_get_allocation_state_t *state = (gf_get_allocation_state_t *)data;
    unsigned int gem = 0;

    gem = state->hAllocation;
    state->hAllocation = gf_gem_get_core_handle(priv, gem);
    ret = gf_core_interface->get_allocation_state(gf->adapter, state);

    if (ret == 0)
    {
        state->hAllocation = gem;
    }

    return ret;
}

int gf_ioctl_gem_create_allocation(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = -1;
    gf_file_t *priv = filp->driver_priv;
    gf_create_allocation_t *create = (gf_create_allocation_t *)data;
    unsigned int gem = 0;

    if (create->reference)
    {
        gem = create->reference;
        create->reference = gf_gem_get_core_handle(priv, gem);
    }

    ret = gf_drm_gem_create_object_ioctl(priv->parent_file, create);

    if (ret == 0)
    {
        create->reference = gem;
    }

    return ret;
}


int gf_ioctl_create_context(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = -1;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_create_context_t *create = (gf_create_context_t *)data;

    ret = gf_core_interface->create_context(gf->adapter, create, GF_MEM_USER);
    return ret;
}

int gf_ioctl_destroy_context(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = -1;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_destroy_context_t *destroy = (gf_destroy_context_t *)data;

    ret = gf_core_interface->destroy_context(gf->adapter, destroy);
    return ret;
}

int gf_ioctl_gem_map_gtt(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = -1;
    gf_file_t *priv = filp->driver_priv;
    gf_drm_gem_map_t *map = (gf_drm_gem_map_t *)data;

    ret = gf_gem_mmap_gtt_ioctl(priv->parent_file, map);
    return ret;
}


int gf_ioctl_gem_create_resource(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = -1;
    gf_file_t *priv = filp->driver_priv;
    gf_create_resource_t *create = (gf_create_resource_t *)data;

    ret = gf_drm_gem_create_resource_ioctl(priv->parent_file, create);
    return ret;
}

int gf_ioctl_add_hw_ctx_buf(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = -1;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_add_hw_ctx_buf_t *add = (gf_add_hw_ctx_buf_t *)data;

    ret = gf_core_interface->add_hw_ctx_buf(gf->adapter, add);
    return ret;
}

int gf_ioctl_rm_hw_ctx_buf(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = -1;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_rm_hw_ctx_buf_t *rm = (gf_rm_hw_ctx_buf_t *)data;

    ret = gf_core_interface->rm_hw_ctx_buf(gf->adapter, rm);
    return ret;
}

int gf_ioctl_gem_begin_cpu_access(struct drm_device *dev, void *data,struct drm_file *filp)
{
    gf_file_t *priv = filp->driver_priv;
    gf_drm_gem_begin_cpu_access_t *begin = (gf_drm_gem_begin_cpu_access_t *)data;

    return gf_gem_object_begin_cpu_access_ioctl(priv->parent_file, begin);
}

int gf_ioctl_cil2_misc(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = -1;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_cil2_misc_t *misc = (gf_cil2_misc_t *)data;

    ret = gf_core_interface->cil2_misc(gf->adapter, misc);
    return ret;
}

int gf_ioctl_gem_end_cpu_access(struct drm_device *dev, void *data,struct drm_file *filp)
{
    gf_file_t *priv = filp->driver_priv;
    gf_drm_gem_end_cpu_access_t *end = (gf_drm_gem_end_cpu_access_t *)data;

    gf_gem_object_end_cpu_access_ioctl(priv->parent_file, end);

    return 0;
}

int gf_ioctl_begin_perf_event(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = 0;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_begin_perf_event_t *begin_perf_event = (gf_begin_perf_event_t *)data;

    ret = gf_core_interface->begin_perf_event(gf->adapter, begin_perf_event);
    return ret;
}

int gf_ioctl_kms_get_pipe_from_crtc(struct drm_device *dev, void *data,struct drm_file *filp)
{
    gf_file_t *priv = filp->driver_priv;
    gf_kms_get_pipe_from_crtc_t *get = (gf_kms_get_pipe_from_crtc_t *)data;

    if (disp_get_pipe_from_crtc(priv, get))
    {
        return -1;
    }

    return 0;
}

int gf_ioctl_begin_miu_dump_perf_event(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = 0;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_begin_miu_dump_perf_event_t *begin_miu_dump = (gf_begin_miu_dump_perf_event_t *)data;

    ret = gf_core_interface->begin_miu_dump_perf_event(gf->adapter, begin_miu_dump);
    return ret;
}

int gf_ioctl_end_perf_event(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = 0;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_end_perf_event_t *end_perf_event = (gf_end_perf_event_t *)data;

    ret = gf_core_interface->end_perf_event(gf->adapter, end_perf_event);
    return ret;
}

int gf_ioctl_end_miu_dump_perf_event(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = 0;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_end_miu_dump_perf_event_t *end_miu_dump = (gf_end_miu_dump_perf_event_t *)data;

    ret = gf_core_interface->end_miu_dump_perf_event(gf->adapter, end_miu_dump);
    return ret;
}


int gf_ioctl_set_miu_reg_list_perf_event(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = 0;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_miu_reg_list_perf_event_t *miu_reg_list = (gf_miu_reg_list_perf_event_t *)data;

    ret = gf_core_interface->set_miu_reg_list_perf_event(gf->adapter, miu_reg_list);
    return ret;
}

int gf_ioctl_get_miu_dump_perf_event(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = 0;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_get_miu_dump_perf_event_t *get_miu_dump = (gf_get_miu_dump_perf_event_t *)data;

    ret = gf_core_interface->get_miu_dump_perf_event(gf->adapter, get_miu_dump);
    return ret;
}

int gf_ioctl_direct_get_miu_dump_perf_event(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = 0;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_direct_get_miu_dump_perf_event_t *direct_get_dump = (gf_direct_get_miu_dump_perf_event_t *)data;

    ret = gf_core_interface->direct_get_miu_dump_perf_event(gf->adapter, direct_get_dump);
    return ret;
}

int gf_ioctl_get_perf_event(struct drm_device *dev, void *data,struct drm_file *filp)
{
    int ret = 0;
    gf_file_t *priv = filp->driver_priv;
    gf_card_t *gf = priv->card;
    gf_get_perf_event_t *get_perf_event = (gf_get_perf_event_t *)data;

    ret = gf_core_interface->get_perf_event(gf->adapter, get_perf_event);
    return ret;
}

struct drm_ioctl_desc gf_ioctls[] =
{
#define GF_IOCTL_DEF_DRV(ioctl, _func, _flags)     \
    [GF_IOCTL_NR(ioctl) - DRM_COMMAND_BASE] = {    \
        .cmd = ioctl,                              \
        .func = _func,                             \
        .flags = _flags,                           \
        .name = #ioctl                             \
    }

    GF_IOCTL_DEF_DRV(GF_IOCTL_WAIT_CHIP_IDLE,gf_ioctl_wait_chip_idle,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_WAIT_ALLOCATION_IDLE,gf_ioctl_wait_allocation_idle,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_QUERY_INFO,gf_ioctl_query_info,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_CREATE_DEVICE,gf_ioctl_create_device,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_DESTROY_DEVICE,gf_ioctl_destroy_device,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_BEGIN_PERF_EVENT,gf_ioctl_begin_perf_event,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_END_PERF_EVENT,gf_ioctl_end_perf_event,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_GET_PERF_EVENT,gf_ioctl_get_perf_event,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_SEND_PERF_EVENT,gf_ioctl_send_perf_event,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_GET_PERF_STATUS,gf_ioctl_get_perf_status,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_BEGIN_MIU_DUMP_PERF_EVENT,gf_ioctl_begin_miu_dump_perf_event,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_END_MIU_DUMP_PERF_EVENT,gf_ioctl_end_miu_dump_perf_event,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_SET_MIU_REG_LIST_PERF_EVENT,gf_ioctl_set_miu_reg_list_perf_event,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_GET_MIU_DUMP_PERF_EVENT,gf_ioctl_get_miu_dump_perf_event,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_DIRECT_GET_MIU_DUMP_PERF_EVENT,gf_ioctl_direct_get_miu_dump_perf_event,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_CREATE_CONTEXT,gf_ioctl_create_context,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_DESTROY_CONTEXT,gf_ioctl_destroy_context,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_RENDER,gf_ioctl_render,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_CREATE_FENCE_SYNC_OBJECT,gf_ioctl_create_fence_sync_object,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_DESTROY_FENCE_SYNC_OBJECT,gf_ioctl_destroy_fence_sync_object,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_WAIT_FENCE_SYNC_OBJECT,gf_ioctl_wait_fence_sync_object,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_FENCE_VALUE,gf_ioctl_fence_value,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_CREATE_DI_CONTEXT,gf_ioctl_create_di_context,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_DESTROY_DI_CONTEXT,gf_ioctl_destroy_di_context,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_GET_ALLOCATION_STATE,gf_ioctl_get_allocation_state,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_ADD_HW_CTX_BUF,gf_ioctl_add_hw_ctx_buf,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_RM_HW_CTX_BUF,gf_ioctl_rm_hw_ctx_buf,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_CIL2_MISC,gf_ioctl_cil2_misc,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_DRM_GEM_CREATE_ALLOCATION,gf_ioctl_gem_create_allocation,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_DRM_GEM_CREATE_RESOURCE,gf_ioctl_gem_create_resource,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_DRM_GEM_MAP_GTT,gf_ioctl_gem_map_gtt,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_DRM_BEGIN_CPU_ACCESS,gf_ioctl_gem_begin_cpu_access,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_DRM_END_CPU_ACCESS,gf_ioctl_gem_end_cpu_access,DRM_AUTH|DRM_RENDER_ALLOW),
    GF_IOCTL_DEF_DRV(GF_IOCTL_KMS_GET_PIPE_FROM_CRTC,gf_ioctl_kms_get_pipe_from_crtc,DRM_AUTH|DRM_RENDER_ALLOW),
};

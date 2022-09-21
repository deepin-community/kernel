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

#include "gf_def.h"
#include "gf_ioctl.h"
#include "os_interface.h"
#include "kernel_interface.h"
#include "gf_debugfs.h"
#include "gf_gem.h"
#include "gf_gem_priv.h"
#include "gf_disp.h"

int gf_ioctl_wait_chip_idle(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_card_t  *gf = priv->card;

    gf_core_interface->wait_chip_idle(gf->adapter);

    return 0;
}


int gf_ioctl_wait_allocation_idle(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_card_t                 *gf  = priv->card;
    void __user                *argp = (void __user*)arg;

    gf_wait_allocation_idle_t  wait_allocation;

    if(gf_copy_from_user(&wait_allocation, argp, sizeof(gf_wait_allocation_idle_t)))
    {
        return -1;
    }

    wait_allocation.hAllocation = gf_gem_get_core_handle(priv, wait_allocation.hAllocation);
    gf_core_interface->wait_allocation_idle(gf->adapter, &wait_allocation);

    return 0;
}

int gf_ioctl_query_info(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_card_t  *gf  = priv->card;
    void __user *argp = (void __user*)arg;
    gf_query_info_t info;
    int ret = 0;
    unsigned int saved = 0;
    
    if(gf_copy_from_user(&info, argp, sizeof(gf_query_info_t)))
    {
        return -1;
    }

    switch(info.type)
    {
    case GF_QUERY_ALLOCATION_INFO:
        saved = info.argu;
        info.argu = gf_gem_get_core_handle(priv, info.argu);
        break;
    case GF_QUERY_ENGINE_CLOCK:
        {
            if (0 == disp_cbios_get_clock(gf->disp_info, GF_QUERY_ENGINE_CLOCK, &info.value))
            {
                info.value /= 1000;//cbios clock is KHZ, change to MHZ
                goto done;
            }
            else
            {
                return -1;
            }
        }
    case GF_QUERY_DIAGS:
        {
            disp_cbios_get_clock(gf->disp_info, GF_QUERY_MCLK, &info.diags.memory_clk);
            disp_cbios_get_clock(gf->disp_info, GF_QUERY_ENGINE_CLOCK, &info.diags.engine_clk);
            disp_cbios_get_clock(gf->disp_info, GF_QUERY_VCLK, &info.diags.video_clk);
        }
    }
    ret = gf_core_interface->query_info(gf->adapter, &info);
    if (saved)
    {
        info.argu = saved;
    }
done:
    if(gf_copy_to_user(argp, &info, sizeof(gf_query_info_t)))
    {
        return -1;
    }

    return ret;
}

int gf_ioctl_create_device(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_card_t  *gf  = priv->card;
    void __user           *argp = (void __user*)arg;
    gf_create_device_t    create_device = {0};

    gf_assert(priv->gpu_device != 0, GF_FUNC_NAME(__func__));
    create_device.device = priv->gpu_device;
    gf_core_interface->update_device_name(gf->adapter, priv->gpu_device);
    if (priv->debug) {
        priv->debug->user_pid = gf_get_current_pid();
    }

    return gf_copy_to_user(argp, &create_device, sizeof(gf_create_device_t));
}

int gf_ioctl_destroy_device(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    void __user       *argp = (void __user*)arg;
    unsigned int       device = 0;

    if(gf_copy_from_user(&device, argp, sizeof(unsigned int)))
    {
        return -1;
    }

    gf_assert(device == priv->gpu_device, GF_FUNC_NAME(__func__));

    return 0;
}

int gf_ioctl_send_perf_event(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_card_t   *gf = priv->card;
    void __user  *argp = (void __user*)arg;
    gf_perf_event_t perf_event;
    int          ret = 0;

    if (gf_copy_from_user(&perf_event, argp, sizeof(gf_perf_event_header_t)))
    {
        return -1;
    }

    if (perf_event.header.size > sizeof(gf_perf_event_header_t))
    {
        if (gf_copy_from_user(&perf_event, argp, perf_event.header.size))
        {
            return -1;
        }
    }

    ret = gf_core_interface->send_perf_event(gf->adapter, &perf_event);

    return ret;
}


int gf_ioctl_get_perf_status(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_card_t   *gf = priv->card;
    void __user  *argp = (void __user*)arg;
    gf_perf_status_t perf_status = {0};
    int ret = 0;

    ret = gf_core_interface->get_perf_status(gf->adapter, &perf_status);

    if (ret == 0)
    {
        gf_copy_to_user(argp, &perf_status, sizeof(gf_perf_status_t));
    }

    return ret;
}

int gf_ioctl_create_di_context(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_card_t          *gf  = priv->card;
    void __user         *argp = (void __user*)arg;

    gf_create_di_context_t create_context;
    int                  ret  = 0;

    if(gf_copy_from_user(&create_context, argp, sizeof(gf_create_di_context_t)))
    {
        return -1;
    }

    ret = gf_core_interface->create_di_context(gf->adapter, &create_context);

    if(gf_copy_to_user(argp, &create_context, sizeof(gf_create_di_context_t)))
    {
        return -1;
    }

    return ret;
}

int gf_ioctl_destroy_di_context(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_card_t          *gf  = priv->card;
    void __user         *argp = (void __user*)arg;

    gf_destroy_di_context_t destroy_context;
    int                      ret  = 0;

    if(gf_copy_from_user(&destroy_context, argp, sizeof(gf_destroy_di_context_t)))
    {
        return -1;
    }

    ret = gf_core_interface->destroy_di_context(gf->adapter, &destroy_context);

    if(gf_copy_to_user(argp, &destroy_context, sizeof(gf_destroy_di_context_t)))
    {
        return -1;
    }

    return ret;
}

int gf_ioctl_render(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_card_t   *gf    = priv->card;
    void __user  *argp   = (void __user*)arg;
    gf_cmdbuf_t  stack_cmdbufs[8];
    gf_cmdbuf_t __user *cmdbuf;
    gf_render_t  render = {0};
    int           ret    = 0;

    if(gf_copy_from_user(&render, argp, sizeof(gf_render_t)))
    {
        return -1;
    }

    cmdbuf = (gf_cmdbuf_t __user*)(unsigned long)render.cmdbuf_array;
    if (render.cmdbuf_count > sizeof(stack_cmdbufs)/sizeof(stack_cmdbufs[0]))
    {
        render.cmdbuf_array = (gf_ptr64_t)(unsigned long)gf_malloc(render.cmdbuf_count * sizeof(gf_cmdbuf_t));
    }
    else
    {
        render.cmdbuf_array = (gf_ptr64_t)(unsigned long)stack_cmdbufs;
    }
    gf_copy_from_user((void*)(unsigned long)render.cmdbuf_array, cmdbuf, sizeof(gf_cmdbuf_t) * render.cmdbuf_count);
    ret = gf_core_interface->render(gf->adapter, &render);
    if ((unsigned long)render.cmdbuf_array != (unsigned long)stack_cmdbufs)
    {
        gf_free((void*)(unsigned long)render.cmdbuf_array);
    }

    return ret;
}

int gf_ioctl_create_fence_sync_object(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_card_t   *gf    = priv->card;
    void __user  *argp   = (void __user*)arg;
    int          result  = 0;

    gf_create_fence_sync_object_t  create = {0};

    if(gf_copy_from_user(&create, argp, sizeof(gf_create_fence_sync_object_t)))
    {
        return -1;
    }

    result = gf_core_interface->create_fence_sync_object(gf->adapter, &create, FALSE);

    if(gf_copy_to_user(argp, &create, sizeof(gf_create_fence_sync_object_t)))
    {
        return -1;
    }

    return result;
}

int gf_ioctl_destroy_fence_sync_object(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_card_t   *gf    = priv->card;
    void __user  *argp   = (void __user*)arg;
    int          result  = 0;

    gf_destroy_fence_sync_object_t  destroy = {0};

    if(gf_copy_from_user(&destroy, argp, sizeof(gf_destroy_fence_sync_object_t)))
    {
        return -1;
    }

    result = gf_core_interface->destroy_fence_sync_object(gf->adapter, &destroy);

    return result;
}

int gf_ioctl_wait_fence_sync_object(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_card_t   *gf    = priv->card;
    void __user  *argp   = (void __user*)arg;
    int          result  = 0;

    gf_wait_fence_sync_object_t  wait = {0};

    if(gf_copy_from_user(&wait, argp, sizeof(gf_wait_fence_sync_object_t)))
    {
        return -1;
    }

    result = gf_core_interface->wait_fence_sync_object(gf->adapter, &wait);

    if(gf_copy_to_user(argp, &wait, sizeof(gf_wait_fence_sync_object_t)))
    {
        return -1;
    }

    return result;
}

int gf_ioctl_fence_value(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_card_t   *gf    = priv->card;
    void __user  *argp   = (void __user*)arg;
    int          result  = 0;

    gf_fence_value_t  value = {0};

    if(gf_copy_from_user(&value, argp, sizeof(gf_fence_value_t)))
    {
        return -1;
    }

    result = gf_core_interface->fence_value(gf->adapter, &value);

    if(gf_copy_to_user(argp, &value, sizeof(gf_fence_value_t)))
    {
        return -1;
    }

    return result;
}


int gf_ioctl_get_allocation_state(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    int               ret = -1;
    gf_card_t        *gf = priv->card;
    void __user       *argp = (void __user*)arg;
    gf_get_allocation_state_t state = {0, };
    unsigned int      gem = 0;

    if (gf_copy_from_user(&state, argp, sizeof(state)))
    {
        return -1;
    }

    gem = state.hAllocation;
    state.hAllocation = gf_gem_get_core_handle(priv, gem);
    ret = gf_core_interface->get_allocation_state(gf->adapter, &state);

    if (ret == 0)
    {
        state.hAllocation = gem;
        ret = gf_copy_to_user(argp, &state, sizeof(state));
    }

    return ret;
}

int gf_ioctl_gem_create_allocation(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    int               ret = -1;
    void __user       *argp = (void __user*)arg;
    gf_create_allocation_t create = {0, };
    unsigned int      gem = 0;

    if (gf_copy_from_user(&create, argp, sizeof(create)))
    {
        return -1;
    }

    if (create.reference)
    {
        gem = create.reference;
        create.reference = gf_gem_get_core_handle(priv, gem);
    }

    ret = gf_drm_gem_create_object_ioctl(priv->parent_file, &create);

    if (ret == 0)
    {
        create.reference = gem;
        ret = gf_copy_to_user(argp, &create, sizeof(create));
    }

    return ret;
}


int gf_ioctl_create_context(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    int               ret = -1;
    void __user       *argp = (void __user*)arg;
    gf_card_t        *gf = priv->card;
    gf_create_context_t create = {0, };

    if (gf_copy_from_user(&create, argp, sizeof(create)))
    {
        return -1;
    }

    ret = gf_core_interface->create_context(gf->adapter, &create, GF_MEM_USER);

    if (ret == 0)
    {
        ret = gf_copy_to_user(argp, &create, sizeof(create));
    }

    return ret;
}

int gf_ioctl_destroy_context(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    int               ret = -1;
    void __user       *argp = (void __user*)arg;
    gf_card_t        *gf = priv->card;
    gf_destroy_context_t destroy = {0, };

    if (gf_copy_from_user(&destroy, argp, sizeof(destroy)))
    {
        return -1;
    }

    ret = gf_core_interface->destroy_context(gf->adapter, &destroy);

    if (ret == 0)
    {
        ret = gf_copy_to_user(argp, &destroy, sizeof(destroy));
    }

    return ret;
}

int gf_ioctl_gem_map_gtt(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    int               ret = -1;
    void __user       *argp = (void __user*)arg;
    gf_drm_gem_map_t map = {0, };

    if (gf_copy_from_user(&map, argp, sizeof(map)))
    {
        return -1;
    }

    ret = gf_gem_mmap_gtt_ioctl(priv->parent_file, &map);

    if (ret == 0)
    {
        ret = gf_copy_to_user(argp, &map, sizeof(map));
    }

    return ret;
}


int gf_ioctl_gem_create_resource(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    int               ret = -1;
    void __user       *argp = (void __user*)arg;
    gf_create_resource_t create = {0, };

    if (gf_copy_from_user(&create, argp, sizeof(create)))
    {
        return -1;
    }

    ret = gf_drm_gem_create_resource_ioctl(priv->parent_file, &create);

    if (ret == 0)
    {
        ret = gf_copy_to_user(argp, &create, sizeof(create));
    }

    return ret;
}

int gf_ioctl_add_hw_ctx_buf(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    int               ret = -1;
    gf_card_t        *gf = priv->card;
    void __user       *argp = (void __user*)arg;
    gf_add_hw_ctx_buf_t add = {0, };

    if (gf_copy_from_user(&add, argp, sizeof(add)))
    {
        return -1;
    }

    ret = gf_core_interface->add_hw_ctx_buf(gf->adapter, &add);

    if (ret == 0)
    {
        ret = gf_copy_to_user(argp, &add, sizeof(add));
    }

    return ret;
}

int gf_ioctl_rm_hw_ctx_buf(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    int               ret = -1;
    gf_card_t        *gf = priv->card;
    void __user       *argp = (void __user*)arg;
    gf_rm_hw_ctx_buf_t rm = {0, };

    if (gf_copy_from_user(&rm, argp, sizeof(rm)))
    {
        return -1;
    }

    ret = gf_core_interface->rm_hw_ctx_buf(gf->adapter, &rm);

    if (ret == 0)
    {
        ret = gf_copy_to_user(argp, &rm, sizeof(rm));
    }

    return ret;
}

int gf_ioctl_gem_begin_cpu_access(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_drm_gem_begin_cpu_access_t begin;
    void __user       *argp = (void __user*)arg;

    if (gf_copy_from_user(&begin, argp, sizeof(begin)))
    {
        return -1;
    }

    return gf_gem_object_begin_cpu_access_ioctl(priv->parent_file, &begin);
}

int gf_ioctl_cil2_misc(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    int               ret = -1;
    gf_card_t        *gf = priv->card;
    void __user       *argp = (void __user*)arg;
    gf_cil2_misc_t  misc = {0, };
    
    if (gf_copy_from_user(&misc, argp, sizeof(misc)))
    {
        return -1;
    }

    ret = gf_core_interface->cil2_misc(gf->adapter, &misc);

    if (ret == 0)
    {
        ret = gf_copy_to_user(argp, &misc, sizeof(misc));
    }

    return ret;
}

int gf_ioctl_gem_end_cpu_access(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_drm_gem_end_cpu_access_t end;
    void __user       *argp = (void __user*)arg;

    if (gf_copy_from_user(&end, argp, sizeof(end)))
    {
        return -1;
    }

    gf_gem_object_end_cpu_access_ioctl(priv->parent_file, &end);

    return 0;
}

int gf_ioctl_begin_perf_event(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_card_t   *gf = priv->card;
    void __user  *argp = (void __user*)arg;
    gf_begin_perf_event_t begin_perf_event = {0};
    int          ret = 0;

    if (gf_copy_from_user(&begin_perf_event, argp, sizeof(gf_begin_perf_event_t)))
    {
        return -1;
    }
    ret = gf_core_interface->begin_perf_event(gf->adapter, &begin_perf_event);

    return ret;
}

int gf_ioctl_kms_get_pipe_from_crtc(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_kms_get_pipe_from_crtc_t get;
    void __user         *argp = (void __user*)arg; 

    if (gf_copy_from_user(&get, argp, sizeof(get)))
    {
        return -1;
    }

    if (disp_get_pipe_from_crtc(priv, &get))
    {
        return -1;
    }
    return gf_copy_to_user(argp, &get, sizeof(get));
}

int gf_ioctl_begin_miu_dump_perf_event(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_card_t *gf = priv->card;
    void __user *argp = (void __user*)arg;
    gf_begin_miu_dump_perf_event_t begin_miu_dump = {0};
    int ret = 0;

    if(gf_copy_from_user(&begin_miu_dump, argp, sizeof(gf_begin_miu_dump_perf_event_t)))
    {
        return -1;
    }

    ret = gf_core_interface->begin_miu_dump_perf_event(gf->adapter, &begin_miu_dump);

    return ret;
}

int gf_ioctl_end_perf_event(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_card_t   *gf = priv->card;
    void __user  *argp = (void __user*)arg;
    gf_end_perf_event_t end_perf_event = {0};
    int          ret = 0;

    if (gf_copy_from_user(&end_perf_event, argp, sizeof(gf_end_perf_event_t)))
    {
        return -1;
    }

    ret = gf_core_interface->end_perf_event(gf->adapter, &end_perf_event);

    return ret;
}

int gf_ioctl_end_miu_dump_perf_event(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_card_t *gf = priv->card;
    void __user *argp = (void __user*)arg;
    gf_end_miu_dump_perf_event_t end_miu_dump = {0};
    int ret = 0;

    if(gf_copy_from_user(&end_miu_dump, argp, sizeof(gf_end_miu_dump_perf_event_t)))
    {
        return -1;
    }

    ret = gf_core_interface->end_miu_dump_perf_event(gf->adapter, &end_miu_dump);

    return ret;
}


int gf_ioctl_set_miu_reg_list_perf_event(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_card_t *gf = priv->card;
    void __user *argp = (void __user*)arg;
    gf_miu_reg_list_perf_event_t miu_reg_list = {0};
    int ret = 0;

    if(gf_copy_from_user(&miu_reg_list, argp, sizeof(gf_miu_reg_list_perf_event_t)))
    {
        return -1;
    }

    ret = gf_core_interface->set_miu_reg_list_perf_event(gf->adapter, &miu_reg_list);

    return ret;
}

int gf_ioctl_get_miu_dump_perf_event(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_card_t *gf = priv->card;
    void __user *argp = (void __user*)arg;
    gf_get_miu_dump_perf_event_t get_miu_dump = {0};
    int ret = 0;

    if(gf_copy_from_user(&get_miu_dump, argp, sizeof(gf_get_miu_dump_perf_event_t)))

    ret = gf_core_interface->get_miu_dump_perf_event(gf->adapter, &get_miu_dump);

    if (ret == 0)
    {
        gf_copy_to_user(argp, &get_miu_dump, sizeof(gf_get_miu_dump_perf_event_t));
    }

    return ret;
}

int gf_ioctl_direct_get_miu_dump_perf_event(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_card_t *gf = priv->card;
    void __user *argp = (void __user*)arg;
    gf_direct_get_miu_dump_perf_event_t direct_get_dump = {0};
    int ret = 0;

    if(gf_copy_from_user(&direct_get_dump, argp, sizeof(gf_direct_get_miu_dump_perf_event_t)))
    {
        return -1;
    }

    ret = gf_core_interface->direct_get_miu_dump_perf_event(gf->adapter, &direct_get_dump);

    if(ret == 0)
    {
        gf_copy_to_user(argp, &direct_get_dump, sizeof(gf_direct_get_miu_dump_perf_event_t));
    }

    return ret;
}

int gf_ioctl_get_perf_event(gf_file_t *priv, unsigned int cmd, unsigned long arg)
{
    gf_card_t   *gf = priv->card;
    void __user  *argp = (void __user*)arg;
    gf_get_perf_event_t get_perf_event = {0};
    int          ret = 0;

    if (gf_copy_from_user(&get_perf_event, argp, sizeof(gf_get_perf_event_t)))
    {
        return -1;
    }

    ret = gf_core_interface->get_perf_event(gf->adapter, &get_perf_event);

    if (ret == 0)
    {
        gf_copy_to_user(argp, &get_perf_event, sizeof(gf_get_perf_event_t));
    }

    return ret;
}


int gf_ioctl_query_dvfs_clamp(gf_file_t *priv, unsigned cmd, unsigned long arg)
{
    gf_card_t            *gf = priv->card;
    void __user           *argp = (void __user*)arg;
    int               ret = -1;
    gf_dvfs_clamp_status_t query_dvfs_clamp;

    ret = gf_core_interface->query_dvfs_clamp(gf->adapter, &query_dvfs_clamp);

    if (ret == 0)
    {
        ret = gf_copy_to_user(argp, &query_dvfs_clamp, sizeof(query_dvfs_clamp));
    }

    return ret;
}

int gf_ioctl_dvfs_set(gf_file_t *priv, unsigned cmd, unsigned long arg)
{
    gf_card_t            *gf = priv->card;
    void __user           *argp = (void __user*)arg;
    int               ret = -1;
    gf_dvfs_set_t dvfs_set;

    if(gf_copy_from_user(&dvfs_set, argp, sizeof(gf_dvfs_set_t)))
    {
        return -1;
    }

    ret = gf_core_interface->dvfs_set(gf->adapter, &dvfs_set);

    return ret;
}

gf_ioctl_t gf_ioctls[] =
{
    [GF_IOCTL_NR(GF_IOCTL_WAIT_CHIP_IDLE)]           = gf_ioctl_wait_chip_idle,
    [GF_IOCTL_NR(GF_IOCTL_WAIT_ALLOCATION_IDLE)]     = gf_ioctl_wait_allocation_idle,
    [GF_IOCTL_NR(GF_IOCTL_QUERY_INFO)]               = gf_ioctl_query_info,
    [GF_IOCTL_NR(GF_IOCTL_CREATE_DEVICE)]            = gf_ioctl_create_device,
    [GF_IOCTL_NR(GF_IOCTL_DESTROY_DEVICE)]           = gf_ioctl_destroy_device,
    [GF_IOCTL_NR(GF_IOCTL_BEGIN_PERF_EVENT)]         = gf_ioctl_begin_perf_event,
    [GF_IOCTL_NR(GF_IOCTL_END_PERF_EVENT)]           = gf_ioctl_end_perf_event,
    [GF_IOCTL_NR(GF_IOCTL_GET_PERF_EVENT)]           = gf_ioctl_get_perf_event,
    [GF_IOCTL_NR(GF_IOCTL_SEND_PERF_EVENT)]          = gf_ioctl_send_perf_event,
    [GF_IOCTL_NR(GF_IOCTL_GET_PERF_STATUS)]          = gf_ioctl_get_perf_status,
    [GF_IOCTL_NR(GF_IOCTL_BEGIN_MIU_DUMP_PERF_EVENT)] = gf_ioctl_begin_miu_dump_perf_event,
    [GF_IOCTL_NR(GF_IOCTL_END_MIU_DUMP_PERF_EVENT)]  = gf_ioctl_end_miu_dump_perf_event,
    [GF_IOCTL_NR(GF_IOCTL_SET_MIU_REG_LIST_PERF_EVENT)] = gf_ioctl_set_miu_reg_list_perf_event,
    [GF_IOCTL_NR(GF_IOCTL_GET_MIU_DUMP_PERF_EVENT)]  = gf_ioctl_get_miu_dump_perf_event,
    [GF_IOCTL_NR(GF_IOCTL_DIRECT_GET_MIU_DUMP_PERF_EVENT)] = gf_ioctl_direct_get_miu_dump_perf_event,
    [GF_IOCTL_NR(GF_IOCTL_CREATE_CONTEXT)]           = gf_ioctl_create_context,
    [GF_IOCTL_NR(GF_IOCTL_DESTROY_CONTEXT)]          = gf_ioctl_destroy_context,
    [GF_IOCTL_NR(GF_IOCTL_RENDER)]                   = gf_ioctl_render,
    [GF_IOCTL_NR(GF_IOCTL_CREATE_FENCE_SYNC_OBJECT)] = gf_ioctl_create_fence_sync_object,
    [GF_IOCTL_NR(GF_IOCTL_DESTROY_FENCE_SYNC_OBJECT)]= gf_ioctl_destroy_fence_sync_object,
    [GF_IOCTL_NR(GF_IOCTL_WAIT_FENCE_SYNC_OBJECT)]   = gf_ioctl_wait_fence_sync_object,
    [GF_IOCTL_NR(GF_IOCTL_FENCE_VALUE)]              = gf_ioctl_fence_value,
    [GF_IOCTL_NR(GF_IOCTL_CREATE_DI_CONTEXT)]        = gf_ioctl_create_di_context,
    [GF_IOCTL_NR(GF_IOCTL_DESTROY_DI_CONTEXT)]       = gf_ioctl_destroy_di_context,
    [GF_IOCTL_NR(GF_IOCTL_GET_ALLOCATION_STATE)]     = gf_ioctl_get_allocation_state,
    [GF_IOCTL_NR(GF_IOCTL_ADD_HW_CTX_BUF)]           = gf_ioctl_add_hw_ctx_buf,
    [GF_IOCTL_NR(GF_IOCTL_RM_HW_CTX_BUF)]            = gf_ioctl_rm_hw_ctx_buf,
    [GF_IOCTL_NR(GF_IOCTL_CIL2_MISC)]                = gf_ioctl_cil2_misc,
    [GF_IOCTL_NR(GF_IOCTL_DRM_GEM_CREATE_ALLOCATION)]= gf_ioctl_gem_create_allocation,
    [GF_IOCTL_NR(GF_IOCTL_DRM_GEM_CREATE_RESOURCE)]  = gf_ioctl_gem_create_resource,
    [GF_IOCTL_NR(GF_IOCTL_DRM_GEM_MAP_GTT)]          = gf_ioctl_gem_map_gtt,
    [GF_IOCTL_NR(GF_IOCTL_DRM_BEGIN_CPU_ACCESS)]     = gf_ioctl_gem_begin_cpu_access,
    [GF_IOCTL_NR(GF_IOCTL_DRM_END_CPU_ACCESS)]       = gf_ioctl_gem_end_cpu_access,
    [GF_IOCTL_NR(GF_IOCTL_KMS_GET_PIPE_FROM_CRTC)]   = gf_ioctl_kms_get_pipe_from_crtc,
    [GF_IOCTL_NR(GF_IOCTL_DVFS_SET)]                 = gf_ioctl_dvfs_set,
    [GF_IOCTL_NR(GF_IOCTL_QUERY_DVFS_CLAMP)]         = gf_ioctl_query_dvfs_clamp,      
};

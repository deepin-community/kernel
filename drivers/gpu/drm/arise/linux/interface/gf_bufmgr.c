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

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <libdrm/drm.h>
#include <xf86drm.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "gf_bufmgr.h"
#include "surface_format.h"

static LIST_HEAD(bufmgr_list);
static pthread_mutex_t bufmgr_list_mutex = PTHREAD_MUTEX_INITIALIZER;

static void gf_bufmgr_lock(gf_bufmgr_t *bufmgr)
{
    if (bufmgr->enable_lock)
        pthread_mutex_lock(&bufmgr->mutex); 
}

static void gf_bufmgr_unlock(gf_bufmgr_t *bufmgr)
{
    if (bufmgr->enable_lock)
        pthread_mutex_unlock(&bufmgr->mutex); 
}

static gf_bufmgr_t *gf_bufmgr_find(int fd)
{
    gf_bufmgr_t *bufmgr;

    list_for_each_entry(bufmgr, &bufmgr_list, link)
    {
        if (bufmgr->fd == fd)
        {
            ++bufmgr->refcount;
            return bufmgr;
        }
    }

    return NULL;
}

static void gf_bo_free(gf_bo_t *bo)
{
    struct drm_gem_close close={0};

    if (bo->mapcount > 0)
    {
        bo->mapcount = 1;
        gf_bo_unmap(bo);
    }

    bo->mapcount = 0;
    bo->virt_addr = NULL;

    close.handle = bo->allocation;
    if (drmIoctl(bo->bufmgr->fd, DRM_IOCTL_GEM_CLOSE, &close) != 0)
    {
        bufmgr_err("destroy gem allocation failed.\n");
    }

    free(bo);
}

static void bufmgr_add_bo(gf_bufmgr_t *bufmgr, gf_bo_t *bo)
{
    unsigned int index = BO_HASH(bo->allocation);

    gf_bufmgr_lock(bufmgr);
    hlist_add_head(&bo->link, bufmgr->bo_hash + index);
    gf_bufmgr_unlock(bufmgr);
}

static gf_bo_t *bufmgr_get_bo(gf_bufmgr_t *bufmgr, unsigned int handle)
{
    gf_bo_t *bo = NULL, *node;

    gf_bufmgr_lock(bufmgr);

    hlist_for_each_entry(node, bufmgr->bo_hash + BO_HASH(handle), link)
    {
        if (node->allocation == handle)
        {
            bo = node;
            break;
        }
    }

    gf_bufmgr_unlock(bufmgr);

    return bo;
}

static void bufmgr_remove_bo(gf_bufmgr_t *bufmgr, gf_bo_t *bo)
{
    unsigned int index = BO_HASH(bo->allocation);

    gf_bufmgr_lock(bufmgr);

    hlist_del(&bo->link);

    gf_bufmgr_unlock(bufmgr);
}

//////////////////////////////////////////////////////////////////////
static gf_resource_t *gf_bufmgr_new_resource(gf_bufmgr_t *bufmgr, unsigned int resource_handle)
{
    gf_resource_t *resource = (gf_resource_t*)calloc(1, sizeof(*resource));

    INIT_LIST_HEAD(&resource->link);
    INIT_LIST_HEAD(&resource->bo_list);

    resource->bufmgr = bufmgr;
    resource->handle = resource_handle;

    gf_bufmgr_lock(bufmgr);
    list_add_tail(&resource->link, &bufmgr->resource_list);
    gf_bufmgr_unlock(bufmgr);

    return resource;
}

static void gf_bufmgr_destroy_resource(gf_resource_t *resource)
{
    bufmgr_assert(list_empty(&resource->bo_list));

    gf_bufmgr_lock(resource->bufmgr);
    list_del(&resource->link);
    gf_bufmgr_unlock(resource->bufmgr);

    free(resource);
}

static void gf_resource_add_bo(gf_resource_t *resource, gf_bo_t *bo)
{
    bufmgr_assert(bo->resource == NULL);
    bufmgr_assert(list_empty(&bo->res_link));

    bo->resource = resource;
    list_add_tail(&bo->res_link, &resource->bo_list);
}


static void gf_resource_remove_bo(gf_resource_t *resource, gf_bo_t *bo)
{
    list_del(&bo->res_link);
    bo->resource = NULL;
}

//////////////////////////////////////////////////////////////////////
gf_bo_t *gf_bo_get_from_handle(gf_bufmgr_t *bufmgr, unsigned int handle)
{
    return bufmgr_get_bo(bufmgr, handle);
}

static int gf_bo_query_info(gf_bufmgr_t *bufmgr, gf_bo_t *bo)
{
    gf_query_info_t query={0};
    gf_open_allocation_t open = {0, };

    query.type = GF_QUERY_ALLOCATION_INFO;
    query.argu = bo->allocation;
    query.buf = &open;

    if (bo->allocation == 0)
    {
        bufmgr_err("query_info for handle 0.!!!\n");
    }

    if (0 != gfQueryInfo(bufmgr->fd, &query))
        return -1;

    bo->size            = open.size;
    bo->width           = open.width;
    bo->height          = open.height;
    bo->alignment       = open.alignment;

    bo->compress_format = open.compress_format;
    bo->bl_slot_index   = open.bl_slot_index;
    bo->hw_format       = open.hw_format;
    bo->unpagable       = open.unpagable;
    bo->tiled           = open.tiled;
    bo->secured         = open.secured;

    bo->bit_cnt         = open.bit_cnt;
    bo->pitch           = open.pitch;
    bo->gpu_virt_addr   = open.gpu_virt_addr;
    bo->cpu_phy_addr    = open.cpu_phy_addr;
    bo->cpu_visible     = open.cpu_visible;
    bo->force_clear     = open.force_clear;

    bo->segment_id      = open.segment_id;
    bo->has_pages       = open.has_pages;

    bo->fence_addr      = open.fence_addr;
    bo->sync_obj        = open.sync_obj;

    bo->core_handle     = open.allocation;
    bo->maybe_shared    = 0;
    return 0;
}

gf_bo_t *gf_bo_alloc(gf_bufmgr_t *bufmgr, gf_bo_alloc_arg_t *arg)
{
    int ret = 0;
    gf_bo_t *bo;
    gf_create_allocation_t create = {0, };

    bo = (gf_bo_t *)calloc(1, sizeof(*bo));
    if (!bo)
        return NULL;

    create.device      = bufmgr->hDevice;
    create.width       = arg->width;
    create.height      = arg->height;
    create.format      = arg->format;
    create.tiled       = arg->tiled;
    create.unpagable   = arg->unpagable;
    create.usage_mask  = arg->usage_mask;
    create.access_hint = arg->access_hint;
    create.primary     = arg->primary;
    create.compressed  = arg->compressed;

    bufmgr_assert(!create.compressed || (create.access_hint == GF_ACCESS_GPU_ONLY));

    ret = gfGemCreateAllocation(bufmgr->fd, &create);

    if (ret == 0)
    {
        bo->allocation  = create.allocation;
        ret = gf_bo_query_info(bufmgr, bo);
        if (ret != 0)
            goto failed;

        bo->bufmgr            = bufmgr;
        bo->refcount          = 1;
        bo->mapcount          = 0;
        memcpy(&bo->alloc_arg, arg, sizeof(*arg));

        INIT_HLIST_NODE(&bo->link);
        bo->resource        = NULL;
        INIT_LIST_HEAD(&bo->res_link);
        INIT_LIST_HEAD(&bo->rename_link);

        bufmgr_add_bo(bufmgr, bo);
        bufmgr_dbg("gf_alloc_bo(handle:0x%x, w:%d, h:%d, fmt:%d, hwfmt:%d, tiled:%d, bit_cnt:%d, pitch:%d, size:%d",
                bo->allocation, bo->width, bo->height, arg->format, bo->hw_format, bo->tiled, bo->bit_cnt,
                bo->pitch, bo->size);
    }
    else
    {
failed:
        free(bo);
        bo = NULL;
    }

    return bo;
}

gf_bo_t *gf_bo_rename_alloc(gf_bufmgr_t *bufmgr, gf_bo_t *ref)
{
    int ret = 0;
    gf_bo_t *bo;
    gf_create_allocation_t create = {0, };

    if (!ref)
        return NULL;

    bo = (gf_bo_t *)calloc(1, sizeof(*bo));
    if (!bo)
        return NULL;

    create.device      = bufmgr->hDevice;
    create.reference   = ref->allocation;

    ret = gfGemCreateAllocation(bufmgr->fd, &create);

    if (ret == 0)
    {
        bo->allocation  = create.allocation;
        ret = gf_bo_query_info(bufmgr, bo);
        if (ret != 0)
            goto failed;

        bo->bufmgr            = bufmgr;
        bo->refcount          = 1;
        bo->mapcount          = 0;

        INIT_HLIST_NODE(&bo->link);
        bo->resource        = NULL;
        INIT_LIST_HEAD(&bo->res_link);
        INIT_LIST_HEAD(&bo->rename_link);

        bufmgr_add_bo(bufmgr, bo);
        bufmgr_dbg("gf_bo_rename_alloc(handle:0x%x, w:%d, h:%d, hwfmt:%d, tiled:%d, bit_cnt:%d, pitch:%d, size:%d",
                bo->allocation, bo->width, bo->height, bo->hw_format, bo->tiled, bo->bit_cnt,
                bo->pitch, bo->size);
    }
    else
    {
failed:
        free(bo);
        bo = NULL;
    }

    return bo;
}

static int get_hw_format(int format)
{
    Hw_Surf_Format hw_format = 0;

    switch(format)
    {
    case GF_FORMAT_A8_UNORM:
        hw_format = HSF_A8_UNORM;
        break;

    case GF_FORMAT_B5G6R5_UNORM:
        hw_format = HSF_B5G6R5_UNORM;
        break;

    case GF_FORMAT_B8G8R8A8_UNORM:
        hw_format = HSF_B8G8R8A8_UNORM;
        break;

    case GF_FORMAT_B8G8R8X8_UNORM:
        hw_format = HSF_B8G8R8X8_UNORM;
        break;

    case GF_FORMAT_YUY2:
        hw_format = HSF_YUY2;
        break;

    case GF_FORMAT_NV12_LINEAR:
        hw_format = HSF_NV12;
        break;

    case GF_FORMAT_FLOAT32:
        hw_format = HSF_D32_FLOAT;
        break;

    case GF_FORMAT_UINT32:
        hw_format = HSF_R32_UINT;
        break;

    case GF_FORMAT_INT32:
        hw_format = HSF_R32_SINT;
        break;

    case GF_FORMAT_R8_UNORM:
        hw_format = HSF_R8_UNORM;
        break;

    case GF_FORMAT_R8G8_UNORM:
        hw_format = HSF_R8G8_UNORM;
        break;

    case GF_FORMAT_R16_UNORM:
        hw_format = HSF_R16_UNORM;
        break;

    case GF_FORMAT_R16G16_UNORM:
        hw_format = HSF_R16G16_UNORM;
        break;

    default:
        bufmgr_err("unknow gf format :%d.\n", format);
        bufmgr_assert(0);
        break;
    }

    return hw_format;
}

static int get_format_bitcnt(int format)
{
    int bit_cnt = 0;

    switch(format)
    {
    case GF_FORMAT_A8_UNORM:
    case GF_FORMAT_NV12_LINEAR:
        bit_cnt = 8;
        break;

    case GF_FORMAT_B5G6R5_UNORM:
    case GF_FORMAT_YUY2:
        bit_cnt = 16;
        break;

    case GF_FORMAT_B8G8R8A8_UNORM:
    case GF_FORMAT_B8G8R8X8_UNORM:
    case GF_FORMAT_FLOAT32:
    case GF_FORMAT_UINT32:
    case GF_FORMAT_INT32:
        bit_cnt = 32;
        break;

    default:
        bufmgr_err("unknow gf format :%d.\n", format);
        bufmgr_assert(0);
        break;
    }

    return bit_cnt;
}

static int calc_pitch(int width, int bitcnt, int usage_mask)
{
    int bMip0NonPow2 = width & (width - 1);
    int pitch = 0;

    if(bMip0NonPow2)
    {
        pitch = (bitcnt < 96) ? (width * bitcnt + 511)/512 * 64 : (width * 128 + 1023)/1024 * 128;
    }
    else
    {
        pitch = (width * bitcnt + 511)/512 * 64;
    }

    /*Temp fix linear texture 3dblit need pitch 256 bytes align*/
    if(usage_mask & (GF_USAGE_TEXTURE_BUFFER|GF_USAGE_FORCE_PCIE))
        pitch = (pitch + 255) & (~255);

    return pitch;
}

static int gf_bo_check_userptr_args(void *userptr, int size, int width, int height, int pitch, int bitcnt, int format, int usage_mask)
{
    int pitch_expect = 0;

    if (userptr == NULL || ((size_t)userptr % 0x1000) != 0)
    {
        return -1;
    }

    if (size == 0 || (size % 0x1000) != 0)
    {
        return -1;
    }

    pitch_expect = calc_pitch(width, bitcnt, usage_mask);
    if (pitch != pitch_expect)
    {
        bufmgr_dbg("%s() expecte pitch should be %d, not pitch %d\n", __func__, pitch_expect, pitch);
        return -1;
    }

    return 0;
}

int gf_bo_relayout(gf_bo_t *bo, int width, int height, int pitch, int bit_cnt, int format)
{
    if(!width || !height || !pitch || !bit_cnt || !format)
    {
        return 1;
    }

    bo->width = width;
    bo->height = height;
    bo->pitch = pitch;
    bo->bit_cnt = bit_cnt;

    if (format)
    {
        bo->hw_format = get_hw_format(format);
    }

    return 1;
}

static gf_bo_t *gf_bo_create_from_handle_internal(gf_bufmgr_t *bufmgr, unsigned int handle)
{
    gf_bo_t *bo = NULL;

    bo = bufmgr_get_bo(bufmgr, handle);
    if (!bo)
    {
        bo = (gf_bo_t *)calloc(1, sizeof(*bo));
        bo->allocation      = handle;
        if (gf_bo_query_info(bufmgr, bo) == 0)
        {
            bo->bufmgr          = bufmgr;
            bo->refcount        = 1;
            bo->mapcount        = 0;

            INIT_HLIST_NODE(&bo->link);
            bo->resource        = NULL;
            INIT_LIST_HEAD(&bo->res_link);
            INIT_LIST_HEAD(&bo->rename_link);

            bufmgr_add_bo(bufmgr, bo);
        }
        else
        {
            free(bo);
            bo = NULL;
        }
    }
    else
    {
        bo->refcount++;
    }

    return bo;
}

gf_bo_t *gf_bo_create_from_handle(gf_bufmgr_t *bufmgr, unsigned int handle, int width, int height, int pitch, int bit_cnt, int format)
{
    gf_bo_t *bo = NULL;

    bo = gf_bo_create_from_handle_internal(bufmgr, handle);

    bufmgr_dbg("gf_bo_create_from_handle(w:%d h:%d pitch:%d bit_cnt:%d format:%d",
            width, height, pitch, bit_cnt, format);

    if (!bo)
        return NULL;

    if (bo->refcount == 1)
    {
        if (!gf_bo_relayout(bo, width, height, pitch, bit_cnt, format))
        {
            bufmgr_err("bo relayout failed.\n");
            gf_bo_unreference(bo);
            bo = NULL;
        }
    }
    else
    {
        if (bo->width != width ||
            bo->height != height ||
            bo->pitch != pitch ||
            bo->bit_cnt != bit_cnt ||
            (format && bo->hw_format != get_hw_format(format))
            )
        {
            bufmgr_info("%s() bo:%d, refcount:%d, invalid_layout\n", __func__, bo->allocation, bo->refcount);
            bo->invalid_layout = TRUE;
        }
    }

    return bo;
}

gf_bo_t *gf_bo_create_from_userptr(gf_bufmgr_t *bufmgr, void *userptr, int size, int width, int height, int pitch, int format)
{
    gf_create_allocation_t create = {0, };
    gf_bo_t *bo = NULL;
    int usage_mask = GF_USAGE_FORCE_PCIE;
    int bitcnt = 0;
    int ret = 0;

    bitcnt = get_format_bitcnt(format);
    if (bitcnt <= 0)
    {
        bufmgr_err("%s() bitcnt-%d is invalid for format-0x%x\n", __func__, bitcnt, format);
        goto failed;
    }

    ret = gf_bo_check_userptr_args(userptr, size, width, height, pitch, bitcnt, format, usage_mask);
    if (ret != 0)
    {
        bufmgr_err("%s() argus doesn't match for width-%d, height-%d, pitch-%d, bitcnt-%d, format-0x%x\n", 
                __func__, width, height, pitch, bitcnt, format);
        goto failed;
    }

    bo = (gf_bo_t *)calloc(1, sizeof(*bo));
    if (!bo)
        return NULL;

    create.device      = bufmgr->hDevice;
    create.width       = width * height;
    create.height      = 1;
    create.format      = format;
    create.tiled       = 0;
    create.unpagable   = 1;
    create.usage_mask  = usage_mask;
    create.access_hint = GF_ACCESS_CPU_ALMOST;
    create.primary     = 0;
    create.user_ptr      = (unsigned long)userptr;
    create.user_buf_size = size;

    ret = gfGemCreateAllocation(bufmgr->fd, &create);
    if (ret != 0)
    {
        bufmgr_err("%s() fail to create gem allocation\n", __func__);
        goto failed;
    }

    bo->allocation  = create.allocation;
    ret = gf_bo_query_info(bufmgr, bo);
    if (ret != 0)
    {
        bufmgr_err("%s() fail to query info: bo-0x%x\n", __func__, bo->allocation);
        goto failed;
    }

    if (!gf_bo_relayout(bo, width, height, pitch, bitcnt, format))
    {
        bufmgr_err("%s() fail to relayout for bo-0x%x, width-%d, height-%d, pitch-%d, bitcnt-%d, format-0x%x!\n", 
                __func__, bo->allocation, width, height, pitch, bitcnt, format);
        goto failed;
    }

    bo->bufmgr            = bufmgr;
    bo->refcount          = 1;
    bo->mapcount          = 0;

    INIT_HLIST_NODE(&bo->link);
    bo->resource        = NULL;
    INIT_LIST_HEAD(&bo->res_link);
    INIT_LIST_HEAD(&bo->rename_link);

    bufmgr_add_bo(bufmgr, bo);
    bufmgr_dbg("gf_alloc_bo(handle:0x%x, w:%d, h:%d, fmt:%d, hwfmt:%d, tiled:%d, bit_cnt:%d, pitch:%d, size:%d",
            bo->allocation, bo->width, bo->height, format, bo->hw_format, bo->tiled, bo->bit_cnt, bo->pitch, bo->size);

    return bo;

failed:
    if (bo)
    {
        if (bo->allocation)
            gfGemDestroyObject(bufmgr->fd, bo->allocation);
        free(bo);
        bo = NULL;
    }

    return bo;
}

int gf_bo_get_fd(gf_bo_t *bo, int *fd)
{
    int ret = 0;
    struct drm_prime_handle args = {0};

    args.fd = -1;
    args.handle = bo->allocation;
    args.flags = DRM_CLOEXEC;

    ret = drmIoctl(bo->bufmgr->fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &args);
    if(ret)
    {
        bufmgr_err("%s() fail to get fd  for gem_handle-0x%x\n", __func__, bo->allocation);
        return -1;
    }

    bo->maybe_shared = 1;
    *fd = args.fd;
    return 0;
}


gf_bo_t *gf_bo_create_from_fd(gf_bufmgr_t *bufmgr, int fd, int width, int height, int pitch, int bit_cnt, int format)
{
    int ret = 0;
    gf_bo_t *bo;
    struct drm_prime_handle args = {0};

    args.fd = fd;
    ret = drmIoctl(bufmgr->fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &args);
    if (ret != 0)
    {
        bufmgr_err("%s() DRM_IOCTL_PRIME_FD_TO_HANDLE failed %d.\n", __func__, ret);
        return NULL;
    }

    bo = gf_bo_create_from_handle(bufmgr, args.handle, width, height, pitch, bit_cnt, format);
    if (bo)
        bo->maybe_shared = 1;
    return bo;
}

void gf_bo_reference(gf_bo_t *bo)
{
    bo->refcount++;
}

void gf_bo_unreference(gf_bo_t *bo)
{
    gf_bufmgr_t *bufmgr = bo->bufmgr;

    if (--bo->refcount == 0)
    {
        bufmgr_remove_bo(bufmgr, bo);

        if (bo->resource)
            gf_resource_remove_bo(bo->resource, bo);

        if (bo->rename)
        {
            gf_bo_t *rename_bo, *next;

            list_splice_init(&bo->rename->reference_list, &bo->rename->unreference_list);

            list_for_each_entry_safe(rename_bo, next, &bo->rename->unreference_list, rename_link)
            {
                bufmgr_assert(rename_bo->rename == bo->rename);

                list_del(&rename_bo->rename_link);
                if (rename_bo != bo)
                {
                    rename_bo->rename = NULL;
                    gf_bo_unreference(rename_bo);
                }
            }

            free(bo->rename);
            bo->rename = NULL;
        }

        gf_bo_free(bo);
    }
}

int gf_bo_map(gf_bo_t *bo, map_args_t *arg)
{
    void *virt_addr = NULL;
    int ret = 0;
    int create_new = 0;
    gf_bo_t *target = NULL;
    gf_rename_resource_t *rename = bo->rename;
    gf_drm_gem_begin_cpu_access_t begin = {0, };

    if (arg->flags.discard)
    {
        if (!rename)
        {
            if (arg->flags.no_existing_reference)
            {
                target = bo;
            }
        }
        else
        {
            if (arg->flags.no_existing_reference)
            {
                list_splice_tail_init(&rename->reference_list, &rename->unreference_list);
            }

            if (!gf_bo_is_busy(bo))
            {
                target = bo;
            }
            else
            {
                target = list_first_entry_or_null(&rename->unreference_list, gf_bo_t, rename_link);
            }

            if (!target || gf_bo_is_busy(target))
            {
                create_new = (rename->allocation_num < rename->max_length) ? 1 : 0;
            }

            if (create_new)
            {
                gf_bo_t *new_bo = gf_bo_rename_alloc(bo->bufmgr, bo);
                if (new_bo)
                {
                    target = new_bo;
                    target->rename = rename;
                    rename->allocation_num++;
                }
            }
        }

        if (target)
        {
            list_move_tail(&target->rename_link, &rename->reference_list);
        }
    }
    else
    {
        target = bo;
    }

    if (!target)
    {
        return -1;
    }

    if(!target->cpu_visible)
    {
        arg->allocation = target->allocation;
        arg->virt_addr = 0;
        return 0;
    }

    if (target->mapcount++ == 0)
    {
        bufmgr_assert(!arg->flags.acquire_aperture);
        bufmgr_assert(target->tiled || !arg->flags.acquire_aperture);

        if (!target->gtt_offset)
        {
            gf_drm_gem_map_t map = {0};

            map.gem_handle = target->allocation;
            map.delay_map = arg->flags.delay_map;
            map.prefault_num = arg->prefault_num;

            ret = gfGemMapGtt(target->bufmgr->fd, &map);
            if (ret != 0)
                goto failed;

            target->gtt_offset = map.offset;
        }

        bufmgr_assert((target->gtt_offset & 4095) == 0);
        virt_addr = mmap(0, target->size, PROT_READ | PROT_WRITE, MAP_SHARED, target->bufmgr->fd, target->gtt_offset);
        if(virt_addr == MAP_FAILED)
            goto failed;

        target->virt_addr = virt_addr;
        bufmgr_assert(ret == 0 && target->virt_addr);
    }

    bufmgr_assert(arg->flags.acquire_aperture == target->use_aperture);

    arg->allocation = target->allocation;
    arg->virt_addr  = target->virt_addr;

    begin.handle = target->allocation;
    begin.offset = 0;
    begin.size = target->size;
    begin.readonly = arg->flags.read_only;
    ret = gfGemBeginCpuAccess(target->bufmgr->fd, &begin);
    return ret;

aperture_failed:
    munmap(virt_addr,  target->size);
failed:
    target->mapcount--;
    return -1;
}

int gf_bo_unmap(gf_bo_t *bo)
{
    int ret = 0;
    gf_drm_gem_end_cpu_access_t end = {0, };
    if (--bo->mapcount == 0)
    {
        bufmgr_assert(!bo->use_aperture);
        ret = munmap(bo->virt_addr,  bo->size);
        bo->virt_addr = NULL;
    }

    end.handle = bo->allocation;
    gfGemEndCpuAccess(bo->bufmgr->fd, &end);

    return ret;
}

static gf_bufmgr_t *gf_bufmgr_create(int fd, unsigned int hDevice, int enable_lock)
{
    int i;
    gf_bufmgr_t *bufmgr = (gf_bufmgr_t *)calloc(1, sizeof(*bufmgr));

    bufmgr->fd      = fd;
    bufmgr->hDevice = hDevice;
    bufmgr->enable_lock = enable_lock;
    bufmgr->resource_id = 0;
    bufmgr->refcount = 1;
    INIT_LIST_HEAD(&bufmgr->link);
     
    for (i = 0; i < BO_HASH_SIZE; i++)
    {
        INIT_HLIST_HEAD(&bufmgr->bo_hash[i]);
    }

    INIT_LIST_HEAD(&bufmgr->resource_list);

    if (bufmgr->enable_lock)
        pthread_mutex_init(&bufmgr->mutex, NULL);

    return bufmgr;
}

void gf_bufmgr_destroy(gf_bufmgr_t *bufmgr)
{
    int i;
    gf_bo_t *bo;
    struct hlist_node *tmp;

    pthread_mutex_lock(&bufmgr_list_mutex);
    if (--bufmgr->refcount == 0)
    {
        list_del(&bufmgr->link);
        for(i = 0; i < BO_HASH_SIZE; i++)
        {
            hlist_for_each_entry_safe(bo, tmp, bufmgr->bo_hash + i, link)
            {
                hlist_del(&bo->link);
                gf_bo_free(bo);
            }
        }
        free(bufmgr);
    }
    pthread_mutex_unlock(&bufmgr_list_mutex);
}

gf_bufmgr_t *gf_bufmgr_init(int fd)
{
    gf_bufmgr_t *bufmgr;
    gf_create_device_t create = {0, };
    
    pthread_mutex_lock(&bufmgr_list_mutex);
    bufmgr = gf_bufmgr_find(fd);
    if (bufmgr)
        goto done;
    
    gfCreateDevice(fd, &create);
    bufmgr_assert(create.device);

    bufmgr = gf_bufmgr_create(fd, create.device, 1);
    bufmgr_assert(bufmgr);

    list_add_tail(&bufmgr->link, &bufmgr_list);
done:
    pthread_mutex_unlock(&bufmgr_list_mutex);

    return bufmgr;
}


void gf_bo_wait_idle(gf_bo_t *bo)
{
    gf_wait_allocation_idle_t wait = {0, };

    wait.engine_mask = ALL_ENGINE_MASK;
    wait.hAllocation = bo->allocation;

    gfWaitAllocationIdle(bo->bufmgr->fd, &wait);
}

static gf_resource_t* gf_resource_get_from_handle(gf_bufmgr_t *bufmgr, unsigned int handle)
{
    gf_resource_t *res = NULL, *node;

    gf_bufmgr_lock(bufmgr);
    list_for_each_entry(node, &bufmgr->resource_list, link)
    {
        if (node->handle == handle)
        {
            res = node;
            break;
        }
    }
    gf_bufmgr_unlock(bufmgr);

    return res;
}

int gf_bo_create_resource (
        gf_bufmgr_t               *bufmgr,
        unsigned int               device,
        unsigned int               allocation_num,
        gf_create_alloc_info_t     *allocation_info,
        unsigned int               create_resource_flag,
        unsigned int               *resource_handle
    )
{
    gf_create_resource_t create = {0};
    gf_resource_t *resource = NULL;
    int i = 0;
    int ret = 0;

    if (*resource_handle)
    {
        resource = gf_resource_get_from_handle(bufmgr, *resource_handle);
        bufmgr_assert(resource);
    }

    create.device = device;
    create.NumAllocations = allocation_num;
    create.pAllocationInfo = ptr_to_ptr64(allocation_info);

    gfGemCreateResource(bufmgr->fd, &create);

    if (!resource && create_resource_flag)
    {
        unsigned int handle;

        gf_bufmgr_lock(bufmgr);
        handle = ++bufmgr->resource_id;
        gf_bufmgr_unlock(bufmgr);
        resource = gf_bufmgr_new_resource(bufmgr, handle);
    }

    if (resource)
    {
        *resource_handle = resource->handle;
    }

    for(i = 0; i < allocation_num; i++)
    {
        gf_bo_t *bo = NULL;
        bo = gf_bo_create_from_handle_internal(bufmgr, allocation_info[i].hAllocation);
        if(resource)
        {
            gf_resource_add_bo(resource, bo);
        }

        if (allocation_info[i].MaximumRenamingListLength > 0)
        {
            bo->rename = calloc(1, sizeof(gf_rename_resource_t));
            bo->rename->max_length = allocation_info[i].MaximumRenamingListLength;
            INIT_LIST_HEAD(&bo->rename->reference_list);
            INIT_LIST_HEAD(&bo->rename->unreference_list);
            bo->rename->allocation_num = 1;
            list_add_tail(&bo->rename_link, &bo->rename->unreference_list);
            bufmgr_info("require renaming...\n");
        }
    }

    return ret;
}

int gf_bo_destroy_resource(
    gf_bufmgr_t *bufmgr,
    unsigned int     device,
    unsigned int     allocation_num,
    unsigned int    *allocation_info,
    unsigned int     resource_handle
    )
{
    gf_bo_t  *bo = NULL;
    int i = 0;
    int ret = 0;

    if(resource_handle)
    {
        gf_resource_t *resource = gf_resource_get_from_handle(bufmgr, resource_handle);
        bufmgr_assert(resource);

        if (resource)
        {
            gf_bo_t *bo, *tmp;
            list_for_each_entry_safe(bo, tmp, &resource->bo_list, res_link)
            {
                gf_resource_remove_bo(resource, bo);
                gf_bo_unreference(bo);
            }
            gf_bufmgr_destroy_resource(resource);
        }
    }
    else
    {
        for( i = 0; i < allocation_num; i++)
        {
            bo = bufmgr_get_bo(bufmgr, allocation_info[i]);

            bufmgr_assert(bo);
            gf_bo_unreference(bo);
        }
    }

    return ret;
}

int gf_bo_is_busy(gf_bo_t *bo)
{
    int ret;
    gf_bufmgr_t *bufmgr = bo->bufmgr;
    gf_get_allocation_state_t state = {0, };

    state.hAllocation = bo->allocation;
    state.engine_mask = ALL_ENGINE_MASK;

    ret = gfGetAllocationState(bufmgr->fd, &state);

    if (ret)
        return -1;

    return state.state;
}

unsigned int gf_bo_get_name(gf_bo_t *bo)
{
	struct drm_gem_flink flink= {0};

    if (!bo->name)
    {
        flink.handle = bo->allocation;
        if (0 == drmIoctl(bo->bufmgr->fd, DRM_IOCTL_GEM_FLINK, &flink))
        {
            bo->name = flink.name;
        }
        else
        {
            bufmgr_err("%s() fail get name for gem_buffer-0x%x\n", __func__, bo->allocation);
        }
    }
    bo->maybe_shared = 1;
    return bo->name;
}

gf_bo_t *gf_bo_create_from_name(gf_bufmgr_t *bufmgr, unsigned int name, int width, int height, int pitch, int bit_cnt, int format)
{
    struct drm_gem_open args = {0};
    gf_bo_t *bo=NULL;
    int ret = 0;

    args.name = name;
    ret = drmIoctl(bufmgr->fd, DRM_IOCTL_GEM_OPEN, &args);
    if (ret < 0)
    {
        bufmgr_err("%s() fail to open buffer-0x%x\n", __func__, name);
        return NULL;
    }

    bo = gf_bo_create_from_handle(bufmgr, args.handle, width, height, pitch, bit_cnt, format);
    if(bo)
    {
        bo->name = name;
        bo->maybe_shared = 1;
    }

    return bo;
}

static struct bufmgr_interface bufmgr_interface_struct_v2 = {
    .bufmgr_init = gf_bufmgr_init,
    .bufmgr_destroy = gf_bufmgr_destroy,
    .bo_alloc = gf_bo_alloc,
    .bo_rename_alloc = gf_bo_rename_alloc,
    .bo_get_from_handle = gf_bo_get_from_handle,
    .bo_create_from_handle = gf_bo_create_from_handle,
    .bo_create_from_userptr = gf_bo_create_from_userptr,
    .bo_create_from_fd = gf_bo_create_from_fd,
    .bo_relayout = gf_bo_relayout,
    .bo_reference = gf_bo_reference,
    .bo_unreference = gf_bo_unreference,
    .bo_is_busy = gf_bo_is_busy,
    .bo_map = gf_bo_map,
    .bo_unmap = gf_bo_unmap,
    .bo_get_fd = gf_bo_get_fd,
    .bo_wait_idle = gf_bo_wait_idle,
    .bo_dump_bmp = gf_bo_dump_bmp,
    .bo_get_name = gf_bo_get_name,
    .bo_create_from_name = gf_bo_create_from_name,
    .bo_create_resource = gf_bo_create_resource,
    .bo_destroy_resource = gf_bo_destroy_resource,
};

__attribute__((visibility("default")))
struct bufmgr_interface *bufmgr_interface_v2 = &bufmgr_interface_struct_v2;

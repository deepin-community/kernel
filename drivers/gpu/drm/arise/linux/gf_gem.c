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
#include "gf.h"
#include "gf_debugfs.h"
#include "gf_gem.h"
#include "gf_gem_priv.h"
#include "gf_fence.h"
#include "gf_driver.h"
#include "gf_trace.h"
#include "os_interface.h"

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
#if DRM_VERSION_CODE < KERNEL_VERSION(6, 13, 0)
MODULE_IMPORT_NS(DMA_BUF);
#else
MODULE_IMPORT_NS("DMA_BUF");
#endif
#endif

extern struct os_pages_memory* gf_allocate_pages_memory_struct(int page_cnt, struct sg_table *st);
extern void gf_pages_memory_extract_st(struct os_pages_memory *memory);
extern unsigned char gf_validate_page_cache(struct os_pages_memory *memory, int start_page, int end_page, unsigned char request_cache_type);
int gf_gem_debugfs_add_object(struct drm_gf_gem_object *obj);
void gf_gem_debugfs_remove_object(struct drm_gf_gem_object *obj);

#define GF_DEFAULT_PREFAULT_NUM 16
#define NUM_PAGES(x) (((x) + PAGE_SIZE-1) / PAGE_SIZE)

#if DRM_VERSION_CODE < KERNEL_VERSION(5,18,0)
#define DMABUF_PREFIX(x) dma_buf_##x
#else
#define DMABUF_PREFIX(x) iosys_##x
#endif

#if DRM_VERSION_CODE < KERNEL_VERSION(5,11,0)
#define DMA_BUF_MAP_CLEAR() do {} while(0)
#else
#define DMA_BUF_MAP_CLEAR() do { DMABUF_PREFIX(map_clear)(map); } while(0)
#endif

unsigned int gf_get_from_gem_handle(void *file_, unsigned int handle)
{
    struct drm_file *file = file_;
    gf_file_t  *priv = file->driver_priv;

    return gf_gem_get_core_handle(priv, handle);
}

static int gf_gem_object_pin_pages(struct drm_gf_gem_object *obj)
{
    int err = 0;

    mutex_lock(&obj->mm.lock);
    if (++obj->mm.pages_pin_count == 1)
    {
        gf_assert(!obj->mm.pages, GF_FUNC_NAME(__func__));
        err = obj->ops->get_pages(obj);
        if (err)
            --obj->mm.pages_pin_count;
    }
    mutex_unlock(&obj->mm.lock);
    return err;
}

static void gf_gem_object_unpin_pages(struct drm_gf_gem_object *obj)
{
    struct sg_table *pages;

    mutex_lock(&obj->mm.lock);
    gf_assert(obj->mm.pages_pin_count > 0, GF_FUNC_NAME(__func__));
    if (--obj->mm.pages_pin_count == 0)
    {
        pages = obj->mm.pages;
        obj->mm.pages = NULL;
        if (!IS_ERR(pages))
            obj->ops->put_pages(obj, pages);
    }
    mutex_unlock(&obj->mm.lock);
}

void gf_gem_free_object(struct drm_gem_object *gem_obj)
{
    struct drm_gf_gem_object *obj = to_gf_bo(gem_obj);
    gf_card_t *gf = obj->base.dev->dev_private;

    trace_gfx_drm_gem_release_object(gf->index, obj);

    drm_gem_object_release(&obj->base);

    if (obj->ops->release)
    {
        obj->ops->release(obj);
    }

    reservation_object_fini(&obj->__builtin_resv);

    gf_assert(!obj->mm.pages_pin_count, GF_FUNC_NAME(__func__));
    gf_assert(!obj->mm.pages, GF_FUNC_NAME(__func__));

    gf_free(obj);
}

static struct sg_table *gf_gem_vram_map_dma_buf(struct device *dev, phys_addr_t phys, size_t size, enum dma_data_direction dir)
{
    struct sg_table *st = NULL;
    struct scatterlist *sg = NULL;
    dma_addr_t dma_addr;

    st = gf_malloc(sizeof(*st));
    if (!st)
        return NULL;

    if (sg_alloc_table(st, 1, GFP_KERNEL))
        goto error_free;

    sg = st->sgl;

#if LINUX_VERSION_CODE > KERNEL_VERSION(4,8,0)
    dma_addr = dma_map_resource(dev, phys, size, dir, DMA_ATTR_SKIP_CPU_SYNC);
    if (dma_mapping_error(dev, dma_addr))
        goto error_free_table;

    sg_dma_address(sg) = dma_addr;
#else
    sg_dma_address(sg) = phys;
#endif

    sg_set_page(sg, NULL, size, 0);
    sg_dma_len(sg) = size;

    return st;

error_free_table:
    gf_error("vram gem map fail\n");
    sg_free_table(st);

error_free:
    gf_free(st);

    return NULL;
}

static struct sg_table *gf_gem_pages_map_dma_buf(struct device *dev, struct sg_table *pages, enum dma_data_direction dir)
{
    struct sg_table *st;
    struct scatterlist *src, *dst;
    int i;

    st = kmalloc(sizeof(struct sg_table), GFP_KERNEL);
    if (!st)
        goto err_exit;

    if (sg_alloc_table(st, pages->nents, GFP_KERNEL))
        goto err_free;

    src = pages->sgl;
    dst = st->sgl;
    for (i = 0; i < pages->nents; i++)
    {
        sg_set_page(dst, sg_page(src), src->length, 0);
        dst = sg_next(dst);
        src = sg_next(src);
    }

    st->nents = dma_map_sg(dev, st->sgl, st->orig_nents, dir);
    if (!st->nents)
        goto err_free_sg;

    return st;

err_free_sg:
    sg_free_table(st);
err_free:
    kfree(st);
err_exit:
    return NULL;
}

static struct sg_table *gf_gem_map_dma_buf(struct dma_buf_attachment *attachment, enum dma_data_direction dir)
{
    struct sg_table *st = NULL;
    struct drm_gf_gem_object *obj = dmabuf_to_gf_bo(attachment->dmabuf);
    gf_card_t *gf = obj->base.dev->dev_private;
    gf_map_argu_t map_argu = {0, };

    gf_core_interface->prepare_and_mark_unpagable(gf->adapter, obj->core_handle, &obj->info);

    if (gf_gem_object_pin_pages(obj))
        goto err_exit;

    map_argu.flags.mem_space = GF_MEM_KERNEL;
    map_argu.flags.read_only = false;
    gf_core_interface->get_map_allocation_info(gf->adapter, obj->core_handle, &map_argu);

    if (map_argu.flags.mem_type == GF_SYSTEM_RAM)
        st = gf_gem_pages_map_dma_buf(attachment->dev, obj->mm.pages, dir);
    else
        st = gf_gem_vram_map_dma_buf(attachment->dev, map_argu.phys_addr, map_argu.size, dir);

    if (!st)
        goto err_unpin_pages;

    return st;

err_unpin_pages:
    gf_gem_object_unpin_pages(obj);
err_exit:
    gf_core_interface->mark_pagable(gf->adapter, obj->core_handle);
    return NULL;
}

static void gf_gem_unmap_dma_buf(struct dma_buf_attachment *attachment, struct sg_table *sg, enum dma_data_direction dir)
{
    struct drm_gf_gem_object *obj = dmabuf_to_gf_bo(attachment->dmabuf);
    gf_card_t *gf = obj->base.dev->dev_private;

    if (obj->mm.pages)
        dma_unmap_sg(attachment->dev, sg->sgl, sg->nents, dir);
#if LINUX_VERSION_CODE > KERNEL_VERSION(4,8,0)
    else
        dma_unmap_resource(attachment->dev, sg->sgl->dma_address, sg->sgl->length, dir, DMA_ATTR_SKIP_CPU_SYNC);
#endif

    sg_free_table(sg);
    kfree(sg);

    gf_gem_object_unpin_pages(obj);
    gf_core_interface->mark_pagable(gf->adapter, obj->core_handle);
}

void *gf_gem_object_vmap(struct drm_gf_gem_object *obj)
{
    gf_card_t *gf = obj->base.dev->dev_private;
    gf_vm_area_t *vma = NULL;
    gf_map_argu_t map_argu = {0, };

    gf_core_interface->prepare_and_mark_unpagable(gf->adapter, obj->core_handle, &obj->info);

    mutex_lock(&obj->mm.lock);
    if (obj->krnl_vma)
    {
        vma = obj->krnl_vma;
        obj->krnl_vma->ref_cnt++;
        goto unlock;
    }

    map_argu.flags.mem_space = GF_MEM_KERNEL;
    map_argu.flags.read_only = false;
    gf_core_interface->get_map_allocation_info(gf->adapter, obj->core_handle, &map_argu);

    gf_assert(map_argu.flags.mem_space == GF_MEM_KERNEL, GF_FUNC_NAME(__func__));
    if (map_argu.flags.mem_type == GF_SYSTEM_IO)
    {
        vma = gf_map_io_memory(NULL, &map_argu);
    }
    else if (map_argu.flags.mem_type == GF_SYSTEM_RAM)
    {
        vma = gf_map_pages_memory(NULL, &map_argu);
    }
    else
    {
        gf_assert(0, GF_FUNC_NAME(__func__));
    }

#if DRM_VERSION_CODE >= KERNEL_VERSION(5,11,0)
    obj->vmap_mem_type = map_argu.flags.mem_type;
#endif

    obj->krnl_vma = vma;
unlock:
    mutex_unlock(&obj->mm.lock);
    return vma ? vma->virt_addr : NULL;
}

#if DRM_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
void *gf_gem_prime_vmap(struct drm_gem_object *gem_obj)
{
    struct drm_gf_gem_object *gf_gem_obj= to_gf_bo(gem_obj);
    void *virt_addr = NULL;

    virt_addr = gf_gem_object_vmap(gf_gem_obj);

    if (!virt_addr)
    {
        return ERR_PTR(-ENOMEM);
    }

    return virt_addr;
}

void gf_gem_prime_vunmap(struct drm_gem_object *gem_obj, void *vaddr)
{
    struct drm_gf_gem_object *gf_gem_obj= to_gf_bo(gem_obj);

    gf_gem_object_vunmap(gf_gem_obj);
}
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(5,11,0)
static int gf_gem_object_vmap_wrapper(struct drm_gem_object *gem, struct DMABUF_PREFIX(map) *map)
{
    struct drm_gf_gem_object *obj= to_gf_bo(gem);
    void *virt_addr = NULL;

    virt_addr = gf_gem_object_vmap(obj);

    if (!virt_addr)
        return -ENOMEM;

    if (obj->vmap_mem_type == GF_SYSTEM_IO)
    {
        DMABUF_PREFIX(map_set_vaddr_iomem)(map, (void __iomem *)virt_addr);
    }
    else if (obj->vmap_mem_type == GF_SYSTEM_RAM)
    {
        DMABUF_PREFIX(map_set_vaddr)(map, virt_addr);
    }
    else
    {
        return -ENODEV;
    }

    return 0;
}
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(5,11,0)
static int gf_gem_dmabuf_vmap(struct dma_buf *dma_buf, struct DMABUF_PREFIX(map) *map)
#else
static void *gf_gem_dmabuf_vmap(struct dma_buf *dma_buf)
#endif
{
#if DRM_VERSION_CODE >= KERNEL_VERSION(5,11,0)
    struct drm_gem_object *obj = dma_buf->priv;
    int ret;

    if(!obj->funcs->vmap)
        return -EOPNOTSUPP;

    ret = obj->funcs->vmap(obj, map);
    if(ret)
        return ret;
    else if(DMABUF_PREFIX(map_is_null)(map))
        return -ENOMEM;

    return 0;
#else
    struct drm_gf_gem_object *obj= dmabuf_to_gf_bo(dma_buf);

    return gf_gem_object_vmap(obj);
#endif
}

void gf_gem_object_vunmap(struct drm_gf_gem_object *obj)
{
    gf_card_t *gf = obj->base.dev->dev_private;

    mutex_lock(&obj->mm.lock);
    if (obj->krnl_vma && --obj->krnl_vma->ref_cnt == 0)
    {
        switch(obj->krnl_vma->flags.mem_type)
        {
        case GF_SYSTEM_IO:
            gf_unmap_io_memory(obj->krnl_vma);
            break;
        case GF_SYSTEM_RAM:
            gf_unmap_pages_memory(obj->krnl_vma);
            break;
        default:
            gf_assert(0, GF_FUNC_NAME(__func__));
            break;
        }
        obj->krnl_vma = NULL;

        gf_core_interface->mark_pagable(gf->adapter, obj->core_handle);
    }
    mutex_unlock(&obj->mm.lock);
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5,11,0)
static void gf_gem_object_vunmap_wrapper(struct drm_gem_object *gem, struct DMABUF_PREFIX(map) *map)
{
    struct drm_gf_gem_object *obj= to_gf_bo(gem);

    gf_gem_object_vunmap(obj);

    DMA_BUF_MAP_CLEAR();
}
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(5,11,0)
static void gf_gem_dmabuf_vunmap(struct dma_buf *dma_buf, struct DMABUF_PREFIX(map) *map)
#else
static void gf_gem_dmabuf_vunmap(struct dma_buf *dma_buf, void *vaddr)
#endif
{
#if DRM_VERSION_CODE >= KERNEL_VERSION(5,11,0)
    struct drm_gem_object *obj = dma_buf->priv;

    if(!obj->funcs->vunmap)
    {
        gf_error("drm gem object funcs vunmap not register.\n");
        DMA_BUF_MAP_CLEAR();
        return;
    }
    else
    {
        obj->funcs->vunmap(obj, map);
    }
#else
    struct drm_gf_gem_object *obj = dmabuf_to_gf_bo(dma_buf);

    gf_gem_object_vunmap(obj);
#endif
}

static void *gf_gem_dmabuf_kmap_atomic(struct dma_buf *dma_buf, unsigned long page_num)
{
    gf_info("kmap_atomic: not support yet.\n");
    return NULL;
}

static void gf_gem_dmabuf_kunmap_atomic(struct dma_buf *dma_buf, unsigned long page_num, void *addr)
{
    gf_info("kunmap_atomic: not support yet.\n");
}

static void *gf_gem_dmabuf_kmap(struct dma_buf *dma_buf, unsigned long page_num)
{
    gf_info("kmap: not support yet.\n");
    return NULL;
}

static void gf_gem_dmabuf_kunmap(struct dma_buf *dma_buf, unsigned long page_num, void *addr)
{
    gf_info("kunmap: not support yet.\n");
}

static int gf_gem_dmabuf_mmap(struct dma_buf *dma_buf, struct vm_area_struct *vma)
{
    struct drm_gf_gem_object *obj = dmabuf_to_gf_bo(dma_buf);
    gf_card_t *gf = obj->base.dev->dev_private;
    gf_map_argu_t map_argu = {0, };
    int ret = 0;

    gf_info("gem_dmabuf_mmap(%p)\n", dma_buf);

    mutex_lock(&obj->mm.lock);

    map_argu.flags.mem_space = GF_MEM_USER;
    map_argu.flags.read_only = false;
    gf_core_interface->get_map_allocation_info(gf->adapter, obj->core_handle, &map_argu);

    gf_assert(map_argu.flags.mem_space == GF_MEM_USER, GF_FUNC_NAME(__func__));

    switch(map_argu.flags.mem_type)
    {
    case GF_SYSTEM_IO:
        vma->vm_pgoff = map_argu.phys_addr >> PAGE_SHIFT;
        ret = gf_map_system_io(vma, &map_argu);
        break;
    case GF_SYSTEM_RAM:
        {
            int start_page = _ALIGN_DOWN(map_argu.offset, PAGE_SIZE)/PAGE_SIZE;
            int end_page = start_page + ALIGN(map_argu.size, PAGE_SIZE) / PAGE_SIZE;
            map_argu.flags.cache_type = gf_validate_page_cache(map_argu.memory, start_page, end_page, map_argu.flags.cache_type);
            ret = gf_map_system_ram(vma, &map_argu);
            break;
        }
    default:
        gf_assert(0, GF_FUNC_NAME(__func__));
        break;
    }

    mutex_unlock(&obj->mm.lock);
    return ret;
}


#if DRM_VERSION_CODE < KERNEL_VERSION(4,6,0) || (defined(YHQILIN) && DRM_VERSION_CODE == KERNEL_VERSION(4,9,0))
static int gf_gem_begin_cpu_access(struct dma_buf *dma_buf, size_t s1, size_t s2, enum dma_data_direction direction)
#else
static int gf_gem_begin_cpu_access(struct dma_buf *dma_buf, enum dma_data_direction direction)
#endif
{
    struct drm_gf_gem_object *obj= dmabuf_to_gf_bo(dma_buf);
    int write = (direction == DMA_BIDIRECTIONAL || direction == DMA_TO_DEVICE);

    gf_gem_object_begin_cpu_access(obj, 10 * HZ, write);

    return 0;
}

#if defined PHYTIUM_2000
#if DRM_VERSION_CODE < KERNEL_VERSION(4,6,0)
static int gf_gem_end_cpu_access(struct dma_buf *dma_buf, size_t s1, size_t s2, enum dma_data_direction direction)
#else
static int gf_gem_end_cpu_access(struct dma_buf *dma_buf, enum dma_data_direction direction)
#endif
#elif (DRM_VERSION_CODE < KERNEL_VERSION(4,6,0)  || (defined(YHQILIN) && (DRM_SUBVERSION_CODE < 20200710)))
static void gf_gem_end_cpu_access(struct dma_buf *dma_buf, size_t s1, size_t s2, enum dma_data_direction direction)
#elif defined(YHQILIN) && DRM_VERSION_CODE == KERNEL_VERSION(4,9,0)
static int gf_gem_end_cpu_access(struct dma_buf *dma_buf, size_t s1, size_t s2, enum dma_data_direction direction)
#else
static int gf_gem_end_cpu_access(struct dma_buf *dma_buf, enum dma_data_direction direction)
#endif
{
    struct drm_gf_gem_object *obj = dmabuf_to_gf_bo(dma_buf);
    int write = (direction == DMA_BIDIRECTIONAL || direction == DMA_TO_DEVICE);

    gf_gem_object_end_cpu_access(obj, write);
#if (DRM_VERSION_CODE >= KERNEL_VERSION(4,6,0) && !defined(YHQILIN)) || defined(PHYTIUM_2000)
    return 0;
#elif defined(YHQILIN) && DRM_VERSION_CODE == KERNEL_VERSION(4,9,0) && (DRM_SUBVERSION_CODE >= 20200710)
    return 0;
#endif
}

static struct os_pages_memory* gf_allocate_pages_memory_from_sg(struct sg_table *st, int size)
{
    int page_cnt = ALIGN(size, PAGE_SIZE) / PAGE_SIZE;
    struct os_pages_memory *memory = NULL;

    memory = gf_allocate_pages_memory_struct(page_cnt, st);
    if (!memory)
        return NULL;

    memory->shared              = TRUE;
    memory->size                = ALIGN(size, PAGE_SIZE);
    memory->need_flush          = TRUE;
    memory->need_zero           = FALSE;
    memory->fixed_page          = TRUE;
    memory->page_size           = PAGE_SIZE;
    memory->dma32               = FALSE;
    memory->page_4K_64K         = FALSE;
    memory->st                  = st;
    memory->has_dma_map         = TRUE;
    gf_pages_memory_extract_st(memory);
    return memory;
}

static int gf_gem_object_get_pages_generic(struct drm_gf_gem_object *obj)
{
    struct os_pages_memory *memory = NULL;
    gf_card_t *gf = obj->base.dev->dev_private;

    memory = gf_core_interface->get_allocation_pages(gf->adapter, obj->core_handle);
    if (memory && memory->st)
        obj->mm.pages = memory->st;
    else
        obj->mm.pages = NULL;

    return 0;
}

static void gf_gem_object_put_pages_generic(struct drm_gf_gem_object *obj, struct sg_table *pages)
{
}

static void gf_gem_object_release_generic(struct drm_gf_gem_object *obj)
{
    gf_destroy_allocation_t destroy = {0, };
    gf_card_t *gf = obj->base.dev->dev_private;

    destroy.device = 0;
    destroy.allocation = obj->core_handle;
    if (gf->a_info.debugfs_mask & GF_DEBUGFS_GEM_ENABLE)
    {
        gf_gem_debugfs_remove_object(obj);
    }
    gf_core_interface->destroy_allocation(gf->adapter, &destroy);
    obj->core_handle = 0;
}


static const struct drm_gf_gem_object_ops gf_gem_object_generic_ops =
{
    .get_pages = gf_gem_object_get_pages_generic,
    .put_pages = gf_gem_object_put_pages_generic,
    .release = gf_gem_object_release_generic,
};

static int gf_gem_dmabuf_open(struct drm_gem_object *gem_obj, struct drm_file *file)
{
    struct dma_buf_attachment *attach = gem_obj->import_attach;
    struct dma_buf *dma_buf = attach->dmabuf;
    struct drm_gf_gem_object *obj = to_gf_bo(gem_obj);
    struct os_pages_memory *pages = NULL;
    gf_file_t *priv = file->driver_priv;
    gf_card_t *gf = priv->card;
    gf_create_allocation_t create = {0, };
    gf_query_info_t query = {0, };
    int ret = 0;

    if (obj->core_handle)
        return 0;

    gf_assert(priv->gpu_device, GF_FUNC_NAME(__func__));

    ret = gf_gem_object_pin_pages(obj);
    if (ret)
        return ret;

    pages = gf_allocate_pages_memory_from_sg(obj->mm.pages, gem_obj->size);
    if (!pages)
        goto fail_unpin;

    create.device       = priv->gpu_device;
    create.width        = gem_obj->size;
    create.height       = 1;
    create.usage_mask   = GF_USAGE_TEMP_BUFFER;
    create.format       = GF_FORMAT_A8_UNORM;
    create.access_hint  = GF_ACCESS_CPU_ALMOST;

    gf_core_interface->create_allocation_from_pages(priv->card->adapter, &create, pages, obj);
    gf_assert(create.allocation != 0, GF_FUNC_NAME(__func__));
    obj->core_handle = create.allocation;

    if (!obj->core_handle)
    {
        ret = -ENOMEM;
        goto fail_unpin;
    }

    query.type = GF_QUERY_ALLOCATION_INFO_KMD;
    query.argu = create.allocation;
    query.buf = &obj->info;
    gf_core_interface->query_info(priv->card->adapter, &query);

    if (gf->a_info.debugfs_mask & GF_DEBUGFS_GEM_ENABLE)
    {
        obj->debug.parent_gem = obj;
        obj->debug.root       = gf_debugfs_get_allocation_root(gf->debugfs_dev);
        obj->debug.parent_dev = &priv->debug;
        obj->debug.is_dma_buf_import = true;
        gf_gem_debugfs_add_object(obj);
    }

    return ret;

fail_unpin:
    if (pages)
        gf_free_pages_memory(gf->pdev, pages);

    gf_gem_object_unpin_pages(obj);
    return ret;
}

int gf_gem_open(struct drm_gem_object *gem_obj, struct drm_file *file)
{
    if (gem_obj->import_attach)
        return gf_gem_dmabuf_open(gem_obj, file);

    return 0;
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5,11,0)
static const struct drm_gem_object_funcs gf_gem_object_funcs =
{
    //.close         = ,
    .open          = gf_gem_open,
    .free          = gf_gem_free_object,
    //mmp is still in drm_driver
    //.mmp
    .vm_ops        = &gf_gem_vm_ops,
    //.pin           = ,
    //.unpin         = ,
    .vmap          = gf_gem_object_vmap_wrapper,
    .vunmap        = gf_gem_object_vunmap_wrapper,
    .export        = gf_gem_prime_export,
    //.get_set_table = ,
};
#endif

static int gf_drm_gem_object_mmap_immediate(struct vm_area_struct* vma, gf_map_argu_t *map_argu)
{
    int ret = 0;

    switch(map_argu->flags.mem_type)
    {
    case GF_SYSTEM_IO:
        vma->vm_pgoff = map_argu->phys_addr >> PAGE_SHIFT;
        ret = gf_map_system_io(vma, map_argu);
        break;
    case GF_SYSTEM_RAM:
        {
            int start_page = _ALIGN_DOWN(map_argu->offset, PAGE_SIZE)/PAGE_SIZE;
            int end_page = start_page + ALIGN(map_argu->size, PAGE_SIZE) / PAGE_SIZE;
            map_argu->flags.cache_type = gf_validate_page_cache(map_argu->memory, start_page, end_page, map_argu->flags.cache_type);
            ret = gf_map_system_ram(vma, map_argu);
            break;
        }
    default:
        gf_assert(0, GF_FUNC_NAME(__func__));
        break;
    }

    return ret;
}

static void gf_drm_gem_object_mmap_delay(struct vm_area_struct* vma, gf_map_argu_t *map_argu)
{
    unsigned int  cache_type;

    switch(map_argu->flags.mem_type)
    {
    case GF_SYSTEM_IO:
        cache_type = map_argu->flags.cache_type;
#if LINUX_VERSION_CODE >= 0x020612
        vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);
#endif
        vma->vm_page_prot = os_get_pgprot_val(&cache_type, vma->vm_page_prot, 1);
        map_argu->flags.cache_type = cache_type;

        break;
    case GF_SYSTEM_RAM:
        // actually map 0 pages here, just force update cache_type
        map_argu->flags.cache_type = gf_validate_page_cache(map_argu->memory, 0, 0, map_argu->flags.cache_type);
        cache_type = map_argu->flags.cache_type;

#if LINUX_VERSION_CODE >= 0x020612
        vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);
#endif
        vma->vm_page_prot = os_get_pgprot_val(&cache_type, vma->vm_page_prot, 0);

        break;
    default:
        gf_assert(0, GF_FUNC_NAME(__func__));
        break;
    }
}

static int gf_drm_gem_object_mmap(struct file *filp, struct drm_gf_gem_object *obj, struct vm_area_struct *vma)
{
    struct drm_file *file = filp->private_data;
    gf_file_t     *priv = file->driver_priv;
    gf_map_argu_t map_argu = {0, };
    gf_wait_allocation_idle_t wait = {0, };
    int ret = 0;

    mutex_lock(&obj->mm.lock);

    map_argu.flags.mem_space = GF_MEM_USER;
    map_argu.flags.read_only = false;
    gf_core_interface->get_map_allocation_info(priv->card->adapter, obj->core_handle, &map_argu);

    wait.engine_mask = ALL_ENGINE_MASK;
    wait.hAllocation = obj->core_handle;
    gf_core_interface->wait_allocation_idle(priv->card->adapter, &wait);

#if DRM_VERSION_CODE < KERNEL_VERSION(6,3,0)
    vma->vm_flags |= (VM_IO | VM_PFNMAP | VM_DONTEXPAND | VM_DONTDUMP);
#else
    vm_flags_set(vma, (VM_IO | VM_PFNMAP | VM_DONTEXPAND | VM_DONTDUMP));
#endif

    if ((map_argu.flags.mem_type == GF_SYSTEM_RAM) &&
        ((obj->info.segment_id) != 2 && (obj->info.segment_id != 3)))
    {
        gf_error("drm_gem_object_mmap failed: mem_type is ram but segment_id is %d.\n", obj->info.segment_id);
        gf_assert(0, GF_FUNC_NAME(__func__));
    }

#if  defined (__mips64__) || defined(__loongarch__)
    if (map_argu.flags.mem_type == GF_SYSTEM_IO)
    {
        map_argu.flags.cache_type = GF_MEM_UNCACHED;
    }
#endif

    if (obj->delay_map == 0)
        ret = gf_drm_gem_object_mmap_immediate(vma, &map_argu);
    else
        gf_drm_gem_object_mmap_delay(vma, &map_argu);

    obj->map_argu = map_argu;

    mutex_unlock(&obj->mm.lock);

    return ret;
}


static void gf_drm_gem_object_vm_prepare(struct drm_gf_gem_object *obj)
{
    gf_card_t *gf = obj->base.dev->dev_private;

    gf_core_interface->prepare_and_mark_unpagable(gf->adapter, obj->core_handle, &(obj->info));
}

static void gf_drm_gem_object_vm_release(struct drm_gf_gem_object *obj)
{
    gf_card_t *gf = obj->base.dev->dev_private;

    gf_core_interface->mark_pagable(gf->adapter, obj->core_handle);
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5,11,0)
static void gf_gem_object_init(struct drm_gf_gem_object *obj, const struct drm_gf_gem_object_ops *ops, const struct drm_gem_object_funcs *funcs)
#else
static void gf_gem_object_init(struct drm_gf_gem_object *obj, const struct drm_gf_gem_object_ops *ops)
#endif
{
    mutex_init(&obj->mm.lock);
    obj->ops = ops;

    reservation_object_init(&obj->__builtin_resv);
    obj->resv = &obj->__builtin_resv;

#if DRM_VERSION_CODE >= KERNEL_VERSION(5,11,0)
    obj->base.funcs = funcs;
#endif
}

static int gf_get_pages_from_userptr(struct device *dev, unsigned long userptr, unsigned int size, struct os_pages_memory **mem)
{
    struct os_pages_memory *memory = NULL;
    struct page **pages = NULL;
    int page_num  = PAGE_ALIGN(size)/PAGE_SIZE;
    int index = 0;
    int pinned = 0;
    int result = 0;

    pages = vzalloc(page_num * sizeof(struct page*));

#if DRM_VERSION_CODE < KERNEL_VERSION(5,8,0)
    down_read(&current->mm->mmap_sem);
#else
    down_read(&current->mm->mmap_lock);
#endif
    while (pinned < page_num)
    {

// For CentOS old kernel + new drm backport, we will include drm_backport.h globally,
// DRM_BACKPORT_H_ is defined in drm_backport.h, and get_user_pages() is also redefined
// in drm_backport.h to be used by backported new drm drivers, so use DRM_VERSION_CODE
// to check which get_user_pages() to call.
// If the DRM_BACKPORT_H_ is not defined, should always use LINUX_VERSION_CODE even if
// the drm is backported, since the get_user_pages() is defined by kernel, not by drm.
#ifdef DRM_BACKPORT_H_
#define GET_USER_PAGES_VERSION_CODE DRM_VERSION_CODE
#else
#define GET_USER_PAGES_VERSION_CODE LINUX_VERSION_CODE
#endif

#if GET_USER_PAGES_VERSION_CODE >= KERNEL_VERSION(6, 5, 0)
        result = get_user_pages(userptr + (pinned * PAGE_SIZE),
                                page_num - pinned,
                                FOLL_WRITE,
                                &pages[pinned]);
#elif GET_USER_PAGES_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
        result = get_user_pages(userptr + (pinned * PAGE_SIZE),
                                page_num - pinned,
                                FOLL_WRITE,
                                &pages[pinned],
                                NULL);
#elif GET_USER_PAGES_VERSION_CODE >= KERNEL_VERSION(4, 6, 0)
        result = get_user_pages(userptr + (pinned * PAGE_SIZE),
                                page_num - pinned,
                                FOLL_WRITE,
                                0,
                                &pages[pinned],
                                NULL);
#elif GET_USER_PAGES_VERSION_CODE >= KERNEL_VERSION(4, 4, 168) && GET_USER_PAGES_VERSION_CODE < KERNEL_VERSION(4, 5, 0)
        result = get_user_pages(current,
                                current->mm,
                                userptr + (pinned * PAGE_SIZE),
                                page_num - pinned,
                                FOLL_WRITE,
                                &pages[pinned],
                                NULL);
#else
        result = get_user_pages(current,
                                current->mm,
                                userptr + (pinned * PAGE_SIZE),
                                page_num - pinned,
                                1,
                                0,
                                &pages[pinned],
                                NULL);
#endif
        if (result < 0)
        {
            goto release_pages;
        }
        pinned += result;
    }
#if DRM_VERSION_CODE < KERNEL_VERSION(5,8,0)
    up_read(&current->mm->mmap_sem);
#else
    up_read(&current->mm->mmap_lock);
#endif

    memory = gf_allocate_pages_memory_struct(page_num, NULL);

    memory->size        = PAGE_ALIGN(size); // supose size==PAGE_ALIGN(size)
    memory->page_size   = PAGE_SIZE;
    memory->need_flush  = TRUE;
    memory->fixed_page  = TRUE;
    memory->need_zero   = FALSE;
    memory->page_4K_64K = FALSE;
    memory->noswap      = FALSE;
    memory->dma32       = FALSE;
    memory->shared      = FALSE;
    memory->userptr     = TRUE;
    sg_alloc_table_from_pages(memory->st, pages, page_num, 0, PAGE_ALIGN(size), GFP_KERNEL);
    gf_pages_memory_extract_st(memory);

    memory->st->nents = dma_map_sg(dev, memory->st->sgl, memory->st->orig_nents, DMA_BIDIRECTIONAL);
    if (memory->st->nents) {
        memory->has_dma_map = TRUE;
    } else {
        gf_assert(0, "dma_map_sg failed.\n");
    }

    *mem = memory;

    vfree(pages);

    return S_OK;


release_pages:
    for(index = 0; index < pinned; index++)
    {
        put_page(pages[index]);
    }
#if DRM_VERSION_CODE < KERNEL_VERSION(5,8,0)
    up_read(&current->mm->mmap_sem);
#else
    up_read(&current->mm->mmap_lock);
#endif
    vfree(pages);

    return E_FAIL;
}


struct drm_gf_gem_object* gf_drm_gem_create_object(gf_card_t *gf, gf_create_allocation_t *create, gf_device_debug_info_t **ddbg)
{
    struct drm_gf_gem_object *obj = NULL;
    gf_query_info_t query = {0, };
    int result = 0;

    obj = gf_calloc(sizeof(*obj));

    if(create->user_ptr)
    {
        struct os_pages_memory *memory=NULL;

        result = gf_get_pages_from_userptr(&(gf->pdev->dev), (unsigned long)create->user_ptr, create->user_buf_size, &memory);
        if(result < 0)
        {
            gf_free(obj);
            return NULL;
        }

        gf_core_interface->create_allocation_from_pages(gf->adapter, create, memory, obj);
    }
    else
    {
        gf_core_interface->create_allocation(gf->adapter, create, obj);
    }

    if(!create->allocation)
    {
        gf_free(obj);
        return NULL;
    }

    query.type = GF_QUERY_ALLOCATION_INFO_KMD;
    query.argu = create->allocation;
    query.buf = &obj->info;
    gf_core_interface->query_info(gf->adapter, &query);
    obj->core_handle = create->allocation;

    drm_gem_private_object_init(pci_get_drvdata(gf->pdev), &obj->base, create->size);

#if DRM_VERSION_CODE >= KERNEL_VERSION(5,11,0)
    gf_gem_object_init(obj, &gf_gem_object_generic_ops, &gf_gem_object_funcs);
#else
    gf_gem_object_init(obj, &gf_gem_object_generic_ops);
#endif

    if (gf->a_info.debugfs_mask & GF_DEBUGFS_GEM_ENABLE)
    {
        obj->debug.parent_dev = ddbg;
        obj->debug.parent_gem = obj;
        obj->debug.root       = gf_debugfs_get_allocation_root(gf->debugfs_dev);
        obj->debug.is_dma_buf_import = false;

        gf_gem_debugfs_add_object(obj);
    }
    trace_gfx_drm_gem_create_object(gf->index, obj);
    return obj;
}

int gf_drm_gem_create_object_ioctl(struct drm_file *file, gf_create_allocation_t *create)
{
    int ret = 0;
    struct drm_gf_gem_object *obj = NULL;
    gf_file_t *priv = file->driver_priv;
    gf_card_t *gf  = priv->card;

    gf_assert(create->device == priv->gpu_device, GF_FUNC_NAME(__func__));

    obj = gf_drm_gem_create_object(gf, create, &priv->debug);

    if (!obj)
        return -ENOMEM;

    ret = drm_gem_handle_create(file, &obj->base, &create->allocation);

    gf_assert(ret == 0, GF_FUNC_NAME(__func__));

    gf_gem_object_put(obj);

    return 0;
}

int gf_drm_gem_create_resource_ioctl(struct drm_file *file, gf_create_resource_t *create)
{
    int ret = 0, i;
    gf_file_t *priv = file->driver_priv;
    gf_card_t *gf  = priv->card;
    int obj_count = create->NumAllocations;
    gf_create_alloc_info_t stack_kinfos[8];
    gf_create_alloc_info_t *kinfo = stack_kinfos;
    gf_create_alloc_info_t __user *uinfo = ptr64_to_ptr(create->pAllocationInfo);
    struct drm_gf_gem_object *stack_objs[8];
    struct drm_gf_gem_object **objs = stack_objs;
    gf_query_info_t query = {0, };

    gf_assert(create->device == priv->gpu_device, GF_FUNC_NAME(__func__));
    gf_assert(uinfo != NULL, GF_FUNC_NAME(__func__));

    if (obj_count > sizeof(stack_kinfos) / sizeof(stack_kinfos[0]))
    {
        kinfo = gf_malloc(obj_count * sizeof(gf_create_alloc_info_t));
    }
    create->pAllocationInfo = ptr_to_ptr64(kinfo);

    if (obj_count > sizeof(stack_objs) / sizeof(stack_objs[0]))
    {
        objs = gf_malloc(obj_count * sizeof(*objs));
    }

    gf_copy_from_user(kinfo, uinfo, obj_count * sizeof(gf_create_alloc_info_t));

    for (i = 0; i < obj_count; i++)
    {
        objs[i] = gf_calloc(sizeof(**objs));
#if DRM_VERSION_CODE >= KERNEL_VERSION(5,11,0)
        gf_gem_object_init(objs[i], &gf_gem_object_generic_ops, &gf_gem_object_funcs);
#else
        gf_gem_object_init(objs[i], &gf_gem_object_generic_ops);
#endif
    }

    ret = gf_core_interface->create_allocation_list(gf->adapter, create, (void**)objs);
    gf_assert(ret == 0, GF_FUNC_NAME(__func__));

    create->pAllocationInfo = ptr_to_ptr64(uinfo);
    if (ret)
        goto failed_free;

    for (i = 0; i < obj_count; i++)
    {
        objs[i]->core_handle = kinfo[i].hAllocation;
        gf_assert(kinfo[i].hAllocation, GF_FUNC_NAME(__func__));

        query.type = GF_QUERY_ALLOCATION_INFO_KMD;
        query.argu = kinfo[i].hAllocation;
        query.buf = &objs[i]->info;
        gf_core_interface->query_info(gf->adapter, &query);

        drm_gem_private_object_init(pci_get_drvdata(gf->pdev), &objs[i]->base, kinfo[i].Size);
        ret = drm_gem_handle_create(file, &objs[i]->base, &kinfo[i].hAllocation);
        gf_assert(0 == ret, GF_FUNC_NAME(__func__));

        if (gf->a_info.debugfs_mask & GF_DEBUGFS_GEM_ENABLE)
        {
            objs[i]->debug.parent_dev = &priv->debug;
            objs[i]->debug.parent_gem = objs[i];
            objs[i]->debug.root       = gf_debugfs_get_allocation_root(gf->debugfs_dev);
            objs[i]->debug.is_dma_buf_import = false;

            gf_gem_debugfs_add_object(objs[i]);
        }
        trace_gfx_drm_gem_create_object(gf->index, objs[i]);
        gf_gem_object_put(objs[i]);
    }

    gf_copy_to_user(uinfo, kinfo, obj_count * sizeof(gf_create_alloc_info_t));

failed_free:
    if (kinfo != stack_kinfos)
    {
        gf_free(kinfo);
    }
    if (objs != stack_objs)
    {
        gf_free(objs);
    }

    return ret;
}


// DMABUF
static int gf_gem_object_get_pages_dmabuf(struct drm_gf_gem_object *obj)
{
    struct sg_table *pages;
    pages = dma_buf_map_attachment(obj->base.import_attach, DMA_BIDIRECTIONAL);
    if (IS_ERR(pages))
        return PTR_ERR(pages);

    obj->mm.pages = pages;
    return 0;
}

static void gf_gem_object_put_pages_dmabuf(struct drm_gf_gem_object *obj, struct sg_table *pages)
{
    gf_assert(pages != NULL, GF_FUNC_NAME(__func__));
    dma_buf_unmap_attachment(obj->base.import_attach, pages, DMA_BIDIRECTIONAL);
}

static void gf_gem_object_release_dmabuf(struct drm_gf_gem_object *obj)
{
    gf_destroy_allocation_t destroy = {0, };
    gf_card_t *gf = obj->base.dev->dev_private;
    gf_wait_allocation_idle_t wait = {0, };
    struct dma_buf_attachment *attach;
    struct dma_buf *dma_buf;

    if (obj->core_handle)
    {
        wait.engine_mask = ALL_ENGINE_MASK;
        wait.hAllocation = obj->core_handle;
        gf_core_interface->wait_allocation_idle(gf->adapter, &wait);

        destroy.device = 0;
        destroy.allocation = obj->core_handle;
        gf_core_interface->destroy_allocation(gf->adapter, &destroy);
        obj->core_handle = 0;

        if (gf->a_info.debugfs_mask & GF_DEBUGFS_GEM_ENABLE)
        {
            gf_gem_debugfs_remove_object(obj);
        }

        gf_gem_object_unpin_pages(obj);
    }

    attach = obj->base.import_attach;
    dma_buf = attach->dmabuf;
    dma_buf_detach(dma_buf, attach);
    dma_buf_put(dma_buf);
}

static const struct drm_gf_gem_object_ops gf_gem_object_dmabuf_ops =
{
    .get_pages = gf_gem_object_get_pages_dmabuf,
    .put_pages = gf_gem_object_put_pages_dmabuf,
    .release = gf_gem_object_release_dmabuf,
};


#if DRM_VERSION_CODE < KERNEL_VERSION(6,6,0)
int gf_gem_prime_fd_to_handle(struct drm_device *dev, struct drm_file *file_priv, int prime_fd, uint32_t *handle)
{
    int ret;
    struct drm_gf_driver *driver = to_drm_gf_driver(dev->driver);

    mutex_lock(&driver->lock);
    gf_assert(!driver->file_priv, GF_FUNC_NAME(__func__));

    driver->file_priv = file_priv;
    ret = drm_gem_prime_fd_to_handle(dev, file_priv, prime_fd, handle);
    driver->file_priv = NULL;
    mutex_unlock(&driver->lock);

    return ret;
}
#endif

signed long gf_gem_object_begin_cpu_access(struct drm_gf_gem_object *obj, long timeout, int write)
{
    gf_card_t *gf = obj->base.dev->dev_private;
    trace_gfx_gem_object_begin_cpu_access(gf->index, obj, timeout, write);

    return gf_gem_fence_await_reservation(obj->resv, 0, timeout, write);
}

int gf_gem_object_begin_cpu_access_ioctl(struct drm_file *file, gf_drm_gem_begin_cpu_access_t *args)
{
    int ret = 0;
    struct drm_device *dev = file->minor->dev;
    struct drm_gf_gem_object *obj;

    obj = gf_drm_gem_object_lookup(dev, file, args->handle);

    if (!obj)
        return -ENOENT;

    ret = gf_gem_object_begin_cpu_access(obj, 10 * HZ, !args->readonly);

    gf_gem_object_put(obj);
    return 0;
}

void gf_gem_object_end_cpu_access(struct drm_gf_gem_object *obj, int write)
{
    gf_card_t *gf = obj->base.dev->dev_private;
    gf_map_argu_t map_argu = {0, };

    map_argu.flags.mem_space = GF_MEM_KERNEL;
    map_argu.flags.read_only = false;
    gf_core_interface->get_map_allocation_info(gf->adapter, obj->core_handle, &map_argu);

    if (map_argu.flags.mem_type == GF_SYSTEM_RAM &&
        map_argu.flags.cache_type == GF_MEM_WRITE_BACK &&
        (!obj->info.snoop) &&
        map_argu.memory)
    {
        gf_flush_cache(gf->pdev, NULL, map_argu.memory, map_argu.offset, map_argu.size);
    }

    if (map_argu.flags.cache_type == GF_MEM_WRITE_COMBINED)
    {
        gf_wmb();
    }

    trace_gfx_gem_object_end_cpu_access(gf->index, obj);

}

void gf_gem_object_end_cpu_access_ioctl(struct drm_file *file, gf_drm_gem_end_cpu_access_t *args)
{
    struct drm_device *dev = file->minor->dev;
    struct drm_gf_gem_object *obj;

    obj = gf_drm_gem_object_lookup(dev, file, args->handle);

    if (!obj)
        return;

    gf_gem_object_end_cpu_access(obj, 1);
    gf_gem_object_put(obj);
}

const struct dma_buf_ops CONCAT(gf_dmabuf_ops,DRIVER_NAME) =
{
    .map_dma_buf    = gf_gem_map_dma_buf,
    .unmap_dma_buf  = gf_gem_unmap_dma_buf,
    .release        = drm_gem_dmabuf_release,
#if DRM_VERSION_CODE < KERNEL_VERSION(5,6,0)
#if DRM_VERSION_CODE > KERNEL_VERSION(4,11,0)
    .map           = gf_gem_dmabuf_kmap,
    .unmap         = gf_gem_dmabuf_kunmap,
#if DRM_VERSION_CODE < KERNEL_VERSION(4,19,0)
    .map_atomic    = gf_gem_dmabuf_kmap_atomic,
    .unmap_atomic  = gf_gem_dmabuf_kunmap_atomic,
#endif
#else
    .kmap            = gf_gem_dmabuf_kmap,
    .kmap_atomic     = gf_gem_dmabuf_kmap_atomic,
    .kunmap          = gf_gem_dmabuf_kunmap,
    .kunmap_atomic   = gf_gem_dmabuf_kunmap_atomic,
#endif
#endif
    .mmap           = gf_gem_dmabuf_mmap,
    .vmap           = gf_gem_dmabuf_vmap,
    .vunmap         = gf_gem_dmabuf_vunmap,
    .begin_cpu_access   = gf_gem_begin_cpu_access,
    .end_cpu_access     = gf_gem_end_cpu_access,
};

EXPORT_SYMBOL(CONCAT(gf_dmabuf_ops,DRIVER_NAME));

struct drm_gem_object *gf_gem_prime_import(struct drm_device *dev, struct dma_buf *dma_buf)
{
    struct drm_gf_driver *driver = to_drm_gf_driver(dev->driver);
    gf_card_t *gf = dev->dev_private;
    struct drm_gf_gem_object *obj = NULL;
    struct dma_buf_attachment *attach;

    if (dma_buf->ops == &CONCAT(gf_dmabuf_ops, DRIVER_NAME))
    {
        obj = dmabuf_to_gf_bo(dma_buf);

        if (obj->base.dev == dev)
        {
            trace_gfx_gem_prime_import(gf->index, dma_buf, obj);
            return &(gf_gem_object_get(obj)->base);
        }
    }

    attach = dma_buf_attach(dma_buf, dev->dev);
    if (IS_ERR(attach))
        return ERR_CAST(attach);

    get_dma_buf(dma_buf);
    obj = gf_calloc(sizeof(*obj));
    if (obj == NULL)
        goto fail_detach;

    drm_gem_private_object_init(dev, &obj->base, dma_buf->size);

#if DRM_VERSION_CODE >= KERNEL_VERSION(5,11,0)
    gf_gem_object_init(obj, &gf_gem_object_dmabuf_ops, &gf_gem_object_funcs);
#else
    gf_gem_object_init(obj, &gf_gem_object_dmabuf_ops);
#endif

    obj->base.import_attach = attach;
    obj->resv = dma_buf->resv;

    trace_gfx_drm_gem_create_object(gf->index, obj);
    trace_gfx_gem_prime_import(gf->index, dma_buf, obj);

    return &obj->base;

fail_detach:
    dma_buf_detach(dma_buf, attach);
    dma_buf_put(dma_buf);

    return NULL;
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(5,4,0)
struct dma_buf *gf_gem_prime_export(struct drm_gem_object *gem_obj, int flags)
#else
struct dma_buf *gf_gem_prime_export(struct drm_device *dev, struct drm_gem_object *gem_obj, int flags)
#endif
{
    struct dma_buf *dma_buf;
    struct drm_gf_gem_object *obj;
    struct drm_device  *dev1;
    gf_card_t *gf;
    DEFINE_DMA_BUF_EXPORT_INFO(exp_info);

#if DRM_VERSION_CODE >= KERNEL_VERSION(5,4,0)
    dev1 = gem_obj->dev;
#else
    dev1 = dev;
#endif
    gf = dev1->dev_private;
    obj = to_gf_bo(gem_obj);

    exp_info.ops = &CONCAT(gf_dmabuf_ops,DRIVER_NAME);
    exp_info.size = obj->base.size;
    exp_info.flags = flags;
    exp_info.priv = gem_obj;
    exp_info.resv = obj->resv;

#if DRM_VERSION_CODE < KERNEL_VERSION(4,9,0) && !defined(PHYTIUM_2000)
    dma_buf = dma_buf_export(&exp_info);
#else
    dma_buf = drm_gem_dmabuf_export(dev1, &exp_info);
#endif

    trace_gfx_gem_prime_export(gf->index, dma_buf, obj);

    return dma_buf;
}

int gf_gem_mmap_gtt(struct drm_file *file, struct drm_device *dev, uint32_t handle, uint64_t *offset)
{
    int ret;
    struct drm_gf_gem_object *obj;

    obj = gf_drm_gem_object_lookup(dev, file, handle);
    if (!obj)
    {
        gf_error("gf_gem_mmap_gtt lookup failed \n");
        ret = -ENOENT;
        goto unlock;
    }

    ret = drm_gem_create_mmap_offset(&obj->base);
    if (ret)
    {
        gf_error("gf_gem_mmap_gtt create mmap offset failed \n");
        goto out;
    }
    *offset = drm_vma_node_offset_addr(&obj->base.vma_node);

out:
    gf_gem_object_put(obj);
unlock:
    return ret;
}

int gf_gem_mmap_gtt_ioctl(struct drm_file *file, gf_drm_gem_map_t *args)
{
    int ret;
    struct drm_gf_gem_object *obj;

    obj = gf_drm_gem_object_lookup(file->minor->dev, file, args->gem_handle);
    if (!obj)
    {
        gf_error("gf_gem_mmap_gtt_ioctl lookup failed \n");
        ret = -ENOENT;
        goto unlock;
    }

    obj->delay_map = args->delay_map;
    obj->prefault_num = args->prefault_num;

    ret = drm_gem_create_mmap_offset(&obj->base);
    if (ret)
    {
        gf_error("gf_gem_mmap_gtt_ioctl create mmap offset failed \n");
        goto out;
    }
    args->offset = drm_vma_node_offset_addr(&obj->base.vma_node);

out:
    gf_gem_object_put(obj);
unlock:
    return ret;
}

int gf_drm_gem_mmap(struct file *filp, struct vm_area_struct *vma)
{
    struct drm_gf_gem_object *obj;
    struct drm_file *file = filp->private_data;
    gf_file_t     *priv = file->driver_priv;
    int             ret = 0;

    if (priv->map)
    {
        return gf_mmap(filp, vma);
    }

    ret = drm_gem_mmap(filp, vma);
    if (ret < 0)
    {
        gf_error("failed to mmap, ret = %d.\n", ret);
        return ret;
    }

    obj = to_gf_bo(vma->vm_private_data);

#if DRM_VERSION_CODE < KERNEL_VERSION(6,3,0)
    vma->vm_flags &= ~(VM_IO | VM_PFNMAP | VM_DONTEXPAND | VM_DONTDUMP);
#else
    vm_flags_clear(vma, (VM_IO | VM_PFNMAP | VM_DONTEXPAND | VM_DONTDUMP));
#endif

    vma->vm_page_prot = pgprot_writecombine(vm_get_page_prot(vma->vm_flags));

    gf_drm_gem_object_vm_prepare(obj);

    return gf_drm_gem_object_mmap(filp, obj, vma);
}

int gf_gem_dumb_create(struct drm_file *file, struct drm_device *dev, struct drm_mode_create_dumb *args)
{
    int ret = 0;
    struct drm_gf_gem_object *obj = NULL;
    gf_file_t *priv = file->driver_priv;
    gf_card_t *gf  = priv->card;
    gf_create_allocation_t create = {0, };
    gf_format gfFormat = GF_FORMAT_R8_UNORM;

    if(args->bpp == 32)
    {
        gfFormat = GF_FORMAT_B8G8R8A8_UNORM;
    }
    else if(args->bpp == 16)
    {
        gfFormat = GF_FORMAT_B5G6R5_UNORM;
    }

    create.device   = priv->gpu_device;
    create.width    = args->width;
    create.height   = args->height;
    create.format   = gfFormat;
    create.tiled    = 0;
    create.unpagable    = TRUE;
    create.usage_mask   = GF_USAGE_DISPLAY_SURFACE | GF_USAGE_FRAMEBUFFER;
    create.access_hint  = GF_ACCESS_CPU_ALMOST;
    create.primary      = 1;

    obj = gf_drm_gem_create_object(gf, &create, &priv->debug);
    if (!obj)
        return -ENOMEM;

    ret = drm_gem_handle_create(file, &obj->base, &args->handle);
    gf_assert(ret == 0, GF_FUNC_NAME(__func__));

    args->pitch = create.pitch;
    args->size  = create.size;
    gf_gem_object_put(obj);

    return ret;
}

static vm_fault_t gf_gem_io_insert(struct vm_area_struct *vma, unsigned long address, gf_map_argu_t *map, unsigned int offset, unsigned int prefault_num)
{
    unsigned long pfn;
    vm_fault_t retval = VM_FAULT_NOPAGE;
    int i ;

    for(i = 0; i < prefault_num ; i++)
    {
        if(offset >= map->size)
            break;

        pfn = (map->phys_addr + offset) >> PAGE_SHIFT;

        retval = vmf_insert_pfn(vma, address, pfn);
        if (unlikely((retval == VM_FAULT_NOPAGE && i > 0)))
            break;
        else if (unlikely(retval & VM_FAULT_ERROR))
        {
            return retval;
        }

        offset += PAGE_SIZE;
        address += PAGE_SIZE;
    }

    return retval;
}

static vm_fault_t gf_gem_ram_insert(struct vm_area_struct *vma, unsigned long address, gf_map_argu_t *map, unsigned int offset, unsigned int prefault_num)
{
    unsigned long pfn;
    vm_fault_t retval = VM_FAULT_NOPAGE;
    int i, start_page, end_page;

    start_page = _ALIGN_DOWN(offset, PAGE_SIZE) / PAGE_SIZE;
    end_page = min((int)(start_page+prefault_num), (int)(map->memory->size / PAGE_SIZE));
    map->flags.cache_type = gf_validate_page_cache(map->memory, start_page, end_page, map->flags.cache_type);

    for (i = start_page; i < end_page; i++)
    {
        pfn  = page_to_pfn(map->memory->pages[i]);
        retval = vmf_insert_pfn(vma, address, pfn);
        if (unlikely((retval == VM_FAULT_NOPAGE && i > start_page)))
            break;
        else if (unlikely(retval & VM_FAULT_ERROR))
        {
            return retval;
        }
        address += PAGE_SIZE;
    }
    return retval;
}


#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
static vm_fault_t gf_gem_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
#else
static vm_fault_t gf_gem_fault(struct vm_fault *vmf)
#endif
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,11,0)
    struct vm_area_struct *vma = vmf->vma;
#endif
    struct drm_gf_gem_object *obj;
    unsigned long offset;
    uint64_t vma_node_offset;
    vm_fault_t ret = VM_FAULT_NOPAGE;
    unsigned long address;
    unsigned int prefault_num = 0;
    unsigned int page_num = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0)
    address = vmf->address;
#else
    address = (unsigned long)vmf->virtual_address;
#endif
    obj = to_gf_bo(vma->vm_private_data);

    vma_node_offset = drm_vma_node_offset_addr(&(obj->base.vma_node));
    offset = ((vma->vm_pgoff << PAGE_SHIFT) - vma_node_offset) + (address - vma->vm_start);

    gf_assert(obj->delay_map != 0, GF_FUNC_NAME(__func__));

    page_num = NUM_PAGES(vma->vm_end - address);

    if (obj->prefault_num == 0)
        prefault_num = min((unsigned int)GF_DEFAULT_PREFAULT_NUM, page_num);
    else
        prefault_num = min(obj->prefault_num, page_num);

    switch(obj->map_argu.flags.mem_type)
    {
    case GF_SYSTEM_IO:
        ret = gf_gem_io_insert(vma, address, &obj->map_argu, offset, prefault_num);
        break;
    case GF_SYSTEM_RAM:
        ret = gf_gem_ram_insert(vma, address, &obj->map_argu, offset, prefault_num);
        break;
    default:
        gf_assert(0, GF_FUNC_NAME(__func__));
        break;
    }

    return ret;
}

static void gf_gem_vm_open(struct vm_area_struct *vma)
{
    struct drm_gf_gem_object *obj = to_gf_bo(vma->vm_private_data);

    gf_drm_gem_object_vm_prepare(obj);
    drm_gem_vm_open(vma);
}

static void gf_gem_vm_close(struct vm_area_struct *vma)
{
    struct drm_gf_gem_object *obj = to_gf_bo(vma->vm_private_data);

    gf_drm_gem_object_vm_release(obj);
    drm_gem_vm_close(vma);
}

static int gf_gem_vm_access(struct vm_area_struct *vma, unsigned long addr, void *buf, int len, int write)
{
    void *virtul_addr = NULL;
    struct drm_gf_gem_object *obj;
    unsigned long offset;
    int rest, this_size;
    char *p;

    obj = to_gf_bo(vma->vm_private_data);
    offset = addr - vma->vm_start + ((vma->vm_pgoff - drm_vma_node_start(&obj->base.vma_node)) << PAGE_SHIFT);
    //gf_info("gf_gem_vm_access(addr:%p, len:%d, write:%d, offset:%llx)\n", (void*)addr, len, write, offset);

    if (len < 1 || (offset + len) > obj->base.size)
    {
        gf_info("access overflow (size:%x, vm_start:%llx, vm_pgoff:%llx, node_offset:%llx)\n",
            obj->base.size, vma->vm_start, vma->vm_pgoff, drm_vma_node_start(&obj->base.vma_node));
        return -EIO;
    }

    virtul_addr = gf_gem_object_vmap(obj);
    if (!virtul_addr)
    {
        gf_info("vmap failed.\n");
        return -EIO;
    }

    rest = len;
    p = (char*)buf;

#define __MIN(a,b)  ((a) > (b) ? (b) : (a))

    this_size = __MIN(rest, obj->base.size - offset);
    if (write)
        gf_memcpy(virtul_addr + offset, p, this_size);
    else
        gf_memcpy(p, virtul_addr + offset, this_size);
    rest -= this_size;

    gf_gem_object_vunmap(obj);

    return len - rest;
}

const struct vm_operations_struct gf_gem_vm_ops = {
    .fault = gf_gem_fault,
    .open = gf_gem_vm_open,
    .close = gf_gem_vm_close,
    .access = gf_gem_vm_access,
};

static int gf_gem_debugfs_show_info(struct seq_file *s, void *unused)
{
    struct gf_gem_debug_info *dnode = (struct gf_gem_debug_info *)(s->private);
    struct drm_gf_gem_object *obj = (struct drm_gf_gem_object *) dnode->parent_gem;
    gf_open_allocation_t *info = &obj->info;
/*print the info*/
    seq_printf(s, "device=0x%x\n", info->device);
    seq_printf(s, "handle=0x%x\n", info->allocation);

    seq_printf(s, "width=%d\n", info->width);
    seq_printf(s, "height=%d\n", info->height);
    seq_printf(s, "pitch=%d\n", info->pitch);
    seq_printf(s, "bit_cnt=%d\n", info->bit_cnt);

    seq_printf(s, "secured=%d\n", info->secured);
    seq_printf(s, "size=%d\n", info->size);
    seq_printf(s, "tile=%d\n", info->tiled);
    seq_printf(s, "segmentid=%d\n", info->segment_id);
    seq_printf(s, "hw_format=%d\n", info->hw_format);
    seq_printf(s, "compress_format=%d\n", info->compress_format);
    seq_printf(s, "bl_slot_index=%d\n", info->bl_slot_index);
    seq_printf(s, "gpu_virt_addr=%lld\n", info->gpu_virt_addr);
    seq_printf(s, "imported=%d\n", dnode->is_dma_buf_import);

    return 0;
}

static int gf_gem_debugfs_info_open(struct inode *inode, struct file *file)
{
    return single_open(file, gf_gem_debugfs_show_info, inode->i_private);
}

static const struct file_operations debugfs_gem_info_fops = {
        .open       = gf_gem_debugfs_info_open,
        .read       = seq_read,
        .llseek     = seq_lseek,
        .release    = single_release,
};

static ssize_t gf_gem_debugfs_data_read(struct file *f, char __user *buf,
                     size_t size, loff_t *pos)
{
  //map & read
    gf_gem_debug_info_t *dbg = file_inode(f)->i_private;
    struct drm_gf_gem_object *obj = dbg->parent_gem;
    gf_card_t *gf = obj->base.dev->dev_private;
    void *data = NULL;
    ssize_t result = 0;
    int data_off = 0;
    int copy_size = 0;
    int a_size = 0;

    if (!gf->allocation_trace_tags) return 0;

    data = gf_gem_object_vmap(obj);
    if (data) {
        gf_gem_object_begin_cpu_access(obj, 10 * HZ, 0);
        a_size = obj->krnl_vma->size;
        data_off = *pos;
        copy_size = min(size, (size_t)(a_size - data_off));
        if (copy_to_user(buf,  data + data_off, copy_size)){
            result = -EFAULT;
        } else {
            result += copy_size;
            *pos += copy_size;
        }
        gf_gem_object_end_cpu_access(obj,0);
    }
    else {
        result = -EINVAL;
    }
    gf_gem_object_vunmap(obj);

    return result;
}

static const struct file_operations debugfs_gem_data_fops = {
        .read       = gf_gem_debugfs_data_read,
        .llseek     = default_llseek,
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35)
static ssize_t simple_write_to_buffer(void *to, size_t available, loff_t *ppos,
                    const void __user *from, size_t count)
{
    loff_t pos = *ppos;
    size_t res;

    if (pos < 0)
        return -EINVAL;
    if (pos >= available || !count)
        return 0;
    if (count > available - pos)
        count = available - pos;
    res = copy_from_user(to + pos, from, count);
    if (res == count)
        return -EFAULT;
    count -= res;
    *ppos = pos + count;
    return count;
}
#endif

static ssize_t gf_gem_debugfs_control_write(struct file *f, const char __user *buf,
                                         size_t size, loff_t *pos)
{
    gf_gem_debug_info_t *dbg = file_inode(f)->i_private;
    struct drm_gf_gem_object *obj = dbg->parent_gem;
    gf_card_t *gf = obj->base.dev->dev_private;
    char c = 0;
    ssize_t ret = 0;

    ret = simple_write_to_buffer(&c, 1, pos, buf, size);
    if (ret > 0) {
        if (c != '0' && !dbg->mark_unpagable) {
            gf_core_interface->prepare_and_mark_unpagable(gf->adapter, obj->core_handle, &obj->info);
            dbg->mark_unpagable = true;
        } else if (c == '0' && dbg->mark_unpagable) {
            gf_core_interface->mark_pagable(gf->adapter, obj->core_handle);
            dbg->mark_unpagable = false;
        }
        ret = size;
    }
    return ret;
}

static ssize_t gf_gem_debugfs_control_read(struct file *f, char __user *buf,
                     size_t size, loff_t *pos)
{
    gf_gem_debug_info_t *dbg = file_inode(f)->i_private;
    struct drm_gf_gem_object *obj = dbg->parent_gem;
    gf_card_t *gf = obj->base.dev->dev_private;
    char c = dbg->mark_unpagable ? '1' : '0';

    if (!gf->allocation_trace_tags) return 0;

    return simple_read_from_buffer(buf, size, pos, &c, 1);
}

static const struct file_operations debugfs_gem_control_fops = {
        .write       = gf_gem_debugfs_control_write,
        .read       = gf_gem_debugfs_control_read,
        .llseek     = default_llseek,
};

//define from mm_e3k.h
#define BL_BUFFER_SIZE         0x2000000   //32M
#define BL_INDEX_BITS          18
#define BL_MAPPING_RATIO       512

#define BL_SLOT_SIZE           (BL_BUFFER_SIZE >> BL_INDEX_BITS)
#define BL_SLICE_ALIGNMENT     (BL_BUFFER_SIZE >> BL_INDEX_BITS)

static ssize_t gf_gem_debugfs_bl_read(struct file *f, char __user *buf,
                     size_t size, loff_t *pos)
{
    gf_gem_debug_info_t *dbg = file_inode(f)->i_private;
    struct drm_gf_gem_object *obj = dbg->parent_gem;
    gf_card_t *gf = obj->base.dev->dev_private;
    adapter_info_t *ainfo = &gf->adapter_info;

    void *data = NULL;
    ssize_t result = 0;
    int data_off = 0;
    int copy_size = 0;
    int a_size = 0;

    gf_vm_area_t *vma = NULL;
    gf_map_argu_t map_argu = {0, };

    if (!gf->allocation_trace_tags) return 0;

    if (!obj->info.bl_slot_index)
    {
        gf_info("uncompress buffer bl_slot_index is 0\n");
        return result;
    }

    map_argu.flags.mem_space = GF_MEM_KERNEL;
    map_argu.flags.cache_type = GF_MEM_UNCACHED;
    map_argu.flags.mem_type   = GF_SYSTEM_IO;
    map_argu.phys_addr = obj->info.bl_slot_index * BL_SLOT_SIZE + ainfo->fb_bus_addr;
    map_argu.size = ALIGN(obj->info.size/BL_MAPPING_RATIO, BL_SLICE_ALIGNMENT); // 1:512, compress ratio

    //gf_info("offset %u, addr %llu size 0x%lu \n",obj->info.bl_slot_index * BL_SLOT_SIZE,  map_argu.phys_addr, map_argu.size);

    vma = gf_map_io_memory(NULL, &map_argu);

    if (vma != NULL)
    {
        data = vma->virt_addr;
        if (data)
        {
            a_size = vma->size;
            data_off = *pos;
            copy_size = min(size, (size_t)(a_size - data_off));
            if (copy_to_user(buf,  data + data_off, copy_size))
            {
               result = -EFAULT;
            }
            else
            {
                result += copy_size;
                *pos += copy_size;
            }
        }
        else
        {
            result = -EINVAL;
        }

        gf_unmap_io_memory(vma);
    }
    else
    {
        gf_error("map bl buffer fail \n");
        result = -EINVAL;
    }

    return result;
}

static const struct file_operations debugfs_gem_bl_fops = {
        .read       = gf_gem_debugfs_bl_read,
        .llseek     = default_llseek,
};

int gf_gem_debugfs_add_object(struct drm_gf_gem_object *obj)
{
    gf_gem_debug_info_t *dbg = &obj->debug;
    int result = 0;

    dbg->is_cpu_accessable = true; //s houdle false for cpu invisable

    gf_vsprintf(dbg->name,"%08x", obj->info.allocation);

    dbg->self_dir = debugfs_create_dir(dbg->name, dbg->root);
    if (dbg->self_dir) {
        dbg->alloc_info = debugfs_create_file("info", 0444, dbg->self_dir, dbg, &debugfs_gem_info_fops);
        dbg->data = debugfs_create_file("data", 0444, dbg->self_dir, dbg, &debugfs_gem_data_fops);
        dbg->control = debugfs_create_file("control", 0444, dbg->self_dir, dbg, &debugfs_gem_control_fops);
        dbg->bl = debugfs_create_file("bl", 0444, dbg->self_dir, dbg, &debugfs_gem_bl_fops);
    }else {
        gf_error("create allocation %s dir failed\n", dbg->name);
        result = -1;
    }
    return result;
}

void gf_gem_debugfs_remove_object(struct drm_gf_gem_object *obj)
{
    gf_gem_debug_info_t *dbg = &obj->debug;
    if (dbg->self_dir) {

        debugfs_remove(dbg->alloc_info);
        debugfs_remove(dbg->data);
        debugfs_remove(dbg->control);
        debugfs_remove(dbg->bl);
        debugfs_remove(dbg->self_dir);

        dbg->alloc_info = NULL;
        dbg->data = NULL;
        dbg->self_dir = NULL;
        dbg->control = NULL;
        dbg->bl = NULL;
    }
}

static inline gf_card_t * get_gfcard(struct pci_dev *dev)
{
    struct drm_device *drm_dev = pci_get_drvdata(dev);
    return (gf_card_t *)drm_dev->dev_private;
}
struct drm_gf_gem_object* CONCAT(gf_krnl_gem_create_object,DRIVER_NAME)(struct pci_dev *dev,
        gf_create_allocation_t *create, gf_device_debug_info_t **ddev)
{
    gf_card_t *gf = get_gfcard(dev);
    return gf_drm_gem_create_object(gf, create, ddev);
}
EXPORT_SYMBOL(CONCAT(gf_krnl_gem_create_object,DRIVER_NAME));

int CONCAT(gf_krnl_create_device, DRIVER_NAME)(struct pci_dev *dev,
        struct gf_krnl_device_create *vdev_create)
{
    int err = 0;
    gf_card_t *gf = get_gfcard(dev);
    err = gf_core_interface->create_device(gf->adapter, NULL, &vdev_create->gpu_device);
    gf_assert(err == 0, GF_FUNC_NAME(__func__));
    if(gf->debugfs_dev)
    {
        vdev_create->debug = gf_debugfs_add_device_node(gf->debugfs_dev, gf_get_current_pid(), vdev_create->gpu_device);
    }
    return err;
}

EXPORT_SYMBOL(CONCAT(gf_krnl_create_device,DRIVER_NAME));

void CONCAT(gf_krnl_destroy_device,DRIVER_NAME)(struct pci_dev *dev,
        struct gf_krnl_device_destroy *destroy)
{
    gf_card_t *gf = get_gfcard(dev);
    gf_core_interface->destroy_device(gf->adapter, destroy->gpu_device);
    if (destroy->debug && gf->debugfs_dev) {
        gf_debugfs_remove_device_node(gf->debugfs_dev, destroy->debug);
    }
}

EXPORT_SYMBOL(CONCAT(gf_krnl_destroy_device,DRIVER_NAME));


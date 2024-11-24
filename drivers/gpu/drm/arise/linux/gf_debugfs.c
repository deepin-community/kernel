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

#include "gf_debugfs.h"

typedef struct gf_mmio_segment
{
    const char *name;
    unsigned int start;
    unsigned int end;
    unsigned int alignment;
}gf_mmio_segment_t;

gf_mmio_segment_t  mmio_seg_e3k[] = {
    {"MMIO1", 0x8000, 0x80E4, 4},
    {"MMIO2", 0x8180, 0x83B0, 4},
    {"MMIO3", 0x8400, 0x8600, 4},
    {"MMIO4", 0x8600, 0x8B00, 1},
    {"MMIO5", 0x9000, 0x9200, 1},
    {"MMIO6", 0x9400, 0x9600, 1},
    {"MMIO7", 0x30000, 0x30380,4}, /* CSP */
    {"MMIO8", 0x33000, 0x35000, 4},
    {"MMIO9", 0x48C00, 0x48CD0, 4},
    {"MMIO10", 0x49000, 0x49080, 4},
};

static int gf_debugfs_node_show(struct seq_file *s, void *unused)
{
    gf_debugfs_node_t *node = (gf_debugfs_node_t *)(s->private);
    struct os_seq_file seq_file={0};
    struct os_printer seq_p = gf_seq_file_printer(&seq_file);
    struct drm_device* dev = node->dev->gf->drm_dev;

    seq_file.seq_file = s;

    gf_mutex_lock(node->dev->lock);

    switch(node->type)
    {
        case DEBUGFS_NODE_DEVICE:
            gf_core_interface->debugfs_dump(&seq_file, node->adapter, node->type, &node->hDevice);
            break;

        case DEBUGFS_NODE_HEAP:
            gf_core_interface->debugfs_dump(&seq_file, node->adapter, node->type, &node->id);
            break;

        case DEBUGFS_NODE_INFO:
            gf_core_interface->debugfs_dump(&seq_file, node->adapter, node->type, NULL);
            gf_debugfs_clock_dump(s, dev);
            break;

        case DEBUGFS_NODE_MEMTRACK:
            gf_core_interface->debugfs_dump(&seq_file, node->adapter, node->type, &node->id);
            break;
        case DEBUGFS_NODE_DVFS:
            gf_core_interface->debugfs_dump(&seq_file, node->adapter, node->type, NULL);
            break;
        case DEBUGFS_NODE_CG:
            gf_core_interface->debugfs_dump(&seq_file, node->adapter, node->type, NULL);
            break;
        case DEBUGFS_NODE_VIDSCH:
            gf_core_interface->debugfs_dump(&seq_file, node->adapter, node->type, &seq_p);
            break;
        case DEBUGFS_NODE_DEBUGBUS:
            gf_core_interface->debugfs_dump(&seq_file, node->adapter, node->type, &seq_p);
            break;
        default:
            gf_info("dump unknow node :%d\n", node->type);
            break;
    }

    gf_mutex_unlock(node->dev->lock);

    return 0;
}

static int gf_debugfs_node_open(struct inode *inode, struct file *file)
{
    return single_open(file, gf_debugfs_node_show, inode->i_private);
}

static ssize_t gf_debugfs_node_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
    struct os_seq_file seq_file = {0};
    gf_debugfs_node_t *node = NULL;
    char info[32] = {'\0'};
    int ret = 0;

    seq_file.seq_file = file->private_data;
    node = (gf_debugfs_node_t *)((seq_file.seq_file)->private);

    if(size > 1 && size < 32)
    {
        ret = gf_copy_from_user(info, buf, size);
        if(!ret)
        {
            info[size-1] = '\0';
        }
        else
            return -1;
    }

    gf_mutex_lock(node->dev->lock);

    if (!gf_strcmp(info, "1"))
    {
        gf_core_interface->dump_hang_info_flag(node->adapter, 1);
    }
    else
    {
       gf_core_interface->dump_hang_info_flag(node->adapter, 0);
    }


    gf_mutex_unlock(node->dev->lock);

    return (ret == 0) ? size : ret;
}

static const struct file_operations debugfs_node_fops = {
    .open       = gf_debugfs_node_open,
    .read       = seq_read,
    .write      = gf_debugfs_node_write,
    .llseek     = seq_lseek,
    .release    = single_release,
};

// displayinfo node
static int gf_debugfs_node_displayinfo_show(struct seq_file *s, void *unused)
{
    gf_debugfs_node_t *node = (gf_debugfs_node_t *)(s->private);
    struct drm_device* dev = node->dev->gf->drm_dev;

    gf_debugfs_displayinfo_dump(s, dev);

    return 0;
}

static int gf_debugfs_node_displayinfo_open(struct inode *inode, struct file *file)
{
    return single_open(file, gf_debugfs_node_displayinfo_show, inode->i_private);
}


static const struct file_operations debugfs_node_displayinfo_fops = {
    .open       = gf_debugfs_node_displayinfo_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
};

static int gf_debugfs_node_umd_trace_show(struct seq_file *s, void *unused)
{
    gf_debugfs_node_t *node = (gf_debugfs_node_t *)(s->private);

    seq_printf(s, "0x%lx\n", node->dev->gf->umd_trace_tags);

    return 0;
}

static int gf_debugfs_node_umd_trace_open(struct inode *inode, struct file *file)
{
    return single_open(file, gf_debugfs_node_umd_trace_show, inode->i_private);
}

static ssize_t gf_debugfs_node_umd_trace_write(struct file *f, const char __user *buf,
                                               size_t size, loff_t *pos)
{
    gf_debugfs_node_t* node = file_inode(f)->i_private;
    gf_card_t *gf_card = node->dev->gf;
    unsigned long val;
    int ret;
    char event_string[32];
    char *envp[] = { event_string, NULL };

    ret = kstrtoul_from_user(buf, size, 0, &val);
    if (ret)
        return ret;

    gf_card->umd_trace_tags = val;

    // trace_buffer is the new way to control trace
    if (gf_card->trace_buffer_vma && gf_card->trace_buffer_vma->virt_addr)
    {
        *((uint64_t *)gf_card->trace_buffer_vma->virt_addr) = val;
    }

    gf_vsprintf(event_string, "GF_TRACE_TAGS=0x%lx", val);
    // send uevent to usermode to enable/disable trace
    kobject_uevent_env(&gf_card->pdev->dev.kobj, KOBJ_CHANGE, envp);

    *pos += size;

    return size;
}

static const struct file_operations debugfs_node_umd_trace_fops = {
    .open       = gf_debugfs_node_umd_trace_open,
    .read       = seq_read,
    .write      = gf_debugfs_node_umd_trace_write,
    .llseek     = seq_lseek,
    .release    = single_release,
};

static int gf_debugfs_node_allocation_trace_show(struct seq_file *s, void *unused)
{
    gf_debugfs_node_t *node = (gf_debugfs_node_t *)(s->private);

    seq_printf(s, "0x%lx\n", node->dev->gf->allocation_trace_tags);

    return 0;
}

static int gf_debugfs_node_allocation_trace_open(struct inode *inode, struct file *file)
{
    return single_open(file, gf_debugfs_node_allocation_trace_show, inode->i_private);
}

static ssize_t gf_debugfs_node_allocation_trace_write(struct file *f, const char __user *buf,
                                               size_t size, loff_t *pos)
{
    gf_debugfs_node_t* node = file_inode(f)->i_private;
    unsigned long val;
    int ret;

    ret = kstrtoul_from_user(buf, size, 0, &val);
    if (ret)
        return ret;

    node->dev->gf->allocation_trace_tags = val;

    *pos += size;

    return size;
}

static const struct file_operations debugfs_node_allocation_trace_fops = {
    .open       = gf_debugfs_node_allocation_trace_open,
    .read       = seq_read,
    .write      = gf_debugfs_node_allocation_trace_write,
    .llseek     = seq_lseek,
    .release    = single_release,
};


static int gf_debugfs_node_video_ctr_debug_show(struct seq_file *s, void *unused)
{
    gf_debugfs_node_t *node = (gf_debugfs_node_t *)(s->private);
    seq_printf(s, "0x%x\n", node->dev->gf->video_irq_info_all);
    gf_info("show node->dev->gf->video_irq_info_all = %d \n", node->dev->gf->video_irq_info_all);

    return 0;
}

static int gf_debugfs_node_video_ctr_debug_open(struct inode *inode, struct file *file)
{
    return single_open(file, gf_debugfs_node_video_ctr_debug_show, inode->i_private);
}

static ssize_t gf_debugfs_node_video_ctr_debug_write(struct file *file, const char __user *buf,
                                               size_t size, loff_t *pos)
{
    struct os_seq_file seq_file = {0};
    gf_debugfs_node_t *node = NULL;
    char info[32] = {'\0'};
    int ret = 0;

    seq_file.seq_file = file->private_data;
    node = (gf_debugfs_node_t *)((seq_file.seq_file)->private);

    if(size > 1 && size < 32)
    {
        ret = gf_copy_from_user(info, buf, size);
        if(!ret)
        {
            info[size-1] = '\0';
        }
        else
            return -1;
    }

    gf_mutex_lock(node->dev->lock);
    if (!gf_strcmp(info, "1"))
    {
        node->dev->gf->video_irq_info_all = 1;
    }
    else
    {
        node->dev->gf->video_irq_info_all = 0;
    }
    gf_mutex_unlock(node->dev->lock);
    gf_info("write node->dev->gf->video_irq_info_all = %d \n", node->dev->gf->video_irq_info_all);

    return (ret == 0) ? size : ret;
}


static const struct file_operations debugfs_node_video_ctr_debug_fops = {
    .open       = gf_debugfs_node_video_ctr_debug_open,
    .read       = seq_read,
    .write      = gf_debugfs_node_video_ctr_debug_write,
    .llseek     = seq_lseek,
    .release    = single_release,
};


static int  gf_mmio_find_segment(gf_mmio_segment_t*  seg, int seg_num, int mmio_size, int pos, int* seg_index, int* empty)
{
    int i = 0;

    if(!seg || !seg_num || !mmio_size || (pos >= mmio_size))
    {
        return  -EINVAL;
    }

    for(i = 0; i < seg_num; i++)
    {
        if(pos >= 0 && pos < seg[0].start)
        {
            *empty = 1;
            *seg_index = 0;
            break;
        }
        else if(pos >= seg[seg_num-1].end)
        {
            *empty = 1;
            *seg_index = seg_num;
            break;
        }
        else if(pos >= seg[i].start && pos < seg[i].end)
        {
            *empty = 0;
            *seg_index = i;
            break;
        }
        else if(pos >= seg[i].end && pos < seg[i+1].start)
        {
            *empty = 1;
            *seg_index = i+1;
            break;
        }
    }

    return 0;
}

static ssize_t gf_mmio_write(struct file *f, const char __user *buf,
                                         size_t size, loff_t *pos)
{
    gf_debugfs_node_t* node = file_inode(f)->i_private;
    adapter_info_t *ainfo = &node->dev->gf->adapter_info;
    unsigned int  mmio_size = ainfo->mmio_size + 1;
    unsigned char  *mmio_base = ainfo->mmio;
    gf_mmio_segment_t*  mmio_seg = mmio_seg_e3k;
    unsigned int   seg_num = sizeof(mmio_seg_e3k)/sizeof(mmio_seg_e3k[0]);
    unsigned int  length = 0, seg_end = 0, empty_hole = 0, seg_align = 0, index = 0;
    ssize_t result = 0;
    uint32_t value = 0;

    if(!size || gf_mmio_find_segment(mmio_seg, seg_num, mmio_size, *pos, &index, &empty_hole))
    {
        return  0;
    }

    seg_align = (empty_hole)? 4 : mmio_seg[index].alignment;
    if(*pos & (seg_align - 1))
    {
        return  -EINVAL;
    }

    if(*pos + size > mmio_size)
    {
        size = mmio_size - *pos;
    }

    gf_mmio_find_segment(mmio_seg, seg_num, mmio_size, *pos+size-1, &index, &empty_hole);
    seg_align = (empty_hole)? 4 : mmio_seg[index].alignment;
    if((*pos + size) & (seg_align - 1))
    {
        return  -EINVAL;
    }

    while(size)
    {
        gf_mmio_find_segment(mmio_seg, seg_num, mmio_size, *pos, &index, &empty_hole);
        if(empty_hole)
        {
            seg_end = (index == seg_num)? mmio_size : mmio_seg[index].start;
        }
        else
        {
            seg_end = mmio_seg[index].end;
        }
        seg_align = (empty_hole)? 4 : mmio_seg[index].alignment;

        if(*pos + size > seg_end)
        {
            length = seg_end - *pos;
        }
        else
        {
            length = size;
        }

        if(empty_hole)
        {
            *pos += length;
            buf += length;
            result += length;
            size -= length;
        }
        else
        {
            size -= length;
            while(length)
            {
                if(seg_align == 4)
                {
                    get_user(value, (uint32_t __user *)buf);
                    gf_write32(mmio_base + *pos, value);
                }
                else if(seg_align == 2)
                {
                    get_user(value, (uint16_t __user *)buf);
                    gf_write16(mmio_base + *pos, (uint16_t)value);
                }
                else if(seg_align == 1)
                {
                    get_user(value, (uint8_t __user *)buf);
                    gf_write8(mmio_base + *pos, (uint8_t)value);
                }
                else
                {
                    gf_assert(0, GF_FUNC_NAME(__func__));
                }
                *pos += seg_align;
                buf += seg_align;
                result += seg_align;
                length -= seg_align;
            }
        }
    }

    return result;
}

static ssize_t gf_mmio_read(struct file *f, char __user *buf,
                    size_t size, loff_t *pos)
{
    gf_debugfs_node_t* node = file_inode(f)->i_private;
    adapter_info_t *ainfo = &node->dev->gf->adapter_info;
    unsigned int  mmio_size = ainfo->mmio_size + 1;
    unsigned char  *mmio_base = ainfo->mmio;
    gf_mmio_segment_t*  mmio_seg = mmio_seg_e3k;
    unsigned int   seg_num = sizeof(mmio_seg_e3k)/sizeof(mmio_seg_e3k[0]);
    unsigned int  length = 0, seg_end = 0, empty_hole = 0, seg_align = 0, index = 0;
    ssize_t result = 0;
    uint32_t value = 0;

    if(!size || gf_mmio_find_segment(mmio_seg, seg_num, mmio_size, *pos, &index, &empty_hole))
    {
        return  0;
    }

    seg_align = (empty_hole)? 4 : mmio_seg[index].alignment;
    if(*pos & (seg_align - 1))
    {
        return  -EINVAL;
    }

    if(*pos + size > mmio_size)
    {
        size = mmio_size - *pos;
    }

    gf_mmio_find_segment(mmio_seg, seg_num, mmio_size, *pos+size-1, &index, &empty_hole);
    seg_align = (empty_hole)? 4 : mmio_seg[index].alignment;
    if((*pos + size) & (seg_align - 1))
    {
        return  -EINVAL;
    }

    while(size)
    {
        gf_mmio_find_segment(mmio_seg, seg_num, mmio_size, *pos, &index, &empty_hole);
        if(empty_hole)
        {
            seg_end = (index == seg_num)? mmio_size : mmio_seg[index].start;
        }
        else
        {
            seg_end = mmio_seg[index].end;
        }
        seg_align = (empty_hole)? 4 : mmio_seg[index].alignment;

        if(*pos + size > seg_end)
        {
            length = seg_end - *pos;
        }
        else
        {
            length = size;
        }

        if(empty_hole)
        {
            value = 0;
            size -= length;
            while(length)
            {
                put_user(value, (uint32_t __user *)buf);
                *pos += seg_align;
                buf += seg_align;
                result += seg_align;
                length -= seg_align;
            }
        }
        else
        {
            size -= length;
            while(length)
            {
                if(seg_align == 4)
                {
                    value = gf_read32(mmio_base + *pos);
                    put_user(value, (uint32_t __user *)buf);
                }
                else if(seg_align == 2)
                {
                    value = gf_read16(mmio_base + *pos);
                    put_user((uint16_t)value, (uint16_t __user *)buf);
                }
                else if(seg_align == 1)
                {
                    value = gf_read8(mmio_base + *pos);
                    put_user((uint8_t)value, (uint8_t __user *)buf);
                }
                else
                {
                    gf_assert(0, GF_FUNC_NAME(__func__));
                }
                *pos += seg_align;
                buf += seg_align;
                result += seg_align;
                length -= seg_align;
            }
        }
    }

    return result;
}

static const struct file_operations debugfs_mmio_reg_fops = {
    .owner      = THIS_MODULE,
    .read       = gf_mmio_read,
    .write      = gf_mmio_write,
    .llseek     = default_llseek,
};

static int gf_debugfs_mmio_info_show(struct seq_file *s, void *unused)
{
    gf_debugfs_node_t *node = (gf_debugfs_node_t *)(s->private);
    adapter_info_t *ainfo = &node->dev->gf->adapter_info;
    int i;

    seq_printf(s, "mmio virtual base=0x%p\n", ainfo->mmio);
    seq_printf(s, "mmio size=0x%x\n", ainfo->mmio_size);
    for (i = 0; i < sizeof(mmio_seg_e3k)/sizeof(mmio_seg_e3k[0]); i++)
    {
        seq_printf(s, "mmio seg %s: start=0x%0x, end=0x%0x, alignment=%d\n",
            mmio_seg_e3k[i].name, mmio_seg_e3k[i].start, mmio_seg_e3k[i].end, mmio_seg_e3k[i].alignment);
    }
    return 0;
}

static int gf_debugfs_mmio_info_open(struct inode *inode, struct file *file)
{
    return single_open(file, gf_debugfs_mmio_info_show, inode->i_private);
}

static const struct file_operations debugfs_mmio_info_fops = {
    .open       = gf_debugfs_mmio_info_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
};

static void gf_debugfs_init_mmio_nodes(gf_debugfs_device_t *debug_dev)
{
    gf_debugfs_mmio_t  *debug_mmio = &debug_dev->mmio;
    debug_mmio->mmio_root = debugfs_create_dir("mmio", debug_dev->debug_root);
    if (debug_mmio->mmio_root) {

        debug_mmio->regs.type       = 0;
        debug_mmio->regs.id         = 0;
        debug_mmio->regs.adapter    = debug_dev->gf->adapter;
        debug_mmio->regs.dev        = debug_dev;

        gf_vsprintf(debug_mmio->regs.name, "regs");

        debug_mmio->regs.node_dentry = debugfs_create_file(debug_mmio->regs.name, 0664, debug_mmio->mmio_root, &(debug_mmio->regs), &debugfs_mmio_reg_fops);

        if (debug_mmio->regs.node_dentry == NULL)
        {
             gf_error("Failed to create debugfs node %s\n", debug_mmio->regs.name);
             return ;
        }

        debug_mmio->info.type       = 0;
        debug_mmio->info.id         = 0;
        debug_mmio->info.adapter    = debug_dev->gf->adapter;
        debug_mmio->info.dev        = debug_dev;

        gf_vsprintf(debug_mmio->info.name, "info");

        debug_mmio->regs.node_dentry = debugfs_create_file(debug_mmio->info.name, 0444, debug_mmio->mmio_root, &(debug_mmio->info), &debugfs_mmio_info_fops);

        if (debug_mmio->regs.node_dentry == NULL)
        {
             gf_error("Failed to create debugfs node %s\n", debug_mmio->info.name);
             return ;
        }
    }
    else  {
        gf_error("debugfs: failed to create debugfs root directory.\n");
    }
}


static int gf_debugfs_crtc_show(struct seq_file *s, void *unused)
{
    gf_debugfs_node_t *node = (gf_debugfs_node_t *)(s->private);
    struct drm_device* dev = node->dev->gf->drm_dev;

    return  gf_debugfs_crtc_dump(s, dev, node->id);
}

static int gf_debugfs_crtc_open(struct inode *inode, struct file *file)
{
    return single_open(file, gf_debugfs_crtc_show, inode->i_private);
}

static const struct file_operations debugfs_crtcs_fops = {
    .open       = gf_debugfs_crtc_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
};

static void  gf_debugfs_init_crtcs_nodes(gf_debugfs_device_t *debug_dev)
{
    gf_debugfs_crtcs_t*  crtcs = &debug_dev->crtcs;
    int i = 0;

    crtcs->crtcs_root = debugfs_create_dir("crtcs", debug_dev->debug_root);
    if(!crtcs->crtcs_root)
    {
        gf_error("Failed to create dir for debugfs crtcs root.\n");
        goto FAIL;
    }

    crtcs->debug_dev = debug_dev;
    crtcs->node_num = debug_dev->gf->drm_dev->mode_config.num_crtc;
    if(crtcs->node_num)
    {
        crtcs->crtcs_nodes = gf_calloc(sizeof(struct gf_debugfs_node)*crtcs->node_num);
    }
    if(!crtcs->crtcs_nodes)
    {
        gf_error("Failed to alloc mem for debugfs crtcs nodes.\n");
        goto  FAIL;
    }

    for(i = 0; i < crtcs->node_num; i++)
    {
        crtcs->crtcs_nodes[i].type = 0;
        crtcs->crtcs_nodes[i].id = i;

        crtcs->crtcs_nodes[i].dev = debug_dev;
        gf_vsprintf(crtcs->crtcs_nodes[i].name, "IGA%d", (i + 1));

        crtcs->crtcs_nodes[i].node_dentry = debugfs_create_file(crtcs->crtcs_nodes[i].name, 0444, crtcs->crtcs_root, &crtcs->crtcs_nodes[i], &debugfs_crtcs_fops);
        if(!crtcs->crtcs_nodes[i].node_dentry)
        {
            gf_error("Failed to create debugfs file for crtc node.\n");
            goto FAIL;
        }
    }

    return;

FAIL:

    if(crtcs->crtcs_root)
    {
        debugfs_remove_recursive(crtcs->crtcs_root);
    }

    if(crtcs->crtcs_nodes)
    {
        gf_free(crtcs->crtcs_nodes);
        crtcs->crtcs_nodes = NULL;
    }
}

gf_debugfs_device_t* gf_debugfs_create(gf_card_t *gf, struct dentry *minor_root)
{
    gf_debugfs_device_t *dev;
    int i=0;
    gf_heap_info_t  *heap;
    dev = gf_calloc(sizeof(struct gf_debugfs_device));
    if (!dev)
    {
        return NULL;
    }

    dev->gf         = gf;
    dev->lock       = gf_create_mutex();
    INIT_LIST_HEAD(&(dev->node_list));
    INIT_LIST_HEAD(&(dev->device_node_list));

    dev->debug_root = debugfs_create_dir("gf", minor_root);
    if (!dev->debug_root)
    {
        gf_error("debugfs: failed to create debugfs root directory.\n");
        return NULL;
    }

    dev->device_root = debugfs_create_dir("devices", dev->debug_root);

    dev->allocation_root = debugfs_create_dir("allocations", dev->debug_root);

/*HEAP info*/
    heap = &dev->heap_info;
    heap->heap_dir = debugfs_create_dir("heaps", dev->debug_root);
    if (!heap->heap_dir)
    {
        gf_error("debugfs: failed to create debugfs root directory.\n");
        return NULL;
    }

    for(i = 0; i<DEBUGFS_HEAP_NUM; i++)
    {
        heap->heap[i].type       = DEBUGFS_NODE_HEAP;
        heap->heap[i].id         = i;
        heap->heap[i].adapter    = dev->gf->adapter;
        heap->heap[i].dev        = dev;

        gf_vsprintf(heap->heap[i].name, "heap%d", i);

        heap->heap[i].node_dentry = debugfs_create_file(heap->heap[i].name, 0664, heap->heap_dir, &(heap->heap[i]), &debugfs_node_fops);

        if (heap->heap[i].node_dentry == NULL)
        {
            gf_error("Failed to create debugfs node %s\n", heap->heap[i].name);
            return NULL;
        }
    }

    dev->info.type       = DEBUGFS_NODE_INFO;
    dev->info.adapter    = dev->gf->adapter;
    dev->info.dev        = dev;

    gf_vsprintf(dev->info.name, "info");

    dev->info.node_dentry = debugfs_create_file(dev->info.name, 0664, dev->debug_root, &(dev->info), &debugfs_node_fops);

    if (dev->info.node_dentry == NULL)
    {
        gf_error("Failed to create debugfs node %s\n", dev->info.name);
        return NULL;
    }

    dev->memtrack.type       = DEBUGFS_NODE_MEMTRACK;
    dev->memtrack.adapter    = dev->gf->adapter;
    dev->memtrack.dev        = dev;
    dev->memtrack.id        = -1;

    gf_vsprintf(dev->memtrack.name, "memtrack");

    dev->memtrack.node_dentry = debugfs_create_file(dev->memtrack.name, 0664, dev->debug_root, &(dev->memtrack), &debugfs_node_fops);

    if (dev->memtrack.node_dentry == NULL)
    {
        gf_error("Failed to create debugfs node %s\n", dev->memtrack.name);
        return NULL;
    }

    dev->vidsch.type       = DEBUGFS_NODE_VIDSCH;
    dev->vidsch.adapter    = dev->gf->adapter;
    dev->vidsch.dev        = dev;

    gf_vsprintf(dev->vidsch.name, "vidsch");

    dev->vidsch.node_dentry = debugfs_create_file(dev->vidsch.name, 0664, dev->debug_root, &(dev->vidsch), &debugfs_node_fops);

    if (dev->vidsch.node_dentry == NULL)
    {
        gf_error("Failed to create debugfs node %s\n", dev->vidsch.name);
        return NULL;
    }

    // here to create  displayinfo node to get info
    dev->displayinfo.type       = 0;
    dev->displayinfo.adapter    = dev->gf->adapter;
    dev->displayinfo.dev        = dev;

    gf_vsprintf(dev->displayinfo.name, "displayinfo");

    dev->displayinfo.node_dentry = debugfs_create_file(dev->displayinfo.name, 0664, dev->debug_root, &(dev->displayinfo), &debugfs_node_displayinfo_fops);

    if (dev->displayinfo.node_dentry == NULL)
    {
        gf_error("Failed to create debugfs node %s\n", dev->displayinfo.name);
        return NULL;
    }

    dev->debugbus.type     = DEBUGFS_NODE_DEBUGBUS;
    dev->debugbus.adapter  = dev->gf->adapter;
    dev->debugbus.dev      = dev;

    gf_vsprintf(dev->debugbus.name, "debugbus");

    dev->debugbus.node_dentry = debugfs_create_file(dev->debugbus.name, 0664, dev->debug_root, &(dev->debugbus), &debugfs_node_fops);

    if (dev->debugbus.node_dentry == NULL)
    {
        gf_error("Failed to create debugfs node %s\n", dev->debugbus.name);
    }

    // create file to enable/disable usermode trace.
    // NOTE: kmd trace is controlled by /sys/kernel/debug/tracing/events/gfx/enable
    dev->umd_trace.type       = 0;
    dev->umd_trace.adapter    = dev->gf->adapter;
    dev->umd_trace.dev        = dev;

    gf_vsprintf(dev->umd_trace.name, "umd_trace");

    dev->umd_trace.node_dentry = debugfs_create_file(dev->umd_trace.name, 0664, dev->debug_root, &(dev->umd_trace), &debugfs_node_umd_trace_fops);

    if (dev->umd_trace.node_dentry == NULL)
    {
        gf_error("Failed to create debugfs node %s\n", dev->umd_trace.name);
        return NULL;
    }

    // create file to enable/disable allocation trace
    dev->allocation_trace.type       = 0;
    dev->allocation_trace.adapter    = dev->gf->adapter;
    dev->allocation_trace.dev        = dev;

    gf_vsprintf(dev->allocation_trace.name, "allocation_trace");

    dev->allocation_trace.node_dentry = debugfs_create_file(dev->allocation_trace.name, 0664, dev->debug_root, &(dev->allocation_trace), &debugfs_node_allocation_trace_fops);

    if (dev->allocation_trace.node_dentry == NULL)
    {
        gf_error("Failed to create debugfs node %s\n", dev->allocation_trace.name);
        return NULL;
    }

    // create file to control video interrupt output info
    dev->video_interrupt_info.type       = 0;
    dev->video_interrupt_info.adapter    = dev->gf->adapter;
    dev->video_interrupt_info.dev        = dev;

    gf_vsprintf(dev->video_interrupt_info.name, "video_interrupt_info");

    dev->video_interrupt_info.node_dentry = debugfs_create_file(dev->video_interrupt_info.name, 0664, dev->debug_root, &(dev->video_interrupt_info), &debugfs_node_video_ctr_debug_fops);

    if (dev->video_interrupt_info.node_dentry == NULL)
    {
        gf_error("Failed to create debugfs node %s\n", dev->video_interrupt_info.name);
        return NULL;
    }

    gf_debugfs_init_mmio_nodes(dev);

    gf_debugfs_init_crtcs_nodes(dev);

    return dev;
}

int gf_debugfs_destroy(gf_debugfs_device_t* dev)
{
    gf_debugfs_node_t *node1 = NULL;
    gf_debugfs_node_t *node2 = NULL;

    if(dev == NULL)
    {
        gf_error("destroy a null dubugfs dev\n");
        return -1;
    }

    if(dev->lock)
    {
        gf_destroy_mutex(dev->lock);
    }

    if(dev->debug_root)
    {
        debugfs_remove_recursive(dev->debug_root);
    }

    list_for_each_entry_safe(node1, node2, &(dev->node_list), list_item)
    {
        gf_free(node1);
    }

    if(dev->crtcs.crtcs_nodes)
    {
        gf_free(dev->crtcs.crtcs_nodes);
    }

    gf_free(dev);

    return 0;
}

static int gf_debugfs_device_show(struct seq_file *s, void *unused)
{
    gf_device_debug_info_t *node = (gf_device_debug_info_t *)(s->private);
    seq_printf(s, "pid = %lu\n", node->user_pid);
    seq_printf(s, "handle = 0x%x\n", node->hDevice);

    return 0;
}

static int gf_debugfs_device_open(struct inode *inode, struct file *file)
{
    return single_open(file, gf_debugfs_device_show, inode->i_private);
}

static const struct file_operations debugfs_device_fops = {
    .open       = gf_debugfs_device_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
};

gf_device_debug_info_t* gf_debugfs_add_device_node(gf_debugfs_device_t* dev, int id, unsigned int handle)
{
    gf_device_debug_info_t *node  = gf_calloc(sizeof(gf_device_debug_info_t));

    gf_device_debug_info_t *node1   = NULL;
    gf_device_debug_info_t *node2   = NULL;
    struct dentry *parent = dev->device_root;

    node->id                = id;
    node->adapter           = dev->gf->adapter;
    node->debugfs_dev       = dev;
    node->hDevice           = handle;

    gf_mutex_lock(dev->lock);

    list_for_each_entry_safe(node1, node2, &(dev->device_node_list), list_item)
    {
        if(node1->id == node->id)
        {
            node->min_id++;
        }
    }
    gf_mutex_unlock(dev->lock);

    gf_vsprintf(node->name, "%08x", handle);
    node->dentry = debugfs_create_dir(node->name, parent);
    if (node->dentry) {
        node->info =   debugfs_create_file(node->name, 0664, node->dentry, node, &debugfs_device_fops);
        if (!node->info)
        {
            gf_error("Failed to create debugfs node %s\n", node->name);
            gf_free(node);

            return NULL;
        }
        node->d_alloc = debugfs_create_dir("allocations", node->dentry);
        if (!node->d_alloc) {
            gf_error("Failed to create debugfs node %s allocations dir%s\n", node->name);
            gf_free(node);

            return NULL;
        }
    } else {
            gf_error("Failed to create debugfs dir %s\n", node->name);
            gf_free(node);
            return NULL;
    }

    gf_mutex_lock(dev->lock);
    list_add_tail(&(node->list_item), &(dev->device_node_list));
    gf_mutex_unlock(dev->lock);

    return node;
}

int gf_debugfs_remove_device_node(gf_debugfs_device_t* dev, gf_device_debug_info_t *dnode)
{
    gf_device_debug_info_t *node1 = NULL;
    gf_device_debug_info_t *node2 = NULL;

    if (dnode == NULL)
    {
        return 0;
    }

    gf_mutex_lock(dev->lock);

    list_for_each_entry_safe(node1, node2, &(dev->device_node_list), list_item)
    {
        if(node1 && (node1->hDevice == dnode->hDevice))
        {
            debugfs_remove_recursive(node1->dentry);
            list_del(&(node1->list_item));

            gf_free(node1);
            break;
        }
    }

    gf_mutex_unlock(dev->lock);

    return 0;
}

int gf_debugfs_init(gf_card_t *gf)
{
    gf->debugfs_dev = gf_debugfs_create(gf, gf->drm_dev->primary->debugfs_root);
    if (!gf->debugfs_dev)
    {
        return -1;
    }

    return 0;
}

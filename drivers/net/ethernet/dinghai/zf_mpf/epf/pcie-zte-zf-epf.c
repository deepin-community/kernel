#include "pcie-zte-zf-epf.h"
#include "./../epc/pcie-zte-zf-epc.h"

#define EPF_MDEV_OPS 1
#define MDEV_FOPS    1

extern void *pci_epf_alloc_space(struct pci_epf *epf, size_t size, enum pci_barno bar, size_t align);
extern void pci_epf_free_space(struct pci_epf *epf, void *addr, enum pci_barno bar);
extern int ep_virtio_module_set(int ep_id, int pf_idx, int en);
extern int pcie_zte_epc_ob_read(struct pci_epc *epc, phys_addr_t phys_addr, unsigned int size, unsigned int *val);

#if EPF_MDEV_OPS
static int pci_epf_dev_set_pf_bar(struct pci_epf *epf)
{
    int bar_no = 0, add = 0;
    int ret = 0;
    struct pci_epf_bar *epf_bar = NULL;
    struct pci_epc *epc = epf->epc;
    struct device *dev = &epf->dev;
    struct pci_epf_mdev_dev *epf_mdev_dev = epf_get_drvdata(epf);
    const struct pci_epc_features *epc_features = NULL;

    epc_features = epf_mdev_dev->epc_features;

    for (bar_no = 0; bar_no < PCI_STD_NUM_BARS; bar_no += add)
    {
        epf_bar = &epf->bar[bar_no];
        epf_bar->flags |= BIT(3);
        add = (epf_bar->flags & PCI_BASE_ADDRESS_MEM_TYPE_64) ? 2 : 1;
        if (!!(epc_features->reserved_bar & (1 << bar_no)))
            continue;

        ret = pci_epc_set_bar(epc, epf->func_no, epf->vfunc_no, epf_bar);
        if (ret)
        {
            dev_err(dev, "Failed to set BAR%d\n", bar_no);
        }
    }

    // set oprom bar
    if (epf->header->pf_rom_size != 0)
    {
        epf->bar[BAR_ROM].phys_addr = page_to_phys(epf->header->pf_rom_page);
        ret = pci_epc_set_bar(epc, epf->func_no, epf->vfunc_no, &epf->bar[BAR_ROM]);
        if (ret)
        {
            dev_err(dev, "Failed to set BAR_ROM\n");
        }
    }

    return 0;
}

static int pci_epf_dev_set_vf_bar(struct pci_epf *epf)
{
    int bar_no = 0, add = 0;
    int ret = 0;
    struct pci_epf_bar *epf_bar;
    struct pci_epc *epc = epf->epc;
    struct device *dev = &epf->dev;
    struct pci_epf_mdev_dev *epf_mdev_dev = epf_get_drvdata(epf);
    const struct pci_epc_features *epc_features;

    epc_features = epf_mdev_dev->epc_features;

    for (bar_no = 0; bar_no < PCI_STD_NUM_BARS; bar_no += add)
    {
        epf_bar = &epf->epf_pf->vf_bar[bar_no];
        epf_bar->flags |= BIT(3);
        add = (epf_bar->flags & PCI_BASE_ADDRESS_MEM_TYPE_64) ? 2 : 1;
        if (!!(epc_features->reserved_bar & (1 << bar_no)))
            continue;

        ret = pci_epc_set_bar(epc, epf->func_no, epf->vfunc_no, epf_bar);
        if (ret)
        {
            dev_err(dev, "Failed to set BAR%d\n", bar_no);
        }
    }
    return 0;
}

// 根据epf来配置epc的bar、header、msi、msi-x
static int pci_epf_dev_core_init(struct pci_epf *epf)
{
    const struct pci_epc_features *epc_features;
    struct device *dev = &epf->dev;
    bool msix_capable = false;
    int ret = 0;

    epc_features = pci_epc_get_features(epf->epc, epf->func_no, epf->vfunc_no);
    if (epc_features)
    {
        msix_capable = epc_features->msix_capable;
    }

    ret = pci_epc_write_header(epf->epc, epf->func_no, epf->vfunc_no, epf->header);
    if (ret)
    {
        dev_err(dev, "Configuration header write failed\n");
        return ret;
    }

    if (isPF(epf->func_no))
    {
        ret = pci_epf_dev_set_pf_bar(epf);
    }
    else if (epf->vfunc_no == 0)
    {
        ret = pci_epf_dev_set_vf_bar(epf);
    }

    return ret;
}

static int get_vf_number(struct pci_epf *epf)
{
    struct pcie_dpu_ep *dpu_ep = epc_get_drvdata(epf->epc);
    int vf_total_num = 0;

    if (!dpu_ep)
    {
        DH_LOG_ERR(MODULE_MPF, "epc is NULL!!!\n");
        return -EINVAL;
    }

    vf_total_num = epf->func_no & PCIE_DPU_EP_GET_PF_NO;
    if (vf_total_num >= PCIE_DPU_PF_NUMS)
    {
        DH_LOG_ERR(MODULE_MPF, "error vf_total_num=%d\n", vf_total_num);
        return -EINVAL;
    }
    return dpu_ep->vf_total_num[vf_total_num];
}

// 填充pci_epf_bar结构体内容
static int alloc_bar_space(struct pci_epf *epf)
{
    u16 bar_no = 0;
    int vf_num = 0;
    struct pci_epf_mdev_dev *epf_mdev_dev = NULL;

    if (!epf)
    {
        DH_LOG_ERR(MODULE_MPF, "epf is NULL!\n");
        return -EINVAL;
    }
    if (!epf->epc)
    {
        DH_LOG_ERR(MODULE_MPF, "epf->epc is NULL!\n");
        return -EINVAL;
    }

    if (!epf->epc->ops->get_max_vfs)
    {
        DH_LOG_ERR(MODULE_MPF, "get_max_vfs is NULL!\n");
        return -EINVAL;
    }

    epf_mdev_dev = epf_get_drvdata(epf);
    vf_num = get_vf_number(epf);

    if (isPF(epf->func_no))
    {
        epf->bar[BAR_0].size = epf->header->pf_bar0_size;
        epf->bar[BAR_2].size = epf->header->pf_bar2_size;
        epf->bar[BAR_4].size = epf->header->pf_bar4_size;
    }
    else
    {
        epf->epf_pf->vf_bar[4].size = epf->epf_pf->header->vf_bar4_size;
    }

    // 申请物理空间
    for (bar_no = 0; bar_no < PCI_STD_NUM_BARS - 2; bar_no += 2)
    {
        epf->bar[bar_no].addr = pci_epf_alloc_space(epf, epf->bar[bar_no].size, bar_no, GFP_KERNEL);
        if (!epf->bar[bar_no].addr)
        {
            DH_LOG_ERR(MODULE_MPF, "Don't have enough memory!\n");
            return -ENOMEM;
        }
    }

    for (bar_no = 0; bar_no < PCI_STD_NUM_BARS; bar_no += 2)
    {
        // PF的信息获取
        epf->bar[bar_no].barno = bar_no;
        epf->bar[bar_no].flags |= PCI_BASE_ADDRESS_MEM_TYPE_64;
        if (isPF(epf->func_no))
        {
            // PF下的VF的信息获取
            epf->vf_bar[bar_no].barno = bar_no;
            epf->vf_bar[bar_no].flags |= PCI_BASE_ADDRESS_MEM_TYPE_64;
        }
    }

    return 0;
}

static void pci_epf_mdev_unbind(struct pci_epf *epf)
{
    int bar_no = 0;
    struct pci_epc *epc = epf->epc;

    DH_LOG_ERR(MODULE_MPF, "pf = 0x%x, vf = 0x%x\n", epf->func_no, epf->vfunc_no);
    if (epf->vfunc_no == 0)
    {
        for (bar_no = 0; bar_no < PCI_STD_NUM_BARS - 2; bar_no += 2)
        {
            pci_epf_free_space(epf, epf->bar[bar_no].addr, bar_no);
        }
    }

    if (isPF(epf->func_no))
    {
        clear_bit(epf->func_no, &epc->function_num_map);
    }
}

static int pci_epf_mdev_bind(struct pci_epf *epf)
{
    int ret = 0;
    struct pci_epf_mdev_dev *epf_mdev_dev = epf_get_drvdata(epf);
    const struct pci_epc_features *epc_features = NULL;
    struct pci_epc *epc = epf->epc;
    bool linkup_notifier = false;
    bool core_init_notifier = false;

    if (!epc)
    {
        DH_LOG_ERR(MODULE_MPF, "epc is NULL!!!\n");
        return -EINVAL;
    }

    if (isPF(epf->func_no))
    {
        epf->is_vf = PCI_EPF_SRIOV_PF;
    }
    else
    {
        epf->is_vf = PCI_EPF_SRIOV_VF;
    }

    ret = alloc_bar_space(epf);
    if (ret)
    {
        DH_LOG_ERR(MODULE_MPF, "Alloc bar sapce error!\n");
        return ret;
    }

    epc_features = pci_epc_get_features(epc, epf->func_no, epf->vfunc_no);
    if (!epc_features)
    {
        dev_err(&epf->dev, "epc_features not implemented\n");
        return -EOPNOTSUPP;
    }

    linkup_notifier = epc_features->linkup_notifier;
    core_init_notifier = epc_features->core_init_notifier;
    epf_mdev_dev->epc_features = epc_features;
    if (!core_init_notifier)
    {
        ret = pci_epf_dev_core_init(epf);
        if (ret)
        {
            return ret;
        }
    }

    return 0;
}

static const struct pci_epf_device_id pci_epf_dev_ids[] = {
    {
     .name = "pci-epf-mdev",
     },
    {},
};

static struct pci_epf_ops epf_mdev_ops = {
    .unbind = pci_epf_mdev_unbind,
    .bind = pci_epf_mdev_bind,
};
#endif

#if MDEV_FOPS
// sample_pci_mdev_dev_show
static ssize_t sample_pci_mdev_dev_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "This is pci-epf-mdev device\n");
}
static DEVICE_ATTR_RO(sample_pci_mdev_dev);

static struct attribute *pci_mdev_dev_attrs[] = {
    &dev_attr_sample_pci_mdev_dev.attr,
    NULL,
};

static const struct attribute_group pci_mdev_dev_group = {
    .name = "pcie_mdev_dev",
    .attrs = pci_mdev_dev_attrs,
};

static const struct attribute_group *pci_mdev_dev_groups[] = {
    &pci_mdev_dev_group,
    NULL,
};

// sample_mdev_dev_show
static ssize_t sample_mdev_dev_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    if (mdev_from_dev(dev))
        return sprintf(buf, "This is MDEV %s\n", dev_name(dev));

    return sprintf(buf, "\n");
}

static DEVICE_ATTR_RO(sample_mdev_dev);

static struct attribute *mdev_dev_attrs[] = {
    &dev_attr_sample_mdev_dev.attr,
    NULL,
};

static const struct attribute_group mdev_dev_group = {
    .name = "vendor",
    .attrs = mdev_dev_attrs,
};

static const struct attribute_group *mdev_dev_groups[] = {
    &mdev_dev_group,
    NULL,
};

// name_show +++ device_api_show
static ssize_t name_show(struct mdev_type *mtype,
                         struct mdev_type_attribute *attr, char *buf)
{
    return sprintf(buf, "%s\n", "ZTE epf mdev");
}
static MDEV_TYPE_ATTR_RO(name);

static ssize_t device_api_show(struct mdev_type *mtype,
                               struct mdev_type_attribute *attr, char *buf)
{
    return sprintf(buf, "%s\n", VFIO_DEVICE_API_PCI_STRING);
}

static MDEV_TYPE_ATTR_RO(device_api);

static struct attribute *mdev_types_attrs[] = {
    &mdev_type_attr_name.attr,
    &mdev_type_attr_device_api.attr,
    NULL,
};

static struct attribute_group mdev_type_group = {
    .name = "single",
    .attrs = mdev_types_attrs,
};

static struct attribute_group *mdev_type_groups[] = {
    &mdev_type_group,
    NULL,
};

static int pci_mdev_create(struct mdev_device *mdev)
{
    struct mdev_state *mdev_state = NULL;
    struct device *dev = mdev->type->parent->dev;
    struct pci_epf *epf = container_of(dev, struct pci_epf, dev);
    struct pci_epf_mdev_dev *epf_mdev_dev = epf_get_drvdata(epf);
    int ret = 0;

    if (epf_mdev_dev->created_flag == 1)
    {
        DH_LOG_ERR(MODULE_MPF, "This mdev has been created!!!\n");
        return -EPERM;
    }

    mdev_state = kzalloc(sizeof(struct mdev_state), GFP_KERNEL);
    if (!mdev_state)
    {
        return -ENOMEM;
    }

    mdev_state->irq_index = -1;
    mdev_state->epf_mdev_dev = epf_mdev_dev;
    mdev_state->mdev = mdev;
    mutex_init(&mdev_state->ops_lock);
    mutex_init(&mdev_state->ioeventfds_lock);
    INIT_LIST_HEAD(&mdev_state->ioeventfds_list);

    mdev_set_drvdata(mdev, mdev_state);

    epf_mdev_dev->created_flag = 1;

    return ret;
}

static int pci_mdev_remove(struct mdev_device *mdev)
{
    int ret = 0;
    // int bar_no = 0;
    struct mdev_state *mdev_state = NULL;
    struct epf_mdev_ioeventfd *ioeventfd = NULL, *temp = NULL;
    struct device *dev = NULL;
    struct pci_epf *epf = NULL;
    struct pci_epf_mdev_dev *epf_mdev_dev = NULL;

    mdev_state = mdev_get_drvdata(mdev);
    if (!mdev_state)
    {
        DH_LOG_ERR(MODULE_MPF, "mdev_state NULL\n");
        return 0;
    }
    dev = mdev->type->parent->dev;
    if (!dev)
    {
        DH_LOG_ERR(MODULE_MPF, "dev NULL\n");
        return 0;
    }
    epf = container_of(dev, struct pci_epf, dev);
    epf_mdev_dev = epf_get_drvdata(epf);
    if (epf_mdev_dev == NULL)
    {
        DH_LOG_ERR(MODULE_MPF, "epf_mdev_dev NULL\n");
        return 0;
    }

    mutex_lock(&mdev_state->ioeventfds_lock);
    list_for_each_entry_safe(ioeventfd, temp, &mdev_state->ioeventfds_list, next)
    {
        vfio_virqfd_disable(&ioeventfd->virqfd);
        list_del(&ioeventfd->next);
        mdev_state->ioeventfds_nr--;
        kfree(ioeventfd);
    }
    mutex_unlock(&mdev_state->ioeventfds_lock);
    mutex_destroy(&mdev_state->ioeventfds_lock);
    mutex_destroy(&mdev_state->ops_lock);
    kfree(mdev_state); // todo LJP

    epf_mdev_dev->created_flag = 0;

    return ret;
}

static int pci_mdev_open(struct mdev_device *mdev)
{
    return 0;
}

static void pci_mdev_close(struct mdev_device *mdev)
{
}

static ssize_t pci_mdev_read(struct mdev_device *mdev, char __user *buf,
                             size_t count, loff_t *ppos)
{
    int ret = 0;

    return ret;
}

static ssize_t pci_mdev_write(struct mdev_device *mdev, const char __user *buf,
                              size_t count, loff_t *ppos)
{
    int ret = 0;
    struct mdev_state *mdev_state = mdev_get_drvdata(mdev);

    if (!mdev_state)
    {
        DH_LOG_ERR(MODULE_MPF, "mdev_state is NULL!!!\n");
        return -EINVAL;
    }

    return ret;
}

static int pci_mdev_mmap(struct mdev_device *mdev, struct vm_area_struct *vma)
{
    struct device *dev = mdev->type->parent->dev;
    struct pci_epf *epf = container_of(dev, struct pci_epf, dev);
    struct pci_epc *epc = epf->epc;
    // struct pci_epf_mdev_dev *epf_mdev_dev = epf_get_drvdata(epf);
    int ret = 0;
    int bar_no = vma->vm_pgoff;

    if (!epc)
    {
        DH_LOG_ERR(MODULE_MPF, "epc is NULL!!!\n");
    }

    if (vma->vm_end < vma->vm_start)
    {
        goto parameter_error;
    }

    if (vma->vm_end - vma->vm_start > epf->bar[bar_no].size)
    {
        goto parameter_error;
    }

    if ((vma->vm_flags & VM_SHARED) == 0)
    {
        goto parameter_error;
    }

    ret = remap_vmalloc_range_partial(vma, vma->vm_start, epf->bar[bar_no].addr, 0, vma->vm_end - vma->vm_start);

    if (ret)
    {
        DH_LOG_ERR(MODULE_MPF, "remap_vmalloc_range_partial error!!!\n");
    }

    return ret;

parameter_error:
    DH_LOG_ERR(MODULE_MPF, "vm area parameter error!!!\n");
    return -EINVAL;
}

#if 1 // ioctl
static int zte_get_device_info(struct mdev_device *mdev, struct vfio_device_info *dev_info)
{
    dev_info->flags = VFIO_DEVICE_FLAGS_PCI;
    dev_info->num_regions = VFIO_PCI_NUM_REGIONS;
    dev_info->num_irqs = VFIO_PCI_NUM_IRQS;

    return 0;
}

static int zte_get_region_info(struct mdev_device *mdev, struct vfio_region_info *region_info)
{
    struct mdev_state *mdev_state = NULL;
    struct device *dev = mdev->type->parent->dev;
    struct pci_epf *epf = container_of(dev, struct pci_epf, dev);
    int bar_no = 0;
    int bar_size = 0;

    mdev_state = mdev_get_drvdata(mdev);
    if (!mdev_state)
    {
        return -EINVAL;
    }

    bar_no = region_info->index;
    if (bar_no >= VFIO_PCI_BAR5_REGION_INDEX)
    {
        DH_LOG_ERR(MODULE_MPF, "bar index invaild!\n");
        return -EINVAL;
    }

    mutex_lock(&mdev_state->ops_lock);

    if (bar_no == VFIO_PCI_BAR0_REGION_INDEX || bar_no == VFIO_PCI_BAR2_REGION_INDEX || bar_no == VFIO_PCI_BAR4_REGION_INDEX)
    {
        bar_size = epf->bar[bar_no].size;
    }
    else
    {
        DH_LOG_ERR(MODULE_MPF, "bar index invaild!\n");
        return -EINVAL;
    }

    mdev_state->region_info[bar_no].size = bar_size;

    region_info->offset = (bar_no << 12);
    region_info->size = bar_size;
    region_info->flags = VFIO_REGION_INFO_FLAG_READ | VFIO_REGION_INFO_FLAG_WRITE;
    mutex_unlock(&mdev_state->ops_lock);
    return 0;
}

static void __attribute__((unused)) epf_mdev_ioeventfd_thread(void *opaque, void *unused)
{
    // 打桩
}

int epf_mdev_ioeventfd_handler(void *opaque, void *unused)
{
    int ret = 0;
    struct epf_mdev_ioeventfd *ioeventfd = opaque;
    struct mdev_state *mdev_state = ioeventfd->mdev_state;
    struct pci_epf_mdev_dev *epf_mdev_dev = mdev_state->epf_mdev_dev;
    struct pci_epf *epf = epf_mdev_dev->epf;
    struct pci_epc *epc = epf->epc;
    enum pci_epc_irq_type irq_type = PCI_EPC_IRQ_MSIX;

    pci_epc_raise_irq(epc, epf->func_no, epf->vfunc_no, irq_type, ioeventfd->offset + 1);
    return ret;
}

#if 0
int epf_mdev_ioeventfd_func(struct mdev_state *mdev_state, loff_t offset, u64 data, int count, int fd)
{
    int ret = 0;
    struct epf_mdev_ioeventfd *ioeventfd = NULL, *temp = NULL;

#ifndef iowrite64
    if (count == 8)
        return -EINVAL;
#endif

    mutex_lock(&mdev_state->ioeventfds_lock);
    list_for_each_entry_safe(ioeventfd, temp, &mdev_state->ioeventfds_list, next)
    {
        if (ioeventfd->offset == offset)
        {
            if (fd == -1 || data == 0)
            {
                vfio_virqfd_disable(&ioeventfd->virqfd);
                list_del(&ioeventfd->next);
                mdev_state->ioeventfds_nr--;
                kfree(ioeventfd);
                ret = 0;
            }
            else if (data == 1)
            {
                vfio_virqfd_disable(&ioeventfd->virqfd);
                list_del(&ioeventfd->next);
                mdev_state->ioeventfds_nr--;
                kfree(ioeventfd);
                goto eventfds_set;
            }
            else
            {
                ret = -EEXIST;
            }

            goto out_unlock;
        }
    }

    if (fd < 0)
    {
        ret = -ENODEV;
        goto out_unlock;
    }

    if (mdev_state->ioeventfds_nr >= EPF_MDEV_IOEVENTFD_MAX)
    {
        ret = -ENOSPC;
        goto out_unlock;
    }

eventfds_set:
    ioeventfd = kzalloc(sizeof(*ioeventfd), GFP_KERNEL);
    if (!ioeventfd)
    {
        ret = -ENOMEM;
        goto out_unlock;
    }

    ioeventfd->mdev_state = mdev_state;
    ioeventfd->data = data;
    ioeventfd->offset = offset;
    ioeventfd->count = count;

    ret = vfio_virqfd_enable(ioeventfd, epf_mdev_ioeventfd_handler,
                             epf_mdev_ioeventfd_thread, NULL,
                             &ioeventfd->virqfd, fd);
    if (ret)
    {
        kfree(ioeventfd);
        goto out_unlock;
    }

    list_add(&ioeventfd->next, &mdev_state->ioeventfds_list);
    mdev_state->ioeventfds_nr++;

out_unlock:
    mutex_unlock(&mdev_state->ioeventfds_lock);

    return ret;
}
#endif

static int device_get_info(struct mdev_device *mdev, struct mdev_state *mdev_state, unsigned long arg)
{
    unsigned long minsz = 0;
    struct vfio_device_info info = {0};

    minsz = offsetofend(struct vfio_device_info, num_irqs);
    if (copy_from_user(&info, (void __user *)arg, minsz))
    {
        return -EFAULT;
    }

    if (info.argsz < minsz)
    {
        return -EINVAL;
    }

    zte_get_device_info(mdev, &info);

    memcpy(&mdev_state->dev_info, &info, sizeof(info));
    if (copy_to_user((void __user *)arg, &info, minsz))
        return -EFAULT;
    return 0;
}

static int device_get_region_info(struct mdev_device *mdev, struct mdev_state *mdev_state, unsigned long arg)
{
    struct vfio_region_info info = {0};
    unsigned long minsz = 0;
    int ret = 0;

    minsz = offsetofend(struct vfio_region_info, offset);
    if (copy_from_user(&info, (void __user *)arg, minsz))
        return -EFAULT;

    if (info.argsz < minsz)
        return -EINVAL;

    ret = zte_get_region_info(mdev, &info);
    if (ret)
        return ret;

    if (copy_to_user((void __user *)arg, &info, minsz))
    {
        return -EFAULT;
    }

    return 0;
}

static int vfio_device_ioeventfd(struct mdev_device *mdev, struct mdev_state *mdev_state, unsigned long arg)
{
    struct vfio_device_ioeventfd ioeventfd = {0};
    int ret = 0;
    int count = 0;
    unsigned long minsz = 0;
    struct pci_epf_mdev_dev *epf_mdev_dev = mdev_state->epf_mdev_dev;
    struct pci_epf *epf = epf_mdev_dev->epf;

    minsz = offsetofend(struct vfio_device_ioeventfd, fd);
    if (copy_from_user(&ioeventfd, (void __user *)arg, minsz))
    {
        DH_LOG_ERR(MODULE_MPF, "copy from user failed! minsz=0x%lx\n", minsz);
        return -EFAULT;
    }

    DH_LOG_INFO(MODULE_MPF, "func = 0x%x, vfunc_no = 0x%x, offset = 0x%llx, data = 0x%llx, minsz=0x%lx\n", epf->func_no, epf->vfunc_no, ioeventfd.offset, ioeventfd.data, minsz);
    if (ioeventfd.argsz < minsz)
    {
        return -EINVAL;
    }

    if (ioeventfd.flags & ~VFIO_DEVICE_IOEVENTFD_SIZE_MASK)
    {
        return -EINVAL;
    }

    count = ioeventfd.flags & VFIO_DEVICE_IOEVENTFD_SIZE_MASK;

    if (hweight8(count) != 1 || ioeventfd.fd < -1)
        return -EINVAL;

    // ret = epf_mdev_ioeventfd_func(mdev_state, (loff_t)ioeventfd.offset, (u64)ioeventfd.data, count, (int)ioeventfd.fd);
    DH_LOG_ERR(MODULE_MPF, "error: epf_mdev_ioeventfd_func not called!\n");
    return ret;
}

static int vfio_outbound_set(struct mdev_device *mdev, struct mdev_state *mdev_state, unsigned long arg)
{
    int ret = 0;
    struct pci_epf_mdev_dev *epf_mdev_dev = mdev_state->epf_mdev_dev;
    struct pci_epf *epf = epf_mdev_dev->epf;
    struct pci_epc *epc = epf->epc;
    struct ioctl_ob_data ob = {0};
    void __iomem *dst_addr = NULL;
    phys_addr_t dst_phys_addr = 0;

    if (copy_from_user(&ob, (void *)arg, sizeof(struct ioctl_ob_data)))
    {
        DH_LOG_ERR(MODULE_MPF, "err:copy_from_user failed!\n");
        return -EFAULT;
    }
    DH_LOG_INFO(MODULE_MPF, "ob->host = 0x%llx, ob->size = 0x%lxs\n", ob.host_addr, ob.size);

    dst_addr = pci_epc_mem_alloc_addr(epc, &dst_phys_addr, ob.size);
    if (!dst_addr)
    {
        DH_LOG_ERR(MODULE_MPF, "Failed to allocate destination address\n");
        return -ENOMEM;
    }

    ret = pci_epc_map_addr(epc, epf->func_no, epf->vfunc_no, dst_phys_addr, ob.host_addr, ob.size);
    if (ret)
    {
        DH_LOG_ERR(MODULE_MPF, "pci_epc_map_addr err!!!\n");
        return ret;
    }
    ob.dpu_vaddr = (unsigned long long)dst_addr;
    ob.dpu_paddr = dst_phys_addr;
    if (copy_to_user((void *)arg, &ob, sizeof(struct ioctl_ob_data)))
    {
        DH_LOG_ERR(MODULE_MPF, "err:copy_to_user failed!\n");
        return -EFAULT;
    }

    return 0;
}

static int vfio_outbound_read(struct mdev_device *mdev, struct mdev_state *mdev_state, unsigned long arg)
{
    struct pci_epf_mdev_dev *epf_mdev_dev = mdev_state->epf_mdev_dev;
    struct pci_epf *epf = epf_mdev_dev->epf;
    struct pci_epc *epc = epf->epc;
    struct pci_ob_rw_data ob_rw_data = {0};

    if (copy_from_user(&ob_rw_data, (void *)arg, sizeof(struct pci_ob_rw_data)))
    {
        DH_LOG_ERR(MODULE_MPF, "err:copy_from_user failed!\n");
        return -EFAULT;
    }
    DH_LOG_INFO(MODULE_MPF, "ob_rw_data.phys_addr = 0x%llx, ob_rw_data.size = 0x%x\n", ob_rw_data.phys_addr, ob_rw_data.size);

    pcie_zte_epc_ob_read(epc, (phys_addr_t)ob_rw_data.phys_addr, ob_rw_data.size, &ob_rw_data.val);

    if (copy_to_user((void *)arg, &ob_rw_data, sizeof(struct pci_ob_rw_data)))
    {
        DH_LOG_ERR(MODULE_MPF, "err:copy_to_user failed!\n");
        return -EFAULT;
    }

    return 0;
}

static int vfio_outbound_clear(struct mdev_device *mdev, struct mdev_state *mdev_state, unsigned long arg)
{
    struct pci_epf_mdev_dev *epf_mdev_dev = mdev_state->epf_mdev_dev;
    struct pci_epf *epf = epf_mdev_dev->epf;
    struct pci_epc *epc = epf->epc;
    struct ioctl_ob_data ob = {0};

    if (copy_from_user(&ob, (void *)arg, sizeof(struct ioctl_ob_data)))
    {
        DH_LOG_ERR(MODULE_MPF, "err:copy_from_user failed!\n");
        return -EFAULT;
    }

    pci_epc_unmap_addr(epc, epf->func_no, epf->vfunc_no, ob.dpu_vaddr);
    pci_epc_mem_free_addr(epc, ob.dpu_paddr, (void *)ob.dpu_vaddr, ob.size);

    return 0;
}

static int vfio_virtio_module_set(unsigned long arg)
{
    struct ioctl_virtio_data virtio_data = {0};

    if (copy_from_user(&virtio_data, (void *)arg, sizeof(struct ioctl_virtio_data)))
    {
        DH_LOG_ERR(MODULE_MPF, "err:copy_from_user failed!\n");
        return -EFAULT;
    }

    return ep_virtio_module_set(virtio_data.ep_id, virtio_data.pf_id, virtio_data.en);
}

static long pci_mdev_ioctl(struct mdev_device *mdev, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct mdev_state *mdev_state;
    struct device *dev = NULL;
    // struct pci_epf *epf = container_of(dev, struct pci_epf, dev);
    // struct pci_epf_mdev_dev *epf_mdev_dev = epf_get_drvdata(epf);

    if (mdev == NULL)
    {
        DH_LOG_ERR(MODULE_MPF, "err: mdev is NULL!\n");
        return -EINVAL;
    }
    dev = mdev->type->parent->dev;

    mdev_state = mdev_get_drvdata(mdev);
    if (!mdev_state)
        return -ENODEV;

    switch (cmd)
    {
        case VFIO_DEVICE_GET_INFO:
            return device_get_info(mdev, mdev_state, arg);
        case VFIO_DEVICE_GET_REGION_INFO:
            return device_get_region_info(mdev, mdev_state, arg);
        case VFIO_DEVICE_IOEVENTFD:
            return vfio_device_ioeventfd(mdev, mdev_state, arg);
        case VFIO_OUTBOUND_SET:
            return vfio_outbound_set(mdev, mdev_state, arg);
        case VFIO_OUTBOUND_CLEAR:
            return vfio_outbound_clear(mdev, mdev_state, arg);
        case VFIO_POWER_RESET:
            ep_power_reset(arg);
            return 0;
        case VFIO_VIRTIO_MODULE_SET:
            return vfio_virtio_module_set(arg);
        case VFIO_LINKUP:
            return is_pcie_ep_link(arg);
        case VFIO_OUTBOUND_READ:
            return vfio_outbound_read(mdev, mdev_state, arg);
        // case VFIO_EP4_LINKUP:
        //     return is_ep4_link_up();
        default:
            DH_LOG_ERR(MODULE_MPF, "zte-pci-epf-mdev ioctl cmd error!\n");
            ret = -ENOTTY;
    }

    return ret;
}
#endif

static const struct mdev_parent_ops mdev_fops = {
    .owner = THIS_MODULE,
    .dev_attr_groups = pci_mdev_dev_groups,
    .mdev_attr_groups = mdev_dev_groups,
    .supported_type_groups = mdev_type_groups,
    .create = pci_mdev_create,
    .remove = pci_mdev_remove,
    .open = pci_mdev_open,
    .release = pci_mdev_close,
    .read = pci_mdev_read,
    .write = pci_mdev_write,
    .mmap = pci_mdev_mmap,
    .ioctl = pci_mdev_ioctl,
};
#endif

static int pci_epf_mdev_probe(struct pci_epf *epf)
{
    struct pci_epf_mdev_dev *epf_mdev_dev;
    struct device *dev = &epf->dev;
    int ret = 0;

    epf_mdev_dev = devm_kzalloc(dev, sizeof(*epf_mdev_dev), GFP_KERNEL);
    if (!epf_mdev_dev)
        return -ENOMEM;

    epf_mdev_dev->epf = epf;
    epf->header = devm_kzalloc(dev, sizeof(struct pci_epf_header), GFP_KERNEL);
    if (!epf->header)
    {
        ret = -ENOMEM;
        goto err;
    }

    epf_mdev_dev->created_flag = 0;

    // DMA一致性设置（未必有效，待查）
    dev->coherent_dma_mask = ~((u64)0x0);

    epf_set_drvdata(epf, epf_mdev_dev);

    ret = mdev_register_device(dev, &mdev_fops);
    if (ret)
    {
        DH_LOG_ERR(MODULE_MPF, "mdev_register_device failed %d\n", ret);
        goto err;
    }

    return 0;

err:
    devm_kfree(dev, epf->header);
    devm_kfree(dev, epf_mdev_dev);
    return ret;
}

static int pci_epf_remove(struct pci_epf *epf)
{
    struct device *dev = &epf->dev;
    struct pci_epf_mdev_dev *epf_mdev_dev = epf_get_drvdata(epf);

    mdev_unregister_device(&epf->dev);
    if (epf->header)
    {
        devm_kfree(dev, epf->header);
    }
    if (epf_mdev_dev)
    {
        devm_kfree(dev, epf_mdev_dev);
    }

    return 0;
}

static struct pci_epf_driver pci_epf_mdev_driver = {
    .driver.name = "pci-epf-mdev",
    .probe = pci_epf_mdev_probe,
    .remove = pci_epf_remove,
    .id_table = pci_epf_dev_ids,
    .ops = &epf_mdev_ops,
    .owner = THIS_MODULE,
};

static int __init pci_epf_mdev_init(void)
{
    int ret = 0;

    ret = pci_epf_register_driver(&pci_epf_mdev_driver);
    if (ret)
    {
        pr_err("Failed to register pci epf test driver --> %d\n", ret);
        return ret;
    }

    DH_LOG_ERR(MODULE_MPF, "zte_epf driver regist successful\n");
    return ret;
}
module_init(pci_epf_mdev_init);

static void __exit pci_epf_mdev_exit(void)
{
    pci_epf_unregister_driver(&pci_epf_mdev_driver);
}
module_exit(pci_epf_mdev_exit);

MODULE_DESCRIPTION("PCI EPF MDEV DRIVER");
MODULE_AUTHOR("ZTE");
MODULE_LICENSE("GPL v2");

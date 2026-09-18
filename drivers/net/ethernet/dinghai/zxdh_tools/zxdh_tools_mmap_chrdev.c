#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/mutex.h>
#include <linux/dinghai/kcompat.h>
#include "zxdh_tools_mmap_chrdev.h"

#define DEVICE_NAME "dhtool_mmap"
#define CLASS_NAME "dhtool_mmap"
static int DEV_MAJOR = 0;
static dev_t G_DEV;
static struct class *G_CHAR_CLASS;
static struct device *G_CHAR_DEVICE;
static struct cdev G_CDEV;
static unsigned int G_DEV_COUNT = 1;
static int G_CHAR_DEV_INIT_SUCC = 0;
static DEFINE_MUTEX(G_PCI_SEARCH_LOCK);

struct check_bar_paras {
    u16 vendor_id;
    u16 device_id;
    unsigned long size;
    phys_addr_t offset;
    unsigned long bar_start_addr;
    unsigned long bar_size;
};

static int dhtool_dev_open(struct inode *inode, struct file *file)
{
    DHTOOLS_CHRDEV_LOG_INFO("[Dhtool Char Device] Device opened\n");
    return 0;
}

static int dhtool_dev_release(struct inode *inode, struct file *file)
{
    DHTOOLS_CHRDEV_LOG_INFO("[Dhtool Char Device] Device released\n");
    return 0;
}

static int check_bar(int bar_no, struct vm_area_struct *vma, struct pci_dev *dev, struct check_bar_paras *check_bar_paras_obj)
{
    int find_flag = 0;
    u16 vendor_id = check_bar_paras_obj->vendor_id;
    u16 device_id = check_bar_paras_obj->device_id;
    phys_addr_t offset = check_bar_paras_obj->offset;
    unsigned long size = check_bar_paras_obj->size;
    unsigned long bar_start_addr = check_bar_paras_obj->bar_start_addr;
    unsigned long bar_size = check_bar_paras_obj->bar_size;
    DHTOOLS_CHRDEV_LOG_INFO(
        "[Dhtool Char Device] dev_mmap: PCI device found, device_id: 0x%x, vendor_id: 0x%x, BDF: %04x:%02x:%02x.%x, bar_no: %d\n",
        device_id, vendor_id, pci_domain_nr(dev->bus), dev->bus->number, PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn), bar_no);
    // DHTOOLS_CHRDEV_LOG_INFO("[Dhtool Char Device] dev_mmap: PCI dev %04x:%04x refcount=%d, bar_no: %d\n",
    //     dev->vendor, dev->device,
    //     kref_read(&dev->dev.kobj.kref), bar_no);
    if (bar_start_addr == 0 || bar_size == 0) {
        DHTOOLS_CHRDEV_LOG_ERR(
            "[Dhtool Char Device] dev_mmap: some BAR parameter is 0, BAR address: 0x%lx, BAR size: 0x%lx, device_id: 0x%x, vendor_id: 0x%x, bar_no: %d\n",
            bar_start_addr, bar_size, device_id, vendor_id, bar_no);
        return find_flag;
    }
    // cherk BAR parameter
    if ((offset < bar_start_addr) || ((offset + (phys_addr_t)size) > (bar_start_addr + bar_size)))
    {
        DHTOOLS_CHRDEV_LOG_INFO(
            "[Dhtool Char Device] dev_mmap: check BAR parameter not matched, "
            "device_id: 0x%x, vendor_id: 0x%x, "
            "offset: %llu, bar_start_addr: %lu (0x%lx), "
            "offset + size: %llu, bar_start_addr + bar_size: %lu, "
            "bar_size: 0x%lx, vm_end - vm_start: 0x%lx, "
            "vm_end: %lu, vm_start: %lu, vm_pgoff: %lu, bar_no: %d\n",
            device_id, vendor_id,
            offset, bar_start_addr, bar_start_addr,
            offset + (phys_addr_t)size, bar_start_addr + bar_size,
            bar_size, size,
            vma->vm_start, vma->vm_end, vma->vm_pgoff, bar_no);
        return find_flag;
    } else {
        find_flag = 1;
        DHTOOLS_CHRDEV_LOG_INFO(
            "[Dhtool Char Device] dev_mmap: check BAR parameter success, "
            "device_id: 0x%x, vendor_id: 0x%x, "
            "offset: %llu, bar_start_addr: %lu (0x%lx), "
            "offset + size: %llu, bar_start_addr + bar_size: %lu, "
            "bar_size: 0x%lx, vm_end - vm_start: 0x%lx, "
            "vm_end: %lu, vm_start: %lu, vm_pgoff: %lu, bar_no: %d\n",
            device_id, vendor_id,
            offset, bar_start_addr, bar_start_addr,
            offset + (phys_addr_t)size, bar_start_addr + bar_size,
            bar_size, size,
            vma->vm_start, vma->vm_end, vma->vm_pgoff, bar_no);
        return find_flag;
    }
}

extern const struct pci_device_id dh_pf_pci_table[];
static int dhtool_dev_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long size = vma->vm_end - vma->vm_start;
    struct pci_dev *dev = NULL;
    struct pci_dev *next_dev = NULL;
    u16 device_id = 0;
    int find_flag = 0;
    u16 vendor_id = 0;
    const struct pci_device_id *entry = NULL;
    phys_addr_t offset = vma->vm_pgoff << PAGE_SHIFT;
    int check_bar_no_array[] = {0, 2, 4};
    int check_bar_no_num = sizeof(check_bar_no_array) / sizeof(int);
    int check_succ_flag = 0;
    struct check_bar_paras check_bar_paras_data = {0};
    int bar_no = 0;
    int index = 0;
    unsigned long bar_start_addr = 0;
    unsigned long bar_size = 0;
    DHTOOLS_CHRDEV_LOG_INFO(
        "[Dhtool Char Device] dev_mmap: Begin, size: %lu, vm_start: %lu, vm_end: %lu, vm_pgoff: %lu, "
        "offset: %llu, offset + (phys_addr_t)size - 1: %llu, vm_flags: %lu, PAGE_SHIFT: %d, check_bar_no_num: %d.\n",
        size, vma->vm_start, vma->vm_end, vma->vm_pgoff, offset, offset + (phys_addr_t)size - 1, vma->vm_flags, PAGE_SHIFT, check_bar_no_num);
    if (offset + (phys_addr_t)size - 1 < offset) {
        DHTOOLS_CHRDEV_LOG_ERR(
            "[Dhtool Char Device] dev_mmap: check parameter failed, offset + (phys_addr_t)size - 1: %llu < offset: %llu.\n",
            offset + (phys_addr_t)size - 1, offset);
        return -EINVAL;
    }
    if (vma->vm_end <= vma->vm_start) {
        DHTOOLS_CHRDEV_LOG_ERR("[Dhtool Char Device] dev_mmap: vm_end: %lu <= vm_start: %lu, failed.\n", vma->vm_end, vma->vm_start);
        return -EINVAL;
    }
    if (size <= 0) {
        DHTOOLS_CHRDEV_LOG_ERR("[Dhtool Char Device] dev_mmap: check size: %lu <= 0, failed.\n", size);
        return -EINVAL;
    }

    // check dh nic bar addr and size
    entry = dh_pf_pci_table;
    if (!entry) {
        DHTOOLS_CHRDEV_LOG_ERR("[Dhtool Char Device] dev_mmap: dh_pf_pci_table is NULL, skip\n");
        return -EINVAL;
    }
    mutex_lock(&G_PCI_SEARCH_LOCK);
    for (; (entry != NULL) && ((entry->vendor != 0) || (entry->device != 0)); entry++) {
        device_id = entry->device;
        vendor_id = entry->vendor;
        dev = pci_get_device(vendor_id, device_id, NULL);
        if (!dev) {
            continue;
        }
        check_bar_paras_data.vendor_id = vendor_id;
        check_bar_paras_data.device_id = device_id;
        check_bar_paras_data.size = size;
        check_bar_paras_data.offset = offset;
        while (dev) {
            for(index = 0; index < check_bar_no_num; index++) {
                bar_no = check_bar_no_array[index];
                if (bar_no < 0 || bar_no > 5) {
                    DHTOOLS_CHRDEV_LOG_ERR("[Dhtool Char Device] dev_mmap: Invalid BAR index %d (max %d)\n", bar_no, 5);
                    continue;
                }
                bar_start_addr = pci_resource_start(dev, bar_no);
                bar_size = pci_resource_len(dev, bar_no);
                check_bar_paras_data.bar_start_addr = bar_start_addr;
                check_bar_paras_data.bar_size = bar_size;
                check_succ_flag = check_bar(bar_no, vma, dev, &check_bar_paras_data);
                if (check_succ_flag == 1) {
                    find_flag = 1;
                    break;
                }
            }
            if (find_flag == 1) {
                break;
            }
            next_dev = pci_get_device(vendor_id, device_id, dev);
            dev = next_dev;
        }
        if (find_flag == 1) {
            break;
        }
    }
    if (dev) {
        pci_dev_put(dev);
    }
    mutex_unlock(&G_PCI_SEARCH_LOCK);
    if (find_flag == 0) {
        DHTOOLS_CHRDEV_LOG_ERR("[Dhtool Char Device] dev_mmap: finish search, check BAR parameter failed.\n");
        return -EINVAL;
    }
    // process mmap
    #if defined(ZXDH_ADAPT_6_6) || defined(USE_FLAG_SET_FUNC) || defined(HAVE_VM_FLAGS_SET)
        vm_flags_set(vma, VM_IO | VM_DONTEXPAND | VM_DONTDUMP);
    #else
        vma->vm_flags |= VM_IO | VM_DONTEXPAND | VM_DONTDUMP;
    #endif
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    DHTOOLS_CHRDEV_LOG_INFO(
        "[Dhtool Char Device] dev_mmap: before remap_pfn_range, size: %lu, start: %lu, end: %lu, vm_pgoff: %lu, vm_flags: %lu.\n",
        size, vma->vm_start, vma->vm_end, vma->vm_pgoff, vma->vm_flags);
    if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, size, vma->vm_page_prot)) {
        DHTOOLS_CHRDEV_LOG_ERR("[Dhtool Char Device] dev_mmap: remap_pfn_range failed.\n");
        return -EAGAIN;
    }
    DHTOOLS_CHRDEV_LOG_INFO(
        "[Dhtool Char Device] dev_mmap: End, size: %lu, start: %lu, end: %lu, vm_flags: %lu.\n",
        size, vma->vm_start, vma->vm_end, vma->vm_flags);
    return 0;
}

static struct file_operations dev_fops = {
    .owner = THIS_MODULE,
    .open = dhtool_dev_open,
    .release = dhtool_dev_release,
    .mmap = dhtool_dev_mmap,
};

//NOTE(), this only called once when module init
int __init dhtool_char_init(void)
{
    int result = 0;
    int origin_char_dev_init_succ_flag = G_CHAR_DEV_INIT_SUCC;
    G_CHAR_DEV_INIT_SUCC = 0;
    DHTOOLS_CHRDEV_LOG_INFO(
        "[Dhtool Char Device] char_init: Begin init char device, origin G_CHAR_DEV_INIT_SUCC: %d, after set G_CHAR_DEV_INIT_SUCC: %d.\n",
        origin_char_dev_init_succ_flag, G_CHAR_DEV_INIT_SUCC);
    // s1: allocate char device
    result = alloc_chrdev_region(&G_DEV, 0, G_DEV_COUNT, DEVICE_NAME);
    if (result < 0) {
        DHTOOLS_CHRDEV_LOG_ERR("[Dhtool Char Device] char_init: alloc_chrdev_region failed.\n");
        return result;
    }
    DEV_MAJOR = MAJOR(G_DEV);
    DHTOOLS_CHRDEV_LOG_INFO("[Dhtool Char Device] char_init: alloc_chrdev_region successed, major: %d.\n", DEV_MAJOR);
    // s2: create class
#if defined(CLASS_CREATE_NO_MODULE) || defined(HAVE_NO_MODULE_PARAM)
    G_CHAR_CLASS = class_create(CLASS_NAME);
#else
    G_CHAR_CLASS = class_create(THIS_MODULE, CLASS_NAME);
#endif
    if (IS_ERR(G_CHAR_CLASS)) {
        result = PTR_ERR(G_CHAR_CLASS);
        DHTOOLS_CHRDEV_LOG_ERR("[Dhtool Char Device] char_init: class_create failed, result: %d.\n", result);
        unregister_chrdev_region(G_DEV, G_DEV_COUNT);
        return result;
    }
    DHTOOLS_CHRDEV_LOG_INFO("[Dhtool Char Device] char_init: class_create successed, major: %d.\n", DEV_MAJOR);
    // s3: create device
    G_CHAR_DEVICE = device_create(G_CHAR_CLASS, NULL, G_DEV, NULL, DEVICE_NAME);
    if (IS_ERR(G_CHAR_DEVICE)) {
        result = PTR_ERR(G_CHAR_DEVICE);
        DHTOOLS_CHRDEV_LOG_ERR("[Dhtool Char Device] char_init: device_create failed, result: %d.\n", result);
        class_destroy(G_CHAR_CLASS);
        unregister_chrdev_region(G_DEV, G_DEV_COUNT);
        return result;
    }
    DHTOOLS_CHRDEV_LOG_INFO("[Dhtool Char Device] char_init: device_create successed, major: %d.\n", DEV_MAJOR);
    // s4: init cdev and add it
    cdev_init(&G_CDEV, &dev_fops);
    G_CDEV.owner = THIS_MODULE;
    result = cdev_add(&G_CDEV, G_DEV, G_DEV_COUNT);
    if (result < 0) {
        DHTOOLS_CHRDEV_LOG_ERR("[Dhtool Char Device] char_init: cdev_add failed, result: %d.\n", result);
        device_destroy(G_CHAR_CLASS, G_DEV);
        class_destroy(G_CHAR_CLASS);
        unregister_chrdev_region(G_DEV, G_DEV_COUNT);
        return result;
    }
    DHTOOLS_CHRDEV_LOG_INFO("[Dhtool Char Device] char_init: cdev_add successed, major: %d.\n", DEV_MAJOR);
    G_CHAR_DEV_INIT_SUCC = 1;
    DHTOOLS_CHRDEV_LOG_INFO("[Dhtool Char Device] char_init: End init char device, G_CHAR_DEV_INIT_SUCC: %d.\n", G_CHAR_DEV_INIT_SUCC);
    return 0;
}

//NOTE(), this only called once when module exit
void dhtool_char_exit(void)
{
    DHTOOLS_CHRDEV_LOG_INFO("[Dhtool Char Device] char_exit: Begin unregistered char device, G_CHAR_DEV_INIT_SUCC: %d.\n", G_CHAR_DEV_INIT_SUCC);
    if (G_CHAR_DEV_INIT_SUCC == 0) {
        DHTOOLS_CHRDEV_LOG_ERR("[Dhtool Char Device] char_exit: End skip unregistered char device, because G_CHAR_DEV_INIT_SUCC: %d.\n", G_CHAR_DEV_INIT_SUCC);
        return;
    }
    device_destroy(G_CHAR_CLASS, G_DEV);
    DHTOOLS_CHRDEV_LOG_INFO("[Dhtool Char Device] char_exit: finish device_destroy.\n");
    class_destroy(G_CHAR_CLASS);
    DHTOOLS_CHRDEV_LOG_INFO("[Dhtool Char Device] char_exit: finish class_destroy.\n");
    cdev_del(&G_CDEV);
    DHTOOLS_CHRDEV_LOG_INFO("[Dhtool Char Device] char_exit: finish cdev_del.\n");
    unregister_chrdev_region(G_DEV, G_DEV_COUNT);
    DHTOOLS_CHRDEV_LOG_INFO("[Dhtool Char Device] char_exit: finish unregister_chrdev_region.\n");
    DHTOOLS_CHRDEV_LOG_INFO("[Dhtool Char Device] char_exit: End unregistered char device.\n");
}

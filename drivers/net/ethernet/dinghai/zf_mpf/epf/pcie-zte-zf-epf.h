#ifndef __ZTE_EPF_H
#define __ZTE_EPF_H

// #include <linux/version.h>

#include <linux/cdev.h>
#include <linux/crc32.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/eventfd.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/iommu.h>
#include <linux/kernel.h>
#include <linux/mdev.h>
#include <linux/module.h>
#include <linux/pci-epc.h>
#include <linux/pci-epf.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>
#include <linux/pci_regs.h>
#include <linux/poll.h>
#include <linux/random.h>
#include <linux/sched.h>
#include <linux/serial.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/uuid.h>
#include <linux/vfio.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <uapi/linux/serial_reg.h>

#include "../epc/pcie-zte-zf-epc.h"

#define EPF_MDEV_IOEVENTFD_MAX 20

/* ioctl cmd */
#define VFIO_OUTBOUND_SET      _IO(VFIO_TYPE, VFIO_BASE + 30)
#define VFIO_OUTBOUND_CLEAR    _IO(VFIO_TYPE, VFIO_BASE + 31)
#define VFIO_POWER_RESET       _IO(VFIO_TYPE, VFIO_BASE + 32)
#define VFIO_VIRTIO_MODULE_SET _IO(VFIO_TYPE, VFIO_BASE + 33)
#define VFIO_LINKUP            _IO(VFIO_TYPE, VFIO_BASE + 34)
#define VFIO_OUTBOUND_READ     _IO(VFIO_TYPE, VFIO_BASE + 35)
// #define VFIO_EP4_LINKUP                         _IO(VFIO_TYPE, VFIO_BASE + 35)

extern int is_pcie_ep_link(int ep_id);
// extern int is_ep4_link_up(void);
extern void ep_power_reset(int ep_id);
extern int ep_virtio_module_set(int ep_id, int pf_id, int en);

struct ioctl_virtio_data {
    int ep_id;
    int pf_id;
    int en;
};

struct ioctl_ob_data {
    unsigned long long dpu_paddr;
    unsigned long long dpu_vaddr;
    unsigned long long host_addr;
    unsigned long size;
};

struct pci_epf_mdev_dev {
    struct pci_epf *epf;
    enum pci_barno epf_barno;
    size_t msix_table_offset;
    const struct pci_epc_features *epc_features;
    int created_flag; // 0:Not created, 1:created
    void *pf_bar_vaddr[PCI_STD_NUM_BARS + 1];
    void *vf_bar_vaddr[PCI_STD_NUM_BARS];
};

struct epf_mdev_ioeventfd {
    struct list_head next;
    struct mdev_state *mdev_state;
    struct virqfd *virqfd;
    u64 data;
    loff_t pos;
    u64 offset;
    int count;
};

struct mdev_region_info {
    u64 start;
    u64 phys_start;
    u32 size;
    u64 vfio_offset;
    u32 argsz;
};

struct mdev_state {
    int irq_fd;
    struct eventfd_ctx *intx_evtfd;
    struct eventfd_ctx *msi_evtfd;
    struct eventfd_ctx *msix_evtfd;
    int irq_index;
    struct mutex ops_lock;
    struct mdev_device *mdev;
    struct mdev_region_info region_info[VFIO_PCI_NUM_REGIONS];
    u32 bar_mask[VFIO_PCI_NUM_REGIONS];
    struct list_head next;
    struct vfio_device_info dev_info;
    struct pci_epf_mdev_dev *epf_mdev_dev;
    struct mutex ioeventfds_lock;
    struct list_head ioeventfds_list;
    int ioeventfds_nr;
};

// mdev_private struct
struct mdev_parent {
    struct device *dev;
    const struct mdev_parent_ops *ops;
    struct kref ref;
    struct list_head next;
    struct kset *mdev_types_kset;
    struct list_head type_list;
    struct rw_semaphore unreg_sem;
};

struct mdev_type {
    struct kobject kobj;
    struct kobject *devices_kobj;
    struct mdev_parent *parent;
    struct list_head next;
    unsigned int type_group_id;
};
// mdev_private struct end

struct pci_ob_rw_data {
    unsigned long long phys_addr;
    unsigned int size;
    unsigned int val;
};

#endif
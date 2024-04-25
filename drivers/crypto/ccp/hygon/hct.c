// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2022 HYGON Corporation . All rights reserved.
 */

#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/wait.h>
#include <linux/uuid.h>
#include <linux/vfio.h>
#include <linux/iommu.h>
#include <linux/sysfs.h>
#include <linux/ctype.h>
#include <linux/file.h>
#include <linux/pagemap.h>
#include <linux/pci.h>
#include <linux/kfifo.h>
#include <linux/eventfd.h>
#include <linux/mem_encrypt.h>
#include <asm/cpuid.h>
#if IS_ENABLED(CONFIG_VFIO_MDEV)
#include <linux/mdev.h>
#endif

/**
 * VERSION_STRING modification instructions:
 * 0.1 -- support hct/mdev mode.
 * 0.2 -- supoort qemu virtualization.
 * 0.3 -- support host-noiommu mode memory encryption function,
 *        and performance optimization in virtual machines (enable caching).
 * 0.4 -- support compiling hct.ko when mdev module is disabled.
 */

#undef  pr_fmt
#define pr_fmt(fmt)				"hct: " fmt

#define VERSION_STRING				"0.4"
#define DRIVER_AUTHOR				"HYGON Corporation"
#define VERSION_SIZE				16

#define MCCP_CLASS_NAME			"hct"
#define MCCP_NAME				"hct"
#define MCCP_STRING_LEN				16

#define MCCP_CONFIG_SPACE_SIZE			0xff

#define MCCP_VFIO_PCI_OFFSET_SHIFT		40
#define MCCP_VFIO_PCI_OFFSET_TO_INDEX(off) \
				(off >> MCCP_VFIO_PCI_OFFSET_SHIFT)
#define MCCP_VFIO_PCI_INDEX_TO_OFFSET(index) \
				((u64)(index) << MCCP_VFIO_PCI_OFFSET_SHIFT)
#define MCCP_VFIO_PCI_OFFSET_MASK \
				(((u64)(1) << MCCP_VFIO_PCI_OFFSET_SHIFT) - 1)
#define vdev_to_mdev_state(vdev)  \
				container_of((vdev), struct mdev_state, vdev)

#define MCCP_SHARE_IOC_TYPE			'C'
#define MCCP_SHARE_OP				0x01
#define MCCP_SHARE_OP_DMA_MAP			0x01
#define MCCP_SHARE_OP_DMA_UNMAP_ALL		0x02
#define MCCP_SHARE_OP_GET_ID			0x03
#define MCCP_SHARE_OP_GET_PASID			0x04
#define MCCP_SHARE_OP_DMA_UNMAP			0x05
#define MCCP_SHARE_OP_GET_VERSION		0x06

#define MCCP_NOIOMMU_IOC_TYPE			MCCP_SHARE_IOC_TYPE
#define MCCP_NOIOMMU_OP				MCCP_SHARE_OP
#define MCCP_NOIOMMU_SET_MEMORY_WB		0x01
#define MCCP_NOIOMMU_GET_SME_ACTIVE		0x02

#define MCCP_SHARE_IOMMU_MAGIC			0x3d6a9c5728633b9e

#define PCI_RESOURCE_BAR2			2
#define MCCP_DEV_ID_SIZE			8

/* fixed iova range for ccp dma. */
#define MCCP_DMA_IOVA_OFFSET			0
#define MCCP_DMA_IOVA_SIZE			(1ul << 30)

#define MCCP_INSTANCE_MAX			1024
#define MCCP_INSTANCE_OFFSET			8
#define MCCP_INSTANCE_MASK			(~((1u << MCCP_INSTANCE_OFFSET) - 1))
#define MCCP_PASID_SIZE                         (1 << 8)
#define MCCP_IOVA_MAX_SLOT			1024
#define MCCP_DEV_MAX				16
#define MCCP_DEV_QUEUE_MAX			8
#define MCCP_DEV_QUEUE                          5
#define MCCP_QUEUES_MAX				(MCCP_DEV_MAX * MCCP_DEV_QUEUE_MAX)
#define MCCP_QUEUE_NEED_INIT			0x01
#define MCCP_SHARED_SIZE                        (MCCP_DEV_MAX * PAGE_SIZE)

#define MCCP_MSIX_ENTRY_SIZE                    2
#define MCCP_NTB_VECTOR_NUM                     1
#define MCCP_PSP_VECTOR_NUM                     2
#define MCCP_GET_QUEUE_FLAG                     (0x55)
#define MCCP_PUT_QUEUE_FLAG                     (0xAA)
#define IRQ_EVENT_SIGNAL                        (1UL)
#define IRQ_EVENT_SIGFAL                        (0xFF)

#define Q_MASK_REG                              0x0000
#define MCMD_Q_STATUS_INCR                      0x1000
#define MCMD_Q_TAIL_LO_BASE                     0x0004
#define MCMD_Q_HEAD_LO_BASE                     0x0008
#define MCMD_Q_INT_ENABLE_BASE                  0x000C
#define MCMD_Q_INTERRUPT_STATUS_BASE            0x0010
#define MCMD_Q_STATUS_BASE                      0x0100
#define MCMD_Q_INT_STATUS_BASE                  0x0104

#define INT_COMPLETION                          0x1
#define INT_ERROR                               0x2
#define INT_QUEUE_STOPPED                       0x4
#define INT_EMPTY_QUEUE                         0x8
#define SUPPORTED_INTERRUPTS                    (INT_COMPLETION | INT_ERROR)
#define MCMD_Q_ERROR(__qs)                      ((__qs) & 0x0000003f)

#define PHY_ADDR_MASK                           0x7FFFFFFFFFFF

struct hct_shared_cfg {
	unsigned int iova_slot[MCCP_IOVA_MAX_SLOT];
	unsigned int ccp_queue_state[MCCP_QUEUES_MAX];
	unsigned int ccps_ref[MCCP_DEV_MAX];
	unsigned int ccps_ref_lock;
	int rsvd1[15];
	u64 qidx[MCCP_QUEUES_MAX];
	unsigned int ccp_state[MCCP_DEV_MAX];
} __aligned(PAGE_SIZE);

struct hct_dev_ctrl {
	unsigned char op;
	unsigned char rsvd[3];
	union {
		unsigned char version[VERSION_SIZE];
		unsigned int id;
		unsigned long sme_mask;
		struct {
			unsigned long vaddr;
			unsigned long iova;
			unsigned long size;
		};
		struct {
			unsigned long vt_addr;
			unsigned int nr_pages;
		};
	};
};

struct hct_dma {
	struct list_head next;
	unsigned long vaddr;
	unsigned long iova;
	size_t size;
	struct page **pages;
	unsigned long npages;
	unsigned int pfnmap_flag;
};

/* record the register address related to interrupt */
struct hct_cmd_queue {
	void __iomem *reg_control;
	void __iomem *reg_tail_lo;
	void __iomem *reg_head_lo;
	void __iomem *reg_int_enable;
	void __iomem *reg_interrupt_status;
	void __iomem *reg_status;
	void __iomem *reg_int_status;
	struct mutex q_lock;
	DECLARE_KFIFO_PTR(ectx_fifo, struct eventfd_ctx *);
} ____cacheline_aligned;

struct hct_dev_ctx {
	struct hct_cmd_queue cmd_q[MCCP_DEV_QUEUE_MAX];
	struct tasklet_struct irq_tasklet;
	char devname[MCCP_STRING_LEN];
	void __iomem *io_regs; /* for BAR2 memory address */
	u32 q_count;
	int irq;
} ____cacheline_aligned;

struct hct_iommu {
	unsigned long magic;
	struct mutex lock;
	struct pci_dev *pdev;
	struct hct_dev_ctx dev_ctx;
	unsigned long id;
	unsigned long ref;
};

#if IS_ENABLED(CONFIG_VFIO_MDEV)
static struct hct_data {
	struct hct_iommu iommu[MCCP_DEV_MAX];
	struct mutex lock;
	unsigned long bitmap;
	struct iommu_domain *domain;
	int prot;
	dma_addr_t dma_share_iova;
	size_t dma_share_size;
	unsigned long dma_share_ref;
	unsigned long mdev_ref;
	unsigned long ids[BITS_TO_LONGS(MCCP_INSTANCE_MAX)];
} hct_data;

static struct hct_share_cfg {
	long ref;
	struct mutex lock;
	struct page *pages[MCCP_DEV_MAX];
	u64 pagecount;
	void *vaddr;
	u64 size;
} hct_share;

static struct hct_dev {
	dev_t vd_devt;
	struct class *vd_class;
	struct cdev vd_cdev;
	struct device dev;
	struct mdev_parent mdev_parent;
} hct_dev;

struct mdev_region_info {
	u64 start;
	u64 phys_start;
	u32 size;
	u64 vfio_offset;
};

struct mdev_state {
	struct vfio_device vdev;
	struct mutex ops_lock;
	struct mdev_device *mdev;
	struct hct_iommu *iommu;
	struct mdev_region_info region_info[VFIO_PCI_NUM_REGIONS];
	struct list_head next;
	struct vfio_device_info dev_info;
	unsigned long ref;
	struct eventfd_ctx *trigger[MCCP_DEV_QUEUE_MAX];
	u8 efd_start;
	u8 efd_count;
};

struct mdev_type hct_mdev_type = {
	.sysfs_name = "1",
	.pretty_name = "hct mdev type"
};
struct mdev_type *hct_mdev_types[] = {
	&hct_mdev_type
};

static void hct_cmd_queue_enable_interrupt(struct hct_dev_ctx *dev_ctx)
{
	unsigned int i;

	for (i = 0; i < dev_ctx->q_count; i++)
		iowrite32(SUPPORTED_INTERRUPTS, dev_ctx->cmd_q[i].reg_int_enable);
}

static void hct_cmd_queue_disable_interrupt(struct hct_dev_ctx *dev_ctx)
{
	unsigned int i;

	for (i = 0; i < dev_ctx->q_count; i++)
		iowrite32(0x00, dev_ctx->cmd_q[i].reg_int_enable);
}

static void hct_cmd_queue_intr_task(unsigned long data)
{
	struct hct_dev_ctx *dev_ctx = (struct hct_dev_ctx *)data;
	u32 i, err, status;

	hct_cmd_queue_disable_interrupt(dev_ctx);

	for (i = 0; i < dev_ctx->q_count; i++) {
		struct hct_cmd_queue *cmd_q = &dev_ctx->cmd_q[i];
		struct eventfd_ctx *trigger;

		status = ioread32(cmd_q->reg_interrupt_status);
		if (status) {
			if (status & INT_ERROR) {
				/* print interrupt numbers for debug */
				err = ioread32(cmd_q->reg_status);
				pr_err("Irq fail, errcode = %d.\n", MCMD_Q_ERROR(err));
				while (kfifo_get(&cmd_q->ectx_fifo, &trigger))
					eventfd_signal(trigger, IRQ_EVENT_SIGFAL);
			} else {
				while (kfifo_get(&cmd_q->ectx_fifo, &trigger))
					eventfd_signal(trigger, IRQ_EVENT_SIGNAL);
			}

			iowrite32(status, cmd_q->reg_interrupt_status);
		}
	}

	hct_cmd_queue_enable_interrupt(dev_ctx);
}

static irqreturn_t hct_cmd_queue_intr_handler(int irq, void *arg)
{
	struct hct_dev_ctx *dev_ctx = (struct hct_dev_ctx *)arg;

	tasklet_schedule(&dev_ctx->irq_tasklet);
	return IRQ_HANDLED;
}

static int hct_dev_cmd_queue_init(struct pci_dev *pdev, struct hct_dev_ctx *dev_ctx, int idx)
{
	struct hct_cmd_queue *cmd_q;
	unsigned long addr, len;
	unsigned int retval, qmr;
	int i, ret;

	if (!pdev || !dev_ctx)
		return -EINVAL;

	memset(dev_ctx, 0, sizeof(*dev_ctx));

	ret = pci_enable_device(pdev);
	if (ret)
		return -EINVAL;

	addr = pci_resource_start(pdev, PCI_RESOURCE_BAR2);
	len = pci_resource_len(pdev, PCI_RESOURCE_BAR2);
	dev_ctx->io_regs = ioremap(addr, len);
	if (!dev_ctx->io_regs)
		return -ENOMEM;

	pci_set_master(pdev);
	retval = pci_alloc_irq_vectors(pdev, 1, MCCP_MSIX_ENTRY_SIZE, PCI_IRQ_MSIX);
	if (retval != MCCP_NTB_VECTOR_NUM && retval != MCCP_PSP_VECTOR_NUM)
		return -ENOMEM;

	snprintf(dev_ctx->devname, MCCP_STRING_LEN, "hct-ccp-%d", idx);
	dev_ctx->irq = pci_irq_vector(pdev, retval - 1);
	/* To request_irq, the fourth parameter dev_name must be global
	 * variable or static variable.
	 */
	ret = request_irq(dev_ctx->irq, hct_cmd_queue_intr_handler, 0, dev_ctx->devname, dev_ctx);
	if (ret) {
		pci_free_irq_vectors(pdev);
		dev_ctx->irq = 0;
		return ret;
	}

	tasklet_init(&dev_ctx->irq_tasklet, hct_cmd_queue_intr_task, (unsigned long)dev_ctx);

	qmr = ioread32(dev_ctx->io_regs + Q_MASK_REG);
	if (qmr == 0) {
		iowrite32(0x1f, dev_ctx->io_regs + Q_MASK_REG);
		qmr = ioread32(dev_ctx->io_regs + Q_MASK_REG);
	}
	for (i = 0; i < MCCP_DEV_QUEUE_MAX; i++) {
		if (!(qmr & (1 << i)))
			continue;

		cmd_q = &dev_ctx->cmd_q[dev_ctx->q_count++];

		mutex_init(&cmd_q->q_lock);
		ret = kfifo_alloc(&cmd_q->ectx_fifo, MCCP_INSTANCE_MAX, GFP_KERNEL);
		if (ret)
			return -ENOMEM;

		cmd_q->reg_control = dev_ctx->io_regs + MCMD_Q_STATUS_INCR * (i + 1);
		cmd_q->reg_tail_lo = cmd_q->reg_control + MCMD_Q_TAIL_LO_BASE;
		cmd_q->reg_head_lo = cmd_q->reg_control + MCMD_Q_HEAD_LO_BASE;
		cmd_q->reg_int_enable = cmd_q->reg_control + MCMD_Q_INT_ENABLE_BASE;
		cmd_q->reg_interrupt_status = cmd_q->reg_control + MCMD_Q_INTERRUPT_STATUS_BASE;
		cmd_q->reg_status = cmd_q->reg_control + MCMD_Q_STATUS_BASE;
		cmd_q->reg_int_status = cmd_q->reg_control + MCMD_Q_INT_STATUS_BASE;
	}

	return (dev_ctx->q_count >= 0) ? 0 : -1;
}

static int hct_iommu_alloc(struct pci_dev *pdev)
{
	unsigned long i;
	int ret = -EINVAL;

	mutex_lock(&hct_data.lock);

	i = find_first_zero_bit(&hct_data.bitmap, MCCP_DEV_MAX);
	if (i != MCCP_DEV_MAX)
		bitmap_set(&hct_data.bitmap, i, 1);

	if (device_iommu_capable(&pdev->dev, IOMMU_CAP_CACHE_COHERENCY))
		hct_data.prot |= IOMMU_CACHE;

	mutex_unlock(&hct_data.lock);

	if (i == MCCP_DEV_MAX)
		return -EINVAL;

	ret = iommu_attach_device(hct_data.domain, &pdev->dev);
	if (ret) {
		mutex_lock(&hct_data.lock);
		bitmap_clear(&hct_data.bitmap, i, 1);
		mutex_unlock(&hct_data.lock);
	} else {
		mutex_lock(&hct_data.iommu[i].lock);
		hct_data.iommu[i].pdev = pdev;
		hct_data.iommu[i].id = i;
		hct_data.iommu[i].ref = 0;
		hct_data.iommu[i].magic = MCCP_SHARE_IOMMU_MAGIC;
		pci_set_drvdata(pdev, &hct_data.iommu[i]);

		ret = hct_dev_cmd_queue_init(pdev, &hct_data.iommu[i].dev_ctx, i);
		mutex_unlock(&hct_data.iommu[i].lock);
	}

	return ret;
}

static void hct_iommu_free(struct hct_iommu *iommu)
{
	struct iommu_domain *domain;

	if (!iommu || iommu->magic != MCCP_SHARE_IOMMU_MAGIC)
		return;

	domain = iommu_get_domain_for_dev(&iommu->pdev->dev);

	mutex_lock(&iommu->lock);
	if (iommu->pdev && domain == hct_data.domain)
		iommu_detach_device(domain, &iommu->pdev->dev);
	iommu->pdev = NULL;
	iommu->magic = 0;
	mutex_unlock(&iommu->lock);

	mutex_lock(&hct_data.lock);
	if (iommu->id < MCCP_DEV_MAX)
		bitmap_clear(&hct_data.bitmap, iommu->id, 1);
	mutex_unlock(&hct_data.lock);
}

static int handle_pci_cfg_read(struct mdev_state *mdev_state, int offset,
				 __le32 *val, int count)
{
	u32 tmp_val = 0;
	int ret = -EINVAL;
	struct pci_dev *pdev = mdev_state->iommu->pdev;

	if (!mdev_state->mdev || !pdev) {
		pr_err("hct: invalid dev or pdev\n");
		return ret;
	}

	if (count == 1) {
		u8 tmp;

		ret = pci_user_read_config_byte(pdev, offset, &tmp);
		tmp_val = tmp;
	} else if (count == 2) {
		u16 tmp;

		ret = pci_user_read_config_word(pdev, offset, &tmp);
		tmp_val = tmp;
	} else if (count == 4)
		ret = pci_user_read_config_dword(pdev, offset, &tmp_val);

	*val = cpu_to_le32(tmp_val);

	return ret;
}

static int handle_pci_cfg_write(struct mdev_state *mdev_state, u16 offset,
				 u8 *buf, u32 count)
{
	u32 tmp_val = le32_to_cpu(*(u32 *)buf);
	int ret = -EINVAL;
	struct pci_dev *pdev = mdev_state->iommu->pdev;


	if (!mdev_state->mdev || !pdev) {
		pr_err("hct: invalid dev or pdev\n");
		return ret;
	}

	if (count == 1)
		ret = pci_user_write_config_byte(pdev, offset, tmp_val);
	else if (count == 2)
		ret = pci_user_write_config_word(pdev, offset, tmp_val);
	else if (count == 4)
		ret = pci_user_write_config_dword(pdev, offset, tmp_val);

	return ret;
}

static ssize_t hct_access(struct mdev_device *mdev, u8 *buf, size_t count,
			   loff_t pos, bool is_write)
{
	struct mdev_state *mdev_state;
	unsigned int index;
	loff_t offset;
	int ret = 0;

	if (!mdev || !buf)
		return -EINVAL;

	mdev_state = dev_get_drvdata(&mdev->dev);
	if (!mdev_state) {
		pr_err("%s mdev_state not found\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&mdev_state->ops_lock);

	index = MCCP_VFIO_PCI_OFFSET_TO_INDEX(pos);
	offset = pos & MCCP_VFIO_PCI_OFFSET_MASK;
	switch (index) {
	case VFIO_PCI_CONFIG_REGION_INDEX:
		if (is_write)
			ret = handle_pci_cfg_write(mdev_state, offset, buf, count);
		else
			ret = handle_pci_cfg_read(mdev_state, offset, (__le32 *)buf, count);
		break;
	default:
		ret = -1;
	}

	if (!ret)
		ret = count;

	mutex_unlock(&mdev_state->ops_lock);

	return ret;
}

static int hct_mdev_state_init(struct mdev_state *mdev_state)
{
	unsigned long *bitmap = &hct_data.bitmap;
	struct hct_iommu *iommu = hct_data.iommu;
	unsigned long ref = -1ul;
	int i, n = -1;
	int ret = 0;

	if (!mdev_state)
		return -EINVAL;

	mutex_init(&mdev_state->ops_lock);

	mutex_lock(&hct_data.lock);
	if (hct_data.mdev_ref > 0) {
		mutex_unlock(&hct_data.lock);
		return -EBUSY;
	}

	for (i = 0; i < MCCP_DEV_MAX; i++) {
		if (test_bit(i, bitmap)) {
			if (ref > iommu[i].ref) {
				n = i;
				ref = iommu[i].ref;
			}
		}
	}

	if (n >= 0 && n < MCCP_DEV_MAX) {
		mdev_state->iommu = &iommu[n];
		mdev_state->ref = iommu[n].ref++;
	} else
		ret = -EINVAL;
	mutex_unlock(&hct_data.lock);

	return ret;
}

static int hct_init_dev(struct vfio_device *vdev)
{
	struct mdev_state *mdev_state = vdev_to_mdev_state(vdev);
	struct mdev_device *mdev = to_mdev_device(vdev->dev);
	int ret = 0;

	if (!mdev || !mdev_state)
		return -EINVAL;

	ret = hct_mdev_state_init(mdev_state);
	if (ret)
		return ret;

	mdev_state->mdev = mdev;
	return 0;
}

static void hct_release_dev(struct vfio_device *vdev)
{
	struct mdev_state *mdev_state = vdev_to_mdev_state(vdev);
	struct mdev_device *mdev = to_mdev_device(vdev->dev);

	mutex_lock(&hct_data.lock);
	if (hct_data.mdev_ref > 0)
		pr_warn("The mdev device is in use.\n");
	else {
		mdev_state->iommu->ref--;
		dev_set_drvdata(&mdev->dev, NULL);
	}
	mutex_unlock(&hct_data.lock);
}

static ssize_t hct_read(struct vfio_device *vdev, char __user *buf,
			size_t count, loff_t *ppos)
{
	struct mdev_device *mdev = to_mdev_device(vdev->dev);
	unsigned int done = 0;
	int ret;
	u32 val;
	size_t filled;

	while (count) {
		if (count >= 4 && !(*ppos % 4)) {
			ret = hct_access(mdev, (u8 *)&val, sizeof(u32), *ppos, false);
			if (ret <= 0)
				goto read_err;

			if (copy_to_user(buf, &val, sizeof(u32)))
				goto read_err;

			filled = 4;
		} else if (count >= 2 && !(*ppos % 2)) {
			ret = hct_access(mdev, (u8 *)&val, sizeof(u16), *ppos, false);
			if (ret <= 0)
				goto read_err;

			if (copy_to_user(buf, &val, sizeof(u16)))
				goto read_err;

			filled = 2;
		} else {
			ret = hct_access(mdev, (u8 *)&val, sizeof(u8), *ppos, false);
			if (ret <= 0)
				goto read_err;

			if (copy_to_user(buf, &val, sizeof(u8)))
				goto read_err;

			filled = 1;
		}

		count -= filled;
		done += filled;
		*ppos += filled;
		buf += filled;
	}

	return done;

read_err:
	return -EFAULT;
}

static ssize_t hct_write(struct vfio_device *vdev, const char __user *buf,
		   size_t count, loff_t *ppos)
{
	struct mdev_device *mdev = to_mdev_device(vdev->dev);
	unsigned int done = 0;
	int ret;
	u64 val;
	u8 idx;

	while (count) {
		size_t filled;

		if (count == MCCP_DEV_ID_SIZE && *ppos == MCCP_GET_QUEUE_FLAG) {
			struct mdev_state *mdev_state;
			struct hct_dev_ctx *dev_ctx;
			struct hct_cmd_queue *cmd_q;

			mdev_state = dev_get_drvdata(&mdev->dev);
			if (!mdev_state)
				goto write_err;

			if (copy_from_user(&val, buf, sizeof(u64)) ||
					val >= MCCP_DEV_QUEUE_MAX ||
					val < mdev_state->efd_start)
				goto write_err;

			idx = val - mdev_state->efd_start;
			dev_ctx = &mdev_state->iommu->dev_ctx;
			cmd_q = &dev_ctx->cmd_q[idx];

			mutex_lock(&cmd_q->q_lock);
			if (kfifo_avail(&cmd_q->ectx_fifo))
				kfifo_put(&cmd_q->ectx_fifo, mdev_state->trigger[idx]);
			mutex_unlock(&cmd_q->q_lock);

			filled = MCCP_DEV_ID_SIZE;
		} else if (count >= 4 && !(*ppos % 4)) {
			if (copy_from_user(&val, buf, sizeof(u32)))
				goto write_err;

			ret = hct_access(mdev, (u8 *)&val, sizeof(u32), *ppos, true);
			if (ret <= 0)
				goto write_err;

			filled = 4;
		} else if (count >= 2 && !(*ppos % 2)) {
			if (copy_from_user(&val, buf, sizeof(u16)))
				goto write_err;

			ret = hct_access(mdev, (u8 *)&val, sizeof(u16), *ppos, true);
			if (ret <= 0)
				goto write_err;

			filled = 2;
		} else {
			if (copy_from_user(&val, buf, sizeof(u8)))
				goto write_err;

			ret = hct_access(mdev, (u8 *)&val, sizeof(u8), *ppos, true);
			if (ret <= 0)
				goto write_err;

			filled = 1;
		}
		count -= filled;
		done += filled;
		*ppos += filled;
		buf += filled;
	}

	return done;
write_err:
	return -EFAULT;
}

static int hct_get_region_info(struct mdev_device *mdev,
			 struct vfio_region_info *region_info,
			 u16 *cap_type_id, void **cap_type)
{
	struct mdev_state *mdev_state = NULL;
	struct pci_dev *pdev = NULL;
	unsigned int size = 0;
	u32 bar_index;

	if (!mdev)
		return -EINVAL;

	mdev_state = dev_get_drvdata(&mdev->dev);
	if (!mdev_state)
		return -ENODEV;

	bar_index = region_info->index;
	if (bar_index >= VFIO_PCI_NUM_REGIONS)
		return -EINVAL;

	pdev = mdev_state->iommu->pdev;
	mutex_lock(&mdev_state->ops_lock);

	switch (bar_index) {
	case VFIO_PCI_CONFIG_REGION_INDEX:
		size = pdev->cfg_size;
		break;
	case VFIO_PCI_BAR0_REGION_INDEX ... VFIO_PCI_BAR5_REGION_INDEX:
		size = pci_resource_len(pdev, bar_index);
		break;
	default:
		size = 0;
		break;
	}

	mdev_state->region_info[bar_index].size = size;
	mdev_state->region_info[bar_index].vfio_offset =
		MCCP_VFIO_PCI_INDEX_TO_OFFSET(bar_index);

	region_info->size = size;
	region_info->offset = MCCP_VFIO_PCI_INDEX_TO_OFFSET(bar_index);
	region_info->flags = VFIO_REGION_INFO_FLAG_READ |
		VFIO_REGION_INFO_FLAG_WRITE;
	if (size >= PAGE_SIZE)
		region_info->flags |= VFIO_REGION_INFO_FLAG_MMAP;

	mutex_unlock(&mdev_state->ops_lock);
	return 0;
}

static int hct_get_irq_info(struct mdev_device *mdev,
			     struct vfio_irq_info *irq_info)
{
	switch (irq_info->index) {
	case VFIO_PCI_INTX_IRQ_INDEX:
	case VFIO_PCI_MSI_IRQ_INDEX:
	case VFIO_PCI_MSIX_IRQ_INDEX:
	case VFIO_PCI_REQ_IRQ_INDEX:
		break;

	default:
		return -EINVAL;
	}

	irq_info->flags = VFIO_IRQ_INFO_EVENTFD;
	irq_info->count = 1;

	if (irq_info->index == VFIO_PCI_INTX_IRQ_INDEX)
		irq_info->flags |= (VFIO_IRQ_INFO_MASKABLE |
				VFIO_IRQ_INFO_AUTOMASKED);
	else
		irq_info->flags |= VFIO_IRQ_INFO_NORESIZE;

	return 0;
}

static int hct_get_device_info(struct mdev_device *mdev,
			 struct vfio_device_info *dev_info)
{
	dev_info->flags = VFIO_DEVICE_FLAGS_PCI;
	dev_info->num_regions = VFIO_PCI_NUM_REGIONS;
	dev_info->num_irqs = VFIO_PCI_NUM_IRQS;

	return 0;
}

/* each ccp vq corresponding to one eventfd */
static int hct_set_irq_efds(struct mdev_device *mdev,
			struct vfio_irq_set *hdr,
			void *data)
{
	struct mdev_state *mdev_state = dev_get_drvdata(&mdev->dev);
	int *fd = (int *)data;
	int i;

	if (!mdev_state || !data)
		return -EINVAL;

	if (hdr->index != VFIO_PCI_MSIX_IRQ_INDEX)
		return -EINVAL;

	if ((hdr->flags & VFIO_IRQ_SET_ACTION_TYPE_MASK) != VFIO_IRQ_SET_ACTION_TRIGGER)
		return -EINVAL;

	if (hdr->start + hdr->count > MCCP_DEV_QUEUE_MAX)
		return -EINVAL;

	mdev_state->efd_start = hdr->start;
	for (i = 0; i < hdr->count; i++) {
		struct eventfd_ctx *trigger;

		trigger = eventfd_ctx_fdget(fd[i]);
		if (IS_ERR(trigger))
			return -1;

		mdev_state->trigger[mdev_state->efd_count++] = trigger;
	}

	return 0;
}

static int hct_reset(struct mdev_device *mdev)
{
	struct mdev_state *mdev_state = NULL;

	if (!mdev)
		return -EINVAL;

	mdev_state = dev_get_drvdata(&mdev->dev);
	if (!mdev_state)
		return -EINVAL;

	return 0;
}

static long hct_ioctl(struct vfio_device *vdev, unsigned int cmd,
			unsigned long arg)
{
	struct mdev_device *mdev = to_mdev_device(vdev->dev);
	struct mdev_state *mdev_state = NULL;
	unsigned long minsz;
	int ret = 0;

	if (!mdev)
		return -EINVAL;

	mdev_state = dev_get_drvdata(&mdev->dev);
	if (!mdev_state)
		return -ENODEV;

	if (!mdev_state->iommu || !mdev_state->iommu->pdev)
		return -EIO;

	switch (cmd) {
	case VFIO_DEVICE_GET_INFO:
	{
		struct vfio_device_info info;

		minsz = offsetofend(struct vfio_device_info, num_irqs);

		if (copy_from_user(&info, (void __user *)arg, minsz))
			return -EFAULT;

		if (info.argsz < minsz)
			return -EINVAL;

		ret = hct_get_device_info(mdev, &info);
		if (ret)
			return ret;

		memcpy(&mdev_state->dev_info, &info, sizeof(info));

		if (copy_to_user((void __user *)arg, &info, minsz))
			return -EFAULT;

		return 0;
	}
	case VFIO_DEVICE_GET_REGION_INFO:
	{
		struct vfio_region_info info;
		u16 cap_type_id = 0;
		void *cap_type = NULL;

		minsz = offsetofend(struct vfio_region_info, offset);

		if (copy_from_user(&info, (void __user *)arg, minsz))
			return -EFAULT;

		if (info.argsz < minsz)
			return -EINVAL;

		ret = hct_get_region_info(mdev, &info, &cap_type_id,
					   &cap_type);
		if (ret)
			return ret;

		if (copy_to_user((void __user *)arg, &info, minsz))
			return -EFAULT;

		return 0;
	}

	case VFIO_DEVICE_GET_IRQ_INFO:
	{
		struct vfio_irq_info info;

		minsz = offsetofend(struct vfio_irq_info, count);

		if (copy_from_user(&info, (void __user *)arg, minsz))
			return -EFAULT;

		if ((info.argsz < minsz) ||
		    (info.index >= mdev_state->dev_info.num_irqs))
			return -EINVAL;

		ret = hct_get_irq_info(mdev, &info);
		if (ret)
			return ret;

		if (copy_to_user((void __user *)arg, &info, minsz))
			return -EFAULT;

		return 0;
	}
	case VFIO_DEVICE_SET_IRQS:
	{
		struct vfio_irq_set hdr;
		u8 *data = NULL;
		size_t data_size = 0;

		minsz = offsetofend(struct vfio_irq_set, count);

		if (copy_from_user(&hdr, (void __user *)arg, minsz))
			return -EFAULT;

		ret = vfio_set_irqs_validate_and_prepare(&hdr, mdev_state->dev_info.num_irqs,
				mdev_state->dev_info.num_irqs, &data_size);
		if (ret)
			return ret;

		if (data_size) {
			data = memdup_user((void __user *)(arg + minsz), data_size);
			if (IS_ERR(data))
				return PTR_ERR(data);
		}

		mutex_lock(&mdev_state->ops_lock);
		ret = hct_set_irq_efds(mdev, &hdr, data);
		mutex_unlock(&mdev_state->ops_lock);
		kfree(data);

		return ret;
	}
	case VFIO_DEVICE_RESET:
		return hct_reset(mdev);
	}
	return -ENOTTY;
}

static int hct_open(struct vfio_device *vdev)
{
	struct mdev_state *mdev_state = vdev_to_mdev_state(vdev);

	if (!mdev_state)
		return -ENODEV;

	if (!mdev_state->iommu || !mdev_state->iommu->pdev)
		return -EIO;

	mutex_lock(&hct_data.lock);
	hct_data.mdev_ref++;
	mutex_unlock(&hct_data.lock);

	return 0;
}

static void hct_close(struct vfio_device *vdev)
{
	struct mdev_state *mdev_state = vdev_to_mdev_state(vdev);
	int i;

	if (!mdev_state || !mdev_state->iommu)
		return;

	for (i = 0; i < mdev_state->efd_count; i++)
		eventfd_ctx_put(mdev_state->trigger[i]);
	mdev_state->efd_count = 0;

	mutex_lock(&hct_data.lock);
	hct_data.mdev_ref--;
	mutex_unlock(&hct_data.lock);
}

static ssize_t address_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct mdev_state *mdev_state = dev_get_drvdata(dev);
	struct pci_dev *pdev = NULL;
	ssize_t size;

	if (!mdev_state || !mdev_state->iommu)
		goto exit;

	mutex_lock(&mdev_state->iommu->lock);
	if (!mdev_state->iommu->pdev ||
		mdev_state->iommu->magic != MCCP_SHARE_IOMMU_MAGIC) {
		mutex_unlock(&mdev_state->iommu->lock);
		goto exit;
	}
	pdev = mdev_state->iommu->pdev;
	size = sprintf(buf, "%04x:%02x:%02x.%x",
			pci_domain_nr(pdev->bus),
			pdev->bus->number,
			0x00ff & (pdev->devfn >> 8),
			0x00ff & pdev->devfn);
	mutex_unlock(&mdev_state->iommu->lock);
	return size;

exit:
	return sprintf(buf, "\n");
}

static ssize_t id_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct mdev_state *mdev_state = dev_get_drvdata(dev);
	ssize_t size;

	if (!mdev_state || !mdev_state->iommu)
		goto exit;

	mutex_lock(&mdev_state->iommu->lock);
	if (!mdev_state->iommu->pdev ||
		mdev_state->iommu->magic != MCCP_SHARE_IOMMU_MAGIC) {
		mutex_unlock(&mdev_state->iommu->lock);
		goto exit;
	}

	size = sprintf(buf, "%lu", mdev_state->iommu->id);
	mutex_unlock(&mdev_state->iommu->lock);
	return size;

exit:
	return sprintf(buf, "\n");
}

static ssize_t idx_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct mdev_state *mdev_state = dev_get_drvdata(dev);
	ssize_t size;

	if (!mdev_state || !mdev_state->iommu)
		goto exit;

	mutex_lock(&mdev_state->iommu->lock);
	if (!mdev_state->iommu->pdev ||
		mdev_state->iommu->magic != MCCP_SHARE_IOMMU_MAGIC) {
		mutex_unlock(&mdev_state->iommu->lock);
		goto exit;
	}

	size = sprintf(buf, "%lu", mdev_state->ref);
	mutex_unlock(&mdev_state->iommu->lock);
	return size;

exit:
	return sprintf(buf, "\n");
}

static DEVICE_ATTR_RO(address);
static DEVICE_ATTR_RO(id);
static DEVICE_ATTR_RO(idx);

static struct attribute *mdev_dev_attrs[] = {
	&dev_attr_address.attr,
	&dev_attr_id.attr,
	&dev_attr_idx.attr,
	NULL,
};

static const struct attribute_group mdev_dev_group = {
	.name  = "vendor",
	.attrs = mdev_dev_attrs,
};

static const struct attribute_group *hct_mdev_groups[] = {
	&mdev_dev_group,
	NULL,
};

static void hct_mmap_open(struct vm_area_struct *vma)
{
	zap_vma_ptes(vma, vma->vm_start, vma->vm_end - vma->vm_start);
}

static void hct_mmap_close(struct vm_area_struct *vma)
{
}

static vm_fault_t hct_mmap_fault(struct vm_fault *vmf)
{
	vm_fault_t ret = VM_FAULT_NOPAGE;
	struct vm_area_struct *vma = vmf->vma;

	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
			vma->vm_end - vma->vm_start, pgprot_decrypted(vma->vm_page_prot)))
		ret = VM_FAULT_SIGBUS;
	return ret;
}

static const struct vm_operations_struct hct_mmap_ops = {
	.open = hct_mmap_open,
	.close = hct_mmap_close,
	.fault = hct_mmap_fault,
};

static int hct_mmap(struct vfio_device *vdev, struct vm_area_struct *vma)
{
	struct mdev_device *mdev = to_mdev_device(vdev->dev);
	struct mdev_state *mds = dev_get_drvdata(&mdev->dev);
	struct pci_dev *pdev = mds->iommu->pdev;
	unsigned int index;

	index = vma->vm_pgoff >> (40 - PAGE_SHIFT);
	vma->vm_private_data = mdev;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_pgoff = pci_resource_start(pdev, index) >> PAGE_SHIFT;
	vm_flags_set(vma, VM_IO | VM_PFNMAP | VM_DONTEXPAND | VM_DONTDUMP);
	vma->vm_ops = &hct_mmap_ops;
	return 0;
}

static const struct vfio_device_ops hct_mdev_ops = {
	.init			= hct_init_dev,
	.release		= hct_release_dev,
	.open_device	= hct_open,
	.close_device	= hct_close,
	.read			= hct_read,
	.write			= hct_write,
	.ioctl			= hct_ioctl,
	.mmap			= hct_mmap,
	.bind_iommufd           = vfio_iommufd_emulated_bind,
	.unbind_iommufd         = vfio_iommufd_emulated_unbind,
	.attach_ioas            = vfio_iommufd_emulated_attach_ioas,
	.detach_ioas            = vfio_iommufd_emulated_detach_ioas,
};

static int hct_mdev_probe(struct mdev_device *mdev)
{
	struct mdev_state *mdev_state = NULL;
	int ret;

	if (!mdev)
		return -EINVAL;

	mdev_state = vfio_alloc_device(mdev_state, vdev, &mdev->dev,
					&hct_mdev_ops);
	if (IS_ERR(mdev_state))
		return PTR_ERR(mdev_state);

	ret = vfio_register_emulated_iommu_dev(&mdev_state->vdev);
	if (ret) {
		vfio_put_device(&mdev_state->vdev);
		return ret;
	}

	dev_set_drvdata(&mdev->dev, mdev_state);
	return 0;
}

static void hct_mdev_remove(struct mdev_device *mdev)
{
	struct mdev_state *mdev_state = dev_get_drvdata(&mdev->dev);

	vfio_unregister_group_dev(&mdev_state->vdev);
	vfio_put_device(&mdev_state->vdev);
}

static unsigned int hct_mdev_get_available(struct mdev_type *mtype)
{
	return MCCP_INSTANCE_MAX;
}

static ssize_t hct_mdev_show_description(struct mdev_type *mtype, char *buf)
{
	return sprintf(buf, "This is HYGON CCP device!");
}

struct mdev_driver hct_mdev_driver = {
	.device_api         = VFIO_DEVICE_API_PCI_STRING,
	.driver = {
		.name       = "hct_mdev",
		.owner      = THIS_MODULE,
		.mod_name   = KBUILD_MODNAME,
		.dev_groups = hct_mdev_groups,
	},
	.probe              = hct_mdev_probe,
	.remove             = hct_mdev_remove,
	.get_available      = hct_mdev_get_available,
	.show_description   = hct_mdev_show_description,
};

struct hct_private {
	struct list_head head;
	struct mutex lock;
	unsigned int id;
};

static int hct_share_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	struct hct_private *private;
	unsigned int id;

	private = kzalloc(sizeof(*private), GFP_KERNEL);
	if (!private)
		return -ENOMEM;

	mutex_lock(&hct_data.lock);
	bitmap_set(hct_data.ids, 0, 1);
	id = (unsigned int)find_first_zero_bit(hct_data.ids, MCCP_INSTANCE_MAX);
	if (id < MCCP_INSTANCE_MAX)
		bitmap_set(hct_data.ids, id, 1);
	mutex_unlock(&hct_data.lock);

	if (id >= MCCP_INSTANCE_MAX) {
		kfree(private);
		return -EBUSY;
	}

	mutex_lock(&hct_share.lock);
	hct_share.ref++;
	hct_share.pagecount = MCCP_DEV_MAX;
	mutex_unlock(&hct_share.lock);

	file->private_data = private;
	private->id = id << MCCP_INSTANCE_OFFSET;
	INIT_LIST_HEAD(&private->head);
	mutex_init(&private->lock);

	return ret;
}

static bool is_invalid_reserved_pfn(unsigned long pfn)
{
	if (pfn_valid(pfn))
		return PageReserved(pfn_to_page(pfn));

	return true;
}

static int follow_fault_pfn(struct vm_area_struct *vma, struct mm_struct *mm,
			unsigned long vaddr, unsigned long *pfn,
			bool write_fault)
{
	int ret;

	ret = follow_pfn(vma, vaddr, pfn);
	if (ret) {
		bool unlocked = false;

		ret = fixup_user_fault(mm, vaddr,
					FAULT_FLAG_REMOTE |
					(write_fault ?  FAULT_FLAG_WRITE : 0),
					&unlocked);

		if (unlocked)
			return -EAGAIN;

		if (ret)
			return ret;

		ret = follow_pfn(vma, vaddr, pfn);
	}

	return ret;
}

static int vaddr_get_pfn(struct mm_struct *mm, unsigned long vaddr,
			 int prot, unsigned long *pfn)
{
	struct page *page[1];
	struct vm_area_struct *vma;
	unsigned int flags = 0;
	int ret;

	if (prot & IOMMU_WRITE)
		flags |= FOLL_WRITE;

	mmap_read_lock(mm);
	ret = pin_user_pages_remote(mm, vaddr, 1, flags | FOLL_LONGTERM,
				    page, NULL);
	if (ret == 1) {
		*pfn = page_to_pfn(page[0]);
		ret = 0;
		goto done;
	}

	vaddr = untagged_addr(vaddr);

retry:
	vma = find_vma_intersection(mm, vaddr, vaddr + 1);

	if (vma && vma->vm_flags & VM_PFNMAP) {
		ret = follow_fault_pfn(vma, mm, vaddr, pfn, prot & IOMMU_WRITE);
		if (ret == -EAGAIN)
			goto retry;

		if (!ret && !is_invalid_reserved_pfn(*pfn))
			ret = -EFAULT;
	}
done:
	mmap_read_unlock(mm);

	return ret;
}

struct page **hct_pin_memory(struct hct_private *private, unsigned long uaddr,
			unsigned long ulen, unsigned long *n)
{
	unsigned long npages, size;
	int npinned;
	struct page **pages;
	unsigned long first, last;

	if (ulen == 0 || uaddr + ulen < uaddr)
		return NULL;

	first = (uaddr & PAGE_MASK) >> PAGE_SHIFT;
	last = ((uaddr + ulen - 1) & PAGE_MASK) >> PAGE_SHIFT;
	npages = (last - first + 1);

	if (WARN_ON_ONCE(npages > INT_MAX))
		return NULL;

	size = npages * sizeof(struct page *);
	if (size > PAGE_SIZE)
		pages = vmalloc(size);
	else
		pages = kmalloc(size, GFP_KERNEL);

	if (!pages)
		return NULL;

	/* Pin the user virtual address. */
	npinned = pin_user_pages_fast(uaddr, npages, FOLL_WRITE, pages);
	if (npinned != npages)
		goto err;

	*n = npages;
	return pages;

err:
	if (npinned > 0)
		unpin_user_pages(pages, npinned);
	kvfree(pages);
	return NULL;
}

static void hct_unpin_memory(struct hct_private *private, struct page **pages,
			     unsigned long npages)
{
	unpin_user_pages(pages, npages);
	kvfree(pages);
}

static inline int is_dma_share(dma_addr_t dma_iova, size_t dma_size)
{
	if (dma_iova >= MCCP_DMA_IOVA_OFFSET &&
	    dma_iova + dma_size <= MCCP_DMA_IOVA_OFFSET + MCCP_DMA_IOVA_SIZE)
		return 1;
	else
		return 0;
}

static int hct_add_dma_share_unsafe(dma_addr_t dma_iova, size_t dma_size)
{
	int ret = 0;

	if (!is_dma_share(dma_iova, dma_size))
		return 0;

	if (!hct_data.dma_share_size) {
		hct_data.dma_share_iova = dma_iova;
		hct_data.dma_share_size = dma_size;
	}

	if (dma_iova != hct_data.dma_share_iova ||
	    dma_size != hct_data.dma_share_size)
		ret = -EINVAL;
	else
		hct_data.dma_share_ref++;

	return ret;
}

static int hct_unmap_dma_share_unsafe(dma_addr_t dma_iova, size_t dma_size)
{
	if (!is_dma_share(dma_iova, dma_size))
		return -EINVAL;

	if (hct_data.dma_share_size) {
		if (hct_data.dma_share_iova == dma_iova &&
		    hct_data.dma_share_size == dma_size)
			hct_data.dma_share_ref--;

		if (hct_data.dma_share_ref == 0) {
			iommu_unmap(hct_data.domain, hct_data.dma_share_iova,
				hct_data.dma_share_size);
			hct_data.dma_share_size = 0;
		}
	}

	return 0;
}

static int hct_iommu_iova_check_unsafe(dma_addr_t dma_iova, size_t dma_size,
				phys_addr_t phys_addr,
				struct iommu_domain *domain)
{
	dma_addr_t iova;
	int ret = 0;
	size_t mapped = 0;

	iova = dma_iova;
	while (iova < dma_iova + dma_size) {
		phys_addr_t phys;

		phys = iommu_iova_to_phys(domain, iova);
		if (phys) {
			if ((phys_addr & PHY_ADDR_MASK) != (phys & PHY_ADDR_MASK)) {
				pr_err("iova=0x%llx phys_addr=0x%llx phys=0x%llx, check fail.\n",
							iova, phys_addr, phys);
				ret = -1;
				break;
			}
			mapped += PAGE_SIZE;
		}
		iova += PAGE_SIZE;
		phys_addr += PAGE_SIZE;
	}

	if (ret == 0 && mapped == dma_size)
		ret = 1;

	return ret;
}

static unsigned long get_num_contig_pages(unsigned long idx,
				struct page **inpages, unsigned long npages)
{
	unsigned long paddr, next_paddr;
	unsigned long i = idx + 1, pages = 1;

	/* find the number of contiguous pages starting from idx */
	paddr = page_to_phys(inpages[idx]);
	while (i < npages) {
		next_paddr = page_to_phys(inpages[i++]);
		if ((paddr + PAGE_SIZE) == next_paddr) {
			pages++;
			paddr = next_paddr;
			continue;
		}
		break;
	}

	return pages;
}

static struct hct_dma *hct_find_dma(struct hct_private *private,
				      dma_addr_t start, size_t size)
{
	struct hct_dma *dma, *tmp;

	list_for_each_entry_safe(dma, tmp, &private->head, next) {
		if (dma->iova <= start &&
		    dma->iova + dma->size >= start + size)
			return dma;
	}

	return NULL;
}

/*
 * Turns out AMD IOMMU has a page table bug where it won't map large pages
 * to a region that previously mapped smaller pages.  This should be fixed
 * soon, so this is just a temporary workaround to break mappings down into
 * PAGE_SIZE.  Better to map smaller pages than nothing.
 */
static int map_try_harder(struct iommu_domain *domain, dma_addr_t iova,
			  unsigned long pfn, long npage, int prot)
{
	long i;
	int ret = 0;

	for (i = 0; i < npage; i++, pfn++, iova += PAGE_SIZE) {
		ret = iommu_map(domain, iova,
				(phys_addr_t)pfn << PAGE_SHIFT,
				PAGE_SIZE, prot, GFP_KERNEL);
		if (ret)
			break;
	}

	for (; i < npage && i > 0; i--, iova -= PAGE_SIZE)
		iommu_unmap(domain, iova, PAGE_SIZE);

	return ret;
}

/*
 * only handle io-memory [vm_flags | VM_PFNMAP == true]
 */
static int hct_iommu_pfnmap(struct hct_private *private, struct hct_dma *dma)
{
	unsigned long pfn;
	unsigned long vaddr;
	dma_addr_t iova;
	size_t mapped_size = 0;
	size_t size;
	int ret = 0;

	if (!private || !dma)
		return -EINVAL;

	dma->pfnmap_flag = 1;
	vaddr = dma->vaddr;
	iova = dma->iova;
	size = dma->size;

	mutex_lock(&hct_data.lock);
	while (size) {
		ret = vaddr_get_pfn(current->mm, vaddr, hct_data.prot, &pfn);
		if (ret)
			goto map_fail;

		ret = iommu_map(hct_data.domain, iova,
				(phys_addr_t)pfn << PAGE_SHIFT,
				1 << PAGE_SHIFT, hct_data.prot,
				GFP_KERNEL);
		if (ret)
			goto map_fail;

		size -= 1 << PAGE_SHIFT;
		vaddr += 1 << PAGE_SHIFT;
		iova += 1 << PAGE_SHIFT;
		mapped_size += 1 << PAGE_SHIFT;
	}
	mutex_unlock(&hct_data.lock);

	list_add(&dma->next, &private->head);
	return 0;

map_fail:
	mutex_unlock(&hct_data.lock);
	iommu_unmap(hct_data.domain, dma->iova, mapped_size);
	return ret;
}

static int hct_iommu_map(struct hct_private *private, unsigned long vaddr,
			dma_addr_t dma_iova, size_t dma_size)
{
	struct hct_dma *dma;
	struct page **pages;
	unsigned long n, i, npages;
	dma_addr_t iova, iova_end, iova_next;
	int ret = 0;
	size_t mapped_size = 0;
	size_t iova_size = dma_size;

	if (!dma_size || (vaddr | dma_iova | dma_size) & (PAGE_SIZE - 1))
		return -EINVAL;

	if (hct_find_dma(private, dma_iova, dma_size))
		return 0;

	dma = kzalloc(sizeof(*dma), GFP_KERNEL);
	if (!dma)
		return -ENOMEM;

	pages = hct_pin_memory(private, vaddr, dma_size, &n);
	if (!pages) {
		/* We will think the vm_flags includes VM_PFNMAP. */
		dma->vaddr = vaddr;
		dma->iova = dma_iova;
		dma->size = dma_size;
		ret = hct_iommu_pfnmap(private, dma);
		if (ret)
			kfree(dma);
		return ret;
	}

	dma->vaddr = vaddr;
	dma->iova = dma_iova;
	dma->pages = pages;
	dma->size = dma_size;
	dma->npages = n;

	iova = dma_iova;
	iova_end = dma_iova + dma_size;
	iova_size = dma_size;

	mutex_lock(&hct_data.lock);
	for (i = 0; iova < iova_end && i < n; iova = iova_next, i += npages) {
		size_t len;
		phys_addr_t phys;

		npages = get_num_contig_pages(i, pages, n);

		/* When the value of npages is 524288, the value of npages * PAGE_SIZE
		 * will be 0x80000000 (bit31 is 1).
		 * When the value of npages is greater than 524288, if the type of len is int,
		 * the len will be a negative value.
		 */
		len = min_t(size_t, (npages * PAGE_SIZE), iova_size);
		phys = page_to_phys(pages[i]);

		iova_size -= len;
		iova_next = iova + len;

		ret = hct_iommu_iova_check_unsafe(iova, len, phys, hct_data.domain);
		if (ret < 0) {
			ret = -EBUSY;
			goto map_fail;
		} else if (ret > 0) {
			ret = 0;
			continue;
		}

		ret = iommu_map(hct_data.domain, iova, phys, len, hct_data.prot, GFP_KERNEL);
		if (ret) {
			if (ret == -EBUSY)
				ret = map_try_harder(hct_data.domain, iova,
						     phys >> PAGE_SHIFT,
						     len >> PAGE_SHIFT,
						     hct_data.prot);
			if (ret)
				goto map_fail;
		}
		mapped_size += len;
		cond_resched();
	}

	ret = hct_add_dma_share_unsafe(dma_iova, dma_size);
	if (ret)
		goto map_fail;

	mutex_unlock(&hct_data.lock);
	list_add(&dma->next, &private->head);
	return 0;
map_fail:
	if (mapped_size)
		iommu_unmap(hct_data.domain, dma_iova, mapped_size);
	mutex_unlock(&hct_data.lock);
	hct_unpin_memory(private, pages, n);
	kfree(dma);
	return ret;
}

static void hct_iommu_unmap(struct hct_private *private,
			dma_addr_t iova, size_t size)
{
	struct iommu_domain *domain = hct_data.domain;
	struct hct_dma *dma;

	if (!size || (iova | size) & (PAGE_SIZE - 1))
		return;

	dma = hct_find_dma(private, iova, size);
	if (!dma)
		return;

	mutex_lock(&hct_data.lock);
	iommu_unmap(domain, dma->iova, dma->size);
	if (dma->pfnmap_flag == 0)
		hct_unpin_memory(private, dma->pages, dma->npages);
	list_del(&dma->next);
	kfree(dma);
	mutex_unlock(&hct_data.lock);
}

static void hct_iommu_unmap_all(struct hct_private *private)
{
	struct iommu_domain *domain = hct_data.domain;
	struct hct_dma *dma, *tmp;

	mutex_lock(&hct_data.lock);
	list_for_each_entry_safe(dma, tmp, &private->head, next) {
		if (hct_unmap_dma_share_unsafe(dma->iova, dma->size))
			iommu_unmap(domain, dma->iova, dma->size);
		if (dma->pfnmap_flag == 0)
			hct_unpin_memory(private, dma->pages, dma->npages);
		cond_resched();
		list_del(&dma->next);
		kfree(dma);
	}
	mutex_unlock(&hct_data.lock);
}

static struct page *hct_get_page(pgoff_t page_idx)
{
	u64 *node;

	mutex_lock(&hct_share.lock);
	if (!hct_share.pages[page_idx]) {
		hct_share.pages[page_idx] =
			alloc_pages(GFP_HIGHUSER | __GFP_ZERO, 0);
		if (!hct_share.pages[page_idx]) {
			mutex_unlock(&hct_share.lock);
			return NULL;
		}
	}
	get_page(hct_share.pages[page_idx]);

	node = page_to_virt(hct_share.pages[page_idx]) + PAGE_SIZE - 8;
	*node = hct_data.iommu[page_idx].pdev->dev.numa_node;
	mutex_unlock(&hct_share.lock);

	return hct_share.pages[page_idx];
}

static void hct_put_pages(void)
{
	int i;

	for (i = 0; i < hct_share.pagecount; i++) {
		if (!hct_share.pages[i])
			continue;

		put_page(hct_share.pages[i]);
		hct_share.pages[i] = NULL;
	}
}

/* Clear status information when exiting abnormally. */
static void hct_clear_shared_lock_memory(unsigned int gid)
{
	int *base;
	int *queue_lck;
	int dev_idx;
	int queue_idx;

	for (dev_idx = 0; dev_idx < MCCP_DEV_MAX &&
			hct_share.pages[dev_idx]; dev_idx++) {
		base = (int *)page_to_virt(hct_share.pages[dev_idx]);
		for (queue_idx = 0; queue_idx < MCCP_DEV_QUEUE; queue_idx++) {
			queue_lck = base + queue_idx;
			if (*queue_lck == gid)
				*queue_lck = 0; /* vq userid will be changed. */
		}
	}
}

static long hct_share_ioctl(struct file *file, unsigned int ioctl, unsigned long arg)
{
	struct hct_dev_ctrl dev_ctrl;
	unsigned int cmd_id;
	unsigned int len;
	unsigned int pasid;
	int ret = 0;
	struct hct_private *private = file->private_data;

	if (_IOC_TYPE(ioctl) != MCCP_SHARE_IOC_TYPE)
		return -EINVAL;

	cmd_id = _IOC_NR(ioctl);
	len = _IOC_SIZE(ioctl);

	if (cmd_id != MCCP_SHARE_OP)
		return -EINVAL;

	if (len != sizeof(dev_ctrl))
		return -EINVAL;

	if (copy_from_user(&dev_ctrl, (void __user *)arg, sizeof(dev_ctrl)))
		return -EINVAL;

	mutex_lock(&private->lock);
	switch (dev_ctrl.op) {
	case MCCP_SHARE_OP_DMA_MAP:
		ret = hct_iommu_map(private, dev_ctrl.vaddr, dev_ctrl.iova, dev_ctrl.size);
		break;
	case MCCP_SHARE_OP_DMA_UNMAP:
		hct_iommu_unmap(private, dev_ctrl.iova, dev_ctrl.size);
		ret = 0;
		break;
	case MCCP_SHARE_OP_DMA_UNMAP_ALL:
		hct_iommu_unmap_all(private);
		ret = 0;
		break;
	case MCCP_SHARE_OP_GET_ID:
		dev_ctrl.id = private->id;
		if (copy_to_user((void __user *)arg, &dev_ctrl, sizeof(dev_ctrl)))
			ret = -EINVAL;
		else
			ret = 0;
		break;
	case MCCP_SHARE_OP_GET_PASID:
		/* The different virtual machines is distinguished through pasid. */
		pasid = private->id >> MCCP_INSTANCE_OFFSET;
		if (pasid >= MCCP_PASID_SIZE) {
			ret = -EINVAL;
			break;
		}

		dev_ctrl.id = pasid;
		if (copy_to_user((void __user *)arg, &dev_ctrl, sizeof(dev_ctrl)))
			ret = -EINVAL;
		break;
	case MCCP_SHARE_OP_GET_VERSION:
		memcpy(dev_ctrl.version, VERSION_STRING, sizeof(VERSION_STRING));
		if (copy_to_user((void __user *)arg, &dev_ctrl, sizeof(dev_ctrl)))
			ret = -EINVAL;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	mutex_unlock(&private->lock);

	return ret;
}

static int hct_share_close(struct inode *inode, struct file *file)
{
	struct hct_private *private = file->private_data;
	unsigned int id = private->id >> MCCP_INSTANCE_OFFSET;

	mutex_lock(&hct_share.lock);
	/* For the vm scenario, the hct_share.vaddr value is NULL. */
	if (hct_share.vaddr) {
		struct hct_shared_cfg *cfg = hct_share.vaddr;
		int i;

		if (private->id == cfg->ccps_ref_lock)
			cfg->ccps_ref_lock = 0;

		for (i = 0; i < MCCP_DEV_MAX; i++)
			if (private->id == (MCCP_INSTANCE_MASK & cfg->ccp_state[i]))
				cfg->ccp_state[i] = 0;

		for (i = 0; i < MCCP_QUEUES_MAX; i++)
			if (private->id == cfg->ccp_queue_state[i])
				cfg->ccp_queue_state[i] = MCCP_QUEUE_NEED_INIT;

		for (i = 0; i < MCCP_IOVA_MAX_SLOT; i++)
			if (private->id == cfg->iova_slot[i])
				cfg->iova_slot[i] = 0;
	}

	hct_clear_shared_lock_memory(private->id);

	hct_share.ref--;
	if (!hct_share.ref) {
		hct_put_pages();
		if (hct_share.vaddr)
			memset(hct_share.vaddr, 0x00, hct_share.size);
	}
	mutex_unlock(&hct_share.lock);

	mutex_lock(&hct_data.lock);
	if (id < MCCP_INSTANCE_MAX)
		bitmap_clear(hct_data.ids, id, 1);
	mutex_unlock(&hct_data.lock);

	mutex_lock(&private->lock);
	hct_iommu_unmap_all(private);
	mutex_unlock(&private->lock);

	kfree(private);
	return 0;
}

static vm_fault_t hct_cdev_vma_fault(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
	pgoff_t page_idx = (vmf->address - vma->vm_start) >> PAGE_SHIFT;

	if (page_idx >= hct_share.pagecount)
		return VM_FAULT_SIGBUS;

	vmf->page = hct_get_page(page_idx);
	if (!vmf->page)
		return VM_FAULT_SIGBUS;

	return 0;
}

static const struct vm_operations_struct hct_cdev_vm_ops = {
	.fault = hct_cdev_vma_fault,
};

static int hct_share_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long len;
	int ret = 0;

	mutex_lock(&hct_share.lock);
	len = vma->vm_end - vma->vm_start;
	if (len == MCCP_SHARED_SIZE) {
		/* The required size for vm is 64KB,
		 * and will follow the pagefault process.
		 */
		vma->vm_ops = &hct_cdev_vm_ops;
		goto exit;
	}

	if (unlikely(!hct_share.vaddr)) {
		hct_share.size = (vma->vm_end - vma->vm_start);
		hct_share.vaddr = kzalloc(hct_share.size, GFP_KERNEL);
	}

	if (!hct_share.vaddr) {
		ret = -ENOMEM;
		goto exit;
	}

	if (hct_share.size != (vma->vm_end - vma->vm_start)) {
		ret = -EINVAL;
		pr_err("invalid hct share size\n");
		goto exit;
	}

	ret = remap_pfn_range(vma, vma->vm_start,
				virt_to_phys(hct_share.vaddr) >> PAGE_SHIFT,
				hct_share.size,
				vma->vm_page_prot);
exit:
	mutex_unlock(&hct_share.lock);
	return ret;
}

static const struct file_operations hct_share_fops = {
	.owner		= THIS_MODULE,
	.open		= hct_share_open,
	.release	= hct_share_close,
	.mmap		= hct_share_mmap,
	.unlocked_ioctl	= hct_share_ioctl,
};

static struct miscdevice hct_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "hct_share",
	.fops = &hct_share_fops,
	.mode = 0666,
};

static int hct_share_init(void)
{
	int i;
	int ret;

	memset(&hct_data, 0x00, sizeof(hct_data));
	mutex_init(&hct_data.lock);

	for (i = 0; i < MCCP_DEV_MAX; i++)
		mutex_init(&hct_data.iommu[i].lock);

	ret = misc_register(&hct_misc);
	if (!ret) {
		hct_data.domain = iommu_domain_alloc(&pci_bus_type);
		if (!hct_data.domain) {
			pr_err("iommu domain alloc failed\n");
			misc_deregister(&hct_misc);
			return -ENOMEM;
		}
		hct_data.prot = IOMMU_READ | IOMMU_WRITE;
	}

	return ret;
}

static void hct_share_exit(void)
{
	int i;
	struct hct_iommu *iommu;
	struct iommu_domain *domain;
	struct pci_dev *pdev;

	mutex_lock(&hct_data.lock);
	for (i = 0; i < MCCP_DEV_MAX; i++) {
		iommu = &hct_data.iommu[i];
		pdev = iommu->pdev;
		if (pdev) {
			domain = iommu_get_domain_for_dev(&pdev->dev);
			if (domain == hct_data.domain)
				iommu_detach_device(domain, &pdev->dev);
		}
	}
	mutex_unlock(&hct_data.lock);

	if (hct_data.domain)
		iommu_domain_free(hct_data.domain);

	misc_deregister(&hct_misc);
	kfree(hct_share.vaddr);
}

static int hct_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	return hct_iommu_alloc(pdev);
}

static void hct_pci_remove(struct pci_dev *pdev)
{
	struct hct_iommu *iommu;
	struct hct_dev_ctx *dev_ctx;
	int i;

	iommu = pci_get_drvdata(pdev);
	if (!iommu) {
		pci_set_drvdata(pdev, NULL);
		return;
	}

	dev_ctx = &iommu->dev_ctx;
	for (i = 0; i < dev_ctx->q_count; i++)
		kfifo_free(&dev_ctx->cmd_q[i].ectx_fifo);

	if (dev_ctx->io_regs)
		iounmap(dev_ctx->io_regs);
	if (dev_ctx->irq) {
		tasklet_kill(&dev_ctx->irq_tasklet);
		free_irq(dev_ctx->irq, dev_ctx);
		dev_ctx->irq = 0;
		pci_free_irq_vectors(pdev);
		pci_clear_master(pdev);
		pci_disable_device(pdev);
	}
	hct_iommu_free(iommu);
	pci_set_drvdata(pdev, NULL);
}

static struct pci_driver hct_pci_driver = {
	.name = "hct",
	.id_table = NULL,
	.probe = hct_pci_probe,
	.remove = hct_pci_remove,
};

static const struct file_operations hct_vd_fops = {
	.owner = THIS_MODULE,
};

static void hct_device_release(struct device *dev)
{
	dev_dbg(dev, "hct: released\n");
}
#endif /* IS_ENABLED(CONFIG_VFIO_MDEV) */

/* set the flags PAT, PCT and PWT of page all to 0
 * for obtaining cache properties.
 */
void hct_noiommu_set_memory_wb(unsigned long address)
{
	pgd_t *pgd = current->mm->pgd + pgd_index(address);
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	pte_t old_pte;
	pte_t new_pte;
	pgprot_t new_prot;
	unsigned long pfn;

	if (pgd_none(*pgd)) {
		pr_err("pgd val shouldn't be none\n");
		return;
	}

	p4d = p4d_offset(pgd, address);
	if (p4d_none(*p4d)) {
		pr_err("p4d val shouldn't be none\n");
		return;
	}

	pud = pud_offset(p4d, address);
	if (pud_none(*pud) || pud_large(*pud) || !pud_present(*pud)) {
		pr_err("pud val is invalid.\n");
		return;
	}

	pmd = pmd_offset(pud, address);
	if (pmd_none(*pmd) || pmd_large(*pmd) || !pmd_present(*pmd)) {
		pr_err("pmd val is invalid.\n");
		return;
	}

	pte = pte_offset_kernel(pmd, address);
	if (pte_none(*pte)) {
		pr_err("pte val shouldn't be none\n");
		return;
	}

	old_pte = *pte;
	pfn = pte_pfn(old_pte);
	new_prot = pte_pgprot(old_pte);
	pgprot_val(new_prot) &= ~(_PAGE_PAT | _PAGE_PCD | _PAGE_PWT);
	new_pte = pfn_pte(pfn, new_prot);
	set_pte_atomic(pte, new_pte);
}

static DEFINE_MUTEX(hct_noiommu_lock);
static long hct_noiommu_ioctl(struct file *file,
		unsigned int ioctl, unsigned long arg)
{
	struct hct_dev_ctrl ctrl;
	unsigned int cmd_id;
	unsigned int len;
	int ret = 0;

	if (_IOC_TYPE(ioctl) != MCCP_NOIOMMU_IOC_TYPE)
		return -EINVAL;

	cmd_id = _IOC_NR(ioctl);
	len = _IOC_SIZE(ioctl);

	if (cmd_id != MCCP_SHARE_OP)
		return -EINVAL;

	if (len != sizeof(ctrl))
		return -EINVAL;

	if (copy_from_user(&ctrl, (void __user *)arg, sizeof(ctrl)))
		return -EINVAL;

	mutex_lock(&hct_noiommu_lock);
	switch (ctrl.op) {
	case MCCP_NOIOMMU_SET_MEMORY_WB:
		while (ctrl.nr_pages && ctrl.nr_pages--) {
			hct_noiommu_set_memory_wb(ctrl.vt_addr);
			ctrl.vt_addr += PAGE_SIZE;
		}
		break;
	case MCCP_NOIOMMU_GET_SME_ACTIVE:
		ctrl.sme_mask = sme_me_mask;
		if (copy_to_user((void __user *)arg, &ctrl, sizeof(ctrl)))
			ret = -EINVAL;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	mutex_unlock(&hct_noiommu_lock);

	return ret;
}

const struct file_operations hct_noiommu_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = hct_noiommu_ioctl,
};

struct miscdevice hct_noiommu_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "hct_noiommu",
	.fops  = &hct_noiommu_fops,
};

#define CPUID_VENDOR_HygonGenuine_ebx	0x6f677948
#define CPUID_VENDOR_HygonGenuine_ecx	0x656e6975
#define CPUID_VENDOR_HygonGenuine_edx	0x6e65476e

static int __init hct_dev_init(void)
{
	int __maybe_unused ret = 0;
	u32 vendor_ebx = 0;
	u32 vendor_ecx = 0;
	u32 vendor_edx = 0;
	u32 vendor_eax = 0;

	cpuid(0, &vendor_eax, &vendor_ebx, &vendor_ecx, &vendor_edx);

	/* HygonGenuine */
	if (!(vendor_ebx == CPUID_VENDOR_HygonGenuine_ebx &&
	      vendor_ecx == CPUID_VENDOR_HygonGenuine_ecx &&
	      vendor_edx == CPUID_VENDOR_HygonGenuine_edx)) {
		pr_err("Not hygon hardware\n");
		return -1;
	}

#if IS_ENABLED(CONFIG_VFIO_MDEV)
	if (!iommu_present(&pci_bus_type))
		return misc_register(&hct_noiommu_misc);

	ret = mdev_register_driver(&hct_mdev_driver);
	if (ret)
		return ret;

	memset(&hct_dev, 0, sizeof(hct_dev));

	ret = alloc_chrdev_region(&hct_dev.vd_devt, 0, MINORMASK + 1,
				  MCCP_NAME);

	if (ret < 0) {
		pr_err("Error: failed to register hct_dev, err:%d\n", ret);
		goto failed0;
	}

	cdev_init(&hct_dev.vd_cdev, &hct_vd_fops);
	cdev_add(&hct_dev.vd_cdev, hct_dev.vd_devt, MINORMASK + 1);

	hct_dev.vd_class = class_create(MCCP_CLASS_NAME);
	if (IS_ERR(hct_dev.vd_class)) {
		pr_err("Error: failed to register hct_dev class\n");
		ret = PTR_ERR(hct_dev.vd_class);
		goto failed1;
	}

	hct_dev.dev.class = hct_dev.vd_class;
	hct_dev.dev.release = hct_device_release;
	dev_set_name(&hct_dev.dev, "%s", MCCP_NAME);
	hct_dev.dev.devt = hct_dev.vd_devt;

	ret = device_register(&hct_dev.dev);
	if (ret)
		goto failed2;

	ret = mdev_register_parent(&hct_dev.mdev_parent, &hct_dev.dev,
				&hct_mdev_driver, hct_mdev_types,
				ARRAY_SIZE(hct_mdev_types));
	if (ret)
		goto failed3;

	ret = hct_share_init();
	if (ret)
		goto failed4;

	memset(&hct_share, 0x00, sizeof(hct_share));
	mutex_init(&hct_share.lock);

	ret = pci_register_driver(&hct_pci_driver);
	if (ret)
		goto failed5;

	goto all_done;

failed5:
	hct_share_exit();

failed4:
	mdev_unregister_parent(&hct_dev.mdev_parent);

failed3:
	device_unregister(&hct_dev.dev);

failed2:
	class_destroy(hct_dev.vd_class);

failed1:
	cdev_del(&hct_dev.vd_cdev);
	unregister_chrdev_region(hct_dev.vd_devt, MINORMASK + 1);

failed0:
	mdev_unregister_driver(&hct_mdev_driver);

all_done:
	return ret;
#else
	pr_info("The module mdev is disabled.\n");
	return misc_register(&hct_noiommu_misc);
#endif
}

static void __exit hct_dev_exit(void)
{
#if IS_ENABLED(CONFIG_VFIO_MDEV)
	if (!iommu_present(&pci_bus_type)) {
		misc_deregister(&hct_noiommu_misc);
		return;
	}

	hct_share_exit();
	hct_dev.dev.bus = NULL;
	mdev_unregister_parent(&hct_dev.mdev_parent);

	device_unregister(&hct_dev.dev);
	cdev_del(&hct_dev.vd_cdev);
	unregister_chrdev_region(hct_dev.vd_devt, MINORMASK + 1);
	class_destroy(hct_dev.vd_class);
	mdev_unregister_driver(&hct_mdev_driver);
	hct_dev.vd_class = NULL;

	pci_unregister_driver(&hct_pci_driver);
#else
	misc_deregister(&hct_noiommu_misc);
#endif
}

module_init(hct_dev_init)
module_exit(hct_dev_exit)

MODULE_LICENSE("GPL");
MODULE_VERSION(VERSION_STRING);
MODULE_AUTHOR(DRIVER_AUTHOR);

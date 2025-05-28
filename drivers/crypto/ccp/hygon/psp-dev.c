// SPDX-License-Identifier: GPL-2.0-only
/*
 * HYGON Platform Security Processor (PSP) interface
 *
 * Copyright (C) 2024 Hygon Info Technologies Ltd.
 *
 * Author: Liyang Han <hanliyang@hygon.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/psp.h>
#include <linux/psp-hygon.h>
#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/rwlock.h>
#include <linux/pgtable.h>

#include "psp-dev.h"
#include "vpsp.h"
#include "csv-dev.h"

/* Function and variable pointers for hooks */
struct hygon_psp_hooks_table hygon_psp_hooks;
static unsigned int psp_int_rcvd;
wait_queue_head_t psp_int_queue;

struct kmem_cache *vpsp_cmd_ctx_slab;
static struct workqueue_struct *psp_wq;
static struct work_struct psp_work;
static struct psp_ringbuffer_cmd_buf *psp_grb_cmdbuf;
static work_func_t psp_worker_notify;

bool psp_in_nowait_mode;
static struct psp_misc_dev *psp_misc;
#define HYGON_PSP_IOC_TYPE 'H'
enum HYGON_PSP_OPCODE {
	HYGON_PSP_MUTEX_ENABLE = 1,
	HYGON_PSP_MUTEX_DISABLE,
	HYGON_VPSP_CTRL_OPT,
	HYGON_PSP_OP_PIN_USER_PAGE,
	HYGON_PSP_OP_UNPIN_USER_PAGE,
	HYGON_PSP_OPCODE_MAX_NR,
};

#define HYGON_RESOURCE2_IOC_TYPE 'R'
enum HYGON_PSP_RESOURCE2_OPCODE {
	HYGON_RESOURCE2_OP_GET_PCI_BAR_RANGE = 1,
	HYGON_RESOURCE2_OPCODE_MAX_NR,
};

uint64_t atomic64_exchange(uint64_t *dst, uint64_t val)
{
	return xchg(dst, val);
}

int psp_mutex_init(struct psp_mutex *mutex)
{
	if (!mutex)
		return -1;
	mutex->locked = 0;
	return 0;
}

int psp_mutex_trylock(struct psp_mutex *mutex)
{
	if (atomic64_exchange(&mutex->locked, 1))
		return 0;
	else
		return 1;
}

int psp_mutex_lock_timeout(struct psp_mutex *mutex, uint64_t ms)
{
	int ret = 0;
	unsigned long je, last_je;

	last_je = jiffies;
	je = jiffies + msecs_to_jiffies(ms);
	do {
		if (psp_mutex_trylock(mutex)) {
			ret = 1;
			break;
		}

		// avoid triggering soft lockup warning
		if (time_after(jiffies, last_je + msecs_to_jiffies(100))) {
			schedule();
			last_je = jiffies;
		}
	} while ((ms == 0) || time_before(jiffies, je));

	return ret;
}

int psp_mutex_unlock(struct psp_mutex *mutex)
{
	if (!mutex)
		return -1;

	atomic64_exchange(&mutex->locked, 0);
	return 0;
}

void psp_worker_register_notify(work_func_t notify)
{
	psp_worker_notify = notify;
}

static void psp_worker_handler(struct work_struct *unused)
{
	int mutex_enabled = READ_ONCE(hygon_psp_hooks.psp_mutex_enabled);

	if (!hygon_psp_hooks.sev_dev_hooks_installed)
		return;

	if (psp_worker_notify) {
		psp_worker_notify(unused);
		psp_worker_notify = NULL;
		psp_in_nowait_mode = false;
	}

	if (mutex_enabled)
		psp_mutex_unlock(&psp_misc->data_pg_aligned->mb_mutex);
	else
		mutex_unlock(hygon_psp_hooks.sev_cmd_mutex);
}

static int mmap_psp(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long page;

	page = virt_to_phys((void *)psp_misc->data_pg_aligned) >> PAGE_SHIFT;

	if (remap_pfn_range(vma, vma->vm_start, page, (vma->vm_end - vma->vm_start),
				vma->vm_page_prot)) {
		pr_info("remap failed...");
		return -1;
	}
	vm_flags_mod(vma, VM_DONTDUMP|VM_DONTEXPAND, 0);
	pr_info("remap_pfn_rang page:[%lu] ok.\n", page);
	return 0;
}

static ssize_t read_psp(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	ssize_t remaining;

	if ((*ppos + count) > PAGE_SIZE) {
		pr_info("%s: invalid address range, pos %llx, count %lx\n",
			__func__, *ppos, count);
		return -EFAULT;
	}

	remaining = copy_to_user(buf, (char *)psp_misc->data_pg_aligned + *ppos, count);
	if (remaining)
		return -EFAULT;
	*ppos += count;

	return count;
}

static ssize_t write_psp(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	ssize_t remaining, written;

	if ((*ppos + count) > PAGE_SIZE) {
		pr_info("%s: invalid address range, pos %llx, count %lx\n",
			__func__, *ppos, count);
		return -EFAULT;
	}

	remaining = copy_from_user((char *)psp_misc->data_pg_aligned + *ppos, buf, count);
	written = count - remaining;
	if (!written)
		return -EFAULT;

	*ppos += written;

	return written;
}

/**
 * Try to pin a page
 *
 * @vaddr: the userspace virtual address, must be aligned to PAGE_SIZE
 */
static int psp_pin_user_page(u64 vaddr)
{
	struct page *page;
	long npinned = 0;
	int ref_count = 0;

	// check must be aligned to PAGE_SIZE
	if (vaddr & (PAGE_SIZE - 1)) {
		pr_err("vaddr %llx not aligned to 0x%lx\n", vaddr, PAGE_SIZE);
		return -EFAULT;
	}

	npinned = pin_user_pages_fast(vaddr, 1, FOLL_WRITE, &page);
	if (npinned != 1) {
		pr_err("PSP: pin_user_pages_fast fail\n");
		return -ENOMEM;
	}

	ref_count = page_ref_count(page);
	pr_debug("pin user page with address %llx, page ref_count %d\n", vaddr, ref_count);
	return 0;
}

/**
 * Try to unpin a page
 *
 * @vaddr: the userspace virtual address, must be aligned to PAGE_SIZE
 */
static int psp_unpin_user_page(u64 vaddr)
{
	struct page *page;
	long npinned = 0;
	int ref_count = 0;

	// check must be aligned to PAGE_SIZE
	if (vaddr & (PAGE_SIZE - 1)) {
		pr_err("vaddr %llx not aligned to 0x%lx\n", vaddr, PAGE_SIZE);
		return -EFAULT;
	}

	// page reference count increment by 1
	npinned = get_user_pages_fast(vaddr, 1, FOLL_WRITE, &page);
	if (npinned != 1) {
		pr_err("PSP: pin_user_pages_fast fail\n");
		return -ENOMEM;
	}

	// page reference count decrement by 2
	put_page(page);
	put_page(page);

	ref_count = page_ref_count(page);
	pr_debug("unpin user page with address %llx, page ref_count %d\n", vaddr, ref_count);
	return 0;
}

static long ioctl_psp(struct file *file, unsigned int ioctl, unsigned long arg)
{
	unsigned int opcode = 0;
	struct vpsp_dev_ctrl vpsp_ctrl_op;
	int ret = -EFAULT;

	if (!hygon_psp_hooks.sev_dev_hooks_installed)
		return -ENODEV;

	if (_IOC_TYPE(ioctl) != HYGON_PSP_IOC_TYPE) {
		pr_info("%s: invalid ioctl type: 0x%x\n", __func__, _IOC_TYPE(ioctl));
		return -EINVAL;
	}
	opcode = _IOC_NR(ioctl);
	switch (opcode) {
	case HYGON_PSP_MUTEX_ENABLE:
		psp_mutex_lock_timeout(&psp_misc->data_pg_aligned->mb_mutex, 0);
		// And get the sev lock to make sure no one is using it now.
		mutex_lock(hygon_psp_hooks.sev_cmd_mutex);
		hygon_psp_hooks.psp_mutex_enabled = 1;
		mutex_unlock(hygon_psp_hooks.sev_cmd_mutex);
		// Wait 10ms just in case someone is right before getting the psp lock.
		mdelay(10);
		psp_mutex_unlock(&psp_misc->data_pg_aligned->mb_mutex);
		ret = 0;
		break;

	case HYGON_PSP_MUTEX_DISABLE:
		mutex_lock(hygon_psp_hooks.sev_cmd_mutex);
		// And get the psp lock to make sure no one is using it now.
		psp_mutex_lock_timeout(&psp_misc->data_pg_aligned->mb_mutex, 0);
		hygon_psp_hooks.psp_mutex_enabled = 0;
		psp_mutex_unlock(&psp_misc->data_pg_aligned->mb_mutex);
		// Wait 10ms just in case someone is right before getting the sev lock.
		mdelay(10);
		mutex_unlock(hygon_psp_hooks.sev_cmd_mutex);
		ret = 0;
		break;

	case HYGON_VPSP_CTRL_OPT:
		if (copy_from_user(&vpsp_ctrl_op, (void __user *)arg,
			sizeof(struct vpsp_dev_ctrl)))
			return -EFAULT;
		ret = do_vpsp_op_ioctl(&vpsp_ctrl_op);
		if (!ret && copy_to_user((void __user *)arg, &vpsp_ctrl_op,
				sizeof(struct vpsp_dev_ctrl)))
			return -EFAULT;
		break;

	case HYGON_PSP_OP_PIN_USER_PAGE:
		ret = psp_pin_user_page((u64)arg);
		break;

	case HYGON_PSP_OP_UNPIN_USER_PAGE:
		ret = psp_unpin_user_page((u64)arg);
		break;

	default:
		pr_info("%s: invalid ioctl number: %d\n", __func__, opcode);
		return -EINVAL;
	}
	return ret;
}

static resource_size_t get_master_psp_bar_size(void)
{
	struct psp_device *psp = psp_master;
	struct pci_dev *pdev = to_pci_dev(psp->dev);

	return pci_resource_len(pdev, 2);
}

static long ioctl_psp_resource2(struct file *file, unsigned int ioctl, unsigned long arg)
{
	unsigned int opcode = 0;
	resource_size_t bar_size = 0;
	int ret = -EFAULT;

	if (_IOC_TYPE(ioctl) != HYGON_RESOURCE2_IOC_TYPE) {
		pr_err("%s: invalid ioctl type: 0x%x\n", __func__, _IOC_TYPE(ioctl));
		return -EINVAL;
	}

	opcode = _IOC_NR(ioctl);
	switch (opcode) {
	case HYGON_RESOURCE2_OP_GET_PCI_BAR_RANGE:
		bar_size = get_master_psp_bar_size();

		if (copy_to_user((void __user *)arg, &bar_size,
				sizeof(unsigned long)))
			return -EFAULT;
		ret = 0;
		break;

	default:
		pr_err("%s: invalid ioctl number: %d\n", __func__, opcode);
		return -EINVAL;
	}
	return ret;
}

static int mmap_psp_resource2(struct file *filp, struct vm_area_struct *vma)
{
	struct psp_device *psp = psp_master;
	struct pci_dev *pdev = to_pci_dev(psp->dev);
	int bar = 2;

	vma->vm_page_prot = pgprot_device(vma->vm_page_prot);
	vma->vm_pgoff += (pci_resource_start(pdev, bar) >> PAGE_SHIFT);

	return io_remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
				  vma->vm_end - vma->vm_start,
				  vma->vm_page_prot);
}

static const struct file_operations psp_source2_fops = {
	.owner          = THIS_MODULE,
	.mmap		= mmap_psp_resource2,
	.unlocked_ioctl = ioctl_psp_resource2,
};

static const struct file_operations psp_fops = {
	.owner          = THIS_MODULE,
	.mmap		= mmap_psp,
	.read		= read_psp,
	.write		= write_psp,
	.unlocked_ioctl = ioctl_psp,
};

int hygon_psp_additional_setup(struct sp_device *sp)
{
	struct device *dev = sp->dev;
	int ret = 0;

	if (!hygon_psp_hooks.sev_dev_hooks_installed)
		return -ENODEV;

	if (!psp_misc) {
		struct miscdevice *misc;

		psp_wq = create_singlethread_workqueue("psp_workqueue");
		if (!psp_wq)
			return -ENOMEM;

		INIT_WORK(&psp_work, psp_worker_handler);

		vpsp_cmd_ctx_slab = kmem_cache_create("vpsp_cmd_ctx",
				sizeof(struct vpsp_cmd_ctx), 0, SLAB_HWCACHE_ALIGN, NULL);
		if (!vpsp_cmd_ctx_slab) {
			destroy_workqueue(psp_wq);
			return -ENOMEM;
		}

		psp_grb_cmdbuf = kmalloc(sizeof(*psp_grb_cmdbuf), GFP_KERNEL);
		if (!psp_grb_cmdbuf) {
			kmem_cache_destroy(vpsp_cmd_ctx_slab);
			destroy_workqueue(psp_wq);
			return -ENOMEM;
		}

		psp_misc = devm_kzalloc(dev, sizeof(*psp_misc), GFP_KERNEL);
		if (!psp_misc)
			return -ENOMEM;
		psp_misc->data_pg_aligned = (struct psp_dev_data *)get_zeroed_page(GFP_KERNEL);
		if (!psp_misc->data_pg_aligned) {
			dev_err(dev, "alloc psp data page failed\n");
			devm_kfree(dev, psp_misc);
			psp_misc = NULL;
			return -ENOMEM;
		}
		SetPageReserved(virt_to_page(psp_misc->data_pg_aligned));
		psp_mutex_init(&psp_misc->data_pg_aligned->mb_mutex);

		*(uint32_t *)((void *)psp_misc->data_pg_aligned + 8) = 0xdeadbeef;
		misc = &psp_misc->dev_misc;
		misc->minor = MISC_DYNAMIC_MINOR;
		misc->name = "hygon_psp_config";
		misc->fops = &psp_fops;

		ret = misc_register(misc);
		if (ret)
			return ret;

		misc = &psp_misc->resource2_misc;
		misc->minor = MISC_DYNAMIC_MINOR;
		misc->name = "hygon_psp_resource2";
		misc->fops = &psp_source2_fops;

		ret = misc_register(misc);
		if (ret)
			return ret;

		psp_ringbuffer_queue_init(vpsp_ring_buffer);
		kref_init(&psp_misc->refcount);
		hygon_psp_hooks.psp_misc = psp_misc;
	} else {
		kref_get(&psp_misc->refcount);
	}

	return ret;
}

void hygon_psp_exit(struct kref *ref)
{
	struct psp_misc_dev *misc_dev = container_of(ref, struct psp_misc_dev, refcount);

	misc_deregister(&misc_dev->dev_misc);
	misc_deregister(&misc_dev->resource2_misc);
	ClearPageReserved(virt_to_page(misc_dev->data_pg_aligned));
	free_page((unsigned long)misc_dev->data_pg_aligned);
	psp_misc = NULL;
	hygon_psp_hooks.psp_misc = NULL;
	kmem_cache_destroy(vpsp_cmd_ctx_slab);
	flush_workqueue(psp_wq);
	destroy_workqueue(psp_wq);
	kfree(psp_grb_cmdbuf);
	psp_ringbuffer_queue_free(vpsp_ring_buffer);
}

int fixup_hygon_psp_caps(struct psp_device *psp)
{
	/* the hygon psp is unavailable if bit0 is cleared in feature reg */
	if (!(psp->capability & PSP_CAPABILITY_SEV))
		return -ENODEV;

	psp->capability &= ~(PSP_CAPABILITY_TEE |
			     PSP_CAPABILITY_PSP_SECURITY_REPORTING);
	return 0;
}

static int psp_wait_cmd_ioc(struct psp_device *psp,
			    unsigned int *reg, unsigned int timeout)
{
	int ret;

	ret = wait_event_timeout(psp_int_queue,
			psp_int_rcvd, timeout * HZ);
	if (!ret)
		return -ETIMEDOUT;

	*reg = ioread32(psp->io_regs + psp->vdata->sev->cmdresp_reg);

	return 0;
}

static int psp_wait_cmd_ioc_ringbuffer(struct psp_device *psp,
			    unsigned int *reg, unsigned int timeout)
{
	int ret;

	ret = wait_event_timeout(psp_int_queue,
			psp_int_rcvd, timeout * HZ);
	if (!ret)
		return -ETIMEDOUT;

	*reg = ioread32(psp->io_regs + psp->vdata->sev->cmdbuff_addr_lo_reg);
	return 0;
}

int psp_do_cmd_locked(int cmd, void *data, int *psp_ret, uint32_t op)
{
	struct psp_device *psp = psp_master;
	unsigned int phys_lsb, phys_msb;
	unsigned int reg, ret = 0;

	if (!psp || !hygon_psp_hooks.sev_dev_hooks_installed)
		return -ENODEV;

	if (*hygon_psp_hooks.psp_dead)
		return -EBUSY;

	if (op & PSP_DO_CMD_OP_NOWAIT) {
		if (psp_worker_notify)
			psp_in_nowait_mode = true;
		else {
			dev_err(psp->dev, "psp_worker_notify not registered in nowait mode\n");
			return -EINVAL;
		}
	} else {
		psp_in_nowait_mode = false;
	}

	if (op & PSP_DO_CMD_OP_PHYADDR) {
		phys_lsb = data ? lower_32_bits((phys_addr_t)data) : 0;
		phys_msb = data ? upper_32_bits((phys_addr_t)data) : 0;
	} else {
		/* Get the physical address of the command buffer */
		phys_lsb = data ? lower_32_bits(__psp_pa(data)) : 0;
		phys_msb = data ? upper_32_bits(__psp_pa(data)) : 0;
	}

	dev_dbg(psp->dev, "psp command id %#x buffer 0x%08x%08x timeout %us\n",
		cmd, phys_msb, phys_lsb, *hygon_psp_hooks.psp_cmd_timeout);

	iowrite32(phys_lsb, psp->io_regs + psp->vdata->sev->cmdbuff_addr_lo_reg);
	iowrite32(phys_msb, psp->io_regs + psp->vdata->sev->cmdbuff_addr_hi_reg);

	psp_int_rcvd = 0;

	reg = FIELD_PREP(SEV_CMDRESP_CMD, cmd) | SEV_CMDRESP_IOC;
	iowrite32(reg, psp->io_regs + psp->vdata->sev->cmdresp_reg);

	if (!(op & PSP_DO_CMD_OP_NOWAIT)) {
		/* wait for command completion */
		ret = psp_wait_cmd_ioc(psp, &reg, *hygon_psp_hooks.psp_cmd_timeout);
		if (ret) {
			if (psp_ret)
				*psp_ret = 0;

			dev_err(psp->dev, "psp command %#x timed out, disabling PSP\n", cmd);
			*hygon_psp_hooks.psp_dead = true;

			return ret;
		}

		if (psp_ret)
			*psp_ret = FIELD_GET(PSP_CMDRESP_STS, reg);

		if (FIELD_GET(PSP_CMDRESP_STS, reg)) {
			dev_dbg(psp->dev, "psp command %#x failed (%#010lx)\n",
				cmd, FIELD_GET(PSP_CMDRESP_STS, reg));
			ret = -EIO;
		}
	}
	return ret;
}

int psp_do_cmd(int cmd, void *data, int *psp_ret)
{
	int rc;
	int mutex_enabled = READ_ONCE(hygon_psp_hooks.psp_mutex_enabled);

	if (!hygon_psp_hooks.sev_dev_hooks_installed)
		return -ENODEV;

	if (mutex_enabled) {
		if (psp_mutex_lock_timeout(&hygon_psp_hooks.psp_misc->data_pg_aligned->mb_mutex,
					   PSP_MUTEX_TIMEOUT) != 1)
			return -EBUSY;
	} else {
		mutex_lock(hygon_psp_hooks.sev_cmd_mutex);
	}

	rc = psp_do_cmd_locked(cmd, data, psp_ret, 0);
	if (mutex_enabled)
		psp_mutex_unlock(&hygon_psp_hooks.psp_misc->data_pg_aligned->mb_mutex);
	else
		mutex_unlock(hygon_psp_hooks.sev_cmd_mutex);

	return rc;
}
EXPORT_SYMBOL_GPL(psp_do_cmd);

uint8_t psp_legacy_rb_supported;	// support legacy ringbuffer
uint8_t psp_rb_oc_supported;		// support overcommit
uint8_t psp_generic_rb_supported;	// support generic ringbuffer
void psp_ringbuffer_check_support(void)
{
	int ret, error = 0;
	static atomic_t rb_checked = ATOMIC_INIT(0);
	int rb_check_old = 0;
	struct tkm_cmdresp_device_info_get *info = NULL;

	if (atomic_try_cmpxchg(&rb_checked, &rb_check_old, 1)) {
		// get buildid to check if the firmware supports ringbuffer mode
		info = kzalloc(sizeof(*info), GFP_KERNEL);
		if (!info) {
			atomic_set(&rb_checked, 0);
			goto end;
		}

		info->head.cmdresp_code = TKM_DEVICE_INFO_GET;
		info->head.cmdresp_size = sizeof(*info);
		info->head.buf_size = sizeof(*info);
		ret = psp_do_cmd(TKM_PSP_CMDID, info, &error);
		if (ret) {
			pr_warn("psp_do_cmd failed ret %d[%#x]\n", ret, error);
			atomic_set(&rb_checked, 0);
			goto end;
		}

		/* check if the firmware supports the ringbuffer mode */
		psp_legacy_rb_supported = PSP_RB_IS_SUPPORTED(info->dev_info.fw_version);
		psp_rb_oc_supported = PSP_RB_OC_IS_SUPPORTED(info->dev_info.fw_version);
		psp_generic_rb_supported = PSP_GRB_IS_SUPPORTED(info->dev_info.fw_version);

		if (!psp_legacy_rb_supported && !psp_generic_rb_supported)
			pr_info("psp ringbuffer not supported\n");
		else
			pr_info("psp ringbuffer is supported\n");
	}
end:
	kfree(info);
}


static DEFINE_MUTEX(psp_rb_mutex);

/*
 * Populate the command from the virtual machine to the queue to
 * support execution in ringbuffer mode
 */
uint32_t psp_ringbuffer_enqueue(struct csv_ringbuffer_queue *ringbuffer,
				uint32_t cmd, phys_addr_t phy_addr, uint16_t flags)
{
	struct csv_cmdptr_entry cmdptr = { };
	struct csv_statval_entry statval = { };
	uint32_t index = -1;

	if (!psp_legacy_rb_supported && !psp_generic_rb_supported)
		return -1;

	cmdptr.cmd_buf_ptr = phy_addr;
	cmdptr.cmd_id = cmd;
	cmdptr.cmd_flags = flags;

	statval.status = PSP_CMD_STATUS_RUNNING;

	mutex_lock(&psp_rb_mutex);
	index = cmd_queue_tail(&ringbuffer->cmd_ptr);

	/**
	 * If the firmware does not support the overcommit function:
	 *	the firmware may not check the 'status' before executing cmd.
	 *	Therefore, the 'status' must be written before the cmd be enqueued,
	 *	otherwise, X86 may overwrite the result written by the firmware.
	 *
	 * If the firmware support the overcommit function:
	 *	The firmware will forcefully check the 'status'
	 *	before executing cmd until the 'status' becomes 0xffff.
	 *	In order to prevent the firmware from getting the cmd to be valid,
	 *	the 'status' must be written after waiting for the cmd to be queued.
	 */
	if (psp_rb_oc_supported) {
		if (enqueue_cmd(&ringbuffer->cmd_ptr, &cmdptr, 1) != 1) {
			index = -1;
			goto out;
		}
		enqueue_stat(&ringbuffer->stat_val, &statval, 1);
	} else {
		if (enqueue_stat(&ringbuffer->stat_val, &statval, 1) != 1) {
			index = -1;
			goto out;
		}
		enqueue_cmd(&ringbuffer->cmd_ptr, &cmdptr, 1);
	}

out:
	mutex_unlock(&psp_rb_mutex);
	return index;
}

void psp_ringbuffer_dequeue(struct csv_ringbuffer_queue *ringbuffer,
				struct csv_cmdptr_entry *cmdptr,
				struct csv_statval_entry *statval, uint32_t num)
{
	int i;
	uint32_t orig_head, que_size;

	mutex_lock(&psp_rb_mutex);

	orig_head = cmd_queue_head(&ringbuffer->cmd_ptr);
	que_size = cmd_queue_size(&ringbuffer->cmd_ptr);
	if (que_size < num)
		num = que_size;

	if (cmdptr)
		dequeue_cmd(&ringbuffer->cmd_ptr, (void *)cmdptr, num);
	else
		ringbuffer->cmd_ptr.head += num;

	if (statval)
		dequeue_stat(&ringbuffer->stat_val, (void *)statval, num);
	else
		ringbuffer->stat_val.head += num;

	/**
	 * Ensure that the statval of the dequeued command is 0
	 * to prevent it from being accessed by the overcommit
	 * function of the psp ringbuffer.
	 */
	for (i = orig_head; i < orig_head + num; ++i)
		ringbuffer_set_status(ringbuffer, i, 0);

	mutex_unlock(&psp_rb_mutex);
}

static int __psp_ringbuffer_queue_init(struct csv_ringbuffer_queue *ring_buffer)
{
	int ret = 0, i;
	void *cmd_ptr_buffer	= NULL;
	void *stat_val_buffer	= NULL;
	struct csv_cmdptr_entry *cmd;

	memset((void *)ring_buffer, 0, sizeof(struct csv_ringbuffer_queue));

	cmd_ptr_buffer = kzalloc(CSV_RING_BUFFER_LEN, GFP_KERNEL);
	if (!cmd_ptr_buffer)
		return -ENOMEM;
	csv_queue_init(&ring_buffer->cmd_ptr, cmd_ptr_buffer,
			CSV_RING_BUFFER_SIZE, CSV_RING_BUFFER_ESIZE);
	/**
	 * For high-priority queue: initialize all commands with a valid cmd_id
	 * to prevent PSP from reading invalid cmd_id.
	 *
	 * Low-priority queue never
	 * attempts to read commands from empty queue.
	 */
	cmd = (struct csv_cmdptr_entry *)ring_buffer->cmd_ptr.data_align;
	for (i = 0; i < CSV_RING_BUFFER_ELEMENT_NUM; ++i)
		cmd[i].cmd_id = TKM_PSP_CMDID;

	stat_val_buffer = kzalloc(CSV_RING_BUFFER_LEN, GFP_KERNEL);
	if (!stat_val_buffer) {
		ret = -ENOMEM;
		goto free_cmdptr;
	}
	csv_queue_init(&ring_buffer->stat_val, stat_val_buffer,
			CSV_RING_BUFFER_SIZE, CSV_RING_BUFFER_ESIZE);
	return 0;

free_cmdptr:
	kfree(cmd_ptr_buffer);

	return ret;
}

int psp_ringbuffer_queue_init(struct csv_ringbuffer_queue *ring_buffer)
{
	int i;
	int ret;

	for (i = CSV_COMMAND_PRIORITY_HIGH; i < CSV_COMMAND_PRIORITY_NUM; i++) {
		ret = __psp_ringbuffer_queue_init(&ring_buffer[i]);
		if (ret)
			goto free;
	}
	return 0;
free:
	psp_ringbuffer_queue_free(ring_buffer);
	return -ENOMEM;
}

void psp_ringbuffer_queue_free(struct csv_ringbuffer_queue *ring_buffer)
{
	int i;

	for (i = CSV_COMMAND_PRIORITY_HIGH; i < CSV_COMMAND_PRIORITY_NUM; i++) {
		kfree((void *)ring_buffer[i].cmd_ptr.data);
		ring_buffer[i].cmd_ptr.data = 0;
		kfree((void *)ring_buffer[i].stat_val.data);
		ring_buffer[i].stat_val.data = 0;
	}
}

static int __psp_do_generic_ringbuf_cmds_locked(struct csv_ringbuffer_queue *ring_buffer,
						int *pspret)
{
	int cmd = PSP_CMD_RING_BUFFER;
	struct csv_ringbuffer_queue *que = NULL;
	int psp_op = 0;

	if (!psp_grb_cmdbuf)
		return -EFAULT;

	que = &ring_buffer[CSV_COMMAND_PRIORITY_HIGH];
	psp_grb_cmdbuf->high.mask = que->cmd_ptr.mask;
	psp_grb_cmdbuf->high.cmdptr_address = __psp_pa(que->cmd_ptr.data_align);
	psp_grb_cmdbuf->high.statval_address = __psp_pa(que->stat_val.data_align);
	psp_grb_cmdbuf->high.head = cmd_queue_head(&que->cmd_ptr);
	psp_grb_cmdbuf->high.tail = cmd_queue_tail(&que->cmd_ptr);

	que = &ring_buffer[CSV_COMMAND_PRIORITY_LOW];
	psp_grb_cmdbuf->low.mask = que->cmd_ptr.mask;
	psp_grb_cmdbuf->low.cmdptr_address = __psp_pa(que->cmd_ptr.data_align);
	psp_grb_cmdbuf->low.statval_address = __psp_pa(que->stat_val.data_align);
	psp_grb_cmdbuf->low.head = cmd_queue_head(&que->cmd_ptr);
	if (psp_rb_oc_supported)
		psp_grb_cmdbuf->low.tail = cmd_queue_overcommit_tail(&que->cmd_ptr);
	else
		psp_grb_cmdbuf->low.tail = cmd_queue_tail(&que->cmd_ptr);

	pr_debug("ringbuffer launch high head %x, tail %x\n",
		psp_grb_cmdbuf->high.head, psp_grb_cmdbuf->high.tail);
	pr_debug("ringbuffer launch low head %x, tail %x\n",
		psp_grb_cmdbuf->low.head, psp_grb_cmdbuf->low.tail);

	if (psp_worker_notify)
		psp_op = PSP_DO_CMD_OP_NOWAIT;

	return psp_do_cmd_locked(cmd, psp_grb_cmdbuf, pspret, psp_op);
}

static int __psp_ringbuffer_enter_locked(struct csv_ringbuffer_queue *ring_buffer, int *error)
{
	int ret;
	struct csv_data_ring_buffer *data;
	struct csv_ringbuffer_queue *low_queue;
	struct csv_ringbuffer_queue *hi_queue;
	struct psp_device *psp = psp_master;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	low_queue = &ring_buffer[CSV_COMMAND_PRIORITY_LOW];
	hi_queue = &ring_buffer[CSV_COMMAND_PRIORITY_HIGH];

	data->queue_lo_cmdptr_address = __psp_pa(low_queue->cmd_ptr.data_align);
	data->queue_lo_statval_address = __psp_pa(low_queue->stat_val.data_align);
	data->queue_hi_cmdptr_address = __psp_pa(hi_queue->cmd_ptr.data_align);
	data->queue_hi_statval_address = __psp_pa(hi_queue->stat_val.data_align);
	data->queue_lo_size = 1;
	data->queue_hi_size = 1;
	data->int_on_empty = 1;

	ret = psp_do_cmd_locked(CSV_CMD_RING_BUFFER, data, error, 0);
	if (!ret)
		iowrite32(0, psp->io_regs + psp->vdata->sev->cmdbuff_addr_hi_reg);

	kfree(data);
	return ret;
}

static int __psp_do_ringbuffer_cmds_locked(struct csv_ringbuffer_queue *ring_buffer, int *psp_ret)
{
	struct psp_device *psp = psp_master;
	unsigned int rb_tail, rb_head;
	unsigned int reg, rb_ctl, ret = 0;
	struct csv_ringbuffer_queue *hi_rb, *low_rb;

	if (!psp || !hygon_psp_hooks.sev_dev_hooks_installed)
		return -ENODEV;

	if (*hygon_psp_hooks.psp_dead)
		return -EBUSY;

	hi_rb = &ring_buffer[CSV_COMMAND_PRIORITY_HIGH];
	low_rb = &ring_buffer[CSV_COMMAND_PRIORITY_LOW];

	/* update rb tail */
	rb_tail = ioread32(psp->io_regs + psp->vdata->sev->cmdbuff_addr_hi_reg);
	rb_tail &= (~PSP_RBTAIL_QHI_TAIL_MASK);
	rb_tail |= (cmd_queue_tail(&hi_rb->cmd_ptr) << PSP_RBTAIL_QHI_TAIL_SHIFT);
	rb_tail &= (~PSP_RBTAIL_QLO_TAIL_MASK);
	if (psp_rb_oc_supported)
		rb_tail |= cmd_queue_overcommit_tail(&low_rb->cmd_ptr);
	else
		rb_tail |= cmd_queue_tail(&low_rb->cmd_ptr);
	iowrite32(rb_tail, psp->io_regs + psp->vdata->sev->cmdbuff_addr_hi_reg);

	/* update rb head */
	rb_head = ioread32(psp->io_regs + psp->vdata->sev->cmdbuff_addr_lo_reg);
	rb_head &= (~PSP_RBHEAD_QHI_HEAD_MASK);
	rb_head |= (cmd_queue_head(&hi_rb->cmd_ptr) << PSP_RBHEAD_QHI_HEAD_SHIFT);
	rb_head &= (~PSP_RBHEAD_QLO_HEAD_MASK);
	rb_head |= cmd_queue_head(&low_rb->cmd_ptr);
	iowrite32(rb_head, psp->io_regs + psp->vdata->sev->cmdbuff_addr_lo_reg);

	pr_debug("ringbuffer launch rb_head %x, rb_tail %x\n", rb_head, rb_tail);

	if (psp_worker_notify)
		psp_in_nowait_mode = true;

	/* update rb ctl to trigger psp irq */
	psp_int_rcvd = 0;
	/* PSP response to x86 only when all queue is empty or error happends */
	rb_ctl = (PSP_RBCTL_X86_WRITES | PSP_RBCTL_RBMODE_ACT | PSP_RBCTL_CLR_INTSTAT);
	iowrite32(rb_ctl, psp->io_regs + psp->vdata->sev->cmdresp_reg);

	if (!psp_in_nowait_mode) {
		/* wait for all commands in ring buffer completed */
		ret = psp_wait_cmd_ioc_ringbuffer(psp, &reg, *hygon_psp_hooks.psp_cmd_timeout*10);
		if (ret) {
			if (psp_ret)
				*psp_ret = 0;
			dev_err(psp->dev,
				"psp command in ringbuffer mode timed out, disabling PSP\n");
			*hygon_psp_hooks.psp_dead = true;
			return ret;
		}
		/* cmd error happends */
		if (reg & PSP_RBHEAD_QPAUSE_INT_STAT)
			ret = -EFAULT;
	}

	return ret;
}

int psp_do_ringbuffer_cmds_locked(struct csv_ringbuffer_queue *ring_buffer, int *psp_ret)
{
	int rc = 0;

	if (psp_generic_rb_supported) {
		rc = __psp_do_generic_ringbuf_cmds_locked(ring_buffer, psp_ret);
	} else {
		rc = __psp_ringbuffer_enter_locked(ring_buffer, psp_ret);
		if (rc)
			goto end;

		rc = __psp_do_ringbuffer_cmds_locked(ring_buffer, psp_ret);
	}
end:
	return rc;
}

int psp_ringbuffer_get_newhead(uint32_t *hi_head, uint32_t *low_head)
{
	struct psp_device *psp = psp_master;
	unsigned int reg;
	unsigned int rb_head, rb_tail;
	unsigned int psp_ret;
	int ret = -EIO;

	if (psp_generic_rb_supported) {
		reg = ioread32(psp->io_regs + psp->vdata->sev->cmdresp_reg);
		psp_ret = FIELD_GET(PSP_CMDRESP_STS, reg);
		if (psp_ret) {
			pr_debug("ringbuffer execve failed (%#010x)\n", psp_ret);
			ret = -psp_ret;
			goto end;
		}

		*hi_head = psp_grb_cmdbuf->high.head;
		*low_head = psp_grb_cmdbuf->low.head;
		pr_debug("ringbuffer exit hi_head %x, low_head %x\n", *hi_head, *low_head);
	} else {
		reg = ioread32(psp->io_regs + psp->vdata->sev->cmdbuff_addr_lo_reg);
		/* cmd error happends */
		if (reg & PSP_RBHEAD_QPAUSE_INT_STAT) {
			pr_debug("ringbuffer execve failed (%#010x)\n", reg);
			goto end;
		}

		rb_head = reg;
		rb_tail = ioread32(psp->io_regs + psp->vdata->sev->cmdbuff_addr_hi_reg);

		*hi_head = (reg & PSP_RBHEAD_QHI_HEAD_MASK) >> PSP_RBHEAD_QHI_HEAD_SHIFT;
		*low_head = reg & PSP_RBHEAD_QLO_HEAD_MASK;
		pr_debug("ringbuffer exit rb_head %x, rb_tail %x\n", rb_head, rb_tail);
	}

	ret = 0;
end:
	return ret;
}

#ifdef CONFIG_HYGON_PSP2CPU_CMD

static DEFINE_SPINLOCK(p2c_notifier_lock);
static p2c_notifier_t p2c_notifiers[P2C_NOTIFIERS_MAX] = {NULL};

int psp_register_cmd_notifier(uint32_t cmd_id, p2c_notifier_t notifier)
{
	int ret = -ENODEV;
	unsigned long flags;

	spin_lock_irqsave(&p2c_notifier_lock, flags);

	if (cmd_id < P2C_NOTIFIERS_MAX && !p2c_notifiers[cmd_id]) {
		p2c_notifiers[cmd_id] = notifier;
		ret = 0;
	}

	spin_unlock_irqrestore(&p2c_notifier_lock, flags);

	return ret;
}
EXPORT_SYMBOL_GPL(psp_register_cmd_notifier);

int psp_unregister_cmd_notifier(uint32_t cmd_id, p2c_notifier_t notifier)
{
	int ret = -ENODEV;
	unsigned long flags;

	spin_lock_irqsave(&p2c_notifier_lock, flags);

	if (cmd_id < P2C_NOTIFIERS_MAX && p2c_notifiers[cmd_id] == notifier) {
		p2c_notifiers[cmd_id] = NULL;
		ret = 0;
	}

	spin_unlock_irqrestore(&p2c_notifier_lock, flags);

	return ret;
}
EXPORT_SYMBOL_GPL(psp_unregister_cmd_notifier);

#define PSP2CPU_MAX_LOOP		100

static irqreturn_t psp_irq_handler_hygon(int irq, void *data)
{
	struct psp_device *psp = data;
	struct sev_device *sev = psp->sev_irq_data;
	unsigned int status;
	int reg;
	unsigned long flags;
	int count = 0;
	uint32_t p2c_cmd;
	uint32_t p2c_lo_data;
	uint32_t p2c_hi_data;
	uint64_t p2c_data;

	/* Read the interrupt status: */
	status = ioread32(psp->io_regs + psp->vdata->intsts_reg);

	while (status && (count++ < PSP2CPU_MAX_LOOP)) {
		/* Clear the interrupt status by writing the same value we read. */
		iowrite32(status, psp->io_regs + psp->vdata->intsts_reg);

		/* Check if it is command completion: */
		if (status & SEV_CMD_COMPLETE) {
			/* Check if it is SEV command completion: */
			reg = ioread32(psp->io_regs + psp->vdata->sev->cmdresp_reg);
			if (reg & PSP_CMDRESP_RESP) {
				if (psp_in_nowait_mode) {
					queue_work(psp_wq, &psp_work);
				} else {
					psp_int_rcvd = 1;
					wake_up(&psp_int_queue);
					if (sev != NULL) {
						sev->int_rcvd = 1;
						wake_up(&sev->int_queue);
					}
				}
			}
		}

		if (status & PSP_X86_CMD) {
			/* Check if it is P2C command completion: */
			reg = ioread32(psp->io_regs + psp->vdata->p2c_cmdresp_reg);
			if (!(reg & PSP_CMDRESP_RESP)) {
				p2c_lo_data = ioread32(psp->io_regs +
						       psp->vdata->p2c_cmdbuff_addr_lo_reg);
				p2c_hi_data = ioread32(psp->io_regs +
						       psp->vdata->p2c_cmdbuff_addr_hi_reg);
				p2c_data = (((uint64_t)(p2c_hi_data) << 32) +
					    ((uint64_t)(p2c_lo_data)));
				p2c_cmd = (uint32_t)(reg & SEV_CMDRESP_IOC);
				if (p2c_cmd < P2C_NOTIFIERS_MAX) {
					spin_lock_irqsave(&p2c_notifier_lock, flags);

					if (p2c_notifiers[p2c_cmd])
						p2c_notifiers[p2c_cmd](p2c_cmd, p2c_data);

					spin_unlock_irqrestore(&p2c_notifier_lock, flags);
				}

				reg |= PSP_CMDRESP_RESP;
				iowrite32(reg, psp->io_regs + psp->vdata->p2c_cmdresp_reg);
			}
		}
		status = ioread32(psp->io_regs + psp->vdata->intsts_reg);
	}

	return IRQ_HANDLED;
}

int sp_request_hygon_psp_irq(struct sp_device *sp, irq_handler_t handler,
			     const char *name, void *data)
{
	return sp_request_psp_irq(sp, psp_irq_handler_hygon, name, data);
}

#else	/* !CONFIG_HYGON_PSP2CPU_CMD */

int sp_request_hygon_psp_irq(struct sp_device *sp, irq_handler_t handler,
			     const char *name, void *data)
{
	return sp_request_psp_irq(sp, handler, name, data);
}

#endif	/* CONFIG_HYGON_PSP2CPU_CMD */

#ifdef CONFIG_PM_SLEEP

void hygon_psp_dev_freeze(struct sp_device *sp)
{
	struct psp_device *psp;

	if (!psp_master)
		return;

	psp = sp->psp_data;
	if (psp == psp_master)
		psp_pci_exit();
}

void hygon_psp_dev_thaw(struct sp_device *sp)
{
	struct psp_device *psp;

	if (!psp_master)
		return;

	psp = sp->psp_data;

	/* re-enable interrupt */
	iowrite32(-1, psp->io_regs + psp->vdata->inten_reg);

	if (psp == psp_master)
		psp_pci_init();
}

void hygon_psp_dev_restore(struct sp_device *sp)
{
	hygon_psp_dev_thaw(sp);
}

#endif

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

#include "psp-dev.h"
#include "vpsp.h"

/* Function and variable pointers for hooks */
struct hygon_psp_hooks_table hygon_psp_hooks;
static unsigned int psp_int_rcvd;
wait_queue_head_t psp_int_queue;

struct kmem_cache *vpsp_cmd_ctx_slab;
static struct workqueue_struct *vpsp_wq;
static struct work_struct vpsp_work;

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
	unsigned long je;

	je = jiffies + msecs_to_jiffies(ms);
	do {
		if (psp_mutex_trylock(mutex)) {
			ret = 1;
			break;
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

		vpsp_wq = create_singlethread_workqueue("vpsp_workqueue");
		if (!vpsp_wq)
			return -ENOMEM;

		INIT_WORK(&vpsp_work, vpsp_worker_handler);

		vpsp_cmd_ctx_slab = kmem_cache_create("vpsp_cmd_ctx",
				sizeof(struct vpsp_cmd_ctx), 0, SLAB_HWCACHE_ALIGN, NULL);
		if (!vpsp_cmd_ctx_slab)
			return -ENOMEM;

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
		misc = &psp_misc->misc;
		misc->minor = MISC_DYNAMIC_MINOR;
		misc->name = "hygon_psp_config";
		misc->fops = &psp_fops;

		ret = misc_register(misc);
		if (ret)
			return ret;
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

	misc_deregister(&misc_dev->misc);
	ClearPageReserved(virt_to_page(misc_dev->data_pg_aligned));
	free_page((unsigned long)misc_dev->data_pg_aligned);
	psp_misc = NULL;
	hygon_psp_hooks.psp_misc = NULL;
	kmem_cache_destroy(vpsp_cmd_ctx_slab);
	flush_workqueue(vpsp_wq);
	destroy_workqueue(vpsp_wq);
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

static int __psp_do_cmd_locked(int cmd, void *data, int *psp_ret)
{
	struct psp_device *psp = psp_master;
	unsigned int phys_lsb, phys_msb;
	unsigned int reg, ret = 0;

	if (!psp || !hygon_psp_hooks.sev_dev_hooks_installed)
		return -ENODEV;

	if (*hygon_psp_hooks.psp_dead)
		return -EBUSY;

	/* Get the physical address of the command buffer */
	phys_lsb = data ? lower_32_bits(__psp_pa(data)) : 0;
	phys_msb = data ? upper_32_bits(__psp_pa(data)) : 0;

	dev_dbg(psp->dev, "psp command id %#x buffer 0x%08x%08x timeout %us\n",
		cmd, phys_msb, phys_lsb, *hygon_psp_hooks.psp_cmd_timeout);

	iowrite32(phys_lsb, psp->io_regs + psp->vdata->sev->cmdbuff_addr_lo_reg);
	iowrite32(phys_msb, psp->io_regs + psp->vdata->sev->cmdbuff_addr_hi_reg);

	psp_int_rcvd = 0;

	reg = FIELD_PREP(SEV_CMDRESP_CMD, cmd) | SEV_CMDRESP_IOC;
	iowrite32(reg, psp->io_regs + psp->vdata->sev->cmdresp_reg);

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

	rc = __psp_do_cmd_locked(cmd, data, psp_ret);
	if (mutex_enabled)
		psp_mutex_unlock(&hygon_psp_hooks.psp_misc->data_pg_aligned->mb_mutex);
	else
		mutex_unlock(hygon_psp_hooks.sev_cmd_mutex);

	return rc;
}
EXPORT_SYMBOL_GPL(psp_do_cmd);

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
				if (vpsp_in_ringbuffer_mode) {
					queue_work(vpsp_wq, &vpsp_work);
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

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
#include <linux/sort.h>
#include <linux/bsearch.h>
#include <linux/rwlock.h>

#include "psp-dev.h"

/* Function and variable pointers for hooks */
struct hygon_psp_hooks_table hygon_psp_hooks;

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

enum VPSP_DEV_CTRL_OPCODE {
	VPSP_OP_VID_ADD,
	VPSP_OP_VID_DEL,
	VPSP_OP_SET_DEFAULT_VID_PERMISSION,
	VPSP_OP_GET_DEFAULT_VID_PERMISSION,
	VPSP_OP_SET_GPA,
};

struct vpsp_dev_ctrl {
	unsigned char op;
	/**
	 * To be compatible with old user mode,
	 * struct vpsp_dev_ctrl must be kept at 132 bytes.
	 */
	unsigned char resv[3];
	union {
		unsigned int vid;
		// Set or check the permissions for the default VID
		unsigned int def_vid_perm;
		struct {
			u64 gpa_start;
			u64 gpa_end;
		} gpa;
		unsigned char reserved[128];
	} __packed data;
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
EXPORT_SYMBOL_GPL(psp_mutex_lock_timeout);

int psp_mutex_unlock(struct psp_mutex *mutex)
{
	if (!mutex)
		return -1;

	atomic64_exchange(&mutex->locked, 0);
	return 0;
}
EXPORT_SYMBOL_GPL(psp_mutex_unlock);

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
DEFINE_RWLOCK(vpsp_rwlock);

/* VPSP_VID_MAX_ENTRIES determines the maximum number of vms that can set vid.
 * but, the performance of finding vid is determined by g_vpsp_vid_num,
 * so VPSP_VID_MAX_ENTRIES can be set larger.
 */
#define VPSP_VID_MAX_ENTRIES    2048
#define VPSP_VID_NUM_MAX        64

static struct vpsp_context g_vpsp_context_array[VPSP_VID_MAX_ENTRIES];
static uint32_t g_vpsp_vid_num;
static int compare_vid_entries(const void *a, const void *b)
{
	return ((struct vpsp_context *)a)->pid - ((struct vpsp_context *)b)->pid;
}
static void swap_vid_entries(void *a, void *b, int size)
{
	struct vpsp_context entry;

	memcpy(&entry, a, size);
	memcpy(a, b, size);
	memcpy(b, &entry, size);
}

/**
 * When 'allow_default_vid' is set to 1,
 * QEMU is allowed to use 'vid 0' by default
 * in the absence of a valid 'vid' setting.
 */
uint32_t allow_default_vid = 1;
void vpsp_set_default_vid_permission(uint32_t is_allow)
{
	allow_default_vid = is_allow;
}

int vpsp_get_default_vid_permission(void)
{
	return allow_default_vid;
}
EXPORT_SYMBOL_GPL(vpsp_get_default_vid_permission);

/**
 * get a vpsp context from pid
 */
int vpsp_get_context(struct vpsp_context **ctx, pid_t pid)
{
	struct vpsp_context new_entry = {.pid = pid};
	struct vpsp_context *existing_entry = NULL;

	read_lock(&vpsp_rwlock);
	existing_entry = bsearch(&new_entry, g_vpsp_context_array, g_vpsp_vid_num,
				sizeof(struct vpsp_context), compare_vid_entries);
	read_unlock(&vpsp_rwlock);

	if (!existing_entry)
		return -ENOENT;

	if (ctx)
		*ctx = existing_entry;

	return 0;
}
EXPORT_SYMBOL_GPL(vpsp_get_context);

/**
 * Upon qemu startup, this section checks whether
 * the '-device psp,vid' parameter is specified.
 * If set, it utilizes the 'vpsp_add_vid' function
 * to insert the 'vid' and 'pid' values into the 'g_vpsp_context_array'.
 * The insertion is done in ascending order of 'pid'.
 */
static int vpsp_add_vid(uint32_t vid)
{
	pid_t cur_pid = task_pid_nr(current);
	struct vpsp_context new_entry = {.vid = vid, .pid = cur_pid};

	if (vpsp_get_context(NULL, cur_pid) == 0)
		return -EEXIST;
	if (g_vpsp_vid_num == VPSP_VID_MAX_ENTRIES)
		return -ENOMEM;
	if (vid >= VPSP_VID_NUM_MAX)
		return -EINVAL;

	write_lock(&vpsp_rwlock);
	memcpy(&g_vpsp_context_array[g_vpsp_vid_num++], &new_entry, sizeof(struct vpsp_context));
	sort(g_vpsp_context_array, g_vpsp_vid_num, sizeof(struct vpsp_context),
				compare_vid_entries, swap_vid_entries);
	pr_info("PSP: add vid %d, by pid %d, total vid num is %d\n", vid, cur_pid, g_vpsp_vid_num);
	write_unlock(&vpsp_rwlock);
	return 0;
}

/**
 * Upon the virtual machine is shut down,
 * the 'vpsp_del_vid' function is employed to remove
 * the 'vid' associated with the current 'pid'.
 */
static int vpsp_del_vid(void)
{
	pid_t cur_pid = task_pid_nr(current);
	int i, ret = -ENOENT;

	write_lock(&vpsp_rwlock);
	for (i = 0; i < g_vpsp_vid_num; ++i) {
		if (g_vpsp_context_array[i].pid == cur_pid) {
			--g_vpsp_vid_num;
			pr_info("PSP: delete vid %d, by pid %d, total vid num is %d\n",
				g_vpsp_context_array[i].vid, cur_pid, g_vpsp_vid_num);
			memmove(&g_vpsp_context_array[i], &g_vpsp_context_array[i + 1],
				sizeof(struct vpsp_context) * (g_vpsp_vid_num - i));
			ret = 0;
			goto end;
		}
	}

end:
	write_unlock(&vpsp_rwlock);
	return ret;
}

static int vpsp_set_gpa_range(u64 gpa_start, u64 gpa_end)
{
	pid_t cur_pid = task_pid_nr(current);
	struct vpsp_context *ctx = NULL;

	vpsp_get_context(&ctx, cur_pid);
	if (!ctx) {
		pr_err("PSP: %s get vpsp_context failed from pid %d\n", __func__, cur_pid);
		return -ENOENT;
	}

	ctx->gpa_start = gpa_start;
	ctx->gpa_end = gpa_end;
	pr_info("PSP: set gpa range (start 0x%llx, end 0x%llx), by pid %d\n",
		gpa_start, gpa_end, cur_pid);
	return 0;
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

static int do_vpsp_op_ioctl(struct vpsp_dev_ctrl *ctrl)
{
	int ret = 0;
	unsigned char op = ctrl->op;

	switch (op) {
	case VPSP_OP_VID_ADD:
		ret = vpsp_add_vid(ctrl->data.vid);
		break;

	case VPSP_OP_VID_DEL:
		ret = vpsp_del_vid();
		break;

	case VPSP_OP_SET_DEFAULT_VID_PERMISSION:
		vpsp_set_default_vid_permission(ctrl->data.def_vid_perm);
		break;

	case VPSP_OP_GET_DEFAULT_VID_PERMISSION:
		ctrl->data.def_vid_perm = vpsp_get_default_vid_permission();
		break;

	case VPSP_OP_SET_GPA:
		ret = vpsp_set_gpa_range(ctrl->data.gpa.gpa_start, ctrl->data.gpa.gpa_end);
		break;

	default:
		ret = -EINVAL;
		break;
	}
	return ret;
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
EXPORT_SYMBOL_GPL(hygon_psp_additional_setup);

void hygon_psp_exit(struct kref *ref)
{
	struct psp_misc_dev *misc_dev = container_of(ref, struct psp_misc_dev, refcount);

	misc_deregister(&misc_dev->misc);
	ClearPageReserved(virt_to_page(misc_dev->data_pg_aligned));
	free_page((unsigned long)misc_dev->data_pg_aligned);
	psp_misc = NULL;
	hygon_psp_hooks.psp_misc = NULL;
}
EXPORT_SYMBOL_GPL(hygon_psp_exit);

int fixup_hygon_psp_caps(struct psp_device *psp)
{
	/* the hygon psp is unavailable if bit0 is cleared in feature reg */
	if (!(psp->capability & PSP_CAPABILITY_SEV))
		return -ENODEV;

	psp->capability &= ~(PSP_CAPABILITY_TEE |
			     PSP_CAPABILITY_PSP_SECURITY_REPORTING);
	return 0;
}

static int __psp_do_cmd_locked(int cmd, void *data, int *psp_ret)
{
	struct psp_device *psp = psp_master;
	struct sev_device *sev;
	unsigned int phys_lsb, phys_msb;
	unsigned int reg, ret = 0;

	if (!psp || !psp->sev_data || !hygon_psp_hooks.sev_dev_hooks_installed)
		return -ENODEV;

	if (*hygon_psp_hooks.psp_dead)
		return -EBUSY;

	sev = psp->sev_data;

	/* Get the physical address of the command buffer */
	phys_lsb = data ? lower_32_bits(__psp_pa(data)) : 0;
	phys_msb = data ? upper_32_bits(__psp_pa(data)) : 0;

	dev_dbg(sev->dev, "sev command id %#x buffer 0x%08x%08x timeout %us\n",
		cmd, phys_msb, phys_lsb, *hygon_psp_hooks.psp_timeout);

	print_hex_dump_debug("(in):  ", DUMP_PREFIX_OFFSET, 16, 2, data,
			     hygon_psp_hooks.sev_cmd_buffer_len(cmd), false);

	iowrite32(phys_lsb, sev->io_regs + sev->vdata->cmdbuff_addr_lo_reg);
	iowrite32(phys_msb, sev->io_regs + sev->vdata->cmdbuff_addr_hi_reg);

	sev->int_rcvd = 0;

	reg = FIELD_PREP(SEV_CMDRESP_CMD, cmd) | SEV_CMDRESP_IOC;
	iowrite32(reg, sev->io_regs + sev->vdata->cmdresp_reg);

	/* wait for command completion */
	ret = hygon_psp_hooks.sev_wait_cmd_ioc(sev, &reg, *hygon_psp_hooks.psp_timeout);
	if (ret) {
		if (psp_ret)
			*psp_ret = 0;

		dev_err(sev->dev, "sev command %#x timed out, disabling PSP\n", cmd);
		*hygon_psp_hooks.psp_dead = true;

		return ret;
	}

	*hygon_psp_hooks.psp_timeout = *hygon_psp_hooks.psp_cmd_timeout;

	if (psp_ret)
		*psp_ret = FIELD_GET(PSP_CMDRESP_STS, reg);

	if (FIELD_GET(PSP_CMDRESP_STS, reg)) {
		dev_dbg(sev->dev, "sev command %#x failed (%#010lx)\n",
			cmd, FIELD_GET(PSP_CMDRESP_STS, reg));
		ret = -EIO;
	}

	print_hex_dump_debug("(out): ", DUMP_PREFIX_OFFSET, 16, 2, data,
			     hygon_psp_hooks.sev_cmd_buffer_len(cmd), false);

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
				sev->int_rcvd = 1;
				wake_up(&sev->int_queue);
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

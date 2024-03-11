// SPDX-License-Identifier: GPL-2.0-only
/*
 * HYGON CSV interface
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
#include <uapi/linux/psp-hygon.h>

#include <asm/csv.h>

#include "psp-dev.h"
#include "csv-dev.h"
#include "ring-buffer.h"

/*
 * Hygon CSV build info:
 *    Hygon CSV build info is 32-bit in length other than 8-bit as that
 *    in AMD SEV.
 */
u32 hygon_csv_build;

int csv_comm_mode = CSV_COMM_MAILBOX_ON;

/*
 * csv_update_api_version used to update the api version of HYGON CSV
 * firmwareat driver side.
 * Currently, we only need to update @hygon_csv_build.
 */
void csv_update_api_version(struct sev_user_data_status *status)
{
	if (status) {
		hygon_csv_build = (status->flags >> 9) |
				   ((u32)status->build << 23);
	}
}

int csv_cmd_buffer_len(int cmd)
{
	switch (cmd) {
	case CSV_CMD_HGSC_CERT_IMPORT:		return sizeof(struct csv_data_hgsc_cert_import);
	case CSV_CMD_RING_BUFFER:		return sizeof(struct csv_data_ring_buffer);
	case CSV3_CMD_LAUNCH_ENCRYPT_DATA:	return sizeof(struct csv3_data_launch_encrypt_data);
	case CSV3_CMD_LAUNCH_ENCRYPT_VMCB:	return sizeof(struct csv3_data_launch_encrypt_vmcb);
	case CSV3_CMD_UPDATE_NPT:		return sizeof(struct csv3_data_update_npt);
	case CSV3_CMD_SET_SMR:			return sizeof(struct csv3_data_set_smr);
	case CSV3_CMD_SET_SMCR:			return sizeof(struct csv3_data_set_smcr);
	case CSV3_CMD_SET_GUEST_PRIVATE_MEMORY:
					return sizeof(struct csv3_data_set_guest_private_memory);
	case CSV3_CMD_DBG_READ_VMSA:		return sizeof(struct csv3_data_dbg_read_vmsa);
	case CSV3_CMD_DBG_READ_MEM:		return sizeof(struct csv3_data_dbg_read_mem);
	default:				return 0;
	}
}

static int csv_ioctl_do_hgsc_import(struct sev_issue_cmd *argp)
{
	struct csv_user_data_hgsc_cert_import input;
	struct csv_data_hgsc_cert_import *data;
	void *hgscsk_blob, *hgsc_blob;
	int ret;

	if (copy_from_user(&input, (void __user *)argp->data, sizeof(input)))
		return -EFAULT;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	/* copy HGSCSK certificate blobs from userspace */
	hgscsk_blob = psp_copy_user_blob(input.hgscsk_cert_address, input.hgscsk_cert_len);
	if (IS_ERR(hgscsk_blob)) {
		ret = PTR_ERR(hgscsk_blob);
		goto e_free;
	}

	data->hgscsk_cert_address = __psp_pa(hgscsk_blob);
	data->hgscsk_cert_len = input.hgscsk_cert_len;

	/* copy HGSC certificate blobs from userspace */
	hgsc_blob = psp_copy_user_blob(input.hgsc_cert_address, input.hgsc_cert_len);
	if (IS_ERR(hgsc_blob)) {
		ret = PTR_ERR(hgsc_blob);
		goto e_free_hgscsk;
	}

	data->hgsc_cert_address = __psp_pa(hgsc_blob);
	data->hgsc_cert_len = input.hgsc_cert_len;

	ret = hygon_psp_hooks.__sev_do_cmd_locked(CSV_CMD_HGSC_CERT_IMPORT,
						  data, &argp->error);

	kfree(hgsc_blob);
e_free_hgscsk:
	kfree(hgscsk_blob);
e_free:
	kfree(data);
	return ret;
}

static int csv_ioctl_do_download_firmware(struct sev_issue_cmd *argp)
{
	struct sev_data_download_firmware *data = NULL;
	struct csv_user_data_download_firmware input;
	int ret, order;
	struct page *p;
	u64 data_size;

	/* Only support DOWNLOAD_FIRMWARE if build greater or equal 1667 */
	if (!csv_version_greater_or_equal(1667)) {
		pr_err("DOWNLOAD_FIRMWARE not supported\n");
		return -EIO;
	}

	if (copy_from_user(&input, (void __user *)argp->data, sizeof(input)))
		return -EFAULT;

	if (!input.address) {
		argp->error = SEV_RET_INVALID_ADDRESS;
		return -EINVAL;
	}

	if (!input.length || input.length > CSV_FW_MAX_SIZE) {
		argp->error = SEV_RET_INVALID_LEN;
		return -EINVAL;
	}

	/*
	 * CSV FW expects the physical address given to it to be 32
	 * byte aligned. Memory allocated has structure placed at the
	 * beginning followed by the firmware being passed to the CSV
	 * FW. Allocate enough memory for data structure + alignment
	 * padding + CSV FW.
	 */
	data_size = ALIGN(sizeof(struct sev_data_download_firmware), 32);

	order = get_order(input.length + data_size);
	p = alloc_pages(GFP_KERNEL, order);
	if (!p)
		return -ENOMEM;

	/*
	 * Copy firmware data to a kernel allocated contiguous
	 * memory region.
	 */
	data = page_address(p);
	if (copy_from_user((void *)(page_address(p) + data_size),
			   (void *)input.address, input.length)) {
		ret = -EFAULT;
		goto err_free_page;
	}

	data->address = __psp_pa(page_address(p) + data_size);
	data->len = input.length;

	ret = hygon_psp_hooks.__sev_do_cmd_locked(SEV_CMD_DOWNLOAD_FIRMWARE,
						  data, &argp->error);
	if (ret)
		pr_err("Failed to update CSV firmware: %#x\n", argp->error);
	else
		pr_info("CSV firmware update successful\n");

err_free_page:
	__free_pages(p, order);

	return ret;
}

static long csv_ioctl(struct file *file, unsigned int ioctl, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct sev_issue_cmd input;
	int ret = -EFAULT;

	if (!hygon_psp_hooks.sev_dev_hooks_installed)
		return -ENODEV;

	if (!psp_master || !psp_master->sev_data)
		return -ENODEV;

	if (ioctl != SEV_ISSUE_CMD)
		return -EINVAL;

	if (copy_from_user(&input, argp, sizeof(struct sev_issue_cmd)))
		return -EFAULT;

	if (input.cmd > CSV_MAX)
		return -EINVAL;

	mutex_lock(hygon_psp_hooks.sev_cmd_mutex);

	switch (input.cmd) {
	case CSV_HGSC_CERT_IMPORT:
		ret = csv_ioctl_do_hgsc_import(&input);
		break;
	case CSV_PLATFORM_INIT:
		ret = hygon_psp_hooks.__sev_platform_init_locked(&input.error);
		break;
	case CSV_PLATFORM_SHUTDOWN:
		ret = hygon_psp_hooks.__sev_platform_shutdown_locked(&input.error);
		break;
	case CSV_DOWNLOAD_FIRMWARE:
		ret = csv_ioctl_do_download_firmware(&input);
		break;
	default:
		/*
		 * If the command is compatible between CSV and SEV, the
		 * native implementation of the driver is invoked.
		 * Release the mutex before calling the native ioctl function
		 * because it will acquires the mutex.
		 */
		mutex_unlock(hygon_psp_hooks.sev_cmd_mutex);
		return hygon_psp_hooks.sev_ioctl(file, ioctl, arg);
	}

	if (copy_to_user(argp, &input, sizeof(struct sev_issue_cmd)))
		ret = -EFAULT;

	mutex_unlock(hygon_psp_hooks.sev_cmd_mutex);

	return ret;
}

const struct file_operations csv_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = csv_ioctl,
};

/*
 * __csv_ring_buffer_enter_locked issues command to switch to RING BUFFER
 * mode, the caller must acquire the mutex lock.
 */
static int __csv_ring_buffer_enter_locked(int *error)
{
	struct psp_device *psp = psp_master;
	struct sev_device *sev;
	struct csv_data_ring_buffer *data;
	struct csv_ringbuffer_queue *low_queue;
	struct csv_ringbuffer_queue *hi_queue;
	int ret = 0;

	if (!psp || !psp->sev_data || !hygon_psp_hooks.sev_dev_hooks_installed)
		return -ENODEV;

	sev = psp->sev_data;

	if (csv_comm_mode == CSV_COMM_RINGBUFFER_ON)
		return -EEXIST;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	low_queue = &sev->ring_buffer[CSV_COMMAND_PRIORITY_LOW];
	hi_queue = &sev->ring_buffer[CSV_COMMAND_PRIORITY_HIGH];

	data->queue_lo_cmdptr_address = __psp_pa(low_queue->cmd_ptr.data_align);
	data->queue_lo_statval_address = __psp_pa(low_queue->stat_val.data_align);
	data->queue_hi_cmdptr_address = __psp_pa(hi_queue->cmd_ptr.data_align);
	data->queue_hi_statval_address = __psp_pa(hi_queue->stat_val.data_align);
	data->queue_lo_size = 1;
	data->queue_hi_size = 1;
	data->int_on_empty = 1;

	ret = hygon_psp_hooks.__sev_do_cmd_locked(CSV_CMD_RING_BUFFER, data, error);
	if (!ret) {
		iowrite32(0, sev->io_regs + sev->vdata->cmdbuff_addr_hi_reg);
		csv_comm_mode = CSV_COMM_RINGBUFFER_ON;
	}

	kfree(data);
	return ret;
}

static int csv_wait_cmd_ioc_ring_buffer(struct sev_device *sev,
					unsigned int *reg,
					unsigned int timeout)
{
	int ret;

	ret = wait_event_timeout(sev->int_queue,
			sev->int_rcvd, timeout * HZ);
	if (!ret)
		return -ETIMEDOUT;

	*reg = ioread32(sev->io_regs + sev->vdata->cmdbuff_addr_lo_reg);

	return 0;
}

static int csv_get_cmd_status(struct sev_device *sev, int prio, int index)
{
	struct csv_queue *queue = &sev->ring_buffer[prio].stat_val;
	struct csv_statval_entry *statval = (struct csv_statval_entry *)queue->data;

	return statval[index].status;
}

static int __csv_do_ringbuf_cmds_locked(int *psp_ret)
{
	struct psp_device *psp = psp_master;
	struct sev_device *sev;
	unsigned int rb_tail;
	unsigned int rb_ctl;
	int last_cmd_index;
	unsigned int reg, ret = 0;

	if (!psp || !psp->sev_data)
		return -ENODEV;

	if (*hygon_psp_hooks.psp_dead)
		return -EBUSY;

	sev = psp->sev_data;

	/* update rb tail */
	rb_tail = ioread32(sev->io_regs + sev->vdata->cmdbuff_addr_hi_reg);
	rb_tail &= (~PSP_RBTAIL_QHI_TAIL_MASK);
	rb_tail |= (sev->ring_buffer[CSV_COMMAND_PRIORITY_HIGH].cmd_ptr.tail
						<< PSP_RBTAIL_QHI_TAIL_SHIFT);
	rb_tail &= (~PSP_RBTAIL_QLO_TAIL_MASK);
	rb_tail |= sev->ring_buffer[CSV_COMMAND_PRIORITY_LOW].cmd_ptr.tail;
	iowrite32(rb_tail, sev->io_regs + sev->vdata->cmdbuff_addr_hi_reg);

	/* update rb ctl to trigger psp irq */
	sev->int_rcvd = 0;

	/* PSP response to x86 only when all queue is empty or error happends */
	rb_ctl = PSP_RBCTL_X86_WRITES |
		 PSP_RBCTL_RBMODE_ACT |
		 PSP_RBCTL_CLR_INTSTAT;
	iowrite32(rb_ctl, sev->io_regs + sev->vdata->cmdresp_reg);

	/* wait for all commands in ring buffer completed */
	ret = csv_wait_cmd_ioc_ring_buffer(sev, &reg,
					   (*hygon_psp_hooks.psp_timeout) * 10);
	if (ret) {
		if (psp_ret)
			*psp_ret = 0;
		dev_err(sev->dev, "csv ringbuffer mode command timed out, disabling PSP\n");
		*hygon_psp_hooks.psp_dead = true;

		return ret;
	}

	/* cmd error happends */
	if (reg & PSP_RBHEAD_QPAUSE_INT_STAT)
		ret = -EFAULT;

	if (psp_ret) {
		last_cmd_index = (reg & PSP_RBHEAD_QHI_HEAD_MASK)
					>> PSP_RBHEAD_QHI_HEAD_SHIFT;
		*psp_ret = csv_get_cmd_status(sev, CSV_COMMAND_PRIORITY_HIGH,
					      last_cmd_index);
		if (*psp_ret == 0) {
			last_cmd_index = reg & PSP_RBHEAD_QLO_HEAD_MASK;
			*psp_ret = csv_get_cmd_status(sev,
					CSV_COMMAND_PRIORITY_LOW, last_cmd_index);
		}
	}

	return ret;
}

/*
 * csv_do_ringbuf_cmds will enter RING BUFFER mode and handling commands
 * queued in RING BUFFER queues, the user is obligate to manage RING
 * BUFFER queues including allocate, enqueue and free, etc.
 */
static int csv_do_ringbuf_cmds(int *psp_ret)
{
	struct sev_user_data_status data;
	int rc;

	if (!hygon_psp_hooks.sev_dev_hooks_installed)
		return -ENODEV;

	mutex_lock(hygon_psp_hooks.sev_cmd_mutex);

	rc = __csv_ring_buffer_enter_locked(psp_ret);
	if (rc)
		goto cmd_unlock;

	rc = __csv_do_ringbuf_cmds_locked(psp_ret);

	/* exit ringbuf mode by send CMD in mailbox mode */
	hygon_psp_hooks.__sev_do_cmd_locked(SEV_CMD_PLATFORM_STATUS, &data, NULL);
	csv_comm_mode = CSV_COMM_MAILBOX_ON;

cmd_unlock:
	mutex_unlock(hygon_psp_hooks.sev_cmd_mutex);

	return rc;
}

int csv_issue_ringbuf_cmds_external_user(struct file *filep, int *psp_ret)
{
	if (!filep || filep->f_op != &csv_fops)
		return -EBADF;

	return csv_do_ringbuf_cmds(psp_ret);
}
EXPORT_SYMBOL_GPL(csv_issue_ringbuf_cmds_external_user);

void csv_restore_mailbox_mode_postprocess(void)
{
	csv_comm_mode = CSV_COMM_MAILBOX_ON;
	csv_ring_buffer_queue_free();
}

/*
 * __csv_ring_buffer_queue_init will allocate memory for command queue
 * and status queue. If error occurs, this function will return directly,
 * the caller must free the memories allocated for queues.
 *
 * Function csv_ring_buffer_queue_free() can be used to handling error
 * return by this function and cleanup ring buffer queues when exiting
 * from RING BUFFER mode.
 *
 * Return -ENOMEM if fail to allocate memory for queues, otherwise 0
 */
static int __csv_ring_buffer_queue_init(struct csv_ringbuffer_queue *ring_buffer)
{
	void *cmd_ptr_buffer = NULL;
	void *stat_val_buffer = NULL;

	/* If reach here, the command and status queues must be NULL */
	WARN_ON(ring_buffer->cmd_ptr.data ||
		ring_buffer->stat_val.data);

	cmd_ptr_buffer = kzalloc(CSV_RING_BUFFER_LEN, GFP_KERNEL);
	if (!cmd_ptr_buffer)
		return -ENOMEM;

	/* the command queue will points to @cmd_ptr_buffer */
	csv_queue_init(&ring_buffer->cmd_ptr, cmd_ptr_buffer,
		       CSV_RING_BUFFER_LEN, CSV_RING_BUFFER_ESIZE);

	stat_val_buffer = kzalloc(CSV_RING_BUFFER_LEN, GFP_KERNEL);
	if (!stat_val_buffer)
		return -ENOMEM;

	/* the status queue will points to @stat_val_buffer */
	csv_queue_init(&ring_buffer->stat_val, stat_val_buffer,
		       CSV_RING_BUFFER_LEN, CSV_RING_BUFFER_ESIZE);
	return 0;
}

int csv_ring_buffer_queue_init(void)
{
	struct psp_device *psp = psp_master;
	struct sev_device *sev;
	int i, ret = 0;

	if (!psp || !psp->sev_data)
		return -ENODEV;

	sev = psp->sev_data;

	for (i = CSV_COMMAND_PRIORITY_HIGH; i < CSV_COMMAND_PRIORITY_NUM; i++) {
		ret = __csv_ring_buffer_queue_init(&sev->ring_buffer[i]);
		if (ret)
			goto e_free;
	}

	return 0;

e_free:
	csv_ring_buffer_queue_free();
	return ret;
}
EXPORT_SYMBOL_GPL(csv_ring_buffer_queue_init);

int csv_ring_buffer_queue_free(void)
{
	struct psp_device *psp = psp_master;
	struct sev_device *sev;
	struct csv_ringbuffer_queue *ring_buffer;
	int i;

	if (!psp || !psp->sev_data)
		return -ENODEV;

	sev = psp->sev_data;

	for (i = 0; i < CSV_COMMAND_PRIORITY_NUM; i++) {
		ring_buffer = &sev->ring_buffer[i];

		/*
		 * If command queue is not NULL, it must points to memory
		 * that allocated in __csv_ring_buffer_queue_init().
		 */
		if (ring_buffer->cmd_ptr.data) {
			kfree((void *)ring_buffer->cmd_ptr.data);
			csv_queue_cleanup(&ring_buffer->cmd_ptr);
		}

		/*
		 * If status queue is not NULL, it must points to memory
		 * that allocated in __csv_ring_buffer_queue_init().
		 */
		if (ring_buffer->stat_val.data) {
			kfree((void *)ring_buffer->stat_val.data);
			csv_queue_cleanup(&ring_buffer->stat_val);
		}
	}
	return 0;
}
EXPORT_SYMBOL_GPL(csv_ring_buffer_queue_free);

int csv_fill_cmd_queue(int prio, int cmd, void *data, uint16_t flags)
{
	struct psp_device *psp = psp_master;
	struct sev_device *sev;
	struct csv_cmdptr_entry cmdptr = { };

	if (!psp || !psp->sev_data)
		return -ENODEV;

	sev = psp->sev_data;

	cmdptr.cmd_buf_ptr = __psp_pa(data);
	cmdptr.cmd_id = cmd;
	cmdptr.cmd_flags = flags;

	if (csv_enqueue_cmd(&sev->ring_buffer[prio].cmd_ptr, &cmdptr, 1) != 1)
		return -EFAULT;

	return 0;
}
EXPORT_SYMBOL_GPL(csv_fill_cmd_queue);

int csv_check_stat_queue_status(int *psp_ret)
{
	struct psp_device *psp = psp_master;
	struct sev_device *sev;
	unsigned int len;
	int prio;

	if (!psp || !psp->sev_data)
		return -ENODEV;

	sev = psp->sev_data;

	for (prio = CSV_COMMAND_PRIORITY_HIGH;
	     prio < CSV_COMMAND_PRIORITY_NUM; prio++) {
		do {
			struct csv_statval_entry statval;

			len = csv_dequeue_stat(&sev->ring_buffer[prio].stat_val,
					       &statval, 1);
			if (len) {
				if (statval.status != 0) {
					*psp_ret = statval.status;
					return -EFAULT;
				}
			}
		} while (len);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(csv_check_stat_queue_status);

#ifdef CONFIG_HYGON_CSV

int csv_platform_cmd_set_secure_memory_region(struct sev_device *sev, int *error)
{
	int ret = 0;
	unsigned int i = 0;
	struct csv3_data_set_smr *cmd_set_smr;
	struct csv3_data_set_smcr *cmd_set_smcr;
	struct csv3_data_memory_region *smr_regions;

	if (!hygon_psp_hooks.sev_dev_hooks_installed) {
		ret = -ENODEV;
		goto l_end;
	}

	if (!csv_smr || !csv_smr_num) {
		ret = -EINVAL;
		goto l_end;
	}

	cmd_set_smr = kzalloc(sizeof(*cmd_set_smr), GFP_KERNEL);
	if (!cmd_set_smr) {
		ret = -ENOMEM;
		goto l_end;
	}

	smr_regions = kcalloc(csv_smr_num, sizeof(*smr_regions),  GFP_KERNEL);
	if (!smr_regions) {
		ret = -ENOMEM;
		goto e_free_cmd_set_smr;
	}

	for (i = 0; i < csv_smr_num; i++) {
		smr_regions[i].base_address = csv_smr[i].start;
		smr_regions[i].size = csv_smr[i].size;
	}
	cmd_set_smr->smr_entry_size = 1 << csv_get_smr_entry_shift();
	cmd_set_smr->regions_paddr = __psp_pa(smr_regions);
	cmd_set_smr->nregions = csv_smr_num;
	ret = hygon_psp_hooks.sev_do_cmd(CSV3_CMD_SET_SMR, cmd_set_smr, error);
	if (ret) {
		pr_err("Fail to set SMR, ret %#x, error %#x\n", ret, *error);
		goto e_free_smr_area;
	}

	cmd_set_smcr = kzalloc(sizeof(*cmd_set_smcr), GFP_KERNEL);
	if (!cmd_set_smcr) {
		ret = -ENOMEM;
		goto e_free_smr_area;
	}

	cmd_set_smcr->base_address = csv_alloc_from_contiguous(1UL << CSV_MR_ALIGN_BITS,
						&node_online_map,
						get_order(1 << CSV_MR_ALIGN_BITS));
	if (!cmd_set_smcr->base_address) {
		pr_err("Fail to alloc SMCR memory\n");
		ret = -ENOMEM;
		goto e_free_cmd_set_smcr;
	}

	cmd_set_smcr->size = 1UL << CSV_MR_ALIGN_BITS;
	ret = hygon_psp_hooks.sev_do_cmd(CSV3_CMD_SET_SMCR, cmd_set_smcr, error);
	if (ret) {
		if (*error == SEV_RET_INVALID_COMMAND)
			ret = 0;
		else
			pr_err("set smcr ret %#x, error %#x\n", ret, *error);

		csv_release_to_contiguous(cmd_set_smcr->base_address,
					1UL << CSV_MR_ALIGN_BITS);
	}

e_free_cmd_set_smcr:
	kfree((void *)cmd_set_smcr);
e_free_smr_area:
	kfree((void *)smr_regions);
e_free_cmd_set_smr:
	kfree((void *)cmd_set_smr);

l_end:
	if (ret)
		dev_warn(sev->dev,
			 "CSV3: fail to set secure memory region, CSV3 support unavailable\n");

	return ret;
}

#else	/* !CONFIG_HYGON_CSV */

int csv_platform_cmd_set_secure_memory_region(struct sev_device *sev, int *error)
{
	dev_warn(sev->dev,
		 "CSV3: needs CONFIG_HYGON_CSV, CSV3 support unavailable\n");
	return -EFAULT;
}

#endif	/* CONFIG_HYGON_CSV */

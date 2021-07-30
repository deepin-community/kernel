// SPDX-License-Identifier: GPL-2.0-only
/*
 * CSV driver for KVM
 *
 * HYGON CSV support
 *
 * Copyright (C) Hygon Info Technologies Ltd.
 */

#include <linux/kvm_host.h>
#include <linux/psp-sev.h>
#include <linux/psp-hygon.h>
#include <linux/memory.h>
#include <linux/kvm_types.h>
#include <asm/cacheflush.h>
#include "kvm_cache_regs.h"
#include "svm.h"
#include "csv.h"
#include "x86.h"

#undef  pr_fmt
#define pr_fmt(fmt) "CSV: " fmt

/* Function and variable pointers for hooks */
struct hygon_kvm_hooks_table hygon_kvm_hooks;

static struct kvm_x86_ops csv_x86_ops;
static const char csv_vm_mnonce[] = "VM_ATTESTATION";
static DEFINE_MUTEX(csv_cmd_batch_mutex);

static int __csv_issue_ringbuf_cmds(int fd, int *psp_ret)
{
	struct fd f;
	int ret;

	f = fdget(fd);
	if (!f.file)
		return -EBADF;

	ret = csv_issue_ringbuf_cmds_external_user(f.file, psp_ret);

	fdput(f);
	return ret;
}

static int csv_issue_ringbuf_cmds(struct kvm *kvm, int *psp_ret)
{
	struct kvm_sev_info *sev = &to_kvm_svm(kvm)->sev_info;

	return __csv_issue_ringbuf_cmds(sev->fd, psp_ret);
}

int csv_vm_attestation(struct kvm *kvm, unsigned long gpa, unsigned long len)
{
	struct kvm_sev_info *sev = &to_kvm_svm(kvm)->sev_info;
	struct sev_data_attestation_report *data = NULL;
	struct page **pages;
	unsigned long guest_uaddr, n;
	int ret = 0, offset, error;

	if (!sev_guest(kvm) || !hygon_kvm_hooks.sev_hooks_installed)
		return -ENOTTY;

	/*
	 * The physical address of guest must valid and page aligned, and
	 * the length of guest memory region must be page size aligned.
	 */
	if (!gpa || (gpa & ~PAGE_MASK) || (len & ~PAGE_MASK)) {
		pr_err("invalid guest address or length\n");
		return -EFAULT;
	}

	guest_uaddr = gfn_to_hva(kvm, gpa_to_gfn(gpa));
	pages = hygon_kvm_hooks.sev_pin_memory(kvm, guest_uaddr, len, &n, 1);
	if (IS_ERR(pages))
		return PTR_ERR(pages);

	/*
	 * The attestation report must be copied into contiguous memory region,
	 * lets verify that userspace memory pages are contiguous before we
	 * issue commmand.
	 */
	if (hygon_kvm_hooks.get_num_contig_pages(0, pages, n) != n) {
		ret = -EINVAL;
		goto e_unpin_memory;
	}

	ret = -ENOMEM;
	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		goto e_unpin_memory;

	/* csv_vm_mnonce indicates attestation request from guest */
	if (sizeof(csv_vm_mnonce) >= sizeof(data->mnonce)) {
		ret = -EINVAL;
		goto e_free;
	}

	memcpy(data->mnonce, csv_vm_mnonce, sizeof(csv_vm_mnonce));

	offset = guest_uaddr & (PAGE_SIZE - 1);
	data->address = __sme_page_pa(pages[0]) + offset;
	data->len = len;

	data->handle = sev->handle;
	ret = hygon_kvm_hooks.sev_issue_cmd(kvm, SEV_CMD_ATTESTATION_REPORT,
					    data, &error);

	if (ret)
		pr_err("vm attestation ret %#x, error %#x\n", ret, error);

e_free:
	kfree(data);
e_unpin_memory:
	hygon_kvm_hooks.sev_unpin_memory(kvm, pages, n);
	return ret;
}

static int csv_ringbuf_infos_free(struct kvm *kvm,
				  struct csv_ringbuf_infos *ringbuf_infos)
{
	int i;

	for (i = 0; i < ringbuf_infos->num; i++) {
		struct csv_ringbuf_info_item *item = ringbuf_infos->item[i];

		if (item) {
			if (item->data_vaddr)
				kfree((void *)item->data_vaddr);

			if (item->hdr_vaddr)
				kfree((void *)item->hdr_vaddr);

			if (item->pages)
				hygon_kvm_hooks.sev_unpin_memory(kvm, item->pages,
								 item->n);

			kfree(item);

			ringbuf_infos->item[i] = NULL;
		}
	}

	return 0;
}

typedef int (*csv_ringbuf_input_fn)(struct kvm *kvm, int prio,
				    uintptr_t data_ptr,
				    struct csv_ringbuf_infos *ringbuf_infos);
typedef int (*csv_ringbuf_output_fn)(struct kvm *kvm,
				     struct csv_ringbuf_infos *ringbuf_infos);

static int get_cmd_helpers(__u32 cmd,
			   csv_ringbuf_input_fn *to_ringbuf_fn,
			   csv_ringbuf_output_fn *to_user_fn)
{
	int ret = 0;

	/* copy commands to ring buffer*/
	switch (cmd) {
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int csv_command_batch(struct kvm *kvm, struct kvm_sev_cmd *argp)
{
	int ret;
	struct kvm_csv_command_batch params;
	uintptr_t node_addr;
	struct csv_ringbuf_infos *ringbuf_infos;
	csv_ringbuf_input_fn csv_cmd_to_ringbuf_fn = NULL;
	csv_ringbuf_output_fn csv_copy_to_user_fn = NULL;
	int prio = CSV_COMMAND_PRIORITY_HIGH;

	if (!sev_guest(kvm))
		return -ENOTTY;

	if (copy_from_user(&params, (void __user *)(uintptr_t)argp->data,
			sizeof(struct kvm_csv_command_batch)))
		return -EFAULT;

	/* return directly if node list is NULL */
	if (!params.csv_batch_list_uaddr)
		return 0;

	/* ring buffer init */
	if (csv_ring_buffer_queue_init())
		return -EINVAL;

	if (get_cmd_helpers(params.command_id,
			    &csv_cmd_to_ringbuf_fn, &csv_copy_to_user_fn)) {
		ret = -EINVAL;
		goto err_free_ring_buffer;
	}

	ringbuf_infos = kzalloc(sizeof(*ringbuf_infos), GFP_KERNEL);
	if (!ringbuf_infos) {
		ret = -ENOMEM;
		goto err_free_ring_buffer;
	}

	node_addr = (uintptr_t)params.csv_batch_list_uaddr;
	while (node_addr) {
		struct kvm_csv_batch_list_node node;

		if (copy_from_user(&node, (void __user *)node_addr,
				sizeof(struct kvm_csv_batch_list_node))) {
			ret = -EFAULT;
			goto err_free_ring_buffer_infos_items;
		}

		if (ringbuf_infos->num > SVM_RING_BUFFER_MAX) {
			pr_err("%s: ring num is too large:%d, cmd:0x%x\n",
				__func__, ringbuf_infos->num, params.command_id);

			ret = -EINVAL;
			goto err_free_ring_buffer_infos_items;
		}

		if (csv_cmd_to_ringbuf_fn(kvm, prio,
					  (uintptr_t)node.cmd_data_addr,
					  ringbuf_infos)) {
			ret = -EFAULT;
			goto err_free_ring_buffer_infos_items;
		}

		/* 1st half set to HIGH queue, 2nd half set to LOW queue */
		if (ringbuf_infos->num == SVM_RING_BUFFER_MAX / 2)
			prio = CSV_COMMAND_PRIORITY_LOW;

		node_addr = node.next_cmd_addr;
	}

	/* ring buffer process */
	ret = csv_issue_ringbuf_cmds(kvm, &argp->error);
	if (ret)
		goto err_free_ring_buffer_infos_items;

	ret = csv_check_stat_queue_status(&argp->error);
	if (ret)
		goto err_free_ring_buffer_infos_items;

	if (csv_copy_to_user_fn && csv_copy_to_user_fn(kvm, ringbuf_infos)) {
		ret = -EFAULT;
		goto err_free_ring_buffer_infos_items;
	}

err_free_ring_buffer_infos_items:
	csv_ringbuf_infos_free(kvm, ringbuf_infos);
	kfree(ringbuf_infos);

err_free_ring_buffer:
	csv_ring_buffer_queue_free();

	return ret;
}

static int csv_mem_enc_ioctl(struct kvm *kvm, void __user *argp)
{
	struct kvm_sev_cmd sev_cmd;
	int r;

	if (!hygon_kvm_hooks.sev_hooks_installed ||
	    !(*hygon_kvm_hooks.sev_enabled))
		return -ENOTTY;

	if (!argp)
		return 0;

	if (copy_from_user(&sev_cmd, argp, sizeof(struct kvm_sev_cmd)))
		return -EFAULT;

	mutex_lock(&kvm->lock);

	switch (sev_cmd.id) {
	case KVM_CSV_COMMAND_BATCH:
		mutex_lock(&csv_cmd_batch_mutex);
		r = csv_command_batch(kvm, &sev_cmd);
		mutex_unlock(&csv_cmd_batch_mutex);
		break;
	default:
		/*
		 * If the command is compatible between CSV and SEV, the
		 * native implementation of the driver is invoked.
		 * Release the mutex before calling the native ioctl function
		 * because it will acquires the mutex.
		 */
		mutex_unlock(&kvm->lock);
		if (likely(csv_x86_ops.mem_enc_ioctl))
			return csv_x86_ops.mem_enc_ioctl(kvm, argp);
	}

	if (copy_to_user(argp, &sev_cmd, sizeof(struct kvm_sev_cmd)))
		r = -EFAULT;

	mutex_unlock(&kvm->lock);
	return r;
}

void csv_exit(void)
{
}

void __init csv_init(struct kvm_x86_ops *ops)
{
	/*
	 * Hygon CSV is indicated by X86_FEATURE_SEV, return directly if CSV
	 * is unsupported.
	 */
	if (!boot_cpu_has(X86_FEATURE_SEV))
		return;

	memcpy(&csv_x86_ops, ops, sizeof(struct kvm_x86_ops));

	ops->mem_enc_ioctl = csv_mem_enc_ioctl;
	ops->vm_attestation = csv_vm_attestation;
}

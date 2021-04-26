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

	ops->vm_attestation = csv_vm_attestation;
}

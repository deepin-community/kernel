// SPDX-License-Identifier: GPL-2.0-only
/*
 * CSV driver for KVM
 *
 * HYGON CSV support
 *
 * Copyright (C) Hygon Info Technologies Ltd.
 */

#include <linux/kvm_host.h>
#include <linux/psp.h>
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

/*--1024--1023--1024--1023--*/
#define TRANS_MEMPOOL_1ST_BLOCK_OFFSET		0
#define TRANS_MEMPOOL_2ND_BLOCK_OFFSET		(1024 << PAGE_SHIFT)
#define TRANS_MEMPOOL_3RD_BLOCK_OFFSET		(2047 << PAGE_SHIFT)
#define TRANS_MEMPOOL_4TH_BLOCK_OFFSET		(3071 << PAGE_SHIFT)
#define TRANS_MEMPOOL_BLOCKS_MAX_OFFSET		(4094 << PAGE_SHIFT)
#define TRANS_MEMPOOL_BLOCK_NUM			4
#define TRANS_MEMPOOL_BLOCK_SIZE		(1024 * PAGE_SIZE)

static size_t g_mempool_offset;
void *g_trans_mempool[TRANS_MEMPOOL_BLOCK_NUM] = { 0, };

static void csv_reset_mempool_offset(void)
{
	g_mempool_offset = 0;
}

int csv_alloc_trans_mempool(void)
{
	int i;

	for (i = 0; i < TRANS_MEMPOOL_BLOCK_NUM; i++) {
		WARN_ONCE(g_trans_mempool[i],
			  "g_trans_mempool[%d] was tainted\n", i);

		g_trans_mempool[i] = kzalloc(TRANS_MEMPOOL_BLOCK_SIZE, GFP_KERNEL);
		if (!g_trans_mempool[i])
			goto free_trans_mempool;
	}

	csv_reset_mempool_offset();
	return 0;

free_trans_mempool:
	csv_free_trans_mempool();
	pr_warn("Fail to allocate mem pool, CSV(2) live migration will very slow\n");

	return -ENOMEM;
}

void csv_free_trans_mempool(void)
{
	int i;

	for (i = 0; i < TRANS_MEMPOOL_BLOCK_NUM; i++) {
		kfree(g_trans_mempool[i]);
		g_trans_mempool[i] = NULL;
	}

	csv_reset_mempool_offset();
}

static void __maybe_unused *get_trans_data_from_mempool(size_t size)
{
	void *trans = NULL;
	char *trans_data = NULL;
	int i;
	size_t offset;

	if (g_mempool_offset < TRANS_MEMPOOL_2ND_BLOCK_OFFSET) {
		i = 0;
		offset = g_mempool_offset - TRANS_MEMPOOL_1ST_BLOCK_OFFSET;
	} else if (g_mempool_offset < TRANS_MEMPOOL_3RD_BLOCK_OFFSET) {
		i = 1;
		offset = g_mempool_offset - TRANS_MEMPOOL_2ND_BLOCK_OFFSET;
	} else if (g_mempool_offset < TRANS_MEMPOOL_4TH_BLOCK_OFFSET) {
		i = 2;
		offset = g_mempool_offset - TRANS_MEMPOOL_3RD_BLOCK_OFFSET;
	} else if (g_mempool_offset < TRANS_MEMPOOL_BLOCKS_MAX_OFFSET) {
		i = 3;
		offset = g_mempool_offset - TRANS_MEMPOOL_4TH_BLOCK_OFFSET;
	} else {
		pr_err("mempool is full (offset: %lu)\n", g_mempool_offset);
		return NULL;
	}

	trans_data = (char *)g_trans_mempool[i];
	if (!trans_data)
		return NULL;

	trans = &trans_data[offset];
	g_mempool_offset += size;

	return trans;
}

static int
csv_send_update_data_to_ringbuf(struct kvm *kvm,
				int prio,
				uintptr_t data_ptr,
				struct csv_ringbuf_infos *ringbuf_infos)
{
	struct kvm_sev_info *sev = &to_kvm_svm(kvm)->sev_info;
	struct sev_data_send_update_data *data;
	struct kvm_sev_send_update_data params;
	struct csv_ringbuf_info_item *item;
	void *hdr, *trans_data;
	struct page **guest_page;
	unsigned long n;
	int ret, offset;

	if (!sev_guest(kvm))
		return -ENOTTY;

	if (copy_from_user(&params, (void __user *)data_ptr,
			sizeof(struct kvm_sev_send_update_data)))
		return -EFAULT;

	/*
	 * userspace shouldn't query either header or trans length in ringbuf
	 * mode.
	 */
	if (!params.trans_len || !params.hdr_len)
		return -EINVAL;

	if (!params.trans_uaddr || !params.guest_uaddr ||
	    !params.guest_len || !params.hdr_uaddr)
		return -EINVAL;

	/* Check if we are crossing the page boundary */
	offset = params.guest_uaddr & (PAGE_SIZE - 1);
	if (params.guest_len > PAGE_SIZE || (params.guest_len + offset) > PAGE_SIZE)
		return -EINVAL;

	/* Pin guest memory */
	guest_page = hygon_kvm_hooks.sev_pin_memory(kvm, params.guest_uaddr & PAGE_MASK,
						    PAGE_SIZE, &n, 0);
	if (IS_ERR(guest_page))
		return PTR_ERR(guest_page);

	/* Allocate memory for header and transport buffer */
	ret = -ENOMEM;
	hdr = kzalloc(params.hdr_len, GFP_KERNEL);
	if (!hdr)
		goto e_unpin;

	trans_data = get_trans_data_from_mempool(params.trans_len);
	if (!trans_data)
		goto e_free_hdr;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		goto e_free_hdr;

	data->hdr_address = __psp_pa(hdr);
	data->hdr_len = params.hdr_len;
	data->trans_address = __psp_pa(trans_data);
	data->trans_len = params.trans_len;

	/* The SEND_UPDATE_DATA command requires C-bit to be always set. */
	data->guest_address = (page_to_pfn(guest_page[0]) << PAGE_SHIFT) +
				offset;
	data->guest_address |= *hygon_kvm_hooks.sev_me_mask;
	data->guest_len = params.guest_len;
	data->handle = sev->handle;

	ret = csv_fill_cmd_queue(prio, SEV_CMD_SEND_UPDATE_DATA, data, 0);
	if (ret)
		goto e_free;

	/*
	 * Create item to save page info and pointer, which will be freed
	 * in function csv_command_batch because it will be used after PSP
	 * return for copy_to_user.
	 */
	item = kzalloc(sizeof(*item), GFP_KERNEL);
	if (!item) {
		ret = -ENOMEM;
		goto e_free;
	}

	item->pages = guest_page;
	item->n = n;
	item->hdr_vaddr = (uintptr_t)hdr;
	item->hdr_uaddr = params.hdr_uaddr;
	item->hdr_len = params.hdr_len;
	item->trans_vaddr = (uintptr_t)trans_data;
	item->trans_uaddr = params.trans_uaddr;
	item->trans_len = params.trans_len;
	item->data_vaddr = (uintptr_t)data;

	ringbuf_infos->item[ringbuf_infos->num++] = item;

	/* copy to ring buffer success, data freed after commands completed */
	return 0;

e_free:
	kfree(data);
e_free_hdr:
	kfree(hdr);
e_unpin:
	hygon_kvm_hooks.sev_unpin_memory(kvm, guest_page, n);
	return ret;
}

static int
csv_send_update_data_copy_to_user(struct kvm *kvm,
				  struct csv_ringbuf_infos *ringbuf_infos)
{
	int i, ret = 0;

	for (i = 0; i < ringbuf_infos->num; i++) {
		struct csv_ringbuf_info_item *item = ringbuf_infos->item[i];

		/* copy transport buffer to user space */
		if (copy_to_user((void __user *)item->trans_uaddr,
				 (void *)item->trans_vaddr, item->trans_len)) {
			ret = -EFAULT;
			break;
		}

		/* Copy packet header to userspace. */
		if (copy_to_user((void __user *)item->hdr_uaddr,
				 (void *)item->hdr_vaddr, item->hdr_len)) {
			ret = -EFAULT;
			break;
		}
	}

	return ret;
}

static int
csv_receive_update_data_to_ringbuf(struct kvm *kvm,
				   int prio,
				   uintptr_t data_ptr,
				   struct csv_ringbuf_infos *ringbuf_infos)
{
	struct kvm_sev_info *sev = &to_kvm_svm(kvm)->sev_info;
	struct kvm_sev_receive_update_data params;
	struct sev_data_receive_update_data *data;
	struct csv_ringbuf_info_item *item;
	void *hdr = NULL, *trans = NULL;
	struct page **guest_page;
	unsigned long n;
	int ret, offset;

	if (!sev_guest(kvm))
		return -EINVAL;

	if (copy_from_user(&params, (void __user *)data_ptr,
			sizeof(struct kvm_sev_receive_update_data)))
		return -EFAULT;

	if (!params.hdr_uaddr || !params.hdr_len ||
	    !params.guest_uaddr || !params.guest_len ||
	    !params.trans_uaddr || !params.trans_len)
		return -EINVAL;

	/* Check if we are crossing the page boundary */
	offset = params.guest_uaddr & (PAGE_SIZE - 1);
	if (params.guest_len > PAGE_SIZE || (params.guest_len + offset) > PAGE_SIZE)
		return -EINVAL;

	hdr = psp_copy_user_blob(params.hdr_uaddr, params.hdr_len);
	if (IS_ERR(hdr))
		return PTR_ERR(hdr);

	ret = -ENOMEM;
	trans = get_trans_data_from_mempool(params.trans_len);
	if (!trans)
		goto e_free_hdr;

	if (copy_from_user(trans, (void __user *)params.trans_uaddr,
			params.trans_len)) {
		ret = -EFAULT;
		goto e_free_hdr;
	}

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		goto e_free_hdr;

	data->hdr_address = __psp_pa(hdr);
	data->hdr_len = params.hdr_len;
	data->trans_address = __psp_pa(trans);
	data->trans_len = params.trans_len;

	/* Pin guest memory */
	guest_page = hygon_kvm_hooks.sev_pin_memory(kvm, params.guest_uaddr & PAGE_MASK,
						    PAGE_SIZE, &n, 1);
	if (IS_ERR(guest_page)) {
		ret = PTR_ERR(guest_page);
		goto e_free;
	}

	/*
	 * Flush (on non-coherent CPUs) before RECEIVE_UPDATE_DATA, the PSP
	 * encrypts the written data with the guest's key, and the cache may
	 * contain dirty, unencrypted data.
	 */
	hygon_kvm_hooks.sev_clflush_pages(guest_page, n);

	/* The RECEIVE_UPDATE_DATA command requires C-bit to be always set. */
	data->guest_address = (page_to_pfn(guest_page[0]) << PAGE_SHIFT) +
				offset;
	data->guest_address |= *hygon_kvm_hooks.sev_me_mask;
	data->guest_len = params.guest_len;
	data->handle = sev->handle;

	ret = csv_fill_cmd_queue(prio, SEV_CMD_RECEIVE_UPDATE_DATA, data, 0);

	if (ret)
		goto e_unpin;

	/*
	 * Create item to save page info and pointer, whitch will be freed
	 * in function csv_command_batch because it will be used after PSP
	 * return for copy_to_user.
	 */
	item = kzalloc(sizeof(*item), GFP_KERNEL);
	if (!item) {
		ret = -ENOMEM;
		goto e_unpin;
	}

	item->pages = guest_page;
	item->n = n;
	item->hdr_vaddr = (uintptr_t)hdr;
	item->trans_vaddr = (uintptr_t)trans;
	item->data_vaddr = (uintptr_t)data;

	ringbuf_infos->item[ringbuf_infos->num++] = item;

	/* copy to ring buffer success, data freed after commands completed */
	return 0;

e_unpin:
	hygon_kvm_hooks.sev_unpin_memory(kvm, guest_page, n);
e_free:
	kfree(data);
e_free_hdr:
	kfree(hdr);

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
	case KVM_SEV_SEND_UPDATE_DATA:
		*to_ringbuf_fn = csv_send_update_data_to_ringbuf;
		*to_user_fn = csv_send_update_data_copy_to_user;
		break;
	case KVM_SEV_RECEIVE_UPDATE_DATA:
		*to_ringbuf_fn = csv_receive_update_data_to_ringbuf;
		*to_user_fn = NULL;
		break;
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
	csv_reset_mempool_offset();

err_free_ring_buffer:
	csv_ring_buffer_queue_free();

	return ret;
}

/* Userspace wants to query either header or trans length. */
static int
__csv_send_update_vmsa_query_lengths(struct kvm *kvm, struct kvm_sev_cmd *argp,
				     struct kvm_csv_send_update_vmsa *params)
{
	struct kvm_sev_info *sev = &to_kvm_svm(kvm)->sev_info;
	struct sev_data_send_update_vmsa *vmsa;
	int ret;

	vmsa = kzalloc(sizeof(*vmsa), GFP_KERNEL_ACCOUNT);
	if (!vmsa)
		return -ENOMEM;

	vmsa->handle = sev->handle;
	ret = hygon_kvm_hooks.sev_issue_cmd(kvm, SEV_CMD_SEND_UPDATE_VMSA,
					    vmsa, &argp->error);

	params->hdr_len = vmsa->hdr_len;
	params->trans_len = vmsa->trans_len;

	if (copy_to_user((void __user *)argp->data, params,
			 sizeof(struct kvm_csv_send_update_vmsa)))
		ret = -EFAULT;

	kfree(vmsa);
	return ret;
}

static int csv_send_update_vmsa(struct kvm *kvm, struct kvm_sev_cmd *argp)
{
	struct kvm_sev_info *sev = &to_kvm_svm(kvm)->sev_info;
	struct sev_data_send_update_vmsa *vmsa;
	struct kvm_csv_send_update_vmsa params;
	struct kvm_vcpu *vcpu;
	void *hdr, *trans_data;
	int ret;

	if (!sev_es_guest(kvm))
		return -ENOTTY;

	if (copy_from_user(&params, (void __user *)(uintptr_t)argp->data,
			   sizeof(struct kvm_csv_send_update_vmsa)))
		return -EFAULT;

	/* userspace wants to query either header or trans length */
	if (!params.trans_len || !params.hdr_len)
		return __csv_send_update_vmsa_query_lengths(kvm, argp, &params);

	if (!params.trans_uaddr || !params.hdr_uaddr)
		return -EINVAL;

	/* Get the target vcpu */
	vcpu = kvm_get_vcpu_by_id(kvm, params.vcpu_id);
	if (!vcpu) {
		pr_err("%s: invalid vcpu\n", __func__);
		return -EINVAL;
	}

	pr_debug("%s: vcpu (%d)\n", __func__, vcpu->vcpu_id);

	/* allocate memory for header and transport buffer */
	ret = -ENOMEM;
	hdr = kzalloc(params.hdr_len, GFP_KERNEL_ACCOUNT);
	if (!hdr)
		return ret;

	trans_data = kzalloc(params.trans_len, GFP_KERNEL_ACCOUNT);
	if (!trans_data)
		goto e_free_hdr;

	vmsa = kzalloc(sizeof(*vmsa), GFP_KERNEL_ACCOUNT);
	if (!vmsa)
		goto e_free_trans_data;

	vmsa->hdr_address = __psp_pa(hdr);
	vmsa->hdr_len = params.hdr_len;
	vmsa->trans_address = __psp_pa(trans_data);
	vmsa->trans_len = params.trans_len;

	/* The SEND_UPDATE_VMSA command requires C-bit to be always set. */
	vmsa->guest_address = __pa(to_svm(vcpu)->sev_es.vmsa) |
			      *hygon_kvm_hooks.sev_me_mask;
	vmsa->guest_len = PAGE_SIZE;
	vmsa->handle = sev->handle;

	ret = hygon_kvm_hooks.sev_issue_cmd(kvm, SEV_CMD_SEND_UPDATE_VMSA,
					    vmsa, &argp->error);

	if (ret)
		goto e_free;

	/* copy transport buffer to user space */
	if (copy_to_user((void __user *)(uintptr_t)params.trans_uaddr,
			 trans_data, params.trans_len)) {
		ret = -EFAULT;
		goto e_free;
	}

	/* Copy packet header to userspace. */
	ret = copy_to_user((void __user *)(uintptr_t)params.hdr_uaddr, hdr,
			   params.hdr_len);

e_free:
	kfree(vmsa);
e_free_trans_data:
	kfree(trans_data);
e_free_hdr:
	kfree(hdr);

	return ret;
}

static int csv_receive_update_vmsa(struct kvm *kvm, struct kvm_sev_cmd *argp)
{
	struct kvm_sev_info *sev = &to_kvm_svm(kvm)->sev_info;
	struct kvm_csv_receive_update_vmsa params;
	struct sev_data_receive_update_vmsa *vmsa;
	struct kvm_vcpu *vcpu;
	void *hdr = NULL, *trans = NULL;
	int ret;

	if (!sev_es_guest(kvm))
		return -ENOTTY;

	if (copy_from_user(&params, (void __user *)(uintptr_t)argp->data,
			   sizeof(struct kvm_csv_receive_update_vmsa)))
		return -EFAULT;

	if (!params.hdr_uaddr || !params.hdr_len ||
	    !params.trans_uaddr || !params.trans_len)
		return -EINVAL;

	/* Get the target vcpu */
	vcpu = kvm_get_vcpu_by_id(kvm, params.vcpu_id);
	if (!vcpu) {
		pr_err("%s: invalid vcpu\n", __func__);
		return -EINVAL;
	}

	pr_debug("%s: vcpu (%d)\n", __func__, vcpu->vcpu_id);

	hdr = psp_copy_user_blob(params.hdr_uaddr, params.hdr_len);
	if (IS_ERR(hdr))
		return PTR_ERR(hdr);

	trans = psp_copy_user_blob(params.trans_uaddr, params.trans_len);
	if (IS_ERR(trans)) {
		ret = PTR_ERR(trans);
		goto e_free_hdr;
	}

	ret = -ENOMEM;
	vmsa = kzalloc(sizeof(*vmsa), GFP_KERNEL_ACCOUNT);
	if (!vmsa)
		goto e_free_trans;

	vmsa->hdr_address = __psp_pa(hdr);
	vmsa->hdr_len = params.hdr_len;
	vmsa->trans_address = __psp_pa(trans);
	vmsa->trans_len = params.trans_len;

	/*
	 * Flush before RECEIVE_UPDATE_VMSA, the PSP encrypts the
	 * written VMSA memory content with the guest's key), and
	 * the cache may contain dirty, unencrypted data.
	 */
	clflush_cache_range(to_svm(vcpu)->sev_es.vmsa, PAGE_SIZE);

	/* The RECEIVE_UPDATE_VMSA command requires C-bit to be always set. */
	vmsa->guest_address = __pa(to_svm(vcpu)->sev_es.vmsa) |
			      *hygon_kvm_hooks.sev_me_mask;
	vmsa->guest_len = PAGE_SIZE;
	vmsa->handle = sev->handle;

	ret = hygon_kvm_hooks.sev_issue_cmd(kvm, SEV_CMD_RECEIVE_UPDATE_VMSA,
					    vmsa, &argp->error);

	if (!ret)
		vcpu->arch.guest_state_protected = true;

	kfree(vmsa);
e_free_trans:
	kfree(trans);
e_free_hdr:
	kfree(hdr);

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
	case KVM_SEV_SEND_UPDATE_VMSA:
		/*
		 * Hygon implement the specific interface, although
		 * KVM_SEV_SEND_UPDATE_VMSA is the command shared by CSV and
		 * SEV. The struct sev_data_send_update_vmsa is also shared
		 * by CSV and SEV, we'll use this structure in the code.
		 */
		r = csv_send_update_vmsa(kvm, &sev_cmd);
		break;
	case KVM_SEV_RECEIVE_UPDATE_VMSA:
		/*
		 * Hygon implement the specific interface, although
		 * KVM_SEV_RECEIVE_UPDATE_VMSA is the command shared by CSV and
		 * SEV. The struct sev_data_receive_update_vmsa is also shared
		 * by CSV and SEV, we'll use this structure in the code.
		 */
		r = csv_receive_update_vmsa(kvm, &sev_cmd);
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

/* The caller must flush the stale caches about svm->sev_es.vmsa */
void csv2_sync_reset_vmsa(struct vcpu_svm *svm)
{
	if (svm->sev_es.reset_vmsa)
		memcpy(svm->sev_es.reset_vmsa, svm->sev_es.vmsa, PAGE_SIZE);
}

void csv2_free_reset_vmsa(struct vcpu_svm *svm)
{
	if (svm->sev_es.reset_vmsa) {
		__free_page(virt_to_page(svm->sev_es.reset_vmsa));
		svm->sev_es.reset_vmsa = NULL;
	}
}

int csv2_setup_reset_vmsa(struct vcpu_svm *svm)
{
	struct page *reset_vmsa_page = NULL;

	reset_vmsa_page = alloc_page(GFP_KERNEL_ACCOUNT | __GFP_ZERO);
	if (!reset_vmsa_page)
		return -ENOMEM;

	svm->sev_es.reset_vmsa = page_address(reset_vmsa_page);
	return 0;
}

static int csv2_map_ghcb_gpa(struct vcpu_svm *svm, u64 ghcb_gpa)
{
	if (kvm_vcpu_map(&svm->vcpu, ghcb_gpa >> PAGE_SHIFT, &svm->sev_es.ghcb_map)) {
		/* Unable to map GHCB from guest */
		vcpu_unimpl(&svm->vcpu, "Missing GHCB [%#llx] from guest\n",
			    ghcb_gpa);

		svm->sev_es.receiver_ghcb_map_fail = true;
		return -EINVAL;
	}

	svm->sev_es.ghcb = svm->sev_es.ghcb_map.hva;
	svm->sev_es.receiver_ghcb_map_fail = false;

	pr_info("Mapping GHCB [%#llx] from guest at recipient\n", ghcb_gpa);

	return 0;
}

static bool is_ghcb_msr_protocol(u64 ghcb_val)
{
	return !!(ghcb_val & GHCB_MSR_INFO_MASK);
}

/*
 * csv_get_msr return msr data to the userspace.
 *
 * Return 0 if get msr success.
 */
int csv_get_msr(struct kvm_vcpu *vcpu, struct msr_data *msr_info)
{
	struct vcpu_svm *svm = to_svm(vcpu);

	switch (msr_info->index) {
	case MSR_AMD64_SEV_ES_GHCB:
		/* Only support userspace get from vmcb.control.ghcb_gpa */
		if (!msr_info->host_initiated || !sev_es_guest(vcpu->kvm))
			return 1;

		msr_info->data = svm->vmcb->control.ghcb_gpa;

		/* Only set status bits when using GHCB page protocol */
		if (msr_info->data &&
		    !is_ghcb_msr_protocol(msr_info->data)) {
			if (svm->sev_es.ghcb)
				msr_info->data |= GHCB_MSR_MAPPED_MASK;

			if (svm->sev_es.received_first_sipi)
				msr_info->data |=
					GHCB_MSR_RECEIVED_FIRST_SIPI_MASK;
		}
		break;
	default:
		return 1;
	}
	return 0;
}

/*
 * csv_set_msr set msr data from the userspace.
 *
 * Return 0 if set msr success.
 */
int csv_set_msr(struct kvm_vcpu *vcpu, struct msr_data *msr_info)
{
	struct vcpu_svm *svm = to_svm(vcpu);
	u32 ecx = msr_info->index;
	u64 data = msr_info->data;

	switch (ecx) {
	case MSR_AMD64_SEV_ES_GHCB:
		/* Only support userspace set to vmcb.control.ghcb_gpa */
		if (!msr_info->host_initiated || !sev_es_guest(vcpu->kvm))
			return 1;

		/*
		 * Value 0 means uninitialized userspace MSR data, userspace
		 * need get the initial MSR data afterwards.
		 */
		if (!data)
			return 0;

		/* Extract status info when using GHCB page protocol */
		if (!is_ghcb_msr_protocol(data)) {
			if (!svm->sev_es.ghcb && (data & GHCB_MSR_MAPPED_MASK)) {
				/*
				 * This happened on the recipient of migration,
				 * should return error if cannot map the ghcb
				 * page.
				 */
				if (csv2_map_ghcb_gpa(to_svm(vcpu),
						data & ~GHCB_MSR_KVM_STATUS_MASK))
					return 1;
			}

			if (data & GHCB_MSR_RECEIVED_FIRST_SIPI_MASK)
				svm->sev_es.received_first_sipi = true;

			data &= ~GHCB_MSR_KVM_STATUS_MASK;
		}

		svm->vmcb->control.ghcb_gpa = data;
		break;
	default:
		return 1;
	}
	return 0;
}

bool csv_has_emulated_ghcb_msr(struct kvm *kvm)
{
	/* this should be determined after KVM_CREATE_VM. */
	if (kvm && !sev_es_guest(kvm))
		return false;

	return true;
}

static int csv_control_pre_system_reset(struct kvm *kvm)
{
	struct kvm_vcpu *vcpu;
	unsigned long i;
	int ret;

	if (!sev_es_guest(kvm))
		return 0;

	kvm_for_each_vcpu(i, vcpu, kvm) {
		ret = mutex_lock_killable(&vcpu->mutex);
		if (ret)
			return ret;

		vcpu->arch.guest_state_protected = false;

		mutex_unlock(&vcpu->mutex);
	}

	return 0;
}

static int csv_control_post_system_reset(struct kvm *kvm)
{
	struct kvm_vcpu *vcpu;
	unsigned long i;
	int ret;

	if (!sev_guest(kvm))
		return 0;

	/* Flush both host and guest caches before next boot flow */
	wbinvd_on_all_cpus();

	if (!sev_es_guest(kvm))
		return 0;

	kvm_for_each_vcpu(i, vcpu, kvm) {
		struct vcpu_svm *svm = to_svm(vcpu);

		ret = mutex_lock_killable(&vcpu->mutex);
		if (ret)
			return ret;

		memcpy(svm->sev_es.vmsa, svm->sev_es.reset_vmsa, PAGE_SIZE);

		/* Flush encrypted vmsa to memory */
		clflush_cache_range(svm->sev_es.vmsa, PAGE_SIZE);

		svm->vcpu.arch.guest_state_protected = true;
		svm->sev_es.received_first_sipi = false;

		mutex_unlock(&vcpu->mutex);
	}

	return 0;
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
	ops->control_pre_system_reset = csv_control_pre_system_reset;
	ops->control_post_system_reset = csv_control_post_system_reset;
}

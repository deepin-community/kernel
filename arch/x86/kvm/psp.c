// SPDX-License-Identifier: GPL-2.0-only
/*
 * PSP virtualization
 *
 * Copyright (c) 2023, HYGON CORPORATION. All rights reserved.
 *     Author: Ge Yang <yangge@hygon.cn>
 *
 */

#include <linux/kvm_types.h>
#include <linux/slab.h>
#include <linux/kvm_host.h>
#include <linux/psp-sev.h>

struct psp_cmdresp_head {
	uint32_t buf_size;
	uint32_t cmdresp_size;
	uint32_t cmdresp_code;
} __packed;

int guest_addr_map_table_op(void *data_hva, gpa_t data_gpa, gpa_t table_gpa,
		int op)
{
	return 0;
}

int kvm_pv_psp_op(struct kvm *kvm, int cmd, gpa_t data_gpa, gpa_t psp_ret_gpa,
		gpa_t table_gpa)
{
	void *data;
	struct psp_cmdresp_head psp_head;
	uint32_t data_size;
	int psp_ret = 0;
	int ret = 0;

	if (unlikely(kvm_read_guest(kvm, data_gpa, &psp_head,
					sizeof(struct psp_cmdresp_head))))
		return -EFAULT;

	data_size = psp_head.buf_size;
	data = kzalloc(data_size, GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	if (unlikely(kvm_read_guest(kvm, data_gpa, data, data_size))) {
		ret = -EFAULT;
		goto e_free;
	}

	if (guest_addr_map_table_op(data, data_gpa, table_gpa, 0)) {
		ret = -EFAULT;
		goto e_free;
	}

	ret = psp_do_cmd(cmd, data, &psp_ret);
	if (ret) {
		pr_err("%s: psp do cmd error, %d\n", __func__, psp_ret);
		ret = -EIO;
		goto e_free;
	}

	if (guest_addr_map_table_op(data, data_gpa, table_gpa, 1)) {
		ret = -EFAULT;
		goto e_free;
	}

	if (unlikely(kvm_write_guest(kvm, data_gpa, data, data_size))) {
		ret = -EFAULT;
		goto e_free;
	}

	if (unlikely(kvm_write_guest(kvm, psp_ret_gpa, &psp_ret,
				sizeof(psp_ret)))) {
		ret = -EFAULT;
		goto e_free;
	}

	return ret;

e_free:
	kfree(data);
	return ret;
}


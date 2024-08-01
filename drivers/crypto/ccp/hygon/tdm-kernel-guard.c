// SPDX-License-Identifier: GPL-2.0-only
/*
 * The Hygon TDM KERNEL GUARD module driver
 *
 * Copyright (C) 2022 Hygon Info Technologies Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <crypto/hash.h>
#include <linux/kallsyms.h>
#include <linux/kprobes.h>
#include <asm/asm-offsets.h>
#include "tdm-dev.h"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

static int eh_obj = -1;
module_param(eh_obj, int, 0644);
MODULE_PARM_DESC(eh_obj, "security enhance object for TDM");

/* Objects are protected by TDM now
 *   SCT: 0
 *   IDT: 1
 */
enum ENHANCE_OBJS {
	SCT = 0,
	IDT,
	MAX_OBJ
};

static char *obj_names[MAX_OBJ] = {
	"SCT",
	"IDT",
};

struct tdm_security_enhance {
	uint64_t vaddr;
	uint32_t size;
	struct addr_range_info *mem_range;
	struct authcode_2b *authcode;
	struct measure_data mdata;
	uint32_t context;
	uint32_t task_id;
	char *obj_name;
} __packed;

static struct tdm_security_enhance eh_objs[MAX_OBJ];

static int tdm_regi_callback_handler(uint32_t task_id)
{
	int i = 0;
	int ret = 0;

	for (i = 0; i < MAX_OBJ; i++) {
		if (task_id == eh_objs[i].task_id) {
			pr_warn("Obj: %s, Task:%d, corruption detected!\n", eh_objs[i].obj_name,
				task_id);
			pr_warn("Please check if it's intended, or your machine may be on danger!\n");
			break;
		}
	}
	return ret;
}

static int calc_expected_hash(uint8_t *base_addr, uint32_t size, uint8_t *hash)
{
	int ret = 0;
	struct crypto_shash *shash = NULL;

	shash = crypto_alloc_shash("sm3", 0, 0);
	if (IS_ERR(shash)) {
		ret = PTR_ERR(shash);
		return ret;
	}

	{
		SHASH_DESC_ON_STACK(sdesc, shash);

		sdesc->tfm = shash;
		ret = crypto_shash_init(sdesc);
		if (ret) {
			pr_err("crypto_shash_init failed\n");
			ret = -1;
			goto out;
		}

		ret = crypto_shash_update(sdesc, base_addr, size);
		if (ret) {
			pr_err("crypto_shash_update failed\n");
			ret = -1;
			goto out;
		}

		ret = crypto_shash_final(sdesc, hash);
		if (ret) {
			pr_err("crypto_shash_final failed\n");
			ret = -1;
			goto out;
		}
	}

out:
	crypto_free_shash(shash);
	return ret;
}

static int tdm_task_create_and_run(struct tdm_security_enhance *data)
{
	int ret = 0;
	int task_status = 0;

	data->task_id = psp_create_measure_task(data->mem_range, &data->mdata, data->context,
		data->authcode);
	if (data->task_id < 0) {
		ret = data->task_id < 0;
		pr_err("create measurement task failed with 0x%x!\n", data->task_id);
		goto end;
	}

	ret = psp_register_measure_exception_handler(data->task_id, data->authcode,
		tdm_regi_callback_handler);
	if (ret < 0) {
		pr_err("task_id %d callback function register failed with 0x%x\n", data->task_id,
			ret);
		goto release_task;
	}

	task_status = psp_startstop_measure_task(data->task_id, data->authcode, true);
	if (task_status < 0) {
		ret = task_status;
		pr_err("task_id %d start failed with 0x%x\n", data->task_id, ret);
		goto release_task;
	}

	return ret;

release_task:
	psp_destroy_measure_task(data->task_id, data->authcode);
end:
	return ret;
}

int tdm_service_run(struct tdm_security_enhance *data)
{
	int ret = 0;
	struct addr_range_info *addr_range = NULL;

	// Allocate memory for addr_range
	addr_range = kzalloc(sizeof(struct addr_range_info) + sizeof(struct addr_info), GFP_KERNEL);
	if (!addr_range) {
		ret = -DYN_ERR_MEM;
		pr_err("addr_range kzalloc memory failed\n");
		goto end;
	}

	// Fill in addr_range
	addr_range->count = 1;
	addr_range->addr[0].addr_start = data->vaddr;
	addr_range->addr[0].length = data->size;
	data->mem_range = addr_range;

	// Context configuration
	data->context |= TASK_CREATE_VADDR;

	// Allocate memory for authcode
	data->authcode = kzalloc(sizeof(struct authcode_2b) + AUTHCODE_MAX, GFP_KERNEL);
	if (!data->authcode) {
		ret = -DYN_ERR_MEM;
		pr_err("authcode_2b kzalloc memory failed\n");
		goto free_addr_range_info;
	}

	data->authcode->len = AUTHCODE_MAX;

	// Measurement data configuration
	data->mdata.hash_algo = HASH_ALGO_SM3;
	data->mdata.period_ms = 0;
	ret = calc_expected_hash((uint8_t *)data->vaddr, data->size,
		data->mdata.expected_measurement);
	if (ret) {
		pr_err("calculate expected hash failed!\n");
		goto free_authcode;
	}

	// Create and start tdm task
	ret = tdm_task_create_and_run(data);
	if (ret) {
		pr_err("tdm_task_create_and_run failed!\n");
		goto free_authcode;
	}

	return ret;

free_authcode:
	kfree(data->authcode);
	data->authcode = NULL;
free_addr_range_info:
	kfree(data->mem_range);
	data->mem_range = NULL;
end:
	return ret;
}

int tdm_service_exit(struct tdm_security_enhance *data)
{
	int ret = 0;
	int task_status = 0;

	task_status = psp_startstop_measure_task(data->task_id, data->authcode, false);
	if (task_status < 0) {
		ret = task_status;
		pr_err("task_id %d stop failed with 0x%x\n", data->task_id, ret);
		goto end;
	}

	// Waiting for the task to end
	msleep(40);

	psp_destroy_measure_task(data->task_id, data->authcode);

	kfree(data->authcode);
	data->authcode = NULL;
	kfree(data->mem_range);
	data->mem_range = NULL;
end:
	return ret;
}

#if !IS_BUILTIN(CONFIG_TDM_KERNEL_GUARD)
static int p_tmp_kprobe_handler(struct kprobe *p_ri, struct pt_regs *p_regs)
{
	return 0;
}

unsigned long kprobe_symbol_address_byname(const char *name)
{
	int p_ret;
	struct kprobe p_kprobe;
	unsigned long addr = 0;

	memset(&p_kprobe, 0, sizeof(p_kprobe));

	p_kprobe.pre_handler = p_tmp_kprobe_handler;
	p_kprobe.symbol_name = name;

	p_ret = register_kprobe(&p_kprobe);
	if (p_ret < 0) {
		pr_err("register_kprobe error [%d] :(\n", p_ret);
		return 0;
	}

	addr = (unsigned long)p_kprobe.addr;
	unregister_kprobe(&p_kprobe);

	return addr;
}
#endif

static int __init kernel_security_enhance_init(void)
{
	int i = 0;
	int ret = 0;
	unsigned long *sct_addr;
	struct desc_ptr idtr;
#if !IS_BUILTIN(CONFIG_TDM_KERNEL_GUARD)
	unsigned long (*f_kallsyms_lookup_name)(const char *);

	f_kallsyms_lookup_name = (unsigned long (*)(const char *))kprobe_symbol_address_byname(
		"kallsyms_lookup_name");
	if (!f_kallsyms_lookup_name) {
		ret = -DYN_ERR_API;
		pr_err("kprobe_symbol_address_byname failed!");
		goto end;
	}

	sct_addr = (unsigned long *)f_kallsyms_lookup_name("sys_call_table");
#else

	sct_addr = (unsigned long *)kallsyms_lookup_name("sys_call_table");
#endif
	if (!sct_addr) {
		ret = -DYN_ERR_API;
		pr_err("kallsyms_lookup_name for sys_call_table failed!");
		goto end;
	}

	asm("sidt %0":"=m"(idtr));

	if (!psp_check_tdm_support())
		return 0;

	for (i = 0; i < MAX_OBJ; i++) {
		memset(&eh_objs[i], 0, sizeof(eh_objs[i]));
		eh_objs[i].context = CONTEXT_CHECK_MODNAME;
		eh_objs[i].obj_name = obj_names[i];
	}

	if ((eh_obj == -1) || (eh_obj & (1 << SCT))) {
		eh_objs[SCT].vaddr = (uint64_t)sct_addr;
		eh_objs[SCT].size = NR_syscalls * sizeof(char *);
	}
	if ((eh_obj == -1) || (eh_obj & (1 << IDT))) {
		eh_objs[IDT].vaddr = idtr.address;
		eh_objs[IDT].size = idtr.size;
	}

	for (i = 0; i < MAX_OBJ; i++) {
		if (eh_objs[i].vaddr)
			tdm_service_run(&eh_objs[i]);
	}

	pr_info("Hygon TDM guard load successfully!\n");

end:
	return ret;
}

static void __exit kernel_security_enhance_exit(void)
{
	int i = 0;

	if (!psp_check_tdm_support())
		return;

	for (i = 0; i < MAX_OBJ; i++) {
		if (eh_objs[i].vaddr)
			tdm_service_exit(&eh_objs[i]);
	}
	pr_info("Hygon TDM guard unload successfully!\n");
}

MODULE_AUTHOR("niuyongwen@hygon.cn");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
MODULE_DESCRIPTION("Kernel security enhancement module by TDM");

/*
 * kernel_security_enhance_init must be done after ccp module init.
 * That's why we use a device_initcall_sync which is
 * called after all the device_initcall(includes ccp) but before the
 * late_initcall(includes ima).
 */
device_initcall_sync(kernel_security_enhance_init);
module_exit(kernel_security_enhance_exit);

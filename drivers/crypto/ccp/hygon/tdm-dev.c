// SPDX-License-Identifier: GPL-2.0-only
/*
 * The Hygon TDM CPU-to-PSP communication driver
 *
 * Copyright (C) 2022 Hygon Info Technologies Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/psp-hygon.h>
#include <linux/spinlock.h>
#include <linux/rwlock.h>
#include <linux/err.h>
#include <asm/current.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/kfifo.h>
#include <linux/scatterlist.h>
#include <crypto/hash.h>
#include <linux/kallsyms.h>
#include <linux/ftrace.h>
#include "tdm-dev.h"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "tdm: " fmt

#define TDM_CMD_ID_MAX		16
#define TDM2PSP_CMD(id)		(0x110 | (id))
#define TDM_P2C_CMD_ID		1
#define TDM_C2P_CMD_SIZE	(3*PAGE_SIZE)
#define TDM_KFIFO_SIZE		1024

#define TDM_IOC_TYPE		'D'
#define TDM_CMD_LEN_LIMIT	(1U << 12)

struct context_message {
	uint32_t flag;
	uint32_t pid;
	uint8_t comm[16];
	uint8_t module_name[64];
};

struct tdm_task_head {
	struct list_head head;
	rwlock_t lock;
};

struct tdm_task_ctx {
	uint32_t task_id;
	uint32_t cmd_ctx_flag;
	measure_exception_handler_t handler;
	struct list_head list;
};

static struct tdm_task_head dyn_head;
static unsigned int p2c_cmd_id = TDM_P2C_CMD_ID;
static struct task_struct *kthread;
static DECLARE_KFIFO(kfifo_error_task, unsigned char, TDM_KFIFO_SIZE);
static spinlock_t kfifo_lock;
static int tdm_support;
static int tdm_init_flag;
static int tdm_destroy_flag;

static int list_check_exist(uint32_t task_id)
{
	int found = 0;
	struct list_head *head = NULL;
	rwlock_t *lock = NULL;
	struct tdm_task_ctx *task_node = NULL, *tmp_node = NULL;

	head = &dyn_head.head;
	lock = &dyn_head.lock;

	read_lock(lock);
	list_for_each_entry_safe(task_node, tmp_node, head, list) {
		if (task_node->task_id == task_id) {
			found = 1;
			break;
		}
	}
	read_unlock(lock);

	return found;
}

static int list_enqueue(void *entry)
{
	int ret = 0;
	struct list_head *head, *entry_list = NULL;
	rwlock_t *lock = NULL;

	if (!entry) {
		ret = -DYN_NULL_POINTER;
		pr_err("Null pointer\n");
		goto end;
	}

	head = &dyn_head.head;
	lock = &dyn_head.lock;
	entry_list = &(((struct tdm_task_ctx *)entry)->list);

	write_lock(lock);
	if (entry_list)
		list_add_tail(entry_list, head);
	write_unlock(lock);

end:
	return 0;
}

static __maybe_unused int list_print(void)
{
	struct list_head *head = NULL;
	rwlock_t *lock = NULL;
	struct tdm_task_ctx *task_node = NULL, *tmp_node = NULL;

	head = &dyn_head.head;
	lock = &dyn_head.lock;

	read_lock(lock);
	list_for_each_entry_safe(task_node, tmp_node, head, list) {
		pr_info("id: %d ", task_node->task_id);
	}
	read_unlock(lock);
	pr_info("\n");

	return 0;
}

static int measure_exception_handling_thread(void *data)
{
	int ret = 0;
	int copied = 0;
	uint32_t error_task_id = 0xffffffff;
	struct measure_status task_measure_status;
	struct list_head *head = NULL;
	rwlock_t *lock = NULL;
	struct tdm_task_ctx *task_node = NULL, *tmp_node = NULL;

	head = &dyn_head.head;
	lock = &dyn_head.lock;

	pr_info("Thread started for measurement exception handler dispatching...\n");
	while (!kthread_should_stop()) {
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();

		while (!kfifo_is_empty(&kfifo_error_task)) {
			copied = kfifo_out_spinlocked(&kfifo_error_task,
				(unsigned char *)&error_task_id, sizeof(uint32_t), &kfifo_lock);
			if (copied != sizeof(uint32_t)) {
				ret = -DYN_ERR_API;
				pr_err("kfifio_out exception,return\n");
				goto end;
			}

			read_lock(lock);
			list_for_each_entry_safe(task_node, tmp_node, head, list) {
				if (task_node->task_id == error_task_id)
					break;
			}
			read_unlock(lock);

			if (!task_node) {
				ret = -DYN_NULL_POINTER;
				pr_err("task_node is null,return\n");
				goto end;
			}

			if (task_node->task_id == error_task_id) {
				if (task_node->handler) {
					pr_info("-----Measurement exception handler dispatching "
							"thread------\n");
					pr_info("Measurement exception received for task %d\n",
							error_task_id);
					pr_info("Step1: Query PSP for task %d status to confirm "
							"the error.\n", error_task_id);
					pr_info("Step2: Error confirmed, CALL measurement "
							"exception handler.\n");
					ret = psp_query_measure_status(error_task_id,
							&task_measure_status);
					if (ret) {
						pr_err("task_id %d status query failed\n",
								error_task_id);
						goto end;
					}

					if (task_measure_status.error == MER_ERR) {
						/*error--1  normal--0 */
						pr_info("Error detected for task %d, "
								"action TODO!\n", error_task_id);
						pr_info("----Measurement exception handler----\n");
						task_node->handler(error_task_id);
						pr_info("Exit measurement exception handler.\n");
					} else {
						pr_info("No error detected for task %d, please "
							"check it again!\n", error_task_id);
					}
				} else {
					pr_err("task %d's callback function is not registered, "
							"please check it\n", error_task_id);
				}
			}
		}
	}
end:
	return ret;
}

static int tdm_interrupt_handler(uint32_t id, uint64_t data)
{
	if (kthread) {
		kfifo_in_spinlocked(&kfifo_error_task, (unsigned char *)&data, sizeof(uint32_t),
				&kfifo_lock);
		wake_up_process(kthread);
	}

	return 0;
}

static int tdm_do_cmd(unsigned int cmd_id, void *cmd_data, int *error)
{
	if (cmd_id >= TDM_CMD_ID_MAX) {
		pr_err("%s cmd_id %u beyond limit\n", __func__, cmd_id);
		return -DYN_BEYOND_MAX;
	}

	return psp_do_cmd(TDM2PSP_CMD(cmd_id), cmd_data, error);
}

static int calc_task_context_hash(struct context_message context_msg, uint8_t *hash)
{
	int ret = 0;
	struct crypto_shash *shash = NULL;

	if (!hash) {
		ret = -DYN_NULL_POINTER;
		pr_err("Null pointer\n");
		goto end;
	}

	shash = crypto_alloc_shash("sha256", 0, 0);
	if (IS_ERR(shash)) {
		pr_err("can't alloc hash\n");
		return -DYN_ERR_API;
	}

	{
		SHASH_DESC_ON_STACK(sdesc, shash);

		sdesc->tfm = shash;

		ret = crypto_shash_init(sdesc);
		if (ret) {
			ret = -DYN_ERR_API;
			pr_err("crypto_shash_init failed\n");
			goto end;
		}

		if (context_msg.flag & CONTEXT_CHECK_PID) {
			ret = crypto_shash_update(sdesc, (uint8_t *)&context_msg.pid,
					sizeof(context_msg.pid));
			if (ret) {
				ret = -DYN_ERR_API;
				pr_err("crypto_shash_update failed\n");
				goto free_shash;
			}
		}

		if (context_msg.flag & CONTEXT_CHECK_COMM) {
			ret = crypto_shash_update(sdesc, context_msg.comm,
					strlen(context_msg.comm));
			if (ret) {
				ret = -DYN_ERR_API;
				pr_err("crypto_shash_update failed\n");
				goto free_shash;
			}
		}

		if (context_msg.flag & CONTEXT_CHECK_MODNAME) {
			ret = crypto_shash_update(sdesc, context_msg.module_name,
					strlen(context_msg.module_name));
			if (ret) {
				ret = -DYN_ERR_API;
				pr_err("crypto_shash_update failed\n");
				goto free_shash;
			}
		}

		ret = crypto_shash_final(sdesc, hash);
		if (ret) {
			ret = -DYN_ERR_API;
			pr_err("crypto_shash_final failed\n");
			goto free_shash;
		}
	}

free_shash:
	crypto_free_shash(shash);
end:
	return ret;
}

static int tdm_get_cmd_context_hash(uint32_t flag, uint8_t *hash)
{
	int ret = 0;
	struct context_message ctx_msg = {0};
	unsigned long return_address = 0;
#if IS_BUILTIN(CONFIG_CRYPTO_DEV_CCP_DD)
	struct module *p_module = NULL;
#elif IS_ENABLED(CONFIG_KALLSYMS)
	char symbol_buf[128] = {0};
	int symbol_len = 0;
	char *symbol_begin = NULL;
	char *symbol_end = NULL;
#endif

	if (!hash) {
		ret = -DYN_NULL_POINTER;
		pr_err("Null pointer\n");
		goto end;
	}

	ctx_msg.flag = flag;
	ctx_msg.pid = current->pid;
	memcpy(ctx_msg.comm, current->comm, sizeof(current->comm));

	return_address = CALLER_ADDR1;
	if (return_address) {
#if IS_BUILTIN(CONFIG_CRYPTO_DEV_CCP_DD)
		p_module = __module_address(return_address);
		// caller is module
		if (p_module)
			memcpy(ctx_msg.module_name, p_module->name, sizeof(p_module->name));
		// caller is build-in
		else
			memset(ctx_msg.module_name, 0, sizeof(ctx_msg.module_name));
#elif IS_ENABLED(CONFIG_KALLSYMS)
		symbol_len = sprint_symbol((char *)symbol_buf, return_address);
		if (!symbol_len) {
			ret = -DYN_ERR_API;
			pr_err("sprint_symbol failed\n");
			goto end;
		}
		symbol_begin = strchr((char *)symbol_buf, '[');
		if (!symbol_begin) {
			ret = -DYN_NULL_POINTER;
			pr_err("module name is not exist\n");
			goto end;
		}
		symbol_end = strchr((char *)symbol_buf, ']');
		if (!symbol_end) {
			ret = -DYN_NULL_POINTER;
			pr_err("module name is not exist\n");
			goto end;
		}
		symbol_begin++;
		if (symbol_end - symbol_begin)
			memcpy(ctx_msg.module_name, symbol_begin, symbol_end - symbol_begin);
		else
			memset(ctx_msg.module_name, 0, sizeof(ctx_msg.module_name));
#else
		memset(ctx_msg.module_name, 0, sizeof(ctx_msg.module_name));
#endif
	} else
		memset(ctx_msg.module_name, 0, sizeof(ctx_msg.module_name));

	ret = calc_task_context_hash(ctx_msg, hash);
	if (ret) {
		pr_err("calc_task_context_hash failed\n");
		goto end;
	}

end:
	return ret;
}

static int tdm_verify_phy_addr_valid(struct addr_range_info *range)
{
	int ret = 0;
#if IS_BUILTIN(CONFIG_CRYPTO_DEV_CCP_DD)
	int i;
	uint64_t phy_addr_start, phy_addr_end;

	for (i = 0; i < range->count; i++) {
		phy_addr_start = __sme_clr(range->addr[i].addr_start);
		phy_addr_end = __sme_clr(range->addr[i].addr_start + range->addr[i].length);

		if ((PHYS_PFN(phy_addr_start) >= max_pfn) || (PHYS_PFN(phy_addr_end) >= max_pfn)) {
			pr_err("phy_addr or length beyond max_pfn\n");
			ret = -DYN_ERR_MEM;
			break;
		}
	}
#else
	pr_warn("TDM: Can't get max_pfn, skip physical address check\n");
#endif

	return ret;
}

/* Convert the virtual address to physics address,then judge whether it is
 * continuous physics memory
 */
static int ptable_virt_to_phy(uint64_t vaddr, struct addr_info *p_addr_info, uint64_t *left_convert)
{
	int ret = 0;
	unsigned int level = 0;
	pte_t *pte;
	uint64_t local_page_mask = 0;
	uint64_t local_page_size = 0;
	uint64_t now_base = vaddr;
	uint64_t last_phy_addr = 0;
	uint64_t last_phy_len = 0;
	uint64_t now_phy_addr = 0;

	pte = lookup_address(now_base, &level);
	if (!pte) {
		ret = -DYN_ERR_MEM;
		pr_err("lookup_address failed!\n");
		goto end;
	}

	local_page_size = page_level_size(level);
	local_page_mask = page_level_mask(level);

	switch (level) {
	case PG_LEVEL_4K:
		p_addr_info->addr_start = (uint64_t)((pte_val(*pte) & local_page_mask & ~_PAGE_NX) +
				(now_base & ~local_page_mask));
		break;
	case PG_LEVEL_2M:
		p_addr_info->addr_start = (uint64_t)((pmd_val(*(pmd_t *)pte) & local_page_mask &
					~_PAGE_NX) + (now_base & ~local_page_mask));
		break;
	case PG_LEVEL_1G:
		p_addr_info->addr_start = (uint64_t)((pud_val(*(pud_t *)pte) & local_page_mask &
					~_PAGE_NX) + (now_base & ~local_page_mask));
		break;
	default:
		pr_err("page table level is not supported!\n");
		return -DYN_ERR_MEM;
	}

	if ((p_addr_info->addr_start & ~local_page_mask) == 0) {
		/*|--------------page_size-------------------|*/
		/*|-------*left_convert-------|*/
		if (*left_convert < local_page_size) {
			p_addr_info->length = *left_convert;
			*left_convert = 0;
		}
		/*|--------------page_size-------------------|-----*/
		/*|---------------------*left_convert-----------------------|*/
		else {
			p_addr_info->length = local_page_size;
			now_base += local_page_size;
			*left_convert -= local_page_size;
		}
	} else {
		/*|--------------page_size-------------------|------*/
		/*	|-------*left_convert---------|*/
		if ((p_addr_info->addr_start + *left_convert) <
				((p_addr_info->addr_start & local_page_mask) + local_page_size)) {
			p_addr_info->length = *left_convert;
			*left_convert = 0;
		}
		/*|--------------page_size-------------------|........*/
		/*	|-----------------*left_convert-----------------|*/
		else {
			p_addr_info->length = (p_addr_info->addr_start & local_page_mask) +
				local_page_size - p_addr_info->addr_start;
			now_base += p_addr_info->length;
			*left_convert -= p_addr_info->length;
		}
	}

	last_phy_len = p_addr_info->length;
	last_phy_addr = p_addr_info->addr_start;

	while (*left_convert) {
		pte = lookup_address(now_base, &level);
		if (!pte) {
			ret = -DYN_ERR_MEM;
			pr_err("lookup_address failed!\n");
			goto end;
		}

		switch (level) {
		case PG_LEVEL_4K:
			now_phy_addr = (uint64_t)((pte_val(*pte) & local_page_mask & ~_PAGE_NX) +
					(now_base & ~local_page_mask));
			break;
		case PG_LEVEL_2M:
			now_phy_addr = (uint64_t)((pmd_val(*(pmd_t *)pte) & local_page_mask &
						~_PAGE_NX) + (now_base & ~local_page_mask));
			break;
		case PG_LEVEL_1G:
			now_phy_addr = (uint64_t)((pud_val(*(pud_t *)pte) & local_page_mask &
						~_PAGE_NX) + (now_base & ~local_page_mask));
			break;
		default:
			pr_err("page table level is not supported!\n");
			return -DYN_ERR_MEM;
		}

		/*not continuous memory*/
		if ((last_phy_addr + last_phy_len) != now_phy_addr)
			break;

		if (*left_convert < local_page_size) {
			p_addr_info->length += *left_convert;
			*left_convert = 0;
		} else {
			p_addr_info->length += local_page_size;
			now_base += local_page_size;
			*left_convert -= local_page_size;
			last_phy_addr = now_phy_addr;
			last_phy_len = local_page_size;
		}
	}

end:
	return ret;
}

int psp_check_tdm_support(void)
{
	int ret = 0;
	struct tdm_version version;

	if (boot_cpu_data.x86_vendor == X86_VENDOR_HYGON) {
		if (tdm_support)
			goto end;

		ret = psp_get_fw_info(&version);
		if (ret) {
			tdm_support = 0;
			goto end;
		}

		tdm_support = 1;
	}

end:
	return tdm_support;
}
EXPORT_SYMBOL_GPL(psp_check_tdm_support);

int psp_get_fw_info(struct tdm_version *version)
{
	int ret = 0;
	int error;
	unsigned char *tdm_cmdresp_data = NULL;
	struct tdm_fw_cmd *fw_cmd = NULL;
	struct tdm_fw_resp *fw_resp = NULL;

	if (!version) {
		ret = -DYN_NULL_POINTER;
		pr_err("version is null pointer\n");
		goto end;
	}

	tdm_cmdresp_data = kzalloc(TDM_C2P_CMD_SIZE, GFP_KERNEL);
	if (!tdm_cmdresp_data) {
		ret = -DYN_ERR_MEM;
		pr_err("kzalloc for size %ld failed\n", TDM_C2P_CMD_SIZE);
		goto end;
	}

	fw_cmd = (struct tdm_fw_cmd *)tdm_cmdresp_data;
	fw_cmd->cmd_type = TDM_FW_VERSION;

	ret = tdm_do_cmd(0, (void *)fw_cmd, &error);
	if (ret && ret != -EIO) {
		pr_err("tdm_do_cmd failed cmd id: 0x%x, error: 0x%x\n", TDM2PSP_CMD(0), error);
		goto free_cmdresp;
	}

	if (error) {
		ret = -error;
		pr_warn("get_fw_info exception: 0x%x\n", error);
		goto free_cmdresp;
	}

	fw_resp = (struct tdm_fw_resp *)tdm_cmdresp_data;
	memcpy(version, &fw_resp->version, sizeof(struct tdm_version));

free_cmdresp:
	kfree(tdm_cmdresp_data);
end:
	return ret;
}
EXPORT_SYMBOL_GPL(psp_get_fw_info);

int psp_create_measure_task(struct addr_range_info *range, struct measure_data *data,
		uint32_t flag, struct authcode_2b *code)
{
	int ret = 0;
	int error;
	struct list_head *head = NULL;
	struct tdm_task_ctx *task_node = NULL;
	unsigned char *tdm_cmdresp_data = NULL;
	struct tdm_create_cmd *create_cmd = NULL;
	struct tdm_create_resp *create_resp = NULL;
	uint32_t addr_range_info_len = 0;
	struct addr_range_info *paddr_range_info = NULL;
	uint32_t info_index = 0;
	uint64_t now_base_vaddr = 0;
	uint64_t tf_left_size = 0;
	uint32_t count = 0;

	if (!range) {
		ret = -DYN_NULL_POINTER;
		pr_err("range is null pointer\n");
		goto end;
	}
	if (!data) {
		ret = -DYN_NULL_POINTER;
		pr_err("data is null pointer\n");
		goto end;
	}
	if (!code) {
		ret = -DYN_NULL_POINTER;
		pr_err("code is null pointer\n");
		goto end;
	}
	if (range->count > RANGE_CNT_MAX) {
		ret = -DYN_BEYOND_MAX;
		pr_err("range->count %d is beyond RANGE_CNT_MAX %d\n", range->count, RANGE_CNT_MAX);
		goto end;
	}
	if (range->count == 0) {
		ret = -DYN_ERR_SIZE_SMALL;
		pr_err("range->count is zero!\n");
		goto end;
	}

	/*create task by vaddr*/
	if (flag & TASK_CREATE_VADDR) {
		paddr_range_info = kzalloc(sizeof(struct addr_range_info) +
				RANGE_CNT_MAX * sizeof(struct addr_info), GFP_KERNEL);
		if (!paddr_range_info) {
			ret = -DYN_ERR_MEM;
			pr_err("kzalloc for paddr_range_info failed\n");
			goto end;
		}

		now_base_vaddr = range->addr[0].addr_start;
		tf_left_size = range->addr[0].length;
		while (tf_left_size && (count++ < RANGE_CNT_MAX + 1)) {
			ret = ptable_virt_to_phy(now_base_vaddr,
					&paddr_range_info->addr[info_index], &tf_left_size);
			if (ret) {
				pr_err("address convert failed!\n");
				goto free_paddr_range_info;
			}

			now_base_vaddr = now_base_vaddr +
				paddr_range_info->addr[info_index++].length;
			if (info_index > RANGE_CNT_MAX) {
				ret = -DYN_BEYOND_MAX;
				pr_err("info_index: %d is beyond %d\n", info_index, RANGE_CNT_MAX);
				goto free_paddr_range_info;
			}
		}

		paddr_range_info->count = info_index;
		addr_range_info_len = paddr_range_info->count * sizeof(struct addr_info) +
			sizeof(struct addr_range_info);
	} else {
		/*check if physics address valid*/
		ret = tdm_verify_phy_addr_valid(range);
		if (ret) {
			pr_err("range address is abnormal!\n");
			goto end;
		}
		addr_range_info_len = range->count * sizeof(struct addr_info) +
			sizeof(struct addr_range_info);
	}

	tdm_cmdresp_data = kzalloc(TDM_C2P_CMD_SIZE, GFP_KERNEL);
	if (!tdm_cmdresp_data) {
		ret = -DYN_ERR_MEM;
		pr_err("kzalloc for size %ld failed\n", TDM_C2P_CMD_SIZE);
		goto free_paddr_range_info;
	}

	create_cmd = (struct tdm_create_cmd *)tdm_cmdresp_data;
	create_cmd->cmd_type = TDM_TASK_CREATE;
	create_cmd->cmd_ctx_flag = flag;

	memcpy(&create_cmd->m_data, data, sizeof(struct measure_data));
	create_cmd->authcode_len = code->len > AUTHCODE_MAX ? AUTHCODE_MAX : code->len;

	ret = tdm_get_cmd_context_hash(flag, create_cmd->context_hash);
	if (ret) {
		pr_err("tdm_get_cmd_context_hash failed\n");
		goto free_cmdresp;
	}

	if (flag & TASK_CREATE_VADDR)
		memcpy(&create_cmd->range_info, paddr_range_info, addr_range_info_len);
	else
		memcpy(&create_cmd->range_info, range, addr_range_info_len);

	ret = tdm_do_cmd(0, (void *)create_cmd, &error);
	if (ret && ret != -EIO) {
		pr_err("tdm_do_cmd failed cmd id: 0x%x, error: 0x%x\n", TDM2PSP_CMD(0), error);
		goto free_cmdresp;
	}
	if (error) {
		ret = -error;
		pr_err("create_measure_task exception error: 0x%x\n", error);
		goto free_cmdresp;
	}

	create_resp = (struct tdm_create_resp *)tdm_cmdresp_data;
	code->len = create_resp->authcode_len;
	code->len = code->len > AUTHCODE_MAX ? AUTHCODE_MAX : code->len;
	memcpy(&code->val[0], &create_resp->authcode_val[0], code->len);

	head = &dyn_head.head;
	task_node = kzalloc(sizeof(struct tdm_task_ctx), GFP_KERNEL);
	if (!task_node) {
		ret = -DYN_ERR_MEM;
		pr_err("kzalloc for size %ld failed\n", sizeof(struct tdm_task_ctx));
		goto free_cmdresp;
	}

	task_node->task_id = create_resp->task_id;
	task_node->handler = NULL;
	task_node->cmd_ctx_flag = flag;

	ret = list_enqueue(task_node);
	if (ret) {
		pr_err("task %d enqueue failed!!!\n", task_node->task_id);
		goto free_task_node;
	}

	kfree(tdm_cmdresp_data);
	if (flag & TASK_CREATE_VADDR)
		kfree(paddr_range_info);

	return task_node->task_id;

free_task_node:
	kfree(task_node);
free_cmdresp:
	kfree(tdm_cmdresp_data);
free_paddr_range_info:
	if (flag & TASK_CREATE_VADDR)
		kfree(paddr_range_info);
end:
	return ret;
}
EXPORT_SYMBOL_GPL(psp_create_measure_task);

int psp_query_measure_status(uint32_t task_id, struct measure_status *status)
{
	int ret = 0;
	int error;
	unsigned char *tdm_cmdresp_data = NULL;
	struct tdm_query_cmd *query_cmd = NULL;
	struct tdm_query_resp *query_resp = NULL;

	if (!status) {
		ret = -DYN_NULL_POINTER;
		pr_err("status is null pointer\n");
		goto end;
	}

	if (!list_check_exist(task_id)) {
		pr_err("task %d isn't created\n", task_id);
		return -DYN_NOT_EXIST;
	}

	tdm_cmdresp_data = kzalloc(TDM_C2P_CMD_SIZE, GFP_KERNEL);
	if (!tdm_cmdresp_data) {
		ret = -DYN_ERR_MEM;
		pr_err("kzalloc for size %ld failed\n", TDM_C2P_CMD_SIZE);
		goto end;
	}

	query_cmd = (struct tdm_query_cmd *)tdm_cmdresp_data;
	query_cmd->cmd_type = TDM_TASK_QUERY;
	query_cmd->task_id = task_id;

	ret = tdm_do_cmd(0, query_cmd, &error);
	if (ret && ret != -EIO) {
		pr_err("tdm_do_cmd failed cmd id: 0x%x, error: 0x%x\n", TDM2PSP_CMD(0), error);
		goto free_cmdresp;
	}
	if (error) {
		ret = -error;
		pr_err("%s exception error: 0x%x\n", __func__, error);
		goto free_cmdresp;
	}

	query_resp = (struct tdm_query_resp *)tdm_cmdresp_data;
	memcpy(status, &query_resp->m_status, sizeof(struct measure_status));
free_cmdresp:
	kfree(tdm_cmdresp_data);
end:
	return ret;
}
EXPORT_SYMBOL_GPL(psp_query_measure_status);

int psp_register_measure_exception_handler(uint32_t task_id, struct authcode_2b *code,
		measure_exception_handler_t handler)
{
	int ret = 0;
	int error;
	struct list_head *head = NULL;
	struct tdm_task_ctx *task_node = NULL, *tmp_node = NULL;
	unsigned char *tdm_cmdresp_data = NULL;
	struct tdm_register_cmd *register_cmd = NULL;
	struct tdm_common_cmd *temp_cmd = NULL;
	rwlock_t *lock = NULL;

	if (!code) {
		ret = -DYN_NULL_POINTER;
		pr_err("code is null pointer\n");
		goto end;
	}
	if (code->len > AUTHCODE_MAX) {
		ret = -DYN_BEYOND_MAX;
		pr_err("authcode len %d is beyond AUTHCODE_MAX %d\n", code->len, AUTHCODE_MAX);
		goto end;
	}

	if (!list_check_exist(task_id)) {
		pr_err("task %d isn't created\n", task_id);
		return -DYN_NOT_EXIST;
	}
	/* check if task_id is registered already */
	head = &dyn_head.head;
	lock = &dyn_head.lock;

	read_lock(lock);
	list_for_each_entry_safe(task_node, tmp_node, head, list) {
		if (task_node->task_id == task_id) {
			if ((handler && task_node->handler)) {
				pr_err("task %d is registered already\n", task_id);
				read_unlock(lock);
				return -DYN_EEXIST;
			}
			break;
			/* task_node will be used for next context */
		}
	}
	read_unlock(lock);

	tdm_cmdresp_data = kzalloc(TDM_C2P_CMD_SIZE, GFP_KERNEL);
	if (!tdm_cmdresp_data) {
		ret = -DYN_ERR_MEM;
		pr_err("kzalloc for size %ld failed\n", TDM_C2P_CMD_SIZE);
		goto end;
	}

	register_cmd = (struct tdm_register_cmd *)tdm_cmdresp_data;
	temp_cmd = &register_cmd->cmd;
	temp_cmd->cmd_type = TDM_TASK_VERIFY_AUTH;
	temp_cmd->task_id = task_id;
	temp_cmd->code_len = code->len;
	temp_cmd->code_len = code->len > AUTHCODE_MAX ? AUTHCODE_MAX : temp_cmd->code_len;
	memcpy(temp_cmd->code_val, code->val, temp_cmd->code_len);

	ret = tdm_get_cmd_context_hash(task_node->cmd_ctx_flag, temp_cmd->context_hash);
	if (ret) {
		pr_err("tdm_get_cmd_context_hash failed\n");
		goto free_cmdresp;
	}

	ret = tdm_do_cmd(0, register_cmd, &error);
	if (ret && ret != -EIO) {
		pr_err("tdm_do_cmd failed cmd id: 0x%x, error: 0x%x\n", TDM2PSP_CMD(0), error);
		goto free_cmdresp;
	}
	if (error) {
		ret = -error;
		pr_err("%s exception error: 0x%x\n", __func__, error);
		goto free_cmdresp;
	}

	write_lock(lock);
	task_node->handler = handler;
	write_unlock(lock);

free_cmdresp:
	kfree(tdm_cmdresp_data);
end:
	return ret;
}
EXPORT_SYMBOL_GPL(psp_register_measure_exception_handler);

int psp_destroy_measure_task(uint32_t task_id, struct authcode_2b *code)
{
	int ret = 0;
	int error;
	struct list_head *head = NULL;
	struct tdm_task_ctx *task_node = NULL, *tmp_node = NULL;
	unsigned char *tdm_cmdresp_data = NULL;
	struct tdm_destroy_cmd *destroy_cmd = NULL;
	struct tdm_common_cmd *temp_cmd = NULL;
	rwlock_t *lock = NULL;

	if (!code) {
		ret = -DYN_NULL_POINTER;
		pr_err("code is null pointer\n");
		goto end;
	}
	if (code->len > AUTHCODE_MAX) {
		ret = -DYN_BEYOND_MAX;
		pr_err("authcode len %d is beyond AUTHCODE_MAX %d\n", code->len, AUTHCODE_MAX);
		goto end;
	}

	if (!list_check_exist(task_id)) {
		pr_err("task %d isn't created\n", task_id);
		return -DYN_NOT_EXIST;
	}

	head = &dyn_head.head;
	lock = &dyn_head.lock;

	read_lock(lock);
	list_for_each_entry_safe(task_node, tmp_node, head, list) {
		if (task_node->task_id == task_id)
			break;
	}
	read_unlock(lock);

	if (task_node->cmd_ctx_flag & TASK_ATTR_NO_UPDATE) {
		pr_warn("Task %d is not allowed to destroy!\n", task_node->task_id);
		ret = -DYN_NO_ALLOW_UPDATE;
		goto end;
	}

	tdm_cmdresp_data = kzalloc(TDM_C2P_CMD_SIZE, GFP_KERNEL);
	if (!tdm_cmdresp_data) {
		ret = -DYN_ERR_MEM;
		pr_err("kzalloc for size %ld failed\n", TDM_C2P_CMD_SIZE);
		goto end;
	}

	destroy_cmd = (struct tdm_destroy_cmd *)tdm_cmdresp_data;
	temp_cmd = &destroy_cmd->cmd;
	temp_cmd->cmd_type = TDM_TASK_DESTROY;
	temp_cmd->task_id = task_id;
	temp_cmd->code_len = code->len;
	temp_cmd->code_len = code->len > AUTHCODE_MAX ? AUTHCODE_MAX : temp_cmd->code_len;
	memcpy(temp_cmd->code_val, code->val, temp_cmd->code_len);

	ret = tdm_get_cmd_context_hash(task_node->cmd_ctx_flag, temp_cmd->context_hash);
	if (ret) {
		pr_err("tdm_get_cmd_context_hash failed\n");
		goto free_cmdresp;
	}

	ret = tdm_do_cmd(0, destroy_cmd, &error);
	if (ret && ret != -EIO) {
		pr_err("tdm_do_cmd failed cmd id: 0x%x, error: 0x%x\n", TDM2PSP_CMD(0), error);
		goto free_cmdresp;
	}
	if (error) {
		ret = -error;
		pr_err("%s exception error: 0x%x\n", __func__, error);
		goto free_cmdresp;
	}

	if (task_node->handler) {
		write_lock(lock);
		task_node->handler = NULL;
		write_unlock(lock);
	}

	write_lock(lock);
	list_del(&task_node->list);
	write_unlock(lock);

	kfree(task_node);

free_cmdresp:
	kfree(tdm_cmdresp_data);
end:
	return ret;
}
EXPORT_SYMBOL_GPL(psp_destroy_measure_task);

int psp_update_measure_task(uint32_t task_id, struct authcode_2b *code,
		struct measure_update_data *data)
{
	int ret = 0;
	int error;
	struct list_head *head = NULL;
	struct tdm_task_ctx *task_node = NULL, *tmp_node = NULL;
	unsigned char *tdm_cmdresp_data = NULL;
	struct tdm_update_cmd *update_cmd = NULL;
	struct tdm_common_cmd *temp_cmd = NULL;
	rwlock_t *lock = NULL;

	if (!data) {
		ret = -DYN_NULL_POINTER;
		pr_err("data is null pointer\n");
		goto end;
	}
	if (!code) {
		ret = -DYN_NULL_POINTER;
		pr_err("code is null pointer\n");
		goto end;
	}
	if (code->len > AUTHCODE_MAX) {
		ret = -DYN_BEYOND_MAX;
		pr_err("authcode len %d is beyond AUTHCODE_MAX %d\n", code->len, AUTHCODE_MAX);
		goto end;
	}

	if (!list_check_exist(task_id)) {
		pr_err("task %d isn't created\n", task_id);
		return -DYN_NOT_EXIST;
	}

	head = &dyn_head.head;
	lock = &dyn_head.lock;

	read_lock(lock);
	list_for_each_entry_safe(task_node, tmp_node, head, list) {
		if (task_node->task_id == task_id)
			break;
	}
	read_unlock(lock);

	if (task_node->cmd_ctx_flag & TASK_ATTR_NO_UPDATE) {
		pr_warn("Task %d is not allowed to update!\n", task_node->task_id);
		ret = -DYN_NO_ALLOW_UPDATE;
		goto end;
	}

	tdm_cmdresp_data = kzalloc(TDM_C2P_CMD_SIZE, GFP_KERNEL);
	if (!tdm_cmdresp_data) {
		ret = -DYN_ERR_MEM;
		pr_err("kzalloc for size %ld failed\n", TDM_C2P_CMD_SIZE);
		goto end;
	}

	update_cmd = (struct tdm_update_cmd *)tdm_cmdresp_data;
	temp_cmd = &update_cmd->cmd;
	temp_cmd->cmd_type = TDM_TASK_UPDATE;
	temp_cmd->task_id = task_id;
	temp_cmd->code_len = code->len;
	temp_cmd->code_len = code->len > AUTHCODE_MAX ? AUTHCODE_MAX : temp_cmd->code_len;
	memcpy(temp_cmd->code_val, code->val, temp_cmd->code_len);

	ret = tdm_get_cmd_context_hash(task_node->cmd_ctx_flag, temp_cmd->context_hash);
	if (ret) {
		pr_err("tdm_get_cmd_context_hash failed\n");
		goto free_cmdresp;
	}

	memcpy(&update_cmd->update_data, data, sizeof(struct measure_update_data));

	ret = tdm_do_cmd(0, tdm_cmdresp_data, &error);
	if (ret && ret != -EIO) {
		pr_err("tdm_do_cmd failed cmd id: 0x%x, error: 0x%x\n", TDM2PSP_CMD(0), error);
		goto free_cmdresp;
	}
	if (error) {
		ret = -error;
		pr_err("%s exception error: 0x%x\n", __func__, error);
		goto free_cmdresp;
	}

free_cmdresp:
	kfree(tdm_cmdresp_data);
end:
	return ret;
}
EXPORT_SYMBOL_GPL(psp_update_measure_task);

int psp_startstop_measure_task(uint32_t task_id, struct authcode_2b *code, bool start)
{
	int ret = 0;
	int error;
	struct list_head *head = NULL;
	struct tdm_task_ctx *task_node = NULL, *tmp_node = NULL;
	unsigned char *tdm_cmdresp_data = NULL;
	struct tdm_startstop_cmd *startstop_cmd = NULL;
	struct tdm_startstop_resp *startstop_resp = NULL;
	struct tdm_common_cmd *temp_cmd = NULL;
	rwlock_t *lock = NULL;

	if (!code) {
		ret = -DYN_NULL_POINTER;
		pr_err("code is null pointer\n");
		goto end;
	}
	if (code->len > AUTHCODE_MAX) {
		ret = -DYN_BEYOND_MAX;
		pr_err("authcode len %d is beyond AUTHCODE_MAX %d\n", code->len, AUTHCODE_MAX);
		goto end;
	}

	if (!list_check_exist(task_id)) {
		pr_err("task %d isn't created\n", task_id);
		return -DYN_NOT_EXIST;
	}

	head = &dyn_head.head;
	lock = &dyn_head.lock;

	read_lock(lock);
	list_for_each_entry_safe(task_node, tmp_node, head, list) {
		if (task_node->task_id == task_id)
			break;
	}
	read_unlock(lock);

	tdm_cmdresp_data = kzalloc(TDM_C2P_CMD_SIZE, GFP_KERNEL);
	if (!tdm_cmdresp_data) {
		ret = -DYN_ERR_MEM;
		pr_err("kzalloc for size %ld failed\n", TDM_C2P_CMD_SIZE);
		goto end;
	}

	startstop_cmd = (struct tdm_startstop_cmd *)tdm_cmdresp_data;
	temp_cmd = &startstop_cmd->cmd;
	temp_cmd->cmd_type = start ? TDM_TASK_START : TDM_TASK_STOP;
	temp_cmd->task_id = task_id;
	temp_cmd->code_len = code->len;
	temp_cmd->code_len = code->len > AUTHCODE_MAX ? AUTHCODE_MAX : temp_cmd->code_len;
	memcpy(temp_cmd->code_val, code->val, temp_cmd->code_len);

	if ((temp_cmd->cmd_type == TDM_TASK_STOP) && (task_node->cmd_ctx_flag &
				TASK_ATTR_NO_UPDATE)) {
		pr_warn("Task %d is not allowed to stop!\n", task_node->task_id);
		ret = -DYN_NO_ALLOW_UPDATE;
		goto free_cmdresp;
	}

	ret = tdm_get_cmd_context_hash(task_node->cmd_ctx_flag, temp_cmd->context_hash);
	if (ret) {
		pr_err("tdm_get_cmd_context_hash failed\n");
		goto free_cmdresp;
	}

	ret = tdm_do_cmd(0, startstop_cmd, &error);
	if (ret && ret != -EIO) {
		pr_err("tdm_do_cmd failed cmd id: 0x%x, error: 0x%x\n", TDM2PSP_CMD(0), error);
		goto free_cmdresp;
	}
	if (error) {
		ret = -error;
		pr_err("%s exception error: 0x%x\n", __func__, error);
		goto free_cmdresp;
	}

	startstop_resp = (struct tdm_startstop_resp *)tdm_cmdresp_data;

	kfree(tdm_cmdresp_data);

	return startstop_resp->m_status.status;

free_cmdresp:
	kfree(tdm_cmdresp_data);
end:
	return ret;
}
EXPORT_SYMBOL_GPL(psp_startstop_measure_task);

int tdm_export_cert(uint32_t key_usage_id, struct tdm_cert *cert)
{
	int ret = 0;
	int error;
	unsigned char *tdm_cmdresp_data = NULL;
	struct tdm_export_cert_cmd *cert_cmd = NULL;
	struct tdm_export_cert_resp *cert_resp = NULL;

	if (!cert) {
		ret = -DYN_NULL_POINTER;
		pr_err("cert is null pointer\n");
		goto end;
	}

	tdm_cmdresp_data = kzalloc(TDM_C2P_CMD_SIZE, GFP_KERNEL);
	if (!tdm_cmdresp_data) {
		ret = -DYN_ERR_MEM;
		pr_err("kzalloc for size %ld failed\n", TDM_C2P_CMD_SIZE);
		goto end;
	}

	cert_cmd = (struct tdm_export_cert_cmd *)tdm_cmdresp_data;
	cert_cmd->cmd_type = TDM_EXPORT_CERT;
	cert_cmd->key_usage_id = key_usage_id;

	ret = tdm_do_cmd(0, (void *)cert_cmd, &error);
	if (ret && ret != -EIO) {
		pr_err("tdm_do_cmd failed cmd id: 0x%x, error: 0x%x\n", TDM2PSP_CMD(0), error);
		goto free_cmdresp;
	}
	if (error) {
		ret = -error;
		pr_err("%s exception error: 0x%x\n", __func__, error);
		goto free_cmdresp;
	}

	cert_resp = (struct tdm_export_cert_resp *)tdm_cmdresp_data;
	memcpy(cert, &cert_resp->cert, sizeof(struct tdm_cert));

free_cmdresp:
	kfree(tdm_cmdresp_data);
end:
	return ret;
}
EXPORT_SYMBOL_GPL(tdm_export_cert);

int tdm_get_report(uint32_t task_id, struct task_selection_2b *selection,
		struct data_2b *user_supplied_data, uint8_t report_type, uint32_t key_usage_id,
		uint8_t *report_buffer, uint32_t *length)
{
	int ret = 0;
	int error;
	unsigned char *tdm_cmdresp_data = NULL;
	struct tdm_get_report_cmd *report_cmd = NULL;
	struct tdm_report *report_resp = NULL;
	uint32_t needed_length = 0;

	if (!user_supplied_data) {
		ret = -DYN_NULL_POINTER;
		pr_err("user_supplied_data is null pointer\n");
		goto end;
	}
	if (!report_buffer) {
		ret = -DYN_NULL_POINTER;
		pr_err("report_buffer is null pointer\n");
		goto end;
	}
	if (!length) {
		ret = -DYN_NULL_POINTER;
		pr_err("length  is null pointer\n");
		goto end;
	}
	if ((report_type != TDM_REPORT_SUMMARY) && (report_type != TDM_REPORT_DETAIL)) {
		ret = -DYN_ERR_REPORT_TYPE;
		pr_err("invalid report_type\n");
		goto end;
	}

	tdm_cmdresp_data = kzalloc(TDM_C2P_CMD_SIZE, GFP_KERNEL);
	if (!tdm_cmdresp_data) {
		ret = -DYN_ERR_MEM;
		pr_err("kzalloc for size %ld failed\n", TDM_C2P_CMD_SIZE);
		goto end;
	}

	report_cmd = (struct tdm_get_report_cmd *)tdm_cmdresp_data;

	report_cmd->cmd_type = TDM_GET_REPORT;
	report_cmd->task_id = task_id;
	if (task_id == TDM_TASK_ALL) {
		if (!selection) {
			ret = -DYN_NULL_POINTER;
			pr_err("selection is null pointer\n");
			goto end;
		}
		report_cmd->selection_len = selection->len;
		report_cmd->selection_len = (report_cmd->selection_len > TDM_MAX_TASK_BITMAP) ?
			TDM_MAX_TASK_BITMAP : report_cmd->selection_len;
		memcpy(&report_cmd->selection_bitmap[0], &selection->bitmap[0],
				report_cmd->selection_len);
	}

	report_cmd->user_data_len = (user_supplied_data->len > TDM_MAX_NONCE_SIZE) ?
		TDM_MAX_NONCE_SIZE : user_supplied_data->len;
	memcpy(&report_cmd->user_data_val[0], &user_supplied_data->val[0],
			report_cmd->user_data_len);
	report_cmd->report_type = report_type;
	report_cmd->key_usage_id = key_usage_id;

	ret = tdm_do_cmd(0, (void *)report_cmd, &error);
	if (ret && ret != -EIO) {
		pr_err("tdm_do_cmd failed cmd id: 0x%x, error: 0x%x\n", TDM2PSP_CMD(0), error);
		goto free_cmdresp;
	}
	if (error) {
		ret = -error;
		pr_err("%s exception error: 0x%x\n", __func__, error);
		goto free_cmdresp;
	}

	report_resp = (struct tdm_report *)tdm_cmdresp_data;
	if (report_type == TDM_REPORT_SUMMARY)
		needed_length = sizeof(struct tdm_report) + sizeof(struct tdm_report_sig);
	else
		needed_length = sizeof(struct tdm_report) +
			report_resp->task_nums * sizeof(struct tdm_detail_task_status) +
			sizeof(struct tdm_report_sig);

	if (needed_length > *length) {
		pr_warn("needed_length %d is beyond length %d\n", needed_length, *length);
		*length = needed_length;
		ret = -DYN_ERR_SIZE_SMALL;
	} else {
		memcpy(report_buffer, report_resp, needed_length);
	}

free_cmdresp:
	kfree(tdm_cmdresp_data);
end:
	return ret;
}
EXPORT_SYMBOL_GPL(tdm_get_report);

int tdm_get_vpcr_audit(struct pcr_select pcr, struct tpm2b_digest *digest,
		struct tdm_pcr_value_2b *pcr_values)
{
	int ret = 0;
	int error;
	unsigned char *tdm_cmdresp_data = NULL;
	struct tdm_get_vpcr_cmd *vpcr_cmd = NULL;
	struct tdm_get_vpcr_resp *vpcr_resp = NULL;

	if (!digest) {
		ret = -DYN_NULL_POINTER;
		pr_err("digest is null pointer\n");
		goto end;
	}
	if (!pcr_values) {
		ret = -DYN_NULL_POINTER;
		pr_err("pcr_values is null pointer\n");
		goto end;
	}

	tdm_cmdresp_data = kzalloc(TDM_C2P_CMD_SIZE, GFP_KERNEL);
	if (!tdm_cmdresp_data) {
		ret = -DYN_ERR_MEM;
		pr_err("kzalloc for size %ld failed\n", TDM_C2P_CMD_SIZE);
		goto end;
	}

	vpcr_cmd = (struct tdm_get_vpcr_cmd *)tdm_cmdresp_data;

	vpcr_cmd->cmd_type = TDM_VPCR_AUDIT;
	memcpy(&vpcr_cmd->pcr, &pcr, sizeof(struct pcr_select));

	ret = tdm_do_cmd(0, (void *)vpcr_cmd, &error);
	if (ret && ret != -EIO) {
		pr_err("tdm_do_cmd failed cmd id: 0x%x, error: 0x%x\n", TDM2PSP_CMD(0), error);
		goto free_cmdresp;
	}
	if (error) {
		ret = -error;
		pr_err("%s exception error: 0x%x\n", __func__, error);
		goto free_cmdresp;
	}

	vpcr_resp = (struct tdm_get_vpcr_resp *)tdm_cmdresp_data;
	memcpy(digest, &vpcr_resp->digest, sizeof(struct tpm2b_digest));
	pcr_values->task_nums = vpcr_resp->pcr_values.task_nums;
	memcpy(&pcr_values->task_data[0], &vpcr_resp->pcr_values.task_data[0],
			pcr_values->task_nums * sizeof(struct tdm_task_data));

free_cmdresp:
	kfree(tdm_cmdresp_data);
end:
	return ret;
}
EXPORT_SYMBOL_GPL(tdm_get_vpcr_audit);

static long tdm_ioctl(struct file *file, unsigned int ioctl, unsigned long arg)
{
	int  ret = 0;
	void __user *argp = (void __user *)arg;
	unsigned int tdm_cmd = 0;
	unsigned char *temp_cmd_data = NULL;
	struct task_selection_2b *selection = NULL;
	struct data_2b *data = NULL;
	uint32_t data_to_user_len = 0;
	uint16_t selection_len = 0;
	uint16_t user_data_len = 0;
	struct tdm_get_report_cmd *report_cmd = NULL;
	struct tdm_user_report_cmd *user_report_cmd = NULL;
	uint32_t needed_length = 0;
	struct tdm_get_vpcr_cmd *vpcr_cmd = NULL;
	struct tdm_get_vpcr_resp *vpcr_resp = NULL;
	uint32_t pcr_num = 0;

	if (_IOC_TYPE(ioctl) != TDM_IOC_TYPE) {
		ret = -EINVAL;
		pr_err("ioctl 0x%08x is invalid\n", ioctl);
		goto end;
	}

	temp_cmd_data = kzalloc(TDM_C2P_CMD_SIZE, GFP_KERNEL);
	if (!temp_cmd_data) {
		ret = -ENOMEM;
		pr_err("kzalloc for size 0x%lx failed\n", TDM_C2P_CMD_SIZE);
		goto end;
	}

	tdm_cmd = _IOC_NR(ioctl);

	switch (tdm_cmd) {
	case USER_EXPORT_CERT:
		ret = tdm_export_cert(TDM_AK_USAGE_ID, (struct tdm_cert *)temp_cmd_data);
		if (ret) {
			pr_err("Execute tdm export cert command failed!\n");
			goto free_mem;
		}
		data_to_user_len = sizeof(struct tdm_cert);
		break;

	case USER_GET_REPORT:
		if (copy_from_user(temp_cmd_data, argp, sizeof(struct tdm_user_report_cmd))) {
			pr_err("%s copy from user failed\n", __func__);
			ret = -EFAULT;
			goto end;
		}

		user_report_cmd = (struct tdm_user_report_cmd *)temp_cmd_data;
		needed_length = user_report_cmd->needed_length;
		report_cmd = &user_report_cmd->report_cmd;
		selection_len = report_cmd->selection_len > TDM_MAX_TASK_BITMAP ?
			TDM_MAX_TASK_BITMAP : report_cmd->selection_len;

		selection = kzalloc(sizeof(struct task_selection_2b) +
				selection_len * sizeof(uint8_t), GFP_KERNEL);
		if (!selection) {
			ret = -ENOMEM;
			pr_err("kzalloc failed\n");
			goto free_mem;
		}

		selection->len = selection_len;
		memcpy(&selection->bitmap[0], &report_cmd->selection_bitmap[0], selection->len);

		user_data_len = report_cmd->user_data_len > TDM_MAX_NONCE_SIZE ?
			TDM_MAX_NONCE_SIZE : report_cmd->user_data_len;
		data = kzalloc(sizeof(struct data_2b) +
				user_data_len * sizeof(uint8_t), GFP_KERNEL);
		if (!data) {
			ret = -ENOMEM;
			pr_err("kzalloc failed\n");
			goto free_mem;
		}

		data->len = user_data_len;
		memcpy(&data->val[0], &report_cmd->user_data_val[0], data->len);

		ret = tdm_get_report(report_cmd->task_id, selection, data, report_cmd->report_type,
				report_cmd->key_usage_id, temp_cmd_data, &needed_length);
		if (ret) {
			pr_err("Execute tdm report command failed!\n");
			goto free_mem;
		}

		data_to_user_len = needed_length;
		break;

	case USER_VPCR_AUDIT:
		if (copy_from_user(temp_cmd_data, argp, sizeof(struct tdm_get_vpcr_cmd))) {
			pr_err("%s copy from user failed\n", __func__);
			ret = -EFAULT;
			goto end;
		}

		vpcr_cmd = (struct tdm_get_vpcr_cmd *)temp_cmd_data;
		vpcr_resp = (struct tdm_get_vpcr_resp *)temp_cmd_data;
		pcr_num = vpcr_cmd->pcr.pcr;

		ret = tdm_get_vpcr_audit(vpcr_cmd->pcr, &vpcr_resp->digest, &vpcr_resp->pcr_values);
		if (ret) {
			pr_err("Execute tdm vpcr audit command failed!\n");
			goto free_mem;
		}

		vpcr_resp->pcr = pcr_num;
		data_to_user_len = sizeof(struct tdm_get_vpcr_resp) +
			vpcr_resp->pcr_values.task_nums * sizeof(struct tdm_task_data);
		break;

	case USER_SHOW_DEVICE:
		ret = psp_get_fw_info(&((struct tdm_show_device *)temp_cmd_data)->version);
		if (ret) {
			pr_err("firmware version get failed!\n");
			goto free_mem;
		}

		data_to_user_len = sizeof(struct tdm_show_device);
		break;

	default:
		pr_err("invalid tdm_cmd: %d from user\n", tdm_cmd);
		ret = -EINVAL;
		goto free_mem;
	}

	if (copy_to_user(argp, temp_cmd_data, data_to_user_len)) {
		pr_err("%s copy to user failed\n", __func__);
		ret = -EFAULT;
		goto free_mem;
	}

free_mem:
	kfree(temp_cmd_data);
	kfree(selection);
	kfree(data);
end:
	return ret;
}

static const struct file_operations tdm_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = tdm_ioctl,
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "tdm",
	.fops = &tdm_fops,
};

int tdm_dev_init(void)
{
	int ret = 0;

	if (tdm_init_flag)
		return 0;

	INIT_KFIFO(kfifo_error_task);
	INIT_LIST_HEAD(&dyn_head.head);
	rwlock_init(&dyn_head.lock);
	spin_lock_init(&kfifo_lock);

	ret = psp_register_cmd_notifier(p2c_cmd_id, tdm_interrupt_handler);
	if (ret) {
		pr_err("notifier function registration failed\n");
		return ret;
	}

	kthread = kthread_create(measure_exception_handling_thread, NULL,
			"measure_exception_handling_thread");
	if (IS_ERR(kthread)) {
		pr_err("kthread_create fail\n");
		ret = PTR_ERR(kthread);
		goto unreg;
	}

	wake_up_process(kthread);

	ret = misc_register(&misc);
	if (ret) {
		pr_err("misc_register for tdm failed\n");
		goto stop_kthread;
	}

	tdm_init_flag = 1;
	pr_info("TDM driver loaded successfully!\n");

	return ret;

stop_kthread:
	if (kthread) {
		kthread_stop(kthread);
		kthread = NULL;
	}
unreg:
	psp_unregister_cmd_notifier(p2c_cmd_id, tdm_interrupt_handler);

	return ret;
}

int tdm_dev_destroy(void)
{
	if (tdm_destroy_flag)
		goto end;

	if (kthread) {
		kthread_stop(kthread);
		kthread = NULL;
	}

	psp_unregister_cmd_notifier(p2c_cmd_id, tdm_interrupt_handler);

	misc_deregister(&misc);
	tdm_destroy_flag = 1;
end:
	return 0;
}


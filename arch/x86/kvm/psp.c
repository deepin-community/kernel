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
#include <linux/psp.h>
#include <linux/psp-hygon.h>

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "vpsp: " fmt

/*
 * The file mainly implements the base execution
 * logic of virtual PSP in kernel mode, which mainly includes:
 *	(1) Obtain the VM command and preprocess the pointer
 *		mapping table information in the command buffer
 *	(2) The command that has been converted will interact
 *		with the channel of the psp through the driver and
 *		try to obtain the execution result
 *	(3) The executed command data is recovered according to
 *		the multilevel pointer of the mapping table, and then returned to the VM
 *
 * The primary implementation logic of virtual PSP in kernel mode
 * call trace:
 * guest command(vmmcall)
 *	|
 *	|	   |-> kvm_pv_psp_cmd_pre_op
 *		   |		|
 *		   |		| -> guest_addr_map_table_op
 *		   |				|
 *		   |				| -> guest_multiple_level_gpa_replace
 *		   |
 *  kvm_pv_psp_op->|-> vpsp_try_do_cmd/vpsp_try_get_result <====> psp device driver
 *		   |
 *		   |
 *		   |-> kvm_pv_psp_cmd_post_op
 *				|
 *				| -> guest_addr_map_table_op
 *						|
 *						| -> guest_multiple_level_gpa_restore
 */

#define TKM_CMD_ID_MIN  0x120
#define TKM_CMD_ID_MAX  0x12f

struct psp_cmdresp_head {
	uint32_t buf_size;
	uint32_t cmdresp_size;
	uint32_t cmdresp_code;
} __packed;

/**
 * struct map_tbl - multilevel pointer address mapping table
 *
 * @parent_pa: parent address block's physics address
 * @offset: offset in parent address block
 * @size: submemory size
 * @align: submemory align size, hva need to keep size alignment in kernel
 * @hva: submemory copy block in kernel virtual address
 */
struct map_tbl {
	uint64_t parent_pa;
	uint32_t offset;
	uint32_t size;
	uint32_t align;
	uint64_t hva;
} __packed;

struct addr_map_tbls {
	uint32_t tbl_nums;
	struct map_tbl tbl[];
} __packed;

/* gpa and hva conversion maintenance table for internal use */
struct gpa2hva_t {
	void *hva;
	gpa_t gpa;
};

struct gpa2hva_tbls {
	uint32_t max_nums;
	uint32_t tbl_nums;
	struct gpa2hva_t tbl[];
};

/* save command data for restoring later */
struct vpsp_hbuf_wrapper {
	void *data;
	uint32_t data_size;
	struct addr_map_tbls *map_tbls;
	struct gpa2hva_tbls *g2h_tbls;
};

/* Virtual PSP host memory information maintenance, used in ringbuffer mode */
struct vpsp_hbuf_wrapper
g_hbuf_wrap[CSV_COMMAND_PRIORITY_NUM][CSV_RING_BUFFER_SIZE / CSV_RING_BUFFER_ESIZE] = {0};

void __maybe_unused map_tbl_dump(const char *title, struct addr_map_tbls *tbls)
{
	int i;

	pr_info("[%s]-> map_tbl_nums: %d", title, tbls->tbl_nums);
	for (i = 0; i < tbls->tbl_nums; i++) {
		pr_info("\t[%d]: parent_pa: 0x%llx, offset: 0x%x, size: 0x%x, align: 0x%x hva: 0x%llx",
			i, tbls->tbl[i].parent_pa, tbls->tbl[i].offset,
			tbls->tbl[i].size, tbls->tbl[i].align, tbls->tbl[i].hva);
	}
	pr_info("\n");
}

void __maybe_unused g2h_tbl_dump(const char *title, struct gpa2hva_tbls *tbls)
{
	int i;

	pr_info("[%s]-> g2h_tbl_nums: %d, max_nums: %d", title, tbls->tbl_nums,
		tbls->max_nums);
	for (i = 0; i < tbls->tbl_nums; i++)
		pr_info("\t[%d]: hva: 0x%llx, gpa: 0x%llx", i,
			(uint64_t)tbls->tbl[i].hva, tbls->tbl[i].gpa);
	pr_info("\n");
}

static int gpa2hva_tbl_fill(struct gpa2hva_tbls *tbls, void *hva, gpa_t gpa)
{
	uint32_t fill_idx = tbls->tbl_nums;

	if (fill_idx >= tbls->max_nums)
		return -EFAULT;

	tbls->tbl[fill_idx].hva = hva;
	tbls->tbl[fill_idx].gpa = gpa;
	tbls->tbl_nums = fill_idx + 1;

	return 0;
}

static void clear_hva_in_g2h_tbls(struct gpa2hva_tbls *g2h, void *hva)
{
	int i;

	for (i = 0; i < g2h->tbl_nums; i++) {
		if (g2h->tbl[i].hva == hva)
			g2h->tbl[i].hva = NULL;
	}
}

static void *get_hva_from_gpa(struct gpa2hva_tbls *g2h, gpa_t gpa)
{
	int i;

	for (i = 0; i < g2h->tbl_nums; i++) {
		if (g2h->tbl[i].gpa == gpa)
			return (void *)g2h->tbl[i].hva;
	}

	return NULL;
}

static gpa_t get_gpa_from_hva(struct gpa2hva_tbls *g2h, void *hva)
{
	int i;

	for (i = 0; i < g2h->tbl_nums; i++) {
		if (g2h->tbl[i].hva == hva)
			return g2h->tbl[i].gpa;
	}

	return 0;
}

/*
 * The virtual machine multilevel pointer command buffer handles the
 * execution entity, synchronizes the data in the original gpa to the
 * newly allocated hva(host virtual address) and updates the mapping
 * relationship in the parent memory
 */
static int guest_multiple_level_gpa_replace(struct kvm *kvm,
		struct map_tbl *tbl, struct gpa2hva_tbls *g2h)
{
	int ret = 0;
	uint32_t sub_block_size;
	uint64_t sub_paddr;
	void *parent_kva = NULL;

	/* kmalloc memory for child block */
	sub_block_size = max(tbl->size, tbl->align);
	tbl->hva = (uint64_t)kzalloc(sub_block_size, GFP_KERNEL);
	if (!tbl->hva)
		return -ENOMEM;

	/* get child gpa from parent gpa */
	if (unlikely(kvm_read_guest(kvm, tbl->parent_pa + tbl->offset,
		&sub_paddr, sizeof(sub_paddr)))) {
		pr_err("[%s]: kvm_read_guest for parent gpa failed\n",
			__func__);
		ret = -EFAULT;
		goto e_free;
	}

	/* copy child block data from gpa to hva */
	if (unlikely(kvm_read_guest(kvm, sub_paddr, (void *)tbl->hva,
		tbl->size))) {
		pr_err("[%s]: kvm_read_guest for sub_data failed\n",
			__func__);
		ret = -EFAULT;
		goto e_free;
	}

	/* get hva from gpa */
	parent_kva = get_hva_from_gpa(g2h, tbl->parent_pa);
	if (unlikely(!parent_kva)) {
		pr_err("[%s]: get_hva_from_gpa for parent_pa failed\n",
			__func__);
		ret = -EFAULT;
		goto e_free;
	}

	/* replace pa of hva from gpa */
	*(uint64_t *)((uint8_t *)parent_kva + tbl->offset) = __psp_pa(tbl->hva);

	/* fill in gpa and hva to map table for restoring later */
	if (unlikely(gpa2hva_tbl_fill(g2h, (void *)tbl->hva, sub_paddr))) {
		pr_err("[%s]: gpa2hva_tbl_fill for sub_addr failed\n",
			__func__);
		ret = -EFAULT;
		goto e_free;
	}

	return ret;

e_free:
	kfree((const void *)tbl->hva);
	return ret;
}

/* The virtual machine multi-level pointer command memory handles the
 * execution entity, synchronizes the data in the hva(host virtual
 * address) back to the memory corresponding to the gpa, and restores
 * the mapping relationship in the original parent memory
 */
static int guest_multiple_level_gpa_restore(struct kvm *kvm,
		struct map_tbl *tbl, struct gpa2hva_tbls *g2h)
{
	int ret = 0;
	gpa_t sub_gpa;
	void *parent_hva = NULL;

	/* get gpa from hva */
	sub_gpa = get_gpa_from_hva(g2h, (void *)tbl->hva);
	if (unlikely(!sub_gpa)) {
		pr_err("[%s]: get_gpa_from_hva for sub_gpa failed\n",
			__func__);
		ret = -EFAULT;
		goto end;
	}

	/* copy child block data from hva to gpa */
	if (unlikely(kvm_write_guest(kvm, sub_gpa, (void *)tbl->hva,
				tbl->size))) {
		pr_err("[%s]: kvm_write_guest for sub_gpa failed\n",
			__func__);
		ret = -EFAULT;
		goto end;
	}

	/* get parent hva from parent gpa  */
	parent_hva = get_hva_from_gpa(g2h, tbl->parent_pa);
	if (unlikely(!parent_hva)) {
		pr_err("[%s]: get_hva_from_gpa for parent_pa failed\n",
			__func__);
		ret = -EFAULT;
		goto end;
	}

	/* restore gpa from pa of hva in parent block  */
	*(uint64_t *)((uint8_t *)parent_hva + tbl->offset) = sub_gpa;

	/* free child block memory  */
	clear_hva_in_g2h_tbls(g2h, (void *)tbl->hva);
	kfree((const void *)tbl->hva);
	tbl->hva = 0;

end:
	return ret;
}

/*
 * The virtual machine multilevel pointer command memory processing
 * executes upper-layer abstract interfaces, including replacing and
 * restoring two sub-processing functions
 */
static int guest_addr_map_table_op(struct kvm *kvm, struct gpa2hva_tbls *g2h,
	struct addr_map_tbls *map_tbls, int op)
{
	int ret = 0;
	int i;
	uint64_t *sub_paddr_ptr;

	if (op) {
		for (i = map_tbls->tbl_nums - 1; i >= 0; i--) {
			/* check if the gpa of root points to itself */
			if (map_tbls->tbl[i].parent_pa == g2h->tbl[0].gpa) {
				sub_paddr_ptr = (uint64_t *)((uint8_t *)g2h->tbl[0].hva
						+ map_tbls->tbl[i].offset);
				/* if the child paddr is equal to the parent paddr */
				if ((uint64_t)g2h->tbl[0].hva == map_tbls->tbl[i].hva) {
					*sub_paddr_ptr = g2h->tbl[0].gpa;
					continue;
				}
			}

			/* restore new pa of kva with the gpa from guest */
			if (unlikely(guest_multiple_level_gpa_restore(kvm,
				&map_tbls->tbl[i], g2h))) {
				pr_err("[%s]: guest_multiple_level_gpa_restore failed\n",
					__func__);
				ret = -EFAULT;
				goto end;
			}
		}
	} else {
		for (i = 0; i < map_tbls->tbl_nums; i++) {
			/* check if the gpa of root points to itself */
			if (map_tbls->tbl[i].parent_pa == g2h->tbl[0].gpa) {
				sub_paddr_ptr = (uint64_t *)((uint8_t *)g2h->tbl[0].hva
							+ map_tbls->tbl[i].offset);
				/* if the child paddr is equal to the parent paddr */
				if (*sub_paddr_ptr == map_tbls->tbl[i].parent_pa) {
					*sub_paddr_ptr = __psp_pa(g2h->tbl[0].hva);
					map_tbls->tbl[i].hva = (uint64_t)g2h->tbl[0].hva;
					continue;
				}
			}

			/* check if parent_pa is valid */
			if (unlikely(!get_hva_from_gpa(g2h, map_tbls->tbl[i].parent_pa))) {
				pr_err("[%s]: g2h->tbl[%d].parent_pa: 0x%llx is invalid\n",
					__func__, i, map_tbls->tbl[i].parent_pa);
				ret = -EFAULT;
				goto end;
			}

			/* replace the gpa from guest with the new pa of kva */
			if (unlikely(guest_multiple_level_gpa_replace(kvm,
				&map_tbls->tbl[i], g2h))) {
				pr_err("[%s]: guest_multiple_level_gpa_replace failed\n",
					__func__);
				ret = -EFAULT;
				goto end;
			}
		}
	}

end:
	return ret;
}

static void kvm_pv_psp_mem_free(struct gpa2hva_tbls *g2h, struct addr_map_tbls
	*map_tbl, void *data)
{
	int i;

	if (g2h) {
		for (i = 0; i < g2h->tbl_nums; i++) {
			if (g2h->tbl[i].hva && (g2h->tbl[i].hva != data)) {
				kfree(g2h->tbl[i].hva);
				g2h->tbl[i].hva = NULL;
			}
		}
		kfree(g2h);
	}

	kfree(map_tbl);
	kfree(data);
}

/*
 * Obtain the VM command and preprocess the pointer mapping table
 * information in the command buffer, the processed data will be
 * used to interact with the psp device
 */
static int kvm_pv_psp_cmd_pre_op(struct kvm *kvm, gpa_t data_gpa,
		gpa_t table_gpa, struct vpsp_hbuf_wrapper *hbuf)
{
	int ret = 0;
	void *data = NULL;
	struct psp_cmdresp_head psp_head;
	uint32_t data_size;
	struct addr_map_tbls map_head, *map_tbls = NULL;
	uint32_t map_tbl_size;
	struct gpa2hva_tbls *g2h = NULL;
	uint32_t g2h_tbl_size;

	if (unlikely(kvm_read_guest(kvm, data_gpa, &psp_head,
					sizeof(struct psp_cmdresp_head))))
		return -EFAULT;

	data_size = psp_head.buf_size;
	data = kzalloc(data_size, GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	if (unlikely(kvm_read_guest(kvm, data_gpa, data, data_size))) {
		ret = -EFAULT;
		goto end;
	}

	if (table_gpa) {
		/* parse address map table from guest */
		if (unlikely(kvm_read_guest(kvm, table_gpa, &map_head,
			sizeof(struct addr_map_tbls)))) {
			pr_err("[%s]: kvm_read_guest for map_head failed\n",
				__func__);
			ret = -EFAULT;
			goto end;
		}

		map_tbl_size = sizeof(struct addr_map_tbls) + map_head.tbl_nums
			* sizeof(struct map_tbl);
		map_tbls = kzalloc(map_tbl_size, GFP_KERNEL);
		if (!map_tbls) {
			ret = -ENOMEM;
			goto end;
		}

		if (unlikely(kvm_read_guest(kvm, table_gpa, map_tbls,
			map_tbl_size))) {
			pr_err("[%s]: kvm_read_guest for map_tbls failed\n",
				__func__);
			ret = -EFAULT;
			goto end;
		}

		/* init for gpa2hva table*/
		g2h_tbl_size = sizeof(struct gpa2hva_tbls) + (map_head.tbl_nums
			+ 1) * sizeof(struct gpa2hva_t);
		g2h = kzalloc(g2h_tbl_size, GFP_KERNEL);
		if (!g2h) {
			ret = -ENOMEM;
			goto end;
		}
		g2h->max_nums = map_head.tbl_nums + 1;

		/* fill the root parent address */
		if (gpa2hva_tbl_fill(g2h, data, data_gpa)) {
			pr_err("[%s]: gpa2hva_tbl_fill for root data address failed\n",
				__func__);
			ret = -EFAULT;
			goto end;
		}

		if (guest_addr_map_table_op(kvm, g2h, map_tbls, 0)) {
			pr_err("[%s]: guest_addr_map_table_op for replacing failed\n",
				__func__);
			ret = -EFAULT;
			goto end;
		}
	}

	hbuf->data = data;
	hbuf->data_size = data_size;
	hbuf->map_tbls = map_tbls;
	hbuf->g2h_tbls = g2h;

end:
	return ret;
}

/*
 * The executed command data is recovered according to the multilevel
 * pointer of the mapping table when the command has finished
 * interacting with the psp device
 */
static int kvm_pv_psp_cmd_post_op(struct kvm *kvm, gpa_t data_gpa,
		struct vpsp_hbuf_wrapper *hbuf)
{
	int ret = 0;

	if (hbuf->map_tbls) {
		if (guest_addr_map_table_op(kvm, hbuf->g2h_tbls,
					hbuf->map_tbls, 1)) {
			pr_err("[%s]: guest_addr_map_table_op for restoring failed\n",
				__func__);
			ret = -EFAULT;
			goto end;
		}
	}

	/* restore cmdresp's buffer from context */
	if (unlikely(kvm_write_guest(kvm, data_gpa, hbuf->data,
					hbuf->data_size))) {
		pr_err("[%s]: kvm_write_guest for cmdresp data failed\n",
			__func__);
		ret = -EFAULT;
		goto end;
	}

end:
	/* release memory and clear hbuf */
	kvm_pv_psp_mem_free(hbuf->g2h_tbls, hbuf->map_tbls, hbuf->data);
	memset(hbuf, 0, sizeof(*hbuf));

	return ret;
}

static int cmd_type_is_tkm(int cmd)
{
	if (cmd >= TKM_CMD_ID_MIN && cmd <= TKM_CMD_ID_MAX)
		return 1;
	return 0;
}

/*
 * The primary implementation interface of virtual PSP in kernel mode
 */
int kvm_pv_psp_op(struct kvm *kvm, int cmd, gpa_t data_gpa, gpa_t psp_ret_gpa,
		gpa_t table_gpa)
{
	int ret = 0;
	struct vpsp_ret psp_ret = {0};
	struct vpsp_hbuf_wrapper hbuf = {0};
	struct vpsp_cmd *vcmd = (struct vpsp_cmd *)&cmd;
	uint8_t prio = CSV_COMMAND_PRIORITY_LOW;
	uint32_t index = 0;
	uint32_t vid = 0;

	// only tkm cmd need vid
	if (cmd_type_is_tkm(vcmd->cmd_id)) {
		// if vm without set vid, then tkm command is not allowed
		ret = vpsp_get_vid(&vid, kvm->userspace_pid);
		if (ret) {
			pr_err("[%s]: not allowed tkm command without vid\n", __func__);
			return -EFAULT;
		}
	}

	if (unlikely(kvm_read_guest(kvm, psp_ret_gpa, &psp_ret,
					sizeof(psp_ret))))
		return -EFAULT;

	switch (psp_ret.status) {
	case VPSP_INIT:
		/* multilevel pointer replace*/
		ret = kvm_pv_psp_cmd_pre_op(kvm, data_gpa, table_gpa, &hbuf);
		if (unlikely(ret)) {
			psp_ret.status = VPSP_FINISH;
			pr_err("[%s]: kvm_pv_psp_cmd_pre_op failed\n",
					__func__);
			ret = -EFAULT;
			goto end;
		}

		/* try to send command to the device for execution*/
		ret = vpsp_try_do_cmd(vid, cmd, (void *)hbuf.data,
				(struct vpsp_ret *)&psp_ret);
		if (unlikely(ret)) {
			pr_err("[%s]: vpsp_do_cmd failed\n", __func__);
			ret = -EFAULT;
			goto end;
		}

		switch (psp_ret.status) {
		case VPSP_RUNNING:
			/* backup host memory message for restoring later*/
			prio = vcmd->is_high_rb ? CSV_COMMAND_PRIORITY_HIGH :
				CSV_COMMAND_PRIORITY_LOW;
			g_hbuf_wrap[prio][psp_ret.index] = hbuf;
			break;

		case VPSP_FINISH:
			/* restore multilevel pointer data */
			ret = kvm_pv_psp_cmd_post_op(kvm, data_gpa, &hbuf);
			if (unlikely(ret)) {
				pr_err("[%s]: kvm_pv_psp_cmd_post_op failed\n",
						__func__);
				ret = -EFAULT;
				goto end;
			}
			break;

		default:
			ret = -EFAULT;
			break;
		}
		break;

	case VPSP_RUNNING:
		prio = vcmd->is_high_rb ? CSV_COMMAND_PRIORITY_HIGH :
			CSV_COMMAND_PRIORITY_LOW;
		index = psp_ret.index;
		/* try to get the execution result from ringbuffer*/
		ret = vpsp_try_get_result(vid, prio, index, g_hbuf_wrap[prio][index].data,
				(struct vpsp_ret *)&psp_ret);
		if (unlikely(ret)) {
			pr_err("[%s]: vpsp_try_get_result failed\n", __func__);
			ret = -EFAULT;
			goto end;
		}

		switch (psp_ret.status) {
		case VPSP_RUNNING:
			break;

		case VPSP_FINISH:
			/* restore multilevel pointer data */
			ret = kvm_pv_psp_cmd_post_op(kvm, data_gpa,
					&g_hbuf_wrap[prio][index]);
			if (unlikely(ret)) {
				pr_err("[%s]: kvm_pv_psp_cmd_post_op failed\n",
						__func__);
				ret = -EFAULT;
				goto end;
			}
			break;

		default:
			ret = -EFAULT;
			break;
		}
		break;

	default:
		pr_err("[%s]: invalid command status\n", __func__);
		ret = -EFAULT;
		break;
	}
end:
	/* return psp_ret to guest */
	kvm_write_guest(kvm, psp_ret_gpa, &psp_ret, sizeof(psp_ret));
	return ret;
}

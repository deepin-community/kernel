// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/gfp.h>

#include "ossl_knl.h"
#include "hinic3_crm.h"
#include "hinic3_hw.h"
#include "hinic3_hwdev.h"
#include "cqm_memsec.h"
#include "cqm_object.h"
#include "cqm_bat_cla.h"
#include "cqm_cmd.h"
#include "cqm_object_intern.h"
#include "cqm_main.h"

#include "cqm_npu_cmd.h"
#include "cqm_npu_cmd_defs.h"
#include "vram_common.h"

#define common_section

struct malloc_memory {
	bool (*check_alloc_mode)(struct hinic3_hwdev *handle, struct tag_cqm_buf *buf);
	s32 (*malloc_func)(struct hinic3_hwdev *handle, struct tag_cqm_buf *buf);
};

struct free_memory {
	bool (*check_alloc_mode)(struct hinic3_hwdev *handle, struct tag_cqm_buf *buf);
	void (*free_func)(struct tag_cqm_buf *buf);
};

/**
 * Prototype    : cqm_swab64(Encapsulation of __swab64)
 * Description  : Perform big-endian conversion for a memory block (8 bytes).
 * Input        : u8 *addr: Start address of the memory block
 *		  u32 cnt: Number of 8 bytes in the memory block
 * Output       : None
 * Return Value : void
 * 1.Date         : 2015/4/15
 *   Modification : Created function
 */
void cqm_swab64(u8 *addr, u32 cnt)
{
	u64 *temp = (u64 *)addr;
	u64 value = 0;
	u32 i;

	for (i = 0; i < cnt; i++) {
		value = __swab64(*temp);
		*temp = value;
		temp++;
	}
}

/**
 * Prototype    : cqm_swab32(Encapsulation of __swab32)
 * Description  : Perform big-endian conversion for a memory block (4 bytes).
 * Input        : u8 *addr: Start address of the memory block
 *		  u32 cnt: Number of 4 bytes in the memory block
 * Output       : None
 * Return Value : void
 * 1.Date         : 2015/7/23
 *   Modification : Created function
 */
void cqm_swab32(u8 *addr, u32 cnt)
{
	u32 *temp = (u32 *)addr;
	u32 value = 0;
	u32 i;

	for (i = 0; i < cnt; i++) {
		value = __swab32(*temp);
		*temp = value;
		temp++;
	}
}

/**
 * Prototype    : cqm_shift
 * Description  : Calculates n in a 2^n number.(Find the logarithm of 2^n)
 * Input        : u32 data
 * Output       : None
 * Return Value : s32
 * 1.Date         : 2015/4/15
 *   Modification : Created function
 */
s32 cqm_shift(u32 data)
{
	u32 data_num = data;
	s32 shift = -1;

	do {
		data_num >>= 1;
		shift++;
	} while (data_num);

	return shift;
}

/**
 * Prototype    : cqm_check_align
 * Description  : Check whether the value is 2^n-aligned. If 0 or 1, false is
 *		  returned.
 * Input        : u32 data
 * Output       : None
 * Return Value : s32
 * 1.Date         : 2015/9/15
 *   Modification : Created function
 */
bool cqm_check_align(u32 data)
{
	u32 data_num = data;

	if (data == 0)
		return false;

	/* Todo: (n & (n - 1) == 0) can be used to determine the value. */
	do {
		/* When the value can be exactly divided by 2,
		 * the value of data is shifted right by one bit, that is,
		 * divided by 2.
		 */
		if ((data_num & 0x1) == 0)
			data_num >>= 1;
			/* If the value cannot be divisible by 2, the value is
			 * not 2^n-aligned and false is returned.
			 */
		else
			return false;
	} while (data_num != 1);

	return true;
}

/**
 * Prototype    : cqm_kmalloc_align
 * Description  : Allocates 2^n-byte-aligned memory for the start address.
 * Input        : size_t size
 *		  gfp_t flags
 *		  u16 align_order
 * Output       : None
 * Return Value : void *
 * 1.Date         : 2017/9/22
 *   Modification : Created function
 */
void *cqm_kmalloc_align(size_t size, gfp_t flags, u16 align_order)
{
	void *orig_addr = NULL;
	void *align_addr = NULL;
	void *index_addr = NULL;

	orig_addr = kmalloc(size + ((u64)1 << align_order) + sizeof(void *),
			    flags);
	if (!orig_addr)
		return NULL;

	index_addr = (void *)((char *)orig_addr + sizeof(void *));
	align_addr =
	    (void *)((((u64)index_addr + ((u64)1 << align_order) - 1) >>
		      align_order) << align_order);

	/* Record the original memory address for memory release. */
	index_addr = (void *)((char *)align_addr - sizeof(void *));
	*(void **)index_addr = orig_addr;

	return align_addr;
}

/**
 * Prototype    : cqm_kfree_align
 * Description  : Release the memory allocated for starting address alignment.
 * Input        : void *addr
 * Output       : None
 * Return Value : void
 * 1.Date         : 2017/9/22
 *   Modification : Created function
 */
void cqm_kfree_align(void *addr)
{
	void *index_addr = NULL;

	/* Release the original memory address. */
	index_addr = (void *)((char *)addr - sizeof(void *));

	cqm_dbg("free aligned address: %p, original address: %p\n", addr,
		*(void **)index_addr);

	kfree(*(void **)index_addr);
}

static void cqm_write_lock(rwlock_t *lock, bool bh)
{
	if (bh)
		write_lock_bh(lock);
	else
		write_lock(lock);
}

static void cqm_write_unlock(rwlock_t *lock, bool bh)
{
	if (bh)
		write_unlock_bh(lock);
	else
		write_unlock(lock);
}

static void cqm_read_lock(rwlock_t *lock, bool bh)
{
	if (bh)
		read_lock_bh(lock);
	else
		read_lock(lock);
}

static void cqm_read_unlock(rwlock_t *lock, bool bh)
{
	if (bh)
		read_unlock_bh(lock);
	else
		read_unlock(lock);
}

static inline bool cqm_bat_entry_in_secure_mem(void *handle, u32 type)
{
	if (!cqm_need_secure_mem(handle))
		return false;

	if (type == CQM_BAT_ENTRY_T_QPC || type == CQM_BAT_ENTRY_T_SCQC ||
	    type == CQM_BAT_ENTRY_T_SRQC || type == CQM_BAT_ENTRY_T_MPT)
		return true;

	return false;
}

s32 cqm_buf_alloc_direct(struct tag_cqm_handle *cqm_handle, struct tag_cqm_buf *buf, bool direct)
{
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;
	struct page **pages = NULL;
	u32 i, j, order;

	order = (u32)get_order(buf->buf_size);

	if (!direct) {
		buf->direct.va = NULL;
		return CQM_SUCCESS;
	}

	pages = vmalloc(sizeof(struct page *) * buf->page_number);
	if (!pages)
		return CQM_FAIL;

	for (i = 0; i < buf->buf_number; i++) {
		for (j = 0; j < ((u32)1 << order); j++)
			pages[(ulong)(unsigned int)((i << order) + j)] =
				(void *)virt_to_page((u8 *)(buf->buf_list[i].va) + (PAGE_SIZE * j));
	}

	buf->direct.va = vmap(pages, buf->page_number, VM_MAP, PAGE_KERNEL);
	vfree(pages);
	if (!buf->direct.va) {
		cqm_err(handle->dev_hdl, CQM_MAP_FAIL(buf->direct.va));
		return CQM_FAIL;
	}

	return CQM_SUCCESS;
}

static bool check_use_vram(struct hinic3_hwdev *handle, struct tag_cqm_buf *buf)
{
	return buf->buf_info.use_vram ? true : false;
}

static bool check_use_non_vram(struct hinic3_hwdev *handle, struct tag_cqm_buf *buf)
{
	return buf->buf_info.use_vram ? false : true;
}

static bool check_for_use_node_alloc(struct hinic3_hwdev *handle, struct tag_cqm_buf *buf)
{
	if (buf->buf_info.use_vram == 0 && handle->board_info.service_mode == 0)
		return true;

	return false;
}

static bool check_for_nouse_node_alloc(struct hinic3_hwdev *handle, struct tag_cqm_buf *buf)
{
	if (buf->buf_info.use_vram == 0 && handle->board_info.service_mode != 0)
		return true;

	return false;
}

static s32 cqm_buf_vram_kalloc(struct hinic3_hwdev *handle, struct tag_cqm_buf *buf)
{
	void *vaddr = NULL;
	int i;

	vaddr = hi_vram_kalloc(buf->buf_info.buf_vram_name, (u64)buf->buf_size * buf->buf_number);
	if (!vaddr) {
		cqm_err(handle->dev_hdl, CQM_ALLOC_FAIL(buf_page));
		return CQM_FAIL;
	}

	for (i = 0; i < (s32)buf->buf_number; i++)
		buf->buf_list[i].va = (void *)((char *)vaddr + i * (u64)buf->buf_size);

	return CQM_SUCCESS;
}

static void cqm_buf_vram_free(struct tag_cqm_buf *buf)
{
	s32 i;

	if (!buf->buf_list)
		return;

	if (buf->buf_list[0].va)
		hi_vram_kfree(buf->buf_list[0].va, buf->buf_info.buf_vram_name,
			      (u64)buf->buf_size * buf->buf_number);

	for (i = 0; i < (s32)buf->buf_number; i++)
		buf->buf_list[i].va = NULL;
}

static void cqm_buf_free_page_common(struct tag_cqm_buf *buf)
{
	u32 order;
	s32 i;

	if (!buf->buf_list)
		return;

	order = (u32)get_order(buf->buf_size);

	for (i = 0; i < (s32)buf->buf_number; i++) {
		if (buf->buf_list[i].va) {
			free_pages((ulong)(buf->buf_list[i].va), order);
			buf->buf_list[i].va = NULL;
		}
	}
}

static s32 cqm_buf_use_node_alloc_page(struct hinic3_hwdev *handle, struct tag_cqm_buf *buf)
{
	struct page *newpage = NULL;
	u32 order;
	void *va = NULL;
	s32 i, node;

	order = (u32)get_order(buf->buf_size);
	node = dev_to_node(handle->dev_hdl);
	for (i = 0; i < (s32)buf->buf_number; i++) {
		newpage = alloc_pages_node(node, GFP_KERNEL | __GFP_ZERO, order);
		if (!newpage) {
			cqm_err(handle->dev_hdl, CQM_ALLOC_FAIL(buf_page));
			break;
		}
		va = (void *)page_address(newpage);
		/* Initialize the page after the page is applied for.
		 * If hash entries are involved, the initialization
		 * value must be 0.
		 */
		memset(va, 0, buf->buf_size);
		buf->buf_list[i].va = va;
	}

	if (i != buf->buf_number) {
		cqm_buf_free_page_common(buf);
		return CQM_FAIL;
	}

	return CQM_SUCCESS;
}

static s32 cqm_buf_unused_node_alloc_page(struct hinic3_hwdev *handle, struct tag_cqm_buf *buf)
{
	u32 order;
	void *va = NULL;
	s32 i;

	order = (u32)get_order(buf->buf_size);

	for (i = 0; i < (s32)buf->buf_number; i++) {
		va = (void *)ossl_get_free_pages(GFP_KERNEL | __GFP_ZERO, order);
		if (!va) {
			cqm_err(handle->dev_hdl, CQM_ALLOC_FAIL(buf_page));
			break;
		}
		/* Initialize the page after the page is applied for.
		 * If hash entries are involved, the initialization
		 * value must be 0.
		 */
		memset(va, 0, buf->buf_size);
		buf->buf_list[i].va = va;
	}

	if (i != buf->buf_number) {
		cqm_buf_free_page_common(buf);
		return CQM_FAIL;
	}

	return CQM_SUCCESS;
}

static const struct malloc_memory g_malloc_funcs[] = {
	{.check_alloc_mode = check_use_vram,
	 .malloc_func = cqm_buf_vram_kalloc},
	{.check_alloc_mode = check_for_use_node_alloc,
	 .malloc_func = cqm_buf_use_node_alloc_page},
	{.check_alloc_mode = check_for_nouse_node_alloc,
	 .malloc_func = cqm_buf_unused_node_alloc_page}
};

static const struct free_memory g_free_funcs[] = {
	{.check_alloc_mode = check_use_vram, .free_func = cqm_buf_vram_free},
	{.check_alloc_mode = check_use_non_vram, .free_func = cqm_buf_free_page_common}
};

static s32 cqm_buf_alloc_page(struct tag_cqm_handle *cqm_handle, struct tag_cqm_buf *buf)
{
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;
	u32 malloc_funcs_num = ARRAY_SIZE(g_malloc_funcs);
	u32 i;

	for (i = 0; i < malloc_funcs_num; i++) {
		if (g_malloc_funcs[i].check_alloc_mode &&
		    g_malloc_funcs[i].malloc_func &&
		    g_malloc_funcs[i].check_alloc_mode(handle, buf))
			return g_malloc_funcs[i].malloc_func(handle, buf);
	}

	cqm_err(handle->dev_hdl, "Unknown alloc mode\n");

	return CQM_FAIL;
}

static void cqm_buf_free_page(struct tag_cqm_buf *buf)
{
	u32 free_funcs_num = ARRAY_SIZE(g_free_funcs);
	u32 i;

	for (i = 0; i < free_funcs_num; i++) {
		if (g_free_funcs[i].check_alloc_mode &&
		    g_free_funcs[i].free_func &&
		    g_free_funcs[i].check_alloc_mode(NULL, buf))
			return g_free_funcs[i].free_func(buf);
	}
}

static s32 cqm_buf_alloc_map(struct tag_cqm_handle *cqm_handle, struct tag_cqm_buf *buf)
{
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;
	struct pci_dev *dev = cqm_handle->dev;
	void *va = NULL;
	s32 i;

	for (i = 0; i < (s32)buf->buf_number; i++) {
		va = buf->buf_list[i].va;
		buf->buf_list[i].pa = pci_map_single(dev, va, buf->buf_size,
						     PCI_DMA_BIDIRECTIONAL);
		if (pci_dma_mapping_error(dev, buf->buf_list[i].pa)) {
			cqm_err(handle->dev_hdl, CQM_MAP_FAIL(buf_list));
			break;
		}
	}

	if (i != buf->buf_number) {
		i--;
		for (; i >= 0; i--)
			pci_unmap_single(dev, buf->buf_list[i].pa,
					 buf->buf_size, PCI_DMA_BIDIRECTIONAL);
		return CQM_FAIL;
	}

	return CQM_SUCCESS;
}

static s32 cqm_buf_get_secure_mem_pages(struct tag_cqm_handle *cqm_handle, struct tag_cqm_buf *buf)
{
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;
	u32 i;

	for (i = 0; i < buf->buf_number; i++) {
		buf->buf_list[i].va =
		    cqm_get_secure_mem_pages(handle,
					     (u32)get_order(buf->buf_size),
					     &buf->buf_list[i].pa);
		if (!buf->buf_list[i].va) {
			cqm_err(handle->dev_hdl,
				CQM_ALLOC_FAIL(cqm_get_secure_mem_pages));
			break;
		}
	}

	if (i != buf->buf_number) {
		cqm_free_secure_mem_pages(handle, buf->buf_list[0].va,
					  (u32)get_order(buf->buf_size));
		return CQM_FAIL;
	}

	return CQM_SUCCESS;
}

/**
 * Prototype    : cqm_buf_alloc
 * Description  : Apply for buffer space and DMA mapping for the struct tag_cqm_buf
 *		  structure.
 * Input        : struct tag_cqm_buf *buf
 *		  struct pci_dev *dev
 *		  bool direct: Whether direct remapping is required
 * Output       : None
 * Return Value : s32
 * 1.Date         : 2015/4/15
 *   Modification : Created function
 */
s32 cqm_buf_alloc(struct tag_cqm_handle *cqm_handle, struct tag_cqm_buf *buf, bool direct)
{
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;
	struct pci_dev *dev = cqm_handle->dev;
	s32 i;
	s32 ret;

	/* Applying for the buffer list descriptor space */
	buf->buf_list = vmalloc(buf->buf_number * sizeof(struct tag_cqm_buf_list));
	if (!buf->buf_list)
		return CQM_FAIL;
	memset(buf->buf_list, 0, buf->buf_number * sizeof(struct tag_cqm_buf_list));

	/* Page for applying for each buffer */
	if (cqm_bat_entry_in_secure_mem((void *)handle, buf->bat_entry_type))
		ret = cqm_buf_get_secure_mem_pages(cqm_handle, buf);
	else
		ret = cqm_buf_alloc_page(cqm_handle, buf);

	if (ret == CQM_FAIL) {
		cqm_err(handle->dev_hdl, CQM_FUNCTION_FAIL(linux_cqm_buf_alloc_page));
		goto err1;
	}

	/* PCI mapping of the buffer */
	if (!cqm_bat_entry_in_secure_mem((void *)handle, buf->bat_entry_type)) {
		if (cqm_buf_alloc_map(cqm_handle, buf) == CQM_FAIL) {
			cqm_err(handle->dev_hdl, CQM_FUNCTION_FAIL(linux_cqm_buf_alloc_map));
			goto err2;
		}
	}

	/* direct remapping */
	if (cqm_buf_alloc_direct(cqm_handle, buf, direct) == CQM_FAIL) {
		cqm_err(handle->dev_hdl, CQM_FUNCTION_FAIL(cqm_buf_alloc_direct));
		goto err3;
	}

	return CQM_SUCCESS;

err3:
	if (!cqm_bat_entry_in_secure_mem((void *)handle, buf->bat_entry_type)) {
		for (i = 0; i < (s32)buf->buf_number; i++) {
			pci_unmap_single(dev, buf->buf_list[i].pa, buf->buf_size,
					 PCI_DMA_BIDIRECTIONAL);
		}
	}
err2:
	if (cqm_bat_entry_in_secure_mem((void *)handle, buf->bat_entry_type))
		cqm_free_secure_mem_pages(handle, buf->buf_list[0].va,
					  (u32)get_order(buf->buf_size));
	else
		cqm_buf_free_page(buf);
err1:
	vfree(buf->buf_list);
	buf->buf_list = NULL;
	return CQM_FAIL;
}

/**
 * Prototype    : cqm_buf_free
 * Description  : Release the buffer space and DMA mapping for the struct tag_cqm_buf
 *		  structure.
 * Input        : struct tag_cqm_buf *buf
 *		  struct pci_dev *dev
 *		  bool direct: Whether direct remapping is required
 * Output       : None
 * Return Value : void
 * 1.Date         : 2015/4/15
 *   Modification : Created function
 */
void cqm_buf_free(struct tag_cqm_buf *buf, struct tag_cqm_handle *cqm_handle)
{
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;
	struct pci_dev *dev = cqm_handle->dev;
	s32 i;

	if (buf->direct.va) {
		vunmap(buf->direct.va);
		buf->direct.va = NULL;
	}

	if (!buf->buf_list)
		return;

	if (cqm_bat_entry_in_secure_mem(handle, buf->bat_entry_type)) {
		cqm_free_secure_mem_pages(handle, buf->buf_list[0].va,
					  (u32)get_order(buf->buf_size));
		goto free;
	}

	for (i = 0; i < (s32)(buf->buf_number); i++) {
		if (buf->buf_list[i].va)
			pci_unmap_single(dev, buf->buf_list[i].pa,
					 buf->buf_size,
					 PCI_DMA_BIDIRECTIONAL);
	}
	cqm_buf_free_page(buf);

free:
	vfree(buf->buf_list);
	buf->buf_list = NULL;
}

static s32 cqm_cla_cache_invalid_cmd(struct tag_cqm_handle *cqm_handle,
				     struct tag_cqm_cmd_buf *buf_in,
				     struct tag_cqm_cla_cache_invalid_cmd *cmd)
{
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;
	struct tag_cqm_cla_cache_invalid_cmd *cla_cache_invalid_cmd = NULL;
	s32 ret;

	cla_cache_invalid_cmd = (struct tag_cqm_cla_cache_invalid_cmd *)(buf_in->buf);
	cla_cache_invalid_cmd->gpa_h = cmd->gpa_h;
	cla_cache_invalid_cmd->gpa_l = cmd->gpa_l;
	cla_cache_invalid_cmd->cache_size = cmd->cache_size;
	cla_cache_invalid_cmd->smf_id = cmd->smf_id;
	cla_cache_invalid_cmd->func_id = cmd->func_id;

	cqm_swab32((u8 *)cla_cache_invalid_cmd,
		   /* shift 2 bits by right to get length of dw(4B) */
		   (sizeof(struct tag_cqm_cla_cache_invalid_cmd) >> 2));

	/* Send the cmdq command. */
	ret = cqm_send_cmd_box((void *)(cqm_handle->ex_handle), CQM_MOD_CQM,
			       CQM_CMD_T_CLA_CACHE_INVALID, buf_in, NULL, NULL,
			       CQM_CMD_TIMEOUT, HINIC3_CHANNEL_DEFAULT);
	if (ret != CQM_SUCCESS) {
		cqm_err(handle->dev_hdl, CQM_FUNCTION_FAIL(cqm_send_cmd_box));
		cqm_err(handle->dev_hdl,
			"Cla cache invalid: cqm_send_cmd_box_ret=%d\n",
			ret);
		cqm_err(handle->dev_hdl,
			"Cla cache invalid: cla_cache_invalid_cmd: 0x%x 0x%x 0x%x\n",
			cmd->gpa_h, cmd->gpa_l, cmd->cache_size);
		return CQM_FAIL;
	}

	return CQM_SUCCESS;
}

s32 cqm_cla_cache_invalid(struct tag_cqm_handle *cqm_handle, dma_addr_t pa, u32 cache_size)
{
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;
	struct tag_cqm_cmd_buf *buf_in = NULL;
	struct hinic3_func_attr *func_attr = NULL;
	struct tag_cqm_bat_entry_vf2pf gpa = {0};
	struct tag_cqm_cla_cache_invalid_cmd cmd;
	u32 cla_gpa_h = 0;
	s32 ret = CQM_FAIL;
	u32 i;

	buf_in = cqm_cmd_alloc((void *)(cqm_handle->ex_handle));
	if (!buf_in)
		return CQM_FAIL;
	buf_in->size = sizeof(struct tag_cqm_cla_cache_invalid_cmd);

	gpa.cla_gpa_h = CQM_ADDR_HI(pa) & CQM_CHIP_GPA_HIMASK;

	/* On the SPU, the value of spu_en in the GPA address
	 * in the BAT is determined by the host ID and fun IDx.
	 */
	if (hinic3_host_id(cqm_handle->ex_handle) == CQM_SPU_HOST_ID) {
		func_attr = &cqm_handle->func_attribute;
		gpa.acs_spu_en = func_attr->func_global_idx & 0x1;
	} else {
		gpa.acs_spu_en = 0;
	}

	/* In non-fake mode, set func_id to 0xffff.
	 * Indicate the current func fake mode.
	 * The value of func_id is a fake func ID.
	 */
	if (cqm_handle->func_capability.fake_func_type == CQM_FAKE_FUNC_CHILD) {
		cmd.func_id = cqm_handle->func_attribute.func_global_idx;
		func_attr = &cqm_handle->parent_cqm_handle->func_attribute;
		gpa.fake_vf_en = 1;
		gpa.pf_id = func_attr->func_global_idx;
	} else {
		cmd.func_id = 0xffff;
	}

	memcpy(&cla_gpa_h, &gpa, sizeof(u32));

	/* Fill command and convert it to big endian */
	cmd.cache_size = cache_size;
	cmd.gpa_l = CQM_ADDR_LW(pa);
	cmd.gpa_h = cla_gpa_h;

	/* Mode 0 is hashed to 4 SMF engines (excluding PPF) by func ID. */
	if (cqm_handle->func_capability.lb_mode == CQM_LB_MODE_NORMAL ||
	    (cqm_handle->func_capability.lb_mode == CQM_LB_MODE_0 &&
	     cqm_handle->func_attribute.func_type != CQM_PPF)) {
		cmd.smf_id = cqm_funcid2smfid(cqm_handle);
		ret = cqm_cla_cache_invalid_cmd(cqm_handle, buf_in, &cmd);
	/* Mode 1/2 are allocated to 4 SMF engines by flow. Therefore,
	 * one function needs to be allocated to 4 SMF engines.
	 */
	/* The PPF in mode 0 needs to be configured on 4 engines,
	 * and the timer resources need to be shared by the 4 engines.
	 */
	} else if (cqm_handle->func_capability.lb_mode == CQM_LB_MODE_1 ||
		   cqm_handle->func_capability.lb_mode == CQM_LB_MODE_2 ||
		   (cqm_handle->func_capability.lb_mode == CQM_LB_MODE_0 &&
		    cqm_handle->func_attribute.func_type == CQM_PPF)) {
		for (i = 0; i < CQM_LB_SMF_MAX; i++) {
			/* The smf_pg stored currently enabled SMF engine. */
			if (cqm_handle->func_capability.smf_pg & (1U << i)) {
				cmd.smf_id = i;
				ret = cqm_cla_cache_invalid_cmd(cqm_handle,
								buf_in, &cmd);
				if (ret != CQM_SUCCESS)
					goto out;
			}
		}
	} else {
		cqm_err(handle->dev_hdl, "Cla cache invalid: unsupport lb mode=%u\n",
			cqm_handle->func_capability.lb_mode);
		ret = CQM_FAIL;
	}

out:
	cqm_cmd_free((void *)(cqm_handle->ex_handle), buf_in);
	return ret;
}

static void free_cache_inv(struct tag_cqm_handle *cqm_handle, struct tag_cqm_buf *buf,
			   s32 *inv_flag)
{
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;
	u32 order;
	s32 i;

	order = (u32)get_order(buf->buf_size);

	if (!handle->chip_present_flag)
		return;

	if (!buf->buf_list)
		return;

	for (i = 0; i < (s32)(buf->buf_number); i++) {
		if (!buf->buf_list[i].va)
			continue;

		if (*inv_flag != CQM_SUCCESS)
			continue;

		/* In the Pangea environment, if the cmdq times out,
		 * no subsequent message is sent.
		 */
		*inv_flag = cqm_cla_cache_invalid(cqm_handle, buf->buf_list[i].pa,
						  (u32)(PAGE_SIZE << order));
		if (*inv_flag != CQM_SUCCESS)
			cqm_err(handle->dev_hdl,
				"Buffer free: fail to invalid buf_list pa cache, inv_flag=%d\n",
				*inv_flag);
	}
}

void cqm_buf_free_cache_inv(struct tag_cqm_handle *cqm_handle, struct tag_cqm_buf *buf,
			    s32 *inv_flag)
{
	/* Send a command to the chip to kick out the cache. */
	free_cache_inv(cqm_handle, buf, inv_flag);

	/* Clear host resources */
	cqm_buf_free(buf, cqm_handle);
}

void cqm_byte_print(u32 *ptr, u32 len)
{
	u32 i;
	u32 len_num = len;

	len_num = (len_num >> 0x2);
	for (i = 0; i < len_num; i = i + 0x4) {
		cqm_dbg("%.8x %.8x %.8x %.8x\n", ptr[i], ptr[i + 1],
			ptr[i + 2],  /* index increases by 2 */
			ptr[i + 3]); /* index increases by 3 */
	}
}

#define bitmap_section

/**
 * Prototype    : cqm_single_bitmap_init
 * Description  : Initialize a bitmap.
 * Input        : struct tag_cqm_bitmap *bitmap
 * Output       : None
 * Return Value : s32
 * 1.Date         : 2015/9/9
 *   Modification : Created function
 */
static s32 cqm_single_bitmap_init(struct tag_cqm_bitmap *bitmap)
{
	u32 bit_number;

	spin_lock_init(&bitmap->lock);

	/* Max_num of the bitmap is 8-aligned and then
	 * shifted rightward by 3 bits to obtain the number of bytes required.
	 */
	bit_number = (ALIGN(bitmap->max_num, CQM_NUM_BIT_BYTE) >>
		      CQM_BYTE_BIT_SHIFT);
	if (bitmap->bitmap_info.use_vram != 0)
		bitmap->table = hi_vram_kalloc(bitmap->bitmap_info.buf_vram_name,
					       bit_number);
	else
		bitmap->table = vmalloc(bit_number);
	if (!bitmap->table)
		return CQM_FAIL;
	memset(bitmap->table, 0, bit_number);

	return CQM_SUCCESS;
}

static s32 cqm_bitmap_toe_init(struct tag_cqm_handle *cqm_handle)
{
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;
	struct tag_cqm_bitmap *bitmap = NULL;

	/* SRQC of TOE services is not managed through the CLA table,
	 * but the bitmap is required to manage SRQid.
	 */
	if (cqm_handle->service[CQM_SERVICE_T_TOE].valid) {
		bitmap = &cqm_handle->toe_own_capability.srqc_bitmap;
		bitmap->max_num =
		    cqm_handle->toe_own_capability.toe_srqc_number;
		bitmap->reserved_top = 0;
		bitmap->reserved_back = 0;
		bitmap->last = 0;
		if (bitmap->max_num == 0) {
			cqm_info(handle->dev_hdl,
				 "Bitmap init: toe_srqc_number=0, don't init bitmap\n");
			return CQM_SUCCESS;
		}

		if (cqm_single_bitmap_init(bitmap) != CQM_SUCCESS)
			return CQM_FAIL;
	}

	return CQM_SUCCESS;
}

static void cqm_bitmap_toe_uninit(struct tag_cqm_handle *cqm_handle)
{
	struct tag_cqm_bitmap *bitmap = NULL;

	if (cqm_handle->service[CQM_SERVICE_T_TOE].valid) {
		bitmap = &cqm_handle->toe_own_capability.srqc_bitmap;
		if (bitmap->table) {
			spin_lock_deinit(&bitmap->lock);
			vfree(bitmap->table);
			bitmap->table = NULL;
		}
	}
}

/**
 * Prototype    : cqm_bitmap_init
 * Description  : Initialize the bitmap.
 * Input        : struct tag_cqm_handle *cqm_handle
 * Output       : None
 * Return Value : s32
 * 1.Date         : 2015/4/15
 *   Modification : Created function
 */
s32 cqm_bitmap_init(struct tag_cqm_handle *cqm_handle)
{
	struct tag_cqm_func_capability *capability = &cqm_handle->func_capability;
	struct tag_cqm_bat_table *bat_table = &cqm_handle->bat_table;
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;
	struct tag_cqm_cla_table *cla_table = NULL;
	struct tag_cqm_bitmap *bitmap = NULL;
	s32 ret = CQM_SUCCESS;
	u32 i;

	for (i = 0; i < CQM_BAT_ENTRY_MAX; i++) {
		cla_table = &bat_table->entry[i];
		if (cla_table->obj_num == 0) {
			cqm_info(handle->dev_hdl,
				 "Cla alloc: cla_type %u, obj_num=0, don't init bitmap\n",
				 cla_table->type);
			continue;
		}

		bitmap = &cla_table->bitmap;
		snprintf(bitmap->bitmap_info.buf_vram_name, VRAM_NAME_MAX_LEN,
			 "%s%s%02d", cla_table->name,
			 VRAM_CQM_BITMAP_BASE, cla_table->type);

		switch (cla_table->type) {
		case CQM_BAT_ENTRY_T_QPC:
			bitmap->max_num = capability->qpc_number;
			bitmap->reserved_top = capability->qpc_reserved;
			bitmap->reserved_back = capability->qpc_reserved_back;
			bitmap->last = capability->qpc_reserved;
			bitmap->bitmap_info.use_vram = get_use_vram_flag();
			cqm_info(handle->dev_hdl,
				 "Bitmap init: cla_table_type=%u, max_num=0x%x\n",
				 cla_table->type, bitmap->max_num);
			ret = cqm_single_bitmap_init(bitmap);
			break;
		case CQM_BAT_ENTRY_T_MPT:
			bitmap->max_num = capability->mpt_number;
			bitmap->reserved_top = capability->mpt_reserved;
			bitmap->reserved_back = 0;
			bitmap->last = capability->mpt_reserved;
			cqm_info(handle->dev_hdl,
				 "Bitmap init: cla_table_type=%u, max_num=0x%x\n",
				 cla_table->type, bitmap->max_num);
			ret = cqm_single_bitmap_init(bitmap);
			break;
		case CQM_BAT_ENTRY_T_SCQC:
			bitmap->max_num = capability->scqc_number;
			bitmap->reserved_top = capability->scq_reserved;
			bitmap->reserved_back = 0;
			bitmap->last = capability->scq_reserved;
			cqm_info(handle->dev_hdl,
				 "Bitmap init: cla_table_type=%u, max_num=0x%x\n",
				 cla_table->type, bitmap->max_num);
			ret = cqm_single_bitmap_init(bitmap);
			break;
		case CQM_BAT_ENTRY_T_SRQC:
			bitmap->max_num = capability->srqc_number;
			bitmap->reserved_top = capability->srq_reserved;
			bitmap->reserved_back = 0;
			bitmap->last = capability->srq_reserved;
			cqm_info(handle->dev_hdl,
				 "Bitmap init: cla_table_type=%u, max_num=0x%x\n",
				 cla_table->type, bitmap->max_num);
			ret = cqm_single_bitmap_init(bitmap);
			break;
		default:
			break;
		}

		if (ret != CQM_SUCCESS) {
			cqm_err(handle->dev_hdl,
				"Bitmap init: failed to init cla_table_type=%u, obj_num=0x%x\n",
				cla_table->type, cla_table->obj_num);
			goto err;
		}
	}

	if (cqm_bitmap_toe_init(cqm_handle) != CQM_SUCCESS)
		goto err;

	return CQM_SUCCESS;

err:
	cqm_bitmap_uninit(cqm_handle);
	return CQM_FAIL;
}

/**
 * Prototype    : cqm_bitmap_uninit
 * Description  : Deinitialize the bitmap.
 * Input        : struct tag_cqm_handle *cqm_handle
 * Output       : None
 * Return Value : void
 * 1.Date         : 2015/4/15
 *   Modification : Created function
 */
void cqm_bitmap_uninit(struct tag_cqm_handle *cqm_handle)
{
	struct tag_cqm_bat_table *bat_table = &cqm_handle->bat_table;
	struct tag_cqm_cla_table *cla_table = NULL;
	struct tag_cqm_bitmap *bitmap = NULL;
	u32 i;

	for (i = 0; i < CQM_BAT_ENTRY_MAX; i++) {
		cla_table = &bat_table->entry[i];
		bitmap = &cla_table->bitmap;
		if (cla_table->type != CQM_BAT_ENTRY_T_INVALID &&
		    bitmap->table) {
			spin_lock_deinit(&bitmap->lock);
			if (bitmap->bitmap_info.use_vram != 0)
				hi_vram_kfree(bitmap->table,
					      bitmap->bitmap_info.buf_vram_name,
					      ALIGN(bitmap->max_num,
						    CQM_NUM_BIT_BYTE) >>
						    CQM_BYTE_BIT_SHIFT);
			else
				vfree(bitmap->table);
			bitmap->table = NULL;
		}
	}

	cqm_bitmap_toe_uninit(cqm_handle);
}

/**
 * Prototype	: cqm_bitmap_check_range
 * Description	: Starting from begin, check whether the bits in number of count
 *		  are idle in the table. Requirement:
 *		  1. This group of bits cannot cross steps.
 *		  2. This group of bits must be 0.
 * Input	: const ulong *table,
 *		  u32 step,
 *		  u32 max_num,
 *		  u32 begin,
 *		  u32 count
 * Output	: None
 * Return Value : u32
 * 1.Date	  : 2015/4/15
 *   Modification : Created function
 */
static u32 cqm_bitmap_check_range(const ulong *table, u32 step, u32 max_num, u32 begin,
				  u32 count)
{
	u32 end = (begin + (count - 1));
	u32 i;

	/* Single-bit check is not performed. */
	if (count == 1)
		return begin;

	/* The end value exceeds the threshold. */
	if (end >= max_num)
		return max_num;

	/* Bit check, the next bit is returned when a non-zero bit is found. */
	for (i = (begin + 1); i <= end; i++) {
		if (test_bit((int)i, table))
			return i + 1;
	}

	/* Check whether it's in different steps. */
	if ((begin & (~(step - 1))) != (end & (~(step - 1))))
		return (end & (~(step - 1)));

	/* If the check succeeds, begin is returned. */
	return begin;
}

static void cqm_bitmap_find(struct tag_cqm_bitmap *bitmap, u32 *index, u32 last,
			    u32 step, u32 count)
{
	u32 last_num = last;
	u32 max_num = bitmap->max_num - bitmap->reserved_back;
	ulong *table = bitmap->table;

	do {
		*index = (u32)find_next_zero_bit(table, max_num, last_num);
		if (*index < max_num)
			last_num = cqm_bitmap_check_range(table, step, max_num,
							  *index, count);
		else
			break;
	} while (last_num != *index);
}

static void cqm_bitmap_find_with_low2bit_align(struct tag_cqm_bitmap *bitmap, u32 *index,
					       u32 max_num, u32 last, u32 low2bit)
{
	ulong *table = bitmap->table;
	u32 offset = last;

	while (offset < max_num) {
		*index = (u32)find_next_zero_bit(table, max_num, offset);
		if (*index >= max_num)
			break;

		if ((*index & 0x3) == (low2bit & 0x3)) /* 0x3 used for low2bit align */
			break;

		offset = *index + 1;
		if (offset == max_num)
			*index = max_num;
	}
}

/**
 * Prototype	: cqm_bitmap_alloc
 * Description	: Apply for a bitmap index. 0 and 1 must be left blank.
 *		  Scan backwards from where you last applied.
 *		  A string of consecutive indexes must be applied for and
 *		  cannot be applied for across trunks.
 * Input	: struct tag_cqm_bitmap *bitmap,
 *		  u32 step,
 *		  u32 count
 * Output	: None
 * Return Value : u32
 *		  The obtained index is returned.
 *		  If a failure occurs, the value of max is returned.
 * 1.Date	  : 2015/4/15
 *   Modification : Created function
 */
u32 cqm_bitmap_alloc(struct tag_cqm_bitmap *bitmap, u32 step, u32 count, bool update_last)
{
	u32 index = 0;
	u32 max_num = bitmap->max_num - bitmap->reserved_back;
	u32 last = bitmap->last;
	ulong *table = bitmap->table;
	u32 i;

	spin_lock(&bitmap->lock);

	/* Search for an idle bit from the last position. */
	cqm_bitmap_find(bitmap, &index, last, step, count);

	/* The preceding search fails. Search for an idle bit
	 * from the beginning.
	 */
	if (index >= max_num) {
		last = bitmap->reserved_top;
		cqm_bitmap_find(bitmap, &index, last, step, count);
	}

	/* Set the found bit to 1 and reset last. */
	if (index < max_num) {
		for (i = index; i < (index + count); i++)
			set_bit(i, table);

		if (update_last) {
			bitmap->last = (index + count);
			if (bitmap->last >= max_num)
				bitmap->last = bitmap->reserved_top;
		}
	}

	spin_unlock(&bitmap->lock);
	return index;
}

/**
 * Prototype	: cqm_bitmap_alloc_low2bit_align
 * Description	: Apply for a bitmap index with low2bit align. 0 and 1 must be left blank.
 *		  Scan backwards from where you last applied.
 *		  A string of consecutive indexes must be applied for and
 *		  cannot be applied for across trunks.
 * Input	: struct tag_cqm_bitmap *bitmap,
 *		  u32 low2bit,
 *		  bool update_last
 * Output	: None
 * Return Value : u32
 *		  The obtained index is returned.
 *		  If a failure occurs, the value of max is returned.
 * 1.Date	  : 2015/4/15
 *   Modification : Created function
 */
u32 cqm_bitmap_alloc_low2bit_align(struct tag_cqm_bitmap *bitmap, u32 low2bit, bool update_last)
{
	u32 index = 0;
	u32 max_num = bitmap->max_num - bitmap->reserved_back;
	u32 last = bitmap->last;
	ulong *table = bitmap->table;

	spin_lock(&bitmap->lock);

	/* Search for an idle bit from the last position. */
	cqm_bitmap_find_with_low2bit_align(bitmap, &index, max_num, last, low2bit);

	/* The preceding search fails. Search for an idle bit from the beginning. */
	if (index >= max_num) {
		last = bitmap->reserved_top;
		cqm_bitmap_find_with_low2bit_align(bitmap, &index, max_num, last, low2bit);
	}

	/* Set the found bit to 1 and reset last. */
	if (index < max_num) {
		set_bit(index, table);

		if (update_last) {
			bitmap->last = index;
			if (bitmap->last >= max_num)
				bitmap->last = bitmap->reserved_top;
		}
	}

	spin_unlock(&bitmap->lock);
	return index;
}

/**
 * Prototype	: cqm_bitmap_alloc_reserved
 * Description	: Reserve bit applied for based on index.
 * Input	: struct tag_cqm_bitmap *bitmap,
 *		  u32 count,
 *		  u32 index
 * Output	: None
 * Return Value : u32
 *		  The obtained index is returned.
 *		  If a failure occurs, the value of max is returned.
 * 1.Date	   : 2015/4/15
 *    Modification : Created function
 */
u32 cqm_bitmap_alloc_reserved(struct tag_cqm_bitmap *bitmap, u32 count, u32 index)
{
	ulong *table = bitmap->table;
	u32 ret_index;

	if (index >= bitmap->max_num || count != 1)
		return CQM_INDEX_INVALID;

	if (index >= bitmap->reserved_top && (index < bitmap->max_num - bitmap->reserved_back))
		return CQM_INDEX_INVALID;

	spin_lock(&bitmap->lock);

	if (test_bit((int)index, table)) {
		ret_index = CQM_INDEX_INVALID;
	} else {
		set_bit(index, table);
		ret_index = index;
	}

	spin_unlock(&bitmap->lock);
	return ret_index;
}

/**
 * Prototype	: cqm_bitmap_free
 * Description	: Releases a bitmap index.
 * Input	: struct tag_cqm_bitmap *bitmap,
 *		  u32 index,
 *		  u32 count
 * Output	: None
 * Return Value : void
 * 1.Date	  : 2015/4/15
 *   Modification : Created function
 */
void cqm_bitmap_free(struct tag_cqm_bitmap *bitmap, u32 index, u32 count)
{
	u32 i;

	spin_lock(&bitmap->lock);

	for (i = index; i < (index + count); i++)
		clear_bit((s32)i, bitmap->table);

	spin_unlock(&bitmap->lock);
}

#define obj_table_section

/**
 * Prototype    : cqm_single_object_table_init
 * Description  : Initialize a object table.
 * Input        : struct tag_cqm_object_table *obj_table
 * Output       : None
 * Return Value : s32
 * 1.Date         : 2015/9/9
 *   Modification : Created function
 */
static s32 cqm_single_object_table_init(struct tag_cqm_object_table *obj_table)
{
	rwlock_init(&obj_table->lock);

	obj_table->table = vmalloc(obj_table->max_num * sizeof(void *));
	if (!obj_table->table)
		return CQM_FAIL;
	memset(obj_table->table, 0, obj_table->max_num * sizeof(void *));
	return CQM_SUCCESS;
}

/**
 * Prototype    : cqm_object_table_init
 * Description  : Initialize the association table between objects and indexes.
 * Input        : struct tag_cqm_handle *cqm_handle
 * Output       : None
 * Return Value : s32
 * 1.Date         : 2015/4/15
 *   Modification : Created function
 */
s32 cqm_object_table_init(struct tag_cqm_handle *cqm_handle)
{
	struct tag_cqm_func_capability *capability = &cqm_handle->func_capability;
	struct tag_cqm_bat_table *bat_table = &cqm_handle->bat_table;
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;
	struct tag_cqm_object_table *obj_table = NULL;
	struct tag_cqm_cla_table *cla_table = NULL;
	s32 ret = CQM_SUCCESS;
	u32 i;

	for (i = 0; i < CQM_BAT_ENTRY_MAX; i++) {
		cla_table = &bat_table->entry[i];
		if (cla_table->obj_num == 0) {
			cqm_info(handle->dev_hdl,
				 "Obj table init: cla_table_type %u, obj_num=0, don't init obj table\n",
				 cla_table->type);
			continue;
		}

		obj_table = &cla_table->obj_table;

		switch (cla_table->type) {
		case CQM_BAT_ENTRY_T_QPC:
			obj_table->max_num = capability->qpc_number;
			ret = cqm_single_object_table_init(obj_table);
			break;
		case CQM_BAT_ENTRY_T_MPT:
			obj_table->max_num = capability->mpt_number;
			ret = cqm_single_object_table_init(obj_table);
			break;
		case CQM_BAT_ENTRY_T_SCQC:
			obj_table->max_num = capability->scqc_number;
			ret = cqm_single_object_table_init(obj_table);
			break;
		case CQM_BAT_ENTRY_T_SRQC:
			obj_table->max_num = capability->srqc_number;
			ret = cqm_single_object_table_init(obj_table);
			break;
		default:
			break;
		}

		if (ret != CQM_SUCCESS) {
			cqm_err(handle->dev_hdl,
				"Obj table init: failed to init cla_table_type=%u, obj_num=0x%x\n",
				cla_table->type, cla_table->obj_num);
			goto err;
		}
	}

	return CQM_SUCCESS;

err:
	cqm_object_table_uninit(cqm_handle);
	return CQM_FAIL;
}

/**
 * Prototype    : cqm_object_table_uninit
 * Description  : Deinitialize the association table between objects and
 *		  indexes.
 * Input        : struct tag_cqm_handle *cqm_handle
 * Output       : None
 * Return Value : void
 * 1.Date         : 2015/4/15
 *   Modification : Created function
 */
void cqm_object_table_uninit(struct tag_cqm_handle *cqm_handle)
{
	struct tag_cqm_bat_table *bat_table = &cqm_handle->bat_table;
	struct tag_cqm_object_table *obj_table = NULL;
	struct tag_cqm_cla_table *cla_table = NULL;
	u32 i;

	for (i = 0; i < CQM_BAT_ENTRY_MAX; i++) {
		cla_table = &bat_table->entry[i];
		obj_table = &cla_table->obj_table;
		if (cla_table->type != CQM_BAT_ENTRY_T_INVALID) {
			if (obj_table->table) {
				rwlock_deinit(&obj_table->lock);
				vfree(obj_table->table);
				obj_table->table = NULL;
			}
		}
	}
}

/**
 * Prototype    : cqm_object_table_insert
 * Description  : Insert an object
 * Input        : struct tag_cqm_handle *cqm_handle
 *		  struct tag_cqm_object_table *object_table
 *		  u32 index
 *		  struct tag_cqm_object *obj
 *		  bool bh
 * Output       : None
 * Return Value : s32
 * 1.Date         : 2015/4/15
 *   Modification : Created function
 */
s32 cqm_object_table_insert(struct tag_cqm_handle *cqm_handle,
			    struct tag_cqm_object_table *object_table,
			    u32 index, struct tag_cqm_object *obj, bool bh)
{
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;

	if (index >= object_table->max_num) {
		cqm_err(handle->dev_hdl,
			"Obj table insert: index 0x%x exceeds max_num 0x%x\n",
			index, object_table->max_num);
		return CQM_FAIL;
	}

	cqm_write_lock(&object_table->lock, bh);

	if (!object_table->table[index]) {
		object_table->table[index] = obj;
		cqm_write_unlock(&object_table->lock, bh);
		return CQM_SUCCESS;
	}

	cqm_write_unlock(&object_table->lock, bh);
	cqm_err(handle->dev_hdl,
		"Obj table insert: object_table->table[0x%x] has been inserted\n",
		index);

	return CQM_FAIL;
}

/**
 * Prototype    : cqm_object_table_remove
 * Description  : Remove an object
 * Input        : struct tag_cqm_handle *cqm_handle
 *		  struct tag_cqm_object_table *object_table
 *		  u32 index
 *		  const struct tag_cqm_object *obj
 *		  bool bh
 * Output       : None
 * Return Value : void
 * 1.Date         : 2015/4/15
 *   Modification : Created function
 */
void cqm_object_table_remove(struct tag_cqm_handle *cqm_handle,
			     struct tag_cqm_object_table *object_table,
			     u32 index, const struct tag_cqm_object *obj, bool bh)
{
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;

	if (index >= object_table->max_num) {
		cqm_err(handle->dev_hdl,
			"Obj table remove: index 0x%x exceeds max_num 0x%x\n",
			index, object_table->max_num);
		return;
	}

	cqm_write_lock(&object_table->lock, bh);

	if (object_table->table[index] && object_table->table[index] == obj)
		object_table->table[index] = NULL;
	else
		cqm_err(handle->dev_hdl,
			"Obj table remove: object_table->table[0x%x] has been removed\n",
			index);

	cqm_write_unlock(&object_table->lock, bh);
}

/**
 * Prototype    : cqm_object_table_get
 * Description  : Remove an object
 * Input        : struct tag_cqm_handle *cqm_handle
 *		  struct tag_cqm_object_table *object_table
 *		  u32 index
 *		  bool bh
 * Output       : None
 * Return Value : struct tag_cqm_object *obj
 * 1.Date         : 2018/6/20
 *   Modification : Created function
 */
struct tag_cqm_object *cqm_object_table_get(struct tag_cqm_handle *cqm_handle,
					    struct tag_cqm_object_table *object_table,
					    u32 index, bool bh)
{
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;
	struct tag_cqm_object *obj = NULL;

	if (index >= object_table->max_num) {
		cqm_err(handle->dev_hdl,
			"Obj table get: index 0x%x exceeds max_num 0x%x\n",
			index, object_table->max_num);
		return NULL;
	}

	cqm_read_lock(&object_table->lock, bh);

	obj = object_table->table[index];
	if (obj)
		atomic_inc(&obj->refcount);

	cqm_read_unlock(&object_table->lock, bh);

	return obj;
}

u32 cqm_bitmap_alloc_by_xid(struct tag_cqm_bitmap *bitmap, u32 count, u32 index)
{
	ulong *table = bitmap->table;
	u32 ret_index;

	if (index >= bitmap->max_num || count != 1)
		return CQM_INDEX_INVALID;

	spin_lock(&bitmap->lock);

	if (test_bit((int)index, table)) {
		ret_index = CQM_INDEX_INVALID;
	} else {
		set_bit(index, table);
		ret_index = index;
	}

	spin_unlock(&bitmap->lock);
	return ret_index;
}

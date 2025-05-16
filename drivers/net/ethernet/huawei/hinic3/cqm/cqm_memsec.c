// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "ossl_knl.h"
#include "hinic3_hw.h"
#include "hinic3_mt.h"
#include "hinic3_hwif.h"
#include "hinic3_hw_cfg.h"

#include "cqm_object.h"
#include "cqm_bitmap_table.h"
#include "cqm_bat_cla.h"
#include "cqm_bloomfilter.h"
#include "cqm_db.h"
#include "cqm_main.h"
#include "vmsec_mpu_common.h"
#include "cqm_memsec.h"

#define SECURE_VA_TO_IDX(va, base) (((va) - (base)) / PAGE_SIZE)
#define PCI_PROC_NAME_LEN 32
#define U8_BIT 8
#define MEM_SEC_PROC_DIR "driver/memsec"
#define BITS_TO_MB(bits) ((bits) * PAGE_SIZE / 1024 / 1024)
#define MEM_SEC_UNNECESSARY 1
#define MEMSEC_TMP_LEN 32
#define STD_INPUT_ONE_PARA 1
#define STD_INPUT_TWO_PARA 2
#define MR_KEY_2_INDEX_SHIFT 8

static int memsec_proc_show(struct seq_file *seq, void *offset);
static int memsec_proc_open(struct inode *inode, struct file *file);
static int memsec_proc_release(struct inode *inode, struct file *file);
static void memsec_info_print(struct seq_file *seq, struct tag_cqm_secure_mem *secure_mem);
static int hinic3_secure_mem_proc_ent_init(void *hwdev);
static void hinic3_secure_mem_proc_ent_deinit(void);
static int hinic3_secure_mem_proc_node_remove(void *hwdev);
static int hinic3_secure_mem_proc_node_add(void *hwdev);
static ssize_t memsec_proc_write(struct file *file, const char __user *data, size_t len,
				 loff_t *pff);

static struct proc_dir_entry *g_hinic3_memsec_proc_ent; /* proc dir */
static atomic_t g_memsec_proc_refcnt = ATOMIC_INIT(0);

static const struct proc_ops memsec_proc_fops = {
	.proc_open = memsec_proc_open,
	.proc_read = seq_read,
	.proc_write = memsec_proc_write,
	.proc_release = memsec_proc_release,
};

bool cqm_need_secure_mem(void *hwdev)
{
	struct tag_cqm_secure_mem *info = NULL;
	struct tag_cqm_handle *cqm_handle = NULL;
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)hwdev;

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	info = &cqm_handle->secure_mem;
	return ((info->need_secure_mem) && hinic3_is_guest_vmsec_enable(hwdev));
}
EXPORT_SYMBOL(cqm_need_secure_mem);

static int memsec_proc_open(struct inode *inode, struct file *file)
{
	struct hinic3_hwdev *handle = pde_data(inode);
	int ret;

	if (!try_module_get(THIS_MODULE))
		return -ENODEV;

	ret = single_open(file, memsec_proc_show, handle);
	if (ret)
		module_put(THIS_MODULE);

	return ret;
}

static int memsec_proc_release(struct inode *inode, struct file *file)
{
	module_put(THIS_MODULE);
	return single_release(inode, file);
}

static void memsec_info_print(struct seq_file *seq, struct tag_cqm_secure_mem *secure_mem)
{
	int i, j;

	seq_printf(seq, "Secure MemPageSize: %lu\n", PAGE_SIZE);
	seq_printf(seq, "Secure MemTotal: %u pages\n", secure_mem->bits_nr);
	seq_printf(seq, "Secure MemTotal: %lu MB\n", BITS_TO_MB(secure_mem->bits_nr));
	seq_printf(seq, "Secure MemUsed: %d pages\n",
		   bitmap_weight(secure_mem->bitmap, secure_mem->bits_nr));
	seq_printf(seq, "Secure MemAvailable: %d pages\n",
		   secure_mem->bits_nr - bitmap_weight(secure_mem->bitmap, secure_mem->bits_nr));
	seq_printf(seq, "Secure MemFirstAvailableIdx: %lu\n",
		   find_first_zero_bit(secure_mem->bitmap, secure_mem->bits_nr));
	seq_printf(seq, "Secure MemVirtualAddrStart: 0x%p\n", secure_mem->va_base);
	seq_printf(seq, "Secure MemVirtualAddrEnd: 0x%p\n", secure_mem->va_end);
	seq_printf(seq, "Secure MemPhysicalAddrStart: 0x%llx\n", secure_mem->pa_base);
	seq_printf(seq, "Secure MemPhysicalAddrEnd: 0x%llx\n",
		   secure_mem->pa_base + secure_mem->gpa_len0);
	seq_printf(seq, "Secure MemAllocCnt: %d\n", secure_mem->alloc_cnt);
	seq_printf(seq, "Secure MemFreeCnt: %d\n", secure_mem->free_cnt);
	seq_printf(seq, "Secure MemProcRefCnt: %d\n", atomic_read(&g_memsec_proc_refcnt));
	seq_puts(seq, "Secure MemBitmap:");

	for (i = 0, j = 0; i < (secure_mem->bits_nr / U8_BIT); i++) {
		if (i % U8_BIT == 0) {
			seq_printf(seq, "\n [%05d-%05d]: ", j, j + (U8_BIT * U8_BIT) - 0x1);
			j += U8_BIT * U8_BIT;
		}
		seq_printf(seq, "0x%x ", *(u8 *)((u8 *)secure_mem->bitmap + i));
	}

	seq_puts(seq, "\nSecure MemBitmap info end\n");
}

static struct tag_cqm_secure_mem *memsec_proc_get_secure_mem(struct hinic3_hwdev *handle)
{
	struct tag_cqm_handle *cqm_handle = NULL;
	struct tag_cqm_secure_mem *info = NULL;

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	if (!cqm_handle) {
		cqm_err(handle->dev_hdl, "[memsec]cqm not inited yet\n");
		return ERR_PTR(-EINVAL);
	}

	info = &cqm_handle->secure_mem;
	if (!info || !info->bitmap) {
		cqm_err(handle->dev_hdl, "[memsec]secure mem not inited yet\n");
		return ERR_PTR(-EINVAL);
	}

	return info;
}

static int memsec_proc_show(struct seq_file *seq, void *offset)
{
	struct hinic3_hwdev *handle = seq->private;
	struct tag_cqm_secure_mem *info = NULL;

	info = memsec_proc_get_secure_mem(handle);
	if (IS_ERR(info))
		return -EINVAL;

	memsec_info_print(seq, info);

	return 0;
}

static int test_read_secure_mem(struct hinic3_hwdev *handle, char *data, size_t len)
{
	u64 mem_ptr;
	struct tag_cqm_handle *cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	struct tag_cqm_secure_mem *info = &cqm_handle->secure_mem;

	if (sscanf(data, "r %llx", &mem_ptr) != STD_INPUT_ONE_PARA) {
		cqm_err(handle->dev_hdl, "[memsec_dfx] read info format unknown!\n");
		return -EINVAL;
	}

	if (mem_ptr < (u64)(info->va_base) || mem_ptr >= (u64)(info->va_end)) {
		cqm_err(handle->dev_hdl, "[memsec_dfx] addr 0x%llx invalid!\n", mem_ptr);
		return -EINVAL;
	}

	cqm_info(handle->dev_hdl, "[memsec_dfx] read addr 0x%llx val 0x%llx\n",
		 mem_ptr, *(u64 *)mem_ptr);
	return 0;
}

static int test_write_secure_mem(struct hinic3_hwdev *handle, char *data, size_t len)
{
	u64 mem_ptr;
	u64 val;
	struct tag_cqm_handle *cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	struct tag_cqm_secure_mem *info = &cqm_handle->secure_mem;

	if (sscanf(data, "w %llx %llx", &mem_ptr, &val) != STD_INPUT_TWO_PARA) {
		cqm_err(handle->dev_hdl, "[memsec_dfx] read info format unknown!\n");
		return -EINVAL;
	}

	if (mem_ptr < (u64)(info->va_base) || mem_ptr >= (u64)(info->va_end)) {
		cqm_err(handle->dev_hdl, "[memsec_dfx] addr 0x%llx invalid!\n", mem_ptr);
		return -EINVAL;
	}

	*(u64 *)mem_ptr = val;

	cqm_info(handle->dev_hdl, "[memsec_dfx] write addr 0x%llx val 0x%llx now val 0x%llx\n",
		 mem_ptr, val, *(u64 *)mem_ptr);
	return 0;
}

static void test_query_usage(struct hinic3_hwdev *handle)
{
	cqm_info(handle->dev_hdl, "\t[memsec_dfx]Usage: q <query_type> <index>\n");
	cqm_info(handle->dev_hdl, "\t[memsec_dfx]Check whether roce context is in secure memory\n");
	cqm_info(handle->dev_hdl, "\t[memsec_dfx]Options:\n");
	cqm_info(handle->dev_hdl, "\t[memsec_dfx]query_type: qpc, mpt, srqc, scqc\n");
	cqm_info(handle->dev_hdl, "\t[memsec_dfx]index: valid index.e.g. 0x3\n");
}

static int test_query_parse_type(struct hinic3_hwdev *handle, char *data,
				 enum cqm_object_type *type, u32 *index)
{
	char query_type[MEMSEC_TMP_LEN] = {'\0'};

	if (sscanf(data, "q %s %x", query_type, index) != STD_INPUT_TWO_PARA) {
		cqm_err(handle->dev_hdl, "[memsec_dfx] parse query cmd fail!\n");
		return -1;
	}
	query_type[MEMSEC_TMP_LEN - 1] = '\0';

	if (*index <= 0) {
		cqm_err(handle->dev_hdl, "[memsec_dfx] query index 0x%x is invalid\n", *index);
		return -1;
	}

	if (strcmp(query_type, "qpc") == 0) {
		*type = CQM_OBJECT_SERVICE_CTX;
	} else if (strcmp(query_type, "mpt") == 0) {
		*type = CQM_OBJECT_MPT;
		*index = (*index >> MR_KEY_2_INDEX_SHIFT) & 0xFFFFFF;
	} else if (strcmp(query_type, "srqc") == 0) {
		*type = CQM_OBJECT_RDMA_SRQ;
	} else if (strcmp(query_type, "scqc") == 0) {
		*type = CQM_OBJECT_RDMA_SCQ;
	} else {
		cqm_err(handle->dev_hdl, "[memsec_dfx] query type is invalid\n");
		return -1;
	}

	return 0;
}

static int test_query_context(struct hinic3_hwdev *handle, char *data, size_t len)
{
	int ret;
	u32 index = 0;
	bool in_secmem = false;
	struct tag_cqm_object *cqm_obj = NULL;
	struct tag_cqm_qpc_mpt *qpc_mpt = NULL;
	struct tag_cqm_queue *cqm_queue = NULL;
	struct tag_cqm_secure_mem *info = NULL;
	enum cqm_object_type query_type;

	ret = test_query_parse_type(handle, data, &query_type, &index);
	if (ret < 0) {
		test_query_usage(handle);
		return -EINVAL;
	}

	info = memsec_proc_get_secure_mem(handle);
	if (IS_ERR(info))
		return -EINVAL;

	cqm_obj = cqm_object_get((void *)handle, query_type, index, false);
	if (!cqm_obj) {
		cqm_err(handle->dev_hdl, "[memsec_dfx] get cmq obj fail!\n");
		return -EINVAL;
	}

	switch (query_type) {
	case CQM_OBJECT_SERVICE_CTX:
	case CQM_OBJECT_MPT:
		qpc_mpt = (struct tag_cqm_qpc_mpt *)cqm_obj;
		if (qpc_mpt->vaddr >= (u8 *)info->va_base &&
		    (qpc_mpt->vaddr + cqm_obj->object_size) < (u8 *)info->va_end)
			in_secmem = true;
		cqm_info(handle->dev_hdl,
			 "[memsec_dfx]Query %s:0x%x, va=%p %sin secure mem\n",
			 query_type == CQM_OBJECT_MPT ? "MPT, mpt_index" : "QPC, qpn",
			 index, qpc_mpt->vaddr, in_secmem ? "" : "not ");
		break;
	case CQM_OBJECT_RDMA_SRQ:
	case CQM_OBJECT_RDMA_SCQ:
		cqm_queue = (struct tag_cqm_queue *)cqm_obj;
		if (cqm_queue->q_ctx_vaddr >= (u8 *)info->va_base &&
		    (cqm_queue->q_ctx_vaddr + cqm_obj->object_size) < (u8 *)info->va_end)
			in_secmem = true;
		cqm_info(handle->dev_hdl,
			 "[memsec_dfx]Query %s:0x%x, va=%p %sin secure mem\n",
			 query_type == CQM_OBJECT_RDMA_SRQ ? "SRQC, srqn " : "SCQC, scqn",
			 index, cqm_queue->q_ctx_vaddr, in_secmem ? "" : "not ");
		break;
	default:
		cqm_err(handle->dev_hdl, "[memsec_dfx] not support query type!\n");
		break;
	}

	cqm_object_put(cqm_obj);
	return 0;
}

static ssize_t memsec_proc_write(struct file *file, const char __user *data,
				 size_t len, loff_t *off)
{
	int ret = -EINVAL;
	struct hinic3_hwdev *handle = pde_data(file->f_inode);
	char tmp[MEMSEC_TMP_LEN] = {0};

	if (!handle)
		return -EIO;

	if (len >= MEMSEC_TMP_LEN)
		return -EFBIG;

	if (copy_from_user(tmp, data, len))
		return -EIO;

	switch (tmp[0]) {
	case 'r':
		ret = test_read_secure_mem(handle, tmp, len);
		break;
	case 'w':
		ret = test_write_secure_mem(handle, tmp, len);
		break;
	case 'q':
		ret = test_query_context(handle, tmp, len);
		break;
	default:
		cqm_err(handle->dev_hdl, "[memsec_dfx] not support cmd!\n");
	}

	return (ret == 0) ? len : ret;
}

static int hinic3_secure_mem_proc_ent_init(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;

	if (g_hinic3_memsec_proc_ent)
		return 0;

	g_hinic3_memsec_proc_ent = proc_mkdir(MEM_SEC_PROC_DIR, NULL);
	if (!g_hinic3_memsec_proc_ent) {
		/* try again */
		remove_proc_entry(MEM_SEC_PROC_DIR, NULL);
		g_hinic3_memsec_proc_ent = proc_mkdir(MEM_SEC_PROC_DIR, NULL);
		if (!g_hinic3_memsec_proc_ent) {
			cqm_err(dev->dev_hdl, "[memsec]create secure mem proc fail!\n");
			return -EINVAL;
		}
	}

	return 0;
}

static void hinic3_secure_mem_proc_ent_deinit(void)
{
	if (g_hinic3_memsec_proc_ent && !atomic_read(&g_memsec_proc_refcnt)) {
		remove_proc_entry(MEM_SEC_PROC_DIR, NULL);
		g_hinic3_memsec_proc_ent = NULL;
	}
}

static int hinic3_secure_mem_proc_node_remove(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;
	struct pci_dev *pdev = dev->pcidev_hdl;
	char pci_name[PCI_PROC_NAME_LEN] = {0};

	if (!g_hinic3_memsec_proc_ent) {
		sdk_info(dev->dev_hdl, "[memsec]proc_ent_null!\n");
		return 0;
	}

	atomic_dec(&g_memsec_proc_refcnt);

	snprintf(pci_name, PCI_PROC_NAME_LEN,
		 "%02x:%02x:%x", pdev->bus->number, pdev->slot->number,
		 PCI_FUNC(pdev->devfn));

	remove_proc_entry(pci_name, g_hinic3_memsec_proc_ent);

	return 0;
}

static int hinic3_secure_mem_proc_node_add(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;
	struct pci_dev *pdev = dev->pcidev_hdl;
	struct proc_dir_entry *res = NULL;
	char pci_name[PCI_PROC_NAME_LEN] = {0};

	if (!g_hinic3_memsec_proc_ent) {
		cqm_err(dev->dev_hdl, "[memsec]proc_ent_null!\n");
		return -EINVAL;
	}

	atomic_inc(&g_memsec_proc_refcnt);

	snprintf(pci_name, PCI_PROC_NAME_LEN,
		 "%02x:%02x:%x", pdev->bus->number, pdev->slot->number,
		 PCI_FUNC(pdev->devfn));
	/* 0400 Read by owner */
	res = proc_create_data(pci_name, 0400, g_hinic3_memsec_proc_ent, &memsec_proc_fops,
			       hwdev);
	if (!res) {
		cqm_err(dev->dev_hdl, "[memsec]proc_create_data fail!\n");
		return -ENOMEM;
	}

	return 0;
}

void hinic3_memsec_proc_init(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;
	int ret;

	ret = hinic3_secure_mem_proc_ent_init(hwdev);
	if (ret != 0) {
		cqm_err(dev->dev_hdl, "[memsec]proc ent init fail!\n");
		return;
	}

	ret = hinic3_secure_mem_proc_node_add(hwdev);
	if (ret != 0) {
		cqm_err(dev->dev_hdl, "[memsec]proc node add fail!\n");
		return;
	}
}

void hinic3_memsec_proc_deinit(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;
	int ret;

	if (!cqm_need_secure_mem(hwdev))
		return;

	ret = hinic3_secure_mem_proc_node_remove(hwdev);
	if (ret != 0) {
		cqm_err(dev->dev_hdl, "[memsec]proc node remove fail!\n");
		return;
	}

	hinic3_secure_mem_proc_ent_deinit();
}

static int cqm_get_secure_mem_cfg(void *dev, struct tag_cqm_secure_mem *info)
{
	struct hinic3_hwdev *hwdev = (struct hinic3_hwdev *)dev;
	struct vmsec_cfg_ctx_gpa_entry_cmd mem_info;
	u16 out_size = sizeof(struct vmsec_cfg_ctx_gpa_entry_cmd);
	int err;

	if (!hwdev || !info)
		return -EINVAL;

	memset(&mem_info, 0, sizeof(mem_info));
	mem_info.entry.func_id = info->func_id;

	err = hinic3_msg_to_mgmt_sync(hwdev, HINIC3_MOD_VMSEC, VMSEC_MPU_CMD_CTX_GPA_SHOW,
				      &mem_info, sizeof(mem_info), &mem_info,
				      &out_size, 0, HINIC3_CHANNEL_COMM);
	if (err || !out_size || mem_info.head.status) {
		cqm_err(hwdev->dev_hdl, "failed to get memsec info, err: %d, status: 0x%x, out size: 0x%x\n",
			err, mem_info.head.status, out_size);
		return -EINVAL;
	}

	info->gpa_len0 = mem_info.entry.gpa_len0;
	info->mode = mem_info.entry.mode;
	info->pa_base = (u64)((((u64)mem_info.entry.gpa_addr0_hi) << CQM_INT_ADDR_SHIFT) |
				  mem_info.entry.gpa_addr0_lo);

	return 0;
}

static int cqm_secure_mem_param_check(void *ex_handle, struct tag_cqm_secure_mem *info)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;

	if (!info->pa_base || !info->gpa_len0)
		goto no_need_secure_mem;

	if (!IS_ALIGNED(info->pa_base, CQM_SECURE_MEM_ALIGNED_SIZE) ||
	    !IS_ALIGNED(info->gpa_len0, CQM_SECURE_MEM_ALIGNED_SIZE)) {
		cqm_err(handle->dev_hdl, "func_id %u secure mem not 2M aligned\n",
			info->func_id);
		return -EINVAL;
	}

	if (info->mode == VM_GPA_INFO_MODE_NMIG)
		goto no_need_secure_mem;

	return 0;

no_need_secure_mem:
	cqm_info(handle->dev_hdl, "func_id %u no need secure mem gpa 0x%llx len0 0x%x mode 0x%x\n",
		 info->func_id, info->pa_base, info->gpa_len0, info->mode);
	info->need_secure_mem = false;
	return MEM_SEC_UNNECESSARY;
}

int cqm_secure_mem_init(void *ex_handle)
{
	int err;
	struct tag_cqm_secure_mem *info = NULL;
	struct tag_cqm_handle *cqm_handle = NULL;
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;

	if (!handle)
		return -EINVAL;

	// only vf in vm need secure mem
	if (!hinic3_is_guest_vmsec_enable(ex_handle)) {
		cqm_info(handle->dev_hdl, "no need secure mem\n");
		return 0;
	}

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	info = &cqm_handle->secure_mem;
	info->func_id = hinic3_global_func_id(ex_handle);

	// get gpa info from mpu
	err = cqm_get_secure_mem_cfg(ex_handle, info);
	if (err) {
		cqm_err(handle->dev_hdl, "func_id %u get secure mem failed, ret %d\n",
			info->func_id, err);
			return err;
	}

	// remap gpa
	err = cqm_secure_mem_param_check(ex_handle, info);
	if (err) {
		cqm_info(handle->dev_hdl, "func_id %u cqm_secure_mem_param_check failed\n",
			 info->func_id);
		return (err == MEM_SEC_UNNECESSARY) ? 0 : err;
	}

	info->va_base = ioremap(info->pa_base, info->gpa_len0);
	info->va_end = info->va_base + info->gpa_len0;
	info->page_num = info->gpa_len0 / PAGE_SIZE;
	info->need_secure_mem = true;
	info->bits_nr = info->page_num;
	info->bitmap = bitmap_zalloc(info->bits_nr, GFP_KERNEL);
	if (!info->bitmap) {
		cqm_err(handle->dev_hdl, "func_id %u bitmap_zalloc failed\n",
			info->func_id);
		iounmap(info->va_base);
		return -ENOMEM;
	}

	hinic3_memsec_proc_init(ex_handle);
	return err;
}

int cqm_secure_mem_deinit(void *ex_handle)
{
	struct tag_cqm_secure_mem *info = NULL;
	struct tag_cqm_handle *cqm_handle = NULL;
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;

	if (!handle)
		return -EINVAL;

	// only vf in vm need secure mem
	if (!cqm_need_secure_mem(ex_handle))
		return 0;

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	info = &cqm_handle->secure_mem;

	if (info && info->va_base)
		iounmap(info->va_base);

	if (info && info->bitmap)
		kfree(info->bitmap);

	hinic3_memsec_proc_deinit(ex_handle);
	return 0;
}

void *cqm_get_secure_mem_pages(struct hinic3_hwdev *handle, u32 order, dma_addr_t *pa_base)
{
	struct tag_cqm_secure_mem *info = NULL;
	struct tag_cqm_handle *cqm_handle = NULL;
	unsigned int nr;
	unsigned long *bitmap = NULL;
	unsigned long index;
	unsigned long flags;

	if (!handle || !(handle->cqm_hdl)) {
		pr_err("[memsec]%s null pointer\n", __func__);
		return NULL;
	}

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	info = &cqm_handle->secure_mem;
	bitmap = info->bitmap;
	nr = 1 << order;

	if (!bitmap) {
		cqm_err(handle->dev_hdl, "[memsec] %s bitmap null\n", __func__);
		return NULL;
	}

	spin_lock_irqsave(&info->bitmap_lock, flags);

	index = (order) ? bitmap_find_next_zero_area(bitmap, info->bits_nr, 0, nr, 0) :
		find_first_zero_bit(bitmap, info->bits_nr);
	if (index >= info->bits_nr) {
		spin_unlock_irqrestore(&info->bitmap_lock, flags);
		cqm_err(handle->dev_hdl,
			"can not find continuous memory, size %d pages, weight %d\n",
			nr, bitmap_weight(bitmap, info->bits_nr));
		return NULL;
	}

	bitmap_set(bitmap, index, nr);
	info->alloc_cnt++;
	spin_unlock_irqrestore(&info->bitmap_lock, flags);

	*pa_base = info->pa_base + index * PAGE_SIZE;
	return (void *)(info->va_base + index * PAGE_SIZE);
}

void cqm_free_secure_mem_pages(struct hinic3_hwdev *handle, void *va, u32 order)
{
	struct tag_cqm_secure_mem *info = NULL;
	struct tag_cqm_handle *cqm_handle = NULL;
	unsigned int nr;
	unsigned long *bitmap = NULL;
	unsigned long index;
	unsigned long flags;

	if (!handle || !(handle->cqm_hdl)) {
		pr_err("%s null pointer\n", __func__);
		return;
	}

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	info = &cqm_handle->secure_mem;
	bitmap = info->bitmap;
	nr = 1UL << order;

	if (!bitmap) {
		cqm_err(handle->dev_hdl, "%s bitmap null\n", __func__);
		return;
	}

	if (va < info->va_base || va > (info->va_end - PAGE_SIZE) ||
	    !PAGE_ALIGNED((va - info->va_base)))
		cqm_err(handle->dev_hdl, "%s va wrong value\n", __func__);

	index = SECURE_VA_TO_IDX(va, info->va_base);
	spin_lock_irqsave(&info->bitmap_lock, flags);
	bitmap_clear(bitmap, index, nr);
	info->free_cnt++;
	spin_unlock_irqrestore(&info->bitmap_lock, flags);
}

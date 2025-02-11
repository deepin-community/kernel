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
#include <asm/cpuid.h>
#include <linux/bsearch.h>
#include <linux/sort.h>
#include <linux/bitfield.h>

#include "ring-buffer.h"
#include "psp-dev.h"
#include "csv-dev.h"
#include "vpsp.h"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "vpsp: " fmt
#define VTKM_VM_BIND	0x904

/*
 * The file mainly implements the base execution logic of virtual PSP in kernel mode,
 *	which mainly includes:
 *	(1) Preprocess the guest data in the host kernel
 *	(2) The command that has been converted will interact with the channel of the
 *		psp through the driver and try to obtain the execution result
 *	(3) The executed command data is recovered, and then returned to the VM
 *
 * The primary implementation logic of virtual PSP in kernel mode
 * call trace:
 * guest command(vmmcall, KVM_HC_PSP_COPY_FORWARD_OP)
 *		   |
 *	kvm_pv_psp_copy_op---->	| -> kvm_pv_psp_cmd_pre_op
 *				|
 *				| -> vpsp_try_do_cmd/vpsp_try_get_result
 *				|	|<=> psp device driver
 *				|
 *				|
 *				|-> kvm_pv_psp_cmd_post_op
 *
 * guest command(vmmcall, KVM_HC_PSP_FORWARD_OP)
 *		   |
 *	kvm_pv_psp_forward_op-> |-> vpsp_try_do_cmd/vpsp_try_get_result
 *					|<=> psp device driver
 */

struct psp_cmdresp_head {
	uint32_t buf_size;
	uint32_t cmdresp_size;
	uint32_t cmdresp_code;
} __packed;

/* save command data for restoring later */
struct vpsp_hbuf_wrapper {
	void *data;
	uint32_t data_size;
};

/* Virtual PSP host memory information maintenance, used in ringbuffer mode */
struct vpsp_hbuf_wrapper
g_hbuf_wrap[CSV_COMMAND_PRIORITY_NUM][CSV_RING_BUFFER_SIZE / CSV_RING_BUFFER_ESIZE] = {0};

static int check_gpa_range(struct vpsp_context *vpsp_ctx, gpa_t addr, uint32_t size)
{
	if (!vpsp_ctx || !addr)
		return -EFAULT;

	if (addr >= vpsp_ctx->gpa_start && (addr + size) <= vpsp_ctx->gpa_end)
		return 0;
	return -EFAULT;
}

static int check_psp_mem_range(struct vpsp_context *vpsp_ctx,
			void *data, uint32_t size)
{
	if ((((uintptr_t)data + size - 1) & ~PSP_2MB_MASK) !=
			((uintptr_t)data & ~PSP_2MB_MASK)) {
		pr_err("data %llx, size %d crossing 2MB\n", (u64)data, size);
		return -EFAULT;
	}

	if (vpsp_ctx)
		return check_gpa_range(vpsp_ctx, (gpa_t)data, size);

	return 0;
}

/**
 * Copy the guest data to the host kernel buffer
 * and record the host buffer address in 'hbuf'.
 * This 'hbuf' is used to restore context information
 * during asynchronous processing.
 */
static int kvm_pv_psp_cmd_pre_op(struct kvm_vpsp *vpsp, gpa_t data_gpa,
		struct vpsp_hbuf_wrapper *hbuf)
{
	int ret = 0;
	void *data = NULL;
	struct psp_cmdresp_head psp_head;
	uint32_t data_size;

	if (unlikely(vpsp->read_guest(vpsp->kvm, data_gpa, &psp_head,
					sizeof(struct psp_cmdresp_head))))
		return -EFAULT;

	data_size = psp_head.buf_size;
	if (check_psp_mem_range(NULL, (void *)data_gpa, data_size))
		return -EFAULT;

	data = kzalloc(data_size, GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	if (unlikely(vpsp->read_guest(vpsp->kvm, data_gpa, data, data_size))) {
		ret = -EFAULT;
		goto end;
	}

	hbuf->data = data;
	hbuf->data_size = data_size;

end:
	return ret;
}

static int kvm_pv_psp_cmd_post_op(struct kvm_vpsp *vpsp, gpa_t data_gpa,
				struct vpsp_hbuf_wrapper *hbuf)
{
	int ret = 0;

	/* restore cmdresp's buffer from context */
	if (unlikely(vpsp->write_guest(vpsp->kvm, data_gpa, hbuf->data,
					hbuf->data_size))) {
		pr_err("[%s]: kvm_write_guest for cmdresp data failed\n",
			__func__);
		ret = -EFAULT;
		goto end;
	}
end:
	kfree(hbuf->data);
	memset(hbuf, 0, sizeof(*hbuf));
	return ret;
}

static int cmd_type_is_tkm(int cmd)
{
	if (cmd >= TKM_CMD_ID_MIN && cmd <= TKM_CMD_ID_MAX)
		return 1;
	return 0;
}

static int cmd_type_is_allowed(int cmd)
{
	if (cmd >= TKM_PSP_CMDID_OFFSET && cmd <= TKM_CMD_ID_MAX)
		return 1;
	return 0;
}

struct psp_cmdresp_vtkm_vm_bind {
	struct psp_cmdresp_head head;
	uint16_t vid;
	uint32_t vm_handle;
	uint8_t reserved[46];
} __packed;

static int kvm_bind_vtkm(uint32_t vm_handle, uint32_t cmd_id, uint32_t vid, uint32_t *pret)
{
	int ret = 0;
	struct psp_cmdresp_vtkm_vm_bind *data;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->head.buf_size = sizeof(*data);
	data->head.cmdresp_size = sizeof(*data);
	data->head.cmdresp_code = VTKM_VM_BIND;
	data->vid = vid;
	data->vm_handle = vm_handle;

	ret = psp_do_cmd(cmd_id, data, pret);
	if (ret == -EIO)
		ret = 0;

	kfree(data);
	return ret;
}

static unsigned long vpsp_get_me_mask(void)
{
	unsigned int eax, ebx, ecx, edx;
	unsigned long me_mask;

#define AMD_SME_BIT	BIT(0)
#define AMD_SEV_BIT	BIT(1)
	/*
	 * Check for the SME/SEV feature:
	 *   CPUID Fn8000_001F[EAX]
	 *   - Bit 0 - Secure Memory Encryption support
	 *   - Bit 1 - Secure Encrypted Virtualization support
	 *   CPUID Fn8000_001F[EBX]
	 *   - Bits 5:0 - Pagetable bit position used to indicate encryption
	 */
	eax = 0x8000001f;
	ecx = 0;
	native_cpuid(&eax, &ebx, &ecx, &edx);
	/* Check whether SEV or SME is supported */
	if (!(eax & (AMD_SEV_BIT | AMD_SME_BIT)))
		return 0;

	me_mask = 1UL << (ebx & 0x3f);
	return me_mask;
}

static phys_addr_t gpa_to_hpa(struct kvm_vpsp *vpsp, unsigned long data_gpa)
{
	phys_addr_t hpa = 0;
	unsigned long pfn = vpsp->gfn_to_pfn(vpsp->kvm, data_gpa >> PAGE_SHIFT);
	unsigned long me_mask = sme_get_me_mask();
	struct page *page;

	if (me_mask == 0 && vpsp->is_csv_guest)
		me_mask = vpsp_get_me_mask();

	if (!is_error_pfn(pfn))
		hpa = ((pfn << PAGE_SHIFT) + offset_in_page(data_gpa)) | me_mask;
	else {
		pr_err("[%s] pfn: %lx is invalid, gpa %lx",
				__func__, pfn, data_gpa);
		return 0;
	}

	/*
	 * Using gfn_to_pfn causes the refcount to increment
	 * atomically by one, which needs to be released.
	 */
	page = pfn_to_page(pfn);
	if (PageCompound(page))
		page = compound_head(page);

	put_page(page);

	pr_debug("gpa %lx, hpa %llx\n", data_gpa, hpa);
	return hpa;

}

static int check_cmd_forward_op_permission(struct kvm_vpsp *vpsp, struct vpsp_context *vpsp_ctx,
				uint64_t data, uint32_t cmd)
{
	int ret;
	struct vpsp_cmd *vcmd = (struct vpsp_cmd *)&cmd;
	struct psp_cmdresp_head psp_head;

	if (!cmd_type_is_allowed(vcmd->cmd_id)) {
		pr_err("[%s]: unsupported cmd id %x\n", __func__, vcmd->cmd_id);
		return -EINVAL;
	}

	if (vpsp->is_csv_guest) {
		/**
		 * If the gpa address range exists,
		 * it means there must be a legal vid
		 */
		if (!vpsp_ctx || !vpsp_ctx->gpa_start || !vpsp_ctx->gpa_end) {
			pr_err("[%s]: No set gpa range or vid in csv guest\n", __func__);
			return -EPERM;
		}

		ret = check_psp_mem_range(vpsp_ctx, (void *)data, 0);
		if (ret)
			return -EFAULT;
	} else {
		if (!vpsp_ctx && cmd_type_is_tkm(vcmd->cmd_id)
				&& !vpsp_get_default_vid_permission()) {
			pr_err("[%s]: not allowed tkm command without vid\n", __func__);
			return -EPERM;
		}

		// the 'data' is gpa address
		if (unlikely(vpsp->read_guest(vpsp->kvm, data, &psp_head,
					sizeof(struct psp_cmdresp_head))))
			return -EFAULT;

		ret = check_psp_mem_range(vpsp_ctx, (void *)data, psp_head.buf_size);
		if (ret)
			return -EFAULT;
	}
	return 0;
}

static int
check_cmd_copy_forward_op_permission(struct kvm_vpsp *vpsp,
				struct vpsp_context *vpsp_ctx,
				uint64_t data, uint32_t cmd)
{
	int ret = 0;
	struct vpsp_cmd *vcmd = (struct vpsp_cmd *)&cmd;

	if (!cmd_type_is_allowed(vcmd->cmd_id)) {
		pr_err("[%s]: unsupported cmd id %x\n", __func__, vcmd->cmd_id);
		return -EINVAL;
	}

	if (vpsp->is_csv_guest) {
		pr_err("[%s]: unsupported run on csv guest\n", __func__);
		ret = -EPERM;
	} else {
		if (!vpsp_ctx && cmd_type_is_tkm(vcmd->cmd_id)
				&& !vpsp_get_default_vid_permission()) {
			pr_err("[%s]: not allowed tkm command without vid\n", __func__);
			ret = -EPERM;
		}
	}
	return ret;
}

static int vpsp_try_bind_vtkm(struct kvm_vpsp *vpsp, struct vpsp_context *vpsp_ctx,
				uint32_t cmd, uint32_t *psp_ret)
{
	int ret;
	struct vpsp_cmd *vcmd = (struct vpsp_cmd *)&cmd;

	if (vpsp_ctx && !vpsp_ctx->vm_is_bound && vpsp->is_csv_guest) {
		ret = kvm_bind_vtkm(vpsp->vm_handle, vcmd->cmd_id,
					vpsp_ctx->vid, psp_ret);
		if (ret || *psp_ret) {
			pr_err("[%s] kvm bind vtkm failed with ret: %d, pspret: %d\n",
				__func__, ret, *psp_ret);
			return ret;
		}
		vpsp_ctx->vm_is_bound = 1;
	}
	return 0;
}

/**
 * @brief Directly convert the gpa address into hpa and forward it to PSP,
 *	  It is another form of kvm_pv_psp_copy_op, mainly used for csv VMs.
 *
 * @param vpsp points to kvm related data
 * @param cmd psp cmd id, bit 31 indicates queue priority
 * @param data_gpa guest physical address of input data
 * @param psp_ret indicates Asynchronous context information
 *
 * Since the csv guest memory cannot be read or written directly,
 * the shared asynchronous context information is shared through psp_ret and return value.
 */
int kvm_pv_psp_forward_op(struct kvm_vpsp *vpsp, uint32_t cmd,
			gpa_t data_gpa, uint32_t psp_ret)
{
	int ret;
	uint64_t data_hpa;
	uint32_t index = 0, vid = 0;
	struct vpsp_ret psp_async = {0};
	struct vpsp_context *vpsp_ctx = NULL;
	struct vpsp_cmd *vcmd = (struct vpsp_cmd *)&cmd;
	uint8_t prio = CSV_COMMAND_PRIORITY_LOW;
	phys_addr_t hpa;

	vpsp_get_context(&vpsp_ctx, vpsp->kvm->userspace_pid);

	ret = check_cmd_forward_op_permission(vpsp, vpsp_ctx, data_gpa, cmd);
	if (unlikely(ret)) {
		pr_err("directly operation not allowed\n");
		goto end;
	}

	ret = vpsp_try_bind_vtkm(vpsp, vpsp_ctx, cmd, (uint32_t *)&psp_async);
	if (unlikely(ret || *(uint32_t *)&psp_async)) {
		pr_err("try to bind vtkm failed (ret %x, psp_async %x)\n",
			ret, *(uint32_t *)&psp_async);
		goto end;
	}

	if (vpsp_ctx)
		vid = vpsp_ctx->vid;

	*((uint32_t *)&psp_async) = psp_ret;

	hpa = gpa_to_hpa(vpsp, data_gpa);
	if (unlikely(!hpa)) {
		ret = -EFAULT;
		goto end;
	}

	data_hpa = PUT_PSP_VID(hpa, vid);

	switch (psp_async.status) {
	case VPSP_INIT:
		/* try to send command to the device for execution*/
		ret = vpsp_try_do_cmd(cmd, data_hpa, &psp_async);
		if (unlikely(ret)) {
			pr_err("[%s]: vpsp_do_cmd failed\n", __func__);
			goto end;
		}
		break;

	case VPSP_RUNNING:
		prio = vcmd->is_high_rb ? CSV_COMMAND_PRIORITY_HIGH :
			CSV_COMMAND_PRIORITY_LOW;
		index = psp_async.index;
		/* try to get the execution result from ringbuffer*/
		ret = vpsp_try_get_result(prio, index, data_hpa, &psp_async);
		if (unlikely(ret)) {
			pr_err("[%s]: vpsp_try_get_result failed\n", __func__);
			goto end;
		}
		break;

	default:
		pr_err("[%s]: invalid command status\n", __func__);
		break;
	}

end:
	/**
	 * In order to indicate both system errors and PSP errors,
	 * the psp_async.pret field needs to be reused.
	 */
	psp_async.format = VPSP_RET_PSP_FORMAT;
	if (ret) {
		psp_async.format = VPSP_RET_SYS_FORMAT;
		if (ret > 0)
			ret = -ret;
		psp_async.pret = (uint16_t)ret;
	}
	return *((int *)&psp_async);
}
EXPORT_SYMBOL_GPL(kvm_pv_psp_forward_op);

/**
 * @brief copy data in gpa to host memory and send it to psp for processing.
 *
 * @param vpsp points to kvm related data
 * @param cmd psp cmd id, bit 31 indicates queue priority
 * @param data_gpa guest physical address of input data
 * @param psp_ret_gpa guest physical address of psp_ret
 */
int kvm_pv_psp_copy_forward_op(struct kvm_vpsp *vpsp, int cmd, gpa_t data_gpa, gpa_t psp_ret_gpa)
{
	int ret = 0;
	struct vpsp_ret psp_ret = {0};
	struct vpsp_hbuf_wrapper hbuf = {0};
	struct vpsp_cmd *vcmd = (struct vpsp_cmd *)&cmd;
	struct vpsp_context *vpsp_ctx = NULL;
	phys_addr_t data_paddr = 0;
	uint8_t prio = CSV_COMMAND_PRIORITY_LOW;
	uint32_t index = 0;
	uint32_t vid = 0;

	vpsp_get_context(&vpsp_ctx, vpsp->kvm->userspace_pid);

	ret = check_cmd_copy_forward_op_permission(vpsp, vpsp_ctx, data_gpa, cmd);
	if (unlikely(ret)) {
		pr_err("copy operation not allowed\n");
		return -EPERM;
	}

	if (vpsp_ctx)
		vid = vpsp_ctx->vid;

	if (unlikely(vpsp->read_guest(vpsp->kvm, psp_ret_gpa, &psp_ret,
					sizeof(psp_ret))))
		return -EFAULT;

	switch (psp_ret.status) {
	case VPSP_INIT:
		/* copy data from guest */
		ret = kvm_pv_psp_cmd_pre_op(vpsp, data_gpa, &hbuf);
		if (unlikely(ret)) {
			psp_ret.status = VPSP_FINISH;
			pr_err("[%s]: kvm_pv_psp_cmd_pre_op failed\n",
					__func__);
			ret = -EFAULT;
			goto end;
		}

		data_paddr = PUT_PSP_VID(__psp_pa(hbuf.data), vid);
		/* try to send command to the device for execution*/
		ret = vpsp_try_do_cmd(cmd, data_paddr, (struct vpsp_ret *)&psp_ret);
		if (unlikely(ret)) {
			pr_err("[%s]: vpsp_try_do_cmd failed\n", __func__);
			ret = -EFAULT;
			goto end;
		}

		if (psp_ret.status == VPSP_RUNNING) {
			prio = vcmd->is_high_rb ? CSV_COMMAND_PRIORITY_HIGH :
				CSV_COMMAND_PRIORITY_LOW;
			g_hbuf_wrap[prio][psp_ret.index] = hbuf;
			break;

		} else if (psp_ret.status == VPSP_FINISH) {
			ret = kvm_pv_psp_cmd_post_op(vpsp, data_gpa, &hbuf);
			if (unlikely(ret)) {
				pr_err("[%s]: kvm_pv_psp_cmd_post_op failed\n",
						__func__);
				ret = -EFAULT;
				goto end;
			}
		}
		break;

	case VPSP_RUNNING:
		prio = vcmd->is_high_rb ? CSV_COMMAND_PRIORITY_HIGH :
			CSV_COMMAND_PRIORITY_LOW;
		index = psp_ret.index;
		data_paddr = PUT_PSP_VID(__psp_pa(g_hbuf_wrap[prio][index].data), vid);
		/* try to get the execution result from ringbuffer*/
		ret = vpsp_try_get_result(prio, index, data_paddr,
					(struct vpsp_ret *)&psp_ret);
		if (unlikely(ret)) {
			pr_err("[%s]: vpsp_try_get_result failed\n", __func__);
			ret = -EFAULT;
			goto end;
		}

		if (psp_ret.status == VPSP_RUNNING) {
			ret = 0;
			goto end;
		} else if (psp_ret.status == VPSP_FINISH) {
			/* copy data to guest */
			ret = kvm_pv_psp_cmd_post_op(vpsp, data_gpa,
					&g_hbuf_wrap[prio][index]);
			if (unlikely(ret)) {
				pr_err("[%s]: kvm_pv_psp_cmd_post_op failed\n",
						__func__);
				ret = -EFAULT;
			}
			goto end;
		}
		ret = -EFAULT;
		break;

	default:
		pr_err("[%s]: invalid command status\n", __func__);
		ret = -EFAULT;
		break;
	}
end:
	/* return psp_ret to guest */
	vpsp->write_guest(vpsp->kvm, psp_ret_gpa, &psp_ret, sizeof(psp_ret));
	return ret;
}
EXPORT_SYMBOL_GPL(kvm_pv_psp_copy_forward_op);

DEFINE_RWLOCK(vpsp_rwlock);

/* VPSP_VID_MAX_ENTRIES determines the maximum number of vms that can set vid.
 * but, the performance of finding vid is determined by g_vpsp_vid_num,
 * so VPSP_VID_MAX_ENTRIES can be set larger.
 */
#define VPSP_VID_MAX_ENTRIES    2048
#define VPSP_VID_NUM_MAX        64

static struct vpsp_context g_vpsp_context_array[VPSP_VID_MAX_ENTRIES];
static uint32_t g_vpsp_vid_num;
static int compare_vid_entries(const void *a, const void *b)
{
	return ((struct vpsp_context *)a)->pid - ((struct vpsp_context *)b)->pid;
}
static void swap_vid_entries(void *a, void *b, int size)
{
	struct vpsp_context entry;

	memcpy(&entry, a, size);
	memcpy(a, b, size);
	memcpy(b, &entry, size);
}

/**
 * When 'allow_default_vid' is set to 1,
 * QEMU is allowed to use 'vid 0' by default
 * in the absence of a valid 'vid' setting.
 */
uint32_t allow_default_vid = 1;
void vpsp_set_default_vid_permission(uint32_t is_allow)
{
	allow_default_vid = is_allow;
}

int vpsp_get_default_vid_permission(void)
{
	return allow_default_vid;
}

/**
 * get a vpsp context from pid
 */
int vpsp_get_context(struct vpsp_context **ctx, pid_t pid)
{
	struct vpsp_context new_entry = {.pid = pid};
	struct vpsp_context *existing_entry = NULL;

	read_lock(&vpsp_rwlock);
	existing_entry = bsearch(&new_entry, g_vpsp_context_array, g_vpsp_vid_num,
				sizeof(struct vpsp_context), compare_vid_entries);
	read_unlock(&vpsp_rwlock);

	if (!existing_entry)
		return -ENOENT;

	if (ctx)
		*ctx = existing_entry;

	return 0;
}

/**
 * Upon qemu startup, this section checks whether
 * the '-device psp,vid' parameter is specified.
 * If set, it utilizes the 'vpsp_add_vid' function
 * to insert the 'vid' and 'pid' values into the 'g_vpsp_context_array'.
 * The insertion is done in ascending order of 'pid'.
 */
static int vpsp_add_vid(uint32_t vid)
{
	pid_t cur_pid = task_pid_nr(current);
	struct vpsp_context new_entry = {.vid = vid, .pid = cur_pid};

	if (vpsp_get_context(NULL, cur_pid) == 0)
		return -EEXIST;
	if (g_vpsp_vid_num == VPSP_VID_MAX_ENTRIES)
		return -ENOMEM;
	if (vid >= VPSP_VID_NUM_MAX)
		return -EINVAL;

	write_lock(&vpsp_rwlock);
	memcpy(&g_vpsp_context_array[g_vpsp_vid_num++], &new_entry, sizeof(struct vpsp_context));
	sort(g_vpsp_context_array, g_vpsp_vid_num, sizeof(struct vpsp_context),
				compare_vid_entries, swap_vid_entries);
	pr_info("PSP: add vid %d, by pid %d, total vid num is %d\n", vid, cur_pid, g_vpsp_vid_num);
	write_unlock(&vpsp_rwlock);
	return 0;
}

/**
 * Upon the virtual machine is shut down,
 * the 'vpsp_del_vid' function is employed to remove
 * the 'vid' associated with the current 'pid'.
 */
static int vpsp_del_vid(void)
{
	pid_t cur_pid = task_pid_nr(current);
	int i, ret = -ENOENT;

	write_lock(&vpsp_rwlock);
	for (i = 0; i < g_vpsp_vid_num; ++i) {
		if (g_vpsp_context_array[i].pid == cur_pid) {
			--g_vpsp_vid_num;
			pr_info("PSP: delete vid %d, by pid %d, total vid num is %d\n",
				g_vpsp_context_array[i].vid, cur_pid, g_vpsp_vid_num);
			memmove(&g_vpsp_context_array[i], &g_vpsp_context_array[i + 1],
				sizeof(struct vpsp_context) * (g_vpsp_vid_num - i));
			ret = 0;
			goto end;
		}
	}

end:
	write_unlock(&vpsp_rwlock);
	return ret;
}

static int vpsp_set_gpa_range(u64 gpa_start, u64 gpa_end)
{
	pid_t cur_pid = task_pid_nr(current);
	struct vpsp_context *ctx = NULL;

	vpsp_get_context(&ctx, cur_pid);
	if (!ctx) {
		pr_err("PSP: %s get vpsp_context failed from pid %d\n", __func__, cur_pid);
		return -ENOENT;
	}

	ctx->gpa_start = gpa_start;
	ctx->gpa_end = gpa_end;
	pr_info("PSP: set gpa range (start 0x%llx, end 0x%llx), by pid %d\n",
		gpa_start, gpa_end, cur_pid);
	return 0;
}

int do_vpsp_op_ioctl(struct vpsp_dev_ctrl *ctrl)
{
	int ret = 0;
	unsigned char op = ctrl->op;

	switch (op) {
	case VPSP_OP_VID_ADD:
		ret = vpsp_add_vid(ctrl->data.vid);
		break;

	case VPSP_OP_VID_DEL:
		ret = vpsp_del_vid();
		break;

	case VPSP_OP_SET_DEFAULT_VID_PERMISSION:
		vpsp_set_default_vid_permission(ctrl->data.def_vid_perm);
		break;

	case VPSP_OP_GET_DEFAULT_VID_PERMISSION:
		ctrl->data.def_vid_perm = vpsp_get_default_vid_permission();
		break;

	case VPSP_OP_SET_GPA:
		ret = vpsp_set_gpa_range(ctrl->data.gpa.gpa_start, ctrl->data.gpa.gpa_end);
		break;

	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}


static DEFINE_MUTEX(vpsp_rb_mutex);
struct csv_ringbuffer_queue vpsp_ring_buffer[CSV_COMMAND_PRIORITY_NUM];

static int get_queue_tail(struct csv_ringbuffer_queue *ringbuffer)
{
	return ringbuffer->cmd_ptr.tail & ringbuffer->cmd_ptr.mask;
}

static int get_queue_head(struct csv_ringbuffer_queue *ringbuffer)
{
	return ringbuffer->cmd_ptr.head & ringbuffer->cmd_ptr.mask;
}

static void vpsp_set_cmd_status(int prio, int index, int status)
{
	struct csv_queue *ringbuf = &vpsp_ring_buffer[prio].stat_val;
	struct csv_statval_entry *statval = (struct csv_statval_entry *)ringbuf->data;

	statval[index].status = status;
}

static int vpsp_get_cmd_status(int prio, int index)
{
	struct csv_queue *ringbuf = &vpsp_ring_buffer[prio].stat_val;
	struct csv_statval_entry *statval = (struct csv_statval_entry *)ringbuf->data;

	return statval[index].status;
}

static unsigned int vpsp_queue_cmd_size(int prio)
{
	return csv_cmd_queue_size(&vpsp_ring_buffer[prio].cmd_ptr);
}

static int vpsp_dequeue_cmd(int prio, int index,
		struct csv_cmdptr_entry *cmd_ptr)
{
	mutex_lock(&vpsp_rb_mutex);

	/* The status update must be before the head update */
	vpsp_set_cmd_status(prio, index, 0);
	csv_dequeue_cmd(&vpsp_ring_buffer[prio].cmd_ptr, (void *)cmd_ptr, 1);

	mutex_unlock(&vpsp_rb_mutex);

	return 0;
}

/*
 * Populate the command from the virtual machine to the queue to
 * support execution in ringbuffer mode
 */
static int vpsp_fill_cmd_queue(int prio, int cmd, phys_addr_t phy_addr, uint16_t flags)
{
	struct csv_cmdptr_entry cmdptr = { };
	int index = -1;

	cmdptr.cmd_buf_ptr = phy_addr;
	cmdptr.cmd_id = cmd;
	cmdptr.cmd_flags = flags;

	mutex_lock(&vpsp_rb_mutex);
	index = get_queue_tail(&vpsp_ring_buffer[prio]);

	/* If status is equal to VPSP_CMD_STATUS_RUNNING, then the queue is full */
	if (vpsp_get_cmd_status(prio, index) == VPSP_CMD_STATUS_RUNNING) {
		index = -1;
		goto out;
	}

	/* The status must be written first, and then the cmd can be enqueued */
	vpsp_set_cmd_status(prio, index, VPSP_CMD_STATUS_RUNNING);
	if (csv_enqueue_cmd(&vpsp_ring_buffer[prio].cmd_ptr, &cmdptr, 1) != 1) {
		vpsp_set_cmd_status(prio, index, 0);
		index = -1;
		goto out;
	}

out:
	mutex_unlock(&vpsp_rb_mutex);
	return index;
}

static void vpsp_ring_update_head(struct csv_ringbuffer_queue *ring_buffer,
		uint32_t new_head)
{
	uint32_t orig_head = get_queue_head(ring_buffer);
	uint32_t comple_num = 0;

	if (new_head >= orig_head)
		comple_num = new_head - orig_head;
	else
		comple_num = ring_buffer->cmd_ptr.mask - (orig_head - new_head)
			+ 1;

	ring_buffer->cmd_ptr.head += comple_num;
}

static int vpsp_psp_mutex_trylock(void)
{
	int mutex_enabled = READ_ONCE(hygon_psp_hooks.psp_mutex_enabled);

	if (!hygon_psp_hooks.sev_dev_hooks_installed)
		return -ENODEV;

	if (mutex_enabled)
		return psp_mutex_trylock(&hygon_psp_hooks.psp_misc->data_pg_aligned->mb_mutex);
	else
		return mutex_trylock(hygon_psp_hooks.sev_cmd_mutex);
}

static int vpsp_psp_mutex_unlock(void)
{
	int mutex_enabled = READ_ONCE(hygon_psp_hooks.psp_mutex_enabled);

	if (!hygon_psp_hooks.sev_dev_hooks_installed)
		return -ENODEV;

	if (mutex_enabled)
		psp_mutex_unlock(&hygon_psp_hooks.psp_misc->data_pg_aligned->mb_mutex);
	else
		mutex_unlock(hygon_psp_hooks.sev_cmd_mutex);

	return 0;
}

static int __vpsp_ring_buffer_enter_locked(int *error)
{
	int ret;
	struct csv_data_ring_buffer *data;
	struct csv_ringbuffer_queue *low_queue;
	struct csv_ringbuffer_queue *hi_queue;
	struct sev_device *sev = psp_master->sev_data;

	if (!hygon_psp_hooks.sev_dev_hooks_installed)
		return -ENODEV;

	if (csv_comm_mode == CSV_COMM_RINGBUFFER_ON)
		return -EEXIST;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	low_queue = &vpsp_ring_buffer[CSV_COMMAND_PRIORITY_LOW];
	hi_queue = &vpsp_ring_buffer[CSV_COMMAND_PRIORITY_HIGH];

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

static int vpsp_wait_cmd_ioc_ring_buffer(struct sev_device *sev,
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

static int __vpsp_do_ringbuf_cmds_locked(int *psp_ret, uint8_t prio, int index)
{
	struct psp_device *psp = psp_master;
	unsigned int reg, ret = 0;
	unsigned int rb_tail, rb_head;
	unsigned int rb_ctl;
	struct sev_device *sev;

	if (!psp || !hygon_psp_hooks.sev_dev_hooks_installed)
		return -ENODEV;

	if (*hygon_psp_hooks.psp_dead)
		return -EBUSY;

	sev = psp->sev_data;

	/* update rb tail */
	rb_tail = ioread32(sev->io_regs + sev->vdata->cmdbuff_addr_hi_reg);
	rb_tail &= (~PSP_RBTAIL_QHI_TAIL_MASK);
	rb_tail |= (get_queue_tail(&vpsp_ring_buffer[CSV_COMMAND_PRIORITY_HIGH])
					<< PSP_RBTAIL_QHI_TAIL_SHIFT);
	rb_tail &= (~PSP_RBTAIL_QLO_TAIL_MASK);
	rb_tail |= get_queue_tail(&vpsp_ring_buffer[CSV_COMMAND_PRIORITY_LOW]);
	iowrite32(rb_tail, sev->io_regs + sev->vdata->cmdbuff_addr_hi_reg);

	/* update rb head */
	rb_head = ioread32(sev->io_regs + sev->vdata->cmdbuff_addr_lo_reg);
	rb_head &= (~PSP_RBHEAD_QHI_HEAD_MASK);
	rb_head |= (get_queue_head(&vpsp_ring_buffer[CSV_COMMAND_PRIORITY_HIGH])
					<< PSP_RBHEAD_QHI_HEAD_SHIFT);
	rb_head &= (~PSP_RBHEAD_QLO_HEAD_MASK);
	rb_head |= get_queue_head(&vpsp_ring_buffer[CSV_COMMAND_PRIORITY_LOW]);
	iowrite32(rb_head, sev->io_regs + sev->vdata->cmdbuff_addr_lo_reg);

	/* update rb ctl to trigger psp irq */
	sev->int_rcvd = 0;
	/* PSP response to x86 only when all queue is empty or error happends */
	rb_ctl = (PSP_RBCTL_X86_WRITES | PSP_RBCTL_RBMODE_ACT | PSP_RBCTL_CLR_INTSTAT);
	iowrite32(rb_ctl, sev->io_regs + sev->vdata->cmdresp_reg);

	/* wait for all commands in ring buffer completed */
	ret = vpsp_wait_cmd_ioc_ring_buffer(sev, &reg, (*hygon_psp_hooks.psp_timeout)*10);
	if (ret) {
		if (psp_ret)
			*psp_ret = 0;

		dev_err(psp->dev, "csv command in ringbuffer mode timed out, disabling PSP\n");
		*hygon_psp_hooks.psp_dead = true;
		return ret;
	}
	/* cmd error happends */
	if (reg & PSP_RBHEAD_QPAUSE_INT_STAT)
		ret = -EFAULT;

	/* update head */
	vpsp_ring_update_head(&vpsp_ring_buffer[CSV_COMMAND_PRIORITY_HIGH],
			(reg & PSP_RBHEAD_QHI_HEAD_MASK) >> PSP_RBHEAD_QHI_HEAD_SHIFT);
	vpsp_ring_update_head(&vpsp_ring_buffer[CSV_COMMAND_PRIORITY_LOW],
			reg & PSP_RBHEAD_QLO_HEAD_MASK);

	if (psp_ret)
		*psp_ret = vpsp_get_cmd_status(prio, index);

	return ret;
}

static int vpsp_do_ringbuf_cmds_locked(int *psp_ret, uint8_t prio, int index)
{
	struct sev_user_data_status data;
	int rc;

	if (!hygon_psp_hooks.sev_dev_hooks_installed)
		return -ENODEV;

	rc = __vpsp_ring_buffer_enter_locked(psp_ret);
	if (rc)
		goto end;

	rc = __vpsp_do_ringbuf_cmds_locked(psp_ret, prio, index);

	/* exit ringbuf mode by send CMD in mailbox mode */
	hygon_psp_hooks.__sev_do_cmd_locked(SEV_CMD_PLATFORM_STATUS,
					&data, NULL);
	csv_comm_mode = CSV_COMM_MAILBOX_ON;

end:
	return rc;
}

static int __vpsp_do_cmd_locked(int cmd, phys_addr_t phy_addr, int *psp_ret)
{
	struct psp_device *psp = psp_master;
	struct sev_device *sev;
	unsigned int phys_lsb, phys_msb;
	unsigned int reg, ret = 0;

	if (!psp || !psp->sev_data || !hygon_psp_hooks.sev_dev_hooks_installed)
		return -ENODEV;

	if (*hygon_psp_hooks.psp_dead)
		return -EBUSY;

	sev = psp->sev_data;

	/* Get the physical address of the command buffer */
	phys_lsb = phy_addr ? lower_32_bits(phy_addr) : 0;
	phys_msb = phy_addr ? upper_32_bits(phy_addr) : 0;

	dev_dbg(sev->dev, "sev command id %#x buffer 0x%08x%08x timeout %us\n",
		cmd, phys_msb, phys_lsb, *hygon_psp_hooks.psp_timeout);

	iowrite32(phys_lsb, sev->io_regs + sev->vdata->cmdbuff_addr_lo_reg);
	iowrite32(phys_msb, sev->io_regs + sev->vdata->cmdbuff_addr_hi_reg);

	sev->int_rcvd = 0;

	reg = FIELD_PREP(SEV_CMDRESP_CMD, cmd) | SEV_CMDRESP_IOC;
	iowrite32(reg, sev->io_regs + sev->vdata->cmdresp_reg);

	/* wait for command completion */
	ret = hygon_psp_hooks.sev_wait_cmd_ioc(sev, &reg, *hygon_psp_hooks.psp_timeout);
	if (ret) {
		if (psp_ret)
			*psp_ret = 0;

		dev_err(sev->dev, "sev command %#x timed out, disabling PSP\n", cmd);
		*hygon_psp_hooks.psp_dead = true;

		return ret;
	}

	*hygon_psp_hooks.psp_timeout = *hygon_psp_hooks.psp_cmd_timeout;

	if (psp_ret)
		*psp_ret = FIELD_GET(PSP_CMDRESP_STS, reg);

	if (FIELD_GET(PSP_CMDRESP_STS, reg)) {
		dev_dbg(sev->dev, "sev command %#x failed (%#010lx)\n",
			cmd, FIELD_GET(PSP_CMDRESP_STS, reg));
		ret = -EIO;
	}

	return ret;
}

int vpsp_do_cmd(int cmd, phys_addr_t phy_addr, int *psp_ret)
{
	int rc;
	int mutex_enabled = READ_ONCE(hygon_psp_hooks.psp_mutex_enabled);

	if (!hygon_psp_hooks.sev_dev_hooks_installed)
		return -ENODEV;

	if (mutex_enabled) {
		if (psp_mutex_lock_timeout(&hygon_psp_hooks.psp_misc->data_pg_aligned->mb_mutex,
					PSP_MUTEX_TIMEOUT) != 1) {
			return -EBUSY;
		}
	} else {
		mutex_lock(hygon_psp_hooks.sev_cmd_mutex);
	}

	rc = __vpsp_do_cmd_locked(cmd, phy_addr, psp_ret);

	if (mutex_enabled)
		psp_mutex_unlock(&hygon_psp_hooks.psp_misc->data_pg_aligned->mb_mutex);
	else
		mutex_unlock(hygon_psp_hooks.sev_cmd_mutex);

	return rc;
}

/*
 * Try to obtain the result again by the command index, this
 * interface is used in ringbuffer mode
 */
int vpsp_try_get_result(uint8_t prio, uint32_t index, phys_addr_t phy_addr,
		struct vpsp_ret *psp_ret)
{
	int ret = 0;
	struct csv_cmdptr_entry cmd = {0};

	if (!hygon_psp_hooks.sev_dev_hooks_installed)
		return -ENODEV;

	/* Get the retult directly if the command has been executed */
	if (index >= 0 && vpsp_get_cmd_status(prio, index) !=
			VPSP_CMD_STATUS_RUNNING) {
		psp_ret->pret = vpsp_get_cmd_status(prio, index);
		psp_ret->status = VPSP_FINISH;
		return 0;
	}

	if (vpsp_psp_mutex_trylock()) {
		/* Use mailbox mode to execute a command if there is only one command */
		if (vpsp_queue_cmd_size(prio) == 1) {
			/* dequeue command from queue*/
			vpsp_dequeue_cmd(prio, index, &cmd);

			ret = __vpsp_do_cmd_locked(cmd.cmd_id, phy_addr, (int *)psp_ret);
			psp_ret->status = VPSP_FINISH;
			vpsp_psp_mutex_unlock();
			if (unlikely(ret)) {
				if (ret == -EIO) {
					ret = 0;
				} else {
					pr_err("[%s]: psp do cmd error, %d\n",
						__func__, psp_ret->pret);
					ret = -EIO;
					goto end;
				}
			}
		} else {
			ret = vpsp_do_ringbuf_cmds_locked((int *)psp_ret, prio,
					index);
			psp_ret->status = VPSP_FINISH;
			vpsp_psp_mutex_unlock();
			if (unlikely(ret)) {
				pr_err("[%s]: vpsp_do_ringbuf_cmds_locked failed %d\n",
						__func__, ret);
				goto end;
			}
		}
	} else {
		/* Change the command to the running state if getting the mutex fails */
		psp_ret->index = index;
		psp_ret->status = VPSP_RUNNING;
		return 0;
	}
end:
	return ret;
}

/*
 * Send the virtual psp command to the PSP device and try to get the
 * execution result, the interface and the vpsp_try_get_result
 * interface are executed asynchronously. If the execution succeeds,
 * the result is returned to the VM. If the execution fails, the
 * vpsp_try_get_result interface will be used to obtain the result
 * later again
 */
int vpsp_try_do_cmd(int cmd, phys_addr_t phy_addr, struct vpsp_ret *psp_ret)
{
	int ret = 0;
	int rb_supported;
	int index = -1;
	uint8_t prio = CSV_COMMAND_PRIORITY_LOW;

	if (!hygon_psp_hooks.sev_dev_hooks_installed)
		return -ENODEV;

	/* ringbuffer mode check and parse command prio*/
	rb_supported = vpsp_rb_check_and_cmd_prio_parse(&prio,
			(struct vpsp_cmd *)&cmd);
	if (rb_supported) {
		/* fill command in ringbuffer's queue and get index */
		index = vpsp_fill_cmd_queue(prio, cmd, phy_addr, 0);
		if (unlikely(index < 0)) {
			/* do mailbox command if queuing failed*/
			ret = vpsp_do_cmd(cmd, phy_addr, (int *)psp_ret);
			if (unlikely(ret)) {
				if (ret == -EIO) {
					ret = 0;
				} else {
					pr_err("[%s]: psp do cmd error, %d\n",
						__func__, psp_ret->pret);
					ret = -EIO;
					goto end;
				}
			}
			psp_ret->status = VPSP_FINISH;
			goto end;
		}

		/* try to get result from the ringbuffer command */
		ret = vpsp_try_get_result(prio, index, phy_addr, psp_ret);
		if (unlikely(ret)) {
			pr_err("[%s]: vpsp_try_get_result failed %d\n", __func__, ret);
			goto end;
		}
	} else {
		/* mailbox mode */
		ret = vpsp_do_cmd(cmd, phy_addr, (int *)psp_ret);
		if (unlikely(ret)) {
			if (ret == -EIO) {
				ret = 0;
			} else {
				pr_err("[%s]: psp do cmd error, %d\n",
						__func__, psp_ret->pret);
				ret = -EIO;
				goto end;
			}
		}
		psp_ret->status = VPSP_FINISH;
	}

end:
	return ret;
}

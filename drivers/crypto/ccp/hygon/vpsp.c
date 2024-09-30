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

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
 *		   |-> kvm_pv_psp_cmd_pre_op
 *		   |
 *  kvm_pv_psp_copy_forward_op->|-> vpsp_try_do_cmd/vpsp_try_get_result <====> psp device driver
 *		   |
 *		   |-> kvm_pv_psp_cmd_post_op
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

/*
 * Obtain the VM command and preprocess the pointer mapping table
 * information in the command buffer, the processed data will be
 * used to interact with the psp device
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
	if ((((uintptr_t)data_gpa + data_size - 1) & ~PSP_2MB_MASK)
			!= ((uintptr_t)data_gpa & ~PSP_2MB_MASK)) {
		pr_err("data_gpa %llx, data_size %d crossing 2MB\n", (u64)data_gpa, data_size);
		return -EFAULT;
	}

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

/**
 * @brief kvm_pv_psp_copy_forward_op is used for ordinary virtual machines to copy data
 * in gpa to host memory and send it to psp for processing.
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
	uint8_t prio = CSV_COMMAND_PRIORITY_LOW;
	uint32_t index = 0;
	uint32_t vid = 0;

	if (vcmd->cmd_id != TKM_PSP_CMDID_OFFSET) {
		pr_err("[%s]: unsupported cmd id %x\n", __func__, vcmd->cmd_id);
		return -EINVAL;
	}

	// only tkm cmd need vid
	if (cmd_type_is_tkm(vcmd->cmd_id)) {
		// check the permission to use the default vid when no vid is set
		ret = vpsp_get_vid(&vid, vpsp->kvm->userspace_pid);
		if (ret && !vpsp_get_default_vid_permission()) {
			pr_err("[%s]: not allowed tkm command without vid\n", __func__);
			return -EPERM;
		}
	}

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

		/* try to send command to the device for execution*/
		ret = vpsp_try_do_cmd(vid, cmd, (void *)hbuf.data,
				(struct vpsp_ret *)&psp_ret);
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
		/* try to get the execution result from ringbuffer*/
		ret = vpsp_try_get_result(vid, prio, index, g_hbuf_wrap[prio][index].data,
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

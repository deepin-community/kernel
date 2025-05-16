// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#include "ossl_knl.h"
#include "hinic3_mt.h"
#include "hinic3_crm.h"
#include "hinic3_hw.h"
#include "mpu_inband_cmd.h"
#include "hinic3_hw_mt.h"

#define	HINIC3_CMDQ_BUF_MAX_SIZE		2048U
#define DW_WIDTH 4

#define MSG_MAX_IN_SIZE		(2048 * 1024)
#define MSG_MAX_OUT_SIZE	(2048 * 1024)

#define API_CSR_MAX_RD_LEN (4 * 1024 * 1024)

/* completion timeout interval, unit is millisecond */
#define MGMT_MSG_UPDATE_TIMEOUT		200000U

void free_buff_in(void *hwdev, const struct msg_module *nt_msg, void *buf_in)
{
	if (!buf_in)
		return;

	if (nt_msg->module == SEND_TO_NPU)
		hinic3_free_cmd_buf(hwdev, buf_in);
	else
		kfree(buf_in);
}

void free_buff_out(void *hwdev, struct msg_module *nt_msg,
		   void *buf_out)
{
	if (!buf_out)
		return;

	if (nt_msg->module == SEND_TO_NPU &&
	    !nt_msg->npu_cmd.direct_resp)
		hinic3_free_cmd_buf(hwdev, buf_out);
	else
		kfree(buf_out);
}

int alloc_buff_in(void *hwdev, struct msg_module *nt_msg,
		  u32 in_size, void **buf_in)
{
	void *msg_buf = NULL;

	if (!in_size)
		return 0;

	if (nt_msg->module == SEND_TO_NPU) {
		struct hinic3_cmd_buf *cmd_buf = NULL;

		if (in_size > HINIC3_CMDQ_BUF_MAX_SIZE) {
			pr_err("Cmdq in size(%u) more than 2KB\n", in_size);
			return -ENOMEM;
		}

		cmd_buf = hinic3_alloc_cmd_buf(hwdev);
		if (!cmd_buf) {
			pr_err("Alloc cmdq cmd buffer failed in %s\n",
			       __func__);
			return -ENOMEM;
		}
		msg_buf = cmd_buf->buf;
		*buf_in = (void *)cmd_buf;
		cmd_buf->size = (u16)in_size;
	} else {
		if (in_size > MSG_MAX_IN_SIZE) {
			pr_err("In size(%u) more than 2M\n", in_size);
			return -ENOMEM;
		}
		msg_buf = kzalloc(in_size, GFP_KERNEL);
		*buf_in = msg_buf;
	}
	if (!(*buf_in)) {
		pr_err("Alloc buffer in failed\n");
		return -ENOMEM;
	}

	if (copy_from_user(msg_buf, nt_msg->in_buf, in_size)) {
		pr_err("%s:%d: Copy from user failed\n",
		       __func__, __LINE__);
		free_buff_in(hwdev, nt_msg, *buf_in);
		return -EFAULT;
	}

	return 0;
}

int alloc_buff_out(void *hwdev, struct msg_module *nt_msg,
		   u32 out_size, void **buf_out)
{
	if (!out_size)
		return 0;

	if (nt_msg->module == SEND_TO_NPU &&
	    !nt_msg->npu_cmd.direct_resp) {
		struct hinic3_cmd_buf *cmd_buf = NULL;

		if (out_size > HINIC3_CMDQ_BUF_MAX_SIZE) {
			pr_err("Cmdq out size(%u) more than 2KB\n", out_size);
			return -ENOMEM;
		}

		cmd_buf = hinic3_alloc_cmd_buf(hwdev);
		*buf_out = (void *)cmd_buf;
	} else {
		if (out_size > MSG_MAX_OUT_SIZE) {
			pr_err("out size(%u) more than 2M\n", out_size);
			return -ENOMEM;
		}
		*buf_out = kzalloc(out_size, GFP_KERNEL);
	}
	if (!(*buf_out)) {
		pr_err("Alloc buffer out failed\n");
		return -ENOMEM;
	}

	return 0;
}

int copy_buf_out_to_user(struct msg_module *nt_msg,
			 u32 out_size, void *buf_out)
{
	int ret = 0;
	void *msg_out = NULL;

	if (out_size == 0 || !buf_out)
		return 0;

	if (nt_msg->module == SEND_TO_NPU &&
	    !nt_msg->npu_cmd.direct_resp)
		msg_out = ((struct hinic3_cmd_buf *)buf_out)->buf;
	else
		msg_out = buf_out;

	if (copy_to_user(nt_msg->out_buf, msg_out, out_size))
		ret = -EFAULT;

	return ret;
}

int get_func_type(struct hinic3_lld_dev *lld_dev, const void *buf_in, u32 in_size,
		  void *buf_out, u32 *out_size)
{
	u16 func_type;

	if (*out_size != sizeof(u16) || !buf_out) {
		pr_err("Unexpect out buf size from user :%u, expect: %lu\n",
		       *out_size, sizeof(u16));
		return -EFAULT;
	}

	func_type = hinic3_func_type(hinic3_get_sdk_hwdev_by_lld(lld_dev));

	*(u16 *)buf_out = func_type;
	return 0;
}

int get_func_id(struct hinic3_lld_dev *lld_dev, const void *buf_in, u32 in_size,
		void *buf_out, u32 *out_size)
{
	u16 func_id;

	if (*out_size != sizeof(u16) || !buf_out) {
		pr_err("Unexpect out buf size from user :%u, expect: %lu\n",
		       *out_size, sizeof(u16));
		return -EFAULT;
	}

	func_id = hinic3_global_func_id(hinic3_get_sdk_hwdev_by_lld(lld_dev));
	*(u16 *)buf_out = func_id;

	return 0;
}

int get_hw_driver_stats(struct hinic3_lld_dev *lld_dev, const void *buf_in, u32 in_size,
			void *buf_out, u32 *out_size)
{
	return hinic3_dbg_get_hw_stats(hinic3_get_sdk_hwdev_by_lld(lld_dev),
		buf_out, out_size);
}

int clear_hw_driver_stats(struct hinic3_lld_dev *lld_dev, const void *buf_in, u32 in_size,
			  void *buf_out, u32 *out_size)
{
	u16 size;

	size = hinic3_dbg_clear_hw_stats(hinic3_get_sdk_hwdev_by_lld(lld_dev));
	if (*out_size != size) {
		pr_err("Unexpect out buf size from user :%u, expect: %u\n",
		       *out_size, size);
		return -EFAULT;
	}

	return 0;
}

int get_self_test_result(struct hinic3_lld_dev *lld_dev, const void *buf_in, u32 in_size,
			 void *buf_out, u32 *out_size)
{
	u32 result;

	if (*out_size != sizeof(u32)  || !buf_out) {
		pr_err("Unexpect out buf size from user :%u, expect: %lu\n",
		       *out_size, sizeof(u32));
		return -EFAULT;
	}

	result = hinic3_get_self_test_result(hinic3_get_sdk_hwdev_by_lld(lld_dev));
	*(u32 *)buf_out = result;

	return 0;
}

int get_chip_faults_stats(struct hinic3_lld_dev *lld_dev, const void *buf_in, u32 in_size,
			  void *buf_out, u32 *out_size)
{
	u32 offset = 0;
	struct nic_cmd_chip_fault_stats *fault_info = NULL;

	if (!buf_in || !buf_out || *out_size != sizeof(*fault_info) ||
	    in_size != sizeof(*fault_info)) {
		pr_err("Unexpect out buf size from user: %u, expect: %lu\n",
		       *out_size, sizeof(*fault_info));
		return -EFAULT;
	}
	fault_info = (struct nic_cmd_chip_fault_stats *)buf_in;
	offset = fault_info->offset;

	fault_info = (struct nic_cmd_chip_fault_stats *)buf_out;
	hinic3_get_chip_fault_stats(hinic3_get_sdk_hwdev_by_lld(lld_dev),
				    fault_info->chip_fault_stats, offset);

	return 0;
}

static u32 get_up_timeout_val(enum hinic3_mod_type mod, u16 cmd)
{
	if (mod == HINIC3_MOD_COMM &&
	    (cmd == COMM_MGMT_CMD_UPDATE_FW ||
	     cmd == COMM_MGMT_CMD_UPDATE_BIOS ||
	     cmd == COMM_MGMT_CMD_ACTIVE_FW ||
	     cmd == COMM_MGMT_CMD_SWITCH_CFG ||
	     cmd == COMM_MGMT_CMD_HOT_ACTIVE_FW))
		return MGMT_MSG_UPDATE_TIMEOUT;

	return 0; /* use default mbox/apichain timeout time */
}

int send_to_mpu(void *hwdev, struct msg_module *nt_msg,
		void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
	enum hinic3_mod_type mod;
	u32 timeout;
	int ret = 0;
	u16 cmd;

	mod = (enum hinic3_mod_type)nt_msg->mpu_cmd.mod;
	cmd = nt_msg->mpu_cmd.cmd;

	if (nt_msg->mpu_cmd.api_type == API_TYPE_MBOX || nt_msg->mpu_cmd.api_type == API_TYPE_CLP) {
		timeout = get_up_timeout_val(mod, cmd);

		if (nt_msg->mpu_cmd.api_type == API_TYPE_MBOX)
			ret = hinic3_msg_to_mgmt_sync(hwdev, mod, cmd, buf_in, (u16)in_size,
						      buf_out, (u16 *)(u8 *)out_size, timeout,
						      HINIC3_CHANNEL_DEFAULT);
		else
			ret = hinic3_clp_to_mgmt(hwdev, mod, cmd, buf_in, (u16)in_size,
						 buf_out, (u16 *)out_size);
		if (ret) {
			pr_err("Message to mgmt cpu return fail, mod: %d, cmd: %u\n", mod, cmd);
			return ret;
		}
	} else if (nt_msg->mpu_cmd.api_type == API_TYPE_API_CHAIN_BYPASS) {
		pr_err("Unsupported api_type %u\n", nt_msg->mpu_cmd.api_type);
		return -EINVAL;
	} else if (nt_msg->mpu_cmd.api_type == API_TYPE_API_CHAIN_TO_MPU) {
		timeout = get_up_timeout_val(mod, cmd);
		if (hinic3_pcie_itf_id(hwdev) != SPU_HOST_ID)
			ret = hinic3_msg_to_mgmt_api_chain_sync(hwdev, mod, cmd, buf_in,
								(u16)in_size, buf_out,
								(u16 *)(u8 *)out_size, timeout);
		else
			ret = hinic3_msg_to_mgmt_sync(hwdev, mod, cmd, buf_in, (u16)in_size,
						      buf_out, (u16 *)(u8 *)out_size, timeout,
						      HINIC3_CHANNEL_DEFAULT);
		if (ret) {
			pr_err("Message to mgmt api chain cpu return fail, mod: %d, cmd: %u\n",
			       mod, cmd);
			return ret;
		}
	} else {
		pr_err("Unsupported api_type %u\n", nt_msg->mpu_cmd.api_type);
		return -EINVAL;
	}

	return ret;
}

int send_to_npu(void *hwdev, struct msg_module *nt_msg,
		void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
	int ret = 0;
	u8 cmd;
	enum hinic3_mod_type mod;

	mod = (enum hinic3_mod_type)nt_msg->npu_cmd.mod;
	cmd = nt_msg->npu_cmd.cmd;

	if (nt_msg->npu_cmd.direct_resp) {
		ret = hinic3_cmdq_direct_resp(hwdev, mod, cmd,
					      buf_in, buf_out, 0,
					      HINIC3_CHANNEL_DEFAULT);
		if (ret)
			pr_err("Send direct cmdq failed, err: %d\n", ret);
	} else {
		ret = hinic3_cmdq_detail_resp(hwdev, mod, cmd, buf_in, buf_out,
					      NULL, 0, HINIC3_CHANNEL_DEFAULT);
		if (ret)
			pr_err("Send detail cmdq failed, err: %d\n", ret);
	}

	return ret;
}

static int sm_rd16(void *hwdev, u32 id, u8 instance,
		   u8 node, struct sm_out_st *buf_out)
{
	u16 val1;
	int ret;

	ret = hinic3_sm_ctr_rd16(hwdev, node, instance, id, &val1);
	if (ret != 0) {
		pr_err("Get sm ctr information (16 bits)failed!\n");
		val1 = 0xffff;
	}

	buf_out->val1 = val1;

	return ret;
}

static int sm_rd16_clear(void *hwdev, u32 id, u8 instance,
			 u8 node, struct sm_out_st *buf_out)
{
	u16 val1;
	int ret;

	ret = hinic3_sm_ctr_rd16_clear(hwdev, node, instance, id, &val1);
	if (ret != 0) {
		pr_err("Get sm ctr clear information (16 bits)failed!\n");
		val1 = 0xffff;
	}

	buf_out->val1 = val1;

	return ret;
}

static int sm_rd32(void *hwdev, u32 id, u8 instance,
		   u8 node, struct sm_out_st *buf_out)
{
	u32 val1;
	int ret;

	ret = hinic3_sm_ctr_rd32(hwdev, node, instance, id, &val1);
	if (ret) {
		pr_err("Get sm ctr information (32 bits)failed!\n");
		val1 = ~0;
	}

	buf_out->val1 = val1;

	return ret;
}

static int sm_rd32_clear(void *hwdev, u32 id, u8 instance,
			 u8 node, struct sm_out_st *buf_out)
{
	u32 val1;
	int ret;

	ret = hinic3_sm_ctr_rd32_clear(hwdev, node, instance, id, &val1);
	if (ret) {
		pr_err("Get sm ctr clear information(32 bits) failed!\n");
		val1 = ~0;
	}

	buf_out->val1 = val1;

	return ret;
}

static int sm_rd64_pair(void *hwdev, u32 id, u8 instance,
			u8 node, struct sm_out_st *buf_out)
{
	u64 val1 = 0, val2 = 0;
	int ret;

	ret = hinic3_sm_ctr_rd64_pair(hwdev, node, instance, id, &val1, &val2);
	if (ret) {
		pr_err("Get sm ctr information (64 bits pair)failed!\n");
		val1 = ~0;
		val2 = ~0;
	}

	buf_out->val1 = val1;
	buf_out->val2 = val2;

	return ret;
}

static int sm_rd64_pair_clear(void *hwdev, u32 id, u8 instance,
			      u8 node, struct sm_out_st *buf_out)
{
	u64 val1 = 0;
	u64 val2 = 0;
	int ret;

	ret = hinic3_sm_ctr_rd64_pair_clear(hwdev, node, instance, id, &val1,
					    &val2);
	if (ret) {
		pr_err("Get sm ctr clear information(64 bits pair) failed!\n");
		val1 = ~0;
		val2 = ~0;
	}

	buf_out->val1 = val1;
	buf_out->val2 = val2;

	return ret;
}

static int sm_rd64(void *hwdev, u32 id, u8 instance,
		   u8 node, struct sm_out_st *buf_out)
{
	u64 val1;
	int ret;

	ret = hinic3_sm_ctr_rd64(hwdev, node, instance, id, &val1);
	if (ret) {
		pr_err("Get sm ctr information (64 bits)failed!\n");
		val1 = ~0;
	}
	buf_out->val1 = val1;

	return ret;
}

static int sm_rd64_clear(void *hwdev, u32 id, u8 instance,
			 u8 node, struct sm_out_st *buf_out)
{
	u64 val1;
	int ret;

	ret = hinic3_sm_ctr_rd64_clear(hwdev, node, instance, id, &val1);
	if (ret) {
		pr_err("Get sm ctr clear information(64 bits) failed!\n");
		val1 = ~0;
	}
	buf_out->val1 = val1;

	return ret;
}

typedef int (*sm_module)(void *hwdev, u32 id, u8 instance,
			 u8 node, struct sm_out_st *buf_out);

struct sm_module_handle {
	enum sm_cmd_type	sm_cmd_name;
	sm_module		sm_func;
};

const struct sm_module_handle sm_module_cmd_handle[] = {
	{.sm_cmd_name = SM_CTR_RD16,		.sm_func = sm_rd16},
	{.sm_cmd_name = SM_CTR_RD32,		.sm_func = sm_rd32},
	{.sm_cmd_name = SM_CTR_RD64_PAIR,	.sm_func = sm_rd64_pair},
	{.sm_cmd_name = SM_CTR_RD64,		.sm_func = sm_rd64},
	{.sm_cmd_name = SM_CTR_RD16_CLEAR,	.sm_func = sm_rd16_clear},
	{.sm_cmd_name = SM_CTR_RD32_CLEAR,	.sm_func = sm_rd32_clear},
	{.sm_cmd_name = SM_CTR_RD64_PAIR_CLEAR,	.sm_func = sm_rd64_pair_clear},
	{.sm_cmd_name = SM_CTR_RD64_CLEAR,	.sm_func = sm_rd64_clear}
};

int send_to_sm(void *hwdev, struct msg_module *nt_msg,
	       void *buf_in, u32 in_size, void *buf_out, u32 *out_size)
{
	struct sm_in_st *sm_in = buf_in;
	struct sm_out_st *sm_out = buf_out;
	u32 msg_formate;
	int index, num_cmds = ARRAY_LEN(sm_module_cmd_handle);
	int ret = 0;

	if (!nt_msg || !buf_in || !buf_out ||
	    in_size != sizeof(*sm_in) || *out_size != sizeof(*sm_out)) {
		pr_err("Unexpect out buf size :%u, in buf size: %u\n",
		       *out_size, in_size);
		return -EINVAL;
	}
	msg_formate = nt_msg->msg_formate;

	for (index = 0; index < num_cmds; index++) {
		if (msg_formate != sm_module_cmd_handle[index].sm_cmd_name)
			continue;

		ret = sm_module_cmd_handle[index].sm_func(hwdev, (u32)sm_in->id,
							  (u8)sm_in->instance,
							  (u8)sm_in->node, sm_out);
		break;
	}

	if (index == num_cmds) {
		pr_err("Can't find callback for %d\n", msg_formate);
		return -EINVAL;
	}
	if (ret != 0)
		pr_err("Get sm information fail, id:%d, instance:%d, node:%d\n",
		       sm_in->id, sm_in->instance, sm_in->node);

	*out_size = sizeof(struct sm_out_st);

	return ret;
}


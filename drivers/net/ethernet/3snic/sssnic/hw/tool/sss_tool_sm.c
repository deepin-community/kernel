// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */
#define pr_fmt(fmt) KBUILD_MODNAME ": [TOOL]" fmt

#include "sss_hwdev.h"
#include "sss_hwif_adm.h"
#include "sss_tool_comm.h"
#include "sss_tool_sm.h"

#define SSS_TOOL_CHIP_ACK 1
#define SSS_TOOL_CHIP_NOACK 0

#define SSS_TOOL_SM_CHIP_OP_READ 0x2
#define SSS_TOOL_SM_CHIP_OP_READ_CLEAR 0x6

#define SSS_TOOL_BIT_32 32

struct sss_tool_sm_in {
	int node;
	int id;
	int instance;
};

struct sss_tool_sm_out {
	u64 val1;
	u64 val2;
};

union sss_tool_sm_chip_request_head {
	struct {
		u32  pad:15;
		u32  ack:1;
		u32  op_id:5;
		u32  instance:6;
		u32  src:5;
	} bs;

	u32 value;
};

/* counter read request struct */
struct sss_tool_sm_chip_request {
	u32 extra;
	union sss_tool_sm_chip_request_head head;
	u32 ctr_id;
	u32 initial;
	u32 pad;
};

/* counter read response union */
union sss_tool_chip_rd_response {
	struct {
		u32 value1:16;
		u32 pad0:16;
		u32 pad1[3];
	} bs_ss16_rsp;

	struct {
		u32 value1;
		u32 pad[3];
	} bs_ss32_rsp;

	struct {
		u32 value1:20;
		u32 pad0:12;
		u32 value2:12;
		u32 pad1:20;
		u32 pad2[2];
	} bs_sp_rsp;

	struct {
		u32 value1;
		u32 value2;
		u32 pad[2];
	} bs_bs64_rsp;

	struct {
		u32 val1_h;
		u32 val1_l;
		u32 val2_h;
		u32 val2_l;
	} bs_bp64_rsp;
};

typedef int (*sss_tool_sm_handler_func)(void *hwdev, u32 id, u8 instance,
			 u8 node, struct sss_tool_sm_out *out_buf);

struct sss_tool_sm_handler {
	enum sss_tool_sm_cmd_type	msg_name;
	sss_tool_sm_handler_func	sm_func;
};

static void sss_tool_sm_read_msg_create(struct sss_tool_sm_chip_request *request,
					u8 instance_id, u8 op_id,
					u8 ack, u32 ctr_id, u32 init_val)
{
	request->head.value = 0;
	request->head.bs.op_id = op_id;
	request->head.bs.ack = ack;
	request->head.bs.instance = instance_id;
	request->head.value = HTONL(request->head.value);

	request->initial = init_val;
	request->ctr_id = ctr_id;
	request->ctr_id = HTONL(request->ctr_id);
}

static void sss_tool_sm_node_htonl(u32 *node, u32 len)
{
	u32 *new_node = node;
	u32 i;

	for (i = 0; i < len; i++) {
		*new_node = HTONL(*new_node);
		new_node++;
	}
}

static int sss_tool_sm_adm_msg_rd(void *hwdev, u32 id, u8 instance,
				  u8 node, union sss_tool_chip_rd_response *rsp, u8 opcode)
{
	struct sss_tool_sm_chip_request req = {0};
	int ret;

	if (!hwdev)
		return -EFAULT;

	if (!SSS_SUPPORT_ADM_MSG((struct sss_hwdev *)hwdev)) {
		tool_err("Fail to read sm data, device not support adm msg\n");
		return -EPERM;
	}

	sss_tool_sm_read_msg_create(&req, instance, opcode,
				    SSS_TOOL_CHIP_ACK, id, 0);

	ret = sss_adm_msg_read_ack(hwdev, node, (u8 *)&req,
				   (unsigned short)sizeof(req),
				   (void *)rsp,
				   (unsigned short)sizeof(*rsp));
	if (ret) {
		tool_err("Fail to read sm data from adm msg, err(%d)\n", ret);
		return ret;
	}

	sss_tool_sm_node_htonl((u32 *)rsp, sizeof(*rsp) / sizeof(u32));

	return 0;
}

static int sss_tool_sm_msg_rd16(void *hwdev, u32 id, u8 instance,
				u8 node, struct sss_tool_sm_out *out_buf)
{
	u16 val1;
	union sss_tool_chip_rd_response rsp;
	int ret = 0;

	ret = sss_tool_sm_adm_msg_rd(hwdev, id, instance, node, &rsp, SSS_TOOL_SM_CHIP_OP_READ);
	if (ret) {
		tool_err("Fail to read sm 32 bits\n");
		val1 = ~0;
		goto out;
	}

	val1 = rsp.bs_ss16_rsp.value1;
out:
	out_buf->val1 = val1;

	return ret;
}

static int sss_tool_sm_msg_rd32(void *hwdev, u32 id, u8 instance,
				u8 node, struct sss_tool_sm_out *out_buf)
{
	u32 val1;
	union sss_tool_chip_rd_response rsp;
	int ret = 0;

	ret = sss_tool_sm_adm_msg_rd(hwdev, id, instance, node, &rsp, SSS_TOOL_SM_CHIP_OP_READ);
	if (ret) {
		tool_err("Fail to read sm 32 bits\n");
		val1 = ~0;
		goto out;
	}

	val1 = rsp.bs_ss32_rsp.value1;
out:
	out_buf->val1 = val1;

	return ret;
}

static int sss_tool_sm_msg_rd32_clear(void *hwdev, u32 id, u8 instance,
				      u8 node, struct sss_tool_sm_out *out_buf)
{
	u32 val1;
	union sss_tool_chip_rd_response rsp;
	int ret = 0;

	ret = sss_tool_sm_adm_msg_rd(hwdev, id, instance, node,
				     &rsp, SSS_TOOL_SM_CHIP_OP_READ_CLEAR);
	if (ret) {
		tool_err("Fail to read sm 32 bits\n");
		val1 = ~0;
		goto out;
	}

	val1 = rsp.bs_ss32_rsp.value1;

out:
	out_buf->val1 = val1;
	return ret;
}

static int sss_tool_sm_msg_rd128(void *hwdev, u32 id, u8 instance,
				 u8 node, struct sss_tool_sm_out *out_buf)
{
	u64 val1 = 0;
	u64 val2 = 0;
	int ret = 0;
	union sss_tool_chip_rd_response rsp;

	if ((id & 0x1) != 0) {
		tool_err("Invalid id(%u), It is odd number\n", id);
		val1 = ~0;
		val2 = ~0;
		ret = -EINVAL;
		goto out;
	}

	ret = sss_tool_sm_adm_msg_rd(hwdev, id, instance, node,
				     &rsp, SSS_TOOL_SM_CHIP_OP_READ);
	if (ret) {
		tool_err("Fail to read sm 128 bits\n");
		val1 = ~0;
		val2 = ~0;
		goto out;
	}

	sss_tool_sm_node_htonl((u32 *)&rsp, sizeof(rsp) / sizeof(u32));
	val1 = ((u64)rsp.bs_bp64_rsp.val1_h << SSS_TOOL_BIT_32) | rsp.bs_bp64_rsp.val1_l;
	val2 = ((u64)rsp.bs_bp64_rsp.val2_h << SSS_TOOL_BIT_32) | rsp.bs_bp64_rsp.val2_l;

out:
	out_buf->val1 = val1;
	out_buf->val2 = val2;

	return ret;
}

static int sss_tool_sm_msg_rd128_clear(void *hwdev, u32 id, u8 instance,
				       u8 node, struct sss_tool_sm_out *out_buf)
{
	u64 val1 = 0;
	u64 val2 = 0;
	int ret = 0;
	union sss_tool_chip_rd_response rsp;

	if ((id & 0x1) != 0) {
		tool_err("Invalid id(%u), It is odd number\n", id);
		val1 = ~0;
		val2 = ~0;
		ret = -EINVAL;
		goto out;
	}

	ret = sss_tool_sm_adm_msg_rd(hwdev, id, instance, node,
				     &rsp, SSS_TOOL_SM_CHIP_OP_READ_CLEAR);
	if (ret) {
		tool_err("Fail to read sm 128 bits\n");
		val1 = ~0;
		val2 = ~0;
		goto out;
	}

	val1 = ((u64)rsp.bs_bp64_rsp.val1_h << SSS_TOOL_BIT_32) | rsp.bs_bp64_rsp.val1_l;
	val2 = ((u64)rsp.bs_bp64_rsp.val2_h << SSS_TOOL_BIT_32) | rsp.bs_bp64_rsp.val2_l;

out:
	out_buf->val1 = val1;
	out_buf->val2 = val2;

	return ret;
}

static int sss_tool_sm_msg_rd64(void *hwdev, u32 id, u8 instance,
				u8 node, struct sss_tool_sm_out *out_buf)
{
	u64 val1 = 0;
	int ret = 0;
	union sss_tool_chip_rd_response rsp;

	ret = sss_tool_sm_adm_msg_rd(hwdev, id, instance, node,
				     &rsp, SSS_TOOL_SM_CHIP_OP_READ);
	if (ret) {
		tool_err("Fail to read sm 64 bits\n");
		val1 = ~0;
		goto out;
	}

	val1 = ((u64)rsp.bs_bs64_rsp.value1 << SSS_TOOL_BIT_32) | rsp.bs_bs64_rsp.value2;

out:
	out_buf->val1 = val1;

	return ret;
}

static int sss_tool_sm_msg_rd64_clear(void *hwdev, u32 id, u8 instance,
				      u8 node, struct sss_tool_sm_out *out_buf)
{
	u64 val1 = 0;
	int ret = 0;
	union sss_tool_chip_rd_response rsp;

	ret = sss_tool_sm_adm_msg_rd(hwdev, id, instance, node,
				     &rsp, SSS_TOOL_SM_CHIP_OP_READ_CLEAR);
	if (ret) {
		tool_err("Fail to read sm 64 bits\n");
		val1 = ~0;
		goto out;
	}

	val1 = ((u64)rsp.bs_bs64_rsp.value1 << SSS_TOOL_BIT_32) | rsp.bs_bs64_rsp.value2;

out:
	out_buf->val1 = val1;

	return ret;
}

const struct sss_tool_sm_handler g_sm_cmd_handle[] = {
	{SSS_TOOL_SM_CMD_RD16,			sss_tool_sm_msg_rd16},
	{SSS_TOOL_SM_CMD_RD32,			sss_tool_sm_msg_rd32},
	{SSS_TOOL_SM_CMD_RD32_CLEAR,		sss_tool_sm_msg_rd32_clear},
	{SSS_TOOL_SM_CMD_RD64,			sss_tool_sm_msg_rd64},
	{SSS_TOOL_SM_CMD_RD64_CLEAR,		sss_tool_sm_msg_rd64_clear},
	{SSS_TOOL_SM_CMD_RD64_PAIR,		sss_tool_sm_msg_rd128},
	{SSS_TOOL_SM_CMD_RD64_PAIR_CLEAR,	sss_tool_sm_msg_rd128_clear}
};

int sss_tool_msg_to_sm(struct sss_hal_dev *hal_dev, struct sss_tool_msg *msg,
		       void *in_buf, u32 in_len, void *out_buf, u32 *out_len)
{
	int index;
	int ret = 0;
	int cmd_num = ARRAY_LEN(g_sm_cmd_handle);
	u32 msg_formate = msg->msg_formate;
	struct sss_tool_sm_in *sm_in = in_buf;
	struct sss_tool_sm_out *sm_out = out_buf;

	if (!in_buf || !out_buf || !out_len) {
		tool_err("Invalid in_buf or out buf param\n");
		return -EINVAL;
	}

	if (in_len != sizeof(*sm_in) || *out_len != sizeof(*sm_out)) {
		tool_err("Invalid out buf size :%u, in buf size: %u\n",
			 *out_len, in_len);
		return -EINVAL;
	}

	for (index = 0; index < cmd_num; index++) {
		if (msg_formate != g_sm_cmd_handle[index].msg_name)
			continue;

		ret = g_sm_cmd_handle[index].sm_func(hal_dev->hwdev, (u32)sm_in->id,
						     (u8)sm_in->instance, (u8)sm_in->node, sm_out);
		break;
	}

	if (index == cmd_num) {
		tool_err("Fail to execute msg %d,could not find callback\n", msg_formate);
		return -EINVAL;
	}

	if (ret != 0)
		tool_err("Fail to get sm information, id:%u, instance:%u, node:%u, msg:%d\n",
			 sm_in->id, sm_in->instance, sm_in->node, msg_formate);

	*out_len = sizeof(*sm_out);

	return ret;
}

// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt

#include <linux/kernel.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/dcbnl.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_nic_cfg.h"
#include "sss_nic_rss_cfg.h"
#include "sss_nic_cfg_define.h"
#include "sss_nic_io_define.h"
#include "sss_nic_event.h"

int sss_nic_cfg_rss_hash_key(struct sss_nic_dev *nic_dev, u8 opcode, u8 *hash_key)
{
	int ret;
	struct sss_nic_mbx_rss_key_cfg cmd_rss_hash_key = {0};
	u16 out_len = sizeof(cmd_rss_hash_key);

	if (opcode == SSSNIC_MBX_OPCODE_SET)
		memcpy(cmd_rss_hash_key.key, hash_key, SSSNIC_RSS_KEY_SIZE);

	cmd_rss_hash_key.opcode = opcode;
	cmd_rss_hash_key.func_id = sss_get_global_func_id(nic_dev->hwdev);

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MBX_OPCODE_CFG_RSS_HASH_KEY,
					     &cmd_rss_hash_key, sizeof(cmd_rss_hash_key),
					     &cmd_rss_hash_key, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_rss_hash_key)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to hash key,opcode: %d ret: %d, status: 0x%x, out_len: 0x%x\n",
			opcode, ret, cmd_rss_hash_key.head.state, out_len);
		return -EINVAL;
	}

	if (opcode == SSSNIC_MBX_OPCODE_GET)
		memcpy(hash_key, cmd_rss_hash_key.key, SSSNIC_RSS_KEY_SIZE);

	return 0;
}

int sss_nic_set_rss_hash_key(struct sss_nic_dev *nic_dev, const u8 *hash_key)
{
	u8 rss_hash_key[SSSNIC_RSS_KEY_SIZE];

	memcpy(rss_hash_key, hash_key, SSSNIC_RSS_KEY_SIZE);
	return sss_nic_cfg_rss_hash_key(nic_dev, SSSNIC_MBX_OPCODE_SET, rss_hash_key);
}

int sss_nic_get_rss_indir_tbl(struct sss_nic_dev *nic_dev, u32 *indir_tbl)
{
	int i;
	int ret;
	u16 *temp_tbl = NULL;
	struct sss_ctrl_msg_buf *msg_buf = NULL;

	if (!nic_dev || !indir_tbl)
		return -EINVAL;

	msg_buf = sss_alloc_ctrlq_msg_buf(nic_dev->hwdev);
	if (!msg_buf) {
		nic_err(nic_dev->dev_hdl, "Fail to allocate cmd buf\n");
		return -ENOMEM;
	}

	msg_buf->size = sizeof(struct sss_nic_rss_indirect_table);
	ret = sss_ctrlq_detail_reply(nic_dev->hwdev, SSS_MOD_TYPE_L2NIC,
				     SSSNIC_CTRLQ_OPCODE_GET_RSS_INDIR_TABLE,
				     msg_buf, msg_buf, NULL, 0,
				     SSS_CHANNEL_NIC);
	if (ret != 0) {
		nic_err(nic_dev->dev_hdl, "Fail to get rss indir tbl\n");
		goto get_tbl_fail;
	}

	temp_tbl = (u16 *)msg_buf->buf;
	for (i = 0; i < SSSNIC_RSS_INDIR_SIZE; i++)
		indir_tbl[i] = *(temp_tbl + i);

get_tbl_fail:
	sss_free_ctrlq_msg_buf(nic_dev->hwdev, msg_buf);

	return ret;
}

static void sss_nic_fill_indir_tbl(struct sss_nic_rss_indirect_table *indir_tbl,
				   const u32 *indir_table)
{
	u32 i;
	u32 tbl_size;
	u32 *temp_entry = NULL;

	memset(indir_tbl, 0, sizeof(*indir_tbl));
	for (i = 0; i < SSSNIC_RSS_INDIR_SIZE; i++)
		indir_tbl->entry[i] = (u16)indir_table[i];

	temp_entry = (u32 *)indir_tbl->entry;
	tbl_size = sizeof(indir_tbl->entry) / (sizeof(u32));
	for (i = 0; i < tbl_size; i++)
		temp_entry[i] = cpu_to_be32(temp_entry[i]);
}

int sss_nic_set_rss_indir_tbl(struct sss_nic_dev *nic_dev, const u32 *indir_tbl)
{
	int ret;
	u64 output_param = 0;
	struct sss_ctrl_msg_buf *msg_buf = NULL;

	if (!nic_dev || !indir_tbl)
		return -EINVAL;

	msg_buf = sss_alloc_ctrlq_msg_buf(nic_dev->hwdev);
	if (!msg_buf) {
		nic_err(nic_dev->dev_hdl, "Fail to allocate cmd buf\n");
		return -ENOMEM;
	}

	msg_buf->size = sizeof(struct sss_nic_rss_indirect_table);

	sss_nic_fill_indir_tbl(msg_buf->buf, indir_tbl);

	ret = sss_ctrlq_direct_reply(nic_dev->hwdev, SSS_MOD_TYPE_L2NIC,
				     SSSNIC_CTRLQ_OPCODE_SET_RSS_INDIR_TABLE,
				     msg_buf, &output_param,
				     0, SSS_CHANNEL_NIC);
	if (ret != 0 || output_param != 0) {
		sss_free_ctrlq_msg_buf(nic_dev->hwdev, msg_buf);
		nic_err(nic_dev->dev_hdl, "Fail to set rss indir tbl\n");
		return -EFAULT;
	}

	sss_free_ctrlq_msg_buf(nic_dev->hwdev, msg_buf);
	return ret;
}

static int sss_nic_set_rss_type_by_ctrlq(struct sss_nic_dev *nic_dev, u32 ctx)
{
	int ret;
	u64 output_param = 0;
	struct sss_nic_rss_ctx_table *rss_ctx_tbl = NULL;
	struct sss_ctrl_msg_buf *msg_buf = NULL;

	msg_buf = sss_alloc_ctrlq_msg_buf(nic_dev->hwdev);
	if (!msg_buf) {
		nic_err(nic_dev->dev_hdl, "Fail to allocate cmd buf\n");
		return -ENOMEM;
	}

	rss_ctx_tbl = (struct sss_nic_rss_ctx_table *)msg_buf->buf;
	memset(rss_ctx_tbl, 0, sizeof(*rss_ctx_tbl));
	rss_ctx_tbl->ctx = cpu_to_be32(ctx);
	msg_buf->size = sizeof(*rss_ctx_tbl);

	ret = sss_ctrlq_direct_reply(nic_dev->hwdev, SSS_MOD_TYPE_L2NIC,
				     SSSNIC_CTRLQ_OPCODE_SET_RSS_CONTEXT_TABLE, msg_buf,
				     &output_param, 0, SSS_CHANNEL_NIC);
	if (ret != 0 || output_param != 0) {
		sss_free_ctrlq_msg_buf(nic_dev->hwdev, msg_buf);
		nic_err(nic_dev->dev_hdl, "Fail to set rss ctx, ret: %d\n", ret);
		return -EFAULT;
	}

	sss_free_ctrlq_msg_buf(nic_dev->hwdev, msg_buf);

	return 0;
}

static int sss_nic_set_rss_type_by_mbx(struct sss_nic_dev *nic_dev, u32 ctx)
{
	struct sss_nic_mbx_rss_ctx ctx_tbl = {0};
	u16 out_len = sizeof(ctx_tbl);
	int ret;

	ctx_tbl.func_id = sss_get_global_func_id(nic_dev->hwdev);
	ctx_tbl.context = ctx;
	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev,
					     SSSNIC_MBX_OPCODE_SET_RSS_CTX_TBL_INTO_FUNC,
					     &ctx_tbl, sizeof(ctx_tbl), &ctx_tbl, &out_len);

	if (ctx_tbl.head.state == SSS_MGMT_CMD_UNSUPPORTED) {
		return SSS_MGMT_CMD_UNSUPPORTED;
	} else if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &ctx_tbl)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to set rss ctx, ret: %d, status: 0x%x, out_len: 0x%x\n",
			ret, ctx_tbl.head.state, out_len);
		return -EINVAL;
	}

	return 0;
}

int sss_nic_set_rss_type(struct sss_nic_dev *nic_dev, struct sss_nic_rss_type rss_type)
{
	int ret;
	u32 ctx = 0;

	ctx |= SSSNIC_RSS_TYPE_SET(rss_type.ipv4, IPV4) |
	       SSSNIC_RSS_TYPE_SET(rss_type.tcp_ipv4, TCP_IPV4) |
	       SSSNIC_RSS_TYPE_SET(rss_type.udp_ipv4, UDP_IPV4) |
	       SSSNIC_RSS_TYPE_SET(rss_type.ipv6, IPV6) |
	       SSSNIC_RSS_TYPE_SET(rss_type.ipv6_ext, IPV6_EXT) |
	       SSSNIC_RSS_TYPE_SET(rss_type.tcp_ipv6, TCP_IPV6) |
	       SSSNIC_RSS_TYPE_SET(rss_type.tcp_ipv6_ext, TCP_IPV6_EXT) |
	       SSSNIC_RSS_TYPE_SET(rss_type.udp_ipv6, UDP_IPV6) |
	       SSSNIC_RSS_TYPE_SET(1, VALID);

	ret = sss_nic_set_rss_type_by_mbx(nic_dev, ctx);
	if (ret == SSS_MGMT_CMD_UNSUPPORTED)
		ret = sss_nic_set_rss_type_by_ctrlq(nic_dev, ctx);

	return ret;
}

int sss_nic_get_rss_type(struct sss_nic_dev *nic_dev, struct sss_nic_rss_type *rss_type)
{
	int ret;
	struct sss_nic_mbx_rss_ctx rss_ctx_tbl = {0};
	u16 out_len = sizeof(rss_ctx_tbl);

	if (!nic_dev || !rss_type)
		return -EINVAL;

	rss_ctx_tbl.func_id = sss_get_global_func_id(nic_dev->hwdev);

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MBX_OPCODE_GET_RSS_CTX_TBL,
					     &rss_ctx_tbl, sizeof(rss_ctx_tbl),
					     &rss_ctx_tbl, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &rss_ctx_tbl)) {
		nic_err(nic_dev->dev_hdl, "Fail to get hash type, ret: %d, status: 0x%x, out_len: 0x%x\n",
			ret, rss_ctx_tbl.head.state, out_len);
		return -EINVAL;
	}

	rss_type->ipv4	       = SSSNIC_RSS_TYPE_GET(rss_ctx_tbl.context, IPV4);
	rss_type->ipv6	       = SSSNIC_RSS_TYPE_GET(rss_ctx_tbl.context, IPV6);
	rss_type->ipv6_ext     = SSSNIC_RSS_TYPE_GET(rss_ctx_tbl.context, IPV6_EXT);
	rss_type->tcp_ipv4     = SSSNIC_RSS_TYPE_GET(rss_ctx_tbl.context, TCP_IPV4);
	rss_type->tcp_ipv6     = SSSNIC_RSS_TYPE_GET(rss_ctx_tbl.context, TCP_IPV6);
	rss_type->tcp_ipv6_ext = SSSNIC_RSS_TYPE_GET(rss_ctx_tbl.context, TCP_IPV6_EXT);
	rss_type->udp_ipv4     = SSSNIC_RSS_TYPE_GET(rss_ctx_tbl.context, UDP_IPV4);
	rss_type->udp_ipv6     = SSSNIC_RSS_TYPE_GET(rss_ctx_tbl.context, UDP_IPV6);

	return 0;
}

int sss_nic_rss_hash_engine(struct sss_nic_dev *nic_dev, u8 cmd, u8 *hash_engine)
{
	int ret;
	struct sss_nic_mbx_rss_engine_cfg cmd_rss_engine = {0};
	u16 out_len = sizeof(cmd_rss_engine);

	cmd_rss_engine.opcode = cmd;
	cmd_rss_engine.func_id = sss_get_global_func_id(nic_dev->hwdev);

	if (cmd == SSSNIC_MBX_OPCODE_SET)
		cmd_rss_engine.hash_engine = *hash_engine;

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev,
					     SSSNIC_MBX_OPCODE_CFG_RSS_HASH_ENGINE,
					     &cmd_rss_engine, sizeof(cmd_rss_engine),
					     &cmd_rss_engine, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_rss_engine)) {
		nic_err(nic_dev->dev_hdl, "Fail to handle hash engine,opcode:%d, ret: %d, status: 0x%x, out_len: 0x%x\n",
			cmd, ret, cmd_rss_engine.head.state, out_len);

		return -EIO;
	}

	if (cmd == SSSNIC_MBX_OPCODE_GET)
		*hash_engine = cmd_rss_engine.hash_engine;

	return 0;
}

int sss_nic_set_rss_hash_engine(struct sss_nic_dev *nic_dev, u8 hash_engine)
{
	return sss_nic_rss_hash_engine(nic_dev, SSSNIC_MBX_OPCODE_SET, &hash_engine);
}

int sss_nic_config_rss_to_hw(struct sss_nic_dev *nic_dev,
			     u8 cos_num, u8 *cos_map, u16 qp_num, u8 rss_en)
{
	int ret;
	struct sss_nic_mbx_rss_cfg cmd_rss_cfg = {0};
	u16 out_len = sizeof(cmd_rss_cfg);

	if (!nic_dev || !cos_map || (cos_num & (cos_num - 1)) != 0)
		return -EINVAL;

	cmd_rss_cfg.rss_en = rss_en;
	cmd_rss_cfg.qp_num = qp_num;
	cmd_rss_cfg.rq_priority_number = (cos_num > 0) ? (u8)ilog2(cos_num) : 0;
	cmd_rss_cfg.func_id = sss_get_global_func_id(nic_dev->hwdev);
	memcpy(cmd_rss_cfg.prio_tc, cos_map, SSSNIC_DCB_UP_MAX);

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MBX_OPCODE_RSS_CFG,
					     &cmd_rss_cfg, sizeof(cmd_rss_cfg),
					     &cmd_rss_cfg, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_rss_cfg)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to set rss cfg, ret: %d, status: 0x%x, out_len: 0x%x\n",
			ret, cmd_rss_cfg.head.state, out_len);
		return -EINVAL;
	}

	return 0;
}

int sss_nic_init_hw_rss(struct sss_nic_dev *nic_dev, u16 qp_num)
{
	int ret;
	struct sss_nic_mbx_rss_cfg cmd_rss_cfg = {0};
	u16 out_len = sizeof(cmd_rss_cfg);

	if (!nic_dev)
		return -EINVAL;

	cmd_rss_cfg.qp_num = qp_num;
	cmd_rss_cfg.func_id = sss_get_global_func_id(nic_dev->hwdev);

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MBX_OPCODE_RSS_CFG,
					     &cmd_rss_cfg, sizeof(cmd_rss_cfg),
					     &cmd_rss_cfg, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_rss_cfg)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to set rss cfg, ret: %d, status: 0x%x, out_len: 0x%x\n",
			ret, cmd_rss_cfg.head.state, out_len);
		return -EINVAL;
	}

	return 0;
}

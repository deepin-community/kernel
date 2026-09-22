// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#include "osdep.h"
#include "status.h"
#include "hmc.h"
#include "defs.h"
#include "type.h"
#include "protos.h"
#include "uda.h"
#include "uda_d.h"
#include "vf.h"
#include "virtchnl.h"
#include "main.h"

static int ah_remote_ip_info_process(struct zxdh_device *iwdev, struct zxdh_ah_info *ah_info, int op_type)
{
	struct zxdh_rdma_to_eth_ip_para ip_para = {0};
	u64 dmac = 0;
	u64 smac = 0;
	int ret = 0;

	dmac = LS_64_1(ah_info->dmac[5], 0) | LS_64_1(ah_info->dmac[4], 8) |
	       LS_64_1(ah_info->dmac[3], 16) | LS_64_1(ah_info->dmac[2], 24) |
	       LS_64_1(ah_info->dmac[1], 32) | LS_64_1(ah_info->dmac[0], 40);
	smac = LS_64_1(ah_info->mac_addr[5], 0) | LS_64_1(ah_info->mac_addr[4], 8) |
	       LS_64_1(ah_info->mac_addr[3], 16) | LS_64_1(ah_info->mac_addr[2], 24) |
	       LS_64_1(ah_info->mac_addr[1], 32) | LS_64_1(ah_info->mac_addr[0], 40);

	ip_para.ifname = iwdev->netdev->name;
	ip_para.ipv4 = ah_info->ipv4_valid;
	if (ip_para.ipv4) {
		ip_para.src_ip[0] = 0;
		ip_para.src_ip[1] = 0;
		ip_para.src_ip[2] = 0;
		ip_para.src_ip[3] = ah_info->src_ip_addr[0];
		ip_para.dst_ip[0] = 0;
		ip_para.dst_ip[1] = 0;
		ip_para.dst_ip[2] = 0;
		ip_para.dst_ip[3] = ah_info->dest_ip_addr[0];
	} else {
		memcpy(ip_para.src_ip, ah_info->src_ip_addr, sizeof(ip_para.src_ip));
		memcpy(ip_para.dst_ip, ah_info->dest_ip_addr, sizeof(ip_para.dst_ip));
	}
	ip_para.src_mac = smac;
	ip_para.dst_mac = dmac;
	ip_para.mode = op_type;

	pr_debug("[zxdh_rdma] double plane %s[%d]: ipv4=%d, op_type=%d, src_ip=0x%x-0x%x-0x%x-0x%x, dst_ip=0x%x-0x%x-0x%x-0x%x, dst_mac=0x%llx, dmac=%x-%x-%x-%x-%x-%x\n",
		__func__, __LINE__, ip_para.ipv4, op_type,
		ip_para.src_ip[0], ip_para.src_ip[1], ip_para.src_ip[2], ip_para.src_ip[3],
		ip_para.dst_ip[0], ip_para.dst_ip[1], ip_para.dst_ip[2], ip_para.dst_ip[3],
		ip_para.dst_mac, ah_info->dmac[0], ah_info->dmac[1], ah_info->dmac[2], ah_info->dmac[3], ah_info->dmac[4], ah_info->dmac[5]);
	pr_debug("[zxdh_rdma] double plane %s[%d]: src_mac=0x%llx, smac=%x-%x-%x-%x-%x-%x\n",
		__func__, __LINE__, ip_para.src_mac,
		ah_info->mac_addr[0], ah_info->mac_addr[1], ah_info->mac_addr[2], ah_info->mac_addr[3], ah_info->mac_addr[4], ah_info->mac_addr[5]);

	if (op_type == RDMA_ADD_REMOTE_IP || op_type == RDMA_DEL_REMOTE_IP) {
		ret = remote_ip_info_process(iwdev, &ip_para);
	} else {
		pr_info("[zxdh_rdma] %s[%d]: error op_type=%d\n", __func__, __LINE__, op_type);
		ret = -1;
	}

	return ret;
}

/**
 * zxdh_sc_access_ah() - Create, modify or delete AH
 * @cqp: struct for cqp hw
 * @info: ah information
 * @op: Operation
 * @scratch: u64 saved to be used during cqp completion
 */
int zxdh_sc_access_ah(struct zxdh_sc_cqp *cqp, struct zxdh_ah_info *info,
		      u32 op, u64 scratch)
{
	__le64 *wqe;
	u64 qw1, qw2;
    struct ib_device *ibdev = zxdh_get_ibdev(cqp->dev);
    struct zxdh_device *iwdev = to_iwdev(ibdev);
    struct iidc_core_dev_info *cdev_info = (struct iidc_core_dev_info *)iwdev->rf->cdev;
    u32 dual_tor_switch = 0xFFFF;

	wqe = zxdh_sc_cqp_get_next_send_wqe(cqp, scratch);
	if (!wqe)
		return -ENOSPC;

	info->tc_tos &= ~ECN_CODE_PT_MASK;
	info->tc_tos |= ECN_CODE_PT_VAL;

	qw1 = FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_PDINDEX, info->pd_idx) |
	      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_AVIDX, info->ah_idx) |
	      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_TC, info->tc_tos) |
	      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_INSERTVLANTAG,
			 info->insert_vlan_tag) |
	      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_IPV4VALID, info->ipv4_valid) |
	      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_WQEVALID, cqp->polarity) |
	      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_OPCODE, op);

	qw2 = FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_FLOWLABEL, info->flow_label) |
	      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_HOPLIMIT, info->hop_ttl);
	set_64bit_val(wqe, 8, qw2);

	set_64bit_val(wqe, 16,
		      FIELD_PREP(ZXDH_UDAQPC_VLANTAG, info->vlan_tag) |
			      LS_64_1(info->mac_addr[5], 16) |
			      LS_64_1(info->mac_addr[4], 24) |
			      LS_64_1(info->mac_addr[3], 32) |
			      LS_64_1(info->mac_addr[2], 40) |
			      LS_64_1(info->mac_addr[1], 48) |
			      LS_64_1(info->mac_addr[0], 56));

	set_64bit_val(wqe, 24,
		      LS_64_1(info->dmac[5], 16) | LS_64_1(info->dmac[4], 24) |
			      LS_64_1(info->dmac[3], 32) |
			      LS_64_1(info->dmac[2], 40) |
			      LS_64_1(info->dmac[1], 48) |
			      LS_64_1(info->dmac[0], 56));

	if (!info->ipv4_valid) {
		set_64bit_val(wqe, 32,
			      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_ADDR1,
					 info->dest_ip_addr[1]) |
				      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_ADDR0,
						 info->dest_ip_addr[0]));
		set_64bit_val(wqe, 40,
			      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_ADDR3,
					 info->dest_ip_addr[3]) |
				      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_ADDR2,
						 info->dest_ip_addr[2]));
		set_64bit_val(wqe, 48,
			      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_ADDR1,
					 info->src_ip_addr[1]) |
				      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_ADDR0,
						 info->src_ip_addr[0]));
		set_64bit_val(wqe, 56,
			      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_ADDR3,
					 info->src_ip_addr[3]) |
				      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_ADDR2,
						 info->src_ip_addr[2]));
	} else {
		set_64bit_val(wqe, 40,
			      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_ADDR3,
					 info->dest_ip_addr[0]));
		set_64bit_val(wqe, 56,
			      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_ADDR3,
					 info->src_ip_addr[0]));
	}

    dual_tor_switch = readl(cdev_info->hw_addr + ZXDH_DUAL_TOR_SWITCH_OFFSET);
    pr_debug("[zxdh_rdma] %s[%d]: hw_addr=0x%llx, dual_tor_switch=0x%x\n", __func__, __LINE__, (u64)cdev_info->hw_addr, dual_tor_switch);
    if ((dual_tor_switch == ZXDH_DUAL_TOR_SWITCH_OPEN)) {
        if (op == ZXDH_CQP_OP_CREATE_AH) {
            ah_remote_ip_info_process(iwdev, info, RDMA_ADD_REMOTE_IP);
        } else if (op == ZXDH_CQP_OP_DESTROY_AH) {
            ah_remote_ip_info_process(iwdev, info, RDMA_DEL_REMOTE_IP);
        }
    }

	dma_wmb(); /* need write block before writing WQE header */

	set_64bit_val(wqe, 0, qw1);

	print_hex_dump_debug("WQE: MANAGE_AH WQE", DUMP_PREFIX_OFFSET, 16, 8,
			     wqe, ZXDH_CQP_WQE_SIZE * 8, false);
	zxdh_sc_cqp_post_sq(cqp);

	return 0;
}

/**
 * zxdh_create_mg_ctx() - create a mcg context
 * @info: multicast group context info
 */
static int zxdh_create_mg_ctx(struct zxdh_mcast_grp_info *info)
{
	struct zxdh_mcast_grp_ctx_entry_info *entry_info = NULL;
	u32 idx = 0; /* index in the array */
	u32 ctx_idx = 0; /* index in the MG context */

	memset(info->dma_mem_mc.va, 0,
	       ZXDH_MAX_MGS_PER_CTX * sizeof(u32) + sizeof(u64));

	for (idx = 0; idx < ZXDH_MAX_MGS_PER_CTX; idx++) {
		entry_info = &info->mg_ctx_info[idx];
		if (entry_info->valid_entry) {
			set_32bit_val((__le32 *)info->dma_mem_mc.va,
				      sizeof(u64) + ctx_idx * sizeof(u32),
				      FIELD_PREP(ZXDH_UDA_MGCTX_QPID,
						 entry_info->qp_id));
			ctx_idx++;
		}
	}
	set_64bit_val((__le64 *)info->dma_mem_mc.va, 0, ctx_idx);

	return 0;
}

/**
 * zxdh_access_mcast_grp() - Access mcast group based on op
 * @cqp: Control QP
 * @info: multicast group context info
 * @op: operation to perform
 * @scratch: u64 saved to be used during cqp completion
 */
int zxdh_access_mcast_grp(struct zxdh_sc_cqp *cqp,
			  struct zxdh_mcast_grp_info *info, u32 op, u64 scratch)
{
	__le64 *wqe;
	u64 dmac;

	if (info->mg_id >= ZXDH_UDA_MAX_FSI_MGS) {
		pr_err("[zxdh_rdma] WQE: mg_id out of range\n");
		return -EINVAL;
	}

	wqe = zxdh_sc_cqp_get_next_send_wqe(cqp, scratch);
	if (!wqe) {
		pr_err("[zxdh_rdma] WQE: ring full\n");
		return -ENOSPC;
	}

	zxdh_create_mg_ctx(info);

	dmac = LS_64_1(info->dest_mac_addr[5], 0) |
	       LS_64_1(info->dest_mac_addr[4], 8) |
	       LS_64_1(info->dest_mac_addr[3], 16) |
	       LS_64_1(info->dest_mac_addr[2], 24) |
	       LS_64_1(info->dest_mac_addr[1], 32) |
	       LS_64_1(info->dest_mac_addr[0], 40);

	set_64bit_val(wqe, 8,
		      FIELD_PREP(ZXDH_UDA_CQPSQ_MG_DMAC, dmac) |
			      FIELD_PREP(ZXDH_UDA_CQPSQ_MG_VLANID,
					 info->vlan_id));

	if (!info->ipv4_valid) {
		set_64bit_val(wqe, 24,
			      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_ADDR0,
					 info->dest_ip_addr[0]) |
				      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_ADDR1,
						 info->dest_ip_addr[1]));
		set_64bit_val(wqe, 16,
			      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_ADDR2,
					 info->dest_ip_addr[2]) |
				      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_ADDR3,
						 info->dest_ip_addr[3]));
	} else {
		set_64bit_val(wqe, 24,
			      FIELD_PREP(ZXDH_UDA_CQPSQ_MAV_ADDR0,
					 info->dest_ip_addr[0]));
	}

	set_64bit_val(wqe, 32, info->dma_mem_mc.pa);

	dma_wmb(); /* need write memory block before writing the WQE header. */

	set_64bit_val(wqe, 0,
		      FIELD_PREP(ZXDH_UDA_CQPSQ_MG_MGIDX, info->mg_id) |
			      FIELD_PREP(ZXDH_UDA_CQPSQ_MG_VLANVALID,
					 info->vlan_valid) |
			      FIELD_PREP(ZXDH_UDA_CQPSQ_MG_IPV4VALID,
					 info->ipv4_valid) |
			      FIELD_PREP(ZXDH_UDA_CQPSQ_MG_WQEVALID,
					 cqp->polarity) |
			      FIELD_PREP(ZXDH_UDA_CQPSQ_MG_OPCODE, op));

	zxdh_sc_cqp_post_sq(cqp);

	return 0;
}

/**
 * zxdh_compare_mgs - Compares two multicast group structures
 * @entry1: Multcast group info
 * @entry2: Multcast group info in context
 */
static bool zxdh_compare_mgs(struct zxdh_mcast_grp_ctx_entry_info *entry1,
			     struct zxdh_mcast_grp_ctx_entry_info *entry2)
{
	if (entry1->dest_port == entry2->dest_port &&
	    entry1->qp_id == entry2->qp_id)
		return true;

	return false;
}

/**
 * zxdh_sc_add_mcast_grp - Allocates mcast group entry in ctx
 * @ctx: Multcast group context
 * @mg: Multcast group info
 */
int zxdh_sc_add_mcast_grp(struct zxdh_mcast_grp_info *ctx,
			  struct zxdh_mcast_grp_ctx_entry_info *mg)
{
	u32 idx;
	bool free_entry_found = false;
	u32 free_entry_idx = 0;

	/* find either an identical or a free entry for a multicast group */
	for (idx = 0; idx < ZXDH_MAX_MGS_PER_CTX; idx++) {
		if (ctx->mg_ctx_info[idx].valid_entry) {
			if (zxdh_compare_mgs(&ctx->mg_ctx_info[idx], mg)) {
				ctx->mg_ctx_info[idx].use_cnt++;
				return 0;
			}
			continue;
		}
		if (!free_entry_found) {
			free_entry_found = true;
			free_entry_idx = idx;
		}
	}

	if (free_entry_found) {
		ctx->mg_ctx_info[free_entry_idx] = *mg;
		ctx->mg_ctx_info[free_entry_idx].valid_entry = true;
		ctx->mg_ctx_info[free_entry_idx].use_cnt = 1;
		ctx->no_of_mgs++;
		return 0;
	}

	return -ENOMEM;
}

/**
 * zxdh_sc_del_mcast_grp - Delete mcast group
 * @ctx: Multcast group context
 * @mg: Multcast group info
 *
 * Finds and removes a specific mulicast group from context, all
 * parameters must match to remove a multicast group.
 */
int zxdh_sc_del_mcast_grp(struct zxdh_mcast_grp_info *ctx,
			  struct zxdh_mcast_grp_ctx_entry_info *mg)
{
	u32 idx;

	/* find an entry in multicast group context */
	for (idx = 0; idx < ZXDH_MAX_MGS_PER_CTX; idx++) {
		if (!ctx->mg_ctx_info[idx].valid_entry)
			continue;

		if (zxdh_compare_mgs(mg, &ctx->mg_ctx_info[idx])) {
			ctx->mg_ctx_info[idx].use_cnt--;

			if (!ctx->mg_ctx_info[idx].use_cnt) {
				ctx->mg_ctx_info[idx].valid_entry = false;
				ctx->no_of_mgs--;
				/* Remove gap if element was not the last */
				if (idx != ctx->no_of_mgs &&
				    ctx->no_of_mgs > 0) {
					memcpy(&ctx->mg_ctx_info[idx],
					       &ctx->mg_ctx_info[ctx->no_of_mgs -
								 1],
					       sizeof(ctx->mg_ctx_info[idx]));
					ctx->mg_ctx_info[ctx->no_of_mgs - 1]
						.valid_entry = false;
				}
			}

			return 0;
		}
	}

	return -EINVAL;
}

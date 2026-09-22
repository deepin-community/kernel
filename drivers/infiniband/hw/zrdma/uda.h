/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef ZXDH_UDA_H
#define ZXDH_UDA_H

#define ZXDH_UDA_MAX_FSI_MGS 8192
#define ZXDH_UDA_MAX_PFS 16
#define ZXDH_UDA_MAX_VFS 128

struct zxdh_sc_cqp;

struct zxdh_ah_info {
	struct zxdh_sc_vsi *vsi;
	u32 pd_idx;
	u32 dest_ip_addr[4];
	u32 src_ip_addr[4];
	u32 flow_label;
	u32 ah_idx;
	u16 vlan_tag;
	u8 insert_vlan_tag;
	u8 tc_tos;
	u8 hop_ttl;
	u8 mac_addr[ETH_ALEN];
	u8 dmac[ETH_ALEN];
	u8 ah_valid : 1;
	u8 ipv4_valid : 1;
	u8 do_lpbk : 1;
};

struct zxdh_sc_ah {
	struct zxdh_sc_dev *dev;
	struct zxdh_ah_info ah_info;
};

int zxdh_sc_add_mcast_grp(struct zxdh_mcast_grp_info *ctx,
			  struct zxdh_mcast_grp_ctx_entry_info *mg);
int zxdh_sc_del_mcast_grp(struct zxdh_mcast_grp_info *ctx,
			  struct zxdh_mcast_grp_ctx_entry_info *mg);
int zxdh_sc_access_ah(struct zxdh_sc_cqp *cqp, struct zxdh_ah_info *info,
		      u32 op, u64 scratch);
int zxdh_access_mcast_grp(struct zxdh_sc_cqp *cqp,
			  struct zxdh_mcast_grp_info *info, u32 op,
			  u64 scratch);

static inline void zxdh_sc_init_ah(struct zxdh_sc_dev *dev,
				   struct zxdh_sc_ah *ah)
{
	ah->dev = dev;
}

static inline int zxdh_sc_create_ah(struct zxdh_sc_cqp *cqp,
				    struct zxdh_ah_info *info, u64 scratch)
{
	return zxdh_sc_access_ah(cqp, info, ZXDH_CQP_OP_CREATE_AH, scratch);
}

static inline int zxdh_sc_destroy_ah(struct zxdh_sc_cqp *cqp,
				     struct zxdh_ah_info *info, u64 scratch)
{
	return zxdh_sc_access_ah(cqp, info, ZXDH_CQP_OP_DESTROY_AH, scratch);
}

static inline int zxdh_sc_create_mcast_grp(struct zxdh_sc_cqp *cqp,
					   struct zxdh_mcast_grp_info *info,
					   u64 scratch)
{
	return zxdh_access_mcast_grp(cqp, info, ZXDH_CQP_OP_CREATE_MCAST_GRP,
				     scratch);
}

static inline int zxdh_sc_modify_mcast_grp(struct zxdh_sc_cqp *cqp,
					   struct zxdh_mcast_grp_info *info,
					   u64 scratch)
{
	return zxdh_access_mcast_grp(cqp, info, ZXDH_CQP_OP_MODIFY_MCAST_GRP,
				     scratch);
}

static inline int zxdh_sc_destroy_mcast_grp(struct zxdh_sc_cqp *cqp,
					    struct zxdh_mcast_grp_info *info,
					    u64 scratch)
{
	return zxdh_access_mcast_grp(cqp, info, ZXDH_CQP_OP_DESTROY_MCAST_GRP,
				     scratch);
}
#endif /* ZXDH_UDA_H */

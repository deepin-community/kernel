/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef ZXDH_VIRTCHNL_H
#define ZXDH_VIRTCHNL_H

#include "hmc.h"

#pragma pack(push, 1)

struct zxdh_virtchnl_op_buf {
	u16 op_code;
	u16 op_ver;
	u16 buf_len;
	u16 rsvd;
	u64 op_ctx;
	/* Member alignment MUST be maintained above this location */
	u8 buf[];
};

struct zxdh_virtchnl_resp_buf {
	u64 op_ctx;
	u16 buf_len;
	s16 op_ret_code;
	/* Member alignment MUST be maintained above this location */
	u16 rsvd[2];
	u8 buf[];
};

enum zxdh_virtchnl_ops {
	ZXDH_VCHNL_OP_GET_VER = 0,
	ZXDH_VCHNL_OP_GET_HMC_FCN = 1,
	ZXDH_VCHNL_OP_PUT_HMC_FCN = 2,
	ZXDH_VCHNL_OP_ADD_HMC_OBJ_RANGE = 3,
	ZXDH_VCHNL_OP_DEL_HMC_OBJ_RANGE = 4,
	ZXDH_VCHNL_OP_GET_STATS = 5,
	ZXDH_VCHNL_OP_MANAGE_STATS_INST = 6,
	ZXDH_VCHNL_OP_MCG = 7,
	ZXDH_VCHNL_OP_UP_MAP = 8,
	ZXDH_VCHNL_OP_MANAGE_WS_NODE = 9,
	ZXDH_VCHNL_OP_VLAN_PARSING = 12,
};

#define ZXDH_VCHNL_CHNL_VER_V0 0
#define ZXDH_VCHNL_CHNL_VER_V1 1

#define ZXDH_VCHNL_OP_GET_VER_V0 0
#define ZXDH_VCHNL_OP_GET_VER_V1 1

#define ZXDH_VCHNL_OP_GET_HMC_FCN_V0 0
#define ZXDH_VCHNL_OP_PUT_HMC_FCN_V0 0
#define ZXDH_VCHNL_OP_ADD_HMC_OBJ_RANGE_V0 0
#define ZXDH_VCHNL_OP_DEL_HMC_OBJ_RANGE_V0 0
#define ZXDH_VCHNL_OP_GET_STATS_V0 0
#define ZXDH_VCHNL_OP_MANAGE_WS_NODE_V0 0
#define ZXDH_VCHNL_OP_VLAN_PARSING_V0 0
#define ZXDH_VCHNL_INVALID_VF_IDX 0xFFFF

struct zxdh_virtchnl_hmc_obj_range {
	u16 obj_type;
	u16 rsvd;
	u32 start_index;
	u32 obj_count;
};

struct zxdh_virtchnl_manage_ws_node {
	u8 add;
	u8 user_pri;
};

struct zxdh_vfdev *zxdh_find_vf_dev(struct zxdh_sc_dev *dev, u16 vf_id);
void zxdh_put_vfdev(struct zxdh_sc_dev *dev, struct zxdh_vfdev *vf_dev);
void zxdh_remove_vf_dev(struct zxdh_sc_dev *dev, struct zxdh_vfdev *vf_dev);
struct zxdh_virtchnl_req {
	struct zxdh_virtchnl_op_buf *vchnl_msg;
	void *parm;
	u32 vf_id;
	u16 parm_len;
	u16 resp_len;
};

#pragma pack(pop)

#endif

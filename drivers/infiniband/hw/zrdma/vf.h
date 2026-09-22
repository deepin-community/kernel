/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef ZXDH_VF_H
#define ZXDH_VF_H

struct zxdh_sc_cqp;

struct zxdh_manage_vf_pble_info {
	u32 sd_index;
	u16 first_pd_index;
	u16 pd_entry_cnt;
	u8 inv_pd_ent;
	u64 pd_pl_pba;
};

int zxdh_manage_vf_pble_bp(struct zxdh_sc_cqp *cqp,
			   struct zxdh_manage_vf_pble_info *info, u64 scratch,
			   bool post_sq);
#endif

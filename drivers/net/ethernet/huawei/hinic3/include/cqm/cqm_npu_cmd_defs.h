/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef CQM_NPU_CMD_DEFS_H
#define CQM_NPU_CMD_DEFS_H

struct tag_cqm_cla_cache_invalid_cmd {
	u32 gpa_h;
	u32 gpa_l;

	u32 cache_size; /* CLA cache size=4096B */

	u32 smf_id;
	u32 func_id;
};

struct tag_cqm_cla_update_cmd {
	/* Gpa address to be updated */
	u32 gpa_h; // byte addr
	u32 gpa_l; // byte addr

	/* Updated Value */
	u32 value_h;
	u32 value_l;

	u32 smf_id;
	u32 func_id;
};

struct tag_cqm_bloomfilter_cmd {
	u32 rsv1;

#if (BYTE_ORDER == LITTLE_ENDIAN)
	u32 k_en : 4;
	u32 func_id : 16;
	u32 rsv2 : 12;
#else
	u32 rsv2 : 12;
	u32 func_id : 16;
	u32 k_en : 4;
#endif

	u32 index_h;
	u32 index_l;
};

#define CQM_BAT_MAX_SIZE 256
struct tag_cqm_cmdq_bat_update {
	u32 offset;	// byte offset,16Byte aligned
	u32 byte_len;	// max size: 256byte
	u8 data[CQM_BAT_MAX_SIZE];
	u32 smf_id;
	u32 func_id;
};

struct tag_cqm_bloomfilter_init_cmd {
	u32 bloom_filter_len;	// 16Byte aligned
	u32 bloom_filter_addr;
};

#endif /* CQM_CMDQ_H */

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2024 Huawei Technologies Co., Ltd */

#ifndef NPU_CMDQ_BASE_DEFS_H
#define NPU_CMDQ_BASE_DEFS_H

/* CmdQ Common subtype */
enum comm_cmdq_cmd {
	COMM_CMD_UCODE_ARM_BIT_SET = 2,
	COMM_CMD_SEND_NPU_DFT_CMD,
};

/* Cmdq ack type */
enum hinic3_ack_type {
	HINIC3_ACK_TYPE_CMDQ,
	HINIC3_ACK_TYPE_SHARE_CQN,
	HINIC3_ACK_TYPE_APP_CQN,

	HINIC3_MOD_ACK_MAX = 15,
};

/* Defines the queue type of the set arm bit. */
enum {
	SET_ARM_BIT_FOR_CMDQ = 0,
	SET_ARM_BIT_FOR_L2NIC_SQ,
	SET_ARM_BIT_FOR_L2NIC_RQ,
	SET_ARM_BIT_TYPE_NUM
};

/* Defines the type. Each function supports a maximum of eight CMDQ types. */
enum {
	CMDQ_0 = 0,
	CMDQ_1 = 1, /* dedicated and non-blocking queues */
	CMDQ_NUM
};

/* *******************cmd common command data structure ************************ */
// Func->ucode, which is used to set arm bit data,
// The microcode needs to perform big-endian conversion.
struct comm_info_ucode_set_arm_bit {
	u32 q_type;
	u32 q_id;
};

/* *******************WQE data structure ************************ */
union cmdq_wqe_cs_dw0 {
	struct {
		u32 err_status : 29;
		u32 error_code : 2;
		u32 rsvd : 1;
	} bs;
	u32 val;
};

union cmdq_wqe_cs_dw1 {
	struct {
		u32 token : 16;	// [15:0]
		u32 cmd : 8;	// [23:16]
		u32 mod : 5;	// [28:24]
		u32 ack_type : 2; // [30:29]
		u32 obit : 1;	// [31]
	} drv_wr;		// This structure is used when the driver writes the wqe.

	struct {
		u32 mod : 5;	// [4:0]
		u32 ack_type : 3; // [7:5]
		u32 cmd : 8;	// [15:8]
		u32 arm : 1;	// [16]
		u32 rsvd : 14;	// [30:17]
		u32 obit : 1;	// [31]
	} wb;
	u32 val;
};

/* CmdQ BD information or write back buffer information */
struct cmdq_sge {
	u32 pa_h;	// Upper 32 bits of the physical address
	u32 pa_l;	// Upper 32 bits of the physical address
	u32 len;	// Invalid bit[31].
	u32 resv;
};

/* Ctrls section definition of WQE */
struct cmdq_wqe_ctrls {
	union {
		struct {
			u32 bdsl : 8;	// [7:0]
			u32 drvsl : 2;	// [9:8]
			u32 rsv : 4;	// [13:10]
			u32 wf : 1;	// [14]
			u32 cf : 1;	// [15]
			u32 tsl : 5;	// [20:16]
			u32 va : 1;	// [21]
			u32 df : 1;	// [22]
			u32 cr : 1;	// [23]
			u32 difsl : 3;	// [26:24]
			u32 csl : 2;	// [28:27]
			u32 ctrlsl : 2;	// [30:29]
			u32 obit : 1;	// [31]
		} bs;
		u32 val;
	} header;
	u32 qsf;
};

/* Complete section definition of WQE */
struct cmdq_wqe_cs {
	union cmdq_wqe_cs_dw0 dw0;
	union cmdq_wqe_cs_dw1 dw1;
	union {
		struct cmdq_sge sge;
		u32 dw2_5[4];
	} ack;
};

/* Inline header in WQE inline, describing the length of inline data */
union cmdq_wqe_inline_header {
	struct {
		u32 buf_len : 11; // [10:0] inline data len
		u32 rsv : 21;	 // [31:11]
	} bs;
	u32 val;
};

/* Definition of buffer descriptor section in WQE */
union cmdq_wqe_bds {
	struct {
		struct cmdq_sge bds_sge;
		u32 rsvd[4]; /* Zwy is used to transfer the virtual address of the buffer. */
	} lcmd;	/* Long command, non-inline, and SGE describe the buffer information. */
};

/* Definition of CMDQ WQE */
/*	(long cmd, 64B)
 * +----------------------------------------+
 * |	ctrl section(8B)			|
 * +----------------------------------------+
 * |										|
 * |	complete section(24B)			|
 * |										|
 * +----------------------------------------+
 * |										|
 * |	buffer descriptor section(16B)		|
 * |										|
 * +----------------------------------------+
 * |	driver section(16B)			|
 * +----------------------------------------+
 *
 *
 * (middle cmd, 128B)
 * +----------------------------------------+
 * |	ctrl section(8B)			|
 * +----------------------------------------+
 * |										|
 * |	complete section(24B)			|
 * |										|
 * +----------------------------------------+
 * |										|
 * |	buffer descriptor section(88B)		|
 * |										|
 * +----------------------------------------+
 * |	driver section(8B)			|
 * +----------------------------------------+
 *
 *
 * (short cmd, 64B)
 * +----------------------------------------+
 * |	ctrl section(8B)			|
 * +----------------------------------------+
 * |										|
 * |	complete section(24B)			|
 * |										|
 * +----------------------------------------+
 * |										|
 * |	buffer descriptor section(24B)		|
 * |										|
 * +----------------------------------------+
 * |	driver section(8B)			|
 * +----------------------------------------+
 */
struct cmdq_wqe {
	struct cmdq_wqe_ctrls ctrls;
	struct cmdq_wqe_cs cs;
	union cmdq_wqe_bds bds;
};

/* Definition of ctrls section in inline WQE */
struct cmdq_wqe_ctrls_inline {
	union {
		struct {
			u32 bdsl : 8;	// [7:0]
			u32 drvsl : 2;	// [9:8]
			u32 rsv : 4;	// [13:10]
			u32 wf : 1;	// [14]
			u32 cf : 1;	// [15]
			u32 tsl : 5;	// [20:16]
			u32 va : 1;	// [21]
			u32 df : 1;	// [22]
			u32 cr : 1;	// [23]
			u32 difsl : 3;	// [26:24]
			u32 csl : 2;	// [28:27]
			u32 ctrlsl : 2;	// [30:29]
			u32 obit : 1;	// [31]
		} bs;
		u32 val;
	} header;
	u32 qsf;
	u64 db;
};

/* Buffer descriptor section definition of WQE */
union cmdq_wqe_bds_inline {
	struct {
		union cmdq_wqe_inline_header header;
		u32 rsvd;
		u8 data_inline[80];
	} mcmd; /* Middle command, inline mode */

	struct {
		union cmdq_wqe_inline_header header;
		u32 rsvd;
		u8 data_inline[16];
	} scmd; /* Short command, inline mode */
};

struct cmdq_wqe_inline {
	struct cmdq_wqe_ctrls_inline ctrls;
	struct cmdq_wqe_cs cs;
	union cmdq_wqe_bds_inline bds;
};

#endif

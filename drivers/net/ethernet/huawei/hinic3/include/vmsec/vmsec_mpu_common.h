/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2024 Huawei Technologies Co., Ltd */

#ifndef VMSEC_MPU_COMMON_H
#define VMSEC_MPU_COMMON_H

#include "mpu_cmd_base_defs.h"

#define VM_GPA_INFO_MODE_MIG	0
#define VM_GPA_INFO_MODE_NMIG	1

/**
 * Commands between VMSEC to MPU
 */
enum tag_vmsec_mpu_cmd {
	/* vmsec ctx gpa */
	VMSEC_MPU_CMD_CTX_GPA_SET = 0,
	VMSEC_MPU_CMD_CTX_GPA_SHOW,
	VMSEC_MPU_CMD_CTX_GPA_DEL,

	/* vmsec pci hole */
	VMSEC_MPU_CMD_PCI_HOLE_SET,
	VMSEC_MPU_CMD_PCI_HOLE_SHOW,
	VMSEC_MPU_CMD_PCI_HOLE_DEL,

	/* vmsec func cfg */
	VMSEC_MPU_CMD_FUN_CFG_ENTRY_IDX_SET,
	VMSEC_MPU_CMD_FUN_CFG_ENTRY_IDX_SHOW,

	VMSEC_MPU_CMD_MAX
};

struct vmsec_ctx_gpa_entry {
#if defined(BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)
	u32 func_id : 16;
	u32 mode	: 8;
	u32 rsvd	: 8;
#else
	u32 rsvd	: 8;
	u32 mode	: 8;
	u32 func_id : 16;
#endif

	/* sml tbl to wr */
	u32 gpa_addr0_hi;
	u32 gpa_addr0_lo;
	u32 gpa_len0;
};

struct vmsec_pci_hole_idx {
#if defined(BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)
	u32 entry_idx : 5;
	u32 rsvd	  : 27;
#else
	u32 rsvd	  : 27;
	u32 entry_idx : 5;
#endif
};

struct vmsec_pci_hole_entry {
	/* sml tbl to wr */
	/* pcie hole 32-bit region */
	u32 gpa_addr0_hi;
	u32 gpa_addr0_lo;
	u32 gpa_len0_hi;
	u32 gpa_len0_lo;

	/* pcie hole 64-bit region */
	u32 gpa_addr1_hi;
	u32 gpa_addr1_lo;
	u32 gpa_len1_hi;
	u32 gpa_len1_lo;

	/* ctrl info used by drv */
	u32 domain_id;	/* unique vm id */
#if defined(BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)
	u32 rsvd1	: 21;
	u32 vf_nums	: 11;
#else
	u32 rsvd1	: 21;
	u32 vf_nums : 11;
#endif
	u32 vroce_vf_bitmap;
};

struct vmsec_funcfg_info_entry {
	/* funcfg to update */
#if defined(BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)
	u32 func_id	: 16;
	u32 entry_vld	: 1;
	u32 entry_idx	: 5;
	u32 rsvd	: 10;
#else
	u32 rsvd	: 10;
	u32 entry_idx	: 5;
	u32 entry_vld	: 1;
	u32 func_id	: 16;
#endif
};

/* set/get/del */
struct vmsec_cfg_ctx_gpa_entry_cmd {
	struct comm_info_head head;
	struct vmsec_ctx_gpa_entry entry;
};

#endif /* VMSEC_MPU_COMMON_H */

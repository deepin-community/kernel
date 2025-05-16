/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef VRAM_COMMON_H
#define VRAM_COMMON_H

#include <linux/pci.h>
#include <linux/notifier.h>

#define VRAM_BLOCK_SIZE_2M		0x200000UL
#define KEXEC_SIGN			"hinic-in-kexec"
// now vram_name max len is 14, when add other vram, attention this value
#define VRAM_NAME_MAX_LEN		16

#define VRAM_CQM_GLB_FUNC_BASE		"F"
#define VRAM_CQM_FAKE_MEM_BASE		"FK"
#define VRAM_CQM_CLA_BASE		"C"
#define VRAM_CQM_CLA_TYPE_BASE		"T"
#define VRAM_CQM_CLA_SMF_BASE		"SMF"
#define VRAM_CQM_CLA_COORD_X		"X"
#define VRAM_CQM_CLA_COORD_Y		"Y"
#define VRAM_CQM_CLA_COORD_Z		"Z"
#define VRAM_CQM_BITMAP_BASE		"B"

#define VRAM_NIC_DCB			"DCB"
#define VRAM_NIC_MHOST_MGMT		"MHOST_MGMT"
#define VRAM_NIC_VRAM			"NIC_VRAM"
#define VRAM_NIC_IRQ_VRAM		"NIC_IRQ"

#define VRAM_NIC_MQM			"NM"

#define VRAM_VBS_BASE_IOCB		"BASE_IOCB"
#define VRAM_VBS_EX_IOCB		"EX_IOCB"
#define VRAM_VBS_RXQS_CQE		"RXQS_CQE"

#define VRAM_VBS_VOLQ_MTT		"VOLQ_MTT"
#define VRAM_VBS_VOLQ_MTT_PAGE		"MTT_PAGE"

#define VRAM_OVS_PORT_CONF		"OVS_PORT_CONF"
#define VRAM_OVS_DFX_MGR		"OVS_DFX_MGR"

#define VRAM_VROCE_ENTRY_POOL		"VROCE_ENTRY"
#define VRAM_VROCE_GROUP_POOL		"VROCE_GROUP"
#define VRAM_VROCE_UUID			"VROCE_UUID"
#define VRAM_VROCE_VID			"VROCE_VID"
#define VRAM_VROCE_BASE			"VROCE_BASE"
#define VRAM_VROCE_DSCP			"VROCE_DSCP"
#define VRAM_VROCE_QOS			"VROCE_QOS"
#define VRAM_VROCE_DEV			"VROCE_DEV"
#define VRAM_VROCE_RGROUP_HT_CNT	"RGROUP_CNT"
#define VRAM_VROCE_RACL_HT_CNT		"RACL_CNT"
#define VRAM_VROCE_MQM_ENQC		"VROCE_MQM_ENQC"

#define VRAM_DTOE_NUMA_MEM		"DTOE_NUMA"
#define VRAM_DTOE_CARD_MEM		"DTOE_CARD"
#define VRAM_DTOE_CONN_MEM		"DTOE_CONN"
#define VRAM_DTOE_SUB_LEN		10

#define VRAM_VROCE_MIG_ENTRY_POOL	"VROCE_MIG_ENTRY"
#define VRAM_VROCE_MIG_ENTRY_HT_CNT	"MIG_ENTRY_CNT"

#define VRAM_VBS_DEV_INDEX		"VBS_INDEX"

#define VRAM_NAME_APPLY_LEN 64

#define MPU_OS_HOTREPLACE_FLAG	0x1
struct vram_buf_info {
	char buf_vram_name[VRAM_NAME_APPLY_LEN];
	int use_vram;
};

enum KUP_HOOK_POINT {
	PRE_FREEZE,
	FREEZE_TO_KILL,
	PRE_UPDATE_KERNEL,
	POST_UPDATE_KERNEL,
	UNFREEZE_TO_RUN,
	POST_RUN,
	KUP_HOOK_MAX,
};

#define hi_vram_kalloc(name, size) 0
#define hi_vram_kfree(vaddr, name, size)
#define get_use_vram_flag(void) 0
#define vram_get_kexec_flag(void) 0
#define hi_vram_get_gfp_vram(void) 0

#endif /* VRAM_COMMON_H */

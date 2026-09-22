/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef _ADK_MMU600_H_
#define _ADK_MMU600_H_

#include "../../manager.h"

/**************************************************************************
 *                        Macro                                           *
 **************************************************************************/
/* max stream id num */
#define SEC0_MAX_STREAM_NUM (64)

/* udRWFlag value */
#define SMMU_PTE_AP_EL_RW (0) // EL1 or higer R/W, EL0 none
#define SMMU_PTE_AP_RW (1) // R/W in all EL
#define SMMU_PTE_AP_EL_RO (2) // EL1 or higer RO, EL0 none
#define SMMU_PTE_AP_RO (3) // RO in all EL

/* udMemAttr value */
#define SMMU_PTE_MEMATTR_DEVICE (0)
#define SMMU_PTE_MEMATTR_NM_WB_WA (1)
#define SMMU_PTE_MEMATTR_NC (2)

/* udShare value */
#define SMMU_PAGETABLE_NONSHAREABLE (0) /**<  non share*/
#define SMMU_PAGETABLE_OUTERSHARE (2) /**<  outer share*/
#define SMMU_PAGETABLE_INNERSHARE (3) /**<  inner share*/

#ifndef SMMU_PTE_REQUEST_ST
#define SMMU_PTE_REQUEST_ST

#define RSC_MSG_SMMU_EVENT_ID (6) // commom通道，SMMU EVENT ID
#define CMDQ_OP_TLBI_NSNH_ALL (0x30)
#define ZXDH_SMMU_INVALID_DELTA_TIME	15000	

struct stInvalidTlbCfg {
	u32 cmd;
	u32 scale;
	u32 num;
	u32 TG;
	u32 leaf;
	u32 TTL;
	u32 vmid;
	u32 asid;
	u64 addr;
};

struct rsvMsgSmmuInfo {
	u32 udIsTlbInvalid;
	struct stInvalidTlbCfg stInvalidTlbCfg;
};

struct stPteRequest {
	u64 uddPhyAddr; /* Request physical address */
	u64 uddVirAddr; /* Request virual address */
	u32 udStreamid; // stream id
	u64 uddSize; // 映射地址范围大小
	u32 udRWFlag; /* AP */
	u32 udMemAttr; /* memory attribute */
	u32 udShare; /* share, 0-nonshare, 2-outershare, 3-innershare*/
};

#endif
struct pteRecord {
	u32 udValid;
	u32 udSid;
	u64 uddVa;
	u64 uddPa;
	u64 uddSize;
};

struct stPagetableParam {
	u64 uddPageTablePhyAddr; ///<页表存放区起始地址
	u64 uddPageTableVirAddr;
	u32 udPageTableSize; ///<页表存放区大小

	u64 uddExPageTablePhyAddr; ///<扩展页表存放区起始地址，ddr，只放4K
	u32 udPExPableSize; ///<扩展页表存放区大小，ddr，只放4K

	u32 udL1PageTableNum; ///<L1页表数，每张512B, 对应一个stream，支持最大64G物理地址
	u32 udL2PageTableNum; ///<L2页表数，每张对应1个1G页表的512个2M小页
	u32 udL3PageTableNum; ///<L3页表数，每张对应1个2M页表的512个4K小页
};

struct smmu_pte_address {
	u64 CmaPageMemBasePA;
	u64 CmaPageMemBaseVA;
	u64 CmaMemBaseVA_pte; // init value equal CmaPageMemBaseVA
	struct stPagetableParam tPageTableCfg;
	u64 uddPageTableVirBaseAddr; // init value equal CmaPageMemBaseVA
	u64 uddSmmuMapManageAddr; // manage l2 l3
	struct pteRecord *ptPteRecords;
	struct page *ptCmaPageAddr;
	u64 uddPTETempVirAddr; // 分配8字节空间存储每一次下发PTE的数据，作为中转
	u64 uddPTETempPhyAddr; // 分配8字节空间存储每一次下发PTE的数据，作为中转
	u32 udL1PageTableNum;
	u32 udL2PageTableNum; // l2 ttb 已经申请使用的个数 由 struct t_Map_Manage 管理
	u32 udL3PageTableNum; // l3 ttb 已经申请使用的个数 由 struct t_Map_Manage 管理
	u32 udPteRecordNum;
	u32 udPteFailRecordNum;
	struct dentry *mmu600_dbg_dentry;
	u32 l2d_smmu_l2_offset;
};

int zxdh_smmu_pagetable_init(struct zxdh_sc_dev *dev);
int zxdh_smmu_pagetable_exit(struct zxdh_sc_dev *dev);
int zxdh_smmu_enable_stream_stage2(u32 sid);
int BspMmu600EnStreamBypass(u32 udSid);

int zxdh_smmu_set_pte(struct stPteRequest *ptMmuMmapCfg,
		      struct zxdh_sc_dev *dev);
u32 bspSmmuDeletePTE(u32 udSid, u64 uddVa, struct zxdh_sc_dev *dev);

void zxdh_smmu_use_l3_test(struct stPteRequest *ptMmuMmapCfg,
			   struct zxdh_sc_dev *dev);
void zxdh_smmu_use_l2_test(struct stPteRequest *ptMmuMmapCfg,
			   struct zxdh_sc_dev *dev);
void zxdh_smmu_use_l1_test(struct stPteRequest *ptMmuMmapCfg,
			   struct zxdh_sc_dev *dev);
void zxdh_smmu_dma_l1_l2_to_risc_test(struct stPteRequest *ptMmuMmapCfg,
				      struct zxdh_sc_dev *dev);
int dh_rdma_chan_smmu_invalid_tlb_send(struct zxdh_sc_dev *dev);
#endif

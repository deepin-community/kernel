/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

/**
 * @file        cmdk_mmu600.h
 * @brief       mmu600 sdk对外接口头文件
 * @details     主要包含mmu600初始化、配置、维测等接口
 * @author      陈港文
 * @date        2021-04-27
 * @version     V1.0
 * @copyright   Copyright (c) 2018-2020  中兴通讯有限公司
 **********************************************************************************
 * @attention
 *    - 硬件平台:msc4.0
 *    - 架构支持：arm64
 * @warning
 *  - xxxxx
 *  - xxxxxxx
 * @bug
 *  -
 * @par 修改日志:
 * <table>
 * <tr><th>Date        <th>Version  <th>Author    <th>Description
 * <tr><td>2021/04/27  <td>1.0      <td>陈港文          <td>创建初始版本
 * </table>
 *
 **********************************************************************************
 */

#ifndef _CMDK_MMU600_H_
#define _CMDK_MMU600_H_

#include "cmdk.h"
#include "../../type.h"
#include "adk_mmu600.h"

/**************************************************************************
 *                        Macro                                           *
 **************************************************************************/
#define PAGE_SIZE_4K 0x1000 ///<页表大小4K
#define PAGE_SIZE_2M 0x200000 ///<页表大小2M
#define PAGE_SIZE_1G 0x40000000 ///<页表大小1G

#define SMMU_S1CDMAX_VALUE (1)

// page table mask
#define PAGE_MASK_4K 0xfffffffff000ULL
#define PAGE_MASK_64K 0xffffffff0000ULL
#define PAGE_MASK_1M 0xfffffff00000ULL
#define PAGE_MASK_2M 0xffffffe00000ULL
#define PAGE_MASK_16M 0xffffff000000ULL
#define PAGE_MASK_512M 0xffffe0000000ULL
#define PAGE_MASK_1G 0xffffc0000000ULL

// reverse mask
#define REV_PAGE_MASK_4K 0x0000000fffULL
#define REV_PAGE_MASK_64K 0x000000ffffULL
#define REV_PAGE_MASK_1M 0x00000fffffULL
#define REV_PAGE_MASK_2M 0x00001fffffULL
#define REV_PAGE_MASK_16M 0x0000ffffffULL
#define REV_PAGE_MASK_512M 0x001fffffffULL
#define REV_PAGE_MASK_1G 0x003fffffffULL

/* udRWFlag value */
#define SMMU_PTE_AP_EL_RW \
	(0) ///<struct stPteRequest.udRWFlag配值，EL1 or higer R/W, EL0 none
#define SMMU_PTE_AP_RW (1) ///<struct stPteRequest.udRWFlag配值，R/W in all EL
#define SMMU_PTE_AP_EL_RO \
	(2) ///<struct stPteRequest.udRWFlag配值，EL1 or higer RO, EL0 none
#define SMMU_PTE_AP_RO (3) ///<struct stPteRequest.udRWFlag配值，RO in all EL

/* udMemAttr value */
#define SMMU_PTE_MEMATTR_DEVICE \
	(0) ///<struct stPteRequest.udMemAttr配值，device属性
#define SMMU_PTE_MEMATTR_NM_WB_WA \
	(1) ///<struct stPteRequest.udMemAttr配值，cache属性
#define SMMU_PTE_MEMATTR_NC \
	(2) ///<struct stPteRequest.udMemAttr配值，noarmal nocache属性

/* udShare value */
#define SMMU_PAGETABLE_NONSHAREABLE \
	(0) ///<struct stPteRequest.udShare配值，non share
#define SMMU_PAGETABLE_OUTERSHARE \
	(2) ///<struct stPteRequest.udShare配值，outer share
#define SMMU_PAGETABLE_INNERSHARE \
	(3) ///<struct stPteRequest.udShare配值，inner share

#ifndef SMMU_PTE_REQUEST_ST
#define SMMU_PTE_REQUEST_ST
/**
 * @brief MMU600页表映射参数
 */
struct stPteRequest {
	u32 udStreamid;
	u64 uddPhyAddr; ///<物理地址
	u64 uddVirAddr; ///<虚拟地址
	u64 uddSize; ///<映射地址范围大小
	u32 udRWFlag; ///<访问权限
	u32 udMemAttr; ///<cache属性
	u32 udShare; ///<共享属性
};
#endif
/* udStage value */
#define STE_STAGE_BYPASS (0) ///<T_STE_CFG.udStage配值，bypass模式
#define STE_STAGE_STAGE1ONLY (1) ///<T_STE_CFG.udStage配值，stage1转换模式
#define STE_STAGE_STAGE2ONLY (2) ///<T_STE_CFG.udStage配值，stage2转换模式
#define STE_STAGE_NESTED (3) ///<T_STE_CFG.udStage配值，nested转换模式

enum eSMMUMSGTYPE {
	SMMU_MSG_BYPASS = 0,
	SMMU_MSG_ENABLE,
	SMMU_MSG_DEL_PTE,
	SMMU_MSG_TLB_IPA,
	SMMU_MSG_TLB_SYNC,
};

struct st2RiscMsg {
	enum eSMMUMSGTYPE type;
	u32 udStreamid;
	u64 vaddr; /* delete PTE address */
	u64 uddSize; /* delete PTE address size */
	u64 uddPteL2DAddr; /*Pte base address*/
	u64 uddPteL2DLen; /*pte address length*/
};

#define SMMU_PRINT_LV_DEBUG ((u8)0x01) /**< 默认仅在debug版本显示 */
#define SMMU_PRINT_LV_INFO ((u8)0x02)
#define SMMU_PRINT_LV_ERROR ((u8)0x08)

/**************************************************************************
 *                        Declare                                         *
 **************************************************************************/
u32 mpf_sync_msg_send(u8 type, u8 module_id, u8 *msg, u16 len);
u32 mpf_async_msg_send(u8 type, u8 module_id, u8 *msg, u16 len);
// initialize interface
u32 zxdh_smmu_struct_init(const struct stPagetableParam *ptPgtPara,
			  struct smmu_pte_address *pstPteAddress, struct device *dmadev);
u32 smmuShowPagetableInfo(struct smmu_pte_address *pstPteAddress);
u32 CmdkSysMmuShowPteRecord(u32 udSid, u64 uddVa,
			    struct smmu_pte_address *pstPteAddress);
u64 zxdh_smmu_host_pa_to_l2d_pa(u64 udd_host_pa, struct zxdh_sc_dev *dev);

u32 CmdkSysMmuSetPrintLevel(u32 udPrintLvl);
u8 CmdkSysMmuGetPrintLevel(void);
u32 memset_8byte(u64 *p, u64 data, u64 size);

// pte interface
int zxdh_smmu_mmap(struct stPteRequest *ptPteRequest, struct zxdh_sc_dev *dev);

// reset
u32 CmdkSysMmuCmdTlbSync(void);
u32 CmdkSysMmuCmdTlbCleanByVa(u32 udSid, u32 udSsid, u64 uddVa, u32 udPageLvl);
u32 CmdkSysMmuCmdTlbCleanByAsid(u32 udSid, u32 udSsid);
u32 CmdkSysMmuCmdCdSync(u32 udSid);
u32 CmdkSysMmuCmdTlbCleanByIpa(u32 udSid, u64 uddVa, u32 udPageLvl);
u32 CmdkSysMmuCmdTlbCleanByVmid(u32 udSid);
u32 CmdkSysMmuCmdSteSync(void);
u32 CmdkSysMmuCmdDeletePte(u32 udSid, u64 uddVa, u64 uddSize, u64 uddPteL2DAddr,
			   u64 uddPteL2DLen);

#endif

// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#include "common_define.h"
#include <linux/init.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/slab.h>
#include "hal_smmu.h"
#include "cmdk_mmu600.h"
#include "cmdk_mmu600_inner.h"
#include "pub_print.h"

//#include "../../../../../../../net/msg_chan_driver/msg_chan_pub.h"
//#include "../../../../../msg_chan_driver/msg_chan_pub.h"
/**************************************************************************
 *                        Macro                                           *
 **************************************************************************/
#define MAX_PTE_RECORDS_NUM (2000)

//-------translation table------------------------------
// SID[4:0] 一个SID对应一个PF，共32个PF，32个PF共享这32G，所以需要32套页表
// 32个PF * 每个PF对应32个L1页表项映射32G = 一共需要L1页表项的个数是1024 个
#define SMMU_L1_PER_PT_SIZE \
	(0x100) // 32个pte，共占用32 * 8Byte = 256Byte内存空间，映射32G
#define SMMU_L1_PT_ALIGN_SIZE (0x100) // 0x100 = 256
#define SMMU_L1_PT_NUM (32) // 32套页表
#define SMMU_L1_PT_SIZE (SMMU_L1_PT_NUM * SMMU_L1_PER_PT_SIZE) // 8K = 0x2000

// 32个PF共用同一套L2，动态维护管理
#define SMMU_L2_PER_PT_SIZE \
	(0x1000) // 4k = 0x1000, 每个块表示1G，包含 512个2M 共占用 512*8=4K 内存
#define SMMU_L2_PT_ALIGN_SIZE (0x1000) // 4k = 0x1000
#define SMMU_L2_PT_NUM (32) // 物理上只有32G，共需要32个 512*2M 就可以表示
#define SMMU_L2_PT_SIZE (SMMU_L2_PT_NUM * SMMU_L2_PER_PT_SIZE) // 128k = 0x20000

// 32个PF共用同一套L3，动态维护管理
#define SMMU_L3_PER_PT_SIZE \
	(0x1000) // 4k = 0x1000, 每个块表示2M，包含 512个4K 共占用 512*8=4K 内存
#define SMMU_L3_PT_ALIGN_SIZE (0x1000) // 4k = 0x1000
#define SMMU_L3_PT_NUM (0x3DE) // 0x3DE = 990
#define SMMU_L3_PT_SIZE (SMMU_L3_PT_NUM * SMMU_L3_PER_PT_SIZE) // 3M + 896K

#define SMMU_PT_TOTAL (SMMU_L1_PT_SIZE + SMMU_L2_PT_SIZE + SMMU_L3_PT_SIZE)

// L1要求0x100对齐
// #define PTE_L2D_START_PA          (0x6200B2E800)
// 4k对齐
#define PTE_L2D_START_PA (0x6200630000)

//--------map manage struct------------------------------
#define SMMU_L2_MAP_MANAGE_SIZE (SMMU_L2_PT_NUM * sizeof(struct t_Map_Manage))
#define SMMU_L3_MAP_MANAGE_SIZE (SMMU_L3_PT_NUM * sizeof(struct t_Map_Manage))

/**************************************************************************
 *                         Global Value                                   *
 **************************************************************************/
struct ttbManage {
	u32 udSid;
	u32 udValid;
	u64 uddPhyTTB;
};

// ======================================================================
// init g_ptTtbMng
// self: TtbMmg用来管理L1基地址
// TtbMng用来管理L1的信息，包括L1的基地址、该L1表是否有效、对应的SID
// ======================================================================
static struct ttbManage g_ptTtbMng[SMMU_L1_PT_NUM] = {0}; // manage l1

/**************************************************************************
 *                         Functions                                      *
 **************************************************************************/
static u64 zxdh_smmu_get_ttb(u32 sid, struct smmu_pte_address *pstPteAddress)
{
	return pstPteAddress->CmaPageMemBasePA + sid * SMMU_L1_PER_PT_SIZE;
}

/**************************************************************************
 * 函数名称： zxdh_smmu_get_pte_size
 * 功能描述：
 *           判断是否可以使用块类型的页表项（优先使用块类型的页表项）
 *
 * 输入参数：udd_request_va   申请映射的虚拟地址
 *          udd_request_size 申请映射的空间大小
 *          ud_granule       使用的粒度
 * 输出参数：
 * 返 回 值： ud_final_pte_size
 * 其它说明：
 *         端午节加班ing
 * 修改日期            版本号            修改人
 * -----------------------------------------------
 * 2023/06/23          V1.0          guoll
 ***************************************************************************/
static u32 zxdh_smmu_get_pte_size(u64 udd_request_va, u64 udd_request_pa,
				  u64 udd_request_size, u32 ud_granule)
{
	u32 ud_final_pte_size = PAGE_SIZE_4K;

	switch (ud_granule) {
	case SMMU_CD_TG0_4K: {
		if ((0 == (udd_request_va & REV_PAGE_MASK_1G)) &&
		    (0 == (udd_request_pa & REV_PAGE_MASK_1G)) &&
		    (udd_request_size >= PAGE_SIZE_1G)) {
			ud_final_pte_size = PAGE_SIZE_1G;
		} else {
			if ((0 == (udd_request_va & REV_PAGE_MASK_2M)) &&
			    (0 == (udd_request_pa & REV_PAGE_MASK_2M)) &&
			    (udd_request_size >= PAGE_SIZE_2M)) {
				ud_final_pte_size = PAGE_SIZE_2M;
			}
		}
		break;
	}
	default: {
		break;
	}
	}
	return ud_final_pte_size;
}

/* 把传下来的配置信息填入到 struct smmu_pte_cfg *ptTlbEntryCfg 中 */
static u32 zxdh_smmu_request_to_pte_cfg(const u32 udPTESize,
					const struct stPteRequest *ptPteRequest,
					struct smmu_pte_cfg *ptTlbEntryCfg)
{
	u64 uddRequestPhyAddr = 0;

	/* param check */
	SMMU_POINTER_CHECK(ptTlbEntryCfg);
	SMMU_POINTER_CHECK(ptPteRequest);

	uddRequestPhyAddr = ptPteRequest->uddPhyAddr;

	ptTlbEntryCfg->udExecuteNever = SMMU_PAGETABLE_EXECUTE;
	ptTlbEntryCfg->udShareable = ptPteRequest->udShare;
	ptTlbEntryCfg->udAccessPermission = ptPteRequest->udRWFlag;
	ptTlbEntryCfg->udMemoryAttribute = ptPteRequest->udMemAttr;

	/* 默认设为0 */
	ptTlbEntryCfg->udRACFG = 0;
	ptTlbEntryCfg->udWACFG = 0;
	if (READ_NOALLOCATE ==
	    (READ_NOALLOCATE & ptTlbEntryCfg->udMemoryAttribute)) {
		ptTlbEntryCfg->udRACFG = 3;
	}
	if (WRITE_NOALLOCATE ==
	    (WRITE_NOALLOCATE & ptTlbEntryCfg->udMemoryAttribute)) {
		ptTlbEntryCfg->udWACFG = 3;
	}

	switch (udPTESize) {
	case PAGE_SIZE_4K: {
		ptTlbEntryCfg->uddPABaseAddr = uddRequestPhyAddr & PAGE_MASK_4K;
		ptTlbEntryCfg->udPageType = SMMU_PAGETABLE_PAGESIZE_4KB; /* */
		break;
	}
	case PAGE_SIZE_2M: {
		ptTlbEntryCfg->uddPABaseAddr = uddRequestPhyAddr & PAGE_MASK_2M;
		ptTlbEntryCfg->udPageType = SMMU_PAGETABLE_PAGESIZE_2MB; /* */
		break;
	}
	case PAGE_SIZE_1G: {
		ptTlbEntryCfg->uddPABaseAddr = uddRequestPhyAddr & PAGE_MASK_1G;
		ptTlbEntryCfg->udPageType = SMMU_PAGETABLE_PAGESIZE_1G; /* */
		break;
	}
	default: /* 默认按4k处理 */
	{
		ptTlbEntryCfg->uddPABaseAddr = uddRequestPhyAddr & PAGE_MASK_4K;
		ptTlbEntryCfg->udPageType = SMMU_PAGETABLE_PAGESIZE_4KB; /* */
		break;
	}
	}

	return CMDK_OK;
}

static u64 zxdh_smmu_sram_pagetable_v2p(u64 uddVa,
					struct smmu_pte_address *pstPteAddress)
{
	u64 uddPa = 0;

	if ((pstPteAddress->uddPageTableVirBaseAddr == 0) ||
	    (pstPteAddress->tPageTableCfg.uddPageTablePhyAddr == 0)) {
		return CMDK_ERROR;
	}

	uddPa = pstPteAddress->tPageTableCfg.uddPageTablePhyAddr + uddVa -
		pstPteAddress->uddPageTableVirBaseAddr;

	return uddPa;
}

static u64 zxdh_smmu_sram_pagetable_p2v(u64 uddPa,
					struct smmu_pte_address *pstPteAddress)
{
	u64 uddVa = 0;

	if ((pstPteAddress->uddPageTableVirBaseAddr == 0) ||
	    (pstPteAddress->tPageTableCfg.uddPageTablePhyAddr == 0)) {
		return CMDK_ERROR;
	}

	uddVa = pstPteAddress->uddPageTableVirBaseAddr + uddPa -
		pstPteAddress->tPageTableCfg.uddPageTablePhyAddr;

	return uddVa;
}

/**************************************************************************
 * 函数名称： zxdh_smmu_get_l1_page_base_addr
 * 功能描述： 获取L1 pte base address
 * 输入参数：
 *           u64 uddPgTblAddr  ： sid对应的页表基地址
 *           u64 uddVa  ：  VA
 * 输出参数：
 * 返 回 值：L1 PTE 地址
 * 其它说明：
 *
 * 修改日期            版本号            修改人
 * -----------------------------------------------
 * 2023/05/29        V1.0          guoll
 ***************************************************************************/
static u64 zxdh_smmu_get_l1_descriptor_va(u64 udd_l1_ttb_va, u64 udd_request_va)
{
	return (udd_l1_ttb_va + ((udd_request_va & 0xffc0000000ULL) >> 27));
}

/**************************************************************************
 * 函数名称： zxdh_smmu_get_l1_page_base_addr
 * 功能描述： 获取L2 pte base address，即获取L2 descriptor
 * 输入参数：
 *           u64 uddPgTblAddr  ： sid对应的页表基地址
 *           u64 udd_request_va  ：  VA
 * 输出参数：
 * 返 回 值：L1 PTE 地址 (VA)
 * 其它说明：
 *
 * 修改日期            版本号            修改人
 * -----------------------------------------------
 * 2023/05/29        V1.0          guoll
 ***************************************************************************/
static u64
zxdh_smmu_get_l2_descriptor_va(struct zxdh_sc_dev *dev, u32 sid, u64 udd_request_va,
			       struct smmu_pte_address *pstPteAddress)
{
	u32 i = 0;
	u64 uddLevelMask = 0;
	u32 udLevelOffset = 0;
	u64 udd_l2_nth_ttb_va = 0;
	u64 udd_l2_start_ttb_va = 0;
	struct t_Map_Manage *ptL2MapManage = NULL;
	u32 *pud_used_l2_ttb_num = NULL;
	/* 记录 l2 已申请使用的 ttb 数量 */

	/* check param */
	SMMU_POINTER_CHECK(pstPteAddress);
	SMMU_POINTER_CHECK(pstPteAddress->uddSmmuMapManageAddr);

	// 1G-1:    11 1111 1111 1111 1111 1111 1111 1111
	// 2M-1:               1 1111 1111 1111 1111 1111
	// ~(2M-1): 11 1111 1110 0000 0000 0000 0000 0000
	uddLevelMask = 0x3fe00000ull; /* (1G-1)&(~(2M-1)) */
	udLevelOffset = 18; /* div by 2M, mul 8 */

	/* l2 map manage struct */
	ptL2MapManage =
		(struct t_Map_Manage *)(pstPteAddress->uddSmmuMapManageAddr);
	/* l2 start ttb 的内存起始地址 */
	udd_l2_start_ttb_va = pstPteAddress->uddPageTableVirBaseAddr +
			      SMMU_L1_PT_SIZE +
			      pstPteAddress->l2d_smmu_l2_offset;

	pud_used_l2_ttb_num = &dev->s_udV8NumL2Pta;

	// 先在已用的L2页表中找，是否在已存在的页表中，如果有，就不用再申请新的页表了，直接返回对应页表项的地址
	// L1 的每一个页表项能够映射1G的空间
	/* if the 1G which this va corresponds to has been allocated, find the existing address */
	for (i = 0; i < *pud_used_l2_ttb_num; i++) {
		if (((udd_request_va & PAGE_MASK_1G) ==
		     ptL2MapManage[i].uddMaskedVa) &&
		    ptL2MapManage[i].udMapValid &&
		    (sid == ptL2MapManage[i].udSteamIndex)) {
			break;
		}
	}

	/* if not, allocate 4K space used for L2 page table for this 1G */
	if (i == *pud_used_l2_ttb_num) {
		/* 使用一个新的 L2 ttb */
		if (*pud_used_l2_ttb_num < SMMU_L2_PT_NUM) {
			/* 得到第 n 个L2页表的起始地址  即得到该1G对应的2M页表的基地址 */
			udd_l2_nth_ttb_va =
				udd_l2_start_ttb_va + i * SMMU_L2_PER_PT_SIZE;
		} else {
			return 0;
		}

		ptL2MapManage[i].udMapValid = 1;
		ptL2MapManage[i].udSteamIndex = sid;
		ptL2MapManage[i].uddTTBaseAddr = udd_l2_nth_ttb_va;
		ptL2MapManage[i].uddMaskedVa = udd_request_va & PAGE_MASK_1G;

		*pud_used_l2_ttb_num += 1;
		pstPteAddress->udL2PageTableNum = *pud_used_l2_ttb_num;
	}

	/* 返回第 n 张 l2 ttb 的 pte base address，即获取 l2 descriptor */
	return (ptL2MapManage[i].uddTTBaseAddr +
		(u64)((udd_request_va & uddLevelMask) >> udLevelOffset));
}

static u64 zxdh_smmu_get_l3_descriptor(u32 sid, u64 udd_request_va,
				       struct smmu_pte_address *pstPteAddress)
{
	u32 i = 0;
	u64 uddLevelMask = 0;
	u32 udLevelOffset = 0;
	u64 udd_l3_nth_ttb_va = 0;
	u64 udd_l3_start_ttb_va = 0;
	struct t_Map_Manage *ptL3ManageMap = NULL;
	u32 *pud_used_l3_ttb_num = NULL;
	/* 记录 l3 已申请使用的 ttb 数量 */
	static u32 s_udV8NumL3Pta;

	/* check param */
	SMMU_POINTER_CHECK(pstPteAddress);
	SMMU_POINTER_CHECK(pstPteAddress->uddSmmuMapManageAddr);

	// 2M-1:   1 1111 1111 1111 1111 1111
	// 4K-1:               1111 1111 1111
	//~(4K-1): 1 1111 1111 0000 0000 0000
	uddLevelMask = 0x001ff000ull; /* (2M-1)&(~(4K-1)) */
	udLevelOffset = 9; /* div 4K, mul 8 */

	/* l3 map manage struct */
	ptL3ManageMap =
		(struct t_Map_Manage *)(pstPteAddress->uddSmmuMapManageAddr +
					SMMU_L2_MAP_MANAGE_SIZE);
	/* l3 start ttb 的内存起始地址 */
	udd_l3_start_ttb_va = pstPteAddress->uddPageTableVirBaseAddr +
			      SMMU_L1_PT_SIZE + SMMU_L2_PT_SIZE;

	pud_used_l3_ttb_num = &s_udV8NumL3Pta;

	/* the same logic as get L2 */
	for (i = 0; i < *pud_used_l3_ttb_num; i++) {
		// 此 L3 页表（每块4K）是给哪个 L2 的 2M 使用的
		if (((udd_request_va & PAGE_MASK_2M) ==
		     ptL3ManageMap[i].uddMaskedVa) &&
		    ptL3ManageMap[i].udMapValid &&
		    (sid == ptL3ManageMap[i].udSteamIndex)) {
			break;
		}
	}

	if (i == *pud_used_l3_ttb_num) {
		/* 使用一个新的 l3 ttb */
		if (*pud_used_l3_ttb_num < SMMU_L3_PT_NUM) {
			/* 得到第 n 个 l3 页表的起始地址 即得到该 2M 对应的 4k 页表的基地址 */
			udd_l3_nth_ttb_va =
				udd_l3_start_ttb_va + i * SMMU_L3_PER_PT_SIZE;
		} else {
			return 0;
		}

		ptL3ManageMap[i].udMapValid = 1;
		ptL3ManageMap[i].udSteamIndex = sid;
		ptL3ManageMap[i].uddTTBaseAddr = udd_l3_nth_ttb_va;
		ptL3ManageMap[i].uddMaskedVa = udd_request_va & PAGE_MASK_2M;

		*pud_used_l3_ttb_num += 1;
		pstPteAddress->udL3PageTableNum = *pud_used_l3_ttb_num;
	}
	/* 返回第 n 张 l3 ttb 的 pte base address，即获取 l3 descriptor */
	return (ptL3ManageMap[i].uddTTBaseAddr +
		(u64)((udd_request_va & uddLevelMask) >> udLevelOffset));
}

/**************************************************************************
 * 函数名称： zxdh_smmu_host_pa_to_l2d_pa
 * 功能描述：
 *			  根据偏移，转换成risc_v l2d 上的 pa
 * 输入参数：
 *          udd_host_pa : host上的pa
 *          dev
 * 输出参数：
 * 返 回 值：
 * 其它说明：
 *
 * 修改日期          版本号         修改人
 * -----------------------------------------------
 * 2023/05/29        V1.0          guoll
 ***************************************************************************/
u64 zxdh_smmu_host_pa_to_l2d_pa(u64 udd_host_pa, struct zxdh_sc_dev *dev)
{
	u64 udd_offset = 0;
	u64 udd_l2d_pa = 0;

	/* check param */
	SMMU_POINTER_CHECK(dev);
	SMMU_POINTER_CHECK(dev->pte_address);
	SMMU_POINTER_CHECK(dev->pte_address->CmaPageMemBasePA);

	if (udd_host_pa < dev->pte_address->CmaPageMemBasePA)
		return -1;

	udd_offset = udd_host_pa - dev->pte_address->CmaPageMemBasePA;
	udd_l2d_pa = dev->pte_l2d_startpa + udd_offset;
	return udd_l2d_pa;
}

/**************************************************************************
 * 函数名称： zxdh_smmu_write_l1_page_table_entry
 * 功能描述： 配置 L1 PTE表项
 *			  如果是块类型页表项：
 *					配置最终的输出地址的高位；
 *					配置高位属性
 *					配置低位属性
 *			  如果是页表类型的页表项：
 *					配置下一级页表的地址；
 *					配置为页表类型的页表项
 * 输入参数：
 *
 * 输出参数：
 * 返 回 值：
 * 其它说明：
 *
 * 修改日期          版本号         修改人
 * -----------------------------------------------
 * 2023/05/29        V1.0          guoll
 ***************************************************************************/
static int zxdh_smmu_write_l1_pagetable_entry(
	const u64 udd_l1_descriptor_va,
	const struct smmu_pte_cfg *const ptMmuPageTableEntryCfg,
	struct zxdh_sc_dev *dev)
{
	u64 udd_l2d_pa = 0;
	u64 uddPysicalAddress = 0;
	u64 udd_l1_pte_offset = 0;
	u64 *pull_l1_descriptor_va = NULL;
	u64 *pull_tmp_descriptor_va = NULL;
	u64 udd_tmp_l1_descriptor_value = 0;
	u64 udd_l2d_tmp_l1_descriptor_value = 0;

	struct zxdh_src_copy_dest src_dest = {};

	/* check param */
	SMMU_POINTER_CHECK(dev);
	SMMU_POINTER_CHECK(ptMmuPageTableEntryCfg);

	if (ptMmuPageTableEntryCfg->udPageFormat != PAGE_FORMAT_V8)
		return CMDK_ERROR;

	/* pte base address */
	pull_l1_descriptor_va = (u64 *)udd_l1_descriptor_va;
	*pull_l1_descriptor_va = 0;

	/* physical block base address or next level page table address */
	uddPysicalAddress = ptMmuPageTableEntryCfg->uddPABaseAddr;

	/* block descriptor */
	if (ptMmuPageTableEntryCfg->udPageType == SMMU_PAGETABLE_PAGESIZE_1G) {
		udd_tmp_l1_descriptor_value =
			((uddPysicalAddress &
			  L1_LONG_DESCRIPTOR_BLOCK_PA_MASK) |
			 ((ptMmuPageTableEntryCfg->udExecuteNever
			   << L1_LONG_DESCRIPTOR_BLOCK_XN_POS) &
			  L1_LONG_DESCRIPTOR_BLOCK_XN_MASK) |
			 (((ptMmuPageTableEntryCfg->udAccessPermission)
			   << L1_LONG_DESCRIPTOR_BLOCK_S2AP_POS) &
			  L1_LONG_DESCRIPTOR_BLOCK_S2AP_MASK) |
			 (((0x1) << L1_LONG_DESCRIPTOR_BLOCK_AF_POS) &
			  L1_LONG_DESCRIPTOR_BLOCK_AF_MASK) |
			 (((ptMmuPageTableEntryCfg->udShareable)
			   << L1_LONG_DESCRIPTOR_BLOCK_SH1SH0_POS) &
			  L1_LONG_DESCRIPTOR_BLOCK_SH1SH0_MASK) |
			 (((ptMmuPageTableEntryCfg->udMemoryAttribute)
			   << L1_LONG_DESCRIPTOR_BLOCK_MEMATTR_POS) &
			  L1_LONG_DESCRIPTOR_BLOCK_MEMATTR_MASK) |
			 (L1_LONG_DESCRIPTOR_FOR_BLOCK) |
			 (((ptMmuPageTableEntryCfg->udRACFG)
			   << LONG_DESCRIPTOR_RACFG_POS) &
			  LONG_DESCRIPTOR_RACFG_MASK) |
			 (((ptMmuPageTableEntryCfg->udWACFG)
			   << LONG_DESCRIPTOR_WACFG_POS) &
			  LONG_DESCRIPTOR_WACFG_MASK));

		udd_l2d_tmp_l1_descriptor_value = udd_tmp_l1_descriptor_value;

	}
	/* page table */
	else if (SMMU_PAGETABLE_PAGESIZE_2MB ==
			 ptMmuPageTableEntryCfg->udPageType ||
		 SMMU_PAGETABLE_PAGESIZE_4KB ==
			 ptMmuPageTableEntryCfg->udPageType) {
		udd_tmp_l1_descriptor_value =
			((uddPysicalAddress &
			  L1_LONG_DESCRIPTOR_TABLE_PA_MASK) |
			 (L1_LONG_DESCRIPTOR_FOR_TABLE));

		udd_l2d_pa =
			zxdh_smmu_host_pa_to_l2d_pa(uddPysicalAddress, dev);
		udd_l2d_tmp_l1_descriptor_value =
			((udd_l2d_pa & L1_LONG_DESCRIPTOR_TABLE_PA_MASK) |
			 (L1_LONG_DESCRIPTOR_FOR_TABLE));
	}

	/* default little endian */
	if (ptMmuPageTableEntryCfg->udEndian == SMMU_TT_BIGENDIAN) {
		udd_tmp_l1_descriptor_value =
			uswap_64(udd_tmp_l1_descriptor_value);
		udd_l2d_tmp_l1_descriptor_value =
			uswap_64(udd_l2d_tmp_l1_descriptor_value);
	}

	*pull_l1_descriptor_va = udd_tmp_l1_descriptor_value;

	memset((void *)dev->pte_address->uddPTETempVirAddr, 0, 8);
	pull_tmp_descriptor_va = (u64 *)dev->pte_address->uddPTETempVirAddr;
	*pull_tmp_descriptor_va = udd_l2d_tmp_l1_descriptor_value;

	udd_l1_pte_offset =
		udd_l1_descriptor_va - dev->pte_address->CmaPageMemBaseVA;

	/* cpy data from host to l2d */
	src_dest.src = dev->pte_address->uddPTETempPhyAddr;
	src_dest.len = 8;
	src_dest.dest = dev->pte_l2d_startpa + udd_l1_pte_offset;

	return dev->cqp->process_config_pte_table(dev, src_dest);
}

static int zxdh_smmu_write_l2_pagetable_entry(
	u32 sid, const u64 udd_l2_descriptor_va,
	const struct smmu_pte_cfg *const ptMmuPageTableEntryCfg,
	struct zxdh_sc_dev *dev)
{
	u64 uddPysicalAddress = 0;
	u64 udd_l2_descriptor_value = 0;
	u64 l2d_l2_descriptor_offset = 0;
	u64 *pull_tmp_l2_descriptor_va = NULL;
	u64 *pull_to_l2d_descriptor_va = NULL;
	u64 udd_l2d_l2_descriptor_value = 0;

	static u64 dma_to_l2d_count;

	struct zxdh_src_copy_dest src_dest = {};

	/* param check */
	SMMU_POINTER_CHECK(dev);
	SMMU_POINTER_CHECK(ptMmuPageTableEntryCfg);
	SMMU_POINTER_CHECK(dev->pte_address->uddPTETempVirAddr);

	if (ptMmuPageTableEntryCfg->udPageFormat != PAGE_FORMAT_V8)
		return CMDK_ERROR;

	/* page table base address */
	pull_tmp_l2_descriptor_va = (u64 *)udd_l2_descriptor_va;
	*pull_tmp_l2_descriptor_va = 0;

	/* block base physical address, or next level page table base address */
	uddPysicalAddress = ptMmuPageTableEntryCfg->uddPABaseAddr;

	/* block descriptor */
	if (ptMmuPageTableEntryCfg->udPageType == SMMU_PAGETABLE_PAGESIZE_2MB) {
		udd_l2_descriptor_value =
			((uddPysicalAddress &
			  L2_LONG_DESCRIPTOR_BLOCK_PA_MASK) |
			 ((ptMmuPageTableEntryCfg->udExecuteNever
			   << L2_LONG_DESCRIPTOR_BLOCK_XN_POS) &
			  L2_LONG_DESCRIPTOR_BLOCK_XN_MASK) |
			 (((ptMmuPageTableEntryCfg->udAccessPermission)
			   << L2_LONG_DESCRIPTOR_BLOCK_S2AP_POS) &
			  L2_LONG_DESCRIPTOR_BLOCK_S2AP_MASK) |
			 (((0x1) << L2_LONG_DESCRIPTOR_BLOCK_AF_POS) &
			  L2_LONG_DESCRIPTOR_BLOCK_AF_MASK) |
			 (((ptMmuPageTableEntryCfg->udShareable)
			   << L2_LONG_DESCRIPTOR_BLOCK_SH1SH0_POS) &
			  L2_LONG_DESCRIPTOR_BLOCK_SH1SH0_MASK) |
			 (((ptMmuPageTableEntryCfg->udMemoryAttribute)
			   << L2_LONG_DESCRIPTOR_BLOCK_MEMATTR_POS) &
			  L2_LONG_DESCRIPTOR_BLOCK_MEMATTR_MASK) |
			 (L2_LONG_DESCRIPTOR_FOR_BLOCK) |
			 (((ptMmuPageTableEntryCfg->udRACFG)
			   << LONG_DESCRIPTOR_RACFG_POS) &
			  LONG_DESCRIPTOR_RACFG_MASK) |
			 (((ptMmuPageTableEntryCfg->udWACFG)
			   << LONG_DESCRIPTOR_WACFG_POS) &
			  LONG_DESCRIPTOR_WACFG_MASK));

		udd_l2d_l2_descriptor_value = udd_l2_descriptor_value;
	}
	/* page table */
	else if (SMMU_PAGETABLE_PAGESIZE_4KB ==
		 ptMmuPageTableEntryCfg->udPageType) {
		udd_l2_descriptor_value = ((uddPysicalAddress &
					    L2_LONG_DESCRIPTOR_TABLE_PA_MASK) |
					   (L2_LONG_DESCRIPTOR_FOR_TABLE));

		udd_l2d_l2_descriptor_value = (
			// 新版本方案
			(uddPysicalAddress & 0x3FFFFFFFFF) // bit[37:0]
			| ((sid & 0xFULL) << 42) // bit[46:42]
			| (1ULL << 47) // bit[51:47]
			| (L2_LONG_DESCRIPTOR_FOR_TABLE));
	}

	/* default little endian */
	if (ptMmuPageTableEntryCfg->udEndian == SMMU_TT_BIGENDIAN) {
		udd_l2_descriptor_value = uswap_64(udd_l2_descriptor_value);
		udd_l2d_l2_descriptor_value =
			uswap_64(udd_l2d_l2_descriptor_value);
	}

	*pull_tmp_l2_descriptor_va = udd_l2_descriptor_value;

	memset((void *)dev->pte_address->uddPTETempVirAddr, 0, 8);
	pull_to_l2d_descriptor_va = (u64 *)dev->pte_address->uddPTETempVirAddr;
	*pull_to_l2d_descriptor_va = udd_l2d_l2_descriptor_value;

	dma_to_l2d_count++;

	// =======================================================================
	// 计算偏移量
	// =======================================================================
	l2d_l2_descriptor_offset =
		udd_l2_descriptor_va - dev->pte_address->CmaPageMemBaseVA;

	/* cpy data from host to l2d */
	src_dest.src = dev->pte_address->uddPTETempPhyAddr;
	src_dest.len = 8;
	src_dest.dest = dev->pte_l2d_startpa + l2d_l2_descriptor_offset;
	return dev->cqp->process_config_pte_table(dev, src_dest);
}

static int zxdh_smmu_write_l3_pagetable_entry(
	const u64 udd_l3_descriptor_va,
	const struct smmu_pte_cfg *const ptMmuPageTableEntryCfg,
	struct smmu_pte_address *pstPteAddress)
{
	u64 uddPysicalAddress = 0;
	u64 *pull_l3_descriptor_va = NULL;
	u64 udd_tmp_l3_descriptor_value = 0;

	/* param check */
	SMMU_POINTER_CHECK(pstPteAddress);
	SMMU_POINTER_CHECK(ptMmuPageTableEntryCfg);

	if (ptMmuPageTableEntryCfg->udPageFormat != PAGE_FORMAT_V8)
		return CMDK_ERROR;

	/* pte address */
	pull_l3_descriptor_va = (u64 *)udd_l3_descriptor_va;
	*pull_l3_descriptor_va = 0;

	uddPysicalAddress = ptMmuPageTableEntryCfg->uddPABaseAddr;

	if (ptMmuPageTableEntryCfg->udPageType == SMMU_PAGETABLE_PAGESIZE_4KB) {
		udd_tmp_l3_descriptor_value =
			((uddPysicalAddress &
			  L3_LONG_DESCRIPTOR_BLOCK_PA_MASK) |
			 ((ptMmuPageTableEntryCfg->udExecuteNever
			   << L3_LONG_DESCRIPTOR_BLOCK_XN_POS) &
			  L3_LONG_DESCRIPTOR_BLOCK_XN_MASK) |
			 (((ptMmuPageTableEntryCfg->udAccessPermission)
			   << L3_LONG_DESCRIPTOR_BLOCK_S2AP_POS) &
			  L3_LONG_DESCRIPTOR_BLOCK_S2AP_MASK) |
			 (((0x1) << L3_LONG_DESCRIPTOR_BLOCK_AF_POS) &
			  L3_LONG_DESCRIPTOR_BLOCK_AF_MASK) |
			 (((ptMmuPageTableEntryCfg->udShareable)
			   << L3_LONG_DESCRIPTOR_BLOCK_SH1SH0_POS) &
			  L3_LONG_DESCRIPTOR_BLOCK_SH1SH0_MASK) |
			 (((ptMmuPageTableEntryCfg->udMemoryAttribute)
			   << L3_LONG_DESCRIPTOR_BLOCK_MEMATTR_POS) &
			  L3_LONG_DESCRIPTOR_BLOCK_MEMATTR_MASK) |
			 (L3_LONG_DESCRIPTOR_FOR_PAGE) |
			 (((ptMmuPageTableEntryCfg->udRACFG)
			   << LONG_DESCRIPTOR_RACFG_POS) &
			  LONG_DESCRIPTOR_RACFG_MASK) |
			 (((ptMmuPageTableEntryCfg->udWACFG)
			   << LONG_DESCRIPTOR_WACFG_POS) &
			  LONG_DESCRIPTOR_WACFG_MASK));
	}

	/* default little endian */
	if (ptMmuPageTableEntryCfg->udEndian == SMMU_TT_BIGENDIAN) {
		udd_tmp_l3_descriptor_value =
			uswap_64(udd_tmp_l3_descriptor_value);
	}

	*pull_l3_descriptor_va = udd_tmp_l3_descriptor_value;

	return CMDK_OK;
}

static int zxdh_smmu_set_l1_pte_entry(u64 udd_l1_descriptor_va,
				      struct smmu_pte_cfg *ptTlbEntryCfg,
				      struct zxdh_sc_dev *dev)
{
	SMMU_POINTER_CHECK(dev);
	SMMU_POINTER_CHECK(ptTlbEntryCfg);

	return zxdh_smmu_write_l1_pagetable_entry(udd_l1_descriptor_va, ptTlbEntryCfg,
					   dev);
}

static int zxdh_smmu_set_l2_pte_entry(u64 udd_l1_descriptor_va,
				      u64 udd_l2_descriptor_va, u32 sid,
				      struct smmu_pte_cfg *ptTlbEntryCfg,
				      struct zxdh_sc_dev *dev)
{
    int ret = CMDK_OK;
	SMMU_POINTER_CHECK(ptTlbEntryCfg);

	/* write L2 block descriptor */
	ret = zxdh_smmu_write_l2_pagetable_entry(sid, udd_l2_descriptor_va,
					   ptTlbEntryCfg, dev);
    if (ret != CMDK_OK) {
        pr_err("[zxdh_rdma] %s[%d]: smmu_write_l2_pagetable_entry failed! ret=%d\n", __func__, __LINE__, ret);
        return ret;
    }

	/* create Level1 page table config struct, get L2 pagetable base phyaddr */
	ptTlbEntryCfg->uddPABaseAddr = zxdh_smmu_sram_pagetable_v2p(
		udd_l2_descriptor_va, dev->pte_address);
	if (ptTlbEntryCfg->uddPABaseAddr == 0)
		return CMDK_ERROR;

	/* write L1 page table entry */
	ret = zxdh_smmu_write_l1_pagetable_entry(udd_l1_descriptor_va, ptTlbEntryCfg,
					   dev);
    if (ret != CMDK_OK) {
        pr_err("[zxdh_rdma] %s[%d]: smmu_write_l1_pagetable_entry failed! ret=%d\n", __func__, __LINE__, ret);
        return ret;
    }
    return ret;
}

static int zxdh_smmu_set_l3_pte_entry(u64 udd_l1_descriptor_va,
				      u64 udd_l2_descriptor_va,
				      u64 udd_l3_descriptor_va, u64 sid,
				      u64 udd_request_va,
				      struct smmu_pte_cfg *ptTlbEntryCfg,
				      struct zxdh_sc_dev *dev)
{
    int ret = CMDK_OK;
	SMMU_POINTER_CHECK(dev);
	SMMU_POINTER_CHECK(ptTlbEntryCfg);

	/* write L3 page table descriptor */
	ret = zxdh_smmu_write_l3_pagetable_entry(udd_l3_descriptor_va, ptTlbEntryCfg,
					   dev->pte_address);
    if (ret != CMDK_OK) {
        pr_err("[zxdh_rdma] %s[%d]: smmu_write_l3_pagetable_entry failed! ret=%u\n", __func__, __LINE__, ret);
        return ret;
    }

    /* structure L2 page table descriptor config */
	ptTlbEntryCfg->uddPABaseAddr = zxdh_smmu_sram_pagetable_v2p(
		udd_l3_descriptor_va, dev->pte_address);

	/* write L2 page table descriptor */
	// 因为L2 PTE中要写L3页表的基地址，所以，这里应该拿L3页表地址算L2 PTE偏移
	ret = zxdh_smmu_write_l2_pagetable_entry(sid, udd_l2_descriptor_va,
					   ptTlbEntryCfg, dev);
    if (ret != CMDK_OK) {
        pr_err("[zxdh_rdma] %s[%d]: smmu_write_l2_pagetable_entry failed! ret=%u\n", __func__, __LINE__, ret);
        return ret;
    }

    /* structure L1 page table descriptor config */
	ptTlbEntryCfg->uddPABaseAddr = zxdh_smmu_sram_pagetable_v2p(
		udd_l2_descriptor_va, dev->pte_address);

	/* write L1 page table descriptor */
	return zxdh_smmu_write_l1_pagetable_entry(udd_l1_descriptor_va, ptTlbEntryCfg,
					   dev);
}

static int zxdh_smmu_set_pte_entry(u64 udd_l1_ttb_va, u64 udd_request_va,
				   u64 udd_request_pa, u32 sid,
				   struct smmu_pte_cfg *ptTlbEntryCfg,
				   struct zxdh_sc_dev *dev)
{
    int ret = CMDK_OK;
	u64 udd_l1_descriptor_va = 0;
	u64 udd_l2_descriptor_va = 0;
	u64 udd_l3_descriptor_va = 0;

	SMMU_POINTER_CHECK(dev);
	SMMU_POINTER_CHECK(ptTlbEntryCfg);

	if (dev->pte_address->uddPageTableVirBaseAddr == 0)
	{
		pr_err("[zxdh_rdma] dev->pte_address->uddPageTableVirBaseAddr == 0\n");
		return CMDK_ERROR;

	}
		
	switch (ptTlbEntryCfg->udPageType) {
	case SMMU_PAGETABLE_PAGESIZE_4KB: {
		udd_l3_descriptor_va = zxdh_smmu_get_l3_descriptor(
			sid, udd_request_va, dev->pte_address);
		udd_l2_descriptor_va = zxdh_smmu_get_l2_descriptor_va(
			dev, sid, udd_request_va, dev->pte_address);
		udd_l1_descriptor_va = zxdh_smmu_get_l1_descriptor_va(
			udd_l1_ttb_va, udd_request_va);

		if (!udd_l3_descriptor_va || !udd_l2_descriptor_va ||
		    !udd_l1_descriptor_va) {
			pr_err("[zxdh_rdma] udd_l3_descriptor_va|udd_l2_descriptor_va|udd_l1_descriptor_va == 0\n");
			return CMDK_ERROR;
		}

		ret = zxdh_smmu_set_l3_pte_entry(udd_l1_descriptor_va,
					   udd_l2_descriptor_va,
					   udd_l3_descriptor_va, sid,
					   udd_request_va, ptTlbEntryCfg, dev);
		break;
	}
	case SMMU_PAGETABLE_PAGESIZE_2MB: {
		udd_l3_descriptor_va = 0;
		udd_l2_descriptor_va = zxdh_smmu_get_l2_descriptor_va(
			dev, sid, udd_request_va, dev->pte_address);
		udd_l1_descriptor_va = zxdh_smmu_get_l1_descriptor_va(
			udd_l1_ttb_va, udd_request_va);

		if (!udd_l2_descriptor_va || !udd_l1_descriptor_va)
		{
			pr_err("[zxdh_rdma] udd_l2_descriptor_va|udd_l1_descriptor_va == 0\n");
			return CMDK_ERROR;
		}
			
		ret = zxdh_smmu_set_l2_pte_entry(udd_l1_descriptor_va,
					   udd_l2_descriptor_va, sid,
					   ptTlbEntryCfg, dev);
		break;
	}
	case SMMU_PAGETABLE_PAGESIZE_1G: {
		udd_l3_descriptor_va = 0;
		udd_l2_descriptor_va = 0;
		udd_l1_descriptor_va = zxdh_smmu_get_l1_descriptor_va(
			udd_l1_ttb_va, udd_request_va);

		if (!udd_l1_descriptor_va){
			pr_err("[zxdh_rdma] udd_l1_descriptor_va == 0\n");
			return CMDK_ERROR;
		}

		ret = zxdh_smmu_set_l1_pte_entry(udd_l1_descriptor_va, ptTlbEntryCfg,
					   dev);
		break;
	}
	default: {
		return CMDK_ERROR;
	}
	}

	return ret;
}

u32 smmuShowPagetableInfo(struct smmu_pte_address *pstPteAddress)
{
	SMMU_PRINT(
		PM_INFO,
		"pagetable info: -------------------------------------------------------------------\n");
	SMMU_PRINT(PM_INFO, "pagetable config.uddPageTablePhyAddr  = 0x%llx\n",
		   pstPteAddress->tPageTableCfg.uddPageTablePhyAddr);
	SMMU_PRINT(PM_INFO, "pagetable config.uddPageTableVirAddr  = 0x%llx\n",
		   pstPteAddress->tPageTableCfg.uddPageTableVirAddr);
	SMMU_PRINT(PM_INFO, "pagetable config.udPageTableSize  = 0x%x\n",
		   pstPteAddress->tPageTableCfg.udPageTableSize);
	SMMU_PRINT(PM_INFO,
		   "pagetable config.uddExPageTablePhyAddr  = 0x%llx\n",
		   pstPteAddress->tPageTableCfg.uddExPageTablePhyAddr);
	SMMU_PRINT(PM_INFO, "pagetable config.udPExPableSize  = 0x%x\n",
		   pstPteAddress->tPageTableCfg.udPExPableSize);
	SMMU_PRINT(PM_INFO, "max L1 pagetable num = %d, used = %d\n",
		   SMMU_L1_PT_NUM, pstPteAddress->udL1PageTableNum);
	SMMU_PRINT(PM_INFO, "max L2 pagetable num = %d, used = %d\n",
		   SMMU_L2_PT_NUM, pstPteAddress->udL2PageTableNum);
	SMMU_PRINT(PM_INFO, "max L3 pagetable num = %d, used = %d\n",
		   SMMU_L3_PT_NUM, pstPteAddress->udL3PageTableNum);
	SMMU_PRINT(
		PM_INFO,
		"pte records num = %d, fail record = %d, max capacity = %d\n",
		pstPteAddress->udPteRecordNum,
		pstPteAddress->udPteFailRecordNum, MAX_PTE_RECORDS_NUM);

	return CMDK_OK;
}
EXPORT_SYMBOL(smmuShowPagetableInfo);

u32 CmdkSysMmuShowPteRecord(u32 udSid, u64 uddVa,
			    struct smmu_pte_address *pstPteAddress)
{
	u32 i = 0;
	u64 uddVaTmp;
	u64 uddTTBAddr;

	for (; i < pstPteAddress->udPteRecordNum; i++) {
		if (pstPteAddress->ptPteRecords[i].udValid) {
			// print all records
			if (uddVa == 0xffffffffffffffff) {
				uddVaTmp = pstPteAddress->ptPteRecords[i].uddVa;
			} else if (uddVa >= pstPteAddress->ptPteRecords[i]
						    .uddVa &&
				   uddVa < (pstPteAddress->ptPteRecords[i].uddVa +
					    pstPteAddress->ptPteRecords[i]
						    .uddSize)) {
				uddVaTmp = uddVa;
			} else {
				continue;
			}

			uddTTBAddr = zxdh_smmu_get_ttb(udSid, pstPteAddress);
			if (uddTTBAddr == CMDK_ERROR || uddTTBAddr == 0)
				return CMDK_ERROR;
			//zxdh_smmu_get_l1_descriptor_va(
			//	zxdh_smmu_sram_pagetable_p2v(uddTTBAddr,
			//				     pstPteAddress),
			//	uddVaTmp);
			if (pstPteAddress->ptPteRecords[i].uddSize ==
			    PAGE_SIZE_2M) {
				//zxdh_smmu_get_l2_descriptor_va(dev, udSid, uddVaTmp,
				//			       pstPteAddress);这里没有用到，暂时注释掉
			}

			if (pstPteAddress->ptPteRecords[i].uddSize ==
			    PAGE_SIZE_4K) {
				//zxdh_smmu_get_l2_descriptor_va(dev, udSid, uddVaTmp,
				//			       pstPteAddress);这里没有用到，暂时注释掉

				zxdh_smmu_get_l3_descriptor(udSid, uddVaTmp,
							    pstPteAddress);
			}
		}
	}
	return CMDK_OK;
}
EXPORT_SYMBOL(CmdkSysMmuShowPteRecord);

struct zxdh_smmu_host_risc_msgs {
	u32 sid;
	u32 va;
};

/**************************************************************************
 * 函数名称： zxdh_smmu_mmap
 * 功能描述： 在host上
 *           写入pte，实现smmu虚实地址映射
 * 输入参数：ptPteRequest 地址映射信息
 *          dev 设备信息
 * 输出参数:
 * 返 回 值： CMDK_OK / CMDK_ERROR
 * 其它说明：
 *
 * 修改日期            版本号            修改人
 * -----------------------------------------------
 * 2023/04/26        V1.0          guoll
 ***************************************************************************/
int zxdh_smmu_mmap(struct stPteRequest *ptPteRequest, struct zxdh_sc_dev *dev)
{
	u32 ret = 0;
	u32 ud_pte_size = 0;
	u64 udd_l1_ttb_pa = 0;
	u64 udd_request_va = 0;
	u64 udd_request_pa = 0;
	u64 udd_request_size = 0;
	struct smmu_pte_cfg tTlbEntryCfg = { 0 };
	u32 ud_mmap_cnt = 0;

	SMMU_POINTER_CHECK(dev);
	SMMU_POINTER_CHECK(ptPteRequest);

	udd_request_va = ptPteRequest->uddVirAddr;
	udd_request_pa = ptPteRequest->uddPhyAddr;
	udd_request_size = ptPteRequest->uddSize;

	if ((udd_request_pa & REV_PAGE_MASK_4K) ||
	    (udd_request_va & REV_PAGE_MASK_4K) ||
	    (udd_request_size & REV_PAGE_MASK_4K)) {
		return CMDK_ERROR;
	}

	tTlbEntryCfg.udEndian = SMMU_TT_LITTLEENDIAN; /* endian cfg */
	tTlbEntryCfg.udPageFormat = PAGE_FORMAT_V8;

	/* pa */
	udd_l1_ttb_pa =
		zxdh_smmu_get_ttb(ptPteRequest->udStreamid, dev->pte_address);
	if (udd_l1_ttb_pa == CMDK_ERROR)
		return CMDK_ERROR;

	while (udd_request_size > 0) {
		ud_mmap_cnt++;

		// if (10 == ud_mmap_cnt)
		// {
		//     g_ucMmu600PrintModuleId = 8;
		// }

		// 判断是否可以使用块类型的页表项（优先使用块类型的页表项）
		// 1G 2M 4k
		ud_pte_size = zxdh_smmu_get_pte_size(udd_request_va,
						     udd_request_pa,
						     udd_request_size,
						     SMMU_CD_TG0_4K);

		// ud_pte_size = PAGE_SIZE_4K;

		zxdh_smmu_request_to_pte_cfg(ud_pte_size, ptPteRequest,
					     &tTlbEntryCfg);

		ret = zxdh_smmu_set_pte_entry(
						    zxdh_smmu_sram_pagetable_p2v(udd_l1_ttb_pa, dev->pte_address),
						    udd_request_va, udd_request_pa,
						    ptPteRequest->udStreamid, &tTlbEntryCfg, dev);

		if (ret!=0) {
            pr_err("[zxdh_rdma]: %s[%d] smmu_set_pte_entry failed! ret=%d\n", __func__, __LINE__, ret);
			return CMDK_ERROR;
        }

		udd_request_va += ud_pte_size;
		udd_request_pa += ud_pte_size;
		ptPteRequest->uddPhyAddr = udd_request_pa;
		if (udd_request_size < ud_pte_size) {
			/* avoid negative value */
			udd_request_size = 0;
		} else {
			udd_request_size -= ud_pte_size;
		}
	}
#ifndef BSP_IS_PC_UT
	wmb();
#endif

	return CMDK_OK;
}
EXPORT_SYMBOL(zxdh_smmu_mmap);

/**************************************************************************
 * 函数名称： zxdh_smmu_struct_init
 * 功能描述： 初始化mmu600页表相关数据结构
 *
 * 输入参数： struct stPagetableParam *ptPgtPara : 页表初始化参数
 *            struct zxdh_sc_dev *
 * 输出参数：
 * 返 回 值： CMDK_OK / CMDK_ERROR
 * 其它说明：
 * 修改日期            版本号            修改人
 * -----------------------------------------------
 * 2023/04/26        V1.0          guoll
 ***************************************************************************/
u32 zxdh_smmu_struct_init(const struct stPagetableParam *ptPgtPara,
			  struct smmu_pte_address *pstPteAddress, struct device *dmadev)
{
	void *pddr = NULL;
	u32 udSize = 0;
	u32 udL1PtIndex = 0;
	u32 udPageTableSize = 0;

	SMMU_POINTER_CHECK(ptPgtPara);
	SMMU_POINTER_CHECK(pstPteAddress);

	// ========== ========== ========== ==========
	// 页表初始化参数值校验
	// ========== ========== ========== ==========
	if (ptPgtPara->udPageTableSize == 0 ||
	    ptPgtPara->udL1PageTableNum == 0 ||
	    ptPgtPara->udL2PageTableNum == 0 ||
	    ptPgtPara->udL3PageTableNum == 0) {
		return CMDK_ERROR;
	}

	udPageTableSize = ptPgtPara->udL1PageTableNum * SMMU_L1_PER_PT_SIZE +
			  ptPgtPara->udL2PageTableNum * SMMU_L2_PER_PT_SIZE +
			  ptPgtPara->udL3PageTableNum * SMMU_L3_PER_PT_SIZE;
	if (udPageTableSize > ptPgtPara->udPageTableSize)
		return CMDK_ERROR;

	MEMCPY(&(pstPteAddress->tPageTableCfg), sizeof(struct stPagetableParam),
	       ptPgtPara, sizeof(struct stPagetableParam));

	if (pstPteAddress->CmaPageMemBaseVA == 0) {
		// ========== ========== ========== ==========
		// use reserve mem
		// ========== ========== ========== ==========

		// self: 我的理解，这个是自己打桩测试，正是代码不需要走这个分支
		SMMU_POINTER_CHECK(ptPgtPara->uddPageTablePhyAddr);

		// Mmap page table space
		pddr = (void *)ioremap(ptPgtPara->uddPageTablePhyAddr,
				       ptPgtPara->udPageTableSize);


		if(!pddr)
			return CMDK_ERROR;

		memset_8byte(pddr, 0, ptPgtPara->udPageTableSize);

		pstPteAddress->uddPageTableVirBaseAddr = (u64)pddr;
	} else {
		// ========== ========== ========== ==========
		// use cma mem
		// ========== ========== ========== ==========

		// ========== ========== ========== ==========
		// 对齐待定  self: 怎么对齐？这里暂时先注销
		// ========== ========== ========== ==========
		if (ptPgtPara->uddPageTablePhyAddr &
		    (SMMU_L1_PT_ALIGN_SIZE - 1)) {
			return CMDK_ERROR;
		}

		pstPteAddress->uddPageTableVirBaseAddr =
			pstPteAddress->CmaPageMemBaseVA;
	}

	// ========== ========== ========== ==========
	// allocate g_uddSmmuMapManageAddr
	// T_MAP_MANGE共有1+512个，分别用来记录1个同一L1下的L2的首地址，512个同一L2下的L3的首地址。
	// ========== ========== ========== ==========
	udSize = SMMU_L2_MAP_MANAGE_SIZE + SMMU_L3_MAP_MANAGE_SIZE;
	pstPteAddress->uddSmmuMapManageAddr = (u64)kmalloc(udSize, GFP_KERNEL);
	SMMU_POINTER_CHECK(pstPteAddress->uddSmmuMapManageAddr);
	MEMSET((void *)pstPteAddress->uddSmmuMapManageAddr, udSize, 0, udSize);

	// ========== ========== ========== ==========
	// allocate g_ptPteRecords
	// ========== ========== ========== ==========
	udSize = sizeof(struct pteRecord) * MAX_PTE_RECORDS_NUM;
	pstPteAddress->ptPteRecords =
		(struct pteRecord *)kmalloc(udSize, GFP_KERNEL);
	SMMU_POINTER_CHECK(pstPteAddress->ptPteRecords);
	MEMSET(pstPteAddress->ptPteRecords, udSize, 0, udSize);

	// ========== ========== ========== ========== =====
	// 分配8字节空间存储每一次下发PTE的数据，作为中转
	// ========== ========== ========== ========== =====
	// dma对源地址有对齐要求，必须32byte对齐
	// kmalloc申请到的va是根据传入的申请大小决定对齐的
	pstPteAddress->uddPTETempVirAddr =
		(u64)dma_alloc_coherent(dmadev, SMMU_L1_PER_PT_SIZE * 4,
							&pstPteAddress->uddPTETempPhyAddr, GFP_KERNEL);
	SMMU_POINTER_CHECK(pstPteAddress->uddPTETempVirAddr);
	MEMSET((void *)pstPteAddress->uddPTETempVirAddr,
	       SMMU_L1_PER_PT_SIZE * 4, 0, SMMU_L1_PER_PT_SIZE * 4);

	// 这里只负责把用到的TTB配置好，具体哪个sid使用，在cmdk进行配置，即由用户自己根据业务需求自己配置
	// 只需要把L1的TTB配置了就可以了，因为L1是确定的，L2 L3共用一份
	for (udL1PtIndex = 0; udL1PtIndex < SMMU_L1_PT_NUM; udL1PtIndex++) {
		g_ptTtbMng[udL1PtIndex].uddPhyTTB =
			(pstPteAddress->tPageTableCfg.uddPageTablePhyAddr +
			 udL1PtIndex * SMMU_L1_PER_PT_SIZE);
	}

	return CMDK_OK;
}

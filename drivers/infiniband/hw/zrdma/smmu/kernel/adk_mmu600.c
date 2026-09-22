// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#include <linux/init.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/slab.h>

#define USE_CMA
#ifdef USE_CMA
//#include <linux/dma-contiguous.h>
#include <linux/ioport.h>
#include <linux/io.h>
#endif

#include "cmdk_mmu600.h"
#include "adk_mmu600.h"
#include "ioctl_mmu600.h"

#include "common_define.h"
#include "../../main.h"

/**************************************************************************
 *                        Macro                                           *
 **************************************************************************/
#define CMA_PAGE_COUNT (16 * 1024) // 64M, 16*1024*4k

//-------structures------------------------------------

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
//#define PTE_L2D_START_PA             (0x6200B2F000)
#define PTE_L2D_START_PA (0x6200630000)

/**************************************************************************
 *                         Functions                                      *
 **************************************************************************/
int BspMmu600EnStreamBypass(u32 udSid)
{
	//需要向risc-v的smmu驱动发送命令,TODO

	return CMDK_OK;
}
EXPORT_SYMBOL(BspMmu600EnStreamBypass);

/**************************************************************************
 * 函数名称： zxdh_smmu_dma_l1_l2_to_risc_test
 * 功能描述： 在host上调用,通过dma模块把建立好的L1和L2搬移到L2D中
 *
 * 输入参数： struct zxdh_sc_dev *ptMmuMmapCfg
 * 输出参数：
 * 返 回 值： CMDK_OK / CMDK_ERROR
 * 其它说明：测试接口
 *
 * 修改日期          版本号         修改人
 * -----------------------------------------------
 * 2023/07/03        V1.0          guoll
 ***************************************************************************/
void zxdh_smmu_dma_l1_l2_to_risc_test(struct stPteRequest *ptMmuMmapCfg,
				      struct zxdh_sc_dev *dev)
{
	static int s_count;
	struct zxdh_src_copy_dest src_dest = {};

	memset((void *)dev->pte_address->uddPTETempVirAddr, 0, 8);

	//cpy data from host to l2d
	src_dest.src = dev->pte_address->uddPTETempPhyAddr;
	src_dest.len = 8;
	src_dest.dest = dev->pte_l2d_startpa + SMMU_L1_PT_SIZE +
			s_count * 8; //	dev->pte_l2d_startpa

	*(u64 *)dev->pte_address->uddPTETempVirAddr = (++s_count);

	dev->cqp->process_config_pte_table(dev, src_dest);
}

/**************************************************************************
 * 函数名称： zxdh_smmu_use_l1_test
 * 功能描述： 在host上调用，通过dma模块出发smmu进行查表，测试L1的转换功能
 *
 * 输入参数： struct zxdh_sc_dev *ptMmuMmapCfg
 * 输出参数：
 * 返 回 值： CMDK_OK / CMDK_ERROR
 * 其它说明：测试接口
 *
 * 修改日期          版本号         修改人
 * -----------------------------------------------
 * 2023/07/03        V1.0          guoll
 ***************************************************************************/
void zxdh_smmu_use_l1_test(struct stPteRequest *ptMmuMmapCfg,
			   struct zxdh_sc_dev *dev)
{
	static int s_count;
	struct zxdh_src_copy_dest src_dest = {};

	memset((void *)dev->pte_address->uddPTETempVirAddr, 0, 8);

	/* cpy data from host to l2d */
	src_dest.src = dev->pte_address->uddPTETempPhyAddr;
	src_dest.len = 8;
	src_dest.dest = dev->pte_l2d_startpa + SMMU_L1_PT_SIZE + s_count * 8;

	*(u64 *)dev->pte_address->uddPTETempVirAddr = (++s_count);
	dev->cqp->process_config_pte_table(dev, src_dest);
}

/**************************************************************************
 * 函数名称： zxdh_smmu_use_l2_test
 * 功能描述： 在host上调用，通过dma模块出发smmu进行查表，测试L3的转换功能
 *
 * 输入参数： struct zxdh_sc_dev *ptMmuMmapCfg
 * 输出参数：
 * 返 回 值： CMDK_OK / CMDK_ERROR
 * 其它说明：测试接口
 *
 * 修改日期          版本号         修改人
 * -----------------------------------------------
 * 2023/07/03        V1.0          guoll
 ***************************************************************************/
void zxdh_smmu_use_l2_test(struct stPteRequest *ptMmuMmapCfg,
			   struct zxdh_sc_dev *dev)
{
	static int s_count;
	struct zxdh_src_copy_dest src_dest = {};

	memset((void *)dev->pte_address->uddPTETempVirAddr, 0, 8);

	/* cpy data from host to l2d */
	src_dest.src = dev->pte_address->uddPTETempPhyAddr;
	src_dest.len = 8;
	src_dest.dest = dev->pte_l2d_startpa + SMMU_L1_PT_SIZE + s_count * 8;

	*(u64 *)dev->pte_address->uddPTETempVirAddr = (++s_count);
	dev->cqp->process_config_pte_table(dev, src_dest);
}

/**************************************************************************
 * 函数名称： zxdh_smmu_use_l3_test
 * 功能描述： 在host上调用，通过dma模块出发smmu进行查表，测试L3的转换功能
 *
 * 输入参数： struct zxdh_sc_dev *ptMmuMmapCfg
 * 输出参数：
 * 返 回 值： CMDK_OK / CMDK_ERROR
 * 其它说明：测试接口
 *
 * 修改日期          版本号         修改人
 * -----------------------------------------------
 * 2023/07/03        V1.0          guoll
 ***************************************************************************/
void zxdh_smmu_use_l3_test(struct stPteRequest *ptMmuMmapCfg,
			   struct zxdh_sc_dev *dev)
{
	static int s_count;
	struct zxdh_src_copy_dest src_dest = {};

	// ========================================================================
	// ========================================================================
	memset((void *)dev->pte_address->uddPTETempVirAddr, 0, 8);

	/* cpy data from host to l2d */
	src_dest.src = dev->pte_address->uddPTETempPhyAddr;
	src_dest.len = 8;
	src_dest.dest = dev->pte_l2d_startpa + SMMU_L1_PT_SIZE + s_count * 8;

	*(u64 *)dev->pte_address->uddPTETempVirAddr = (++s_count);

	dev->cqp->process_config_pte_table(dev, src_dest);
}

/**************************************************************************
 * 函数名称： zxdh_smmu_set_pte
 * 功能描述： 在host上调用，配置页表
 *
 * 输入参数： struct zxdh_sc_dev *
 * 输出参数：
 * 返 回 值： CMDK_OK / CMDK_ERROR
 * 其它说明：对外接口
 *
 * 修改日期            版本号            修改人
 * -----------------------------------------------
 * 2023/04/26        V1.0          guoll
 ***************************************************************************/
int zxdh_smmu_set_pte(struct stPteRequest *ptMmuMmapCfg,
		      struct zxdh_sc_dev *dev)
{
	int ret = 0;

	SMMU_POINTER_CHECK(dev);
	SMMU_POINTER_CHECK(ptMmuMmapCfg);

	// tmpFlag = g_ucMmu600PrintModuleId;

	// g_ucMmu600PrintModuleId = 1;

	// 临时规避方法
	// ptMmuMmapCfg->udRWFlag = 3;

	// 仅用2M粒度
	//if((0 == ptMmuMmapCfg->udStreamid) && (1 > (s0_count)) && (0x200000 == ptMmuMmapCfg->uddSize))
	//if((0 == ptMmuMmapCfg->udStreamid) && (s0_count) && (0x200000 == ptMmuMmapCfg->uddSize))
	// 仅用4K粒度
	//if((0 == ptMmuMmapCfg->udStreamid) && (1 > s0_count) && (0x1000 == ptMmuMmapCfg->uddSize))
	//if((0 == ptMmuMmapCfg->udStreamid) && (1 > s0_count))
	ret = zxdh_smmu_mmap((struct stPteRequest *)ptMmuMmapCfg, dev);

	// g_ucMmu600PrintModuleId = tmpFlag;

	return ret;
}
EXPORT_SYMBOL(zxdh_smmu_set_pte);

u32 bspSmmuDeletePTE(u32 udSid, u64 uddVa, struct zxdh_sc_dev *dev)
{
	return CmdkSysMmuCmdTlbSync();
}
EXPORT_SYMBOL(bspSmmuDeletePTE);

static int zxdh_smmu_alloc_from_cma(struct device *device,
				    struct smmu_pte_address *pstPteAddress)
{
#ifdef USE_CMA
	SMMU_POINTER_CHECK(device);
	SMMU_POINTER_CHECK(pstPteAddress);
	pstPteAddress->CmaPageMemBaseVA = (u64)dma_alloc_coherent(
		device, SMMU_PT_TOTAL,
		(dma_addr_t *)(&(pstPteAddress->CmaPageMemBasePA)), GFP_KERNEL);

	SMMU_POINTER_CHECK(pstPteAddress->CmaPageMemBaseVA);
	SMMU_POINTER_CHECK(pstPteAddress->CmaPageMemBasePA);

	memset_8byte((u64 *)pstPteAddress->CmaPageMemBaseVA, 0, SMMU_PT_TOTAL);

	// self : init pte base va = page base va
	pstPteAddress->CmaMemBaseVA_pte = pstPteAddress->CmaPageMemBaseVA;
#endif

	return CMDK_OK;
}

/**************************************************************************
 * 函数名称： zxdh_smmu_pagetable_init
 * 功能描述： 初始化入口函数，在host上调用；
 *			  申请分配存储SMMU数据结构的内存；
 *            并且初始化页表数据结构。
 * 输入参数： struct zxdh_sc_dev *
 * 输出参数：
 * 返 回 值： CMDK_OK / CMDK_ERROR
 * 其它说明：对外接口
 *
 * 修改日期            版本号            修改人
 * -----------------------------------------------
 * 2023/04/26        V1.0          guoll
 ***************************************************************************/
int zxdh_smmu_pagetable_init(struct zxdh_sc_dev *dev)
{
	int ret = 0;

	// 存放页表初始化参数的数据结构
	struct stPagetableParam stPage = { 0 };

	SMMU_POINTER_CHECK(dev);

	dh_rdma_chan_smmu_invalid_tlb_send(dev);

	dev->pte_l2d_startpa = dev->l2d_smmu_addr;
	dev->pte_address = (struct smmu_pte_address *)kmalloc(
		sizeof(struct smmu_pte_address), GFP_KERNEL);
	SMMU_POINTER_CHECK(dev->pte_address);
	MEMSET((void *)dev->pte_address, sizeof(struct smmu_pte_address), 0,
	       sizeof(struct smmu_pte_address));
	zxdh_smmu_alloc_from_cma(dev->hw->device, dev->pte_address);

	stPage.udPageTableSize = SMMU_PT_TOTAL;
	stPage.udL1PageTableNum = SMMU_L1_PT_NUM;
	stPage.udL2PageTableNum = SMMU_L2_PT_NUM;
	stPage.udL3PageTableNum = SMMU_L3_PT_NUM;
	stPage.uddPageTablePhyAddr = dev->pte_address->CmaPageMemBasePA;
	stPage.uddPageTableVirAddr = dev->pte_address->CmaPageMemBaseVA;

	dev->pte_address->l2d_smmu_l2_offset = dev->l2d_smmu_l2_offset;

	ret = zxdh_smmu_struct_init(&stPage, dev->pte_address, dev->hw->device);
	if (ret)
		return CMDK_ERROR;

	return CMDK_OK;
}

int zxdh_smmu_pagetable_exit(struct zxdh_sc_dev *dev)
{
	struct smmu_pte_address *pstPteAddress = dev->pte_address;

	//mmu600test_dbg_exit(dev);

	if (pstPteAddress->uddPTETempVirAddr) {
		dma_free_coherent(dev->hw->device, SMMU_L1_PER_PT_SIZE * 4,
				(void *)pstPteAddress->uddPTETempVirAddr,
				(dma_addr_t)pstPteAddress->uddPTETempPhyAddr);
	}

	if (pstPteAddress->ptPteRecords)
		kfree((void *)pstPteAddress->ptPteRecords);

	if (pstPteAddress->uddSmmuMapManageAddr)
		kfree((void *)pstPteAddress->uddSmmuMapManageAddr);

	kfree((void *)dev->pte_address);
	return CMDK_OK;
}

int dh_rdma_chan_smmu_invalid_tlb_send(struct zxdh_sc_dev *dev)
{
	int ret = 0;
	u64 recv_buffer = 0;
	u8 *reply_ptr = NULL;
	//uint32_t pf_id = 0;
	uint8_t *risc_smmu_back_result = NULL;
	uint16_t *risc_smmu_back_len = NULL;
	struct zxdh_pci_bar_msg in = { 0 };
	struct zxdh_msg_recviver_mem result = { 0 };
	struct rsvMsgSmmuInfo tRscMsgSmmuInfo = { 0 };
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	struct zxdh_mgr mgr = { 0 };
	struct iidc_core_dev_info *cdev_info;
	u32 cnt = 0;
	u32 cnt_num = ZXDH_BAR_MSG_RETRY_NUM;

	if (rf->sc_dev.driver_load == false)
		cnt_num = ZXDH_BAR_MSG_DEFAULT_NUM;
	
	cdev_info = (struct iidc_core_dev_info *)rf->cdev;
        // query pcie id
        mgr.pdev = cdev_info->pdev;
        ret = dh_rdma_pf_pcie_id_get(&mgr);
        if (ret) {
            pr_err("[zxdh_rdma] [%s] get pf pcie_id failed, ret=%d\n", __func__, ret);
            return -EINVAL;
        }

	result.recv_buffer = &recv_buffer;
	result.buffer_len = sizeof(u64);

	//pf_id = pmgr->iwdev->rf->pf_id;

	tRscMsgSmmuInfo.udIsTlbInvalid = 1;
	tRscMsgSmmuInfo.stInvalidTlbCfg.cmd = CMDQ_OP_TLBI_NSNH_ALL;
	//tRscMsgSmmuInfo.stInvalidTlbCfg.vmid = pf_id;

	in.payload_addr = (uint8_t *)&tRscMsgSmmuInfo;
	in.payload_len = sizeof(struct rsvMsgSmmuInfo);

	in.src = MSG_CHAN_END_PF;
	in.dst = MSG_CHAN_END_RISC;
	in.virt_addr = (u64)dev->hw->pci_hw_addr + 0x2000; // bar空间偏移

	in.event_id = RSC_MSG_SMMU_EVENT_ID;
	in.src_pcieid = mgr.pcie_id;

	do {
		ret = zxdh_bar_chan_sync_msg_send(&in, &result);
		if ((ret != ZXDH_BAR_ERR_TIME_OUT) && (ret != ZXDH_BAR_ERR_LOCK_FAILED)) {
			break;
		}
		cnt++;
	}while(cnt < cnt_num);
	
	if (ret != 0) {
		SMMU_PRINT(PM_ERROR,
			   "zxdh_bar_chan_sync_msg_send error, ret = %d cnt=%d\n",
			   ret, cnt);
	}

	reply_ptr = (u8 *)result.recv_buffer; // common 通道处理状态信息
	if (*reply_ptr == 0xFF) {
		risc_smmu_back_result = (u8 *)(reply_ptr + 4);
		risc_smmu_back_len = (u16 *)(reply_ptr + 1);

		//risc_len = *(u16 *)(reply_ptr + MSG_REP_LEN_OFFSET);

		SMMU_PRINT(
			PM_ERROR,
			"risc_back_result = 0x%x, risc_smmu_back_len = 0x%x\n",
			*(u8 *)risc_smmu_back_result,
			*(u8 *)risc_smmu_back_len);
	}

	return 0;
}

MODULE_AUTHOR("ZTE, Inc");
MODULE_LICENSE("GPL");

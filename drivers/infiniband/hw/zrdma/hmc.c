// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */
#include "osdep.h"
#include "status.h"
#include "hmc.h"
#include "defs.h"
#include "type.h"
#include "protos.h"
#include "vf.h"
#include "virtchnl.h"
#include "icrdma_hw.h"
#include "main.h"
#include "smmu/kernel/adk_mmu600.h"

extern enum zxdh_hmc_rsrc_type iw_hmc_obj_types[ZXDH_HMC_IW_TXWINDOW + 1];

int zxdh_sc_create_date_cap_obj(struct zxdh_sc_dev *dev)
{
	struct zxdh_hmc_sd_entry *sd_entry = NULL;
	u32 sd_lmt = 0;
	u32 i = 0;
	int ret = 0;
	u32 hmc_entry_total = 0;
	u64 fpm_addr = 0, fpm_limit = 0;
	struct zxdh_hw *hw = dev->hw;
	struct zxdh_dma_mem dma_mem = {};
	u64 alloc_len = 0;
	struct stPteRequest ptMmuMmapCfg = {};

	memset(&ptMmuMmapCfg, 0, sizeof(ptMmuMmapCfg));

	fpm_addr = dev->data_cap_sd.data_cap_base;
	fpm_limit = dev->data_cap_sd.data_len;
	fpm_limit = ALIGN(fpm_limit, ZXDH_HMC_DIRECT_BP_SIZE);

	sd_lmt = fpm_limit / ZXDH_HMC_DIRECT_BP_SIZE;
	sd_lmt += 1;
	if (sd_lmt == 1) {
		hmc_entry_total++;
	} else {
		hmc_entry_total = sd_lmt - 1;
	}
	sd_entry = kzalloc(sizeof(struct zxdh_hmc_sd_entry) * hmc_entry_total,
			   GFP_KERNEL);
	if (!sd_entry) {
		pr_err("[zxdh_rdma] HMC: failed to allocate memory for sd_entry buffer\n");
		return -ENOMEM;
	}
	dev->data_cap_sd.entry = sd_entry;
	dev->data_cap_sd.sd_cnt = hmc_entry_total;
	pr_info("[zxdh_rdma] HMC: zxdh_sc_create_date_cap_obj sd_lmt:%d\n", sd_lmt);
	for (i = 0; i < sd_lmt - 1; i++) { // 满足2M空间
		alloc_len = ZXDH_HMC_DIRECT_BP_SIZE; //按实际2M分配空间
		dma_mem.size = ALIGN(alloc_len, ZXDH_HMC_PD_BP_BUF_ALIGNMENT);
		dma_mem.va = dma_alloc_coherent(hw->device, dma_mem.size,
						&dma_mem.pa, GFP_KERNEL);

		if (!dma_mem.va) {
			pr_err("[zxdh_rdma] %s[%d]: dma alloc failed!\n", __func__, __LINE__);
			return -ENOMEM;
		}

		memset(dma_mem.va, 0, alloc_len);
		// ********************调用SMMU接口***********************
		ptMmuMmapCfg.uddPhyAddr = dma_mem.pa;
		ptMmuMmapCfg.uddVirAddr = fpm_addr;
		ptMmuMmapCfg.uddSize = alloc_len;
		ptMmuMmapCfg.udStreamid = dev->hmc_fn_id;
		// bspSmmuSetPTE(&ptMmuMmapCfg,dev);  // for Crash
		ptMmuMmapCfg.udRWFlag = 0x03;

		memcpy(&sd_entry->u.bp.addr, &dma_mem,
		       sizeof(sd_entry->u.bp.addr));
		sd_entry->valid = true;
		fpm_addr = fpm_addr + alloc_len;
		sd_entry++;

		ret = zxdh_smmu_set_pte(&ptMmuMmapCfg, dev);
		if (ret) {
			pr_err("[zxdh_rdma] %s[%d]: smmu_set_pte failed! ret=%d sd_cnt=%u\n", __func__, __LINE__, ret, dev->data_cap_sd.sd_cnt);
			return ret;
		}
	}

	return 0;
}

/**
 * zxdh_sc_create_hmc_obj - allocate backing store for hmc objects
 * @dev: pointer to the device structure
 * @info: pointer to zxdh_hmc_create_obj_info struct
 *
 * This will allocate memory for PDs and backing pages and populate
 * the sd and pd entries.
 */
int zxdh_sc_create_hmc_obj(struct zxdh_sc_dev *dev,
			   struct zxdh_hmc_create_obj_info *info)
{
	struct zxdh_hmc_sd_entry *sd_entry;
	u32 sd_lmt = 0;
	u32 i = 0, cnt = 0;
	int ret = 0;
	u64 fpm_addr = 0, fpm_limit = 0;
	struct zxdh_hw *hw = dev->hw;
	struct zxdh_dma_mem dma_mem = {};
	u64 alloc_len = 0;
	struct stPteRequest ptMmuMmapCfg = {};

	memset(&ptMmuMmapCfg, 0, sizeof(ptMmuMmapCfg));

	fpm_addr = info->hmc_info->hmc_obj[info->rsrc_type].base;
	switch (info->rsrc_type) {
	case ZXDH_HMC_IW_QP:
		cnt = dev->hmc_pf_manager_info.total_qp_cnt;
		break;
	case ZXDH_HMC_IW_CQ:
		cnt = dev->hmc_pf_manager_info.total_cq_cnt;
		break;
	case ZXDH_HMC_IW_SRQ:
		cnt = dev->hmc_pf_manager_info.total_srq_cnt;
		break;
	default:
		cnt = info->hmc_info->hmc_obj[info->rsrc_type].cnt;
		break;
	}
	fpm_limit = info->hmc_info->hmc_obj[info->rsrc_type].size * cnt;
	fpm_limit = ALIGN(fpm_limit, ZXDH_HMC_DIRECT_BP_SIZE);

	sd_lmt = fpm_limit / ZXDH_HMC_DIRECT_BP_SIZE;

	for (i = 0; i < sd_lmt; i++) { // 满足2M空间
		sd_entry = &info->hmc_info->sd_table.sd_entry[info->add_sd_cnt];

		alloc_len = ZXDH_HMC_DIRECT_BP_SIZE; //按实际2M分配空间
		dma_mem.size = ALIGN(alloc_len, ZXDH_HMC_PD_BP_BUF_ALIGNMENT);
		dma_mem.va = dma_alloc_coherent(hw->device, dma_mem.size,
						&dma_mem.pa, GFP_KERNEL);

		if (!dma_mem.va) {
			pr_err("[zxdh_rdma] %s[%d]: dma alloc failed!\n", __func__, __LINE__);
			return -ENOMEM;
		}

		memset(dma_mem.va, 0, alloc_len);
		// ********************调用SMMU接口***********************
		ptMmuMmapCfg.uddPhyAddr = dma_mem.pa;
		ptMmuMmapCfg.uddVirAddr = fpm_addr;
		ptMmuMmapCfg.uddSize = alloc_len;
		ptMmuMmapCfg.udStreamid = dev->hmc_fn_id;
		// bspSmmuSetPTE(&ptMmuMmapCfg,dev);  // for Crash
		ptMmuMmapCfg.udRWFlag = 0x03;

		memcpy(&sd_entry->u.bp.addr, &dma_mem,
		       sizeof(sd_entry->u.bp.addr));

		sd_entry->u.bp.sd_pd_index = info->add_sd_cnt;
		info->hmc_info->sd_indexes[info->add_sd_cnt] =
			(u16)info->add_sd_cnt;
		sd_entry->valid = true;
		fpm_addr = fpm_addr + alloc_len;
		info->add_sd_cnt++;

		ret = zxdh_smmu_set_pte(&ptMmuMmapCfg, dev);
		if (ret) {
			pr_err("[zxdh_rdma] %s[%d]: smmu_set_pte failed! ret=%d sd_cnt=%u\n", __func__, __LINE__, ret, info->add_sd_cnt);
			return ret;
		}
	}

	return 0;
}

static int zxdh_pf2vf_add_pble_hmc_obj(struct zxdh_sc_dev *dev,
				       struct zxdh_vfdev *vf_dev, u32 rsrc_type)
{
	struct zxdh_hmc_sd_entry *sd_entry = NULL;
	struct zxdh_dma_mem dma_mem = {};
	u32 pble_hmc_comm_index = 0, pages = 0;
	u32 unallocated_pble = 0, ret = 0;
	u64 alloc_len = 0, size = 0;
	u64 next_fpm_addr = 0, fpm_base_addr = 0;
	u32 pd_idx = 0, rel_pd_idx = 0;
	struct zxdh_hmc_info *hmc_info = &vf_dev->hmc_info;

	struct stPteRequest ptMmuMmapCfg = {};

	if (rsrc_type == ZXDH_HMC_IW_PBLE) {
		pble_hmc_comm_index = hmc_info->pble_hmc_index;
		unallocated_pble = vf_dev->pbleq_unallocated_pble;
		fpm_base_addr = vf_dev->pbleq_fpm_base_addr;
		next_fpm_addr = vf_dev->pbleq_next_fpm_addr;
	} else if (rsrc_type == ZXDH_HMC_IW_PBLE_MR) {
		pble_hmc_comm_index = hmc_info->pble_mr_hmc_index;
		unallocated_pble = vf_dev->pblemr_unallocated_pble;
		fpm_base_addr = vf_dev->pblemr_fpm_base_addr;
		next_fpm_addr = vf_dev->pblemr_next_fpm_addr;
	}

	if (unallocated_pble < PBLE_PER_PAGE)
		return -ENOMEM;

	sd_entry = &hmc_info->sd_table.sd_entry[pble_hmc_comm_index];
	pd_idx = (u32)((next_fpm_addr - fpm_base_addr) /
		       ZXDH_HMC_PAGED_BP_SIZE); //4096
	rel_pd_idx = (pd_idx % ZXDH_HMC_PD_CNT_IN_SD); // 512
	pages = (rel_pd_idx) ? (ZXDH_HMC_PD_CNT_IN_SD - rel_pd_idx) :
				     ZXDH_HMC_PD_CNT_IN_SD;

	pages = min(pages,
		    unallocated_pble >> PBLE_512_SHIFT); // PBLE_512_SHIFT==9

	if (!sd_entry->valid) {
		alloc_len = pages * ZXDH_HMC_PAGED_BP_SIZE;
		dma_mem.size = ALIGN(alloc_len, ZXDH_HMC_PD_BP_BUF_ALIGNMENT);
		dma_mem.va = dma_alloc_coherent(dev->hw->device, dma_mem.size,
						&dma_mem.pa, GFP_KERNEL);
		if (!dma_mem.va)
		{
			pr_info("[zxdh_rdma] %s %d failed to alloc mem\n", __func__, __LINE__);
			return -ENOMEM;
		}

		memcpy(&sd_entry->u.bp.addr, &dma_mem,
		       sizeof(sd_entry->u.bp.addr));

		ptMmuMmapCfg.uddPhyAddr = dma_mem.pa;
		ptMmuMmapCfg.uddVirAddr = next_fpm_addr;
		ptMmuMmapCfg.uddSize = alloc_len;
		ptMmuMmapCfg.udStreamid = dev->hmc_fn_id; // 这个SID后续需要修改
		ptMmuMmapCfg.udRWFlag = 0x03;
		// 这里调用SMMU接口
		ret = zxdh_smmu_set_pte(&ptMmuMmapCfg, dev);
		if (ret)
		{
			pr_info("[zxdh_rdma] %s %d set pte failed ret:%d\n", __func__, __LINE__, ret);
			dma_free_coherent(dev->hw->device, dma_mem.size, dma_mem.va, dma_mem.pa);
			return ret;
		}

		sd_entry->u.bp.sd_pd_index = pble_hmc_comm_index;
		hmc_info->sd_table.use_cnt = pble_hmc_comm_index;
		hmc_info->sd_table.sd_entry->entry_type = ZXDH_SD_TYPE_DIRECT;
	}

	sd_entry->valid = true;
	size = pages << HMC_PAGED_BP_SHIFT;
	if (rsrc_type == ZXDH_HMC_IW_PBLE) {
		vf_dev->pbleq_next_fpm_addr += size;
		vf_dev->pbleq_unallocated_pble -= (u32)(size >> 3);
	} else {
		vf_dev->pblemr_next_fpm_addr += size;
		vf_dev->pblemr_unallocated_pble -= (u32)(size >> 3);
	}

	return 0;
}

int zxdh_vf_add_pble_hmc_obj(struct zxdh_sc_dev *dev,
			     struct zxdh_hmc_info *hmc_info,
			     struct zxdh_hmc_pble_rsrc *pble_rsrc, u32 pages)
{
	struct zxdh_hmc_sd_entry *sd_entry;
	struct zxdh_dma_mem dma_mem = {};
	struct zxdh_pci_f *rf;
	u64 alloc_len;
	u32 pble_hmc_comm_index = 0, cnt = 0, val = 0;
	struct zxdh_hw *hw = pble_rsrc->dev->hw;
	int status = 0;

	rf = container_of(dev, struct zxdh_pci_f, sc_dev);

	if (pble_rsrc->pble_type == PBLE_QUEUE)
		pble_hmc_comm_index = hmc_info->pble_hmc_index;
	else
		pble_hmc_comm_index = hmc_info->pble_mr_hmc_index;

	sd_entry = &hmc_info->sd_table.sd_entry[pble_hmc_comm_index];

	if (!sd_entry->valid) {
		if (!dev->hmc_use_dpu_ddr) {
			writel(0, (u32 __iomem *)(dev->hw->hw_addr +
				  C_RDMA_CQP_CQ_DISTRIBUTE_DONE(dev->ep_id)));

			if (pble_rsrc->pble_type == PBLE_QUEUE) {
				zxdh_sc_send_mailbox_cmd(
					dev, ZTE_ZXDH_OP_ADD_QPBLE_HMC_RANGE, 0,
					0, 0, rf->vf_id);
			} else {
				zxdh_sc_send_mailbox_cmd(
					dev, ZTE_ZXDH_OP_ADD_MRPBLE_HMC_RANGE,
					0, 0, 0, rf->vf_id);
			}

			do {
				val = readl(dev->hw->hw_addr +
							C_RDMA_CQP_CQ_DISTRIBUTE_DONE(dev->ep_id));			
				if (cnt++ > ZXDH_MAILBOX_CYC_NUM * dev->hw_attrs.max_done_count) {
					status = -ETIMEDOUT;
					pr_info("[zxdh_rdma] vhca_id:%d waiting completed PBLE mailbox too long time,timeout!\n",dev->vhca_id);
					break;
			}
			if (dev->hw_attrs.skip_hw == true) {
				status = -ETIMEDOUT;
				break;
			}
			usleep_range(ZXDH_MAILBOX_SLEEP_TIME, ZXDH_MAILBOX_SLEEP_TIME + 10);
			} while (!val);
		}

		alloc_len = (u64)pages * ZXDH_HMC_PAGED_BP_SIZE;
		dma_mem.size = ALIGN(alloc_len, ZXDH_HMC_PD_BP_BUF_ALIGNMENT);
		dma_mem.va = dma_alloc_coherent(hw->device, dma_mem.size,
						&dma_mem.pa, GFP_KERNEL);
		if (!dma_mem.va)
			return -ENOMEM;

		memcpy(&sd_entry->u.bp.addr, &dma_mem,
		       sizeof(sd_entry->u.bp.addr));

		sd_entry->u.bp.sd_pd_index = pble_hmc_comm_index;
		if (pble_rsrc->pble_type == PBLE_QUEUE)
			hmc_info->pble_hmc_index++;
		else
			hmc_info->pble_mr_hmc_index++;

		hmc_info->sd_table.use_cnt++;
		hmc_info->sd_table.sd_entry->entry_type = ZXDH_SD_TYPE_DIRECT;
	}
	return status;
}

int zxdh_add_pble_hmc_obj(struct zxdh_hmc_info *hmc_info,
			  struct zxdh_hmc_pble_rsrc *pble_rsrc, u32 pages)
{
	struct zxdh_pci_f *rf;
	struct zxdh_hmc_sd_entry *sd_entry;
	struct zxdh_dma_mem dma_mem = {};  /* First allocation, for cleanup */
	struct zxdh_dma_mem dma_mem_hw = {};  /* Second allocation */

	struct stPteRequest ptMmuMmapCfg = {};

	u64 alloc_len;
	u32 pble_hmc_comm_index;
	u32 ret = 0;
	struct zxdh_hw *hw = pble_rsrc->dev->hw;

	memset(&ptMmuMmapCfg, 0, sizeof(ptMmuMmapCfg));
	rf = container_of(pble_rsrc->dev, struct zxdh_pci_f, sc_dev);

	if (pble_rsrc->pble_type == PBLE_QUEUE)
		pble_hmc_comm_index = hmc_info->pble_hmc_index;
	else
		pble_hmc_comm_index = hmc_info->pble_mr_hmc_index;

	sd_entry = &hmc_info->sd_table.sd_entry[pble_hmc_comm_index];

	if (!sd_entry->valid) {
		alloc_len = (u64)pages * ZXDH_HMC_PAGED_BP_SIZE;
		dma_mem.size = ALIGN(alloc_len, ZXDH_HMC_PD_BP_BUF_ALIGNMENT);
		dma_mem.va = dma_alloc_coherent(hw->device, dma_mem.size,
						&dma_mem.pa, GFP_KERNEL);
		if (!dma_mem.va)
			return -ENOMEM;
		//这里申请的2M内存是软件用的，因此E200也可以用dma_alloc_coherent申请主机上的内存
		memset(dma_mem.va, 0, dma_mem.size);

		memcpy(&sd_entry->u.bp.addr, &dma_mem,
		       sizeof(sd_entry->u.bp.addr));

		if (!rf->use_ext_mem_flag)
		{
			if (false == pble_rsrc->dev->hmc_use_dpu_ddr) { // is HOST DDR
				dma_mem_hw.size = ALIGN(alloc_len, ZXDH_HMC_PD_BP_BUF_ALIGNMENT);
				dma_mem_hw.va = dma_alloc_coherent(hw->device, dma_mem_hw.size,
							&dma_mem_hw.pa, GFP_KERNEL);
				if (!dma_mem_hw.va) {
					/* Release first allocated memory */
					dma_free_coherent(hw->device, dma_mem.size,
							dma_mem.va, dma_mem.pa);
					return -ENOMEM;
				}
				memset(dma_mem_hw.va, 0, dma_mem_hw.size);

				memcpy(&sd_entry->u.bp.addr_hardware, &dma_mem_hw,
					sizeof(sd_entry->u.bp.addr_hardware));

				ptMmuMmapCfg.uddPhyAddr = dma_mem_hw.pa;
				ptMmuMmapCfg.uddVirAddr = pble_rsrc->next_fpm_addr;
				ptMmuMmapCfg.uddSize = alloc_len;
				ptMmuMmapCfg.udStreamid =
					pble_rsrc->dev->hmc_fn_id; // 这个SID后续需要修改
				ptMmuMmapCfg.udRWFlag = 0x03;
				// 这里调用SMMU接口
				ret = zxdh_smmu_set_pte(&ptMmuMmapCfg, pble_rsrc->dev);
				if (ret) {
					/* Release second allocated memory */
					dma_free_coherent(hw->device, dma_mem_hw.size,
							dma_mem_hw.va, dma_mem_hw.pa);
					/* Release first allocated memory */
					dma_free_coherent(hw->device, dma_mem.size,
							dma_mem.va, dma_mem.pa);
					return ret;
				}
			}
		}
		sd_entry->u.bp.sd_pd_index = pble_hmc_comm_index;
		if (pble_rsrc->pble_type == PBLE_QUEUE)
			hmc_info->pble_hmc_index++;
		else
			hmc_info->pble_mr_hmc_index++;

		hmc_info->sd_table.use_cnt++;
		hmc_info->sd_table.sd_entry->entry_type = ZXDH_SD_TYPE_DIRECT;
	}

	return 0;
}

/**
 * zxdh_prep_remove_sd_bp - Prepares to remove a backing page from a sd entry
 * @hmc_info: pointer to the HMC configuration information structure
 * @idx: the page index
 */
int zxdh_prep_remove_sd_bp(struct zxdh_hmc_info *hmc_info, u32 idx)
{
	struct zxdh_hmc_sd_entry *sd_entry;

	sd_entry = &hmc_info->sd_table.sd_entry[idx];

	hmc_info->sd_table.use_cnt--;
	sd_entry->valid = false;

	return 0;
}

/**
 * zxdh_get_next_vf_idx - return the next vf_idx available
 * @dev: pointer to RDMA dev structure
 */
static u16 zxdh_get_next_vf_idx(struct zxdh_sc_dev *dev)
{
	u16 vf_idx;

	for (vf_idx = 0; vf_idx < dev->num_vfs; vf_idx++) {
		if (!dev->vf_dev[vf_idx])
			break;
	}

	return vf_idx < dev->num_vfs ? vf_idx : ZXDH_VCHNL_INVALID_VF_IDX;
}

static int zxdh_get_vf_hmc_baseinfo(struct zxdh_sc_dev *dev,
				    struct zxdh_hmc_obj_info *hmc_obj,
				    u16 iw_vf_idx, u16 vf_id)
{
	u16 i = 0;

	for (i = ZXDH_HMC_IW_AH; i < ZXDH_HMC_IW_MAX; i++) {
		if ((i == ZXDH_HMC_IW_IRD) || (i == ZXDH_HMC_IW_TXWINDOW))
			hmc_obj[i].max_cnt = dev->hmc_pf_manager_info.vf_qp_cnt;
		else if (i == ZXDH_HMC_IW_AH)
			hmc_obj[i].max_cnt = dev->hmc_pf_manager_info.vf_ah_cnt;
		else if (i == ZXDH_HMC_IW_MR)
			hmc_obj[i].max_cnt = dev->hmc_pf_manager_info.vf_mr_cnt;
		else if (i == ZXDH_HMC_IW_PBLE_MR)
			hmc_obj[i].max_cnt = dev->hmc_pf_manager_info.vf_pblemr_cnt;
		else if (i == ZXDH_HMC_IW_PBLE)
			hmc_obj[i].max_cnt = dev->hmc_pf_manager_info.vf_pblequeue_cnt;

		hmc_obj[i].cnt = hmc_obj[i].max_cnt;
		hmc_obj[i].size = dev->hmc_info->hmc_obj[i].size;
		hmc_obj[i].type = dev->hmc_info->hmc_obj[i].type;
		hmc_obj[i].base = dev->hmc_info->hmc_obj[i].base +
				(dev->hmc_info->hmc_obj[i].cnt +
				hmc_obj[i].cnt * vf_id) *
					hmc_obj[i].size;
	}

	return 0;
}

struct zxdh_vfdev *zxdh_pf_get_vf_hmc_res(struct zxdh_sc_dev *dev, u16 vf_id)
{
	struct zxdh_virt_mem virt_mem;
	struct zxdh_vfdev *vf_dev;
	u16 iw_vf_idx = 0;
	unsigned long flags;

	spin_lock_irqsave(&dev->vf_dev_lock, flags);
	iw_vf_idx = zxdh_get_next_vf_idx(dev);
	if (iw_vf_idx == ZXDH_VCHNL_INVALID_VF_IDX ||
	    iw_vf_idx >= ZXDH_MAX_PE_ENA_VF_COUNT) {

		spin_unlock_irqrestore(&dev->vf_dev_lock, flags);
		return NULL;
	}

	virt_mem.size = sizeof(struct zxdh_vfdev) +
			sizeof(struct zxdh_hmc_obj_info) * ZXDH_HMC_IW_MAX;
	virt_mem.va = kzalloc(virt_mem.size, GFP_ATOMIC);

	if (!virt_mem.va) {
		pr_err("[zxdh_rdma] VIRT: VF%u Unable to allocate a VF device structure.\n",
		       vf_id);
		spin_unlock_irqrestore(&dev->vf_dev_lock, flags);
		return NULL;
	}

	vf_dev = virt_mem.va;
	vf_dev->pf_dev = dev;
	vf_dev->vf_id = vf_id;
	vf_dev->iw_vf_idx = iw_vf_idx;
	vf_dev->pf_hmc_initialized = false;
	vf_dev->hmc_info.hmc_obj = (struct zxdh_hmc_obj_info *)(&vf_dev[1]);
	zxdh_get_vf_hmc_baseinfo(dev, vf_dev->hmc_info.hmc_obj, iw_vf_idx,
				 vf_id);

	refcount_set(&vf_dev->refcnt, 1);
	dev->vf_dev[iw_vf_idx] = vf_dev;
	spin_unlock_irqrestore(&dev->vf_dev_lock, flags);
	return vf_dev;
}

/**
 * zxdh_mailbox_worker - process mailbox message
 * @work: work task structure
 */
static void zxdh_mailbox_worker(struct work_struct *work)
{
    u32 ret = 0;
	int resp_code = 0;
	u32 i = 0;
	u16 srcvhcaid = 0, mb_vfid = 0, iw_vf_idx = 0, vf_vhca_id = 0;
	u8 opt = 0xff; // 避免与mailbox消息类型宏定义同值.
	struct zxdh_vfdev *vf_dev = NULL;
	struct zxdh_hmc_create_obj_info obj_info = {};
	dpp_pf_info_t pf_info = { 0 };
	void *vf_dev_mac_addr = NULL;
	void *vf_dev_addr = NULL;
	u64 op_ret_val;
	__le64 addrbuf[ZXDH_MAILBOX_ADDR_BUF_LEN];
	struct zxdh_sc_dev *dev;
	struct zxdh_pci_f *rf;
	struct iidc_core_dev_info *cdev_info;
	struct mailbox_work *dwork = container_of(work, struct mailbox_work, work);
	struct zxdh_np_zcam_pos_info pos_info = { 0 };
	int pos_sta = 0;
	bool mac_in_use = false;
	zxdh_mac_info_t mac_info = {0};
	const u8 *mac = NULL;

	dev = dwork->dev;
	rf = container_of(dev, struct zxdh_pci_f, sc_dev);
	cdev_info = rf->cdev;
	for (i = 0; i < ZXDH_MAILBOX_ADDR_BUF_LEN; i++) {
		addrbuf[i] = dwork->addrbuf[i];
	}
	op_ret_val = dwork->op_ret_val;
	opt = (u8)dwork->addrbuf[0]; //避免opt默认为0情况，ZTE_ZXDH_VCHNL_OP_GET_HMC_FCN修改为1
	mb_vfid = FIELD_GET(ZXDH_SRC_PFVF_ID, op_ret_val);
	srcvhcaid = FIELD_GET(ZXDH_SRC_VHCA_ID, op_ret_val);
	vf_dev = zxdh_find_vf_dev(dev, mb_vfid);
	kfree(dwork);

	switch (opt) {
	case ZTE_ZXDH_VCHNL_OP_GET_HMC_FCN: // fix is 1,
		if (!vf_dev) {
			vf_dev = zxdh_pf_get_vf_hmc_res(dev, mb_vfid);
			if (!vf_dev) {
				resp_code = -ENODEV;
				pr_err("[zxdh_rdma] %s vhca_id:%d get vf hmc res failed!\n",__func__,dev->vhca_id);
				break;
			}
			refcount_inc(&vf_dev->refcnt);
		}
		vf_dev->vhca_id = srcvhcaid;
		obj_info.hmc_info = &vf_dev->hmc_info;
		zxdh_del_hmc_objects(dev, obj_info.hmc_info);
		obj_info.add_sd_cnt = 0;
		zxdh_vfhmc_enter(dev, obj_info.hmc_info);

		vf_dev->hmc_info.pble_hmc_index =
			vf_dev->hmc_info.hmc_first_entry_pble;
		vf_dev->pbleq_unallocated_pble =
			obj_info.hmc_info->hmc_obj[ZXDH_HMC_IW_PBLE].cnt;
		vf_dev->pbleq_fpm_base_addr =
			obj_info.hmc_info->hmc_obj[ZXDH_HMC_IW_PBLE].base;
		vf_dev->pbleq_next_fpm_addr = vf_dev->pbleq_fpm_base_addr;

		vf_dev->hmc_info.pble_mr_hmc_index =
			vf_dev->hmc_info.hmc_first_entry_pble_mr;
		vf_dev->pblemr_unallocated_pble =
			obj_info.hmc_info->hmc_obj[ZXDH_HMC_IW_PBLE_MR].cnt;
		vf_dev->pblemr_fpm_base_addr =
			obj_info.hmc_info->hmc_obj[ZXDH_HMC_IW_PBLE_MR].base;
		vf_dev->pblemr_next_fpm_addr = vf_dev->pblemr_fpm_base_addr;

		for (i = ZXDH_HMC_IW_AH; i < ZXDH_HMC_IW_TXWINDOW + 1; i++) {
			zxdh_create_vf_hmc_objs(dev, &vf_dev->hmc_info, i,
						&obj_info);
		}

		break;
	case ZTE_ZXDH_OP_REQ_NP_CONFIG:
		pf_info.vport = addrbuf[2];

		pf_info.slot = cdev_info->slot_id;
		dpp_vport_vhca_id_add(&pf_info, srcvhcaid);
		ret = dpp_vport_attr_set(&pf_info, EGR_FLAG_VHCA, srcvhcaid);
        if (ret != 0) {
            pr_err("[zxdh_rdma] %s[%d]: dpp vport attr set EGR_FLAG_VHCA fail! ret=%u!\n", __func__, __LINE__, ret);
			return;
        }
		ret = dpp_vport_attr_set(&pf_info, EGR_FLAG_RDMA_OFFLOAD_EN_OFF,
				   EGR_RDMA_OFFLOAD_EN);
        if (ret != 0) {
            pr_err("[zxdh_rdma] %s[%d]: dpp vport attr set EGR_FLAG_RDMA_OFFLOAD_EN_OFF fail! ret=%u!\n", __func__, __LINE__, ret);
			return;
        }

		break;
	case ZTE_ZXDH_OP_DEL_NP_CONFIG:
		pf_info.vport = addrbuf[2];
		pf_info.slot = cdev_info->slot_id;
		ret = dpp_vport_attr_set(&pf_info, EGR_FLAG_RDMA_OFFLOAD_EN_OFF,
				   EGR_RDMA_OFFLOAD_OFF);
        if (ret != 0) {
            pr_err("[zxdh_rdma] %s[%d]: dpp vport attr set EGR_FLAG_RDMA_OFFLOAD_EN_OFF fail! ret=%u!\n", __func__, __LINE__, ret);
			return;
        }
		break;
	case ZTE_ZXDH_OP_DEL_HMC_OBJ_RANGE:
		if (!vf_dev) {
				resp_code = -ENODEV;
				pr_info("[zxdh_rdma] VF[%d] remove failed by mailbox!\n", mb_vfid);
				break;
		}
		iw_vf_idx = vf_dev->iw_vf_idx;
		vf_vhca_id = vf_dev->vhca_id;
		zxdh_del_hmc_objects(dev,
				     &rf->sc_dev.vf_dev[iw_vf_idx]->hmc_info);
		zxdh_remove_vf_dev(dev, rf->sc_dev.vf_dev[iw_vf_idx]);
		vf_dev = NULL;
		break;
	case ZTE_ZXDH_OP_REQ_NP_MAC_DEL:
		pf_info.vport = addrbuf[2];
		vf_dev_mac_addr = (void *)&addrbuf[3];
		pf_info.slot = cdev_info->slot_id;
		mac_info.mac_addr = vf_dev_mac_addr;
		mac_info.pf_info = &pf_info;
		mac_info.vhca_id = srcvhcaid;
		mac_in_use = zxdh_check_mac_in_use(rf, &mac_info);
		if (mac_in_use) {
			break;
		}
		pos_sta = zxdh_dpp_get_trans_item_pos(rf, &pf_info, vf_dev_mac_addr, &pos_info);
		if (pos_sta) {
			pr_err("[zxdh_rdma][%s][%d]:dpp_get_trans_item_pos failed ret:%d\n", __func__, __LINE__, pos_sta);
			break;
		}
		pr_info("[zxdh_rdma][%s][%d]: dpp del rdma trans item, addr:0x%x, size:%u, pos:%u, vhca:%u\n", __func__, __LINE__, pos_info.entry_addr, pos_info.entry_size, pos_info.entry_pos, srcvhcaid);
		ret = dpp_del_rdma_trans_item(&pf_info, vf_dev_mac_addr);
		if (ret != 0) {
			pr_err("[zxdh_rdma] %s[%d]: dpp vport attr set ZTE_ZXDH_OP_REQ_NP_MAC_DEL fail! ret=%u!\n", __func__, __LINE__, ret);
			return;
		}
		zxdh_process_np_mac(rf, ZXDH_CMD_NP_MAC_DEL, srcvhcaid, &pos_info);
		break;
	case ZTE_ZXDH_OP_REQ_NP_MAC_ADD:
		pf_info.vport = addrbuf[2];
		vf_dev_addr = (void *)&addrbuf[3];
		pf_info.slot = cdev_info->slot_id;
		mac_info.mac_addr = vf_dev_addr;
		mac_info.pf_info = &pf_info;
		mac_info.vhca_id = srcvhcaid;
		mac = mac_info.mac_addr;
		mac_in_use = zxdh_check_mac_in_use(rf, &mac_info);
		if (mac_in_use) {
			pr_err("[zxdh_rdma][%s][%d] mac:%02x:%02x:%02x:%02x:%02x:%02x is in use, cannot add mac to vhca:%u\n",
			       __func__, __LINE__, mac[0], mac[1], mac[2],
			       mac[3], mac[4], mac[5], mac_info.vhca_id);
			break;
		}
		dpp_add_rdma_trans_item(&pf_info, vf_dev_addr, srcvhcaid);
		pos_sta = zxdh_dpp_get_trans_item_pos(rf, &pf_info, vf_dev_addr, &pos_info);
		pr_info("[zxdh_rdma][%s][%d]: dpp add rdma trans item, addr:0x%x, size:%u, pos:%u, vhca:%u\n", __func__, __LINE__, pos_info.entry_addr, pos_info.entry_size, pos_info.entry_pos, srcvhcaid);
		if (pos_sta) {
			pr_err("[zxdh_rdma][%s][%d]:dpp_get_trans_item_pos failed, ret:%d\n", __func__, __LINE__, pos_sta);
			break;
		}
		zxdh_process_np_mac(rf, ZXDH_CMD_NP_MAC_ADD, srcvhcaid, &pos_info);
		break;
	case ZTE_ZXDH_OP_ADD_QPBLE_HMC_RANGE:
	case ZTE_ZXDH_OP_ADD_MRPBLE_HMC_RANGE:
		if (!vf_dev) {
				resp_code = -ENODEV;
				pr_err("[zxdh_rdma] %s vhca_id:%d get vf_dev failed!\n",__func__,dev->vhca_id);
				break;
			}
		if (!dev->hmc_use_dpu_ddr) {
			if (opt == ZTE_ZXDH_OP_ADD_QPBLE_HMC_RANGE)
				i = ZXDH_HMC_IW_PBLE; /* code */
			else
				i = ZXDH_HMC_IW_PBLE_MR;

			if (vf_dev->hmc_info.hmc_obj[i].cnt) {
				resp_code = zxdh_pf2vf_add_pble_hmc_obj(
					dev, vf_dev, i);
				if (i == ZXDH_HMC_IW_PBLE)
					vf_dev->hmc_info.pble_hmc_index++;
				else
					vf_dev->hmc_info.pble_mr_hmc_index++;
			}
		}
		break;
	case ZTE_ZXDH_OP_SET_SMMU_INVALID:
		if (!dev->hmc_use_dpu_ddr) 
			dh_rdma_chan_smmu_invalid_tlb_send(dev);
		vf_vhca_id = srcvhcaid;
		break;
	case ZTE_ZXDH_OP_DEVICE_LOAD:
	    zxdh_add_vf_id(rf, srcvhcaid, mb_vfid);
	    break;
	case ZTE_ZXDH_OP_DEVICE_UNLOAD:
	    zxdh_del_vf_id(rf, srcvhcaid);
	    break;
	default:
		break;
	}

	if (!vf_dev) {
		if((opt != ZTE_ZXDH_OP_DEL_HMC_OBJ_RANGE) && (opt != ZTE_ZXDH_OP_SET_SMMU_INVALID)) {
			resp_code = -ENODEV;
			pr_err("[zxdh_rdma] %s vhca_id:%d vf_dev is NULL!\n",__func__,dev->vhca_id);
		} else 
			resp_code = zxdh_rdma_reg_write(rf, C_RDMA_VF_HMC_CQP_CQ_DISTRIBUTE_DONE(vf_vhca_id), 1);
		return ;
	} 
	if (opt != ZTE_ZXDH_OP_SET_SMMU_INVALID)
		vf_vhca_id = vf_dev->vhca_id;
	zxdh_put_vfdev(dev, vf_dev);
	resp_code = zxdh_rdma_reg_write(rf, C_RDMA_VF_HMC_CQP_CQ_DISTRIBUTE_DONE(vf_vhca_id), 1);
	if (resp_code) {
		pr_err("[zxdh_rdma] %s failed msg, resp_code:%d\n",__func__,resp_code);
	}
}

/**
 * zxdh_vf_mailbox_worker - process mailbox message
 * @work: work task structure
 */
static void zxdh_vf_mailbox_worker(struct work_struct *work)
{
    u16 srcvhcaid = 0;
    u8 opt = 0xff; // 避免与mailbox消息类型宏定义同值.
    u64 op_ret_val;
    int resp_code = 0;
    struct zxdh_sc_dev *dev;
    struct zxdh_pci_f *rf;
    struct mailbox_work *dwork = container_of(work, struct mailbox_work, work);
    struct zxdh_device *iwdev;
    __le64 addrbuf[ZXDH_MAILBOX_ADDR_BUF_LEN];
    u32 i;
    dev = dwork->dev;
    rf = container_of(dev, struct zxdh_pci_f, sc_dev);
    op_ret_val = dwork->op_ret_val;
    opt = (u8)dwork->addrbuf[0]; //避免opt默认为0情况，ZTE_ZXDH_VCHNL_OP_GET_HMC_FCN修改为1
    for (i = 0; i < ZXDH_MAILBOX_ADDR_BUF_LEN; i++) {
		addrbuf[i] = dwork->addrbuf[i];
	}
    srcvhcaid = FIELD_GET(ZXDH_SRC_VHCA_ID, op_ret_val);
    kfree(dwork);
    resp_code = zxdh_rdma_reg_write(rf, C_RDMA_VF_HMC_CQP_CQ_DISTRIBUTE_DONE(srcvhcaid), 1);
    if (resp_code) {
       pr_err("[zxdh_rdma] %s failed msg, resp_code:%d\n",__func__,resp_code);
    }

    switch (opt) {
    case ZTE_ZXDH_OP_PF_UNLOAD: 
        zxdh_flr_config(rf);
        zxdh_rdma_skip_hw_cfg(rf);
        break;
    case ZTE_ZXDH_OP_CONFIG_TRUST_TYPE:
        iwdev = rf->iwdev;
        iwdev->trust_type = addrbuf[2];

        pr_info("[zxdh_rdma] pf=%u, vf=%u, is_vf=%u change trust to %u.\n", 
            iwdev->rf->pf_id, iwdev->rf->vf_id, iwdev->rf->ftype, iwdev->trust_type);
        break;
    default:
        break;
    }
}

int zxdh_recv_mb(struct zxdh_sc_dev *dev, struct zxdh_ccq_cqe_info *info) 
{
	struct mailbox_work *work;
	struct zxdh_pci_f *rf;
	struct zxdh_device *iwdev;
	int i = 0;

	work = kzalloc(sizeof(*work), GFP_ATOMIC);
	if (!work) {
		pr_err("[zxdh_rdma] %s[%d]: kzalloc work failed!\n",__func__,__LINE__);
		return -ENODEV;
	}
	work->dev = dev;
	work->op_ret_val = info->op_ret_val;
	for (i = 0; i < ZXDH_MAILBOX_ADDR_BUF_LEN; i++) {
		work->addrbuf[i] = info->addrbuf[i];
	}
	rf = container_of(dev, struct zxdh_pci_f, sc_dev);
	iwdev = rf->iwdev;
	INIT_WORK(&work->work, zxdh_mailbox_worker);
	queue_work(iwdev->cleanup_wq, &work->work);
	return 0;
}

int zxdh_recv_vf_mb(struct zxdh_sc_dev *dev, struct zxdh_ccq_cqe_info *info) 
{
    struct mailbox_work *work;
    struct zxdh_pci_f *rf;
    struct zxdh_device *iwdev;
    int i = 0;

    work = kzalloc(sizeof(*work), GFP_ATOMIC);
    if (!work) {
        pr_err("[zxdh_rdma] %s[%d]: kzalloc work failed!\n",__func__,__LINE__);
        return -ENODEV;
    }
    work->dev = dev;
    work->op_ret_val = info->op_ret_val;
    for (i = 0; i < ZXDH_MAILBOX_ADDR_BUF_LEN; i++) {
        work->addrbuf[i] = info->addrbuf[i];
    }
    rf = container_of(dev, struct zxdh_pci_f, sc_dev);
    iwdev = rf->iwdev;
    INIT_WORK(&work->work, zxdh_vf_mailbox_worker);
    queue_work(iwdev->cleanup_wq, &work->work);
    return 0;
}

int zxdh_create_vf_hmc_objs(struct zxdh_sc_dev *dev,
			    struct zxdh_hmc_info *hmc_info, u8 type,
			    struct zxdh_hmc_create_obj_info *obj_info)
{
	int status = 0;

	if (hmc_info->hmc_obj[type].cnt) {
		obj_info->rsrc_type = type;
		obj_info->count = hmc_info->hmc_obj[obj_info->rsrc_type].cnt;
		status = zxdh_sc_create_hmc_obj(dev, obj_info);
		if (status) {
			zxdh_del_hmc_objects(dev, hmc_info);
			pr_err("[zxdh_rdma] ERR: create obj type %d status = %d\n",
			       iw_hmc_obj_types[obj_info->rsrc_type], status);
		}
	}

	return status;
}

int zxdh_vfhmc_enter(struct zxdh_sc_dev *dev, struct zxdh_hmc_info *vf_hmc_info)
{
	u32 sd_lmt = 0, hmc_entry_total = 0, j = 0, k = 0, mem_size = 0,
	    cnt = 0;
	u64 fpm_limit = 0;
	struct zxdh_hmc_info *hmc_info = NULL;
	struct zxdh_virt_mem virt_mem = {};
	struct zxdh_hmc_obj_info *obj_info = NULL;

	hmc_info = vf_hmc_info;
	obj_info = hmc_info->hmc_obj;

	for (k = ZXDH_HMC_IW_AH; k < ZXDH_HMC_IW_MAX; k++) {
		cnt = obj_info[k].cnt;

		fpm_limit = obj_info[k].size * cnt;

		if (fpm_limit == 0)
			continue;

		if (k == ZXDH_HMC_IW_PBLE)
			hmc_info->hmc_first_entry_pble = hmc_entry_total;

		if (k == ZXDH_HMC_IW_PBLE_MR)
			hmc_info->hmc_first_entry_pble_mr = hmc_entry_total;

		if ((fpm_limit % ZXDH_HMC_DIRECT_BP_SIZE) == 0) {
			sd_lmt = fpm_limit / ZXDH_HMC_DIRECT_BP_SIZE;
			sd_lmt += 1;
		} else {
			sd_lmt = (u32)((fpm_limit - 1) /
				       ZXDH_HMC_DIRECT_BP_SIZE);
			sd_lmt += 1;
		}

		if (sd_lmt == 1)
			hmc_entry_total++;
		else {
			for (j = 0; j < sd_lmt - 1; j++)
				hmc_entry_total++;

			if (fpm_limit % ZXDH_HMC_DIRECT_BP_SIZE)
				hmc_entry_total++;
		}
	}

	mem_size = sizeof(struct zxdh_hmc_sd_entry) * hmc_entry_total;
	virt_mem.size = mem_size;
	virt_mem.va = kzalloc(virt_mem.size, GFP_KERNEL);
	if (!virt_mem.va) {
		pr_err("[zxdh_rdma] HMC: failed to allocate memory for sd_entry buffer\n");
		return -ENOMEM;
	}
	hmc_info->sd_table.sd_entry = virt_mem.va;
	hmc_info->hmc_entry_total = hmc_entry_total;
	return 0;
}

int zxdh_sc_write_hmc_register(struct zxdh_sc_dev *dev,
			       struct zxdh_hmc_obj_info *obj_info,
			       u32 rsrc_type, u16 vhca_id)
{
	u32 base_low = 0, base_high = 0, val = 0;
	u64 base = 0;
	struct zxdh_sc_cqp *cqp = dev->cqp;
	struct zxdh_pci_f *rf;

	if (dev->cache_id > 3) {
		pr_info("[zxdh_rdma] cache id is error!!!\n");
		return -EACCES;
	}

	rf = container_of(dev, struct zxdh_pci_f, sc_dev);

	base = obj_info[rsrc_type].base;

	base = base / 512;
	base_low = (u32)(base & 0x00000000ffffffff);
	base_high = (u32)((base & 0xffffffff00000000) >> 32);

	switch (rsrc_type) {
	case ZXDH_HMC_IW_PBLE_MR:
		val = zxdh_hmc_register_config_comval(rf, dev, rsrc_type);
		dev->hmc_info->pble_mr_cachid = FIELD_GET(GENMASK_ULL(1, 0), val);
		writel(val, (u32 __iomem *)(cqp->dev->hw->hw_addr +
					    C_HMC_PBLEMR_TX1(cqp->dev->ep_id)));
		writel(val, (u32 __iomem *)(cqp->dev->hw->hw_addr +
					    C_HMC_PBLEMR_RX1(cqp->dev->ep_id)));
		writel(val, (u32 __iomem *)(cqp->dev->hw->hw_addr +
					    C_HMC_PBLEMR_RX2(cqp->dev->ep_id)));
		if (dev->hmc_use_dpu_ddr == true) {
			writel(ZXDH_INDICATE_DPU_DDR,
			       (u32 __iomem *)(cqp->dev->hw->hw_addr +
					       C_HMC_PBLEMR_RDMAIO_INDICATE(cqp->dev->ep_id)));
		} else {
			if (rf->use_ext_mem_flag)
				writel(ZXDH_INDICATE_HOST_NOSMMU,
					(u32 __iomem *)(cqp->dev->hw->hw_addr +
							C_HMC_PBLEMR_RDMAIO_INDICATE(cqp->dev->ep_id)));
			else
				writel(ZXDH_INDICATE_HOST_SMMU,
			       	(u32 __iomem *)(cqp->dev->hw->hw_addr +
					       	C_HMC_PBLEMR_RDMAIO_INDICATE(cqp->dev->ep_id)));
		}
		writel(base_low, (u32 __iomem *)(cqp->dev->hw->hw_addr +
						 C_HMC_PBLEMR_RDMAIO_BASE_LOW(cqp->dev->ep_id)));
		writel(base_high,
		       (u32 __iomem *)(cqp->dev->hw->hw_addr +
				       C_HMC_PBLEMR_RDMAIO_BASE_HIGH(cqp->dev->ep_id)));
		break;
	case ZXDH_HMC_IW_PBLE:
		val = zxdh_hmc_register_config_comval(rf, dev, rsrc_type);
		writel(val, (u32 __iomem *)(cqp->dev->hw->hw_addr +
					    C_HMC_PBLEQUEUE_TX1(cqp->dev->ep_id)));
		writel(val, (u32 __iomem *)(cqp->dev->hw->hw_addr +
					    C_HMC_PBLEQUEUE_TX2(cqp->dev->ep_id)));
		writel(val, (u32 __iomem *)(cqp->dev->hw->hw_addr +
					    RDMATX_DB_PBLE_ID_CFG(cqp->dev->ep_id)));

		writel(val, (u32 __iomem *)(cqp->dev->hw->hw_addr +
					    C_HMC_PBLEQUEUE_RX1(cqp->dev->ep_id)));
		writel(val, (u32 __iomem *)(cqp->dev->hw->hw_addr +
					    C_HMC_PBLEQUEUE_RX2(cqp->dev->ep_id)));
		writel(val, (u32 __iomem *)(cqp->dev->hw->hw_addr +
					    C_HMC_PBLEQUEUE_RX3(cqp->dev->ep_id)));
		writel(val, (u32 __iomem *)(cqp->dev->hw->hw_addr +
					    C_HMC_PBLEQUEUE_RX4(cqp->dev->ep_id)));
		writel(val, (u32 __iomem *)(cqp->dev->hw->hw_addr +
					    C_HMC_PBLEQUEUE_RX5(cqp->dev->ep_id)));
		if (dev->hmc_use_dpu_ddr == true) {
			writel(ZXDH_INDICATE_DPU_DDR,
			       (u32 __iomem *)(cqp->dev->hw->hw_addr +
					       C_HMC_PBLEQUEUE_RDMAIO_INDICATE(cqp->dev->ep_id)));
		} else {
			if (rf->use_ext_mem_flag)
				writel(ZXDH_INDICATE_HOST_NOSMMU,
					(u32 __iomem *)(cqp->dev->hw->hw_addr +
							C_HMC_PBLEQUEUE_RDMAIO_INDICATE(cqp->dev->ep_id)));
			else
				writel(ZXDH_INDICATE_HOST_SMMU,
					(u32 __iomem *)(cqp->dev->hw->hw_addr +
							C_HMC_PBLEQUEUE_RDMAIO_INDICATE(cqp->dev->ep_id)));
		}
		writel(base_low,
		       (u32 __iomem *)(cqp->dev->hw->hw_addr +
				       C_HMC_PBLEQUEUE_RDMAIO_BASE_LOW(cqp->dev->ep_id)));
		writel(base_high,
		       (u32 __iomem *)(cqp->dev->hw->hw_addr +
				       C_HMC_PBLEQUEUE_RDMAIO_BASE_HIGH(cqp->dev->ep_id)));
		break;
	case ZXDH_HMC_IW_MR:
		val = zxdh_hmc_register_config_comval(rf, dev, rsrc_type);
		writel(val,
		       (u32 __iomem *)(cqp->dev->hw->hw_addr + C_HMC_MRTE_TX1(cqp->dev->ep_id)));
		writel(val,
		       (u32 __iomem *)(cqp->dev->hw->hw_addr + C_HMC_MRTE_TX3(cqp->dev->ep_id)));

		writel(val,
		       (u32 __iomem *)(cqp->dev->hw->hw_addr + C_HMC_MRTE_RX1(cqp->dev->ep_id)));
		writel(val,
		       (u32 __iomem *)(cqp->dev->hw->hw_addr + C_HMC_MRTE_RX2(cqp->dev->ep_id)));

		val = zxdh_hmc_register_config_cqpval(
			dev, obj_info[rsrc_type].max_cnt, rsrc_type);
		writel(val,
		       (u32 __iomem *)(cqp->dev->hw->hw_addr + C_HMC_MRTE_CQP(cqp->dev->ep_id)));

		if (dev->hmc_use_dpu_ddr == true) {
			writel(ZXDH_INDICATE_DPU_DDR,
			       (u32 __iomem *)(cqp->dev->hw->hw_addr +
					       C_HMC_MRTE_RDMAIO_INDICATE(cqp->dev->ep_id)));
		} else {
			if (rf->use_ext_mem_flag)
				writel(ZXDH_INDICATE_HOST_NOSMMU,
					(u32 __iomem *)(cqp->dev->hw->hw_addr +
							C_HMC_MRTE_RDMAIO_INDICATE(cqp->dev->ep_id)));
			else
				writel(ZXDH_INDICATE_HOST_SMMU,
					(u32 __iomem *)(cqp->dev->hw->hw_addr +
							C_HMC_MRTE_RDMAIO_INDICATE(cqp->dev->ep_id)));
		}
		writel(base_low, (u32 __iomem *)(cqp->dev->hw->hw_addr +
						 C_HMC_MRTE_RDMAIO_BASE_LOW(cqp->dev->ep_id)));
		writel(base_high, (u32 __iomem *)(cqp->dev->hw->hw_addr +
						  C_HMC_MRTE_RDMAIO_BASE_HIGH(cqp->dev->ep_id)));
		break;
	case ZXDH_HMC_IW_AH:

		val = zxdh_hmc_register_config_comval(rf, dev, rsrc_type);
		writel(val,
		       (u32 __iomem *)(cqp->dev->hw->hw_addr + C_HMC_AH_TX(cqp->dev->ep_id)));

		val = zxdh_hmc_register_config_cqpval(
			dev, obj_info[rsrc_type].max_cnt, rsrc_type);
		writel(val,
		       (u32 __iomem *)(cqp->dev->hw->hw_addr + C_HMC_AH_CQP(cqp->dev->ep_id)));

		writel(base_low, (u32 __iomem *)(cqp->dev->hw->hw_addr +
						 C_HMC_AH_RDMAIO_BASE_LOW(cqp->dev->ep_id)));
		writel(base_high, (u32 __iomem *)(cqp->dev->hw->hw_addr +
						  C_HMC_AH_RDMAIO_BASE_HIGH(cqp->dev->ep_id)));
		if (dev->hmc_use_dpu_ddr == true) {
			writel(ZXDH_INDICATE_DPU_DDR,
			       (u32 __iomem *)(cqp->dev->hw->hw_addr +
					       C_HMC_AH_RDMAIO_INDICATE(cqp->dev->ep_id)));
		} else {
			if (rf->use_ext_mem_flag)
				writel(ZXDH_INDICATE_HOST_NOSMMU,
					(u32 __iomem *)(cqp->dev->hw->hw_addr +
							C_HMC_AH_RDMAIO_INDICATE(cqp->dev->ep_id)));
			else
				writel(ZXDH_INDICATE_HOST_SMMU,
					(u32 __iomem *)(cqp->dev->hw->hw_addr +
							C_HMC_AH_RDMAIO_INDICATE(cqp->dev->ep_id)));
		}
		break;
	case ZXDH_HMC_IW_IRD:
		val = zxdh_hmc_register_config_comval(rf, dev, rsrc_type);
		dev->hmc_info->pble_ird_cachid = FIELD_GET(GENMASK_ULL(1, 0), val);
		writel(val,
		       (u32 __iomem *)(cqp->dev->hw->hw_addr + C_HMC_IRD_RX1(cqp->dev->ep_id)));
		writel(val,
		       (u32 __iomem *)(cqp->dev->hw->hw_addr + C_HMC_IRD_RX2(cqp->dev->ep_id)));
		writel(val,
		       (u32 __iomem *)(cqp->dev->hw->hw_addr + C_HMC_IRD_RX3(cqp->dev->ep_id)));

		writel(base_low, (u32 __iomem *)(cqp->dev->hw->hw_addr +
						 C_HMC_IRD_RDMAIO_BASE_LOW(cqp->dev->ep_id)));
		writel(base_high, (u32 __iomem *)(cqp->dev->hw->hw_addr +
						  C_HMC_IRD_RDMAIO_BASE_HIGH(cqp->dev->ep_id)));
		if (dev->hmc_use_dpu_ddr == true) {
			writel(ZXDH_INDICATE_DPU_DDR,
			       (u32 __iomem *)(cqp->dev->hw->hw_addr +
					       C_HMC_IRD_RDMAIO_INDICATE(cqp->dev->ep_id)));
		} else {
			if (rf->use_ext_mem_flag)
				writel(ZXDH_INDICATE_HOST_NOSMMU,
					(u32 __iomem *)(cqp->dev->hw->hw_addr +
							C_HMC_IRD_RDMAIO_INDICATE(cqp->dev->ep_id)));
			else
				writel(ZXDH_INDICATE_HOST_SMMU,
					(u32 __iomem *)(cqp->dev->hw->hw_addr +
							C_HMC_IRD_RDMAIO_INDICATE(cqp->dev->ep_id)));
		}
		break;
	case ZXDH_HMC_IW_TXWINDOW:

		val = zxdh_hmc_register_config_comval(rf, dev, rsrc_type);
		writel(val, (u32 __iomem *)(cqp->dev->hw->hw_addr +
					    C_HMC_TX_WINDOW_TX(cqp->dev->ep_id)));
		if (dev->hmc_use_dpu_ddr == true) {
			writel(ZXDH_INDICATE_DPU_DDR,
			       (u32 __iomem *)(cqp->dev->hw->hw_addr +
					       C_HMC_TX_WINDOW_RDMAIO_INDICATE(cqp->dev->ep_id)));
		} else {
			if (rf->use_ext_mem_flag)
				writel(ZXDH_INDICATE_HOST_NOSMMU,
					(u32 __iomem *)(cqp->dev->hw->hw_addr +
							C_HMC_TX_WINDOW_RDMAIO_INDICATE(cqp->dev->ep_id)));
			else
				writel(ZXDH_INDICATE_HOST_SMMU,
					(u32 __iomem *)(cqp->dev->hw->hw_addr +
							C_HMC_TX_WINDOW_RDMAIO_INDICATE(cqp->dev->ep_id)));
		}
		writel(base_low,
		       (u32 __iomem *)(cqp->dev->hw->hw_addr +
				       C_HMC_TX_WINDOW_RDMAIO_BASE_LOW(cqp->dev->ep_id)));
		writel(base_high,
		       (u32 __iomem *)(cqp->dev->hw->hw_addr +
				       C_HMC_TX_WINDOW_RDMAIO_BASE_HIGH(cqp->dev->ep_id)));
		break;
	case ZXDH_HMC_IW_QP:
		val = zxdh_hmc_register_config_comval(rf, dev, rsrc_type);
		writel(val,
		       (u32 __iomem *)(cqp->dev->hw->hw_addr + C_HMC_QPC_RX(cqp->dev->ep_id)));

		writel(base_low, (u32 __iomem *)(cqp->dev->hw->hw_addr +
						 C_HMC_QPC_RX_BASE_LOW(cqp->dev->ep_id)));
		writel(base_high, (u32 __iomem *)(cqp->dev->hw->hw_addr +
						  C_HMC_QPC_RX_BASE_HIGH(cqp->dev->ep_id)));
		writel(val,
		       (u32 __iomem *)(cqp->dev->hw->hw_addr + C_HMC_QPC_TX(cqp->dev->ep_id)));

		writel(base_low, (u32 __iomem *)(cqp->dev->hw->hw_addr +
						 C_HMC_QPC_TX_BASE_LOW(cqp->dev->ep_id)));
		writel(base_high, (u32 __iomem *)(cqp->dev->hw->hw_addr +
						  C_HMC_QPC_TX_BASE_HIGH(cqp->dev->ep_id)));
		break;
	case ZXDH_HMC_IW_SRQ:
		val = zxdh_hmc_register_config_comval(rf, dev, rsrc_type);
		writel(val,
		       (u32 __iomem *)(cqp->dev->hw->hw_addr + C_HMC_SRQC_RX(cqp->dev->ep_id)));
		writel(base_low, (u32 __iomem *)(cqp->dev->hw->hw_addr +
						 C_HMC_SRQC_RX_BASE_LOW(cqp->dev->ep_id)));
		writel(base_high, (u32 __iomem *)(cqp->dev->hw->hw_addr +
						  C_HMC_SRQC_RX_BASE_HIGH(cqp->dev->ep_id)));
		break;
	case ZXDH_HMC_IW_CQ:
		val = zxdh_hmc_register_config_comval(rf, dev, rsrc_type);
		writel(val,
		       (u32 __iomem *)(cqp->dev->hw->hw_addr + C_HMC_CQC_RX1(cqp->dev->ep_id)));
		writel(val,
		       (u32 __iomem *)(cqp->dev->hw->hw_addr + C_HMC_CQC_RX2(cqp->dev->ep_id)));
		writel(base_low, (u32 __iomem *)(cqp->dev->hw->hw_addr +
						 C_HMC_CQC_RX_BASE_LOW(cqp->dev->ep_id)));
		writel(base_high, (u32 __iomem *)(cqp->dev->hw->hw_addr +
						  C_HMC_CQC_RX_BASE_HIGH(cqp->dev->ep_id)));
		break;
	default:
		break;
	}
	return 0;
}

int zxdh_ext_sc_write_hmc_register(struct zxdh_sc_dev *dev,
			       struct zxdh_hmc_obj_info *obj_info,
			       u32 rsrc_type, u16 vhca_id)
{
	struct zxdh_pci_f *rf = dev_to_rf(dev);
	struct zxdh_sc_dev *ext_dev = &rf->ext_sc_dev;
	u32 base_low = 0, base_high = 0, val = 0;
	u64 base = 0;

	if (dev->cache_id > 3) {
		pr_err("[zxdh_rdma] cache id is error!!!\n");
		return -EACCES;
	}

	base = obj_info[rsrc_type].base;

	base = base / 512;
	base_low = (u32)(base & 0x00000000ffffffff);
	base_high = (u32)((base & 0xffffffff00000000) >> 32);

	switch (rsrc_type) {
	case ZXDH_HMC_IW_PBLE_MR:
		val = zxdh_hmc_register_config_comval(rf, ext_dev, rsrc_type);
		writel(val, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
					    C_EXT_HMC_PBLEMR_TX1(ext_dev->ep_id)));
		writel(val, (u32 __iomem *)(ext_dev->hw->ext_hw_addr+
					    C_EXT_HMC_PBLEMR_RX1(ext_dev->ep_id)));
		writel(val, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
					    C_EXT_HMC_PBLEMR_RX2(ext_dev->ep_id)));
		if (ext_dev->hmc_use_dpu_ddr == true) {
			writel(ZXDH_INDICATE_DPU_DDR,
			       (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
					       C_EXT_HMC_PBLEMR_RDMAIO_INDICATE(ext_dev->ep_id)));
		} else {
			writel(ZXDH_INDICATE_HOST_SMMU,
				(u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						C_EXT_HMC_PBLEMR_RDMAIO_INDICATE(ext_dev->ep_id)));
		}
		writel(base_low, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
					    C_EXT_HMC_PBLEMR_RDMAIO_BASE_LOW(ext_dev->ep_id)));
		writel(base_high,
		       (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
				       C_EXT_HMC_PBLEMR_RDMAIO_BASE_HIGH(ext_dev->ep_id)));
		break;
	case ZXDH_HMC_IW_PBLE:
		val = zxdh_hmc_register_config_comval(rf, ext_dev, rsrc_type);
		writel(val, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
					    C_EXT_HMC_PBLEQUEUE_TX1(ext_dev->ep_id)));
		writel(val, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
					    C_EXT_HMC_PBLEQUEUE_TX2(ext_dev->ep_id)));
		writel(val, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
					    RDMATX_EXT_DB_PBLE_ID_CFG(ext_dev->ep_id)));

		writel(val, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
					    C_EXT_HMC_PBLEQUEUE_RX1(ext_dev->ep_id)));
		writel(val, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
					    C_EXT_HMC_PBLEQUEUE_RX2(ext_dev->ep_id)));
		writel(val, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
					    C_EXT_HMC_PBLEQUEUE_RX3(ext_dev->ep_id)));
		writel(val, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
					    C_EXT_HMC_PBLEQUEUE_RX4(ext_dev->ep_id)));
		writel(val, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
					    C_EXT_HMC_PBLEQUEUE_RX5(ext_dev->ep_id)));
		if (ext_dev->hmc_use_dpu_ddr == true) {
			writel(ZXDH_INDICATE_DPU_DDR,
			       (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
					       C_EXT_HMC_PBLEQUEUE_RDMAIO_INDICATE(ext_dev->ep_id)));
		} else {
			writel(ZXDH_INDICATE_HOST_SMMU,
				(u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						C_EXT_HMC_PBLEQUEUE_RDMAIO_INDICATE(ext_dev->ep_id)));
		}
		writel(base_low,
		       (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
				       C_EXT_HMC_PBLEQUEUE_RDMAIO_BASE_LOW(ext_dev->ep_id)));
		writel(base_high,
		       (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
				       C_EXT_HMC_PBLEQUEUE_RDMAIO_BASE_HIGH(ext_dev->ep_id)));
		break;
	case ZXDH_HMC_IW_MR:
		val = zxdh_hmc_register_config_comval(rf, ext_dev, rsrc_type);
		writel(val,
		       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_EXT_HMC_MRTE_TX1(ext_dev->ep_id)));
		writel(val,
		       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_EXT_HMC_MRTE_TX3(ext_dev->ep_id)));

		writel(val,
		       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_EXT_HMC_MRTE_RX1(ext_dev->ep_id)));
		writel(val,
		       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_EXT_HMC_MRTE_RX2(ext_dev->ep_id)));

		val = zxdh_hmc_register_config_cqpval(
			ext_dev, obj_info[rsrc_type].max_cnt, rsrc_type);
		writel(val,
		       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_EXT_HMC_MRTE_CQP(ext_dev->ep_id)));

		if (ext_dev->hmc_use_dpu_ddr == true) {
			writel(ZXDH_INDICATE_DPU_DDR,
			       (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
					       C_EXT_HMC_MRTE_RDMAIO_INDICATE(ext_dev->ep_id)));
		} else {
			writel(ZXDH_INDICATE_HOST_SMMU,
				(u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						C_EXT_HMC_MRTE_RDMAIO_INDICATE(ext_dev->ep_id)));
		}
		writel(base_low, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						 C_EXT_HMC_MRTE_RDMAIO_BASE_LOW(ext_dev->ep_id)));
		writel(base_high, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						  C_EXT_HMC_MRTE_RDMAIO_BASE_HIGH(ext_dev->ep_id)));
		break;
	case ZXDH_HMC_IW_AH:

		val = zxdh_hmc_register_config_comval(rf, ext_dev, rsrc_type);
		writel(val,
		       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_EXT_HMC_AH_TX(ext_dev->ep_id)));

		val = zxdh_hmc_register_config_cqpval(
			ext_dev, obj_info[rsrc_type].max_cnt, rsrc_type);
		writel(val,
		       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_EXT_HMC_AH_CQP(ext_dev->ep_id)));

		writel(base_low, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						 C_EXT_HMC_AH_RDMAIO_BASE_LOW(ext_dev->ep_id)));
		writel(base_high, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						  C_EXT_HMC_AH_RDMAIO_BASE_HIGH(ext_dev->ep_id)));
		if (ext_dev->hmc_use_dpu_ddr == true) {
			writel(ZXDH_INDICATE_DPU_DDR,
			       (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
					       C_EXT_HMC_AH_RDMAIO_INDICATE(ext_dev->ep_id)));
		} else {
			writel(ZXDH_INDICATE_HOST_SMMU,
				(u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						C_EXT_HMC_AH_RDMAIO_INDICATE(ext_dev->ep_id)));
		}
		break;
	case ZXDH_HMC_IW_IRD:

		val = zxdh_hmc_register_config_comval(rf, ext_dev, rsrc_type);
		writel(val,
		       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_EXT_HMC_IRD_RX1(ext_dev->ep_id)));
		writel(val,
		       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_EXT_HMC_IRD_RX2(ext_dev->ep_id)));
		writel(val,
		       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_EXT_HMC_IRD_RX3(ext_dev->ep_id)));

		writel(base_low, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						 C_EXT_HMC_IRD_RDMAIO_BASE_LOW(ext_dev->ep_id)));
		writel(base_high, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						  C_EXT_HMC_IRD_RDMAIO_BASE_HIGH(ext_dev->ep_id)));
		if (ext_dev->hmc_use_dpu_ddr == true) {
			writel(ZXDH_INDICATE_DPU_DDR,
			       (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
					       C_EXT_HMC_IRD_RDMAIO_INDICATE(ext_dev->ep_id)));
		} else {
			writel(ZXDH_INDICATE_HOST_SMMU,
				(u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						C_EXT_HMC_IRD_RDMAIO_INDICATE(ext_dev->ep_id)));
		}
		break;
	case ZXDH_HMC_IW_TXWINDOW:
		val = zxdh_hmc_register_config_comval(rf, ext_dev, rsrc_type);
		writel(val, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
					    C_EXT_HMC_TX_WINDOW_TX(ext_dev->ep_id)));
		if (ext_dev->hmc_use_dpu_ddr == true) {
			writel(ZXDH_INDICATE_DPU_DDR,
			       (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
					       C_EXT_HMC_TX_WINDOW_RDMAIO_INDICATE(ext_dev->ep_id)));
		} else {
			writel(ZXDH_INDICATE_HOST_SMMU,
				(u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						C_EXT_HMC_TX_WINDOW_RDMAIO_INDICATE(ext_dev->ep_id)));
		}
		writel(base_low,
		       (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
				       C_EXT_HMC_TX_WINDOW_RDMAIO_BASE_LOW(ext_dev->ep_id)));
		writel(base_high,
		       (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
				       C_EXT_HMC_TX_WINDOW_RDMAIO_BASE_HIGH(ext_dev->ep_id)));
		break;
	case ZXDH_HMC_IW_QP:
		val = zxdh_hmc_register_config_comval(rf, ext_dev, rsrc_type);
		writel(val,
		       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_EXT_HMC_QPC_RX(ext_dev->ep_id)));
		writel(base_low, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						 C_EXT_HMC_QPC_RX_BASE_LOW(ext_dev->ep_id)));
		writel(base_high, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						  C_EXT_HMC_QPC_RX_BASE_HIGH(ext_dev->ep_id)));
		writel(val,
		       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_EXT_HMC_QPC_TX(ext_dev->ep_id)));

		writel(base_low, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						 C_EXT_HMC_QPC_TX_BASE_LOW(ext_dev->ep_id)));
		writel(base_high, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						  C_EXT_HMC_QPC_TX_BASE_HIGH(ext_dev->ep_id)));
		break;
	case ZXDH_HMC_IW_SRQ:
		val = zxdh_hmc_register_config_comval(rf, ext_dev, rsrc_type);
		writel(val,
		       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_EXT_HMC_SRQC_RX(ext_dev->ep_id)));
		writel(base_low, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						 C_EXT_HMC_SRQC_RX_BASE_LOW(ext_dev->ep_id)));
		writel(base_high, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						  C_EXT_HMC_SRQC_RX_BASE_HIGH(ext_dev->ep_id)));
		break;
	case ZXDH_HMC_IW_CQ:
		val = zxdh_hmc_register_config_comval(rf, ext_dev, rsrc_type);
		writel(val,
		       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_EXT_HMC_CQC_RX1(ext_dev->ep_id)));
		writel(val,
		       (u32 __iomem *)(ext_dev->hw->ext_hw_addr + C_EXT_HMC_CQC_RX2(ext_dev->ep_id)));
		writel(base_low, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						 C_EXT_HMC_CQC_RX_BASE_LOW(ext_dev->ep_id)));
		writel(base_high, (u32 __iomem *)(ext_dev->hw->ext_hw_addr +
						  C_EXT_HMC_CQC_RX_BASE_HIGH(ext_dev->ep_id)));
		break;
	default:
		break;
	}
	return 0;
}

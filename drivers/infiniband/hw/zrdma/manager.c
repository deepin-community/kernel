// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#include <linux/pci.h>
#include <linux/dmi.h>
//#include "/home/chenhuan/code/rdma_dev/zxdh_kernel/incldue/linux/dinghai/dh_cmd.h"
//#include "dh_cmd.h"
#include "iidc.h"
#include "main.h"
#include "manager.h"
#include "icrdma_hw.h"
#include <net/bonding.h>

#ifdef CONFIG_X86
#include <asm/cpufeature.h>
#endif

/* bit7~4: ep_id, bit3~0: pf_id */
#define DH_MP_SPEED_EPID_PFID_GEN(ep_id, pf_id) (0xF & pf_id) | ((0xF & ep_id) << 4)
#define DH_MP_SPEED_EPID_EXTRACT(data) (((data) & 0xF0) >> 4)
#define DH_MP_SPEED_PFID_EXTRACT(data) ((data) & 0xF)
#define DH_MP_SPEED_PFID1    9

u64 zxdh_hw_bar_pages[8][C_RDMA_HW_BAR_PAGE_NUM] = { { 0 } };
u64 zxdh_ext_hw_bar_pages[8][C_RDMA_EXT_HW_BAR_PAGE_NUM] = { { 0 } };

struct zxdh_rdma_hb_if hwbond_ops = {
	.cfg_rdma_hb_master = switch_bound_master_netdev,
	.cfg_rdma_hb_speed = set_rdma_firmware_speed,
};

int dh_rdma_pf_pcie_id_get(struct zxdh_mgr *mgr)
{
	u32 pos = 0;
	u8 type = 0;
	u16 padding = 0;
	struct pci_dev *pdev = mgr->pdev;

	if(NULL == pdev) {
		pr_err("[zxdh_rdma] %s[%d]: pdev is null\n", __func__, __LINE__);
		return -1;
	}

	for (pos = pci_find_capability(pdev, PCI_CAP_ID_VNDR); pos > 0;
	     pos = pci_find_next_capability(pdev, pos, PCI_CAP_ID_VNDR)) {
		pci_read_config_byte(
			pdev, pos + offsetof(struct zxdh_pf_pci_cap, cfg_type),
			&type);

		if (type == ZXDH_PCI_CAP_PCI_CFG) {
			pci_read_config_word(
				pdev,
				pos + offsetof(struct zxdh_pf_pci_cap,
					       padding[0]),
				&padding);
			mgr->pcie_id = padding;
			return 0;
		}
	}
	return -1;
}

int zxdh_chan_sync_send(struct zxdh_mgr *pmgr, struct zxdh_chan_msg *pmsg,
			u32 *pdata, u32 rep_len)
{
	u16 buffer_len = 0;
	void *recv_buffer = NULL;
	int ret = 0;
	u8 *reply_ptr = NULL;
	u16 reply_msg_len = 0;
	u32 cnt = 0;

	struct zxdh_pci_bar_msg in = { 0 };
	struct zxdh_msg_recviver_mem result = { 0 };

	if (pmgr == NULL || pmsg == NULL || pdata == NULL)
		return -1;

	buffer_len = rep_len + ZXDH_CHAN_REPS_LEN;
	recv_buffer = (void *)kmalloc(buffer_len, GFP_KERNEL);
	if (recv_buffer == NULL)
		return -1;

	in.virt_addr =
		(u64)pmgr->pci_hw_addr + ZXDH_BAR_CHAN_OFFSET; //bar空间偏移?
	in.payload_addr = pmsg->msg;
	in.payload_len = pmsg->msg_len;

	if (!pmgr->ftype)
		in.src = MSG_CHAN_END_PF;
	else
		in.src = MSG_CHAN_END_VF;

	in.dst = MSG_CHAN_END_RISC;
	in.event_id = MODULE_RDMA;

	if (0 == dh_rdma_pf_pcie_id_get(pmgr))
		in.src_pcieid = pmgr->pcie_id;
	else {
		kfree(recv_buffer);
		return -1;
	}

	result.buffer_len = buffer_len;
	result.recv_buffer = recv_buffer;

	do {
		ret = zxdh_bar_chan_sync_msg_send(&in, &result);
		if ((ret != ZXDH_BAR_ERR_TIME_OUT) && (ret != ZXDH_BAR_ERR_LOCK_FAILED)) {
			break;
		}
		cnt++;
	}while(cnt < ZXDH_BAR_MSG_RETRY_NUM);
	
	if (ret != 0) {
		pr_err("[zxdh_rdma] zxdh_bar_chan_sync_msg_send faile, ret=%d cnt=%d\n", ret, cnt);
		kfree(recv_buffer);
		return -1;
	}

	reply_ptr = (u8 *)result.recv_buffer;
	if (*reply_ptr == MSG_REP_VALID) {
		reply_msg_len = *(u16 *)(reply_ptr + MSG_REP_LEN_OFFSET);
		memcpy(pdata, reply_ptr + ZXDH_CHAN_REPS_LEN,
		       ((reply_msg_len > rep_len) ? rep_len : reply_msg_len));
		kfree(recv_buffer);
		return 0;
	}

	kfree(recv_buffer);
	return 0;
}

int zxdh_mgr_par_get(struct zxdh_mgr *dh_mgr)
{
    int ret = 0;

    struct zxdh_mgr_msg *cmd;
    struct zxdh_chan_msg *pmsg;
    struct zxdh_mgr_par param;

    cmd = kzalloc(sizeof(struct zxdh_mgr_msg), GFP_KERNEL);
    if (!cmd)
        return -ENOMEM;

    pmsg = kzalloc(sizeof(struct zxdh_chan_msg), GFP_KERNEL);
    if (!pmsg) {
        kfree(cmd);
        return -ENOMEM;
    }

	cmd->op_code = 0;
	cmd->pf_id = dh_mgr->pf_id;
	cmd->vport_vf_id = dh_mgr->vport_vf_id;
	cmd->ftype = dh_mgr->ftype;
	cmd->ep_id = dh_mgr->ep_id;

	pmsg->msg_len = sizeof(struct zxdh_mgr_msg);
	pmsg->msg = (void *)cmd;

	ret = zxdh_chan_sync_send(dh_mgr, pmsg, (void *)&dh_mgr->param,
				  sizeof(struct zxdh_mgr_par));
	param = dh_mgr->param;
	pr_info("[zxdh_rdma] mgr cfg param:");
	pr_info("[zxdh_rdma] ftype=%d, ep_id=%d, pf_id=%d, max_vf_num=%d, vhca_id=%d, bar_offset=0x%x.\n",
		param.ftype, param.ep_id, param.pf_id, param.max_vf_num,
		param.vhca_id, param.bar_offset);
	pr_info("[zxdh_rdma] l2d_smmu_addr=0x%llx, vf_id=%d, vhca_id_pf=%d, l2d_smmu_l2_offset=%d.\n",
		param.l2d_smmu_addr, param.vf_id, param.vhca_id_pf,
		param.l2d_smmu_l2_offset);
	pr_info("[zxdh_rdma] qp_cnt=%d, cq_cnt=%d, srq_cnt=%d, ceq_cnt=%d, ah_cnt=%d, mr_cnt=%d, pbleq_cnt=%d, pblem_cnt=%d.\n",
		param.qp_cnt, param.cq_cnt, param.srq_cnt, param.ceq_cnt,
		param.ah_cnt, param.mr_cnt, param.pbleq_cnt, param.pblem_cnt);
	pr_info("[zxdh_rdma] base_qpn=%d, base_cqn=%d, base_srqn=%d, base_ceqn=%d.\n",
		param.base_qpn, param.base_cqn, param.base_srqn,
		param.base_ceqn);
	pr_info("[zxdh_rdma] qp_hmc_base=0x%llx, cq_hmc_base=0x%llx, srq_hmc_base=0x%llx, txwindow_hmc_base=0x%llx.\n",
		param.qp_hmc_base, param.cq_hmc_base, param.srq_hmc_base,
		param.txwindow_hmc_base);
	pr_info("[zxdh_rdma] ird_hmc_base=0x%llx,ah_hmc_base=0x%llx,mr_hmc_base=0x%llx,pbleq_hmc_base=0x%llx,pblem_hmc_base=0x%llx.\n",
		param.ird_hmc_base, param.ah_hmc_base, param.mr_hmc_base,
		param.pbleq_hmc_base, param.pblem_hmc_base);
	pr_info("[zxdh_rdma] mcode_type=%d,  chip_version=%d, max_hw_wq_frags=%d, max_hw_read_sges=%d\n",
		param.mcode_type, param.chip_version, param.max_hw_wq_frags,
		param.max_hw_read_sges);

	if (ret != 0) {
		pr_info("[zxdh_rdma] get pf param failed, ret=%d.\n", ret);
		kfree(cmd);
		kfree(pmsg);
		return -EPIPE;
	}

	if (param.ftype != dh_mgr->ftype || param.ep_id != dh_mgr->ep_id ||
	    param.pf_id != dh_mgr->pf_id) {
		
		if(param.vf_id >= param.max_vf_num){
			pr_warn("[zxdh_rdma]: Enabled VF number exceeds the RDMA max_vf_num:%u\n", param.max_vf_num);
			ret = -ENODEV;
		} else {
			pr_err("[zxdh_rdma] mgr cfg param not match, param.ftype=%d dh_mgr->ftype=%d param.ep_id=%d dh_mgr->ep_id=%d param.pf_id=%d dh_mgr->pf_id=%d.\n",
				param.ftype, dh_mgr->ftype, param.ep_id, dh_mgr->ep_id, param.pf_id, dh_mgr->pf_id);
			ret = -EPIPE;
		}
		kfree(cmd);
		kfree(pmsg);
		return ret;
	}

	kfree(cmd);
	kfree(pmsg);

	return 0;
}

static int zxdh_init_8k_index(struct zxdh_sc_dev *dev)
{
	u16 pri_8k_index_start = dev->vhca_8k_index_start;
	u16 pri_8k_index_cnt[8] = {};
	u8 pri_index;

	// 如果 rc_8k_idx 不是 rc_gqp 的4倍或者5倍，就有异常
	if ((dev->vhca_8k_index_cnt != (4 * dev->vhca_gqp_cnt)) &&
	    (dev->vhca_8k_index_cnt != (5 * dev->vhca_gqp_cnt))) {
		pr_err("[%s] ERR: rc_8k_idx is not 4 times or 5 times of rc_gqp\n", __func__);
		return -EINVAL;
	}

	// rc_8k_index 尽可能按照 1 1 1/2 1/2 1/4 1/4 1/4 1/4 * gqp数量分配
	pri_8k_index_cnt[0] = dev->vhca_gqp_cnt;
	pri_8k_index_cnt[1] = dev->vhca_gqp_cnt;

	pri_8k_index_cnt[2] = DIV_ROUND_UP(dev->vhca_gqp_cnt, 2);
	pri_8k_index_cnt[3] = dev->vhca_gqp_cnt / 2;

	pri_8k_index_cnt[4] = DIV_ROUND_UP(pri_8k_index_cnt[2], 2);
	pri_8k_index_cnt[5] = DIV_ROUND_UP(pri_8k_index_cnt[3], 2);

	pri_8k_index_cnt[6] = pri_8k_index_cnt[2] / 2;
	pri_8k_index_cnt[7] = pri_8k_index_cnt[3] / 2;

	// 兼容旧固件处理
	// rc_8k_index 尽可能按照 1 1 1 1 1/4 1/4 1/4 1/4 * gqp数量分配
	if (dev->vhca_8k_index_cnt == 5 * dev->vhca_gqp_cnt) {
		pri_8k_index_cnt[2] = dev->vhca_gqp_cnt;
		pri_8k_index_cnt[3] = dev->vhca_gqp_cnt;
	}

	for (pri_index = 0; pri_index < ZXDH_MAX_USER_PRIORITY; pri_index++)
	{
		dev->pri_8k_index_info[pri_index].pri_8k_index_start = pri_8k_index_start;
		dev->pri_8k_index_info[pri_index].pri_8k_index_cnt = pri_8k_index_cnt[pri_index];
		pr_info("[zxdh_rdma] pri_index:%d 8k_start:%d 8k_cnt:%d\n", pri_index, pri_8k_index_start, pri_8k_index_cnt[pri_index]);
		pri_8k_index_start += pri_8k_index_cnt[pri_index];
	}

	return 0;
}

static int get_pri_8k_idx_oft_from_gqp(struct zxdh_pci_f *rf, u16 gqp, u16 pri_index, u16* pri_8k_idx_oft) 
{
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	u16 vhca_gqp_start = dev->vhca_gqp_start;
	u16 vhca_gqp_cnt = dev->vhca_gqp_cnt;
	u16 pri_8k_idx_start;
	u16 pri_8k_idx_cnt;
    u16 target_remainder;
    u16 start_remainder;
    u16 rc_8k_idx_oft;
	*pri_8k_idx_oft = 0;

    if (pri_index >= ZXDH_MAX_USER_PRIORITY || vhca_gqp_cnt == 0) {
        return -1;
    }

	pri_8k_idx_start = dev->pri_8k_index_info[pri_index].pri_8k_index_start;
	pri_8k_idx_cnt = dev->pri_8k_index_info[pri_index].pri_8k_index_cnt;
	target_remainder = gqp - vhca_gqp_start;
	start_remainder = pri_8k_idx_start % vhca_gqp_cnt;

    if (start_remainder <= target_remainder) {
        rc_8k_idx_oft = target_remainder - start_remainder;
    } else {
        rc_8k_idx_oft = vhca_gqp_cnt - start_remainder + target_remainder;
    }
    
    if (rc_8k_idx_oft <= pri_8k_idx_cnt) {
		*pri_8k_idx_oft = rc_8k_idx_oft;
		return 0;
    }
    
    return -1;
}

static int zxdh_calculate_8k_index_offset(struct zxdh_pci_f *rf)
{
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	u16 pri_gqp_start[ZXDH_MAX_PRI_NUM] = {};
	u16 pri_8k_idx_s;
	u16 continuous_gqp_cnt;
	u8 pri_index;

	for (pri_index = 0; pri_index < ZXDH_MAX_PRI_NUM; pri_index++)
	{
		rf->pri_8k_idx_oft[pri_index] = 0;
	}

	if (dev->vhca_8k_index_cnt != (4 * dev->vhca_gqp_cnt)) {
		return 0;
	}

	if (rf->max_rdma_vfs > 64) {
		return 0;
	}

	if (dev->pri_8k_index_info[ZXDH_PRI_IDX_7].pri_8k_index_cnt < 3) {
		return 0;
	}

	for (pri_index = 0; pri_index < ZXDH_MAX_PRI_NUM; pri_index++) {
		pri_8k_idx_s = dev->pri_8k_index_info[pri_index].pri_8k_index_start;
		pri_gqp_start[pri_index] = get_rc_gqp_from_8k_index(pri_8k_idx_s, dev->vhca_gqp_start, dev->vhca_gqp_cnt);
	}
	continuous_gqp_cnt = (dev->pri_8k_index_info[ZXDH_PRI_IDX_7].pri_8k_index_cnt - 1) / 2;
	pri_gqp_start[ZXDH_PRI_IDX_0] = pri_gqp_start[ZXDH_PRI_IDX_7] + continuous_gqp_cnt;
	pri_gqp_start[ZXDH_PRI_IDX_1] = pri_gqp_start[ZXDH_PRI_IDX_5] + continuous_gqp_cnt;
	pri_gqp_start[ZXDH_PRI_IDX_2] = pri_gqp_start[ZXDH_PRI_IDX_4] + continuous_gqp_cnt;
	pri_gqp_start[ZXDH_PRI_IDX_3] = pri_gqp_start[ZXDH_PRI_IDX_6] + continuous_gqp_cnt;

	for (pri_index = 0; pri_index < ZXDH_MAX_PRI_NUM; pri_index++) {
		if (get_pri_8k_idx_oft_from_gqp(rf, pri_gqp_start[pri_index], pri_index, &(rf->pri_8k_idx_oft[pri_index]))) {
			return -1;
		}
	}

	return 0;
}

static int zxdh_sc_init_hmccnt(struct zxdh_pci_f *rf,
			       struct zxdh_mgr_par *param)
{
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	u32 hmc_info_mem_size;

	hmc_info_mem_size =
		sizeof(struct zxdh_hmc_pble_rsrc) * 2 +
		sizeof(struct zxdh_hmc_info) +
		(sizeof(struct zxdh_hmc_obj_info) * ZXDH_HMC_IW_MAX);

	rf->hmc_info_mem = kzalloc(hmc_info_mem_size, GFP_KERNEL);
	if (!rf->hmc_info_mem)
		return -ENOMEM;

	rf->pble_mr_rsrc = (struct zxdh_hmc_pble_rsrc *)rf->hmc_info_mem;
	rf->pble_rsrc = (struct zxdh_hmc_pble_rsrc *)(rf->pble_mr_rsrc + 1);
	dev->hmc_info = &rf->hw.hmc;
	dev->hmc_info->hmc_obj =
		(struct zxdh_hmc_obj_info *)(rf->pble_rsrc + 1);

	rf->max_rdma_vfs = param->max_vf_num;
	dev->hmc_use_dpu_ddr = param->hmc_use_dpu_ddr;
	dev->l2d_smmu_addr = param->l2d_smmu_addr;
	dev->l2d_smmu_l2_offset = param->l2d_smmu_l2_offset;

	dev->hmc_pf_manager_info.hmc_base = param->qp_hmc_base;
	dev->hmc_pf_manager_info.hmc_size = param->pf_hmc_size;

	rf->max_qp = param->qp_cnt;
	rf->max_cq = param->cq_cnt;
	rf->max_srq = param->srq_cnt;
	
	rf->max_ah = param->ah_cnt;
	rf->max_mr = param->mr_cnt;

    if (rf->chip_srq_base_paddr != 0 && rf->srq_mem_size != 0) {
        rf->max_srq = rf->srq_mem_size / ZXDH_SIZE_PER_SRQ_DB;
        pr_debug("[zxdh_rdma] %s[%d]: srq_mem_size=%d max_srq=%d\n", __func__, __LINE__, rf->srq_mem_size, rf->max_srq);
    }
    else {
        rf->max_srq = 0;
        pr_debug("[zxdh_rdma] %s[%d]: warning SRQ can not use DDR memory! ep_id=%d pf_id=%d vf_id=%d ftype=%d\n", __func__, __LINE__,
            rf->ep_id, rf->pf_id, rf->vf_id, rf->ftype);
    }

	if (rf->use_blue_flame_flag && (!rf->ftype)){
		rf->max_bf_qp = ZXDH_MAX_BLUE_FLAME_QP_NUM;
		rf->blue_flame_qp_num = 0;
		mutex_init(&rf->blue_flame_mtx_lock);
	}

	dev->max_qp = rf->max_qp;
	dev->max_cq = rf->max_cq;
	dev->max_srq = rf->max_srq;
	dev->base_qpn = param->base_qpn;
	dev->base_cqn = param->base_cqn;
	dev->base_srqn = param->base_srqn;

	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_QP].max_cnt = rf->max_qp;
	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_CQ].max_cnt = rf->max_cq;
	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_SRQ].max_cnt = rf->max_srq;
	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_TXWINDOW].max_cnt = rf->max_qp;
	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_IRD].max_cnt = rf->max_qp;
	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_AH].max_cnt = rf->max_ah;
	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_MR].max_cnt = param->mr_cnt;
	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_PBLE].max_cnt = param->pbleq_cnt;
	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_PBLE_MR].max_cnt = param->pblem_cnt;

	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_QP].base = param->qp_hmc_base;
	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_CQ].base = param->cq_hmc_base;
	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_SRQ].base = param->srq_hmc_base;
	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_TXWINDOW].base =
		param->txwindow_hmc_base;
	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_IRD].base = param->ird_hmc_base;
	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_AH].base = param->ah_hmc_base;
	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_MR].base = param->mr_hmc_base;
	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_PBLE].base = param->pbleq_hmc_base;
	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_PBLE_MR].base =
		param->pblem_hmc_base;

	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_QP].cnt = param->qp_cnt;
	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_CQ].cnt = param->cq_cnt;
	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_SRQ].cnt = param->srq_cnt;
	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_AH].cnt = param->ah_cnt;
	dev->hmc_info->hmc_obj[ZXDH_HMC_IW_MR].cnt = param->mr_cnt;
	if (enable_iova_cap) {
		if (rf->sc_dev.ep_id != ZXDH_ZF_EPID || dev->hmc_use_dpu_ddr) {
			dev->data_cap_sd.data_cap_base = C_HMC_DATA_CAP_IOVA_BASE;
			dev->data_cap_sd.data_len = C_HMC_DATA_CAP_IOVA_LEN;
		}
	}

	if (!rf->ftype) {
		dev->hmc_pf_manager_info.total_qp_cnt =
			param->qp_cnt + param->max_vf_num * param->vf_qp_cnt;
		dev->hmc_pf_manager_info.total_cq_cnt =
			param->cq_cnt + param->max_vf_num * param->vf_cq_cnt;
		dev->hmc_pf_manager_info.total_srq_cnt =
			param->srq_cnt + param->max_vf_num * param->vf_srq_cnt;

		dev->hmc_pf_manager_info.pf_pblemr_cnt = param->pblem_cnt;
		dev->hmc_pf_manager_info.pf_pblequeue_cnt = param->pbleq_cnt;

		dev->hmc_pf_manager_info.vf_qp_cnt = param->vf_qp_cnt;
		dev->hmc_pf_manager_info.vf_ah_cnt = param->vf_ah_cnt;
		dev->hmc_pf_manager_info.vf_mr_cnt = param->vf_mr_cnt;
		dev->hmc_pf_manager_info.vf_pblemr_cnt = param->vf_pblem_cnt;
		dev->hmc_pf_manager_info.vf_pblequeue_cnt = param->vf_pbleq_cnt;
	}
	return 0;
}

static void zxdh_init_hw_bar_pages(u8 ep_id, u64 *base_bar_offset)
{
	int i;
	u64 page_bar_offset;
	u64 bar_offset_low;
	u64 bar_offset_high;

	if (ep_id == ZXDH_ZF_EPID) {
		page_bar_offset = *base_bar_offset;
		bar_offset_low = page_bar_offset & 0xFFFF;
		bar_offset_high = page_bar_offset & 0xF0000;
		*base_bar_offset = bar_offset_low + (bar_offset_high << 4);
	} else
		page_bar_offset = 0;

	for (i = 0; i < C_RDMA_HW_BAR_PAGE_NUM; i++) {
		if (ep_id == ZXDH_ZF_EPID) {
			bar_offset_low = page_bar_offset & 0xFFFF;
			bar_offset_high = page_bar_offset & 0xF0000;
			zxdh_hw_bar_pages[ep_id][i] =
				bar_offset_low + (bar_offset_high << 4);
			zxdh_hw_bar_pages[ep_id][i] -= *base_bar_offset;
		} else
			zxdh_hw_bar_pages[ep_id][i] = page_bar_offset;

		page_bar_offset += C_RDMA_HW_BAR_PAGE_SIZE;
	}
}

static int zxdh_vf_wait_pf_cqp_init(struct zxdh_pci_f *rf, uint64_t phy_addr)
{
    uint32_t cqp_status = 0xFFFF;
    int ret = 0, cnt = 0;
    
    do {
        msleep(ZXDH_WAIT_CQP_STATUS_TIME);
        ret = zxdh_rdma_reg_read(rf, phy_addr, &cqp_status);
        if (ret) {
            pr_err("[zxdh_rdma] %s[%d]: rdma reg read failed!\n", __func__, __LINE__);
            return ret;
        }
        if (cqp_status == 1)
            return 0;
        if (rf->sc_dev.hw_attrs.skip_hw == true)
            return -ETIMEDOUT;
        cnt++;
    }while(cnt < ZXDH_WAIT_CQP_STATUS_CNT);

    return -ETIMEDOUT;
}

static int zxdh_vf_wait_pf_device_init(struct zxdh_pci_f *rf, u64 phy_addr)
{
    uint32_t cqp_status = 0xFFFF;
    int ret = 0, cnt = 0;

    do {
        ret = zxdh_rdma_reg_read(rf, phy_addr, &cqp_status);
        if (ret) {
            pr_err("[zxdh_rdma] %s[%d]: rdma reg read failed!\n", __func__, __LINE__);
            return ret;
        }
        cqp_status &= ZXDH_32BIT_1_2_MASK;
        if (cqp_status == 0) {
            pr_info("[zxdh_rdma] %s[%d] cqp_status:%d\n",__func__,__LINE__,cqp_status);
            return 0;
        }

        if ((cqp_status & ZXDH_CREATE_CQP_FLAG) == 0) {
            if ((cqp_status & ZXDH_DEVICE_INIT_COMPLETED) == ZXDH_DEVICE_INIT_COMPLETED) {
                pr_info("[zxdh_rdma] %s[%d] cqp_status:%d\n",__func__,__LINE__,cqp_status);
                return 0;
            } else
                return -ENODEV;
        }
        if (rf->sc_dev.hw_attrs.skip_hw == true)
            return -ETIMEDOUT;
        msleep(ZXDH_WAIT_CQP_STATUS_TIME);
        cnt++;
    }while(cnt < ZXDH_WAIT_DEVICE_INIT_CNT);

    return -ETIMEDOUT;
}

static int zxdh_vf_wait_pf_init(struct zxdh_pci_f *rf)
{
    uint64_t cqp_status_phy_addr = 0;
    uint64_t cqp_context_7_phy_addr = 0;
    uint32_t cqp_status = 0xFFFF;
    int ret = 0;

    if (rf == NULL) {
        pr_err("[zxdh_rdma][%s] rf is NULL\n",__func__);
        return -ENOMEM;
    }
	
    cqp_status_phy_addr = C_RDMA_CQP_STATUS_PHY_ADDR + rf->sc_dev.vhca_id_pf * 0x1000;
    cqp_context_7_phy_addr = C_RDMA_CQP_CONTEXT7_PHY_ADDR + rf->sc_dev.vhca_id_pf * 0x1000;
    ret = zxdh_rdma_reg_read(rf, cqp_status_phy_addr, &cqp_status);
    if (ret) {
        pr_err("[zxdh_rdma] %s[%d]: rdma reg read failed!\n", __func__, __LINE__);
        return ret;
    }

    if (cqp_status != 1) {
        ret = zxdh_vf_wait_pf_cqp_init(rf, cqp_status_phy_addr);
        if (ret) {
            pr_err("[zxdh_rdma] %s[%d]: wait pf cqp status failed!\n", __func__, __LINE__);
            return ret;
        }
        ret = zxdh_vf_wait_pf_device_init(rf, cqp_context_7_phy_addr);
        if (ret) {
            pr_err("[zxdh_rdma] %s[%d]: wait pf device init failed!\n", __func__, __LINE__);
            return ret;
        }
    } else {
        ret = zxdh_vf_wait_pf_device_init(rf, cqp_context_7_phy_addr);
        if (ret) {
            pr_err("[zxdh_rdma] %s[%d]: wait pf device init failed!\n", __func__, __LINE__);
            return ret;
        }
    }
    return 0;
}

static void zxdh_init_ext_hw_bar_pages(u8 ep_id, u64 *base_bar_offset)
{
	int i;
	u64 page_bar_offset;
	u64 bar_offset_low;
	u64 bar_offset_high;

	if (ep_id == ZXDH_ZF_EPID) {
		page_bar_offset = *base_bar_offset;
		bar_offset_low = page_bar_offset & 0xFFFF;
		bar_offset_high = page_bar_offset & 0xF0000;
		*base_bar_offset = bar_offset_low + (bar_offset_high << 4);
	} else
		page_bar_offset = 0;

	for (i = 0; i < C_RDMA_EXT_HW_BAR_PAGE_NUM; i++) {
		if (ep_id == ZXDH_ZF_EPID) {
			bar_offset_low = page_bar_offset & 0xFFFF;
			bar_offset_high = page_bar_offset & 0xF0000;
			zxdh_ext_hw_bar_pages[ep_id][i] =
				bar_offset_low + (bar_offset_high << 4);
			zxdh_ext_hw_bar_pages[ep_id][i] -= *base_bar_offset;
		} else
			zxdh_ext_hw_bar_pages[ep_id][i] = page_bar_offset;

		page_bar_offset += C_RDMA_EXT_HW_BAR_PAGE_SIZE;
	}
}

int zxdh_get_ext_vhca_info(struct zxdh_pci_f *rf)
{
	int ret = 0;
    u32 function_id = 0;
	u8 rep_valid = 0;
	u16 rep_len = 0;
	u8 *rep_ptr;
	struct zxdh_mgr mgr = { 0 };
	struct zxdh_pci_bar_msg in = { 0 };
	struct zxdh_msg_recviver_mem result = { 0 };
	struct dh_get_ext_vhca_info_req get_cmd = { 0 };
	struct iidc_core_dev_info *cdev_info;
	size_t recv_len;
	void *recv_buffer;
	struct dh_get_ext_vhca_info_resp *get_resp;
	u32 cnt = 0;
	u32 cnt_num = ZXDH_BAR_MSG_RETRY_NUM;

	if (!rf) {
        pr_err("[zxdh_rdma] %s[%d]: rf is null\n", __func__, __LINE__);
		return -EINVAL;
    }
	if (rf->sc_dev.driver_load == false)
		cnt_num = ZXDH_BAR_MSG_DEFAULT_NUM;
    
	// if(rf->ftype)
	// {
    //     pr_err("[zxdh_rdma] %s[%d]: vf does not have ext vhca.\n", __func__, __LINE__);
	// 	return -EINVAL;
	// }

    function_id = DH_FUNC_ID_GEN(rf->ftype, rf->ep_id, 0, rf->pf_id, rf->vf_id);

	cdev_info = (struct iidc_core_dev_info *)rf->cdev;
	// query pcie id
	mgr.pdev = cdev_info->pdev;
	ret = dh_rdma_pf_pcie_id_get(&mgr);
	if (ret) {
		pr_err("[zxdh_rdma] [%s] get pf pcie_id failed, ret=%d\n", __func__, ret);
		return -EINVAL;
	}

	recv_len = ZXDH_CHAN_REPS_LEN + sizeof(struct dh_get_ext_vhca_info_resp);
	recv_buffer = (void *)kzalloc(recv_len, GFP_KERNEL);
	if (!recv_buffer) {
        pr_err("%s[zxdh_rdma] [%d]: kzalloc failed!\n", __func__, __LINE__);
		return -ENOMEM;
    }

	// commnad preparation
	get_cmd.op_code = RDMA_GET_EXT_VHCA_INFO;
	get_cmd.function_id = function_id;
    pr_info("[zxdh_rdma] %s[%d]: function_id=0x%x ftype=%d ep_id=%d pf_id=%d vf_id=%d\n", __func__, __LINE__,
        function_id, rf->ftype, rf->ep_id, rf->pf_id, rf->vf_id);

	// get message preparation
	in.payload_addr = (void *)&get_cmd;
	in.payload_len = sizeof(struct dh_get_ext_vhca_info_req);
	in.src = rf->ftype == 0 ? MSG_CHAN_END_PF : MSG_CHAN_END_VF;
	in.dst = MSG_CHAN_END_RISC;
	in.event_id = MODULE_RDMA;
	in.virt_addr = (u64)cdev_info->hw_addr + ZXDH_BAR_CHAN_OFFSET;
	in.src_pcieid = mgr.pcie_id;

	// resv buffer preparation
	result.recv_buffer = recv_buffer;
	result.buffer_len = recv_len;

	do {
		ret = zxdh_bar_chan_sync_msg_send(&in, &result);
		if ((ret != ZXDH_BAR_ERR_TIME_OUT) && (ret != ZXDH_BAR_ERR_LOCK_FAILED)) {
			break;
		}
		cnt++;
	} while(cnt < cnt_num);

	if (ret) {
		pr_err("[zxdh_rdma] [%s] message send failed, ret=%d cnt=%d\n", __func__, ret, cnt);
        ret = -EPROTO;
        goto finish;
	}

	rep_ptr = (uint8_t *)recv_buffer;
	rep_valid = *rep_ptr;
	if (rep_valid != MSG_REP_VALID) {
		pr_err("[zxdh_rdma] [%s] response message invalid, rep_valid=0x%x\n", __func__, rep_valid);
		ret = -EPROTO;
        goto finish;
	}

	rep_len = *(uint16_t *)(rep_ptr + MSG_REP_LEN_OFFSET);
	if (rep_len != (recv_len - ZXDH_CHAN_REPS_LEN)) {
		pr_err("[zxdh_rdma] [%s] response length invalid, rep_len=0x%x recv_len=0x%zx\n", __func__, rep_len, recv_len);
		ret = -EPROTO;
        goto finish;
	}

	get_resp = (struct dh_get_ext_vhca_info_resp *)(rep_ptr + ZXDH_CHAN_REPS_LEN);
	if (get_resp->status_code != BAR_MSG_STATUS_OK) {
		pr_err("[zxdh_rdma] [%s] response status invalid, statuc_code=0x%x\n", __func__, get_resp->status_code);
		ret = -EPROTO;
        goto finish;
	}

    rf->ext_sc_dev.vhca_id = get_resp->ext_vhca_id;
    rf->ext_sc_dev.vhca_gqp_start = get_resp->ext_vhca_gqp_start;
    rf->ext_sc_dev.vhca_gqp_cnt = get_resp->ext_vhca_gqp_cnt;
    rf->ext_sc_dev.vhca_8k_index_start = get_resp->ext_vhca_8k_index_start;
    rf->ext_sc_dev.vhca_8k_index_cnt = get_resp->ext_vhca_8k_index_cnt;
	rf->ext_sc_dev.hw_attrs.uk_attrs.max_hw_sq_sges = get_resp->max_hw_sq_sges;
	rf->ext_sc_dev.hw_attrs.uk_attrs.max_hw_rq_sges = get_resp->max_hw_rq_sges;
    rf->ext_base_bar_offset = get_resp->ext_vhca_bar_offset;
	rf->ext_l2d_addr_base = get_resp->ext_l2d_addr_base;

	if (!rf->ftype) {
		pr_info("[zxdh_rdma] ext_vhca_id=%d bar_oft=0x%llx gqp_s:0x%x "
				"gqp_cnt:0x%x 8k_idx_s:0x%x 8k_idx_cnt:0x%x ext_l2d_addr_base:0x%llx\n", 
				rf->ext_sc_dev.vhca_id, rf->ext_base_bar_offset, rf->ext_sc_dev.vhca_gqp_start,
				rf->ext_sc_dev.vhca_gqp_cnt, rf->ext_sc_dev.vhca_8k_index_start, 
				rf->ext_sc_dev.vhca_8k_index_cnt, rf->ext_l2d_addr_base);
	}

finish:
	kfree(recv_buffer);
    recv_buffer = NULL;
	return ret;
}

static int zxdh_pf_dev_exist_for_vf(struct zxdh_pci_f *rf)
{
    uint64_t cqp_status_phy_addr = 0;
    uint32_t cqp_status = 0xFFFF;
    int ret = 0;

    if (rf->ftype == 1) {
        cqp_status_phy_addr = C_RDMA_CQP_STATUS_PHY_ADDR + rf->sc_dev.vhca_id_pf * 0x1000;
        ret = zxdh_rdma_reg_read(rf, cqp_status_phy_addr, &cqp_status);
        if (ret) {
            pr_err("[zxdh_rdma] %s[%d]: rdma reg read failed!\n", __func__, __LINE__);
            return ret;
        }

        pr_info("[zxdh_rdma] vf rdma probe: ep_id=%d, pf_id=%d, vf_id=%d, vhca_id=%d, vhca_id_pf=%d, cqp_status=%d\n",
            rf->ep_id, rf->pf_id, rf->vf_id, rf->sc_dev.vhca_id, rf->sc_dev.vhca_id_pf, cqp_status);
        if (rf->vm_info.vm_mode == false) {
            if (cqp_status != 1) {
               pr_err("[zxdh_rdma][%d] vf rdma probe: The RDMA device for EP%d PF%d corresponding to VF%d does not exist!\n", __LINE__, rf->ep_id, rf->pf_id, rf->vf_id);
               return -ENODEV;
            }
        } else {
            ret = zxdh_vf_wait_pf_init(rf);
            if (ret) {
                pr_err("[zxdh_rdma][%d] vf rdma probe: The RDMA device for EP%d PF%d corresponding to VF%d does not exist!\n", __LINE__, rf->ep_id, rf->pf_id, rf->vf_id);
                return -ENODEV;
            }
        }
    }

    return 0;
}

static bool zxdh_is_device_support_low_lat(struct zxdh_pci_f *rf)
{
	if (rf->pcidev->device == ZXDH_DEV_ID_X512_ROCE_PF_EP0 || rf->pcidev->device == ZXDH_DEV_ID_X512_ROCE_PF_EP1)
	{
		return false;
	}
	else
	{
		return true;
	}
}

int zxdh_manager_init(struct zxdh_pci_f *rf,
		      struct iidc_core_dev_info *cdev_info)
{
	int ret = 0;
	struct zxdh_mgr *dh_mgr = kzalloc(sizeof(struct zxdh_mgr), GFP_KERNEL);

	if (dh_mgr == NULL)
		return -ENOMEM;

	dh_mgr->pdev = cdev_info->pdev;
	dh_mgr->pf_id = rf->pf_id;
	dh_mgr->vport_vf_id = (cdev_info->vport_id) & 0xFF;
	dh_mgr->ftype = rf->ftype;
	dh_mgr->ep_id = rf->sc_dev.ep_id;

	dh_mgr->device_id = cdev_info->pdev->subsystem_device;
	dh_mgr->pci_hw_addr = cdev_info->hw_addr;

	ret = zxdh_mgr_par_get(dh_mgr);
	if (ret != 0) {
		kfree(dh_mgr);
		if (ret != (-ENODEV)) {
			pr_info("[zxdh_rdma] dh_rdma_mgr_par_get failed. ret=%d\n", ret);
		}
		return ret;
	}

	pr_info("[zxdh_rdma] manager pcie_id=0x%x, device_id=0x%x, slot_id=0x%x\n", dh_mgr->pcie_id, dh_mgr->device_id, cdev_info->slot_id);
	rf->pcie_id = dh_mgr->pcie_id;
	rf->vf_id = dh_mgr->param.vf_id;
	rf->sc_dev.vf_id = dh_mgr->param.vf_id;
	rf->sc_dev.vhca_id = dh_mgr->param.vhca_id;
	rf->sc_dev.vhca_id_pf = dh_mgr->param.vhca_id_pf;
	if (rf->sc_dev.vhca_id == 1023)
	{
        kfree(dh_mgr);
		pr_info("[zxdh_rdma] vhca_id:1023 invalid\n");
		return -1;
	}

    ret = zxdh_pf_dev_exist_for_vf(rf);
    if (ret != 0) {
        kfree(dh_mgr);
		return -1;
    }

	rf->sc_dev.hmc_fn_id = dh_mgr->param.hmc_sid;
	rf->sc_dev.total_vhca = dh_mgr->param.dh_total_vhca;
	if(!zxdh_is_device_support_low_lat(rf))
	{
		rf->sc_dev.np_mode_low_lat = false;
	}
	else
	{
		rf->sc_dev.np_mode_low_lat = dh_mgr->param.np_mode_low_lat;
	}
	rf->sc_dev.chip_version = dh_mgr->param.chip_version;

	rf->sc_dev.nof_ioq_ddr_addr = dh_mgr->param.nof_ioq_ddr_addr;
	rf->sc_dev.hw_attrs.uk_attrs.max_hw_wq_frags =
		dh_mgr->param.max_hw_wq_frags;
	rf->sc_dev.hw_attrs.uk_attrs.max_hw_read_sges =
		dh_mgr->param.max_hw_read_sges;
	//VHCA_RC_UD_GQP_MAX_CNT
	rf->sc_dev.vhca_gqp_start = dh_mgr->param.vhca_gqp_start;
	rf->sc_dev.vhca_gqp_cnt = dh_mgr->param.vhca_gqp_cnt;
	rf->sc_dev.vhca_8k_index_start = dh_mgr->param.vhca_8k_index_start;
	//VHCA_RC_UD_8K_MAX_CNT
	rf->sc_dev.vhca_8k_index_cnt = dh_mgr->param.vhca_8k_index_cnt;
	rf->sc_dev.vhca_ud_gqp = dh_mgr->param.vhca_ud_gqp;
	rf->sc_dev.vhca_ud_8k_index = dh_mgr->param.vhca_ud_8k_index;
	rf->max_rdma_vfs = dh_mgr->param.max_vf_num;

	pr_info("[zxdh_rdma] vhca_gqp_start:0x%x\n", rf->sc_dev.vhca_gqp_start);
	pr_info("[zxdh_rdma] vhca_gqp_cnt:0x%x\n", rf->sc_dev.vhca_gqp_cnt);
	pr_info("[zxdh_rdma] vhca_8k_index_start:0x%x\n", rf->sc_dev.vhca_8k_index_start);
	pr_info("[zxdh_rdma] vhca_8k_index_cnt:0x%x\n", rf->sc_dev.vhca_8k_index_cnt);
	pr_info("[zxdh_rdma] vhca_ud_gqp:0x%x\n", rf->sc_dev.vhca_ud_gqp);
	pr_info("[zxdh_rdma] vhca_ud_8k_index:0x%x\n", rf->sc_dev.vhca_ud_8k_index);

	ret = zxdh_init_8k_index(&rf->sc_dev);
	if (ret != 0) {
		kfree(dh_mgr);
		pr_info("zxdh_init_8k_index fail.\n");
		return ret;
	}

	ret = zxdh_calculate_8k_index_offset(rf);
	if (ret != 0) {
		kfree(dh_mgr);
		pr_info("zxdh_calculate_8k_index_offset fail.\n");
		return ret;
	}
	pr_info("[zxdh_rdma] pri_8k_idx_oft TC0:%hu TC1:%hu TC2:%hu TC3:%hu TC4:%hu TC5:%hu TC6:%hu TC7:%hu \n",
			rf->pri_8k_idx_oft[ZXDH_PRI_IDX_0],rf->pri_8k_idx_oft[ZXDH_PRI_IDX_1],
			rf->pri_8k_idx_oft[ZXDH_PRI_IDX_2],rf->pri_8k_idx_oft[ZXDH_PRI_IDX_3],
			rf->pri_8k_idx_oft[ZXDH_PRI_IDX_4],rf->pri_8k_idx_oft[ZXDH_PRI_IDX_5],
			rf->pri_8k_idx_oft[ZXDH_PRI_IDX_6],rf->pri_8k_idx_oft[ZXDH_PRI_IDX_7]);

	ret = zxdh_init_dip_info_hlist(rf);
	if (ret != 0) {
		kfree(dh_mgr);
		pr_info("init_dip_info_hlist fail.\n");
		return ret;
	}

	/* 初始化ETS动态令牌管理器 */
	zxdh_ets_token_mgr_init(rf);

	rf->base_bar_offset = dh_mgr->param.bar_offset;
	zxdh_init_hw_bar_pages(rf->sc_dev.ep_id, &rf->base_bar_offset);
	rf->hw.hw_addr = cdev_info->hw_addr + rf->base_bar_offset;
	if (rf->use_ext_mem_flag)
	{
		rf->hw.hbm_info_page_addr = cdev_info->hw_addr+ 0xF08000;     //新增L2D映射的bar空间地址
	}
	
    pr_info("[zxdh_rdma] rf->hw.hw_addr=0x%llx, cdev_info->hw_addr=0x%llx, rf->base_bar_offset=0x%llx\n", (u64)rf->hw.hw_addr, (u64)cdev_info->hw_addr, rf->base_bar_offset);

	// 不区分pf和vf，只要固件支持蓝焰，就发送消息获取蓝焰信息
	if (rf->use_blue_flame_flag) {
        ret = zxdh_get_ext_vhca_info(rf);
        if (ret) {
            kfree(dh_mgr);
            pr_err("[zxdh_rdma] get ext vhca info failed");
            return -1;
        }
        rf->ext_sc_dev.vhca_id_pf = rf->sc_dev.vhca_id_pf;
        rf->ext_sc_dev.ep_id = rf->sc_dev.ep_id;
		rf->ext_sc_dev.hmc_fn_id = rf->sc_dev.hmc_fn_id;
        zxdh_init_ext_hw_bar_pages(rf->ext_sc_dev.ep_id, &rf->ext_base_bar_offset);
        rf->hw.ext_hw_addr = cdev_info->hw_addr + rf->ext_base_bar_offset;
        pr_info("[zxdh_rdma] rf->hw.ext_hw_addr=0x%llx, cdev_info->hw_addr=0x%llx, rf->ext_base_bar_offset=0x%llx\n", (u64)rf->hw.ext_hw_addr, (u64)cdev_info->hw_addr, rf->ext_base_bar_offset);
        rf->ext_sc_dev.hw_attrs.uk_attrs.max_hw_wq_frags =
            dh_mgr->param.max_hw_wq_frags;
        rf->ext_sc_dev.hw_attrs.uk_attrs.max_hw_read_sges =
            dh_mgr->param.max_hw_read_sges;

		rf->ext_sc_dev.hmc_use_dpu_ddr = dh_mgr->param.hmc_use_dpu_ddr;

		rf->ext_sc_dev.max_qp = dh_mgr->param.qp_cnt;
		rf->ext_sc_dev.max_cq = dh_mgr->param.cq_cnt;
		rf->ext_sc_dev.max_srq = dh_mgr->param.srq_cnt;
		rf->ext_sc_dev.base_qpn = dh_mgr->param.base_qpn;
		rf->ext_sc_dev.base_cqn = dh_mgr->param.base_cqn;
		rf->ext_sc_dev.base_srqn = dh_mgr->param.base_srqn;
		
		rf->sc_dev.hw_attrs.uk_attrs.max_hw_sq_sges = rf->ext_sc_dev.hw_attrs.uk_attrs.max_hw_sq_sges;
		rf->sc_dev.hw_attrs.uk_attrs.max_hw_rq_sges = rf->ext_sc_dev.hw_attrs.uk_attrs.max_hw_rq_sges;
    } else {
		rf->sc_dev.hw_attrs.uk_attrs.max_hw_sq_sges = rf->sc_dev.hw_attrs.uk_attrs.max_hw_wq_frags;
		rf->sc_dev.hw_attrs.uk_attrs.max_hw_rq_sges = rf->sc_dev.hw_attrs.uk_attrs.max_hw_wq_frags;
	}
	pr_info("[zxdh_rdma] max_hw_sq_sges:0x%x max_hw_rq_sges:0x%x\n", 
			rf->sc_dev.hw_attrs.uk_attrs.max_hw_sq_sges, 
			rf->sc_dev.hw_attrs.uk_attrs.max_hw_rq_sges);

	ret = zxdh_sc_init_hmccnt(rf, &dh_mgr->param);
	if (ret != 0) {
		kfree(dh_mgr);
		pr_info("[zxdh_rdma] init_hmccnt faile.\n");
		return ret;
	}

	rf->sc_dev.max_ceqs = dh_mgr->param.ceq_cnt;
	rf->sc_dev.base_ceqn = dh_mgr->param.base_ceqn;

	rf->msix_count = min(rf->msix_count, (rf->sc_dev.max_ceqs + 1));
	if (rf->msix_count > 1)
		rf->sc_dev.max_ceqs = (rf->msix_count - 1);
	else
		rf->sc_dev.max_ceqs = rf->msix_count;
	if (rf->msix_count == 0) {
		kfree(dh_mgr);
		pr_info("[zxdh_rdma] misx_count is 0\n");
		return -EINVAL;
	}
	rf->mcode_type = dh_mgr->param.mcode_type;
	kfree(dh_mgr);

	return 0;
}

/***
 * @brief send general rdma message to firmware
 *
 * @param rf
 * @param para includes infos of buffers to send and receive
 * @return int
 * @retval 0 on success
 * @retval -EINVAL when input arguments are invalid
 * @retval -ENOMEM when alloc buffer fails
 * @retval -EPROTO when bar message send/recv fails
 */
int rdma_chan_msg_send(struct zxdh_pci_f *rf, struct rdma_chan_msg_para *para)
{
	int ret = 0;
	uint8_t *rep_ptr;
	uint16_t rep_len = 0;
	uint8_t rep_valid = 0;
	size_t recv_len = 0;
	void *recv_buffer = NULL;

	struct zxdh_mgr mgr = { 0 };
	struct zxdh_pci_bar_msg in = { 0 };
	struct zxdh_msg_recviver_mem result = { 0 };
	struct iidc_core_dev_info *cdev_info = NULL;
	u32 cnt = 0;
	u32 cnt_num = ZXDH_BAR_MSG_RETRY_NUM;

	if (!para || !rf)
		return -EINVAL;
	if (rf->sc_dev.driver_load == false)
		cnt_num = ZXDH_BAR_MSG_DEFAULT_NUM;

	if (!(para->in_buf) || !(para->out_buf))
		return -EINVAL;

	cdev_info = (struct iidc_core_dev_info *)rf->cdev;

	// query pcie id
	mgr.pdev = cdev_info->pdev;
	ret = dh_rdma_pf_pcie_id_get(&mgr);
	if (ret) {
		pr_err("[zxdh_rdma] [%s] get pf pcie_id failed, ret=%d\n", __func__, ret);
		return -EINVAL;
	}

	// malloc recv buffer with extra ZXDH_CHAN_REPS_LEN size
	recv_len = ZXDH_CHAN_REPS_LEN + para->out_size;
	recv_buffer = (void *)kzalloc(recv_len, GFP_KERNEL);
	if (!recv_buffer)
		return -ENOMEM;

	// get message preparation
	in.payload_addr = (void *)para->in_buf;
	in.payload_len = para->in_size;

	in.src = rf->ftype == 0 ? MSG_CHAN_END_PF : MSG_CHAN_END_VF;
	in.dst = MSG_CHAN_END_RISC;
	in.event_id = MODULE_RDMA;
	in.virt_addr = (uint64_t)(cdev_info->hw_addr) + ZXDH_BAR_CHAN_OFFSET;
	in.src_pcieid = mgr.pcie_id;

	// resv buffer preparation
	result.recv_buffer = recv_buffer;
	result.buffer_len = recv_len;

	do {
		ret = zxdh_bar_chan_sync_msg_send(&in, &result);
		if ((ret != ZXDH_BAR_ERR_TIME_OUT) && (ret != ZXDH_BAR_ERR_LOCK_FAILED)) {
			break;
		}
		cnt++;
	}while(cnt < cnt_num);

	if (ret) {
		pr_err("[zxdh_rdma] [%s] message send failed, ret=%d cnt=%d\n", __func__, ret, cnt);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_ptr = (uint8_t *)recv_buffer;
	rep_valid = *rep_ptr;
	if (rep_valid != MSG_REP_VALID) {
		pr_err("[zxdh_rdma] [%s] response message invalid, rep_valid=0x%x\n",
		       __func__, rep_valid);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_len = *(uint16_t *)(rep_ptr + MSG_REP_LEN_OFFSET);
	if (rep_len != para->out_size) {
		pr_err("[zxdh_rdma] [%s] response length invalid, rep_len=0x%x\n", __func__,
		       rep_len);
		kfree(recv_buffer);
		return -EPROTO;
	}

	memcpy(para->out_buf, rep_ptr + ZXDH_CHAN_REPS_LEN, rep_len);
	kfree(recv_buffer);
	return 0;
}

/***
 * @brief read register value from rdma
 *
 * @param rf for accessing hardware info
 * @param phy_addr physical address on rdma. registuer width is uint32_t
 * @param outdata
 * @return int
 * 	- 0: ok
 * 	- -1: failed
 */
int zxdh_rdma_reg_read(struct zxdh_pci_f *rf, uint64_t phy_addr,
		       uint32_t *outdata)
{
	int ret = 0;
	uint8_t rep_valid = 0;
	uint16_t rep_len = 0;
	uint8_t *rep_ptr;
	struct zxdh_pci_bar_msg in = { 0 };
	struct zxdh_msg_recviver_mem result = { 0 };
	struct zxdh_mgr mgr = { 0 };
	struct iidc_core_dev_info *cdev_info;
	struct zxdh_reg_read_cmd *read_cmd;
	size_t recv_len;
	void *recv_buffer;
	struct dh_rdma_reg_read_resp *read_resp;
	u32 cnt = 0;
	u32 cnt_num = ZXDH_BAR_MSG_RETRY_NUM;

	
	if (!rf || !outdata)
		return -EINVAL;
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

	read_cmd = (struct zxdh_reg_read_cmd *)kzalloc(
		sizeof(struct zxdh_reg_read_cmd), GFP_KERNEL);
	if (!read_cmd)
		return -ENOMEM;

	recv_len = ZXDH_CHAN_REPS_LEN + sizeof(struct dh_rdma_reg_read_resp) +
		   1 * sizeof(uint32_t); // data
	recv_buffer = (void *)kzalloc(recv_len, GFP_KERNEL);
	if (!recv_buffer) {
		kfree(read_cmd);
		return -ENOMEM;
	}

	// commnad preparation
	read_cmd->op_code = RDMA_REG_READ;
	read_cmd->req.phy_addr = phy_addr;
	read_cmd->req.reg_num = 1;

	// send message preparation
	in.payload_addr = (void *)read_cmd;
	in.payload_len = sizeof(struct zxdh_reg_read_cmd);
	in.src = rf->ftype == 0 ? MSG_CHAN_END_PF : MSG_CHAN_END_VF;
	in.dst = MSG_CHAN_END_RISC;
	in.event_id = MODULE_RDMA;
	in.virt_addr = (u64)cdev_info->hw_addr + ZXDH_BAR_CHAN_OFFSET;
	in.src_pcieid = mgr.pcie_id;

	// resv buffer preparation
	result.recv_buffer = recv_buffer;
	result.buffer_len = recv_len;

	do {
		ret = zxdh_bar_chan_sync_msg_send(&in, &result);
		if ((ret != ZXDH_BAR_ERR_TIME_OUT) && (ret != ZXDH_BAR_ERR_LOCK_FAILED)) {
			break;
		}
		cnt++;
		if (rf->sc_dev.hw_attrs.skip_hw == true) {
			kfree(read_cmd);
			kfree(recv_buffer);
			return -EPROTO;
		}
        
	}while(cnt < cnt_num);
	
	kfree(read_cmd);

	if (ret) {
		pr_err("[zxdh_rdma] [%s] message send failed, ret=%d\n", __func__, ret);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_ptr = (uint8_t *)recv_buffer;
	rep_valid = *rep_ptr;
	if (rep_valid != MSG_REP_VALID) {
		pr_err("[zxdh_rdma] [%s] response message invalid, rep_valid=0x%x\n",
		       __func__, rep_valid);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_len = *(uint16_t *)(rep_ptr + MSG_REP_LEN_OFFSET);
	if (rep_len != recv_len - ZXDH_CHAN_REPS_LEN) {
		pr_err("[zxdh_rdma] [%s] response length invalid, rep_len=0x%x\n", __func__,
		       rep_len);
		kfree(recv_buffer);
		return -EPROTO;
	}

	read_resp =
		(struct dh_rdma_reg_read_resp *)(rep_ptr + ZXDH_CHAN_REPS_LEN);
	if (read_resp->status_code != 200) {
		pr_err("[zxdh_rdma] [%s] response status invalid, statuc_code=0x%x\n",
		       __func__, read_resp->status_code);
		kfree(recv_buffer);
		return -EPROTO;
	}

	*outdata = read_resp->data[0];

	kfree(recv_buffer);
	return 0;
}

int zxdh_rdma_regs_read(struct zxdh_pci_f *rf, uint64_t phy_addr,
		       uint32_t *outdata, uint32_t num)
{
	int ret = 0;
	uint8_t rep_valid = 0;
	uint16_t rep_len = 0;
	uint8_t *rep_ptr;
	struct zxdh_pci_bar_msg in = { 0 };
	struct zxdh_msg_recviver_mem result = { 0 };
	struct zxdh_mgr mgr = { 0 };
	struct iidc_core_dev_info *cdev_info;
	struct zxdh_reg_read_cmd *read_cmd;
	size_t recv_len;
	void *recv_buffer;
	struct dh_rdma_reg_read_resp *read_resp;
	u32 cnt = 0;
	u32 cnt_num = ZXDH_BAR_MSG_RETRY_NUM;

	if (!rf || !outdata)
		return -EINVAL;
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

	read_cmd = (struct zxdh_reg_read_cmd *)kzalloc(
		sizeof(struct zxdh_reg_read_cmd), GFP_KERNEL);
	if (!read_cmd)
		return -ENOMEM;

	recv_len = ZXDH_CHAN_REPS_LEN + sizeof(struct dh_rdma_reg_read_resp) +
		   num * sizeof(uint32_t); // data
	recv_buffer = (void *)kzalloc(recv_len, GFP_KERNEL);
	if (!recv_buffer) {
		kfree(read_cmd);
		return -ENOMEM;
	}

	// commnad preparation
	read_cmd->op_code = RDMA_REG_READ;
	read_cmd->req.phy_addr = phy_addr;
	read_cmd->req.reg_num = num;

	// send message preparation
	in.payload_addr = (void *)read_cmd;
	in.payload_len = sizeof(struct zxdh_reg_read_cmd);
	in.src = rf->ftype == 0 ? MSG_CHAN_END_PF : MSG_CHAN_END_VF;
	in.dst = MSG_CHAN_END_RISC;
	in.event_id = MODULE_RDMA;
	in.virt_addr = (u64)cdev_info->hw_addr + ZXDH_BAR_CHAN_OFFSET;
	in.src_pcieid = mgr.pcie_id;

	// resv buffer preparation
	result.recv_buffer = recv_buffer;
	result.buffer_len = recv_len;

	do {
		ret = zxdh_bar_chan_sync_msg_send(&in, &result);
		if ((ret != ZXDH_BAR_ERR_TIME_OUT) && (ret != ZXDH_BAR_ERR_LOCK_FAILED)) {
			break;
		}
		cnt++;
	}while(cnt < cnt_num);
	
	kfree(read_cmd);

	if (ret) {
		pr_err("[zxdh_rdma] [%s] message send failed, ret=%d cnt=%d\n", __func__, ret, cnt);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_ptr = (uint8_t *)recv_buffer;
	rep_valid = *rep_ptr;
	if (rep_valid != MSG_REP_VALID) {
		pr_err("[zxdh_rdma] [%s] response message invalid, rep_valid=0x%x\n",
		       __func__, rep_valid);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_len = *(uint16_t *)(rep_ptr + MSG_REP_LEN_OFFSET);
	if (rep_len != recv_len - ZXDH_CHAN_REPS_LEN) {
		pr_err("[zxdh_rdma] [%s] response length invalid, rep_len=0x%x\n", __func__,
		       rep_len);
		kfree(recv_buffer);
		return -EPROTO;
	}

	read_resp =
		(struct dh_rdma_reg_read_resp *)(rep_ptr + ZXDH_CHAN_REPS_LEN);
	if (read_resp->status_code != 200) {
		pr_err("[zxdh_rdma] [%s] response status invalid, statuc_code=0x%x\n",
		       __func__, read_resp->status_code);
		kfree(recv_buffer);
		return -EPROTO;
	}

	memcpy(outdata, read_resp->data, num * sizeof(uint32_t));

	kfree(recv_buffer);
	return 0;
}

int zxdh_rdma_reg_write(struct zxdh_pci_f *rf, uint64_t phy_addr, uint32_t val)
{
	int ret = 0;
	uint8_t rep_valid = 0;
	uint16_t rep_len = 0;
	uint8_t *rep_ptr;
	struct zxdh_pci_bar_msg in = { 0 };
	struct zxdh_msg_recviver_mem result = { 0 };
	struct zxdh_mgr mgr = { 0 };
	struct iidc_core_dev_info *cdev_info;
	size_t write_cmd_len;
	size_t recv_len;
	void *recv_buffer;
	struct zxdh_reg_write_cmd *write_cmd;
	struct dh_rdma_reg_write_resp *write_resp;
	u32 cnt = 0;
	u32 cnt_num = ZXDH_BAR_MSG_RETRY_NUM;

	if (!rf)
		return -EINVAL;
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

	write_cmd_len =
		sizeof(struct zxdh_reg_write_cmd) + 1 * sizeof(uint32_t);
	write_cmd =
		(struct zxdh_reg_write_cmd *)kzalloc(write_cmd_len, GFP_KERNEL);
	if (!write_cmd)
		return -ENOMEM;

	recv_len = ZXDH_CHAN_REPS_LEN + sizeof(struct dh_rdma_reg_write_resp);
	recv_buffer = (void *)kzalloc(recv_len, GFP_KERNEL);
	if (!recv_buffer) {
		kfree(write_cmd);
		return -ENOMEM;
	}

	// commnad preparation
	write_cmd->op_code = RDMA_REG_WRITE;
	write_cmd->req.phy_addr = phy_addr;
	write_cmd->req.reg_num = 1;
	write_cmd->req.data[0] = val;

	// send message preparation
	in.payload_addr = (void *)write_cmd;
	in.payload_len = write_cmd_len;
	in.src = rf->ftype == 0 ? MSG_CHAN_END_PF : MSG_CHAN_END_VF;
	in.dst = MSG_CHAN_END_RISC;
	in.event_id = MODULE_RDMA;
	in.virt_addr = (u64)cdev_info->hw_addr + ZXDH_BAR_CHAN_OFFSET;
	in.src_pcieid = mgr.pcie_id;

	// resv buffer preparation
	result.recv_buffer = recv_buffer;
	result.buffer_len = recv_len;

	do {
		ret = zxdh_bar_chan_sync_msg_send(&in, &result);
		if ((ret != ZXDH_BAR_ERR_TIME_OUT) && (ret != ZXDH_BAR_ERR_LOCK_FAILED)) {
			break;
		}
		cnt++;
		pr_info("[zxdh_rdma] [%s] cnt:%d ret:%d\n",__func__,cnt,ret);
	}while(cnt < cnt_num);
	
	kfree(write_cmd);

	if (ret) {
		pr_err("[zxdh_rdma] [%s] message send failed, ret=%d cnt=%d\n", __func__, ret, cnt);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_ptr = (uint8_t *)recv_buffer;
	rep_valid = *rep_ptr;
	if (rep_valid != MSG_REP_VALID) {
		pr_err("[zxdh_rdma] [%s] response message invalid, rep_valid=0x%x\n",
		       __func__, rep_valid);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_len = *(uint16_t *)(rep_ptr + MSG_REP_LEN_OFFSET);
	if (rep_len != recv_len - ZXDH_CHAN_REPS_LEN) {
		pr_err("[zxdh_rdma] [%s] response length invalid, rep_len=0x%x\n", __func__,
		       rep_len);
		kfree(recv_buffer);
		return -EPROTO;
	}

	write_resp =
		(struct dh_rdma_reg_write_resp *)(rep_ptr + ZXDH_CHAN_REPS_LEN);
	if (write_resp->status_code != 200) {
		pr_err("[zxdh_rdma] [%s] response status invalid, statuc_code=0x%x\n",
		       __func__, write_resp->status_code);
		kfree(recv_buffer);
		return -EPROTO;
	}

	kfree(recv_buffer);
	return 0;
}

int zxdh_mp_dtcm_para_get(struct zxdh_pci_f *rf, uint16_t mcode_type,
			  uint16_t para_id, uint32_t *outdata)
{
	int ret = 0;
	uint8_t rep_valid = 0;
	uint16_t rep_len = 0;
	uint8_t *rep_ptr;
	struct zxdh_mgr mgr = { 0 };
	struct zxdh_pci_bar_msg in = { 0 };
	struct zxdh_msg_recviver_mem result = { 0 };
	struct zxdh_mp_dtcm_para_get_cmd get_cmd = { 0 };
	struct iidc_core_dev_info *cdev_info;
	size_t recv_len;
	void *recv_buffer;
	struct dh_mp_dtcm_para_get_resp *get_resp;
	u32 cnt = 0;
	u32 cnt_num = ZXDH_BAR_MSG_RETRY_NUM;

	if (!rf || !outdata)
		return -EINVAL;
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

	recv_len = ZXDH_CHAN_REPS_LEN + sizeof(struct dh_mp_dtcm_para_get_resp);
	recv_buffer = (void *)kzalloc(recv_len, GFP_KERNEL);
	if (!recv_buffer)
		return -ENOMEM;

	// commnad preparation
	get_cmd.op_code = RDMA_MP_DTCM_PARA_GET;
	get_cmd.req.mcode_type = mcode_type;
	get_cmd.req.para_id = para_id;

	// get message preparation
	in.payload_addr = (void *)&get_cmd;
	in.payload_len = sizeof(struct zxdh_mp_dtcm_para_get_cmd);
	in.src = rf->ftype == 0 ? MSG_CHAN_END_PF : MSG_CHAN_END_VF;
	in.dst = MSG_CHAN_END_RISC;
	in.event_id = MODULE_RDMA;
	in.virt_addr = (u64)cdev_info->hw_addr + ZXDH_BAR_CHAN_OFFSET;
	in.src_pcieid = mgr.pcie_id;

	// resv buffer preparation
	result.recv_buffer = recv_buffer;
	result.buffer_len = recv_len;

	do {
		ret = zxdh_bar_chan_sync_msg_send(&in, &result);
		if ((ret != ZXDH_BAR_ERR_TIME_OUT) && (ret != ZXDH_BAR_ERR_LOCK_FAILED)) {
			break;
		}
		cnt++;
	}while(cnt < cnt_num);
	
	if (ret) {
		pr_err("[zxdh_rdma] [%s] message send failed, ret=%d cnt=%d\n", __func__, ret, cnt);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_ptr = (uint8_t *)recv_buffer;
	rep_valid = *rep_ptr;
	if (rep_valid != MSG_REP_VALID) {
		pr_err("[zxdh_rdma] [%s] response message invalid, rep_valid=0x%x\n",
		       __func__, rep_valid);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_len = *(uint16_t *)(rep_ptr + MSG_REP_LEN_OFFSET);
	if (rep_len != recv_len - ZXDH_CHAN_REPS_LEN) {
		pr_err("[zxdh_rdma] [%s] response length invalid, rep_len=0x%x\n", __func__,
		       rep_len);
		kfree(recv_buffer);
		return -EPROTO;
	}

	get_resp = (struct dh_mp_dtcm_para_get_resp *)(rep_ptr +
						       ZXDH_CHAN_REPS_LEN);
	if (get_resp->status_code != 200) {
		pr_err("[zxdh_rdma] [%s] response status invalid, statuc_code=0x%x\n",
		       __func__, get_resp->status_code);
		kfree(recv_buffer);
		return -EPROTO;
	}

	pr_info("[zxdh_rdma] resp: para_id=%d val=%d\n", get_resp->para_id, get_resp->val);

	*outdata = get_resp->val;
	kfree(recv_buffer);
	return 0;
}

int zxdh_mp_dtcm_para_set(struct zxdh_pci_f *rf, uint16_t mcode_type,
			  uint16_t para_id, uint32_t val)
{
	int ret = 0;
	uint8_t rep_valid = 0;
	uint16_t rep_len = 0;
	uint8_t *rep_ptr;
	struct zxdh_mgr mgr = { 0 };
	struct zxdh_pci_bar_msg in = { 0 };
	struct zxdh_msg_recviver_mem result = { 0 };
	struct zxdh_mp_dtcm_para_set_cmd set_cmd = { 0 };
	struct iidc_core_dev_info *cdev_info;
	size_t recv_len;
	void *recv_buffer;
	struct dh_mp_dtcm_para_set_resp *set_resp;
	u32 cnt = 0;
	u32 cnt_num = ZXDH_BAR_MSG_RETRY_NUM;

	if (!rf)
		return -EINVAL;
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

	recv_len = ZXDH_CHAN_REPS_LEN + sizeof(struct dh_mp_dtcm_para_set_resp);
	recv_buffer = (void *)kzalloc(recv_len, GFP_KERNEL);
	if (!recv_buffer)
		return -ENOMEM;

	// commnad preparation
	set_cmd.op_code = RDMA_MP_DTCM_PARA_SET;
	set_cmd.req.mcode_type = mcode_type;
	set_cmd.req.para_id = para_id;
	set_cmd.req.val = val;

	// get message preparation
	in.payload_addr = (void *)&set_cmd;
	in.payload_len = sizeof(struct zxdh_mp_dtcm_para_set_cmd);
	in.src = rf->ftype == 0 ? MSG_CHAN_END_PF : MSG_CHAN_END_VF;
	in.dst = MSG_CHAN_END_RISC;
	in.event_id = MODULE_RDMA;
	in.virt_addr = (u64)cdev_info->hw_addr + ZXDH_BAR_CHAN_OFFSET;
	in.src_pcieid = mgr.pcie_id;

	// resv buffer preparation
	result.recv_buffer = recv_buffer;
	result.buffer_len = recv_len;

	do {
		ret = zxdh_bar_chan_sync_msg_send(&in, &result);
		if ((ret != ZXDH_BAR_ERR_TIME_OUT) && (ret != ZXDH_BAR_ERR_LOCK_FAILED)) {
			break;
		}
		cnt++;
	}while(cnt < cnt_num);
	if (ret) {
		pr_err("[zxdh_rdma] [%s] message send failed, ret=%d cnt=%d\n", __func__, ret, cnt);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_ptr = (uint8_t *)recv_buffer;
	rep_valid = *rep_ptr;
	if (rep_valid != MSG_REP_VALID) {
		pr_err("[zxdh_rdma] [%s] response message invalid, rep_valid=0x%x\n",
		       __func__, rep_valid);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_len = *(uint16_t *)(rep_ptr + MSG_REP_LEN_OFFSET);
	if (rep_len != recv_len - ZXDH_CHAN_REPS_LEN) {
		pr_err("[zxdh_rdma] [%s] response length invalid, rep_len=0x%x\n", __func__,
		       rep_len);
		kfree(recv_buffer);
		return -EPROTO;
	}

	set_resp = (struct dh_mp_dtcm_para_set_resp *)(rep_ptr +
						       ZXDH_CHAN_REPS_LEN);
	if (set_resp->status_code != 200) {
		pr_err("[zxdh_rdma] [%s] response status invalid, statuc_code=0x%x\n",
		       __func__, set_resp->status_code);
		kfree(recv_buffer);
		return -EPROTO;
	}

	pr_info("[zxdh_rdma] resp: para_id=%d\n", para_id);
	kfree(recv_buffer);

	return 0;
}

static void clean_bond_old_gid(struct ib_device *ibdev, struct net_device *primary_netdev){

	struct ib_port_attr attr;
	const struct ib_gid_attr *gid_attr;
	struct net_device *ndev;
	int err;
	int i;
	err = ib_query_port(ibdev, 1, &attr);
	if (err)
		return;

	for (i = 0; i < attr.gid_tbl_len; i++) {
		gid_attr = rdma_get_gid_attr(ibdev, 1, i);
		if (IS_ERR(gid_attr))
			continue;
		#ifndef IB_READ_GID_ATTRIBUTE_NETDEVICE_NOT_DEFINE
		rcu_read_lock();
		ndev = rdma_read_gid_attr_ndev_rcu(gid_attr);
		if (IS_ERR(ndev)) {
			rcu_read_unlock();
			continue;
		}
		rcu_read_unlock();
		#else
		ndev = gid_attr->ndev;
		#endif

		if (ndev != NULL && ndev == primary_netdev) {
			pr_info("[zxdh_rdma] [rdma_bond] clean bond old gid ndev name=%s primary_netdev name=%s i=%d", ndev->name, primary_netdev->name, i);
			pr_info("[zxdh_rdma] [rdma_bond] gid.subnet_prefix=0x%llx, gid=%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x", gid_attr->gid.global.subnet_prefix,
				gid_attr->gid.raw[0], gid_attr->gid.raw[1], gid_attr->gid.raw[2], gid_attr->gid.raw[3],
				gid_attr->gid.raw[4], gid_attr->gid.raw[5], gid_attr->gid.raw[6], gid_attr->gid.raw[7],
				gid_attr->gid.raw[8], gid_attr->gid.raw[9], gid_attr->gid.raw[10], gid_attr->gid.raw[11],
				gid_attr->gid.raw[12], gid_attr->gid.raw[13], gid_attr->gid.raw[14], gid_attr->gid.raw[15]);
			rdma_put_gid_attr(gid_attr);
		} else if (ndev != NULL && primary_netdev != NULL) {
			pr_info("[zxdh_rdma] [rdma_bond] ndev and primary_netdev not equal: ndev name=%s primary_netdev name=%s i=%d", ndev->name, primary_netdev->name, i);
		}
		rdma_put_gid_attr(gid_attr);
	}
}

int32_t switch_bound_master_netdev(struct net_device *primary_netdev, struct net_device *linux_bond_netdev, bool hb_enable)
{
	struct zxdh_device *iwdev = NULL;
	struct ib_device *ibdev;
	struct ib_device *new_ibdev = NULL;
	struct zxdh_pci_f *rf;
	struct net_device *old_netdev;
	struct net_device *new_netdev;
	struct bonding *bond;
#ifdef NETDEV_TO_IBDEV_SUPPORT
	int ret;
#endif
	if (!primary_netdev || !linux_bond_netdev) {
		pr_err("[zxdh_rdma] [rdma_bond] primary_netdev or linux_bond_netdev is NULL.\n");
		return -1;
	}

	pr_info("[zxdh_rdma] [rdma_bond] primary:%s bond:%s hb_enable:%d\n", primary_netdev->name, linux_bond_netdev->name, hb_enable);

	if (hb_enable) {
		old_netdev = primary_netdev;
		new_netdev = linux_bond_netdev;
	} else {
		old_netdev = linux_bond_netdev;
		new_netdev = primary_netdev;
	}

	ibdev = ib_device_get_by_netdev(old_netdev, RDMA_DRIVER_ZXDH);
	if (!ibdev) {
		new_ibdev = ib_device_get_by_netdev(new_netdev, RDMA_DRIVER_ZXDH);
		if (!new_ibdev) {
			pr_warn("[zxdh_rdma] [rdma_bond] cann't get ib device by netdev.\n");
			return -1;
		} else {
			pr_info("[zxdh_rdma] [rdma_bond] ib device of new netdev: %s already exists\n", new_netdev->name);
			ib_device_put(new_ibdev);
			return 0;
		}
    }
	iwdev = to_iwdev(ibdev);
	if (!iwdev) {
		pr_err("[zxdh_rdma] [rdma_bond] ibdev to iwdev failed.\n");
		ib_device_put(ibdev);
		return -1;
	}
	clean_bond_old_gid(ibdev, old_netdev);

	// bond/解bond的时候需要把 主口/bond口 子接口都删掉
	zxdh_handle_upper_dev(iwdev, ZXDH_CMD_NP_MAC_DEL);
	zxdh_del_dpp_mac_tbl(iwdev, old_netdev, false);

	rf = iwdev->rf;
	if (!rf) {
		pr_err("[zxdh_rdma] [rdma_bond] rf is NULL\n");
		ib_device_put(ibdev);
		return -1;
	}

	if (rf->sc_dev.hw_attrs.skip_hw == true) {
		pr_err("[rdma_bond] skip_hw is true\n");
		ib_device_put(ibdev);
		return -1;
	}

    pr_info("[zxdh_rdma] [rdma_bond] %s ==> %s\n", old_netdev->name, new_netdev->name);
#ifdef NETDEV_TO_IBDEV_SUPPORT
    pr_info("[zxdh_rdma] [rdma_bond] NETDEV_TO_IBDEV_SUPPORT is defined\n");
	ret = ib_device_set_netdev(ibdev, new_netdev, 1);
	if (ret) {
		pr_err("[zxdh_rdma] [rdma_bond] ib device set netdev error, ret=%d\n", ret);
		ib_device_put(ibdev);
		return -1;
	}
	iwdev->netdev = new_netdev;
#else
    pr_info("[zxdh_rdma] [rdma_bond] NETDEV_TO_IBDEV_SUPPORT is not defined\n");
    iwdev->netdev = new_netdev;
#endif
    rdma_roce_rescan_device(ibdev);

	zxdh_add_dpp_mac_tbl(iwdev, iwdev->netdev);
	zxdh_handle_upper_dev(iwdev, ZXDH_CMD_NP_MAC_ADD);
	pr_info("[zxdh_rdma] [rdma_bond] add dpp mac tbl\n");
	if (netif_is_lag_master(new_netdev) && hb_enable) {
		bond = netdev_priv(new_netdev);
		iwdev->bond_slave_cnt = bond->slave_cnt;
	} else {
		iwdev->bond_slave_cnt = 0;
	}
	create_debugfs_default_entry(rf, ZRDMA_DEBUGFS_MODE_BOND);

    ib_device_put(ibdev);
    return 0;
}



/***
 * @brief Set the rdma speed to firmware, triggering speed reconfiguration.
 *
 * @param netdev zxdh_net device
 * @param bps for speed
 * @param epid_pfid ep and pf id
 * @return int32_t
 */
int32_t set_rdma_firmware_speed(struct net_device *netdev, uint32_t bps)
{
	int ret = 0;
	uint32_t status_code = 0;
	struct zxdh_device *iwdev;
	struct ib_device *ibdev;
	struct zxdh_pci_f *rf;
	struct zxdh_hwbond_speed_set_cmd cmd = { 0 };
	struct rdma_chan_msg_para para = { 0 };
	struct bonding *bond;
	u8 up_slaves = 0;
	struct slave *slave;
	struct list_head *iter;

	if (netdev == NULL)
	{
		pr_err("[zxdh_rdma] %s: [rdma_bond] netdev is null\n", __func__);
		return -1;
	}

	pr_info("[zxdh_rdma] [rdma_bond] new speed: %d, netdev: %s\n", bps, netdev->name);
	ibdev = ib_device_get_by_netdev(netdev, RDMA_DRIVER_ZXDH);
	if (!ibdev)
		return -1;
	iwdev = to_iwdev(ibdev);
	if (!iwdev) {
		ib_device_put(ibdev);
		return -1;
	}
	rf = iwdev->rf;
	if (!rf) {
		ib_device_put(ibdev);
		return -1;
	}

	if (rf->sc_dev.hw_attrs.skip_hw == true) {
		ib_device_put(ibdev);
		return -1;
	}

	if (netif_is_lag_master(netdev)) {
		bond = netdev_priv(netdev);
		rcu_read_lock();
		bond_for_each_slave_rcu(bond, slave, iter) {
			if (bond_slave_is_up(slave))
				up_slaves++;
		}
		rcu_read_unlock();
		if (iwdev->bond_slave_cnt != up_slaves)
			iwdev->bond_slave_cnt = up_slaves;
		cmd.op_code = RDMA_HWBOND_SPEED_SET;
	} else {
		iwdev->bond_slave_cnt = 0;
		cmd.op_code = RDMA_UNBOND_SPEED_SET;
	}

	iwdev->netdev_speed = bps;
	cmd.req.speed = bps;
	pr_info("[zxdh_rdma] [rdma_bond] new ep_id=%d pf_id=%d up_slave_cnt:%d\n", rf->ep_id, rf->pf_id, iwdev->bond_slave_cnt);
	if (rf->pf_id == 1) /* Avoidance problem: The 25.30 driver and 25.40 firmware passed a speed_valid of true, causing the bond's speed to be configured to PF1 */
		cmd.req.epid_pfid = DH_MP_SPEED_EPID_PFID_GEN(rf->ep_id, DH_MP_SPEED_PFID1);
	else
		cmd.req.epid_pfid = DH_MP_SPEED_EPID_PFID_GEN(rf->ep_id, rf->pf_id);
	pr_info("[zxdh_rdma] [rdma_bond] epid_pfid=0x%x\n", cmd.req.epid_pfid);

	para.in_buf = (uint8_t *)&cmd;
	para.in_size = sizeof(cmd);
	para.out_buf = (uint8_t *)&status_code;
	para.out_size = sizeof(status_code);

	ret = rdma_chan_msg_send(rf, &para);
	if (ret) {
		pr_info("[zxdh_rdma] [%s] send msg failed, ret:%d", __func__, ret);
		ib_device_put(ibdev);
		return -1;
	}

	if (status_code != 200) {
		pr_info("[zxdh_rdma] [%s] status code not ok, status:%d", __func__,
			status_code);
		ib_device_put(ibdev);
		return -1;
	}

	ib_device_put(ibdev);
	return 0;
}

int32_t set_rdma_port_speed(struct net_device *netdev, uint32_t bps)
{
	int ret = 0;
	uint32_t status_code = 0;
	struct zxdh_device *iwdev;
	struct ib_device *ibdev;
	struct zxdh_pci_f *rf;
	struct zxdh_port_speed_set_cmd cmd = { 0 };
	struct rdma_chan_msg_para para = { 0 };
	uint32_t epid_pfid;

	if (netdev == NULL) {
		pr_err("[zxdh_rdma] %s: [rdma_port] netdev is null\n", __func__);
		return -1;
	}

	pr_info("[zxdh_rdma] [rdma_port] new speed: %d, netdev: %s\n", bps, netdev->name);
	ibdev = ib_device_get_by_netdev(netdev, RDMA_DRIVER_ZXDH);
	if (!ibdev)
		return -1;
	iwdev = to_iwdev(ibdev);
	if (!iwdev) {
		ib_device_put(ibdev);
		return -1;
	}
	rf = iwdev->rf;
	if (!rf) {
		ib_device_put(ibdev);
		return -1;
	}

	if (rf->sc_dev.hw_attrs.skip_hw == true) {
		ib_device_put(ibdev);
		return -1;
	}

	iwdev->netdev_speed = bps;
	epid_pfid = DH_MP_SPEED_EPID_PFID_GEN(rf->ep_id, rf->pf_id);
	cmd.op_code = RDMA_PORT_SPEED_SET;
	cmd.req.speed = bps;
	cmd.req.epid_pfid = epid_pfid;

	pr_debug("[zxdh_rdma] [rdma_port] ep_id=%d pf_id=%d epid_pfid=0x%x\n",
		 rf->ep_id, rf->pf_id, epid_pfid);

	para.in_buf = (uint8_t *)&cmd;
	para.in_size = sizeof(cmd);
	para.out_buf = (uint8_t *)&status_code;
	para.out_size = sizeof(status_code);

	ret = rdma_chan_msg_send(rf, &para);
	if (ret) {
		pr_info("[zxdh_rdma] [%s] send msg failed, ret:%d", __func__, ret);
		ib_device_put(ibdev);
		return -1;
	}

	if (status_code != 200) {
		pr_info("[zxdh_rdma] [%s] status code not ok, status:%d", __func__,
			status_code);
		ib_device_put(ibdev);
		return -1;
	}

	pr_info("[zxdh_rdma] [rdma_port] Port speed tokens configured successfully: netdev=%s, speed=%u Mbps, ep_id=%d, pf_id=%d\n",
		netdev->name, bps, rf->ep_id, rf->pf_id);

	ib_device_put(ibdev);
	return 0;
}

int set_rdma_vf_num(struct zxdh_rdma_sriov_event_info *sriov_info, u64 *vf_pblem_cnt)
{
	int ret = 0;
	uint8_t *rep_ptr;
	uint16_t rep_len = 0;
	uint8_t rep_valid = 0;
	size_t recv_len = 0;
	void *recv_buffer = NULL;

	struct zxdh_rdma_vf_num_set_cmd cmd = { 0 };
	struct zxdh_mgr mgr = { 0 };
	struct zxdh_pci_bar_msg in = { 0 };
	struct zxdh_msg_recviver_mem result = { 0 };
	struct dh_rdma_vf_num_set_resp *resp;
	u32 cnt = 0;
	u32 cnt_num = ZXDH_BAR_MSG_RETRY_NUM;

	mgr.pdev = sriov_info->pdev;
	ret = dh_rdma_pf_pcie_id_get(&mgr);
	if (ret) {
		pr_err("[zxdh_rdma] [%s] get pf pcie_id failed, ret=%d\n", __func__, ret);
		return -EINVAL;
	}

	recv_len = ZXDH_CHAN_REPS_LEN +  sizeof(struct dh_rdma_vf_num_set_resp);
	recv_buffer = (void *)kzalloc(recv_len, GFP_KERNEL);
	if (!recv_buffer)
		return -ENOMEM;

	cmd.op_code = RDMA_VFS_NUM_SET;
	cmd.req.ep_id = (sriov_info->vport_id >> 12) & 0x7;
	cmd.req.pf_id = (sriov_info->vport_id >> 8) & 0x7;
	cmd.req.num_vfs = sriov_info->num_vfs;

	in.payload_addr = (void *)&cmd;
	in.payload_len = sizeof(cmd);

	in.src = MSG_CHAN_END_PF;
	in.dst = MSG_CHAN_END_RISC;
	in.event_id = MODULE_RDMA;
	in.virt_addr = sriov_info->bar0_virt_addr + ZXDH_BAR_CHAN_OFFSET;
	in.src_pcieid = mgr.pcie_id;

	// resv buffer preparation
	result.recv_buffer = recv_buffer;
	result.buffer_len = recv_len;

	do {
		ret = zxdh_bar_chan_sync_msg_send(&in, &result);
		if ((ret != ZXDH_BAR_ERR_TIME_OUT) && (ret != ZXDH_BAR_ERR_LOCK_FAILED)) {
			break;
		}
		cnt++;
	}while(cnt < cnt_num);

	if (ret) {
		pr_err("[zxdh_rdma] [%s] message send failed, ret=%d cnt=%d\n", __func__, ret, cnt);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_ptr = (uint8_t *)recv_buffer;
	rep_valid = *rep_ptr;
	if (rep_valid != MSG_REP_VALID) {
		pr_err("[zxdh_rdma] [%s] response message invalid, rep_valid=0x%x\n",
		       __func__, rep_valid);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_len = *(uint16_t *)(rep_ptr + MSG_REP_LEN_OFFSET);
	if (rep_len != recv_len - ZXDH_CHAN_REPS_LEN) {
		pr_err("[zxdh_rdma] [%s] response length invalid, rep_len=0x%x\n", __func__,
		       rep_len);
		kfree(recv_buffer);
		return -EPROTO;
	}

	resp = (struct dh_rdma_vf_num_set_resp *)(rep_ptr + ZXDH_CHAN_REPS_LEN);
	if (resp->status_code != 200) {
		pr_info("[zxdh_rdma] [%s] status code not ok, status:%d", __func__,
			resp->status_code);
		kfree(recv_buffer);
		return -EPROTO;
	}

	*vf_pblem_cnt = resp->vf_pblem_cnt;
	kfree(recv_buffer);
	return 0;
}
static int zxdh_data_cal_sum(u8 *buf, u32 buf_len)
{
	u8 sum = 0;
	u32 i;

	if (buf == NULL)
		return -ENOMEM;

	for (i = 0; i < buf_len; i++)
        sum += buf[i];
	
	return sum;
}

static int zxdh_data_check_sum(u8 *buf, u8 check_sum, u32 buf_len)
{
	u8 sum = 0;
	u32 i;

	if (buf == NULL)
		return -ENOMEM;
	for (i = 0; i < buf_len; i++)
		sum += buf[i];

	if (sum != check_sum)
		return -EINVAL;
	
	return 0;
}

static int zxdh_resp_msg_check(u8 *buf, u32 buf_len)
{
	u32 len;

	if (buf == NULL)
		return -ENOMEM;
	if ((buf_len > ZXDH_RESP_MSG_LEN) || (buf_len < ZXDH_MSG_MIN_LEN))
		return -ERANGE;
	if (buf[0] != ZXDH_VER_HEADER_H)
		return -EINVAL;
	if (buf[1] != ZXDH_VER_HEADER_L)
		return -EINVAL;

	len = buf[2] + 3;
	return zxdh_data_check_sum(&buf[3], buf[len], buf[2]);
}


int zxdh_req_cmd_ver(struct zxdh_pci_f *rf)
{
	int ret = 0;
	uint8_t rep_valid = 0;
	uint16_t rep_len = 0;
	uint32_t msg_len = 0;
	uint8_t *rep_ptr;
	struct zxdh_mgr mgr = { 0 };
	struct zxdh_pci_bar_msg in = { 0 };
	struct zxdh_msg_recviver_mem result = { 0 };
	struct iidc_core_dev_info *cdev_info;
	struct zxdh_req_msg req_msg = { 0 };
	struct zxdh_resp_msg *resp_msg;
	size_t recv_len;
	void *recv_buffer;
	u32 cnt = 0;
	u32 cnt_num = ZXDH_BAR_MSG_RETRY_NUM;

	if (!rf)
		return -ENOMEM;
	if (rf->sc_dev.driver_load == false)
		cnt_num = ZXDH_BAR_MSG_DEFAULT_NUM;
	rf->sc_dev.flr_query = 0;
	rf->sc_dev.fw_np_mac = 0;
	rf->sc_dev.vf_flr_cap = 0;
	/*The default version number is 0, and subsequent modified versions will be incremented by 1.
     The storage location of the version number is defined by oneself.*/
	memset(rf->ver_buf, 0 , ZXDH_RDMA_VER_LEN*sizeof(u8));
	cdev_info = (struct iidc_core_dev_info *)rf->cdev;

	// query pcie id
	mgr.pdev = cdev_info->pdev;
	ret = dh_rdma_pf_pcie_id_get(&mgr);
	if (ret) {
		pr_err("[zxdh_rdma] [%s] get pf pcie_id failed, ret=%d\n", __func__, ret);
		return -EINVAL;
	}

	recv_len = ZXDH_CHAN_REPS_LEN + sizeof(struct zxdh_resp_msg);
	recv_buffer = (void *)kzalloc(recv_len, GFP_KERNEL);
	if (!recv_buffer)
		return -ENOMEM;

	// commnad preparation
	req_msg.op_code = RDMA_REQ_VER;
	req_msg.buf[0] = ZXDH_VER_HEADER_H;
	req_msg.buf[1] = ZXDH_VER_HEADER_L;
	req_msg.buf[2] = 1;
	req_msg.buf[3] = 1;
	req_msg.buf[4] = 1;
	
	// get message preparation
	in.payload_addr = (void *)&req_msg;
	in.payload_len = sizeof(struct zxdh_req_msg);
	in.src = rf->ftype == 0 ? MSG_CHAN_END_PF : MSG_CHAN_END_VF;
	in.dst = MSG_CHAN_END_RISC;
	in.event_id = MODULE_RDMA;
	in.virt_addr = (u64)cdev_info->hw_addr + ZXDH_BAR_CHAN_OFFSET;
	in.src_pcieid = mgr.pcie_id;

	// resv buffer preparation
	result.recv_buffer = recv_buffer;
	result.buffer_len = recv_len;

	do {
		ret = zxdh_bar_chan_sync_msg_send(&in, &result);
		if ((ret != ZXDH_BAR_ERR_TIME_OUT) && (ret != ZXDH_BAR_ERR_LOCK_FAILED)) {
			break;
		}
		cnt++;
	}while(cnt < cnt_num);
	
	if (ret) {
		pr_err("[zxdh_rdma] [%s] message send failed, ret=%d cnt=%d\n", __func__, ret, cnt);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_ptr = (uint8_t *)recv_buffer;
	rep_valid = *rep_ptr;
	if (rep_valid != MSG_REP_VALID) {
		pr_err("[zxdh_rdma] [%s] response message invalid, rep_valid=0x%x\n",
		       __func__, rep_valid);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_len = *(uint16_t *)(rep_ptr + MSG_REP_LEN_OFFSET);
	if (rep_len != recv_len - ZXDH_CHAN_REPS_LEN) {
		pr_err("[zxdh_rdma] [%s] response length invalid, rep_len=0x%x\n", __func__,
		       rep_len);
		kfree(recv_buffer);
		return -ERANGE;
	}

	resp_msg = (struct zxdh_resp_msg *)(rep_ptr + ZXDH_CHAN_REPS_LEN);
	if (resp_msg->op_code != RDMA_RESP_VER) {
		pr_err("[zxdh_rdma] [%s] response op code invalid, op_code=0x%x\n",
		       __func__, resp_msg->op_code);
		kfree(recv_buffer);
		return -EPROTO;
	}
	
	msg_len =  resp_msg->buf[2] + 4;
	ret = zxdh_resp_msg_check(resp_msg->buf, msg_len);
	if (ret == 0) {
		pr_info("[zxdh_rdma] [%s] rdma get ver cfg success!\n",__func__);
		if(resp_msg->buf[2] > ZXDH_RDMA_VER_LEN)
			memcpy(rf->ver_buf, &resp_msg->buf[3], ZXDH_RDMA_VER_LEN*sizeof(u8));
		else
			memcpy(rf->ver_buf, &resp_msg->buf[3], resp_msg->buf[2]*sizeof(u8));
		rf->sc_dev.flr_query = rf->ver_buf[0];
        rf->sc_dev.fw_np_mac = rf->ver_buf[1];
        rf->sc_dev.vf_flr_cap = rf->ver_buf[2];
		rf->use_blue_flame_flag = resp_msg->buf[6];
		#if !(defined(__x86_64__) || defined(__i386__))  // 非x86架构不支持蓝焰
		if(rf->use_blue_flame_flag) {
			rf->use_blue_flame_flag = 0;
			if (!rf->ftype)
				pr_warn("[zxdh_rdma] blue flame not supported on current architecture (only x86 supported)\n");
		}
		#endif
        pr_info("[zxdh_rdma] [%s] flr_query:%d fw_np_mac:%d vf_flr_cap:%d use_blue_flame_flag:%hhu !\n", 
			__func__, rf->sc_dev.flr_query, rf->sc_dev.fw_np_mac, rf->sc_dev.vf_flr_cap, rf->use_blue_flame_flag);
	}
	kfree(recv_buffer);

	return ret;
}

static int zxdh_common_msg_check(u8 *buf, u32 buf_len)
{
	u8 idx = 0;
	if (buf == NULL)
		return -ENOMEM;
	if (buf_len > ZXDH_COMMON_VALID_LEN) 
		return -ERANGE;
	if (buf[0] != ZXDH_COMMON_MSG_HEADER_H)
		return -EINVAL;
	if (buf[1] != ZXDH_COMMON_MSG_HEADER_L)
		return -EINVAL;
	idx = buf_len + 4;

	return zxdh_data_check_sum(&buf[4], buf[idx], buf_len);
}

static int zxdh_rdma_check_common_msg(struct zxdh_pci_f *rf, struct zxdh_rdma_common_req_msg *req, struct zxdh_rdma_common_resp_msg *resp)
{
	if (!rf) {
		pr_err("[zxdh_rdma] [%s] rf is NULL!\n",__func__);
		return -ENOMEM;
	}

	if (!req) {
		pr_err("[zxdh_rdma] [%s] req is NULL!\n",__func__);
		return -ENOMEM;
	}

	if (req->len > ZXDH_COMMON_VALID_LEN) {
		pr_err("[zxdh_rdma] [%s]req date len out of range,len:%d\n", __func__, req->len);
		return -EINVAL;
	}

	return 0;
}

/*
Protocol Frame Format
1. External opcode
2. Header
    byte_0: 0xAA
	byte_1: 0x55
3. Internal opcode
    byte_2
4. Payload length
    byte_3
5. Payload
    byte_4~byte_n
6. Checksum
    byte_n + 1:sum,payload accumulated without rounding
buf max value is 65
*/
int zxdh_rdma_send_common_msg(struct zxdh_pci_f *rf, struct zxdh_rdma_common_req_msg *req, struct zxdh_rdma_common_resp_msg *resp)
{
	int ret = 0;
	uint8_t rep_valid = 0;
	uint16_t rep_len = 0;
	uint32_t msg_len = 0;
	uint8_t *rep_ptr;
	struct zxdh_mgr mgr = { 0 };
	struct zxdh_pci_bar_msg in = { 0 };
	struct zxdh_msg_recviver_mem result = { 0 };
	struct iidc_core_dev_info *cdev_info;
	struct zxdh_rdma_bar_req_msg req_msg = { 0 };
	struct zxdh_rdma_bar_resp_msg *resp_msg;
	size_t recv_len;
	void *recv_buffer;
	u32 cnt = 0;
	u8 resp_opcode;
	u32 cnt_num = ZXDH_BAR_MSG_RETRY_NUM;
	u8 idx = 0;

	ret = zxdh_rdma_check_common_msg(rf, req, resp);
	if (ret)
		return ret;

	if (rf->sc_dev.driver_load == false)
		cnt_num = ZXDH_BAR_MSG_DEFAULT_NUM;

	memset(req_msg.buf, 0 , ZXDH_COMMON_BUF_LEN*sizeof(u8));
	cdev_info = (struct iidc_core_dev_info *)rf->cdev;

	// query pcie id
	mgr.pdev = cdev_info->pdev;
	ret = dh_rdma_pf_pcie_id_get(&mgr);
	if (ret) {
		pr_err("[zxdh_rdma] [%s] get pf pcie_id failed, ret=%d\n", __func__, ret);
		return -EINVAL;
	}

	recv_len = ZXDH_CHAN_REPS_LEN + sizeof(struct zxdh_rdma_bar_resp_msg);
	recv_buffer = (void *)kzalloc(recv_len, GFP_KERNEL);
	if (!recv_buffer) {
		pr_err("[zxdh_rdma] [%s] recv_buffer is NULL!\n",__func__);
		return -ENOMEM;
	}

	// commnad preparation
	req_msg.op_code = RDMA_COMMON_OPCODE;
	req_msg.buf[0] = ZXDH_COMMON_MSG_HEADER_H;
	req_msg.buf[1] = ZXDH_COMMON_MSG_HEADER_L;
	req_msg.buf[2] = req->opcode;
	req_msg.buf[3] = req->len;
	memcpy(&req_msg.buf[4], req->buf, req->len*sizeof(u8));
	idx = 4 + req->len;
	req_msg.buf[idx] = zxdh_data_cal_sum((u8 *)req->buf, req->len);
	// get message preparation
	in.payload_addr = (void *)&req_msg;
	in.payload_len = sizeof(struct zxdh_rdma_bar_req_msg);
	in.src = rf->ftype == 0 ? MSG_CHAN_END_PF : MSG_CHAN_END_VF;
	in.dst = MSG_CHAN_END_RISC;
	in.event_id = MODULE_RDMA;
	in.virt_addr = (u64)cdev_info->hw_addr + ZXDH_BAR_CHAN_OFFSET;
	in.src_pcieid = mgr.pcie_id;

	// resv buffer preparation
	result.recv_buffer = recv_buffer;
	result.buffer_len = recv_len;

	do {
		ret = zxdh_bar_chan_sync_msg_send(&in, &result);
		if ((ret != ZXDH_BAR_ERR_TIME_OUT) && (ret != ZXDH_BAR_ERR_LOCK_FAILED)) {
			break;
		}
		cnt++;
	}while(cnt < cnt_num);
	
	if (ret) {
		pr_err("[zxdh_rdma] [%s] message send failed, ret=%d cnt=%d\n", __func__, ret, cnt);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_ptr = (uint8_t *)recv_buffer;
	rep_valid = *rep_ptr;
	if (rep_valid != MSG_REP_VALID) {
		pr_err("[zxdh_rdma] [%s] response message invalid, rep_valid=0x%x\n",
		       __func__, rep_valid);
		kfree(recv_buffer);
		return -EPROTO;
	}

	rep_len = *(uint16_t *)(rep_ptr + MSG_REP_LEN_OFFSET);
	if (rep_len != (recv_len - ZXDH_CHAN_REPS_LEN)) {
		pr_err("[zxdh_rdma] [%s] response length invalid, rep_len=0x%x\n", __func__,
		       rep_len);
		kfree(recv_buffer);
		return -ERANGE;
	}

	resp_msg = (struct zxdh_rdma_bar_resp_msg *)(rep_ptr + ZXDH_CHAN_REPS_LEN);
	if (resp_msg->op_code != RDMA_COMMON_RESP_OPCODE) {
		pr_err("[zxdh_rdma] [%s] response op code invalid, op_code=0x%x\n",
		       __func__, resp_msg->op_code);
		kfree(recv_buffer);
		return -EPROTO;
	}
	
	msg_len =  resp_msg->buf[3];
	resp_opcode = resp_msg->buf[2];
	if (msg_len > ZXDH_COMMON_VALID_LEN) {
		pr_err("[zxdh_rdma] [%s]resp date len out of range,msg_len:%d\n", __func__, msg_len);
		kfree(recv_buffer);
		return -EINVAL;
	}
	ret = zxdh_common_msg_check(resp_msg->buf, msg_len);
	if (ret == 0) {
		if (resp) {
			resp->opcode = resp_opcode;
			resp->len = msg_len;
			memcpy(resp->buf, &resp_msg->buf[4], msg_len*sizeof(u8));
		}
	} else
		pr_err("[zxdh_rdma] [%s] check msg failed! ret:%d\n", __func__, ret);
	kfree(recv_buffer);

	return ret;
}


extern void rdma_add_del_ip(struct zxdh_rdma_to_eth_ip_para *info);
void rdma_update_remote_ip(struct zxdh_rdma_to_eth_ip_para *info)
{
	pr_debug("[zxdh_rdma] %s[%d]: ipv4=%d, name=%s, src_ip=0x%x-0x%x-0x%x-0x%x, dst_ip=0x%x-0x%x-0x%x-0x%x, src_mac=0x%llx, dst_mac=0x%llx, mode=%d, linked_fid=0x%x\n",
		__func__, __LINE__, info->ipv4, info->ifname, info->src_ip[0], info->src_ip[1], info->src_ip[2], info->src_ip[3],
		info->dst_ip[0], info->dst_ip[1], info->dst_ip[2], info->dst_ip[3], info->src_mac, info->dst_mac, info->mode, info->linked_fid);
	rdma_add_del_ip(info);
}

/* 检查是否为虚拟机环境
返回值: 
0, 表示主机环境;
1, 表示虚拟机环境 */
static int is_virtual_env(void)
{
	int ret = 0;

#ifdef CONFIG_X86
	/* x86架构检查CPU状态的 hypervisor 位 (Intel/AMD 支持) */
	ret = boot_cpu_has(X86_FEATURE_HYPERVISOR);

#else
	/* 其他架构(ARM), 使用DMI信息来判断 */
	const char *vendor;
	const char *product;
	vendor = dmi_get_system_info(DMI_SYS_VENDOR);
	product = dmi_get_system_info(DMI_PRODUCT_NAME);
	if (!vendor || !product) {
		// 获取DMI信息失败, 默认使用主机模式
		pr_err("[zxdh_rdma] [%s] failed to get DMI info, default in host mode! \n", __func__);
	} else {
		ret = (strstr(vendor, "QEMU") || strstr(vendor, "KVM") || strstr(vendor, "VMware") ||
			strstr(product, "QEMU") || strstr(product, "KVM") || strstr(vendor, "VMware"));
	}
	
#endif

	return ret;
}

int zxdh_vm_env_check(struct zxdh_pci_f *rf)
{
	int ret = 0;

	ret = is_virtual_env();

	if (ret) {
		rf->vm_info.vm_mode = true;
	} else {
		rf->vm_info.vm_mode = false;
	}
	pr_info("[zxdh_rdma] [%s] vm_mode:0x%x \n", __func__, rf->vm_info.vm_mode);	
	return ret;
}

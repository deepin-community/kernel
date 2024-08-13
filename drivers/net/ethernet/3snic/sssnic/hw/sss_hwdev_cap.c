// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/semaphore.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_hwdev.h"
#include "sss_hwif_api.h"
#include "sss_hwdev_cap.h"

/* RDMA resource */
#define K_UNIT			BIT(10)
#define M_UNIT			BIT(20)
#define G_UNIT			BIT(30)

/* L2NIC */
#define SSS_CFG_MAX_QP	256

/* RDMA */
#define SSS_RDMA_RSVD_QP		2
#define SSS_ROCE_MAX_WQE		(8 * K_UNIT - 1)

#define SSS_RDMA_MAX_SQ_SGE			16

#define SSS_ROCE_MAX_RQ_SGE			16

#define SSS_RDMA_MAX_SQ_DESC_SIZE		256

/* (256B(cache_line_len) - 16B(ctrl_seg_len) - 48B(max_task_seg_len)) */
#define SSS_ROCE_MAX_SQ_INLINE_DATA_SIZE	192

#define SSS_ROCE_MAX_RQ_DESC_SIZE			256

#define SSS_ROCE_QPC_ENTRY_SIZE			512

#define SSS_WQEBB_SIZE					64

#define SSS_ROCE_RDMARC_ENTRY_SIZE		32
#define SSS_ROCE_MAX_QP_INIT_RDMA		128
#define SSS_ROCE_MAX_QP_DEST_RDMA		128

#define SSS_ROCE_MAX_SRQ_WQE		(16 * K_UNIT - 1)
#define SSS_ROCE_RSVD_SRQ			0
#define SSS_ROCE_MAX_SRQ_SGE		15
#define ROCE_SRQC_ENTERY_SIZE			64

#define SSS_ROCE_MAX_SRQ		0x400
#define SSS_ROCE_MAX_CQ			0x800
#define SSS_ROCE_MAX_QP			0x400
#define SSS_ROCE_MAX_MPT		0x400
#define SSS_ROCE_MAX_DRC_QP		0x40

#define SSS_RDMA_MAX_CQE		(8 * M_UNIT - 1)
#define SSS_RDMA_RSVD_CQ		0

#define SSS_RDMA_CQC_ENTRY_SIZE	128

#define SSS_RDMA_CQE_SIZE			64
#define SSS_RDMA_RSVD_MRW		128
#define SSS_RDMA_MPT_ENTRY_SIZE	64
#define SSS_RDMA_MTT_NUM		(1 * G_UNIT)
#define SSS_LOG_MTT_SEG			5
#define SSS_MTT_ENTRY_SIZE		8
#define SSS_LOG_RDMARC_SEG		3

#define SSS_LOCAL_ACK_DELAY		15
#define SSS_RDMA_PORT_NUM		1
#define SSS_ROCE_MAX_MSG_SIZE		(2 * G_UNIT)

#define SSS_DB_PAGE_SIZE_K			(4 * K_UNIT)
#define SSS_DWQE_SIZE				256

#define SSS_PD_NUM				(128 * K_UNIT)
#define SSS_RSVD_PD				0

#define SSS_MAX_XRCD			(64 * K_UNIT)
#define SSS_RSVD_XRCD			0

#define SSS_MAX_GID_PER_PORT	128
#define SSS_GID_ENTRY_SIZE		32
#define SSS_RSVD_LKEY			((SSS_RDMA_RSVD_MRW - 1) << 8)
#define SSS_PAGE_SIZE_CAP			((1UL << 12) | (1UL << 16) | (1UL << 21))
#define SSS_ROCE_MODE			1

#define SSS_MAX_FRPL_LEN		511
#define SSS_MAX_PKEY			1

/* ToE */
#define SSS_TOE_PCTX_SIZE			1024
#define SSS_TOE_SCQC_SIZE			64

/* FC */
#define SSS_FC_PQPC_SIZE			256
#define SSS_FC_CQPC_SIZE			256
#define SSS_FC_SQE_SIZE			128
#define SSS_FC_SCQC_SIZE			64
#define SSS_FC_SCQE_SIZE			64
#define SSS_FC_SRQC_SIZE			64
#define SSS_FC_SRQE_SIZE			32

/* OVS */
#define SSS_OVS_PCTX_SIZE			512

/* PPA */
#define SSS_PPA_PCTX_SIZE			512

/* IPsec */
#define SSS_IPSEC_SACTX_SIZE		512

/* VirtIO */
#define SSS_VIRTIO_BASE_VQ_SIZE		2048U
#define SSS_VIRTIO_DEFAULT_VQ_SIZE	8192U

struct sss_cmd_dev_cap_cfg {
	struct sss_mgmt_msg_head head;

	u16 func_id;
	u16 rsvd;

	u8 host_id;
	u8 ep_id;
	u8 er_id;
	u8 port_id;

	u16 host_total_function;
	u8 pf_num;
	u8 pf_id_start;
	u16 vf_num;
	u16 vf_id_start;
	u8 host_oq_id_mask_val;
	u8 timer_en;
	u8 host_valid_bitmap;
	u8 rsvd_host;

	u16 svc_type;
	u16 max_vf;
	u8 flexq_en;
	u8 cos_valid_bitmap;
	u8 port_cos_valid_bitmap;
	u8 rsvd_func1;
	u32 rsvd_func2;

	u8 sf_svc_attr;
	u8 func_sf_en;
	u8 lb_mode;
	u8 smf_pg;

	u32 max_connect_num;
	u16 max_stick2cache_num;
	u16 bfilter_start_addr;
	u16 bfilter_len;
	u16 hash_bucket_num;

	u8 host_sf_en;
	u8 master_host_id;
	u8 srv_multi_host_mode;
	u8 rsvd2_sr;

	u32 rsvd_func3[5];

	/* l2nic */
	u16 nic_max_sq_id;
	u16 nic_max_rq_id;
	u16 nic_def_queue_num;
	u16 rsvd_nic1;
	u32 rsvd_nic2[2];

	/* RoCE */
	u32 roce_max_qp;
	u32 roce_max_cq;
	u32 roce_max_srq;
	u32 roce_max_mpt;
	u32 roce_max_drc_qp;

	u32 roce_cmtt_cl_start;
	u32 roce_cmtt_cl_end;
	u32 roce_cmtt_cl_size;

	u32 roce_dmtt_cl_start;
	u32 roce_dmtt_cl_end;
	u32 roce_dmtt_cl_size;

	u32 roce_wqe_cl_start;
	u32 roce_wqe_cl_end;
	u32 roce_wqe_cl_size;
	u8 roce_srq_container_mode;
	u8 rsvd_roce1[3];
	u32 rsvd_roce2[5];

	/* IPsec */
	u32 ipsec_max_sactx;
	u16 ipsec_max_cq;
	u16 rsvd_ipsec1;
	u32 rsvd_ipsec2[2];

	/* OVS */
	u32 ovs_max_qpc;
	u32 rsvd_ovs[3];

	/* ToE */
	u32 toe_max_pctx;
	u32 toe_max_cq;
	u16 toe_max_srq;
	u16 toe_srq_id_start;
	u16 toe_max_mpt;
	u16 toe_max_cctxt;
	u32 rsvd_toe[2];

	/* FC */
	u32 fc_max_pctx;
	u32 fc_max_scq;
	u32 fc_max_srq;

	u32 fc_max_cctx;
	u32 fc_cctx_id_start;

	u8 fc_vp_id_start;
	u8 fc_vp_id_end;
	u8 rsvd_fc1[2];
	u32 rsvd_fc2[5];

	/* VBS */
	u16 vbs_max_volq;
	u16 rsvd_vbs1;
	u32 rsvd_vbs2[3];

	u16 pseudo_vf_start_id;
	u16 pseudo_vf_num;
	u32 pseudo_vf_max_pctx;
	u16 pseudo_vf_bfilter_start_addr;
	u16 pseudo_vf_bfilter_len;
	u32 rsvd_glb[8];
};

enum {
	SSS_SF_SVC_FT_BIT = (1 << 0),
	SSS_SF_SVC_RDMA_BIT = (1 << 1),
};

enum sss_cfg_cmd {
	SSS_CFG_CMD_GET_CAP_CFG = 0,
	SSS_CFG_CMD_GET_HOST_TIMER = 1,
};

static void sss_print_pubic_cap(void *dev_hdl, const struct sss_service_cap *svc_cap)
{
	sdk_info(dev_hdl,
		 "Get public capbility: svc_type: 0x%x, chip_svc_type: 0x%x\n",
		 svc_cap->svc_type, svc_cap->chip_svc_type);
	sdk_info(dev_hdl,
		 "host_id: 0x%x, ep_id: 0x%x, er_id: 0x%x, port_id: 0x%x\n",
		 svc_cap->host_id, svc_cap->ep_id, svc_cap->er_id, svc_cap->port_id);
	sdk_info(dev_hdl,
		 "host_total_function: 0x%x, host_oq_id_mask_val: 0x%x, max_vf: 0x%x\n",
		 svc_cap->host_total_function, svc_cap->host_oq_id_mask_val, svc_cap->max_vf);
	sdk_info(dev_hdl,
		 "pf_num: 0x%x, pf_id_start: 0x%x, vf_num: 0x%x, vf_id_start: 0x%x\n",
		 svc_cap->pf_num, svc_cap->pf_id_start, svc_cap->vf_num, svc_cap->vf_id_start);
	sdk_info(dev_hdl,
		 "host_valid_bitmap: 0x%x, master_host_id: 0x%x, srv_multi_host_mode: 0x%x\n",
		 svc_cap->host_valid_bitmap, svc_cap->master_host_id, svc_cap->srv_multi_host_mode);
	sdk_info(dev_hdl,
		 "cos_valid_bitmap: 0x%x, port_cos_valid_bitmap: 0x%x, flexq_en: 0x%x, virtio_vq_size: 0x%x\n",
		 svc_cap->cos_valid_bitmap, svc_cap->port_cos_valid_bitmap, svc_cap->flexq_en,
		 svc_cap->virtio_vq_size);
	sdk_info(dev_hdl,
		 "pseudo_vf_start_id: 0x%x, pseudo_vf_num: 0x%x, pseudo_vf_max_pctx: 0x%x\n",
		 svc_cap->pseudo_vf_start_id, svc_cap->pseudo_vf_num, svc_cap->pseudo_vf_max_pctx);
	sdk_info(dev_hdl,
		 "pseudo_vf_bfilter_start_addr: 0x%x, pseudo_vf_bfilter_len: 0x%x\n",
		 svc_cap->pseudo_vf_bfilter_start_addr, svc_cap->pseudo_vf_bfilter_len);
}

static void sss_parse_qmm_cap(struct sss_hwdev *hwdev,
			      struct sss_service_cap *svc_cap, struct sss_cmd_dev_cap_cfg *cmd_cap)
{
	struct sss_dev_sf_svc_attr *sf_svc_attr = &svc_cap->sf_svc_attr;

	svc_cap->pseudo_vf_num = cmd_cap->pseudo_vf_num;
	svc_cap->pseudo_vf_cfg_num = cmd_cap->pseudo_vf_num;
	svc_cap->pseudo_vf_start_id = cmd_cap->pseudo_vf_start_id;
	svc_cap->pseudo_vf_max_pctx = cmd_cap->pseudo_vf_max_pctx;
	svc_cap->pseudo_vf_bfilter_start_addr = cmd_cap->pseudo_vf_bfilter_start_addr;
	svc_cap->pseudo_vf_bfilter_len = cmd_cap->pseudo_vf_bfilter_len;

	if (SSS_SUPPORT_VIRTIO_VQ_SIZE(hwdev))
		svc_cap->virtio_vq_size = (u16)(SSS_VIRTIO_BASE_VQ_SIZE << svc_cap->virtio_vq_size);
	else
		svc_cap->virtio_vq_size = SSS_VIRTIO_DEFAULT_VQ_SIZE;

	sf_svc_attr->rdma_en = !!(cmd_cap->sf_svc_attr & SSS_SF_SVC_RDMA_BIT);

	svc_cap->smf_pg = cmd_cap->smf_pg;
	svc_cap->lb_mode = cmd_cap->lb_mode;

	svc_cap->timer_en = cmd_cap->timer_en;
	svc_cap->bfilter_start_addr = cmd_cap->bfilter_start_addr;
	svc_cap->bfilter_len = cmd_cap->bfilter_len;
	svc_cap->host_oq_id_mask_val = cmd_cap->host_oq_id_mask_val;
	svc_cap->hash_bucket_num = cmd_cap->hash_bucket_num;
	svc_cap->max_stick2cache_num = cmd_cap->max_stick2cache_num;
	svc_cap->max_connect_num = cmd_cap->max_connect_num;
}

static void sss_parse_pubic_cap(struct sss_hwdev *hwdev,
				struct sss_service_cap *svc_cap,
				struct sss_cmd_dev_cap_cfg *cmd_cap,
				enum sss_func_type type)
{
	svc_cap->svc_type = cmd_cap->svc_type;
	svc_cap->chip_svc_type = cmd_cap->svc_type;

	svc_cap->ep_id = cmd_cap->ep_id;
	svc_cap->er_id = cmd_cap->er_id;
	svc_cap->host_id = cmd_cap->host_id;
	svc_cap->port_id = cmd_cap->port_id;

	svc_cap->host_total_function = cmd_cap->host_total_function;
	svc_cap->host_valid_bitmap = cmd_cap->host_valid_bitmap;
	svc_cap->master_host_id = cmd_cap->master_host_id;
	svc_cap->srv_multi_host_mode = cmd_cap->srv_multi_host_mode;

	svc_cap->flexq_en = cmd_cap->flexq_en;
	svc_cap->cos_valid_bitmap = cmd_cap->cos_valid_bitmap;
	svc_cap->port_cos_valid_bitmap = cmd_cap->port_cos_valid_bitmap;

	if (type != SSS_FUNC_TYPE_VF) {
		svc_cap->pf_num = cmd_cap->pf_num;
		svc_cap->pf_id_start = cmd_cap->pf_id_start;
		svc_cap->vf_num = cmd_cap->vf_num;
		svc_cap->vf_id_start = cmd_cap->vf_id_start;
		svc_cap->max_vf = cmd_cap->max_vf;
	} else {
		svc_cap->max_vf = 0;
	}

	svc_cap->sf_en = (type == SSS_FUNC_TYPE_PPF) ?
			 (!!cmd_cap->host_sf_en) : (!!cmd_cap->func_sf_en);

	sss_parse_qmm_cap(hwdev, svc_cap, cmd_cap);
	sss_print_pubic_cap(hwdev->dev_hdl, svc_cap);
}

static void sss_parse_l2nic_cap(struct sss_hwdev *hwdev,
				struct sss_service_cap *svc_cap,
				struct sss_cmd_dev_cap_cfg *cmd_cap,
				enum sss_func_type type)
{
	struct sss_nic_service_cap *nic_svc_cap = &svc_cap->nic_cap;

	if (!SSS_IS_NIC_TYPE(hwdev))
		return;

	nic_svc_cap->max_rq = cmd_cap->nic_max_rq_id + 1;
	nic_svc_cap->max_sq = cmd_cap->nic_max_sq_id + 1;
	nic_svc_cap->def_queue_num = cmd_cap->nic_def_queue_num;

	sdk_info(hwdev->dev_hdl,
		 "Get Nic capbility, max_sq: 0x%x, max_rq: 0x%x, def_queue_num: 0x%x\n",
		 nic_svc_cap->max_sq, nic_svc_cap->max_rq, nic_svc_cap->def_queue_num);

	/* Check parameters from firmware */
	if (nic_svc_cap->max_sq > SSS_CFG_MAX_QP ||
	    nic_svc_cap->max_rq > SSS_CFG_MAX_QP) {
		sdk_info(hwdev->dev_hdl, "Exceed limit[1-%d]:sq: %u, rq: %u\n",
			 SSS_CFG_MAX_QP, nic_svc_cap->max_sq, nic_svc_cap->max_rq);
		nic_svc_cap->max_rq = SSS_CFG_MAX_QP;
		nic_svc_cap->max_sq = SSS_CFG_MAX_QP;
	}
}

static void sss_parse_fc_cap(struct sss_hwdev *hwdev,
			     struct sss_service_cap *svc_cap,
			     struct sss_cmd_dev_cap_cfg *cmd_cap,
			     enum sss_func_type type)
{
	struct sss_fc_service_cap *fc_svc_cap = &svc_cap->fc_cap;
	struct sss_dev_fc_svc_cap *dev_fc_cap = &fc_svc_cap->dev_fc_cap;

	if (!SSS_IS_FC_TYPE(hwdev))
		return;

	/* FC without virtulization */
	if (type != SSS_FUNC_TYPE_PF && type != SSS_FUNC_TYPE_PPF)
		return;

	dev_fc_cap->srq_num = cmd_cap->fc_max_srq;
	dev_fc_cap->scq_num = cmd_cap->fc_max_scq;
	dev_fc_cap->max_parent_qpc_num = cmd_cap->fc_max_pctx;
	dev_fc_cap->max_child_qpc_num = cmd_cap->fc_max_cctx;
	dev_fc_cap->child_qpc_id_start = cmd_cap->fc_cctx_id_start;
	dev_fc_cap->vp_id_start = cmd_cap->fc_vp_id_start;
	dev_fc_cap->vp_id_end = cmd_cap->fc_vp_id_end;

	fc_svc_cap->parent_qpc_size = SSS_FC_PQPC_SIZE;
	fc_svc_cap->child_qpc_size = SSS_FC_CQPC_SIZE;
	fc_svc_cap->sqe_size = SSS_FC_SQE_SIZE;

	fc_svc_cap->scqc_size = SSS_FC_SCQC_SIZE;
	fc_svc_cap->scqe_size = SSS_FC_SCQE_SIZE;

	fc_svc_cap->srqc_size = SSS_FC_SRQC_SIZE;
	fc_svc_cap->srqe_size = SSS_FC_SRQE_SIZE;

	sdk_info(hwdev->dev_hdl, "Get FC capbility, type: 0x%x\n", type);
	sdk_info(hwdev->dev_hdl,
		 "max_parent_qpc_num: 0x%x, max_child_qpc_num: 0x%x, scq_num: 0x%x, srq_num: 0x%x\n",
		 dev_fc_cap->max_parent_qpc_num, dev_fc_cap->max_child_qpc_num,
		 dev_fc_cap->scq_num, dev_fc_cap->srq_num);
	sdk_info(hwdev->dev_hdl, "child_qpc_id_start: 0x%x, vp_id_start: 0x%x, vp_id_end: 0x%x\n",
		 dev_fc_cap->child_qpc_id_start, dev_fc_cap->vp_id_start, dev_fc_cap->vp_id_end);
}

static void sss_init_rdma_cap_param(struct sss_hwdev *hwdev)
{
	struct sss_rdma_service_cap *rdma_svc_cap = &hwdev->mgmt_info->svc_cap.rdma_cap;
	struct sss_dev_roce_svc_own_cap *roce_own_cap =
			&rdma_svc_cap->dev_rdma_cap.roce_own_cap;

	rdma_svc_cap->log_mtt = SSS_LOG_MTT_SEG;
	rdma_svc_cap->log_rdmarc = SSS_LOG_RDMARC_SEG;
	rdma_svc_cap->reserved_qp = SSS_RDMA_RSVD_QP;
	rdma_svc_cap->max_sq_sg = SSS_RDMA_MAX_SQ_SGE;

	/* RoCE */
	roce_own_cap->qpc_entry_size = SSS_ROCE_QPC_ENTRY_SIZE;
	roce_own_cap->max_wqe = SSS_ROCE_MAX_WQE;
	roce_own_cap->max_rq_sg = SSS_ROCE_MAX_RQ_SGE;
	roce_own_cap->max_sq_inline_data_size = SSS_ROCE_MAX_SQ_INLINE_DATA_SIZE;
	roce_own_cap->max_rq_desc_size = SSS_ROCE_MAX_RQ_DESC_SIZE;
	roce_own_cap->rdmarc_entry_size = SSS_ROCE_RDMARC_ENTRY_SIZE;
	roce_own_cap->max_qp_init_rdma = SSS_ROCE_MAX_QP_INIT_RDMA;
	roce_own_cap->max_qp_dest_rdma = SSS_ROCE_MAX_QP_DEST_RDMA;
	roce_own_cap->max_srq_wqe = SSS_ROCE_MAX_SRQ_WQE;
	roce_own_cap->reserved_srq = SSS_ROCE_RSVD_SRQ;
	roce_own_cap->max_srq_sge = SSS_ROCE_MAX_SRQ_SGE;
	roce_own_cap->srqc_entry_size = ROCE_SRQC_ENTERY_SIZE;
	roce_own_cap->max_msg_size = SSS_ROCE_MAX_MSG_SIZE;

	rdma_svc_cap->max_sq_desc_size = SSS_RDMA_MAX_SQ_DESC_SIZE;
	rdma_svc_cap->wqebb_size = SSS_WQEBB_SIZE;
	rdma_svc_cap->max_cqe = SSS_RDMA_MAX_CQE;
	rdma_svc_cap->reserved_cq = SSS_RDMA_RSVD_CQ;
	rdma_svc_cap->cqc_entry_size = SSS_RDMA_CQC_ENTRY_SIZE;
	rdma_svc_cap->cqe_size = SSS_RDMA_CQE_SIZE;
	rdma_svc_cap->reserved_mrw = SSS_RDMA_RSVD_MRW;
	rdma_svc_cap->mpt_entry_size = SSS_RDMA_MPT_ENTRY_SIZE;

	rdma_svc_cap->max_fmr_map = 0xff;
	rdma_svc_cap->mtt_num = SSS_RDMA_MTT_NUM;
	rdma_svc_cap->log_mtt_seg = SSS_LOG_MTT_SEG;
	rdma_svc_cap->mtt_entry_size = SSS_MTT_ENTRY_SIZE;
	rdma_svc_cap->log_rdmarc_seg = SSS_LOG_RDMARC_SEG;
	rdma_svc_cap->local_ca_ack_delay = SSS_LOCAL_ACK_DELAY;
	rdma_svc_cap->port_num = SSS_RDMA_PORT_NUM;
	rdma_svc_cap->db_page_size = SSS_DB_PAGE_SIZE_K;
	rdma_svc_cap->direct_wqe_size = SSS_DWQE_SIZE;
	rdma_svc_cap->pd_num = SSS_PD_NUM;
	rdma_svc_cap->reserved_pd = SSS_RSVD_PD;
	rdma_svc_cap->max_xrcd = SSS_MAX_XRCD;
	rdma_svc_cap->reserved_xrcd = SSS_RSVD_XRCD;
	rdma_svc_cap->max_gid_per_port = SSS_MAX_GID_PER_PORT;
	rdma_svc_cap->gid_entry_size = SSS_GID_ENTRY_SIZE;
	rdma_svc_cap->reserved_lkey = SSS_RSVD_LKEY;
	rdma_svc_cap->comp_vector_num = (u32)hwdev->mgmt_info->eq_info.ceq_num;
	rdma_svc_cap->page_size_cap = SSS_PAGE_SIZE_CAP;
	rdma_svc_cap->flag = (SSS_RDMA_BMME_FLAG_LOCAL_INV |
			      SSS_RDMA_BMME_FLAG_REMOTE_INV |
			      SSS_RDMA_BMME_FLAG_FAST_REG_WR |
			      SSS_RDMA_DEV_CAP_FLAG_XRC |
			      SSS_RDMA_DEV_CAP_FLAG_MEM_WINDOW |
			      SSS_RDMA_BMME_FLAG_TYPE_2_WIN |
			      SSS_RDMA_BMME_FLAG_WIN_TYPE_2B |
			      SSS_RDMA_DEV_CAP_FLAG_ATOMIC);
	rdma_svc_cap->max_frpl_len = SSS_MAX_FRPL_LEN;
	rdma_svc_cap->max_pkey = SSS_MAX_PKEY;
}

static void sss_parse_roce_cap(struct sss_hwdev *hwdev,
			       struct sss_service_cap *svc_cap,
			       struct sss_cmd_dev_cap_cfg *cmd_cap,
			       enum sss_func_type type)
{
	struct sss_dev_roce_svc_own_cap *roce_own_cap =
			&svc_cap->rdma_cap.dev_rdma_cap.roce_own_cap;

	if (!SSS_IS_ROCE_TYPE(hwdev))
		return;

	roce_own_cap->max_srq = cmd_cap->roce_max_srq;
	roce_own_cap->max_cq = cmd_cap->roce_max_cq;
	roce_own_cap->max_qp = cmd_cap->roce_max_qp;
	roce_own_cap->max_mpt = cmd_cap->roce_max_mpt;
	roce_own_cap->max_drc_qp = cmd_cap->roce_max_drc_qp;

	roce_own_cap->wqe_cl_size = cmd_cap->roce_wqe_cl_size;
	roce_own_cap->wqe_cl_start = cmd_cap->roce_wqe_cl_start;
	roce_own_cap->wqe_cl_end = cmd_cap->roce_wqe_cl_end;

	if (roce_own_cap->max_qp == 0) {
		roce_own_cap->max_drc_qp = SSS_ROCE_MAX_DRC_QP;
		if (type == SSS_FUNC_TYPE_PF || type == SSS_FUNC_TYPE_PPF) {
			roce_own_cap->max_srq = SSS_ROCE_MAX_SRQ;
			roce_own_cap->max_cq = SSS_ROCE_MAX_CQ;
			roce_own_cap->max_qp = SSS_ROCE_MAX_QP;
			roce_own_cap->max_mpt = SSS_ROCE_MAX_MPT;
		} else {
			roce_own_cap->max_srq = SSS_ROCE_MAX_SRQ / 2;
			roce_own_cap->max_cq = SSS_ROCE_MAX_CQ / 2;
			roce_own_cap->max_qp = SSS_ROCE_MAX_QP / 2;
			roce_own_cap->max_mpt = SSS_ROCE_MAX_MPT / 2;
		}
	}

	sss_init_rdma_cap_param(hwdev);

	sdk_info(hwdev->dev_hdl, "Get ROCE capbility, type: 0x%x\n", type);
	sdk_info(hwdev->dev_hdl,
		 "max_qps: 0x%x, max_srq: 0x%x, max_cq: 0x%x, max_mpt: 0x%x, max_drct: 0x%x\n",
		 roce_own_cap->max_qp, roce_own_cap->max_srq, roce_own_cap->max_cq,
		 roce_own_cap->max_mpt, roce_own_cap->max_drc_qp);
	sdk_info(hwdev->dev_hdl, "wqe_start: 0x%x, wqe_end: 0x%x, wqe_sz: 0x%x\n",
		 roce_own_cap->wqe_cl_start, roce_own_cap->wqe_cl_end, roce_own_cap->wqe_cl_size);
}

static void sss_parse_rdma_cap(struct sss_hwdev *hwdev,
			       struct sss_service_cap *svc_cap,
			       struct sss_cmd_dev_cap_cfg *cmd_cap,
			       enum sss_func_type type)
{
	struct sss_rdma_service_cap *rdma_svc_cap = &svc_cap->rdma_cap;
	struct sss_dev_roce_svc_own_cap *roce_own_cap =
			&rdma_svc_cap->dev_rdma_cap.roce_own_cap;

	if (!SSS_IS_RDMA_ENABLE(hwdev))
		return;

	roce_own_cap->dmtt_cl_start = cmd_cap->roce_dmtt_cl_start;
	roce_own_cap->dmtt_cl_end = cmd_cap->roce_dmtt_cl_end;
	roce_own_cap->dmtt_cl_size = cmd_cap->roce_dmtt_cl_size;

	roce_own_cap->cmtt_cl_start = cmd_cap->roce_cmtt_cl_start;
	roce_own_cap->cmtt_cl_end = cmd_cap->roce_cmtt_cl_end;
	roce_own_cap->cmtt_cl_size = cmd_cap->roce_cmtt_cl_size;

	rdma_svc_cap->log_mtt = SSS_LOG_MTT_SEG;
	rdma_svc_cap->log_mtt_seg = SSS_LOG_MTT_SEG;
	rdma_svc_cap->mtt_entry_size = SSS_MTT_ENTRY_SIZE;
	rdma_svc_cap->mpt_entry_size = SSS_RDMA_MPT_ENTRY_SIZE;
	rdma_svc_cap->mtt_num = SSS_RDMA_MTT_NUM;

	sdk_info(hwdev->dev_hdl, "Get RDMA capbility, type: 0x%x\n", type);
	sdk_info(hwdev->dev_hdl, "cmtt_cl_start: 0x%x, cmtt_cl_end: 0x%x, cmtt_cl_size: 0x%x\n",
		 roce_own_cap->cmtt_cl_start, roce_own_cap->cmtt_cl_end,
		 roce_own_cap->cmtt_cl_size);
	sdk_info(hwdev->dev_hdl, "dmtt_cl_start: 0x%x, dmtt_cl_end: 0x%x, dmtt_cl_size: 0x%x\n",
		 roce_own_cap->dmtt_cl_start, roce_own_cap->dmtt_cl_end,
		 roce_own_cap->dmtt_cl_size);
}

static void sss_parse_ovs_cap(struct sss_hwdev *hwdev,
			      struct sss_service_cap *svc_cap,
			      struct sss_cmd_dev_cap_cfg *cmd_cap,
			      enum sss_func_type type)
{
	struct sss_ovs_service_cap *ovs_cap = &svc_cap->ovs_cap;
	struct sss_dev_ovs_svc_cap *dev_ovs_cap = &ovs_cap->dev_ovs_cap;

	if (!SSS_IS_OVS_TYPE(hwdev))
		return;

	dev_ovs_cap->max_pctx = cmd_cap->ovs_max_qpc;
	dev_ovs_cap->pseudo_vf_start_id = cmd_cap->pseudo_vf_start_id;
	dev_ovs_cap->pseudo_vf_num = cmd_cap->pseudo_vf_num;
	dev_ovs_cap->pseudo_vf_max_pctx = cmd_cap->pseudo_vf_max_pctx;
	dev_ovs_cap->dynamic_qp_en = cmd_cap->flexq_en;
	ovs_cap->pctx_size = SSS_OVS_PCTX_SIZE;

	sdk_info(hwdev->dev_hdl, "Get OVS capbility, type: 0x%x\n", type);
	sdk_info(hwdev->dev_hdl, "max_pctxs: 0x%x, pseudo_vf_start_id: 0x%x, pseudo_vf_num: 0x%x\n",
		 dev_ovs_cap->max_pctx, dev_ovs_cap->pseudo_vf_start_id,
		 dev_ovs_cap->pseudo_vf_num);
	sdk_info(hwdev->dev_hdl, "pseudo_vf_max_pctx: 0x%x, dynamic_qp_en: 0x%x\n",
		 dev_ovs_cap->pseudo_vf_max_pctx, dev_ovs_cap->dynamic_qp_en);
}

static void sss_parse_ppa_cap(struct sss_hwdev *hwdev,
			      struct sss_service_cap *svc_cap,
			      struct sss_cmd_dev_cap_cfg *cmd_cap,
			      enum sss_func_type type)
{
	struct sss_ppa_service_cap *ppa_cap = &svc_cap->ppa_cap;

	if (!SSS_IS_PPA_TYPE(hwdev))
		return;

	ppa_cap->qpc_pseudo_vf_start = cmd_cap->pseudo_vf_start_id;
	ppa_cap->qpc_pseudo_vf_num = cmd_cap->pseudo_vf_num;
	ppa_cap->qpc_pseudo_vf_ctx_num = cmd_cap->pseudo_vf_max_pctx;
	ppa_cap->bloomfilter_len = cmd_cap->pseudo_vf_bfilter_len;
	ppa_cap->bloomfilter_en = !!cmd_cap->pseudo_vf_bfilter_len;
	ppa_cap->pctx_size = SSS_PPA_PCTX_SIZE;

	sdk_info(hwdev->dev_hdl, "Get PPA capbility, type: 0x%x\n", type);
	sdk_info(hwdev->dev_hdl,
		 "qpc_pseudo_vf_start: 0x%x, qpc_pseudo_vf_num: 0x%x, qpc_pseudo_vf_ctx_num: 0x%x\n",
		 ppa_cap->qpc_pseudo_vf_start, ppa_cap->qpc_pseudo_vf_num,
		 ppa_cap->qpc_pseudo_vf_ctx_num);
}

static void sss_parse_toe_cap(struct sss_hwdev *hwdev,
			      struct sss_service_cap *svc_cap,
			      struct sss_cmd_dev_cap_cfg *cmd_cap,
			      enum sss_func_type type)
{
	struct sss_toe_service_cap *toe_svc_cap = &svc_cap->toe_cap;
	struct sss_dev_toe_svc_cap *dev_toe_cap = &toe_svc_cap->dev_toe_cap;

	if (!SSS_IS_TOE_TYPE(hwdev))
		return;

	dev_toe_cap->max_srq = cmd_cap->toe_max_srq;
	dev_toe_cap->max_cq = cmd_cap->toe_max_cq;
	dev_toe_cap->srq_id_start = cmd_cap->toe_srq_id_start;
	dev_toe_cap->max_pctx = cmd_cap->toe_max_pctx;
	dev_toe_cap->max_cctxt = cmd_cap->toe_max_cctxt;
	dev_toe_cap->max_mpt = cmd_cap->toe_max_mpt;

	toe_svc_cap->pctx_size = SSS_TOE_PCTX_SIZE;
	toe_svc_cap->scqc_size = SSS_TOE_SCQC_SIZE;

	sdk_info(hwdev->dev_hdl, "Get TOE capbility, type: 0x%x\n", type);
	sdk_info(hwdev->dev_hdl,
		 "max_pctx: 0x%x, max_cq: 0x%x, max_srq: 0x%x, srq_id_start: 0x%x, max_mpt: 0x%x\n",
		 dev_toe_cap->max_pctx, dev_toe_cap->max_cq, dev_toe_cap->max_srq,
		 dev_toe_cap->srq_id_start, dev_toe_cap->max_mpt);
}

static void sss_parse_ipsec_cap(struct sss_hwdev *hwdev,
				struct sss_service_cap *svc_cap,
				struct sss_cmd_dev_cap_cfg *cmd_cap,
				enum sss_func_type type)
{
	struct sss_ipsec_service_cap *ipsec_cap = &svc_cap->ipsec_cap;
	struct sss_dev_ipsec_svc_cap *dev_ipsec_cap = &ipsec_cap->dev_ipsec_cap;

	if (!SSS_IS_IPSEC_TYPE(hwdev))
		return;

	dev_ipsec_cap->max_sactx = cmd_cap->ipsec_max_sactx;
	dev_ipsec_cap->max_cq = cmd_cap->ipsec_max_cq;
	ipsec_cap->sactx_size = SSS_IPSEC_SACTX_SIZE;

	sdk_info(hwdev->dev_hdl, "Get IPSEC capbility, type: 0x%x\n", type);
	sdk_info(hwdev->dev_hdl, "max_sactx: 0x%x, max_cq: 0x%x\n",
		 dev_ipsec_cap->max_sactx, dev_ipsec_cap->max_cq);
}

static void sss_parse_vbs_cap(struct sss_hwdev *hwdev,
			      struct sss_service_cap *svc_cap,
			      struct sss_cmd_dev_cap_cfg *cmd_cap,
			      enum sss_func_type type)
{
	struct sss_vbs_service_cap *vbs_cap = &svc_cap->vbs_cap;

	if (!SSS_IS_VBS_TYPE(hwdev))
		return;

	vbs_cap->vbs_max_volq = cmd_cap->vbs_max_volq;

	sdk_info(hwdev->dev_hdl, "Get VBS capbility, type: 0x%x, vbs_max_volq: 0x%x\n",
		 type, vbs_cap->vbs_max_volq);
}

static void sss_parse_dev_cap(struct sss_hwdev *hwdev,
			      struct sss_cmd_dev_cap_cfg *cmd_cap, enum sss_func_type type)
{
	struct sss_service_cap *svc_cap = &hwdev->mgmt_info->svc_cap;

	sss_parse_pubic_cap(hwdev, svc_cap, cmd_cap, type);
	sss_parse_l2nic_cap(hwdev, svc_cap, cmd_cap, type);
	sss_parse_fc_cap(hwdev, svc_cap, cmd_cap, type);
	sss_parse_toe_cap(hwdev, svc_cap, cmd_cap, type);
	sss_parse_rdma_cap(hwdev, svc_cap, cmd_cap, type);
	sss_parse_roce_cap(hwdev, svc_cap, cmd_cap, type);
	sss_parse_ovs_cap(hwdev, svc_cap, cmd_cap, type);
	sss_parse_ipsec_cap(hwdev, svc_cap, cmd_cap, type);
	sss_parse_ppa_cap(hwdev, svc_cap, cmd_cap, type);
	sss_parse_vbs_cap(hwdev, svc_cap, cmd_cap, type);
}

static int sss_chip_get_cap(struct sss_hwdev *hwdev, struct sss_cmd_dev_cap_cfg *cmd_cap)
{
	int ret;
	u16 out_len = sizeof(*cmd_cap);

	cmd_cap->func_id = sss_get_global_func_id(hwdev);
	sdk_info(hwdev->dev_hdl, "Get svc_cap, func_id: %u\n", cmd_cap->func_id);

	ret = sss_sync_mbx_send_msg(hwdev, SSS_MOD_TYPE_CFGM, SSS_CFG_CMD_GET_CAP_CFG,
				    cmd_cap, sizeof(*cmd_cap), cmd_cap, &out_len, 0,
				    SSS_CHANNEL_COMM);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, cmd_cap)) {
		sdk_err(hwdev->dev_hdl,
			"Fail to get capability, err: %d, status: 0x%x, out_len: 0x%x\n",
			ret, cmd_cap->head.state, out_len);
		return -EIO;
	}

	return 0;
}

int sss_init_capability(struct sss_hwdev *hwdev)
{
	int ret;
	enum sss_func_type type = SSS_GET_FUNC_TYPE(hwdev);
	struct sss_cmd_dev_cap_cfg cmd_cap = {0};

	if (type != SSS_FUNC_TYPE_PF &&
	    type != SSS_FUNC_TYPE_VF &&
	    type != SSS_FUNC_TYPE_PPF) {
		sdk_err(hwdev->dev_hdl, "Unsupported PCI Function type: %d\n", type);
		return -EINVAL;
	}

	ret = sss_chip_get_cap(hwdev, &cmd_cap);
	if (ret != 0)
		return ret;

	sss_parse_dev_cap(hwdev, &cmd_cap, type);

	sdk_info(hwdev->dev_hdl, "Success to init capability\n");
	return 0;
}

void sss_deinit_capability(struct sss_hwdev *hwdev)
{
	sdk_info(hwdev->dev_hdl, "Success to deinit capability");
}

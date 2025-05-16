// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [COMM]" fmt

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/semaphore.h>

#include "ossl_knl.h"
#include "hinic3_crm.h"
#include "hinic3_hw.h"
#include "hinic3_hwdev.h"
#include "hinic3_hwif.h"
#include "cfg_mgmt_mpu_cmd.h"
#include "cfg_mgmt_mpu_cmd_defs.h"
#include "hinic3_hw_cfg.h"

static void parse_pub_res_cap_dfx(struct hinic3_hwdev *hwdev,
				  const struct service_cap *cap)
{
	sdk_info(hwdev->dev_hdl, "Get public resource capbility: svc_cap_en: 0x%x\n",
		 cap->svc_type);
	sdk_info(hwdev->dev_hdl, "Host_id: 0x%x, ep_id: 0x%x, er_id: 0x%x, port_id: 0x%x\n",
		 cap->host_id, cap->ep_id, cap->er_id, cap->port_id);
	sdk_info(hwdev->dev_hdl, "cos_bitmap: 0x%x, flexq: 0x%x, virtio_vq_size: 0x%x\n",
		 cap->cos_valid_bitmap, cap->flexq_en, cap->virtio_vq_size);
	sdk_info(hwdev->dev_hdl, "Host_total_function: 0x%x, host_oq_id_mask_val: 0x%x, max_vf: 0x%x\n",
		 cap->host_total_function, cap->host_oq_id_mask_val,
		 cap->max_vf);
	sdk_info(hwdev->dev_hdl, "Host_pf_num: 0x%x, pf_id_start: 0x%x, host_vf_num: 0x%x, vf_id_start: 0x%x\n",
		 cap->pf_num, cap->pf_id_start, cap->vf_num, cap->vf_id_start);
	sdk_info(hwdev->dev_hdl,
		 "host_valid_bitmap: 0x%x, master_host_id: 0x%x, srv_multi_host_mode: 0x%x, hot_plug_disable: 0x%x\n",
		 cap->host_valid_bitmap, cap->master_host_id,
		 cap->srv_multi_host_mode, cap->hot_plug_disable);
	sdk_info(hwdev->dev_hdl,
		 "os_hot_replace: 0x%x, fake_vf_start_id: 0x%x, fake_vf_num: 0x%x, fake_vf_max_pctx: 0x%x\n",
		 cap->os_hot_replace, cap->fake_vf_start_id,
		 cap->fake_vf_num, cap->fake_vf_max_pctx);
	sdk_info(hwdev->dev_hdl,
		 "fake_vf_bfilter_start_addr: 0x%x, fake_vf_bfilter_len: 0x%x, bond_create_mode: 0x%x\n",
		 cap->fake_vf_bfilter_start_addr, cap->fake_vf_bfilter_len,
		 cap->bond_create_mode);
}

static void parse_cqm_res_cap(struct hinic3_hwdev *hwdev, struct service_cap *cap,
			      struct cfg_cmd_dev_cap *dev_cap)
{
	struct dev_sf_svc_attr *attr = &cap->sf_svc_attr;

	cap->fake_vf_start_id = dev_cap->fake_vf_start_id;
	cap->fake_vf_num = dev_cap->fake_vf_num;
	cap->fake_vf_max_pctx = dev_cap->fake_vf_max_pctx;
	cap->fake_vf_num_cfg = dev_cap->fake_vf_num;
	cap->fake_vf_bfilter_start_addr = dev_cap->fake_vf_bfilter_start_addr;
	cap->fake_vf_bfilter_len = dev_cap->fake_vf_bfilter_len;

	if (COMM_SUPPORT_VIRTIO_VQ_SIZE(hwdev))
		cap->virtio_vq_size = (u16)(VIRTIO_BASE_VQ_SIZE << dev_cap->virtio_vq_size);
	else
		cap->virtio_vq_size = VIRTIO_DEFAULT_VQ_SIZE;

	if (dev_cap->sf_svc_attr & SF_SVC_FT_BIT)
		attr->ft_en = true;
	else
		attr->ft_en = false;

	if (dev_cap->sf_svc_attr & SF_SVC_RDMA_BIT)
		attr->rdma_en = true;
	else
		attr->rdma_en = false;

	/* PPF will overwrite it when parse dynamic resource */
	if (dev_cap->func_sf_en)
		cap->sf_en = true;
	else
		cap->sf_en = false;

	cap->lb_mode = dev_cap->lb_mode;
	cap->smf_pg = dev_cap->smf_pg;

	cap->timer_en = dev_cap->timer_en;
	cap->host_oq_id_mask_val = dev_cap->host_oq_id_mask_val;
	cap->max_connect_num = dev_cap->max_conn_num;
	cap->max_stick2cache_num = dev_cap->max_stick2cache_num;
	cap->bfilter_start_addr = dev_cap->max_bfilter_start_addr;
	cap->bfilter_len = dev_cap->bfilter_len;
	cap->hash_bucket_num = dev_cap->hash_bucket_num;
}

static void parse_pub_res_cap(struct hinic3_hwdev *hwdev,
			      struct service_cap *cap,
			      struct cfg_cmd_dev_cap *dev_cap,
			      enum func_type type)
{
	cap->host_id = dev_cap->host_id;
	cap->ep_id = dev_cap->ep_id;
	cap->er_id = dev_cap->er_id;
	cap->port_id = dev_cap->port_id;

	cap->svc_type = dev_cap->svc_cap_en;
	cap->chip_svc_type = cap->svc_type;

	cap->cos_valid_bitmap = dev_cap->valid_cos_bitmap;
	cap->port_cos_valid_bitmap = dev_cap->port_cos_valid_bitmap;
	cap->flexq_en = dev_cap->flexq_en;

	cap->host_total_function = dev_cap->host_total_func;
	cap->host_valid_bitmap = dev_cap->host_valid_bitmap;
	cap->master_host_id = dev_cap->master_host_id;
	cap->srv_multi_host_mode = dev_cap->srv_multi_host_mode;
	cap->hot_plug_disable = dev_cap->hot_plug_disable;
	cap->bond_create_mode = dev_cap->bond_create_mode;
	cap->os_hot_replace = dev_cap->os_hot_replace;
	cap->fake_vf_en = dev_cap->fake_vf_en;
	cap->fake_vf_start_bit = dev_cap->fake_vf_start_bit;
	cap->fake_vf_end_bit = dev_cap->fake_vf_end_bit;
	cap->fake_vf_page_bit = dev_cap->fake_vf_page_bit;
	cap->map_host_id = dev_cap->map_host_id;

	if (type != TYPE_VF) {
		cap->max_vf = dev_cap->max_vf;
		cap->pf_num = dev_cap->host_pf_num;
		cap->pf_id_start = dev_cap->pf_id_start;
		cap->vf_num = dev_cap->host_vf_num;
		cap->vf_id_start = dev_cap->vf_id_start;
	} else {
		cap->max_vf = 0;
	}

	parse_cqm_res_cap(hwdev, cap, dev_cap);
	parse_pub_res_cap_dfx(hwdev, cap);
}

static void parse_dynamic_share_res_cap(struct service_cap *cap,
					const struct cfg_cmd_dev_cap *dev_cap)
{
	if (dev_cap->host_sf_en)
		cap->sf_en = true;
	else
		cap->sf_en = false;
}

static void parse_l2nic_res_cap(struct hinic3_hwdev *hwdev,
				struct service_cap *cap,
				struct cfg_cmd_dev_cap *dev_cap,
				enum func_type type)
{
	struct nic_service_cap *nic_cap = &cap->nic_cap;

	nic_cap->max_sqs = dev_cap->nic_max_sq_id + 1;
	nic_cap->max_rqs = dev_cap->nic_max_rq_id + 1;
	nic_cap->default_num_queues = dev_cap->nic_default_num_queues;
	nic_cap->outband_vlan_cfg_en = dev_cap->outband_vlan_cfg_en;
	nic_cap->lro_enable = dev_cap->lro_enable;

	sdk_info(hwdev->dev_hdl, "L2nic resource capbility, max_sqs: 0x%x, max_rqs: 0x%x\n",
		 nic_cap->max_sqs, nic_cap->max_rqs);

	/* Check parameters from firmware */
	if (nic_cap->max_sqs > HINIC3_CFG_MAX_QP) {
		sdk_info(hwdev->dev_hdl, "Number of sq exceed limit[1-%d]: sq: %u\n",
			 HINIC3_CFG_MAX_QP, nic_cap->max_sqs);
		nic_cap->max_sqs = HINIC3_CFG_MAX_QP;
	}

	if (nic_cap->max_rqs > HINIC3_CFG_MAX_QP) {
		sdk_info(hwdev->dev_hdl, "Number of rq exceed limit[1-%d]: rq: %u\n",
			 HINIC3_CFG_MAX_QP, nic_cap->max_rqs);
		nic_cap->max_rqs = HINIC3_CFG_MAX_QP;
	}

	if (nic_cap->outband_vlan_cfg_en)
		sdk_info(hwdev->dev_hdl, "L2nic outband vlan cfg enabled\n");
}

static void parse_fc_res_cap(struct hinic3_hwdev *hwdev,
			     struct service_cap *cap,
			     struct cfg_cmd_dev_cap *dev_cap,
			     enum func_type type)
{
	struct dev_fc_svc_cap *fc_cap = &cap->fc_cap.dev_fc_cap;

	fc_cap->max_parent_qpc_num = dev_cap->fc_max_pctx;
	fc_cap->scq_num = dev_cap->fc_max_scq;
	fc_cap->srq_num = dev_cap->fc_max_srq;
	fc_cap->max_child_qpc_num = dev_cap->fc_max_cctx;
	fc_cap->child_qpc_id_start = dev_cap->fc_cctx_id_start;
	fc_cap->vp_id_start = dev_cap->fc_vp_id_start;
	fc_cap->vp_id_end = dev_cap->fc_vp_id_end;

	sdk_info(hwdev->dev_hdl, "Get fc resource capbility\n");
	sdk_info(hwdev->dev_hdl,
		 "Max_parent_qpc_num: 0x%x, scq_num: 0x%x, srq_num: 0x%x, max_child_qpc_num: 0x%x, child_qpc_id_start: 0x%x\n",
		 fc_cap->max_parent_qpc_num, fc_cap->scq_num, fc_cap->srq_num,
		 fc_cap->max_child_qpc_num, fc_cap->child_qpc_id_start);
	sdk_info(hwdev->dev_hdl, "Vp_id_start: 0x%x, vp_id_end: 0x%x\n",
		 fc_cap->vp_id_start, fc_cap->vp_id_end);
}

static void parse_roce_res_cap(struct hinic3_hwdev *hwdev,
			       struct service_cap *cap,
			       struct cfg_cmd_dev_cap *dev_cap,
			       enum func_type type)
{
	struct dev_roce_svc_own_cap *roce_cap =
		&cap->rdma_cap.dev_rdma_cap.roce_own_cap;

	roce_cap->max_qps = dev_cap->roce_max_qp;
	roce_cap->max_cqs = dev_cap->roce_max_cq;
	roce_cap->max_srqs = dev_cap->roce_max_srq;
	roce_cap->max_mpts = dev_cap->roce_max_mpt;
	roce_cap->max_drc_qps = dev_cap->roce_max_drc_qp;

	roce_cap->wqe_cl_start = dev_cap->roce_wqe_cl_start;
	roce_cap->wqe_cl_end = dev_cap->roce_wqe_cl_end;
	roce_cap->wqe_cl_sz = dev_cap->roce_wqe_cl_size;

	sdk_info(hwdev->dev_hdl, "Get roce resource capbility, type: 0x%x\n",
		 type);
	sdk_info(hwdev->dev_hdl, "Max_qps: 0x%x, max_cqs: 0x%x, max_srqs: 0x%x, max_mpts: 0x%x, max_drcts: 0x%x\n",
		 roce_cap->max_qps, roce_cap->max_cqs, roce_cap->max_srqs,
		 roce_cap->max_mpts, roce_cap->max_drc_qps);

	sdk_info(hwdev->dev_hdl, "Wqe_start: 0x%x, wqe_end: 0x%x, wqe_sz: 0x%x\n",
		 roce_cap->wqe_cl_start, roce_cap->wqe_cl_end,
		 roce_cap->wqe_cl_sz);

	if (roce_cap->max_qps == 0) {
		if (type == TYPE_PF || type == TYPE_PPF) {
			roce_cap->max_qps = 0x400;
			roce_cap->max_cqs = 0x800;
			roce_cap->max_srqs = 0x400;
			roce_cap->max_mpts = 0x400;
			roce_cap->max_drc_qps = 0x40;
		} else {
			roce_cap->max_qps = 0x200;
			roce_cap->max_cqs = 0x400;
			roce_cap->max_srqs = 0x200;
			roce_cap->max_mpts = 0x200;
			roce_cap->max_drc_qps = 0x40;
		}
	}
}

static void parse_rdma_res_cap(struct hinic3_hwdev *hwdev,
			       struct service_cap *cap,
			       struct cfg_cmd_dev_cap *dev_cap,
			       enum func_type type)
{
	struct dev_roce_svc_own_cap *roce_cap =
		&cap->rdma_cap.dev_rdma_cap.roce_own_cap;

	roce_cap->cmtt_cl_start = dev_cap->roce_cmtt_cl_start;
	roce_cap->cmtt_cl_end = dev_cap->roce_cmtt_cl_end;
	roce_cap->cmtt_cl_sz = dev_cap->roce_cmtt_cl_size;

	roce_cap->dmtt_cl_start = dev_cap->roce_dmtt_cl_start;
	roce_cap->dmtt_cl_end = dev_cap->roce_dmtt_cl_end;
	roce_cap->dmtt_cl_sz = dev_cap->roce_dmtt_cl_size;

	sdk_info(hwdev->dev_hdl, "Get rdma resource capbility, Cmtt_start: 0x%x, cmtt_end: 0x%x, cmtt_sz: 0x%x\n",
		 roce_cap->cmtt_cl_start, roce_cap->cmtt_cl_end,
		 roce_cap->cmtt_cl_sz);

	sdk_info(hwdev->dev_hdl, "Dmtt_start: 0x%x, dmtt_end: 0x%x, dmtt_sz: 0x%x\n",
		 roce_cap->dmtt_cl_start, roce_cap->dmtt_cl_end,
		 roce_cap->dmtt_cl_sz);
}

static void parse_ovs_res_cap(struct hinic3_hwdev *hwdev,
			      struct service_cap *cap,
			      struct cfg_cmd_dev_cap *dev_cap,
			      enum func_type type)
{
	struct ovs_service_cap *ovs_cap = &cap->ovs_cap;

	ovs_cap->dev_ovs_cap.max_pctxs = dev_cap->ovs_max_qpc;
	ovs_cap->dev_ovs_cap.fake_vf_max_pctx = dev_cap->fake_vf_max_pctx;
	ovs_cap->dev_ovs_cap.fake_vf_start_id = dev_cap->fake_vf_start_id;
	ovs_cap->dev_ovs_cap.fake_vf_num = dev_cap->fake_vf_num;
	ovs_cap->dev_ovs_cap.dynamic_qp_en = dev_cap->flexq_en;

	sdk_info(hwdev->dev_hdl,
		 "Get ovs resource capbility, max_qpc: 0x%x, fake_vf_start_id: 0x%x, fake_vf_num: 0x%x\n",
		 ovs_cap->dev_ovs_cap.max_pctxs,
		 ovs_cap->dev_ovs_cap.fake_vf_start_id,
		 ovs_cap->dev_ovs_cap.fake_vf_num);
	sdk_info(hwdev->dev_hdl,
		 "fake_vf_max_qpc: 0x%x, dynamic_qp_en: 0x%x\n",
		 ovs_cap->dev_ovs_cap.fake_vf_max_pctx,
		 ovs_cap->dev_ovs_cap.dynamic_qp_en);
}

static void parse_ppa_res_cap(struct hinic3_hwdev *hwdev,
			      struct service_cap *cap,
			      struct cfg_cmd_dev_cap *dev_cap,
			      enum func_type type)
{
	struct ppa_service_cap *dip_cap = &cap->ppa_cap;

	dip_cap->qpc_fake_vf_ctx_num = dev_cap->fake_vf_max_pctx;
	dip_cap->qpc_fake_vf_start = dev_cap->fake_vf_start_id;
	dip_cap->qpc_fake_vf_num = dev_cap->fake_vf_num;
	dip_cap->bloomfilter_en = dev_cap->fake_vf_bfilter_len ? 1 : 0;
	dip_cap->bloomfilter_length = dev_cap->fake_vf_bfilter_len;
	sdk_info(hwdev->dev_hdl,
		 "Get ppa resource capbility, fake_vf_start_id: 0x%x, fake_vf_num: 0x%x, fake_vf_max_qpc: 0x%x\n",
		 dip_cap->qpc_fake_vf_start,
		 dip_cap->qpc_fake_vf_num,
		 dip_cap->qpc_fake_vf_ctx_num);
}

static void parse_toe_res_cap(struct hinic3_hwdev *hwdev,
			      struct service_cap *cap,
			      struct cfg_cmd_dev_cap *dev_cap,
			      enum func_type type)
{
	struct dev_toe_svc_cap *toe_cap = &cap->toe_cap.dev_toe_cap;

	toe_cap->max_pctxs = dev_cap->toe_max_pctx;
	toe_cap->max_cqs = dev_cap->toe_max_cq;
	toe_cap->max_srqs = dev_cap->toe_max_srq;
	toe_cap->srq_id_start = dev_cap->toe_srq_id_start;
	toe_cap->max_mpts = dev_cap->toe_max_mpt;
	toe_cap->max_cctxt = dev_cap->toe_max_cctxt;

	sdk_info(hwdev->dev_hdl,
		 "Get toe resource capbility, max_pctxs: 0x%x, max_cqs: 0x%x, max_srqs: 0x%x, srq_id_start: 0x%x, max_mpts: 0x%x\n",
		 toe_cap->max_pctxs, toe_cap->max_cqs, toe_cap->max_srqs,
		 toe_cap->srq_id_start, toe_cap->max_mpts);
}

static void parse_ipsec_res_cap(struct hinic3_hwdev *hwdev,
				struct service_cap *cap,
				struct cfg_cmd_dev_cap *dev_cap,
				enum func_type type)
{
	struct ipsec_service_cap *ipsec_cap = &cap->ipsec_cap;

	ipsec_cap->dev_ipsec_cap.max_sactxs = dev_cap->ipsec_max_sactx;
	ipsec_cap->dev_ipsec_cap.max_cqs = dev_cap->ipsec_max_cq;

	sdk_info(hwdev->dev_hdl, "Get IPsec resource capbility, max_sactxs: 0x%x, max_cqs: 0x%x\n",
		 dev_cap->ipsec_max_sactx, dev_cap->ipsec_max_cq);
}

static void parse_vbs_res_cap(struct hinic3_hwdev *hwdev,
			      struct service_cap *cap,
			      struct cfg_cmd_dev_cap *dev_cap,
			      enum func_type type)
{
	struct vbs_service_cap *vbs_cap = &cap->vbs_cap;

	vbs_cap->vbs_max_volq = dev_cap->vbs_max_volq;
	vbs_cap->vbs_main_pf_enable = dev_cap->vbs_main_pf_enable;
	vbs_cap->vbs_vsock_pf_enable = dev_cap->vbs_vsock_pf_enable;
	vbs_cap->vbs_fushion_queue_pf_enable = dev_cap->vbs_fushion_queue_pf_enable;

	sdk_info(hwdev->dev_hdl,
		 "Get VBS resource capbility, vbs_max_volq: 0x%x\n",
		 dev_cap->vbs_max_volq);
	sdk_info(hwdev->dev_hdl,
		 "Get VBS pf info, vbs_main_pf_enable: 0x%x, vbs_vsock_pf_enable: 0x%x, vbs_fushion_queue_pf_enable: 0x%x\n",
		 dev_cap->vbs_main_pf_enable,
		 dev_cap->vbs_vsock_pf_enable,
		 dev_cap->vbs_fushion_queue_pf_enable);
}

static void parse_dev_cap(struct hinic3_hwdev *dev,
			  struct cfg_cmd_dev_cap *dev_cap, enum func_type type)
{
	struct service_cap *cap = &dev->cfg_mgmt->svc_cap;

	/* Public resource */
	parse_pub_res_cap(dev, cap, dev_cap, type);

	/* PPF managed dynamic resource */
	if (type == TYPE_PPF)
		parse_dynamic_share_res_cap(cap, dev_cap);

	/* L2 NIC resource */
	if (IS_NIC_TYPE(dev))
		parse_l2nic_res_cap(dev, cap, dev_cap, type);

	/* FC without virtulization */
	if (type == TYPE_PF || type == TYPE_PPF) {
		if (IS_FC_TYPE(dev))
			parse_fc_res_cap(dev, cap, dev_cap, type);
	}

	/* toe resource */
	if (IS_TOE_TYPE(dev))
		parse_toe_res_cap(dev, cap, dev_cap, type);

	/* mtt cache line */
	if (IS_RDMA_ENABLE(dev))
		parse_rdma_res_cap(dev, cap, dev_cap, type);

	/* RoCE resource */
	if (IS_ROCE_TYPE(dev))
		parse_roce_res_cap(dev, cap, dev_cap, type);

	if (IS_OVS_TYPE(dev))
		parse_ovs_res_cap(dev, cap, dev_cap, type);

	if (IS_IPSEC_TYPE(dev))
		parse_ipsec_res_cap(dev, cap, dev_cap, type);

	if (IS_PPA_TYPE(dev))
		parse_ppa_res_cap(dev, cap, dev_cap, type);

	if (IS_VBS_TYPE(dev))
		parse_vbs_res_cap(dev, cap, dev_cap, type);
}

static int get_cap_from_fw(struct hinic3_hwdev *dev, enum func_type type)
{
	struct cfg_cmd_dev_cap dev_cap;
	u16 out_len = sizeof(dev_cap);
	int err;

	memset(&dev_cap, 0, sizeof(dev_cap));
	dev_cap.func_id = hinic3_global_func_id(dev);
	sdk_info(dev->dev_hdl, "Get cap from fw, func_idx: %u\n",
		 dev_cap.func_id);

	err = hinic3_msg_to_mgmt_sync(dev, HINIC3_MOD_CFGM, CFG_CMD_GET_DEV_CAP,
				      &dev_cap, sizeof(dev_cap),
				      &dev_cap, &out_len, 0,
				      HINIC3_CHANNEL_COMM);
	if (err || dev_cap.head.status || !out_len) {
		sdk_err(dev->dev_hdl,
			"Failed to get capability from FW, err: %d, status: 0x%x, out size: 0x%x\n",
			err, dev_cap.head.status, out_len);
		return -EIO;
	}

	parse_dev_cap(dev, &dev_cap, type);

	return 0;
}

u8 hinic3_get_bond_create_mode(void *dev)
{
	struct hinic3_hwdev *hwdev = NULL;
	struct service_cap *cap = NULL;

	if (!dev) {
		pr_err("pointer dev is NULL\n");
		return -EINVAL;
	}

	hwdev = (struct hinic3_hwdev *)dev;
	cap = &hwdev->cfg_mgmt->svc_cap;

	return cap->bond_create_mode;
}
EXPORT_SYMBOL(hinic3_get_bond_create_mode);

int hinic3_get_dev_cap(void *dev)
{
	enum func_type type;
	int err;
	struct hinic3_hwdev *hwdev = NULL;

	if (!dev) {
		pr_err("pointer dev is NULL\n");
		return -EINVAL;
	}
	hwdev = (struct hinic3_hwdev *)dev;
	type = HINIC3_FUNC_TYPE(hwdev);

	switch (type) {
	case TYPE_PF:
	case TYPE_PPF:
	case TYPE_VF:
		err = get_cap_from_fw(hwdev, type);
		if (err != 0) {
			sdk_err(hwdev->dev_hdl,
				"Failed to get PF/PPF capability\n");
			return err;
		}
		break;
	default:
		sdk_err(hwdev->dev_hdl,
			"Unsupported PCI Function type: %d\n", type);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(hinic3_get_dev_cap);

int hinic3_get_ppf_timer_cfg(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;
	struct cfg_cmd_host_timer cfg_host_timer;
	struct service_cap *cap = &dev->cfg_mgmt->svc_cap;
	u16 out_len = sizeof(cfg_host_timer);
	int err;

	memset(&cfg_host_timer, 0, sizeof(cfg_host_timer));
	cfg_host_timer.host_id = dev->cfg_mgmt->svc_cap.host_id;

	err = hinic3_msg_to_mgmt_sync(dev, HINIC3_MOD_CFGM, CFG_CMD_GET_HOST_TIMER,
				      &cfg_host_timer, sizeof(cfg_host_timer),
				      &cfg_host_timer, &out_len, 0,
				      HINIC3_CHANNEL_COMM);
	if (err || cfg_host_timer.head.status || !out_len) {
		sdk_err(dev->dev_hdl,
			"Failed to get host timer cfg from FW, err: %d, status: 0x%x, out size: 0x%x\n",
			err, cfg_host_timer.head.status, out_len);
		return -EIO;
	}

	cap->timer_pf_id_start = cfg_host_timer.timer_pf_id_start;
	cap->timer_pf_num = cfg_host_timer.timer_pf_num;
	cap->timer_vf_id_start = cfg_host_timer.timer_vf_id_start;
	cap->timer_vf_num = cfg_host_timer.timer_vf_num;

	return 0;
}

static void nic_param_fix(struct hinic3_hwdev *dev)
{
}

static void rdma_mtt_fix(struct hinic3_hwdev *dev)
{
	struct service_cap *cap = &dev->cfg_mgmt->svc_cap;
	struct rdma_service_cap *rdma_cap = &cap->rdma_cap;

	rdma_cap->log_mtt = LOG_MTT_SEG;
	rdma_cap->log_mtt_seg = LOG_MTT_SEG;
	rdma_cap->mtt_entry_sz = MTT_ENTRY_SZ;
	rdma_cap->mpt_entry_sz = RDMA_MPT_ENTRY_SZ;
	rdma_cap->num_mtts = RDMA_NUM_MTTS;
}

static void rdma_param_fix(struct hinic3_hwdev *dev)
{
	struct service_cap *cap = &dev->cfg_mgmt->svc_cap;
	struct rdma_service_cap *rdma_cap = &cap->rdma_cap;
	struct dev_roce_svc_own_cap *roce_cap =
		&rdma_cap->dev_rdma_cap.roce_own_cap;

	rdma_cap->log_mtt = LOG_MTT_SEG;
	rdma_cap->log_rdmarc = LOG_RDMARC_SEG;
	rdma_cap->reserved_qps = RDMA_RSVD_QPS;
	rdma_cap->max_sq_sg = RDMA_MAX_SQ_SGE;

	/* RoCE */
	if (IS_ROCE_TYPE(dev)) {
		roce_cap->qpc_entry_sz = ROCE_QPC_ENTRY_SZ;
		roce_cap->max_wqes = ROCE_MAX_WQES;
		roce_cap->max_rq_sg = ROCE_MAX_RQ_SGE;
		roce_cap->max_sq_inline_data_sz = ROCE_MAX_SQ_INLINE_DATA_SZ;
		roce_cap->max_rq_desc_sz = ROCE_MAX_RQ_DESC_SZ;
		roce_cap->rdmarc_entry_sz = ROCE_RDMARC_ENTRY_SZ;
		roce_cap->max_qp_init_rdma = ROCE_MAX_QP_INIT_RDMA;
		roce_cap->max_qp_dest_rdma = ROCE_MAX_QP_DEST_RDMA;
		roce_cap->max_srq_wqes = ROCE_MAX_SRQ_WQES;
		roce_cap->reserved_srqs = ROCE_RSVD_SRQS;
		roce_cap->max_srq_sge = ROCE_MAX_SRQ_SGE;
		roce_cap->srqc_entry_sz = ROCE_SRQC_ENTERY_SZ;
		roce_cap->max_msg_sz = ROCE_MAX_MSG_SZ;
	}

	rdma_cap->max_sq_desc_sz = RDMA_MAX_SQ_DESC_SZ;
	rdma_cap->wqebb_size = WQEBB_SZ;
	rdma_cap->max_cqes = RDMA_MAX_CQES;
	rdma_cap->reserved_cqs = RDMA_RSVD_CQS;
	rdma_cap->cqc_entry_sz = RDMA_CQC_ENTRY_SZ;
	rdma_cap->cqe_size = RDMA_CQE_SZ;
	rdma_cap->reserved_mrws = RDMA_RSVD_MRWS;
	rdma_cap->mpt_entry_sz = RDMA_MPT_ENTRY_SZ;

	/* 2^8 - 1
	 *	+------------------------+-----------+
	 *	|   4B   |      1M(20b)  | Key(8b)   |
	 *	+------------------------+-----------+
	 * key = 8bit key + 24bit index,
	 * now Lkey of SGE uses 2bit(bit31 and bit30), so key only have 10bit,
	 * we use original 8bits directly for simpilification
	 */
	rdma_cap->max_fmr_maps = 0xff;
	rdma_cap->num_mtts = RDMA_NUM_MTTS;
	rdma_cap->log_mtt_seg = LOG_MTT_SEG;
	rdma_cap->mtt_entry_sz = MTT_ENTRY_SZ;
	rdma_cap->log_rdmarc_seg = LOG_RDMARC_SEG;
	rdma_cap->local_ca_ack_delay = LOCAL_ACK_DELAY;
	rdma_cap->num_ports = RDMA_NUM_PORTS;
	rdma_cap->db_page_size = DB_PAGE_SZ;
	rdma_cap->direct_wqe_size = DWQE_SZ;
	rdma_cap->num_pds = NUM_PD;
	rdma_cap->reserved_pds = RSVD_PD;
	rdma_cap->max_xrcds = MAX_XRCDS;
	rdma_cap->reserved_xrcds = RSVD_XRCDS;
	rdma_cap->max_gid_per_port = MAX_GID_PER_PORT;
	rdma_cap->gid_entry_sz = GID_ENTRY_SZ;
	rdma_cap->reserved_lkey = RSVD_LKEY;
	rdma_cap->num_comp_vectors = (u32)dev->cfg_mgmt->eq_info.num_ceq;
	rdma_cap->page_size_cap = PAGE_SZ_CAP;
	rdma_cap->flags = (RDMA_BMME_FLAG_LOCAL_INV |
			   RDMA_BMME_FLAG_REMOTE_INV |
			   RDMA_BMME_FLAG_FAST_REG_WR |
			   RDMA_DEV_CAP_FLAG_XRC |
			   RDMA_DEV_CAP_FLAG_MEM_WINDOW |
			   RDMA_BMME_FLAG_TYPE_2_WIN |
			   RDMA_BMME_FLAG_WIN_TYPE_2B |
			   RDMA_DEV_CAP_FLAG_ATOMIC);
	rdma_cap->max_frpl_len = MAX_FRPL_LEN;
	rdma_cap->max_pkeys = MAX_PKEYS;
}

static void toe_param_fix(struct hinic3_hwdev *dev)
{
	struct service_cap *cap = &dev->cfg_mgmt->svc_cap;
	struct toe_service_cap *toe_cap = &cap->toe_cap;

	toe_cap->pctx_sz = TOE_PCTX_SZ;
	toe_cap->scqc_sz = TOE_CQC_SZ;
}

static void ovs_param_fix(struct hinic3_hwdev *dev)
{
	struct service_cap *cap = &dev->cfg_mgmt->svc_cap;
	struct ovs_service_cap *ovs_cap = &cap->ovs_cap;

	ovs_cap->pctx_sz = OVS_PCTX_SZ;
}

static void ppa_param_fix(struct hinic3_hwdev *dev)
{
	struct service_cap *cap = &dev->cfg_mgmt->svc_cap;
	struct ppa_service_cap *ppa_cap = &cap->ppa_cap;

	ppa_cap->pctx_sz = PPA_PCTX_SZ;
}

static void fc_param_fix(struct hinic3_hwdev *dev)
{
	struct service_cap *cap = &dev->cfg_mgmt->svc_cap;
	struct fc_service_cap *fc_cap = &cap->fc_cap;

	fc_cap->parent_qpc_size = FC_PCTX_SZ;
	fc_cap->child_qpc_size = FC_CCTX_SZ;
	fc_cap->sqe_size = FC_SQE_SZ;

	fc_cap->scqc_size = FC_SCQC_SZ;
	fc_cap->scqe_size = FC_SCQE_SZ;

	fc_cap->srqc_size = FC_SRQC_SZ;
	fc_cap->srqe_size = FC_SRQE_SZ;
}

static void ipsec_param_fix(struct hinic3_hwdev *dev)
{
	struct service_cap *cap = &dev->cfg_mgmt->svc_cap;
	struct ipsec_service_cap *ipsec_cap = &cap->ipsec_cap;

	ipsec_cap->sactx_sz = IPSEC_SACTX_SZ;
}

static void init_service_param(struct hinic3_hwdev *dev)
{
	if (IS_NIC_TYPE(dev))
		nic_param_fix(dev);
	if (IS_RDMA_ENABLE(dev))
		rdma_mtt_fix(dev);
	if (IS_ROCE_TYPE(dev))
		rdma_param_fix(dev);
	if (IS_FC_TYPE(dev))
		fc_param_fix(dev);
	if (IS_TOE_TYPE(dev))
		toe_param_fix(dev);
	if (IS_OVS_TYPE(dev))
		ovs_param_fix(dev);
	if (IS_IPSEC_TYPE(dev))
		ipsec_param_fix(dev);
	if (IS_PPA_TYPE(dev))
		ppa_param_fix(dev);
}

static void cfg_get_eq_num(struct hinic3_hwdev *dev)
{
	struct cfg_eq_info *eq_info = &dev->cfg_mgmt->eq_info;

	eq_info->num_ceq = dev->hwif->attr.num_ceqs;
	eq_info->num_ceq_remain = eq_info->num_ceq;
}

static int cfg_init_eq(struct hinic3_hwdev *dev)
{
	struct cfg_mgmt_info *cfg_mgmt = dev->cfg_mgmt;
	struct cfg_eq *eq = NULL;
	u8 num_ceq, i = 0;

	cfg_get_eq_num(dev);
	num_ceq = cfg_mgmt->eq_info.num_ceq;

	sdk_info(dev->dev_hdl, "Cfg mgmt: ceqs=0x%x, remain=0x%x\n",
		 cfg_mgmt->eq_info.num_ceq, cfg_mgmt->eq_info.num_ceq_remain);

	if (!num_ceq) {
		return 0;
	}

	eq = kcalloc(num_ceq, sizeof(*eq), GFP_KERNEL);
	if (!eq)
		return -ENOMEM;

	for (i = 0; i < num_ceq; ++i) {
		eq[i].eqn = i;
		eq[i].free = CFG_FREE;
		eq[i].type = SERVICE_T_MAX;
	}

	cfg_mgmt->eq_info.eq = eq;

	mutex_init(&cfg_mgmt->eq_info.eq_mutex);

	return 0;
}

int hinic3_vector_to_eqn(void *hwdev, enum hinic3_service_type type, int vector)
{
	struct hinic3_hwdev *dev = hwdev;
	struct cfg_mgmt_info *cfg_mgmt = NULL;
	struct cfg_eq *eq = NULL;
	int eqn = -EINVAL;
	int vector_num = vector;

	if (!hwdev || vector < 0)
		return -EINVAL;

	if (type != SERVICE_T_ROCE) {
		sdk_err(dev->dev_hdl,
			"Service type :%d, only RDMA service could get eqn by vector.\n",
			type);
		return -EINVAL;
	}

	cfg_mgmt = dev->cfg_mgmt;
	vector_num = (vector_num % cfg_mgmt->eq_info.num_ceq) + CFG_RDMA_CEQ_BASE;

	eq = cfg_mgmt->eq_info.eq;
	if (eq[vector_num].type == SERVICE_T_ROCE && eq[vector_num].free == CFG_BUSY)
		eqn = eq[vector_num].eqn;

	return eqn;
}
EXPORT_SYMBOL(hinic3_vector_to_eqn);

static int cfg_init_interrupt(struct hinic3_hwdev *dev)
{
	struct cfg_mgmt_info *cfg_mgmt = dev->cfg_mgmt;
	struct cfg_irq_info *irq_info = &cfg_mgmt->irq_param_info;
	u16 intr_num = dev->hwif->attr.num_irqs;
	u16 intr_needed = dev->hwif->attr.msix_flex_en ? (dev->hwif->attr.num_aeqs +
			  dev->hwif->attr.num_ceqs + dev->hwif->attr.num_sq) : intr_num;

	if (!intr_num) {
		sdk_err(dev->dev_hdl, "Irq num cfg in fw is zero, msix_flex_en %d\n",
			dev->hwif->attr.msix_flex_en);
		return -EFAULT;
	}

	if (intr_needed > intr_num) {
		sdk_warn(dev->dev_hdl, "Irq num cfg(%d) is less than the needed irq num(%d) msix_flex_en %d\n",
			 intr_num, intr_needed, dev->hwif->attr.msix_flex_en);
		intr_needed = intr_num;
	}

	irq_info->alloc_info = kcalloc(intr_num, sizeof(*irq_info->alloc_info),
				       GFP_KERNEL);
	if (!irq_info->alloc_info)
		return -ENOMEM;

	irq_info->num_irq_hw = intr_needed;
	/* Production requires VF only surppots MSI-X */
	if (HINIC3_FUNC_TYPE(dev) == TYPE_VF)
		cfg_mgmt->svc_cap.interrupt_type = INTR_TYPE_MSIX;
	else
		cfg_mgmt->svc_cap.interrupt_type = 0;

	mutex_init(&irq_info->irq_mutex);
	return 0;
}

static int cfg_enable_interrupt(struct hinic3_hwdev *dev)
{
	struct cfg_mgmt_info *cfg_mgmt = dev->cfg_mgmt;
	u16 nreq = cfg_mgmt->irq_param_info.num_irq_hw;

	void *pcidev = dev->pcidev_hdl;
	struct irq_alloc_info_st *irq_info = NULL;
	struct msix_entry *entry = NULL;
	u16 i = 0;
	int actual_irq;

	irq_info = cfg_mgmt->irq_param_info.alloc_info;

	sdk_info(dev->dev_hdl, "Interrupt type: %u, irq num: %u.\n",
		 cfg_mgmt->svc_cap.interrupt_type, nreq);

	switch (cfg_mgmt->svc_cap.interrupt_type) {
	case INTR_TYPE_MSIX:
		if (!nreq) {
			sdk_err(dev->dev_hdl, "Interrupt number cannot be zero\n");
			return -EINVAL;
		}
		entry = kcalloc(nreq, sizeof(*entry), GFP_KERNEL);
		if (!entry)
			return -ENOMEM;

		for (i = 0; i < nreq; i++)
			entry[i].entry = i;

		actual_irq = pci_enable_msix_range(pcidev, entry,
						   VECTOR_THRESHOLD, nreq);
		if (actual_irq < 0) {
			sdk_err(dev->dev_hdl, "Alloc msix entries with threshold 2 failed. actual_irq: %d\n",
				actual_irq);
			kfree(entry);
			return -ENOMEM;
		}

		nreq = (u16)actual_irq;
		cfg_mgmt->irq_param_info.num_total = nreq;
		cfg_mgmt->irq_param_info.num_irq_remain = nreq;
		sdk_info(dev->dev_hdl, "Request %u msix vector success.\n",
			 nreq);

		for (i = 0; i < nreq; ++i) {
			/* u16 driver uses to specify entry, OS writes */
			irq_info[i].info.msix_entry_idx = entry[i].entry;
			/* u32 kernel uses to write allocated vector */
			irq_info[i].info.irq_id = entry[i].vector;
			irq_info[i].type = SERVICE_T_MAX;
			irq_info[i].free = CFG_FREE;
		}

		kfree(entry);

		break;

	default:
		sdk_err(dev->dev_hdl, "Unsupport interrupt type %d\n",
			cfg_mgmt->svc_cap.interrupt_type);
		break;
	}

	return 0;
}

int hinic3_alloc_irqs(void *hwdev, enum hinic3_service_type type, u16 num,
		      struct irq_info *irq_info_array, u16 *act_num)
{
	struct hinic3_hwdev *dev = hwdev;
	struct cfg_mgmt_info *cfg_mgmt = NULL;
	struct cfg_irq_info *irq_info = NULL;
	struct irq_alloc_info_st *alloc_info = NULL;
	int max_num_irq;
	u16 free_num_irq;
	int i, j;
	u16 num_new = num;

	if (!hwdev || !irq_info_array || !act_num)
		return -EINVAL;

	cfg_mgmt = dev->cfg_mgmt;
	irq_info = &cfg_mgmt->irq_param_info;
	alloc_info = irq_info->alloc_info;
	max_num_irq = irq_info->num_total;
	free_num_irq = irq_info->num_irq_remain;

	mutex_lock(&irq_info->irq_mutex);

	if (num > free_num_irq) {
		if (free_num_irq == 0) {
			sdk_err(dev->dev_hdl, "no free irq resource in cfg mgmt.\n");
			mutex_unlock(&irq_info->irq_mutex);
			return -ENOMEM;
		}

		sdk_warn(dev->dev_hdl, "only %u irq resource in cfg mgmt.\n", free_num_irq);
		num_new = free_num_irq;
	}

	*act_num = 0;

	for (i = 0; i < num_new; i++) {
		for (j = 0; j < max_num_irq; j++) {
			if (alloc_info[j].free == CFG_FREE) {
				if (irq_info->num_irq_remain == 0) {
					sdk_err(dev->dev_hdl, "No free irq resource in cfg mgmt\n");
					mutex_unlock(&irq_info->irq_mutex);
					return -EINVAL;
				}
				alloc_info[j].type = type;
				alloc_info[j].free = CFG_BUSY;

				irq_info_array[i].msix_entry_idx =
					alloc_info[j].info.msix_entry_idx;
				irq_info_array[i].irq_id = alloc_info[j].info.irq_id;
				(*act_num)++;
				irq_info->num_irq_remain--;

				break;
			}
		}
	}

	mutex_unlock(&irq_info->irq_mutex);
	return 0;
}
EXPORT_SYMBOL(hinic3_alloc_irqs);

void hinic3_free_irq(void *hwdev, enum hinic3_service_type type, u32 irq_id)
{
	struct hinic3_hwdev *dev = hwdev;
	struct cfg_mgmt_info *cfg_mgmt = NULL;
	struct cfg_irq_info *irq_info = NULL;
	struct irq_alloc_info_st *alloc_info = NULL;
	int max_num_irq;
	int i;

	if (!hwdev)
		return;

	cfg_mgmt = dev->cfg_mgmt;
	irq_info = &cfg_mgmt->irq_param_info;
	alloc_info = irq_info->alloc_info;
	max_num_irq = irq_info->num_total;

	mutex_lock(&irq_info->irq_mutex);

	for (i = 0; i < max_num_irq; i++) {
		if (irq_id == alloc_info[i].info.irq_id &&
		    type == alloc_info[i].type) {
			if (alloc_info[i].free == CFG_BUSY) {
				alloc_info[i].free = CFG_FREE;
				irq_info->num_irq_remain++;
				if (irq_info->num_irq_remain > max_num_irq) {
					sdk_err(dev->dev_hdl, "Find target,but over range\n");
					mutex_unlock(&irq_info->irq_mutex);
					return;
				}
				break;
			}
		}
	}

	if (i >= max_num_irq)
		sdk_warn(dev->dev_hdl, "Irq %u don`t need to free\n", irq_id);

	mutex_unlock(&irq_info->irq_mutex);
}
EXPORT_SYMBOL(hinic3_free_irq);

int hinic3_alloc_ceqs(void *hwdev, enum hinic3_service_type type, int num,
		      int *ceq_id_array, int *act_num)
{
	struct hinic3_hwdev *dev = hwdev;
	struct cfg_mgmt_info *cfg_mgmt = NULL;
	struct cfg_eq_info *eq = NULL;
	int free_ceq;
	int i, j;
	int num_new = num;

	if (!hwdev || !ceq_id_array || !act_num)
		return -EINVAL;

	cfg_mgmt = dev->cfg_mgmt;
	eq = &cfg_mgmt->eq_info;
	free_ceq = eq->num_ceq_remain;

	mutex_lock(&eq->eq_mutex);

	if (num > free_ceq) {
		if (free_ceq <= 0) {
			sdk_err(dev->dev_hdl, "No free ceq resource in cfg mgmt\n");
			mutex_unlock(&eq->eq_mutex);
			return -ENOMEM;
		}

		sdk_warn(dev->dev_hdl, "Only %d ceq resource in cfg mgmt\n",
			 free_ceq);
	}

	*act_num = 0;

	num_new = min(num_new, eq->num_ceq - CFG_RDMA_CEQ_BASE);
	for (i = 0; i < num_new; i++) {
		if (eq->num_ceq_remain == 0) {
			sdk_warn(dev->dev_hdl, "Alloc %d ceqs, less than required %d ceqs\n",
				 *act_num, num_new);
			mutex_unlock(&eq->eq_mutex);
			return 0;
		}

		for (j = CFG_RDMA_CEQ_BASE; j < eq->num_ceq; j++) {
			if (eq->eq[j].free == CFG_FREE) {
				eq->eq[j].type = type;
				eq->eq[j].free = CFG_BUSY;
				eq->num_ceq_remain--;
				ceq_id_array[i] = eq->eq[j].eqn;
				(*act_num)++;
				break;
			}
		}
	}

	mutex_unlock(&eq->eq_mutex);
	return 0;
}
EXPORT_SYMBOL(hinic3_alloc_ceqs);

void hinic3_free_ceq(void *hwdev, enum hinic3_service_type type, int ceq_id)
{
	struct hinic3_hwdev *dev = hwdev;
	struct cfg_mgmt_info *cfg_mgmt = NULL;
	struct cfg_eq_info *eq = NULL;
	u8 num_ceq;
	u8 i = 0;

	if (!hwdev)
		return;

	cfg_mgmt = dev->cfg_mgmt;
	eq = &cfg_mgmt->eq_info;
	num_ceq = eq->num_ceq;

	mutex_lock(&eq->eq_mutex);

	for (i = 0; i < num_ceq; i++) {
		if (ceq_id == eq->eq[i].eqn &&
		    type == cfg_mgmt->eq_info.eq[i].type) {
			if (eq->eq[i].free == CFG_BUSY) {
				eq->eq[i].free = CFG_FREE;
				eq->num_ceq_remain++;
				if (eq->num_ceq_remain > num_ceq)
					eq->num_ceq_remain %= num_ceq;

				mutex_unlock(&eq->eq_mutex);
				return;
			}
		}
	}

	if (i >= num_ceq)
		sdk_warn(dev->dev_hdl, "ceq %d don`t need to free.\n", ceq_id);

	mutex_unlock(&eq->eq_mutex);
}
EXPORT_SYMBOL(hinic3_free_ceq);

int init_cfg_mgmt(struct hinic3_hwdev *dev)
{
	int err;
	struct cfg_mgmt_info *cfg_mgmt;

	cfg_mgmt = kzalloc(sizeof(*cfg_mgmt), GFP_KERNEL);
	if (!cfg_mgmt)
		return -ENOMEM;

	dev->cfg_mgmt = cfg_mgmt;
	cfg_mgmt->hwdev = dev;

	err = cfg_init_eq(dev);
	if (err != 0) {
		sdk_err(dev->dev_hdl, "Failed to init cfg event queue, err: %d\n",
			err);
		goto free_mgmt_mem;
	}

	err = cfg_init_interrupt(dev);
	if (err != 0) {
		sdk_err(dev->dev_hdl, "Failed to init cfg interrupt, err: %d\n",
			err);
		goto free_eq_mem;
	}

	err = cfg_enable_interrupt(dev);
	if (err != 0) {
		sdk_err(dev->dev_hdl, "Failed to enable cfg interrupt, err: %d\n",
			err);
		goto free_interrupt_mem;
	}

	return 0;

free_interrupt_mem:
	kfree(cfg_mgmt->irq_param_info.alloc_info);
	mutex_deinit(&((cfg_mgmt->irq_param_info).irq_mutex));
	cfg_mgmt->irq_param_info.alloc_info = NULL;

free_eq_mem:
	kfree(cfg_mgmt->eq_info.eq);
	mutex_deinit(&cfg_mgmt->eq_info.eq_mutex);
	cfg_mgmt->eq_info.eq = NULL;

free_mgmt_mem:
	kfree(cfg_mgmt);
	return err;
}

void free_cfg_mgmt(struct hinic3_hwdev *dev)
{
	struct cfg_mgmt_info *cfg_mgmt = dev->cfg_mgmt;

	/* if the allocated resource were recycled */
	if (cfg_mgmt->irq_param_info.num_irq_remain !=
	    cfg_mgmt->irq_param_info.num_total ||
	    cfg_mgmt->eq_info.num_ceq_remain != cfg_mgmt->eq_info.num_ceq)
		sdk_err(dev->dev_hdl, "Can't reclaim all irq and event queue, please check\n");

	switch (cfg_mgmt->svc_cap.interrupt_type) {
	case INTR_TYPE_MSIX:
		pci_disable_msix(dev->pcidev_hdl);
		break;

	case INTR_TYPE_MSI:
		pci_disable_msi(dev->pcidev_hdl);
		break;

	case INTR_TYPE_INT:
	default:
		break;
	}

	kfree(cfg_mgmt->irq_param_info.alloc_info);
	cfg_mgmt->irq_param_info.alloc_info = NULL;
	mutex_deinit(&((cfg_mgmt->irq_param_info).irq_mutex));

	kfree(cfg_mgmt->eq_info.eq);
	cfg_mgmt->eq_info.eq = NULL;
	mutex_deinit(&cfg_mgmt->eq_info.eq_mutex);

	kfree(cfg_mgmt);
}

/**
 * hinic3_init_vf_dev_cap - Set max queue num for VF
 * @hwdev: the HW device for VF
 */
int hinic3_init_vf_dev_cap(void *hwdev)
{
	struct hinic3_hwdev *dev = NULL;
	enum func_type type;
	int err;

	if (!hwdev)
		return -EFAULT;

	dev = (struct hinic3_hwdev *)hwdev;
	type = HINIC3_FUNC_TYPE(dev);
	if (type != TYPE_VF)
		return -EPERM;

	err = hinic3_get_dev_cap(dev);
	if (err != 0)
		return err;

	nic_param_fix(dev);

	return 0;
}

int init_capability(struct hinic3_hwdev *dev)
{
	int err;
	struct cfg_mgmt_info *cfg_mgmt = dev->cfg_mgmt;

	cfg_mgmt->svc_cap.sf_svc_attr.ft_pf_en = false;
	cfg_mgmt->svc_cap.sf_svc_attr.rdma_pf_en = false;

	err = hinic3_get_dev_cap(dev);
	if (err != 0)
		return err;

	init_service_param(dev);

	sdk_info(dev->dev_hdl, "Init capability success\n");
	return 0;
}

void free_capability(struct hinic3_hwdev *dev)
{
	sdk_info(dev->dev_hdl, "Free capability success");
}

bool hinic3_support_nic(void *hwdev, struct nic_service_cap *cap)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev)
		return false;

	if (!IS_NIC_TYPE(dev))
		return false;

	if (cap)
		memcpy(cap, &dev->cfg_mgmt->svc_cap.nic_cap, sizeof(struct nic_service_cap));

	return true;
}
EXPORT_SYMBOL(hinic3_support_nic);

bool hinic3_support_ppa(void *hwdev, struct ppa_service_cap *cap)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev)
		return false;

	if (!IS_PPA_TYPE(dev))
		return false;

	if (cap)
		memcpy(cap, &dev->cfg_mgmt->svc_cap.ppa_cap, sizeof(struct ppa_service_cap));

	return true;
}
EXPORT_SYMBOL(hinic3_support_ppa);

bool hinic3_support_bifur(void *hwdev, struct bifur_service_cap *cap)
{
	struct hinic3_hwdev *dev = (struct hinic3_hwdev *)hwdev;

	if (!hwdev)
		return false;

	if (!IS_BIFUR_TYPE(dev))
		return false;

	return true;
}
EXPORT_SYMBOL(hinic3_support_bifur);

bool hinic3_support_migr(void *hwdev, struct migr_service_cap *cap)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev)
		return false;

	if (!IS_MIGR_TYPE(dev))
		return false;

	if (cap)
		cap->master_host_id = dev->cfg_mgmt->svc_cap.master_host_id;

	return true;
}
EXPORT_SYMBOL(hinic3_support_migr);

bool hinic3_support_ipsec(void *hwdev, struct ipsec_service_cap *cap)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev)
		return false;

	if (!IS_IPSEC_TYPE(dev))
		return false;

	if (cap)
		memcpy(cap, &dev->cfg_mgmt->svc_cap.ipsec_cap, sizeof(struct ipsec_service_cap));

	return true;
}
EXPORT_SYMBOL(hinic3_support_ipsec);

bool hinic3_support_roce(void *hwdev, struct rdma_service_cap *cap)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev)
		return false;

	if (!IS_ROCE_TYPE(dev))
		return false;

	if (cap)
		memcpy(cap, &dev->cfg_mgmt->svc_cap.rdma_cap, sizeof(struct rdma_service_cap));

	return true;
}
EXPORT_SYMBOL(hinic3_support_roce);

bool hinic3_support_fc(void *hwdev, struct fc_service_cap *cap)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev)
		return false;

	if (!IS_FC_TYPE(dev))
		return false;

	if (cap)
		memcpy(cap, &dev->cfg_mgmt->svc_cap.fc_cap, sizeof(struct fc_service_cap));

	return true;
}
EXPORT_SYMBOL(hinic3_support_fc);

bool hinic3_support_rdma(void *hwdev, struct rdma_service_cap *cap)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev)
		return false;

	if (!IS_RDMA_TYPE(dev) && !(IS_RDMA_ENABLE(dev)))
		return false;

	if (cap)
		memcpy(cap, &dev->cfg_mgmt->svc_cap.rdma_cap, sizeof(struct rdma_service_cap));

	return true;
}
EXPORT_SYMBOL(hinic3_support_rdma);

bool hinic3_support_ovs(void *hwdev, struct ovs_service_cap *cap)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev)
		return false;

	if (!IS_OVS_TYPE(dev))
		return false;

	if (cap)
		memcpy(cap, &dev->cfg_mgmt->svc_cap.ovs_cap, sizeof(struct ovs_service_cap));

	return true;
}
EXPORT_SYMBOL(hinic3_support_ovs);

bool hinic3_support_vbs(void *hwdev, struct vbs_service_cap *cap)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev)
		return false;

	if (!IS_VBS_TYPE(dev))
		return false;

	if (cap)
		memcpy(cap, &dev->cfg_mgmt->svc_cap.vbs_cap, sizeof(struct vbs_service_cap));

	return true;
}
EXPORT_SYMBOL(hinic3_support_vbs);

bool hinic3_is_guest_vmsec_enable(void *hwdev)
{
	struct hinic3_hwdev *hw_dev = hwdev;

	if (!hwdev) {
		pr_err("hwdev is null\n");
		return false;
	}

    /* vf used in vm */
	if (IS_VM_SLAVE_HOST(hw_dev) && (hinic3_func_type(hwdev) == TYPE_VF) &&
	    IS_RDMA_TYPE(hw_dev)) {
		return true;
	}

	return false;
}
EXPORT_SYMBOL(hinic3_is_guest_vmsec_enable);

/* Only PPF support it, PF is not */
bool hinic3_support_toe(void *hwdev, struct toe_service_cap *cap)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev)
		return false;

	if (!IS_TOE_TYPE(dev))
		return false;

	if (cap)
		memcpy(cap, &dev->cfg_mgmt->svc_cap.toe_cap, sizeof(struct toe_service_cap));

	return true;
}
EXPORT_SYMBOL(hinic3_support_toe);

bool hinic3_func_for_mgmt(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev)
		return false;

	if (dev->cfg_mgmt->svc_cap.chip_svc_type)
		return false;
	else
		return true;
}

bool hinic3_get_stateful_enable(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev)
		return false;

	return dev->cfg_mgmt->svc_cap.sf_en;
}
EXPORT_SYMBOL(hinic3_get_stateful_enable);

bool hinic3_get_timer_enable(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev)
		return false;

	return dev->cfg_mgmt->svc_cap.timer_en;
}
EXPORT_SYMBOL(hinic3_get_timer_enable);

u8 hinic3_host_oq_id_mask(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!dev) {
		pr_err("Hwdev pointer is NULL for getting host oq id mask\n");
		return 0;
	}
	return dev->cfg_mgmt->svc_cap.host_oq_id_mask_val;
}
EXPORT_SYMBOL(hinic3_host_oq_id_mask);

u8 hinic3_host_id(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!dev) {
		pr_err("Hwdev pointer is NULL for getting host id\n");
		return 0;
	}
	return dev->cfg_mgmt->svc_cap.host_id;
}
EXPORT_SYMBOL(hinic3_host_id);

u16 hinic3_host_total_func(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!dev) {
		pr_err("Hwdev pointer is NULL for getting host total function number\n");
		return 0;
	}
	return dev->cfg_mgmt->svc_cap.host_total_function;
}
EXPORT_SYMBOL(hinic3_host_total_func);

u16 hinic3_func_max_qnum(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!dev) {
		pr_err("Hwdev pointer is NULL for getting function max queue number\n");
		return 0;
	}
	return dev->cfg_mgmt->svc_cap.nic_cap.max_sqs;
}
EXPORT_SYMBOL(hinic3_func_max_qnum);

u16 hinic3_func_max_nic_qnum(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!dev) {
		pr_err("Hwdev pointer is NULL for getting function max queue number\n");
		return 0;
	}
	return dev->cfg_mgmt->svc_cap.nic_cap.max_sqs;
}
EXPORT_SYMBOL(hinic3_func_max_nic_qnum);

u8 hinic3_ep_id(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!dev) {
		pr_err("Hwdev pointer is NULL for getting ep id\n");
		return 0;
	}
	return dev->cfg_mgmt->svc_cap.ep_id;
}
EXPORT_SYMBOL(hinic3_ep_id);

u8 hinic3_er_id(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!dev) {
		pr_err("Hwdev pointer is NULL for getting er id\n");
		return 0;
	}
	return dev->cfg_mgmt->svc_cap.er_id;
}
EXPORT_SYMBOL(hinic3_er_id);

u8 hinic3_physical_port_id(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!dev) {
		pr_err("Hwdev pointer is NULL for getting physical port id\n");
		return 0;
	}
	return dev->cfg_mgmt->svc_cap.port_id;
}
EXPORT_SYMBOL(hinic3_physical_port_id);

u16 hinic3_func_max_vf(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!dev) {
		pr_err("Hwdev pointer is NULL for getting max vf number\n");
		return 0;
	}
	return dev->cfg_mgmt->svc_cap.max_vf;
}
EXPORT_SYMBOL(hinic3_func_max_vf);

int hinic3_cos_valid_bitmap(void *hwdev, u8 *func_dft_cos, u8 *port_cos_bitmap)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!dev) {
		pr_err("Hwdev pointer is NULL for getting cos valid bitmap\n");
		return 1;
	}
	*func_dft_cos = dev->cfg_mgmt->svc_cap.cos_valid_bitmap;
	*port_cos_bitmap = dev->cfg_mgmt->svc_cap.port_cos_valid_bitmap;

	return 0;
}
EXPORT_SYMBOL(hinic3_cos_valid_bitmap);

void hinic3_shutdown_hwdev(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev)
		return;

	if (IS_SLAVE_HOST(dev))
		hinic3_set_slave_host_enable(hwdev,
					     hinic3_pcie_itf_id(hwdev), false);
}

u32 hinic3_host_pf_num(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!dev) {
		pr_err("Hwdev pointer is NULL for getting pf number capability\n");
		return 0;
	}

	return dev->cfg_mgmt->svc_cap.pf_num;
}
EXPORT_SYMBOL(hinic3_host_pf_num);

u32 hinic3_host_pf_id_start(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!dev) {
		pr_err("Hwdev pointer is NULL for getting pf id start capability\n");
		return 0;
	}

	return dev->cfg_mgmt->svc_cap.pf_id_start;
}
EXPORT_SYMBOL(hinic3_host_pf_id_start);

u8 hinic3_flexq_en(void *hwdev)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev)
		return 0;

	return dev->cfg_mgmt->svc_cap.flexq_en;
}
EXPORT_SYMBOL(hinic3_flexq_en);

int hinic3_get_fake_vf_info(void *hwdev, u8 *fake_vf_vld,
			    u8 *page_bit, u8 *pf_start_bit, u8 *map_host_id)
{
	struct hinic3_hwdev *dev = hwdev;

	if (!dev) {
		pr_err("Hwdev pointer is NULL for getting pf id start capability\n");
		return -EINVAL;
	}

	if (!fake_vf_vld || !page_bit || !pf_start_bit || !map_host_id) {
		pr_err("Fake vf member pointer is NULL for getting pf id start capability\n");
		return -EINVAL;
	}

	*fake_vf_vld = dev->cfg_mgmt->svc_cap.fake_vf_en;
	*page_bit = dev->cfg_mgmt->svc_cap.fake_vf_page_bit;
	*pf_start_bit = dev->cfg_mgmt->svc_cap.fake_vf_start_bit;
	*map_host_id = dev->cfg_mgmt->svc_cap.map_host_id;

	return 0;
}
EXPORT_SYMBOL(hinic3_get_fake_vf_info);


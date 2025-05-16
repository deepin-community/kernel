// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>

#include "ossl_knl.h"
#include "hinic3_hw.h"
#include "hinic3_mt.h"
#include "hinic3_hwdev.h"
#include "hinic3_hwif.h"
#include "hinic3_hw_cfg.h"

#include "cqm_object.h"
#include "cqm_bitmap_table.h"
#include "cqm_bat_cla.h"
#include "cqm_bloomfilter.h"
#include "cqm_db.h"
#include "cqm_memsec.h"
#include "cqm_main.h"

#include "vram_common.h"

static unsigned char roce_qpc_rsv_mode = CQM_QPC_ROCE_NORMAL;
module_param(roce_qpc_rsv_mode, byte, 0644);
MODULE_PARM_DESC(roce_qpc_rsv_mode,
		 "for roce reserve 4k qpc(qpn) (default=0, 0-rsv:2, 1-rsv:4k, 2-rsv:200k+2)");

static s32 cqm_set_fake_vf_child_timer(struct tag_cqm_handle *cqm_handle,
				       struct tag_cqm_handle *fake_cqm_handle, bool en)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)cqm_handle->ex_handle;
	u16 func_global_idx;
	s32 ret;

	if (fake_cqm_handle->func_capability.timer_enable == 0)
		return CQM_SUCCESS;

	func_global_idx = fake_cqm_handle->func_attribute.func_global_idx;
	ret = hinic3_func_tmr_bitmap_set(cqm_handle->ex_handle, func_global_idx, en);
	if (ret != CQM_SUCCESS) {
		cqm_err(handle->dev_hdl, "func_id %u Timer %s timer bitmap failed\n",
			func_global_idx, en ? "enable" : "disable");
		return CQM_FAIL;
}

	return CQM_SUCCESS;
}

static s32 cqm_unset_fake_vf_timer(struct tag_cqm_handle *cqm_handle)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)cqm_handle->ex_handle;
	s32 child_func_number;
	u32 i;

	child_func_number = cqm_get_child_func_number(cqm_handle);
	if (child_func_number == CQM_FAIL) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(child_func_number));
		return CQM_FAIL;
	}

	for (i = 0; i < (u32)child_func_number; i++)
		(void)cqm_set_fake_vf_child_timer(cqm_handle,
						  cqm_handle->fake_cqm_handle[i], false);

	return CQM_SUCCESS;
}

static s32 cqm_set_fake_vf_timer(struct tag_cqm_handle *cqm_handle)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)cqm_handle->ex_handle;
	s32 child_func_number;
	u32 i;
	s32 ret;

	child_func_number = cqm_get_child_func_number(cqm_handle);
	if (child_func_number == CQM_FAIL) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(child_func_number));
		return CQM_FAIL;
	}

	for (i = 0; i < (u32)child_func_number; i++) {
		ret = cqm_set_fake_vf_child_timer(cqm_handle,
						  cqm_handle->fake_cqm_handle[i], true);
		if (ret != CQM_SUCCESS)
			goto err;
	}

	return CQM_SUCCESS;
err:
	(void)cqm_unset_fake_vf_timer(cqm_handle);
	return CQM_FAIL;
}

static s32 cqm_set_timer_enable(void *ex_handle)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_cqm_handle *cqm_handle = NULL;
	int is_in_kexec;

	if (!ex_handle)
		return CQM_FAIL;

	is_in_kexec = vram_get_kexec_flag();
	if (is_in_kexec != 0) {
		cqm_info(handle->dev_hdl, "Skip starting cqm timer during kexec\n");
		return CQM_SUCCESS;
	}

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	if (cqm_handle->func_capability.fake_func_type == CQM_FAKE_FUNC_PARENT &&
	    cqm_set_fake_vf_timer(cqm_handle) != CQM_SUCCESS)
		return CQM_FAIL;

	/* The timer bitmap is set directly at the beginning of the CQM.
	 * The ifconfig up/down command is not used to set or clear the bitmap.
	 */
	if (hinic3_func_tmr_bitmap_set(ex_handle, hinic3_global_func_id(ex_handle),
				       true) != CQM_SUCCESS) {
		cqm_err(handle->dev_hdl, "func_id %u Timer start: enable timer bitmap failed\n",
			hinic3_global_func_id(ex_handle));
		goto err;
	}

	return CQM_SUCCESS;

err:
	cqm_unset_fake_vf_timer(cqm_handle);
	return CQM_FAIL;
}

static s32 cqm_set_timer_disable(void *ex_handle)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_cqm_handle *cqm_handle = NULL;

	if (!ex_handle)
		return CQM_FAIL;

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);

	if (cqm_handle->func_capability.fake_func_type != CQM_FAKE_FUNC_CHILD_CONFLICT &&
	    hinic3_func_tmr_bitmap_set(ex_handle, hinic3_global_func_id(ex_handle),
				       false) != CQM_SUCCESS)
		cqm_err(handle->dev_hdl, "func_id %u Timer stop: disable timer bitmap failed\n",
			hinic3_global_func_id(ex_handle));

	if (cqm_handle->func_capability.fake_func_type == CQM_FAKE_FUNC_PARENT &&
	    cqm_unset_fake_vf_timer(cqm_handle) != CQM_SUCCESS)
		return CQM_FAIL;

	return CQM_SUCCESS;
}

static s32 cqm_init_all(void *ex_handle)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;

	/* Initialize secure memory. */
	if (cqm_secure_mem_init(ex_handle) != CQM_SUCCESS) {
		cqm_err(handle->dev_hdl, CQM_FUNCTION_FAIL(cqm_mem_init));
		return CQM_FAIL;
	}

	/* Initialize memory entries such as BAT, CLA, and bitmap. */
	if (cqm_mem_init(ex_handle) != CQM_SUCCESS) {
		cqm_err(handle->dev_hdl, CQM_FUNCTION_FAIL(cqm_mem_init));
		goto err1;
	}

	/* Event callback initialization */
	if (cqm_event_init(ex_handle) != CQM_SUCCESS) {
		cqm_err(handle->dev_hdl, CQM_FUNCTION_FAIL(cqm_event_init));
		goto err2;
	}

	/* Doorbell initiation */
	if (cqm_db_init(ex_handle) != CQM_SUCCESS) {
		cqm_err(handle->dev_hdl, CQM_FUNCTION_FAIL(cqm_db_init));
		goto err3;
	}

	/* Initialize the bloom filter. */
	if (cqm_bloomfilter_init(ex_handle) != CQM_SUCCESS) {
		cqm_err(handle->dev_hdl, CQM_FUNCTION_FAIL(cqm_bloomfilter_init));
		goto err4;
	}

	if (cqm_set_timer_enable(ex_handle) != CQM_SUCCESS) {
		cqm_err(handle->dev_hdl, CQM_FUNCTION_FAIL(cqm_set_timer_enable));
		goto err5;
	}

	return CQM_SUCCESS;
err5:
	cqm_bloomfilter_uninit(ex_handle);
err4:
	cqm_db_uninit(ex_handle);
err3:
	cqm_event_uninit(ex_handle);
err2:
	cqm_mem_uninit(ex_handle);
err1:
	cqm_secure_mem_deinit(ex_handle);
	return CQM_FAIL;
}

/**
 * cqm_init - Complete CQM initialization.
 *            If the function is a parent fake function, copy the fake.
 *            If it is a child fake function (in the fake copy function,
 *            not in this function), set fake_en in the BAT/CLA table.
 *            cqm_init->cqm_mem_init->cqm_fake_init(copy)
 *            If the child fake conflict occurs, resources are not
 *            initialized, but the timer must be enabled.
 *            If the function is of the normal type,
 *            follow the normal process.
 * @ex_handle: device pointer that represents the PF
 */
s32 cqm_init(void *ex_handle)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_cqm_handle *cqm_handle = NULL;
	s32 ret;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: ex_handle is null\n", __func__);
		return CQM_FAIL;
	}

	cqm_handle = kmalloc(sizeof(*cqm_handle), GFP_KERNEL | __GFP_ZERO);
	if (!cqm_handle)
		return CQM_FAIL;

	/* Clear the memory to prevent other systems from
	 * not clearing the memory.
	 */
	memset(cqm_handle, 0, sizeof(struct tag_cqm_handle));

	cqm_handle->ex_handle = handle;
	cqm_handle->dev = (struct pci_dev *)(handle->pcidev_hdl);
	handle->cqm_hdl = (void *)cqm_handle;

	/* Clearing Statistics */
	memset(&handle->hw_stats.cqm_stats, 0, sizeof(struct cqm_stats));

	/* Reads VF/PF information. */
	cqm_handle->func_attribute = handle->hwif->attr;
	cqm_info(handle->dev_hdl, "Func init: function[%u] type %d(0:PF,1:VF,2:PPF)\n",
		 cqm_handle->func_attribute.func_global_idx,
		 cqm_handle->func_attribute.func_type);

	/* Read capability from configuration management module */
	ret = cqm_capability_init(ex_handle);
	if (ret == CQM_FAIL) {
		cqm_err(handle->dev_hdl,
			CQM_FUNCTION_FAIL(cqm_capability_init));
		goto err;
	}

	/* In FAKE mode, only the bitmap of the timer of the function is
	 * enabled, and resources are not initialized. Otherwise, the
	 * configuration of the fake function is overwritten.
	 */
	if (cqm_handle->func_capability.fake_func_type == CQM_FAKE_FUNC_CHILD_CONFLICT) {
		handle->cqm_hdl = NULL;
		kfree(cqm_handle);
		return CQM_SUCCESS;
	}

	ret = cqm_init_all(ex_handle);
	if (ret == CQM_FAIL)
		goto err;

	return CQM_SUCCESS;
err:
	handle->cqm_hdl = NULL;
	kfree(cqm_handle);
	return CQM_FAIL;
}

/**
 * cqm_uninit - Deinitializes the CQM module. This function is called once each time
 *              a function is removed.
 * @ex_handle: device pointer that represents the PF
 */
void cqm_uninit(void *ex_handle)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_cqm_handle *cqm_handle = NULL;
	s32 ret;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: ex_handle is null\n", __func__);
		return;
	}

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	if (unlikely(!cqm_handle)) {
		pr_err("[CQM]%s: cqm_handle is null\n", __func__);
		return;
	}

	cqm_set_timer_disable(ex_handle);

	/* After the TMR timer stops, the system releases resources
	 * after a delay of one or two milliseconds.
	 */
	if (cqm_handle->func_attribute.func_type == CQM_PPF) {
		if (cqm_handle->func_capability.timer_enable ==
		    CQM_TIMER_ENABLE) {
			cqm_info(handle->dev_hdl, "PPF timer stop\n");
			ret = hinic3_ppf_tmr_stop(handle);
			if (ret != CQM_SUCCESS)
				/* The timer fails to be stopped,
				 * and the resource release is not affected.
				 */
				cqm_info(handle->dev_hdl, "PPF timer stop, ret=%d\n", ret);
		}

		hinic3_ppf_ht_gpa_deinit(handle);

		usleep_range(0x384, 0x3E8); /* Somebody requires a delay of 1 ms,
					     * which is inaccurate.
					     */
	}

	/* Release Bloom Filter Table */
	cqm_bloomfilter_uninit(ex_handle);

	/* Release hardware doorbell */
	cqm_db_uninit(ex_handle);

	/* Cancel the callback of the event */
	cqm_event_uninit(ex_handle);

	/* Release various memory tables and require the service
	 * to release all objects.
	 */
	cqm_mem_uninit(ex_handle);

	cqm_secure_mem_deinit(ex_handle);

	/* Release cqm_handle */
	handle->cqm_hdl = NULL;
	kfree(cqm_handle);
}

static void cqm_test_mode_init(struct tag_cqm_handle *cqm_handle,
			       struct service_cap *service_capability)
{
	struct tag_cqm_func_capability *func_cap = &cqm_handle->func_capability;
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;

	if (service_capability->test_mode == 0)
		return;

	cqm_info(handle->dev_hdl, "Enter CQM test mode\n");

	func_cap->qpc_number = service_capability->test_qpc_num;
	func_cap->qpc_reserved =
	    GET_MAX(func_cap->qpc_reserved,
		    service_capability->test_qpc_resvd_num);
	func_cap->xid_alloc_mode = service_capability->test_xid_alloc_mode;
	func_cap->gpa_check_enable = service_capability->test_gpa_check_enable;
	func_cap->pagesize_reorder = service_capability->test_page_size_reorder;
	func_cap->qpc_alloc_static =
	    (bool)(service_capability->test_qpc_alloc_mode);
	func_cap->scqc_alloc_static =
	    (bool)(service_capability->test_scqc_alloc_mode);
	func_cap->flow_table_based_conn_number =
	    service_capability->test_max_conn_num;
	func_cap->flow_table_based_conn_cache_number =
	    service_capability->test_max_cache_conn_num;
	func_cap->scqc_number = service_capability->test_scqc_num;
	func_cap->mpt_number = service_capability->test_mpt_num;
	func_cap->mpt_reserved = service_capability->test_mpt_recvd_num;
	func_cap->reorder_number = service_capability->test_reorder_num;
	/* 256K buckets, 256K*64B = 16MB */
	func_cap->hash_number = service_capability->test_hash_num;
}

static void cqm_service_capability_update(struct tag_cqm_handle *cqm_handle)
{
	struct tag_cqm_func_capability *func_cap = &cqm_handle->func_capability;

	func_cap->qpc_number = GET_MIN(CQM_MAX_QPC_NUM, func_cap->qpc_number);
	func_cap->scqc_number = GET_MIN(CQM_MAX_SCQC_NUM,
					func_cap->scqc_number);
	func_cap->srqc_number = GET_MIN(CQM_MAX_SRQC_NUM,
					func_cap->srqc_number);
	func_cap->childc_number = GET_MIN(CQM_MAX_CHILDC_NUM,
					  func_cap->childc_number);
}

static void cqm_service_valid_init(struct tag_cqm_handle *cqm_handle,
				   const struct service_cap *service_capability)
{
	u16 type = service_capability->chip_svc_type;
	struct tag_cqm_service *svc = cqm_handle->service;

	svc[CQM_SERVICE_T_NIC].valid = ((type & CFG_SERVICE_MASK_NIC) != 0) ?
				       true : false;
	svc[CQM_SERVICE_T_OVS].valid = ((type & CFG_SERVICE_MASK_OVS) != 0) ?
				       true : false;
	svc[CQM_SERVICE_T_ROCE].valid = ((type & CFG_SERVICE_MASK_ROCE) != 0) ?
					true : false;
	svc[CQM_SERVICE_T_TOE].valid = ((type & CFG_SERVICE_MASK_TOE) != 0) ?
				       true : false;
	svc[CQM_SERVICE_T_FC].valid = ((type & CFG_SERVICE_MASK_FC) != 0) ?
				      true : false;
	svc[CQM_SERVICE_T_IPSEC].valid = ((type & CFG_SERVICE_MASK_IPSEC) != 0) ?
					 true : false;
	svc[CQM_SERVICE_T_VBS].valid = ((type & CFG_SERVICE_MASK_VBS) != 0) ?
				       true : false;
	svc[CQM_SERVICE_T_VIRTIO].valid = ((type & CFG_SERVICE_MASK_VIRTIO) != 0) ?
					  true : false;
	svc[CQM_SERVICE_T_IOE].valid = false;
	svc[CQM_SERVICE_T_PPA].valid = ((type & CFG_SERVICE_MASK_PPA) != 0) ?
					  true : false;
}

static void cqm_service_capability_init_nic(struct tag_cqm_handle *cqm_handle, void *pra)
{
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;

	cqm_info(handle->dev_hdl, "Cap init: nic is valid, but nic need not be init by cqm\n");
}

static void cqm_service_capability_init_ovs(struct tag_cqm_handle *cqm_handle, void *pra)
{
	struct tag_cqm_func_capability *func_cap = &cqm_handle->func_capability;
	struct service_cap *service_capability = (struct service_cap *)pra;
	struct ovs_service_cap *ovs_cap = &service_capability->ovs_cap;
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;

	cqm_info(handle->dev_hdl, "Cap init: ovs is valid\n");
	cqm_info(handle->dev_hdl, "Cap init: ovs qpc 0x%x\n",
		 ovs_cap->dev_ovs_cap.max_pctxs);
	func_cap->hash_number += ovs_cap->dev_ovs_cap.max_pctxs;
	func_cap->hash_basic_size = CQM_HASH_BUCKET_SIZE_64;
	func_cap->qpc_number += ovs_cap->dev_ovs_cap.max_pctxs;
	func_cap->qpc_basic_size = GET_MAX(ovs_cap->pctx_sz,
					   func_cap->qpc_basic_size);
	func_cap->qpc_reserved += ovs_cap->dev_ovs_cap.max_pctxs;
	func_cap->qpc_alloc_static = true;
	func_cap->pagesize_reorder = CQM_OVS_PAGESIZE_ORDER;
}

static void cqm_service_capability_init_roce(struct tag_cqm_handle *cqm_handle, void *pra)
{
	struct tag_cqm_func_capability *func_cap = &cqm_handle->func_capability;
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;
	struct hinic3_board_info *board_info = &handle->board_info;
	struct service_cap *service_capability = (struct service_cap *)pra;
	struct rdma_service_cap *rdma_cap = &service_capability->rdma_cap;
	struct dev_roce_svc_own_cap *roce_own_cap =
	    &rdma_cap->dev_rdma_cap.roce_own_cap;

	cqm_info(handle->dev_hdl, "Cap init: roce is valid\n");
	cqm_info(handle->dev_hdl, "Cap init: roce qpc 0x%x, scqc 0x%x, srqc 0x%x, drc_qp 0x%x\n",
		 roce_own_cap->max_qps, roce_own_cap->max_cqs,
		 roce_own_cap->max_srqs, roce_own_cap->max_drc_qps);
	cqm_info(handle->dev_hdl, "Cap init: type 0x%x, scenes:0x%x, qpc_rsv:0x%x, srv_bmp:0x%x\n",
		 board_info->board_type, board_info->scenes_id,
		 roce_qpc_rsv_mode, board_info->service_en_bitmap);

	if (roce_qpc_rsv_mode == CQM_QPC_ROCE_VBS_MODE) {
		func_cap->qpc_reserved += CQM_QPC_ROCE_RSVD;
		func_cap->qpc_reserved_back += CQM_QPC_ROCE_VBS_RSVD_BACK;
	} else if ((service_capability->chip_svc_type & CFG_SERVICE_MASK_ROCEAA) != 0) {
		func_cap->qpc_reserved += CQM_QPC_ROCEAA_RSVD;
		func_cap->scq_reserved += CQM_CQ_ROCEAA_RSVD;
		func_cap->srq_reserved += CQM_SRQ_ROCEAA_RSVD;
	} else {
		func_cap->qpc_reserved += CQM_QPC_ROCE_RSVD;
	}
	func_cap->qpc_number += roce_own_cap->max_qps;
	func_cap->qpc_basic_size = GET_MAX(roce_own_cap->qpc_entry_sz,
					   func_cap->qpc_basic_size);
	if (cqm_handle->func_attribute.func_type == CQM_PF &&
	    (IS_MASTER_HOST(handle))) {
		func_cap->hash_number = roce_own_cap->max_qps;
		func_cap->hash_basic_size = CQM_HASH_BUCKET_SIZE_64;
	}
	func_cap->qpc_alloc_static = true;
	func_cap->scqc_number += roce_own_cap->max_cqs;
	func_cap->scqc_basic_size = GET_MAX(rdma_cap->cqc_entry_sz,
					    func_cap->scqc_basic_size);
	func_cap->srqc_number += roce_own_cap->max_srqs;
	func_cap->srqc_basic_size = GET_MAX(roce_own_cap->srqc_entry_sz,
					    func_cap->srqc_basic_size);
	func_cap->mpt_number += roce_own_cap->max_mpts;
	func_cap->mpt_reserved += rdma_cap->reserved_mrws;
	func_cap->mpt_basic_size = GET_MAX(rdma_cap->mpt_entry_sz,
					   func_cap->mpt_basic_size);
	func_cap->gid_number = CQM_GID_RDMA_NUM;
	func_cap->gid_basic_size = CQM_GID_SIZE_32;
	func_cap->childc_number += CQM_CHILDC_ROCE_NUM;
	func_cap->childc_basic_size = GET_MAX(CQM_CHILDC_SIZE_256,
					      func_cap->childc_basic_size);
}

static void cqm_service_capability_init_toe(struct tag_cqm_handle *cqm_handle, void *pra)
{
	struct tag_cqm_toe_private_capability *toe_own_cap = &cqm_handle->toe_own_capability;
	struct tag_cqm_func_capability *func_cap = &cqm_handle->func_capability;
	struct service_cap *service_capability = (struct service_cap *)pra;
	struct rdma_service_cap *rdma_cap = &service_capability->rdma_cap;
	struct toe_service_cap *toe_cap = &service_capability->toe_cap;
	struct dev_toe_svc_cap *dev_toe_cap = &toe_cap->dev_toe_cap;
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;

	cqm_info(handle->dev_hdl, "Cap init: toe is valid\n");
	cqm_info(handle->dev_hdl, "Cap init: toe qpc 0x%x, scqc 0x%x, srqc 0x%x\n",
		 dev_toe_cap->max_pctxs, dev_toe_cap->max_cqs,
		 dev_toe_cap->max_srqs);
	func_cap->hash_number += dev_toe_cap->max_pctxs;
	func_cap->hash_basic_size = CQM_HASH_BUCKET_SIZE_64;
	func_cap->qpc_number += dev_toe_cap->max_pctxs;
	func_cap->qpc_basic_size = GET_MAX(toe_cap->pctx_sz,
					   func_cap->qpc_basic_size);
	func_cap->qpc_alloc_static = true;
	func_cap->scqc_number += dev_toe_cap->max_cqs;
	func_cap->scqc_basic_size = GET_MAX(toe_cap->scqc_sz,
					    func_cap->scqc_basic_size);
	func_cap->scqc_alloc_static = true;

	toe_own_cap->toe_srqc_number = dev_toe_cap->max_srqs;
	toe_own_cap->toe_srqc_start_id = dev_toe_cap->srq_id_start;
	toe_own_cap->toe_srqc_basic_size = CQM_SRQC_SIZE_64;
	func_cap->childc_number += dev_toe_cap->max_cctxt;
	func_cap->childc_basic_size = GET_MAX(CQM_CHILDC_SIZE_256,
					      func_cap->childc_basic_size);
	func_cap->mpt_number += dev_toe_cap->max_mpts;
	func_cap->mpt_reserved = 0;
	func_cap->mpt_basic_size = GET_MAX(rdma_cap->mpt_entry_sz,
					   func_cap->mpt_basic_size);
}

static void cqm_service_capability_init_ioe(struct tag_cqm_handle *cqm_handle, void *pra)
{
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;

	cqm_info(handle->dev_hdl, "Cap init: ioe is valid\n");
}

static void cqm_service_capability_init_fc(struct tag_cqm_handle *cqm_handle, void *pra)
{
	struct tag_cqm_func_capability *func_cap = &cqm_handle->func_capability;
	struct service_cap *service_capability = (struct service_cap *)pra;
	struct fc_service_cap *fc_cap = &service_capability->fc_cap;
	struct dev_fc_svc_cap *dev_fc_cap = &fc_cap->dev_fc_cap;
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;

	cqm_info(handle->dev_hdl, "Cap init: fc is valid\n");
	cqm_info(handle->dev_hdl, "Cap init: fc qpc 0x%x, scqc 0x%x, srqc 0x%x\n",
		 dev_fc_cap->max_parent_qpc_num, dev_fc_cap->scq_num,
		 dev_fc_cap->srq_num);
	func_cap->hash_number += dev_fc_cap->max_parent_qpc_num;
	func_cap->hash_basic_size = CQM_HASH_BUCKET_SIZE_64;
	func_cap->qpc_number += dev_fc_cap->max_parent_qpc_num;
	func_cap->qpc_basic_size = GET_MAX(fc_cap->parent_qpc_size,
					   func_cap->qpc_basic_size);
	func_cap->qpc_alloc_static = true;
	func_cap->scqc_number += dev_fc_cap->scq_num;
	func_cap->scqc_basic_size = GET_MAX(fc_cap->scqc_size,
					    func_cap->scqc_basic_size);
	func_cap->srqc_number += dev_fc_cap->srq_num;
	func_cap->srqc_basic_size = GET_MAX(fc_cap->srqc_size,
					    func_cap->srqc_basic_size);
	func_cap->lun_number = CQM_LUN_FC_NUM;
	func_cap->lun_basic_size = CQM_LUN_SIZE_8;
	func_cap->taskmap_number = CQM_TASKMAP_FC_NUM;
	func_cap->taskmap_basic_size = PAGE_SIZE;
	func_cap->childc_number += dev_fc_cap->max_child_qpc_num;
	func_cap->childc_basic_size = GET_MAX(fc_cap->child_qpc_size,
					      func_cap->childc_basic_size);
	func_cap->pagesize_reorder = CQM_FC_PAGESIZE_ORDER;
}

static void cqm_service_capability_init_vbs(struct tag_cqm_handle *cqm_handle, void *pra)
{
	struct tag_cqm_func_capability *func_cap = &cqm_handle->func_capability;
	struct service_cap *service_capability = (struct service_cap *)pra;
	struct vbs_service_cap *vbs_cap = &service_capability->vbs_cap;
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;

	cqm_info(handle->dev_hdl, "Cap init: vbs is valid\n");

	/* If the entry size is greater than the cache line (256 bytes),
	 * align the entries by cache line.
	 */
	func_cap->xid2cid_number +=
	    (CQM_XID2CID_VBS_NUM * service_capability->virtio_vq_size) / CQM_CHIP_CACHELINE;
	func_cap->xid2cid_basic_size = CQM_CHIP_CACHELINE;
	func_cap->qpc_number += (vbs_cap->vbs_max_volq * 2); // VOLQ group * 2
	func_cap->qpc_basic_size = GET_MAX(CQM_VBS_QPC_SIZE,
					   func_cap->qpc_basic_size);
	func_cap->qpc_alloc_static = true;
}

static void cqm_service_capability_init_ipsec(struct tag_cqm_handle *cqm_handle, void *pra)
{
	struct tag_cqm_func_capability *func_cap = &cqm_handle->func_capability;
	struct service_cap *service_capability = (struct service_cap *)pra;
	struct ipsec_service_cap *ipsec_cap = &service_capability->ipsec_cap;
	struct dev_ipsec_svc_cap *ipsec_srvcap = &ipsec_cap->dev_ipsec_cap;
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;

	func_cap->childc_number += ipsec_srvcap->max_sactxs;
	func_cap->childc_basic_size = GET_MAX(CQM_CHILDC_SIZE_256,
					      func_cap->childc_basic_size);
	func_cap->scqc_number += ipsec_srvcap->max_cqs;
	func_cap->scqc_basic_size = GET_MAX(CQM_SCQC_SIZE_64,
					    func_cap->scqc_basic_size);
	func_cap->scqc_alloc_static = true;
	cqm_info(handle->dev_hdl, "Cap init: ipsec is valid\n");
	cqm_info(handle->dev_hdl, "Cap init: ipsec 0x%x, childc %d, scqc 0x%x, scqc_bsize %d\n",
		 ipsec_srvcap->max_sactxs, func_cap->childc_basic_size,
		 ipsec_srvcap->max_cqs, func_cap->scqc_basic_size);
}

static void cqm_service_capability_init_virtio(struct tag_cqm_handle *cqm_handle, void *pra)
{
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;
	struct service_cap *service_capability = (struct service_cap *)pra;

	cqm_info(handle->dev_hdl, "Cap init: virtio is valid\n");
	/* If the entry size is greater than the cache line (256 bytes),
	 * align the entries by cache line.
	 */
	cqm_handle->func_capability.xid2cid_number +=
	    (CQM_XID2CID_VIRTIO_NUM * service_capability->virtio_vq_size) / CQM_CHIP_CACHELINE;
	cqm_handle->func_capability.xid2cid_basic_size = CQM_CHIP_CACHELINE;
}

static void cqm_service_capability_init_ppa(struct tag_cqm_handle *cqm_handle, void *pra)
{
	struct tag_cqm_func_capability *func_cap = &cqm_handle->func_capability;
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;
	struct service_cap *service_capability = (struct service_cap *)pra;
	struct ppa_service_cap *ppa_cap = &service_capability->ppa_cap;

	cqm_info(handle->dev_hdl, "Cap init: ppa is valid\n");
	func_cap->hash_basic_size = CQM_HASH_BUCKET_SIZE_64;
	func_cap->qpc_alloc_static = true;
	func_cap->pagesize_reorder = CQM_PPA_PAGESIZE_ORDER;
	func_cap->qpc_basic_size = GET_MAX(ppa_cap->pctx_sz,
					   func_cap->qpc_basic_size);
}

struct cqm_srv_cap_init serv_cap_init_list[] = {
	{CQM_SERVICE_T_NIC, cqm_service_capability_init_nic},
	{CQM_SERVICE_T_OVS, cqm_service_capability_init_ovs},
	{CQM_SERVICE_T_ROCE, cqm_service_capability_init_roce},
	{CQM_SERVICE_T_TOE, cqm_service_capability_init_toe},
	{CQM_SERVICE_T_IOE, cqm_service_capability_init_ioe},
	{CQM_SERVICE_T_FC, cqm_service_capability_init_fc},
	{CQM_SERVICE_T_VBS, cqm_service_capability_init_vbs},
	{CQM_SERVICE_T_IPSEC, cqm_service_capability_init_ipsec},
	{CQM_SERVICE_T_VIRTIO, cqm_service_capability_init_virtio},
	{CQM_SERVICE_T_PPA, cqm_service_capability_init_ppa},
};

static void cqm_service_capability_init(struct tag_cqm_handle *cqm_handle,
					struct service_cap *service_capability)
{
	u32 list_size = ARRAY_SIZE(serv_cap_init_list);
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;
	u32 i;

	for (i = 0; i < CQM_SERVICE_T_MAX; i++) {
		cqm_handle->service[i].valid = false;
		cqm_handle->service[i].has_register = false;
		cqm_handle->service[i].buf_order = 0;
	}

	cqm_service_valid_init(cqm_handle, service_capability);

	cqm_info(handle->dev_hdl, "Cap init: service type %d\n",
		 service_capability->chip_svc_type);

	for (i = 0; i < list_size; i++) {
		if (cqm_handle->service[serv_cap_init_list[i].service_type].valid &&
		    serv_cap_init_list[i].serv_cap_proc) {
			serv_cap_init_list[i].serv_cap_proc(cqm_handle,
							    (void *)service_capability);
		}
	}
}

s32 cqm_get_fake_func_type(struct tag_cqm_handle *cqm_handle)
{
	struct tag_cqm_func_capability *func_cap = &cqm_handle->func_capability;
	u32 parent_func, child_func_start, child_func_number, i;
	u32 idx = cqm_handle->func_attribute.func_global_idx;

	/* Currently, only one set of fake configurations is implemented.
	 * fake_cfg_number = 1
	 */
	for (i = 0; i < func_cap->fake_cfg_number; i++) {
		parent_func = func_cap->fake_cfg[i].parent_func;
		child_func_start = func_cap->fake_cfg[i].child_func_start;
		child_func_number = func_cap->fake_cfg[i].child_func_number;

		if (idx == parent_func) {
			return CQM_FAKE_FUNC_PARENT;
		} else if ((idx >= child_func_start) &&
			   (idx < (child_func_start + child_func_number))) {
			return CQM_FAKE_FUNC_CHILD_CONFLICT;
		}
	}

	return CQM_FAKE_FUNC_NORMAL;
}

s32 cqm_get_child_func_start(struct tag_cqm_handle *cqm_handle)
{
	struct tag_cqm_func_capability *func_cap = &cqm_handle->func_capability;
	struct hinic3_func_attr *func_attr = &cqm_handle->func_attribute;
	u32 i;

	/* Currently, only one set of fake configurations is implemented.
	 * fake_cfg_number = 1
	 */
	for (i = 0; i < func_cap->fake_cfg_number; i++) {
		if (func_attr->func_global_idx ==
		    func_cap->fake_cfg[i].parent_func)
			return (s32)(func_cap->fake_cfg[i].child_func_start);
	}

	return CQM_FAIL;
}

s32 cqm_get_child_func_number(struct tag_cqm_handle *cqm_handle)
{
	struct tag_cqm_func_capability *func_cap = &cqm_handle->func_capability;
	struct hinic3_func_attr *func_attr = &cqm_handle->func_attribute;
	u32 i;

	for (i = 0; i < func_cap->fake_cfg_number; i++) {
		if (func_attr->func_global_idx ==
		    func_cap->fake_cfg[i].parent_func)
			return (s32)(func_cap->fake_cfg[i].child_func_number);
	}

	return CQM_FAIL;
}

/* Set func_type in fake_cqm_handle to ppf, pf, or vf. */
static void cqm_set_func_type(struct tag_cqm_handle *cqm_handle)
{
	u32 idx = cqm_handle->func_attribute.func_global_idx;

	if (idx == 0)
		cqm_handle->func_attribute.func_type = CQM_PPF;
	else if (idx < CQM_MAX_PF_NUM)
		cqm_handle->func_attribute.func_type = CQM_PF;
	else
		cqm_handle->func_attribute.func_type = CQM_VF;
}

static void cqm_lb_fake_mode_init(struct hinic3_hwdev *handle, struct service_cap *svc_cap)
{
	struct tag_cqm_handle *cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	struct tag_cqm_func_capability *func_cap = &cqm_handle->func_capability;
	struct tag_cqm_fake_cfg *cfg = func_cap->fake_cfg;

	func_cap->lb_mode = svc_cap->lb_mode;

	/* Initializing the LB Mode */
	if (func_cap->lb_mode == CQM_LB_MODE_NORMAL)
		func_cap->smf_pg = 0;
	else
		func_cap->smf_pg = svc_cap->smf_pg;

	/* Initializing the FAKE Mode */
	if (svc_cap->fake_vf_num == 0) {
		func_cap->fake_cfg_number = 0;
		func_cap->fake_func_type = CQM_FAKE_FUNC_NORMAL;
		func_cap->fake_vf_qpc_number = 0;
	} else {
		func_cap->fake_cfg_number = 1;

		/* When configuring fake mode, ensure that the parent function
		 * cannot be contained in the child function; otherwise, the
		 * system will be initialized repeatedly. The following
		 * configuration is used to verify the OVS fake configuration on
		 * the FPGA.
		 */
		cfg[0].parent_func = cqm_handle->func_attribute.port_to_port_idx;
		cfg[0].child_func_start = svc_cap->fake_vf_start_id;
		cfg[0].child_func_number = svc_cap->fake_vf_num_cfg;

		func_cap->fake_func_type = (u32)cqm_get_fake_func_type(cqm_handle);
		func_cap->fake_vf_qpc_number = svc_cap->fake_vf_max_pctx;
	}

	cqm_info(handle->dev_hdl, "Cap init: lb_mode=%u\n", func_cap->lb_mode);
	cqm_info(handle->dev_hdl, "Cap init: smf_pg=%u\n", func_cap->smf_pg);
	cqm_info(handle->dev_hdl, "Cap init: fake_func_type=%u\n", func_cap->fake_func_type);
	cqm_info(handle->dev_hdl, "Cap init: fake_cfg_number=%u\n", func_cap->fake_cfg_number);
}

static int cqm_capability_init_bloomfilter(struct hinic3_hwdev *handle)
{
	struct tag_cqm_handle *cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	struct tag_cqm_func_capability *func_cap = &cqm_handle->func_capability;
	struct service_cap *service_capability = &handle->cfg_mgmt->svc_cap;

	func_cap->bloomfilter_enable = service_capability->bloomfilter_en;
	cqm_info(handle->dev_hdl, "Cap init: bloomfilter_enable %u (1: enable; 0: disable)\n",
		 func_cap->bloomfilter_enable);

	if (func_cap->bloomfilter_enable != 0) {
		func_cap->bloomfilter_length = service_capability->bfilter_len;
		func_cap->bloomfilter_addr = service_capability->bfilter_start_addr;
		if (func_cap->bloomfilter_length != 0 &&
		    !cqm_check_align(func_cap->bloomfilter_length)) {
			cqm_err(handle->dev_hdl, "Cap bloomfilter len %u is not the power of 2\n",
				func_cap->bloomfilter_length);

			return CQM_FAIL;
		}
	}

	cqm_info(handle->dev_hdl, "Cap init: bloomfilter_length 0x%x, bloomfilter_addr 0x%x\n",
		 func_cap->bloomfilter_length, func_cap->bloomfilter_addr);

	return 0;
}

static void cqm_capability_init_part_cap(struct hinic3_hwdev *handle)
{
	struct tag_cqm_handle *cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	struct tag_cqm_func_capability *func_cap = &cqm_handle->func_capability;
	struct service_cap *service_capability = &handle->cfg_mgmt->svc_cap;

	func_cap->flow_table_based_conn_number = service_capability->max_connect_num;
	func_cap->flow_table_based_conn_cache_number = service_capability->max_stick2cache_num;
	cqm_info(handle->dev_hdl, "Cap init: cfg max_conn_num 0x%x, max_cache_conn_num 0x%x\n",
		 func_cap->flow_table_based_conn_number,
		 func_cap->flow_table_based_conn_cache_number);

	func_cap->qpc_reserved = 0;
	func_cap->qpc_reserved_back = 0;
	func_cap->mpt_reserved = 0;
	func_cap->scq_reserved = 0;
	func_cap->srq_reserved = 0;
	func_cap->qpc_alloc_static = false;
	func_cap->scqc_alloc_static = false;

	func_cap->l3i_number = 0;
	func_cap->l3i_basic_size = CQM_L3I_SIZE_8;

	func_cap->xid_alloc_mode = true; /* xid alloc do not reuse */
	func_cap->gpa_check_enable = true;
}

static int cqm_capability_init_timer(struct hinic3_hwdev *handle)
{
	struct tag_cqm_handle *cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	struct service_cap *service_capability = &handle->cfg_mgmt->svc_cap;
	struct hinic3_func_attr *func_attr = &cqm_handle->func_attribute;
	struct tag_cqm_func_capability *func_cap = &cqm_handle->func_capability;
	u32 total_timer_num = 0;
	int err;

	/* Initializes the PPF capabilities: include timer, pf, vf. */
	if (func_attr->func_type == CQM_PPF && service_capability->timer_en) {
		func_cap->pf_num = service_capability->pf_num;
		func_cap->pf_id_start = service_capability->pf_id_start;
		func_cap->vf_num = service_capability->vf_num;
		func_cap->vf_id_start = service_capability->vf_id_start;
		cqm_info(handle->dev_hdl, "Cap init: total function num 0x%x\n",
			 service_capability->host_total_function);
		cqm_info(handle->dev_hdl,
			 "Cap init: pf_num 0x%x, pf_start 0x%x, vf_num 0x%x, vf_start 0x%x\n",
			 func_cap->pf_num, func_cap->pf_id_start,
			 func_cap->vf_num, func_cap->vf_id_start);

		err = hinic3_get_ppf_timer_cfg(handle);
		if (err != 0)
			return err;

		func_cap->timer_pf_num =  service_capability->timer_pf_num;
		func_cap->timer_pf_id_start = service_capability->timer_pf_id_start;
		func_cap->timer_vf_num = service_capability->timer_vf_num;
		func_cap->timer_vf_id_start = service_capability->timer_vf_id_start;
		cqm_info(handle->dev_hdl,
			 "timer init: pf_num 0x%x, pf_start 0x%x, vf_num 0x%x, vf_start 0x%x\n",
			 func_cap->timer_pf_num, func_cap->timer_pf_id_start,
			 func_cap->timer_vf_num, func_cap->timer_vf_id_start);

		total_timer_num = func_cap->timer_pf_num + func_cap->timer_vf_num;
		if (IS_SLAVE_HOST(handle)) {
			total_timer_num *= CQM_TIMER_NUM_MULTI;
			cqm_info(handle->dev_hdl,
				 "timer init: need double tw resources, total_timer_num=0x%x\n",
				 total_timer_num);
		}
	}

	func_cap->timer_enable = service_capability->timer_en;
	cqm_info(handle->dev_hdl, "Cap init: timer_enable %u (1: enable; 0: disable)\n",
		 func_cap->timer_enable);

	func_cap->timer_number = CQM_TIMER_ALIGN_SCALE_NUM * total_timer_num;
	func_cap->timer_basic_size = CQM_TIMER_SIZE_32;

	return 0;
}

static void cqm_capability_init_cap_print(struct hinic3_hwdev *handle)
{
	struct tag_cqm_handle *cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	struct tag_cqm_func_capability *func_cap = &cqm_handle->func_capability;
	struct service_cap *service_capability = &handle->cfg_mgmt->svc_cap;

	func_cap->ft_enable = service_capability->sf_svc_attr.ft_en;
	func_cap->rdma_enable = service_capability->sf_svc_attr.rdma_en;

	cqm_info(handle->dev_hdl, "Cap init: pagesize_reorder %u\n", func_cap->pagesize_reorder);
	cqm_info(handle->dev_hdl, "Cap init: xid_alloc_mode %d, gpa_check_enable %d\n",
		 func_cap->xid_alloc_mode, func_cap->gpa_check_enable);
	cqm_info(handle->dev_hdl, "Cap init: qpc_alloc_mode %d, scqc_alloc_mode %d\n",
		 func_cap->qpc_alloc_static, func_cap->scqc_alloc_static);
	cqm_info(handle->dev_hdl, "Cap init: hash_number 0x%x\n", func_cap->hash_number);
	cqm_info(handle->dev_hdl, "Cap init: qpc_num 0x%x, qpc_rsvd 0x%x, qpc_basic_size 0x%x\n",
		 func_cap->qpc_number, func_cap->qpc_reserved, func_cap->qpc_basic_size);
	cqm_info(handle->dev_hdl, "Cap init: scqc_num 0x%x, scqc_rsvd 0x%x, scqc_basic 0x%x\n",
		 func_cap->scqc_number, func_cap->scq_reserved, func_cap->scqc_basic_size);
	cqm_info(handle->dev_hdl, "Cap init: srqc_num 0x%x, srqc_rsvd 0x%x, srqc_basic 0x%x\n",
		 func_cap->srqc_number, func_cap->srq_reserved, func_cap->srqc_basic_size);
	cqm_info(handle->dev_hdl, "Cap init: mpt_number 0x%x, mpt_reserved 0x%x\n",
		 func_cap->mpt_number, func_cap->mpt_reserved);
	cqm_info(handle->dev_hdl, "Cap init: gid_number 0x%x, lun_number 0x%x\n",
		 func_cap->gid_number, func_cap->lun_number);
	cqm_info(handle->dev_hdl, "Cap init: taskmap_number 0x%x, l3i_number 0x%x\n",
		 func_cap->taskmap_number, func_cap->l3i_number);
	cqm_info(handle->dev_hdl, "Cap init: timer_number 0x%x, childc_number 0x%x\n",
		 func_cap->timer_number, func_cap->childc_number);
	cqm_info(handle->dev_hdl, "Cap init: childc_basic_size 0x%x\n",
		 func_cap->childc_basic_size);
	cqm_info(handle->dev_hdl, "Cap init: xid2cid_number 0x%x, reorder_number 0x%x\n",
		 func_cap->xid2cid_number, func_cap->reorder_number);
	cqm_info(handle->dev_hdl, "Cap init: ft_enable %d, rdma_enable %d\n",
		 func_cap->ft_enable, func_cap->rdma_enable);
}

/**
 * cqm_capability_init - Initializes the function and service capabilities of the CQM.
 *                       Information needs to be read from the configuration management module.
 * @ex_handle: device pointer that represents the PF
 */
s32 cqm_capability_init(void *ex_handle)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_cqm_handle *cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	struct service_cap *service_capability = &handle->cfg_mgmt->svc_cap;
	struct hinic3_func_attr *func_attr = &cqm_handle->func_attribute;
	struct tag_cqm_func_capability *func_cap = &cqm_handle->func_capability;
	int err = 0;

	err = cqm_capability_init_timer(handle);
	if (err != 0)
		goto out;

	err = cqm_capability_init_bloomfilter(handle);
	if (err != 0)
		goto out;

	cqm_capability_init_part_cap(handle);

	cqm_lb_fake_mode_init(handle, service_capability);

	cqm_service_capability_init(cqm_handle, service_capability);

	cqm_test_mode_init(cqm_handle, service_capability);

	cqm_service_capability_update(cqm_handle);

	cqm_capability_init_cap_print(handle);

	return CQM_SUCCESS;

out:
	if (func_attr->func_type == CQM_PPF)
		func_cap->timer_enable = 0;

	return err;
}

static void cqm_fake_uninit(struct tag_cqm_handle *cqm_handle)
{
	u32 i;

	if (cqm_handle->func_capability.fake_func_type !=
	    CQM_FAKE_FUNC_PARENT)
		return;

	for (i = 0; i < CQM_FAKE_FUNC_MAX; i++) {
		kfree(cqm_handle->fake_cqm_handle[i]);
		cqm_handle->fake_cqm_handle[i] = NULL;
	}
}

static void set_fake_cqm_attr(struct hinic3_hwdev *handle, struct tag_cqm_handle *fake_cqm_handle,
			      s32 child_func_start, u32 i)
{
	struct tag_cqm_func_capability *func_cap = NULL;
	struct hinic3_func_attr *func_attr = NULL;
	struct service_cap *cap = &handle->cfg_mgmt->svc_cap;

	func_attr = &fake_cqm_handle->func_attribute;
	func_cap = &fake_cqm_handle->func_capability;
	func_attr->func_global_idx = (u16)(child_func_start + i);
	cqm_set_func_type(fake_cqm_handle);
	func_cap->fake_func_type = CQM_FAKE_FUNC_CHILD;
	cqm_info(handle->dev_hdl, "Fake func init: function[%u] type %d(0:PF,1:VF,2:PPF)\n",
		 func_attr->func_global_idx, func_attr->func_type);

	func_cap->qpc_number = cap->fake_vf_max_pctx;
	func_cap->qpc_number = GET_MIN(CQM_MAX_QPC_NUM, func_cap->qpc_number);
	func_cap->hash_number = cap->fake_vf_max_pctx;
	func_cap->qpc_reserved = cap->fake_vf_max_pctx;

	if (cap->fake_vf_bfilter_len != 0) {
		func_cap->bloomfilter_enable = true;
		func_cap->bloomfilter_addr = cap->fake_vf_bfilter_start_addr +
			cap->fake_vf_bfilter_len * i;
		func_cap->bloomfilter_length = cap->fake_vf_bfilter_len;
	}
}

/**
 * cqm_fake_init - When the fake VF mode is supported, the CQM handles of the fake VFs
 *                 need to be copied.
 * @cqm_handle: Parent CQM handle of the current PF
 */
static s32 cqm_fake_init(struct tag_cqm_handle *cqm_handle)
{
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;
	struct tag_cqm_handle *fake_cqm_handle = NULL;
	struct tag_cqm_func_capability *func_cap = NULL;
	s32 child_func_start, child_func_number;
	u32 i;

	func_cap = &cqm_handle->func_capability;
	if (func_cap->fake_func_type != CQM_FAKE_FUNC_PARENT)
		return CQM_SUCCESS;

	child_func_start = cqm_get_child_func_start(cqm_handle);
	if (child_func_start == CQM_FAIL) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(child_func_start));
		return CQM_FAIL;
	}

	child_func_number = cqm_get_child_func_number(cqm_handle);
	if (child_func_number == CQM_FAIL) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(child_func_number));
		return CQM_FAIL;
	}

	for (i = 0; i < (u32)child_func_number; i++) {
		fake_cqm_handle = kmalloc(sizeof(*fake_cqm_handle), GFP_KERNEL | __GFP_ZERO);
		if (!fake_cqm_handle)
			goto err;

		/* Copy the attributes of the parent CQM handle to the child CQM
		 * handle and modify the values of function.
		 */
		memcpy(fake_cqm_handle, cqm_handle, sizeof(struct tag_cqm_handle));
		set_fake_cqm_attr(handle, fake_cqm_handle, child_func_start, i);

		fake_cqm_handle->parent_cqm_handle = cqm_handle;
		cqm_handle->fake_cqm_handle[i] = fake_cqm_handle;
	}

	return CQM_SUCCESS;

err:
	cqm_fake_uninit(cqm_handle);
	return CQM_FAIL;
}

static void cqm_fake_mem_uninit(struct tag_cqm_handle *cqm_handle)
{
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;
	struct tag_cqm_handle *fake_cqm_handle = NULL;
	s32 child_func_number;
	u32 i;

	if (cqm_handle->func_capability.fake_func_type !=
	    CQM_FAKE_FUNC_PARENT)
		return;

	child_func_number = cqm_get_child_func_number(cqm_handle);
	if (child_func_number == CQM_FAIL) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(child_func_number));
		return;
	}

	for (i = 0; i < (u32)child_func_number; i++) {
		fake_cqm_handle = cqm_handle->fake_cqm_handle[i];

		cqm_object_table_uninit(fake_cqm_handle);
		cqm_bitmap_uninit(fake_cqm_handle);
		cqm_cla_uninit(fake_cqm_handle, CQM_BAT_ENTRY_MAX);
		cqm_bat_uninit(fake_cqm_handle);
	}
}

/**
 * cqm_fake_mem_init - Initialize resources of the extended fake function
 * @cqm_handle: Parent CQM handle of the current PF
 */
static s32 cqm_fake_mem_init(struct tag_cqm_handle *cqm_handle)
{
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;
	struct tag_cqm_handle *fake_cqm_handle = NULL;
	s32 child_func_number;
	u32 i;

	if (cqm_handle->func_capability.fake_func_type !=
	    CQM_FAKE_FUNC_PARENT)
		return CQM_SUCCESS;

	child_func_number = cqm_get_child_func_number(cqm_handle);
	if (child_func_number == CQM_FAIL) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(child_func_number));
		return CQM_FAIL;
	}

	for (i = 0; i < (u32)child_func_number; i++) {
		fake_cqm_handle = cqm_handle->fake_cqm_handle[i];
		snprintf(fake_cqm_handle->name, VRAM_NAME_MAX_LEN,
			 "%s%s%02u", cqm_handle->name,
			 VRAM_CQM_FAKE_MEM_BASE, i);

		if (cqm_bat_init(fake_cqm_handle) != CQM_SUCCESS) {
			cqm_err(handle->dev_hdl,
				CQM_FUNCTION_FAIL(cqm_bat_init));
			goto err;
		}

		if (cqm_cla_init(fake_cqm_handle) != CQM_SUCCESS) {
			cqm_err(handle->dev_hdl,
				CQM_FUNCTION_FAIL(cqm_cla_init));
			goto err;
		}

		if (cqm_bitmap_init(fake_cqm_handle) != CQM_SUCCESS) {
			cqm_err(handle->dev_hdl,
				CQM_FUNCTION_FAIL(cqm_bitmap_init));
			goto err;
		}

		if (cqm_object_table_init(fake_cqm_handle) != CQM_SUCCESS) {
			cqm_err(handle->dev_hdl,
				CQM_FUNCTION_FAIL(cqm_object_table_init));
			goto err;
		}
	}

	return CQM_SUCCESS;

err:
	cqm_fake_mem_uninit(cqm_handle);
	return CQM_FAIL;
}

/**
 * cqm_mem_init - Initialize CQM memory, including tables at different levels.
 * @ex_handle: device pointer that represents the PF
 */
s32 cqm_mem_init(void *ex_handle)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_cqm_handle *cqm_handle = NULL;

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	snprintf(cqm_handle->name, VRAM_NAME_MAX_LEN,
		 "%s%02u", VRAM_CQM_GLB_FUNC_BASE,
		 hinic3_global_func_id(handle));

	if (cqm_fake_init(cqm_handle) != CQM_SUCCESS) {
		cqm_err(handle->dev_hdl, CQM_FUNCTION_FAIL(cqm_fake_init));
		return CQM_FAIL;
	}

	if (cqm_fake_mem_init(cqm_handle) != CQM_SUCCESS) {
		cqm_err(handle->dev_hdl, CQM_FUNCTION_FAIL(cqm_fake_mem_init));
		goto err1;
	}

	if (cqm_bat_init(cqm_handle) != CQM_SUCCESS) {
		cqm_err(handle->dev_hdl, CQM_FUNCTION_FAIL(cqm_bat_init));
		goto err2;
	}

	if (cqm_cla_init(cqm_handle) != CQM_SUCCESS) {
		cqm_err(handle->dev_hdl, CQM_FUNCTION_FAIL(cqm_cla_init));
		goto err3;
	}

	if (cqm_bitmap_init(cqm_handle) != CQM_SUCCESS) {
		cqm_err(handle->dev_hdl, CQM_FUNCTION_FAIL(cqm_bitmap_init));
		goto err4;
	}

	if (cqm_object_table_init(cqm_handle) != CQM_SUCCESS) {
		cqm_err(handle->dev_hdl,
			CQM_FUNCTION_FAIL(cqm_object_table_init));
		goto err5;
	}

	return CQM_SUCCESS;

err5:
	cqm_bitmap_uninit(cqm_handle);
err4:
	cqm_cla_uninit(cqm_handle, CQM_BAT_ENTRY_MAX);
err3:
	cqm_bat_uninit(cqm_handle);
err2:
	cqm_fake_mem_uninit(cqm_handle);
err1:
	cqm_fake_uninit(cqm_handle);
	return CQM_FAIL;
}

/**
 * cqm_mem_uninit - Deinitialize CQM memory, including tables at different levels
 * @ex_handle: device pointer that represents the PF
 */
void cqm_mem_uninit(void *ex_handle)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_cqm_handle *cqm_handle = NULL;

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);

	cqm_object_table_uninit(cqm_handle);
	cqm_bitmap_uninit(cqm_handle);
	cqm_cla_uninit(cqm_handle, CQM_BAT_ENTRY_MAX);
	cqm_bat_uninit(cqm_handle);
	cqm_fake_mem_uninit(cqm_handle);
	cqm_fake_uninit(cqm_handle);
}

/**
 * cqm_event_init - Initialize CQM event callback
 * @ex_handle: device pointer that represents the PF
 */
s32 cqm_event_init(void *ex_handle)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;

	/* Registers the CEQ and AEQ callback functions. */
	if (hinic3_ceq_register_cb(ex_handle, ex_handle, HINIC3_NON_L2NIC_SCQ,
				   cqm_scq_callback) != CHIPIF_SUCCESS) {
		cqm_err(handle->dev_hdl, "Event: fail to register scq callback\n");
		return CQM_FAIL;
	}

	if (hinic3_ceq_register_cb(ex_handle, ex_handle, HINIC3_NON_L2NIC_ECQ,
				   cqm_ecq_callback) != CHIPIF_SUCCESS) {
		cqm_err(handle->dev_hdl, "Event: fail to register ecq callback\n");
		goto err1;
	}

	if (hinic3_ceq_register_cb(ex_handle, ex_handle, HINIC3_NON_L2NIC_NO_CQ_EQ,
				   cqm_nocq_callback) != CHIPIF_SUCCESS) {
		cqm_err(handle->dev_hdl, "Event: fail to register nocq callback\n");
		goto err2;
	}

	if (hinic3_aeq_register_swe_cb(ex_handle, ex_handle, HINIC3_STATEFUL_EVENT,
				       cqm_aeq_callback) != CHIPIF_SUCCESS) {
		cqm_err(handle->dev_hdl, "Event: fail to register aeq callback\n");
		goto err3;
	}

	return CQM_SUCCESS;

err3:
	hinic3_ceq_unregister_cb(ex_handle, HINIC3_NON_L2NIC_NO_CQ_EQ);
err2:
	hinic3_ceq_unregister_cb(ex_handle, HINIC3_NON_L2NIC_ECQ);
err1:
	hinic3_ceq_unregister_cb(ex_handle, HINIC3_NON_L2NIC_SCQ);
	return CQM_FAIL;
}

/**
 * cqm_event_uninit - Deinitialize CQM event callback
 * @ex_handle: device pointer that represents the PF
 */
void cqm_event_uninit(void *ex_handle)
{
	hinic3_aeq_unregister_swe_cb(ex_handle, HINIC3_STATEFUL_EVENT);
	hinic3_ceq_unregister_cb(ex_handle, HINIC3_NON_L2NIC_NO_CQ_EQ);
	hinic3_ceq_unregister_cb(ex_handle, HINIC3_NON_L2NIC_ECQ);
	hinic3_ceq_unregister_cb(ex_handle, HINIC3_NON_L2NIC_SCQ);
}

/**
 * cqm_scq_callback - CQM module callback processing for the ceq, which processes NON_L2NIC_SCQ
 * @ex_handle: device pointer that represents the PF
 * @ceqe_data: CEQE data
 */
void cqm_scq_callback(void *ex_handle, u32 ceqe_data)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_service_register_template *service_template = NULL;
	struct tag_cqm_handle *cqm_handle = NULL;
	struct tag_cqm_service *service = NULL;
	struct tag_cqm_queue *cqm_queue = NULL;
	struct tag_cqm_object *obj = NULL;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: scq_callback_ex_handle is null\n", __func__);
		return;
	}

	atomic_inc(&handle->hw_stats.cqm_stats.cqm_scq_callback_cnt);

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	if (unlikely(!cqm_handle)) {
		pr_err("[CQM]%s: scq_callback_cqm_handle is null\n", __func__);
		return;
	}

	cqm_dbg("Event: %s, ceqe_data=0x%x\n", __func__, ceqe_data);
	obj = cqm_object_get(ex_handle, CQM_OBJECT_NONRDMA_SCQ,
			     CQM_CQN_FROM_CEQE(ceqe_data), true);
	if (unlikely(!obj)) {
		pr_err("[CQM]%s: scq_callback_obj is null\n", __func__);
		return;
	}

	if (unlikely(obj->service_type >= CQM_SERVICE_T_MAX)) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(obj->service_type));
		cqm_object_put(obj);
		return;
	}

	service = &cqm_handle->service[obj->service_type];
	service_template = &service->service_template;
	if (service_template->shared_cq_ceq_callback) {
		cqm_queue = (struct tag_cqm_queue *)obj;
		service_template->shared_cq_ceq_callback(service_template->service_handle,
							 CQM_CQN_FROM_CEQE(ceqe_data),
							 cqm_queue->priv);
	} else {
		cqm_err(handle->dev_hdl, CQM_PTR_NULL(shared_cq_ceq_callback));
	}

	cqm_object_put(obj);
}

/**
 * cqm_ecq_callback - CQM module callback processing for the ceq, which processes NON_L2NIC_ECQ.
 * @ex_handle: device pointer that represents the PF
 * @ceqe_data: CEQE data
 */
void cqm_ecq_callback(void *ex_handle, u32 ceqe_data)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_service_register_template *service_template = NULL;
	struct tag_cqm_handle *cqm_handle = NULL;
	struct tag_cqm_service *service = NULL;
	struct tag_cqm_qpc_mpt *qpc = NULL;
	struct tag_cqm_object *obj = NULL;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: ecq_callback_ex_handle is null\n", __func__);
		return;
	}

	atomic_inc(&handle->hw_stats.cqm_stats.cqm_ecq_callback_cnt);

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	if (unlikely(!cqm_handle)) {
		pr_err("[CQM]%s: ecq_callback_cqm_handle is null\n", __func__);
		return;
	}

	obj = cqm_object_get(ex_handle, CQM_OBJECT_SERVICE_CTX,
			     CQM_XID_FROM_CEQE(ceqe_data), true);
	if (unlikely(!obj)) {
		pr_err("[CQM]%s: ecq_callback_obj is null\n", __func__);
		return;
	}

	if (unlikely(obj->service_type >= CQM_SERVICE_T_MAX)) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(obj->service_type));
		cqm_object_put(obj);
		return;
	}

	service = &cqm_handle->service[obj->service_type];
	service_template = &service->service_template;
	if (service_template->embedded_cq_ceq_callback) {
		qpc = (struct tag_cqm_qpc_mpt *)obj;
		service_template->embedded_cq_ceq_callback(service_template->service_handle,
							   CQM_XID_FROM_CEQE(ceqe_data),
							   qpc->priv);
	} else {
		cqm_err(handle->dev_hdl,
			CQM_PTR_NULL(embedded_cq_ceq_callback));
	}

	cqm_object_put(obj);
}

/**
 * cqm_nocq_callback - CQM module callback processing for the ceq,
 *                     which processes NON_L2NIC_NO_CQ_EQ
 * @ex_handle: device pointer that represents the PF
 * @ceqe_data: CEQE data
 */
void cqm_nocq_callback(void *ex_handle, u32 ceqe_data)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_service_register_template *service_template = NULL;
	struct tag_cqm_handle *cqm_handle = NULL;
	struct tag_cqm_service *service = NULL;
	struct tag_cqm_qpc_mpt *qpc = NULL;
	struct tag_cqm_object *obj = NULL;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: nocq_callback_ex_handle is null\n", __func__);
		return;
	}

	atomic_inc(&handle->hw_stats.cqm_stats.cqm_nocq_callback_cnt);

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	if (unlikely(!cqm_handle)) {
		pr_err("[CQM]%s: nocq_callback_cqm_handle is null\n", __func__);
		return;
	}

	obj = cqm_object_get(ex_handle, CQM_OBJECT_SERVICE_CTX,
			     CQM_XID_FROM_CEQE(ceqe_data), true);
	if (unlikely(!obj)) {
		pr_err("[CQM]%s: nocq_callback_obj is null\n", __func__);
		return;
	}

	if (unlikely(obj->service_type >= CQM_SERVICE_T_MAX)) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(obj->service_type));
		cqm_object_put(obj);
		return;
	}

	service = &cqm_handle->service[obj->service_type];
	service_template = &service->service_template;
	if (service_template->no_cq_ceq_callback) {
		qpc = (struct tag_cqm_qpc_mpt *)obj;
		service_template->no_cq_ceq_callback(service_template->service_handle,
						     CQM_XID_FROM_CEQE(ceqe_data),
						     CQM_QID_FROM_CEQE(ceqe_data),
						     qpc->priv);
	} else {
		cqm_err(handle->dev_hdl, CQM_PTR_NULL(no_cq_ceq_callback));
	}

	cqm_object_put(obj);
}

static u32 cqm_aeq_event2type(u8 event)
{
	u32 service_type;

	/* Distributes events to different service modules
	 * based on the event type.
	 */
	if (event < CQM_AEQ_BASE_T_ROCE)
		service_type = CQM_SERVICE_T_NIC;
	else if (event < CQM_AEQ_BASE_T_FC)
		service_type = CQM_SERVICE_T_ROCE;
	else if (event < CQM_AEQ_BASE_T_IOE)
		service_type = CQM_SERVICE_T_FC;
	else if (event < CQM_AEQ_BASE_T_TOE)
		service_type = CQM_SERVICE_T_IOE;
	else if (event < CQM_AEQ_BASE_T_VBS)
		service_type = CQM_SERVICE_T_TOE;
	else if (event < CQM_AEQ_BASE_T_IPSEC)
		service_type = CQM_SERVICE_T_VBS;
	else if (event < CQM_AEQ_BASE_T_MAX)
		service_type = CQM_SERVICE_T_IPSEC;
	else
		service_type = CQM_SERVICE_T_MAX;

	return service_type;
}

/**
 * cqm_aeq_callback - CQM module callback processing for the aeq
 * @ex_handle: device pointer that represents the PF
 * @event: QEQ event
 * @data: callback private data
 */
u8 cqm_aeq_callback(void *ex_handle, u8 event, u8 *data)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_service_register_template *service_template = NULL;
	struct tag_cqm_handle *cqm_handle = NULL;
	struct tag_cqm_service *service = NULL;
	u8 event_level = FAULT_LEVEL_MAX;
	u32 service_type;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: aeq_callback_ex_handle is null\n", __func__);
		return event_level;
	}

	atomic_inc(&handle->hw_stats.cqm_stats.cqm_aeq_callback_cnt[event]);

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	if (unlikely(!cqm_handle)) {
		pr_err("[CQM]%s: aeq_callback_cqm_handle is null\n", __func__);
		return event_level;
	}

	/* Distributes events to different service modules
	 * based on the event type.
	 */
	service_type = cqm_aeq_event2type(event);
	if (service_type == CQM_SERVICE_T_MAX) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(event));
		return event_level;
	}

	service = &cqm_handle->service[service_type];
	service_template = &service->service_template;

	if (!service_template->aeq_level_callback)
		cqm_err(handle->dev_hdl,
			"Event: service_type %u aeq_level_callback unregistered, event %u\n",
			service_type, event);
	else
		event_level =
			service_template->aeq_level_callback(service_template->service_handle,
							     event, data);

	if (!service_template->aeq_callback)
		cqm_err(handle->dev_hdl, "Event: service_type %u aeq_callback unregistered\n",
			service_type);
	else
		service_template->aeq_callback(service_template->service_handle,
					       event, data);

	return event_level;
}

/**
 * cqm_service_register - Callback template for the service driver to register with the CQM
 * @ex_handle: device pointer that represents the PF
 * @service_template: CQM service template
 */
s32 cqm_service_register(void *ex_handle, struct tag_service_register_template *service_template)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_cqm_handle *cqm_handle = NULL;
	struct tag_cqm_service *service = NULL;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: ex_handle is null\n", __func__);
		return CQM_FAIL;
	}

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	if (unlikely(!cqm_handle)) {
		pr_err("[CQM]%s: cqm_handle is null\n", __func__);
		return CQM_FAIL;
	}
	if (unlikely(!service_template)) {
		pr_err("[CQM]%s: service_template is null\n", __func__);
		return CQM_FAIL;
	}

	if (service_template->service_type >= CQM_SERVICE_T_MAX) {
		cqm_err(handle->dev_hdl,
			CQM_WRONG_VALUE(service_template->service_type));
		return CQM_FAIL;
	}
	service = &cqm_handle->service[service_template->service_type];
	if (!service->valid) {
		cqm_err(handle->dev_hdl, "Service register: service_type %u is invalid\n",
			service_template->service_type);
		return CQM_FAIL;
	}

	if (service->has_register) {
		cqm_err(handle->dev_hdl, "Service register: service_type %u has registered\n",
			service_template->service_type);
		return CQM_FAIL;
	}

	service->has_register = true;
	memcpy((void *)(&service->service_template), (void *)service_template,
	       sizeof(struct tag_service_register_template));

	return CQM_SUCCESS;
}
EXPORT_SYMBOL(cqm_service_register);

/**
 * cqm_service_unregister - The service driver deregisters the callback function from the CQM
 * @ex_handle: device pointer that represents the PF
 * @service_type: CQM service type
 */
void cqm_service_unregister(void *ex_handle, u32 service_type)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_cqm_handle *cqm_handle = NULL;
	struct tag_cqm_service *service = NULL;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: ex_handle is null\n", __func__);
		return;
	}

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	if (unlikely(!cqm_handle)) {
		pr_err("[CQM]%s: cqm_handle is null\n", __func__);
		return;
	}

	if (service_type >= CQM_SERVICE_T_MAX) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(service_type));
		return;
	}

	service = &cqm_handle->service[service_type];
	if (!service->valid)
		cqm_err(handle->dev_hdl, "Service unregister: service_type %u is disable\n",
			service_type);

	service->has_register = false;
	memset(&service->service_template, 0, sizeof(struct tag_service_register_template));
}
EXPORT_SYMBOL(cqm_service_unregister);

s32 cqm_fake_vf_num_set(void *ex_handle, u16 fake_vf_num_cfg)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct service_cap *svc_cap = NULL;

	if (!ex_handle)
		return CQM_FAIL;

	svc_cap = &handle->cfg_mgmt->svc_cap;

	if (fake_vf_num_cfg > svc_cap->fake_vf_num) {
		cqm_err(handle->dev_hdl, "fake_vf_num_cfg is invlaid, fw fake_vf_num is %u\n",
			svc_cap->fake_vf_num);
		return CQM_FAIL;
	}

	/* fake_vf_num_cfg is valid when func type is CQM_FAKE_FUNC_PARENT */
	svc_cap->fake_vf_num_cfg = fake_vf_num_cfg;

	return CQM_SUCCESS;
}
EXPORT_SYMBOL(cqm_fake_vf_num_set);

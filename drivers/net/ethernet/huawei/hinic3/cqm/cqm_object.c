// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/device.h>
#include <linux/gfp.h>
#include <linux/mm.h>

#include "ossl_knl.h"
#include "hinic3_crm.h"
#include "hinic3_hw.h"
#include "hinic3_mt.h"
#include "hinic3_hwdev.h"

#include "cqm_bitmap_table.h"
#include "cqm_bat_cla.h"
#include "cqm_object_intern.h"
#include "cqm_main.h"
#include "cqm_object.h"

/**
 * cqm_object_qpc_mpt_create - create QPC/MPT
 * @ex_handle: device pointer that represents the PF
 * @service_type: CQM service type
 * @object_type: object type, must be MPT or CTX.
 * @object_size: object size, unit is Byte
 * @object_priv: private structure of the service layer, it can be NULL.
 * @index: apply for the reserved qpn based on this value, if automatic allocation is required,
 *         please fill CQM_INDEX_INVALID.
 */
struct tag_cqm_qpc_mpt *cqm_object_qpc_mpt_create(void *ex_handle, u32 service_type,
						  enum cqm_object_type object_type,
						  u32 object_size, void *object_priv, u32 index,
						  bool low2bit_align_en)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_cqm_qpc_mpt_info *qpc_mpt_info = NULL;
	struct tag_cqm_handle *cqm_handle = NULL;
	s32 ret = CQM_FAIL;
	u32 relative_index;
	u32 fake_func_id;
	u32 index_num = index;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: ex_handle is null\n", __func__);
		return NULL;
	}

	atomic_inc(&handle->hw_stats.cqm_stats.cqm_qpc_mpt_create_cnt);

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	if (unlikely(!cqm_handle)) {
		pr_err("[CQM]%s: cqm_handle is null\n", __func__);
		return NULL;
	}

	if (service_type >= CQM_SERVICE_T_MAX || !cqm_handle->service[service_type].has_register) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(service_type));
		return NULL;
	}

	if (object_type != CQM_OBJECT_SERVICE_CTX && object_type != CQM_OBJECT_MPT) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(object_type));
		return NULL;
	}

	/* fake vf adaption, switch to corresponding VF. */
	if (cqm_handle->func_capability.fake_func_type == CQM_FAKE_FUNC_PARENT) {
		fake_func_id = index_num / cqm_handle->func_capability.fake_vf_qpc_number;
		relative_index = index_num % cqm_handle->func_capability.fake_vf_qpc_number;

		if ((s32)fake_func_id >= cqm_get_child_func_number(cqm_handle)) {
			cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(fake_func_id));
			return NULL;
		}

		index_num = relative_index;
		cqm_handle = cqm_handle->fake_cqm_handle[fake_func_id];
	}

	qpc_mpt_info = kmalloc(sizeof(*qpc_mpt_info), GFP_ATOMIC | __GFP_ZERO);
	if (!qpc_mpt_info)
		return NULL;

	qpc_mpt_info->common.object.service_type = service_type;
	qpc_mpt_info->common.object.object_type = object_type;
	qpc_mpt_info->common.object.object_size = object_size;
	atomic_set(&qpc_mpt_info->common.object.refcount, 1);
	init_completion(&qpc_mpt_info->common.object.free);
	qpc_mpt_info->common.object.cqm_handle = cqm_handle;
	qpc_mpt_info->common.xid = index_num;

	qpc_mpt_info->common.priv = object_priv;

	ret = cqm_qpc_mpt_create(&qpc_mpt_info->common.object, low2bit_align_en);
	if (ret == CQM_SUCCESS)
		return &qpc_mpt_info->common;

	cqm_err(handle->dev_hdl, CQM_FUNCTION_FAIL(cqm_qpc_mpt_create));
	kfree(qpc_mpt_info);
	return NULL;
}
EXPORT_SYMBOL(cqm_object_qpc_mpt_create);

/**
 * cqm_object_recv_queue_create - when srq is used, create rq
 * @ex_handle: device pointer that represents the PF
 * @service_type: CQM service type
 * @object_type: object type
 * @init_rq_num: init RQ number
 * @container_size: CQM queue container size
 * @wqe_size: CQM WQE size
 * @object_priv: RQ privite data
 */
struct tag_cqm_queue *cqm_object_recv_queue_create(void *ex_handle, u32 service_type,
						   enum cqm_object_type object_type,
						   u32 init_rq_num, u32 container_size,
						   u32 wqe_size, void *object_priv)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_cqm_nonrdma_qinfo *rq_qinfo = NULL;
	struct tag_cqm_handle *cqm_handle = NULL;
	s32 ret;
	u32 i;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: ex_handle is null\n", __func__);
		return NULL;
	}

	atomic_inc(&handle->hw_stats.cqm_stats.cqm_rq_create_cnt);

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	if (unlikely(!cqm_handle)) {
		pr_err("[CQM]%s: cqm_handle is null\n", __func__);
		return NULL;
	}

	if (object_type != CQM_OBJECT_NONRDMA_EMBEDDED_RQ) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(object_type));
		return NULL;
	}

	if (service_type != CQM_SERVICE_T_TOE) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(service_type));
		return NULL;
	}

	if (!cqm_handle->service[service_type].has_register) {
		cqm_err(handle->dev_hdl, "Rq create: service_type %u has not registered\n",
			service_type);
		return NULL;
	}

	/* 1. create rq qinfo */
	rq_qinfo = kmalloc(sizeof(*rq_qinfo), GFP_KERNEL | __GFP_ZERO);
	if (!rq_qinfo)
		return NULL;

	/* 2. init rq qinfo */
	rq_qinfo->container_size = container_size;
	rq_qinfo->wqe_size = wqe_size;
	rq_qinfo->wqe_per_buf = container_size / wqe_size - 1;

	rq_qinfo->common.queue_link_mode = CQM_QUEUE_TOE_SRQ_LINK_MODE;
	rq_qinfo->common.priv = object_priv;
	rq_qinfo->common.object.cqm_handle = cqm_handle;
	/* this object_size is used as container num */
	rq_qinfo->common.object.object_size = init_rq_num;
	rq_qinfo->common.object.service_type = service_type;
	rq_qinfo->common.object.object_type = object_type;
	atomic_set(&rq_qinfo->common.object.refcount, 1);
	init_completion(&rq_qinfo->common.object.free);

	/* 3. create queue header */
	rq_qinfo->common.q_header_vaddr =
	    cqm_kmalloc_align(sizeof(struct tag_cqm_queue_header),
			      GFP_KERNEL | __GFP_ZERO, CQM_QHEAD_ALIGN_ORDER);
	if (!rq_qinfo->common.q_header_vaddr)
		goto err1;

	rq_qinfo->common.q_header_paddr =
	    pci_map_single(cqm_handle->dev, rq_qinfo->common.q_header_vaddr,
			   sizeof(struct tag_cqm_queue_header), PCI_DMA_BIDIRECTIONAL);
	if (pci_dma_mapping_error(cqm_handle->dev,
				  rq_qinfo->common.q_header_paddr) != 0) {
		cqm_err(handle->dev_hdl, CQM_MAP_FAIL(q_header_vaddr));
		goto err2;
	}

	/* 4. create rq */
	for (i = 0; i < init_rq_num; i++) {
		ret = cqm_container_create(&rq_qinfo->common.object, NULL,
					   true);
		if (ret == CQM_FAIL) {
			cqm_err(handle->dev_hdl,
				CQM_FUNCTION_FAIL(cqm_container_create));
			goto err3;
		}
		if (!rq_qinfo->common.head_container)
			rq_qinfo->common.head_container =
			    rq_qinfo->common.tail_container;
	}

	return &rq_qinfo->common;

err3:
	cqm_container_free(rq_qinfo->common.head_container, NULL,
			   &rq_qinfo->common);
err2:
	cqm_kfree_align(rq_qinfo->common.q_header_vaddr);
	rq_qinfo->common.q_header_vaddr = NULL;
err1:
	kfree(rq_qinfo);
	return NULL;
}
EXPORT_SYMBOL(cqm_object_recv_queue_create);

/**
 * cqm_object_share_recv_queue_add_container - allocate new container for srq
 * @common: queue structure pointer
 */
s32 cqm_object_share_recv_queue_add_container(struct tag_cqm_queue *common)
{
	if (unlikely(!common)) {
		pr_err("[CQM]%s: common is null\n", __func__);
		return CQM_FAIL;
	}

	return cqm_container_create(&common->object, NULL, true);
}
EXPORT_SYMBOL(cqm_object_share_recv_queue_add_container);

s32 cqm_object_srq_add_container_free(struct tag_cqm_queue *common, u8 **container_addr)
{
	if (unlikely(!common)) {
		pr_err("[CQM]%s: common is null\n", __func__);
		return CQM_FAIL;
	}

	return cqm_container_create(&common->object, container_addr, false);
}
EXPORT_SYMBOL(cqm_object_srq_add_container_free);

static bool cqm_object_share_recv_queue_param_check(struct hinic3_hwdev *handle, u32 service_type,
						    enum cqm_object_type object_type,
						    u32 container_size, u32 wqe_size)
{
	struct tag_cqm_handle *cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);

	/* service_type must be CQM_SERVICE_T_TOE */
	if (service_type != CQM_SERVICE_T_TOE) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(service_type));
		return false;
	}

	/* exception of service registration check */
	if (!cqm_handle->service[service_type].has_register) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(service_type));
		return false;
	}

	/* container size2^N aligning */
	if (!cqm_check_align(container_size)) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(container_size));
		return false;
	}

	/* external parameter check: object_type must be
	 * CQM_OBJECT_NONRDMA_SRQ
	 */
	if (object_type != CQM_OBJECT_NONRDMA_SRQ) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(object_type));
		return false;
	}

	/* wqe_size, the divisor, cannot be 0 */
	if (wqe_size == 0) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(wqe_size));
		return false;
	}

	return true;
}

/**
 * cqm_object_share_recv_queue_create - create srq
 * @ex_handle: device pointer that represents the PF
 * @service_type: CQM service type
 * @object_type: object type
 * @container_number: CQM queue container number
 * @container_size: CQM queue container size
 * @wqe_size: CQM WQE size
 */
struct tag_cqm_queue *cqm_object_share_recv_queue_create(void *ex_handle, u32 service_type,
							 enum cqm_object_type object_type,
							 u32 container_number, u32 container_size,
							 u32 wqe_size)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_cqm_nonrdma_qinfo *srq_qinfo = NULL;
	struct tag_cqm_handle *cqm_handle = NULL;
	struct tag_cqm_service *service = NULL;
	s32 ret;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: ex_handle is null\n", __func__);
		return NULL;
	}

	atomic_inc(&handle->hw_stats.cqm_stats.cqm_srq_create_cnt);

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	if (unlikely(!cqm_handle)) {
		pr_err("[CQM]%s: cqm_handle is null\n", __func__);
		return NULL;
	}

	if (!cqm_object_share_recv_queue_param_check(handle, service_type, object_type,
						     container_size, wqe_size))
		return NULL;

	/* 2. create and initialize srq info */
	srq_qinfo = kmalloc(sizeof(*srq_qinfo), GFP_KERNEL | __GFP_ZERO);
	if (!srq_qinfo)
		return NULL;

	srq_qinfo->common.object.cqm_handle = cqm_handle;
	srq_qinfo->common.object.object_size = container_number;
	srq_qinfo->common.object.object_type = object_type;
	srq_qinfo->common.object.service_type = service_type;
	atomic_set(&srq_qinfo->common.object.refcount, 1);
	init_completion(&srq_qinfo->common.object.free);

	srq_qinfo->common.queue_link_mode = CQM_QUEUE_TOE_SRQ_LINK_MODE;
	srq_qinfo->common.priv = NULL;
	srq_qinfo->wqe_per_buf = container_size / wqe_size - 1;
	srq_qinfo->wqe_size = wqe_size;
	srq_qinfo->container_size = container_size;
	service = &cqm_handle->service[service_type];
	srq_qinfo->q_ctx_size = service->service_template.srq_ctx_size;

	/* 3. create srq and srq ctx */
	ret = cqm_share_recv_queue_create(&srq_qinfo->common.object);
	if (ret == CQM_SUCCESS)
		return &srq_qinfo->common;

	cqm_err(handle->dev_hdl,
		CQM_FUNCTION_FAIL(cqm_share_recv_queue_create));
	kfree(srq_qinfo);
	return NULL;
}
EXPORT_SYMBOL(cqm_object_share_recv_queue_create);

/**
 * cqm_object_fc_srq_create - RQ creation temporarily provided for the FC service.
 *                            Special requirement: The number of valid WQEs in the queue
 *                            must meet the number of transferred WQEs. Linkwqe can only be
 *                            filled at the end of the page. The actual valid number exceeds
 *                            the requirement. In this case, the service needs to be
 *                            informed of the additional number to be created.
 * @ex_handle: device pointer that represents the PF
 * @service_type: CQM service type
 * @object_type: object type
 * @wqe_number: number of valid WQEs
 * @wqe_size: size of valid WQEs
 * @object_priv: private structure of the service layer, it can be NULL
 */
struct tag_cqm_queue *cqm_object_fc_srq_create(void *ex_handle, u32 service_type,
					       enum cqm_object_type object_type,
					       u32 wqe_number, u32 wqe_size,
					       void *object_priv)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_cqm_nonrdma_qinfo *nonrdma_qinfo = NULL;
	struct tag_cqm_handle *cqm_handle = NULL;
	struct tag_cqm_service *service = NULL;
	u32 valid_wqe_per_buffer;
	u32 wqe_sum; /* include linkwqe, normal wqe */
	u32 buf_size;
	u32 buf_num;
	s32 ret;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: ex_handle is null\n", __func__);
		return NULL;
	}

	atomic_inc(&handle->hw_stats.cqm_stats.cqm_fc_srq_create_cnt);

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	if (unlikely(!cqm_handle)) {
		pr_err("[CQM]%s: cqm_handle is null\n", __func__);
		return NULL;
	}

	/* service_type must be fc */
	if (service_type != CQM_SERVICE_T_FC) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(service_type));
		return NULL;
	}

	/* exception of service unregistered check */
	if (!cqm_handle->service[service_type].has_register) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(service_type));
		return NULL;
	}

	/* wqe_size cannot exceed PAGE_SIZE and must be 2^n aligned. */
	if (wqe_size >= PAGE_SIZE || (!cqm_check_align(wqe_size))) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(wqe_size));
		return NULL;
	}

	/* FC RQ is SRQ. (Different from the SRQ concept of TOE, FC indicates
	 * that packets received by all flows are placed on the same RQ.
	 * The SRQ of TOE is similar to the RQ resource pool.)
	 */
	if (object_type != CQM_OBJECT_NONRDMA_SRQ) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(object_type));
		return NULL;
	}

	service = &cqm_handle->service[service_type];
	buf_size = (u32)(PAGE_SIZE << (service->buf_order));
	/* subtract 1 link wqe */
	valid_wqe_per_buffer = buf_size / wqe_size - 1;
	buf_num = wqe_number / valid_wqe_per_buffer;
	if (wqe_number % valid_wqe_per_buffer != 0)
		buf_num++;

	/* calculate the total number of WQEs */
	wqe_sum = buf_num * (valid_wqe_per_buffer + 1);
	nonrdma_qinfo = kmalloc(sizeof(*nonrdma_qinfo), GFP_KERNEL | __GFP_ZERO);
	if (!nonrdma_qinfo)
		return NULL;

	/* initialize object member */
	nonrdma_qinfo->common.object.service_type = service_type;
	nonrdma_qinfo->common.object.object_type = object_type;
	/* total number of WQEs */
	nonrdma_qinfo->common.object.object_size = wqe_sum;
	atomic_set(&nonrdma_qinfo->common.object.refcount, 1);
	init_completion(&nonrdma_qinfo->common.object.free);
	nonrdma_qinfo->common.object.cqm_handle = cqm_handle;

	/* Initialize the doorbell used by the current queue.
	 * The default doorbell is the hardware doorbell.
	 */
	nonrdma_qinfo->common.current_q_doorbell = CQM_HARDWARE_DOORBELL;
	/* Currently, the connection mode is fixed. In the future,
	 * the service needs to transfer the connection mode.
	 */
	nonrdma_qinfo->common.queue_link_mode = CQM_QUEUE_RING_MODE;

	/* initialize public members */
	nonrdma_qinfo->common.priv = object_priv;
	nonrdma_qinfo->common.valid_wqe_num = wqe_sum - buf_num;

	/* initialize internal private members */
	nonrdma_qinfo->wqe_size = wqe_size;
	/* RQ (also called SRQ of FC) created by FC services,
	 * CTX needs to be created.
	 */
	nonrdma_qinfo->q_ctx_size = service->service_template.srq_ctx_size;

	ret = cqm_nonrdma_queue_create(&nonrdma_qinfo->common.object);
	if (ret == CQM_SUCCESS)
		return &nonrdma_qinfo->common;

	cqm_err(handle->dev_hdl, CQM_FUNCTION_FAIL(cqm_fc_queue_create));
	kfree(nonrdma_qinfo);
	return NULL;
}
EXPORT_SYMBOL(cqm_object_fc_srq_create);

static bool cqm_object_nonrdma_queue_param_check(struct hinic3_hwdev *handle, u32 service_type,
						 enum cqm_object_type object_type, u32 wqe_size)
{
	struct tag_cqm_handle *cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);

	/* exception of service registrion check */
	if (!cqm_handle->service[service_type].has_register) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(service_type));
		return false;
	}
	/* wqe_size can't be more than PAGE_SIZE, can't be zero, must be power
	 * of 2 the function of cqm_check_align is to check above
	 */
	if (wqe_size >= PAGE_SIZE || (!cqm_check_align(wqe_size))) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(wqe_size));
		return false;
	}

	/* nonrdma supports: RQ, SQ, SRQ, CQ, SCQ */
	if (object_type < CQM_OBJECT_NONRDMA_EMBEDDED_RQ ||
	    object_type > CQM_OBJECT_NONRDMA_SCQ) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(object_type));
		return false;
	}

	return true;
}

/**
 * cqm_object_nonrdma_queue_create - create nonrdma queue
 * @ex_handle: device pointer that represents the PF
 * @service_type: CQM service type
 * @object_type: object type, can be embedded RQ/SQ/CQ and SRQ/SCQ
 * @wqe_number: include link wqe
 * @wqe_size: fixed length, must be power of 2
 * @object_priv: private structure of the service layer, it can be NULL
 */
struct tag_cqm_queue *cqm_object_nonrdma_queue_create(void *ex_handle, u32 service_type,
						      enum cqm_object_type object_type,
						      u32 wqe_number, u32 wqe_size,
						      void *object_priv)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_cqm_nonrdma_qinfo *nonrdma_qinfo = NULL;
	struct tag_cqm_handle *cqm_handle = NULL;
	struct tag_cqm_service *service = NULL;
	s32 ret;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: ex_handle is null\n", __func__);
		return NULL;
	}

	atomic_inc(&handle->hw_stats.cqm_stats.cqm_nonrdma_queue_create_cnt);

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	if (unlikely(!cqm_handle)) {
		pr_err("[CQM]%s: cqm_handle is null\n", __func__);
		return NULL;
	}

	if (!cqm_object_nonrdma_queue_param_check(handle, service_type, object_type, wqe_size))
		return NULL;

	nonrdma_qinfo = kmalloc(sizeof(*nonrdma_qinfo), GFP_KERNEL | __GFP_ZERO);
	if (!nonrdma_qinfo)
		return NULL;

	nonrdma_qinfo->common.object.service_type = service_type;
	nonrdma_qinfo->common.object.object_type = object_type;
	nonrdma_qinfo->common.object.object_size = wqe_number;
	atomic_set(&nonrdma_qinfo->common.object.refcount, 1);
	init_completion(&nonrdma_qinfo->common.object.free);
	nonrdma_qinfo->common.object.cqm_handle = cqm_handle;

	/* Initialize the doorbell used by the current queue.
	 * The default value is hardware doorbell
	 */
	nonrdma_qinfo->common.current_q_doorbell = CQM_HARDWARE_DOORBELL;
	/* Currently, the link mode is hardcoded and needs to be transferred by
	 * the service side.
	 */
	nonrdma_qinfo->common.queue_link_mode = CQM_QUEUE_RING_MODE;

	nonrdma_qinfo->common.priv = object_priv;

	/* Initialize internal private members */
	nonrdma_qinfo->wqe_size = wqe_size;
	service = &cqm_handle->service[service_type];
	switch (object_type) {
	case CQM_OBJECT_NONRDMA_SCQ:
		nonrdma_qinfo->q_ctx_size = service->service_template.scq_ctx_size;
		break;
	case CQM_OBJECT_NONRDMA_SRQ:
		/* Currently, the SRQ of the service is created through a
		 * dedicated interface.
		 */
		nonrdma_qinfo->q_ctx_size = service->service_template.srq_ctx_size;
		break;
	default:
		break;
	}

	ret = cqm_nonrdma_queue_create(&nonrdma_qinfo->common.object);
	if (ret == CQM_SUCCESS)
		return &nonrdma_qinfo->common;

	cqm_err(handle->dev_hdl, CQM_FUNCTION_FAIL(cqm_nonrdma_queue_create));
	kfree(nonrdma_qinfo);
	return NULL;
}
EXPORT_SYMBOL(cqm_object_nonrdma_queue_create);

static bool cqm_object_rdma_queue_param_check(struct hinic3_hwdev *handle, u32 service_type,
					      enum cqm_object_type object_type)
{
	struct tag_cqm_handle *cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);

	/* service_type must be CQM_SERVICE_T_ROCE */
	if (service_type != CQM_SERVICE_T_ROCE) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(service_type));
		return false;
	}
	/* exception of service registrion check */
	if (!cqm_handle->service[service_type].has_register) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(service_type));
		return false;
	}

	/* rdma supports: QP, SRQ, SCQ */
	if (object_type > CQM_OBJECT_RDMA_SCQ || object_type < CQM_OBJECT_RDMA_QP) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(object_type));
		return false;
	}

	return true;
}

/**
 * cqm_object_rdma_queue_create - create rdma queue
 * @ex_handle: device pointer that represents the PF
 * @service_type: CQM service type
 * @object_type: object type, can be QP and SRQ/SCQ
 * @object_size: object size
 * @object_priv: private structure of the service layer, it can be NULL
 * @room_header_alloc: Whether to apply for queue room and header space
 * @xid: common index
 */
struct tag_cqm_queue *cqm_object_rdma_queue_create(void *ex_handle, u32 service_type,
						   enum cqm_object_type object_type,
						   u32 object_size, void *object_priv,
						   bool room_header_alloc, u32 xid)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_cqm_rdma_qinfo *rdma_qinfo = NULL;
	struct tag_cqm_handle *cqm_handle = NULL;
	struct tag_cqm_service *service = NULL;
	s32 ret;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: ex_handle is null\n", __func__);
		return NULL;
	}

	atomic_inc(&handle->hw_stats.cqm_stats.cqm_rdma_queue_create_cnt);

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	if (unlikely(!cqm_handle)) {
		pr_err("[CQM]%s: cqm_handle is null\n", __func__);
		return NULL;
	}

	if (!cqm_object_rdma_queue_param_check(handle, service_type, object_type))
		return NULL;

	rdma_qinfo = kmalloc(sizeof(*rdma_qinfo), GFP_KERNEL | __GFP_ZERO);
	if (!rdma_qinfo)
		return NULL;

	rdma_qinfo->common.object.service_type = service_type;
	rdma_qinfo->common.object.object_type = object_type;
	rdma_qinfo->common.object.object_size = object_size;
	atomic_set(&rdma_qinfo->common.object.refcount, 1);
	init_completion(&rdma_qinfo->common.object.free);
	rdma_qinfo->common.object.cqm_handle = cqm_handle;
	rdma_qinfo->common.queue_link_mode = CQM_QUEUE_RDMA_QUEUE_MODE;
	rdma_qinfo->common.priv = object_priv;
	rdma_qinfo->common.current_q_room = CQM_RDMA_Q_ROOM_1;
	rdma_qinfo->room_header_alloc = room_header_alloc;
	rdma_qinfo->common.index = xid;

	/* Initializes the doorbell used by the current queue.
	 * The default value is hardware doorbell
	 */
	rdma_qinfo->common.current_q_doorbell = CQM_HARDWARE_DOORBELL;

	service = &cqm_handle->service[service_type];
	switch (object_type) {
	case CQM_OBJECT_RDMA_SCQ:
		rdma_qinfo->q_ctx_size = service->service_template.scq_ctx_size;
		break;
	case CQM_OBJECT_RDMA_SRQ:
		rdma_qinfo->q_ctx_size = service->service_template.srq_ctx_size;
		break;
	default:
		break;
	}

	ret = cqm_rdma_queue_create(&rdma_qinfo->common.object);
	if (ret == CQM_SUCCESS)
		return &rdma_qinfo->common;

	cqm_err(handle->dev_hdl, CQM_FUNCTION_FAIL(cqm_rdma_queue_create));
	kfree(rdma_qinfo);
	return NULL;
}
EXPORT_SYMBOL(cqm_object_rdma_queue_create);

/**
 * cqm_object_rdma_table_get - create mtt and rdmarc of the rdma service
 * @ex_handle: device pointer that represents the PF
 * @service_type: CQM service type
 * @object_type: object type
 * @index_base: start of index
 * @index_number: number of created
 */
struct tag_cqm_mtt_rdmarc *cqm_object_rdma_table_get(void *ex_handle, u32 service_type,
						     enum cqm_object_type object_type,
						     u32 index_base, u32 index_number)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_cqm_rdma_table *rdma_table = NULL;
	struct tag_cqm_handle *cqm_handle = NULL;
	s32 ret;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: ex_handle is null\n", __func__);
		return NULL;
	}

	atomic_inc(&handle->hw_stats.cqm_stats.cqm_rdma_table_create_cnt);

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	if (unlikely(!cqm_handle)) {
		pr_err("[CQM]%s: cqm_handle is null\n", __func__);
		return NULL;
	}

	/* service_type must be CQM_SERVICE_T_ROCE */
	if (service_type != CQM_SERVICE_T_ROCE) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(service_type));
		return NULL;
	}

	/* exception of service registrion check */
	if (!cqm_handle->service[service_type].has_register) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(service_type));
		return NULL;
	}

	if (object_type != CQM_OBJECT_MTT &&
	    object_type != CQM_OBJECT_RDMARC) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(object_type));
		return NULL;
	}

	rdma_table = kmalloc(sizeof(*rdma_table), GFP_KERNEL | __GFP_ZERO);
	if (!rdma_table)
		return NULL;

	rdma_table->common.object.service_type = service_type;
	rdma_table->common.object.object_type = object_type;
	rdma_table->common.object.object_size = (u32)(index_number *
						      sizeof(dma_addr_t));
	atomic_set(&rdma_table->common.object.refcount, 1);
	init_completion(&rdma_table->common.object.free);
	rdma_table->common.object.cqm_handle = cqm_handle;
	rdma_table->common.index_base = index_base;
	rdma_table->common.index_number = index_number;

	ret = cqm_rdma_table_create(&rdma_table->common.object);
	if (ret == CQM_SUCCESS)
		return &rdma_table->common;

	cqm_err(handle->dev_hdl, CQM_FUNCTION_FAIL(cqm_rdma_table_create));
	kfree(rdma_table);
	return NULL;
}
EXPORT_SYMBOL(cqm_object_rdma_table_get);

static s32 cqm_qpc_mpt_delete_ret(struct tag_cqm_object *object)
{
	u32 object_type;

	object_type = object->object_type;
	switch (object_type) {
	case CQM_OBJECT_SERVICE_CTX:
	case CQM_OBJECT_MPT:
		cqm_qpc_mpt_delete(object);
		return CQM_SUCCESS;
	default:
		return CQM_FAIL;
	}
}

static s32 cqm_nonrdma_queue_delete_ret(struct tag_cqm_object *object)
{
	u32 object_type;

	object_type = object->object_type;
	switch (object_type) {
	case CQM_OBJECT_NONRDMA_EMBEDDED_RQ:
	case CQM_OBJECT_NONRDMA_EMBEDDED_SQ:
	case CQM_OBJECT_NONRDMA_EMBEDDED_CQ:
	case CQM_OBJECT_NONRDMA_SCQ:
		cqm_nonrdma_queue_delete(object);
		return CQM_SUCCESS;
	case CQM_OBJECT_NONRDMA_SRQ:
		if (object->service_type == CQM_SERVICE_T_TOE)
			cqm_share_recv_queue_delete(object);
		else
			cqm_nonrdma_queue_delete(object);

		return CQM_SUCCESS;
	default:
		return CQM_FAIL;
	}
}

static s32 cqm_rdma_queue_delete_ret(struct tag_cqm_object *object)
{
	u32 object_type;

	object_type = object->object_type;
	switch (object_type) {
	case CQM_OBJECT_RDMA_QP:
	case CQM_OBJECT_RDMA_SRQ:
	case CQM_OBJECT_RDMA_SCQ:
		cqm_rdma_queue_delete(object);
		return CQM_SUCCESS;
	default:
		return CQM_FAIL;
	}
}

static s32 cqm_rdma_table_delete_ret(struct tag_cqm_object *object)
{
	u32 object_type;

	object_type = object->object_type;
	switch (object_type) {
	case CQM_OBJECT_MTT:
	case CQM_OBJECT_RDMARC:
		cqm_rdma_table_delete(object);
		return CQM_SUCCESS;
	default:
		return CQM_FAIL;
	}
}

/**
 * cqm_object_delete - Deletes a created object. This function will be sleep and wait
 *                     for all operations on this object to be performed.
 * @object: CQM object
 */
void cqm_object_delete(struct tag_cqm_object *object)
{
	struct tag_cqm_handle *cqm_handle = NULL;
	struct hinic3_hwdev *handle = NULL;

	if (unlikely(!object)) {
		pr_err("[CQM]%s: object is null\n", __func__);
		return;
	}
	if (!object->cqm_handle) {
		pr_err("[CQM]object del: cqm_handle is null, service type %u, refcount %d\n",
		       object->service_type, (int)object->refcount.counter);
		kfree(object);
		return;
	}

	cqm_handle = (struct tag_cqm_handle *)object->cqm_handle;

	if (!cqm_handle->ex_handle) {
		pr_err("[CQM]object del: ex_handle is null, service type %u, refcount %d\n",
		       object->service_type, (int)object->refcount.counter);
		kfree(object);
		return;
	}

	handle = cqm_handle->ex_handle;

	if (object->service_type >= CQM_SERVICE_T_MAX) {
		cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(object->service_type));
		kfree(object);
		return;
	}

	if (cqm_qpc_mpt_delete_ret(object) == CQM_SUCCESS) {
		kfree(object);
		return;
	}

	if (cqm_nonrdma_queue_delete_ret(object) == CQM_SUCCESS) {
		kfree(object);
		return;
	}

	if (cqm_rdma_queue_delete_ret(object) == CQM_SUCCESS) {
		kfree(object);
		return;
	}

	if (cqm_rdma_table_delete_ret(object) == CQM_SUCCESS) {
		kfree(object);
		return;
	}

	cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(object->object_type));
	kfree(object);
}
EXPORT_SYMBOL(cqm_object_delete);

/**
 * cqm_object_offset_addr - Only the rdma table can be searched to obtain the PA and VA
 *                          at the specified offset of the object buffer.
 * @object: CQM object
 * @offset: For a rdma table, the offset is the absolute index number
 * @paddr: physical address
 */
u8 *cqm_object_offset_addr(struct tag_cqm_object *object, u32 offset, dma_addr_t *paddr)
{
	u32 object_type = object->object_type;

	/* The data flow path takes performance into consideration and
	 * does not check input parameters.
	 */
	switch (object_type) {
	case CQM_OBJECT_MTT:
	case CQM_OBJECT_RDMARC:
		return cqm_rdma_table_offset_addr(object, offset, paddr);
	default:
		break;
	}

	return NULL;
}
EXPORT_SYMBOL(cqm_object_offset_addr);

/**
 * cqm_object_get - Obtain an object based on the index
 * @ex_handle: device pointer that represents the PF
 * @object_type: object type
 * @index: support qpn,mptn,scqn,srqn (n->number)
 * @bh: barrier or not
 */
struct tag_cqm_object *cqm_object_get(void *ex_handle, enum cqm_object_type object_type,
				      u32 index, bool bh)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_cqm_handle *cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	struct tag_cqm_bat_table *bat_table = &cqm_handle->bat_table;
	struct tag_cqm_object_table *object_table = NULL;
	struct tag_cqm_cla_table *cla_table = NULL;
	struct tag_cqm_object *object = NULL;

	/* The data flow path takes performance into consideration and
	 * does not check input parameters.
	 */
	switch (object_type) {
	case CQM_OBJECT_SERVICE_CTX:
		cla_table = cqm_cla_table_get(bat_table, CQM_BAT_ENTRY_T_QPC);
		break;
	case CQM_OBJECT_MPT:
		cla_table = cqm_cla_table_get(bat_table, CQM_BAT_ENTRY_T_MPT);
		break;
	case CQM_OBJECT_RDMA_SRQ:
		cla_table = cqm_cla_table_get(bat_table, CQM_BAT_ENTRY_T_SRQC);
		break;
	case CQM_OBJECT_RDMA_SCQ:
	case CQM_OBJECT_NONRDMA_SCQ:
		cla_table = cqm_cla_table_get(bat_table, CQM_BAT_ENTRY_T_SCQC);
		break;
	default:
		return NULL;
	}

	if (!cla_table) {
		cqm_err(handle->dev_hdl, CQM_FUNCTION_FAIL(cqm_cla_table_get));
		return NULL;
	}

	object_table = &cla_table->obj_table;
	object = cqm_object_table_get(cqm_handle, object_table, index, bh);
	return object;
}
EXPORT_SYMBOL(cqm_object_get);

/**
 * cqm_object_put - This function must be called after the cqm_object_get function.
 *                  Otherwise, the object cannot be released.
 * @object: CQM object
 */
void cqm_object_put(struct tag_cqm_object *object)
{
	/* The data flow path takes performance into consideration and
	 * does not check input parameters.
	 */
	if (atomic_dec_and_test(&object->refcount) != 0)
		complete(&object->free);
}
EXPORT_SYMBOL(cqm_object_put);

/**
 * cqm_object_funcid - Obtain the ID of the function to which the object belongs
 * @object: CQM object
 */
s32 cqm_object_funcid(struct tag_cqm_object *object)
{
	struct tag_cqm_handle *cqm_handle = NULL;

	if (unlikely(!object)) {
		pr_err("[CQM]%s: object is null\n", __func__);
		return CQM_FAIL;
	}
	if (unlikely(!object->cqm_handle)) {
		pr_err("[CQM]%s: cqm_handle is null\n", __func__);
		return CQM_FAIL;
	}

	cqm_handle = (struct tag_cqm_handle *)object->cqm_handle;

	return cqm_handle->func_attribute.func_global_idx;
}
EXPORT_SYMBOL(cqm_object_funcid);

/**
 * cqm_object_resize_alloc_new - Currently this function is only used for RoCE. The CQ buffer
 *                               is ajusted, but the cqn and cqc remain unchanged.
 *                               This function allocates new buffer, but do not release old buffer.
 *                               The valid buffer is still old buffer.
 * @object: CQM object
 * @object_size: new buffer size
 */
s32 cqm_object_resize_alloc_new(struct tag_cqm_object *object, u32 object_size)
{
	struct tag_cqm_rdma_qinfo *qinfo = (struct tag_cqm_rdma_qinfo *)(void *)object;
	struct tag_cqm_handle *cqm_handle = NULL;
	struct tag_cqm_service *service = NULL;
	struct tag_cqm_buf *q_room_buf = NULL;
	struct hinic3_hwdev *handle = NULL;
	u32 order, buf_size;

	if (unlikely(!object)) {
		pr_err("[CQM]%s: object is null\n", __func__);
		return CQM_FAIL;
	}

	cqm_handle = (struct tag_cqm_handle *)object->cqm_handle;
	if (unlikely(!cqm_handle)) {
		pr_err("[CQM]%s: cqm_handle is null\n", __func__);
		return CQM_FAIL;
	}
	handle = cqm_handle->ex_handle;

	/* This interface is used only for the CQ of RoCE service. */
	if (object->service_type == CQM_SERVICE_T_ROCE &&
	    object->object_type == CQM_OBJECT_RDMA_SCQ) {
		service = cqm_handle->service + object->service_type;
		order = service->buf_order;
		buf_size = (u32)(PAGE_SIZE << order);

		if (qinfo->common.current_q_room == CQM_RDMA_Q_ROOM_1)
			q_room_buf = &qinfo->common.q_room_buf_2;
		else
			q_room_buf = &qinfo->common.q_room_buf_1;

		if (qinfo->room_header_alloc) {
			q_room_buf->buf_number = ALIGN(object_size, buf_size) /
						 buf_size;
			q_room_buf->page_number = q_room_buf->buf_number <<
						  order;
			q_room_buf->buf_size = buf_size;
			if (cqm_buf_alloc(cqm_handle, q_room_buf, true) ==
			    CQM_FAIL) {
				cqm_err(handle->dev_hdl,
					CQM_FUNCTION_FAIL(cqm_buf_alloc));
				return CQM_FAIL;
			}

			qinfo->new_object_size = object_size;
			return CQM_SUCCESS;
		}

		cqm_err(handle->dev_hdl,
			CQM_WRONG_VALUE(qinfo->room_header_alloc));
		return CQM_FAIL;
	}

	cqm_err(handle->dev_hdl,
		"Cq resize alloc: service_type %u object_type %u do not support resize\n",
		object->service_type, object->object_type);
	return CQM_FAIL;
}
EXPORT_SYMBOL(cqm_object_resize_alloc_new);

/**
 * cqm_object_resize_free_new - Currently this function is only used for RoCE. The CQ buffer
 *                              is ajusted, but the cqn and cqc remain unchanged. This function
 *                              frees new buffer, and is used to deal with exceptions.
 * @object: CQM object
 */
void cqm_object_resize_free_new(struct tag_cqm_object *object)
{
	struct tag_cqm_rdma_qinfo *qinfo = (struct tag_cqm_rdma_qinfo *)(void *)object;
	struct tag_cqm_handle *cqm_handle = NULL;
	struct tag_cqm_buf *q_room_buf = NULL;
	struct hinic3_hwdev *handle = NULL;

	if (unlikely(!object)) {
		pr_err("[CQM]%s: object is null\n", __func__);
		return;
	}

	cqm_handle = (struct tag_cqm_handle *)object->cqm_handle;
	if (unlikely(!cqm_handle)) {
		pr_err("[CQM]%s: cqm_handle is null\n", __func__);
		return;
	}
	handle = cqm_handle->ex_handle;

	/* This interface is used only for the CQ of RoCE service. */
	if (object->service_type == CQM_SERVICE_T_ROCE &&
	    object->object_type == CQM_OBJECT_RDMA_SCQ) {
		if (qinfo->common.current_q_room == CQM_RDMA_Q_ROOM_1)
			q_room_buf = &qinfo->common.q_room_buf_2;
		else
			q_room_buf = &qinfo->common.q_room_buf_1;

		qinfo->new_object_size = 0;

		cqm_buf_free(q_room_buf, cqm_handle);
	} else {
		cqm_err(handle->dev_hdl,
			"Cq resize free: service_type %u object_type %u do not support resize\n",
			object->service_type, object->object_type);
	}
}
EXPORT_SYMBOL(cqm_object_resize_free_new);

/**
 * cqm_object_resize_free_old - Currently this function is only used for RoCE. The CQ buffer
 *                              is ajusted, but the cqn and cqc remain unchanged. This function
 *                              frees old buffer and switches the valid buffer to new buffer.
 * @object: CQM object
 */
void cqm_object_resize_free_old(struct tag_cqm_object *object)
{
	struct tag_cqm_rdma_qinfo *qinfo = (struct tag_cqm_rdma_qinfo *)(void *)object;
	struct tag_cqm_handle *cqm_handle = NULL;
	struct tag_cqm_buf *q_room_buf = NULL;

	if (unlikely(!object)) {
		pr_err("[CQM]%s: object is null\n", __func__);
		return;
	}

	cqm_handle = (struct tag_cqm_handle *)object->cqm_handle;
	if (unlikely(!cqm_handle)) {
		pr_err("[CQM]%s: cqm_handle is null\n", __func__);
		return;
	}

	/* This interface is used only for the CQ of RoCE service. */
	if (object->service_type == CQM_SERVICE_T_ROCE &&
	    object->object_type == CQM_OBJECT_RDMA_SCQ) {
		if (qinfo->common.current_q_room == CQM_RDMA_Q_ROOM_1) {
			q_room_buf = &qinfo->common.q_room_buf_1;
			qinfo->common.current_q_room = CQM_RDMA_Q_ROOM_2;
		} else {
			q_room_buf = &qinfo->common.q_room_buf_2;
			qinfo->common.current_q_room = CQM_RDMA_Q_ROOM_1;
		}

		object->object_size = qinfo->new_object_size;

		cqm_buf_free(q_room_buf, cqm_handle);
	}
}
EXPORT_SYMBOL(cqm_object_resize_free_old);

/**
 * cqm_gid_base - Obtain the base virtual address of the gid table for FT debug
 * @ex_handle: device pointer that represents the PF
 */
void *cqm_gid_base(void *ex_handle)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_cqm_cla_table *cla_table = NULL;
	struct tag_cqm_bat_table *bat_table = NULL;
	struct tag_cqm_handle *cqm_handle = NULL;
	struct tag_cqm_buf *cla_z_buf = NULL;
	u32 entry_type, i;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: ex_handle is null\n", __func__);
		return NULL;
	}

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	if (unlikely(!cqm_handle)) {
		pr_err("[CQM]%s: cqm_handle is null\n", __func__);
		return NULL;
	}

	bat_table = &cqm_handle->bat_table;
	for (i = 0; i < CQM_BAT_ENTRY_MAX; i++) {
		entry_type = bat_table->bat_entry_type[i];
		if (entry_type == CQM_BAT_ENTRY_T_GID) {
			cla_table = &bat_table->entry[i];
			cla_z_buf = &cla_table->cla_z_buf;
			if (cla_z_buf->buf_list)
				return cla_z_buf->buf_list->va;
		}
	}

	return NULL;
}

/**
 * cqm_timer_base - Obtain the base virtual address of the timer for live migration
 * @ex_handle: device pointer that represents the PF
 */
void *cqm_timer_base(void *ex_handle)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_cqm_cla_table *cla_table = NULL;
	struct tag_cqm_bat_table *bat_table = NULL;
	struct tag_cqm_handle *cqm_handle = NULL;
	struct tag_cqm_buf *cla_z_buf = NULL;
	u32 entry_type, i;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: ex_handle is null\n", __func__);
		return NULL;
	}

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	if (unlikely(!cqm_handle)) {
		pr_err("[CQM]%s: cqm_handle is null\n", __func__);
		return NULL;
	}

	/* Timer resource is configured on PPF. */
	if (handle->hwif->attr.func_type != CQM_PPF) {
		cqm_err(handle->dev_hdl, "%s: wrong function type:%d\n",
			__func__, handle->hwif->attr.func_type);
		return NULL;
	}

	bat_table = &cqm_handle->bat_table;
	for (i = 0; i < CQM_BAT_ENTRY_MAX; i++) {
		entry_type = bat_table->bat_entry_type[i];
		if (entry_type != CQM_BAT_ENTRY_T_TIMER)
			continue;

		cla_table = &bat_table->entry[i];
		cla_z_buf = &cla_table->cla_z_buf;

		if (!cla_z_buf->direct.va) {
			if (cqm_buf_alloc_direct(cqm_handle, cla_z_buf, true) ==
			    CQM_FAIL) {
				cqm_err(handle->dev_hdl,
					CQM_FUNCTION_FAIL(cqm_buf_alloc_direct));
				return NULL;
			}
		}

		return cla_z_buf->direct.va;
	}

	return NULL;
}
EXPORT_SYMBOL(cqm_timer_base);

static s32 cqm_function_timer_clear_getindex(struct hinic3_hwdev *ex_handle, u32 *buffer_index,
					     u32 function_id, u32 timer_page_num,
					     const struct tag_cqm_buf *cla_z_buf)
{
	struct tag_cqm_handle *cqm_handle = (struct tag_cqm_handle *)(ex_handle->cqm_hdl);
	struct tag_cqm_func_capability *func_cap = &cqm_handle->func_capability;
	u32 index;

	/* Convert functionid and the functionid does not exceed the value range
	 * of the tiemr buffer.
	 */
	if (function_id < (func_cap->timer_pf_id_start + func_cap->timer_pf_num) &&
	    function_id >= func_cap->timer_pf_id_start) {
		index = function_id - func_cap->timer_pf_id_start;
	} else if (function_id < (func_cap->timer_vf_id_start + func_cap->timer_vf_num) &&
		   function_id >= func_cap->timer_vf_id_start) {
		index = (function_id - func_cap->timer_vf_id_start) +
			func_cap->timer_pf_num;
	} else {
		cqm_err(ex_handle->dev_hdl, "Timer clear: wrong function_id=0x%x\n",
			function_id);
		return CQM_FAIL;
	}

	if ((index * timer_page_num + timer_page_num) > cla_z_buf->buf_number) {
		cqm_err(ex_handle->dev_hdl,
			"Timer clear: over cla_z_buf_num, buffer_i=0x%x, zbuf_num=0x%x\n",
			index, cla_z_buf->buf_number);
		return CQM_FAIL;
	}

	*buffer_index = index;
	return CQM_SUCCESS;
}

static void cqm_clear_timer(void *ex_handle, u32 function_id, struct hinic3_hwdev *handle,
			    struct tag_cqm_cla_table *cla_table)
{
	u32 timer_buffer_size = CQM_TIMER_ALIGN_SCALE_NUM * CQM_TIMER_SIZE_32;
	struct tag_cqm_buf *cla_z_buf = &cla_table->cla_z_buf;
	u32 timer_page_num, i;
	u32 buffer_index = 0;
	s32 ret;

	/* During CQM capability initialization, ensure that the basic size of
	 * the timer buffer page does not exceed 128 x 4 KB. Otherwise,
	 * clearing the timer buffer of the function is complex.
	 */
	timer_page_num = timer_buffer_size /
			 (PAGE_SIZE << cla_table->trunk_order);
	if (timer_page_num == 0) {
		cqm_err(handle->dev_hdl,
			"Timer clear: fail to clear timer, buffer_size=0x%x, trunk_order=0x%x\n",
			timer_buffer_size, cla_table->trunk_order);
		return;
	}

	/* Convert functionid and the functionid does not exceed the value range
	 * of the tiemr buffer.
	 */
	ret = cqm_function_timer_clear_getindex(ex_handle, &buffer_index,
						function_id, timer_page_num,
						cla_z_buf);
	if (ret == CQM_FAIL) {
		cqm_err(handle->dev_hdl,
			CQM_FUNCTION_FAIL(cqm_function_timer_clear_getindex));
		return;
	}

	if (cla_table->cla_lvl == CQM_CLA_LVL_1 ||
	    cla_table->cla_lvl == CQM_CLA_LVL_2) {
		for (i = buffer_index * timer_page_num;
		     i < (buffer_index * timer_page_num + timer_page_num); i++)
			memset((u8 *)(cla_z_buf->buf_list[i].va), 0,
			       (PAGE_SIZE << cla_table->trunk_order));
	} else {
		cqm_err(handle->dev_hdl, "Timer clear: timer cla lvl: %u, cla_z_buf_num=0x%x\n",
			cla_table->cla_lvl, cla_z_buf->buf_number);
		cqm_err(handle->dev_hdl,
			"Timer clear: buf_i=0x%x, buf_size=0x%x, page_num=0x%x, order=0x%x\n",
			buffer_index, timer_buffer_size, timer_page_num,
			cla_table->trunk_order);
	}
}

/**
 * cqm_function_timer_clear - Clear the timer buffer based on the function ID.
 *                            The function ID starts from 0 and the timer buffer is arranged
 *                            in sequence by function ID.
 * @ex_handle: device pointer that represents the PF
 * @function_id: the function id of CQM timer
 */
void cqm_function_timer_clear(void *ex_handle, u32 function_id)
{
	/* The timer buffer of one function is 32B*8wheel*2048spoke=128*4k */
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_cqm_cla_table *cla_table = NULL;
	struct tag_cqm_handle *cqm_handle = NULL;
	int loop, i;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: ex_handle is null\n", __func__);
		return;
	}

	atomic_inc(&handle->hw_stats.cqm_stats.cqm_func_timer_clear_cnt);

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	if (unlikely(!cqm_handle)) {
		pr_err("[CQM]%s: cqm_handle is null\n", __func__);
		return;
	}

	if (cqm_handle->func_capability.lb_mode == CQM_LB_MODE_1 ||
	    cqm_handle->func_capability.lb_mode == CQM_LB_MODE_2) {
		cla_table = &cqm_handle->bat_table.timer_entry[0];
		loop = CQM_LB_SMF_MAX;
	} else {
		cla_table = cqm_cla_table_get(&cqm_handle->bat_table, CQM_BAT_ENTRY_T_TIMER);
		loop = 1;
	}

	if (unlikely(!cla_table)) {
		pr_err("[CQM]%s: cla_table is null\n", __func__);
		return;
	}
	for (i = 0; i < loop; i++) {
		cqm_clear_timer(ex_handle, function_id, handle, cla_table);
		cla_table++;
	}
}
EXPORT_SYMBOL(cqm_function_timer_clear);

/**
 * cqm_function_hash_buf_clear - clear hash buffer based on global function_id
 * @ex_handle: device pointer that represents the PF
 * @global_funcid: the function id of clear hash buf
 */
void cqm_function_hash_buf_clear(void *ex_handle, s32 global_funcid)
{
	struct hinic3_hwdev *handle = (struct hinic3_hwdev *)ex_handle;
	struct tag_cqm_func_capability *func_cap = NULL;
	struct tag_cqm_cla_table *cla_table = NULL;
	struct tag_cqm_handle *cqm_handle = NULL;
	struct tag_cqm_buf *cla_z_buf = NULL;
	s32 fake_funcid;
	u32 i;

	if (unlikely(!ex_handle)) {
		pr_err("[CQM]%s: ex_handle is null\n", __func__);
		return;
	}

	atomic_inc(&handle->hw_stats.cqm_stats.cqm_func_hash_buf_clear_cnt);

	cqm_handle = (struct tag_cqm_handle *)(handle->cqm_hdl);
	if (unlikely(!cqm_handle)) {
		pr_err("[CQM]%s: cqm_handle is null\n", __func__);
		return;
	}
	func_cap = &cqm_handle->func_capability;

	/* fake vf adaption, switch to corresponding VF. */
	if (func_cap->fake_func_type == CQM_FAKE_FUNC_PARENT) {
		fake_funcid = global_funcid -
			      (s32)(func_cap->fake_cfg[0].child_func_start);
		cqm_info(handle->dev_hdl, "fake_funcid =%d\n", fake_funcid);
		if (fake_funcid < 0 || fake_funcid >= CQM_FAKE_FUNC_MAX) {
			cqm_err(handle->dev_hdl, CQM_WRONG_VALUE(fake_funcid));
			return;
		}

		cqm_handle = cqm_handle->fake_cqm_handle[fake_funcid];
	}

	cla_table = cqm_cla_table_get(&cqm_handle->bat_table,
				      CQM_BAT_ENTRY_T_HASH);
	if (unlikely(!cla_table)) {
		pr_err("[CQM]%s: cla_table is null\n", __func__);
		return;
	}
	cla_z_buf = &cla_table->cla_z_buf;

	for (i = 0; i < cla_z_buf->buf_number; i++)
		memset(cla_z_buf->buf_list[i].va, 0, cla_z_buf->buf_size);
}
EXPORT_SYMBOL(cqm_function_hash_buf_clear);

void cqm_srq_used_rq_container_delete(struct tag_cqm_object *object, u8 *container)
{
	struct tag_cqm_queue *common = container_of(object, struct tag_cqm_queue, object);
	struct tag_cqm_nonrdma_qinfo *qinfo = container_of(common, struct tag_cqm_nonrdma_qinfo,
							   common);
	u32 link_wqe_offset = qinfo->wqe_per_buf * qinfo->wqe_size;
	struct tag_cqm_handle *cqm_handle = (struct tag_cqm_handle *)(common->object.cqm_handle);
	struct hinic3_hwdev *handle = cqm_handle->ex_handle;
	struct tag_cqm_srq_linkwqe *srq_link_wqe = NULL;
	dma_addr_t addr;

	/* 1. Obtain the current container pa through link wqe table,
	 * unmap pa
	 */
	srq_link_wqe = (struct tag_cqm_srq_linkwqe *)(container + link_wqe_offset);
	/* shift right by 2 bits to get the length of dw(4B) */
	cqm_swab32((u8 *)(srq_link_wqe), sizeof(struct tag_cqm_linkwqe) >> 2);

	addr = CQM_ADDR_COMBINE(srq_link_wqe->current_buffer_gpa_h,
				srq_link_wqe->current_buffer_gpa_l);
	if (addr == 0) {
		cqm_err(handle->dev_hdl, "Rq container del: buffer physical addr is null\n");
		return;
	}
	pci_unmap_single(cqm_handle->dev, addr, qinfo->container_size,
			 PCI_DMA_BIDIRECTIONAL);

	/* 2. Obtain the current container va through link wqe table, free va */
	addr = CQM_ADDR_COMBINE(srq_link_wqe->current_buffer_addr_h,
				srq_link_wqe->current_buffer_addr_l);
	if (addr == 0) {
		cqm_err(handle->dev_hdl, "Rq container del: buffer virtual addr is null\n");
		return;
	}
	kfree((void *)addr);
}
EXPORT_SYMBOL(cqm_srq_used_rq_container_delete);

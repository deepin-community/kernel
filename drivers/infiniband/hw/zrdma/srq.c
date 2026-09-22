// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#include "osdep.h"
#include "status.h"
#include "hmc.h"
#include "defs.h"
#include "type.h"
#include "ws.h"
#include "protos.h"
#include "vf.h"
#include "virtchnl.h"
#include "icrdma_hw.h"
#include "main.h"
#include "srq.h"

static unsigned int ft_debug_srq_msg;
module_param(ft_debug_srq_msg, uint, 0444);
MODULE_PARM_DESC(ft_debug_srq_msg, "ft_debug_srq_msg =1, printk srq info");

/**
 * zxdh_get_srq_wqe_shift - get shift count for maximum srq wqe size
 * @uk_attrs: srq HW attributes
 * @sge: Maximum Scatter Gather Elements wqe
 * @shift: Returns the shift needed based on sge
 *
 * Shift can be used to left shift the srq wqe size based on number of SGEs.
 * For 1 SGE, shift = 1 (wqe size of 2*16 bytes).
 * For 2 or 3 SGEs, shift = 2 (wqe size of 4*16 bytes).
 * For 4-7 SGE's Shift of 3.
 *  For 8-15 SGE's Shift of 4 otherwise (wqe size of 512 bytes).
 */
static void zxdh_get_srq_wqe_shift(struct zxdh_uk_attrs *uk_attrs, u32 sge,
				   u8 *shift)
{
	*shift = 0; //16bytes RQE, need to confirm configuration
	if (sge < 2)
		*shift = 1;
	else if (sge < 4)
		*shift = 2;
	else if (sge < 8)
		*shift = 3;
	else if (sge < 16)
		*shift = 4;
	else
		*shift = 5;
}

/**
 * zxdh_srq_round_up - return round up srq wq depth
 * @wqdepth: wq depth in quanta to round up
 */
static int zxdh_srq_round_up(u32 wqdepth)
{
	int scount = 1;

	for (wqdepth--; scount <= 16; scount *= 2)
		wqdepth |= wqdepth >> scount;

	return ++wqdepth;
}

/*
 * zxdh_get_srqdepth - get SRQ depth (quanta)
 * @max_hw_rq_quanta: HW SRQ size limit
 * @srq_size: SRQ size
 * @shift: shift which determines size of WQE
 * @srqdepth: depth of SRQ
 */
static int zxdh_get_srqdepth(u32 max_hw_srq_quanta, u32 srq_size, u8 shift,
			     u32 *srqdepth)
{
	*srqdepth = zxdh_srq_round_up((srq_size << shift) + ZXDH_SRQ_RSVD);

	if (*srqdepth < (ZXDH_QP_SW_MIN_WQSIZE << shift))
		*srqdepth = ZXDH_QP_SW_MIN_WQSIZE << shift;
	else if ((*srqdepth >> shift) > max_hw_srq_quanta) {
		pr_err("[zxdh_rdma] %s[%d]: (*srqdepth >> shift) > max_hw_srq_quanta, srqdepth=%d, shift=%d, max_hw_srq_quanta=%d\n", __func__, __LINE__, (*srqdepth >> shift), shift, max_hw_srq_quanta);
		return -EINVAL;
	}

	return 0;
}

static __le64 *zxdh_get_srq_wqe(struct zxdh_srq *srq, int wqe_index)
{
	struct zxdh_srq_uk *srq_uk;
	__le64 *wqe;

	srq_uk = &srq->sc_srq.srq_uk;
	wqe = srq_uk->srq_base[wqe_index * srq_uk->srq_wqe_size_multiplier].elem;
	return wqe;
}

//each srq index occupies 2 Bytes
static __le16 *zxdh_get_srq_list_wqe(struct zxdh_srq *srq, u16 *idx)
{
	struct zxdh_srq_uk *srq_uk;
	__le16 *wqe;
	u16 wqe_idx;

	srq_uk = &srq->sc_srq.srq_uk;
	wqe_idx = ZXDH_RING_CURRENT_TAIL(srq_uk->srq_list_ring);
	dma_wmb(); /* make sure shadow area is updated before moving tail */
	ZXDH_RING_MOVE_TAIL(srq_uk->srq_list_ring);
	*idx = ZXDH_RING_CURRENT_TAIL(srq_uk->srq_list_ring);

	if (!(*idx))
		srq_uk->srq_list_polarity = !srq_uk->srq_list_polarity;

	wqe = &srq->sc_srq.srq_uk.srq_list_base[wqe_idx];

	return wqe;
}

void zxdh_free_srq_wqe(struct zxdh_srq_uk *srq, int wqe_index)
{
	struct zxdh_srq *iwsrq;
	struct zxdh_sc_srq *sc_srq;
	unsigned long flags;
	__le64 *wqe;
	u64 hdr;

	sc_srq = container_of(srq, struct zxdh_sc_srq, srq_uk);
	iwsrq = container_of(sc_srq, struct zxdh_srq, sc_srq);
	/* always called with interrupts disabled. */
	spin_lock_irqsave(&iwsrq->lock, flags);
	wqe = zxdh_get_srq_wqe(iwsrq, srq->srq_ring.tail);

	srq->srq_ring.tail = wqe_index;
	hdr = FIELD_PREP(IRDMAQPSRQ_NEXT_WQE_INDEX, wqe_index);

	dma_wmb(); /* make sure WQE is written before valid bit is set */
	set_64bit_val(wqe, 0, hdr);

	spin_unlock_irqrestore(&iwsrq->lock, flags);
}

/**
 * zxdh_setup_kmode_srq - setup initialization for kernel mode srq
 * @iwdev: iwarp device
 * @iwsrq: srq ptr (user or kernel)
 * @info: initialize info to return
 * @init_attr: Initial SRQ create attributes
 */
static int zxdh_setup_kmode_srq(struct zxdh_device *iwdev,
				struct zxdh_srq *iwsrq,
				struct zxdh_srq_init_info *info,
				struct ib_srq_init_attr *init_attr)
{
	struct zxdh_dma_mem *mem = &iwsrq->kmem;
	struct zxdh_dma_mem *mem_list = &iwsrq->kmem_list;
	struct zxdh_dma_mem *mem_db = &iwsrq->kmem_db;
	u32 srqdepth;
	u8 srqshift;
	u32 srq_size;
	u32 srq_list_size;
	u32 db_size;
	u32 log2_srq_size;
	int status;
	struct zxdh_srq_uk_init_info *ukinfo = &info->srq_uk_init_info;
	struct zxdh_uk_attrs *uk_attrs = &iwdev->rf->sc_dev.hw_attrs.uk_attrs;
    struct zxdh_pci_f *rf = iwdev->rf;

	//get shift count for maximum wqe size
	zxdh_get_srq_wqe_shift(uk_attrs, ukinfo->max_srq_frag_cnt, &srqshift);

	//get SRQ depth (quanta)
	status = zxdh_get_srqdepth(uk_attrs->max_hw_srq_quanta,
				   ukinfo->srq_size, srqshift, &srqdepth);
	if (status)
		return status;

	iwsrq->ksrq.srq_wrid_mem = kcalloc(ukinfo->srq_size,
					   sizeof(*iwsrq->ksrq.srq_wrid_mem),
					   GFP_KERNEL);
	if (!iwsrq->ksrq.srq_wrid_mem) {
		pr_err("[zxdh_rdma] %s[%d]: srq_wrid_mem kcalloc failed\n", __func__, __LINE__);
		return -ENOMEM;
	}

	ukinfo->srq_wrid_array = iwsrq->ksrq.srq_wrid_mem;
	srq_size = srqdepth * ZXDH_SRQ_WQE_MIN_SIZE;
	ukinfo->srq_size = srqdepth >> srqshift;
	log2_srq_size = roundup_pow_of_two(ukinfo->srq_size);
	log2_srq_size = order_base_2(log2_srq_size);
	ukinfo->log2_srq_size = log2_srq_size;

	mem->size = ALIGN(srq_size, ZXDH_HW_PAGE_SIZE);
	mem->va = dma_alloc_coherent(iwdev->rf->hw.device, mem->size, &mem->pa,
				     GFP_KERNEL);
	if (!mem->va) {
		kfree(iwsrq->ksrq.srq_wrid_mem);
		iwsrq->ksrq.srq_wrid_mem = NULL;
		pr_err("[zxdh_rdma] %s[%d]: srq_wrid_mem dma_alloc_coherent failed\n", __func__, __LINE__);
		return -ENOMEM;
	}

	srq_list_size = ukinfo->srq_size * sizeof(u16);
	ukinfo->srq_list_size = ukinfo->srq_size;
	mem_list->size = ALIGN(srq_list_size, 64);
	mem_list->va = dma_alloc_coherent(iwdev->rf->hw.device, mem_list->size,
					  &mem_list->pa, GFP_KERNEL);
	if (!mem_list->va) {
		kfree(iwsrq->ksrq.srq_wrid_mem);
		iwsrq->ksrq.srq_wrid_mem = NULL;
		dma_free_coherent(iwdev->rf->hw.device, mem->size, mem->va,
				  mem->pa);
		mem->va = NULL;
		pr_err("[zxdh_rdma] %s[%d]: srq_list_mem dma_alloc_coherent failed\n", __func__, __LINE__);
		return -ENOMEM;
	}

    if (rf->rdma_srq_mem_type != USER_L2D_KERNEL_L2D && rf->rdma_srq_mem_type != USER_L2D_KERNEL_L2D_V3) { // use ddr memory
        db_size = 8;
        mem_db->size = ALIGN(db_size, 8);
        mem_db->va = dma_alloc_coherent(iwdev->rf->hw.device, mem_db->size,
                        &mem_db->pa, GFP_KERNEL);
        if (!mem_db->va) {
            kfree(iwsrq->ksrq.srq_wrid_mem);
            iwsrq->ksrq.srq_wrid_mem = NULL;
            dma_free_coherent(iwdev->rf->hw.device, mem->size, mem->va,
                    mem->pa);
            mem->va = NULL;
            dma_free_coherent(iwdev->rf->hw.device, mem_list->size,
                    mem_list->va, mem_list->pa);
            mem_list->va = NULL;
			pr_err("[zxdh_rdma] %s[%d]: srq_db_mem dma_alloc_coherent failed\n", __func__, __LINE__);
            return -ENOMEM;
        }

        *(u64 *)mem_db->va = ZXDH_SRQ_DB_INIT_VALUE;
        info->srq_db_pa = mem_db->pa;
	    init_attr->attr.max_wr = (srqdepth - ZXDH_SRQ_RSVD) >> srqshift;
    }

	ukinfo->srq_base = mem->va;
	info->srq_pa = mem->pa;
	ukinfo->srq_list_base = mem_list->va;
	info->srq_list_pa = mem_list->pa;
	ukinfo->srq_db_base = mem_db->va;

	return 0;
}

static void zxdh_srq_wqe_init(struct zxdh_srq *srq)
{
	int i;
	struct zxdh_srq_uk *srq_uk;
	__le64 *wqe;
	u64 hdr;

	srq_uk = &srq->sc_srq.srq_uk;

	for (i = srq_uk->srq_ring.head; i < srq_uk->srq_ring.tail; i++) {
		wqe = zxdh_get_srq_wqe(srq, i);
		hdr = FIELD_PREP(IRDMAQPSRQ_NEXT_WQE_INDEX, (i + 1));

		dma_wmb(); /* make sure WQE is written before valid bit is set */
		set_64bit_val(wqe, 0, hdr);
	}
}

static int zxdh_validate_srq_attrs(struct ib_srq_init_attr *init_attr,
				   struct zxdh_device *iwdev)
{
	struct zxdh_sc_dev *dev = &iwdev->rf->sc_dev;
	struct zxdh_uk_attrs *uk_attrs = &dev->hw_attrs.uk_attrs;

    if (iwdev->rf->use_blue_flame_flag) {
        if (init_attr->attr.max_sge > uk_attrs->max_hw_rq_sges) {
            pr_err("[zxdh_rdma] %s[%d]: "
                "max_sge:%u is over max_hw_rq_sges:%u in blue_flame mode\n"
                "If you want to use RQ max SGE of %u, set use_blue_flame=false via dhtool\n",
                __func__, __LINE__, init_attr->attr.max_sge,
                uk_attrs->max_hw_rq_sges, uk_attrs->max_hw_wq_frags);
            return -EINVAL;
        }
    } else {
		if (init_attr->attr.max_sge > uk_attrs->max_hw_wq_frags) {
			pr_err("[zxdh_rdma] %s[%d]: max_sge > max_hw_wq_frags, max_sge=%d, max_hw_wq_frags=%d\n", __func__, __LINE__, init_attr->attr.max_sge, uk_attrs->max_hw_wq_frags);
			return -EINVAL;
		}
    }

	if (init_attr->attr.max_wr > uk_attrs->max_hw_srq_quanta) {
		pr_err("[zxdh_rdma] %s[%d]: max_wr > max_hw_srq_quanta, max_wr=%d, max_hw_srq_quanta=%d\n", __func__, __LINE__, init_attr->attr.max_wr, uk_attrs->max_hw_srq_quanta);
		return -EINVAL;
	}

	if (init_attr->attr.srq_limit > init_attr->attr.max_wr) {
		pr_err("[zxdh_rdma] %s[%d]: srq_limit > max_wr, srq_limit=%d, max_wr=%d\n", __func__, __LINE__, init_attr->attr.srq_limit, init_attr->attr.max_wr);
		return -EINVAL;
	}

	if (init_attr->srq_type != IB_SRQT_BASIC) {
		pr_err("[zxdh_rdma] %s[%d]: srq_type != IB_SRQT_BASIC, srq_type=%d\n", __func__, __LINE__, init_attr->srq_type);
		return -EOPNOTSUPP;
	}

	return 0;
}

/**
 * zxdh_free_srq_rsrc - free up memory resources for srq
 * @iwsrq: srq ptr (user or kernel)
 */
static void zxdh_free_srq_rsrc(struct zxdh_srq *iwsrq)
{
	struct zxdh_device *iwdev = iwsrq->iwdev;
	struct zxdh_pci_f *rf = iwdev->rf;
	struct zxdh_sc_dev *dev;
	u32 srq_num;

	dev = &rf->sc_dev;
	srq_num = iwsrq->ibsrq.ext.xrc.srq_num - dev->base_srqn;
	zxdh_free_rsrc(rf, rf->allocated_srqs, srq_num);

	if (!iwsrq->user_mode) {
		kfree(iwsrq->ksrq.srq_wrid_mem);
		iwsrq->ksrq.srq_wrid_mem = NULL;
		dma_free_coherent(iwdev->rf->hw.device, iwsrq->kmem.size,
				  iwsrq->kmem.va, iwsrq->kmem.pa);
		iwsrq->kmem.va = NULL;
		dma_free_coherent(iwdev->rf->hw.device, iwsrq->kmem_list.size,
				  iwsrq->kmem_list.va, iwsrq->kmem_list.pa);
		iwsrq->kmem_list.va = NULL;
        if (rf->rdma_srq_mem_type != USER_L2D_KERNEL_L2D && rf->rdma_srq_mem_type != USER_L2D_KERNEL_L2D_V3) { // use ddr memory
		    dma_free_coherent(iwdev->rf->hw.device, iwsrq->kmem_db.size,
				  iwsrq->kmem_db.va, iwsrq->kmem_db.pa);
        }
		iwsrq->kmem_db.va = NULL;
	}
}

/**
 * zxdh_uk_srq_init - initialize srq
 * @srq: hw srq (user and kernel)
 * @info: srq initialization info
 *
 * initializes the vars used in both user and kernel mode.
 * size of the wqe depends on numbers of max. fragements
 * allowed. Then size of wqe * the number of wqes should be the
 * amount of memory allocated for srq.
 */
static int zxdh_uk_srq_init(struct zxdh_srq_uk *srq,
			    struct zxdh_srq_uk_init_info *info)
{
	u32 srq_ring_size;
	u8 srqshift;

	srq->uk_attrs = info->uk_attrs;
	if (info->max_srq_frag_cnt > srq->uk_attrs->max_hw_wq_frags) {
		pr_err("[zxdh_rdma] %s[%d]: max_srq_frag_cnt > max_hw_wq_frags, max_srq_frag_cnt=%d, max_hw_wq_frags=%d\n", __func__, __LINE__, 
			info->max_srq_frag_cnt, srq->uk_attrs->max_hw_wq_frags);
		return -EINVAL;
	}

	zxdh_get_srq_wqe_shift(srq->uk_attrs, info->max_srq_frag_cnt,
			       &srqshift);
	srq->srq_base = info->srq_base;
	srq->srq_list_base = info->srq_list_base;
	srq->srq_db_base = info->srq_db_base;
	srq->srq_wrid_array = info->srq_wrid_array;
	srq->srq_id = info->srq_id;
	srq->srq_size = info->srq_size;
	srq->log2_srq_size = info->log2_srq_size;
	srq->srq_list_size = info->srq_list_size;
	srq->max_srq_frag_cnt = info->max_srq_frag_cnt;
	srq_ring_size = srq->srq_size;
	srq->srq_wqe_size = srqshift;
	srq->srq_wqe_size_multiplier = 1 << srqshift;
	ZXDH_RING_INIT(srq->srq_ring, srq_ring_size);
	ZXDH_RING_INIT(srq->srq_list_ring, srq->srq_list_size);
	srq->srq_ring.tail = srq->srq_size - 1;
	//initial value is 0, initial use is 1
	srq->srq_list_polarity = 1;

	return 0;
}

/**
 * zxdh_sc_srq_init - initialize srq
 * @srq: sc srq
 * @info: initialization srq info
 */
static int zxdh_sc_srq_init(struct zxdh_sc_srq *srq,
			    struct zxdh_srq_init_info *info)
{
	int ret_code;
	u32 pble_obj_cnt;
    struct zxdh_sc_dev *dev = srq->dev;
	struct zxdh_pci_f *rf = container_of(dev, struct zxdh_pci_f, sc_dev);

	if (info->srq_uk_init_info.max_srq_frag_cnt >
	    info->pd->dev->hw_attrs.uk_attrs.max_hw_wq_frags) {
		pr_err("[zxdh_rdma] %s[%d]: max_srq_frag_cnt > max_hw_wq_frags, max_srq_frag_cnt=%d, max_hw_wq_frags=%d\n", __func__, __LINE__, 
			info->srq_uk_init_info.max_srq_frag_cnt, info->pd->dev->hw_attrs.uk_attrs.max_hw_wq_frags);
		return -EINVAL;
	}

	srq->srq_pa = info->srq_pa;
	srq->srq_list_pa = info->srq_list_pa;

    if (rf->rdma_srq_mem_type == USER_L2D_KERNEL_L2D || rf->rdma_srq_mem_type == USER_L2D_KERNEL_L2D_V3) {
        info->srq_db_pa = rf->chip_srq_base_paddr + ((info->srq_uk_init_info.srq_id - dev->base_srqn) % rf->max_srq) * ZXDH_SIZE_PER_SRQ_DB;
        pr_debug("[zxdh_rdma] %s[%d]: srq use L2D memory! l2d_base_paddr=0x%llx srq_db_pa=0x%llx srq_id=%d ep_id=%d pf_id=%d vf_id=%d ftype=%d max_srq=%d\n", __func__, __LINE__,
            rf->chip_srq_base_paddr, info->srq_db_pa, info->srq_uk_init_info.srq_id, rf->ep_id, rf->pf_id, rf->vf_id, rf->ftype, rf->max_srq);
    } else {
        info->srq_db_pa = 0;
        pr_err("[zxdh_rdma] %s[%d]: rdma srq can not use ddr! ep_id=%d pf_id=%d vf_id=%d ftype=%d srq_mem_type=%d\n", __func__, __LINE__,
            rf->ep_id, rf->pf_id, rf->vf_id, rf->ftype, rf->rdma_srq_mem_type);
        return -EPERM;
    }

	srq->srq_db_pa = info->srq_db_pa;
	srq->pd = info->pd;
	srq->virtual_map = info->virtual_map;
	srq->list_virtual_map = info->list_virtual_map;
	srq->pbl_chunk_size = info->pbl_chunk_size;
	srq->list_pbl_chunk_size = info->list_pbl_chunk_size;
	srq->first_pm_pbl_idx = info->first_pm_pbl_idx;
	srq->list_first_pm_pbl_idx = info->list_first_pm_pbl_idx;
	srq->srq_limit = info->srq_limit;
	ret_code = zxdh_uk_srq_init(&srq->srq_uk, &info->srq_uk_init_info);
	if (ret_code)
		return ret_code;

	pble_obj_cnt = info->pd->dev->hmc_info->hmc_obj[ZXDH_HMC_IW_PBLE].cnt;

	if ((info->virtual_map && info->srq_pa >= pble_obj_cnt) ||
	    (info->list_virtual_map && info->srq_list_pa >= pble_obj_cnt)) {
		pr_err("[zxdh_rdma] %s[%d]: virtual_map is true and srq_pa or srq_list_pa >= pble_obj_cnt, srq_pa=%lld, srq_list_pa=%lld, pble_obj_cnt=%d\n", __func__, __LINE__, 
			info->srq_pa, info->srq_list_pa, pble_obj_cnt);
		return -EINVAL;
	}
	srq->hw_srq_size = zxdh_get_encoded_wqe_size(srq->srq_uk.srq_ring.size,
						     ZXDH_QUEUE_TYPE_SQ_RQ);

	return 0;
}

static int zxdh_cqp_create_srq_cmd(struct zxdh_srq *iwsrq)
{
	struct zxdh_pci_f *rf = iwsrq->iwdev->rf;
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_create_srq_info *srq_info;
	int status;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request) {
		pr_err("[zxdh_rdma] %s[%d]: cqp_request alloc failed\n", __func__, __LINE__);
		return -ENOMEM;
	}

	cqp_info = &cqp_request->info;
	cqp_info->cqp_cmd = ZXDH_OP_SRQ_CREATE;
	srq_info = &cqp_request->info.in.u.srq_create.info;
	memset(srq_info, 0, sizeof(*srq_info));
	srq_info->state = ZXDH_SRQ_STATE_GOOD;
	cqp_info->post_sq = 1;
	cqp_info->in.u.srq_create.srq = &iwsrq->sc_srq;
	cqp_info->in.u.srq_create.scratch = (uintptr_t)cqp_request;
	status = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);

	return status;
}

#ifdef ZRDMA_CREATE_SRQ_VER_1
/**
 * zxdh_create_srq - create srq
 * @ibpd: ptr of ibpd
 * @init_attr: attributes for srq
 * @udata: user data for create srq
 */
struct ib_srq *zxdh_create_srq(struct ib_pd *ibpd,
			       struct ib_srq_init_attr *init_attr,
			       struct ib_udata *udata)
{
	struct zxdh_device *iwdev = to_iwdev(ibpd->device);
	struct zxdh_pd *iwpd = to_iwpd(ibpd);
	struct zxdh_srq *iwsrq;
	struct zxdh_pci_f *rf = iwdev->rf;
	struct zxdh_create_srq_req req;
	struct zxdh_create_srq_resp uresp = {};
	u32 srq_num = 0;
	int ret;
	int err_code;
	int srq_size;
	u32 log2_srq_size;
	struct zxdh_sc_srq *srq;
	struct zxdh_srq_init_info init_info = {};
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_uk_attrs *uk_attrs = &dev->hw_attrs.uk_attrs;
	struct zxdh_ucontext *ucontext;
	unsigned long flags;
	struct zxdh_srq_mr *srqmr;

	if (!rf) {
		pr_err("[zxdh_rdma] %s[%d]: rf is null\n", __func__, __LINE__);
		err_code = -EINVAL;
		return ERR_PTR(err_code);
	}

    if (rf->rdma_srq_mem_type != USER_L2D_KERNEL_L2D && rf->rdma_srq_mem_type != USER_L2D_KERNEL_L2D_V3) {
        pr_err("[zxdh_rdma] %s[%d]: rdma srq can not use ddr! ep_id=%d pf_id=%d vf_id=%d ftype=%d srq_mem_type=%d\n", __func__, __LINE__,
            rf->ep_id, rf->pf_id, rf->vf_id, rf->ftype, rf->rdma_srq_mem_type);
        return ERR_PTR(-ENOMEM);
    }

	err_code = zxdh_validate_srq_attrs(init_attr, iwdev);
	if (err_code) {
		pr_err("[zxdh_rdma] %s[%d]: validate srq attrs failed, err_code=%d\n", __func__, __LINE__, err_code);
		return ERR_PTR(err_code);
	}

	iwsrq = kzalloc(sizeof(*iwsrq), GFP_KERNEL);
	if (!iwsrq) {
		pr_err("[zxdh_rdma] %s[%d]: srq kzalloc memory failed\n", __func__, __LINE__);
		return ERR_PTR(-ENOMEM);
	}

	srq_size = init_attr->attr.max_wr;
	log2_srq_size = order_base_2(srq_size);

	init_info.srq_uk_init_info.srq_size = srq_size;
	init_info.srq_uk_init_info.log2_srq_size = log2_srq_size;
	init_info.srq_uk_init_info.max_srq_frag_cnt = init_attr->attr.max_sge;
	init_info.srq_uk_init_info.srq_limit = init_attr->attr.srq_limit;
	init_info.srq_limit = init_attr->attr.srq_limit;
	init_info.srq_uk_init_info.uk_attrs = uk_attrs;

	if (rf->rdma_srq_mem_type == USER_L2D_KERNEL_L2D_V3) {
	    err_code = zxdh_alloc_rsrc(rf, rf->allocated_srqs, rf->max_srq,
				   &srq_num, &rf->next_srq);
	} else {
		if(rf->ftype) {
			err_code = -EOVERFLOW;
			pr_err("[zxdh_rdma] %s[%d]: vf srq is not supported when userspace_ver is under ZXDH_CONTEXT_VER_V3\n", __func__, __LINE__);

		}else {
			err_code = zxdh_alloc_rsrc(rf, rf->allocated_srqs, ZXDH_PF_MAX_SRQ_NUM_USE_L2D,
					   &srq_num, &rf->next_srq);
		}
	}
	pr_debug("[zxdh_rdma] %s[%d]: max_srq=%d srq_num=%d\n", __func__, __LINE__, rf->max_srq, srq_num);
	if (err_code) {
		pr_err("[zxdh_rdma] %s[%d]: srq alloc rsrc failed, err_code=%d\n", __func__, __LINE__, err_code);
		goto error;
	}
	iwsrq->iwdev = iwdev;
	iwsrq->ibsrq.ext.xrc.srq_num = dev->base_srqn + srq_num;
	srq = &iwsrq->sc_srq;
	srq->dev = dev;
	srq->back_srq = iwsrq;
	init_info.pd = &iwpd->sc_pd;
	init_info.srq_uk_init_info.srq_id = dev->base_srqn + srq_num;
	iwsrq->max_wr = srq_size;
	iwsrq->max_sge = init_attr->attr.max_sge;
	iwsrq->srq_limit = init_attr->attr.srq_limit;
	iwsrq->srq_compl_ctx = (uintptr_t)srq;
	iwsrq->sc_srq.srq_compl_ctx = iwsrq->srq_compl_ctx;
	refcount_set(&iwsrq->refcnt, 1);
	spin_lock_init(&iwsrq->lock);

	if (udata) {
		err_code = ib_copy_from_udata(&req, udata,
					      min(sizeof(req), udata->inlen));
		if (err_code) {
			zxdh_dbg(iwdev_to_idev(iwdev),
				 "VERBS: ib_copy_from_data fail\n");
			goto free_rsrc;
		}
		iwsrq->user_mode = 1;
		init_info.srq_uk_init_info.srq_base =
			(void *)((unsigned long)req.user_wqe_bufs);

		ucontext = kc_rdma_udata_to_drv_context(ibpd, udata);

		spin_lock_irqsave(&ucontext->srq_reg_mem_list_lock, flags);
		iwsrq->iwpbl = zxdh_get_pbl((unsigned long)req.user_wqe_bufs,
					    &ucontext->srq_reg_mem_list);
		spin_unlock_irqrestore(&ucontext->srq_reg_mem_list_lock, flags);

		if (!iwsrq->iwpbl) {
			err_code = -ENODATA;
			zxdh_dbg(iwdev_to_idev(iwdev), "VERBS: no pbl info\n");
			goto free_rsrc;
		}
		srqmr = &iwsrq->iwpbl->srq_mr;

		//srq wqe addr configuration
		if (iwsrq->iwpbl->pbl_allocated) {
			init_info.virtual_map = true;
			init_info.pbl_chunk_size = 1;
			init_info.first_pm_pbl_idx = srqmr->srq_pbl.idx;
			init_info.srq_pa = srqmr->srq_pbl.idx;
		} else {
			init_info.srq_pa = srqmr->srq_pbl.addr;
			init_info.virtual_map = false;
			init_info.pbl_chunk_size = 0;
		}

		//srq wqe idx addr configuration
		if (iwsrq->iwpbl->pbl_allocated) {
			init_info.list_virtual_map = true;
			init_info.list_pbl_chunk_size = 1;
			init_info.list_first_pm_pbl_idx =
				srqmr->srq_list_pbl.idx;
			init_info.srq_list_pa = srqmr->srq_list_pbl.idx;
		} else {
			init_info.srq_list_pa = srqmr->srq_list_pbl.addr;
			init_info.list_virtual_map = false;
			init_info.list_pbl_chunk_size = 0;
		}

		//srq wqe db addr configuration
		init_info.srq_db_pa = srqmr->db_addr;
		init_info.db_virtual_map = false;
		init_info.db_pbl_chunk_size = 0;
	} else {
		err_code = zxdh_setup_kmode_srq(iwdev, iwsrq, &init_info,
						init_attr);
	}

	if (err_code) {
		zxdh_dbg(iwdev_to_idev(iwdev), "VERBS: setup srq failed\n");
		goto free_rsrc;
	}

	ret = zxdh_sc_srq_init(srq, &init_info);
	if (ret) {
		err_code = -EPROTO;
		zxdh_dbg(iwdev_to_idev(iwdev), "VERBS: srq_init fail\n");
		goto free_rsrc;
	}

	if (!udata)
		zxdh_srq_wqe_init(iwsrq);

	err_code = zxdh_cqp_create_srq_cmd(iwsrq);
	if (err_code) {
		pr_err("[zxdh_rdma] %s[%d]: create srq failed, err_code=%d\n", __func__, __LINE__, err_code);
		goto free_rsrc;
	}

	if (udata) {
		uresp.srq_size = srq_size;
		uresp.srq_list_size = srq_size;
		uresp.srq_id = dev->base_srqn + srq_num;
        uresp.base_srqn = dev->base_srqn;

		err_code = ib_copy_to_udata(udata, &uresp,
					    min(sizeof(uresp), udata->outlen));
		if (err_code) {
			zxdh_dbg(iwdev_to_idev(iwdev),
				 "VERBS: copy_to_udata failed\n");
#ifdef ZRDMA_DESTROY_SRQ_VER_1
			zxdh_destroy_srq(&iwsrq->ibsrq);
#else
			zxdh_destroy_srq(&iwsrq->ibsrq, udata);
#endif
			goto free_rsrc;
		}
	}
	iwsrq->state = ZXDH_SRQ_STATE_GOOD;
	rf->srq_table[srq_num] = iwsrq;
	init_completion(&iwsrq->free_srq);
	return &iwsrq->ibsrq;
free_rsrc:
	zxdh_free_srq_rsrc(iwsrq);
error:
	kfree(iwsrq);
	return ERR_PTR(err_code);
}
#endif

#ifdef ZRDMA_CREATE_SRQ_VER_2
/**
 * zxdh_create_srq - create srq
 * @ibsrq: ptr of srq
 * @init_attr: attributes for srq
 * @udata: user data for create srq
 */
int zxdh_create_srq(struct ib_srq *ibsrq, struct ib_srq_init_attr *init_attr,
		    struct ib_udata *udata)
{
	struct ib_pd *ibpd = ibsrq->pd;
	struct zxdh_pd *iwpd = to_iwpd(ibpd);
	struct zxdh_device *iwdev = to_iwdev(ibpd->device);
	struct zxdh_srq *iwsrq = to_iwsrq(ibsrq);
	struct zxdh_pci_f *rf = iwdev->rf;
	struct zxdh_create_srq_req req;
	struct zxdh_create_srq_resp uresp = {0};
	u32 srq_num = 0;
	int ret;
	int err_code;
	int srq_size;
	u32 log2_srq_size;
	struct zxdh_sc_srq *srq;
	struct zxdh_srq_init_info init_info = {0};
	struct zxdh_sc_dev *dev = &rf->sc_dev;
	struct zxdh_uk_attrs *uk_attrs = &dev->hw_attrs.uk_attrs;
	struct zxdh_ucontext *ucontext;
	unsigned long flags;
	struct zxdh_srq_mr *srqmr;

	if (!rf) {
		pr_err("[zxdh_rdma] %s[%d]: rf is null\n", __func__, __LINE__);
		return -EINVAL; 
	}

    if (rf->rdma_srq_mem_type != USER_L2D_KERNEL_L2D && rf->rdma_srq_mem_type != USER_L2D_KERNEL_L2D_V3) {
        pr_err("[zxdh_rdma] %s[%d]: rdma srq can not use ddr! ep_id=%d pf_id=%d vf_id=%d ftype=%d srq_mem_type=%d\n", __func__, __LINE__,
            rf->ep_id, rf->pf_id, rf->vf_id, rf->ftype, rf->rdma_srq_mem_type);
        return -ENOMEM;
    }

	err_code = zxdh_validate_srq_attrs(init_attr, iwdev);
	if (err_code) {
		pr_err("[zxdh_rdma] %s[%d]: validate srq attrs failed, err_code=%d\n", __func__, __LINE__, err_code);
		return err_code;
	}

	srq_size = init_attr->attr.max_wr;
	log2_srq_size = order_base_2(srq_size);

	init_info.srq_uk_init_info.srq_size = srq_size;
	init_info.srq_uk_init_info.log2_srq_size = log2_srq_size;
	init_info.srq_uk_init_info.max_srq_frag_cnt = init_attr->attr.max_sge;
	init_info.srq_uk_init_info.srq_limit = init_attr->attr.srq_limit;
	init_info.srq_limit = init_attr->attr.srq_limit;
	init_info.srq_uk_init_info.uk_attrs = uk_attrs;

	if (rf->rdma_srq_mem_type == USER_L2D_KERNEL_L2D_V3) {
		err_code = zxdh_alloc_rsrc(rf, rf->allocated_srqs, rf->max_srq,
				   &srq_num, &rf->next_srq);
	} else {
		if(rf->ftype) {
			err_code = -EOVERFLOW;
			pr_err("[zxdh_rdma] %s[%d]: vf srq is not supported when userspace_ver is under ZXDH_CONTEXT_VER_V3\n", __func__, __LINE__);

		}else {
			err_code = zxdh_alloc_rsrc(rf, rf->allocated_srqs, ZXDH_PF_MAX_SRQ_NUM_USE_L2D,
					   &srq_num, &rf->next_srq);
		}

	}
    pr_debug("[zxdh_rdma] %s[%d]: max_srq=%d srq_num=%d\n", __func__, __LINE__, rf->max_srq, srq_num);
	if (err_code) {
		pr_err("[zxdh_rdma] %s[%d]: alloc srq rsrc failed, err_code=%d\n", __func__, __LINE__, err_code);
		goto error;
	}
	iwsrq->iwdev = iwdev;
	iwsrq->ibsrq.ext.xrc.srq_num = dev->base_srqn + srq_num;
	srq = &iwsrq->sc_srq;
	srq->dev = dev;
	srq->back_srq = iwsrq;
	init_info.pd = &iwpd->sc_pd;
	init_info.srq_uk_init_info.srq_id = dev->base_srqn + srq_num;
	iwsrq->max_wr = srq_size;
	iwsrq->max_sge = init_attr->attr.max_sge;
	iwsrq->srq_limit = init_attr->attr.srq_limit;
	iwsrq->srq_compl_ctx = (uintptr_t)srq;
	iwsrq->sc_srq.srq_compl_ctx = iwsrq->srq_compl_ctx;
	refcount_set(&iwsrq->refcnt, 1);
	spin_lock_init(&iwsrq->lock);

	if (udata) {
		err_code = ib_copy_from_udata(&req, udata,
					      min(sizeof(req), udata->inlen));
		if (err_code) {
			zxdh_dbg(iwdev_to_idev(iwdev),
				 "VERBS: ib_copy_from_data fail\n");
			goto free_rsrc;
		}
		iwsrq->user_mode = 1;
		init_info.srq_uk_init_info.srq_base =
			(void *)((unsigned long)req.user_wqe_bufs);

		ucontext = kc_rdma_udata_to_drv_context(ibpd, udata);

		spin_lock_irqsave(&ucontext->srq_reg_mem_list_lock, flags);
		iwsrq->iwpbl = zxdh_get_pbl((unsigned long)req.user_wqe_bufs,
					    &ucontext->srq_reg_mem_list);
		spin_unlock_irqrestore(&ucontext->srq_reg_mem_list_lock, flags);

		if (!iwsrq->iwpbl) {
			err_code = -ENODATA;
			zxdh_dbg(iwdev_to_idev(iwdev), "VERBS: no pbl info\n");
			goto free_rsrc;
		}
		srqmr = &iwsrq->iwpbl->srq_mr;

		//srq wqe addr configuration
		if (iwsrq->iwpbl->pbl_allocated) {
			init_info.virtual_map = true;
			init_info.pbl_chunk_size = 1;
			init_info.first_pm_pbl_idx = srqmr->srq_pbl.idx;
			init_info.srq_pa = srqmr->srq_pbl.idx;
		} else {
			init_info.srq_pa = srqmr->srq_pbl.addr;
			init_info.virtual_map = false;
			init_info.pbl_chunk_size = 0;
		}

		//srq wqe idx addr configuration
		if (iwsrq->iwpbl->pbl_allocated) {
			init_info.list_virtual_map = true;
			init_info.list_pbl_chunk_size = 1;
			init_info.list_first_pm_pbl_idx =
				srqmr->srq_list_pbl.idx;
			init_info.srq_list_pa = srqmr->srq_list_pbl.idx;
		} else {
			init_info.srq_list_pa = srqmr->srq_list_pbl.addr;
			init_info.list_virtual_map = false;
			init_info.list_pbl_chunk_size = 0;
		}

		//srq wqe db addr configuration
		init_info.srq_db_pa = srqmr->db_addr;
		init_info.db_virtual_map = false;
		init_info.db_pbl_chunk_size = 0;
	} else {
		err_code = zxdh_setup_kmode_srq(iwdev, iwsrq, &init_info,
						init_attr);
	}

	if (err_code) {
		zxdh_dbg(iwdev_to_idev(iwdev), "VERBS: setup srq failed\n");
		goto free_rsrc;
	}

	ret = zxdh_sc_srq_init(srq, &init_info);
	if (ret) {
		err_code = -EPROTO;
		zxdh_dbg(iwdev_to_idev(iwdev), "VERBS: srq_init fail\n");
		goto free_rsrc;
	}

	if (!udata)
		zxdh_srq_wqe_init(iwsrq);

	err_code = zxdh_cqp_create_srq_cmd(iwsrq);
	if (err_code) {
		pr_err("[zxdh_rdma] %s[%d]: create srq failed, err_code=%d\n", __func__, __LINE__, err_code);
		goto free_rsrc;
	}

	if (udata) {
		uresp.srq_size = srq_size;
		uresp.srq_list_size = srq_size;
		uresp.srq_id = dev->base_srqn + srq_num;
        uresp.base_srqn = dev->base_srqn;

		err_code = ib_copy_to_udata(udata, &uresp,
					    min(sizeof(uresp), udata->outlen));
		if (err_code) {
			zxdh_dbg(iwdev_to_idev(iwdev),
				 "VERBS: copy_to_udata failed\n");
#ifdef ZRDMA_DESTROY_SRQ_VER_1
			zxdh_destroy_srq(&iwsrq->ibsrq);
#else
			zxdh_destroy_srq(&iwsrq->ibsrq, udata);
#endif
			goto free_rsrc;
		}
	}
	iwsrq->state = ZXDH_SRQ_STATE_GOOD;
	rf->srq_table[srq_num] = iwsrq;
	init_completion(&iwsrq->free_srq);

	return 0;
free_rsrc:
	zxdh_free_srq_rsrc(iwsrq);
error:
	return err_code;
}
#endif

void zxdh_srq_add_ref(struct ib_srq *ibsrq)
{
	struct zxdh_srq *iwsrq = to_iwsrq(ibsrq);

	refcount_inc(&iwsrq->refcnt);
}

void zxdh_srq_rem_ref(struct ib_srq *ibsrq)
{
	struct zxdh_srq *iwsrq = to_iwsrq(ibsrq);
	struct zxdh_device *iwdev = iwsrq->iwdev;
	unsigned long flags;

	spin_lock_irqsave(&iwdev->rf->srqtable_lock, flags);
	if (!refcount_dec_and_test(&iwsrq->refcnt)) {
		spin_unlock_irqrestore(&iwdev->rf->srqtable_lock, flags);
		return;
	}

	iwdev->rf->srq_table[iwsrq->ibsrq.ext.xrc.srq_num -
			     iwdev->rf->sc_dev.base_srqn] = NULL;
	spin_unlock_irqrestore(&iwdev->rf->srqtable_lock, flags);
	complete(&iwsrq->free_srq);
}

/**
 * zxdh_srq_wq_destroy - send srq destroy cqp
 * @rf: RDMA PCI function
 * @srq: hardware control srq
 */
static void zxdh_srq_wq_destroy(struct zxdh_pci_f *rf, struct zxdh_sc_srq *srq)
{
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_destroy_srq_info *srq_info;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request)
		return;

	cqp_info = &cqp_request->info;
	srq_info = &cqp_request->info.in.u.srq_destroy.info;
	cqp_info->cqp_cmd = ZXDH_OP_SRQ_DESTROY;
	srq_info->state = ZXDH_SRQ_STATE_ERROR;
	cqp_info->post_sq = 1;
	cqp_info->in.u.srq_destroy.srq = srq;
	cqp_info->in.u.srq_destroy.scratch = (uintptr_t)cqp_request;

	zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);
}

#ifdef ZRDMA_DESTROY_SRQ_VER_3
/**
 * zxdh_destroy_srq - destroy
 * @ibsrq: ptr of srq
 * @udata: user data for destroy srq
 */
int zxdh_destroy_srq(struct ib_srq *ibsrq, struct ib_udata *udata)
{
	struct zxdh_srq *iwsrq = to_iwsrq(ibsrq);
	struct zxdh_device *iwdev = iwsrq->iwdev;

	if (iwsrq->sc_srq.srq_uk.destroy_pending)
		goto free_rsrc;
	iwsrq->sc_srq.srq_uk.destroy_pending = true;

	zxdh_srq_rem_ref(&iwsrq->ibsrq);
	wait_for_completion(&iwsrq->free_srq);
	zxdh_srq_wq_destroy(iwdev->rf, &iwsrq->sc_srq);

free_rsrc:
	zxdh_free_srq_rsrc(iwsrq);

	return 0;
}
#endif
#ifdef ZRDMA_DESTROY_SRQ_VER_2
/**
 * zxdh_destroy_srq - destroy
 * @ibsrq: ptr of srq
 * @udata: user data for destroy srq
 */
void zxdh_destroy_srq(struct ib_srq *ibsrq, struct ib_udata *udata)
{
	struct zxdh_srq *iwsrq = to_iwsrq(ibsrq);
	struct zxdh_device *iwdev = iwsrq->iwdev;

	if (iwsrq->sc_srq.srq_uk.destroy_pending)
		goto free_rsrc;
	iwsrq->sc_srq.srq_uk.destroy_pending = true;

	zxdh_srq_rem_ref(&iwsrq->ibsrq);
	wait_for_completion(&iwsrq->free_srq);
	zxdh_srq_wq_destroy(iwdev->rf, &iwsrq->sc_srq);

free_rsrc:
	zxdh_free_srq_rsrc(iwsrq);
}
#endif

#ifdef ZRDMA_DESTROY_SRQ_VER_1
/**
 * zxdh_destroy_srq - destroy
 * @ibsrq: ptr of srq
 */
int zxdh_destroy_srq(struct ib_srq *ibsrq)
{
	struct zxdh_srq *iwsrq = to_iwsrq(ibsrq);
	struct zxdh_device *iwdev = iwsrq->iwdev;

	if (iwsrq->sc_srq.srq_uk.destroy_pending)
		goto free_rsrc;
	iwsrq->sc_srq.srq_uk.destroy_pending = true;

	zxdh_srq_rem_ref(&iwsrq->ibsrq);
	wait_for_completion(&iwsrq->free_srq);
	zxdh_srq_wq_destroy(iwdev->rf, &iwsrq->sc_srq);

free_rsrc:
	zxdh_free_srq_rsrc(iwsrq);
	kfree(iwsrq);
	return 0;
}
#endif

/**
 * zxdh_modify_srq - modify srq
 * @ibsrq: ptr of srq
 * @attr: access attributes
 * @attr_mask: state mask
 * @udata: user data
 */
int zxdh_modify_srq(struct ib_srq *ibsrq, struct ib_srq_attr *attr,
		    enum ib_srq_attr_mask attr_mask, struct ib_udata *udata)
{
	struct zxdh_srq *iwsrq = to_iwsrq(ibsrq);
	struct zxdh_device *iwdev = iwsrq->iwdev;
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	struct zxdh_pci_f *rf = iwdev->rf;

	/* We don't support resizing SRQs yet */
	if (attr_mask & IB_SRQ_MAX_WR)
		return -EINVAL;

	if (attr_mask & IB_SRQ_LIMIT) {
		if (attr->srq_limit >= iwsrq->max_wr)
			return -EINVAL;
	}
	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request)
		return -ENOMEM;

	iwsrq->srq_limit = attr->srq_limit;
	cqp_info = &cqp_request->info;
	cqp_info->cqp_cmd = ZXDH_OP_SRQ_MODIFY;
	cqp_info->post_sq = 1;
	cqp_info->in.u.srq_modify.srq = &iwsrq->sc_srq;
	cqp_info->in.u.srq_modify.info.limit = attr->srq_limit;
	cqp_info->in.u.srq_modify.scratch = (uintptr_t)cqp_request;

	zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);

	return 0;
}

/**
 * zxdh_query_srq - query srq
 * @ibsrq: ptr of srq
 * @srq_attr: srq attributes to return
 */
int zxdh_query_srq(struct ib_srq *ibsrq, struct ib_srq_attr *srq_attr)
{
	struct zxdh_srq *iwsrq = to_iwsrq(ibsrq);
	u32 limit;

	zxdh_query_srqc(&iwsrq->sc_srq, &limit);

	srq_attr->max_wr = (iwsrq->max_wr - 1);
	srq_attr->max_sge = iwsrq->max_sge;
	srq_attr->srq_limit = limit;

	return 0;
}

/**
 * zxdh_post_srq_recv - post srq recv
 * @ibsrq: ptr of srq
 * @ib_wr: work request for receive
 * @bad_wr: bad wr caused an error
 */
int zxdh_post_srq_recv(struct ib_srq *ibsrq, const struct ib_recv_wr *ib_wr,
		       const struct ib_recv_wr **bad_wr)
{
	struct zxdh_srq *iwsrq = to_iwsrq(ibsrq);
	struct zxdh_srq_uk *srq_uk;
	__le16 *wqe_16;
	__le64 *wqe_64;
	u64 temp_val;
	unsigned long flags;
	int err = 0;
	int nreq;
	int i;
	u16 *buf;
	u32 buf_size;
	u16 idx = 0;
	u64 hdr;
	__u32 byte_off;

	srq_uk = &iwsrq->sc_srq.srq_uk;
	spin_lock_irqsave(&iwsrq->lock, flags);
	buf_size = (iwsrq->max_wr * sizeof(u16));
	buf = kzalloc(buf_size, GFP_ATOMIC);
	if (!buf) {
		err = -ENOMEM;
		*bad_wr = ib_wr;
		goto out;
	}

	if (iwsrq->sc_srq.dev->hw_attrs.skip_hw == true) {
		err = -EINVAL;
		*bad_wr = ib_wr;
		goto out;
	}

	if (iwsrq->state == ZXDH_SRQ_STATE_ERROR) {
		err = -EIO;
		*bad_wr = ib_wr;
		goto out;
	}

	for (nreq = 0; ib_wr; nreq++, ib_wr = ib_wr->next) {
		if (unlikely(ib_wr->num_sge > iwsrq->max_sge)) {
			err = -EINVAL;
			*bad_wr = ib_wr;
			break;
		}

		if (unlikely(srq_uk->srq_ring.head == srq_uk->srq_ring.tail)) {
			err = -ENOMEM;
			*bad_wr = ib_wr;
			break;
		}

		if (unlikely(nreq >= iwsrq->max_wr)) {
			err = -EINVAL;
			*bad_wr = ib_wr;
			break;
		}

		srq_uk->srq_wrid_array[srq_uk->srq_ring.head] = ib_wr->wr_id;
		buf[nreq] = srq_uk->srq_ring.head;

		wqe_64 = zxdh_get_srq_wqe(iwsrq, srq_uk->srq_ring.head);
		get_64bit_val(wqe_64, 0, &temp_val);
		srq_uk->srq_ring.head =
			(__u16)FIELD_GET(IRDMAQPSRQ_NEXT_WQE_INDEX, temp_val);

		for (i = 0, byte_off = ZXDH_SRQ_FRAG_BYTESIZE;
		     i < ib_wr->num_sge; i++) {
			set_64bit_val(wqe_64, byte_off, ib_wr->sg_list[i].addr);
			set_64bit_val(
				wqe_64, byte_off + 8,
				FIELD_PREP(IRDMAQPSRQ_FRAG_LEN,
					   ib_wr->sg_list[i].length) |
					FIELD_PREP(IRDMAQPSRQ_FRAG_STAG,
						   ib_wr->sg_list[i].lkey));
			byte_off += ZXDH_SRQ_FRAG_BYTESIZE;
		}

		if ((ib_wr->num_sge < iwsrq->max_sge) ||
		    (ib_wr->num_sge == 0)) {
			set_64bit_val(wqe_64, byte_off, 0);
			set_64bit_val(
				wqe_64, byte_off + 8,
				FIELD_PREP(IRDMAQPSRQ_FRAG_LEN, 0) |
					FIELD_PREP(IRDMAQPSRQ_FRAG_STAG,
						   ZXDH_SRQ_INVALID_LKEY));
		}
		set_64bit_val(wqe_64, 8, ((u64)srq_uk->srq_id) << 32);

		hdr = FIELD_PREP(IRDMAQPSRQ_RSV, 0) |
		      FIELD_PREP(IRDMAQPSRQ_VALID_SGE_NUM, ib_wr->num_sge) |
		      FIELD_PREP(IRDMAQPSRQ_SIGNATURE, 0) |
		      FIELD_PREP(IRDMAQPSRQ_NEXT_WQE_INDEX,
				 srq_uk->srq_ring.head);

		dma_wmb(); /* make sure WQE is written before valid bit is set */

		set_64bit_val(wqe_64, 0, hdr);
	}

	if (err == 0) {
		for (i = 0; i < nreq; i++) {
			wqe_16 = zxdh_get_srq_list_wqe(iwsrq, &idx);
			set_16bit_val(wqe_16, 0, buf[i]);
		}

		hdr = FIELD_PREP(ZXDH_SRQ_PARITY_SIGN,
				 iwsrq->sc_srq.srq_uk.srq_list_polarity) |
		      FIELD_PREP(ZXDH_SRQ_SW_SRQ_HEAD, idx);
		dma_wmb();
		set_64bit_val(iwsrq->sc_srq.srq_uk.srq_db_base, 0, hdr);
	}
out:
	spin_unlock_irqrestore(&iwsrq->lock, flags);
	if (err)
		*bad_wr = ib_wr;
	if (buf)
		kfree(buf);
	return err;
}

/**
 * zxdh_sc_srq_create - create srq
 * @srq: sc srq
 * @info: srq create info
 * @scratch: u64 saved to be used during cqp completion
 * @post_sq: flag for cqp db to ring
 */
int zxdh_sc_srq_create(struct zxdh_sc_srq *srq,
		       struct zxdh_create_srq_info *info, u64 scratch,
		       bool post_sq)
{
	struct zxdh_sc_cqp *cqp;
	__le64 *wqe;
	u64 hdr;
	struct zxdh_sc_dev *dev;

	dev = srq->dev;
	cqp = srq->dev->cqp;
	if ((srq->srq_uk.srq_id < dev->base_srqn) ||
	    (srq->srq_uk.srq_id - dev->base_srqn) > (cqp->dev->hmc_info->hmc_obj[ZXDH_HMC_IW_SRQ].max_cnt - 1))
	{
		dev_err(idev_to_dev(dev), "srq_id=%d base_srqn=%d max_cnt=%d\n",
			srq->srq_uk.srq_id, dev->base_srqn, cqp->dev->hmc_info->hmc_obj[ZXDH_HMC_IW_SRQ].max_cnt);
		return -EINVAL;
	}
	wqe = zxdh_sc_cqp_get_next_send_wqe(cqp, scratch);
	if (!wqe)
		return -ENOSPC;

	hdr = FIELD_PREP(ZXDH_CQPSQ_SRQ_SWWQECNT, 0) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_LISTVIRTMAP, srq->list_virtual_map) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_LIST_LEAFPBLSIZE,
			 srq->list_pbl_chunk_size) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_LOGSRQSTRIDE,
			 srq->srq_uk.srq_wqe_size) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_SRQAXIERRSIG, 0) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_REVERSEDLKEY, 0) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_SRQVIRTMAP, srq->virtual_map) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_CONTSRQ, ZXDH_SRQ_WQE_NOT_CONT) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_WQSIG, 0) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_PD_INDEX, srq->pd->pd_id) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_LOGSRQSIZE, srq->srq_uk.log2_srq_size) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_LEAFPBLSIZE, srq->pbl_chunk_size) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_STATE, ZXDH_SRQ_STATE_GOOD);

	dma_wmb(); /* make sure WQE is written before valid bit is set */
	set_64bit_val(wqe, 16, hdr);

	set_64bit_val(wqe, 24,
		      srq->virtual_map ? srq->first_pm_pbl_idx : srq->srq_pa);
	set_64bit_val(wqe, 32,
		      srq->list_virtual_map ? srq->list_first_pm_pbl_idx :
						    srq->srq_list_pa);

	set_64bit_val(wqe, 40, srq->srq_db_pa);
	hdr = FIELD_PREP(ZXDH_CQPSQ_SRQ_LIMITWATERMARK, srq->srq_limit);
	dma_wmb(); /* make sure WQE is written before valid bit is set */
	set_64bit_val(wqe, 48, hdr);

	set_64bit_val(wqe, 56, srq->srq_compl_ctx);

	//bit0 bit1 bit2 set to 1
	set_64bit_val(wqe, 8,
		      (RDMA_SRQC_MASK_GENERAL_CFG |
		       RDMA_SRQC_MASK_LIMIT_WATER_CFG |
		       RDMA_SRQC_MASK_DEBUG_SET_CFG));

	hdr = FIELD_PREP(ZXDH_CQPSQ_SRQ_ID, srq->srq_uk.srq_id) |
	      FIELD_PREP(ZXDH_CQPSQ_WQEVALID, cqp->polarity) |
	      FIELD_PREP(ZXDH_CQPSQ_OPCODE, ZXDH_CQP_OP_CREATE_SRQ);

	dma_wmb(); /* make sure WQE is written before valid bit is set */

	set_64bit_val(wqe, 0, hdr);

	print_hex_dump_debug("WQE: SRQ_CREATE WQE", DUMP_PREFIX_OFFSET, 16, 8,
			     wqe, ZXDH_CQP_WQE_SIZE * 8, false);
	if (post_sq)
		zxdh_sc_cqp_post_sq(cqp);
	// print_hex_dump(KERN_DEBUG, "srq create ", DUMP_PREFIX_OFFSET, 16, 8, wqe, 9*8, false);
	return 0;
}

/**
 * zxdh_sc_srq_modify - modify srq cqp wqe
 * @srq: sc srq
 * @info: modify srq info
 * @scratch: u64 saved to be used during cqp completion
 * @post_sq: flag for cqp db to ring
 */
int zxdh_sc_srq_modify(struct zxdh_sc_srq *srq,
		       struct zxdh_modify_srq_info *info, u64 scratch,
		       bool post_sq)
{
	__le64 *wqe;
	struct zxdh_sc_cqp *cqp;
	u64 hdr;

	cqp = srq->dev->cqp;
	wqe = zxdh_sc_cqp_get_next_send_wqe(cqp, scratch);
	if (!wqe)
		return -ENOSPC;

	set_64bit_val(wqe, 8, RDMA_SRQC_MASK_LIMIT_WATER_CFG);
	hdr = FIELD_PREP(ZXDH_CQPSQ_SRQ_LIMITWATERMARK, info->limit);
	dma_wmb(); /* make sure WQE is written before valid bit is set */
	set_64bit_val(wqe, 48, hdr);
	hdr = FIELD_PREP(ZXDH_CQPSQ_SRQ_ID, srq->srq_uk.srq_id) |
	      FIELD_PREP(ZXDH_CQPSQ_WQEVALID, cqp->polarity) |
	      FIELD_PREP(ZXDH_CQPSQ_OPCODE, ZXDH_CQP_OP_MODIFY_SRQ);

	dma_wmb(); /* make sure WQE is written before valid bit is set */

	set_64bit_val(wqe, 0, hdr);

	print_hex_dump_debug("WQE: SRQ_MODIFY WQE", DUMP_PREFIX_OFFSET, 16, 8,
			     wqe, ZXDH_CQP_WQE_SIZE * 8, false);
	if (post_sq)
		zxdh_sc_cqp_post_sq(cqp);
	// print_hex_dump(KERN_DEBUG, "srq modify ", DUMP_PREFIX_OFFSET, 16, 8, wqe, 9*8, false);

	return 0;
}

/**
 * zxdh_sc_srq_destroy - cqp destroy srq
 * @srq: sc srq
 * @scratch: u64 saved to be used during cqp completion
 * @post_sq: flag for cqp db to ring
 */
int zxdh_sc_srq_destroy(struct zxdh_sc_srq *srq, u64 scratch, bool post_sq)
{
	__le64 *wqe;
	struct zxdh_sc_cqp *cqp;
	u64 hdr;

	cqp = srq->dev->cqp;
	wqe = zxdh_sc_cqp_get_next_send_wqe(cqp, scratch);
	if (!wqe)
		return -ENOSPC;

	hdr = FIELD_PREP(ZXDH_CQPSQ_SRQ_SWWQECNT, 0) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_LISTVIRTMAP, srq->list_virtual_map) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_LIST_LEAFPBLSIZE,
			 srq->list_pbl_chunk_size) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_LOGSRQSTRIDE,
			 ZXDH_SRQ_WQE_MIN_LEN_32_BYTE) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_SRQAXIERRSIG, 0) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_REVERSEDLKEY, 0) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_SRQVIRTMAP, srq->virtual_map) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_CONTSRQ, ZXDH_SRQ_WQE_NOT_CONT) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_WQSIG, 0) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_PD_INDEX, srq->pd->pd_id) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_LOGSRQSIZE, srq->srq_uk.log2_srq_size) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_LEAFPBLSIZE, srq->pbl_chunk_size) |
	      FIELD_PREP(ZXDH_CQPSQ_SRQ_STATE, ZXDH_SRQ_STATE_ERROR);

	dma_wmb(); /* make sure WQE is written before valid bit is set */
	set_64bit_val(wqe, 16, hdr);

	//bit0  set to 1
	set_64bit_val(wqe, 8, RDMA_SRQC_MASK_GENERAL_CFG);

	hdr = FIELD_PREP(ZXDH_CQPSQ_SRQ_ID, srq->srq_uk.srq_id) |
	      FIELD_PREP(ZXDH_CQPSQ_WQEVALID, cqp->polarity) |
	      FIELD_PREP(ZXDH_CQPSQ_OPCODE, ZXDH_CQP_OP_DESTROY_SRQ);

	dma_wmb(); /* make sure WQE is written before valid bit is set */

	set_64bit_val(wqe, 0, hdr);

	print_hex_dump_debug("WQE: SRQ_DESTROY WQE", DUMP_PREFIX_OFFSET, 16, 8,
			     wqe, ZXDH_CQP_WQE_SIZE * 8, false);
	if (post_sq)
		zxdh_sc_cqp_post_sq(cqp);

	return 0;
}

int zxdh_query_srqc(struct zxdh_sc_srq *srq, u32 *limit)
{
	struct zxdh_sc_dev *dev = srq->dev;
	struct zxdh_pci_f *rf = container_of(dev, struct zxdh_pci_f, sc_dev);
	struct zxdh_dma_mem srqc_buf;
	struct zxdh_cqp_request *cqp_request;
	struct cqp_cmds_info *cqp_info;
	int err_code = 0;
	int status;
	u64 temp;

	srqc_buf.va = NULL;
	srqc_buf.size = ALIGN(ZXDH_SRQ_CTX_SIZE, ZXDH_SRQC_ALIGNMENT);
	srqc_buf.va = dma_alloc_coherent(dev->hw->device, srqc_buf.size,
					 &srqc_buf.pa, GFP_KERNEL);
	if (!srqc_buf.va)
		return -ENOMEM;

	cqp_request = zxdh_alloc_and_get_cqp_request(&rf->cqp, true);
	if (!cqp_request) {
		err_code = -ENOMEM;
		goto free_rsrc;
	}
	cqp_info = &cqp_request->info;
	cqp_info->cqp_cmd = ZXDH_OP_QUERY_SRQC;
	cqp_info->post_sq = 1;
	cqp_info->in.u.query_srqc.dev = dev;
	cqp_info->in.u.query_srqc.srqn = srq->srq_uk.srq_id;
	cqp_info->in.u.query_srqc.srqc_buf_pa = srqc_buf.pa;
	cqp_info->in.u.query_srqc.scratch = (uintptr_t)cqp_request;
	status = zxdh_handle_cqp_op(rf, cqp_request);
	zxdh_put_cqp_request(&rf->cqp, cqp_request);
	if (status) {
		err_code = -ENOMEM;
		goto free_rsrc;
	}

	if (limit) {
		get_64bit_val(srqc_buf.va, 32, &temp);
		*limit = FIELD_GET(ZXDH_CQPSQ_SRQ_LIMITWATERMARK, temp);
	}
free_rsrc:
	dma_free_coherent(dev->hw->device, srqc_buf.size, srqc_buf.va,
			  srqc_buf.pa);
	srqc_buf.va = NULL;
	return err_code;
}

int zxdh_sc_query_srqc(struct zxdh_sc_dev *dev, u32 srqn, u64 srqc_buf_pa, u64 scratch,
		       bool post_sq)
{
	__le64 *wqe;
	struct zxdh_sc_cqp *cqp = dev->cqp;
	u64 hdr;

	wqe = zxdh_sc_cqp_get_next_send_wqe(cqp, scratch);
	if (!wqe)
		return -ENOSPC;

	hdr = FIELD_PREP(ZXDH_CQPSQ_OPCODE, ZXDH_CQP_OP_QUERY_SRQ) |
	      FIELD_PREP(ZXDH_CQPSQ_WQEVALID, cqp->polarity) |
	      FIELD_PREP(ZXDH_CQPSQ_QUERY_SRQC_ID, srqn);
	set_64bit_val(wqe, 8, srqc_buf_pa);

	dma_wmb();
	set_64bit_val(wqe, 0, hdr);

	if (post_sq)
		zxdh_sc_cqp_post_sq(cqp);

	return 0;
}

// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#include "osdep.h"
#include "status.h"
#include "defs.h"
#include "user.h"
#include "zrdma.h"
#include "type.h"
#include "srq.h"

/**
 * zxdh_set_fragment - set fragment in wqe
 * @wqe: wqe for setting fragment
 * @offset: offset value
 * @sge: sge length and stag
 * @valid: The wqe valid
 */
static void zxdh_set_fragment(__le64 *wqe, u32 offset, struct zxdh_sge *sge,
			      u8 valid)
{
	if (sge) {
		set_64bit_val(wqe, offset + 8,
			      FIELD_PREP(IRDMAQPSQ_FRAG_TO, sge->tag_off));
		set_64bit_val(wqe, offset,
			      FIELD_PREP(IRDMAQPSQ_VALID, valid) |
				      FIELD_PREP(IRDMAQPSQ_FRAG_LEN, sge->len) |
				      FIELD_PREP(IRDMAQPSQ_FRAG_STAG,
						 sge->stag));
	} else {
		set_64bit_val(wqe, offset + 8, 0);
		set_64bit_val(wqe, offset, FIELD_PREP(IRDMAQPSQ_VALID, valid));
	}
}

/**
 * zxdh_nop_1 - insert a NOP wqe
 * @qp: hw qp ptr
 */
static int zxdh_nop_1(struct zxdh_qp_uk *qp)
{
	u64 hdr;
	__le64 *wqe;
	u32 wqe_idx;
	bool signaled = false;

	if (!qp->sq_ring.head)
		return -EINVAL;

	wqe_idx = ZXDH_RING_CURRENT_HEAD(qp->sq_ring);
	wqe = qp->sq_base[wqe_idx].elem;

	qp->sq_wrtrk_array[wqe_idx].quanta = ZXDH_QP_WQE_MIN_QUANTA;

	set_64bit_val(wqe, 8, 0);
	set_64bit_val(wqe, 16, 0);
	set_64bit_val(wqe, 24, 0);

	hdr = FIELD_PREP(IRDMAQPSQ_OPCODE, ZXDH_OP_TYPE_NOP) |
	      FIELD_PREP(IRDMAQPSQ_SIGCOMPL, signaled) |
	      FIELD_PREP(IRDMAQPSQ_VALID, qp->swqe_polarity);

	/* make sure WQE is written before valid bit is set */
	dma_wmb();

	set_64bit_val(wqe, 0, hdr);

	return 0;
}

/**
 * zxdh_clr_wqes - clear next 128 sq entries
 * @qp: hw qp ptr
 * @qp_wqe_idx: wqe_idx
 */
void zxdh_clr_wqes(struct zxdh_qp_uk *qp, u32 qp_wqe_idx)
{
	__le64 *wqe;
	u32 wqe_idx;

	if (!(qp_wqe_idx & 0x7F)) {
		wqe_idx = (qp_wqe_idx + 128) % qp->sq_ring.size;
		wqe = qp->sq_base[wqe_idx].elem;
		if (wqe_idx)
			__memset(wqe, qp->swqe_polarity ? 0 : 0xFF, 0x1000);
		else
			__memset(wqe, qp->swqe_polarity ? 0xFF : 0, 0x1000);
	}
}

/**
 * zxdh_uk_qp_post_wr - ring doorbell
 * @qp: hw qp ptr
 */
void zxdh_uk_qp_post_wr(struct zxdh_qp_uk *qp)
{
	dma_wmb();

	writel(qp->qp_id, qp->wqe_alloc_db);
	qp->initial_ring.head = qp->sq_ring.head;
}

/**
 * zxdh_uk_qp_set_shadow_area - fill SW_RQ_Head
 * @qp: hw qp ptr
 */
void zxdh_uk_qp_set_shadow_area(struct zxdh_qp_uk *qp)
{
	set_64bit_val(qp->shadow_area, 0,
		      FIELD_PREP(IRDMAQPDBSA_RQ_POLARITY, qp->rwqe_polarity) |
			      FIELD_PREP(IRDMAQPDBSA_RQ_SW_HEAD,
					 ZXDH_RING_CURRENT_HEAD(qp->rq_ring)));
}

#ifdef Z_CONFIG_RDMA_PUSH_MODE
/**
 * zxdh_qp_ring_push_db -  ring qp doorbell
 * @qp: hw qp ptr
 * @wqe_idx: wqe index
 */
static void zxdh_qp_ring_push_db(struct zxdh_qp_uk *qp, u32 wqe_idx)
{
	set_32bit_val(qp->push_db, 0,
		      FIELD_PREP(ZXDH_WQEALLOC_WQE_DESC_INDEX, wqe_idx >> 3) |
			      qp->qp_id);
	qp->initial_ring.head = qp->sq_ring.head;
	qp->push_mode = true;
	qp->push_dropped = false;
}

void zxdh_qp_push_wqe(struct zxdh_qp_uk *qp, __le64 *wqe, u16 quanta,
		      u32 wqe_idx, bool post_sq)
{
	__le64 *push;

	if (ZXDH_RING_CURRENT_HEAD(qp->initial_ring) !=
		    ZXDH_RING_CURRENT_TAIL(qp->sq_ring) &&
	    !qp->push_mode) {
		if (post_sq)
			zxdh_uk_qp_post_wr(qp);
	} else {
		push = (__le64 *)((uintptr_t)qp->push_wqe +
				  (wqe_idx & 0x7) * 0x20);
		memcpy(push, wqe, quanta * ZXDH_QP_WQE_MIN_SIZE);
		zxdh_qp_ring_push_db(qp, wqe_idx);
	}
}
#endif
/**
 * zxdh_qp_get_next_send_wqe - pad with NOP if needed, return where next WR should go
 * @qp: hw qp ptr
 * @wqe_idx: return wqe index
 * @quanta: size of WR in quanta
 * @total_size: size of WR in bytes
 * @info: info on WR
 */
__le64 *zxdh_qp_get_next_send_wqe(struct zxdh_qp_uk *qp, u32 *wqe_idx,
				  u16 quanta, u32 total_size,
				  struct zxdh_post_sq_info *info)
{
	__le64 *wqe;
	u16 avail_quanta;
	u16 i;

	avail_quanta = ZXDH_MAX_SQ_WQES_PER_PAGE -
		       (ZXDH_RING_CURRENT_HEAD(qp->sq_ring) %
			ZXDH_MAX_SQ_WQES_PER_PAGE);

	if (quanta <= avail_quanta) {
		/* WR fits in current chunk */
		if (quanta > ZXDH_SQ_RING_FREE_QUANTA(qp->sq_ring))
			return NULL;
	} else {
		/* Need to pad with NOP */
		if (quanta + avail_quanta >
		    ZXDH_SQ_RING_FREE_QUANTA(qp->sq_ring))
			return NULL;

		for (i = 0; i < avail_quanta; i++) {
			zxdh_nop_1(qp);
			ZXDH_RING_MOVE_HEAD_NOCHECK(qp->sq_ring);
		}
	}

	*wqe_idx = ZXDH_RING_CURRENT_HEAD(qp->sq_ring);
	if (!*wqe_idx)
		qp->swqe_polarity = !qp->swqe_polarity;

	ZXDH_RING_MOVE_HEAD_BY_COUNT_NOCHECK(qp->sq_ring, quanta);

	wqe = qp->sq_base[*wqe_idx].elem;

	qp->sq_wrtrk_array[*wqe_idx].wrid = info->wr_id;
	qp->sq_wrtrk_array[*wqe_idx].wr_len = total_size;
	qp->sq_wrtrk_array[*wqe_idx].quanta = quanta;

	return wqe;
}

/**
 * zxdh_qp_get_next_recv_wqe - get next qp's rcv wqe
 * @qp: hw qp ptr
 * @wqe_idx: return wqe index
 */
__le64 *zxdh_qp_get_next_recv_wqe(struct zxdh_qp_uk *qp, u32 *wqe_idx)
{
	__le64 *wqe;
	int ret_code;

	if (ZXDH_RING_FULL_ERR(qp->rq_ring))
		return NULL;

	ZXDH_ATOMIC_RING_MOVE_HEAD(qp->rq_ring, *wqe_idx, ret_code);
	if (ret_code)
		return NULL;

	if (!*wqe_idx)
		qp->rwqe_polarity = !qp->rwqe_polarity;
	/* rq_wqe_size_multiplier is no of 16 byte quanta in one rq wqe */
	wqe = qp->rq_base[*wqe_idx * qp->rq_wqe_size_multiplier].elem;

	return wqe;
}

/**
 * zxdh_uk_rdma_write - rdma write operation
 * @qp: hw qp ptr
 * @info: post sq information
 * @post_sq: flag to post sq
 */
int zxdh_uk_rdma_write(struct zxdh_qp_uk *qp, struct zxdh_post_sq_info *info,
		       bool post_sq)
{
	u64 hdr;
	__le64 *wqe;
	struct zxdh_rdma_write *op_info;
	u32 i, wqe_idx;
	u32 total_size = 0, byte_off;
	int ret_code;
	u32 frag_cnt, addl_frag_cnt;
	bool read_fence = false;
	u16 quanta;
	bool imm_data_flag = info->imm_data_valid ? 1 : 0;

	op_info = &info->op.rdma_write;
	if (op_info->num_lo_sges > qp->max_sq_frag_cnt)
		return -EINVAL;

	for (i = 0; i < op_info->num_lo_sges; i++) {
		total_size += op_info->lo_sg_list[i].len;
		if (0 != i && 0 == op_info->lo_sg_list[i].len)
			return -EINVAL;
	}

	if (total_size > ZXDH_MAX_SQ_PAYLOAD_SIZE)
		return -EINVAL;

	read_fence |= info->read_fence;

	if (imm_data_flag)
		frag_cnt = op_info->num_lo_sges ? (op_info->num_lo_sges + 1) :
							2;
	else
		frag_cnt = op_info->num_lo_sges;
	addl_frag_cnt = op_info->num_lo_sges > 1 ? (op_info->num_lo_sges - 1) :
							 0;
	ret_code = zxdh_fragcnt_to_quanta_sq(frag_cnt, &quanta);
	if (ret_code)
		return ret_code;

	wqe = zxdh_qp_get_next_send_wqe(qp, &wqe_idx, quanta, total_size, info);
	if (!wqe)
		return -ENOSPC;

	zxdh_clr_wqes(qp, wqe_idx);

	if (op_info->num_lo_sges) {
		set_64bit_val(
			wqe, 16,
			FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_VALID,
				   op_info->lo_sg_list->len ==
						   ZXDH_MAX_SQ_PAYLOAD_SIZE ?
					   1 :
						 0) |
				FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_LEN,
					   op_info->lo_sg_list->len) |
				FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_STAG,
					   op_info->lo_sg_list->stag));
		set_64bit_val(wqe, 8,
			      FIELD_PREP(IRDMAQPSQ_FRAG_TO,
					 op_info->lo_sg_list->tag_off));
	} else {
		/*if zero sge,post a special sge with zero lenth*/
		set_64bit_val(wqe, 16,
			      FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_VALID, 0) |
				      FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_LEN, 0) |
				      FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_STAG,
						 0x100));
		set_64bit_val(wqe, 8, FIELD_PREP(IRDMAQPSQ_FRAG_TO, 0));
	}

	if (imm_data_flag) {
		byte_off = ZXDH_SQ_WQE_BYTESIZE + ZXDH_QP_FRAG_BYTESIZE;
		if (op_info->num_lo_sges > 1) {
			qp->wqe_ops.iw_set_fragment(wqe, byte_off,
						    &op_info->lo_sg_list[1],
						    qp->swqe_polarity);
			byte_off += ZXDH_QP_FRAG_BYTESIZE;
		}
		set_64bit_val(
			wqe, ZXDH_SQ_WQE_BYTESIZE,
			FIELD_PREP(IRDMAQPSQ_IMMDATA_VALID, qp->swqe_polarity) |
				FIELD_PREP(IRDMAQPSQ_IMMDATA, info->imm_data));
		i = 2;
		if (i < op_info->num_lo_sges) {
			for (byte_off = ZXDH_SQ_WQE_BYTESIZE + 2 * ZXDH_QP_FRAG_BYTESIZE;
			    i < op_info->num_lo_sges; i += 2) {
				if (i == addl_frag_cnt) {
					qp->wqe_ops.iw_set_fragment(
						wqe, byte_off,
						&op_info->lo_sg_list[i],
						qp->swqe_polarity);
					byte_off += ZXDH_QP_FRAG_BYTESIZE;
					break;
				}
				byte_off += ZXDH_QP_FRAG_BYTESIZE;
				qp->wqe_ops.iw_set_fragment(
					wqe, byte_off,
					&op_info->lo_sg_list[i + 1],
					qp->swqe_polarity);
				byte_off -= ZXDH_QP_FRAG_BYTESIZE;
				qp->wqe_ops.iw_set_fragment(
					wqe, byte_off, &op_info->lo_sg_list[i],
					qp->swqe_polarity);
				byte_off += 2 * ZXDH_QP_FRAG_BYTESIZE;
			}
		}
	} else {
		i = 1;
		for (byte_off = ZXDH_SQ_WQE_BYTESIZE; i < op_info->num_lo_sges; i += 2) {
			if (i == addl_frag_cnt) {
				qp->wqe_ops.iw_set_fragment(
					wqe, byte_off, &op_info->lo_sg_list[i],
					qp->swqe_polarity);
				byte_off += ZXDH_QP_FRAG_BYTESIZE;
				break;
			}
			byte_off += ZXDH_QP_FRAG_BYTESIZE;
			qp->wqe_ops.iw_set_fragment(wqe, byte_off,
						    &op_info->lo_sg_list[i + 1],
						    qp->swqe_polarity);
			byte_off -= ZXDH_QP_FRAG_BYTESIZE;
			qp->wqe_ops.iw_set_fragment(wqe, byte_off,
						    &op_info->lo_sg_list[i],
						    qp->swqe_polarity);
			byte_off += 2 * ZXDH_QP_FRAG_BYTESIZE;
		}
	}

	/* if not an odd number set valid bit in next fragment */
	if (!(frag_cnt & 0x01) && frag_cnt) {
		qp->wqe_ops.iw_set_fragment(wqe, byte_off, NULL,
					    qp->swqe_polarity);
	}

	hdr = FIELD_PREP(IRDMAQPSQ_VALID, qp->swqe_polarity) |
	      FIELD_PREP(IRDMAQPSQ_OPCODE, info->op_type) |
	      FIELD_PREP(IRDMAQPSQ_SIGCOMPL, info->signaled) |
	      FIELD_PREP(IRDMAQPSQ_LOCALFENCE, info->local_fence) |
	      FIELD_PREP(IRDMAQPSQ_READFENCE, read_fence) |
	      FIELD_PREP(IRDMAQPSQ_SOLICITED, info->solicited) |
	      FIELD_PREP(IRDMAQPSQ_IMMDATAFLAG, imm_data_flag) |
	      FIELD_PREP(IRDMAQPSQ_ADDFRAGCNT, addl_frag_cnt) |
	      FIELD_PREP(IRDMAQPSQ_REMSTAG, op_info->rem_addr.stag);
	set_64bit_val(wqe, 24,
		      FIELD_PREP(IRDMAQPSQ_FRAG_TO, op_info->rem_addr.tag_off));

	dma_wmb(); /* make sure WQE is populated before valid bit is set */

	set_64bit_val(wqe, 0, hdr);
	if (post_sq)
		zxdh_uk_qp_post_wr(qp);

	return 0;
}

/**
 * zxdh_uk_rdma_read - rdma read command
 * @qp: hw qp ptr
 * @info: post sq information
 * @post_sq: flag to post sq
 */
int zxdh_uk_rdma_read(struct zxdh_qp_uk *qp, struct zxdh_post_sq_info *info,
		      bool post_sq)
{
	struct zxdh_rdma_read *op_info;
	int ret_code;
	u32 i, byte_off, total_size = 0;
	bool local_fence = false;
	bool ord_fence = false;
	u32 addl_frag_cnt;
	__le64 *wqe;
	u32 wqe_idx;
	u16 quanta;
	u64 hdr;

	op_info = &info->op.rdma_read;
	if (qp->max_sq_frag_cnt < op_info->num_lo_sges)
		return -EINVAL;

	for (i = 0; i < op_info->num_lo_sges; i++) {
		total_size += op_info->lo_sg_list[i].len;
		if (0 != i && 0 == op_info->lo_sg_list[i].len)
			return -EINVAL;
	}

	if (total_size > ZXDH_MAX_SQ_PAYLOAD_SIZE)
		return -EINVAL;

	ret_code = zxdh_fragcnt_to_quanta_sq(op_info->num_lo_sges, &quanta);
	if (ret_code)
		return ret_code;

	wqe = zxdh_qp_get_next_send_wqe(qp, &wqe_idx, quanta, total_size, info);
	if (!wqe)
		return -ENOSPC;

	if (qp->rd_fence_rate && (qp->ord_cnt++ == qp->rd_fence_rate)) {
		ord_fence = true;
		qp->ord_cnt = 0;
	}

	zxdh_clr_wqes(qp, wqe_idx);

	addl_frag_cnt = op_info->num_lo_sges > 1 ? (op_info->num_lo_sges - 1) :
							 0;
	local_fence |= info->local_fence;

	if (op_info->num_lo_sges) {
		set_64bit_val(
			wqe, 16,
			FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_VALID,
				   op_info->lo_sg_list->len ==
						   ZXDH_MAX_SQ_PAYLOAD_SIZE ?
					   1 :
						 0) |
				FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_LEN,
					   op_info->lo_sg_list->len) |
				FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_STAG,
					   op_info->lo_sg_list->stag));
		set_64bit_val(wqe, 8,
			      FIELD_PREP(IRDMAQPSQ_FRAG_TO,
					 op_info->lo_sg_list->tag_off));
	} else {
		/*if zero sge,post a special sge with zero lenth*/
		set_64bit_val(wqe, 16,
			      FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_VALID, 0) |
				      FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_LEN, 0) |
				      FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_STAG,
						 0x100));
		set_64bit_val(wqe, 8, FIELD_PREP(IRDMAQPSQ_FRAG_TO, 0));
	}

	i = 1;
	for (byte_off = ZXDH_SQ_WQE_BYTESIZE; i < op_info->num_lo_sges; i += 2) {
		if (i == addl_frag_cnt) {
			qp->wqe_ops.iw_set_fragment(wqe, byte_off,
						    &op_info->lo_sg_list[i],
						    qp->swqe_polarity);
			byte_off += ZXDH_QP_FRAG_BYTESIZE;
			break;
		}
		byte_off += ZXDH_QP_FRAG_BYTESIZE;
		qp->wqe_ops.iw_set_fragment(wqe, byte_off,
					    &op_info->lo_sg_list[i + 1],
					    qp->swqe_polarity);
		byte_off -= ZXDH_QP_FRAG_BYTESIZE;
		qp->wqe_ops.iw_set_fragment(wqe, byte_off,
					    &op_info->lo_sg_list[i],
					    qp->swqe_polarity);
		byte_off += 2 * ZXDH_QP_FRAG_BYTESIZE;
	}

	/* if not an odd number set valid bit in next fragment */
	if (!(op_info->num_lo_sges & 0x01) && op_info->num_lo_sges) {
		qp->wqe_ops.iw_set_fragment(wqe, byte_off, NULL,
					    qp->swqe_polarity);
	}

	hdr = FIELD_PREP(IRDMAQPSQ_VALID, qp->swqe_polarity) |
	      FIELD_PREP(IRDMAQPSQ_OPCODE, ZXDH_OP_TYPE_READ) |
	      FIELD_PREP(IRDMAQPSQ_SIGCOMPL, info->signaled) |
	      FIELD_PREP(IRDMAQPSQ_LOCALFENCE, info->local_fence) |
	      FIELD_PREP(IRDMAQPSQ_READFENCE,
			 info->read_fence || ord_fence ? 1 : 0) |
	      FIELD_PREP(IRDMAQPSQ_SOLICITED, info->solicited) |
	      FIELD_PREP(IRDMAQPSQ_ADDFRAGCNT, addl_frag_cnt) |
	      FIELD_PREP(IRDMAQPSQ_REMSTAG, op_info->rem_addr.stag);
	set_64bit_val(wqe, 24,
		      FIELD_PREP(IRDMAQPSQ_FRAG_TO, op_info->rem_addr.tag_off));

	dma_wmb(); /* make sure WQE is populated before valid bit is set */

	set_64bit_val(wqe, 0, hdr);

	if (post_sq)
		zxdh_uk_qp_post_wr(qp);

	return 0;
}

/**
 * zxdh_uk_rc_send - rdma send command
 * @qp: hw qp ptr
 * @info: post sq information
 * @post_sq: flag to post sq
 */
int zxdh_uk_rc_send(struct zxdh_qp_uk *qp, struct zxdh_post_sq_info *info,
		    bool post_sq)
{
	__le64 *wqe;
	struct zxdh_post_send *op_info;
	u64 hdr;
	u32 i, wqe_idx, total_size = 0, byte_off = ZXDH_SQ_WQE_BYTESIZE;
	int ret_code;
	u32 frag_cnt, addl_frag_cnt;
	bool read_fence = false;
	u16 quanta;
	bool imm_data_flag = info->imm_data_valid ? 1 : 0;

	op_info = &info->op.send;
	if (qp->max_sq_frag_cnt < op_info->num_sges)
		return -EINVAL;

	for (i = 0; i < op_info->num_sges; i++) {
		total_size += op_info->sg_list[i].len;
		if (0 != i && 0 == op_info->sg_list[i].len)
			return -EINVAL;
	}

	if (total_size > ZXDH_MAX_SQ_PAYLOAD_SIZE)
		return -EINVAL;

	if (imm_data_flag)
		frag_cnt = op_info->num_sges ? (op_info->num_sges + 1) : 2;
	else
		frag_cnt = op_info->num_sges;
	ret_code = zxdh_fragcnt_to_quanta_sq(frag_cnt, &quanta);
	if (ret_code)
		return ret_code;

	wqe = zxdh_qp_get_next_send_wqe(qp, &wqe_idx, quanta, total_size, info);
	if (!wqe)
		return -ENOSPC;

	zxdh_clr_wqes(qp, wqe_idx);

	read_fence |= info->read_fence;
	addl_frag_cnt = op_info->num_sges > 1 ? (op_info->num_sges - 1) : 0;
	if (op_info->num_sges) {
		set_64bit_val(
			wqe, 16,
			FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_VALID,
				   op_info->sg_list->len ==
						   ZXDH_MAX_SQ_PAYLOAD_SIZE ?
					   1 :
						 0) |
				FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_LEN,
					   op_info->sg_list->len) |
				FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_STAG,
					   op_info->sg_list->stag));
		set_64bit_val(wqe, 8,
			      FIELD_PREP(IRDMAQPSQ_FRAG_TO,
					 op_info->sg_list->tag_off));
	} else {
		/*if zero sge,post a special sge with zero lenth*/
		set_64bit_val(wqe, 16,
			      FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_VALID, 0) |
				      FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_LEN, 0) |
				      FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_STAG,
						 0x100));
		set_64bit_val(wqe, 8, FIELD_PREP(IRDMAQPSQ_FRAG_TO, 0));
	}

	if (imm_data_flag) {
		set_64bit_val(
			wqe, ZXDH_SQ_WQE_BYTESIZE,
			FIELD_PREP(IRDMAQPSQ_IMMDATA_VALID, qp->swqe_polarity) |
				FIELD_PREP(IRDMAQPSQ_IMMDATA, info->imm_data));

        i = 2;
		if (i < op_info->num_sges) {
			for (byte_off = ZXDH_SQ_WQE_BYTESIZE + 2 * ZXDH_QP_FRAG_BYTESIZE;
			     i < op_info->num_sges; i += 2) {
				if (i == addl_frag_cnt) {
					qp->wqe_ops.iw_set_fragment(
						wqe, byte_off,
						&op_info->sg_list[i],
						qp->swqe_polarity);
					byte_off += ZXDH_QP_FRAG_BYTESIZE;
					break;
				}
				byte_off += ZXDH_QP_FRAG_BYTESIZE;
				qp->wqe_ops.iw_set_fragment(
					wqe, byte_off, &op_info->sg_list[i + 1],
					qp->swqe_polarity);
				byte_off -= ZXDH_QP_FRAG_BYTESIZE;
				qp->wqe_ops.iw_set_fragment(
					wqe, byte_off, &op_info->sg_list[i],
					qp->swqe_polarity);
				byte_off += 2 * ZXDH_QP_FRAG_BYTESIZE;
			}
		}
	} else {
        i = 1;
		for (byte_off = ZXDH_SQ_WQE_BYTESIZE; i < op_info->num_sges;
		     i += 2) {
			if (i == addl_frag_cnt) {
				qp->wqe_ops.iw_set_fragment(
					wqe, byte_off, &op_info->sg_list[i],
					qp->swqe_polarity);
				byte_off += ZXDH_QP_FRAG_BYTESIZE;
				break;
			}
			byte_off += ZXDH_QP_FRAG_BYTESIZE;
			qp->wqe_ops.iw_set_fragment(wqe, byte_off,
						    &op_info->sg_list[i + 1],
						    qp->swqe_polarity);
			byte_off -= ZXDH_QP_FRAG_BYTESIZE;
			qp->wqe_ops.iw_set_fragment(wqe, byte_off,
						    &op_info->sg_list[i],
						    qp->swqe_polarity);
			byte_off += 2 * ZXDH_QP_FRAG_BYTESIZE;
		}
	}

	/* if not an odd number set valid bit in next fragment */
	if (!(frag_cnt & 0x01) && frag_cnt) {
		qp->wqe_ops.iw_set_fragment(wqe, byte_off, NULL,
					    qp->swqe_polarity);
	}

	hdr = FIELD_PREP(IRDMAQPSQ_VALID, qp->swqe_polarity) |
	      FIELD_PREP(IRDMAQPSQ_OPCODE, info->op_type) |
	      FIELD_PREP(IRDMAQPSQ_SIGCOMPL, info->signaled) |
	      FIELD_PREP(IRDMAQPSQ_LOCALFENCE, info->local_fence) |
	      FIELD_PREP(IRDMAQPSQ_READFENCE, read_fence) |
	      FIELD_PREP(IRDMAQPSQ_SOLICITED, info->solicited) |
	      FIELD_PREP(IRDMAQPSQ_IMMDATAFLAG, imm_data_flag) |
	      FIELD_PREP(IRDMAQPSQ_ADDFRAGCNT, addl_frag_cnt) |
	      FIELD_PREP(IRDMAQPSQ_REMSTAG, info->stag_to_inv);
	set_64bit_val(wqe, 24,
		      FIELD_PREP(IRDMAQPSQ_INLINEDATAFLAG, 0) |
			      FIELD_PREP(IRDMAQPSQ_INLINEDATALEN, 0));

	dma_wmb(); /* make sure WQE is populated before valid bit is set */

	set_64bit_val(wqe, 0, hdr);
	if (post_sq)
		zxdh_uk_qp_post_wr(qp);

	return 0;
}

/**
 * zxdh_uk_ud_send - rdma send command
 * @qp: hw qp ptr
 * @info: post sq information
 * @post_sq: flag to post sq
 */
int zxdh_uk_ud_send(struct zxdh_qp_uk *qp, struct zxdh_post_sq_info *info,
		    bool post_sq)
{
	__le64 *wqe_base;
	__le64 *wqe_ex = NULL;
	struct zxdh_post_send *op_info;
	u64 hdr;
	u32 i, wqe_idx, total_size = 0, byte_off;
	int ret_code;
	u32 frag_cnt, addl_frag_cnt;
	bool read_fence = false;
	u16 quanta;
	bool imm_data_flag = info->imm_data_valid ? 1 : 0;

	op_info = &info->op.send;
	if (qp->max_sq_frag_cnt < op_info->num_sges)
		return -EINVAL;

	for (i = 0; i < op_info->num_sges; i++) {
		total_size += op_info->sg_list[i].len;
		if (0 != i && 0 == op_info->sg_list[i].len)
			return -EINVAL;
	}

	if (total_size > ZXDH_MAX_SQ_PAYLOAD_SIZE)
		return -EINVAL;

	if (imm_data_flag)
		frag_cnt = op_info->num_sges ? (op_info->num_sges + 1) : 2;
	else
		frag_cnt = op_info->num_sges;
	ret_code = zxdh_fragcnt_to_quanta_sq(frag_cnt, &quanta);
	if (ret_code)
		return ret_code;

	if (quanta > ZXDH_SQ_RING_FREE_QUANTA(qp->sq_ring))
		return -ENOSPC;

	wqe_idx = ZXDH_RING_CURRENT_HEAD(qp->sq_ring);
	if (!wqe_idx)
		qp->swqe_polarity = !qp->swqe_polarity;

	ZXDH_RING_MOVE_HEAD_BY_COUNT_NOCHECK(qp->sq_ring, quanta);

	wqe_base = qp->sq_base[wqe_idx].elem;
	qp->sq_wrtrk_array[wqe_idx].wrid = info->wr_id;
	qp->sq_wrtrk_array[wqe_idx].wr_len = total_size;
	qp->sq_wrtrk_array[wqe_idx].quanta = quanta;

	zxdh_clr_wqes(qp, wqe_idx);

	read_fence |= info->read_fence;
	addl_frag_cnt = op_info->num_sges > 1 ? (op_info->num_sges - 1) : 0;
	hdr = FIELD_PREP(IRDMAQPSQ_VALID, qp->swqe_polarity) |
	      FIELD_PREP(IRDMAQPSQ_OPCODE, info->op_type) |
	      FIELD_PREP(IRDMAQPSQ_SIGCOMPL, info->signaled) |
	      FIELD_PREP(IRDMAQPSQ_SOLICITED, info->solicited) |
	      FIELD_PREP(IRDMAQPSQ_IMMDATAFLAG, imm_data_flag) |
	      FIELD_PREP(IRDMAQPSQ_UD_INLINEDATAFLAG, 0) |
	      FIELD_PREP(IRDMAQPSQ_UD_INLINEDATALEN, 0) |
	      FIELD_PREP(IRDMAQPSQ_UD_ADDFRAGCNT, addl_frag_cnt) |
	      FIELD_PREP(IRDMAQPSQ_AHID, op_info->ah_id);

	if (op_info->num_sges) {
		set_64bit_val(
			wqe_base, 16,
			FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_VALID,
				   op_info->sg_list->len ==
						   ZXDH_MAX_SQ_PAYLOAD_SIZE ?
					   1 :
						 0) |
				FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_LEN,
					   op_info->sg_list->len) |
				FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_STAG,
					   op_info->sg_list->stag));
		set_64bit_val(wqe_base, 8,
			      FIELD_PREP(IRDMAQPSQ_FRAG_TO,
					 op_info->sg_list->tag_off));
	} else {
		/*if zero sge,post a special sge with zero lenth*/
		set_64bit_val(wqe_base, 16,
			      FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_VALID, 0) |
				      FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_LEN, 0) |
				      FIELD_PREP(IRDMAQPSQ_FIRST_FRAG_STAG,
						 0x100));
		set_64bit_val(wqe_base, 8, FIELD_PREP(IRDMAQPSQ_FRAG_TO, 0));
	}

	if (imm_data_flag) {
		wqe_idx = (wqe_idx + 1) % qp->sq_ring.size;
		if (!wqe_idx)
			qp->swqe_polarity = !qp->swqe_polarity;
		wqe_ex = qp->sq_base[wqe_idx].elem;
		if (op_info->num_sges > 1) {
			qp->wqe_ops.iw_set_fragment(wqe_ex,
						    ZXDH_QP_FRAG_BYTESIZE,
						    &op_info->sg_list[1],
						    qp->swqe_polarity);
		}
		set_64bit_val(
			wqe_ex, 0,
			FIELD_PREP(IRDMAQPSQ_IMMDATA_VALID, qp->swqe_polarity) |
				FIELD_PREP(IRDMAQPSQ_IMMDATA, info->imm_data));
		i = 2;
		for (byte_off = ZXDH_QP_FRAG_BYTESIZE; i < op_info->num_sges;
		     i += 2) {
			if (!(i & 0x1)) {
				wqe_idx = (wqe_idx + 1) % qp->sq_ring.size;
				if (!wqe_idx)
					qp->swqe_polarity = !qp->swqe_polarity;
				wqe_ex = qp->sq_base[wqe_idx].elem;
			}
			if (i == addl_frag_cnt) {
				qp->wqe_ops.iw_set_fragment(
					wqe_ex, 0, &op_info->sg_list[i],
					qp->swqe_polarity);
				break;
			}
			qp->wqe_ops.iw_set_fragment(
				wqe_ex, byte_off % ZXDH_SQ_WQE_BYTESIZE,
				&op_info->sg_list[i + 1], qp->swqe_polarity);
			byte_off -= ZXDH_QP_FRAG_BYTESIZE;
			qp->wqe_ops.iw_set_fragment(
				wqe_ex, byte_off % ZXDH_SQ_WQE_BYTESIZE,
				&op_info->sg_list[i], qp->swqe_polarity);
			byte_off += ZXDH_QP_FRAG_BYTESIZE;
		}
	} else {
		i = 1;
		for (byte_off = 0; i < op_info->num_sges; i += 2) {
			if (i & 0x1) {
				wqe_idx = (wqe_idx + 1) % qp->sq_ring.size;
				if (!wqe_idx)
					qp->swqe_polarity = !qp->swqe_polarity;
				wqe_ex = qp->sq_base[wqe_idx].elem;
			}
			if (i == addl_frag_cnt) {
				qp->wqe_ops.iw_set_fragment(
					wqe_ex, 0, &op_info->sg_list[i],
					qp->swqe_polarity);
				break;
			}
			byte_off += ZXDH_QP_FRAG_BYTESIZE;
			qp->wqe_ops.iw_set_fragment(
				wqe_ex, byte_off % ZXDH_SQ_WQE_BYTESIZE,
				&op_info->sg_list[i + 1], qp->swqe_polarity);
			byte_off -= ZXDH_QP_FRAG_BYTESIZE;
			qp->wqe_ops.iw_set_fragment(
				wqe_ex, byte_off % ZXDH_SQ_WQE_BYTESIZE,
				&op_info->sg_list[i], qp->swqe_polarity);
		}
	}

	/* if not an odd number set valid bit in next fragment */
	if (!(frag_cnt & 0x01) && frag_cnt && wqe_ex) {
		qp->wqe_ops.iw_set_fragment(wqe_ex, ZXDH_QP_FRAG_BYTESIZE, NULL,
					    qp->swqe_polarity);
	}

	set_64bit_val(wqe_base, 24,
		      FIELD_PREP(IRDMAQPSQ_DESTQPN, op_info->dest_qp) |
			      FIELD_PREP(IRDMAQPSQ_DESTQKEY, op_info->qkey));

	dma_wmb(); /* make sure WQE is populated before valid bit is set */

	set_64bit_val(wqe_base, 0, hdr);
	if (post_sq)
		zxdh_uk_qp_post_wr(qp);

	return 0;
}

/**
 * zxdh_set_mw_bind_wqe - set mw bind in wqe
 * @wqe: wqe for setting mw bind
 * @op_info: info for setting wqe values
 */
static void zxdh_set_mw_bind_wqe(__le64 *wqe, struct zxdh_bind_window *op_info)
{
	set_64bit_val(wqe, 8, (uintptr_t)op_info->va);
	set_64bit_val(wqe, 16,
		      FIELD_PREP(IRDMAQPSQ_PARENTMRSTAG, op_info->mr_stag));
	set_64bit_val(wqe, 24, op_info->bind_len);
}

/**
 * zxdh_copy_inline_data - Copy inline data to wqe
 * @dest: pointer to wqe
 * @src: pointer to inline data
 * @len: length of inline data to copy
 * @polarity: polarity of wqe valid bit
 * @imm_data_flag: flag to imm_data
 */
static void zxdh_copy_inline_data(u8 *dest, u8 *src, u32 len, u8 polarity,
				  bool imm_data_flag)
{
	u8 inline_valid = polarity << ZXDH_INLINE_VALID_S;
	u32 copy_size;
	u8 *inline_valid_addr;

	dest += ZXDH_WQE_SIZE_32; /* point to additional 32 byte quanta */

	if (len) {
		inline_valid_addr = dest + WQE_OFFSET_7BYTES;
		if (imm_data_flag) {
			copy_size = len < INLINE_DATASIZE_24BYTES ?
					    len :
						  INLINE_DATASIZE_24BYTES;
			dest += WQE_OFFSET_8BYTES;
			memcpy(dest, src, copy_size);
			len -= copy_size;
			dest += WQE_OFFSET_24BYTES;
			src += copy_size;
		} else {
			if (len <= INLINE_DATASIZE_7BYTES) {
				copy_size = len;
				memcpy(dest, src, copy_size);
				*inline_valid_addr = inline_valid;
				return;
			}
			memcpy(dest, src, INLINE_DATASIZE_7BYTES);
			len -= INLINE_DATASIZE_7BYTES;
			dest += WQE_OFFSET_8BYTES;
			src += INLINE_DATA_OFFSET_7BYTES;
			copy_size = len < INLINE_DATASIZE_24BYTES ?
					    len :
						  INLINE_DATASIZE_24BYTES;
			memcpy(dest, src, copy_size);
			len -= copy_size;
			dest += WQE_OFFSET_24BYTES;
			src += copy_size;
		}
		*inline_valid_addr = inline_valid;
	}

	while (len) {
		inline_valid_addr = dest + WQE_OFFSET_7BYTES;
		if (len <= INLINE_DATASIZE_7BYTES) {
			copy_size = len;
			memcpy(dest, src, copy_size);
			*inline_valid_addr = inline_valid;
			return;
		} else {
			memcpy(dest, src, INLINE_DATASIZE_7BYTES);
			len -= INLINE_DATASIZE_7BYTES;
			dest += WQE_OFFSET_8BYTES;
			src += INLINE_DATA_OFFSET_7BYTES;
			copy_size = len < INLINE_DATASIZE_24BYTES ?
					    len :
						  INLINE_DATASIZE_24BYTES;
			memcpy(dest, src, copy_size);
			len -= copy_size;
			dest += WQE_OFFSET_24BYTES;
			src += copy_size;
		}
		*inline_valid_addr = inline_valid;
	}
}

/**
 * zxdh_inline_data_size_to_quanta - based on inline data, quanta
 * @data_size: data size for inline
 * @imm_data_flag: flag to imm_data
 * @imm_data_flag: flag for immediate data
 *
 * Gets the quanta based on inline and immediate data.
 */
static u16 zxdh_inline_data_size_to_quanta(u32 data_size, bool imm_data_flag)
{
	if (imm_data_flag)
		data_size += INLINE_DATASIZE_7BYTES;

	return data_size % 31 ? data_size / 31 + 2 : data_size / 31 + 1;
}

/**
 * zxdh_uk_inline_rdma_write - inline rdma write operation
 * @qp: hw qp ptr
 * @info: post sq information
 * @post_sq: flag to post sq
 */
int zxdh_uk_inline_rdma_write(struct zxdh_qp_uk *qp,
			      struct zxdh_post_sq_info *info, bool post_sq)
{
	__le64 *wqe;
	u8 imm_valid;
	struct zxdh_inline_rdma_write *op_info;
	u64 hdr = 0;
	u32 wqe_idx;
	bool read_fence = false;
	u16 quanta;
	bool imm_data_flag = info->imm_data_valid ? 1 : 0;

	op_info = &info->op.inline_rdma_write;

	if (op_info->len > qp->max_inline_data)
		return -EINVAL;
	if (imm_data_flag && op_info->len > ZXDH_MAX_SQ_INLINE_DATELEN_WITH_IMM)
		return -EINVAL;

	quanta = qp->wqe_ops.iw_inline_data_size_to_quanta(op_info->len,
							   imm_data_flag);
	wqe = zxdh_qp_get_next_send_wqe(qp, &wqe_idx, quanta, op_info->len,
					info);
	if (!wqe)
		return -ENOSPC;

	zxdh_clr_wqes(qp, wqe_idx);

	read_fence |= info->read_fence;
	hdr = FIELD_PREP(IRDMAQPSQ_VALID, qp->swqe_polarity) |
	      FIELD_PREP(IRDMAQPSQ_OPCODE, info->op_type) |
	      FIELD_PREP(IRDMAQPSQ_SIGCOMPL, info->signaled) |
	      FIELD_PREP(IRDMAQPSQ_LOCALFENCE, info->local_fence) |
	      FIELD_PREP(IRDMAQPSQ_READFENCE, read_fence) |
	      FIELD_PREP(IRDMAQPSQ_SOLICITED, info->solicited) |
	      FIELD_PREP(IRDMAQPSQ_IMMDATAFLAG, imm_data_flag) |
	      FIELD_PREP(IRDMAQPSQ_WRITE_INLINEDATAFLAG, 1) |
	      FIELD_PREP(IRDMAQPSQ_WRITE_INLINEDATALEN, op_info->len) |
	      FIELD_PREP(IRDMAQPSQ_ADDFRAGCNT, quanta - 1) |
	      FIELD_PREP(IRDMAQPSQ_REMSTAG, op_info->rem_addr.stag);
	set_64bit_val(wqe, 24,
		      FIELD_PREP(IRDMAQPSQ_FRAG_TO, op_info->rem_addr.tag_off));

	if (imm_data_flag) {
		/* if inline exist, not update imm valid */
		imm_valid = (op_info->len == 0) ? qp->swqe_polarity :
							(!qp->swqe_polarity);
		set_64bit_val(wqe, 32,
			      FIELD_PREP(IRDMAQPSQ_IMMDATA_VALID, imm_valid) |
				      FIELD_PREP(IRDMAQPSQ_IMMDATA,
						 info->imm_data));
	}

	qp->wqe_ops.iw_copy_inline_data((u8 *)wqe, op_info->data, op_info->len,
					qp->swqe_polarity, imm_data_flag);
	dma_wmb(); /* make sure WQE is populated before valid bit is set */

	set_64bit_val(wqe, 0, hdr);

	if (post_sq)
		zxdh_uk_qp_post_wr(qp);

	return 0;
}

/**
 * zxdh_uk_rc_inline_send - inline send operation
 * @qp: hw qp ptr
 * @info: post sq information
 * @post_sq: flag to post sq
 */
int zxdh_uk_rc_inline_send(struct zxdh_qp_uk *qp,
			   struct zxdh_post_sq_info *info, bool post_sq)
{
	__le64 *wqe;
	u8 imm_valid;
	struct zxdh_post_inline_send *op_info;
	u64 hdr;
	u32 wqe_idx;
	bool read_fence = false;
	u16 quanta;
	bool imm_data_flag = info->imm_data_valid ? 1 : 0;

	op_info = &info->op.inline_send;

	if (op_info->len > qp->max_inline_data)
		return -EINVAL;
	if (imm_data_flag && op_info->len > ZXDH_MAX_SQ_INLINE_DATELEN_WITH_IMM)
		return -EINVAL;

	quanta = qp->wqe_ops.iw_inline_data_size_to_quanta(op_info->len,
							   imm_data_flag);
	wqe = zxdh_qp_get_next_send_wqe(qp, &wqe_idx, quanta, op_info->len,
					info);
	if (!wqe)
		return -ENOSPC;

	zxdh_clr_wqes(qp, wqe_idx);

	read_fence |= info->read_fence;
	hdr = FIELD_PREP(IRDMAQPSQ_VALID, qp->swqe_polarity) |
	      FIELD_PREP(IRDMAQPSQ_OPCODE, info->op_type) |
	      FIELD_PREP(IRDMAQPSQ_SIGCOMPL, info->signaled) |
	      FIELD_PREP(IRDMAQPSQ_LOCALFENCE, info->local_fence) |
	      FIELD_PREP(IRDMAQPSQ_READFENCE, read_fence) |
	      FIELD_PREP(IRDMAQPSQ_SOLICITED, info->solicited) |
	      FIELD_PREP(IRDMAQPSQ_ADDFRAGCNT, quanta - 1) |
	      FIELD_PREP(IRDMAQPSQ_IMMDATAFLAG, imm_data_flag) |
	      FIELD_PREP(IRDMAQPSQ_REMSTAG, info->stag_to_inv);
	set_64bit_val(wqe, 24,
		      FIELD_PREP(IRDMAQPSQ_INLINEDATAFLAG, 1) |
			      FIELD_PREP(IRDMAQPSQ_INLINEDATALEN,
					 op_info->len));

	if (imm_data_flag) {
		/* if inline exist, not update imm valid */
		imm_valid = (op_info->len == 0) ? qp->swqe_polarity :
							(!qp->swqe_polarity);
		set_64bit_val(wqe, 32,
			      FIELD_PREP(IRDMAQPSQ_IMMDATA_VALID, imm_valid) |
				      FIELD_PREP(IRDMAQPSQ_IMMDATA,
						 info->imm_data));
	}

	qp->wqe_ops.iw_copy_inline_data((u8 *)wqe, op_info->data, op_info->len,
					qp->swqe_polarity, imm_data_flag);

	dma_wmb(); /* make sure WQE is populated before valid bit is set */

	set_64bit_val(wqe, 0, hdr);

	if (post_sq)
		zxdh_uk_qp_post_wr(qp);

	return 0;
}

/**
 * zxdh_uk_ud_inline_send - inline send operation
 * @qp: hw qp ptr
 * @info: post sq information
 * @post_sq: flag to post sq
 */
int zxdh_uk_ud_inline_send(struct zxdh_qp_uk *qp,
			   struct zxdh_post_sq_info *info, bool post_sq)
{
	__le64 *wqe_base;
	__le64 *wqe_ex;
	struct zxdh_post_inline_send *op_info;
	u64 hdr;
	u32 wqe_idx;
	bool read_fence = false;
	u16 quanta;
	bool imm_data_flag = info->imm_data_valid ? 1 : 0;
	u8 *inline_dest;
	u8 *inline_src;
	u32 inline_len;
	u32 copy_size;
	u8 *inline_valid_addr;

	op_info = &info->op.inline_send;
	inline_len = op_info->len;

	if (op_info->len > qp->max_inline_data)
		return -EINVAL;
	if (imm_data_flag && op_info->len > ZXDH_MAX_SQ_INLINE_DATELEN_WITH_IMM)
		return -EINVAL;

	quanta = qp->wqe_ops.iw_inline_data_size_to_quanta(op_info->len,
							   imm_data_flag);
	if (quanta > ZXDH_SQ_RING_FREE_QUANTA(qp->sq_ring))
		return -ENOSPC;

	wqe_idx = ZXDH_RING_CURRENT_HEAD(qp->sq_ring);
	if (!wqe_idx)
		qp->swqe_polarity = !qp->swqe_polarity;

	ZXDH_RING_MOVE_HEAD_BY_COUNT_NOCHECK(qp->sq_ring, quanta);

	wqe_base = qp->sq_base[wqe_idx].elem;
	qp->sq_wrtrk_array[wqe_idx].wrid = info->wr_id;
	qp->sq_wrtrk_array[wqe_idx].wr_len = op_info->len;
	qp->sq_wrtrk_array[wqe_idx].quanta = quanta;

	zxdh_clr_wqes(qp, wqe_idx);

	read_fence |= info->read_fence;
	hdr = FIELD_PREP(IRDMAQPSQ_VALID, qp->swqe_polarity) |
	      FIELD_PREP(IRDMAQPSQ_OPCODE, info->op_type) |
	      FIELD_PREP(IRDMAQPSQ_SIGCOMPL, info->signaled) |
	      FIELD_PREP(IRDMAQPSQ_SOLICITED, info->solicited) |
	      FIELD_PREP(IRDMAQPSQ_IMMDATAFLAG, imm_data_flag) |
	      FIELD_PREP(IRDMAQPSQ_UD_INLINEDATAFLAG, 1) |
	      FIELD_PREP(IRDMAQPSQ_UD_INLINEDATALEN, op_info->len) |
	      FIELD_PREP(IRDMAQPSQ_UD_ADDFRAGCNT, quanta - 1) |
	      FIELD_PREP(IRDMAQPSQ_AHID, op_info->ah_id);
	set_64bit_val(wqe_base, 24,
		      FIELD_PREP(IRDMAQPSQ_DESTQPN, op_info->dest_qp) |
			      FIELD_PREP(IRDMAQPSQ_DESTQKEY, op_info->qkey));

	if (imm_data_flag) {
		wqe_idx = (wqe_idx + 1) % qp->sq_ring.size;
		if (!wqe_idx)
			qp->swqe_polarity = !qp->swqe_polarity;
		wqe_ex = qp->sq_base[wqe_idx].elem;

		if (inline_len) {
			/* imm and inline use the same valid, valid set after inline data updated*/
			copy_size = inline_len < INLINE_DATASIZE_24BYTES ?
					    inline_len :
						  INLINE_DATASIZE_24BYTES;
			inline_dest = (u8 *)wqe_ex + WQE_OFFSET_8BYTES;
			inline_src = (u8 *)op_info->data;
			memcpy(inline_dest, inline_src, copy_size);
			inline_len -= copy_size;
			inline_src += copy_size;
		}
		set_64bit_val(
			wqe_ex, 0,
			FIELD_PREP(IRDMAQPSQ_IMMDATA_VALID, qp->swqe_polarity) |
				FIELD_PREP(IRDMAQPSQ_IMMDATA, info->imm_data));

	} else if (inline_len) {
		wqe_idx = (wqe_idx + 1) % qp->sq_ring.size;
		if (!wqe_idx)
			qp->swqe_polarity = !qp->swqe_polarity;
		wqe_ex = qp->sq_base[wqe_idx].elem;
		inline_dest = (u8 *)wqe_ex;
		inline_src = (u8 *)op_info->data;

		if (inline_len <= INLINE_DATASIZE_7BYTES) {
			copy_size = inline_len;
			memcpy(inline_dest, inline_src, copy_size);
			inline_len = 0;
		} else {
			copy_size = INLINE_DATASIZE_7BYTES;
			memcpy(inline_dest, inline_src, copy_size);
			inline_len -= copy_size;
			inline_src += copy_size;
			inline_dest += WQE_OFFSET_8BYTES;
			copy_size = inline_len < INLINE_DATASIZE_24BYTES ?
					    inline_len :
						  INLINE_DATASIZE_24BYTES;
			memcpy(inline_dest, inline_src, copy_size);
			inline_len -= copy_size;
			inline_src += copy_size;
		}
		inline_valid_addr = (u8 *)wqe_ex + WQE_OFFSET_7BYTES;
		*inline_valid_addr = qp->swqe_polarity << ZXDH_INLINE_VALID_S;
	}

	while (inline_len) {
		wqe_idx = (wqe_idx + 1) % qp->sq_ring.size;
		if (!wqe_idx)
			qp->swqe_polarity = !qp->swqe_polarity;
		wqe_ex = qp->sq_base[wqe_idx].elem;
		inline_dest = (u8 *)wqe_ex;

		if (inline_len <= INLINE_DATASIZE_7BYTES) {
			copy_size = inline_len;
			memcpy(inline_dest, inline_src, copy_size);
			inline_len = 0;
		} else {
			copy_size = INLINE_DATASIZE_7BYTES;
			memcpy(inline_dest, inline_src, copy_size);
			inline_len -= copy_size;
			inline_src += copy_size;
			inline_dest += WQE_OFFSET_8BYTES;
			copy_size = inline_len < INLINE_DATASIZE_24BYTES ?
					    inline_len :
						  INLINE_DATASIZE_24BYTES;
			memcpy(inline_dest, inline_src, copy_size);
			inline_len -= copy_size;
			inline_src += copy_size;
		}
		inline_valid_addr = (u8 *)wqe_ex + WQE_OFFSET_7BYTES;
		*inline_valid_addr = qp->swqe_polarity << ZXDH_INLINE_VALID_S;
	}

	dma_wmb(); /* make sure WQE is populated before valid bit is set */

	set_64bit_val(wqe_base, 0, hdr);

	if (post_sq)
		zxdh_uk_qp_post_wr(qp);

	return 0;
}

/**
 * zxdh_uk_stag_local_invalidate - stag invalidate operation
 * @qp: hw qp ptr
 * @info: post sq information
 * @post_sq: flag to post sq
 */
int zxdh_uk_stag_local_invalidate(struct zxdh_qp_uk *qp,
				  struct zxdh_post_sq_info *info, bool post_sq)
{
	__le64 *wqe;
	struct zxdh_inv_local_stag *op_info;
	u64 hdr;
	u32 wqe_idx;
	bool local_fence = true;

	op_info = &info->op.inv_local_stag;

	wqe = zxdh_qp_get_next_send_wqe(qp, &wqe_idx, ZXDH_QP_WQE_MIN_QUANTA, 0,
					info);
	if (!wqe)
		return -ENOSPC;

	zxdh_clr_wqes(qp, wqe_idx);

	hdr = FIELD_PREP(IRDMAQPSQ_VALID, qp->swqe_polarity) |
	      FIELD_PREP(IRDMAQPSQ_OPCODE, ZXDH_OP_TYPE_LOCAL_INV) |
	      FIELD_PREP(IRDMAQPSQ_SIGCOMPL, info->signaled) |
	      FIELD_PREP(IRDMAQPSQ_LOCALFENCE, local_fence) |
	      FIELD_PREP(IRDMAQPSQ_READFENCE, info->read_fence) |
	      FIELD_PREP(IRDMAQPSQ_REMSTAG, op_info->target_stag);

	dma_wmb(); /* make sure WQE is populated before valid bit is set */

	set_64bit_val(wqe, 0, hdr);

	if (post_sq)
		zxdh_uk_qp_post_wr(qp);

	return 0;
}

/**
 * zxdh_uk_mw_bind - bind Memory Window
 * @qp: hw qp ptr
 * @info: post sq information
 * @post_sq: flag to post sq
 */
int zxdh_uk_mw_bind(struct zxdh_qp_uk *qp, struct zxdh_post_sq_info *info,
		    bool post_sq)
{
	__le64 *wqe;
	struct zxdh_bind_window *op_info;
	u64 hdr;
	u32 wqe_idx;
	bool local_fence;

	info->push_wqe = qp->push_db ? true : false;
	op_info = &info->op.bind_window;
	local_fence = info->local_fence;

	wqe = zxdh_qp_get_next_send_wqe(qp, &wqe_idx, ZXDH_QP_WQE_MIN_QUANTA, 0,
					info);
	if (!wqe)
		return -ENOSPC;

	zxdh_clr_wqes(qp, wqe_idx);

	qp->wqe_ops.iw_set_mw_bind_wqe(wqe, op_info);

	hdr = FIELD_PREP(IRDMAQPSQ_OPCODE, ZXDH_OP_TYPE_BIND_MW) |
	      FIELD_PREP(IRDMAQPSQ_MWSTAG, op_info->mw_stag) |
	      FIELD_PREP(IRDMAQPSQ_STAGRIGHTS, ((op_info->ena_reads << 2) |
						(op_info->ena_writes << 3))) |
	      FIELD_PREP(IRDMAQPSQ_VABASEDTO,
			 (op_info->addressing_type == ZXDH_ADDR_TYPE_VA_BASED ?
				  1 :
					0)) |
	      FIELD_PREP(IRDMAQPSQ_MEMWINDOWTYPE,
			 (op_info->mem_window_type_1 ? 1 : 0)) |
	      FIELD_PREP(IRDMAQPSQ_READFENCE, info->read_fence) |
	      FIELD_PREP(IRDMAQPSQ_LOCALFENCE, local_fence) |
	      FIELD_PREP(IRDMAQPSQ_SIGCOMPL, info->signaled) |
	      FIELD_PREP(IRDMAQPSQ_VALID, qp->swqe_polarity);

	dma_wmb(); /* make sure WQE is populated before valid bit is set */

	set_64bit_val(wqe, 0, hdr);

	if (post_sq)
		zxdh_uk_qp_post_wr(qp);

	return 0;
}

/**
 * zxdh_uk_post_receive - post receive wqe
 * @qp: hw qp ptr
 * @info: post rq information
 */
int zxdh_uk_post_receive(struct zxdh_qp_uk *qp, struct zxdh_post_rq_info *info)
{
	u32 wqe_idx, i, byte_off;
	__le64 *wqe;
	struct zxdh_sge *sge;

	if (qp->max_rq_frag_cnt < info->num_sges)
		return -EINVAL;

	wqe = zxdh_qp_get_next_recv_wqe(qp, &wqe_idx);
	if (!wqe)
		return -ENOSPC;

	qp->rq_wrid_array[wqe_idx] = info->wr_id;

	for (i = 0, byte_off = ZXDH_QP_FRAG_BYTESIZE; i < info->num_sges; i++) {
		sge = &info->sg_list[i];
		set_64bit_val(wqe, byte_off, sge->tag_off);
		set_64bit_val(wqe, byte_off + 8,
			      FIELD_PREP(IRDMAQPRQ_FRAG_LEN, sge->len) |
				      FIELD_PREP(IRDMAQPRQ_STAG, sge->stag));
		byte_off += ZXDH_QP_FRAG_BYTESIZE;
	}

	/*
	 * while info->num_sges < qp->max_rq_frag_cnt, or 0 == info->num_sges,
	 * fill next fragment with FRAG_LEN=0, FRAG_STAG=0x00000100,
	 * witch indicates a invalid fragment
	 */
	if (info->num_sges < qp->max_rq_frag_cnt || 0 == info->num_sges) {
		set_64bit_val(wqe, byte_off, 0);
		set_64bit_val(wqe, byte_off + 8,
			      FIELD_PREP(IRDMAQPRQ_FRAG_LEN, 0) |
				      FIELD_PREP(IRDMAQPRQ_STAG, 0x00000100));
	}

	set_64bit_val(wqe, 0,
		      FIELD_PREP(IRDMAQPRQ_ADDFRAGCNT, info->num_sges) |
			      FIELD_PREP(IRDMAQPRQ_SIGNATURE,
					 qp->rwqe_signature));

	dma_wmb(); /* make sure WQE is populated before valid bit is set */

	if (info->num_sges > 3) {
		udelay(1);
	}

	set_64bit_val(wqe, 8, FIELD_PREP(IRDMAQPRQ_VALID, qp->rwqe_polarity));

	return 0;
}

/**
 * zxdh_uk_cq_resize - reset the cq buffer info
 * @cq: cq to resize
 * @cq_base: new cq buffer addr
 * @cq_size: number of cqes
 */
void zxdh_uk_cq_resize(struct zxdh_cq_uk *cq, void *cq_base, int cq_size)
{
	cq->cq_base = cq_base;
	cq->cq_size = cq_size;
	cq->cq_log_size = zxdh_num_to_log(cq_size);
	ZXDH_RING_INIT(cq->cq_ring, cq->cq_size);
	cq->polarity = 1;
}

/**
 * zxdh_uk_cq_set_resized_cnt - record the count of the resized buffers
 * @cq: cq to resize
 * @cq_cnt: the count of the resized cq buffers
 */
void zxdh_uk_cq_set_resized_cnt(struct zxdh_cq_uk *cq, u16 cq_cnt)
{
	u64 temp_val;
	u16 sw_cq_sel;
	u8 arm_next;
	u8 arm_seq_num;

	get_64bit_val(cq->shadow_area, 0, &temp_val);

	sw_cq_sel = (u16)FIELD_GET(ZXDH_CQ_DBSA_SW_CQ_SELECT, temp_val);
	sw_cq_sel += cq_cnt;

	arm_seq_num = (u8)FIELD_GET(ZXDH_CQ_DBSA_ARM_SEQ_NUM, temp_val);
	arm_next = (u8)FIELD_GET(ZXDH_CQ_DBSA_ARM_NEXT, temp_val);
	cq->cqe_rd_cnt = 0;

	temp_val = FIELD_PREP(ZXDH_CQ_DBSA_ARM_SEQ_NUM, arm_seq_num) |
		   FIELD_PREP(ZXDH_CQ_DBSA_SW_CQ_SELECT, sw_cq_sel) |
		   FIELD_PREP(ZXDH_CQ_DBSA_ARM_NEXT, arm_next) |
		   FIELD_PREP(ZXDH_CQ_DBSA_CQEIDX, cq->cqe_rd_cnt);

	set_64bit_val(cq->shadow_area, 0, temp_val);
}

/**
 * zxdh_uk_cq_request_notification - cq notification request (door bell)
 * @cq: hw cq
 * @cq_notify: notification type
 */
void zxdh_uk_cq_request_notification(struct zxdh_cq_uk *cq,
				     enum zxdh_cmpl_notify cq_notify)
{
	u64 temp_val;
	u16 sw_cq_sel;
	u8 arm_next = 0;
	u8 arm_seq_num;
	u32 cqe_index;
	u32 hdr;

	cq->armed = true;
	get_64bit_val(cq->shadow_area, 0, &temp_val);
	arm_seq_num = (u8)FIELD_GET(ZXDH_CQ_DBSA_ARM_SEQ_NUM, temp_val);
	arm_seq_num++;
	sw_cq_sel = (u16)FIELD_GET(ZXDH_CQ_DBSA_SW_CQ_SELECT, temp_val);
	cqe_index = (u32)FIELD_GET(ZXDH_CQ_DBSA_CQEIDX, temp_val);

	if (cq_notify == ZXDH_CQ_COMPL_SOLICITED)
		arm_next = 1;
	temp_val = FIELD_PREP(ZXDH_CQ_DBSA_ARM_SEQ_NUM, arm_seq_num) |
		   FIELD_PREP(ZXDH_CQ_DBSA_SW_CQ_SELECT, sw_cq_sel) |
		   FIELD_PREP(ZXDH_CQ_DBSA_ARM_NEXT, arm_next) |
		   FIELD_PREP(ZXDH_CQ_DBSA_CQEIDX, cqe_index);

	set_64bit_val(cq->shadow_area, 0, temp_val);

	hdr = FIELD_PREP(ZXDH_CQ_ARM_DBSA_VLD, 0) |
	      FIELD_PREP(ZXDH_CQ_ARM_CQ_ID, cq->cq_id);

	dma_wmb(); /* make sure WQE is populated before valid bit is set */

	writel(hdr, cq->cqe_alloc_db);
}

static enum zxdh_status_code
process_tx_window_query_item_err(struct zxdh_qp_uk *qp,
				 				 struct zxdh_cq_poll_info *info)
{
	int ret;
	struct zxdh_sc_qp *sc_qp;
	struct zxdh_qp *iwqp;
	struct zxdh_dma_mem qpc = { 0 };
	u32 tx_last_ack_psn;

	if (in_interrupt() || in_atomic())
		return ZXDH_ERR_RETRY_ACK_NOT_EXCEED_ERR;

	sc_qp = container_of(qp, struct zxdh_sc_qp, qp_uk);
	iwqp = container_of(sc_qp, struct zxdh_qp, sc_qp);

	qpc.size = ALIGN(ZXDH_QP_CTX_SIZE, ZXDH_QPC_ALIGNMENT);
	qpc.va = dma_alloc_coherent(iwqp->iwdev->rf->sc_dev.hw->device,
					qpc.size, &qpc.pa, GFP_KERNEL);
	if (!qpc.va) {
		pr_err("no memory\n");
		return ZXDH_ERR_RETRY_ACK_ERR;
	}

	ret = zxdh_query_qpc(sc_qp, &qpc);

	if (ret) {
		zxdh_dbg(sc_qp->dev, "process retry ack query qpc failed:%d\n",
			 ret);
		dma_free_coherent(iwqp->iwdev->rf->sc_dev.hw->device, qpc.size,
				  qpc.va, qpc.pa);
		return ZXDH_ERR_RETRY_ACK_ERR;
	}

	tx_last_ack_psn = ZXDH_GET_QPC_ITEM(u32, qpc.va, ZXDH_QPC_TX_LAST_ACK_PSN_BYTE_OFFSET,
				  RDMAQPC_TX_LAST_ACK_PSN);

	if (tx_last_ack_psn != qp->qp_last_ack_psn)
		qp->qp_reset_cnt = 0;

	qp->qp_last_ack_psn = tx_last_ack_psn;
	if (qp->qp_reset_cnt >= ZXDH_QP_RETRY_COUNT) {
		dma_free_coherent(iwqp->iwdev->rf->sc_dev.hw->device, qpc.size,
				  qpc.va, qpc.pa);
		return ZXDH_ERR_RETRY_ACK_ERR;
	}
		
	ret = zxdh_reset_retry_tx_item(iwqp);
	if (ret) {
		dma_free_coherent(iwqp->iwdev->rf->sc_dev.hw->device, qpc.size,
				  qpc.va, qpc.pa);
		return ZXDH_ERR_RETRY_ACK_ERR;
	}
	qp->qp_reset_cnt++;

	dma_free_coherent(iwqp->iwdev->rf->sc_dev.hw->device, qpc.size,
			  qpc.va, qpc.pa);
	return ZXDH_ERR_RETRY_ACK_NOT_EXCEED_ERR;
}

static enum zxdh_status_code
process_retry_ack_err(struct zxdh_qp_uk *qp, struct zxdh_cq_poll_info *info)
{
	int ret;
	struct zxdh_sc_qp *sc_qp;
	struct zxdh_qp *iwqp;
	struct zxdh_dma_mem qpc = { 0 };
	struct zxdh_modify_qpc_item modify_item = { 0 };
	u64 modify_mask = 0;

	u8 retry_cqe_sq_opcode, recv_err_flag;
	u32 tx_last_ack_psn;

	if (in_interrupt() || in_atomic())
		return ZXDH_ERR_RETRY_ACK_NOT_EXCEED_ERR;

	sc_qp = container_of(qp, struct zxdh_sc_qp, qp_uk);
	iwqp = container_of(sc_qp, struct zxdh_qp, sc_qp);

	qpc.size = ALIGN(ZXDH_QP_CTX_SIZE, ZXDH_QPC_ALIGNMENT);
	qpc.va = dma_alloc_coherent(iwqp->iwdev->rf->sc_dev.hw->device,
					qpc.size, &qpc.pa, GFP_KERNEL);
	if (!qpc.va) {
		pr_err("no memory\n");
		return ZXDH_ERR_RETRY_ACK_ERR;
	}

	ret = zxdh_query_qpc(sc_qp, &qpc);
	if (ret) {
		zxdh_dbg(sc_qp->dev, "process retry ack query qpc failed:%d\n",
			 ret);
		dma_free_coherent(iwqp->iwdev->rf->sc_dev.hw->device, qpc.size,
				  qpc.va, qpc.pa);
		return ZXDH_ERR_RETRY_ACK_ERR;
	}
	retry_cqe_sq_opcode = ZXDH_GET_QPC_ITEM(
		u8, qpc.va, ZXDH_QPC_RETRY_CQE_SQ_OPCODE_BYTE_OFFSET,
		RDMAQPC_TX_RETRY_CQE_SQ_OPCODE_FLAG);

	recv_err_flag = ZXDH_GET_QPC_ITEM(u8, qpc.va, ZXDH_QPC_RECV_ERR_FLAG_BYTE_OFFSET,
				  RDMAQPC_TX_RECV_ERR_FLAG);

	tx_last_ack_psn = ZXDH_GET_QPC_ITEM(u32, qpc.va, ZXDH_QPC_TX_LAST_ACK_PSN_BYTE_OFFSET,
				  RDMAQPC_TX_LAST_ACK_PSN);

	if (!(retry_cqe_sq_opcode >= ZXDH_RETRY_CQE_SQ_OPCODE_ERR &&
	      (recv_err_flag == ZXDH_RECV_ERR_FLAG_NAK_RNR_NAK ||
	       recv_err_flag == ZXDH_RECV_ERR_FLAG_READ_RESP))) {
		dma_free_coherent(iwqp->iwdev->rf->sc_dev.hw->device, qpc.size,
				  qpc.va, qpc.pa);
		return ZXDH_ERR_RETRY_ACK_ERR;
	}
	if (tx_last_ack_psn != qp->cqe_last_ack_psn) {
		qp->cqe_retry_cnt = 0;
	}

	qp->cqe_last_ack_psn = tx_last_ack_psn;
	if (qp->cqe_retry_cnt >= ZXDH_QP_RETRY_COUNT) {
		dma_free_coherent(iwqp->iwdev->rf->sc_dev.hw->device, qpc.size,
				  qpc.va, qpc.pa);
		return ZXDH_ERR_RETRY_ACK_ERR;
	}

	modify_item.retry_flag = ZXDH_GET_QPC_ITEM(
		u8, qpc.va, ZXDH_QPC_RETRY_FALG_BYTE_OFFSET, RDMAQPC_TX_RETRY_FLAG);
	modify_item.rnr_retry_flag =
		ZXDH_GET_QPC_ITEM(u8, qpc.va, ZXDH_QPC_RNR_RETRY_FALG_BYTE_OFFSET,
				  RDMAQPC_TX_RNR_RETRY_FLAG);
	modify_item.read_retry_flag =
		ZXDH_GET_QPC_ITEM(u8, qpc.va, ZXDH_QPC_READ_RETRY_FALG_BYTE_OFFSET,
				  RDMAQPC_TX_READ_RETRY_FLAG);
	// Cur_Retry_Count置为Retry_Count
	modify_item.cur_retry_count =
		ZXDH_GET_QPC_ITEM(u8, qpc.va, ZXDH_QPC_RETY_COUNT_BYTE_OFFSET, RDMAQPC_TX_RETRY_CNT);

	modify_item.retry_cqe_sq_opcode = (retry_cqe_sq_opcode & ZXDH_RESET_RETRY_CQE_SQ_OPCODE_ERR);
	modify_item.err_flag = 0;
	modify_item.ack_err_flag = 0;
	modify_item.package_err_flag = 0;

	modify_mask = ZXDH_PACKAGE_ERR_FLAG | ZXDH_ERR_FLAG_SET |
				      ZXDH_RETRY_CQE_SQ_OPCODE |
				      ZXDH_TX_READ_RETRY_FLAG_SET; 

	ret = process_hw_modify_qpc_cmd(iwqp, &modify_item, modify_mask);
	if (ret) {
		zxdh_dbg(sc_qp->dev,
			 "process retry ack modify qpc failed:%d\n", ret);
		dma_free_coherent(iwqp->iwdev->rf->sc_dev.hw->device, qpc.size,
				  qpc.va, qpc.pa);
		return ZXDH_ERR_RETRY_ACK_ERR;
	}
	qp->cqe_retry_cnt++;

	dma_free_coherent(iwqp->iwdev->rf->sc_dev.hw->device, qpc.size,
			  qpc.va, qpc.pa);
	return ZXDH_ERR_RETRY_ACK_NOT_EXCEED_ERR;
}

static inline void build_comp_status(u32 cq_type, struct zxdh_cq_poll_info *info)
{
	if (!info->error) {
		info->comp_status = ZXDH_COMPL_STATUS_SUCCESS;
		if (cq_type == ZXDH_CQE_QTYPE_RQ) {
			if (info->major_err != ERROR_CODE_VALUE &&
			    info->minor_err != ERROR_CODE_VALUE) {
				info->comp_status = ZXDH_COMPL_STATUS_UNKNOWN;
			}
		}
		return;
	}

	switch (info->major_err) {
		case ZXDH_RETRY_ACK_MAJOR_ERR:
			if (info->minor_err == ZXDH_RETRY_ACK_MINOR_ERR) {
				info->comp_status = ZXDH_COMPL_STATUS_RETRY_ACK_ERR;
				return;
			}
			if (info->minor_err == ZXDH_TX_WINDOW_QUERY_ITEM_MINOR_ERR) {
				info->comp_status =
					ZXDH_COMPL_STATUS_TX_WINDOW_QUERY_ITEM_ERR;
				return;
			}
			break;
		case ZXDH_FLUSH_MAJOR_ERR:
			info->comp_status = ZXDH_COMPL_STATUS_FLUSHED;
			return;
		default:
			info->comp_status = ZXDH_COMPL_STATUS_UNKNOWN;
			return;
	}
}
static enum zxdh_status_code
zxdh_flush_sq_comp_info(struct zxdh_qp_uk *qp, struct zxdh_cq_poll_info *info,
			bool *move_cq_head)
{
	if (!ZXDH_RING_MORE_WORK(qp->sq_ring)) {
		ZXDH_RING_INIT(qp->sq_ring, qp->sq_ring.size)
		return ZXDH_ERR_Q_EMPTY;
	}
	do {
		__le64 *sw_wqe;
		__u64 wqe_qword;
		__u64 wqe_idx;
		wqe_idx = qp->sq_ring.tail;
		sw_wqe = qp->sq_base[wqe_idx].elem;
		get_64bit_val(sw_wqe, 0, &wqe_qword);
		info->op_type = (__u8)FIELD_GET(IRDMAQPSQ_OPCODE, wqe_qword);
		ZXDH_RING_SET_TAIL(qp->sq_ring,
				   wqe_idx +
					   qp->sq_wrtrk_array[wqe_idx].quanta);

		if (info->op_type != ZXDH_OP_TYPE_NOP) {
			info->wr_id = qp->sq_wrtrk_array[wqe_idx].wrid;
			break;
		}
	} while (1);
	qp->sq_flush_seen = true;
	if (!ZXDH_RING_MORE_WORK(qp->sq_ring)) {
		qp->sq_flush_complete = true;
		ZXDH_RING_INIT(qp->sq_ring, qp->sq_ring.size)
	} else
		*move_cq_head = false;
	return ZXDH_SUCCESS;
}

static enum zxdh_status_code update_cq_poll_info(struct zxdh_qp_uk *qp,
						 struct zxdh_cq_poll_info *info,
						 __u32 wqe_idx, __u64 qword0)
{
	info->wr_id = qp->sq_wrtrk_array[wqe_idx].wrid;
	if (!info->comp_status)
		info->bytes_xfered = qp->sq_wrtrk_array[wqe_idx].wr_len;
	info->op_type = (__u8)FIELD_GET(IRDMACQ_OP, qword0);
	ZXDH_RING_SET_TAIL(qp->sq_ring,
			   wqe_idx + qp->sq_wrtrk_array[wqe_idx].quanta);
	return ZXDH_SUCCESS;
}

static enum zxdh_status_code zxdh_sq_comp_info(struct zxdh_qp_uk *qp,
					       					   struct zxdh_cq_poll_info *info,
					       					   u32 wqe_idx, u64 qword0,
					       					   bool *move_cq_head)
{
	enum zxdh_status_code status_code;
	switch (info->comp_status) {
		case ZXDH_COMPL_STATUS_SUCCESS:
		case ZXDH_COMPL_STATUS_UNKNOWN:
			break;
		case ZXDH_COMPL_STATUS_RETRY_ACK_ERR:
			if (qp->qp_type == ZXDH_QP_TYPE_ROCE_RC) {
				status_code = process_retry_ack_err(qp, info);
				return (status_code == ZXDH_ERR_RETRY_ACK_ERR) ?
					       update_cq_poll_info(qp, info, wqe_idx,
								   qword0) :
					       status_code;
			}
			break;
		case ZXDH_COMPL_STATUS_TX_WINDOW_QUERY_ITEM_ERR:
			if (qp->qp_type == ZXDH_QP_TYPE_ROCE_RC) {
				status_code = process_tx_window_query_item_err(qp, info);
				return (status_code == ZXDH_ERR_RETRY_ACK_ERR) ?
					       update_cq_poll_info(qp, info, wqe_idx,
								   qword0) :
					       status_code;
			}
			break;
		case ZXDH_COMPL_STATUS_FLUSHED:
			return zxdh_flush_sq_comp_info(qp, info, move_cq_head);
		default:
			break;
	}
	return update_cq_poll_info(qp, info, wqe_idx, qword0);
}

static void zxdh_get_cq_poll_info(struct zxdh_qp_uk *qp,
					 			  struct zxdh_cq_poll_info *info,
					 			  u64 qword2, u64 qword3, u64 qword4)
{
	u8 qp_type;

	qp_type = qp->qp_type;

	info->imm_valid = (bool)FIELD_GET(ZXDH_CQ_IMMVALID, qword2);
	if (info->imm_valid) {
		info->imm_data = (__u32)FIELD_GET(ZXDH_CQ_IMMDATA, qword3);
		info->op_type = ZXDH_OP_TYPE_REC_IMM;
	} else {
		info->op_type = ZXDH_OP_TYPE_REC;
	}

	info->bytes_xfered = (u32)FIELD_GET(IRDMACQ_PAYLDLEN, qword3);

	if (likely(qp_type == ZXDH_QP_TYPE_ROCE_RC)) {
		if (qword2 & IRDMACQ_STAG) {
			info->stag_invalid_set = true;
			info->inv_stag =
				(u32)FIELD_GET(IRDMACQ_INVSTAG, qword2);
		} else {
			info->stag_invalid_set = false;
		}
	} else if (qp_type == ZXDH_QP_TYPE_ROCE_UD) {
		info->ipv4 = (bool)FIELD_GET(IRDMACQ_IPV4, qword2);
		info->ud_src_qpn = (__u32)FIELD_GET(IRDMACQ_UDSRCQPN, qword2);
		info->ud_smac_valid =
					(bool)FIELD_GET(ZXDH_CQ_UDSMACVALID, qword2);
		info->ud_vlan_valid =
			(bool)FIELD_GET(ZXDH_CQ_UDVLANVALID, qword2);
		if (info->ud_smac_valid || info->ud_vlan_valid) {
			if (info->ud_vlan_valid)
				info->ud_vlan = (u16)FIELD_GET(
					ZXDH_CQ_UDVLAN, qword4);
			if (info->ud_smac_valid) {
				info->ud_smac[5] = qword4 & 0xFF;
				info->ud_smac[4] = (qword4 >> 8) & 0xFF;
				info->ud_smac[3] = (qword4 >> 16) &
						   0xFF;
				info->ud_smac[2] = (qword4 >> 24) &
						   0xFF;
				info->ud_smac[1] = (qword4 >> 32) &
						   0xFF;
				info->ud_smac[0] = (qword4 >> 40) &
						   0xFF;
			}
		}
	}
}

static enum zxdh_status_code zxdh_rq_comp_info(struct zxdh_qp_uk *qp,
					       					   struct zxdh_cq_poll_info *info,
					       					   u32 wqe_idx, __le64 *cqe, bool *move_cq_head)
{
	struct zxdh_sc_qp *sc_qp = NULL;
	struct zxdh_sc_srq *sc_srq = NULL;
	struct zxdh_srq_uk *srq_uk= NULL;
	u64 qword2, qword3, qword4;
	get_64bit_val(cqe, 16, &qword2);
	get_64bit_val(cqe, 24, &qword3);
	get_64bit_val(cqe, 32, &qword4);
	if (qp->is_srq) {
		sc_qp = container_of(qp, struct zxdh_sc_qp, qp_uk);
		sc_srq = sc_qp->srq;
		srq_uk = &sc_srq->srq_uk;
		zxdh_free_srq_wqe(srq_uk, wqe_idx);
		info->wr_id = srq_uk->srq_wrid_array[wqe_idx];
		zxdh_get_cq_poll_info(qp, info, qword2, qword3, qword4);
	} else {
		if (unlikely(info->comp_status == ZXDH_COMPL_STATUS_FLUSHED ||
			     info->comp_status == ZXDH_COMPL_STATUS_UNKNOWN)) {
			if (!ZXDH_RING_MORE_WORK(qp->rq_ring)) {
				return ZXDH_ERR_Q_EMPTY;
			}
			wqe_idx = qp->rq_ring.tail;
		}
		info->wr_id = qp->rq_wrid_array[wqe_idx];
		zxdh_get_cq_poll_info(qp, info, qword2, qword3, qword4);
		ZXDH_RING_SET_TAIL(qp->rq_ring, wqe_idx + 1);
		if (info->comp_status == ZXDH_COMPL_STATUS_FLUSHED) {
			qp->rq_flush_seen = true;
			if (!ZXDH_RING_MORE_WORK(qp->rq_ring))
				qp->rq_flush_complete = true;
			else
				*move_cq_head = false;
		}
	}
	return ZXDH_SUCCESS;
}

/**
 * zxdh_uk_cq_poll_cmpl - get cq completion info
 * @cq: hw cq
 * @info: cq poll information returned
 */
enum zxdh_status_code zxdh_uk_cq_poll_cmpl(struct zxdh_cq_uk *cq, struct zxdh_cq_poll_info *info)
{
	u64 comp_ctx, qword0, qword2;
	__le64 *cqe;
	struct zxdh_qp_uk *qp;

	struct zxdh_ring *pring = NULL;
	u32 wqe_idx, q_type;
	enum zxdh_status_code ret_code = ZXDH_SUCCESS;
	bool move_cq_head = true;
	u8 polarity;

	if (cq->valid_cq == false)
		return ZXDH_ERR_Q_EMPTY;

	cqe = ZXDH_GET_CURRENT_EXTENDED_CQ_ELEM(cq);

	get_64bit_val(cqe, 0, &qword0);
	polarity = (u8)FIELD_GET(ZXDH_CQ_VALID, qword0);
	if (polarity != cq->polarity)
		return ZXDH_ERR_Q_EMPTY;

	/* Ensure CQE contents are read after valid bit is checked */
	dma_rmb();
	get_64bit_val(cqe, 8, &comp_ctx);
	get_64bit_val(cqe, 16, &qword2);

	qp = (struct zxdh_qp_uk *)(unsigned long)comp_ctx;
	if (!qp || qp->destroy_pending) {
		ret_code = ZXDH_ERR_Q_DESTROYED;
		goto exit;
	}

	info->qp_handle = (zxdh_qp_handle)(unsigned long)qp;
	q_type = (u8)FIELD_GET(ZXDH_CQ_SQ, qword0);
	info->solicited_event = (bool)FIELD_GET(IRDMACQ_SOEVENT, qword0);
	wqe_idx = (u32)FIELD_GET(ZXDH_CQ_WQEIDX, qword0);
	info->error = (bool)FIELD_GET(ZXDH_CQ_ERROR, qword0);
	info->major_err = FIELD_GET(ZXDH_CQ_MAJERR, qword0);
	info->minor_err = FIELD_GET(ZXDH_CQ_MINERR, qword0);

	if (unlikely(info->error && info->major_err == ZXDH_FLUSH_MAJOR_ERR &&
		     info->minor_err != FLUSH_GENERAL_ERR)) {
		qword0 &= ~ZXDH_CQ_MINERR;
		qword0 |= FIELD_PREP(ZXDH_CQ_MINERR, FLUSH_GENERAL_ERR);
		set_64bit_val(cqe, 0, qword0);
	}
	build_comp_status(q_type, info);

	info->qp_id = (u32)FIELD_GET(IRDMACQ_QPID, qword2);
	info->imm_valid = false;
	info->ud_smac_valid = false;
	info->ud_vlan_valid = false;

	info->qp_handle = (zxdh_qp_handle)(unsigned long)qp;

	if (q_type == ZXDH_CQE_QTYPE_SQ) {
		ret_code = zxdh_sq_comp_info(qp, info, wqe_idx, qword0, &move_cq_head);
		pring = &qp->sq_ring;
	} else if (q_type == ZXDH_CQE_QTYPE_RQ) {
		ret_code = zxdh_rq_comp_info(qp, info, wqe_idx, cqe, &move_cq_head);
		pring = &qp->rq_ring;
	}

exit:
	if (move_cq_head) {
		u64 cq_shadow_temp;

		ZXDH_RING_MOVE_HEAD_NOCHECK(cq->cq_ring);
		if (!ZXDH_RING_CURRENT_HEAD(cq->cq_ring))
			cq->polarity ^= 1;

		ZXDH_RING_MOVE_TAIL(cq->cq_ring);
		cq->cqe_rd_cnt++;
		get_64bit_val(cq->shadow_area, 0, &cq_shadow_temp);
		cq_shadow_temp &= ~ZXDH_CQ_DBSA_CQEIDX;
		cq_shadow_temp |=
			FIELD_PREP(ZXDH_CQ_DBSA_CQEIDX, cq->cqe_rd_cnt);
		set_64bit_val(cq->shadow_area, 0, cq_shadow_temp);
	} else {
		qword0 &= ~ZXDH_CQ_WQEIDX;
		qword0 |= FIELD_PREP(ZXDH_CQ_WQEIDX, pring->tail);
		set_64bit_val(cqe, 0, qword0);
	}

	return ret_code;
}

/**
 * zxdh_qp_round_up - return round up qp wq depth
 * @wqdepth: wq depth in quanta to round up
 */
static int zxdh_qp_round_up(u32 wqdepth)
{
	int scount = 1;

	for (wqdepth--; scount <= 16; scount *= 2)
		wqdepth |= wqdepth >> scount;

	return ++wqdepth;
}

/**
 * zxdh_get_rq_wqe_shift - get shift count for maximum rq wqe size
 * @uk_attrs: qp HW attributes
 * @sge: Maximum Scatter Gather Elements wqe
 * @shift: Returns the shift needed based on sge
 *
 * Shift can be used to left shift the rq wqe size based on number of SGEs.
 * For 1 SGE, shift = 1 (wqe size of 2*16 bytes).
 * For 2 or 3 SGEs, shift = 2 (wqe size of 4*16 bytes).
 * For 4-7 SGE's Shift of 3.
 *  For 8-15 SGE's Shift of 4 otherwise (wqe size of 512 bytes).
 */
void zxdh_get_rq_wqe_shift(struct zxdh_uk_attrs *uk_attrs, u32 sge, u8 *shift)
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
 * zxdh_get_sq_wqe_shift - get shift count for maximum wqe size
 * @uk_attrs: qp HW attributes
 * @sge: Maximum Scatter Gather Elements wqe
 * @inline_data: Maximum inline data size
 * @shift: Returns the shift needed based on sge
 *
 * Shift can be used to left shift the wqe size based on number of SGEs and inlind data size.
 * To surport WR with imm_data,shift = 1 (wqe size of 2*32 bytes).
 * For 2-7 SGEs or 24 < inline data <= 86, shift = 2 (wqe size of 4*32 bytes).
 * Otherwise (wqe size of 256 bytes).
 */
void zxdh_get_sq_wqe_shift(struct zxdh_uk_attrs *uk_attrs, u32 sge,
			   u32 inline_data, u8 *shift)
{
	*shift = 1;

	if (sge > 1 || inline_data > 24) {
		if (sge < 8 && inline_data <= 86)
			*shift = 2;
		else
			*shift = 3;
	}
}

/*
 * zxdh_get_sqdepth - get SQ depth (quanta)
 * @max_hw_wq_quanta: HW SQ size limit
 * @sq_size: SQ size
 * @shift: shift which determines size of WQE
 * @sqdepth: depth of SQ
 */
int zxdh_get_sqdepth(u32 max_hw_wq_quanta, u32 sq_size, u8 shift, u32 *sqdepth)
{
	if (sq_size > ZXDH_MAX_SQ_DEPTH) {
		pr_err("[zxdh_rdma] %s[%d]: sq_size > ZXDH_MAX_SQ_DEPTH, sq_size=%d\n", __func__, __LINE__, sq_size);
		return -EINVAL;
	}
	*sqdepth = zxdh_qp_round_up((sq_size << shift) + ZXDH_SQ_RSVD);

	if (*sqdepth < (ZXDH_QP_SW_MIN_WQSIZE << shift)) {
		*sqdepth = ZXDH_QP_SW_MIN_WQSIZE << shift;
	} else if (*sqdepth > max_hw_wq_quanta) {
		pr_err("[zxdh_rdma] %s[%d]: sqdepth > max_hw_wq_quanta, sqdepth=%d, max_hw_wq_quanta=%d\n", __func__, __LINE__, 
			*sqdepth, max_hw_wq_quanta);
		return -EINVAL;
	}

	return 0;
}

/*
 * zxdh_get_rqdepth - get RQ/SRQ depth (quanta)
 * @max_hw_rq_quanta: HW RQ/SRQ size limit
 * @rq_size: RQ/SRQ size
 * @shift: shift which determines size of WQE
 * @rqdepth: depth of RQ/SRQ
 */
int zxdh_get_rqdepth(u32 max_hw_rq_quanta, u32 rq_size, u8 shift, u32 *rqdepth)
{
	*rqdepth = zxdh_qp_round_up(rq_size + ZXDH_RQ_RSVD);

	if (*rqdepth < ZXDH_QP_SW_MIN_WQSIZE)
		*rqdepth = ZXDH_QP_SW_MIN_WQSIZE;
	else if (*rqdepth > max_hw_rq_quanta) {
		pr_err("[zxdh_rdma] %s[%d]: rqdepth > max_hw_rq_quanta, rqdepth=%d, max_hw_rq_quanta=%d\n", __func__, __LINE__, 
			*rqdepth, max_hw_rq_quanta);
		return -EINVAL;
	}

	return 0;
}

static const struct zxdh_wqe_uk_ops iw_wqe_uk_ops = {
	.iw_copy_inline_data = zxdh_copy_inline_data,
	.iw_inline_data_size_to_quanta = zxdh_inline_data_size_to_quanta,
	.iw_set_fragment = zxdh_set_fragment,
	.iw_set_mw_bind_wqe = zxdh_set_mw_bind_wqe,
};

/**
 * zxdh_uk_qp_init - initialize shared qp
 * @qp: hw qp (user and kernel)
 * @info: qp initialization info
 *
 * initializes the vars used in both user and kernel mode.
 * size of the wqe depends on numbers of max. fragements
 * allowed. Then size of wqe * the number of wqes should be the
 * amount of memory allocated for sq and rq.
 */
int zxdh_uk_qp_init(struct zxdh_qp_uk *qp, struct zxdh_qp_uk_init_info *info)
{
	int ret_code = 0;
	u32 sq_ring_size;
	u8 sqshift, rqshift;

	qp->uk_attrs = info->uk_attrs;
	if (info->max_sq_frag_cnt > qp->uk_attrs->max_hw_wq_frags ||
	    info->max_rq_frag_cnt > qp->uk_attrs->max_hw_wq_frags) {
		pr_err("[zxdh_rdma] %s[%d]: max_sq_frag_cnt or max_rq_frag_cnt is too large\n", __func__, __LINE__);
		return -EINVAL;
	}

	zxdh_get_sq_wqe_shift(qp->uk_attrs, info->max_sq_frag_cnt,
			      info->max_inline_data, &sqshift);
	zxdh_get_rq_wqe_shift(qp->uk_attrs, info->max_rq_frag_cnt, &rqshift);
	qp->qp_caps = info->qp_caps;
	qp->sq_base = info->sq;
	qp->rq_base = info->rq;
	qp->qp_type = info->type;
	qp->shadow_area = info->shadow_area;
	qp->sq_wrtrk_array = info->sq_wrtrk_array;

	qp->rq_wrid_array = info->rq_wrid_array;
	qp->wqe_alloc_db = info->wqe_alloc_db;
	qp->rd_fence_rate = info->rd_fence_rate;
	qp->qp_id = info->qp_id;
	qp->sq_size = info->sq_size;
	qp->max_sq_frag_cnt = info->max_sq_frag_cnt;
	sq_ring_size = qp->sq_size << sqshift;
	ZXDH_RING_INIT(qp->sq_ring, sq_ring_size);
	ZXDH_RING_INIT(qp->initial_ring, sq_ring_size);
	qp->swqe_polarity = 0;

	qp->swqe_polarity_deferred = 1;
	qp->rwqe_polarity = 0;
	qp->rwqe_signature = 0;
	qp->rq_size = info->rq_size;
	qp->max_rq_frag_cnt = info->max_rq_frag_cnt;
	qp->max_inline_data = (info->max_inline_data == 0) ?
				      ZXDH_MAX_INLINE_DATA_SIZE :
					    info->max_inline_data;
	qp->rq_wqe_size = rqshift;
	ZXDH_RING_INIT(qp->rq_ring, qp->rq_size);
	qp->rq_wqe_size_multiplier = 1 << rqshift;

	qp->wqe_ops = iw_wqe_uk_ops;
	return ret_code;
}

/**
 * zxdh_uk_cq_init - initialize shared cq (user and kernel)
 * @cq: hw cq
 * @info: hw cq initialization info
 */
void zxdh_uk_cq_init(struct zxdh_cq_uk *cq, struct zxdh_cq_uk_init_info *info)
{
	cq->cq_base = info->cq_base;
	cq->cq_id = info->cq_id;
	cq->cq_size = info->cq_size;
	cq->cq_log_size = info->cq_log_size;
	cq->cqe_alloc_db = info->cqe_alloc_db;
	cq->shadow_area = info->shadow_area;
	cq->cqe_size = info->cqe_size;
	ZXDH_RING_INIT(cq->cq_ring, cq->cq_size);
	cq->polarity = 1;
	cq->cqe_rd_cnt = 0;
	cq->valid_cq = true;
}

/**
 * zxdh_uk_clean_cq - clean cq entries
 * @q: completion context
 * @cq: cq to clean
 */
void zxdh_uk_clean_cq(void *q, struct zxdh_cq_uk *cq)
{
	__le64 *cqe;
	u64 qword0, comp_ctx;
	u32 cq_head;
	u8 polarity, temp;

	cq_head = cq->cq_ring.head;
	temp = cq->polarity;
	do {
		if (cq->cqe_size)
			cqe = ((struct zxdh_extended_cqe
					*)(cq->cq_base))[cq_head]
				      .buf;
		else
			cqe = cq->cq_base[cq_head].buf;
		get_64bit_val(cqe, 0, &qword0);
		polarity = (u8)FIELD_GET(ZXDH_CQ_VALID, qword0);

		if (polarity != temp)
			break;

		get_64bit_val(cqe, 8, &comp_ctx);
		if ((void *)(unsigned long)comp_ctx == q)
			set_64bit_val(cqe, 8, 0);

		cq_head = (cq_head + 1) % cq->cq_ring.size;
		if (!cq_head)
			temp ^= 1;
	} while (true);
}

/**
 * zxdh_nop - post a nop
 * @qp: hw qp ptr
 * @wr_id: work request id
 * @signaled: signaled for completion
 * @post_sq: ring doorbell
 */
int zxdh_nop(struct zxdh_qp_uk *qp, u64 wr_id, bool signaled, bool post_sq)
{
	__le64 *wqe;
	u64 hdr;
	u32 wqe_idx;
	struct zxdh_post_sq_info info = {};

	info.push_wqe = false;
	info.wr_id = wr_id;
	wqe = zxdh_qp_get_next_send_wqe(qp, &wqe_idx, ZXDH_QP_WQE_MIN_QUANTA, 0,
					&info);
	if (!wqe)
		return -ENOSPC;

	zxdh_clr_wqes(qp, wqe_idx);

	set_64bit_val(wqe, 0, 0);
	set_64bit_val(wqe, 8, 0);
	set_64bit_val(wqe, 16, 0);

	hdr = FIELD_PREP(IRDMAQPSQ_OPCODE, ZXDH_OP_TYPE_NOP) |
	      FIELD_PREP(IRDMAQPSQ_SIGCOMPL, signaled) |
	      FIELD_PREP(IRDMAQPSQ_VALID, qp->swqe_polarity);

	dma_wmb(); /* make sure WQE is populated before valid bit is set */

	set_64bit_val(wqe, 24, hdr);
	if (post_sq)
		zxdh_uk_qp_post_wr(qp);

	return 0;
}

/**
 * zxdh_fragcnt_to_quanta_sq - calculate quanta based on fragment count for SQ
 * @frag_cnt: number of fragments
 * @quanta: quanta for frag_cnt
 */
int zxdh_fragcnt_to_quanta_sq(u32 frag_cnt, u16 *quanta)
{
	if (frag_cnt > ZXDH_MAX_SQ_FRAG)
		return -EINVAL;
	*quanta = frag_cnt / 2 + 1;
	return 0;
}

/**
 * zxdh_fragcnt_to_wqesize_rq - calculate wqe size based on fragment count for RQ
 * @frag_cnt: number of fragments
 * @wqe_size: size in bytes given frag_cnt
 */
int zxdh_fragcnt_to_wqesize_rq(u32 frag_cnt, u16 *wqe_size)
{
	if (frag_cnt < 2)
		*wqe_size = 32;
	else if (frag_cnt < 4)
		*wqe_size = 64;
	else if (frag_cnt < 8)
		*wqe_size = 128;
	else if (frag_cnt < 16)
		*wqe_size = 256;
	else if (frag_cnt < 32)
		*wqe_size = 512;
	else
		return -EINVAL;

	return 0;
}

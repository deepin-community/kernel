// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium USB DRD Driver.
 *
 * Copyright (C) 2023 - 2024 Phytium.
 */
#include <linux/dma-mapping.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/list.h>
#include "gadget.h"
#include "ring.h"

#define DB_VALUE(ep, stream)	((((ep) + 1) & 0xff) | ((stream) << 16))
#define DB_VALUE_EP0_OUT(ep, stream) ((ep) & 0xff)
#define DB_VALUE_CMD 0x0

#define TRB_BSR	BIT(9)
#define TRB_DC	BIT(9)

#define TRB_SETUPID_BITMASK		GENMASK(9, 8)
#define TRB_SETUPID(p)			((p) << 8)
#define TRB_SETUPID_TO_TYPE(p)	(((p) & TRB_SETUPID_BITMASK) >> 8)

#define TRB_SETUP_SPEED_USB3	0x1
#define TRB_SETUP_SPEED_USB2	0x1

#define TRB_FH_TO_PACKET_TYPE(p)	((p) & GENMASK(4, 0))
#define TRB_FH_TO_DEVICE_ADDRESS(p)	(((p) << 25) & GENMASK(31, 25))
#define TRB_FH_TO_NOT_TYPE(p)		(((p) << 4) & GENMASK(7, 4))
#define TRB_FH_TO_INTERFACE(p)		(((p) << 8) & GENMASK(15, 8))
#define TRB_FH_TR_PACKET	0x4
#define TRB_FH_TR_PACKET_DEV_NOT 0x6
#define TRB_FH_TR_PACKET_FUNCTION_WAKE 0x1

#define TRB_SETUPSTAT_STALL	0x0
#define TRB_SETUPSTAT_ACK	0x1
#define TRB_SETUPSTAT(p)	((p) << 6)

#define TRB_SIA				BIT(31)

#define TRB_SETUP_SPEEDID(p)	((p) & (1 << 7))
#define GET_PORT_ID(p)		(((p) & GENMASK(31, 24)) >> 24)
#define SET_PORT_ID(p)		(((p) << 24) & GENMASK(31, 24))

#define EVENT_TRB_LEN(p)	((p) & GENMASK(23, 0))

#define TRB_TO_DEV_STREAM(p)	((p) & GENMASK(16, 0))
#define TRB_TO_HOST_STREAM(p)	((p) & GENMASK(16, 0))
#define STREAM_PRIME_ACK		0xFFFE
#define STREAM_REJECTED			0xFFFF

dma_addr_t gadget_trb_virt_to_dma(struct gadget_segment *seg,
		union gadget_trb *trb)
{
	unsigned long segment_offset = trb - seg->trbs;

	if (trb < seg->trbs || segment_offset >= TRBS_PER_SEGMENT)
		return 0;

	return seg->dma + (segment_offset * sizeof(*trb));
}

static bool gadget_room_on_ring(struct phytium_device *pdev,
			struct gadget_ring *ring, unsigned int num_trbs)
{
	int num_trbs_in_deq_seg;

	if (ring->num_trbs_free < num_trbs)
		return false;

	if (ring->type != TYPE_COMMAND && ring->type != TYPE_EVENT) {
		num_trbs_in_deq_seg = ring->dequeue - ring->deq_seg->trbs;

		if (ring->num_trbs_free < num_trbs + num_trbs_in_deq_seg)
			return false;
	}

	return true;
}

static bool gadget_trb_is_noop(union gadget_trb *trb)
{
	return TRB_TYPE_NOOP_LE32(trb->generic.field[3]);
}

static bool gadget_trb_is_link(union gadget_trb *trb)
{
	return TRB_TYPE_LINK_LE32(trb->link.control);
}

static bool gadget_link_trb_toggles_cycle(union gadget_trb *trb)
{
	return le32_to_cpu(trb->link.control) & LINK_TOGGLE;
}

static int gadget_prepare_ring(struct phytium_device *pdev,
		struct gadget_ring *ep_ring, u32 ep_state, unsigned int num_trbs,
		gfp_t mem_flags)
{
	unsigned int num_trbs_needed;

	switch (ep_state) {
	case EP_STATE_STOPPED:
	case EP_STATE_RUNNING:
	case EP_STATE_HALTED:
		break;
	default:
		dev_err(pdev->dev, "Error: incorrect endpoint state\n");
		return -EINVAL;
	}

	while (1) {
		if (gadget_room_on_ring(pdev, ep_ring, num_trbs))
			break;

		num_trbs_needed = num_trbs - ep_ring->num_trbs_free;
		if (gadget_ring_expansion(pdev, ep_ring, num_trbs_needed, mem_flags)) {
			dev_err(pdev->dev, "Ring expansion failed\n");
			return -ENOMEM;
		}
	}

	while (gadget_trb_is_link(ep_ring->enqueue)) {
		ep_ring->enqueue->link.control |= cpu_to_le32(TRB_CHAIN);
		/* The cycle bit must be set as the last operation. */
		wmb();
		ep_ring->enqueue->link.control ^= cpu_to_le32(TRB_CYCLE);

		if (gadget_link_trb_toggles_cycle(ep_ring->enqueue))
			ep_ring->cycle_state ^= 1;

		ep_ring->enq_seg = ep_ring->enq_seg->next;
		ep_ring->enqueue = ep_ring->enq_seg->trbs;
	}

	return 0;
}

static void gadget_inc_enq(struct phytium_device *pdev, struct gadget_ring *ring,
			bool more_trbs_coming)
{
	union gadget_trb *next;
	u32 chain;

	chain = le32_to_cpu(ring->enqueue->generic.field[3]) & TRB_CHAIN;

	if (!gadget_trb_is_link(ring->enqueue))
		ring->num_trbs_free--;
	next = ++(ring->enqueue);

	while (gadget_trb_is_link(next)) {
		if (!chain && !more_trbs_coming)
			break;

		next->link.control &= cpu_to_le32(~TRB_CHAIN);
		next->link.control |= cpu_to_le32(chain);

		/* Give this link TRB to the hardware */
		wmb();
		next->link.control ^= cpu_to_le32(TRB_CYCLE);

		if (gadget_link_trb_toggles_cycle(next))
			ring->cycle_state ^= 1;

		ring->enq_seg = ring->enq_seg->next;
		ring->enqueue = ring->enq_seg->trbs;
		next = ring->enqueue;
	}
}

static void gadget_queue_trb(struct phytium_device *pdev, struct gadget_ring *ring,
	bool more_trbs_coming, u32 field1, u32 field2, u32 field3, u32 field4)
{
	struct gadget_generic_trb *trb;

	trb = &ring->enqueue->generic;

	trb->field[0] = cpu_to_le32(field1);
	trb->field[1] = cpu_to_le32(field2);
	trb->field[2] = cpu_to_le32(field3);
	trb->field[3] = cpu_to_le32(field4);

	gadget_inc_enq(pdev, ring, more_trbs_coming);
}

static void gadget_queue_command(struct phytium_device *pdev,
				u32 field1, u32 field2, u32 field3, u32 field4)
{
	gadget_prepare_ring(pdev, pdev->cmd_ring, EP_STATE_RUNNING, 1, GFP_ATOMIC);

	pdev->cmd.command_trb = pdev->cmd_ring->enqueue;

	gadget_queue_trb(pdev, pdev->cmd_ring, false, field1, field2, field3,
		field4 | pdev->cmd_ring->cycle_state);
}

void gadget_queue_configure_endpoint(struct phytium_device *pdev,
		dma_addr_t in_ctx_ptr)
{
	gadget_queue_command(pdev, lower_32_bits(in_ctx_ptr),
			upper_32_bits(in_ctx_ptr), 0,
			TRB_TYPE(TRB_CONFIG_EP) |
			SLOT_ID_FOR_TRB(pdev->slot_id));
}

void gadget_ring_cmd_db(struct phytium_device *pdev)
{
	writel(DB_VALUE_CMD, &pdev->dba->cmd_db);
}

void gadget_queue_stop_endpoint(struct phytium_device *pdev, unsigned int ep_indx)
{
	gadget_queue_command(pdev, 0, 0, 0, SLOT_ID_FOR_TRB(pdev->slot_id) |
		EP_ID_FOR_TRB(ep_indx) | TRB_TYPE(TRB_STOP_RING));
}

int gadget_cmd_stop_ep(struct phytium_device *pdev, struct gadget_ep *pep)
{
	u32 ep_state = GET_EP_CTX_STATE(pep->out_ctx);
	int ret = 0;

	if (ep_state == EP_STATE_STOPPED || ep_state == EP_STATE_DISABLED ||
		ep_state == EP_STATE_HALTED)
		goto ep_stopped;

	gadget_queue_stop_endpoint(pdev, pep->idx);
	gadget_ring_cmd_db(pdev);
	ret = gadget_wait_for_cmd_compl(pdev);

ep_stopped:
	pep->ep_state |= EP_STOPPED;

	return ret;
}

void gadget_queue_flush_endpoint(struct phytium_device *pdev, unsigned int ep_index)
{
	gadget_queue_command(pdev, 0, 0, 0, TRB_TYPE(TRB_FLUSH_ENDPOINT) |
			SLOT_ID_FOR_TRB(pdev->slot_id) | EP_ID_FOR_TRB(ep_index));
}

int gadget_cmd_flush_ep(struct phytium_device *pdev, struct gadget_ep *pep)
{
	int ret;

	gadget_queue_flush_endpoint(pdev, pep->idx);
	gadget_ring_cmd_db(pdev);
	ret = gadget_wait_for_cmd_compl(pdev);

	return ret;
}

static struct gadget_ring *gadget_get_transfer_ring(struct phytium_device *pdev,
							struct gadget_ep *pep,
							unsigned int stream_id)
{
	if (!(pep->ep_state & EP_HAS_STREAMS))
		return pep->ring;

	if (stream_id == 0 || stream_id >= pep->stream_info.num_streams) {
		dev_err(pdev->dev, "Err:%s ring doesn't exist for SID:%d\n", pep->name, stream_id);
		return NULL;
	}

	return pep->stream_info.stream_rings[stream_id];
}

static struct gadget_ring *
	gadget_request_to_transfer_ring(struct phytium_device *pdev,
			struct gadget_request *preq)
{
	return gadget_get_transfer_ring(pdev, preq->pep, preq->request.stream_id);
}

static u64 gadget_get_hw_deq(struct phytium_device *pdev, unsigned int ep_index,
			unsigned int stream_id)
{
	struct gadget_stream_ctx *st_ctx;
	struct gadget_ep *pep;

	pep = &pdev->eps[stream_id];

	if (pep->ep_state & EP_HAS_STREAMS) {
		st_ctx = &pep->stream_info.stream_ctx_array[stream_id];
		return le64_to_cpu(st_ctx->stream_ring);
	}

	return le64_to_cpu(pep->out_ctx->deq);
}

static struct gadget_segment *gadget_trb_in_td(struct phytium_device *pdev,
				struct gadget_segment *start_seg, union gadget_trb *start_trb,
				union gadget_trb *end_trb, dma_addr_t suspect_dma)
{
	struct gadget_segment *cur_seg;
	union gadget_trb *temp_trb;
	dma_addr_t end_seg_dma;
	dma_addr_t end_trb_dma;
	dma_addr_t start_dma;

	start_dma = gadget_trb_virt_to_dma(start_seg, start_trb);
	cur_seg = start_seg;

	do {
		if (start_dma == 0)
			return NULL;

		temp_trb = &cur_seg->trbs[TRBS_PER_SEGMENT - 1];
		end_seg_dma = gadget_trb_virt_to_dma(cur_seg, temp_trb);
		end_trb_dma = gadget_trb_virt_to_dma(cur_seg, end_trb);

		if (end_trb_dma > 0) {
			if (start_dma <= end_trb_dma) {
				if (suspect_dma >= start_dma && suspect_dma <= end_trb_dma)
					return cur_seg;
			} else {
				if ((suspect_dma >= start_dma && suspect_dma <= end_seg_dma) ||
					(suspect_dma >= cur_seg->dma && suspect_dma <= end_trb_dma))
					return cur_seg;
			}

			return NULL;
		}
		if (suspect_dma >= start_dma && suspect_dma <= end_seg_dma)
			return cur_seg;

		cur_seg = cur_seg->next;
		start_dma = gadget_trb_virt_to_dma(cur_seg, &cur_seg->trbs[0]);
	} while (cur_seg != start_seg);

	return NULL;
}

static void gadget_next_trb(struct phytium_device *pdev, struct gadget_ring *ring,
			struct gadget_segment **seg, union gadget_trb **trb)
{
	if (gadget_trb_is_link(*trb)) {
		*seg = (*seg)->next;
		*trb = ((*seg)->trbs);
	} else {
		(*trb)++;
	}
}

static void gadget_find_new_dequeue_state(struct phytium_device *pdev,
			struct gadget_ep *pep, unsigned int stream_id, struct gadget_td *cur_td,
			struct gadget_dequeue_state *state)
{
	bool td_last_trb_found = false;
	struct gadget_segment *new_seg;
	struct gadget_ring *ep_ring;
	union gadget_trb *new_deq;
	bool cycle_found = false;
	u64 hw_dequeue;

	ep_ring = gadget_get_transfer_ring(pdev, pep, stream_id);
	if (!ep_ring)
		return;

	hw_dequeue = gadget_get_hw_deq(pdev, pep->idx, stream_id);
	new_seg = ep_ring->deq_seg;
	new_deq = ep_ring->dequeue;
	state->new_cycle_state = hw_dequeue & 0x1;
	state->stream_id = stream_id;

	do {
		if (!cycle_found && gadget_trb_virt_to_dma(new_seg, new_deq) ==
			(dma_addr_t)(hw_dequeue & ~0xf)) {
			cycle_found = true;
			if (td_last_trb_found)
				break;
		}

		if (new_deq == cur_td->last_trb)
			td_last_trb_found = true;

		if (cycle_found && gadget_trb_is_link(new_deq) &&
				gadget_link_trb_toggles_cycle(new_deq))
			state->new_cycle_state ^= 0x1;

		gadget_next_trb(pdev, ep_ring, &new_seg, &new_deq);

		if (new_deq == pep->ring->dequeue) {
			dev_err(pdev->dev, "Error: Failed finding new deuque state\n");
			state->new_deq_seg = NULL;
			state->new_deq_ptr = NULL;
			return;
		}
	} while (!cycle_found || !td_last_trb_found);

	state->new_deq_seg = new_seg;
	state->new_deq_ptr = new_deq;

}

static void gadget_trb_to_noop(union gadget_trb *trb, u32 noop_type)
{
	if (gadget_trb_is_link(trb)) {
		trb->link.control &= cpu_to_le32(~TRB_CHAIN);
	} else {
		trb->generic.field[0] = 0;
		trb->generic.field[1] = 0;
		trb->generic.field[2] = 0;
		trb->generic.field[3] &= cpu_to_le32(TRB_CYCLE);
		trb->generic.field[3] |= cpu_to_le32(TRB_TYPE(noop_type));
	}
}

static void gadget_td_to_noop(struct phytium_device *pdev,
			struct gadget_ring *ep_ring, struct gadget_td *td, bool flip_cycle)
{
	struct gadget_segment *seg = td->start_seg;
	union gadget_trb *trb = td->first_trb;

	while (1) {
		gadget_trb_to_noop(trb, TRB_TR_NOOP);

		if (flip_cycle && trb != td->first_trb && trb != td->last_trb)
			trb->generic.field[3] ^= cpu_to_le32(TRB_CYCLE);

		if (trb == td->last_trb)
			break;

		gadget_next_trb(pdev, ep_ring, &seg, &trb);
	}
}

void gadget_set_link_state(struct phytium_device *pdev,
		__le32 __iomem *port_regs, u32 link_state)
{
	int port_num = 0xff;
	u32 temp;

	temp = readl(port_regs);
	temp = gadget_port_state_to_neutral(temp);
	temp |= PORT_WKCONN_E | PORT_WKDISC_E;
	writel(temp, port_regs);

	temp &= ~PORT_PLS_MASK;
	temp |= PORT_LINK_STROBE | link_state;

	if (pdev->active_port)
		port_num = pdev->active_port->port_num;

	writel(temp, port_regs);
}

static void gadget_force_10_go(struct phytium_device *pdev)
{
	if (pdev->active_port == &pdev->usb2_port && pdev->gadget.lpm_capable)
		gadget_set_link_state(pdev, &pdev->active_port->regs->portsc, XDEV_U0);
}

static bool gadget_ring_ep_doorbell(struct phytium_device *pdev,
			struct gadget_ep *pep, unsigned int stream_id)
{
	__le32 __iomem *reg_addr = &pdev->dba->ep_db;
	unsigned int ep_state = pep->ep_state;
	unsigned int db_value;

	if (ep_state & EP_HALTED || !(ep_state & EP_ENABLED))
		return false;

	if (pep->ep_state & EP_HAS_STREAMS) {
		if (pep->stream_info.drbls_count >= 2)
			return false;

		pep->stream_info.drbls_count++;
	}

	pep->ep_state &= ~EP_STOPPED;

	if (pep->idx == 0 && pdev->ep0_stage == GADGET_DATA_STAGE &&
			!pdev->ep0_expect_in)
		db_value = DB_VALUE_EP0_OUT(pep->idx, stream_id);
	else
		db_value = DB_VALUE(pep->idx, stream_id);

	writel(db_value, reg_addr);

	gadget_force_10_go(pdev);

	return true;
}

void gadget_ring_doorbell_for_active_rings(struct phytium_device *pdev,
		struct gadget_ep *pep)
{
	struct gadget_stream_info *stream_info;
	unsigned int stream_id;
	int ret;

	if (pep->ep_state & EP_DIS_IN_PROGRESS)
		return;

	if (!(pep->ep_state & EP_HAS_STREAMS) && pep->number) {
		if (pep->ring && !list_empty(&pep->ring->td_list))
			gadget_ring_ep_doorbell(pdev, pep, 0);

		return;
	}

	stream_info = &pep->stream_info;
	for (stream_id = 1; stream_id < stream_info->num_streams; stream_id++) {
		struct gadget_td *td, *td_temp;
		struct gadget_ring *ep_ring;

		if (stream_info->drbls_count >= 2)
			return;

		ep_ring = gadget_get_transfer_ring(pdev, pep, stream_id);
		if (!ep_ring)
			continue;

		if (!ep_ring->stream_active || ep_ring->stream_rejected)
			continue;

		list_for_each_entry_safe(td, td_temp, &ep_ring->td_list, td_list) {
			if (td->drbl)
				continue;

			ret = gadget_ring_ep_doorbell(pdev, pep, stream_id);
			if (ret)
				td->drbl = 1;
		}
	}
}

void gadget_queue_new_dequeue_state(struct phytium_device *pdev,
	struct gadget_ep *pep, struct gadget_dequeue_state *deq_state)
{
	u32 trb_stream_id = STREAM_ID_FOR_TRB(deq_state->stream_id);
	u32 trb_slot_id = SLOT_ID_FOR_TRB(pdev->slot_id);
	u32 type = TRB_TYPE(TRB_SET_DEQ);
	u32 trb_sct = 0;
	dma_addr_t addr;

	addr = gadget_trb_virt_to_dma(deq_state->new_deq_seg, deq_state->new_deq_ptr);

	if (deq_state->stream_id)
		trb_sct = SCT_FOR_TRB(SCT_PRI_TR);

	gadget_queue_command(pdev, lower_32_bits(addr) | trb_sct |
			deq_state->new_cycle_state, upper_32_bits(addr), trb_stream_id,
			trb_slot_id | EP_ID_FOR_TRB(pep->idx) | type);
}

static int gadget_cmd_set_deq(struct phytium_device *pdev,
			struct gadget_ep *pep, struct gadget_dequeue_state *deq_state)
{
	struct gadget_ring *ep_ring;
	int ret;

	if (!deq_state->new_deq_ptr || !deq_state->new_deq_seg) {
		gadget_ring_doorbell_for_active_rings(pdev, pep);

		return 0;
	}

	gadget_queue_new_dequeue_state(pdev, pep, deq_state);
	gadget_ring_cmd_db(pdev);
	ret = gadget_wait_for_cmd_compl(pdev);

	ep_ring = gadget_get_transfer_ring(pdev, pep, deq_state->stream_id);

	if (gadget_trb_is_link(ep_ring->dequeue)) {
		ep_ring->deq_seg = ep_ring->deq_seg->next;
		ep_ring->dequeue = ep_ring->deq_seg->trbs;
	}

	while (ep_ring->dequeue != deq_state->new_deq_ptr) {
		ep_ring->num_trbs_free++;
		ep_ring->dequeue++;

		if (gadget_trb_is_link(ep_ring->dequeue)) {
			if (ep_ring->dequeue == deq_state->new_deq_ptr)
				break;

			ep_ring->deq_seg = ep_ring->deq_seg->next;
			ep_ring->dequeue = ep_ring->deq_seg->trbs;
		}
	}

	if (ret)
		return -ESHUTDOWN;

	gadget_ring_doorbell_for_active_rings(pdev, pep);

	return 0;
}

static void gadget_unmap_td_bounce_buffer(struct phytium_device *pdev,
			struct gadget_ring *ring, struct gadget_td *td)
{
	struct gadget_segment *seg = td->bounce_seg;
	struct gadget_request *preq;
	size_t len;

	if (!seg)
		return;

	preq = td->preq;

	if (!preq->direction) {
		dma_unmap_single(pdev->dev, seg->bounce_dma, ring->bounce_buf_len, DMA_TO_DEVICE);
		return;
	}

	dma_unmap_single(pdev->dev, seg->bounce_dma, ring->bounce_buf_len, DMA_FROM_DEVICE);

	len = sg_pcopy_from_buffer(preq->request.sg, preq->request.num_sgs,
		seg->bounce_buf, seg->bounce_len, seg->bounce_offs);
	if (len != seg->bounce_len)
		dev_warn(pdev->dev, "WARN Wrong bounce buffer read length:%ld != %d\n",
				len, seg->bounce_len);

	seg->bounce_len = 0;
	seg->bounce_offs = 0;
}

static void gadget_giveback(struct gadget_ep *pep, struct gadget_request *preq, int status)
{
	struct phytium_device *pdev = pep->pdev;

	list_del(&preq->list);

	if (preq->request.status == -EINPROGRESS)
		preq->request.status = status;

	usb_gadget_unmap_request_by_dev(pdev->dev, &preq->request, preq->direction);

	if (preq != &pdev->ep0_preq) {
		spin_unlock(&pdev->lock);
		usb_gadget_giveback_request(&pep->endpoint, &preq->request);
		spin_lock(&pdev->lock);
	}
}

int gadget_remove_request(struct phytium_device *pdev, struct gadget_request *preq,
			struct gadget_ep *pep)
{
	struct gadget_dequeue_state deq_state;
	struct gadget_td *cur_td = NULL;
	struct gadget_ring *ep_ring;
	struct gadget_segment *seg;
	int status = -ECONNRESET;
	int ret = 0;
	u64 hw_deq;

	memset(&deq_state, 0, sizeof(deq_state));

	cur_td = &preq->td;
	ep_ring = gadget_request_to_transfer_ring(pdev, preq);

	hw_deq = gadget_get_hw_deq(pdev, pep->idx, preq->request.stream_id);
	hw_deq &= ~0xf;

	seg = gadget_trb_in_td(pdev, cur_td->start_seg, cur_td->first_trb,
					cur_td->last_trb, hw_deq);
	if (seg && (pep->ep_state & EP_ENABLED))
		gadget_find_new_dequeue_state(pdev, pep, preq->request.stream_id,
					cur_td, &deq_state);
	else
		gadget_td_to_noop(pdev, ep_ring, cur_td, false);

	list_del_init(&cur_td->td_list);
	ep_ring->num_tds--;
	pep->stream_info.td_count--;

	if (pdev->gadget_state & GADGET_STATE_DISCONNECT_PENDING) {
		status = -ESHUTDOWN;
		ret = gadget_cmd_set_deq(pdev, pep, &deq_state);
	}

	gadget_unmap_td_bounce_buffer(pdev, ep_ring, cur_td);
	gadget_giveback(pep, cur_td->preq, status);

	return ret;
}

void gadget_queue_slot_control(struct phytium_device *pdev, u32 trb_type)
{
	gadget_queue_command(pdev, 0, 0, 0, TRB_TYPE(trb_type) |
		SLOT_ID_FOR_TRB(pdev->slot_id));
}

void gadget_inc_deq(struct phytium_device *pdev, struct gadget_ring *ring)
{
	if (ring->type == TYPE_EVENT) {
		if (!gadget_last_trb_on_seg(ring->deq_seg, ring->dequeue)) {
			ring->dequeue++;
			return;
		}

		if (gadget_last_trb_on_ring(ring, ring->deq_seg, ring->dequeue))
			ring->cycle_state ^= 1;

		ring->deq_seg = ring->deq_seg->next;
		ring->dequeue = ring->deq_seg->trbs;
		return;
	}

	if (!gadget_trb_is_link(ring->dequeue)) {
		ring->dequeue++;
		ring->num_trbs_free++;
	}

	while (gadget_trb_is_link(ring->dequeue)) {
		ring->deq_seg = ring->deq_seg->next;
		ring->dequeue = ring->deq_seg->trbs;
	}
}

void gadget_queue_reset_ep(struct phytium_device *pdev, unsigned int ep_index)
{
	return gadget_queue_command(pdev, 0, 0, 0, SLOT_ID_FOR_TRB(pdev->slot_id) |
		EP_ID_FOR_TRB(ep_index) | TRB_TYPE(TRB_RESET_EP));
}

void gadget_queue_halt_endpoint(struct phytium_device *pdev, unsigned int ep_index)
{
	return gadget_queue_command(pdev, 0, 0, 0, TRB_TYPE(TRB_HALT_ENDPOINT) |
		SLOT_ID_FOR_TRB(pdev->slot_id) | EP_ID_FOR_TRB(ep_index));
}

void gadget_queue_reset_device(struct phytium_device *pdev)
{
	gadget_queue_command(pdev, 0, 0, 0, TRB_TYPE(TRB_RESET_DEV) |
			SLOT_ID_FOR_TRB(pdev->slot_id));
}

static int gadget_update_port_id(struct phytium_device *pdev, u32 port_id)
{
	struct gadget_port *port = pdev->active_port;
	u8 old_port = 0;

	if (port && port->port_num == port_id)
		return 0;

	if (port)
		old_port = port->port_num;

	if (port_id == pdev->usb2_port.port_num)
		port = &pdev->usb2_port;
	else if (port_id == pdev->usb3_port.port_num)
		port = &pdev->usb3_port;
	else {
		dev_err(pdev->dev, "Port event with invalid port ID %d\n", port_id);
		return -EINVAL;
	}

	if (port_id != old_port) {
		gadget_disable_slot(pdev);
		pdev->active_port = port;
		gadget_enable_slot(pdev);
	}

	if (port_id == pdev->usb2_port.port_num)
		gadget_set_usb2_hardware_lpm(pdev, NULL, 1);
	else
		writel(PORT_U1_TIMEOUT(1) | PORT_U2_TIMEOUT(1), &pdev->usb3_port.regs->portpmsc);

	return 0;
}

void gadget_force_header_wakeup(struct phytium_device *pdev, int intf_num)
{
	u32 lo, mid;

	lo = TRB_FH_TO_PACKET_TYPE(TRB_FH_TR_PACKET) |
			TRB_FH_TO_DEVICE_ADDRESS(pdev->device_address);

	mid = TRB_FH_TR_PACKET_DEV_NOT | TRB_FH_TO_NOT_TYPE(TRB_FH_TR_PACKET_FUNCTION_WAKE) |
			TRB_FH_TO_INTERFACE(intf_num);

	gadget_queue_command(pdev, lo, mid, 0, TRB_TYPE(TRB_FORCE_HEADER) | SET_PORT_ID(2));
}

static void gadget_handle_port_status(struct phytium_device *pdev, union gadget_trb *event)
{
	struct gadget_port_regs __iomem *port_regs;
	u32 portsc, cmd_regs, link_state, port_id;
	bool port2 = false;

	if (GET_COMP_CODE(le32_to_cpu(event->generic.field[2])) != COMP_SUCCESS)
		dev_err(pdev->dev, "Err: incorrect PSC event\n");

	port_id = GET_PORT_ID(le32_to_cpu(event->generic.field[0]));

	if (gadget_update_port_id(pdev, port_id))
		goto cleanup;

	port_regs = pdev->active_port->regs;

	if (port_id == pdev->usb2_port.port_num)
		port2 = true;

new_event:
	portsc = readl(&port_regs->portsc);
	writel(gadget_port_state_to_neutral(portsc) |
		(portsc & PORT_CHANGE_BITS), &port_regs->portsc);

	pdev->gadget.speed = gadget_port_speed(portsc);
	link_state = portsc & PORT_PLS_MASK;

	if (portsc & PORT_PLC) {
		if (!(pdev->gadget_state & GADGET_WAKEUP_PENDING) && link_state == XDEV_RESUME) {
			cmd_regs = readl(&pdev->op_regs->command);
			if (!(cmd_regs & CMD_R_S))
				goto cleanup;

			if (DEV_SUPERSPEED_ANY(portsc)) {
				gadget_set_link_state(pdev, &port_regs->portsc, XDEV_U0);
				resume_gadget(pdev);
			}
		}

		if ((pdev->gadget_state & GADGET_WAKEUP_PENDING) && link_state == XDEV_U0) {
			pdev->gadget_state &= ~GADGET_WAKEUP_PENDING;
			gadget_force_header_wakeup(pdev, 1);
			gadget_ring_cmd_db(pdev);
			gadget_wait_for_cmd_compl(pdev);
		}

		if (link_state == XDEV_U0 && pdev->link_state == XDEV_U3 &&
				!DEV_SUPERSPEED_ANY(portsc))
			resume_gadget(pdev);

		if (link_state == XDEV_U3 && pdev->link_state != XDEV_U3)
			suspend_gadget(pdev);

		pdev->link_state = link_state;
	}

	if (portsc & PORT_CSC) {
		if (pdev->gadget.connected && !(portsc & PORT_CONNECT))
			disconnect_gadget(pdev);

		if (portsc & PORT_CONNECT) {
			if (!port2)
				gadget_irq_reset(pdev);

			usb_gadget_set_state(&pdev->gadget, USB_STATE_ATTACHED);
		}
	}

	if ((portsc & (PORT_RC | PORT_WRC)) && (portsc & PORT_CONNECT)) {
		gadget_irq_reset(pdev);
		pdev->u1_allowed = 0;
		pdev->u2_allowed = 0;
		pdev->may_wakeup = 0;
	}

	if (portsc & PORT_CEC)
		dev_err(pdev->dev, "Port Over Current detected\n");

	if (portsc & PORT_CEC)
		dev_err(pdev->dev, "Port Configure Error detected\n");

	if (readl(&port_regs->portsc) & PORT_CHANGE_BITS)
		goto new_event;

cleanup:
	gadget_inc_deq(pdev, pdev->event_ring);
}

static void gadget_td_cleanup(struct phytium_device *pdev, struct gadget_td *td,
				struct gadget_ring *ep_ring, int *status)
{
	struct gadget_request *preq = td->preq;

	gadget_unmap_td_bounce_buffer(pdev, ep_ring, td);

	if (preq->request.actual > preq->request.length) {
		preq->request.actual = 0;
		*status = 0;
	}

	list_del_init(&td->td_list);
	ep_ring->num_tds--;
	preq->pep->stream_info.td_count--;

	gadget_giveback(preq->pep, preq, *status);
}

static void gadget_skip_isoc_td(struct phytium_device *pdev, struct gadget_td *td,
				struct gadget_transfer_event *event,
				struct gadget_ep *pep, int status)
{
	struct gadget_ring *ep_ring;

	ep_ring = gadget_dma_to_transfer_ring(pep, le64_to_cpu(event->buffer));
	td->preq->request.status = -EXDEV;
	td->preq->request.actual = 0;

	while (ep_ring->dequeue != td->last_trb)
		gadget_inc_deq(pdev, ep_ring);

	gadget_inc_deq(pdev, ep_ring);

	gadget_td_cleanup(pdev, td, ep_ring, &status);
}

static int gadget_giveback_first_trb(struct phytium_device *pdev, struct gadget_ep *pep,
			unsigned int stream_id, int start_cycle,
			struct gadget_generic_trb *start_trb)
{
	/*
	 * Pass all the TRBs to the hardware at once and make sure this write
	 * isn't reordered.
	 */
	wmb();

	if (start_cycle)
		start_trb->field[3] |= cpu_to_le32(start_cycle);
	else
		start_trb->field[3] &= cpu_to_le32(~TRB_CYCLE);

	if ((pep->ep_state & EP_HAS_STREAMS) && !pep->stream_info.first_prime_det)
		return 0;

	return gadget_ring_ep_doorbell(pdev, pep, stream_id);
}

static void gadget_finish_td(struct phytium_device *pdev, struct gadget_td *td,
			struct gadget_transfer_event *event, struct gadget_ep *ep, int *status)
{
	struct gadget_ring *ep_ring;
	u32 trb_comp_code;

	ep_ring = gadget_dma_to_transfer_ring(ep, le64_to_cpu(event->buffer));
	trb_comp_code = GET_COMP_CODE(le32_to_cpu(event->transfer_len));

	if (trb_comp_code == COMP_STOPPED_LENGTH_INVALID ||
		trb_comp_code == COMP_STOPPED_SHORT_PACKET || trb_comp_code == COMP_STOPPED)
		return;

	while (ep_ring->dequeue != td->last_trb)
		gadget_inc_deq(pdev, ep_ring);

	gadget_inc_deq(pdev, ep_ring);

	gadget_td_cleanup(pdev, td, ep_ring, status);
}

static void gadget_process_ctrl_td(struct phytium_device *pdev, struct gadget_td *td,
				union gadget_trb *event_trb, struct gadget_transfer_event *event,
				struct gadget_ep *pep, int *status)
{
	struct gadget_ring *ep_ring;
	u32 remaining;
	u32 trb_type;

	trb_type = TRB_FIELD_TO_TYPE(le32_to_cpu(event_trb->generic.field[3]));
	ep_ring = gadget_dma_to_transfer_ring(pep, le64_to_cpu(event->buffer));
	remaining = EVENT_TRB_LEN(le32_to_cpu(event->transfer_len));

	if (trb_type == TRB_DATA) {
		td->request_length_set = true;
		td->preq->request.actual = td->preq->request.length - remaining;
	}

	if (!td->request_length_set)
		td->preq->request.actual = td->preq->request.length;

	if (pdev->ep0_stage == GADGET_DATA_STAGE && pep->number == 0 && pdev->three_stage_setup) {
		td = list_entry(ep_ring->td_list.next, struct gadget_td, td_list);
		pdev->ep0_stage = GADGET_STATUS_STAGE;

		gadget_giveback_first_trb(pdev, pep, 0, ep_ring->cycle_state,
				&td->last_trb->generic);

		return;
	}

	*status = 0;

	gadget_finish_td(pdev, td, event, pep, status);
}

static int gadget_sum_trb_lengths(struct phytium_device *pdev,
			struct gadget_ring *ring, union gadget_trb *stop_trb)
{
	struct gadget_segment *seg = ring->deq_seg;
	union gadget_trb *trb = ring->dequeue;
	u32 sum;

	for (sum = 0; trb != stop_trb; gadget_next_trb(pdev, ring, &seg, &trb)) {
		if (!gadget_trb_is_noop(trb) && !gadget_trb_is_link(trb))
			sum += TRB_LEN(le32_to_cpu(trb->generic.field[2]));
	}

	return sum;
}

static void gadget_process_isoc_td(struct phytium_device *pdev,
				struct gadget_td *td, union gadget_trb *ep_trb,
				struct gadget_transfer_event *event,
				struct gadget_ep *pep, int status)
{
	struct gadget_request *preq = td->preq;
	u32 remaining, requested, ep_trb_len;
	bool sum_trbs_for_length = false;
	struct gadget_ring *ep_ring;
	u32 trb_comp_code, td_length;

	ep_ring = gadget_dma_to_transfer_ring(pep, le64_to_cpu(event->buffer));
	trb_comp_code = GET_COMP_CODE(le32_to_cpu(event->transfer_len));
	remaining = EVENT_TRB_LEN(le32_to_cpu(event->transfer_len));
	ep_trb_len = TRB_LEN(le32_to_cpu(ep_trb->generic.field[2]));

	requested = preq->request.length;

	switch (trb_comp_code) {
	case COMP_SUCCESS:
		preq->request.status = 0;
		break;
	case COMP_SHORT_PACKET:
		preq->request.status = 0;
		sum_trbs_for_length = true;
		break;
	case COMP_ISOCH_BUFFER_OVERRUN:
	case COMP_BABBLE_DETECTED_ERROR:
		preq->request.status = -EOVERFLOW;
		break;
	case COMP_STOPPED:
		sum_trbs_for_length = true;
		break;
	case COMP_STOPPED_SHORT_PACKET:
		preq->request.status = 0;
		requested = remaining;
		break;
	case COMP_STOPPED_LENGTH_INVALID:
		requested = 0;
		remaining = 0;
		break;
	default:
		sum_trbs_for_length = true;
		preq->request.status = -1;
		break;
	}

	if (sum_trbs_for_length) {
		td_length = gadget_sum_trb_lengths(pdev, ep_ring, ep_trb);
		td_length += ep_trb_len - remaining;
	} else {
		td_length = requested;
	}

	td->preq->request.actual += td_length;

	gadget_finish_td(pdev, td, event, pep, &status);
}

static void gadget_process_bulk_intr_td(struct phytium_device *pdev,
			struct gadget_td *td, union gadget_trb *ep_trb,
			struct gadget_transfer_event *event, struct gadget_ep *ep, int *status)
{
	u32 remaining, requested, ep_trb_len, trb_comp_code;
	struct gadget_ring *ep_ring;

	ep_ring = gadget_dma_to_transfer_ring(ep, le64_to_cpu(event->buffer));
	trb_comp_code = GET_COMP_CODE(le32_to_cpu(event->transfer_len));
	remaining = EVENT_TRB_LEN(le32_to_cpu(event->transfer_len));
	ep_trb_len = TRB_LEN(le32_to_cpu(ep_trb->generic.field[2]));
	requested = td->preq->request.length;

	switch (trb_comp_code) {
	case COMP_SUCCESS:
	case COMP_SHORT_PACKET:
		*status = 0;
		break;
	case COMP_STOPPED_SHORT_PACKET:
		td->preq->request.actual = remaining;
		goto finish_td;
	case COMP_STOPPED_LENGTH_INVALID:
		ep_trb_len = 0;
		remaining = 0;
		break;
	}

	if (ep_trb == td->last_trb)
		ep_trb_len = gadget_sum_trb_lengths(pdev, ep_ring, ep_trb) + ep_trb_len - remaining;

	td->preq->request.actual = ep_trb_len;

finish_td:
	ep->stream_info.drbls_count--;

	gadget_finish_td(pdev, td, event, ep, status);
}

static int gadget_handle_tx_event(struct phytium_device *pdev,
			struct gadget_transfer_event *event)
{
	const struct usb_endpoint_descriptor *desc;
	bool handling_skipped_tds = false;
	struct gadget_segment *ep_seg;
	struct gadget_ring *ep_ring;
	int status = -EINPROGRESS;
	union gadget_trb *ep_trb;
	dma_addr_t ep_trb_dma;
	struct gadget_ep *pep;
	struct gadget_td *td;
	u32 trb_comp_code;
	int invalidate, ep_index;

	invalidate = le32_to_cpu(event->flags) & TRB_EVENT_INVALIDATE;
	ep_index = TRB_TO_EP_ID(le32_to_cpu(event->flags)) - 1;
	trb_comp_code = GET_COMP_CODE(le32_to_cpu(event->transfer_len));
	ep_trb_dma = le64_to_cpu(event->buffer);

	pep = &pdev->eps[ep_index];
	ep_ring = gadget_dma_to_transfer_ring(pep, le64_to_cpu(event->buffer));

	if (invalidate || !pdev->gadget.connected)
		goto cleanup;

	if (GET_EP_CTX_STATE(pep->out_ctx) == EP_STATE_DISABLED)
		goto err_out;

	if (!ep_ring) {
		switch (trb_comp_code) {
		case COMP_INVALID_STREAM_TYPE_ERROR:
		case COMP_INVALID_STREAM_ID_ERROR:
		case COMP_RING_OVERRUN:
		case COMP_RING_UNDERRUN:
			goto cleanup;
		default:
			dev_err(pdev->dev, "Err: %s event for unknown ring\n", pep->name);
			goto err_out;
		}
	}

	switch (trb_comp_code) {
	case COMP_BABBLE_DETECTED_ERROR:
		status = -EOVERFLOW;
		break;
	case COMP_RING_UNDERRUN:
	case COMP_RING_OVERRUN:
		goto cleanup;
	case COMP_MISSED_SERVICE_ERROR:
		pep->skip = true;
		break;
	}

	do {
		if (list_empty(&ep_ring->td_list))
			goto cleanup;

		td = list_entry(ep_ring->td_list.next, struct gadget_td, td_list);

		ep_seg = gadget_trb_in_td(pdev, ep_ring->deq_seg,
				ep_ring->dequeue, td->last_trb, ep_trb_dma);

		if (!ep_seg && (trb_comp_code == COMP_STOPPED ||
					trb_comp_code == COMP_STOPPED_LENGTH_INVALID)) {
			pep->skip = false;
			goto cleanup;
		}

		desc = td->preq->pep->endpoint.desc;
		if (!ep_seg) {
			if (!pep->skip || !usb_endpoint_xfer_isoc(desc))
				dev_err(pdev->dev, "Err Transfer event ep_index %d comp_code %u\n",
						ep_index, trb_comp_code);

			gadget_skip_isoc_td(pdev, td, event, pep, status);
			goto cleanup;
		}

		if (trb_comp_code == COMP_SHORT_PACKET)
			ep_ring->last_td_was_short = true;
		else
			ep_ring->last_td_was_short = false;

		if (pep->skip) {
			pep->skip = false;
			gadget_skip_isoc_td(pdev, td, event, pep, status);
			goto cleanup;
		}

		ep_trb = &ep_seg->trbs[(ep_trb_dma - ep_seg->dma) / sizeof(*ep_trb)];

		if (gadget_trb_is_noop(ep_trb))
			goto cleanup;

		if (usb_endpoint_xfer_control(desc))
			gadget_process_ctrl_td(pdev, td, ep_trb, event, pep, &status);
		else if (usb_endpoint_xfer_isoc(desc))
			gadget_process_isoc_td(pdev, td, ep_trb, event, pep, status);
		else
			gadget_process_bulk_intr_td(pdev, td, ep_trb, event, pep, &status);

cleanup:
		handling_skipped_tds = pep->skip;

		if (!handling_skipped_tds)
			gadget_inc_deq(pdev, pdev->event_ring);
	} while (handling_skipped_tds);

	return 0;

err_out:
	dev_err(pdev->dev, "0x%llx %08x %08x %08x %08x\n",
		(unsigned long long)gadget_trb_virt_to_dma(pdev->event_ring->deq_seg,
		pdev->event_ring->dequeue), lower_32_bits(le64_to_cpu(event->buffer)),
		upper_32_bits(le64_to_cpu(event->buffer)), le32_to_cpu(event->transfer_len),
		le32_to_cpu(event->flags));

	return -EINVAL;
}

static int gadget_ep0_delegate_req(struct phytium_device *pdev, struct usb_ctrlrequest *ctrl)
{
	int ret;

	spin_unlock(&pdev->lock);
	ret = pdev->gadget_driver->setup(&pdev->gadget, ctrl);
	spin_lock(&pdev->lock);

	return ret;
}

static int gadget_w_index_to_ep_index(u16 wIndex)
{
	if (!(wIndex & USB_ENDPOINT_NUMBER_MASK))
		return 0;

	return ((wIndex & USB_ENDPOINT_NUMBER_MASK) * 2) +
			(wIndex & USB_ENDPOINT_DIR_MASK ? 1 : 0) - 1;
}


static int gadget_ep0_handle_status(struct phytium_device *pdev, struct usb_ctrlrequest *ctrl)
{
	struct gadget_ep *pep;
	__le16 *response;
	int ep_sts = 0;
	u16 status = 0;
	u32 recipient;

	recipient = ctrl->bRequestType & USB_RECIP_MASK;

	switch (recipient) {
	case USB_RECIP_DEVICE:
		status = pdev->gadget.is_selfpowered;
		status |= pdev->may_wakeup << USB_DEVICE_REMOTE_WAKEUP;

		if (pdev->gadget.speed >= USB_SPEED_SUPER) {
			status |= pdev->u1_allowed << USB_DEV_STAT_U1_ENABLED;
			status |= pdev->u2_allowed << USB_DEV_STAT_U2_ENABLED;
		}
		break;
	case USB_RECIP_INTERFACE:
		return gadget_ep0_delegate_req(pdev, ctrl);
	case USB_RECIP_ENDPOINT:
		ep_sts = gadget_w_index_to_ep_index(le16_to_cpu(ctrl->wIndex));
		pep = &pdev->eps[ep_sts];
		ep_sts = GET_EP_CTX_STATE(pep->out_ctx);

		if (ep_sts == EP_STATE_HALTED)
			status = BIT(USB_ENDPOINT_HALT);
		break;
	default:
		return -EINVAL;
	}

	response = (__le16 *)pdev->setup_buf;
	*response = cpu_to_le16(status);

	pdev->ep0_preq.request.length = sizeof(*response);
	pdev->ep0_preq.request.buf = pdev->setup_buf;

	return gadget_ep_enqueue(pdev->ep0_preq.pep, &pdev->ep0_preq);
}

static void gadget_enter_test_mode(struct phytium_device *pdev)
{
	u32 temp;

	temp = readl(&pdev->active_port->regs->portpmsc) & ~GENMASK(31, 28);
	temp |= PORT_TEST_MODE(pdev->test_mode);
	writel(temp, &pdev->active_port->regs->portpmsc);
}

static int gadget_ep0_handle_feature_device(struct phytium_device *pdev,
			struct usb_ctrlrequest *ctrl, int set)
{
	enum usb_device_state state;
	enum usb_device_speed speed;
	u16 tmode;

	state = pdev->gadget.state;
	speed = pdev->gadget.speed;

	switch (le16_to_cpu(ctrl->wValue)) {
	case USB_DEVICE_REMOTE_WAKEUP:
		pdev->may_wakeup = !!set;
		break;
	case USB_DEVICE_U1_ENABLE:
		if (state != USB_STATE_CONFIGURED || speed < USB_SPEED_SUPER)
			return -EINVAL;

		pdev->u1_allowed = !!set;
		break;
	case USB_DEVICE_U2_ENABLE:
		if (state != USB_STATE_CONFIGURED || speed < USB_SPEED_SUPER)
			return -EINVAL;

		pdev->u2_allowed = !!set;
		break;
	case USB_DEVICE_LTM_ENABLE:
		return -EINVAL;
	case USB_DEVICE_TEST_MODE:
		if (state != USB_STATE_CONFIGURED || speed < USB_SPEED_SUPER)
			return -EINVAL;

		tmode = le16_to_cpu(ctrl->wIndex);

		if (!set || (tmode & 0xff) != 0)
			return -EINVAL;

		tmode = tmode >> 8;

		if (tmode > USB_TEST_FORCE_ENABLE || tmode < USB_TEST_J)
			return -EINVAL;

		pdev->test_mode = tmode;
		gadget_enter_test_mode(pdev);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int gadget_ep0_handle_feature_intf(struct phytium_device *pdev,
			struct usb_ctrlrequest *ctrl, int set)
{
	u16 wValue, wIndex;
	int ret;

	wValue = le16_to_cpu(ctrl->wValue);
	wIndex = le16_to_cpu(ctrl->wIndex);

	switch (wValue) {
	case USB_INTRF_FUNC_SUSPEND:
		ret = gadget_ep0_delegate_req(pdev, ctrl);
		if (ret)
			return ret;

		if (wIndex & USB_INTRF_FUNC_SUSPEND_RW)
			pdev->may_wakeup++;
		else
			if (pdev->may_wakeup > 0)
				pdev->may_wakeup--;

		return 0;
	default:
		return -EINVAL;
	}

	return 0;
}

static int gadget_ep0_handle_feature_endpoint(struct phytium_device *pdev,
			struct usb_ctrlrequest *ctrl, int set)
{
	struct gadget_ep *pep;
	u16 wValue;

	wValue = le16_to_cpu(ctrl->wValue);
	pep = &pdev->eps[gadget_w_index_to_ep_index(le16_to_cpu(ctrl->wIndex))];

	switch (wValue) {
	case USB_ENDPOINT_HALT:
		if (!set && (pep->ep_state & EP_WEDGE)) {
			gadget_halt_endpoint(pdev, pep, 0);
			gadget_halt_endpoint(pdev, pep, 1);
			break;
		}
		return gadget_halt_endpoint(pdev, pep, set);
	default:
		dev_warn(pdev->dev, "Warn Incorrect wValue %04x\n", wValue);
		return -EINVAL;
	}

	return 0;
}

static int gadget_ep0_handle_feature(struct phytium_device *pdev,
			struct usb_ctrlrequest *ctrl, int set)
{
	switch (ctrl->bRequestType & USB_RECIP_MASK) {
	case USB_RECIP_DEVICE:
		return gadget_ep0_handle_feature_device(pdev, ctrl, set);
	case USB_RECIP_INTERFACE:
		return gadget_ep0_handle_feature_intf(pdev, ctrl, set);
	case USB_RECIP_ENDPOINT:
		return gadget_ep0_handle_feature_endpoint(pdev, ctrl, set);
	default:
		return -EINVAL;
	}
}

static int gadget_ep0_set_address(struct phytium_device *pdev,
			struct usb_ctrlrequest *ctrl)
{
	enum usb_device_state state = pdev->gadget.state;
	struct gadget_slot_ctx *slot_ctx;
	unsigned int slot_state;
	int ret;
	u32 addr;

	addr = le16_to_cpu(ctrl->wValue);
	if (addr > 127) {
		dev_err(pdev->dev, "Invalid device address %d\n", addr);
		return -EINVAL;
	}

	slot_ctx = gadget_get_slot_ctx(&pdev->out_ctx);

	if (state == USB_STATE_CONFIGURED) {
		dev_err(pdev->dev, "Can't Set Address from Configured State\n");
		return -EINVAL;
	}

	pdev->device_address = le16_to_cpu(ctrl->wValue);

	slot_ctx = gadget_get_slot_ctx(&pdev->out_ctx);
	slot_state = GET_SLOT_STATE(le32_to_cpu(slot_ctx->dev_state));
	if (slot_state == SLOT_STATE_ADDRESSED)
		gadget_reset_device(pdev);

	ret = gadget_setup_device(pdev, SETUP_CONTEXT_ADDRESS);
	if (ret)
		return ret;

	if (addr)
		usb_gadget_set_state(&pdev->gadget, USB_STATE_ADDRESS);
	else
		usb_gadget_set_state(&pdev->gadget, USB_STATE_DEFAULT);

	return 0;
}

static int gadget_ep0_set_config(struct phytium_device *pdev,
			struct usb_ctrlrequest *ctrl)
{
	enum usb_device_state state = pdev->gadget.state;
	u32 cfg;
	int ret;

	cfg = le16_to_cpu(ctrl->wValue);

	switch (state) {
	case USB_STATE_ADDRESS:
		break;
	case USB_STATE_CONFIGURED:
		break;
	default:
		dev_err(pdev->dev, "Set Configuration - bad device state\n");
		return -EINVAL;
	}

	ret = gadget_ep0_delegate_req(pdev, ctrl);
	if (ret)
		return ret;

	if (!cfg)
		usb_gadget_set_state(&pdev->gadget, USB_STATE_ADDRESS);

	return 0;
}

static int gadget_ep0_set_sel(struct phytium_device *pdev,
			struct usb_ctrlrequest *ctrl)
{
	enum usb_device_state state = pdev->gadget.state;
	u16 wLength;

	if (state == USB_STATE_DEFAULT)
		return -EINVAL;

	wLength = le16_to_cpu(ctrl->wLength);

	if (wLength != 6) {
		dev_err(pdev->dev, "Set SEL should be 6 bytes, got %d\n", wLength);
		return -EINVAL;
	}

	pdev->ep0_preq.request.length = 6;
	pdev->ep0_preq.request.buf = pdev->setup_buf;

	return gadget_ep_enqueue(pdev->ep0_preq.pep, &pdev->ep0_preq);
}

static int gadget_ep0_set_isoch_delay(struct phytium_device *pdev,
			struct usb_ctrlrequest *ctrl)
{
	if (le16_to_cpu(ctrl->wIndex) || le16_to_cpu(ctrl->wLength))
		return -EINVAL;

	pdev->gadget.isoch_delay = le16_to_cpu(ctrl->wValue);

	return 0;
}

static int gadget_ep0_std_request(struct phytium_device *pdev, struct usb_ctrlrequest *ctrl)
{
	int ret;

	switch (ctrl->bRequest) {
	case USB_REQ_GET_STATUS:
		ret = gadget_ep0_handle_status(pdev, ctrl);
		break;
	case USB_REQ_CLEAR_FEATURE:
		ret = gadget_ep0_handle_feature(pdev, ctrl, 0);
		break;
	case USB_REQ_SET_FEATURE:
		ret = gadget_ep0_handle_feature(pdev, ctrl, 1);
		break;
	case USB_REQ_SET_ADDRESS:
		ret = gadget_ep0_set_address(pdev, ctrl);
		break;
	case USB_REQ_SET_CONFIGURATION:
		ret = gadget_ep0_set_config(pdev, ctrl);
		break;
	case USB_REQ_SET_SEL:
		ret = gadget_ep0_set_sel(pdev, ctrl);
		break;
	case USB_REQ_SET_ISOCH_DELAY:
		ret = gadget_ep0_set_isoch_delay(pdev, ctrl);
		break;
	case USB_REQ_SET_INTERFACE:
		list_add_tail(&pdev->ep0_preq.list, &pdev->ep0_preq.pep->pending_list);
		ret = gadget_ep0_delegate_req(pdev, ctrl);
		if (ret == -EBUSY)
			ret = 0;

		list_del(&pdev->ep0_preq.list);
		break;
	default:
		ret = gadget_ep0_delegate_req(pdev, ctrl);
		break;
	}

	return ret;
}

int gadget_status_stage(struct phytium_device *pdev)
{
	pdev->ep0_stage = GADGET_STATUS_STAGE;
	pdev->ep0_preq.request.length = 0;

	return gadget_ep_enqueue(pdev->ep0_preq.pep, &pdev->ep0_preq);
}

static void gadget_ep0_stall(struct phytium_device *pdev)
{
	struct gadget_request *preq;
	struct gadget_ep *pep;

	pep = &pdev->eps[0];
	preq = next_request(&pep->pending_list);

	if (pdev->three_stage_setup) {
		gadget_halt_endpoint(pdev, pep, true);
		if (preq)
			gadget_giveback(pep, preq, -ECONNRESET);
	} else {
		pep->ep_state |= EP0_HALTED_STATUS;

		if (preq)
			list_del(&preq->list);

		gadget_status_stage(pdev);
	}
}

static void gadget_setup_analyze(struct phytium_device *pdev)
{
	struct usb_ctrlrequest *ctrl = &pdev->setup;
	int ret = 0;
	u16 len;

	if (!pdev->gadget_driver)
		goto out;

	if (pdev->gadget.state == USB_STATE_NOTATTACHED) {
		dev_err(pdev->dev, "Err: Setup detected in unattached state\n");
		ret = -EINVAL;
		goto out;
	}

	if (pdev->eps[0].ep_state & EP_HALTED)
		gadget_halt_endpoint(pdev, &pdev->eps[0], 0);

	if (!list_empty(&pdev->eps[0].pending_list)) {
		struct gadget_request *req;

		req = next_request(&pdev->eps[0].pending_list);
		ep_dequeue(&pdev->eps[0], req);
	}

	len = le16_to_cpu(ctrl->wLength);
	if (!len) {
		pdev->three_stage_setup = false;
		pdev->ep0_expect_in = false;
	} else {
		pdev->three_stage_setup = true;
		pdev->ep0_expect_in = !!(ctrl->bRequestType & USB_DIR_IN);
	}

	if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD)
		ret = gadget_ep0_std_request(pdev, ctrl);
	else
		ret = gadget_ep0_delegate_req(pdev, ctrl);

	if (!len)
		pdev->ep0_stage = GADGET_STATUS_STAGE;

	if (ret == USB_GADGET_DELAYED_STATUS)
		return;

out:
	if (ret < 0)
		gadget_ep0_stall(pdev);
	else if (pdev->ep0_stage == GADGET_STATUS_STAGE)
		gadget_status_stage(pdev);
}

static void gadget_handle_tx_nrdy(struct phytium_device *pdev, struct gadget_transfer_event *event)
{
	struct gadget_generic_trb *generic;
	struct gadget_ring *ep_ring;
	struct gadget_ep *pep;
	int cur_stream, ep_index, host_sid, dev_sid;

	generic = (struct gadget_generic_trb *)event;
	ep_index = TRB_TO_EP_ID(le32_to_cpu(event->flags)) - 1;
	dev_sid = TRB_TO_DEV_STREAM(le32_to_cpu(generic->field[0]));
	host_sid = TRB_TO_HOST_STREAM(le32_to_cpu(generic->field[2]));

	pep = &pdev->eps[ep_index];

	if (!(pep->ep_state & EP_HAS_STREAMS))
		return;

	if (host_sid == STREAM_PRIME_ACK) {
		pep->stream_info.first_prime_det = 1;
		for (cur_stream = 1; cur_stream < pep->stream_info.num_streams; cur_stream++) {
			ep_ring = pep->stream_info.stream_rings[cur_stream];
			ep_ring->stream_active = 1;
			ep_ring->stream_rejected = 0;
		}
	}

	if (host_sid == STREAM_REJECTED) {
		struct gadget_td *td, *td_temp;

		pep->stream_info.drbls_count--;
		ep_ring = pep->stream_info.stream_rings[dev_sid];
		ep_ring->stream_active = 0;
		ep_ring->stream_rejected = 1;

		list_for_each_entry_safe(td, td_temp, &ep_ring->td_list, td_list)
			td->drbl = 0;
	}

	gadget_ring_doorbell_for_active_rings(pdev, pep);
}

static bool gadget_handle_event(struct phytium_device *pdev)
{
	unsigned int comp_code;
	union gadget_trb *event;
	bool update_ptrs = true;
	u32 cycle_bit, flags;
	int ret = 0;

	event = pdev->event_ring->dequeue;
	flags = le32_to_cpu(event->event_cmd.flags);
	cycle_bit = (flags & TRB_CYCLE);

	if (cycle_bit != pdev->event_ring->cycle_state)
		return false;

	/*
	 * Barrier between reading the TRB_CYCLE (valid) flag above and any
	 * reads of the event's flags/data below.
	 */
	rmb();

	switch (flags & TRB_TYPE_BITMASK) {
	case TRB_TYPE(TRB_COMPLETION):
		gadget_inc_deq(pdev, pdev->cmd_ring);
		break;
	case TRB_TYPE(TRB_PORT_STATUS):
		gadget_handle_port_status(pdev, event);
		update_ptrs = false;
		break;
	case TRB_TYPE(TRB_TRANSFER):
		ret = gadget_handle_tx_event(pdev, &event->trans_event);
		if (ret >= 0)
			update_ptrs = false;
		break;
	case TRB_TYPE(TRB_SETUP):
		pdev->ep0_stage = GADGET_SETUP_STAGE;
		pdev->setup_id = TRB_SETUPID_TO_TYPE(flags);
		pdev->setup_speed = TRB_SETUP_SPEEDID(flags);
		pdev->setup = *((struct usb_ctrlrequest *)&event->trans_event.buffer);

		gadget_setup_analyze(pdev);
		break;
	case TRB_TYPE(TRB_ENDPOINT_NRDY):
		gadget_handle_tx_nrdy(pdev, &event->trans_event);
		break;
	case TRB_TYPE(TRB_HC_EVENT): {
		comp_code = GET_COMP_CODE(le32_to_cpu(event->generic.field[2]));
		switch (comp_code) {
		case COMP_EVENT_RING_FULL_ERROR:
			dev_err(pdev->dev, "Event Ring Full\n");
			break;
		default:
			dev_err(pdev->dev, "Controller error code 0x%02x\n", comp_code);
		}
		break;
	}
	default:
		dev_warn(pdev->dev, "Error unknown event type %ld\n", TRB_FIELD_TO_TYPE(flags));
	}

	if (update_ptrs)
		gadget_inc_deq(pdev, pdev->event_ring);

	return true;
}

irqreturn_t gadget_irq_handler(int irq, void *priv)
{
	struct phytium_device *pdev = (struct phytium_device *)priv;
	u32 irq_pending;
	u32 status;
	union gadget_trb *event_ring_deq;
	unsigned long flags;
	int counter = 0;

	spin_lock_irqsave(&pdev->lock, flags);
	status = readl(&pdev->op_regs->status);

	if (status == ~(u32)0) {
		gadget_died(pdev);
		spin_unlock_irqrestore(&pdev->lock, flags);
		return IRQ_HANDLED;
	}

	if (!(status & STS_EINT)) {
		spin_unlock_irqrestore(&pdev->lock, flags);
		return IRQ_NONE;
	}

	writel(status | STS_EINT, &pdev->op_regs->status);
	irq_pending = readl(&pdev->ir_set->irq_pending);
	irq_pending |= IMAN_IP;
	writel(irq_pending, &pdev->ir_set->irq_pending);

	if (status & STS_FATAL) {
		gadget_died(pdev);
		spin_unlock_irqrestore(&pdev->lock, flags);
		return IRQ_HANDLED;
	}

	if (pdev->gadget_state & (GADGET_STATE_HALTED | GADGET_STATE_DYING)) {
		if (pdev->gadget_driver)
			gadget_died(pdev);

		spin_unlock_irqrestore(&pdev->lock, flags);
		return IRQ_HANDLED;
	}

	event_ring_deq = pdev->event_ring->dequeue;

	while (gadget_handle_event(pdev)) {
		if (++counter >= TRBS_PER_EV_DEQ_UPDATE) {
			gadget_update_erst_dequeue(pdev, event_ring_deq, 0);
			event_ring_deq = pdev->event_ring->dequeue;
			counter = 0;
		}
	}

	gadget_update_erst_dequeue(pdev, event_ring_deq, 1);
	spin_unlock_irqrestore(&pdev->lock, flags);

	return IRQ_HANDLED;
}
void gadget_queue_address_device(struct phytium_device *pdev,
		dma_addr_t in_ctx_ptr, enum gadget_setup_dev setup)
{
	gadget_queue_command(pdev, lower_32_bits(in_ctx_ptr), upper_32_bits(in_ctx_ptr), 0,
		TRB_TYPE(TRB_ADDR_DEV) | SLOT_ID_FOR_TRB(pdev->slot_id) |
		(setup == SETUP_CONTEXT_ONLY ? TRB_BSR : 0));
}

static int gadget_prepare_transfer(struct phytium_device *pdev,
				struct gadget_request *preq, unsigned int num_trbs)
{
	struct gadget_ring *ep_ring;
	int ret;

	ep_ring = gadget_get_transfer_ring(pdev, preq->pep, preq->request.stream_id);
	if (!ep_ring)
		return -EINVAL;

	ret = gadget_prepare_ring(pdev, ep_ring, GET_EP_CTX_STATE(preq->pep->out_ctx),
			num_trbs, GFP_ATOMIC);
	if (ret)
		return ret;

	INIT_LIST_HEAD(&preq->td.td_list);
	preq->td.preq = preq;

	list_add_tail(&preq->td.td_list, &ep_ring->td_list);
	ep_ring->num_tds++;
	preq->pep->stream_info.td_count++;

	preq->td.start_seg = ep_ring->enq_seg;
	preq->td.first_trb = ep_ring->enqueue;

	return 0;
}

static u32 gadget_td_remainder(struct phytium_device *pdev, int transferred, int trb_buff_len,
				unsigned int td_total_len, struct gadget_request *preq,
				bool more_trbs_coming, bool zlp)
{
	u32 maxp, total_packet_count;

	if (zlp)
		return 1;

	if (!more_trbs_coming || (transferred == 0 && trb_buff_len == 0)
			|| trb_buff_len == td_total_len)
		return 0;

	maxp = usb_endpoint_maxp(preq->pep->endpoint.desc);
	total_packet_count = DIV_ROUND_UP(td_total_len, maxp);

	return (total_packet_count - ((transferred + trb_buff_len) / maxp));
}

int gadget_queue_ctrl_tx(struct phytium_device *pdev, struct gadget_request *preq)
{
	u32 field, length_field, remainder;
	struct gadget_ep *pep = preq->pep;
	struct gadget_ring *ep_ring;
	int num_trbs;
	int ret;

	ep_ring = gadget_request_to_transfer_ring(pdev, preq);
	if (!ep_ring)
		return -EINVAL;

	num_trbs = (pdev->three_stage_setup) ? 2 : 1;

	ret = gadget_prepare_transfer(pdev, preq, num_trbs);
	if (ret)
		return ret;

	if (pdev->ep0_expect_in)
		field = TRB_TYPE(TRB_DATA) | TRB_IOC;
	else
		field = TRB_ISP | TRB_TYPE(TRB_DATA) | TRB_IOC;

	if (preq->request.length > 0) {
		remainder = gadget_td_remainder(pdev, 0, preq->request.length,
				preq->request.length, preq, 1, 0);

		length_field = TRB_LEN(preq->request.length) | TRB_TD_SIZE(remainder) |
			TRB_INTR_TARGET(0);

		if (pdev->ep0_expect_in)
			field |= TRB_DIR_IN;

		gadget_queue_trb(pdev, ep_ring, true, lower_32_bits(preq->request.dma),
				upper_32_bits(preq->request.dma), length_field, field |
				ep_ring->cycle_state | TRB_SETUPID(pdev->setup_id) |
				pdev->setup_speed);

				pdev->ep0_stage = GADGET_DATA_STAGE;
	}

	preq->td.last_trb = ep_ring->enqueue;

	if (preq->request.length == 0)
		field = ep_ring->cycle_state;
	else
		field = (ep_ring->cycle_state ^ 1);

	if (preq->request.length > 0 && pdev->ep0_expect_in)
		field |= TRB_DIR_IN;

	if (pep->ep_state & EP0_HALTED_STATUS) {
		pep->ep_state &= ~EP0_HALTED_STATUS;
		field |= TRB_SETUPSTAT(TRB_SETUPSTAT_STALL);
	} else {
		field |= TRB_SETUPSTAT(TRB_SETUPSTAT_ACK);
	}

	gadget_queue_trb(pdev, ep_ring, false, 0, 0, TRB_INTR_TARGET(0), field | TRB_IOC |
			TRB_SETUPID(pdev->setup_id) | TRB_TYPE(TRB_STATUS) | pdev->setup_speed);

	gadget_ring_ep_doorbell(pdev, pep, preq->request.stream_id);

	return 0;
}

static unsigned int gadget_count_trbs(u64 addr, u64 len)
{
	unsigned int num_trbs;

	num_trbs = DIV_ROUND_UP(len + (addr & (TRB_MAX_BUFF_SIZE - 1)), TRB_MAX_BUFF_SIZE);

	if (num_trbs == 0)
		num_trbs++;

	return num_trbs;
}

static unsigned int count_trbs_needed(struct gadget_request *preq)
{
	return gadget_count_trbs(preq->request.dma, preq->request.length);
}

static unsigned int count_sg_trbs_needed(struct gadget_request *preq)
{
	unsigned int i, len, full_len, num_trbs = 0;
	struct scatterlist *sg;

	full_len = preq->request.length;

	for_each_sg(preq->request.sg, sg, preq->request.num_sgs, i) {
		len = sg_dma_len(sg);
		num_trbs += gadget_count_trbs(sg_dma_address(sg), len);
		len = min(len, full_len);
		full_len -= len;
		if (full_len == 0)
			break;
	}

	return num_trbs;
}

static int gadget_align_td(struct phytium_device *pdev, struct gadget_request *preq,
			u32 enqd_len, u32 *trb_buff_len, struct gadget_segment *seg)
{
	struct device *dev = pdev->dev;
	unsigned int unalign, max_pkt;
	u32 new_buff_len;

	max_pkt = usb_endpoint_maxp(preq->pep->endpoint.desc);
	unalign = (enqd_len + *trb_buff_len) % max_pkt;

	if (unalign == 0)
		return 0;

	if (*trb_buff_len > unalign) {
		*trb_buff_len -= unalign;
		return 0;
	}

	new_buff_len = max_pkt - (enqd_len % max_pkt);

	if (new_buff_len > (preq->request.length - enqd_len))
		new_buff_len = preq->request.length - enqd_len;

	if (preq->direction) {
		sg_pcopy_to_buffer(preq->request.sg, preq->request.num_mapped_sgs,
			seg->bounce_buf, new_buff_len, enqd_len);

		seg->bounce_dma = dma_map_single(dev, seg->bounce_buf, max_pkt, DMA_TO_DEVICE);
	} else {
		seg->bounce_dma = dma_map_single(dev, seg->bounce_buf, max_pkt, DMA_FROM_DEVICE);
	}

	if (dma_mapping_error(dev, seg->bounce_dma)) {
		dev_warn(dev, "Failed mapping bounce buffer, not aligning\n");
		return 0;
	}

	*trb_buff_len = new_buff_len;
	seg->bounce_len = new_buff_len;
	seg->bounce_offs = enqd_len;

	return 1;
}

static void gadget_check_trb_math(struct gadget_request *preq, int running_total)
{
	if (running_total != preq->request.length)
		dev_err(preq->pep->pdev->dev,
				"%s - Miscalculated tx length, queued %#x, asked for %#x (%d)\n",
				preq->pep->name, running_total, preq->request.length,
				preq->request.actual);
}

int gadget_queue_bulk_tx(struct phytium_device *pdev, struct gadget_request *preq)
{
	unsigned int enqd_len, block_len, trb_buff_len, full_len;
	unsigned int start_cycle, num_sgs = 0;
	struct gadget_generic_trb *start_trb;
	u32 field, length_field, remainder;
	struct scatterlist *sg = NULL;
	bool more_trbs_coming = true;
	bool need_zero_pkt = false;
	bool zero_len_trb = false;
	struct gadget_ring *ring;
	bool first_trb = true;
	unsigned int num_trbs;
	struct gadget_ep *pep;
	u64 addr, send_addr;
	int sent_len, ret;

	ring = gadget_request_to_transfer_ring(pdev, preq);
	if (!ring)
		return -EINVAL;

	full_len = preq->request.length;

	if (preq->request.num_sgs) {
		num_sgs = preq->request.num_sgs;
		sg = preq->request.sg;
		addr = (u64)sg_dma_address(sg);
		block_len = sg_dma_len(sg);
		num_trbs = count_sg_trbs_needed(preq);
	} else {
		num_trbs = count_trbs_needed(preq);
		addr = (u64)preq->request.dma;
		block_len = full_len;
	}

	pep = preq->pep;

	if (preq->request.zero && preq->request.length &&
		 IS_ALIGNED(full_len, usb_endpoint_maxp(pep->endpoint.desc))) {
		need_zero_pkt = true;
		num_trbs++;
	}

	ret = gadget_prepare_transfer(pdev, preq, num_trbs);
	if (ret)
		return ret;

	start_trb = &ring->enqueue->generic;
	start_cycle = ring->cycle_state;
	send_addr = addr;

	for (enqd_len = 0; zero_len_trb || first_trb || enqd_len < full_len;
			enqd_len += trb_buff_len) {
		field = TRB_TYPE(TRB_NORMAL);

		trb_buff_len = TRB_BUFF_LEN_UP_TO_BOUNDARY(addr);
		trb_buff_len = min(trb_buff_len, block_len);
		if (enqd_len + trb_buff_len > full_len)
			trb_buff_len = full_len - enqd_len;

		if (first_trb) {
			first_trb = false;
			if (start_cycle == 0)
				field |= TRB_CYCLE;
		} else {
			field |= ring->cycle_state;
		}

		if (enqd_len + trb_buff_len < full_len || need_zero_pkt) {
			field |= TRB_CHAIN;
			if (gadget_trb_is_link(ring->enqueue + 1)) {
				if (gadget_align_td(pdev, preq, enqd_len,
							&trb_buff_len, ring->enq_seg)) {
					send_addr = ring->enq_seg->bounce_dma;
					preq->td.bounce_seg = ring->enq_seg;
				}
			}
		}

		if (enqd_len + trb_buff_len >= full_len) {
			if (need_zero_pkt && !zero_len_trb) {
				zero_len_trb = true;
			} else {
				zero_len_trb = false;
				field &= ~TRB_CHAIN;
				field |= TRB_IOC;
				more_trbs_coming = false;
				need_zero_pkt = false;
				preq->td.last_trb = ring->enqueue;
			}
		}

		if (!preq->direction)
			field |= TRB_ISP;

		remainder = gadget_td_remainder(pdev, enqd_len, trb_buff_len, full_len, preq,
						more_trbs_coming, zero_len_trb);

		length_field = TRB_LEN(trb_buff_len) | TRB_TD_SIZE(remainder) | TRB_INTR_TARGET(0);

		gadget_queue_trb(pdev, ring, more_trbs_coming, lower_32_bits(send_addr),
						 upper_32_bits(send_addr), length_field, field);

		addr += trb_buff_len;
		sent_len = trb_buff_len;

		while (sg && sent_len >= block_len) {
			--num_sgs;
			sent_len -= block_len;
			if (num_sgs != 0) {
				sg = sg_next(sg);
				block_len = sg_dma_len(sg);
				addr = (u64)sg_dma_address(sg);
				addr += sent_len;
			}
		}

		block_len -= sent_len;
		send_addr = addr;
	}

	gadget_check_trb_math(preq, enqd_len);
	ret = gadget_giveback_first_trb(pdev, pep, preq->request.stream_id, start_cycle, start_trb);
	if (ret)
		preq->td.drbl = 1;

	return 0;
}

static unsigned int count_isoc_trbs_needed(struct gadget_request *preq)
{
	return gadget_count_trbs(preq->request.dma, preq->request.length);
}

static unsigned int gadget_get_burst_count(struct phytium_device *pdev,
				struct gadget_request *preq, unsigned int total_packet_count)
{
	unsigned int max_burst;

	if (pdev->gadget.speed < USB_SPEED_SUPER)
		return 0;

	max_burst = preq->pep->endpoint.comp_desc->bMaxBurst;

	return DIV_ROUND_UP(total_packet_count, max_burst + 1) - 1;
}

static unsigned int gadget_get_last_burst_packet_count(struct phytium_device *pdev,
			struct gadget_request *preq, unsigned int total_packet_count)
{
	unsigned int max_burst, residue;

	if (pdev->gadget.speed >= USB_SPEED_SUPER) {
		max_burst = preq->pep->endpoint.comp_desc->bMaxBurst;
		residue = total_packet_count % (max_burst + 1);

		if (residue == 0)
			return max_burst;

		return residue - 1;
	}

	if (total_packet_count == 0)
		return 0;

	return total_packet_count - 1;
}

static int gadget_queue_isoc_tx(struct phytium_device *pdev, struct gadget_request *preq)
{
	int trb_buff_len, td_len, td_remain_len, ret;
	unsigned int burst_count, last_burst_pkt, total_pkt_count, max_pkt;
	struct gadget_generic_trb *start_trb;
	bool more_trbs_coming = true;
	struct gadget_ring *ep_ring;
	u32 field, length_field;
	int start_cycle, trbs_per_td, i;
	int running_total = 0;
	u64 addr;

	ep_ring = preq->pep->ring;
	start_trb = &ep_ring->enqueue->generic;
	start_cycle = ep_ring->cycle_state;
	td_len = preq->request.length;
	addr = (u64)preq->request.dma;
	td_remain_len = td_len;

	max_pkt = usb_endpoint_maxp(preq->pep->endpoint.desc);
	total_pkt_count = DIV_ROUND_UP(td_len, max_pkt);

	if (total_pkt_count == 0)
		total_pkt_count++;

	burst_count = gadget_get_burst_count(pdev, preq, total_pkt_count);
	last_burst_pkt = gadget_get_last_burst_packet_count(pdev, preq, total_pkt_count);
	trbs_per_td = count_isoc_trbs_needed(preq);

	ret = gadget_prepare_transfer(pdev, preq, trbs_per_td);
	if (ret)
		goto cleanup;

	field = TRB_TYPE(TRB_ISOC) | TRB_TLBPC(last_burst_pkt) | TRB_SIA | TRB_TBC(burst_count);

	if	(!start_cycle)
		field |= TRB_CYCLE;

	for (i = 0; i < trbs_per_td; i++) {
		u32 remainder;

		trb_buff_len = TRB_BUFF_LEN_UP_TO_BOUNDARY(addr);
		if (trb_buff_len > td_remain_len)
			trb_buff_len = td_remain_len;

		remainder = gadget_td_remainder(pdev, running_total, trb_buff_len, td_len,
						preq, more_trbs_coming, 0);

		length_field = TRB_LEN(trb_buff_len) | TRB_INTR_TARGET(0);

		if (i) {
			field = TRB_TYPE(TRB_NORMAL) | ep_ring->cycle_state;
			length_field |= TRB_TD_SIZE(remainder);
		} else {
			length_field |= TRB_TD_SIZE_TBC(burst_count);
		}

		if (usb_endpoint_dir_out(preq->pep->endpoint.desc))
			field |= TRB_ISP;

		if (i < trbs_per_td - 1) {
			more_trbs_coming = true;
			field |= TRB_CHAIN;
		} else {
			more_trbs_coming = false;
			preq->td.last_trb = ep_ring->enqueue;
			field |= TRB_IOC;
		}

		gadget_queue_trb(pdev, ep_ring, more_trbs_coming, lower_32_bits(addr),
					upper_32_bits(addr), length_field, field);

		running_total += trb_buff_len;
		addr += trb_buff_len;
		td_remain_len -= trb_buff_len;
	}

	if (running_total != td_len) {
		dev_err(pdev->dev, "ISOC TD length unmatch\n");
		ret = -EINVAL;
		goto cleanup;
	}

	gadget_giveback_first_trb(pdev, preq->pep, preq->request.stream_id, start_cycle, start_trb);

	return 0;

cleanup:
	list_del_init(&preq->td.td_list);
	ep_ring->num_tds--;
	preq->td.last_trb = ep_ring->enqueue;
	gadget_td_to_noop(pdev, ep_ring, &preq->td, true);

	ep_ring->enqueue = preq->td.first_trb;
	ep_ring->enq_seg = preq->td.start_seg;
	ep_ring->cycle_state = start_cycle;

	return ret;
}

int gadget_queue_isoc_tx_prepare(struct phytium_device *pdev, struct gadget_request *preq)
{
	struct gadget_ring *ep_ring;
	u32 ep_state;
	int num_trbs, ret;

	ep_ring = preq->pep->ring;
	ep_state = GET_EP_CTX_STATE(preq->pep->out_ctx);
	num_trbs = count_isoc_trbs_needed(preq);

	ret = gadget_prepare_ring(pdev, ep_ring, ep_state, num_trbs, GFP_ATOMIC);
	if (ret)
		return ret;

	return gadget_queue_isoc_tx(pdev, preq);
}

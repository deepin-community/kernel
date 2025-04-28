// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium USB DRD Driver.
 *
 * Copyright (C) 2023 - 2024 Phytium.
 */

#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include "mem.h"
#include "gadget.h"
#include "ring.h"

#define NUM_PORT_REGS	4

#define GADGET_EXT_PORT_MAJOR(x)	(((x) >> 24) & 0xff)
#define GADGET_EXT_PORT_MINOR(x)	(((x) >> 16) & 0xff)
#define GADGET_EXT_PORT_OFF(x)		((x) & 0xff)
#define GADGET_EXT_PORT_COUNT(x)	(((x) >> 8) & 0xff)

#define HCC_64BYTE_CONTEXT(p) ((p) & BIT(2))
#define CTX_SIZE(hcc) (HCC_64BYTE_CONTEXT(hcc) ? 64 : 32)

#define GADGET_CTX_TYPE_DEVICE 0x1
#define GADGET_CTX_TYPE_INPUT 0x2

#define EXT_CAPS_PROTOCOL 2
#define EXT_CAP_CFG_DEV_20PORT_CAP_ID 0xC1
#define D_XEC_CFG_3XPORT_CAP 0xC0

#define MAX_DEVS GENMASK(7, 0)
#define CONFIG_U3E BIT(8)

#define ERST_SIZE_MASK GENMASK(31, 16)
#define ERST_NUM_SEGS 1
#define TRB_SEGMENT_SIZE (TRBS_PER_SEGMENT * 16)
#define TRB_SEGMENT_SHIFT (ilog2(TRB_SEGMENT_SIZE))


#define ERROR_COUNT(p)	(((p) & 0x3) << 1)

#define MAX_BURST(p)	(((p) << 8) & GENMASK(15, 8))
#define MAX_PACKET(p)	(((p) << 16) & GENMASK(31, 16))

#define EP_INTERVAL(p)	(((p) << 16) & GENMASK(23, 16))
#define EP_MULT(p)	(((p) << 8) & GENMASK(9, 8))

#define EP_AVG_TRB_LENGTH(p)	((p) & GENMASK(15, 0))

#define EP_MAX_ESIT_PAYLOAD_LO(p)	(((p) << 16) & GENMASK(31, 16))
#define EP_MAX_ESIT_PAYLOAD_HI(p)	((((p) & GENMASK(23, 16)) >> 16) << 24)

#define GADGET_CTX_TYPE_DEVICE	0x1
#define GADGET_CTX_TYPE_INPTU	0x2

#define SLOT_SPEED_FS	(XDEV_FS << 10)
#define SLOT_SPEED_HS	(XDEV_HS << 10)
#define SLOT_SPEED_SS	(XDEV_SS << 10)
#define SLOT_SPEED_SSP	(XDEV_SSP << 10)

struct gadget_ep_ctx *gadget_get_ep_ctx(struct gadget_container_ctx *ctx,
		unsigned int ep_index)
{
	ep_index++;
	if (ctx->type == GADGET_CTX_TYPE_INPUT)
		ep_index++;

	return (struct gadget_ep_ctx *)(ctx->bytes + (ep_index * ctx->ctx_size));
}

struct gadget_slot_ctx *gadget_get_slot_ctx(struct gadget_container_ctx *ctx)
{
	if (ctx->type == GADGET_CTX_TYPE_DEVICE)
		return (struct gadget_slot_ctx *)ctx->bytes;

	return (struct gadget_slot_ctx *)(ctx->bytes + ctx->ctx_size);
}

	struct gadget_input_control_ctx
	*gadget_get_input_control_ctx(struct gadget_container_ctx *ctx)
{
	if (ctx->type != GADGET_CTX_TYPE_INPUT)
		return NULL;

	return (struct gadget_input_control_ctx *)ctx->bytes;
}

static void gadget_link_segments(struct phytium_device *pdev,
		struct gadget_segment *prev, struct gadget_segment *next,
		enum gadget_ring_type type)
{
	struct gadget_link_trb *link;
	u32 val;

	if (!prev || !next)
		return;

	prev->next = next;
	if (type != TYPE_EVENT) {
		link = &prev->trbs[TRBS_PER_SEGMENT - 1].link;
		link->segment_ptr = cpu_to_le64(next->dma);

		val = le32_to_cpu(link->control);
		val &= ~TRB_TYPE_BITMASK;
		val |= TRB_TYPE(TRB_LINK);
		link->control = cpu_to_le32(val);
	}
}

static void gadget_segment_free(struct phytium_device *pdev,
		struct gadget_segment *seg)
{
	if (seg->trbs)
		dma_pool_free(pdev->segment_pool, seg->trbs, seg->dma);

	kfree(seg->bounce_buf);
	kfree(seg);
}

static void gadget_free_segments_for_ring(struct phytium_device *pdev,
		struct gadget_segment *first)
{
	struct gadget_segment *seg;

	seg = first->next;

	while (seg != first) {
		struct gadget_segment *next = seg->next;

		gadget_segment_free(pdev, seg);
		seg = next;
	}

	gadget_segment_free(pdev, first);
}

static struct gadget_segment *gadget_segment_alloc(struct phytium_device *pdev,
		unsigned int cycle_state, unsigned int max_packet, gfp_t flags)
{
	struct gadget_segment *seg;
	dma_addr_t dma;
	int i;

	seg = kzalloc(sizeof(struct gadget_segment), flags);
	if (!seg)
		return NULL;

	seg->trbs = dma_pool_zalloc(pdev->segment_pool, flags, &dma);
	if (!seg->trbs) {
		kfree(seg);
		return NULL;
	}

	if (max_packet) {
		seg->bounce_buf = kzalloc(max_packet, flags | GFP_DMA);
		if (!seg->bounce_buf)
			goto free_dma;
	}

	if (cycle_state == 0) {
		for (i = 0; i < TRBS_PER_SEGMENT; i++)
			seg->trbs[i].link.control |= cpu_to_le32(TRB_CYCLE);
	}

	seg->dma = dma;
	seg->next = NULL;

	return seg;

free_dma:
	dma_pool_free(pdev->segment_pool, seg->trbs, dma);
	kfree(seg);

	return NULL;
}

static int gadget_alloc_segments_for_ring(struct phytium_device *pdev,
		struct gadget_segment **first, struct gadget_segment **last,
		unsigned int num_segs, unsigned int cycle_state,
		enum gadget_ring_type type, unsigned int max_packet,
		gfp_t flags)
{
	struct gadget_segment *prev;

	prev = gadget_segment_alloc(pdev, cycle_state, max_packet, flags);
	if (!prev)
		return -ENOMEM;

	num_segs--;
	*first = prev;

	while (num_segs > 0) {
		struct gadget_segment *next;

		next = gadget_segment_alloc(pdev, cycle_state,
				max_packet, flags);
		if (!next) {
			gadget_free_segments_for_ring(pdev, *first);
			return -ENOMEM;
		}

		gadget_link_segments(pdev, prev, next, type);

		prev = next;
		num_segs--;
	}

	gadget_link_segments(pdev, prev, *first, type);
	*last = prev;

	return 0;
}

void gadget_initialize_ring_info(struct gadget_ring *ring)
{
	ring->enqueue = ring->first_seg->trbs;
	ring->enq_seg = ring->first_seg;
	ring->dequeue = ring->enqueue;
	ring->deq_seg = ring->first_seg;

	ring->cycle_state = 1;
	ring->num_trbs_free = ring->num_segs * (TRBS_PER_SEGMENT - 1) - 1;
}

static struct gadget_ring *gadget_ring_alloc(struct phytium_device *pdev,
		unsigned int num_segs, enum gadget_ring_type type,
		unsigned int max_packet, gfp_t flags)
{
	struct gadget_ring *ring;
	int ret;

	ring = kzalloc(sizeof(struct gadget_ring), flags);
	if (!ring)
		return NULL;

	ring->num_segs = num_segs;
	ring->bounce_buf_len = max_packet;
	INIT_LIST_HEAD(&ring->td_list);
	ring->type = type;

	if (num_segs == 0)
		return ring;

	ret = gadget_alloc_segments_for_ring(pdev, &ring->first_seg,
			&ring->last_seg, num_segs, 1, type,
			max_packet, flags);
	if (ret)
		goto fail;

	if (type != TYPE_EVENT)
		ring->last_seg->trbs[TRBS_PER_SEGMENT - 1].link.control |=
			cpu_to_le32(LINK_TOGGLE);

	gadget_initialize_ring_info(ring);

	return ring;

fail:
	kfree(ring);

	return NULL;
}

static int gadget_insert_segment_mapping(struct radix_tree_root *addr_map,
		struct gadget_ring *ring, struct gadget_segment *seg, gfp_t mem_flags)
{
	unsigned long key;
	int ret;

	key = (unsigned long)(seg->dma >> TRB_SEGMENT_SHIFT);

	if (radix_tree_lookup(addr_map, key))
		return 0;

	ret = radix_tree_maybe_preload(mem_flags);
	if (ret)
		return ret;

	ret = radix_tree_insert(addr_map, key, ring);
	radix_tree_preload_end();

	return ret;
}

static void gadget_remove_segment_mapping(struct radix_tree_root *addr_map,
		struct gadget_segment *seg)
{
	unsigned long key;

	key = (unsigned long)(seg->dma >> TRB_SEGMENT_SHIFT);
	if (radix_tree_lookup(addr_map, key))
		radix_tree_delete(addr_map, key);
}

static void gadget_remove_stream_mapping(struct gadget_ring *ring)
{
	struct gadget_segment *seg;

	seg = ring->first_seg;
	do {
		gadget_remove_segment_mapping(ring->trb_address_map, seg);
		seg = seg->next;
	} while (seg != ring->first_seg);
}

static void gadget_ring_free(struct phytium_device *pdev,
		struct gadget_ring *ring)
{
	if (!ring)
		return;

	if (ring->first_seg) {
		if (ring->type == TYPE_STREAM)
			gadget_remove_stream_mapping(ring);

		gadget_free_segments_for_ring(pdev, ring->first_seg);
	}

	kfree(ring);
}

static void gadget_free_stream_ctx(struct phytium_device *pdev,
		struct gadget_ep *pep)
{
	dma_pool_free(pdev->device_pool, pep->stream_info.stream_ctx_array,
			pep->stream_info.ctx_array_dma);
}

static void gadget_free_stream_info(struct phytium_device *pdev,
		struct gadget_ep *pep)
{
	struct gadget_stream_info *stream_info = &pep->stream_info;
	struct gadget_ring *cur_ring;
	int cur_stream;

	if (!(pep->ep_state & EP_HAS_STREAMS))
		return;

	for (cur_stream = 1; cur_stream < stream_info->num_streams;
			cur_stream++) {
		cur_ring = stream_info->stream_rings[cur_stream];
		if (cur_ring) {
			gadget_ring_free(pdev, cur_ring);
			stream_info->stream_rings[cur_stream] = NULL;
		}
	}

	if (stream_info->stream_ctx_array)
		gadget_free_stream_ctx(pdev, pep);

	kfree(stream_info->stream_rings);
	pep->ep_state &= ~EP_HAS_STREAMS;
}

void gadget_free_endpoint_rings(struct phytium_device *pdev,
		struct gadget_ep *pep)
{
	gadget_ring_free(pdev, pep->ring);
	pep->ring = NULL;
	gadget_free_stream_info(pdev, pep);
}

static int gadget_alloc_erst(struct phytium_device *pdev,
		struct gadget_ring *evt_ring,
		struct gadget_erst *erst)
{
	struct gadget_erst_entry *entry;
	struct gadget_segment *seg;
	unsigned int val;
	size_t size;

	size = sizeof(struct gadget_erst_entry) * evt_ring->num_segs;
	erst->entries = dma_alloc_coherent(pdev->dev, size,
			&erst->erst_dma_addr, GFP_KERNEL);
	if (!erst->entries)
		return -ENOMEM;

	erst->num_entries = evt_ring->num_segs;
	seg = evt_ring->first_seg;
	for (val = 0; val < evt_ring->num_segs; val++) {
		entry = &erst->entries[val];
		entry->seg_addr = cpu_to_le64(seg->dma);
		entry->seg_size = cpu_to_le32(TRBS_PER_SEGMENT);
		entry->rsvd = 0;
		seg = seg->next;
	}

	return 0;
}

static void gadget_free_erst(struct phytium_device *pdev,
		struct gadget_erst *erst)
{
	size_t size = sizeof(struct gadget_erst_entry) * (erst->num_entries);
	struct device *dev = pdev->dev;

	if (erst->entries)
		dma_free_coherent(dev, size, erst->entries,
				erst->erst_dma_addr);

	erst->entries = NULL;
}

static void gadget_set_event_deq(struct phytium_device *pdev)
{
	dma_addr_t deq;
	u64 temp;

	deq = gadget_trb_virt_to_dma(pdev->event_ring->deq_seg,
			pdev->event_ring->dequeue);

	temp = lo_hi_readq(&pdev->ir_set->erst_dequeue);
	temp &= ERST_PTR_MASK;
	temp &= ~ERST_EHB;
	lo_hi_writeq(((u64)deq & (u64)~ERST_PTR_MASK) | temp,
			&pdev->ir_set->erst_dequeue);
}

static void gadget_add_in_port(struct phytium_device *pdev,
		struct gadget_port *port, __le32 __iomem *addr)
{
	u32 temp, port_offset, port_count;

	temp = readl(addr);
	port->maj_rev = GADGET_EXT_PORT_MAJOR(temp);
	port->min_rev = GADGET_EXT_PORT_MINOR(temp);

	temp = readl(addr + 2);
	port_offset = GADGET_EXT_PORT_OFF(temp);
	port_count = GADGET_EXT_PORT_COUNT(temp);

	port->port_num = port_offset;
	port->exist = 1;
}

static int gadget_setup_port_arrays(struct phytium_device *pdev)
{
	void __iomem *base;
	u32 offset;
	int i;

	base = &pdev->cap_regs->hc_capbase;
	offset = phytium_gadget_find_next_ext_cap(base, 0,
			EXT_CAP_CFG_DEV_20PORT_CAP_ID);
	pdev->port20_regs = base + offset;

	offset = phytium_gadget_find_next_ext_cap(base, 0,
			D_XEC_CFG_3XPORT_CAP);
	pdev->port3x_regs = base + offset;

	offset = 0;
	base = &pdev->cap_regs->hc_capbase;

	for (i = 0; i < 2; i++) {
		u32 temp;

		offset = phytium_gadget_find_next_ext_cap(base, offset,
				EXT_CAPS_PROTOCOL);
		temp = readl(base + offset);

		if (GADGET_EXT_PORT_MAJOR(temp) == 0x03 &&
				!pdev->usb3_port.port_num)
			gadget_add_in_port(pdev, &pdev->usb3_port,
					base + offset);

		if (GADGET_EXT_PORT_MAJOR(temp) == 0x02 &&
				!pdev->usb2_port.port_num)
			gadget_add_in_port(pdev, &pdev->usb2_port,
					base + offset);
	}

	pdev->usb2_port.regs = (struct gadget_port_regs __iomem *)
		(&pdev->op_regs->port_reg_base + NUM_PORT_REGS * (pdev->usb2_port.port_num - 1));

	pdev->usb3_port.regs = (struct gadget_port_regs __iomem *)
		(&pdev->op_regs->port_reg_base + NUM_PORT_REGS * (pdev->usb3_port.port_num - 1));

	return 0;
}

static int gadget_init_device_ctx(struct phytium_device *pdev)
{
	int size = HCC_64BYTE_CONTEXT(pdev->hcc_params) ? 2048 : 1024;

	pdev->out_ctx.type = GADGET_CTX_TYPE_DEVICE;
	pdev->out_ctx.size = size;
	pdev->out_ctx.ctx_size = CTX_SIZE(pdev->hcc_params);
	pdev->out_ctx.bytes = dma_pool_zalloc(pdev->device_pool, GFP_ATOMIC,
			&pdev->out_ctx.dma);
	if (!pdev->out_ctx.bytes)
		return -ENOMEM;

	pdev->in_ctx.type = GADGET_CTX_TYPE_INPUT;
	pdev->in_ctx.ctx_size = pdev->out_ctx.ctx_size;
	pdev->in_ctx.size = size + pdev->out_ctx.ctx_size;
	pdev->in_ctx.bytes = dma_pool_zalloc(pdev->device_pool, GFP_ATOMIC,
			&pdev->in_ctx.dma);

	if (!pdev->in_ctx.bytes) {
		dma_pool_free(pdev->device_pool, pdev->out_ctx.bytes,
				pdev->out_ctx.dma);
		return -ENOMEM;
	}

	return 0;
}

static int gadget_alloc_priv_device(struct phytium_device *pdev)
{
	int ret;

	ret = gadget_init_device_ctx(pdev);
	if (ret)
		return ret;

	pdev->eps[0].ring = gadget_ring_alloc(pdev, 2, TYPE_CTRL, 0,
			GFP_ATOMIC);
	if (!pdev->eps[0].ring)
		goto fail;

	pdev->dcbaa->dev_context_ptrs[1] = cpu_to_le64(pdev->out_ctx.dma);
	pdev->cmd.in_ctx = &pdev->in_ctx;

	return 0;

fail:
	dma_pool_free(pdev->device_pool, pdev->out_ctx.bytes,
			pdev->out_ctx.dma);
	dma_pool_free(pdev->device_pool, pdev->in_ctx.bytes,
			pdev->in_ctx.dma);

	return ret;
}

int gadget_mem_init(void *data)
{
	unsigned int val;
	u32 page_size;
	dma_addr_t dma;
	int ret = -ENOMEM;
	u64 val_64;
	struct phytium_device *pdev = (struct phytium_device *)data;

	if (!pdev)
		return 0;

	page_size = 1 << 12;

	val = readl(&pdev->op_regs->config_reg);
	val |= ((val & ~MAX_DEVS) | GADGET_DEV_MAX_SLOTS) | CONFIG_U3E;
	writel(val, &pdev->op_regs->config_reg);

	pdev->dcbaa = dma_alloc_coherent(pdev->dev, sizeof(*pdev->dcbaa),
			&dma, GFP_KERNEL);
	if (!pdev->dcbaa)
		return -ENOMEM;

	pdev->dcbaa->dma = dma;
	lo_hi_writeq(dma, &pdev->op_regs->dcbaa_ptr);

	pdev->segment_pool = dma_pool_create("gadget input/output contexts",
				pdev->dev, TRB_SEGMENT_SIZE, TRB_SEGMENT_SIZE,
				page_size);
	if (!pdev->segment_pool)
		goto release_dcbaa;

	pdev->device_pool = dma_pool_create("gadget input/output contexts",
				pdev->dev, GADGET_CTX_SIZE, 64, page_size);
	if (!pdev->device_pool)
		goto destroy_segment_pool;

	pdev->cmd_ring = gadget_ring_alloc(pdev, 1, TYPE_COMMAND, 0,
			GFP_KERNEL);
	if (!pdev->cmd_ring)
		goto destroy_device_pool;

	val_64 = lo_hi_readq(&pdev->op_regs->cmd_ring);
	val_64 = (val_64 & (u64)CMD_RING_RSVD_BITS) |
		(pdev->cmd_ring->first_seg->dma & (u64)~CMD_RING_RSVD_BITS) |
		pdev->cmd_ring->cycle_state;
	lo_hi_writeq(val_64, &pdev->op_regs->cmd_ring);

	val = readl(&pdev->cap_regs->db_off);
	val &= DBOFF_MASK;
	pdev->dba = (void __iomem *)pdev->cap_regs + val;
	pdev->ir_set = &pdev->run_regs->ir_set[0];

	pdev->event_ring = gadget_ring_alloc(pdev, ERST_NUM_SEGS, TYPE_EVENT,
				0, GFP_KERNEL);
	if (!pdev->event_ring)
		goto free_cmd_ring;

	ret = gadget_alloc_erst(pdev, pdev->event_ring, &pdev->erst);
	if (ret)
		goto free_event_ring;

	val = readl(&pdev->ir_set->erst_size);
	val &= ERST_SIZE_MASK;
	val |= ERST_NUM_SEGS;
	writel(val, &pdev->ir_set->erst_size);

	val_64 = lo_hi_readq(&pdev->ir_set->erst_base);
	val_64 &= ERST_PTR_MASK;
	val_64 |= (pdev->erst.erst_dma_addr & (u64)~ERST_PTR_MASK);
	lo_hi_writeq(val_64, &pdev->ir_set->erst_base);

	gadget_set_event_deq(pdev);

	ret = gadget_setup_port_arrays(pdev);
	if (ret)
		goto free_erst;

	ret = gadget_alloc_priv_device(pdev);
	if (ret) {
		dev_err(pdev->dev, "gadget_alloc_priv_device failed\n");
		goto free_erst;
	}

	return 0;
free_erst:
	gadget_free_erst(pdev, &pdev->erst);
free_event_ring:
	gadget_ring_free(pdev, pdev->event_ring);
free_cmd_ring:
	gadget_ring_free(pdev, pdev->cmd_ring);
destroy_device_pool:
	dma_pool_destroy(pdev->device_pool);
destroy_segment_pool:
	dma_pool_destroy(pdev->segment_pool);

release_dcbaa:
	dma_free_coherent(pdev->dev, sizeof(*pdev->dcbaa), pdev->dcbaa,
			pdev->dcbaa->dma);

	gadget_reset(pdev);

	return ret;
}

static u32 gadget_get_endpoint_type(const struct usb_endpoint_descriptor *desc)
{
	int in;

	in = usb_endpoint_dir_in(desc);

	switch (usb_endpoint_type(desc)) {
	case USB_ENDPOINT_XFER_CONTROL:
		return CTRL_EP;
	case USB_ENDPOINT_XFER_BULK:
		return in ? BULK_IN_EP : BULK_OUT_EP;
	case USB_ENDPOINT_XFER_ISOC:
		return in ? ISOC_IN_EP : ISOC_OUT_EP;
	case USB_ENDPOINT_XFER_INT:
		return in ? INT_IN_EP : INT_OUT_EP;
	}

	return 0;
}

static unsigned int gadget_parse_exponent_interval(struct usb_gadget *g,
		struct gadget_ep *pep)
{
	unsigned int interval;

	interval = clamp_val(pep->endpoint.desc->bInterval, 1, 16) - 1;
	if (interval != pep->endpoint.desc->bInterval - 1)
		dev_warn(&g->dev, "ep %s -rounding interval to %d %s frames\n",
			pep->name, 1 << interval,
			g->speed == USB_SPEED_FULL ? "" : "micro");

	if (g->speed == USB_SPEED_FULL)
		interval += 3;

	if (interval > 12)
		interval = 12;

	return interval;
}

static unsigned int gadget_microframes_to_exponent(struct usb_gadget *g,
		struct gadget_ep *pep, unsigned int desc_interval,
		unsigned int min_exponent, unsigned int max_exponent)
{
	unsigned int interval;

	interval = fls(desc_interval) - 1;

	return clamp_val(interval, min_exponent, max_exponent);
}

static u32 gadget_get_max_esit_payload(struct usb_gadget *g,
		struct gadget_ep *pep)
{
	int max_packet;
	int max_burst;

	if (usb_endpoint_xfer_control(pep->endpoint.desc) ||
	    usb_endpoint_xfer_bulk(pep->endpoint.desc))
		return 0;

	if (g->speed >= USB_SPEED_SUPER_PLUS &&
			USB_SS_SSP_ISOC_COMP(pep->endpoint.desc->bmAttributes))
		return le16_to_cpu(pep->endpoint.comp_desc->wBytesPerInterval);
	else if (g->speed >= USB_SPEED_SUPER)
		return le16_to_cpu(pep->endpoint.comp_desc->wBytesPerInterval);

	max_packet = usb_endpoint_maxp(pep->endpoint.desc);
	max_burst = usb_endpoint_maxp_mult(pep->endpoint.desc);

	return max_packet * max_burst;
}

static unsigned int gadget_get_endpoint_interval(struct usb_gadget *g,
		struct gadget_ep *pep)
{
	unsigned int interval = 0;

	switch (g->speed) {
	case USB_SPEED_HIGH:
	case USB_SPEED_SUPER:
	case USB_SPEED_SUPER_PLUS:
		if (usb_endpoint_xfer_int(pep->endpoint.desc) ||
		    usb_endpoint_xfer_isoc(pep->endpoint.desc))
			interval = gadget_parse_exponent_interval(g, pep);
		break;
	case USB_SPEED_FULL:
		if (usb_endpoint_xfer_isoc(pep->endpoint.desc))
			interval = gadget_parse_exponent_interval(g, pep);
		else if (usb_endpoint_xfer_int(pep->endpoint.desc)) {
			interval = pep->endpoint.desc->bInterval << 3;
			interval = gadget_microframes_to_exponent(g, pep,
					interval, 3, 10);
		}
		break;
	default:
		break;
	}

	return interval;
}

static u32 gadget_get_endpoint_mult(struct usb_gadget *g, struct gadget_ep *pep)
{
	if (g->speed < USB_SPEED_SUPER ||
		!usb_endpoint_xfer_isoc(pep->endpoint.desc))
		return 0;

	return pep->endpoint.comp_desc->bmAttributes;
}

static u32 gadget_get_endpoint_max_burst(struct usb_gadget *g,
		struct gadget_ep *pep)
{
	if (g->speed >= USB_SPEED_SUPER)
		return pep->endpoint.comp_desc->bMaxBurst;

	if (g->speed == USB_SPEED_HIGH &&
		(usb_endpoint_xfer_isoc(pep->endpoint.desc) ||
		 usb_endpoint_xfer_int(pep->endpoint.desc)))
		return usb_endpoint_maxp_mult(pep->endpoint.desc) - 1;

	return 0;
}

int gadget_endpoint_init(struct phytium_device *pdev,
		struct gadget_ep *pep, gfp_t mem_flags)
{
	enum gadget_ring_type ring_type;
	struct gadget_ep_ctx *ep_ctx;
	unsigned int err_count = 0;
	unsigned int avg_trb_len, max_packet, max_burst, interval, mult;
	u32 max_esit_payload, endpoint_type;
	int ret;

	ep_ctx = pep->in_ctx;

	endpoint_type = gadget_get_endpoint_type(pep->endpoint.desc);
	if (!endpoint_type)
		return -EINVAL;

	ring_type = usb_endpoint_type(pep->endpoint.desc);

	max_esit_payload = gadget_get_max_esit_payload(&pdev->gadget, pep);
	interval = gadget_get_endpoint_interval(&pdev->gadget, pep);
	mult = gadget_get_endpoint_mult(&pdev->gadget, pep);
	max_packet = usb_endpoint_maxp(pep->endpoint.desc);
	max_burst = gadget_get_endpoint_max_burst(&pdev->gadget, pep);
	avg_trb_len = max_esit_payload;

	if (!usb_endpoint_xfer_isoc(pep->endpoint.desc))
		err_count = 3;

	if (usb_endpoint_xfer_bulk(pep->endpoint.desc) &&
		pdev->gadget.speed == USB_SPEED_HIGH)
		max_packet = 512;

	if (usb_endpoint_xfer_control(pep->endpoint.desc))
		avg_trb_len = 8;

	pep->ring = gadget_ring_alloc(pdev, 2, ring_type, max_packet,
					mem_flags);
	if (!pep->ring)
		return -ENOMEM;

	pep->skip = false;

	ep_ctx->ep_info = cpu_to_le32(EP_INTERVAL(interval) | EP_MULT(mult) |
		EP_MAX_ESIT_PAYLOAD_HI(max_esit_payload));

	ep_ctx->ep_info2 = cpu_to_le32(EP_TYPE(endpoint_type) |
			MAX_PACKET(max_packet) | MAX_BURST(max_burst) |
			ERROR_COUNT(err_count));

	ep_ctx->deq = cpu_to_le64(pep->ring->first_seg->dma |
			pep->ring->cycle_state);

	ep_ctx->tx_info = cpu_to_le32(EP_MAX_ESIT_PAYLOAD_LO(max_esit_payload) |
			EP_AVG_TRB_LENGTH(avg_trb_len));

	if (usb_endpoint_xfer_bulk(pep->endpoint.desc) &&
		pdev->gadget.speed > USB_SPEED_HIGH) {
		ret = gadget_alloc_streams(pdev, pep);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int gadget_update_stream_segment_mapping(struct radix_tree_root *trb_address_map,
			struct gadget_ring *ring, struct gadget_segment *first_seg,
			struct gadget_segment *last_seg, gfp_t mem_flags)
{
	struct gadget_segment *failed_seg;
	struct gadget_segment *seg;
	int ret;

	seg = first_seg;
	do {
		ret = gadget_insert_segment_mapping(trb_address_map, ring, seg, mem_flags);
		if (ret)
			goto remove_streams;

		if (seg == last_seg)
			return 0;

		seg = seg->next;
	} while (seg != first_seg);

	return 0;

remove_streams:
	failed_seg = seg;
	seg = first_seg;
	do {
		gadget_remove_segment_mapping(trb_address_map, seg);
		if (seg == failed_seg)
			return ret;
		seg = seg->next;
	} while (seg != first_seg);

	return ret;
}

static void gadget_link_rings(struct phytium_device *pdev, struct gadget_ring *ring,
		struct gadget_segment *first, struct gadget_segment *last, unsigned int num_segs)
{
	struct gadget_segment *next;

	if (!ring || !first || !last)
		return;

	next = ring->enq_seg->next;
	gadget_link_segments(pdev, ring->enq_seg, first, ring->type);
	gadget_link_segments(pdev, last, next, ring->type);
	ring->num_segs += num_segs;
	ring->num_trbs_free = (TRBS_PER_SEGMENT - 1) * num_segs;

	if (ring->type != TYPE_EVENT && ring->enq_seg == ring->last_seg) {
		ring->last_seg->trbs[TRBS_PER_SEGMENT - 1].link.control &=
			~cpu_to_le32(LINK_TOGGLE);
		last->trbs[TRBS_PER_SEGMENT - 1].link.control |= cpu_to_le32(LINK_TOGGLE);
		ring->last_seg = last;
	}
}

int gadget_ring_expansion(struct phytium_device *pdev, struct gadget_ring *ring,
		unsigned int num_trbs, gfp_t flags)
{
	unsigned int num_segs_needed;
	struct gadget_segment *first;
	struct gadget_segment *last;
	unsigned int num_segs;
	int ret;

	num_segs_needed = (num_trbs + (TRBS_PER_SEGMENT - 1) - 1)/
		(TRBS_PER_SEGMENT - 1);

	num_segs = max(ring->num_segs, num_segs_needed);

	ret = gadget_alloc_segments_for_ring(pdev, &first, &last, num_segs,
			ring->cycle_state, ring->type, ring->bounce_buf_len, flags);
	if (ret)
		return -ENOMEM;

	if (ring->type == TYPE_STREAM)
		ret = gadget_update_stream_segment_mapping(ring->trb_address_map, ring,
				first, last, flags);
	if (ret) {
		gadget_free_segments_for_ring(pdev, first);
		return ret;
	}

	gadget_link_rings(pdev, ring, first, last, num_segs);

	return 0;
}

void gadget_endpoint_zero(struct phytium_device *pdev, struct gadget_ep *pep)
{
	pep->in_ctx->ep_info = 0;
	pep->in_ctx->ep_info2 = 0;
	pep->in_ctx->deq = 0;
	pep->in_ctx->tx_info = 0;
}

static void gadget_free_priv_device(struct phytium_device *pdev)
{
	pdev->dcbaa->dev_context_ptrs[1] = 0;

	gadget_free_endpoint_rings(pdev, &pdev->eps[0]);

	if (pdev->in_ctx.bytes)
		dma_pool_free(pdev->device_pool, pdev->in_ctx.bytes, pdev->in_ctx.dma);

	if (pdev->out_ctx.bytes)
		dma_pool_free(pdev->device_pool, pdev->out_ctx.bytes, pdev->out_ctx.dma);

	pdev->in_ctx.bytes = NULL;
	pdev->out_ctx.bytes = NULL;
}

void gadget_mem_cleanup(void *data)
{
	struct phytium_device *pdev = (struct phytium_device *)data;
	struct device *dev = pdev->dev;

	gadget_free_priv_device(pdev);
	gadget_free_erst(pdev, &pdev->erst);

	if (pdev->event_ring)
		gadget_ring_free(pdev, pdev->event_ring);
	pdev->event_ring = NULL;

	if (pdev->cmd_ring)
		gadget_ring_free(pdev, pdev->cmd_ring);
	pdev->cmd_ring = NULL;

	dma_pool_destroy(pdev->segment_pool);
	pdev->segment_pool = NULL;

	dma_pool_destroy(pdev->device_pool);
	pdev->device_pool = NULL;

	dma_free_coherent(dev, sizeof(*pdev->dcbaa), pdev->dcbaa, pdev->dcbaa->dma);
	pdev->dcbaa = NULL;

	pdev->usb2_port.exist = 0;
	pdev->usb3_port.exist = 0;
	pdev->usb2_port.port_num = 0;
	pdev->usb3_port.port_num = 0;
	pdev->active_port = NULL;
}

int gadget_setup_addressable_priv_dev(void *data)
{
	struct phytium_device *pdev = (struct phytium_device *)data;
	struct gadget_slot_ctx *slot_ctx;
	struct gadget_ep_ctx *ep0_ctx;
	u32 max_packets, port;

	ep0_ctx = gadget_get_ep_ctx(&pdev->in_ctx, 0);
	slot_ctx = gadget_get_slot_ctx(&pdev->in_ctx);

	slot_ctx->dev_info |= cpu_to_le32(LAST_CTX(1));

	switch (pdev->gadget.speed) {
	case USB_SPEED_SUPER_PLUS:
		slot_ctx->dev_info |= cpu_to_le32(SLOT_SPEED_SSP);
		max_packets = MAX_PACKET(512);
		break;
	case USB_SPEED_SUPER:
		slot_ctx->dev_info |= cpu_to_le32(SLOT_SPEED_SS);
		max_packets = MAX_PACKET(512);
		break;
	case USB_SPEED_HIGH:
		slot_ctx->dev_info |= cpu_to_le32(SLOT_SPEED_HS);
		max_packets = MAX_PACKET(64);
		break;
	case USB_SPEED_FULL:
		slot_ctx->dev_info |= cpu_to_le32(SLOT_SPEED_FS);
		max_packets = MAX_PACKET(64);
		break;
	default:
		return -EINVAL;
	}

	port = DEV_PORT(pdev->active_port->port_num);
	slot_ctx->dev_port |= cpu_to_le32(port);
	slot_ctx->dev_state = cpu_to_le32((pdev->device_address & DEV_ADDR_MASK));
	ep0_ctx->tx_info = cpu_to_le32(EP_AVG_TRB_LENGTH(0x8));
	ep0_ctx->ep_info2 = cpu_to_le32(EP_TYPE(CTRL_EP));
	ep0_ctx->ep_info2 |= cpu_to_le32(MAX_BURST(0) | ERROR_COUNT(3) | max_packets);
	ep0_ctx->deq = cpu_to_le64(pdev->eps[0].ring->first_seg->dma |
			pdev->eps[0].ring->cycle_state);

	return 0;
}

void gadget_copy_ep0_dequeue_into_input_ctx(void *data)
{
	struct phytium_device *pdev = (struct phytium_device *)data;
	struct gadget_ring *ep_ring = pdev->eps[0].ring;
	struct gadget_ep_ctx *ep0_ctx = pdev->eps[0].in_ctx;
	dma_addr_t dma;

	dma = gadget_trb_virt_to_dma(ep_ring->enq_seg, ep_ring->enqueue);
	ep0_ctx->deq = cpu_to_le64(dma | ep_ring->cycle_state);
}

struct gadget_ring *gadget_dma_to_transfer_ring(struct gadget_ep *pep, u64 address)
{
	if (pep->ep_state & EP_HAS_STREAMS)
		return radix_tree_lookup(&pep->stream_info.trb_address_map,
				address >> TRB_SEGMENT_SHIFT);

	return pep->ring;
}

// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium USB DRD Driver.
 *
 * Copyright (C) 2023 - 2024 Phytium.
 */

#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/dma-mapping.h>
#include <linux/io-64-nonatomic-lo-hi.h>

#include "gadget.h"
#include "otg.h"
#include "core.h"
#include "mem.h"
#include "ring.h"

#define GADGET_IF_EP_EXIST(pdev, ep_num, dir) \
	(readl(&(pdev)->rev_cap->ep_supported) & \
	 (BIT(ep_num) << ((dir) ? 0 : 16)))

#define STREAM_LOG_STREAMS 4
#define STREAM_NUM_STREAMS BIT(STREAM_LOG_STREAMS)

#define EP0_SETUP_SIZE 512
#define RTL_REV_CAP 0xc4

#define CFG_3XPORT_U1_PIPE_CLK_GATE_EN BIT(0)

#define HCC_PARAMS_OFFSET 0x10
#define HCC_EXT_CAPS(p) (((p) & GENMASK(31, 16)) >> 16)
#define EXT_CAPS_ID(p) (((p) >> 0) & GENMASK(7, 0))
#define EXT_CAPS_NEXT(p) (((p) >> 8) & GENMASK(7, 0))

#define MAX_HALT_USEC (16 * 1000) //16ms

#define D_XEC_PER_REGS_CAP 0xC8
#define REG_CHICKEN_BITS_2_OFFSET 0x48
#define CHICKEN_XDMA_2_TP_CACHE_DIS BIT(28)

#define XBUF_CAP_ID	0xCB
#define XBUF_RX_TAG_MASK_0_OFFSET 0x1C
#define XBUF_RX_TAG_MASK_1_OFFSET 0x24
#define XBUF_TX_CMD_OFFSET	0x2c

#define HCS_ENDPOINTS_MASK	GENMASK(7, 0)
#define HCS_ENDPOINTS(p)	(((p) & HCS_ENDPOINTS_MASK) >> 0)

#define to_gadget_ep(ep) (container_of(ep, struct gadget_ep, endpoint))
#define to_gadget_request(r) (container_of(r, struct gadget_request, request))
#define gadget_to_device(g)	(container_of(g, struct phytium_device, gadget))
#define request_to_gadget_request(r) (container_of(r, struct gadget_request, request))

#define IMOD_INTERVAL_MASK		GENMASK(15, 0)
#define IMOD_COUNTER_MASK		GENMASK(31, 16)
#define IMOD_DEFAULT_INTERVAL	0

#define CFG_3XPORT_SSP_SUPPORT	BIT(31)
#define PORT_REG6_FORCE_FS		BIT(0)
#define PORT_REG6_L1_L0_HW_EN	BIT(1)

#define GADGET_MAX_HALT_USEC (16 * 1000)
#define GADGET_DEFAULT_BESL	0

unsigned int gadget_port_speed(unsigned int port_status)
{
	if (DEV_SUPERSPEEDPLUS(port_status))
		return USB_SPEED_SUPER_PLUS;
	else if (DEV_SUPERSPEED(port_status))
		return USB_SPEED_SUPER;
	else if (DEV_HIGHSPEED(port_status))
		return USB_SPEED_HIGH;
	else if (DEV_FULLSPEED(port_status))
		return USB_SPEED_FULL;

	return USB_SPEED_UNKNOWN;
}


u32 gadget_port_state_to_neutral(u32 state)
{
	return (state & GADGET_PORT_RO) | (state & GADGET_PORT_RWS);
}

int gadget_alloc_streams(struct phytium_device *pdev, struct gadget_ep *pep)
{
	return 0;
}

static unsigned int
	gadget_get_endpoint_index(const struct usb_endpoint_descriptor *desc)
{
	unsigned int index = (unsigned int)usb_endpoint_num(desc);

	if (usb_endpoint_xfer_control(desc))
		return index * 2;

	return (index * 2) + (usb_endpoint_dir_in(desc) ? 1 : 0) - 1;
}

static unsigned int
	gadget_get_endpoint_flag(const struct usb_endpoint_descriptor *desc)
{
	return 1 << (gadget_get_endpoint_index(desc) + 1);
}

static void gadget_set_chicken_bits_2(struct phytium_device *pdev, u32 bit)
{
	__le32 __iomem *reg;
	void __iomem *base;
	u32 offset = 0;

	base = &pdev->cap_regs->hc_capbase;
	offset = phytium_gadget_find_next_ext_cap(base, offset,
			D_XEC_PER_REGS_CAP);
	reg = base + offset + REG_CHICKEN_BITS_2_OFFSET;

	bit = readl(reg) | bit;
	writel(bit, reg);
}

bool gadget_last_trb_on_seg(struct gadget_segment *seg, union gadget_trb *trb)
{
	return trb == &seg->trbs[TRBS_PER_SEGMENT - 1];
}

bool gadget_last_trb_on_ring(struct gadget_ring *ring, struct gadget_segment *seg,
		union gadget_trb *trb)
{
	return gadget_last_trb_on_seg(seg, trb) && (seg->next == ring->first_seg);
}

int gadget_wait_for_cmd_compl(struct phytium_device *pdev)
{
	struct gadget_segment *event_deq_seg;
	union gadget_trb *cmd_trb;
	dma_addr_t cmd_deq_dma;
	union gadget_trb *event;
	u32 cycle_state;
	int ret, val;
	u64 cmd_dma;
	u32 flags;

	cmd_trb = pdev->cmd.command_trb;
	pdev->cmd.status = 0;

	ret = readl_poll_timeout_atomic(&pdev->op_regs->cmd_ring, val,
		!CMD_RING_BUSY(val), 1, GADGET_CMD_TIMEOUT);
	if (ret) {
		dev_err(pdev->dev, "ERR: Timeout while waiting for command\n");
		pdev->gadget_state = GADGET_STATE_DYING;
		return -ETIMEDOUT;
	}

	event = pdev->event_ring->dequeue;
	event_deq_seg = pdev->event_ring->deq_seg;
	cycle_state = pdev->event_ring->cycle_state;

	cmd_deq_dma = gadget_trb_virt_to_dma(pdev->cmd_ring->deq_seg, cmd_trb);
	if (!cmd_deq_dma)
		return -EINVAL;

	while (1) {
		flags = le32_to_cpu(event->event_cmd.flags);

		if ((flags & TRB_CYCLE) != cycle_state)
			return -EINVAL;

		cmd_dma = le64_to_cpu(event->event_cmd.cmd_trb);

		if (TRB_FIELD_TO_TYPE(flags) != TRB_COMPLETION ||
			cmd_dma != (u64)cmd_deq_dma) {
			if (!gadget_last_trb_on_seg(event_deq_seg, event)) {
				event++;
				continue;
			}

			if (gadget_last_trb_on_ring(pdev->event_ring, event_deq_seg, event))
				cycle_state ^= 1;

			event_deq_seg = event_deq_seg->next;
			event = event_deq_seg->trbs;
			continue;
		}

		pdev->cmd.status = GET_COMP_CODE(le32_to_cpu(event->event_cmd.status));
		if (pdev->cmd.status == COMP_SUCCESS)
			return 0;

		return pdev->cmd.status;
	}
}

static int gadget_configure_endpoint(struct phytium_device *pdev)
{
	int ret;

	gadget_queue_configure_endpoint(pdev, pdev->cmd.in_ctx->dma);
	gadget_ring_cmd_db(pdev);

	ret = gadget_wait_for_cmd_compl(pdev);
	if (ret) {
		dev_err(pdev->dev, "ERR: unexpected command completion code\n");
		return -EINVAL;
	}

	return ret;
}

static void gadget_zero_in_ctx(struct phytium_device *pdev)
{
	struct gadget_input_control_ctx *ctrl_ctx;
	struct gadget_slot_ctx *slot_ctx;
	struct gadget_ep_ctx *ep_ctx;
	int i;

	ctrl_ctx = gadget_get_input_control_ctx(&pdev->in_ctx);

	ctrl_ctx->drop_flags = 0;
	ctrl_ctx->add_flags = 0;
	slot_ctx = gadget_get_slot_ctx(&pdev->in_ctx);
	slot_ctx->dev_info &= cpu_to_le32(~LAST_CTX_MASK);
	slot_ctx->dev_info |= cpu_to_le32(LAST_CTX(1));

	for (i = 1; i < GADGET_ENDPOINTS_NUM; ++i) {
		ep_ctx = gadget_get_ep_ctx(&pdev->in_ctx, i);
		ep_ctx->ep_info = 0;
		ep_ctx->ep_info2 = 0;
		ep_ctx->deq = 0;
		ep_ctx->tx_info = 0;
	}
}

static int gadget_update_eps_configuration(struct phytium_device *pdev,
		struct gadget_ep *pep)
{
	struct gadget_input_control_ctx *ctrl_ctx;
	struct gadget_slot_ctx *slot_ctx;
	int ret = 0;
	int i;
	u32 ep_sts;

	ctrl_ctx = gadget_get_input_control_ctx(&pdev->in_ctx);

	if (ctrl_ctx->add_flags == 0 && ctrl_ctx->drop_flags == 0)
		return 0;

	ctrl_ctx->add_flags |= cpu_to_le32(SLOT_FLAG);
	ctrl_ctx->add_flags &= cpu_to_le32(~EP0_FLAG);
	ctrl_ctx->drop_flags &= cpu_to_le32(~(SLOT_FLAG | EP0_FLAG));

	slot_ctx = gadget_get_slot_ctx(&pdev->in_ctx);
	for (i = GADGET_ENDPOINTS_NUM; i >= 1; i--) {
		__le32 le32 = cpu_to_le32(BIT(i));

		if ((pdev->eps[i - 1].ring && !(ctrl_ctx->drop_flags & le32)) ||
			(ctrl_ctx->add_flags & le32) || i == 1) {
			slot_ctx->dev_info &= cpu_to_le32(~LAST_CTX_MASK);
			slot_ctx->dev_info |= cpu_to_le32(LAST_CTX(i));
			break;
		}
	}

	ep_sts = GET_EP_CTX_STATE(pep->out_ctx);

	if ((ctrl_ctx->add_flags != cpu_to_le32(SLOT_FLAG) &&
			ep_sts == EP_STATE_DISABLED) ||
			(ep_sts != EP_STATE_DISABLED && ctrl_ctx->drop_flags))
		ret = gadget_configure_endpoint(pdev);

	gadget_zero_in_ctx(pdev);

	return ret;
}

static int gadget_ep_enable(struct usb_ep *ep,
		const struct usb_endpoint_descriptor *desc)
{
	struct gadget_input_control_ctx *ctrl_ctx;
	struct phytium_device *pdev;
	struct gadget_ep *pep;
	unsigned long flags;
	u32 added_ctxs;
	int ret;

	if (!ep || !desc || desc->bDescriptorType != USB_DT_ENDPOINT ||
			!desc->wMaxPacketSize)
		return -EINVAL;

	pep = to_gadget_ep(ep);
	pdev = pep->pdev;
	pep->ep_state &= ~EP_UNCONFIGURED;

	if (pep->ep_state & EP_ENABLED) {
		dev_warn(pdev->dev, "%s is already enalbed\n", pep->name);
		return 0;
	}

	spin_lock_irqsave(&pdev->lock, flags);

	added_ctxs = gadget_get_endpoint_flag(desc);
	if (added_ctxs == SLOT_FLAG || added_ctxs == EP0_FLAG) {
		dev_err(pdev->dev, "Bad endpoint number\n");
		ret = -EINVAL;
		goto unlock;
	}

	pep->interval = desc->bInterval ? BIT(desc->bInterval - 1) : 0;

	if (pdev->gadget.speed == USB_SPEED_FULL) {
		if (usb_endpoint_type(desc) == USB_ENDPOINT_XFER_INT)
			pep->interval = desc->bInterval << 3;
		if (usb_endpoint_type(desc) == USB_ENDPOINT_XFER_ISOC)
			pep->interval = BIT(desc->bInterval - 1) << 3;
	}

	if (usb_endpoint_type(desc) == USB_ENDPOINT_XFER_ISOC) {
		if (pep->interval > BIT(12)) {
			dev_err(pdev->dev, "bInterval %d not supported\n",
					desc->bInterval);
			ret = -EINVAL;
			goto unlock;
		}
		gadget_set_chicken_bits_2(pdev, CHICKEN_XDMA_2_TP_CACHE_DIS);
	}

	ret = gadget_endpoint_init(pdev, pep, GFP_ATOMIC);
	if (ret)
		goto unlock;

	ctrl_ctx = gadget_get_input_control_ctx(&pdev->in_ctx);
	ctrl_ctx->add_flags = cpu_to_le32(added_ctxs);
	ctrl_ctx->drop_flags = 0;

	ret = gadget_update_eps_configuration(pdev, pep);
	if (ret) {
		gadget_free_endpoint_rings(pdev, pep);
		goto unlock;
	}

	pep->ep_state |= EP_ENABLED;
	pep->ep_state &= ~EP_STOPPED;

unlock:
	spin_unlock_irqrestore(&pdev->lock, flags);

	return ret;
}

static struct usb_request *gadget_ep_alloc_request(struct usb_ep *ep, gfp_t gfp_flags)
{
	struct gadget_ep *pep = to_gadget_ep(ep);
	struct gadget_request *preq;

	preq = kzalloc(sizeof(*preq), gfp_flags);
	if (!preq)
		return NULL;

	preq->epnum = pep->number;
	preq->pep = pep;

	return &preq->request;
}

static void gadget_ep_free_request(struct usb_ep *ep, struct usb_request *request)
{
	struct gadget_request *preq = to_gadget_request(request);

	kfree(preq);
}


static void gadget_invalidate_ep_events(struct phytium_device *pdev, struct gadget_ep *pep)
{
	struct gadget_segment *segment;
	union gadget_trb *event;
	u32 cycle_state;
	u32 data;

	event = pdev->event_ring->dequeue;
	segment = pdev->event_ring->deq_seg;
	cycle_state = pdev->event_ring->cycle_state;

	while (1) {
		data = le32_to_cpu(event->trans_event.flags);
		if ((data & TRB_CYCLE) != cycle_state)
			break;

		if (TRB_FIELD_TO_TYPE(data) == TRB_TRANSFER &&
			TRB_TO_EP_ID(data) == (pep->idx + 1)) {
			data |= TRB_EVENT_INVALIDATE;
			event->trans_event.flags = cpu_to_le32(data);
		}

		if (gadget_last_trb_on_seg(segment, event)) {
			cycle_state ^= 1;
			segment = pdev->event_ring->deq_seg->next;
			event = segment->trbs;
		} else {
			event++;
		}
	}
}

int ep_dequeue(struct gadget_ep *pep, struct gadget_request *preq)
{
	struct phytium_device *pdev = pep->pdev;
	int ret_stop = 0;
	int ret_rem;

	if (GET_EP_CTX_STATE(pep->out_ctx) == EP_STATE_RUNNING)
		ret_stop = gadget_cmd_stop_ep(pdev, pep);

	ret_rem = gadget_remove_request(pdev, preq, pep);

	return ret_rem ? ret_rem : ret_stop;
}

int gadget_ep_enqueue(struct gadget_ep *pep, struct gadget_request *preq)
{
	struct phytium_device *pdev = pep->pdev;
	struct usb_request *request;
	int ret;

	if (preq->epnum == 0 && !list_empty(&pep->pending_list))
		return -EBUSY;

	request = &preq->request;
	request->actual = 0;
	request->status = -EINPROGRESS;
	preq->direction = pep->direction;
	preq->epnum = pep->number;
	preq->td.drbl = 0;

	ret = usb_gadget_map_request_by_dev(pdev->dev, request, pep->direction);
	if (ret)
		return ret;

	list_add_tail(&preq->list, &pep->pending_list);

	switch (usb_endpoint_type(pep->endpoint.desc)) {
	case USB_ENDPOINT_XFER_CONTROL:
		ret = gadget_queue_ctrl_tx(pdev, preq);
		break;
	case USB_ENDPOINT_XFER_BULK:
	case USB_ENDPOINT_XFER_INT:
		ret = gadget_queue_bulk_tx(pdev, preq);
		break;
	case USB_ENDPOINT_XFER_ISOC:
		ret = gadget_queue_isoc_tx_prepare(pdev, preq);
		break;
	}

	if (ret)
		goto unmap;

	return 0;

unmap:
	usb_gadget_unmap_request_by_dev(pdev->dev, &preq->request, pep->direction);

	list_del(&preq->list);

	return ret;
}

static int gadget_ep_disable(struct usb_ep *ep)
{
	struct gadget_input_control_ctx *ctrl_ctx;
	struct gadget_request *preq;
	struct phytium_device *pdev;
	struct gadget_ep *pep;
	unsigned long flags;
	u32 drop_flag;
	int ret = 0;

	if (!ep)
		return -EINVAL;

	pep = to_gadget_ep(ep);
	pdev = pep->pdev;

	spin_lock_irqsave(&pdev->lock, flags);
	if (!(pep->ep_state & EP_ENABLED)) {
		dev_err(pdev->dev, "%s is already disabled\n", pep->name);
		ret = -EINVAL;
		goto finish;
	}

	pep->ep_state |= EP_DIS_IN_PROGRESS;

	if (!(pep->ep_state & EP_UNCONFIGURED)) {
		gadget_cmd_stop_ep(pdev, pep);
		gadget_cmd_flush_ep(pdev, pep);
	}

	while (!list_empty(&pep->pending_list)) {
		preq = next_request(&pep->pending_list);
		ep_dequeue(pep, preq);
	}

	gadget_invalidate_ep_events(pdev, pep);

	pep->ep_state &= ~EP_DIS_IN_PROGRESS;
	drop_flag = gadget_get_endpoint_flag(pep->endpoint.desc);
	ctrl_ctx = gadget_get_input_control_ctx(&pdev->in_ctx);
	ctrl_ctx->drop_flags = cpu_to_le32(drop_flag);
	ctrl_ctx->add_flags = 0;

	gadget_endpoint_zero(pdev, pep);

	if (!(pep->ep_state & EP_UNCONFIGURED))
		ret = gadget_update_eps_configuration(pdev, pep);

	gadget_free_endpoint_rings(pdev, pep);

	pep->ep_state &= ~(EP_ENABLED | EP_UNCONFIGURED);
	pep->ep_state |= EP_STOPPED;

finish:
	spin_unlock_irqrestore(&pdev->lock, flags);

	return ret;
}

static int gadget_ep_queue(struct usb_ep *ep, struct usb_request *request, gfp_t gfp_flags)
{
	struct gadget_request *preq;
	struct phytium_device *pdev;
	struct gadget_ep *pep;
	unsigned long flags;
	int ret;

	if (!request || !ep)
		return -EINVAL;

	pep = to_gadget_ep(ep);
	pdev = pep->pdev;

	if (!(pep->ep_state & EP_ENABLED)) {
		dev_err(pdev->dev, "%s: can't queue to disabled endpoint\n", pep->name);
		return -EINVAL;
	}

	preq = to_gadget_request(request);
	spin_lock_irqsave(&pdev->lock, flags);
	ret = gadget_ep_enqueue(pep, preq);
	spin_unlock_irqrestore(&pdev->lock, flags);

	return ret;
}

static int gadget_ep_dequeue(struct usb_ep *ep, struct usb_request *request)
{
	struct phytium_device *pdev = NULL;
	struct gadget_ep *pep;
	unsigned long flags;
	int ret = 0;

	if (!request || !ep)
		return -EINVAL;

	pep = to_gadget_ep(ep);
	if (!pep->endpoint.desc) {
		dev_err(pdev->dev, "%s: can't dequeue to disabled endpoint\n", pep->name);
		return -ESHUTDOWN;
	}

	if (!(pep->ep_state & EP_ENABLED))
		return 0;

	spin_lock_irqsave(&pdev->lock, flags);
	ret = ep_dequeue(pep, to_gadget_request(request));
	spin_unlock_irqrestore(&pdev->lock, flags);

	return ret;
}

static int gadget_ep_set_halt(struct usb_ep *ep, int value)
{
	struct gadget_ep *pep = to_gadget_ep(ep);
	struct phytium_device *pdev = pep->pdev;
	struct gadget_request *preq;
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&pdev->lock, flags);

	preq = next_request(&pep->pending_list);
	if (value) {
		if (preq) {
			ret = -EAGAIN;
			goto done;
		}
	}

	ret = gadget_halt_endpoint(pdev, pep, value);

done:
	spin_unlock_irqrestore(&pdev->lock, flags);

	return ret;
}

static int gadget_ep_set_wedge(struct usb_ep *ep)
{
	struct gadget_ep *pep = to_gadget_ep(ep);
	struct phytium_device *pdev = pep->pdev;
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&pdev->lock, flags);
	pep->ep_state |= EP_WEDGE;
	ret = gadget_halt_endpoint(pdev, pep, 1);
	spin_unlock_irqrestore(&pdev->lock, flags);

	return ret;
}

static const struct usb_ep_ops gadget_ep0_ops = {
	.enable		= gadget_ep_enable,
	.disable	= gadget_ep_disable,
	.alloc_request	= gadget_ep_alloc_request,
	.free_request	= gadget_ep_free_request,
	.queue		= gadget_ep_queue,
	.dequeue	= gadget_ep_dequeue,
	.set_halt	= gadget_ep_set_halt,
	.set_wedge	= gadget_ep_set_wedge,
};

static const struct usb_ep_ops gadget_ep_ops = {
	.enable		= gadget_ep_enable,
	.disable	= gadget_ep_disable,
	.alloc_request	= gadget_ep_alloc_request,
	.free_request	= gadget_ep_free_request,
	.queue		= gadget_ep_queue,
	.dequeue	= gadget_ep_dequeue,
	.set_halt	= gadget_ep_set_halt,
	.set_wedge	= gadget_ep_set_wedge,
};

static struct usb_endpoint_descriptor gadget_ep0_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bmAttributes = USB_ENDPOINT_XFER_CONTROL,
};

void resume_gadget(struct phytium_device *pdev)
{
	if (pdev->gadget_driver && pdev->gadget_driver->resume) {
		spin_unlock(&pdev->lock);
		pdev->gadget_driver->resume(&pdev->gadget);
		spin_lock(&pdev->lock);
	}
}

void suspend_gadget(struct phytium_device *pdev)
{
	if (pdev->gadget_driver && pdev->gadget_driver->suspend) {
		spin_unlock(&pdev->lock);
		pdev->gadget_driver->suspend(&pdev->gadget);
		spin_lock(&pdev->lock);
	}
}

static void gadget_clear_chicken_bit_2(struct phytium_device *pdev, u32 bit)
{
	__le32 __iomem *reg;
	void __iomem *base;
	u32 offset = 0;

	base = &pdev->cap_regs->hc_capbase;
	offset = phytium_gadget_find_next_ext_cap(base, offset, D_XEC_PER_REGS_CAP);
	reg = base + offset + REG_CHICKEN_BITS_2_OFFSET;

	bit = readl(reg) & ~bit;
	writel(bit, reg);
}

int gadget_setup_device(struct phytium_device *pdev, enum gadget_setup_dev setup)
{
	struct gadget_input_control_ctx *ctrl_ctx;
	struct gadget_slot_ctx *slot_ctx;
	int dev_state = 0;
	int ret;

	if (!pdev->slot_id)
		return -EINVAL;

	if (!pdev->active_port->port_num)
		return -EINVAL;

	slot_ctx = gadget_get_slot_ctx(&pdev->out_ctx);
	dev_state = GET_SLOT_STATE(le32_to_cpu(slot_ctx->dev_state));

	if (setup == SETUP_CONTEXT_ONLY && dev_state == SLOT_STATE_DEFAULT)
		return 0;

	slot_ctx = gadget_get_slot_ctx(&pdev->in_ctx);
	ctrl_ctx = gadget_get_input_control_ctx(&pdev->in_ctx);

	if (!slot_ctx->dev_info || dev_state == SLOT_STATE_DEFAULT) {
		ret = gadget_setup_addressable_priv_dev((void *)pdev);
		if (ret)
			return ret;
	}

	gadget_copy_ep0_dequeue_into_input_ctx(pdev);

	ctrl_ctx->add_flags = cpu_to_le32(SLOT_FLAG | EP0_FLAG);
	ctrl_ctx->drop_flags = 0;

	gadget_queue_address_device(pdev, pdev->in_ctx.dma, setup);
	gadget_ring_cmd_db(pdev);
	ret = gadget_wait_for_cmd_compl(pdev);

	ctrl_ctx->add_flags = 0;
	ctrl_ctx->drop_flags = 0;

	return ret;
}

static int gadget_get_frame(struct usb_gadget *g)
{
	struct phytium_device *pdev = gadget_to_device(g);

	return readl(&pdev->run_regs->microframe_index) >> 3;
}

static void __gadget_wakeup(struct phytium_device *pdev)
{
	struct gadget_port_regs __iomem *port_regs;
	u32 portpm, portsc;

	port_regs = pdev->active_port->regs;
	portsc = readl(&port_regs->portsc) & PORT_PLS_MASK;

	if (pdev->gadget.speed < USB_SPEED_SUPER && portsc == XDEV_U2) {
		portpm = readl(&port_regs->portpmsc);
		if (!(portpm & PORT_RWE))
			return;
	}

	if (portsc == XDEV_U3 && !pdev->may_wakeup)
		return;

	gadget_set_link_state(pdev, &port_regs->portsc, XDEV_U0);

	pdev->gadget_state |= GADGET_WAKEUP_PENDING;
}

static int gadget_wakeup(struct usb_gadget *g)
{
	struct phytium_device *pdev = gadget_to_device(g);
	unsigned long flags;

	spin_lock_irqsave(&pdev->lock, flags);
	__gadget_wakeup(pdev);
	spin_unlock_irqrestore(&pdev->lock, flags);

	return 0;
}

static int gadget_set_selfpowered(struct usb_gadget *g,
		int is_selfpowered)
{
	struct phytium_device *pdev = gadget_to_device(g);
	unsigned long flags;

	spin_lock_irqsave(&pdev->lock, flags);
	g->is_selfpowered = !!is_selfpowered;
	spin_unlock_irqrestore(&pdev->lock, flags);

	return 0;
}

int gadget_halt_endpoint(struct phytium_device *pdev, struct gadget_ep *pep, int value)
{
	int ret;

	ret = gadget_cmd_stop_ep(pdev, pep);
	if (ret)
		return ret;

	if (value) {
		if (GET_EP_CTX_STATE(pep->out_ctx) == EP_STATE_STOPPED) {
			gadget_queue_halt_endpoint(pdev, pep->idx);
			gadget_ring_cmd_db(pdev);
			ret = gadget_wait_for_cmd_compl(pdev);
		}
		pep->ep_state |= EP_HALTED;
	} else {
		gadget_queue_reset_ep(pdev, pep->idx);
		gadget_ring_cmd_db(pdev);
		ret = gadget_wait_for_cmd_compl(pdev);

		if (ret)
			return ret;

		pep->ep_state &= ~EP_HALTED;

		if (pep->idx != 0 && !(pep->ep_state & EP_WEDGE))
			gadget_ring_doorbell_for_active_rings(pdev, pep);

		pep->ep_state &= ~EP_WEDGE;
	}

	return 0;
}

int gadget_reset_device(struct phytium_device *pdev)
{
	struct gadget_slot_ctx *slot_ctx;
	int slot_state;
	int ret, i;

	slot_ctx = gadget_get_slot_ctx(&pdev->in_ctx);
	slot_ctx->dev_info = 0;
	pdev->device_address = 0;

	slot_ctx = gadget_get_slot_ctx(&pdev->out_ctx);
	slot_state = GET_SLOT_STATE(le32_to_cpu(slot_ctx->dev_state));

	if (slot_state <= SLOT_STATE_DEFAULT && pdev->eps[0].ep_state & EP_HALTED)
		gadget_halt_endpoint(pdev, &pdev->eps[0], 0);

	pdev->eps[0].ep_state &= ~(EP_STATE_STOPPED | EP_HALTED);
	pdev->eps[0].ep_state |= EP_ENABLED;

	if (slot_state <= SLOT_STATE_DEFAULT)
		return 0;

	gadget_queue_reset_device(pdev);
	gadget_ring_cmd_db(pdev);
	ret = gadget_wait_for_cmd_compl(pdev);

	for (i = 1; i < GADGET_ENDPOINTS_NUM; i++)
		pdev->eps[i].ep_state |= EP_STATE_STOPPED | EP_UNCONFIGURED;

	if (ret)
		dev_err(pdev->dev, "Reset device failed with %d\n", ret);

	return ret;
}

void gadget_irq_reset(struct phytium_device *pdev)
{
	struct gadget_port_regs __iomem *port_regs;

	gadget_reset_device(pdev);

	port_regs = pdev->active_port->regs;
	pdev->gadget.speed = gadget_port_speed(readl(port_regs));

	spin_unlock(&pdev->lock);
	usb_gadget_udc_reset(&pdev->gadget, pdev->gadget_driver);
	spin_lock(&pdev->lock);

	switch (pdev->gadget.speed) {
	case USB_SPEED_SUPER_PLUS:
	case USB_SPEED_SUPER:
		gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(512);
		pdev->gadget.ep0->maxpacket = 512;
		break;
	case USB_SPEED_HIGH:
	case USB_SPEED_FULL:
		gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(64);
		pdev->gadget.ep0->maxpacket = 64;
		break;
	default:
		dev_err(pdev->dev, "Unknown device speed\n");
		break;
	}

	gadget_clear_chicken_bit_2(pdev, CHICKEN_XDMA_2_TP_CACHE_DIS);
	gadget_setup_device(pdev, SETUP_CONTEXT_ONLY);
	usb_gadget_set_state(&pdev->gadget, USB_STATE_DEFAULT);
}

static int gadget_pullup(struct usb_gadget *g, int is_on)
{
	struct phytium_device *pdev = gadget_to_device(g);
	struct phytium_usb *phytium_usb = dev_get_drvdata(pdev->dev);
	unsigned long flags;

	disable_irq(phytium_usb->device_irq);
	spin_lock_irqsave(&pdev->lock, flags);

	if (!is_on)
		gadget_reset_device(pdev);

	spin_unlock_irqrestore(&pdev->lock, flags);
	enable_irq(phytium_usb->device_irq);

	return 0;
}

static void gadget_disable_port(struct phytium_device *pdev, __le32 __iomem *port_regs)
{
	u32 temp = gadget_port_state_to_neutral(readl(port_regs));

	writel(temp | PORT_PED, port_regs);
}

static int start(struct phytium_device *pdev)
{
	u32 temp;
	int ret;

	temp = readl(&pdev->op_regs->command);
	temp |= (CMD_R_S | CMD_DEVEN);
	writel(temp, &pdev->op_regs->command);

	pdev->gadget_state = 0;

	ret = readl_poll_timeout_atomic(&pdev->op_regs->status, temp,
			!(temp & STS_HALT), 1, GADGET_MAX_HALT_USEC);
	if (ret) {
		pdev->gadget_state = GADGET_STATE_DYING;
		dev_err(pdev->dev, "Error: Controller run failed\n");
	}

	return ret;
}

static int gadget_run(struct phytium_device *pdev, enum usb_device_speed speed)
{
	u32 fs_speed = 0;
	u32 temp;
	int ret;

	temp = readl(&pdev->ir_set->irq_control);
	temp &= ~IMOD_INTERVAL_MASK;
	temp |= ((IMOD_DEFAULT_INTERVAL / 250) & IMOD_INTERVAL_MASK);
	writel(temp, &pdev->ir_set->irq_control);

	temp = readl(&pdev->port3x_regs->mode_addr);

	switch (speed) {
	case USB_SPEED_SUPER_PLUS:
		temp |= CFG_3XPORT_SSP_SUPPORT;
		break;
	case USB_SPEED_SUPER:
		temp &= ~CFG_3XPORT_SSP_SUPPORT;
		break;
	case USB_SPEED_HIGH:
		break;
	case USB_SPEED_FULL:
		fs_speed = PORT_REG6_FORCE_FS;
		break;
	default:
		dev_err(pdev->dev, "Invalid max_speed parameter %d\n", speed);
		fallthrough;
	case USB_SPEED_UNKNOWN:
		speed = USB_SPEED_SUPER;
		break;
	}

	if (speed >= USB_SPEED_SUPER) {
		writel(temp, &pdev->port3x_regs->mode_addr);
		gadget_set_link_state(pdev, &pdev->usb3_port.regs->portsc, XDEV_RXDETECT);
	} else {
		gadget_disable_port(pdev, &pdev->usb3_port.regs->portsc);
	}

	gadget_set_link_state(pdev, &pdev->usb2_port.regs->portsc, XDEV_RXDETECT);

	gadget_ep0_desc.wMaxPacketSize = cpu_to_le16(512);

	writel(PORT_REG6_L1_L0_HW_EN | fs_speed, &pdev->port20_regs->port_regs6);

	ret = start(pdev);
	if (ret) {
		ret = -ENODEV;
		goto err;
	}

	temp = readl(&pdev->op_regs->command);
	temp |= CMD_INTE;
	writel(temp, &pdev->op_regs->command);

	temp = readl(&pdev->ir_set->irq_pending);
	writel(IMAN_IE_SET(temp), &pdev->ir_set->irq_pending);

	return 0;

err:
	gadget_halt(pdev);

	return ret;
}

static int gadget_udc_start(struct usb_gadget *g,
		struct usb_gadget_driver *driver)
{
	enum usb_device_speed max_speed = driver->max_speed;
	struct phytium_device *pdev = gadget_to_device(g);
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&pdev->lock, flags);
	pdev->gadget_driver = driver;

	max_speed = min(driver->max_speed, g->max_speed);
	ret = gadget_run(pdev, max_speed);

	spin_unlock_irqrestore(&pdev->lock, flags);

	return ret;
}

void gadget_set_usb2_hardware_lpm(struct phytium_device *pdev,
		struct usb_request *req, int enable)
{
	if (pdev->active_port != &pdev->usb2_port || !pdev->gadget.lpm_capable)
		return;

	if (enable)
		writel(PORT_BESL(GADGET_DEFAULT_BESL) | PORT_L1S_NYET | PORT_HLE,
				&pdev->active_port->regs->portpmsc);
	else
		writel(PORT_L1S_NYET, &pdev->active_port->regs->portpmsc);
}

int gadget_disable_slot(struct phytium_device *pdev)
{
	int ret;

	gadget_queue_slot_control(pdev, TRB_DISABLE_SLOT);
	gadget_ring_cmd_db(pdev);
	ret = gadget_wait_for_cmd_compl(pdev);

	pdev->slot_id = 0;
	pdev->active_port = NULL;

	memset(pdev->in_ctx.bytes, 0, GADGET_CTX_SIZE);
	memset(pdev->out_ctx.bytes, 0, GADGET_CTX_SIZE);

	return ret;
}

int gadget_enable_slot(struct phytium_device *pdev)
{
	struct gadget_slot_ctx *slot_ctx;
	int slot_state, ret;

	slot_ctx = gadget_get_slot_ctx(&pdev->out_ctx);
	slot_state = GET_SLOT_STATE(le32_to_cpu(slot_ctx->dev_state));

	if (slot_state != SLOT_STATE_DISABLED)
		return 0;

	gadget_queue_slot_control(pdev, TRB_ENABLE_SLOT);
	gadget_ring_cmd_db(pdev);
	ret = gadget_wait_for_cmd_compl(pdev);
	if (ret)
		return ret;

	pdev->slot_id = 1;

	return ret;
}

static void gadget_clear_port_change_bit(struct phytium_device *pdev, __le32 __iomem *port_regs)
{
	u32 portsc = readl(port_regs);

	writel(gadget_port_state_to_neutral(portsc) | (portsc & PORT_CHANGE_BITS), port_regs);
}

void gadget_update_erst_dequeue(struct phytium_device *pdev, union gadget_trb *event_ring_deq,
		u8 clear_ehb)
{
	u64 temp_64;
	dma_addr_t deq;

	temp_64 = lo_hi_readq(&pdev->ir_set->erst_dequeue);

	if (event_ring_deq != pdev->event_ring->dequeue) {
		deq = gadget_trb_virt_to_dma(pdev->event_ring->deq_seg, pdev->event_ring->dequeue);
		temp_64 &= ERST_PTR_MASK;
		temp_64 |= ((u64)deq & (u64)~ERST_PTR_MASK);
	}

	if (clear_ehb)
		temp_64 |= ERST_EHB;
	else
		temp_64 &= ~ERST_EHB;

	lo_hi_writeq(temp_64, &pdev->ir_set->erst_dequeue);
}

static void gadget_consume_all_events(struct phytium_device *pdev)
{
	struct gadget_segment *event_deq_seg;
	union gadget_trb *event_ring_deq;
	union gadget_trb *event;
	u32 cycle_bit;

	event_ring_deq = pdev->event_ring->dequeue;
	event_deq_seg = pdev->event_ring->deq_seg;
	event = pdev->event_ring->dequeue;

	while (1) {
		cycle_bit = (le32_to_cpu(event->event_cmd.flags) & TRB_CYCLE);

		if (cycle_bit != pdev->event_ring->cycle_state)
			break;

		gadget_inc_deq(pdev, pdev->event_ring);

		if (!gadget_last_trb_on_seg(event_deq_seg, event)) {
			event++;
			continue;
		}

		if (gadget_last_trb_on_ring(pdev->event_ring, event_deq_seg, event))
			cycle_bit ^= 1;

		event_deq_seg = event_deq_seg->next;
		event = event_deq_seg->trbs;
	}

	gadget_update_erst_dequeue(pdev, event_ring_deq, 1);
}

static void gadget_clear_cmd_ring(struct phytium_device *pdev)
{
	struct gadget_segment *seg;
	u64 val_64;
	int i;

	gadget_initialize_ring_info(pdev->cmd_ring);

	seg = pdev->cmd_ring->first_seg;
	for (i = 0; i < pdev->cmd_ring->num_segs; i++) {
		memset(seg->trbs, 0, sizeof(union gadget_trb) * (TRBS_PER_SEGMENT - 1));
		seg = seg->next;
	}

	val_64 = lo_hi_readq(&pdev->op_regs->cmd_ring);
	val_64 = (val_64 & (u64)CMD_RING_RSVD_BITS) |
		(pdev->cmd_ring->first_seg->dma & (u64)~CMD_RING_RSVD_BITS)
		| pdev->cmd_ring->cycle_state;
	lo_hi_writeq(val_64, &pdev->op_regs->cmd_ring);
}

static void stop(struct phytium_device *pdev)
{
	u32 temp;

	gadget_cmd_flush_ep(pdev, &pdev->eps[0]);

	if (!list_empty(&pdev->eps[0].pending_list)) {
		struct gadget_request *req;

		req = next_request(&pdev->eps[0].pending_list);
		if (req == &pdev->ep0_preq)
			ep_dequeue(&pdev->eps[0], req);
	}

	gadget_disable_port(pdev, &pdev->usb2_port.regs->portsc);
	gadget_disable_port(pdev, &pdev->usb3_port.regs->portsc);
	gadget_disable_slot(pdev);
	gadget_halt(pdev);

	temp = readl(&pdev->op_regs->status);
	writel((temp & ~0x1fff) | STS_EINT, &pdev->op_regs->status);

	temp = readl(&pdev->ir_set->irq_pending);
	writel(IMAN_IE_CLEAR(temp), &pdev->ir_set->irq_pending);

	gadget_clear_port_change_bit(pdev, &pdev->usb2_port.regs->portsc);
	gadget_clear_port_change_bit(pdev, &pdev->usb3_port.regs->portsc);

	temp = readl(&pdev->ir_set->irq_pending);
	temp |= IMAN_IP;
	writel(temp, &pdev->ir_set->irq_pending);

	gadget_consume_all_events(pdev);
	gadget_clear_cmd_ring(pdev);
}

static int gadget_udc_stop(struct usb_gadget *g)
{
	struct phytium_device *pdev = gadget_to_device(g);
	unsigned long flags;

	spin_lock_irqsave(&pdev->lock, flags);
	stop(pdev);
	pdev->gadget_driver = NULL;
	spin_unlock_irqrestore(&pdev->lock, flags);

	return 0;
}

static const struct usb_gadget_ops phytium_usb_gadget_ops = {
	.get_frame		= gadget_get_frame,
	.wakeup			= gadget_wakeup,
	.set_selfpowered	= gadget_set_selfpowered,
	.pullup			= gadget_pullup,
	.udc_start		= gadget_udc_start,
	.udc_stop		= gadget_udc_stop,
};

void gadget_died(struct phytium_device *pdev)
{
	dev_err(pdev->dev, "Error: controller not responding\n");
	pdev->gadget_state |= GADGET_STATE_DYING;
	gadget_halt(pdev);
}

static void gadget_quiesce(struct phytium_device *pdev)
{
	u32 halted, mask, cmd;

	mask = ~(u32)(GADGET_IRQS);

	halted = readl(&pdev->op_regs->status) & STS_HALT;
	if (!halted)
		mask &= ~(CMD_R_S | CMD_DEVEN);

	cmd = readl(&pdev->op_regs->command);
	cmd &= mask;
	writel(cmd, &pdev->op_regs->command);
}

int gadget_halt(struct phytium_device *pdev)
{
	int ret;
	u32 val;

	gadget_quiesce(pdev);

	ret = readl_poll_timeout_atomic(&pdev->op_regs->status, val,
			val & STS_HALT, 1, MAX_HALT_USEC);
	if (ret) {
		dev_err(pdev->dev, "gadget halt failed\n");
		return ret;
	}

	pdev->gadget_state |= GADGET_STATE_HALTED;

	return 0;
}

int gadget_reset(struct phytium_device *pdev)
{
	u32 cmd, temp;
	int ret;

	temp = readl(&pdev->op_regs->status);
	if (temp == ~(u32)0) {
		dev_err(pdev->dev, "Controller not accessible\n");
		return -ENODEV;
	}

	if ((temp & STS_HALT) == 0) {
		dev_err(pdev->dev, "controller not halted, abort reset\n");
		return -EINVAL;
	}

	cmd = readl(&pdev->op_regs->command);
	cmd |= CMD_RESET;
	writel(cmd, &pdev->op_regs->command);

	ret = readl_poll_timeout_atomic(&pdev->op_regs->command, temp,
			!(temp & CMD_RESET), 1, 10 * 1000);
	if (ret) {
		dev_err(pdev->dev, "controller reset failed\n");
		return ret;
	}

	ret = readl_poll_timeout_atomic(&pdev->op_regs->status, temp,
			!(temp & STS_CNR), 1, 10 * 1000);
	if (ret) {
		dev_err(pdev->dev, "controller not ready to work\n");
		return ret;
	}

	dev_info(pdev->dev, "controller ready to work\n");

	return ret;
}

int phytium_gadget_find_next_ext_cap(void __iomem *base, u32 start, int id)
{
	u32 offset = start;
	u32 next;
	u32 val;

	if (!start || start == HCC_PARAMS_OFFSET) {
		val = readl(base + HCC_PARAMS_OFFSET);
		if (val == ~0)
			return 0;

		offset = HCC_EXT_CAPS(val) << 2;
		if (!offset)
			return 0;
	}

	do {
		val = readl(base + offset);
		if (val == ~0)
			return 0;

		if (EXT_CAPS_ID(val) == id && offset != start)
			return offset;

		next = EXT_CAPS_NEXT(val);
		offset += next << 2;
	} while (next);

	return 0;
}

static void phytium_gadget_get_rev_cap(struct phytium_device *pdev)
{
	void __iomem *reg;

	if (!pdev)
		return;

	reg = &pdev->cap_regs->hc_capbase;
	reg += phytium_gadget_find_next_ext_cap(reg, 0, RTL_REV_CAP);
	pdev->rev_cap = reg;

	dev_info(pdev->dev, "Rev: %08x/%08x, eps: %08x, buff: %08x/%08x\n",
			readl(&pdev->rev_cap->ctrl_revision),
			readl(&pdev->rev_cap->rtl_revision),
			readl(&pdev->rev_cap->ep_supported),
			readl(&pdev->rev_cap->rx_buff_size),
			readl(&pdev->rev_cap->tx_buff_size));
}

static int gadget_gen_setup(struct phytium_device *pdev)
{
	int ret;
	u32 reg;

	if (!pdev)
		return 0;

	pdev->cap_regs = pdev->regs;
	pdev->op_regs = pdev->regs +
		HC_LENGTH(readl(&pdev->cap_regs->hc_capbase));
	pdev->run_regs = pdev->regs +
		(readl(&pdev->cap_regs->run_regs_off) & RTSOFF_MASK);
	pdev->hcs_params1 = readl(&pdev->cap_regs->hcs_params1);
	pdev->hcc_params = readl(&pdev->cap_regs->hc_capbase);
	pdev->hci_version = HC_VERSION(pdev->hcc_params);
	pdev->hcc_params = readl(&pdev->cap_regs->hcc_params);

	phytium_gadget_get_rev_cap(pdev);

	ret = gadget_halt(pdev);
	if (ret)
		return ret;

	ret = gadget_reset(pdev);
	if (ret)
		return ret;

	if (HCC_64BIT_ADDR(pdev->hcc_params) &&
		!dma_set_mask(pdev->dev, DMA_BIT_MASK(64))) {
		dev_info(pdev->dev, "Enableing 64 bit DMA addr\n");
		dma_set_coherent_mask(pdev->dev, DMA_BIT_MASK(64));
	} else {
		ret = dma_set_mask(pdev->dev, DMA_BIT_MASK(32));
		if (ret)
			return ret;

		dev_info(pdev->dev, "Enableing 32 bit DMA addr\n");
		dma_set_coherent_mask(pdev->dev, DMA_BIT_MASK(32));
	}

	spin_lock_init(&pdev->lock);

	ret = gadget_mem_init((void *)pdev);
	if (ret)
		return ret;

	reg = readl(&pdev->port3x_regs->mode_2);
	reg &= ~CFG_3XPORT_U1_PIPE_CLK_GATE_EN;
	writel(reg, &pdev->port3x_regs->mode_2);

	return 0;
}

static void gadget_get_ep_buffering(struct phytium_device *pdev, struct gadget_ep *pep)
{
	void __iomem *reg = &pdev->cap_regs->hc_capbase;
	int endpoints;

	reg += phytium_gadget_find_next_ext_cap(reg, 0, XBUF_CAP_ID);

	if (!pep->direction) {
		pep->buffering = readl(reg + XBUF_RX_TAG_MASK_0_OFFSET);
		pep->buffering_period = readl(reg + XBUF_RX_TAG_MASK_1_OFFSET);
		pep->buffering = (pep->buffering + 1) / 2;
		pep->buffering_period = (pep->buffering_period + 1) / 2;

		return;
	}

	endpoints = HCS_ENDPOINTS(pdev->hcs_params1) / 2;

	reg += XBUF_TX_CMD_OFFSET + (endpoints * 2 + 2) * sizeof(u32);
	reg += pep->number * sizeof(u32) * 2;
	pep->buffering = (readl(reg) + 1) / 2;
	pep->buffering_period = pep->buffering;
}

static int gadget_init_endpoints(struct phytium_device *pdev)
{
	int max_streams = HCC_MAX_PSA(pdev->hcc_params);
	struct gadget_ep *pep;
	int i;

	INIT_LIST_HEAD(&pdev->gadget.ep_list);

	if (max_streams < STREAM_LOG_STREAMS) {
		dev_err(pdev->dev, "Stream size %d not supported\n",
				max_streams);
		return -EINVAL;
	}

	max_streams = STREAM_LOG_STREAMS;

	for (i = 0; i < GADGET_ENDPOINTS_NUM; i++) {
		bool direction = !(i & 1);
		u8 epnum = ((i + 1) >> 1);

		if (!GADGET_IF_EP_EXIST(pdev, epnum, direction))
			continue;

		pep = &pdev->eps[i];
		pep->pdev = pdev;
		pep->number = epnum;
		pep->direction = direction;

		if (epnum == 0) {
			snprintf(pep->name, sizeof(pep->name), "ep%d%s",
					epnum, "BiDir");

			pep->idx = 0;
			usb_ep_set_maxpacket_limit(&pep->endpoint, 512);
			pep->endpoint.maxburst = 1;
			pep->endpoint.ops = &gadget_ep0_ops;
			pep->endpoint.desc = &gadget_ep0_desc;
			pep->endpoint.comp_desc = NULL;
			pep->endpoint.caps.type_control = true;
			pep->endpoint.caps.dir_in = true;
			pep->endpoint.caps.dir_out = true;

			pdev->ep0_preq.epnum = pep->number;
			pdev->ep0_preq.pep = pep;
			pdev->gadget.ep0 = &pep->endpoint;
		} else {
			snprintf(pep->name, sizeof(pep->name), "ep%d%s", epnum,
				(pep->direction) ? "in" : "out");

				pep->idx = (epnum * 2 + (direction ? 1 : 0)) - 1;
				usb_ep_set_maxpacket_limit(&pep->endpoint, 1024);

				pep->endpoint.max_streams = max_streams;
				pep->endpoint.ops = &gadget_ep_ops;
				list_add_tail(&pep->endpoint.ep_list, &pdev->gadget.ep_list);

				pep->endpoint.caps.type_iso = true;
				pep->endpoint.caps.type_bulk = true;
				pep->endpoint.caps.type_int = true;
				pep->endpoint.caps.dir_in = direction;
				pep->endpoint.caps.dir_out = !direction;
		}

		pep->endpoint.name = pep->name;
		pep->in_ctx = gadget_get_ep_ctx(&pdev->in_ctx, pep->idx);
		pep->out_ctx = gadget_get_ep_ctx(&pdev->out_ctx, pep->idx);
		gadget_get_ep_buffering(pdev, pep);

		INIT_LIST_HEAD(&pep->pending_list);
	}

	return 0;
}

static void gadget_free_endpoints(struct phytium_device *pdev)
{
}

static int gadget_restart(void *data, struct phytium_device *pdev)
{
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;
	int ret;
	u32 reg;

	if (!phytium_usb)
		return 0;

	ret = gadget_mem_init((void *)pdev);
	if (ret)
		return ret;

	reg = readl(&pdev->port3x_regs->mode_2);
	reg &= ~CFG_3XPORT_U1_PIPE_CLK_GATE_EN;
	writel(reg, &pdev->port3x_regs->mode_2);

	ret = gadget_init_endpoints(pdev);
	if (ret) {
		dev_err(pdev->dev, "gadget_init_endpoints failed\n");
		return ret;
	}

	return 0;
}

static int gadget_start(void *data)
{
	struct phytium_device *pdev;
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;
	u32 max_speed;
	int ret;

	if (!phytium_usb)
		return 0;

	phytium_usb_otg_gadget_on((void *)phytium_usb);

	pdev = kzalloc(sizeof(*pdev), GFP_KERNEL);
	if (!pdev)
		return -ENOMEM;

	phytium_usb->gadget_dev = pdev;
	pdev->dev = phytium_usb->dev;
	pdev->regs = phytium_usb->device_regs;
	max_speed = usb_get_maximum_speed(pdev->dev);

	switch (max_speed) {
	case USB_SPEED_FULL:
	case USB_SPEED_HIGH:
	case USB_SPEED_SUPER:
	case USB_SPEED_SUPER_PLUS:
		break;
	case USB_SPEED_UNKNOWN:
		max_speed = USB_SPEED_SUPER_PLUS;
		break;
	default:
		dev_err(pdev->dev, "invalid max speed %d\n", max_speed);
	}

	pdev->gadget.ops = &phytium_usb_gadget_ops;
	pdev->gadget.name = "phytium_usb_gadget";
	pdev->gadget.speed = USB_SPEED_UNKNOWN;
	pdev->gadget.sg_supported = 0;
	pdev->gadget.max_speed = max_speed;
	pdev->gadget.lpm_capable = 0;
	pdev->gadget.quirk_ep_out_aligned_size = true;

	pdev->setup_buf = kzalloc(EP0_SETUP_SIZE, GFP_KERNEL);
	if (!pdev->setup_buf)
		goto free_pdev;

	ret = gadget_gen_setup(pdev);
	if (ret) {
		dev_err(pdev->dev, "gadget_gen_setup failed\n");
		goto free_setup;
	}

	ret = gadget_init_endpoints(pdev);
	if (ret) {
		dev_err(pdev->dev, "gadget_init_endpoints failed\n");
		goto free_pdev;
	}

	ret = usb_add_gadget_udc(pdev->dev, &pdev->gadget);
	if (ret) {
		dev_err(pdev->dev, "usb_add_gadget_udc failed\n");
		goto free_endpoints;
	}

	ret = devm_request_irq(pdev->dev, phytium_usb->device_irq, gadget_irq_handler,
			IRQF_SHARED, "phytium_usb_gadget", pdev);
	if (ret)
		goto del_gadget;

	return 0;

del_gadget:
	usb_del_gadget_udc(&pdev->gadget);
free_endpoints:
	gadget_free_endpoints(pdev);
free_setup:
	kfree(pdev->setup_buf);
free_pdev:
	kfree(pdev);

	return ret;
}

static void gadget_stop(void *data)
{
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;
	struct phytium_device *pdev = phytium_usb->gadget_dev;

	devm_free_irq(pdev->dev, phytium_usb->device_irq, pdev);
	usb_del_gadget_udc(&pdev->gadget);
	gadget_free_endpoints(pdev);
	gadget_mem_cleanup((void *)pdev);
	kfree(pdev);
	phytium_usb->gadget_dev = NULL;
	phytium_usb_otg_gadget_off((void *)phytium_usb);
}

void disconnect_gadget(struct phytium_device *pdev)
{
	pdev->gadget_state |= GADGET_STATE_DISCONNECT_PENDING;

	if (pdev->gadget_driver && pdev->gadget_driver->disconnect) {
		spin_unlock(&pdev->lock);
		pdev->gadget_driver->disconnect(&pdev->gadget);
		spin_lock(&pdev->lock);
	}

	pdev->gadget.speed = USB_SPEED_UNKNOWN;
	usb_gadget_set_state(&pdev->gadget, USB_STATE_NOTATTACHED);

	pdev->gadget_state &= ~GADGET_STATE_DISCONNECT_PENDING;
}

static int gadget_suspend(void *data, bool do_wakeup)
{
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;
	struct phytium_device *pdev = phytium_usb->gadget_dev;
	unsigned long flags;

	spin_lock_irqsave(&pdev->lock, flags);
	disconnect_gadget(pdev);
	stop(pdev);
	gadget_mem_cleanup((void *)pdev);
	spin_unlock_irqrestore(&pdev->lock, flags);

	return 0;
}

static int gadget_resume(void *data, bool hibernated)
{
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;
	struct phytium_device *pdev = phytium_usb->gadget_dev;
	unsigned long flags;
	enum usb_device_speed max_speed;
	int ret;

	spin_lock_irqsave(&pdev->lock, flags);
	gadget_restart(phytium_usb, pdev);

	if (!pdev->gadget_driver) {
		spin_unlock_irqrestore(&pdev->lock, flags);
		return 0;
	}

	max_speed = pdev->gadget_driver->max_speed;
	max_speed = min(max_speed, pdev->gadget.max_speed);

	ret = gadget_run(pdev, max_speed);
	phytium_usb_otg_gadget_on((void *)phytium_usb);

	if (pdev->link_state == XDEV_U3)
		__gadget_wakeup(pdev);
	spin_unlock_irqrestore(&pdev->lock, flags);

	return ret;
}

int phytium_usb_gadget_init(void *data)
{
	struct phytium_usb_role_driver *role_driver;
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;

	if (phytium_usb) {
		role_driver = devm_kzalloc(phytium_usb->dev, sizeof(*phytium_usb), GFP_KERNEL);
		if (!role_driver)
			return -ENOMEM;

		role_driver->start = gadget_start;
		role_driver->stop = gadget_stop;
		role_driver->suspend = gadget_suspend;
		role_driver->resume = gadget_resume;
		role_driver->state = ROLE_STATE_INACTIVE;
		role_driver->name = "gadget";
		phytium_usb->roles[USB_ROLE_DEVICE] = role_driver;
	}

	return 0;
}

/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __LINUX_PHYTIUM_USB_RING_H__
#define __LINUX_PHYTIUM_USB_RING_H__

#include "gadget.h"
#include <linux/irq.h>

dma_addr_t gadget_trb_virt_to_dma(struct gadget_segment *seg,
		union gadget_trb *trb);

void gadget_queue_configure_endpoint(struct phytium_device *pdev,
		dma_addr_t in_ctx_ptr);

void gadget_ring_doorbell_for_active_rings(struct phytium_device *pdev,
		struct gadget_ep *pep);

void gadget_initialize_ring_info(struct gadget_ring *ring);

irqreturn_t gadget_irq_handler(int irq, void *priv);

irqreturn_t gadget_thread_irq_handler(int irq, void *data);

void gadget_queue_address_device(struct phytium_device *pdev,
		dma_addr_t in_ctx_ptr, enum gadget_setup_dev setup);

int gadget_queue_ctrl_tx(struct phytium_device *pdev, struct gadget_request *preq);
int gadget_queue_bulk_tx(struct phytium_device *pdev, struct gadget_request *preq);
int gadget_queue_isoc_tx_prepare(struct phytium_device *pdev, struct gadget_request *preq);
#endif

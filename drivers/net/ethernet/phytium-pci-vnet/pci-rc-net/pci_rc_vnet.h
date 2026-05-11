/* SPDX-License-Identifier: GPL-2.0
 *
 * phytium pcie ep vnet Ethernet Controller driver
 *
 * Copyright (C) 2023-2025 PHYTIUM Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __PCI_RC_VNET_H
#define __PCI_RC_VNET_H
#include <linux/circ_buf.h>
#include <linux/crc32.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/ethtool.h>
#include <linux/phy.h>
#include <linux/if_vlan.h>
#include <linux/in.h>
#include <linux/io.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/pm_runtime.h>
#include <linux/bitfield.h>
#include <linux/prefetch.h>
#include <linux/ipv6.h>
#include <linux/pci_ids.h>
#include <net/ip6_checksum.h>
#include <linux/spinlock.h>
#include <linux/kernel.h>
#include <linux/timer.h>

#define DRV_MODULE_NAME "pci_rc_vnet"
#define DRV_VERSION		"1.0.1"

#define PCI_ENDPOINT_VNET_IRQ_NUMBER    0x00
#define PCI_ENDPOINT_VNET_QUEUE_TAIL    0x04
#define PCI_ENDPOINT_VNET_QUEUE_HEAD	0x08
#define PCI_ENDPOINT_VNET_DMA_DESC_BASE   0x0c

#define PCI_ENDPOINT_VNET_DMA_DESC_LOWER_ADDR	0x00
#define PCI_ENDPOINT_VNET_DMA_DESC_UPPER_ADDR	0x04
#define PCI_ENDPOINT_VNET_DMA_DESC_CTRL			0x08

#define PCI_RC_VNET_DMA_DESC_CTRL_PKT_USED_OFFSET		0
#define PCI_RC_VNET_DMA_DESC_CTRL_PKT_USED_SIZE			1
#define PCI_RC_VNET_DMA_DESC_CTRL_PKT_ERR_OFFSET		1
#define PCI_RC_VNET_DMA_DESC_CTRL_PKT_ERR_SIZE			1
#define PCI_RC_VNET_DMA_DESC_CTRL_PKT_ADDR_DIFF_OFFSET		2
#define PCI_RC_VNET_DMA_DESC_CTRL_PKT_ADDR_DIFF_SIZE		1
#define PCI_RC_VNET_DMA_DESC_CTRL_PKT_MISALIGN_OFFSET		3
#define PCI_RC_VNET_DMA_DESC_CTRL_PKT_MISALIGN_SIZE		5
#define PCI_RC_VNET_DMA_DESC_CTRL_PKT_RESERVE_OFFSET	8
#define PCI_RC_VNET_DMA_DESC_CTRL_PKT_RESERVE_SIZE		8
#define PCI_RC_VNET_DMA_DESC_CTRL_PKT_PKT_LEN_OFFSET	16
#define PCI_RC_VNET_DMA_DESC_CTRL_PKT_PKT_LEN_SIZE		16
#define PCI_RC_VNET_DMA_DESC_CTRL_PKT_CHECKSUM_OFFSET	32
#define PCI_RC_VNET_DMA_DESC_CTRL_PKT_CHECKSUM_SIZE		32

#define PCI_RC_VNET_DMA_DESC_CTRL_PKT_USED	BIT(0)

#define PCI_RC_VNET_RXBUFFER_1024		1024
#define PCI_RC_VNET_RXBUFFER_2048		2048
#define PCI_RC_VNET_RXBUFFER_4096		4096
#define PCI_RC_VNET_RXBUFFER_8192		8192
#define PCI_RC_VNET_RXBUFFER_16384		16384

#define PCI_RC_VNET_DMA_BUFF_NUM	4096
#define PCI_RC_VNET_WAKE_NET_QUEUE_THRESHOLD	10
#define PCI_RC_VNET_TX_QUEUE_STOP_THRESHOLD	64
#define PCI_RC_VNET_TX_QUEUE_RESTART_THRESHOLD	64

#define PCI_RC_VNET_WATCHDOG_PERIOD	(2 * HZ)
#define PCI_RC_VNET_LINK_DETECT_PERIOD_NS	500000000
#define PCI_RC_VNET_RECEIVE_PERIOD_NS		5000
#define PCI_RC_VNET_TX_POLL_PERIOD_NS		5000
#define PCI_RC_VNET_NAPI_PULL_BUDGET	64

#define BAR_NUM		6
#define TX_IRQ		2
#define RX_IRQ		1

#define PCI_RC_VNET_FLAGS_EP_LINK	BIT(0)
#define PCI_RC_VNET_FLAGS_RC_LINK	BIT(1)

#define MAX_JUMBO_FRAME_SIZE         4000

#define PCI_RC_VNET_DMA_DESC_CTRL_FIELD_MASK(name)		\
	(((1UL << PCI_RC_VNET_DMA_DESC_CTRL_PKT_##name##_SIZE) - 1)	\
	<< PCI_RC_VNET_DMA_DESC_CTRL_PKT_##name##_OFFSET)

#define PCI_RC_VNET_DMA_DESC_FIELD_GET(name, desc)		\
	(((desc) >> PCI_RC_VNET_DMA_DESC_CTRL_PKT_##name##_OFFSET)	\
	& ((1UL << PCI_RC_VNET_DMA_DESC_CTRL_PKT_##name##_SIZE) - 1))

#define PCI_RC_VNET_DMA_DESC_SET(name, value)		\
	((((u64)value) & ((1UL << PCI_RC_VNET_DMA_DESC_CTRL_PKT_##name##_SIZE) - 1))	\
	<<	PCI_RC_VNET_DMA_DESC_CTRL_PKT_##name##_OFFSET)

#define SEND_MSI_IRQ	0xf
#define RC_SKB_PAD		(0)

#define RC_RX_DMA_ATTR \
	(DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING)

#define RC_GFP_FLAGS \
	(GFP_ATOMIC | __GFP_NOWARN)

#define RC_RX_PAGE_ORDER	0
#define RC_RX_PAGE_SIZE	(PAGE_SIZE << RC_RX_PAGE_ORDER)

#define RC_RXBUFFER_2048 2048
#define RX_BUFFER_MULTIPLE 64
#define RC_MAX_FRAME_BUILD_SKB \
	(SKB_WITH_OVERHEAD(RC_RXBUFFER_2048) - RC_SKB_PAD)
enum pci_barno {
	BAR_0,
	BAR_1,
	BAR_2,
	BAR_3,
	BAR_4,
	BAR_5,
};

struct param_range {
	u32 min;
	u32 max;
	u32 count;
};

struct params {
	struct param_range rx_ring;
	struct param_range tx_ring;
};

struct skb_dma {
	void *virt_addr;
	dma_addr_t phys_addr;
};

struct pci_rc_vnet_dma_desc {
	u64 addr;
	u64 ctrl;
};

struct pci_rc_vnet_tx_queue {
	u32	irq;
	u32 tail;
	u32 head;
	u32 nb_desc;
	u32 flags;
	struct pci_rc_vnet_dma_desc desc_ring[PCI_RC_VNET_DMA_BUFF_NUM];
};

struct pci_rc_vnet_rx_queue {
	u32	irq;
	u32 tail;
	u32 head;
	u32 nb_desc;
	u32 flags;
	struct pci_rc_vnet_dma_desc desc_ring[PCI_RC_VNET_DMA_BUFF_NUM];
};

struct rc_rx_buffer {
	dma_addr_t addr;
	struct page *page;
	__u16 page_offset;
	__u16 pagecnt_bias;
};

/* bar space */
struct pci_rc_vnet_queue {
	struct pci_rc_vnet_tx_queue tx_queue;
	struct pci_rc_vnet_rx_queue rx_queue;
} __packed;

struct pci_rc_vnet_private {
	u32 msg_enable				____cacheline_aligned;
	int rx_buffer_len;
	struct pci_dev *pci_dev;
	void __iomem	*bar[BAR_NUM];

	dma_addr_t *tx_phys_addr_list;
	struct sk_buff **tx_skbuff;
	struct rc_rx_buffer *rx_buffer_info;
	u32 rx_next_to_alloc;

	struct pci_rc_vnet_queue *queue;

	struct pci_rc_vnet_tx_queue *tx_queue;
	struct pci_rc_vnet_rx_queue *rx_queue;

	struct net_device *netdev;
	int irq_type;
	int num_irqs;

	/* Lock to protect tx */
	spinlock_t tx_lock;
	/* Lock to protect tx_reclaim_num */
	spinlock_t tx_reclaim_lock;
	/* Lock to protect rx */
	spinlock_t rx_lock;

	struct params params;

	struct work_struct tx_timeout_task;

	int (*clean_ep2rc)(struct pci_rc_vnet_private *tp, int budget);

	struct napi_struct	tx_napi;
	struct napi_struct	rx_napi;

	int tx_reclaim_start;
	int tx_reclaim_num;

	int rx_refill_start;
	struct hrtimer link_detect_timer;
	struct hrtimer receive_timer;
	struct hrtimer tx_poll_timer;
	void __iomem *msi_irq_addr;
};

#endif //end __PCI_RC_VNET_H

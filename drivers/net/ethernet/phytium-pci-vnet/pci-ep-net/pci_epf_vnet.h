/* SPDX-License-Identifier: GPL-2.0
 *
 * phytium pcie ep vnet Ethernet Controller driver
 *
 * Copyright (C) 2023-2023 PHYTIUM Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __PCI_EPF_VNET_H__
#define __PCI_EPF_VNET_H__

#include <linux/timer.h>
#include <linux/hrtimer.h>
#include <linux/ethtool.h>

#define DRV_MODULE_NAME		"pci_epf_vnet"
#define DRV_VERSION		"1.0.0"

/* IRQ type */
#define IRQ_TYPE_LEGACY					0
#define IRQ_TYPE_MSI					1
#define IRQ_TYPE_MSIX					2

/* DMA transfer state */
#define DMA_STATUS_DONE					BIT(0)
#define DMA_STATUS_ERROR				0xfffffffe

/* DMA transmission direction */
#define DMA_READ					0
#define DMA_WRITE					1

/* Supported Rx Buffer Sizes */
#define VNET_RXBUFFER_1024				1024
#define VNET_RXBUFFER_2048				2048
#define VNET_RXBUFFER_4096				4096
#define VNET_RXBUFFER_8192				8192
#define VNET_RXBUFFER_16384				16384

#define MAX_JUMBO_FRAME_SIZE				4000

/* Ctrl in the bar space descriptor*/
#define PCI_EP_DMA_DESC_CTRL_PKT_USED_OFFSET		0 /* 1=Valid packet */
								/*0=Invalid packet*/
#define PCI_EP_DMA_DESC_CTRL_PKT_USED_SIZE		1
#define PCI_EP_DMA_DESC_CTRL_PKT_ERR_OFFSET		1 /* 1=DMA transfer successful */
								/* 0=DMA transfer failure */
#define PCI_EP_DMA_DESC_CTRL_PKT_ERR_SIZE		1
#define PCI_EP_DMA_DESC_CTRL_PKT_ADDR_DIFF_OFFSET	2 /* 1=Move the receiving */
						/* address to higher position */
						/* 0=Move receiving address to lower position */
#define PCI_EP_DMA_DESC_CTRL_PKT_ADDR_DIFF_SIZE		1
#define PCI_EP_DMA_DESC_CTRL_PKT_MISALIGN_OFFSET	3
#define PCI_EP_DMA_DESC_CTRL_PKT_MISALIGN_SIZE		5
#define PCI_EP_DMA_DESC_CTRL_PKT_RESERVE_OFFSET		8
#define	PCI_EP_DMA_DESC_CTRL_PKT_RESERVE_SIZE		8
#define PCI_EP_DMA_DESC_CTRL_PKT_LEN_OFFSET		16 /* Packet length */
#define PCI_EP_DMA_DESC_CTRL_PKT_LEN_SIZE		16
#define PCI_EP_DMA_DESC_CTRL_PKT_CHECKSUM_OFFSET	32 /* Checksum bit */
#define PCI_EP_DMA_DESC_CTRL_PKT_CHECKSUM_SIZE		32

#define PCI_EP_DMA_DESC_CTRL_PKT_USED				BIT(0)
#define PCI_EP_DMA_DESC_CTRL_PKT_ERR				BIT(1)
#define PCI_EP_DMA_DESC_CTRL_PKT_ADDR_DIFF			BIT(10)

#define PCI_EP_DMA_DESC_CTRL_FIELD_MASK(name)                                         \
		(((1 << PCI_EP_DMA_DESC_CTRL_PKT_##name##_SIZE) - 1)                    \
		<< PCI_EP_DMA_DESC_CTRL_PKT_##name##_OFFSET)

#define PCI_EP_DMA_DESC_FIELD_SET(name, value)                                        \
	((((u64)value) & ((1UL << PCI_EP_DMA_DESC_CTRL_PKT_##name##_SIZE) - 1UL))       \
	 << PCI_EP_DMA_DESC_CTRL_PKT_##name##_OFFSET)

#define PCI_EP_DMA_DESC_FIELD_GET(name, desc)		\
	(((desc) >> PCI_EP_DMA_DESC_CTRL_PKT_##name##_OFFSET) \
	& ((1UL << PCI_EP_DMA_DESC_CTRL_PKT_##name##_SIZE) - 1))

/* Flags in the bar space */
#define PCI_EP_VNET_FLAGS_EP_LINK_ACTIVE_OFFSET				0
#define PCI_EP_VNET_FLAGS_RC_LINK_ACTIVE_OFFSET				1

#define PCI_EP_VNET_FLAGS_BIT(name)					\
	(1 << PCI_EP_VNET_FLAGS_##name##_OFFSET)

/* TX/RX descriptor defines */
#define VNET_DEFAULT_TXD		1024
#define VNET_MAX_TXD			4096
#define VNET_MIN_TXD			48

#define VNET_DEFAULT_RXD		1024
#define VNET_MAX_RXD			4096
#define VNET_MIN_RXD			48

#define DIVISOR				0x1000

#define MAX_BAR_NUM			6

#define DMA_ALIGN_SIZE			16 /* Address alignment value of DMA */

#define RX_IRQ				1

#define	PHYTIUM_PCIE_FUNC_BASE(fn)		(((fn) << 14) & GENMASK(16, 14))
#define	PHYTIUM_PCI_WIN0_BASE			0x600
#define	PHYTIUM_PCI_WIN0_TRSL_ADDR0(table)	(PHYTIUM_PCI_WIN0_BASE + 0X20 * (table) + 0x8)
#define	PHYTIUM_PCI_WIN0_TRSL_ADDR1(table)	(PHYTIUM_PCI_WIN0_BASE + 0X20 * (table) + 0xc)

#define HPB_ERR_EVT_LOG_EN		BIT(6)
#define HPB_ERR_EVT_INT_EN		BIT(6)
#define	HPB_ERR_EVT_INT_CLR		0x0
#define	HPB_MSI_EN			0xF
#define HPB_NS_MSI_SPI_EN		0x1

#define PHYTIUM_HPB_REG_ERR_EVT_LOG_EN		0x0C4
#define	PHYTIUM_HPB_REG_ERR_EVT_INT_EN		0x0C8
#define PHYTIUM_HPB_REG_ERR_EVT_WIC		0x0D0
#define PHYTIUM_HPB_REG_MSI_EN			0x200
#define PHYTIUM_HPB_REG_MSI64_HI_ADDR		0x208
#define PHYTIUM_HPB_REG_MSI64_LO_ADDR		0x20C
#define PHYTIUM_HPB_REG_NS_MSI_SPI_EN		0x608
#define PHYTIUM_HPB_REG_NS_MSI_DATA		0x60c

#define LINK_DETECT_PERIOD			500

#define EP_SKB_PAD		(0)

#define EP_DMA_ATTR \
	(DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING)

#define EP_GFP_FLAGS \
	(GFP_ATOMIC | __GFP_NOWARN | __GFP_DMA32)

#define EP_RX_PAGE_ORDER	0
#define EP_RX_PAGE_SIZE	(PAGE_SIZE << EP_RX_PAGE_ORDER)

#define EP_TX_PAGE_ORDER	0
#define EP_TX_PAGE_SIZE		(PAGE_SIZE << EP_TX_PAGE_ORDER)
#define EP_RXBUFFER_2048 2048
#define RX_BUFFER_MULTIPLE 64
#define EP_MAX_FRAME_BUILD_SKB \
	(SKB_WITH_OVERHEAD(EP_RXBUFFER_2048) - EP_SKB_PAD)
struct param_range {
	u32 min;
	u32 max;
	u32 count;
};

/* Private structure of EPC */
struct phytium_pcie_ep {
	void __iomem		*reg_base;
	struct resource		*mem_res;
	void __iomem		*hpb_base;
	unsigned int		max_regions;
	unsigned long		ob_region_map;
	phys_addr_t		*ob_addr;
	phys_addr_t		irq_phys_addr;
	void __iomem		*irq_cpu_addr;
	unsigned long		irq_pci_addr;
	u8			irq_pci_fn;
	struct pci_epc		*epc;

	struct platform_device *pdev;
};

/* Shared descriptor */
struct pci_ep_dma_desc {
	u64 addr;
	u64 ctrl;
};

struct ep_queue {
	u32 irq_num;
	u32 tail;
	u32 head;
	u32 nb_desc;
	u32 flags;
	struct pci_ep_dma_desc dma_desc_base[VNET_MAX_RXD];
};

/* bar space */
struct pci_ep_queue {
	struct ep_queue ep_rx_queue;
	struct ep_queue ep_tx_queue;
} __packed;

struct ep_rx_buffer {
	dma_addr_t addr;
	struct page *page;
	__u16 page_offset;
	__u16 pagecnt_bias;
};

struct ep_tx_buffer {
	dma_addr_t addr;
	void *vaddr;
	struct page *page;
	__u16 page_offset;
	__u16 pagecnt_bias;
};

/* Structure containing variables used by the shared code */
struct pci_epf_vnet {
	u32 msg_enable	____cacheline_aligned;

	void *reg[MAX_BAR_NUM];
	struct pci_epf *epf;
	enum pci_barno vnet_reg_barno;
	struct hrtimer timer;
	struct timer_list link_detect_timer;

	u32 rx_refill_start;
	unsigned int rx_refill_num;
	u32 rx_ring_size;   /* Shared descriptor size */
	u32 rx_buffer_size;
	struct sk_buff **rx_skb;
	struct ep_rx_buffer *rx_buffer_info;
	int	rx_buffer_len;
	u32 rx_next_to_alloc;
	struct param_range rxd_size;   /* Records rx descriptor ring size range */

	u32 tx_head, tx_tail;
	u32 tx_ring_size;
	struct param_range txd_size;
	struct ep_tx_buffer *tx_buffer_info;

	struct net_device *netdev;

	/* Lock to protect tx */
	spinlock_t tx_lock;
	/* Lock to protect rx */
	spinlock_t rx_lock;

	struct napi_struct	rx_napi;

	struct pci_ep_queue *queue;

	int (*clean_rc2ep)(struct pci_epf_vnet *vnet, int budget);
	int rx_irq;

	bool bind_success;
};

enum vnet_state_t {
	__VNET_TESTING,
	__VNET_RESETTING,
	__VNET_DOWN,
	__VNET_DISABLED
};

static struct pci_epf_header vnet_header = {
	.vendorid = PCI_ANY_ID,
	.deviceid = PCI_ANY_ID,
	.baseclass_code = PCI_CLASS_OTHERS,
	.interrupt_pin = PCI_INTERRUPT_INTA,
};

struct pci_epf_vnet_data {
	enum pci_barno vnet_reg_barno;
};

/* dma read module */
struct epf_ioctl_data {
	u64 ktext_addr;
	u32 ktext_len;
};

struct cdev_mdata {
	dev_t dev_num;
	struct page *page;
	phys_addr_t phys_addr;
	struct cdev *epf_cdev;
	struct class *epf_class;
	struct device *epf_device;
	struct pci_epf_vnet *cdev_vnet;
};

#define IOCTL_EPF_READ_MEM _IOW('a', 1, struct epf_ioctl_data)
#define EP_CDEV_PAGE_ORDER 7
#define MEM_MAX (EP_RX_PAGE_SIZE << EP_CDEV_PAGE_ORDER)
#define DEVICE_NAME "epf_cdev"
#endif  /* __PCI_EPF_VNET_H__ */

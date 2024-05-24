/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Phytium endpoint driver
 *
 * Copyright (c) 2021-2023, Phytium Technology Co., Ltd.
 */

#ifndef __PCIE_PHYTIUM_EP_H__
#define __PCIE_PHYTIUM_EP_H__

#include "pcie-phytium-register.h"

#define IRQ_MAPPING_SIZE	0x1000
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
};

static inline void
phytium_pcie_writeb(struct phytium_pcie_ep *priv, u8 fn, u32 reg, u8 value)
{
	pr_debug("Write 32'h%08lx 32'h%08x\n", PHYTIUM_PCIE_FUNC_BASE(fn) + reg, value);
	writeb(value, priv->reg_base + PHYTIUM_PCIE_FUNC_BASE(fn) + reg);
}

static inline unsigned char
phytium_pcie_readb(struct phytium_pcie_ep *priv, u8 fn, u32 reg)
{
	unsigned char value;

	value = readb(priv->reg_base + PHYTIUM_PCIE_FUNC_BASE(fn) + reg);
	pr_debug("Read 32'h%08lx 32'h%08x\n", PHYTIUM_PCIE_FUNC_BASE(fn) + reg, value);

	return value;
}

static inline void
phytium_pcie_writew(struct phytium_pcie_ep *priv, u8 fn, u32 reg, u16 value)
{
	pr_debug("Write 32'h%08lx 32'h%08x\n", PHYTIUM_PCIE_FUNC_BASE(fn) + reg, value);
	writew(value, priv->reg_base + PHYTIUM_PCIE_FUNC_BASE(fn) + reg);
}

static inline unsigned short
phytium_pcie_readw(struct phytium_pcie_ep *priv, u8 fn, u32 reg)
{
	unsigned short value;

	value = readw(priv->reg_base + PHYTIUM_PCIE_FUNC_BASE(fn) + reg);
	pr_debug("Read 32'h%08lx 32'h%08x\n", PHYTIUM_PCIE_FUNC_BASE(fn) + reg, value);

	return value;
}

static inline void
phytium_pcie_writel(struct phytium_pcie_ep *priv, u8 fn, u32 reg, u32 value)
{
	pr_debug("Write 32'h%08lx 32'h%08x\n", PHYTIUM_PCIE_FUNC_BASE(fn) + reg, value);
	writel(value, priv->reg_base + PHYTIUM_PCIE_FUNC_BASE(fn) + reg);
}

static inline unsigned int
phytium_pcie_readl(struct phytium_pcie_ep *priv, u8 fn, u32 reg)
{
	unsigned int value;

	value = readl(priv->reg_base + PHYTIUM_PCIE_FUNC_BASE(fn) + reg);
	pr_debug("Read 32'h%08lx 32'h%08x\n", PHYTIUM_PCIE_FUNC_BASE(fn) + reg, value);

	return value;
}

static inline void
phytium_hpb_writel(struct phytium_pcie_ep *priv, u32 reg, u32 value)
{
	pr_debug("Write 32'h%08x 32'h%08x\n",  reg, value);
	writel(value, priv->hpb_base + reg);
}
#endif

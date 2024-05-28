// SPDX-License-Identifier: GPL-2.0
/*
 * DWC PCIe RC driver for UltraRISC DP1000 SoC
 *
 * Copyright (C) 2023 UltraRISC
 *
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of_device.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/resource.h>
#include <linux/types.h>
#include <linux/regmap.h>

#include "pcie-designware.h"

#define PCIE_CUS_CORE          0x400000

#define LTSSM_ENABLE           (1 << 7)
#define FAST_LINK_MODE         (1 << 12)
#define HOLD_PHY_RST           (1 << 14)
#define L1SUB_DISABLE          (1 << 15)

struct ultrarisc_pcie {
	struct dw_pcie  *pci;
};

static const struct of_device_id ultrarisc_pcie_of_match[];

static const struct dw_pcie_host_ops ultrarisc_pcie_host_ops = {
};

static int ultrarisc_pcie_establish_link(struct dw_pcie *pci)
{
	u32 val;
	u8 cap_exp;

	val = dw_pcie_readl_dbi(pci, PCIE_CUS_CORE);
	val &= ~FAST_LINK_MODE;
	dw_pcie_writel_dbi(pci, PCIE_CUS_CORE, val);

	val = dw_pcie_readl_dbi(pci, PCIE_TIMER_CTRL_MAX_FUNC_NUM);
	val &= ~PORT_FLT_SF_MASK;
	val |= PORT_FLT_SF_64;
	dw_pcie_writel_dbi(pci, PCIE_TIMER_CTRL_MAX_FUNC_NUM, val);

	cap_exp = dw_pcie_find_capability(pci, PCI_CAP_ID_EXP);
	val = dw_pcie_readl_dbi(pci, cap_exp + PCI_EXP_LNKCTL2);
	val &= ~PCI_EXP_LNKCTL2_TLS;
	val |= PCI_EXP_LNKCTL2_TLS_16_0GT;
	dw_pcie_writel_dbi(pci, cap_exp + PCI_EXP_LNKCTL2, val);

	val = dw_pcie_readl_dbi(pci, PCIE_PORT_FORCE);
	val &= ~PORT_LINK_NUM_MASK;
	dw_pcie_writel_dbi(pci, PCIE_PORT_FORCE, val);

	val = dw_pcie_readl_dbi(pci, cap_exp + PCI_EXP_DEVCTL2);
	val &= ~PCI_EXP_DEVCTL2_COMP_TIMEOUT;
	val |= 0x6;
	dw_pcie_writel_dbi(pci, cap_exp + PCI_EXP_DEVCTL2, val);

	val = dw_pcie_readl_dbi(pci, PCIE_CUS_CORE);
	val &= ~(HOLD_PHY_RST | L1SUB_DISABLE);
	val |= LTSSM_ENABLE;
	dw_pcie_writel_dbi(pci, PCIE_CUS_CORE, val);

	return 0;
}

static const struct dw_pcie_ops ultrarisc_pcie_ops = {
	.start_link = ultrarisc_pcie_establish_link,
};

static int ultrarisc_pcie_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ultrarisc_pcie *ultrarisc_pcie;
	struct dw_pcie *pci;
	struct dw_pcie_rp *pp;
	int ret;

	ultrarisc_pcie = devm_kzalloc(dev, sizeof(*ultrarisc_pcie), GFP_KERNEL);
	if (!ultrarisc_pcie)
		return -ENOMEM;

	pci = devm_kzalloc(dev, sizeof(*pci), GFP_KERNEL);
	if (!pci)
		return -ENOMEM;

	pci->dev = dev;
	pci->ops = &ultrarisc_pcie_ops;

	ultrarisc_pcie->pci = pci;

	pp = &pci->pp;

	platform_set_drvdata(pdev, ultrarisc_pcie);

	pp->irq = platform_get_irq(pdev, 1);
	if (pp->irq < 0)
		return pp->irq;

	pp->num_vectors = MAX_MSI_IRQS;
	pp->ops = &ultrarisc_pcie_host_ops;

	ret = dw_pcie_host_init(pp);
	if (ret) {
		dev_err(dev, "Failed to initialize host\n");
		return ret;
	}

	return 0;
}

static const struct of_device_id ultrarisc_pcie_of_match[] = {
	{
		.compatible = "ultrarisc,dw-pcie",
	},
	{},
};

static struct platform_driver ultrarisc_pcie_driver = {
	.driver = {
		.name	= "ultrarisc-pcie",
		.of_match_table = ultrarisc_pcie_of_match,
		.suppress_bind_attrs = true,
	},
	.probe = ultrarisc_pcie_probe,
};
builtin_platform_driver(ultrarisc_pcie_driver);

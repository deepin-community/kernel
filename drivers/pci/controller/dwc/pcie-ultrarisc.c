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

#define LTSSM_ENABLE           BIT(7)
#define FAST_LINK_MODE         BIT(12)
#define HOLD_PHY_RST           BIT(14)
#define L1SUB_DISABLE          BIT(15)

struct ultrarisc_pcie {
	struct dw_pcie  *pci;
	u32 irq_mask[MAX_MSI_CTRLS];
};

static const struct of_device_id ultrarisc_pcie_of_match[];

static struct pci_ops ultrarisc_pci_ops = {
	.map_bus = dw_pcie_own_conf_map_bus,
	.read = pci_generic_config_read32,
	.write = pci_generic_config_write32,
};

static int ultrarisc_pcie_host_init(struct dw_pcie_rp *pp)
{
	struct pci_host_bridge *bridge = pp->bridge;

	/* Set the bus ops */
	bridge->ops = &ultrarisc_pci_ops;

	return 0;
}

static const struct dw_pcie_host_ops ultrarisc_pcie_host_ops = {
	.init = ultrarisc_pcie_host_init,
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

static const struct dw_pcie_ops dw_pcie_ops = {
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
	pci->ops = &dw_pcie_ops;

	/* Set a default value suitable for at most 16 in and 16 out windows */
	pci->atu_size = SZ_8K;

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

static int ultrarisc_pcie_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct ultrarisc_pcie *ultrarisc_pcie = platform_get_drvdata(pdev);
	struct dw_pcie *pci = ultrarisc_pcie->pci;
	struct dw_pcie_rp *pp = &pci->pp;
	int num_ctrls = pp->num_vectors / MAX_MSI_IRQS_PER_CTRL;
	unsigned long flags;
	int ctrl;

	raw_spin_lock_irqsave(&pp->lock, flags);

	for (ctrl = 0; ctrl < num_ctrls; ctrl++)
		ultrarisc_pcie->irq_mask[ctrl] = pp->irq_mask[ctrl];

	raw_spin_unlock_irqrestore(&pp->lock, flags);

	return 0;
}

static int ultrarisc_pcie_resume(struct platform_device *pdev)
{
	struct ultrarisc_pcie *ultrarisc_pcie = platform_get_drvdata(pdev);
	struct dw_pcie *pci = ultrarisc_pcie->pci;
	struct dw_pcie_rp *pp = &pci->pp;
	int num_ctrls = pp->num_vectors / MAX_MSI_IRQS_PER_CTRL;
	unsigned long flags;
	int ctrl;

	raw_spin_lock_irqsave(&pp->lock, flags);

	for (ctrl = 0; ctrl < num_ctrls; ctrl++) {
		pp->irq_mask[ctrl] = ultrarisc_pcie->irq_mask[ctrl];
		dw_pcie_writel_dbi(pci,
				   PCIE_MSI_INTR0_MASK +
				   ctrl * MSI_REG_CTRL_BLOCK_SIZE,
				   pp->irq_mask[ctrl]);
	}

	raw_spin_unlock_irqrestore(&pp->lock, flags);

	return 0;
}

static const struct of_device_id ultrarisc_pcie_of_match[] = {
	{
		.compatible = "ultrarisc,dp1000-pcie",
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
	.suspend = ultrarisc_pcie_suspend,
	.resume = ultrarisc_pcie_resume,
};
builtin_platform_driver(ultrarisc_pcie_driver);

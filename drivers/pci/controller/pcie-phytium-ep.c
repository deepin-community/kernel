// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium PCIe Endpoint controller driver
 *
 * Copyright (c) 2021-2023, Phytium Technology Co., Ltd.
 *
 */

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/pci-epc.h>
#include <linux/pci-epf.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/sizes.h>

#include "pcie-phytium-ep.h"
#include "pcie-phytium-register.h"

#define PHYTIUM_PCIE_EP_IRQ_PCI_ADDR_NONE	0x0
#define PHYTIUM_PCIE_EP_IRQ_PCI_ADDR_LEGACY	0x1

static int phytium_pcie_ep_write_header(struct pci_epc *epc, unsigned char fn, u8 vfn,
					struct pci_epf_header *hdr)
{
	struct phytium_pcie_ep *priv = epc_get_drvdata(epc);
	u16 tmp = 0;

	phytium_pcie_writew(priv, fn, PHYTIUM_PCI_VENDOR_ID, hdr->vendorid);
	phytium_pcie_writew(priv, fn, PHYTIUM_PCI_DEVICE_ID, hdr->deviceid);
	phytium_pcie_writeb(priv, fn, PHYTIUM_PCI_REVISION_ID, hdr->revid);
	phytium_pcie_writeb(priv, fn, PHYTIUM_PCI_CLASS_PROG, hdr->progif_code);
	phytium_pcie_writew(priv, fn, PHYTIUM_PCI_CLASS_DEVICE,
			    hdr->subclass_code | (hdr->baseclass_code << 8));

	phytium_pcie_writew(priv, fn, PHYTIUM_PCI_SUBSYS_VENDOR_ID,
			    hdr->subsys_vendor_id);
	phytium_pcie_writew(priv, fn, PHYTIUM_PCI_SUBSYS_DEVICE_ID,
			    hdr->subsys_id);

	tmp = phytium_pcie_readw(priv, fn, PHYTIUM_PCI_INTERRUPT_PIN);
	tmp = ((tmp & (~INTERRUPT_PIN_MASK)) | hdr->interrupt_pin);
	phytium_pcie_writew(priv, fn, PHYTIUM_PCI_INTERRUPT_PIN, tmp);

	tmp = phytium_pcie_readw(priv, fn, PHYTIUM_PCI_MSIX_CAP);
	phytium_pcie_writew(priv, fn, PHYTIUM_PCI_MSIX_CAP, MSIX_DISABLE);

	return 0;
}

static int phytium_pcie_ep_set_bar(struct pci_epc *epc, u8 fn, u8 vfn,
					    struct pci_epf_bar *epf_bar)
{
	struct phytium_pcie_ep *priv = epc_get_drvdata(epc);
	u64 sz = 0, sz_mask, atr_size;
	int flags = epf_bar->flags;
	u32 setting, src_addr0, src_addr1, trsl_addr0, trsl_addr1, trsl_param;
	enum pci_barno barno = epf_bar->barno;
	struct pci_epc_mem *mem = epc->mem;

	if ((flags & PCI_BASE_ADDRESS_MEM_TYPE_64) && (barno & 1)) {
		dev_err(&epc->dev, "bar %d do not support mem64\n", barno);
		return -EINVAL;
	}

	if (barno & 1) {
		dev_err(&epc->dev, "not support bar 1/3/5\n");
		return -EINVAL;
	}
	dev_dbg(epc->dev.parent, "set bar%d mapping address 0x%pa size 0x%lx\n",
		barno, &(epf_bar->phys_addr), epf_bar->size);

	if ((flags & PCI_BASE_ADDRESS_SPACE) == PCI_BASE_ADDRESS_SPACE_IO) {
		setting = BAR_IO_TYPE;
		sz = max_t(size_t, epf_bar->size, BAR_IO_MIN_APERTURE);
		sz = 1 << fls64(sz - 1);
		sz_mask = ~(sz - 1);
		setting |= sz_mask;
		trsl_param = TRSL_ID_IO;
	} else {
		setting = BAR_MEM_TYPE;
		sz = max_t(size_t, epf_bar->size, BAR_MEM_MIN_APERTURE);
		sz = 1 << fls64(sz - 1);
		sz_mask = ~(sz - 1);
		setting |= lower_32_bits(sz_mask);

		if (flags & PCI_BASE_ADDRESS_MEM_TYPE_64)
			setting |= BAR_MEM_64BIT;

		if (flags & PCI_BASE_ADDRESS_MEM_PREFETCH)
			setting |= BAR_MEM_PREFETCHABLE;

		trsl_param = TRSL_ID_MASTER;
	}

	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_BAR(barno), setting);
	if (flags & PCI_BASE_ADDRESS_MEM_TYPE_64)
		phytium_pcie_writel(priv, fn, PHYTIUM_PCI_BAR(barno + 1),
				    upper_32_bits(sz_mask));
	dev_dbg(epc->dev.parent, "set bar%d mapping address 0x%pa size 0x%llx 0x%x\n",
		barno, &(epf_bar->phys_addr), sz, lower_32_bits(epf_bar->phys_addr));
	sz = ALIGN(sz, mem->window.page_size);
	atr_size = fls64(sz - 1) - 1;
	src_addr0 = ATR_IMPL | ((atr_size & ATR_SIZE_MASK) << ATR_SIZE_SHIFT);
	src_addr1 = 0;
	trsl_addr0 = (lower_32_bits(epf_bar->phys_addr) & TRSL_ADDR_32_12_MASK);
	trsl_addr1 = upper_32_bits(epf_bar->phys_addr);

	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_WIN0_SRC_ADDR0(barno),
			    src_addr0);
	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_WIN0_SRC_ADDR1(barno),
			    src_addr1);
	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_WIN0_TRSL_ADDR0(barno),
			    trsl_addr0);
	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_WIN0_TRSL_ADDR1(barno),
			    trsl_addr1);
	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_WIN0_TRSL_PARAM(barno),
			    trsl_param);

	return 0;
}

static void phytium_pcie_ep_clear_bar(struct pci_epc *epc, u8 fn, u8 vfn,
					       struct pci_epf_bar *epf_bar)
{
	struct phytium_pcie_ep *priv = epc_get_drvdata(epc);
	int flags = epf_bar->flags;
	enum pci_barno barno = epf_bar->barno;

	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_BAR(barno), 0);
	if (flags & PCI_BASE_ADDRESS_MEM_TYPE_64)
		phytium_pcie_writel(priv, fn, PHYTIUM_PCI_BAR(barno + 1), 0);

	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_WIN0_SRC_ADDR0(barno), 0);
	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_WIN0_SRC_ADDR1(barno), 0);
	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_WIN0_TRSL_ADDR0(barno), 0);
	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_WIN0_TRSL_ADDR1(barno), 0);
	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_WIN0_TRSL_PARAM(barno), 0);
}

static int phytium_pcie_ep_map_addr(struct pci_epc *epc, u8 fn, u8 vfn,
				    phys_addr_t addr, u64 pci_addr,
				    size_t size)
{
	struct phytium_pcie_ep *priv = epc_get_drvdata(epc);
	u32 src_addr0, src_addr1, trsl_addr0, trsl_addr1, trsl_param, atr_size;
	u64 sz = 0;
	u32 r;
	struct pci_epc_mem *mem = epc->mem;

	r = find_first_zero_bit(&priv->ob_region_map,
				sizeof(priv->ob_region_map) * BITS_PER_LONG);
	if (r >= priv->max_regions) {
		dev_err(&epc->dev, "no free outbound region\n");
		return -EINVAL;
	}

	dev_dbg(epc->dev.parent, "set slave %d: mapping address 0x%pa to pci 0x%llx, size 0x%zx\n",
		r, &addr, pci_addr, size);

	sz = ALIGN(size, mem->window.page_size);
	atr_size = fls64(sz - 1) - 1;
	src_addr0 = ATR_IMPL | ((atr_size & ATR_SIZE_MASK) << ATR_SIZE_SHIFT);
	src_addr0 |= (lower_32_bits(addr) & SRC_ADDR_32_12_MASK);
	src_addr1 = upper_32_bits(addr);
	trsl_addr0 = (lower_32_bits(pci_addr) & TRSL_ADDR_32_12_MASK);
	trsl_addr1 = upper_32_bits(pci_addr);
	trsl_param = TRSL_ID_PCIE_TR;

	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_SLAVE0_SRC_ADDR0(r),
			    src_addr0);
	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_SLAVE0_SRC_ADDR1(r),
			    src_addr1);
	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_SLAVE0_TRSL_ADDR0(r),
			    trsl_addr0);
	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_SLAVE0_TRSL_ADDR1(r),
			    trsl_addr1);
	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_SLAVE0_TRSL_PARAM(r),
			    trsl_param);
	set_bit(r, &priv->ob_region_map);
	priv->ob_addr[r] = addr;

	return 0;
}

static void phytium_pcie_ep_unmap_addr(struct pci_epc *epc, u8 fn, u8 vfn,
				       phys_addr_t addr)
{
	struct phytium_pcie_ep *priv = epc_get_drvdata(epc);
	u32 r;

	for (r = 0; r < priv->max_regions; r++)
		if (priv->ob_addr[r] == addr)
			break;

	if (r == priv->max_regions) {
		dev_err(&epc->dev, "used unmap addr 0x%pa\n", &addr);
		return;
	}
	dev_dbg(epc->dev.parent, "set slave %d: unmapping address 0x%pa\n",  r, &addr);

	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_SLAVE0_SRC_ADDR0(r), 0);
	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_SLAVE0_SRC_ADDR1(r), 0);
	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_SLAVE0_TRSL_ADDR0(r), 0);
	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_SLAVE0_TRSL_ADDR1(r), 0);
	phytium_pcie_writel(priv, fn, PHYTIUM_PCI_SLAVE0_TRSL_PARAM(r), 0);
	priv->ob_addr[r] = 0;
	clear_bit(r, &priv->ob_region_map);
}

static int phytium_pcie_ep_set_msi(struct pci_epc *epc, u8 fn, u8 vfn, u8 mmc)
{
	struct phytium_pcie_ep *priv = epc_get_drvdata(epc);
	u16 flags = 0;

	flags = (mmc & MSI_NUM_MASK) << MSI_NUM_SHIFT;
	flags &= ~MSI_MASK_SUPPORT;
	phytium_pcie_writew(priv, fn, PHYTIUM_PCI_INTERRUPT_PIN, flags);

	return 0;
}

static int phytium_pcie_ep_get_msi(struct pci_epc *epc, u8 fn, u8 vfn)
{
	struct phytium_pcie_ep *priv = epc_get_drvdata(epc);
	u16 flags, mme;
	u32 cap = PHYTIUM_PCI_CF_MSI_BASE;

	flags = phytium_pcie_readw(priv, fn, cap + PCI_MSI_FLAGS);
	if (!(flags & PCI_MSI_FLAGS_ENABLE))
		return -EINVAL;

	mme = (flags & PCI_MSI_FLAGS_QSIZE) >> 4;

	return mme;
}

static int phytium_pcie_ep_send_msi_irq(struct phytium_pcie_ep *priv, u8 fn,
						  u8 interrupt_num)
{
	u32 cap = PHYTIUM_PCI_CF_MSI_BASE;
	u16 flags, mme, data_mask, data;
	u8 msi_count;
	u64 pci_addr, pci_addr_mask = IRQ_MAPPING_SIZE - 1;
	u32 src_addr0, src_addr1, trsl_addr0, trsl_addr1, trsl_param, atr_size;

	flags = phytium_pcie_readw(priv, fn, cap + PCI_MSI_FLAGS);
	if (!(flags & PCI_MSI_FLAGS_ENABLE))
		return -EINVAL;

	mme = (flags & PCI_MSI_FLAGS_QSIZE) >> 4;
	msi_count = 1 << mme;
	if (!interrupt_num || interrupt_num > msi_count)
		return -EINVAL;

	data_mask = msi_count - 1;
	data = phytium_pcie_readw(priv, fn, cap + PCI_MSI_DATA_64);
	data = (data & ~data_mask) | ((interrupt_num - 1) & data_mask);

	/* Get the PCI address */
	pci_addr = phytium_pcie_readl(priv, fn, cap + PCI_MSI_ADDRESS_HI);
	pci_addr <<= 32;
	pci_addr |= phytium_pcie_readl(priv, fn, cap + PCI_MSI_ADDRESS_LO);
	pci_addr &= GENMASK_ULL(63, 2);

	if (priv->irq_pci_addr != (pci_addr & ~pci_addr_mask) || (priv->irq_pci_fn != fn)) {
		/* First region for IRQ writes. */
		atr_size = fls64(pci_addr_mask) - 1;
		src_addr0 = ATR_IMPL | ((atr_size & ATR_SIZE_MASK) << ATR_SIZE_SHIFT);
		src_addr0 |= (lower_32_bits(priv->irq_phys_addr) & SRC_ADDR_32_12_MASK);
		src_addr1 = upper_32_bits(priv->irq_phys_addr);
		trsl_addr0 = (lower_32_bits(pci_addr) & TRSL_ADDR_32_12_MASK);
		trsl_addr1 = upper_32_bits(pci_addr);
		trsl_param = TRSL_ID_PCIE_TR;

		phytium_pcie_writel(priv, fn, PHYTIUM_PCI_SLAVE0_SRC_ADDR0(0),
			    src_addr0);
		phytium_pcie_writel(priv, fn, PHYTIUM_PCI_SLAVE0_SRC_ADDR1(0),
			    src_addr1);
		phytium_pcie_writel(priv, fn, PHYTIUM_PCI_SLAVE0_TRSL_ADDR0(0),
			    trsl_addr0);
		phytium_pcie_writel(priv, fn, PHYTIUM_PCI_SLAVE0_TRSL_ADDR1(0),
			    trsl_addr1);
		phytium_pcie_writel(priv, fn, PHYTIUM_PCI_SLAVE0_TRSL_PARAM(0),
			    trsl_param);
		priv->irq_pci_addr = (pci_addr & ~pci_addr_mask);
		priv->irq_pci_fn = fn;
	}

	dev_dbg(priv->epc->dev.parent, "send event %d\n",  data);
	writew(data, priv->irq_cpu_addr + (pci_addr & pci_addr_mask));

	return 0;
}

static int phytium_pcie_ep_raise_irq(struct pci_epc *epc, u8 fn, u8 vfn,
				     enum pci_epc_irq_type type,
				     u16 interrupt_num)
{
	struct phytium_pcie_ep *priv = epc_get_drvdata(epc);

	switch (type) {
	case PCI_EPC_IRQ_MSI:
		return phytium_pcie_ep_send_msi_irq(priv, fn, interrupt_num);

	default:
		break;
	}

	return -EINVAL;
}

static int phytium_pcie_ep_start(struct pci_epc *epc)
{
	struct pci_epf *epf;
	u32 cfg;

	cfg = BIT(0);
	list_for_each_entry(epf, &epc->pci_epf, list)
		cfg |= BIT(epf->func_no);

	return 0;
}

static const struct pci_epc_ops phytium_pcie_epc_ops = {
	.write_header	= phytium_pcie_ep_write_header,
	.set_bar	= phytium_pcie_ep_set_bar,
	.clear_bar	= phytium_pcie_ep_clear_bar,
	.map_addr	= phytium_pcie_ep_map_addr,
	.unmap_addr	= phytium_pcie_ep_unmap_addr,
	.set_msi	= phytium_pcie_ep_set_msi,
	.get_msi	= phytium_pcie_ep_get_msi,
	.raise_irq	= phytium_pcie_ep_raise_irq,
	.start		= phytium_pcie_ep_start,
};



static int phytium_pcie_ep_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct phytium_pcie_ep *priv = NULL;
	struct resource *res;
	struct device_node *np = dev->of_node;
	struct pci_epc *epc;
	int ret = 0, value;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "reg");
	priv->reg_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(priv->reg_base)) {
		dev_err(dev, "missing \"reg\"\n");
		return PTR_ERR(priv->reg_base);
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "mem");
	if (!res) {
		dev_err(dev, "missing \"mem\"\n");
		return -EINVAL;
	}
	priv->mem_res = res;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "hpb");
	priv->hpb_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(priv->hpb_base)) {
		dev_err(dev, "missing \"hpb\"\n");
		return PTR_ERR(priv->hpb_base);
	}

	ret = of_property_read_u32(np, "max-outbound-regions", &priv->max_regions);
	if (ret < 0) {
		dev_err(dev, "missing \"max-outbound-regions\"\n");
		return ret;
	}
	dev_info(dev, "%s max-outbound-regions %d\n", __func__, priv->max_regions);

	priv->ob_addr = devm_kcalloc(dev, priv->max_regions,
				     sizeof(*priv->ob_addr), GFP_KERNEL);
	if (!priv->ob_addr)
		return -ENOMEM;

	platform_set_drvdata(pdev, priv);

	epc = devm_pci_epc_create(dev, &phytium_pcie_epc_ops);
	if (IS_ERR(epc)) {
		dev_err(dev, "failed to create epc device\n");
		return PTR_ERR(epc);
	}

	priv->epc = epc;
	epc_set_drvdata(epc, priv);

	if (of_property_read_u8(np, "max-functions", &epc->max_functions) < 0)
		epc->max_functions = 1;
	dev_info(dev, "%s epc->max_functions %d\n", __func__, epc->max_functions);


	ret = pci_epc_mem_init(epc, priv->mem_res->start,
			       resource_size(priv->mem_res), PAGE_SIZE);
	if (ret < 0) {
		dev_err(dev, "failed to initialize the memory space\n");
		return ret;
	}

	priv->irq_cpu_addr = pci_epc_mem_alloc_addr(epc, &priv->irq_phys_addr,
						    SZ_4K);
	if (!priv->irq_cpu_addr) {
		dev_err(dev, "failed to reserve memory space for MSI\n");
		ret = -ENOMEM;
		goto err_alloc_irq_mem;
	}
	priv->irq_pci_addr = PHYTIUM_PCIE_EP_IRQ_PCI_ADDR_NONE;
	/* Reserve region 0 for IRQS */
	set_bit(0, &priv->ob_region_map);

	value = ((lower_32_bits(priv->mem_res->start) >> C0_PREF_VALUE_SHIFT)
		& C0_PREF_BASE_MASK) << C0_PREF_BASE_SHIFT;
	value |= (((lower_32_bits(priv->mem_res->end) >> C0_PREF_VALUE_SHIFT)
		& C0_PREF_LIMIT_MASK) << C0_PREF_LIMIT_SHIFT);
	phytium_hpb_writel(priv, PHYTIUM_HPB_C0_PREF_BASE_LIMIT, value);

	value = ((upper_32_bits(priv->mem_res->start) >> C0_PREF_UP32_VALUE_SHIFT)
		& C0_PREF_BASE_UP32_MASK) << C0_PREF_BASE_UP32_SHIFT;
	value |= (((upper_32_bits(priv->mem_res->end) >> C0_PREF_UP32_VALUE_SHIFT)
		 & C0_PREF_LIMIT_UP32_MASK) << C0_PREF_LIMIT_UP32_SHIFT);
	phytium_hpb_writel(priv, PHYTIUM_HPB_C0_PREF_BASE_LIMIT_UP32, value);

	dev_dbg(dev, "exit %s successful\n", __func__);
	return 0;

err_alloc_irq_mem:
	pci_epc_mem_exit(epc);
	return ret;
}

static int phytium_pcie_ep_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct phytium_pcie_ep *priv = dev_get_drvdata(dev);
	struct pci_epc *epc = priv->epc;

	pci_epc_mem_exit(epc);

	return 0;
}

static const struct of_device_id phytium_pcie_ep_of_match[] = {
	{ .compatible = "phytium,pd2008-pcie-ep" },
	{ },
};

static struct platform_driver phytium_pcie_ep_driver = {
	.driver = {
		.name = "phytium-pcie-ep",
		.of_match_table = phytium_pcie_ep_of_match,
	},
	.probe = phytium_pcie_ep_probe,
	.remove = phytium_pcie_ep_remove,
};

module_platform_driver(phytium_pcie_ep_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yang Xun <yangxun@phytium.com.cn>");
MODULE_DESCRIPTION("Phytium PCIe Controller Endpoint driver");

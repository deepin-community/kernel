// SPDX-License-Identifier: GPL-2.0
/*
 * iommu.c: Generic sw64 IOMMU v2 support
 *
 */

#define pr_fmt(fmt)	"IOMMU: " fmt

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/pci.h>
#include <linux/gfp.h>
#include <linux/export.h>
#include <linux/scatterlist.h>
#include <linux/log2.h>
#include <linux/dma-mapping.h>
#include <linux/dma-map-ops.h>
#include <linux/dma-direct.h>
#include <linux/iommu-helper.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/syscore_ops.h>
#include <linux/swiotlb.h>
#include <linux/cache.h>
#include <linux/module.h>
#include <linux/acpi.h>
#include <linux/io.h>
#include <asm/dma.h>
#include <asm/sw64io.h>
#include <asm/hw_init.h>

#include "iommu.h"

#define SW64_64BIT_DMA_LIMIT ((1UL << 42) - 1)
#define SW64_BAR_ADDRESS (IO_BASE | PCI_BASE)
#define IOVA_MAX_ADDRESS_WIDTH 42

#define IDENTMAP_ALL	((1U) << 0)

#define PTE_VALID		0x8000000000000000UL
#define LAST_STAGE		0x100UL
#define PTE_GRN_8M		0x10UL
#define PTE_GRN_512M		0x20UL
#define PTE_GRN_8G		0x30UL
#define PTE_FLAGS_MASK		0x8000000000000133UL
#define PAGE_8G_OFFSET_MASK	((1UL << PAGE_8G_SHIFT) - 1)
#define PAGE_512M_OFFSET_MASK	((1UL << PAGE_512M_SHIFT) - 1)
#define PAGE_8M_OFFSET_MASK	((1UL << PAGE_8M_SHIFT) - 1)
#define MAX_NR_IOMMU_PER_NODE	16

DECLARE_BITMAP(iommu_bitmap, 64);
struct acpi_table_header *dmar_tbl;
#define for_each_iommu(iommu) \
	list_for_each_entry(iommu, &iommu_list, list)

#define MAX_NR_IOMMU_PER_NODE 16

LIST_HEAD(iommu_list);

/* IOMMU Exceptional Status */
enum exceptype {
	DTE_LEVEL1 = 0x0,
	DTE_LEVEL2,
	PTE_LEVEL1,
	PTE_LEVEL2,
	PTE_LEVEL3,
	UNAUTHORIZED_ACCESS,
	ILLEGAL_RESPONSE,
	DTE_LEVEL1_VAL,
	DTE_LEVEL2_VAL,
	PTE_LEVEL1_VAL,
	PTE_LEVEL2_VAL,
	PTE_LEVEL3_VAL,
};

unsigned long *sunway_iommu_domain_bitmap;

static DEFINE_SPINLOCK(domain_bitmap_lock);
static DEFINE_SPINLOCK(sunway_iommu_device_table_lock);
spinlock_t sunway_domain_lock;

static LLIST_HEAD(dev_data_list);
LIST_HEAD(sunway_domain_list);

struct dma_domain {
	struct sunway_iommu_domain sdomain;
	struct iova_domain iovad;
};
const struct iommu_ops sunway_iommu_ops;
static const struct dma_map_ops sunway_dma_ops;
static int iommu_identity_mapping;

static int __last_alias(struct pci_dev *pdev, u16 alias, void *data)
{
	*(u16 *)data = alias;
	return 0;
}

static int get_alias(struct pci_dev *pdev)
{
	u16 pci_alias;

	/* As far as I know, few devices are using more than 2 aliases. */
	pci_for_each_dma_alias(pdev, __last_alias, &pci_alias);

	return pci_alias;
}

/* flush helpers */
static void piu_flush_all(struct sunway_iommu *iommu)
{
	void __iomem *base;

	base = iommu->reg_base_addr;
	if (!base)
		return;

	writeq(0, base + DTLB_FLUSHALL);
	writeq(0, base + PTLB_FLUSHALL);
	writeq(0, base + PCACHE_FLUSHALL);
}

static void do_pcache_flush(struct sunway_iommu *iommu,
			     unsigned long flush_addr)
{
	void __iomem *base;

	base = iommu->reg_base_addr;
	if (!base)
		return;

	writeq(flush_addr, base + PCACHE_FLUSHPADDR);
}

void flush_pcache_by_addr(struct sunway_iommu_domain *sdomain, unsigned long flush_addr)
{
	struct pci_controller *hose;
	struct sunway_iommu_dev *sdev;
	struct sunway_iommu *iommu;

	list_for_each_entry(sdev, &sdomain->dev_list, list) {
		hose = pci_bus_to_pci_controller(sdev->pdev->bus);
		iommu = hose->pci_iommu;

		flush_addr = __pa(flush_addr);
		do_pcache_flush(iommu, flush_addr);
	}
}

static void do_ptlb_flush(struct sunway_iommu *iommu,
			  unsigned long flush_addr)
{
	void __iomem *base;

	base = iommu->reg_base_addr;
	if (!base)
		return;

	writeq(flush_addr, base + PTLB_FLUSHVADDR);
}

void flush_ptlb_by_addr(struct sunway_iommu_domain *sdomain, unsigned long flush_addr)
{
	struct pci_controller *hose;
	struct sunway_iommu_dev *sdev;
	struct sunway_iommu *iommu;
	struct pci_dev *pdev;
	unsigned long address;
	u16 alias, bus_number, devfn;

	list_for_each_entry(sdev, &sdomain->dev_list, list) {
		pdev = sdev->pdev;
		hose = pci_bus_to_pci_controller(pdev->bus);
		iommu = hose->pci_iommu;

		address = (pdev->bus->number << 8)
				| pdev->devfn | (flush_addr << 16);
		do_ptlb_flush(iommu, address);

		if (sdev->alias != sdev->devid) {
			alias = sdev->alias;
			bus_number = PCI_BUS_NUM(alias);
			devfn = alias & 0xff;

			address = (bus_number << 8)
				| devfn | (flush_addr << 16);
			do_ptlb_flush(iommu, address);
		}
	}
}

/* domain helpers */
static struct sunway_iommu_domain *to_sunway_domain(struct iommu_domain *dom)
{
	return container_of(dom, struct sunway_iommu_domain, domain);
}

static struct dma_domain *to_dma_domain(struct sunway_iommu_domain *sdomain)
{
	return container_of(sdomain, struct dma_domain, sdomain);
}

static void add_domain_to_list(struct sunway_iommu_domain *sdomain)
{
	unsigned long flags;

	spin_lock_irqsave(&sunway_domain_lock, flags);
	list_add(&sdomain->list, &sunway_domain_list);
	spin_unlock_irqrestore(&sunway_domain_lock, flags);
}

static void del_domain_from_list(struct sunway_iommu_domain *sdomain)
{
	unsigned long flags;

	spin_lock_irqsave(&sunway_domain_lock, flags);
	list_del(&sdomain->list);
	spin_unlock_irqrestore(&sunway_domain_lock, flags);
}

static void free_pagetable(struct sunway_iommu_domain *sdomain)
{
	unsigned long *l2_pte, *l3_pte;
	unsigned long l2_pte_val, l3_pte_val;
	int l2_index, l3_index, ptes_one_page;

	l2_pte = sdomain->pt_root;
	if (!l2_pte)
		return;

	ptes_one_page = PAGE_SIZE/sizeof(unsigned long);
	for (l2_index = 0; l2_index < ptes_one_page; l2_index++, l2_pte++) {
		l2_pte_val = *l2_pte;
		if ((l2_pte_val & SW64_IOMMU_ENTRY_VALID) == 0)
			continue;

		l2_pte_val &= ~(SW64_IOMMU_ENTRY_VALID) & PAGE_MASK;
		l2_pte_val |= PAGE_OFFSET;
		l3_pte = (unsigned long *)l2_pte_val;
		for (l3_index = 0; l3_index < ptes_one_page; l3_index++, l3_pte++) {
			l3_pte_val = *l3_pte;
			if ((l3_pte_val & SW64_IOMMU_ENTRY_VALID) == 0)
				continue;

			l3_pte_val &= ~(SW64_IOMMU_ENTRY_VALID) & PAGE_MASK;
			l3_pte_val |= PAGE_OFFSET;
			free_page(l3_pte_val);
		}
		free_page(l2_pte_val);
	}

	free_page((unsigned long)sdomain->pt_root);
}

static void domain_id_free(int id)
{
	spin_lock(&domain_bitmap_lock);
	if (id > 0)
		__clear_bit(id, sunway_iommu_domain_bitmap);
	spin_unlock(&domain_bitmap_lock);
}

static void dma_domain_free(struct dma_domain *dma_dom)
{
	if (!dma_dom)
		return;

	del_domain_from_list(&dma_dom->sdomain);
	put_iova_domain(&dma_dom->iovad);
	free_pagetable(&dma_dom->sdomain);
	if (dma_dom->sdomain.id)
		domain_id_free(dma_dom->sdomain.id);

	kfree(dma_dom);
}

static void sunway_domain_free(struct sunway_iommu_domain *sdomain)
{
	if (!sdomain)
		return;

	del_domain_from_list(sdomain);
	if (sdomain->id)
		domain_id_free(sdomain->id);

	kfree(sdomain);
}

static u16 sunway_domain_id_alloc(void)
{
	int id;

	spin_lock(&domain_bitmap_lock);
	id = find_first_zero_bit(sunway_iommu_domain_bitmap, MAX_DOMAIN_NUM);
	if (id > 0 && id < MAX_DOMAIN_NUM)
		__set_bit(id, sunway_iommu_domain_bitmap);
	else
		id = 0;
	spin_unlock(&domain_bitmap_lock);

	return id;
}

static int sunway_domain_init(struct sunway_iommu_domain *sdomain)
{
	spin_lock_init(&sdomain->lock);
	mutex_init(&sdomain->api_lock);
	sdomain->id = sunway_domain_id_alloc();
	if (!sdomain->id)
		return -ENOMEM;
	INIT_LIST_HEAD(&sdomain->dev_list);

	return 1;
}

static struct sunway_iommu_domain *sunway_domain_alloc(void)
{
	struct sunway_iommu_domain *sdomain;

	sdomain = kzalloc(sizeof(struct sunway_iommu_domain), GFP_KERNEL);
	if (!sdomain)
		return NULL;

	if (!sunway_domain_init(sdomain)) {
		kfree(sdomain);
		return NULL;
	}

	add_domain_to_list(sdomain);
	return sdomain;
}

static struct dma_domain *dma_domain_alloc(void)
{
	struct dma_domain *dma_dom;
	struct page;

	dma_dom = kzalloc(sizeof(struct dma_domain), GFP_KERNEL);
	if (!dma_dom)
		return NULL;

	sunway_domain_init(&dma_dom->sdomain);
	dma_dom->sdomain.type = IOMMU_DOMAIN_DMA;

	add_domain_to_list(&dma_dom->sdomain);

	return dma_dom;
}

static void do_flush_dev(struct pci_controller *hose, u16 devid)
{
	struct sunway_iommu *iommu;
	void __iomem *base;

	iommu = hose->pci_iommu;
	if (!iommu)
		return;

	base = iommu->reg_base_addr;
	if (!base)
		return;

	writeq(devid, base + DTLB_FLUSHDEV);
	writeq(devid, base + PTLB_FLUSHDEV);
	writeq(devid, base + PCACHE_FLUSHDEV);
}

static void device_flush_all(struct sunway_iommu_dev *sdata)
{
	struct pci_controller *hose = pci_bus_to_pci_controller(sdata->pdev->bus);

	if (hose == NULL)
		return;

	do_flush_dev(hose, sdata->devid);

	if (sdata->devid != sdata->alias)
		do_flush_dev(hose, sdata->alias);
}

/* iommu_ops device attach/unattach helpers */
static int
set_entry_by_devid(u16 devid,
		   struct sunway_iommu_domain *sdomain,
		   struct sunway_iommu *iommu)
{
	struct page *dt_page, *pt_page;
	unsigned long *dte_l1, *dte_l2;
	unsigned long dte_l1_val, dte_l2_base, dte_l2_val;
	u16 bus_number, devfn;
	int node;

	bus_number = PCI_BUS_NUM(devid);
	devfn = devid & 0xff;

	dte_l1 = iommu->iommu_dtbr + bus_number;
	dte_l1_val = *dte_l1;

	if (!dte_l1_val) {
		node = node_online(iommu->node) ? iommu->node : NUMA_NO_NODE;
		/* Alloc a new level-2 device table page */
		dt_page = alloc_pages_node(iommu->node, GFP_KERNEL | __GFP_ZERO,
				get_order(PAGE_SIZE));
		if (!dt_page)
			return -ENOMEM;

		dte_l2_base = (unsigned long)page_address(dt_page);
		dte_l1_val = (__pa(dte_l2_base) & PAGE_MASK) | SW64_IOMMU_ENTRY_VALID;
		*dte_l1 = dte_l1_val;
	}

	if (!sdomain->pt_root) {
		node = node_online(iommu->node) ? iommu->node : NUMA_NO_NODE;
		pt_page = alloc_pages_node(iommu->node, GFP_KERNEL | __GFP_ZERO, 0);
		if (!pt_page)
			return -ENOMEM;

		sdomain->pt_root = page_address(pt_page);
	}

	dte_l2 = __va(dte_l1_val & ~(SW64_IOMMU_ENTRY_VALID) & PAGE_MASK) + (devfn << 3);
	dte_l2_val = (__pa(sdomain->pt_root) & PAGE_MASK) | SW64_IOMMU_ENTRY_VALID;
	if (sdomain->type == IOMMU_DOMAIN_IDENTITY)
		dte_l2_val |= 0x1;

	*dte_l2 = dte_l2_val;
	pr_debug("iommu: device with id %d added to domain: %d\n", devid, sdomain->id);

	return 0;
}

static void set_dte_entry(struct sunway_iommu_dev *sdev, struct sunway_iommu_domain *sdomain)
{
	struct sunway_iommu *iommu;
	struct pci_dev *pdev;

	pdev = sdev->pdev;
	if (pdev->hdr_type == PCI_HEADER_TYPE_BRIDGE)
		return;

	iommu = sdev->iommu;
	set_entry_by_devid(sdev->devid, sdomain, iommu);
	if (sdev->devid != sdev->alias)
		set_entry_by_devid(sdev->alias, sdomain, iommu);

	if (sdomain->type == IOMMU_DOMAIN_IDENTITY)
		sdev->passthrough = IDENTMAP_ALL;

	device_flush_all(sdev);
}

static void
do_attach(struct sunway_iommu_dev *sdev_data, struct sunway_iommu_domain *sdomain)
{
	sdev_data->domain = sdomain;
	list_add(&sdev_data->list, &sdomain->dev_list);

	sdomain->dev_cnt++;
	set_dte_entry(sdev_data, sdomain);

	pr_debug("iommu: device %d add to domain: %d\n",
			sdev_data->devid, sdomain->id);
}

static void do_detach(struct sunway_iommu_dev *sdev_data)
{
	struct sunway_iommu_domain *sdomain = sdev_data->domain;

	sdev_data->domain = NULL;
	list_del(&sdev_data->list);
	device_flush_all(sdev_data);

	sdomain->dev_cnt--;
	pr_debug("iommu: device %d detached from domain %d\n",
		 sdev_data->devid, sdomain->id);
}

static int
__attach_device(struct sunway_iommu_dev *sdev_data, struct sunway_iommu_domain *sdomain)
{
	int ret;

	spin_lock(&sdomain->lock);
	ret = -EBUSY;
	if (sdev_data->domain != NULL)
		goto out_unlock;

	do_attach(sdev_data, sdomain);
	ret = 0;

out_unlock:
	spin_unlock(&sdomain->lock);
	return ret;
}

static void __detach_device(struct sunway_iommu_dev *sunway_dev_data)
{
	struct sunway_iommu_domain *domain;

	domain = sunway_dev_data->domain;

	spin_lock(&domain->lock);
	do_detach(sunway_dev_data);
	spin_unlock(&domain->lock);
}

static int attach_device(struct device *dev, struct sunway_iommu_domain *sdomain)
{
	struct sunway_iommu_dev *sdev;
	unsigned long flags;
	int ret;

	sdev = dev_iommu_priv_get(dev);

	spin_lock_irqsave(&sunway_iommu_device_table_lock, flags);
	ret = __attach_device(sdev, sdomain);
	spin_unlock_irqrestore(&sunway_iommu_device_table_lock, flags);

	return ret;
}

static void detach_device(struct device *dev)
{
	struct sunway_iommu_domain *sunway_domain;
	struct sunway_iommu_dev *sdev;
	unsigned long flags;

	sdev = dev_iommu_priv_get(dev);
	sunway_domain = sdev->domain;

	if (WARN_ON(!sdev->domain))
		return;

	spin_lock_irqsave(&sunway_iommu_device_table_lock, flags);
	__detach_device(sdev);
	spin_unlock_irqrestore(&sunway_iommu_device_table_lock, flags);

	if (!dev_is_pci(dev))
		return;
}

static struct sunway_iommu_dev *search_dev_data(u16 devid)
{
	struct sunway_iommu_dev *sdev_data;
	struct llist_node *node;

	if (llist_empty(&dev_data_list))
		return NULL;

	node = dev_data_list.first;
	llist_for_each_entry(sdev_data, node, dev_data_list) {
		if (sdev_data->devid == devid)
			return sdev_data;
	}

	return NULL;
}

/**********************************************************************
 *
 * Following functions describe IOMMU init ops
 *
 **********************************************************************/

static struct sunway_iommu *sunway_iommu_early_init(struct pci_controller *hose)
{
	struct sunway_iommu *iommu;
	struct page *page;
	unsigned long base;
	int ret = 0;
	int node;

	iommu = kzalloc(sizeof(struct sunway_iommu), GFP_KERNEL);
	if (!iommu) {
		ret = -ENOMEM;
		goto out;
	}

	spin_lock_init(&iommu->dt_lock);

	iommu->node = hose->node;
	iommu->index = hose->index;

	node = node_online(iommu->node) ? iommu->node : NUMA_NO_NODE;
	page = alloc_pages_node(node, __GFP_ZERO, get_order(PAGE_SIZE));
	if (!page) {
		ret = -ENOMEM;
		goto free_iommu;
	}

	iommu->iommu_dtbr = page_address(page);
	base = __pa(iommu->iommu_dtbr) & PAGE_MASK;
	iommu->reg_base_addr = hose->piu_ior0_base;
	writeq(base, iommu->reg_base_addr + DTBASEADDR);

	hose->pci_iommu = iommu;
	iommu->enabled = true;

	return iommu;

free_iommu:
	kfree(iommu);
out:
	return ERR_PTR(ret);
}

unsigned long fetch_dte(struct sunway_iommu *iommu, unsigned long devid,
			enum exceptype type)
{
	unsigned long *dte_l1, *dte_l2;
	unsigned long dte_l1_val, dte_l2_val;

	if (!iommu)
		return 0;
	dte_l1 = iommu->iommu_dtbr + (devid >> 8);
	if (type == DTE_LEVEL1)
		return (unsigned long)dte_l1;

	dte_l1_val = *dte_l1;
	if (type == DTE_LEVEL1_VAL)
		return dte_l1_val;

	dte_l1_val &= (~(SW64_IOMMU_ENTRY_VALID)) & (PAGE_MASK);
	dte_l1_val |= PAGE_OFFSET;
	dte_l2 = (unsigned long *)(dte_l1_val + ((devid & 0xff) << 3));
	if (type == DTE_LEVEL2)
		return (unsigned long)dte_l2;

	dte_l2_val = *dte_l2;
	if (type == DTE_LEVEL2_VAL)
		return dte_l2_val;

	return dte_l2_val;
}

unsigned long fetch_pte(struct sunway_iommu_domain *sdomain, dma_addr_t iova,
			enum exceptype type)
{
	unsigned long iova_pfn;
	unsigned long pte_l1_val, pte_l2_val, pte_l3_val;
	unsigned long *pte_l1, *pte_l2, *pte_l3;
	unsigned long pte_root;
	unsigned long offset;

	if (!sdomain)
		return -EINVAL;

	pte_root = __pa(sdomain->pt_root) & PAGE_MASK;
	iova_pfn = iova >> PAGE_SHIFT;
	pte_root = ((pte_root) & (~(SW64_IOMMU_ENTRY_VALID)) & (PAGE_MASK));
	pte_root |= PAGE_OFFSET;
	offset = ((iova_pfn >> 20) & SW64_IOMMU_LEVEL1_OFFSET) << 3;
	pte_l1 = (unsigned long *)(pte_root + offset);
	if (type == PTE_LEVEL1)
		return (unsigned long)pte_l1;

	pte_l1_val = *pte_l1;
	if (type == PTE_LEVEL1_VAL)
		return pte_l1_val;

	pte_l1_val &= (~(SW64_IOMMU_ENTRY_VALID)) & (PAGE_MASK);
	pte_l1_val |= PAGE_OFFSET;
	offset = ((iova_pfn >> 10) & SW64_IOMMU_LEVEL2_OFFSET) << 3;
	pte_l2 = (unsigned long *)(pte_l1_val + offset);

	if (type == PTE_LEVEL2)
		return (unsigned long)pte_l2;

	pte_l2_val = *pte_l2;
	if (type == PTE_LEVEL2_VAL)
		return pte_l2_val;

	pte_l2_val &= (~(SW64_IOMMU_ENTRY_VALID)) & (PAGE_MASK);
	pte_l2_val |= PAGE_OFFSET;
	offset = (iova_pfn & SW64_IOMMU_LEVEL3_OFFSET) << 3;
	pte_l3 = (unsigned long *)(pte_l2_val + offset);
	if (type == PTE_LEVEL3)
		return (unsigned long)pte_l3;

	pte_l3_val = *pte_l3;
	if (type == PTE_LEVEL3_VAL)
		return pte_l3_val;

	return pte_l3_val;
}

/* IOMMU Interrupt handle */
irqreturn_t iommu_interrupt(int irq, void *dev)
{
	struct pci_controller *hose = (struct pci_controller *)dev;
	struct sunway_iommu_domain *sdomain;
	struct sunway_iommu *iommu;
	struct sunway_iommu_dev *sdev;
	unsigned long iommu_status;
	unsigned long type;
	unsigned long devid, dva;

	iommu = hose->pci_iommu;
	iommu_status = readq(iommu->reg_base_addr + IOMMUEXCPT_STATUS);
	if (!(iommu_status >> 63))
		return IRQ_NONE;

	type = (iommu_status >> 58) & 0xf;
	devid = (iommu_status >> 36) & 0xffff;
	dva = ((iommu_status & 0xffffffff) >> 3) << 13;
	pr_info("%s, iommu_status = %#lx, devid %#lx, dva %#lx, ",
			__func__, iommu_status, devid, dva);

	sdev = search_dev_data(devid);
	if (sdev == NULL) {
		pr_info("no such dev!!!\n");

		iommu_status &= ~(1UL << 62);
		writeq(iommu_status, iommu->reg_base_addr + IOMMUEXCPT_STATUS);

		return IRQ_HANDLED;
	}

		sdomain = sdev->domain;
	switch (type) {
	case DTE_LEVEL1:
		pr_info("invalid level1 dte, addr:%#lx, val:%#lx\n",
			fetch_dte(iommu, devid, DTE_LEVEL1),
			fetch_dte(iommu, devid, DTE_LEVEL1_VAL));
		break;
	case DTE_LEVEL2:
		pr_info("invalid level2 dte, addr:%#lx, val:%#lx\n",
			fetch_dte(iommu, devid, DTE_LEVEL2),
			fetch_dte(iommu, devid, DTE_LEVEL2_VAL));
		break;
	case PTE_LEVEL1:
		pr_info("invalid level1 pte, addr: %#lx, val:%#lx\n",
			fetch_pte(sdomain, dva, PTE_LEVEL1),
			fetch_pte(sdomain, dva, PTE_LEVEL1_VAL));

		iommu_status &= ~(1UL << 62);
		writeq(iommu_status, iommu->reg_base_addr + IOMMUEXCPT_STATUS);
		break;
	case PTE_LEVEL2:
		pr_info("invalid level2 pte, addr: %#lx, val: %#lx\n",
			fetch_pte(sdomain, dva, PTE_LEVEL2),
			fetch_pte(sdomain, dva, PTE_LEVEL2_VAL));

		iommu_status &= ~(1UL << 62);
		writeq(iommu_status, iommu->reg_base_addr + IOMMUEXCPT_STATUS);
		break;

	case PTE_LEVEL3:
		pr_info("invalid level3 pte, addr: %#lx, val: %#lx\n",
			fetch_pte(sdomain, dva, PTE_LEVEL3),
			fetch_pte(sdomain, dva, PTE_LEVEL3_VAL));

		iommu_status &= ~(1UL << 62);
		writeq(iommu_status, iommu->reg_base_addr + IOMMUEXCPT_STATUS);
		break;
	default:
		pr_info("iommu exception type %ld\n", type);
		break;
	}

	return IRQ_HANDLED;
}

struct irqaction iommu_irqaction = {
	.handler = iommu_interrupt,
	.flags = IRQF_SHARED | IRQF_NO_THREAD,
	.name = "sunway_iommu",
};

void sunway_enable_iommu_func(struct pci_controller *hose)
{
	struct sunway_iommu *iommu;
	unsigned int iommu_irq, err;
	unsigned long iommu_conf, iommu_ctrl;

	iommu_irq = hose->int_irq;
	pr_debug("%s node %ld rc %ld iommu_irq %d\n",
			__func__, hose->node, hose->index, iommu_irq);
	err = request_irq(iommu_irq, iommu_interrupt,
			IRQF_SHARED, "sunway_iommu", hose);
	if (err < 0)
		pr_info("sw iommu request irq failed!\n");

	iommu = hose->pci_iommu;
	iommu_ctrl = (1UL << 63) | (0x100UL << 10);
	writeq(iommu_ctrl, iommu->reg_base_addr + IOMMUEXCPT_CTRL);
	iommu_conf = readq(iommu->reg_base_addr + PIUCONFIG0);
	iommu_conf = iommu_conf | (0x3 << 7);
	writeq(iommu_conf, iommu->reg_base_addr + PIUCONFIG0);
	writeq(0xf, iommu->reg_base_addr + TIMEOUT_CONFIG);
	iommu_conf = readq(iommu->reg_base_addr + PIUCONFIG0);
	pr_debug("SW arch configure node %ld hose-%ld iommu_conf = %#lx\n",
			hose->node, hose->index, iommu_conf);
}

/* iommu cpu syscore ops */
static int iommu_cpu_suspend(void)
{
	return 0;
}

static void iommu_cpu_resume(void)
{

}

struct syscore_ops iommu_cpu_syscore_ops = {
	.suspend = iommu_cpu_suspend,
	.resume = iommu_cpu_resume,
};

static struct iommu_domain *sunway_iommu_domain_alloc(unsigned int type);

/* Init functions */
static int do_detect(void)
{
	acpi_status status = AE_OK;

	status = acpi_get_table(ACPI_SIG_DMAR, 0, &dmar_tbl);

	if (ACPI_SUCCESS(status) && !dmar_tbl) {
		pr_warn("No DMAR found!\n");
		status = AE_NOT_FOUND;
	}

	return ACPI_SUCCESS(status) ? 0 : -ENOENT;
}

static struct pci_controller *find_hose_by_rcid(int node, int index)
{
	struct pci_controller *hose;

	for (hose = hose_head; hose; hose = hose->next)
		if (hose->node == node && hose->index == index)
			return hose;

	return NULL;
}

static int parse_one_drhd_unit(struct acpi_sw_dmar_header *header)
{
	struct acpi_dmar_sw_hardware_unit *drhd;
	struct sunway_iommu *iommu;
	struct pci_controller *hose;
	struct page *page;
	unsigned long base;
	int cmdline_enabled;
	int rc_mask, ret, node;
	int rc_node, rc_index;

	drhd = (struct acpi_dmar_sw_hardware_unit *)header;
	if (!drhd->enable)
		return 0;

	rc_node = (drhd->index >> 8) & 0xff;
	rc_index = drhd->index & 0xff;

	hose = find_hose_by_rcid(rc_node, rc_index);
	if (!hose)
		return 0;

	iommu = kzalloc(sizeof(struct sunway_iommu), GFP_KERNEL);
	if (!iommu)
		return -ENOMEM;

	iommu->node = rc_node;
	iommu->index = rc_index;
	iommu->reg_base_addr = ioremap(drhd->address, drhd->size);

	rc_mask = MAX_NR_IOMMU_PER_NODE * iommu->node + iommu->index;
	cmdline_enabled = test_bit(rc_mask, iommu_bitmap);
	if (!cmdline_enabled) {
		iommu->enabled = false;
		ret = 0;
		goto free_iommu;
	}

	node = node_online(iommu->node) ? iommu->node : NUMA_NO_NODE;
	page = alloc_pages_node(node, __GFP_ZERO, get_order(PAGE_SIZE));
	if (!page) {
		ret = -ENOMEM;
		goto free_iommu;
	}

	iommu->iommu_dtbr = page_address(page);
	base = __pa(iommu->iommu_dtbr) & PAGE_MASK;
	writeq(base, iommu->reg_base_addr + DTBASEADDR);

	list_add(&iommu->list, &iommu_list);
	iommu->enabled = true;

	hose->pci_iommu = iommu;

	pr_info("iommu: node: %ld index: %ld IOMMU enabled!\n",
			iommu->node, iommu->index);
	return 0;

free_iommu:
	kfree(iommu);
	return ret;
}

static int parse_drhd_units(struct acpi_table_sw_dmar *dmar)
{
	struct acpi_sw_dmar_header *iter, *start, *next, *end;
	size_t len = dmar->header.length - sizeof(*dmar);
	int ret, count = 0;

	/* Skip DMAR table, point to first DRHD table. */
	start = (struct acpi_sw_dmar_header *)(dmar + 1);
	end = ((void *)start) + len;

	for (iter = start; iter < end; iter = next) {
		next = (void *)iter + iter->length;
		if (iter->length == 0) {
			pr_warn(FW_BUG "Invalid 0-length structure\n");
			break;
		} else if (next > end) {
			pr_warn(FW_BUG "Record passes table end\n");
			return -EINVAL;
		}

		if (iter->type >= ACPI_SW_DMAR_TYPE_RESERVED) {
			pr_info("Unknown DMAR structure type %d\n",
					iter->type);
		} else if (iter->type == 0) {
			ret = parse_one_drhd_unit(iter);
			if (ret)
				return ret;
		}
		count++;
	}

	return 0;
}

static int sunway_iommu_acpi_early_init(void)
{
	int ret;

	struct acpi_table_sw_dmar *dmar;

	ret = do_detect();
	if (ret)
		return ret;

	dmar = (struct acpi_table_sw_dmar *)dmar_tbl;
	if (!dmar)
		return -ENODEV;

	if (dmar->width < 42) {
		pr_warn("Invalid DMAR haw\n");
		return -EINVAL;
	}
	pr_info("Host address width: %d\n", dmar->width);

	ret = parse_drhd_units(dmar);

	return ret;
}

static int sunway_iommu_acpi_init(void)
{
	struct sunway_iommu *iommu;
	struct pci_controller *hose;
	int iommu_index = 0;
	int ret;

	ret = sunway_iommu_acpi_early_init();
	if (ret)
		return ret;

	for_each_iommu(iommu) {
		hose = find_hose_by_rcid(iommu->node, iommu->index);
		if (!hose)
			continue;

		if (!iommu->enabled || hose->iommu_enable)
			continue;

		iommu_device_sysfs_add(&iommu->iommu, NULL, NULL, "%d",
				iommu_index);
		iommu_device_register(&iommu->iommu, &sunway_iommu_ops, NULL);
		iommu_index++;
		sunway_enable_iommu_func(hose);
		hose->iommu_enable = true;
		piu_flush_all(iommu);
	}

	ret = iova_cache_get();
	if (ret)
		return ret;

	register_syscore_ops(&iommu_cpu_syscore_ops);

	return 0;
}

static int sunway_iommu_legacy_init(void)
{
	struct pci_controller *hose;
	struct sunway_iommu *iommu;
	unsigned long rc_mask;
	int iommu_index = 0;
	int ret;

	/* Do the loop */
	for (hose = hose_head; hose; hose = hose->next) {
		rc_mask = MAX_NR_IOMMU_PER_NODE * hose->node + hose->index;
		if (!test_bit(rc_mask, iommu_bitmap)) {
			hose->iommu_enable = false;
			continue;
		}

		if (hose->iommu_enable)
			continue;

		iommu = sunway_iommu_early_init(hose);
		iommu_device_sysfs_add(&iommu->iommu, NULL, NULL, "%d",
				       iommu_index);
		iommu_device_register(&iommu->iommu, &sunway_iommu_ops, NULL);
		iommu_index++;
		sunway_enable_iommu_func(hose);
		hose->iommu_enable = true;
		piu_flush_all(iommu);
	}

	ret = iova_cache_get();
	if (ret)
		return ret;

	register_syscore_ops(&iommu_cpu_syscore_ops);

	return 0;
}

static int sunway_iommu_init(void)
{
	int ret;

	sunway_iommu_domain_bitmap =
	       (void *)__get_free_pages(GFP_KERNEL | __GFP_ZERO,
				get_order(MAX_DOMAIN_NUM / 8));
	if (!sunway_iommu_domain_bitmap)
		return 0;
	__set_bit(0, sunway_iommu_domain_bitmap);

	if (!acpi_disabled)
		ret = sunway_iommu_acpi_init();
	else
		ret = sunway_iommu_legacy_init();

	return ret;
}
subsys_initcall_sync(sunway_iommu_init);

/*******************************************************************************
 *
 * DMA OPS Functions
 *
 ******************************************************************************/

struct sunway_iommu *get_first_iommu_from_domain(struct sunway_iommu_domain *sdomain)
{
	struct sunway_iommu *iommu;
	struct sunway_iommu_dev *entry;

	entry = list_first_entry(&sdomain->dev_list, struct sunway_iommu_dev, list);
	iommu = entry->iommu;

	return iommu;
}

static unsigned long
sunway_iommu_unmap_page(struct sunway_iommu_domain *sunway_domain,
			unsigned long iova, unsigned long page_size)
{
	unsigned long offset, iova_pfn;
	unsigned long *pte_base, *pte;
	unsigned long grn;
	int level, current_level;
	int tmp = 1;

	pr_debug("%s iova %#lx, page_size %#lx\n", __func__, iova, page_size);
	BUG_ON(!is_power_of_2(page_size));

	switch (page_size) {
	case (1UL << 33):
		level = 1;
		grn = PTE_GRN_8G;
		break;
	case (1UL << 29):
		level = 2;
		grn = PTE_GRN_512M;
		break;
	case (1UL << 23):
		level = 2;
		grn = PTE_GRN_8M;
		break;
	default:
		level = 3;
		break;
	}

	pte_base = sunway_domain->pt_root;
	iova_pfn = iova >> PAGE_SHIFT;
	offset = (iova_pfn >> 20) & 0x1ff;
	current_level = 1;
	while (current_level <= level) {
		pte = &pte_base[offset];
		if (current_level == level) {
			if (grn == PTE_GRN_512M) {
				int i;

				for (i = 0; i < 64; i++) {
					*(pte + i) = 0;
					flush_pcache_by_addr(sunway_domain, (unsigned long)pte);
				}

			} else {
				*pte = 0;
				flush_pcache_by_addr(sunway_domain, (unsigned long)pte);
			}
			flush_ptlb_by_addr(sunway_domain, (iova >> PAGE_SHIFT));
			break;
		}

		pte_base = (unsigned long *)((*pte & (~PTE_FLAGS_MASK)) | PAGE_OFFSET);
		offset = (iova_pfn >> (tmp--) * 10) & 0x3ff;
		current_level++;
	}

	return page_size;
}

int sunway_iommu_map_page(struct sunway_iommu_domain *sunway_domain,
			  unsigned long bus_addr, unsigned long paddr,
			  size_t page_size, int iommu_prot)
{
	struct page *page;
	struct sunway_iommu *iommu;
	unsigned long iova_pfn, pte_val;
	unsigned long *pte_base, *pte;
	unsigned long offset, grn = 0;
	int level = 0, current_level;
	int tmp = 1;

	iommu = get_first_iommu_from_domain(sunway_domain);
	if (!iommu)
		return -1;
	iova_pfn = bus_addr >> PAGE_SHIFT;
	pte_base = sunway_domain->pt_root;

	switch (page_size) {
	case (1UL << 33):
		level = 1;
		grn = PTE_GRN_8G;
		break;
	case (1UL << 29):
		level = 2;
		grn = PTE_GRN_512M;
		break;
	case (1UL << 23):
		grn = PTE_GRN_8M;
		level = 2;
		break;
	default:
		level = 3;
		break;
	}

	offset = (iova_pfn >> 20) & 0x1ff;
	current_level = 1;
	while (current_level <= level) {
		pte = &pte_base[offset];

		if (!(*pte) || (current_level == level)) {
			pte_val = PTE_VALID | grn;
			if (current_level == level) {
				*(volatile u64 *)(pte) = 0;
				pte_val |= ((paddr & PAGE_MASK) | LAST_STAGE);
			} else {
				page = alloc_pages_node(iommu->node, GFP_ATOMIC | __GFP_ZERO, 0);
				if (!page) {
					pr_err("Allocating level%d page table pages failed.\n", (level + 1));
					return -ENOMEM;
				}

				pte_val |= (page_to_phys(page) & PAGE_MASK);
			}

			pte_val |= PTE_READE;
			if (iommu_prot & IOMMU_WRITE)
				pte_val |= PTE_WRITEE;

			if ((grn == PTE_GRN_512M) && (current_level == 2)) {
				int i;

				for (i = 0; i < 64; i++) {
					cmpxchg64((volatile u64 *)(pte + i), 0UL, pte_val);
					flush_pcache_by_addr(sunway_domain, (unsigned long)(pte + i));
				}
			} else {
				if (cmpxchg64((volatile u64 *)pte, 0UL, pte_val))
					free_page((unsigned long)page_address(page));
				else
					flush_pcache_by_addr(sunway_domain, (unsigned long)pte);
			}
		}

		pte_base = (unsigned long *)__va((*pte) & (~PTE_FLAGS_MASK));
		offset = (iova_pfn >> (tmp--) * 10) & 0x3ff;
		current_level++;
	}

	return 0;
}

/**********************************************************************
 *
 * IOMMU OPS Functions
 *
 **********************************************************************/

static struct iommu_domain *sunway_iommu_domain_alloc(unsigned int type)
{
	struct sunway_iommu_domain *sdomain;
	struct dma_domain *dma_dom;

	switch (type) {
	case IOMMU_DOMAIN_UNMANAGED:
		sdomain = sunway_domain_alloc();
		if (!sdomain) {
			pr_err("Allocating sunway_domain failed!\n");
			return NULL;
		}

		sdomain->domain.geometry.aperture_start = 0UL;
		sdomain->domain.geometry.aperture_end	= ~0ULL;
		sdomain->domain.geometry.force_aperture	= true;
		sdomain->type = IOMMU_DOMAIN_UNMANAGED;
		break;

	case IOMMU_DOMAIN_DMA:
		dma_dom = dma_domain_alloc();
		if (!dma_dom) {
			pr_err("Failed to alloc dma domain!\n");
			return NULL;
		}

		sdomain = &dma_dom->sdomain;
		break;

	case IOMMU_DOMAIN_IDENTITY:
		sdomain = sunway_domain_alloc();
		if (!sdomain)
			return NULL;

		sdomain->type = IOMMU_DOMAIN_IDENTITY;
		break;

	default:
		return NULL;
	}

	return &sdomain->domain;
}

static void clean_domain(struct sunway_iommu_domain *sdomain)
{
	struct sunway_iommu_dev *entry;
	unsigned long flags;

	spin_lock_irqsave(&sunway_iommu_device_table_lock, flags);

	while (!list_empty(&sdomain->dev_list)) {
		entry = list_first_entry(&sdomain->dev_list,
					 struct sunway_iommu_dev, list);

		BUG_ON(!entry->domain);
		__detach_device(entry);
	}

	spin_unlock_irqrestore(&sunway_iommu_device_table_lock, flags);
}

static void sunway_iommu_domain_free(struct iommu_domain *dom)
{
	struct sunway_iommu_domain *sdomain;
	struct dma_domain *dma_dom;

	sdomain = to_sunway_domain(dom);

	if (sdomain->dev_cnt > 0)
		clean_domain(sdomain);

	BUG_ON(sdomain->dev_cnt != 0);

	if (!dom)
		return;

	switch (dom->type) {
	case IOMMU_DOMAIN_DMA:
		dma_dom = to_dma_domain(sdomain);
		dma_domain_free(dma_dom);
		break;

	default:
		free_pagetable(sdomain);
		sunway_domain_free(sdomain);
		break;
	}

}

static int sunway_iommu_attach_device(struct iommu_domain *dom, struct device *dev)
{
	struct sunway_iommu_domain *sdomain = to_sunway_domain(dom);
	struct sunway_iommu_dev *sdev;
	struct pci_dev *pdev;
	struct pci_controller *hose;
	int ret;

	pdev = to_pci_dev(dev);
	if (!pdev)
		return -EINVAL;

	hose = pci_bus_to_pci_controller(pdev->bus);
	if (!hose)
		return -EINVAL;

	if (!hose->iommu_enable)
		return -EINVAL;

	sdev = dev_iommu_priv_get(dev);
	if (!sdev)
		return -EINVAL;

	if (sdev->domain)
		detach_device(dev);

	ret = attach_device(dev, sdomain);

	return ret;
}

static phys_addr_t
sunway_iommu_iova_to_phys(struct iommu_domain *dom, dma_addr_t iova)
{
	struct sunway_iommu_domain *sdomain = to_sunway_domain(dom);
	unsigned long paddr, grn;
	unsigned long is_last;

	if (iova >= SW64_BAR_ADDRESS)
		return iova;

	paddr = fetch_pte(sdomain, iova, PTE_LEVEL1_VAL);
	if ((paddr & SW64_IOMMU_ENTRY_VALID) == 0)
		return 0;

	is_last = paddr & SW64_PTE_LAST_MASK;
	grn = paddr & SW64_PTE_GRN_MASK;
	if (is_last) {
		if (grn == PTE_GRN_8G) {
			paddr &= ~PTE_FLAGS_MASK;
			paddr += iova & PAGE_8G_OFFSET_MASK;
			return paddr;
		}

		return 0;
	}

	paddr = fetch_pte(sdomain, iova, PTE_LEVEL2_VAL);
	if ((paddr & SW64_IOMMU_ENTRY_VALID) == 0)
		return 0;

	is_last = paddr & SW64_PTE_LAST_MASK;
	grn = paddr & SW64_PTE_GRN_MASK;
	if (is_last) {
		if (grn == PTE_GRN_512M) {
			paddr &= ~PTE_FLAGS_MASK;
			paddr += iova & PAGE_512M_OFFSET_MASK;
			return paddr;
		}

		if (grn == PTE_GRN_8M) {
			paddr &= ~PTE_FLAGS_MASK;
			paddr += iova & PAGE_8M_OFFSET_MASK;
			return paddr;
		}

		return 0;
	}

	paddr = fetch_pte(sdomain, iova, PTE_LEVEL3_VAL);
	if ((paddr & SW64_IOMMU_ENTRY_VALID) == 0)
		return 0;

	grn = paddr & SW64_PTE_GRN_MASK;
	if (grn != 0)
		return 0;

	paddr &= ~PTE_FLAGS_MASK;
	paddr += iova & ~PAGE_MASK;
	return paddr;
}

static int
sunway_iommu_map_pages(struct iommu_domain *dom, unsigned long iova,
		phys_addr_t paddr, size_t page_size, size_t pgcount,
		int iommu_prot, gfp_t gfp, size_t *mapped)
{
	struct sunway_iommu_domain *sdomain = to_sunway_domain(dom);
	size_t size = pgcount << PAGE_SHIFT;
	int ret;

	/*
	 * As VFIO cannot distinguish between normal DMA request
	 * and pci device BAR, check should be introduced manually
	 * to avoid VFIO trying to map pci config space.
	 */
	if (iova >= SW64_BAR_ADDRESS)
		return 0;

	if (iova >= (1UL << IOVA_MAX_ADDRESS_WIDTH)) {
		pr_err("IOMMU cannot map provided address: %lx\n", iova);
		return -EFAULT;
	}

	while (pgcount--) {
		ret = sunway_iommu_map_page(sdomain, iova, paddr, page_size, iommu_prot);
		if (ret) {
			pr_info("Failed to map page from IOVA %lx.\n", iova);
			return ret;
		}
		iova += page_size;
		paddr += page_size;
	}

	if (!ret && mapped)
		*mapped = size;

	return ret;
}

static size_t
sunway_iommu_unmap_pages(struct iommu_domain *dom, unsigned long iova,
		size_t page_size, size_t pgcount,
		struct iommu_iotlb_gather *gather)
{
	struct sunway_iommu_domain *sdomain = to_sunway_domain(dom);
	size_t unmap_size;
	size_t total_unmap = 0;

	if (iova >= SW64_BAR_ADDRESS)
		return page_size;

	if (iova >= (1UL << IOVA_MAX_ADDRESS_WIDTH)) {
		pr_err("IOMMU cannot map provided address: %lx\n", iova);
		return -EFAULT;
	}

	while (pgcount--) {
		unmap_size = sunway_iommu_unmap_page(sdomain, iova, page_size);
		iova += page_size;
		total_unmap += page_size;
	}

	return total_unmap;
}

static struct iommu_group *sunway_iommu_device_group(struct device *dev)
{
	return generic_device_group(dev);
}

static void iommu_uninit_device(struct device *dev)
{
	struct sunway_iommu_dev *sdev;

	sdev = dev_iommu_priv_get(dev);
	if (!sdev)
		return;

	if (sdev->domain)
		detach_device(dev);

	dev_iommu_priv_set(dev, NULL);
}

static void sunway_iommu_release_device(struct device *dev)
{
	struct pci_dev *pdev;
	struct pci_controller *hose;

	pdev = to_pci_dev(dev);
	if (!pdev)
		return;

	hose = pci_bus_to_pci_controller(pdev->bus);
	if (!hose->iommu_enable)
		return;

	iommu_uninit_device(dev);
}

static int iommu_init_device(struct device *dev)
{
	struct sunway_iommu_dev *sdev;
	struct sunway_iommu *iommu;
	struct pci_dev *pdev;
	struct pci_controller *hose;

	if (dev_iommu_priv_get(dev))
		return 0;

	sdev = kzalloc(sizeof(struct sunway_iommu_dev), GFP_KERNEL);
	if (!sdev)
		return -ENOMEM;

	pdev = to_pci_dev(dev);
	sdev->devid = PCI_DEVID(pdev->bus->number, pdev->devfn);
	sdev->alias = get_alias(pdev);

	hose = pci_bus_to_pci_controller(pdev->bus);
	iommu = hose->pci_iommu;
	llist_add(&sdev->dev_data_list, &dev_data_list);
	sdev->pdev = pdev;
	sdev->iommu = iommu;

	dev_iommu_priv_set(dev, sdev);

	return 0;
}

static struct iommu_device *sunway_iommu_probe_device(struct device *dev)
{
	struct pci_dev *pdev;
	struct pci_controller *hose;
	struct sunway_iommu *iommu;
	int ret;

	pdev = to_pci_dev(dev);
	if (!pdev)
		return ERR_PTR(-ENODEV);

	if (pdev->hdr_type == PCI_HEADER_TYPE_BRIDGE)
		return ERR_PTR(-ENODEV);

	if (pci_pcie_type(pdev) == PCI_EXP_TYPE_ROOT_PORT)
		return ERR_PTR(-ENODEV);

	hose = pci_bus_to_pci_controller(pdev->bus);
	if (!hose)
		return ERR_PTR(-ENODEV);

	if (!hose->iommu_enable)
		return ERR_PTR(-ENODEV);

	if (dev_iommu_priv_get(dev)) {
		iommu = hose->pci_iommu;
		return &iommu->iommu;
	}

	ret = iommu_init_device(dev);
	if (ret)
		return ERR_PTR(ret);

	iommu = hose->pci_iommu;

	return &iommu->iommu;
}

static int sunway_iommu_def_domain_type(struct device *dev)
{
	if (dev_is_pci(dev))
		if (iommu_identity_mapping)
			return IOMMU_DOMAIN_IDENTITY;

	return 0;
}

static bool sunway_iommu_capable(struct device *dev, enum iommu_cap cap)
{
	switch (cap) {
	case IOMMU_CAP_CACHE_COHERENCY:
		return true;
	case IOMMU_CAP_NOEXEC:
		return true;
	default:
		return false;
	}
}

static void sunway_iommu_probe_finalize(struct device *dev)
{
	struct iommu_domain *domain;

	domain = iommu_get_domain_for_dev(dev);
	if (domain->type == IOMMU_DOMAIN_DMA) {
		if (min(dev->coherent_dma_mask, *dev->dma_mask) == DMA_BIT_MASK(32))
			iommu_setup_dma_ops(dev, SW64_DMA_START, SW64_32BIT_DMA_LIMIT);
		else
			iommu_setup_dma_ops(dev, SW64_DMA_START, SW64_64BIT_DMA_LIMIT);
	} else
		set_dma_ops(dev, get_arch_dma_ops());
}

const struct iommu_ops sunway_iommu_ops = {
	.capable = sunway_iommu_capable,
	.domain_alloc = sunway_iommu_domain_alloc,
	.probe_device = sunway_iommu_probe_device,
	.probe_finalize = sunway_iommu_probe_finalize,
	.release_device = sunway_iommu_release_device,
	.device_group = sunway_iommu_device_group,
	.pgsize_bitmap	= SZ_8K | SZ_8M | SZ_512M | SZ_8G,
	.def_domain_type = sunway_iommu_def_domain_type,
	.default_domain_ops = &(const struct iommu_domain_ops) {
		.attach_dev = sunway_iommu_attach_device,
		.map_pages = sunway_iommu_map_pages,
		.unmap_pages = sunway_iommu_unmap_pages,
		.iova_to_phys = sunway_iommu_iova_to_phys,
		.free = sunway_iommu_domain_free,
	}
};

/* Handle boot parameters */
static int __init sunway_iommu_setup(char *str)
{
	unsigned long rc_val;
	int ret;

	if (!str)
		return -EINVAL;

	bitmap_zero(iommu_bitmap, 64);

	if (!strncmp(str, "on", 2)) {
		bitmap_fill(iommu_bitmap, 64);
	} else if (!strncmp(str, "off", 3)) {
		bitmap_zero(iommu_bitmap, 64);
	} else {
		ret = kstrtoul(str, 16, &rc_val);
		if (!ret)
			return -EINVAL;

		bitmap_from_u64(iommu_bitmap, rc_val);
	}

	return 1;
}
__setup("sunway_iommu=", sunway_iommu_setup);

static int __init iommu_enable_setup(char *str)
{
	unsigned long rc_val;
	int ret;

	if (!str)
		return -EINVAL;

	pr_info("iommu_enable= deprecated; use sunway_iommu=on/off instead\n");
	bitmap_zero(iommu_bitmap, 64);

	ret = kstrtoul(str, 16, &rc_val);
	if (!ret)
		return -EINVAL;

	bitmap_from_u64(iommu_bitmap, rc_val);

	return 1;
}
__setup("iommu_enable=", iommu_enable_setup);

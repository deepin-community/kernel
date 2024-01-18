// SPDX-License-Identifier: GPL-2.0
/*
 * iommu.c: Generic sw64 IOMMU support
 *
 * This is designed and tested for 3231. If there are no changes in hardware
 * in later chips, then it should work just as well.
 *
 */

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
#include <linux/iommu.h>
#include <linux/iommu-helper.h>
#include <linux/iova.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/syscore_ops.h>
#include <linux/swiotlb.h>
#include <linux/cache.h>
#include <linux/module.h>
#include <asm/dma.h>
#include <linux/io.h>
#include <asm/sw64io.h>
#include <asm/hw_init.h>

#include "sunway_iommu.h"

#define MAX_DOMAIN_NUM 65536
#define IOVA_PFN(addr) ((addr) >> PAGE_SHIFT)
#define SW64_32BIT_DMA_LIMIT (0xe0000000 - 1)
#define SW64_64BIT_DMA_LIMIT ((1UL << 41) - 1)
#define SW64_BAR_ADDRESS (IO_BASE | PCI_BASE)

#define SW64_IOMMU_PGSIZES (((1ULL) << PAGE_SHIFT) \
			  | ((1ULL) << PAGE_8M_SHIFT) \
			  | ((1ULL) << PAGE_512M_SHIFT) \
			  | ((1ULL) << PAGE_8G_SHIFT))

#define IDENTMAP_ALL	((1U) << 0)
#define DMA_MASK64	((1U) << 1)

#define PTE_VALID		0x8000000000000000UL
#define LAST_STAGE		0x100UL
#define PTE_GRN_8M		0x10UL
#define PTE_GRN_512M		0x20UL
#define PTE_GRN_8G		0x30UL
#define PTE_WRITEE		0x2UL
#define PTE_READE		0x1UL
#define PTE_RWE			0x3UL
#define PTE_FLAGS_MASK		0x8000000000000133UL
#define PAGE_8G_OFFSET_MASK	((1UL << PAGE_8G_SHIFT) - 1)
#define PAGE_512M_OFFSET_MASK	((1UL << PAGE_512M_SHIFT) - 1)
#define PAGE_8M_OFFSET_MASK	((1UL << PAGE_8M_SHIFT) - 1)

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

u64 iommu_enable_cmd;			/* default IOMMU boot param: 0 */

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


/* flush helpers */
static void piu_flush_all(struct pci_controller *hose)
{
	write_piu_ior0(hose->node, hose->index, DTLB_FLUSHALL, 0);
	write_piu_ior0(hose->node, hose->index, PTLB_FLUSHALL, 0);
	write_piu_ior0(hose->node, hose->index, PCACHE_FLUSHALL, 0);
}

void flush_pcache_by_addr(struct sunway_iommu_domain *sdomain, unsigned long flush_addr)
{
	struct pci_controller *hose;
	struct sunway_iommu_dev *sdev;

	list_for_each_entry(sdev, &sdomain->dev_list, list) {
		hose = pci_bus_to_pci_controller(sdev->pdev->bus);

		flush_addr = __pa(flush_addr);
		/* Set memory bar here */
		mb();
		write_piu_ior0(hose->node, hose->index,
				PCACHE_FLUSHPADDR, flush_addr);
	}
}

void flush_ptlb_by_addr(struct sunway_iommu_domain *sdomain, unsigned long flush_addr)
{
	struct pci_controller *hose;
	struct sunway_iommu_dev *sdev;
	struct pci_dev *pdev;

	list_for_each_entry(sdev, &sdomain->dev_list, list) {
		pdev = sdev->pdev;
		hose = pci_bus_to_pci_controller(pdev->bus);

		flush_addr = (pdev->bus->number << 8)
				| pdev->devfn | (flush_addr << 16);
		write_piu_ior0(hose->node, hose->index,
				PTLB_FLUSHVADDR, flush_addr);
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
	init_iova_domain(&dma_dom->iovad, PAGE_SIZE, IOVA_PFN(SW64_DMA_START));
	reserve_iova(&dma_dom->iovad, (0xe0000000UL >> PAGE_SHIFT), (0x100000000UL >> PAGE_SHIFT));

	add_domain_to_list(&dma_dom->sdomain);

	return dma_dom;
}

static void device_flush_all(struct sunway_iommu_dev *sdata)
{
	struct pci_controller *hose = pci_bus_to_pci_controller(sdata->pdev->bus);

	if (hose == NULL)
		return;

	write_piu_ior0(hose->node, hose->index, DTLB_FLUSHDEV, sdata->devid);
	write_piu_ior0(hose->node, hose->index, PTLB_FLUSHDEV, sdata->devid);
	write_piu_ior0(hose->node, hose->index, PCACHE_FLUSHDEV, sdata->devid);
}

/* iommu_ops device attach/unattach helpers */
static void
set_dte_entry(struct sunway_iommu_dev *sdev, struct sunway_iommu_domain *sdomain)
{
	struct sunway_iommu *iommu;
	struct pci_dev *pdev;
	struct page *dt_page, *pt_page;
	unsigned long *dte_l1, *dte_l2;
	unsigned long dte_l1_val, dte_l2_base, dte_l2_val;

	pdev = sdev->pdev;
	if (pdev->hdr_type == PCI_HEADER_TYPE_BRIDGE)
		return;

	sdev->devid = PCI_DEVID(pdev->bus->number, pdev->devfn);
	iommu = sdev->iommu;
	dte_l1 = iommu->iommu_dtbr + (pdev->bus->number);
	dte_l1_val = *dte_l1;

	if (!dte_l1_val) {
		/* Alloc a new level-2 device table page */
		dt_page = alloc_pages_node(iommu->node, GFP_KERNEL | __GFP_ZERO,
				get_order(PAGE_SIZE));
		if (!dt_page) {
			pr_err("Allocating a new level-2 device table page failed.\n");
			return;
		}

		dte_l2_base = (unsigned long)page_address(dt_page);
		dte_l1_val = (__pa(dte_l2_base) & PAGE_MASK) | SW64_IOMMU_ENTRY_VALID;
		*dte_l1 = dte_l1_val;
	}

	if (!sdomain->pt_root) {
		pt_page = alloc_pages_node(iommu->node, GFP_KERNEL | __GFP_ZERO, 0);
		if (!pt_page) {
			pr_err("Allocating pt_root failed!\n");
			return;
		}

		sdomain->pt_root = page_address(pt_page);
	}

	dte_l2 = __va(dte_l1_val & ~(SW64_IOMMU_ENTRY_VALID) & PAGE_MASK) + (pdev->devfn << 3);
	dte_l2_val = (__pa(sdomain->pt_root) & PAGE_MASK) | SW64_IOMMU_ENTRY_VALID;
	if (sdomain->type == IOMMU_DOMAIN_IDENTITY) {
		dte_l2_val |= 0x1;
		sdev->passthrough = IDENTMAP_ALL;
	}
	*dte_l2 = dte_l2_val;
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

/* dma_ops helpers*/
static struct sunway_iommu_domain *get_sunway_domain(struct device *dev)
{
	struct sunway_iommu_domain *sdomain;
	struct iommu_domain *domain;
	struct pci_dev *pdev;
	struct sunway_iommu_dev *sdev;

	pdev = to_pci_dev(dev);
	if (!pdev)
		return ERR_PTR(-ENODEV);

	sdev = dev_iommu_priv_get(dev);
	sdomain = sdev->domain;
	if (sdomain == NULL) {
		domain = iommu_get_domain_for_dev(dev);
		sdomain = to_sunway_domain(domain);
		attach_device(dev, sdomain);
	}

	if (sdomain == NULL)
		return ERR_PTR(-EBUSY);

	return sdomain;
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

	hose->pci_iommu = kzalloc(sizeof(struct sunway_iommu), GFP_KERNEL);
	if (!hose->pci_iommu)
		return 0;

	iommu = hose->pci_iommu;
	spin_lock_init(&iommu->dt_lock);

	iommu->node = hose->node;
	if (!node_online(hose->node))
		iommu->node = -1;

	page = alloc_pages_node(iommu->node, __GFP_ZERO, get_order(PAGE_SIZE));
	if (!page) {
		pr_err("Allocating a new iommu_dtbr page failed.\n");
		kfree(hose->pci_iommu);
		return NULL;
	}
	iommu->iommu_dtbr = page_address(page);

	iommu->hose_pt = hose;
	iommu->index = hose->index;

	iommu->enabled = true;

	base = __pa(iommu->iommu_dtbr) & PAGE_MASK;
	write_piu_ior0(hose->node, hose->index, DTBASEADDR, base);

	return iommu;
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
	struct sunway_iommu_dev *sdev;
	unsigned long iommu_status;
	unsigned long type;
	unsigned long devid, dva;

	iommu_status = read_piu_ior0(hose->node, hose->index, IOMMUEXCPT_STATUS);
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
		write_piu_ior0(hose->node, hose->index,
				IOMMUEXCPT_STATUS, iommu_status);

		return IRQ_HANDLED;
	}

	sdomain = sdev->domain;
	switch (type) {
	case DTE_LEVEL1:
		pr_info("invalid level1 dte, addr:%#lx, val:%#lx\n",
			fetch_dte(hose->pci_iommu, devid, DTE_LEVEL1),
			fetch_dte(hose->pci_iommu, devid, DTE_LEVEL1_VAL));
		break;
	case DTE_LEVEL2:
		pr_info("invalid level2 dte, addr:%#lx, val:%#lx\n",
			fetch_dte(hose->pci_iommu, devid, DTE_LEVEL2),
			fetch_dte(hose->pci_iommu, devid, DTE_LEVEL2_VAL));
		break;
	case PTE_LEVEL1:
		pr_info("invalid level1 pte, addr: %#lx, val:%#lx\n",
			fetch_pte(sdomain, dva, PTE_LEVEL1),
			fetch_pte(sdomain, dva, PTE_LEVEL1_VAL));

		iommu_status &= ~(1UL << 62);
		write_piu_ior0(hose->node, hose->index,
				IOMMUEXCPT_STATUS, iommu_status);
		break;
	case PTE_LEVEL2:
		pr_info("invalid level2 pte, addr: %#lx, val: %#lx\n",
			fetch_pte(sdomain, dva, PTE_LEVEL2),
			fetch_pte(sdomain, dva, PTE_LEVEL2_VAL));

		iommu_status &= ~(1UL << 62);
		write_piu_ior0(hose->node, hose->index,
				IOMMUEXCPT_STATUS, iommu_status);
		break;

	case PTE_LEVEL3:
		pr_info("invalid level3 pte, addr: %#lx, val: %#lx\n",
			fetch_pte(sdomain, dva, PTE_LEVEL3),
			fetch_pte(sdomain, dva, PTE_LEVEL3_VAL));

		iommu_status &= ~(1UL << 62);
		write_piu_ior0(hose->node, hose->index,
				IOMMUEXCPT_STATUS, iommu_status);
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
	unsigned int iommu_irq, err;
	unsigned long iommu_conf, iommu_ctrl;

	iommu_irq = hose->int_irq;
	pr_debug("%s node %ld rc %ld iommu_irq %d\n",
			__func__, hose->node, hose->index, iommu_irq);
	err = request_irq(iommu_irq, iommu_interrupt,
			IRQF_SHARED, "sunway_iommu", hose);
	if (err < 0)
		pr_info("sw iommu request irq failed!\n");

	iommu_ctrl = (1UL << 63) | (0x100UL << 10);
	write_piu_ior0(hose->node, hose->index, IOMMUEXCPT_CTRL, iommu_ctrl);
	iommu_conf = read_piu_ior0(hose->node, hose->index, PIUCONFIG0);
	iommu_conf = iommu_conf | (0x3 << 7);
	write_piu_ior0(hose->node, hose->index, PIUCONFIG0, iommu_conf);
	write_piu_ior0(hose->node, hose->index, TIMEOUT_CONFIG, 0xf);
	iommu_conf = read_piu_ior0(hose->node, hose->index, PIUCONFIG0);
	pr_debug("SW arch configure node %ld hose-%ld iommu_conf = %#lx\n",
			hose->node, hose->index, iommu_conf);
}

static bool is_iommu_enable(struct pci_controller *hose)
{
	u64 rc_mask = 0x1;

	rc_mask <<= (8 * hose->node + hose->index);
	if (iommu_enable_cmd & rc_mask)
		return true;

	return false;
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

static int sunway_iommu_init(void)
{
	struct pci_controller *hose;
	struct sunway_iommu *iommu;
	int ret;
	int iommu_index = 0;

	sunway_iommu_domain_bitmap =
		(void *)__get_free_pages(GFP_KERNEL | __GFP_ZERO,
				get_order(MAX_DOMAIN_NUM / 8));
	if (sunway_iommu_domain_bitmap == NULL)
		return 0;
	__set_bit(0, sunway_iommu_domain_bitmap);

	/* Do the loop */
	for (hose = hose_head; hose; hose = hose->next) {
		if (!is_iommu_enable(hose)) {
			hose->iommu_enable = false;
			continue;
		}

		iommu = sunway_iommu_early_init(hose);
		if (!iommu) {
			pr_err("Allocating sunway_iommu failed\n");
			hose->iommu_enable = false;
			continue;
		}

		iommu_device_sysfs_add(&iommu->iommu, NULL, NULL, "%d",
				       iommu_index);
		iommu_device_set_ops(&iommu->iommu, &sunway_iommu_ops);
		iommu_device_register(&iommu->iommu);
		iommu_index++;
		sunway_enable_iommu_func(hose);
		hose->iommu_enable = true;
	}

	ret = iova_cache_get();
	if (ret)
		return ret;

	ret = bus_set_iommu(&pci_bus_type, &sunway_iommu_ops);
	if (ret)
		return ret;

	for (hose = hose_head; hose; hose = hose->next)
		if (hose->iommu_enable)
			piu_flush_all(hose);

	register_syscore_ops(&iommu_cpu_syscore_ops);

	return 1;
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
			  size_t page_size)
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
			pte_val = PTE_VALID | PTE_RWE | grn;
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

static unsigned long
sunway_alloc_iova(struct dma_domain *dma_dom, unsigned long pages, struct pci_dev *pdev)
{
	struct device *dev;
	unsigned long pfn = 0;

	pages = __roundup_pow_of_two(pages);
	dev = &(pdev->dev);
	if (min(dev->coherent_dma_mask, *dev->dma_mask) == DMA_BIT_MASK(32)) {
		pfn = alloc_iova_fast(&dma_dom->iovad, pages,
				IOVA_PFN(SW64_32BIT_DMA_LIMIT), true);
	} else {
		/* IOVA boundary should be 16M ~ 3.5G */
		pfn = alloc_iova_fast(&dma_dom->iovad, pages,
				IOVA_PFN(SW64_64BIT_DMA_LIMIT), true);
	}

	return (pfn << PAGE_SHIFT);
}

static void sunway_free_iova(struct dma_domain *dma_dom,
			 unsigned long address, unsigned long pages)
{
	pages = __roundup_pow_of_two(pages);
	address >>= PAGE_SHIFT;

	free_iova_fast(&dma_dom->iovad, address, pages);
}

static dma_addr_t
__sunway_map_single(struct dma_domain *dma_dom,
		struct pci_dev *pdev, phys_addr_t paddr, size_t size)
{
	dma_addr_t ret, address, start;
	unsigned long npages, i;

	npages = iommu_num_pages(paddr, size, PAGE_SIZE);

	address = sunway_alloc_iova(dma_dom, npages, pdev);
	if (!address)
		return 0;

	start = address;
	for (i = 0; i < npages; ++i) {
		ret = sunway_iommu_map_page(&dma_dom->sdomain, start,
					paddr, PAGE_SIZE);
		if (ret) {
			pr_info("error when map page.\n");
			goto out_unmap;
		}

		start += PAGE_SIZE;
		paddr += PAGE_SIZE;
	}

	address += paddr & ~PAGE_MASK;
	return address;

out_unmap:
	for (--i; i >= 0; --i) {
		start -= PAGE_SIZE;
		sunway_iommu_unmap_page(&dma_dom->sdomain, start, PAGE_SIZE);
	}

	sunway_free_iova(dma_dom, address, npages);
	return 0;
}

static dma_addr_t
pci_iommu_map_single(struct pci_dev *pdev,
		     struct dma_domain *dma_dom, void *cpu_addr, size_t size)
{
	struct pci_controller *hose = pci_bus_to_pci_controller(pdev->bus);
	unsigned long paddr;

	if (hose == NULL) {
		pr_err("%s: hose does not exist!\n", __func__);
		return 0;
	}

	paddr = __sunway_map_single(dma_dom, pdev, __pa(cpu_addr), size);

	pr_debug("pci_alloc_consistent: %zx -> [%px,%lx] from %ps\n",
			size, cpu_addr, paddr, __builtin_return_address(0));

	return paddr;
}

static void *sunway_alloc_coherent(struct device *dev,
				   size_t size,
				   dma_addr_t *dma_addr, gfp_t gfp,
				   unsigned long attrs)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct pci_controller *hose;
	struct sunway_iommu_domain *sdomain;
	struct dma_domain *dma_dom;
	struct sunway_iommu_dev *sdev;
	struct page *page;
	void *cpu_addr;

	if (!pdev)
		return NULL;

	hose = pci_bus_to_pci_controller(pdev->bus);
	if (!hose)
		return NULL;

	gfp &= ~GFP_DMA;

try_again:
	page = alloc_pages_node(dev_to_node(dev), gfp | __GFP_ZERO, get_order(size));
	if (!page) {
		pr_err("Allocating pages failed.\n");
		return NULL;
	}

	cpu_addr = page_address(page);
	if (!cpu_addr) {
		pr_info
			("pci_alloc_consistent: get_free_pages failed from %ps\n",
			 __builtin_return_address(0));

		return NULL;
	}

	*dma_addr = __pa(cpu_addr);
	if (!(hose->iommu_enable))
		return cpu_addr;

	sdev = dev_iommu_priv_get(dev);
	if (sdev->passthrough & DMA_MASK64)
		return cpu_addr;
	else if (sdev->passthrough) {
		if (min(dev->coherent_dma_mask, *dev->dma_mask) > DMA_BIT_MASK(32)) {
			sdev->passthrough |= DMA_MASK64;
			return cpu_addr;
		}

		__free_pages(page, get_order(size));
		set_dma_ops(dev, get_arch_dma_ops(dev->bus));
		return dev->dma_ops->alloc(dev, size, dma_addr, gfp, attrs);
	}

	sdomain = get_sunway_domain(dev);
	dma_dom = to_dma_domain(sdomain);

	*dma_addr = pci_iommu_map_single(pdev, dma_dom, cpu_addr, size);
	if (*dma_addr == 0) {
		free_pages((unsigned long)cpu_addr, get_order(size));
		if (gfp & GFP_DMA)
			return NULL;

		gfp |= GFP_DMA;
		goto try_again;
	}

	return cpu_addr;
}

static void
__sunway_unmap_single(struct dma_domain *dma_dom, dma_addr_t dma_addr, size_t size)
{
	dma_addr_t start;
	unsigned long npages;
	int i;

	npages = iommu_num_pages(dma_addr, size, PAGE_SIZE);
	dma_addr &= PAGE_MASK;
	start = dma_addr;

	for (i = 0; i < npages; i++) {
		sunway_iommu_unmap_page(&dma_dom->sdomain, start, PAGE_SIZE);
		start += PAGE_SIZE;
	}

	sunway_free_iova(dma_dom, dma_addr, npages);
	pr_debug("pci_free_consistent: %zx -> [%llx] from %ps\n",
			size, dma_addr, __builtin_return_address(0));

}

static void
sunway_free_coherent(struct device *dev, size_t size,
		 void *vaddr, dma_addr_t dma_addr, unsigned long attrs)
{
	struct sunway_iommu_domain *sdomain;
	struct dma_domain *dma_dom;
	struct pci_dev *pdev = to_pci_dev(dev);
	struct pci_controller *hose;
	struct sunway_iommu_dev *sdev;

	if (!pdev)
		goto out_unmap;

	hose = pci_bus_to_pci_controller(pdev->bus);
	if (!hose || !(hose->iommu_enable))
		goto out_unmap;

	sdev = dev_iommu_priv_get(dev);
	if (sdev->passthrough)
		goto out_unmap;

	sdomain = get_sunway_domain(dev);
	dma_dom = to_dma_domain(sdomain);
	__sunway_unmap_single(dma_dom, dma_addr, size);
	goto out_free;

out_unmap:
	pci_unmap_single(pdev, dma_addr, size, PCI_DMA_BIDIRECTIONAL);

out_free:
	pr_debug("sunway_free_consistent: [%llx,%zx] from %ps\n",
		dma_addr, size, __builtin_return_address(0));

	free_pages((unsigned long)vaddr, get_order(size));
}

static dma_addr_t
sunway_map_page(struct device *dev, struct page *page,
		unsigned long offset, size_t size,
		enum dma_data_direction dir, unsigned long attrs)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct sunway_iommu_domain *sdomain;
	struct dma_domain *dma_dom;
	struct pci_controller *hose;
	struct sunway_iommu_dev *sdev;
	phys_addr_t paddr = page_to_phys(page) + offset;

	if (!pdev)
		return 0;

	hose = pci_bus_to_pci_controller(pdev->bus);
	if (!hose || !(hose->iommu_enable))
		return paddr;

	sdev = dev_iommu_priv_get(dev);
	if (sdev->passthrough & DMA_MASK64)
		return paddr;
	else if (sdev->passthrough) {
		if (min(dev->coherent_dma_mask, *dev->dma_mask) > DMA_BIT_MASK(32)) {
			sdev->passthrough |= DMA_MASK64;
			return paddr;
		}

		set_dma_ops(dev, get_arch_dma_ops(dev->bus));
		return dev->dma_ops->map_page(dev, page, offset, size, dir, attrs);
	}

	sdomain = get_sunway_domain(dev);
	dma_dom = to_dma_domain(sdomain);

	return pci_iommu_map_single(pdev, dma_dom,
		(char *)page_address(page) + offset, size);
}

static void
sunway_unmap_page(struct device *dev, dma_addr_t dma_addr,
		  size_t size, enum dma_data_direction dir, unsigned long attrs)
{
	struct sunway_iommu_domain *sdomain;
	struct dma_domain *dma_dom;
	struct pci_dev *pdev;
	struct pci_controller *hose;
	struct sunway_iommu_dev *sdev;

	pdev = to_pci_dev(dev);
	if (!pdev)
		return;

	hose = pci_bus_to_pci_controller(pdev->bus);
	if (hose == NULL)
		return;

	if (!hose->iommu_enable)
		return;

	sdev = dev_iommu_priv_get(dev);
	if (sdev->passthrough)
		return;

	sdomain = get_sunway_domain(dev);
	dma_dom = to_dma_domain(sdomain);
	__sunway_unmap_single(dma_dom, dma_addr, size);
}

#define SG_ENT_VIRT_ADDRESS(SG) (sg_virt((SG)))
static int
sunway_map_sg(struct device *dev, struct scatterlist *sgl,
	      int nents, enum dma_data_direction dir, unsigned long attrs)
{
	struct sunway_iommu_domain *sdomain;
	struct dma_domain *dma_dom = NULL;
	struct scatterlist *sg;
	struct pci_dev *pdev = to_pci_dev(dev);
	struct pci_controller *hose;
	struct sunway_iommu_dev *sdev;
	int i, out_nents = 0;

	if (dir == PCI_DMA_NONE)
		BUG();

	if (!pdev)
		return 0;

	hose = pci_bus_to_pci_controller(pdev->bus);
	if (!hose)
		return 0;

	sdomain = get_sunway_domain(dev);
	dma_dom = to_dma_domain(sdomain);

	for_each_sg(sgl, sg, nents, i) {
		BUG_ON(!sg_page(sg));

		sg_dma_address(sg) = __pa(SG_ENT_VIRT_ADDRESS(sg));
		if (!(hose->iommu_enable))
			goto check;

		sdev = dev_iommu_priv_get(dev);
		if (sdev->passthrough & DMA_MASK64)
			goto check;
		else if (sdev->passthrough) {
			if (min(dev->coherent_dma_mask, *dev->dma_mask) > DMA_BIT_MASK(32)) {
				sdev->passthrough |= DMA_MASK64;
				goto check;
			}

			set_dma_ops(dev, get_arch_dma_ops(dev->bus));
			return dev->dma_ops->map_sg(dev, sgl, nents, dir, attrs);
		}

		sg_dma_address(sg) =
			pci_iommu_map_single(pdev, dma_dom,
					SG_ENT_VIRT_ADDRESS(sg), sg->length);
check:
		if (sg_dma_address(sg) == 0)
			goto error;

		sg_dma_len(sg) = sg->length;
		out_nents++;
	}

	return nents;

error:
	pr_warn("pci_map_sg failed:");
	pr_warn("could not allocate dma page tables\n");

	if (out_nents)
		pci_unmap_sg(pdev, sgl, out_nents, dir);
	return 0;
}

static void
sunway_unmap_sg(struct device *dev, struct scatterlist *sgl,
		int nents, enum dma_data_direction dir, unsigned long attrs)
{
	struct sunway_iommu_domain *sdomain;
	struct dma_domain *dma_dom;
	struct scatterlist *sg;
	struct pci_dev *pdev;
	struct pci_controller *hose;
	struct sunway_iommu_dev *sdev;
	dma_addr_t dma_addr;
	long size;
	int j;

	pdev = to_pci_dev(dev);
	if (!pdev)
		return;

	hose = pci_bus_to_pci_controller(pdev->bus);
	if (!hose->iommu_enable)
		return;

	sdev = dev_iommu_priv_get(dev);
	if (sdev->passthrough)
		return;

	sdomain = get_sunway_domain(dev);
	dma_dom = to_dma_domain(sdomain);

	for_each_sg(sgl, sg, nents, j) {
		dma_addr = sg->dma_address;
		size = sg->dma_length;
		if (!size)
			break;

		__sunway_unmap_single(dma_dom, dma_addr, size);
	}
}

static const struct dma_map_ops sunway_dma_ops = {
	.alloc = sunway_alloc_coherent,
	.free = sunway_free_coherent,
	.map_sg = sunway_map_sg,
	.unmap_sg = sunway_unmap_sg,
	.map_page = sunway_map_page,
	.unmap_page = sunway_unmap_page,
	.dma_supported = dma_direct_supported,
};

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

static void sunway_iommu_detach_device(struct iommu_domain *dom, struct device *dev)
{
	struct sunway_iommu_dev *sdev;
	struct pci_dev *pdev = to_pci_dev(dev);

	if (!pdev)
		return;

	sdev = dev_iommu_priv_get(dev);
	if (sdev->domain != NULL)
		detach_device(dev);
}

static phys_addr_t
sunway_iommu_iova_to_phys(struct iommu_domain *dom, dma_addr_t iova)
{
	struct sunway_iommu_domain *sdomain = to_sunway_domain(dom);
	unsigned long paddr, grn;
	unsigned long is_last;

	if (iova > SW64_BAR_ADDRESS)
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
	paddr += iova & PAGE_MASK;
	return paddr;
}

static int
sunway_iommu_map(struct iommu_domain *dom, unsigned long iova,
		 phys_addr_t paddr, size_t page_size, int iommu_prot, gfp_t gfp)
{
	struct sunway_iommu_domain *sdomain = to_sunway_domain(dom);
	int ret;

	/*
	 * As VFIO cannot distinguish between normal DMA request
	 * and pci device BAR, check should be introduced manually
	 * to avoid VFIO trying to map pci config space.
	 */
	if (iova > SW64_BAR_ADDRESS)
		return 0;

	mutex_lock(&sdomain->api_lock);
	ret = sunway_iommu_map_page(sdomain, iova, paddr, page_size);
	mutex_unlock(&sdomain->api_lock);

	return ret;
}

static size_t
sunway_iommu_unmap(struct iommu_domain *dom, unsigned long iova,
			size_t page_size,
			struct iommu_iotlb_gather *gather)
{
	struct sunway_iommu_domain *sdomain = to_sunway_domain(dom);
	size_t unmap_size;

	if (iova > SW64_BAR_ADDRESS)
		return page_size;

	mutex_lock(&sdomain->api_lock);
	unmap_size = sunway_iommu_unmap_page(sdomain, iova, page_size);
	mutex_unlock(&sdomain->api_lock);

	return unmap_size;
}

static struct iommu_group *sunway_iommu_device_group(struct device *dev)
{
	return pci_device_group(dev);
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
	struct sunway_iommu_dev *sdev;

	sdev = dev_iommu_priv_get(dev);
	if (sdev->domain)
		return 0;

	return sdev->domain->type;
}

static bool sunway_iommu_capable(enum iommu_cap cap)
{
	switch (cap) {
	case IOMMU_CAP_INTR_REMAP:
		return true;
	default:
		return false;
	}
}

static void sunway_iommu_probe_finalize(struct device *dev)
{
	struct iommu_domain *domain;

	domain = iommu_get_domain_for_dev(dev);
	if (domain)
		set_dma_ops(dev, &sunway_dma_ops);
}

const struct iommu_ops sunway_iommu_ops = {
	.capable = sunway_iommu_capable,
	.domain_alloc = sunway_iommu_domain_alloc,
	.domain_free = sunway_iommu_domain_free,
	.attach_dev = sunway_iommu_attach_device,
	.detach_dev = sunway_iommu_detach_device,
	.probe_device = sunway_iommu_probe_device,
	.probe_finalize = sunway_iommu_probe_finalize,
	.release_device = sunway_iommu_release_device,
	.map = sunway_iommu_map,
	.unmap = sunway_iommu_unmap,
	.iova_to_phys = sunway_iommu_iova_to_phys,
	.device_group = sunway_iommu_device_group,
	.pgsize_bitmap = SW64_IOMMU_PGSIZES,
	.def_domain_type = sunway_iommu_def_domain_type,
};

/*****************************************************************************
 *
 * Boot param handle
 * Each bit of iommu_enable bitmap represents an rc enable, and every 8 bits
 * represents one cpu node. For example, iommu_enable=0x0100 means enabling
 * rc0 for cpu node 1.
 *
 *****************************************************************************/
static int __init iommu_enable_setup(char *str)
{
	int ret;
	unsigned long rc_bitmap = 0xffffffffUL;

	ret = kstrtoul(str, 16, &rc_bitmap);
	iommu_enable_cmd = rc_bitmap;

	return ret;
}
__setup("iommu_enable=", iommu_enable_setup);

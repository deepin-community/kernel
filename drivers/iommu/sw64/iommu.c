// SPDX-License-Identifier: GPL-2.0
/*
 * iommu.c: Generic sw64 IOMMU support
 *
 * This is designed and tested for 3231. If there are no changes in hardware
 * in later chips, then it should work just as well.
 *
 */
#define pr_fmt(fmt) "IOMMU: " fmt

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
#include <linux/io.h>

#include <asm/dma.h>
#include <asm/sw64io.h>

#include "iommu.h"

#define SW64_BAR_ADDRESS (IO_BASE | PCI_BASE)

#define SW64_IOMMU_GRN_8K		((0UL) << 4)	/* page size as 8KB */
#define SW64_IOMMU_GRN_8M		((0x2UL) << 4)	/* page size as 8MB */
#define SW64_IOMMU_PGSIZES (((1ULL) << PAGE_SHIFT) | ((1ULL) << PAGE_8M_SHIFT))

#define IDENTMAP_ALL	((1U) << 0)

#define MAX_NR_IOMMU_PER_NODE 8

/* IOMMU Exceptional Status */
enum exceptype {
	DTE_LEVEL1 = 0x0,
	DTE_LEVEL2,
	PTE_LEVEL1,
	PTE_LEVEL2,
	UNAUTHORIZED_ACCESS,
	ILLEGAL_RESPONSE,
	DTE_LEVEL1_VAL,
	DTE_LEVEL2_VAL,
	PTE_LEVEL1_VAL,
	PTE_LEVEL2_VAL,
};

DECLARE_BITMAP(iommu_bitmap, 32);

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

void flush_pcache_by_addr(struct sunway_iommu_domain *sdomain,
			unsigned long flush_addr)
{
	struct pci_controller *hose;
	struct sunway_iommu *iommu;
	struct sunway_iommu_dev *sdev_data;

	list_for_each_entry(sdev_data, &sdomain->dev_list, list) {
		hose = pci_bus_to_pci_controller(sdev_data->pdev->bus);
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

void flush_ptlb_by_addr(struct sunway_iommu_domain *sdomain,
			unsigned long flush_addr)
{
	struct pci_controller *hose;
	struct pci_dev *pdev;
	struct sunway_iommu_dev *sdev_data;
	struct sunway_iommu *iommu;
	unsigned long address;
	u16 alias, bus_number, devfn;

	list_for_each_entry(sdev_data, &sdomain->dev_list, list) {
		pdev = sdev_data->pdev;
		hose = pci_bus_to_pci_controller(pdev->bus);
		iommu = hose->pci_iommu;

		address = (pdev->bus->number << 8)
			    | pdev->devfn | (flush_addr << 16);
		do_ptlb_flush(iommu, address);

		if (sdev_data->alias != sdev_data->devid) {
			alias = sdev_data->alias;
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
	unsigned long pde;
	unsigned long *pde_ptr;
	int i, pdes_one_page;

	pde_ptr = sdomain->pt_root;
	if (!pde_ptr)
		return;

	pdes_one_page = PAGE_SIZE/sizeof(pde);
	for (i = 0; i < pdes_one_page; i++, pde_ptr++) {
		pde = *pde_ptr;
		if ((pde & SW64_IOMMU_ENTRY_VALID) == 0)
			continue;

		pde &= ~(SW64_IOMMU_ENTRY_VALID) & PAGE_MASK;
		pde |= PAGE_OFFSET;
		free_page(pde);
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

	node = node_online(iommu->node) ? iommu->node : NUMA_NO_NODE;
	if (!dte_l1_val) {
		/* Alloc a new level-2 device table page */
		dt_page = alloc_pages_node(node, GFP_ATOMIC | __GFP_ZERO,
				get_order(PAGE_SIZE));
		if (!dt_page)
			return -ENOMEM;

		dte_l2_base = (unsigned long)page_address(dt_page);
		dte_l1_val = (__pa(dte_l2_base) & PAGE_MASK) | SW64_IOMMU_ENTRY_VALID;
		*dte_l1 = dte_l1_val;
	}

	if (!sdomain->pt_root) {
		pt_page = alloc_pages_node(node, GFP_ATOMIC | __GFP_ZERO, 0);
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

static void
set_dte_entry(struct sunway_iommu_dev *sdev, struct sunway_iommu_domain *sdomain)
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

	pr_debug("device %d add to domain: %d\n",
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
	struct sunway_iommu_dev *sdev_data;
	unsigned long flags;

	sdev_data = dev_iommu_priv_get(dev);
	sunway_domain = sdev_data->domain;

	if (WARN_ON(!sdev_data->domain))
		return;

	spin_lock_irqsave(&sunway_iommu_device_table_lock, flags);
	__detach_device(sdev_data);
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
	unsigned long iova_pfn, pte_l1_val, pte_l2_val;
	unsigned long *pte_l1, *pte_l2;
	unsigned long  pte_root;
	unsigned long offset;

	if (!sdomain)
		return -EINVAL;

	pte_root = __pa(sdomain->pt_root) & PAGE_MASK;
	iova_pfn = iova >> PAGE_SHIFT;
	pte_root = ((pte_root) & (~(SW64_IOMMU_ENTRY_VALID)) & (PAGE_MASK));
	pte_root |= PAGE_OFFSET;
	offset = ((iova_pfn >> 10) & SW64_IOMMU_LEVEL1_OFFSET) << 3;
	pte_l1 = (unsigned long *)(pte_root + offset);
	if (type == PTE_LEVEL1)
		return (unsigned long)pte_l1;

	pte_l1_val = *pte_l1;
	if (type == PTE_LEVEL1_VAL)
		return pte_l1_val;

	pte_l1_val &= (~(SW64_IOMMU_ENTRY_VALID)) & (PAGE_MASK);
	pte_l1_val |= PAGE_OFFSET;
	offset = (iova_pfn & SW64_IOMMU_LEVEL2_OFFSET) << 3;
	pte_l2 = (unsigned long *)(pte_l1_val + offset);

	if (type == PTE_LEVEL2)
		return (unsigned long)pte_l2;

	pte_l2_val = *pte_l2;
	if (type == PTE_LEVEL2_VAL)
		return pte_l2_val;

	return pte_l2_val;
}

/* IOMMU Interrupt handle */
irqreturn_t iommu_interrupt(int irq, void *dev)
{
	struct pci_controller *hose = (struct pci_controller *)dev;
	struct sunway_iommu_domain *sdomain;
	struct sunway_iommu_dev *sdev;
	struct sunway_iommu *iommu;
	unsigned long iommu_status;
	unsigned long type;
	unsigned long devid, dva;

	iommu = hose->pci_iommu;
	iommu_status = readq(iommu->reg_base_addr + IOMMUEXCPT_STATUS);
	if (!(iommu_status >> 63))
		return IRQ_NONE;

	type = (iommu_status >> 59) & 0x7;
	devid = (iommu_status >> 37) & 0xffff;
	dva = iommu_status & 0xffffffff;
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
		break;
	case PTE_LEVEL2:
		pr_info("invalid level2 pte, addr: %#lx, val: %#lx\n",
			fetch_pte(sdomain, dva, PTE_LEVEL2),
			fetch_pte(sdomain, dva, PTE_LEVEL2_VAL));

		iommu_status &= ~(1UL << 62);
		writeq(iommu_status, iommu->reg_base_addr + IOMMUEXCPT_STATUS);
		break;

	case UNAUTHORIZED_ACCESS:
		pr_info("unauthorized access\n");
		break;
	case ILLEGAL_RESPONSE:
		pr_info("illegal response\n");
		break;
	default:
		pr_info("unknown error\n");
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

static struct sunway_iommu *sunway_iommu_early_init(struct pci_controller *hose)
{
	struct sunway_iommu *iommu;
	struct page *page;
	unsigned long base;
	int ret = 0, node;

	iommu = kzalloc(sizeof(struct sunway_iommu), GFP_KERNEL);
	if (!iommu) {
		ret = -ENOMEM;
		return 0;
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

	return ERR_PTR(ret);
}

static int sunway_iommu_init(void)
{
	struct pci_controller *hose;
	struct sunway_iommu *iommu;
	unsigned long rc_mask;
	int iommu_index = 0;
	int ret;

	sunway_iommu_domain_bitmap =
		(void *)__get_free_pages(GFP_KERNEL | __GFP_ZERO,
				get_order(MAX_DOMAIN_NUM / 8));
	if (sunway_iommu_domain_bitmap == NULL)
		return 0;
	__set_bit(0, sunway_iommu_domain_bitmap);

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
		if (!iommu) {
			pr_err("Allocating sunway_iommu failed\n");
			hose->iommu_enable = false;
			continue;
		}

		iommu_device_sysfs_add(&iommu->iommu, NULL, NULL, "%d",
					iommu_index);
		iommu_device_register(&iommu->iommu, &sunway_iommu_ops, NULL);

		sunway_enable_iommu_func(hose);
		hose->iommu_enable = true;
		piu_flush_all(iommu);

		iommu_index++;
	}

	ret = iova_cache_get();
	if (ret)
		return ret;

	register_syscore_ops(&iommu_cpu_syscore_ops);

	return 0;
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
	unsigned long *pte_l2, unmapped;

	pr_debug("%s iova %#lx, page_size %#lx\n", __func__, iova, page_size);
	BUG_ON(!is_power_of_2(page_size));

	unmapped = 0;
	while (unmapped < page_size) {
		pte_l2 = (unsigned long *)fetch_pte(sunway_domain, iova, PTE_LEVEL2);
		*pte_l2 = 0;

		flush_pcache_by_addr(sunway_domain, (unsigned long)pte_l2);
		flush_ptlb_by_addr(sunway_domain, (iova >> PAGE_SHIFT));

		iova += PAGE_SIZE;
		unmapped += PAGE_SIZE;
	}

	return unmapped;
}

int sunway_iommu_map_page(struct sunway_iommu_domain *sunway_domain,
		      unsigned long bus_addr, unsigned long paddr,
		      size_t page_size, int iommu_prot)
{
	/*
	 * pde: page table entry
	 * pte: level 2 page table entry
	 * pte_root: page table root
	 */
	struct page *page;
	struct sunway_iommu *iommu;
	unsigned long pde, pte, iova_pfn;
	unsigned long pdebaseaddr;
	u64 *ptebasecond, ptebaseaddr;
	u64 pte_root = (__pa(sunway_domain->pt_root) & PAGE_MASK);

	iova_pfn = (unsigned long)(bus_addr >> PAGE_SHIFT);

	pdebaseaddr = ((iova_pfn >> 10) & SW64_IOMMU_LEVEL1_OFFSET) << 3;
	pdebaseaddr += ((pte_root) & (~(SW64_IOMMU_ENTRY_VALID)) & (PAGE_MASK))
			+ PAGE_OFFSET;

	pde = *(unsigned long *)pdebaseaddr;
	if (pde) {
		ptebaseaddr = (pde & (~SW64_IOMMU_ENTRY_VALID) & PAGE_MASK) + PAGE_OFFSET;
		ptebaseaddr += (iova_pfn & SW64_IOMMU_LEVEL2_OFFSET) << 3;

		goto direct_map;
	}

	iommu = get_first_iommu_from_domain(sunway_domain);
	if (!iommu)
		return -1;
	page = alloc_pages_node(iommu->node, GFP_ATOMIC | __GFP_ZERO, 0);
	if (!page) {
		pr_err("Allocating pages failed.\n");
		return -1;
	}

	ptebasecond = page_address(page);
	pde = (__pa(ptebasecond) & PAGE_MASK) | SW64_IOMMU_ENTRY_VALID;

	/*
	 * If pde exists, no need to allocate a new page.
	 * Atomic compare and exchange, compare the value the pointer points to
	 * with 0UL. If identical, store pde where the pointer points to, return
	 * 0UL. Otherwise, return the value the pointer points to.
	 */
	if (cmpxchg64((volatile u64 *)pdebaseaddr, 0ULL, pde)) {
		ptebaseaddr = ((*(volatile u64 *)pdebaseaddr)
				& (~SW64_IOMMU_ENTRY_VALID) & PAGE_MASK) + PAGE_OFFSET;
		ptebaseaddr += (iova_pfn & SW64_IOMMU_LEVEL2_OFFSET) << 3;
		free_page((unsigned long)ptebasecond);
	} else {
		flush_pcache_by_addr(sunway_domain, pdebaseaddr);
		ptebaseaddr = (unsigned long)ptebasecond
			+ ((iova_pfn & SW64_IOMMU_LEVEL2_OFFSET) << 3);
	}

direct_map:
	/* case 8K */
	if (page_size == (1UL << PAGE_SHIFT)) {
		if (*(volatile u64 *)ptebaseaddr) {
			pr_err("IOVA 4G overlap. IOVA is %#lx.\n", bus_addr);
			return -EFAULT;
		}

		pte = (paddr & PAGE_MASK) | SW64_IOMMU_ENTRY_VALID
		    | SW64_IOMMU_GRN_8K;

		pte |= PTE_READE;
		if (iommu_prot & IOMMU_WRITE)
			pte |= PTE_WRITEE;

		*(volatile u64 *)ptebaseaddr = pte;
		flush_pcache_by_addr(sunway_domain, ptebaseaddr);
	/* case 8M */
	} else if (page_size == (1UL << PAGE_8M_SHIFT)) {
		unsigned long *ptr;
		int i, ptes_one_page, ptes_one_cache;

		ptr = (unsigned long *)ptebaseaddr;
		ptes_one_page = PAGE_SIZE/sizeof(pte);
		ptes_one_cache = L1_CACHE_BYTES/sizeof(pte);

		pte = (paddr & PAGE_MASK) | SW64_IOMMU_ENTRY_VALID
		    | SW64_IOMMU_GRN_8M;

		pte |= PTE_READE;
		if (iommu_prot & IOMMU_WRITE)
			pte |= PTE_WRITEE;

		for (i = 0; i < ptes_one_page; i++) {
			if (*ptr) {
				pr_err("IOVA 4G overlap. IOVA is %#lx.\n", bus_addr);
				return -EFAULT;
			}

			*ptr = pte;

			/* just do once flush per cache line */
			if (i % ptes_one_cache == (ptes_one_cache - 1))
				flush_pcache_by_addr(sunway_domain, (unsigned long)ptr);
			ptr++;
		}
	}
#ifdef CONFIG_SW64_GUEST
	flush_ptlb_by_addr(sunway_domain, pfn | SW64_IOMMU_MAP_FLAG);
#endif
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

		sdomain->pt_root = (void *)get_zeroed_page(GFP_KERNEL);
		if (!sdomain->pt_root) {
			pr_err("Allocating pt_root failed!\n");
			sunway_domain_free(sdomain);
			return NULL;
		}

		sdomain->domain.geometry.aperture_start = 0ULL;
		sdomain->domain.geometry.aperture_end	= (~0ULL);
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

		sdomain->pt_root = (void *)get_zeroed_page(GFP_KERNEL);
		if (!sdomain->pt_root) {
			pr_err("Allocating pt_root failed!\n");
			sunway_domain_free(sdomain);
			return NULL;
		}

		sdomain->type = IOMMU_DOMAIN_IDENTITY;
		iommu_identity_mapping = 1;
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
	struct sunway_iommu_dev *sdev_data;
	struct pci_dev *pdev;
	struct pci_controller *hose;
	int ret;

	if (!dev_is_pci(dev))
		return -ENODEV;

	pdev = to_pci_dev(dev);
	if (!pdev)
		return -EINVAL;

	hose = pci_bus_to_pci_controller(pdev->bus);
	if (!hose)
		return -EINVAL;

	if (!hose->iommu_enable)
		return -EINVAL;

	sdev_data = dev_iommu_priv_get(dev);
	if (!sdev_data)
		return -EINVAL;

	if (sdev_data->domain)
		detach_device(dev);

	ret = attach_device(dev, sdomain);

	return ret;
}

static phys_addr_t
sunway_iommu_iova_to_phys(struct iommu_domain *dom, dma_addr_t iova)
{
	struct sunway_iommu_domain *sdomain = to_sunway_domain(dom);
	unsigned long paddr, grn;

	if (iova >= SW64_BAR_ADDRESS)
		return iova;

	paddr = fetch_pte(sdomain, iova, PTE_LEVEL2_VAL);

	if ((paddr & SW64_IOMMU_ENTRY_VALID) == 0)
		return 0;

	paddr &= ~SW64_IOMMU_ENTRY_VALID;
	grn = paddr & SW64_PTE_GRN_MASK;	/* get page granularity */
	paddr &= PAGE_MASK;

	switch (grn) {
	case SW64_IOMMU_GRN_8M:
		paddr += (iova & ~HPAGE_MASK);
		break;
	case SW64_IOMMU_GRN_8K:
	default:
		paddr += (iova & ~PAGE_MASK);
		break;
	}

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

	mutex_lock(&sdomain->api_lock);
	while (pgcount--) {
		ret = sunway_iommu_map_page(sdomain, iova, paddr, page_size, iommu_prot);
		if (ret) {
			pr_info("Failed to map page from IOVA %lx.\n", iova);
			return ret;
		}
		iova += page_size;
		paddr += page_size;
	}
	mutex_unlock(&sdomain->api_lock);

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

	mutex_lock(&sdomain->api_lock);
	while (pgcount--) {
		unmap_size = sunway_iommu_unmap_page(sdomain, iova, page_size);
		iova += page_size;
		total_unmap += page_size;
	}
	mutex_unlock(&sdomain->api_lock);

	return total_unmap;
}

static struct iommu_group *sunway_iommu_device_group(struct device *dev)
{
	return generic_device_group(dev);
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

static struct iommu_device *sunway_iommu_probe_device(struct device *dev)
{
	struct pci_dev *pdev;
	struct pci_controller *hose;
	struct sunway_iommu *iommu;
	int ret;

	if (!dev_is_pci(dev))
		return ERR_PTR(-ENODEV);

	pdev = to_pci_dev(dev);
	if (!pdev)
		return ERR_PTR(-ENODEV);

	hose = pci_bus_to_pci_controller(pdev->bus);
	if (!hose)
		return ERR_PTR(-ENODEV);

	if (!hose->iommu_enable)
		return ERR_PTR(-ENODEV);

	if (dev_iommu_priv_get(dev))
		return &iommu->iommu;

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
	return false;
}

static void sunway_iommu_probe_finalize(struct device *dev)
{
	struct iommu_domain *domain;

	domain = iommu_get_domain_for_dev(dev);
	if (domain->type == IOMMU_DOMAIN_DMA) {
		iommu_setup_dma_ops(dev, 0, SW64_32BIT_DMA_LIMIT);
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
	.pgsize_bitmap = SW64_IOMMU_PGSIZES,
	.def_domain_type = sunway_iommu_def_domain_type,
	.default_domain_ops = &(const struct iommu_domain_ops) {
		.attach_dev = sunway_iommu_attach_device,
		.map_pages = sunway_iommu_map_pages,
		.unmap_pages = sunway_iommu_unmap_pages,
		.iova_to_phys = sunway_iommu_iova_to_phys,
		.free = sunway_iommu_domain_free,
	}
};

/*****************************************************************************
 *
 * Boot param handle
 * Each bit of iommu_enable bitmap represents an rc enable, and every 8 bits
 * represents one cpu node. For example, iommu_enable=0x0100 means enabling
 * rc0 for cpu node 1.
 *
 *****************************************************************************/
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

	pr_info("iommu_enable= deprecated; use sunway_iommu=on/off instead.\n");
	bitmap_zero(iommu_bitmap, 64);

	ret = kstrtoul(str, 16, &rc_val);
	if (!ret)
		return -EINVAL;

	bitmap_from_u64(iommu_bitmap, rc_val);

	return 1;
}
__setup("iommu_enable=", iommu_enable_setup);

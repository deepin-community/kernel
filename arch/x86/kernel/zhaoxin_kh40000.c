// SPDX-License-Identifier: GPL-2.0
#include <linux/device.h>
#include <linux/dma-direct.h>
#include <linux/dma-map-ops.h>
#include <linux/dma-mapping.h>
#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/iommu.h>
#include <linux/kstrtox.h>
#include <linux/pci.h>
#include <linux/pfn.h>
#include <linux/printk.h>
#include <linux/scatterlist.h>
#include <linux/types.h>

#include <asm/dma-mapping.h>

#include "../../../kernel/dma/direct.h"

/***
 * usage:
 *  set "zhaoxin_patch_bitmask=<value>" in cmdline
 * value description:
 *  bit 0: enable(1) node check or not(0). default 1
 */
enum {
	ZHAOXIN_P2CW_NODE_CHECK = BIT(0),
	ZHAOXIN_PATCH_CODE_MAX  = ZHAOXIN_P2CW_NODE_CHECK,
};

#define ZHAOXIN_PATCH_CODE_DEFAULT	ZHAOXIN_P2CW_NODE_CHECK

unsigned long zhaoxin_patch_code = ZHAOXIN_PATCH_CODE_DEFAULT;

static int __init zhaoxin_patch_code_setup(char *str)
{
	int err = kstrtoul(str, 0, &zhaoxin_patch_code);

	if (err || (zhaoxin_patch_code > ZHAOXIN_PATCH_CODE_MAX)) {
		pr_err("cmdline 'zhaoxin_patch_bitmask=%s' inappropriate\n",
				str);
		return err;
	}

	if (ZHAOXIN_P2CW_NODE_CHECK | zhaoxin_patch_code)
		pr_info("zhaoxin dma patch node check is enabled\n");

	return 0;
}
__setup("zhaoxin_patch_bitmask=", zhaoxin_patch_code_setup);

static struct pci_dev *kh40000_get_pci_dev(struct device *dev)
{
	if (dev_is_pci(dev))
		return to_pci_dev(dev);

	if (dev->parent)
		return kh40000_get_pci_dev(dev->parent);

	return NULL;
}

static void kh40000_sync_single_dma_for_cpu(struct device *dev, dma_addr_t paddr,
		enum dma_data_direction dir, bool is_iommu)
{
	u8 vid;
	struct pci_dev *pci;
	u64 dma_mask = *dev->dma_mask;

	/* check direction */
	if ((dir != DMA_FROM_DEVICE) && (dir != DMA_BIDIRECTIONAL))
		return;

	/* check dma capability */
	if (dma_mask <= DMA_BIT_MASK(32))
		return;

	/* check device type */
	pci = kh40000_get_pci_dev(dev);
	if (pci == NULL)
		return;

	/* get real physical address */
	if (is_iommu) {
		struct iommu_domain *domain = iommu_get_dma_domain(dev);

		paddr = iommu_iova_to_phys(domain, paddr);
		if (!paddr)
			return;
	}

	/* check node or not */
	if ((zhaoxin_patch_code & ZHAOXIN_P2CW_NODE_CHECK)
	   && pfn_to_nid(PFN_DOWN(paddr)) == dev_to_node(dev))
		return;

	/* flush data by one pci read cycle */
	pci_read_config_byte(pci, PCI_VENDOR_ID, &vid);
}

/* zhaoxin kh-40000 direct dma ops */
static void *kh40000_dma_direct_alloc(struct device *dev, size_t size,
		dma_addr_t *addr, gfp_t gfp, unsigned long attrs)
{
	if (dev->coherent_dma_mask > DMA_BIT_MASK(32))
		gfp |= __GFP_THISNODE;

	return dma_direct_alloc(dev, size, addr, gfp, attrs);
}

static void kh40000_dma_direct_unmap_page(struct device *dev, dma_addr_t addr,
		size_t size, enum dma_data_direction dir, unsigned long attrs)
{
	kh40000_sync_single_dma_for_cpu(dev, addr, dir, 0);
	dma_direct_unmap_page(dev, addr, size, dir, attrs);
}

static void kh40000_dma_direct_sync_sg_for_cpu(struct device *dev,
		struct scatterlist *sgl, int nents, enum dma_data_direction dir)
{
	struct scatterlist *sg;
	int i;

	for_each_sg(sgl,  sg, nents, i)
		kh40000_sync_single_dma_for_cpu(dev, sg_dma_address(sg), dir, 0);

	dma_direct_sync_sg_for_cpu(dev, sgl, nents, dir);
}

static void kh40000_dma_direct_sync_single_for_cpu(struct device *dev,
		dma_addr_t addr, size_t size, enum dma_data_direction dir)
{
	kh40000_sync_single_dma_for_cpu(dev, addr, dir, 0);
	dma_direct_sync_single_for_cpu(dev, addr, size, dir);
}

static void kh40000_dma_direct_unmap_sg(struct device *dev, struct scatterlist *sgl,
		int nents, enum dma_data_direction dir, unsigned long attrs)
{
	struct scatterlist *sg;
	int i;

	for_each_sg(sgl,  sg, nents, i)
		kh40000_sync_single_dma_for_cpu(dev, sg_dma_address(sg), dir, 0);

	dma_direct_unmap_sg(dev, sgl, nents, dir, attrs);
}

static void kh40000_dma_direct_unmap_resource(struct device *dev, dma_addr_t addr,
		size_t size, enum dma_data_direction dir, unsigned long attrs)
{
	kh40000_sync_single_dma_for_cpu(dev, addr, dir, 0);
}

const struct dma_map_ops kh40000_dma_direct_ops = {
	.flags			= DMA_F_PCI_P2PDMA_SUPPORTED,
	.alloc			= kh40000_dma_direct_alloc,
	.sync_sg_for_cpu	= kh40000_dma_direct_sync_sg_for_cpu,
	.unmap_page		= kh40000_dma_direct_unmap_page,
	.sync_single_for_cpu	= kh40000_dma_direct_sync_single_for_cpu,
	.unmap_sg		= kh40000_dma_direct_unmap_sg,
	.unmap_resource		= kh40000_dma_direct_unmap_resource,
	.dma_supported		= dma_direct_supported,
	.free			= dma_direct_free,
	.alloc_pages		= dma_direct_alloc_pages,
	.free_pages		= dma_direct_free_pages,
	.sync_single_for_device	= dma_direct_sync_single_for_device,
	.sync_sg_for_device	= dma_direct_sync_sg_for_device,
	.get_required_mask	= dma_direct_get_required_mask,
	.max_mapping_size	= dma_direct_max_mapping_size,
	.mmap			= dma_direct_mmap,
	.get_sgtable		= dma_direct_get_sgtable,
	.map_page		= dma_direct_map_page,
	.map_sg			= dma_direct_map_sg,
	.map_resource		= dma_direct_map_resource,
};

/* zhaoxin kh-40000 iommu dma ops */
static const struct dma_map_ops *iommu_dma_ops;

static void *kh40000_iommu_dma_alloc(struct device *dev, size_t size,
		dma_addr_t *addr, gfp_t gfp, unsigned long attrs)
{
	gfp |= __GFP_THISNODE;

	return iommu_dma_ops->alloc(dev, size, addr, gfp, attrs);
}

static void kh40000_iommu_dma_free(struct device *dev, size_t size, void *cpu_addr,
		dma_addr_t handle, unsigned long attrs)
{
	iommu_dma_ops->free(dev, size, cpu_addr, handle, attrs);
}

static struct page *kh40000_dma_common_alloc_pages(struct device *dev, size_t size,
		dma_addr_t *dma_handle, enum dma_data_direction dir, gfp_t gfp)
{
	return iommu_dma_ops->alloc_pages(dev, size, dma_handle, dir, gfp);
}

static void kh40000_dma_common_free_pages(struct device *dev, size_t size, struct page *page,
		dma_addr_t dma_handle, enum dma_data_direction dir)
{
	iommu_dma_ops->free_pages(dev, size, page, dma_handle, dir);
}

static struct sg_table *kh40000_iommu_dma_alloc_noncontiguous(struct device *dev,
		size_t size, enum dma_data_direction dir, gfp_t gfp,
		unsigned long attrs)
{
	return iommu_dma_ops->alloc_noncontiguous(dev, size, dir, gfp, attrs);
}

static void kh40000_iommu_dma_free_noncontiguous(struct device *dev, size_t size,
		struct sg_table *sgt, enum dma_data_direction dir)
{
	return iommu_dma_ops->free_noncontiguous(dev, size, sgt, dir);
}

static int kh40000_iommu_dma_mmap(struct device *dev, struct vm_area_struct *vma,
		void *cpu_addr, dma_addr_t dma_addr, size_t size,
		unsigned long attrs)
{
	return iommu_dma_ops->mmap(dev, vma, cpu_addr, dma_addr, size, attrs);
}

static void kh40000_iommu_dma_unmap_page(struct device *dev, dma_addr_t addr,
		size_t size, enum dma_data_direction dir, unsigned long attrs)
{
	kh40000_sync_single_dma_for_cpu(dev, addr, dir, 1);
	iommu_dma_ops->unmap_page(dev, addr, size, dir, attrs);
}

static int kh40000_iommu_dma_get_sgtable(struct device *dev, struct sg_table *sgt,
		void *cpu_addr, dma_addr_t dma_addr, size_t size,
		unsigned long attrs)
{
	return iommu_dma_ops->get_sgtable(dev, sgt, cpu_addr, dma_addr, size, attrs);
}

static dma_addr_t kh40000_iommu_dma_map_page(struct device *dev, struct page *page,
		unsigned long offset, size_t size, enum dma_data_direction dir,
		unsigned long attrs)
{
	return iommu_dma_ops->map_page(dev, page, offset, size, dir, attrs);
}

static int kh40000_iommu_dma_map_sg(struct device *dev, struct scatterlist *sgl,
		int nents, enum dma_data_direction dir, unsigned long attrs)
{
	return iommu_dma_ops->map_sg(dev, sgl, nents, dir, attrs);
}

static void kh40000_iommu_dma_unmap_sg(struct device *dev, struct scatterlist *sgl,
		int nelems, enum dma_data_direction dir, unsigned long attrs)
{
	struct scatterlist *sg;
	int i;

	for_each_sg(sgl, sg, nelems, i)
		kh40000_sync_single_dma_for_cpu(dev, sg_dma_address(sg), dir, 1);
	iommu_dma_ops->unmap_sg(dev, sgl, nelems, dir, attrs);
}

static void kh40000_iommu_dma_sync_single_for_cpu(struct device *dev,
		dma_addr_t addr, size_t size, enum dma_data_direction dir)
{
	kh40000_sync_single_dma_for_cpu(dev, addr, dir, 1);
	iommu_dma_ops->sync_single_for_cpu(dev, addr, size, dir);
}

static void kh40000_iommu_dma_sync_single_for_device(struct device *dev,
		dma_addr_t addr, size_t size, enum dma_data_direction dir)
{
	iommu_dma_ops->sync_single_for_device(dev, addr, size, dir);
}

static void kh40000_iommu_dma_sync_sg_for_cpu(struct device *dev,
		struct scatterlist *sgl, int nelems,
		enum dma_data_direction dir)
{
	struct scatterlist *sg;
	int i;

	for_each_sg(sgl, sg, nelems, i)
		kh40000_sync_single_dma_for_cpu(dev, sg_dma_address(sg), dir, 1);
	iommu_dma_ops->sync_sg_for_cpu(dev, sgl, nelems, dir);
}

static void kh40000_iommu_dma_sync_sg_for_device(struct device *dev,
		struct scatterlist *sgl, int nelems,
		enum dma_data_direction dir)
{
	iommu_dma_ops->sync_sg_for_device(dev, sgl, nelems, dir);
}

static dma_addr_t kh40000_iommu_dma_map_resource(struct device *dev, phys_addr_t phys,
		size_t size, enum dma_data_direction dir, unsigned long attrs)
{
	return iommu_dma_ops->map_resource(dev, phys, size, dir, attrs);
}

static void kh40000_iommu_dma_unmap_resource(struct device *dev, dma_addr_t addr,
		size_t size, enum dma_data_direction dir, unsigned long attrs)
{
	kh40000_sync_single_dma_for_cpu(dev, addr, dir, 1);
	iommu_dma_ops->unmap_resource(dev, addr, size, dir, attrs);
}

static unsigned long kh40000_iommu_dma_get_merge_boundary(struct device *dev)
{
	return iommu_dma_ops->get_merge_boundary(dev);
}

static size_t kh40000_iommu_dma_opt_mapping_size(void)
{
	return iommu_dma_ops->opt_mapping_size();
}

const struct dma_map_ops kh40000_dma_iommu_ops = {
	.flags			= DMA_F_PCI_P2PDMA_SUPPORTED,
	.alloc			= kh40000_iommu_dma_alloc,
	.free			= kh40000_iommu_dma_free,
	.unmap_page		= kh40000_iommu_dma_unmap_page,
	.alloc_pages		= kh40000_dma_common_alloc_pages,
	.free_pages		= kh40000_dma_common_free_pages,
	.alloc_noncontiguous	= kh40000_iommu_dma_alloc_noncontiguous,
	.free_noncontiguous	= kh40000_iommu_dma_free_noncontiguous,
	.mmap			= kh40000_iommu_dma_mmap,
	.get_sgtable		= kh40000_iommu_dma_get_sgtable,
	.map_page		= kh40000_iommu_dma_map_page,
	.map_sg			= kh40000_iommu_dma_map_sg,
	.unmap_sg		= kh40000_iommu_dma_unmap_sg,
	.sync_single_for_cpu	= kh40000_iommu_dma_sync_single_for_cpu,
	.sync_single_for_device	= kh40000_iommu_dma_sync_single_for_device,
	.sync_sg_for_cpu	= kh40000_iommu_dma_sync_sg_for_cpu,
	.sync_sg_for_device	= kh40000_iommu_dma_sync_sg_for_device,
	.map_resource		= kh40000_iommu_dma_map_resource,
	.unmap_resource		= kh40000_iommu_dma_unmap_resource,
	.get_merge_boundary	= kh40000_iommu_dma_get_merge_boundary,
	.opt_mapping_size	= kh40000_iommu_dma_opt_mapping_size,
};

void kh40000_set_iommu_dma_ops(struct device *dev)
{
	if (dev->dma_ops) {
		iommu_dma_ops = dev->dma_ops;
		set_dma_ops(dev, &kh40000_dma_iommu_ops);
		pr_info_once("zhaoxin iommu dma patch enabled\n");
	}
}

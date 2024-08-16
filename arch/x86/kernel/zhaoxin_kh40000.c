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

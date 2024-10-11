// SPDX-License-Identifier: GPL-2.0
/*
 *	linux/arch/sw/kernel/pci-noop.c
 *
 * Stub PCI interfaces for NO PCI kernels.
 */

#include <linux/pci.h>
#include <linux/memblock.h>
#include <linux/dma-mapping.h>
#include <linux/dma-map-ops.h>

/*
 * The PCI controller list.
 */

struct pci_controller *hose_head, **hose_tail = &hose_head;

struct pci_controller * __init
alloc_pci_controller(void)
{
	struct pci_controller *hose;

	hose = memblock_alloc(sizeof(*hose), SMP_CACHE_BYTES);

	*hose_tail = hose;
	hose_tail = &hose->next;

	return hose;
}

struct resource * __init
alloc_resource(void)
{
	struct resource *res;

	res = memblock_alloc(sizeof(*res), SMP_CACHE_BYTES);

	return res;
}

asmlinkage long
sys_pciconfig_iobase(long which, unsigned long bus, unsigned long dfn)
{
	return -ENODEV;
}

asmlinkage long
sys_pciconfig_read(unsigned long bus, unsigned long dfn,
		   unsigned long off, unsigned long len, void *buf)
{
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	else
		return -ENODEV;
}

asmlinkage long
sys_pciconfig_write(unsigned long bus, unsigned long dfn,
		    unsigned long off, unsigned long len, void *buf)
{
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	else
		return -ENODEV;
}

static void *sw64_noop_alloc_coherent(struct device *dev, size_t size,
				       dma_addr_t *dma_handle, gfp_t gfp,
				       unsigned long attrs)
{
	void *ret;

	if (!dev || *dev->dma_mask >= 0xffffffffUL)
		gfp &= ~GFP_DMA;
	ret = (void *)__get_free_pages(gfp, get_order(size));
	if (ret) {
		memset(ret, 0, size);
		*dma_handle = virt_to_phys(ret);
	}
	return ret;
}

static void sw64_noop_free_coherent(struct device *dev, size_t size,
				     void *cpu_addr, dma_addr_t dma_addr,
				     unsigned long attrs)
{
	free_pages((unsigned long)cpu_addr, get_order(size));
}

static dma_addr_t sw64_noop_map_page(struct device *dev, struct page *page,
				      unsigned long offset, size_t size,
				      enum dma_data_direction dir,
				      unsigned long attrs)
{
	return page_to_pa(page) + offset;
}

static int sw64_noop_map_sg(struct device *dev, struct scatterlist *sgl, int nents,
			     enum dma_data_direction dir, unsigned long attrs)
{
	int i;
	struct scatterlist *sg;

	for_each_sg(sgl, sg, nents, i) {
		void *va;

		BUG_ON(!sg_page(sg));
		va = sg_virt(sg);
		sg_dma_address(sg) = (dma_addr_t)virt_to_phys(va);
		sg_dma_len(sg) = sg->length;
	}

	return nents;
}

static int sw64_noop_supported(struct device *dev, u64 mask)
{
	return mask < 0x00ffffffUL ? 0 : 1;
}

const struct dma_map_ops sw64_noop_ops = {
	.alloc			= sw64_noop_alloc_coherent,
	.free			= sw64_noop_free_coherent,
	.map_page		= sw64_noop_map_page,
	.map_sg			= sw64_noop_map_sg,
	.dma_supported		= sw64_noop_supported,
};

const struct dma_map_ops *dma_ops = &sw64_noop_ops;
EXPORT_SYMBOL(dma_ops);

void __init common_init_pci(void)
{
}

void __init sw64_init_arch(void) { }
void __init sunway_init_pci_intx(void) { }

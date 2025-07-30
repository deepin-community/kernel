/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_PSWIOTLB_H
#define __LINUX_PSWIOTLB_H

#include <linux/device.h>
#include <linux/dma-direction.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/limits.h>
#include <linux/spinlock.h>
#include <linux/pci.h>
#include <linux/workqueue.h>
#include <linux/arm-smccc.h>

struct device;
struct page;
struct scatterlist;
extern bool pswiotlb_force_disable;
struct p_io_tlb_pool;

#define SOC_ID_PS23064	0x8
#define SOC_ID_PS24080	0x6
#define MIDR_PS    0x700F8620
#define SYS_AIDR_EL1    sys_reg(3, 1, 0, 0, 7)
#define PSWIOTLB_VERBOSE	(1 << 0) /* verbose initialization */
#define PSWIOTLB_FORCEOFF	(1 << 1) /* force phytium bounce buffering off*/
#define PSWIOTLB_ANY	(1 << 2) /* allow any memory for the buffer */
#define PSWIOTLB_FREE_THRESHOLD 30
static bool is_ps_socs;

/*
 * Maximum allowable number of contiguous slabs to map,
 * must be a power of 2.  What is the appropriate value ?
 * The complexity of {map,unmap}_single is linearly dependent on this value.
 */
#define P_IO_TLB_SEGSIZE	1024

/*
 * log of the size of each Phytium IO TLB slab.  The number of slabs is command line
 * controllable.
 */
#define P_IO_TLB_SHIFT 11
#define P_IO_TLB_SIZE (1 << P_IO_TLB_SHIFT)

/* default to 256MB */
#define P_IO_TLB_DEFAULT_SIZE (256UL<<20)
#define P_IO_TLB_INC_THR (64UL<<20)
#define P_IO_TLB_EXT_WATERMARK (80)

/* passthroughlist which incompatible with pswiotlb temporarily */
#define BL_PCI_VENDOR_ID_NVIDIA          0x10de
#define BL_PCI_VENDOR_ID_ILUVATAR        0x1E3E
#define BL_PCI_VENDOR_ID_METAX			 0x9999

unsigned long pswiotlb_size_or_default(void);
void __init pswiotlb_init_remap(bool addressing_limit, int nid, unsigned int flags,
	int (*remap)(void *tlb, unsigned long nslabs));

phys_addr_t pswiotlb_tbl_map_single(struct device *hwdev, int nid, phys_addr_t phys,
		size_t mapping_size, size_t alloc_size, unsigned int alloc_align_mask,
		enum dma_data_direction dir,
		unsigned long attrs);

extern void pswiotlb_tbl_unmap_single(struct device *hwdev,
				     int nid,
					 phys_addr_t tlb_addr,
				     size_t offset,
				     size_t mapping_size,
				     enum dma_data_direction dir,
				     unsigned long attrs,
					 struct p_io_tlb_pool *pool);

void pswiotlb_sync_single_for_device(struct device *dev, int nid, phys_addr_t tlb_addr,
		size_t size, enum dma_data_direction dir, struct p_io_tlb_pool *pool);
void pswiotlb_sync_single_for_cpu(struct device *dev, int nid, phys_addr_t tlb_addr,
		size_t size, enum dma_data_direction dir, struct p_io_tlb_pool *pool);
dma_addr_t pswiotlb_map(struct device *dev, int nid, phys_addr_t phys,
		size_t size, enum dma_data_direction dir, unsigned long attrs);
void pswiotlb_store_local_node(struct pci_dev *dev, struct pci_bus *bus);
void iommu_dma_unmap_sg_pswiotlb(struct device *dev, struct scatterlist *sg, unsigned long iova,
			size_t mapped, int nents, enum dma_data_direction dir, unsigned long attrs);
#ifdef CONFIG_PSWIOTLB
struct pswiotlb_passthroughlist {
	struct list_head node;
	unsigned short vendor;
	unsigned short device;
	bool from_grub;
};

struct pswiotlb_bypass_rules {
	unsigned short vendor_id;
	bool dma_is_sg;
	enum dma_data_direction dir;
};

/**
 * struct p_io_tlb_pool - Phytium IO TLB memory pool descriptor
 * @start:	The start address of the pswiotlb memory pool. Used to do a quick
 *		range check to see if the memory was in fact allocated by this
 *		API.
 * @end:	The end address of the pswiotlb memory pool. Used to do a quick
 *		range check to see if the memory was in fact allocated by this
 *		API.
 * @nslabs:	The number of Phytium IO TLB blocks (in groups of 64) between @start and
 *		@end. For default pswiotlb, this is command line adjustable via
 *		setup_io_tlb_npages.
 * @used:	The number of used Phytium IO TLB block.
 * @list:	The free list describing the number of free entries available
 *		from each index.
 * @index:	The index to start searching in the next round.
 * @orig_addr:	The original address corresponding to a mapped entry.
 * @alloc_size:	Size of the allocated buffer.
 * @lock:	The lock to protect the above data structures in the map and
 *		unmap calls.
 * @vaddr:	The vaddr of the pswiotlb memory pool. The pswiotlb memory pool
 *		may be remapped in the memory encrypted case and store virtual
 *		address for bounce buffer operation.
 * @nslabs:	The number of Phytium IO TLB slots between @start and @end. For the
 *		default pswiotlb, this can be adjusted with a boot parameter,
 *		see setup_io_tlb_npages().
 * @late_alloc:	%true if allocated using the page allocator.
 * @nareas:	Number of areas in the pool.
 * @area_nslabs: Number of slots in each area.
 * @areas:	Array of memory area descriptors.
 * @slots:	Array of slot descriptors.
 * @node:	Member of the Phytium IO TLB memory pool list.
 * @rcu:	RCU head for pswiotlb_dyn_free().
 * @transient:  %true if transient memory pool.
 * @busy_flag:  %true if the pool is used by devices.
 * @free_cnt:  Counters every time the pool is free when checked by monitor.
 * @free_th:  Free threshold determine when to free the pool to memory.
 * @busy_recode:  Bitmap to record the busy status of the areas in the pool.
 * @node_min_addr:  Minimum physical address of the numa node.
 * @numa_max_addr:  Maximum physical address of the numa node.
 * @numa_node_id:  Numa node id the pool belong to.
 */
struct p_io_tlb_pool {
	phys_addr_t start;
	phys_addr_t end;
	void *vaddr;
	unsigned long nslabs;
	bool late_alloc;
	unsigned int nareas;
	unsigned int area_nslabs;
	struct p_io_tlb_area *areas;
	struct p_io_tlb_slot *slots;
	struct list_head node;
	struct rcu_head rcu;
	bool transient;
	bool busy_flag;
	unsigned int free_cnt;
	unsigned int free_th;
	unsigned long *busy_record;
	phys_addr_t node_min_addr;
	phys_addr_t node_max_addr;
	int numa_node_id;
};

/**
 * struct p_io_tlb_mem - Phytium Software IO TLB allocator
 * @defpool:	Default (initial) Phytium IO TLB memory pool descriptor.
 * @pool:	Phytium IO TLB memory pool descriptor (if not dynamic).
 * @nslabs:	Total number of Phytium IO TLB slabs in all pools.
 * @debugfs:	The dentry to debugfs.
 * @force_bounce: %true if pswiotlb bouncing is forced
 * @for_alloc:  %true if the pool is used for memory allocation
 * @can_grow:	%true if more pools can be allocated dynamically.
 * @phys_limit:	Maximum allowed physical address.
 * @pool_addr:	Array where all the pools stored.
 * @capacity:	Number of pools which could be allocated.
 * @whole_size:	Number of pools which stored in the pool array.
 * @lock:	Lock to synchronize changes to the list.
 * @pools:	List of Phytium IO TLB memory pool descriptors (if dynamic).
 * @dyn_alloc:	Dynamic Phytium IO TLB pool allocation work.
 * @total_used:	The total number of slots in the pool that are currently used
 *		across all areas. Used only for calculating used_hiwater in
 *		debugfs.
 * @used_hiwater: The high water mark for total_used.  Used only for reporting
 *		in debugfs.
 * @node_min_addr:  Minimum physical address of the numa node.
 * @numa_max_addr:  Maximum physical address of the numa node.
 * @numa_node_id:  Numa node id the mem belong to.
 */
struct p_io_tlb_mem {
	struct p_io_tlb_pool defpool;
	unsigned long nslabs;
	struct dentry *debugfs;
	bool force_bounce;
	bool for_alloc;
	bool can_grow;
	u64 phys_limit;
	struct p_io_tlb_pool *pool_addr[64*1024/8];
	int capacity;
	int whole_size;
	spinlock_t lock;
	struct list_head pools;
	struct work_struct dyn_alloc;
#ifdef CONFIG_DEBUG_FS
	atomic_long_t total_used;
	atomic_long_t used_hiwater;
#endif
	phys_addr_t node_min_addr;
	phys_addr_t node_max_addr;
	unsigned long node_total_mem;
	int numa_node_id;
};

extern struct p_io_tlb_mem p_io_tlb_default_mem[MAX_NUMNODES];

struct p_io_tlb_pool *pswiotlb_find_pool(struct device *dev, int nid, phys_addr_t paddr);

static inline bool is_phytium_ps_socs(void)
{
	unsigned int soc_id;
	unsigned int midr;

	if (likely(is_ps_socs))
		return true;

	soc_id = read_sysreg_s(SYS_AIDR_EL1);
	midr = read_cpuid_id();
	if ((soc_id == SOC_ID_PS23064 || soc_id == SOC_ID_PS24080)
				&& midr == MIDR_PS) {
		is_ps_socs = true;
		return true;
	} else
		return false;
}

static inline bool is_pswiotlb_buffer(struct device *dev, int nid, phys_addr_t paddr,
			struct p_io_tlb_pool **pool)
{
	struct p_io_tlb_mem *mem = &dev->dma_p_io_tlb_mem[nid];
	struct page *page;

	if (!paddr || (paddr == DMA_MAPPING_ERROR))
		return false;

	page = pfn_to_page(PFN_DOWN(paddr));

	if (test_bit(PG_pswiotlb, &page->flags) == false)
		return false;

	if (!mem)
		return false;

	/*
	 * All PSWIOTLB buffer addresses must have been returned by
	 * pswiotlb_tbl_map_single() and passed to a device driver.
	 * If a PSWIOTLB address is checked on another CPU, then it was
	 * presumably loaded by the device driver from an unspecified private
	 * data structure. Make sure that this load is ordered before reading
	 * dev->dma_uses_p_io_tlb here and mem->pools in pswiotlb_find_pool().
	 *
	 * This barrier pairs with smp_mb() in pswiotlb_find_slots().
	 */
	smp_rmb();

	*pool = pswiotlb_find_pool(dev, nid, paddr);
	if (READ_ONCE(dev->dma_uses_p_io_tlb) && *pool)
		return true;

	return false;
}

static inline bool dma_is_in_local_node(struct device *dev, int nid, dma_addr_t addr, size_t size)
{
	dma_addr_t end = addr + size - 1;
	struct p_io_tlb_mem *mem = &p_io_tlb_default_mem[nid];

	if (addr >= mem->node_min_addr && end <= mem->node_max_addr)
		return true;

	return false;
}

void pswiotlb_init(bool addressing_limited, unsigned int flags);
void pswiotlb_dev_init(struct device *dev);
size_t pswiotlb_max_mapping_size(struct device *dev);
bool is_pswiotlb_allocated(struct device *dev);
bool is_pswiotlb_active(struct device *dev);
void __init pswiotlb_adjust_size(unsigned long size);
phys_addr_t default_pswiotlb_base(struct device *dev);
phys_addr_t default_pswiotlb_limit(struct device *dev);
bool pswiotlb_is_dev_in_passthroughlist(struct pci_dev *dev);

extern const struct pswiotlb_bypass_rules bypass_rules_list[];
static inline bool pswiotlb_bypass_is_needed(struct device *dev, int nelems,
			enum dma_data_direction dir)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	bool dma_is_sg = nelems ? true : false;
	const struct pswiotlb_bypass_rules *list = bypass_rules_list;

	while (list->vendor_id) {
		if ((pdev->vendor == list->vendor_id) &&
					(dma_is_sg == list->dma_is_sg) &&
					(dir == list->dir))
			return true;
		list++;
	}

	return false;
}

#else
static inline void pswiotlb_init(bool addressing_limited, unsigned int flags)
{
}

static inline void pswiotlb_dev_init(struct device *dev)
{
}
static inline bool is_pswiotlb_buffer(struct device *dev, int nid, phys_addr_t paddr,
			struct p_io_tlb_pool **pool)
{
	return false;
}
static inline bool dma_is_in_local_node(struct device *dev, int nid, dma_addr_t addr, size_t size)
{
	return false;
}
static inline size_t pswiotlb_max_mapping_size(struct device *dev)
{
	return SIZE_MAX;
}

static inline bool is_pswiotlb_allocated(struct device *dev)
{
	return false;
}
static inline bool is_pswiotlb_active(struct device *dev)
{
	return false;
}

static inline void pswiotlb_adjust_size(unsigned long size)
{
}

static inline phys_addr_t default_pswiotlb_base(struct device *dev)
{
	return 0;
}

static inline phys_addr_t default_pswiotlb_limit(struct device *dev)
{
	return 0;
}

static inline bool pswiotlb_is_dev_in_passthroughlist(struct pci_dev *dev)
{
	return false;
}

static inline bool pswiotlb_bypass_is_needed(struct device *dev, int nelems,
			enum dma_data_direction dir)
{
	return true;
}
#endif /* CONFIG_PSWIOTLB */

extern void pswiotlb_print_info(int);
extern bool pswiotlb_dma_coherent_ok(struct device *dev, phys_addr_t phys, size_t size);

#endif /* __LINUX_PSWIOTLB_H */

// SPDX-License-Identifier: GPL-2.0-only
/*
 * Phytium software IO tlb to improve DMA performance.
 *
 * Copyright (c) 2024, Phytium Technology Co., Ltd.
 */

#define pr_fmt(fmt) "Phytium software IO TLB: " fmt

#include <linux/cache.h>
#include <linux/ctype.h>
#include <linux/debugfs.h>
#include <linux/dma-direct.h>
#include <linux/dma-map-ops.h>
#include <linux/mm.h>
#include <linux/export.h>
#include <linux/gfp.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/iommu-helper.h>
#include <linux/init.h>
#include <linux/memblock.h>
#include <linux/pfn.h>
#include <linux/rculist.h>
#include <linux/scatterlist.h>
#include <linux/set_memory.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/pswiotlb.h>
#include <linux/pfn.h>
#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/highmem.h>
#include <linux/gfp.h>
#include <linux/scatterlist.h>
#include <linux/mem_encrypt.h>
#include <linux/set_memory.h>
#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#endif
#ifdef CONFIG_DMA_RESTRICTED_POOL
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/slab.h>
#endif

#include <asm/dma.h>

#define CREATE_TRACE_POINTS
#include <trace/events/pswiotlb.h>

#define SLABS_PER_PAGE (1 << (PAGE_SHIFT - P_IO_TLB_SHIFT))

/*
 * Minimum Phytium IO TLB size to bother booting with.  If we can't
 * allocate a contiguous 1MB, we're probably in trouble anyway.
 */
#define P_IO_TLB_MIN_SLABS ((1<<20) >> P_IO_TLB_SHIFT)
#define PSWIOTLB_VERSION "1.0.0"
#define INVALID_PHYS_ADDR (~(phys_addr_t)0)

int pswiotlb_node_num;
bool pswiotlb_mtimer_alive;

/**
 * struct p_io_tlb_slot - Phytium IO TLB slot descriptor
 * @orig_addr:	The original address corresponding to a mapped entry.
 * @alloc_size:	Size of the allocated buffer.
 * @list:	The free list describing the number of free entries available
 *		from each index.
 */
struct p_io_tlb_slot {
	phys_addr_t orig_addr;
	size_t alloc_size;
	unsigned int list;
};

bool __ro_after_init pswiotlb_force_disable;

static struct page *alloc_dma_pages(int nid, gfp_t gfp, size_t bytes);

struct p_io_tlb_mem p_io_tlb_default_mem[MAX_NUMNODES];
static struct timer_list service_timer;

static unsigned long default_npslabs = P_IO_TLB_DEFAULT_SIZE >> P_IO_TLB_SHIFT;
static unsigned long dynamic_inc_thr_npslabs = P_IO_TLB_INC_THR >> P_IO_TLB_SHIFT;
static unsigned long default_npareas;

LIST_HEAD(passthroughlist);
static spinlock_t passthroughlist_lock;
static struct pswiotlb_passthroughlist passthroughlist_entry[1024];
static struct dentry *passthroughlist_debugfs;
static struct dentry *pswiotlb_debugfs;
/**
 * struct p_io_tlb_area - Phytium IO TLB memory area descriptor
 *
 * This is a single area with a single lock.
 *
 * @used:	The number of used Phytium IO TLB block.
 * @index:	The slot index to start searching in this area for next round.
 * @lock:	The lock to protect the above data structures in the map and
 *		unmap calls.
 */
struct p_io_tlb_area {
	unsigned long used;
	unsigned int index;
	spinlock_t lock;
};

static struct pswiotlb_passthroughlist_entry {
	unsigned short vendor;
	unsigned short device;
} ps_passthroughlist[] = {
	{BL_PCI_VENDOR_ID_NVIDIA,	  0xFFFF},
	{BL_PCI_VENDOR_ID_ILUVATAR,	  0xFFFF},
	{BL_PCI_VENDOR_ID_METAX,      0xFFFF},
	{}
};

/*
 * Round up number of slabs to the next power of 2. The last area is going
 * be smaller than the rest if default_npslabs is not power of two.
 * The number of slot in an area should be a multiple of P_IO_TLB_SEGSIZE,
 * otherwise a segment may span two or more areas. It conflicts with free
 * contiguous slots tracking: free slots are treated contiguous no matter
 * whether they cross an area boundary.
 *
 * Return true if default_npslabs is rounded up.
 */
static bool round_up_default_npslabs(void)
{
	if (!default_npareas)
		return false;

	if (default_npslabs < P_IO_TLB_SEGSIZE * default_npareas)
		default_npslabs = P_IO_TLB_SEGSIZE * default_npareas;
	else if (is_power_of_2(default_npslabs))
		return false;
	default_npslabs = roundup_pow_of_two(default_npslabs);
	return true;
}

/**
 * pswiotlb_adjust_nareas() - adjust the number of areas and slots
 * @nareas:	Desired number of areas. Zero is treated as 1.
 *
 * Adjust the default number of areas in a memory pool.
 * The default size of the memory pool may also change to meet minimum area
 * size requirements.
 */
static void pswiotlb_adjust_nareas(unsigned int nareas)
{
	if (!nareas)
		nareas = 1;
	else if (!is_power_of_2(nareas))
		nareas = roundup_pow_of_two(nareas);

	default_npareas = nareas;

	pr_info("area num %d.\n", nareas);
	if (round_up_default_npslabs())
		pr_info("PSWIOTLB bounce buffer size roundup to %luMB",
			(default_npslabs << P_IO_TLB_SHIFT) >> 20);
}

/**
 * limit_nareas() - get the maximum number of areas for a given memory pool size
 * @nareas:	Desired number of areas.
 * @nslots:	Total number of slots in the memory pool.
 *
 * Limit the number of areas to the maximum possible number of areas in
 * a memory pool of the given size.
 *
 * Return: Maximum possible number of areas.
 */
static unsigned int limit_nareas(unsigned int nareas, unsigned long nslots)
{
	if (nslots < nareas * P_IO_TLB_SEGSIZE)
		return nslots / P_IO_TLB_SEGSIZE;
	return nareas;
}

static int __init
setup_p_io_tlb_npages(char *str)
{
	unsigned long nareas;

	if (!strcmp(str, "forceoff")) {
		pswiotlb_force_disable = true;
	} else if (isdigit(*str) && !kstrtoul(str, 0, &default_npslabs)) {
		default_npslabs = ALIGN(default_npslabs, P_IO_TLB_SEGSIZE);
		str = strchr(str, ',');
		if (str++ && isdigit(*str) && !kstrtoul(str, 0, &nareas))
			pswiotlb_adjust_nareas(nareas);
	}
	return 0;
}
early_param("pswiotlb", setup_p_io_tlb_npages);

static int __init
setup_pswiotlb_passthroughlist(char *str)
{
	char tmp_str[5] = {'\0'};
	unsigned long flags;
	int i, j, k;
	int ret;

	for (i = 0, j = 0, k = 0; i < strlen(str) + 1; i++) {
		if (*(str + i) != ',' && *(str + i) != '\0') {
			tmp_str[j++] = *(str + i);
		} else {
			j = 0;

			ret = kstrtou16(tmp_str, 16, &passthroughlist_entry[k].vendor);
			if (ret)
				return ret;

			passthroughlist_entry[k].from_grub = true;

			spin_lock_irqsave(&passthroughlist_lock, flags);
			list_add_rcu(&passthroughlist_entry[k].node, &passthroughlist);
			spin_unlock_irqrestore(&passthroughlist_lock, flags);

			k++;
		}
	}

	return 0;
}
early_param("pswiotlb_passthroughlist", setup_pswiotlb_passthroughlist);

unsigned long pswiotlb_size_or_default(void)
{
	return default_npslabs << P_IO_TLB_SHIFT;
}

void __init pswiotlb_adjust_size(unsigned long size)
{
	if (default_npslabs != P_IO_TLB_DEFAULT_SIZE >> P_IO_TLB_SHIFT)
		return;
	size = ALIGN(size, P_IO_TLB_SIZE);
	default_npslabs = ALIGN(size >> P_IO_TLB_SHIFT, P_IO_TLB_SEGSIZE);
	if (round_up_default_npslabs())
		size = default_npslabs << P_IO_TLB_SHIFT;
	pr_info("PSWIOTLB bounce buffer size adjusted to %luMB", size >> 20);
}

void pswiotlb_print_info(int nid)
{
	struct p_io_tlb_pool *mem = &p_io_tlb_default_mem[nid].defpool;

	if (!mem->nslabs) {
		pr_warn("No local mem of numa node %d\n", nid);
		return;
	}

	pr_info("numa %d mapped [mem %pa-%pa] (%luMB)\n", nid, &mem->start, &mem->end,
	       (mem->nslabs << P_IO_TLB_SHIFT) >> 20);
}

static inline unsigned long io_tlb_offset(unsigned long val)
{
	return val & (P_IO_TLB_SEGSIZE - 1);
}

static inline unsigned long nr_slots(u64 val)
{
	return DIV_ROUND_UP(val, P_IO_TLB_SIZE);
}

static void pswiotlb_record_mem_range(struct p_io_tlb_mem *mem)
{
	unsigned long start_pfn, end_pfn;
	unsigned long min_pfn = (~(phys_addr_t)0 >> PAGE_SHIFT), max_pfn = 0;
	int i, nid;
	unsigned long total_pfn = 0;

	for_each_mem_pfn_range(i, MAX_NUMNODES, &start_pfn, &end_pfn, &nid) {
		pr_info(" node %3d: [mem %#018Lx-%#018Lx]\n", nid,
					(u64)start_pfn << PAGE_SHIFT,
					((u64)end_pfn << PAGE_SHIFT) - 1);
		if (nid == mem->numa_node_id) {
			if (min_pfn > start_pfn)
				min_pfn = start_pfn;
			if (max_pfn < end_pfn)
				max_pfn = end_pfn;
			total_pfn += end_pfn - start_pfn + 1;
		}
	}

	mem->node_min_addr = (u64)min_pfn << PAGE_SHIFT;
	mem->node_max_addr = ((u64)max_pfn << PAGE_SHIFT) - 1;
	mem->node_total_mem = (u64)total_pfn << PAGE_SHIFT;
}

static void pswiotlb_init_io_tlb_pool(struct p_io_tlb_pool *mem, int nid, phys_addr_t start,
				    unsigned long npslabs, bool late_alloc, unsigned int nareas)
{
	void *vaddr = phys_to_virt(start);
	unsigned long bytes = npslabs << P_IO_TLB_SHIFT, i;

	mem->nslabs = npslabs;
	mem->start = start;
	mem->end = mem->start + bytes;
	mem->late_alloc = late_alloc;
	mem->numa_node_id = nid;
	mem->nareas = nareas;
	mem->area_nslabs = npslabs / mem->nareas;
	mem->free_th = PSWIOTLB_FREE_THRESHOLD;

	for (i = 0; i < mem->nareas; i++) {
		spin_lock_init(&mem->areas[i].lock);
		mem->areas[i].index = 0;
		mem->areas[i].used = 0;
	}

	for (i = 0; i < mem->nslabs; i++) {
		mem->slots[i].list = P_IO_TLB_SEGSIZE - io_tlb_offset(i);
		mem->slots[i].orig_addr = INVALID_PHYS_ADDR;
		mem->slots[i].alloc_size = 0;
	}
	memset(vaddr, 0, bytes);
	mem->vaddr = vaddr;
}

/**
 * add_mem_pool() - add a memory pool to the allocator
 * @mem:	Phytium software IO TLB allocator.
 * @pool:	Memory pool to be added.
 */
static void add_mem_pool(struct p_io_tlb_mem *mem, struct p_io_tlb_pool *pool)
{
	unsigned long flags;

	spin_lock_irqsave(&mem->lock, flags);
	if (mem->capacity != mem->whole_size) {
		mem->pool_addr[mem->whole_size] = mem->pool_addr[mem->capacity];
		mem->pool_addr[mem->capacity] = pool;
	} else {
		mem->pool_addr[mem->capacity] = pool;
	}
	/* prevent any other writes prior to this time */
	smp_wmb();
	mem->capacity++;
	mem->whole_size++;
	mem->nslabs += pool->nslabs;
	spin_unlock_irqrestore(&mem->lock, flags);
}

static void __init *pswiotlb_memblock_alloc(unsigned long npslabs,
		int nid, unsigned int flags,
		int (*remap)(void *tlb, unsigned long npslabs))
{
	size_t bytes = PAGE_ALIGN(npslabs << P_IO_TLB_SHIFT);
	void *tlb;

	tlb = memblock_alloc_node(bytes, PAGE_SIZE, nid);

	if (!tlb) {
		pr_warn("%s: Failed to allocate %zu bytes tlb structure\n",
			__func__, bytes);
		return NULL;
	}

	if (remap && remap(tlb, npslabs) < 0) {
		memblock_free(tlb, PAGE_ALIGN(bytes));
		pr_warn("%s: Failed to remap %zu bytes\n", __func__, bytes);
		return NULL;
	}

	return tlb;
}

static void check_if_pswiotlb_in_local_node(struct p_io_tlb_mem *mem,
			struct p_io_tlb_pool *pool)
{
	if ((pool->start < mem->node_min_addr) ||
			pool->end > mem->node_max_addr) {
		mem->nslabs = 0;
		pool->nslabs = 0;
	}
}

/*
 * Statically reserve bounce buffer space and initialize bounce buffer data
 * structures for the Phytium software IO TLB used to implement the DMA API.
 */
void __init pswiotlb_init_remap(bool addressing_limit, int nid, unsigned int flags,
		int (*remap)(void *tlb, unsigned long npslabs))
{
	struct p_io_tlb_pool *mem = &p_io_tlb_default_mem[nid].defpool;
	unsigned long npslabs;
	unsigned int nareas;
	size_t alloc_size;
	void *tlb;

	if (!addressing_limit)
		return;
	if (pswiotlb_force_disable)
		return;

	if (!remap)
		p_io_tlb_default_mem[nid].can_grow = true;
	p_io_tlb_default_mem[nid].phys_limit = virt_to_phys(high_memory - 1);

	if (!default_npareas)
		pswiotlb_adjust_nareas(num_possible_cpus());

	npslabs = default_npslabs;
	nareas = limit_nareas(default_npareas, npslabs);
	while ((tlb = pswiotlb_memblock_alloc(npslabs, nid, flags, remap)) == NULL) {
		if (npslabs <= P_IO_TLB_MIN_SLABS)
			return;
		npslabs = ALIGN(npslabs >> 1, P_IO_TLB_SEGSIZE);
		nareas = limit_nareas(nareas, npslabs);
	}

	if (default_npslabs != npslabs) {
		pr_info("PSWIOTLB bounce buffer size adjusted %lu -> %lu slabs",
			default_npslabs, npslabs);
		default_npslabs = npslabs;
	}

	alloc_size = PAGE_ALIGN(array_size(sizeof(*mem->slots), npslabs));
	mem->slots = memblock_alloc(alloc_size, PAGE_SIZE);
	if (!mem->slots) {
		pr_warn("%s: Failed to allocate %zu bytes align=0x%lx\n",
			__func__, alloc_size, PAGE_SIZE);
		return;
	}

	mem->areas = memblock_alloc(array_size(sizeof(struct p_io_tlb_area),
		nareas), SMP_CACHE_BYTES);
	if (!mem->areas) {
		pr_warn("%s: Failed to allocate mem->areas.\n", __func__);
		return;
	}

	pswiotlb_init_io_tlb_pool(mem, nid, __pa(tlb), npslabs, false, nareas);
	add_mem_pool(&p_io_tlb_default_mem[nid], mem);
	check_if_pswiotlb_in_local_node(&p_io_tlb_default_mem[nid], mem);

	if (flags & PSWIOTLB_VERBOSE)
		pswiotlb_print_info(nid);
}
/**
 * pswiotlb_free_tlb() - free a dynamically allocated Phytium IO TLB buffer
 * @vaddr:	Virtual address of the buffer.
 * @bytes:	Size of the buffer.
 */
static void pswiotlb_free_tlb(void *vaddr, size_t bytes)
{
	if (IS_ENABLED(CONFIG_DMA_COHERENT_POOL) &&
	    dma_free_from_pool(NULL, vaddr, bytes))
		return;

	/* Intentional leak if pages cannot be encrypted again. */
	if (!set_memory_encrypted((unsigned long)vaddr, PFN_UP(bytes)))
		__free_pages(virt_to_page(vaddr), get_order(bytes));
}
/**
 * pswiotlb_alloc_tlb() - allocate a dynamic Phytium IO TLB buffer
 * @dev:	Device for which a memory pool is allocated.
 * @bytes:	Size of the buffer.
 * @phys_limit:	Maximum allowed physical address of the buffer.
 * @gfp:	GFP flags for the allocation.
 *
 * Return: Allocated pages, or %NULL on allocation failure.
 */
static struct page *pswiotlb_alloc_tlb(struct device *dev, int nid, size_t bytes,
		u64 phys_limit, gfp_t gfp)
{
	struct page *page;

	/*
	 * Allocate from the atomic pools if memory is encrypted and
	 * the allocation is atomic, because decrypting may block.
	 */
	if (!gfpflags_allow_blocking(gfp) && dev && force_dma_unencrypted(dev)) {
		void *vaddr;

		if (!IS_ENABLED(CONFIG_DMA_COHERENT_POOL))
			return NULL;

		return dma_alloc_from_pool(dev, bytes, &vaddr, gfp,
					   pswiotlb_dma_coherent_ok);
	}

	gfp &= ~GFP_ZONEMASK;
	if (phys_limit <= DMA_BIT_MASK(zone_dma_bits))
		gfp |= __GFP_DMA;
	else if (phys_limit <= DMA_BIT_MASK(32))
		gfp |= __GFP_DMA32;

	while ((page = alloc_dma_pages(nid, gfp, bytes)) &&
	       page_to_phys(page) + bytes - 1 > phys_limit) {
		/* allocated, but too high */
		__free_pages(page, get_order(bytes));

		if (IS_ENABLED(CONFIG_ZONE_DMA32) &&
		    phys_limit < DMA_BIT_MASK(64) &&
		    !(gfp & (__GFP_DMA32 | __GFP_DMA)))
			gfp |= __GFP_DMA32;
		else if (IS_ENABLED(CONFIG_ZONE_DMA) &&
			 !(gfp & __GFP_DMA))
			gfp = (gfp & ~__GFP_DMA32) | __GFP_DMA;
		else
			return NULL;
	}

	return page;
}
/**
 * pswiotlb_alloc_pool() - allocate a new Phytium IO TLB memory pool
 * @dev:	Device for which a memory pool is allocated.
 * @minslabs:	Minimum number of slabs.
 * @nslabs:	Desired (maximum) number of slabs.
 * @nareas:	Number of areas.
 * @phys_limit:	Maximum DMA buffer physical address.
 * @gfp:	GFP flags for the allocations.
 *
 * Allocate and initialize a new Phytium IO TLB memory pool. The actual number of
 * slabs may be reduced if allocation of @nslabs fails. If even
 * @minslabs cannot be allocated, this function fails.
 *
 * Return: New memory pool, or %NULL on allocation failure.
 */
static struct p_io_tlb_pool *pswiotlb_alloc_pool(struct device *dev,
		int nid, unsigned long minslabs, unsigned long nslabs,
		unsigned int nareas, u64 phys_limit, bool transient, gfp_t gfp)
{
	struct p_io_tlb_pool *pool;
	unsigned int slot_order;
	struct page *tlb;
	size_t pool_size;
	size_t tlb_size;

	if (nslabs > SLABS_PER_PAGE << MAX_ORDER) {
		nslabs = SLABS_PER_PAGE << MAX_ORDER;
		nareas = limit_nareas(nareas, nslabs);
	}

	pool_size = sizeof(*pool) + array_size(sizeof(*pool->areas), nareas);
	pool = kzalloc(pool_size, gfp);
	if (!pool)
		goto error;
	pool->areas = (void *)pool + sizeof(*pool);

	if (!transient) {
		nslabs = ALIGN(nslabs >> 1, P_IO_TLB_SEGSIZE);
		nareas = limit_nareas(nareas, nslabs);
	}
	tlb_size = nslabs << P_IO_TLB_SHIFT;
	while (!(tlb = pswiotlb_alloc_tlb(dev, nid, tlb_size, phys_limit, gfp))) {
		if (nslabs <= minslabs)
			goto error_tlb;
		nslabs = ALIGN(nslabs >> 1, P_IO_TLB_SEGSIZE);
		nareas = limit_nareas(nareas, nslabs);
		tlb_size = nslabs << P_IO_TLB_SHIFT;
	}
	if (page_to_nid(tlb) != nid)
		goto error_slots;

	slot_order = get_order(array_size(sizeof(*pool->slots), nslabs));
	pool->slots = (struct p_io_tlb_slot *)
		__get_free_pages(gfp, slot_order);
	if (!pool->slots)
		goto error_slots;

	pswiotlb_init_io_tlb_pool(pool, nid, page_to_phys(tlb), nslabs, true, nareas);
	return pool;

error_slots:
	pswiotlb_free_tlb(page_address(tlb), tlb_size);
error_tlb:
	kfree(pool);
error:
	return NULL;
}
static void pswiotlb_prepare_release_pool(struct p_io_tlb_mem *mem,
			struct p_io_tlb_pool *pool, int pool_idx)
{
	int capacity;
	unsigned long flags;

	spin_lock_irqsave(&mem->lock, flags);
	capacity = mem->capacity;
	mem->pool_addr[pool_idx] = mem->pool_addr[capacity - 1];
	mem->pool_addr[capacity - 1] = pool;
	mem->capacity--;
	mem->nslabs -= pool->nslabs;
	spin_unlock_irqrestore(&mem->lock, flags);
}
static void pswiotlb_release_pool(struct p_io_tlb_mem *mem,
			struct p_io_tlb_pool *pool, int pool_idx)
{
	unsigned int bytes = pool->nslabs * P_IO_TLB_SIZE;
	unsigned int order = get_order(bytes);
	struct page *page_start;
	size_t slots_size = array_size(sizeof(*pool->slots), pool->nslabs);
	int pool_idx1;
	unsigned long flags;

	spin_lock_irqsave(&mem->lock, flags);
	pool_idx1 = mem->whole_size - 1;
	mem->pool_addr[pool_idx] = mem->pool_addr[pool_idx1];
	mem->whole_size--;
	spin_unlock_irqrestore(&mem->lock, flags);

	bitmap_free(pool->busy_record);
	free_pages((unsigned long)pool->slots, get_order(slots_size));
	page_start = pfn_to_page(PFN_DOWN(pool->start));
	__free_pages(page_start, order);
	kfree(pool);
}
static void pswiotlb_monitor_service(struct timer_list *timer)
{
	int i, j, pool_idx;
	struct p_io_tlb_pool *pool;
	struct p_io_tlb_mem *mem;
	int capacity, whole_size;

	for (i = 0; i < pswiotlb_node_num; i++) {
		mem = &p_io_tlb_default_mem[i];
		whole_size = mem->whole_size;
		capacity = mem->capacity;
		rcu_read_lock();
		for (pool_idx = 1; pool_idx < whole_size; pool_idx++) {
			pool = mem->pool_addr[pool_idx];
			for (j = 0; j < DIV_ROUND_UP(pool->nareas, BITS_PER_LONG); j++) {
				if (*(pool->busy_record + j) != 0) {
					pool->busy_flag = true;
					break;
				}
				pool->busy_flag = false;
			}
			if (!pool->busy_flag)
				pool->free_cnt++;
			else
				pool->free_cnt = 0;
			if (pool->free_cnt >= pool->free_th && pool_idx < capacity) {
				pswiotlb_prepare_release_pool(mem, pool, pool_idx);
				capacity--;
			}
			if (pool->free_cnt >= 2 * pool->free_th && !pool->busy_flag) {
				pswiotlb_release_pool(mem, pool, pool_idx);
				whole_size--;
			}
		}
		rcu_read_unlock();
	}

	mod_timer(timer, jiffies + 2 * HZ);
}
static struct p_io_tlb_pool *pswiotlb_formal_alloc(struct device *dev,
		struct p_io_tlb_mem *mem)
{
	struct p_io_tlb_pool *pool;

	pool = pswiotlb_alloc_pool(dev, mem->numa_node_id,
				P_IO_TLB_MIN_SLABS, dynamic_inc_thr_npslabs,
				dynamic_inc_thr_npslabs, mem->phys_limit,
				0, GFP_ATOMIC | GFP_NOWAIT | __GFP_NOWARN);
	if (!pool) {
		pr_warn_once("Failed to allocate new formal pool");
		return NULL;
	}

	pool->busy_record = bitmap_zalloc(pool->nareas, GFP_ATOMIC);
	if (!pool->busy_record) {
		pr_warn_ratelimited("%s: Failed to allocate pool busy record.\n", __func__);
		return NULL;
	}

	add_mem_pool(mem, pool);

	return pool;
}

/**
 * pswiotlb_dyn_free() - RCU callback to free a memory pool
 * @rcu:	RCU head in the corresponding struct p_io_tlb_pool.
 */
static void pswiotlb_dyn_free(struct rcu_head *rcu)
{
	struct p_io_tlb_pool *pool = container_of(rcu, struct p_io_tlb_pool, rcu);
	size_t slots_size = array_size(sizeof(*pool->slots), pool->nslabs);
	size_t tlb_size = pool->end - pool->start;

	free_pages((unsigned long)pool->slots, get_order(slots_size));
	pswiotlb_free_tlb(pool->vaddr, tlb_size);
	kfree(pool);
}
static void pswiotlb_init_tlb_mem_dynamic(struct p_io_tlb_mem *mem, int nid)
{
	spin_lock_init(&mem->lock);
	mem->capacity = 0;
	mem->whole_size = 0;
	mem->numa_node_id = nid;
}

bool pswiotlb_is_dev_in_passthroughlist(struct pci_dev *dev)
{
	struct pswiotlb_passthroughlist *bl_entry;

	rcu_read_lock();
	list_for_each_entry_rcu(bl_entry, &passthroughlist, node) {
		if (bl_entry->vendor == dev->vendor) {
			rcu_read_unlock();
			goto out;
		}
	}
	rcu_read_unlock();

	return true;
out:
	return false;
}

static void pswiotlb_show_passthroughlist(void)
{
	struct pswiotlb_passthroughlist *bl_entry;

	pr_info("The following vendors devices belong to are incompatible with pswiotlb temporarily:\n");
	rcu_read_lock();
	list_for_each_entry_rcu(bl_entry, &passthroughlist, node)
		printk(KERN_INFO "0x%06x", bl_entry->vendor);
	rcu_read_unlock();
}

static void __init pswiotlb_passthroughlist_init(void)
{
	int dev_num = 0;
	int i;
	size_t alloc_size;
	struct pswiotlb_passthroughlist *passthroughlist_array;

	spin_lock_init(&passthroughlist_lock);

	for (i = 0; ps_passthroughlist[i].vendor != 0; i++)
		dev_num++;

	alloc_size = PAGE_ALIGN(array_size(sizeof(struct pswiotlb_passthroughlist), dev_num));
	passthroughlist_array = memblock_alloc(alloc_size, PAGE_SIZE);
	if (!passthroughlist_array) {
		pr_warn("%s: Failed to allocate memory for passthroughlist\n",
					__func__);
		return;
	}

	for (i = 0; i < dev_num; i++) {
		passthroughlist_array[i].vendor = ps_passthroughlist[i].vendor;
		passthroughlist_array[i].device = ps_passthroughlist[i].device;

		spin_lock(&passthroughlist_lock);
		list_add_rcu(&passthroughlist_array[i].node, &passthroughlist);
		spin_unlock(&passthroughlist_lock);
	}

	pswiotlb_show_passthroughlist();
}

/*
 * Statically reserve bounce buffer space and initialize bounce buffer data
 * structures for the software IO TLB used to implement the DMA API.
 */
void __init pswiotlb_init(bool addressing_limit, unsigned int flags)
{
	int i;
	int nid;
	unsigned long start_pfn, end_pfn;

	/* Get number of numa node*/
	for_each_mem_pfn_range(i, MAX_NUMNODES, &start_pfn, &end_pfn, &nid);
	pswiotlb_node_num = nid + 1;
	pr_info("Total number of numa nodes is %d\n", pswiotlb_node_num);
	for (i = 0; i < pswiotlb_node_num; i++) {
		struct p_io_tlb_mem *mem = &p_io_tlb_default_mem[i];

		pswiotlb_init_tlb_mem_dynamic(mem, i);
		pswiotlb_record_mem_range(mem);
		pr_info(" node %3d memory range: [%#018Lx-%#018Lx], total memory: %ldMB\n",
					i, mem->node_min_addr, mem->node_max_addr,
					mem->node_total_mem >> 20);
	}
	/* Get P TLB memory according to numa node id */
	for (i = 0; i < pswiotlb_node_num; i++)
		pswiotlb_init_remap(addressing_limit, i, flags, NULL);

	pswiotlb_passthroughlist_init();
}

/**
 * alloc_dma_pages() - allocate pages to be used for DMA
 * @gfp:	GFP flags for the allocation.
 * @bytes:	Size of the buffer.
 *
 * Allocate pages from the buddy allocator. If successful, make the allocated
 * pages decrypted that they can be used for DMA.
 *
 * Return: Decrypted pages, or %NULL on failure.
 */
static struct page *alloc_dma_pages(int nid, gfp_t gfp, size_t bytes)
{
	unsigned int order = get_order(bytes);
	struct page *page;
	void *vaddr;

	page = alloc_pages_node(nid, gfp, order);
	if (!page)
		return NULL;

	vaddr = page_address(page);
	if (set_memory_decrypted((unsigned long)vaddr, PFN_UP(bytes)))
		goto error;
	return page;

error:
	__free_pages(page, order);
	return NULL;
}

/**
 * pswiotlb_find_pool() - find the Phytium IO TLB pool for a physical address
 * @dev:        Device which has mapped the DMA buffer.
 * @paddr:      Physical address within the DMA buffer.
 *
 * Find the Phytium IO TLB memory pool descriptor which contains the given physical
 * address, if any.
 *
 * Return: Memory pool which contains @paddr, or %NULL if none.
 */
struct p_io_tlb_pool *pswiotlb_find_pool(struct device *dev, int nid, phys_addr_t paddr)
{
	struct p_io_tlb_mem *mem = &dev->dma_p_io_tlb_mem[nid];
	struct p_io_tlb_pool *pool;
	int i;
	int whole_size;

	/* prevent any other reads prior to this time */
	smp_rmb();
	whole_size = mem->whole_size;
	rcu_read_lock();
	for (i = 0; i < whole_size; i++) {
		pool = mem->pool_addr[i];
		if (paddr >= pool->start && paddr < pool->end)
			goto out;
	}

	pool = NULL;
out:
	rcu_read_unlock();
	return pool;
}

/**
 * pswiotlb_dev_init() - initialize pswiotlb fields in &struct device
 * @dev:	Device to be initialized.
 */
void pswiotlb_dev_init(struct device *dev)
{
	dev->dma_uses_p_io_tlb = false;
}

void pswiotlb_store_local_node(struct pci_dev *dev, struct pci_bus *bus)
{
	int nid;
	struct p_io_tlb_pool *defpool;
	struct p_io_tlb_mem *mem;

	dev->dev.local_node = pcibus_to_node(bus);
	if (dev->dev.local_node == NUMA_NO_NODE)
		return;

	/* register pswiotlb resources */
	dev->dev.dma_p_io_tlb_mem = p_io_tlb_default_mem;
	nid = dev->dev.local_node;
	defpool = &dev->dev.dma_p_io_tlb_mem[nid].defpool;
	mem = &dev->dev.dma_p_io_tlb_mem[nid];
	pci_info(dev, "numa node: %d, pswiotlb defpool range: [%#018Lx-%#018Lx]\n"
				"local node range: [%#018Lx-%#018Lx]\n", nid,
		defpool->start, defpool->end, mem->node_min_addr, mem->node_max_addr);
}
/*
 * Return the offset into a pswiotlb slot required to keep the device happy.
 */
static unsigned int pswiotlb_align_offset(struct device *dev, u64 addr)
{
	if (dma_get_min_align_mask(dev))
		return addr & dma_get_min_align_mask(dev) & (P_IO_TLB_SIZE - 1);
	else
		return addr & (P_IO_TLB_SIZE - 1);
}
/*
 * Bounce: copy the pswiotlb buffer from or back to the original dma location
 */
static void pswiotlb_bounce(struct device *dev, int nid, phys_addr_t tlb_addr, size_t size,
			   enum dma_data_direction dir, struct p_io_tlb_pool *mem)
{
	int index = (tlb_addr - mem->start) >> P_IO_TLB_SHIFT;
	phys_addr_t orig_addr = mem->slots[index].orig_addr;
	size_t alloc_size = mem->slots[index].alloc_size;
	unsigned long pfn = PFN_DOWN(orig_addr);
	unsigned char *vaddr = mem->vaddr + tlb_addr - mem->start;
	unsigned int tlb_offset, orig_addr_offset;

	if (orig_addr == INVALID_PHYS_ADDR)
		return;

	tlb_offset = tlb_addr & (P_IO_TLB_SIZE - 1);
	orig_addr_offset = pswiotlb_align_offset(dev, orig_addr);
	if (tlb_offset < orig_addr_offset) {
		dev_WARN_ONCE(dev, 1,
			"Access before mapping start detected. orig offset %u, requested offset %u.\n",
			orig_addr_offset, tlb_offset);
		return;
	}

	tlb_offset -= orig_addr_offset;
	if (tlb_offset > alloc_size) {
		dev_WARN_ONCE(dev, 1,
			"Buffer overflow detected. Allocation size: %zu. Mapping size: %zu+%u.\n",
			alloc_size, size, tlb_offset);
		return;
	}

	orig_addr += tlb_offset;
	alloc_size -= tlb_offset;

	if (size > alloc_size) {
		dev_WARN_ONCE(dev, 1,
			"Buffer overflow detected. Allocation size: %zu. Mapping size: %zu.\n",
			alloc_size, size);
		size = alloc_size;
	}

	if (PageHighMem(pfn_to_page(pfn))) {
		unsigned int offset = orig_addr & ~PAGE_MASK;
		struct page *page;
		unsigned int sz = 0;
		unsigned long flags;

		dev_info(dev, "%s line=%d !!!!!!HighMem!!!!!! dir: %d, tlb_addr: %#018Lx, size: %#lx\n",
			__func__, __LINE__, dir, tlb_addr, size);

		while (size) {
			sz = min_t(size_t, PAGE_SIZE - offset, size);

			local_irq_save(flags);
			page = pfn_to_page(pfn);
			if (dir == DMA_TO_DEVICE)
				memcpy_from_page(vaddr, page, offset, sz);
			else
				memcpy_to_page(page, offset, vaddr, sz);
			local_irq_restore(flags);

			size -= sz;
			pfn++;
			vaddr += sz;
			offset = 0;
		}
	} else if (dir == DMA_TO_DEVICE) {
		memcpy(vaddr, phys_to_virt(orig_addr), size);
	} else {
		memcpy(phys_to_virt(orig_addr), vaddr, size);
	}
}
static inline phys_addr_t slot_addr(phys_addr_t start, phys_addr_t idx)
{
	return start + (idx << P_IO_TLB_SHIFT);
}
/*
 * Carefully handle integer overflow which can occur when boundary_mask == ~0UL.
 */
static inline unsigned long get_max_slots(unsigned long boundary_mask)
{
	return (boundary_mask >> P_IO_TLB_SHIFT) + 1;
}

static unsigned int wrap_area_index(struct p_io_tlb_pool *mem, unsigned int index)
{
	if (index >= mem->area_nslabs)
		return 0;
	return index;
}

/**
 * pswiotlb_area_find_slots() - search for slots in one Phytium IO TLB memory area
 * @dev:	Device which maps the buffer.
 * @pool:	Memory pool to be searched.
 * @area_index:	Index of the Phytium IO TLB memory area to be searched.
 * @orig_addr:	Original (non-bounced) Phytium IO buffer address.
 * @alloc_size: Total requested size of the bounce buffer,
 *		including initial alignment padding.
 * @alloc_align_mask:	Required alignment of the allocated buffer.
 *
 * Find a suitable sequence of Phytium IO TLB entries for the request and allocate
 * a buffer from the given Phytium IO TLB memory area.
 * This function takes care of locking.
 *
 * Return: Index of the first allocated slot, or -1 on error.
 */
static int pswiotlb_area_find_slots(struct device *dev, int nid, struct p_io_tlb_pool *pool,
		int area_index, phys_addr_t orig_addr, size_t alloc_size,
		unsigned int alloc_align_mask)
{
	struct p_io_tlb_area *area = pool->areas + area_index;
	unsigned long boundary_mask = dma_get_seg_boundary(dev);
	dma_addr_t tbl_dma_addr =
		phys_to_dma_unencrypted(dev, pool->start) & boundary_mask;
	unsigned long max_slots = get_max_slots(boundary_mask);
	unsigned int iotlb_align_mask =
		dma_get_min_align_mask(dev) | alloc_align_mask;
	unsigned int nslots = nr_slots(alloc_size), stride;
	unsigned int offset = pswiotlb_align_offset(dev, orig_addr);
	unsigned int index, slots_checked, count = 0, i;
	unsigned long flags;
	unsigned int slot_base;
	unsigned int slot_index;

	WARN_ON(!nslots);
	WARN_ON(area_index >= pool->nareas);

	/*
	 * For allocations of PAGE_SIZE or larger only look for page aligned
	 * allocations.
	 */
	if (alloc_size >= PAGE_SIZE)
		iotlb_align_mask |= ~PAGE_MASK;
	iotlb_align_mask &= ~(P_IO_TLB_SIZE - 1);

	/*
	 * For mappings with an alignment requirement don't bother looping to
	 * unaligned slots once we found an aligned one.
	 */
	stride = (iotlb_align_mask >> P_IO_TLB_SHIFT) + 1;

	if (spin_trylock_irqsave(&area->lock, flags)) {
		if (unlikely(nslots > pool->area_nslabs - area->used))
			goto not_found;

		slot_base = area_index * pool->area_nslabs;
		index = area->index;

		for (slots_checked = 0; slots_checked < pool->area_nslabs;) {
			slot_index = slot_base + index;

			if (orig_addr &&
				 (slot_addr(tbl_dma_addr, slot_index) &
				 iotlb_align_mask) != (orig_addr & iotlb_align_mask)) {
				index = wrap_area_index(pool, index + 1);
				slots_checked++;
				continue;
			}

			if (!iommu_is_span_boundary(slot_index, nslots,
						    nr_slots(tbl_dma_addr),
						    max_slots)) {
				if (pool->slots[slot_index].list >= nslots)
					goto found;
			}
			index = wrap_area_index(pool, index + stride);
			slots_checked += stride;
		}
	} else {
		return -1;
	}

not_found:
	spin_unlock_irqrestore(&area->lock, flags);
	return -1;

found:
	/*
	 * If we find a slot that indicates we have 'nslots' number of
	 * contiguous buffers, we allocate the buffers from that slot onwards
	 * and set the list of free entries to '0' indicating unavailable.
	 */
	for (i = slot_index; i < slot_index + nslots; i++) {
		pool->slots[i].list = 0;
		pool->slots[i].alloc_size = alloc_size - (offset +
				((i - slot_index) << P_IO_TLB_SHIFT));
	}
	for (i = slot_index - 1;
	     io_tlb_offset(i) != P_IO_TLB_SEGSIZE - 1 &&
	     pool->slots[i].list; i--)
		pool->slots[i].list = ++count;

	/*
	 * Update the indices to avoid searching in the next round.
	 */
	area->index = wrap_area_index(pool, index + nslots);
	area->used += nslots;
	spin_unlock_irqrestore(&area->lock, flags);

	return slot_index;
}

/**
 * pswiotlb_pool_find_slots() - search for slots in one memory pool
 * @dev:	Device which maps the buffer.
 * @pool:	Memory pool to be searched.
 * @orig_addr:	Original (non-bounced)Phytium  IO buffer address.
 * @alloc_size: Total requested size of the bounce buffer,
 *		including initial alignment padding.
 * @alloc_align_mask:	Required alignment of the allocated buffer.
 *
 * Search through one memory pool to find a sequence of slots that match the
 * allocation constraints.
 *
 * Return: Index of the first allocated slot, or -1 on error.
 */
static int pswiotlb_pool_find_slots(struct device *dev, int nid, struct p_io_tlb_pool *pool,
		phys_addr_t orig_addr, size_t alloc_size,
		unsigned int alloc_align_mask)
{
	int start = raw_smp_processor_id() & (pool->nareas - 1);
	int i = start, index;

	do {
		index = pswiotlb_area_find_slots(dev, nid, pool, i, orig_addr,
						alloc_size, alloc_align_mask);
		if (index >= 0) {
			if ((pool != &p_io_tlb_default_mem[nid].defpool)
						&& !pool->transient
						&& !test_bit(i, pool->busy_record))
				set_bit(i, pool->busy_record);
			return index;
		}
		if (++i >= pool->nareas)
			i = 0;
	} while (i != start);

	return -1;
}

/**
 * pswiotlb_find_slots() - search for slots in the whole pswiotlb
 * @dev:	Device which maps the buffer.
 * @orig_addr:	Original (non-bounced) Phytium IO buffer address.
 * @alloc_size: Total requested size of the bounce buffer,
 *		including initial alignment padding.
 * @alloc_align_mask:	Required alignment of the allocated buffer.
 * @retpool:	Used memory pool, updated on return.
 *
 * Search through the whole Phytium software IO TLB to find a sequence of slots that
 * match the allocation constraints.
 *
 * Return: Index of the first allocated slot, or -1 on error.
 */
static int pswiotlb_find_slots(struct device *dev, int nid, phys_addr_t orig_addr,
		size_t alloc_size, unsigned int alloc_align_mask,
		struct p_io_tlb_pool **retpool)
{
	struct p_io_tlb_mem *mem = &dev->dma_p_io_tlb_mem[nid];
	struct p_io_tlb_pool *pool;
	int index;
	int try_pool_idx;
	int i;
	int cpuid;
	int current_ratio;
	unsigned long pswiotlb_mem;
	unsigned long nslabs_per_pool = dynamic_inc_thr_npslabs;

	cpuid = raw_smp_processor_id();

	rcu_read_lock();
#ifndef CONFIG_ARM64_4K_PAGES
	for (i = 0; i < 15; i++) {
		if (i == 0) {
			pool = mem->pool_addr[0];
			index = pswiotlb_pool_find_slots(dev, nid, pool, orig_addr,
						alloc_size, alloc_align_mask);
		} else if (i == 1 && mem->capacity > (cpuid + 1)) {
			pool = mem->pool_addr[cpuid + 1];
			index = pswiotlb_pool_find_slots(dev, nid, pool, orig_addr,
						alloc_size, alloc_align_mask);
		} else {
			try_pool_idx = get_random_u32() % mem->capacity;
			pool = mem->pool_addr[try_pool_idx];
			index = pswiotlb_pool_find_slots(dev, nid, pool, orig_addr,
							alloc_size, alloc_align_mask);
		}

		if (index >= 0) {
			rcu_read_unlock();
			goto found;
		}
	}
#else
	for (i = 0; i < 15; i++) {
		try_pool_idx = get_random_u32() % mem->capacity;
		pool = mem->pool_addr[try_pool_idx];
		index = pswiotlb_pool_find_slots(dev, nid, pool, orig_addr,
						alloc_size, alloc_align_mask);

		if (index >= 0) {
			rcu_read_unlock();
			goto found;
		}
	}
#endif
	rcu_read_unlock();
	if (nslabs_per_pool > SLABS_PER_PAGE << MAX_ORDER)
		nslabs_per_pool = SLABS_PER_PAGE << MAX_ORDER;

	nslabs_per_pool = ALIGN(nslabs_per_pool >> 1, P_IO_TLB_SEGSIZE);
	pswiotlb_mem = P_IO_TLB_DEFAULT_SIZE +
		(nslabs_per_pool << P_IO_TLB_SHIFT) * (mem->whole_size - 1);
	current_ratio = (pswiotlb_mem * 100 + mem->node_total_mem / 2) / mem->node_total_mem;
	if (current_ratio >= P_IO_TLB_EXT_WATERMARK) {
		dev_warn_once(dev, "Total pswiotlb (%ld MB) exceeds the watermark (%d%%)\n"
					"of memory (%ld MB) in node %d, pswiotlb expansion is prohibited.\n",
					pswiotlb_mem >> 20, P_IO_TLB_EXT_WATERMARK,
					mem->node_total_mem >> 20, nid);
		return -1;
	}

	if (!mem->can_grow)
		return -1;

	pool = pswiotlb_formal_alloc(dev, mem);
	if (!pool)
		return -1;

	/* retry */
	rcu_read_lock();
	index = pswiotlb_pool_find_slots(dev, nid, pool, orig_addr,
					alloc_size, alloc_align_mask);
	rcu_read_unlock();

	if (index < 0) {
		pswiotlb_dyn_free(&pool->rcu);
		return -1;
	}

found:
	WRITE_ONCE(dev->dma_uses_p_io_tlb, true);

	/*
	 * The general barrier orders reads and writes against a presumed store
	 * of the PSWIOTLB buffer address by a device driver (to a driver private
	 * data structure). It serves two purposes.
	 *
	 * First, the store to dev->dma_uses_p_io_tlb must be ordered before the
	 * presumed store. This guarantees that the returned buffer address
	 * cannot be passed to another CPU before updating dev->dma_uses_p_io_tlb.
	 *
	 * Second, the load from mem->pools must be ordered before the same
	 * presumed store. This guarantees that the returned buffer address
	 * cannot be observed by another CPU before an update of the RCU list
	 * that was made by pswiotlb_dyn_alloc() on a third CPU (cf. multicopy
	 * atomicity).
	 *
	 * See also the comment in is_pswiotlb_buffer().
	 */
	smp_mb();

	*retpool = pool;
	return index;
}
#ifdef CONFIG_DEBUG_FS

/**
 * mem_used() - get number of used slots in an allocator
 * @mem:	Phytium software IO TLB allocator.
 *
 * The result is accurate in this version of the function, because an atomic
 * counter is available if CONFIG_DEBUG_FS is set.
 *
 * Return: Number of used slots.
 */
static unsigned long mem_used(struct p_io_tlb_mem *mem)
{
	return atomic_long_read(&mem->total_used);
}

#else /* !CONFIG_DEBUG_FS */

/**
 * mem_pool_used() - get number of used slots in a memory pool
 * @pool:	Phytium software IO TLB memory pool.
 *
 * The result is not accurate, see mem_used().
 *
 * Return: Approximate number of used slots.
 */
static unsigned long mem_pool_used(struct p_io_tlb_pool *pool)
{
	int i;
	unsigned long used = 0;

	for (i = 0; i < pool->nareas; i++)
		used += pool->areas[i].used;
	return used;
}

/**
 * mem_used() - get number of used slots in an allocator
 * @mem:	Phytium software IO TLB allocator.
 *
 * The result is not accurate, because there is no locking of individual
 * areas.
 *
 * Return: Approximate number of used slots.
 */
static unsigned long mem_used(struct p_io_tlb_mem *mem)
{
	struct p_io_tlb_pool *pool;
	unsigned long used = 0;

	rcu_read_lock();
	list_for_each_entry_rcu(pool, &mem->pools, node)
		used += mem_pool_used(pool);
	rcu_read_unlock();

	return used;
}

#endif /* CONFIG_DEBUG_FS */

phys_addr_t pswiotlb_tbl_map_single(struct device *dev, int nid, phys_addr_t orig_addr,
		size_t mapping_size, size_t alloc_size,
		unsigned int alloc_align_mask, enum dma_data_direction dir,
		unsigned long attrs)
{
	struct p_io_tlb_mem *mem = &dev->dma_p_io_tlb_mem[nid];
	unsigned int offset = pswiotlb_align_offset(dev, orig_addr);
	struct p_io_tlb_pool *pool;
	unsigned int i;
	unsigned long index;
	phys_addr_t tlb_addr;
	struct page *page;

	if (alloc_size > (P_IO_TLB_SEGSIZE << P_IO_TLB_SHIFT)) {
		dev_warn_ratelimited(dev, "alloc size 0x%lx is larger than segment(0x%x) of pswiotlb\n",
					alloc_size, P_IO_TLB_SEGSIZE << P_IO_TLB_SHIFT);
		return (phys_addr_t)DMA_MAPPING_ERROR;
	}

	if (!mem || !mem->nslabs) {
		dev_warn_ratelimited(dev,
			"Can not allocate PSWIOTLB buffer earlier and can't now provide you with the DMA bounce buffer");
		return (phys_addr_t)DMA_MAPPING_ERROR;
	}

	if (mapping_size > alloc_size) {
		dev_warn_once(dev, "Invalid sizes (mapping: %zd bytes, alloc: %zd bytes)",
			      mapping_size, alloc_size);
		return (phys_addr_t)DMA_MAPPING_ERROR;
	}

	index = pswiotlb_find_slots(dev, nid, orig_addr,
				   alloc_size + offset, alloc_align_mask, &pool);
	if (index == -1) {
		if (!(attrs & DMA_ATTR_NO_WARN))
			dev_warn_once(dev,
	"pswiotlb buffer is full (sz: %zd bytes), total %lu (slots), used %lu (slots)\n",
				 alloc_size, mem->nslabs, mem_used(mem));
		return (phys_addr_t)DMA_MAPPING_ERROR;
	}

	/*
	 * Save away the mapping from the original address to the DMA address.
	 * This is needed when we sync the memory.  Then we sync the buffer if
	 * needed.
	 */
	for (i = 0; i < nr_slots(alloc_size + offset); i++)
		pool->slots[index + i].orig_addr = slot_addr(orig_addr, i);
	tlb_addr = slot_addr(pool->start, index) + offset;
	page = pfn_to_page(PFN_DOWN(tlb_addr));
	set_bit(PG_pswiotlb, &page->flags);

	/*
	 * When dir == DMA_FROM_DEVICE we could omit the copy from the orig
	 * to the tlb buffer, if we knew for sure the device will
	 * overwrite the entire current content. But we don't. Thus
	 * unconditional bounce may prevent leaking pswiotlb content (i.e.
	 * kernel memory) to user-space.
	 */
	pswiotlb_bounce(dev, nid, tlb_addr, mapping_size, DMA_TO_DEVICE, pool);
	return tlb_addr;
}
static void pswiotlb_release_slots(struct device *dev, int nid, phys_addr_t tlb_addr,
			struct p_io_tlb_pool *mem)
{
	unsigned long flags;
	unsigned int offset = pswiotlb_align_offset(dev, tlb_addr);
	int index = (tlb_addr - offset - mem->start) >> P_IO_TLB_SHIFT;
	int nslots = nr_slots(mem->slots[index].alloc_size + offset);
	int aindex = index / mem->area_nslabs;
	struct p_io_tlb_area *area = &mem->areas[aindex];
	int count, i;
	struct page *page = pfn_to_page(PFN_DOWN(tlb_addr));

	/*
	 * Return the buffer to the free list by setting the corresponding
	 * entries to indicate the number of contiguous entries available.
	 * While returning the entries to the free list, we merge the entries
	 * with slots below and above the pool being returned.
	 */
	WARN_ON(aindex >= mem->nareas);

	spin_lock_irqsave(&area->lock, flags);
	if (index + nslots < ALIGN(index + 1, P_IO_TLB_SEGSIZE))
		count = mem->slots[index + nslots].list;
	else
		count = 0;

	/*
	 * Step 1: return the slots to the free list, merging the slots with
	 * superceeding slots
	 */
	for (i = index + nslots - 1; i >= index; i--) {
		mem->slots[i].list = ++count;
		mem->slots[i].orig_addr = INVALID_PHYS_ADDR;
		mem->slots[i].alloc_size = 0;
	}

	/*
	 * Step 2: merge the returned slots with the preceding slots, if
	 * available (non zero)
	 */
	for (i = index - 1;
	     io_tlb_offset(i) != P_IO_TLB_SEGSIZE - 1 && mem->slots[i].list;
	     i--)
		mem->slots[i].list = ++count;
	area->used -= nslots;
	if ((mem != &p_io_tlb_default_mem[nid].defpool) && (area->used == 0)
				&& test_bit(aindex, mem->busy_record))
		clear_bit(aindex, mem->busy_record);
	clear_bit(PG_pswiotlb, &page->flags);
	spin_unlock_irqrestore(&area->lock, flags);
}
/*
 * tlb_addr is the physical address of the bounce buffer to unmap.
 */
void pswiotlb_tbl_unmap_single(struct device *dev, int nid, phys_addr_t tlb_addr,
			      size_t offset, size_t mapping_size, enum dma_data_direction dir,
			      unsigned long attrs, struct p_io_tlb_pool *pool)
{
	struct page *page = pfn_to_page(PFN_DOWN(tlb_addr));
	/*
	 * First, sync the memory before unmapping the entry
	 */
	if (!(attrs & DMA_ATTR_SKIP_CPU_SYNC) &&
	    (dir == DMA_FROM_DEVICE || dir == DMA_BIDIRECTIONAL) &&
		(test_bit(PG_pswiotlbsync, &page->flags) == false))
		pswiotlb_bounce(dev, nid, tlb_addr, mapping_size, DMA_FROM_DEVICE, pool);

	tlb_addr -= offset;
	pswiotlb_release_slots(dev, nid, tlb_addr, pool);

	clear_bit(PG_pswiotlbsync, &page->flags);
}
void pswiotlb_sync_single_for_device(struct device *dev, int nid, phys_addr_t tlb_addr,
		size_t size, enum dma_data_direction dir, struct p_io_tlb_pool *pool)
{
	if (dir == DMA_TO_DEVICE || dir == DMA_BIDIRECTIONAL)
		pswiotlb_bounce(dev, nid, tlb_addr, size, DMA_TO_DEVICE, pool);
	else
		WARN_ON(dir != DMA_FROM_DEVICE);
}

void pswiotlb_sync_single_for_cpu(struct device *dev, int nid, phys_addr_t tlb_addr,
		size_t size, enum dma_data_direction dir, struct p_io_tlb_pool *pool)
{
	if (dir == DMA_FROM_DEVICE || dir == DMA_BIDIRECTIONAL) {
		struct page *page = pfn_to_page(PFN_DOWN(tlb_addr));

		pswiotlb_bounce(dev, nid, tlb_addr, size, DMA_FROM_DEVICE, pool);
		set_bit(PG_pswiotlbsync, &page->flags);
	} else
		WARN_ON(dir != DMA_TO_DEVICE);
}
/*
 * Create a pswiotlb mapping for the buffer at @paddr, and in case of DMAing
 * to the device copy the data into it as well.
 */
dma_addr_t pswiotlb_map(struct device *dev, int nid, phys_addr_t paddr, size_t size,
		enum dma_data_direction dir, unsigned long attrs)
{
	phys_addr_t pswiotlb_addr;
	dma_addr_t dma_addr;

	trace_pswiotlb_bounced(dev, phys_to_dma(dev, paddr), size);

	pswiotlb_addr = pswiotlb_tbl_map_single(dev, nid, paddr, size,
				PAGE_ALIGN(size), PAGE_SIZE - 1, dir, attrs);
	if (pswiotlb_addr == (phys_addr_t)DMA_MAPPING_ERROR)
		return DMA_MAPPING_ERROR;

	dma_addr = phys_to_dma_unencrypted(dev, pswiotlb_addr);

	if (!dev_is_dma_coherent(dev) && !(attrs & DMA_ATTR_SKIP_CPU_SYNC))
		arch_sync_dma_for_device(pswiotlb_addr, size, dir);
	return dma_addr;
}

const struct pswiotlb_bypass_rules bypass_rules_list[] = {
	{PCI_VENDOR_ID_MELLANOX, true, DMA_BIDIRECTIONAL},
	{0, }
};

size_t pswiotlb_max_mapping_size(struct device *dev)
{
	int min_align_mask = dma_get_min_align_mask(dev);
	int min_align = 0;

	/*
	 * pswiotlb_find_slots() skips slots according to
	 * min align mask. This affects max mapping size.
	 * Take it into acount here.
	 */
	if (min_align_mask)
		min_align = roundup(min_align_mask, P_IO_TLB_SIZE);

	return ((size_t)P_IO_TLB_SIZE) * P_IO_TLB_SEGSIZE - min_align;
}

/**
 * is_pswiotlb_allocated() - check if the default Phytium software IO TLB is initialized
 */
bool is_pswiotlb_allocated(struct device *dev)
{
	int nid = dev->local_node;
	return p_io_tlb_default_mem[nid].nslabs;
}

bool is_pswiotlb_active(struct device *dev)
{
	int nid = dev->local_node;
	struct p_io_tlb_mem *mem = &dev->dma_p_io_tlb_mem[nid];

	return mem && mem->nslabs;
}

/**
 * default_pswiotlb_base() - get the base address of the default PSWIOTLB
 *
 * Get the lowest physical address used by the default Phytium software IO TLB pool.
 */
phys_addr_t default_pswiotlb_base(struct device *dev)
{
	int nid = dev->local_node;

	p_io_tlb_default_mem[nid].can_grow = false;

	return p_io_tlb_default_mem[nid].defpool.start;
}

/**
 * default_pswiotlb_limit() - get the address limit of the default PSWIOTLB
 *
 * Get the highest physical address used by the default Phytium software IO TLB pool.
 */
phys_addr_t default_pswiotlb_limit(struct device *dev)
{
	int nid = dev->local_node;

	return p_io_tlb_default_mem[nid].phys_limit;
}
#ifdef CONFIG_DEBUG_FS

static int p_io_tlb_used_get(void *data, u64 *val)
{
	struct p_io_tlb_mem *mem = data;

	*val = mem_used(mem);
	return 0;
}

static int p_io_tlb_hiwater_get(void *data, u64 *val)
{
	struct p_io_tlb_mem *mem = data;

	*val = atomic_long_read(&mem->used_hiwater);
	return 0;
}

static int p_io_tlb_hiwater_set(void *data, u64 val)
{
	struct p_io_tlb_mem *mem = data;

	/* Only allow setting to zero */
	if (val != 0)
		return -EINVAL;

	atomic_long_set(&mem->used_hiwater, val);
	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(fops_p_io_tlb_used, p_io_tlb_used_get, NULL, "%llu\n");
DEFINE_DEBUGFS_ATTRIBUTE(fops_p_io_tlb_hiwater, p_io_tlb_hiwater_get,
				p_io_tlb_hiwater_set, "%llu\n");

static void pswiotlb_create_debugfs_files(struct p_io_tlb_mem *mem,
					 int nid, const char *dirname)
{
	atomic_long_set(&mem->total_used, 0);
	atomic_long_set(&mem->used_hiwater, 0);

	mem->debugfs = debugfs_create_dir(dirname, pswiotlb_debugfs);
	if (!mem->nslabs)
		return;

	debugfs_create_ulong("p_io_tlb_nslabs", 0400, mem->debugfs, &mem->nslabs);
	debugfs_create_file("p_io_tlb_used", 0400, mem->debugfs, mem,
			&fops_p_io_tlb_used);
	debugfs_create_file("p_io_tlb_used_hiwater", 0600, mem->debugfs, mem,
			&fops_p_io_tlb_hiwater);
}

static int passthroughlist_display_show(struct seq_file *m, void *v)
{
	struct pswiotlb_passthroughlist *bl_entry;

	rcu_read_lock();
	list_for_each_entry_rcu(bl_entry, &passthroughlist, node) {
		seq_printf(m, "0x%04x\n", bl_entry->vendor);
	}
	rcu_read_unlock();

	return 0;
}

static int version_display_show(struct seq_file *m, void *v)
{
	seq_puts(m, "pswiotlb version ");
	seq_printf(m, "%s\n", PSWIOTLB_VERSION);

	return 0;
}

static int passthroughlist_add(void *data, u64 val)
{
	struct pswiotlb_passthroughlist *bl_entry;
	unsigned long flags;

	bl_entry = kzalloc(sizeof(*bl_entry), GFP_ATOMIC);
	if (!bl_entry)
		return -ENOMEM;

	bl_entry->vendor = val;
	bl_entry->from_grub = false;

	spin_lock_irqsave(&passthroughlist_lock, flags);
	list_add_rcu(&bl_entry->node, &passthroughlist);
	spin_unlock_irqrestore(&passthroughlist_lock, flags);

	return 0;
}

static int passthroughlist_del(void *data, u64 val)
{
	struct pswiotlb_passthroughlist *bl_entry;
	unsigned long flags;

	rcu_read_lock();
	list_for_each_entry_rcu(bl_entry, &passthroughlist, node) {
		if (bl_entry->vendor == val)
			goto found;
	}
	rcu_read_unlock();

	return 0;
found:
	rcu_read_unlock();
	spin_lock_irqsave(&passthroughlist_lock, flags);
	list_del_rcu(&bl_entry->node);
	spin_unlock_irqrestore(&passthroughlist_lock, flags);

	if (bl_entry->from_grub == false)
		kfree(bl_entry);

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(passthroughlist_display);
DEFINE_SHOW_ATTRIBUTE(version_display);
DEFINE_DEBUGFS_ATTRIBUTE(fops_passthroughlist_add, NULL,
				passthroughlist_add, "%llu\n");
DEFINE_DEBUGFS_ATTRIBUTE(fops_passthroughlist_del, NULL,
				passthroughlist_del, "%llu\n");

static void pswiotlb_create_passthroughlist_debugfs_files(const char *dirname)
{
	passthroughlist_debugfs = debugfs_create_dir(dirname, pswiotlb_debugfs);
	if (!passthroughlist_debugfs)
		return;

	debugfs_create_file("show_devices", 0400, passthroughlist_debugfs, NULL,
			&passthroughlist_display_fops);
	debugfs_create_file("add_device", 0600, passthroughlist_debugfs, NULL,
			&fops_passthroughlist_add);
	debugfs_create_file("del_device", 0600, passthroughlist_debugfs, NULL,
			&fops_passthroughlist_del);
}

static void pswiotlb_create_pswiotlb_debugfs_files(const char *dirname)
{
	int i;
	char name[20] = "";
	char passthroughlist_name[50] = "";

	pswiotlb_debugfs = debugfs_create_dir(dirname, pswiotlb_debugfs);
	if (!pswiotlb_debugfs)
		return;

	debugfs_create_file("version", 0400, pswiotlb_debugfs, NULL,
			&version_display_fops);

	for (i = 0; i < pswiotlb_node_num; i++) {
		sprintf(name, "%s-%d", "pswiotlb", i);
		pswiotlb_create_debugfs_files(&p_io_tlb_default_mem[i], i, name);
	}
	sprintf(passthroughlist_name, "%s", "pswiotlb-passthroughlist");
	pswiotlb_create_passthroughlist_debugfs_files(passthroughlist_name);
}

static int __init pswiotlb_create_default_debugfs(void)
{
	char name[20] = "";

	if (!pswiotlb_mtimer_alive && !pswiotlb_force_disable
				&& is_phytium_ps_socs()) {
		pr_info("setup pswiotlb monitor timer service\n");
		timer_setup(&service_timer, pswiotlb_monitor_service, 0);
		pswiotlb_mtimer_alive = true;

		/* check pswiotlb every 2 seconds*/
		mod_timer(&service_timer, jiffies + 2 * HZ);
	}

	if (!pswiotlb_force_disable && is_phytium_ps_socs()) {
		sprintf(name, "%s", "pswiotlb");
		pswiotlb_create_pswiotlb_debugfs_files(name);
	}

	return 0;
}

late_initcall(pswiotlb_create_default_debugfs);

#else  /* !CONFIG_DEBUG_FS */

static inline void pswiotlb_create_debugfs_files(struct p_io_tlb_mem *mem,
						const char *dirname)
{
}

#endif	/* CONFIG_DEBUG_FS */

// SPDX-License-Identifier: GPL-2.0
#include <linux/memblock.h>
#include <linux/gfp_types.h>
#include <linux/compiler.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/cpu.h>
#include <linux/preempt.h>
#include <linux/ptp-cache.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <asm/haoc/iee-access.h>
#include <asm/haoc/iee.h>
#include <asm/haoc/iee-func.h>
#if defined(CONFIG_PTP_S) && defined(CONFIG_X86_64)
#include <asm/haoc/ptp.h>
#endif

struct iee_cache pg_cache;
#ifdef CONFIG_PTP_S
struct iee_cache pg_user_cache;
#endif

int expend_num;
#ifdef CONFIG_X86_64
static LIST_HEAD(iee_cache_pending_protect_list);
static DEFINE_SPINLOCK(iee_cache_pending_protect_lock);
static atomic_t iee_cache_pending_protect_count = ATOMIC_INIT(0);

enum iee_cache_deferred_op {
	IEE_CACHE_DEFER_PROTECT,
	IEE_CACHE_DEFER_FREE,
};

struct iee_cache_deferred_work {
	struct work_struct work;
	struct list_head list;
	void *object;
	unsigned int order;
	int num_pages;
	enum HAOC_BITMAP_TYPE type;
	enum iee_cache_deferred_op op;
	bool free_after_protect;
};

static void *iee_cache_alloc_pending_protect(struct iee_cache *cache, gfp_t gfp);
static bool iee_cache_mark_pending_protect_free(void *object);
#endif

static inline void *__get_freepointer(void *object)
{
	return *((void **)object);
};

static inline void __set_freepointer(void *object, void *next_object)
{
	*(void **)object = next_object;
};

static inline void __iee_set_freepointer(void *object, void *next_object)
{
	iee_set_freeptr(object, next_object);
};

static inline bool __update_freelist(struct iee_cache *cache,
					    void *freelist_old, void *freelist_new,
					    unsigned long tid)
{
	union freelist_aba_t old = { .freelist = freelist_old, .counter = tid };
	union freelist_aba_t new = { .freelist = freelist_new, .counter = tid + 1 };

	return try_cmpxchg128(&(cache->freelist_tid.full), &old.full, new.full);
}

#ifdef CONFIG_ARM64
static void __ptp_set_iee_pages(unsigned long start_addr, unsigned long end_addr,
				struct iee_cache *cache)
{
	unsigned long addr;

	if (!haoc_enabled)
		return;

	if (start_addr != ALIGN(start_addr, PMD_SIZE))
		panic("IEE: pool (HAOC_BITMAP_TYPE %u) not PMD-aligned.",
		      (unsigned int)cache->name);

	addr = start_addr;
	while (addr < end_addr) {
		set_iee_page(addr, PMD_ORDER, cache->name);
		addr += PMD_SIZE;
	}
	flush_tlb_kernel_range(start_addr, end_addr);
}
#endif

void __init iee_cache_init(struct iee_cache *cache, unsigned long object_order,
					int levels, enum HAOC_BITMAP_TYPE bitmap_type,
					unsigned long reserve_order)
{
	unsigned long addr;
	unsigned long addr_next;
	unsigned long reserve_pages;
	unsigned long start_addr;
	unsigned long end_addr;
	unsigned long object_size;
	struct page *page;

	if (!haoc_enabled && bitmap_type != IEE_PGTABLE
#ifdef CONFIG_PTP_S
	    && bitmap_type != IEE_USER_PGTABLE
#endif
	)
		return;

	object_size = (1 << object_order) * PAGE_SIZE;
	while (1) {
		reserve_pages = (1 << reserve_order) * levels;
		start_addr = (unsigned long)memblock_alloc(reserve_pages * PAGE_SIZE,
			reserve_pages * PAGE_SIZE);
		if (start_addr)
			break;
		reserve_order--;
		/* Allocate pages in pmd blocks to reduce the mapping cost. */
		if (reserve_order < PMD_ORDER)
			panic("IEE: fail to reserve pages for HAOC_BITMAP_TYPE %u",
			      (unsigned int)bitmap_type);
	}
	end_addr = start_addr + reserve_pages * PAGE_SIZE;
	pr_err("IEE: reserve %lu pages for HAOC_BITMAP_TYPE %u, range[0x%lx, 0x%lx]",
	       reserve_pages, (unsigned int)bitmap_type, start_addr, end_addr);

	addr = start_addr;
	addr_next = addr + object_size;
	while (addr < end_addr) {
		page = virt_to_page(addr);
		set_page_count(page, 1);
		__set_freepointer((void *)addr, (void *)addr_next);
		addr += object_size;
		addr_next += object_size;
	}
	__set_freepointer((void *)(addr - object_size), NULL);

	cache->object_order = object_order;
	cache->reserve_order = reserve_order;
	cache->reserve_start_addr = start_addr;
	cache->reserve_end_addr = end_addr;
	cache->levels = levels;
	cache->freelist = (void *)start_addr;
	cache->tid = 0;
	cache->name = bitmap_type;
	cache->init = true;

#ifdef CONFIG_ARM64
	/* IEE for ARM64 needs to access these data by IEE addresses. */
	__ptp_set_iee_pages(start_addr, end_addr, cache);
#endif
#ifdef DEBUG
	atomic_set(&cache->count, reserve_pages);
	atomic_set(&cache->fail_count, 0);
	pr_info("IEE: HAOC_BITMAP_TYPE %u ready. object size 0x%lx, count %d.",
		(unsigned int)cache->name, object_size, atomic_read(&cache->count));
#endif
}

#ifdef CONFIG_ARM64
/* Expand the cache pool with PMD_SIZE for each time. */
static __maybe_unused int __ref iee_cache_expand(struct iee_cache *cache, gfp_t gfp)
{
	unsigned long addr;
	unsigned long addr_next;
	unsigned long start_addr;
	unsigned long end_addr;
	unsigned long object_size;
	unsigned long tid;
	void **freelist;
	struct page *page;

	if (slab_is_available())
		start_addr = __get_free_pages(gfp, PMD_ORDER);
	else
		start_addr = (unsigned long)memblock_alloc(PMD_SIZE, PMD_SIZE);

	if (!start_addr)
		return 0;

	addr = start_addr;
	object_size = (1 << cache->object_order) * PAGE_SIZE;
	addr_next = addr + object_size;
	end_addr = start_addr + PMD_SIZE;
	while (addr < end_addr) {
		page = virt_to_page(addr);
		set_page_count(page, 1);
		__set_freepointer((void *)addr, (void *)addr_next);
		addr += object_size;
		addr_next += object_size;
	}
	__set_freepointer((void *)(addr - object_size), NULL);

	/* IEE for ARM64 needs to access these data by IEE addresses. */
	__ptp_set_iee_pages(start_addr, end_addr, cache);

#ifdef DEBUG
	atomic_add(1 << PMD_ORDER, &cache->count);
	pr_info("IEE: HAOC_BITMAP_TYPE %u expand to count %d. Curr failed: %d",
		(unsigned int)cache->name, atomic_read(&cache->count),
		atomic_read(&cache->fail_count));
#endif

/* Fill the new allocated pages into the cache freelist. */
redo:
	tid = READ_ONCE(cache->tid);
	barrier();
	freelist = READ_ONCE(cache->freelist);
#ifdef CONFIG_PTP_S
	if (cache->name == IEE_USER_PGTABLE)
		__set_freepointer((void *)(end_addr - object_size), freelist);
	else
#endif
	__iee_set_freepointer((void *)(end_addr - object_size), freelist);
	if (unlikely(!__update_freelist(cache, freelist, (void *)start_addr, tid)))
		goto redo;

	expend_num++;
	pr_alert("gwm %s num %d , start_addr: 0x%lx, end_addr: 0x%lx, start_addr-end_addr: 0x%lx\n",
		 __func__, expend_num, start_addr, end_addr, start_addr - end_addr);
	return 1;
}
#endif

void *iee_cache_alloc(struct iee_cache *cache, gfp_t gfp)
{
	unsigned long tid;
	size_t object_size;
	void *object;
	void *next_object;

#ifdef CONFIG_PTP_S
#ifdef CONFIG_ARM64
	if (haoc_enabled && iee_init_done)
#else
	if (haoc_enabled && haoc_init_done)
#endif
		if ((gfp & __GFP_ACCOUNT) && cache == &pg_cache)
			cache = &pg_user_cache;
#endif

	if (!cache->init)
		return (void *)__get_free_pages(gfp, cache->object_order);

	object_size = (1UL << cache->object_order) * PAGE_SIZE;
redo:
	tid = READ_ONCE(cache->tid);
	barrier();
	object = READ_ONCE(cache->freelist);
	if (unlikely(!object)) {
		// slow path alloc
#ifdef CONFIG_ARM64
		// if (iee_cache_expand(cache, gfp) || READ_ONCE(cache->freelist))
		// goto redo;

		/* If the expandsion failed, alloc a singel object without RO protection to
		 * avoid block spliting.
		 */
		object = (void *)__get_free_pages(gfp, cache->object_order);
		if (!object)
			return NULL;
		set_iee_address_valid((unsigned long)object, cache->object_order);
		iee_set_bitmap_type((unsigned long)object, 1 << cache->object_order, cache->name);
#ifdef DEBUG
		WARN_ONCE(1, "IEE: Failed on HAOC_BITMAP_TYPE %u expansion.",
			  (unsigned int)cache->name);
		atomic_add(1 << cache->object_order, &cache->fail_count);
		#endif
#else
		if (!preemptible())
			return iee_cache_alloc_pending_protect(cache, gfp);
		object = (void *)__get_free_pages(gfp, cache->object_order);
		if (!object)
			return NULL;
		set_iee_page((unsigned long)object, 1 << cache->object_order, cache->name);
#endif
	} else {
		/* fast path alloc */
		next_object = __get_freepointer(object);
		if (unlikely(!__update_freelist(cache, object, next_object, tid)))
			goto redo;
		prefetchw(next_object);
		if (gfp & __GFP_ZERO) {
#ifdef CONFIG_PTP_S
			if (cache == &pg_user_cache)
				clear_page(object);
			else
#endif
			iee_memset(object, 0, object_size);
		}
	}
	return object;
};

#ifdef CONFIG_X86_64
static void iee_cache_deferred_work(struct work_struct *work)
{
	struct iee_cache_deferred_work *deferred =
		container_of(work, struct iee_cache_deferred_work, work);
	unsigned long addr = (unsigned long)deferred->object;
	bool free_after_protect = false;

	if (deferred->op == IEE_CACHE_DEFER_PROTECT) {
		set_iee_page(addr, deferred->num_pages, deferred->type);

		spin_lock(&iee_cache_pending_protect_lock);
		list_del(&deferred->list);
		free_after_protect = deferred->free_after_protect;
		atomic_dec(&iee_cache_pending_protect_count);
		spin_unlock(&iee_cache_pending_protect_lock);

		if (!free_after_protect)
			goto out;
	}

	unset_iee_page(addr, deferred->num_pages);
	free_pages(addr, deferred->order);

out:
	kfree(deferred);
}

static void *iee_cache_alloc_pending_protect(struct iee_cache *cache, gfp_t gfp)
{
	struct iee_cache_deferred_work *deferred;
	gfp_t atomic_gfp = GFP_ATOMIC;
	void *object;

	if (gfp & __GFP_ZERO)
		atomic_gfp |= __GFP_ZERO;
	if (gfp & __GFP_ACCOUNT)
		atomic_gfp |= __GFP_ACCOUNT;

	deferred = kmalloc(sizeof(*deferred), GFP_ATOMIC);
	if (!deferred) {
		WARN_ONCE(1,
			  "IEE: failed to allocate pending PTP protect work\n");
		return NULL;
	}

	object = (void *)__get_free_pages(atomic_gfp, cache->object_order);
	if (!object) {
		kfree(deferred);
		return NULL;
	}

	deferred->object = object;
	deferred->order = cache->object_order;
	deferred->num_pages = 1 << cache->object_order;
	deferred->type = cache->name;
	deferred->op = IEE_CACHE_DEFER_PROTECT;
	deferred->free_after_protect = false;
	INIT_WORK(&deferred->work, iee_cache_deferred_work);

	spin_lock(&iee_cache_pending_protect_lock);
	list_add(&deferred->list, &iee_cache_pending_protect_list);
	atomic_inc(&iee_cache_pending_protect_count);
	spin_unlock(&iee_cache_pending_protect_lock);

	WARN_ONCE(1,
		  "IEE: returning pending-protect PTP page, count=%d\n",
		  atomic_read(&iee_cache_pending_protect_count));
	queue_work(system_unbound_wq, &deferred->work);

	return object;
}

static bool iee_cache_mark_pending_protect_free(void *object)
{
	struct iee_cache_deferred_work *deferred;
	bool found = false;

	spin_lock(&iee_cache_pending_protect_lock);
	list_for_each_entry(deferred, &iee_cache_pending_protect_list, list) {
		if (deferred->object == object) {
			deferred->free_after_protect = true;
			found = true;
			break;
		}
	}
	spin_unlock(&iee_cache_pending_protect_lock);

	return found;
}

/*
 * Pool overflow pages are freed from exit_mmap/zap_* with preemption
 * disabled. unset_iee_page() calls set_memory_rw() and must not run there.
 */
static void iee_cache_slow_free_pages(struct iee_cache *cache, void *object)
{
	unsigned long addr = (unsigned long)object;
	int num_pages = 1 << cache->object_order;
	struct iee_cache_deferred_work *deferred;

	if (preemptible()) {
		unset_iee_page(addr, num_pages);
		free_pages(addr, cache->object_order);
		return;
	}

	deferred = kmalloc(sizeof(*deferred), GFP_ATOMIC);
	if (!deferred) {
		WARN_ONCE(1, "IEE: failed to defer PTP page free, leaking page %px\n",
			  object);
		return;
	}

	deferred->object = object;
	deferred->order = cache->object_order;
	deferred->num_pages = num_pages;
	deferred->op = IEE_CACHE_DEFER_FREE;
	INIT_WORK(&deferred->work, iee_cache_deferred_work);
	queue_work(system_unbound_wq, &deferred->work);
}
#endif

void iee_cache_free(struct iee_cache *cache, void *object)
{
	unsigned long tid;
	void **freelist;

#ifdef CONFIG_PTP_S
	if (cache == &pg_cache) {
		if (ptp_is_user_pgtable(object))
			cache = &pg_user_cache;
	}
#endif

	if (!cache->init) {
		free_pages((unsigned long)object, cache->object_order);
		return;
	}

#ifdef CONFIG_X86_64
	if (unlikely((unsigned long)object < cache->reserve_start_addr
		|| (unsigned long)object >= cache->reserve_end_addr)) {
		if (iee_cache_mark_pending_protect_free(object))
			return;
		iee_cache_slow_free_pages(cache, object);
		return;
	}
#endif

#ifdef CONFIG_ARM64
	if (unlikely((unsigned long)object < cache->reserve_start_addr
		|| (unsigned long)object >= cache->reserve_end_addr)) {

		set_iee_address_invalid((unsigned long)object, cache->object_order);
		iee_set_bitmap_type((unsigned long)object, 1 << cache->object_order, IEE_NORMAL);
		free_pages((unsigned long)object, cache->object_order);
		return;
	}
#endif
	// fast path free
redo:
	tid = READ_ONCE(cache->tid);
	barrier();
	freelist = READ_ONCE(cache->freelist);
#ifdef CONFIG_PTP_S
	if (cache == &pg_user_cache)
		__set_freepointer(object, freelist);
	else
#endif
	__iee_set_freepointer(object, freelist);
#ifdef CONFIG_ARM64
	dsb(sy);
#endif
	if (unlikely(!__update_freelist(cache, freelist, object, tid)))
		goto redo;
}

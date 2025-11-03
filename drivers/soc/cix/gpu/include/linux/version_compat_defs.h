/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 *
 * (C) COPYRIGHT 2022-2024 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 */

#ifndef _VERSION_COMPAT_DEFS_H_
#define _VERSION_COMPAT_DEFS_H_

#include <linux/version.h>
#include <linux/highmem.h>
#include <linux/timer.h>
#include <linux/iopoll.h>
#include <linux/bitmap.h>
#include <linux/math64.h>
#include <linux/mm.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/lockdep.h>
#include <linux/ptrace.h>
#include <linux/compiler.h>
#include <linux/overflow.h>
#include <linux/bitops.h>
#include <linux/bits.h>

#ifndef BITS_PER_TYPE
#define BITS_PER_TYPE(type) (sizeof(type) * BITS_PER_BYTE)
#endif

/* This is defined inside kbase for matching the default to kernel's
 * mmap_min_addr, used inside file mali_kbase_mmap.c.
 * Note: the value is set at compile time, matching a kernel's configuration
 * value. It would not be able to track any runtime update of mmap_min_addr.
 */
#ifdef CONFIG_MMU
#define kbase_mmap_min_addr CONFIG_DEFAULT_MMAP_MIN_ADDR

#ifdef CONFIG_LSM_MMAP_MIN_ADDR
#if (CONFIG_LSM_MMAP_MIN_ADDR > CONFIG_DEFAULT_MMAP_MIN_ADDR)
/* Replace the default definition with CONFIG_LSM_MMAP_MIN_ADDR */
#undef kbase_mmap_min_addr
#define kbase_mmap_min_addr CONFIG_LSM_MMAP_MIN_ADDR
#define KBASE_COMPILED_MMAP_MIN_ADDR_MSG \
	"* MALI kbase_mmap_min_addr compiled to CONFIG_LSM_MMAP_MIN_ADDR, no runtime update possible! *"
#endif /* (CONFIG_LSM_MMAP_MIN_ADDR > CONFIG_DEFAULT_MMAP_MIN_ADDR) */
#endif /* CONFIG_LSM_MMAP_MIN_ADDR */

#if (kbase_mmap_min_addr == CONFIG_DEFAULT_MMAP_MIN_ADDR)
#define KBASE_COMPILED_MMAP_MIN_ADDR_MSG \
	"* MALI kbase_mmap_min_addr compiled to CONFIG_DEFAULT_MMAP_MIN_ADDR, no runtime update possible! *"
#endif

#else /* CONFIG_MMU */
#define kbase_mmap_min_addr (0UL)
#define KBASE_COMPILED_MMAP_MIN_ADDR_MSG \
	"* MALI kbase_mmap_min_addr compiled to (0UL), no runtime update possible! *"
#endif /* CONFIG_MMU */

static inline void kbase_timer_setup(struct timer_list *timer,
				     void (*callback)(struct timer_list *timer))
{
	timer_setup(timer, callback, 0);
}

#ifndef WRITE_ONCE
#ifdef ASSIGN_ONCE
#define WRITE_ONCE(x, val) ASSIGN_ONCE(val, x)
#else
#define WRITE_ONCE(x, val) (ACCESS_ONCE(x) = (val))
#endif
#endif

#ifndef READ_ONCE
#define READ_ONCE(x) ACCESS_ONCE(x)
#endif

#ifndef CSTD_UNUSED
#define CSTD_UNUSED(x) ((void)(x))
#endif

static inline void *kbase_kmap(struct page *p)
{
	return kmap_local_page(p);
}

static inline void *kbase_kmap_atomic(struct page *p)
{
	return kmap_local_page(p);
}

static inline void kbase_kunmap(struct page *p, void *address)
{
	CSTD_UNUSED(p);
	kunmap_local(address);

}

static inline void kbase_kunmap_atomic(void *address)
{
	kunmap_local(address);
}

/*
 * There was a big rename in the 4.10 kernel (fence* -> dma_fence*),
 * with most of the related functions keeping the same signatures.
 */

#include <linux/dma-fence.h>

static inline void dma_fence_set_error_helper(struct dma_fence *fence, int error)
{
	dma_fence_set_error(fence, error);
}

#include <linux/mm.h>

static inline void kbase_unpin_user_buf_page(struct page *page)
{
	unpin_user_page(page);
}

static inline long kbase_get_user_pages(unsigned long start, unsigned long nr_pages,
					unsigned int gup_flags, struct page **pages,
					struct vm_area_struct **vmas)
{
	return get_user_pages(start, nr_pages, gup_flags, pages);
}

static inline long kbase_pin_user_pages_remote(struct task_struct *tsk, struct mm_struct *mm,
					       unsigned long start, unsigned long nr_pages,
					       unsigned int gup_flags, struct page **pages,
					       struct vm_area_struct **vmas, int *locked)
{
	return pin_user_pages_remote(mm, start, nr_pages, gup_flags, pages, locked);
}

#define KBASE_CLASS_CREATE(owner, name) class_create(name)
#define kbase_totalram_pages() totalram_pages()

/* For kernel versions from 6.5 onward, the read_poll_timeout_atomic() implementation does not
 * suit our usecase where we have a delay_us of zero. This causes the timeout to take allot longer
 * than expected. mali_read_poll_timeout_atomic() is the previous kernel implementation with the
 * desired timekeeping.
 */
#define mali_read_poll_timeout_atomic(op, val, cond, delay_us, timeout_us, delay_before_read, \
				      args...)                                                \
	({                                                                                    \
		u64 __timeout_us = (timeout_us);                                              \
		unsigned long __delay_us = (delay_us);                                        \
		ktime_t __timeout = ktime_add_us(ktime_get(), __timeout_us);                  \
		if (delay_before_read && __delay_us)                                          \
			udelay(__delay_us);                                                   \
		for (;;) {                                                                    \
			(val) = op(args);                                                     \
			if (cond)                                                             \
				break;                                                        \
			if (__timeout_us && ktime_compare(ktime_get(), __timeout) > 0) {      \
				(val) = op(args);                                             \
				break;                                                        \
			}                                                                     \
			if (__delay_us)                                                       \
				udelay(__delay_us);                                           \
		}                                                                             \
		(cond) ? 0 : -ETIMEDOUT;                                                      \
	})

#ifndef read_poll_timeout_atomic
#define read_poll_timeout_atomic(op, val, cond, delay_us, timeout_us, delay_before_read, ...) \
	mali_read_poll_timeout_atomic(op, val, cond, delay_us, timeout_us, delay_before_read, \
				      __VA_ARGS__)
#endif

#include <linux/refcount.h>

#define kbase_refcount_t refcount_t
#define kbase_refcount_read(x) refcount_read(x)
#define kbase_refcount_set(x, v) refcount_set(x, v)
#define kbase_refcount_dec_and_test(x) refcount_dec_and_test(x)
#define kbase_refcount_dec(x) refcount_dec(x)
#define kbase_refcount_inc_not_zero(x) refcount_inc_not_zero(x)
#define kbase_refcount_inc(x) refcount_inc(x)
#define KBASE_REGISTER_SHRINKER(reclaim, name, priv_data) register_shrinker(reclaim, name)

/* clang-format on */
#define KBASE_UNREGISTER_SHRINKER(reclaim) unregister_shrinker(&reclaim)
#define KBASE_GET_KBASE_DATA_FROM_SHRINKER(s, type, var) container_of(s, type, var)
#define DEFINE_KBASE_SHRINKER struct shrinker
#define KBASE_INIT_RECLAIM(var, attr, name) (&((var)->attr))
#define KBASE_SET_RECLAIM(var, attr, reclaim) ((var)->attr = (*reclaim))

static inline int kbase_param_set_uint_minmax(const char *val, const struct kernel_param *kp,
					      unsigned int min, unsigned int max)
{
	return param_set_uint_minmax(val, kp, min, max);
}


#include <linux/compiler_attributes.h>

#ifndef __maybe_unused
#define __maybe_unused __attribute__((unused))
#endif

#define mali_sysfs_emit(buf, fmt, ...) sysfs_emit(buf, fmt, __VA_ARGS__)

/* Definition of struct defined as extern in of.h */
#define mali_kobj_type const struct kobj_type


/* Define missing stubs from <linux/of.h> for the case when OF_DYNAMIC is disabled. */
#ifndef CONFIG_OF_DYNAMIC
static inline void of_changeset_init(struct of_changeset *ocs)
{
}

static inline void of_changeset_destroy(struct of_changeset *ocs)
{
}

static inline int of_changeset_apply(struct of_changeset *ocs)
{
	return -EINVAL;
}

static inline int of_changeset_revert(struct of_changeset *ocs)
{
	return -EINVAL;
}

static inline int of_changeset_action(struct of_changeset *ocs, unsigned long action,
				      struct device_node *np, struct property *prop)
{
	return -EINVAL;
}

static inline int of_changeset_attach_node(struct of_changeset *ocs, struct device_node *np)
{
	return -EINVAL;
}

static inline int of_changeset_detach_node(struct of_changeset *ocs, struct device_node *np)
{
	return -EINVAL;
}

static inline int of_changeset_add_property(struct of_changeset *ocs, struct device_node *np,
					    struct property *prop)
{
	return -EINVAL;
}

static inline int of_changeset_remove_property(struct of_changeset *ocs, struct device_node *np,
					       struct property *prop)
{
	return -EINVAL;
}

static inline int of_changeset_update_property(struct of_changeset *ocs, struct device_node *np,
					       struct property *prop)
{
	return -EINVAL;
}

static inline int of_changeset_add_prop_u32(struct of_changeset *ocs, struct device_node *np,
					    const char *prop_name, const u32 val)
{
	return -EINVAL;
}

#ifndef CONFIG_SPARC
static inline int of_property_check_flag(const struct property *p, unsigned long flag)
{
	return -EINVAL;
}

static inline void of_property_set_flag(struct property *p, unsigned long flag)
{
}

static inline void of_property_clear_flag(struct property *p, unsigned long flag)
{
}
#endif /* CONFIG_SPARC*/
#endif /* CONFIG_OF_DYNAMIC */

#ifndef fallthrough
#define fallthrough __fallthrough
#endif /* fallthrough */

#ifndef __fallthrough
#define __fallthrough __attribute__((fallthrough))
#endif /* __fallthrough */

static inline void kbase_lockdep_assert_not_held(struct mutex *lock)
{
	lockdep_assert_not_held(lock);
}

#include <linux/minmax.h>

static inline unsigned long
kbase_mm_get_unmapped_area_helper(struct mm_struct *mm, struct file *filp, unsigned long addr,
				  unsigned long len, unsigned long pgoff, unsigned long flags)
{
	return mm->get_unmapped_area(filp, addr, len, pgoff, flags);

}

static inline void kbase_lockdep_assert_held_read(struct rw_semaphore *rwlock)
{
	lockdep_assert_held_read(rwlock);
}

#ifndef DEVFREQ_GOV_SIMPLE_ONDEMAND
#define DEVFREQ_GOV_SIMPLE_ONDEMAND "simple_ondemand"
#endif
#ifndef DEVFREQ_GOV_PASSIVE
#define DEVFREQ_GOV_PASSIVE "passive"
#endif

#endif /* _VERSION_COMPAT_DEFS_H_ */

/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef ZXDH_OSDEP_H
#define ZXDH_OSDEP_H

#include <linux/pci.h>
#ifdef FIELD_PREP
#include <linux/bitfield.h>
#endif
#include <crypto/hash.h>
#include <rdma/ib_verbs.h>
#include <linux/workqueue.h>
#if defined(__OFED_4_8__)
#define refcount_t atomic_t
#define refcount_inc atomic_inc
#define refcount_dec_and_test atomic_dec_and_test
#define refcount_set atomic_set
#else
#include <linux/refcount.h>
#endif /* OFED_4_8 */
#define STATS_TIMER_DELAY 60000

/*
 * See include/linux/compiler_attributes.h in kernel >=5.4 for fallthrough.
 * This code really should be in zxdh_kcompat.h but to cover shared code
 * it had to be here.
 * The two #if checks implements fallthrough definition for kernels < 5.4
 * The first check is for new compiler, GCC >= 5.0.  If code in compiler_attributes.h
 * is not invoked and compiler supports  __has_attribute.
 * If fallthrough is not defined after the first check, the second check against fallthrough
 * will define the macro for the older compiler.
 */
#if !defined(fallthrough) && !defined(__GCC4_has_attribute___noclone__) && \
	defined(__has_attribute)
#define fallthrough __attribute__((__fallthrough__))
#endif
#ifndef fallthrough
#define fallthrough \
	do {        \
	} while (0)
#endif
#define idev_to_dev(ptr) (((ptr)->hw->device))
#ifndef ibdev_dbg
#define zxdh_dbg(idev, fmt, ...) dev_dbg(idev_to_dev(idev), fmt, ##__VA_ARGS__)
#define ibdev_err(ibdev, fmt, ...) dev_err(&((ibdev)->dev), fmt, ##__VA_ARGS__)
#define ibdev_warn(ibdev, fmt, ...) \
	dev_warn(&((ibdev)->dev), fmt, ##__VA_ARGS__)
#define ibdev_info(ibdev, fmt, ...) \
	dev_info(&((ibdev)->dev), fmt, ##__VA_ARGS__)
#define ibdev_notice(ibdev, fmt, ...) \
	dev_notice(&((ibdev)->dev), fmt, ##__VA_ARGS__)
#else
/* #define zxdh_dbg(idev, fmt, ...)                                        \
	do {                                                            \
		struct ib_device *ibdev = zxdh_get_ibdev(idev);         \
		if (ibdev)                                              \
			ibdev_dbg(ibdev, fmt, ##__VA_ARGS__);           \
		else                                                    \
			dev_dbg(idev_to_dev(idev), fmt, ##__VA_ARGS__); \
	} while (0) */
#define zxdh_dbg(idev, fmt, ...) dev_dbg(idev_to_dev(idev), fmt, ##__VA_ARGS__)
#endif

struct zxdh_dma_info {
	dma_addr_t *dmaaddrs;
};

struct zxdh_dma_mem {
	void *va;
	dma_addr_t pa;
	u32 size;
} __packed;

struct zxdh_virt_mem {
	void *va;
	u32 size;
} __packed;

struct zxdh_sc_vsi;
struct zxdh_sc_dev;
struct zxdh_sc_qp;
struct zxdh_puda_buf;
struct zxdh_puda_cmpl_info;
struct zxdh_update_sds_info;
struct zxdh_hmc_fcn_info;
struct zxdh_manage_vf_pble_info;
struct zxdh_hw;
struct zxdh_pci_f;
struct zxdh_virtchnl_req;

#ifndef FIELD_PREP

#if defined(__OFED_4_8__)
/* Special handling for 7.2/OFED. The GENMASK macros need to be updated */
#undef GENMASK
#define GENMASK(h, l) \
	(((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))
#undef GENMASK_ULL
#define GENMASK_ULL(h, l) \
	(((~0ULL) << (l)) & (~0ULL >> (BITS_PER_LONG_LONG - 1 - (h))))
#endif
/* Compat for rdma-core-27.0 and OFED 4.8/RHEL 7.2. Not for UPSTREAM */
#define __bf_shf(x) (__builtin_ffsll(x) - 1)
#define FIELD_PREP(_mask, _val) \
	({ ((typeof(_mask))(_val) << __bf_shf(_mask)) & (_mask); })

#define FIELD_GET(_mask, _reg) \
	({ (typeof(_mask))(((_reg) & (_mask)) >> __bf_shf(_mask)); })
#endif /* FIELD_PREP */
struct ib_device *zxdh_get_ibdev(struct zxdh_sc_dev *dev);
void *zxdh_remove_cqp_head(struct zxdh_sc_dev *dev);
void zxdh_terminate_del_timer(struct zxdh_sc_qp *qp);
void zxdh_hw_stats_start_timer(struct zxdh_sc_vsi *vsi);
void zxdh_hw_stats_stop_timer(struct zxdh_sc_vsi *vsi);
void wr32(struct zxdh_hw *hw, u32 reg, u32 val);
u32 rd32(struct zxdh_hw *hw, u32 reg);
u64 rd64(struct zxdh_hw *hw, u32 reg);
int zxdh_map_vm_page_list(struct zxdh_hw *hw, void *va, dma_addr_t *pg_dma,
			  u32 pg_cnt);
void zxdh_unmap_vm_page_list(struct zxdh_hw *hw, dma_addr_t *pg_dma,
			     u32 pg_cnt);
#endif /* ZXDH_OSDEP_H */

/*******************************************************************************

  WangXun(R) 10GbE PCI Express Virtual Function Linux Network Driver
  Copyright(c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:
  Software Team <linux.nic@trustnetic.com>
  WangXun Technology, HuaXing Times Square A507, Hangzhou, China.

*******************************************************************************/

#ifndef _TXGBE_HELPER_H_
#define _TXGBE_HELPER_H_

#include "txgbe_alloc.h"

/**
 * txgbe_allocate_dma_mem_d - OS specific memory alloc for shared code
 * @hw:   pointer to the HW structure
 * @mem:  ptr to mem struct to fill out
 * @size: size of memory requested
 * @alignment: what to align the allocation to
 **/
inline int txgbe_allocate_dma_mem_d(struct txgbe_hw *hw,
				   struct txgbe_dma_mem *mem,
				   __always_unused enum txgbe_memory_type mtype,
				   u64 size, u32 alignment)
{
	struct txgbe_pf *nf = (struct txgbe_pf *)hw->back;

	mem->size = ALIGN(size, alignment);
	mem->va = dma_zalloc_coherent(&nf->pdev->dev, mem->size,
				      &mem->pa, GFP_KERNEL);
	if (!mem->va)
		return -ENOMEM;

	return 0;
}

/**
 * txgbe_free_dma_mem_d - OS specific memory free for shared code
 * @hw:   pointer to the HW structure
 * @mem:  ptr to mem struct to free
 **/
inline int txgbe_free_dma_mem_d(struct txgbe_hw *hw, struct txgbe_dma_mem *mem)
{
	struct txgbe_pf *nf = (struct txgbe_pf *)hw->back;

	dma_free_coherent(&nf->pdev->dev, mem->size, mem->va, mem->pa);
	mem->va = NULL;
	mem->pa = 0;
	mem->size = 0;

	return 0;
}

/**
 * txgbe_allocate_virt_mem_d - OS specific memory alloc for shared code
 * @hw:   pointer to the HW structure
 * @mem:  ptr to mem struct to fill out
 * @size: size of memory requested
 **/
inline int txgbe_allocate_virt_mem_d(struct txgbe_hw *hw,
				    struct txgbe_virt_mem *mem,
				    u32 size)
{
	mem->size = size;
	mem->va = kzalloc(size, GFP_KERNEL);

	if (!mem->va)
		return -ENOMEM;

	return 0;
}

/**
 * txgbe_free_virt_mem_d - OS specific memory free for shared code
 * @hw:   pointer to the HW structure
 * @mem:  ptr to mem struct to free
 **/
inline int txgbe_free_virt_mem_d(struct txgbe_hw *hw, struct txgbe_virt_mem *mem)
{
	/* it's ok to kfree a NULL pointer */
	kfree(mem->va);
	mem->va = NULL;
	mem->size = 0;

	return 0;
}

/**
 * txgbe_init_spinlock_d - OS specific spinlock init for shared code
 * @sp: pointer to a spinlock declared in driver space
 **/
inline void txgbe_init_spinlock_d(struct txgbe_spinlock *sp)
{
	mutex_init((struct mutex *)sp);
}

/**
 * txgbe_acquire_spinlock_d - OS specific spinlock acquire for shared code
 * @sp: pointer to a spinlock declared in driver space
 **/
inline void txgbe_acquire_spinlock_d(struct txgbe_spinlock *sp)
{
	mutex_lock((struct mutex *)sp);
}

/**
 * txgbe_release_spinlock_d - OS specific spinlock release for shared code
 * @sp: pointer to a spinlock declared in driver space
 **/
inline void txgbe_release_spinlock_d(struct txgbe_spinlock *sp)
{
	mutex_unlock((struct mutex *)sp);
}

/**
 * txgbe_destroy_spinlock_d - OS specific spinlock destroy for shared code
 * @sp: pointer to a spinlock declared in driver space
 **/
inline void txgbe_destroy_spinlock_d(struct txgbe_spinlock *sp)
{
	mutex_destroy((struct mutex *)sp);
}
#endif /* _TXGBE_HELPER_H_ */

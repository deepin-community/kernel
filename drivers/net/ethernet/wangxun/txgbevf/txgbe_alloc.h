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

#ifndef _TXGBE_ALLOC_H_
#define _TXGBE_ALLOC_H_

struct txgbe_hw;

/* Memory allocation types */
enum txgbe_memory_type {
	txgbe_mem_arq_buf = 0,		/* ARQ indirect command buffer */
	txgbe_mem_asq_buf = 1,
	txgbe_mem_atq_buf = 2,		/* ATQ indirect command buffer */
	txgbe_mem_arq_ring = 3,		/* ARQ descriptor ring */
	txgbe_mem_atq_ring = 4,		/* ATQ descriptor ring */
	txgbe_mem_pd = 5,		/* Page Descriptor */
	txgbe_mem_bp = 6,		/* Backing Page - 4KB */
	txgbe_mem_bp_jumbo = 7,		/* Backing Page - > 4KB */
	txgbe_mem_reserved
};

/* prototype for functions used for dynamic memory allocation */
txgbe_status txgbe_allocate_dma_mem(struct txgbe_hw *hw,
					    struct txgbe_dma_mem *mem,
					    enum txgbe_memory_type type,
					    u64 size, u32 alignment);
txgbe_status txgbe_free_dma_mem(struct txgbe_hw *hw,
					struct txgbe_dma_mem *mem);
txgbe_status txgbe_allocate_virt_mem(struct txgbe_hw *hw,
					     struct txgbe_virt_mem *mem,
					     u32 size);
txgbe_status txgbe_free_virt_mem(struct txgbe_hw *hw,
					 struct txgbe_virt_mem *mem);

#endif /* _TXGBE_ALLOC_H_ */

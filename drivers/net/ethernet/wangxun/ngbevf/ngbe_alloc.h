/*******************************************************************************

  WangXun(R) GbE PCI Express Virtual Function Linux Network Driver
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

#ifndef _NGBE_ALLOC_H_
#define _NGBE_ALLOC_H_

struct ngbevf_hw;

/* Memory allocation types */
enum ngbe_memory_type {
	ngbe_mem_arq_buf = 0,		/* ARQ indirect command buffer */
	ngbe_mem_asq_buf = 1,
	ngbe_mem_atq_buf = 2,		/* ATQ indirect command buffer */
	ngbe_mem_arq_ring = 3,		/* ARQ descriptor ring */
	ngbe_mem_atq_ring = 4,		/* ATQ descriptor ring */
	ngbe_mem_pd = 5,		/* Page Descriptor */
	ngbe_mem_bp = 6,		/* Backing Page - 4KB */
	ngbe_mem_bp_jumbo = 7,		/* Backing Page - > 4KB */
	ngbe_mem_reserved
};

/* prototype for functions used for dynamic memory allocation */
ngbe_status ngbe_allocate_dma_mem(struct ngbevf_hw *hw,
					    struct ngbe_dma_mem *mem,
					    enum ngbe_memory_type type,
					    u64 size, u32 alignment);
ngbe_status ngbe_free_dma_mem(struct ngbevf_hw *hw,
					struct ngbe_dma_mem *mem);
ngbe_status ngbe_allocate_virt_mem(struct ngbevf_hw *hw,
					     struct ngbe_virt_mem *mem,
					     u32 size);
ngbe_status ngbe_free_virt_mem(struct ngbevf_hw *hw,
					 struct ngbe_virt_mem *mem);

#endif /* _NGBE_ALLOC_H_ */

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PS3_UTIL_H_
#define _PS3_UTIL_H_

#ifndef _WINDOWS
#include <linux/kernel.h>
#include "ps3_instance_manager.h"

#endif
#include "htp_v200/ps3_htp_def.h"
#include "ps3_driver_log.h"
#include "ps3_platform_utils.h"
#include "ps3_kernel_version.h"

#define PS3_DRV_MAX(x, y) ((x) > (y) ? (x) : (y))

#define PCIE_DMA_HOST_ADDR_BIT53_MASK_CHECK(addr)                              \
	(((1ULL) << (PCIE_DMA_HOST_ADDR_BIT_POS_F1)) & (addr))

#define PCIE_DMA_HOST_ADDR_BIT54_MASK_CHECK(addr)                              \
	(((1ULL) << (PCIE_DMA_HOST_ADDR_BIT_POS_F0)) & (addr))

enum {
	PS3_BLOCK_SIZE_16 = 16,
	PS3_BLOCK_SIZE_32 = 32,
	PS3_BLOCK_SIZE_64 = 64,
	PS3_BLOCK_SIZE_128 = 128,
	PS3_BLOCK_SIZE_256 = 256,
	PS3_BLOCK_SIZE_512 = 512,
	PS3_BLOCK_SIZE_1024 = 1024,
	PS3_BLOCK_SIZE_2048 = 2048,
	PS3_BLOCK_SIZE_4096 = 4096,
	PS3_BLOCK_SIZE_8192 = 8192,
};

#define PS3_BLOCK_SIZE_16K (16uLL << 10)
#define PS3_BLOCK_SIZE_32K (32uLL << 10)
#define PS3_BLOCK_SIZE_64K (64uLL << 10)
#define PS3_BLOCK_SIZE_128K (128uLL << 10)
#define PS3_BLOCK_SIZE_256K (256uLL << 10)
#define PS3_BLOCK_SIZE_512K (512uLL << 10)
#define PS3_BLOCK_SIZE_1M (1uLL << 20)
#define PS3_BLOCK_SIZE_2M (2uLL << 20)
#define PS3_BLOCK_SIZE_4M (4uLL << 20)
#define PS3_BLOCK_SIZE_8M (8uLL << 20)
#define PS3_BLOCK_SIZE_16M (16uLL << 20)
#define PS3_BLOCK_SIZE_32M (32uLL << 20)
#define PS3_BLOCK_SIZE_64M (64uLL << 20)
#define PS3_BLOCK_SIZE_128M (128uLL << 20)
#define PS3_BLOCK_SIZE_256M (256uLL << 20)
#define PS3_BLOCK_SIZE_512M (512uLL << 20)
#define PS3_BLOCK_SIZE_1G (1uLL << 30)
#define PS3_BLOCK_SIZE_2G (2uLL << 30)
#define PS3_BLOCK_SIZE_4G (4uLL << 30)
#define PS3_BLOCK_SIZE_8G (8uLL << 30)
#define PS3_BLOCK_SIZE_16G (16uLL << 30)
#define PS3_BLOCK_SIZE_32G (32uLL << 30)
#define PS3_BLOCK_SIZE_64G (64uLL << 30)
#define PS3_BLOCK_SIZE_128G (128uLL << 30)
#define PS3_BLOCK_SIZE_256G (256uLL << 30)
#define PS3_BLOCK_SIZE_512G (512uLL << 30)
#define PS3_BLOCK_SIZE_1T (1uLL << 40)
#define PS3_BLOCK_SIZE_2T (2uLL << 40)
#define PS3_BLOCK_SIZE_4T (4uLL << 40)
#define PS3_BLOCK_SIZE_8T (8uLL << 40)
#define PS3_BLOCK_SIZE_16T (16uLL << 40)
#define PS3_BLOCK_SIZE_32T (32uLL << 40)

enum {
	PS3_BLOCK_SIZE_SHIFT_4 = 4,
	PS3_BLOCK_SIZE_SHIFT_5 = 5,
	PS3_BLOCK_SIZE_SHIFT_6 = 6,
	PS3_BLOCK_SIZE_SHIFT_7 = 7,
	PS3_BLOCK_SIZE_SHIFT_8 = 8,
	PS3_BLOCK_SIZE_SHIFT_9 = 9,
	PS3_BLOCK_SIZE_SHIFT_10 = 10,
	PS3_BLOCK_SIZE_SHIFT_11 = 11,
	PS3_BLOCK_SIZE_SHIFT_12 = 12,
	PS3_BLOCK_SIZE_SHIFT_13 = 13,
	PS3_BLOCK_SIZE_SHIFT_16K = 14,
	PS3_BLOCK_SIZE_SHIFT_32K = 15,
	PS3_BLOCK_SIZE_SHIFT_64K,
	PS3_BLOCK_SIZE_SHIFT_128K,
	PS3_BLOCK_SIZE_SHIFT_256K,
	PS3_BLOCK_SIZE_SHIFT_512K,
	PS3_BLOCK_SIZE_SHIFT_1M = 20,
	PS3_BLOCK_SIZE_SHIFT_2M,
	PS3_BLOCK_SIZE_SHIFT_4M,
	PS3_BLOCK_SIZE_SHIFT_8M,
	PS3_BLOCK_SIZE_SHIFT_16M,
	PS3_BLOCK_SIZE_SHIFT_32M = 25,
	PS3_BLOCK_SIZE_SHIFT_64M,
	PS3_BLOCK_SIZE_SHIFT_128M,
	PS3_BLOCK_SIZE_SHIFT_256M,
	PS3_BLOCK_SIZE_SHIFT_512M,
	PS3_BLOCK_SIZE_SHIFT_1G = 30,
	PS3_BLOCK_SIZE_SHIFT_2G,
	PS3_BLOCK_SIZE_SHIFT_4G,
	PS3_BLOCK_SIZE_SHIFT_8G,
	PS3_BLOCK_SIZE_SHIFT_16G,
	PS3_BLOCK_SIZE_SHIFT_32G = 35,
	PS3_BLOCK_SIZE_SHIFT_64G,
	PS3_BLOCK_SIZE_SHIFT_128G,
	PS3_BLOCK_SIZE_SHIFT_256G,
	PS3_BLOCK_SIZE_SHIFT_512G,
	PS3_BLOCK_SIZE_SHIFT_1T = 40,
	PS3_BLOCK_SIZE_SHIFT_2T,
	PS3_BLOCK_SIZE_SHIFT_4T,
	PS3_BLOCK_SIZE_SHIFT_8T,
	PS3_BLOCK_SIZE_SHIFT_16T,
	PS3_BLOCK_SIZE_SHIFT_32T = 45,
};

static inline unsigned int ps3_blocksize_to_shift(unsigned int block_size)
{
	unsigned int shift = 0;

	switch (block_size) {
	case PS3_BLOCK_SIZE_16:
		shift = PS3_BLOCK_SIZE_SHIFT_4;
		break;
	case PS3_BLOCK_SIZE_32:
		shift = PS3_BLOCK_SIZE_SHIFT_5;
		break;
	case PS3_BLOCK_SIZE_64:
		shift = PS3_BLOCK_SIZE_SHIFT_6;
		break;
	case PS3_BLOCK_SIZE_128:
		shift = PS3_BLOCK_SIZE_SHIFT_7;
		break;
	case PS3_BLOCK_SIZE_256:
		shift = PS3_BLOCK_SIZE_SHIFT_8;
		break;
	case PS3_BLOCK_SIZE_512:
		shift = PS3_BLOCK_SIZE_SHIFT_9;
		break;
	case PS3_BLOCK_SIZE_1024:
		shift = PS3_BLOCK_SIZE_SHIFT_10;
		break;
	case PS3_BLOCK_SIZE_2048:
		shift = PS3_BLOCK_SIZE_SHIFT_11;
		break;
	case PS3_BLOCK_SIZE_4096:
		shift = PS3_BLOCK_SIZE_SHIFT_12;
		break;
	case PS3_BLOCK_SIZE_8192:
		shift = PS3_BLOCK_SIZE_SHIFT_13;
		break;
	default:
		PS3_BUG();
		break;
	}

	return shift;
}

static inline unsigned int ps3_ringsize_to_shift(unsigned long long ring_size)
{
	unsigned int shift = 0;

	switch (ring_size) {
	case PS3_BLOCK_SIZE_512:
		shift = PS3_BLOCK_SIZE_SHIFT_9;
		break;
	case PS3_BLOCK_SIZE_1024:
		shift = PS3_BLOCK_SIZE_SHIFT_10;
		break;
	case PS3_BLOCK_SIZE_2048:
		shift = PS3_BLOCK_SIZE_SHIFT_11;
		break;
	case PS3_BLOCK_SIZE_4096:
		shift = PS3_BLOCK_SIZE_SHIFT_12;
		break;
	case PS3_BLOCK_SIZE_8192:
		shift = PS3_BLOCK_SIZE_SHIFT_13;
		break;
	case PS3_BLOCK_SIZE_16K:
		shift = PS3_BLOCK_SIZE_SHIFT_16K;
		break;
	case PS3_BLOCK_SIZE_32K:
		shift = PS3_BLOCK_SIZE_SHIFT_32K;
		break;
	case PS3_BLOCK_SIZE_64K:
		shift = PS3_BLOCK_SIZE_SHIFT_64K;
		break;
	case PS3_BLOCK_SIZE_128K:
		shift = PS3_BLOCK_SIZE_SHIFT_128K;
		break;
	case PS3_BLOCK_SIZE_256K:
		shift = PS3_BLOCK_SIZE_SHIFT_256K;
		break;
	case PS3_BLOCK_SIZE_512K:
		shift = PS3_BLOCK_SIZE_SHIFT_512K;
		break;
	case PS3_BLOCK_SIZE_1M:
		shift = PS3_BLOCK_SIZE_SHIFT_1M;
		break;
	case PS3_BLOCK_SIZE_2M:
		shift = PS3_BLOCK_SIZE_SHIFT_2M;
		break;
	case PS3_BLOCK_SIZE_4M:
		shift = PS3_BLOCK_SIZE_SHIFT_4M;
		break;
	case PS3_BLOCK_SIZE_8M:
		shift = PS3_BLOCK_SIZE_SHIFT_8M;
		break;
	case PS3_BLOCK_SIZE_16M:
		shift = PS3_BLOCK_SIZE_SHIFT_16M;
		break;
	case PS3_BLOCK_SIZE_32M:
		shift = PS3_BLOCK_SIZE_SHIFT_32M;
		break;
	case PS3_BLOCK_SIZE_64M:
		shift = PS3_BLOCK_SIZE_SHIFT_64M;
		break;
	case PS3_BLOCK_SIZE_128M:
		shift = PS3_BLOCK_SIZE_SHIFT_128M;
		break;
	case PS3_BLOCK_SIZE_256M:
		shift = PS3_BLOCK_SIZE_SHIFT_256M;
		break;
	case PS3_BLOCK_SIZE_512M:
		shift = PS3_BLOCK_SIZE_SHIFT_512M;
		break;
	case PS3_BLOCK_SIZE_1G:
		shift = PS3_BLOCK_SIZE_SHIFT_1G;
		break;
	case PS3_BLOCK_SIZE_2G:
		shift = PS3_BLOCK_SIZE_SHIFT_2G;
		break;
	case PS3_BLOCK_SIZE_4G:
		shift = PS3_BLOCK_SIZE_SHIFT_4G;
		break;
	case PS3_BLOCK_SIZE_8G:
		shift = PS3_BLOCK_SIZE_SHIFT_8G;
		break;
	case PS3_BLOCK_SIZE_16G:
		shift = PS3_BLOCK_SIZE_SHIFT_16G;
		break;
	case PS3_BLOCK_SIZE_32G:
		shift = PS3_BLOCK_SIZE_SHIFT_32G;
		break;
	case PS3_BLOCK_SIZE_64G:
		shift = PS3_BLOCK_SIZE_SHIFT_64G;
		break;
	case PS3_BLOCK_SIZE_128G:
		shift = PS3_BLOCK_SIZE_SHIFT_128G;
		break;
	case PS3_BLOCK_SIZE_256G:
		shift = PS3_BLOCK_SIZE_SHIFT_256G;
		break;
	case PS3_BLOCK_SIZE_512G:
		shift = PS3_BLOCK_SIZE_SHIFT_512G;
		break;
	case PS3_BLOCK_SIZE_1T:
		shift = PS3_BLOCK_SIZE_SHIFT_1T;
		break;
	case PS3_BLOCK_SIZE_2T:
		shift = PS3_BLOCK_SIZE_SHIFT_2T;
		break;
	case PS3_BLOCK_SIZE_4T:
		shift = PS3_BLOCK_SIZE_SHIFT_4T;
		break;
	case PS3_BLOCK_SIZE_8T:
		shift = PS3_BLOCK_SIZE_SHIFT_8T;
		break;
	case PS3_BLOCK_SIZE_16T:
		shift = PS3_BLOCK_SIZE_SHIFT_16T;
		break;
	case PS3_BLOCK_SIZE_32T:
		shift = PS3_BLOCK_SIZE_SHIFT_32T;
		break;
	default:
		shift = PS3_BLOCK_SIZE_SHIFT_1T;
		break;
	}
	return shift;
}

static unsigned char ps3_dma_addr_bit_pos_check(dma_addr_t handle)
{
	unsigned char bit_pos = 0;

	if (PCIE_DMA_HOST_ADDR_BIT54_MASK_CHECK(handle))
		bit_pos = PCIE_DMA_HOST_ADDR_BIT_POS_F0;
	else if (PCIE_DMA_HOST_ADDR_BIT53_MASK_CHECK(handle))
		bit_pos = PCIE_DMA_HOST_ADDR_BIT_POS_F1;
	else
		bit_pos = PCIE_DMA_HOST_ADDR_BIT_POS;
	return bit_pos;
}

static inline struct dma_pool *ps3_dma_pool_create(const char *name,
						   struct device *dev,
						   size_t size, size_t align,
						   size_t boundary)
{
	struct dma_pool *pool = NULL;

	pool = dma_pool_create(name, dev, size, align, boundary);
	LOG_INFO(
		"create dma pool:name '%s', size %lu, align %lu, boundary %lu, pool %p\n",
		name, (unsigned long)size, (unsigned long)align,
		(unsigned long)boundary, pool);
	return pool;
}

static inline void *ps3_dma_pool_alloc(struct ps3_instance *instance,
				       struct dma_pool *pool, gfp_t mem_flags,
				       dma_addr_t *handle)
{
	void *ret = dma_pool_alloc(pool, mem_flags, handle);

	if (ret != NULL) {
		*handle = PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(
			instance->dma_addr_bit_pos, *handle);
	}
	return ret;
}

static inline void *ps3_dma_pool_zalloc(struct ps3_instance *instance,
					struct dma_pool *pool, gfp_t mem_flags,
					dma_addr_t *handle)
{
	void *ret = dma_pool_zalloc(pool, mem_flags, handle);
	*handle = PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos,
						     *handle);
	return ret;
}

static inline void ps3_dma_pool_destroy(struct dma_pool *pool)
{
	dma_pool_destroy(pool);
	LOG_INFO("pool destroy %p\n", pool);
}

static inline void ps3_dma_pool_free(struct dma_pool *pool, void *vaddr,
				     dma_addr_t dma)
{
	unsigned char bit_pos = 0;

	bit_pos = ps3_dma_addr_bit_pos_check(dma);
	dma_pool_free(pool, vaddr,
		      PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos, dma));
}

static inline unsigned int ps3_scsi_channel_query(struct scsi_cmnd *scmd)
{
	unsigned int ret = U32_MAX;

	if (scmd == NULL)
		goto l_out;

	if (scmd->device == NULL)
		goto l_out;

	ret = scmd->device->channel;
l_out:
	return ret;
}

static inline unsigned int ps3_scsi_target_query(struct scsi_cmnd *scmd)
{
	unsigned int ret = U32_MAX;

	if (scmd == NULL)
		goto l_out;

	if (scmd->device == NULL)
		goto l_out;

	ret = scmd->device->id;
l_out:
	return ret;
}

static inline void *ps3_dma_alloc_coherent(struct ps3_instance *instance,
					   size_t size,
					   unsigned long long *handle)
{
	void *buffer = NULL;

	buffer = dma_alloc_coherent(&instance->pdev->dev, size,
				    (dma_addr_t *)handle, GFP_KERNEL);
	*handle = PCIE_DMA_HOST_ADDR_BIT_POS_SET_NEW(instance->dma_addr_bit_pos,
						     *handle);
	return buffer;
}

static inline void ps3_dma_free_coherent(struct ps3_instance *instance,
					 size_t size, void *vaddr,
					 unsigned long long dma_handle)
{
	unsigned char bit_pos = 0;

	bit_pos = ps3_dma_addr_bit_pos_check(dma_handle);
	dma_free_coherent(&instance->pdev->dev, size, vaddr,
			  PCIE_DMA_HOST_ADDR_BIT_POS_CLEAR_NEW(bit_pos,
							       dma_handle));
}

static inline unsigned int ps3_utility_mod64(unsigned long long dividend,
					     unsigned int divisor)
{
	unsigned long long d = dividend;
	unsigned int remainder = 0;

	if (!divisor) {
		LOG_ERROR("DIVISOR is zero, in div fn\n");
		return (unsigned int)-1;
	}

#ifndef _WINDOWS
	remainder = do_div(d, divisor);
#else
	remainder = d % divisor;
#endif
	return remainder;
}

static inline unsigned long long
ps3_utility_div64_32(unsigned long long dividend, unsigned int divisor)
{
	unsigned long long d = dividend;

	if (!divisor)
		LOG_ERROR("DIVISOR is zero, in div64_32 fn\n");

	do_div(d, divisor);

	return d;
}

#if defined(PS3_STRSCPY)
#define PS3_STRCPY(dest, src, n) strscpy(dest, src, n)
#else
#endif

#if defined(PS3_KMAP_LOCAL)
#define PS3_KMAP(page) kmap_local_page(page)
#else
#endif

#if defined(PS3_KUNMAP_LOCAL)
#define PS3_KUNMAP(page, buff) kunmap_local(buff)
#else
#endif

#if defined(CONFIG_64BIT)
#define PS3_DIV64_32(dividend, divisor) ((dividend) / (divisor))
#define PS3_MOD64(dividend, divisor) ((dividend) % (divisor))
#else
#define PS3_DIV64_32(dividend, divisor)                                        \
	(ps3_utility_div64_32((dividend), (divisor)))
#define PS3_MOD64(dividend, divisor) (ps3_utility_mod64((dividend), (divisor)))
#endif

#endif

// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#include <linux/kernel.h>
#include <linux/io-mapping.h>
#include <linux/delay.h>

#include "sss_kernel.h"
#include "sss_common.h"

#define SSS_MIN_SLEEP_TIME(us) ((us) - (us) / 10)

/* Sleep more than 20ms using msleep is accurate */
#define SSS_HANDLER_SLEEP(usleep_min, wait_once_us) \
do { \
	if ((wait_once_us) >= 20 * USEC_PER_MSEC) \
		msleep((wait_once_us) / USEC_PER_MSEC); \
	else \
		usleep_range((usleep_min), (wait_once_us)); \
} while (0)

int sss_dma_zalloc_coherent_align(void *dev_hdl, u64 size, u64 align,
				  unsigned int flag, struct sss_dma_addr_align *addr)
{
	dma_addr_t pa;
	dma_addr_t pa_align;
	void *va = NULL;
	void *va_align = NULL;

	va = dma_zalloc_coherent(dev_hdl, size, &pa, flag);
	if (!va)
		return -ENOMEM;

	pa_align = ALIGN(pa, align);
	if (pa_align == pa) {
		va_align = va;
		goto same_addr_after_align;
	}

	dma_free_coherent(dev_hdl, size, va, pa);

	va = dma_zalloc_coherent(dev_hdl, size + align, &pa, flag);
	if (!va)
		return -ENOMEM;

	pa_align = ALIGN(pa, align);
	va_align = (void *)((u64)va + (pa_align - pa));

same_addr_after_align:
	addr->origin_paddr = pa;
	addr->align_paddr = pa_align;
	addr->origin_vaddr = va;
	addr->align_vaddr = va_align;
	addr->real_size = (u32)size;

	return 0;
}

void sss_dma_free_coherent_align(void *dev_hdl, struct sss_dma_addr_align *addr)
{
	dma_free_coherent(dev_hdl, addr->real_size, addr->origin_vaddr, addr->origin_paddr);
}

int sss_check_handler_timeout(void *priv_data, sss_wait_handler_t handler,
			      u32 wait_total_ms, u32 wait_once_us)
{
	enum sss_process_ret ret;
	unsigned long end;
	u32 usleep_min = SSS_MIN_SLEEP_TIME(wait_once_us);

	if (!handler)
		return -EINVAL;

	end = jiffies + msecs_to_jiffies(wait_total_ms);
	do {
		ret = handler(priv_data);
		if (ret == SSS_PROCESS_OK)
			return 0;
		else if (ret == SSS_PROCESS_ERR)
			return -EIO;

		SSS_HANDLER_SLEEP(usleep_min, wait_once_us);
	} while (time_before(jiffies, end));

	ret = handler(priv_data);
	if (ret == SSS_PROCESS_OK)
		return 0;
	else if (ret == SSS_PROCESS_ERR)
		return -EIO;

	return -ETIMEDOUT;
}

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HW_COMMON_H
#define SSS_HW_COMMON_H

#include <linux/types.h>

#ifndef BIG_ENDIAN
#define BIG_ENDIAN		0x4321
#endif

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN	0x1234
#endif

#ifdef BYTE_ORDER
#undef BYTE_ORDER
#endif
/* X86 */
#define BYTE_ORDER		LITTLE_ENDIAN

#define ARRAY_LEN(arr) ((int)((int)sizeof(arr) / (int)sizeof((arr)[0])))

#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

enum sss_func_type {
	SSS_FUNC_TYPE_PF,
	SSS_FUNC_TYPE_VF,
	SSS_FUNC_TYPE_PPF,
	SSS_FUNC_TYPE_UNKNOWN,
};

struct sss_dma_addr_align {
	u32			real_size;

	void			*origin_vaddr;
	dma_addr_t		origin_paddr;

	void			*align_vaddr;
	dma_addr_t		align_paddr;
};

enum sss_process_ret {
	SSS_PROCESS_OK	= 0,
	SSS_PROCESS_DOING = 1,
	SSS_PROCESS_ERR = 2,
};

struct sss_sge {
	u32 high_addr;
	u32 low_addr;
	u32 len;
};

typedef enum sss_process_ret(*sss_wait_handler_t)(void *priv_data);

/* *
 * sssnic_cpu_to_be32 - convert data to big endian 32 bit format
 * @data: the data to convert
 * @len: length of data to convert, must be Multiple of 4B
 */
static inline void sss_cpu_to_be32(void *data, int len)
{
	int i, chunk_sz = sizeof(u32);
	int data_len = len;
	u32 *mem = data;

	if (!data)
		return;

	data_len = data_len / chunk_sz;

	for (i = 0; i < data_len; i++) {
		*mem = cpu_to_be32(*mem);
		mem++;
	}
}

/* *
 * sss_cpu_to_be32 - convert data from big endian 32 bit format
 * @data: the data to convert
 * @len: length of data to convert
 */
static inline void sss_be32_to_cpu(void *data, int len)
{
	int i;
	int data_len;
	u32 *array = data;

	if (!data)
		return;

	data_len = len / sizeof(u32);

	for (i = 0; i < data_len; i++) {
		*array = be32_to_cpu(*array);
		array++;
	}
}

/* *
 * sss_set_sge - set dma area in scatter gather entry
 * @sge: scatter gather entry
 * @addr: dma address
 * @len: length of relevant data in the dma address
 */
static inline void sss_set_sge(struct sss_sge *sge, dma_addr_t addr, int len)
{
	sge->high_addr = upper_32_bits(addr);
	sge->low_addr = lower_32_bits(addr);
	sge->len = len;
}

#define sss_hw_be32(val) (val)
#define sss_hw_cpu32(val) (val)
#define sss_hw_cpu16(val) (val)

#endif

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_COMMON_H
#define HINIC3_COMMON_H

#include <linux/types.h>

struct hinic3_dma_addr_align {
	u32 real_size;

	void *ori_vaddr;
	dma_addr_t ori_paddr;

	void *align_vaddr;
	dma_addr_t align_paddr;
};

enum hinic3_wait_return {
	WAIT_PROCESS_CPL	= 0,
	WAIT_PROCESS_WAITING	= 1,
	WAIT_PROCESS_ERR	= 2,
};

struct hinic3_sge {
	u32 hi_addr;
	u32 lo_addr;
	u32 len;
};

/* *
 * hinic_cpu_to_be32 - convert data to big endian 32 bit format
 * @data: the data to convert
 * @len: length of data to convert, must be Multiple of 4B
 */
static inline void hinic3_cpu_to_be32(void *data, int len)
{
	int i, chunk_sz = sizeof(u32);
	int data_len = len;
	u32 *mem = (u32 *)data;

	if (!data)
		return;

	data_len = data_len / chunk_sz;

	for (i = 0; i < data_len; i++) {
		*mem = cpu_to_be32(*mem);
		mem++;
	}
}

/* *
 * hinic3_cpu_to_be32 - convert data from big endian 32 bit format
 * @data: the data to convert
 * @len: length of data to convert
 */
static inline void hinic3_be32_to_cpu(void *data, int len)
{
	int i, chunk_sz = sizeof(u32);
	int data_len = len;
	u32 *mem = (u32 *)data;

	if (!data)
		return;

	data_len = data_len / chunk_sz;

	for (i = 0; i < data_len; i++) {
		*mem = be32_to_cpu(*mem);
		mem++;
	}
}

/* *
 * hinic3_set_sge - set dma area in scatter gather entry
 * @sge: scatter gather entry
 * @addr: dma address
 * @len: length of relevant data in the dma address
 */
static inline void hinic3_set_sge(struct hinic3_sge *sge, dma_addr_t addr,
				  int len)
{
	sge->hi_addr = upper_32_bits(addr);
	sge->lo_addr = lower_32_bits(addr);
	sge->len = (u32)len;
}

#define hinic3_hw_be32(val) (val)
#define hinic3_hw_cpu32(val) (val)
#define hinic3_hw_cpu16(val) (val)

static inline void hinic3_hw_be32_len(void *data, int len)
{
}

static inline void hinic3_hw_cpu32_len(void *data, int len)
{
}

int hinic3_dma_zalloc_coherent_align(void *dev_hdl, u64 size, u64 align,
				     unsigned int flag,
				     struct hinic3_dma_addr_align *mem_align);

void hinic3_dma_free_coherent_align(void *dev_hdl,
				    struct hinic3_dma_addr_align *mem_align);

typedef enum hinic3_wait_return (*wait_cpl_handler)(void *priv_data);

int hinic3_wait_for_timeout(void *priv_data, wait_cpl_handler handler,
			    u32 wait_total_ms, u32 wait_once_us);

/* func_attr.glb_func_idx, global function index */
u16 hinic3_global_func_id(void *hwdev);

int hinic3_global_func_id_get(void *hwdev, u16 *func_id);

/* func_attr.p2p_idx, belongs to which pf */
u8 hinic3_pf_id_of_vf(void *hwdev);

/* func_attr.itf_idx, pcie interface index */
u8 hinic3_pcie_itf_id(void *hwdev);
int hinic3_get_vfid_by_vfpci(void *hwdev, struct pci_dev *pdev, u16 *global_func_id);
/* func_attr.vf_in_pf, the vf offset in pf */
u8 hinic3_vf_in_pf(void *hwdev);

/* func_attr.func_type, 0-PF 1-VF 2-PPF */
enum func_type hinic3_func_type(void *hwdev);

/* The PF func_attr.glb_pf_vf_offset,
 * PF use only
 */
u16 hinic3_glb_pf_vf_offset(void *hwdev);

/* func_attr.mpf_idx, mpf global function index,
 * This value is valid only when it is PF
 */
u8 hinic3_mpf_idx(void *hwdev);

u8 hinic3_ppf_idx(void *hwdev);

/* func_attr.intr_num, MSI-X table entry in function */
u16 hinic3_intr_num(void *hwdev);

#endif

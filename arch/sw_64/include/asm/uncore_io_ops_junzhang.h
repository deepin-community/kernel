/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_UNCORE_IO_OPS_JUNZHANG_H
#define _ASM_SW64_UNCORE_IO_OPS_JUNZHANG_H

#define OFFSET_CFG_INFO	0x1100UL

static inline int __get_cpu_nums(void)
{
	int cpus;
	unsigned long cfg_info;
	void __iomem *spbu_base;

	spbu_base = misc_platform_get_spbu_base(0);

	cfg_info = readq(spbu_base + OFFSET_CFG_INFO);
	cfg_info = (cfg_info >> 33) & 0x3;
	cpus = 1 << cfg_info;

	return cpus;
}

static inline unsigned long __get_node_mem(int node)
{
	unsigned long node_mem;
	unsigned long total_mem;
	void __iomem *spbu_base;

	spbu_base = misc_platform_get_spbu_base(node);

	total_mem = readq(spbu_base + OFFSET_CFG_INFO) >> 3;
	total_mem = (total_mem & 0xffff) << 28;
	node_mem = total_mem / __get_cpu_nums();

	return node_mem;
}

#endif /* _ASM_SW64_UNCORE_IO_OPS_JUNZHANG_H */

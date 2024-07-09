/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_UNCORE_IO_OPS_XUELANG_H
#define _ASM_SW64_UNCORE_IO_OPS_XUELANG_H

#define OFFSET_TRKMODE		0x80UL

#define OFFSET_MC_CAP_CFG	0x1180UL
#define OFFSET_MC_ONLINE	0x3780UL

static inline int __get_cpu_nums(void)
{
	int cpus;
	unsigned long trkmode;
	void __iomem *cab0_base;

	cab0_base = misc_platform_get_cab0_base(0);

	trkmode = readq(cab0_base + OFFSET_TRKMODE);
	trkmode = (trkmode >> 6) & 0x3;
	cpus = 1 << trkmode;

	return cpus;
}

static inline unsigned long __get_node_mem(int node)
{
	unsigned long node_mem;
	unsigned long mc_config;
	unsigned long mc_online;
	unsigned long mc_cap;
	unsigned long mc_num;
	void __iomem *mcu_base = misc_platform_get_spbu_base(node);

	mc_config = readq(mcu_base + OFFSET_MC_CAP_CFG) & 0xf;
	mc_cap = (1UL << mc_config) << 28;
	mc_online = readq(mcu_base + OFFSET_MC_ONLINE) & 0xff;
	mc_num = __kernel_ctpop(mc_online);
	node_mem = mc_cap * mc_num;

	return node_mem;
}

#endif /* _ASM_SW64_UNCORE_IO_OPS_XUELANG_H */

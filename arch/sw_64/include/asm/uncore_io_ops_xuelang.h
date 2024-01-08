/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_UNCORE_IO_OPS_XUELANG_H
#define _ASM_SW64_UNCORE_IO_OPS_XUELANG_H

static inline int __get_cpu_nums(void)
{
	int cpus;
	unsigned long trkmode;

	trkmode = sw64_io_read(0, TRKMODE);
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

	mc_config = sw64_io_read(node, MC_CAP_CFG) & 0xf;
	mc_cap = (1UL << mc_config) << 28;
	mc_online = sw64_io_read(node, MC_ONLINE) & 0xff;
	mc_num = __kernel_ctpop(mc_online);
	node_mem = mc_cap * mc_num;

	return node_mem;
}

static inline unsigned long
__io_read_longtime(int node)
{
	return sw64_io_read(node, LONG_TIME);
}

static inline void
__io_write_longtime(int node, unsigned long data)
{
	sw64_io_write(node, LONG_TIME, data);
}

static inline void
__io_write_longtime_start_en(int node, unsigned long data)
{
	sw64_io_write(node, LONG_TIME_START_EN, data);
}

static inline void
__io_write_fault_int_en(int node, unsigned long data)
{
	sw64_io_write(node, DUAL_CG0_FAULT_INTEN, data);
	sw64_io_write(node, DUAL_CG1_FAULT_INTEN, data);
	sw64_io_write(node, DUAL_CG2_FAULT_INTEN, data);
	sw64_io_write(node, DUAL_CG3_FAULT_INTEN, data);
	sw64_io_write(node, DUAL_CG4_FAULT_INTEN, data);
	sw64_io_write(node, DUAL_CG5_FAULT_INTEN, data);
	sw64_io_write(node, DUAL_CG6_FAULT_INTEN, data);
	sw64_io_write(node, DUAL_CG7_FAULT_INTEN, data);
}

#endif /* _ASM_SW64_UNCORE_IO_OPS_XUELANG_H */

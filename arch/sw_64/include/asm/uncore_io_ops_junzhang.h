/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_UNCORE_IO_OPS_JUNZHANG_H
#define _ASM_SW64_UNCORE_IO_OPS_JUNZHANG_H

static inline int __get_cpu_nums(void)
{
	int cpus;
	unsigned long cfg_info;

	cfg_info = sw64_io_read(0, CFG_INFO);
	cfg_info = (cfg_info >> 33) & 0x3;
	cpus = 1 << cfg_info;

	return cpus;
}

static inline unsigned long __get_node_mem(int node)
{
	unsigned long node_mem;
	unsigned long total_mem;

	total_mem = sw64_io_read(node, CFG_INFO) >> 3;
	total_mem = (total_mem & 0xffff) << 28;
	node_mem = total_mem / __get_cpu_nums();

	return node_mem;
}

#define __io_read_longtime(node)			(0UL)
#define __io_write_longtime(node, data)			do { } while (0)
#define __io_write_longtime_start_en(node, data)	do { } while (0)

static inline void
__io_write_fault_int_en(int node, unsigned long data)
{
	sw64_io_write(node, FAULT_INT_CONFIG, data);
}

#endif /* _ASM_SW64_UNCORE_IO_OPS_JUNZHANG_H */

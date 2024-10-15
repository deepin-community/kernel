/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _ASM_SW64_CPU_H
#define _ASM_SW64_CPU_H

#include <linux/kernel.h>
#include <linux/cache.h>
#include <linux/cacheinfo.h>

#define current_cpu_data cpu_data[smp_processor_id()]

enum hmcall_cpuid_cmd {
	GET_TABLE_ENTRY = 1,
	GET_VENDOR_ID   = 2,
	GET_MODEL       = 3,
	GET_CPU_FREQ    = 4,
	GET_CACHE_INFO  = 5,
	GET_FEATURES    = 6
};

#define CPU_FEAT_FPU	0x1
#define CPU_FEAT_SIMD	0x2
#define CPU_FEAT_UNA	0x4

enum sunway_cpu_model {
	CPU_SW3231 = 0x31,
	CPU_SW831  = 0x32,
	CPU_SW8A   = 0x41
};

struct cpuinfo_sw64 {
	__u8 model;
	__u8 family;
	__u8 chip_var;
	__u8 arch_var;
	__u8 arch_rev;
	__u8 pa_bits;
	__u8 va_bits;
	const char *vendor_id;
	const char *model_id;
	unsigned long last_asid;
	unsigned long last_vpn;
	unsigned long ipi_count;
} __aligned(SMP_CACHE_BYTES);

extern struct cpuinfo_sw64 cpu_data[NR_CPUS];
extern cpumask_t cpu_offline;

extern void store_cpu_data(int cpu);
extern unsigned long get_cpu_freq(void);
extern void update_cpu_freq(unsigned long khz);

extern unsigned int get_cpu_cache_size(int cpu, int level, enum cache_type type);
extern unsigned int get_cpu_cacheline_size(int cpu, int level, enum cache_type type);

#endif /* _ASM_SW64_CPU_H */

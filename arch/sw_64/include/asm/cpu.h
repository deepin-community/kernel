/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _ASM_SW64_CPU_H
#define _ASM_SW64_CPU_H

#include <linux/kernel.h>
#include <linux/cache.h>

#define TABLE_ENTRY_MAX 32
#define VENDOR_ID_MAX   2
#define MODEL_MAX       8

#define CPUID_ARCH_REV_MASK	0xf
#define CPUID_ARCH_REV(val)	((val) & CPUID_ARCH_REV_MASK)
#define CPUID_ARCH_VAR_SHIFT	4
#define CPUID_ARCH_VAR_MASK	(0xf << CPUID_ARCH_VAR_SHIFT)
#define CPUID_ARCH_VAR(val)	\
	(((val) & CPUID_ARCH_VAR_MASK) >> CPUID_ARCH_VAR_SHIFT)
#define CPUID_CHIP_VAR_SHIFT	8
#define CPUID_CHIP_VAR_MASK	(0xf << CPUID_CHIP_VAR_SHIFT)
#define CPUID_CHIP_VAR(val)	\
	(((val) & CPUID_CHIP_VAR_MASK) >> CPUID_CHIP_VAR_SHIFT)
#define CPUID_FAMILY_SHIFT	12
#define CPUID_FAMILY_MASK	(0xf << CPUID_FAMILY_SHIFT)
#define CPUID_FAMILY(val)	\
	(((val) & CPUID_FAMILY_MASK) >> CPUID_FAMILY_SHIFT)
#define CPUID_MODEL_SHIFT	24
#define CPUID_MODEL_MASK	(0xff << CPUID_MODEL_SHIFT)
#define CPUID_MODEL(val)	\
	(((val) & CPUID_MODEL_MASK) >> CPUID_MODEL_SHIFT)
#define CPUID_PA_BITS_SHIFT	32
#define CPUID_PA_BITS_MASK	(0x7fUL << CPUID_PA_BITS_SHIFT)
#define CPUID_PA_BITS(val)	\
	(((val) & CPUID_PA_BITS_MASK) >> CPUID_PA_BITS_SHIFT)
#define CPUID_VA_BITS_SHIFT	39
#define CPUID_VA_BITS_MASK	(0x7fUL << CPUID_VA_BITS_SHIFT)
#define CPUID_VA_BITS(val)	\
	(((val) & CPUID_VA_BITS_MASK) >> CPUID_VA_BITS_SHIFT)

#define current_cpu_data cpu_data[smp_processor_id()]

enum hmcall_cpuid_cmd {
	GET_TABLE_ENTRY = 1,
	GET_VENDOR_ID   = 2,
	GET_MODEL       = 3,
	GET_CPU_FREQ    = 4,
	GET_CACHE_INFO  = 5
};

enum sunway_cpu_model {
	CPU_SW3231 = 0x31,
	CPU_SW831  = 0x32,
	CPU_SW8A   = 0x41
};

struct cache_desc {
	unsigned int size;	/* Bytes per way */
	unsigned int sets;	/* Number of lines per set */
	unsigned char ways;	/* Number of ways */
	unsigned char linesz;	/* Size of line in bytes */
	unsigned char flags;	/* Flags describing cache properties */
};

struct cpuinfo_sw64 {
	unsigned long last_asid;
	unsigned long last_vpn;
	unsigned long ipi_count;
	struct cache_desc icache; /* Primary I-cache */
	struct cache_desc dcache; /* Primary D or combined I/D cache */
	struct cache_desc scache; /* Secondary cache */
	struct cache_desc tcache; /* Tertiary/split secondary cache */
} __aligned(SMP_CACHE_BYTES);

struct cpu_desc_t {
	__u8 model;
	__u8 family;
	__u8 chip_var;
	__u8 arch_var;
	__u8 arch_rev;
	__u8 pa_bits;
	__u8 va_bits;
	char vendor_id[16];
	char model_id[64];
	unsigned long frequency;
} __randomize_layout;

extern struct cpuinfo_sw64 cpu_data[NR_CPUS];
extern struct cpu_desc_t cpu_desc;
extern cpumask_t cpu_offline;

extern void store_cpu_data(int cpu);
extern void __init setup_cpu_info(void);

static inline unsigned long get_cpu_freq(void)
{
	return cpu_desc.frequency;
}

static inline void update_cpu_freq(unsigned long khz)
{
	cpu_desc.frequency = khz * 1000;
}

#endif /* _ASM_SW64_CPU_H */

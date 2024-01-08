/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_HW_INIT_H
#define _ASM_SW64_HW_INIT_H
#include <linux/numa.h>
#include <linux/jump_label.h>

#include <asm/cache.h>

#define MMSIZE		__va(0x2040)

/*
 * Descriptor for a cache
 */
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

#define MAX_NUMSOCKETS		8
struct socket_desc_t {
	bool is_online;	/* 1 for online, 0 for offline */
	int numcores;
	unsigned long socket_mem;
};

enum memmap_types {
	memmap_reserved,
	memmap_pci,
	memmap_initrd,
	memmap_kvm,
	memmap_crashkernel,
	memmap_acpi,
	memmap_use,
	memmap_protected,
};

#define MAX_NUMMEMMAPS		64
struct memmap_entry {
	u64 addr;	/* start of memory segment */
	u64 size;	/* size of memory segment */
	enum memmap_types type;
};

extern struct cpuinfo_sw64 cpu_data[NR_CPUS];
extern void store_cpu_data(int cpu);

extern struct cpu_desc_t cpu_desc;
extern struct socket_desc_t socket_desc[MAX_NUMSOCKETS];
extern int memmap_nr;
extern struct memmap_entry memmap_map[MAX_NUMMEMMAPS];
extern cpumask_t cpu_offline;
extern bool memblock_initialized;

int __init add_memmap_region(u64 addr, u64 size, enum memmap_types type);
void __init process_memmap(void);

static inline unsigned long get_cpu_freq(void)
{
	return cpu_desc.frequency;
}

static inline void update_cpu_freq(unsigned long khz)
{
	cpu_desc.frequency = khz * 1000;
}

#define EMUL_FLAG	(0x1UL << 63)
#define MMSIZE_MASK	(EMUL_FLAG - 1)

DECLARE_STATIC_KEY_TRUE(run_mode_host_key);
DECLARE_STATIC_KEY_FALSE(run_mode_guest_key);
DECLARE_STATIC_KEY_FALSE(run_mode_emul_key);

#define is_in_host()		static_branch_likely(&run_mode_host_key)
#define is_in_guest()		static_branch_unlikely(&run_mode_guest_key)
#define is_in_emul()		static_branch_unlikely(&run_mode_emul_key)
#define is_guest_or_emul()	!static_branch_likely(&run_mode_host_key)

#define CPU_SW3231		0x31
#define CPU_SW831		0x32
#define CPU_SW8A		0x41

#define GET_TABLE_ENTRY		1
#define GET_VENDOR_ID		2
#define GET_MODEL		3
#define GET_CPU_FREQ		4
#define GET_CACHE_INFO		5

#define TABLE_ENTRY_MAX		32
#define VENDOR_ID_MAX		2
#define MODEL_MAX		8
#define CACHE_INFO_MAX		4

#define L1_ICACHE		0
#define L1_DCACHE		1
#define L2_CACHE		2
#define L3_CACHE		3

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


#define CACHE_SIZE_SHIFT	0
#define CACHE_SIZE_MASK		(0xffffffffUL << CACHE_SIZE_SHIFT)
#define CACHE_SIZE(val)	\
	(((val) & CACHE_SIZE_MASK) >> CACHE_SIZE_SHIFT)
#define CACHE_LINE_BITS_SHIFT	32
#define CACHE_LINE_BITS_MASK	(0xfUL << CACHE_LINE_BITS_SHIFT)
#define CACHE_LINE_BITS(val)	\
	(((val) & CACHE_LINE_BITS_MASK) >> CACHE_LINE_BITS_SHIFT)
#define CACHE_INDEX_BITS_SHIFT	36
#define CACHE_INDEX_BITS_MASK	(0x3fUL << CACHE_INDEX_BITS_SHIFT)
#define CACHE_INDEX_BITS(val)	\
	(((val) & CACHE_INDEX_BITS_MASK) >> CACHE_INDEX_BITS_SHIFT)
#define current_cpu_data cpu_data[smp_processor_id()]

#endif /* _ASM_SW64_HW_INIT_H */

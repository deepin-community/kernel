// SPDX-License-Identifier: GPL-2.0

#include <linux/cpumask.h>
#include <linux/delay.h>
#include <linux/seq_file.h>

#include <asm/cache.h>
#include <asm/cpu.h>
#include <asm/mmu_context.h>

/* Map logical to physical */
int __cpu_to_rcid[NR_CPUS];
EXPORT_SYMBOL(__cpu_to_rcid);

/* A collection of per-processor data.  */
struct cpuinfo_sw64 cpu_data[NR_CPUS];
EXPORT_SYMBOL(cpu_data);

struct cpu_desc_t cpu_desc;

cpumask_t cpu_offline = CPU_MASK_NONE;

/* Move global data into per-processor storage */
void store_cpu_data(int cpu)
{
	cpu_data[cpu].last_asid = ASID_FIRST_VERSION;
}

void __init setup_cpu_info(void)
{
	int i;
	struct cache_desc *c;
	unsigned long val;

	val = cpuid(GET_TABLE_ENTRY, 0);
	cpu_desc.model = CPUID_MODEL(val);
	cpu_desc.family = CPUID_FAMILY(val);
	cpu_desc.chip_var = CPUID_CHIP_VAR(val);
	cpu_desc.arch_var = CPUID_ARCH_VAR(val);
	cpu_desc.arch_rev = CPUID_ARCH_REV(val);
	cpu_desc.pa_bits = CPUID_PA_BITS(val);
	cpu_desc.va_bits = CPUID_VA_BITS(val);

	for (i = 0; i < VENDOR_ID_MAX; i++) {
		val = cpuid(GET_VENDOR_ID, i);
		memcpy(cpu_desc.vendor_id + (i * 8), &val, 8);
	}

	for (i = 0; i < MODEL_MAX; i++) {
		val = cpuid(GET_MODEL, i);
		memcpy(cpu_desc.model_id + (i * 8), &val, 8);
	}

	cpu_desc.frequency = cpuid(GET_CPU_FREQ, 0) * 1000UL * 1000UL;

	for (i = 0; i < NR_CPUS; i++) {
		c = &(cpu_data[i].icache);
		val = cpuid(GET_CACHE_INFO, L1_ICACHE);
		c->size = CACHE_SIZE(val);
		c->linesz = 1 << (CACHE_LINE_BITS(val));
		c->sets = 1 << (CACHE_INDEX_BITS(val));
		c->ways = c->size / c->sets / c->linesz;

		c = &(cpu_data[i].dcache);
		val = cpuid(GET_CACHE_INFO, L1_DCACHE);
		c->size = CACHE_SIZE(val);
		c->linesz = 1 << (CACHE_LINE_BITS(val));
		c->sets = 1 << (CACHE_INDEX_BITS(val));
		c->ways = c->size / c->sets / c->linesz;

		c = &(cpu_data[i].scache);
		val = cpuid(GET_CACHE_INFO, L2_CACHE);
		c->size = CACHE_SIZE(val);
		c->linesz = 1 << (CACHE_LINE_BITS(val));
		c->sets = 1 << (CACHE_INDEX_BITS(val));
		c->ways = c->size / c->sets / c->linesz;

		c = &(cpu_data[i].tcache);
		val = cpuid(GET_CACHE_INFO, L3_CACHE);
		c->size = CACHE_SIZE(val);
		c->linesz = 1 << (CACHE_LINE_BITS(val));
		c->sets = 1 << (CACHE_INDEX_BITS(val));
		c->ways = c->size / c->sets / c->linesz;
	}
}

static int show_cpuinfo(struct seq_file *f, void *slot)
{
	int i;
	unsigned long freq;

	freq = cpuid(GET_CPU_FREQ, 0);

	for_each_online_cpu(i) {
		/*
		 * glibc reads /proc/cpuinfo to determine the number of
		 * online processors, looking for lines beginning with
		 * "processor".  Give glibc what it expects.
		 */
		seq_printf(f, "processor\t: %u\n"
				"vendor_id\t: %s\n"
				"cpu family\t: %d\n"
				"model\t\t: %u\n"
				"model name\t: %s CPU @ %lu.%lu%luGHz\n"
				"cpu variation\t: %u\n"
				"cpu revision\t: %u\n",
				i, cpu_desc.vendor_id, cpu_desc.family,
				cpu_desc.model, cpu_desc.model_id,
				freq / 1000, (freq % 1000) / 100,
				(freq % 100) / 10,
				cpu_desc.arch_var, cpu_desc.arch_rev);
		seq_printf(f, "cpu MHz\t\t: %lu.00\n"
				"cache size\t: %u KB\n"
				"physical id\t: %d\n"
				"bogomips\t: %lu.%02lu\n",
				get_cpu_freq() / 1000 / 1000, cpu_data[i].tcache.size >> 10,
				cpu_topology[i].package_id,
				loops_per_jiffy / (500000/HZ),
				(loops_per_jiffy / (5000/HZ)) % 100);

		seq_printf(f, "flags\t\t: fpu simd vpn upn cpuid\n");
		seq_printf(f, "page size\t: %d\n", 8192);
		seq_printf(f, "cache_alignment\t: %d\n", cpu_data[i].tcache.linesz);
		seq_printf(f, "address sizes\t: %u bits physical, %u bits virtual\n\n",
				cpu_desc.pa_bits, cpu_desc.va_bits);
	}

	return 0;
}

/*
 * We show only CPU #0 info.
 */
static void *c_start(struct seq_file *f, loff_t *pos)
{
	return *pos < 1 ? (void *)1 : NULL;
}

static void *c_next(struct seq_file *f, void *v, loff_t *pos)
{
	(*pos)++;
	return NULL;
}

static void c_stop(struct seq_file *f, void *v)
{
}

const struct seq_operations cpuinfo_op = {
	.start	= c_start,
	.next	= c_next,
	.stop	= c_stop,
	.show	= show_cpuinfo,
};


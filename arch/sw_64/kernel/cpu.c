// SPDX-License-Identifier: GPL-2.0

#include <linux/cacheinfo.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/delay.h>
#include <linux/seq_file.h>

#include <asm/cache.h>
#include <asm/cpu.h>
#include <asm/mmu_context.h>

#define TABLE_ENTRY_MAX 32
#define VENDOR_ID_MAX   2
#define MODEL_MAX       8

#define cpuinfo_arch_rev(cpu_info) ((cpu_info) & 0xf)
#define cpuinfo_arch_var(cpu_info) (((cpu_info) >> 4) & 0xf)
#define cpuinfo_chip_var(cpu_info) (((cpu_info) >> 8) & 0xf)
#define cpuinfo_family(cpu_info)   (((cpu_info) >> 12) & 0xf)
#define cpuinfo_model(cpu_info)    (((cpu_info) >> 24) & 0xff)
#define cpuinfo_pa_bits(cpu_info)  (((cpu_info) >> 32) & 0x7fUL)
#define cpuinfo_va_bits(cpu_info)  (((cpu_info) >> 39) & 0x7fUL)

/* Map logical to physical */
int __cpu_to_rcid[NR_CPUS];
EXPORT_SYMBOL(__cpu_to_rcid);

/* A collection of per-processor data.  */
struct cpuinfo_sw64 cpu_data[NR_CPUS];
EXPORT_SYMBOL(cpu_data);

cpumask_t cpu_offline = CPU_MASK_NONE;

static unsigned long cpu_freq;
static unsigned long cpu_info;
static char vendor_id[16];
static char model_id[64];

unsigned long get_cpu_freq(void)
{
	if (likely(cpu_freq))
		return cpu_freq;

	return cpuid(GET_CPU_FREQ, 0) * 1000UL * 1000UL;
}

void update_cpu_freq(unsigned long khz)
{
	cpu_freq = khz * 1000;
}

/* Move global data into per-processor storage */
void store_cpu_data(int cpu)
{
	cpu_data[cpu].last_asid = ASID_FIRST_VERSION;
}

static int cpuinfo_cpu_online(unsigned int cpu)
{
	/* Currently, cpu info is shared by all cores */
	cpu_data[cpu].model    = cpuinfo_model(cpu_info);
	cpu_data[cpu].family   = cpuinfo_family(cpu_info);
	cpu_data[cpu].chip_var = cpuinfo_chip_var(cpu_info);
	cpu_data[cpu].arch_var = cpuinfo_arch_var(cpu_info);
	cpu_data[cpu].arch_rev = cpuinfo_arch_rev(cpu_info);
	cpu_data[cpu].pa_bits  = cpuinfo_pa_bits(cpu_info);
	cpu_data[cpu].va_bits  = cpuinfo_va_bits(cpu_info);

	cpu_data[cpu].vendor_id = vendor_id;
	cpu_data[cpu].model_id  = model_id;

	return 0;
}

static int __init sw64_cpuinfo_init(void)
{
	int i, ret;
	unsigned long val;

	/* Get cpu info */
	cpu_info = cpuid(GET_TABLE_ENTRY, 0);

	/* Get vendor name in string format */
	for (i = 0; i < VENDOR_ID_MAX; i++) {
		val = cpuid(GET_VENDOR_ID, i);
		memcpy(vendor_id + (i * 8), &val, 8);
	}

	/* Get model name in string format */
	for (i = 0; i < MODEL_MAX; i++) {
		val = cpuid(GET_MODEL, i);
		memcpy(model_id + (i * 8), &val, 8);
	}

	ret = cpuhp_setup_state(CPUHP_AP_ONLINE_DYN, "sw64/cpuinfo:online",
			cpuinfo_cpu_online, NULL);
	if (ret < 0) {
		pr_err("cpuinfo: failed to register cpuinfo_cpu_online\n");
		return ret;
	}

	return 0;
}
arch_initcall(sw64_cpuinfo_init);

static int show_cpuinfo(struct seq_file *f, void *slot)
{
	int i;
	unsigned int l3_cache_size, l3_cachline_size;
	unsigned long freq;

	freq = cpuid(GET_CPU_FREQ, 0);

	for_each_online_cpu(i) {
		l3_cache_size = get_cpu_cache_size(i, 3, CACHE_TYPE_UNIFIED);
		l3_cachline_size = get_cpu_cacheline_size(i, 3, CACHE_TYPE_UNIFIED);

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
				i, vendor_id, cpu_data[i].family,
				cpu_data[i].model, model_id,
				freq / 1000, (freq % 1000) / 100,
				(freq % 100) / 10,
				cpu_data[i].arch_var, cpu_data[i].arch_rev);
		seq_printf(f, "cpu MHz\t\t: %lu.00\n"
				"cache size\t: %u KB\n"
				"physical id\t: %d\n"
				"bogomips\t: %lu.%02lu\n",
				get_cpu_freq() / 1000 / 1000, l3_cache_size >> 10,
				cpu_topology[i].package_id,
				loops_per_jiffy / (500000/HZ),
				(loops_per_jiffy / (5000/HZ)) % 100);

		seq_printf(f, "flags\t\t: fpu simd vpn upn cpuid%s\n",
				(cpuid(GET_FEATURES, 0) & CPU_FEAT_UNA) ? " una" : "");
		seq_printf(f, "page size\t: %d\n", 8192);
		seq_printf(f, "cache_alignment\t: %d\n", l3_cachline_size);
		seq_printf(f, "address sizes\t: %u bits physical, %u bits virtual\n\n",
				cpu_data[i].pa_bits, cpu_data[i].va_bits);
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

bool arch_match_cpu_phys_id(int cpu, u64 phys_id)
{
	return phys_id == cpu_physical_id(cpu);
}


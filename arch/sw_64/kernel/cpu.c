// SPDX-License-Identifier: GPL-2.0

#include <linux/cacheinfo.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <linux/dmi.h>

#include <asm/cache.h>
#include <asm/cpu.h>
#include <asm/mmu_context.h>
#include <asm/efi.h>

#define TABLE_ENTRY_MAX 32
#define VENDOR_ID_MAX   2
#define MODEL_MAX       8

/* Offset in the DMI processor structure (Type 4) */
#define DMI_PROCESSOR_FAMILY		0x06
#define DMI_PROCESSOR_MANUFACTURER	0x07
#define DMI_PROCESSOR_VERSION		0x10
#define DMI_PROCESSOR_FAMILY2		0x28

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

static unsigned long cpu_info;
static __u16 family;
static char vendor_id[64];
static char model_id[64];

/* Move global data into per-processor storage */
void store_cpu_data(int cpu)
{
	cpu_data[cpu].last_asid = ASID_FIRST_VERSION;
}

static int cpuinfo_cpu_online(unsigned int cpu)
{
	/* Currently, cpu info is shared by all cores */
	cpu_data[cpu].model    = cpuinfo_model(cpu_info);
	cpu_data[cpu].chip_var = cpuinfo_chip_var(cpu_info);
	cpu_data[cpu].arch_var = cpuinfo_arch_var(cpu_info);
	cpu_data[cpu].arch_rev = cpuinfo_arch_rev(cpu_info);
	cpu_data[cpu].pa_bits  = cpuinfo_pa_bits(cpu_info);
	cpu_data[cpu].va_bits  = cpuinfo_va_bits(cpu_info);

	cpu_data[cpu].family   = family;

	cpu_data[cpu].vendor_id = vendor_id;
	cpu_data[cpu].model_id  = model_id;

	return 0;
}

static __maybe_unused const char * __init dmi_get_string(const struct dmi_header *dm, u8 s)
{
	const u8 *bp = ((u8 *) dm) + dm->length;
	const u8 *nsp;

	if (s) {
		while (--s > 0 && *bp)
			bp += strlen(bp) + 1;

		/* Strings containing only spaces are considered empty */
		nsp = bp;
		while (*nsp == ' ')
			nsp++;
		if (*nsp != '\0')
			return bp;
	}

	return "";
}

static __maybe_unused void __init find_dmi_processor_version(const struct dmi_header *dm,
		void *private)
{
	char *dmi_data = (char *)dm;
	const char *p;
	size_t len;

	if (dm->type != DMI_ENTRY_PROCESSOR)
		return;

	p = dmi_get_string(dm, dmi_data[DMI_PROCESSOR_VERSION]);

	len = strlen(p);

	if ((len > 0) && (len < ARRAY_SIZE(model_id)))
		strcpy(model_id, p);
}

static void __init get_model_id(void)
{
	int i;
	unsigned long val;

#ifdef CONFIG_EFI
	/* Prefer model id from SMBIOS */
	if (!IS_ENABLED(CONFIG_SUBARCH_C3B) &&
			sunway_bios_version)
		dmi_walk(find_dmi_processor_version, NULL);
#endif

	if (strlen(model_id) > 0)
		return;

	/* Fallback to HMCode */
	for (i = 0; i < MODEL_MAX; i++) {
		val = cpuid(GET_MODEL, i);
		memcpy(model_id + (i * 8), &val, 8);
	}
}

static __maybe_unused void __init find_dmi_processor_manufacturer(const struct dmi_header *dm,
		void *private)
{
	char *dmi_data = (char *)dm;
	const char *p;
	size_t len;

	if (dm->type != DMI_ENTRY_PROCESSOR)
		return;

	p = dmi_get_string(dm, dmi_data[DMI_PROCESSOR_MANUFACTURER]);

	len = strlen(p);

	if ((len > 0) && (len < ARRAY_SIZE(vendor_id)))
		strcpy(vendor_id, p);
}

static void __init get_vendor_id(void)
{
	int i;
	unsigned long val;

#ifdef CONFIG_EFI
	/* Prefer vendor id from SMBIOS */
	if (!IS_ENABLED(CONFIG_SUBARCH_C3B) &&
			sunway_bios_version)
		dmi_walk(find_dmi_processor_manufacturer, NULL);
#endif

	if (strlen(vendor_id) > 0)
		return;

	/* Fallback to HMCode */
	for (i = 0; i < VENDOR_ID_MAX; i++) {
		val = cpuid(GET_VENDOR_ID, i);
		memcpy(vendor_id + (i * 8), &val, 8);
	}
}

static __maybe_unused void __init find_dmi_processor_family(const struct dmi_header *dm,
		void *private)
{
	char *dmi_data = (char *)dm;

	if (dm->type != DMI_ENTRY_PROCESSOR)
		return;

	family = *(__u8 *)(dmi_data + DMI_PROCESSOR_FAMILY);

	if (family == 0xfe)
		family = *(__u16 *)(dmi_data + DMI_PROCESSOR_FAMILY2);
}

static void __init get_family(void)
{
#ifdef CONFIG_EFI
	/* Prefer processor family from SMBIOS */
	if (!IS_ENABLED(CONFIG_SUBARCH_C3B) &&
			sunway_bios_version)
		dmi_walk(find_dmi_processor_family, NULL);
#endif

	if (family)
		return;

	/* Fallback to HMCode */
	family = cpuinfo_family(cpu_info);
}

static int __init sw64_cpuinfo_init(void)
{
	int ret;

	/* Get CPU information from HMCode */
	cpu_info = cpuid(GET_TABLE_ENTRY, 0);

	/* Get processor family */
	get_family();

	/* Get vendor name in string format */
	get_vendor_id();

	/* Get processor name in string format */
	get_model_id();

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

	freq = sunway_max_cpu_freq() / MHZ;

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
				get_cpu_freq(i) / 1000 / 1000, l3_cache_size >> 10,
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

#ifdef CONFIG_SW64_CPUFREQ

unsigned long get_cpu_freq(unsigned int cpu)
{
	unsigned long freq = cpufreq_quick_get(cpu);

	if (likely(freq))
		return freq * KHZ;

	return sunway_max_cpu_freq();
}

#endif

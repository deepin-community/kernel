// SPDX-License-Identifier: GPL-2.0-or-later
#include <linux/dmi.h>
#include <linux/string.h>
#include <linux/cputypes.h>
#include <linux/classtypes.h>

static char core_count;

/* Callback function used to retrieve the core count from DMI */
static void get_core_count_by_dmi(const struct dmi_header *dm, void *cpu_count)
{
	const u8 *dmi_data = (const u8 *)dm;

	if (dm->type == 4 && dm->length >= 0x28)
		*(u8 *)cpu_count = *(const u8 *)(dmi_data + 0x23);
}

/* Look up the core count in DMI */
static u8 get_core_count(void)
{
	if (!core_count)
		dmi_walk(get_core_count_by_dmi, &core_count);

	return core_count;
}

static DEFINE_MUTEX(system_cpu_lock);
static struct cpu_identifier system_cpu;
static bool system_cpu_initialized;

static void detect_cpu_vendor(void)
{
	if (read_cpuid_implementor() == ARM_CPU_IMP_PHYTIUM)
		system_cpu.vendor = CPU_VENDOR_PHYTIUM;
	else
		system_cpu.vendor = CPU_VENDOR_UNKNOWN;
}

static void detect_phytium_models(void)
{
	int model = read_cpuid_id() & MIDR_CPU_MODEL_MASK;
	const char *name = get_cpu_name();

	if (model == MIDR_PHYTIUM_FTC663 || model == MIDR_PHYTIUM_PS17064) {
		if (get_core_count() == 8) {
			system_cpu.model_bits = PHYTIUM_MODEL_D2000;
		} else if (get_core_count() == 4 ||
			   strncmp(name, "FT2004", 6) == 0 ||
			   strncmp(name, "FT2000A", 7) == 0 ||
			   strncmp(name, "FT-2000/4", 9) == 0) {
			system_cpu.model_bits = PHYTIUM_MODEL_FT2004;
		} else {
			pr_info("%s: core_count:%d from SMBIOS is invalid!\n",
				__func__, get_core_count());
		}
	} else if (model == MIDR_PHYTIUM_FT3000) {
		if (strstr(name, "D3000M"))
			system_cpu.model_bits = PHYTIUM_MODEL_D3000M;
		else if (strstr(name, "D3000"))
			system_cpu.model_bits = PHYTIUM_MODEL_D3000;
		else
			system_cpu.model_bits = PHYTIUM_MODEL_D3000;
	} else {
		pr_info("%s: unknown model %x", __func__, model);
	}
}

static void init_cpu_identifier(void)
{
	mutex_lock(&system_cpu_lock);

	if (unlikely(system_cpu_initialized)) {
		mutex_unlock(&system_cpu_lock);
		return;
	}

	detect_cpu_vendor();
	if (system_cpu.vendor == CPU_VENDOR_PHYTIUM)
		detect_phytium_models();

	/* Pairs with smp_load_acquire() in cpu_match() */
	smp_store_release(&system_cpu_initialized, true);

	mutex_unlock(&system_cpu_lock);
}

bool cpu_match(u32 vendor, u32 model_mask)
{
	/* Pairs with smp_store_release() in init_cpu_identifier() */
	if (unlikely(!smp_load_acquire(&system_cpu_initialized)))
		init_cpu_identifier();

	return (vendor == system_cpu.vendor) &&
		(model_mask & system_cpu.model_bits);
}
EXPORT_SYMBOL(cpu_match);

bool cpu_is_ft2004(void)
{
	return cpu_match(CPU_VENDOR_PHYTIUM, PHYTIUM_MODEL_FT2004);
}
EXPORT_SYMBOL(cpu_is_ft2004);

bool cpu_is_ft_d2000(void)
{
	return cpu_match(CPU_VENDOR_PHYTIUM, PHYTIUM_MODEL_D2000);
}
EXPORT_SYMBOL(cpu_is_ft_d2000);

bool cpu_is_ft_d3000(void)
{
	return cpu_match(CPU_VENDOR_PHYTIUM, PHYTIUM_MODEL_D3000);
}
EXPORT_SYMBOL(cpu_is_ft_d3000);

bool cpu_is_ft_d3000m(void)
{
	return cpu_match(CPU_VENDOR_PHYTIUM, PHYTIUM_MODEL_D3000M);
}
EXPORT_SYMBOL(cpu_is_ft_d3000m);

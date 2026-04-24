#include <linux/cpu.h>

#include <asm/cpu.h>
#include <asm/cpucaps.h>
#include <asm/cpufeature.h>

unsigned long elf_hwcap __read_mostly;

DECLARE_BITMAP(system_cpucaps, SW64_NCAPS);
EXPORT_SYMBOL(system_cpucaps);

void cpu_set_feature(unsigned int num)
{
	WARN_ON(num >= MAX_CPU_FEATURES);
	elf_hwcap |= BIT(num);
}
EXPORT_SYMBOL_GPL(cpu_set_feature);

bool cpu_have_feature(unsigned int num)
{
	WARN_ON(num >= MAX_CPU_FEATURES);
	return elf_hwcap & BIT(num);
}
EXPORT_SYMBOL_GPL(cpu_have_feature);

static void setup_cpu_features_common(void)
{
	elf_hwcap = 0;
	if (cpuid(GET_FEATURES, 0) & CPU_FEAT_UNA) {
		cpu_set_named_feature(HWUNA);
		set_bit(CPU_FEATURE_HWUNA, system_cpucaps);
	}
}

void __init setup_cpu_features(void)
{
	setup_cpu_features_common();
}

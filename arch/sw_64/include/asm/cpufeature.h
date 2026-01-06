#ifndef __ASM_CPUFEATURE_H
#define __ASM_CPUFEATURE_H

#include <linux/bug.h>
#include <linux/cpu.h>
#include <linux/jump_label.h>
#include <linux/kernel.h>

#include <asm/cpucaps.h>
#include <asm/hwcap.h>

#define MAX_CPU_FEATURES	64
#define cpu_feature(x)		KERNEL_HWCAP_SW64_ ## x
#define cpu_set_named_feature(name) cpu_set_feature(cpu_feature(name))
#define cpu_have_named_feature(name) cpu_have_feature(cpu_feature(name))

extern DECLARE_BITMAP(system_cpucaps, SW64_NCAPS);

void __init setup_cpu_features(void);
void cpu_set_feature(unsigned int num);
bool cpu_have_feature(unsigned int num);

static inline bool cpus_have_cap(unsigned int num)
{
	if (num >= SW64_NCAPS)
		return false;
	return test_bit(num, system_cpucaps);
}

#endif

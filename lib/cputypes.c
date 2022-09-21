#include <linux/dmi.h>
#include <linux/string.h>
#include <asm/dmi.h>
#include <linux/cputypes.h>
#include <linux/classtypes.h>

static char core_count = 0;
static u32 huawei_cpu_type = 0;
/* Callback function used to retrieve the core count from DMI */
static void get_core_count_by_dmi(const struct dmi_header *dm, void *cpu_count)
{
    const u8 *dmi_data = (const u8 *)dm;

    if (dm->type == 4 && dm->length >= 0x28) {
        *(u8 *)cpu_count = *(const u8 *)(dmi_data + 0x23);
    }
}

/* Look up the core count in DMI */
static u8 get_core_count(void)
{
    if (0 == core_count)
        dmi_walk(get_core_count_by_dmi, &core_count);

    return core_count;
}

int cpuid_phytium_flag = -1;
static bool cpuid_is_phytium(void)
{
	int cpu_model;

	if (cpuid_phytium_flag != -1)
		return cpuid_phytium_flag;

	cpu_model = read_cpuid_id() & MIDR_CPU_MODEL_MASK;

	if((cpu_model == MIDR_FT_2500) || (cpu_model == MIDR_PHYTIUM_FT2000PLUS)){
		cpuid_phytium_flag = true;
	} else {
		cpuid_phytium_flag = false;
	}

	return cpuid_phytium_flag;
}

bool cpu_is_ft_d2000(void)
{
	return cpuid_is_phytium() && (get_core_count() == 8);
}
EXPORT_SYMBOL(cpu_is_ft_d2000);

bool cpu_is_phytium(void)
{
	return cpuid_is_phytium();
}
EXPORT_SYMBOL(cpu_is_phytium);

bool cpu_is_ft2004(void)
{
	if ((strncmp(get_cpu_name(), "FT2004", 6) == 0) ||
        (strncmp(get_cpu_name(), "FT2000A",7) == 0) ||
        (strncmp(get_cpu_name(), "FT-2000/4",9) == 0)) {
		return true;
	 }

	 return false;
}
EXPORT_SYMBOL(cpu_is_ft2004);

int cpuid_kunpeng_flag = -1;
bool cpuid_is_kunpeng(void)
{
	int cpu_model;

	if (cpuid_kunpeng_flag != -1)
		return cpuid_kunpeng_flag;

	cpu_model = read_cpuid_id() & MIDR_CPU_MODEL_MASK;

        if(cpu_model == MIDR_HISI_TSV110) {
		cpuid_kunpeng_flag = true;
	} else {
		cpuid_kunpeng_flag = false;
	}

	return cpuid_kunpeng_flag;
}
EXPORT_SYMBOL(cpuid_is_kunpeng);

/* For Pangu CPU kunpeng920 3211k */
bool cpu_is_kunpeng_3211k(void)
{
	if (strncmp(get_cpu_name(),
		    "HUAWEI Kunpeng920 3211K", 32) == 0)
		return true;

	return false;
}
EXPORT_SYMBOL(cpu_is_kunpeng_3211k);

bool cpu_is_hisi(void)
{
	if (strncmp(get_cpu_name(), "HUAWEI",6) == 0) {
		return true;
	}
	return false;
}
EXPORT_SYMBOL(cpu_is_hisi);

/* For CPU pangu */
bool cpu_is_pangu(void)
{
	return cpu_is_kunpeng_3211k();
}
EXPORT_SYMBOL(cpu_is_pangu);

bool cpu_is_kunpeng920_series(void)
{
	if (strncmp(get_cpu_name(), "HUAWEI Kunpeng 920", 18) == 0) {
		return true;
	}
	return false;
}
EXPORT_SYMBOL(cpu_is_kunpeng920_series);

/* kunpeng920 cpu type register */
static u32 get_920_type_reg(void)
{
#define KUNPENG_920_CPU_TYPE_REG_ADDR 0x9038e200
    static void __iomem *ioaddr = NULL;
    u32 cpu_type = 0;

    if (ioaddr) {
        return huawei_cpu_type;
    }

    ioaddr = ioremap(KUNPENG_920_CPU_TYPE_REG_ADDR, 0x100);
    if(ioaddr == NULL) {
        printk(KERN_INFO "get_920_type_reg ioremap return NULL \n\r");
        return -1;
    }

    cpu_type =readl(ioaddr + 0x24);
    huawei_cpu_type = (cpu_type >> 8) & 0xf;

    return huawei_cpu_type;
}

/* For CPU kunpeng920L */
bool cpu_is_kunpeng_920l(void)
{
	if (!cpu_is_kunpeng920_series())
		return false;

	if ((get_core_count() <= 12) && (get_920_type_reg() == 0x1))
		return true;
	else
		return false;
}
EXPORT_SYMBOL(cpu_is_kunpeng_920l);

/* For CPU kunpeng920s */
bool cpu_is_kunpeng_920s(void)
{
	if (!cpu_is_kunpeng920_series())
		return false;

	if (cpu_is_pangu() || cpu_is_kunpeng_920l())
		return false;

    return true;
}
EXPORT_SYMBOL(cpu_is_kunpeng_920s);
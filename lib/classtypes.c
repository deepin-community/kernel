// SPDX-License-Identifier: GPL-2.0-or-later
#include <linux/dmi.h>
#include <linux/string.h>
#include <linux/classtypes.h>
#include <linux/slab.h>
#include <linux/kstrtox.h>

static char cpu_name[256];
static bool get_cpu_flag;
static DEFINE_SPINLOCK(cpu_name_lock);

static void get_cpuname_by_dmi(const struct dmi_header *dm, void *data)
{
	const char *bp;
	const u8 *nsp;
	char *sm;
	char s;

	if (!dm)
		return;
	if (dm->type != 4)
		return;

	bp = ((u8 *) dm) + dm->length;
	sm = (char *)dm;
	s = sm[0x10];

	if (s) {
		while (--s > 0 && *bp)
			bp += strlen(bp) + 1;

		/* Strings containing only spaces are considered empty */
		nsp = bp;
		while (*nsp == ' ')
			nsp++;
		if (*nsp != '\0') {
			spin_lock(&cpu_name_lock);
			if (!get_cpu_flag) {
				strscpy(cpu_name, bp, sizeof(cpu_name));
				get_cpu_flag = true;
			}
			spin_unlock(&cpu_name_lock);
			return;
		}
	}
}

char *get_cpu_name(void)
{
	spin_lock(&cpu_name_lock);
	if (!get_cpu_flag) {
		spin_unlock(&cpu_name_lock);
		dmi_walk(get_cpuname_by_dmi, cpu_name);
	} else {
		spin_unlock(&cpu_name_lock);
	}

	if (cpu_name[0] == '\0')
		return "UNKNOWN-CPU";

	return cpu_name;
}
EXPORT_SYMBOL(get_cpu_name);

static unsigned long chassis_type;

unsigned long get_chassis_types(void)
{
	const char *chassis_type_str = NULL;

	if (chassis_type)
		return chassis_type;

	chassis_type_str = dmi_get_system_info(DMI_CHASSIS_TYPE);
	if (!chassis_type_str)
		return 0;

	if (kstrtoul(chassis_type_str, 10, &chassis_type) != 0)
		return 0;

	return chassis_type;
}
EXPORT_SYMBOL(get_chassis_types);

bool chassis_types_is_laptop(void)
{
	unsigned long type;

	type = get_chassis_types();

	switch (type) {
	case 0x09: /* Laptop */
	case 0x0A: /* Notebook */
		return true;
	}

	return false;
}
EXPORT_SYMBOL(chassis_types_is_laptop);

bool chassis_types_is_allinone(void)
{
	unsigned long type;

	type = get_chassis_types();

	switch (type) {
	case 0x0D: /* All in One */
		return true;
	}

	return false;
}
EXPORT_SYMBOL(chassis_types_is_allinone);

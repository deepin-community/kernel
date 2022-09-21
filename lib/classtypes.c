#include <linux/dmi.h>
#include <linux/string.h>
#include <asm/dmi.h>
#include <linux/classtypes.h>

static char *cpu_name = NULL;
static bool get_cpu_flag = false;
static void get_cpuname_by_dmi(const struct dmi_header *dm, void *data)
{
	const char *bp;
	const u8 *nsp;
	char *sm ;
	char s;
	if(!dm)
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
		if (*nsp != '\0'){
			cpu_name = dmi_alloc(strlen(bp) + 1);
			if (cpu_name != NULL)
				strcpy(cpu_name, bp);

			get_cpu_flag = true;
			return;
		}
	}

	return;
}

char *get_cpu_name(void)
{
	if(!get_cpu_flag)
		dmi_walk(get_cpuname_by_dmi, cpu_name);

	if(cpu_name == NULL) {
		/* Some BIOS don't support getting CPU name from DMI  */
		return "UNKNOWN-CPU";
	}

	return cpu_name;
}
EXPORT_SYMBOL(get_cpu_name);

static unsigned long chassis_type = 0;
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

bool chassis_types_is_desktop(void)
{
	unsigned long type;

	type = get_chassis_types();

	switch (type) {
		case 0x03: /* Desktop */
		case 0x04: /* Low Profile Desktop */
		case 0x05: /* Pizza Box */
		case 0x06: /* Mini Tower */
		case 0x07: /* Tower */
		case 0x10: /* Lunch Box */
			return true;
	}

	return false;
}
EXPORT_SYMBOL(chassis_types_is_desktop);

bool chassis_types_is_server(void)
{
    unsigned long type;

    type = get_chassis_types();

    switch (type) {
        case 0x11: /* Main Server Chassis */
            return true;
    }

    return false;
}
EXPORT_SYMBOL(chassis_types_is_server);

bool chassis_types_is_allinone(void)
{
    unsigned long type;

    type = get_chassis_types();

    switch (type) {
        case 0x0D: // All in One
            return true;
    }

    return false;
}
EXPORT_SYMBOL(chassis_types_is_allinone);

bool os_run_evn_is_virt(void)
{
	if (strncmp(get_cpu_name(), "virt",4) == 0) {
		return true;
	}

	return false;
}
EXPORT_SYMBOL(os_run_evn_is_virt);
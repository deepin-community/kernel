/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * CIX cpu type detection
 */

#ifndef __SOC_CIX_CPU_H__
#define __SOC_CIX_CPU_H__

#include <linux/dmi.h>

static inline void cpu_version_parse(const struct dmi_header *dm, void *data)
{
	char *str, *entry, epos;
	int *ret = data;

	if (!dm || dm->type != DMI_ENTRY_PROCESSOR)
		return;

	str = ((u8 *) dm) + dm->length;
	entry = (char *)dm;
	epos = entry[0x10]; /* version index */

	if (epos) {
		while (--epos > 0 && *str)
			str += strlen(str) + 1;

		if (!strncmp(str, "CIX P1", 6))
			if (ret)
				*ret = 1;
	}

}


static inline int cpu_is_cix_p1_family(void)
{
	int is_p1 = 0;

	dmi_walk(cpu_version_parse, &is_p1);

	return is_p1;
}
#endif


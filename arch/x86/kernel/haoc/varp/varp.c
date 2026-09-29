// SPDX-License-Identifier: GPL-2.0
#include <asm/haoc/haoc-def.h>
#include <asm/haoc/iee.h>
#include <asm/haoc/iee-func.h>
#include <asm/haoc/iee-varp.h>

void _iee_set_varp_modprobe_path(unsigned long __unused, char *data,
					 int maxlen, size_t len, char *buffer,
					 size_t *lenp)
{
	char c, *p;

	if (!data || !buffer || !lenp)
		return;
	p = buffer;
	while ((p - buffer) < *lenp && len < maxlen - 1) {
		c = *(p++);
		if (c == 0 || c == '\n')
			break;
		data[len++] = c;
	}
	data[len] = 0;
}

void __init varp_init(void)
{
	unsigned long start, end, varp_logical_addr;
	int num_pages;

	/* Map .iee.varp as RO pages, the variable need to be protected can be added */
	start = (unsigned long)__iee_varp_data_start;
	end = (unsigned long)__iee_varp_data_end;
	num_pages = (end - start) / PAGE_SIZE;
	set_iee_page(start, num_pages, IEE_VARP);

	/* protect linear mapping */
	varp_logical_addr = (unsigned long)__va(__pa_symbol(start));
	set_iee_page(varp_logical_addr, num_pages, IEE_VARP);

	pr_info("HAOC: CONFIG_VARP enabled.");
}

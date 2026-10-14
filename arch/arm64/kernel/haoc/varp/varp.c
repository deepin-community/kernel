// SPDX-License-Identifier: GPL-2.0
#include <asm/haoc/haoc-def.h>
#include <asm/haoc/iee.h>

void __iee_code _iee_set_varp_modprobe_path(unsigned long __unused, char *data,
					 int maxlen, size_t len, char *buffer,
					 size_t *lenp)
{
	char c, *p;

	if (!data || !buffer || !lenp)
		return;
	data = __ptr_to_iee(data);
	p = buffer;
	while ((p - buffer) < *lenp && len < maxlen - 1) {
		c = *(p++);
		if (c == 0 || c == '\n')
			break;
		data[len++] = c;
	}
	data[len] = 0;
}

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IEE_VARP_H
#define _LINUX_IEE_VARP_H

#include <asm/haoc/haoc-def.h>

#ifdef CONFIG_VARP
#define __iee_varp_data   __section(".iee.varp")
#endif

extern unsigned long long iee_rw_gate(int flag, ...);

extern unsigned long __iee_varp_data_start[];
extern unsigned long __iee_varp_data_end[];

static void __maybe_unused iee_set_varp_modprobe_path(char *data, int maxlen,
						      size_t len, char *buffer,
						      size_t *lenp)
{
	iee_rw_gate(IEE_OP_SET_VARP_MODPROBE_PATH, data, maxlen, len, buffer,
		    lenp);
}
#endif

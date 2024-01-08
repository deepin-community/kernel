/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_MMZONE_H
#define _ASM_SW64_MMZONE_H

#include <asm/smp.h>

/*
 * Following are macros that are specific to this numa platform.
 */

extern pg_data_t *node_data[];

#ifdef CONFIG_NUMA
#define NODE_DATA(nid)		(node_data[(nid)])
#endif

#endif /* _ASM_SW64_MMZONE_H */

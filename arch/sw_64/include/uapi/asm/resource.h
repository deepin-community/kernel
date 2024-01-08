/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_ASM_SW64_RESOURCE_H
#define _UAPI_ASM_SW64_RESOURCE_H

/*
 * SW-64/Linux-specific ordering of these four resource limit IDs,
 * the rest comes from the generic header:
 */
#define RLIMIT_NOFILE		6	/* max number of open files */
#define RLIMIT_AS		7	/* address space limit */
#define RLIMIT_NPROC		8	/* max number of processes */
#define RLIMIT_MEMLOCK		9	/* max locked-in-memory address space */

#include <asm-generic/resource.h>

#endif /* _UAPI_ASM_SW64_RESOURCE_H */

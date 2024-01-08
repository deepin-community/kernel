/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * include/asm/sysinfo.h
 */

#ifndef _UAPI_ASM_SW64_SYSINFO_H
#define _UAPI_ASM_SW64_SYSINFO_H

#define GSI_IEEE_FP_CONTROL		45

#define SSI_IEEE_FP_CONTROL		14
#define SSI_IEEE_RAISE_EXCEPTION	1001	/* linux specific */

#define UAC_BITMASK			7
#define UAC_NOPRINT			1
#define UAC_NOFIX			2
#define UAC_SIGBUS			4
#define PR_NOFIX			4	/* do not fix up unaligned accesses */

#endif /* _UAPI_ASM_SW64_SYSINFO_H */

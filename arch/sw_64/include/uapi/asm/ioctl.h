/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_ASM_SW64_IOCTL_H
#define _UAPI_ASM_SW64_IOCTL_H

#define _IOC_SIZEBITS	13
#define _IOC_DIRBITS	3

/*
 * Direction bits _IOC_NONE could be 0, but legacy version gives it a bit.
 * And this turns out useful to catch old ioctl numbers in header files for
 * us.
 */
#define _IOC_NONE	1U
#define _IOC_READ	2U
#define _IOC_WRITE	4U

#include <asm-generic/ioctl.h>

#endif /* _UAPI_ASM_SW64_IOCTL_H */

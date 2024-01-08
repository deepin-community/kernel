/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_UNISTD_H
#define _ASM_SW64_UNISTD_H

#include <uapi/asm/unistd.h>

#define NR_SYSCALLS			__NR_syscalls
#define NR_syscalls			NR_SYSCALLS

#define __ARCH_WANT_NEW_STAT
#define __ARCH_WANT_OLD_READDIR
#define __ARCH_WANT_STAT64
#define __ARCH_WANT_SYS_GETHOSTNAME
#define __ARCH_WANT_SYS_FADVISE64
#define __ARCH_WANT_SYS_GETPGRP
#define __ARCH_WANT_SYS_OLD_GETRLIMIT
#define __ARCH_WANT_SYS_OLDUMOUNT
#define __ARCH_WANT_SYS_SIGPENDING
#define __ARCH_WANT_SYS_UTIME
#define __ARCH_WANT_SYS_FORK
#define __ARCH_WANT_SYS_VFORK
#define __ARCH_WANT_SYS_CLONE
#define __ARCH_WANT_SYS_SOCKETCALL
#define __ARCH_WANT_SYS_SIGPROCMASK
#define __ARCH_WANT_SYS_CLONE3

#endif /* _ASM_SW64_UNISTD_H */

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _TOOLS_LINUX_ASM_SW64_BARRIER_H
#define _TOOLS_LINUX_ASM_SW64_BARRIER_H

#define mb()	__asm__ __volatile__("mb" : : : "memory")
#define rmb()	__asm__ __volatile__("mb" : : : "memory")
#define wmb()	__asm__ __volatile__("mb" : : : "memory")

#endif /* _TOOLS_LINUX_ASM_SW64_BARRIER_H */

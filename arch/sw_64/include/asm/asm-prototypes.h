/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_ASM_PROTOTYPES_H
#define _ASM_SW64_ASM_PROTOTYPES_H

#include <linux/spinlock.h>
#include <asm/checksum.h>
#include <asm/page.h>
#include <asm/string.h>
#include <linux/uaccess.h>

#include <asm-generic/asm-prototypes.h>

extern void __divl(void);
extern void __reml(void);
extern void __divw(void);
extern void __remw(void);
extern void __divlu(void);
extern void __remlu(void);
extern void __divwu(void);
extern void __remwu(void);

#endif /* _ASM_SW64_ASM_PROTOTYPES_H */

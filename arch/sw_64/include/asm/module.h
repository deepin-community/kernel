/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_MODULE_H
#define _ASM_SW64_MODULE_H

#include <asm-generic/module.h>

struct mod_arch_specific {
	unsigned int gotsecindex;
};

#define ARCH_SHF_SMALL	SHF_SW64_GPREL

#ifdef MODULE
asm(".section .got, \"aw\", @progbits; .align 3; .previous");
#endif

#endif /* _ASM_SW64_MODULE_H */

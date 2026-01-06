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

static inline const Elf_Shdr *find_section(const Elf_Ehdr *hdr,
				    const Elf_Shdr *sechdrs,
				    const char *name)
{
	const Elf_Shdr *s, *se;
	const char *secstrs = (void *)hdr + sechdrs[hdr->e_shstrndx].sh_offset;

	for (s = sechdrs, se = sechdrs + hdr->e_shnum; s < se; s++) {
		if (strcmp(name, secstrs + s->sh_name) == 0)
			return s;
	}

	return NULL;
}

#endif /* _ASM_SW64_MODULE_H */

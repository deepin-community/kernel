/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _SW64_TOOLS_RELOCS_H
#define _SW64_TOOLS_RELOCS_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <elf.h>
#include <byteswap.h>
#define USE_BSD
#include <endian.h>
#include <regex.h>

#define EM_SW64        0x9916
/*
 * SW64 ELF relocation types
 */
#define R_SW64_NONE		0       /* No reloc */
#define R_SW64_REFLONG		1       /* Direct 32 bit */
#define R_SW64_REFQUAD		2       /* Direct 64 bit */
#define R_SW64_GPREL32		3       /* GP relative 32 bit */
#define R_SW64_LITERAL		4       /* GP relative 16 bit w/optimization */
#define R_SW64_LITUSE		5       /* Optimization hint for LITERAL */
#define R_SW64_GPDISP		6       /* Add displacement to GP */
#define R_SW64_BRADDR		7       /* PC+4 relative 23 bit shifted */
#define R_SW64_HINT		8       /* PC+4 relative 16 bit shifted */
#define R_SW64_SREL16		9       /* PC relative 16 bit */
#define R_SW64_SREL32		10      /* PC relative 32 bit */
#define R_SW64_SREL64		11      /* PC relative 64 bit */
#define R_SW64_GPRELHIGH	17      /* GP relative 32 bit, high 16 bits */
#define R_SW64_GPRELLOW		18      /* GP relative 32 bit, low 16 bits */
#define R_SW64_GPREL16		19      /* GP relative 16 bit */
#define R_SW64_COPY		24      /* Copy symbol at runtime */
#define R_SW64_GLOB_DAT		25      /* Create GOT entry */
#define R_SW64_JMP_SLOT		26      /* Create PLT entry */
#define R_SW64_RELATIVE		27      /* Adjust by program base */
#define R_SW64_BRSGP		28
#define R_SW64_TLSGD		29
#define R_SW64_TLS_LDM		30
#define R_SW64_DTPMOD64		31
#define R_SW64_GOTDTPREL	32
#define R_SW64_DTPREL64		33
#define R_SW64_DTPRELHI		34
#define R_SW64_DTPRELLO		35
#define R_SW64_DTPREL16		36
#define R_SW64_GOTTPREL		37
#define R_SW64_TPREL64		38
#define R_SW64_TPRELHI		39
#define R_SW64_TPRELLO		40
#define R_SW64_TPREL16		41
#define R_SW64_LITERAL_GOT	43	/* GP relative */

void die(char *fmt, ...);

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

enum symtype {
	S_ABS,
	S_REL,
	S_SEG,
	S_LIN,
	S_NSYMTYPES
};

void process(FILE *fp, int as_text, int as_bin,
		int show_reloc_info, int keep_relocs);
#endif /* _SW64_TOOLS_RELOCS_H */

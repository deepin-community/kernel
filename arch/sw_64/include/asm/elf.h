/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_ELF_H
#define _ASM_SW64_ELF_H
#ifdef __KERNEL__
#include <asm/auxvec.h>
#endif
/* Special values for the st_other field in the symbol table.  */


#define STO_SW64_NOPV		0x80
#define STO_SW64_STD_GPLOAD	0x88

/*
 * SW-64 ELF relocation types
 */
#define R_SW64_NONE		0	/* No reloc */
#define R_SW64_REFLONG		1	/* Direct 32 bit */
#define R_SW64_REFQUAD		2	/* Direct 64 bit */
#define R_SW64_GPREL32		3	/* GP relative 32 bit */
#define R_SW64_LITERAL		4	/* GP relative 16 bit w/optimization */
#define R_SW64_LITUSE		5	/* Optimization hint for LITERAL */
#define R_SW64_GPDISP		6	/* Add displacement to GP */
#define R_SW64_BRADDR		7	/* PC+4 relative 23 bit shifted */
#define R_SW64_HINT		8	/* PC+4 relative 16 bit shifted */
#define R_SW64_SREL16		9	/* PC relative 16 bit */
#define R_SW64_SREL32		10	/* PC relative 32 bit */
#define R_SW64_SREL64		11	/* PC relative 64 bit */
#define R_SW64_GPRELHIGH	17	/* GP relative 32 bit, high 16 bits */
#define R_SW64_GPRELLOW		18	/* GP relative 32 bit, low 16 bits */
#define R_SW64_GPREL16		19	/* GP relative 16 bit */
#define R_SW64_COPY		24	/* Copy symbol at runtime */
#define R_SW64_GLOB_DAT		25	/* Create GOT entry */
#define R_SW64_JMP_SLOT		26	/* Create PLT entry */
#define R_SW64_RELATIVE		27	/* Adjust by program base */
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

#define SHF_SW64_GPREL		0x10000000

/* Legal values for e_flags field of Elf64_Ehdr.  */

#define EF_SW64_32BIT		1	/* All addresses are below 2GB */

/*
 * ELF register definitions.
 *
 * For now, we just leave it at 33 (32 general regs + processor status word).
 */
#define ELF_NGREG	33

typedef unsigned long elf_greg_t;
typedef elf_greg_t elf_gregset_t[ELF_NGREG];

/* Same with user_fpsimd_state */
#include <uapi/asm/ptrace.h>
typedef struct user_fpsimd_state elf_fpregset_t;

/*
 * This is used to ensure we don't load something for the wrong architecture.
 */
#define elf_check_arch(x) ((x)->e_machine == EM_SW64)

/*
 * These are used to set parameters in the core dumps.
 */
#define ELF_CLASS		ELFCLASS64
#define ELF_DATA		ELFDATA2LSB
#define ELF_ARCH		EM_SW64

#define CORE_DUMP_USE_REGSET
#define ELF_EXEC_PAGESIZE	PAGE_SIZE

/*
 * This is the location that an ET_DYN program is loaded if exec'ed.  Typical
 * use of this is to invoke "./ld.so someprog" to test out a new version of
 * the loader.  We need to make sure that it is out of the way of the program
 * that it will "exec", and that there is sufficient room for the brk.
 */

#define ELF_ET_DYN_BASE		(TASK_UNMAPPED_BASE + 0x1000000)

/*
 * $0 is set by ld.so to a pointer to a function which might be
 * registered using atexit.  This provides a mean for the dynamic
 * linker to call DT_FINI functions for shared libraries that have
 * been loaded before the code runs.

 * So that we can use the same startup file with static executables,
 * we start programs with a value of 0 to indicate that there is no
 * such function.
 */

#define ELF_PLAT_INIT(_r, load_addr)	(_r->regs[0] = 0)

/*
 * The registers are laid out in pt_regs for HMCODE and syscall
 * convenience.  Re-order them for the linear elf_gregset_t.
 */

#define ARCH_HAS_SETUP_ADDITIONAL_PAGES 1
struct linux_binprm;
extern int arch_setup_additional_pages(struct linux_binprm *bprm,
				       int uses_interp);

#ifdef __KERNEL__
struct pt_regs;
struct task_struct;
extern void sw64_elf_core_copy_regs(elf_greg_t *dest, struct pt_regs *pt);
#define ELF_CORE_COPY_REGS(DEST, REGS) sw64_elf_core_copy_regs(DEST, REGS);

/*
 * This yields a mask that user programs can use to figure out what
 * instruction set this CPU supports.
 */

#define ELF_HWCAP	0

/*
 * This yields a string that ld.so will use to load implementation
 * specific libraries for optimization.  This is more specific in
 * intent than poking at uname or /proc/cpuinfo.
 */

#define ELF_PLATFORM	("sw_64")


/* update AT_VECTOR_SIZE_ARCH if the number of NEW_AUX_ENT entries changes */
#define ARCH_DLINFO						\
do {								\
	NEW_AUX_ENT(AT_SYSINFO_EHDR,				\
	(elf_addr_t)current->mm->context.vdso);			\
} while (0)

struct mm_struct;
extern unsigned long arch_randomize_brk(struct mm_struct *mm);
#define arch_randomize_brk arch_randomize_brk
#endif

#endif /* _ASM_SW64_ELF_H */

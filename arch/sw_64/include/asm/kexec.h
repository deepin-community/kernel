/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_KEXEC_H
#define _ASM_SW64_KEXEC_H

#ifdef CONFIG_KEXEC

/* Maximum physical address we can use pages from */
#define KEXEC_SOURCE_MEMORY_LIMIT	(-1UL)
/* Maximum address we can reach in physical address mode */
#define KEXEC_DESTINATION_MEMORY_LIMIT	(-1UL)
/* Maximum address we can use for the control code buffer */
#define KEXEC_CONTROL_MEMORY_LIMIT	(-1UL)

#define KEXEC_CONTROL_PAGE_SIZE		8192

#define KEXEC_ARCH			KEXEC_ARCH_SW64

#define KEXEC_SW64_ATAGS_OFFSET		0x1000
#define KEXEC_SW64_ZIMAGE_OFFSET	0x8000

#ifndef __ASSEMBLY__

/**
 * crash_setup_regs() - save registers for the panic kernel
 * @newregs: registers are saved here
 * @oldregs: registers to be saved (may be %NULL)
 *
 * Function copies machine registers from @oldregs to @newregs. If @oldregs is
 * %NULL then current registers are stored there.
 */
static inline void crash_setup_regs(struct pt_regs *newregs,
				    struct pt_regs *oldregs)
{
	if (oldregs) {
		memcpy(newregs, oldregs, sizeof(*newregs));
	} else {
		__asm__ __volatile__ ("stl $0, %0" : "=m" (newregs->regs[0]));
		__asm__ __volatile__ ("stl $1, %0" : "=m" (newregs->regs[1]));
		__asm__ __volatile__ ("stl $2, %0" : "=m" (newregs->regs[2]));
		__asm__ __volatile__ ("stl $3, %0" : "=m" (newregs->regs[3]));
		__asm__ __volatile__ ("stl $4, %0" : "=m" (newregs->regs[4]));
		__asm__ __volatile__ ("stl $5, %0" : "=m" (newregs->regs[5]));
		__asm__ __volatile__ ("stl $6, %0" : "=m" (newregs->regs[6]));
		__asm__ __volatile__ ("stl $7, %0" : "=m" (newregs->regs[7]));
		__asm__ __volatile__ ("stl $8, %0" : "=m" (newregs->regs[8]));
		__asm__ __volatile__ ("stl $9, %0" : "=m" (newregs->regs[9]));
		__asm__ __volatile__ ("stl $10, %0" : "=m" (newregs->regs[10]));
		__asm__ __volatile__ ("stl $11, %0" : "=m" (newregs->regs[11]));
		__asm__ __volatile__ ("stl $12, %0" : "=m" (newregs->regs[12]));
		__asm__ __volatile__ ("stl $13, %0" : "=m" (newregs->regs[13]));
		__asm__ __volatile__ ("stl $14, %0" : "=m" (newregs->regs[14]));
		__asm__ __volatile__ ("stl $15, %0" : "=m" (newregs->regs[15]));
		__asm__ __volatile__ ("stl $16, %0" : "=m" (newregs->regs[16]));
		__asm__ __volatile__ ("stl $17, %0" : "=m" (newregs->regs[17]));
		__asm__ __volatile__ ("stl $18, %0" : "=m" (newregs->regs[18]));
		__asm__ __volatile__ ("stl $19, %0" : "=m" (newregs->regs[19]));
		__asm__ __volatile__ ("stl $20, %0" : "=m" (newregs->regs[20]));
		__asm__ __volatile__ ("stl $21, %0" : "=m" (newregs->regs[21]));
		__asm__ __volatile__ ("stl $22, %0" : "=m" (newregs->regs[22]));
		__asm__ __volatile__ ("stl $23, %0" : "=m" (newregs->regs[23]));
		__asm__ __volatile__ ("stl $24, %0" : "=m" (newregs->regs[24]));
		__asm__ __volatile__ ("stl $25, %0" : "=m" (newregs->regs[25]));
		__asm__ __volatile__ ("stl $26, %0" : "=m" (newregs->regs[26]));
		__asm__ __volatile__ ("stl $27, %0" : "=m" (newregs->regs[27]));
		__asm__ __volatile__ ("stl $28, %0" : "=m" (newregs->regs[28]));
		__asm__ __volatile__ ("stl $29, %0" : "=m" (newregs->regs[29]));
		__asm__ __volatile__ ("stl $30, %0" : "=m" (newregs->regs[30]));
		newregs->pc = (unsigned long)current_text_addr();
	}
}

/* Function pointer to optional machine-specific reinitialization */
extern void (*kexec_reinit)(void);

extern void __init reserve_crashkernel(void);
extern void __init kexec_control_page_init(void);

#endif /* __ASSEMBLY__ */

struct kimage;
extern unsigned long kexec_args[4];

#else  /* CONFIG_KEXEC */

#ifndef __ASSEMBLY__
static inline void __init reserve_crashkernel(void) {}
static inline void __init kexec_control_page_init(void) {}
#endif

#endif /* CONFIG_KEXEC */

#endif /* _ASM_SW64_KEXEC_H */

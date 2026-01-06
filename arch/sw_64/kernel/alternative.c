#define pr_fmt(fmt) "alternatives: " fmt

#include <linux/cpu.h>
#include <linux/elf.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/stop_machine.h>

#include <asm/cacheflush.h>
#include <asm/alternative.h>
#include <asm/cpufeature.h>
#include <asm/insn.h>
#include <asm/module.h>
#include <asm/sections.h>
#include <asm/vdso.h>

#define MAX_PATCH_SIZE (((u8)(-1)) / SW64_INSN_SIZE)

extern struct alt_instr __alt_instructions[], __alt_instructions_end[];

/* Use this to add nops to a buffer, then text_poke the whole buffer. */
static void __init_or_module add_nops(u32 *insn, int count)
{
	while (count--) {
		*insn = SW64_NOP;
		insn++;
	}
}

/* Is the jump addr in local .altinstructions */
static inline bool in_alt_jump(unsigned long jump, void *start, void *end)
{
	return jump >= (unsigned long)start && jump < (unsigned long)end;
}

static void __init_or_module recompute_jump(u32 *buf, u32 *dest, u32 *src,
					void *start, void *end)
{
	unsigned long si_lo21, si_lo26, disp;
	unsigned long cur_pc, jump_addr, pc;

	cur_pc = (unsigned long)src;
	pc = (unsigned long)dest;

	si_lo21 = *src & 0x1fffff;
	si_lo26 = *src & 0x3ffffff;
	*buf = *src;

	if (sw64_insn_is_lbr(*src)) {
		jump_addr = cur_pc + SW64_INSN_SIZE * si_lo26;
		if (in_alt_jump(jump_addr, start, end))
			return;
		disp = (jump_addr - pc) / SW64_INSN_SIZE;
		*buf = (*buf & ~0x3ffffff) | disp;
	} else {
		jump_addr = cur_pc + SW64_INSN_SIZE * si_lo21;
		if (in_alt_jump(jump_addr, start, end))
			return;
		disp = (jump_addr - pc) / SW64_INSN_SIZE;
		*buf = (*buf & ~0x1fffff) | disp;
	}

	return;
}

/* Not support pcrel instruction at present! */
static int __init_or_module copy_alt_insns(u32 *buf,
	u32 *dest, u32 *src, int nr)
{
	int i;

	for (i = 0; i < nr; i++) {
		buf[i] = src[i];

		if (sw64_insn_is_branch(src[i])) {
			recompute_jump(&buf[i], &dest[i], &src[i], src, src + nr);
		}
	}

	return 0;
}

static void *__init_or_module text_poke_early(u32 *insn, u32 *buf, unsigned int nr)
{
	int i;
	unsigned long flags;

	local_irq_save(flags);

	for (i = 0; i < nr; i++)
		insn[i] = buf[i];

	local_irq_restore(flags);

	tbiv();

	return insn;
}

void __init_or_module apply_alternatives(struct alt_instr *start, struct alt_instr *end)
{
	struct alt_instr *a;
	unsigned int nr_instr, nr_repl, nr_insnbuf;
	u32 *instr, *replacement;
	u32 insnbuf[MAX_PATCH_SIZE];
	/*
	 * The scan order should be from start to end. A later scanned
	 * alternative code can overwrite previously scanned alternative code.
	 * Some kernel functions (e.g. memcpy, memset, etc) use this order to
	 * patch code.
	 *
	 * So be careful if you want to change the scan order to any other
	 * order.
	 */
	for (a = start; a < end; a++) {
		nr_insnbuf = 0;

		instr = (void *)&a->instr_offset + a->instr_offset;
		replacement = (void *)&a->replace_offset + a->replace_offset;

		nr_instr = a->instrlen / SW64_INSN_SIZE;
		nr_repl = a->replacementlen / SW64_INSN_SIZE;

		if (!cpus_have_cap(a->feature))
			continue;

		copy_alt_insns(insnbuf, instr, replacement, nr_repl);
		nr_insnbuf = nr_repl;

		if (nr_instr > nr_repl) {
			add_nops(insnbuf + nr_repl, nr_instr - nr_repl);
			nr_insnbuf += nr_instr - nr_repl;
		}

		text_poke_early(instr, insnbuf, nr_insnbuf);
	}
}

static void __init apply_vdso_alternatives(void)
{
	const Elf_Ehdr *hdr;
	const Elf_Shdr *shdr;
	const Elf_Shdr *alt;
	struct alt_instr *begin, *end;

	hdr = (Elf_Ehdr *)vdso_start;
	shdr = (void *)hdr + hdr->e_shoff;
	alt = find_section(hdr, shdr, ".altinstructions");
	if (!alt)
		return;

	begin = (void *)hdr + alt->sh_offset,
	end = (void *)hdr + alt->sh_offset + alt->sh_size,

	apply_alternatives((struct alt_instr *)begin,
			    (struct alt_instr *)end);
}

void __init apply_alternatives_all(void)
{
	apply_vdso_alternatives();

	apply_alternatives(__alt_instructions, __alt_instructions_end);
}

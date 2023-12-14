// SPDX-License-Identifier: GPL-2.0
/*
 * Modify History
 *
 * who		when		what
 * ---		----		----
 * stone	2004-09-02	Add SIMD floating emulation code
 * fire3        2008-12-27      Add SIMD floating emulation code for SW64
 */

#include <linux/uaccess.h>

#include <asm/ptrace.h>

#include "sfp-util.h"

#include <math-emu/soft-fp.h>
#include <math-emu/single.h>
#include <math-emu/double.h>

/*
 * This is for sw64
 */

#define SW64_FP_DENOMAL	1		/* A denormal data */
#define SW64_FP_NORMAL	0		/* A denormal data */
#define SW64_FP_NAN	2

#define SW64_FP_NAN_S(X, val)				\
do {							\
	union _FP_UNION_S *_flo =			\
		(union _FP_UNION_S *)(val);		\
							\
	X##_f = _flo->bits.frac;			\
	X##_e = _flo->bits.exp;				\
	X##_s = _flo->bits.sign;			\
							\
	switch (X##_e) {				\
	case 255:					\
		if (_FP_FRAC_ZEROP_1(X))		\
			X##_c = SW64_FP_NORMAL;		\
		else					\
			X##_c = SW64_FP_NAN;		\
		break;					\
	default:					\
		X##_c = SW64_FP_NORMAL;			\
		break;					\
	}						\
} while (0)


#define SW64_FP_NAN_D(X, val)				\
do {							\
	union _FP_UNION_D *_flo =			\
		(union _FP_UNION_D *)(val);		\
							\
	X##_f = _flo->bits.frac;			\
	X##_e = _flo->bits.exp;				\
	X##_s = _flo->bits.sign;			\
							\
	switch (X##_e) {				\
	case 2047:					\
		if (_FP_FRAC_ZEROP_1(X))		\
			X##_c = SW64_FP_NORMAL;		\
		else					\
			X##_c = SW64_FP_NAN;		\
		break;					\
	default:					\
		X##_c = SW64_FP_NORMAL;			\
		break;					\
	}						\
} while (0)



#define SW64_FP_NORMAL_S(X, val)			\
do {							\
	union _FP_UNION_S *_flo =			\
		(union _FP_UNION_S *)(val);		\
							\
	X##_f = _flo->bits.frac;			\
	X##_e = _flo->bits.exp;				\
	X##_s = _flo->bits.sign;			\
							\
	switch (X##_e) {				\
	case 0:						\
		if (_FP_FRAC_ZEROP_1(X))		\
			X##_c = SW64_FP_NORMAL;		\
		else					\
			X##_c = SW64_FP_DENOMAL;	\
		break;					\
	default:					\
		X##_c = SW64_FP_NORMAL;			\
		break;					\
	}						\
} while (0)

#define SW64_FP_NORMAL_D(X, val)			\
do {							\
	union _FP_UNION_D *_flo =			\
		(union _FP_UNION_D *)(val);		\
							\
	X##_f = _flo->bits.frac;			\
	X##_e = _flo->bits.exp;				\
	X##_s = _flo->bits.sign;			\
							\
	switch (X##_e) {				\
	case 0:						\
		if (_FP_FRAC_ZEROP_1(X))		\
			X##_c = SW64_FP_NORMAL;		\
		else					\
			X##_c = SW64_FP_DENOMAL;	\
		break;					\
	default:					\
		X##_c = SW64_FP_NORMAL;			\
		break;					\
	}						\
} while (0)

/* Operation Code for SW64 */
#define OP_SIMD_1	0x1A
#define OP_SIMD_2	0x1B
#define OP_SIMD_MUL_ADD	0x1B
#define OP_SIMD_NORMAL	0x1A
#define OP_MUL_ADD	0x19

#define FNC_FMAS	0x0
#define FNC_FMAD	0x1
#define FNC_FMSS	0x2
#define FNC_FMSD	0x3
#define FNC_FNMAS	0x4
#define FNC_FNMAD	0x5
#define FNC_FNMSS	0x6
#define FNC_FNMSD	0x7

#define FNC_VADDS	0x80
#define FNC_VADDD	0x81
#define FNC_VSUBS	0x82
#define FNC_VSUBD	0x83
#define FNC_VMULS	0x84
#define FNC_VMULD	0x85
#define FNC_VDIVS	0x86
#define FNC_VDIVD	0x87
#define FNC_VSQRTS	0x88
#define FNC_VSQRTD	0x89

#define FNC_VFCMPEQ	0x8c
#define FNC_VFCMPLE	0x8d
#define FNC_VFCMPLT	0x8e
#define FNC_VFCMPUN	0x8f

#define FNC_VCPYS	0x90
#define FNC_VCPYSE	0x91
#define FNC_VCPYSN	0x92

#define FNC_VMAS	0x0
#define FNC_VMAD	0x1
#define FNC_VMSS	0x2
#define FNC_VMSD	0x3
#define FNC_VNMAS	0x4
#define FNC_VNMAD	0x5
#define FNC_VNMSS	0x6
#define FNC_VNMSD	0x7

long simd_fp_emul_s(unsigned long pc);
long simd_fp_emul_d(unsigned long pc);
long mul_add_fp_emul(unsigned long pc);
long simd_cmp_emul_d(unsigned long pc);

long simd_mul_add_fp_emul_d(unsigned long pc);
long simd_mul_add_fp_emul_s(unsigned long pc);

void read_fp_reg_s(unsigned long reg, unsigned long *p0,
		unsigned long *p1, unsigned long *p2, unsigned long *p3);
void read_fp_reg_d(unsigned long reg, unsigned long *val_p0,
		unsigned long *p1, unsigned long *p2, unsigned long *p3);
void write_fp_reg_s(unsigned long reg, unsigned long val_p0,
		unsigned long p1, unsigned long p2, unsigned long p3);
void write_fp_reg_d(unsigned long reg, unsigned long val_p0,
		unsigned long p1, unsigned long p2, unsigned long p3);
#define LOW_64_WORKING	1
#define HIGH_64_WORKING	2

/*
 * End for sw64
 */

#define OPC_HMC		0x00
#define OPC_INTA	0x10
#define OPC_INTL	0x11
#define OPC_INTS	0x12
#define OPC_INTM	0x13
#define OPC_FLTC	0x14
#define OPC_FLTV	0x15
#define OPC_FLTI	0x16
#define OPC_FLTL	0x17
#define OPC_MISC	0x18
#define OPC_JSR		0x1a

#define FOP_SRC_S	0
#define FOP_SRC_T	2
#define FOP_SRC_Q	3

#define FOP_FNC_ADDx	0
#define FOP_FNC_CVTQL	0
#define FOP_FNC_SUBx	1
#define FOP_FNC_MULx	2
#define FOP_FNC_DIVx	3
#define FOP_FNC_CMPxUN	4
#define FOP_FNC_CMPxEQ	5
#define FOP_FNC_CMPxLT	6
#define FOP_FNC_CMPxLE	7
#define FOP_FNC_SQRTx	11
#define FOP_FNC_CVTxS	12
#define FOP_FNC_CVTxT	14
#define FOP_FNC_CVTxQ	15

/* this is for sw64 added by fire3*/
#define FOP_FNC_ADDS	0
#define FOP_FNC_ADDD	1
#define FOP_FNC_SUBS	2
#define FOP_FNC_SUBD	3
#define FOP_FNC_MULS	4
#define FOP_FNC_MULD	5
#define FOP_FNC_DIVS	6
#define FOP_FNC_DIVD	7
#define FOP_FNC_SQRTS	8
#define FOP_FNC_SQRTD	9

#define FOP_FNC_CMPEQ	0x10
#define FOP_FNC_CMPLE	0x11
#define FOP_FNC_CMPLT	0x12
#define FOP_FNC_CMPUN	0x13

#define FOP_FNC_CVTSD	0x20
#define FOP_FNC_CVTDS	0x21
#define FOP_FNC_CVTLS	0x2D
#define FOP_FNC_CVTLD	0x2F
#define FOP_FNC_CVTDL	0x27
#define FOP_FNC_CVTDL_G	0x22
#define FOP_FNC_CVTDL_P	0x23
#define FOP_FNC_CVTDL_Z	0x24
#define FOP_FNC_CVTDL_N	0x25

#define FOP_FNC_CVTWL	0x28
#define FOP_FNC_CVTLW	0x29

/* fire3 added end */


#define MISC_TRAPB	0x0000
#define MISC_EXCB	0x0400

extern unsigned long sw64_read_fp_reg(unsigned long reg);
extern void sw64_write_fp_reg(unsigned long reg, unsigned long val);
extern unsigned long sw64_read_fp_reg_s(unsigned long reg);
extern void sw64_write_fp_reg_s(unsigned long reg, unsigned long val);


#ifdef MODULE

MODULE_DESCRIPTION("FP Software completion module");

extern long (*sw64_fp_emul_imprecise)(struct pt_regs *regs, unsigned long write_mask);
extern long (*sw64_fp_emul)(unsigned long pc);

static long (*save_emul_imprecise)(struct pt_regs *regs, unsigned long write_mask);
static long (*save_emul)(unsigned long pc);

long do_sw_fp_emul_imprecise(struct pt_regs *regs, unsigned long write_mask);
long do_sw_fp_emul(unsigned long pc);

int init_module(void)
{
	save_emul_imprecise = sw64_fp_emul_imprecise;
	save_emul = sw64_fp_emul;
	sw64_fp_emul_imprecise = do_sw_fp_emul_imprecise;
	sw64_fp_emul = do_sw_fp_emul;
	return 0;
}

void cleanup_module(void)
{
	sw64_fp_emul_imprecise = save_emul_imprecise;
	sw64_fp_emul = save_emul;
}

#undef sw64_fp_emul_imprecise
#define sw64_fp_emul_imprecise		do_sw_fp_emul_imprecise
#undef sw64_fp_emul
#define sw64_fp_emul			do_sw_fp_emul

#endif /* MODULE */


/*
 * Emulate the floating point instruction at address PC.  Returns -1 if the
 * instruction to be emulated is illegal (such as with the opDEC trap), else
 * the SI_CODE for a SIGFPE signal, else 0 if everything's ok.
 *
 * Notice that the kernel does not and cannot use FP regs.  This is good
 * because it means that instead of saving/restoring all fp regs, we simply
 * stick the result of the operation into the appropriate register.
 */
long sw64_fp_emul(unsigned long pc)
{
	FP_DECL_EX;
	FP_DECL_S(SA); FP_DECL_S(SB); FP_DECL_S(SR);
	FP_DECL_D(DA); FP_DECL_D(DB); FP_DECL_D(DR);

	unsigned long fa, fb, fc, func, mode, mode_bk, src;
	unsigned long res, va, vb, vc, swcr, fpcr;
	__u32 insn;
	long si_code;
	unsigned long opcode;

	get_user(insn, (__u32 *)pc);
	opcode = (insn >> 26) & 0x3f;
	fc     = (insn >>  0) & 0x1f;	/* destination register */
	fb     = (insn >> 16) & 0x1f;
	fa     = (insn >> 21) & 0x1f;
	func   = (insn >>  5) & 0xff;
	fpcr = rdfpcr();
	mode   = (fpcr >> FPCR_DYN_SHIFT) & 0x3;
	pr_debug("======= Entering Floating mathe emulation =====\n");
	pr_debug("Floating math emulation insn = %#lx, opcode=%d, func=%d\n", insn, opcode, func);
	pr_debug("SW64 hardware fpcr = %#lx\n", fpcr);
	swcr = swcr_update_status(current_thread_info()->ieee_state, fpcr);
	pr_debug("SW64 software swcr = %#lx\n", swcr);
	pr_debug("fa:%#lx,fb:%#lx,fc:%#lx,func:%#lx,mode:%#lx\n", fa, fb, fc, func, mode);

	if (opcode == OP_SIMD_NORMAL) { /* float simd math  */
		if (func == FNC_VADDS || func == FNC_VSUBS  || func == FNC_VSQRTS
				|| func == FNC_VMULS || func == FNC_VDIVS)
			si_code = simd_fp_emul_s(pc);
		if (func == FNC_VADDD || func == FNC_VSUBD  || func == FNC_VSQRTD
				|| func == FNC_VMULD || func == FNC_VDIVD)
			si_code = simd_fp_emul_d(pc);
		if (func == FNC_VFCMPUN || func == FNC_VFCMPLT  || func == FNC_VFCMPLE
				|| func == FNC_VFCMPEQ)
			si_code = simd_cmp_emul_d(pc);
		return si_code;
	}
	if (opcode == OP_SIMD_MUL_ADD) {/* simd mul and add */
		func = (insn >> 10) & 0x3f;
		if (func == FNC_VMAS || func == FNC_VMSS || func == FNC_VNMAS
			|| func == FNC_VNMSS) {
			si_code = simd_mul_add_fp_emul_s(pc);
			return si_code;
		}

		if (func == FNC_VMAD || func == FNC_VMSD || func == FNC_VNMAD
			|| func == FNC_VNMSD) {
			si_code = simd_mul_add_fp_emul_d(pc);
			return si_code;
		}
		func = (insn >>  5) & 0xff;
	}

	if (opcode == OP_MUL_ADD) {
		si_code = mul_add_fp_emul(pc);
		return si_code;
	}
	switch (func) {
	case FOP_FNC_SUBS:
		va = sw64_read_fp_reg_s(fa);
		vb = sw64_read_fp_reg_s(fb);
		FP_UNPACK_SP(SA, &va);
		FP_UNPACK_SP(SB, &vb);
		FP_SUB_S(SR, SA, SB);
		goto pack_s;

	case FOP_FNC_SUBD:
		va = sw64_read_fp_reg(fa);
		vb = sw64_read_fp_reg(fb);
		FP_UNPACK_DP(DA, &va);
		FP_UNPACK_DP(DB, &vb);
		FP_SUB_D(DR, DA, DB);
		goto pack_d;

	case FOP_FNC_ADDS:
		va = sw64_read_fp_reg_s(fa);
		vb = sw64_read_fp_reg_s(fb);
		FP_UNPACK_SP(SA, &va);
		FP_UNPACK_SP(SB, &vb);
		FP_ADD_S(SR, SA, SB);
		goto pack_s;

	case FOP_FNC_ADDD:
		va = sw64_read_fp_reg(fa);
		vb = sw64_read_fp_reg(fb);
		FP_UNPACK_DP(DA, &va);
		FP_UNPACK_DP(DB, &vb);
		FP_ADD_D(DR, DA, DB);
		goto pack_d;

	case FOP_FNC_MULS:
		va = sw64_read_fp_reg_s(fa);
		vb = sw64_read_fp_reg_s(fb);
		FP_UNPACK_SP(SA, &va);
		FP_UNPACK_SP(SB, &vb);
		FP_MUL_S(SR, SA, SB);
		goto pack_s;

	case FOP_FNC_MULD:
		va = sw64_read_fp_reg(fa);
		vb = sw64_read_fp_reg(fb);
		FP_UNPACK_DP(DA, &va);
		FP_UNPACK_DP(DB, &vb);
		FP_MUL_D(DR, DA, DB);
		goto pack_d;

	case FOP_FNC_DIVS:
		pr_debug("FOP_FNC_DIVS\n");
		va = sw64_read_fp_reg_s(fa);
		vb = sw64_read_fp_reg_s(fb);
		FP_UNPACK_SP(SA, &va);
		FP_UNPACK_SP(SB, &vb);
		FP_DIV_S(SR, SA, SB);
		goto pack_s;

	case FOP_FNC_DIVD:
		pr_debug("FOP_FNC_DIVD\n");
		va = sw64_read_fp_reg(fa);
		vb = sw64_read_fp_reg(fb);
		FP_UNPACK_DP(DA, &va);
		FP_UNPACK_DP(DB, &vb);
		FP_DIV_D(DR, DA, DB);
		goto pack_d;

	case FOP_FNC_SQRTS:
		va = sw64_read_fp_reg_s(fa);
		vb = sw64_read_fp_reg_s(fb);
		FP_UNPACK_SP(SA, &va);
		FP_UNPACK_SP(SB, &vb);
		FP_SQRT_S(SR, SB);
		goto pack_s;
	case FOP_FNC_SQRTD:
		va = sw64_read_fp_reg(fa);
		vb = sw64_read_fp_reg(fb);
		FP_UNPACK_DP(DA, &va);
		FP_UNPACK_DP(DB, &vb);
		FP_SQRT_D(DR, DB);
		goto pack_d;
	}


	va = sw64_read_fp_reg(fa);
	vb = sw64_read_fp_reg(fb);
	if ((func & ~0xf) == FOP_FNC_CMPEQ) {
		va = sw64_read_fp_reg(fa);
		vb = sw64_read_fp_reg(fb);

		FP_UNPACK_RAW_DP(DA, &va);
		FP_UNPACK_RAW_DP(DB, &vb);
		if (!DA_e && !_FP_FRAC_ZEROP_1(DA)) {
			FP_SET_EXCEPTION(FP_EX_DENORM);
			if (FP_DENORM_ZERO)
				_FP_FRAC_SET_1(DA, _FP_ZEROFRAC_1);
		}
		if (!DB_e && !_FP_FRAC_ZEROP_1(DB)) {
			FP_SET_EXCEPTION(FP_EX_DENORM);
			if (FP_DENORM_ZERO)
				_FP_FRAC_SET_1(DB, _FP_ZEROFRAC_1);
		}
		FP_CMP_D(res, DA, DB, 3);
		vc = 0x4000000000000000;
		/* CMPTEQ, CMPTUN don't trap on QNaN, while CMPTLT and CMPTLE do */
		if (res == 3 && (((func == FOP_FNC_CMPLT) || (func == FOP_FNC_CMPLE))
					|| FP_ISSIGNAN_D(DA) || FP_ISSIGNAN_D(DB))) {
			pr_debug("CMPLT CMPLE:func:%d, trap on QNaN.", func);
			FP_SET_EXCEPTION(FP_EX_INVALID);
		}
		switch (func) {
		case FOP_FNC_CMPUN:
			if (res != 3)
				vc = 0;
			break;
		case FOP_FNC_CMPEQ:
			if (res)
				vc = 0;
			break;
		case FOP_FNC_CMPLT:
			if (res != -1)
				vc = 0;
			break;
		case FOP_FNC_CMPLE:
			if ((long)res > 0)
				vc = 0;
			break;
		}
		goto done_d;
	}
	FP_UNPACK_DP(DA, &va);
	FP_UNPACK_DP(DB, &vb);

	if (func == FOP_FNC_CVTSD) {
		vb = sw64_read_fp_reg_s(fb);
		FP_UNPACK_SP(SB, &vb);
		DR_c = DB_c;
		DR_s = DB_s;
		DR_e = DB_e + (1024 - 128);
		DR_f = SB_f << (52 - 23);
		goto pack_d;
	}

	if (func == FOP_FNC_CVTDS) {
		FP_CONV(S, D, 1, 1, SR, DB);
		goto pack_s;
	}

	if (func == FOP_FNC_CVTDL || func == FOP_FNC_CVTDL_G || func == FOP_FNC_CVTDL_P
			|| func == FOP_FNC_CVTDL_Z || func == FOP_FNC_CVTDL_N) {
		mode_bk = mode;
		if (func == FOP_FNC_CVTDL_Z)
			mode = 0x0UL;
		else if (func == FOP_FNC_CVTDL_N)
			mode = 0x1UL;
		else if (func == FOP_FNC_CVTDL_G)
			mode = 0x2UL;
		else if (func == FOP_FNC_CVTDL_P)
			mode = 0x3UL;

		if (DB_c == FP_CLS_NAN && (_FP_FRAC_HIGH_RAW_D(DB) & _FP_QNANBIT_D)) {
			/* AAHB Table B-2 says QNaN should not trigger INV */
			vc = 0;
		} else
			FP_TO_INT_ROUND_D(vc, DB, 64, 2);
		mode = mode_bk;
		goto done_d;
	}

	vb = sw64_read_fp_reg(fb);

	switch (func) {
	case FOP_FNC_CVTLW:
		/*
		 * Notice: We can get here only due to an integer
		 * overflow.  Such overflows are reported as invalid
		 * ops.  We return the result the hw would have
		 * computed.
		 */
		vc = ((vb & 0xc0000000) << 32 |	/* sign and msb */
				(vb & 0x3fffffff) << 29);	/* rest of the int */
		FP_SET_EXCEPTION(FP_EX_INVALID);
		goto done_d;

	case FOP_FNC_CVTLS:
		FP_FROM_INT_S(SR, ((long)vb), 64, long);
		goto pack_s;

	case FOP_FNC_CVTLD:
		FP_FROM_INT_D(DR, ((long)vb), 64, long);
		goto pack_d;
	}
	goto bad_insn;


pack_s:
	FP_PACK_SP(&vc, SR);

	if ((_fex & FP_EX_UNDERFLOW) && (swcr & IEEE_MAP_UMZ))
		vc = 0;
	pr_debug("SW64 Emulation S-floating _fex=%#lx, va=%#lx, vb=%#lx, vc=%#lx\n", _fex, va, vb, vc);
	pr_debug("SW64 Emulation S-floating mode=%#lx,func=%#lx, swcr=%#lx\n", mode, func, swcr);
	sw64_write_fp_reg_s(fc, vc);
	goto done;

pack_d:
	FP_PACK_DP(&vc, DR);
	if ((_fex & FP_EX_UNDERFLOW) && (swcr & IEEE_MAP_UMZ))
		vc = 0;
	pr_debug("SW64 Emulation D-floating _fex=%#lx, va=%#lx, vb=%#lx, vc=%#lx\n", _fex, va, vb, vc);
	pr_debug("SW64 Emulation D-floating mode=%#lx,func=%#lx, swcr=%#lx\n", mode, func, swcr);
done_d:
	sw64_write_fp_reg(fc, vc);
	goto done;

	/*
	 * Take the appropriate action for each possible
	 * floating-point result:
	 *
	 *	- Set the appropriate bits in the FPCR
	 *	- If the specified exception is enabled in the FPCR,
	 *	  return.  The caller (entArith) will dispatch
	 *	  the appropriate signal to the translated program.
	 *
	 * In addition, properly track the exception state in software
	 * as described in the SW64 Architecture Handbook section 4.7.7.3.
	 */
done:
	if (_fex) {
		/* Record exceptions in software control word.  */
		swcr |= fex_to_swcr_status(_fex, 0);
		current_thread_info()->ieee_state
			|= fex_to_swcr_status(_fex, 0);

		/* Update hardware control register.  */
		fpcr &= (~FPCR_MASK | FPCR_DYN_MASK);
		fpcr |= ieee_swcr_to_fpcr(swcr);
		pr_debug("SW64 before write fpcr = %#lx\n", fpcr);
		wrfpcr(fpcr);

		/* Do we generate a signal?  */
		_fex = _fex & swcr & IEEE_TRAP_ENABLE_MASK;
		si_code = 0;
		if (_fex) {
			if (_fex & IEEE_TRAP_ENABLE_DNO)
				si_code = FPE_FLTUND;
			if (_fex & IEEE_TRAP_ENABLE_INE)
				si_code = FPE_FLTRES;
			if (_fex & IEEE_TRAP_ENABLE_UNF)
				si_code = FPE_FLTUND;
			if (_fex & IEEE_TRAP_ENABLE_OVF)
				si_code = FPE_FLTOVF;
			if (_fex & IEEE_TRAP_ENABLE_DZE)
				si_code = FPE_FLTDIV;
			if (_fex & IEEE_TRAP_ENABLE_INV)
				si_code = FPE_FLTINV;
		}

		return si_code;
	}

	/*
	 * We used to write the destination register here, but DEC FORTRAN
	 * requires that the result *always* be written... so we do the write
	 * immediately after the operations above.
	 */

	return 0;

bad_insn:
	pr_err("%s: Invalid FP insn %#x at %#lx\n", __func__, insn, pc);
	return -1;
}

long sw64_fp_emul_imprecise(struct pt_regs *regs, unsigned long write_mask)
{
	unsigned long trigger_pc = regs->pc - 4;
	unsigned long insn, opcode, rc, si_code = 0;


	/*
	 * Turn off the bits corresponding to registers that are the
	 * target of instructions that set bits in the exception
	 * summary register.  We have some slack doing this because a
	 * register that is the target of a trapping instruction can
	 * be written at most once in the trap shadow.
	 *
	 * Branches, jumps, TRAPBs, EXCBs and calls to HMcode all
	 * bound the trap shadow, so we need not look any further than
	 * up to the first occurrence of such an instruction.
	 */
	while (write_mask) {
		get_user(insn, (__u32 *)(trigger_pc));
		opcode = insn >> 26;
		rc = insn & 0x1f;

		switch (opcode) {
		case OPC_HMC:
		case OPC_JSR:
		case 0x30 ... 0x3f:	/* branches */
			goto egress;

		case OPC_MISC:
		switch (insn & 0xffff) {
		case MISC_TRAPB:
		case MISC_EXCB:
			goto egress;

		default:
			break;
			}
		break;

		case OPC_INTA:
		case OPC_INTL:
		case OPC_INTS:
		case OPC_INTM:
			write_mask &= ~(1UL << rc);
			break;

		case OPC_FLTC:
		case OPC_FLTV:
		case OPC_FLTI:
		case OPC_FLTL:
			write_mask &= ~(1UL << (rc + 32));
			break;
		}
		if (!write_mask) {
			/* Re-execute insns in the trap-shadow.  */
			regs->pc = trigger_pc + 4;
			si_code = sw64_fp_emul(trigger_pc);
			goto egress;
		}
		trigger_pc -= 4;
	}

egress:
	return si_code;
}

#define WORKING_PART_0 0
#define WORKING_PART_1 1
#define WORKING_PART_2 2
#define WORKING_PART_3 3


/*
 * This is for sw64
 */

long simd_cmp_emul_d(unsigned long pc)
{
	FP_DECL_EX;
	FP_DECL_D(DA); FP_DECL_D(DB); FP_DECL_D(DR); FP_DECL_D(DC);
	unsigned long fa, fb, fc, func, mode, src;
	unsigned long res, va, vb, vc, swcr, fpcr;
	__u32 insn;
	long si_code;

	unsigned long va_p0, va_p1, va_p2, va_p3;
	unsigned long vb_p0, vb_p1, vb_p2, vb_p3;
	unsigned long vc_p0, vc_p1, vc_p2, vc_p3;
	unsigned long fex_p0, fex_p1, fex_p2, fex_p3;

	int working_part;

	get_user(insn, (__u32 *)pc);
	fc     = (insn >>  0) & 0x1f;	/* destination register */
	fb     = (insn >> 16) & 0x1f;
	fa     = (insn >> 21) & 0x1f;
	func   = (insn >>  5) & 0xff;
	fpcr = rdfpcr();
	mode   = (fpcr >> FPCR_DYN_SHIFT) & 0x3;

	pr_debug("======== Entering SIMD floating-CMP math emulation =======\n");
	pr_debug("hardware fpcr = %#lx\n", fpcr);
	swcr = swcr_update_status(current_thread_info()->ieee_state, fpcr);
	pr_debug("software swcr = %#lx\n", swcr);
	pr_debug("fa:%#lx,fb:%#lx,fc:%#lx,func:%#lx,mode:%#lx\n", fa, fb, fc, func, mode);
	read_fp_reg_d(fa, &va_p0, &va_p1, &va_p2, &va_p3);
	read_fp_reg_d(fb, &vb_p0, &vb_p1, &vb_p2, &vb_p3);
	read_fp_reg_d(fc, &vc_p0, &vc_p1, &vc_p2, &vc_p3);
	pr_debug("va_p0:%#lx, va_p1:%#lx, va_p2:%#lx, va_p3:%#lx\n", va_p0, va_p1, va_p2, va_p3);
	pr_debug("vb_p0:%#lx, vb_p1:%#lx, vb_p2:%#lx, vb_p3:%#lx\n", vb_p0, vb_p1, vb_p2, vb_p3);
	pr_debug("vc_p0:%#lx, vc_p1:%#lx, vc_p2:%#lx, vc_p3:%#lx\n", vc_p0, vc_p1, vc_p2, vc_p3);
	working_part = WORKING_PART_0;
simd_working:
	_fex = 0;
	switch (working_part) {
	case WORKING_PART_0:
		pr_debug("WORKING_PART_0\n");
		va = va_p0;
		vb = vb_p0;
		vc = vc_p0;
		break;
	case WORKING_PART_1:
		pr_debug("WORKING_PART_1\n");
		va = va_p1;
		vb = vb_p1;
		vc = vc_p1;
		break;
	case WORKING_PART_2:
		pr_debug("WORKING_PART_2\n");
		va = va_p2;
		vb = vb_p2;
		vc = vc_p2;
		break;
	case WORKING_PART_3:
		pr_debug("WORKING_PART_3\n");
		va = va_p3;
		vb = vb_p3;
		vc = vc_p3;
		break;
	}
	pr_debug("Before unpack va:%#lx, vb:%#lx\n", va, vb);
	FP_UNPACK_RAW_DP(DA, &va);
	FP_UNPACK_RAW_DP(DB, &vb);
	pr_debug("DA_e:%d, _FP_FRAC_ZEROP_1(DA):%d\n", DA_e, _FP_FRAC_ZEROP_1(DA));
	pr_debug("DB_e:%d, _FP_FRAC_ZEROP_1(DB):%d\n", DA_e, _FP_FRAC_ZEROP_1(DA));
	pr_debug("DA iszero:%d, DB iszero:%d\n", ((!DA_e && _FP_FRAC_ZEROP_1(DA)) ? 1 : 0),
			((!DB_e && _FP_FRAC_ZEROP_1(DB))));
	if (!DA_e && !_FP_FRAC_ZEROP_1(DA)) {
		FP_SET_EXCEPTION(FP_EX_DENORM);
		if (FP_DENORM_ZERO)
			_FP_FRAC_SET_1(DA, _FP_ZEROFRAC_1);
	}
	if (!DB_e && !_FP_FRAC_ZEROP_1(DB)) {
		FP_SET_EXCEPTION(FP_EX_DENORM);
		if (FP_DENORM_ZERO)
			_FP_FRAC_SET_1(DB, _FP_ZEROFRAC_1);
	}
	FP_CMP_D(res, DA, DB, 3);
	vc = 0x4000000000000000;
	/* CMPTEQ, CMPTUN don't trap on QNaN, while CMPTLT and CMPTLE do */
	if (res == 3 && (((func == FOP_FNC_CMPLT) || (func == FOP_FNC_CMPLE))
				|| FP_ISSIGNAN_D(DA) || FP_ISSIGNAN_D(DB))) {
		pr_debug("CMPLT CMPLE:func:%d, trap on QNaN.", func);
		FP_SET_EXCEPTION(FP_EX_INVALID);
	}
	pr_debug("res:%d\n", res);
	switch (func) {
	case FNC_VFCMPUN:
		if (res != 3)
			vc = 0;
		break;
	case FNC_VFCMPEQ:
		if (res)
			vc = 0;
		break;
	case FNC_VFCMPLT:
		if (res != -1)
			vc = 0;
		break;
	case FNC_VFCMPLE:
		if ((long)res > 0)
			vc = 0;
		break;
	}
next_working_s:
	switch (working_part) {
	case WORKING_PART_0:
		working_part = WORKING_PART_1;
		vc_p0 = vc;
		fex_p0 = _fex;
		goto simd_working;
	case WORKING_PART_1:
		working_part = WORKING_PART_2;
		vc_p1 = vc;
		fex_p1 = _fex;
		goto simd_working;
	case WORKING_PART_2:
		working_part = WORKING_PART_3;
		vc_p2 = vc;
		fex_p2 = _fex;
		goto simd_working;
	case WORKING_PART_3:
		vc_p3 = vc;
		fex_p3 = _fex;
		goto done;
	}
done:
	if (fex_p0 || fex_p1 || fex_p2 || fex_p3) {
		unsigned long fpcr_p0, fpcr_p1, fpcr_p2, fpcr_p3;
		unsigned long swcr_p0, swcr_p1, swcr_p2, swcr_p3;

		fpcr_p0 = fpcr_p1 = fpcr_p2 = fpcr_p3 = 0;
		swcr_p0 = swcr_p1 = swcr_p2 = swcr_p3 = swcr;
		/* manage fpcr_p0 */
		if (fex_p0) {
			swcr_p0 |= fex_to_swcr_status(fex_p0, 0);
			current_thread_info()->ieee_state
				|= fex_to_swcr_status(fex_p0, 0);

			/* Update hardware control register.  */
			fpcr_p0 = fpcr;
			fpcr_p0 &= (~FPCR_MASK | FPCR_DYN_MASK);
			fpcr_p0 |= ieee_swcr_to_fpcr(swcr_p0);
		}

		if (fex_p1) {
			swcr_p1 |= fex_to_swcr_status(fex_p1, 1);
			current_thread_info()->ieee_state
				|= fex_to_swcr_status(fex_p1, 1);

			/* Update hardware control register.  */
			fpcr_p1 = fpcr;
			fpcr_p1 &= (~FPCR_MASK | FPCR_DYN_MASK);
			fpcr_p1 |= ieee_swcr_to_fpcr(swcr_p1);
		}

		if (fex_p2) {
			swcr_p2 |= fex_to_swcr_status(fex_p2, 2);
			current_thread_info()->ieee_state
				|= fex_to_swcr_status(fex_p2, 2);

			/* Update hardware control register.  */
			fpcr_p2 = fpcr;
			fpcr_p2 &= (~FPCR_MASK | FPCR_DYN_MASK);
			fpcr_p2 |= ieee_swcr_to_fpcr(swcr_p2);
		}

		if (fex_p3) {
			swcr_p3 |= fex_to_swcr_status(fex_p3, 3);
			current_thread_info()->ieee_state
				|= fex_to_swcr_status(fex_p3, 3);

			/* Update hardware control register.  */
			fpcr_p3 = fpcr;
			fpcr_p3 &= (~FPCR_MASK | FPCR_DYN_MASK);
			fpcr_p3 |= ieee_swcr_to_fpcr(swcr_p3);
		}

		fpcr = fpcr_p0 | fpcr_p1 | fpcr_p2 | fpcr_p3;
		pr_debug("fex_p0 = %#lx\n", fex_p0);
		pr_debug("fex_p1 = %#lx\n", fex_p1);
		pr_debug("fex_p2 = %#lx\n", fex_p2);
		pr_debug("fex_p3 = %#lx\n", fex_p3);
		pr_debug("SIMD emulation almost finished.before write fpcr = %#lx\n", fpcr);
		wrfpcr(fpcr);
		pr_debug("Before write fp: vc_p0=%#lx, vc_p1=%#lx, vc_p2=%#lx, vc_p3=%#lx\n", vc_p0, vc_p1, vc_p2, vc_p3);
		write_fp_reg_d(fc, vc_p0, vc_p1, vc_p2, vc_p3);

		/* Do we generate a signal?  */
		_fex = (fex_p0 & swcr & IEEE_TRAP_ENABLE_MASK) | (fex_p1 & swcr & IEEE_TRAP_ENABLE_MASK)
			| (fex_p2 & swcr & IEEE_TRAP_ENABLE_MASK) | (fex_p3 & swcr & IEEE_TRAP_ENABLE_MASK);
		si_code = 0;
		if (_fex) {
			if (_fex & IEEE_TRAP_ENABLE_DNO)
				si_code = FPE_FLTUND;
			if (_fex & IEEE_TRAP_ENABLE_INE)
				si_code = FPE_FLTRES;
			if (_fex & IEEE_TRAP_ENABLE_UNF)
				si_code = FPE_FLTUND;
			if (_fex & IEEE_TRAP_ENABLE_OVF)
				si_code = FPE_FLTOVF;
			if (_fex & IEEE_TRAP_ENABLE_DZE)
				si_code = FPE_FLTDIV;
			if (_fex & IEEE_TRAP_ENABLE_INV)
				si_code = FPE_FLTINV;
		}
		pr_debug("SIMD finished.. si_code:%#lx\n", si_code);
		return si_code;

	}
	pr_debug("SIMD finished.. si_code:%#lx\n", si_code);
	return 0;

bad_insn:
	pr_err("%s: Invalid FP insn %#x at %#lx\n", __func__, insn, pc);
	return -1;
}


long simd_fp_emul_d(unsigned long pc)
{
	FP_DECL_EX;
	FP_DECL_D(DA); FP_DECL_D(DB); FP_DECL_D(DR); FP_DECL_D(DC);
	unsigned long fa, fb, fc, func, mode, src;
	unsigned long res, va, vb, vc, swcr, fpcr;
	__u32 insn;
	long si_code;

	unsigned long va_p0, va_p1, va_p2, va_p3;
	unsigned long vb_p0, vb_p1, vb_p2, vb_p3;
	unsigned long vc_p0, vc_p1, vc_p2, vc_p3;
	unsigned long fex_p0, fex_p1, fex_p2, fex_p3;

	int working_part;

	get_user(insn, (__u32 *)pc);
	fc     = (insn >>  0) & 0x1f;	/* destination register */
	fb     = (insn >> 16) & 0x1f;
	fa     = (insn >> 21) & 0x1f;
	func   = (insn >>  5) & 0xff;
	fpcr = rdfpcr();
	mode   = (fpcr >> FPCR_DYN_SHIFT) & 0x3;

	pr_debug("======== Entering SIMD D-floating math emulation =======\n");
	pr_debug("hardware fpcr = %#lx\n", fpcr);
	swcr = swcr_update_status(current_thread_info()->ieee_state, fpcr);
	pr_debug("software swcr = %#lx\n", swcr);
	pr_debug("fa:%#lx,fb:%#lx,fc:%#lx,func:%#lx,mode:%#lx\n", fa, fb, fc, func, mode);
	read_fp_reg_d(fa, &va_p0, &va_p1, &va_p2, &va_p3);
	read_fp_reg_d(fb, &vb_p0, &vb_p1, &vb_p2, &vb_p3);
	read_fp_reg_d(fc, &vc_p0, &vc_p1, &vc_p2, &vc_p3);
	pr_debug("va_p0:%#lx, va_p1:%#lx, va_p2:%#lx, va_p3:%#lx\n", va_p0, va_p1, va_p2, va_p3);
	pr_debug("vb_p0:%#lx, vb_p1:%#lx, vb_p2:%#lx, vb_p3:%#lx\n", vb_p0, vb_p1, vb_p2, vb_p3);
	pr_debug("vc_p0:%#lx, vc_p1:%#lx, vc_p2:%#lx, vc_p3:%#lx\n", vc_p0, vc_p1, vc_p2, vc_p3);
	working_part = WORKING_PART_0;
simd_working:
	_fex = 0;
	switch (working_part) {
	case WORKING_PART_0:
		pr_debug("WORKING_PART_0\n");
		va = va_p0;
		vb = vb_p0;
		vc = vc_p0;
		if ((fpcr & FPCR_STATUS_MASK0) == 0) {
			SW64_FP_NORMAL_D(DA, &va);
			SW64_FP_NORMAL_D(DB, &vb);
			if ((DA_c == SW64_FP_NORMAL) && (DB_c == SW64_FP_NORMAL))
				goto next_working_s;
			else
				pr_debug("LOW: DA_c = %#lx, DB_c = %#lx\n", DA_c, DB_c);
		} else {
			SW64_FP_NAN_D(DA, &va);
			SW64_FP_NAN_D(DB, &vb);
			if (((DA_c == SW64_FP_NAN) || (DB_c == SW64_FP_NAN)))
				goto next_working_s;
		}
		break;
	case WORKING_PART_1:
		pr_debug("WORKING_PART_1\n");
		va = va_p1;
		vb = vb_p1;
		vc = vc_p1;
		if ((fpcr & FPCR_STATUS_MASK1) == 0) {
			SW64_FP_NORMAL_D(DA, &va);
			SW64_FP_NORMAL_D(DB, &vb);
			if ((DA_c == SW64_FP_NORMAL) && (DB_c == SW64_FP_NORMAL))
				goto next_working_s;
			else
				pr_debug("HIGH: DA_c = %#lx, DB_c = %#lx\n", DA_c, DB_c);
		} else {
			SW64_FP_NAN_D(DA, &va);
			SW64_FP_NAN_D(DB, &vb);
			if (((DA_c == SW64_FP_NAN) || (DB_c == SW64_FP_NAN)))
				goto next_working_s;
		}

		break;
	case WORKING_PART_2:
		pr_debug("WORKING_PART_2\n");
		va = va_p2;
		vb = vb_p2;
		vc = vc_p2;
		if ((fpcr & FPCR_STATUS_MASK2) == 0) {
			SW64_FP_NORMAL_D(DA, &va);
			SW64_FP_NORMAL_D(DB, &vb);
			if ((DA_c == SW64_FP_NORMAL) && (DB_c == SW64_FP_NORMAL))
				goto next_working_s;
			else
				pr_debug("HIGH: DA_c = %#lx, DB_c = %#lx\n", DA_c, DB_c);
		} else {
			SW64_FP_NAN_D(DA, &va);
			SW64_FP_NAN_D(DB, &vb);
			if (((DA_c == SW64_FP_NAN) || (DB_c == SW64_FP_NAN)))
				goto next_working_s;
		}
		break;
	case WORKING_PART_3:
		pr_debug("WORKING_PART_3\n");
		va = va_p3;
		vb = vb_p3;
		vc = vc_p3;
		if ((fpcr & FPCR_STATUS_MASK3) == 0) {
			SW64_FP_NORMAL_D(DA, &va);
			SW64_FP_NORMAL_D(DB, &vb);
			if ((DA_c == SW64_FP_NORMAL) && (DB_c == SW64_FP_NORMAL))
				goto next_working_s;
			else
				pr_debug("HIGH: DA_c = %#lx, DB_c = %#lx\n", DA_c, DB_c);
		} else {
			SW64_FP_NAN_D(DA, &va);
			SW64_FP_NAN_D(DB, &vb);
			if (((DA_c == SW64_FP_NAN) || (DB_c == SW64_FP_NAN)))
				goto next_working_s;
		}
		break;
	}

	FP_UNPACK_DP(DA, &va);
	FP_UNPACK_DP(DB, &vb);

	switch (func) {
	case FNC_VSUBD:
		pr_debug("FNC_VSUBD\n");
		FP_SUB_D(DR, DA, DB);
		goto pack_d;
	case FNC_VMULD:
		pr_debug("FNC_VMULD\n");
		FP_MUL_D(DR, DA, DB);
		goto pack_d;
	case FNC_VADDD:
		pr_debug("FNC_VADDD\n");
		FP_ADD_D(DR, DA, DB);
		goto pack_d;
	case FNC_VDIVD:
		pr_debug("FNC_VDIVD\n");
		FP_DIV_D(DR, DA, DB);
		goto pack_d;
	case FNC_VSQRTD:
		pr_debug("FNC_VSQRTD\n");
		FP_SQRT_D(DR, DB);
		goto pack_d;
	}
pack_d:
	FP_PACK_DP(&vc, DR);
	if ((_fex & FP_EX_UNDERFLOW) && (swcr & IEEE_MAP_UMZ)) {
		pr_debug("pack_d, vc=0 !!!!\n");
		vc = 0;
	}

	pr_debug("SW64 SIMD Emulation D-floating _fex=%#lx, va=%#lx, vb=%#lx, vc=%#lx\n", _fex, va, vb, vc);
	pr_debug("SW64 SIMD Emulation D-floating mode=%#lx,func=%#lx, swcr=%#lx\n", mode, func, swcr);
next_working_s:
	switch (working_part) {
	case WORKING_PART_0:
		working_part = WORKING_PART_1;
		vc_p0 = vc;
		fex_p0 = _fex;
		goto simd_working;
	case WORKING_PART_1:
		working_part = WORKING_PART_2;
		vc_p1 = vc;
		fex_p1 = _fex;
		goto simd_working;
	case WORKING_PART_2:
		working_part = WORKING_PART_3;
		vc_p2 = vc;
		fex_p2 = _fex;
		goto simd_working;
	case WORKING_PART_3:
		vc_p3 = vc;
		fex_p3 = _fex;
		goto done;
	}
done:
	if (fex_p0 || fex_p1 || fex_p2 || fex_p3) {
		unsigned long fpcr_p0, fpcr_p1, fpcr_p2, fpcr_p3;
		unsigned long swcr_p0, swcr_p1, swcr_p2, swcr_p3;

		fpcr_p0 = fpcr_p1 = fpcr_p2 = fpcr_p3 = 0;
		swcr_p0 = swcr_p1 = swcr_p2 = swcr_p3 = swcr;
		/* manage fpcr_p0 */
		if (fex_p0) {
			swcr_p0 |= fex_to_swcr_status(fex_p0, 0);
			current_thread_info()->ieee_state
				|= fex_to_swcr_status(fex_p0, 0);

			/* Update hardware control register.  */
			fpcr_p0 = fpcr;
			fpcr_p0 &= (~FPCR_MASK | FPCR_DYN_MASK);
			fpcr_p0 |= ieee_swcr_to_fpcr(swcr_p0);
		}

		if (fex_p1) {
			swcr_p1 |= fex_to_swcr_status(fex_p1, 1);
			current_thread_info()->ieee_state
				|= fex_to_swcr_status(fex_p1, 1);

			/* Update hardware control register.  */
			fpcr_p1 = fpcr;
			fpcr_p1 &= (~FPCR_MASK | FPCR_DYN_MASK);
			fpcr_p1 |= ieee_swcr_to_fpcr(swcr_p1);
		}

		if (fex_p2) {
			swcr_p2 |= fex_to_swcr_status(fex_p2, 2);
			current_thread_info()->ieee_state
				|= fex_to_swcr_status(fex_p2, 2);

			/* Update hardware control register.  */
			fpcr_p2 = fpcr;
			fpcr_p2 &= (~FPCR_MASK | FPCR_DYN_MASK);
			fpcr_p2 |= ieee_swcr_to_fpcr(swcr_p2);
		}

		if (fex_p3) {
			swcr_p3 |= fex_to_swcr_status(fex_p3, 3);
			current_thread_info()->ieee_state
				|= fex_to_swcr_status(fex_p3, 3);

			/* Update hardware control register.  */
			fpcr_p3 = fpcr;
			fpcr_p3 &= (~FPCR_MASK | FPCR_DYN_MASK);
			fpcr_p3 |= ieee_swcr_to_fpcr(swcr_p3);
		}

		fpcr = fpcr_p0 | fpcr_p1 | fpcr_p2 | fpcr_p3;
		pr_debug("fex_p0 = %#lx\n", fex_p0);
		pr_debug("fex_p1 = %#lx\n", fex_p1);
		pr_debug("fex_p2 = %#lx\n", fex_p2);
		pr_debug("fex_p3 = %#lx\n", fex_p3);
		pr_debug("SIMD emulation almost finished.before write fpcr = %#lx\n", fpcr);
		wrfpcr(fpcr);
		pr_debug("Before write fp: vp_p0=%#lx, vc_p1=%#lx, vc_p2=%#lx, vc_p3=%#lx\n", vc_p0, vc_p1, vc_p2, vc_p3);
		write_fp_reg_d(fc, vc_p0, vc_p1, vc_p2, vc_p3);

		/* Do we generate a signal?  */
		_fex = (fex_p0 & swcr & IEEE_TRAP_ENABLE_MASK) | (fex_p1 & swcr & IEEE_TRAP_ENABLE_MASK)
			| (fex_p2 & swcr & IEEE_TRAP_ENABLE_MASK) | (fex_p3 & swcr & IEEE_TRAP_ENABLE_MASK);
		si_code = 0;
		if (_fex) {
			if (_fex & IEEE_TRAP_ENABLE_DNO)
				si_code = FPE_FLTUND;
			if (_fex & IEEE_TRAP_ENABLE_INE)
				si_code = FPE_FLTRES;
			if (_fex & IEEE_TRAP_ENABLE_UNF)
				si_code = FPE_FLTUND;
			if (_fex & IEEE_TRAP_ENABLE_OVF)
				si_code = FPE_FLTOVF;
			if (_fex & IEEE_TRAP_ENABLE_DZE)
				si_code = FPE_FLTDIV;
			if (_fex & IEEE_TRAP_ENABLE_INV)
				si_code = FPE_FLTINV;
		}
		pr_debug("SIMD finished.. si_code:%#lx\n", si_code);
		return si_code;
	}
	pr_debug("SIMD finished.. si_code:%#lx\n", si_code);
	return 0;

bad_insn:
	pr_err("%s: Invalid FP insn %#x at %#lx\n", __func__, insn, pc);
	return -1;
}

long simd_fp_emul_s(unsigned long pc)
{
	FP_DECL_EX;
	FP_DECL_S(SA); FP_DECL_S(SB); FP_DECL_S(SR);

	unsigned long fa, fb, fc, func, mode, src;
	unsigned long res, va, vb, vc, swcr, fpcr;
	__u32 insn;
	long si_code;

	unsigned long va_p0, va_p1, va_p2, va_p3;
	unsigned long vb_p0, vb_p1, vb_p2, vb_p3;
	unsigned long vc_p0, vc_p1, vc_p2, vc_p3;
	unsigned long fex_p0, fex_p1, fex_p2, fex_p3;

	int working_part;

	get_user(insn, (__u32 *)pc);
	fc     = (insn >>  0) & 0x1f;	/* destination register */
	fb     = (insn >> 16) & 0x1f;
	fa     = (insn >> 21) & 0x1f;
	func   = (insn >>  5) & 0xff;
	fpcr = rdfpcr();
	mode   = (fpcr >> FPCR_DYN_SHIFT) & 0x3;

	pr_debug("======== Entering SIMD S-floating math emulation =======\n");
	pr_debug("hardware fpcr = %#lx\n", fpcr);
	swcr = swcr_update_status(current_thread_info()->ieee_state, fpcr);
	pr_debug("software swcr = %#lx\n", swcr);
	pr_debug("fa:%#lx,fb:%#lx,fc:%#lx,func:%#lx,mode:%#lx\n", fa, fb, fc, func, mode);
	read_fp_reg_s(fa, &va_p0, &va_p1, &va_p2, &va_p3);
	read_fp_reg_s(fb, &vb_p0, &vb_p1, &vb_p2, &vb_p3);
	read_fp_reg_s(fc, &vc_p0, &vc_p1, &vc_p2, &vc_p3);
	pr_debug("va_p0:%#lx, va_p1:%#lx, va_p2:%#lx, va_p3:%#lx\n", va_p0, va_p1, va_p2, va_p3);
	pr_debug("vb_p0:%#lx, vb_p1:%#lx, vb_p2:%#lx, vb_p3:%#lx\n", vb_p0, vb_p1, vb_p2, vb_p3);
	pr_debug("vc_p0:%#lx, vc_p1:%#lx, vc_p2:%#lx, vc_p3:%#lx\n", vc_p0, vc_p1, vc_p2, vc_p3);
	working_part = WORKING_PART_0;
simd_working:
	_fex = 0;
	switch (working_part) {
	case WORKING_PART_0:
		pr_debug("WORKING_PART_0\n");
		va = va_p0;
		vb = vb_p0;
		vc = vc_p0;
		if ((fpcr & FPCR_STATUS_MASK0) == 0) {
			SW64_FP_NORMAL_S(SA, &va);
			SW64_FP_NORMAL_S(SB, &vb);
			if ((SA_c == SW64_FP_NORMAL) && (SB_c == SW64_FP_NORMAL))
				goto next_working_s;
			else
				pr_debug("PART0: SA_c = %#lx, SB_c = %#lx\n", SA_c, SB_c);
		} else {
			SW64_FP_NAN_S(SA, &va);
			SW64_FP_NAN_S(SB, &vb);
			if ((SA_c == SW64_FP_NAN) && (SB_c == SW64_FP_NAN))
				goto next_working_s;
		}
		break;
	case WORKING_PART_1:
		pr_debug("WORKING_PART_1\n");
		va = va_p1;
		vb = vb_p1;
		vc = vc_p1;
		if ((fpcr & FPCR_STATUS_MASK1) == 0) {
			SW64_FP_NORMAL_S(SA, &va);
			SW64_FP_NORMAL_S(SB, &vb);
			if ((SA_c == SW64_FP_NORMAL) && (SB_c == SW64_FP_NORMAL))
				goto next_working_s;
			else
				pr_debug("PART1: SA_c = %#lx, SB_c = %#lx\n", SA_c, SB_c);
		} else {
			SW64_FP_NAN_S(SA, &va);
			SW64_FP_NAN_S(SB, &vb);
			if ((SA_c == SW64_FP_NAN) && (SB_c == SW64_FP_NAN))
				goto next_working_s;
		}
		break;
	case WORKING_PART_2:
		pr_debug("WORKING_PART_2\n");
		va = va_p2;
		vb = vb_p2;
		vc = vc_p2;
		if ((fpcr & FPCR_STATUS_MASK2) == 0) {
			SW64_FP_NORMAL_S(SA, &va);
			SW64_FP_NORMAL_S(SB, &vb);
			if ((SA_c == SW64_FP_NORMAL) && (SB_c == SW64_FP_NORMAL))
				goto next_working_s;
			else
				pr_debug("PART2: SA_c = %#lx, SB_c = %#lx\n", SA_c, SB_c);
		} else {
			SW64_FP_NAN_S(SA, &va);
			SW64_FP_NAN_S(SB, &vb);
			if ((SA_c == SW64_FP_NAN) && (SB_c == SW64_FP_NAN))
				goto next_working_s;
		}
		break;
	case WORKING_PART_3:
		pr_debug("WORKING_PART_3\n");
		va = va_p3;
		vb = vb_p3;
		vc = vc_p3;
		if ((fpcr & FPCR_STATUS_MASK3) == 0) {
			SW64_FP_NORMAL_S(SA, &va);
			SW64_FP_NORMAL_S(SB, &vb);
			if ((SA_c == SW64_FP_NORMAL) && (SB_c == SW64_FP_NORMAL))
				goto next_working_s;
			else
				pr_debug("PART3: SA_c = %#lx, SB_c = %#lx\n", SA_c, SB_c);
		} else {
			SW64_FP_NAN_S(SA, &va);
			SW64_FP_NAN_S(SB, &vb);
			if ((SA_c == SW64_FP_NAN) && (SB_c == SW64_FP_NAN))
				goto next_working_s;
		}
		break;

	}

	FP_UNPACK_SP(SA, &va);
	FP_UNPACK_SP(SB, &vb);

	switch (func) {
	case FNC_VSUBS:
		pr_debug("FNC_VSUBS\n");
		FP_SUB_S(SR, SA, SB);
		goto pack_s;
	case FNC_VMULS:
		pr_debug("FNC_VMULS\n");
		FP_MUL_S(SR, SA, SB);
		goto pack_s;
	case FNC_VADDS:
		pr_debug("FNC_VADDS\n");
		FP_ADD_S(SR, SA, SB);
		goto pack_s;
	case FNC_VDIVS:
		pr_debug("FNC_VDIVS\n");
		FP_DIV_S(SR, SA, SB);
		goto pack_s;
	case FNC_VSQRTS:
		pr_debug("FNC_VSQRTS\n");
		FP_SQRT_S(SR, SB);
		goto pack_s;
	}
pack_s:
	FP_PACK_SP(&vc, SR);
	if ((_fex & FP_EX_UNDERFLOW) && (swcr & IEEE_MAP_UMZ)) {
		pr_debug("pack_s, vc=0 !!!!\n");
		vc = 0;
	}

	pr_debug("SW64 SIMD Emulation S-floating _fex=%#lx, va=%#lx, vb=%#lx, vc=%#lx\n", _fex, va, vb, vc);
	pr_debug("SW64 SIMD Emulation S-floating mode=%#lx,func=%#lx, swcr=%#lx\n", mode, func, swcr);
next_working_s:
	switch (working_part) {
	case WORKING_PART_0:
		working_part = WORKING_PART_1;
		vc_p0 = vc;
		fex_p0 = _fex;
		goto simd_working;
	case WORKING_PART_1:
		working_part = WORKING_PART_2;
		vc_p1 = vc;
		fex_p1 = _fex;
		goto simd_working;
	case WORKING_PART_2:
		working_part = WORKING_PART_3;
		vc_p2 = vc;
		fex_p2 = _fex;
		goto simd_working;
	case WORKING_PART_3:
		vc_p3 = vc;
		fex_p3 = _fex;
		goto done;
	}
done:
	if (fex_p0 || fex_p1 || fex_p2 || fex_p3) {
		unsigned long fpcr_p0, fpcr_p1, fpcr_p2, fpcr_p3;
		unsigned long swcr_p0, swcr_p1, swcr_p2, swcr_p3;

		fpcr_p0 = fpcr_p1 = fpcr_p2 = fpcr_p3 = 0;
		swcr_p0 = swcr_p1 = swcr_p2 = swcr_p3 = swcr;
		/* manage fpcr_p0 */
		if (fex_p0) {
			swcr_p0 |= fex_to_swcr_status(fex_p0, 0);
			current_thread_info()->ieee_state
				|= fex_to_swcr_status(fex_p0, 0);

			/* Update hardware control register.  */
			fpcr_p0 = fpcr;
			fpcr_p0 &= (~FPCR_MASK | FPCR_DYN_MASK);
			fpcr_p0 |= ieee_swcr_to_fpcr(swcr_p0);
			pr_debug("fex_p0: fpcr_p0:%#lx\n", fpcr_p0);
		}

		if (fex_p1) {
			swcr_p1 |= fex_to_swcr_status(fex_p1, 1);
			current_thread_info()->ieee_state
				|= fex_to_swcr_status(fex_p1, 1);

			/* Update hardware control register.  */
			fpcr_p1 = fpcr;
			fpcr_p1 &= (~FPCR_MASK | FPCR_DYN_MASK);
			fpcr_p1 |= ieee_swcr_to_fpcr(swcr_p1);
			pr_debug("fex_p1: fpcr_p1:%#lx\n", fpcr_p1);
		}

		if (fex_p2) {
			swcr_p2 |= fex_to_swcr_status(fex_p2, 2);
			current_thread_info()->ieee_state
				|= fex_to_swcr_status(fex_p2, 2);

			/* Update hardware control register.  */
			fpcr_p2 = fpcr;
			fpcr_p2 &= (~FPCR_MASK | FPCR_DYN_MASK);
			fpcr_p2 |= ieee_swcr_to_fpcr(swcr_p2);
			pr_debug("fex_p2: fpcr_p2:%#lx\n", fpcr_p2);
		}

		if (fex_p3) {
			swcr_p3 |= fex_to_swcr_status(fex_p3, 3);
			current_thread_info()->ieee_state
				|= fex_to_swcr_status(fex_p3, 3);

			/* Update hardware control register.  */
			fpcr_p3 = fpcr;
			fpcr_p3 &= (~FPCR_MASK | FPCR_DYN_MASK);
			fpcr_p3 |= ieee_swcr_to_fpcr(swcr_p3);
			pr_debug("fex_p3: fpcr_p3:%#lx\n", fpcr_p3);
		}

		fpcr = fpcr_p0 | fpcr_p1 | fpcr_p2 | fpcr_p3;
		pr_debug("fex_p0 = %#lx\n", fex_p0);
		pr_debug("fex_p1 = %#lx\n", fex_p1);
		pr_debug("fex_p2 = %#lx\n", fex_p2);
		pr_debug("fex_p3 = %#lx\n", fex_p3);
		pr_debug("SIMD emulation almost finished.before write fpcr = %#lx\n", fpcr);
		wrfpcr(fpcr);

		pr_debug("Before write fp: vc_p0=%#lx, vc_p1=%#lx, vc_p2=%#lx, vc_p3=%#lx\n", vc_p0, vc_p1, vc_p2, vc_p3);
		write_fp_reg_s(fc, vc_p0, vc_p1, vc_p2, vc_p3);

		/* Do we generate a signal?  */
		_fex = (fex_p0 & swcr & IEEE_TRAP_ENABLE_MASK) | (fex_p1 & swcr & IEEE_TRAP_ENABLE_MASK)
			| (fex_p2 & swcr & IEEE_TRAP_ENABLE_MASK) | (fex_p3 & swcr & IEEE_TRAP_ENABLE_MASK);
		si_code = 0;
		if (_fex) {
			if (_fex & IEEE_TRAP_ENABLE_DNO)
				si_code = FPE_FLTUND;
			if (_fex & IEEE_TRAP_ENABLE_INE)
				si_code = FPE_FLTRES;
			if (_fex & IEEE_TRAP_ENABLE_UNF)
				si_code = FPE_FLTUND;
			if (_fex & IEEE_TRAP_ENABLE_OVF)
				si_code = FPE_FLTOVF;
			if (_fex & IEEE_TRAP_ENABLE_DZE)
				si_code = FPE_FLTDIV;
			if (_fex & IEEE_TRAP_ENABLE_INV)
				si_code = FPE_FLTINV;
		}
		pr_debug("SIMD finished.. si_code:%#lx\n", si_code);
		return si_code;
	}
	pr_debug("SIMD finished.. si_code:%#lx\n", si_code);
	return 0;

bad_insn:
	pr_err("%s: Invalid FP insn %#x at %#lx\n", __func__, insn, pc);
	return -1;

}

static inline unsigned long negative_value(unsigned long va)
{
	return (va ^ 0x8000000000000000UL);
}

static inline unsigned long s_negative_value(unsigned long va)
{
	return (va ^ 0x80000000UL);
}

/*
 * sw64 mul-add  floating emulation
 */
long mul_add_fp_emul(unsigned long pc)
{
	FP_DECL_EX;
	FP_DECL_S(SA); FP_DECL_S(SB); FP_DECL_S(SC); FP_DECL_S(S_TMP); FP_DECL_S(SR);
	FP_DECL_D(DA); FP_DECL_D(DB); FP_DECL_D(DC); FP_DECL_D(D_TMP); FP_DECL_D(DR);
	FP_DECL_S(S_ZERO);
	FP_DECL_D(D_ZERO);
	FP_DECL_S(S_TMP2);
	FP_DECL_D(D_TMP2);

	unsigned long fa, fb, fc, fd, func, mode, src;
	unsigned long res, va, vb, vc, vd, vtmp, vtmp2, swcr, fpcr;
	__u32 insn;
	long si_code;
	unsigned long vzero = 0;

	get_user(insn, (__u32 *)pc);
	fd     = (insn >>  0) & 0x1f;	/* destination register */
	fc     = (insn >>  5) & 0x1f;
	fb     = (insn >> 16) & 0x1f;
	fa     = (insn >> 21) & 0x1f;
	func   = (insn >> 10) & 0x3f;

	fpcr = rdfpcr();
	mode   = (fpcr >> FPCR_DYN_SHIFT) & 0x3;

	pr_debug("===== Entering SW64 MUL-ADD Emulation =====\n");
	pr_debug("hardware fpcr = %#lx\n", fpcr);
	swcr = swcr_update_status(current_thread_info()->ieee_state, fpcr);
	pr_debug("software swcr = %#lx\n", swcr);

	if (func == FNC_FMAS || func == FNC_FMSS || func == FNC_FNMAS || func == FNC_FNMSS) {
		va = sw64_read_fp_reg_s(fa);
		vb = sw64_read_fp_reg_s(fb);
		vc = sw64_read_fp_reg_s(fc);
		FP_UNPACK_SP(SA, &va);
		FP_UNPACK_SP(SB, &vb);
		FP_UNPACK_SP(SC, &vc);
		FP_UNPACK_SP(S_ZERO, &vzero);
	}
	if (func == FNC_FMAD || func == FNC_FMSD || func == FNC_FNMAD || func == FNC_FNMSD) {
		va = sw64_read_fp_reg(fa);
		vb = sw64_read_fp_reg(fb);
		vc = sw64_read_fp_reg(fc);
		FP_UNPACK_DP(DA, &va);
		FP_UNPACK_DP(DB, &vb);
		FP_UNPACK_DP(DC, &vc);
		FP_UNPACK_DP(D_ZERO, &vzero);
	}
	pr_debug("va = %#lx, vb = %#lx, vc = %#lx\n", va, vb, vc);
	switch (func) {
	case FNC_FMAS:
		FP_MUL_S(S_TMP, SA, SB);
		FP_ADD_S(SR, S_TMP, SC);
		goto pack_s;
	case FNC_FMSS:
		FP_MUL_S(S_TMP, SA, SB);
		FP_SUB_S(SR, S_TMP, SC);
		goto pack_s;
	case FNC_FNMAS: /* (-va*vb) + vc */
		va = s_negative_value(va);
		FP_UNPACK_SP(SA, &va);
		FP_MUL_S(S_TMP, SA, SB);
		FP_ADD_S(SR, S_TMP, SC);
		goto pack_s;
	case FNC_FNMSS: /* (-va*vb) - vc */
		va = s_negative_value(va);
		FP_UNPACK_SP(SA, &va);
		FP_MUL_S(S_TMP, SA, SB);
		FP_SUB_S(SR, S_TMP, SC);
		goto pack_s;
	case FNC_FMAD:
		FP_MUL_D(D_TMP, DA, DB);
		FP_ADD_D(DR, D_TMP, DC);
		goto pack_d;
	case FNC_FMSD:
		FP_MUL_D(D_TMP, DA, DB);
		FP_SUB_D(DR, D_TMP, DC);
		goto pack_d;
	case FNC_FNMAD:
		va = negative_value(va);
		FP_UNPACK_DP(DA, &va);
		FP_MUL_D(D_TMP, DA, DB);
		FP_ADD_D(DR, D_TMP, DC);
		goto pack_d;
	case FNC_FNMSD:
		va = negative_value(va);
		FP_UNPACK_DP(DA, &va);
		FP_MUL_D(D_TMP, DA, DB);
		FP_SUB_D(DR, D_TMP, DC);
		goto pack_d;
	default:
		goto bad_insn;

	}
pack_s:
	FP_PACK_SP(&vd, SR);
	if ((_fex & FP_EX_UNDERFLOW) && (swcr & IEEE_MAP_UMZ))
		vd = 0;
	sw64_write_fp_reg_s(fd, vd);
	goto done;

pack_d:
	FP_PACK_DP(&vd, DR);
	if ((_fex & FP_EX_UNDERFLOW) && (swcr & IEEE_MAP_UMZ))
		vd = 0;
	sw64_write_fp_reg(fd, vd);

done:
	pr_debug("vd = %#lx\n", vd);
	if (_fex) {
		/* Record exceptions in software control word.  */
		swcr |= fex_to_swcr_status(_fex, 0);
		current_thread_info()->ieee_state
			|= fex_to_swcr_status(_fex, 0);

		/* Update hardware control register.  */
		fpcr &= (~FPCR_MASK | FPCR_DYN_MASK);
		fpcr |= ieee_swcr_to_fpcr(swcr);
		wrfpcr(fpcr);                           /** wrfpcr will destroy vector register! */
		if (func == FNC_FMAS || func == FNC_FMSS || func == FNC_FNMAS || func == FNC_FNMSS)
			sw64_write_fp_reg_s(fd, vd);
		if (func == FNC_FMAD || func == FNC_FMSD || func == FNC_FNMAD || func == FNC_FNMSD)
			sw64_write_fp_reg(fd, vd);

		/* Do we generate a signal?  */
		_fex = _fex & swcr & IEEE_TRAP_ENABLE_MASK;
		si_code = 0;
		if (_fex) {
			if (_fex & IEEE_TRAP_ENABLE_DNO)
				si_code = FPE_FLTUND;
			if (_fex & IEEE_TRAP_ENABLE_INE)
				si_code = FPE_FLTRES;
			if (_fex & IEEE_TRAP_ENABLE_UNF)
				si_code = FPE_FLTUND;
			if (_fex & IEEE_TRAP_ENABLE_OVF)
				si_code = FPE_FLTOVF;
			if (_fex & IEEE_TRAP_ENABLE_DZE)
				si_code = FPE_FLTDIV;
			if (_fex & IEEE_TRAP_ENABLE_INV)
				si_code = FPE_FLTINV;
		}

		return si_code;
	}

	/*
	 * We used to write the destination register here, but DEC FORTRAN
	 * requires that the result *always* be written... so we do the write
	 * immediately after the operations above.
	 */

	return 0;

bad_insn:
	pr_err("%s: Invalid FP insn %#x at %#lx\n", __func__, insn, pc);
	return -1;
}


long simd_mul_add_fp_emul_s(unsigned long pc)
{
	FP_DECL_EX;
	FP_DECL_S(SA); FP_DECL_S(SB); FP_DECL_S(SC); FP_DECL_S(S_TMP); FP_DECL_S(SR);
	FP_DECL_S(S_ZERO);
	FP_DECL_S(S_TMP2);

	unsigned long fa, fb, fc, fd, func, mode, src;
	unsigned long res, va, vb, vc, vd, vtmp, vtmp2, swcr, fpcr;
	__u32 insn;
	long si_code;
	unsigned long vzero = 0;

	get_user(insn, (__u32 *)pc);
	fd     = (insn >>  0) & 0x1f;	/* destination register */
	fc     = (insn >>  5) & 0x1f;
	fb     = (insn >> 16) & 0x1f;
	fa     = (insn >> 21) & 0x1f;
	func   = (insn >> 10) & 0x3f;

	fpcr = rdfpcr();
	mode   = (fpcr >> FPCR_DYN_SHIFT) & 0x3;

	unsigned long va_p0, va_p1, va_p2, va_p3;
	unsigned long vb_p0, vb_p1, vb_p2, vb_p3;
	unsigned long vc_p0, vc_p1, vc_p2, vc_p3;
	unsigned long vd_p0, vd_p1, vd_p2, vd_p3;
	unsigned long fex_p0, fex_p1, fex_p2, fex_p3;

	int working_part;

	pr_debug("======== Entering SIMD S-floating mul-add emulation =======\n");
	swcr = swcr_update_status(current_thread_info()->ieee_state, fpcr);
	pr_debug("software swcr = %#lx\n", swcr);
	pr_debug("hardware fpcr = %#lx\n", fpcr);
	read_fp_reg_s(fa, &va_p0, &va_p1, &va_p2, &va_p3);
	read_fp_reg_s(fb, &vb_p0, &vb_p1, &vb_p2, &vb_p3);
	read_fp_reg_s(fc, &vc_p0, &vc_p1, &vc_p2, &vc_p3);
	read_fp_reg_s(fd, &vd_p0, &vd_p1, &vd_p2, &vd_p3);
	pr_debug("va_p0:%#lx, va_p1:%#lx, va_p2:%#lx, va_p3:%#lx\n", va_p0, va_p1, va_p2, va_p3);
	pr_debug("vb_p0:%#lx, vb_p1:%#lx, vb_p2:%#lx, vb_p3:%#lx\n", vb_p0, vb_p1, vb_p2, vb_p3);
	pr_debug("vc_p0:%#lx, vc_p1:%#lx, vc_p2:%#lx, vc_p3:%#lx\n", vc_p0, vc_p1, vc_p2, vc_p3);
	pr_debug("vd_p0:%#lx, vd_p1:%#lx, vd_p2:%#lx, vd_p3:%#lx\n", vd_p0, vd_p1, vd_p2, vd_p3);
	working_part = WORKING_PART_0;
simd_working:
	_fex = 0;
	switch (working_part) {
	case WORKING_PART_0:
		pr_debug("WORKING_PART_0\n");
		va = va_p0;
		vb = vb_p0;
		vc = vc_p0;
		pr_debug("FPCR_STATUS_MASK0 : %#lx, fpcr :%#lx\n", FPCR_STATUS_MASK0, fpcr);
		if ((fpcr & FPCR_STATUS_MASK0) == 0) {
			SW64_FP_NORMAL_S(SA, &va);
			SW64_FP_NORMAL_S(SB, &vb);
			SW64_FP_NORMAL_S(SC, &vc);
			if ((SA_c == SW64_FP_NORMAL) && (SB_c == SW64_FP_NORMAL) && (SC_c == SW64_FP_NORMAL))
				goto next_working_s;
			else
				pr_debug("LOW: SA_c = %#lx, SB_c = %#lx\n", SA_c, SB_c);
		} else {
			SW64_FP_NAN_S(SA, &va);
			SW64_FP_NAN_S(SB, &vb);
			if ((SA_c == SW64_FP_NAN) && (SB_c == SW64_FP_NAN))
				goto next_working_s;
		}
		break;
	case WORKING_PART_1:
		pr_debug("WORKING_PART_1\n");
		va = va_p1;
		vb = vb_p1;
		vc = vc_p1;
		pr_debug("FPCR_STATUS_MASK1 : %#lx, fpcr :%#lx\n", FPCR_STATUS_MASK0, fpcr);
		if ((fpcr & FPCR_STATUS_MASK1) == 0) {
			SW64_FP_NORMAL_S(SA, &va);
			SW64_FP_NORMAL_S(SB, &vb);
			SW64_FP_NORMAL_S(SC, &vc);
			if ((SA_c == SW64_FP_NORMAL) && (SB_c == SW64_FP_NORMAL) && (SC_c == SW64_FP_NORMAL))
				goto next_working_s;
			else
				pr_debug("HIGH: SA_c = %#lx, SB_c = %#lx\n", SA_c, SB_c);
		} else {
			SW64_FP_NAN_S(SA, &va);
			SW64_FP_NAN_S(SB, &vb);
			if ((SA_c == SW64_FP_NAN) && (SB_c == SW64_FP_NAN))
				goto next_working_s;
		}
		break;
	case WORKING_PART_2:
		pr_debug("WORKING_PART_2\n");
		va = va_p2;
		vb = vb_p2;
		vc = vc_p2;
		if ((fpcr & FPCR_STATUS_MASK2) == 0) {
			SW64_FP_NORMAL_S(SA, &va);
			SW64_FP_NORMAL_S(SB, &vb);
			SW64_FP_NORMAL_S(SC, &vc);
			if ((SA_c == SW64_FP_NORMAL) && (SB_c == SW64_FP_NORMAL) && (SC_c == SW64_FP_NORMAL))
				goto next_working_s;
			else
				pr_debug("HIGH: SA_c = %#lx, SB_c = %#lx\n", SA_c, SB_c);
		} else {
			SW64_FP_NAN_S(SA, &va);
			SW64_FP_NAN_S(SB, &vb);
			if ((SA_c == SW64_FP_NAN) && (SB_c == SW64_FP_NAN))
				goto next_working_s;
		}
		break;
	case WORKING_PART_3:
		pr_debug("WORKING_PART_3\n");
		va = va_p3;
		vb = vb_p3;
		vc = vc_p3;
		if ((fpcr & FPCR_STATUS_MASK3) == 0) {
			SW64_FP_NORMAL_S(SA, &va);
			SW64_FP_NORMAL_S(SB, &vb);
			SW64_FP_NORMAL_S(SC, &vc);
			if ((SA_c == SW64_FP_NORMAL) && (SB_c == SW64_FP_NORMAL) && (SC_c == SW64_FP_NORMAL))
				goto next_working_s;
			else
				pr_debug("HIGH: SA_c = %#lx, SB_c = %#lx\n", SA_c, SB_c);
		} else {
			SW64_FP_NAN_S(SA, &va);
			SW64_FP_NAN_S(SB, &vb);
			if ((SA_c == SW64_FP_NAN) && (SB_c == SW64_FP_NAN))
				goto next_working_s;
		}
		break;
	}

	FP_UNPACK_SP(SA, &va);
	FP_UNPACK_SP(SB, &vb);
	FP_UNPACK_SP(SC, &vc);
	FP_UNPACK_SP(S_ZERO, &vzero);
	switch (func) {
	case FNC_FMAS:
		FP_MUL_S(S_TMP, SA, SB);
		FP_ADD_S(SR, S_TMP, SC);
		goto pack_s;
	case FNC_FMSS:
		FP_MUL_S(S_TMP, SA, SB);
		FP_SUB_S(SR, S_TMP, SC);
		goto pack_s;
	case FNC_FNMAS: /* (-va*vb) + vc */
		va = s_negative_value(va);
		FP_UNPACK_SP(SA, &va);
		FP_MUL_S(S_TMP, SA, SB);
		FP_ADD_S(SR, S_TMP, SC);
		goto pack_s;
	case FNC_FNMSS: /* (-va*vb) - vc */
		va = s_negative_value(va);
		FP_UNPACK_SP(SA, &va);
		FP_MUL_S(S_TMP, SA, SB);
		FP_SUB_S(SR, S_TMP, SC);
		goto pack_s;
	default:
		goto bad_insn;
	}

pack_s:
	FP_PACK_SP(&vd, SR);
	if ((_fex & FP_EX_UNDERFLOW) && (swcr & IEEE_MAP_UMZ))
		vd = 0;
	pr_debug("SW64 SIMD Emulation S-floating _fex=%#lx, va=%#lx, vb=%#lx, vc=%#lx\n", _fex, va, vb, vc);
	pr_debug("SW64 SIMD Emulation S-floating mode=%#lx,func=%#lx, swcr=%#lx\n", mode, func, swcr);
next_working_s:
	switch (working_part) {
	case WORKING_PART_0:
		working_part = WORKING_PART_1;
		vd_p0 = vd;
		fex_p0 = _fex;
		goto simd_working;
	case WORKING_PART_1:
		working_part = WORKING_PART_2;
		vd_p1 = vd;
		fex_p1 = _fex;
		goto simd_working;
	case WORKING_PART_2:
		working_part = WORKING_PART_3;
		vd_p2 = vd;
		fex_p2 = _fex;
		goto simd_working;
	case WORKING_PART_3:
		vd_p3 = vd;
		fex_p3 = _fex;
		goto done;
	}
done:
	if (fex_p0 || fex_p1 || fex_p2 || fex_p3) {
		unsigned long fpcr_p0, fpcr_p1, fpcr_p2, fpcr_p3;
		unsigned long swcr_p0, swcr_p1, swcr_p2, swcr_p3;

		fpcr_p0 = fpcr_p1 = fpcr_p2 = fpcr_p3 = 0;
		swcr_p0 = swcr_p1 = swcr_p2 = swcr_p3 = swcr;
		/* manage fpcr_p0 */
		if (fex_p0) {
			swcr_p0 |= fex_to_swcr_status(fex_p0, 0);
			current_thread_info()->ieee_state
				|= fex_to_swcr_status(fex_p0, 0);

			/* Update hardware control register.  */
			fpcr_p0 = fpcr;
			fpcr_p0 &= (~FPCR_MASK | FPCR_DYN_MASK);
			fpcr_p0 |= ieee_swcr_to_fpcr(swcr_p0);
		}

		if (fex_p1) {
			swcr_p1 |= fex_to_swcr_status(fex_p1, 1);
			current_thread_info()->ieee_state
				|= fex_to_swcr_status(fex_p1, 1);

			/* Update hardware control register.  */
			fpcr_p1 = fpcr;
			fpcr_p1 &= (~FPCR_MASK | FPCR_DYN_MASK);
			fpcr_p1 |= ieee_swcr_to_fpcr(swcr_p1);
		}

		if (fex_p2) {
			swcr_p2 |= fex_to_swcr_status(fex_p2, 2);
			current_thread_info()->ieee_state
				|= fex_to_swcr_status(fex_p2, 2);

			/* Update hardware control register.  */
			fpcr_p2 = fpcr;
			fpcr_p2 &= (~FPCR_MASK | FPCR_DYN_MASK);
			fpcr_p2 |= ieee_swcr_to_fpcr(swcr_p2);
		}

		if (fex_p3) {
			swcr_p3 |= fex_to_swcr_status(fex_p3, 3);
			current_thread_info()->ieee_state
				|= fex_to_swcr_status(fex_p3, 3);

			/* Update hardware control register.  */
			fpcr_p3 = fpcr;
			fpcr_p3 &= (~FPCR_MASK | FPCR_DYN_MASK);
			fpcr_p3 |= ieee_swcr_to_fpcr(swcr_p3);
		}

		fpcr = fpcr_p0 | fpcr_p1 | fpcr_p2 | fpcr_p3;
		pr_debug("fex_p0 = %#lx\n", fex_p0);
		pr_debug("fex_p1 = %#lx\n", fex_p1);
		pr_debug("fex_p2 = %#lx\n", fex_p2);
		pr_debug("fex_p3 = %#lx\n", fex_p3);
		pr_debug("SIMD emulation almost finished.before write fpcr = %#lx\n", fpcr);
		wrfpcr(fpcr);
		pr_debug("Before write fp: vp_p0=%#lx, vc_p1=%#lx, vc_p2=%#lx, vc_p3=%#lx\n", vc_p0, vc_p1, vc_p2, vc_p3);
		write_fp_reg_s(fd, vd_p0, vd_p1, vd_p2, vd_p3); /* write to fd */

		/* Do we generate a signal?  */
		_fex = (fex_p0 & swcr & IEEE_TRAP_ENABLE_MASK) | (fex_p1 & swcr & IEEE_TRAP_ENABLE_MASK)
			| (fex_p2 & swcr & IEEE_TRAP_ENABLE_MASK) | (fex_p3 & swcr & IEEE_TRAP_ENABLE_MASK);
		si_code = 0;
		if (_fex) {
			if (_fex & IEEE_TRAP_ENABLE_DNO)
				si_code = FPE_FLTUND;
			if (_fex & IEEE_TRAP_ENABLE_INE)
				si_code = FPE_FLTRES;
			if (_fex & IEEE_TRAP_ENABLE_UNF)
				si_code = FPE_FLTUND;
			if (_fex & IEEE_TRAP_ENABLE_OVF)
				si_code = FPE_FLTOVF;
			if (_fex & IEEE_TRAP_ENABLE_DZE)
				si_code = FPE_FLTDIV;
			if (_fex & IEEE_TRAP_ENABLE_INV)
				si_code = FPE_FLTINV;
		}
		pr_debug("SIMD finished.. si_code:%#lx\n", si_code);
		return si_code;

	}
	pr_debug("SIMD finished.. si_code:%#lx\n", si_code);
	return 0;

bad_insn:
	pr_err("%s: Invalid FP insn %#x at %#lx\n", __func__, insn, pc);
	return -1;
}

long simd_mul_add_fp_emul_d(unsigned long pc)
{
	FP_DECL_EX;
	FP_DECL_D(DA); FP_DECL_D(DB); FP_DECL_D(DC); FP_DECL_D(D_TMP); FP_DECL_D(DR);
	FP_DECL_D(D_ZERO);
	FP_DECL_D(D_TMP2);

	unsigned long fa, fb, fc, fd, func, mode, src;
	unsigned long res, va, vb, vc, vd, vtmp, vtmp2, swcr, fpcr;
	__u32 insn;
	long si_code;
	unsigned long vzero = 0;

	get_user(insn, (__u32 *)pc);
	fd     = (insn >>  0) & 0x1f;	/* destination register */
	fc     = (insn >>  5) & 0x1f;
	fb     = (insn >> 16) & 0x1f;
	fa     = (insn >> 21) & 0x1f;
	func   = (insn >> 10) & 0x3f;

	fpcr = rdfpcr();
	mode   = (fpcr >> FPCR_DYN_SHIFT) & 0x3;

	unsigned long va_p0, va_p1, va_p2, va_p3;
	unsigned long vb_p0, vb_p1, vb_p2, vb_p3;
	unsigned long vc_p0, vc_p1, vc_p2, vc_p3;
	unsigned long vd_p0, vd_p1, vd_p2, vd_p3;
	unsigned long fex_p0, fex_p1, fex_p2, fex_p3;

	int working_part;

	pr_debug("======== Entering SIMD D-floating mul-add emulation =======\n");
	pr_debug("hardware fpcr = %#lx\n", fpcr);
	swcr = swcr_update_status(current_thread_info()->ieee_state, fpcr);
	pr_debug("software swcr = %#lx\n", swcr);
	read_fp_reg_d(fa, &va_p0, &va_p1, &va_p2, &va_p3);
	read_fp_reg_d(fb, &vb_p0, &vb_p1, &vb_p2, &vb_p3);
	read_fp_reg_d(fc, &vc_p0, &vc_p1, &vc_p2, &vc_p3);
	read_fp_reg_d(fd, &vd_p0, &vd_p1, &vd_p2, &vd_p3);
	pr_debug("va_p0:%#lx, va_p1:%#lx, va_p2:%#lx, va_p3:%#lx\n", va_p0, va_p1, va_p2, va_p3);
	pr_debug("vb_p0:%#lx, vb_p1:%#lx, vb_p2:%#lx, vb_p3:%#lx\n", vb_p0, vb_p1, vb_p2, vb_p3);
	pr_debug("vc_p0:%#lx, vc_p1:%#lx, vc_p2:%#lx, vc_p3:%#lx\n", vc_p0, vc_p1, vc_p2, vc_p3);
	pr_debug("vd_p0:%#lx, vd_p1:%#lx, vd_p2:%#lx, vd_p3:%#lx\n", vd_p0, vd_p1, vd_p2, vd_p3);
	working_part = WORKING_PART_0;
simd_working:
	_fex = 0;
	switch (working_part) {
	case WORKING_PART_0:
		pr_debug("WORKING_PART_0\n");
		va = va_p0;
		vb = vb_p0;
		vc = vc_p0;
		vd = vd_p0;
		if ((fpcr & FPCR_STATUS_MASK0) == 0) {
			SW64_FP_NORMAL_D(DA, &va);
			SW64_FP_NORMAL_D(DB, &vb);
			SW64_FP_NORMAL_D(DC, &vc);
			if ((DA_c == SW64_FP_NORMAL) && (DB_c == SW64_FP_NORMAL) && (DC_c == SW64_FP_NORMAL))
				goto next_working_s;
			else
				pr_debug("LOW: DA_c = %#lx, DB_c = %#lx\n", DA_c, DB_c);
		} else {
			SW64_FP_NAN_D(DA, &va);
			SW64_FP_NAN_D(DB, &vb);
			SW64_FP_NAN_D(DC, &vc);
			if ((DA_c == SW64_FP_NAN) && (DB_c == SW64_FP_NAN) && (DC_c == SW64_FP_NAN))
				goto next_working_s;
		}
		break;
	case WORKING_PART_1:
		pr_debug("WORKING_PART_1\n");
		va = va_p1;
		vb = vb_p1;
		vc = vc_p1;
		vd = vd_p1;
		if ((fpcr & FPCR_STATUS_MASK1) == 0) {
			SW64_FP_NORMAL_D(DA, &va);
			SW64_FP_NORMAL_D(DB, &vb);
			SW64_FP_NORMAL_D(DC, &vc);
			if ((DA_c == SW64_FP_NORMAL) && (DB_c == SW64_FP_NORMAL) && (DC_c == SW64_FP_NORMAL))
				goto next_working_s;
			else
				pr_debug("HIGH: DA_c = %#lx, DB_c = %#lx\n", DA_c, DB_c);
		} else {
			SW64_FP_NAN_D(DA, &va);
			SW64_FP_NAN_D(DB, &vb);
			SW64_FP_NAN_D(DC, &vc);
			if ((DA_c == SW64_FP_NAN) && (DB_c == SW64_FP_NAN) && (DC_c == SW64_FP_NAN))
				goto next_working_s;
		}
		break;
	case WORKING_PART_2:
		pr_debug("WORKING_PART_2\n");
		va = va_p2;
		vb = vb_p2;
		vc = vc_p2;
		vd = vd_p2;
		if ((fpcr & FPCR_STATUS_MASK2) == 0) {
			SW64_FP_NORMAL_D(DA, &va);
			SW64_FP_NORMAL_D(DB, &vb);
			SW64_FP_NORMAL_D(DC, &vc);
			if ((DA_c == SW64_FP_NORMAL) && (DB_c == SW64_FP_NORMAL) && (DC_c == SW64_FP_NORMAL))
				goto next_working_s;
			else
				pr_debug("HIGH: DA_c = %#lx, DB_c = %#lx\n", DA_c, DB_c);
		} else {
			SW64_FP_NAN_D(DA, &va);
			SW64_FP_NAN_D(DB, &vb);
			SW64_FP_NAN_D(DC, &vc);
			if ((DA_c == SW64_FP_NAN) && (DB_c == SW64_FP_NAN) && (DC_c == SW64_FP_NAN))
				goto next_working_s;
		}
		break;
	case WORKING_PART_3:
		pr_debug("WORKING_PART_3\n");
		va = va_p3;
		vb = vb_p3;
		vc = vc_p3;
		vd = vd_p3;
		if ((fpcr & FPCR_STATUS_MASK3) == 0) {
			SW64_FP_NORMAL_D(DA, &va);
			SW64_FP_NORMAL_D(DB, &vb);
			SW64_FP_NORMAL_D(DC, &vc);
			if ((DA_c == SW64_FP_NORMAL) && (DB_c == SW64_FP_NORMAL) && (DC_c == SW64_FP_NORMAL))
				goto next_working_s;
			else
				pr_debug("HIGH: DA_c = %#lx, DB_c = %#lx\n", DA_c, DB_c);
		} else {
			SW64_FP_NAN_D(DA, &va);
			SW64_FP_NAN_D(DB, &vb);
			SW64_FP_NAN_D(DC, &vc);
			if ((DA_c == SW64_FP_NAN) && (DB_c == SW64_FP_NAN) && (DC_c == SW64_FP_NAN))
				goto next_working_s;
		}
		break;
	}

	FP_UNPACK_DP(DA, &va);
	FP_UNPACK_DP(DB, &vb);
	FP_UNPACK_DP(DC, &vc);
	FP_UNPACK_DP(D_ZERO, &vzero);

	switch (func) {
	case FNC_FMAD:
		FP_MUL_D(D_TMP, DA, DB);
		FP_ADD_D(DR, D_TMP, DC);
		goto pack_d;
	case FNC_FMSD:
		FP_MUL_D(D_TMP, DA, DB);
		FP_SUB_D(DR, D_TMP, DC);
		goto pack_d;
	case FNC_FNMAD:
		va = negative_value(va);
		FP_UNPACK_DP(DA, &va);
		FP_MUL_D(D_TMP, DA, DB);
		FP_ADD_D(DR, D_TMP, DC);
		goto pack_d;
	case FNC_FNMSD:
		va = negative_value(va);
		FP_UNPACK_DP(DA, &va);
		FP_MUL_D(D_TMP, DA, DB);
		FP_SUB_D(DR, D_TMP, DC);

		goto pack_d;
	default:
		goto bad_insn;
	}

pack_d:
	FP_PACK_DP(&vd, DR);
	if ((_fex & FP_EX_UNDERFLOW) && (swcr & IEEE_MAP_UMZ))
		vd = 0;
	pr_debug("SW64 SIMD Emulation D-floating _fex=%#lx, va=%#lx, vb=%#lx, vc=%#lx\n", _fex, va, vb, vc);
	pr_debug("SW64 SIMD Emulation D-floating mode=%#lx,func=%#lx, swcr=%#lx\n", mode, func, swcr);
next_working_s:
	switch (working_part) {
	case WORKING_PART_0:
		working_part = WORKING_PART_1;
		vd_p0 = vd;
		fex_p0 = _fex;
		goto simd_working;
	case WORKING_PART_1:
		working_part = WORKING_PART_2;
		vd_p1 = vd;
		fex_p1 = _fex;
		goto simd_working;
	case WORKING_PART_2:
		working_part = WORKING_PART_3;
		vd_p2 = vd;
		fex_p2 = _fex;
		goto simd_working;
	case WORKING_PART_3:
		vd_p3 = vd;
		fex_p3 = _fex;
		goto done;
	}
done:
	if (fex_p0 || fex_p1 || fex_p2 || fex_p3) {
		unsigned long fpcr_p0, fpcr_p1, fpcr_p2, fpcr_p3;
		unsigned long swcr_p0, swcr_p1, swcr_p2, swcr_p3;

		fpcr_p0 = fpcr_p1 = fpcr_p2 = fpcr_p3 = 0;
		swcr_p0 = swcr_p1 = swcr_p2 = swcr_p3 = swcr;
		/* manage fpcr_p0 */
		if (fex_p0) {
			swcr_p0 |= fex_to_swcr_status(fex_p0, 0);
			current_thread_info()->ieee_state
				|= fex_to_swcr_status(fex_p0, 0);

			/* Update hardware control register.  */
			fpcr_p0 = fpcr;
			fpcr_p0 &= (~FPCR_MASK | FPCR_DYN_MASK);
			fpcr_p0 |= ieee_swcr_to_fpcr(swcr_p0);
		}

		if (fex_p1) {
			swcr_p1 |= fex_to_swcr_status(fex_p1, 1);
			current_thread_info()->ieee_state
				|= fex_to_swcr_status(fex_p1, 1);

			/* Update hardware control register.  */
			fpcr_p1 = fpcr;
			fpcr_p1 &= (~FPCR_MASK | FPCR_DYN_MASK);
			fpcr_p1 |= ieee_swcr_to_fpcr(swcr_p1);
		}

		if (fex_p2) {
			swcr_p2 |= fex_to_swcr_status(fex_p2, 2);
			current_thread_info()->ieee_state
				|= fex_to_swcr_status(fex_p2, 2);

			/* Update hardware control register.  */
			fpcr_p2 = fpcr;
			fpcr_p2 &= (~FPCR_MASK | FPCR_DYN_MASK);
			fpcr_p2 |= ieee_swcr_to_fpcr(swcr_p2);
		}

		if (fex_p3) {
			swcr_p3 |= fex_to_swcr_status(fex_p3, 3);
			current_thread_info()->ieee_state
				|= fex_to_swcr_status(fex_p3, 3);

			/* Update hardware control register.  */
			fpcr_p3 = fpcr;
			fpcr_p3 &= (~FPCR_MASK | FPCR_DYN_MASK);
			fpcr_p3 |= ieee_swcr_to_fpcr(swcr_p3);
		}

		fpcr = fpcr_p0 | fpcr_p1 | fpcr_p2 | fpcr_p3;
		pr_debug("fex_p0 = %#lx\n", fex_p0);
		pr_debug("fex_p1 = %#lx\n", fex_p1);
		pr_debug("fex_p2 = %#lx\n", fex_p2);
		pr_debug("fex_p3 = %#lx\n", fex_p3);
		pr_debug("SIMD emulation almost finished.before write fpcr = %#lx\n", fpcr);
		wrfpcr(fpcr);

		pr_debug("Before write fp: vp_p0=%#lx, vc_p1=%#lx, vc_p2=%#lx, vc_p3=%#lx\n", vc_p0, vc_p1, vc_p2, vc_p3);
		write_fp_reg_d(fd, vd_p0, vd_p1, vd_p2, vd_p3); /* write to fd */

		/* Do we generate a signal?  */
		_fex = (fex_p0 & swcr & IEEE_TRAP_ENABLE_MASK) | (fex_p1 & swcr & IEEE_TRAP_ENABLE_MASK)
			| (fex_p2 & swcr & IEEE_TRAP_ENABLE_MASK) | (fex_p3 & swcr & IEEE_TRAP_ENABLE_MASK);
		si_code = 0;
		if (_fex) {
			if (_fex & IEEE_TRAP_ENABLE_DNO)
				si_code = FPE_FLTUND;
			if (_fex & IEEE_TRAP_ENABLE_INE)
				si_code = FPE_FLTRES;
			if (_fex & IEEE_TRAP_ENABLE_UNF)
				si_code = FPE_FLTUND;
			if (_fex & IEEE_TRAP_ENABLE_OVF)
				si_code = FPE_FLTOVF;
			if (_fex & IEEE_TRAP_ENABLE_DZE)
				si_code = FPE_FLTDIV;
			if (_fex & IEEE_TRAP_ENABLE_INV)
				si_code = FPE_FLTINV;
		}
		pr_debug("SIMD finished.. si_code:%#lx\n", si_code);
		return si_code;
	}
	pr_debug("SIMD finished.. si_code:%#lx\n", si_code);
	return 0;

bad_insn:
	pr_err("%s: Invalid FP insn %#x at %#lx\n", __func__, insn, pc);
	return -1;
}

void read_fp_reg_s(unsigned long reg, unsigned long *val_p0,
		unsigned long *val_p1, unsigned long *val_p2, unsigned long *val_p3)
{
	unsigned long fp[2];

	sw64_read_simd_fp_m_s(reg, fp);
	*val_p0 = fp[0] & 0xffffffffUL;
	*val_p1 = (fp[0] >> 32) & 0xffffffffUL;
	*val_p2 = fp[1] & 0xffffffffUL;
	*val_p3 = (fp[1] >> 32) & 0xffffffffUL;
}

void read_fp_reg_d(unsigned long reg, unsigned long *val_p0,
		unsigned long *val_p1, unsigned long *val_p2, unsigned long *val_p3)
{
	unsigned long fp[4];

	sw64_read_simd_fp_m_d(reg, fp);
	*val_p0 = fp[0];
	*val_p1 = fp[1];
	*val_p2 = fp[2];
	*val_p3 = fp[3];
}

void write_fp_reg_s(unsigned long reg, unsigned long val_p0,
		unsigned long val_p1, unsigned long val_p2, unsigned long val_p3)
{
	unsigned long fp[2];

	fp[0] = ((val_p1 & 0xffffffffUL) << 32) | (val_p0 & 0xffffffffUL);
	fp[1] = ((val_p3 & 0xffffffffUL) << 32) | (val_p2 & 0xffffffffUL);
	sw64_write_simd_fp_reg_s(reg, fp[0], fp[1]);
}

void write_fp_reg_d(unsigned long reg, unsigned long val_p0,
		unsigned long val_p1, unsigned long val_p2, unsigned long val_p3)
{
	sw64_write_simd_fp_reg_d(reg, val_p0, val_p1, val_p2, val_p3);
}

/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_ASM_SW64_PTRACE_H
#define _UAPI_ASM_SW64_PTRACE_H

#include <linux/types.h>

#ifndef __ASSEMBLY__
/*
 * User structures for general purpose, floating point and debug registers.
 */
struct user_pt_regs {
	__u64 regs[31];
	__u64 pc;
	__u64 pstate;
};

/* 256 bits aligned for simd */
struct fpreg {
	__u64 v[4] __attribute__((aligned(32)));
};

struct user_fpsimd_state {
	struct fpreg fp[31];
	__u64 fpcr;
	__u64 __reserved[3];
};
#endif

/* PTRACE_ATTACH is 16 */
/* PTRACE_DETACH is 17 */

#define PT_REG_BASE		0
#define PT_REG_END		30
#define PT_FPREG_BASE		32
#define PT_FPREG_END		62
#define PT_FPCR			63
#define PT_PC			64
#define PT_TP			65
#define PT_UNIQUE		PT_TP
#define PT_VECREG_BASE		67
#define PT_VECREG_END		161
#define PT_F31_V1		98
#define PT_F31_V2		130
#define PT_DA_MATCH		163
#define PT_DA_MASK		164
#define PT_DV_MATCH		165
#define PT_DV_MASK		166
#define PT_DC_CTL		167
#define PT_MATCH_CTL		167
#define PT_IA_MATCH		168
#define PT_IA_MASK		169
#define PT_IV_MATCH		170
#define PT_IDA_MATCH		171
#define PT_IDA_MASK		172

#endif /* _UAPI_ASM_SW64_PTRACE_H */

// SPDX-License-Identifier: GPL-2.0
/*
 * Generate definitions needed by assembly language modules.
 * This code generates raw asm output which is post-processed to extract
 * and format the required data.
 */

#define GENERATING_ASM_OFFSETS  /* asm/smp.h */
#include <linux/stddef.h>
#include <linux/sched.h>
#include <linux/kbuild.h>
#include <linux/suspend.h>

#include <asm/suspend.h>
#include <asm/kvm.h>

#include "traps.c"
#include "signal.c"

void foo(void)
{
	DEFINE(ASM_THREAD_SIZE, THREAD_SIZE);
	DEFINE(TI_FLAGS, offsetof(struct thread_info, flags));
	OFFSET(TI_PCB_KSP, thread_info, pcb.ksp);
	BLANK();

	DEFINE(TASK_BLOCKED, offsetof(struct task_struct, blocked));
	DEFINE(TASK_CRED, offsetof(struct task_struct, cred));
	DEFINE(TASK_REAL_PARENT, offsetof(struct task_struct, real_parent));
	DEFINE(TASK_GROUP_LEADER, offsetof(struct task_struct, group_leader));
	DEFINE(TASK_TGID, offsetof(struct task_struct, tgid));
	DEFINE(TASK_STACK, offsetof(struct task_struct, stack));
	BLANK();

	OFFSET(PSTATE_REGS, processor_state, regs);
	OFFSET(PSTATE_FPREGS, processor_state, fpregs);
	OFFSET(PSTATE_FPCR, processor_state, fpcr);
	OFFSET(PSTATE_KTP, processor_state, ktp);
#ifdef CONFIG_HIBERNATION
	OFFSET(PSTATE_SP, processor_state, sp);
#endif
	OFFSET(PBE_ADDR, pbe, address);
	OFFSET(PBE_ORIG_ADDR, pbe, orig_address);
	OFFSET(PBE_NEXT, pbe, next);
	OFFSET(CALLEE_R9, callee_saved_regs, r9);
	OFFSET(CALLEE_R10, callee_saved_regs, r10);
	OFFSET(CALLEE_R11, callee_saved_regs, r11);
	OFFSET(CALLEE_R12, callee_saved_regs, r12);
	OFFSET(CALLEE_R13, callee_saved_regs, r13);
	OFFSET(CALLEE_R14, callee_saved_regs, r14);
	OFFSET(CALLEE_R15, callee_saved_regs, r15);
	OFFSET(CALLEE_RA, callee_saved_regs, ra);
	OFFSET(CALLEE_F2, callee_saved_fpregs, f2);
	OFFSET(CALLEE_F3, callee_saved_fpregs, f3);
	OFFSET(CALLEE_F4, callee_saved_fpregs, f4);
	OFFSET(CALLEE_F5, callee_saved_fpregs, f5);
	OFFSET(CALLEE_F6, callee_saved_fpregs, f6);
	OFFSET(CALLEE_F7, callee_saved_fpregs, f7);
	OFFSET(CALLEE_F8, callee_saved_fpregs, f8);
	OFFSET(CALLEE_F9, callee_saved_fpregs, f9);
	BLANK();
	DEFINE(CRED_UID,  offsetof(struct cred, uid));
	DEFINE(CRED_EUID, offsetof(struct cred, euid));
	DEFINE(CRED_GID,  offsetof(struct cred, gid));
	DEFINE(CRED_EGID, offsetof(struct cred, egid));
	BLANK();

	DEFINE(PT_REGS_SIZE, sizeof(struct pt_regs));
	DEFINE(PT_REGS_R0, offsetof(struct pt_regs, regs[0]));
	DEFINE(PT_REGS_R1, offsetof(struct pt_regs, regs[1]));
	DEFINE(PT_REGS_R2, offsetof(struct pt_regs, regs[2]));
	DEFINE(PT_REGS_R3, offsetof(struct pt_regs, regs[3]));
	DEFINE(PT_REGS_R4, offsetof(struct pt_regs, regs[4]));
	DEFINE(PT_REGS_R5, offsetof(struct pt_regs, regs[5]));
	DEFINE(PT_REGS_R6, offsetof(struct pt_regs, regs[6]));
	DEFINE(PT_REGS_R7, offsetof(struct pt_regs, regs[7]));
	DEFINE(PT_REGS_R8, offsetof(struct pt_regs, regs[8]));
	DEFINE(PT_REGS_R9, offsetof(struct pt_regs, regs[9]));
	DEFINE(PT_REGS_R10, offsetof(struct pt_regs, regs[10]));
	DEFINE(PT_REGS_R11, offsetof(struct pt_regs, regs[11]));
	DEFINE(PT_REGS_R12, offsetof(struct pt_regs, regs[12]));
	DEFINE(PT_REGS_R13, offsetof(struct pt_regs, regs[13]));
	DEFINE(PT_REGS_R14, offsetof(struct pt_regs, regs[14]));
	DEFINE(PT_REGS_R15, offsetof(struct pt_regs, regs[15]));
	DEFINE(PT_REGS_R16, offsetof(struct pt_regs, regs[16]));
	DEFINE(PT_REGS_R17, offsetof(struct pt_regs, regs[17]));
	DEFINE(PT_REGS_R18, offsetof(struct pt_regs, regs[18]));
	DEFINE(PT_REGS_R19, offsetof(struct pt_regs, regs[19]));
	DEFINE(PT_REGS_R20, offsetof(struct pt_regs, regs[20]));
	DEFINE(PT_REGS_R21, offsetof(struct pt_regs, regs[21]));
	DEFINE(PT_REGS_R22, offsetof(struct pt_regs, regs[22]));
	DEFINE(PT_REGS_R23, offsetof(struct pt_regs, regs[23]));
	DEFINE(PT_REGS_R24, offsetof(struct pt_regs, regs[24]));
	DEFINE(PT_REGS_R25, offsetof(struct pt_regs, regs[25]));
	DEFINE(PT_REGS_R26, offsetof(struct pt_regs, regs[26]));
	DEFINE(PT_REGS_R27, offsetof(struct pt_regs, regs[27]));
	DEFINE(PT_REGS_R28, offsetof(struct pt_regs, regs[28]));
	DEFINE(PT_REGS_GP, offsetof(struct pt_regs, regs[29]));
	DEFINE(PT_REGS_SP, offsetof(struct pt_regs, regs[30]));
	DEFINE(PT_REGS_PC, offsetof(struct pt_regs, pc));
	DEFINE(PT_REGS_PS, offsetof(struct pt_regs, ps));
	DEFINE(PT_REGS_ORIG_R0, offsetof(struct pt_regs, orig_r0));
	DEFINE(PT_REGS_ORIG_R19, offsetof(struct pt_regs, orig_r19));
	DEFINE(PT_REGS_CAUSE, offsetof(struct pt_regs, cause));
	DEFINE(PT_REGS_HM_PS, offsetof(struct pt_regs, hm_ps));
	DEFINE(PT_REGS_HM_PC, offsetof(struct pt_regs, hm_pc));
	DEFINE(PT_REGS_HM_GP, offsetof(struct pt_regs, hm_gp));
	DEFINE(PT_REGS_EARG0, offsetof(struct pt_regs, earg0));
	DEFINE(PT_REGS_EARG1, offsetof(struct pt_regs, earg1));
	DEFINE(PT_REGS_EARG2, offsetof(struct pt_regs, earg2));
	BLANK();

	DEFINE(KVM_REGS_SIZE, sizeof(struct kvm_regs));
#ifdef CONFIG_SUBARCH_C3B
	DEFINE(KVM_REGS_R0, offsetof(struct kvm_regs, r0));
	DEFINE(KVM_REGS_R1, offsetof(struct kvm_regs, r1));
	DEFINE(KVM_REGS_R2, offsetof(struct kvm_regs, r2));
	DEFINE(KVM_REGS_R3, offsetof(struct kvm_regs, r3));
	DEFINE(KVM_REGS_R4, offsetof(struct kvm_regs, r4));
	DEFINE(KVM_REGS_R5, offsetof(struct kvm_regs, r5));
	DEFINE(KVM_REGS_R6, offsetof(struct kvm_regs, r6));
	DEFINE(KVM_REGS_R7, offsetof(struct kvm_regs, r7));
	DEFINE(KVM_REGS_R8, offsetof(struct kvm_regs, r8));
	DEFINE(KVM_REGS_R9, offsetof(struct kvm_regs, r9));
	DEFINE(KVM_REGS_R10, offsetof(struct kvm_regs, r10));
	DEFINE(KVM_REGS_R11, offsetof(struct kvm_regs, r11));
	DEFINE(KVM_REGS_R12, offsetof(struct kvm_regs, r12));
	DEFINE(KVM_REGS_R13, offsetof(struct kvm_regs, r13));
	DEFINE(KVM_REGS_R14, offsetof(struct kvm_regs, r14));
	DEFINE(KVM_REGS_R15, offsetof(struct kvm_regs, r15));
	DEFINE(KVM_REGS_R16, offsetof(struct kvm_regs, r16));
	DEFINE(KVM_REGS_R17, offsetof(struct kvm_regs, r17));
	DEFINE(KVM_REGS_R18, offsetof(struct kvm_regs, r18));
	DEFINE(KVM_REGS_R19, offsetof(struct kvm_regs, r19));
	DEFINE(KVM_REGS_R20, offsetof(struct kvm_regs, r20));
	DEFINE(KVM_REGS_R21, offsetof(struct kvm_regs, r21));
	DEFINE(KVM_REGS_R22, offsetof(struct kvm_regs, r22));
	DEFINE(KVM_REGS_R23, offsetof(struct kvm_regs, r23));
	DEFINE(KVM_REGS_R24, offsetof(struct kvm_regs, r24));
	DEFINE(KVM_REGS_R25, offsetof(struct kvm_regs, r25));
	DEFINE(KVM_REGS_R26, offsetof(struct kvm_regs, r26));
	DEFINE(KVM_REGS_R27, offsetof(struct kvm_regs, r27));
	DEFINE(KVM_REGS_R28, offsetof(struct kvm_regs, r28));
	DEFINE(KVM_REGS_GP, offsetof(struct kvm_regs, gp));
	DEFINE(KVM_REGS_FPCR, offsetof(struct kvm_regs, fpcr));
	DEFINE(KVM_REGS_F0, offsetof(struct kvm_regs, fp[0 * 4]));
	DEFINE(KVM_REGS_F1, offsetof(struct kvm_regs, fp[1 * 4]));
	DEFINE(KVM_REGS_F2, offsetof(struct kvm_regs, fp[2 * 4]));
	DEFINE(KVM_REGS_F3, offsetof(struct kvm_regs, fp[3 * 4]));
	DEFINE(KVM_REGS_F4, offsetof(struct kvm_regs, fp[4 * 4]));
	DEFINE(KVM_REGS_F5, offsetof(struct kvm_regs, fp[5 * 4]));
	DEFINE(KVM_REGS_F6, offsetof(struct kvm_regs, fp[6 * 4]));
	DEFINE(KVM_REGS_F7, offsetof(struct kvm_regs, fp[7 * 4]));
	DEFINE(KVM_REGS_F8, offsetof(struct kvm_regs, fp[8 * 4]));
	DEFINE(KVM_REGS_F9, offsetof(struct kvm_regs, fp[9 * 4]));
	DEFINE(KVM_REGS_F10, offsetof(struct kvm_regs, fp[10 * 4]));
	DEFINE(KVM_REGS_F11, offsetof(struct kvm_regs, fp[11 * 4]));
	DEFINE(KVM_REGS_F12, offsetof(struct kvm_regs, fp[12 * 4]));
	DEFINE(KVM_REGS_F13, offsetof(struct kvm_regs, fp[13 * 4]));
	DEFINE(KVM_REGS_F14, offsetof(struct kvm_regs, fp[14 * 4]));
	DEFINE(KVM_REGS_F15, offsetof(struct kvm_regs, fp[15 * 4]));
	DEFINE(KVM_REGS_F16, offsetof(struct kvm_regs, fp[16 * 4]));
	DEFINE(KVM_REGS_F17, offsetof(struct kvm_regs, fp[17 * 4]));
	DEFINE(KVM_REGS_F18, offsetof(struct kvm_regs, fp[18 * 4]));
	DEFINE(KVM_REGS_F19, offsetof(struct kvm_regs, fp[19 * 4]));
	DEFINE(KVM_REGS_F20, offsetof(struct kvm_regs, fp[20 * 4]));
	DEFINE(KVM_REGS_F21, offsetof(struct kvm_regs, fp[21 * 4]));
	DEFINE(KVM_REGS_F22, offsetof(struct kvm_regs, fp[22 * 4]));
	DEFINE(KVM_REGS_F23, offsetof(struct kvm_regs, fp[23 * 4]));
	DEFINE(KVM_REGS_F24, offsetof(struct kvm_regs, fp[24 * 4]));
	DEFINE(KVM_REGS_F25, offsetof(struct kvm_regs, fp[25 * 4]));
	DEFINE(KVM_REGS_F26, offsetof(struct kvm_regs, fp[26 * 4]));
	DEFINE(KVM_REGS_F27, offsetof(struct kvm_regs, fp[27 * 4]));
	DEFINE(KVM_REGS_F28, offsetof(struct kvm_regs, fp[28 * 4]));
	DEFINE(KVM_REGS_F29, offsetof(struct kvm_regs, fp[29 * 4]));
	DEFINE(KVM_REGS_F30, offsetof(struct kvm_regs, fp[30 * 4]));
	DEFINE(KVM_REGS_PS, offsetof(struct kvm_regs, ps));
	DEFINE(KVM_REGS_PC, offsetof(struct kvm_regs, pc));
#elif CONFIG_SUBARCH_C4
	DEFINE(KVM_REGS_R0, offsetof(struct kvm_regs, regs.regs[0]));
	DEFINE(KVM_REGS_R1, offsetof(struct kvm_regs, regs.regs[1]));
	DEFINE(KVM_REGS_R2, offsetof(struct kvm_regs, regs.regs[2]));
	DEFINE(KVM_REGS_R3, offsetof(struct kvm_regs, regs.regs[3]));
	DEFINE(KVM_REGS_R4, offsetof(struct kvm_regs, regs.regs[4]));
	DEFINE(KVM_REGS_R5, offsetof(struct kvm_regs, regs.regs[5]));
	DEFINE(KVM_REGS_R6, offsetof(struct kvm_regs, regs.regs[6]));
	DEFINE(KVM_REGS_R7, offsetof(struct kvm_regs, regs.regs[7]));
	DEFINE(KVM_REGS_R8, offsetof(struct kvm_regs, regs.regs[8]));
	DEFINE(KVM_REGS_R9, offsetof(struct kvm_regs, regs.regs[9]));
	DEFINE(KVM_REGS_R10, offsetof(struct kvm_regs, regs.regs[10]));
	DEFINE(KVM_REGS_R11, offsetof(struct kvm_regs, regs.regs[11]));
	DEFINE(KVM_REGS_R12, offsetof(struct kvm_regs, regs.regs[12]));
	DEFINE(KVM_REGS_R13, offsetof(struct kvm_regs, regs.regs[13]));
	DEFINE(KVM_REGS_R14, offsetof(struct kvm_regs, regs.regs[14]));
	DEFINE(KVM_REGS_R15, offsetof(struct kvm_regs, regs.regs[15]));
	DEFINE(KVM_REGS_R16, offsetof(struct kvm_regs, regs.regs[16]));
	DEFINE(KVM_REGS_R17, offsetof(struct kvm_regs, regs.regs[17]));
	DEFINE(KVM_REGS_R18, offsetof(struct kvm_regs, regs.regs[18]));
	DEFINE(KVM_REGS_R19, offsetof(struct kvm_regs, regs.regs[19]));
	DEFINE(KVM_REGS_R20, offsetof(struct kvm_regs, regs.regs[20]));
	DEFINE(KVM_REGS_R21, offsetof(struct kvm_regs, regs.regs[21]));
	DEFINE(KVM_REGS_R22, offsetof(struct kvm_regs, regs.regs[22]));
	DEFINE(KVM_REGS_R23, offsetof(struct kvm_regs, regs.regs[23]));
	DEFINE(KVM_REGS_R24, offsetof(struct kvm_regs, regs.regs[24]));
	DEFINE(KVM_REGS_R25, offsetof(struct kvm_regs, regs.regs[25]));
	DEFINE(KVM_REGS_R26, offsetof(struct kvm_regs, regs.regs[26]));
	DEFINE(KVM_REGS_R27, offsetof(struct kvm_regs, regs.regs[27]));
	DEFINE(KVM_REGS_R28, offsetof(struct kvm_regs, regs.regs[28]));
	DEFINE(KVM_REGS_GP, offsetof(struct kvm_regs, regs.regs[29]));
	DEFINE(KVM_REGS_SP, offsetof(struct kvm_regs, regs.regs[30]));
	DEFINE(KVM_REGS_PC, offsetof(struct kvm_regs, regs.pc));
	DEFINE(KVM_REGS_PS, offsetof(struct kvm_regs, regs.pstate));
	DEFINE(KVM_REGS_F0, offsetof(struct kvm_regs, fpstate.fp[0]));
	DEFINE(KVM_REGS_F1, offsetof(struct kvm_regs, fpstate.fp[1]));
	DEFINE(KVM_REGS_F2, offsetof(struct kvm_regs, fpstate.fp[2]));
	DEFINE(KVM_REGS_F3, offsetof(struct kvm_regs, fpstate.fp[3]));
	DEFINE(KVM_REGS_F4, offsetof(struct kvm_regs, fpstate.fp[4]));
	DEFINE(KVM_REGS_F5, offsetof(struct kvm_regs, fpstate.fp[5]));
	DEFINE(KVM_REGS_F6, offsetof(struct kvm_regs, fpstate.fp[6]));
	DEFINE(KVM_REGS_F7, offsetof(struct kvm_regs, fpstate.fp[7]));
	DEFINE(KVM_REGS_F8, offsetof(struct kvm_regs, fpstate.fp[8]));
	DEFINE(KVM_REGS_F9, offsetof(struct kvm_regs, fpstate.fp[9]));
	DEFINE(KVM_REGS_F10, offsetof(struct kvm_regs, fpstate.fp[10]));
	DEFINE(KVM_REGS_F11, offsetof(struct kvm_regs, fpstate.fp[11]));
	DEFINE(KVM_REGS_F12, offsetof(struct kvm_regs, fpstate.fp[12]));
	DEFINE(KVM_REGS_F13, offsetof(struct kvm_regs, fpstate.fp[13]));
	DEFINE(KVM_REGS_F14, offsetof(struct kvm_regs, fpstate.fp[14]));
	DEFINE(KVM_REGS_F15, offsetof(struct kvm_regs, fpstate.fp[15]));
	DEFINE(KVM_REGS_F16, offsetof(struct kvm_regs, fpstate.fp[16]));
	DEFINE(KVM_REGS_F17, offsetof(struct kvm_regs, fpstate.fp[17]));
	DEFINE(KVM_REGS_F18, offsetof(struct kvm_regs, fpstate.fp[18]));
	DEFINE(KVM_REGS_F19, offsetof(struct kvm_regs, fpstate.fp[19]));
	DEFINE(KVM_REGS_F20, offsetof(struct kvm_regs, fpstate.fp[20]));
	DEFINE(KVM_REGS_F21, offsetof(struct kvm_regs, fpstate.fp[21]));
	DEFINE(KVM_REGS_F22, offsetof(struct kvm_regs, fpstate.fp[22]));
	DEFINE(KVM_REGS_F23, offsetof(struct kvm_regs, fpstate.fp[23]));
	DEFINE(KVM_REGS_F24, offsetof(struct kvm_regs, fpstate.fp[24]));
	DEFINE(KVM_REGS_F25, offsetof(struct kvm_regs, fpstate.fp[25]));
	DEFINE(KVM_REGS_F26, offsetof(struct kvm_regs, fpstate.fp[26]));
	DEFINE(KVM_REGS_F27, offsetof(struct kvm_regs, fpstate.fp[27]));
	DEFINE(KVM_REGS_F28, offsetof(struct kvm_regs, fpstate.fp[28]));
	DEFINE(KVM_REGS_F29, offsetof(struct kvm_regs, fpstate.fp[29]));
	DEFINE(KVM_REGS_F30, offsetof(struct kvm_regs, fpstate.fp[30]));
	DEFINE(KVM_REGS_FPCR, offsetof(struct kvm_regs, fpstate.fpcr));
#endif
	BLANK();

	DEFINE(VCPU_RET_SIZE, sizeof(struct vcpu_run_ret_stack));
	DEFINE(VCPU_RET_RA, offsetof(struct vcpu_run_ret_stack, ra));
	DEFINE(VCPU_RET_R0, offsetof(struct vcpu_run_ret_stack, r0));
	BLANK();

	DEFINE(HOST_INT_SIZE, sizeof(struct host_int_args));
	DEFINE(HOST_INT_R18, offsetof(struct host_int_args, r18));
	DEFINE(HOST_INT_R17, offsetof(struct host_int_args, r17));
	DEFINE(HOST_INT_R16, offsetof(struct host_int_args, r16));
	BLANK();

	OFFSET(TASK_THREAD, task_struct, thread);
	OFFSET(TASK_THREAD_F0, task_struct, thread.fpstate.fp[0]);
	OFFSET(TASK_THREAD_F1, task_struct, thread.fpstate.fp[1]);
	OFFSET(TASK_THREAD_F2, task_struct, thread.fpstate.fp[2]);
	OFFSET(TASK_THREAD_F3, task_struct, thread.fpstate.fp[3]);
	OFFSET(TASK_THREAD_F4, task_struct, thread.fpstate.fp[4]);
	OFFSET(TASK_THREAD_F5, task_struct, thread.fpstate.fp[5]);
	OFFSET(TASK_THREAD_F6, task_struct, thread.fpstate.fp[6]);
	OFFSET(TASK_THREAD_F7, task_struct, thread.fpstate.fp[7]);
	OFFSET(TASK_THREAD_F8, task_struct, thread.fpstate.fp[8]);
	OFFSET(TASK_THREAD_F9, task_struct, thread.fpstate.fp[9]);
	OFFSET(TASK_THREAD_F10, task_struct, thread.fpstate.fp[10]);
	OFFSET(TASK_THREAD_F11, task_struct, thread.fpstate.fp[11]);
	OFFSET(TASK_THREAD_F12, task_struct, thread.fpstate.fp[12]);
	OFFSET(TASK_THREAD_F13, task_struct, thread.fpstate.fp[13]);
	OFFSET(TASK_THREAD_F14, task_struct, thread.fpstate.fp[14]);
	OFFSET(TASK_THREAD_F15, task_struct, thread.fpstate.fp[15]);
	OFFSET(TASK_THREAD_F16, task_struct, thread.fpstate.fp[16]);
	OFFSET(TASK_THREAD_F17, task_struct, thread.fpstate.fp[17]);
	OFFSET(TASK_THREAD_F18, task_struct, thread.fpstate.fp[18]);
	OFFSET(TASK_THREAD_F19, task_struct, thread.fpstate.fp[19]);
	OFFSET(TASK_THREAD_F20, task_struct, thread.fpstate.fp[20]);
	OFFSET(TASK_THREAD_F21, task_struct, thread.fpstate.fp[21]);
	OFFSET(TASK_THREAD_F22, task_struct, thread.fpstate.fp[22]);
	OFFSET(TASK_THREAD_F23, task_struct, thread.fpstate.fp[23]);
	OFFSET(TASK_THREAD_F24, task_struct, thread.fpstate.fp[24]);
	OFFSET(TASK_THREAD_F25, task_struct, thread.fpstate.fp[25]);
	OFFSET(TASK_THREAD_F26, task_struct, thread.fpstate.fp[26]);
	OFFSET(TASK_THREAD_F27, task_struct, thread.fpstate.fp[27]);
	OFFSET(TASK_THREAD_F28, task_struct, thread.fpstate.fp[28]);
	OFFSET(TASK_THREAD_F29, task_struct, thread.fpstate.fp[29]);
	OFFSET(TASK_THREAD_F30, task_struct, thread.fpstate.fp[30]);
	OFFSET(TASK_THREAD_FPCR, task_struct, thread.fpstate.fpcr);
	BLANK();
	OFFSET(TASK_THREAD_RA, task_struct, thread.ra);
	OFFSET(TASK_THREAD_SP, task_struct, thread.sp);
	OFFSET(TASK_THREAD_S0, task_struct, thread.s[0]);
	OFFSET(TASK_THREAD_S1, task_struct, thread.s[1]);
	OFFSET(TASK_THREAD_S2, task_struct, thread.s[2]);
	OFFSET(TASK_THREAD_S3, task_struct, thread.s[3]);
	OFFSET(TASK_THREAD_S4, task_struct, thread.s[4]);
	OFFSET(TASK_THREAD_S5, task_struct, thread.s[5]);
	OFFSET(TASK_THREAD_S6, task_struct, thread.s[6]);
	BLANK();
	DEFINE(ASM_THREAD_SIZE, THREAD_SIZE);
	BLANK();
	DEFINE(RT_SIGFRAME_SIZE, sizeof(struct rt_sigframe));
	OFFSET(RT_SIGFRAME_MCTX, rt_sigframe, uc.uc_mcontext);
	BLANK();
#ifdef CONFIG_FRAME_POINTER
	DEFINE(STACKFRAME_SIZE, sizeof(struct stackframe));
	OFFSET(STACKFRAME_PC, stackframe, pc);
	OFFSET(STACKFRAME_FP, stackframe, fp);
#else
	DEFINE(STACKFRAME_SIZE, 0);
#endif
}

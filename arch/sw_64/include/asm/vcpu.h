/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_VCPU_H
#define _ASM_SW64_VCPU_H

#ifndef __ASSEMBLY__

#ifdef CONFIG_SUBARCH_C3B

struct vcpucb {
	unsigned long go_flag;
	unsigned long pcbb;
	unsigned long ksp;
	unsigned long usp;
	unsigned long kgp;
	unsigned long ent_arith;
	unsigned long ent_if;
	unsigned long ent_int;
	unsigned long ent_mm;
	unsigned long ent_sys;
	unsigned long ent_una;
	unsigned long stack_pc;
	unsigned long new_a0;
	unsigned long new_a1;
	unsigned long new_a2;
	unsigned long soft_cid;
	unsigned long csr_save;
	unsigned long wakeup_magic;
	unsigned long host_vcpucb;
	unsigned long upcr;
	unsigned long vpcr;
	unsigned long dtb_vpcr;
	unsigned long guest_ksp;
	unsigned long guest_usp;
	unsigned long vcpu_irq_disabled;
	unsigned long vcpu_irq;
	unsigned long ptbr;
	unsigned long soft_tid;
	unsigned long int_stat1;
	unsigned long int_stat2;
	unsigned long int_stat3;
	unsigned long reset_entry;
	unsigned long pvcpu;
	unsigned long exit_reason;
	unsigned long ipaddr;
	unsigned long vcpu_irq_vector;
	unsigned long pri_base;
	unsigned long stack_pc_dfault;
	unsigned long guest_p20;
	unsigned long guest_dfault_double;
	unsigned long guest_irqs_pending;
	unsigned long guest_hm_r30;
	unsigned long migration_mark;
	unsigned long guest_longtime;
	unsigned long guest_longtime_offset;
	unsigned long reserved[3];
};

#elif CONFIG_SUBARCH_C4

struct vcpucb {
	unsigned long ktp;
	unsigned long as_info;
	unsigned long ksp;
	unsigned long usp;
	unsigned long kgp;
	unsigned long ent_arith;
	unsigned long ent_if;
	unsigned long ent_int;
	unsigned long ent_mm;
	unsigned long ent_sys;
	unsigned long ent_una;
	unsigned long stack_pc;
	unsigned long new_a0;
	unsigned long new_a1;
	unsigned long new_a2;
	unsigned long soft_cid;
	unsigned long csr_save;
	unsigned long wakeup_magic;
	unsigned long host_vcpucb;
	unsigned long upcr;
	unsigned long vpcr;
	unsigned long dtb_vpcr;
	unsigned long dtb_upcr;
	unsigned long guest_ksp;
	unsigned long guest_usp;
	unsigned long vcpu_irq_disabled;
	unsigned long vcpu_irq;
	unsigned long ptbr_usr;
	unsigned long ptbr_sys;
	unsigned long soft_tid;
	unsigned long int_stat0;
	unsigned long int_stat1;
	unsigned long int_stat2;
	unsigned long int_stat3;
	unsigned long reset_entry;
	unsigned long pvcpu;
	unsigned long exit_reason;
	unsigned long fault_gpa; /* CSR:EXC_GPA */
	unsigned long vcpu_pc_save;
	unsigned long migration_mark;
	unsigned long shtclock;
	unsigned long shtclock_offset;
	unsigned long csr_pc;
	unsigned long csr_ps;
	unsigned long csr_sp;
	unsigned long csr_ktp;
	unsigned long csr_earg0;
	unsigned long csr_earg1;
	unsigned long csr_earg2;
	unsigned long csr_scratch;
	unsigned long atc;
	unsigned long reserved[45];
};
#endif

#endif /* __ASSEMBLY__ */
#endif /* _ASM_SW64_VCPU_H */

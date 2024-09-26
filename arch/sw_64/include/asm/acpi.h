/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _ASM_SW64_ACPI_H
#define _ASM_SW64_ACPI_H

#include <asm/processor.h>
#include <asm/mmu.h>
#include <asm/numa.h>
#include <asm/early_ioremap.h>
#include <asm/smp.h>

#ifdef CONFIG_ACPI
extern int acpi_noirq;
extern int acpi_strict;
extern int acpi_disabled;
extern int acpi_pci_disabled;

/* _ASM_SW64_PDC_H */
#define ACPI_PDC_P_FFH                  (0x0001)
#define ACPI_PDC_C_C1_HALT              (0x0002)
#define ACPI_PDC_T_FFH                  (0x0004)
#define ACPI_PDC_SMP_C1PT               (0x0008)
#define ACPI_PDC_SMP_C2C3               (0x0010)
#define ACPI_PDC_SMP_P_SWCOORD          (0x0020)
#define ACPI_PDC_SMP_C_SWCOORD          (0x0040)
#define ACPI_PDC_SMP_T_SWCOORD          (0x0080)
#define ACPI_PDC_C_C1_FFH               (0x0100)
#define ACPI_PDC_C_C2C3_FFH             (0x0200)
#define ACPI_PDC_SMP_P_HWCOORD          (0x0800)

#define ACPI_PDC_EST_CAPABILITY_SMP     (ACPI_PDC_SMP_C1PT | \
					ACPI_PDC_C_C1_HALT | \
					ACPI_PDC_P_FFH)

#define ACPI_PDC_EST_CAPABILITY_SWSMP   (ACPI_PDC_SMP_C1PT | \
					ACPI_PDC_C_C1_HALT | \
					ACPI_PDC_SMP_P_SWCOORD | \
					ACPI_PDC_SMP_P_HWCOORD | \
					ACPI_PDC_P_FFH)

#define ACPI_PDC_C_CAPABILITY_SMP	(ACPI_PDC_SMP_C2C3 | \
					ACPI_PDC_SMP_C1PT  | \
					ACPI_PDC_C_C1_HALT | \
					ACPI_PDC_C_C1_FFH  | \
					ACPI_PDC_C_C2C3_FFH)

#define ACPI_TABLE_UPGRADE_MAX_PHYS MEMBLOCK_ALLOC_ACCESSIBLE

/**
 * Use the number 64 is just because this number is the most
 * frequently used number in other architectures. Actually,
 * SW64 does not have fixmap area in memory layout.
 */
#define NR_FIX_BTMAPS 64

static inline void disable_acpi(void)
{
	acpi_disabled = 1;
	acpi_pci_disabled = 1;
	acpi_noirq = 1;
}

static inline void enable_acpi(void)
{
	acpi_disabled = 0;
	acpi_pci_disabled = 0;
	acpi_noirq = 0;
}

static inline void acpi_noirq_set(void)
{
	acpi_noirq = 1;
}

static inline void acpi_disable_pci(void)
{
	acpi_pci_disabled = 1;
	acpi_noirq_set();
}

static inline bool acpi_has_cpu_in_madt(void)
{
	return true;
}

/* Low-level suspend routine. */
extern int (*acpi_suspend_lowlevel)(void);
extern unsigned long long arch_acpi_wakeup_start;

/* Physical address to resume after wakeup */
#define acpi_wakeup_address arch_acpi_wakeup_start

/*
 * Check if the CPU can handle C2 and deeper
 */
static inline unsigned int acpi_processor_cstate_check(unsigned int max_cstate)
{
	return max_cstate;
}

static inline bool arch_has_acpi_pdc(void)
{
	return false;
}

static inline void arch_acpi_set_pdc_bits(u32 *buf)
{
}

static inline u32 get_acpi_id_for_cpu(unsigned int cpu)
{
	/* We take rcid as processor _UID */
	return cpu_physical_id(cpu);
}

static inline unsigned long acpi_get_wakeup_address(void)
{
	return 0;
}

static inline bool acpi_skip_set_wakeup_address(void)
{
	return true;
}

#define acpi_skip_set_wakeup_address acpi_skip_set_wakeup_address
#else /* !CONFIG_ACPI */

static inline void acpi_noirq_set(void) { }
static inline void acpi_disable_pci(void) { }
static inline void disable_acpi(void) { }

#endif /* !CONFIG_ACPI */

#define acpi_unlazy_tlb(x)
#endif /* _ASM_SW64_ACPI_H */

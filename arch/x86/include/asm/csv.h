/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Hygon China Secure Virtualization (CSV)
 *
 * Copyright (C) Hygon Info Technologies Ltd.
 *
 * Author: Jiang Xin <jiangxin@hygon.cn>
 */

#ifndef __ASM_X86_CSV_H__
#define __ASM_X86_CSV_H__

#ifndef __ASSEMBLY__

#ifdef CONFIG_HYGON_CSV

struct csv_mem {
	uint64_t start;
	uint64_t size;
};

#define CSV_MR_ALIGN_BITS		(28)

extern struct csv_mem *csv_smr;
extern unsigned int csv_smr_num;

void __init early_csv_reserve_mem(void);

phys_addr_t csv_alloc_from_contiguous(size_t size, nodemask_t *nodes_allowed,
				      unsigned int align);
void csv_release_to_contiguous(phys_addr_t pa, size_t size);

uint32_t csv_get_smr_entry_shift(void);

#else	/* !CONFIG_HYGON_CSV */

#define csv_smr		NULL
#define csv_smr_num	0U

static inline void __init early_csv_reserve_mem(void) { }

static inline phys_addr_t
csv_alloc_from_contiguous(size_t size, nodemask_t *nodes_allowed,
			  unsigned int align) { return 0; }
static inline void csv_release_to_contiguous(phys_addr_t pa, size_t size) { }

static inline uint32_t csv_get_smr_entry_shift(void) { return 0; }

#endif	/* CONFIG_HYGON_CSV */

#define CPUID_VENDOR_HygonGenuine_ebx	0x6f677948
#define CPUID_VENDOR_HygonGenuine_ecx	0x656e6975
#define CPUID_VENDOR_HygonGenuine_edx	0x6e65476e

#define MSR_CSV3_ENABLED_BIT		30
#define MSR_CSV3_ENABLED		BIT_ULL(MSR_CSV3_ENABLED_BIT)

#ifdef CONFIG_HYGON_CSV

bool csv3_active(void);

void __init csv_early_reset_memory(struct boot_params *bp);
void __init csv_early_update_memory_enc(u64 vaddr, u64 pages);
void __init csv_early_update_memory_dec(u64 vaddr, u64 pages);

void __init csv_early_memory_enc_dec(u64 vaddr, u64 size, bool enc);

#else	/* !CONFIG_HYGON_CSV */

static inline bool csv3_active(void) { return false; }

static inline void __init csv_early_reset_memory(struct boot_params *bp) { }
static inline void __init csv_early_update_memory_enc(u64 vaddr, u64 pages) { }
static inline void __init csv_early_update_memory_dec(u64 vaddr, u64 pages) { }

static inline void __init csv_early_memory_enc_dec(u64 vaddr, u64 size,
						   bool enc) { }

#endif	/* CONFIG_HYGON_CSV */

#endif	/* __ASSEMBLY__ */

#endif	/* __ASM_X86_CSV_H__ */

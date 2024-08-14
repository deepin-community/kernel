/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_HW_INIT_H
#define _ASM_SW64_HW_INIT_H
#include <linux/numa.h>
#include <linux/jump_label.h>
#include <linux/cpumask.h>

#include <asm/cache.h>

#define MM_SIZE		__va(0x2040)
#define VPCR_SHIFT	44

#define MAX_NUMSOCKETS		8
struct socket_desc_t {
	bool is_online;	/* 1 for online, 0 for offline */
	int numcores;
	unsigned long socket_mem;
};

enum memmap_types {
	memmap_reserved,
	memmap_pci,
	memmap_initrd,
	memmap_kvm,
	memmap_crashkernel,
	memmap_acpi,
	memmap_use,
	memmap_protected,
};

#define MAX_NUMMEMMAPS		64
struct memmap_entry {
	u64 addr;	/* start of memory segment */
	u64 size;	/* size of memory segment */
	enum memmap_types type;
};

extern struct socket_desc_t socket_desc[MAX_NUMSOCKETS];
extern int memmap_nr;
extern struct memmap_entry memmap_map[MAX_NUMMEMMAPS];
extern bool memblock_initialized;

int __init add_memmap_region(u64 addr, u64 size, enum memmap_types type);
void __init process_memmap(void);

#define EMUL_FLAG	(0x1UL << 63)
#define MM_SIZE_MASK	(EMUL_FLAG - 1)

DECLARE_STATIC_KEY_TRUE(run_mode_host_key);
DECLARE_STATIC_KEY_FALSE(run_mode_guest_key);
DECLARE_STATIC_KEY_FALSE(run_mode_emul_key);

DECLARE_STATIC_KEY_FALSE(hw_una_enabled);
DECLARE_STATIC_KEY_FALSE(junzhang_v1_key);
DECLARE_STATIC_KEY_FALSE(junzhang_v2_key);
DECLARE_STATIC_KEY_FALSE(junzhang_v3_key);

#define is_in_host()		static_branch_likely(&run_mode_host_key)
#define is_in_guest()		static_branch_unlikely(&run_mode_guest_key)
#define is_in_emul()		static_branch_unlikely(&run_mode_emul_key)
#define is_guest_or_emul()	!static_branch_likely(&run_mode_host_key)
#define is_junzhang_v1()	static_branch_unlikely(&junzhang_v1_key)
#define is_junzhang_v2()	static_branch_likely(&junzhang_v2_key)
#define is_junzhang_v3()	static_branch_unlikely(&junzhang_v3_key)

#endif /* _ASM_SW64_HW_INIT_H */

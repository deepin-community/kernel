/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _ASM_SW64_EFI_H
#define _ASM_SW64_EFI_H

#include <linux/efi.h>

#include <asm/io.h>
#include <asm/early_ioremap.h>

#ifdef CONFIG_EFI

extern void efi_init(void);
extern unsigned long entSuspend;

#define SLEEP_ENTRY_GUID	EFI_GUID(0x59cb76bb, 0x9c3a, 0x4c8f, 0xbd, 0x5c, 0xc0, 0x0f, 0x20, 0x61, 0x18, 0x4b)
#define BIOS_VERSION_GUID	EFI_GUID(0xc47a23c3, 0xcebb, 0x4cc9, 0xa5, 0xe2, 0xde, 0xd0, 0x8f, 0xe4, 0x20, 0xb5)

extern unsigned long sunway_bios_version;

#else
#define efi_init()
#define efi_idmap_init()
#define sunway_bios_version	(0)
#endif /* CONFIG_EFI */

#ifdef CONFIG_SW64_KERNEL_PAGE_TABLE
int efi_create_mapping(struct mm_struct *mm, efi_memory_desc_t *md);
#define arch_efi_call_virt_setup()	efi_virtmap_load()
#define arch_efi_call_virt_teardown()	efi_virtmap_unload()
void efi_virtmap_load(void);
void efi_virtmap_unload(void);
#else
#define arch_efi_call_virt_setup()
#define arch_efi_call_virt_teardown()
#endif

#define ARCH_EFI_IRQ_FLAGS_MASK		0x00000001

/* arch specific definitions used by the stub code */

#endif /* _ASM_SW64_EFI_H */

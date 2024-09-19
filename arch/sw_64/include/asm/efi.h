/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_EFI_H
#define _ASM_SW64_EFI_H

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

#define arch_efi_call_virt_setup()
#define arch_efi_call_virt_teardown()

#define ARCH_EFI_IRQ_FLAGS_MASK		0x00000001

/* arch specific definitions used by the stub code */

/*
 * AArch64 requires the DTB to be 8-byte aligned in the first 512MiB from
 * start of kernel and may not cross a 2MiB boundary. We set alignment to
 * 2MiB so we know it won't cross a 2MiB boundary.
 */
#define EFI_FDT_ALIGN	SZ_2M   /* used by allocate_new_fdt_and_exit_boot() */
#define MAX_FDT_OFFSET	SZ_512M

#define efi_call_early(f, ...) sys_table_arg->boottime->f(__VA_ARGS__)

#endif /* _ASM_SW64_EFI_H */

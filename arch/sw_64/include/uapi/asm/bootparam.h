/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_ASM_SW64_BOOTPARAM_H
#define _UAPI_ASM_SW64_BOOTPARAM_H

#ifndef __ASSEMBLY__

#include <linux/types.h>

struct boot_params {
	__u64 initrd_start;			/* logical address of initrd */
	__u64 initrd_size;			/* size of initrd */
	__u64 dtb_start;			/* logical address of dtb */
	__u64 efi_systab;			/* logical address of EFI system table */
	__u64 efi_memmap;			/* logical address of EFI memory map */
	__u64 efi_memmap_size;			/* size of EFI memory map */
	__u64 efi_memdesc_size;			/* size of an EFI memory map descriptor */
	__u64 efi_memdesc_version;		/* memory descriptor version */
	__u64 cmdline;				/* logical address of cmdline */
};
#endif

#endif /* _UAPI_ASM_SW64_BOOTPARAM_H */

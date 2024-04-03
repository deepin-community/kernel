/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_SETUP_H
#define _ASM_SW64_SETUP_H

#include <uapi/asm/setup.h>

/*
 * We leave one page for the initial stack page, and one page for
 * the initial process structure. Also, the console eats 3 MB for
 * the initial bootloader (one of which we can reclaim later).
 */
#define BOOT_PCB		0x20000000
#define BOOT_ADDR		0x20000000
/* Remove when official MILO sources have ELF support: */
#define BOOT_SIZE		(16 * 1024)

#define KERNEL_START_PHYS	CONFIG_PHYSICAL_START
#define KERNEL_START		(__START_KERNEL_map + CONFIG_PHYSICAL_START)

/* INIT_STACK may be used for merging lwk to kernel*/
#define INIT_STACK		(KERNEL_START + 0x02000)

/*
 * This is setup by the secondary bootstrap loader.  Because
 * the zero page is zeroed out as soon as the vm system is
 * initialized, we need to copy things out into a more permanent
 * place.
 */
#define PARAM			(KERNEL_START + 0x0A000)
#define COMMAND_LINE		((char *)(KERNEL_START + 0x0B000))
#define INITRD_START		(*(unsigned long *)(PARAM + 0x100))
#define INITRD_SIZE		(*(unsigned long *)(PARAM + 0x108))
#define DTB_START		(*(unsigned long *)(PARAM + 0x118))

#define _TEXT_START		(KERNEL_START + 0x10000)

#define COMMAND_LINE_OFF	(0x10000UL - 0xB000UL)
#define INITRD_START_OFF	(0x10000UL - 0xA100UL)
#define INITRD_SIZE_OFF		(0x10000UL - 0xA108UL)

/**
 * Motherboard Configuration Tables
 *
 * Starting from junzhang, we will not directly access Motherboard
 * Configuration Tables in kernel. These macros and related code can
 * be removed when kernel no longer support C3B(xuelang).
 */
#define	MB_CONFIG_START		0x908000
#define	MB_MCLK			(MB_CONFIG_START + 0x1)
#define	MB_EXTCLK		(MB_CONFIG_START + 0x11)

#ifndef __ASSEMBLY__
#include <asm/bootparam.h>
extern struct boot_params *sunway_boot_params;
extern u64 sunway_mclk_hz;
extern u64 sunway_extclk_hz;
#endif

#endif /* _ASM_SW64_SETUP_H */

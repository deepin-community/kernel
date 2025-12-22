/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LEGACY_BOOT_H_
#define __LEGACY_BOOT_H_
#include <linux/acpi.h>
#include <linux/of_address.h>
#include <linux/screen_info.h>

#define ADDRESS_TYPE_SYSRAM	1
#define ADDRESS_TYPE_RESERVED	2
#define ADDRESS_TYPE_ACPI	3
#define ADDRESS_TYPE_NVS	4
#define ADDRESS_TYPE_PMEM	5

#define LOONGSON3_BOOT_MEM_MAP_MAX	128
#define RT_MAP_START			100
#define FIX_MAP_ENTRY			32

/* mask of the flags in bootparamsinterface */
#define BPI_FLAGS_UEFI_SUPPORTED	BIT(0)
#define BPI_FLAGS_SOC_CPU		BIT(1)

#define LOONGSON_DMA_MASK_BIT			64
#define LOONGSON_MEM_SIGNATURE			"MEM"
#define LOONGSON_VBIOS_SIGNATURE		"VBIOS"
#define LOONGSON_EFIBOOT_SIGNATURE		"BPI"
#define LOONGSON_SCREENINFO_SIGNATURE		"SINFO"
#define LOONGSON_EFIBOOT_VERSION		1000

/* Values for Version firmware */

enum bpi_vers {
	BPI_VERSION_NONE = 0,
	BPI_VERSION_V1 = 1000,
	BPI_VERSION_V2 = 1001,
};

struct boot_params {
	u64	signature;	/* {"BPIXXXXX"} */
	void	*systemtable;
	struct  _extention_list_hdr *extlist;
	u64		flags;
} __packed;

struct _extention_list_hdr {
	u64	signature;
	u32	length;
	u8	revision;
	u8	checksum;
	struct  _extention_list_hdr *next;
} __packed;

struct loongsonlist_mem_map {
	struct	_extention_list_hdr header;	/*{"M", "E", "M"}*/
	u8	map_count;
	struct	_loongson_mem_map {
		u32 mem_type;
		u64 mem_start;
		u64 mem_size;
	} __packed map[LOONGSON3_BOOT_MEM_MAP_MAX];
} __packed;

struct loongsonlist_vbios {
	struct	_extention_list_hdr header;	/* {VBIOS} */
	u64	vbios_addr;
} __packed;

struct loongsonlist_screeninfo {
	struct  _extention_list_hdr header;
	struct  screen_info si;
};
unsigned long legacy_boot_init(unsigned long argc,
		unsigned long cmdptr, unsigned long bpi);
extern int bpi_version;
extern struct boot_params *efi_bp;
extern struct loongsonlist_mem_map *g_mmap;
extern int set_processor_mask(u32 id, u32 flags);
extern int __init setup_legacy_IRQ(void);
extern struct loongson_system_configuration loongson_sysconf;
extern unsigned long long smp_group[MAX_PACKAGES];
extern int legacy_madt_table_init(void);
extern struct pch_pic *pch_pic_priv[MAX_IO_PICS];
extern struct irq_domain *get_cpudomain(void);
extern int __init cpuintc_acpi_init(union acpi_subtable_headers *header,
				   const unsigned long end);
extern int __init
pch_pic_parse_madt(union acpi_subtable_headers *header,
		const unsigned long end);
extern int __init
pch_msi_parse_madt(union acpi_subtable_headers *header,
		const unsigned long end);
extern struct irq_domain *get_pchpic_irq_domain(void);
#endif

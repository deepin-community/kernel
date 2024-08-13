#include <linux/efi.h>
#include <linux/memblock.h>
#include <linux/acpi.h>
#include <linux/kmemleak.h>
#include <linux/kernel.h>

#include <asm/early_ioremap.h>
#include <asm/loongson.h>
#include <asm/irq.h>
#include <asm/numa.h>

#include "legacy_boot.h"

#define LOONGARCH_BOOT_MEM_MAP_MAX 128

enum bpi_vers {
	BPI_VERSION_NONE = 0,
	BPI_VERSION_V1 = 1000,
	BPI_VERSION_V2 = 1001,
};

struct loongarch_bpi_ext_hdr {
	u64 signature;
	u32 length;
	u8 revision;
	u8 checksum;
	u64 next;
} __packed;

struct loongarch_bpi_hdr {
	u64 signature;
	u64 systemtable;
	u64 extlist;
	u64 flags;
} __packed;

enum bpi_mem_type {
	ADDRESS_TYPE_SYSRAM	= 1,
	ADDRESS_TYPE_RESERVED	= 2,
	ADDRESS_TYPE_ACPI	= 3,
	ADDRESS_TYPE_NVS	= 4,
	ADDRESS_TYPE_PMEM	= 5,
	ADDRESS_TYPE_MAX,
};

#define MSI_MSG_ADDRESS		0x2FF00000
#define MSI_MSG_DEFAULT_COUNT	0xC0

/*
  ``This should be configured by firmware"
    -- Section 4, Page 25 of Loongson-7A1000 User Manual v2.0
  but unable to figure out the exact value.
  The manual gives a typical value in Table 3.2, Page 23.
*/
#define LS7A_CHIPCFG_REG_OFF	0x00010000

/* Section 4.1, Page 26 of Loongson-7A1000 User Manual v2.0 */
#define LS7A_DMA_CFG_OFF	0x041c
#define LS7A_DMA_NODE_ID_OFFSET_SHF	8
#define LS7A_DMA_NODE_ID_OFFSET_MASK	GENMASK(12, 8)

/* Section 14.4.1, Page 100 of Loongson-3A5000 User Manual v1.03 */
#define HT_CTRL_CFG_OFF		0xfdfb000000ull
/* Section 14.5.34, Page 141 of Loongson-3A5000 User Manual v1.03 */
#define HT_CTRL_HT_RX_INT_TRANS_LO_OFF	0x270
#define HT_CTRL_HT_RX_INT_TRANS_HI_OFF	0x274
#define HT_CTRL_HT_RX_INT_TRANS_INT_TRANS_ALLOW BIT(30)

/* Section 11.2.3, Page 61 of Loongson-3A5000 User Manual v1.03 */
#define EXT_IOI_SEND_OFF	0x1140

struct loongarch_bpi_mem_map {
	struct	loongarch_bpi_ext_hdr header;	/*{"M", "E", "M"}*/
	u8	map_count;
	struct	loongarch_bpi_mem_map_entry {
		u32 mem_type;
		u64 mem_start;
		u64 mem_size;
	} __packed map[];
} __packed;

struct loongarch_bpi_info __initdata loongarch_bpi_info = {
	.bpi = EFI_INVALID_TABLE_ADDR,
};

static enum bpi_vers bpi_version = BPI_VERSION_NONE;
static u64 __initdata bpi_flags = 0;

static int have_bpi = 0;

static __initdata struct {
	size_t map_count;
	struct loongarch_bpi_memmap{
		enum bpi_mem_type type;
		unsigned long mem_start;
		size_t mem_size;
	} map[LOONGARCH_BOOT_MEM_MAP_MAX];
} bpi_meminfo = {0};

static __initdata struct {
	unsigned long addr;
	size_t size;
} bpi_memmap = {
	.addr = EFI_INVALID_TABLE_ADDR,
	.size = 0,
};

static const __initconst struct bpi_extlist_desc {
	union {
		u64 signature;
		char chars[8];
	};
	unsigned long *ptr;
	size_t *length;
} bpi_extlist[] = {
	{ .chars = "MEM", &bpi_memmap.addr, &bpi_memmap.size },
	{ .signature = 0, NULL, NULL },
};


static void __init parse_bpi_ext_list(unsigned long exthdr)
{
	unsigned long cur_hdr = exthdr;
	while(cur_hdr){
		struct loongarch_bpi_ext_hdr *hdr = early_memremap_ro(cur_hdr, sizeof(*hdr));
		if(!hdr) {
			break;
		}
		u64 sig = hdr->signature;
		u32 len = hdr->length;
		u8 rev = hdr->revision;
		unsigned long next = hdr->next;
		early_memunmap(hdr, sizeof(*hdr));

		pr_info("BPI: ext hdr: %.8s, rev %u\n", (const char *)&sig, rev);

		for(const struct bpi_extlist_desc *desc = bpi_extlist; desc->signature; desc++) {
			if(sig == desc->signature) {
				*(desc->ptr) = cur_hdr;
				*(desc->length) = len;
			}
		}
		cur_hdr = next;
	}
}


static u8 __init ext_listhdr_checksum(void *buffer, size_t length)
{
	u8 sum = 0;
	u8 *end = buffer + length;
	u8 *buf = buffer;

	while (buf < end)
		sum = (u8)(sum + *(buf++));

	return sum;
}

static void __init parse_bpi_mem_map(void)
{
	if(bpi_memmap.addr == EFI_INVALID_TABLE_ADDR) {
		return;
	}
	if(bpi_memmap.size < sizeof(struct loongarch_bpi_mem_map)) {
		pr_err("BPI: invalid memmap size %ld\n", bpi_memmap.size);
		return;
	}
	struct loongarch_bpi_mem_map *memmap = early_memremap_ro(bpi_memmap.addr, bpi_memmap.size);
	if(!memmap) {
		return;
	}

	u8 checksum = ext_listhdr_checksum(memmap, bpi_memmap.size);
	if (checksum != 0) {
		pr_err("BPI: memmap checksum mismatch\n");
		goto err_out;
	}

	size_t map_count = memmap->map_count;
	if (map_count > LOONGARCH_BOOT_MEM_MAP_MAX) {
		pr_err("BPI: too many memmap entries\n");
		goto err_out;
	}
	if (map_count * sizeof(memmap->map[0]) + sizeof(*memmap) > bpi_memmap.size) {
		pr_err("BPI: invalid memmap size %ld, not enough to hold %ld entries\n", bpi_memmap.size, map_count);
		goto err_out;
	}
	for(int i = 0; i < map_count; i++) {
		bpi_meminfo.map[i].type = memmap->map[i].mem_type;
		bpi_meminfo.map[i].mem_start = memmap->map[i].mem_start;
		bpi_meminfo.map[i].mem_size = memmap->map[i].mem_size;

		static const char * const __initconst mem_type_str[] = {
			NULL,
			[ADDRESS_TYPE_SYSRAM] = "SYSRAM",
			[ADDRESS_TYPE_RESERVED] = "RESERVED",
			[ADDRESS_TYPE_ACPI] = "ACPI",
			[ADDRESS_TYPE_NVS] = "NVS",
			[ADDRESS_TYPE_PMEM] = "PMEM",
		};
		if(bpi_meminfo.map[i].type >= ADDRESS_TYPE_MAX || bpi_meminfo.map[i].type == 0) {
			pr_info("BPI: memmap type unknown(%d), start 0x%lx, size 0x%lx\n", bpi_meminfo.map[i].type, bpi_meminfo.map[i].mem_start, bpi_meminfo.map[i].mem_size);
		}else{
			pr_info("BPI: memmap type %s, start 0x%lx, size 0x%lx\n", mem_type_str[bpi_meminfo.map[i].type], bpi_meminfo.map[i].mem_start, bpi_meminfo.map[i].mem_size);
		}
	}
	bpi_meminfo.map_count = map_count;

err_out:
	early_memunmap(memmap, bpi_memmap.size);
}

static int __init bpi_parse_signature (u64 signature)
{
	union {
		u64 signature;
		char chars[9];
	} sig;
	sig.signature = signature;
	sig.chars[8] = '\0';

	if (!(sig.chars[0] == 'B' && sig.chars[1] == 'P' && sig.chars[2] == 'I')) {
		pr_err("BPI: invalid signature\n");
		return -EINVAL;
	}
	int version;
	int rc = kstrtoint(&sig.chars[3], 10, &version);
	if(rc != 0 || version == BPI_VERSION_NONE) {
		pr_err("BPI: invalid version\n");
		return -EINVAL;
	}
	bpi_version = version;
	pr_info("BPI: version %d\n", bpi_version);
	return 0;
}

static void (*p_init_acpi_arch_os_table_override)(struct acpi_table_header *, struct acpi_table_header **) = NULL;
static void init_acpi_arch_os_table_override (struct acpi_table_header *existing_table, struct acpi_table_header **new_table);

void __init bpi_init(void)
{
	if (loongarch_bpi_info.bpi == EFI_INVALID_TABLE_ADDR) {
		return;
	}
	struct loongarch_bpi_hdr *tbl;

	tbl = early_memremap_ro(loongarch_bpi_info.bpi, sizeof(*tbl));
	if (tbl) {
		int rc = bpi_parse_signature(tbl->signature);
		if (rc != 0) {
			goto err_out;
		}
		have_bpi = 1;
		if (bpi_version >= BPI_VERSION_V2) {
			bpi_flags = tbl->flags;
			pr_info("BPI: flags 0x%llx\n", bpi_flags);
		} else {
			p_init_acpi_arch_os_table_override = init_acpi_arch_os_table_override;
		}
		unsigned long bpi_extlist = tbl->extlist;
		parse_bpi_ext_list(bpi_extlist);
		parse_bpi_mem_map();
err_out:
		early_memunmap(tbl, sizeof(*tbl));
	}
}

void __init bpi_memblock_init(unsigned long *p_max_low_pfn)
{
	for(int i = 0; i < bpi_meminfo.map_count; i++) {
		unsigned long start = bpi_meminfo.map[i].mem_start;
		size_t size = bpi_meminfo.map[i].mem_size;
		unsigned long end = start + size;

		switch(bpi_meminfo.map[i].type) {
		case ADDRESS_TYPE_SYSRAM:
		// EFI_CONVENTIONAL_MEMORY
		case ADDRESS_TYPE_PMEM:
		// EFI_PERSISTENT_MEMORY
			memblock_add(start, size);
			if (*p_max_low_pfn < (end >> PAGE_SHIFT))
				*p_max_low_pfn = end >> PAGE_SHIFT;
			break;
		case ADDRESS_TYPE_ACPI:
		// EFI_ACPI_RECLAIM_MEMORY
			memblock_add(start, size);
			fallthrough;
		case ADDRESS_TYPE_RESERVED:
		// EFI_RUNTIME_SERVICES_DATA or
		// EFI_RUNTIME_SERVICES_CODE or
		// EFI_RESERVED_MEMORY_TYPE
			memblock_reserve(start, size);
			break;
		default:
			break;
		}
	}
}

void __init bpi_init_node_memblock(void (*p_add_numamem_region)(u64 start, u64 end, u32 type))
{
	for(int i = 0; i < bpi_meminfo.map_count; i++) {
		u64 mem_start = bpi_meminfo.map[i].mem_start;
		u64 mem_size = bpi_meminfo.map[i].mem_size;
		u64 mem_end = mem_start + mem_size;
		u32 mem_type;

		switch(bpi_meminfo.map[i].type) {
		case ADDRESS_TYPE_SYSRAM:
			mem_type = EFI_CONVENTIONAL_MEMORY;
			p_add_numamem_region(mem_start, mem_end, mem_type);
			break;
		case ADDRESS_TYPE_PMEM:
			mem_type = EFI_PERSISTENT_MEMORY;
			p_add_numamem_region(mem_start, mem_end, mem_type);
			break;
		case ADDRESS_TYPE_ACPI:
			mem_type = EFI_ACPI_RECLAIM_MEMORY;
			p_add_numamem_region(mem_start, mem_end, mem_type);
			fallthrough;
		case ADDRESS_TYPE_RESERVED:
			pr_info("Resvd: mem_type:BPI_%d, mem_start:0x%llx, mem_size:0x%llx Bytes\n",
					bpi_meminfo.map[i].type, mem_start, mem_size);
			break;
		default:
			break;
		}
	}
}

static u8 new_mcfg_buf [ 0
	+ sizeof(struct acpi_table_mcfg)
	+ MAX_IO_PICS * sizeof(struct acpi_mcfg_allocation)
];
static int __initdata new_mcfg_available = 0;

void __init acpi_arch_table_init_complete(void){
	if (!new_mcfg_available){
		return;
	}
	pr_info("BPI: installing new MCFG table\n");
	acpi_install_table((struct acpi_table_header *) new_mcfg_buf);
}

static void __init init_acpi_arch_os_table_override (struct acpi_table_header *existing_table, struct acpi_table_header **new_table){
	static int __initdata madt_table_installed = 0;
	static u8 new_madt_buf[ 0
		+ sizeof(struct acpi_table_madt)
		+ sizeof(struct acpi_madt_lio_pic)
		+ sizeof(struct acpi_madt_core_pic) * MAX_CORE_PIC
		+ MAX_IO_PICS * ( 0
			+ sizeof(struct acpi_madt_eio_pic)
			+ sizeof(struct acpi_madt_msi_pic)
			+ sizeof(struct acpi_madt_bio_pic)
		)
	];
	if (madt_table_installed){
		return;
	}
	if (bpi_version == BPI_VERSION_NONE) {
		return;
	}
	if (bpi_version > BPI_VERSION_V1) {
		return;
	}
	if (strncmp(existing_table->signature, ACPI_SIG_MADT, 4) != 0) {
		return;
	}
	pr_info("BPI: replacing MADT table\n");
	struct acpi_table_madt *madt = (struct acpi_table_madt *)existing_table;
	if (madt->header.length < sizeof(struct acpi_table_madt)) {
		pr_warn("BPI: MADT table length %u is too small\n", madt->header.length);
		return;
	}
	void *madt_end = (void *)madt + madt->header.length;
	struct acpi_subtable_header *entry = (struct acpi_subtable_header *)(madt + 1);
	int entry_count = 0;
	int local_apic_count = 0;
	int io_apic_count = 0;
	u64 node_map = 0;
	while((void *)entry < madt_end) {
		unsigned int ent_len = entry->length;
		if (ent_len < sizeof(struct acpi_subtable_header)) {
			pr_warn("BPI: MADT entry %d length %u is too small\n", entry_count, ent_len);
			return;
		}
		if((void *)entry + ent_len > madt_end) {
			pr_warn("BPI: MADT entry %d length overflow\n", entry_count);
			return;
		}
		switch(entry->type) {
		case ACPI_MADT_TYPE_LOCAL_APIC:
			local_apic_count++;
			struct acpi_madt_local_apic *lapic = (struct acpi_madt_local_apic *)entry;
			node_map |= 1 << (lapic->id / CORES_PER_EIO_NODE);
			break;
		case ACPI_MADT_TYPE_IO_APIC:
			io_apic_count++;
			break;
		}
		acpi_table_print_madt_entry(entry);

		entry = (struct acpi_subtable_header *)((void *)entry + ent_len);
	}

	if (local_apic_count == 0) {
		pr_warn("BPI: MADT has no local APIC entries\n");
		return;
	}
	if (local_apic_count > MAX_CORE_PIC) {
		pr_warn("BPI: MADT has too many local APIC entries\n");
		return;
	}
	if (io_apic_count == 0) {
		pr_warn("BPI: MADT has no IO APIC entries\n");
		return;
	}
	if (io_apic_count > MAX_IO_PICS) {
		pr_warn("BPI: MADT has too many IO APIC entries\n");
		return;
	}
	size_t new_madt_size = sizeof(struct acpi_table_madt);
	size_t new_mcfg_size = sizeof(struct acpi_table_mcfg);
	new_madt_size += sizeof(struct acpi_madt_lio_pic);
	new_madt_size += sizeof(struct acpi_madt_core_pic) * local_apic_count;
	if (cpu_has_extioi) {
		pr_info("BPI: Using EIOINTC interrupt mode\n");
		new_madt_size += io_apic_count * ( 0
			+ sizeof(struct acpi_madt_eio_pic)
			+ sizeof(struct acpi_madt_msi_pic)
			+ sizeof(struct acpi_madt_bio_pic)
		);
		new_madt_size += sizeof(struct acpi_madt_lpc_pic);
		new_mcfg_size += io_apic_count * sizeof(struct acpi_mcfg_allocation);
	} else {
		pr_info("BPI: Using HTVECINTC interrupt mode\n");
		new_madt_size += 0;
		new_mcfg_size += sizeof(struct acpi_mcfg_allocation);
	}

	if (new_madt_size > sizeof(new_madt_buf)) {
		pr_warn("BPI: new madt will be too large");
		return;
	}
	if (new_mcfg_size > sizeof(new_mcfg_buf)) {
		pr_warn("BPI: new mcfg will be too large");
		return;
	}
	madt_table_installed = 1;
	new_mcfg_available = 1;

	struct acpi_table_madt *new_madt = (struct acpi_table_madt *)new_madt_buf;

	new_madt->header = madt->header;
	new_madt->header.length = new_madt_size;
	new_madt->header.checksum = 0;
	new_madt->header.asl_compiler_id[0] = 'B';
	new_madt->header.asl_compiler_id[1] = 'P';
	new_madt->header.asl_compiler_id[2] = 'I';
	new_madt->address = madt->address;
	new_madt->flags = madt->flags;

	struct acpi_table_mcfg *new_mcfg = (struct acpi_table_mcfg *)new_mcfg_buf;

	memcpy(new_mcfg->header.signature, ACPI_SIG_MCFG, sizeof(new_mcfg->header.signature));
	new_mcfg->header.length = new_mcfg_size;
	new_mcfg->header.revision = 1;
	new_mcfg->header.checksum = 0;
	memcpy(new_mcfg->header.oem_id, madt->header.oem_id, sizeof(new_mcfg->header.oem_id));
	memcpy(new_mcfg->header.oem_table_id, madt->header.oem_table_id, sizeof(new_mcfg->header.oem_table_id));
	new_mcfg->header.oem_revision = madt->header.oem_revision;
	memcpy(new_mcfg->header.asl_compiler_id, "BPI", sizeof(new_mcfg->header.asl_compiler_id));
	new_mcfg->header.asl_compiler_revision = 1;
	memset(new_mcfg->reserved, 0, sizeof(new_mcfg->reserved));

	struct acpi_mcfg_allocation *mcfg_entry = (struct acpi_mcfg_allocation *)(new_mcfg + 1);

	static struct acpi_madt_core_pic __initdata core_pics[MAX_CORE_PIC];
	static struct acpi_madt_eio_pic __initdata eio_pics[MAX_IO_PICS];
	static struct acpi_madt_msi_pic __initdata msi_pics[MAX_IO_PICS];
	static struct acpi_madt_bio_pic __initdata bio_pics[MAX_IO_PICS];

	entry = (struct acpi_subtable_header *)(madt + 1);
	int core_idx = 0;
	int eio_idx = 0;
	while((void *)entry < madt_end) {
		unsigned int ent_len = entry->length;
		if(entry->type == ACPI_MADT_TYPE_LOCAL_APIC) {
			struct acpi_madt_local_apic *lapic = (struct acpi_madt_local_apic *)entry;

			if(core_idx >= local_apic_count){
				panic("BPI: MADT local APIC entries are more than expected\n");
			}

			core_pics[core_idx].header.type = ACPI_MADT_TYPE_CORE_PIC;
			core_pics[core_idx].header.length = sizeof(core_pics[0]);
			core_pics[core_idx].version = ACPI_MADT_CORE_PIC_VERSION_V1;
			core_pics[core_idx].processor_id = lapic->processor_id;
			core_pics[core_idx].core_id = lapic->id;
			core_pics[core_idx].flags = lapic->lapic_flags;

			core_idx++;
		}else if(entry->type == ACPI_MADT_TYPE_IO_APIC) {
			struct acpi_madt_io_apic *ioapic = (struct acpi_madt_io_apic *)entry;

			if(eio_idx >= io_apic_count){
				panic("BPI: MADT IO APIC entries are more than expected\n");
			}

			eio_pics[eio_idx].header.type = ACPI_MADT_TYPE_EIO_PIC;
			eio_pics[eio_idx].header.length = sizeof(eio_pics[0]);
			eio_pics[eio_idx].version = ACPI_MADT_EIO_PIC_VERSION_V1;
			eio_pics[eio_idx].cascade = 3 + eio_idx;
			eio_pics[eio_idx].node = ioapic->id;
			if(eio_idx == 0){
				eio_pics[eio_idx].node_map = node_map;
			}else{
				eio_pics[0].node_map = node_map & 0x0f0f0f0f0f0f0f0full;
				eio_pics[eio_idx].node_map = node_map & 0xf0f0f0f0f0f0f0f0ull;
			}

			unsigned long addr = ioapic->address;
			if(eio_idx > 0){
				addr |= nid_to_addrbase(ioapic->id) | HT1LO_OFFSET;
			}

			msi_pics[eio_idx].header.type = ACPI_MADT_TYPE_MSI_PIC;
			msi_pics[eio_idx].header.length = sizeof(msi_pics[0]);
			msi_pics[eio_idx].version = ACPI_MADT_MSI_PIC_VERSION_V1;
			msi_pics[eio_idx].msg_address = MSI_MSG_ADDRESS;
			pr_info("BPI: will read MSI start addr for node %d from 0x%lx\n", ioapic->id, addr);
			msi_pics[eio_idx].start = (((unsigned long)ls7a_readq(addr) >> 48) & 0xff) + 1;
			pr_info("BPI: done read MSI start addr for node %d from 0x%lx\n", ioapic->id, addr);
			msi_pics[eio_idx].count = MSI_MSG_DEFAULT_COUNT;

			bio_pics[eio_idx].header.type = ACPI_MADT_TYPE_BIO_PIC;
			bio_pics[eio_idx].header.length = sizeof(bio_pics[0]);
			bio_pics[eio_idx].version = ACPI_MADT_BIO_PIC_VERSION_V1;
			bio_pics[eio_idx].address = addr;
			bio_pics[eio_idx].size = 0x1000;
			bio_pics[eio_idx].id = ioapic->id;
			bio_pics[eio_idx].gsi_base = ioapic->global_irq_base;

			mcfg_entry->address = mcfg_addr_init(ioapic->id);
			mcfg_entry->pci_segment = eio_idx;
			mcfg_entry->start_bus_number = 0;
			mcfg_entry->end_bus_number = 0xFF; // Who knows?
			mcfg_entry->reserved = 0;
			mcfg_entry++;

			eio_idx++;
		}
		entry = (struct acpi_subtable_header *)((void *)entry + ent_len);
	}
	if(eio_idx != io_apic_count || core_idx != local_apic_count) {
		panic("BPI: MADT entries are less than expected\n");
	}

	// 1. Core APIC 0x11
	struct acpi_subtable_header *new_entry = (struct acpi_subtable_header *)(new_madt + 1);

	memcpy(new_entry, core_pics, local_apic_count * sizeof(core_pics[0]));
	new_entry = (struct acpi_subtable_header *)((void *)new_entry + local_apic_count * sizeof(core_pics[0]));

	// 2. LIO PIC 0x12
	{
		struct acpi_madt_lio_pic *lio_pic = (struct acpi_madt_lio_pic *)new_entry;
		lio_pic->header.type = ACPI_MADT_TYPE_LIO_PIC;
		lio_pic->header.length = sizeof(*lio_pic);
		lio_pic->version = ACPI_MADT_LIO_PIC_VERSION_V1;
		lio_pic->address = LOONGSON_REG_BASE + 0x1400;
		lio_pic->size = 256;
		lio_pic->cascade[0] = 2;
		lio_pic->cascade[1] = 3;
		lio_pic->cascade_map[0] = 0x00FFFFFF;
		lio_pic->cascade_map[1] = 0xFF000000;
		new_entry = (struct acpi_subtable_header *)((void *)new_entry + sizeof(*lio_pic));
	}
	// 3. HT PIC 0x13
	if (!cpu_has_extioi) {
		// FIX ME: Unsupported
	} else {
		// 4. EIO PIC 0x14
		memcpy(new_entry, eio_pics, io_apic_count * sizeof(eio_pics[0]));
		new_entry = (struct acpi_subtable_header *)((void *)new_entry + io_apic_count * sizeof(eio_pics[0]));
		// 5. MSI PIC 0x15
		memcpy(new_entry, msi_pics, io_apic_count * sizeof(msi_pics[0]));
		new_entry = (struct acpi_subtable_header *)((void *)new_entry + io_apic_count * sizeof(msi_pics[0]));
		// 6. BIO PIC 0x16
		memcpy(new_entry, bio_pics, io_apic_count * sizeof(bio_pics[0]));
		new_entry = (struct acpi_subtable_header *)((void *)new_entry + io_apic_count * sizeof(bio_pics[0]));
		// 7. LPC PIC 0x17
		{
			struct acpi_madt_lpc_pic *lpc_pic = (struct acpi_madt_lpc_pic *)new_entry;
			lpc_pic->header.type = ACPI_MADT_TYPE_LPC_PIC;
			lpc_pic->header.length = sizeof(*lpc_pic);
			lpc_pic->version = ACPI_MADT_LPC_PIC_VERSION_V1;
			lpc_pic->address = LS7A_LPC_REG_BASE;
			lpc_pic->size = SZ_4K;
			lpc_pic->cascade = 19;
			new_entry = (struct acpi_subtable_header *)((void *)new_entry + sizeof(*lpc_pic));
		}
	}
	if((void *)new_entry != (void *)new_madt + new_madt_size) {
		panic("BPI: missing bytes while constructing new MADT\n");
	}
	if((void *)mcfg_entry != (void *)new_mcfg + new_mcfg_size) {
		panic("BPI: missing bytes while constructing new MCFG\n");
	}
	new_madt->header.checksum = 0 - ext_listhdr_checksum((u8 *)new_madt, new_madt_size);
	new_mcfg->header.checksum = 0 - ext_listhdr_checksum((u8 *)new_mcfg, new_mcfg_size);
	*new_table = (struct acpi_table_header *)new_madt;

	// Override LS7A dma_node_id_offset
	for(int i = 0; i < io_apic_count; i++){
		u64 ls7a_base_addr = bio_pics[i].address;
		void __iomem *dma_node_id_addr = (void __iomem *) TO_UNCACHE(ls7a_base_addr + LS7A_CHIPCFG_REG_OFF + LS7A_DMA_CFG_OFF);
		u32 dma_cfg = readl(dma_node_id_addr);
		u32 dma_node_id_offset = (dma_cfg & LS7A_DMA_NODE_ID_OFFSET_MASK) >> LS7A_DMA_NODE_ID_OFFSET_SHF;
		if(dma_node_id_offset != 8){
			pr_info("BPI: LS7A %d DMA node id offset is %d, will set to 8\n", i, dma_node_id_offset);
			dma_cfg &= ~LS7A_DMA_NODE_ID_OFFSET_MASK;
			dma_cfg |= 8 << LS7A_DMA_NODE_ID_OFFSET_SHF;
			writel(dma_cfg, dma_node_id_addr);
		}
	}

	// Override HT_RX_INT_TRANS
	for(int i = 0; i < io_apic_count; i++){
		unsigned int node = eio_pics[i].node;
		void __iomem *ht_rx_int_trans_hi = (void __iomem *) TO_UNCACHE(nid_to_addrbase(node) + HT1LO_OFFSET + HT_CTRL_CFG_OFF + HT_CTRL_HT_RX_INT_TRANS_HI_OFF);
		void __iomem *ht_rx_int_trans_lo = (void __iomem *) TO_UNCACHE(nid_to_addrbase(node) + HT1LO_OFFSET + HT_CTRL_CFG_OFF + HT_CTRL_HT_RX_INT_TRANS_LO_OFF);
		u64 ext_ioi_addr = nid_to_addrbase(node) + LOONGSON_REG_BASE + EXT_IOI_SEND_OFF;
		u64 ht_rx_int_trans_cfg_orig = ((u64)readl(ht_rx_int_trans_hi) << 32) | readl(ht_rx_int_trans_lo);
		u32 ht_rx_int_trans_cfg_hi = HT_CTRL_HT_RX_INT_TRANS_INT_TRANS_ALLOW | (ext_ioi_addr >> 32);
		u32 ht_rx_int_trans_cfg_lo = ext_ioi_addr & GENMASK(31, 0);
		pr_info("BPI: HT controller on node %u RX_INT_TRANS is 0x%llx, will set to 0x%llx\n", node, ht_rx_int_trans_cfg_orig, ((u64)ht_rx_int_trans_cfg_hi << 32) | ht_rx_int_trans_cfg_lo);
		writel(ht_rx_int_trans_cfg_hi, ht_rx_int_trans_hi);
		writel(ht_rx_int_trans_cfg_lo, ht_rx_int_trans_lo);
	}
}

void acpi_arch_os_table_override (struct acpi_table_header *existing_table, struct acpi_table_header **new_table){
	if(p_init_acpi_arch_os_table_override && system_state == SYSTEM_BOOTING) {
		p_init_acpi_arch_os_table_override(existing_table, new_table);
	}
}

void acpi_arch_pci_probe_root_dev_filter(struct resource_entry *entry)
{
	if (bpi_version == BPI_VERSION_NONE) {
		return;
	}
	if (bpi_version > BPI_VERSION_V1) {
		return;
	}

	if (entry->res->flags & IORESOURCE_IO) {
		if (entry->offset == 0) {
			if (entry->res->start == ISA_IOSIZE) {
				entry->res->start = 0;
			}
			entry->offset = LOONGSON_LIO_BASE;
			entry->res->start = LOONGSON_LIO_BASE + PFN_ALIGN(entry->res->start);
			entry->res->end = LOONGSON_LIO_BASE + PFN_ALIGN(entry->res->end + 1) - 1;
		}
	}
}

static __initconst const struct {
	struct acpi_table_header header;
	unsigned char code [];
} __packed dsdt_add_aml_code = {
	.header = {
		.signature = ACPI_SIG_DSDT
	},
	.code = {
		                    0x14,0x21,0x5C,0x2F,  /* 00000020    "    .!\/" */
		0x05,0x5F,0x53,0x42,0x5F,0x50,0x43,0x49,  /* 00000028    "._SB_PCI" */
		0x30,0x4C,0x50,0x43,0x5F,0x45,0x43,0x5F,  /* 00000030    "0LPC_EC_" */
		0x5F,0x5F,0x44,0x45,0x50,0x08,0xA4,0x12,  /* 00000038    "__DEP..." */
		0x06,0x01,0x50,0x43,0x49,0x30             /* 00000040    "..PCI0"   */
	},
};

void __init acpi_arch_init (){
	if (bpi_version == BPI_VERSION_NONE) {
		return;
	}
	if (bpi_version > BPI_VERSION_V1) {
		return;
	}
	pr_info("BPI: Trying to patch DSDT\n");

	acpi_status status;
	acpi_handle handle;

	status = acpi_get_handle(NULL, "\\_SB.PCI0.LPC.EC", &handle);
	if (ACPI_FAILURE(status)) {
		if (status != AE_NOT_FOUND) {
			pr_info("BPI: Unable to find EC device: %s\n", acpi_format_exception(status));
		}
		return;
	}
	if (acpi_has_method(handle, "_DEP")) {
		return;
	}

	status = acpi_install_method((u8 *)&dsdt_add_aml_code);
	if (ACPI_FAILURE(status)) {
		pr_info("BPI: Unable to patch DSDT(0x%x)\n", status);
		return;
	}
	acpi_handle_info(handle, "BPI: Patched DSDT\n");
}

int loongarch_have_legacy_bpi (void){
	return have_bpi;
}

#include <linux/efi.h>
#include <linux/memblock.h>

#include <asm/early_ioremap.h>

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

static enum bpi_vers __initdata bpi_version = BPI_VERSION_NONE;
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

int loongarch_have_legacy_bpi (void){
	return have_bpi;
}

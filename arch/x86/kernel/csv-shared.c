// SPDX-License-Identifier: GPL-2.0-only
/*
 * Hygon CSV support
 *
 * This file is shared between decompression boot code and running
 * linux kernel.
 *
 * Copyright (C) Hygon Info Technologies Ltd.
 */

#include <asm/e820/types.h>

/*
 ****************************** CSV3 secure call *******************************
 *
 * CSV3 guest is based on hygon secure isolated virualization feature. An secure
 * processor which resides in hygon SOC manages guest's private memory. The
 * secure processor allocates or frees private memory for CSV3 guest and manages
 * CSV3 guest's nested page table.
 *
 * As the secure processor is considered as a PCI device in host, CSV3 guest can
 * not communicate with it directly. Howerver, CSV3 guest must request the secure
 * processor to change its physical memory between private memory and shared
 * memory. CSV3 secure call command is a method used to communicate with secure
 * processor that host cannot tamper with the data in CSV3 guest. Host can only
 * perform an external command to notify the secure processor to handle the
 * pending guest's command.
 *
 * CSV3 secure call pages:
 * Secure call pages are two dedicated pages that reserved by BIOS. We define
 * secure call pages as page A and page B. During guest launch stage, the secure
 * processor will parse the address of secure call pages. The secure processor
 * maps the two pages with same private memory page in NPT. The secure processor
 * always set one page as present and another page as non-present in NPT.

 * CSV3 secure call main work flow:
 * If we write the guest's commands in one page then read them from another page,
 * nested page fault happens and the guest exits to host. Then host will perform
 * an external command with the gpa(page A or page B) to the secure processor.
 * The secure processor checks that the gpa in NPF belongs to secure call pages,
 * read the guest's command to handle, then switch the present bit between the
 * two pages.
 *
 *			guest page A    guest page B
 *			      |              |
 *			  ____|______________|____
 *			  |                      |
 *			  |  nested page table   |
 *			  |______________________|
 *			      \              /
 *			       \            /
 *			        \          /
 *			         \        /
 *			          \      /
 *			       secure memory page
 *
 * CSV3_SECURE_CMD_ENC:
 *	CSV3 guest declares a specifid memory range as secure. By default, all of
 *	CSV3 guest's memory mapped as secure.
 *	The secure processor allocate a block of secure memory and map the memory
 *	in CSV3 guest's NPT with the specified guest physical memory range in CSV3
 *	secure call.
 *
 * CSV3_SECURE_CMD_DEC:
 *	CSV3 guest declares a specified memory range as shared.
 *	The secure processor save the guest physical memory range in its own ram
 *	and free the range in CSV3 guest's NPT. When CSV3 guest access the memory,
 *	a new nested page fault happens.
 *
 * CSV3_SECURE_CMD_RESET:
 *	CSV3 guest switches all of the shared memory to secure.
 *	The secure processor resets all the shared memory in CSV3 guest's NPT and
 *	clears the saved shared memory range. Then the secure process allocates
 *	secure memory to map in CSV3 guest's NPT.
 *
 * CSV3_SECURE_CMD_UPDATE_SECURE_CALL_TABLE:
 *	CSV3 guest wants to change the secure call pages.
 *	The secure processor re-init the secure call context.
 */
enum csv3_secure_command_type {
	CSV3_SECURE_CMD_ENC	= 1,
	CSV3_SECURE_CMD_DEC,
	CSV3_SECURE_CMD_RESET,
	CSV3_SECURE_CMD_UPDATE_SECURE_CALL_TABLE,
};

/*
 * Secure call page fields.
 * Secure call page size is 4KB always. We define CSV3 secure call page structure
 * as below.
 * guid:	Must be in the first 128 bytes of the page. Its value should be
 *		(0xceba2fa59a5d926ful, 0xa556555d276b21abul) always.
 * cmd_type:	Command to be issued to the secure processor.
 * nums:	number of entries in the command.
 * base_address:Start address of the memory range.
 * size:	Size of the memory range.
 */
#define SECURE_CALL_ENTRY_MAX	(254)

/* size of secure call cmd is 4KB. */
struct csv3_secure_call_cmd {
	union {
		u8	guid[16];
		u64	guid_64[2];
	};
	u32	cmd_type;
	u32	nums;
	u64	unused;
	struct {
		u64	base_address;
		u64	size;
	} entry[SECURE_CALL_ENTRY_MAX];
};

/* csv3 secure call guid, do not change the value. */
#define CSV3_SECURE_CALL_GUID_LOW	0xceba2fa59a5d926ful
#define CSV3_SECURE_CALL_GUID_HIGH	0xa556555d276b21abul

static u64 csv3_boot_sc_page_a __initdata = -1ul;
static u64 csv3_boot_sc_page_b __initdata = -1ul;
static u32 early_page_idx __initdata;

/**
 * csv3_scan_secure_call_pages - try to find the secure call pages.
 * @boot_params:	boot parameters where e820_table resides.
 *
 * The secure call pages are reserved by BIOS. We scan all the reserved pages
 * to check the CSV3 secure call guid bytes.
 */
void __init csv3_scan_secure_call_pages(struct boot_params *boot_params)
{
	struct boot_e820_entry *entry;
	struct csv3_secure_call_cmd *sc_page;
	u64 offset;
	u64 addr;
	u8 i;
	u8 table_num;
	int count = 0;

	if (!boot_params)
		return;

	if (csv3_boot_sc_page_a != -1ul && csv3_boot_sc_page_b != -1ul)
		return;

	table_num = min_t(u8, boot_params->e820_entries,
			  E820_MAX_ENTRIES_ZEROPAGE);
	entry = &boot_params->e820_table[0];
	for (i = 0; i < table_num; i++) {
		if (entry[i].type != E820_TYPE_RESERVED)
			continue;

		addr = entry[i].addr & PAGE_MASK;
		for (offset = 0; offset < entry[i].size; offset += PAGE_SIZE) {
			sc_page = (void *)(addr + offset);
			if (sc_page->guid_64[0] == CSV3_SECURE_CALL_GUID_LOW &&
			    sc_page->guid_64[1] == CSV3_SECURE_CALL_GUID_HIGH) {
				if (count == 0)
					csv3_boot_sc_page_a = addr + offset;
				else if (count == 1)
					csv3_boot_sc_page_b = addr + offset;
				count++;
			}
			if (count >= 2)
				return;
		}
	}
}

/**
 * csv3_early_secure_call_ident_map - issue early secure call command at the
 *			stage where identity page table is created.
 * @base_address:	Start address of the specified memory range.
 * @num_pages:		number of the specific pages.
 * @cmd_type:		Secure call cmd type.
 */
void __init csv3_early_secure_call_ident_map(u64 base_address, u64 num_pages,
					     enum csv3_secure_command_type cmd_type)
{
	struct csv3_secure_call_cmd *page_rd;
	struct csv3_secure_call_cmd *page_wr;
	u32 cmd_ack;

	if (csv3_boot_sc_page_a == -1ul || csv3_boot_sc_page_b == -1ul)
		return;

	/* identity mapping at the stage. */
	page_rd = (void *)(early_page_idx ? csv3_boot_sc_page_a : csv3_boot_sc_page_b);
	page_wr = (void *)(early_page_idx ? csv3_boot_sc_page_b : csv3_boot_sc_page_a);

	while (1) {
		page_wr->cmd_type = (u32)cmd_type;
		page_wr->nums = 1;
		page_wr->entry[0].base_address = base_address;
		page_wr->entry[0].size = num_pages << PAGE_SHIFT;

		/*
		 * Write command in page_wr must be done before retrieve cmd
		 * ack from page_rd, and it is ensured by the mb below.
		 */
		mb();

		cmd_ack = page_rd->cmd_type;
		if (cmd_ack != cmd_type)
			break;
	}
	early_page_idx ^= 1;
}

/* SPDX-License-Identifier: GPL-2.0 */

#define RUNTIME_DISCARD_EXIT
#define EMITS_PT_NOTE
#define RO_EXCEPTION_TABLE_ALIGN	16

#include <asm-generic/vmlinux.lds.h>
#include <asm/thread_info.h>
#include <asm/cache.h>
#include <asm/page.h>
#include <asm/setup.h>

OUTPUT_FORMAT("elf64-sw_64")
OUTPUT_ARCH(sw_64)
ENTRY(__start)
PHDRS { text PT_LOAD; note PT_NOTE; }
jiffies = jiffies_64;
SECTIONS
{
	. = _TEXT_START;

	__start = .;
	_text = .;	/* Text and read-only data */
	_stext = .;
	.text : {
		HEAD_TEXT
		TEXT_TEXT
		SCHED_TEXT
		LOCK_TEXT
		IRQENTRY_TEXT
		SOFTIRQENTRY_TEXT
		KPROBES_TEXT
		*(.fixup)
		*(.gnu.warning)
	} :text
	_etext = .;	/* End of text section */

	RO_DATA(PAGE_SIZE)

	/* Will be freed after init */
	__init_begin = ALIGN(PAGE_SIZE);
	INIT_TEXT_SECTION(PAGE_SIZE)
	INIT_DATA_SECTION(16)
	/* we have to discard exit text and such at runtime, not link time */
	.exit.text :
	{
		EXIT_TEXT
	}
	.exit.data :
	{
		EXIT_DATA
	}
	PERCPU_SECTION(L1_CACHE_BYTES)

	/*
	 * Align to THREAD_SIZE rather than PAGE_SIZE here so any padding page
	 * needed for the THREAD_SIZE aligned init_task gets freed after init
	 */
	. = ALIGN(THREAD_SIZE);
	__init_end = .;
	/* Freed after init ends here */

	_sdata = .;	/* Start of rw data section */
	_data = .;
	RW_DATA(L1_CACHE_BYTES, PAGE_SIZE, THREAD_SIZE)
	.got : {
#ifdef CONFIG_RELOCATABLE
	_got_start = .;
#endif
		*(.got)
#ifdef CONFIG_RELOCATABLE
	_got_end = .;
#endif
	}
	.sdata : {
		*(.sdata)
	}
	_edata = .;	/* End of data section */

#ifdef CONFIG_RELOCATABLE
	_. = ALIGN(4);
	.data.reloc : {
		_relocation_start = .;
		/*
		 * Space for relocation table
		 * This needs to be filled so that the
		 * relocs tool can overwrite the content.
		 * An invalid value is left at the start of the
		 * section to abort relocation if the table
		 * has not been filled in.
		 */
		LONG(0xFFFFFFFF);
		FILL(0);
		. += CONFIG_RELOCATION_TABLE_SIZE - 4;
		_relocation_end = .;
	}
#endif
	BSS_SECTION(0, 0, 0)
	_end = .;

	.mdebug 0 : {
		*(.mdebug)
	}
	.note 0 : {
		*(.note)
	}

	STABS_DEBUG
	DWARF_DEBUG
	ELF_DETAILS

	DISCARDS
}

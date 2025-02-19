// SPDX-License-Identifier: GPL-2.0-or-later
#include <objtool/special.h>
#include <objtool/warn.h>

bool arch_support_alt_relocation(struct special_alt *special_alt,
				 struct instruction *insn,
				 struct reloc *reloc)
{
	return false;
}

struct table_info {
	struct list_head jump_info;
	unsigned long insn_offset;
	unsigned long rodata_offset;
};

static void get_rodata_table_size_by_table_annotate(struct objtool_file *file,
						    struct instruction *insn)
{
	struct section *rsec;
	struct reloc *reloc;
	struct list_head table_list;
	struct table_info *orig_table;
	struct table_info *next_table;
	unsigned long tmp_insn_offset;
	unsigned long tmp_rodata_offset;

	rsec = find_section_by_name(file->elf, ".rela.discard.tablejump_annotate");
	if (!rsec)
		return;

	INIT_LIST_HEAD(&table_list);

	for_each_reloc(rsec, reloc) {
		if (reloc->sym->type != STT_SECTION)
			return;

		orig_table = malloc(sizeof(struct table_info));
		if (!orig_table) {
			WARN("malloc failed");
			return;
		}

		orig_table->insn_offset = reloc_addend(reloc);
		reloc++;
		orig_table->rodata_offset = reloc_addend(reloc);

		list_add_tail(&orig_table->jump_info, &table_list);

		if (reloc_idx(reloc) + 1 == sec_num_entries(rsec))
			break;
	}

	list_for_each_entry(orig_table, &table_list, jump_info) {
		next_table = list_next_entry(orig_table, jump_info);
		list_for_each_entry_from(next_table, &table_list, jump_info) {
			if (next_table->rodata_offset < orig_table->rodata_offset) {
				tmp_insn_offset = next_table->insn_offset;
				tmp_rodata_offset = next_table->rodata_offset;
				next_table->insn_offset = orig_table->insn_offset;
				next_table->rodata_offset = orig_table->rodata_offset;
				orig_table->insn_offset = tmp_insn_offset;
				orig_table->rodata_offset = tmp_rodata_offset;
			}
		}
	}

	list_for_each_entry(orig_table, &table_list, jump_info) {
		if (insn->offset == orig_table->insn_offset) {
			next_table = list_next_entry(orig_table, jump_info);
			if (&next_table->jump_info == &table_list)
				break;

			while (next_table->rodata_offset == orig_table->rodata_offset)
				next_table = list_next_entry(next_table, jump_info);

			insn->table_size = next_table->rodata_offset - orig_table->rodata_offset;
		}
	}
}

struct reloc *arch_find_switch_table(struct objtool_file *file,
				     struct instruction *insn)
{
	return NULL;
}

// SPDX-License-Identifier: GPL-2.0-or-later
#include <string.h>
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

static struct reloc *find_reloc_by_table_annotate(struct objtool_file *file,
						  struct instruction *insn)
{
	struct section *rsec;
	struct reloc *reloc;
	unsigned long offset;

	rsec = find_section_by_name(file->elf, ".rela.discard.tablejump_annotate");
	if (!rsec)
		return NULL;

	for_each_reloc(rsec, reloc) {
		if (reloc->sym->sec->rodata)
			continue;

		if (strcmp(insn->sec->name, reloc->sym->sec->name))
			continue;

		if (reloc->sym->type == STT_SECTION)
			offset = reloc_addend(reloc);
		else
			offset = reloc->sym->offset;

		if (insn->offset == offset) {
			get_rodata_table_size_by_table_annotate(file, insn);
			reloc++;
			return reloc;
		}
	}

	return NULL;
}

static struct reloc *find_reloc_of_rodata_c_jump_table(struct section *sec,
						       unsigned long offset)
{
	struct section *rsec;
	struct reloc *reloc;

	rsec = sec->rsec;
	if (!rsec)
		return NULL;

	for_each_reloc(rsec, reloc) {
		if (reloc_offset(reloc) > offset)
			break;

		if (!strncmp(reloc->sym->sec->name, ".rodata..c_jump_table", 21))
			return reloc;
	}

	return NULL;
}

struct reloc *arch_find_switch_table(struct objtool_file *file,
				     struct instruction *insn)
{
	struct reloc *annotate_reloc;
	struct reloc *rodata_reloc;
	struct section *table_sec;
	unsigned long table_offset;

	annotate_reloc = find_reloc_by_table_annotate(file, insn);
	if (!annotate_reloc) {
		annotate_reloc = find_reloc_of_rodata_c_jump_table(insn->sec, insn->offset);
		if (!annotate_reloc)
			return NULL;
	}

	table_sec = annotate_reloc->sym->sec;
	if (annotate_reloc->sym->type == STT_SECTION)
		table_offset = reloc_addend(annotate_reloc);
	else
		table_offset = annotate_reloc->sym->offset;

	/*
	 * Each table entry has a rela associated with it.  The rela
	 * should reference text in the same function as the original
	 * instruction.
	 */
	rodata_reloc = find_reloc_by_dest(file->elf, table_sec, table_offset);
	if (!rodata_reloc)
		return NULL;

	return rodata_reloc;
}

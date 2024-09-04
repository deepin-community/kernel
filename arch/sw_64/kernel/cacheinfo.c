// SPDX-License-Identifier: GPL-2.0
/*
 * SW64 cacheinfo support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/cacheinfo.h>
#include <linux/acpi.h>

#include <asm/cpu.h>
#include <asm/cache.h>

#define get_cache_info(type)     cpuid(GET_CACHE_INFO, (type))
#define get_cache_size(info)     ((info) & 0xffffffffUL)
#define get_cacheline_size(info) (1 << (((info) >> 32) & 0xfUL))
#define get_cache_sets(info)     (1 << (((info) >> 36) & 0x3fUL))
#define get_cache_ways(info)     (get_cache_size(info) / get_cache_sets(info) / get_cacheline_size(info))
#define cache_size(type)         get_cache_size(get_cache_info((type)))
#define cache_level(type)        ((type) < L2_CACHE ? 1 : (type))

/* Populates leaf and increments to next leaf */
#define populate_cache(cache_info, leaf, c_level, c_type, c_id)		\
do {									\
	leaf->id = c_id;						\
	leaf->attributes = CACHE_ID;					\
	leaf->type = c_type;						\
	leaf->level = c_level;						\
	leaf->coherency_line_size = get_cacheline_size(cache_info);	\
	leaf->number_of_sets = get_cache_sets(cache_info);		\
	leaf->ways_of_associativity = get_cache_ways(cache_info);	\
	leaf->size = get_cache_size(cache_info);			\
} while (0)

static struct cacheinfo *get_cacheinfo(int cpu, int level, enum cache_type type)
{
	struct cpu_cacheinfo *this_cpu_ci = get_cpu_cacheinfo(cpu);
	struct cacheinfo *leaf;
	int index;

	for (index = 0; index < this_cpu_ci->num_leaves; index++) {
		leaf = this_cpu_ci->info_list + index;
		if ((leaf->level == level) && (leaf->type == type))
			return leaf;
	}

	return NULL;
}

unsigned int get_cpu_cache_size(int cpu, int level, enum cache_type type)
{
	struct cacheinfo *leaf = get_cacheinfo(cpu, level, type);

	return leaf ? leaf->size : 0;
}

unsigned int get_cpu_cacheline_size(int cpu, int level, enum cache_type type)
{
	struct cacheinfo *leaf = get_cacheinfo(cpu, level, type);

	return leaf ? leaf->coherency_line_size : 0;
}


static inline enum cache_type kernel_cache_type(enum sunway_cache_type type)
{
	if ((type > L1_DCACHE) || !cache_size(L1_ICACHE))
		return CACHE_TYPE_UNIFIED;

	return (type == L1_DCACHE) ? CACHE_TYPE_DATA : CACHE_TYPE_INST;
}

int init_cache_level(unsigned int cpu)
{
	struct cpu_cacheinfo *this_cpu_ci = get_cpu_cacheinfo(cpu);
	int levels = 0, leaves = 0;

	/*
	 * If Dcache is not set, we assume the cache structures
	 * are not properly initialized.
	 */
	if (cache_size(L1_DCACHE))
		levels += 1;
	else
		return -ENOENT;

	leaves += cache_size(L1_ICACHE) ? 2 : 1;

	if (cache_size(L2_CACHE)) {
		levels++;
		leaves++;
	}

	if (cache_size(L3_CACHE)) {
		levels++;
		leaves++;
	}

	this_cpu_ci->num_levels = levels;
	this_cpu_ci->num_leaves = leaves;

	return 0;
}

static void setup_shared_cpu_map(unsigned int cpu)
{
	unsigned int index;
	unsigned int rcid = cpu_to_rcid(cpu);
	struct cacheinfo *this_leaf;
	struct cpu_cacheinfo *this_cpu_ci = get_cpu_cacheinfo(cpu);

	for (index = 0; index < this_cpu_ci->num_leaves; index++) {
		unsigned int i;

		this_leaf = this_cpu_ci->info_list + index;

		cpumask_set_cpu(cpu, &this_leaf->shared_cpu_map);

		for_each_possible_cpu(i) {
			unsigned int sib_rcid = cpu_to_rcid(i);

			if ((rcid_to_domain_id(sib_rcid) != rcid_to_domain_id(rcid)) ||
					(i == cpu))
				continue;

			if ((rcid_to_core_id(rcid) == rcid_to_core_id(sib_rcid)) ||
					(this_leaf->level == 3))
				cpumask_set_cpu(i, &this_leaf->shared_cpu_map);
		}
	}
}

static bool is_pptt_cache_info_valid(void)
{
	struct acpi_table_header *table;
	acpi_status status;

	if (is_guest_or_emul() || acpi_disabled)
		return false;

	status = acpi_get_table(ACPI_SIG_PPTT, 0, &table);
	if (ACPI_FAILURE(status))
		return false;

	acpi_put_table(table);

	return true;
}

int populate_cache_leaves(unsigned int cpu)
{
	enum sunway_cache_type type;
	unsigned int cache_id;
	struct cpu_cacheinfo *this_cpu_ci = get_cpu_cacheinfo(cpu);
	struct cacheinfo *this_leaf = this_cpu_ci->info_list;
	bool pptt_valid = is_pptt_cache_info_valid();

	for (type = L1_ICACHE; type <= L3_CACHE; type++, this_leaf++) {
		if (!cache_size(type))
			continue;

		/* L3 Cache is shared */
		cache_id = (type == L3_CACHE) ? rcid_to_domain_id(cpu_to_rcid(cpu)) :
			rcid_to_core_id(cpu_to_rcid(cpu));

		populate_cache(get_cache_info(type), this_leaf, cache_level(type),
				kernel_cache_type(type), cache_id);

		if (pptt_valid)
			this_leaf->attributes &= ~CACHE_ID;

	}

	if (!pptt_valid) {
		setup_shared_cpu_map(cpu);
		this_cpu_ci->cpu_map_populated = true;
	}

	return 0;
}

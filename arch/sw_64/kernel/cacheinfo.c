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

#include <asm/cpu.h>
#include <asm/topology.h>
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
	leaf++;								\
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

int populate_cache_leaves(unsigned int cpu)
{
	enum sunway_cache_type type;
	struct cpu_cacheinfo *this_cpu_ci = get_cpu_cacheinfo(cpu);
	struct cacheinfo *this_leaf = this_cpu_ci->info_list;
	struct cpu_topology *topo = &cpu_topology[cpu];

	for (type = L1_ICACHE; type <= L3_CACHE; type++) {
		if (!cache_size(type))
			continue;

		/* L3 Cache is shared */
		if (type == L3_CACHE) {
			cpumask_copy(&this_leaf->shared_cpu_map,
					topology_llc_cpumask(cpu));
			populate_cache(get_cache_info(type), this_leaf, cache_level(type),
					kernel_cache_type(type), topo->package_id);
		} else {
			cpumask_set_cpu(cpu, &this_leaf->shared_cpu_map);
			populate_cache(get_cache_info(type), this_leaf, cache_level(type),
					kernel_cache_type(type), cpu);
		}
	}

	this_cpu_ci->cpu_map_populated = true;

	return 0;
}

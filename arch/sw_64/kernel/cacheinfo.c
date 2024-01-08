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

#include <asm/topology.h>

/* Populates leaf and increments to next leaf */
#define populate_cache(cache, leaf, c_level, c_type, c_id)	\
do {								\
	leaf->id = c_id;					\
	leaf->attributes = CACHE_ID;				\
	leaf->type = c_type;					\
	leaf->level = c_level;					\
	leaf->coherency_line_size = c->cache.linesz;		\
	leaf->number_of_sets = c->cache.sets;			\
	leaf->ways_of_associativity = c->cache.ways;		\
	leaf->size = c->cache.size;				\
	leaf++;							\
} while (0)

int init_cache_level(unsigned int cpu)
{
	struct cpuinfo_sw64 *c = &cpu_data[cpu];
	struct cpu_cacheinfo *this_cpu_ci = get_cpu_cacheinfo(cpu);
	int levels = 0, leaves = 0;

	/*
	 * If Dcache is not set, we assume the cache structures
	 * are not properly initialized.
	 */
	if (c->dcache.size)
		levels += 1;
	else
		return -ENOENT;


	leaves += (c->icache.size) ? 2 : 1;

	if (c->scache.size) {
		levels++;
		leaves++;
	}

	if (c->tcache.size) {
		levels++;
		leaves++;
	}

	this_cpu_ci->num_levels = levels;
	this_cpu_ci->num_leaves = leaves;
	return 0;
}

int populate_cache_leaves(unsigned int cpu)
{
	struct cpuinfo_sw64 *c = &cpu_data[cpu];
	struct cpu_cacheinfo *this_cpu_ci = get_cpu_cacheinfo(cpu);
	struct cacheinfo *this_leaf = this_cpu_ci->info_list;
	struct cpu_topology *topo = &cpu_topology[cpu];

	if (c->icache.size) {
		cpumask_set_cpu(cpu, &this_leaf->shared_cpu_map);
		populate_cache(dcache, this_leaf, 1, CACHE_TYPE_DATA, cpu);
		cpumask_set_cpu(cpu, &this_leaf->shared_cpu_map);
		populate_cache(icache, this_leaf, 1, CACHE_TYPE_INST, cpu);

	} else {
		cpumask_set_cpu(cpu, &this_leaf->shared_cpu_map);
		populate_cache(dcache, this_leaf, 1, CACHE_TYPE_UNIFIED, cpu);
	}

	if (c->scache.size) {
		cpumask_set_cpu(cpu, &this_leaf->shared_cpu_map);
		populate_cache(scache, this_leaf, 2, CACHE_TYPE_UNIFIED, cpu);
	}

	if (c->tcache.size) {
		cpumask_copy(&this_leaf->shared_cpu_map, topology_llc_cpumask(cpu));
		populate_cache(tcache, this_leaf, 3, CACHE_TYPE_UNIFIED, topo->package_id);
	}

	this_cpu_ci->cpu_map_populated = true;

	return 0;
}

// SPDX-License-Identifier: GPL-2.0
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/elf.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/binfmts.h>
#include <linux/time_namespace.h>
#include <asm/page.h>
#include <asm/vdso.h>

enum vvar_pages {
	VVAR_DATA_PAGE_OFFSET,
	VVAR_TIMENS_PAGE_OFFSET,
	VVAR_NR_PAGES,
};

enum sw64_vdso_map {
	SW64_VDSO_MAP_VVAR,
	SW64_VDSO_MAP_VDSO,
};

#define VVAR_SIZE  (VVAR_NR_PAGES << PAGE_SHIFT)

/*
 * The vDSO data page.
 */
static union {
	struct vdso_data	data;
	u8			page[PAGE_SIZE];
} vdso_data_store __page_aligned_data;
struct vdso_data *vdso_data = &vdso_data_store.data;

struct __vdso_info {
	const char *name;
	const char *vdso_code_start;
	const char *vdso_code_end;
	unsigned long vdso_pages;
	/* Data Mapping */
	struct vm_special_mapping *dm;
	/* Code Mapping */
	struct vm_special_mapping *cm;
};

static struct __vdso_info vdso_info;

#ifdef CONFIG_SUBARCH_C3B
#define V_NODE_SHIFT	6
static void init_cpu_map(void)
{

	int i, whami, domain;
	unsigned int shift, mask;

	if (is_in_host())
		shift = DOMAIN_ID_SHIFT;
	else
		shift = V_NODE_SHIFT;
	mask = (1 << shift) - 1;

	vdso_data_write_begin(vdso_data);
	for (i = 0; i < num_possible_cpus(); i++) {
		domain = cpu_to_rcid(i) >> shift;
		whami = (domain << DOMAIN_ID_SHIFT) | (cpu_to_rcid(i) & mask);
		vdso_data->vdso_whami_to_cpu[whami] = i;
		vdso_data->vdso_whami_to_node[whami] = domain;
	}
	vdso_data_write_end(vdso_data);
}
#else
static void init_cpu_map(void)
{
	int i;

	vdso_data_write_begin(vdso_data);
	for (i = 0; i < num_possible_cpus(); i++)
		vdso_data->vdso_cpu_to_node[i] = (cpu_to_rcid(i) & DOMAIN_ID_MASK) >> DOMAIN_ID_SHIFT;
	vdso_data_write_end(vdso_data);
}
#endif

static int vdso_mremap(const struct vm_special_mapping *sm,
		struct vm_area_struct *new_vma)
{
	unsigned long new_size = new_vma->vm_end - new_vma->vm_start;
	unsigned long vdso_size = vdso_end - vdso_start;

	if (vdso_size != new_size)
		return -EINVAL;

	current->mm->context.vdso = (void *)new_vma->vm_start;
	return 0;
}

static void __init __vdso_init(struct __vdso_info *vdso_info)
{
	unsigned int i;
	struct page **vdso_pagelist;
	unsigned long pfn;

	if (memcmp(vdso_info->vdso_code_start, "\177ELF", 4))
		panic("vDSO is not a valid ELF object!\n");

	vdso_info->vdso_pages = (
		vdso_info->vdso_code_end -
		vdso_info->vdso_code_start) >>
		PAGE_SHIFT;

	vdso_pagelist = kcalloc(vdso_info->vdso_pages,
				sizeof(struct page *),
				GFP_KERNEL);
	if (vdso_pagelist == NULL)
		panic("vDSO kcalloc failed!\n");

	/* Grab the vDSO code pages. */
	pfn = sym_to_pfn(vdso_info->vdso_code_start);

	for (i = 0; i < vdso_info->vdso_pages; i++)
		vdso_pagelist[i] = pfn_to_page(pfn + i);

	vdso_info->cm->pages = vdso_pagelist;

	init_cpu_map();
}

#ifdef CONFIG_TIME_NS
struct vdso_data *arch_get_vdso_data(void *vvar_page)
{
	return (struct vdso_data *)(vvar_page);
}

/*
 * The vvar mapping contains data for a specific time namespace, so when a task
 * changes namespace we must unmap its vvar data for the old namespace.
 * Subsequent faults will map in data for the new namespace.
 *
 * For more details see timens_setup_vdso_data().
 */
int vdso_join_timens(struct task_struct *task, struct time_namespace *ns)
{
	struct mm_struct *mm = task->mm;
	struct vm_area_struct *vma;
	VMA_ITERATOR(vmi, mm, 0);

	mmap_read_lock(mm);

	for_each_vma(vmi, vma) {
		if (vma_is_special_mapping(vma, vdso_info.dm))
			zap_vma_pages(vma);
	}

	mmap_read_unlock(mm);
	return 0;
}
#endif

static vm_fault_t vvar_fault(const struct vm_special_mapping *sm,
			     struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct page *timens_page = find_timens_vvar_page(vma);
	unsigned long pfn;

	switch (vmf->pgoff) {
	case VVAR_DATA_PAGE_OFFSET:
		if (timens_page)
			pfn = page_to_pfn(timens_page);
		else
			pfn = sym_to_pfn(vdso_data);
		break;
#ifdef CONFIG_TIME_NS
	case VVAR_TIMENS_PAGE_OFFSET:
		/*
		 * If a task belongs to a time namespace then a namespace
		 * specific VVAR is mapped with the VVAR_DATA_PAGE_OFFSET and
		 * the real VVAR page is mapped with the VVAR_TIMENS_PAGE_OFFSET
		 * offset.
		 * See also the comment near timens_setup_vdso_data().
		 */
		if (!timens_page)
			return VM_FAULT_SIGBUS;
		pfn = sym_to_pfn(vdso_data);
		break;
#endif /* CONFIG_TIME_NS */
	default:
		return VM_FAULT_SIGBUS;
	}

	return vmf_insert_pfn(vma, vmf->address, pfn);
}

static struct vm_special_mapping sw64_vdso_maps[] __ro_after_init = {
	[SW64_VDSO_MAP_VVAR] = {
		.name   = "[vvar]",
		.fault = vvar_fault,
	},
	[SW64_VDSO_MAP_VDSO] = {
		.name   = "[vdso]",
		.mremap = vdso_mremap,
	},
};

static struct __vdso_info vdso_info __ro_after_init = {
	.name = "vdso",
	.vdso_code_start = vdso_start,
	.vdso_code_end = vdso_end,
	.dm = &sw64_vdso_maps[SW64_VDSO_MAP_VVAR],
	.cm = &sw64_vdso_maps[SW64_VDSO_MAP_VDSO],
};

static int __init vdso_init(void)
{
	__vdso_init(&vdso_info);

	return 0;
}
arch_initcall(vdso_init);

static int __setup_additional_pages(struct mm_struct *mm,
				    struct linux_binprm *bprm,
				    int uses_interp,
				    struct __vdso_info *vdso_info)
{
	unsigned long vdso_base, vdso_text_len, vdso_mapping_len;
	void *ret;

	BUILD_BUG_ON(VVAR_NR_PAGES != __VVAR_PAGES);

	vdso_text_len = vdso_info->vdso_pages << PAGE_SHIFT;
	/* Be sure to map the data page */
	vdso_mapping_len = vdso_text_len + VVAR_SIZE;

	vdso_base = get_unmapped_area(NULL, 0, vdso_mapping_len, 0, 0);
	if (IS_ERR_VALUE(vdso_base)) {
		ret = ERR_PTR(vdso_base);
		goto up_fail;
	}

	ret = _install_special_mapping(mm, vdso_base, VVAR_SIZE,
		(VM_READ | VM_MAYREAD | VM_PFNMAP), vdso_info->dm);
	if (IS_ERR(ret))
		goto up_fail;

	vdso_base += VVAR_SIZE;
	mm->context.vdso = (void *)vdso_base;

	ret =
	   _install_special_mapping(mm, vdso_base, vdso_text_len,
		(VM_READ | VM_EXEC | VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC),
		vdso_info->cm);

	if (IS_ERR(ret))
		goto up_fail;

	return 0;

up_fail:
	mm->context.vdso = NULL;
	return PTR_ERR(ret);
}

int arch_setup_additional_pages(struct linux_binprm *bprm, int uses_interp)
{
	struct mm_struct *mm = current->mm;
	int ret;

	if (mmap_write_lock_killable(mm))
		return -EINTR;

	ret = __setup_additional_pages(mm, bprm, uses_interp, &vdso_info);
	mmap_write_unlock(mm);

	return ret;
}

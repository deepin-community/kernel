// SPDX-License-Identifier: GPL-2.0
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Shu Hang <shuh2023@zgclab.edu.cn>
 *          Hu Bing <hubing2023@zgclab.edu.cn>
 */

#include <linux/set_memory.h>
#include <asm/haoc/iee-func.h>
#include "slab.h"
#ifdef CONFIG_IEE_PTRP
#include <asm/haoc/iee-token.h>
#endif
extern bool haoc_enabled;

void set_iee_page(unsigned long addr, int num_pages, enum HAOC_BITMAP_TYPE type)
{
	set_memory_ro(addr, num_pages);
	iee_set_bitmap_type(__pa(addr), num_pages, type);
}

void unset_iee_page(unsigned long addr, int num_pages)
{
	iee_set_bitmap_type(__pa(addr), num_pages, IEE_NORMAL);
	set_memory_rw(addr, num_pages);
}

void set_iee_pages(unsigned long addr, int num_pages, enum HAOC_BITMAP_TYPE type)
{
	set_iee_page(addr, num_pages, type);
}

void unset_iee_pages(unsigned long addr, int num_pages)
{
	unset_iee_page(addr, num_pages);
}

struct iee_free_slab_work {
	struct work_struct work;
	struct kmem_cache *s;
	struct slab *slab;
};

void iee_free_slab(struct kmem_cache *s, struct slab *slab,
		   void (*do_free_slab)(struct work_struct *work))
{
	struct iee_free_slab_work *iee_free_slab_work =
		kmalloc(sizeof(struct iee_free_slab_work), GFP_ATOMIC);

	if (!iee_free_slab_work) {
		pr_warn("HAOC: failed to allocate deferred slab free work\n");
		return;
	}

	iee_free_slab_work->s = s;
	iee_free_slab_work->slab = slab;
	INIT_WORK(&iee_free_slab_work->work, do_free_slab);
	schedule_work(&iee_free_slab_work->work);
}

#ifdef CONFIG_IEE_PTRP
static void iee_free_task_struct_slab(struct work_struct *work)
{
	struct iee_free_slab_work *iee_free_slab_work =
		container_of(work, struct iee_free_slab_work, work);
	struct slab *slab = iee_free_slab_work->slab;
	struct folio *folio = slab_folio(slab);
	unsigned int task_order = folio_order(folio);
	unsigned int token_order = IEE_TOKEN_ORDER(task_order);
	unsigned long token = __slab_to_iee(slab);

	iee_set_token_page_invalid(token, 0, token_order);
	__free_pages(&folio->page, task_order);
	kfree(iee_free_slab_work);
}
#endif

bool iee_free_slab_data(struct kmem_cache *s, struct slab *slab,
			unsigned int order)
{
#ifdef CONFIG_IEE_PTRP
	if (s == task_struct_cachep) {
		iee_free_slab(s, slab, iee_free_task_struct_slab);
		return true;
	}
#endif
	return false;
}

unsigned int iee_calculate_order(struct kmem_cache *s, unsigned int order)
{
	if(!haoc_enabled)
	{
		return order;
	}
#ifdef CONFIG_IEE_PTRP
	if (strcmp(s->name, "task_struct") == 0)
		return IEE_DATA_ORDER;
#endif
#ifdef CONFIG_CREDP
 	if (strcmp(s->name, "cred_jar") == 0)
 		order = IEE_DATA_ORDER;
#endif
	return order;
}

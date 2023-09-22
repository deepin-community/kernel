
#ifndef __GSGPU_VM_IT_H__
#define __GSGPU_VM_IT_H__

#include <linux/rbtree.h>
#include <linux/types.h>
#include "gsgpu_object.h"

/* bo virtual addresses in a vm */
struct gsgpu_bo_va_mapping {
	struct rb_node rb;
	uint64_t start;
	uint64_t last;
	uint64_t __subtree_last;

	struct gsgpu_bo_va *bo_va;
	struct list_head list;
	uint64_t offset;
	uint64_t flags;
};

extern void gsgpu_vm_it_insert(struct gsgpu_bo_va_mapping *node,
					struct rb_root_cached *root);

extern void gsgpu_vm_it_remove(struct gsgpu_bo_va_mapping *node,
					struct rb_root_cached *root);

extern struct gsgpu_bo_va_mapping *
gsgpu_vm_it_iter_first(struct rb_root_cached *root,
					uint64_t start, uint64_t last);

extern struct gsgpu_bo_va_mapping *
gsgpu_vm_it_iter_next(struct gsgpu_bo_va_mapping *node,
					uint64_t start, uint64_t last);

#endif  /* __GSGPU_VM_IT_H__ */

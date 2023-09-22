#include <linux/interval_tree_generic.h>
#include "gsgpu_vm_it.h"

#define START(node) ((node)->start)
#define LAST(node) ((node)->last)

INTERVAL_TREE_DEFINE(struct gsgpu_bo_va_mapping, rb, uint64_t, __subtree_last,
				START, LAST,, gsgpu_vm_it)


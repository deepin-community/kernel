/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __PS3_RBTREE_H__
#define __PS3_RBTREE_H__

#ifndef _WINDOWS
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/bitmap.h>
#endif

#include "ps3_driver_log.h"
#include "ps3_err_def.h"

static inline void setBitNonAtomic(unsigned int nr,
				   unsigned long *addr)
{
	set_bit(nr, addr);
}

static inline void clearBitNonAtomic(unsigned int nr,
				     unsigned long *addr)
{
	clear_bit(nr, addr);
}

static inline int testBitNonAtomic(unsigned int nr,
				   const unsigned long *addr)
{
	return test_bit(nr, addr);
}


struct Ps3RbNode {
	__aligned(8) unsigned long long pParentColor;

	struct {
		struct Ps3RbNode *pLeft;
		struct Ps3RbNode *pRight;
	};
};

struct Ps3RbRoot {
	struct Ps3RbNode *pRoot;
};

#define PS3_RBROOT_INITNIL                                                     \
	{                                                                      \
		NULL                                                           \
	}

enum Ps3Cmp {
	PS3_EQ = 0,
	PS3_GT = 1,
	PS3_LT = 2,
	PS3_CMPNR = 3,
};

enum Ps3RbtreebFlag {
	RBTBF_KEYOFFSET_ENABLE = 0,
	RBTBF_CONFLICT_ENABLE,
};

struct Ps3RbTreeOps {
	enum Ps3Cmp (*cmpkey)(void *pKey1, void *pKey2);

	union {
		void *(*getkey)(struct Ps3RbNode *pNode, void *pCtxt);
		unsigned long long keyoffset;
	};

	unsigned int flags;
	void *pCtxt;
};

struct Ps3RbTree {
	struct Ps3RbRoot root;
	unsigned int nodenr;
	struct Ps3RbTreeOps ops;
};

static inline void ps3RbNodeInit(struct Ps3RbNode *pNode)
{
	pNode->pParentColor = ((uintptr_t)(void *)pNode);
}

static inline void ps3RbNodeLink(struct Ps3RbNode *pNode,
				 struct Ps3RbNode *pParent,
				 struct Ps3RbNode **ppLinker)
{
	pNode->pParentColor = ((uintptr_t)(void *)pParent);
	pNode->pLeft = NULL;
	pNode->pRight = NULL;

	(*ppLinker) = pNode;
}

static inline void *ps3RbNodeGetKey(struct Ps3RbNode *pNode,
				    struct Ps3RbTreeOps *pOps)
{
	if (testBitNonAtomic(RBTBF_KEYOFFSET_ENABLE,
			     (unsigned long *)&pOps->flags)) {
		return (void *)((unsigned char *)pNode + pOps->keyoffset);
	}

	return pOps->getkey(pNode, pOps->pCtxt);
}

static inline void ps3RbRootInit(struct Ps3RbRoot *pRoot)
{
	pRoot->pRoot = NULL;
}

static inline void ps3RbRootFini(struct Ps3RbRoot *pRoot)
{
	BUG_ON(pRoot->pRoot != NULL);
}

void ps3RbtColorAfterAdd(struct Ps3RbRoot *pRoot, struct Ps3RbNode *pNode);

struct Ps3RbNode *ps3RbtHeadNode(struct Ps3RbRoot *pRoot);

struct Ps3RbNode *ps3RbtTailNode(struct Ps3RbRoot *pRoot);

struct Ps3RbNode *ps3RbtPrevNode(struct Ps3RbNode *pNode);

struct Ps3RbNode *ps3RbtNextNode(struct Ps3RbNode *pNode);

void ps3RbtReplaceNode(struct Ps3RbRoot *pRoot, struct Ps3RbNode *pNew,
		       struct Ps3RbNode *pVictim);

int ps3RbtDelNode(struct Ps3RbRoot *pRoot, struct Ps3RbNode *pNode);

int ps3RbtAddNode(struct Ps3RbRoot *pRoot, struct Ps3RbNode *pNode,
		  struct Ps3RbTreeOps *pOps);

struct Ps3RbNode *ps3RbtFindNode(struct Ps3RbRoot *pRoot, void *pKey,
				 struct Ps3RbTreeOps *pOps);

struct Ps3RbNode *ps3RbtFindNextNode(struct Ps3RbRoot *pRoot, void *pKey,
				     struct Ps3RbTreeOps *pOps);

void ps3RbtClean(struct Ps3RbRoot *pRoot);

int ps3RbtTraverse(struct Ps3RbRoot *pRoot,
		   int (*visit)(struct Ps3RbNode *pNode, void *pCtxt),
		   void *pCtxt);

static inline unsigned char ps3RbtIsEmpty(struct Ps3RbRoot *pRoot)
{
	return (unsigned char)(pRoot->pRoot == NULL);
}

static inline int ps3RbTreeInit(struct Ps3RbTree *pTree,
				struct Ps3RbTreeOps *pOps)
{
	int rc = 0;

	ps3RbRootInit(&pTree->root);
	pTree->nodenr = 0;

	memset(&pTree->ops, 0, sizeof(struct Ps3RbTreeOps));
	if (pOps != NULL)
		pTree->ops = (*pOps);

	return rc;
}

static inline int
ps3RbTreeInitGetKey(struct Ps3RbTree *pTree,
		    enum Ps3Cmp (*cmpkey)(void *pKey1, void *pKey2),
		    void *(*getkey)(struct Ps3RbNode *pNode, void *pCtxt),
		    void *pCtxt)
{
	struct Ps3RbTreeOps ops;

	memset(&ops, 0, sizeof(struct Ps3RbTreeOps));
	ops.cmpkey = cmpkey;
	ops.getkey = getkey;
	ops.pCtxt = pCtxt;

	return ps3RbTreeInit(pTree, &ops);
}

static inline int
ps3RbTreeInitKeyOffset(struct Ps3RbTree *pTree,
		       enum Ps3Cmp (*cmpkey)(void *pKey1, void *pKey2),
		       unsigned long long keyoffset, void *pCtxt)
{
	struct Ps3RbTreeOps ops;

	memset(&ops, 0, sizeof(struct Ps3RbTreeOps));
	ops.cmpkey = cmpkey;
	ops.keyoffset = keyoffset;
	ops.pCtxt = pCtxt;
	setBitNonAtomic(RBTBF_KEYOFFSET_ENABLE,
			(unsigned long *)&ops.flags);

	return ps3RbTreeInit(pTree, &ops);
}

static inline void ps3RbTreeFini(struct Ps3RbTree *pTree)
{
	BUG_ON(pTree->nodenr != 0);
	ps3RbRootFini(&pTree->root);
}

static inline struct Ps3RbNode *ps3RbTreeHeadNode(struct Ps3RbTree *pTree)
{
	return ps3RbtHeadNode(&pTree->root);
}

static inline struct Ps3RbNode *ps3RbTreeTailNode(struct Ps3RbTree *pTree)
{
	return ps3RbtTailNode(&pTree->root);
}

static inline struct Ps3RbNode *ps3RbTreePrevNode(struct Ps3RbNode *pNode)
{
	return ps3RbtPrevNode(pNode);
}

static inline struct Ps3RbNode *ps3RbTreeNextNode(struct Ps3RbNode *pNode)
{
	return ps3RbtNextNode(pNode);
}

static inline void ps3RbTreeReplaceNode(struct Ps3RbTree *pTree,
					struct Ps3RbNode *pNew,
					struct Ps3RbNode *pVictim)
{
	ps3RbtReplaceNode(&pTree->root, pNew, pVictim);
}

static inline int ps3RbTreeDelNode(struct Ps3RbTree *pTree,
				   struct Ps3RbNode *pNode)
{
	int rc = 0;

	rc = ps3RbtDelNode(&pTree->root, pNode);
	if (rc >= 0)
		pTree->nodenr--;

	return rc;
}

static inline int ps3RbTreeAddNode(struct Ps3RbTree *pTree,
				   struct Ps3RbNode *pNode)
{
	int rc = 0;

	rc = ps3RbtAddNode(&pTree->root, pNode, &pTree->ops);
	if (rc >= 0)
		pTree->nodenr++;

	return rc;
}

static inline struct Ps3RbNode *ps3RbTreeFindNode(struct Ps3RbTree *pTree,
						  void *pKey)
{
	return ps3RbtFindNode(&pTree->root, pKey, &pTree->ops);
}

static inline struct Ps3RbNode *ps3RbTreeFindNextNode(struct Ps3RbTree *pTree,
						      void *pKey)
{
	return ps3RbtFindNextNode(&pTree->root, pKey, &pTree->ops);
}

static inline void ps3RbTreeClean(struct Ps3RbTree *pTree)
{
	ps3RbtClean(&pTree->root);
	pTree->nodenr = 0;
}

static inline int ps3RbTreeTraverse(struct Ps3RbTree *pTree,
				    int (*visit)(struct Ps3RbNode *pNode,
						 void *pCtxt),
				    void *pCtxt)
{
	return ps3RbtTraverse(&pTree->root, visit, pCtxt);
}

static inline unsigned char ps3RbTreeIsEmpty(struct Ps3RbTree *pTree)
{
	return (unsigned char)(pTree->root.pRoot == NULL);
}

static inline unsigned int ps3RbTreeNodeNr(struct Ps3RbTree *pTree)
{
	return pTree->nodenr;
}

#define RBT_RED (0)
#define RBT_BLACK (1)

#define RBT_PARENT(_n)                                                         \
	((struct Ps3RbNode *)(uintptr_t)((_n)->pParentColor & ~3ULL))
#define RBT_COLOR(_n) ((_n)->pParentColor & 1ULL)

#define RBT_IS_RED(_n) (!RBT_COLOR(_n))
#define RBT_IS_BLACK(_n) RBT_COLOR(_n)
#define RBT_SET_RED(_n) ((_n)->pParentColor &= ~1ULL)
#define RBT_SET_BLACK(_n) ((_n)->pParentColor |= 1ULL)
#define RBT_ROOT_IS_EMPTY(_r) ((_r)->pRoot == NULL)
#define RBT_TREE_IS_EMPTY(_t) RBT_ROOT_IS_EMPTY(&(_t)->root)
#define RBT_NODE_IS_EMPTY(_n) (RBT_PARENT(_n) == (_n))
#define RBT_NODE_CLEAR(_n) (ps3RbNodeInit(_n))
#define RBT_FOR_EACH(_p_node, _p_root)                                         \
	for (_p_node = ps3RbtHeadNode(_p_root); _p_node != NULL;               \
	     _p_node = ps3RbtNextNode(_p_node))

#define RBT_FOR_EACH_SAFE(_p_node, _p_next, _p_root)                           \
	for (_p_node = ps3RbtHeadNode(_p_root),                                \
	    _p_next = ps3RbtNextNode(_p_node);                                 \
	     _p_node != NULL;                                                  \
	     _p_node = _p_next, _p_next = ps3RbtNextNode(_p_node))

#define RBTREE_FOR_EACH(_p_node, _p_tree)                                      \
	RBT_FOR_EACH((_p_node), &(_p_tree)->root)

#define RBTREE_FOR_EACH_SAFE(_p_node, _p_next, _p_tree)                        \
	RBT_FOR_EACH_SAFE((_p_node), (_p_next), &(_p_tree)->root)

#endif

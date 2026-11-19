// SPDX-License-Identifier: GPL-2.0
#include "ps3_rb_tree.h"
static void rbtNodeSetParent(struct Ps3RbNode *pNode, struct Ps3RbNode *pParent)
{
	pNode->pParentColor =
		((pNode->pParentColor & 3ULL) | ((uintptr_t)(void *)pParent));
}

static void rbtNodeSetColor(struct Ps3RbNode *pNode, unsigned int color)
{
	pNode->pParentColor = ((pNode->pParentColor & ~1ULL) | color);
}

static void rbtRotateLeft(struct Ps3RbRoot *pRoot, struct Ps3RbNode *pNode)
{
	struct Ps3RbNode *pRight = pNode->pRight;
	struct Ps3RbNode *pParent = RBT_PARENT(pNode);

	pNode->pRight = pRight->pLeft;
	if (pNode->pRight != NULL)
		rbtNodeSetParent(pRight->pLeft, pNode);

	pRight->pLeft = pNode;
	rbtNodeSetParent(pRight, pParent);

	if (pParent != NULL) {
		if (pNode == pParent->pLeft)
			pParent->pLeft = pRight;
		else
			pParent->pRight = pRight;
	} else {
		pRoot->pRoot = pRight;
	}

	rbtNodeSetParent(pNode, pRight);
}

static void rbtRotateRight(struct Ps3RbRoot *pRoot, struct Ps3RbNode *pNode)
{
	struct Ps3RbNode *pLeft = pNode->pLeft;
	struct Ps3RbNode *pParent = RBT_PARENT(pNode);

	pNode->pLeft = pLeft->pRight;
	if (pNode->pLeft != NULL)
		rbtNodeSetParent(pLeft->pRight, pNode);

	pLeft->pRight = pNode;
	rbtNodeSetParent(pLeft, pParent);

	if (pParent != NULL) {
		if (pNode == pParent->pRight)
			pParent->pRight = pLeft;
		else
			pParent->pLeft = pLeft;
	} else {
		pRoot->pRoot = pLeft;
	}

	rbtNodeSetParent(pNode, pLeft);
}

static void rbtColorAfterDel(struct Ps3RbRoot *pRoot, struct Ps3RbNode *pNode,
			     struct Ps3RbNode *pParent)
{
	struct Ps3RbNode *pOther = NULL;
	struct Ps3RbNode *pOLeft = NULL;
	struct Ps3RbNode *pORight = NULL;

	while (((pNode == NULL) || RBT_IS_BLACK(pNode)) &&
	       (pNode != pRoot->pRoot)) {
		if (pParent->pLeft == pNode) {
			pOther = pParent->pRight;
			if (RBT_IS_RED(pOther)) {
				RBT_SET_BLACK(pOther);
				RBT_SET_RED(pParent);
				rbtRotateLeft(pRoot, pParent);
				pOther = pParent->pRight;
			}
			if (((pOther->pLeft == NULL) ||
			     RBT_IS_BLACK(pOther->pLeft)) &&
			    ((pOther->pRight == NULL) ||
			     RBT_IS_BLACK(pOther->pRight))) {
				RBT_SET_RED(pOther);
				pNode = pParent;
				pParent = RBT_PARENT(pNode);

				continue;
			}
			if ((pOther->pRight == NULL) ||
			    RBT_IS_BLACK(pOther->pRight)) {
				pOLeft = pOther->pLeft;
				if (pOLeft != NULL)
					RBT_SET_BLACK(pOLeft);

				RBT_SET_RED(pOther);
				rbtRotateRight(pRoot, pOther);
				pOther = pParent->pRight;
			}

			rbtNodeSetColor(pOther, RBT_COLOR(pParent));
			RBT_SET_BLACK(pParent);

			if (pOther->pRight != NULL)
				RBT_SET_BLACK(pOther->pRight);

			rbtRotateLeft(pRoot, pParent);
			pNode = pRoot->pRoot;

			break;
		}
		pOther = pParent->pLeft;
		if (RBT_IS_RED(pOther)) {
			RBT_SET_BLACK(pOther);
			RBT_SET_RED(pParent);

			rbtRotateRight(pRoot, pParent);
			pOther = pParent->pLeft;
		}
		if (((pOther->pLeft == NULL) || RBT_IS_BLACK(pOther->pLeft)) &&
		    ((pOther->pRight == NULL) ||
		     RBT_IS_BLACK(pOther->pRight))) {
			RBT_SET_RED(pOther);
			pNode = pParent;
			pParent = RBT_PARENT(pNode);

			continue;
		}
		if ((pOther->pLeft == NULL) || RBT_IS_BLACK(pOther->pLeft)) {
			pORight = pOther->pRight;
			if (pORight != NULL)
				RBT_SET_BLACK(pORight);

			RBT_SET_RED(pOther);
			rbtRotateLeft(pRoot, pOther);
			pOther = pParent->pLeft;
		}

		rbtNodeSetColor(pOther, RBT_COLOR(pParent));
		RBT_SET_BLACK(pParent);

		if (pOther->pLeft != NULL)
			RBT_SET_BLACK(pOther->pLeft);

		rbtRotateRight(pRoot, pParent);
		pNode = pRoot->pRoot;

		break;
	}

	if (pNode != NULL)
		RBT_SET_BLACK(pNode);
}

static void rbtDelNodeDo(struct Ps3RbRoot *pRoot, struct Ps3RbNode *pNode)
{
	struct Ps3RbNode *pParent = NULL;
	struct Ps3RbNode *pChild = NULL;
	struct Ps3RbNode *pOld = NULL;
	unsigned int color = 0;

	if (pNode->pLeft == NULL) {
		pChild = pNode->pRight;
	} else if (pNode->pRight == NULL) {
		pChild = pNode->pLeft;
	} else {
		pOld = pNode;

		pNode = pNode->pRight;
		while (pNode->pLeft != NULL)
			pNode = pNode->pLeft;

		pChild = pNode->pRight;
		pParent = RBT_PARENT(pNode);
		color = RBT_COLOR(pNode);

		if (pChild != NULL)
			rbtNodeSetParent(pChild, pParent);

		if (pParent == pOld) {
			pParent->pRight = pChild;
			pParent = pNode;
		} else {
			pParent->pLeft = pChild;
		}

		pNode->pParentColor = pOld->pParentColor;
		pNode->pRight = pOld->pRight;
		pNode->pLeft = pOld->pLeft;

		if (RBT_PARENT(pOld) != NULL) {
			if (RBT_PARENT(pOld)->pLeft == pOld)
				RBT_PARENT(pOld)->pLeft = pNode;
			else
				RBT_PARENT(pOld)->pRight = pNode;
		} else {
			pRoot->pRoot = pNode;
		}

		rbtNodeSetParent(pOld->pLeft, pNode);
		if (pOld->pRight != NULL)
			rbtNodeSetParent(pOld->pRight, pNode);

		goto l_color;
	}

	pParent = RBT_PARENT(pNode);
	color = RBT_COLOR(pNode);

	if (pChild != NULL)
		rbtNodeSetParent(pChild, pParent);

	if (pParent != NULL) {
		if (pParent->pLeft == pNode)
			pParent->pLeft = pChild;
		else
			pParent->pRight = pChild;
	} else {
		pRoot->pRoot = pChild;
	}

l_color:
	if (color == RBT_BLACK)
		rbtColorAfterDel(pRoot, pChild, pParent);
}

static struct Ps3RbNode *rbtFindNodeDo(struct Ps3RbRoot *pRoot, void *pKey,
				       struct Ps3RbTreeOps *pOps,
				       unsigned char intent_addnode,
				       struct Ps3RbNode **ppParent,
				       struct Ps3RbNode ***pppLinker)
{
	struct Ps3RbNode *pNode = NULL;
	struct Ps3RbNode *pParent = NULL;
	struct Ps3RbNode **ppLinker = NULL;
	void *pKeyCur = NULL;
	enum Ps3Cmp cmprc = PS3_EQ;

	BUG_ON(pOps->cmpkey == NULL);
	BUG_ON((pOps->getkey == NULL) &&
	       (!testBitNonAtomic(RBTBF_KEYOFFSET_ENABLE,
				  (unsigned long *)&pOps->flags)));

	ppLinker = &pRoot->pRoot;
	while (NULL != (*ppLinker)) {
		pParent = (*ppLinker);

		pKeyCur = ps3RbNodeGetKey(pParent, pOps);
		cmprc = pOps->cmpkey(pKey, pKeyCur);
		if (cmprc == PS3_LT) {
			ppLinker = &pParent->pLeft;
		} else if (cmprc == PS3_GT) {
			ppLinker = &pParent->pRight;
		} else if ((intent_addnode == PS3_TRUE) &&
			   testBitNonAtomic(
				   RBTBF_CONFLICT_ENABLE,
				   (unsigned long *)&pOps->flags)) {
			ppLinker = &pParent->pLeft;
		} else {
			pNode = pParent;
			break;
		}
	}

	if (pppLinker != NULL)
		(*pppLinker) = ppLinker;

	if (ppParent != NULL) {
		if (pNode != NULL)
			(*ppParent) = RBT_PARENT(pNode);
		else
			(*ppParent) = pParent;
	}

	return pNode;
}

void ps3RbtColorAfterAdd(struct Ps3RbRoot *pRoot, struct Ps3RbNode *pNode)
{
	struct Ps3RbNode *pGparent = NULL;
	struct Ps3RbNode *pParent = NULL;
	struct Ps3RbNode *pUncle = NULL;
	struct Ps3RbNode *pTmp = NULL;

	while (1) {
		pParent = RBT_PARENT(pNode);
		if ((pParent == NULL) || RBT_IS_BLACK(pParent))
			break;

		pGparent = RBT_PARENT(pParent);
		if (pParent == pGparent->pLeft) {
			pUncle = pGparent->pRight;
			if ((pUncle != NULL) && RBT_IS_RED(pUncle)) {
				RBT_SET_BLACK(pUncle);
				RBT_SET_BLACK(pParent);
				RBT_SET_RED(pGparent);

				pNode = pGparent;
				continue;
			}

			if (pParent->pRight == pNode) {
				rbtRotateLeft(pRoot, pParent);

				pTmp = pParent;
				pParent = pNode;
				pNode = pTmp;
			}

			RBT_SET_BLACK(pParent);
			RBT_SET_RED(pGparent);
			rbtRotateRight(pRoot, pGparent);
		} else {
			pUncle = pGparent->pLeft;
			if ((pUncle != NULL) && RBT_IS_RED(pUncle)) {
				RBT_SET_BLACK(pUncle);
				RBT_SET_BLACK(pParent);
				RBT_SET_RED(pGparent);

				pNode = pGparent;
				continue;
			}

			if (pParent->pLeft == pNode) {
				rbtRotateRight(pRoot, pParent);

				pTmp = pParent;
				pParent = pNode;
				pNode = pTmp;
			}

			RBT_SET_BLACK(pParent);
			RBT_SET_RED(pGparent);
			rbtRotateLeft(pRoot, pGparent);
		}
	}

	RBT_SET_BLACK(pRoot->pRoot);
}

struct Ps3RbNode *ps3RbtHeadNode(struct Ps3RbRoot *pRoot)
{
	struct Ps3RbNode *pNode = NULL;

	pNode = pRoot->pRoot;
	if (pNode == NULL)
		goto end;

	while (pNode->pLeft != NULL)
		pNode = pNode->pLeft;

end:
	return pNode;
}

struct Ps3RbNode *ps3RbtTailNode(struct Ps3RbRoot *pRoot)
{
	struct Ps3RbNode *pNode = NULL;

	pNode = pRoot->pRoot;
	if (pNode == NULL)
		goto end;

	while (pNode->pRight != NULL)
		pNode = pNode->pRight;

end:
	return pNode;
}

struct Ps3RbNode *ps3RbtPrevNode(struct Ps3RbNode *pNode)
{
	struct Ps3RbNode *pParent = NULL;

	if (pNode == NULL)
		goto end;

	if (pNode->pLeft != NULL) {
		pNode = pNode->pLeft;
		while (pNode->pRight != NULL)
			pNode = pNode->pRight;

		return pNode;
	}
	while (1) {
		pParent = RBT_PARENT(pNode);
		if ((pParent == NULL) || (pNode != pParent->pLeft))
			goto end;

		pNode = pParent;
	}

end:
	return pParent;
}

struct Ps3RbNode *ps3RbtNextNode(struct Ps3RbNode *pNode)
{
	struct Ps3RbNode *pParent = NULL;

	if (pNode == NULL)
		goto end;

	if (pNode->pRight != NULL) {
		pNode = pNode->pRight;
		while (pNode->pLeft != NULL)
			pNode = pNode->pLeft;

		return pNode;
	}

	while (1) {
		pParent = RBT_PARENT(pNode);
		if ((pParent == NULL) || (pNode != pParent->pRight))
			goto end;

		pNode = pParent;
	}

end:
	return pParent;
}

void ps3RbtReplaceNode(struct Ps3RbRoot *pRoot, struct Ps3RbNode *pNew,
		       struct Ps3RbNode *pVictim)
{
	struct Ps3RbNode *pParent = RBT_PARENT(pVictim);

	if (pParent != NULL) {
		if (pVictim == pParent->pLeft)
			pParent->pLeft = pNew;
		else
			pParent->pRight = pNew;
	} else {
		pRoot->pRoot = pNew;
	}

	if (pVictim->pLeft != NULL)
		rbtNodeSetParent(pVictim->pLeft, pNew);

	if (pVictim->pRight != NULL)
		rbtNodeSetParent(pVictim->pRight, pNew);

	(*pNew) = (*pVictim);
}

int ps3RbtDelNode(struct Ps3RbRoot *pRoot, struct Ps3RbNode *pNode)
{
	int rc = 0;

	if (RBT_NODE_IS_EMPTY(pNode)) {
		rc = -PS3_FAILED;
		goto end;
	}

	rbtDelNodeDo(pRoot, pNode);
	ps3RbNodeInit(pNode);

end:
	return rc;
}

int ps3RbtAddNode(struct Ps3RbRoot *pRoot, struct Ps3RbNode *pNode,
		  struct Ps3RbTreeOps *pOps)
{
	struct Ps3RbNode *pParent = NULL;
	struct Ps3RbNode **ppLinker = NULL;
	void *pKey = NULL;
	int rc = 0;

	BUG_ON((pOps->getkey == NULL) &&
	       (!testBitNonAtomic(RBTBF_KEYOFFSET_ENABLE,
				  (unsigned long *)&pOps->flags)));

	if (!RBT_NODE_IS_EMPTY(pNode)) {
		rc = -PS3_FAILED;
		goto end;
	}

	pKey = ps3RbNodeGetKey(pNode, pOps);
	if (NULL !=
	    rbtFindNodeDo(pRoot, pKey, pOps, PS3_TRUE, &pParent, &ppLinker)) {
		rc = -PS3_FAILED;
		goto end;
	}

	ps3RbNodeLink(pNode, pParent, ppLinker);
	ps3RbtColorAfterAdd(pRoot, pNode);

end:
	return rc;
}

struct Ps3RbNode *ps3RbtFindNode(struct Ps3RbRoot *pRoot, void *pKey,
				 struct Ps3RbTreeOps *pOps)
{
	struct Ps3RbNode *pNode = NULL;

	if (pKey == NULL) {
		pNode = ps3RbtHeadNode(pRoot);
		goto end;
	}

	pNode = rbtFindNodeDo(pRoot, pKey, pOps, PS3_FALSE, NULL, NULL);

end:
	return pNode;
}

struct Ps3RbNode *ps3RbtFindNextNode(struct Ps3RbRoot *pRoot, void *pKey,
				     struct Ps3RbTreeOps *pOps)
{
	struct Ps3RbNode *pNode = NULL;
	struct Ps3RbNode *pParent = NULL;
	struct Ps3RbNode **ppLinker = NULL;
	void *pKeyCur = NULL;

	if (pKey == NULL) {
		pNode = ps3RbtHeadNode(pRoot);
		goto end;
	}

	pNode = rbtFindNodeDo(pRoot, pKey, pOps, PS3_FALSE, &pParent,
			      &ppLinker);
	if (pNode != NULL) {
		pNode = ps3RbtNextNode(pNode);

		if (!testBitNonAtomic(RBTBF_CONFLICT_ENABLE,
				      (unsigned long *)&pOps->flags)) {
			goto end;
		}

		while (pNode != NULL) {
			pKeyCur = ps3RbNodeGetKey(pNode, pOps);
			if (pOps->cmpkey(pKey, pKeyCur) != PS3_EQ)
				break;

			pNode = ps3RbtNextNode(pNode);
		}

		goto end;
	}

	if (pParent == NULL)
		goto end;

	if (ppLinker == &pParent->pLeft) {
		pNode = pParent;
		goto end;
	}

	pNode = ps3RbtNextNode(pParent);

end:
	return pNode;
}

void ps3RbtClean(struct Ps3RbRoot *pRoot)
{
	struct Ps3RbNode *pNode = NULL;

	pNode = ps3RbtHeadNode(pRoot);
	while (pNode != NULL) {
		(void)ps3RbtDelNode(pRoot, pNode);

		pNode = ps3RbtHeadNode(pRoot);
	}
}

int ps3RbtTraverse(struct Ps3RbRoot *pRoot,
		   int (*visit)(struct Ps3RbNode *pNode, void *pCtxt),
		   void *p_ctxt)
{
	struct Ps3RbNode *pNode = NULL;
	struct Ps3RbNode *pNext = NULL;
	int rc = 0;

	BUG_ON(visit == NULL);

	RBT_FOR_EACH_SAFE(pNode, pNext, pRoot)
	{
		rc = visit(pNode, p_ctxt);
		if (rc < 0)
			goto end;
	}

end:
	return rc;
}

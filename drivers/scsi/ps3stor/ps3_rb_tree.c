
#include "ps3_rb_tree.h"

static void rbtNodeSetParent(Ps3RbNode_s *pNode,
    Ps3RbNode_s *pParent)
{
    pNode->pParentColor =
        ((pNode->pParentColor & 3ULL) | ((uintptr_t)(void*)pParent));
}

static void rbtNodeSetColor(Ps3RbNode_s *pNode, U32 color)
{
    pNode->pParentColor = ((pNode->pParentColor & ~1ULL) | color);
}

static void rbtRotateLeft(Ps3RbRoot_s *pRoot, Ps3RbNode_s *pNode)
{
    Ps3RbNode_s *pRight = pNode->pRight;
    Ps3RbNode_s *pParent = RBT_PARENT(pNode);

    pNode->pRight = pRight->pLeft;
    if (NULL != pNode->pRight)
    {
        rbtNodeSetParent(pRight->pLeft, pNode);
    }

    pRight->pLeft = pNode;
    rbtNodeSetParent(pRight, pParent);

    if (NULL != pParent)
    {
        if (pNode == pParent->pLeft)
        {
            pParent->pLeft = pRight;
        }
        else
        {
            pParent->pRight = pRight;
        }
    }
    else
    {
        pRoot->pRoot = pRight;
    }

    rbtNodeSetParent(pNode, pRight);
}

static void rbtRotateRight(Ps3RbRoot_s *pRoot, Ps3RbNode_s *pNode)
{
    Ps3RbNode_s *pLeft = pNode->pLeft;
    Ps3RbNode_s *pParent = RBT_PARENT(pNode);

    pNode->pLeft = pLeft->pRight;
    if (NULL != pNode->pLeft)
    {
        rbtNodeSetParent(pLeft->pRight, pNode);
    }

    pLeft->pRight = pNode;
    rbtNodeSetParent(pLeft, pParent);

    if (NULL != pParent)
    {
        if (pNode == pParent->pRight)
        {
            pParent->pRight = pLeft;
        }
        else
        {
            pParent->pLeft = pLeft;
        }
    }
    else
    {
        pRoot->pRoot = pLeft;
    }

    rbtNodeSetParent(pNode, pLeft);
}

static void rbtColorAfterDel(Ps3RbRoot_s *pRoot,
    Ps3RbNode_s *pNode, Ps3RbNode_s *pParent)
{
    Ps3RbNode_s *pOther = NULL;
    Ps3RbNode_s *pOLeft = NULL;
    Ps3RbNode_s *pORight = NULL;

    while (((NULL == pNode) || RBT_IS_BLACK(pNode)) &&
        (pNode != pRoot->pRoot))
    {
        if (pParent->pLeft == pNode)
        {
            pOther = pParent->pRight;

            if (RBT_IS_RED(pOther))
            {
                RBT_SET_BLACK(pOther);
                RBT_SET_RED(pParent);
                rbtRotateLeft(pRoot, pParent);
                pOther = pParent->pRight;
            }

            if (((NULL == pOther->pLeft) || RBT_IS_BLACK(pOther->pLeft)) &&
                ((NULL == pOther->pRight) || RBT_IS_BLACK(pOther->pRight)))
            {
                RBT_SET_RED(pOther);
                pNode = pParent;
                pParent = RBT_PARENT(pNode);

                continue ;
            }

            if ((NULL == pOther->pRight) || RBT_IS_BLACK(pOther->pRight))
            {
                pOLeft = pOther->pLeft;
                if (NULL != pOLeft)
                {
                    RBT_SET_BLACK(pOLeft);
                }

                RBT_SET_RED(pOther);
                rbtRotateRight(pRoot, pOther);
                pOther = pParent->pRight;
            }

            rbtNodeSetColor(pOther, RBT_COLOR(pParent));
            RBT_SET_BLACK(pParent);

            if (NULL != pOther->pRight)
            {
                RBT_SET_BLACK(pOther->pRight);
            }

            rbtRotateLeft(pRoot, pParent);
            pNode = pRoot->pRoot;

            break;
        }

        pOther = pParent->pLeft;

        if (RBT_IS_RED(pOther))
        {
            RBT_SET_BLACK(pOther);
            RBT_SET_RED(pParent);

            rbtRotateRight(pRoot, pParent);
            pOther = pParent->pLeft;
        }

        if (((NULL == pOther->pLeft) || RBT_IS_BLACK(pOther->pLeft)) &&
            ((NULL == pOther->pRight) || RBT_IS_BLACK(pOther->pRight)))
        {
            RBT_SET_RED(pOther);
            pNode = pParent;
            pParent = RBT_PARENT(pNode);

            continue ;
        }

        if ((NULL == pOther->pLeft) || RBT_IS_BLACK(pOther->pLeft))
        {
            pORight = pOther->pRight;
            if (NULL != pORight)
            {
                RBT_SET_BLACK(pORight);
            }

            RBT_SET_RED(pOther);
            rbtRotateLeft(pRoot, pOther);
            pOther = pParent->pLeft;
        }

        rbtNodeSetColor(pOther, RBT_COLOR(pParent));
        RBT_SET_BLACK(pParent);

        if (NULL != pOther->pLeft)
        {
            RBT_SET_BLACK(pOther->pLeft);
        }

        rbtRotateRight(pRoot, pParent);
        pNode = pRoot->pRoot;

        break;
    }

    if (NULL != pNode)
    {
        RBT_SET_BLACK(pNode);
    }
}

void rbtDelNodeDo(Ps3RbRoot_s *pRoot, Ps3RbNode_s *pNode)
{
    Ps3RbNode_s *pParent = NULL;
    Ps3RbNode_s *pChild = NULL;
    Ps3RbNode_s *pOld = NULL;
    U32 color = 0;

    if (NULL == pNode->pLeft)
    {
        pChild = pNode->pRight;
    }
    else if (NULL == pNode->pRight)
    {
        pChild = pNode->pLeft;
    }
    else
    {
        pOld = pNode;

        pNode = pNode->pRight;
        while (NULL != pNode->pLeft)
        {
            pNode = pNode->pLeft;
        }

        pChild = pNode->pRight;
        pParent = RBT_PARENT(pNode);
        color = RBT_COLOR(pNode);

        if (NULL != pChild)
        {
            rbtNodeSetParent(pChild, pParent);
        }

        if (pParent == pOld)
        {
            pParent->pRight = pChild;
            pParent = pNode;
        }
        else
        {
            pParent->pLeft = pChild;
        }

        pNode->pParentColor = pOld->pParentColor;
        pNode->pRight = pOld->pRight;
        pNode->pLeft = pOld->pLeft;

        if (NULL != RBT_PARENT(pOld))
        {
            if (RBT_PARENT(pOld)->pLeft == pOld)
            {
                RBT_PARENT(pOld)->pLeft = pNode;
            }
            else
            {
                RBT_PARENT(pOld)->pRight = pNode;
            }
        }
        else
        {
            pRoot->pRoot = pNode;
        }

        rbtNodeSetParent(pOld->pLeft, pNode);
        if (NULL != pOld->pRight)
        {
            rbtNodeSetParent(pOld->pRight, pNode);
        }

        goto l_color;
    }

    pParent = RBT_PARENT(pNode);
    color = RBT_COLOR(pNode);

    if (NULL != pChild)
    {
        rbtNodeSetParent(pChild, pParent);
    }

    if (NULL != pParent)
    {
        if (pParent->pLeft == pNode)
        {
            pParent->pLeft = pChild;
        }
        else
        {
            pParent->pRight = pChild;
        }
    }
    else
    {
        pRoot->pRoot = pChild;
    }

l_color:
    if (color == RBT_BLACK)
    {
        rbtColorAfterDel(pRoot, pChild, pParent);
    }
}

static Ps3RbNode_s* rbtFindNodeDo(Ps3RbRoot_s *pRoot, void *pKey,
    Ps3RbTreeOps_s *pOps, Bool intent_addnode, Ps3RbNode_s **ppParent,
    Ps3RbNode_s ***pppLinker)
{
    Ps3RbNode_s *pNode = NULL;
    Ps3RbNode_s *pParent = NULL;
    Ps3RbNode_s **ppLinker = NULL;
    void *pKeyCur = NULL;
    Ps3Cmp_e cmprc = PS3_EQ;

    BUG_ON(NULL == pOps->cmpkey);
    BUG_ON((NULL == pOps->getkey) &&
        (!testBitNonAtomic(RBTBF_KEYOFFSET_ENABLE, (volatile ULong *)&pOps->flags)));

    ppLinker = &pRoot->pRoot;
    while (NULL != (*ppLinker))
    {
        pParent = (*ppLinker);

        pKeyCur = ps3RbNodeGetKey(pParent, pOps);
        cmprc = pOps->cmpkey(pKey, pKeyCur);
        if (PS3_LT == cmprc)
        {
            ppLinker = &pParent->pLeft;
        }
        else if (PS3_GT == cmprc)
        {
            ppLinker = &pParent->pRight;
        }
        else if ((PS3_TRUE == intent_addnode) &&
            testBitNonAtomic(RBTBF_CONFLICT_ENABLE, (volatile ULong *)&pOps->flags))
        {
            ppLinker = &pParent->pLeft;
        }
        else
        {
            pNode = pParent;
            break ;
        }
    }

    if (NULL != pppLinker)
    {
        (*pppLinker) = ppLinker;
    }

    if (NULL != ppParent)
    {
        if (NULL != pNode)
        {
            (*ppParent) = RBT_PARENT(pNode);
        }
        else
        {
            (*ppParent) = pParent;
        }
    }

    return pNode;
}

void ps3RbtColorAfterAdd(Ps3RbRoot_s *pRoot, Ps3RbNode_s *pNode)
{
    Ps3RbNode_s *pGparent = NULL;
    Ps3RbNode_s *pParent = NULL;
    Ps3RbNode_s *pUncle = NULL;
    Ps3RbNode_s *pTmp = NULL;

    while (1)
    {
        pParent = RBT_PARENT(pNode);
        if ((NULL == pParent) || RBT_IS_BLACK(pParent))
        {
            break ;
        }

        pGparent = RBT_PARENT(pParent);
        if (pParent == pGparent->pLeft)
        {
            pUncle = pGparent->pRight;
            if ((NULL != pUncle) && RBT_IS_RED(pUncle))
            {
                RBT_SET_BLACK(pUncle);
                RBT_SET_BLACK(pParent);
                RBT_SET_RED(pGparent);

                pNode = pGparent;
                continue ;
            }

            if (pParent->pRight == pNode)
            {
                rbtRotateLeft(pRoot, pParent);

                pTmp = pParent;
                pParent = pNode;
                pNode = pTmp;
            }

            RBT_SET_BLACK(pParent);
            RBT_SET_RED(pGparent);
            rbtRotateRight(pRoot, pGparent);
        }
        else
        {
            pUncle = pGparent->pLeft;
            if ((NULL != pUncle) && RBT_IS_RED(pUncle))
            {
                RBT_SET_BLACK(pUncle);
                RBT_SET_BLACK(pParent);
                RBT_SET_RED(pGparent);

                pNode = pGparent;
                continue ;
            }

            if (pParent->pLeft == pNode)
            {
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

Ps3RbNode_s* ps3RbtHeadNode(Ps3RbRoot_s *pRoot)
{
    Ps3RbNode_s *pNode = NULL;

    pNode = pRoot->pRoot;
    if (NULL == pNode)
    {
        goto end;
    }

    while (NULL != pNode->pLeft)
    {
        pNode = pNode->pLeft;
    }

end:
    return pNode;
}

Ps3RbNode_s* ps3RbtTailNode(Ps3RbRoot_s *pRoot)
{
    Ps3RbNode_s *pNode = NULL;

    pNode = pRoot->pRoot;
    if (NULL == pNode)
    {
        goto end;
    }

    while (NULL != pNode->pRight)
    {
        pNode = pNode->pRight;
    }

end:
    return pNode;
}

Ps3RbNode_s* ps3RbtPrevNode(Ps3RbNode_s *pNode)
{
    Ps3RbNode_s *pParent = NULL;

    if (NULL == pNode)
    {
        goto end;
    }

    if (NULL != pNode->pLeft)
    {
        pNode = pNode->pLeft;
        while (NULL != pNode->pRight)
        {
            pNode = pNode->pRight;
        }

        return pNode;
    }

    while (1)
    {
        pParent = RBT_PARENT(pNode);
        if ((NULL == pParent) || (pNode != pParent->pLeft))
        {
            goto end;
        }

        pNode = pParent;
    }

end:
    return pParent;
}

Ps3RbNode_s* ps3RbtNextNode(Ps3RbNode_s *pNode)
{
    Ps3RbNode_s *pParent = NULL;

    if (NULL == pNode)
    {
        goto end;
    }

    if (NULL != pNode->pRight)
    {
        pNode = pNode->pRight;
        while (NULL != pNode->pLeft)
        {
            pNode = pNode->pLeft;
        }

        return pNode;
    }

    while (1)
    {
        pParent = RBT_PARENT(pNode);
        if ((NULL == pParent) || (pNode != pParent->pRight))
        {
            goto end;
        }

        pNode = pParent;
    }

end:
    return pParent;
}

void ps3RbtReplaceNode(Ps3RbRoot_s *pRoot, Ps3RbNode_s *pNew,
    Ps3RbNode_s *pVictim)
{
    Ps3RbNode_s *pParent = RBT_PARENT(pVictim);

    if (NULL != pParent)
    {
        if (pVictim == pParent->pLeft)
        {
            pParent->pLeft = pNew;
        }
        else
        {
            pParent->pRight = pNew;
        }
    }
    else
    {
        pRoot->pRoot = pNew;
    }

    if (NULL != pVictim->pLeft)
    {
        rbtNodeSetParent(pVictim->pLeft, pNew);
    }

    if (NULL != pVictim->pRight)
    {
        rbtNodeSetParent(pVictim->pRight, pNew);
    }

    (*pNew) = (*pVictim);
}

S32 ps3RbtDelNode(Ps3RbRoot_s *pRoot, Ps3RbNode_s *pNode)
{
    S32 rc = 0;

    if (RBT_NODE_IS_EMPTY(pNode))
    {
        rc = -PS3_FAILED;
        goto end;
    }

    rbtDelNodeDo(pRoot, pNode);
    ps3RbNodeInit(pNode);

end:
    return rc;
}

S32 ps3RbtAddNode(Ps3RbRoot_s *pRoot, Ps3RbNode_s *pNode,
    Ps3RbTreeOps_s *pOps)
{
    Ps3RbNode_s *pParent = NULL;
    Ps3RbNode_s **ppLinker = NULL;
    void *pKey = NULL;
    S32 rc = 0;

    BUG_ON((NULL == pOps->getkey) &&
        (!testBitNonAtomic(RBTBF_KEYOFFSET_ENABLE, (volatile ULong *)&pOps->flags)));

    if (!RBT_NODE_IS_EMPTY(pNode))
    {
        rc = -PS3_FAILED;
        goto end;
    }

    pKey = ps3RbNodeGetKey(pNode, pOps);
    if (NULL != rbtFindNodeDo(pRoot, pKey, pOps, PS3_TRUE, &pParent,
        &ppLinker))
    {
        rc = -PS3_FAILED;
        goto end;
    }

    ps3RbNodeLink(pNode, pParent, ppLinker);
    ps3RbtColorAfterAdd(pRoot, pNode);

end:
    return rc;
}

Ps3RbNode_s* ps3RbtFindNode(Ps3RbRoot_s *pRoot, void *pKey,
    Ps3RbTreeOps_s *pOps)
{
    Ps3RbNode_s *pNode = NULL;

    if (NULL == pKey)
    {
        pNode = ps3RbtHeadNode(pRoot);
        goto end;
    }

    pNode = rbtFindNodeDo(pRoot, pKey, pOps, PS3_FALSE, NULL, NULL);

end:
    return pNode;
}

Ps3RbNode_s* ps3RbtFindNextNode(Ps3RbRoot_s *pRoot, void *pKey,
    Ps3RbTreeOps_s *pOps)
{
    Ps3RbNode_s *pNode = NULL;
    Ps3RbNode_s *pParent = NULL;
    Ps3RbNode_s **ppLinker = NULL;
    void *pKeyCur = NULL;

    if (NULL == pKey)
    {
        pNode = ps3RbtHeadNode(pRoot);
        goto end;
    }

    pNode = rbtFindNodeDo(pRoot, pKey, pOps, PS3_FALSE, &pParent,
        &ppLinker);
    if (NULL != pNode)
    {
        pNode = ps3RbtNextNode(pNode);

        if (!testBitNonAtomic(RBTBF_CONFLICT_ENABLE, (volatile ULong *)&pOps->flags))
        {
            goto end;
        }

        while (NULL != pNode)
        {
            pKeyCur = ps3RbNodeGetKey(pNode, pOps);
            if (PS3_EQ != pOps->cmpkey(pKey, pKeyCur))
            {
                break;
            }

            pNode = ps3RbtNextNode(pNode);
        }

        goto end;
    }

    if (NULL == pParent)
    {
        goto end;
    }

    if (ppLinker == &pParent->pLeft)
    {
        pNode = pParent;
        goto end;
    }

    pNode = ps3RbtNextNode(pParent);

end:
    return pNode;
}

void ps3RbtClean(Ps3RbRoot_s *pRoot)
{
    Ps3RbNode_s *pNode = NULL;

    pNode = ps3RbtHeadNode(pRoot);
    while (NULL != pNode)
    {
        (void)ps3RbtDelNode(pRoot, pNode);

        pNode = ps3RbtHeadNode(pRoot);
    }
}

S32 ps3RbtTraverse(Ps3RbRoot_s *pRoot, ps3RbtreeVisitFunc visit,
    void *p_ctxt)
{
    Ps3RbNode_s *pNode = NULL;
    Ps3RbNode_s *pNext = NULL;
    S32 rc = 0;

    BUG_ON(NULL == visit);

    RBT_FOR_EACH_SAFE(pNode, pNext, pRoot)
    {
        rc = visit(pNode, p_ctxt);
        if (rc < 0)
        {
            goto end;
        }
    }

end:
    return rc;
}


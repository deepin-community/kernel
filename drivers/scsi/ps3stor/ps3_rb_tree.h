
#ifndef __PS3_RBTREE_H__
#define __PS3_RBTREE_H__

#ifndef _WINDOWS
#include <linux/kernel.h>
#include <linux/version.h>
#endif

#include "ps3_driver_log.h"
#include "ps3_define.h"
#include "ps3_err_def.h"

static inline void setBitNonAtomic(U32 nr, volatile ULong *addr)
{
    ULong mask = BIT_MASK(nr);
    ULong *p   = ((ULong *)addr) + BIT_WORD(nr);

    *p |= mask;
}

static inline void clearBitNonAtomic(U32 nr, volatile ULong *addr)
{
    ULong mask = BIT_MASK(nr);
    ULong *p   = ((ULong *)addr) + BIT_WORD(nr);

    *p &= ~mask;
}

static inline S32 testBitNonAtomic(U32 nr, const volatile ULong *addr)
{
    return 1UL & (addr[BIT_WORD(nr)] >> (nr & (BITS_PER_LONG-1)));
}


typedef struct Ps3RbNode
{
    __attribute__((__aligned__(8)))
    U64 pParentColor;                  

    struct
    {
        struct Ps3RbNode *pLeft;  
        struct Ps3RbNode *pRight; 
    };
} Ps3RbNode_s;

typedef struct Ps3RbRoot
{
    Ps3RbNode_s *pRoot;                   
} Ps3RbRoot_s;

#define PS3_RBROOT_INITNIL          {NULL}

typedef enum Ps3Cmp
{
    PS3_EQ = 0,    
    PS3_GT = 1,    
    PS3_LT = 2,    
    PS3_CMPNR = 3,
} Ps3Cmp_e;

typedef Ps3Cmp_e (*ps3CmpKeyFunc)(void *pKey1, void *pKey2);

typedef void* (*ps3RbtreeGetKeyFunc)(Ps3RbNode_s *pNode, void *pCtxt);

typedef S32 (*ps3RbtreeVisitFunc)(Ps3RbNode_s *pNode, void *pCtxt);

typedef enum Ps3RbtreebFlag
{
    RBTBF_KEYOFFSET_ENABLE = 0,              
    RBTBF_CONFLICT_ENABLE,                  
} Ps3RbtreebFlag_e;

typedef struct Ps3RbTreeOps
{
    ps3CmpKeyFunc cmpkey;                     

    union
    {
        ps3RbtreeGetKeyFunc getkey;    
        U64 keyoffset;                 
    };

    U32 flags;                         
    void *pCtxt;                      
} Ps3RbTreeOps_s;

typedef struct Ps3RbTree
{
    Ps3RbRoot_s root;                  
    U32 nodenr;                        
    Ps3RbTreeOps_s ops;                
} Ps3RbTree_s;

static inline void ps3RbNodeInit(Ps3RbNode_s *pNode)
{
    pNode->pParentColor = ((uintptr_t)(void*)pNode);
}

static inline void ps3RbNodeLink(Ps3RbNode_s *pNode,
    Ps3RbNode_s *pParent, Ps3RbNode_s **ppLinker)
{
    pNode->pParentColor = ((uintptr_t)(void*)pParent);
    pNode->pLeft = NULL;
    pNode->pRight = NULL;

    (*ppLinker) = pNode;
}

static inline void* ps3RbNodeGetKey(Ps3RbNode_s *pNode,
    Ps3RbTreeOps_s *pOps)
{
    if (testBitNonAtomic(RBTBF_KEYOFFSET_ENABLE, (volatile ULong *)&pOps->flags))
    {
        return (void *)((U8 *)pNode + pOps->keyoffset);
    }

    return pOps->getkey(pNode, pOps->pCtxt);
}

static inline void ps3RbRootInit(Ps3RbRoot_s *pRoot)
{
    pRoot->pRoot = NULL;
}

static inline void ps3RbRootFini(Ps3RbRoot_s *pRoot)
{
	BUG_ON(pRoot->pRoot != NULL);
}

void ps3RbtColorAfterAdd(Ps3RbRoot_s *pRoot, Ps3RbNode_s *pNode);

Ps3RbNode_s* ps3RbtHeadNode(Ps3RbRoot_s *pRoot);

Ps3RbNode_s* ps3RbtTailNode(Ps3RbRoot_s *pRoot);

Ps3RbNode_s* ps3RbtPrevNode(Ps3RbNode_s *pNode);

Ps3RbNode_s* ps3RbtNextNode(Ps3RbNode_s *pNode);

void ps3RbtReplaceNode(Ps3RbRoot_s *pRoot, Ps3RbNode_s *pNew,
    Ps3RbNode_s *pVictim);

S32 ps3RbtDelNode(Ps3RbRoot_s *pRoot, Ps3RbNode_s *pNode);

S32 ps3RbtAddNode(Ps3RbRoot_s *pRoot, Ps3RbNode_s *pNode,
    Ps3RbTreeOps_s *pOps);

Ps3RbNode_s* ps3RbtFindNode(Ps3RbRoot_s *pRoot, void *pKey,
    Ps3RbTreeOps_s *pOps);

Ps3RbNode_s* ps3RbtFindNextNode(Ps3RbRoot_s *pRoot, void *pKey,
    Ps3RbTreeOps_s *pOps);

void ps3RbtClean(Ps3RbRoot_s *pRoot);

S32 ps3RbtTraverse(Ps3RbRoot_s *pRoot, ps3RbtreeVisitFunc visit,
    void *pCtxt);

static inline Bool ps3RbtIsEmpty(Ps3RbRoot_s *pRoot)
{
    return (Bool)(pRoot->pRoot == NULL);
}

static inline S32 ps3RbTreeInit(Ps3RbTree_s *pTree,
    Ps3RbTreeOps_s *pOps)
{
    S32 rc = 0;

    ps3RbRootInit(&pTree->root);
    pTree->nodenr = 0;

    memset(&pTree->ops, 0, sizeof(Ps3RbTreeOps_s));
    if (NULL != pOps)
    {
        pTree->ops = (*pOps);
    }

    return rc;
}

static inline S32 ps3RbTreeInitGetKey(Ps3RbTree_s *pTree,
    ps3CmpKeyFunc cmpkey, ps3RbtreeGetKeyFunc getkey, void *pCtxt)
{
    Ps3RbTreeOps_s ops;

    memset(&ops, 0, sizeof(Ps3RbTreeOps_s));
    ops.cmpkey = cmpkey;
    ops.getkey = getkey;
    ops.pCtxt = pCtxt;

    return ps3RbTreeInit(pTree, &ops);
}

static inline S32 ps3RbTreeInitKeyOffset(Ps3RbTree_s *pTree,
    ps3CmpKeyFunc cmpkey, U64 keyoffset, void *pCtxt)
{
    Ps3RbTreeOps_s ops;

    memset(&ops, 0, sizeof(Ps3RbTreeOps_s));
    ops.cmpkey = cmpkey;
    ops.keyoffset = keyoffset;
    ops.pCtxt = pCtxt;
    setBitNonAtomic(RBTBF_KEYOFFSET_ENABLE, (volatile ULong *)&ops.flags);

    return ps3RbTreeInit(pTree, &ops);
}

static inline void ps3RbTreeFini(Ps3RbTree_s *pTree)
{
	BUG_ON(pTree->nodenr != 0);
	ps3RbRootFini(&pTree->root);
}

static inline Ps3RbNode_s* ps3RbTreeHeadNode(Ps3RbTree_s *pTree)
{
    return ps3RbtHeadNode(&pTree->root);
}

static inline Ps3RbNode_s* ps3RbTreeTailNode(Ps3RbTree_s *pTree)
{
    return ps3RbtTailNode(&pTree->root);
}

static inline Ps3RbNode_s* ps3RbTreePrevNode(Ps3RbNode_s *pNode)
{
    return ps3RbtPrevNode(pNode);
}

static inline Ps3RbNode_s* ps3RbTreeNextNode(Ps3RbNode_s *pNode)
{
    return ps3RbtNextNode(pNode);
}

static inline void ps3RbTreeReplaceNode(Ps3RbTree_s *pTree,
    Ps3RbNode_s *pNew, Ps3RbNode_s *pVictim)
{
    ps3RbtReplaceNode(&pTree->root, pNew, pVictim);
}

static inline S32 ps3RbTreeDelNode(Ps3RbTree_s *pTree,
    Ps3RbNode_s *pNode)
{
    S32 rc = 0;

    rc = ps3RbtDelNode(&pTree->root, pNode);
    if (rc >= 0)
    {
        pTree->nodenr--;
    }

    return rc;
}

static inline S32 ps3RbTreeAddNode(Ps3RbTree_s *pTree,
    Ps3RbNode_s *pNode)
{
    S32 rc = 0;

    rc = ps3RbtAddNode(&pTree->root, pNode, &pTree->ops);
    if (rc >= 0)
    {
        pTree->nodenr++;
    }

    return rc;
}

static inline Ps3RbNode_s* ps3RbTreeFindNode(Ps3RbTree_s *pTree,
    void *pKey)
{
    return ps3RbtFindNode(&pTree->root, pKey, &pTree->ops);
}

static inline Ps3RbNode_s* ps3RbTreeFindNextNode(Ps3RbTree_s *pTree,
    void *pKey)
{
    return ps3RbtFindNextNode(&pTree->root, pKey, &pTree->ops);
}

static inline void ps3RbTreeClean(Ps3RbTree_s *pTree)
{
    ps3RbtClean(&pTree->root);
    pTree->nodenr = 0;
}

static inline S32 ps3RbTreeTraverse(Ps3RbTree_s *pTree,
    ps3RbtreeVisitFunc visit, void *pCtxt)
{
    return ps3RbtTraverse(&pTree->root, visit, pCtxt);
}

static inline Bool ps3RbTreeIsEmpty(Ps3RbTree_s *pTree)
{
    return (Bool)(pTree->root.pRoot == NULL);
}

static inline U32 ps3RbTreeNodeNr(Ps3RbTree_s *pTree)
{
    return pTree->nodenr;
}

#define RBT_RED                   (0)
#define RBT_BLACK                 (1)

#define RBT_PARENT(_n)            ((Ps3RbNode_s*)(uintptr_t)((_n)->pParentColor & ~3ULL))
#define RBT_COLOR(_n)             ((_n)->pParentColor & 1ULL)

#define RBT_IS_RED(_n)            (!RBT_COLOR(_n))
#define RBT_IS_BLACK(_n)          RBT_COLOR(_n)
#define RBT_SET_RED(_n)           \
    do { (_n)->pParentColor &= ~1ULL; } while (0)
#define RBT_SET_BLACK(_n)         \
    do { (_n)->pParentColor |= 1ULL; } while (0)

#define RBT_ROOT_IS_EMPTY(_r)     ((_r)->pRoot == NULL)
#define RBT_TREE_IS_EMPTY(_t)     RBT_ROOT_IS_EMPTY(&(_t)->root)
#define RBT_NODE_IS_EMPTY(_n)     (RBT_PARENT(_n) == (_n))
#define RBT_NODE_CLEAR(_n)        do { ps3RbNodeInit(_n); } while (0)

#define RBT_FOR_EACH(_p_node, _p_root)                               \
    for (_p_node = ps3RbtHeadNode(_p_root);                        \
        _p_node != NULL;                                             \
        _p_node = ps3RbtNextNode(_p_node))

#define RBT_FOR_EACH_SAFE(_p_node, _p_next, _p_root)                 \
    for (_p_node = ps3RbtHeadNode(_p_root),                        \
        _p_next = ps3RbtNextNode(_p_node);                         \
        _p_node != NULL;                                             \
        _p_node = _p_next,                                           \
        _p_next = ps3RbtNextNode(_p_node))

#define RBTREE_FOR_EACH(_p_node, _p_tree)                            \
    RBT_FOR_EACH((_p_node), &(_p_tree)->root)

#define RBTREE_FOR_EACH_SAFE(_p_node, _p_next, _p_tree)              \
    RBT_FOR_EACH_SAFE((_p_node), (_p_next), &(_p_tree)->root)

#endif


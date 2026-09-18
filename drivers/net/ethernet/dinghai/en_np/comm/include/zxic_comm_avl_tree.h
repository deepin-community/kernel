/*********************************************************************
 * 版权所有 (C)2012, 深圳市中兴通讯股份有限公司。
 *
 * 文件名称：zxic_avl_tree.h
 * 文件标识：
 * 内容摘要：//  本文件从pfe_index 移植过来
 * 其它说明：// 其它内容的说明
 * 当前版本：
 * 作    者：
 * 完成日期：
 *
 ********************************************************************/
#ifndef __ZXIC_COMM_AVL_TREE_H__
#define __ZXIC_COMM_AVL_TREE_H__


/* Since the trees are balanced, their heignse will never be large. */
#define avl_maxheight    41 /* why this? a small exercise */
#define heightoftree(tree)  ((tree) == NULL ? 0 : (tree)->avl_height)

struct _ZXIC_AVL_CFG;
struct _ZXIC_AVL_NODE;

#define ZXIC_LIST_ENTRY(ptr, type, member) \
    ((type *)((ZXIC_UINT8 *)(ptr)-(((unsigned long)(&((type *)64)->member)) - 64)))

#define ZXIC_GET_AVL_KEY_ADDR(p_avl_cfg,key_index) \
                  ((p_avl_cfg->p_key_base)+(p_avl_cfg->key_len*(key_index)));

typedef ZXIC_SINT32 (*ZXIC_KEY_CMP_FUNC)(void *p_new_key, void *p_old_key, ZXIC_UINT32 key_len);

typedef struct _ZXIC_AVL_NODE
{
    void                   *p_key;
    //void                   *p_owner;      /*the owner of this node*/
    ZXIC_UINT32                  result;       /*该节点的索引*/
    ZXIC_SINT32                 avl_height;
    struct _ZXIC_AVL_NODE   *p_avl_left;
    struct _ZXIC_AVL_NODE   *p_avl_right;
    D_NODE                  avl_node_list;/*the data is owner*/
} ZXIC_AVL_NODE;

typedef struct _ZXIC_AVL_CFG
{

    ZXIC_AVL_NODE         *p_root;
    ZXIC_UINT32                avl_node_num;          /*avl 中已使用节点的数目*/
    D_HEAD                avl_node_list_head;    /*avl 线索化节点的头节点*/
    ZXIC_UINT32                key_len;
    ZXIC_UINT32                item_num;
    ZXIC_KEY_CMP_FUNC      avl_cmp_func;
    ZXIC_UINT8                 *p_key_base;
    ZXIC_AVL_NODE         *p_avl_node_base;
    ZXIC_LISTSTACK_MANGER *p_avl_node_liststack;

    ZXIC_UINT32                is_dynamic;            /*是否支持avl 的节点动态生成，1:支持;0:不支持*/    
    ZXIC_UINT32                is_init;

} ZXIC_AVL_CFG;



ZXIC_RTN32 zxic_comm_avl_init(ZXIC_AVL_CFG*        p_avl_cfg, /* avl配置*/
                              ZXIC_UINT32          item_num,    /* 插入键值的数目,如果为0表示动态申请节点*/
                              ZXIC_UINT32          key_length,  /* 插入键值的长度，以字节为单位*/
                              ZXIC_KEY_CMP_FUNC    avl_cmp_func);   /* 键值比较函数,使用默认比较函数可以为NULL,
                                                                       如果按整型比较，需要用户提供比较函数*/
ZXIC_RTN32 zxic_comm_avl_insert(ZXIC_AVL_CFG*   p_avl_cfg,
                                void*           p_new_key,
                                ZXIC_UINT32*    p_index);

ZXIC_RTN32 zxic_comm_avl_remove(ZXIC_AVL_CFG*  p_avl_cfg,
                                void*          p_delete_key,
                                void*          p_out);

ZXIC_RTN32 zxic_comm_avl_find(ZXIC_AVL_CFG*   p_avl_cfg,
                              void*           p_find_key,
                              void*           p_out);

ZXIC_RTN32 zxic_comm_avl_destroy(ZXIC_AVL_CFG*  p_avl_cfg );




ZXIC_UINT32 ic_comm_avl_get_node_num(ZXIC_AVL_CFG* p_avl_cfg);
ZXIC_UINT32 ic_comm_avl_is_none(ZXIC_AVL_CFG* p_avl_cfg);
ZXIC_RTN32 ic_comm_avl_get_1st_key(ZXIC_AVL_CFG*  p_avl_cfg, void *p_key_out);
ZXIC_RTN32 ic_comm_avl_get_last_key(ZXIC_AVL_CFG*  p_avl_cfg, void *p_key_out);
ZXIC_RTN32 ic_comm_avl_get_1st_node(ZXIC_AVL_CFG*  p_avl_cfg, ZXIC_AVL_NODE** p_node_out);
ZXIC_RTN32 ic_comm_avl_get_last_node(ZXIC_AVL_CFG*  p_avl_cfg, ZXIC_AVL_NODE** p_node_out);

ZXIC_RTN32 zxic_comm_avl_show_info(ZXIC_AVL_CFG*  p_avl_cfg);

#endif/*__ZXIC_AVL_TREE_H__*/



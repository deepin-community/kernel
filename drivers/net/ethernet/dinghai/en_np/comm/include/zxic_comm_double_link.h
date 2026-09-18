/*****************************************************************************
 * 版权所有 (C)2008-2010, 深圳市中兴通讯股份有限公司。
 * 
 * 文件名称：    zxic_comm_double_link.h
 * 文件标识：    
 * 内容摘要：    双链表实现
 * 其它说明： 
 * 当前版本：    V1.00.10
 * 作           者：   
 * 完成日期：    2012年2月14日
 * 当前责任人-1：ChenWei188471
 * 当前责任人-2：
 * 历史责任人-3: 
 *
 * 修改记录：   
 * 修改日期：   版 本 号   修 改 人     修改内容 
 1    20120214               V1.00.10       ChenWei188471  创建
 2      
 *****************************************************************************/

#ifndef _ZXIC_COMM_DOUBLE_LINK_H
#define _ZXIC_COMM_DOUBLE_LINK_H
#define TEST_NUMBER (255)




/**************************************************************************
 *                        double_link api                                  *
 **************************************************************************/
typedef struct _d_node
{
    void *data;
    struct _d_node *prev;
    struct _d_node *next;
}D_NODE;

typedef struct _d_head
{
    ZXIC_UINT32  used;
    ZXIC_UINT32  maxnum;
    D_NODE *p_next;
    D_NODE *p_prev;
}D_HEAD;

typedef ZXIC_SINT32 (*CMP_FUNC)(D_NODE* data1,D_NODE* data2,void* );

typedef ZXIC_RTN32 (*fun_free)(void *);

ZXIC_RTN32  zxic_comm_double_link_insert_1st (D_NODE *newnode,  D_HEAD *head);
ZXIC_RTN32  zxic_comm_double_link_insert_aft (D_NODE *newnode,  D_NODE *oldnode,D_HEAD*head);
ZXIC_RTN32  zxic_comm_double_link_insert_pre (D_NODE *newnode,  D_NODE *oldnode,D_HEAD*head);
ZXIC_RTN32  zxic_comm_double_link_insert_last(D_NODE *newnode,  D_HEAD *head);
ZXIC_RTN32  zxic_comm_double_link_merge_list (D_HEAD *d_list,   D_HEAD *s_list);

ZXIC_RTN32  zxic_comm_double_link_insert_sort(D_NODE *newnode,  D_HEAD *head,  CMP_FUNC fuc,void*);

ZXIC_RTN32  zxic_comm_double_link_search     (D_NODE *data,     D_HEAD *head);
ZXIC_RTN32  zxic_comm_double_link_del        (D_NODE *data,     D_HEAD *head);
ZXIC_RTN32  zxic_comm_double_link_init       (ZXIC_UINT32 elmemtnum, D_HEAD *head);
ZXIC_RTN32  zxic_comm_double_link_insert_merge(D_NODE *p_newnode,D_HEAD *p_head,ZXIC_UINT32 is_head);

/*fun指向的是释放dnode指向的空间，如果没有，可以传NULL*/
ZXIC_RTN32  zxic_comm_dlink_release(D_HEAD *p_head,fun_free fun);
ZXIC_SINT32 zxic_comm_double_link_default_cmp_fuc(D_NODE* p_data1,D_NODE* p_data2,void*);

ZXIC_RTN32 zxic_comm_double_link_del_pos(D_HEAD *p_head,void* cmp_data,fun_free fun);
ZXIC_RTN32 zxic_comm_double_link_insert_cmp(D_HEAD *p_head, void* cmp_data, ZXIC_UINT32 *is_same);

#define INIT_D_NODE(ptr,pdata) \
    do{\
        (ptr)->data = pdata;\
        (ptr)->prev = NULL;\
        (ptr)->next = NULL;\
    }while(0)


/*add by lius
将0转 换成(TYPE*)，结构以内存空间首地址0作为起始地址，则成员地址为偏移地址；*/
#define MEM_OFF(type,member) \
    (ZXIC_COMM_PTR_TO_VAL(&(((type*)0)->member)))

/*根据当前双链表的指针，找到本节点的指针*/
#define STRUCT_ENTRY_POINT(ptr, type, member) \
    ((type *)(ZXIC_COMM_PTR_TO_VAL(ptr)-MEM_OFF(type,member)))

/* 不依据0，找结构体首地址 */
#define MEM_OFF_NOT_NULL(type,member) \
    (ZXIC_COMM_PTR_TO_VAL(&(((type*)4)->member)) - ZXIC_COMM_PTR_TO_VAL(((type*)4)))

/* 为了消除原STRUCT_ENTRY_POINT中“直接解引用 NULL”的coverity */
#define GET_STRUCT_ENTRY_POINT(ptr, type, member) \
    ((type *)(ZXIC_COMM_PTR_TO_VAL(ptr)-MEM_OFF_NOT_NULL(type,member)))

#define DLINK_IS_FULL(p_dlink) \
    ((p_dlink)->used == (p_dlink)->maxnum)


ZXIC_RTN32 zxic_comm_double_link_sort   (D_HEAD *p_head,  CMP_FUNC cmp_fuc);
ZXIC_RTN32 zxic_comm_double_link_swap   (D_NODE *p_pre,   D_NODE *p_next);
ZXIC_RTN32 zxic_comm_double_link_test   (ZXIC_VOID );
ZXIC_RTN32 zxic_comm_double_link_print  (D_HEAD *p_head);
ZXIC_RTN32 zxic_comm_double_link_del_by_data(D_HEAD *p_head,ZXIC_VOID* cmp_data,fun_free fun);
ZXIC_RTN32 zxic_comm_double_link_del_by_info(D_HEAD *p_head, void* cmp_data, CMP_FUNC cmp_fuc, ZXIC_UINT32 *p_deled_num);



#endif 


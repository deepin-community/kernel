/*****************************************************************************
 * 版权所有 (C)2008-2010, 深圳市中兴通讯股份有限公司。
 *
 * 文件名称：    zxic_comm_double_link.c
 * 文件标识：
 * 内容摘要：    双链表实现
 * 其它说明：
 * 当前版本：    V1.00.10
 * 作    者：
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

#include "zxic_common.h"
#include "zxic_comm_double_link.h"

/*当前节点都插在头节点之后,新插入的节点永远在第一个*/
ZXIC_RTN32 zxic_comm_double_link_insert_1st(D_NODE *p_newnode,D_HEAD *p_head)
{
    ZXIC_COMM_CHECK_POINT(p_newnode);
    ZXIC_COMM_CHECK_POINT(p_head);

    ZXIC_COMM_CHECK_INDEX((p_head->used+1), 1, p_head->maxnum);

    ZXIC_COMM_ASSERT(!(!p_head->p_next&& p_head->p_prev));
    ZXIC_COMM_ASSERT(!(p_head->p_next && !p_head->p_prev));

    p_newnode->next = p_head->p_next;
    p_newnode->prev = NULL;/*新节点前驱为NULL*/

    if(p_head->p_next)
    {
        p_head->p_next->prev = p_newnode;
    }
    else
    {
        p_head->p_prev = p_newnode;
    }

    p_head->p_next = p_newnode;
    p_head->used++;


    return ZXIC_OK;
}

/*目的是配合sort函数实现相同节点无需重复写入*/
ZXIC_RTN32 zxic_comm_double_link_insert_cmp(D_HEAD *p_head, void* cmp_data, ZXIC_UINT32 *is_same)
{
    D_NODE *p_dn = 0;

    *is_same = 0;

    p_dn = p_head->p_next;

    while(p_dn)
    {
        if(*(ZXIC_UINT32*)cmp_data == *(ZXIC_UINT32*)p_dn->data)
        {
            *is_same = 1;

            break;
        }

        p_dn = p_dn->next;
    }

    return ZXIC_OK;
}

ZXIC_RTN32 zxic_comm_double_link_insert_merge(D_NODE *p_newnode,D_HEAD *p_head,ZXIC_UINT32 is_head)
{
    D_NODE *p_dn = 0;
    ZXIC_UINT32  is_same = 0;

    p_dn = p_head->p_next;

    while(p_dn)
    {
        if(p_dn->data == p_newnode->data)
        {
            is_same = 1;
            break;
        }

        p_dn = p_dn->next;
    }

    if(! is_same )
    {
        if (is_head)
        {
            return zxic_comm_double_link_insert_1st(p_newnode, p_head);
        }
        else
        {
            return zxic_comm_double_link_insert_last(p_newnode, p_head);
        }
    }

    return ZXIC_OK;

}

/* 在OLD节点之后插入 */
ZXIC_RTN32  zxic_comm_double_link_insert_aft(D_NODE *p_newnode,D_NODE *p_oldnode,D_HEAD*p_head)
{


    ZXIC_COMM_CHECK_POINT(p_newnode);
    ZXIC_COMM_CHECK_POINT(p_oldnode);
    ZXIC_COMM_CHECK_POINT(p_head);

    ZXIC_COMM_CHECK_INDEX((p_head->used+1), 1, p_head->maxnum);

    ZXIC_COMM_ASSERT(!(!p_head->p_next&& p_head->p_prev));
    ZXIC_COMM_ASSERT(!(p_head->p_next && !p_head->p_prev));

    p_newnode->next = p_oldnode->next;
    p_newnode->prev = p_oldnode;

    if(p_oldnode->next)
    {
        p_oldnode->next->prev = p_newnode;
    }
    else
    {
        p_head->p_prev = p_newnode;
    }

    p_oldnode->next = p_newnode;
    p_head->used++;

    return ZXIC_OK;
}

/* 在OLD节点前插入 */
ZXIC_RTN32 zxic_comm_double_link_insert_pre(D_NODE *p_newnode,D_NODE *p_oldnode,D_HEAD*p_head)
{

    ZXIC_COMM_CHECK_POINT(p_newnode);
    ZXIC_COMM_CHECK_POINT(p_oldnode);
    ZXIC_COMM_CHECK_POINT(p_head);

    ZXIC_COMM_CHECK_INDEX((p_head->used+1), 1, p_head->maxnum);

    ZXIC_COMM_ASSERT(!(!p_head->p_next&& p_head->p_prev));
    ZXIC_COMM_ASSERT(!(p_head->p_next && !p_head->p_prev));

    p_newnode->next = p_oldnode;
    p_newnode->prev = p_oldnode->prev;

    if(p_oldnode->prev)
    {
        p_oldnode->prev->next = p_newnode;
    }
    else
    {
        p_head->p_next = p_newnode;
    }

    p_oldnode->prev = p_newnode;
    p_head->used++;

    return ZXIC_OK;
}
ZXIC_RTN32 zxic_comm_double_link_insert_last(D_NODE *p_newnode,D_HEAD* p_head)
{
    D_NODE *p_dnode = NULL;

    ZXIC_COMM_CHECK_POINT(p_newnode);
    ZXIC_COMM_CHECK_POINT(p_head);

    ZXIC_COMM_CHECK_INDEX((p_head->used+1), 1, p_head->maxnum);

    ZXIC_COMM_ASSERT(!(!p_head->p_next&& p_head->p_prev));
    ZXIC_COMM_ASSERT(!(p_head->p_next && !p_head->p_prev));


    p_dnode = p_head->p_prev;


    if(!p_dnode)
    {
        p_head->p_next  = p_newnode;
        p_head->p_prev  = p_newnode;
        p_newnode->next = NULL;
        p_newnode->prev = NULL;
    }
    else
    {
        p_newnode->prev = p_dnode;
        p_newnode->next = NULL;
        p_head->p_prev  = p_newnode;
        p_dnode->next   = p_newnode;
    }

    p_head->used++;

    return ZXIC_OK;
}
ZXIC_RTN32  zxic_comm_double_link_del(D_NODE *delnode,D_HEAD *p_head)
{
    D_NODE *next = NULL;
    D_NODE *pre  = NULL;

    ZXIC_COMM_CHECK_POINT(delnode);
    ZXIC_COMM_CHECK_POINT(p_head);

    ZXIC_COMM_CHECK_INDEX(p_head->used, 1, p_head->maxnum);

    next = delnode->next;
    pre  = delnode->prev;

    if(next)
    {
        next->prev = delnode->prev;
    }
    else
    {
        p_head->p_prev= delnode->prev;
    }

    if(pre)
    {
        pre->next = delnode->next;
    }
    else
    {
        p_head->p_next = delnode->next;
    }

    p_head->used--;
    delnode->next = NULL;
    delnode->prev = NULL;
    return ZXIC_OK;
}


ZXIC_RTN32 zxic_comm_double_link_init(ZXIC_UINT32 elmemtnum,D_HEAD *p_head)
{
    ZXIC_UINT32 err_code = 0;


    ZXIC_COMM_CHECK_POINT(p_head);

    if(elmemtnum == 0 )
    {
        err_code = ZXIC_DOUBLE_LINK_INIT_ELEMENT_NUM_ERR;
        ZXIC_COMM_TRACE_ERROR("\nError:[0x%x] zxic_doule_link_init Element Num Err !",err_code);
        return err_code;
    }

    p_head->maxnum   = elmemtnum;
    p_head->used     = 0;
    p_head->p_next   = NULL;
    p_head->p_prev   = NULL;

    return ZXIC_OK;
}

ZXIC_RTN32 zxic_comm_dlink_release(D_HEAD *p_head,fun_free fun)
{
    ZXIC_UINT32 rc = 0;
    D_NODE *p_node = NULL;

    ZXIC_COMM_CHECK_POINT(p_head);

    while(p_head->used)
    {
        p_node = p_head->p_next;

        if(NULL != fun)
        {
            rc = fun(p_node->data);
            ZXIC_COMM_CHECK_RC(rc, "fun");
        }

        rc = zxic_comm_double_link_del(p_node,p_head);
        ZXIC_COMM_CHECK_RC(rc, "zxic_comm_double_link_del");

        ZXIC_COMM_FREE(p_node);
    }

    return ZXIC_OK;
}
/*connetc the s_list to d_list */

ZXIC_RTN32 zxic_comm_double_link_merge_list(D_HEAD *d_list,D_HEAD *s_list)
{
    if(d_list->p_prev)
    {
        d_list->p_prev->next  = s_list->p_next;
    }
    else
    {
        ZXIC_COMM_ASSERT(!d_list->p_next);
        d_list->p_next = s_list->p_next;
    }

    if(s_list->p_next)
    {
        ZXIC_COMM_ASSERT(s_list->p_prev);
        s_list->p_next->prev = d_list->p_prev;
        d_list->p_prev       = s_list->p_prev;
    }

    d_list->used         += s_list->used;

    return ZXIC_OK;
}
/*********************************************************************
 * 函数名称：zxic_comm_double_link_insert_sort
 * 功能描述:
 * 函数功能简介
 *            插入排序，
 * 输入参数：
 * 输出参数：
 * 返 回 值：ZXIC_RTN32
 * 全局变量：
 * 注    释：
============================================================
 * 修改记录:
 * 修改日期      版本号      修改人   修改内容
============================================================
 * 2012-03-19             jiangwenming

 ******************************************************************/
ZXIC_RTN32 zxic_comm_double_link_insert_sort(D_NODE *p_newnode,D_HEAD *p_head,CMP_FUNC cmp_fuc,void* cmp_data)
{
    D_NODE*    pre_node = NULL;

    ZXIC_COMM_CHECK_POINT(p_head);
    ZXIC_COMM_CHECK_POINT(p_newnode);

    if(NULL == cmp_fuc )
    {
        cmp_fuc = zxic_comm_double_link_default_cmp_fuc;
    }

    ZXIC_COMM_CHECK_INDEX((p_head->used+1), 1, p_head->maxnum);

    /*此时表中的数据，已经排序了，再插入时，只需从表头开始比较，然后插入适当位置*/
    if( 0 == p_head->used )
    {
        return zxic_comm_double_link_insert_1st(p_newnode,p_head);
    }
    else
    {
        pre_node = p_head->p_next;

        while(NULL!=pre_node)
        {
            /*新节点的键值小于等于当前的键值*/
            if(cmp_fuc(p_newnode,pre_node,cmp_data) <= 0)
            {
                 return zxic_comm_double_link_insert_pre(p_newnode,pre_node,p_head);
            }
            else
            {
                pre_node = pre_node->next;
            }
        }

        /*循环结束后，说明插入的节点大于链表中的所有节点，需要在尾部插入*/
        return zxic_comm_double_link_insert_last(p_newnode,p_head);
    }
}


/*********************************************************************
 * 函数名称：zxic_comm_double_link_del_pos
 * 功能描述:
 * 函数功能简介
 *           根据node中指定的信息删除节点
 * 输入参数：
 * 输出参数：
 * 返 回 值：ZXIC_RTN32
 * 全局变量：
 * 注    释： 根据指定的信息，可能会删除多个
============================================================
 * 修改记录:
 * 修改日期      版本号      修改人   修改内容
============================================================
 * 2020-11-11            徐晨曦

 ******************************************************************/
ZXIC_RTN32 zxic_comm_double_link_del_by_info(D_HEAD *p_head, void* cmp_data, CMP_FUNC cmp_fuc, ZXIC_UINT32 *p_deled_num)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 is_same = 0;

    D_NODE *p;
    D_NODE *p_cur_node = ZXIC_NULL;

    if(NULL == cmp_fuc )
    {
        cmp_fuc = zxic_comm_double_link_default_cmp_fuc;
    }

    p = p_head->p_next;

    while(p)
    {
        /*新节点的键值等于当前的键值*/
        if(cmp_fuc(cmp_data, p,cmp_data) == 0)
        {
            rc = zxic_comm_double_link_del(p, p_head);
            ZXIC_COMM_CHECK_RC(rc, "zxic_comm_double_link_del");

            p_cur_node = p;
            p = p->next;

            ZXIC_COMM_FREE(p_cur_node);
            is_same ++;
            continue;
        }

        p = p->next;
    }

    if(0 == is_same)
    {
        return ZXIC_ERR;
    }

    *p_deled_num = is_same;

    ZXIC_COMM_TRACE_DEBUG(" DOUBLE LINK DEL NUM %d \n", is_same);

    return ZXIC_OK;
}

/*********************************************************************
 * 函数名称：zxic_comm_double_link_del_pos
 * 功能描述:
 * 函数功能简介
 *            删除指定位置的数据
 * 输入参数：
 * 输出参数：
 * 返 回 值：ZXIC_RTN32
 * 全局变量：
 * 注    释：
============================================================
 * 修改记录:
 * 修改日期      版本号      修改人   修改内容
============================================================
 * 2012-03-19             jiangwenming

 ******************************************************************/
ZXIC_RTN32 zxic_comm_double_link_del_pos(D_HEAD *p_head,void* cmp_data,fun_free fun)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 is_same = 0;

    D_NODE *p;

    p = p_head->p_next;

    while(p)
    {
        if(*(ZXIC_UINT32*)cmp_data == *(ZXIC_UINT32*)p->data)
        {
            is_same = 1;

            break;
        }

        p = p->next;
    }

    if(is_same)
    {
        rc = zxic_comm_double_link_del(p, p_head);
        ZXIC_COMM_CHECK_RC(rc, "zxic_comm_double_link_del");

        if(NULL != fun)
        {
            rc = fun(p->data);
            ZXIC_COMM_CHECK_RC(rc, "fun");
        }

        ZXIC_COMM_FREE(p);
    }

    return ZXIC_OK;
}

/*********************************************************************
 * 函数名称：zxic_comm_double_link_swap
 * 功能描述:
 * 函数功能简介
 *            交换链表中的两个节点
 * 输入参数：
 * 输出参数：
 * 返 回 值：ZXIC_RTN32
 * 全局变量：
 * 注    释：
============================================================
 * 修改记录:
 * 修改日期      版本号      修改人   修改内容
============================================================
 * 2012-03-19             jiangwenming

 ******************************************************************/
ZXIC_RTN32 zxic_comm_double_link_print(D_HEAD *p_head)
{

    D_NODE *p_pre   = NULL;
    D_NODE *p_next  = NULL;

    ZXIC_COMM_CHECK_POINT(p_head);
    /*正向打印*/
    p_next = p_head->p_next;
    ZXIC_COMM_PRINT("*************sequ order***********\n");

    while(p_next)
    {
        ZXIC_COMM_PRINT("==>%d",*(ZXIC_UINT32*)(p_next->data));
        p_next=p_next->next;
    }

    /*反向打印*/
    ZXIC_COMM_PRINT("\n\n*************reverve order***********\n");
    p_pre=p_head->p_prev;

    while(p_pre != NULL)
    {
        ZXIC_COMM_PRINT("==>%d",*(ZXIC_UINT32*)(p_pre->data));
        p_pre   = p_pre->prev;
    }

    return ZXIC_OK;

}

ZXIC_SINT32 zxic_comm_double_link_default_cmp_fuc(D_NODE* p_data1,D_NODE* p_data2,void* p_data)
{
    ZXIC_UINT32 data1= *(ZXIC_UINT32*)p_data1->data;
    ZXIC_UINT32 data2= *(ZXIC_UINT32*)p_data2->data;

    if(data1>data2)
    {
        return 1;
    }
    else if(data1==data2)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

ZXIC_RTN32 zxic_comm_double_link_del_by_data(D_HEAD *p_head,ZXIC_VOID* cmp_data,fun_free fun)
{
    ZXIC_UINT32 rc = 0;
    ZXIC_UINT32 is_same = 0;

    D_NODE *p;

    p = p_head->p_next;

    while(p)
    {
        if(cmp_data == p->data)
        {
            is_same = 1;

            break;
        }
        p = p->next;
    }

    if(is_same)
    {
        rc = zxic_comm_double_link_del(p,p_head);
        ZXIC_COMM_CHECK_RC(rc, "zxic_comm_double_link_del");
        if(NULL != fun)
        {
            rc = fun(p->data);
            ZXIC_COMM_CHECK_RC(rc, "fun");
        }

        ZXIC_COMM_FREE(p);
    }
    else
    {
        ZXIC_COMM_TRACE_ERROR("\nError:data not exist.\n");
    }

    return ZXIC_OK;
}





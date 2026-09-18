/*****************************************************************************
 * 版权所有 (C)2001-2015, 深圳市中兴通讯股份有限公司。
 *
 * 文件名称：   zxic_comm_rb_tree.c
 * 文件标识：
 * 内容摘要：
 * 其它说明：
 * 当前版本：
 * 作    者：    ChenWei10088471
 * 完成日期：
 * 当前责任人-1：
 * 当前责任人-2：
 *
 * DEPARTMENT       : ASIC_FPGA_R&D_Dept
 * MANUAL_PERCENT   : 100%
 *****************************************************************************/
#include "zxic_common.h"
#include "zxic_comm_rb_tree.h"
#include "zxic_comm_double_link.h"

ZXIC_SINT32 zxic_comm_rb_def_cmp(ZXIC_VOID *p_new, ZXIC_VOID *p_old, ZXIC_UINT32 key_size)
{
    return ZXIC_COMM_MEMCMP(p_new, p_old, key_size);
}

ZXIC_RTN32 zxic_comm_rb_init(ZXIC_RB_CFG *p_rb_cfg,
                        ZXIC_UINT32      total_num,
                        ZXIC_UINT32      key_size,
                        ZXIC_RB_CMPFUN cmpfun )
{
    ZXIC_UINT32      rtn  = ZXIC_OK;
    ZXIC_UINT32      malloc_size = 0;
    ZXIC_UINT32      memset_size = 0;

    ZXIC_COMM_CHECK_POINT(p_rb_cfg);
    if (p_rb_cfg->is_init)
    {
        ZXIC_COMM_TRACE_ERROR("\n zxic_comm_rb_init already init!");
        return ZXIC_OK;
    }

    p_rb_cfg->key_size =  key_size;
    p_rb_cfg->p_root   =  NULL;

    if (cmpfun)
    {
        p_rb_cfg->p_cmpfun =  cmpfun;
    }
    else
    {
        p_rb_cfg->p_cmpfun = zxic_comm_rb_def_cmp;
    }

    if (total_num)
    {
        p_rb_cfg->is_dynamic = 0;

        rtn = zxic_comm_double_link_init(total_num, &p_rb_cfg->tn_list);
        ZXIC_COMM_CHECK_RC(rtn, "zxic_comm_double_link_init");

        rtn = zxic_comm_liststack_creat(total_num, &p_rb_cfg->p_lsm);
        ZXIC_COMM_CHECK_RC(rtn, "zxic_comm_liststack_creat");

        p_rb_cfg->p_keybase = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(total_num * p_rb_cfg->key_size);
        ZXIC_COMM_CHECK_POINT(p_rb_cfg->p_keybase);
        memset_size = total_num * p_rb_cfg->key_size;
        ZXIC_COMM_MEMSET(p_rb_cfg->p_keybase, 0, memset_size);
        
        malloc_size = (ZXIC_SIZEOF(ZXIC_RB_TN) * total_num) & ZXIC_UINT32_MASK;

        p_rb_cfg->p_tnbase  = (ZXIC_RB_TN*)ZXIC_COMM_MALLOC(malloc_size);
        ZXIC_COMM_CHECK_POINT(p_rb_cfg->p_tnbase);
        ZXIC_COMM_MEMSET(p_rb_cfg->p_tnbase, 0, total_num * ZXIC_SIZEOF(ZXIC_RB_TN));
    }
    else /*totalnum = 0 indicate that customer manage the memory*/
    {
        p_rb_cfg->is_dynamic = 1;

        rtn = zxic_comm_double_link_init(0xFFFFFFFF, &p_rb_cfg->tn_list);
        ZXIC_COMM_CHECK_RC(rtn, "zxic_comm_double_link_init");
    }
    p_rb_cfg->is_init = 1;

    return ZXIC_OK;
}

ZXIC_RTN32 zxic_comm_rb_destroy(ZXIC_RB_CFG *p_rb_cfg)
{
    ZXIC_UINT32       rtn     = 0;

    ZXIC_COMM_CHECK_POINT(p_rb_cfg);
    /*静态才使用*/
    if (0 == p_rb_cfg->is_dynamic)
    {
        zxic_comm_liststack_destroy(p_rb_cfg->p_lsm);    
    }
    if (NULL != p_rb_cfg->p_keybase)
    {
        ZXIC_COMM_FREE(p_rb_cfg->p_keybase);
        p_rb_cfg->p_keybase = NULL;
    }

    if (NULL != p_rb_cfg->p_tnbase)
    {
        ZXIC_COMM_FREE(p_rb_cfg->p_tnbase);
        p_rb_cfg->p_tnbase = NULL;
    }

    ZXIC_COMM_MEMSET(p_rb_cfg, 0, ZXIC_SIZEOF(ZXIC_RB_CFG));

    return rtn;
}

ZXIC_VOID zxic_comm_rb_swich_color(ZXIC_RB_TN  *p_tn1, ZXIC_RB_TN *p_tn2)
{
    ZXIC_UINT32 color1, color2;

    ZXIC_COMM_CHECK_POINT_NONE(p_tn1);
    ZXIC_COMM_CHECK_POINT_NONE(p_tn2);

    color1 = GET_TN_COLOR(p_tn1);
    color2 = GET_TN_COLOR(p_tn2);

    SET_TN_COLOR(p_tn1, color2);
    SET_TN_COLOR(p_tn2, color1);

    return;
}


ZXIC_RB_TN* zxic_comm_rb_get_brotn(ZXIC_RB_TN  *p_cur_tn)
{
    ZXIC_COMM_CHECK_POINT_RETURN_NULL(p_cur_tn);
    ZXIC_COMM_CHECK_POINT_RETURN_NULL(p_cur_tn->p_parent);

    return (p_cur_tn->p_parent->p_left == p_cur_tn) ? p_cur_tn->p_parent->p_right : p_cur_tn->p_parent->p_left;
}


ZXIC_RTN32 zxic_comm_rb_handle_ins(ZXIC_RB_CFG *p_rb_cfg,
                              ZXIC_RB_TN  ***stack_tn,
                              ZXIC_UINT32      stack_top)
{
    ZXIC_RB_TN  **pp_cur_tn        = NULL;
    ZXIC_RB_TN  *p_cur_tn          = NULL;
    ZXIC_RB_TN  **pp_tmp_tn        = NULL;
    ZXIC_RB_TN  *p_tmp_tn          = NULL;

    ZXIC_COMM_CHECK_POINT(p_rb_cfg);
    ZXIC_COMM_CHECK_POINT(stack_tn);

    while (stack_top > 0)
    {
        pp_cur_tn = stack_tn[stack_top];
        p_cur_tn  = *pp_cur_tn;

        if (!p_cur_tn->p_parent) /*root must be black*/
        {
            SET_TN_COLOR(p_cur_tn, ZXIC_RBT_BLACK);
            break;
        }
        else if (GET_TN_COLOR(p_cur_tn->p_parent) == ZXIC_RBT_RED)
        {
            ZXIC_RB_TN *p_unc_tn = zxic_comm_rb_get_brotn(p_cur_tn->p_parent);

            ZXIC_COMM_ASSERT(p_cur_tn->p_parent == *stack_tn[stack_top - 1]);

            if (GET_TN_COLOR(p_unc_tn) == ZXIC_RBT_RED) /*unc is red,so we change the black of parent and unc*/
            {
                ZXIC_COMM_ASSERT(p_unc_tn);
                SET_TN_COLOR(p_cur_tn->p_parent, ZXIC_RBT_BLACK);
                SET_TN_COLOR(p_unc_tn, ZXIC_RBT_BLACK);

                ZXIC_COMM_ASSERT(p_cur_tn->p_parent->p_parent == *stack_tn[stack_top - 2]);

                SET_TN_COLOR(p_cur_tn->p_parent->p_parent, ZXIC_RBT_RED);
                stack_top -= 2;
            }
            else /*we need shift ,p_cur_tn->parent->parent*/
            {
                ZXIC_RB_TN *p_bro_tn = NULL;

                pp_tmp_tn = stack_tn[stack_top - 2];
                p_tmp_tn  = *pp_tmp_tn;


                if (p_cur_tn->p_parent == p_tmp_tn->p_left && p_cur_tn == p_cur_tn->p_parent->p_left)
                {
                    *pp_tmp_tn = p_cur_tn->p_parent;

                    p_bro_tn  = zxic_comm_rb_get_brotn(p_cur_tn);
                    p_cur_tn->p_parent->p_parent = p_tmp_tn->p_parent;

                    p_tmp_tn->p_left   = p_bro_tn;
                    p_tmp_tn->p_parent = p_cur_tn->p_parent;
                    p_cur_tn->p_parent->p_right = p_tmp_tn;

                    if (p_bro_tn)
                    {
                        p_bro_tn->p_parent  = p_tmp_tn;
                    }

                    zxic_comm_rb_swich_color(*pp_tmp_tn, p_tmp_tn);
                }
                else if (p_cur_tn->p_parent == p_tmp_tn->p_left && p_cur_tn == p_cur_tn->p_parent->p_right)
                {
                    *pp_tmp_tn = p_cur_tn;

                    p_cur_tn->p_parent->p_right = p_cur_tn->p_left;

                    if (p_cur_tn->p_left)
                    {
                        p_cur_tn->p_left->p_parent = p_cur_tn->p_parent;
                    }

                    p_cur_tn->p_parent->p_parent = p_cur_tn;
                    p_tmp_tn->p_left = p_cur_tn->p_right;

                    if (p_cur_tn->p_right)
                    {
                        p_cur_tn->p_right->p_parent = p_tmp_tn;
                    }

                    p_cur_tn->p_left = p_cur_tn->p_parent;
                    p_cur_tn->p_right = p_tmp_tn;

                    p_cur_tn->p_parent = p_tmp_tn->p_parent;
                    p_tmp_tn->p_parent = p_cur_tn;

                    zxic_comm_rb_swich_color(*pp_tmp_tn, p_tmp_tn);
                }
                else if (p_cur_tn->p_parent == p_tmp_tn->p_right && p_cur_tn == p_cur_tn->p_parent->p_right)
                {
                    *pp_tmp_tn = p_cur_tn->p_parent;
                    p_bro_tn  = zxic_comm_rb_get_brotn(p_cur_tn);

                    p_cur_tn->p_parent->p_parent = p_tmp_tn->p_parent;

                    p_tmp_tn->p_right = p_cur_tn->p_parent->p_left;
                    p_tmp_tn->p_parent = p_cur_tn->p_parent;
                    p_cur_tn->p_parent->p_left = p_tmp_tn;

                    if (p_bro_tn)
                    {
                        p_bro_tn->p_parent  = p_tmp_tn;
                    }

                    zxic_comm_rb_swich_color(*pp_tmp_tn, p_tmp_tn);
                }
                else
                {
                    *pp_tmp_tn = p_cur_tn;
                    p_cur_tn->p_parent->p_left = p_cur_tn->p_right;

                    if (p_cur_tn->p_right)
                    {
                        p_cur_tn->p_right->p_parent = p_cur_tn->p_parent;
                    }

                    p_cur_tn->p_parent->p_parent = p_cur_tn;
                    p_tmp_tn->p_right = p_cur_tn->p_left;

                    if (p_cur_tn->p_left)
                    {
                        p_cur_tn->p_left->p_parent = p_tmp_tn;
                    }

                    p_cur_tn->p_right = p_cur_tn->p_parent;
                    p_cur_tn->p_left = p_tmp_tn;

                    p_cur_tn->p_parent = p_tmp_tn->p_parent;
                    p_tmp_tn->p_parent = p_cur_tn;

                    zxic_comm_rb_swich_color(*pp_tmp_tn, p_tmp_tn);

                }

                /*change color*/

                /* SET_TN_COLOR(p_cur_tn->p_parent,ZXIC_RBT_BLACK);
                SET_TN_COLOR(p_tmp_tn,ZXIC_RBT_RED); */
                break;
            }
        }
        else /*parent is black ,nothing to do ,end*/
        {
            break;
        }
    }

    return ZXIC_OK;
}


ZXIC_RTN32 zxic_comm_rb_insert(ZXIC_RB_CFG *p_rb_cfg,
                         ZXIC_VOID       *p_key,
                         ZXIC_VOID       *out_val)
{
    ZXIC_UINT32     rtn            = 0;
    ZXIC_UINT32     stack_top      = 1;
    ZXIC_SINT32    cmprtn         = 0;
    ZXIC_UINT32     lsm_out        = 0;

    ZXIC_RB_TN  **stack_tn[ZXIC_RBT_MAX_DEPTH] = {0};
    //ZXIC_RB_TN  **pp_tmp_tn     = NULL;
    ZXIC_RB_TN  *p_cur_tn      = NULL;
    ZXIC_RB_TN  *p_pre_tn      = NULL;
    ZXIC_RB_TN **pp_cur_tn     = NULL;
    ZXIC_VOID       *p_cur_key     = NULL;
    ZXIC_RB_TN  *p_ins_tn      = p_key;

    ZXIC_COMM_CHECK_POINT(p_rb_cfg);
    ZXIC_COMM_CHECK_POINT(p_key);

    p_cur_key = p_rb_cfg->is_dynamic ? ((ZXIC_RB_TN*)p_key)->p_key : p_key;

    pp_cur_tn = &p_rb_cfg->p_root;

    for (;;)
    {
        p_cur_tn = *pp_cur_tn;

        if (!p_cur_tn) /*find the insert position*/
        {
            if (p_rb_cfg->is_dynamic == 0)
            {
                rtn = zxic_comm_liststack_alloc(p_rb_cfg->p_lsm, &lsm_out);

                if (rtn == ZXIC_LIST_STACK_ISEMPTY_ERR)
                {
                    return ZXIC_RBT_RC_FULL;
                }

                ZXIC_COMM_CHECK_RC(rtn, "zxic_comm_liststack_alloc");

                p_ins_tn = p_rb_cfg->p_tnbase + lsm_out;

                ZXIC_COMM_CHECK_INDEX_MUL_OVERFLOW( p_rb_cfg->key_size, lsm_out);
                INIT_RBT_TN(p_ins_tn, p_rb_cfg->key_size * lsm_out + p_rb_cfg->p_keybase);

                ZXIC_COMM_MEMCPY_S(p_ins_tn->p_key, p_rb_cfg->key_size, p_key, p_rb_cfg->key_size);

                SET_TN_LSV(p_ins_tn, lsm_out);

                if (out_val)
                {
                    *((ZXIC_UINT32*)out_val) = lsm_out;
                }
            }
            else
            {
                INIT_D_NODE(&p_ins_tn->tn_ln, p_ins_tn);
            }

            /*all insert tn color set to red*/
            SET_TN_COLOR(p_ins_tn, ZXIC_RBT_RED);

            /*insert list*/
            if (cmprtn < 0)
            {
                rtn = zxic_comm_double_link_insert_pre(&p_ins_tn->tn_ln, &p_pre_tn->tn_ln, &p_rb_cfg->tn_list);
                ZXIC_COMM_CHECK_RC(rtn, "zxic_comm_double_link_insert_pre");
            }
            else if (cmprtn > 0)
            {
                rtn = zxic_comm_double_link_insert_aft(&p_ins_tn->tn_ln, &p_pre_tn->tn_ln, &p_rb_cfg->tn_list);
                ZXIC_COMM_CHECK_RC(rtn, "zxic_comm_double_link_insert_aft");
            }
            else
            {
                /*first insert*/
                ZXIC_COMM_ASSERT(!p_pre_tn);

                rtn = zxic_comm_double_link_insert_1st(&p_ins_tn->tn_ln, &p_rb_cfg->tn_list);
                ZXIC_COMM_CHECK_RC(rtn, "zxic_comm_double_link_insert_1st");
            }

            /*get out loop */
            break;
        }

        ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW(stack_top, 1);
        stack_tn[stack_top++] =  pp_cur_tn;
        p_pre_tn = p_cur_tn;
        cmprtn = p_rb_cfg->p_cmpfun(p_cur_key, p_cur_tn->p_key, p_rb_cfg->key_size);

        if (cmprtn > 0)
        {
            pp_cur_tn = &p_cur_tn->p_right;
        }
        else if (cmprtn < 0)
        {
            pp_cur_tn = &p_cur_tn->p_left;
        }
        else
        {
            ZXIC_COMM_TRACE_ALL("info ,rb_key is same \n");

            if (p_rb_cfg->is_dynamic)
            {
                if (out_val)
                {
                    *((ZXIC_RB_TN**)out_val) = p_cur_tn;
                }
            }
            else
            {
                if (out_val)
                {
                    *((ZXIC_UINT32*)out_val) = GET_TN_LSV(p_cur_tn);
                }
            }

            return ZXIC_RBT_RC_UPDATE;
        }
    }

    /*handle parenet ptr*/
    //pp_tmp_tn  = stack_tn[stack_top - 1];

    /*p_ins_tn->p_parent = stack_top != 1 ? *pp_tmp_tn : NULL;*/
    p_ins_tn->p_parent = (stack_top != 1) ? *stack_tn[stack_top - 1] : NULL;

    stack_tn[stack_top] = pp_cur_tn;

    *pp_cur_tn = p_ins_tn;

    rtn = zxic_comm_rb_handle_ins(p_rb_cfg, stack_tn, stack_top);
    ZXIC_COMM_CHECK_RC(rtn, "zxic_comm_rb_handle_ins");

    /* 新插入时也返回 */
    if (p_rb_cfg->is_dynamic)
    {
        if (out_val)
        {
            *((ZXIC_RB_TN**)out_val) = p_ins_tn;
        }
    }

    return ZXIC_OK;
}



ZXIC_RTN32 zxic_comm_rb_handle_del(ZXIC_RB_CFG *p_rb_cfg,
                             ZXIC_RB_TN***stack_tn,
                             ZXIC_UINT32   stack_top)
{
    ZXIC_RB_TN  **pp_cur_tn        = NULL;
    ZXIC_RB_TN  *p_cur_tn          = NULL;
    ZXIC_RB_TN  *p_tmp_tn          = NULL;
    ZXIC_RB_TN  *p_unc_tn          = NULL;
    ZXIC_RB_TN  *p_par_tn          = NULL;

    ZXIC_COMM_CHECK_POINT(p_rb_cfg);
    ZXIC_COMM_CHECK_POINT(stack_tn);

    while (stack_top > 1 )
    {
        pp_cur_tn =  stack_tn[stack_top];
        p_cur_tn  = *pp_cur_tn;

        p_par_tn  = *stack_tn[stack_top - 1];

        if (p_cur_tn && p_cur_tn->p_parent)
        {
            p_unc_tn  = zxic_comm_rb_get_brotn(p_cur_tn);
        }
        else if (p_cur_tn && !p_cur_tn->p_parent)
        {
            ZXIC_COMM_ASSERT(p_par_tn == p_cur_tn->p_parent);

            SET_TN_COLOR(p_cur_tn, ZXIC_RBT_BLACK);

            break;
        }
        else
        {
            ZXIC_COMM_ASSERT(!p_cur_tn);

            if (p_par_tn)
            {
                p_unc_tn = p_par_tn->p_left ? p_par_tn->p_left : p_par_tn->p_right;
            }
            else
            {
                break;
            }
        }

        if (p_unc_tn)
        {
            ZXIC_COMM_ASSERT(p_unc_tn->p_parent == p_par_tn);
        }

        if (GET_TN_COLOR(p_unc_tn) == ZXIC_RBT_RED) /*shift */
        {
            ZXIC_COMM_CHECK_INDEX_BOTH(stack_top, 1, (ZXIC_RBT_MAX_DEPTH - 2));
            if (p_unc_tn == p_par_tn->p_left) /*shift right */
            {
                *stack_tn[stack_top - 1] = p_unc_tn;
                p_unc_tn->p_parent = p_par_tn->p_parent;
                p_par_tn->p_left = p_unc_tn->p_right;

                if (p_unc_tn->p_right)
                {
                    p_unc_tn->p_right->p_parent = p_par_tn;
                }

                p_par_tn->p_parent = p_unc_tn;
                p_unc_tn->p_right = p_par_tn;

                stack_tn[stack_top++] = &p_unc_tn->p_right;
                ZXIC_COMM_CHECK_INDEX_UPPER(stack_top, (ZXIC_RBT_MAX_DEPTH - 1));
                stack_tn[stack_top]   = &p_par_tn->p_right;
            }
            else /*shift left*/
            {
                ZXIC_COMM_ASSERT(p_unc_tn == p_par_tn->p_right);
                *stack_tn[stack_top - 1] = p_unc_tn;
                p_unc_tn->p_parent = p_par_tn->p_parent;
                p_par_tn->p_right = p_unc_tn->p_left;

                if (p_unc_tn->p_left)
                {
                    p_unc_tn->p_left->p_parent = p_par_tn;
                }

                p_par_tn->p_parent = p_unc_tn;
                p_unc_tn->p_left  = p_par_tn;

                stack_tn[stack_top++] = &p_unc_tn->p_left;
                ZXIC_COMM_CHECK_INDEX_UPPER(stack_top, (ZXIC_RBT_MAX_DEPTH - 1));
                stack_tn[stack_top]   = &p_par_tn->p_left;
            }

            zxic_comm_rb_swich_color(p_unc_tn, p_par_tn);
        }
        else if (!p_unc_tn)
        {
            /*this branch will never run ,consider too much*/
            ZXIC_COMM_ASSERT(0);
            ZXIC_COMM_ASSERT(GET_TN_COLOR(p_par_tn) ==  ZXIC_RBT_RED);

            SET_TN_COLOR(p_par_tn, ZXIC_RBT_BLACK);

            break;
        }
        else
        {
            if (GET_TN_COLOR(p_unc_tn->p_left) == ZXIC_RBT_BLACK && GET_TN_COLOR(p_unc_tn->p_right) == ZXIC_RBT_BLACK)
            {
                if (GET_TN_COLOR(p_unc_tn->p_parent) == ZXIC_RBT_BLACK)
                {
                    SET_TN_COLOR(p_unc_tn, ZXIC_RBT_RED);
                    stack_top--;
                }
                else
                {
                    ZXIC_COMM_ASSERT(GET_TN_COLOR(p_unc_tn->p_parent) == ZXIC_RBT_RED);

                    zxic_comm_rb_swich_color(p_unc_tn->p_parent, p_unc_tn);

                    break;
                }
            }
            else if (p_unc_tn == p_par_tn->p_right)
            {
                if (GET_TN_COLOR(p_unc_tn->p_right) == ZXIC_RBT_RED) /*shift left*/
                {
                    *stack_tn[stack_top - 1] = p_unc_tn;
                    p_unc_tn->p_parent = p_par_tn->p_parent;
                    p_par_tn->p_right = p_unc_tn->p_left;

                    if (p_unc_tn->p_left)
                    {
                        p_unc_tn->p_left->p_parent = p_par_tn;
                    }

                    p_par_tn->p_parent = p_unc_tn;
                    p_unc_tn->p_left  = p_par_tn;

                    zxic_comm_rb_swich_color(p_unc_tn, p_par_tn);

                    SET_TN_COLOR(p_unc_tn->p_right, ZXIC_RBT_BLACK);

                    break;
                }
                else
                {
                    ZXIC_COMM_ASSERT(GET_TN_COLOR(p_unc_tn->p_left) == ZXIC_RBT_RED);

                    p_tmp_tn = p_unc_tn->p_left;

                    p_par_tn->p_right  = p_tmp_tn;
                    p_tmp_tn->p_parent = p_par_tn;
                    p_unc_tn->p_left  = p_tmp_tn->p_right;

                    if (p_tmp_tn->p_right)
                    {
                        p_tmp_tn->p_right->p_parent = p_unc_tn;
                    }

                    p_tmp_tn->p_right = p_unc_tn;
                    p_unc_tn->p_parent = p_tmp_tn;

                    zxic_comm_rb_swich_color(p_tmp_tn, p_unc_tn);
                }
            }
            else
            {
                ZXIC_COMM_ASSERT(p_unc_tn == p_par_tn->p_left);

                if (GET_TN_COLOR(p_unc_tn->p_left) == ZXIC_RBT_RED) /*shift right*/
                {
                    *stack_tn[stack_top - 1] = p_unc_tn;
                    p_unc_tn->p_parent = p_par_tn->p_parent;
                    p_par_tn->p_left  = p_unc_tn->p_right;

                    if (p_unc_tn->p_right)
                    {
                        p_unc_tn->p_right->p_parent = p_par_tn;
                    }

                    p_par_tn->p_parent = p_unc_tn;
                    p_unc_tn->p_right = p_par_tn;

                    zxic_comm_rb_swich_color(p_unc_tn, p_par_tn);

                    SET_TN_COLOR(p_unc_tn->p_left, ZXIC_RBT_BLACK);
                    break;
                }
                else
                {
                    ZXIC_COMM_ASSERT(GET_TN_COLOR(p_unc_tn->p_right) == ZXIC_RBT_RED);

                    p_tmp_tn = p_unc_tn->p_right;

                    p_par_tn->p_left  = p_tmp_tn;
                    p_tmp_tn->p_parent = p_par_tn;
                    p_unc_tn->p_right  = p_tmp_tn->p_left;

                    if (p_tmp_tn->p_left)
                    {
                        p_tmp_tn->p_left->p_parent = p_unc_tn;
                    }

                    p_tmp_tn->p_left = p_unc_tn;
                    p_unc_tn->p_parent = p_tmp_tn;

                    zxic_comm_rb_swich_color(p_tmp_tn, p_unc_tn);
                }
            }
        }
    }

    return ZXIC_OK;
}

ZXIC_RTN32 zxic_comm_rb_delete(ZXIC_RB_CFG *p_rb_cfg,
                         ZXIC_VOID       *p_key,
                         ZXIC_VOID       *out_val)
{
    ZXIC_UINT32     rtn            = 0;
    ZXIC_UINT32     stack_top      = 1;
    ZXIC_SINT32    cmprtn         = 0;
    ZXIC_UINT32     rsv_stack      = 0;
    ZXIC_UINT32     del_is_red     = 0;

    ZXIC_RB_TN  **stack_tn[ZXIC_RBT_MAX_DEPTH] = {0};
    ZXIC_RB_TN  *p_cur_tn      = NULL;
    ZXIC_RB_TN **pp_cur_tn     = NULL;
    ZXIC_VOID       *p_cur_key     = NULL;
    ZXIC_RB_TN  *p_rsv_tn      = NULL;
    ZXIC_RB_TN  *p_del_tn      = NULL;

    ZXIC_COMM_CHECK_POINT(p_rb_cfg);
    ZXIC_COMM_CHECK_POINT(out_val);

    p_cur_key = p_key;

    pp_cur_tn = &p_rb_cfg->p_root;

    for (;;)
    {
        p_cur_tn = *pp_cur_tn;

        if (!p_cur_tn)
        {
            /*ZXIC_COMM_TRACE_ERROR("\n error ,the key is not exist !");*/
            return ZXIC_RBT_RC_SRHFAIL;
        }
        ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW(stack_top, 1);
        stack_tn[stack_top++] = pp_cur_tn;

        cmprtn = p_rb_cfg->p_cmpfun(p_cur_key, p_cur_tn->p_key, p_rb_cfg->key_size);

        if (cmprtn > 0)
        {
            pp_cur_tn = &p_cur_tn->p_right;
        }
        else if (cmprtn < 0)
        {
            pp_cur_tn = &p_cur_tn->p_left;
        }
        else
        {
            ZXIC_COMM_TRACE_ALL(" find the key!\n");

            break;
        }
    }
    ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW(stack_top, 1);
    rsv_stack =  stack_top - 1; /*save stack pos*/
    p_rsv_tn  =  p_cur_tn;

    pp_cur_tn = &p_cur_tn->p_right;
    p_cur_tn  = *pp_cur_tn;

    if (p_cur_tn)
    {
    
        ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW(stack_top, 1);
        stack_tn[stack_top++] = pp_cur_tn;

        pp_cur_tn = &p_cur_tn->p_left;
        p_cur_tn  = *pp_cur_tn;

        while (p_cur_tn)
        {
            ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW(stack_top, 1);        
            stack_tn[stack_top++] = pp_cur_tn;
            pp_cur_tn = &p_cur_tn->p_left;
            p_cur_tn  = *pp_cur_tn;
        }

        /*get the del tn*/
        ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW(stack_top, 1);        
        p_del_tn = *stack_tn[stack_top - 1];

        /*set tn is left child to cur place*/
        *stack_tn[stack_top - 1] = p_del_tn->p_right;

        if (p_del_tn->p_right)
        {
            p_del_tn->p_right->p_parent =  p_del_tn->p_parent;
        }

        /*rsv the del tn info for delete*/
        if (GET_TN_COLOR(p_del_tn) == ZXIC_RBT_RED)
        {
            del_is_red = 1;
        }

        /*replace the delete val*/
        ZXIC_COMM_CHECK_INDEX_UPPER(rsv_stack, (ZXIC_RBT_MAX_DEPTH - 2));
        *stack_tn[rsv_stack]   = p_del_tn;

        stack_tn[rsv_stack + 1]  = &p_del_tn->p_right;

        SET_TN_COLOR(p_del_tn, GET_TN_COLOR(p_rsv_tn));
        p_del_tn->p_parent = p_rsv_tn->p_parent;

        p_del_tn->p_left   = p_rsv_tn->p_left;

        if (p_rsv_tn->p_left)
        {
            p_rsv_tn->p_left->p_parent = p_del_tn;
        }

        p_del_tn->p_right  = p_rsv_tn->p_right;

        if (p_rsv_tn->p_right)
        {
            p_rsv_tn->p_right->p_parent = p_del_tn;
        }
    }
    else
    {
        if (GET_TN_COLOR(p_rsv_tn) == ZXIC_RBT_RED)
        {
            del_is_red = 1;
        }
        
        ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW(stack_top, 1); 
        *stack_tn[stack_top - 1] = p_rsv_tn->p_left;

        if (p_rsv_tn->p_left)
        {
            p_rsv_tn->p_left->p_parent = p_rsv_tn->p_parent;
        }
    }

    ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW(stack_top, 1);
    stack_top--;
    ZXIC_COMM_CHECK_INDEX_UPPER(stack_top, (ZXIC_RBT_MAX_DEPTH - 1));
    if (GET_TN_COLOR(*stack_tn[stack_top]) == ZXIC_RBT_RED)
    {
        SET_TN_COLOR(*stack_tn[stack_top], ZXIC_RBT_BLACK);
    }
    else if (!del_is_red) /*del node is red ,do nothing*/
    {
        rtn = zxic_comm_rb_handle_del(p_rb_cfg, stack_tn, stack_top);
        ZXIC_COMM_CHECK_RC(rtn, "zxic_comm_rb_handle_del");
    }

    /*clear the node from the list */
    rtn = zxic_comm_double_link_del(&p_rsv_tn->tn_ln, &p_rb_cfg->tn_list);
    ZXIC_COMM_CHECK_RC(rtn, "zxic_comm_double_link_del");

    if (p_rb_cfg->is_dynamic)
    {
        *(ZXIC_RB_TN**)out_val = p_rsv_tn;
    }
    else
    {
        rtn = zxic_comm_liststack_free(p_rb_cfg->p_lsm, GET_TN_LSV(p_rsv_tn));
        ZXIC_COMM_CHECK_RC(rtn, "zxic_comm_liststack_free");

        *(ZXIC_UINT32*)out_val = GET_TN_LSV(p_rsv_tn);

        ZXIC_COMM_MEMSET(p_rsv_tn->p_key, 0, p_rb_cfg->key_size);
        ZXIC_COMM_MEMSET(p_rsv_tn, 0, ZXIC_SIZEOF(ZXIC_RB_TN));
    }

    return ZXIC_OK;
}

/* 注意此处传入的p_key和insert时候传入的p_key不是同一个东西 */
ZXIC_RTN32 zxic_comm_rb_search(ZXIC_RB_CFG *p_rb_cfg,
                         ZXIC_VOID       *p_key,
                         ZXIC_VOID       *out_val)
{
    ZXIC_SINT32    cmprtn   = 0;
    ZXIC_RB_TN *p_cur_tn = NULL;

    ZXIC_COMM_CHECK_POINT(p_rb_cfg);
    ZXIC_COMM_CHECK_POINT(p_key);
    ZXIC_COMM_CHECK_POINT(out_val);

    p_cur_tn = p_rb_cfg->p_root;

    while (p_cur_tn)
    {
        cmprtn = p_rb_cfg->p_cmpfun(p_key, p_cur_tn->p_key, p_rb_cfg->key_size);

        if (cmprtn > 0)
        {
            p_cur_tn = p_cur_tn->p_right;
        }
        else if (cmprtn < 0)
        {
            p_cur_tn = p_cur_tn->p_left;
        }
        else
        {
            break;
        }
    }

    if (!p_cur_tn)
    {
        ZXIC_COMM_TRACE_ALL("rb srh fail \n");
        return ZXIC_RBT_RC_SRHFAIL;
    }

    if (p_rb_cfg->is_dynamic)
    {
        *(ZXIC_RB_TN**)out_val = p_cur_tn;
    }
    else
    {
        *(ZXIC_UINT32*)out_val = GET_TN_LSV(p_cur_tn);
    }

    return ZXIC_OK;
}

ZXIC_SINT32 zxic_comm_rb_is_none(ZXIC_RB_CFG*  p_rb_cfg)
{
    ZXIC_COMM_CHECK_POINT(p_rb_cfg);

    if (0 == p_rb_cfg->tn_list.used)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

ZXIC_RB_TN *zxic_comm_rb_get_1st_tn(ZXIC_RB_CFG *p_rb_cfg)
{
    ZXIC_COMM_CHECK_POINT_RETURN_NULL(p_rb_cfg);

    return (p_rb_cfg->p_root) ? p_rb_cfg->tn_list.p_next->data : NULL;
}

ZXIC_RTN32 zxic_comm_rb_get_1st_key(ZXIC_RB_CFG* p_rb_cfg, ZXIC_VOID *p_key_out)
{
    D_NODE    *rb_list_node  = NULL;
    ZXIC_RB_TN *p_rb_node     = NULL;

    ZXIC_COMM_CHECK_POINT(p_rb_cfg);
    ZXIC_COMM_CHECK_POINT(p_key_out);

    if (zxic_comm_rb_is_none(p_rb_cfg))
    {
        return ZXIC_RBT_ISEMPTY_ERR;
    }

    rb_list_node = p_rb_cfg->tn_list.p_next;
    ZXIC_COMM_CHECK_POINT(rb_list_node);

    p_rb_node = (ZXIC_RB_TN *)rb_list_node->data;
    ZXIC_COMM_CHECK_POINT(p_rb_node);

    ZXIC_COMM_MEMCPY_S(p_key_out, p_rb_cfg->key_size, p_rb_node->p_key, p_rb_cfg->key_size);

    return ZXIC_OK;
}

ZXIC_RB_TN *zxic_comm_rb_get_last_tn(ZXIC_RB_CFG *p_rb_cfg)
{
    ZXIC_COMM_CHECK_POINT_RETURN_NULL(p_rb_cfg);

    return (p_rb_cfg->p_root) ? p_rb_cfg->tn_list.p_prev->data : NULL;
}

ZXIC_RTN32 zxic_comm_rb_get_last_key(ZXIC_RB_CFG*  p_rb_cfg, ZXIC_VOID *p_key_out)
{
    D_NODE        *p_rb_list_node      = NULL;
    ZXIC_RB_TN     *p_rb_node           = NULL;

    ZXIC_COMM_CHECK_POINT(p_rb_cfg);

    if (zxic_comm_rb_is_none(p_rb_cfg))
    {
        return ZXIC_RBT_ISEMPTY_ERR;
    }

    p_rb_list_node = p_rb_cfg->tn_list.p_prev;

    p_rb_node = (ZXIC_RB_TN *)p_rb_list_node->data;
    ZXIC_COMM_MEMCPY_S(p_key_out, p_rb_cfg->key_size, p_rb_node->p_key, p_rb_cfg->key_size);

    return ZXIC_OK;
}

/* 此函数仅用于CPU软复位 */
ZXIC_RTN32 zxic_comm_rb_insert_spec_index(ZXIC_RB_CFG *p_rb_cfg, ZXIC_VOID *p_key, ZXIC_UINT32 in_idx)
{
    ZXIC_UINT32     rtn            = 0;
    ZXIC_UINT32     stack_top      = 1;
    ZXIC_SINT32    cmprtn         = 0;

    ZXIC_RB_TN  **stack_tn[ZXIC_RBT_MAX_DEPTH] = {0};
    //ZXIC_RB_TN  **pp_tmp_tn     = NULL;
    ZXIC_RB_TN  *p_cur_tn      = NULL;
    ZXIC_RB_TN  *p_pre_tn      = NULL;
    ZXIC_RB_TN **pp_cur_tn     = NULL;
    ZXIC_VOID       *p_cur_key     = NULL;
    ZXIC_RB_TN  *p_ins_tn      = p_key;

    ZXIC_COMM_CHECK_POINT(p_rb_cfg);
    ZXIC_COMM_CHECK_POINT(p_key);

    if (p_rb_cfg->is_dynamic)
    {
        ZXIC_COMM_PRINT("zxic_comm_rb_insert_spec_index: dynamic mode is not support ! Error");
        return ZXIC_RBT_PARA_INVALID;
    }

    p_cur_key = p_key;

    pp_cur_tn = &p_rb_cfg->p_root;

    for (;;)
    {
        p_cur_tn = *pp_cur_tn;

        if (!p_cur_tn) /*find the insert position*/
        {
            rtn = zxic_comm_liststack_alloc_spec_index(p_rb_cfg->p_lsm, in_idx);
            ZXIC_COMM_CHECK_RC(rtn, "zxic_comm_liststack_alloc_spec_index");

            p_ins_tn = p_rb_cfg->p_tnbase + in_idx;

            ZXIC_COMM_CHECK_INDEX_MUL_OVERFLOW(p_rb_cfg->key_size, in_idx);
            INIT_RBT_TN(p_ins_tn, p_rb_cfg->key_size * in_idx + p_rb_cfg->p_keybase);

            ZXIC_COMM_MEMCPY_S(p_ins_tn->p_key, p_rb_cfg->key_size, p_key, p_rb_cfg->key_size);

            SET_TN_LSV(p_ins_tn, in_idx);

            /*all insert tn color set to red*/
            SET_TN_COLOR(p_ins_tn, ZXIC_RBT_RED);

            /*insert list*/
            if (cmprtn < 0)
            {
                rtn = zxic_comm_double_link_insert_pre(&p_ins_tn->tn_ln, &p_pre_tn->tn_ln, &p_rb_cfg->tn_list);
                ZXIC_COMM_CHECK_RC(rtn, "zxic_comm_double_link_insert_pre");
            }
            else if (cmprtn > 0)
            {
                rtn = zxic_comm_double_link_insert_aft(&p_ins_tn->tn_ln, &p_pre_tn->tn_ln, &p_rb_cfg->tn_list);
                ZXIC_COMM_CHECK_RC(rtn, "zxic_comm_double_link_insert_aft");
            }
            else
            {
                /*first insert*/
                ZXIC_COMM_ASSERT(!p_pre_tn);

                rtn = zxic_comm_double_link_insert_1st(&p_ins_tn->tn_ln, &p_rb_cfg->tn_list);
                ZXIC_COMM_CHECK_RC(rtn, "zxic_comm_double_link_insert_1st");
            }

            /*get out loop */
            break;
        }

        
        ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW(stack_top, 1);
        stack_tn[stack_top++] =  pp_cur_tn;
        p_pre_tn = p_cur_tn;
        cmprtn = p_rb_cfg->p_cmpfun(p_cur_key, p_cur_tn->p_key, p_rb_cfg->key_size);

        if (cmprtn > 0)
        {
            pp_cur_tn = &p_cur_tn->p_right;
        }
        else if (cmprtn < 0)
        {
            pp_cur_tn = &p_cur_tn->p_left;
        }
        else
        {
            ZXIC_COMM_TRACE_ALL("info ,rb_key is same \n");

            return ZXIC_RBT_RC_UPDATE;
        }
    }

    /*handle parenet ptr*/
    p_ins_tn->p_parent = (stack_top != 1) ? *stack_tn[stack_top - 1] : NULL;

    stack_tn[stack_top] = pp_cur_tn;

    *pp_cur_tn = p_ins_tn;

    rtn = zxic_comm_rb_handle_ins(p_rb_cfg, stack_tn, stack_top);
    ZXIC_COMM_CHECK_RC(rtn, "zxic_comm_rb_handle_ins");

    return ZXIC_OK;
}



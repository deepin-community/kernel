/*****************************************************************************
 * 版权所有 (C)2001-2015, 深圳市中兴通讯股份有限公司。
 * 
 * 文件名称：   zxic_index_fill.c
 * 文件标识：    
 * 内容摘要：   索引空位填充源代码头文件
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
#include "zxic_comm_index_fill.h"
#include "zxic_comm_double_link.h"

/***********************************************************/
/** 释放数据节点
* @param   p_data
*
* @return
* @remark  无
* @see
* @author  ChenWei10088471      @date  2014/02/07
************************************************************/
ZXIC_RTN32 ic_comm_node_data_free(void *p_data)
{
    ZXIC_COMM_CHECK_POINT(p_data);
    ZXIC_COMM_FREE(p_data);

    return ZXIC_OK;
}

ZXIC_RTN32 zxic_comm_indexfill_init(INDEX_FILL_CFG *p_fill_cfg,ZXIC_UINT32 index_num,ZXIC_KEY_CMP_FUNC p_cmp_fun,INDEXFILL_SWAP_FUNC p_swap_fun,ZXIC_UINT32 key_len)
{
    ZXIC_UINT32   rtn = 0;
    
    p_fill_cfg->index_num = index_num;
    p_fill_cfg->total_used = 0;
    p_fill_cfg->swap_fun   = p_swap_fun;

    rtn = zxic_comm_rb_init(&p_fill_cfg->fill_rb,0,key_len,p_cmp_fun);
    ZXIC_COMM_CHECK_RC(rtn,"zxic_comm_rb_init");
    
    return ZXIC_OK;
}
/* 前向挤压 */
ZXIC_RTN32 zxic_comm_indexfill_handle_position_right(INDEX_FILL_CFG *p_fill_cfg,INDEX_FILL_NODE* p_start,INDEX_FILL_NODE* p_end)
{
    ZXIC_UINT32   rtn       = 0;

    INDEX_FILL_NODE *p_cur_node = p_start;
    INDEX_FILL_NODE *p_nxt_node = (INDEX_FILL_NODE *)STRUCT_ENTRY_POINT(p_cur_node->rb_node.tn_ln.next,ZXIC_RB_TN,tn_ln);
    INDEX_FILL_NODE *p_pre_node = NULL;

    while(p_cur_node != p_end)
    {
        if(ICMINF_GET_NODE_LASTPOS(p_cur_node) + 1 < ICMINF_GET_NODE_FSTPOS(p_nxt_node))
        {
            break;
        }
        else
        {
            p_cur_node = p_nxt_node;
            
            if(p_cur_node->rb_node.tn_ln.next)
            {
                p_nxt_node = (INDEX_FILL_NODE *)STRUCT_ENTRY_POINT(p_cur_node->rb_node.tn_ln.next,ZXIC_RB_TN,tn_ln);
            }
        }
    }

    ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW(p_fill_cfg->index_num, 1);
    if(p_cur_node == p_end && p_fill_cfg->index_num - 1 == ICMINF_GET_NODE_LASTPOS(p_end))
    {
        return ZXIC_INDEX_FILL_FULL;
    }
    
    p_pre_node = (INDEX_FILL_NODE *)STRUCT_ENTRY_POINT(p_cur_node->rb_node.tn_ln.prev,ZXIC_RB_TN,tn_ln);

    while(p_cur_node != p_start)
    {
        if(p_fill_cfg->swap_fun)
        {
            rtn = p_fill_cfg->swap_fun(ICMINF_GET_NODE_FSTPOS(p_cur_node),ICMINF_GET_NODE_LASTPOS(p_cur_node) + 1,p_fill_cfg);
        }

        ICMINF_GET_NODE_FSTPOS(p_cur_node) = ICMINF_GET_NODE_FSTPOS(p_cur_node) + 1;

        p_cur_node = p_pre_node;

        if(p_cur_node->rb_node.tn_ln.prev)
        {
            p_pre_node = (INDEX_FILL_NODE *)STRUCT_ENTRY_POINT(p_cur_node->rb_node.tn_ln.prev,ZXIC_RB_TN,tn_ln);
        }        
    }

    if(p_fill_cfg->swap_fun)
    {
        rtn = p_fill_cfg->swap_fun(ICMINF_GET_NODE_FSTPOS(p_start),ICMINF_GET_NODE_LASTPOS(p_start)+1,p_fill_cfg);
    }

    ICMINF_GET_NODE_FSTPOS(p_start) =  ICMINF_GET_NODE_FSTPOS(p_start) + 1;
    
    return rtn;
}
/* 后向挤压 */
ZXIC_RTN32 zxic_comm_indexfill_handle_position_left(INDEX_FILL_CFG *p_fill_cfg,INDEX_FILL_NODE* p_start,INDEX_FILL_NODE* p_end)
{
    ZXIC_UINT32   rtn       = 0;

    INDEX_FILL_NODE *p_cur_node = p_start;
    INDEX_FILL_NODE *p_nxt_node = (INDEX_FILL_NODE *)STRUCT_ENTRY_POINT(p_cur_node->rb_node.tn_ln.prev,ZXIC_RB_TN,tn_ln);
    INDEX_FILL_NODE *p_pre_node = NULL;

    while(p_cur_node != p_end)
    {
        ZXIC_COMM_CHECK_POINT(p_nxt_node);
        ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW(p_nxt_node->position, p_nxt_node->usednum);
        ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW((p_nxt_node->position + p_nxt_node->usednum), 1);
        
        if(ICMINF_GET_NODE_FSTPOS(p_cur_node) -1 > ICMINF_GET_NODE_LASTPOS(p_nxt_node))
        {
            break;
        }
        else
        {
            p_cur_node = p_nxt_node;
            
            if(p_cur_node->rb_node.tn_ln.prev)
            {
                p_nxt_node = (INDEX_FILL_NODE *)STRUCT_ENTRY_POINT(p_cur_node->rb_node.tn_ln.prev,ZXIC_RB_TN,tn_ln);
            }
        }
    }

    if(p_cur_node == p_end && 0 == ICMINF_GET_NODE_FSTPOS(p_end))
    {
        return ZXIC_INDEX_FILL_FULL;
    }

    p_pre_node = (INDEX_FILL_NODE *)STRUCT_ENTRY_POINT(p_cur_node->rb_node.tn_ln.next,ZXIC_RB_TN,tn_ln);

    while(p_cur_node != p_start)
    {
        if(p_fill_cfg->swap_fun)/*将最后一个移动到本节点第一个*/
        {
            rtn = p_fill_cfg->swap_fun(ICMINF_GET_NODE_LASTPOS(p_cur_node),ICMINF_GET_NODE_FSTPOS(p_cur_node)-1,p_fill_cfg);

        }

        ICMINF_GET_NODE_FSTPOS(p_cur_node) = ICMINF_GET_NODE_FSTPOS(p_cur_node)-1;
        
        p_cur_node = p_pre_node;

        if(p_cur_node->rb_node.tn_ln.next)
        {
            p_pre_node = (INDEX_FILL_NODE *)STRUCT_ENTRY_POINT(p_cur_node->rb_node.tn_ln.next,ZXIC_RB_TN,tn_ln);
        }        
    }
    
    if(p_fill_cfg->swap_fun)
    {
        rtn = p_fill_cfg->swap_fun(ICMINF_GET_NODE_LASTPOS(p_start),ICMINF_GET_NODE_FSTPOS(p_start)-1,p_fill_cfg);
    }    

    ICMINF_GET_NODE_FSTPOS(p_start) = ICMINF_GET_NODE_FSTPOS(p_start) -1;
    
    return rtn;
}

ZXIC_RTN32 zxic_comm_indexfill_free(INDEX_FILL_CFG *p_fill_cfg,ZXIC_UINT32 free_index,ZXIC_VOID* p_rb_key,ZXIC_UINT32 *out_index)
{
    ZXIC_UINT32   rtn       = 0;
    ZXIC_RB_TN       *p_rb_out   = NULL;
    INDEX_FILL_NODE *p_inf_node = NULL;

    rtn = zxic_comm_rb_search(&p_fill_cfg->fill_rb, p_rb_key, &p_rb_out);

    if ((!p_rb_out)||(rtn!=ZXIC_OK) )
    {
        ZXIC_COMM_TRACE_ERROR("\n srh fail ,the key is not exist");
        return ZXIC_INDEX_DEL_FAIL;
    }

    p_inf_node = (INDEX_FILL_NODE *)p_rb_out;
    
    ZXIC_COMM_ASSERT(p_inf_node->usednum);

    ZXIC_COMM_CHECK_INDEX(free_index,ICMINF_GET_NODE_FSTPOS(p_inf_node),ICMINF_GET_NODE_LASTPOS(p_inf_node));

     *out_index = free_index;
    
    if(free_index == ICMINF_GET_NODE_FSTPOS(p_inf_node))
    {
        p_inf_node->position++;
    }
    else if(free_index != ICMINF_GET_NODE_LASTPOS(p_inf_node))
    {
       *out_index = ICMINF_GET_NODE_FSTPOS(p_inf_node);

       if(p_fill_cfg->swap_fun)
       {
            p_fill_cfg->swap_fun(ICMINF_GET_NODE_FSTPOS(p_inf_node), free_index, p_fill_cfg);
       }

       p_inf_node->position++;
    }
    else
    {
        ZXIC_COMM_TRACE_DEBUG("\n Free the last position,do nothing \n");
    }

    p_inf_node->usednum--;
    
    if(p_inf_node->usednum == 0)
    {
        rtn = zxic_comm_rb_delete(&p_fill_cfg->fill_rb,p_rb_key,&p_rb_out);
        ZXIC_COMM_CHECK_RC(rtn,"zxic_comm_rb_delete"); 

        if(!p_rb_out)
        {
            ZXIC_COMM_TRACE_ERROR("\n srh fail ,the key is not exist");
            return ZXIC_INDEX_DEL_FAIL;
        }

        ZXIC_COMM_FREE(p_inf_node->rb_node.p_key);

        ZXIC_COMM_FREE(p_inf_node); 
    }
    else
    {
        /*ICMINF_GET_NODE_FSTPOS(p_inf_node) = ICMINF_GET_NODE_FSTPOS(p_inf_node)-1;*/
    }

    ZXIC_COMM_CHECK_INDEX_SUB_OVERFLOW(p_fill_cfg->total_used, 1);
    p_fill_cfg->total_used--;
    
    return ZXIC_OK;
}

ZXIC_RTN32 zxic_comm_indexfill_destroy(INDEX_FILL_CFG *p_fill_cfg)
{
    ZXIC_UINT32 rtn = 0;

    rtn = zxic_comm_dlink_release(&p_fill_cfg->fill_rb.tn_list,ic_comm_node_data_free);
    ZXIC_COMM_CHECK_RC(rtn,"zxic_comm_dlink_release");

    ZXIC_COMM_MEMSET(p_fill_cfg,0,ZXIC_SIZEOF(INDEX_FILL_CFG));
    
    return ZXIC_OK;
}

ZXIC_RTN32 zxic_comm_indexfill_show_all_position(INDEX_FILL_CFG *p_fill_cfg)
{
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    D_NODE       *p_node     = NULL;
    INDEX_FILL_NODE *p_inf_node = NULL;

    p_node = p_fill_cfg->fill_rb.tn_list.p_next;

    ZXIC_COMM_PRINT("\n *************************Used Position*************************\n");

    while(p_node)
    {
        ZXIC_COMM_PRINT("\n ==== Num [%d ] ==== :",i); 
        
        p_inf_node = (INDEX_FILL_NODE *)(STRUCT_ENTRY_POINT(p_node,ZXIC_RB_TN,tn_ln));
        
        for(j = 0 ;j < p_inf_node->usednum;j++)
        {            
            ZXIC_COMM_PRINT(" %d ",ICMINF_GET_NODE_FSTPOS(p_inf_node)+ j);

            if (j != 0 && j % 8 == 0)
            {
                ZXIC_COMM_PRINT("\n                   ");
            }
        }        

        ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW(i, 1);
        i++;
        
        p_node = p_node->next; 
    }

    ZXIC_COMM_PRINT("\n*******End*******\n");

    return ZXIC_OK;
    
}

ZXIC_RTN32 zxic_comm_indexfill_store(INDEX_FILL_CFG *p_fill_cfg, ZXIC_UINT32 *p_size, ZXIC_UINT8 **p_data_buff)
{
    ZXIC_UINT32   rtn       = 0;
    ZXIC_UINT32   i         = 0;
    ZXIC_UINT32   used_node_num      = 0;
    ZXIC_UINT32   rb_node_size       = 0;
    ZXIC_UINT32   max_index_num      = 0;  
    ZXIC_UINT32   buff_offset        = 0;
    ZXIC_UINT32   tmp_val            = 0;
    D_NODE          *p_node     = NULL;
    INDEX_FILL_NODE *p_inf_node = NULL;
    ZXIC_UINT8 *p_item_buff           = NULL;
    ZXIC_UINT32 item_buff_offset     = 0;
    /*
    |   used_node_num  |  rb_node_size | max_index_num  |//head
    | node_start_index | node_used_num | rb_key ... ... |//item
    */
    
    ZXIC_COMM_CHECK_POINT(p_fill_cfg);
    ZXIC_COMM_CHECK_POINT(p_size);
    //item size
    rb_node_size = p_fill_cfg->fill_rb.key_size + 8;/*sizeof(ZXIC_UINT32) + sizeof(ZXIC_UINT32);*/
    max_index_num = p_fill_cfg->index_num;
    
    p_node = p_fill_cfg->fill_rb.tn_list.p_next;

    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW(used_node_num, 1);
    while(p_node)
    {
        //p_inf_node = (INDEX_FILL_NODE *)(STRUCT_ENTRY_POINT(p_node,ZXIC_RB_TN,tn_ln));
        used_node_num++;
        p_node = p_node->next; 
    }
    tmp_val = ZXIC_SIZEOF(ZXIC_UINT32) * 3;
    ZXIC_COMM_CHECK_INDEX_MUL_OVERFLOW(rb_node_size, used_node_num);
    ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW(tmp_val, rb_node_size * used_node_num);    
    *p_size = tmp_val + rb_node_size * used_node_num;
    
    *p_data_buff = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(*p_size);
    ZXIC_COMM_CHECK_POINT(*p_data_buff);
    ZXIC_COMM_MEMSET(*p_data_buff, 0, *p_size);
    
    buff_offset = 0;
    
    ZXIC_COMM_MEMCPY_S(*p_data_buff + buff_offset, ZXIC_SIZEOF(ZXIC_UINT32), &used_node_num, ZXIC_SIZEOF(ZXIC_UINT32));
    buff_offset += ZXIC_SIZEOF(ZXIC_UINT32);
    ZXIC_COMM_MEMCPY_S(*p_data_buff + buff_offset, ZXIC_SIZEOF(ZXIC_UINT32), &rb_node_size, ZXIC_SIZEOF(ZXIC_UINT32));
    buff_offset += ZXIC_SIZEOF(ZXIC_UINT32);
    ZXIC_COMM_MEMCPY_S(*p_data_buff + buff_offset, ZXIC_SIZEOF(ZXIC_UINT32), &max_index_num, ZXIC_SIZEOF(ZXIC_UINT32));
    buff_offset += ZXIC_SIZEOF(ZXIC_UINT32);

    p_item_buff = (ZXIC_UINT8 *)ZXIC_COMM_MALLOC(rb_node_size);
    ZXIC_COMM_CHECK_POINT(p_item_buff);
    ZXIC_COMM_MEMSET(p_item_buff, 0, rb_node_size);
    
    for(i = 0; i < used_node_num; i++)
    {
        if(0 == i)
        {
            p_node = p_fill_cfg->fill_rb.tn_list.p_next;
            ZXIC_COMM_CHECK_POINT_MEMORY_FREE(p_node, p_item_buff);
        }
        else
        {
            p_node = p_node->next;
            ZXIC_COMM_CHECK_POINT_MEMORY_FREE(p_node, p_item_buff);
        }
        item_buff_offset = 0;
        
        p_inf_node = (INDEX_FILL_NODE *)(STRUCT_ENTRY_POINT(p_node,ZXIC_RB_TN,tn_ln));

        ZXIC_COMM_MEMCPY_S(p_item_buff + item_buff_offset, ZXIC_SIZEOF(ZXIC_UINT32), &(p_inf_node->position), ZXIC_SIZEOF(ZXIC_UINT32));
        item_buff_offset += ZXIC_SIZEOF(ZXIC_UINT32);
        ZXIC_COMM_MEMCPY_S(p_item_buff + item_buff_offset, ZXIC_SIZEOF(ZXIC_UINT32), &(p_inf_node->usednum), ZXIC_SIZEOF(ZXIC_UINT32));
        item_buff_offset += ZXIC_SIZEOF(ZXIC_UINT32);
        ZXIC_COMM_MEMCPY_S(p_item_buff + item_buff_offset, p_fill_cfg->fill_rb.key_size, p_inf_node->rb_node.p_key, p_fill_cfg->fill_rb.key_size);

        ZXIC_COMM_MEMCPY_S(*p_data_buff + buff_offset, rb_node_size, p_item_buff, rb_node_size);

        ZXIC_COMM_CHECK_INDEX_ADD_OVERFLOW_MEMORY_FREE(buff_offset, rb_node_size, p_item_buff);
        buff_offset += rb_node_size;
    }

    ZXIC_COMM_ASSERT(buff_offset == *p_size);
    
    ZXIC_COMM_FREE(p_item_buff);
    
    return rtn;
}

ZXIC_RTN32 zxic_comm_indexfill_clear(INDEX_FILL_CFG *p_fill_cfg)
{
    ZXIC_UINT32 rtn = 0;
    ZXIC_UINT32 key_len = p_fill_cfg->fill_rb.key_size;
    ZXIC_KEY_CMP_FUNC p_cmp_fun = p_fill_cfg->fill_rb.p_cmpfun;
    INDEX_FILL_NODE *fill_node;
    D_NODE *p_curnode = NULL;
    void *cur_data;
    p_curnode = p_fill_cfg->fill_rb.tn_list.p_next;
    
    while(p_curnode)
    {
        cur_data = p_curnode->data;
        p_curnode = p_curnode->next;
        fill_node = cur_data;
        ZXIC_COMM_FREE(fill_node->rb_node.p_key);
        ZXIC_COMM_FREE(fill_node);
        
    }
    
    rtn = zxic_comm_rb_destroy(&p_fill_cfg->fill_rb);
    ZXIC_COMM_CHECK_RC(rtn, "zxic_comm_rb_destroy");

    rtn = zxic_comm_rb_init(&p_fill_cfg->fill_rb, 0, key_len, p_cmp_fun);
    ZXIC_COMM_CHECK_RC(rtn, "zxic_comm_rb_init");

    
    p_fill_cfg->total_used = 0;


    return ZXIC_OK;
}



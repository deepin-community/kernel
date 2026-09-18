/*****************************************************************************
 * 版权所有 (C)2001-2015, 深圳市中兴通讯股份有限公司。
 * 
 * 文件名称：   zxic_index_fill.h
 * 文件标识：    
 * 内容摘要：  索引空位填充源代码头文件
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

#ifndef _ZXIC_COMM_INDEX_FILL_H
#define _ZXIC_COMM_INDEX_FILL_H


typedef ZXIC_RTN32 (*INDEXFILL_SWAP_FUNC)(ZXIC_UINT32 old_index,ZXIC_UINT32 new_index,ZXIC_VOID *p_cfg);


typedef struct
{
    ZXIC_RB_TN    rb_node;
    ZXIC_UINT32       position;
    ZXIC_UINT32       usednum;
}INDEX_FILL_NODE;

typedef struct
{
    ZXIC_RB_CFG           fill_rb;
    ZXIC_UINT32               index_num;  
    INDEX_FILL_NODE      *fill_node;
    INDEXFILL_SWAP_FUNC  swap_fun;
    ZXIC_UINT32               total_used;
}INDEX_FILL_CFG;

ZXIC_RTN32 ic_comm_node_data_free(void *p_data);


ZXIC_RTN32 zxic_comm_indexfill_init(INDEX_FILL_CFG *p_fill_cfg,
                              ZXIC_UINT32 index_num,
                              ZXIC_KEY_CMP_FUNC p_cmp_fun,
                              INDEXFILL_SWAP_FUNC p_swap_fun,
                              ZXIC_UINT32 key_len);

ZXIC_RTN32 zxic_comm_indexfill_free(INDEX_FILL_CFG *p_fill_cfg,
                              ZXIC_UINT32 free_index,
                              ZXIC_VOID* p_rb_key,
                              ZXIC_UINT32 *out_index);

ZXIC_RTN32 zxic_comm_indexfill_destroy(INDEX_FILL_CFG *p_fill_cfg);

ZXIC_RTN32 zxic_comm_indexfill_store(INDEX_FILL_CFG *p_fill_cfg, ZXIC_UINT32 *p_size, ZXIC_UINT8 **p_data_buff);

ZXIC_RTN32 zxic_comm_indexfill_show_all_position(INDEX_FILL_CFG *p_fill_cfg);
ZXIC_RTN32 zxic_comm_indexfill_clear(INDEX_FILL_CFG *p_fill_cfg);

#define    ICMINF_GET_NODE_LASTPOS(p_inf_node) \
    ((p_inf_node)->position + (p_inf_node)->usednum -1)

#define    ICMINF_GET_NODE_FSTPOS(p_inf_node) \
    ((p_inf_node)->position)

#endif


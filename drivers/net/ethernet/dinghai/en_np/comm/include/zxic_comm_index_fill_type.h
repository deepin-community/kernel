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

#ifndef _ZXIC_COMM_INDEX_FILL_TYPE_H
#define _ZXIC_COMM_INDEX_FILL_TYPE_H

typedef ZXIC_RTN32 (*INDEXFILL_TYPE_SWAP_FUNC)(ZXIC_UINT32 old_index,ZXIC_UINT32 new_index,ZXIC_VOID *p_cfg);

typedef struct
{
    ZXIC_UINT32 is_used;    /* 0空闲，1已分配 */
    ZXIC_UINT32 prio;
}INDEX_FILL_TYPE_INDEX_STATUS;  /* is_used==0时，不关心prio */

typedef struct
{
    ZXIC_RB_TN prio_rb_node;                /* prio_rb_tree中的节点 */
    ZXIC_RB_CFG idx_rb_cfg;                 /* 以index为key，详细记录每个prio的每个index信息 */
    ZXIC_UINT32 prio;                       /* prio值*/
}INDEX_FILL_TYPE_PRIO_NODE;

typedef struct {
    ZXIC_UINT32 prio;
} SSP4_INDEX_FILL_TYPE_PRIO_RB_KEY;         /* prio红黑树的key */

typedef struct
{
    ZXIC_RB_CFG prio_rb;    /* 每type的prio红黑树，以prio为key */
    D_HEAD mv_list_head;    /* 同type的移位链表 */
    ZXIC_VOID *p_cfg;       /* type相关的其他参数定义，外部自行定义 */
}INDEX_FILL_TYPE_MNG_CFG;

typedef struct
{
    ZXIC_UINT32                     index_num;                        /* 本池的索引数量 */
    ZXIC_UINT32                     total_used;                       /* 本池已使用的索引数量 */
    ZXIC_UINT32                     prio_max;                       /* 最大优先级范围 */
    ZXIC_UINT32                     global_max_num;                       /* 全局最大数量*/
    INDEX_FILL_TYPE_INDEX_STATUS    *p_idx_status;               /* 全局索引池状态位记录数组指针 */
    INDEXFILL_TYPE_SWAP_FUNC        swap_fun;                         /* 向前或者向后挤压时的移位操作函数 */
}INDEX_FILL_TYPE_INDEX_POOL_CFG; /* 多种type共享的索引池 */



ZXIC_UINT32 zxic_comm_indexfill_type_idx_status_get(INDEX_FILL_TYPE_INDEX_STATUS *index_status, 
                                                    ZXIC_UINT32 index, 
                                                    ZXIC_UINT32 *used_status_flag, 
                                                    ZXIC_UINT32 *used_status_prio);


ZXIC_UINT32 zxic_comm_indexfill_type_idx_status_set(INDEX_FILL_TYPE_INDEX_STATUS *index_status, 
                                                    ZXIC_UINT32 index, 
                                                    ZXIC_UINT32 prio, 
                                                    ZXIC_UINT32 used_flag);

ZXIC_RTN32 zxic_comm_indexfill_type_init(INDEX_FILL_TYPE_INDEX_POOL_CFG *p_fill_type_index_pool_cfg, 
                                            ZXIC_UINT32 index_num, 
                                            ZXIC_UINT32 prio_max, 
                                            ZXIC_UINT32 global_max_num,
                                            INDEXFILL_TYPE_SWAP_FUNC p_swap_fun);

ZXIC_RTN32 zxic_comm_indexfill_type_rb_init(INDEX_FILL_TYPE_MNG_CFG *p_fill_type_mng_cfg);

/* 多张type共享的索引记录 */
/* 每种type，一个prio红黑树管理结构，一个prio+index红黑树管理结构 */
ZXIC_RTN32 zxic_comm_indexfill_type_alloc(INDEX_FILL_TYPE_INDEX_POOL_CFG *p_fill_type_index_pool_cfg, 
                                          INDEX_FILL_TYPE_MNG_CFG *p_fill_type_mng_cfg, 
                                          ZXIC_UINT32 prio,
                                          ZXIC_UINT32 *out_index);
/* 多张type共享的索引记录 */
/* 每种type，一个prio红黑树管理结构，一个prio+index红黑树管理结构 */
ZXIC_RTN32 zxic_comm_indexfill_type_free(INDEX_FILL_TYPE_INDEX_POOL_CFG *p_fill_type_index_pool_cfg, 
                                          INDEX_FILL_TYPE_MNG_CFG *p_fill_type_mng_cfg, 
                                          ZXIC_UINT32 free_index, 
                                          ZXIC_UINT32 *out_index);

ZXIC_RTN32 zxic_comm_indexfill_type_show_all_position(INDEX_FILL_TYPE_INDEX_POOL_CFG *p_fill_type_index_pool_cfg, 
                                                        INDEX_FILL_TYPE_MNG_CFG *p_fill_type_mng_cfg);


#endif


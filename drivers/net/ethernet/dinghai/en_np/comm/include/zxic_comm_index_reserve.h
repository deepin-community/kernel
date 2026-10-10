/*****************************************************************************
 * 版权所有 (C)2001-2015, 深圳市中兴通讯股份有限公司。
 * 
 * 文件名称：   zxic_index_reserve.h
 * 文件标识：    
 * 内容摘要：   索引预留算法源代码头文件
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
#ifndef _ZXIC_COMM_INDEX_RESERVE_H
#define _ZXIC_COMM_INDEX_RESERVE_H

#define  CMP_MODE_LOW   (0)
#define  CMP_MODE_HIGH  (1)
typedef ZXIC_UINT32 (*SWAP_FUNC)(ZXIC_UINT32 old_index,ZXIC_UINT32 new_index);
typedef ZXIC_UINT32 (*LOCAL_SWAP_FUNC)(ZXIC_VOID *p_cfg,ZXIC_UINT32 old_index,ZXIC_UINT32 new_index);

typedef struct
{
    ZXIC_UINT32 head_curr;
    ZXIC_UINT32 tail_curr;
}INDEX_CURR;

typedef struct
{
    ZXIC_UINT32 old_handle;
    ZXIC_UINT32 new_handle;
}INR_SWAP_NODE;

typedef struct _index_res_cfg
{
    ZXIC_UINT32          total_num;
    ZXIC_UINT32          space_num;
    ZXIC_UINT32*         index_prop;
    ZXIC_RB_CFG*     index_usedrb;
    ZXIC_RB_CFG*     index_freerb;
    ZXIC_RB_TN*      index_node;
    SWAP_FUNC       swap_fun;
    LOCAL_SWAP_FUNC local_fun;
    INDEX_CURR*     index_curr;
    D_HEAD          swap_list;
    ZXIC_UINT32          total_used;    
    ZXIC_UINT32           is_init;
    ZXIC_UINT32     indexres_id;
}INDEX_RES_CFG;
ZXIC_VOID zxic_comm_rb_tn_relation_clear(ZXIC_RB_TN *rb_tn_node);

ZXIC_RTN32 zxic_comm_indexres_init(INDEX_RES_CFG *p_indexres_cfg,  /*配置句柄*/
                             ZXIC_UINT32         arg_total_num,   /*索引总数*/
                             ZXIC_UINT32         arg_space_num,   /*空间总数*/
                             ZXIC_UINT32*        arg_index_prop,  /*空间大小，若用户提供，则按用户提供的进行空间分配，否则平均分配所有空间*/
                             SWAP_FUNC      p_swap_fun,
                             LOCAL_SWAP_FUNC local_fun);     /*重排函数，注册则调用，否则不调*/

ZXIC_RTN32 zxic_comm_indexres_alloc(INDEX_RES_CFG *p_indexres_cfg, /*配置句柄*/
                                 ZXIC_UINT32         space_val,   /*空间序列*/
                                 ZXIC_UINT32         *out_index); /*出参，分配的索引*/

ZXIC_RTN32 zxic_comm_indexres_free(INDEX_RES_CFG *p_indexres_cfg,
                             ZXIC_UINT32         space_val,
                             ZXIC_UINT32         free_index);

ZXIC_RTN32 zxic_comm_indexres_destory(INDEX_RES_CFG *p_indexres_cfg);

ZXIC_RTN32 zxic_comm_indexres_reset(INDEX_RES_CFG *p_indexres_cfg);

ZXIC_RTN32 zxic_comm_indexres_showinfo(INDEX_RES_CFG *p_indexres_cfg);/*重置整个索引空间，恢复到最初的配置状态，注意，此时所有的索引都需要在未使用状态*/


#endif


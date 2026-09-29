/*****************************************************************************
 * 版权所有 (C)2001-2015, 深圳市中兴通讯股份有限公司。
 * 
 * 文件名称：   zxic_comm_rb_tree.h
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

#ifndef _ZXIC_COMM_RB_TREE_H
#define _ZXIC_COMM_RB_TREE_H

#include "zxic_comm_double_link.h"
#include "zxic_comm_liststack.h"

#define   ZXIC_RBT_RED          (0x1)
#define   ZXIC_RBT_BLACK        (0x2)
#define   ZXIC_RBT_MAX_DEPTH    (96)

typedef ZXIC_SINT32  (*ZXIC_RB_CMPFUN)(ZXIC_VOID *p_new,ZXIC_VOID *p_old,ZXIC_UINT32 keysize);


typedef struct _rb_tn
{
    ZXIC_VOID     *p_key;
    ZXIC_UINT32    color_lsv;       /*last 2 bits indicate color, bit2-31 if dynamic=0 indicate list val*/
    struct    _rb_tn   *p_left;
    struct    _rb_tn   *p_right;
    struct    _rb_tn   *p_parent;
    D_NODE    tn_ln;
}ZXIC_RB_TN;

typedef struct _rb_cfg
{
    ZXIC_UINT32                 key_size;
    ZXIC_UINT32                 is_dynamic;  /* 1 - customer manage memory;0 - alloc all memory*/
    ZXIC_RB_TN             *p_root;      /* rb tree root node */
    D_HEAD                 tn_list;
    ZXIC_RB_CMPFUN          p_cmpfun;
    ZXIC_LISTSTACK_MANGER   *p_lsm;      /* list stack manage*/
    ZXIC_UINT8                   *p_keybase;
    ZXIC_RB_TN              *p_tnbase;
    ZXIC_UINT32             is_init;
}ZXIC_RB_CFG;



#define GET_TN_COLOR(p_tn) \
           ((p_tn == NULL) ? ZXIC_RBT_BLACK :(p_tn)->color_lsv & 0x3)
           
#define SET_TN_COLOR(p_tn,color) \
    do{\
        (p_tn)->color_lsv  &= 0xfffffffc;\
        (p_tn)->color_lsv  |= (color & 0x3);\
    }while(0)
   

           
#define GET_TN_LSV(p_tn)   \
        ((p_tn)->color_lsv >> 2 )

#define SET_TN_LSV(p_tn,list_val) \
    do{\
        (p_tn)->color_lsv &= 0x3;\
        (p_tn)->color_lsv |= ((list_val) << 2); \
    }while(0)


/*init the rb node ,be careful init_color is red*/
#define INIT_RBT_TN(p_tn,p_newkey) \
    do{\
        (p_tn)->p_key    = p_newkey; \
        (p_tn)->color_lsv= 0; \
        (p_tn)->p_left   = NULL; \
        (p_tn)->p_right  = NULL; \
        (p_tn)->p_parent = NULL; \
        INIT_D_NODE(&((p_tn)->tn_ln),(p_tn));\
    }while(0)

ZXIC_RTN32 zxic_comm_rb_init(ZXIC_RB_CFG *p_rb_cfg,
                       ZXIC_UINT32      total_num,
                       ZXIC_UINT32      key_size,
                       ZXIC_RB_CMPFUN cmpfun);


ZXIC_RTN32 zxic_comm_rb_insert(ZXIC_RB_CFG *p_rb_cfg,
                         ZXIC_VOID       *p_key,
                         ZXIC_VOID       *out_val);

ZXIC_RTN32 zxic_comm_rb_delete(ZXIC_RB_CFG *p_rb_cfg,
                         ZXIC_VOID       *p_key,
                         ZXIC_VOID       *out_val);

ZXIC_RTN32 zxic_comm_rb_search(ZXIC_RB_CFG *p_rb_cfg,
                         ZXIC_VOID       *p_key,
                         ZXIC_VOID       *out_val);

ZXIC_RTN32 zxic_comm_rb_destroy(ZXIC_RB_CFG *p_rb_cfg);

ZXIC_RB_TN *zxic_comm_rb_get_1st_tn(ZXIC_RB_CFG *p_rb_cfg);

ZXIC_RB_TN *zxic_comm_rb_get_last_tn(ZXIC_RB_CFG *p_rb_cfg);

ZXIC_RTN32 zxic_comm_rb_get_1st_key(ZXIC_RB_CFG* p_rb_cfg, ZXIC_VOID *p_key_out);

ZXIC_RTN32 zxic_comm_rb_get_last_key(ZXIC_RB_CFG*  p_rb_cfg, ZXIC_VOID *p_key_out);

ZXIC_RTN32 zxic_comm_rb_insert_spec_index(ZXIC_RB_CFG *p_rb_cfg, ZXIC_VOID *p_key, ZXIC_UINT32 in_idx);


#define ZXIC_RBT_RC_BASE               (0x1000)

#define ZXIC_RBT_RC_UPDATE             (ZXIC_RBT_RC_BASE | 0x1)
#define ZXIC_RBT_RC_SRHFAIL            (ZXIC_RBT_RC_BASE | 0x2)
#define ZXIC_RBT_RC_FULL               (ZXIC_RBT_RC_BASE | 0x3)
#define ZXIC_RBT_ISEMPTY_ERR           (ZXIC_RBT_RC_BASE | 0x4)
#define ZXIC_RBT_PARA_INVALID          (ZXIC_RBT_RC_BASE | 0x5)

#endif


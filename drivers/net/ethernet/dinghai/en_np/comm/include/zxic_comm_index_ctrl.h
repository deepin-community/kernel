/*********************************************************************
* 版权所有 (C)2001, 深圳市中兴通讯股份有限公司。
*
* 文件名称：
* 文件标识：
* 内容摘要:
* 其它说明:
*
*
* 当前版本：
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT   : 100% 
* 作    者： 
* 完成日期：2010-10-29
********************************************************************/
#ifndef _ZXIC_COMM_INDEX_CONTROLLER_H_
#define _ZXIC_COMM_INDEX_CONTROLLER_H_

//#include "zxic_common.h"
//#include "zxic_comm_rb_tree.h"
//#include "zxic_comm_double_link.h"


#define ZXIC_INDEX_EXPAND_MAX_NUM   (900)
#define INDEX_KEY_LENGTH           (4)
typedef struct zxic_index_ctrl_cfg 
{

    ZXIC_UINT32                   index_cursor_current    ;
    ZXIC_UINT32                   index_cursor_last       ;
    ZXIC_UINT32                   index_cursor_max_cur    ;
    ZXIC_UINT32                   index_ctrl_is_init      ;
    ZXIC_UINT32*                  p_index_buf             ;

    ZXIC_RB_CFG               index_ctrl_rb_tree      ;
    ZXIC_RB_CFG               rcd_ctrl_rb_tree        ;
    
    D_HEAD                  *p_index_ctrl_link       ;
}ZXIC_INDEX_CTRL_CFG;

typedef struct _zxic_index_api_params
{
    ZXIC_UINT32 zxic_expand_num            ;    /*the expand num of this item  */
    ZXIC_UINT32 zxic_opera_mode            ;    /*0:add;1:del;2:sch            */
    ZXIC_UINT32 zxic_rsp_isexit            ;    /*the rsp of whether is exit   */
    ZXIC_UINT32 *p_zxic_out_index          ;    /*the address of response      */ 
    ZXIC_VOID   *p_zxic_data               ;    /*the data of inserting in tcam*/
  
} ZXIC_INDEX_API_PARAMS;

typedef enum functionNo     /* 接口提供的表操作类型 */                                                           
{
    INDEX_CTRL_ADD,
    INDEX_CTRL_ADD_FROM_LAST,
    INDEX_CTRL_DEL,
    INDEX_CTRL_SEARCH,   
    INDEX_CTRL_UNDEFINED
}FUNCTION_NO;




ZXIC_RTN32 zxic_comm_indexctrl_get_free_index(ZXIC_INDEX_CTRL_CFG *p_table_info,ZXIC_UINT32 func_type,ZXIC_UINT32 *p_free_index_num);
ZXIC_RTN32 zxic_comm_indexctrl_add_from_last(ZXIC_INDEX_CTRL_CFG *p_table_info,ZXIC_VOID *data,ZXIC_UINT32 expand_num,ZXIC_UINT32 *out_index);
ZXIC_RTN32 zxic_comm_indexctrl_sch(ZXIC_INDEX_CTRL_CFG *p_table_info,ZXIC_VOID *data,ZXIC_UINT32 *p_is_exit,ZXIC_UINT32 *out_index);
ZXIC_RTN32 zxic_comm_indexctrl_extcommand(ZXIC_INDEX_API_PARAMS *p_zxic_api_params,ZXIC_INDEX_CTRL_CFG *p_table_info) ;
ZXIC_RTN32 zxic_comm_indexctrl_add(ZXIC_INDEX_CTRL_CFG *p_table_info,ZXIC_VOID *data,ZXIC_UINT32 expand_num,ZXIC_UINT32 *out_index);
ZXIC_RTN32 zxic_comm_indexctrl_init(ZXIC_INDEX_CTRL_CFG *p_table_info,ZXIC_UINT32 index_max_num,ZXIC_UINT32 table_key_len);
ZXIC_RTN32 zxic_comm_indexctrl_getindex_from_last(ZXIC_INDEX_CTRL_CFG *p_table_info,ZXIC_UINT32 *p_index_out);
ZXIC_RTN32 zxic_comm_indexctrl_del(ZXIC_INDEX_CTRL_CFG *p_table_info,ZXIC_VOID *data,ZXIC_UINT32 *out_index);
ZXIC_RTN32 zxic_comm_indexctrl_getindex(ZXIC_INDEX_CTRL_CFG *p_table_info,ZXIC_UINT32 *p_index_out);
ZXIC_SINT32    zxic_comm_indexctrl_cmp_key(ZXIC_VOID *new_key, ZXIC_VOID *old_key,ZXIC_UINT32 key_len);


#endif



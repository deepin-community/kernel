/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_mem.h
* 文件标识 : 
* 内容摘要 : 
* 其它说明 : 
* 当前版本 : 
* 完成日期 : 2014/03/20
* DEPARTMENT: ASIC_FPGA_R&D_Dept 
* MANUAL_PERCENT: 100%   
 
* 修改记录1: 
* 修改日期:  
* 版 本 号:  
* 修 改 人:  
* 修改内容:  
***************************************************************/

#ifndef _DPP_MEM_H_
#define _DPP_MEM_H_

#ifdef __cplusplus
extern "C"{
#endif

typedef enum dpp_mem_no_e
{
    PKTRX_PHYPORT_UDF_ATTRIm           = 0,
    PKTRX_PHYPORT_HDW_ATTRIm           = 1,
    PKTRX_PHYPORT_FLOW_PCm             = 2,
    PKTRX_ICU_TCAMm                    = 3,
    PKTRX_FLOWNUM_TCAMm                = 4,
}DPP_MEM_NO_E;

typedef struct dpp_mem_field_t
{
    ZXIC_SINT8   *p_name;         /* 字段名 */
    ZXIC_UINT32  flags;           /* 标志位 */
    ZXIC_UINT16  msb_pos;         /* 最高比特位置，以寄存器列表为准*/
    ZXIC_UINT16  len;             /* 字段长度，以比特为单位 */
}DPP_MEM_FIELD_T;


#define DPP_MEM_FLAG_TCAM     (1<<0)  /* Tcam类型的表 */
typedef struct dpp_mem_info_t
{
    ZXIC_UINT32 mem_no;
    ZXIC_UINT32 module_no;
    ZXIC_UINT32 flags;
    ZXIC_UINT32 mem_id;     /* 模块内部的mem标识 */
    ZXIC_UINT32 index_min;  /* 最小索引值 */
    ZXIC_UINT32 index_max;  /* 最大索引值 */
    ZXIC_UINT32 width;      /* 表项宽度，以字节为单位 */

    DPP_MEM_FIELD_T *p_fileds;  /* 表项所有字段 */

}DPP_MEM_INFO_T;

#ifdef __cplusplus
}
#endif

#endif



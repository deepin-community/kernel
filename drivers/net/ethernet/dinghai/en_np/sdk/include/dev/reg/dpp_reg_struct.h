/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_reg_struct.h
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : 石金锋
* 完成日期 : 2014/02/10
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/
#ifndef _DPP_REG_STRUCT_H_
#define _DPP_REG_STRUCT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "zxic_common.h"
#include "dpp_dev.h"
#include "dpp_type_api.h"


typedef ZXIC_UINT32 (*DPP_REG_WRITE)(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 *p_data);
typedef ZXIC_UINT32 (*DPP_REG_READ)(DPP_DEV_T *dev, ZXIC_UINT32 addr, ZXIC_UINT32 *p_data);


#define DPP_FIELD_FLAG_RO      (1<<0)        /* 只读标志 */
#define DPP_FIELD_FLAG_RW      (1<<1)        /* 读写标志 */
#define DPP_FIELD_FLAG_RC      (1<<2)        /* 读清标志 */
#define DPP_FIELD_FLAG_WO      (1<<3)        /* 只写标志 */
#define DPP_FIELD_FLAG_WC      (1<<4)        /* 写清标志 */

typedef struct dpp_field_t
{
    ZXIC_CHAR    *p_name;                         /* 字段名 */
    ZXIC_UINT32  flags;                           /* 标志位 */
    ZXIC_UINT16  msb_pos;                         /* 最高比特位置，以寄存器列表为准*/
    ZXIC_UINT16  len;                             /* 字段长度，以比特为单位 */
    ZXIC_UINT32  default_value;                   /* 缺省值 */
    ZXIC_UINT32  default_step;                    /* 缺省值步长*/
}DPP_FIELD_T;


#define DPP_REG_FLAG_DIRECT      (0<<0)      /* 直接读写寄存器 */
#define DPP_REG_FLAG_INDIRECT    (1<<0)      /* 间接读写寄存器 */
#define DPP_REG_FLAG_WO          (1<<1)      /* 只写寄存器 */

#define DPP_REG_NUL_ARRAY        (0<<0)      /*零元寄存器序列 */
#define DPP_REG_UNI_ARRAY        (1<<0)      /*一元寄存器序列 */
#define DPP_REG_BIN_ARRAY        (1<<1)      /*二元寄存器序列 */

typedef struct dpp_reg_t
{
    ZXIC_CHAR    *reg_name;                       /* 寄存器名称*/
    ZXIC_UINT32  reg_no;                          /* 寄存器的编号 */
    ZXIC_UINT32  module_no;                       /* 寄存器归属的模块的编号 */
    ZXIC_UINT32  flags;                           /* 标志位 */
    ZXIC_UINT32  array_type;                      /* 寄存器偏移类型*/
    ZXIC_UINT32  addr;                            /* 寄存器的芯片地址*/
    ZXIC_UINT32  width;                           /* 寄存器位宽，以字节为单位 */
    ZXIC_UINT32  m_size;                          /* 寄存器序列参数1的个数 */
    ZXIC_UINT32  n_size;                          /* 寄存器序列参数2的个数*/
    ZXIC_UINT32  m_step;                          /* 寄存器序列参数1的偏移步长 */
    ZXIC_UINT32  n_step;                          /* 寄存器序列参数2的偏移步长 */
    ZXIC_UINT32  field_num;                       /* 包含的字段个数 */
    DPP_FIELD_T *p_fields;                   /* 寄存器所有字段 */

    DPP_REG_WRITE      p_write_fun;          /* 寄存器写函数 */
    DPP_REG_READ       p_read_fun;           /* 寄存器读函数 */
}DPP_REG_T;


#ifdef __cplusplus
}
#endif

#endif




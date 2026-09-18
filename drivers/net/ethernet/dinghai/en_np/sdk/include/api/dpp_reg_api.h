/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_reg.h
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : wcl
* 完成日期 : 2014/02/12
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/

#ifndef _DPP_REG_API_H_
#define _DPP_REG_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dpp_dev.h"
#include "dpp_reg_struct.h"

/**  所有上层模块不需要直接使用该全局变量*/
extern  DPP_REG_T g_dpp_reg_info[];

/**  public*/
#define DPP_REG(no)              (g_dpp_reg_info[no])
#define DPP_REG_NAME(no)         ((DPP_REG(no)).reg_name)
#define DPP_REG_NO(no)           ((DPP_REG(no)).reg_no)
#define DPP_REG_MODULE_NO(no)    ((DPP_REG(no)).module_no)
#define DPP_REG_FLAGS(no)        ((DPP_REG(no)).flags)
#define DPP_REG_TYPE(no)         ((DPP_REG(no)).array_type)
#define DPP_REG_ADDR(no)         ((DPP_REG(no)).addr)
#define DPP_REG_WIDTH(no)        ((DPP_REG(no)).width)
#define DPP_REG_M_SIZE(no)       ((DPP_REG(no)).m_size)
#define DPP_REG_N_SIZE(no)       ((DPP_REG(no)).n_size)
#define DPP_REG_M_STEP(no)       ((DPP_REG(no)).m_step)
#define DPP_REG_N_STEP(no)       ((DPP_REG(no)).n_step)
#define DPP_REG_FIELD_NUM(no)    ((DPP_REG(no)).field_num)
#define DPP_REG_FIELD_NAME(no, field_no)    (((DPP_REG(no)).p_fields+field_no)->p_name)

typedef enum dpp_bar_4k_e
{
   BAR_4K_DTB = 0,  /**<  @brief 0*/
   BAR_4K_ETCAM,    /**<  @brief 1*/
   BAR_4K_CLS0,     /**<  @brief 2*/
   BAR_4K_CLS1,     /**<  @brief 3*/
   BAR_4K_CLS2,     /**<  @brief 4*/
   BAR_4K_CLS3,     /**<  @brief 5*/
   BAR_4K_CLS4,     /**<  @brief 6*/
   BAR_4K_CLS5,     /**<  @brief 7*/
   BAR_4K_SE,       /**<  @brief 8*/
   BAR_4K_SMMU1,    /**<  @brief 9*/
   BAR_4K_MAX           
} DPP_BAR_4K_E;

typedef struct
{ 
    ZXIC_UINT32 reg_module;      /*DPP_MODULE_E*/
    ZXIC_UINT32 index_4k;        /*BAR NP空间映射4K相对索引*/
    ZXIC_UINT32 addr_offset;     /*寄存器偏移，用于计算BAR映射空间位置*/
}DPP_REG_OFFSET_ADDR;

/***********************************************************/
/** 获取寄存器属性
* @param   reg_no  寄存器编号
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_REG_T* dpp_reg_info_get(ZXIC_UINT32 reg_no);

/***********************************************************/
/** 通用寄存器写函数
* @param   dev_id  设备号，支持多芯片
* @param   reg_no 寄存器编号
* @param   m_offset 二元寄存器的m偏移
* @param   n_offset 一元寄存器或二元寄存器的n偏移
* @param   p_data 数据指针
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_reg_write(DPP_DEV_T *dev, ZXIC_UINT32 reg_no, ZXIC_UINT32 m_offset, ZXIC_UINT32 n_offset, ZXIC_VOID *p_data);

/***********************************************************/
/** 通用寄存器读函数
* @param   dev_id 设备号，支持多芯片
* @param   reg_no 寄存器编号
* @param   m_offset 二元寄存器的m偏移
* @param   n_offset 一元寄存器或二元寄存器的n偏移
* @param   p_data 数据指针
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_reg_read(DPP_DEV_T *dev, ZXIC_UINT32 reg_no, ZXIC_UINT32 m_offset, ZXIC_UINT32 n_offset, ZXIC_VOID *p_data);

/***********************************************************/
/** 根据寄存器编号获得寄存器芯片内绝对地址
* @param   reg_no
* @param   m_offset
* @param   n_offset
*
* @return
* @remark  无
* @see
************************************************************/
ZXIC_UINT32 dpp_reg_get_reg_addr(ZXIC_UINT32 reg_no, ZXIC_UINT32 m_offset, ZXIC_UINT32 n_offset);

/***********************************************************/
/** 通过寄存器编号配置寄存器，仅适用于32bit位宽
    的常规寄存器
* @param   dev_id  设备号
* @param   reg_no     寄存器编号
* @param   m_offset   二元寄存器的m偏移
* @param   n_offset   一元寄存器或二元寄存器的n偏移
* @param   data       数据，32bit
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
************************************************************/
DPP_STATUS dpp_reg_write32(DPP_DEV_T *dev, ZXIC_UINT32 reg_no, ZXIC_UINT32 data);

/***********************************************************/
/** 通过寄存器编号读取寄存器的值，仅适用于32bit位宽的常规寄存器
* @param   dev_id   设备号
* @param   reg_no   寄存器编号
* @param   m_offset   二元寄存器的m偏移
* @param   n_offset   一元寄存器或二元寄存器的n偏移
* @param   p_data   出参，返回读取寄存器数值
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  石金锋      @date  2015/03/09
************************************************************/
DPP_STATUS dpp_reg_read32(DPP_DEV_T *dev, ZXIC_UINT32 reg_no, ZXIC_UINT32 m_offset, ZXIC_UINT32 n_offset, ZXIC_UINT32 *p_data);
/***********************************************************/
/** 判断是否为4K寄存器
* @param   reg_module
*
* @return  
* @remark  无
* @see
* @author  cq      @date  2023/11/29
************************************************************/
BOOLEAN dpp_4k_reg(ZXIC_UINT32 reg_module);
/***********************************************************/
/** 获取NP对应模块的映射地址偏移(riscv或者非4K寄存器不做转换，host根据映射情况做转换)
* @param   dev_id
* @param   reg_module
* @param   flags  标志位，DPP_REG_FLAG_INDIRECT DPP_REG_FLAG_DIRECT
* @param   addr
*
* @return  映射地址
* @remark  无
* @see
* @author  cq      @date  2023/11/29
************************************************************/
ZXIC_UINT32 dpp_reg_addr_convert(ZXIC_UINT32 dev_id, ZXIC_UINT32 reg_module,ZXIC_UINT32 flags, ZXIC_UINT32 addr);

#ifdef __cplusplus
}
#endif

#endif

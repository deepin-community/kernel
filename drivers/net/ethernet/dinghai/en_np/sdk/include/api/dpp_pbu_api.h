/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_pbu_api.h
* 文件标识 : pbu模块对外数据类型定义和接口函数声明
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : djf
* 完成日期 : 2015/02/04
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/

#ifndef  _DPP_PBU_API_H_
#define _DPP_PBU_API_H_

#include "dpp_module.h"

#if ZXIC_REAL("data struct define")
typedef struct dpp_pbu_mc_cos_para_t
{
    ZXIC_UINT32 mc_cos_th[8];   /**<  @brief cos0~7的mc指针阈值*/
    ZXIC_UINT32 mc_cos_mode[8]; /**<  @brief cos0~7的mc指针溢出处理模式*/
} DPP_PBU_MC_COS_PARA_T;


typedef struct dpp_pbu_port_th_para_t
{
    ZXIC_UINT32 lif_th; /**<  @brief lif阈值*/
    ZXIC_UINT32 lif_prv; /**<  @brief lif私有阈值*/
    ZXIC_UINT32 idma_prv; /**<  @brief idma私有阈值*/
    ZXIC_UINT32 idma_th_cos0; /**<  @brief idma cos0指针阈值*/
    ZXIC_UINT32 idma_th_cos1; /**<  @brief idma cos1指针阈值*/
    ZXIC_UINT32 idma_th_cos2; /**<  @brief idma cos2指针阈值*/
    ZXIC_UINT32 idma_th_cos3; /**<  @brief idma cos3指针阈值*/
    ZXIC_UINT32 idma_th_cos4; /**<  @brief idma cos4指针阈值*/
    ZXIC_UINT32 idma_th_cos5; /**<  @brief idma cos5指针阈值*/
    ZXIC_UINT32 idma_th_cos6; /**<  @brief idma cos6指针阈值*/
    ZXIC_UINT32 idma_th_cos7; /**<  @brief idma cos7指针阈值*/
} DPP_PBU_PORT_TH_PARA_T;

typedef struct dpp_pbu_port_cos_th_para_t
{
    ZXIC_UINT32 cos_th[8];  /**< @brief 各cos对应的阈值，要求高优先级的阈值不小于低优先级的阈值 */
} DPP_PBU_PORT_COS_TH_PARA_T;


typedef struct dpp_pbu_global_th_t
{
    ZXIC_UINT32 idma_public_th;/**<  @brief idma总的共有指针*/
    ZXIC_UINT32 lif_public_th;/**<  @brief lif总的共有指针*/
    ZXIC_UINT32 idma_total_th;/**<  @brief idma总的指针阈值，最大支持16384*/
    ZXIC_UINT32 lif_total_th;/**<  @brief lif总的指针阈值，最大支持16384*/
    ZXIC_UINT32 mc_total_th;/**<  @brief 组播总的指针阈值*/
} DPP_PBU_GLOBAL_TH_T;






#endif//struct


#if ZXIC_REAL("function declaration")
#if 0
/***********************************************************/
/** 全局公共使用指针阈值总体配置
* @param   dev_id  芯片ID 0~3
* @param   p_global_th   参见DPP_PBU_GLOBAL_TH_T详细定义
*
* @return
* @remark  无
* @see
* @author  pj      @date  2019/12/04
************************************************************/
DPP_STATUS dpp_pbu_global_th_set(ZXIC_UINT32 dev_id,
                                 DPP_PBU_GLOBAL_TH_T *p_global_th);

#endif
/***********************************************************/
/** 配置基于端口的指针阈值
* @param   dev_id   设备编号
* @param   port_id   端口号
* @param   p_para   端口阈值
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/07/09
************************************************************/
DPP_STATUS dpp_pbu_port_th_set(DPP_DEV_T *dev,
                               ZXIC_UINT32 port_id,
                               DPP_PBU_PORT_TH_PARA_T *p_para);

/***********************************************************/
/** 配置指定端口按cos优先级起pfc流控的优先级流控指针阈值，仅对lif0的48个通道有效
* @param   dev_id   设备编号
* @param   port_id   端口号
* @param   p_para   cos阈值，要求高优先级的阈值不小于低优先级的阈值
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/07/09
************************************************************/
DPP_STATUS dpp_pbu_port_cos_th_set(DPP_DEV_T *dev,
                                   ZXIC_UINT32 port_id,
                                   DPP_PBU_PORT_COS_TH_PARA_T *p_para);

#if 0
/***********************************************************/
/** pbu初始化函数
* @param   dev_id   设备编号
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/07/09
************************************************************/
DPP_STATUS dpp_pbu_init(ZXIC_UINT32 dev_id);

/***********************************************************/
/** 配置基于cos的mc复制指针阈值和mc指针溢出处理模式
* @param   dev_id   设备编号
* @param   p_para   cos0~7的mc指针阈值和mc指针溢出处理模式 0-wait 1-disc
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/14
************************************************************/
DPP_STATUS dpp_pbu_mc_cos_para_set(ZXIC_UINT32 dev_id,
                                   DPP_PBU_MC_COS_PARA_T *p_para);
#endif
#endif//function
#endif



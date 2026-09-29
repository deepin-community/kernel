#ifndef  _DPP_DRV_FC_H_
#define  _DPP_DRV_FC_H_

#include "zxic_common.h"
#include "dpp_pbu.h"
#include "dpp_pbu_api.h"
#include "dpp_drv_qos.h"

/***********************************************************/
/**对外接口  配置基于vport的端口指针阈值
* @param   vport_id--vport号
* @param   port_id   端口号
* @param   p_para   端口阈值
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_port_th_set(DPP_PF_INFO_T* pf_info,
                           ZXIC_UINT32 port_id,
                           DPP_PBU_PORT_TH_PARA_T *p_para);

/***********************************************************/
/** 读取端口的阈值
* @param   vport_id--vport号
* @param   port_id   端口号
* @param   p_para   端口阈值
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_port_th_get(DPP_PF_INFO_T* pf_info,
                           ZXIC_UINT32 port_id,
                           DPP_PBU_PORT_TH_PARA_T *p_para);

/***********************************************************/
/**对外接口  配置基于vport的端口指定端口按cos优先级起pfc流控的优先级流控指针阈值
* @param   vport_id--vport号
* @param   port_id   端口号
* @param   p_para   cos阈值，要求高优先级的阈值不小于低优先级的阈值
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_port_cos_th_set(DPP_PF_INFO_T* pf_info,
                               ZXIC_UINT32 port_id,
                               DPP_PBU_PORT_COS_TH_PARA_T *p_para);

/***********************************************************/
/** 读取指定端口中各cos的优先级流控指针阈值，仅对lif0的48个通道有效
* @param   vport_id--vport号
* @param   port_id   端口号
* @param   p_para   cos阈值，要求高优先级的阈值不小于低优先级的阈值
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_port_cos_th_get(DPP_PF_INFO_T* pf_info,
                                   ZXIC_UINT32 port_id,
                                   DPP_PBU_PORT_COS_TH_PARA_T *p_para);

/***********************************************************/
/**对外接口  配置PFC防抖延时时间
* @param   pf_info   PF信息
* @param   delayTime 延时时间（单位ns）
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_pfc_delay_time_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT64 delayTime);

/***********************************************************/
/**对外接口  获取PFC防抖延时时间
* @param   pf_info   PF信息
* @param   delayTime 延时时间（单位ns）
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_pfc_delay_time_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT64* delayTime);

#endif

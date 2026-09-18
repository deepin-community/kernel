#include "dpp_drv_fc.h"

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
                           DPP_PBU_PORT_TH_PARA_T *p_para)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(port_id, 0, DPP_TM_PP_NUM - 1);
    ZXIC_COMM_CHECK_POINT(p_para);

    ret = dpp_pbu_port_th_set(&dev, port_id, p_para);
    ZXIC_COMM_CHECK_RC(ret, "dpp_pbu_port_th_set");

    return ret;
}
EXPORT_SYMBOL(dpp_port_th_set);

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
                           DPP_PBU_PORT_TH_PARA_T *p_para)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(port_id, 0, DPP_TM_PP_NUM - 1);
    ZXIC_COMM_CHECK_POINT(p_para);

    ret = dpp_pbu_port_th_get(&dev, port_id, p_para);
    ZXIC_COMM_CHECK_RC(ret, "dpp_pbu_port_th_get");

    return ret;
}
EXPORT_SYMBOL(dpp_port_th_get);

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
                               DPP_PBU_PORT_COS_TH_PARA_T *p_para)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(port_id, 0, DPP_TM_PP_NUM - 1);
    ZXIC_COMM_CHECK_POINT(p_para);

    ret = dpp_pbu_port_cos_th_set(&dev, port_id, p_para);
    ZXIC_COMM_CHECK_RC(ret, "dpp_pbu_port_cos_th_set");

    return ret;
}
EXPORT_SYMBOL(dpp_port_cos_th_set);

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
                               DPP_PBU_PORT_COS_TH_PARA_T *p_para)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);
    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(port_id, 0, DPP_TM_PP_NUM - 1);
    ZXIC_COMM_CHECK_POINT(p_para);

    ret = dpp_pbu_port_cos_th_get(&dev, port_id, p_para);
    ZXIC_COMM_CHECK_RC(ret, "dpp_pbu_port_cos_th_get");

    return ret;
}
EXPORT_SYMBOL(dpp_port_cos_th_get);

/***********************************************************/
/**对外接口  配置PFC防抖延时时间
* @param   pf_info   PF信息
* @param   delayTime 延时时间（单位ns）
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_pfc_delay_time_set(DPP_PF_INFO_T* pf_info, ZXIC_UINT64 delayTime)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);

    ret = dpp_pbu_pfc_delay_time_set(&dev, delayTime);
    ZXIC_COMM_CHECK_RC(ret, "dpp_pbu_pfc_delay_time_set");

    return ret;
}
EXPORT_SYMBOL(dpp_pfc_delay_time_set);

/***********************************************************/
/**对外接口  获取PFC防抖延时时间
* @param   pf_info   PF信息
* @param   delayTime 延时时间（单位ns）
* @return
* @remark  无
* @see
* @author  sun      @date  2023/11/17
************************************************************/
DPP_STATUS dpp_pfc_delay_time_get(DPP_PF_INFO_T* pf_info, ZXIC_UINT64* delayTime)
{
    DPP_STATUS ret     = DPP_OK;
    DPP_DEV_T dev      = {0};

    ZXIC_COMM_CHECK_POINT(pf_info);
    ZXIC_COMM_CHECK_POINT(delayTime);

    ret = dpp_dev_get(pf_info, &dev);
    ZXIC_COMM_CHECK_RC(ret, "dpp_dev_get");

    ZXIC_COMM_CHECK_INDEX_NO_ASSERT(pf_info->vport, 0, DPP_VPORT_NUM_MAX);

    ret = dpp_pbu_pfc_delay_time_get(&dev, delayTime);
    ZXIC_COMM_CHECK_RC(ret, "dpp_pbu_pfc_delay_time_get");

    return ret;
}
EXPORT_SYMBOL(dpp_pfc_delay_time_get);
/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_pbu.c
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : djf
* 完成日期 : 2014/04/14
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/

/******************************************************************************
 *                       START: 头文件                       *
 *****************************************************************************/
#include "zxic_common.h"
#include "dpp_type_api.h"
#include "dpp_reg.h"
#include "dpp_pbu_api.h"
#include "dpp_pbu.h"
#include "dpp_dev.h"
/******************************************************************************
 *                       END: 头文件                       *
 *****************************************************************************/


/******************************************************************************
 *                       START: 常量定义                       *
 *****************************************************************************/

/******************************************************************************
 *                       END: 常量定义                       *
 *****************************************************************************/
#define MF_MAX_BIT (4095)
#define MF_START_BIT (2047)
#define PKT_MAX_BIT (2047)
#define CAP_MAX_NUM (64)
#define PKT_BUFF_NUM (128)
#define PKT_BUF_SIZE (32)


/***********************************************************/
/** 配置端口的阈值
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
                               DPP_PBU_PORT_TH_PARA_T *p_para)
{
    DPP_STATUS  rc = 0;
    ZXIC_UINT32 *p_data = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), port_id, 0, DPP_PBU_PORT_NUM - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_para);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_para->lif_th, 0, DPP_PBU_PORT_TH_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_para->lif_prv, 0, DPP_PBU_PORT_TH_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_para->idma_prv, 0, DPP_PBU_PORT_TH_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_para->idma_th_cos7, 0, DPP_PBU_PORT_TH_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_para->idma_th_cos6, 0, p_para->idma_th_cos7);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_para->idma_th_cos5, 0, p_para->idma_th_cos6);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_para->idma_th_cos4, 0, p_para->idma_th_cos5);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_para->idma_th_cos3, 0, p_para->idma_th_cos4);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_para->idma_th_cos2, 0, p_para->idma_th_cos3);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_para->idma_th_cos1, 0, p_para->idma_th_cos2);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_para->idma_th_cos0, 0, p_para->idma_th_cos1);


    p_data = (ZXIC_UINT32*)p_para;

    rc = dpp_reg_write(dev,
                       NPPU_PBU_CFG_MEMID_0_PBU_FC_IDMATH_RAMr,
                       0,
                       port_id,
                       p_data);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/** 读取端口的阈值
* @param   dev_id   设备编号
* @param   port_id   端口号
* @param   p_para   端口阈值
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/07/09
************************************************************/
DPP_STATUS dpp_pbu_port_th_get(DPP_DEV_T *dev,
                               ZXIC_UINT32 port_id,
                               DPP_PBU_PORT_TH_PARA_T *p_para)
{
    DPP_STATUS  rc = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), port_id, 0, DPP_PBU_PORT_NUM - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_para);

    rc = dpp_reg_read(dev, NPPU_PBU_CFG_MEMID_0_PBU_FC_IDMATH_RAMr, 0, port_id, (ZXIC_UINT32*)p_para);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");


    return DPP_OK;
}

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
                                   DPP_PBU_PORT_COS_TH_PARA_T *p_para)
{
    DPP_STATUS  rc = 0;
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 tmp_index = 0;
    ZXIC_UINT32 *p_data = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), port_id, 0, DPP_PBU_PORT_NUM - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_para);

    if (port_id < DPP_PBU_LIF1_PORT_NUM)
    {
        tmp_index = port_id;

    }
    else if (port_id == DPP_PBU_TM_LOOP_PORT_NUM)
    {
        tmp_index = 56;

    }
    else
    {
        ZXIC_COMM_TRACE_DEV_ERROR(DEV_ID(dev), "dpp_pbu_port_cos_th_set: please check input port:%d !!!!!!\n", port_id);
        return DPP_OK;
    }

    for (i = 0; i < DPP_PBU_COS_NUM; i++)
    {
        if (0 == i)
        {
            ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_para->cos_th[i], 0, DPP_PBU_PORT_COS_MAX_TH );
        }
        else
        {
            ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_para->cos_th[i], p_para->cos_th[i - 1], DPP_PBU_PORT_COS_MAX_TH);
        }
    }

    p_data = (ZXIC_UINT32*)p_para;

    rc = dpp_reg_write(dev, NPPU_PBU_CFG_MEMID_1_PBU_FC_MACTH_RAMr, 0, tmp_index, p_data );
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}


/***********************************************************/
/** 读取指定端口中各cos的优先级流控指针阈值，仅对lif0的48个通道有效
* @param   dev_id   设备编号
* @param   port_id   端口号
* @param   p_para   cos阈值，要求高优先级的阈值不小于低优先级的阈值
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/07/09
************************************************************/
DPP_STATUS dpp_pbu_port_cos_th_get(DPP_DEV_T *dev,
                                   ZXIC_UINT32 port_id,
                                   DPP_PBU_PORT_COS_TH_PARA_T *p_para)
{
    DPP_STATUS  rc = 0;
    ZXIC_UINT32 tmp_index = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), port_id, 0, DPP_PBU_PORT_NUM - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_para);

    if (port_id < DPP_PBU_LIF1_PORT_NUM)
    {
        tmp_index = port_id;

    }
    else if (port_id == DPP_PBU_TM_LOOP_PORT_NUM)
    {
        tmp_index = 56;

    }
    else
    {
        return DPP_OK;
    }

    rc = dpp_reg_read(dev, NPPU_PBU_CFG_MEMID_1_PBU_FC_MACTH_RAMr, 0, tmp_index, (ZXIC_UINT32*)p_para);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    return DPP_OK;
}

/***********************************************************/
/**
* @param   dev_id     设备编号
* @param   delayTime  延时时间
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_pbu_pfc_delay_time_set(DPP_DEV_T *dev, ZXIC_UINT64 delayTime)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_NPPU_PBU_CFG_CFG_PFC_RDY_HIGH_TIME_T  pbuDelayHighTime = {0};
    DPP_NPPU_PBU_CFG_CFG_PFC_RDY_LOW_TIME_T   pbuDelayLowTime  = {0};

    ZXIC_COMM_CHECK_POINT(dev);

    pbuDelayHighTime.cfg_pfc_rdy_high_time = (ZXIC_UINT32)(delayTime >> 32);
    pbuDelayLowTime.cfg_pfc_rdy_low_time   = (ZXIC_UINT32)(delayTime & 0x00000000FFFFFFFF);

    rc  = dpp_reg_write(dev,
                       NPPU_PBU_CFG_CFG_PFC_RDY_HIGH_TIMEr,
                       0,
                       0,
                       &pbuDelayHighTime);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    rc  = dpp_reg_write(dev,
                       NPPU_PBU_CFG_CFG_PFC_RDY_LOW_TIMEr,
                       0,
                       0,
                       &pbuDelayLowTime);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return DPP_OK;
}

/***********************************************************/
/**
* @param   dev_id    设备编号
* @param   delayTime 延时时间
*
* @return  DPP_OK-成功，DPP_ERR-失败
* @remark  无
* @see
* @author  djf      @date  2014/04/09
************************************************************/
DPP_STATUS dpp_pbu_pfc_delay_time_get(DPP_DEV_T *dev, ZXIC_UINT64 *delayTime)
{
    DPP_STATUS  rc = DPP_OK;
    DPP_NPPU_PBU_CFG_CFG_PFC_RDY_HIGH_TIME_T  pbuDelayHighTime = {0};
    DPP_NPPU_PBU_CFG_CFG_PFC_RDY_LOW_TIME_T   pbuDelayLowTime  = {0};

    ZXIC_COMM_CHECK_POINT(delayTime);
    ZXIC_COMM_CHECK_POINT(dev);

    rc  = dpp_reg_read(dev,
                       NPPU_PBU_CFG_CFG_PFC_RDY_HIGH_TIMEr,
                       0,
                       0,
                       &pbuDelayHighTime);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    rc  = dpp_reg_read(dev,
                       NPPU_PBU_CFG_CFG_PFC_RDY_LOW_TIMEr,
                       0,
                       0,
                       &pbuDelayLowTime);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    *delayTime = (((ZXIC_UINT64)pbuDelayHighTime.cfg_pfc_rdy_high_time) << 32) | (ZXIC_UINT64)pbuDelayLowTime.cfg_pfc_rdy_low_time;

    return DPP_OK;
}
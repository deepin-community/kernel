/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_stat_car.c
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : XXX
* 完成日期 : 2017/02/09
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/

#include "zxic_common.h"
#include "dpp_type_api.h"
#include "dpp_reg.h"
// #include "dpp_devmng_api.h"
#include "dpp_stat_car.h"
#include "dpp_stat_api.h"

#if ZXIC_REAL("Global Variable")


/** 软复位相关 */
static DPP_CAR_SOFT_RESET_DATA_T g_car_store_data[DPP_DEV_CHANNEL_MAX] = {{{0}}};

#define GET_DPP_CAR_SOFT_RESET_INFO(dev_id) (&g_car_store_data[dev_id])

#define DPP_CAR1_REG_OFFSET (0)

#endif

#if 0
/***********************************************************/
/** car 模块初始化
* @param   dev_id       设备号
* @param   p_car_cfg    CAR配置信息
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/05/05
************************************************************/
DPP_STATUS dpp_stat_car_init(ZXIC_UINT32 dev_id, DPP_CAR_CFG_T *p_car_cfg)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 car_type = 0;
    ZXIC_UINT32 car_mono_mode = 0;

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_car_cfg);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, p_car_cfg->car0_mono_mode[dev_id], CAR_SMMU0_MONO_MODE_NONE, CAR_SMMU0_MONO_MODE_MAX - 1);
    /* 已经初始化完成，待后添加重新初始化的代码 */
    if (p_car_cfg->is_init[dev_id])
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "Error! CAR cfg has been initialized!\n");
    }
 
    car_mono_mode = p_car_cfg->car0_mono_mode[dev_id];

    for (car_type = STAT_CAR_A_TYPE; car_type < STAT_CAR_MAX_TYPE; car_type++)
    {
        ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "dpp_stat_car_hardware_init(car_type[%d]) begin\n", car_type);
        rc = dpp_stat_car_hardware_init(dev_id,
                                        car_type,
                                        car_mono_mode);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_car_hardware_init");
    }

    p_car_cfg->is_init[dev_id] = 1;

    /* 软复位相关 */
    ZXIC_COMM_MEMSET(&g_car_store_data[dev_id], 0, sizeof(DPP_CAR_SOFT_RESET_DATA_T));
    g_car_store_data[dev_id].is_init = 1;

    return rc;
}
#endif
#if ZXIC_REAL("Basic Reg Operation ")
/***********************************************************/
/** car A的流设置
* @param   dev_id           设备号           car编号
* @param   flow_id          流号
* @param   drop_flag        丢弃标志
* @param   plcr_en          监管使能
* @param   profile_id       监管模板号
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/05
************************************************************/
DPP_STATUS dpp_stat_cara_queue_cfg_set(DPP_DEV_T *dev,
                                       ZXIC_UINT32 flow_id,
                                       ZXIC_UINT32 drop_flag,
                                       ZXIC_UINT32 plcr_en,
                                       ZXIC_UINT32 profile_id)
{
    DPP_STATUS rc = DPP_OK;

    DPP_STAT_CAR0_CARA_QUEUE_RAM0_159_0_T queue_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_A_FLOW_ID_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), drop_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), plcr_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_A_PROFILE_ID_MAX);

    queue_cfg.cara_drop = drop_flag;
    queue_cfg.cara_plcr_en = plcr_en;
    queue_cfg.cara_profile_id = profile_id;

    rc = dpp_reg_write(dev,
                       STAT_CAR0_CARA_QUEUE_RAM0_159_0r ,
                       0,
                       flow_id,
                       &queue_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return rc;
}

/***********************************************************/
/** 获取car A的流设置
* @param   dev_id               设备号
* @param   flow_id              流号
* @param   p_cara_queue_cfg     car A流配置信息
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/05
************************************************************/
DPP_STATUS dpp_stat_cara_queue_cfg_get(DPP_DEV_T *dev,
                                       ZXIC_UINT32 flow_id,
                                       DPP_STAT_CAR_A_QUEUE_CFG_T *p_cara_queue_cfg)
{
    DPP_STATUS rc = DPP_OK;

    DPP_STAT_CAR0_CARA_QUEUE_RAM0_159_0_T  queue_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_A_FLOW_ID_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_cara_queue_cfg);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_CARA_QUEUE_RAM0_159_0r ,
                      0,
                      flow_id,
                      &queue_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    p_cara_queue_cfg->flow_id       = flow_id;
    p_cara_queue_cfg->drop_flag     = queue_cfg.cara_drop;
    p_cara_queue_cfg->plcr_en       = queue_cfg.cara_plcr_en;
    p_cara_queue_cfg->profile_id    = queue_cfg.cara_profile_id;

    p_cara_queue_cfg->tq = ZXIC_COMM_COUNTER64_BUILD(queue_cfg.cara_tq_h, queue_cfg.cara_tq_l);

    p_cara_queue_cfg->ted = queue_cfg.cara_ted;
    p_cara_queue_cfg->tcd = queue_cfg.cara_tcd;
    p_cara_queue_cfg->tei = queue_cfg.cara_tei;
    p_cara_queue_cfg->tci = queue_cfg.cara_tci;

    return rc;
}

/***********************************************************/
/** 获取car A 包长监管流配置
* @param   dev_id               设备号
* @param   flow_id              流号
* @param   p_cara_queue_cfg     car A流配置信息
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/05
************************************************************/
DPP_STATUS dpp_stat_cara_pkt_queue_cfg_get(DPP_DEV_T *dev,
                                           ZXIC_UINT32 flow_id,
                                           DPP_STAT_CAR_A_PKT_QUEUE_CFG_T *p_cara_queue_cfg)
{
    DPP_STATUS rc = DPP_OK;

    DPP_STAT_CAR0_CARA_QUEUE_RAM0_159_0_PKT_T  queue_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_A_FLOW_ID_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_cara_queue_cfg);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_CARA_QUEUE_RAM0_159_0_PKTr ,
                      0,
                      flow_id,
                      &queue_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    p_cara_queue_cfg->flow_id = flow_id;
    p_cara_queue_cfg->plcr_en = queue_cfg.cara_plcr_en;
    p_cara_queue_cfg->profile_id = queue_cfg.cara_profile_id;

    p_cara_queue_cfg->tq = ZXIC_COMM_COUNTER64_BUILD(queue_cfg.cara_tq_h, queue_cfg.cara_tq_l);

    p_cara_queue_cfg->dc = ZXIC_COMM_COUNTER64_BUILD(queue_cfg.cara_dc_high, queue_cfg.cara_dc_low);
    p_cara_queue_cfg->tc = queue_cfg.cara_tc;

    return rc;
}


/***********************************************************/
/** car A 字节限速监管模板设定
* @param   dev_id               设备号
* @param   profile_id           监管模板号
* @param   p_cara_profile_cfg   监管模板配置
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/05
************************************************************/
DPP_STATUS dpp_stat_cara_profile_cfg_set(DPP_DEV_T *dev,
                                         ZXIC_UINT32 profile_id,
                                         DPP_STAT_CAR_PROFILE_CFG_T *p_cara_profile_cfg)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 i = 0;

    DPP_STAT_CAR0_CARA_PROFILE_RAM1_255_0_T profile_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_A_PROFILE_ID_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_cara_profile_cfg);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_cara_profile_cfg->profile_id, 0, DPP_CAR_A_PROFILE_ID_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_cara_profile_cfg->pkt_sign, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_cara_profile_cfg->cf, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_cara_profile_cfg->cm, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_cara_profile_cfg->cd, CAR_CD_MODE_SRTCM, CAR_CD_MODE_INVALID - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_cara_profile_cfg->cir, 0, DPP_CAR_MAX_CIR_VALUE);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_cara_profile_cfg->eir, 0, DPP_CAR_MAX_EIR_VALUE);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_cara_profile_cfg->cbs, 0, DPP_CAR_MAX_CBS_VALUE);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_cara_profile_cfg->ebs, 0, DPP_CAR_MAX_EBS_VALUE);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_cara_profile_cfg->random_disc_e, 0, 0xffffffff);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_cara_profile_cfg->random_disc_c, 0, 0xffffffff);

    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_cara_profile_cfg->e_yellow_pri[0], 0, DPP_CAR_MAX_PRI_VALUE);

    for (i = 1; i < DPP_CAR_PRI_MAX; i++)
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_cara_profile_cfg->c_pri[i],        0, DPP_CAR_MAX_PRI_VALUE);
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_cara_profile_cfg->e_green_pri[i],  0, DPP_CAR_MAX_PRI_VALUE);
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_cara_profile_cfg->e_yellow_pri[i], 0, DPP_CAR_MAX_PRI_VALUE);
    }
    ZXIC_COMM_TRACE_DEV_INFO(DEV_ID(dev), "==> dpp_stat_cara_profile_cfg_set profile_id[%d]: \n",  profile_id);
    ZXIC_COMM_TRACE_DEV_INFO(DEV_ID(dev), "| -------------------------------------------------------------- | \n");
    ZXIC_COMM_TRACE_DEV_INFO(DEV_ID(dev), "| %-5s | %-5s | %-5s | %-10s | %-10s | %-10s | %-10s | \n", "cd", "cf", "cm", "cir", "cbs", "eir", "ebs");
    ZXIC_COMM_TRACE_DEV_INFO(DEV_ID(dev), "| %-5d | %-5d | %-5d | %-10d | %-10d | %-10d | %-10d | \n", p_cara_profile_cfg->cd,
                                                                                                   p_cara_profile_cfg->cf,
                                                                                                   p_cara_profile_cfg->cm,
                                                                                                   p_cara_profile_cfg->cir,
                                                                                                   p_cara_profile_cfg->cbs,
                                                                                                   p_cara_profile_cfg->eir,
                                                                                                   p_cara_profile_cfg->ebs);
    ZXIC_COMM_TRACE_DEV_INFO(DEV_ID(dev), "| ----- | ----- | ----- | ---------- | ---------- | ---------- | ---------- | \n");

    profile_cfg.cara_e_y_pri7 = p_cara_profile_cfg->e_yellow_pri[7];
    profile_cfg.cara_e_y_pri6 = p_cara_profile_cfg->e_yellow_pri[6];
    profile_cfg.cara_e_y_pri5 = p_cara_profile_cfg->e_yellow_pri[5];
    profile_cfg.cara_e_y_pri4 = p_cara_profile_cfg->e_yellow_pri[4];
    profile_cfg.cara_e_y_pri3 = p_cara_profile_cfg->e_yellow_pri[3];
    profile_cfg.cara_e_y_pri2 = p_cara_profile_cfg->e_yellow_pri[2];
    profile_cfg.cara_e_y_pri1 = p_cara_profile_cfg->e_yellow_pri[1];
    profile_cfg.cara_e_y_pri0 = p_cara_profile_cfg->e_yellow_pri[0];

    profile_cfg.cara_e_g_pri7 = p_cara_profile_cfg->e_green_pri[7];
    profile_cfg.cara_e_g_pri6 = p_cara_profile_cfg->e_green_pri[6];
    profile_cfg.cara_e_g_pri5 = p_cara_profile_cfg->e_green_pri[5];
    profile_cfg.cara_e_g_pri4 = p_cara_profile_cfg->e_green_pri[4];
    profile_cfg.cara_e_g_pri3 = p_cara_profile_cfg->e_green_pri[3];
    profile_cfg.cara_e_g_pri2 = p_cara_profile_cfg->e_green_pri[2];
    profile_cfg.cara_e_g_pri1 = p_cara_profile_cfg->e_green_pri[1];

    profile_cfg.cara_c_pri7 = p_cara_profile_cfg->c_pri[7];
    profile_cfg.cara_c_pri6 = p_cara_profile_cfg->c_pri[6];
    profile_cfg.cara_c_pri5 = p_cara_profile_cfg->c_pri[5];
    profile_cfg.cara_c_pri4 = p_cara_profile_cfg->c_pri[4];
    profile_cfg.cara_c_pri3 = p_cara_profile_cfg->c_pri[3];
    profile_cfg.cara_c_pri2 = p_cara_profile_cfg->c_pri[2];
    profile_cfg.cara_c_pri1 = p_cara_profile_cfg->c_pri[1];

    profile_cfg.cara_cbs = p_cara_profile_cfg->cbs;
    profile_cfg.cara_ebs_pbs = p_cara_profile_cfg->ebs;
    profile_cfg.cara_cir = p_cara_profile_cfg->cir;
    profile_cfg.cara_eir = p_cara_profile_cfg->eir;

    profile_cfg.cara_cd = p_cara_profile_cfg->cd;
    profile_cfg.cara_cf = p_cara_profile_cfg->cf;
    profile_cfg.cara_cm = p_cara_profile_cfg->cm;
    profile_cfg.cara_pkt_sign = p_cara_profile_cfg->pkt_sign;

    profile_cfg.cara_profile_wr = 0;

    rc = dpp_reg_write(dev,
                       STAT_CAR0_CARA_PROFILE_RAM1_255_0r ,
                       0,
                       profile_id,
                       &profile_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return rc;
}


/***********************************************************/
/** 获取car A包的监管模板配置
* @param   dev_id               设备号
* @param   profile_id           监管模板号
* @param   p_cara_profile_cfg   监管模板配置
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/05
************************************************************/
DPP_STATUS dpp_stat_cara_profile_cfg_get(DPP_DEV_T *dev,
                                         ZXIC_UINT32 profile_id,
                                         DPP_STAT_CAR_PROFILE_CFG_T *p_cara_profile_cfg)
{
    DPP_STATUS rc = DPP_OK;

    DPP_STAT_CAR0_CARA_PROFILE_RAM1_255_0_T profile_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_A_PROFILE_ID_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_cara_profile_cfg);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_CARA_PROFILE_RAM1_255_0r,
                      0,
                      profile_id,
                      &profile_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    p_cara_profile_cfg->profile_id = profile_id;
    p_cara_profile_cfg->pkt_sign = profile_cfg.cara_pkt_sign;
    p_cara_profile_cfg->cd = profile_cfg.cara_cd;
    p_cara_profile_cfg->cf = profile_cfg.cara_cf;
    p_cara_profile_cfg->cm = profile_cfg.cara_cm;
    p_cara_profile_cfg->eir = profile_cfg.cara_eir;
    p_cara_profile_cfg->cir = profile_cfg.cara_cir;
    p_cara_profile_cfg->ebs = profile_cfg.cara_ebs_pbs;
    p_cara_profile_cfg->cbs = profile_cfg.cara_cbs;

    p_cara_profile_cfg->c_pri[1] = profile_cfg.cara_c_pri1;
    p_cara_profile_cfg->c_pri[2] = profile_cfg.cara_c_pri2;
    p_cara_profile_cfg->c_pri[3] = profile_cfg.cara_c_pri3;
    p_cara_profile_cfg->c_pri[4] = profile_cfg.cara_c_pri4;
    p_cara_profile_cfg->c_pri[5] = profile_cfg.cara_c_pri5;
    p_cara_profile_cfg->c_pri[6] = profile_cfg.cara_c_pri6;
    p_cara_profile_cfg->c_pri[7] = profile_cfg.cara_c_pri7;

    p_cara_profile_cfg->e_green_pri[1] = profile_cfg.cara_e_g_pri1;
    p_cara_profile_cfg->e_green_pri[2] = profile_cfg.cara_e_g_pri2;
    p_cara_profile_cfg->e_green_pri[3] = profile_cfg.cara_e_g_pri3;
    p_cara_profile_cfg->e_green_pri[4] = profile_cfg.cara_e_g_pri4;
    p_cara_profile_cfg->e_green_pri[5] = profile_cfg.cara_e_g_pri5;
    p_cara_profile_cfg->e_green_pri[6] = profile_cfg.cara_e_g_pri6;
    p_cara_profile_cfg->e_green_pri[7] = profile_cfg.cara_e_g_pri7;

    p_cara_profile_cfg->e_yellow_pri[0] = profile_cfg.cara_e_y_pri0;
    p_cara_profile_cfg->e_yellow_pri[1] = profile_cfg.cara_e_y_pri1;
    p_cara_profile_cfg->e_yellow_pri[2] = profile_cfg.cara_e_y_pri2;
    p_cara_profile_cfg->e_yellow_pri[3] = profile_cfg.cara_e_y_pri3;
    p_cara_profile_cfg->e_yellow_pri[4] = profile_cfg.cara_e_y_pri4;
    p_cara_profile_cfg->e_yellow_pri[5] = profile_cfg.cara_e_y_pri5;
    p_cara_profile_cfg->e_yellow_pri[6] = profile_cfg.cara_e_y_pri6;
    p_cara_profile_cfg->e_yellow_pri[7] = profile_cfg.cara_e_y_pri7;

    return rc;
}


/***********************************************************/
/** car A包限速监管模板设定
* @param   dev_id               设备号
* @param   profile_id           监管模板号
* @param   p_cara_profile_cfg   监管模板配置
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/05
************************************************************/
DPP_STATUS dpp_stat_cara_pkt_profile_cfg_set(DPP_DEV_T *dev,
                                             ZXIC_UINT32 profile_id,
                                             DPP_STAT_CAR_PKT_PROFILE_CFG_T *p_cara_profile_cfg)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 i = 0;

    DPP_STAT_CAR0_CARA_PROFILE_RAM1_255_0_PKT_T profile_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_A_PROFILE_ID_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_cara_profile_cfg);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_cara_profile_cfg->pkt_sign, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_cara_profile_cfg->cir, 0, DPP_CAR_MAX_PKT_CIR_VALUE);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_cara_profile_cfg->cbs, 0, DPP_CAR_MAX_PKT_CBS_VALUE);

    for (i = 0; i < DPP_CAR_PRI_MAX; i++)
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_cara_profile_cfg->pri[i], 0, DPP_CAR_MAX_PRI_VALUE);
    }

    profile_cfg.cara_pri7 = p_cara_profile_cfg->pri[7];
    profile_cfg.cara_pri6 = p_cara_profile_cfg->pri[6];
    profile_cfg.cara_pri5 = p_cara_profile_cfg->pri[5];
    profile_cfg.cara_pri4 = p_cara_profile_cfg->pri[4];
    profile_cfg.cara_pri3 = p_cara_profile_cfg->pri[3];
    profile_cfg.cara_pri2 = p_cara_profile_cfg->pri[2];
    profile_cfg.cara_pri1 = p_cara_profile_cfg->pri[1];
    profile_cfg.cara_pri0 = p_cara_profile_cfg->pri[0];

    profile_cfg.cara_pkt_cbs = p_cara_profile_cfg->cbs;
    profile_cfg.cara_pkt_cir = p_cara_profile_cfg->cir;
    profile_cfg.cara_pkt_sign = p_cara_profile_cfg->pkt_sign;

    profile_cfg.cara_profile_wr = 0;

    rc = dpp_reg_write(dev,
                       STAT_CAR0_CARA_PROFILE_RAM1_255_0_PKTr,
                       0,
                       profile_id,
                       &profile_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return rc;
}


/***********************************************************/
/** 获取car A包长监管模板配置
* @param   dev_id               设备号
* @param   profile_id           监管模板号
* @param   p_cara_profile_cfg   监管模板配置
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/05
************************************************************/
DPP_STATUS dpp_stat_cara_pkt_profile_cfg_get(DPP_DEV_T *dev,
                                             ZXIC_UINT32 profile_id,
                                             DPP_STAT_CAR_PKT_PROFILE_CFG_T *p_cara_profile_cfg)
{
    DPP_STATUS rc = DPP_OK;

    DPP_STAT_CAR0_CARA_PROFILE_RAM1_255_0_PKT_T profile_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_A_PROFILE_ID_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_cara_profile_cfg);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_CARA_PROFILE_RAM1_255_0_PKTr ,
                      0,
                      profile_id,
                      &profile_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    p_cara_profile_cfg->profile_id = profile_id;
    p_cara_profile_cfg->pkt_sign = profile_cfg.cara_pkt_sign;
    p_cara_profile_cfg->cir = profile_cfg.cara_pkt_cir;
    p_cara_profile_cfg->cbs = profile_cfg.cara_pkt_cbs;

    p_cara_profile_cfg->pri[0] = profile_cfg.cara_pri0;
    p_cara_profile_cfg->pri[1] = profile_cfg.cara_pri1;
    p_cara_profile_cfg->pri[2] = profile_cfg.cara_pri2;
    p_cara_profile_cfg->pri[3] = profile_cfg.cara_pri3;
    p_cara_profile_cfg->pri[4] = profile_cfg.cara_pri4;
    p_cara_profile_cfg->pri[5] = profile_cfg.cara_pri5;
    p_cara_profile_cfg->pri[6] = profile_cfg.cara_pri6;
    p_cara_profile_cfg->pri[7] = profile_cfg.cara_pri7;

    return rc;
}

/***********************************************************/
/** 配置car A的qvos溢出模式
* @param   dev_id           设备号
* @param   flow_id          流号
* @param   qvos_mode        溢出模式，参见DPP_CAR_QVOS_MODE_E
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_cara_queue_qvos_set(DPP_DEV_T *dev,
                                        ZXIC_UINT32 flow_id,
                                        ZXIC_UINT32 qvos_mode)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARA_QOVS_RAM_RAM2_T qvos_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_A_FLOW_ID_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), qvos_mode, CAR_QVOS_MODE_OVERFLOW_0, CAR_QVOS_MODE_OVERFLOW_MAX - 1);

    qvos_cfg.cara_qovs = qvos_mode;

    rc = dpp_reg_write(dev,
                       STAT_CAR0_CARA_QOVS_RAM_RAM2r ,
                       0,
                       flow_id,
                       &qvos_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return rc;
}

/***********************************************************/
/** 获取car A的qvos溢出模式
* @param   dev_id           设备号
* @param   flow_id          流号
* @param   p_qvos_mode      qvos溢出模式
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_cara_queue_qvos_get(DPP_DEV_T *dev,
                                        ZXIC_UINT32 flow_id,
                                        ZXIC_UINT32 *p_qvos_mode)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARA_QOVS_RAM_RAM2_T qvos_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_A_FLOW_ID_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_qvos_mode);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_CARA_QOVS_RAM_RAM2r ,
                      0,
                      flow_id,
                      &qvos_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    *p_qvos_mode = qvos_cfg.cara_qovs;

    return rc;
}


/***********************************************************/
/** 设置第一级的映射关系
* @param   dev_id           设备号
* @param   flow_id          流号
* @param   map_flow_id      映射的流号
* @param   map_sp           映射的优先级
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_cara_queue_map_set(DPP_DEV_T *dev,
                                       ZXIC_UINT32 flow_id,
                                       ZXIC_UINT32 map_flow_id,
                                       ZXIC_UINT32 map_sp)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_LOOK_UP_TABLE1_T map_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_A_FLOW_ID_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), map_flow_id, 0, DPP_CAR_B_FLOW_ID_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), map_sp, DPP_CAR_PRI0, DPP_CAR_PRI_MAX - 1);

    map_cfg.cara_flow_id = map_flow_id;
    map_cfg.cara_sp = map_sp;

    rc = dpp_reg_write(dev,
                       STAT_CAR0_LOOK_UP_TABLE1r ,
                       0,
                       flow_id,
                       &map_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return rc;
}

/***********************************************************/
/** 获取第一级的映射关系
* @param   dev_id           设备号
* @param   flow_id          流号
* @param   p_map_flow_id    映射流号
* @param   p_map_sp         映射优先级
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_cara_queue_map_get(DPP_DEV_T *dev,
                                       ZXIC_UINT32 flow_id,
                                       ZXIC_UINT32 *p_map_flow_id,
                                       ZXIC_UINT32 *p_map_sp)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_LOOK_UP_TABLE1_T map_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_A_FLOW_ID_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_map_flow_id);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_map_sp);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_LOOK_UP_TABLE1r,
                      0,
                      flow_id,
                      &map_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    *p_map_flow_id = map_cfg.cara_flow_id;
    *p_map_sp = map_cfg.cara_sp;

    return rc;
}

/***********************************************************/
/** car A指定队列模式配置，仅用于调试
* @param   dev_id           设备号
* @param   global_en        全局队列使能，0-不使能，1-使能
* @param   sp_en            优先级队列使能，0-不使能，1-使能
* @param   appoint_sp       指定的优先级
* @param   appoint_queue    指定的队列号
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_cara_queue_appoint_mode_set(DPP_DEV_T *dev,
                                                ZXIC_UINT32 global_en,
                                                ZXIC_UINT32 sp_en,
                                                ZXIC_UINT32 appoint_sp,
                                                ZXIC_UINT32 appoint_queue)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARA_APPOINT_QNUM_OR_SP_T appoint_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), global_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), sp_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), appoint_sp, DPP_CAR_PRI0, DPP_CAR_PRI_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), appoint_queue, 0, DPP_CAR_A_FLOW_ID_MAX);

    appoint_cfg.cara_appoint_qnum_or_not = global_en;
    appoint_cfg.cara_appoint_sp_or_not = sp_en;
    appoint_cfg.cara_plcr_stat_sp = appoint_sp;
    appoint_cfg.cara_plcr_stat_qnum = appoint_queue;

    rc = dpp_reg_write(dev,
                       STAT_CAR0_CARA_APPOINT_QNUM_OR_SPr ,
                       0,
                       0,
                       &appoint_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return rc;
}

/***********************************************************/
/** 获取car A指定队列模式的配置
* @param   dev_id           设备号
* @param   p_global_en      全局队列使能，0-不使能，1-使能
* @param   p_sp_en          优先级队列使能，0-不使能，1-使能
* @param   p_appoint_sp     指定的优先级
* @param   p_appoint_queue  指定的队列号
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/07
************************************************************/
DPP_STATUS dpp_stat_cara_queue_appoint_mode_get(DPP_DEV_T *dev,
                                                ZXIC_UINT32 *p_global_en,
                                                ZXIC_UINT32 *p_sp_en,
                                                ZXIC_UINT32 *p_appoint_sp,
                                                ZXIC_UINT32 *p_appoint_queue)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARA_APPOINT_QNUM_OR_SP_T appoint_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_global_en);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_sp_en);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_appoint_sp);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_appoint_queue);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_CARA_APPOINT_QNUM_OR_SPr,
                      0,
                      0,
                      &appoint_cfg);

    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    *p_global_en = appoint_cfg.cara_appoint_qnum_or_not;
    *p_sp_en = appoint_cfg.cara_appoint_sp_or_not;
    *p_appoint_sp = appoint_cfg.cara_plcr_stat_sp;
    *p_appoint_queue = appoint_cfg.cara_plcr_stat_qnum;

    return rc;
}


/***********************************************************/
/** car A 调试计数读取模式配置
* @param   dev_id           设备号
* @param   overflow_mode    溢出模式，0-计数最大保持，1-计数最大翻转
* @param   rd_mode          读取模式，0-不读清，1-读清模式
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_cara_dbg_cnt_mode_set(DPP_DEV_T *dev,
                                          ZXIC_UINT32 overflow_mode,
                                          ZXIC_UINT32 rd_mode)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARA_CFGMT_COUNT_MODE_T cnt_mode_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), overflow_mode, CAR_KEEP_COUNT, CAR_RE_COUNT);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), rd_mode, CAR_READ_NOT_CLEAR, CAR_READ_AND_CLEAR);

    cnt_mode_cfg.cara_cfgmt_count_overflow_mode = overflow_mode;
    cnt_mode_cfg.cara_cfgmt_count_rd_mode = rd_mode;

    rc = dpp_reg_write(dev,
                       STAT_CAR0_CARA_CFGMT_COUNT_MODEr ,
                       0,
                       0,
                       &cnt_mode_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return rc;
}

/***********************************************************/
/** car A 调试计数读取模式获取
* @param   dev_id           设备号
* @param   p_overflow_mode
* @param   p_rd_mode
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_cara_dbg_cnt_mode_get(DPP_DEV_T *dev,
                                          ZXIC_UINT32 *p_overflow_mode,
                                          ZXIC_UINT32 *p_rd_mode)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARA_CFGMT_COUNT_MODE_T cnt_mode_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_overflow_mode);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_rd_mode);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_CARA_CFGMT_COUNT_MODEr,
                      0,
                      0,
                      &cnt_mode_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    *p_overflow_mode = cnt_mode_cfg.cara_cfgmt_count_overflow_mode;
    *p_rd_mode = cnt_mode_cfg.cara_cfgmt_count_rd_mode;

    return rc;
}

#if 0
/***********************************************************/
/** car a 的调试计数获取
* @param   dev_id
* @param   p_car_dbg_cnt
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/09/27
************************************************************/
DPP_STATUS dpp_stat_cara_dbg_cnt_get(ZXIC_UINT32 dev_id,
                                     DPP_STAT_CAR_DBG_CNT_T *p_car_dbg_cnt)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARA_PKT_DES_I_CNT_T cara_pkt_in_total_cnt_cfg = {0};
    DPP_STAT_CAR0_CARA_PKT_SIZE_CNT_T  cara_pkt_size_cnt_cfg = {0};
    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 size = 0;
    ZXIC_UINT32 *p_tmp_cnt = NULL;

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_car_dbg_cnt);

    size = sizeof(DPP_STAT_CAR_DBG_CNT_T) / sizeof(ZXIC_UINT32) - 1;
    p_tmp_cnt = (ZXIC_UINT32 *)p_car_dbg_cnt;

    for (i = 0; i < size; i++)
    {
        rc = dpp_reg_read(dev_id,
                          STAT_CAR0_CARA_PKT_DES_I_CNTr + i,
                          0,
                          0,
                          &cara_pkt_in_total_cnt_cfg);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_reg_read");

        ZXIC_COMM_MEMCPY(p_tmp_cnt + i, &(cara_pkt_in_total_cnt_cfg.cara_pkt_des_i_cnt), sizeof(ZXIC_UINT32));
    }

    /* STAT_CAR0_CARA_PKT_SIZE_CNTr 不连续，单独读取 */
    rc = dpp_reg_read(dev_id,
                      STAT_CAR0_CARA_PKT_SIZE_CNTr,
                      0,
                      0,
                      &cara_pkt_size_cnt_cfg);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_reg_read");

    p_car_dbg_cnt->pkt_size_cnt = cara_pkt_size_cnt_cfg.cara_pkt_size_cnt;

    return rc;
}

/***********************************************************/
/** 获取car A的初始化状态
* @param   dev_id           设备号
* @param   p_init_done      初始化完成使能
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_cara_init_done_get(ZXIC_UINT32 dev_id,
                                       ZXIC_UINT32 *p_init_done)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARA_PLCR_INIT_DONT_T cara_init_done_cfg = {0};

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_init_done);

    rc = dpp_reg_read(dev_id,
                      STAT_CAR0_CARA_PLCR_INIT_DONTr,
                      0,
                      0,
                      &cara_init_done_cfg);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_reg_read");

    *p_init_done = cara_init_done_cfg.cara_plcr_init_done;

    return rc;
}

#endif
/***********************************************************/
/** car B的流设置
* @param   dev_id           设备号
* @param   flow_id          流号
* @param   drop_flag        丢弃标志
* @param   plcr_en          监管使能
* @param   profile_id       监管模板号
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/05
************************************************************/
DPP_STATUS dpp_stat_carb_queue_cfg_set(DPP_DEV_T *dev,
                                       ZXIC_UINT32 flow_id,
                                       ZXIC_UINT32 drop_flag,
                                       ZXIC_UINT32 plcr_en,
                                       ZXIC_UINT32 profile_id)
{
    DPP_STATUS rc = DPP_OK;

    DPP_STAT_CAR0_CARB_QUEUE_RAM0_159_0_T queue_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_B_FLOW_ID_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), drop_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), plcr_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_B_PROFILE_ID_MAX);

    queue_cfg.carb_drop = drop_flag;
    queue_cfg.carb_plcr_en = plcr_en;
    queue_cfg.carb_profile_id = profile_id;

    rc = dpp_reg_write(dev,
                       STAT_CAR0_CARB_QUEUE_RAM0_159_0r ,
                       0,
                       flow_id,
                       &queue_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return rc;
}

/***********************************************************/
/** 获取car B的流设置
* @param   dev_id               设备号
* @param   flow_id              流号
* @param   p_carb_queue_cfg     car A流配置信息
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/05
************************************************************/
DPP_STATUS dpp_stat_carb_queue_cfg_get(DPP_DEV_T *dev,
                                       ZXIC_UINT32 flow_id,
                                       DPP_STAT_CAR_B_QUEUE_CFG_T *p_carb_queue_cfg)
{
    DPP_STATUS rc = DPP_OK;

    DPP_STAT_CAR0_CARB_QUEUE_RAM0_159_0_T queue_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_B_FLOW_ID_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_carb_queue_cfg);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_CARB_QUEUE_RAM0_159_0r,
                      0,
                      flow_id,
                      &queue_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    p_carb_queue_cfg->flow_id = flow_id;
    p_carb_queue_cfg->drop_flag = queue_cfg.carb_drop;
    p_carb_queue_cfg->plcr_en = queue_cfg.carb_plcr_en;
    p_carb_queue_cfg->profile_id = queue_cfg.carb_profile_id;

    p_carb_queue_cfg->tq = ZXIC_COMM_COUNTER64_BUILD(queue_cfg.carb_tq_h, queue_cfg.carb_tq_l);

    p_carb_queue_cfg->tce_flag = queue_cfg.carb_ted;
    p_carb_queue_cfg->tce = queue_cfg.carb_tcd;
    p_carb_queue_cfg->te = queue_cfg.carb_tei;
    p_carb_queue_cfg->tc = queue_cfg.carb_tci;

    return rc;
}

/***********************************************************/
/** car B监管模板设定
* @param   dev_id               设备号
* @param   profile_id           监管模板号
* @param   p_carb_profile_cfg   监管模板配置
*          pkt_sign 包限速标记写死成0，防止用户在carB包限速
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/05
************************************************************/
DPP_STATUS dpp_stat_carb_profile_cfg_set(DPP_DEV_T *dev,
                                         ZXIC_UINT32 profile_id,
                                         DPP_STAT_CAR_PROFILE_CFG_T *p_carb_profile_cfg)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 i = 0;

    DPP_STAT_CAR0_CARB_PROFILE_RAM1_255_0_T profile_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_B_PROFILE_ID_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_carb_profile_cfg);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carb_profile_cfg->pkt_sign, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carb_profile_cfg->cf, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carb_profile_cfg->cm, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carb_profile_cfg->random_disc_e, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carb_profile_cfg->random_disc_c, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carb_profile_cfg->cd, CAR_CD_MODE_SRTCM, CAR_CD_MODE_INVALID - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carb_profile_cfg->cir, 0, DPP_CAR_MAX_CIR_VALUE);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carb_profile_cfg->eir, 0, DPP_CAR_MAX_EIR_VALUE);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carb_profile_cfg->cbs, 0, DPP_CAR_MAX_CBS_VALUE);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carb_profile_cfg->ebs, 0, DPP_CAR_MAX_EBS_VALUE);

    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carb_profile_cfg->e_yellow_pri[0], 0, DPP_CAR_MAX_PRI_VALUE);

    for (i = 1; i < DPP_CAR_PRI_MAX; i++)
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carb_profile_cfg->c_pri[i], 0, DPP_CAR_MAX_PRI_VALUE);
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carb_profile_cfg->e_green_pri[i], 0, DPP_CAR_MAX_PRI_VALUE);
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carb_profile_cfg->e_yellow_pri[i], 0, DPP_CAR_MAX_PRI_VALUE);
    }

    profile_cfg.carb_e_y_pri7 = p_carb_profile_cfg->e_yellow_pri[7];
    profile_cfg.carb_e_y_pri6 = p_carb_profile_cfg->e_yellow_pri[6];
    profile_cfg.carb_e_y_pri5 = p_carb_profile_cfg->e_yellow_pri[5];
    profile_cfg.carb_e_y_pri4 = p_carb_profile_cfg->e_yellow_pri[4];
    profile_cfg.carb_e_y_pri3 = p_carb_profile_cfg->e_yellow_pri[3];
    profile_cfg.carb_e_y_pri2 = p_carb_profile_cfg->e_yellow_pri[2];
    profile_cfg.carb_e_y_pri1 = p_carb_profile_cfg->e_yellow_pri[1];
    profile_cfg.carb_e_y_pri0 = p_carb_profile_cfg->e_yellow_pri[0];

    profile_cfg.carb_e_g_pri7 = p_carb_profile_cfg->e_green_pri[7];
    profile_cfg.carb_e_g_pri6 = p_carb_profile_cfg->e_green_pri[6];
    profile_cfg.carb_e_g_pri5 = p_carb_profile_cfg->e_green_pri[5];
    profile_cfg.carb_e_g_pri4 = p_carb_profile_cfg->e_green_pri[4];
    profile_cfg.carb_e_g_pri3 = p_carb_profile_cfg->e_green_pri[3];
    profile_cfg.carb_e_g_pri2 = p_carb_profile_cfg->e_green_pri[2];
    profile_cfg.carb_e_g_pri1 = p_carb_profile_cfg->e_green_pri[1];

    profile_cfg.carb_c_pri7 = p_carb_profile_cfg->c_pri[7];
    profile_cfg.carb_c_pri6 = p_carb_profile_cfg->c_pri[6];
    profile_cfg.carb_c_pri5 = p_carb_profile_cfg->c_pri[5];
    profile_cfg.carb_c_pri4 = p_carb_profile_cfg->c_pri[4];
    profile_cfg.carb_c_pri3 = p_carb_profile_cfg->c_pri[3];
    profile_cfg.carb_c_pri2 = p_carb_profile_cfg->c_pri[2];
    profile_cfg.carb_c_pri1 = p_carb_profile_cfg->c_pri[1];

    profile_cfg.carb_cbs = p_carb_profile_cfg->cbs;
    profile_cfg.carb_ebs_pbs = p_carb_profile_cfg->ebs;
    profile_cfg.carb_cir = p_carb_profile_cfg->cir;
    profile_cfg.carb_eir = p_carb_profile_cfg->eir;

    profile_cfg.carb_cd = p_carb_profile_cfg->cd;
    profile_cfg.carb_cf = p_carb_profile_cfg->cf;
    profile_cfg.carb_cm = p_carb_profile_cfg->cm;

    profile_cfg.carb_random_discard_en_e = p_carb_profile_cfg->random_disc_e;
    profile_cfg.carb_random_discard_en_c = p_carb_profile_cfg->random_disc_c;
    /* B级car包限速标记写死为0，以免用户配置成1对CARB进行包限速 */
    profile_cfg.carb_pkt_sign = 0;

    profile_cfg.carb_profile_wr = 0;

    rc = dpp_reg_write(dev,
                       STAT_CAR0_CARB_PROFILE_RAM1_255_0r,
                       0,
                       profile_id,
                       &profile_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return rc;
}


/***********************************************************/
/** 获取car B的监管模板配置
* @param   dev_id               设备号
* @param   profile_id           监管模板号
* @param   p_carb_profile_cfg   监管模板配置
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/05
************************************************************/
DPP_STATUS dpp_stat_carb_profile_cfg_get(DPP_DEV_T *dev,
                                         ZXIC_UINT32 profile_id,
                                         DPP_STAT_CAR_PROFILE_CFG_T *p_carb_profile_cfg)
{
    DPP_STATUS rc = DPP_OK;

    DPP_STAT_CAR0_CARB_PROFILE_RAM1_255_0_T profile_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_B_PROFILE_ID_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_carb_profile_cfg);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_CARB_PROFILE_RAM1_255_0r,
                      0,
                      profile_id,
                      &profile_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    p_carb_profile_cfg->profile_id = profile_id;
    p_carb_profile_cfg->pkt_sign = profile_cfg.carb_pkt_sign;
    p_carb_profile_cfg->cd = profile_cfg.carb_cd;
    p_carb_profile_cfg->cf = profile_cfg.carb_cf;
    p_carb_profile_cfg->cm = profile_cfg.carb_cm;
    p_carb_profile_cfg->eir = profile_cfg.carb_eir;
    p_carb_profile_cfg->cir = profile_cfg.carb_cir;
    p_carb_profile_cfg->ebs = profile_cfg.carb_ebs_pbs;
    p_carb_profile_cfg->cbs = profile_cfg.carb_cbs;
    p_carb_profile_cfg->random_disc_e = profile_cfg.carb_random_discard_en_e;
    p_carb_profile_cfg->random_disc_c = profile_cfg.carb_random_discard_en_c;

    p_carb_profile_cfg->c_pri[1] = profile_cfg.carb_c_pri1;
    p_carb_profile_cfg->c_pri[2] = profile_cfg.carb_c_pri2;
    p_carb_profile_cfg->c_pri[3] = profile_cfg.carb_c_pri3;
    p_carb_profile_cfg->c_pri[4] = profile_cfg.carb_c_pri4;
    p_carb_profile_cfg->c_pri[5] = profile_cfg.carb_c_pri5;
    p_carb_profile_cfg->c_pri[6] = profile_cfg.carb_c_pri6;
    p_carb_profile_cfg->c_pri[7] = profile_cfg.carb_c_pri7;

    p_carb_profile_cfg->e_green_pri[1] = profile_cfg.carb_e_g_pri1;
    p_carb_profile_cfg->e_green_pri[2] = profile_cfg.carb_e_g_pri2;
    p_carb_profile_cfg->e_green_pri[3] = profile_cfg.carb_e_g_pri3;
    p_carb_profile_cfg->e_green_pri[4] = profile_cfg.carb_e_g_pri4;
    p_carb_profile_cfg->e_green_pri[5] = profile_cfg.carb_e_g_pri5;
    p_carb_profile_cfg->e_green_pri[6] = profile_cfg.carb_e_g_pri6;
    p_carb_profile_cfg->e_green_pri[7] = profile_cfg.carb_e_g_pri7;

    p_carb_profile_cfg->e_yellow_pri[0] = profile_cfg.carb_e_y_pri0;
    p_carb_profile_cfg->e_yellow_pri[1] = profile_cfg.carb_e_y_pri1;
    p_carb_profile_cfg->e_yellow_pri[2] = profile_cfg.carb_e_y_pri2;
    p_carb_profile_cfg->e_yellow_pri[3] = profile_cfg.carb_e_y_pri3;
    p_carb_profile_cfg->e_yellow_pri[4] = profile_cfg.carb_e_y_pri4;
    p_carb_profile_cfg->e_yellow_pri[5] = profile_cfg.carb_e_y_pri5;
    p_carb_profile_cfg->e_yellow_pri[6] = profile_cfg.carb_e_y_pri6;
    p_carb_profile_cfg->e_yellow_pri[7] = profile_cfg.carb_e_y_pri7;

    return rc;
}

/***********************************************************/
/** 配置car B的qvos溢出模式
* @param   dev_id           设备号
* @param   flow_id          流号
* @param   qvos_mode        溢出模式，参见DPP_CAR_QVOS_MODE_E
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_carb_queue_qvos_set(DPP_DEV_T *dev,
                                        ZXIC_UINT32 flow_id,
                                        ZXIC_UINT32 qvos_mode)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARB_QOVS_RAM_RAM2_T qvos_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_B_FLOW_ID_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), qvos_mode, CAR_QVOS_MODE_OVERFLOW_0, CAR_QVOS_MODE_OVERFLOW_MAX - 1);

    qvos_cfg.carb_qovs = qvos_mode;

    rc = dpp_reg_write(dev,
                       STAT_CAR0_CARB_QOVS_RAM_RAM2r,
                       0,
                       flow_id,
                       &qvos_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return rc;
}
/***********************************************************/
/** 获取car B的qvos溢出模式
* @param   dev_id           设备号
* @param   flow_id          流号
* @param   p_qvos_mode      qvos溢出模式
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_carb_queue_qvos_get(DPP_DEV_T *dev,
                                        ZXIC_UINT32 flow_id,
                                        ZXIC_UINT32 *p_qvos_mode)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARB_QOVS_RAM_RAM2_T qvos_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_B_FLOW_ID_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_qvos_mode);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_CARB_QOVS_RAM_RAM2r ,
                      0,
                      flow_id,
                      &qvos_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    *p_qvos_mode = qvos_cfg.carb_qovs;

    return rc;
}

/***********************************************************/
/** 设置第二级的映射关系
* @param   dev_id           设备号
* @param   flow_id          流号
* @param   map_flow_id      映射的流号
* @param   map_sp           映射的优先级
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_carb_queue_map_set(DPP_DEV_T *dev,
                                       ZXIC_UINT32 flow_id,
                                       ZXIC_UINT32 map_flow_id,
                                       ZXIC_UINT32 map_sp)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_LOOK_UP_TABLE2_T map_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_B_FLOW_ID_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), map_flow_id, 0, DPP_CAR_C_FLOW_ID_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), map_sp, DPP_CAR_PRI0, DPP_CAR_PRI_MAX - 1);

    map_cfg.carb_flow_id = map_flow_id;
    map_cfg.carb_sp = map_sp;

    rc = dpp_reg_write(dev,
                       STAT_CAR0_LOOK_UP_TABLE2r ,
                       0,
                       flow_id,
                       &map_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return rc;
}

/***********************************************************/
/** 获取第二级的映射关系
* @param   dev_id           设备号
* @param   flow_id          流号
* @param   p_map_flow_id    映射流号
* @param   p_map_sp         映射优先级
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_carb_queue_map_get(DPP_DEV_T *dev,
                                       ZXIC_UINT32 flow_id,
                                       ZXIC_UINT32 *p_map_flow_id,
                                       ZXIC_UINT32 *p_map_sp)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_LOOK_UP_TABLE2_T map_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_B_FLOW_ID_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_map_flow_id);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_map_sp);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_LOOK_UP_TABLE2r ,
                      0,
                      flow_id,
                      &map_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    *p_map_flow_id = map_cfg.carb_flow_id;
    *p_map_sp      = map_cfg.carb_sp;

    return rc;
}

#if 0
/***********************************************************/
/** car B指定队列模式配置，仅用于调试
* @param   dev_id           设备号
* @param   global_en        全局队列使能，0-不使能，1-使能
* @param   sp_en            优先级队列使能，0-不使能，1-使能
* @param   appoint_sp       指定的优先级
* @param   appoint_queue    指定的队列号
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_carb_queue_appoint_mode_set(ZXIC_UINT32 dev_id,
                                                ZXIC_UINT32 global_en,
                                                ZXIC_UINT32 sp_en,
                                                ZXIC_UINT32 appoint_sp,
                                                ZXIC_UINT32 appoint_queue)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARB_APPOINT_QNUM_OR_SP_T appoint_cfg = {0};

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, global_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sp_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, appoint_sp, DPP_CAR_PRI0, DPP_CAR_PRI_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, appoint_queue, 0, DPP_CAR_B_FLOW_ID_MAX);

    appoint_cfg.carb_appoint_qnum_or_not = global_en;
    appoint_cfg.carb_appoint_sp_or_not = sp_en;
    appoint_cfg.carb_plcr_stat_sp = appoint_sp;
    appoint_cfg.carb_plcr_stat_qnum = appoint_queue;

    rc = dpp_reg_write(dev_id,
                       STAT_CAR0_CARB_APPOINT_QNUM_OR_SPr,
                       0,
                       0,
                       &appoint_cfg);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_reg_write");

    return rc;
}

/***********************************************************/
/** 获取car B指定队列模式的配置
* @param   dev_id           设备号
* @param   p_global_en      全局队列使能，0-不使能，1-使能
* @param   p_sp_en          优先级队列使能，0-不使能，1-使能
* @param   p_appoint_sp     指定的优先级
* @param   p_appoint_queue  指定的队列号
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/07
************************************************************/
DPP_STATUS dpp_stat_carb_queue_appoint_mode_get(ZXIC_UINT32 dev_id,
                                                ZXIC_UINT32 *p_global_en,
                                                ZXIC_UINT32 *p_sp_en,
                                                ZXIC_UINT32 *p_appoint_sp,
                                                ZXIC_UINT32 *p_appoint_queue)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARB_APPOINT_QNUM_OR_SP_T appoint_cfg = {0};

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_global_en);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_sp_en);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_appoint_sp);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_appoint_queue);

    rc = dpp_reg_read(dev_id,
                      STAT_CAR0_CARB_APPOINT_QNUM_OR_SPr,
                      0,
                      0,
                      &appoint_cfg);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_reg_read");

    *p_global_en = appoint_cfg.carb_appoint_qnum_or_not;
    *p_sp_en = appoint_cfg.carb_appoint_sp_or_not;
    *p_appoint_sp = appoint_cfg.carb_plcr_stat_sp;
    *p_appoint_queue = appoint_cfg.carb_plcr_stat_qnum;

    return rc;
}

/***********************************************************/
/** car B 调试计数读取模式配置
* @param   dev_id           设备号
* @param   overflow_mode    溢出模式，0-计数最大保持，1-计数最大翻转
* @param   rd_mode          读取模式，0-不读清，1-读清模式
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_carb_dbg_cnt_mode_set(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 overflow_mode,
                                          ZXIC_UINT32 rd_mode)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARB_CFGMT_COUNT_MODE_T cnt_mode_cfg = {0};

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, overflow_mode, CAR_KEEP_COUNT, CAR_RE_COUNT);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, rd_mode, CAR_READ_NOT_CLEAR, CAR_READ_AND_CLEAR);

    cnt_mode_cfg.carb_cfgmt_count_overflow_mode = overflow_mode;
    cnt_mode_cfg.carb_cfgmt_count_rd_mode = rd_mode;

    rc = dpp_reg_write(dev_id,
                       STAT_CAR0_CARB_CFGMT_COUNT_MODEr,
                       0,
                       0,
                       &cnt_mode_cfg);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_reg_write");

    return rc;
}

/***********************************************************/
/**
* @param   dev_id
* @param   p_overflow_mode
* @param   p_rd_mode
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_carb_dbg_cnt_mode_get(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 *p_overflow_mode,
                                          ZXIC_UINT32 *p_rd_mode)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARB_CFGMT_COUNT_MODE_T cnt_mode_cfg = {0};

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_overflow_mode);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_rd_mode);

    rc = dpp_reg_read(dev_id,
                      STAT_CAR0_CARB_CFGMT_COUNT_MODEr,
                      0,
                      0,
                      &cnt_mode_cfg);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_reg_read");

    *p_overflow_mode = cnt_mode_cfg.carb_cfgmt_count_overflow_mode;
    *p_rd_mode = cnt_mode_cfg.carb_cfgmt_count_rd_mode;

    return rc;
}

/***********************************************************/
/** car b 的调试计数获取
* @param   dev_id
* @param   p_car_dbg_cnt
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/09/27
************************************************************/
DPP_STATUS dpp_stat_carb_dbg_cnt_get(ZXIC_UINT32 dev_id,
                                     DPP_STAT_CAR_DBG_CNT_T *p_car_dbg_cnt)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 size = 0;
    ZXIC_UINT32 *p_tmp_cnt = NULL;

    DPP_STAT_CAR0_CARB_PKT_SIZE_CNT_T carb_pkt_size_cnt_cfg = {0};
    DPP_STAT_CAR0_CARB_PKT_DES_I_CNT_T carb_pkt_in_total_cnt_cfg = {0};

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_car_dbg_cnt);

    size = sizeof(DPP_STAT_CAR_DBG_CNT_T) / sizeof(ZXIC_UINT32) - 1;
    p_tmp_cnt = (ZXIC_UINT32 *)p_car_dbg_cnt;

    for (i = 0; i < size; i++)
    {
        rc = dpp_reg_read(dev_id,
                          STAT_CAR0_CARB_PKT_DES_I_CNTr + i,
                          0,
                          0,
                          &carb_pkt_in_total_cnt_cfg);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_reg_read");

        ZXIC_COMM_MEMCPY(p_tmp_cnt + i, &(carb_pkt_in_total_cnt_cfg.carb_pkt_des_i_cnt), sizeof(ZXIC_UINT32));
    }

    rc = dpp_reg_read(dev_id,
                      STAT_CAR0_CARB_PKT_SIZE_CNTr,
                      0,
                      0,
                      &carb_pkt_size_cnt_cfg);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_reg_read");

    p_car_dbg_cnt->pkt_size_cnt = carb_pkt_size_cnt_cfg.carb_pkt_size_cnt;

    return rc;
}


/***********************************************************/
/** 获取car B的初始化状态
* @param   dev_id           设备号           car编号
* @param   p_init_done      初始化完成使能
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_carb_init_done_get(ZXIC_UINT32 dev_id,
                                       ZXIC_UINT32 *p_init_done)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARB_PLCR_INIT_DONT_T carb_init_done_cfg = {0};

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_init_done);

    rc = dpp_reg_read(dev_id,
                      STAT_CAR0_CARB_PLCR_INIT_DONTr ,
                      0,
                      0,
                      &carb_init_done_cfg);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_reg_read");

    *p_init_done = carb_init_done_cfg.carb_plcr_init_done;

    return rc;
}

#endif
/***********************************************************/
/** 
* @param   dev_id
* @param   profile_id
* @param   p_random_ram
*
* @return  
* @remark  无
* @see     
* @author  YXH      @date  2019/04/01
************************************************************/
DPP_STATUS dpp_stat_carb_random_ram_set(DPP_DEV_T *dev,
                                        ZXIC_UINT32 profile_id,
                                        DPP_CAR_RANDOM_RAM_T *p_random_ram_e,
                                        DPP_CAR_RANDOM_RAM_T *p_random_ram_c)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT64 para0_temp = 0;
    ZXIC_UINT64 para2_temp = 0;
    ZXIC_UINT64 para4_temp = 0;
    DPP_STAT_CAR0_CARB_RANDOM_RAM_T carb_random_ram_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_random_ram_e);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_random_ram_e->p1, 0, 100);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_random_ram_e->p2, 0, 100);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_random_ram_e->p3, 0, 100);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_random_ram_c);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_random_ram_c->p1, 0, 100);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_random_ram_c->p2, 0, 100);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_random_ram_c->p3, 0, 100);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_B_PROFILE_ID_RANDOM_MAX);

    para0_temp = ((ZXIC_UINT64)((((ZXIC_UINT64)(p_random_ram_e->t2)) - ((ZXIC_UINT64)(p_random_ram_e->t1))) * ((ZXIC_UINT64)(p_random_ram_e->p1))) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;
    carb_random_ram_cfg.para0_l_e = (para0_temp & 0xFFFFFFFF);
    carb_random_ram_cfg.para0_h_e = (para0_temp >> 32) & 0xFFFFFFFF;

    carb_random_ram_cfg.para1_e = ((p_random_ram_e->p2 - p_random_ram_e->p1) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;

    para2_temp = ((ZXIC_UINT64)((((ZXIC_UINT64)(p_random_ram_e->t3)) - ((ZXIC_UINT64)(p_random_ram_e->t2))) * ((ZXIC_UINT64)(p_random_ram_e->p2))) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;
    carb_random_ram_cfg.para2_l_e = (para2_temp & 0xFFFFFFFF);
    carb_random_ram_cfg.para2_h_e = (para2_temp >> 32) & 0xFFFFFFFF;

    carb_random_ram_cfg.para3_e = ((p_random_ram_e->p3 - p_random_ram_e->p2) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;

    para4_temp = ((ZXIC_UINT64)((((ZXIC_UINT64)(p_random_ram_e->tc)) - ((ZXIC_UINT64)(p_random_ram_e->t3))) * ((ZXIC_UINT64)(p_random_ram_e->p3))) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;
    carb_random_ram_cfg.para4_l_e = (para4_temp & 0xFFFFFFFF);
    carb_random_ram_cfg.para4_h_e = (para4_temp >> 32) & 0xFFFFFFFF;

    carb_random_ram_cfg.para5_e = ((100 - p_random_ram_e->p3) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;
    carb_random_ram_cfg.para6_e = p_random_ram_e->t1;
    carb_random_ram_cfg.para7_e = p_random_ram_e->t2;
    carb_random_ram_cfg.para8_e = p_random_ram_e->t3;

    /* para0_temp = 0;
    para2_temp = 0;
    para4_temp = 0; */

    para0_temp = ((ZXIC_UINT64)((((ZXIC_UINT64)(p_random_ram_c->t2)) - ((ZXIC_UINT64)(p_random_ram_c->t1))) * ((ZXIC_UINT64)(p_random_ram_c->p1))) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;
    carb_random_ram_cfg.para0_l_c = (para0_temp & 0xFFFFFFFF);
    carb_random_ram_cfg.para0_h_c = (para0_temp >> 32) & 0xFFFFFFFF;

    carb_random_ram_cfg.para1_c = ((p_random_ram_c->p2 - p_random_ram_c->p1) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;

    para2_temp = ((ZXIC_UINT64)((((ZXIC_UINT64)(p_random_ram_c->t3)) - ((ZXIC_UINT64)(p_random_ram_c->t2))) * ((ZXIC_UINT64)(p_random_ram_c->p2))) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;
    carb_random_ram_cfg.para2_l_c = (para2_temp & 0xFFFFFFFF);
    carb_random_ram_cfg.para2_h_c = (para2_temp >> 32) & 0xFFFFFFFF;

    carb_random_ram_cfg.para3_c = ((p_random_ram_c->p3 - p_random_ram_c->p2) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;

    para4_temp = ((ZXIC_UINT64)((((ZXIC_UINT64)(p_random_ram_c->tc)) - ((ZXIC_UINT64)(p_random_ram_c->t3))) * ((ZXIC_UINT64)(p_random_ram_c->p3))) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;
    carb_random_ram_cfg.para4_l_c = (para4_temp & 0xFFFFFFFF);
    carb_random_ram_cfg.para4_h_c = (para4_temp >> 32) & 0xFFFFFFFF;

    carb_random_ram_cfg.para5_c = ((100 - p_random_ram_c->p3) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;
    carb_random_ram_cfg.para6_c = p_random_ram_c->t1;
    carb_random_ram_cfg.para7_c = p_random_ram_c->t2;
    carb_random_ram_cfg.para8_c = p_random_ram_c->t3;

    rc = dpp_reg_write(dev,
                       STAT_CAR0_CARB_RANDOM_RAMr ,
                       0,
                       profile_id,
                       &carb_random_ram_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return rc;
}

/***********************************************************/
/** 
* @param   dev_id
* @param   p_random_ram
*
* @return
* @remark  无
* @see
* @author  YXH      @date  2019/04/01
************************************************************/
DPP_STATUS dpp_stat_carb_random_ram_get(DPP_DEV_T *dev,
                                        ZXIC_UINT32 profile_id,
                                        DPP_CAR_RANDOM_RAM_T *p_random_ram_e,
                                        DPP_CAR_RANDOM_RAM_T *p_random_ram_c)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARB_RANDOM_RAM_T carb_random_ram_cfg = {0};
    ZXIC_UINT32 tmp_val = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_random_ram_e);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_random_ram_c);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_B_PROFILE_ID_RANDOM_MAX);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_CARB_RANDOM_RAMr ,
                      0,
                      profile_id,
                      &carb_random_ram_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    p_random_ram_e->t1 = carb_random_ram_cfg.para6_e;
    p_random_ram_e->t2 = carb_random_ram_cfg.para7_e;
    p_random_ram_e->t3 = carb_random_ram_cfg.para8_e;
    tmp_val = (carb_random_ram_cfg.para5_e * 100) >> DPP_CAR_RANDOM_OFFSET_VAL;
    p_random_ram_e->p3 = 100 - tmp_val;
    tmp_val = (carb_random_ram_cfg.para3_e * 100) >> DPP_CAR_RANDOM_OFFSET_VAL;
    p_random_ram_e->p2 = p_random_ram_e->p3 - tmp_val;
    tmp_val = (carb_random_ram_cfg.para1_e * 100) >> DPP_CAR_RANDOM_OFFSET_VAL;
    p_random_ram_e->p1 = p_random_ram_e->p2 - tmp_val;

    p_random_ram_c->t1 = carb_random_ram_cfg.para6_c;
    p_random_ram_c->t2 = carb_random_ram_cfg.para7_c;
    p_random_ram_c->t3 = carb_random_ram_cfg.para8_c;
    tmp_val = (carb_random_ram_cfg.para5_c * 100) >> DPP_CAR_RANDOM_OFFSET_VAL;
    p_random_ram_c->p3 = 100 - tmp_val;
    tmp_val = (carb_random_ram_cfg.para3_c * 100) >> DPP_CAR_RANDOM_OFFSET_VAL;
    p_random_ram_c->p2 = p_random_ram_c->p3 - tmp_val;
    tmp_val = (carb_random_ram_cfg.para1_c * 100) >> DPP_CAR_RANDOM_OFFSET_VAL;
    p_random_ram_c->p1 = p_random_ram_c->p2 - tmp_val;

    return rc;
}

/***********************************************************/
/** car C的流设置
* @param   dev_id           设备号           car编号
* @param   flow_id          流号
* @param   drop_flag        丢弃标志
* @param   plcr_en          监管使能
* @param   profile_id       监管模板号
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/05
************************************************************/
DPP_STATUS dpp_stat_carc_queue_cfg_set(DPP_DEV_T *dev,
                                       ZXIC_UINT32 flow_id,
                                       ZXIC_UINT32 drop_flag,
                                       ZXIC_UINT32 plcr_en,
                                       ZXIC_UINT32 profile_id)
{
    DPP_STATUS rc = DPP_OK;

    DPP_STAT_CAR0_CARC_QUEUE_RAM0_159_0_T queue_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_C_FLOW_ID_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), drop_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), plcr_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_C_PROFILE_ID_MAX);

    queue_cfg.carc_drop = drop_flag;
    queue_cfg.carc_plcr_en = plcr_en;
    queue_cfg.carc_profile_id = profile_id;

    rc = dpp_reg_write(dev,
                       STAT_CAR0_CARC_QUEUE_RAM0_159_0r ,
                       0,
                       flow_id,
                       &queue_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return rc;
}

/***********************************************************/
/** 获取car C的流设置
* @param   dev_id               设备号               car编号
* @param   flow_id              流号
* @param   p_carc_queue_cfg     car c流配置信息
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/05
************************************************************/
DPP_STATUS dpp_stat_carc_queue_cfg_get(DPP_DEV_T *dev,
                                       ZXIC_UINT32 flow_id,
                                       DPP_STAT_CAR_C_QUEUE_CFG_T *p_carc_queue_cfg)
{
    DPP_STATUS rc = DPP_OK;

    DPP_STAT_CAR0_CARC_QUEUE_RAM0_159_0_T queue_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_C_FLOW_ID_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_carc_queue_cfg);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_CARC_QUEUE_RAM0_159_0r,
                      0,
                      flow_id,
                      &queue_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    p_carc_queue_cfg->flow_id = flow_id;
    p_carc_queue_cfg->drop_flag = queue_cfg.carc_drop;
    p_carc_queue_cfg->plcr_en = queue_cfg.carc_plcr_en;
    p_carc_queue_cfg->profile_id = queue_cfg.carc_profile_id;

    p_carc_queue_cfg->tq = ZXIC_COMM_COUNTER64_BUILD(queue_cfg.carc_tq_h, queue_cfg.carc_tq_l);

    p_carc_queue_cfg->tce_flag = queue_cfg.carc_ted;
    p_carc_queue_cfg->tce = queue_cfg.carc_tcd;
    p_carc_queue_cfg->te = queue_cfg.carc_tei;
    p_carc_queue_cfg->tc = queue_cfg.carc_tci;

    return rc;
}

/***********************************************************/
/** car C监管模板设定
* @param   dev_id               设备号               car编号
* @param   profile_id           监管模板号
* @param   p_carc_profile_cfg   监管模板配置
*          pkt_sign 包限速标记写死成0，防止用户在carC包限速
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/05
************************************************************/
DPP_STATUS dpp_stat_carc_profile_cfg_set(DPP_DEV_T *dev,
                                         ZXIC_UINT32 profile_id,
                                         DPP_STAT_CAR_PROFILE_CFG_T *p_carc_profile_cfg)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 i = 0;

    DPP_STAT_CAR0_CARC_PROFILE_RAM1_255_0_T profile_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_C_PROFILE_ID_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_carc_profile_cfg);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carc_profile_cfg->pkt_sign, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carc_profile_cfg->cf, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carc_profile_cfg->cm, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carc_profile_cfg->random_disc_e, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carc_profile_cfg->random_disc_c, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carc_profile_cfg->cd, CAR_CD_MODE_SRTCM, CAR_CD_MODE_INVALID - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carc_profile_cfg->cir, 0, DPP_CAR_MAX_CIR_VALUE);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carc_profile_cfg->eir, 0, DPP_CAR_MAX_EIR_VALUE);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carc_profile_cfg->cbs, 0, DPP_CAR_MAX_CBS_VALUE);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carc_profile_cfg->ebs, 0, DPP_CAR_MAX_EBS_VALUE);

    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carc_profile_cfg->e_yellow_pri[0], 0, DPP_CAR_MAX_PRI_VALUE);

    for (i = 1; i < DPP_CAR_PRI_MAX; i++)
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carc_profile_cfg->c_pri[i], 0, DPP_CAR_MAX_PRI_VALUE);
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carc_profile_cfg->e_green_pri[i], 0, DPP_CAR_MAX_PRI_VALUE);
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_carc_profile_cfg->e_yellow_pri[i], 0, DPP_CAR_MAX_PRI_VALUE);
    }

    profile_cfg.carc_e_y_pri7 = p_carc_profile_cfg->e_yellow_pri[7];
    profile_cfg.carc_e_y_pri6 = p_carc_profile_cfg->e_yellow_pri[6];
    profile_cfg.carc_e_y_pri5 = p_carc_profile_cfg->e_yellow_pri[5];
    profile_cfg.carc_e_y_pri4 = p_carc_profile_cfg->e_yellow_pri[4];
    profile_cfg.carc_e_y_pri3 = p_carc_profile_cfg->e_yellow_pri[3];
    profile_cfg.carc_e_y_pri2 = p_carc_profile_cfg->e_yellow_pri[2];
    profile_cfg.carc_e_y_pri1 = p_carc_profile_cfg->e_yellow_pri[1];
    profile_cfg.carc_e_y_pri0 = p_carc_profile_cfg->e_yellow_pri[0];

    profile_cfg.carc_e_g_pri7 = p_carc_profile_cfg->e_green_pri[7];
    profile_cfg.carc_e_g_pri6 = p_carc_profile_cfg->e_green_pri[6];
    profile_cfg.carc_e_g_pri5 = p_carc_profile_cfg->e_green_pri[5];
    profile_cfg.carc_e_g_pri4 = p_carc_profile_cfg->e_green_pri[4];
    profile_cfg.carc_e_g_pri3 = p_carc_profile_cfg->e_green_pri[3];
    profile_cfg.carc_e_g_pri2 = p_carc_profile_cfg->e_green_pri[2];
    profile_cfg.carc_e_g_pri1 = p_carc_profile_cfg->e_green_pri[1];

    profile_cfg.carc_c_pri7 = p_carc_profile_cfg->c_pri[7];
    profile_cfg.carc_c_pri6 = p_carc_profile_cfg->c_pri[6];
    profile_cfg.carc_c_pri5 = p_carc_profile_cfg->c_pri[5];
    profile_cfg.carc_c_pri4 = p_carc_profile_cfg->c_pri[4];
    profile_cfg.carc_c_pri3 = p_carc_profile_cfg->c_pri[3];
    profile_cfg.carc_c_pri2 = p_carc_profile_cfg->c_pri[2];
    profile_cfg.carc_c_pri1 = p_carc_profile_cfg->c_pri[1];

    profile_cfg.carc_cbs = p_carc_profile_cfg->cbs;
    profile_cfg.carc_ebs_pbs = p_carc_profile_cfg->ebs;
    profile_cfg.carc_cir = p_carc_profile_cfg->cir;
    profile_cfg.carc_eir = p_carc_profile_cfg->eir;

    profile_cfg.carc_cd = p_carc_profile_cfg->cd;
    profile_cfg.carc_cf = p_carc_profile_cfg->cf;
    profile_cfg.carc_cm = p_carc_profile_cfg->cm;
    profile_cfg.carc_random_discard_en_e = p_carc_profile_cfg->random_disc_e;
    profile_cfg.carc_random_discard_en_c = p_carc_profile_cfg->random_disc_c;
    /* C级car包限速标记写死为0，以免用户配置成1对CARC进行包限速 */
    profile_cfg.carc_pkt_sign = 0;

    profile_cfg.carc_profile_wr = 0;

    rc = dpp_reg_write(dev,
                       STAT_CAR0_CARC_PROFILE_RAM1_255_0r ,
                       0,
                       profile_id,
                       &profile_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return rc;
}


/***********************************************************/
/** 获取car C的监管模板配置
* @param   dev_id               设备号               car编号
* @param   profile_id           监管模板号
* @param   p_carc_profile_cfg   监管模板配置
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/05
************************************************************/
DPP_STATUS dpp_stat_carc_profile_cfg_get(DPP_DEV_T *dev,
                                         ZXIC_UINT32 profile_id,
                                         DPP_STAT_CAR_PROFILE_CFG_T *p_carc_profile_cfg)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARC_PROFILE_RAM1_255_0_T profile_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_C_PROFILE_ID_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_carc_profile_cfg);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_CARC_PROFILE_RAM1_255_0r,
                      0,
                      profile_id,
                      &profile_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    p_carc_profile_cfg->profile_id = profile_id;
    p_carc_profile_cfg->pkt_sign = profile_cfg.carc_pkt_sign;
    p_carc_profile_cfg->cd = profile_cfg.carc_cd;
    p_carc_profile_cfg->cf = profile_cfg.carc_cf;
    p_carc_profile_cfg->cm = profile_cfg.carc_cm;

    p_carc_profile_cfg->eir = profile_cfg.carc_eir;
    p_carc_profile_cfg->cir = profile_cfg.carc_cir;
    p_carc_profile_cfg->ebs = profile_cfg.carc_ebs_pbs;
    p_carc_profile_cfg->cbs = profile_cfg.carc_cbs;

    p_carc_profile_cfg->random_disc_e = profile_cfg.carc_random_discard_en_e;
    p_carc_profile_cfg->random_disc_c = profile_cfg.carc_random_discard_en_c;

    p_carc_profile_cfg->c_pri[1] = profile_cfg.carc_c_pri1;
    p_carc_profile_cfg->c_pri[2] = profile_cfg.carc_c_pri2;
    p_carc_profile_cfg->c_pri[3] = profile_cfg.carc_c_pri3;
    p_carc_profile_cfg->c_pri[4] = profile_cfg.carc_c_pri4;
    p_carc_profile_cfg->c_pri[5] = profile_cfg.carc_c_pri5;
    p_carc_profile_cfg->c_pri[6] = profile_cfg.carc_c_pri6;
    p_carc_profile_cfg->c_pri[7] = profile_cfg.carc_c_pri7;

    p_carc_profile_cfg->e_green_pri[1] = profile_cfg.carc_e_g_pri1;
    p_carc_profile_cfg->e_green_pri[2] = profile_cfg.carc_e_g_pri2;
    p_carc_profile_cfg->e_green_pri[3] = profile_cfg.carc_e_g_pri3;
    p_carc_profile_cfg->e_green_pri[4] = profile_cfg.carc_e_g_pri4;
    p_carc_profile_cfg->e_green_pri[5] = profile_cfg.carc_e_g_pri5;
    p_carc_profile_cfg->e_green_pri[6] = profile_cfg.carc_e_g_pri6;
    p_carc_profile_cfg->e_green_pri[7] = profile_cfg.carc_e_g_pri7;

    p_carc_profile_cfg->e_yellow_pri[0] = profile_cfg.carc_e_y_pri0;
    p_carc_profile_cfg->e_yellow_pri[1] = profile_cfg.carc_e_y_pri1;
    p_carc_profile_cfg->e_yellow_pri[2] = profile_cfg.carc_e_y_pri2;
    p_carc_profile_cfg->e_yellow_pri[3] = profile_cfg.carc_e_y_pri3;
    p_carc_profile_cfg->e_yellow_pri[4] = profile_cfg.carc_e_y_pri4;
    p_carc_profile_cfg->e_yellow_pri[5] = profile_cfg.carc_e_y_pri5;
    p_carc_profile_cfg->e_yellow_pri[6] = profile_cfg.carc_e_y_pri6;
    p_carc_profile_cfg->e_yellow_pri[7] = profile_cfg.carc_e_y_pri7;

    return rc;
}

/***********************************************************/
/** 配置car C的qvos溢出模式
* @param   dev_id           设备号           car编号
* @param   flow_id          流号
* @param   qvos_mode        溢出模式，参见DPP_CAR_QVOS_MODE_E
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_carc_queue_qvos_set(DPP_DEV_T *dev,
                                        ZXIC_UINT32 flow_id,
                                        ZXIC_UINT32 qvos_mode)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARC_QOVS_RAM_RAM2_T qvos_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_C_FLOW_ID_MAX);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), qvos_mode, CAR_QVOS_MODE_OVERFLOW_0, CAR_QVOS_MODE_OVERFLOW_MAX - 1);

    qvos_cfg.carc_qovs = qvos_mode;

    rc = dpp_reg_write(dev,
                       STAT_CAR0_CARC_QOVS_RAM_RAM2r,
                       0,
                       flow_id,
                       &qvos_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return rc;
}
/***********************************************************/
/** 获取car C的qvos溢出模式
* @param   dev_id           设备号           car编号
* @param   flow_id          流号
* @param   p_qvos_mode      qvos溢出模式
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_carc_queue_qvos_get(DPP_DEV_T *dev,
                                        ZXIC_UINT32 flow_id,
                                        ZXIC_UINT32 *p_qvos_mode)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARC_QOVS_RAM_RAM2_T qvos_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_C_FLOW_ID_MAX);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_qvos_mode);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_CARC_QOVS_RAM_RAM2r ,
                      0,
                      flow_id,
                      &qvos_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    *p_qvos_mode = qvos_cfg.carc_qovs;

    return rc;
}

#if 0
/***********************************************************/
/** car C指定队列模式配置，仅用于调试
* @param   dev_id           设备号           car编号
* @param   global_en        全局队列使能，0-不使能，1-使能
* @param   sp_en            优先级队列使能，0-不使能，1-使能
* @param   appoint_sp       指定的优先级
* @param   appoint_queue    指定的队列号
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_carc_queue_appoint_mode_set(ZXIC_UINT32 dev_id,
                                                ZXIC_UINT32 global_en,
                                                ZXIC_UINT32 sp_en,
                                                ZXIC_UINT32 appoint_sp,
                                                ZXIC_UINT32 appoint_queue)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARC_APPOINT_QNUM_OR_SP_T appoint_cfg = {0};

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, global_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, sp_en, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, appoint_sp, DPP_CAR_PRI0, DPP_CAR_PRI_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, appoint_queue, 0, DPP_CAR_C_FLOW_ID_MAX);

    appoint_cfg.carc_appoint_qnum_or_not = global_en;
    appoint_cfg.carc_appoint_sp_or_not = sp_en;
    appoint_cfg.carc_plcr_stat_sp = appoint_sp;
    appoint_cfg.carc_plcr_stat_qnum = appoint_queue;

    rc = dpp_reg_write(dev_id,
                       STAT_CAR0_CARC_APPOINT_QNUM_OR_SPr ,
                       0,
                       0,
                       &appoint_cfg);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_reg_write");

    return rc;
}

/***********************************************************/
/** 获取car C指定队列模式的配置
* @param   dev_id           设备号           car编号
* @param   p_global_en      全局队列使能，0-不使能，1-使能
* @param   p_sp_en          优先级队列使能，0-不使能，1-使能
* @param   p_appoint_sp     指定的优先级
* @param   p_appoint_queue  指定的队列号
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/07
************************************************************/
DPP_STATUS dpp_stat_carc_queue_appoint_mode_get(ZXIC_UINT32 dev_id,
                                                ZXIC_UINT32 *p_global_en,
                                                ZXIC_UINT32 *p_sp_en,
                                                ZXIC_UINT32 *p_appoint_sp,
                                                ZXIC_UINT32 *p_appoint_queue)
{
    DPP_STATUS rc = DPP_OK;

    DPP_STAT_CAR0_CARC_APPOINT_QNUM_OR_SP_T appoint_cfg = {0};

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_global_en);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_sp_en);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_appoint_sp);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_appoint_queue);

    rc = dpp_reg_read(dev_id,
                      STAT_CAR0_CARC_APPOINT_QNUM_OR_SPr ,
                      0,
                      0,
                      &appoint_cfg);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_reg_read");

    *p_global_en = appoint_cfg.carc_appoint_qnum_or_not;
    *p_sp_en = appoint_cfg.carc_appoint_sp_or_not;
    *p_appoint_sp = appoint_cfg.carc_plcr_stat_sp;
    *p_appoint_queue = appoint_cfg.carc_plcr_stat_qnum;

    return rc;
}

/***********************************************************/
/** car C 调试计数读取模式配置
* @param   dev_id           设备号           car编号
* @param   overflow_mode    溢出模式，0-计数最大保持，1-计数最大翻转
* @param   rd_mode          读取模式，0-不读清，1-读清模式
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_carc_dbg_cnt_mode_set(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 overflow_mode,
                                          ZXIC_UINT32 rd_mode)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARC_CFGMT_COUNT_MODE_T cnt_mode_cfg = {0};

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, overflow_mode, CAR_KEEP_COUNT, CAR_RE_COUNT);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, rd_mode, CAR_READ_NOT_CLEAR, CAR_READ_AND_CLEAR);

    cnt_mode_cfg.carc_cfgmt_count_overflow_mode = overflow_mode;
    cnt_mode_cfg.carc_cfgmt_count_rd_mode = rd_mode;

    rc = dpp_reg_write(dev_id,
                       STAT_CAR0_CARC_CFGMT_COUNT_MODEr ,
                       0,
                       0,
                       &cnt_mode_cfg);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_reg_write");

    return rc;
}

/***********************************************************/
/** car C 调试计数读取模式获取
* @param   dev_id           设备号           car编号
* @param   p_overflow_mode  溢出模式，0-计数最大保持，1-计数最大翻转
* @param   p_rd_mode        读清模式，0-不读清，1-读清模式
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_carc_dbg_cnt_mode_get(ZXIC_UINT32 dev_id,
                                          ZXIC_UINT32 *p_overflow_mode,
                                          ZXIC_UINT32 *p_rd_mode)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARC_CFGMT_COUNT_MODE_T cnt_mode_cfg = {0};

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_overflow_mode);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_rd_mode);

    rc = dpp_reg_read(dev_id,
                      STAT_CAR0_CARC_CFGMT_COUNT_MODEr ,
                      0,
                      0,
                      &cnt_mode_cfg);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_reg_read");

    *p_overflow_mode = cnt_mode_cfg.carc_cfgmt_count_overflow_mode;
    *p_rd_mode = cnt_mode_cfg.carc_cfgmt_count_rd_mode;

    return rc;
}

/***********************************************************/
/** car b 的调试计数获取
* @param   dev_id
* @param   p_car_dbg_cnt
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/09/27
************************************************************/
DPP_STATUS dpp_stat_carc_dbg_cnt_get(ZXIC_UINT32 dev_id,
                                     DPP_STAT_CAR_DBG_CNT_T *p_car_dbg_cnt)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 size = 0;
    ZXIC_UINT32 *p_tmp_cnt = NULL;

    DPP_STAT_CAR0_CARC_PKT_SIZE_CNT_T  carc_pkt_size_cnt_cfg = {0};
    DPP_STAT_CAR0_CARC_PKT_DES_I_CNT_T carc_pkt_in_total_cnt_cfg = {0};

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_car_dbg_cnt);

    size = sizeof(DPP_STAT_CAR_DBG_CNT_T) / sizeof(ZXIC_UINT32) - 1;
    p_tmp_cnt = (ZXIC_UINT32 *)p_car_dbg_cnt;

    for (i = 0; i < size; i++)
    {
        rc = dpp_reg_read(dev_id,
                          STAT_CAR0_CARC_PKT_DES_I_CNTr + i,
                          0,
                          0,
                          &carc_pkt_in_total_cnt_cfg);
        ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_reg_read");

        ZXIC_COMM_MEMCPY(p_tmp_cnt + i, &(carc_pkt_in_total_cnt_cfg.carc_pkt_des_i_cnt), sizeof(ZXIC_UINT32));
    }

    rc = dpp_reg_read(dev_id,
                      STAT_CAR0_CARC_PKT_SIZE_CNTr,
                      0,
                      0,
                      &carc_pkt_size_cnt_cfg);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_reg_read");

    p_car_dbg_cnt->pkt_size_cnt = carc_pkt_size_cnt_cfg.carc_pkt_size_cnt;

    return rc;
}

/***********************************************************/
/** 获取car C的初始化状态
* @param   dev_id           设备号           car编号
* @param   p_init_done      初始化完成使能
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/06
************************************************************/
DPP_STATUS dpp_stat_carc_init_done_get(ZXIC_UINT32 dev_id,
                                       ZXIC_UINT32 *p_init_done)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARC_PLCR_INIT_DONT_T carc_init_done_cfg = {0};

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_init_done);

    rc = dpp_reg_read(dev_id,
                      STAT_CAR0_CARC_PLCR_INIT_DONTr ,
                      0,
                      0,
                      &carc_init_done_cfg);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_reg_read");

    *p_init_done = carc_init_done_cfg.carc_plcr_init_done;

    return rc;
}

#endif
/***********************************************************/
/** 
* @param   dev_id      
* @param   profile_id   
* @param   p_random_ram   
*
* @return  
* @remark  无
* @see     
* @author  YXH      @date  2019/04/01
************************************************************/
DPP_STATUS dpp_stat_carc_random_ram_set(DPP_DEV_T *dev,
                                        ZXIC_UINT32 profile_id,
                                        DPP_CAR_RANDOM_RAM_T *p_random_ram_e,
                                        DPP_CAR_RANDOM_RAM_T *p_random_ram_c)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT64 para0_temp = 0;
    ZXIC_UINT64 para2_temp = 0;
    ZXIC_UINT64 para4_temp = 0;
    DPP_STAT_CAR0_CARC_RANDOM_RAM_T carc_random_ram_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_random_ram_e);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_random_ram_e->p1, 0, 100);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_random_ram_e->p2, 0, 100);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_random_ram_e->p3, 0, 100);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_random_ram_c);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_random_ram_c->p1, 0, 100);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_random_ram_c->p2, 0, 100);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), p_random_ram_c->p3, 0, 100);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_C_PROFILE_ID_RANDOM_MAX);

    para0_temp = ((ZXIC_UINT64)((((ZXIC_UINT64)(p_random_ram_e->t2)) - ((ZXIC_UINT64)(p_random_ram_e->t1))) * ((ZXIC_UINT64)(p_random_ram_e->p1))) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;
    carc_random_ram_cfg.para0_l_e = (para0_temp & 0xFFFFFFFF);
    carc_random_ram_cfg.para0_h_e = (para0_temp >> 32) & 0xFFFFFFFF;

    carc_random_ram_cfg.para1_e = ((p_random_ram_e->p2 - p_random_ram_e->p1) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;

    para2_temp = ((ZXIC_UINT64)((((ZXIC_UINT64)(p_random_ram_e->t3)) - ((ZXIC_UINT64)(p_random_ram_e->t2))) * ((ZXIC_UINT64)(p_random_ram_e->p2))) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;
    carc_random_ram_cfg.para2_l_e = (para2_temp & 0xFFFFFFFF);
    carc_random_ram_cfg.para2_h_e = (para2_temp >> 32) & 0xFFFFFFFF;

    carc_random_ram_cfg.para3_e = ((p_random_ram_e->p3 - p_random_ram_e->p2) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;

    para4_temp = ((ZXIC_UINT64)((((ZXIC_UINT64)(p_random_ram_e->tc)) - ((ZXIC_UINT64)(p_random_ram_e->t3))) * ((ZXIC_UINT64)(p_random_ram_e->p3))) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;
    carc_random_ram_cfg.para4_l_e = (para4_temp & 0xFFFFFFFF);
    carc_random_ram_cfg.para4_h_e = (para4_temp >> 32) & 0xFFFFFFFF;

    carc_random_ram_cfg.para5_e = ((100 - p_random_ram_e->p3) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;
    carc_random_ram_cfg.para6_e = p_random_ram_e->t1;
    carc_random_ram_cfg.para7_e = p_random_ram_e->t2;
    carc_random_ram_cfg.para8_e = p_random_ram_e->t3;

	/* para0_temp = 0;
	para2_temp = 0;
	para4_temp = 0; */

    para0_temp = ((ZXIC_UINT64)((((ZXIC_UINT64)(p_random_ram_c->t2)) - ((ZXIC_UINT64)(p_random_ram_c->t1))) * ((ZXIC_UINT64)(p_random_ram_c->p1))) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;
    carc_random_ram_cfg.para0_l_c = (para0_temp & 0xFFFFFFFF);
    carc_random_ram_cfg.para0_h_c = (para0_temp >> 32) & 0xFFFFFFFF;

    carc_random_ram_cfg.para1_c = ((p_random_ram_c->p2 - p_random_ram_c->p1) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;

    para2_temp = ((ZXIC_UINT64)((((ZXIC_UINT64)(p_random_ram_c->t3)) - ((ZXIC_UINT64)(p_random_ram_c->t2))) * ((ZXIC_UINT64)(p_random_ram_c->p2))) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;
    carc_random_ram_cfg.para2_l_c = (para2_temp & 0xFFFFFFFF);
    carc_random_ram_cfg.para2_h_c = (para2_temp >> 32) & 0xFFFFFFFF;

    carc_random_ram_cfg.para3_c = ((p_random_ram_c->p3 - p_random_ram_c->p2) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;

    para4_temp = ((ZXIC_UINT64)((((ZXIC_UINT64)(p_random_ram_c->tc)) - ((ZXIC_UINT64)(p_random_ram_c->t3))) * ((ZXIC_UINT64)(p_random_ram_c->p3))) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;
    carc_random_ram_cfg.para4_l_c = (para4_temp & 0xFFFFFFFF);
    carc_random_ram_cfg.para4_h_c = (para4_temp >> 32) & 0xFFFFFFFF;

    carc_random_ram_cfg.para5_c = ((100 - p_random_ram_c->p3) << DPP_CAR_RANDOM_OFFSET_VAL) / 100;
    carc_random_ram_cfg.para6_c = p_random_ram_c->t1;
    carc_random_ram_cfg.para7_c = p_random_ram_c->t2;
    carc_random_ram_cfg.para8_c = p_random_ram_c->t3;

    rc = dpp_reg_write(dev,
                       STAT_CAR0_CARC_RANDOM_RAMr ,
                       0,
                       profile_id,
                       &carc_random_ram_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return rc;
}

/***********************************************************/
/** 
* @param   dev_id   
* @param   p_random_ram   
*
* @return  
* @remark  无
* @see     
* @author  YXH      @date  2019/04/01
************************************************************/
DPP_STATUS dpp_stat_carc_random_ram_get(DPP_DEV_T *dev,
                                        ZXIC_UINT32 profile_id,
                                        DPP_CAR_RANDOM_RAM_T *p_random_ram_e,
                                        DPP_CAR_RANDOM_RAM_T *p_random_ram_c)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CARB_RANDOM_RAM_T carc_random_ram_cfg = {0};
    ZXIC_UINT32 tmp_val = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_random_ram_e);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_random_ram_c);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_C_PROFILE_ID_RANDOM_MAX);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_CARC_RANDOM_RAMr ,
                      0,
                      profile_id,
                      &carc_random_ram_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    p_random_ram_e->t1 = carc_random_ram_cfg.para6_e;
    p_random_ram_e->t2 = carc_random_ram_cfg.para7_e;
    p_random_ram_e->t3 = carc_random_ram_cfg.para8_e;
    tmp_val = (carc_random_ram_cfg.para5_e * 100) >> DPP_CAR_RANDOM_OFFSET_VAL;
    p_random_ram_e->p3 = 100 - tmp_val;
    tmp_val = (carc_random_ram_cfg.para3_e * 100) >> DPP_CAR_RANDOM_OFFSET_VAL;
    p_random_ram_e->p2 = p_random_ram_e->p3 - tmp_val;
    tmp_val = (carc_random_ram_cfg.para1_e * 100) >> DPP_CAR_RANDOM_OFFSET_VAL;
    p_random_ram_e->p1 = p_random_ram_e->p2 - tmp_val;

    p_random_ram_c->t1 = carc_random_ram_cfg.para6_c;
    p_random_ram_c->t2 = carc_random_ram_cfg.para7_c;
    p_random_ram_c->t3 = carc_random_ram_cfg.para8_c;
    tmp_val = (carc_random_ram_cfg.para5_c * 100) >> DPP_CAR_RANDOM_OFFSET_VAL;
    p_random_ram_c->p3 = 100 - tmp_val;
    tmp_val = (carc_random_ram_cfg.para3_c * 100) >> DPP_CAR_RANDOM_OFFSET_VAL;
    p_random_ram_c->p2 = p_random_ram_c->p3 - tmp_val;
    tmp_val = (carc_random_ram_cfg.para1_c * 100) >> DPP_CAR_RANDOM_OFFSET_VAL;
    p_random_ram_c->p1 = p_random_ram_c->p2 - tmp_val;

    return rc;
}

/***********************************************************/
/** 配置car的层级模式
* @param   dev_id
* @param   mode   2 - 三级car, 第一级支持16K
*                 1 - 两级car, 第一级扩展为17K
*                 0 - 一级car, 第一级扩展为21K
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/09/28
************************************************************/
DPP_STATUS dpp_stat_car_en_mode_set(DPP_DEV_T *dev, ZXIC_UINT32 mode)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CAR_HIERARCHY_MODE_T car_en_mode_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), mode, DPP_CAR_EN_MODE_BOTH_EN, DPP_CAR_EN_MODE_INVALID - 1);

    car_en_mode_cfg.car_hierarchy_mode = mode;

    rc = dpp_reg_write(dev,
                       STAT_CAR0_CAR_HIERARCHY_MODEr ,
                       0,
                       0,
                       &car_en_mode_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return rc;
}

/***********************************************************/
/**
* @param   dev_id
* @param   p_mode
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/09/28
************************************************************/
DPP_STATUS dpp_stat_car_en_mode_get(DPP_DEV_T *dev,
                                    ZXIC_UINT32 *p_mode)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_CAR_HIERARCHY_MODE_T car_en_mode_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_mode);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_CAR_HIERARCHY_MODEr ,
                      0,
                      0,
                      &car_en_mode_cfg);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    *p_mode = car_en_mode_cfg.car_hierarchy_mode;

    return rc;
}

/***********************************************************/
/** 配置car的包长偏移
* @param   dev_id
* @param   pkt_size_off
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  wll      @date  2019/06/06
************************************************************/
DPP_STATUS dpp_stat_car_pkt_size_offset_set(DPP_DEV_T *dev,
                                            ZXIC_UINT32 pkt_size_off)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_PKT_SIZE_OFFSET_T car_pkt_size_offset = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), pkt_size_off, 0, 0xffffffff);

    car_pkt_size_offset.pkt_size_offset = pkt_size_off;

    rc = dpp_reg_write(dev,
                       STAT_CAR0_PKT_SIZE_OFFSETr,
                       0,
                       0,
                       &car_pkt_size_offset);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return rc;
}

/***********************************************************/
/** 获取car的包长偏移
* @param   dev_id
* @param   p_pkt_size_off
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  wll      @date  2019/06/06
************************************************************/
DPP_STATUS dpp_stat_car_pkt_size_offset_get(DPP_DEV_T *dev,
                                            ZXIC_UINT32 *p_pkt_size_off)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_PKT_SIZE_OFFSET_T car_pkt_size_offset = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_pkt_size_off);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_PKT_SIZE_OFFSETr,
                      0,
                      0,
                      &car_pkt_size_offset);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    *p_pkt_size_off = car_pkt_size_offset.pkt_size_offset;

    return rc;
}

/***********************************************************/
/** 配置cara的最大包长
* @param   dev_id
* @param   max_pkt_size
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  wll      @date  2019/06/06
************************************************************/
DPP_STATUS dpp_stat_cara_max_pkt_size_set(DPP_DEV_T *dev,
                                          ZXIC_UINT32 max_pkt_size)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_MAX_PKT_SIZE_A_T car_max_pkt_size = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), max_pkt_size, 0, 0x3fff);

    car_max_pkt_size.max_pkt_size_a = max_pkt_size;

    rc = dpp_reg_write(dev,
                       STAT_CAR0_MAX_PKT_SIZE_Ar ,
                       0,
                       0,
                       &car_max_pkt_size);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return rc;
}

/***********************************************************/
/** 获取cara的最大包长
* @param   dev_id
* @param   p_max_pkt_size
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  wll      @date  2019/06/06
************************************************************/
DPP_STATUS dpp_stat_cara_max_pkt_size_get(DPP_DEV_T *dev,
                                          ZXIC_UINT32 *p_max_pkt_size)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_MAX_PKT_SIZE_A_T car_max_pkt_size = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_max_pkt_size);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_MAX_PKT_SIZE_Ar ,
                      0,
                      0,
                      &car_max_pkt_size);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    *p_max_pkt_size = car_max_pkt_size.max_pkt_size_a;

    return rc;
}

/***********************************************************/
/** 配置carb的最大包长
* @param   dev_id
* @param   max_pkt_size
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  wll      @date  2019/06/06
************************************************************/
DPP_STATUS dpp_stat_carb_max_pkt_size_set(DPP_DEV_T *dev,
                                          ZXIC_UINT32 max_pkt_size)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_MAX_PKT_SIZE_B_T car_max_pkt_size = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), max_pkt_size, 0, 0x3fff);

    car_max_pkt_size.max_pkt_size_b = max_pkt_size;

    rc = dpp_reg_write(dev,
                       STAT_CAR0_MAX_PKT_SIZE_Br,
                       0,
                       0,
                       &car_max_pkt_size);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return rc;
}

/***********************************************************/
/** 获取carb的最大包长
* @param   dev_id
* @param   p_max_pkt_size
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  wll      @date  2019/06/06
************************************************************/
DPP_STATUS dpp_stat_carb_max_pkt_size_get(DPP_DEV_T *dev,
                                          ZXIC_UINT32 *p_max_pkt_size)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_MAX_PKT_SIZE_B_T car_max_pkt_size = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_max_pkt_size);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_MAX_PKT_SIZE_Br,
                      0,
                      0,
                      &car_max_pkt_size);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    *p_max_pkt_size = car_max_pkt_size.max_pkt_size_b;

    return rc;
}

/***********************************************************/
/** 配置carc的最大包长
* @param   dev_id
* @param   max_pkt_size
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  wll      @date  2019/06/06
************************************************************/
DPP_STATUS dpp_stat_carc_max_pkt_size_set(DPP_DEV_T *dev,
                                          ZXIC_UINT32 max_pkt_size)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_MAX_PKT_SIZE_C_T car_max_pkt_size = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), max_pkt_size, 0, 0x3fff);

    car_max_pkt_size.max_pkt_size_c = max_pkt_size;

    rc = dpp_reg_write(dev,
                       STAT_CAR0_MAX_PKT_SIZE_Cr ,
                       0,
                       0,
                       &car_max_pkt_size);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_write");

    return rc;
}

/***********************************************************/
/** 获取carc的最大包长
* @param   dev_id
* @param   p_max_pkt_size
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  wll      @date  2019/06/06
************************************************************/
DPP_STATUS dpp_stat_carc_max_pkt_size_get(DPP_DEV_T *dev,
                                          ZXIC_UINT32 *p_max_pkt_size)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR0_MAX_PKT_SIZE_C_T car_max_pkt_size = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_max_pkt_size);

    rc = dpp_reg_read(dev,
                      STAT_CAR0_MAX_PKT_SIZE_Cr,
                      0,
                      0,
                      &car_max_pkt_size);
    ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_reg_read");

    *p_max_pkt_size = car_max_pkt_size.max_pkt_size_c;

    return rc;
}

#endif

#if ZXIC_REAL("Advanced Function")
#if 0
/***********************************************************/
/** car硬件初始化
* @param   dev_id        设备号        car编号
* @param   car_type      car模式，参见STAT_CAR_TYPE_E
* @param   car_mono_mode car独占mono模式
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/04/27
************************************************************/
DPP_STATUS dpp_stat_car_hardware_init(ZXIC_UINT32 dev_id,
                                      ZXIC_UINT32 car_type,
                                      ZXIC_UINT32 car_mono_mode)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 init_done = 0;

    DPP_STAT_CAR_A_QUEUE_CFG_T car_a_queue_cfg = {0};
    DPP_STAT_CAR_B_QUEUE_CFG_T car_b_queue_cfg = {0};
    DPP_STAT_CAR_C_QUEUE_CFG_T car_c_queue_cfg = {0};
    DPP_STAT_CAR_PROFILE_CFG_T car_profile_cfg = {0};

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, car_type, STAT_CAR_A_TYPE, STAT_CAR_MAX_TYPE - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, car_mono_mode, CAR_SMMU0_MONO_MODE_NONE, CAR_SMMU0_MONO_MODE_MAX - 1);

    ZXIC_COMM_MEMSET(&car_profile_cfg, 0, sizeof(DPP_STAT_CAR_PROFILE_CFG_T));

    rc = dpp_se_smmu0_cfg_car_mono_set(dev_id, car_mono_mode);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_se_smmu0_cfg_car_mono_set");

    switch (car_type)
    {
        case STAT_CAR_A_TYPE:
        {
            rc = dpp_stat_cara_init_done_get(dev_id, &init_done);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_cara_init_done_get");

            if (0 == init_done)
            {
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "Error! Get car_a init done fail!\n");
                return DPP_RC_CAR_INIT_FAIL;
            }

            ZXIC_COMM_MEMSET(&car_a_queue_cfg, 0, sizeof(DPP_STAT_CAR_A_QUEUE_CFG_T));

            /** 清空queue的配置 和绑定队列号配置 稍后补全 */
            for (i = 0; i <= DPP_CAR_A_FLOW_ID_MAX; i++)
            {
                rc = dpp_stat_cara_queue_cfg_set(dev_id,
                                                 i,
                                                 0,
                                                 0,
                                                 0);
                ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_cara_queue_cfg_set");
            }

            /** 清空profile的配置 */
            for (i = 0; i <= DPP_CAR_A_PROFILE_ID_MAX; i++)
            {
                rc = dpp_stat_cara_profile_cfg_set(dev_id,
                                                   i,
                                                   &car_profile_cfg);
                ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_cara_profile_cfg_set");
            }
        }
        break;

        case STAT_CAR_B_TYPE:
        {
            rc = dpp_stat_carb_init_done_get(dev_id, &init_done);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_carb_init_done_get");

            if (0 == init_done)
            {
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "Error! Get car_b init done fail!\n");
                return DPP_RC_CAR_INIT_FAIL;
            }

            ZXIC_COMM_MEMSET(&car_b_queue_cfg, 0, sizeof(DPP_STAT_CAR_B_QUEUE_CFG_T));

            /** 清空queue的配置 和绑定队列号配置 稍后补全 */

            for (i = 0; i <= DPP_CAR_B_FLOW_ID_MAX; i++)
            {
                rc = dpp_stat_carb_queue_cfg_set(dev_id,
                                                 i,
                                                 0,
                                                 0,
                                                 0);
                ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_cara_queue_cfg_set");
            }

            /** 清空profile的配置 */
            for (i = 0; i <= DPP_CAR_B_PROFILE_ID_MAX; i++)
            {
                rc = dpp_stat_carb_profile_cfg_set(dev_id,
                                                   i,
                                                   &car_profile_cfg);
                ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_carb_profile_cfg_set");
            }
        }
        break;

        case STAT_CAR_C_TYPE:
        {
            rc = dpp_stat_carc_init_done_get(dev_id,
                                             &init_done);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_carc_init_done_get");

            if (0 == init_done)
            {
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "Error! Get car_c init done fail!\n");
                return DPP_RC_CAR_INIT_FAIL;
            }

            ZXIC_COMM_MEMSET(&car_c_queue_cfg, 0, sizeof(DPP_STAT_CAR_C_QUEUE_CFG_T));

            /** 清空queue的配置 和绑定队列号配置 稍后补全 */

            for (i = 0; i <= DPP_CAR_C_FLOW_ID_MAX; i++)
            {
                rc = dpp_stat_carc_queue_cfg_set(dev_id,
                                                 i,
                                                 0,
                                                 0,
                                                 0);
                ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_carc_queue_cfg_set");
            }

            /** 清空profile的配置 */
            for (i = 0; i <= DPP_CAR_C_PROFILE_ID_MAX; i++)
            {
                rc = dpp_stat_carc_profile_cfg_set(dev_id,
                                                   i,
                                                   &car_profile_cfg);
                ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_carc_profile_cfg_set");
            }
        }
        break;

        default:
        {
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id, "car_type[%d] error!\n", car_type);
            return DPP_ERR;
        }
    }

    return rc;
}
#endif
/***********************************************************/
/** car 模块流配置
* @param   dev_id       设备号
* @param   car_type     car模式，参见STAT_CAR_TYPE_E
* @param   flow_id      队列号
* @param   drop_flag    丢弃标志
* @param   plcr_en      限速使能
* @param   profile_id   模板编号
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/05/06
************************************************************/
DPP_STATUS dpp_stat_car_queue_cfg_set(DPP_DEV_T *dev,
                                      ZXIC_UINT32 car_type,
                                      ZXIC_UINT32 flow_id,
                                      ZXIC_UINT32 drop_flag,
                                      ZXIC_UINT32 plcr_en,
                                      ZXIC_UINT32 profile_id)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 flow_num = 0;

    DPP_CAR_SOFT_RESET_DATA_T *p_restore_data = NULL; /* 软复位相关 */

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), car_type, STAT_CAR_A_TYPE, STAT_CAR_MAX_TYPE - 1);

    if (STAT_CAR_A_TYPE == car_type)
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_A_FLOW_ID_MAX);
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_A_PROFILE_ID_MAX);
    }
    else if (STAT_CAR_B_TYPE == car_type)
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_B_FLOW_ID_MAX);
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_B_PROFILE_ID_MAX);
    }
    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_C_FLOW_ID_MAX);
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_C_PROFILE_ID_MAX);
    }

    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), drop_flag, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), plcr_en, 0, 1);

    p_restore_data = GET_DPP_CAR_SOFT_RESET_INFO(DEV_ID(dev)); /* 软复位相关 */

    switch (car_type)
    {
        case STAT_CAR_A_TYPE:
        {
            rc = dpp_stat_cara_queue_cfg_set(dev,
                                             flow_id,
                                             drop_flag,
                                             plcr_en,
                                             profile_id);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_cara_queue_cfg_set");

            /* 软复位相关 */
            flow_num = p_restore_data->cara_flow_num;
            if (flow_num < DPP_CAR_A_FLOW_ID_NUM)
            {
                p_restore_data->cara_item[flow_num].flow_id = flow_id;
                p_restore_data->cara_item[flow_num].profile_id = profile_id;
                ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW(DEV_ID(dev), p_restore_data->cara_flow_num, 1);
                p_restore_data->cara_flow_num++;
            }

        }
        break;

        case STAT_CAR_B_TYPE:
        {
            rc = dpp_stat_carb_queue_cfg_set(dev,
                                             flow_id,
                                             drop_flag,
                                             plcr_en,
                                             profile_id);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_carb_queue_cfg_set");

            /* 软复位相关 */
            flow_num = p_restore_data->carb_flow_num;
            if (flow_num < DPP_CAR_B_FLOW_ID_NUM)
            {
                p_restore_data->carb_item[flow_num].flow_id = flow_id;
                p_restore_data->carb_item[flow_num].profile_id = profile_id;
                ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW(DEV_ID(dev), p_restore_data->carb_flow_num, 1);
                p_restore_data->carb_flow_num++;
            }

        }
        break;

        case STAT_CAR_C_TYPE:
        {
            rc = dpp_stat_carc_queue_cfg_set(dev,
                                             flow_id,
                                             drop_flag,
                                             plcr_en,
                                             profile_id);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_carc_queue_cfg_set");

            /* 软复位相关 */
            flow_num = p_restore_data->carc_flow_num;
            if (flow_num < DPP_CAR_C_FLOW_ID_NUM)
            {
                p_restore_data->carc_item[flow_num].flow_id = flow_id;
                p_restore_data->carc_item[flow_num].profile_id = profile_id;
                ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev), p_restore_data->carc_flow_num, 1);
                p_restore_data->carc_flow_num++;
            }

        }
        break;
    }

    return rc;
}

/***********************************************************/
/**队列设置参数获取
* @param   dev_id   设备号   car编号
* @param   car_type car模式，参见STAT_CAR_TYPE_E
* @param   pkt_sign 限速标志，1-包限速，0-字节限速
* @param   flow_id  队列号
* @param   p_data   获取队列配置信息
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/09/27
************************************************************/
DPP_STATUS dpp_stat_car_queue_get(DPP_DEV_T *dev,
                                  ZXIC_UINT32 car_type,
                                  ZXIC_UINT32 pkt_sign,
                                  ZXIC_UINT32 flow_id,
                                  ZXIC_VOID *p_data)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), car_type, STAT_CAR_A_TYPE, STAT_CAR_MAX_TYPE - 1);

    if (STAT_CAR_A_TYPE == car_type)
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), pkt_sign, 0, 1);
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_A_FLOW_ID_MAX);
    }
    else if (STAT_CAR_B_TYPE == car_type)
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_B_FLOW_ID_MAX);
    }
    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_C_FLOW_ID_MAX);
    }

    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_data);

    switch (car_type)
    {
        case STAT_CAR_A_TYPE:
        {
            if (0 == pkt_sign)
            {
                rc = dpp_stat_cara_queue_cfg_get(dev,
                                                 flow_id,
                                                 (DPP_STAT_CAR_A_QUEUE_CFG_T *)p_data);
                ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_cara_queue_cfg_get");
            }
            else
            {
                rc = dpp_stat_cara_pkt_queue_cfg_get(dev,
                                                     flow_id,
                                                     (DPP_STAT_CAR_A_PKT_QUEUE_CFG_T *) p_data);
                ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_cara_pkt_queue_cfg_get");
            }

        }
        break;

        case STAT_CAR_B_TYPE:
        {
            rc = dpp_stat_carb_queue_cfg_get(dev,
                                             flow_id,
                                             (DPP_STAT_CAR_B_QUEUE_CFG_T *)p_data);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_carb_queue_cfg_get");
        }
        break;

        case STAT_CAR_C_TYPE:
        {
            rc = dpp_stat_carc_queue_cfg_get(dev,
                                             flow_id,
                                             (DPP_STAT_CAR_C_QUEUE_CFG_T *)p_data);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_carc_queue_cfg_get");
        }
        break;
    }

    return rc;
}

/***********************************************************/
/** car 模块流配置获取
* @param   dev_id       设备号
* @param   car_type     car模式，参见STAT_CAR_TYPE_E
* @param   flow_id      队列号
* @param   p_drop_flag  drop标记
* @param   p_plcr_en    监管使能信号
* @param   p_profile_id 模板id
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/08/19
************************************************************/
DPP_STATUS dpp_stat_car_queue_cfg_get(DPP_DEV_T *dev,
                                      ZXIC_UINT32 car_type,
                                      ZXIC_UINT32 flow_id,
                                      ZXIC_UINT32 *p_drop_flag,
                                      ZXIC_UINT32 *p_plcr_en,
                                      ZXIC_UINT32 *p_profile_id)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR_A_QUEUE_CFG_T car_a_queue_cfg = {0};
    DPP_STAT_CAR_B_QUEUE_CFG_T car_b_queue_cfg = {0};
    DPP_STAT_CAR_C_QUEUE_CFG_T car_c_queue_cfg = {0};

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), car_type, STAT_CAR_A_TYPE, STAT_CAR_MAX_TYPE - 1);

    if (STAT_CAR_A_TYPE == car_type)
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_A_FLOW_ID_MAX);
    }
    else if (STAT_CAR_B_TYPE == car_type)
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_B_FLOW_ID_MAX);
    }
    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_C_FLOW_ID_MAX);
    }

    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_drop_flag);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_plcr_en);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_profile_id);

    switch (car_type)
    {
        case STAT_CAR_A_TYPE:
        {
            rc = dpp_stat_cara_queue_cfg_get(dev,
                                             flow_id,
                                             &car_a_queue_cfg);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_cara_queue_cfg_get");

            *p_profile_id = car_a_queue_cfg.profile_id;
            *p_plcr_en    = car_a_queue_cfg.plcr_en;
            *p_drop_flag  = car_a_queue_cfg.drop_flag;
        }
        break;

        case STAT_CAR_B_TYPE:
        {
            rc = dpp_stat_carb_queue_cfg_get(dev,
                                             flow_id,
                                             &car_b_queue_cfg);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_carb_queue_cfg_get");

            *p_profile_id = car_b_queue_cfg.profile_id;
            *p_plcr_en    = car_b_queue_cfg.plcr_en;
            *p_drop_flag  = car_b_queue_cfg.drop_flag;
        }
        break;

        case STAT_CAR_C_TYPE:
        {
            rc = dpp_stat_carc_queue_cfg_get(dev,
                                             flow_id,
                                             &car_c_queue_cfg);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_carc_queue_cfg_get");

            *p_profile_id = car_c_queue_cfg.profile_id;
            *p_plcr_en    = car_c_queue_cfg.plcr_en;
            *p_drop_flag  = car_c_queue_cfg.drop_flag;
        }
        break;
    }

    return rc;
}

/***********************************************************/
/** car profile硬件写入
* @param   dev_id               设备号               car编号
* @param   car_type             car模式，参见STAT_CAR_TYPE_E
* @param   pkt_sign             限速模式，1-包限速，0-字节限速
* @param   profile_id           模板号
* @param   p_car_profile_cfg    模板配置信息
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/05/06
************************************************************/
DPP_STATUS dpp_stat_car_profile_cfg_set(DPP_DEV_T *dev,
                                        ZXIC_UINT32 car_type,
                                        ZXIC_UINT32 pkt_sign,
                                        ZXIC_UINT32 profile_id,
                                        ZXIC_VOID *p_car_profile_cfg)
{
    DPP_STATUS rc = DPP_OK;

    DPP_CAR_SOFT_RESET_DATA_T *p_restore_data = NULL; /* 软复位相关 */
    DPP_STAT_CAR_PROFILE_CFG_T *p_stat_car_profile_cfg = NULL;
    DPP_STAT_CAR_PKT_PROFILE_CFG_T *p_stat_pkt_car_profile_cfg = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), car_type, STAT_CAR_A_TYPE, STAT_CAR_MAX_TYPE - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), pkt_sign, 0, 1);

    if (STAT_CAR_A_TYPE == car_type)
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_A_PROFILE_ID_MAX);
    }
    else if (STAT_CAR_B_TYPE == car_type)
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_B_PROFILE_ID_MAX);
    }
    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_C_PROFILE_ID_MAX);
    }

    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_car_profile_cfg);

    p_restore_data = GET_DPP_CAR_SOFT_RESET_INFO(DEV_ID(dev)); /* 软复位相关 */

    if ((STAT_CAR_A_TYPE == car_type) && (1 == pkt_sign))
    {
        p_stat_pkt_car_profile_cfg = (DPP_STAT_CAR_PKT_PROFILE_CFG_T *)p_car_profile_cfg;

        rc = dpp_stat_cara_pkt_profile_cfg_set(dev,
                                               profile_id,
                                               p_stat_pkt_car_profile_cfg);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_cara_pkt_profile_cfg_set");

        /* 软复位相关 */
        p_restore_data->car_pkt_sign[profile_id] = ZXIC_TRUE;

        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev), p_restore_data->car0_pkt_num, 1);
        p_restore_data->car0_pkt_num++;
    }
    else
    {
        p_stat_car_profile_cfg = (DPP_STAT_CAR_PROFILE_CFG_T *)p_car_profile_cfg;
        ZXIC_COMM_TRACE_DEV_INFO(DEV_ID(dev), "==> dpp_stat_car_profile_cfg_set : \n");
        ZXIC_COMM_TRACE_DEV_INFO(DEV_ID(dev), "| %-10s | %-10s | %-10s | %-10s | \n", "profile_id", "car_id", "car_type", "pkt_sign");
        ZXIC_COMM_TRACE_DEV_INFO(DEV_ID(dev), "| %-10d | %-10d | %-10d | 0x%-8x | \n", profile_id, 0, car_type, pkt_sign);
        ZXIC_COMM_TRACE_DEV_INFO(DEV_ID(dev), "| ------------------------------------------------- | \n");
        ZXIC_COMM_TRACE_DEV_INFO(DEV_ID(dev), "| %-5s | %-5s | %-5s | %-10s | %-10s | %-10s | %-10s | \n", "cd", "cf", "cm", "cir", "cbs", "eir", "ebs");
        ZXIC_COMM_TRACE_DEV_INFO(DEV_ID(dev), "| %-5d | %-5d | %-5d | %-10d | %-10d | %-10d | %-10d | \n", p_stat_car_profile_cfg->cd,
                                                                                                       p_stat_car_profile_cfg->cf,
                                                                                                       p_stat_car_profile_cfg->cm,
                                                                                                       p_stat_car_profile_cfg->cir,
                                                                                                       p_stat_car_profile_cfg->cbs,
                                                                                                       p_stat_car_profile_cfg->eir,
                                                                                                       p_stat_car_profile_cfg->ebs);

        if (STAT_CAR_A_TYPE == car_type)
        {
            rc = dpp_stat_cara_profile_cfg_set(dev,
                                               profile_id,
                                               p_stat_car_profile_cfg);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_cara_profile_cfg_set");

            /* 软复位相关 */
            if (ZXIC_TRUE == p_restore_data->car_pkt_sign[profile_id])
            {
                p_restore_data->car0_pkt_num--;
                p_restore_data->car_pkt_sign[profile_id] = ZXIC_FALSE;
            }
        }
        else if (STAT_CAR_B_TYPE == car_type)
        {
            rc = dpp_stat_carb_profile_cfg_set(dev,
                                               profile_id,
                                               p_stat_car_profile_cfg);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_carb_profile_cfg_set");
        }
        else
        {
            rc = dpp_stat_carc_profile_cfg_set(dev,
                                               profile_id,
                                               p_stat_car_profile_cfg);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_carc_profile_cfg_set");
        }
    }

    return rc;

}

/***********************************************************/
/**获取监管模板配置
* @param   dev_id               设备号
* @param   car_type             car模式类型，参见STAT_CAR_TYPE_E
* @param   pkt_sign             限速模式，1-包限速，0-字节限速
* @param   profile_id           监管模板id
* @param   p_car_profile_cfg    模板配置信息
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/08/29
************************************************************/
DPP_STATUS dpp_stat_car_profile_cfg_get(DPP_DEV_T *dev,
                                        ZXIC_UINT32 car_type,
                                        ZXIC_UINT32 pkt_sign,
                                        ZXIC_UINT32 profile_id,
                                        ZXIC_VOID *p_car_profile_cfg)
{
    DPP_STATUS rc = DPP_OK;
    DPP_STAT_CAR_PROFILE_CFG_T *p_stat_car_profile_cfg = NULL;
    DPP_STAT_CAR_PKT_PROFILE_CFG_T *p_stat_pkt_car_profile_cfg = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), car_type, STAT_CAR_A_TYPE, STAT_CAR_MAX_TYPE - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), pkt_sign, 0, 1);

    if (STAT_CAR_A_TYPE == car_type)
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_A_PROFILE_ID_MAX);
    }
    else if (STAT_CAR_B_TYPE == car_type)
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_B_PROFILE_ID_MAX);
    }
    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_C_PROFILE_ID_MAX);
    }

    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_car_profile_cfg);

    if ((STAT_CAR_A_TYPE == car_type) && (1 == pkt_sign))
    {
        p_stat_pkt_car_profile_cfg = (DPP_STAT_CAR_PKT_PROFILE_CFG_T *)p_car_profile_cfg;
        rc = dpp_stat_cara_pkt_profile_cfg_get(dev,
                                               profile_id,
                                               p_stat_pkt_car_profile_cfg);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_cara_pkt_profile_cfg_get");
    }
    else
    {
        p_stat_car_profile_cfg = (DPP_STAT_CAR_PROFILE_CFG_T *)p_car_profile_cfg;

        if (STAT_CAR_A_TYPE == car_type)
        {
            rc = dpp_stat_cara_profile_cfg_get(dev,
                                               profile_id,
                                               p_stat_car_profile_cfg);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_cara_profile_cfg_get");
        }
        else if (STAT_CAR_B_TYPE == car_type)
        {
            rc = dpp_stat_carb_profile_cfg_get(dev,
                                               profile_id,
                                               p_stat_car_profile_cfg);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_carb_profile_cfg_get");
        }
        else
        {
            rc = dpp_stat_carc_profile_cfg_get(dev,
                                               profile_id,
                                               p_stat_car_profile_cfg);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_carc_profile_cfg_get");
        }
    }

    return rc;
}

/***********************************************************/
/** car 队列映射关系配置
* @param   dev_id       设备号
* @param   car_type     car模式类型，参见STAT_CAR_TYPE_E
* @param   flow_id      队列号
* @param   map_flow_id  映射队列号
* @param   map_sp       映射sp
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/09/27
************************************************************/
DPP_STATUS dpp_stat_car_queue_map_set(DPP_DEV_T *dev,
                                      ZXIC_UINT32 car_type,
                                      ZXIC_UINT32 flow_id,
                                      ZXIC_UINT32 map_flow_id,
                                      ZXIC_UINT32 map_sp)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), car_type, STAT_CAR_A_TYPE, STAT_CAR_B_TYPE);

    if (STAT_CAR_A_TYPE == car_type)
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_A_FLOW_ID_MAX);
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), map_flow_id, 0, DPP_CAR_B_FLOW_ID_MAX);
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), map_sp, DPP_CAR_PRI0, DPP_CAR_PRI_MAX - 1);
        rc = dpp_stat_cara_queue_map_set(dev,
                                         flow_id,
                                         map_flow_id,
                                         map_sp);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_cara_queue_map_set");
    }
    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_B_FLOW_ID_MAX);
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), map_flow_id, 0, DPP_CAR_C_FLOW_ID_MAX);
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), map_sp, DPP_CAR_PRI0, DPP_CAR_PRI_MAX - 1);
        rc = dpp_stat_carb_queue_map_set(dev,
                                         flow_id,
                                         map_flow_id,
                                         map_sp);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_carb_queue_map_set");
    }

    return rc;
}

/***********************************************************/
/** 获取 car 流号的绑定关系
* @param   dev_id         设备号
* @param   car_type       car模式类型，参见STAT_CAR_TYPE_E
* @param   flow_id        队列号
* @param   p_map_flow_id  映射队列号
* @param   p_map_sp       映射sp
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/09/27
************************************************************/
DPP_STATUS dpp_stat_car_queue_map_get(DPP_DEV_T *dev,
                                      ZXIC_UINT32 car_type,
                                      ZXIC_UINT32 flow_id,
                                      ZXIC_UINT32 *p_map_flow_id,
                                      ZXIC_UINT32 *p_map_sp)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), car_type, STAT_CAR_A_TYPE, STAT_CAR_B_TYPE);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_map_flow_id);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_map_sp);

    if (STAT_CAR_A_TYPE == car_type)
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_A_FLOW_ID_MAX);
        rc = dpp_stat_cara_queue_map_get(dev,
                                         flow_id,
                                         p_map_flow_id,
                                         p_map_sp);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_cara_queue_map_get");
    }
    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), flow_id, 0, DPP_CAR_B_FLOW_ID_MAX);
        rc = dpp_stat_carb_queue_map_get(dev,
                                         flow_id,
                                         p_map_flow_id,
                                         p_map_sp);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_carb_queue_map_get");
    }

    return rc;
}

/***********************************************************/
/** 
* @param   dev_id         设备ID
* @param   profile_id     模板ID
* @param   p_random_ram_e E桶概率丢弃配置参数
* @param   p_random_ram_c C桶概率丢弃配置参数
*
* @return  
* @remark  无
* @see     
* @author  YXH      @date  2019/04/01
************************************************************/
DPP_STATUS dpp_stat_car_random_ram_set(DPP_DEV_T *dev,
                                       ZXIC_UINT32 car_type,
                                       ZXIC_UINT32 profile_id,
                                       DPP_CAR_RANDOM_RAM_T *p_random_ram_e,
                                       DPP_CAR_RANDOM_RAM_T *p_random_ram_c)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), car_type, STAT_CAR_B_TYPE, STAT_CAR_C_TYPE);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_random_ram_e);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_random_ram_c);

    if (STAT_CAR_B_TYPE == car_type)
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_B_PROFILE_ID_RANDOM_MAX);
        rc = dpp_stat_carb_random_ram_set(dev,
                                          profile_id,
                                          p_random_ram_e,
                                          p_random_ram_c);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_carb_random_ram_set");
    }
    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_C_PROFILE_ID_RANDOM_MAX);
        rc = dpp_stat_carc_random_ram_set(dev,
                                          profile_id,
                                          p_random_ram_e,
                                          p_random_ram_c);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_carc_random_ram_set");
    }

    return rc;
}

/***********************************************************/
/** 
* @param   dev_id         设备ID
* @param   profile_id     模板ID
* @param   p_random_ram_e E桶概率丢弃配置参数
* @param   p_random_ram_c C桶概率丢弃配置参数
*
* @return  
* @remark  无
* @see     
* @author  YXH      @date  2019/04/01
************************************************************/
DPP_STATUS dpp_stat_car_random_ram_get(DPP_DEV_T *dev,
                                       ZXIC_UINT32 car_type,
                                       ZXIC_UINT32 profile_id,
                                       DPP_CAR_RANDOM_RAM_T *p_random_ram_e,
                                       DPP_CAR_RANDOM_RAM_T *p_random_ram_c)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), car_type, STAT_CAR_B_TYPE, STAT_CAR_C_TYPE);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_random_ram_e);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_random_ram_c);

    if (STAT_CAR_B_TYPE == car_type)
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_B_PROFILE_ID_RANDOM_MAX);
        rc = dpp_stat_carb_random_ram_get(dev,
                                          profile_id,
                                          p_random_ram_e,
                                          p_random_ram_c);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_carb_random_ram_get");
    }
    else
    {
        ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), profile_id, 0, DPP_CAR_C_PROFILE_ID_RANDOM_MAX);
        rc = dpp_stat_carc_random_ram_get(dev,
                                          profile_id,
                                          p_random_ram_e,
                                          p_random_ram_c);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_carc_random_ram_get");
    }

    return rc;
}

#if 0
/***********************************************************/
/**car模块dbg计数模式设置
* @param   dev_id           设备号
* @param   car_type         car模式类型，参见STAT_CAR_TYPE_E
* @param   overflow_mode    溢出模式，0-计数最大保持，1-计数最大翻转
* @param   rd_mode          读取模式，0-不读清，1-读清模式
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/09/27
************************************************************/
DPP_STATUS dpp_stat_car_dbg_cnt_mode_set(ZXIC_UINT32 dev_id,
                                         ZXIC_UINT32 car_type,
                                         ZXIC_UINT32 overflow_mode,
                                         ZXIC_UINT32 rd_mode)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, car_type, STAT_CAR_A_TYPE, STAT_CAR_MAX_TYPE - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, overflow_mode, 0, 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, rd_mode, 0, 1);

    switch (car_type)
    {
        case STAT_CAR_A_TYPE:
        {
            rc = dpp_stat_cara_dbg_cnt_mode_set(dev_id,
                                                overflow_mode,
                                                rd_mode);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_cara_dbg_cnt_mode_set");
        }
        break;

        case STAT_CAR_B_TYPE:
        {
            rc = dpp_stat_carb_dbg_cnt_mode_set(dev_id,
                                                overflow_mode,
                                                rd_mode);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_carb_dbg_cnt_mode_set");
        }
        break;

        case STAT_CAR_C_TYPE:
        {
            rc = dpp_stat_carc_dbg_cnt_mode_set(dev_id,
                                                overflow_mode,
                                                rd_mode);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_carc_dbg_cnt_mode_set");
        }
        break;
    }

    return rc;
}

/***********************************************************/
/**car模块dbg计数模式获取
* @param   dev_id             设备号
* @param   car_type           car模式类型，参见STAT_CAR_TYPE_E
* @param   p_overflow_mode    溢出模式，0-计数最大保持，1-计数最大翻转
* @param   p_rd_mode          读取模式，0-不读清，1-读清模式
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/09/27
************************************************************/
DPP_STATUS dpp_stat_car_dbg_cnt_mode_get(ZXIC_UINT32 dev_id,
                                         ZXIC_UINT32 car_type,
                                         ZXIC_UINT32 *p_overflow_mode,
                                         ZXIC_UINT32 *p_rd_mode)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, car_type, STAT_CAR_A_TYPE, STAT_CAR_MAX_TYPE - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_overflow_mode);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_rd_mode);

    switch (car_type)
    {
        case STAT_CAR_A_TYPE:
        {
            rc = dpp_stat_cara_dbg_cnt_mode_get(dev_id,
                                                p_overflow_mode,
                                                p_rd_mode);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_cara_dbg_cnt_mode_get");
        }
        break;

        case STAT_CAR_B_TYPE:
        {
            rc = dpp_stat_carb_dbg_cnt_mode_get(dev_id,
                                                p_overflow_mode,
                                                p_rd_mode);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_carb_dbg_cnt_mode_get");
        }
        break;

        case STAT_CAR_C_TYPE:
        {
            rc = dpp_stat_carc_dbg_cnt_mode_get(dev_id,
                                                p_overflow_mode,
                                                p_rd_mode);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_carc_dbg_cnt_mode_get");
        }
        break;
    }

    return rc;
}

/***********************************************************/
/** car 模块调试计数 获取
* @param   dev_id          设备号
* @param   car_type        car模式类型，参见STAT_CAR_TYPE_E
* @param   p_car_dbg_cnt   dbg计数信息
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  ls      @date  2016/09/27
************************************************************/
DPP_STATUS dpp_stat_car_dbg_cnt_get(ZXIC_UINT32 dev_id,
                                    ZXIC_UINT32 car_type,
                                    DPP_STAT_CAR_DBG_CNT_T *p_car_dbg_cnt)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, car_type, STAT_CAR_A_TYPE, STAT_CAR_MAX_TYPE - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_car_dbg_cnt);

    switch (car_type)
    {
        case STAT_CAR_A_TYPE:
        {
            rc = dpp_stat_cara_dbg_cnt_get(dev_id,
                                           p_car_dbg_cnt);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_cara_dbg_cnt_get");
        }
        break;

        case STAT_CAR_B_TYPE:
        {
            rc = dpp_stat_carb_dbg_cnt_get(dev_id,
                                           p_car_dbg_cnt);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_carb_dbg_cnt_get");
        }
        break;

        case STAT_CAR_C_TYPE:
        {
            rc = dpp_stat_carc_dbg_cnt_get(dev_id,
                                           p_car_dbg_cnt);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_carc_dbg_cnt_get");
        }
        break;
    }

    return rc;
}
#endif
/***********************************************************/
/** 获取最大包长
* @param   dev_id
* @param   car_type
* @param   p_max_pkt_len
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  wll      @date  2019/06/06
************************************************************/
DPP_STATUS dpp_stat_car_max_pkt_size_get(DPP_DEV_T *dev,
                                         ZXIC_UINT32 car_type,
                                         ZXIC_UINT32 *p_max_pkt_len)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT32 pkt_len = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), car_type, STAT_CAR_A_TYPE, STAT_CAR_MAX_TYPE - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_max_pkt_len);

    switch (car_type)
    {
        case STAT_CAR_A_TYPE:
        {
            rc = dpp_stat_cara_max_pkt_size_get(dev,
                                                &pkt_len);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_cara_max_pkt_size_get");
        }
        break;

        case STAT_CAR_B_TYPE:
        {
            rc = dpp_stat_carb_max_pkt_size_get(dev,
                                                &pkt_len);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_carb_max_pkt_size_get");
        }
        break;

        case STAT_CAR_C_TYPE:
        {
            rc = dpp_stat_carc_max_pkt_size_get(dev,
                                                &pkt_len);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_carc_max_pkt_size_get");
        }
        break;
    }

    *p_max_pkt_len = pkt_len;

    return rc;
}

/***********************************************************/
/** 配置最大包长
* @param   dev_id
* @param   car_type
* @param   max_pkt_size
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  wll      @date  2019/06/06
************************************************************/
DPP_STATUS dpp_stat_car_max_pkt_size_set(DPP_DEV_T *dev,
                                         ZXIC_UINT32 car_type,
                                         ZXIC_UINT32 max_pkt_size)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), car_type, STAT_CAR_A_TYPE, STAT_CAR_MAX_TYPE - 1);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), max_pkt_size, 0, 0x3fff);

    switch (car_type)
    {
        case STAT_CAR_A_TYPE:
        {
            rc = dpp_stat_cara_max_pkt_size_set(dev,
                                                max_pkt_size);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_cara_max_pkt_size_set");
        }
        break;

        case STAT_CAR_B_TYPE:
        {
            rc = dpp_stat_carb_max_pkt_size_set(dev,
                                                max_pkt_size);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_carb_max_pkt_size_set");
        }
        break;

        case STAT_CAR_C_TYPE:
        {
            rc = dpp_stat_carc_max_pkt_size_set(dev,
                                                max_pkt_size);
            ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_stat_carc_max_pkt_size_set");
        }
        break;
    }

    return rc;
}


/***********************************************************/
/** STAT CAR复位获取全局变量大小函数
* @param   dev_id
* @param   p_size
*
* @return
* @remark  无
* @see
* @author  yxh      @date  2018/06/26
************************************************************/
DPP_STATUS dpp_stat_car_glb_size_get(DPP_DEV_T *dev, ZXIC_UINT32 *p_size)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 queue_num = 0;
    ZXIC_UINT32 profile_num = 0;
    ZXIC_UINT32 pkt_profile_num = 0;

    DPP_CAR_SOFT_RESET_DATA_T *p_g_restore_data = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_DEV_INDEX(DEV_ID(dev), DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(DEV_ID(dev), p_size);

    p_g_restore_data = GET_DPP_CAR_SOFT_RESET_INFO(DEV_ID(dev));

    if (0 == p_g_restore_data->is_init)
    {
        ZXIC_COMM_PRINT("Not init!!!\n");
        *p_size = sizeof(ZXIC_UINT32);
    }
    else
    {
        /* CAR A */
        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev), queue_num, p_g_restore_data->cara_flow_num);
        queue_num += p_g_restore_data->cara_flow_num;

        /* CAR B */
        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev), queue_num, p_g_restore_data->carb_flow_num);
        queue_num += p_g_restore_data->carb_flow_num;

        /* CAR C */
        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(DEV_ID(dev), queue_num, p_g_restore_data->carc_flow_num);
        queue_num += p_g_restore_data->carc_flow_num;

        pkt_profile_num = p_g_restore_data->car0_pkt_num;
        ZXIC_COMM_CHECK_DEV_INDEX_SUB_OVERFLOW_NO_ASSERT(DEV_ID(dev), (DPP_CAR_A_PROFILE_ID_MAX + DPP_CAR_B_PROFILE_ID_MAX + DPP_CAR_C_PROFILE_ID_MAX + 3), pkt_profile_num);
        profile_num = DPP_CAR_A_PROFILE_ID_MAX + DPP_CAR_B_PROFILE_ID_MAX + DPP_CAR_C_PROFILE_ID_MAX + 3 - pkt_profile_num;
        /**占用的格式如下:
        (ZXIC_UINT32)  (ZXIC_UINT32)               (ZXIC_UINT32)(CAR0)   (ZXIC_UINT32)(CAR1)                    (ZXIC_UINT32)
         is_init  queue_num  queue_info  pkt_profile_num  pkt_profile_num  pkt_profile_cfg  profile_num  profile_cfg
        */
        ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(DEV_ID(dev), queue_num, ((ZXIC_UINT32)ZXIC_SIZEOF(DPP_CAR_SOFT_RESET_QUEUE_T)));
        ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(DEV_ID(dev), pkt_profile_num, ((ZXIC_UINT32)ZXIC_SIZEOF(DPP_STAT_CAR_PKT_PROFILE_CFG_T)));
        ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(DEV_ID(dev), profile_num, ((ZXIC_UINT32)ZXIC_SIZEOF(DPP_STAT_CAR_PROFILE_CFG_T)));
        *p_size = ((ZXIC_UINT32)ZXIC_SIZEOF(ZXIC_UINT32)) * 5 + queue_num * ((ZXIC_UINT32)ZXIC_SIZEOF(DPP_CAR_SOFT_RESET_QUEUE_T))
                  + pkt_profile_num * ((ZXIC_UINT32)ZXIC_SIZEOF(DPP_STAT_CAR_PKT_PROFILE_CFG_T))
                  + profile_num * ((ZXIC_UINT32)ZXIC_SIZEOF(DPP_STAT_CAR_PROFILE_CFG_T));
        ZXIC_COMM_PRINT("glb_size = %d!!!\n", *p_size);
    }

    return rc;
}

#if 0
/***********************************************************/
/** STAT CAR复位设置全局变量函数
* @param   dev_id           设备号
* @param   size             大小，字节数
* @param   p_data_buff      全局变量数据
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  yxh      @date  2018/06/26
************************************************************/
DPP_STATUS dpp_stat_car_glb_mgr_set(ZXIC_UINT32 dev_id, ZXIC_UINT32 size, ZXIC_UINT8 *p_data_buff)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 is_init = 0;
    ZXIC_UINT32 queue_num = 0;
    ZXIC_UINT32 profile_num = 0;
    ZXIC_UINT32 cara_profile_num = 0;
    ZXIC_UINT32 car0_pkt_profile_num = 0;
    ZXIC_UINT32 buff_offset = 0;
    ZXIC_UINT32 size_of_stat_car = 0;

    DPP_CAR_SOFT_RESET_QUEUE_T *p_car_glb_queue_info = NULL;
    DPP_STAT_CAR_PROFILE_CFG_T *p_car_glb_profile_info = NULL;
    DPP_STAT_CAR_PKT_PROFILE_CFG_T *p_car_glb_pkt_profile_info = NULL;

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_data_buff);

    rc = dpp_stat_car_glb_size_get(dev_id, &size_of_stat_car);
    ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_car_glb_size_get");

    if (size < size_of_stat_car)
    {
        ZXIC_COMM_TRACE_ERROR("dpp stat car glb mgr recovery, size of buffer is smaller than defaultss\n");
        return DPP_ERR;
    }

    ZXIC_COMM_MEMCPY(&is_init, p_data_buff, size);
    buff_offset = sizeof(ZXIC_UINT32);

    if (0 == is_init)
    {
        ZXIC_COMM_PRINT("Not init!!!\n");
    }
    else
    {
        /* 获取queue的数目 */
        ZXIC_COMM_MEMCPY(&queue_num, p_data_buff + buff_offset, sizeof(ZXIC_UINT32));
        buff_offset += sizeof(ZXIC_UINT32);
        p_car_glb_queue_info = (DPP_CAR_SOFT_RESET_QUEUE_T *)(p_data_buff + buff_offset);

        /* 设置flow_id的信息 */
        for (i = 0; i < queue_num; i++)
        {
            rc = dpp_stat_car_queue_cfg_set(dev_id,
                                            p_car_glb_queue_info->car_type,
                                            p_car_glb_queue_info->flow_id,
                                            p_car_glb_queue_info->drop_flag,
                                            p_car_glb_queue_info->plcr_en,
                                            p_car_glb_queue_info->profile_id);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_car_queue_cfg_set");
        }

        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id, buff_offset, queue_num * ZXIC_SIZEOF(DPP_CAR_SOFT_RESET_QUEUE_T));
        buff_offset += (queue_num * ZXIC_SIZEOF(DPP_CAR_SOFT_RESET_QUEUE_T)) & 0xffffffff;

        ZXIC_COMM_MEMCPY(&car0_pkt_profile_num, p_data_buff + buff_offset, sizeof(ZXIC_UINT32));
        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id, buff_offset, sizeof(ZXIC_UINT32));
        buff_offset += sizeof(ZXIC_UINT32);

        /* 设置pkt_frofile的信息 */
        p_car_glb_pkt_profile_info = (DPP_STAT_CAR_PKT_PROFILE_CFG_T *)(p_data_buff + buff_offset);

        for (i = 0; i < car0_pkt_profile_num; i++)
        {
            rc = dpp_stat_car_profile_cfg_set(dev_id,
                                              STAT_CAR_A_TYPE,
                                              ZXIC_TRUE,
                                              p_car_glb_pkt_profile_info->profile_id,
                                              p_car_glb_pkt_profile_info);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_car_profile_cfg_set");
            p_car_glb_pkt_profile_info++;
        }

        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id, buff_offset, car0_pkt_profile_num * ZXIC_SIZEOF(DPP_STAT_CAR_PKT_PROFILE_CFG_T));
        buff_offset += (car0_pkt_profile_num  * ZXIC_SIZEOF(DPP_STAT_CAR_PKT_PROFILE_CFG_T)) & 0xffffffff;

        ZXIC_COMM_MEMCPY(&profile_num, p_data_buff + buff_offset, sizeof(ZXIC_UINT32));
        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id, buff_offset, sizeof(ZXIC_UINT32));
        buff_offset += sizeof(ZXIC_UINT32);

        /* 设置profile的信息 */
        p_car_glb_profile_info = (DPP_STAT_CAR_PROFILE_CFG_T *)(p_data_buff + buff_offset);

        cara_profile_num = DPP_CAR_PKT_PROFILE_ID_MAX - car0_pkt_profile_num;

        ZXIC_COMM_PRINT(">>>>>>>>>>> Set CAR_A_PROFILE info start!\n");

        for (j = 0; j < cara_profile_num; j++)
        {
            rc = dpp_stat_car_profile_cfg_set(dev_id,
                                                STAT_CAR_A_TYPE,
                                                ZXIC_FALSE,
                                                p_car_glb_profile_info->profile_id,
                                                p_car_glb_profile_info);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_car_profile_cfg_set");
            p_car_glb_profile_info++;
        }

        ZXIC_COMM_PRINT(">>>>>>>>>>> Set car CAR_B_PROFILE info start!\n");

        for (j = 0; j <= DPP_CAR_B_PROFILE_ID_MAX; j++)
        {
            rc = dpp_stat_car_profile_cfg_set(dev_id,
                                                STAT_CAR_B_TYPE,
                                                ZXIC_FALSE,
                                                p_car_glb_profile_info->profile_id,
                                                p_car_glb_profile_info);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_car_profile_cfg_set");
            p_car_glb_profile_info++;
        }

        ZXIC_COMM_PRINT(">>>>>>>>>>> Set car CAR_C_PROFILE info start!\n");

        for (j = 0; j <= DPP_CAR_C_PROFILE_ID_MAX; j++)
        {
            rc = dpp_stat_car_profile_cfg_set(dev_id,
                                                STAT_CAR_C_TYPE,
                                                ZXIC_FALSE,
                                                p_car_glb_profile_info->profile_id,
                                                p_car_glb_profile_info);
            ZXIC_COMM_CHECK_DEV_RC(dev_id, rc, "dpp_stat_car_profile_cfg_set");
            p_car_glb_profile_info++;
        }

    }/*  end (1 == is_init) */

    return rc;
}

/***********************************************************/
/** STAT CAR复位获取全局变量函数
* @param   dev_id               设备号
* @param   p_flag               释放使能，1-需要手动free，0-不需要手动free
* @param   p_size               数据大小
* @param   pp_data_buff         全局变量数据
*
* @return  NPE_OK-成功，NPE_ERR-失败
* @remark  无
* @see
* @author  yxh      @date  2018/06/26
************************************************************/
DPP_STATUS dpp_stat_car_glb_mgr_get(ZXIC_UINT32 dev_id,
                                    ZXIC_UINT32 *p_flag,
                                    ZXIC_UINT32 *p_size,
                                    ZXIC_UINT8 **pp_data_buff)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 j = 0;
    ZXIC_UINT32 is_init = 0;
    ZXIC_UINT32 flow_num = 0;
    ZXIC_UINT32 flow_num_total = 0;
    ZXIC_UINT32 plcr_en = 0;
    ZXIC_UINT32 drop_flag = 0;
    ZXIC_UINT32 profile_id = 0;
    ZXIC_UINT32 profile_num_total = 0;
    ZXIC_UINT32 profile_pkt_total = 0;
    ZXIC_UINT32 buff_offset = 0;
    ZXIC_UINT32 size = 0;
    DPP_CAR_SOFT_RESET_DATA_T *p_g_restore_data = NULL;
    DPP_CAR_SOFT_RESET_QUEUE_T *p_car_glb_queue_info = NULL;
    DPP_CAR_SOFT_RESET_QUEUE_T *p_car_glb_queue_info_temp = NULL;
    DPP_STAT_CAR_PROFILE_CFG_T *p_car_glb_profile_info = NULL;
    DPP_STAT_CAR_PROFILE_CFG_T *p_car_glb_profile_info_temp = NULL;
    DPP_STAT_CAR_PKT_PROFILE_CFG_T *p_car_glb_pkt_profile_info = NULL;
    DPP_STAT_CAR_PKT_PROFILE_CFG_T *p_car_glb_pkt_profile_info_temp = NULL;

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_size);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_flag);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, pp_data_buff);

    p_g_restore_data = GET_DPP_CAR_SOFT_RESET_INFO(dev_id);

    is_init = p_g_restore_data->is_init;

    if (0 == is_init)
    {
        *p_size = sizeof(ZXIC_UINT32);
        *p_flag = ZXIC_TRUE;

        ZXIC_COMM_PRINT(">>>>>>>>>>>[dpp_stat_car_glb_mgr_get]: No init\n");

        *pp_data_buff = ZXIC_COMM_MALLOC(*p_size);
        ZXIC_COMM_CHECK_DEV_POINT(dev_id, *pp_data_buff);
        ZXIC_COMM_MEMSET(*pp_data_buff, 0, *p_size);

        ZXIC_COMM_MEMCPY(*pp_data_buff, &is_init, sizeof(ZXIC_UINT32));
    }
    else
    {
        ZXIC_COMM_PRINT(">>>>>>>>>>>[dpp_stat_car_glb_mgr_get]: Inited\n");

        /** queue 绑定的信息 */
        /* CAR A */
        flow_num = p_g_restore_data->cara_flow_num;
        ZXIC_COMM_TRACE_DEV_INFO(dev_id, "[dpp_stat_car_glb_mgr_get] cara_queue_num     : 0x%08x\n", flow_num);
        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id, flow_num_total, flow_num);
        flow_num_total += flow_num;

        /* CAR B */
        flow_num = p_g_restore_data->carb_flow_num;
        ZXIC_COMM_TRACE_DEV_INFO(dev_id, "[dpp_stat_car_glb_mgr_get] carb_queue_num     : 0x%08x\n", flow_num);
        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id, flow_num_total, flow_num);
        flow_num_total += flow_num;

        /* CAR C */
        flow_num = p_g_restore_data->carc_flow_num;
        ZXIC_COMM_TRACE_DEV_INFO(dev_id, "[dpp_stat_car_glb_mgr_get] carc_queue_num     : 0x%08x\n", flow_num);
        ZXIC_COMM_CHECK_DEV_INDEX_ADD_OVERFLOW_NO_ASSERT(dev_id, flow_num_total, flow_num);
        flow_num_total += flow_num;

        profile_pkt_total = p_g_restore_data->car0_pkt_num;
        ZXIC_COMM_CHECK_DEV_INDEX_SUB_OVERFLOW_NO_ASSERT(dev_id, DPP_CAR_PROFILE_ID_TOTAL, profile_pkt_total);
        profile_num_total = DPP_CAR_PROFILE_ID_TOTAL - profile_pkt_total;

        /**占用的格式如下:
        (ZXIC_UINT32)  (ZXIC_UINT32)               (ZXIC_UINT32)(CAR0)   (ZXIC_UINT32)(CAR1)                    (ZXIC_UINT32)
         is_init  queue_num  queue_info  pkt_profile_num  pkt_profile_num  pkt_profile_cfg  profile_num  profile_cfg
        */
        ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, flow_num_total, ((ZXIC_UINT32)ZXIC_SIZEOF(DPP_CAR_SOFT_RESET_QUEUE_T)));
        ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, profile_pkt_total, ((ZXIC_UINT32)ZXIC_SIZEOF(DPP_STAT_CAR_PKT_PROFILE_CFG_T)));
        ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, profile_num_total, ((ZXIC_UINT32)ZXIC_SIZEOF(DPP_STAT_CAR_PROFILE_CFG_T)));
        *p_size = ((ZXIC_UINT32)ZXIC_SIZEOF(ZXIC_UINT32)) * 5 + flow_num_total * ((ZXIC_UINT32)ZXIC_SIZEOF(DPP_CAR_SOFT_RESET_QUEUE_T))
                  + profile_pkt_total * ((ZXIC_UINT32)ZXIC_SIZEOF(DPP_STAT_CAR_PKT_PROFILE_CFG_T))
                  + profile_num_total * ((ZXIC_UINT32)ZXIC_SIZEOF(DPP_STAT_CAR_PROFILE_CFG_T));

        /* 存放init_flag */
        *pp_data_buff = ZXIC_COMM_MALLOC(*p_size);
        ZXIC_COMM_CHECK_DEV_POINT(dev_id, *pp_data_buff);
        ZXIC_COMM_MEMSET(*pp_data_buff, 0, *p_size);
        ZXIC_COMM_MEMCPY(*pp_data_buff, &is_init, sizeof(ZXIC_UINT32));
        buff_offset = sizeof(ZXIC_UINT32);

        /* 存放queue_num */
        ZXIC_COMM_MEMCPY(*pp_data_buff + buff_offset, &flow_num_total, sizeof(ZXIC_UINT32));
        buff_offset += sizeof(ZXIC_UINT32);

        /* 获取、存放queue 绑定的信息 */
        if (0 != flow_num_total)
        {
            ZXIC_COMM_PRINT(">>>>>>>>>>>[dpp_stat_car_glb_mgr_get]: Get queue info start!\n");
            ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, flow_num_total, ((ZXIC_UINT32)ZXIC_SIZEOF(DPP_CAR_SOFT_RESET_QUEUE_T)));
            p_car_glb_queue_info = (DPP_CAR_SOFT_RESET_QUEUE_T *)ZXIC_COMM_MALLOC(flow_num_total * ((ZXIC_UINT32)ZXIC_SIZEOF(DPP_CAR_SOFT_RESET_QUEUE_T)));
            ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_car_glb_queue_info);
            ZXIC_COMM_MEMSET(p_car_glb_queue_info, 0, flow_num_total * sizeof(DPP_CAR_SOFT_RESET_QUEUE_T));
            p_car_glb_queue_info_temp = p_car_glb_queue_info;

            /* CAR A */
            flow_num = p_g_restore_data->cara_flow_num;

            for (j = 0; j < flow_num; j++)
            {
                rc = dpp_stat_car_queue_cfg_get(dev_id,
                                                STAT_CAR_A_TYPE,
                                                p_g_restore_data->cara_item[j].flow_id,
                                                &drop_flag,
                                                &plcr_en,
                                                &profile_id);
                if (DPP_OK != rc)
                {
                    ZXIC_COMM_FREE(p_car_glb_queue_info);
                    ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ICM  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,"dpp_stat_car_queue_cfg_get");
                    return rc;
                }

                p_car_glb_queue_info_temp->car_type = STAT_CAR_A_TYPE;
                p_car_glb_queue_info_temp->flow_id = p_g_restore_data->cara_item[j].flow_id;
                p_car_glb_queue_info_temp->drop_flag = drop_flag;
                p_car_glb_queue_info_temp->plcr_en = plcr_en;
                p_car_glb_queue_info_temp->profile_id = profile_id;
                p_car_glb_queue_info_temp++;
                ZXIC_COMM_TRACE_DEV_INFO(dev_id, "carA used flow_id[%d]     : 0x%08x\n", j, p_g_restore_data->cara_item[j].flow_id);
            }

            /* CAR B */
            flow_num = p_g_restore_data->carb_flow_num;

            for (j = 0; j < flow_num; j++)
            {
                rc = dpp_stat_car_queue_cfg_get(dev_id,
                                                STAT_CAR_B_TYPE,
                                                p_g_restore_data->carb_item[j].flow_id,
                                                &drop_flag,
                                                &plcr_en,
                                                &profile_id);
                if (DPP_OK != rc)
                {
                    ZXIC_COMM_FREE(p_car_glb_queue_info);
                    ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ICM  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,"dpp_stat_car_queue_cfg_get");
                    return rc;
                }

                p_car_glb_queue_info_temp->car_type = STAT_CAR_B_TYPE;
                p_car_glb_queue_info_temp->flow_id = p_g_restore_data->carb_item[j].flow_id;
                p_car_glb_queue_info_temp->drop_flag = drop_flag;
                p_car_glb_queue_info_temp->plcr_en = plcr_en;
                p_car_glb_queue_info_temp->profile_id = profile_id;
                p_car_glb_queue_info_temp++;
                ZXIC_COMM_TRACE_DEV_INFO(dev_id, "carB used flow_id[%d]     : 0x%08x\n", j, p_g_restore_data->carb_item[j].flow_id);
            }

            /* CAR C */
            flow_num = p_g_restore_data->carc_flow_num;

            for (j = 0; j < flow_num; j++)
            {
                rc = dpp_stat_car_queue_cfg_get(dev_id,
                                                STAT_CAR_C_TYPE,
                                                p_g_restore_data->carc_item[j].flow_id,
                                                &drop_flag,
                                                &plcr_en,
                                                &profile_id);
                if (DPP_OK != rc)
                {
                    ZXIC_COMM_FREE(p_car_glb_queue_info);
                    ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ICM  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,"dpp_stat_car_queue_cfg_get");
                    return rc;
                }

                p_car_glb_queue_info_temp->car_type = STAT_CAR_C_TYPE;
                p_car_glb_queue_info_temp->flow_id = p_g_restore_data->carc_item[j].flow_id;
                p_car_glb_queue_info_temp->drop_flag = drop_flag;
                p_car_glb_queue_info_temp->plcr_en = plcr_en;
                p_car_glb_queue_info_temp->profile_id = profile_id;
                p_car_glb_queue_info_temp++;
                ZXIC_COMM_TRACE_DEV_INFO(dev_id, "carC used flow_id[%d]     : 0x%08x\n", j, p_g_restore_data->carb_item[j].flow_id);
            }

            ZXIC_COMM_MEMCPY(*pp_data_buff + buff_offset, p_car_glb_queue_info, flow_num_total * sizeof(DPP_CAR_SOFT_RESET_QUEUE_T));
            if((0xFFFFFFFF - (buff_offset)) < (flow_num_total * ZXIC_SIZEOF(DPP_CAR_SOFT_RESET_QUEUE_T)))
            {
                ZXIC_COMM_FREE(p_car_glb_queue_info);
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id,  "ICM %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, buff_offset, flow_num_total * ZXIC_SIZEOF(DPP_CAR_SOFT_RESET_QUEUE_T));
                return ZXIC_PAR_CHK_INVALID_INDEX;
            }
            buff_offset += (flow_num_total * ZXIC_SIZEOF(DPP_CAR_SOFT_RESET_QUEUE_T)) % 0xffffffff;
        }

        ZXIC_COMM_PRINT(">>>>>>>>>>>[dpp_stat_car_glb_mgr_get]: total flow_num = %d !\n", flow_num_total);

        /** profile 的信息 */
        if (0 != profile_pkt_total)
        {
            p_car_glb_pkt_profile_info = (DPP_STAT_CAR_PKT_PROFILE_CFG_T *)ZXIC_COMM_MALLOC(profile_pkt_total * ZXIC_SIZEOF(DPP_STAT_CAR_PKT_PROFILE_CFG_T));
            if (NULL == (p_car_glb_pkt_profile_info))
            {
                ZXIC_COMM_FREE(p_car_glb_queue_info);
                ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ICM %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);
                return ZXIC_PAR_CHK_POINT_NULL;
            }

            ZXIC_COMM_MEMSET(p_car_glb_pkt_profile_info, 0, profile_pkt_total * sizeof(DPP_STAT_CAR_PKT_PROFILE_CFG_T));
            p_car_glb_pkt_profile_info_temp = p_car_glb_pkt_profile_info;
        }
        size = ZXIC_SIZEOF(DPP_STAT_CAR_PROFILE_CFG_T);
        ZXIC_COMM_CHECK_DEV_INDEX_MUL_OVERFLOW_NO_ASSERT(dev_id, profile_num_total, size);
        p_car_glb_profile_info = (DPP_STAT_CAR_PROFILE_CFG_T *)ZXIC_COMM_MALLOC(profile_num_total * size);
        if (NULL == (p_car_glb_profile_info))
        {
            ZXIC_COMM_FREE(p_car_glb_queue_info);
            ZXIC_COMM_FREE(p_car_glb_pkt_profile_info);
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ICM %s:%d[Error:POINT NULL] !\n",__FILE__,__LINE__);
            return ZXIC_PAR_CHK_POINT_NULL;
        }

        ZXIC_COMM_MEMSET(p_car_glb_profile_info, 0, profile_num_total * sizeof(DPP_STAT_CAR_PROFILE_CFG_T));
        p_car_glb_profile_info_temp = p_car_glb_profile_info;

        ZXIC_COMM_PRINT(">>>>>>>>>>>[dpp_stat_car_glb_mgr_get]: Get profile info start!\n");
        ZXIC_COMM_PRINT(">>>>>>>>>>> Get car CAR_A_PROFILE info start!\n");

        for (j = 0; j <= DPP_CAR_A_PROFILE_ID_MAX; j++)
        {
            /* (0 != profile_pkt_total) -- Coverity err : 递增 null 指针 p_car_glb_pkt_profile_info_temp */
            if ((0 != profile_pkt_total) && (ZXIC_TRUE == p_g_restore_data->car_pkt_sign[j]))
            {
                ZXIC_COMM_TRACE_DEV_INFO(dev_id, "car profile_id[%d] is pkt_profile!!!!\n", j);
                rc = dpp_stat_car_profile_cfg_get(dev_id,
                                                    STAT_CAR_A_TYPE,
                                                    ZXIC_TRUE,
                                                    j,
                                                    p_car_glb_pkt_profile_info_temp);
                if (DPP_OK != rc)
                {
                    ZXIC_COMM_FREE(p_car_glb_queue_info);
                    ZXIC_COMM_FREE(p_car_glb_pkt_profile_info);
                    ZXIC_COMM_FREE(p_car_glb_profile_info);
                    ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ICM  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,"dpp_stat_car_profile_cfg_get");
                    return rc;
                }

                p_car_glb_pkt_profile_info_temp++;
            }
        }

        for (j = 0; j <= DPP_CAR_A_PROFILE_ID_MAX; j++)
        {
            if (ZXIC_TRUE != p_g_restore_data->car_pkt_sign[j])
            {
                rc = dpp_stat_car_profile_cfg_get(dev_id,
                                                    STAT_CAR_A_TYPE,
                                                    ZXIC_FALSE,
                                                    j,
                                                    p_car_glb_profile_info_temp);
                if (DPP_OK != rc)
                {
                    ZXIC_COMM_FREE(p_car_glb_queue_info);
                    ZXIC_COMM_FREE(p_car_glb_pkt_profile_info);
                    ZXIC_COMM_FREE(p_car_glb_profile_info);
                    ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ICM  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,"dpp_stat_car_profile_cfg_get");
                    return rc;
                }

                p_car_glb_profile_info_temp++;
            }
        }

        ZXIC_COMM_PRINT(">>>>>>>>>>> Get car CAR_B_PROFILE info start!\n");

        for (j = 0; j <= DPP_CAR_B_PROFILE_ID_MAX; j++)
        {
            rc = dpp_stat_car_profile_cfg_get(dev_id,
                                                STAT_CAR_B_TYPE,
                                                ZXIC_FALSE,
                                                j,
                                                p_car_glb_profile_info_temp);
        if (DPP_OK != rc)
        {
            ZXIC_COMM_FREE(p_car_glb_queue_info);
            ZXIC_COMM_FREE(p_car_glb_pkt_profile_info);
            ZXIC_COMM_FREE(p_car_glb_profile_info);
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ICM  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,"dpp_stat_car_profile_cfg_get");
            return rc;
        }

            p_car_glb_profile_info_temp++;
        }

        ZXIC_COMM_PRINT(">>>>>>>>>>> Get car CAR_C_PROFILE info start!\n");

        for (j = 0; j <= DPP_CAR_C_PROFILE_ID_MAX; j++)
        {
            rc = dpp_stat_car_profile_cfg_get(dev_id,
                                                STAT_CAR_C_TYPE,
                                                ZXIC_FALSE,
                                                j,
                                                p_car_glb_profile_info_temp);
        if (DPP_OK != rc)
        {
            ZXIC_COMM_FREE(p_car_glb_queue_info);
            ZXIC_COMM_FREE(p_car_glb_pkt_profile_info);
            ZXIC_COMM_FREE(p_car_glb_profile_info);
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id,"\n ICM  %s:%d [ErrorCode:0x%x] !-- %s Call %s Fail!\n",__FILE__,__LINE__,rc,__FUNCTION__,"dpp_stat_car_profile_cfg_get");
            return rc;
        }
            p_car_glb_profile_info_temp++;
        }

        ZXIC_COMM_MEMCPY(*pp_data_buff + buff_offset, &(p_g_restore_data->car0_pkt_num), sizeof(ZXIC_UINT32));
        buff_offset += sizeof(ZXIC_UINT32);

        if((0xFFFFFFFF - (buff_offset)) < (sizeof(ZXIC_UINT32)))
        {
            ZXIC_COMM_FREE(p_car_glb_queue_info);
            ZXIC_COMM_FREE(p_car_glb_profile_info);
            ZXIC_COMM_FREE(p_car_glb_pkt_profile_info);
            ZXIC_COMM_TRACE_DEV_ERROR(dev_id,  "ICM %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, buff_offset, sizeof(ZXIC_UINT32));
            return ZXIC_PAR_CHK_INVALID_INDEX;
        }
        buff_offset += sizeof(ZXIC_UINT32);

        if (0 != profile_pkt_total)
        {
            ZXIC_COMM_MEMCPY(*pp_data_buff + buff_offset, p_car_glb_pkt_profile_info, profile_pkt_total * sizeof(DPP_STAT_CAR_PKT_PROFILE_CFG_T));
            buff_offset += profile_pkt_total * ZXIC_SIZEOF(DPP_STAT_CAR_PKT_PROFILE_CFG_T);
        }

        ZXIC_COMM_PRINT(">>>>>>>>>>>[dpp_stat_car_glb_mgr_get]: total pkt_profile = %d !\n", profile_pkt_total);

        ZXIC_COMM_MEMCPY(*pp_data_buff + buff_offset, &profile_num_total, sizeof(ZXIC_UINT32));
        if((0xFFFFFFFF - (buff_offset)) < (sizeof(ZXIC_UINT32)))
        {
           ZXIC_COMM_FREE(p_car_glb_queue_info);
		   ZXIC_COMM_FREE(p_car_glb_profile_info);
		   ZXIC_COMM_FREE(p_car_glb_pkt_profile_info);
           ZXIC_COMM_TRACE_DEV_ERROR(dev_id,  "ICM %s:%d[Error:VALUE[val0=0x%x] INVALID] [val1=0x%x] !\n", __FILE__, __LINE__, buff_offset, sizeof(ZXIC_UINT32));
           return ZXIC_PAR_CHK_INVALID_INDEX;
        }
        buff_offset += sizeof(ZXIC_UINT32);

        if (0 != profile_num_total)
        {
            ZXIC_COMM_PRINT(">>>>>>>>>>>[dpp_stat_car_glb_mgr_get]: total profile = %d !\n", profile_num_total);
            ZXIC_COMM_MEMCPY(*pp_data_buff + buff_offset, p_car_glb_profile_info, profile_num_total * sizeof(DPP_STAT_CAR_PROFILE_CFG_T));
            buff_offset += profile_num_total * ZXIC_SIZEOF(DPP_STAT_CAR_PROFILE_CFG_T);
        }

        ZXIC_COMM_PRINT(">>>>>>>>>>>[dpp_stat_car_glb_mgr_get]: total profile = %d !\n", profile_num_total);
        ZXIC_COMM_PRINT(">>>>>>>>>>>[dpp_stat_car_glb_mgr_get]: buff_offset= %d !\n", buff_offset);

        *p_flag = ZXIC_TRUE;

        ZXIC_COMM_FREE(p_car_glb_queue_info);

        if (0 != profile_pkt_total)
        {
            ZXIC_COMM_FREE(p_car_glb_pkt_profile_info);
        }

        ZXIC_COMM_FREE(p_car_glb_profile_info);
    }

    ZXIC_COMM_PRINT(">>>>>>>>>>>[dpp_stat_car_glb_mgr_get]: END! total size = %d !\n", *p_size);

    return rc;
}

#endif
#endif

#if ZXIC_REAL("TEST")

#if 0
/***********************************************************/
/**
* @param   p_rb_cfg
*
* @return
* @remark  无
* @see
* @author  XXX      @date  2017/10/31
************************************************************/
DPP_STATUS dpp_stat_queue_rb_root_prt(ZXIC_RB_CFG *p_rb_cfg)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 i = 0;
    ZXIC_UINT32 dev_id = 0;

    DPP_CAR_QUEUE_RB_KEY_T *p_tmp_car_queue_rb_key = NULL;

    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_rb_cfg);

    if (NULL != p_rb_cfg->p_root)
    {
        p_tmp_car_queue_rb_key = (DPP_CAR_QUEUE_RB_KEY_T *)p_rb_cfg->p_root->p_key;

        if (NULL != p_tmp_car_queue_rb_key)
        {
            ZXIC_COMM_PRINT("dpp_stat_queue_rb_root_prt[GET] car_queue_cfg:\n");

            for (i = 0; i < DPP_CAR_QUEUE_CFG_ZXIC_UINT8; i++)
            {
                ZXIC_COMM_PRINT("%02x\t", p_tmp_car_queue_rb_key->profile_cfg[i]);

                if (0 ==  (i + 1) % 4)
                {
                    ZXIC_COMM_PRINT("\n");
                }
            }

            ZXIC_COMM_PRINT("\n");

        }

    }

    return rc;
}

/***********************************************************/
/**
* @param   p_rb_cfg
*
* @return
* @remark  无
* @see
* @author  XXX      @date  2017/10/31
************************************************************/
DPP_STATUS dpp_stat_car_profile_id_rb_root_prt(ZXIC_UINT32 dev_id, ZXIC_RB_CFG *p_rb_cfg)
{
    DPP_STATUS rc = DPP_OK;

    ZXIC_UINT32 i = 0;

    DPP_CAR_PROFILE_ID_RB_KEY_T *p_tmp_car_profile_id_rb_key = NULL;
    ZXIC_RB_TN                   *p_car_rb_node               = NULL;
    DPP_CAR_PROFILE_RB_KEY_T    *p_car_profile_rb_key        = NULL;

    ZXIC_COMM_CHECK_DEV_INDEX(dev_id, dev_id, 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_DEV_POINT(dev_id, p_rb_cfg);

    if (NULL != p_rb_cfg->p_root)
    {
        p_tmp_car_profile_id_rb_key = (DPP_CAR_PROFILE_ID_RB_KEY_T *)p_rb_cfg->p_root->p_key;

        if (NULL != p_tmp_car_profile_id_rb_key)
        {
            /* ZXIC_COMM_PRINT("dpp_stat_car_profile_id_rb_root_prt pp_car_node ADDRESS: 0x%08x\n", (ZXIC_UINT32)p_tmp_car_profile_id_rb_key->p_car_node); */
            p_car_rb_node = (ZXIC_RB_TN * )p_tmp_car_profile_id_rb_key->p_car_node;

            if (NULL != p_car_rb_node)
            {
                p_car_profile_rb_key = (DPP_CAR_PROFILE_RB_KEY_T *)p_car_rb_node->p_key;

                if (NULL != p_car_profile_rb_key)
                {
                    for (i = 0; i < DPP_CAR_PROFILE_CFG_ZXIC_UINT32; i++)
                    {
                        ZXIC_COMM_PRINT("%08x\t", p_car_profile_rb_key->profile_cfg[i]);

                        if (0 == (i + 1) % 4)
                        {
                            ZXIC_COMM_PRINT("\n");
                        }
                    }

                    ZXIC_COMM_PRINT("\n");
                }
            }
        }
    }

    return rc;
}

#endif
#endif




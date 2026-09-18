//#include <linux/device.h>
#include <linux/dinghai/kcompat.h>
#if defined(CHECK_DCB_ENABLED) && !defined(CONFIG_DCB)
#include <net/dcbnl.h>
#endif
#ifdef CGS_V5_693
#ifndef IEEE_8021QAZ_APP_SEL_DSCP
#define IEEE_8021QAZ_APP_SEL_DSCP  5
#endif
#endif
#include "../../en_aux.h"
#include "en_dcbnl.h"
#include "en_np/qos/include/dpp_drv_qos.h"
#include "en_aux/en_aux_cmd.h"
#include "en_dcbnl_api.h"
#include "en_aux/en_aux_events.h"

uint32_t g_maxrate_num= 0;
static int zxdh_dcbnl_ieee_getets(struct net_device *netdev, struct ieee_ets *ets)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint32_t tc = 0;
    uint32_t j = 0;

    if (!zxdh_en_is_panel_port(en_dev))
        return 0;

    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_dcbnl_ieee_getets: coredev type is not a PF");
        return -EOPNOTSUPP;
    }

    ets->willing = 0;

    ets->ets_cap = ZXDH_DCBNL_MAX_TRAFFIC_CLASS;

    memcpy(ets->tc_tsa, en_dev->dcb_para.ets_cfg.tc_tsa, sizeof(ets->tc_tsa));
    memcpy(ets->tc_tx_bw, en_dev->dcb_para.ets_cfg.tc_tx_bw, sizeof(ets->tc_tx_bw));
    memcpy(ets->prio_tc, en_dev->dcb_para.ets_cfg.prio_tc, sizeof(ets->prio_tc));

    for (tc = 0; tc < ZXDH_DCBNL_MAX_TRAFFIC_CLASS; tc++)
    {
        if (ets->tc_tsa[tc] != IEEE_8021QAZ_TSA_ETS)
        {
            ets->tc_tx_bw[tc] = 0;
        }
    }

    /* debug */
    for (j = 0; j < ZXDH_DCBNL_MAX_TRAFFIC_CLASS; j++)
    {
        LOG_DEBUG_DEV(en_dev->parent, " idx:%d, ets->tc_tsa:%d, ets->tc_tx_bw:%d, ets->prio_tc:%d \n", j,
              ets->tc_tsa[j], ets->tc_tx_bw[j], ets->prio_tc[j]);
    }

    return 0;
}

static int zxdh_dcbnl_check_ets_maxtc(struct ieee_ets *ets)
{
    uint32_t i;

    for (i = 0; i < ZXDH_DCBNL_MAX_PRIORITY; i++)
    {
        if (ets->prio_tc[i] >= ZXDH_DCBNL_MAX_TRAFFIC_CLASS)
        {
            LOG_ERR("dcbnl_check_ets: Failed! TC value greater than max(%d)\n", ZXDH_DCBNL_MAX_TRAFFIC_CLASS);
            return 1;
        }
    }
    return 0;
}

static int zxdh_dcbnl_check_ets_tcbw(struct ieee_ets *ets)
{
    bool have_ets_tc = false;
    uint32_t bw_sum = 0;
    uint32_t i;

    for (i = 0; i < ZXDH_DCBNL_MAX_TRAFFIC_CLASS; i++)
    {
        if (ets->tc_tsa[i] == IEEE_8021QAZ_TSA_ETS)
        {
            have_ets_tc = true;
            bw_sum += ets->tc_tx_bw[i];
        }
    }

    if (have_ets_tc && ((bw_sum != 100) && (bw_sum != 0)))
    {
        LOG_ERR("dcbnl_check_ets_tcbw: Failed! ETS BW sum is illegal\n");
        return 1;
    }

    return 0;
}

static int zxdh_dcbnl_check_ets_para(struct ieee_ets *ets)
{
    uint32_t err = 0;

    err = zxdh_dcbnl_check_ets_maxtc(ets);
    if (err)
    {
        return -EINVAL;
    }

    err = zxdh_dcbnl_check_ets_tcbw(ets);
    if (err)
    {
        return -EINVAL;
    }
    LOG_INFO("%s end \n", __func__);
    return 0;
}


static int zxdh_dcbnl_ieee_divide_tc_type(struct ieee_ets *ets, uint8_t *tc_type)
{
    uint32_t i;

    for (i = 0; i < ZXDH_DCBNL_MAX_TRAFFIC_CLASS; i++)
    {
        switch (ets->tc_tsa[i])
        {
        case IEEE_8021QAZ_TSA_ETS:
            tc_type[i] = ets->tc_tx_bw[i] ? ZXDH_DCBNL_ETS_TC : ZXDH_DCBNL_ZEROBW_ETS_TC;
            break;
        case IEEE_8021QAZ_TSA_STRICT:
            tc_type[i] = ZXDH_DCBNL_STRICT_TC;
            break;
        case IEEE_8021QAZ_TSA_VENDOR:
            tc_type[i] = ZXDH_DCBNL_VENDOR_TC;
            break;
         default:
            tc_type[i] = ZXDH_DCBNL_STRICT_TC;
            LOG_ERR("dcbnl: %d tsa error, change to strict \n", ets->tc_tsa[i]);
            break;
        }
    }

    return 0;
}

static int zxdh_dcbnl_ieee_convert_tc_bw(struct ieee_ets *ets, uint8_t *tc_type, uint8_t *tc_tx_bw)
{
    uint32_t i;
    uint8_t zero_ets_bw = 0;
    uint8_t zero_ets_num = 0;

    for (i = 0; i < ZXDH_DCBNL_MAX_TRAFFIC_CLASS; i++)
    {
        if (tc_type[i] == ZXDH_DCBNL_ZEROBW_ETS_TC)
        {
            zero_ets_num++;
        }
    }

    if (zero_ets_num)
    {
        zero_ets_bw = (uint8_t)ZXDH_DCBNL_MAX_BW_ALLOC / zero_ets_num;
    }

    for (i = 0; i < ZXDH_DCBNL_MAX_TRAFFIC_CLASS; i++)
    {
        switch (tc_type[i])
        {
            case ZXDH_DCBNL_ZEROBW_ETS_TC:
                tc_tx_bw[i] = zero_ets_bw;
                break;
            case ZXDH_DCBNL_ETS_TC:
                tc_tx_bw[i] = ets->tc_tx_bw[i];
                break;
            case ZXDH_DCBNL_STRICT_TC:
            case ZXDH_DCBNL_VENDOR_TC:
                tc_tx_bw[i] = ZXDH_DCBNL_MAX_BW_ALLOC;
                break;
            default:
                break;
        }
    }
    /* debug */
    LOG_INFO(" zero_ets_num:%d, zero_ets_bw:%d \n", zero_ets_num, zero_ets_bw);

    return 0;
}

static uint32_t zxdh_dcbnl_ieee_set_ets_para(struct zxdh_en_priv *en_priv, struct ieee_ets *ets)
{
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint8_t tc_type[ZXDH_DCBNL_MAX_TRAFFIC_CLASS];
    uint8_t tc_tx_bw[ZXDH_DCBNL_MAX_TRAFFIC_CLASS];
    uint32_t err = 0;
    uint32_t j = 0;

    zxdh_dcbnl_ieee_divide_tc_type(ets, tc_type);

    zxdh_dcbnl_ieee_convert_tc_bw(ets, tc_type, tc_tx_bw);

    if (zxdh_en_is_panel_port(en_dev))
    {
        err = zxdh_dcbnl_set_tc_scheduling(en_priv, tc_type, tc_tx_bw);
        if (err)
        {
            LOG_ERR_DEV(en_dev->parent, "set_tc_scheduling failed \n");
            return err;
        }

        err = zxdh_dcbnl_set_ets_up_tc_map(en_priv, ets->prio_tc);
        if (err)
        {
            LOG_ERR_DEV(en_dev->parent, "set_prio_tc_map failed \n");
            return err;
        }
    }

    memcpy(en_dev->dcb_para.ets_cfg.tc_tsa, ets->tc_tsa, sizeof(ets->tc_tsa));
    memcpy(en_dev->dcb_para.ets_cfg.tc_tx_bw, ets->tc_tx_bw, sizeof(ets->tc_tx_bw));
    memcpy(en_dev->dcb_para.ets_cfg.prio_tc, ets->prio_tc, sizeof(ets->prio_tc));
    /* debug */
    for (j = 0; j < ZXDH_DCBNL_MAX_TRAFFIC_CLASS; j++)
    {
        LOG_DEBUG_DEV(en_dev->parent, " idx:%d, tc_tsa:%d, tc_tx_bw:%d, prio_tc:%d \n", j,
              en_dev->dcb_para.ets_cfg.tc_tsa[j], en_dev->dcb_para.ets_cfg.tc_tx_bw[j], en_dev->dcb_para.ets_cfg.prio_tc[j]);

        LOG_DEBUG_DEV(en_dev->parent, " idx:%d, tc_type:%d, tc_tx_bw:%d \n", j, tc_type[j], tc_tx_bw[j]);
    }

    return 0;
}

static int zxdh_dcbnl_ieee_rdma_setets(struct net_device *netdev, struct ieee_ets *ets)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct zxdh_dcbnl_ieee_rdma_ets rdma_ets_para = {0};
    uint32_t err = 0;
    uint32_t j = 0;

    rdma_ets_para.port = en_dev->phy_port;
    for (j = 0; j < ZXDH_DCBNL_MAX_TRAFFIC_CLASS; ++j) 
    {
        rdma_ets_para.tc_tsa[j] = ets->tc_tsa[j];
        rdma_ets_para.tc_tx_bw[j] = ets->tc_tx_bw[j];
        rdma_ets_para.prio_tc[j] = ets->prio_tc[j];
    }

    rdma_ets_para.mode = en_dev->rdma_ets_flag;

    err = zxdh_rdma_events_call(en_dev->netdev, ZXDH_RDMA_ETS_EVENT, &rdma_ets_para);
    if (err) 
    {
        LOG_ERR_DEV(en_dev->parent, "Failed to set RDMA ETS parameters: %u\n", err);
    }

    return err;
}

static int zxdh_dcbnl_ieee_setets(struct net_device *netdev, struct ieee_ets *ets)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint32_t err;
    uint32_t j = 0;

    if (!zxdh_en_is_panel_port(en_dev))
        return 0;

    /* debug */
    for (j = 0; j < ZXDH_DCBNL_MAX_TRAFFIC_CLASS; j++)
    {
        LOG_DEBUG_DEV(en_dev->parent, " idx:%d, ets->tc_tsa:%d, ets->tc_tx_bw:%d, ets->prio_tc:%d \n", j,
                 ets->tc_tsa[j], ets->tc_tx_bw[j], ets->prio_tc[j]);
    }

    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        LOG_ERR_DEV(en_dev->parent, " coredev type is not a PF");
        return -EOPNOTSUPP;
    }

    err = zxdh_dcbnl_check_ets_para(ets);
    if (err)
    {
        return err;
    }

    err = zxdh_dcbnl_ieee_set_ets_para(en_priv, ets);
    if (err)
    {
        return err;
    }

    /*同步配置rdma ets参数*/
    err = zxdh_dcbnl_ieee_rdma_setets(netdev, ets);
    if (err) 
    {
        LOG_ERR_DEV(en_dev->parent, "Failed to set RDMA ETS parameters: %u\n", err);
        return err;
    }

    return 0;
}

static int zxdh_dcbnl_ieee_getpfc(struct net_device *netdev, struct ieee_pfc *pfc)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint32_t pfc_cur_mac_en = 0;
    int32_t ret = 0;
    bool pfc_map_support = false;
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;
    pfc_map_support = (en_dev->board_type == DH_STD_E318) && en_dev->ops->is_fw_feature_support(en_dev->parent, FW_FEATURE_NP_NPPU_TCAM_PFC_MAP);

    /*获取端口pfc使能函数*/
    ret = zxdh_en_fc_mode_get(en_dev, &pfc_cur_mac_en);

    if(0 != ret)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_port_pfc_enable_get failed");
        return ret;
    }

    if (pfc_cur_mac_en == BIT(SPM_FC_PFC_FULL))
    {
        if (pfc_map_support && zxdh_en_is_panel_port(en_dev))
        {
            ret = (int32_t)dpp_pktrx_tcam_pfc_get(&pf_info, &pfc->pfc_en);
            if (0 != ret)
            {
                LOG_ERR_DEV(en_dev->parent, "dpp_pktrx_tcam_pfc_get failed, ret = %u", ret);
                return ret;
            }
        }
        else
        {
            pfc->pfc_en = 0xff;
        }
    }
    else
    {
        pfc->pfc_en = 0;
    }

    /*ieee要的最多8个优先级，最大延迟为7*/
    pfc->pfc_cap = 8;
    pfc->delay = 7;

    return ret;
}

static int zxdh_dcbnl_ieee_setpfc(struct net_device *netdev, struct ieee_pfc *pfc)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint32_t port_mac_en = 0;
    uint32_t cur_port_mac_en = 0;
    int32_t ret = 0;
    bool pfc_map_support = false;
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;
    pfc_map_support = (en_dev->board_type == DH_STD_E318) && en_dev->ops->is_fw_feature_support(en_dev->parent, FW_FEATURE_NP_NPPU_TCAM_PFC_MAP);
    
    if (!(pfc->pfc_en == 0 || pfc->pfc_en == 0xff || (pfc_map_support && (pfc->pfc_en & 0x1) == 0)))
    {
        LOG_INFO_DEV(en_dev->parent, "pfc->pfc_en input invalid: %d", pfc->pfc_en);
        return EINVAL;
    }

    ret = zxdh_en_fc_mode_get(en_dev, &cur_port_mac_en);
    if (0 != ret)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_fc_mode_get failed, %d", ret);
        return ret;
    }

    if(pfc->pfc_en != 0)
    {
        port_mac_en = BIT(SPM_FC_PFC_FULL);
    }
    else if(cur_port_mac_en == BIT(SPM_FC_PFC_FULL))
    {
        port_mac_en = BIT(SPM_FC_NONE);
    }
    else
    {
        return 0;
    }

    if (port_mac_en != cur_port_mac_en)
    {
        /*mac部分端口pfc使能*/
        ret = zxdh_en_fc_mode_set(en_dev, port_mac_en);
        if (0 != ret)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_en_fc_mode_set failed, %d", ret);
            return ret;
        }
    }

    if (pfc_map_support && zxdh_en_is_panel_port(en_dev))
    {
        ret = (int32_t)dpp_pktrx_tcam_pfc_set(&pf_info, pfc->pfc_en);
        /*错误判断及打印*/
        if (0 != ret)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_pktrx_tcam_pfc_set pfc_en:%c failed, %d", pfc->pfc_en, ret);
        }
    }

    return ret;
}

static int zxdh_dcbnl_ieee_getmaxrate(struct net_device *netdev, struct ieee_maxrate *maxrate)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint32_t i = 0;
    uint32_t j = 0;

    if (!zxdh_en_is_panel_port(en_dev))
        return 0;

    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        LOG_ERR_DEV(en_dev->parent, "coredev type is not a PF");
        return -EOPNOTSUPP;
    }

    for (i = 0; i < ZXDH_DCBNL_MAX_TRAFFIC_CLASS; i++)
    {
        if (ZXDH_DCBNL_MAXRATE_KBITPS <= en_dev->dcb_para.tc_maxrate[i])
        {
            maxrate->tc_maxrate[i] = 0;  //0 indicates unlimited
        }
        else
        {
            maxrate->tc_maxrate[i] = en_dev->dcb_para.tc_maxrate[i];
        }
    }

    /* debug */
    for (j = 0; j < ZXDH_DCBNL_MAX_TRAFFIC_CLASS; j++)
    {
        LOG_DEBUG_DEV(en_dev->parent, " tc:%d,tc_maxrate:%lld \n", j, maxrate->tc_maxrate[j]);
    }

    return 0;
}

static int zxdh_dcbnl_ieee_rdma_setmaxrate(struct net_device *netdev, uint32_t *maxrate)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct zxdh_dcbnl_ieee_rdma_maxrate tc_maxrate = {0};
    uint32_t err = 0;
    uint32_t j = 0;

    ZXDH_DCBNL_CHECK_POINT_RET(maxrate,ZXDH_DCBNL_INVALID_PARA);

    tc_maxrate.port = en_dev->phy_port;
    for (j = 0; j < ZXDH_DCBNL_MAX_TRAFFIC_CLASS; ++j) 
    {
        tc_maxrate.tc_maxrate[j] = maxrate[j];
        LOG_DEBUG_DEV(en_dev->parent, " tc:%d,tc_maxrate:%d \n", j, maxrate[j]);
    }

    tc_maxrate.mode = en_dev->rdma_ets_flag;

    err = zxdh_rdma_events_call(en_dev->netdev, ZXDH_RDMA_TC_MAX_RATE_EVENT, &tc_maxrate);
    if (err) 
    {
        LOG_ERR_DEV(en_dev->parent, "Failed to set RDMA tc parameters: %d\n", err);
    }

    return err;
}

static int zxdh_dcbnl_ieee_setmaxrate(struct net_device *netdev, struct ieee_maxrate *maxrate)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev; 
    uint32_t maxrate_kbps[ZXDH_DCBNL_MAX_TRAFFIC_CLASS] = {0};
    uint32_t err,i;
    uint32_t j = 0;
    uint32_t tc_td_th[ZXDH_DCBNL_MAX_TRAFFIC_CLASS] = {ZXDH_DCBNL_FLOW_TDTH_DEFAULT};
    struct dh_core_dev    *dh_dev;
    struct zxdh_pf_device *pf_dev;
    uint64_t oldmaxrate = 0;

    ZXDH_DCBNL_CHECK_POINT_RET(netdev,ZXDH_DCBNL_INVALID_PARA);
    ZXDH_DCBNL_CHECK_POINT_RET(maxrate,ZXDH_DCBNL_INVALID_PARA);

    ZXDH_DCBNL_CHECK_POINT_RET(en_priv,ZXDH_DCBNL_INVALID_PARA);
    en_dev = &en_priv->edev;
    ZXDH_DCBNL_CHECK_POINT_RET(en_dev,ZXDH_DCBNL_INVALID_PARA);

    if (!zxdh_en_is_panel_port(en_dev))
        return 0;

    dh_dev = en_dev->parent;
    ZXDH_DCBNL_CHECK_POINT_RET(dh_dev,ZXDH_DCBNL_INVALID_PARA);
    
    pf_dev = dh_core_priv(dh_dev->parent);
    ZXDH_DCBNL_CHECK_POINT_RET(pf_dev,ZXDH_DCBNL_INVALID_PARA);

    ZXDH_DCBNL_CHECK_POINT_RET(en_dev->ops,ZXDH_DCBNL_INVALID_PARA);
    ZXDH_DCBNL_CHECK_POINT_RET(en_dev->ops->get_coredev_type,ZXDH_DCBNL_INVALID_PARA);

    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        LOG_ERR_DEV(en_dev->parent, "coredev type is not a PF");
        return -EOPNOTSUPP;
    }

    /* Values are 64 bits and specified in Kbps */
    for (i = 0; i < ZXDH_DCBNL_MAX_TRAFFIC_CLASS; i++)
    {
        oldmaxrate = en_dev->dcb_para.tc_maxrate[i];

        if ((maxrate->tc_maxrate[i] == 0) || (maxrate->tc_maxrate[i] >= ZXDH_DCBNL_MAXRATE_KBITPS))
        {
            if(pf_dev->board_type == DH_STDB || pf_dev->board_type == DH_STDA || pf_dev->board_type == DH_STDC)
            {
                tc_td_th[i] = ZXDH_DCBNL_FLOW_TDTH_DEFAULT;
                LOG_DEBUG_DEV(en_dev->parent, " old[%u]: maxrate %llu num %u\n",i, oldmaxrate, g_maxrate_num);
                if((g_maxrate_num > 0) && (oldmaxrate > 0) && (oldmaxrate < ZXDH_DCBNL_MAXRATE_KBITPS)) 
                {
                    g_maxrate_num--;
                    err = zxdh_dcbnl_set_single_td_th(en_priv, i, tc_td_th[i]);
                    ZXDH_DCBNL_CHECK_RET_RETURN(err);
                }
            }
            maxrate_kbps[i] = ZXDH_DCBNL_MAXRATE_KBITPS;

        }
        else if (maxrate->tc_maxrate[i] <= ZXDH_DCBNL_MINRATE_KBITPS)
        {
            maxrate_kbps[i] = ZXDH_DCBNL_MINRATE_KBITPS;
        }
        else
        {
            maxrate_kbps[i] = (uint32_t)maxrate->tc_maxrate[i];

            /* Started by AICoder, pid:810b4i0f5ee515a147a5098fc09d1b0ef8947567 */
            if(pf_dev->board_type == DH_STDB || pf_dev->board_type == DH_STDA || pf_dev->board_type == DH_STDC)
            {
                LOG_DEBUG_DEV(en_dev->parent, " old[%u]: maxrate %llu new %u num %u\n",i, oldmaxrate, maxrate_kbps[i],g_maxrate_num);
                tc_td_th[i] = ZXDH_DCBNL_FLOW_TDTH_DEFAULT;
                if((oldmaxrate == 0) || (oldmaxrate >= ZXDH_DCBNL_MAXRATE_KBITPS))
                {
                    g_maxrate_num++;
                }
    
                if(g_maxrate_num <= MAX_RATE_LIMITED_NUM)
                {
                    tc_td_th[i] = ZXDH_DCBNL_FLOW_TDTH_OPT;
                    err = zxdh_dcbnl_set_single_td_th(en_priv, i, tc_td_th[i]);
                    ZXDH_DCBNL_CHECK_RET_RETURN(err);
                }
            }
            /* Ended by AICoder, pid:810b4i0f5ee515a147a5098fc09d1b0ef8947567 */
        }
    }
    LOG_DEBUG_DEV(en_dev->parent, " g_maxrate_num %u\n",g_maxrate_num);

    tc_td_th[0] = ZXDH_DCBNL_FLOW_TDTH_BD;
    /* debug */
    for (j = 0; j < ZXDH_DCBNL_MAX_TRAFFIC_CLASS; j++)
    {
        LOG_DEBUG_DEV(en_dev->parent, " tc:%d,maxrate->tc_maxrate:%lld,maxrate_kbps:%d \n",
                 j, maxrate->tc_maxrate[j], maxrate_kbps[j]);
    }

    err = zxdh_dcbnl_set_tc_maxrate(en_priv, maxrate_kbps);
    if (err)
    {
        return err;
    }

    /*同步配置rdma tc maxrate参数*/
    err = zxdh_dcbnl_ieee_rdma_setmaxrate(netdev, maxrate_kbps);
    if (err) 
    {
        LOG_ERR_DEV(en_dev->parent, "Failed to set RDMA tc parameters: %d\n", err);
    }

    return err;
}

static int zxdh_dcbnl_rdma_trust_set(struct net_device *netdev, uint32_t trust)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct zxdh_dcbnl_rdma_trust rdma_trust_para = {0};
    uint32_t err = 0;

    rdma_trust_para.port = en_dev->phy_port;
    rdma_trust_para.trust_mode = trust;

    err = zxdh_rdma_events_call(en_dev->netdev, ZXDH_RDMA_TRUST_MODE_EVENT, &rdma_trust_para);
    if (err)
    {
        LOG_ERR("Failed to set RDMA trust parameters: %u\n", err);
    }

    en_dev->trust = trust;

    return err;
}

static int zxdh_dcbnl_ieee_setapp(struct net_device *netdev, struct dcb_app *app)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct dcb_app app_old;
    bool is_new = false;
    int err = 0;

    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        LOG_ERR_DEV(en_dev->parent, " coredev type is not a PF");
        return -EOPNOTSUPP;
    }

    if ((app->selector != IEEE_8021QAZ_APP_SEL_DSCP) ||
        (app->protocol >= ZXDH_DCBNL_MAX_DSCP) ||
        (app->priority >= ZXDH_DCBNL_MAX_PRIORITY))
    {
        return -EINVAL;
    }
    /* Save the old entry info */
    app_old.selector = IEEE_8021QAZ_APP_SEL_DSCP;
    app_old.protocol = app->protocol;
    app_old.priority = en_dev->dcb_para.dscp2prio[app->protocol];

    LOG_INFO_DEV(en_dev->parent, " protocol:%d, priority:%d \n", app->protocol, app->priority);

    if (!en_dev->dcb_para.dscp_app_num)
    {
        if (zxdh_en_is_panel_port(en_dev))
        {
            err =  zxdh_dcbnl_set_ets_trust(en_priv, ZXDH_DCBNL_ETS_TRUST_DSCP);
            if (err)
            {
                return err;
            }
        }
        zxdh_dcbnl_rdma_trust_set(netdev, ZXDH_DCBNL_ETS_TRUST_DSCP);
    }

    if (app->priority != en_dev->dcb_para.dscp2prio[app->protocol])
    {
        err = zxdh_dcbnl_set_dscp2prio(en_priv, app->protocol, app->priority);
        if (err)
        {   
            if (zxdh_en_is_panel_port(en_dev))
                zxdh_dcbnl_set_ets_trust(en_priv, ZXDH_DCBNL_ETS_TRUST_PCP);
            zxdh_dcbnl_rdma_trust_set(netdev, ZXDH_DCBNL_ETS_TRUST_PCP);
            return err;
        }
    }

    /* Delete the old entry if exists */
    err = dcb_ieee_delapp(netdev, &app_old);
    if (err)
    {
        is_new = true;
    }
    /* Add new entry and update counter */
    err = dcb_ieee_setapp(netdev, app);
    if (err)
    {
        return err;
    }
    if (is_new)
    {
        en_dev->dcb_para.dscp_app_num++;
    }
    LOG_INFO_DEV(en_dev->parent, " dscp_app_num:%d \n", en_dev->dcb_para.dscp_app_num);

    return err;
}

static int zxdh_dcbnl_ieee_delapp(struct net_device *netdev, struct dcb_app *app)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int err = 0;

    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_dcbnl_ieee_delapp coredev type is not a PF");
        return -EOPNOTSUPP;
    }

    if ((app->selector != IEEE_8021QAZ_APP_SEL_DSCP) ||
        (app->protocol >= ZXDH_DCBNL_MAX_DSCP))
    {
        return -EINVAL;
    }

    if (!en_dev->dcb_para.dscp_app_num)
    {
        return -ENOENT;
    }

    if (app->priority != en_dev->dcb_para.dscp2prio[app->protocol])
    {
        return -ENOENT;
    }

    /* Delete the app entry */
    err = dcb_ieee_delapp(netdev, app);
    if (err)
    {
        return err;
    }

    /* Restore to default */
    err = zxdh_dcbnl_set_dscp2prio(en_priv, app->protocol, app->protocol>>3);
    if (err)
    {
        if (zxdh_en_is_panel_port(en_dev))
            zxdh_dcbnl_set_ets_trust(en_priv, ZXDH_DCBNL_ETS_TRUST_PCP);
        zxdh_dcbnl_rdma_trust_set(netdev, ZXDH_DCBNL_ETS_TRUST_PCP);
        return err;
    }
    en_dev->dcb_para.dscp_app_num--;
    LOG_INFO_DEV(en_dev->parent, " protocol:%d, dscp_app_num:%d \n", app->protocol, en_dev->dcb_para.dscp_app_num);

    if (!en_dev->dcb_para.dscp_app_num)
    {
        if (zxdh_en_is_panel_port(en_dev))
        {
            err = zxdh_dcbnl_set_ets_trust(en_priv, ZXDH_DCBNL_ETS_TRUST_PCP);
        }
        zxdh_dcbnl_rdma_trust_set(netdev, ZXDH_DCBNL_ETS_TRUST_PCP);
    }

    return err;

}
#ifdef ZXDH_DCBNL_CEE_SUPPORT
static void zxdh_dcbnl_setpgtccfgtx(struct net_device *netdev, int tc,
                                   uint8_t prio_type, uint8_t pgid,
                                   uint8_t bw_pct, uint8_t up_map)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct zxdh_dcbnl_cee_ets *cee_ets_cfg;
    uint32_t i;

    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_dcbnl_setpgtccfgtx coredev type is not a PF");
        return;
    }

    if ((tc < 0) || (tc >= ZXDH_DCBNL_MAX_TRAFFIC_CLASS))
    {
        return;
    }

    cee_ets_cfg = &en_dev->dcb_para.cee_ets_cfg;
    for (i = 0; i < ZXDH_DCBNL_MAX_PRIORITY; i++)
    {
        if (up_map & BIT(i))
        {
            cee_ets_cfg->prio_tc[i] = tc;
        }
    }
    cee_ets_cfg->tc_tsa[tc] = IEEE_8021QAZ_TSA_ETS;

}
static void zxdh_dcbnl_setpgbwgcfgtx(struct net_device *netdev, int pgid, uint8_t bw_pct)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_dcbnl_setpgbwgcfgtx coredev type is not a PF");
        return;
    }

    if ((pgid >= 0) && (pgid < ZXDH_DCBNL_MAX_TRAFFIC_CLASS))
    {
        en_dev->dcb_para.cee_ets_cfg.tc_tx_bw[pgid] = bw_pct;
    }
    LOG_INFO_DEV(en_dev->parent, " tc_tx_bw[%d]:%d \n", pgid, bw_pct);

}

static void zxdh_dcbnl_getpgtccfgtx(struct net_device *netdev, int prio,
                                    uint8_t *prio_type, uint8_t *pgid,
                                    uint8_t *bw_pct, uint8_t *up_map)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    /* pf检查 */
    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_dcbnl_getpgtccfgtx coredev type is not a PF");
        return;
    }

    if ((prio >= 0) && (prio < ZXDH_DCBNL_MAX_PRIORITY))
    {
        *pgid = en_dev->dcb_para.ets_cfg.prio_tc[prio];
    }

}

static void zxdh_dcbnl_getpgbwgcfgtx(struct net_device *netdev, int pgid, uint8_t *bw_pct)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_dcbnl_getpgbwgcfgtx coredev type is not a PF");
        return;
    }

    if ((pgid >= 0) && (pgid < ZXDH_DCBNL_MAX_TRAFFIC_CLASS))
    {
        *bw_pct = en_dev->dcb_para.ets_cfg.tc_tx_bw[pgid];
    }

}


static void zxdh_dcbnl_setpgtccfgrx(struct net_device *netdev, int prio,
                                   uint8_t prio_type, uint8_t pgid,
                                   uint8_t bw_pct, uint8_t up_map)
{
    LOG_ERR("Rx PG TC Config Not Supported.\n");
}

static void zxdh_dcbnl_setpgbwgcfgrx(struct net_device *netdev, int pgid, uint8_t bw_pct)
{
    LOG_ERR("Rx PG BWG Config Not Supported.\n");
}


static void zxdh_dcbnl_getpgtccfgrx(struct net_device *netdev, int prio,
                                    uint8_t *prio_type, uint8_t *pgid,
                                    uint8_t *bw_pct, uint8_t *up_map)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_dcbnl_getpgtccfgrx coredev type is not a PF");
        return;
    }

    if ((prio >= 0) && (prio < ZXDH_DCBNL_MAX_PRIORITY))
    {
        *pgid = en_dev->dcb_para.ets_cfg.prio_tc[prio];
    }

}

static void zxdh_dcbnl_getpgbwgcfgrx(struct net_device *netdev, int pgid, uint8_t *bw_pct)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_dcbnl_getpgbwgcfgrx coredev type is not a PF");
        return;
    }

    if ((pgid >= 0) && (pgid < ZXDH_DCBNL_MAX_TRAFFIC_CLASS))
    {
        *bw_pct = 0;
    }

}

static uint8_t zxdh_dcbnl_setall(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct ieee_ets ets = {0};
    uint32_t i = 0;
    uint32_t err = 0;
    uint32_t j = 0;

    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_dcbnl_setall coredev type is not a PF");
        return 1;
    }

    ets.ets_cap = ZXDH_DCBNL_MAX_TRAFFIC_CLASS;
    for (i = 0; i < ZXDH_DCBNL_MAX_TRAFFIC_CLASS; i++)
    {
        ets.tc_tx_bw[i] = en_dev->dcb_para.cee_ets_cfg.tc_tx_bw[i];
        ets.tc_rx_bw[i] = en_dev->dcb_para.cee_ets_cfg.tc_tx_bw[i];
        ets.tc_tsa[i] = en_dev->dcb_para.cee_ets_cfg.tc_tsa[i];
    }

    for (i = 0; i < ZXDH_DCBNL_MAX_PRIORITY; i++)
    {
        ets.prio_tc[i] = en_dev->dcb_para.cee_ets_cfg.prio_tc[i];
    }
    /* debug */
    for (j = 0; j < ZXDH_DCBNL_MAX_TRAFFIC_CLASS; j++)
    {
        LOG_INFO_DEV(en_dev->parent, " idx:%d, tc_tsa:%d, tc_tx_bw:%d, prio_tc:%d \n",
                    j, ets.tc_tx_bw[j], ets.tc_tsa[j], ets.prio_tc[j]);
    }

    err = zxdh_dcbnl_check_ets_para(&ets);
    if (err)
    {
        return err;
    }

    err = zxdh_dcbnl_ieee_set_ets_para(en_priv, &ets);
    if (err)
    {
        return err;
    }

    return 0;
}

static uint8_t zxdh_dcbnl_getstate(struct net_device *netdev)
{
    return ZXDH_DCBNL_CEE_STATE_UP;
}

static uint8_t zxdh_dcbnl_setstate(struct net_device *netdev, u8 state)
{

    return 0;
}
#endif

static const struct dcbnl_rtnl_ops zxdh_dcbnl_ops ={
    .ieee_getets    = zxdh_dcbnl_ieee_getets,
    .ieee_setets    = zxdh_dcbnl_ieee_setets,
    .ieee_getpfc    = zxdh_dcbnl_ieee_getpfc,
    .ieee_setpfc    = zxdh_dcbnl_ieee_setpfc,

    .ieee_getmaxrate = zxdh_dcbnl_ieee_getmaxrate,
    .ieee_setmaxrate = zxdh_dcbnl_ieee_setmaxrate,

    .ieee_setapp    = zxdh_dcbnl_ieee_setapp,
    .ieee_delapp    = zxdh_dcbnl_ieee_delapp,

#ifdef ZXDH_DCBNL_CEE_SUPPORT
    /* CEE not support */
    .setall         = zxdh_dcbnl_setall,

    .getstate       = zxdh_dcbnl_getstate,
    .setstate       = zxdh_dcbnl_setstate,

    .setpgtccfgtx   = zxdh_dcbnl_setpgtccfgtx,
    .setpgbwgcfgtx  = zxdh_dcbnl_setpgbwgcfgtx,
    .getpgtccfgtx   = zxdh_dcbnl_getpgtccfgtx,
    .getpgbwgcfgtx  = zxdh_dcbnl_getpgbwgcfgtx,

    .setpgtccfgrx   = zxdh_dcbnl_setpgtccfgrx,
    .setpgbwgcfgrx  = zxdh_dcbnl_setpgbwgcfgrx,
    .getpgtccfgrx   = zxdh_dcbnl_getpgtccfgrx,
    .getpgbwgcfgrx  = zxdh_dcbnl_getpgbwgcfgrx,
#endif
};

uint32_t zxdh_dcbnl_set_tm_pport_mcode_gate_open(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    uint32_t err = 0;
    err = zxdh_dcbnl_set_tm_gate(en_priv, 1);
    if (err)
    {
        LOG_ERR_DEV(en_priv->edev.parent, " set_tm_gate close failed \n");
    }
    LOG_INFO_DEV(en_priv->edev.parent, " tm mcode gate open ");
    return err;
}
EXPORT_SYMBOL(zxdh_dcbnl_set_tm_pport_mcode_gate_open);

uint32_t zxdh_dcbnl_set_tm_pport_mcode_gate_close(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    uint32_t err = 0;
    err = zxdh_dcbnl_set_tm_gate(en_priv, 0);
    if (err)
    {
        LOG_ERR_DEV(en_priv->edev.parent, " set_tm_gate close failed \n");
    }
    LOG_INFO_DEV(en_priv->edev.parent, " tm mcode gate close ");
    return err;
}
EXPORT_SYMBOL(zxdh_dcbnl_set_tm_pport_mcode_gate_close);

uint32_t zxdh_dcbnl_set_np_flow_monitor_flag(struct net_device *netdev, uint32_t flag)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    uint32_t err = 0;
    err = zxdh_dcbnl_set_flow_monitor_gate(en_priv, flag);
    if (err)
    {
        LOG_ERR_DEV(en_priv->edev.parent, " zxdh_dcbnl_set_flow_monitor_gate failed \n");
    }
    
    return err;
}
EXPORT_SYMBOL(zxdh_dcbnl_set_np_flow_monitor_flag);

uint32_t zxdh_dcbnl_initialize(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint32_t err = 0;

    LOG_DEBUG_DEV(en_dev->parent, "%s dcbnl init begin\n", netdev->name);
    if (zxdh_en_is_panel_port(en_dev))
    {
        err = zxdh_dcbnl_init_port_speed(en_priv);
        if (err)
        {
            LOG_INFO_DEV(en_dev->parent, "dcbnl_init_ets: init_port_speed failed \n");
            //return err;
        }

        err = zxdh_dcbnl_init_ets_scheduling_tree(en_priv, true);
        if (err)
        {
            LOG_ERR_DEV(en_dev->parent, "dcbnl_init_ets: init_ets_scheduling_tree failed \n");
            return err;
        }

        zxdh_dcbnl_printk_ets_tree(en_priv);
    }

    en_dev->dcb_para.init_flag = ZXDH_DCBNL_INIT_FLAG;
    #if defined(CHECK_DCB_ENABLED)
    #ifdef CONFIG_DCB
    netdev->dcbnl_ops = &zxdh_dcbnl_ops;
    #endif
    #else
    netdev->dcbnl_ops = &zxdh_dcbnl_ops;
    #endif

    //zxdh_dcbnl_set_tm_pport_mcode_gate_open(netdev);
    LOG_DEBUG_DEV(en_dev->parent, "%s dcbnl init ok ", netdev->name);
    return 0;
}

uint32_t zxdh_dcbnl_ets_uninit(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    if ((en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF) ||
        (!zxdh_en_is_panel_port(en_dev)))
    {
        return 0;
    }
    LOG_DEBUG_DEV(en_dev->parent, "%s dcbnl uninit begin\n", netdev->name);

    en_dev->dcb_para.init_flag = 0;
    #if defined(CHECK_DCB_ENABLED)
    #ifdef CONFIG_DCB
    netdev->dcbnl_ops = NULL;
    #endif
    #else
    netdev->dcbnl_ops = NULL;
    #endif
    zxdh_dcbnl_set_tm_pport_mcode_gate_close(netdev);

    zxdh_dcbnl_free_flow_resources(en_priv);

    zxdh_dcbnl_free_se_resources(en_priv);

    LOG_DEBUG_DEV(en_dev->parent, "%s dcbnl uninit ok ", netdev->name);
    return 0;
}

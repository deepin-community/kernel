#include <linux/dinghai/zxdh_auxiliary_bus.h>
#include <linux/dinghai/driver.h>
#include <net/devlink.h>
#include <net/udp_tunnel.h>
#include <linux/dinghai/devlink.h>
#include <linux/dinghai/dh_cmd.h>
#include <linux/netdevice.h>
#ifdef CGS_V5_693
#include <linux/device.h>
#endif
#include "en_aux.h"
#include "en_ethtool/ethtool.h"
#include <linux/dinghai/en_sf.h>
#include <linux/etherdevice.h>
#include <linux/dinghai/helper.h>
#include "en_np/table/include/dpp_tbl_api.h"
#include "en_np/table/include/dpp_tbl_plcr.h"
#include "en_aux/en_aux_events.h"
#include "en_aux/en_aux_eq.h"
#include "en_aux/en_aux_cmd.h"
#include "msg_common.h"
#include "cmd/msg_chan_priv.h"
#include "en_pf.h"
#include <linux/dinghai/kcompat.h>
#include "en_aux/en_aux_ioctl.h"
#ifdef TIME_STAMP_1588
#include "en_aux/en_1588_pkt_proc.h"
#endif

#define VQM_BAR_MSG 36

#define OPCODE_GET  0
#define OPCODE_SET  1

#define CMD_MAC              1
#define CMD_ENABLED_QP       4
#define CMD_FEATURES         5
#define CMD_DRIVER_STATUS    6
#define CMD_VF_STATS         7
#define CMD_VF_FLAG          8
#define CMD_VF_QOS           9
#define CMD_VF_POLL          10
#define CMD_GLOBAL_FEATURES  11

const uint32_t gaudPlcrCarxProfileNum[E_PLCR_CAR_NUM]={
    PLCR_CAR_A_PROFILE_RES_NUM,   //一级CAR：512个限速模板
    PLCR_CAR_B_PROFILE_RES_NUM,   //二级CAR：128个限速模板
    PLCR_CAR_C_PROFILE_RES_NUM,   //三级CAR：32个限速模板
};

const uint32_t gaudPlcrCarxFlowIdNum[E_PLCR_CAR_NUM]={
    PLCR_CAR_A_FLOWID_RES_NUM,   //一级CAR：包含内核和dpdk的id
    PLCR_CAR_B_FLOWID_RES_NUM,   //二级CAR：前2304个分配给vf，后64个个分配给pf
    PLCR_CAR_C_FLOWID_RES_NUM,   //三级CAR
};

struct zxdh_plcr_cbs gat_carA_byte_rate_limit_cbs[] =
{
    {0,     500,    4*1024*1024},
    {500,   800,    10*1024*1024},
    {800,   1500,   12*1024*1024},
    {1500,  3000,   15*1024*1024},
    {3000,  12000,  20*1024*1024},
    {12000, 20000,  30*1024*1024},
    {20000, 500000, 50*1024*1024},
};

struct zxdh_plcr_cbs gat_carB_byte_rate_limit_cbs[] =
{
    {0,     4000,   8*1024*1024},
    {4000,  8000,   16*1024*1024},
    {8000,  16000,  64*1024*1024},
    {16000, 500000, 128*1024*1024 - 1},
};

inline struct zxdh_en_device *pf_dev_get_edev(struct zxdh_pf_device *pf_dev)
{
    struct zxdh_auxiliary_device *adev = NULL;
    struct zxdh_en_sf_container *sf_con = NULL;
    struct zxdh_en_sf_device *en_sf_dev = NULL;
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;

    adev = pf_dev->adevs_table[0].adev; //sf adev
    sf_con = container_of(adev, struct zxdh_en_sf_container, adev);
    en_sf_dev = dh_core_priv(sf_con->cdev); //sf cdev
#ifdef CGS_V5_693
    en_priv = dev_get_drvdata(&en_sf_dev->adev[0]->dev); //en adev
#else
    en_priv = en_sf_dev->adev[0]->dev.driver_data; //en adev
#endif
    if (en_priv == NULL)
        return ERR_PTR(-ENODEV);

    en_dev = &en_priv->edev;
    if (en_dev == NULL && !en_dev->init_comp_flag) {
        LOG_ERR("en_device not initialized!\n");
        return ERR_PTR(-ENODEV);
    }

    return en_dev;
}

/*
todo:
rsvd字段为1标识失败，为0标识成功
修改原有的限速值，只用修改限速模板，不用再调用关联函数；
还有代码中在配置限速模板的时候引用+1了，会影响现有的流程的（结果可能没问题，需要考虑是不是将+1和-1更换位置）；
如果找到共享模板，在配置队列失败的时候会减一，加一和减一的位置不对称，会导致计数值不正确。
*/

/*
函数功能：将用户输入的速率，转换成限速模板配置寄存器的格式
入参：
    ---max_rate   : 单位是Mbit/s

返回值：返回vqm中的发送队列号，发送队列号为奇数
*/
uint32_t zxdh_plcr_user_maxrate_2_reg(uint32_t user_max_rate)
{
    uint64_t reg_maxrate;

    // PLCR_FUNC_DBG_ENTER();
    reg_maxrate = ((uint64_t)user_max_rate << 10 / PLCR_STEP_SIZE);
    return (uint32_t)reg_maxrate;
}

/*
函数功能：将寄存器中的配置值，转换成用户的限速值（）
将用户输入的速率，转换成限速模板配置寄存器的格式
入参：
    ---maxrate_cfg   : 单位是61Kb/s
出参：
    ---user_max_rate : 单位是Mbit/s

返回值：返回vqm中的发送队列号，发送队列号为奇数
*/
uint32_t zxdh_plcr_reg_maxrate_user(uint32_t reg_maxrate)
{
    uint32_t user_max_rate;

    // PLCR_FUNC_DBG_ENTER();

    user_max_rate = reg_maxrate * PLCR_STEP_SIZE / 1024;

    return user_max_rate;
}

/*
函数功能：查找是否有共享的限速模板
入参：
    ---pf_dev     : pf设备结构体
    ---car_type   : plcr CAR层级，取值：CAR_A，CAR_B，CAR_C
    ---profile_cfg: 待查询的限速模板参数，可能是字节限速模板或者包限速模板
    ---profile_id : 保存查询到的限速模板的id

返回值：0表示查询成功，其它值表示查询失败
*/
static int32_t zxdh_plcr_match_profile(struct zxdh_pf_device *pf_dev,
                                              E_PLCR_CAR_TYPE car_type,
                                  DPP_STAT_CAR_PROFILE_CFG_T *profile_cfg,
                                                    uint16_t *profile_id)
{
    struct xarray            *xarray_profile = &(pf_dev->plcr_table.plcr_profiles[car_type]);
    struct zxdh_plcr_profile *profile;
    unsigned long             index;
    DPP_STAT_CAR_PKT_PROFILE_CFG_T  *pkt_profile_cfg = (DPP_STAT_CAR_PKT_PROFILE_CFG_T  *)(profile_cfg);
    uint32_t                  profile_max_num = gaudPlcrCarxProfileNum[car_type];
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    // PLCR_FUNC_DBG_ENTER();

    /*遍历vf申请的某一级CAR所有限速模板，是否有指定速率的限速模板存在*/
    xa_for_each_range(xarray_profile, index, profile, 0, profile_max_num)
    {
        if(0 == profile->ref_cnt)
        {
            continue;
        }

        /*包限速模板比较*/
        if (E_RATE_LIMIT_PACKET ==profile_cfg->pkt_sign)
        {
            if ((pkt_profile_cfg->pkt_sign == (((DPP_STAT_CAR_PKT_PROFILE_CFG_T  *)(&profile->profile_cfg))->pkt_sign)) &&
                (pkt_profile_cfg->cir      == (((DPP_STAT_CAR_PKT_PROFILE_CFG_T  *)(&profile->profile_cfg))->cir))      &&
                (pkt_profile_cfg->cbs      == (((DPP_STAT_CAR_PKT_PROFILE_CFG_T  *)(&profile->profile_cfg))->cbs)))
            {
                *profile_id = profile->profile_id;
                PLCR_LOG_DEBUG_DEV(dh_dev, "profile_id = %d\n", *profile_id);

                return 0;
            }
        }
        //字节限速模板比较
        else if (E_RATE_LIMIT_BYTE ==profile_cfg->pkt_sign)
        {
            if((profile->profile_cfg.pkt_sign == profile_cfg->pkt_sign) &&
               (profile->profile_cfg.cd       == profile_cfg->cd)  &&  /**<  @brief CD算法标志/令牌桶算法标志 0:srtcm 1:trtcm 2:MEF10.1*/
               (profile->profile_cfg.cf       == profile_cfg->cf)  &&  /**<  @brief CF溢出耦合标志，0:不溢出，1:溢出*/
               (profile->profile_cfg.cm       == profile_cfg->cm)  &&  /**<  @brief CM色盲/色敏标志，0:色盲模式，1:色敏模式 */
               (profile->profile_cfg.cir      == profile_cfg->cir) &&  /**<  @brief C令牌桶添加速率(0~X, X Gbps/64kbps),最小值为64Kbps，步长为64Kbps*/
               (profile->profile_cfg.cbs      == profile_cfg->cbs) &&  /**<  @brief C桶桶深(XM),配置范围为0~XMByte-1，步长为1Byte*/
               (profile->profile_cfg.eir      == profile_cfg->eir) &&  /**<  @brief E令牌桶添加速率(0~X, XGbps/64kbps),最小值为64Kbps，步长为64Kbps*/
               (profile->profile_cfg.ebs      == profile_cfg->ebs))    /**<  @brief E桶桶深(XM),配置范围为0~XMByte-1，步长为1Byte*/
            {
                *profile_id = profile->profile_id;
                PLCR_LOG_DEBUG_DEV(dh_dev, "profile_id = %d\n", *profile_id);

                return 0;
            }
        }
    }

    /* 未搜索到匹配项*/
    return -ERANGE;
}

/*
函数功能：为flowid申请一个xarray成员
入参：
    ---pf_dev     : pf设备结构体
    ---car_type   : plcr CAR层级，一级，二级或三级
    ---flow_id    : 作为xarray的索引
返回值：返回创建的zxdh_plcr_flow *指针
*/
int32_t zxdh_plcr_req_flow(struct zxdh_pf_device *pf_dev,
                                  E_PLCR_CAR_TYPE car_type,
                                         uint16_t flow_id,
                          struct zxdh_plcr_flow **flow)
{
    struct zxdh_plcr_flow *flow_old;
    struct xarray         *xarray_flow = &(pf_dev->plcr_table.plcr_flows[car_type]);
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    // PLCR_FUNC_DBG_ENTER();

    /*1. malloc一个flow结构体*/
    *flow = kzalloc(sizeof(struct zxdh_plcr_flow), GFP_KERNEL);
    if (unlikely(NULL == *flow))
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed to kzalloc\n");
        return -ENOMEM;
    }

    /*2. 存储到xarray*/
    flow_old = xa_store(xarray_flow, flow_id, *flow, GFP_KERNEL);
    if (flow_old)
    {
        /* 正常情况下，这里应该都是空的*/
        kfree(flow_old);
    }

    return 0;
}

/*
函数功能：释放一个xarray下的flowid成员
入参：
    ---pf_dev     : pf设备结构体
    ---car_type   : plcr CAR层级，一级，二级或三级
    ---flow_id    : 作为xarray的索引
返回值：返回创建的zxdh_plcr_flow *指针
*/
int32_t zxdh_plcr_release_flow(struct zxdh_pf_device *pf_dev,
                                      E_PLCR_CAR_TYPE car_type,
                                             uint16_t flow_id)
{
    struct zxdh_plcr_flow *flow;
    struct xarray         *xarray_flow = &(pf_dev->plcr_table.plcr_flows[car_type]);
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    // PLCR_FUNC_DBG_ENTER();

    /*1. 检查xarray里是否有该flow*/
    flow = xa_load(xarray_flow, flow_id);
    if (NULL == flow)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed to release an invalid flow_id=%d\n", flow_id);
        return EINVAL;
    }

    /*2. 从xarray删除该成员*/
    xa_erase(xarray_flow, flow_id);

    /*3. 释放flow*/
    kfree(flow);

    return 0;
}

/*
函数功能：更新flow的成员信息
入参：
    ---flow       : xarray的成员
    ---vport      :
    ---max_rate   :
    ---min_rate   :
返回值：无
*/
void zxdh_plcr_update_flow(struct zxdh_plcr_flow *flow,
                                         uint16_t vport,
                                         uint32_t max_rate,
                                         uint32_t min_rate)
{
    flow->vport    = vport;
    flow->max_rate = max_rate;
    flow->min_rate = min_rate;
}

/*
函数功能：申请一个指定CAR的限速模板
入参：
    ---pf_dev     : pf设备结构体
    ---car_type   : plcr CAR层级，一级，二级或三级
    ---vport      : vf端口号
    ---profile_id_out : 返回申请到的限速模板的profile_id
返回值：成功返回0，失败返回其它值
*/
int zxdh_plcr_req_profile(struct zxdh_pf_device *pf_dev,
                                 E_PLCR_CAR_TYPE car_type,
                                       uint16_t *profile_id_out)
{
    int  rtn = 0;
    struct zxdh_plcr_profile *profile;
    struct zxdh_plcr_profile *profile_old;
    struct xarray *xarray_profile = &(pf_dev->plcr_table.plcr_profiles[car_type]);
    uint16_t profile_id = 0;
    uint64_t cred_id = 0;
    DPP_PF_INFO_T pf_info = {0};
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    pf_info.slot = pf_dev->slot_id;
    pf_info.vport = pf_dev->vport;

    // PLCR_FUNC_DBG_ENTER();

    /*在指定的CAR申请一个新的限速模板*/
    rtn = dpp_car_profile_id_add(&pf_info, (uint32_t)car_type, &cred_id);
    if (rtn)
    {
        /*判断消息交互是否正常*/
        PLCR_LOG_ERR_DEV(dh_dev, "failed to request a new profile\n");
        return -EINVAL;
    }

    /*判断riscv是否成功返回了有效的profile：bit[56 - 63]为0标识成功，为1标识失败*/
    if (0 != ((cred_id >> 56) & 0xFF))
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed to request a new profile\n");
        return -EINVAL;
    }

    /*提取profile id*/
    profile_id = PROFILE_ID(cred_id);
    *profile_id_out = profile_id;
    PLCR_LOG_DEBUG_DEV(dh_dev, "dpp_car_profile_id_add: pf_info.vport = 0x%x, car_type = %d, profile_id = %d, cred_id = 0x%llx\n", pf_info.vport, car_type, profile_id, cred_id);

    /*申请一个限速模板结构体，保存限速模板信息*/
    profile = kzalloc(sizeof(struct zxdh_plcr_profile), GFP_KERNEL);
    if (unlikely(NULL == profile))
    {
        dpp_car_profile_id_delete(&pf_info, (uint32_t)car_type, cred_id);
        PLCR_LOG_ERR_DEV(dh_dev, "failed to kzalloc profile\n");

        return -ENOMEM;
    }
    profile->ref_cnt    = 0;
    profile->max_rate   = 0;
    profile->min_rate   = 0;
    profile->cred_id    = cred_id;
    profile->profile_id = profile_id;
    profile->vport      = pf_dev->vport;

    /*将申请到的限速模板资源存储起来*/
    profile_old = xa_store(xarray_profile, profile_id, profile, GFP_KERNEL);
    if (profile_old)          //正常情况下，这里应该都是空的
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed to unreachable branch\n");
        kfree(profile_old);
    }


    return rtn;
}

/*
函数功能：释放一个指定CAR的限速模板
入参：
    ---pf_dev     : pf设备结构体
    ---car_type   : plcr CAR层级，一级，二级或三级
    ---Profile_id : 限速模板的profile_id
    ---flag       : 热插拔标记
返回值：成功返回0，失败返回其它值
*/
int zxdh_plcr_release_profile(struct zxdh_pf_device *pf_dev, E_PLCR_CAR_TYPE car_type,
                              uint16_t profile_id, uint32_t flag)
{
    int    rtn = 0;
    struct xarray *xarray_profile = &(pf_dev->plcr_table.plcr_profiles[car_type]);
    struct zxdh_plcr_profile *profile;
    DPP_PF_INFO_T pf_info = {0};
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    // PLCR_FUNC_DBG_ENTER();

    /*判断是有是有效成员*/
    profile = xa_load(xarray_profile, profile_id);
    if (NULL == profile)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed to release an invalid profile=%d\n", profile_id);
        return EINVAL;
    }

    /*如果引用计数为0，则可以释放所有资源*/
    if (0 == profile->ref_cnt)
    {
        pf_info.slot  = pf_dev->slot_id;
        pf_info.vport = profile->vport;
        PLCR_LOG_DEBUG_DEV(dh_dev, "dpp_car_profile_id_delete: pf_info.vport = 0x%x, car_type = %d, profile_id = %d, cred_id = 0x%llx\n", pf_info.vport, car_type, profile_id, profile->cred_id);

        /*归还限速模板资源（注意，引用计数为0表示没有关联的flow了）*/
        if(!flag)
        {
            rtn = dpp_car_profile_id_delete(&pf_info, car_type, profile->cred_id);
            if (rtn)
            {
                PLCR_LOG_ERR_DEV(dh_dev, "failed to call dpp_car_profile_id_delete, car_type=%d,profile_id=%d)\n", car_type, profile_id);
                rtn = EINVAL;
            }
        }

        /*删除xarray中的元素*/
        xa_erase(xarray_profile, profile_id);

        /*释放profile指针*/
        kfree(profile);
    }

    /*如果引用计数不为0，就不释放任何资源*/
    return rtn;
}

/*
函数功能：内核态使用的接口，根据car_type & is_byte_rate_limit & max_rate & min_rate 这4个参数生成限速模板配置参数
        内核态下各限速场景使用的参数应该是固定的，我们自己根据这4个参数生成完整的结构体参数：DPP_STAT_CAR_PROFILE_CFG_T
        用户态的场景下，会通过消息直接传递过来，不需要组装
入参：
    ---is_pkt_mode: 包限速还是字节限速
    ---car_type   : plcr CAR层级，一级，二级或三级
    ---is_byte_rate_limit   : 是否是字节限速，为后续包限速预留参数，这个值传参的时候暂时固定为1，
                              后续可能会有一个全局变量进行指示，会提供对应的接口来获取这个入参值进行传递
    ---max_rate             : 用户指定的最大限速值
    ---min_rate             : 用户指定的最小承诺速率
    ---profile_cfg          : 返回值，填充好的限速模板参数
返回值：成功返回0，失败返回其它值
*/
static int zxdh_plcr_gen_profile(struct zxdh_pf_device *pf_dev,
                                  E_RATE_LIMIT_PKT_BYTE is_pkt_mode,
                                        E_PLCR_CAR_TYPE car_type,
                                               uint32_t max_rate,
                                               uint32_t min_rate,
                            DPP_STAT_CAR_PROFILE_CFG_T *profile_cfg)
{
    int      rtn = 0;
    int      pri = 0;
    uint32_t cbs = 0;
    uint32_t ebs = 0;
    DPP_STAT_CAR_PKT_PROFILE_CFG_T  *pkt_profile_cfg = (DPP_STAT_CAR_PKT_PROFILE_CFG_T  *)(profile_cfg);
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    // PLCR_FUNC_DBG_ENTER();

    //入参检测
    if ((E_RATE_LIMIT_PACKET == is_pkt_mode) && (E_PLCR_CAR_A != car_type))
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed and only CAR A supports packet rate limit\n");
        rtn = EINVAL;
    }

    /*2. 重新生成参数*/

    /*填充限速模板的参数，准备将这些参数配置到寄存器*/
    memset(profile_cfg, 0, sizeof(*profile_cfg));
    if (E_RATE_LIMIT_BYTE == is_pkt_mode)
    {
        if (USER_MAX_BYTE_RATE < max_rate)
            PLCR_COMM_ASSERT(-EINVAL);

        profile_cfg->pkt_sign       = E_RATE_LIMIT_BYTE;
        profile_cfg->cf             = 1; //溢出标志，默认使能

        if (pf_dev->plcr_table.burst_size)
        {
            cbs = pf_dev->plcr_table.burst_size;
            ebs = pf_dev->plcr_table.burst_size;
        }
        else
        {
            cbs = DPP_CAR_MAX_CBS_VALUE;
            ebs = DPP_CAR_MAX_EBS_VALUE;
        }

        profile_cfg->cbs            = cbs;
        profile_cfg->ebs            = ebs;
        profile_cfg->random_disc_c  = 0;
        profile_cfg->random_disc_e  = 0;

        if (E_PLCR_CAR_A == car_type)
        {
            profile_cfg->cm         = 0; //色盲模式
            profile_cfg->cd         = 0; //0: srTCM，单速率；1：双速率
            profile_cfg->cir        = zxdh_plcr_user_maxrate_2_reg(max_rate);
            profile_cfg->eir        = 0;
        }
        else if (E_PLCR_CAR_B == car_type)
        {
            profile_cfg->cm         = 1; //色敏模式
            profile_cfg->cd         = 1; //0: srTCM，单速率；1：双速率
            profile_cfg->cir        = zxdh_plcr_user_maxrate_2_reg(min_rate);
            profile_cfg->eir        = zxdh_plcr_user_maxrate_2_reg(max_rate);
        }
        else if (E_PLCR_CAR_C == car_type)
        {
            //todo：端口组限速，待调试确认
            profile_cfg->cm         = 1; //色敏模式
            profile_cfg->cd         = 0; //0: srTCM，单速率；1：双速率
            profile_cfg->cir        = zxdh_plcr_user_maxrate_2_reg(max_rate);
            profile_cfg->eir        = 0;
        }

        for (pri = 0; pri < DPP_CAR_PRI_MAX; pri ++)
        {
            profile_cfg->c_pri[pri] = 0;
            profile_cfg->e_green_pri[pri] = 0;
            profile_cfg->e_yellow_pri[pri] = 0;
        }

        PLCR_LOG_DEBUG_DEV(dh_dev, "cir = 0x%x, eir = 0x%x, cbs = 0x%x, ebs = 0x%x\n", profile_cfg->cir, profile_cfg->eir, profile_cfg->cbs, profile_cfg->ebs);
    }
    else
    {
        //超过最大值就报错
        if (USER_MAX_PKT_RATE < max_rate)
            PLCR_COMM_ASSERT(-EINVAL);

        if (pf_dev->plcr_table.burst_size)
        {
            cbs = pf_dev->plcr_table.burst_size;
        }
        else
        {
            cbs = DPP_CAR_MAX_PKT_CBS_VALUE;
        }

        pkt_profile_cfg->pkt_sign = E_RATE_LIMIT_PACKET;
        pkt_profile_cfg->cbs      = cbs;
        pkt_profile_cfg->cir      = max_rate;

        PLCR_LOG_DEBUG_DEV(dh_dev, "pkt_type = 0x%x, cir = 0x%x, cbs = 0x%x\n", pkt_profile_cfg->pkt_sign, pkt_profile_cfg->cir, pkt_profile_cfg->cbs);
    }

    return rtn;
}

/*
函数功能：更新限速模板参数
入参：
    ---profile_cfg: 限速模板参数结构体
    ---Profile_id : 限速模板的profile_id
返回值：无
*/
static void zxdh_plcr_update_profile(DPP_STAT_CAR_PROFILE_CFG_T *profile_cfg, u_int16_t profile_id)
{
    // PLCR_FUNC_DBG_ENTER();

    /*1. 重新生成参数*/
    profile_cfg->profile_id = profile_id;
}

/*
函数功能：将限速模板参数，存储到profile下
入参：
    ---pf_dev     : pf设备结构体
    ---car_type   : plcr CAR层级，一级，二级或三级
    ---max_rate   : 用户原始的限速速率，单位是Mbit/s
    ---min_rate   : 用户原始的限速速率，单位是Mbit/s
    ---profile_cfg: 要存储到profile中的，限速模板参数
返回值：成功返回0，失败返回其它值
*/
int zxdh_plcr_store_profile(struct zxdh_pf_device *pf_dev,
                                   E_PLCR_CAR_TYPE car_type,
                                          uint32_t user_max_rate,
                                          uint32_t user_min_rate,
                       DPP_STAT_CAR_PROFILE_CFG_T *profile_cfg)
{
    int    rtn = 0;
    uint16_t profile_id;
    struct zxdh_plcr_profile *profile;
    struct xarray *xarray_profile = &(pf_dev->plcr_table.plcr_profiles[car_type]);
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    // PLCR_FUNC_DBG_ENTER();

    /*1. 从限速模板结构体，获取profile_id*/
    profile_id = profile_cfg->profile_id;

    /*2. 从xarray获取profile*/
    profile = xa_load(xarray_profile, profile_id);
    if (NULL == profile)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed to specify an invalid profile, profile_id=%d\n", profile_id);
        return -EINVAL;
    }

    /*3. 更新profile的参数*/
    profile->max_rate = user_max_rate;
    profile->min_rate = user_min_rate;

    /*4. 将完整的限速模板参数，存储到profile结构体下*/
    memcpy(&profile->profile_cfg, profile_cfg, sizeof(DPP_STAT_CAR_PROFILE_CFG_T));

    return rtn;
}

/*
函数功能：将限速模板参数，配置到plcr寄存器中去
入参：
    ---pf_dev     : pf设备结构体
    ---car_type   : plcr CAR层级，一级，二级或三级
    ---profile_cfg: 要配置个plcr寄存器的限速模板参数
返回值：成功返回0，失败返回其它值
*/
int zxdh_plcr_cfg_profile(struct zxdh_pf_device *pf_dev,
                                 E_PLCR_CAR_TYPE car_type,
                     DPP_STAT_CAR_PROFILE_CFG_T *profile_cfg)
{
    int      rtn = 0;
    uint16_t profile_id = 0;
    uint32_t pkt_sign   = 0;
    DPP_PF_INFO_T pf_info = {0};
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    pf_info.slot = pf_dev->slot_id;
    pf_info.vport = pf_dev->vport;

    // PLCR_FUNC_DBG_ENTER();

    /*1. 根据限速模板参数，获取profile_id*/
    profile_id = profile_cfg->profile_id;

    /*2. 根据限速模板参数，获取包/字节模式*/
    pkt_sign = profile_cfg->pkt_sign;

    /*3. 将限速模板参数，配置到寄存器中去*/
    rtn = dpp_car_profile_cfg_set(&pf_info, (uint32_t)car_type, pkt_sign, profile_id, profile_cfg);
    if (rtn)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed to configure the profile registers, car_type=%d,profile_id=%d\n", car_type, profile_id);
        return -EINVAL;
    }
    PLCR_LOG_DEBUG_DEV(dh_dev, "dpp_car_profile_cfg_set: pf_info.vport = 0x%x, car_type = %d, profile_id = %d, pkt_sign = %d\n", pf_info.vport, car_type, profile_id, pkt_sign);

    return rtn;
}

/*
函数功能：获取寄存器中限速模板的参数
入参：
    ---pf_dev     : pf设备结构体
    ---car_type   : plcr CAR层级，一级，二级或三级
    ---profile_cfg: 要配置个plcr寄存器的限速模板参数
返回值：成功返回0，失败返回其它值
*/
int zxdh_plcr_get_profile(struct zxdh_pf_device *pf_dev,
                                 E_PLCR_CAR_TYPE car_type,
                                        uint32_t pkt_sign,
                                        uint16_t profile_id,
                     DPP_STAT_CAR_PROFILE_CFG_T *profile_cfg)
{
    int    rtn = 0;
    DPP_PF_INFO_T pf_info = {0};
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    pf_info.slot = pf_dev->slot_id;
    pf_info.vport = pf_dev->vport;

    // PLCR_FUNC_DBG_ENTER();

    /*3. 将限速模板参数，配置到寄存器中去*/
    PLCR_LOG_DEBUG_DEV(dh_dev, "dpp_car_profile_cfg_get: pf_info.vport = 0x%x, car_type = %d, profile_id = %d, pkt_sign = %d\n", pf_info.vport, car_type, profile_id, pkt_sign);
    rtn = dpp_car_profile_cfg_get(&pf_info, car_type, pkt_sign, profile_id, profile_cfg);
    if (rtn)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed to call dpp_car_profile_cfg_get(), car_type=%d,profile_id=%d\n", car_type, profile_id);
        return -EINVAL;
    }

    return rtn;
}

/*
函数功能：配置指定CAR的限速模板：考虑plcr三级CAR能共享接口
入参：
    ---pf_dev     : pf设备结构体
    ---car_type   : plcr CAR层级，一级，二级或三级
    ---flowid     : 指定CAR的flow编号
    ---profile_id : 申请到的指定CAR层级的profile资源的id
返回值：成功返回0，失败返回其它值
*/
static int zxdh_plcr_bind_flow_profile(struct zxdh_pf_device *pf_dev,
                                              E_PLCR_CAR_TYPE car_type,
                                                     uint32_t flowid,
                                                     uint16_t profile_id)
{
    int                       rtn = 0;
    struct zxdh_plcr_flow    *plcr_flow;
    struct xarray            *xarray_flow = &(pf_dev->plcr_table.plcr_flows[car_type]);
    DPP_PF_INFO_T pf_info = {0};
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    pf_info.slot = pf_dev->slot_id;
    pf_info.vport = pf_dev->vport;

    // PLCR_FUNC_DBG_ENTER();

    /*1. 获取plcr*/
    plcr_flow = xa_load(xarray_flow, flowid);
    if (NULL == plcr_flow)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed to xa_load an invalid element,car_type=%d,flowid=%d,profile_id=%d\n", car_type, flowid, profile_id);
        return -EINVAL;
    }

    /*2. 调用接口，将flow与profile进行绑定*/
    PLCR_LOG_DEBUG_DEV(dh_dev, "dpp_car_queue_cfg_set: pf_info.vport = 0x%x, car_type = %d, flowid = %d, profile_id = %d\n", pf_info.vport, car_type, flowid, profile_id);
    rtn = dpp_car_queue_cfg_set(&pf_info, (uint32_t)car_type, flowid, DROP_DISABLE, PLCR_ENABLE, profile_id);
    if (rtn)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed to call dpp_car_queue_cfg_set(),car_type=%d,flowid=%d,profile_id=%d\n", car_type, flowid, profile_id);
        return -EINVAL;
    }
    PLCR_LOG_DEBUG_DEV(dh_dev, "Bind profile_%d to flow_%d complete\n", profile_id, flowid);

    /*3. 将profile_id更新到plcr中*/
    plcr_flow->profile_id = profile_id;

    return rtn;
}

/*
函数功能：解除flow和profile之间的绑定 & 删除xarray中的元素 & 释放flow指针
入参：
    ---pf_dev     : pf设备结构体
    ---car_type   : plcr CAR层级，一级，二级或三级
    ---flowid     : 指定CAR的flow编号
    ---profile_id : 申请到的指定CAR层级的profile资源的id
    ---flag       : 热插拔标记
返回值：成功返回0，失败返回其它值
*/
int zxdh_plcr_unbind_flow_profile(struct zxdh_pf_device *pf_dev,
                                         E_PLCR_CAR_TYPE car_type,
                                                uint32_t flowid,
                                                uint16_t profile_id,
                                                uint32_t flag)
{
    int                    rtn = 0;
    struct zxdh_plcr_flow *plcr_flow;
    struct xarray         *xarray_flow = &(pf_dev->plcr_table.plcr_flows[car_type]);
    DPP_PF_INFO_T pf_info = {0};
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    pf_info.slot = pf_dev->slot_id;
    pf_info.vport = pf_dev->vport;

    // PLCR_FUNC_DBG_ENTER();

    /*1. 检查flow是否与profile是否已绑定*/
    plcr_flow = xa_load(xarray_flow, flowid);
    if (NULL == plcr_flow)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "xa_load an invalid element, flowid=%d,profile_id=%d\n", flowid, profile_id);
        return -EINVAL;
    }
    if (profile_id != plcr_flow->profile_id)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "xa_load an invalid element, profile_id=%d,plcr_flow->profile_id=%d\n", profile_id, plcr_flow->profile_id);
        return -EINVAL;
    }

    /*调用接口，将flow与profile进行解除绑定：配置flow，将其不要指向profile*/
    PLCR_LOG_DEBUG_DEV(dh_dev, "dpp_car_queue_cfg_set: pf_info.vport = 0x%x, car_type = %d, flowid = %d, profile_id = %d\n", pf_info.vport, car_type, flowid, profile_id);
    if(!flag)
    {
        rtn = dpp_car_queue_cfg_set(&pf_info, (uint32_t)car_type, flowid, DROP_DISABLE, PLCR_DISABLE, profile_id);
        if (rtn)
        {
            PLCR_LOG_ERR_DEV(dh_dev, "failed to call dpp_car_queue_cfg_set(),car_type=%d,flowid=%d,profile_id=%d\n", car_type, flowid,profile_id);
            return rtn;
        }
    }

    return rtn;
}

int zxdh_plcr_count_up_profile(struct zxdh_pf_device *pf_dev,
                                      E_PLCR_CAR_TYPE car_type,
                                             uint16_t profile_id)
{
    int    rtn = 0;
    struct zxdh_plcr_profile *plcr_profile;
    struct xarray *xarray_profile = &(pf_dev->plcr_table.plcr_profiles[car_type]);
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    // PLCR_FUNC_DBG_ENTER();

    plcr_profile = xa_load(xarray_profile, profile_id);
    if (NULL == plcr_profile)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed to load element form xarray_profile, car_type=%d,profile_id=%d\n", car_type, profile_id);
        return -EINVAL;
    }

    plcr_profile->ref_cnt++;

    return rtn;
}

int zxdh_plcr_count_down_profile(struct zxdh_pf_device *pf_dev,
                                        E_PLCR_CAR_TYPE car_type,
                                               uint16_t profile_id)
{
    int    rtn = 0;
    struct zxdh_plcr_profile *plcr_profile;
    struct xarray *xarray_profile = &(pf_dev->plcr_table.plcr_profiles[car_type]);
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    // PLCR_FUNC_DBG_ENTER();

    plcr_profile = xa_load(xarray_profile, profile_id);
    if (NULL == plcr_profile)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed to load element form xarray_profile, car_type=%d,profile_id=%d\n", car_type, profile_id);
        return -EINVAL;
    }

    //不能对计数为0的profile进行减操作
    if (0 == plcr_profile->ref_cnt)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed and plcr_profile->ref_cnt=0\n");
        return -EINVAL;
    }

    plcr_profile->ref_cnt--;

    return rtn;
}

static int zxdh_plcr_get_profile_by_flowid(struct zxdh_pf_device *pf_dev,
                                                  E_PLCR_CAR_TYPE car_type,
                                                         uint32_t flowid,
                                       struct zxdh_plcr_profile **pplcr_profile)
{
    int rtn = 0;
    uint16_t profile_id = 0;
    struct zxdh_plcr_profile *plcr_profile;
    struct zxdh_plcr_flow    *plcr_flow;
    struct xarray *xarray_profile = &(pf_dev->plcr_table.plcr_profiles[car_type]);
    struct xarray *xarray_flow    = &(pf_dev->plcr_table.plcr_flows[car_type]);
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    // PLCR_FUNC_DBG_ENTER();

    plcr_flow = xa_load(xarray_flow, flowid);
    if (NULL == plcr_flow)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed to load element form xarray_flow, car_type=%d,flowid=%d\n", car_type, flowid);
        return -EINVAL;
    }
    profile_id = plcr_flow->profile_id;

    plcr_profile = xa_load(xarray_profile, profile_id);
    if (NULL == plcr_profile)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed to load element form xarray_profile,car_type=%d,profile_id=%d\n", car_type, profile_id);
        return -EINVAL;
    }

    *pplcr_profile = plcr_profile;

    return rtn;
}

/*******************************下面是新实现的代码*******************************/
/*
函数功能：将car之间flowid的映射关系存储起来

入参：
    ---
出参：
    ---

返回值：成功返回0，失败返回其它值
*/
int32_t zxdh_plcr_stroe_map(struct zxdh_pf_device *pf_dev,
                                   E_PLCR_CAR_TYPE car_type,
                                          uint32_t flowid,
                                          uint32_t map_flowid)
{   int32_t rtn = 0;

    struct xarray *xarray_map = &(pf_dev->plcr_table.plcr_maps[car_type]);
    if((E_PLCR_CAR_A == car_type) || (E_PLCR_CAR_B == car_type))
    {
        xa_store(xarray_map, flowid, (void *)(uintptr_t)(FLOWID_2_XARRAY(map_flowid)), GFP_KERNEL);
    }

    return rtn;
}

int32_t zxdh_plcr_clear_map(struct zxdh_pf_device *pf_dev,
                                   E_PLCR_CAR_TYPE car_type,
                                          uint32_t flowid)
{   int32_t rtn = 0;
    void *  xarray_element;

    struct xarray *xarray_map = &(pf_dev->plcr_table.plcr_maps[car_type]);
    if((E_PLCR_CAR_A == car_type) || (E_PLCR_CAR_B == car_type))
    {
        xarray_element = xa_load(xarray_map, flowid);
        if(NULL != xarray_element)
        {
            xa_erase(xarray_map, flowid);
        }
    }

    return rtn;
}

/*
函数功能：指定前一级的flowid，查询下一级映射的flowid

入参：
    ---
出参：
    ---

返回值：成功返回0，失败返回其它值
*/
int32_t zxdh_plcr_get_next_map(struct zxdh_pf_device *pf_dev,
                                      E_PLCR_CAR_TYPE car_type,
                                            uint32_t  flowid,
                                            uint32_t *map_flowid)
{
    int32_t rtn = 0;
    void *  xarray_element;

    struct xarray *xarray_map = &(pf_dev->plcr_table.plcr_maps[car_type]);
    if((E_PLCR_CAR_A == car_type) || (E_PLCR_CAR_B == car_type))
    {
        xarray_element = xa_load(xarray_map, flowid);
        if(NULL == xarray_element)
        {
            rtn = -EINVAL;
        }
        else
        {
            *map_flowid = XARRAY_2_FLOWID((uint32_t)(uintptr_t)xarray_element);
        }
    }
    else
    {
        rtn = -ERANGE;
    }

    return rtn;
}

/*
函数功能：检查指定的car_type所在的三级car flowid链是否全都没有限速，如果是就进行资源清理
入参：
    ---pf_dev : 设备结构体
    ---vport  : 标识vf端口
场景说明：
    1. 为什么要引入这个接口？
       新的方案引入了mode 0，mode 1，mode 2三种模式；
       vport在三级car上有一条完整的flowid映射链；
       用户在解除某一级car指定flowid的限速之后，驱动程序需要检查这个链上是不是没有限速了，且car C是不是处于group 0，如果是这样的话就要切换到模式0；
    2. 切换到模式0的必要性
       驱动程序中很多限速要需要先判断当前的限速模式；
       如果当前链上已经没有限速，且car C还是处于group 0，就必须切换回模式0，这也是从模式1和模式2切换回模式0的唯一途径
    3. 在什么时候需要调用这个接口？
       用户在解除某一级car指定flowid的限速之后，需要调用这个接口。
    4.流程：
      3.1 如果是对car C的group解除限速，说明group是非0的，函数直接返回(废弃，因为有可能是将vf端口从非0group移动到group0)
      3.2 根据vport得到car B的flowid
      3.3 根据car B的flowid，查询得到car c的flowid
      3.4.1 如果car C的flowid处于group 0，就进行模式切换
      3.4.2 如果car C的flowid不处于group 0，就不进行模式切换，函数返回
    5.plcr_maps是否需要清除？
      这个资源不需要清除：只有本函数会用到这个记录，且进行限速配置的时候会重新进行映射，且存储的成员不是动态分配的内存，没有必要清理资源；

返回值：成功返回0，失败返回其它值
*/
int zxdh_plcr_check_release_flow_chain(struct zxdh_pf_device *pf_dev, E_PLCR_CAR_TYPE e_car_type, uint16_t vport)
{
    int rtn = 0;
    E_PLCR_CAR_TYPE car_type;
    uint32_t flag1 = 0;
    uint32_t flag2 = 0;
    unsigned long flow_index;
    uint32_t flowid_car_B;
    uint32_t flowid_car_C;
    struct zxdh_plcr_flow *flow;
    struct xarray *xarray_flow;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    // PLCR_FUNC_DBG_ENTER();

    //如果是解除端口组的字节限速：不需要模式切换，因为操作端口组使用的是pf的vport
    if(E_PLCR_CAR_C == e_car_type)
    {
        PLCR_LOG_DEBUG_DEV(dh_dev, "It is not necessary to change mode for vf group limit!\n");
        return rtn;
    }

    //统计car A和car B上是否有限速
    for(car_type = E_PLCR_CAR_A, flag1 = 0; car_type < E_PLCR_CAR_C; car_type++)
    {
        xarray_flow = &(pf_dev->plcr_table.plcr_flows[car_type]);

        xa_for_each_range(xarray_flow, flow_index, flow, 0, gaudPlcrCarxFlowIdNum[car_type])
        {
            if(vport == flow->vport)
            {
                flag1 = 1;
                break;
            }
        }
        if(1 == flag1)
        {
            PLCR_LOG_DEBUG_DEV(dh_dev, "flow->flowid = 0x%x, vport = 0x%x\n", flow->flowid, vport);
            break;
        }
    }

    //检查是否处于group 0
    if(0 == flag1)
    {
        //如果是vf，就要检查group是否为0
        if (VF_ACTIVE(vport))
        {
            flowid_car_B = VQM_VFID(vport) * 2;
            rtn = zxdh_plcr_get_next_map(pf_dev, E_PLCR_CAR_B, flowid_car_B, &flowid_car_C);
            PLCR_COMM_ASSERT(rtn);
            PLCR_LOG_DEBUG_DEV(dh_dev, "flowid_car_B = 0x%x\n", flowid_car_B);
            PLCR_LOG_DEBUG_DEV(dh_dev, "flowid_car_C = 0x%x\n", flowid_car_C);

            //4个EP * 8PF * 2收发方向，每个pf占32个car c flowid，前面2个映射到group 0
            if (0 == (flowid_car_C%(PLCR_CAR_C_FLOWIDS_PER_PF)))
            {
                flag2 = 1;
            }
        }
        else
        {
            //如果是pf（队列限速），就没有car B和Car C
            flag2 = 1;
        }

    }

    if(0 != flag2)
    {
        //执行清理操作
        PLCR_LOG_DEBUG_DEV(dh_dev, "Change to mode0: e_car_type = 0x%x, vport = 0x%x, \n", e_car_type, vport);

        //1.清理级间映射
        // for (index=0; index<cnt; index++)
        // {
        //     rtn = zxdh_plcr_clear_map(pf_dev, flow_car_type[index], flow_id[index]);
        // }

        //2.切换到mode 0模式
        zxdh_plcr_set_mode(pf_dev, vport, E_RATE_LIMIT_MODE0);
    }

    return rtn;
}

static int zxdh_plcr_create_rate_limit(struct zxdh_pf_device *pf_dev,
                                        E_RATE_LIMIT_PKT_BYTE is_pkt_mode,
                                              E_PLCR_CAR_TYPE car_type,
                                                     uint16_t vport,
                                                     uint32_t flowid,
                                                     uint32_t max_rate,
                                                     uint32_t min_rate)
{
    int rtn = 0;
    uint16_t profile_id = 0;
    DPP_STAT_CAR_PROFILE_CFG_T profile_cfg;
    struct zxdh_plcr_flow *plcr_flow = NULL;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    // PLCR_FUNC_DBG_ENTER();

    /*1. 对于一个先前还未限速的队列，如果限速值是0，这个限速配置操作无意义，就直接结束*/
    if ((0 == max_rate) && (0 == min_rate))
    {
        PLCR_LOG_DEBUG_DEV(dh_dev, "duplicate max_rate=%d on flowid=%d\n", max_rate, flowid);
        return PLCR_DUPLICATE_RATE;
    }

    /*2. 申请flow结构体，并存储到xarray*/
    rtn = zxdh_plcr_req_flow(pf_dev, car_type, flowid, &plcr_flow);
    PLCR_COMM_ASSERT(rtn);

    /*3. 更新flow信息*/
    zxdh_plcr_update_flow(plcr_flow, vport, max_rate, min_rate);

    /*3. 根据限速值，生成限速模板配置参数*/
    rtn = zxdh_plcr_gen_profile(pf_dev, is_pkt_mode, car_type, max_rate, min_rate, &profile_cfg);
    if (rtn)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed to call zxdh_plcr_gen_profile()\n");
        goto err3;
    }

    /*4. 查询是否有共享模板：没有共享限速模板，就要申请一个*/
    rtn = zxdh_plcr_match_profile(pf_dev, car_type, &profile_cfg, &profile_id);
    if (rtn)
    {
        /*4.1 申请新的限速模板*/
        rtn = zxdh_plcr_req_profile(pf_dev, car_type, &profile_id);
        if (rtn)
        {
            PLCR_LOG_ERR_DEV(dh_dev, "failed to call zxdh_plcr_req_profile()\n");
            goto err3;
        }

        /*4.2 将profile_id更新到限速模板参数中去*/
        zxdh_plcr_update_profile(&profile_cfg, profile_id);

        /*4.3 将限速模板参数，配置到寄存器中去*/
        rtn = zxdh_plcr_cfg_profile(pf_dev, car_type, &profile_cfg);
        if (rtn)
        {
            PLCR_LOG_ERR_DEV(dh_dev, "failed to call zxdh_plcr_cfg_profile()\n");
            goto err2;
        }

        /*4.4 将限速模板配置参数，保存到zxdh_plcr_profile结构体*/
        rtn = zxdh_plcr_store_profile(pf_dev, car_type, max_rate, min_rate, &profile_cfg);
        if (rtn)
        {
            PLCR_LOG_ERR_DEV(dh_dev, "failed to call zxdh_plcr_store_profile()\n");
            goto err2;
        }
    }

    /*5. 关联flow与profile*/
    rtn = zxdh_plcr_bind_flow_profile(pf_dev, car_type, flowid, profile_id);
    if (rtn)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed to call zxdh_plcr_bind_flow_profile()\n");
        goto err2;
    }

    /*6. 新模板使用计数+1*/
    rtn = zxdh_plcr_count_up_profile(pf_dev, car_type, profile_id);
    if (rtn)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed to call zxdh_plcr_count_up_profile()\n");
        goto err1;
    }

    return rtn;

    /*7. 错误处理*/
err1:
    zxdh_plcr_unbind_flow_profile(pf_dev, car_type, flowid, profile_id, 0);
err2:
    zxdh_plcr_release_profile(pf_dev, car_type, profile_id, 0);
err3:
    zxdh_plcr_release_flow(pf_dev, car_type, flowid);

    return rtn;
}

static int zxdh_plcr_modify_rate_limit(struct zxdh_pf_device *pf_dev,
                                        E_RATE_LIMIT_PKT_BYTE is_pkt_mode,
                                              E_PLCR_CAR_TYPE car_type,
                                                     uint32_t flowid,
                                                     uint32_t max_rate,
                                                     uint32_t min_rate)
{
    int rtn                                 = 0;
    uint16_t profile_id                     = 0;
    DPP_STAT_CAR_PROFILE_CFG_T profile_cfg;
    struct xarray            *xarray_flowid = &(pf_dev->plcr_table.plcr_flows[car_type]);
    struct zxdh_plcr_flow    *plcr_flow = xa_load(xarray_flowid, flowid);
    struct zxdh_plcr_profile *profile_old   = NULL;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    // PLCR_FUNC_DBG_ENTER();

    /*1. 如果二次修改的限速值和原来的一样，这个配置操作无意义，直接结束*/
    if ((E_PLCR_CAR_A == car_type) || (E_PLCR_CAR_C == car_type))
    {
        if (plcr_flow->max_rate == max_rate)
        {
            PLCR_LOG_DEBUG_DEV(dh_dev, "duplicate max_rate=%d on flowid=%d\n", max_rate, flowid);
            return PLCR_DUPLICATE_RATE;
        }
    }
    else if (E_PLCR_CAR_B == car_type)
    {
        if ((plcr_flow->max_rate == max_rate) && (plcr_flow->min_rate == min_rate))
        {
            PLCR_LOG_DEBUG_DEV(dh_dev, "duplicate max_rate=%d, min_rate=%d on flowid=%d\n", max_rate, min_rate, flowid);
            return PLCR_DUPLICATE_RATE;
        }
    }
    else
    {
        return -EINVAL;
    }

    /*2. 根据新的限速值，生成限速模板参数*/
    rtn = zxdh_plcr_gen_profile(pf_dev, is_pkt_mode, car_type, max_rate, min_rate, &profile_cfg);
    PLCR_COMM_ASSERT(rtn);

    /*3. 获取原来关联的限速模板*/
    rtn = zxdh_plcr_get_profile_by_flowid(pf_dev, car_type, flowid, &profile_old);
    PLCR_COMM_ASSERT(rtn);

    /*3. 先查询有没有相同速率的限速模板*/
    rtn = zxdh_plcr_match_profile(pf_dev, car_type, &profile_cfg, &profile_id);
    if (rtn)
    {
        /*3.1 没有找到能共享的限速模板*/

        /*3.2 原来的模板不是共享模板：直接修改限速模板的限速值*/
        if (1 == profile_old->ref_cnt)
        {
            /*3.2.1 将profile_id更新到限速模板参数中去*/
            zxdh_plcr_update_profile(&profile_cfg, profile_old->profile_id);

            /*3.2.2 将限速模板的参数，配置到寄存器中去*/
            rtn = zxdh_plcr_cfg_profile(pf_dev, car_type, &profile_cfg);
            PLCR_COMM_ASSERT(rtn);

            /*3.2.3 将限速模板配置参数，保存到zxdh_plcr_profile结构体*/
            rtn = zxdh_plcr_store_profile(pf_dev, car_type, max_rate, min_rate, &profile_cfg);
            PLCR_COMM_ASSERT(rtn);

            /* 更新flow中记录的用户原始限速值，todo：是否使用bind函数，要考虑后期在哪里加锁*/
            zxdh_plcr_update_flow(plcr_flow, plcr_flow->vport, max_rate, min_rate);

            /*这种情况只修改限速模板的寄存器，flowid先前已经与profile绑定了*/
            return rtn;
        }

        /*3.3 原来的限速模板是共享模板，所以要申请新的限速模板*/
        rtn = zxdh_plcr_req_profile(pf_dev, car_type, &profile_id);
        PLCR_COMM_ASSERT(rtn);

        /*3.4 将profile_id更新到限速模板参数中去*/
        zxdh_plcr_update_profile(&profile_cfg, profile_id);

        /*3.5 将限速模板的参数，配置到寄存器中去*/
        rtn = zxdh_plcr_cfg_profile(pf_dev, car_type, &profile_cfg);
        if (rtn)
        {
            PLCR_LOG_ERR_DEV(dh_dev, "failed to call zxdh_plcr_cfg_profile()\n");
            goto err4;
        }

        /*3.6 将限速模板配置参数，保存到zxdh_plcr_profile结构体*/
        rtn = zxdh_plcr_store_profile(pf_dev, car_type, max_rate, min_rate, &profile_cfg);
        if (rtn)
        {
            PLCR_LOG_ERR_DEV(dh_dev, "failed to call zxdh_plcr_store_profile()\n");
            goto err4;
        }

        /*3.7 接下来的绑定流程，和下面是共享的*/
    }
    /*4. 查询到共享的限速模板：直接进行绑定即可*/
    rtn = zxdh_plcr_bind_flow_profile(pf_dev, car_type, flowid, profile_id);
    if (rtn)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed to call zxdh_plcr_bind_flow_profile()\n");
        goto err4;
    }

    /*5. 更新flow中记录的用户原始限速值*/
    zxdh_plcr_update_flow(plcr_flow, plcr_flow->vport, max_rate, min_rate);

    /*6. 新模板使用计数+1*/
    rtn = zxdh_plcr_count_up_profile(pf_dev, car_type, profile_id);
    if (rtn)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed to call zxdh_plcr_count_up_profile()\n");
        goto err4;
    }

    /*7. 旧的计数模板-1*/
    rtn = zxdh_plcr_count_down_profile(pf_dev, car_type, profile_old->profile_id);
    if (rtn)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed to call zxdh_plcr_count_up_profile()\n");
        goto err4;
    }

    zxdh_plcr_release_profile(pf_dev, car_type, profile_old->profile_id, 0);

    return rtn;

err4:
    zxdh_plcr_release_profile(pf_dev, car_type, profile_id, 0);
    return rtn;
}

int zxdh_plcr_remove_rate_limit(struct zxdh_pf_device *pf_dev, E_PLCR_CAR_TYPE car_type,
                                uint32_t flowid, uint32_t flag)
{
    int rtn = 0;
    struct zxdh_plcr_profile *profile_old = NULL;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    // PLCR_FUNC_DBG_ENTER();

    PLCR_LOG_DEBUG_DEV(dh_dev, "car_type=%d,flowid=%d\n",car_type,flowid);

    /* 获取原来关联的限速模板*/
    rtn = zxdh_plcr_get_profile_by_flowid(pf_dev, car_type, flowid, &profile_old);
    PLCR_COMM_ASSERT(rtn);

    /*解除绑定*/
    rtn = zxdh_plcr_unbind_flow_profile(pf_dev, car_type, flowid, profile_old->profile_id, flag);
    //PLCR_COMM_ASSERT(rtn);

    /*限速模板引用计数 -1*/
    rtn = zxdh_plcr_count_down_profile(pf_dev, car_type, profile_old->profile_id);
    //PLCR_COMM_ASSERT(rtn);

    /*释放限速模板：如果引用计数为0，归还模板资源 & 删除xarray元素 & 释放profile指针*/
    rtn = zxdh_plcr_release_profile(pf_dev, car_type, profile_old->profile_id, flag);
    //PLCR_COMM_ASSERT(rtn);

    /*释放掉flow*/
    rtn = zxdh_plcr_release_flow(pf_dev, car_type, flowid);
    //PLCR_COMM_ASSERT(rtn);

    return rtn;
}

void zxdh_plcr_count_profiles(struct zxdh_pf_device *pf_dev)
{
    struct zxdh_plcr_profile *profile;
    unsigned long index;
    uint32_t count = 0;
    E_PLCR_CAR_TYPE car_type;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    for (car_type = E_PLCR_CAR_A; car_type < E_PLCR_CAR_NUM; car_type ++)
    {
        count = 0;
        xa_for_each_range(&(pf_dev->plcr_table.plcr_profiles[car_type]), index, profile, 0, gaudPlcrCarxProfileNum[car_type])
        {
            count++;
        }
        PLCR_LOG_DEBUG_DEV(dh_dev, "car_type = %d, profiles_num = %d\n", car_type, count);
    }
}

int zxdh_plcr_set_rate_limit(struct zxdh_pf_device *pf_dev,
                              E_RATE_LIMIT_PKT_BYTE is_pkt_mode,
                                    E_PLCR_CAR_TYPE car_type,
                                           uint16_t vport,
                                           uint32_t flowid,
                                           uint32_t max_rate,
                                           uint32_t min_rate)
{
    int rtn = 0;
    struct xarray *xarray_flow = &(pf_dev->plcr_table.plcr_flows[car_type]);
    struct zxdh_plcr_flow *flow = NULL;

    // PLCR_FUNC_DBG_ENTER();

    /*1. 判断该队列先前是否已经配置过限速值*/
    flow = xa_load(xarray_flow, flowid);
    if (NULL == flow)
    {
        /*1.1 初次配置限速值*/
        rtn = zxdh_plcr_create_rate_limit(pf_dev, is_pkt_mode, car_type, vport, flowid, max_rate, min_rate);
        PLCR_COMM_ASSERT(rtn);
    }
    else if ((max_rate != 0) || (min_rate != 0))
    {
        /*1.2 修改限速值*/
        rtn = zxdh_plcr_modify_rate_limit(pf_dev, is_pkt_mode, car_type, flowid, max_rate, min_rate);
        PLCR_COMM_ASSERT(rtn);
    }
    else
    {
        /*1.3.1 解除限速：即，第二次配置，且max_rate=0，表示用户要解除限速*/
        rtn = zxdh_plcr_remove_rate_limit(pf_dev, car_type, flowid, 0);
        PLCR_COMM_ASSERT(rtn);

        zxdh_plcr_check_release_flow_chain(pf_dev, car_type, vport);

        //不是错误码，用来标记是进行了解除限速的操作
        rtn = PLCR_REMOVE_RATE_LIMIT;
    }

    zxdh_plcr_count_profiles(pf_dev);

    return rtn;
}

int zxdh_pf_plcr_set_mode(struct zxdh_pf_device *pf_dev, uint16_t vport, E_RATE_LIMIT_MODE mode)
{
    int rtn = 0;
    uint32_t enable = 0;
    DPP_PF_INFO_T pf_info = {0};
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    pf_info.slot  = pf_dev->slot_id;
    pf_info.vport = vport;
    PLCR_LOG_DEBUG_DEV(dh_dev, "plcr_set_mode slot: %d, vport: 0x%x, mode: 0x%x\n", pf_info.slot, pf_info.vport, mode);

    //Check if the vport attribute table exists.
    rtn = dpp_vport_egress_meter_en_get(&pf_info, &enable);
    if(ZXIC_PAR_CHK_INVALID_INDEX == rtn)
    {
        PLCR_LOG_DEBUG_DEV(dh_dev, "Write vport attribute table which does not exist!\n");
        return 0;
    }
    else
    {
        PLCR_COMM_ASSERT(rtn);
    }

    //modify the vport attribute table
    if(E_RATE_LIMIT_MODE0 == mode)
    {
        rtn = dpp_vport_egress_meter_en_set(&pf_info, 0);
        PLCR_COMM_ASSERT(rtn);
        rtn = dpp_vport_ingress_meter_en_set(&pf_info, 0);
        PLCR_COMM_ASSERT(rtn);
    }
    else if(E_RATE_LIMIT_MODE1 == mode)
    {
        rtn = dpp_vport_egress_meter_en_set(&pf_info, 0);
        PLCR_COMM_ASSERT(rtn);
        rtn = dpp_vport_ingress_meter_en_set(&pf_info, 0);
        PLCR_COMM_ASSERT(rtn);

        rtn = dpp_vport_egress_meter_mode_set(&pf_info, 1);
        PLCR_COMM_ASSERT(rtn);
        rtn = dpp_vport_ingress_meter_mode_set(&pf_info, 1);
        PLCR_COMM_ASSERT(rtn);

        rtn = dpp_vport_egress_meter_en_set(&pf_info, 1);
        PLCR_COMM_ASSERT(rtn);
        rtn = dpp_vport_ingress_meter_en_set(&pf_info, 1);
        PLCR_COMM_ASSERT(rtn);
    }
    else if(E_RATE_LIMIT_MODE2 == mode)
    {
        rtn = dpp_vport_egress_meter_en_set(&pf_info, 0);
        PLCR_COMM_ASSERT(rtn);
        rtn = dpp_vport_ingress_meter_en_set(&pf_info, 0);
        PLCR_COMM_ASSERT(rtn);

        rtn = dpp_vport_egress_meter_mode_set(&pf_info, 0);
        PLCR_COMM_ASSERT(rtn);
        rtn = dpp_vport_ingress_meter_mode_set(&pf_info, 0);
        PLCR_COMM_ASSERT(rtn);

        rtn = dpp_vport_egress_meter_en_set(&pf_info, 1);
        PLCR_COMM_ASSERT(rtn);
        rtn = dpp_vport_ingress_meter_en_set(&pf_info, 1);
        PLCR_COMM_ASSERT(rtn);
    }
    else
    {
        return -ERANGE;
    }

    return rtn;
}

int zxdh_pf_plcr_get_mode(struct zxdh_pf_device *pf_dev, uint16_t vport, E_RATE_LIMIT_MODE *p_mode)
{
    int      rtn    = 0;
    uint32_t enable = 0;
    uint32_t mode   = 0;
    DPP_PF_INFO_T pf_info = {0};
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    // PLCR_FUNC_DBG_ENTER();

    pf_info.slot  = pf_dev->slot_id;
    pf_info.vport = vport;
    PLCR_LOG_DEBUG_DEV(dh_dev, "pf_info.slot = %d, pf_info.vport = 0x%x\n", pf_info.slot, pf_info.vport);

    rtn = dpp_vport_egress_meter_en_get(&pf_info, &enable);
    if(ZXIC_PAR_CHK_INVALID_INDEX == rtn)
    {
        PLCR_LOG_DEBUG_DEV(dh_dev, "Read vport attribute table which does not exist!\n");
        *p_mode = E_RATE_LIMIT_MODE3;
        return 0;
    }
    else
    {
        PLCR_COMM_ASSERT(rtn);
    }

    if(0 == enable)
    {
        *p_mode = E_RATE_LIMIT_MODE0;
    }
    else
    {
        rtn = dpp_vport_egress_meter_mode_get(&pf_info, &mode);
        PLCR_COMM_ASSERT(rtn);

        if(1 == mode)
        {
            *p_mode = E_RATE_LIMIT_MODE1;
        }
        else
        {
            *p_mode = E_RATE_LIMIT_MODE2;
        }
    }

    PLCR_LOG_DEBUG_DEV(dh_dev, "mode = %d\n", *p_mode);

    return rtn;
}

int zxdh_plcr_set_mode(struct zxdh_pf_device *pf_dev, uint16_t vport, E_RATE_LIMIT_MODE mode)
{
    int32_t  rtn = 0;
    struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    if (dh_dev->coredev_type == DH_COREDEV_PF)
    {
        rtn = zxdh_pf_plcr_set_mode(pf_dev, vport, mode);
        PLCR_COMM_ASSERT(rtn);
    }
    else
    {
        msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
        if (unlikely(NULL == msg))
        {
            PLCR_LOG_ERR_DEV(dh_dev, "failed to kzalloc\n");
            return -ENOMEM;
        }
        msg->payload.hdr.op_code = ZXDH_PLCR_SET_MODE;
        msg->payload.hdr.vport   = pf_dev->vport;
        msg->payload.hdr.pcie_id = pf_dev->pcie_id;
        msg->payload.hdr.vf_id   = pf_dev->pcie_id & (0xff);

        msg->payload.plcr_work_mode_msg.vport = vport;
        msg->payload.plcr_work_mode_msg.mode  = mode;

        rtn = zxdh_pf_msg_send_cmd(dh_dev, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
        kfree(msg);
        PLCR_COMM_ASSERT(rtn);
    }

    return rtn;
}
EXPORT_SYMBOL(zxdh_plcr_set_mode);


int zxdh_plcr_get_mode(struct zxdh_pf_device *pf_dev, uint16_t vport, E_RATE_LIMIT_MODE *mode)
{
    int32_t  rtn = 0;
    struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    if (dh_dev->coredev_type == DH_COREDEV_PF)
    {
        rtn = zxdh_pf_plcr_get_mode(pf_dev, vport, mode);
        PLCR_COMM_ASSERT(rtn);
    }
    else
    {
        msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
        if (unlikely(NULL == msg))
        {
            PLCR_LOG_ERR_DEV(dh_dev, "failed to kzalloc\n");
            return -ENOMEM;
        }
        msg->payload.hdr.op_code = ZXDH_PLCR_GET_MODE;
        msg->payload.hdr.vport   = pf_dev->vport;
        msg->payload.hdr.pcie_id = pf_dev->pcie_id;
        msg->payload.hdr.vf_id   = pf_dev->pcie_id & (0xff);

        msg->payload.plcr_work_mode_msg.vport = vport;
        PLCR_LOG_DEBUG_DEV(dh_dev, "msg->payload.hdr.vf_id %u\n",msg->payload.hdr.vf_id);
        rtn = zxdh_pf_msg_send_cmd(dh_dev, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
        if (rtn)
        {
            PLCR_LOG_ERR_DEV(dh_dev, "failed and rtn=%d\n", rtn);
            kfree(msg);
            return rtn;
        }

        *mode = msg->reps.plcr_work_mode_rsp.mode;
        kfree(msg);

        PLCR_LOG_DEBUG_DEV(dh_dev, "mode = %d\n", *mode);
    }

    return rtn;
}

int zxdh_plcr_show_rate_limit_paras(zxdh_plcr_rate_limit_paras *rate_limit_paras)
{
    PLCR_LOG_INFO("rate_limit_paras->req_type   = 0x%x\n", rate_limit_paras->req_type);
    PLCR_LOG_INFO("rate_limit_paras->direction  = 0x%x\n", rate_limit_paras->direction);
    PLCR_LOG_INFO("rate_limit_paras->mode       = 0x%x\n", rate_limit_paras->mode);
    PLCR_LOG_INFO("rate_limit_paras->max_rate   = 0x%x\n", rate_limit_paras->max_rate);
    PLCR_LOG_INFO("rate_limit_paras->min_rate   = 0x%x\n", rate_limit_paras->min_rate);
    PLCR_LOG_INFO("rate_limit_paras->queue_id   = 0x%x\n", rate_limit_paras->queue_id);
    PLCR_LOG_INFO("rate_limit_paras->vf_idx     = 0x%x\n", rate_limit_paras->vf_idx);
    PLCR_LOG_INFO("rate_limit_paras->vfid       = 0x%x\n", rate_limit_paras->vfid);
    PLCR_LOG_INFO("rate_limit_paras->vport      = 0x%x\n", rate_limit_paras->vport);
    PLCR_LOG_INFO("rate_limit_paras->group_id   = 0x%x\n", rate_limit_paras->group_id);

    return 0;
}

int zxdh_plcr_check_req_type(struct zxdh_pf_device *pf_dev, E_RATE_LIMIT_MODE mode, zxdh_plcr_rate_limit_paras *rate_limit_paras, E_RATE_LIMIT_REQ_TYPE *req_type)
{
    // PLCR_FUNC_DBG_ENTER();

    //队列字节限速
    if ((rate_limit_paras->req_type  == E_RATE_LIMIT_REQ_QUEUE_BYTE) &&
        (rate_limit_paras->mode      == E_RATE_LIMIT_BYTE) &&
        ((rate_limit_paras->min_rate != PLCR_INVALID_PARAM) || (rate_limit_paras->max_rate != PLCR_INVALID_PARAM)) &&
        (rate_limit_paras->direction == E_RATE_LIMIT_TX) &&
        (rate_limit_paras->vf_idx    == PLCR_INVALID_PARAM) &&
        (rate_limit_paras->group_id  == PLCR_INVALID_PARAM) &&
        (rate_limit_paras->queue_id  <  PLCR_MAX_QUEUE_PAIRS))
    {
        *req_type =  E_RATE_LIMIT_REQ_QUEUE_BYTE;

        //mode2模式下不支持队列字节限速
        if(E_RATE_LIMIT_MODE2 == mode)
        {
            PLCR_LOG_ERR("E_RATE_LIMIT_REQ_QUEUE_BYTE is not supported under E_RATE_LIMIT_MODE2\n");
            return -EPERM;
        }

        //端口组默认为0
        rate_limit_paras->group_id = 0;

        return 0;
    }
    //vf端口字节限速
    else if ((rate_limit_paras->req_type  == E_RATE_LIMIT_REQ_VF_BYTE) &&
             (rate_limit_paras->mode      == E_RATE_LIMIT_BYTE) &&
            ((rate_limit_paras->min_rate  != PLCR_INVALID_PARAM) || (rate_limit_paras->max_rate != PLCR_INVALID_PARAM)) &&
            ((rate_limit_paras->direction == E_RATE_LIMIT_RX) || (rate_limit_paras->direction == E_RATE_LIMIT_TX)) &&
             (rate_limit_paras->group_id  == PLCR_INVALID_PARAM) &&
             (rate_limit_paras->vf_idx    != PLCR_INVALID_PARAM))
    {

        //端口组默认为0：
        rate_limit_paras->group_id = 0;

        *req_type =  E_RATE_LIMIT_REQ_VF_BYTE;
        return 0;
    }
    //vf端口包限速
    else if ((rate_limit_paras->req_type   == E_RATE_LIMIT_REQ_VF_PKT) &&
             (rate_limit_paras->mode       == E_RATE_LIMIT_PACKET) &&
             ((rate_limit_paras->min_rate  != PLCR_INVALID_PARAM) || (rate_limit_paras->max_rate != PLCR_INVALID_PARAM)) &&
             ((rate_limit_paras->direction == E_RATE_LIMIT_RX) || (rate_limit_paras->direction == E_RATE_LIMIT_TX)) &&
             (rate_limit_paras->group_id   == PLCR_INVALID_PARAM) &&
             (rate_limit_paras->vf_idx     != PLCR_INVALID_PARAM))
    {
        *req_type =  E_RATE_LIMIT_REQ_VF_PKT;
        //mode1模式下不支持端口包限速
        if(E_RATE_LIMIT_MODE1 == mode)
        {
            PLCR_LOG_ERR("E_RATE_LIMIT_REQ_VF_PKT is not supported under E_RATE_LIMIT_MODE1\n");
            return -EPERM;
        }

        //端口组默认为0
        rate_limit_paras->group_id = 0;

        return 0;
    }
    //端口组字节限速
    else if ((rate_limit_paras->req_type   == E_RATE_LIMIT_REQ_VF_GROUP_BYTE) &&
             (rate_limit_paras->mode       == E_RATE_LIMIT_BYTE) &&
             ((rate_limit_paras->min_rate  != PLCR_INVALID_PARAM) || (rate_limit_paras->max_rate != PLCR_INVALID_PARAM)) &&
             ((rate_limit_paras->direction == E_RATE_LIMIT_RX) || (rate_limit_paras->direction == E_RATE_LIMIT_TX)) &&
              (rate_limit_paras->vf_idx    == PLCR_INVALID_PARAM) && //不需要flowid级间映射，不需要vf_idx，不需要Vfid
              (rate_limit_paras->group_id  != PLCR_INVALID_PARAM))
    {
        *req_type =  E_RATE_LIMIT_REQ_VF_GROUP_BYTE;
        return 0;
    }
    //移动vf端口组
    else if ((rate_limit_paras->req_type   == E_RATE_LIMIT_REQ_MOVE_VF_GROUP) &&
             (rate_limit_paras->mode       == PLCR_INVALID_PARAM) &&
             ((rate_limit_paras->min_rate  == PLCR_INVALID_PARAM) && (rate_limit_paras->max_rate == PLCR_INVALID_PARAM)) &&
             ((rate_limit_paras->direction == E_RATE_LIMIT_RX) || (rate_limit_paras->direction == E_RATE_LIMIT_TX)) &&
             (rate_limit_paras->vf_idx     != PLCR_INVALID_PARAM) &&
             (rate_limit_paras->group_id   != PLCR_INVALID_PARAM))
    {
        *req_type =  E_RATE_LIMIT_REQ_MOVE_VF_GROUP;
        return 0;
    }
    else
    {
        //将入参中的请求信息打印出来
        zxdh_plcr_show_rate_limit_paras(rate_limit_paras);

        return PLCR_GET_REQ_TYPE_INVALID_ERR;
    }
}

int32_t zxdh_pf_get_vf_queue_info(struct zxdh_pf_device *pf_dev, int32_t vf_idx, int32_t *phy_queue_num, int32_t *phy_rx_queue, int32_t *phy_tx_queue)
{
    int32_t rtn = 0;
    int32_t i;
    union zxdh_msg *msg = NULL;
    int32_t queue_pair_index;
    int32_t queue_num;
    int32_t queue_pair = 0;
    struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
    struct zxdh_en_device *en_dev;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    // PLCR_FUNC_DBG_ENTER();

    en_dev = pf_dev_get_edev(pf_dev);
    if (IS_ERR(en_dev))
        return PTR_ERR(en_dev);

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed to kzalloc\n");
        return -ENOMEM;
    }
    msg->payload.hdr_vf.op_code = ZXDH_PF_GET_VF_QUEUE_INFO;
    msg->payload.hdr_vf.dst_pcie_id = FIND_VF_PCIE_ID(pf_dev->pcie_id, vf_idx);

    for(queue_pair_index = 0; queue_pair_index < PLCR_MAX_QUEUE_PAIRS;)
    {
        msg->payload.plcr_pf_get_vf_queue_info_msg.vir_queue_start = queue_pair_index;
        msg->payload.plcr_pf_get_vf_queue_info_msg.vir_queue_num = 16;

        //get rx&tx phy queue
        PLCR_LOG_DEBUG_DEV(dh_dev, "vir_queue_start = 0x%x\n", msg->payload.plcr_pf_get_vf_queue_info_msg.vir_queue_start);
        PLCR_LOG_DEBUG_DEV(dh_dev, "vir_queue_num   = 0x%x\n", msg->payload.plcr_pf_get_vf_queue_info_msg.vir_queue_num);
        rtn = zxdh_pf_msg_send_cmd(dh_dev, MODULE_PF_BAR_MSG_TO_VF, msg, msg, &para);
        if (rtn)
        {
            PLCR_LOG_ERR_DEV(dh_dev, "failed and rtn=%d\n", rtn);
            goto free_msg;
        }

        queue_num = msg->reps.plcr_pf_get_vf_queue_info_rsp.phy_queue_num;
        for(i = 0; i < queue_num; i++)
        {
            phy_rx_queue[queue_pair_index*16 + i] = msg->reps.plcr_pf_get_vf_queue_info_rsp.phy_rxq[i];
            phy_tx_queue[queue_pair_index*16 + i] = msg->reps.plcr_pf_get_vf_queue_info_rsp.phy_txq[i];

            PLCR_LOG_DEBUG_DEV(dh_dev, "rxq: 0x%x  -  0x%x\n", queue_pair_index*16 + i, phy_rx_queue[queue_pair_index*16 + i]);
            PLCR_LOG_DEBUG_DEV(dh_dev, "txq: 0x%x  -  0x%x\n", queue_pair_index*16 + i, phy_tx_queue[queue_pair_index*16 + i]);
        }

        queue_pair += queue_num;

        if(queue_num < 16)
        {
            *phy_queue_num = queue_pair;
            PLCR_LOG_DEBUG_DEV(dh_dev, "phy_queue_num = 0x%x\n", *phy_queue_num);
            goto free_msg;
        }
        else
        {
            queue_pair_index += 16;
        }
    }

free_msg:
    kfree(msg);
    return rtn;
}

int zxdh_plcr_map_flowid(struct zxdh_pf_device *pf_dev, E_PLCR_CAR_TYPE car_type, uint32_t flowid, uint32_t map_flowid)
{
    int rtn = 0;
    uint32_t map_sp = 0;  //priority
    union zxdh_msg *msg = NULL;
    DPP_PF_INFO_T pf_info = {0};
    struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    // PLCR_FUNC_DBG_ENTER();
    PLCR_LOG_DEBUG_DEV(dh_dev, "car_type = 0x%x, flowid = 0x%x, map_flowid = 0x%x\n", car_type, flowid, map_flowid);

    if (dh_dev->coredev_type == DH_COREDEV_PF)
    {
        //前面的流程会判断是否需要进行映射，不会出现vf端口原来是非0group，现在会被group 0覆盖的情况
        PLCR_LOG_DEBUG_DEV(dh_dev, "flowid=0x%x, map_flowid=0x%x\n", flowid, map_flowid);
        pf_info.slot = pf_dev->slot_id;
        pf_info.vport = pf_dev->vport;
        PLCR_LOG_DEBUG_DEV(dh_dev, "dpp_car_queue_map_set: pf_info.vport = 0x%x, car_type = %d, flowid = %d, map_flowid = %d\n", pf_info.vport, car_type, flowid, map_flowid);
        rtn = dpp_car_queue_map_set(&pf_info, car_type, flowid, map_flowid, map_sp);
        PLCR_COMM_ASSERT(rtn);

        rtn = zxdh_plcr_stroe_map(pf_dev, car_type, flowid, map_flowid);
    }
    else
    {
        msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
        if (unlikely(NULL == msg))
        {
            PLCR_LOG_ERR_DEV(dh_dev, "failed to kzalloc\n");
            return -ENOMEM;
        }
        msg->payload.hdr.op_code = ZXDH_MAP_PLCR_FLOWID;
        msg->payload.hdr.vport   = pf_dev->vport;
        msg->payload.hdr.pcie_id = pf_dev->pcie_id;
        msg->payload.hdr.vf_id   = pf_dev->pcie_id & (0xff);

        msg->payload.plcr_flowid_map_msg.car_type   = car_type;
        msg->payload.plcr_flowid_map_msg.flowid     = flowid;
        msg->payload.plcr_flowid_map_msg.map_flowid = map_flowid;
        msg->payload.plcr_flowid_map_msg.sp         = map_sp;
        PLCR_LOG_DEBUG_DEV(dh_dev, "flowid=0x%x, map_flowid=0x%x\n", flowid, map_flowid);

        rtn = zxdh_pf_msg_send_cmd(dh_dev, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
        kfree(msg);
        PLCR_COMM_ASSERT(rtn);
    }

    return rtn;
}

int zxdh_plcr_mode_init(struct zxdh_pf_device *pf_dev)
{
    int32_t rtn = 0;
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot  = pf_dev->slot_id;
    pf_info.vport = pf_dev->vport;

    rtn = zxdh_plcr_set_mode(pf_dev, pf_dev->vport, E_RATE_LIMIT_MODE0);
    PLCR_COMM_ASSERT(rtn);

    return rtn;
}
int32_t zxdh_plcr_recover_cfg(struct zxdh_vf_item *vf_item,struct zxdh_pf_device *pf_dev,int32_t vf_idx)
{
    int32_t rtn = 0;
    zxdh_plcr_rate_limit_paras rate_limit_paras;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    PLCR_LOG_DEBUG_DEV(dh_dev, "zxdh_plcr_recover_cfg  zxdh_vf_item %p\n",vf_item);
    if(vf_item == NULL)
    {
        PLCR_LOG_DEBUG_DEV(dh_dev, "plcr init  vfid %u   vfitem null\n",vf_idx);
        return 0;
    }
    PLCR_LOG_DEBUG_DEV(dh_dev, "zxdh_plcr_recover_cfg  vfid %u   maxrate %u\n",vf_idx,vf_item->max_tx_rate);
    if(vf_item->max_tx_rate != 0 || vf_item->min_tx_rate != 0)
    {
            rate_limit_paras.req_type  = E_RATE_LIMIT_REQ_VF_BYTE;
            rate_limit_paras.direction = E_RATE_LIMIT_TX;
            rate_limit_paras.mode      = E_RATE_LIMIT_BYTE ;
            rate_limit_paras.max_rate  = vf_item->max_tx_rate;
            rate_limit_paras.min_rate  = vf_item->min_tx_rate;
            rate_limit_paras.queue_id  = PLCR_INVALID_PARAM;
            rate_limit_paras.vf_idx    = vf_idx;
            rate_limit_paras.vfid      = PLCR_INVALID_PARAM;
            rate_limit_paras.group_id  = PLCR_INVALID_PARAM;

            rtn = zxdh_plcr_unified_set_rate_limit(pf_dev, &rate_limit_paras);
    }
    
    return rtn;
}
int32_t zxdh_plcr_init(struct zxdh_en_priv *en_priv)
{
    int32_t rtn = 0;
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct dh_core_dev    *dh_dev = en_dev->parent->parent;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    if (dh_dev->coredev_type == DH_COREDEV_PF)
    {
        xa_init(&pf_dev->plcr_table.plcr_profiles[E_PLCR_CAR_A]);
        xa_init(&pf_dev->plcr_table.plcr_flows[E_PLCR_CAR_A]);
        xa_init(&pf_dev->plcr_table.plcr_maps[E_PLCR_CAR_A]);

        xa_init(&pf_dev->plcr_table.plcr_profiles[E_PLCR_CAR_B]);
        xa_init(&pf_dev->plcr_table.plcr_flows[E_PLCR_CAR_B]);
        xa_init(&pf_dev->plcr_table.plcr_maps[E_PLCR_CAR_B]);

        xa_init(&pf_dev->plcr_table.plcr_profiles[E_PLCR_CAR_C]);
        xa_init(&pf_dev->plcr_table.plcr_flows[E_PLCR_CAR_C]);

        pf_dev->plcr_table.burst_size = 0;
        pf_dev->plcr_table.is_xarray_init = true;
    }

    //vf需要设置到mode0模式
    if (dh_dev->coredev_type != DH_COREDEV_VF)
    {
        rtn = zxdh_plcr_mode_init(pf_dev);
        PLCR_COMM_ASSERT(rtn);
    }
    pf_dev->plcr_table.is_init = true;

    return rtn;
}
EXPORT_SYMBOL(zxdh_plcr_init);

/*释放PF/VF*/
int32_t zxdh_plcr_uninit(struct zxdh_en_priv *en_priv)
{
    int rtn = 0;
    union zxdh_msg *msg = NULL;
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct dh_core_dev    *dh_dev = en_dev->parent->parent;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev);

    unsigned long             flow_id;
    E_PLCR_CAR_TYPE           car_index;
    struct xarray            *xarray_flow;
    struct xarray            *xarray_profile;
    struct zxdh_plcr_flow    *flow = NULL;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    if (dh_dev->coredev_type == DH_COREDEV_PF)
    {
        for(car_index = E_PLCR_CAR_A; car_index <= E_PLCR_CAR_C; car_index ++)
        {
            xarray_flow    = &(pf_dev->plcr_table.plcr_flows[car_index]);
            xarray_profile = &(pf_dev->plcr_table.plcr_profiles[car_index]);
            xa_for_each_range(xarray_flow, flow_id, flow, 0, gaudPlcrCarxFlowIdNum[car_index])
            {
                zxdh_plcr_remove_rate_limit(pf_dev, car_index, flow_id, en_dev->quick_remove);

                //clear all vport mappings between car B and car C.
                if(E_PLCR_CAR_B == car_index)
                {
                    zxdh_plcr_clear_map(pf_dev, car_index, flow_id);
                }
            }
            xa_destroy(xarray_flow);
            xa_destroy(xarray_profile);
        }

        pf_dev->plcr_table.is_init = false;
    }
    else if (dh_dev->coredev_type == DH_COREDEV_VF)
    {
        if(!en_dev->quick_remove)
        {
            //解除一二级flowid与profile之间的绑定关系，并释放profile
            msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
            if (unlikely(NULL == msg))
            {
                PLCR_LOG_ERR_DEV(dh_dev, "failed to kzalloc\n");
                return -ENOMEM;
            }
            msg->payload.hdr.op_code = ZXDH_PLCR_UNINIT;
            msg->payload.hdr.vport = pf_dev->vport;
            msg->payload.hdr.pcie_id = pf_dev->pcie_id;
            msg->payload.hdr.vf_id = pf_dev->pcie_id & (0xff);

            rtn = zxdh_pf_msg_send_cmd(dh_dev, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
            kfree(msg);
            PLCR_COMM_ASSERT(rtn);
        }
    }
    return rtn;
}
EXPORT_SYMBOL(zxdh_plcr_uninit);



/*******************************下面是新实现的代码*******************************/
/*
函数功能：获取car A的flowid
入参：
    ---
    ---
场景：
    ---1. 如果处于mode0模式，用户请求队列字节限速，则car A的flowid就是队列
          这种场景下，pf和vf调用各自的钩子函数，所以通过en_dev直接获取队列信息
    ---2. 如果处于mode0模式，用户请求非队列字节限速，则car A的flowid就是vf端口
    ---3. 如果处于mode1模式，则不管用户是什么请求，没有必要获取car A的flowid，因为场景1已经进行了映射
    ---4. 如果处于mode2模式，则不管用户是什么请求，没有必要获取car A的flowid，因为场景2已经进行了映射
返回值：成功返回0，失败返回其它值
*/
int zxdh_plcr_get_car_a_flowid(struct zxdh_pf_device *pf_dev, E_RATE_LIMIT_MODE mode, zxdh_plcr_rate_limit_paras *rate_limit_paras, zxdh_plcr_flowids *flowids)
{
    int rtn = 0;
    uint32_t queue_pair_index;
    struct zxdh_en_device *en_dev;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    // PLCR_FUNC_DBG_ENTER();

    PLCR_LOG_DEBUG_DEV(dh_dev, "mode = %d, rate_limit_paras->req_type = %d\n", mode, rate_limit_paras->req_type);

    en_dev = pf_dev_get_edev(pf_dev);
    if (IS_ERR(en_dev))
        return PTR_ERR(en_dev);

    if (PLCR_MAX_QUEUE_PAIRS < (en_dev->curr_queue_pairs))
    {
        PLCR_COMM_ASSERT(PLCR_DEV_ALL_QID_2_FLOWID_QUEUE_PAIRS_OVERFLOW);
    }

    PLCR_LOG_DEBUG_DEV(dh_dev, "rate_limit_paras->req_type = 0x%x\n", rate_limit_paras->req_type);

    if (E_RATE_LIMIT_REQ_VF_GROUP_BYTE == rate_limit_paras->req_type)
    {
        PLCR_LOG_DEBUG_DEV(dh_dev, "E_RATE_LIMIT_REQ_VF_GROUP_BYTE does not need car A flowid!\n");
        return 0;
    }
    else if ((((E_RATE_LIMIT_MODE0 == mode)) || ((E_RATE_LIMIT_MODE1 == mode))) && (E_RATE_LIMIT_REQ_QUEUE_BYTE == rate_limit_paras->req_type))
    {
        //队列字节限速调用的是pf和vf各自的钩子，所以这里直接使用en_dev->curr_queue_pairs
        for (queue_pair_index=0; queue_pair_index < en_dev->curr_queue_pairs; queue_pair_index++)
        {
            //rx
            flowids->flowids_A[0][queue_pair_index] = en_dev->rq[queue_pair_index].vq->phy_index;

            //tx
            flowids->flowids_A[1][queue_pair_index] = en_dev->sq[queue_pair_index].vq->phy_index;

            PLCR_LOG_DEBUG_DEV(dh_dev, "flowids->flowids_A[0][%d] = 0x%x\n", queue_pair_index, flowids->flowids_A[0][queue_pair_index]);
            PLCR_LOG_DEBUG_DEV(dh_dev, "flowids->flowids_A[1][%d] = 0x%x\n", queue_pair_index, flowids->flowids_A[1][queue_pair_index]);
        }
        flowids->queue_pairs = en_dev->curr_queue_pairs;

        PLCR_LOG_DEBUG_DEV(dh_dev, "flowids->queue_pairs = 0x%x\n", flowids->queue_pairs);
    }
    else if (((E_RATE_LIMIT_MODE0 == mode) && (E_RATE_LIMIT_REQ_QUEUE_BYTE != rate_limit_paras->req_type)) ||
             ((E_RATE_LIMIT_MODE2 == mode) && (E_RATE_LIMIT_REQ_VF_PKT == rate_limit_paras->req_type)))
    {
        flowids->flowid_A[0] = rate_limit_paras->vfid * 2      + PLCR_CAR_A_DPDK_FLOWID_OFFSET;
        flowids->flowid_A[1] = rate_limit_paras->vfid * 2 + 1  + PLCR_CAR_A_DPDK_FLOWID_OFFSET;
        PLCR_LOG_DEBUG_DEV(dh_dev, "flowids->flowid_A[0] = 0x%x\n", flowids->flowid_A[0]);
        PLCR_LOG_DEBUG_DEV(dh_dev, "flowids->flowid_A[1] = 0x%x\n", flowids->flowid_A[1]);
    }
    else
    {
        PLCR_LOG_DEBUG_DEV(dh_dev, "Car A's flowid is not needed!\n");
        return 0;
    }

    return rtn;
}

int zxdh_plcr_get_car_b_flowid(struct zxdh_pf_device *pf_dev, E_RATE_LIMIT_MODE mode, zxdh_plcr_rate_limit_paras *rate_limit_paras, zxdh_plcr_flowids *flowids)
{
    int rtn = 0;

    // PLCR_FUNC_DBG_ENTER();

    //pf队列字节限速不需要指定vf_idx，视作一个特殊vf端口，同样根据vqm_vfid分配flowid
    //端口组字节限速的前提是先建立端口组，在后面流程会直接退出
    flowids->flowid_B[0] = rate_limit_paras->vfid * 2;
    flowids->flowid_B[1] = rate_limit_paras->vfid * 2 + 1;

    LOG_DEBUG("flowids->flowid_B[0] = 0x%x\n", flowids->flowid_B[0]);
    LOG_DEBUG("flowids->flowid_B[1] = 0x%x\n", flowids->flowid_B[1]);


    return rtn;
}

/*
函数功能：获取car c的flowid
资源分配：
    ---car c有1024个flowid资源
    ---4个EP * 8个PF * 2个方向（接收和发送）= 64
    ---1024 / 64 = 16，即每个pf分配16个group

背景描述：
    ---除了队列限速，其它所有限速都是操作pf下的文件系统，en_priv对应的肯定是pf设备
    ---rate_limit_paras->vport，这个指向的是vf_idx对应的vf设备
    ---所以，我们需要通过en_priv获取ep和pf func num

返回值：成功返回0，失败返回其它值
*/
int zxdh_plcr_get_car_c_flowid(struct zxdh_pf_device *pf_dev, E_RATE_LIMIT_MODE mode, zxdh_plcr_rate_limit_paras *rate_limit_paras, zxdh_plcr_flowids *flowids)
{
    uint16_t vport  = 0;
    uint32_t epid   = 0;
    uint32_t pf_num = 0;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    // PLCR_FUNC_DBG_ENTER();

    vport = pf_dev->vport;
    PLCR_LOG_DEBUG_DEV(dh_dev, "pf's info : vport = 0x%x\n", vport);

    if(rate_limit_paras->group_id >= 16)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "group_id must be less than 16!\n");
        return -ERANGE;
    }

    //用户设置vf的队列限速的时候，这里的vport是vf端口，不是pf端口
    //vf的vport和pf的vport一样，都包含了相同的epid和pf function
    epid   = EPID(vport);
    pf_num = FUNC_NUM(vport);
    PLCR_LOG_DEBUG_DEV(dh_dev, "pf's info : epid = 0x%x, pf_num = 0x%x\n", epid, pf_num);

    //ZF group0和EP0共用flowid
    epid = epid == 4 ? 0 : epid;
    //端口组要么是0，要么是用户指定（端口组字节限速，移动端口组）
    flowids->flowid_C[0] = epid * PLCR_CAR_C_FLOWIDS_PER_EP + pf_num * PLCR_CAR_C_FLOWIDS_PER_PF + rate_limit_paras->group_id * 2;
    flowids->flowid_C[1] = epid * PLCR_CAR_C_FLOWIDS_PER_EP + pf_num * PLCR_CAR_C_FLOWIDS_PER_PF + rate_limit_paras->group_id * 2 + 1;

    PLCR_LOG_DEBUG_DEV(dh_dev, "flowids->flowid_C[0] = 0x%x\n", flowids->flowid_C[0]);
    PLCR_LOG_DEBUG_DEV(dh_dev, "flowids->flowid_C[1] = 0x%x\n", flowids->flowid_C[1]);

    return 0;
}

/*
函数功能：获取对应的Vfi
入参：
    ---vf_idx : pf内vf的编号，vf_idx是查找vport的索引，是中间工具，最终使用Vfid来定位vport
    ---vfid   : vport的Vfid
场景：
    ---1.  pf队列字节限速： 用户使用echo，      调用pf的钩子函数，走if分支，    不需要指定vf_idx，不需要Vfid（不需要进行级间映射）
    ---2.  vf队列字节限速： 用户命令使用echo，   调用vf的否子函数，走else分支，  不需要指定vf_idx
    ---3.  vf端口字节限速： 用户命令使用ip link，调用pf的钩子函数，走if分支，    需要指定vf_idx
    ---3.  vf端口字节限速： 后期会支持echo，     调用pf的钩子函数，走if分支，     需要指定vf_idx
    ---4.  vf端口包限速：   用户命令使用echo，   调用pf的钩子函数，走if分支，     需要指定vf_idx
    ---5.  vf端口组字节限速：用户命令使用echo，   调用pf的钩子函数，走if分支，    不需要指定vf_idx，不需要Vfid（不需要进行级间映射）
    ---6.  vf端口移动组：   用户命令使用echo，   调用pf的钩子函数，走if分支，    需要指定vf_idx
返回值：成功返回0，失败返回其它值
*/
int zxdh_plcr_get_vport_vfid(struct zxdh_pf_device *pf_dev, uint32_t vf_idx, uint32_t *vport, uint32_t *vfid)
{
    int32_t rtn = 0;
    struct zxdh_vf_item   *vf_item;
    struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);

    // PLCR_FUNC_DBG_ENTER();

    if (dh_dev->coredev_type == DH_COREDEV_PF)
    {
        PLCR_LOG_DEBUG_DEV(dh_dev, "pf: pf_dev->vport = 0x%x, pf_dev->pcie_id = 0x%x\n", pf_dev->vport, pf_dev->pcie_id);
        //场景中有不需要指定vf_idx的情况，不会生成vf_id
        if (PLCR_INVALID_PARAM == vf_idx)
        {
            //pf队列限速和vf端口组字节限速，都不需要指定vf_idx
            //但vf端口组字节限速必须要用到vport, pf队列也需分配一个carb flowid
            *vport = pf_dev->vport;
            *vfid  = VQM_VFID(pf_dev->vport);

            PLCR_LOG_DEBUG_DEV(dh_dev, "vf_idx is not specified! vport = %x, vfid = %x\n", pf_dev->vport, *vfid);
            return rtn;
        }
        else
        {
            vf_item = &pf_dev->vf_item[vf_idx];
            if(ERR_PTR(-EINVAL) == vf_item)
            {
                return -EINVAL;
            }

            *vport = vf_item->vport;
            *vfid  = VQM_VFID(vf_item->vport);

            PLCR_LOG_DEBUG_DEV(dh_dev, "vf_idx = 0x%x, vf_item->vport = 0x%x, vfid = 0x%x\n", vf_idx, vf_item->vport, *vfid);
            PLCR_LOG_DEBUG_DEV(dh_dev, "mac address = %x %x %x %x %x %x\n", vf_item->mac[0],vf_item->mac[1],vf_item->mac[2],vf_item->mac[3],vf_item->mac[4],vf_item->mac[5]);
        }
    }
    else
    {
        PLCR_LOG_DEBUG_DEV(dh_dev, "vf: pf_dev->vport = 0x%x, pf_dev->pcie_id = 0x%x\n", pf_dev->vport, pf_dev->pcie_id);

        *vport = pf_dev->vport;
        *vfid  = VQM_VFID(pf_dev->vport);

        PLCR_LOG_DEBUG_DEV(dh_dev, "vf_idx = 0x%x, pf_dev->vport = 0x%x, vfid = 0x%x\n", vf_idx, pf_dev->vport, *vfid);
    }

    return rtn;
}

int zxdh_plcr_get_cars_flowid(struct zxdh_pf_device *pf_dev, E_RATE_LIMIT_MODE mode, zxdh_plcr_rate_limit_paras *rate_limit_paras, zxdh_plcr_flowids *flowids)
{
    int rtn = 0;

    // PLCR_FUNC_DBG_ENTER();

    //init the pointer flowids with invalid value
    memset(flowids, 0xff, sizeof(zxdh_plcr_flowids));

    //get car A flowid
    rtn = zxdh_plcr_get_car_a_flowid(pf_dev, mode, rate_limit_paras, flowids);
    PLCR_COMM_ASSERT(rtn);

    //get car B flowid
    rtn = zxdh_plcr_get_car_b_flowid(pf_dev, mode, rate_limit_paras, flowids);
    PLCR_COMM_ASSERT(rtn);

    //get car C flowid
    rtn = zxdh_plcr_get_car_c_flowid(pf_dev, mode, rate_limit_paras, flowids);
    PLCR_COMM_ASSERT(rtn);

    return rtn;
}

int zxdh_plcr_get_next_mode(struct zxdh_pf_device *pf_dev, zxdh_plcr_rate_limit_paras *rate_limit_paras, uint32_t *next_mode)
{
    int rtn = 0;
    E_RATE_LIMIT_MODE cur_mode;

    //get vport current mode
    rtn = zxdh_plcr_get_mode(pf_dev, rate_limit_paras->vport, &cur_mode);
    PLCR_COMM_ASSERT(rtn);

    if(E_RATE_LIMIT_MODE0 == cur_mode)
    {
        if(E_RATE_LIMIT_REQ_QUEUE_BYTE == rate_limit_paras->req_type)
        {
            *next_mode = E_RATE_LIMIT_MODE1;
        }
        else
        {
            *next_mode = E_RATE_LIMIT_MODE2;
        }

        return 0;
    }
    else
    {
        return -EINVAL;
    }

    return rtn;
}

int zxdh_plcr_init_flow(struct zxdh_pf_device *pf_dev, E_PLCR_CAR_TYPE car_type, uint32_t flowid)
{
    int rtn = 0;
    uint32_t vf_idx;
    uint32_t vport;
    uint32_t flowid_offset;
    uint32_t vfid;
    uint32_t map_flowid;
    union zxdh_msg *msg = NULL;
    DPP_PF_INFO_T pf_info = {0};
    struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    // PLCR_FUNC_DBG_ENTER();

    if (dh_dev->coredev_type == DH_COREDEV_PF)
    {
        vport = pf_dev->vport;
        flowid_offset = EPID(vport) * PLCR_CAR_C_FLOWIDS_PER_EP + FUNC_NUM(vport) * PLCR_CAR_C_FLOWIDS_PER_PF;

        if ((car_type == E_PLCR_CAR_C) && (flowid - flowid_offset > 1)) //group_id为0时，不进行检验，直接初始化；只有移动group才需要检验
        {
            //对于carC flow的初始化，需检查目标group中num_vfs是否为0，确定其是否是本次移动前新建
            for (vf_idx = 0; vf_idx < pf_dev->num_vfs; vf_idx ++)
            {
                map_flowid = 0xffff;
                rtn = zxdh_plcr_get_vport_vfid(pf_dev, vf_idx, &vport, &vfid);
                PLCR_COMM_ASSERT(rtn);

                if (0 == (flowid%2))
                {
                    rtn = zxdh_plcr_get_next_map(pf_dev, E_PLCR_CAR_B, vfid * 2, &map_flowid);
                }
                else
                {
                    rtn = zxdh_plcr_get_next_map(pf_dev, E_PLCR_CAR_B, vfid * 2 + 1, &map_flowid);
                }
                if ((!rtn) && (flowid == map_flowid))
                {
                    PLCR_LOG_DEBUG_DEV(dh_dev, "Group is currently being used by at least one VF\n");
                    return 0;
                }
            }
        }
        pf_info.slot = pf_dev->slot_id;
        pf_info.vport = pf_dev->vport;
        PLCR_LOG_DEBUG_DEV(dh_dev, "dpp_car_queue_cfg_set: vport = 0x%x, car_type = %d, flowid = %d, plcr_en = 0\n",
                        pf_dev->vport, car_type, flowid);
        rtn = dpp_car_queue_cfg_set(&pf_info, (uint32_t)car_type, flowid, DROP_DISABLE, PLCR_DISABLE, 0);
        if (rtn)
        {
            PLCR_LOG_ERR_DEV(dh_dev, "failed to call dpp_car_queue_cfg_set()\n");
        }
    }
    else
    {
        //针对vf队列限速的场景
        msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
        if (unlikely(NULL == msg))
        {
            PLCR_LOG_ERR_DEV(dh_dev, "failed to kzalloc\n");
            return -ENOMEM;
        }
        msg->payload.hdr.op_code = ZXDH_PLCR_FLOW_INIT;
        msg->payload.hdr.vport   = pf_dev->vport;
        msg->payload.hdr.pcie_id = pf_dev->pcie_id;
        msg->payload.hdr.vf_id   = pf_dev->pcie_id & (0xff);

        msg->payload.plcr_flow_init_msg.car_type = car_type;
        msg->payload.plcr_flow_init_msg.flowid = flowid;

        rtn = zxdh_pf_msg_send_cmd(dh_dev, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
        kfree(msg);
        PLCR_COMM_ASSERT(rtn);
    }

    return rtn;
}

/*
函数功能：根据不同的场景进行三级映射，下面枚举出所有场景。

    原始模式        限速请求        转换模式        映射

    mode0          队列字节限速    mode1          (car A -> car B) & (car B -> car C)
    mode0          端口字节限速    mode2          (car A -> car B) & (car B -> car C)
    mode0          端口包限速      mode2          (car A -> car B) & (car B -> car C)
    mode0          移动组         mode2          (car A -> car B) & (car B -> car C)

    mode1          队列字节限速    mode1          不需要
    mode1          端口字节限速    mode1          不需要
    mode1          移动组         mode1          (car B -> car C)

    mode2          端口包限速     mode2           不需要
    mode2          端口字节限速    mode2          不需要
    mode2          移动组         mode2          (car B -> car C)

    x              端口组字节限速  无端口，无模式    不需要映射，直接配置限速
*/
int zxdh_plcr_set_cars_map(struct zxdh_pf_device *pf_dev, zxdh_plcr_rate_limit_paras *rate_limit_paras, zxdh_plcr_flowids *flowids)
{
    int rtn = 0;
    uint32_t flowid;
    uint32_t map_flowid;
    E_RATE_LIMIT_MODE mode;
    E_RATE_LIMIT_REQ_TYPE req_type;
    int queue_pair_index;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    // PLCR_FUNC_DBG_ENTER();

    //get vport and vfid
    rtn = zxdh_plcr_get_vport_vfid(pf_dev, rate_limit_paras->vf_idx, &rate_limit_paras->vport, &rate_limit_paras->vfid);
    PLCR_COMM_ASSERT(rtn);

    //get vport current mode
    rtn = zxdh_plcr_get_mode(pf_dev, rate_limit_paras->vport, &mode);
    PLCR_COMM_ASSERT(rtn);

    //get rate limit type
    rtn = zxdh_plcr_check_req_type(pf_dev, mode, rate_limit_paras, &req_type);
    PLCR_COMM_ASSERT(rtn);

    //获取三级car的flowid
    rtn = zxdh_plcr_get_cars_flowid(pf_dev, mode, rate_limit_paras, flowids);
    PLCR_COMM_ASSERT(rtn);

    if ((E_RATE_LIMIT_MODE0 == mode) && (E_RATE_LIMIT_REQ_VF_GROUP_BYTE != req_type))
    {
        if (E_RATE_LIMIT_REQ_QUEUE_BYTE == req_type)
        {
            for(queue_pair_index = 0; queue_pair_index < flowids->queue_pairs; queue_pair_index++)
            {
                //rxq：初始化flow，避免有复位前的配置遗留，将car A flowid映射到car B flowid
                flowid     = flowids->flowids_A[0][queue_pair_index];
                map_flowid = flowids->flowid_B[0];
                PLCR_LOG_DEBUG_DEV(dh_dev, "car_type = 0x%x, flowid = 0x%x, map_flowid = 0x%x\n", E_PLCR_CAR_A, flowid, map_flowid);

                rtn = zxdh_plcr_init_flow(pf_dev, E_PLCR_CAR_A, flowid);
                PLCR_COMM_ASSERT(rtn);
                rtn = zxdh_plcr_map_flowid(pf_dev, E_PLCR_CAR_A, flowid, map_flowid);
                PLCR_COMM_ASSERT(rtn);


                //txq：初始化car A flow，映射到car B flowid
                flowid     = flowids->flowids_A[1][queue_pair_index];
                map_flowid = flowids->flowid_B[1];
                PLCR_LOG_DEBUG_DEV(dh_dev, "car_type = 0x%x, flowid = 0x%x, map_flowid = 0x%x\n", E_PLCR_CAR_A, flowid, map_flowid);

                rtn = zxdh_plcr_init_flow(pf_dev, E_PLCR_CAR_A, flowid);
                PLCR_COMM_ASSERT(rtn);
                rtn = zxdh_plcr_map_flowid(pf_dev, E_PLCR_CAR_A, flowid, map_flowid);
                PLCR_COMM_ASSERT(rtn);
            }
        }
        else if ((E_RATE_LIMIT_REQ_VF_PKT == req_type) || (E_RATE_LIMIT_REQ_VF_BYTE == req_type) || (E_RATE_LIMIT_REQ_MOVE_VF_GROUP == req_type))
        {
            //vfid's rx ：初始化car A flow，映射到car B flowid
            flowid = flowids->flowid_A[0];
            map_flowid = flowids->flowid_B[0];
            PLCR_LOG_DEBUG_DEV(dh_dev, "car_type = 0x%x, flowid = 0x%x, map_flowid = 0x%x\n", E_PLCR_CAR_A, flowid, map_flowid);

            rtn = zxdh_plcr_init_flow(pf_dev, E_PLCR_CAR_A, flowid);
            PLCR_COMM_ASSERT(rtn);
            rtn = zxdh_plcr_map_flowid(pf_dev, E_PLCR_CAR_A, flowid, map_flowid);
            PLCR_COMM_ASSERT(rtn);


            //vfid's tx ：初始化car A flow，映射到car B flowid
            flowid = flowids->flowid_A[1];
            map_flowid = flowids->flowid_B[1];
            PLCR_LOG_DEBUG_DEV(dh_dev, "car_type = 0x%x, flowid = 0x%x, map_flowid = 0x%x\n", E_PLCR_CAR_A, flowid, map_flowid);

            rtn = zxdh_plcr_init_flow(pf_dev, E_PLCR_CAR_A, flowid);
            PLCR_COMM_ASSERT(rtn);
            rtn = zxdh_plcr_map_flowid(pf_dev, E_PLCR_CAR_A, flowid, map_flowid);
            PLCR_COMM_ASSERT(rtn);
        }
    }

    /*
    功能：car B的flowid映射到car C的flowid
    场景：
        ---1.当前mode0模式，用户请求队列字节限速，  用户未指定group：group默认为0
        ---2.当前mode0模式，用户请求vf端口字节限速，用户未指定group：group默认为0
        ---3.当前mode0模式，用户请求vf端口包限速，  用户未指定group：group默认为0
        ---4.当前mode0模式，用户请求端口组字节限速， 用户指定group：不需要car B到car C的映射，直接设置限速

        ---5.任意模式，     用户请求移动group，    用户指定group：按照用户指定的group
        ---其它场景下：不需要进行映射
    */
    if (((E_RATE_LIMIT_MODE0 == mode) && (E_RATE_LIMIT_REQ_VF_GROUP_BYTE != req_type)) || (E_RATE_LIMIT_REQ_MOVE_VF_GROUP == req_type))
    {
        //car B rx flowid is mapped to car C rx flowid
        flowid = flowids->flowid_B[0];
        map_flowid = flowids->flowid_C[0];
        PLCR_LOG_DEBUG_DEV(dh_dev, "car_type = 0x%x, flowid = 0x%x, map_flowid = 0x%x\n", E_PLCR_CAR_B, flowid, map_flowid);

        if (E_RATE_LIMIT_MODE0 == mode)
        {
            rtn = zxdh_plcr_init_flow(pf_dev, E_PLCR_CAR_B, flowid);
            PLCR_COMM_ASSERT(rtn);
        }
        rtn = zxdh_plcr_init_flow(pf_dev, E_PLCR_CAR_C, map_flowid);
        PLCR_COMM_ASSERT(rtn);

        rtn = zxdh_plcr_map_flowid(pf_dev, E_PLCR_CAR_B, flowid, map_flowid);
        PLCR_COMM_ASSERT(rtn);


        //car B tx flowid is mapped to car C tx flowid
        flowid = flowids->flowid_B[1];
        map_flowid = flowids->flowid_C[1];
        PLCR_LOG_DEBUG_DEV(dh_dev, "car_type = 0x%x, flowid = 0x%x, map_flowid = 0x%x\n", E_PLCR_CAR_B, flowid, map_flowid);

        if (E_RATE_LIMIT_MODE0 == mode)
        {
            rtn = zxdh_plcr_init_flow(pf_dev, E_PLCR_CAR_B, flowid);
            PLCR_COMM_ASSERT(rtn);
        }
        rtn = zxdh_plcr_init_flow(pf_dev, E_PLCR_CAR_C, map_flowid);
        PLCR_COMM_ASSERT(rtn);

        rtn = zxdh_plcr_map_flowid(pf_dev, E_PLCR_CAR_B, flowid, map_flowid);
        PLCR_COMM_ASSERT(rtn);
        PLCR_LOG_DEBUG_DEV(dh_dev, "Successfull to map!\n");
    }

    return rtn;
}

int zxdh_plcr_unified_set_rate_limit(struct zxdh_pf_device *pf_dev,
                              zxdh_plcr_rate_limit_paras *rate_limit_paras)
{
    int rtn = 0;
    union zxdh_msg *msg = NULL;
    zxdh_plcr_flowids flowids;
    uint32_t next_mode;
    E_RATE_LIMIT_MODE cur_mode;
    E_RATE_LIMIT_REQ_TYPE req_type;
    //uint32_t vport     = 0;
    uint32_t flowid    = 0;
    uint32_t car_type  = 0;
    uint32_t is_packet = 0;
    uint32_t max_rate  = 0;
    uint32_t min_rate  = 0;
    struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    // PLCR_FUNC_DBG_ENTER();

    //对三级car进行映射关联
    rtn = zxdh_plcr_set_cars_map(pf_dev, rate_limit_paras, &flowids);
    PLCR_COMM_ASSERT(rtn);

    req_type = rate_limit_paras->req_type;

    //移动vf端口组：不需要设置限速，只需要级间映射（上面已经完成映射）；但是需要切换模式
    if (E_RATE_LIMIT_REQ_MOVE_VF_GROUP == req_type)
    {
        rtn = zxdh_plcr_get_mode(pf_dev, rate_limit_paras->vport, &cur_mode);

        //如果当前是模式0，且vf端口移动到非0 group，则切换到模式2
        if((E_RATE_LIMIT_MODE0 == cur_mode) && (0 != rate_limit_paras->group_id))
        {
            rtn = zxdh_plcr_set_mode(pf_dev, rate_limit_paras->vport, E_RATE_LIMIT_MODE2);
            PLCR_COMM_ASSERT(rtn);
        }
        //如果当前是非模式0移动到group 0，则要检查三级car上是否都没有限速，则切换到模式0
        else if((E_RATE_LIMIT_MODE0 != cur_mode) && (0 == rate_limit_paras->group_id))
        {
            zxdh_plcr_check_release_flow_chain(pf_dev, E_PLCR_CAR_B, rate_limit_paras->vport);;
        }

        return rtn;
    }//队列字节限速
    else if (E_RATE_LIMIT_REQ_QUEUE_BYTE == req_type)
    {
        if (E_RATE_LIMIT_RX == rate_limit_paras->direction)
        {
            flowid = flowids.flowids_A[0][rate_limit_paras->queue_id];
        }
        else
        {
            flowid = flowids.flowids_A[1][rate_limit_paras->queue_id];
        }

        max_rate = rate_limit_paras->max_rate;
        min_rate = 0;
        car_type  = E_PLCR_CAR_A;
        is_packet = E_RATE_LIMIT_BYTE;
    }//vf端口字节限速
    else if(E_RATE_LIMIT_REQ_VF_BYTE == req_type)
    {
        if (E_RATE_LIMIT_RX == rate_limit_paras->direction)
        {
            flowid = flowids.flowid_B[0];
        }
        else
        {
            flowid = flowids.flowid_B[1];
        }

        max_rate = rate_limit_paras->max_rate;
        min_rate = rate_limit_paras->min_rate;
        car_type  = E_PLCR_CAR_B;
        is_packet = E_RATE_LIMIT_BYTE;
    }//vf端口包限速
    else if(E_RATE_LIMIT_REQ_VF_PKT == req_type)
    {
        if (E_RATE_LIMIT_RX == rate_limit_paras->direction)
        {
            flowid = flowids.flowid_A[0];
        }
        else
        {
            flowid = flowids.flowid_A[1];
        }

        max_rate = rate_limit_paras->max_rate;
        min_rate = rate_limit_paras->min_rate;
        car_type  = E_PLCR_CAR_A;
        is_packet = E_RATE_LIMIT_PACKET;
    }//端口组字节限速
    else if(E_RATE_LIMIT_REQ_VF_GROUP_BYTE == req_type)
    {
        if (E_RATE_LIMIT_RX == rate_limit_paras->direction)
        {
            flowid = flowids.flowid_C[0];
        }
        else
        {
            flowid = flowids.flowid_C[1];
        }

        max_rate = rate_limit_paras->max_rate;
        min_rate = 0;
        car_type  = E_PLCR_CAR_C;
        is_packet = E_RATE_LIMIT_BYTE;
    }
    else
    {
        return -EINVAL;
    }

    PLCR_LOG_DEBUG_DEV(dh_dev, "rate_limit_paras vport    = 0x%x\n", rate_limit_paras->vport);
    PLCR_LOG_DEBUG_DEV(dh_dev, "rate_limit_paras vfid    = 0x%x\n", rate_limit_paras->vfid);
    PLCR_LOG_DEBUG_DEV(dh_dev, "flowid    = 0x%x\n", flowid);
    PLCR_LOG_DEBUG_DEV(dh_dev, "car_type  = 0x%x\n", car_type);
    PLCR_LOG_DEBUG_DEV(dh_dev, "is_packet = 0x%x\n", is_packet);
    PLCR_LOG_DEBUG_DEV(dh_dev, "max_rate  = 0x%x\n", max_rate);
    PLCR_LOG_DEBUG_DEV(dh_dev, "min_rate  = 0x%x\n", min_rate);

    //set rate limit
    if (dh_dev->coredev_type == DH_COREDEV_PF)
    {
        rtn = zxdh_plcr_set_rate_limit(pf_dev, is_packet, car_type, rate_limit_paras->vport, flowid, max_rate, min_rate);
    }
    else if (dh_dev->coredev_type == DH_COREDEV_VF)
    {
        msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
        if (unlikely(NULL == msg))
        {
            PLCR_LOG_ERR_DEV(dh_dev, "failed to kzalloc\n");
            return -ENOMEM;
        }
        msg->payload.hdr.op_code = ZXDH_VF_RATE_LIMIT_SET;
        msg->payload.hdr.vport   = pf_dev->vport;
        msg->payload.hdr.pcie_id = pf_dev->pcie_id;
        msg->payload.hdr.vf_id   = pf_dev->pcie_id & (0xff);

        msg->payload.rate_limit_set_msg.flowid    = flowid;
        msg->payload.rate_limit_set_msg.car_type  = car_type;
        msg->payload.rate_limit_set_msg.is_packet = is_packet;
        msg->payload.rate_limit_set_msg.max_rate  = max_rate;
        msg->payload.rate_limit_set_msg.min_rate  = min_rate;

        PLCR_LOG_DEBUG_DEV(dh_dev, "rate_limit_set_msg.flowid    = 0x%x\n", msg->payload.rate_limit_set_msg.flowid);
        PLCR_LOG_DEBUG_DEV(dh_dev, "rate_limit_set_msg.car_type  = 0x%x\n", msg->payload.rate_limit_set_msg.car_type);
        PLCR_LOG_DEBUG_DEV(dh_dev, "rate_limit_set_msg.is_packet = 0x%x\n", msg->payload.rate_limit_set_msg.is_packet);
        PLCR_LOG_DEBUG_DEV(dh_dev, "rate_limit_set_msg.max_rate  = 0x%x\n", msg->payload.rate_limit_set_msg.max_rate);
        PLCR_LOG_DEBUG_DEV(dh_dev, "rate_limit_set_msg.min_rate  = 0x%x\n", msg->payload.rate_limit_set_msg.min_rate);

        rtn = zxdh_pf_msg_send_cmd(dh_dev, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
        if (rtn)
        {
            PLCR_LOG_ERR_DEV(dh_dev, "failed and rtn=%d\n", rtn);
            kfree(msg);
            return rtn;
        }

        rtn = msg->reps.rate_limit_set_rsp.err_code;
        kfree(msg);
    }
    //0：限速设置成功
    //PLCR_REMOVE_RATE_LIMIT：解除限速成功
    //其它值：错误码
    if (rtn && (PLCR_REMOVE_RATE_LIMIT != rtn))
    {
        PLCR_LOG_ERR_DEV(dh_dev, "failed and rtn=%d\n", rtn);
        return (rtn == PLCR_DUPLICATE_RATE) ? 0 : -EPERM;
    }

    /*************下面2种场景需要模式切换*************
     * 1. mode 0 -> mode 1
     * 2. mode 0 -> mode 2
     * 3. vport处于mode 1或者mode 2保持不变，只有在解除限速的时候可能会跳转到mode 0
     * 4. 设置端口组字节限速时，不需要进行模式切换（端口组不直接与pf或vf关联）
    ****************************************/
    if ((0 == rtn) && (E_RATE_LIMIT_REQ_VF_GROUP_BYTE != req_type))
    {
        rtn = zxdh_plcr_get_next_mode(pf_dev, rate_limit_paras, &next_mode);
        if(0 == rtn)
        {
            rtn = zxdh_plcr_set_mode(pf_dev, rate_limit_paras->vport, next_mode);
            PLCR_COMM_ASSERT(rtn);
        }
    }

    return 0;

}
EXPORT_SYMBOL(zxdh_plcr_unified_set_rate_limit);

static int zxdh_vqm_send_rate_msg(struct zxdh_pf_device *pf_dev, uint16_t vqm_vfid, void *in_payload, uint16_t in_len, struct bar_recv_msg *out)
{
    int rtn = 0;
    uint16_t pcie_id = 0;
    struct zxdh_pci_bar_msg in = {0};
    struct zxdh_msg_recviver_mem result = {0};
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    pcie_id = FIND_VF_PCIE_ID(pf_dev->pcie_id, vqm_vfid);

    in.virt_addr = (uint64_t)ZXDH_BAR_MSG_BASE(pf_dev->pci_ioremap_addr[0]);
    in.payload_addr = in_payload;
    in.payload_len = in_len;
    in.emec = 0;
    in.src = MSG_CHAN_END_PF;
    in.dst = MSG_CHAN_END_RISC;
    in.event_id = VQM_BAR_MSG;
    in.src_pcieid = pcie_id;
    in.dst_pcieid = 0,
	
    result.recv_buffer = (void *)out;
    result.buffer_len = sizeof(struct bar_recv_msg);
    rtn = zxdh_bar_chan_sync_msg_send(&in, &result);
    if (rtn != BAR_MSG_OK)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "zxdh_vqm_send_rate_msg failed\n");
    }
    return BAR_MSG_OK;
}

int zxdh_vqm_vf_set_rate_limit(struct zxdh_pf_device *pf_dev, uint16_t vqm_vfid, uint32_t vf_rate)
{
    int rtn = 0;
    uint32_t index  = 0;
    struct zxdh_vqm_param param = {0};
    struct bar_recv_msg *recv_msg = NULL;
    struct dh_core_dev *dh_dev = NULL;

    if (NULL == pf_dev)
    {
        PLCR_LOG_ERR("pf_dev NULL ptr\n");
        return -1;
    }

    dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);
    index = VQM_VFID(vqm_vfid);

    param.vqm_vfid = index, 
    param.opcode = OPCODE_SET, 
    param.cmd = CMD_VF_QOS,
    param.vqm_rate.pack_rate  = 0,
    param.vqm_rate.rate = (uint32_t)(((uint64_t)1000 * vf_rate * 106)/100);//配置上浮6%，提高整体精度

    recv_msg = kzalloc(sizeof(struct bar_recv_msg), GFP_KERNEL);
    if (NULL == recv_msg)
    {
        PLCR_LOG_ERR_DEV(dh_dev, "recv_msg NULL ptr\n");
        return -1;
    }

    rtn = zxdh_vqm_send_rate_msg(pf_dev, vqm_vfid, &param, (uint16_t)sizeof(param) ,recv_msg);
    if (0 != rtn)
    {
        //考虑兼容性,返回值错误，不影响plcr限速，限速系统正常工作
        PLCR_LOG_ERR_DEV(dh_dev, "zxdh_vqm_send_rate_msg failed\n");
    }

    PLCR_LOG_DEBUG_DEV(dh_dev, "The Rate of VF vfid:0x%x index: %d has been set to: Max Tx Rate: %dMbit/s\n",
                vqm_vfid, index ,vf_rate);

    return 0;
}
EXPORT_SYMBOL(zxdh_vqm_vf_set_rate_limit);
